#include "hypervisor_tx.h"

// *********** Global variables ***********
static hypervisor_tx_t* hypervisor_tx_handle = NULL;

// *********** Definition of functions ***********
int hypervisor_tx_initialize(transceiver_args_t* const args, srslte_rf_t* const rf) {
  // Allocate memory for a hypervisor Tx object;
  hypervisor_tx_handle = (hypervisor_tx_t*)srslte_vec_malloc(sizeof(hypervisor_tx_t));
  if(hypervisor_tx_handle == NULL) {
    HYPER_TX_ERROR("Error when allocating memory for hypervisor_tx_handle\n",0);
    return -1;
  }
  // Initialize structure with zeros.
  bzero(hypervisor_tx_handle, sizeof(hypervisor_tx_t));
  // Instatiate circular buffer for phy control structures.
  phy_control_cb_make(&hypervisor_tx_handle->phy_ctrl_handle, NOF_PHY_CTRL_IN_CB);
  // Initialize hypervisor Tx object.
  int ret = hypervisor_tx_init_handle(args, rf);
  if(ret < 0) {
    HYPER_TX_ERROR("Error initializing Hyper Tx handle.\n",0);
    return -1;
  }
  // Set TX sample rate for TX RADIO according to the number of PRBs for the radio transmissions.
  // Sample rate must be the one used to transmit several vPHYS concurrently.
  hypervisor_tx_set_sample_rate(args->radio_nof_prb, args->use_std_carrier_sep);
  // Configure initial Radio Tx central frequency and gain. Central frequency is set to the competition center frequency.
  hypervisor_tx_set_radio_center_freq_and_gain(args);
  // Initilize cell struture with values.
  hypervisor_tx_init_cell_parameters(args);
  // Initialize hypervisor control structure for last configured parameters.
  hypervisor_tx_init_last_hypervisor_ctrl(args);
  // Allocate memory for transmission buffers.
  hypervisor_tx_init_buffers(args);
  // Create ifft object.
  if(srslte_ofdm_tx_init(&hypervisor_tx_handle->ifft, hypervisor_tx_handle->phy_tx_cell_info.cp, hypervisor_tx_handle->phy_tx_cell_info.nof_prb)) {
    HYPER_TX_ERROR("Error creating IFFT object.\n",0);
    return -1;
  }
  // Enable IFFT normalization.
  srslte_ofdm_set_normalize(&hypervisor_tx_handle->ifft, true);
  // Initialize mutex.
  if(pthread_mutex_init(&hypervisor_tx_handle->hypervisor_tx_mutex, NULL) != 0) {
    HYPER_TX_ERROR("Mutex for Hypervisor Tx access init failed.\n",0);
    return -1;
  }
  // Initialize conditional variable.
  if(pthread_cond_init(&hypervisor_tx_handle->hypervisor_tx_cv, NULL)) {
    HYPER_TX_ERROR("Conditional variable init failed.\n",0);
    return -1;
  }
  // Initialize mutex.
  if(pthread_mutex_init(&hypervisor_tx_handle->vphy_modulation_done_mutex, NULL) != 0) {
    HYPER_TX_ERROR("Init of mutex for number of active vPHY array access failed.\n",0);
    return -1;
  }
  // Initialize conditional variable.
  if(pthread_cond_init(&hypervisor_tx_handle->vphy_modulation_done_cv, NULL)) {
    HYPER_TX_ERROR("Init of conditional variable for number of active vPHY array failed.\n",0);
    return -1;
  }
  // Create a watchdog timer for hypervisor Tx.
#if(ENABLE_WATCHDOG_TIMER==1)
  if(timer_init(&hypervisor_tx_handle->hyper_tx_timer_id) < 0) {
    HYPER_TX_ERROR("Not possible to create a timer for Hypervisor Tx.\n", 0);
    return -1;
  }
#endif
  // Create frequency-shift object and populate signal array.
#if(ENABLE_FREQ_SHIFT_CORRECTION==1)
  // Instantiate complex waveform object.
  srslte_cexptab_init_finer(&hypervisor_tx_handle->freq_shift_waveform_obj, srslte_symbol_sz(hypervisor_tx_handle->phy_tx_cell_info.nof_prb));
  // Allocate memory for frequency-shift waveform signal.
  hypervisor_tx_handle->freq_shift_waveform = (cf_t*)srslte_vec_malloc((hypervisor_tx_handle->sf_n_samples+hypervisor_tx_handle->number_of_tx_offset_samples+NOF_PADDING_ZEROS)*sizeof(cf_t));
  if(hypervisor_tx_handle->freq_shift_waveform == NULL) {
    HYPER_TX_ERROR("Error when allocating memory for frequency shift signal array.\n", 0);
    exit(-1);
  }
  // Generate frequency-shift waveform signal array.
  float freq = -((float)srslte_symbol_sz(hypervisor_tx_handle->last_tx_hypervisor_ctrl.vphy_nof_prb)/2)/srslte_symbol_sz(hypervisor_tx_handle->phy_tx_cell_info.nof_prb);
  srslte_cexptab_gen_finer(&hypervisor_tx_handle->freq_shift_waveform_obj, hypervisor_tx_handle->freq_shift_waveform, freq, (hypervisor_tx_handle->sf_n_samples+hypervisor_tx_handle->number_of_tx_offset_samples+NOF_PADDING_ZEROS));
  //printf("freq: %1.4f - subframe_buffer_offset: %d - nof samples: %d\n", freq, FIX_TX_OFFSET_SAMPLES, (hypervisor_tx_handle->sf_n_samples+hypervisor_tx_handle->number_of_tx_offset_samples+NOF_PADDING_ZEROS));
#endif

  return 0;
}

