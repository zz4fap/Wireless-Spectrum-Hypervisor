#include "vphy_tx.h"

//************************************************
// *************** Global variables **************
//************************************************
// This vector of pointers is used to keep pointers to context structures that hold information of the vPHY Tx module.
static vphy_tx_thread_context_t *vphy_tx_threads[MAX_NUM_CONCURRENT_VPHYS] = {NULL};

//************************************************
// *********** Definition of functions ***********
//************************************************

// This function is used to set everything needed for a vPHY Tx thread to run accordinly.
int vphy_tx_start_thread(transceiver_args_t* const args, uint32_t vphy_id) {
  // Allocate memory for a new vPHY Tx thread object. Every vPHY Tx thread will have one of this objects.
  vphy_tx_threads[vphy_id] = (vphy_tx_thread_context_t*)srslte_vec_malloc(sizeof(vphy_tx_thread_context_t));
  // Check if memory allocation was correctly done.
  if(vphy_tx_threads[vphy_id] == NULL) {
    PHY_TX_ERROR("vPHY Tx ID: %d - Error allocating memory for vPHY Tx context\n", vphy_id);
    return -1;
  }
  // Initialize structure with zeros.
  bzero(vphy_tx_threads[vphy_id], sizeof(vphy_tx_thread_context_t));
  // Set vPHY ID.
  vphy_tx_threads[vphy_id]->vphy_id = vphy_id;
  // Initialize vPHY Tx thread context.
  vphy_tx_init_thread_context(vphy_tx_threads[vphy_id], args);
  // Set structures for vPHY Tx thread management.
  pthread_attr_init(&vphy_tx_threads[vphy_id]->vphy_tx_thread_attr);
  pthread_attr_setdetachstate(&vphy_tx_threads[vphy_id]->vphy_tx_thread_attr, PTHREAD_CREATE_JOINABLE);
  // Spawn vPHY Tx thread.
  int ret = pthread_create(&vphy_tx_threads[vphy_id]->vphy_tx_thread_id, &vphy_tx_threads[vphy_id]->vphy_tx_thread_attr, vphy_tx_work, (void*)vphy_tx_threads[vphy_id]);
  if(ret) {
    PHY_TX_ERROR("vPHY Tx ID: %d - Return code from pthread_create() is %d\n", vphy_id, ret);
    return -1;
  }
  return 0;
}

// Destroy the current vPHY transmission thread.
int vphy_tx_stop_thread(uint32_t vphy_id) {
  // Stop vPHY Tx thread.
  vphy_tx_threads[vphy_id]->vphy_run_tx_thread = false;
  // Notify condition variable.
  pthread_cond_signal(&vphy_tx_threads[vphy_id]->vphy_tx_cv);
  // Notify waiting threads so that they do not stay stuck.
  hypervisor_tx_notify_all_threads_waiting_for_semaphores();
  // Destroy vPHY Tx thread.
  pthread_attr_destroy(&vphy_tx_threads[vphy_id]->vphy_tx_thread_attr);
  int ret = pthread_join(vphy_tx_threads[vphy_id]->vphy_tx_thread_id, NULL);
  if(ret) {
    PHY_TX_ERROR("vPHY Tx ID: %d - Return code from pthread_join() is %d\n", vphy_id, ret);
    return -1;
  }
  // Destroy mutex.
  pthread_mutex_destroy(&vphy_tx_threads[vphy_id]->vphy_tx_mutex);
  // Destory conditional variable.
  if(pthread_cond_destroy(&vphy_tx_threads[vphy_id]->vphy_tx_cv) != 0) {
    PHY_TX_ERROR("vPHY Tx ID: %d - Conditional variable destruction for vPHY Tx failed.\n",vphy_id);
    return -1;
  }
  // Free all vPHY structures.
  vphy_tx_free_tx_structs(vphy_tx_threads[vphy_id]);
  VPHY_TX_INFO("vPHY Tx ID: %d - Tx Structs deleted!\n",vphy_id);
  // Delete vPHY Tx slot container.
  tx_cb_free(&vphy_tx_threads[vphy_id]->tx_handle);
  VPHY_TX_INFO("vPHY Tx ID: %d - Tx Slot container deleted!\n",vphy_id);
  // Destroy vPHY Tx PHY Control done semaphore.
  sem_destroy(&vphy_tx_threads[vphy_id]->vphy_tx_frame_done_semaphore);
  // Free memory used to store vPHY Tx thread object.
  if(vphy_tx_threads[vphy_id]) {
    free(vphy_tx_threads[vphy_id]);
    vphy_tx_threads[vphy_id] = NULL;
  }
  VPHY_TX_INFO("vPHY Tx ID: %d - Thread context deleted!\n",vphy_id);
  return 0;
}

// Initialize vPHY Tx object structure.
int vphy_tx_init_thread_context(vphy_tx_thread_context_t* const vphy_tx_thread_context, transceiver_args_t* const args) {
  vphy_tx_thread_context->vphy_run_tx_thread        = true;
  vphy_tx_thread_context->competition_bw            = args->competition_bw;
  vphy_tx_thread_context->radio_center_freq         = args->radio_center_frequency;
  vphy_tx_thread_context->rnti                      = args->rnti;
  vphy_tx_thread_context->use_std_carrier_sep       = args->use_std_carrier_sep;
  vphy_tx_thread_context->is_lbt_enabled            = args->lbt_threshold < 100.0?true:false;
  vphy_tx_thread_context->send_tx_stats_to_mac      = args->send_tx_stats_to_mac;
  vphy_tx_thread_context->add_tx_timestamp          = args->add_tx_timestamp;
  vphy_tx_thread_context->initial_subframe_index    = args->initial_subframe_index;
  vphy_tx_thread_context->freq_boost                = args->freq_boost;
  vphy_tx_thread_context->vphy_sampling_rate        = helpers_get_bw_from_nprb(args->nof_prb); // Get bandwidth from number of physical resource blocks.
  vphy_tx_thread_context->radio_nof_prb             = args->radio_nof_prb;
  vphy_tx_thread_context->radio_sampling_rate       = helpers_get_bw_from_nprb(args->radio_nof_prb); // Get bandwidth from number of physical resource blocks.
  vphy_tx_thread_context->radio_fft_len             = srslte_symbol_sz(args->radio_nof_prb);
  vphy_tx_thread_context->vphy_fft_len              = srslte_symbol_sz(args->nof_prb);
  vphy_tx_thread_context->num_of_tx_vphys           = args->num_of_tx_vphys;
  vphy_tx_thread_context->last_nof_subframes_to_tx  = 0;
  vphy_tx_thread_context->last_mcs                  = 0;
#if(ENABLE_WINDOWING==1)
  vphy_tx_thread_context->window_type               = args->window_type;
#endif
  vphy_tx_thread_context->decode_pdcch              = args->decode_pdcch; // If enabled, then PDCCH is decoded, otherwise, SCH is decoded.
  vphy_tx_thread_context->node_id                   = args->node_id;      // SRN ID, a number from 0 to 255.
  vphy_tx_thread_context->phy_filtering             = args->phy_filtering;
  vphy_tx_thread_context->max_turbo_decoder_noi     = args->max_turbo_decoder_noi;

#if(ENABLE_PACKET_DROPPING==1)
  // Instantiate and reserve NUMBER_OF_TX_DATA_BUFFERS divided by number of vPHY Tx threads positions.
  tx_cb_make(&vphy_tx_thread_context->tx_handle, NUMBER_OF_USER_DATA_BUFFERS/vphy_tx_thread_context->num_of_tx_vphys);
#else
  // Instantiate and reserve NUMBER_OF_TX_DATA_BUFFERS positions.
  tx_cb_make(&vphy_tx_thread_context->tx_handle, NUMBER_OF_USER_DATA_BUFFERS);
#endif
  // Initialize mutex used to synchronize between PHY and vPHY Tx threads.
  if(pthread_mutex_init(&vphy_tx_thread_context->vphy_tx_mutex, NULL) != 0) {
    PHY_TX_ERROR("vPHY Tx ID: %d - Mutex init failed.\n",vphy_tx_thread_context->vphy_id);
    return -1;
  }
  // Initialize condition variable used to synchronize between PHY and vPHY Tx threads.
  if(pthread_cond_init(&vphy_tx_thread_context->vphy_tx_cv, NULL)) {
    PHY_TX_ERROR("vPHY Tx ID: %d - Conditional variable init failed.\n",vphy_tx_thread_context->vphy_id);
    return -1;
  }
  // Initilize cell struture with transmission values.
  vphy_tx_init_cell_parameters(vphy_tx_thread_context, args);
  // Initialize structure with last configured Tx PHY control.
  vphy_tx_init_last_tx_slot_control(vphy_tx_thread_context, args);
  // Initialize base station structures.
  vphy_tx_init_tx_structs(vphy_tx_thread_context, args->rnti);
  // Initial update of allocation with MCS 0 and initial number of resource blocks.
  vphy_tx_update_radl(vphy_tx_thread_context, 0, args->nof_prb);
  // Initialize vPHY Tx PHY Control done semaphore.
  sem_init(&vphy_tx_thread_context->vphy_tx_frame_done_semaphore, 0, 0);
  // Create a watchdog timer for vPHY Tx thread.
#if(ENABLE_WATCHDOG_TIMER==1)
  if(timer_init(&vphy_tx_thread_context->vphy_tx_timer_id) < 0) {
    PHY_TX_ERROR("vPHY Tx ID: %d - Not possible to create a timer for vPHY Tx thread.\n", vphy_tx_thread_context->vphy_id);
    return -1;
  }
#endif
  // Everything went well.
  return 0;
}

