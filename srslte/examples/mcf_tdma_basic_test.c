#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/time.h>
#define __STDC_FORMAT_MACROS
#include <inttypes.h>
#include <time.h>
#include <semaphore.h>
#include <assert.h>
#include <signal.h>
#include <pthread.h>
#include <string.h>
#include <uhd.h>

#include "srslte/srslte.h"
#include "srslte/intf/intf.h"
#include "../phy/helpers.h"
#include "../../../../communicator/cpp/communicator_wrapper.h"

#define MAX_NOF_VPHYS 12

#define MAX_TX_SLOTS 100000

#define CHECK_RX_SEQUENCE 1

#define DISABLE_TX_TIMESTAMP 1

#define MAX_DATA_COUNTER_VALUE 256

#define FREQ_BOOST_AMP 1.0/2.0 // 1.0/120.0 (value for channel emulator)

#define ENABLE_DECODING_TIME_PROFILLING 1

typedef struct {
  uint32_t phy_bw_idx;
  uint32_t mcs;
  uint32_t rx_channel;
  uint32_t rx_gain;
  uint32_t tx_channel;
  uint32_t tx_gain;
  uint32_t nof_slots_to_tx;
  int64_t nof_packets_to_tx;
  uint32_t nof_tx_vphys;
  uint32_t nof_rx_vphys;
  uint32_t nof_slots;
  double radio_center_frequency;
  uint32_t radio_nof_prb;
  uint32_t vphy_nof_prb;
  double rf_boost;
  uint32_t radio_id;
  uint32_t frame_type;
} app_args_t;

typedef struct {
  bool run_application;
  app_args_t args;
  LayerCommunicator_handle handle;
  pthread_attr_t rx_side_thread_attr;
  pthread_t rx_side_thread_id;
  uint32_t *nof_tbs_per_slot;
  uint32_t max_data_counter_value;
} app_context_t;

app_context_t *app_ctx = NULL;

void print_received_basic_control(phy_ctrl_t *phy_control);

void generateData(uint32_t numOfBytes, uchar *data);

void setPhyControl(phy_ctrl_t* const phy_control, trx_flag_e trx_flag, uint32_t nof_slots);

void setSlotControl(slot_ctrl_t* const slot_ctrl, uint32_t vphy_id, uint64_t timestamp, uint64_t seq_num, uint32_t send_to, uint32_t bw_idx, uint32_t channel, uint32_t mcs, double freq_boost, uint32_t length, uchar *data);

void setHypervisorControl(hypervisor_ctrl_t* const hypervisor_ctrl, uint32_t radio_id, double radio_center_frequency, int32_t gain, uint32_t radio_nof_prb, uint32_t vphy_nof_prb, double rf_boost);

void addSlotCtrlToPhyCtrl(phy_ctrl_t* const phy_control, slot_ctrl_t* const slot_ctrl, uint32_t nof_slots, uint32_t largest_nof_tbs_in_slot);

void addHypervisorCtrlToPhyCtrl(phy_ctrl_t* const phy_control, hypervisor_ctrl_t* const hypervisor_ctrl);

void sendPhyCtrlToPhy(LayerCommunicator_handle handle, trx_flag_e trx_flag, phy_ctrl_t* const phy_control);

void calculateNofActiveVphysPerSlot(phy_ctrl_t* phy_ctrl);

void printPhyControl(phy_ctrl_t* const phy_control);

void sigIntHandler(int signo);

void initializeSignalHandler();

void setDefaultArgs(app_args_t *app_args);

void parseArgs(int argc, char **argv, app_args_t *app_args);

inline uint64_t get_host_time_now_us();

inline uint64_t get_host_time_now_ms();

void *rx_side_work(void *h);

int startRxThread(app_context_t *app_ctx);

int stopRxThread(app_context_t *app_ctx);

static uint32_t get_tb_size(uint32_t bw_idx, uint32_t mcs);

