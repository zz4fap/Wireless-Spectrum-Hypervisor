#ifndef _HYPERVISOR_TX_H_
#define _HYPERVISOR_TX_H_

#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <semaphore.h>

#include "srslte/srslte.h"

#include "../../../../communicator/cpp/communicator_wrapper.h"

#include "helpers.h"

#if(ENABLE_SW_RF_MONITOR==1)
#include "../rf_monitor/lbt.h"
#endif

// *********** Defintion of flags ***********
// TX local offset.
#define PHY_TX_LO_OFFSET -42.0e6

// Number of phy control structures that can be stored in the circular buffer (CB) without reaching its end.
#define NOF_PHY_CTRL_IN_CB 10000

// Set the number of zeros to be padded before the slot.
#define FIX_TX_OFFSET_SAMPLES 0

// Number of padding zeros after the end of the last subframe.
// OBS.1: This value MUST be an integer multiple of the number of channels used by the channelizer. In the current implementation it is set to 12 channels.
// OBS.2: 24 is the best value found so far.
#define NOF_PADDING_ZEROS 24

#define ENABLE_HYPER_TX_PRINTS 1

#define WRITE_RESOURCE_GRID_INTO_FILE 0

#define ENABLE_TEST_SIGNALS 0

#define WRITE_TX_SUBFRAME_INTO_FILE 0 // Enbale or disable dumping of Tx samples.

// *********** Defintion of debugging macros ***********
#define HYPER_TX_PRINT(_fmt, ...) do { if(ENABLE_HYPER_TX_PRINTS && scatter_verbose_level >= 0) { \
  fprintf(stdout, "[HYPER TX PRINT]: " _fmt, __VA_ARGS__); } } while(0)

#define HYPER_TX_DEBUG(_fmt, ...) do { if(ENABLE_HYPER_TX_PRINTS && scatter_verbose_level >= SRSLTE_VERBOSE_DEBUG) { \
  fprintf(stdout, "[HYPER TX DEBUG]: " _fmt, __VA_ARGS__); } } while(0)

#define HYPER_TX_INFO(_fmt, ...) do { if(ENABLE_HYPER_TX_PRINTS && scatter_verbose_level >= SRSLTE_VERBOSE_INFO) { \
  fprintf(stdout, "[HYPER TX INFO]: " _fmt, __VA_ARGS__); } } while(0)

#define HYPER_TX_ERROR(_fmt, ...) do { fprintf(stdout, "[HYPER TX ERROR]: " _fmt, __VA_ARGS__); } while(0)

#define HYPER_TX_PRINT_TIME(_fmt, ...) do { if(ENABLE_HYPER_TX_PRINTS && scatter_verbose_level >= 0) { \
  char date_time_str[30]; helpers_get_data_time_string(date_time_str); \
  fprintf(stdout, "[HYPER TX PRINT]: %s - " _fmt, date_time_str, __VA_ARGS__); } } while(0)

#define HYPER_TX_DEBUG_TIME(_fmt, ...) do { if(ENABLE_HYPER_TX_PRINTS && scatter_verbose_level >= SRSLTE_VERBOSE_DEBUG) { \
  char date_time_str[30]; helpers_get_data_time_string(date_time_str); \
  fprintf(stdout, "[HYPER TX DEBUG]: %s - " _fmt, date_time_str, __VA_ARGS__); } } while(0)

#define HYPER_TX_INFO_TIME(_fmt, ...) do { if(ENABLE_HYPER_TX_PRINTS && scatter_verbose_level >= SRSLTE_VERBOSE_INFO) { \
  char date_time_str[30]; helpers_get_data_time_string(date_time_str); \
  fprintf(stdout, "[HYPER TX INFO]: %s - " _fmt, date_time_str, __VA_ARGS__); } } while(0)

#define HYPER_TX_ERROR_TIME(_fmt, ...) do { char date_time_str[30]; helpers_get_data_time_string(date_time_str); \
  fprintf(stdout, "[HYPER TX ERROR]: %s - " _fmt, date_time_str, __VA_ARGS__); } while(0)