// Initialize struture with Cell parameters.
void vphy_tx_init_cell_parameters(vphy_tx_thread_context_t* const vphy_tx_thread_context, transceiver_args_t* const args) {
  vphy_tx_thread_context->vphy_enodeb.nof_prb          = args->nof_prb;      // nof_prb
  vphy_tx_thread_context->vphy_enodeb.nof_ports        = args->nof_ports;    // nof_ports
  vphy_tx_thread_context->vphy_enodeb.bw_idx           = 0;                  // bw idx
  vphy_tx_thread_context->vphy_enodeb.id               = args->radio_id;     // cell_id
  vphy_tx_thread_context->vphy_enodeb.cp               = SRSLTE_CP_NORM;     // cyclic prefix
  vphy_tx_thread_context->vphy_enodeb.phich_length     = SRSLTE_PHICH_NORM;  // PHICH length
  vphy_tx_thread_context->vphy_enodeb.phich_resources  = SRSLTE_PHICH_R_1;   // PHICH resources
}

// Initialize structure with last configured Tx slot control.
void vphy_tx_init_last_tx_slot_control(vphy_tx_thread_context_t* const vphy_tx_thread_context, transceiver_args_t* const args) {
  vphy_tx_thread_context->last_tx_slot_control.send_to      = args->node_id;
  vphy_tx_thread_context->last_tx_slot_control.intf_id      = vphy_tx_thread_context->vphy_id;
  vphy_tx_thread_context->last_tx_slot_control.bw_idx       = helpers_get_bw_index_from_prb(args->nof_prb); // This is the virtual PHY BW. Convert from number of Resource Blocks to BW Index.
  vphy_tx_thread_context->last_tx_slot_control.ch           = args->default_tx_channel;                     // Initial channel is set to 0.
  vphy_tx_thread_context->last_tx_slot_control.frame        = 0;
  vphy_tx_thread_context->last_tx_slot_control.slot         = 0;
  vphy_tx_thread_context->last_tx_slot_control.mcs          = 0;
  vphy_tx_thread_context->last_tx_slot_control.freq_boost   = args->freq_boost;
  vphy_tx_thread_context->last_tx_slot_control.length       = 1;
}