int main(int argc, char *argv[]) {

  // Allocate memory for a new Application context object.
  app_ctx = (app_context_t*)srslte_vec_malloc(sizeof(app_context_t));
  // Check if memory allocation was correctly done.
  if(app_ctx == NULL) {
    printf("Error allocating memory for Application context object.\n");
    exit(-1);
  }

  // Parse received parameters.
  parseArgs(argc, argv, &app_ctx->args);

  // Initialize signal handler.
  initializeSignalHandler();

  char module_name[] = "MODULE_MAC";
  char target_name[] = "MODULE_PHY";

  uint32_t nof_tx_slots_cnt = 0;
  uint32_t data_cnt = 0;

  uint32_t nof_tx_vphys = app_ctx->args.nof_tx_vphys; // Number of active Tx vPHYs, i.e., actual number of spawned threads.
  uint32_t nof_rx_vphys = app_ctx->args.nof_rx_vphys; // Number of active Rx vPHYs, i.e., actual number of spawned threads.
  uint32_t nof_slots = app_ctx->args.nof_slots; // Number of slot messages inside one phy control message.

  uint32_t nof_tbs_per_slot[MAX_NOF_VPHYS] = {4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4};
  uint32_t channel_list[MAX_NOF_VPHYS] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11};
  double freq_boost_lits[MAX_NOF_VPHYS] = {0.1, 0.1, 0.1, 0.1, 0.1, 0.1, 0.1, 0.1, 0.1, 0.1, 0.1, 0.1};
  uint32_t largest_nof_tbs_in_slot = 0, data_pos = 0;
  uint64_t timestamp;
  uint64_t seq_number = 0;

  phy_ctrl_t phy_control;
  slot_ctrl_t slot_ctrl[nof_slots];
  hypervisor_ctrl_t hypervisor_ctrl;
  uchar *slot_data[MAX_NUM_CONCURRENT_VPHYS] = {NULL};

  // Set context with the number of TBs per slot.
  app_ctx->nof_tbs_per_slot = nof_tbs_per_slot;

  // Set the maximum value for the data counter based on the number of vPHYs.
  if(MAX_DATA_COUNTER_VALUE % nof_tx_vphys == 0) {
    app_ctx->max_data_counter_value = MAX_DATA_COUNTER_VALUE;
  } else {
    app_ctx->max_data_counter_value = 240;
  }
  printf("[Main] Maximum data counter is: %d\n", app_ctx->max_data_counter_value);

  // Instantiate communicator module so that we can receive/transmit commands and data to PHY.
  communicator_initialization(module_name, target_name, &app_ctx->handle, nof_tx_vphys, nof_rx_vphys);

  sleep(2);

  // Start Rx thread.
  printf("[Main] Starting Rx side thread...\n");
  startRxThread(app_ctx);

  sleep(2);

  // Set high priority to send data.
  //uhd_set_thread_priority(1.0, true);

  // Create phy control message to control Tx chain.
  setPhyControl(&phy_control, TRX_TX_ST, nof_slots);

  // Retrieve current timestamp.
  timestamp = get_host_time_now_us();

  // Create slots.
  uint32_t nof_bytes = 0;
  for(uint32_t i = 0; i < nof_slots; i++) {
    // Calculate number of bytes per slot.
    if(app_ctx->args.mcs >= 28) {
      // Number of bytes for 1.4 MHz and MCS 27.
      nof_bytes = get_tb_size(BW_IDX_OneDotFour-1, app_ctx->args.mcs-1);
      // Number of bytes for 1.4 MHz and MCS 28.
      nof_bytes += (nof_tbs_per_slot[i]-1)*get_tb_size(BW_IDX_OneDotFour-1, app_ctx->args.mcs);
    } else {
      nof_bytes = nof_tbs_per_slot[i]*get_tb_size(BW_IDX_OneDotFour-1, app_ctx->args.mcs);
    }
    // Get the largest number of TBs in one of the slot messages.
    if(nof_tbs_per_slot[i] > largest_nof_tbs_in_slot) {
      largest_nof_tbs_in_slot = nof_tbs_per_slot[i];
    }
    // Allocate memory for data slots.
    slot_data[i] = (uchar*)srslte_vec_malloc(nof_bytes);
    // Generate slot data.
    generateData(nof_bytes, slot_data[i]);
    // Create slot structure.
    bzero(&slot_ctrl[i], sizeof(slot_ctrl_t));
    setSlotControl(&slot_ctrl[i], i, timestamp, 66, 0, BW_IDX_OneDotFour, channel_list[i], app_ctx->args.mcs, FREQ_BOOST_AMP*freq_boost_lits[i], nof_bytes, slot_data[i]);
  }

  // Add slots to PHY Control structure.
  addSlotCtrlToPhyCtrl(&phy_control, slot_ctrl, nof_slots, largest_nof_tbs_in_slot);

  // Create Hypervisor control structure.
  setHypervisorControl(&hypervisor_ctrl, app_ctx->args.radio_id, app_ctx->args.radio_center_frequency, app_ctx->args.tx_gain, app_ctx->args.radio_nof_prb, app_ctx->args.vphy_nof_prb, app_ctx->args.rf_boost);

  // Add Hypervisor control structure to PHY control.
  addHypervisorCtrlToPhyCtrl(&phy_control, &hypervisor_ctrl);

  // Send Hypervisor Control message to change Tx parameters.
  sendPhyCtrlToPhy(app_ctx->handle, TRX_RADIO_TX_ST, &phy_control);

  // Print PHY Control message.
  helpers_print_phy_control(&phy_control);

  uint64_t time_offset = ((uint64_t)largest_nof_tbs_in_slot)*1000 + 1000;

  // Send first slots some time in the future.
  timestamp = get_host_time_now_us() + 1000;

  while(app_ctx->run_application && nof_tx_slots_cnt < MAX_TX_SLOTS) {

    // Transmit different data for debug purposes.
    for(uint32_t i = 0; i < nof_slots; i++) {
      phy_control.slot_ctrl[i].vphy_id = i;
      phy_control.slot_ctrl[i].intf_id = i;
      phy_control.slot_ctrl[i].seq_number = seq_number;
      printf("[Tx side] vPHY ID: %d - seq. number: %d\n", i, phy_control.slot_ctrl[i].seq_number);
#if(DISABLE_TX_TIMESTAMP==1)
      phy_control.slot_ctrl[i].timestamp = 0;
#else
      phy_control.slot_ctrl[i].timestamp = timestamp;
#endif
      // Iterate over the slots.
      data_pos = 0;
      for(uint32_t k = 0; k < nof_tbs_per_slot[i]; k++) {
        phy_control.slot_ctrl[i].data[data_pos] = data_cnt;
        // Increment data position.
        if(app_ctx->args.mcs >= 28 && k == 0) {
          data_pos += get_tb_size(BW_IDX_OneDotFour-1, app_ctx->args.mcs-1);
        } else {
          data_pos += get_tb_size(BW_IDX_OneDotFour-1, app_ctx->args.mcs);
        }
        //printf("[Tx side] vPHY Tx Id: %d - slot: %d - data: %d\n", i, k, data_cnt);
        data_cnt = (data_cnt + 1) % app_ctx->max_data_counter_value;
      }
    }

    // Send Slot Control message to PHY.
    sendPhyCtrlToPhy(app_ctx->handle, TRX_TX_ST, &phy_control);

    // Sleep for the duration of the largest frame in the slot message.
    usleep(largest_nof_tbs_in_slot*1000);

    // Add some time to the current time.
    timestamp += time_offset;

    // Count number of transmitted packets.
    nof_tx_slots_cnt++;

    // Increment sequence number.
    seq_number++;
  }
  printf("Finished transmission.....\n");

  // Wait Rx thread to finish its work.
  pthread_join(app_ctx->rx_side_thread_id, NULL);

  sleep(2);

  // Start Rx thread.
  printf("[Main] Stopping Rx side thread...\n");
  stopRxThread(app_ctx);

  sleep(2);

  // Free communicator related objects.
  communicator_uninitialization(&app_ctx->handle);

  // Free alocated data.
  for(uint32_t i = 0; i < nof_slots; i++) {
    if(slot_data[i]) {
      free(slot_data[i]);
      slot_data[i] = NULL;
    } else {
      printf("Error: Slot data should not be NULL here!!!!\n");
    }
  }

  // Free memory used to store Application context object.
  if(app_ctx) {
    free(app_ctx);
    app_ctx = NULL;
  }

  printf("Bye...\n");

  return 0;
}