int hypervisor_tx_uninitialize() {
  // Destroy mutex.
  pthread_mutex_destroy(&hypervisor_tx_handle->hypervisor_tx_mutex);
  // Destory conditional variable.
  if(pthread_cond_destroy(&hypervisor_tx_handle->hypervisor_tx_cv) != 0) {
    HYPER_TX_ERROR("Conditional variable destruction failed.\n",0);
    return -1;
  }
  // Destroy mutex.
  pthread_mutex_destroy(&hypervisor_tx_handle->vphy_modulation_done_mutex);
  // Destory conditional variable.
  if(pthread_cond_destroy(&hypervisor_tx_handle->vphy_modulation_done_cv) != 0) {
    HYPER_TX_ERROR("Destruction of conditional variable for number of active vPHY array failed.\n",0);
    return -1;
  }
  // Free IFFT object.
  srslte_ofdm_tx_free(&hypervisor_tx_handle->ifft);
  // Free memory used for transmission buffers.
  hypervisor_tx_free_buffers();
  HYPER_TX_INFO("hypervisor_tx_free_buffers done!\n",0);
  // Notify all waiting threads before freeing semaphores.
  hypervisor_tx_notify_all_threads_waiting_for_semaphores();
  // Free Semaphore FIFO.
  vphy_tx_semaphore_cb_free(&hypervisor_tx_handle->subframe_mod_done_sem_fifo);
  // Free Semaphore FIFO.
  vphy_tx_semaphore_cb_free(&hypervisor_tx_handle->frame_mod_done_sem_fifo);
  // Free circular buffer holding phy control structures.
  phy_control_cb_free(&hypervisor_tx_handle->phy_ctrl_handle);
  // Free frequency shift signal array.
#if(ENABLE_FREQ_SHIFT_CORRECTION==1)
  free(hypervisor_tx_handle->freq_shift_waveform);
#endif
  // Free memory used to store hypervisor Tx object.
  if(hypervisor_tx_handle) {
    free(hypervisor_tx_handle);
    hypervisor_tx_handle = NULL;
  }
  return 0;
}

// Initialize struture with Cell parameters.
void hypervisor_tx_init_cell_parameters(transceiver_args_t* const args) {
  hypervisor_tx_handle->phy_tx_cell_info.nof_prb          = args->radio_nof_prb;    // radio_nof_prb
  hypervisor_tx_handle->phy_tx_cell_info.nof_ports        = args->nof_ports;        // nof_ports
  hypervisor_tx_handle->phy_tx_cell_info.bw_idx           = 0;                      // bw idx
  hypervisor_tx_handle->phy_tx_cell_info.id               = args->radio_id;         // cell_id
  hypervisor_tx_handle->phy_tx_cell_info.cp               = args->phy_cylic_prefix; // cyclic prefix
  hypervisor_tx_handle->phy_tx_cell_info.phich_length     = SRSLTE_PHICH_NORM;      // PHICH length
  hypervisor_tx_handle->phy_tx_cell_info.phich_resources  = SRSLTE_PHICH_R_1;       // PHICH resources
}

// Initialize structure with last configured Tx hypervisor control.
void hypervisor_tx_init_last_hypervisor_ctrl(transceiver_args_t* const args) {
  hypervisor_tx_handle->last_tx_hypervisor_ctrl.radio_center_frequency  = args->radio_center_frequency;
  hypervisor_tx_handle->last_tx_hypervisor_ctrl.gain                    = args->initial_tx_gain;
  hypervisor_tx_handle->last_tx_hypervisor_ctrl.radio_nof_prb           = args->radio_nof_prb;
  hypervisor_tx_handle->last_tx_hypervisor_ctrl.vphy_nof_prb            = args->nof_prb;
  hypervisor_tx_handle->last_tx_hypervisor_ctrl.rf_boost                = args->rf_boost;
}

int hypervisor_tx_init_handle(transceiver_args_t* const args, srslte_rf_t* const rf) {
  hypervisor_tx_handle->hypervisor_tx_rf            = rf; // Create a local pointer to the RF (i.e., USRP) object.
  hypervisor_tx_handle->output_buffer               = NULL;
  hypervisor_tx_handle->number_of_re_in_sf          = 0;
  hypervisor_tx_handle->sf_n_samples                = 0;
  hypervisor_tx_handle->number_of_tx_offset_samples = 0;
  hypervisor_tx_handle->run_hypervisor_tx_thread    = true;
  hypervisor_tx_handle->add_preamble_to_front       = args->add_preamble_to_front;
  hypervisor_tx_handle->is_lbt_enabled              = args->lbt_threshold < 100.0?true:false;
  hypervisor_tx_handle->use_std_carrier_sep         = args->use_std_carrier_sep;
  hypervisor_tx_handle->enable_ifft_adjust          = args->enable_ifft_adjust;
  hypervisor_tx_handle->modulate_with_zeros         = args->modulate_with_zeros;
  hypervisor_tx_handle->mod_with_zeros_cnt          = 0;
  hypervisor_tx_handle->num_of_tx_vphys             = args->num_of_tx_vphys;
  // Set all subframe counter to 0.
  bzero(hypervisor_tx_handle->subframe_counter, sizeof(uint32_t)*MAX_NUMBER_OF_TBS_IN_A_SLOT);
  // Instantiate semaphore fifo.
  vphy_tx_semaphore_cb_make(&hypervisor_tx_handle->subframe_mod_done_sem_fifo, MAX_NUM_CONCURRENT_VPHYS);
  // Instantiate semaphore fifo.
  vphy_tx_semaphore_cb_make(&hypervisor_tx_handle->frame_mod_done_sem_fifo, MAX_NUM_CONCURRENT_VPHYS);

  return 0;
}

