#ifndef _VPHY_TX_H_
#define _VPHY_TX_H_

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <unistd.h>
#include <sys/time.h>
#include <signal.h>
#include <string.h>
#include <semaphore.h>

#include <uhd.h>

#include "liquid/liquid.h"

#include "srslte/srslte.h"
#include "srslte/intf/intf.h"

#include "../../../../communicator/cpp/communicator_wrapper.h"

#include "helpers.h"
#include "transceiver.h"
#include "hypervisor_tx.h"
#include "phy_comm_control.h"

#include "../rf_monitor/lbt.h"

// ****************** Definition of flags ***********************
#define ENABLE_WINDOWING 0 // Enable/Disable applying windowing to the frequency mapped OFDM symbols.

#define WINDOWING_GUARD_BAND 12 // Number of subcarriers added to each side of the vPHY allocation. These are used and guard-bands and are in fact, a waste of resources.

#define ENABLE_MIB_ENCODING 0 // Enable or disable MIB encoding. By default it is disabled.

#define ENABLE_PHY_TX_PRINTS 1

#define DEBUG_TX_CB_BUFFER 0

#define CHECK_TX_OUT_OF_SEQUENCE 0

// ****************** Definition of debugging macros ***********************
#define PHY_TX_PRINT(_fmt, ...) do { if(ENABLE_PHY_TX_PRINTS && scatter_verbose_level >= 0) { \
  fprintf(stdout, "[PHY Tx PRINT]: " _fmt, __VA_ARGS__); } } while(0)

#define VPHY_TX_PRINT(_fmt, ...) do { if(ENABLE_PHY_TX_PRINTS && scatter_verbose_level >= 0) { \
  fprintf(stdout, "[vPHY Tx PRINT]: " _fmt, __VA_ARGS__); } } while(0)

#define PHY_TX_DEBUG(_fmt, ...) do { if(ENABLE_PHY_TX_PRINTS && scatter_verbose_level >= SRSLTE_VERBOSE_DEBUG) { \
  fprintf(stdout, "[PHY Tx DEBUG]: " _fmt, __VA_ARGS__); } } while(0)

#define VPHY_TX_DEBUG(_fmt, ...) do { if(ENABLE_PHY_TX_PRINTS && scatter_verbose_level >= SRSLTE_VERBOSE_DEBUG) { \
  fprintf(stdout, "[vPHY Tx DEBUG]: " _fmt, __VA_ARGS__); } } while(0)

#define PHY_TX_INFO(_fmt, ...) do { if(ENABLE_PHY_TX_PRINTS && scatter_verbose_level >= SRSLTE_VERBOSE_INFO) { \
  fprintf(stdout, "[PHY Tx INFO]: " _fmt, __VA_ARGS__); } } while(0)

#define VPHY_TX_INFO(_fmt, ...) do { if(ENABLE_PHY_TX_PRINTS && scatter_verbose_level >= SRSLTE_VERBOSE_INFO) { \
  fprintf(stdout, "[vPHY Tx INFO]: " _fmt, __VA_ARGS__); } } while(0)

#define PHY_TX_ERROR(_fmt, ...) do { fprintf(stdout, "[PHY Tx ERROR]: " _fmt, __VA_ARGS__); } while(0)

#define VPHY_TX_ERROR(_fmt, ...) do { fprintf(stdout, "[vPHY Tx ERROR]: " _fmt, __VA_ARGS__); } while(0)

#define PHY_TX_PRINT_TIME(_fmt, ...) do { if(ENABLE_PHY_TX_PRINTS && scatter_verbose_level >= 0) { \
  char date_time_str[30]; helpers_get_data_time_string(date_time_str); \
  fprintf(stdout, "[PHY Tx PRINT]: %s - " _fmt, date_time_str, __VA_ARGS__); } } while(0)

#define PHY_TX_DEBUG_TIME(_fmt, ...) do { if(ENABLE_PHY_TX_PRINTS && scatter_verbose_level >= SRSLTE_VERBOSE_DEBUG) { \
  char date_time_str[30]; helpers_get_data_time_string(date_time_str); \
  fprintf(stdout, "[PHY Tx DEBUG]: %s - " _fmt, date_time_str, __VA_ARGS__); } } while(0)

