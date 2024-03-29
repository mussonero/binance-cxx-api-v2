/*
	Author: tensaix2j
	Date  : 2017/10/15

	C++ library for Binance API.
*/
#include <libwebsockets.h>
#include "binance_websocket.h"
#include "binance_logger.h"

#include <atomic>
#include <csignal>

#if defined(LWS_WITH_LIBUV_INTERNAL)
#include <uv.h>
#endif

using namespace binance;
using namespace std;

#if defined(LWS_WITH_MBEDTLS) || defined(LWS_WITH_WOLFSSL)
/*  MbedTLS / WolfSSL force trust this CA explicitly. */
static const char *const sslRootsCA =
    "-----BEGIN CERTIFICATE-----\n"
    "MIIDrzCCApegAwIBAgIQCDvgVpBCRrGhdWrJWZHHSjANBgkqhkiG9w0BAQUFADBh\n"
    "MQswCQYDVQQGEwJVUzEVMBMGA1UEChMMRGlnaUNlcnQgSW5jMRkwFwYDVQQLExB3\n"
    "d3cuZGlnaWNlcnQuY29tMSAwHgYDVQQDExdEaWdpQ2VydCBHbG9iYWwgUm9vdCBD\n"
    "QTAeFw0wNjExMTAwMDAwMDBaFw0zMTExMTAwMDAwMDBaMGExCzAJBgNVBAYTAlVT\n"
    "MRUwEwYDVQQKEwxEaWdpQ2VydCBJbmMxGTAXBgNVBAsTEHd3dy5kaWdpY2VydC5j\n"
    "b20xIDAeBgNVBAMTF0RpZ2lDZXJ0IEdsb2JhbCBSb290IENBMIIBIjANBgkqhkiG\n"
    "9w0BAQEFAAOCAQ8AMIIBCgKCAQEA4jvhEXLeqKTTo1eqUKKPC3eQyaKl7hLOllsB\n"
    "CSDMAZOnTjC3U/dDxGkAV53ijSLdhwZAAIEJzs4bg7/fzTtxRuLWZscFs3YnFo97\n"
    "nh6Vfe63SKMI2tavegw5BmV/Sl0fvBf4q77uKNd0f3p4mVmFaG5cIzJLv07A6Fpt\n"
    "43C/dxC//AH2hdmoRBBYMql1GNXRor5H4idq9Joz+EkIYIvUX7Q6hL+hqkpMfT7P\n"
    "T19sdl6gSzeRntwi5m3OFBqOasv+zbMUZBfHWymeMr/y7vrTC0LUq7dBMtoM1O/4\n"
    "gdW7jVg/tRvoSSiicNoxBN33shbyTApOB6jtSj1etX+jkMOvJwIDAQABo2MwYTAO\n"
    "BgNVHQ8BAf8EBAMCAYYwDwYDVR0TAQH/BAUwAwEB/zAdBgNVHQ4EFgQUA95QNVbR\n"
    "TLtm8KPiGxvDl7I90VUwHwYDVR0jBBgwFoAUA95QNVbRTLtm8KPiGxvDl7I90VUw\n"
    "DQYJKoZIhvcNAQEFBQADggEBAMucN6pIExIK+t1EnE9SsPTfrgT1eXkIoyQY/Esr\n"
    "hMAtudXH/vTBH1jLuG2cenTnmCmrEbXjcKChzUyImZOMkXDiqw8cvpOp/2PV5Adg\n"
    "06O/nVsJ8dWO41P0jmP6P6fbtGbfYmbW0W5BjfIttep3Sp+dWOIrWcBAI+0tKIJF\n"
    "PnlUkiaY4IBIqDfv8NZ5YBberOgOzW6sRBc4L0na4UU+Krk2U886UAb3LujEV0ls\n"
    "YSEY1QSteDwsOoBrp+uvFRTp2InBuThs4pFsiv9kuXclVzDAGySj4dzp30d8tbQk\n"
    "CAUw7C29C79Fv1C5qfPrmAESrciIxpg0X40KPMbp1ZWVbd4=\n"
    "-----END CERTIFICATE-----\n";
