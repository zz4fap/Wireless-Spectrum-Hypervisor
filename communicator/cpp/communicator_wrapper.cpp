#include "communicator_wrapper.h"

using namespace SafeQueue;

// ************ Definition of global variables ************
static communicator_t* communicator_handle = NULL;

// ************ Definition of functions ************
int communicator_initialization(char* module_name, char* target_name, LayerCommunicator_handle* const handle, uint32_t num_of_tx_vphys, uint32_t num_of_rx_vphys) {
  // Allocate memory for a communicator object;
  communicator_handle = (communicator_t*)communicator_vec_malloc(sizeof(communicator_t));
  if(communicator_handle == NULL) {
    std::cerr << "[COMM ERROR] Error when allocating memory for communicator." << std::endl;
    return -1;
  }
  // Set circular buffer counter to zero.
  communicator_handle->user_data_buffer_cnt = 0;
  // Set the number of spwawned Tx vPHY threads.
  communicator_handle->num_of_tx_vphys = num_of_tx_vphys;
  // Set the number of spwawned Rx vPHY threads.
  communicator_handle->num_of_rx_vphys = num_of_rx_vphys;
  // Make sure user data buffer is all set to NULL.
  communicator_set_user_data_buffer_to_null();
  // Check if needs to free the buffer.
  communicator_free_user_data_buffer();
  // Allocate memory to store user data sent by upper layers.
  communicator_allocate_user_data_buffer();
  // Instantiate communicator module so that we can receive/transmit commands and data.
  communicator_make(module_name, target_name, handle);
  return 0;
}

int communicator_uninitialization(LayerCommunicator_handle* const handle) {
  // Free user data vector.
  communicator_free_user_data_buffer();
  // After use, communicator handle MUST be freed.
  communicator_free(handle);
  // Free memory used to store communicator object.
  if(communicator_handle) {
    free(communicator_handle);
    communicator_handle = NULL;
  }
  return 0;
}

void communicator_make(char* module_name, char* target_name, LayerCommunicator_handle* const handle) {
  communicator::MODULE m = communicator::MODULE_UNKNOWN;
  communicator::MODULE t = communicator::MODULE_UNKNOWN;
  std::string m_str(module_name);
  bool success = true;
  success = success && MODULE_Parse(m_str, &m);
  std::string t_str(target_name);
  success = success && MODULE_Parse(t_str, &t);
  if(success && m != communicator::MODULE_UNKNOWN && t != communicator::MODULE_UNKNOWN) {
    *handle = new layer_communicator_t;
    (*handle)->layer_communicator_cpp = new communicator::DefaultLayerCommunicator(m, {t});
  } else {
    std::cerr << "[COMM ERROR] This should not have happened, invalid modules for communicator_make" << std::endl;
  }
}

void communicator_send_phy_stat_message(LayerCommunicator_handle handle, stat_e type, phy_stat_t* const phy_stats, message_handle* msg_handle) {
  communicator_send_phy_stat_msg_dest(handle, handle->layer_communicator_cpp->getDestinationModule(), type, phy_stats, msg_handle);
}

