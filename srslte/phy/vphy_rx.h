#ifndef _VPHY_RX_H_
#define _VPHY_RX_H_

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

#include <uhd.h>

#include "srslte/srslte.h"
#include "srslte/intf/intf.h"

#include "helpers.h"
#include "transceiver.h"
#include "phy_comm_control.h"

// ****************** Definition of flags ***********************
#define NUMBER_OF_DECODED_DATA_BUFFERS 100

#define PHY_RX_LO_OFFSET +42.0e6

#define ENABLE_PHY_RX_PRINTS 1

#define ENABLE_DECODING_TIME_MEASUREMENT 0

#define WRITE_VPHY_RX_SUBFRAME_INTO_FILE 0

#define WRITE_VPHY_RX_WRONGLY_DECODED_SUBFRAME_INTO_FILE 0

#define WRITE_VPHY_DECODING_SIGNAL_INTO_FILE 0

// ****************** Definition of macros ***********************
#define PHY_RX_PRINT(_fmt, ...) do { if(ENABLE_PHY_RX_PRINTS && scatter_verbose_level >= 0) { \
  fprintf(stdout, "[PHY RX PRINT]: " _fmt, __VA_ARGS__); } } while(0)

#define PHY_RX_DEBUG(_fmt, ...) do { if(ENABLE_PHY_RX_PRINTS && scatter_verbose_level >= SRSLTE_VERBOSE_DEBUG) { \
  fprintf(stdout, "[PHY RX DEBUG]: " _fmt, __VA_ARGS__); } } while(0)

#define PHY_RX_INFO(_fmt, ...) do { if(ENABLE_PHY_RX_PRINTS && scatter_verbose_level >= SRSLTE_VERBOSE_INFO) { \
  fprintf(stdout, "[PHY RX INFO]: " _fmt, __VA_ARGS__); } } while(0)

#define PHY_RX_ERROR(_fmt, ...) do { fprintf(stdout, "[PHY RX ERROR]: " _fmt, __VA_ARGS__); } while(0)

#define PHY_RX_PRINT_TIME(_fmt, ...) do { if(ENABLE_PHY_RX_PRINTS && scatter_verbose_level >= 0) { \
  char date_time_str[30]; helpers_get_data_time_string(date_time_str); \
  fprintf(stdout, "[PHY RX PRINT]: %s - " _fmt, date_time_str, __VA_ARGS__); } } while(0)

#define PHY_RX_DEBUG_TIME(_fmt, ...) do { if(ENABLE_PHY_RX_PRINTS && scatter_verbose_level >= SRSLTE_VERBOSE_DEBUG) { \
  char date_time_str[30]; helpers_get_data_time_string(date_time_str); \
  fprintf(stdout, "[PHY RX DEBUG]: %s - " _fmt, date_time_str, __VA_ARGS__); } } while(0)

#define PHY_RX_INFO_TIME(_fmt, ...) do { if(ENABLE_PHY_RX_PRINTS && scatter_verbose_level >= SRSLTE_VERBOSE_INFO) { \
  char date_time_str[30]; helpers_get_data_time_string(date_time_str); \
  fprintf(stdout, "[PHY RX INFO]: %s - " _fmt, date_time_str, __VA_ARGS__); } } while(0)

#define PHY_RX_ERROR_TIME(_fmt, ...) do { char date_time_str[30]; helpers_get_data_time_string(date_time_str); \
  fprintf(stdout, "[PHY RX ERROR]: %s - " _fmt, date_time_str, __VA_ARGS__); } while(0)