#define VPHY_TX_DEBUG_TIME(_fmt, ...) do { if(ENABLE_PHY_TX_PRINTS && scatter_verbose_level >= SRSLTE_VERBOSE_DEBUG) { \
  char date_time_str[30]; helpers_get_data_time_string(date_time_str); \
  fprintf(stdout, "[vPHY Tx DEBUG]: %s - " _fmt, date_time_str, __VA_ARGS__); } } while(0)

#define PHY_TX_INFO_TIME(_fmt, ...) do { if(ENABLE_PHY_TX_PRINTS && scatter_verbose_level >= SRSLTE_VERBOSE_INFO) { \
  char date_time_str[30]; helpers_get_data_time_string(date_time_str); \
  fprintf(stdout, "[PHY Tx INFO]: %s - " _fmt, date_time_str, __VA_ARGS__); } } while(0)

#define VPHY_TX_INFO_TIME(_fmt, ...) do { if(ENABLE_PHY_TX_PRINTS && scatter_verbose_level >= SRSLTE_VERBOSE_INFO) { \
  char date_time_str[30]; helpers_get_data_time_string(date_time_str); \
  fprintf(stdout, "[vPHY Tx INFO]: %s - " _fmt, date_time_str, __VA_ARGS__); } } while(0)

#define PHY_TX_ERROR_TIME(_fmt, ...) do { char date_time_str[30]; helpers_get_data_time_string(date_time_str); \
  fprintf(stdout, "[PHY Tx ERROR]: %s - " _fmt, date_time_str, __VA_ARGS__); } while(0)

#define MAX_NUM_OF_CHANNELS 58

// ****************** Definition of types ******************
// This struct is used to keep the context of each one of the PHY Tx threads.
typedef struct {
  uint32_t vphy_id;                        // ID of the current virtual PHY Tx thread.

  tx_cb_handle tx_handle;                  // Circular buffer used to transfer vPHY Tx slot messages from main thread to vPHY Tx threads.

  volatile sig_atomic_t vphy_run_tx_thread; // Variable used to stop a vPHY Tx thread.

  uint32_t num_of_tx_vphys;

  double competition_bw;
  double radio_center_freq;
  uint16_t rnti;
  bool use_std_carrier_sep;
  bool is_lbt_enabled;
  bool send_tx_stats_to_mac;
  bool add_tx_timestamp;
  int initial_subframe_index;               // Set the subframe index number to be used to start from.
  float freq_boost;
  double vphy_sampling_rate;
  uint32_t radio_nof_prb;
  double radio_sampling_rate;
  bool add_preamble_to_front;
  uint32_t radio_fft_len;
  uint32_t vphy_fft_len;
  bool decode_pdcch;
  uint32_t node_id;
  bool phy_filtering;
  uint32_t max_turbo_decoder_noi;

  pthread_mutex_t vphy_tx_mutex;            // Mutex used to synchronize between main and transmission threads.
  pthread_cond_t vphy_tx_cv;                // Condition variable used to synchronize between main and transmission threads.

  slot_ctrl_t last_tx_slot_control;         // This phy controls stores the last configured values.

  pthread_attr_t vphy_tx_thread_attr;       // Structure for thread management.
  pthread_t vphy_tx_thread_id;              // Structure for thread management.

  srslte_cell_t vphy_enodeb;
  srslte_pcfich_t pcfich;
  srslte_pdcch_t pdcch;
  srslte_pdsch_t pdsch;
  srslte_pdsch_cfg_t pdsch_cfg;
  srslte_softbuffer_tx_t softbuffer;
  srslte_regs_t regs;
  srslte_ra_dl_dci_t ra_dl;
  srslte_ra_dl_grant_t grant;

  cf_t pss_signal[SRSLTE_PSS_LEN];
  float sch_signal0[SRSLTE_SCH_LEN];
  float sch_signal1[SRSLTE_SCH_LEN];
  float sss_signal[SRSLTE_SSS_LEN];

  cf_t *slot1_symbols[MAX_NUMBER_OF_TBS_IN_A_SLOT][SRSLTE_MAX_PORTS];
  cf_t *subframe_symbols[MAX_NUMBER_OF_TBS_IN_A_SLOT][SRSLTE_MAX_PORTS];
  cf_t *preamble_symbols[MAX_NUMBER_OF_TBS_IN_A_SLOT];
  srslte_chest_dl_t est;
  srslte_dci_location_t locations[SRSLTE_NSUBFRAMES_X_FRAME][30];

  sem_t vphy_tx_frame_done_semaphore;

#if(ENABLE_WINDOWING==1)
  float *tx_window;
  float *rf_boosted_tx_window;
  uint32_t tx_window_length;
  uint32_t useful_length;
  uint32_t guard_band_length;
  uint32_t window_type;
#endif

  // ID of the timer used to check abnormal behaviours.
  timer_t vphy_tx_timer_id;

  uint32_t last_nof_subframes_to_tx;
  uint32_t last_mcs;

} vphy_tx_thread_context_t;