// Initialize vPHY Tx structs used to create slots.
int vphy_tx_init_tx_structs(vphy_tx_thread_context_t* const vphy_tx_thread_context, uint16_t rnti) {
  // Initialize control channels only if enabled.
  if(vphy_tx_thread_context->decode_pdcch) {
    // Initialize registers.
    if(srslte_regs_init(&vphy_tx_thread_context->regs, vphy_tx_thread_context->vphy_enodeb)) {
      PHY_TX_ERROR("Error initiating regs\n",0);
      return -1;
    }
    // Initialize PCFICH object.
    if(srslte_pcfich_init(&vphy_tx_thread_context->pcfich, &vphy_tx_thread_context->regs, vphy_tx_thread_context->vphy_enodeb)) {
      PHY_TX_ERROR("Error creating PBCH object\n",0);
      return -1;
    }
    // Initialize CFI register object.
    if(srslte_regs_set_cfi(&vphy_tx_thread_context->regs, DEFAULT_CFI)) {
      PHY_TX_ERROR("Error setting CFI\n",0);
      return -1;
    }
    // Initialize PDCCH object.
    if(srslte_pdcch_init(&vphy_tx_thread_context->pdcch, &vphy_tx_thread_context->regs, vphy_tx_thread_context->vphy_enodeb)) {
      PHY_TX_ERROR("Error creating PDCCH object\n",0);
      return -1;
    }
    // Initiate valid DCI locations.
    for(uint32_t i = 0; i < SRSLTE_NSUBFRAMES_X_FRAME; i++) {
      srslte_pdcch_ue_locations(&vphy_tx_thread_context->pdcch, vphy_tx_thread_context->locations[i], 30, i, DEFAULT_CFI, rnti);
    }
  }
  // Initialize PDSCH object.
  if(srslte_pdsch_init_generic(&vphy_tx_thread_context->pdsch, vphy_tx_thread_context->vphy_enodeb, vphy_tx_thread_context->vphy_id)) {
    PHY_TX_ERROR("Error creating PDSCH object\n",0);
    return -1;
  }
  // Set RNTI for PDSCH object.
  srslte_pdsch_set_rnti(&vphy_tx_thread_context->pdsch, rnti);
  // Initilize softbuffer.
  if(srslte_softbuffer_tx_init(&vphy_tx_thread_context->softbuffer, vphy_tx_thread_context->vphy_enodeb.nof_prb)) {
    PHY_TX_ERROR("Error initiating soft buffer\n",0);
    return -1;
  }
  // Reset softbuffer x reset.
  srslte_softbuffer_tx_reset(&vphy_tx_thread_context->softbuffer);
  // Generate CRS signals.
  if(srslte_chest_dl_init(&vphy_tx_thread_context->est, vphy_tx_thread_context->vphy_enodeb)) {
    PHY_TX_ERROR("Error initializing equalizer.\n", 0);
    return -1;
  }
  // Generate PSS sequence.
  int N_id_2 = vphy_tx_thread_context->vphy_enodeb.id % 3;
  srslte_pss_generate(vphy_tx_thread_context->pss_signal, N_id_2);
  // Generate SCH carrying SRN and Interface ID only if PHY filtering is enabled.
  if(vphy_tx_thread_context->phy_filtering) {
    // Generate SCH sequence carrying SRN ID plus Interface ID set to 0.
    srslte_sch_generate_from_pair(vphy_tx_thread_context->sch_signal0, true, vphy_tx_thread_context->last_tx_slot_control.send_to, vphy_tx_thread_context->last_tx_slot_control.intf_id);
  }
  // Generate SCH sequence carrying MCS plus number of transmitted slots set to 0.
  srslte_sch_generate_from_pair(vphy_tx_thread_context->sch_signal1, false, vphy_tx_thread_context->last_mcs, vphy_tx_thread_context->last_nof_subframes_to_tx);
  // Retrieve pointer to subframe buffer.
  cf_t** const subframe_buffer = hypervisor_tx_get_subframe_buffer();
  for(uint32_t i = 0; i < MAX_NUMBER_OF_TBS_IN_A_SLOT; i++) {
    if(subframe_buffer[i] == NULL) {
      PHY_TX_ERROR("vPHY Tx ID: %d - Subframer buffer index: %d was not allocated.\n", vphy_tx_thread_context->vphy_id, i);
      return -1;
    }
  }
  // for now, there is only 1 port.
  for(uint32_t k = 0; k < MAX_NUMBER_OF_TBS_IN_A_SLOT; k++) {
    for(uint32_t i = 0; i < SRSLTE_MAX_PORTS; i++) {
      vphy_tx_thread_context->subframe_symbols[k][i] = subframe_buffer[k];
      vphy_tx_thread_context->preamble_symbols[k] = NULL;
    }
  }

#if(ENABLE_WINDOWING==1)
  // Initialize window function.
  vphy_tx_thread_context->useful_length = vphy_tx_thread_context->vphy_enodeb.nof_prb*SRSLTE_NRE; // This is the number of useful subcarriers.
  vphy_tx_thread_context->guard_band_length = WINDOWING_GUARD_BAND; // hard-coded value for guard-bands (GB), there will be 20 subcarriers at the fron and 20 at the back as GB.
  vphy_tx_thread_context->tx_window_length = vphy_tx_thread_context->useful_length + 2*vphy_tx_thread_context->guard_band_length;
  // Allocate memory for window function.
  vphy_tx_thread_context->tx_window = (float*)srslte_vec_malloc(sizeof(float)*vphy_tx_thread_context->tx_window_length);
  // Check if memory allocation was correctly done.
  if(vphy_tx_thread_context->tx_window == NULL) {
    PHY_TX_ERROR("vPHY Tx ID: %d - Error allocating memory for window function.\n", vphy_tx_thread_context->vphy_id);
    return -1;
  }
  // Allocate memory for window function.
  vphy_tx_thread_context->rf_boosted_tx_window = (float*)srslte_vec_malloc(sizeof(float)*vphy_tx_thread_context->tx_window_length);
  // Check if memory allocation was correctly done.
  if(vphy_tx_thread_context->rf_boosted_tx_window == NULL) {
    PHY_TX_ERROR("vPHY Tx ID: %d - Error allocating memory for rf boosted window function.\n", vphy_tx_thread_context->vphy_id);
    return -1;
  }
  // Initiate window function.
  for(uint32_t i = 0; i < vphy_tx_thread_context->tx_window_length; i++) {
    // Select which window should be used.
    switch(vphy_tx_thread_context->window_type) {
      case WINDOW_HANN:
        vphy_tx_thread_context->tx_window[i] = hann(i,vphy_tx_thread_context->tx_window_length);
        break;
      case WINDOW_KAISER:
        vphy_tx_thread_context->tx_window[i] = kaiser(i,vphy_tx_thread_context->tx_window_length, 2.5, 0.0f);
        break;
      case WINDOW_LIQUID:
        vphy_tx_thread_context->tx_window[i] = liquid_rcostaper_windowf(i, vphy_tx_thread_context->guard_band_length, vphy_tx_thread_context->tx_window_length);
        break;
      default:
        PHY_TX_ERROR("vPHY Tx ID: %d - Wrong window type selected: %d.\n", vphy_tx_thread_context->vphy_id, vphy_tx_thread_context->window_type);
        return -1;
    }
  }
  // Apply initial RF boost gain to window.
  srslte_vec_sc_prod_fff(vphy_tx_thread_context->tx_window, vphy_tx_thread_context->freq_boost, vphy_tx_thread_context->rf_boosted_tx_window, vphy_tx_thread_context->tx_window_length);
#endif
  // Everything went well.
  return 0;
}

// Update user control data, which is carried in PDCCH.
int vphy_tx_update_radl(vphy_tx_thread_context_t* const vphy_tx_thread_context, uint32_t mcs, uint32_t nof_prb) {
  bzero(&vphy_tx_thread_context->ra_dl, sizeof(srslte_ra_dl_dci_t));
  vphy_tx_thread_context->ra_dl.harq_process = 0;
  vphy_tx_thread_context->ra_dl.mcs_idx = mcs;
  vphy_tx_thread_context->ra_dl.ndi = 0;
  vphy_tx_thread_context->ra_dl.rv_idx = 0;
  vphy_tx_thread_context->ra_dl.alloc_type = SRSLTE_RA_ALLOC_TYPE0;
  vphy_tx_thread_context->ra_dl.type0_alloc.rbg_bitmask = vphy_tx_prbset_to_bitmask(nof_prb);
  return 0;
}

void vphy_tx_free_tx_structs(vphy_tx_thread_context_t* const vphy_tx_thread_context) {
  srslte_softbuffer_tx_free(&vphy_tx_thread_context->softbuffer);
  srslte_pdsch_free(&vphy_tx_thread_context->pdsch);
  srslte_chest_dl_free(&vphy_tx_thread_context->est);
  if(vphy_tx_thread_context->decode_pdcch) {
    srslte_pcfich_free(&vphy_tx_thread_context->pcfich);
    srslte_pdcch_free(&vphy_tx_thread_context->pdcch);
    srslte_regs_free(&vphy_tx_thread_context->regs);
  }
}

