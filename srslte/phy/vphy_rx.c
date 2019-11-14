#include "vphy_rx.h"

//************************************************
// *************** Global variables **************
//************************************************
// This vector of pointers is used to keep pointers to context structures that hold information of the vPHY Rx module.
// This handle is used to pass some important objects/parameters to the vPHY Rx threads.
static vphy_rx_thread_context_t *vphy_rx_threads[MAX_NUM_CONCURRENT_VPHYS] = {NULL};

//************************************************
// *********** Definition of functions ***********
//************************************************
// This function is used to set everything needed for a vPHY Rx thread to run accordinly.
int vphy_rx_start_thread(transceiver_args_t* const args, uint32_t vphy_id) {
  // Allocate memory for a new vPHY Rx thread object. Every vPHY Rx thread will have one of this objects.
  vphy_rx_threads[vphy_id] = (vphy_rx_thread_context_t*)srslte_vec_malloc(sizeof(vphy_rx_thread_context_t));
  // Check if memory allocation was correctly done.
  if(vphy_rx_threads[vphy_id] == NULL) {
    PHY_RX_ERROR("vPHY Rx ID: %d - Error allocating memory for vPHY Rx context\n", vphy_id);
    return -1;
  }
  // Initialize structure with zeros.
  bzero(vphy_rx_threads[vphy_id], sizeof(vphy_rx_thread_context_t));
  // Set vPHY Rx context with vPHY ID number.
  vphy_rx_threads[vphy_id]->vphy_id = vphy_id;
  // Initialize vPHY Rx thread object.
  if(vphy_rx_init_thread_context(vphy_rx_threads[vphy_id], args) < 0) {
    PHY_RX_ERROR("vPHY Rx ID: %d - Error when initializing thread context.\n", vphy_id);
    return -1;
  }
  // Start vPHY Rx synchronization thread.
  if(vphy_rx_start_sync_thread(vphy_rx_threads[vphy_id]) < 0) {
    PHY_RX_ERROR("vPHY Rx ID: %d - Error when starting PHY synchronization thread.\n", vphy_id);
    return -1;
  }
  // Start vPHY Rx decoding thread.
  if(vphy_rx_start_decoding_thread(vphy_rx_threads[vphy_id]) < 0) {
    PHY_RX_ERROR("vPHY Rx ID: %d - Error when starting PHY decoding thread.\n", vphy_id);
    return -1;
  }
  return 0;
}

// Destroy the current vPHY reception thread.
// Free all the resources used by this vPHY reception thread.
int vphy_rx_stop_thread(uint32_t vphy_id) {
  // Stop vPHY Rx synchronization thread.
  if(vphy_rx_stop_sync_thread(vphy_rx_threads[vphy_id]) < 0) {
    PHY_RX_ERROR("vPHY Rx ID: %d - Error when stopping PHY synchronization thread.\n", vphy_id);
    return -1;
  }
  PHY_RX_DEBUG("vPHY Rx ID: %d - PHY synchronization thread terminated\n", vphy_id);
  // Stop vPHY Rx decoding thread.
  if(vphy_rx_stop_decoding_thread(vphy_rx_threads[vphy_id]) < 0) {
    PHY_RX_ERROR("vPHY Rx ID: %d - Error when stopping PHY decoding thread.\n", vphy_id);
    return -1;
  }
  PHY_RX_DEBUG("vPHY Rx ID: %d - PHY decoding thread terminated\n", vphy_id);
  // Destroy mutex for phy control fields access.
  pthread_mutex_destroy(&vphy_rx_threads[vphy_id]->phy_rx_slot_control_mutex);
  // Destroy mutex for ue sync structure access.
  pthread_mutex_destroy(&vphy_rx_threads[vphy_id]->vphy_rx_ue_sync_mutex);
  // Free all related UE Downlink structures.
  vphy_rx_ue_free(vphy_rx_threads[vphy_id]);
  // free plot object.
#if(ENBALE_VPHY_RX_INFO_PLOT==1)
  if(vphy_rx_threads[vphy_id]->plot_rx_info == true && vphy_id == vphy_rx_threads[vphy_id]->vphy_id_rx_info) {
    free_ue_decoding_plot_object();
  }
#endif

  // Delete vector object.
  sync_cb_free(&vphy_rx_threads[vphy_id]->synch_handle);
  // Free memory used to store vPHY Rx thread object.
  if(vphy_rx_threads[vphy_id]) {
    free(vphy_rx_threads[vphy_id]);
    vphy_rx_threads[vphy_id] = NULL;
  }
  return 0;
}

int vphy_rx_init_thread_context(vphy_rx_thread_context_t* const vphy_rx_thread_ctx, transceiver_args_t* const args) {
  // Initialize vPHY Rx context.
  vphy_rx_thread_ctx->rnti                            = args->rnti;
  vphy_rx_thread_ctx->use_std_carrier_sep             = args->use_std_carrier_sep;
  vphy_rx_thread_ctx->initial_subframe_index          = args->initial_subframe_index;
  vphy_rx_thread_ctx->radio_sampling_rate             = helpers_get_bw_from_nprb(args->radio_nof_prb);
  vphy_rx_thread_ctx->enable_cfo_correction           = args->enable_cfo_correction;
  vphy_rx_thread_ctx->plot_rx_info                    = args->plot_rx_info;
  vphy_rx_thread_ctx->vphy_id_rx_info                 = args->vphy_id_rx_info;
  vphy_rx_thread_ctx->run_phy_decoding_thread         = true; // Set variable used to stop vPHY Rx decoding thread with initial value.
  vphy_rx_thread_ctx->run_phy_synchronization_thread  = true; // Set variable used to stop vPHY Rx synchronization thread with initial value
  vphy_rx_thread_ctx->pss_peak_threshold              = args->pss_peak_threshold;
  vphy_rx_thread_ctx->sequence_number                 = 0;
  vphy_rx_thread_ctx->decode_pdcch                    = args->decode_pdcch; // If enabled, then PDCCH is decoded, otherwise, SCH is decoded.
  vphy_rx_thread_ctx->node_id                         = args->node_id; // SRN ID, a number from 0 to 255.
  vphy_rx_thread_ctx->phy_filtering                   = args->phy_filtering;
  vphy_rx_thread_ctx->max_turbo_decoder_noi           = args->max_turbo_decoder_noi;

  // Instantiate vector.
  sync_cb_make(&vphy_rx_thread_ctx->synch_handle, NUMBER_OF_SUBFRAME_BUFFERS);
  // Initialize reader object.
  vphy_rx_initialize_reader(vphy_rx_thread_ctx, args);
  // Initialize struture with Cell parameters.
  vphy_rx_init_cell_parameters(vphy_rx_thread_ctx, args);
  // Initialize structure with last configured Rx slot control.
  vphy_rx_init_last_slot_control(vphy_rx_thread_ctx, args);
  // Do all UE related intilization: SYNC, MIB and Downlink.
  if(vphy_rx_ue_init(vphy_rx_thread_ctx) < 0) {
    PHY_RX_ERROR("vPHY Rx ID: %d - Error initializing synch and decoding structures.\n", vphy_rx_thread_ctx->vphy_id);
    return -1;
  }
  // Initialize mutex for slot control fields access.
  if(pthread_mutex_init(&vphy_rx_thread_ctx->phy_rx_slot_control_mutex, NULL) != 0) {
    PHY_RX_ERROR("vPHY Rx ID: %d - Mutex for slot control access init failed.\n", vphy_rx_thread_ctx->vphy_id);
    return -1;
  }
  // Initialize mutex for ue sync structure access.
  if(pthread_mutex_init(&vphy_rx_thread_ctx->vphy_rx_ue_sync_mutex, NULL) != 0) {
    PHY_RX_ERROR("vPHY Rx ID: %d - Mutex for ue sync structure access init failed.\n", vphy_rx_thread_ctx->vphy_id);
    return -1;
  }
  // Initialize conditional variable.
  if(pthread_cond_init(&vphy_rx_thread_ctx->vphy_rx_ue_sync_cv, NULL)) {
    PHY_RX_ERROR("vPHY Rx ID: %d - Conditional variable init failed.\n", vphy_rx_thread_ctx->vphy_id);
    return -1;
  }
  // Create a watchdog timer for synchronization thread.
#if(ENABLE_WATCHDOG_TIMER==1)
  if(timer_init(&vphy_rx_thread_ctx->vphy_rx_sync_timer_id) < 0) {
    PHY_RX_ERROR("vPHY Rx ID: %d - Not possible to create a timer for vPHY Rx sync thread.\n", vphy_rx_thread_ctx->vphy_id);
    return -1;
  }
#endif
  // Enable plot for debugging purposes.
#if(ENBALE_VPHY_RX_INFO_PLOT==1)
  if(vphy_rx_thread_ctx->plot_rx_info == true && vphy_rx_thread_ctx->vphy_id == vphy_rx_thread_ctx->vphy_id_rx_info) {
    // Initialize plot.
    init_ue_decoding_plot_object();
  }
#endif
  // Everything went well.
  return 0;
}