int hypervisor_tx_init_buffers(transceiver_args_t* const args) {
  // Decide number of OFDM symbols in one slot.
  uint32_t number_of_symbols_in_slot = 0;
  if(args->phy_cylic_prefix == SRSLTE_CP_NORM) {
    number_of_symbols_in_slot = SRSLTE_CP_NORM_NSYMB;
  } else {
    number_of_symbols_in_slot = SRSLTE_CP_EXT_NSYMB;
  }
  // Calculate number of samples based on flag indicating is preamble is added to the front ot middle of the subframe.
  // If added to the front of the slot is 1 ms + 2*(0.5 ms/7) long for the normal CP case.
  if(args->add_preamble_to_front) {
    // Calculate number of resource elements (RE) in the frequency-domain subframe resource grid buffer.
    // Two more symbols are added in order to accomodate PSS and SSS signals.
    // PREAMBLE_SLOT_SIZE_IN_SYMBOLS is defined in ofdm.h.
    hypervisor_tx_handle->number_of_re_in_sf = (2*number_of_symbols_in_slot + PREAMBLE_SLOT_SIZE_IN_SYMBOLS)*srslte_symbol_sz(args->radio_nof_prb);
    uint32_t cp_length = 0;
    if(args->phy_cylic_prefix == SRSLTE_CP_NORM) {
      cp_length = SRSLTE_CP_NORM_LEN;
    } else {
      cp_length = SRSLTE_CP_EXT_LEN;
    }
    // Calculate number of IQ samples in the time-domain subframe buffer.
    hypervisor_tx_handle->sf_n_samples = 2*SRSLTE_SLOT_LEN(srslte_symbol_sz(args->radio_nof_prb)) + PREAMBLE_SLOT_SIZE_IN_SYMBOLS*(srslte_symbol_sz(args->radio_nof_prb) + cp_length);
  } else {
    // Calculate number of resource elements (RE) in the frequency-domain subframe resource grid buffer.
    hypervisor_tx_handle->number_of_re_in_sf = 2*number_of_symbols_in_slot*srslte_symbol_sz(args->radio_nof_prb);
    // Calculate number of IQ samples in the time-domain subframe buffer.
    hypervisor_tx_handle->sf_n_samples = 2*SRSLTE_SLOT_LEN(srslte_symbol_sz(args->radio_nof_prb));
  }

  // Retrieve device name.
  const char *devname = srslte_rf_name(hypervisor_tx_handle->hypervisor_tx_rf);
  HYPER_TX_DEBUG("Device name: %s\n",devname);

  // Decide the number of samples in a subframe.
  uint32_t number_of_subframe_samples = hypervisor_tx_handle->sf_n_samples;
  set_number_of_tx_offset_samples(0);
  if(strcmp(devname,DEVNAME_X300) == 0 && FIX_TX_OFFSET_SAMPLES > 0) {
    number_of_subframe_samples = hypervisor_tx_handle->sf_n_samples + FIX_TX_OFFSET_SAMPLES;
    set_number_of_tx_offset_samples(FIX_TX_OFFSET_SAMPLES);
    HYPER_TX_DEBUG("HW: %s and Tx Offset: %d - zero padding: %d\n",devname,FIX_TX_OFFSET_SAMPLES,NOF_PADDING_ZEROS);
  }

  // Increase the number of samples so that there is room for padding zeros.
  if(NOF_PADDING_ZEROS > 0) {
    number_of_subframe_samples = number_of_subframe_samples + NOF_PADDING_ZEROS;
  }

  // Set hypervisor tx context with the number of samples in a subframe.
  hypervisor_tx_handle->number_of_subframe_samples = number_of_subframe_samples;

  // Initialize output buffer memory.
  if(hypervisor_tx_handle->output_buffer == NULL) {
    hypervisor_tx_handle->output_buffer = (cf_t*)srslte_vec_malloc(sizeof(cf_t)*number_of_subframe_samples);
    if(!hypervisor_tx_handle->output_buffer) {
      HYPER_TX_ERROR("Error allocating memory to output_buffer",0);
      return -1;
    }
    HYPER_TX_DEBUG("Size of output_buffer is %d bytes\n",(sizeof(cf_t)*number_of_subframe_samples));
    // Set allocated memory to 0.
    // TODO: when online bandwidth change is implemented this setting of memory to 0 will have an impact on the time to change the bandwidth. That should be taken into account.
    bzero(hypervisor_tx_handle->output_buffer, (sizeof(cf_t)*number_of_subframe_samples));
  } else {
    HYPER_TX_ERROR("output_buffer not NULL at start!!!",0);
    return -1;
  }

  // Initialize subframe buffer memory.
  for(uint32_t i = 0; i < MAX_NUMBER_OF_TBS_IN_A_SLOT; i++) {
    hypervisor_tx_handle->subframe_buffer[i] = (cf_t*)srslte_vec_malloc(sizeof(cf_t)*(hypervisor_tx_handle->number_of_re_in_sf));
    if(!hypervisor_tx_handle->subframe_buffer[i]) {
      HYPER_TX_ERROR("Error allocating memory to subframe_buffer index: %d", i);
      return -1;
    }
    bzero(hypervisor_tx_handle->subframe_buffer[i], sizeof(cf_t)*(hypervisor_tx_handle->number_of_re_in_sf));
    HYPER_TX_DEBUG("Size of subframe_buffer index: %d is equal to %d samples and %d bytes\n", i, hypervisor_tx_handle->number_of_re_in_sf, (sizeof(cf_t)*(hypervisor_tx_handle->number_of_re_in_sf)));
  }

  return 0;
}

void hypervisor_tx_free_buffers() {
  if(hypervisor_tx_handle->output_buffer) {
    free(hypervisor_tx_handle->output_buffer);
    hypervisor_tx_handle->output_buffer = NULL;
  }
  for(uint32_t i = 0; i < MAX_NUMBER_OF_TBS_IN_A_SLOT; i++) {
    if(hypervisor_tx_handle->subframe_buffer[i]) {
      free(hypervisor_tx_handle->subframe_buffer[i]);
      hypervisor_tx_handle->subframe_buffer[i] = NULL;
    }
  }
}

void hypervisor_tx_register_frame_mod_done_sem(sem_t* const semaphore) {
  // Lock mutex.
  pthread_mutex_lock(&hypervisor_tx_handle->vphy_modulation_done_mutex);
  // In case this is not the last vPHY to finish, then, its semaphore must be kept so that the hypervisor can inform it when to go on.
  vphy_tx_semaphore_cb_push_back(hypervisor_tx_handle->frame_mod_done_sem_fifo, semaphore);
  // Unlock mutex.
  pthread_mutex_unlock(&hypervisor_tx_handle->vphy_modulation_done_mutex);
}

// Resource grid vector
// inform resource grid number -> resource_grid_idx
// 1 subframe_counter per resource resource grid
bool hypervisor_tx_inform_subframe_modulation_done(slot_ctrl_t* const slot_ctrl, uint32_t subframe_cnt) {
  bool ret = false;
  // Lock mutex.
  pthread_mutex_lock(&hypervisor_tx_handle->vphy_modulation_done_mutex);
  hypervisor_tx_handle->subframe_counter[subframe_cnt]++;
  if(hypervisor_tx_handle->subframe_counter[subframe_cnt] == slot_ctrl->slot_info.nof_active_vphys_per_slot[subframe_cnt]) {
    // Zero the TB counter so that it can start counting for the next 1 ms long slot.
    hypervisor_tx_handle->subframe_counter[subframe_cnt] = 0;
    ret = true;
  }
  // Unlock mutex.
  pthread_mutex_unlock(&hypervisor_tx_handle->vphy_modulation_done_mutex);
  return ret;
}

