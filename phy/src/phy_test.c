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

#define MAX_TX_SLOTS 10000000

#define CHECK_RX_SEQUENCE 0

#define DISABLE_TX_TIMESTAMP 1

#define MAX_DATA_COUNTER_VALUE 256

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
} app_args_t;

typedef struct {
  bool run_application;
  app_args_t args;
  LayerCommunicator_handle handle;
  pthread_attr_t rx_side_thread_attr;
  pthread_t rx_side_thread_id;
  uint32_t *nof_tbs_per_slot;
} phy_app_context_t;

phy_app_context_t *app_ctx = NULL;

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

int startRxThread(phy_app_context_t *app_ctx);

int stopRxThread(phy_app_context_t *app_ctx);

static uint32_t get_tb_size(uint32_t bw_idx, uint32_t mcs);

void createRxStats(phy_stat_t *statistic, uint32_t numOfBytes, uchar *data);

int main(int argc, char *argv[]) {

  // Set priority to Channelizer thread.
  uhd_set_thread_priority(1.0, true);

  // Allocate memory for a new Application context object.
  app_ctx = (phy_app_context_t*)srslte_vec_malloc(sizeof(phy_app_context_t));
  // Check if memory allocation was correctly done.
  if(app_ctx == NULL) {
    printf("Error allocating memory for Application context object.\n");
    exit(-1);
  }

  // Parse received parameters.
  parseArgs(argc, argv, &app_ctx->args);

  // Initialize signal handler.
  initializeSignalHandler();

  char module_name[] = "MODULE_PHY";
  char target_name[] = "MODULE_MAC";

  uint32_t nof_tx_vphys = app_ctx->args.nof_tx_vphys; // Number of active Tx vPHYs, i.e., actual number of spawned threads.
  uint32_t nof_rx_vphys = app_ctx->args.nof_rx_vphys; // Number of active Rx vPHYs, i.e., actual number of spawned threads.
  uint32_t numOfBytesInOneSlot = get_tb_size(BW_IDX_OneDotFour-1, app_ctx->args.mcs); // Number of bytes for 1.4 MHz.

  printf("[Tx side] MCS: %d - numOfBytesInOneSlot: %d\n", app_ctx->args.mcs, numOfBytesInOneSlot);

  // Instantiate communicator module so that we can receive/transmit commands and data to PHY.
  communicator_initialization(module_name, target_name, &app_ctx->handle, nof_tx_vphys, nof_rx_vphys);

  sleep(2);

  // Start Rx thread.
  printf("[Main] Starting PHY Rx side thread...\n");
  startRxThread(app_ctx);

  sleep(2);

  // Generate fake received data.
  uchar data[numOfBytesInOneSlot];
  generateData(numOfBytesInOneSlot, data);

  // Create PHY Rx stats.
  phy_stat_t phy_rx_stat;
  createRxStats(&phy_rx_stat, numOfBytesInOneSlot, data);

  // Loop that keeps sending messages back to MAC.
  while(app_ctx->run_application) {

    communicator_send_phy_stat_message(app_ctx->handle, RX_STAT, &phy_rx_stat, NULL);

    usleep(1500);
  }

  // Wait Rx thread to finish its work.
  pthread_join(app_ctx->rx_side_thread_id, NULL);

  sleep(2);

  // Start Rx thread.
  printf("[Main] Stopping Rx side thread...\n");
  stopRxThread(app_ctx);

  sleep(2);

  // Free communicator related objects.
  communicator_uninitialization(&app_ctx->handle);

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
  statistic->mcs = 0;
  statistic->num_cb_total = 123;
  statistic->num_cb_err = 0;
  statistic->stat.rx_stat.gain = 10;
  statistic->stat.rx_stat.cqi = 15;
  statistic->stat.rx_stat.cfo = 0.1;
  statistic->stat.rx_stat.rssi = -12;
  statistic->stat.rx_stat.rsrp = 89;
  statistic->stat.rx_stat.rsrp = 56;
  statistic->stat.rx_stat.rsrq = 78;
  statistic->stat.rx_stat.length = numOfBytes;
  statistic->stat.rx_stat.data = data;
}

void generateData(uint32_t numOfBytes, uchar *data) {
  // Create some data.
  printf("Creating %d data bytes\n",numOfBytes);
  for(int i = 0; i < numOfBytes; i++) {
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
}

void parseArgs(int argc, char **argv, app_args_t *args) {
  int opt;
  setDefaultArgs(args);
  while((opt = getopt(argc, argv, "bgikmnprstRT0123456789")) != -1) {
    switch(opt) {
    case 'b':
      args->tx_gain = atoi(argv[optind]);
      printf("[Input argument] Tx gain: %d\n", args->tx_gain);
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
  phy_app_context_t *app_context = (phy_app_context_t *)h;
  phy_ctrl_t phy_ctrl;
  bool ret;

  // Set priority to Channelizer thread.
  uhd_set_thread_priority(1.0, true);

  // Loop until otherwise said.
  printf("[Rx side] Starting PHY Rx side thread loop...\n");
  while(app_context->run_application) {

    // Retrieve message sent by MAC.
    // Try to retrieve a message from the QUEUE. It waits for a specified amount of time before timing out.
    ret = communicator_get_low_queue_wait_for(app_context->handle, 1000, (void * const)&phy_ctrl, NULL);

    if(ret) {
      printf("Received %s message from MAC.\n", phy_ctrl.trx_flag==TRX_TX_ST ? "TRX_TX_ST":"TRX_RX_ST");
    }

  }

  printf("[Rx side] Leaving Rx side thread.\n");
  // Exit thread with result code.
  pthread_exit(NULL);
}

int startRxThread(phy_app_context_t *app_ctx) {
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

int stopRxThread(phy_app_context_t *app_ctx) {
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