int vphy_rx_start_decoding_thread(vphy_rx_thread_context_t* const vphy_rx_thread_ctx) {
  // Enable receiving thread.
  vphy_rx_thread_ctx->run_phy_decoding_thread = true;
  // Create threads to perform phy reception.
  pthread_attr_init(&vphy_rx_thread_ctx->vphy_rx_decoding_thread_attr);
  pthread_attr_setdetachstate(&vphy_rx_thread_ctx->vphy_rx_decoding_thread_attr, PTHREAD_CREATE_JOINABLE);
  // Create thread to sense channel.
  int rc = pthread_create(&vphy_rx_thread_ctx->vphy_rx_decoding_thread_id, &vphy_rx_thread_ctx->vphy_rx_decoding_thread_attr, phy_decoding_work, (void *)vphy_rx_thread_ctx);
  if(rc) {
    PHY_RX_ERROR("vPHY Rx ID: %d - Return code from PHY reception pthread_create() is %d\n", vphy_rx_thread_ctx->vphy_id, rc);
    return -1;
  }
  return 0;
}

int vphy_rx_stop_decoding_thread(vphy_rx_thread_context_t* const vphy_rx_thread_ctx) {
  vphy_rx_thread_ctx->run_phy_decoding_thread = false; // Stop decoding thread.
  pthread_attr_destroy(&vphy_rx_thread_ctx->vphy_rx_decoding_thread_attr);
  int rc = pthread_join(vphy_rx_thread_ctx->vphy_rx_decoding_thread_id, NULL);
  if(rc) {
    PHY_RX_ERROR("vPHY Rx ID: %d - Return code from phy reception pthread_join() is %d\n", vphy_rx_thread_ctx->vphy_id, rc);
    return -1;
  }
  return 0;
}

int vphy_rx_start_sync_thread(vphy_rx_thread_context_t* const vphy_rx_thread_ctx) {
  // Enable synchronization thread.
  vphy_rx_thread_ctx->run_phy_synchronization_thread = true;
  // Create threads to perform phy synchronization.
  pthread_attr_init(&vphy_rx_thread_ctx->vphy_rx_sync_thread_attr);
  pthread_attr_setdetachstate(&vphy_rx_thread_ctx->vphy_rx_sync_thread_attr, PTHREAD_CREATE_JOINABLE);
  // Create thread to synchronize slots.
  int rc = pthread_create(&vphy_rx_thread_ctx->vphy_rx_sync_thread_id, &vphy_rx_thread_ctx->vphy_rx_sync_thread_attr, vphy_rx_sync_frame_type_two_work, (void *)vphy_rx_thread_ctx);
  if(rc) {
    PHY_RX_ERROR("vPHY Rx ID: %d - Return code from PHY synchronization pthread_create() is %d\n", vphy_rx_thread_ctx->vphy_id, rc);
    return -1;
  }
  // Everything went well.
  return 0;
}

int vphy_rx_stop_sync_thread(vphy_rx_thread_context_t* const vphy_rx_thread_ctx) {
  // Stop synchronization thread.
  vphy_rx_thread_ctx->run_phy_synchronization_thread = false;
  // Notify condition variable.
  pthread_cond_signal(&vphy_rx_thread_ctx->vphy_rx_ue_sync_cv);
  pthread_attr_destroy(&vphy_rx_thread_ctx->vphy_rx_sync_thread_attr);
  int rc = pthread_join(vphy_rx_thread_ctx->vphy_rx_sync_thread_id, NULL);
  if(rc) {
    PHY_RX_ERROR("vPHY Rx ID: %d - Return code from phy synchronization pthread_join() is %d\n", vphy_rx_thread_ctx->vphy_id, rc);
    return -1;
  }
  // Destory conditional variable.
  if(pthread_cond_destroy(&vphy_rx_thread_ctx->vphy_rx_ue_sync_cv) != 0) {
    PHY_RX_ERROR("vPHY Rx ID: %d - Conditional variable destruction failed.\n", vphy_rx_thread_ctx->vphy_id);
    return -1;
  }
  return 0;
}

int vphy_rx_ue_init(vphy_rx_thread_context_t* const vphy_rx_thread_ctx) {
  // Initialize parameters for UE Cell.
  if(srslte_ue_sync_init_with_hyper_rx(&vphy_rx_thread_ctx->ue_sync, vphy_rx_thread_ctx->vphy_ue, vphy_rx_recv, (void*)&vphy_rx_thread_ctx->vphy_reader, vphy_rx_thread_ctx->initial_subframe_index, vphy_rx_thread_ctx->enable_cfo_correction, vphy_rx_thread_ctx->decode_pdcch, vphy_rx_thread_ctx->node_id, vphy_rx_thread_ctx->vphy_id, vphy_rx_thread_ctx->phy_filtering, vphy_rx_thread_ctx->vphy_id_rx_info, vphy_rx_thread_ctx->pss_peak_threshold)) {
    PHY_RX_ERROR("vPHY Rx ID: %d - Error initializing ue_sync\n", vphy_rx_thread_ctx->vphy_id);
    return -1;
  }
  // Initialize UE downlink structure.
  if(srslte_ue_dl_init_generic(&vphy_rx_thread_ctx->ue_dl, vphy_rx_thread_ctx->vphy_ue, vphy_rx_thread_ctx->vphy_id)) {
    PHY_RX_ERROR("vPHY Rx ID: %d - Error initializing UE downlink processing module\n", vphy_rx_thread_ctx->vphy_id);
    return -1;
  }
  // Configure downlink receiver for the SI-RNTI since will be the only one we'll use.
  // This is the User RNTI.
  srslte_ue_dl_set_rnti(&vphy_rx_thread_ctx->ue_dl, vphy_rx_thread_ctx->rnti);
  // Set the expected CFI.
  srslte_ue_dl_set_expected_cfi(&vphy_rx_thread_ctx->ue_dl, DEFAULT_CFI);
  // Enable estimation of CFO based on CSR signals.
  srslte_ue_dl_set_cfo_csr(&vphy_rx_thread_ctx->ue_dl, false);
  // Set the maxium number of turbo decoder iterations.
  srslte_ue_dl_set_max_noi(&vphy_rx_thread_ctx->ue_dl, vphy_rx_thread_ctx->max_turbo_decoder_noi);
  // Enable or disable decoding of PDCCH/PCFICH control channels. If true, it decodes PDCCH/PCFICH otherwise, decodes SCH control.
  srslte_ue_dl_set_decode_pdcch(&vphy_rx_thread_ctx->ue_dl, vphy_rx_thread_ctx->decode_pdcch);
  // Set initial CFO for ue_sync to 0.
  srslte_ue_sync_set_cfo(&vphy_rx_thread_ctx->ue_sync, 0.0);
  //Everything went well.
  return 0;
}

// Free all related UE Downlink structures.
void vphy_rx_ue_free(vphy_rx_thread_context_t* const vphy_rx_thread_ctx) {
  // Free all UE related structures.
  srslte_ue_dl_free(&vphy_rx_thread_ctx->ue_dl);
  PHY_RX_INFO("vPHY Rx ID: %d - srslte_ue_dl_free done!\n", vphy_rx_thread_ctx->vphy_id);
  srslte_ue_sync_free_except_reentry(&vphy_rx_thread_ctx->ue_sync);
  PHY_RX_INFO("vPHY Rx ID: %d - srslte_ue_sync_free_except_reentry done!\n", vphy_rx_thread_ctx->vphy_id);
}

// Initialize struture with Cell parameters.
void vphy_rx_init_cell_parameters(vphy_rx_thread_context_t* const vphy_rx_thread_ctx, transceiver_args_t* const args) {
  vphy_rx_thread_ctx->vphy_ue.nof_prb         = args->nof_prb;          // nof_prb
  vphy_rx_thread_ctx->vphy_ue.nof_ports       = args->nof_ports;        // nof_ports
  vphy_rx_thread_ctx->vphy_ue.bw_idx          = 0;                      // bw idx
  vphy_rx_thread_ctx->vphy_ue.id              = args->radio_id;         // cell_id
  vphy_rx_thread_ctx->vphy_ue.cp              = args->phy_cylic_prefix; // cyclic prefix
  vphy_rx_thread_ctx->vphy_ue.phich_length    = SRSLTE_PHICH_NORM;      // PHICH length
  vphy_rx_thread_ctx->vphy_ue.phich_resources = SRSLTE_PHICH_R_1;       // PHICH resources
}

// Initialize structure with last configured Rx slot control.
void vphy_rx_init_last_slot_control(vphy_rx_thread_context_t* const vphy_rx_thread_ctx, transceiver_args_t* const args) {
  vphy_rx_thread_ctx->last_rx_slot_control.send_to      = args->node_id;
  vphy_rx_thread_ctx->last_rx_slot_control.bw_idx       = helpers_get_bw_index_from_prb(args->nof_prb); // Convert from number of Resource Blocks to BW Index.
  vphy_rx_thread_ctx->last_rx_slot_control.frame        = 0;
  vphy_rx_thread_ctx->last_rx_slot_control.slot         = 0;
  vphy_rx_thread_ctx->last_rx_slot_control.mcs          = 0;
  vphy_rx_thread_ctx->last_rx_slot_control.length       = 1;
}

