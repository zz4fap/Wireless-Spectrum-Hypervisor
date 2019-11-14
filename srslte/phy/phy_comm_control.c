#include "phy_comm_control.h"

// ************** Global variables **************
static phy_comm_ctrl_t* phy_comm_ctrl_handle = NULL;

// ************** Definition of functions **************
// Initialize PHY communicator control module.
int phy_comm_ctrl_initialize(transceiver_args_t *prog_args) {
  char module_name[20];
  char target_name[20];
  // Allocate memory for a new PHY Comm. control object.
  phy_comm_ctrl_handle = (phy_comm_ctrl_t*)srslte_vec_malloc(sizeof(phy_comm_ctrl_t));
  // Check if memory allocation was correctly done.
  if(phy_comm_ctrl_handle == NULL) {
    PHY_COMM_CTRL_ERROR("Error allocating memory for PHY Communication control object.\n",0);
    return -1;
  }
  // Initialize structure with zeros.
  bzero(phy_comm_ctrl_handle, sizeof(phy_comm_ctrl_t));
  // Communicator handle used to exchange messages with other layers via 0MQ bus.
  phy_comm_ctrl_handle->phy_comm_handle = NULL;
  // Set the number of spwawned Tx vPHY threads.
  phy_comm_ctrl_handle->num_of_tx_vphys = prog_args->num_of_tx_vphys;
  // Set the number of spwawned Rx vPHY threads.
  phy_comm_ctrl_handle->num_of_rx_vphys = prog_args->num_of_rx_vphys;
#if(ENABLE_PACKET_DROPPING==1)
  // Set dropping of packets initially to false.
  bzero(phy_comm_ctrl_handle->dropping_activated, sizeof(bool)*MAX_NUM_CONCURRENT_VPHYS);
#endif
  // Rertieve target name.
  phy_comm_ctrl_get_module_and_target_name(module_name, target_name, prog_args);
  // Instantiate communicator module so that we can receive/transmit commands and data
  communicator_initialization(module_name, target_name, &(phy_comm_ctrl_handle->phy_comm_handle), phy_comm_ctrl_handle->num_of_tx_vphys, phy_comm_ctrl_handle->num_of_rx_vphys);
  // Initialize mutex for communicator access.
  if(pthread_mutex_init(&(phy_comm_ctrl_handle->phy_comm_ctrl_handle_mutex), NULL) != 0) {
    PHY_COMM_CTRL_ERROR("Mutex for communicator access init failed.\n",0);
    return -1;
  }
  return 0;
}

// Uninitialize PHY communicator control module.
int phy_comm_ctrl_uninitialize() {
  // After use, communicator phy_comm_handle MUST be freed.
  if(communicator_uninitialization(&(phy_comm_ctrl_handle->phy_comm_handle)) != 0) {
    PHY_COMM_CTRL_ERROR("Error unitializing communicator.\n",0);
    return -1;
  }
  PHY_COMM_CTRL_INFO("Communicator unitialization done.\n",0);
  // Destroy mutex for communicator access.
  if(pthread_mutex_destroy(&(phy_comm_ctrl_handle->phy_comm_ctrl_handle_mutex)) != 0) {
    PHY_COMM_CTRL_ERROR("Mutex destroy function failed.\n",0);
    return -1;
  }
  // Free memory used to store PHY Comm. control object.
  if(phy_comm_ctrl_handle) {
    free(phy_comm_ctrl_handle);
    phy_comm_ctrl_handle = NULL;
  }
  PHY_COMM_CTRL_INFO("PHY Communication control context deleted!\n",0);
  return 0;
}

// Get the module names.
void phy_comm_ctrl_get_module_and_target_name(char *module_name, char *target_name, transceiver_args_t *prog_args) {
  // If we are testing the PHY with a single host PC we need to have two distinct names for PHY, otherwise, the test scripts will receive
  // responses from the other PHY.
  if(prog_args->node_operation == -1) {
    strcpy(module_name, "MODULE_PHY");
    strcpy(target_name, "MODULE_MAC");
    PHY_COMM_CTRL_DEBUG("module_name: %s - target_name: %s\n", module_name, target_name);
  } else if(prog_args->node_operation == PHY_DEBUG_1) {
    strcpy(module_name, "MODULE_PHY_DEBUG_1");
    strcpy(target_name, "MODULE_MAC_DEBUG_1");
    PHY_COMM_CTRL_DEBUG("module_name: %s - target_name: %s\n", module_name, target_name);
  } else if(prog_args->node_operation == PHY_DEBUG_2) {
    strcpy(module_name, "MODULE_PHY_DEBUG_2");
    strcpy(target_name, "MODULE_MAC_DEBUG_2");
    PHY_COMM_CTRL_DEBUG("module_name: %s - target_name: %s\n", module_name, target_name);
  } else {
    PHY_COMM_CTRL_DEBUG("Node operation was not defined correctly.\n",0);
    exit(-1);
  }
}