// This fucntion is used to check and change configuration parameters related to transmission.
int vphy_tx_change_parameters(vphy_tx_thread_context_t* const vphy_tx_thread_context, slot_ctrl_t* const slot_ctrl) {
  uint32_t bw_idx, req_mcs;

  // This is a basic sanity test, can be removed in the future as both MUST be the same.
  if(vphy_tx_thread_context->vphy_id != slot_ctrl->vphy_id) {
    PHY_TX_DEBUG("vPHY Tx ID: %d - Context vPHY Tx ID: %d different from phy control vPHY Tx ID: %d\n", vphy_tx_thread_context->vphy_id, vphy_tx_thread_context->vphy_id, slot_ctrl->vphy_id);
    return -1;
  }
  // Check MCS index range.
  if(slot_ctrl->mcs > 28) {
    VPHY_TX_ERROR("vPHY Tx ID: %d - Invalid MCS: %d!!\n", vphy_tx_thread_context->vphy_id, slot_ctrl->mcs);
    return -1;
  }
  // Retrieve the bandwidth used by this vPHY.
  vphy_tx_thread_context->vphy_sampling_rate = helpers_get_bandwidth(slot_ctrl->bw_idx, &bw_idx);
  if(vphy_tx_thread_context->vphy_sampling_rate < 0.0 || bw_idx >= 100) {
    VPHY_TX_ERROR("vPHY Tx ID: %d - Invalid Tx BW Index: %d\n", vphy_tx_thread_context->vphy_id, slot_ctrl->bw_idx);
    return -1;
  }
  // Check if channel number is outside the possible range.
  if(slot_ctrl->ch > MAX_NUM_OF_CHANNELS) {
    VPHY_TX_ERROR("vPHY Tx ID: %d - Invalid Channel: %d\n", vphy_tx_thread_context->vphy_id, slot_ctrl->ch);
    return -1;
  }
  // Check send_to field range.
  if(slot_ctrl->send_to >= MAXIMUM_NUMBER_OF_RADIOS) {
    VPHY_TX_ERROR("vPHY Tx ID: %d - Invalid send_to field: %d, it must be less than or equal to %d.\n", vphy_tx_thread_context->vphy_id, slot_ctrl->send_to, MAXIMUM_NUMBER_OF_RADIOS);
    return -1;
  }
  // Check Interface ID field range.
  if(slot_ctrl->intf_id >= MAX_NUM_CONCURRENT_VPHYS) {
    VPHY_TX_ERROR("vPHY Tx ID: %d - Invalid interface ID field: %d, it must be less than or equal to %d.\n", vphy_tx_thread_context->vphy_id, slot_ctrl->intf_id, MAX_NUM_CONCURRENT_VPHYS);
    return -1;
  }
  // Check Transport Block (TB) size.
  if(vphy_tx_validate_tb_size(slot_ctrl, bw_idx) < 0) {
    VPHY_TX_ERROR("vPHY Tx ID: %d - Invalid TB size.\n", vphy_tx_thread_context->vphy_id);
    return -1;
  }
  // Change channel only if one of the parameters have changed.
  if(vphy_tx_thread_context->last_tx_slot_control.ch != slot_ctrl->ch) {
    // Update last tx slot control structure.
    vphy_tx_thread_context->last_tx_slot_control.ch = slot_ctrl->ch;
    VPHY_TX_DEBUG_TIME("vPHY Tx ID: %d - BW[%d]: %1.2f [MHz] - Channel: %d - Set freq to: %.2f [MHz]\n", vphy_tx_thread_context->vphy_id, slot_ctrl->bw_idx, (vphy_tx_thread_context->vphy_sampling_rate/1000000.0), slot_ctrl->ch, vphy_tx_get_channel_center_freq(vphy_tx_thread_context, slot_ctrl->ch)/1000000.0);
  }
  // Generate new PSS and SSS signals if BW index is different from last one.
  if(vphy_tx_thread_context->last_tx_slot_control.bw_idx != slot_ctrl->bw_idx) {
    // Set eNodeB PRB based on BW index.
    vphy_tx_thread_context->vphy_enodeb.nof_prb = helpers_get_prb_from_bw_index(slot_ctrl->bw_idx);
    // Free all related Tx structures.
    vphy_tx_free_tx_structs(vphy_tx_thread_context);
    // Initialize base station structures.
    vphy_tx_init_tx_structs(vphy_tx_thread_context, vphy_tx_thread_context->rnti);
  }

  // Change MCS. For 1.4 MHz we can not set MCS 28 for the very first subframe as it carries PSS/SSS and does not have enough "room" for FEC bits (code rate is greater than 1).
  req_mcs = slot_ctrl->mcs;
  if(slot_ctrl->bw_idx == BW_IDX_OneDotFour && slot_ctrl->mcs >= 28) {
    // Maximum MCS for 1.4 MHz PHY BW is 27.
    req_mcs = slot_ctrl->mcs - 1;
  }
  // Update MCS.
  vphy_tx_change_allocation(vphy_tx_thread_context, req_mcs, slot_ctrl->bw_idx);

  // Change RF boost only if the parameter has changed.
  if(slot_ctrl->freq_boost > 0.0 && vphy_tx_thread_context->last_tx_slot_control.freq_boost != slot_ctrl->freq_boost) {
    vphy_tx_thread_context->freq_boost = slot_ctrl->freq_boost;
    vphy_tx_thread_context->last_tx_slot_control.freq_boost = slot_ctrl->freq_boost;

#if(ENABLE_WINDOWING==1)
    // Update rf boosted window with new RF boost gain.
    // Apply frequency gain to window. Frequency domain boost.
    srslte_vec_sc_prod_fff(vphy_tx_thread_context->tx_window, vphy_tx_thread_context->freq_boost, vphy_tx_thread_context->rf_boosted_tx_window, vphy_tx_thread_context->tx_window_length);
#endif

    VPHY_TX_DEBUG_TIME("vPHY Tx ID: %d - Frequency domain boost set to: %1.2f\n", vphy_tx_thread_context->vphy_id, slot_ctrl->freq_boost);
  }
  // Everything went well.
  return 0;
}