void vphy_rx_initialize_reader(vphy_rx_thread_context_t* const vphy_rx_thread_ctx, transceiver_args_t* const args) {
  // Calculate number of channels.
  vphy_rx_thread_ctx->channelizer_nof_channels = (uint32_t)(helpers_get_bw_from_nprb(args->radio_nof_prb)/helpers_get_bw_from_nprb(args->nof_prb));
  // ID of this virtual Rx PHY.
  vphy_rx_thread_ctx->vphy_reader.vphy_id = vphy_rx_thread_ctx->vphy_id;
  // Set current buffer being read.
  vphy_rx_thread_ctx->vphy_reader.current_buffer_idx = 0; // MUST be initialized to zero.
  // Calculate number of samples to read.
  uint32_t nof_samples_to_read = (uint32_t)(0.001*helpers_get_bw_from_nprb(args->radio_nof_prb)); // Get number of samples to read.
  // Set number of remaining samples in current buffer.
  vphy_rx_thread_ctx->vphy_reader.nof_remaining_samples = nof_samples_to_read/vphy_rx_thread_ctx->channelizer_nof_channels;
  // Set number of samples read from the current buffer.
  vphy_rx_thread_ctx->vphy_reader.nof_read_samples = 0; // MUST be initialized to zero.
  // Set default Rx channel. The default channel each vPHY Rx listens to increases from the default channel plus the vphy_id.
  vphy_rx_set_reader_channel(vphy_rx_thread_ctx, (args->default_rx_channel + vphy_rx_thread_ctx->vphy_id));
}

void set_number_of_expected_slots(vphy_rx_thread_context_t* const vphy_rx_thread_ctx, uint32_t length) {
  // Lock a mutex prior to using the slot control object.
  pthread_mutex_lock(&vphy_rx_thread_ctx->phy_rx_slot_control_mutex);
  vphy_rx_thread_ctx->last_rx_slot_control.length = length;
  // Unlock mutex upon using the slot control object.
  pthread_mutex_unlock(&vphy_rx_thread_ctx->phy_rx_slot_control_mutex);
}

uint32_t get_number_of_expected_slots(vphy_rx_thread_context_t* const vphy_rx_thread_ctx) {
  uint32_t length;
  // Lock a mutex prior to using the slot control object.
  pthread_mutex_lock(&vphy_rx_thread_ctx->phy_rx_slot_control_mutex);
  length = vphy_rx_thread_ctx->last_rx_slot_control.length;
  // Unlock mutex upon using the slot control object.
  pthread_mutex_unlock(&vphy_rx_thread_ctx->phy_rx_slot_control_mutex);
  return length;
}

void set_bw_index(vphy_rx_thread_context_t* const vphy_rx_thread_ctx, uint32_t bw_index) {
  // Lock a mutex prior to using the slot control object.
  pthread_mutex_lock(&vphy_rx_thread_ctx->phy_rx_slot_control_mutex);
  vphy_rx_thread_ctx->last_rx_slot_control.bw_idx = bw_index;
  // Unlock mutex upon using the slot control object.
  pthread_mutex_unlock(&vphy_rx_thread_ctx->phy_rx_slot_control_mutex);
}

uint32_t get_bw_index(vphy_rx_thread_context_t* const vphy_rx_thread_ctx) {
  uint32_t bw_index;
  // Lock a mutex prior to using the slot control object.
  pthread_mutex_lock(&vphy_rx_thread_ctx->phy_rx_slot_control_mutex);
  bw_index = vphy_rx_thread_ctx->last_rx_slot_control.bw_idx;
  // Unlock mutex upon using the slot control object.
  pthread_mutex_unlock(&vphy_rx_thread_ctx->phy_rx_slot_control_mutex);
  return bw_index;
}

void vphy_rx_set_reader_channel(vphy_rx_thread_context_t* const vphy_rx_thread_ctx, uint32_t channel) {
  // Lock a mutex prior to using the slot control object.
  pthread_mutex_lock(&vphy_rx_thread_ctx->phy_rx_slot_control_mutex);
  vphy_rx_thread_ctx->vphy_reader.vphy_channel = vphy_rx_translate_channel(vphy_rx_thread_ctx, channel);
  // Unlock mutex upon using the slot control object.
  pthread_mutex_unlock(&vphy_rx_thread_ctx->phy_rx_slot_control_mutex);
}

uint32_t vphy_rx_get_reader_channel(vphy_rx_thread_context_t* const vphy_rx_thread_ctx) {
  int channel;
  // Lock a mutex prior to using the slot control object.
  pthread_mutex_lock(&vphy_rx_thread_ctx->phy_rx_slot_control_mutex);
  channel = vphy_rx_thread_ctx->vphy_reader.vphy_channel - (vphy_rx_thread_ctx->channelizer_nof_channels/2);
  if(channel < 0) {
    channel += vphy_rx_thread_ctx->channelizer_nof_channels;
  }
  // Unlock mutex upon using the slot control object.
  pthread_mutex_unlock(&vphy_rx_thread_ctx->phy_rx_slot_control_mutex);
  return (uint32_t)channel;
}

uint32_t vphy_rx_translate_channel(vphy_rx_thread_context_t* const vphy_rx_thread_ctx, uint32_t channel) {
  return ((channel + (vphy_rx_thread_ctx->channelizer_nof_channels/2)) % vphy_rx_thread_ctx->channelizer_nof_channels);
}

int vphy_rx_change_parameters(uint32_t vphy_id, slot_ctrl_t* const slot_ctrl) {
  // Create an alias to this vPHY Rx thread context.
  vphy_rx_thread_context_t *vphy_rx_thread = vphy_rx_threads[vphy_id];
  // Change channel only if one of the parameters have changed.
  if(vphy_rx_get_reader_channel(vphy_rx_thread) != vphy_rx_translate_channel(vphy_rx_thread, slot_ctrl->ch) || vphy_rx_thread->last_rx_slot_control.bw_idx != slot_ctrl->bw_idx) {
    // If bandwidth has changed then free and initialize UE DL.
    if(vphy_rx_thread->last_rx_slot_control.bw_idx != slot_ctrl->bw_idx) {
      // Stop synchronization thread before configuring a new bandwidth.
      if(vphy_rx_stop_sync_thread(vphy_rx_thread) < 0) {
        PHY_RX_ERROR("vPHY Rx ID: %d - Error stopping synchronization thread.\n", vphy_rx_thread->vphy_id);
        return -1;
      }
      // Stop decoding thread before configuring a new bandwidth.
      if(vphy_rx_stop_decoding_thread(vphy_rx_thread) < 0) {
        PHY_RX_ERROR("vPHY Rx ID: %d - Error stopping decoding thread.\n", vphy_rx_thread->vphy_id);
        return -1;
      }
      // Set the new number of PRB based on PRB retrieved from BW index..
      vphy_rx_thread->vphy_ue.nof_prb = helpers_get_prb_from_bw_index(slot_ctrl->bw_idx);
      // Free all Rx related structures.
      vphy_rx_ue_free(vphy_rx_thread);
      // Initialize all Rx related structures.
      if(vphy_rx_ue_init(vphy_rx_thread) < 0) {
        PHY_RX_ERROR("vPHY Rx ID: %d - Error initializing synch and decoding structures.\n", vphy_rx_thread->vphy_id);
        return -1;
      }
      // After configuring a new bandwidth the synchronization thread needs to be restarted.
      if(vphy_rx_start_sync_thread(vphy_rx_thread) < 0) {
        PHY_RX_ERROR("vPHY Rx ID: %d - Error starting synchronization thread.\n", vphy_rx_thread->vphy_id);
        return -1;
      }
      // After configuring a new bandwidth the decoding thread needs to be restarted.
      if(vphy_rx_start_decoding_thread(vphy_rx_thread) < 0) {
        PHY_RX_ERROR("vPHY Rx ID: %d - Error starting decoding thread.\n", vphy_rx_thread->vphy_id);
        return -1;
      }
    }
    // Update reader channel.
    vphy_rx_set_reader_channel(vphy_rx_thread, slot_ctrl->ch);
    // Update last Rx slot control structure with BW index.
    set_bw_index(vphy_rx_thread, slot_ctrl->bw_idx);
    // Retrieve PHY BW for logging purposes.
    float vphy_sampling_rate = helpers_get_bandwidth_float(slot_ctrl->bw_idx);
    if(vphy_sampling_rate < 0.0) {
      PHY_RX_ERROR("vPHY Rx ID: %d - Undefined BW Index: %d....\n", vphy_rx_thread->vphy_id, slot_ctrl->bw_idx);
      return -1;
    }
    PHY_RX_INFO_TIME("vPHY Rx ID: %d - Rx <--- BW[%d]: %1.1f [MHz] - Channel: %d\n", vphy_rx_thread->vphy_id, slot_ctrl->bw_idx, (vphy_sampling_rate/1000000.0), slot_ctrl->ch);
  }

  // Check if number of slots is valid.
  if(slot_ctrl->length <= 0) {
    PHY_RX_ERROR("vPHY Rx ID: %d - Invalid number of slots. It MUST be greater than 0. Current value is: %d\n", vphy_rx_thread->vphy_id, slot_ctrl->length);
    return -1;
  }
  // Set number of slots to be received.
  if(vphy_rx_thread->last_rx_slot_control.length != slot_ctrl->length) {
    set_number_of_expected_slots(vphy_rx_thread, slot_ctrl->length);
    vphy_rx_thread->last_rx_slot_control.length = slot_ctrl->length;
    PHY_RX_DEBUG_TIME("vPHY Rx ID: %d - Expected number of slots set to: %d\n", vphy_rx_thread->vphy_id, slot_ctrl->length);
  }
  // Evevrything went well.
  return 0;
}

