#ifndef _HYPERVISOR_RX_H_
#define _HYPERVISOR_RX_H_

#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>

#include "srslte/srslte.h"

#include "liquid/liquid.h"

#include "helpers.h"
#include "../rf_monitor/lbt.h"
#include "phy_comm_control.h"
#include "plot.h"

// *********** Defintion of RX macros ***********
// RX local offset.
#define PHY_RX_LO_OFFSET +42.0e6

#define NOF_USRP_READ_BUFFERS 9500

#define NOF_CHANNELIZER_BUFFERS 500

#define ENABLE_HYPER_RX_PRINTS 1

#define ENABLE_USRP_READ_SAMPLES_THREAD 1

#define ENABLE_CHANNELIZER_THREAD 1

#define ENABLE_LOCAL_RX_FREQ_CORRECTION 1

#define MEASURE_USRP_BUFFER_FULL_TIME 0

#define MEASURE_CHANN_BUFFER_FULL_TIME 0

#define DEBUG_USRP_READ_BUFFER_COUNTER 0

#define DEBUG_CHANNELIZER_BUFFER_COUNTER 0

#define WRITE_RX_USRP_READ_INTO_FILE 0

#define WRITE_RX_FREQ_SHIFTED_USRP_READ_INTO_FILE 0

// *********** Defintion of debugging macros ***********
#define HYPER_RX_PRINT(_fmt, ...) do { if(ENABLE_HYPER_RX_PRINTS && scatter_verbose_level >= 0) { \
  fprintf(stdout, "[HYPER RX PRINT]: " _fmt, __VA_ARGS__); } } while(0)

#define HYPER_RX_DEBUG(_fmt, ...) do { if(ENABLE_HYPER_RX_PRINTS && scatter_verbose_level >= SRSLTE_VERBOSE_DEBUG) { \
  fprintf(stdout, "[HYPER RX DEBUG]: " _fmt, __VA_ARGS__); } } while(0)

#define HYPER_RX_INFO(_fmt, ...) do { if(ENABLE_HYPER_RX_PRINTS && scatter_verbose_level >= SRSLTE_VERBOSE_INFO) { \
  fprintf(stdout, "[HYPER RX INFO]: " _fmt, __VA_ARGS__); } } while(0)

#define HYPER_RX_ERROR(_fmt, ...) do { fprintf(stdout, "[HYPER RX ERROR]: " _fmt, __VA_ARGS__); } while(0)

#define HYPER_RX_PRINT_TIME(_fmt, ...) do { if(ENABLE_HYPER_RX_PRINTS && scatter_verbose_level >= 0) { \
  char date_time_str[30]; helpers_get_data_time_string(date_time_str); \
  fprintf(stdout, "[HYPER RX PRINT]: %s - " _fmt, date_time_str, __VA_ARGS__); } } while(0)

#define HYPER_RX_DEBUG_TIME(_fmt, ...) do { if(ENABLE_HYPER_RX_PRINTS && scatter_verbose_level >= SRSLTE_VERBOSE_DEBUG) { \
  char date_time_str[30]; helpers_get_data_time_string(date_time_str); \
  fprintf(stdout, "[HYPER RX DEBUG]: %s - " _fmt, date_time_str, __VA_ARGS__); } } while(0)

#define HYPER_RX_INFO_TIME(_fmt, ...) do { if(ENABLE_HYPER_RX_PRINTS && scatter_verbose_level >= SRSLTE_VERBOSE_INFO) { \
  char date_time_str[30]; helpers_get_data_time_string(date_time_str); \
  fprintf(stdout, "[HYPER RX INFO]: %s - " _fmt, date_time_str, __VA_ARGS__); } } while(0)

#define HYPER_RX_ERROR_TIME(_fmt, ...) do { char date_time_str[30]; helpers_get_data_time_string(date_time_str); \
  fprintf(stdout, "[HYPER RX ERROR]: %s - " _fmt, date_time_str, __VA_ARGS__); } while(0)

