#ifndef _COMMUNICATOR_WRAPPER_H
#define _COMMUNICATOR_WRAPPER_H

// *********** Definition of COMM macros ***********
// As we can have overlapping of Tx data we create a vector with a given number of buffers.
#define NUMBER_OF_USER_DATA_BUFFERS 100000
// Constant used to check when packets needs to start being dropped.
#define LIMIT_TO_DROP_PACKETS (NUMBER_OF_USER_DATA_BUFFERS - NUMBER_OF_USER_DATA_BUFFERS*0.1)

#ifdef __cplusplus

#include "LayerCommunicator.h"
#include "interf.pb.h"
#include <iostream>
#include <stdexcept>
#include <mutex>
#include <map>
#include <thread>
#include <condition_variable>

#include "../../phy/srslte/include/srslte/intf/intf.h"

#define ENABLE_COMMUNICATOR_PRINT 0

#define CHECK_COMM_OUT_OF_SEQUENCE 0

#define COMMUNICATOR_DEBUG(x) do { if(ENABLE_COMMUNICATOR_PRINT) {std::cout << x;} } while (0)

struct layer_communicator_t {
  communicator::DefaultLayerCommunicator *layer_communicator_cpp;
};

struct message_t {
  communicator::Message message_cpp;
};

typedef struct {
  // This is a pointer to the user data buffer.
  unsigned char *user_data_buffer[NUMBER_OF_USER_DATA_BUFFERS];
  // This counter is used to change the current buffer row so that we don't have overlapping even when Tx is fast enough.
  uint32_t user_data_buffer_cnt;
  // Number of available Tx vPHYs, i.e, number of threads that were created.
  uint32_t num_of_tx_vphys;
  // Number of available Rx vPHYs, i.e, number of threads that were created.
  uint32_t num_of_rx_vphys;
} communicator_t;

void parse_received_message(communicator::Message msg, void *msg_struct);

extern "C" {
#else
struct layer_communicator_t;
struct message_t;
#endif

typedef struct layer_communicator_t* LayerCommunicator_handle;

typedef struct message_t* message_handle;

int communicator_initialization(char* module_name, char* target_name, LayerCommunicator_handle* const handle, uint32_t num_of_tx_vphys, uint32_t num_of_rx_vphys);

int communicator_uninitialization(LayerCommunicator_handle* const handle);

void communicator_make(char* module_name, char* target_name, LayerCommunicator_handle* const handle);

void communicator_send_phy_stat_message(LayerCommunicator_handle handle, stat_e type, phy_stat_t* const phy_stats, message_handle* msg_handle);

void communicator_send_phy_stat_msg_dest(LayerCommunicator_handle handle, int destination, stat_e type, phy_stat_t* const phy_stats, message_handle* msg_handle);

bool communicator_get_high_queue(LayerCommunicator_handle handle, void* const msg_struct, message_handle* msg_handle);

void communicator_get_high_queue_wait(LayerCommunicator_handle handle, void* const msg_struct, message_handle* msg_handle);

bool communicator_get_high_queue_wait_for(LayerCommunicator_handle handle, uint32_t timeout, void* const msg_struct, message_handle *msg_handle);

bool communicator_get_low_queue(LayerCommunicator_handle handle, void* const msg_struct, message_handle* msg_handle);

void communicator_get_low_queue_wait(LayerCommunicator_handle handle, void* const msg_struct, message_handle* msg_handle);

bool communicator_get_low_queue_wait_for(LayerCommunicator_handle handle, uint32_t timeout, void* const msg_struct, message_handle* msg_handle);

bool communicator_is_high_queue_empty(LayerCommunicator_handle handle);

bool communicator_is_low_queue_empty(LayerCommunicator_handle handle);

void communicator_print_message(message_handle msg_handle);

void communicator_free(LayerCommunicator_handle* const handle);

void communicator_free_msg(message_handle* msg_handle);

void communicator_send_basic_control(LayerCommunicator_handle handle, basic_ctrl_t* const phy_ctrl);

uint32_t communicator_get_tb_size(uint32_t bw_idx, uint32_t mcs);

void communicator_allocate_user_data_buffer();

void communicator_free_user_data_buffer();

void communicator_set_user_data_buffer_to_null();

void *communicator_vec_malloc(uint32_t size);

void communicator_send_phy_control(LayerCommunicator_handle handle, phy_ctrl_t* const phy_ctrl);

uint32_t communicator_get_bw_index(uint32_t index);

void communicator_calculate_nof_active_vphys_per_slot(phy_ctrl_t* const phy_ctrl);

uint32_t communicator_calculate_nof_subframes(uint32_t mcs, uint32_t bw_idx, uint32_t length);

#ifdef __cplusplus
}
#endif

#endif /* _COMMUNICATOR_WRAPPER_H */