// Functions to transfer ue sync structure from sync thread to reception thread.
void vphy_rx_push_ue_sync_to_queue(vphy_rx_thread_context_t *vphy_rx_thread_ctx, short_ue_sync_t* const short_ue_sync) {
  // Lock mutex so that we can push ue sync to queue.
  pthread_mutex_lock(&vphy_rx_thread_ctx->vphy_rx_ue_sync_mutex);
  // Push ue sync into queue.
  sync_cb_push_back(vphy_rx_thread_ctx->synch_handle, short_ue_sync);
  // Unlock mutex so that function can do other things.
  pthread_mutex_unlock(&vphy_rx_thread_ctx->vphy_rx_ue_sync_mutex);
  // Notify other thread that ue sync structure was pushed into queue.
  pthread_cond_signal(&vphy_rx_thread_ctx->vphy_rx_ue_sync_cv);
}

void vphy_rx_pop_ue_sync_from_queue(vphy_rx_thread_context_t *vphy_rx_thread_ctx, short_ue_sync_t* const short_ue_sync) {
  // Lock mutex so that we can pop ue sync structure from queue.
  pthread_mutex_lock(&vphy_rx_thread_ctx->vphy_rx_ue_sync_mutex);
  // Retrieve sync element from circular buffer.
  sync_cb_front(vphy_rx_thread_ctx->synch_handle, short_ue_sync);
  // Remove sync element from  circular buffer.
  sync_cb_pop_front(vphy_rx_thread_ctx->synch_handle);
  // Unlock mutex.
  pthread_mutex_unlock(&vphy_rx_thread_ctx->vphy_rx_ue_sync_mutex);
}

bool vphy_rx_wait_queue_not_empty(vphy_rx_thread_context_t *vphy_rx_thread_ctx) {
  bool ret = true;
  // Lock mutex so that we can wait for ue sync strucure.
  pthread_mutex_lock(&vphy_rx_thread_ctx->vphy_rx_ue_sync_mutex);
  // Wait for conditional variable only if container is empty.
  if(sync_cb_empty(vphy_rx_thread_ctx->synch_handle)) {
    // Wait for conditional variable to be true.
    pthread_cond_wait(&vphy_rx_thread_ctx->vphy_rx_ue_sync_cv, &vphy_rx_thread_ctx->vphy_rx_ue_sync_mutex);
    if(!vphy_rx_thread_ctx->run_phy_decoding_thread || !vphy_rx_thread_ctx->run_phy_synchronization_thread) {
      ret = false;
    }
  }
  // Unlock mutex.
  pthread_mutex_unlock(&vphy_rx_thread_ctx->vphy_rx_ue_sync_mutex);
  return ret;
}

bool vphy_rx_wait_and_pop_ue_sync_from_queue(vphy_rx_thread_context_t *vphy_rx_thread_ctx, short_ue_sync_t* const short_ue_sync) {
  bool ret = true;
  // Lock mutex so that we can wait for ue sync strucure.
  pthread_mutex_lock(&vphy_rx_thread_ctx->vphy_rx_ue_sync_mutex);
  // Wait for conditional variable only if container is empty.
  if(sync_cb_empty(vphy_rx_thread_ctx->synch_handle)) {
    // Wait for conditional variable to be true.
    pthread_cond_wait(&vphy_rx_thread_ctx->vphy_rx_ue_sync_cv, &vphy_rx_thread_ctx->vphy_rx_ue_sync_mutex);
    if(!vphy_rx_thread_ctx->run_phy_decoding_thread || !vphy_rx_thread_ctx->run_phy_synchronization_thread) {
      ret = false;
    }
  }
  // If still running, pop message from container.
  if(ret) {
    // Retrieve sync element from circular buffer.
    sync_cb_front(vphy_rx_thread_ctx->synch_handle, short_ue_sync);
    // Remove sync element from  circular buffer.
    sync_cb_pop_front(vphy_rx_thread_ctx->synch_handle);
  }
  // Unlock mutex.
  pthread_mutex_unlock(&vphy_rx_thread_ctx->vphy_rx_ue_sync_mutex);
  return ret;
}