// *************** Declaration of functions ***************
int vphy_tx_start_thread(transceiver_args_t* const args, uint32_t vphy_id);

int vphy_tx_stop_thread(uint32_t vphy_id);

void vphy_tx_init_last_slot_control(uint32_t center_freq, int32_t tx_gain, uint32_t bw_idx, uint32_t radio_id, uint32_t default_channel, float freq_boost);

void vphy_tx_init_cell_parameters(vphy_tx_thread_context_t* const vphy_tx_thread_context, transceiver_args_t* const args);

int vphy_tx_initialize(transceiver_args_t* const args, unsigned char ***ciruclar_data_buffer_ptr);

int vphy_tx_uninitialize();

int vphy_tx_change_parameters(vphy_tx_thread_context_t* const vphy_tx_thread_context, slot_ctrl_t* const slot_ctrl);

void *vphy_tx_work(void* h);

void set_number_of_tx_offset_samples(int num_samples_to_offset);

int get_number_of_tx_offset_samples();

unsigned int vphy_tx_reverse_bits(register unsigned int x);

uint32_t vphy_tx_prbset_to_bitmask(uint32_t nof_prb);

int vphy_tx_update_radl(vphy_tx_thread_context_t* const vphy_tx_thread_context, uint32_t mcs, uint32_t nof_prb);

void vphy_tx_free_buffers();

void vphy_tx_base_init(uint16_t rnti);

void vphy_tx_base_free();

void vphy_tx_push_tx_slot_control_to_container(uint32_t vphy_id, slot_ctrl_t* const slot_ctrl);

void vphy_tx_pop_tx_slot_control_from_container(vphy_tx_thread_context_t* const vphy_tx_thread_context, slot_ctrl_t* const slot_ctrl);

bool vphy_tx_wait_container_not_empty(vphy_tx_thread_context_t* const vphy_tx_thread_context);

void vphy_tx_allocate_tx_buffer(size_t tx_data_buffer_length);

void vphy_tx_free_tx_buffer();

uint32_t vphy_tx_get_tx_data_buffer_counter();

void vphy_tx_increment_tx_data_buffer_counter();

void vphy_tx_print_slot_control(slot_ctrl_t* const slot_ctrl);

double vphy_tx_get_channel_center_freq(vphy_tx_thread_context_t* const vphy_tx_thread_context, uint32_t channel);

int vphy_tx_init_thread_context(vphy_tx_thread_context_t* const vphy_tx_thread_context, transceiver_args_t* const args);

void vphy_tx_free_tx_structs(vphy_tx_thread_context_t* const vphy_tx_thread_context);

void vphy_tx_init_last_tx_slot_control(vphy_tx_thread_context_t* const vphy_tx_thread_context, transceiver_args_t* const args);

int vphy_tx_init_tx_structs(vphy_tx_thread_context_t* const vphy_tx_thread_context, uint16_t rnti);

void vphy_tx_print_slot_control(slot_ctrl_t* const slot_ctrl);

timer_t* vphy_tx_get_timer_id(uint32_t vphy_id);

bool vphy_tx_wait_and_pop_tx_slot_control_from_container(vphy_tx_thread_context_t* const vphy_tx_thread_context, slot_ctrl_t* const slot_ctrl);

int vphy_tx_get_tx_slot_control_container_size(uint32_t vphy_id);

// ****************** Functions for Frame structure type II ********************
int vphy_tx_validate_tb_size(slot_ctrl_t* const slot_ctrl, uint32_t bw_idx);

void vphy_tx_change_allocation(vphy_tx_thread_context_t* const vphy_tx_thread_context, uint32_t req_mcs, uint32_t req_bw_idx);

#endif // _VPHY_TX_H_
