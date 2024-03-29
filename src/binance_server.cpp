/*
	Author: tensaix2j
	Date  : 2017/10/15

	C++ library for Binance API.
*/

#include "binance.h"
#include "binance_logger.h"
#include "binance_utils.h"
#include <wolfssl/openssl/evp.h>
#include <wolfssl/openssl/sha.h>

using namespace binance;
using namespace std;

binance::Server::Server(const char* hostname_, bool simulation_) : hostname(hostname_), simulation(simulation_) { }

const std::string& binance::Server::getHostname() const { return hostname; }

bool binance::Server::isSimulator() const { return simulation; }

// GET /api/v3/time
binanceError_t binance::Server::getTime(Json::Value &json_result)
{
	binanceError_t status = binanceSuccess;

	Logger::write_log("<get_serverTime>");

	string url(hostname);
	url += "/api/v3/time";

	string str_result;
	getCurl(str_result, url);

	if (str_result.size() == 0)
		status = binanceErrorEmptyServerResponse;
	else
	{
		try
		{
			json_result.clear();
			JSONCPP_STRING err;
			Json::CharReaderBuilder builder;
			const std::unique_ptr<Json::CharReader> reader(builder.newCharReader());
			if (!reader->parse(str_result.c_str(), str_result.c_str() + str_result.length(), &json_result,
							   &err)) {
				Logger::write_log("<get_serverTime> Error ! %s", err.c_str());
				status = binanceErrorParsingServerResponse;
				return status;
			}
			CHECK_SERVER_ERR(json_result);
		}
		catch (exception &e)
		{
		 	Logger::write_log("<get_serverTime> Error ! %s", e.what());
			status = binanceErrorParsingServerResponse;
		}
	}

	Logger::write_log("<get_serverTime> Done.");

	return status;
}

// Curl's callback
static size_t getCurlCb(void *content, size_t size, size_t nmemb, std::string *buffer)
{
	Logger::write_log("<curl_cb> ");

	size_t newLength = size * nmemb;
	size_t oldLength = buffer->size();

	buffer->resize(oldLength + newLength);

	std::copy((char*)content, (char*)content + newLength, buffer->begin() + oldLength);

	Logger::write_log("<curl_cb> Done.");

	return newLength;
}

#if defined(LWS_WITH_OPENSSL) || defined(LWS_WITH_WOLFSSL)
/* MbedTLS / WolfSSL force trust this CA explicitly. */
static const char * const sslRootsCA =
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

static CURLcode sslctx_function(CURL *curl, void *sslctx, void *parm)
{
  X509_STORE *store;
  X509 *cert = NULL;
  BIO *bio;
  char *mypem = static_cast<char *>(parm);
  /* get a BIO */
  bio = BIO_new_mem_buf(mypem, -1);
  /* use it to read the PEM formatted certificate from memory into an X509 structure that SSL can use */
  PEM_read_bio_X509(bio, &cert, 0, NULL);
  if(cert == NULL)
    Logger::write_log("<curl_api> PEM_read_bio_X509 failed...\n");

  /* get a pointer to the X509 certificate store (which may be empty) */
  store = SSL_CTX_get_cert_store((SSL_CTX *)sslctx);

  /* add our certificate to this store */
  if(X509_STORE_add_cert(store, cert) == 0)
    Logger::write_log("<curl_api> error adding certificate..\n");

  /* decrease reference counts */
  X509_free(cert);
  BIO_free(bio);

  /* all set to go */
  return CURLE_OK;
}
#endif

binanceError_t binance::Server::getCurl(string& result_json, const string& url)
{
	vector<string> v;
	string action = "GET";
	string post_data = "";
	return getCurlWithHeader(result_json, url, v, post_data, action);
}

class SmartCURL
{
	CURL* curl;

public :

	CURL* get() { return curl; }