// This function returns timestamp with microseconds precision.
inline uint64_t get_host_time_now_us() {
  struct timespec host_timestamp;
  // Retrieve current time from host PC.
  clock_gettime(CLOCK_REALTIME, &host_timestamp);
  return (uint64_t)(host_timestamp.tv_sec*1000000LL) + (uint64_t)((double)host_timestamp.tv_nsec/1000LL);
}

// This function returns timestamp with miliseconds precision.
inline uint64_t get_host_time_now_ms() {
  struct timespec host_timestamp;
  // Retrieve current time from host PC.
  clock_gettime(CLOCK_REALTIME, &host_timestamp);
  return (uint64_t)(host_timestamp.tv_sec*1000LL) + (uint64_t)host_timestamp.tv_nsec/1000000LL;
}

void setPhyControl(phy_ctrl_t* const phy_control, trx_flag_e trx_flag, uint32_t nof_slots) {
  phy_control->trx_flag = trx_flag;
  phy_control->nof_slots = nof_slots;
}

void setSlotControl(slot_ctrl_t* const slot_ctrl, uint32_t vphy_id, uint64_t timestamp, uint64_t seq_num, uint32_t send_to, uint32_t bw_idx, uint32_t channel, uint32_t mcs, double freq_boost, uint32_t length, uchar *data) {
  slot_ctrl->vphy_id = vphy_id;
  slot_ctrl->timestamp = timestamp;
  slot_ctrl->seq_number = seq_num;
  slot_ctrl->send_to = send_to;
  slot_ctrl->intf_id = vphy_id;
  slot_ctrl->bw_idx = bw_idx;
  slot_ctrl->ch = channel;
  slot_ctrl->frame = 0;
  slot_ctrl->slot = 0;
  slot_ctrl->mcs = mcs;
  slot_ctrl->freq_boost = freq_boost;
  slot_ctrl->length = length;
  slot_ctrl->data = data;
  // Calculate the number of TB in this slot control message.
  slot_ctrl->nof_tb_in_slot = communicator_calculate_nof_subframes(slot_ctrl->mcs, communicator_get_bw_index(slot_ctrl->bw_idx), slot_ctrl->length);
}