#endif

static struct lws_context *context;
static struct lws_context_creation_info info;
static atomic<int> protocol_init(0);
static atomic<int> lws_service_cancelled(0);
static int force_create_ccinfo(const std::string &path);
static uv_signal_t sigint_watcher;

/*
 * This "contains" the endpoint connection property and has
 * the connection bound to it
 */
struct endpoint_connection {
  struct lws *wsi;         /* related wsi if any */
  uint16_t retry_count; /* count of consecutive retries */
  CB json_cb;
  std::string ws_path;
  atomic<bool> close_conn;
  atomic<bool> creating_conn;
};

static std::map<std::string, endpoint_connection> endpoints_prop;
static pthread_mutex_t lock_concurrent; /* serialize access */

/*
 * The retry and backoff policy we want to use for our client connections
 */
static const uint32_t backoff_ms[] = {1000 * 4, 1000 * 5, 1000 * 6, 1000 * 7, 1000 * 8, 1000 * 9};

/*
 * This struct sets the policy for delays between retries,
 * and for how long a connection may be 'idle'
 * before it first tries to ping / pong on it to confirm it's up,
 * or drops the connection if still idle.
 */
static const lws_retry_bo_t retry = {
    .retry_ms_table            = backoff_ms,
    .retry_ms_table_count        = LWS_ARRAY_SIZE(backoff_ms),
    .conceal_count            = LWS_ARRAY_SIZE(backoff_ms),
    .secs_since_valid_ping        = 60 * 3, /* force PINGs after secs idle */
    .secs_since_valid_hangup    = 60 * 10, /* hangup after secs idle */
    .jitter_percent            = 30,
    /*
     * jitter_percent controls how much additional random delay is
     * added to the actual interval to be used, default 30
     */
};

static int event_cb(lws *wsi, enum lws_callback_reasons reason, void *user, void *in, size_t len);

struct lws_protocols protocols[] =
    {
        {
            .name = "binance-websocket-api",
            .callback = event_cb,
            .per_session_data_size = sizeof(struct endpoint_connection),
            .rx_buffer_size = 128 * 1024,
            .id = 0,
            .tx_packet_size = 128 * 1024
        },

        LWS_PROTOCOL_LIST_TERM
    };