void *phy_decoding_work(void *h) {
  vphy_rx_thread_context_t *vphy_rx_ctx = (vphy_rx_thread_context_t *)h;
  srslte_ue_dl_t *ue_dl = &vphy_rx_ctx->ue_dl;
  srslte_ue_sync_t *ue_sync = &vphy_rx_ctx->ue_sync;
  int decoded_slot_counter = 0, pdsch_num_rxd_bits;
  float rsrp = 0.0, rsrq = 0.0, noise = 0.0, rssi = 0.0, sinr = 0.0;
  double synch_plus_decoding_time = 0.0, decoding_time = 0.0;
  uint32_t nof_prb = 0, sfn = 0, bw_index = 0;
  phy_stat_t phy_rx_stat;
  short_ue_sync_t short_ue_sync;
  cf_t *subframe_buffer = NULL;
  uint8_t *data = NULL;

  // Get the maximum number of bytes for the highest possible MCS.
  uint32_t max_tb_size = communicator_get_tb_size(communicator_get_bw_index(get_bw_index(vphy_rx_ctx)), MAX_MCS_VALUE);
  // Allocate memory for decoded data vector.
  data = (uint8_t*)srslte_vec_malloc(sizeof(uint8_t)*max_tb_size);
  // Check if memory allocation was correctly done.
  if(data == NULL) {
    PHY_RX_ERROR("vPHY Rx ID: %d - Error allocating memory for decoded data vector.\n", vphy_rx_ctx->vphy_id);
    exit(-1);
  }

  // Set priority to Rx thread.
  uhd_set_thread_priority(1.0, true);

#if(ENABLE_STICKING_THREADS_TO_CORES==1)
  int ret = helpers_stick_this_thread_to_core(vphy_rx_ctx->vphy_id);
  if(ret != 0) {
    PHY_RX_ERROR("--------> Set vPHY Rx ID: %d Decoding thread to CPU ret: %d\n", vphy_rx_ctx->vphy_id, ret);
  }
#endif

  /****************************** PHY Reception loop - BEGIN ******************************/
  PHY_RX_DEBUG("vPHY Rx ID: %d - Entering PHY Decoding thread loop.\n", vphy_rx_ctx->vphy_id);
  while(vphy_rx_wait_and_pop_ue_sync_from_queue(vphy_rx_ctx, &short_ue_sync) && vphy_rx_ctx->run_phy_decoding_thread) {

    // Reset number of decoded PDSCH bits every loop iteration.
    pdsch_num_rxd_bits = 0;

    // Reset decoded slot counter every time a new MAC frame starts.
    if(short_ue_sync.subframe_counter == 1) {
      decoded_slot_counter = 0;
    }

    //vphy_rx_print_ue_sync(&short_ue_sync,"********** decoding thread **********\n");

    // Create an alias to the input buffer containing the synchronized and aligned subframe.
    subframe_buffer = &ue_sync->input_buffer[short_ue_sync.buffer_number][short_ue_sync.subframe_start_index];

#if(WRITE_VPHY_DECODING_SIGNAL_INTO_FILE==1)
    nof_prb = helpers_get_prb_from_bw_index(bw_index);
    static unsigned int dump_cnt2 = 0;
    char output_file_name2[200];
    sprintf(output_file_name2, "vphy_decoding_%d_%d.dat",vphy_rx_ctx->vphy_id,dump_cnt2);
    srslte_filesink_t file_sink2;
    if(dump_cnt2 < 10) {
      filesink_init(&file_sink2, output_file_name2, SRSLTE_COMPLEX_FLOAT_BIN);
      // Write samples into file.
      filesink_write(&file_sink2, subframe_buffer, SRSLTE_SF_LEN(srslte_symbol_sz(nof_prb)));
      // Close file.
      filesink_free(&file_sink2);
      dump_cnt2++;
      PHY_RX_PRINT("File dumped: %d.\n",dump_cnt2);
    }
#endif

    // Retrieve BW index.
    bw_index = get_bw_index(vphy_rx_ctx);

    // Change MCS. For PHY BW 1.4 MHz we can not set MCS 28 for the very first subframe as it carries PSS/SSS and does not have enough "room" for FEC bits.
    if(vphy_rx_ctx->decode_pdcch == false && bw_index == BW_IDX_OneDotFour && short_ue_sync.mcs >= 28 && short_ue_sync.sf_idx == vphy_rx_ctx->initial_subframe_index) {
      // Maximum MCS for 1st subframe of 1.4 MHz PHY BW is 27.
      short_ue_sync.mcs -= 1;
    }

    // Function srslte_ue_dl_decode() returns the number of bits received.
    pdsch_num_rxd_bits = srslte_ue_dl_decode2(ue_dl,
                                             subframe_buffer,
                                             data,
                                             sfn*10+short_ue_sync.sf_idx,
                                             short_ue_sync.mcs);

    // Calculate time it takes to decode control and data (PDCCH/PCFICH/PDSCH or SCH/PDSCH).
    decoding_time = helpers_profiling_diff_time(ue_dl->decoding_start_timestamp);

    //if(vphy_rx_ctx->vphy_id == 2)
    //  printf("vPHY ID: %d - MCS: %d - SF: %d\n", vphy_rx_ctx->vphy_id, short_ue_sync.mcs, sfn*10+short_ue_sync.sf_idx);

#if(WRITE_VPHY_RX_SUBFRAME_INTO_FILE==1)
    nof_prb = helpers_get_prb_from_bw_index(bw_index);
    static unsigned int dump_cnt = 0;
    char output_file_name[200];
    sprintf(output_file_name, "vphy_rx_subframe_vphy_id_%d_mcs_%d_seq_num_%d_cnt_%d.dat", vphy_rx_ctx->vphy_id, short_ue_sync.mcs, phy_rx_stat.seq_number, dump_cnt);
    srslte_filesink_t file_sink;
    if(vphy_rx_ctx->vphy_id == 2 && dump_cnt < 4) {
      filesink_init(&file_sink, output_file_name, SRSLTE_COMPLEX_FLOAT_BIN);
      // Write samples into file.
      filesink_write(&file_sink, subframe_buffer, SRSLTE_SF_LEN(srslte_symbol_sz(nof_prb)));
      // Close file.
      filesink_free(&file_sink);
      dump_cnt++;
      PHY_RX_PRINT("vPHY Rx ID: %d - File dumped: %d.\n", vphy_rx_ctx->vphy_id, dump_cnt);
    }
#endif

    if(pdsch_num_rxd_bits < 0) {
      PHY_RX_ERROR("vPHY Rx ID: %d - Error decoding UE DL.\n", vphy_rx_ctx->vphy_id);
    } else if(pdsch_num_rxd_bits > 0) {

      // Only if packet is really received we update the received slot counter.
      decoded_slot_counter++;
      PHY_RX_DEBUG("vPHY Rx ID: %d - Decoded slot counter: %d\n", vphy_rx_ctx->vphy_id, decoded_slot_counter);

      // Retrieve number pf physical resource blocks.
      nof_prb = helpers_get_prb_from_bw_index(bw_index);
      // Calculate statistics.
      rssi = srslte_vec_avg_power_cf(subframe_buffer, SRSLTE_SF_LEN(srslte_symbol_sz(nof_prb)));
      rsrq = srslte_chest_dl_get_rsrq(&ue_dl->chest);
      rsrp = srslte_chest_dl_get_rsrp(&ue_dl->chest);
      noise = srslte_chest_dl_get_noise_estimate(&ue_dl->chest);

      // Check if the values are valid numbers, if not, set them to 0.
      if(isnan(rssi)) {
        rssi = 0.0;
      }
      if(isnan(rsrq)) {
        rsrq = 0.0;
      }
      if(isnan(noise)) {
        noise = 0.0;
      }
      if(isnan(rsrp)) {
        rsrp = 0.0;
      }
      // Calculate SNR out of RSRP and noise estimation.
      sinr = 10*log10(rsrp/noise);

      // Set PHY Rx Stats with valid values.
      // When data is correctly decoded return SUCCESS status.
      phy_rx_stat.status                                              = PHY_SUCCESS;                                                              // Status tells upper layers that if successfully received data.
      phy_rx_stat.vphy_id                                             = vphy_rx_ctx->vphy_id;
      phy_rx_stat.seq_number                                          = ++vphy_rx_ctx->sequence_number;                                           // Sequence number represents the counter of received slots.
      phy_rx_stat.host_timestamp                                      = helpers_convert_host_timestamp(&short_ue_sync.peak_detection_timestamp);  // Retrieve host's time. Host PC time value when (ch,slot) PHY data are demodulated
      phy_rx_stat.ch                                                  = vphy_rx_get_reader_channel(vphy_rx_ctx);                                  // Set the channel number where the data was received at.
      phy_rx_stat.mcs                                                 = ue_dl->pdsch_cfg.grant.mcs.idx;	                                          // MCS index is decoded when the DCI is found and correctly decoded. Modulation Scheme. Range: [0, 28]. check TBS table num_of_bytes_per_slot_versus_mcs[29] in intf.h to know MCS
      phy_rx_stat.num_cb_total                                        = ue_dl->nof_detected;	                                                    // Number of Code Blocks (CB) received in the (ch, slot)
      phy_rx_stat.num_cb_err                                          = ue_dl->pkt_errors;		                                                    // How many CBs get CRC error in the (ch, slot)
      phy_rx_stat.wrong_decoding_counter                              = ue_dl->wrong_decoding_counter;
      // Assign the values to Rx Stat structure.
      phy_rx_stat.stat.rx_stat.nof_slots_in_frame                     = short_ue_sync.nof_subframes_to_rx;                                        // This field indicates the number decoded from SSS, indicating the number of subframes part of a MAC frame.
      phy_rx_stat.stat.rx_stat.slot_counter                           = short_ue_sync.subframe_counter;                                           // This field indicates the slot number inside of a MAC frame.
      phy_rx_stat.stat.rx_stat.cqi                                    = srslte_cqi_from_snr(sinr);                                                 // Channel Quality Indicator. Range: [1, 15]
      phy_rx_stat.stat.rx_stat.rssi                                   = 10*log10(rssi);			                                                      // Received Signal Strength Indicator. Range: [–2^31, (2^31) - 1]. dBm*10. For example, value -567 means -56.7dBm.
      phy_rx_stat.stat.rx_stat.rsrp                                   = 10*log10(rsrp);				                                                    // Reference Signal Received Power. Range: [-1400, -400]. dBm*10. For example, value -567 means -56.7dBm.
      phy_rx_stat.stat.rx_stat.rsrq                                   = 10*log10(rsrq);				                                                    // Reference Signal Receive Quality. Range: [-340, -25]. dB*10. For example, value 301 means 30.1 dB.
      phy_rx_stat.stat.rx_stat.sinr                                   = sinr; 			                                                                // Signal to Interference plus Noise Ratio. Range: [–2^31, (2^31) - 1]. dB*10. For example, value 256 means 25.6 dB.
      phy_rx_stat.stat.rx_stat.cfo                                    = short_ue_sync.cfo/1000.0;                                                 // CFO value given in KHz
      phy_rx_stat.stat.rx_stat.peak_value                             = short_ue_sync.peak_value;
      phy_rx_stat.stat.rx_stat.noise                                  = ue_dl->noise_estimate;
      phy_rx_stat.stat.rx_stat.last_noi                               = srslte_ue_dl_last_noi(ue_dl);
      phy_rx_stat.stat.rx_stat.detection_errors                       = ue_dl->wrong_decoding_counter;
      phy_rx_stat.stat.rx_stat.decoding_errors                        = ue_dl->pkt_errors;
      phy_rx_stat.stat.rx_stat.filler_bits_error                      = ue_dl->pdsch.dl_sch.filler_bits_error;
      phy_rx_stat.stat.rx_stat.nof_cbs_exceeds_softbuffer_size_error  = ue_dl->pdsch.dl_sch.nof_cbs_exceeds_softbuffer_size_error;
      phy_rx_stat.stat.rx_stat.rate_matching_error                    = ue_dl->pdsch.dl_sch.rate_matching_error;
      phy_rx_stat.stat.rx_stat.cb_crc_error                           = ue_dl->pdsch.dl_sch.cb_crc_error;
      phy_rx_stat.stat.rx_stat.tb_crc_error                           = ue_dl->pdsch.dl_sch.tb_crc_error;
      phy_rx_stat.stat.rx_stat.total_packets_synchronized             = ue_dl->pkts_total;                                                        // Total number of slots synchronized. It contains correct and wrong slots.
      phy_rx_stat.stat.rx_stat.length                                 = pdsch_num_rxd_bits/8;				                                              // How many bytes are after this header. It should be equal to current TB size.
      phy_rx_stat.stat.rx_stat.data                                   = data;
      phy_rx_stat.stat.rx_stat.decoding_time                          = decoding_time;                                                            // Time for subframe decoding in milliseconds.
      // Calculate decoding time based on peak detection timestamp or start of each iteration of subframe TRACK state.
      if(short_ue_sync.subframe_counter == 1) {
        synch_plus_decoding_time = helpers_profiling_diff_time(short_ue_sync.peak_detection_timestamp);
      } else {
        synch_plus_decoding_time = helpers_profiling_diff_time(short_ue_sync.subframe_track_start);
      }
      phy_rx_stat.stat.rx_stat.synch_plus_decoding_time = synch_plus_decoding_time; // Time elapsed since peak is detected and subframe is decoded in milliseconds.

      //PHY_PROFILLING_AVG3("Avg. sync + decoding time: %f - min: %f - max: %f - max counter %d - diff >= 0.5 ms: %d - total counter: %d - perc: %f\n", synch_plus_decoding_time, 0.5, 1000);

      //PHY_PROFILLING_AVG3("Avg. read samples + sync + decoding time: %f - min: %f - max: %f - max counter %d - diff >= 2ms: %d - total counter: %d - perc: %f\n", helpers_profiling_diff_time(short_ue_sync.start_of_rx_sample), 2.0, 10000);

      // Information on data decoding process.
      PHY_RX_INFO_TIME("[Rx STATS]: vPHY Rx ID: %d - Rx slots: %d - Channel: %d - Rx bytes: %d - CFO: %+2.2f [kHz] - Peak value: %1.2f - Noise: %1.4f - RSSI: %1.2f [dBm] - SINR: %4.1f [dB] - RSRQ: %1.2f [dB] - CQI: %d - MCS: %d - Total: %d - Error: %d - Last NOI: %d - Avg. NOI: %1.2f - Decoding time: %f [ms]\n", vphy_rx_ctx->vphy_id, decoded_slot_counter, phy_rx_stat.ch, phy_rx_stat.stat.rx_stat.length, short_ue_sync.cfo/1000.0, short_ue_sync.peak_value, ue_dl->noise_estimate, phy_rx_stat.stat.rx_stat.rssi, phy_rx_stat.stat.rx_stat.sinr, phy_rx_stat.stat.rx_stat.rsrq, phy_rx_stat.stat.rx_stat.cqi, phy_rx_stat.mcs, ue_dl->nof_detected, ue_dl->pkt_errors, srslte_ue_dl_last_noi(ue_dl), srslte_ul_dl_average_noi(ue_dl), synch_plus_decoding_time);

      DEV_INFO("vPHY Rx ID: %d - Out of sequence message: - SINR: %1.4f - Rx byte: %d - host timestamp: %" PRIu64 "\n", vphy_rx_ctx->vphy_id, phy_rx_stat.stat.rx_stat.sinr,data[0],phy_rx_stat.host_timestamp);

      // Uncomment this line to measure the number of packets received in one second.
      //helpers_measure_packets_per_second("Rx");

#if(ENABLE_TX_TO_RX_TIME_DIFF==1)
      // Enable add_tx_timestamp to measure the time it takes for a transmitted packet to be received.
      if(vphy_rx_ctx->add_tx_timestamp) {
        uint64_t rx_timestamp, tx_timestamp;
        struct timespec rx_timestamp_struct;
        clock_gettime(CLOCK_REALTIME, &rx_timestamp_struct);
        rx_timestamp = helpers_convert_host_timestamp(&rx_timestamp_struct);
        memcpy((void*)&tx_timestamp, (void*)data, sizeof(uint64_t));
        PHY_PROFILLING_AVG3("Diff between Rx and Rx time: %0.4f [s] - min: %f - max: %f - max counter %d - diff >= 1ms: %d - total counter: %d - perc: %f\n", (double)((rx_timestamp-tx_timestamp)/1000000000.0), 0.001, 1000);
      }
#endif

#if(ENBALE_VPHY_RX_INFO_PLOT==1)
      // Plot some useful information on the decoded data.
      if(vphy_rx_ctx->plot_rx_info == true && vphy_rx_ctx->vphy_id == vphy_rx_ctx->vphy_id_rx_info) {
        update_ue_decoding_plot(ue_dl, ue_sync);
      }
#endif

      // Send phy received (Rx) statistics and TB (data) to upper layers.
      vphy_rx_send_rx_statistics(&phy_rx_stat);
    } else {
      // There was an error if the code reaches this point: (1) wrong CFI or DCI detected or (2) data was incorrectly decoded.
      rssi = 10.0*log10f(srslte_vec_avg_power_cf(subframe_buffer, short_ue_sync.frame_len));
      phy_rx_stat.status                                              = PHY_ERROR;
      phy_rx_stat.vphy_id                                             = vphy_rx_ctx->vphy_id;
      phy_rx_stat.seq_number                                          = ++vphy_rx_ctx->sequence_number;
      phy_rx_stat.ch                                                  = vphy_rx_get_reader_channel(vphy_rx_ctx);
      phy_rx_stat.num_cb_total                                        = ue_dl->nof_detected;
      phy_rx_stat.stat.rx_stat.nof_slots_in_frame                     = short_ue_sync.nof_subframes_to_rx;  // This field indicates the number decoded from SSS, indicating the number of subframes part of a MAC frame.
      phy_rx_stat.stat.rx_stat.slot_counter                           = short_ue_sync.subframe_counter;     // This field indicates the slot number inside of a MAC frame.
      phy_rx_stat.stat.rx_stat.detection_errors                       = ue_dl->wrong_decoding_counter;
      phy_rx_stat.stat.rx_stat.decoding_errors                        = ue_dl->pkt_errors;
      phy_rx_stat.stat.rx_stat.filler_bits_error                      = ue_dl->pdsch.dl_sch.filler_bits_error;
      phy_rx_stat.stat.rx_stat.nof_cbs_exceeds_softbuffer_size_error  = ue_dl->pdsch.dl_sch.nof_cbs_exceeds_softbuffer_size_error;
      phy_rx_stat.stat.rx_stat.rate_matching_error                    = ue_dl->pdsch.dl_sch.rate_matching_error;
      phy_rx_stat.stat.rx_stat.cb_crc_error                           = ue_dl->pdsch.dl_sch.cb_crc_error;
      phy_rx_stat.stat.rx_stat.tb_crc_error                           = ue_dl->pdsch.dl_sch.tb_crc_error;
      phy_rx_stat.stat.rx_stat.rssi                                   = rssi;
      phy_rx_stat.stat.rx_stat.cfo                                    = short_ue_sync.cfo/1000.0;  // CFO value given in KHz
      phy_rx_stat.stat.rx_stat.peak_value                             = short_ue_sync.peak_value;
      phy_rx_stat.stat.rx_stat.noise                                  = ue_dl->noise_estimate;
      phy_rx_stat.stat.rx_stat.decoded_cfi                            = ue_dl->decoded_cfi;
      phy_rx_stat.stat.rx_stat.found_dci                              = ue_dl->found_dci;
      phy_rx_stat.stat.rx_stat.last_noi                               = srslte_ue_dl_last_noi(ue_dl);
      phy_rx_stat.stat.rx_stat.total_packets_synchronized             = ue_dl->pkts_total;
      // Send phy received (Rx) statistics and TB (data) to upper layers.
      vphy_rx_send_rx_statistics(&phy_rx_stat);
      PHY_RX_INFO_TIME("[Rx STATS]: vPHY Rx ID: %d - Detection errors: %d - Channel: %d - CFO: %+2.2f [kHz] - Peak value: %1.2f - RSSI: %3.2f [dBm] - Decoded CFI: %d - Found DCI: %d - Last NOI: %d - Avg. NOI: %1.2f - Noise: %1.4f - Decoding errors: %d\n",vphy_rx_ctx->vphy_id,ue_dl->wrong_decoding_counter,vphy_rx_get_reader_channel(vphy_rx_ctx),short_ue_sync.cfo/1000.0,short_ue_sync.peak_value,rssi,ue_dl->decoded_cfi,ue_dl->found_dci,srslte_ue_dl_last_noi(ue_dl), srslte_ul_dl_average_noi(ue_dl), ue_dl->noise_estimate,ue_dl->pkt_errors);

#if(WRITE_VPHY_RX_WRONGLY_DECODED_SUBFRAME_INTO_FILE==1)
      static unsigned int dump_cnt[12] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
      char output_file_name[200];
      sprintf(output_file_name, "wrong_decoding_subframe_vphy_id_%d_seq_num_%d_peak_%1.2f_cnt_%d.dat", vphy_rx_ctx->vphy_id, phy_rx_stat.seq_number, phy_rx_stat.stat.rx_stat.peak_value, dump_cnt[vphy_rx_ctx->vphy_id]);
      srslte_filesink_t file_sink;
      if(phy_rx_stat.stat.rx_stat.peak_value > 3.0 && dump_cnt[vphy_rx_ctx->vphy_id] < 10) {
        filesink_init(&file_sink, output_file_name, SRSLTE_COMPLEX_FLOAT_BIN);
        // Write samples into file.
        filesink_write(&file_sink, subframe_buffer, SRSLTE_SF_LEN(srslte_symbol_sz(nof_prb)));
        // Close file.
        filesink_free(&file_sink);
        dump_cnt[vphy_rx_ctx->vphy_id]++;
        PHY_RX_PRINT("vPHY Rx ID: %d - Wrong decoding file dumped: %d.\n", vphy_rx_ctx->vphy_id, dump_cnt[vphy_rx_ctx->vphy_id]);
      }
#endif

    }

  }
  // Free memory used to store decoded data.
  if(data) {
    free(data);
    data = NULL;
  }
  /****************************** PHY Decoding loop - END ******************************/
  PHY_RX_DEBUG("vPHY Rx ID: %d - Leaving vPHY Rx Decoding thread.\n", vphy_rx_ctx->vphy_id);
  // Exit thread with result code.
  pthread_exit(NULL);
}