// PHY sends Tx/Rx statistics to other layers along with received data if available.
void communicator_send_phy_stat_msg_dest(LayerCommunicator_handle handle, int destination, stat_e type, phy_stat_t* const phy_stats, message_handle* msg_handle) {

  // Check if handle is not NULL.
  if(handle==NULL || handle->layer_communicator_cpp==NULL) {
    std::cout << "[COMM ERROR] Communicator Handle is NULL." << std::endl;
    exit(-1);
  }
  // Check if PHY stats is not NULL.
  if(phy_stats==NULL) {
    std::cout << "[COMM ERROR] PHY stats is NULL." << std::endl;
    exit(-1);
  }

  // Create an Internal Message.
  std::shared_ptr<communicator::Internal> internal = std::make_shared<communicator::Internal>();

  // Sequence number used by upper layer to track the response of PHY, i.e., correlates one phy_control message with a phy_stat message.
  internal->set_transaction_index(phy_stats->seq_number); // Transaction index is the same as sequence number.

  // Create PHY stat message.
  communicator::Phy_stat* stat = new communicator::Phy_stat();

  // Set statistics common for both Rx and Tx.
  stat->set_vphy_id(phy_stats->vphy_id);
  stat->set_host_timestamp(phy_stats->host_timestamp);
  stat->set_fpga_timestamp(phy_stats->fpga_timestamp);
	stat->set_frame(phy_stats->frame);
	stat->set_ch(phy_stats->ch);
	stat->set_slot(phy_stats->slot);
	stat->set_mcs(phy_stats->mcs);
	stat->set_num_cb_total(phy_stats->num_cb_total);
	stat->set_num_cb_err(phy_stats->num_cb_err);
  stat->set_wrong_decoding_counter(phy_stats->wrong_decoding_counter);
  // Set the corresponding statistics for Rx or Tx.
  if(type == RX_STAT) {
    // Create a Receive_r message.
    communicator::Receive_r *receive_r = new communicator::Receive_r();
    // Set the status comming from PHY so that the upper layer knows what happened.
    receive_r->set_result((communicator::TRANSACTION_RESULT)phy_stats->status);
    // Create a PHY stat Rx message
    communicator::Phy_rx_stat *stat_rx = new communicator::Phy_rx_stat();
    stat_rx->set_nof_slots_in_frame(phy_stats->stat.rx_stat.nof_slots_in_frame);
    stat_rx->set_slot_counter(phy_stats->stat.rx_stat.slot_counter);
    stat_rx->set_gain(phy_stats->stat.rx_stat.gain);
    stat_rx->set_cqi(phy_stats->stat.rx_stat.cqi);
    stat_rx->set_rssi(phy_stats->stat.rx_stat.rssi);
    stat_rx->set_rsrp(phy_stats->stat.rx_stat.rsrp);
    stat_rx->set_rsrq(phy_stats->stat.rx_stat.rsrq);
    stat_rx->set_sinr(phy_stats->stat.rx_stat.sinr);
    stat_rx->set_cfo(phy_stats->stat.rx_stat.cfo);
    stat_rx->set_detection_errors(phy_stats->stat.rx_stat.detection_errors);
    stat_rx->set_decoding_errors(phy_stats->stat.rx_stat.decoding_errors);
    stat_rx->set_peak_value(phy_stats->stat.rx_stat.peak_value);
    stat_rx->set_noise(phy_stats->stat.rx_stat.noise);
    stat_rx->set_decoded_cfi(phy_stats->stat.rx_stat.decoded_cfi);
    stat_rx->set_found_dci(phy_stats->stat.rx_stat.found_dci);
    stat_rx->set_last_noi(phy_stats->stat.rx_stat.last_noi);
    stat_rx->set_total_packets_synchronized(phy_stats->stat.rx_stat.total_packets_synchronized);
    stat_rx->set_decoding_time(phy_stats->stat.rx_stat.decoding_time);
    stat_rx->set_synch_plus_decoding_time(phy_stats->stat.rx_stat.synch_plus_decoding_time);
    stat_rx->set_length(phy_stats->stat.rx_stat.length);
    if(phy_stats->stat.rx_stat.data != NULL) {
      try {
        // Check if length is greater than 0.
        if(phy_stats->stat.rx_stat.length <= 0) {
          std::cout << "[COMM ERROR] Data length is incorrect: " << phy_stats->stat.rx_stat.length << " !" << std::endl;
          exit(-1);
        }
        // Instantiate string with data.
        std::string data_str((char*)phy_stats->stat.rx_stat.data, phy_stats->stat.rx_stat.length);
        receive_r->set_data(data_str);
      } catch(const std::length_error &e) {
        std::cout << "[COMM ERROR] Exception Caught " << e.what() << std::endl;
        std::cout << "[COMM ERROR] Exception Type " << typeid(e).name() << std::endl;
        exit(-1);
      }
    } else {
      if(phy_stats->status == PHY_SUCCESS) {
        std::cout << "[COMM WARNING] PHY status is success but received data is NULL..." << std::endl;
		  }
    }
    // Add PHY Rx Stats to PHY Stat.
    stat->set_allocated_rx_stat(stat_rx);
    // Add PHY stat to Send_r message.
    receive_r->set_allocated_stat(stat);	// receive_r has ownership over the stat pointer
    // Add Send_r to the message
    internal->set_allocated_receiver(receive_r); // internal has ownership over the receive_r pointer
  } else if(type == TX_STAT) {
    // Create a Send_r message.
    communicator::Send_r *send_r = new communicator::Send_r();
    // Set the status comming from PHY so that the upper layer knows what happened.
    send_r->set_result((communicator::TRANSACTION_RESULT)phy_stats->status);
    // Create a PHY stat Tx message
    communicator::Phy_tx_stat* stat_tx = new communicator::Phy_tx_stat();
    stat_tx->set_gain(phy_stats->stat.tx_stat.gain);
    stat_tx->set_rf_boost(phy_stats->stat.tx_stat.rf_boost);
    stat_tx->set_channel_free_cnt(phy_stats->stat.tx_stat.channel_free_cnt);
    stat_tx->set_channel_busy_cnt(phy_stats->stat.tx_stat.channel_busy_cnt);
    stat_tx->set_free_energy(phy_stats->stat.tx_stat.free_energy);
    stat_tx->set_busy_energy(phy_stats->stat.tx_stat.busy_energy);
    stat_tx->set_total_dropped_slots(phy_stats->stat.tx_stat.total_dropped_slots);
    stat_tx->set_coding_time(phy_stats->stat.tx_stat.coding_time);
    stat_tx->set_freq_boost(phy_stats->stat.tx_stat.freq_boost);
    stat->set_allocated_tx_stat(stat_tx);
    // Add PHY stat to Send_r message.
    send_r->set_allocated_phy_stat(stat);	// send_r has ownership over the stat pointer
    // Add Send_r to the message
    internal->set_allocated_sendr(send_r); // internal has ownership over the send_r pointer
  } else if(type == SENSING_STAT) {
    // Create a Receive_r message.
    communicator::Receive_r *receive_r = new communicator::Receive_r();
    // Set the status comming from PHY so that the upper layer knows what happened.
    receive_r->set_result((communicator::TRANSACTION_RESULT)phy_stats->status);
    // Create a PHY stat Rx message
    communicator::Phy_sensing_stat *stat_sensing = new communicator::Phy_sensing_stat();
    stat_sensing->set_frequency(phy_stats->stat.sensing_stat.frequency);
    stat_sensing->set_sample_rate(phy_stats->stat.sensing_stat.sample_rate);
    stat_sensing->set_gain(phy_stats->stat.sensing_stat.gain);
    stat_sensing->set_rssi(phy_stats->stat.sensing_stat.rssi);
    stat_sensing->set_length(phy_stats->stat.sensing_stat.length);
    if(phy_stats->stat.sensing_stat.data != NULL) {
      try {
        // Check if length is greater than 0.
        if(phy_stats->stat.sensing_stat.length <= 0) {
          std::cout << "[COMM ERROR] Data length is incorrect: " << phy_stats->stat.sensing_stat.length << " !" << std::endl;
          exit(-1);
        }
        // Instantiate string with data.
        std::string data_str((char*)phy_stats->stat.sensing_stat.data, phy_stats->stat.sensing_stat.length);
        receive_r->set_data(data_str);
      } catch(const std::length_error &e) {
        std::cout << "[COMM ERROR] Exception Caught " << e.what() << std::endl;
        std::cout << "[COMM ERROR] Exception Type " << typeid(e).name() << std::endl;
        exit(-1);
      }
    } else {
      if(phy_stats->status == PHY_SUCCESS) {
        std::cout << "[COMM ERROR] PHY status is success but sensing data is NULL..." << std::endl;
		  }
    }
    // Add PHY Sensing Stats to PHY Stat.
    stat->set_allocated_sensing_stat(stat_sensing);
    // Add PHY stat to Send_r message.
    receive_r->set_allocated_stat(stat);	// receive_r has ownership over the stat pointer
    // Add Send_r to the message
    internal->set_allocated_receiver(receive_r); // internal has ownership over the receive_r pointer
  } else {
    std::cout << "[COMM ERROR] Undefined type of statistics." << std::endl;
    exit(-1);
  }

  // Create a Message object what will be sent upwards.
  communicator::Message msg(communicator::MODULE_PHY, (communicator::MODULE)destination, internal);

  // Add message to the communicator queue.
  handle->layer_communicator_cpp->send(msg);

#if(CHECK_COMM_OUT_OF_SEQUENCE==1)
  static uint32_t data_cnt[2] = {0,0};
  uint32_t data_vector[2][32] = {{0, 1, 2, 3, 32, 33, 34, 35, 64, 65, 66, 67, 96, 97, 98, 99, 128, 129, 130, 131, 160, 161, 162, 163, 192, 193, 194, 195, 224, 225, 226, 227},
                                 {4, 5, 6, 7, 36, 37, 38, 39, 68, 69, 70, 71, 100, 101, 102, 103, 132, 133, 134, 135, 164, 165, 166, 167, 196, 197, 198, 199, 228, 229, 230, 231}};
  if(type == RX_STAT) {
    unsigned char* data_str = (unsigned char*)internal->receiver().data().c_str();
    if(phy_stats->vphy_id <= 1) {
      if(data_str[0] != data_vector[phy_stats->vphy_id][data_cnt[phy_stats->vphy_id]]) {
        std::cout << "[COMM] PHY Rx Stats - vPHY Id: " << (int)phy_stats->vphy_id << " - Received: " << (int)data_str[0] << " - Expected: " << (int)data_vector[phy_stats->vphy_id][data_cnt[phy_stats->vphy_id]] << "\n";
      }
      data_cnt[phy_stats->vphy_id] = (data_cnt[phy_stats->vphy_id] + 1) % 32;
    }
  }
#endif

  // Add a Message object only if different from NULL.
  if(msg_handle != NULL) {
    *msg_handle = new message_t;
    (*msg_handle)->message_cpp = msg;
  }
}