// This is the transmission work function, i.e., the function that implements the vPHY Tx thread.
void *vphy_tx_work(void *h) {

  vphy_tx_thread_context_t* const vphy_tx_thread_context = (vphy_tx_thread_context_t * const)h;
  phy_stat_t phy_tx_stat;
  int sf_idx, subframe_cnt, tx_data_offset;
  uint32_t mcs_local, change_param_status;
  srslte_dci_msg_t dci_msg;
  double coding_time = 0.0;
  struct timespec coding_timespec;
  slot_ctrl_t slot_ctrl;
  bool transmission_done = false;
  uint64_t number_of_dropped_packets = 0;
  int dc_pos = 0;
#if(CHECK_TX_OUT_OF_SEQUENCE==1)
  uint32_t data_cnt = 0;
#endif

  // Set priority to Tx thread.
  uhd_set_thread_priority(1.0, true);

#if(ENABLE_STICKING_THREADS_TO_CORES==1)
  int ret = helpers_stick_this_thread_to_core(vphy_tx_thread_context->vphy_id + 12);
  if(ret != 0) {
    VPHY_TX_ERROR("--------> Set vPHY Tx ID: %d Tx thread to CPU ret: %d\n", vphy_tx_thread_context->vphy_id, ret);
  }
#endif

  /****************************** vPHY Tx thread loop - BEGIN ******************************/
  VPHY_TX_DEBUG("vPHY Tx ID : %d - Entering main thread loop...\n", vphy_tx_thread_context->vphy_id);
  while(vphy_tx_wait_and_pop_tx_slot_control_from_container(vphy_tx_thread_context, &slot_ctrl) && vphy_tx_thread_context->vphy_run_tx_thread) {

    // Timestamp start of transmission procedure.
    clock_gettime(CLOCK_REALTIME, &coding_timespec);

    // Set watchdog timer for synchronization thread. Always wait for some seconds.
#if(ENABLE_WATCHDOG_TIMER==1)
    if(timer_set(&vphy_tx_thread_context->vphy_tx_timer_id, 2) < 0) {
      PHY_TX_ERROR("vPHY Tx ID: %d - Not possible to set the watchdog timer for vPHY Tx thread.\n", vphy_tx_thread_context->vphy_id);
    }
#endif

    // Check is vPHY IDs match.
    if(vphy_tx_thread_context->vphy_id != slot_ctrl.vphy_id) {
      VPHY_TX_ERROR("vPHY ID in the context: %d does not match the ID: %d in the slot control message. Dropping MAC message.\n", vphy_tx_thread_context->vphy_id, slot_ctrl.vphy_id);
      number_of_dropped_packets++;
      continue;
    }

    // Change transmission parameters according to received PHY control message.
    change_param_status = vphy_tx_change_parameters(vphy_tx_thread_context, &slot_ctrl);

    if(change_param_status >= 0) {
      // Register frame done semaphore.
      hypervisor_tx_register_frame_mod_done_sem(&vphy_tx_thread_context->vphy_tx_frame_done_semaphore);

      // Reset all necessary counters and flags before transmitting.
      sf_idx = vphy_tx_thread_context->initial_subframe_index;
      tx_data_offset = 0;
      subframe_cnt = 0;

      // Calculate vPHY DC subcarrier position.
      dc_pos = (slot_ctrl.ch*vphy_tx_thread_context->vphy_fft_len);

      VPHY_TX_DEBUG("vPHY Tx ID: %d - Number of TBs to be transmitted: %d - Entering modulation loop...\n", vphy_tx_thread_context->vphy_id, slot_ctrl.nof_tb_in_slot);
      while(subframe_cnt < slot_ctrl.nof_tb_in_slot && vphy_tx_thread_context->vphy_run_tx_thread) {

        // Add PSS/SCH/SSS to subframe. We use frame structure type II (PSS/SSS added only to very first subframe).
        if(sf_idx == 0 || sf_idx == 5) {
          // Map PSS sequence into the resource grid taking into account the DC subcarrier.
          srslte_insert_pss_into_subframe_with_dc(vphy_tx_thread_context->pss_signal, vphy_tx_thread_context->subframe_symbols[subframe_cnt][0], vphy_tx_thread_context->radio_fft_len, dc_pos, vphy_tx_thread_context->vphy_enodeb.cp);
          // If decode PDCCH/PCFICH is enabled, then we map SSS sequence carrying number of transmitted slots, otherwise, we map SCH sequences carrying SRN ID/Radio Interface ID and MCS/number of transmitted slots.
          if(vphy_tx_thread_context->decode_pdcch) {
            // Check if current number of subframes to transmit is different from last one.
            if(vphy_tx_thread_context->last_nof_subframes_to_tx != slot_ctrl.nof_tb_in_slot) {
              // Generate SSS sequence carrying number of slots.
              srslte_sss_generate_nof_packets_signal(vphy_tx_thread_context->sss_signal, sf_idx, slot_ctrl.nof_tb_in_slot);
              // Update variable keeping last configured number of transmitted subframes.
              vphy_tx_thread_context->last_nof_subframes_to_tx = slot_ctrl.nof_tb_in_slot;
            }
            // Insert SSS sequence into resource grid taking into account the DC subcarrier.
            srslte_insert_sss_into_subframe_with_dc(vphy_tx_thread_context->sss_signal, vphy_tx_thread_context->subframe_symbols[subframe_cnt][0], vphy_tx_thread_context->radio_fft_len, dc_pos, vphy_tx_thread_context->vphy_enodeb.cp);
          } else {
            // If PHY filtering is enable then we encode SRN ID and Radio Interface ID.
            if(vphy_tx_thread_context->phy_filtering) {
              // Check if send_to or interface ID values are diferent and then, generate new signal.
              if(vphy_tx_thread_context->last_tx_slot_control.send_to != slot_ctrl.send_to || vphy_tx_thread_context->last_tx_slot_control.intf_id != slot_ctrl.intf_id) {
                // Generate SCH sequence carrying SRN ID plus Interface ID.
                srslte_sch_generate_from_pair(vphy_tx_thread_context->sch_signal0, true, slot_ctrl.send_to, slot_ctrl.intf_id);
                // Update last configured values.
                vphy_tx_thread_context->last_tx_slot_control.send_to = slot_ctrl.send_to;
                vphy_tx_thread_context->last_tx_slot_control.intf_id = slot_ctrl.intf_id;
              }
              // Map SCH sequence into the front of the resource grid.
              srslte_insert_sch_into_subframe_front_with_dc(vphy_tx_thread_context->sch_signal0, vphy_tx_thread_context->subframe_symbols[subframe_cnt][0], vphy_tx_thread_context->radio_fft_len, vphy_tx_thread_context->vphy_fft_len, dc_pos);
            }
            // Check if current number of subframes to transmit or MCS is different from the last ones.
            if(vphy_tx_thread_context->last_nof_subframes_to_tx != slot_ctrl.nof_tb_in_slot || vphy_tx_thread_context->last_mcs != slot_ctrl.mcs) {
              // Generate SCH sequence carrying MCS plus number of transmitted slots.
              srslte_sch_generate_from_pair(vphy_tx_thread_context->sch_signal1, false, slot_ctrl.mcs, slot_ctrl.nof_tb_in_slot);
              // Update last configured values.
              vphy_tx_thread_context->last_mcs = slot_ctrl.mcs;
              vphy_tx_thread_context->last_nof_subframes_to_tx = slot_ctrl.nof_tb_in_slot;
            }
            // Map SCH sequence into the middle of the resource grid.
            srslte_insert_sch_into_subframe_with_dc_generic(vphy_tx_thread_context->sch_signal1, vphy_tx_thread_context->subframe_symbols[subframe_cnt][0], vphy_tx_thread_context->radio_fft_len, dc_pos, 5, true);
          }
        }

        // Add reference signal (RS).
        srslte_insert_refsignal_into_subframe_with_dc(vphy_tx_thread_context->vphy_enodeb, vphy_tx_thread_context->radio_fft_len, dc_pos, 0, vphy_tx_thread_context->est.csr_signal.pilots[0][sf_idx], vphy_tx_thread_context->subframe_symbols[subframe_cnt][0]);

        // Change MCS of subsequent subframes to the highest possible value.
        if(subframe_cnt == 1 && slot_ctrl.bw_idx == BW_IDX_OneDotFour && slot_ctrl.mcs >= 28) {
          vphy_tx_change_allocation(vphy_tx_thread_context, slot_ctrl.mcs, slot_ctrl.bw_idx);
        }
        VPHY_TX_DEBUG("subframe_cnt: %d - MCS set to: %d\n", subframe_cnt, vphy_tx_thread_context->ra_dl.mcs_idx);

        // If decode PDCCH/PCFICH is enabled, then we map those signals into the resource grid.
        if(vphy_tx_thread_context->decode_pdcch) {
          // Encode PCFICH.
          srslte_encode_and_map_pcfich_with_dc(&vphy_tx_thread_context->pcfich, DEFAULT_CFI, vphy_tx_thread_context->subframe_symbols[subframe_cnt], sf_idx, vphy_tx_thread_context->radio_fft_len, dc_pos);
          // Encode PDCCH with control for user data decoding.
          VPHY_TX_DEBUG("vPHY Tx ID: %d - Putting DCI to location: n=%d, L=%d\n", vphy_tx_thread_context->vphy_id, vphy_tx_thread_context->locations[sf_idx][0].ncce, vphy_tx_thread_context->locations[sf_idx][0].L);
          srslte_dci_msg_pack_pdsch(&vphy_tx_thread_context->ra_dl, SRSLTE_DCI_FORMAT1, &dci_msg, vphy_tx_thread_context->vphy_enodeb.nof_prb, false);
          if(srslte_encode_and_map_pdcch_with_dc(&vphy_tx_thread_context->pdcch, &dci_msg, vphy_tx_thread_context->locations[sf_idx][0], vphy_tx_thread_context->rnti, vphy_tx_thread_context->subframe_symbols[subframe_cnt], sf_idx, DEFAULT_CFI, vphy_tx_thread_context->radio_fft_len, dc_pos)) {
            VPHY_TX_ERROR("Error encoding DCI message for vPHY Tx ID: %d. Droping MAC message.\n",vphy_tx_thread_context->vphy_id);
            number_of_dropped_packets++;
            break;
          }
        }

        // Configure PDSCH to transmit the requested allocation.
        srslte_ra_dl_dci_to_grant(&vphy_tx_thread_context->ra_dl, vphy_tx_thread_context->vphy_enodeb.nof_prb, vphy_tx_thread_context->rnti, &vphy_tx_thread_context->grant);
        if(srslte_pdsch_cfg(&vphy_tx_thread_context->pdsch_cfg, vphy_tx_thread_context->vphy_enodeb, &vphy_tx_thread_context->grant, DEFAULT_CFI, sf_idx, 0)) {
          VPHY_TX_ERROR("vPHY Tx ID: %d - Error configuring PDSCH. Dropping MAC message.\n",vphy_tx_thread_context->vphy_id);
          number_of_dropped_packets++;
          break;
        }

        // Add Tx timestamp to data.
  #if(ENABLE_TX_TO_RX_TIME_DIFF==1)
        uint64_t tx_timestamp;
        struct timespec tx_timestamp_struct;
        if(vphy_tx_thread_context->add_tx_timestamp) {
          clock_gettime(CLOCK_REALTIME, &tx_timestamp_struct);
          tx_timestamp = helpers_convert_host_timestamp(&tx_timestamp_struct);
          memcpy((void*)(slot_ctrl.data+tx_data_offset), (void*)&tx_timestamp, sizeof(uint64_t));
        }
  #endif

        // Encode PDSCH.
        if(srslte_encode_and_map_pdsch_with_dc(&vphy_tx_thread_context->pdsch, &vphy_tx_thread_context->pdsch_cfg, &vphy_tx_thread_context->softbuffer, slot_ctrl.data+tx_data_offset, vphy_tx_thread_context->subframe_symbols[subframe_cnt], vphy_tx_thread_context->radio_fft_len, dc_pos)) {
          VPHY_TX_ERROR("vPHY Tx ID: %d - Error encoding PDSCH. Dropping MAC message.\n",vphy_tx_thread_context->vphy_id);
          number_of_dropped_packets++;
          break;
        }

  #if(CHECK_TX_OUT_OF_SEQUENCE==1)
        uint32_t data_vector[32] = {0, 1, 2, 3, 32, 33, 34, 35, 64, 65, 66, 67, 96, 97, 98, 99, 128, 129, 130, 131, 160, 161, 162, 163, 192, 193, 194, 195, 224, 225, 226, 227};
        if(vphy_tx_thread_context->vphy_id == 0) {
          if(slot_ctrl.data[tx_data_offset] != data_vector[data_cnt]) {
            printf("Received: %d - Expected: %d\n", slot_ctrl.data[tx_data_offset], data_vector[data_cnt]);
          }
          data_cnt = (data_cnt + 1) % 32;
        }
  #endif

        // Increment data offset when there is more than 1 slot to be transmitted.
        mcs_local = slot_ctrl.mcs;
        if(subframe_cnt == 0 && slot_ctrl.bw_idx == BW_IDX_OneDotFour && slot_ctrl.mcs >= 28) {
          mcs_local = mcs_local - 1;
        }
        tx_data_offset = tx_data_offset + communicator_get_tb_size(slot_ctrl.bw_idx-1, mcs_local);

  #if(ENABLE_WINDOWING==1)
        // Apply windowing to signal in frequency domain.
        if(vphy_tx_thread_context->window_type != WINDOW_LIQUID) {
          srslte_vec_prod_cfc_with_dc(vphy_tx_thread_context->subframe_symbols[subframe_cnt][0], vphy_tx_thread_context->rf_boosted_tx_window, vphy_tx_thread_context->subframe_symbols[subframe_cnt][0], SRSLTE_CP_NSYMB(vphy_tx_thread_context->vphy_enodeb.cp), vphy_tx_thread_context->radio_fft_len, vphy_tx_thread_context->vphy_fft_len, vphy_tx_thread_context->vphy_enodeb.nof_prb, slot_ctrl.ch);
        } else {
          srslte_vec_prod_cfc_with_dc_liq_window(vphy_tx_thread_context->subframe_symbols[subframe_cnt][0], vphy_tx_thread_context->rf_boosted_tx_window, vphy_tx_thread_context->subframe_symbols[subframe_cnt][0], SRSLTE_CP_NSYMB(vphy_tx_thread_context->vphy_enodeb.cp), vphy_tx_thread_context->radio_fft_len, vphy_tx_thread_context->vphy_fft_len, vphy_tx_thread_context->vphy_enodeb.nof_prb, slot_ctrl.ch, vphy_tx_thread_context->guard_band_length);
        }
  #else
        // Apply gain to modulation symbols in the frequency domain. Frequency domain boost.
        srslte_vec_sc_prod_cfc_with_dc(vphy_tx_thread_context->subframe_symbols[subframe_cnt][0], vphy_tx_thread_context->freq_boost, vphy_tx_thread_context->subframe_symbols[subframe_cnt][0], SRSLTE_CP_NSYMB(vphy_tx_thread_context->vphy_enodeb.cp), vphy_tx_thread_context->radio_fft_len, vphy_tx_thread_context->vphy_fft_len, vphy_tx_thread_context->vphy_enodeb.nof_prb, slot_ctrl.ch);
  #endif

        //VPHY_TX_PRINT("vPHY Tx IDd: %d - Coding time: %f\n",vphy_tx_thread_context->vphy_id, helpers_profiling_diff_time(coding_timespec));

        // Only informs Hypervisor Tx if thread is still running.
        if(vphy_tx_thread_context->vphy_run_tx_thread) {
          transmission_done = false;
          // Call function from hypervisor that modulates the signal into OFDM symbols and then transmits it as a 23.04 MHz PHY BW signal.
          // The last vPHY Tx thread to call this function gets to transmit the OFDM modulated signal.
          if(hypervisor_tx_inform_subframe_modulation_done(&slot_ctrl, subframe_cnt)) {
            // Timestamp start of transmission procedure.
            //clock_gettime(CLOCK_REALTIME, &coding_timespec);

            // Transmit the OFDM modulated signal.
            hypervisor_tx_transmit(&slot_ctrl, subframe_cnt);

            //PHY_PROFILLING_AVG6("vPHY ID: %d - Avg. coding time: %f [ms] - min: %f [ms] - max: %f [ms] - max counter %d - diff >= 1.0 [ms]: %d - total counter: %d - perc: %f\n", vphy_tx_thread_context->vphy_id, helpers_profiling_diff_time(coding_timespec), 1.0, 500);

            // Flag used to inform that the OFDM symbol containing all the vPHY Tx OFDM subframes was transferred to the USRP.
            transmission_done = true;
          }
        }

        // Increase subframe counter number.
        subframe_cnt++;

        // Here we use subframe (initial_subframe_index + 1) to send the remaning data, however, it could be any subframe different from 0 and 5.
        sf_idx = vphy_tx_thread_context->initial_subframe_index + 1;

      } // End of subframe encoding loop.

      // Check if transmission of vPHY Tx stats to MAC is enabled.
      if(vphy_tx_thread_context->send_tx_stats_to_mac) {
        // Calculate coding time.
        coding_time = helpers_profiling_diff_time(coding_timespec);
        // Create a PHY Tx Stat struture to inform upper layers transmission was successful.
        phy_tx_stat.seq_number                = slot_ctrl.seq_number;                                                                // Sequence number used by upper layer to track the response of PHY, i.e., correlates one basic_control message with a phy_stat message.
        phy_tx_stat.status                    = PHY_SUCCESS;          // Layer receceiving this message MUST check the statistics it is carrying for real status of the current request.
        phy_tx_stat.frame                     = 0;                                                                                        // Frame number.
        phy_tx_stat.slot                      = 0;			                                                                                    // Time slot number. Range: [0, 2000]
        phy_tx_stat.host_timestamp            = helpers_get_host_time_now();                                                     // Host PC time value when (ch,slot) PHY data are demodulated.
        phy_tx_stat.ch                        = slot_ctrl.ch;				                                                                        // Channel number which in turn is translated to a central frequency. Range: [0, 59]
        phy_tx_stat.mcs                       = slot_ctrl.mcs;                                                                              // Set MCS to unspecified number. If this number is receiber by upper layer it means nothing was received and status MUST be checked.
        phy_tx_stat.num_cb_total              = slot_ctrl.nof_tb_in_slot;
        phy_tx_stat.num_cb_err                = number_of_dropped_packets;                                                         // Number of slots requested to be transmitted.
        phy_tx_stat.stat.tx_stat.coding_time  = coding_time;                                                           // Time it takes to code the whole COT, i.e., all the slots being sent in a row.
        phy_tx_stat.stat.tx_stat.freq_boost   = vphy_tx_thread_context->freq_boost;                                     // Freq. boost is applied to frequency domain modulation symbols before being mapped to subcarriers.
        // Send PHY transmission (Tx) statistics.
        phy_comm_ctrl_send_tx_statistics(&phy_tx_stat);
      }

      //VPHY_TX_DEBUG("vPHY Tx ID: %d - Coding time: %f\n",vphy_tx_thread_context->vphy_id, helpers_profiling_diff_time(coding_timespec));

      // Only informs Hypervisor Tx if thread is still running.
      if(vphy_tx_thread_context->vphy_run_tx_thread) {
        // If this is the last transmitted subframe of a PHY control message, then we notify all waiting threads.
        if(transmission_done && subframe_cnt >= slot_ctrl.slot_info.largest_nof_tbs_in_slot) {
          // If this is the last vPHY Tx to transmit a subframe coming in a PHY control message, then, it notifies the vPHY Tx threads that had less TBs to transmit, i.e., the ones that finished earlier.
          hypervisor_tx_notify_vphy_tx_threads_frame_mod_done(&vphy_tx_thread_context->vphy_tx_frame_done_semaphore);
        } else {
          // Otherwise, the current vPHY Tx thread has to wait until all other vPHY Tx threads have finished their work.
          sem_wait(&vphy_tx_thread_context->vphy_tx_frame_done_semaphore);
        }
      }

      // Print vPHY Tx statistics on screen.
      VPHY_TX_INFO_TIME("[Tx STATS]: vPHY Tx ID: %d - Tx slots: %d - PRB: %d - Channel: %d - Freq: %.2f [MHz] - MCS: %d - Freq. boost gain: %1.2f - CID: %d - Coding time: %f [ms]\n", vphy_tx_thread_context->vphy_id, slot_ctrl.nof_tb_in_slot, vphy_tx_thread_context->vphy_enodeb.nof_prb, slot_ctrl.ch, vphy_tx_get_channel_center_freq(vphy_tx_thread_context, slot_ctrl.ch)/1000000.0, slot_ctrl.mcs, slot_ctrl.freq_boost, vphy_tx_thread_context->vphy_enodeb.id, helpers_profiling_diff_time(coding_timespec));
    } else {
      VPHY_TX_ERROR("vPHY Tx ID: %d - Change of Tx parameters returned an error. Dropping MAC message.\n", vphy_tx_thread_context->vphy_id);
      number_of_dropped_packets++;
    }

    // Disarm watchdog timer for vPHY Tx thread.
#if(ENABLE_WATCHDOG_TIMER==1)
    if(timer_disarm(&vphy_tx_thread_context->vphy_tx_timer_id) < 0) {
      PHY_TX_ERROR("vPHY Tx ID: %d - Not possible to disarm the watchdog timer for vPHY Tx thread.\n", vphy_tx_thread_context->vphy_id);
    }
#endif

  }
  /****************************** vPHY Tx thread loop - END ******************************/

  VPHY_TX_DEBUG("vPHY Tx ID: %d - Leaving thread.\n",vphy_tx_thread_context->vphy_id);
  // Exit thread with result code.
  pthread_exit(NULL);
}