void vphy_rx_send_rx_statistics(phy_stat_t* const phy_rx_stat) {
  // Set values to the Rx Stats Structure.
  // Set frame number.
  phy_rx_stat->frame = 0;
  // Set time slot number.
  phy_rx_stat->slot = 0;
  // Set some default values.
  if(phy_rx_stat->status == PHY_SUCCESS) {
    phy_rx_stat->stat.rx_stat.decoded_cfi = 1;
    phy_rx_stat->stat.rx_stat.found_dci = 1;
    phy_rx_stat->stat.rx_stat.gain = 0;
  } else if(phy_rx_stat->status == PHY_TIMEOUT) {
    phy_rx_stat->host_timestamp = helpers_get_host_time_now(); // Host PC time value when reception timesout.
    phy_rx_stat->mcs = 100;
    phy_rx_stat->num_cb_total = 0;
    phy_rx_stat->num_cb_err = 0;
    phy_rx_stat->stat.rx_stat.cqi = 100;
    phy_rx_stat->stat.rx_stat.rssi = 0.0;
    phy_rx_stat->stat.rx_stat.rsrp = 0.0;
    phy_rx_stat->stat.rx_stat.rsrq = 0.0;
    phy_rx_stat->stat.rx_stat.sinr = 0.0;
    phy_rx_stat->stat.rx_stat.length = 0;
    phy_rx_stat->stat.rx_stat.data = NULL;
  } else if(phy_rx_stat->status == PHY_ERROR) {
    phy_rx_stat->host_timestamp = 0;
    phy_rx_stat->mcs = 100;
    phy_rx_stat->num_cb_err = 0;
    phy_rx_stat->stat.rx_stat.gain = 0;
    phy_rx_stat->stat.rx_stat.cqi = 0;
    phy_rx_stat->stat.rx_stat.rsrp = 0;
    phy_rx_stat->stat.rx_stat.rsrq = 0;
    phy_rx_stat->stat.rx_stat.sinr = 0;
    phy_rx_stat->stat.rx_stat.length = 0;
    phy_rx_stat->stat.rx_stat.data = NULL;
  }

  // Sending PHY Rx statistics to upper layer.
  PHY_RX_DEBUG("vPHY Rx ID: %d - Sending Rx statistics information upwards...\n", phy_rx_stat->vphy_id);
  // Send Rx stats. There is a mutex on this function which prevents multiple vPHYs from sending Rx statistics to MAC at the same time.
  phy_comm_ctrl_send_rx_statistics(phy_rx_stat);
}