// This function will retrieve messages (phy_control) addressed to the PHY from the QUEUE.
// OBS.: It is a blocking call.
// MUST cast to phy_ctrl_t *
void communicator_get_high_queue_wait(LayerCommunicator_handle handle, void* const msg_struct, message_handle* msg_handle) {

  //WAIT UNTIL THERE IS A MESSAGE IN THE HIGH QUEUE
  communicator::Message msg = handle->layer_communicator_cpp->get_high_queue().pop_wait();
  // Parse message into the corresponding structure.
  parse_received_message(msg, msg_struct);

  // Add a Message object only if diffrent from NULL.
  if(msg_handle != NULL) {
    *msg_handle = new message_t;
    (*msg_handle)->message_cpp = msg;
  }
}

// Blocking call, it waits until someone pushes a message into the QUEUE or the waiting times out.
bool communicator_get_high_queue_wait_for(LayerCommunicator_handle handle, uint32_t timeout, void* const msg_struct, message_handle *msg_handle) {
  // Wait until there is a message in the high priority QUEUE.
  communicator::Message msg;
  bool ret = handle->layer_communicator_cpp->get_high_queue().pop_wait_for(std::chrono::milliseconds(timeout), msg);

  if(ret) {
    // Parse message into the corresponding structure.
    parse_received_message(msg, msg_struct);

    // Add a Message object only if diffrent from NULL.
    if(msg_handle != NULL) {
      *msg_handle = new message_t;
      (*msg_handle)->message_cpp = msg;
    }
  }
  return ret;
}

// Non-blocking call.
bool communicator_get_high_queue(LayerCommunicator_handle handle, void* const msg_struct, message_handle* msg_handle) {
  bool ret;
  communicator::Message msg;
  if(handle->layer_communicator_cpp->get_high_queue().pop(msg)) {
    ret = true;
    // Parse message into the corresponding structure.
    parse_received_message(msg, msg_struct);

    // Add a Message object only if diffrent from NULL.
    if(msg_handle != NULL) {
      *msg_handle = new message_t;
      (*msg_handle)->message_cpp = msg;
    }
  } else {
    msg_handle = NULL;  // Make sure it returns NULL for the caller to check before using it.
    ret = false;
    std::cout << "[COMM ERROR] No message in High Priority QUEUE." << std::endl;
  }

  return ret;
}

// Blocking call.
void communicator_get_low_queue_wait(LayerCommunicator_handle handle, void* const msg_struct, message_handle* msg_handle) {

  //WAIT UNTIL THERE IS A MESSAGE IN THE LOW QUEUE
  communicator::Message msg = handle->layer_communicator_cpp->get_low_queue().pop_wait();
  // Parse message into the corresponding structure.
  parse_received_message(msg, msg_struct);

  // Add a Message object only if diffrent from NULL.
  if(msg_handle != NULL) {
    *msg_handle = new message_t;
    (*msg_handle)->message_cpp = msg;
  }
}