unsigned int vphy_tx_reverse_bits(register unsigned int x) {
  x = (((x & 0xaaaaaaaa) >> 1) | ((x & 0x55555555) << 1));
  x = (((x & 0xcccccccc) >> 2) | ((x & 0x33333333) << 2));
  x = (((x & 0xf0f0f0f0) >> 4) | ((x & 0x0f0f0f0f) << 4));
  x = (((x & 0xff00ff00) >> 8) | ((x & 0x00ff00ff) << 8));
  return((x >> 16) | (x << 16));
}

uint32_t vphy_tx_prbset_to_bitmask(uint32_t nof_prb) {
  uint32_t mask = 0;
  int nb = (int) ceilf((float) nof_prb / srslte_ra_type0_P(nof_prb));
  for(int i = 0; i < nb; i++) {
    if(i >= 0 && i < nb) {
      mask = mask | (0x1<<i);
    }
  }
  return vphy_tx_reverse_bits(mask)>>(32-nb);
}

// Functions to transfer slot control message from main thread to transmission thread.
void vphy_tx_push_tx_slot_control_to_container(uint32_t vphy_id, slot_ctrl_t* const slot_ctrl) {
  // Lock mutex so that we can push slot control to container.
  pthread_mutex_lock(&vphy_tx_threads[vphy_id]->vphy_tx_mutex);
  // Push slot control into container.
  tx_cb_push_back(vphy_tx_threads[vphy_id]->tx_handle, slot_ctrl);
#if(DEBUG_TX_CB_BUFFER==1)
  int size = tx_cb_size(vphy_tx_threads[vphy_id]->tx_handle);
  printf("[Tx ctrl/data buffer] [WR] vphy_id: %d - size: %d\n", vphy_id, size);
#endif
  // Unlock mutex so that function can do other things.
  pthread_mutex_unlock(&vphy_tx_threads[vphy_id]->vphy_tx_mutex);
  // Notify other thread that slot control was pushed into container.
  pthread_cond_signal(&vphy_tx_threads[vphy_id]->vphy_tx_cv);
}

