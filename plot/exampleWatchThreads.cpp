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

struct miniTicker_context_1 {
  string s;
  double v ,q ,h ,l, o, c;
  string chart_name, description;
};
struct miniTicker_context_2 {
  string s;
  double v ,q ,h ,l, o, c;
  string chart_name, description;
};
static struct miniTicker_context_1 candle_context_1 = {.chart_name = "bitcoin:BNBUSDT", .description = "ohlc4:avg30:sma15:ema25"};
static struct miniTicker_context_2 candle_context_2 = {.chart_name = "bitcoin:BNBBUSD", .description = "ohlc4:avg30:sma15:ema25"};

static uint32_t
get_candle_1(void *_ctx, double *out, uint32_t out_max) {
  struct miniTicker_context_1* ctx = static_cast<  miniTicker_context_1 *>(_ctx);
  out[0] = (ctx->h + ctx->l + ctx->o + ctx->c) / 4;
  return 1;
}

static uint32_t
get_candle_2(void *_ctx, double *out, uint32_t out_max) {
  struct miniTicker_context_2* ctx = static_cast<  miniTicker_context_2 *>(_ctx);
  out[0] = (ctx->h + ctx->l + ctx->o + ctx->c) / 4;
  return 1;
}

static int ws_klines_onData_1(Json::Value &json_result) {
//      "E" : 1634887189206,
//      "c" : "62725.85000000",
//      "e" : "24hrMiniTicker",
//      "h" : "66639.74000000",
//      "l" : "62000.00000000",
//      "o" : "64814.44000000",
//      "q" : "4042794379.61456920",
//      "s" : "BTCUSDT",
//      "v" : "63363.65559000"
    candle_context_1.s = json_result["s"].asCString();
    candle_context_1.v = atof(json_result["v"].asCString());
    candle_context_1.q = atof(json_result["q"].asCString());
    candle_context_1.h = atof(json_result["h"].asCString());
    candle_context_1.l = atof(json_result["l"].asCString());
    candle_context_1.o = atof(json_result["o"].asCString());
    candle_context_1.c = atof(json_result["c"].asCString());

  return 0;
}

static int ws_klines_onData_2(Json::Value &json_result) {

    candle_context_2.s = json_result["s"].asCString();
    candle_context_2.v = atof(json_result["v"].asCString());
    candle_context_2.q = atof(json_result["q"].asCString());
    candle_context_2.h = atof(json_result["h"].asCString());
    candle_context_2.l = atof(json_result["l"].asCString());
    candle_context_2.o = atof(json_result["o"].asCString());
    candle_context_2.c = atof(json_result["c"].asCString());

  return 0;
}

static atomic<int> loop(1);

static void
handle_sigint(int sig) {
  atomic_store(&loop, 0);
  Websocket::kill_all();
}

enum esc_seq {
  esc_curs_hide,
  esc_curs_show,
  esc_curs_up,
  esc_curs_down,
  esc_curs_forward,
  esc_curs_back,
};

static const char *esc_seq[] = {
    [esc_curs_hide] = "\033[?25l",
    [esc_curs_show] = "\033[?12l\033[?25h",
    [esc_curs_up]   = "\033[%dA",
    [esc_curs_down] = "\033[%dB",
    [esc_curs_forward] = "\033[%dC",
    [esc_curs_back] = "\033[%dD",
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
  if (loop.load() && plot_fetch(p, 0)) {
    return true;
  }
  return false;
}

static void
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
  if(p->depth > 6){// test, need to find batter way
    do_esc(esc_curs_down, height);
  }

  while (loop.load()) {
    if (!fetch(p)) {
      loop.store(0);
      break;
    }
    plot_string(p, buf, bufsize);
    if (*buf) {
      if(p->depth > 6){// test, need to find batter way
        //do_esc(esc_curs_back, p->width);
        do_esc(esc_curs_up, height);
        fputs(buf, stdout);
        fflush(stdout);
        //do_esc(esc_curs_forward, p->width);
      }else{
        fputs(buf, stdout);
        do_esc(esc_curs_up, height);
        fflush(stdout);
      }
    }
    nanosleep(&sleep, NULL);
  }
  do_esc(esc_curs_down, height);
  do_esc(esc_curs_show);
}

void *fetch_ws_candle(void *) {
  Websocket::init();
  Websocket::connect_endpoint(ws_klines_onData_1, "/ws/bnbusdt@miniTicker");
  Websocket::connect_endpoint(ws_klines_onData_2, "/ws/bnbbusd@miniTicker");
  Websocket::enter_event_loop();
  atomic_store(&loop, 0);
  return NULL;
}