// Resource grid vector
// inform resource grid number -> resource_grid_idx
// 1 semaphore QUEUE per resource grid
void hypervisor_tx_transfer_to_usrp_done_notify_vphy_tx_threads() {
  sem_t* semaphore = NULL;
  // Lock mutex.
  pthread_mutex_lock(&hypervisor_tx_handle->vphy_modulation_done_mutex);
  // Retrieve size of QUEUE.
  uint32_t size = vphy_tx_semaphore_cb_size(hypervisor_tx_handle->subframe_mod_done_sem_fifo);
  // Inform all vPHY Tx threads to continue the work.
  for(uint32_t i = 0; i < size; i++) {
    semaphore = vphy_tx_semaphore_cb_front(hypervisor_tx_handle->subframe_mod_done_sem_fifo);
    vphy_tx_semaphore_cb_pop_front(hypervisor_tx_handle->subframe_mod_done_sem_fifo);
    if(semaphore) {
      // Inform all the waiting vPHY Tx threads we processing can continue.
      sem_post(semaphore);
    }
  }
  // Unlock mutex.
  pthread_mutex_unlock(&hypervisor_tx_handle->vphy_modulation_done_mutex);
}

// Inform all the waiting threads that the last slot in a PHY Control message was transmitted and
// that the threads can proceed to the next PHY control message.
void hypervisor_tx_notify_vphy_tx_threads_frame_mod_done(sem_t *frame_done_semaphore) {
  sem_t* semaphore = NULL;
  // Lock mutex.
  pthread_mutex_lock(&hypervisor_tx_handle->vphy_modulation_done_mutex);
  // Inform the earlier vPHY Tx threads that they can proceed to the next PHY control message.
  // Retrieve size of QUEUE.
  uint32_t size = vphy_tx_semaphore_cb_size(hypervisor_tx_handle->frame_mod_done_sem_fifo);
  // Inform all vPHY Tx threads to continue the work.
  for(uint32_t i = 0; i < size; i++) {
    // Get next semaphore from circular buffer.
    semaphore = vphy_tx_semaphore_cb_front(hypervisor_tx_handle->frame_mod_done_sem_fifo);
    // Remove semaphore from circular buffer.
    vphy_tx_semaphore_cb_pop_front(hypervisor_tx_handle->frame_mod_done_sem_fifo);
    if(semaphore && semaphore != frame_done_semaphore) {
      // Inform all the waiting vPHY Tx threads we processing can continue.
      sem_post(semaphore);
    }
  }
  // Unlock mutex.
  pthread_mutex_unlock(&hypervisor_tx_handle->vphy_modulation_done_mutex);
}

// Notify all vPHY Tx threads waiting for subframe modulation done semaphore.
void hypervisor_tx_notify_all_vphy_tx_threads_waiting_subframe_mod_done_semaphore() {
  sem_t* semaphore = NULL;
  // Lock mutex.
  pthread_mutex_lock(&hypervisor_tx_handle->vphy_modulation_done_mutex);
  // Retrieve size of QUEUE.
  uint32_t size = vphy_tx_semaphore_cb_size(hypervisor_tx_handle->subframe_mod_done_sem_fifo);
  if(size > 0) {
    // Inform all vPHY Tx threads to continue the work.
    for(uint32_t i = 0; i < size; i++) {
      semaphore = vphy_tx_semaphore_cb_front(hypervisor_tx_handle->subframe_mod_done_sem_fifo);
      vphy_tx_semaphore_cb_pop_front(hypervisor_tx_handle->subframe_mod_done_sem_fifo);
      if(semaphore) {
        // Inform all the waiting vPHY Tx threads we processing can continue.
        sem_post(semaphore);
      }
    }
  }
  // Unlock mutex.
  pthread_mutex_unlock(&hypervisor_tx_handle->vphy_modulation_done_mutex);
}

// Notify all vPHY Tx threads waiting for slot modulation done semaphore.
void hypervisor_tx_notify_all_vphy_tx_threads_waiting_frame_mod_done_semaphore() {
  sem_t* semaphore = NULL;
  // Lock mutex.
  pthread_mutex_lock(&hypervisor_tx_handle->vphy_modulation_done_mutex);
  // Retrieve size of QUEUE.
  uint32_t size = vphy_tx_semaphore_cb_size(hypervisor_tx_handle->frame_mod_done_sem_fifo);
  if(size > 0) {
    // Inform all vPHY Tx threads to continue the work.
    for(uint32_t i = 0; i < size; i++) {
      semaphore = vphy_tx_semaphore_cb_front(hypervisor_tx_handle->frame_mod_done_sem_fifo);
      vphy_tx_semaphore_cb_pop_front(hypervisor_tx_handle->frame_mod_done_sem_fifo);
      if(semaphore) {
        // Inform all the waiting vPHY Tx threads we processing can continue.
        sem_post(semaphore);
      }
    }
  }
  // Unlock mutex.
  pthread_mutex_unlock(&hypervisor_tx_handle->vphy_modulation_done_mutex);
}

// Notify all the waiting threads.
void hypervisor_tx_notify_all_threads_waiting_for_semaphores() {
  hypervisor_tx_notify_all_vphy_tx_threads_waiting_subframe_mod_done_semaphore();
  hypervisor_tx_notify_all_vphy_tx_threads_waiting_frame_mod_done_semaphore();
}