void vphy_tx_pop_tx_slot_control_from_container(vphy_tx_thread_context_t* const vphy_tx_thread_context, slot_ctrl_t* const slot_ctrl) {
  // Lock mutex so that we can push slot control to container.
  pthread_mutex_lock(&vphy_tx_thread_context->vphy_tx_mutex);
  // Retrieve mapped element from container.
  tx_cb_front(vphy_tx_thread_context->tx_handle, slot_ctrl);
  tx_cb_pop_front(vphy_tx_thread_context->tx_handle);
  // Unlock mutex.
  pthread_mutex_unlock(&vphy_tx_thread_context->vphy_tx_mutex);
}

bool vphy_tx_wait_container_not_empty(vphy_tx_thread_context_t* const vphy_tx_thread_context) {
  bool ret = true;
  // Lock mutex so that we can wait for PHY control.
  pthread_mutex_lock(&vphy_tx_thread_context->vphy_tx_mutex);
  // Wait for conditional variable only if container is empty.
  if(tx_cb_empty(vphy_tx_thread_context->tx_handle)) {
    // Wait for conditional variable to be true.
    pthread_cond_wait(&(vphy_tx_thread_context->vphy_tx_cv), &(vphy_tx_thread_context->vphy_tx_mutex));
    if(!vphy_tx_thread_context->vphy_run_tx_thread) {
      ret = false;
    }
  }
  // Unlock mutex.
  pthread_mutex_unlock(&vphy_tx_thread_context->vphy_tx_mutex);
  return ret;
}

