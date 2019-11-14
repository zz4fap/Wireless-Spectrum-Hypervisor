#ifndef _PLOT_H_
#define _PLOT_H_

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <unistd.h>
#include <sys/time.h>
#include <signal.h>
#include <string.h>
#define __STDC_FORMAT_MACROS
#include <inttypes.h>

#include "srsgui/srsgui.h"

#include "srslte/srslte.h"

#include "liquid/liquid.h"

#define ENABLE_PLOT_PRINTS 1

// *********** Defintion of debugging macros ***********
#define PLOT_PRINT(_fmt, ...) do { if(ENABLE_PLOT_PRINTS && scatter_verbose_level >= 0) { \
  fprintf(stdout, "[PLOT PRINT]: " _fmt, __VA_ARGS__); } } while(0)

#define PLOT_DEBUG(_fmt, ...) do { if(ENABLE_PLOT_PRINTS && scatter_verbose_level >= SRSLTE_VERBOSE_DEBUG) { \
  fprintf(stdout, "[PLOT DEBUG]: " _fmt, __VA_ARGS__); } } while(0)

#define PLOT_INFO(_fmt, ...) do { if(ENABLE_PLOT_PRINTS && scatter_verbose_level >= SRSLTE_VERBOSE_INFO) { \
  fprintf(stdout, "[PLOT INFO]: " _fmt, __VA_ARGS__); } } while(0)

#define PLOT_ERROR(_fmt, ...) do { fprintf(stdout, "[PLOT ERROR]: " _fmt, __VA_ARGS__); } while(0)

// *********** Definition of types ***********
// Define plotting types.
typedef enum {PLOT_PSD=0, PLOT_WATERFALL=1, PLOT_PERIODOGRAM=2} plot_t;

typedef struct {
  plot_real_t p_sync;
  plot_real_t pce;
  plot_scatter_t pscatequal;
  plot_scatter_t pscatequal_pdcch;
  float tmp_plot[110*15*2048];
  float tmp_plot2[110*15*2048];
} ue_decoding_plot_context_t;

typedef struct {
  uint32_t fft_size;
  plot_real_t spectrum;
  plot_waterfall_t waterfall;
  spgramcf periodogram;
  float *psd;
  plot_t plot_type;
  uint32_t nof_samples;
  // Thread parameters.
  // Handle vector used to access the circular buffer storing usrp buffer read pointers.
  in_buf_ctx_cb_handle buffer_index_cb_handle;
  bool run_plot_thread;
  pthread_attr_t plot_thread_attr;
  pthread_t plot_thread_id;
  // Mutex used to synchronize threads.
  pthread_mutex_t buffer_index_mutex;
  // Condition variable used to threads.
  pthread_cond_t buffer_index_cv;
  // USRP circular buffer.
  cf_t **usrp_buffer;
} spectrum_plot_context_t;

// *********** Declaration of functions ***********
int init_ue_decoding_plot_object();

void free_ue_decoding_plot_object();

void update_ue_decoding_plot(srslte_ue_dl_t* const ue_dl, srslte_ue_sync_t* const ue_sync);

// ------------------------- Hypervisor Rx functions ---------------------------
int init_spectrum_plot_object(uint32_t fft_size, uint32_t nof_samples, plot_t plot_type, cf_t **usrp_buffer);

int free_spectrum_plot_object();

void *spectrum_plot_work(void *h);

void spectrum_plot_push_buffer_index(uint32_t index);

bool spectrum_plot_timedwait_and_get_buffer_index(uint32_t* index);

int init_psd_plot(spectrum_plot_context_t *q);

void update_psd_plot(spectrum_plot_context_t *q, cf_t *sig, uint32_t nof_samples);

void update_psd_plot2(uint32_t buffer_index);

int init_waterfall_plot(spectrum_plot_context_t *q);

void update_waterfall_plot(spectrum_plot_context_t *q, cf_t *sig, uint32_t nof_samples);

void update_waterfall_plot2(spectrum_plot_context_t *q, cf_t *sig, uint32_t nof_samples);

void update_waterfall_plot3(uint32_t buffer_index);

int init_periodogram_plot(spectrum_plot_context_t *q);

int destroy_periodogram_plot(spectrum_plot_context_t *q);

void update_periodgram_plot(spectrum_plot_context_t *q, cf_t *sig, uint32_t nof_samples);

void update_periodgram_plot2(uint32_t buffer_index);

#endif // _PLOT_H_