// *********** Definition of types ***********
typedef struct {
  srslte_rf_t* hypervisor_tx_rf;

  cf_t* output_buffer;

  // Set of buffers used store several subframers so that it is not necessary for a vPHY to wait others to finish.
  cf_t* subframe_buffer[MAX_NUMBER_OF_TBS_IN_A_SLOT];
  // Counter used to count the number of processed subframes in 1 ms-long slot.
  uint32_t subframe_counter[MAX_NUMBER_OF_TBS_IN_A_SLOT];

  srslte_ofdm_t ifft;

  srslte_cell_t phy_tx_cell_info;

  hypervisor_ctrl_t last_tx_hypervisor_ctrl;

  int number_of_re_in_sf;
  int sf_n_samples;
  int number_of_tx_offset_samples;

  bool enable_ifft_adjust;

  uint32_t number_of_subframe_samples;

  pthread_attr_t hypervisor_tx_thread_attr;
  pthread_t hypervisor_tx_thread_id;
  // Variable used to stop hypervisor tx thread.
  volatile sig_atomic_t run_hypervisor_tx_thread;

  // Mutex used to synchronize access to the Hypervisor Tx thread.
  pthread_mutex_t hypervisor_tx_mutex;
  // Condition variable used to enable data transmission.
  pthread_cond_t hypervisor_tx_cv;

  bool add_preamble_to_front;

  bool is_lbt_enabled;

  bool use_std_carrier_sep;

  uint32_t modulate_with_zeros;

  // Mutex used to synchronize the access to the active vPHYs per slot array.
  pthread_mutex_t vphy_modulation_done_mutex;
  // Conditional variable used to synchronize the access to the active vPHYs per slot array.
  pthread_cond_t vphy_modulation_done_cv;

  // semaphore FIFO with all the sempahores from vPHY Tx threads waiting for the OFDM modulation to be finished.
  vphy_tx_semaphore_cb_handle subframe_mod_done_sem_fifo;

  // semaphore FIFO with all the sempahores from vPHYs that finished earlier and do not have more TBs to process.
  vphy_tx_semaphore_cb_handle frame_mod_done_sem_fifo;

  uint32_t mod_with_zeros_cnt;

  phy_control_cb_handle phy_ctrl_handle;

  // ID of the timer used to check abnormal behaviours.
  timer_t hyper_tx_timer_id;

  uint32_t num_of_tx_vphys;

#if(ENABLE_FREQ_SHIFT_CORRECTION==1)
  srslte_cexptab_t freq_shift_waveform_obj;
  cf_t *freq_shift_waveform;
#endif

} hypervisor_tx_t;

// *********** Declaration of functions ***********
int hypervisor_tx_initialize(transceiver_args_t* const args, srslte_rf_t* const rf);

int hypervisor_tx_uninitialize();

int hypervisor_tx_init_handle(transceiver_args_t* const args, srslte_rf_t* const rf);

void hypervisor_tx_init_cell_parameters(transceiver_args_t* const args);

void hypervisor_tx_init_last_hypervisor_ctrl(transceiver_args_t* const args);

int hypervisor_tx_init_buffers(transceiver_args_t* const args);

void hypervisor_tx_free_buffers();

void hypervisor_tx_register_frame_mod_done_sem(sem_t* const semaphore);

void hypervisor_tx_transfer_to_usrp_done_notify_vphy_tx_threads();

void hypervisor_tx_notify_vphy_tx_threads_frame_mod_done(sem_t *frame_done_semaphore);

bool hypervisor_tx_inform_subframe_modulation_done(slot_ctrl_t* const slot_ctrl, uint32_t subframe_cnt);

void *hypervisor_tx_work(void* h);

void set_number_of_tx_offset_samples(int num_samples_to_offset);

int get_number_of_tx_offset_samples();

float hypervisor_tx_set_gain(float tx_gain);

void hypervisor_tx_set_center_frequency(double tx_center_frequency);

void hypervisor_tx_set_radio_center_freq_and_gain(transceiver_args_t* const args);

void hypervisor_tx_set_channel_center_freq_and_gain(float tx_gain, double center_frequency, double radio_sampling_rate, double vphy_sampling_rate, uint32_t tx_channel);

void hypervisor_tx_set_sample_rate(uint32_t radio_nof_prb, bool use_std_carrier_sep);

cf_t** const hypervisor_tx_get_subframe_buffer();

void hypervisor_tx_change_parameters(hypervisor_ctrl_t* const hypervisor_ctrl);

void hypervisor_tx_transfer_to_usrp_done();

bool hypervisor_tx_wait_container_not_empty();

void hypervisor_tx_get_phy_control(phy_ctrl_t* const phy_ctrl);

void hypervisor_tx_push_phy_control_to_container(phy_ctrl_t* const phy_ctrl);

void hypervisor_tx_transmit(slot_ctrl_t* const slot_ctrl, uint32_t subframe_cnt);

void hypervisor_tx_notify_all_vphy_tx_threads_waiting_subframe_mod_done_semaphore();

void hypervisor_tx_notify_all_vphy_tx_threads_waiting_frame_mod_done_semaphore();

void hypervisor_tx_notify_all_threads_waiting_for_semaphores();

int hypervisor_tx_send(hypervisor_tx_t* hyper_tx_handle, void *data, int nof_samples, time_t full_secs, double frac_secs, bool has_time_spec, bool blocking, bool is_start_of_burst, bool is_end_of_burst, bool is_lbt_enabled, void *lbt_stats_void_ptr, size_t channel);

timer_t* hypervisor_tx_get_timer_id();

#endif // _HYPERVISOR_TX_H_