// This is the function called when the synchronization thread is called/started.
void *vphy_rx_sync_frame_type_one_work(void *h) {

  int ret;
  vphy_rx_thread_context_t *vphy_rx_ctx = (vphy_rx_thread_context_t *)h;
  srslte_ue_sync_t *ue_sync = &vphy_rx_ctx->ue_sync;
  short_ue_sync_t short_ue_sync;

#if(CHECK_TIME_BETWEEN_SYNC_ITER==1)
  struct timespec time_between_synchs;
#endif

  // Set priority to Rx thread.
  uhd_set_thread_priority(1.0, true);

  // Set some constant parameters of short_ue_sync structure.
  short_ue_sync.frame_len = ue_sync->frame_len;

  PHY_RX_DEBUG("vPHY Rx ID: %d - Entering PHY synchronization thread loop.\n", vphy_rx_ctx->vphy_id);
  while(vphy_rx_ctx->run_phy_synchronization_thread) {

#if(CHECK_TIME_BETWEEN_SYNC_ITER==1)
    double diff = helpers_profiling_diff_time(&time_between_synchs);
    clock_gettime(CLOCK_REALTIME, &time_between_synchs);
    PHY_RX_PRINT("[SYNC] time between synch iterations: %f\n",diff);
#endif

    // Timestamp the start of the reception procedure. We calculate the total time it takes to read, sync and decode the subframe on the decoding thread.
    //clock_gettime(CLOCK_REALTIME, &short_ue_sync.start_of_rx_sample);

    // Set watchdog timer for synchronization thread. Always wait for some seconds.
#if(ENABLE_WATCHDOG_TIMER==1)
    if(timer_set(&vphy_rx_ctx->vphy_rx_sync_timer_id, 2) < 0) {
      PHY_RX_ERROR("vPHY Rx ID: %d - Not possible to set the watchdog timer for vPHY Rx sync thread.\n", vphy_rx_ctx->vphy_id);
    }
#endif

    // synchronize and align subframes.
    ret = srslte_ue_sync_get_subframe_with_hyper_rx(ue_sync);

    // srslte_ue_sync_get_buffer_new() returns 1 if it successfully synchronizes to a slot (also known as subframe).
    if(ret == 1) {
      //PHY_PROFILLING_AVG3("Average synchronization time: %f - min: %f - max: %f - max counter %d - diff >= 0.5 ms: %d - total counter: %d - perc: %f\n",helpers_profiling_diff_time(ue_sync->sfind.peak_detection_timestamp), 1.0, 1000);

      //struct timespec start_push_queue;
      //clock_gettime(CLOCK_REALTIME, &start_push_queue);

      // Update the short ue sync strucure with the current subframe counter number and other parameters.
      short_ue_sync.buffer_number             = ue_sync->previous_subframe_buffer_counter_value;
      short_ue_sync.subframe_start_index      = ue_sync->subframe_start_index;
      short_ue_sync.sf_idx                    = ue_sync->sf_idx;
      short_ue_sync.peak_value                = ue_sync->sfind.peak_value;
      short_ue_sync.peak_detection_timestamp  = ue_sync->sfind.peak_detection_timestamp;
      short_ue_sync.cfo                       = srslte_ue_sync_get_carrier_freq_offset(&ue_sync->sfind);
      short_ue_sync.mcs                       = ue_sync->sfind.mcs;

      // Push ue sync strucute to queue (FIFO).
      vphy_rx_push_ue_sync_to_queue(vphy_rx_ctx, &short_ue_sync);

      //double diff_queue = helpers_profiling_diff_time(start_push_queue);
      //if(diff_queue > 0.05)
      //  printf("Average decoding time: %f\n",diff_queue);

      //vphy_rx_print_ue_sync(&short_ue_sync,"********** synchronization thread **********\n");
    } else if(ret == 0) {
      // No slot synchronized and aligned. We don't do nothing for now.
    } else if(ret < 0) {
      PHY_RX_ERROR("vPHY Rx ID: %d - Error calling srslte_ue_sync_get_buffer_new()\n", vphy_rx_ctx->vphy_id);
      continue;
    }

    // We increment the subframe counter the first time if we have just found a peak and the subframe data was correctly decoded.
    if(ue_sync->last_state == SF_FIND && ue_sync->state == SF_TRACK) {
      ue_sync->subframe_counter = 1;
      PHY_RX_DEBUG("vPHY Rx ID: %d - First increment of subframe counter: %d.\n", vphy_rx_ctx->vphy_id, ue_sync->subframe_counter);
    } else if(ue_sync->last_state == SF_TRACK && ue_sync->state == SF_TRACK && ue_sync->subframe_counter > 0) {
      // Increment the subframe counter in order to receive the correct number of subframes.
      ue_sync->subframe_counter++;
    } else {
      ue_sync->subframe_counter = 0;
    }

    if(ue_sync->subframe_counter >= get_number_of_expected_slots(vphy_rx_ctx)) {
      // Light weight way to reset ue_sync for new reception.
      if(srslte_ue_sync_init_reentry_loop(ue_sync)) {
        PHY_RX_ERROR("vPHY Rx ID: %d - Error re-initiating ue_sync\n", vphy_rx_ctx->vphy_id);
        continue;
      }
    }

    // Disarm watchdog timer for synchronization thread.
#if(ENABLE_WATCHDOG_TIMER==1)
    if(timer_disarm(&vphy_rx_ctx->vphy_rx_sync_timer_id) < 0) {
      PHY_RX_ERROR("vPHY Rx ID: %d - Not possible to disarm the watchdog timer for vPHY Rx sync thread.\n", vphy_rx_ctx->vphy_id);
    }
#endif
  }

  PHY_RX_DEBUG("vPHY Rx ID: %d - Leaving PHY synchronization thread.\n", vphy_rx_ctx->vphy_id);
  // Exit thread with result code.
  pthread_exit(NULL);
}