	SmartCURL()
	{
		curl = curl_easy_init();
        curl_easy_setopt(curl, CURLOPT_VERBOSE, 0L);
        curl_easy_setopt(curl, CURLOPT_HEADER, 0L);
        curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 1L);
        curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
#if defined(LWS_WITH_OPENSSL) || defined(LWS_WITH_WOLFSSL)
        curl_easy_setopt(curl, CURLOPT_SSLCERTTYPE, "PEM");
#endif
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
        curl_easy_setopt(curl, CURLOPT_TCP_NODELAY, 0L);
        curl_easy_setopt(curl, CURLOPT_TCP_FASTOPEN, 1L);
        curl_easy_setopt(curl, CURLOPT_DNS_CACHE_TIMEOUT, -1L);
        curl_easy_setopt(curl, CURLOPT_IPRESOLVE, CURL_IPRESOLVE_V4);
	}

	~SmartCURL()
	{
		curl_easy_cleanup(curl);
	}
};

// Do the curl
binanceError_t binance::Server::getCurlWithHeader(string& str_result, 
	const string& url, const vector<string>& extra_http_header, const string& post_data, const string& action)
{
	binanceError_t status = binanceSuccess;
	
	Logger::write_log("<curl_api> version |%s|", curl_version());

	SmartCURL curl;

	while (curl.get())
	{
		curl_easy_setopt(curl.get(), CURLOPT_URL, url.c_str());
		curl_easy_setopt(curl.get(), CURLOPT_WRITEFUNCTION, getCurlCb);
		curl_easy_setopt(curl.get(), CURLOPT_WRITEDATA, &str_result);
#if defined(LWS_WITH_OPENSSL) || defined(LWS_WITH_WOLFSSL)
       /* Turn off the default CA locations. */
        curl_easy_setopt(curl.get(), CURLOPT_CAINFO, NULL);
        curl_easy_setopt(curl.get(), CURLOPT_CAPATH, NULL);
        /* load the certificate by installing a function doing the necessary
        * "modifications" to the SSL CONTEXT just before link init.
        */
        curl_easy_setopt(curl.get(), CURLOPT_SSL_CTX_FUNCTION, *sslctx_function);
        curl_easy_setopt(curl.get(), CURLOPT_SSL_CTX_DATA, sslRootsCA);
#endif
		if (curl_easy_setopt(curl.get(), CURLOPT_SSL_VERIFYHOST, 1L) != CURLE_OK)
		{
			Logger::write_log("<curl_api> curl_easy_setopt(CURLOPT_SSL_VERIFYPEER) is not supported");
			status = binanceErrorCurlFailed;
			break;
		}

		if (extra_http_header.size() > 0)
		{
			struct curl_slist *chunk = NULL;
			for (int i = 0; i < extra_http_header.size(); i++)
				chunk = curl_slist_append(chunk, extra_http_header[i].c_str());

 			curl_easy_setopt(curl.get(), CURLOPT_HTTPHEADER, chunk);
 		}

 		if (post_data.size() > 0 || action == "POST" || action == "PUT" || action == "DELETE")
 		{
 			if (action == "PUT" || action == "DELETE")
 				curl_easy_setopt(curl.get(), CURLOPT_CUSTOMREQUEST, action.c_str());
 			curl_easy_setopt(curl.get(), CURLOPT_POSTFIELDS, post_data.c_str());
 		}

		CURLcode res;

		try
		{
			res = curl_easy_perform(curl.get());
		}
		catch (std::bad_alloc &e)
		{
			status = binanceErrorCurlOutOfMemory;
		}
		
		if (status == binanceSuccess)
		{
			// Check for errors.
			if (res != CURLE_OK)
			{
				Logger::write_log("<curl_api> curl_easy_perform() failed: %s", curl_easy_strerror(res));
				status = binanceErrorCurlFailed;
			}
		}

		break;
	}

	Logger::write_log("<curl_api> Done.");

	return status;
}