// Resource grid vector
// inform resource grid number -> resource_grid_idx
// Mutex necessary to block when one vPHY Tx finishes processing the current slot message and reads the next one.
void hypervisor_tx_transmit(slot_ctrl_t* const slot_ctrl, uint32_t subframe_cnt) {

  HYPER_TX_INFO("vPHY Tx ID %d - Executing hypervisor_tx_transmit()\n", slot_ctrl->vphy_id);

  time_t full_secs = 0;
  double frac_secs = 0.0;
  bool start_of_burst = false, end_of_burst = false, has_time_spec = false;
  uint32_t nof_used_rbs = 0, number_of_additional_samples = 0, subframe_buffer_offset = FIX_TX_OFFSET_SAMPLES, nof_zero_padding_samples = 0;
  float norm_factor, ifft_adjust;
#if(ENABLE_SW_RF_MONITOR==1)
  lbt_stats_t lbt_stats;
#endif

  // Set watchdog timer for Hypervisor Tx. Always wait for some seconds.
#if(ENABLE_WATCHDOG_TIMER==1)
  if(timer_set(&hypervisor_tx_handle->hyper_tx_timer_id, 2) < 0) {
    HYPER_TX_ERROR("Not possible to set the watchdog timer for Hypervisor Tx.\n", 0);
  }
#endif

  // Check if this is the first subframe, then, set flag of SOB to true.
  if(subframe_cnt == 0) {
    start_of_burst = true;
  }

  // Check if this is the last subframe, then, set flag of EOB to true.
  if(subframe_cnt == (slot_ctrl->slot_info.largest_nof_tbs_in_slot-1)) {
    end_of_burst = true;
    // If this is the last subframe of a frame then we add some zeros at the end of this subframe.
    nof_zero_padding_samples = NOF_PADDING_ZEROS;
  }

  // Check if it is the fisrt subframe and if there is timestamp specificication.
  if(start_of_burst && slot_ctrl->timestamp != 0) {
    has_time_spec = true;
    helpers_convert_host_timestamp_into_uhd_timestamp_us(slot_ctrl->timestamp, &full_secs, &frac_secs);
    HYPER_TX_DEBUG("Time difference: %d\n",(int)(slot_ctrl->timestamp-helpers_get_fpga_timestamp_us(hypervisor_tx_handle->hypervisor_tx_rf)));
  }

  // Check if it is necessary to add zeros before the subframe.
  if(start_of_burst) {
    number_of_additional_samples = get_number_of_tx_offset_samples();
    subframe_buffer_offset = 0;
    HYPER_TX_DEBUG("Adding %d zero samples before start of subframe.\n",number_of_additional_samples);
  }

  //bzero(hypervisor_tx_handle->output_buffer, sizeof(cf_t)*hypervisor_tx_handle->number_of_subframe_samples);

#if(WRITE_RESOURCE_GRID_INTO_FILE==1)
  static unsigned int dump_cnt = 0;
  char output_file_name[200];
  sprintf(output_file_name, "resource_grid_%d.dat", dump_cnt);
  srslte_filesink_t file_sink;
  if(dump_cnt < 10) {
   filesink_init(&file_sink, output_file_name, SRSLTE_COMPLEX_FLOAT_BIN);
   // Write samples into file.
   filesink_write(&file_sink, hypervisor_tx_handle->subframe_buffer[subframe_cnt], hypervisor_tx_handle->number_of_re_in_sf);
   // Close file.
   filesink_free(&file_sink);
   HYPER_TX_PRINT("File dumped.\n",0);
  }
  dump_cnt++;
#endif

  // This is a debug option!! Should never be used for other purposes.
#if(ENABLE_TEST_SIGNALS==1)
  // Set all subcarriers to zero during some predefined interval.
  if(hypervisor_tx_handle->modulate_with_zeros == 1) {
    if(hypervisor_tx_handle->mod_with_zeros_cnt < (slot_ctrl->slot_info.largest_nof_tbs_in_slot*3)) {
      bzero(hypervisor_tx_handle->subframe_buffer[subframe_cnt], sizeof(cf_t)*(hypervisor_tx_handle->number_of_re_in_sf));
      hypervisor_tx_handle->mod_with_zeros_cnt++;
    } else {
      if(hypervisor_tx_handle->mod_with_zeros_cnt >= ((slot_ctrl->slot_info.largest_nof_tbs_in_slot*3)+slot_ctrl->slot_info.largest_nof_tbs_in_slot-1)) {
        hypervisor_tx_handle->mod_with_zeros_cnt = 0;
      } else {
        hypervisor_tx_handle->mod_with_zeros_cnt++;
      }
    }
    //helpers_print_resource_grid_gt_zero(hypervisor_tx_handle->subframe_buffer[subframe_cnt], hypervisor_tx_handle->number_of_re_in_sf);
  }

  // Null the subcarriers and transmit some random signal at the specified position.
  if(hypervisor_tx_handle->modulate_with_zeros == 2) {
    uint32_t lower_idx_start = 0;
    //helpers_print_complex_vector(hypervisor_tx_handle->subframe_buffer[subframe_cnt], hypervisor_tx_handle->number_of_re_in_sf);
    bzero(hypervisor_tx_handle->subframe_buffer[subframe_cnt], sizeof(cf_t)*(hypervisor_tx_handle->number_of_re_in_sf));
    for(uint32_t symb_cnt = 0; symb_cnt < 14; symb_cnt++) {
      for(uint32_t subcarrier_cnt = 0; subcarrier_cnt < 72; subcarrier_cnt++) {
        cf_t value = (1.0/sqrtf(2.0)) - (1.0/sqrtf(2.0))*I;
        hypervisor_tx_handle->subframe_buffer[subframe_cnt][lower_idx_start + 1200*symb_cnt + subcarrier_cnt] = value;
      }
    }
    //helpers_print_complex_vector(hypervisor_tx_handle->subframe_buffer[subframe_cnt], hypervisor_tx_handle->number_of_re_in_sf);
  }

  if(hypervisor_tx_handle->modulate_with_zeros == 3) {
    uint32_t lower_idx_start = 0;//564;
    cf_t value;
    //helpers_print_complex_vector(hypervisor_tx_handle->subframe_buffer[subframe_cnt], hypervisor_tx_handle->number_of_re_in_sf);
    bzero(hypervisor_tx_handle->subframe_buffer[subframe_cnt], sizeof(cf_t)*(hypervisor_tx_handle->number_of_re_in_sf));
    // Generate signal.
    if(hypervisor_tx_handle->mod_with_zeros_cnt < (slot_ctrl->slot_info.largest_nof_tbs_in_slot*3)) {
      hypervisor_tx_handle->mod_with_zeros_cnt++;
    } else {
      for(uint32_t symb_cnt = 0; symb_cnt < 14; symb_cnt++) {
        for(uint32_t subcarrier_cnt = 0; subcarrier_cnt <= 72; subcarrier_cnt++) {
          if(subcarrier_cnt==36) {
            value = 0.0 + 0.0*I;
          } else {
            value = (1.0/sqrtf(2.0)) - (1.0/sqrtf(2.0))*I;
          }
          //cf_t value = helpers_random_modulator();
          hypervisor_tx_handle->subframe_buffer[subframe_cnt][lower_idx_start + 1200*symb_cnt + subcarrier_cnt] = value;
        }
      }
      if(hypervisor_tx_handle->mod_with_zeros_cnt >= ((slot_ctrl->slot_info.largest_nof_tbs_in_slot*3)+slot_ctrl->slot_info.largest_nof_tbs_in_slot-1)) {
        hypervisor_tx_handle->mod_with_zeros_cnt = 0;
      } else {
        hypervisor_tx_handle->mod_with_zeros_cnt++;
      }
    }
    //helpers_print_complex_vector(hypervisor_tx_handle->subframe_buffer[subframe_cnt], hypervisor_tx_handle->number_of_re_in_sf);
  }
#endif // ENABLE_TEST_SIGNALS

  // Transform modulation symbols into OFDM symbols.
  if(!hypervisor_tx_handle->add_preamble_to_front) {
    srslte_ofdm_tx_sf_no_gb(&hypervisor_tx_handle->ifft, hypervisor_tx_handle->subframe_buffer[subframe_cnt], hypervisor_tx_handle->output_buffer+FIX_TX_OFFSET_SAMPLES);
  } else {
    srslte_ofdm_tx_longer_sf_no_gb(&hypervisor_tx_handle->ifft, hypervisor_tx_handle->subframe_buffer[subframe_cnt], hypervisor_tx_handle->output_buffer+FIX_TX_OFFSET_SAMPLES);
  }

  // Apply configurable gain/attenuation to IFFT signal if flag is enabled.
  if(hypervisor_tx_handle->enable_ifft_adjust) {
    // Apply normalization constant to subframe.
    nof_used_rbs = hypervisor_tx_handle->last_tx_hypervisor_ctrl.vphy_nof_prb*slot_ctrl->slot_info.nof_active_vphys_per_slot[subframe_cnt];
    if(nof_used_rbs == 0) {
      HYPER_TX_ERROR("Number of active vPHYs should be greater than 0!!!!!!!\n",0);
      exit(-1);
    }
    //norm_factor = (float)hypervisor_tx_handle->phy_tx_cell_info.nof_prb/15/sqrtf(nof_used_rbs);
    //ifft_adjust = (hypervisor_tx_handle->last_tx_hypervisor_ctrl.rf_boost*norm_factor);
    norm_factor = (float)hypervisor_tx_handle->phy_tx_cell_info.nof_prb/15/sqrtf(100);
    ifft_adjust =  0.2667; //0.8*norm_factor;//(hypervisor_tx_handle->last_tx_hypervisor_ctrl.rf_boost*norm_factor); //;
    //printf("ifft_adjust: %f - phy_tx_cell_info.nof_prb: %d - norm_factor: %f\n",ifft_adjust, hypervisor_tx_handle->phy_tx_cell_info.nof_prb,norm_factor);
    srslte_vec_sc_prod_cfc(hypervisor_tx_handle->output_buffer+FIX_TX_OFFSET_SAMPLES, ifft_adjust, hypervisor_tx_handle->output_buffer+FIX_TX_OFFSET_SAMPLES, SRSLTE_SF_LEN_PRB(hypervisor_tx_handle->phy_tx_cell_info.nof_prb));
    HYPER_TX_DEBUG("norm_factor: %f - ifft_adjust: %f\n", norm_factor, ifft_adjust);
  }
  // Apply frequency shift to received signal.
#if(ENABLE_FREQ_SHIFT_CORRECTION==1)
  srslte_vec_prod_ccc(hypervisor_tx_handle->freq_shift_waveform, (hypervisor_tx_handle->output_buffer+subframe_buffer_offset), (hypervisor_tx_handle->output_buffer+subframe_buffer_offset), (hypervisor_tx_handle->sf_n_samples+number_of_additional_samples+nof_zero_padding_samples));
#endif

  // Timestamp start of transmission procedure.
  //struct timespec transfer_timespec;
  //clock_gettime(CLOCK_REALTIME, &transfer_timespec);

  // Transfer subframe to USRP.
#if(ENABLE_SW_RF_MONITOR==1)
  hypervisor_tx_send(hypervisor_tx_handle, (hypervisor_tx_handle->output_buffer+subframe_buffer_offset), (hypervisor_tx_handle->sf_n_samples+number_of_additional_samples+nof_zero_padding_samples), full_secs, frac_secs, has_time_spec, true, start_of_burst, end_of_burst, hypervisor_tx_handle->is_lbt_enabled, (void*)&lbt_stats, PHY_CHANNEL);
#else
  hypervisor_tx_send(hypervisor_tx_handle, (hypervisor_tx_handle->output_buffer+subframe_buffer_offset), (hypervisor_tx_handle->sf_n_samples+number_of_additional_samples+nof_zero_padding_samples), full_secs, frac_secs, has_time_spec, true, start_of_burst, end_of_burst, hypervisor_tx_handle->is_lbt_enabled, NULL, PHY_CHANNEL);
#endif

  //HYPER_TX_PRINT("nof samples: %d - is_lbt_enabled: %d - start_of_burst: %d - Transfer time: %f\n",(hypervisor_tx_handle->sf_n_samples+number_of_additional_samples),hypervisor_tx_handle->is_lbt_enabled,start_of_burst,helpers_profiling_diff_time(transfer_timespec));

  //helpers_print_subframe((hypervisor_tx_handle->output_buffer+subframe_buffer_offset), (hypervisor_tx_handle->sf_n_samples+number_of_additional_samples+nof_zero_padding_samples), start_of_burst, end_of_burst);

#if(WRITE_TX_SUBFRAME_INTO_FILE==1)
  static unsigned int dump_cnt = 0;
  char output_file_name[200];
  sprintf(output_file_name, "tx_side_assessment_20MHz_%d.dat",dump_cnt);
  srslte_filesink_t file_sink;
  if(dump_cnt < 10) {
    filesink_init(&file_sink, output_file_name, SRSLTE_COMPLEX_FLOAT_BIN);
    // Write samples into file.
    filesink_write(&file_sink, hypervisor_tx_handle->output_buffer+FIX_TX_OFFSET_SAMPLES, hypervisor_tx_handle->sf_n_samples);
    // Close file.
    filesink_free(&file_sink);
    dump_cnt++;
    HYPER_TX_PRINT("File dumped: %d.\n",dump_cnt);
    //helpers_print_resource_grid(hypervisor_tx_handle->subframe_buffer[subframe_cnt], hypervisor_tx_handle->number_of_re_in_sf, 100*12);
  }
#endif

  // After transmitting, the subframe buffer (i.e., frequency domain resource grid) must be cleaned for the next transmission.
  bzero(hypervisor_tx_handle->subframe_buffer[subframe_cnt], sizeof(cf_t)*(hypervisor_tx_handle->number_of_re_in_sf));

  // Disarm watchdog timer for Hypervisor Tx.
#if(ENABLE_WATCHDOG_TIMER==1)
  if(timer_disarm(&hypervisor_tx_handle->hyper_tx_timer_id) < 0) {
    HYPER_TX_ERROR("Not possible to disarm the watchdog timer for Hypervisor Tx.\n", 0);
  }
#endif

  HYPER_TX_INFO("vPHY Tx ID %d - Leaving hypervisor_tx_transmit()\n", slot_ctrl->vphy_id);
}