static int event_cb(lws *wsi, enum lws_callback_reasons reason, void *user, void *in, size_t len) {

  //struct endpoint_connection *current_data_old = static_cast< endpoint_connection *>(user);
  struct endpoint_connection *current_session = static_cast<endpoint_connection *>(lws_get_opaque_user_data(wsi));

  switch (reason) {

  case LWS_CALLBACK_PROTOCOL_INIT:
  case LWS_CALLBACK_OPENSSL_LOAD_EXTRA_CLIENT_VERIFY_CERTS:
    atomic_store(&protocol_init, 1);
    lwsl_user("%s: PROTOCOL_INIT OKAY, reason:%d\n",
              __func__, reason);
    break;

  case LWS_CALLBACK_WSI_CREATE:
    if (!lws_service_cancelled && current_session) {
      const std::string ws_path = current_session->ws_path;
      if (!ws_path.empty() && ws_path.find("/ws/") != std::string::npos) {
        if (!current_session->close_conn.load()) {
          pthread_mutex_lock(&lock_concurrent);
          current_session->wsi = wsi;
          lwsl_user("%s: create wsi for current_session#:%s\n",
                    __func__, ws_path.c_str());
          pthread_mutex_unlock(&lock_concurrent);
        }
      }
    }
    break;

  case LWS_CALLBACK_CLIENT_ESTABLISHED:
    if (!lws_service_cancelled && current_session) {
      const std::string ws_path = current_session->ws_path;
      if (!ws_path.empty() && ws_path.find("/ws/") != std::string::npos) {
        if (!current_session->close_conn.load()) {
          pthread_mutex_lock(&lock_concurrent);
          lws_callback_on_writable(wsi);
          current_session->wsi = wsi;
          lwsl_user("%s: connection established with success current_session#:%s\n",
                    __func__, ws_path.c_str());
          pthread_mutex_unlock(&lock_concurrent);
        }
      }

    }
    break;

  case LWS_CALLBACK_CLIENT_RECEIVE:
    if (!lws_service_cancelled && current_session) {
      const std::string ws_path = current_session->ws_path;
      if (!ws_path.empty() && ws_path.find("/ws/") != std::string::npos) {
        if (!current_session->close_conn.load()) {
          pthread_mutex_lock(&lock_concurrent);
          string str_result = string(reinterpret_cast<const char *>(in), len);
          Json::Value json_result;
          JSONCPP_STRING err;
          Json::CharReaderBuilder builder;
          const std::unique_ptr<Json::CharReader> reader(builder.newCharReader());
          if (!reader->parse(str_result.c_str(), str_result.c_str() + str_result.length(), &json_result,
                             &err)) {
            lwsl_err("%s: LWS_CALLBACK_CLIENT_RECEIVE Error Json:%s\n",
                     __func__, err.c_str());
            json_result.clear();
            pthread_mutex_unlock(&lock_concurrent);
            break;
          }
          current_session->json_cb(json_result);
          current_session->retry_count = 0;
          json_result.clear();
          pthread_mutex_unlock(&lock_concurrent);
        }
        break;
      }
      break;
    }
    break;

  case LWS_CALLBACK_CLOSED :
  case LWS_CALLBACK_CLIENT_WRITEABLE:
    if (in != nullptr) {
      lwsl_err("case LWS_CALLBACK_reason:%d  ERROR: %s\n", reason, (char *) in);
    }
    break;

  case LWS_CALLBACK_CLIENT_CLOSED:
  case LWS_CALLBACK_CLIENT_CONNECTION_ERROR:
  case LWS_CALLBACK_CLOSED_CLIENT_HTTP:
  case LWS_CALLBACK_WSI_DESTROY:
    if (in != nullptr) {
      lwsl_err("case reason:%d  ERROR: %s\n", reason, (char *) in);
    }

    if (!lws_service_cancelled && current_session) {
      const std::string ws_path = current_session->ws_path;
      if (!ws_path.empty() && ws_path.find("/ws/") != std::string::npos) {
        if (current_session->close_conn.load() && !current_session->creating_conn.load()) {
          pthread_mutex_lock(&lock_concurrent);
          current_session->wsi = nullptr;
          current_session->ws_path.clear();
          lws_set_opaque_user_data(wsi, nullptr);
          lwsl_err("reason:%d  deleted: %s : %s\n", reason,
                   in ? (char *) in : "(null)", ws_path.c_str());
          pthread_mutex_unlock(&lock_concurrent);
        } else if (!current_session->close_conn.load() && !current_session->creating_conn.load()) {
          pthread_mutex_lock(&lock_concurrent);
          current_session->retry_count++;
          current_session->wsi = nullptr;
          pthread_mutex_unlock(&lock_concurrent);
          if (current_session->retry_count > (LWS_ARRAY_SIZE(backoff_ms)) || force_create_ccinfo(ws_path)) {
            pthread_mutex_lock(&lock_concurrent);
            current_session->wsi = nullptr;
            current_session->ws_path.clear();
            lws_set_opaque_user_data(wsi, nullptr);
            lws_cancel_service(lws_get_context(wsi));
            atomic_store(&lws_service_cancelled, 1);
            pthread_mutex_unlock(&lock_concurrent);
#if defined(LWS_WITH_LIBUV_INTERNAL)
            uv_stop(uv_default_loop());
            uv_loop_close(uv_default_loop());
#endif
            return -1;
          }
        }
      } else {
        /*unknown*/
        lwsl_user("case reason:%d  ERROR unknown WSI: %s\n", reason,
                  in ? (char *) in : "(null)");
      }
    }
    break;
  case LWS_CALLBACK_GET_THREAD_ID: {
#ifdef __APPLE__
    // On OS X pthread_threadid_np() is used, as pthread_self() returns a structure.
    // Note the _np suffix suggests that it is an extension to POSIX.
    uint64_t tid;
    pthread_threadid_np(NULL, &tid);
#else
    auto tid = pthread_self();
#endif
    return (int) (uint64_t) tid;
  }
    break;
  default :
    // Make compiler happy regarding unhandled enums.
    break;
  }

  if (lws_service_cancelled) {
    return -1;
  }
  return lws_callback_http_dummy(wsi, reason, user, in, len);
}