void setHypervisorControl(hypervisor_ctrl_t* const hypervisor_ctrl, uint32_t radio_id, double radio_center_frequency, int32_t gain, uint32_t radio_nof_prb, uint32_t vphy_nof_prb, double rf_boost) {
  hypervisor_ctrl->radio_id = radio_id;
  hypervisor_ctrl->radio_center_frequency = radio_center_frequency;
  hypervisor_ctrl->gain = gain;
  hypervisor_ctrl->radio_nof_prb = radio_nof_prb;
  hypervisor_ctrl->vphy_nof_prb = vphy_nof_prb;
  hypervisor_ctrl->rf_boost = rf_boost;
}

// Add slots to PHY Control structure.
void addSlotCtrlToPhyCtrl(phy_ctrl_t* const phy_control, slot_ctrl_t* const slot_ctrl, uint32_t nof_slots, uint32_t largest_nof_tbs_in_slot) {
  memcpy((void*)phy_control->slot_ctrl, (void*)slot_ctrl, sizeof(slot_ctrl_t)*nof_slots);
  // Set largest_nof_tbs_in_slot for all slots.
  for(uint32_t i = 0; i < nof_slots; i++) {
    phy_control->slot_ctrl[i].slot_info.largest_nof_tbs_in_slot = largest_nof_tbs_in_slot;
  }
  calculateNofActiveVphysPerSlot(phy_control);
}

void calculateNofActiveVphysPerSlot(phy_ctrl_t* phy_ctrl) {
  // Number of active vPHYs in a given 1 ms period.
  uint32_t nof_active_vphys_per_slot[MAX_NUMBER_OF_TBS_IN_A_SLOT];
  // Zero the array before using it.
  bzero(nof_active_vphys_per_slot, sizeof(uint32_t)*MAX_NUMBER_OF_TBS_IN_A_SLOT);
  // Calculate number of active vPHYs per slot, i.e., per subframe.
  for(uint32_t i = 0; i < phy_ctrl->nof_slots; i++) {
    for(uint32_t k = 0; k < phy_ctrl->slot_ctrl[i].nof_tb_in_slot; k++) {
      nof_active_vphys_per_slot[k]++;
    }
  }
  // Make sure all vPHY Tx have the same set information on the number of active slots per 1ms.
  for(uint32_t i = 0; i < phy_ctrl->nof_slots; i++) {
    memcpy(phy_ctrl->slot_ctrl[i].slot_info.nof_active_vphys_per_slot, nof_active_vphys_per_slot, sizeof(uint32_t)*MAX_NUMBER_OF_TBS_IN_A_SLOT);
  }
}

// Add Hypervisor control structure to PHY control.
void addHypervisorCtrlToPhyCtrl(phy_ctrl_t* const phy_control, hypervisor_ctrl_t* const hypervisor_ctrl) {
  memcpy((void*)&phy_control->hypervisor_ctrl, (void*)hypervisor_ctrl, sizeof(hypervisor_ctrl_t));
}

void createRxStats(phy_stat_t *statistic, uint32_t numOfBytes, uchar *data) {
  statistic->seq_number = 678;
  statistic->status = PHY_SUCCESS;
  statistic->host_timestamp = 156;
  statistic->fpga_timestamp = 12;
  statistic->frame = 89;
  statistic->ch = 8;
  statistic->slot = 1;
  statistic->mcs = 100;
  statistic->num_cb_total = 123;
  statistic->num_cb_err = 1;
  statistic->stat.rx_stat.gain = 200;
  statistic->stat.rx_stat.cqi = 23;
  statistic->stat.rx_stat.rssi = 11;
  statistic->stat.rx_stat.rsrp = 89;
  statistic->stat.rx_stat.rsrp = 56;
  statistic->stat.rx_stat.rsrq = 78;
  statistic->stat.rx_stat.length = numOfBytes;
  statistic->stat.rx_stat.data = data;
}

void generateData(uint32_t numOfBytes, uchar *data) {
  // Create some data.
  printf("Creating %d data bytes\n",numOfBytes);
  for(uint32_t i = 0; i < numOfBytes; i++) {
    data[i] = (uchar)(rand() % 256);
  }
}

void sendPhyCtrlToPhy(LayerCommunicator_handle handle, trx_flag_e trx_flag, phy_ctrl_t* const phy_control) {
  phy_control->trx_flag = trx_flag;
  // Send PHY control with Hypervisor Tx control to the Hypervisor.
  communicator_send_phy_control(handle, phy_control);
}

void sigIntHandler(int signo) {
  if(signo == SIGINT) {
    app_ctx->run_application = false;
    printf("SIGINT received. Exiting...\n");
  }
}

