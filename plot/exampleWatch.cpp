#include <json/json.h>
#include <plot/plot.h>

#include <csignal>
#include <atomic>

#include "binance_logger.h"
#include "binance_websocket.h"

#define MAX_HEIGHT 512
#define MAX_WIDTH 512
#define MAX_DATASETS 32
#define MAX_PIPELINE_ELEMENTS 32
#define OUT_BUF (MAX_WIDTH * MAX_HEIGHT)

using namespace binance;
using namespace std;

struct plot_static_memory {
  struct plot plot;
  uint8_t canvas[MAX_WIDTH * MAX_HEIGHT];
  double data_buf[MAX_WIDTH * MAX_HEIGHT];
  struct plot_data pd[MAX_DATASETS];
  char out_buf[MAX_WIDTH * MAX_HEIGHT];
  struct plot_pipeline_elem pipeline_elems[MAX_DATASETS * MAX_PIPELINE_ELEMENTS];
  struct plot_pipeline_elem default_pipeline[MAX_PIPELINE_ELEMENTS];
  struct plot_data default_dataset;
};

typedef bool ((*fetch_new_data)(struct plot *p));

struct miniTicker_context {
  string s;
  double v ,q ,h ,l, o, c;
  string chart_name, description;
};

static struct miniTicker_context candle_context = {.chart_name = "bitcoin:BNB", .description = "ohlc4:avg12:sma9"};

static uint32_t
get_candle(void *_ctx, double *out, uint32_t out_max) {
  struct miniTicker_context* ctx = static_cast<  miniTicker_context *>(_ctx);
  out[0] = (ctx->h + ctx->l + ctx->o + ctx->c) / 4;
  return 1;
}

static int ws_klines_onData(Json::Value &json_result) {
//      "E" : 1634887189206,
//      "c" : "62725.85000000",
//      "e" : "24hrMiniTicker",
//      "h" : "66639.74000000",
//      "l" : "62000.00000000",
//      "o" : "64814.44000000",
//      "q" : "4042794379.61456920",
//      "s" : "BTCUSDT",
//      "v" : "63363.65559000"
  candle_context.s = json_result["s"].asCString();
  candle_context.v = atof(json_result["v"].asCString());
  candle_context.q = atof(json_result["q"].asCString());
  candle_context.h = atof(json_result["h"].asCString());
  candle_context.l = atof(json_result["l"].asCString());
  candle_context.o = atof(json_result["o"].asCString());
  candle_context.c = atof(json_result["c"].asCString());
  return 0;
}

static atomic<int> loop(1);

static void
handle_sigint(int sig) {
  Logger::write_log("<handle_sigint:%d\n", sig);
  Websocket::kill_all();
  loop.store(0);
}

enum esc_seq {
  esc_curs_hide,
  esc_curs_show,
  esc_curs_up,
  esc_curs_down,
};

static const char *esc_seq[] = {
    [esc_curs_hide] = "\033[?25l",
    [esc_curs_show] = "\033[?12l\033[?25h",
    [esc_curs_up]   = "\033[%dA",
    [esc_curs_down] = "\033[%dB",
};

void
do_esc(enum esc_seq es, ...) {
  va_list ap;
  va_start(ap, es);
  vprintf(esc_seq[es], ap);
  va_end(ap);
}

bool
follow_cb(struct plot *p) {
  if (plot_fetch(p, 0) && loop.load()) {
    return true;
  }
  return false;
}

void
animate_plot(struct plot *p, char *buf, uint32_t bufsize, long ms,
             fetch_new_data fetch) {
  int height = p->height;

  if (p->x_label.every && p->x_label.side) {
    height += p->x_label.side == plot_label_side_both ? 2 : 1;
  }

  struct timespec sleep = {
      .tv_sec = 0,
      .tv_nsec = ms * 100000,
  };

  do_esc(esc_curs_hide);

  while (loop.load()) {
    if (!fetch(p)) {
      loop.store(0);
      break;
    }
    plot_string(p, buf, bufsize);
    if (*buf) {
      fputs(buf, stdout);
      do_esc(esc_curs_up, height);
      fflush(stdout);
    }
    nanosleep(&sleep, NULL);
  }
  do_esc(esc_curs_down, height);
  do_esc(esc_curs_show);
}

void *fetch_ws_candle(void *) {
  Websocket::init();
  Websocket::connect_endpoint(ws_klines_onData, "/ws/bnbusdt@miniTicker");
  Websocket::enter_event_loop();
  loop.store(0);
  return NULL;
}

int main() {
  Logger::set_debug_level(1);
  Logger::set_debug_logfp(stderr);
  signal(SIGINT, handle_sigint);
  pthread_t ws_thread;
  if (pthread_create(&ws_thread, NULL, fetch_ws_candle, reinterpret_cast<void *>(0))) {
    fprintf(stderr, "Error creating thread\n");
    return 1;
  }
  Logger::write_log(":%s : %s\n", candle_context.chart_name.c_str(), candle_context.description.c_str());
  static struct plot_static_memory mem = {0};
  struct plot *p = &mem.plot;
  plot_init(p, mem.canvas, mem.data_buf, mem.pd, 24, 80, MAX_DATASETS);

  p->y_label.side = plot_label_side::plot_label_side_both;
  p->x_label.side = plot_label_side::plot_label_side_neither;

  plot_dataset_init(p->data, plot_color_black, mem.default_pipeline, MAX_PIPELINE_ELEMENTS, NULL, NULL);

  /*SMA line*/
  {
    plot_add_dataset(p, plot_color_red, &mem.pipeline_elems[p->datasets * MAX_PIPELINE_ELEMENTS], MAX_PIPELINE_ELEMENTS, get_candle, &candle_context);
    int32_t sma = 9;
    void *ctx_sma = &sma;
    plot_pipeline_append(&p->data[p->datasets - 1], plot_data_proc_type::data_proc_sma, ctx_sma, sizeof(sma));
  }

  /*AVG line*/
  {
    plot_add_dataset(p, plot_color_cyan, &mem.pipeline_elems[p->datasets * MAX_PIPELINE_ELEMENTS], MAX_PIPELINE_ELEMENTS, get_candle, &candle_context);
    int32_t avg = 12;
    void *ctx_avg = &avg;
    plot_pipeline_append(&p->data[p->datasets - 1], plot_data_proc_type::data_proc_avg, ctx_avg, sizeof(avg));
  }

  {
    animate_plot(p, mem.out_buf, OUT_BUF, 500, follow_cb);
  }
  if (pthread_join(ws_thread, NULL)) {
    fprintf(stderr, "Error joining thread, it seems terminated\n");
    return 0;
  }
  return 0;
}