// Blocking call, it waits until someone pushes a message into the QUEUE or the waiting times out.
bool communicator_get_low_queue_wait_for(LayerCommunicator_handle handle, uint32_t timeout, void* const msg_struct, message_handle* msg_handle) {
  //WAIT UNTIL THERE IS A MESSAGE IN THE LOW QUEUE
  communicator::Message msg;
  bool ret = handle->layer_communicator_cpp->get_low_queue().pop_wait_for(std::chrono::milliseconds(timeout), msg);
  if(ret) {

    // Parse message into the corresponding structure.
    parse_received_message(msg, msg_struct);

    // Add a Message object only if diffrent from NULL.
    if(msg_handle != NULL) {
      *msg_handle = new message_t;
      (*msg_handle)->message_cpp = msg;
    }
  }
  return ret;
}

// Non-blocking call.
bool communicator_get_low_queue(LayerCommunicator_handle handle, void* const msg_struct, message_handle* msg_handle) {
  bool ret;
  //CHECK IF THERE IS A MESSAGE IN THE LOW QUEUE
  communicator::Message msg;

  if(handle->layer_communicator_cpp->get_low_queue().pop(msg)) {
    ret = true;
    // Parse message into the corresponding structure.
    parse_received_message(msg, msg_struct);

    // Add a Message object only if diffrent from NULL.
    if(msg_handle != NULL) {
      *msg_handle = new message_t;
      (*msg_handle)->message_cpp = msg;
    }
  } else {
    msg_handle = NULL; // Make sure it returns NULL for the caller to check before using it.
    ret = false;
  }

  return ret;
}

void communicator_print_message(message_handle msg_handle) {
  // Print the Message.
  std::cout << "[COMM PRINT] Get message " << msg_handle->message_cpp << std::endl;
}

void communicator_free(LayerCommunicator_handle* handle) {
  communicator::DefaultLayerCommunicator *ptr = ((*handle)->layer_communicator_cpp);
  delete ptr;
  delete *handle;
  *handle = NULL;
}

void communicator_free_msg(message_handle* msg_handle) {
  delete *msg_handle;
  *msg_handle = NULL;
}

