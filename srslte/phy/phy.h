#ifndef _PHY_H_
#define _PHY_H_

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

#include "helpers.h"
#include "transceiver.h"
#include "vphy_rx.h"
#include "vphy_tx.h"
#include "hypervisor_control.h"
#include "phy_comm_control.h"

// ****************** Definition of flags ***********************
// This macro is used to enable/disable the SW-based RF Monitor.
#define ENABLE_SW_RF_MONITOR 0

// Set to 1 the macro below to enable the sensing thread to be initialized.
#define ENABLE_SENSING_THREAD 0

#define ENABLE_PHY_PRINTS 1

#if(ENABLE_SW_RF_MONITOR==1)
#include "../rf_monitor/rf_monitor.h"
#endif

// ****************** Definition of debugging macros ***********************
#define PHY_PRINT(_fmt, ...) do { if(ENABLE_PHY_PRINTS && scatter_verbose_level >= 0) { \
  fprintf(stdout, "[PHY PRINT]: " _fmt, __VA_ARGS__); } } while(0)

#define PHY_DEBUG(_fmt, ...) do { if(ENABLE_PHY_PRINTS && scatter_verbose_level >= SRSLTE_VERBOSE_DEBUG) { \
  fprintf(stdout, "[PHY DEBUG]: " _fmt, __VA_ARGS__); } } while(0)

#define PHY_INFO(_fmt, ...) do { if(ENABLE_PHY_PRINTS && scatter_verbose_level >= SRSLTE_VERBOSE_INFO) { \
  fprintf(stdout, "[PHY INFO]: " _fmt, __VA_ARGS__); } } while(0)

#define PHY_ERROR(_fmt, ...) do { fprintf(stdout, "[PHY ERROR]: " _fmt, __VA_ARGS__); } while(0)

#define PHY_PRINT_TIME(_fmt, ...) do { if(ENABLE_PHY_PRINTS && scatter_verbose_level >= 0) { \
  char date_time_str[30]; helpers_get_data_time_string(date_time_str); \
  fprintf(stdout, "[PHY PRINT]: %s - " _fmt, date_time_str, __VA_ARGS__); } } while(0)

#define PHY_DEBUG_TIME(_fmt, ...) do { if(ENABLE_PHY_PRINTS && scatter_verbose_level >= SRSLTE_VERBOSE_DEBUG) { \
  char date_time_str[30]; helpers_get_data_time_string(date_time_str); \
  fprintf(stdout, "[PHY DEBUG]: %s - " _fmt, date_time_str, __VA_ARGS__); } } while(0)

#define PHY_INFO_TIME(_fmt, ...) do { if(ENABLE_PHY_PRINTS && scatter_verbose_level >= SRSLTE_VERBOSE_INFO) { \
  char date_time_str[30]; helpers_get_data_time_string(date_time_str); \
  fprintf(stdout, "[PHY INFO]: %s - " _fmt, date_time_str, __VA_ARGS__); } } while(0)

#define PHY_ERROR_TIME(_fmt, ...) do { char date_time_str[30]; helpers_get_data_time_string(date_time_str); \
  fprintf(stdout, "[PHY ERROR]: %s - " _fmt, date_time_str, __VA_ARGS__); } while(0)

// ********************* Declaration of types *********************
typedef struct {
  transceiver_args_t prog_args;
  timer_t *vphy_rx_timer_ids[MAX_NUM_CONCURRENT_VPHYS];
  timer_t *vphy_tx_timer_ids[MAX_NUM_CONCURRENT_VPHYS];
  timer_t *hyper_tx_timer_id;
  // Flag used to leave the PHY executable.
  volatile sig_atomic_t run_phy;
} phy_t;

// ********************* Declaration of functions *********************
void args_default(transceiver_args_t *args);

void usage(transceiver_args_t *args, char *prog);

void parse_args(transceiver_args_t *args, int argc, char **argv);

void change_process_priority(int inc);

void initialize_rf_monitor(uint32_t rf_monitor_option, transceiver_args_t *prog_args);

void uninitialize_rf_monitor(uint32_t rf_monitor_option);

#if(ENABLE_WATCHDOG_TIMER==1)
static void phy_watchdog_handler(int sig, siginfo_t *info, void *ptr);
#endif

void phy_sig_int_handler(int signo);

void phy_initialize_signal_handler();

bool phy_is_running();

#endif // _PHY_H_
