#include "hypervisor_control.h"

#ifndef DISABLE_RF
#include "srslte/rf/rf.h"
#include "srslte/rf/rf_utils.h"

static srslte_rf_t rf;

#else
#error Compiling PHY with no RF support. Add RF support.
#endif

void hypervisor_ctrl_initialize(transceiver_args_t* const args) {
  // Open RF device (USRP).
  hypervisor_ctrl_open_rf_device(args->rf_args);
  // Set master clock rate.
  // We always set the master clock rate to the maximum one, i.e., for a system with 100 PRB (20 MHz BW).
  hypervisor_ctrl_set_master_clock_rate(args->radio_nof_prb);
  // Set FPGA time to the current host time so that we can use the host time (ntp) to synchronize among nodes.
  hypervisor_ctrl_set_fpga_time();
  // Initialize Hypervisor Rx.
  hypervisor_rx_initialize(args, &rf);
  HYPER_CTRL_DEBUG("Hypervisor Rx initialized\n", 0);
  // Initialize Hypervisor Tx.
  hypervisor_tx_initialize(args, &rf);
  HYPER_CTRL_DEBUG("Hypervisor Tx initialized\n", 0);
}

void hypervisor_ctrl_uninitialize() {
  // Uninitialize the Hypervisor Tx.
  hypervisor_tx_uninitialize();
  HYPER_CTRL_DEBUG("Hypervisor Tx uninitialization done.\n", 0);
  // Uninitialize the Hypervisor Rx.
  hypervisor_rx_uninitialize();
  HYPER_CTRL_DEBUG("Hypervisor Rx uninitialization done.\n", 0);
  // Close the USRP device.
  hypervisor_ctrl_close_rf_device();
  HYPER_CTRL_DEBUG("RF device uninitialization done.\n", 0);
}

void hypervisor_ctrl_open_rf_device(char* const rf_args) {
  // Open USRP.
  HYPER_CTRL_PRINT("Opening RF device...\n",0);
  if(srslte_rf_open(&rf, rf_args)) {
    HYPER_CTRL_ERROR("Error opening rf.\n",0);
    exit(-1);
  }
}

void hypervisor_ctrl_close_rf_device() {
  if(srslte_rf_close(&rf)) {
    HYPER_CTRL_ERROR("Error closing rf.\n",0);
    exit(-1);
  }
  HYPER_CTRL_INFO("srslte_rf_close done!\n",0);
}

void hypervisor_ctrl_set_master_clock_rate(uint32_t radio_nof_prb) {
  // Calculate the master clock rate for a bandwidth of 20 MHz, i.e., 100 PRB.
  int srate = srslte_sampling_freq_hz(radio_nof_prb);
  // Set the master clock rate.
  srslte_rf_set_master_clock_rate(&rf, srate);
  HYPER_CTRL_PRINT("Set master clock rate to: %.2f [MHz]\n", (float)srate/1000000);
}

srslte_rf_t* hypervisor_ctrl_get_rf_object() {
  return &rf;
}

// Check FPGA time and ajust it.
inline void hypervisor_ctrl_check_and_adjust_fpga_time() {
  // Adjust FPGA time if it is different from host pc.
  uint64_t fpga_time = helpers_get_fpga_timestamp_us(&rf);
  uint64_t host_time = helpers_get_host_timestamp_us();
  if((host_time-fpga_time) > 200) {
    hypervisor_ctrl_set_fpga_time();
    HYPER_CTRL_DEBUG("FPGA: %" PRIu64 " - Host: %" PRIu64 " - diff: %d\n",fpga_time,host_time,(host_time-fpga_time));
  }
}

// Set FPGA time now to host time.
inline void hypervisor_ctrl_set_fpga_time() {
  struct timespec host_time_now;
  // Retrieve current time from host PC.
  clock_gettime(CLOCK_REALTIME, &host_time_now);
  srslte_rf_set_time_now(&rf, host_time_now.tv_sec, (double)host_time_now.tv_nsec/1000000000LL);
  HYPER_CTRL_DEBUG("FPGA Time set to: %f\n",((double)(uintmax_t)host_time_now.tv_sec + (double)host_time_now.tv_nsec/1000000000LL));
}

void hypervisor_ctrl_set_vphy_rx_running_flag(bool vphy_rx_running) {
  hypervisor_rx_set_vphy_rx_running_flag(vphy_rx_running);
}

timer_t* hypervisor_ctrl_get_hyper_tx_timer_id() {
  return hypervisor_tx_get_timer_id();
}