// *********** Definition of types ***********
typedef struct {
  srslte_rf_t *hypervisor_rx_rf;
  float initial_rx_gain;
  hypervisor_ctrl_t last_rx_hypervisor_ctrl;
  bool use_std_carrier_sep;
  bool run_channelizer_thread;
  bool vphy_rx_running;
  uint32_t num_of_rx_vphys;
#if(ENABLE_USRP_READ_SAMPLES_THREAD==1)
  // USRP circular buffer.
  cf_t *usrp_read_buffer[NOF_USRP_READ_BUFFERS];
  // USRP read samples write counter.
  uint32_t usrp_read_samples_buffer_wr_cnt;
  // Mutex used to synchronize to the usrp buffer.
  pthread_mutex_t usrp_read_mutex;
  // Condition variable used to enable access to the usrp buffer.
  pthread_cond_t usrp_read_cond_var;
  // Handle vector used to access the circular buffer storing usrp buffer read pointers.
  in_buf_ctx_cb_handle usrp_read_cb_buffer_handle;
  // Structures for USRP read thread management.
  pthread_attr_t usrp_read_thread_attr;
  pthread_t usrp_read_thread_id;
#endif
#if(ENABLE_CHANNELIZER_THREAD==1)
  // Structures for channelizer thread management.
  pthread_attr_t channelizer_thread_attr;
  pthread_t channelizer_thread_id;
  // Channelizer channel circular buffer.
  cf_t *channelizer_buffer[NOF_CHANNELIZER_BUFFERS];
  // Channelizer channel buffer circular write counter.
  uint32_t channelizer_buffer_wr_cnt;
  // Mutex used to synchronize to the channelizer buffer.
  pthread_mutex_t channelizer_mutex[MAX_NUM_CONCURRENT_VPHYS];
  // Condition variable used to enable access to the channelizer buffer.
  pthread_cond_t channelizer_cond_var[MAX_NUM_CONCURRENT_VPHYS];
  // Handle vector used to access the circular buffer storing channel buffer read pointers.
  chan_buf_ctx_cb_handle channelizer_cb_buffer_handle[MAX_NUM_CONCURRENT_VPHYS];
#endif
  // Channelizer parameters.
  uint32_t nof_samples_to_read;
  uint32_t channelizer_nof_channels;
  uint32_t channelizer_nof_frames;
  uint32_t channelizer_filter_delay;
  float channelizer_stop_band_att;
  firpfbch_crcf channelizer;
  // Mutex used to synchronize access to the Hypervisor Rx thread.
  pthread_mutex_t hypervisor_rx_radio_params_mutex;
  // Mutex used to synchronize access to channel list.
  pthread_mutex_t hypervisor_rx_channel_list_mutex;
  // Channel list.
  uint32_t channel_list[MAX_NUM_CONCURRENT_VPHYS];
#if(ENABLE_FREQ_SHIFT_CORRECTION==1)
  srslte_cexptab_t freq_shift_waveform_obj;
  cf_t *freq_shift_waveform;
#endif
} hypervisor_rx_t;

int hypervisor_rx_initialize(transceiver_args_t* const args, srslte_rf_t* const rf);

int hypervisor_rx_uninitialize();

void hypervisor_rx_init_last_hypervisor_ctrl(transceiver_args_t* const args);

int hypervisor_rx_set_sample_rate(uint32_t nof_prb, bool use_std_carrier_sep);

void hypervisor_rx_set_radio_center_freq_and_gain(transceiver_args_t* const args);

float hypervisor_rx_set_gain(float rx_gain);

void hypervisor_rx_set_center_frequency(double rx_center_frequency);

int hypervisor_rx_initialize_stream();

int hypervisor_rx_stop_stream_and_flush_buffer();

void hypervisor_rx_change_parameters(hypervisor_ctrl_t *hypervisor_ctrl);

void *hypervisor_rx_usrp_read_work(void *h);

void *hypervisor_rx_channelizer_work(void *h);

void hypervisor_rx_initialize_channelizer(transceiver_args_t* const args);

void hypervisor_rx_uninitialize_channelizer();

int hypervisor_rx_recv(void *h, void *data, uint32_t nof_samples);

void hypervisor_rx_push_channel_buffer_context_to_container(uint32_t vphy_id, channel_buffer_context_t* const channel_buffer_ctx);

void hypervisor_rx_get_channel_buffer_context_inc_nof_reads(uint32_t vphy_id, channel_buffer_context_t* const channel_buffer_ctx);

bool hypervisor_rx_wait_and_get_channel_buffer_context(uint32_t vphy_id, channel_buffer_context_t* const channel_buffer_ctx);

void hypervisor_rx_set_channel(uint32_t vphy_id, uint32_t channel);

uint32_t hypervisor_rx_get_channel(uint32_t vphy_id);

void hypervisor_rx_set_vphy_rx_running_flag(bool vphy_rx_running);

void hypervisor_rx_set_last_gain(uint32_t rx_gain);

uint32_t hypervisor_rx_get_last_gain();

int hypervisor_rx_radio_recv(hypervisor_rx_t* hyper_rx_handle, void *data, uint32_t nof_samples, bool blocking, time_t *full_secs, double *frac_secs, size_t channel);

bool hypervisor_rx_timedwait_and_get_channel_buffer_context(uint32_t vphy_id, channel_buffer_context_t* const channel_buffer_ctx);

void hypervisor_rx_push_usrp_read_buffer_context_to_container(uint32_t usrp_read_samples_buffer_rd_cnt);

bool hypervisor_rx_push_usrp_read_buffer_context_to_container_with_dropping(uint32_t usrp_read_samples_buffer_rd_cnt);

bool hypervisor_rx_timedwait_and_get_usrp_buffer_context(uint32_t* in_buffer_cnt);

#endif // _HYPERVISOR_RX_H_