void initializeSignalHandler() {
  // Enable loops and threads to run indefinitely.
  app_ctx->run_application = true;
  sigset_t sigset;
  sigemptyset(&sigset);
  sigaddset(&sigset, SIGINT);
  sigprocmask(SIG_UNBLOCK, &sigset, NULL);
  signal(SIGINT, sigIntHandler);
}

void setDefaultArgs(app_args_t *app_args) {
  app_args->nof_tx_vphys = 1;
  app_args->nof_rx_vphys = 1;
  app_args->nof_slots = 1;
  app_args->phy_bw_idx = BW_IDX_Five;
  app_args->mcs = 0;
  app_args->rx_channel = 0;
  app_args->rx_gain = 10;
  app_args->tx_channel = 0;
  app_args->tx_gain = 0;
  app_args->nof_slots_to_tx = 1;
  app_args->nof_packets_to_tx = -1;
  app_args->radio_center_frequency = 2000000000.0; // Given in Hz.
  app_args->radio_nof_prb = 100; // Number of PRBs for the radio transmissions with 20 MHz BW.
  app_args->vphy_nof_prb = 6;
  app_args->rf_boost = 0.8;
  app_args->radio_id = 0;
  app_args->frame_type = 1; // Frame type I: (0) - Frame type II: (1)
}

void parseArgs(int argc, char **argv, app_args_t *args) {
  int opt;
  setDefaultArgs(args);
  while((opt = getopt(argc, argv, "bfgikmnprstRT0123456789")) != -1) {
    switch(opt) {
    case 'b':
      args->tx_gain = atoi(argv[optind]);
      printf("[Input argument] Tx gain: %d\n", args->tx_gain);
      break;
    case 'f':
      args->radio_center_frequency = atof(argv[optind]);
      printf("[Input argument] Radio center frequency: %f\n", args->radio_center_frequency);
      break;
    case 'g':
      args->rx_gain = atoi(argv[optind]);
      printf("[Input argument] Rx gain: %d\n", args->rx_gain);
      break;
    case 'k':
      args->nof_packets_to_tx = atoi(argv[optind]);
      printf("[Input argument] Number of packets to transmit: %d\n", args->nof_packets_to_tx);
      break;
    case 'm':
      args->mcs = atoi(argv[optind]);
      printf("[Input argument] MCS: %d\n", args->mcs);
      break;
    case 'n':
      args->nof_slots_to_tx = atoi(argv[optind]);
      printf("[Input argument] Number of consecutive slots to be transmitted: %d\n", args->nof_slots_to_tx);
      break;
    case 'p':
      args->phy_bw_idx = helpers_get_bw_index_from_prb(atoi(argv[optind]));
      printf("[Input argument] PHY BW in PRB: %d - Mapped index: %d\n", atoi(argv[optind]), args->phy_bw_idx);
      break;
    case 'r':
      args->rx_channel = atoi(argv[optind]);
      printf("[Input argument] Rx channel: %d\n", args->rx_channel);
      break;
    case 's':
      args->nof_slots = atoi(argv[optind]);
      printf("[Input argument] Number of slots: %d\n", args->nof_slots);
      break;
    case 't':
      args->tx_channel = atoi(argv[optind]);
      printf("[Input argument] Tx channel: %d\n", args->tx_channel);
      break;
    case 'R':
      args->nof_rx_vphys = atoi(argv[optind]);
      printf("[Input argument] Number of Rx vPHYs: %d\n", args->nof_rx_vphys);
      break;
    case 'T':
      args->nof_tx_vphys = atoi(argv[optind]);
      printf("[Input argument] Number of Tx vPHYs: %d\n", args->nof_tx_vphys);
      break;
    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9':
      break;
    default:
      printf("Error parsing arguments...\n");
      exit(-1);
    }
  }
}

void *rx_side_work(void *h) {
  app_context_t *app_context = (app_context_t *)h;
  phy_stat_t phy_rx_stat;
  uchar data[10000];
  bool ret;
  uint32_t correct_cnt[MAX_NOF_VPHYS] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
  uint32_t error_cnt[MAX_NOF_VPHYS] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
  phy_ctrl_t phy_control;
  hypervisor_ctrl_t hypervisor_ctrl;
  uint32_t errors[MAX_NOF_VPHYS] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
  uint32_t nof_decoded[MAX_NOF_VPHYS] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
  uint32_t nof_detected[MAX_NOF_VPHYS] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
  uint32_t last_nof_decoded_counter[MAX_NOF_VPHYS] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
  uint32_t last_nof_detected_counter[MAX_NOF_VPHYS] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
  double prr[MAX_NOF_VPHYS] = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0};