#if defined(LWS_WITH_LIBUV_INTERNAL)
static void signal_cb(uv_signal_t *handle, int sig) {
  Logger::write_log("<binance::Websocket::sigint_handler> Interactive attention signal : %d\n", sig);
  uv_close((uv_handle_t *) &sigint_watcher, nullptr);
  pthread_mutex_lock(&lock_concurrent);
  atomic_store(&lws_service_cancelled, 1);
  pthread_mutex_unlock(&lock_concurrent);
  if (context) {
    lws_context_destroy(context);
    context = nullptr;
  }
  uv_stop(uv_default_loop());
}
#else
static void sigint_handler(int sig) {
  Logger::write_log("<binance::Websocket::sigint_handler> Interactive attention signal : %d\n", sig);
  lwsl_warn("exiting...\n");
  pthread_mutex_lock(&lock_concurrent);
  atomic_store(&lws_service_cancelled, 1);
  pthread_mutex_unlock(&lock_concurrent);
}
#endif

/* do not use pthread_mutex_lock on force_create_ccinfo*/
static int force_create_ccinfo(const std::string &path) {

  while (endpoints_prop.at(path).creating_conn.load()) {
    if (!endpoints_prop.at(path).creating_conn.load())
      break;
    if (!context || lws_service_cancelled)
      return 1;
  }

  atomic_store(&endpoints_prop.at(path).creating_conn, true);
  atomic_store(&endpoints_prop.at(path).close_conn, false);

  struct lws_client_connect_info ccinfo{};
  memset(&ccinfo, 0, sizeof(ccinfo));
  ccinfo.context = context;
  ccinfo.port = BINANCE_WS_PORT;
  ccinfo.address = BINANCE_WS_HOST;
  ccinfo.path = endpoints_prop.at(path).ws_path.c_str();
  ccinfo.host = ccinfo.address;
  ccinfo.origin = ccinfo.address;
  ccinfo.ssl_connection = LCCSCF_USE_SSL | LCCSCF_ALLOW_SELFSIGNED |
      LCCSCF_SKIP_SERVER_CERT_HOSTNAME_CHECK | LCCSCF_ALLOW_INSECURE |
      LCCSCF_PIPELINE | LCCSCF_PRIORITIZE_READS |
      LCCSCF_WAKE_SUSPEND__VALIDITY | LCCSCF_ALLOW_EXPIRED;
  ccinfo.protocol = protocols[0].name;
  ccinfo.local_protocol_name = protocols[0].name;
  ccinfo.retry_and_idle_policy = &retry;
  ccinfo.opaque_user_data = &endpoints_prop.at(path);
  //ccinfo.userdata = &endpoints_prop.at(path);
  ccinfo.pwsi = &endpoints_prop.at(path).wsi;

  if (!lws_client_connect_via_info(&ccinfo)) {
    lwsl_err("%s: Failed :%s\n",
             __func__, endpoints_prop.at(path).ws_path.c_str());
    atomic_store(&endpoints_prop.at(path).creating_conn, false);
    atomic_store(&endpoints_prop.at(path).close_conn, true);
    /*try cancel the service*/
    atomic_store(&lws_service_cancelled, 1);
    return 1;
  }
  atomic_store(&endpoints_prop.at(path).creating_conn, false);
  atomic_store(&endpoints_prop.at(path).close_conn, false);
  lwsl_user("%s: success, original_path#:%s retry_count[%d] ws_path::%s\n",
            __func__, path.c_str(), endpoints_prop.at(path).retry_count, endpoints_prop.at(path).ws_path.c_str());
  return 0;
}

