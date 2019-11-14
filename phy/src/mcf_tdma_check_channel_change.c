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

#include "srslte/srslte.h"
#include "srslte/intf/intf.h"
#include "../phy/helpers.h"
#include "../../../../communicator/cpp/communicator_wrapper.h"

#define MAX_TX_SLOTS 10000

typedef struct {
  uint32_t nof_vphys;
  uint32_t nof_slots;
} app_ctx_t;

void print_received_basic_control(phy_ctrl_t *phy_control);

void generateData(uint32_t numOfBytes, uchar *data);

void setPhyControl(phy_ctrl_t* const phy_control, trx_flag_e trx_flag, uint32_t nof_slots);

void setSlotControl(slot_ctrl_t* const slot_ctrl, uint32_t vphy_id, uint64_t timestamp, uint64_t seq_num, uint32_t send_to, uint32_t bw_idx, uint32_t channel, uint32_t mcs, double freq_boost, uint32_t length, uchar *data);

void setHypervisorControl(hypervisor_ctrl_t* const hypervisor_ctrl, double radio_center_frequency, int32_t gain, uint32_t radio_nof_prb, uint32_t vphy_nof_prb, double rf_boost);

void addSlotCtrlToPhyCtrl(phy_ctrl_t* const phy_control, slot_ctrl_t* const slot_ctrl, uint32_t nof_slots, uint32_t largest_nof_tbs_in_slot);

void addHypervisorCtrlToPhyCtrl(phy_ctrl_t* const phy_control, hypervisor_ctrl_t* const hypervisor_ctrl);

void sendPhyCtrlToPhy(LayerCommunicator_handle handle, trx_flag_e trx_flag, phy_ctrl_t* const phy_control);

void calculateNofActiveVphysPerSlot(phy_ctrl_t* phy_ctrl);

void printPhyControl(phy_ctrl_t* const phy_control);

void sigIntHandler(int signo);

void initializeSignalHandler();

void setDeafultArgs(app_ctx_t *app_ctx);

void parseArgs(int argc, char **argv, app_ctx_t *app_ctx);

inline uint64_t get_host_time_now_us();

inline uint64_t get_host_time_now_ms();

bool go_exit = false;

int main(int argc, char *argv[]) {

  // Parse received parameters.
  app_ctx_t app_ctx;
  parseArgs(argc, argv, &app_ctx);

  // Initialize signal handler.
  initializeSignalHandler();

  LayerCommunicator_handle handle;
  char module_name[] = "MODULE_MAC";
  char target_name[] = "MODULE_PHY";

  uint32_t nof_tx_slots_cnt = 0;

  uint32_t nof_vphys = app_ctx.nof_vphys; // Number of active vPHYs, i.e., actual number of spawned threads.
  uint32_t nof_slots = app_ctx.nof_slots; // Number of slot messages inside one phy control message.

  double freq_boost_lits[12] = {0.1, 0.125, 0.5, 0.3, 0.4, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5};
  uint32_t largest_nof_tbs_in_slot = 0;

  double radio_center_frequency = 2000000000.0; // Given in Hz.
  int32_t gain = 25; // Given in dB.
  uint32_t radio_nof_prb = 100; // Number of PRBs for the radio transmissions with 20 MHz BW.
  uint32_t vphy_nof_prb = 6;
  double rf_boost = 0.8;

  phy_ctrl_t phy_control;
  slot_ctrl_t slot_ctrl[nof_slots];
  hypervisor_ctrl_t hypervisor_ctrl;

  // Instantiate communicator module so that we can receive/transmit commands and data to PHY.
  communicator_initialization(module_name, target_name, &handle, nof_vphys, nof_vphys);

  sleep(2);

  // Create phy control message to control Tx chain.
  setPhyControl(&phy_control, TRX_RX_ST, nof_slots);

  // Set slot control.
  setSlotControl(&slot_ctrl[0], 0, 0, 66, 0, BW_IDX_OneDotFour, 0, 0, freq_boost_lits[0], 1, NULL);

  // Add slots to PHY Control structure.
  addSlotCtrlToPhyCtrl(&phy_control, slot_ctrl, nof_slots, largest_nof_tbs_in_slot);

  // Create Hypervisor control structure.
  setHypervisorControl(&hypervisor_ctrl, radio_center_frequency, gain, radio_nof_prb, vphy_nof_prb, rf_boost);

  // Add Hypervisor control structure to PHY control.
  addHypervisorCtrlToPhyCtrl(&phy_control, &hypervisor_ctrl);

  // Send Hypervisor Control message to change Tx parameters.
  //sendPhyCtrlToPhy(handle, TRX_RADIO_RX_ST, &phy_control);

  // Print PHY Control message.
  helpers_print_phy_control(&phy_control);

  while(!go_exit) {

    printf("Channel: %d\n", phy_control.slot_ctrl[0].ch);

    // Send Slot Control message to PHY.
    sendPhyCtrlToPhy(handle, TRX_RX_ST, &phy_control);

    sleep(5);
    //usleep(300);

    // Add some time to the current time.
    phy_control.slot_ctrl[0].ch = rand() % 12;

    // Count number of transmitted packets.
    nof_tx_slots_cnt++;
  }

  sleep(2);

  // Free communicator related objects.
  communicator_uninitialization(&handle);

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
  if(data != NULL) {
    // Calculate the number of TB in this slot control message.
    slot_ctrl->nof_tb_in_slot = communicator_calculate_nof_subframes(slot_ctrl->mcs, communicator_get_bw_index(slot_ctrl->bw_idx), slot_ctrl->length);
  } else {
    // Calculate the number of TB in this slot control message.
    slot_ctrl->nof_tb_in_slot = 0;
  }
}

void setHypervisorControl(hypervisor_ctrl_t* const hypervisor_ctrl, double radio_center_frequency, int32_t gain, uint32_t radio_nof_prb, uint32_t vphy_nof_prb, double rf_boost) {
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
  statistic->mcs = 18;
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
    go_exit = true;
    printf("SIGINT received. Exiting...\n");
  }
}

void initializeSignalHandler() {
  sigset_t sigset;
  sigemptyset(&sigset);
  sigaddset(&sigset, SIGINT);
  sigprocmask(SIG_UNBLOCK, &sigset, NULL);
  signal(SIGINT, sigIntHandler);
}

void setDeafultArgs(app_ctx_t *app_ctx) {
  app_ctx->nof_vphys = 1;
  app_ctx->nof_slots = 1;
}

void parseArgs(int argc, char **argv, app_ctx_t *app_ctx) {
  int opt;
  setDeafultArgs(app_ctx);
  while((opt = getopt(argc, argv, "Vs")) != -1) {
    switch(opt) {
    case 'V':
      app_ctx->nof_vphys = atoi(argv[optind]);
      printf("Number of vPHYs: %d\n", app_ctx->nof_vphys);
      break;
    case 's':
      app_ctx->nof_slots = atoi(argv[optind]);
      printf("Number of slots: %d\n", app_ctx->nof_slots);
      break;
    default:
      exit(-1);
    }
  }
}
