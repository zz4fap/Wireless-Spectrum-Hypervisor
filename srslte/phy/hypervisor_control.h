#ifndef _HYPERVISOR_CONTROL_H_
#define _HYPERVISOR_CONTROL_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include <math.h>
#include <sys/time.h>
#include <assert.h>
#include <signal.h>
#include <pthread.h>
#include <semaphore.h>
#include <float.h>
#define __STDC_FORMAT_MACROS
#include <inttypes.h>
#include <errno.h>
#include <time.h>
#include <stddef.h>
#include <stdint.h>

#include "srslte/srslte.h"
#include "srslte/intf/intf.h"

#include "../../../../communicator/cpp/communicator_wrapper.h"

#include "helpers.h"
#include "hypervisor_tx.h"
#include "hypervisor_rx.h"

// *********** Definition of debug macros ***********
#define ENABLE_HYPER_CTRL_PRINTS 1

#define HYPER_CTRL_PRINT(_fmt, ...) do { if(ENABLE_HYPER_CTRL_PRINTS && scatter_verbose_level >= 0) { \
  fprintf(stdout, "[HYPER CTRL PRINT]: " _fmt, __VA_ARGS__); } } while(0)

#define HYPER_CTRL_DEBUG(_fmt, ...) do { if(ENABLE_HYPER_CTRL_PRINTS && scatter_verbose_level >= SRSLTE_VERBOSE_DEBUG) { \
  fprintf(stdout, "[HYPER CTRL DEBUG]: " _fmt, __VA_ARGS__); } } while(0)

#define HYPER_CTRL_INFO(_fmt, ...) do { if(ENABLE_HYPER_CTRL_PRINTS && scatter_verbose_level >= SRSLTE_VERBOSE_INFO) { \
  fprintf(stdout, "[HYPER CTRL INFO]: " _fmt, __VA_ARGS__); } } while(0)

#define HYPER_CTRL_ERROR(_fmt, ...) do { fprintf(stdout, "[HYPER CTRL ERROR]: " _fmt, __VA_ARGS__); } while(0)

#define HYPER_CTRL_PRINT_TIME(_fmt, ...) do { if(ENABLE_HYPER_CTRL_PRINTS && scatter_verbose_level >= 0) { \
  char date_time_str[30]; helpers_get_data_time_string(date_time_str); \
  fprintf(stdout, "[HYPER CTRL PRINT]: %s - " _fmt, date_time_str, __VA_ARGS__); } } while(0)

#define HYPER_CTRL_DEBUG_TIME(_fmt, ...) do { if(ENABLE_HYPER_CTRL_PRINTS && scatter_verbose_level >= SRSLTE_VERBOSE_DEBUG) { \
  char date_time_str[30]; helpers_get_data_time_string(date_time_str); \
  fprintf(stdout, "[HYPER CTRL DEBUG]: %s - " _fmt, date_time_str, __VA_ARGS__); } } while(0)

#define HYPER_CTRL_INFO_TIME(_fmt, ...) do { if(ENABLE_HYPER_CTRL_PRINTS && scatter_verbose_level >= SRSLTE_VERBOSE_INFO) { \
  char date_time_str[30]; helpers_get_data_time_string(date_time_str); \
  fprintf(stdout, "[HYPER CTRL INFO]: %s - " _fmt, date_time_str, __VA_ARGS__); } } while(0)

#define HYPER_CTRL_ERROR_TIME(_fmt, ...) do { char date_time_str[30]; helpers_get_data_time_string(date_time_str); \
  fprintf(stdout, "[HYPER CTRL ERROR]: %s - " _fmt, date_time_str, __VA_ARGS__); } while(0)

// *********** Declaration of Functions ***********
void hypervisor_ctrl_initialize(transceiver_args_t* const args);

void hypervisor_ctrl_uninitialize();

void hypervisor_ctrl_open_rf_device(char* const rf_args);

void hypervisor_ctrl_close_rf_device();

void hypervisor_ctrl_set_master_clock_rate(uint32_t nof_prb);

// Set FPGA time now to host time.
void hypervisor_ctrl_set_fpga_time();

void hypervisor_ctrl_check_and_adjust_fpga_time();

srslte_rf_t* hypervisor_ctrl_get_rf_object();

void hypervisor_ctrl_set_vphy_rx_running_flag(bool vphy_rx_running);

timer_t* hypervisor_ctrl_get_hyper_tx_timer_id();

#endif // _HYPERVISOR_CONTROL_H_