void parse_received_message(communicator::Message msg, void* const msg_struct) {

  if(msg_struct==NULL) {
    std::cout << "[COMM ERROR] Message struct is NULL." << std::endl;
    exit(-1);
  }

  // Check if message was addressed to PHY.
  if(msg.destination == communicator::MODULE::MODULE_PHY) {

    // Cast msg_struct to be a phy control struct.
    phy_ctrl_t* const phy_ctrl = (phy_ctrl_t* const)msg_struct;

    // Cast protobuf message into a Internal message one.
    std::shared_ptr<communicator::Internal> internal = std::static_pointer_cast<communicator::Internal>(msg.message);

    // Switch case used to select among different messages.
    switch(internal->payload_case()) {
      case communicator::Internal::kReceive:
      {
        // Copy from Internal message values into phy control structure.
        phy_ctrl->trx_flag = (trx_flag_e)internal->receive().phy_ctrl().trx_flag();
        switch(phy_ctrl->trx_flag) {
          case TRX_RX_ST:
          {
            phy_ctrl->nof_slots = internal->receive().phy_ctrl().slot_ctrl_size();
            // Check if the number of slots is less than or equal to the number of spawned vPHYs.
            if(phy_ctrl->nof_slots > communicator_handle->num_of_rx_vphys) {
              std::cout << "[COMM ERROR] Number of slots: " << phy_ctrl->nof_slots << " must be less than or equal to the number of spwawned Rx vPHYs: " << communicator_handle->num_of_rx_vphys << " !!!" << std::endl;
              exit(-1);
            }
            // Set slot_ctrl structure.
            for(uint32_t i = 0; i < phy_ctrl->nof_slots; i++) {
              phy_ctrl->slot_ctrl[i].slot_info.largest_nof_tbs_in_slot = 0;
              phy_ctrl->slot_ctrl[i].seq_number = internal->transaction_index();
              phy_ctrl->slot_ctrl[i].vphy_id = internal->receive().phy_ctrl().slot_ctrl(i).vphy_id();
              phy_ctrl->slot_ctrl[i].timestamp = internal->receive().phy_ctrl().slot_ctrl(i).timestamp();
              phy_ctrl->slot_ctrl[i].send_to = internal->receive().phy_ctrl().slot_ctrl(i).send_to();
              phy_ctrl->slot_ctrl[i].intf_id = internal->receive().phy_ctrl().slot_ctrl(i).intf_id();
              phy_ctrl->slot_ctrl[i].bw_idx = internal->receive().phy_ctrl().slot_ctrl(i).bw_index();
              phy_ctrl->slot_ctrl[i].ch = internal->receive().phy_ctrl().slot_ctrl(i).ch();
              phy_ctrl->slot_ctrl[i].frame = internal->receive().phy_ctrl().slot_ctrl(i).frame();
              phy_ctrl->slot_ctrl[i].slot = internal->receive().phy_ctrl().slot_ctrl(i).slot();
              phy_ctrl->slot_ctrl[i].mcs = internal->receive().phy_ctrl().slot_ctrl(i).mcs();
              phy_ctrl->slot_ctrl[i].freq_boost = internal->receive().phy_ctrl().slot_ctrl(i).freq_boost();
              phy_ctrl->slot_ctrl[i].length = internal->receive().phy_ctrl().slot_ctrl(i).length();
            }
            break;
          }
          case TRX_RADIO_RX_ST:
          {
            phy_ctrl->hypervisor_ctrl.radio_id = internal->receive().phy_ctrl().hypervisor_ctrl().radio_id();
            phy_ctrl->hypervisor_ctrl.radio_center_frequency = internal->receive().phy_ctrl().hypervisor_ctrl().radio_center_frequency();
            phy_ctrl->hypervisor_ctrl.gain = internal->receive().phy_ctrl().hypervisor_ctrl().gain();
            phy_ctrl->hypervisor_ctrl.radio_nof_prb = internal->receive().phy_ctrl().hypervisor_ctrl().radio_nof_prb();
            phy_ctrl->hypervisor_ctrl.vphy_nof_prb = internal->receive().phy_ctrl().hypervisor_ctrl().vphy_nof_prb();
            phy_ctrl->hypervisor_ctrl.rf_boost = internal->receive().phy_ctrl().hypervisor_ctrl().rf_boost();
            break;
          }
          default:
            std::cout << "[COMM ERROR] TRX Flag different from Rx!!" << std::endl;
            exit(-1);
        }
        break;
      }
      case communicator::Internal::kSend:
      {
        // Copy from Internal message values into phy control structure.
        phy_ctrl->trx_flag = (trx_flag_e)internal->send().phy_ctrl().trx_flag();
        switch(phy_ctrl->trx_flag) {
          case TRX_TX_ST:
          {
            phy_ctrl->nof_slots = internal->send().phy_ctrl().slot_ctrl_size();
            // Check if there if slots are less than or equal to number of spawned vPHYs.
            if(phy_ctrl->nof_slots > communicator_handle->num_of_tx_vphys) {
              std::cout << "[COMM ERROR] Number of slots: " << phy_ctrl->nof_slots << " must be less than or equal to the number of spwawned Tx vPHYs: " << communicator_handle->num_of_tx_vphys << " !!!" << std::endl;
              exit(-1);
            }
            // Set slot_ctrl structure.
            uint32_t largest_nof_tbs_in_slot = 0;
            for(uint32_t i = 0; i < phy_ctrl->nof_slots; i++) {
              phy_ctrl->slot_ctrl[i].seq_number = internal->transaction_index();
              phy_ctrl->slot_ctrl[i].vphy_id = internal->send().phy_ctrl().slot_ctrl(i).vphy_id();
              phy_ctrl->slot_ctrl[i].timestamp = internal->send().phy_ctrl().slot_ctrl(i).timestamp();
              phy_ctrl->slot_ctrl[i].send_to = internal->send().phy_ctrl().slot_ctrl(i).send_to();
              phy_ctrl->slot_ctrl[i].intf_id = internal->send().phy_ctrl().slot_ctrl(i).intf_id();
              phy_ctrl->slot_ctrl[i].bw_idx = internal->send().phy_ctrl().slot_ctrl(i).bw_index();
              phy_ctrl->slot_ctrl[i].ch = internal->send().phy_ctrl().slot_ctrl(i).ch();
              phy_ctrl->slot_ctrl[i].frame = internal->send().phy_ctrl().slot_ctrl(i).frame();
              phy_ctrl->slot_ctrl[i].slot = internal->send().phy_ctrl().slot_ctrl(i).slot();
              phy_ctrl->slot_ctrl[i].mcs = internal->send().phy_ctrl().slot_ctrl(i).mcs();
              phy_ctrl->slot_ctrl[i].freq_boost = internal->send().phy_ctrl().slot_ctrl(i).freq_boost();
              phy_ctrl->slot_ctrl[i].length = internal->send().phy_ctrl().slot_ctrl(i).length();
              // Only do phy checking. Other checkings are done by PHY.
              if(phy_ctrl->slot_ctrl[i].length <= 0) {
                std::cout << "[COMM ERROR] Invalid vPHY slot control length field: " << phy_ctrl->slot_ctrl[i].length << std::endl;
                exit(-1);
              }
              // Only allocate memory for data if it is a phy_control for Tx processing.
              size_t data_length = internal->send().phy_ctrl().slot_ctrl(i).data().length();
              // The data length field should have the same value as the phy control length.
              if(data_length != phy_ctrl->slot_ctrl[i].length) {
                std::cout << "[COMM ERROR] vPHY slot control length: " << phy_ctrl->slot_ctrl[i].length << " does not match data length: " << data_length << std::endl;
                exit(-1);
              }
              // Copy received user data into user data buffer.
              memcpy(communicator_handle->user_data_buffer[communicator_handle->user_data_buffer_cnt], (unsigned char*)internal->send().phy_ctrl().slot_ctrl(i).data().c_str(), data_length);
              phy_ctrl->slot_ctrl[i].data = communicator_handle->user_data_buffer[communicator_handle->user_data_buffer_cnt];
              // Increment the counter used in the circular user data buffer.
              communicator_handle->user_data_buffer_cnt = (communicator_handle->user_data_buffer_cnt + 1)%NUMBER_OF_USER_DATA_BUFFERS;
              // Calculate the number of TB in this slot control message.
              phy_ctrl->slot_ctrl[i].nof_tb_in_slot = communicator_calculate_nof_subframes(phy_ctrl->slot_ctrl[i].mcs, communicator_get_bw_index(phy_ctrl->slot_ctrl[i].bw_idx), phy_ctrl->slot_ctrl[i].length);
              // Check if data size sent by upper layers is not bigger than the expected.
              if(phy_ctrl->slot_ctrl[i].nof_tb_in_slot == 0) {
                std::cout << "[COMM ERROR] vPHY number of TBs in one slot is equal to zero!" << std::endl;
                std::cout << "[COMM ERROR] Slot contol length:" << phy_ctrl->slot_ctrl[i].length << std::endl;
                exit(-1);
              }
              // Find the largest number of TBs in the slots sent by upper layers.
              if(phy_ctrl->slot_ctrl[i].nof_tb_in_slot > largest_nof_tbs_in_slot) {
                largest_nof_tbs_in_slot = phy_ctrl->slot_ctrl[i].nof_tb_in_slot;
              }
            }
            // Set the largest number of slots for all the slot messages.
            for(uint32_t i = 0; i < phy_ctrl->nof_slots; i++) {
              phy_ctrl->slot_ctrl[i].slot_info.largest_nof_tbs_in_slot = largest_nof_tbs_in_slot;
            }
            // Calculate number of active vPHYs in a 1 ms-long slot.
            communicator_calculate_nof_active_vphys_per_slot(phy_ctrl);
            break;
          }
          case TRX_RADIO_TX_ST:
          {
            phy_ctrl->hypervisor_ctrl.radio_id = internal->send().phy_ctrl().hypervisor_ctrl().radio_id();
            phy_ctrl->hypervisor_ctrl.radio_center_frequency = internal->send().phy_ctrl().hypervisor_ctrl().radio_center_frequency();
            phy_ctrl->hypervisor_ctrl.gain = internal->send().phy_ctrl().hypervisor_ctrl().gain();
            phy_ctrl->hypervisor_ctrl.radio_nof_prb = internal->send().phy_ctrl().hypervisor_ctrl().radio_nof_prb();
            phy_ctrl->hypervisor_ctrl.vphy_nof_prb = internal->send().phy_ctrl().hypervisor_ctrl().vphy_nof_prb();
            phy_ctrl->hypervisor_ctrl.rf_boost = internal->send().phy_ctrl().hypervisor_ctrl().rf_boost();
            break;
          }
          default:
            std::cout << "[COMM ERROR] TRX flag different from Tx!!" << std::endl;
            exit(-1);
        }
        break;
      }
      case communicator::Internal::kSet:
      case communicator::Internal::kSetr:
      case communicator::Internal::kReceiver:
      case communicator::Internal::kStats:
      case communicator::Internal::kGet:
      case communicator::Internal::kGetr:
      case communicator::Internal::kSendr:
      default:
        std::cout << "[COMM ERROR] This message type is not handled by PHY!!" << std::endl;
        break;
    }
  } else if(msg.destination == communicator::MODULE::MODULE_MAC) {
    // Cast msg_struct to be a phy stat struct.
    phy_stat_t *phy_rx_stat = (phy_stat_t *)msg_struct;
    // Cast protobuf message into a Internal message one.
    std::shared_ptr<communicator::Internal> internal = std::static_pointer_cast<communicator::Internal>(msg.message);
    // Parse accordind to the specific flag.
    switch(internal->payload_case()) {
      case communicator::Internal::kReceiver:
      {
        phy_rx_stat->seq_number = internal->transaction_index();
        phy_rx_stat->status = internal->receiver().result();
        phy_rx_stat->vphy_id = internal->receiver().stat().vphy_id();
        phy_rx_stat->host_timestamp = internal->receiver().stat().host_timestamp();
        phy_rx_stat->fpga_timestamp = internal->receiver().stat().fpga_timestamp();
        phy_rx_stat->frame = internal->receiver().stat().frame();
        phy_rx_stat->slot = internal->receiver().stat().slot();
        phy_rx_stat->ch = internal->receiver().stat().ch();
        phy_rx_stat->mcs = internal->receiver().stat().mcs();
        phy_rx_stat->num_cb_total = internal->receiver().stat().num_cb_total();
        phy_rx_stat->num_cb_err = internal->receiver().stat().num_cb_err();
        phy_rx_stat->wrong_decoding_counter = internal->receiver().stat().wrong_decoding_counter();
        phy_rx_stat->stat.rx_stat.nof_slots_in_frame = internal->receiver().stat().rx_stat().nof_slots_in_frame();
        phy_rx_stat->stat.rx_stat.slot_counter = internal->receiver().stat().rx_stat().slot_counter();
        phy_rx_stat->stat.rx_stat.gain = internal->receiver().stat().rx_stat().gain();
        phy_rx_stat->stat.rx_stat.cqi = internal->receiver().stat().rx_stat().cqi();
        phy_rx_stat->stat.rx_stat.rssi = internal->receiver().stat().rx_stat().rssi();
        phy_rx_stat->stat.rx_stat.rsrp = internal->receiver().stat().rx_stat().rsrp();
        phy_rx_stat->stat.rx_stat.rsrq = internal->receiver().stat().rx_stat().rsrq();
        phy_rx_stat->stat.rx_stat.sinr = internal->receiver().stat().rx_stat().sinr();
        phy_rx_stat->stat.rx_stat.detection_errors = internal->receiver().stat().rx_stat().detection_errors();
        phy_rx_stat->stat.rx_stat.decoding_errors = internal->receiver().stat().rx_stat().decoding_errors();
        phy_rx_stat->stat.rx_stat.cfo = internal->receiver().stat().rx_stat().cfo();
        phy_rx_stat->stat.rx_stat.peak_value = internal->receiver().stat().rx_stat().peak_value();
        phy_rx_stat->stat.rx_stat.noise = internal->receiver().stat().rx_stat().noise();
        phy_rx_stat->stat.rx_stat.decoded_cfi = internal->receiver().stat().rx_stat().decoded_cfi();
        phy_rx_stat->stat.rx_stat.found_dci = internal->receiver().stat().rx_stat().found_dci();
        phy_rx_stat->stat.rx_stat.last_noi = internal->receiver().stat().rx_stat().last_noi();
        phy_rx_stat->stat.rx_stat.total_packets_synchronized = internal->receiver().stat().rx_stat().total_packets_synchronized();
        phy_rx_stat->stat.rx_stat.decoding_time = internal->receiver().stat().rx_stat().decoding_time();
        phy_rx_stat->stat.rx_stat.synch_plus_decoding_time = internal->receiver().stat().rx_stat().synch_plus_decoding_time();
        phy_rx_stat->stat.rx_stat.length = internal->receiver().stat().rx_stat().length();
        if(phy_rx_stat->status == PHY_SUCCESS) {
          if(phy_rx_stat->stat.rx_stat.length > 0 && phy_rx_stat->stat.rx_stat.data != NULL) {
            memcpy(phy_rx_stat->stat.rx_stat.data, internal->receiver().data().c_str(), internal->receiver().data().length());
          } else {
            std::cout << "[COMM ERROR] Data length less or equal to 0!" << std::endl;
            exit(-1);
          }
        }
        break;
      }
      default:
        std::cout << "[COMM ERROR] This message type is not handled by MAC!!" << std::endl;
        break;
    }
  } else {
    std::cout << "[COMM ERROR] Message not addressed neither to PHY nor to MAC." << std::endl;
    exit(-1);
  }
}