#if(CHECK_RX_SEQUENCE==1)
  uint32_t sequence_error_cnt[MAX_NOF_VPHYS] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
  uint32_t cnt[MAX_NOF_VPHYS] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
  uint32_t loop_cnt[MAX_NOF_VPHYS] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
  uint32_t data_cnt[MAX_NOF_VPHYS];
  // Initialize expected data vector.
  for(uint32_t i = 0; i < MAX_NOF_VPHYS; i++) {
    data_cnt[i] = i*app_context->nof_tbs_per_slot[i];
  }
#endif
#if(ENABLE_DECODING_TIME_PROFILLING==1)
  double decoding_time[MAX_NOF_VPHYS] = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0};
  double synch_plus_decoding_time[MAX_NOF_VPHYS] = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0};
  double max_decoding_time[MAX_NOF_VPHYS] = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0};
  uint32_t max_decoding_time_cnt[MAX_NOF_VPHYS] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
#endif

  // Create Hypervisor control structure.
  setHypervisorControl(&hypervisor_ctrl, app_context->args.radio_id, app_context->args.radio_center_frequency, app_context->args.rx_gain, app_context->args.radio_nof_prb, app_context->args.vphy_nof_prb, app_context->args.rf_boost);
  // Add Hypervisor control structure to PHY control.
  addHypervisorCtrlToPhyCtrl(&phy_control, &hypervisor_ctrl);
  // Send Hypervisor Control message to change Rx parameters.
  sendPhyCtrlToPhy(app_context->handle, TRX_RADIO_RX_ST, &phy_control);

  // Assign address of data vector to PHY Rx stat structure.
  phy_rx_stat.stat.rx_stat.data = data;

  // Loop until otherwise said.
  printf("[Rx side] Starting Rx side thread loop...\n");
  while(app_context->run_application) {

    // Retrieve message sent by PHY.
    // Try to retrieve a message from the QUEUE. It waits for a specified amount of time before timing out.
    ret = communicator_get_high_queue_wait_for(app_context->handle, 500, (void * const)&phy_rx_stat, NULL);

    // If message is properly retrieved and parsed, then relay it to the correct module.
    if(app_context->run_application && ret) {

      if(phy_rx_stat.status == PHY_SUCCESS) {
#if(ENABLE_DECODING_TIME_PROFILLING==1)
        if(phy_rx_stat.stat.rx_stat.decoding_time >= 0) {
          decoding_time[phy_rx_stat.vphy_id] += phy_rx_stat.stat.rx_stat.decoding_time;
          // Get the maximum decoding time.
          if(phy_rx_stat.stat.rx_stat.decoding_time > max_decoding_time[phy_rx_stat.vphy_id]) {
            max_decoding_time[phy_rx_stat.vphy_id] = phy_rx_stat.stat.rx_stat.decoding_time;
            max_decoding_time_cnt[phy_rx_stat.vphy_id]++;
          }
        }
        if(phy_rx_stat.stat.rx_stat.synch_plus_decoding_time >= 0) {
          synch_plus_decoding_time[phy_rx_stat.vphy_id] += phy_rx_stat.stat.rx_stat.synch_plus_decoding_time;
        }
#endif
        printf("[Rx side] vPHY Rx Id: %d - TB #: %d - # Slots: %d - Slot #: %d - channel: %d - MCS: %d - peak: %1.2f - CQI: %d - RSSI: %1.2f [dBW] - Noise: %1.4f - CFO: %+2.2f [KHz] - nof received bytes: %d - data: %d\n", phy_rx_stat.vphy_id, (correct_cnt[phy_rx_stat.vphy_id]+1), phy_rx_stat.stat.rx_stat.nof_slots_in_frame, phy_rx_stat.stat.rx_stat.slot_counter, phy_rx_stat.ch, phy_rx_stat.mcs, phy_rx_stat.stat.rx_stat.peak_value, phy_rx_stat.stat.rx_stat.cqi, phy_rx_stat.stat.rx_stat.rssi, phy_rx_stat.stat.rx_stat.noise, phy_rx_stat.stat.rx_stat.cfo, phy_rx_stat.stat.rx_stat.length, phy_rx_stat.stat.rx_stat.data[0]);
        correct_cnt[phy_rx_stat.vphy_id]++;
#if(CHECK_RX_SEQUENCE==1)
        // Check if CQI is different from 15.
        //if(phy_rx_stat.stat.rx_stat.cqi != 15) {
        //  printf("[Rx side] vPHY Rx Id: %d - CQI != 15.........................\n", phy_rx_stat.vphy_id);
        //  exit(-1);
        //}
        // This piece of code was written to test with any number of vPHYs but with all of them transmitting 2 TBs each.
        if(phy_rx_stat.stat.rx_stat.data[0] != data_cnt[phy_rx_stat.vphy_id]) {
          sequence_error_cnt[phy_rx_stat.vphy_id]++;
          printf("[Rx side] vPHY Rx Id: %d - Channel #: %d - Received data: %d != Expected data: %d - # Errors: %d\n", phy_rx_stat.vphy_id, phy_rx_stat.ch, phy_rx_stat.stat.rx_stat.data[0], data_cnt[phy_rx_stat.vphy_id], sequence_error_cnt[phy_rx_stat.vphy_id]);
          if(app_context->args.frame_type == 1) {
            loop_cnt[phy_rx_stat.vphy_id] = (phy_rx_stat.stat.rx_stat.data[0] - (phy_rx_stat.vphy_id*phy_rx_stat.stat.rx_stat.nof_slots_in_frame + (phy_rx_stat.stat.rx_stat.slot_counter-1)))/(app_context->args.nof_rx_vphys*app_context->nof_tbs_per_slot[phy_rx_stat.vphy_id]);
            cnt[phy_rx_stat.vphy_id] = phy_rx_stat.stat.rx_stat.slot_counter - 1;
          }
          data_cnt[phy_rx_stat.vphy_id] = phy_rx_stat.stat.rx_stat.data[0];
          printf("Now data holds: %d\n",data_cnt[phy_rx_stat.vphy_id]); // zz4fap debug
        }
        if(cnt[phy_rx_stat.vphy_id] >= app_context->nof_tbs_per_slot[phy_rx_stat.vphy_id]-1) {
          loop_cnt[phy_rx_stat.vphy_id]++;
          data_cnt[phy_rx_stat.vphy_id] = phy_rx_stat.vphy_id*app_context->nof_tbs_per_slot[phy_rx_stat.vphy_id] + loop_cnt[phy_rx_stat.vphy_id]*app_context->args.nof_rx_vphys*app_context->nof_tbs_per_slot[phy_rx_stat.vphy_id];
          cnt[phy_rx_stat.vphy_id] = 0;
        } else {
          data_cnt[phy_rx_stat.vphy_id]++;
          cnt[phy_rx_stat.vphy_id]++;
        }
        if(data_cnt[phy_rx_stat.vphy_id] >= app_ctx->max_data_counter_value) {
          data_cnt[phy_rx_stat.vphy_id] = phy_rx_stat.vphy_id*app_context->nof_tbs_per_slot[phy_rx_stat.vphy_id];
          loop_cnt[phy_rx_stat.vphy_id] = 0;
        }
        //printf("vPHY Rx Id: %d - next expected data: %d\n", phy_rx_stat.vphy_id, data_cnt[phy_rx_stat.vphy_id]);
        // Store PRR for each one of the vPHYs.
        prr[phy_rx_stat.vphy_id] = (double)phy_rx_stat.num_cb_total/phy_rx_stat.stat.rx_stat.total_packets_synchronized;
        // Store errors for each one of the vPHYs.
        errors[phy_rx_stat.vphy_id] = (phy_rx_stat.stat.rx_stat.detection_errors+phy_rx_stat.stat.rx_stat.decoding_errors);
        // Store number of correctly decoded packets.
        if(nof_decoded[phy_rx_stat.vphy_id] == 0 && phy_rx_stat.num_cb_total > 1) {
          last_nof_decoded_counter[phy_rx_stat.vphy_id] = phy_rx_stat.num_cb_total - 1;
        }
        nof_decoded[phy_rx_stat.vphy_id] = phy_rx_stat.num_cb_total - last_nof_decoded_counter[phy_rx_stat.vphy_id];
        // Store number of synchronized packets, i.e., whatever has correlation peak higher than the threashold.
        if(nof_detected[phy_rx_stat.vphy_id] == 0 && phy_rx_stat.stat.rx_stat.total_packets_synchronized > 1) {
          last_nof_detected_counter[phy_rx_stat.vphy_id] = phy_rx_stat.stat.rx_stat.total_packets_synchronized - 1;
        }
        nof_detected[phy_rx_stat.vphy_id] = phy_rx_stat.stat.rx_stat.total_packets_synchronized - last_nof_detected_counter[phy_rx_stat.vphy_id];
#endif
      } else if(phy_rx_stat.status == PHY_ERROR) {
        printf("[Rx side] vPHY Rx Id: %d - Detection errors: %d - Channel: %d - CFO: %+2.2f [kHz] - Peak value: %1.2f - RSSI: %3.2f [dBm] - Decoded CFI: %d - Found DCI: %d - Last NOI: %d - Noise: %1.4f - Decoding errors: %d\n",phy_rx_stat.vphy_id,phy_rx_stat.stat.rx_stat.detection_errors,phy_rx_stat.ch,phy_rx_stat.stat.rx_stat.cfo,phy_rx_stat.stat.rx_stat.peak_value,phy_rx_stat.stat.rx_stat.rssi, phy_rx_stat.stat.rx_stat.decoded_cfi, phy_rx_stat.stat.rx_stat.found_dci, phy_rx_stat.stat.rx_stat.last_noi, phy_rx_stat.stat.rx_stat.noise, phy_rx_stat.stat.rx_stat.decoding_errors);
        error_cnt[phy_rx_stat.vphy_id]++;
      }
    }
  }

  uint32_t total_correct_cnt = 0, total_error_cnt = 0;
  for(uint32_t i = 0; i < app_context->args.nof_rx_vphys; i++) {
    total_correct_cnt += correct_cnt[i];
    total_error_cnt += error_cnt[i];
    printf("[Rx side] vPHY Rx Id: %d - Total # of Packets: %d - Total # of Errors: %d\n", i, correct_cnt[i], sequence_error_cnt[i]);
  }

  printf("\n\n[Rx side] # Correct pkts: %d - # Error pkts: %d\n", total_correct_cnt, total_error_cnt);