bool vphy_tx_wait_and_pop_tx_slot_control_from_container(vphy_tx_thread_context_t* const vphy_tx_thread_context, slot_ctrl_t* const slot_ctrl) {
  bool ret = true;
  // Lock mutex so that we can wait for PHY control.
  pthread_mutex_lock(&vphy_tx_thread_context->vphy_tx_mutex);
  // Wait for conditional variable only if container is empty.
  if(tx_cb_empty(vphy_tx_thread_context->tx_handle)) {
    // Wait for conditional variable to be true.
    pthread_cond_wait(&(vphy_tx_thread_context->vphy_tx_cv), &(vphy_tx_thread_context->vphy_tx_mutex));
    if(!vphy_tx_thread_context->vphy_run_tx_thread) {
      ret = false;
    }
  }
  // If still running, pop from container.
  if(ret) {
    // Retrieve mapped element from container.
    tx_cb_front(vphy_tx_thread_context->tx_handle, slot_ctrl);
    tx_cb_pop_front(vphy_tx_thread_context->tx_handle);
#if(DEBUG_TX_CB_BUFFER==1)
    int size = tx_cb_size(vphy_tx_thread_context->tx_handle);
    printf("[Tx ctrl/data buffer] [RD] vphy_id: %d - size: %d\n", slot_ctrl->vphy_id, size);
#endif
  }
  // Unlock mutex.
  pthread_mutex_unlock(&vphy_tx_thread_context->vphy_tx_mutex);
  return ret;
}

// Function used to get the container size.
int vphy_tx_get_tx_slot_control_container_size(uint32_t vphy_id) {
  int size = -1;
  // Lock mutex so that we can push slot control to container.
  pthread_mutex_lock(&vphy_tx_threads[vphy_id]->vphy_tx_mutex);
  // Get container size.
  size = tx_cb_size(vphy_tx_threads[vphy_id]->tx_handle);
  // Unlock mutex so that function can do other things.
  pthread_mutex_unlock(&vphy_tx_threads[vphy_id]->vphy_tx_mutex);
  return size;
}

double vphy_tx_get_channel_center_freq(vphy_tx_thread_context_t* const vphy_tx_thread_context, uint32_t channel) {
  return helpers_calculate_vphy_channel_center_frequency(vphy_tx_thread_context->radio_center_freq, vphy_tx_thread_context->radio_sampling_rate, vphy_tx_thread_context->vphy_sampling_rate, channel);
}

void vphy_tx_print_slot_control(slot_ctrl_t* const slot_ctrl) {
  printf("Send to: %d\n",slot_ctrl->send_to);
  printf("BW idx: %d\n",slot_ctrl->bw_idx);
  printf("Channel: %d\n",slot_ctrl->ch);
  printf("MCS: %d\n",slot_ctrl->mcs);
  printf("Frequency boost: \n",slot_ctrl->freq_boost);
  printf("Data length: %d\n",slot_ctrl->length);
  printf("Data ptr: %x\n",slot_ctrl->data);
}

timer_t* vphy_tx_get_timer_id(uint32_t vphy_id) {
  return &vphy_tx_threads[vphy_id]->vphy_tx_timer_id;
}

// ****************** Functions for Frame structure type II ********************
int vphy_tx_validate_tb_size(slot_ctrl_t* const slot_ctrl, uint32_t bw_idx) {
  // Change MCS. For 1.4 MHz we can not set MCS 28 for the very first subframe once the code rate is greater than 1.
  if(slot_ctrl->bw_idx == BW_IDX_OneDotFour && slot_ctrl->mcs >= 28) {
    // Check size.
    int length = slot_ctrl->length - communicator_get_tb_size(bw_idx, slot_ctrl->mcs-1);
    if(length > 0 && length % communicator_get_tb_size(bw_idx, slot_ctrl->mcs) != 0) {
      PHY_TX_ERROR("vPHY Tx ID: %d - Data length set by MAC to subsequent subframes is invalid. Length field in Basic control command: %d - expected size: %d\n", slot_ctrl->vphy_id, slot_ctrl->length, communicator_get_tb_size(bw_idx, slot_ctrl->mcs));
      return -1;
    }
    // Check if expected TB size is not bigger than length field in basic control message.
    if(length < 0) {
      PHY_TX_ERROR("vPHY Tx ID: %d - Data length set by MAC is invalid (negative). Length field in Basic control command: %d - expected size: %d\n", slot_ctrl->vphy_id, slot_ctrl->length, communicator_get_tb_size(bw_idx, slot_ctrl->mcs));
      return -1;
    }
  } else {
    // Check size.
    if(slot_ctrl->length % communicator_get_tb_size(bw_idx, slot_ctrl->mcs) != 0) {
      PHY_TX_ERROR("vPHY Tx ID: %d - Data length set by MAC is invalid. Length field in Basic control command: %d - expected size: %d\n", slot_ctrl->vphy_id, slot_ctrl->length, communicator_get_tb_size(bw_idx, slot_ctrl->mcs));
      return -1;
    }
  }

  return 0;
}

void vphy_tx_change_allocation(vphy_tx_thread_context_t* const vphy_tx_thread_context, uint32_t req_mcs, uint32_t req_bw_idx) {
  // Change MCS only if MCS or BW have changed.
  if(vphy_tx_thread_context->last_tx_slot_control.mcs != req_mcs || vphy_tx_thread_context->last_tx_slot_control.bw_idx != req_bw_idx) {
    // Update allocation with number of resource blocks and MCS.
    vphy_tx_update_radl(vphy_tx_thread_context, req_mcs, vphy_tx_thread_context->vphy_enodeb.nof_prb);
    // Update last TX basic control structure.
    vphy_tx_thread_context->last_tx_slot_control.mcs = req_mcs;
    vphy_tx_thread_context->last_tx_slot_control.bw_idx = req_bw_idx;
    VPHY_TX_DEBUG_TIME("vPHY Tx ID: %d - MCS set to: %d - Tx BW index set to: %d\n", vphy_tx_thread_context->vphy_id, req_mcs, req_bw_idx);
  }
}