void communicator_calculate_nof_active_vphys_per_slot(phy_ctrl_t* const phy_ctrl) {
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

bool communicator_is_high_queue_empty(LayerCommunicator_handle handle) {
  return handle->layer_communicator_cpp->get_high_queue().empty();
}

bool communicator_is_low_queue_empty(LayerCommunicator_handle handle) {
  return handle->layer_communicator_cpp->get_low_queue().empty();
}

// Create and send PHY control message to connected modules.
void communicator_send_phy_control(LayerCommunicator_handle handle, phy_ctrl_t* const phy_ctrl) {

  std::shared_ptr<communicator::Internal> message = std::make_shared<communicator::Internal>();
  message->set_transaction_index(phy_ctrl->slot_ctrl[0].seq_number);
  message->set_owner_module(communicator::MODULE_MAC);
  communicator::Message mes = communicator::Message(communicator::MODULE_MAC, communicator::MODULE_PHY, message);

  // Instantiate new PHY Control object.
  communicator::Phy_ctrl *phy_control = new communicator::Phy_ctrl();
  // Set the TRX flag.
  phy_control->set_trx_flag((communicator::TRX)phy_ctrl->trx_flag);

  // Decide which message container should be used for sending this upper layer message.
  switch(phy_ctrl->trx_flag) {
    case TRX_RADIO_RX_ST:
    case TRX_RX_ST:
    {
      // Instatiate receive object.
      communicator::Receive* receive = new communicator::Receive();
      mes.message->set_allocated_receive(receive);
      receive->set_allocated_phy_ctrl(phy_control);
      break;
    }
    case TRX_RADIO_TX_ST:
    case TRX_TX_ST:
    {
      // Instatiate send object.
      communicator::Send* send = new communicator::Send();
      mes.message->set_allocated_send(send);
      send->set_allocated_phy_ctrl(phy_control);
      break;
    }
    default:
      std::cout << "[COMM ERROR] Invalid TRX flag." << std::endl;
      exit(-1);
  }

  // Decide wich structure to fill in, either slot control or hypervisor control.
  switch(phy_ctrl->trx_flag) {
    case TRX_TX_ST:
    case TRX_RX_ST:
    {
      for(uint32_t i = 0; i < phy_ctrl->nof_slots; i++) {
        communicator::Slot_ctrl* slot_ctrl = phy_control->add_slot_ctrl();
        slot_ctrl->set_vphy_id(phy_ctrl->slot_ctrl[i].vphy_id);
        slot_ctrl->set_timestamp(phy_ctrl->slot_ctrl[i].timestamp);
        slot_ctrl->set_send_to(phy_ctrl->slot_ctrl[i].send_to);
        slot_ctrl->set_intf_id(phy_ctrl->slot_ctrl[i].intf_id);
        slot_ctrl->set_bw_index((communicator::BW_INDEX)phy_ctrl->slot_ctrl[i].bw_idx);
        slot_ctrl->set_ch(phy_ctrl->slot_ctrl[i].ch);
        slot_ctrl->set_mcs(phy_ctrl->slot_ctrl[i].mcs);
        slot_ctrl->set_freq_boost(phy_ctrl->slot_ctrl[i].freq_boost);
        slot_ctrl->set_length(phy_ctrl->slot_ctrl[i].length);
        if(phy_ctrl->slot_ctrl[i].length > 0 && phy_ctrl->slot_ctrl[i].data != NULL) {
          slot_ctrl->set_data(std::string((char*)phy_ctrl->slot_ctrl[i].data, phy_ctrl->slot_ctrl[i].length));
        }
      }
      break;
    }
    case TRX_RADIO_TX_ST:
    case TRX_RADIO_RX_ST:
    {
      communicator::Hypervisor_ctrl* hypervisor_ctrl = new communicator::Hypervisor_ctrl();
      phy_control->set_allocated_hypervisor_ctrl(hypervisor_ctrl);
      hypervisor_ctrl->set_radio_id(phy_ctrl->hypervisor_ctrl.radio_id);
      hypervisor_ctrl->set_radio_center_frequency(phy_ctrl->hypervisor_ctrl.radio_center_frequency);
      hypervisor_ctrl->set_radio_nof_prb(phy_ctrl->hypervisor_ctrl.radio_nof_prb);
      hypervisor_ctrl->set_vphy_nof_prb(phy_ctrl->hypervisor_ctrl.vphy_nof_prb);
      hypervisor_ctrl->set_gain(phy_ctrl->hypervisor_ctrl.gain);
      hypervisor_ctrl->set_rf_boost(phy_ctrl->hypervisor_ctrl.rf_boost);
      break;
    }
    default:
      std::cout << "[COMM ERROR] Invalid TRX flag." << std::endl;
      exit(-1);
  }
  // Send the message to connected layer through 0MQ bus.
  handle->layer_communicator_cpp->send(mes);
}

// Allocate heap memory used to store user data comming from upper layers.
void communicator_allocate_user_data_buffer() {
  for(int i = 0; i < NUMBER_OF_USER_DATA_BUFFERS; i++) {
    communicator_handle->user_data_buffer[i] = (unsigned char*)communicator_vec_malloc(USER_DATA_BUFFER_LEN);
    if(communicator_handle->user_data_buffer[i] == NULL) {
      std::cout << "[COMM ERROR] User data buffer malloc failed." << std::endl;
      exit(-1);
    }
    // Initialize memory.
    bzero(communicator_handle->user_data_buffer[i], USER_DATA_BUFFER_LEN);
  }
}

// Deallocate heap memory used to store user data comming from upper layers.
void communicator_free_user_data_buffer() {
  for(int i = 0; i < NUMBER_OF_USER_DATA_BUFFERS; i++) {
    if(communicator_handle->user_data_buffer[i] != NULL) {
      free(communicator_handle->user_data_buffer[i]);
      communicator_handle->user_data_buffer[i] = NULL;
    }
  }
}

// make sure the user data buffer is initialized with NULL.
void communicator_set_user_data_buffer_to_null() {
  for(int i = 0; i < NUMBER_OF_USER_DATA_BUFFERS; i++) {
    communicator_handle->user_data_buffer[i] = NULL;
  }
}

// Note: We align memory to 32 bytes (for AVX compatibility)
// because in some cases volk can incorrectly detect the architecture.
// This could be inefficient for SSE or non-SIMD platforms but shouldn't
// be a huge problem.
void *communicator_vec_malloc(uint32_t size) {
  void *ptr;
  if(posix_memalign(&ptr, 32, size)) {
    return NULL;
  } else {
    return ptr;
  }
}

uint32_t communicator_calculate_nof_subframes(uint32_t mcs, uint32_t bw_idx, uint32_t length) {
  uint32_t nof_subframes = 0;
  // Verify is it is 1.4 MHz and MCS 28.
  if((bw_idx+1) == BW_IDX_OneDotFour && mcs >= 28) {
    length = length - communicator_get_tb_size(bw_idx, mcs-1);
    nof_subframes = 1;
    if(length > 0) {
      nof_subframes = nof_subframes + (length/communicator_get_tb_size(bw_idx, mcs));
    }
  } else {
    // Calculate number of slots to be transmitted.
    nof_subframes = (length/communicator_get_tb_size(bw_idx, mcs));
  }
  return nof_subframes;
}

// ********************* Declaration of constants *********************
// MCS TB size mapping table
// how many bytes (TB size) in one slot under MCS 0~28 (this slot is our MF-TDMA slot, not LTE slot, here it refers to 1ms length!)
static const unsigned int num_of_bytes_per_slot_versus_mcs[6][29] = {{19,26,32,41,51,63,75,89,101,117,117,129,149,169,193,217,225,225,241,269,293,325,349,373,405,437,453,469,549}, // Values for 1.4 MHz BW
{49,65,81,109,133,165,193,225,261,293,293,333,373,421,485,533,573,573,621,669,749,807,871,935,999,1063,1143,1191,1383}, // Values for 3 MHz BW
{85,113,137,177,225,277,325,389,437,501,501,549,621,717,807,903,967,967,999,1143,1239,1335,1431,1572,1692,1764,1908,1980,2292}, // Values for 5 MHz BW
{173,225,277,357,453,549,645,775,871,999,999,1095,1239,1431,1620,1764,1908,1908,2052,2292,2481,2673,2865,3182,3422,3542,3822,3963,4587}, // Values for 10 MHz BW
{261,341,421,549,669,839,967,1143,1335,1479,1479,1620,1908,2124,2385,2673,2865,2865,3062,3422,3662,4107,4395,4736,5072,5477,5669,5861,6882}, // Values for 15 MHz BW
{349,453,573,717,903,1095,1287,1527,1764,1980,1980,2196,2481,2865,3182,3542,3822,3822,4107,4587,4904,5477,5861,6378,6882,7167,7708,7972,9422}}; // Values for 20 MHz BW

// This function is used to get the Transport Block size in bytes.
uint32_t communicator_get_tb_size(uint32_t bw_idx, uint32_t mcs) {
  return num_of_bytes_per_slot_versus_mcs[bw_idx][mcs];
}

uint32_t communicator_get_bw_index(uint32_t index) {
  uint32_t bw_idx;
  switch(index) {
    case BW_IDX_OneDotFour:
      bw_idx = 0;
      break;
    case BW_IDX_Three:
      bw_idx = 1;
      break;
    case BW_IDX_Five:
      bw_idx = 2;
      break;
    case BW_IDX_Ten:
      bw_idx = 3;
      break;
    case BW_IDX_Fifteen:
      bw_idx = 4;
      break;
    case BW_IDX_Twenty:
      bw_idx = 5;
      break;
    case BW_IDX_UNKNOWN:
    default:
      bw_idx = 100;
  }
  return bw_idx;
}
