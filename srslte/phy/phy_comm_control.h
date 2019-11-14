#ifndef _PHY_COMM_CONTROL_H_
#define _PHY_COMM_CONTROL_H_

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

#include "phy.h"
#include "helpers.h"
#include "transceiver.h"
#include "vphy_rx.h"
#include "vphy_tx.h"
#include "hypervisor_control.h"

// *********** Defintion of flags ***********
#define ENABLE_PHY_COMM_CTRL_PRINTS 1

#define PHY_DEBUG_1 1
#define PHY_DEBUG_2 2

// *********** Defintion of debugging macros ***********
#define PHY_COMM_CTRL_PRINT(_fmt, ...) do { if(ENABLE_PHY_COMM_CTRL_PRINTS && scatter_verbose_level >= 0) { \
  fprintf(stdout, "[PHY COMM CTRL PRINT]: " _fmt, __VA_ARGS__); } } while(0)

#define PHY_COMM_CTRL_DEBUG(_fmt, ...) do { if(ENABLE_PHY_COMM_CTRL_PRINTS && scatter_verbose_level >= SRSLTE_VERBOSE_DEBUG) { \
  fprintf(stdout, "[PHY COMM CTRL DEBUG]: " _fmt, __VA_ARGS__); } } while(0)

#define PHY_COMM_CTRL_INFO(_fmt, ...) do { if(ENABLE_PHY_COMM_CTRL_PRINTS && scatter_verbose_level >= SRSLTE_VERBOSE_INFO) { \
  fprintf(stdout, "[PHY COMM CTRL INFO]: " _fmt, __VA_ARGS__); } } while(0)

#define PHY_COMM_CTRL_ERROR(_fmt, ...) do { fprintf(stdout, "[PHY COMM CTRL ERROR]: " _fmt, __VA_ARGS__); } while(0)

#define PHY_COMM_CTRL_PRINT_TIME(_fmt, ...) do { if(ENABLE_PHY_COMM_CTRL_PRINTS && scatter_verbose_level >= 0) { \
  char date_time_str[30]; helpers_get_data_time_string(date_time_str); \
  fprintf(stdout, "[PHY COMM CTRL PRINT]: %s - " _fmt, date_time_str, __VA_ARGS__); } } while(0)

#define PHY_COMM_CTRL_DEBUG_TIME(_fmt, ...) do { if(ENABLE_PHY_COMM_CTRL_PRINTS && scatter_verbose_level >= SRSLTE_VERBOSE_DEBUG) { \
  char date_time_str[30]; helpers_get_data_time_string(date_time_str); \
  fprintf(stdout, "[PHY COMM CTRL DEBUG]: %s - " _fmt, date_time_str, __VA_ARGS__); } } while(0)

#define PHY_COMM_CTRL_INFO_TIME(_fmt, ...) do { if(ENABLE_PHY_COMM_CTRL_PRINTS && scatter_verbose_level >= SRSLTE_VERBOSE_INFO) { \
  char date_time_str[30]; helpers_get_data_time_string(date_time_str); \
  fprintf(stdout, "[PHY COMM CTRL INFO]: %s - " _fmt, date_time_str, __VA_ARGS__); } } while(0)

#define PHY_COMM_CTRL_ERROR_TIME(_fmt, ...) do { char date_time_str[30]; helpers_get_data_time_string(date_time_str); \
  fprintf(stdout, "[PHY COMM CTRL ERROR]: %s - " _fmt, date_time_str, __VA_ARGS__); } while(0)

// ****************** Definition of types ******************
typedef struct {
  // Communicator handle used to exchange messages with other layers via 0MQ bus.
  LayerCommunicator_handle phy_comm_handle;
  // This mutex is used to synchronize the access to the communicator handle.
  pthread_mutex_t phy_comm_ctrl_handle_mutex;
  // Number of available Tx vPHYs, i.e, number of threads that were created.
  uint32_t num_of_tx_vphys;
  // Number of available Rx vPHYs, i.e, number of threads that were created.
  uint32_t num_of_rx_vphys;
  // Flag used to indicate if dropping of packets is enabled or not.
#if(ENABLE_PACKET_DROPPING==1)
  bool dropping_activated[MAX_NUM_CONCURRENT_VPHYS];
#endif
} phy_comm_ctrl_t;

// ******************** Declaration of functions **********************
int phy_comm_ctrl_initialize(transceiver_args_t *prog_args);

int phy_comm_ctrl_uninitialize();

void phy_comm_ctrl_get_module_and_target_name(char *module_name, char *target_name, transceiver_args_t *prog_args);

void phy_comm_ctrl_control_work();

void phy_comm_ctrl_send_tx_statistics(phy_stat_t* const phy_tx_stat);

void phy_comm_ctrl_send_rx_statistics(phy_stat_t* const phy_rx_stat);

#endif // _PHY_COMM_CONTROL_H_