void *chart_1(void *) {
  static struct plot_static_memory mem = {0};
  struct plot *p = &mem.plot;
  plot_init(p, mem.canvas, mem.data_buf, mem.pd, 24, 80, /*MAX_DATASETS*/6);

  p->y_label.side = plot_label_side::plot_label_side_both;
  p->x_label.side = plot_label_side::plot_label_side_bottom;
  p->x_label.every =2;
  p->x_label.mod = 3;

  plot_dataset_init(p->data, plot_color_black, mem.default_pipeline, MAX_PIPELINE_ELEMENTS, NULL, NULL);
  /*OHLC4 line*/
  {
    plot_add_dataset(p, plot_color_brwhite, &mem.pipeline_elems[p->datasets * MAX_PIPELINE_ELEMENTS], MAX_PIPELINE_ELEMENTS, get_candle_1, &candle_context_1);
  }
  /*SMA line*/
  {
    plot_add_dataset(p, plot_color_green, &mem.pipeline_elems[p->datasets * MAX_PIPELINE_ELEMENTS], MAX_PIPELINE_ELEMENTS, get_candle_1, &candle_context_1);
    int32_t sma = 15;
    void *ctx_sma = &sma;
    plot_pipeline_append(&p->data[p->datasets - 1], plot_data_proc_type::data_proc_sma, ctx_sma, sizeof(sma));
  }
  /*EMA line*/
  {
    plot_add_dataset(p, plot_color_red, &mem.pipeline_elems[p->datasets * MAX_PIPELINE_ELEMENTS], MAX_PIPELINE_ELEMENTS, get_candle_1, &candle_context_1);
    int32_t ema = 25;
    void *ctx_ema = &ema;
    plot_pipeline_append(&p->data[p->datasets - 1], plot_data_proc_type::data_proc_ema, ctx_ema, sizeof(ema));
  }
  /*AVG line*/
  {
    plot_add_dataset(p, plot_color_yellow, &mem.pipeline_elems[p->datasets * MAX_PIPELINE_ELEMENTS], MAX_PIPELINE_ELEMENTS, get_candle_1, &candle_context_1);
    int32_t avg = 30;
    void *ctx_avg = &avg;
    plot_pipeline_append(&p->data[p->datasets - 1], plot_data_proc_type::data_proc_avg, ctx_avg, sizeof(avg));
  }

  {
    animate_plot(p, mem.out_buf, OUT_BUF, 500, follow_cb);
  }
  return NULL;
}

void *chart_2(void *) {
  static struct plot_static_memory mem = {0};
  struct plot *p = &mem.plot;
  plot_init(p, mem.canvas, mem.data_buf, mem.pd, 24, 80, /*MAX_DATASETS*/8);

  p->y_label.side = plot_label_side::plot_label_side_both;
  p->x_label.side = plot_label_side::plot_label_side_bottom;
  p->x_label.every =4;
  p->x_label.mod = 3;

  plot_dataset_init(p->data, plot_color_black, mem.default_pipeline, MAX_PIPELINE_ELEMENTS, NULL, NULL);
  /*OHLC4 line*/
  {
    plot_add_dataset(p, plot_color_brwhite, &mem.pipeline_elems[p->datasets * MAX_PIPELINE_ELEMENTS], MAX_PIPELINE_ELEMENTS, get_candle_2, &candle_context_2);
  }
  /*SMA line*/
  {
    plot_add_dataset(p, plot_color_blue, &mem.pipeline_elems[p->datasets * MAX_PIPELINE_ELEMENTS], MAX_PIPELINE_ELEMENTS, get_candle_2, &candle_context_2);
    int32_t sma = 15;
    void *ctx_sma = &sma;
    plot_pipeline_append(&p->data[p->datasets - 1], plot_data_proc_type::data_proc_sma, ctx_sma, sizeof(sma));
  }
  /*EMA line*/
  {
    plot_add_dataset(p, plot_color_magenta, &mem.pipeline_elems[p->datasets * MAX_PIPELINE_ELEMENTS], MAX_PIPELINE_ELEMENTS, get_candle_2, &candle_context_2);
    int32_t ema = 25;
    void *ctx_ema = &ema;
    plot_pipeline_append(&p->data[p->datasets - 1], plot_data_proc_type::data_proc_ema, ctx_ema, sizeof(ema));
  }
  /*AVG line*/
  {
    plot_add_dataset(p, plot_color_cyan, &mem.pipeline_elems[p->datasets * MAX_PIPELINE_ELEMENTS], MAX_PIPELINE_ELEMENTS, get_candle_2, &candle_context_2);
    int32_t avg = 30;
    void *ctx_avg = &avg;
    plot_pipeline_append(&p->data[p->datasets - 1], plot_data_proc_type::data_proc_avg, ctx_avg, sizeof(avg));
  }
  {
    char *charset = "  ╔║╚╠╗═╦╝╣╩╬";
    plot_set_custom_charset(p, &charset[1], strlen(charset));
  }

  {
    animate_plot(p, mem.out_buf, OUT_BUF, 500, follow_cb);
  }
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

  Logger::write_log("\n       %s : %s :<VS>: %s : %s     \n",
                    candle_context_1.chart_name.c_str(), candle_context_1.description.c_str(),
                    candle_context_2.chart_name.c_str(), candle_context_2.description.c_str());

  pthread_t drow_chart[1];
  if (pthread_create(&drow_chart[0], NULL, chart_1, reinterpret_cast<void *>(0))) {
    fprintf(stderr, "Error creating thread\n");
    return 1;
  }

  if (pthread_create(&drow_chart[1], NULL, chart_2, reinterpret_cast<void *>(0))) {
    fprintf(stderr, "Error creating thread\n");
    return 1;
  }
  
  if (pthread_join(ws_thread, NULL)) {
    fprintf(stderr, "Error joining thread, it seems terminated\n");
    return 0;
  }
  /* wait for the thread to finish */
  for (int i = 0; i < 1; i++)
  {

    if(pthread_join(drow_chart[i], NULL)) {
      fprintf(stderr, "Error joining thread\n");
      return -2;
    }
  }

  return 0;
}