void set_number_of_tx_offset_samples(int num_samples_to_offset) {
  hypervisor_tx_handle->number_of_tx_offset_samples = num_samples_to_offset;
}

inline int get_number_of_tx_offset_samples() {
  return hypervisor_tx_handle->number_of_tx_offset_samples;
}

float hypervisor_tx_set_gain(float tx_gain) {
  float current_tx_gain = srslte_rf_set_tx_gain(hypervisor_tx_handle->hypervisor_tx_rf, tx_gain, PHY_CHANNEL);
  HYPER_TX_INFO("Set Tx gain to: %.1f dB\n", current_tx_gain);
  return current_tx_gain;
}

void hypervisor_tx_set_center_frequency(double tx_center_frequency) {
#if(ENABLE_HW_RF_MONITOR==1)
  // Always apply offset when HW RF Monitor is enabled.
  double lo_offset = (double)PHY_TX_LO_OFFSET;
#else
  // If using default FPGA image, then check number of channels.
  double lo_offset = (hypervisor_tx_handle->hypervisor_tx_rf)->num_of_channels == 1 ? 0.0:(double)PHY_TX_LO_OFFSET;
#endif
  // Set central frequency for transmission.
  double actual_tx_freq = srslte_rf_set_tx_freq2(hypervisor_tx_handle->hypervisor_tx_rf, tx_center_frequency, lo_offset, PHY_CHANNEL);
  // Check if actual frequency is inside a range of +/- 10 Hz.
  if(actual_tx_freq < (tx_center_frequency - 10.0) || actual_tx_freq > (tx_center_frequency + 10.0)) {
     HYPER_TX_ERROR("Requested frequency: %1.2f [MHz] - Actual frequency: %1.2f [MHz]\n", tx_center_frequency/1000000.0, actual_tx_freq/1000000.0);
  }
  HYPER_TX_PRINT("Set Tx central frequency to: %.2f [MHz] with offset of: %.2f [MHz]\n", (actual_tx_freq/1000000.0),(lo_offset/1000000.0));
}