// This is the function called when the synchronization thread is called/started.
void *vphy_rx_sync_frame_type_two_work(void *h) {

  int ret;
  vphy_rx_thread_context_t *vphy_rx_ctx = (vphy_rx_thread_context_t *)h;
  srslte_ue_sync_t *ue_sync = &vphy_rx_ctx->ue_sync;
  short_ue_sync_t short_ue_sync;

#if(CHECK_TIME_BETWEEN_SYNC_ITER==1)
  struct timespec time_between_synchs;
#endif

  // Set priority to RX thread.
  uhd_set_thread_priority(1.0, true);

#if(ENABLE_STICKING_THREADS_TO_CORES==1)
  ret = helpers_stick_this_thread_to_core(vphy_rx_ctx->vphy_id);
  if(ret != 0) {
    PHY_RX_ERROR("--------> Set vPHY Rx ID: %d Synch thread to CPU ret: %d\n", vphy_rx_ctx->vphy_id, ret);
  }
#endif

  // Set some constant parameters of short_ue_sync structure.
  short_ue_sync.frame_len = ue_sync->frame_len;

  // Initialize subframe counter to 0.
  ue_sync->subframe_counter = 0;

  PHY_RX_DEBUG("vPHY Rx ID: %d - Entering PHY synchronization thread loop.\n", vphy_rx_ctx->vphy_id);
  while(vphy_rx_ctx->run_phy_synchronization_thread) {

#if(CHECK_TIME_BETWEEN_SYNC_ITER==1)
    double diff = helpers_profiling_diff_time(&time_between_synchs);
    clock_gettime(CLOCK_REALTIME, &time_between_synchs);
    PHY_RX_PRINT("vPHY Rx ID: %d - [SYNC] time between synch iterations: %f\n", vphy_rx_ctx->vphy_id, diff);
#endif

    // Timestamp the start of the reception procedure. We calculate the total time it takes to read, sync and decode the subframe on the decoding thread.
    //clock_gettime(CLOCK_REALTIME, &short_ue_sync.start_of_rx_sample);

    // Create watchdog timer for synchronization thread. Always wait for some seconds.
#if(ENABLE_WATCHDOG_TIMER==1)
    if(timer_set(&vphy_rx_ctx->vphy_rx_sync_timer_id, 2) < 0) {
      PHY_RX_ERROR("vPHY Rx ID: %d - Not possible to set the watchdog timer for sync thread.\n", vphy_rx_ctx->vphy_id);
    }
#endif

    // synchronize and align subframes.
    ret = srslte_ue_sync_get_subframe_with_hyper_rx(ue_sync);

    // srslte_ue_sync_get_buffer_new() returns 1 if it successfully synchronizes to a slot (also known as subframe).
    if(ret == 1) {
      //PHY_PROFILLING_AVG3("Average synchronization time: %f - min: %f - max: %f - max counter %d - diff >= 0.5 ms: %d - total counter: %d - perc: %f\n",helpers_profiling_diff_time(ue_sync->sfind.peak_detection_timestamp), 1.0, 1000);

      //struct timespec start_push_queue;
      //clock_gettime(CLOCK_REALTIME, &start_push_queue);

      // We increment the subframe counter the first time if we have just found a peak and the subframe data was correctly decoded.
      if(ue_sync->last_state == SF_FIND && ue_sync->state == SF_TRACK && ue_sync->subframe_counter == 0) {
        ue_sync->subframe_counter = 1;
        PHY_RX_DEBUG("vPHY Rx ID: %d - First increment of subframe counter: %d.\n", vphy_rx_ctx->vphy_id, ue_sync->subframe_counter);
      } else if(ue_sync->last_state == SF_TRACK && ue_sync->state == SF_TRACK && ue_sync->subframe_counter > 0) {
        // Increment the subframe counter in order to receive the correct number of subframes.
        ue_sync->subframe_counter++;
      } else {
        PHY_RX_ERROR("vPHY Rx ID: %d - There was something wrong with the subframe_counter.\n", vphy_rx_ctx->vphy_id);
        continue;
      }

      // Update the short ue sync structure with the current subframe counter number and other parameters.
      short_ue_sync.buffer_number             = ue_sync->previous_subframe_buffer_counter_value;
      short_ue_sync.subframe_start_index      = ue_sync->subframe_start_index;
      short_ue_sync.sf_idx                    = ue_sync->sf_idx;
      short_ue_sync.peak_value                = ue_sync->sfind.peak_value;
      short_ue_sync.peak_detection_timestamp  = ue_sync->sfind.peak_detection_timestamp;
      short_ue_sync.cfo                       = srslte_ue_sync_get_carrier_freq_offset(&ue_sync->sfind); // CFO value given in Hz
      short_ue_sync.nof_subframes_to_rx       = ue_sync->sfind.nof_subframes_to_rx;
      short_ue_sync.subframe_counter          = ue_sync->subframe_counter;
      short_ue_sync.subframe_track_start      = ue_sync->subframe_track_start;
      short_ue_sync.mcs                       = ue_sync->sfind.mcs;

      //printf("-----------------> short_ue_sync.nof_subframes_to_rx: %d - short_ue_sync.subframe_counter: %d\n", short_ue_sync.nof_subframes_to_rx, short_ue_sync.subframe_counter);

#if(WRITE_RX_SUBFRAME_INTO_FILE==1)
      static unsigned int dump_cnt = 0;
      char output_file_name[200] = "f_ofdm_rx_side_assessment_5MHz.dat";
      srslte_filesink_t file_sink;
      if(dump_cnt==0) {
         filesink_init(&file_sink, output_file_name, SRSLTE_COMPLEX_FLOAT_BIN);
         // Write samples into file.
         filesink_write(&file_sink, &ue_sync->input_buffer[short_ue_sync.buffer_number][short_ue_sync.subframe_start_index], SRSLTE_SF_LEN(srslte_symbol_sz(helpers_get_prb_from_bw_index(get_bw_index(vphy_rx_ctx)))));
         // Close file.
         filesink_free(&file_sink);
      }
      dump_cnt++;
      PHY_RX_PRINT("vPHY Rx ID: %d - File dumped: %d.\n", vphy_rx_ctx->vphy_id, dump_cnt);
#endif

      // Push ue sync strucute to queue (FIFO).
      vphy_rx_push_ue_sync_to_queue(vphy_rx_ctx, &short_ue_sync);

      // After pushing the ue synch message into the queue, reset ue_synch object if this was the last subframe of a MAC frame.
      if(ue_sync->subframe_counter >= ue_sync->sfind.nof_subframes_to_rx) {
        // Light weight way to reset ue_dl for new reception.
        if(srslte_ue_sync_init_reentry_loop(ue_sync)) {
          PHY_RX_ERROR("vPHY Rx ID: %d - Error re-initiating ue_sync\n", vphy_rx_ctx->vphy_id);
          continue;
        }
        // Reset subframe counter so that it can be used again.
        ue_sync->subframe_counter = 0;
      }

      //double diff_queue = helpers_profiling_diff_time(start_push_queue);
      //if(diff_queue > 0.05)
      //  PHY_RX_PRINT("UE Synch enQUEUE time: %f\n",diff_queue);

      //phy_reception_print_ue_sync(&short_ue_sync,"********** synchronization thread **********\n");
    } else if(ret == 0) {
      // No slot synchronized and aligned. We don't do nothing for now.
    } else if(ret < 0) {
      PHY_RX_ERROR("vPHY Rx ID: %d - Error calling srslte_ue_sync_get_buffer_new()\n", vphy_rx_ctx->vphy_id);
      continue;
    }

    // Disarm watchdog timer for synchronization thread.
#if(ENABLE_WATCHDOG_TIMER==1)
    if(timer_disarm(&vphy_rx_ctx->vphy_rx_sync_timer_id) < 0) {
      PHY_RX_ERROR("vPHY Rx ID: %d - Not possible to disarm the watchdog timer for sync thread.\n", vphy_rx_ctx->vphy_id);
    }
#endif

  }

  PHY_RX_DEBUG("vPHY Rx ID: %d - Leaving PHY synchronization thread.\n", vphy_rx_ctx->vphy_id);
  // Exit thread with result code.
  pthread_exit(NULL);
}

int vphy_rx_recv(void *h, void *samples, uint32_t nof_samples) {
  return hypervisor_rx_recv(h, samples, nof_samples);
}

void vphy_rx_print_ue_sync(short_ue_sync_t* const short_ue_sync, char* const str) {
  printf("%s",str);
  printf("********** UE Sync Structure **********\n");
  printf("Buffer number: %d\n",short_ue_sync->buffer_number);
  printf("Subframe start index: %d\n",short_ue_sync->subframe_start_index);
  printf("Subframe index: %d\n",short_ue_sync->sf_idx);
  printf("Peak value: %0.2f\n",short_ue_sync->peak_value);
  printf("Timestamp: %" PRIu64 "\n" ,short_ue_sync->peak_detection_timestamp);
  printf("CFO: %0.3f\n",short_ue_sync->cfo/1000.0);
  printf("Subframe length: %d\n",short_ue_sync->frame_len);
}

timer_t* vphy_rx_get_sync_timer_id(uint32_t vphy_id) {
  return &vphy_rx_threads[vphy_id]->vphy_rx_sync_timer_id;
}