// ****************** Definition of types ******************
typedef struct {

  pthread_attr_t vphy_rx_decoding_thread_attr;
  pthread_t vphy_rx_decoding_thread_id;

  pthread_attr_t vphy_rx_sync_thread_attr;
  pthread_t vphy_rx_sync_thread_id;

  // Variable used to stop phy decoding thread.
  volatile sig_atomic_t run_phy_decoding_thread;
  // Variable used to stop phy synchronization thread.
  volatile sig_atomic_t run_phy_synchronization_thread;

  // This phy controls stores the last configured values.
  slot_ctrl_t last_rx_slot_control;

  // This mutex is used to synchronize the access to the last configured slot control.
  pthread_mutex_t phy_rx_slot_control_mutex;

  // Mutex used to synchronize between synchronization and decoding thread.
  pthread_mutex_t vphy_rx_ue_sync_mutex;
  // Condition variable used to synchronize between synchronization and decoding thread.
  pthread_cond_t vphy_rx_ue_sync_cv;

  // Structures used to decode subframes.
  srslte_ue_sync_t ue_sync;
  srslte_ue_dl_t ue_dl;
  srslte_cell_t vphy_ue;

  float initial_rx_gain;
  double competition_bw;
  double radio_center_freq;
  uint16_t rnti;
  float initial_agc_gain;
  bool use_std_carrier_sep;
  int initial_subframe_index; // Set the subframe index number to be used to start from.
  bool add_tx_timestamp;
  double radio_sampling_rate;
  bool enable_cfo_correction;
  sync_cb_handle synch_handle;
  uint32_t channelizer_nof_channels;
  vphy_reader_t vphy_reader;
  uint32_t vphy_id_rx_info;
  bool plot_rx_info;
  // ID of the timer used to check abnormal behaviours.
  timer_t vphy_rx_sync_timer_id;
  float pss_peak_threshold;
  uint32_t sequence_number;
  bool decode_pdcch;
  uint32_t vphy_id;
  uint32_t node_id;
  bool phy_filtering;
  uint32_t max_turbo_decoder_noi;

} vphy_rx_thread_context_t;

// *************** Declaration of functions ***************
int vphy_rx_start_thread(transceiver_args_t* const args, uint32_t vphy_id);

int vphy_rx_stop_thread(uint32_t vphy_id);

int vphy_rx_init_thread_context(vphy_rx_thread_context_t* const vphy_rx_thread_ctx, transceiver_args_t* const args);

void vphy_rx_init_cell_parameters(vphy_rx_thread_context_t* const vphy_rx_thread_ctx, transceiver_args_t* const args);

void vphy_rx_init_last_slot_control(vphy_rx_thread_context_t* const vphy_rx_thread_ctx, transceiver_args_t* const args);

int vphy_rx_ue_init(vphy_rx_thread_context_t* const vphy_rx_thread_ctx);

void vphy_rx_ue_free(vphy_rx_thread_context_t* const vphy_rx_thread_ctx);

int vphy_rx_start_sync_thread(vphy_rx_thread_context_t* const vphy_rx_thread_ctx);

int vphy_rx_stop_sync_thread(vphy_rx_thread_context_t* const vphy_rx_thread_ctx);

int vphy_rx_start_decoding_thread(vphy_rx_thread_context_t* const vphy_rx_thread_ctx);

int vphy_rx_stop_decoding_thread(vphy_rx_thread_context_t* const vphy_rx_thread_ctx);

void vphy_rx_initialize_reader(vphy_rx_thread_context_t* const vphy_rx_thread_ctx, transceiver_args_t* const args);

void set_number_of_expected_slots(vphy_rx_thread_context_t* const vphy_rx_thread_ctx, uint32_t length);

uint32_t get_number_of_expected_slots(vphy_rx_thread_context_t* const vphy_rx_thread_ctx);

void set_bw_index(vphy_rx_thread_context_t* const vphy_rx_thread_ctx, uint32_t bw_index);

uint32_t get_bw_index(vphy_rx_thread_context_t* const vphy_rx_thread_ctx);

int vphy_rx_change_parameters(uint32_t vphy_id, slot_ctrl_t* const slot_ctrl);

uint32_t vphy_rx_translate_channel(vphy_rx_thread_context_t* const vphy_rx_thread_ctx, uint32_t channel);

void vphy_rx_set_reader_channel(vphy_rx_thread_context_t* const vphy_rx_thread_ctx, uint32_t channel);

uint32_t vphy_rx_get_reader_channel(vphy_rx_thread_context_t* const vphy_rx_thread_ctx);

void *phy_decoding_work(void *h);

void vphy_rx_send_rx_statistics(phy_stat_t* const phy_rx_stat);

void *vphy_rx_sync_frame_type_one_work(void *h);

void *vphy_rx_sync_frame_type_two_work(void *h);

int vphy_rx_recv(void *h, void *data, uint32_t nsamples);

void vphy_rx_print_ue_sync(short_ue_sync_t* const ue_sync, char* const str);

void vphy_rx_push_ue_sync_to_queue(vphy_rx_thread_context_t *vphy_rx_thread_ctx, short_ue_sync_t* const short_ue_sync);

void vphy_rx_pop_ue_sync_from_queue(vphy_rx_thread_context_t *vphy_rx_thread_ctx, short_ue_sync_t* const short_ue_sync);

bool vphy_rx_wait_queue_not_empty(vphy_rx_thread_context_t *vphy_rx_thread_ctx);

timer_t* vphy_rx_get_sync_timer_id(uint32_t vphy_id);

#endif // _VPHY_RX_H_