void hypervisor_tx_set_radio_center_freq_and_gain(transceiver_args_t* const args) {
  // Set Tx gain.
  hypervisor_tx_set_gain(args->initial_tx_gain);
  // Set Tx frequency.
  hypervisor_tx_set_center_frequency(args->radio_center_frequency);
}

// This function is only used for channelization. As we transmit over 23.04 MHz bandwidth and center to its center we don't use this function for now.
void hypervisor_tx_set_channel_center_freq_and_gain(float tx_gain, double center_frequency, double radio_sampling_rate, double vphy_sampling_rate, uint32_t tx_channel) {
  // Set default Tx gain.
  float current_tx_gain = hypervisor_tx_set_gain(tx_gain);
  // Calculate default central Tx frequency.
  double tx_channel_center_freq = helpers_calculate_vphy_channel_center_frequency(center_frequency, radio_sampling_rate, vphy_sampling_rate, tx_channel);
#if(ENABLE_HW_RF_MONITOR==1)
  // Always apply offset when HW RF Monitor is enabled.
  double lo_offset = (double)PHY_TX_LO_OFFSET;
#else
  // If using default FPGA image, then check number of channels.
  double lo_offset = (hypervisor_tx_handle->hypervisor_tx_rf)->num_of_channels == 1 ? 0.0:(double)PHY_TX_LO_OFFSET;
#endif
  // Set central frequency for transmission.
  double actual_tx_freq = srslte_rf_set_tx_freq2(hypervisor_tx_handle->hypervisor_tx_rf, tx_channel_center_freq, lo_offset, PHY_CHANNEL);
  // Check if actual frequency is inside a range of +/- 10 Hz.
  if(actual_tx_freq < (tx_channel_center_freq - 10.0) || actual_tx_freq > (tx_channel_center_freq + 10.0)) {
     HYPER_TX_ERROR("Requested frequency: %1.2f [MHz] - Actual frequency: %1.2f [MHz] - Center Frequency: %1.2f [MHz] - Competition BW: %1.2f [MHz] - PHY BW: %1.2f [MHz] - Channel: %d \n", tx_channel_center_freq/1000000.0, actual_tx_freq/1000000.0, center_frequency/1000000.0, radio_sampling_rate/1000000.0, vphy_sampling_rate/1000000.0, tx_channel);
  }
  HYPER_TX_PRINT("Set Tx gain to: %.1f dB\n", current_tx_gain);
  HYPER_TX_PRINT("Set Tx channel central frequency to: %.2f [MHz] with offset of: %.2f [MHz]\n", (actual_tx_freq/1000000.0),(lo_offset/1000000.0));
}