#if(ENABLE_DECODING_TIME_PROFILLING==1)
  for(uint32_t i = 0; i < app_context->args.nof_rx_vphys; i++) {
    printf("[Rx side] vPHY Rx Id: %d - # of Measurements: %d - MCS: %d - Avg. Synch + Decoding time: %1.4f [ms] - Avg. Decoding time: %1.4f [ms] - Max. Decoding time (%d): %1.4f [ms]\n", i, correct_cnt[i], app_context->args.mcs, synch_plus_decoding_time[i]/correct_cnt[i], decoding_time[i]/correct_cnt[i], max_decoding_time_cnt[i], max_decoding_time[i]);
  }
#endif

  printf("\n\n");
  for(uint32_t i = 0; i < app_context->args.nof_rx_vphys; i++) {
    printf("[Rx side] vPHY Rx Id: %d - MCS: %d - PRR: %1.2f - # Decoded: %d - # Detected[0]: %d - # errors: %d\n", i, app_context->args.mcs, prr[i], nof_decoded[i], nof_detected[i], errors[i]);
  }

  printf("[Rx side] Leaving Rx side thread.\n");
  // Exit thread with result code.
  pthread_exit(NULL);
}

int startRxThread(app_context_t *app_ctx) {
  // Enable Rx side thread.
  app_ctx->run_application = true;
  // Create thread attr and Id.
  pthread_attr_init(&app_ctx->rx_side_thread_attr);
  pthread_attr_setdetachstate(&app_ctx->rx_side_thread_attr, PTHREAD_CREATE_JOINABLE);
  // Create thread.
  int rc = pthread_create(&app_ctx->rx_side_thread_id, &app_ctx->rx_side_thread_attr, rx_side_work, (void *)app_ctx);
  if(rc) {
    printf("[Rx side] Return code from Rx side pthread_create() is %d\n", rc);
    return -1;
  }
  return 0;
}