// Function called by handle messages received from upper layers.
void phy_comm_ctrl_control_work() {
  uint32_t vphy_id = 0;
  phy_ctrl_t phy_ctrl;

  // Loop over messages received from upper layers via 0MQ bus.
  while(phy_is_running()) {
    // Try to retrieve a message from the QUEUE. It waits for a specified amount of time before timing out.
    bool ret = communicator_get_low_queue_wait_for(phy_comm_ctrl_handle->phy_comm_handle, 1000, (void * const)&phy_ctrl, NULL);
    // If message is properly retrieved and parsed, then relay it to the correct module.
    if(phy_is_running() && ret) {
      // Perform task according to TRX flag.
      switch(phy_ctrl.trx_flag) {
        case TRX_TX_ST:
        {
          // Push vPHY Tx slot control message into the container.
          for(uint32_t slot_cnt = 0; slot_cnt < phy_ctrl.nof_slots; slot_cnt++) {
            vphy_id = phy_ctrl.slot_ctrl[slot_cnt].vphy_id;
// Packet dropping is used to control the load on the Tx buffer of each vPHY Tx thread.
#if(ENABLE_PACKET_DROPPING==1)
            int size = vphy_tx_get_tx_slot_control_container_size(vphy_id);
            if(!phy_comm_ctrl_handle->dropping_activated[vphy_id]) {
              if(size >= (LIMIT_TO_DROP_PACKETS/phy_comm_ctrl_handle->num_of_tx_vphys)) {
                PHY_COMM_CTRL_PRINT("vPHY Tx ID: %d - Packet dropping enabled as threshold (%d) was reached.\n", vphy_id, (LIMIT_TO_DROP_PACKETS/phy_comm_ctrl_handle->num_of_tx_vphys));
                phy_comm_ctrl_handle->dropping_activated[vphy_id] = true;
              }
            } else {
              if(size <= (NUMBER_OF_USER_DATA_BUFFERS/(phy_comm_ctrl_handle->num_of_tx_vphys*2))) {
                PHY_COMM_CTRL_PRINT("vPHY Tx ID: %d - Packet dropping disabled as container size is %d.\n", vphy_id, size);
                phy_comm_ctrl_handle->dropping_activated[vphy_id] = false;
              }
            }
            // Push slot into container based on dropping packet flag.
            if(!phy_comm_ctrl_handle->dropping_activated[vphy_id]) {
              vphy_tx_push_tx_slot_control_to_container(vphy_id, &phy_ctrl.slot_ctrl[vphy_id]);
            }
#else
            vphy_tx_push_tx_slot_control_to_container(vphy_id, &phy_ctrl.slot_ctrl[vphy_id]);
#endif
          }
          break;
        }
        case TRX_RX_ST:
          // Change vPHY Rx parameters according to received vPHY Rx slot control message.
          for(uint32_t slot_cnt = 0; slot_cnt < phy_ctrl.nof_slots; slot_cnt++) {
            vphy_id = phy_ctrl.slot_ctrl[slot_cnt].vphy_id;
            vphy_rx_change_parameters(vphy_id, &phy_ctrl.slot_ctrl[vphy_id]);
          }
          break;
        case TRX_RADIO_RX_ST:
          hypervisor_rx_change_parameters(&phy_ctrl.hypervisor_ctrl);
          break;
        case TRX_RADIO_TX_ST:
          hypervisor_tx_change_parameters(&phy_ctrl.hypervisor_ctrl);
          break;
        default:
          PHY_COMM_CTRL_ERROR("Invalid TRX option...\n",0);
      }
      // Print some PHY information.
      HELPERS_PRINT_CONTROL_MSG(&phy_ctrl);
    }
  }
  PHY_COMM_CTRL_INFO("Leaving phy_comm_ctrl_control_work()\n",0);
}

void phy_comm_ctrl_send_tx_statistics(phy_stat_t* const phy_tx_stat) {
  // Lock a mutex prior to accessing the communicator handle.
  pthread_mutex_lock(&(phy_comm_ctrl_handle->phy_comm_ctrl_handle_mutex));

  // TODO: update with current values.
  phy_tx_stat->stat.tx_stat.gain = 0;
  phy_tx_stat->stat.tx_stat.rf_boost = 0;

  // Sending PHY Tx statistics to upper layer.
  PHY_COMM_CTRL_DEBUG("Sending vHPY Tx statistics information upwards...\n",0);
  // Send Tx stats. There is a mutex on this function which prevents Rx from sending statistics to PHY at the same time.
  communicator_send_phy_stat_message(phy_comm_ctrl_handle->phy_comm_handle, TX_STAT, phy_tx_stat, NULL);
  // Print Tx stats information.
  HELPERS_PRINT_TX_STATS(phy_tx_stat);

  // Unlock mutex upon accessing the communicator handle.
  pthread_mutex_unlock(&(phy_comm_ctrl_handle->phy_comm_ctrl_handle_mutex));
}

void phy_comm_ctrl_send_rx_statistics(phy_stat_t* const phy_rx_stat) {
  // Lock a mutex prior to accessing the communicator handle.
  pthread_mutex_lock(&phy_comm_ctrl_handle->phy_comm_ctrl_handle_mutex);
  communicator_send_phy_stat_message(phy_comm_ctrl_handle->phy_comm_handle, RX_STAT, phy_rx_stat, NULL);
  // Unlock mutex upon accessing the communicator handle.
  pthread_mutex_unlock(&phy_comm_ctrl_handle->phy_comm_ctrl_handle_mutex);
}