/* do not use pthread_mutex_lock on force_delete_ccinfo*/
static void force_delete_ccinfo(const std::string &path) {

  if (endpoints_prop.find(path) != endpoints_prop.end()) {
    lwsl_info("%s: found connect_endpoints ws_path::%s\n",
              __func__, endpoints_prop.at(path).ws_path.c_str());
    while (endpoints_prop.at(path).creating_conn.load()) {
      if (!endpoints_prop.at(path).creating_conn.load())
        break;
      if (!context || lws_service_cancelled)
        return;
    }
    if (!lws_service_cancelled && endpoints_prop.at(path).wsi && !endpoints_prop.at(path).close_conn.load()) {
      pthread_mutex_lock(&lock_concurrent);
      atomic_store(&endpoints_prop.at(path).creating_conn, false);
      atomic_store(&endpoints_prop.at(path).close_conn, true);
      pthread_mutex_unlock(&lock_concurrent);
      /*use Async Kill*/
      lws_set_timeout(endpoints_prop.at(path).wsi, PENDING_TIMEOUT_CLOSE_SEND, LWS_TO_KILL_ASYNC);
    }
  } else {
    lwsl_err("%s: not found connect_endpoints error path::%s\n",
             __func__, path.c_str());
  }
}

void binance::Websocket::kill_all() {
#if defined(LWS_WITH_LIBUV_INTERNAL)
  pthread_mutex_lock(&lock_concurrent);
  atomic_store(&lws_service_cancelled, 1);
  pthread_mutex_unlock(&lock_concurrent);
  uv_stop(uv_default_loop());
  uv_loop_close(uv_default_loop());
#else
  pthread_mutex_lock(&lock_concurrent);
  atomic_store(&lws_service_cancelled, 1);
  if(context) {
    lws_context_destroy(context);
    context = nullptr;
  }
  pthread_mutex_unlock(&lock_concurrent);
#endif
}

void binance::Websocket::init() {
  pthread_mutex_init(&lock_concurrent, nullptr);
  endpoints_prop.clear();

  //struct lws_context_creation_info info{};

  memset(&info, 0, sizeof(info));
#if defined(LWS_WITH_LIBUV_INTERNAL)
  info.foreign_loops = (void *[]) {uv_default_loop()};
#else
  signal(SIGINT, sigint_handler);
#endif
  info.port = CONTEXT_PORT_NO_LISTEN;
  info.protocols = protocols;
  info.gid = -1;
  info.uid = -1;
  info.fd_limit_per_thread = 1024;
  info.max_http_header_pool = 1024;
  // This option is needed here to imply LWS_SERVER_OPTION_DO_SSL_GLOBAL_INIT
  // option, which must be set on newer versions of SSL_Libs.
  info.options = LWS_SERVER_OPTION_DO_SSL_GLOBAL_INIT | LWS_SERVER_OPTION_REQUIRE_VALID_OPENSSL_CLIENT_CERT;

#if defined(LWS_WITH_LIBUV_INTERNAL)
  info.options |= LWS_SERVER_OPTION_LIBUV;
  info.pcontext = &context;
#endif

#if defined(LWS_WITH_MBEDTLS) || defined(LWS_WITH_WOLFSSL)
  /* MbedTLS / WolfSSL, load the certificate. */
  info.client_ssl_ca_filepath = nullptr;
  info.client_ssl_ca_mem = sslRootsCA;
  info.client_ssl_ca_mem_len = (unsigned int) strlen(sslRootsCA);
#endif

  context = lws_create_context(&info);
  if (!context) {
    lwsl_err("lws init failed\n");
    atomic_store(&lws_service_cancelled, 1);
    pthread_mutex_destroy(&lock_concurrent);
    return;
  } else {
    atomic_store(&lws_service_cancelled, 0);
#if defined(LWS_WITH_LIBUV_INTERNAL)
    uv_signal_init(uv_default_loop(), &sigint_watcher);
    uv_signal_start(&sigint_watcher, signal_cb, SIGINT);
    uv_run(uv_default_loop(), UV_RUN_NOWAIT);
#endif
    while (!protocol_init.load()) {
      if (protocol_init.load())
        break;
      if (lws_service_cancelled) {
        lwsl_err("lws init failed\n");
        return;
      }
    }
  }
}