int stopRxThread(app_context_t *app_ctx) {
  app_ctx->run_application = false; // Stop Rx side thread.
  pthread_attr_destroy(&app_ctx->rx_side_thread_attr);
  int rc = pthread_join(app_ctx->rx_side_thread_id, NULL);
  if(rc) {
    printf("[Rx side] Return code from Rx side pthread_join() is %d\n", rc);
    return -1;
  }
  return 0;
}

// MCS TB size mapping table
// how many bytes (TB size) in one slot under MCS 0~28 (this slot is our MF-TDMA slot, not LTE slot, here it refers to 1ms length!)
static const unsigned int num_of_bytes_per_slot_versus_mcs[6][29] = {{19,26,32,41,51,63,75,89,101,117,117,129,149,169,193,217,225,225,241,269,293,325,349,373,405,437,453,469,549}, // Values for 1.4 MHz BW
{49,65,81,109,133,165,193,225,261,293,293,333,373,421,485,533,573,573,621,669,749,807,871,935,999,1063,1143,1191,1383}, // Values for 3 MHz BW
{85,113,137,177,225,277,325,389,437,501,501,549,621,717,807,903,967,967,999,1143,1239,1335,1431,1572,1692,1764,1908,1980,2292}, // Values for 5 MHz BW
{173,225,277,357,453,549,645,775,871,999,999,1095,1239,1431,1620,1764,1908,1908,2052,2292,2481,2673,2865,3182,3422,3542,3822,3963,4587}, // Values for 10 MHz BW
{261,341,421,549,669,839,967,1143,1335,1479,1479,1620,1908,2124,2385,2673,2865,2865,3062,3422,3662,4107,4395,4736,5072,5477,5669,5861,6882}, // Values for 15 MHz BW
{349,453,573,717,903,1095,1287,1527,1764,1980,1980,2196,2481,2865,3182,3542,3822,3822,4107,4587,4904,5477,5861,6378,6882,7167,7708,7972,9422}}; // Values for 20 MHz BW

// This function is used to get the Transport Block size in bytes.
static uint32_t get_tb_size(uint32_t bw_idx, uint32_t mcs) {
  return num_of_bytes_per_slot_versus_mcs[bw_idx][mcs];
}