void hypervisor_tx_set_sample_rate(uint32_t radio_nof_prb, bool use_std_carrier_sep) {
  int srate = -1;
  if(use_std_carrier_sep) {
    srate = srslte_sampling_freq_hz(radio_nof_prb);
  } else {
    srate = helpers_non_std_sampling_freq_hz(radio_nof_prb);
    HYPER_TX_PRINT("Setting a non-standard sampling rate: %1.2f [MHz]\n",srate/1000000.0);
  }
  if(srate != -1) {
    float srate_rf = srslte_rf_set_tx_srate(hypervisor_tx_handle->hypervisor_tx_rf, (double)srate, PHY_CHANNEL);
    if(srate_rf != srate) {
      HYPER_TX_ERROR("Could not set Tx sampling rate\n",0);
      exit(-1);
    }
    HYPER_TX_PRINT("Set Tx sampling rate to: %.2f [MHz]\n", srate_rf/1000000.0);
  } else {
    HYPER_TX_ERROR("Invalid number of PRB (Tx): %d\n", radio_nof_prb);
    exit(-1);
  }
}

cf_t** const hypervisor_tx_get_subframe_buffer() {
  return hypervisor_tx_handle->subframe_buffer;
}

void hypervisor_tx_change_parameters(hypervisor_ctrl_t* const hypervisor_ctrl) {
  // Change Tx frequency if the requested one is different from last one.
  if(hypervisor_tx_handle->last_tx_hypervisor_ctrl.radio_center_frequency != hypervisor_ctrl->radio_center_frequency) {
    hypervisor_tx_set_center_frequency(hypervisor_ctrl->radio_center_frequency);
    hypervisor_tx_handle->last_tx_hypervisor_ctrl.radio_center_frequency = hypervisor_ctrl->radio_center_frequency;
  }
  // Change Tx gain if the requested one is different from last one.
  if(hypervisor_tx_handle->last_tx_hypervisor_ctrl.gain != hypervisor_ctrl->gain) {
    hypervisor_tx_set_gain(hypervisor_ctrl->gain);
    hypervisor_tx_handle->last_tx_hypervisor_ctrl.gain = hypervisor_ctrl->gain;
  }
  // Change Tx sampling rate if the requested one is different from last one.
  if(hypervisor_tx_handle->last_tx_hypervisor_ctrl.radio_nof_prb != hypervisor_ctrl->radio_nof_prb) {
    // Sample rate must be the one used to transmit several vPHYS concurrently.
    hypervisor_tx_set_sample_rate(hypervisor_ctrl->radio_nof_prb, hypervisor_tx_handle->use_std_carrier_sep);
    hypervisor_tx_handle->last_tx_hypervisor_ctrl.radio_nof_prb = hypervisor_ctrl->radio_nof_prb;
  }
  // Change vPHY number of PRB if the requested one is different from last one.
  if(hypervisor_tx_handle->last_tx_hypervisor_ctrl.vphy_nof_prb != hypervisor_ctrl->vphy_nof_prb) {
    hypervisor_tx_handle->last_tx_hypervisor_ctrl.vphy_nof_prb = hypervisor_ctrl->vphy_nof_prb;
  }
  // Change RF boost if the requested one is different from last one.
  if(hypervisor_tx_handle->last_tx_hypervisor_ctrl.rf_boost != hypervisor_ctrl->rf_boost) {
    hypervisor_tx_handle->last_tx_hypervisor_ctrl.rf_boost = hypervisor_ctrl->rf_boost;
  }
}

// Wait until container is not empty.
bool hypervisor_tx_wait_container_not_empty() {
  bool ret = true;
  // Lock mutex so that we can wait for all vPHYs to finish modulating the data.
  pthread_mutex_lock(&hypervisor_tx_handle->hypervisor_tx_mutex);
  // Wait for conditional variable only if container is empty.
  if(phy_control_cb_empty(hypervisor_tx_handle->phy_ctrl_handle)) {
    // Wait for conditional variable to be true.
    pthread_cond_wait(&hypervisor_tx_handle->hypervisor_tx_cv, &hypervisor_tx_handle->hypervisor_tx_mutex);
    if(!hypervisor_tx_handle->run_hypervisor_tx_thread) {
      ret = false;
    }
  }
  // Unlock mutex.
  pthread_mutex_unlock(&hypervisor_tx_handle->hypervisor_tx_mutex);
  return ret;
}

// Get PHY Control from container. It must be a blocking function.
void hypervisor_tx_get_phy_control(phy_ctrl_t* const phy_ctrl) {
  // Lock mutex so that we can push slot control to container.
  pthread_mutex_lock(&hypervisor_tx_handle->hypervisor_tx_mutex);
  // Retrieve phy control element from circular buffer.
  phy_control_cb_front(hypervisor_tx_handle->phy_ctrl_handle, phy_ctrl);
  // Remove phy control element from  circular buffer.
  phy_control_cb_pop_front(hypervisor_tx_handle->phy_ctrl_handle);
  // Unlock mutex.
  pthread_mutex_unlock(&hypervisor_tx_handle->hypervisor_tx_mutex);
}

// Push PHY control into container.
void hypervisor_tx_push_phy_control_to_container(phy_ctrl_t* const phy_ctrl) {
  // Lock mutex so that we can push slot control to container.
  pthread_mutex_lock(&hypervisor_tx_handle->hypervisor_tx_mutex);
  // Push phy control into circular buffer.
  phy_control_cb_push_back(hypervisor_tx_handle->phy_ctrl_handle, phy_ctrl);
  // Unlock mutex so that function can do other things.
  pthread_mutex_unlock(&hypervisor_tx_handle->hypervisor_tx_mutex);
  // Notify other thread that slot control was pushed into container.
  pthread_cond_signal(&hypervisor_tx_handle->hypervisor_tx_cv);
}

int hypervisor_tx_send(hypervisor_tx_t* hyper_tx_handle, void *data, int nof_samples, time_t full_secs, double frac_secs, bool has_time_spec, bool blocking, bool is_start_of_burst, bool is_end_of_burst, bool is_lbt_enabled, void *lbt_stats_void_ptr, size_t channel) {
  return srslte_rf_send_timed3(hyper_tx_handle->hypervisor_tx_rf, data, nof_samples, full_secs, frac_secs, has_time_spec, blocking, is_start_of_burst, is_end_of_burst, is_lbt_enabled, (void*)lbt_stats_void_ptr, channel);
}

timer_t* hypervisor_tx_get_timer_id() {
  return &hypervisor_tx_handle->hyper_tx_timer_id;
}