// Register call backs
void binance::Websocket::connect_endpoint(CB cb, const std::string &path) {

  while (!protocol_init.load()) {
    if (protocol_init.load() && context)
      break;
    sleep(1);
    if (!context || lws_service_cancelled) {
      lwsl_err("lws init/connect_endpoint failed\n");
      return;
    }
  }

  if (endpoints_prop.size() > 1024) {
    lwsl_err("%s: maximum of 1024 connect_endpoints reached,\n",
             __func__);
    return;
  }
  if (!lws_service_cancelled && context && protocol_init.load()) {
    pthread_mutex_lock(&lock_concurrent);
    endpoints_prop[path].ws_path.clear();
    endpoints_prop[path].json_cb = cb;
    endpoints_prop[path].retry_count = 0;
    endpoints_prop[path].wsi = nullptr;
    endpoints_prop[path].creating_conn = false;
    endpoints_prop[path].close_conn = true;
    endpoints_prop[path].ws_path = path;
    pthread_mutex_unlock(&lock_concurrent);
    int n = force_create_ccinfo(path);
    lwsl_user("%s: connecting::%s connect result[%s],\n",
              __func__, path.c_str(), n ? "NotOkay" : "Okay");
  } else {
    lwsl_err("%s: no service running,\n",
             __func__);
    return;
  }
}

// Unregister call backs
void binance::Websocket::disconnect_endpoint(const std::string &path) {

  if (endpoints_prop.empty()) {
    lwsl_err("%s: error connect_endpoints is empty,\n",
             __func__);
    return;
  }
  if (!lws_service_cancelled && context && protocol_init.load()) {
    force_delete_ccinfo(path);
  } else {
    lwsl_err("%s: no service running,\n",
             __func__);
    return;
  }
}

// Entering event loop
void binance::Websocket::enter_event_loop(const std::chrono::hours &hours) {
  auto start = std::chrono::steady_clock::now();
  auto end = start + hours;
  auto n = 0;
  lwsl_info("%s: INIT\n", __func__);
  do {
    try {
      if (context) {
        n = lws_service(context, 500);
      }
      if (lws_service_cancelled)
        break;
    } catch (exception &e) {
      lwsl_err("%s:::%s\n",
               __func__, e.what());
      break;
    }
  } while (n >= 0 && !lws_service_cancelled && std::chrono::steady_clock::now() < end);

  /* extra ensure the service is canceled */
  pthread_mutex_lock(&lock_concurrent);
  atomic_store(&lws_service_cancelled, 1);
  atomic_store(&protocol_init, 0);
  pthread_mutex_unlock(&lock_concurrent);
  if (context) {
    /* extra check if not null */
    lws_context_destroy(context);
    context = nullptr;
  }
  uv_stop(uv_default_loop());
  uv_loop_close(uv_default_loop());

  pthread_mutex_destroy(&lock_concurrent);
  endpoints_prop.clear();
  lwsl_info("%s: ENDED\n", __func__);
}
