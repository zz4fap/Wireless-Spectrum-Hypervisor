#include "hypervisor_rx.h"

// *********** Global variables ***********
static hypervisor_rx_t* hypervisor_rx_handle = NULL;

extern int errno;

// *********** Definition of functions ***********
int hypervisor_rx_initialize(transceiver_args_t* const args, srslte_rf_t* const rf) {
  int ret;
  // Allocate memory for a hypervisor Rx object;
  hypervisor_rx_handle = (hypervisor_rx_t*)srslte_vec_malloc(sizeof(hypervisor_rx_t));
  if(hypervisor_rx_handle == NULL) {
    HYPER_RX_ERROR("Error when allocating memory for hypervisor_rx_handle\n", 0);
    return -1;
  }
  // Initialize structure with zeros.
  bzero(hypervisor_rx_handle, sizeof(hypervisor_rx_t));
  // Initialize context with default or command line values.
  hypervisor_rx_handle->initial_rx_gain         = args->initial_rx_gain;
  hypervisor_rx_handle->hypervisor_rx_rf        = rf;
  hypervisor_rx_handle->use_std_carrier_sep     = args->use_std_carrier_sep;
  hypervisor_rx_handle->run_channelizer_thread  = true;
  hypervisor_rx_handle->num_of_rx_vphys         = args->num_of_rx_vphys;
  hypervisor_rx_handle->vphy_rx_running         = false;
  // Calculate number of USRP samples to read.
  hypervisor_rx_handle->nof_samples_to_read     = (uint32_t)(0.001*helpers_get_bw_from_nprb(args->radio_nof_prb));
  // Set all channels in the list to zero.
  bzero(hypervisor_rx_handle->channel_list, MAX_NUM_CONCURRENT_VPHYS*sizeof(uint32_t));
  // Initialize last configured values.
  hypervisor_rx_init_last_hypervisor_ctrl(args);
  // Initialize mutex.
  if(pthread_mutex_init(&hypervisor_rx_handle->hypervisor_rx_radio_params_mutex, NULL) != 0) {
    HYPER_RX_ERROR("Initialization of mutex for accessing Rx radio parameters failed.\n",0);
    return -1;
  }
  // Initialize mutex.
  if(pthread_mutex_init(&hypervisor_rx_handle->hypervisor_rx_channel_list_mutex, NULL) != 0) {
    HYPER_RX_ERROR("Initialization of mutex for channel list access failed.\n",0);
    return -1;
  }

#if(ENABLE_USRP_READ_SAMPLES_THREAD==1)
  // Allocate memory for USRP read buffer.
  for(uint32_t i = 0; i < NOF_USRP_READ_BUFFERS; i++) {
    hypervisor_rx_handle->usrp_read_buffer[i] = (cf_t*)srslte_vec_malloc(sizeof(cf_t)*hypervisor_rx_handle->nof_samples_to_read);
    if(hypervisor_rx_handle->usrp_read_buffer[i] == NULL) {
      HYPER_RX_ERROR("Error allocating memory for USRP read buffer.\n", 0);
      return -1;
    }
    // Intialize USRP buffer memory.
    bzero(hypervisor_rx_handle->usrp_read_buffer[i], sizeof(cf_t)*hypervisor_rx_handle->nof_samples_to_read);
  }
  // Initialize USRP read mutex.
  if(pthread_mutex_init(&hypervisor_rx_handle->usrp_read_mutex, NULL) != 0) {
    HYPER_RX_ERROR("Initialization of USRP read mutex failed.\n",0);
    return -1;
  }
  // Initialize USRP read conditional variable.
  if(pthread_cond_init(&hypervisor_rx_handle->usrp_read_cond_var, NULL)) {
    HYPER_RX_ERROR("Initialization of USRP read conditional variable failed.\n",0);
    return -1;
  }
  // Instantiate USRP read circular buffer object.
  input_buffer_ctx_cb_make(&hypervisor_rx_handle->usrp_read_cb_buffer_handle, NOF_USRP_READ_BUFFERS);
  // Reset USRP read samples write counter.
  hypervisor_rx_handle->usrp_read_samples_buffer_wr_cnt = 0;
  // Create complex waveform for shifting signal back to the frequencies expected by the channelizer.
  #if(ENABLE_FREQ_SHIFT_CORRECTION==1 && ENABLE_LOCAL_RX_FREQ_CORRECTION==1)
    // Instantiate complex waveform object.
    srslte_cexptab_init_finer(&hypervisor_rx_handle->freq_shift_waveform_obj, srslte_symbol_sz(args->radio_nof_prb));
    // Allocate memory for frequency-shift waveform signal.
    hypervisor_rx_handle->freq_shift_waveform = (cf_t*)srslte_vec_malloc(hypervisor_rx_handle->nof_samples_to_read*sizeof(cf_t));
    if(hypervisor_rx_handle->freq_shift_waveform == NULL) {
      HYPER_RX_ERROR("Error when allocating memory for frequency shift signal array.\n", 0);
      return -1;
    }
    // Generate frequency-shift waveform signal array.
    float freq = ((float)srslte_symbol_sz(args->nof_prb)/2)/srslte_symbol_sz(args->radio_nof_prb);
    srslte_cexptab_gen_finer(&hypervisor_rx_handle->freq_shift_waveform_obj, hypervisor_rx_handle->freq_shift_waveform, freq, hypervisor_rx_handle->nof_samples_to_read);
  #endif
#endif

#if(ENABLE_CHANNELIZER_THREAD==1)
  // Instantiate resources related to the channelizer.
  for(uint32_t i = 0; i < args->num_of_rx_vphys; i++) {
    // Instantiate a channelizer circular buffer object for each vPHY.
    channel_buffer_ctx_cb_make(&hypervisor_rx_handle->channelizer_cb_buffer_handle[i], NOF_CHANNELIZER_BUFFERS);
    // Initialize channelizer buffer mutex for each vPHY.
    if(pthread_mutex_init(&hypervisor_rx_handle->channelizer_mutex[i], NULL) != 0) {
      HYPER_RX_ERROR("Initialization of channelizer mutex failed.\n",0);
      return -1;
    }
    // Initialize channelizer conditional variable for each vPHY.
    if(pthread_cond_init(&hypervisor_rx_handle->channelizer_cond_var[i], NULL)) {
      HYPER_RX_ERROR("Initialization of channelizer conditional variable failed.\n",0);
      return -1;
    }
  }
  // Initialize channelizer structures and parameters.
  hypervisor_rx_initialize_channelizer(args);
#endif

  // Initialize plot object only if enabled.
#ifdef ENABLE_PLOT_RX_SPECTRUM
  // Initialize spectrum plot object.
  if(init_spectrum_plot_object(srslte_symbol_sz(args->radio_nof_prb), hypervisor_rx_handle->nof_samples_to_read, PLOT_WATERFALL, hypervisor_rx_handle->usrp_read_buffer) < 0) {
    HYPER_RX_ERROR("Error initializing spectrum plot object.\n", 0);
    return -1;
  }
#endif

#if(ENABLE_CHANNELIZER_THREAD==1)
  // Start channelizer thread fisrt then the one to read USRP samples.
  // Set structures for channelizer thread management.
  pthread_attr_init(&hypervisor_rx_handle->channelizer_thread_attr);
  pthread_attr_setdetachstate(&hypervisor_rx_handle->channelizer_thread_attr, PTHREAD_CREATE_JOINABLE);
  // Spawn channelizer thread.
  ret = pthread_create(&hypervisor_rx_handle->channelizer_thread_id, &hypervisor_rx_handle->channelizer_thread_attr, hypervisor_rx_channelizer_work, (void*)hypervisor_rx_handle);
  if(ret) {
    HYPER_RX_ERROR("Error spwaning channelizer thread. Error: %d\n", ret);
    return -1;
  }
#endif

#if(ENABLE_USRP_READ_SAMPLES_THREAD==1)
  // Set Radio Rx sample rate accoring to the number of PRBs.
  hypervisor_rx_set_sample_rate(args->radio_nof_prb, args->use_std_carrier_sep);
  // Configure initial Radio Rx central frequency and gain. Central frequency is set to the competition center frequency.
  hypervisor_rx_set_radio_center_freq_and_gain(args);
  // Try to stop Rx stream if that is open, flush reception buffer and open Rx stream.
  hypervisor_rx_initialize_stream();
  // Set structures for management of USRP read samples thread.
  pthread_attr_init(&hypervisor_rx_handle->usrp_read_thread_attr);
  pthread_attr_setdetachstate(&hypervisor_rx_handle->usrp_read_thread_attr, PTHREAD_CREATE_JOINABLE);
  // Spawn USRP read samples thread.
  ret = pthread_create(&hypervisor_rx_handle->usrp_read_thread_id, &hypervisor_rx_handle->usrp_read_thread_attr, hypervisor_rx_usrp_read_work, (void*)hypervisor_rx_handle);
  if(ret) {
    HYPER_RX_ERROR("Error spwaning USRP read samples thread. Error: %d\n", ret);
    return -1;
  }
#endif

  return 0;
}

int hypervisor_rx_uninitialize() {
  int ret = 0;
  // Free spectrum plot object.
#ifdef ENABLE_PLOT_RX_SPECTRUM
  // Destroy all plot related resources.
  if(free_spectrum_plot_object() < 0) {
    HYPER_RX_ERROR("Error freeing plot object.\n", 0);
    return -1;
  }
#endif
  // Uninitialize threads below.
  hypervisor_rx_handle->run_channelizer_thread = false;
#if(ENABLE_CHANNELIZER_THREAD==1)
  // Destroy and join channelizer thread.
  pthread_attr_destroy(&hypervisor_rx_handle->channelizer_thread_attr);
  ret = pthread_join(hypervisor_rx_handle->channelizer_thread_id, NULL);
  if(ret) {
    HYPER_RX_ERROR("Error joining channelizer thread. Error: %d\n", ret);
    return -1;
  }
  // Uninitialize channelizer structures and parameters.
  hypervisor_rx_uninitialize_channelizer();
  // Free resources related to channelizer thread.
  for(uint32_t i = 0; i < hypervisor_rx_handle->num_of_rx_vphys; i++) {
    // Destroy channelizer mutex.
    pthread_mutex_destroy(&hypervisor_rx_handle->channelizer_mutex[i]);
    // Destory channelizer conditional variable.
    if(pthread_cond_destroy(&hypervisor_rx_handle->channelizer_cond_var[i]) != 0) {
      HYPER_RX_ERROR("Channelizer conditional variable destruction failed.\n",0);
      return -1;
    }
    // Free channelizer circular buffer object.
    channel_buffer_ctx_cb_free(&hypervisor_rx_handle->channelizer_cb_buffer_handle[i]);
  }
#endif

#if(ENABLE_USRP_READ_SAMPLES_THREAD==1)
  // Destroy and join USRP read samples thread.
  pthread_attr_destroy(&hypervisor_rx_handle->usrp_read_thread_attr);
  ret = pthread_join(hypervisor_rx_handle->usrp_read_thread_id, NULL);
  if(ret) {
    HYPER_RX_ERROR("Error joining USRP read samples thread. Error: %d\n", ret);
    return -1;
  }
  // Stop Rx stream and flush reception buffer.
  hypervisor_rx_stop_stream_and_flush_buffer();
  // Destroy mutex for input IQ samples read buffer access.
  pthread_mutex_destroy(&hypervisor_rx_handle->usrp_read_mutex);
  // Destory conditional variable for input IQ samples read buffer access.
  if(pthread_cond_destroy(&hypervisor_rx_handle->usrp_read_cond_var) != 0) {
    HYPER_RX_ERROR("USRP read samples conditional variable destruction failed.\n",0);
    return -1;
  }
  // Free circular buffer holding input IQ samples buffer context structures.
  input_buffer_ctx_cb_free(&hypervisor_rx_handle->usrp_read_cb_buffer_handle);
  // Free USRP read buffer allocated memory.
  for(uint32_t i = 0; i < NOF_USRP_READ_BUFFERS; i++) {
    if(hypervisor_rx_handle->usrp_read_buffer[i]) {
      free(hypervisor_rx_handle->usrp_read_buffer[i]);
      hypervisor_rx_handle->usrp_read_buffer[i] = NULL;
    }
  }
  // Free frequency-shift waveform.
  #if(ENABLE_FREQ_SHIFT_CORRECTION==1 && ENABLE_LOCAL_RX_FREQ_CORRECTION==1)
  srslte_cexptab_free_finer(&hypervisor_rx_handle->freq_shift_waveform_obj);
  // Free frequency-shift waveform signal.
  if(hypervisor_rx_handle->freq_shift_waveform) {
    free(hypervisor_rx_handle->freq_shift_waveform);
    hypervisor_rx_handle->freq_shift_waveform = NULL;
  }
  #endif
#endif
  // Destroy radio parameters mutex.
  pthread_mutex_destroy(&hypervisor_rx_handle->hypervisor_rx_radio_params_mutex);
  // Destroy channel list mutex.
  pthread_mutex_destroy(&hypervisor_rx_handle->hypervisor_rx_channel_list_mutex);
  // Free memory used to store hypervisor Rx object.
  if(hypervisor_rx_handle) {
    free(hypervisor_rx_handle);
    hypervisor_rx_handle = NULL;
  }
  // Everything went well.
  return 0;
}

// Initialize structure with last configured Rx slot control.
void hypervisor_rx_init_last_hypervisor_ctrl(transceiver_args_t* const args) {
  hypervisor_rx_handle->last_rx_hypervisor_ctrl.radio_center_frequency  = args->radio_center_frequency;
  hypervisor_rx_handle->last_rx_hypervisor_ctrl.radio_nof_prb           = args->radio_nof_prb;
  hypervisor_rx_handle->last_rx_hypervisor_ctrl.vphy_nof_prb            = args->nof_prb;
  hypervisor_rx_handle->last_rx_hypervisor_ctrl.rf_boost                = args->rf_boost;
  hypervisor_rx_set_last_gain(args->initial_rx_gain);
}

// Set Radio Rx sampling rate.
int hypervisor_rx_set_sample_rate(uint32_t nof_prb, bool use_std_carrier_sep) {
  int srate = -1;
  if(use_std_carrier_sep) {
    srate = srslte_sampling_freq_hz(nof_prb);
  } else {
    srate = helpers_non_std_sampling_freq_hz(nof_prb);
    HYPER_RX_PRINT("Setting a non-standard sampling rate: %1.2f [MHz]\n",srate/1000000.0);
  }
  if(srate != -1) {
    float srate_rf = srslte_rf_set_rx_srate(hypervisor_rx_handle->hypervisor_rx_rf, (double)srate, PHY_CHANNEL);
    if(srate_rf != srate) {
      HYPER_RX_ERROR("Could not set Rx sampling rate.\n",0);
      return -1;
    }
    HYPER_RX_PRINT("Set Rx sampling rate to: %.2f [MHz]\n", srate_rf/1000000.0);
  } else {
    HYPER_RX_ERROR("Invalid number of PRB (Rx): %d\n", nof_prb);
    return -1;
  }
  // Everything went well.
  return 0;
}

// Set Radio center frequency and gain of Radio Rx.
void hypervisor_rx_set_radio_center_freq_and_gain(transceiver_args_t* const args) {
  // Set Radio Rx gain.
  hypervisor_rx_set_gain(args->initial_rx_gain);
  // Set Radio Rx center frequency.
  hypervisor_rx_set_center_frequency(args->radio_center_frequency);
}

float hypervisor_rx_set_gain(float rx_gain) {
  double current_rx_gain = -1.0;
  // Set Radio Rx gain.
  if(rx_gain >= 0.0) {
    current_rx_gain = srslte_rf_set_rx_gain(hypervisor_rx_handle->hypervisor_rx_rf, rx_gain, PHY_CHANNEL);
    HYPER_RX_INFO("Set Rx gain to: %.1f [dB].\n",current_rx_gain);
  } else {
    HYPER_RX_ERROR("Rx gain must be greater than or equal to 0, current value: %f\n", rx_gain);
    return -1.0;
  }
  return current_rx_gain;
}

void hypervisor_rx_set_center_frequency(double rx_center_frequency) {
#if(ENABLE_HW_RF_MONITOR==1)
  // Always apply offset when HW RF Monitor is enabled.
  double lo_offset = (double)PHY_RX_LO_OFFSET;
#else
  // If using default FPGA image, then check number of channels.
  double lo_offset = (hypervisor_rx_handle->hypervisor_rx_rf)->num_of_channels == 1 ? 0.0:(double)PHY_RX_LO_OFFSET;
#endif
  // Set central frequency for reception.
  double current_rx_freq = srslte_rf_set_rx_freq2(hypervisor_rx_handle->hypervisor_rx_rf, rx_center_frequency, lo_offset, PHY_CHANNEL);
  // Check if actual frequency is inside a range of +/- 10 Hz.
  if(current_rx_freq < (rx_center_frequency - 10.0) || current_rx_freq > (rx_center_frequency + 10.0)) {
     HYPER_RX_ERROR("Requested frequency: %1.2f [MHz] - Actual frequency: %1.2f [MHz]\n", rx_center_frequency/1000000.0, current_rx_freq/1000000.0);
  }
  srslte_rf_rx_wait_lo_locked(hypervisor_rx_handle->hypervisor_rx_rf, PHY_CHANNEL);
  HYPER_RX_PRINT("Set Rx central frequency to: %.2f [MHz] with offset of: %.2f [MHz]\n", (current_rx_freq/1000000.0),(lo_offset/1000000.0));
}

int hypervisor_rx_initialize_stream() {
  int error;
  // Stop Rx stream and flush Reception Buffer.
  hypervisor_rx_stop_stream_and_flush_buffer();
  // Start Rx stream.
  if((error = srslte_rf_start_rx_stream(hypervisor_rx_handle->hypervisor_rx_rf, PHY_CHANNEL)) != 0) {
    HYPER_RX_ERROR("Error starting rx stream: %d....\n",error);
    return -1;
  }
  // Everything went well.
  return 0;
}

// Stop Rx stream and flush reception buffer.
int hypervisor_rx_stop_stream_and_flush_buffer() {
  int error;
  if((error = srslte_rf_stop_rx_stream(hypervisor_rx_handle->hypervisor_rx_rf, PHY_CHANNEL)) != 0) {
    HYPER_RX_ERROR("Error stopping rx stream: %d....\n",error);
    return -1;
  }
  srslte_rf_flush_buffer(hypervisor_rx_handle->hypervisor_rx_rf, PHY_CHANNEL);
  // Everything went well.
  return 0;
}

void hypervisor_rx_change_parameters(hypervisor_ctrl_t *hypervisor_ctrl) {
  // Change Rx frequency if the requested one is different from last one.
  if(hypervisor_rx_handle->last_rx_hypervisor_ctrl.radio_center_frequency != hypervisor_ctrl->radio_center_frequency) {
    hypervisor_rx_set_center_frequency(hypervisor_ctrl->radio_center_frequency);
    hypervisor_rx_handle->last_rx_hypervisor_ctrl.radio_center_frequency = hypervisor_ctrl->radio_center_frequency;
  }
  // Change Rx gain if the requested one is different from last one.
  if(hypervisor_rx_get_last_gain() != hypervisor_ctrl->gain) {
    hypervisor_rx_set_gain(hypervisor_ctrl->gain);
    hypervisor_rx_set_last_gain(hypervisor_ctrl->gain);
  }
  // Change Rx sampling rate if the requested one is different from last one.
  if(hypervisor_rx_handle->last_rx_hypervisor_ctrl.radio_nof_prb != hypervisor_ctrl->radio_nof_prb) {
    // Sample rate must be the one used to transmit several vPHYS concurrently.
    hypervisor_rx_set_sample_rate(hypervisor_ctrl->radio_nof_prb, hypervisor_rx_handle->use_std_carrier_sep);
    hypervisor_rx_handle->last_rx_hypervisor_ctrl.radio_nof_prb = hypervisor_ctrl->radio_nof_prb;
  }
  // Change vPHY number of PRB if the requested one is different from last one.
  if(hypervisor_rx_handle->last_rx_hypervisor_ctrl.vphy_nof_prb != hypervisor_ctrl->vphy_nof_prb) {
    hypervisor_rx_handle->last_rx_hypervisor_ctrl.vphy_nof_prb = hypervisor_ctrl->vphy_nof_prb;
  }
}

//******************************************************************************
// ----------------------- USRP Read Samples (BEGIN) ---------------------------
//******************************************************************************
#if(ENABLE_USRP_READ_SAMPLES_THREAD==1)
// Thread reading samples from USRP and adding to buffer.
void *hypervisor_rx_usrp_read_work(void *h) {

  // Cast handle to hypervisor Rx context.
  hypervisor_rx_t *hyper_rx_handle_ptr = (hypervisor_rx_t*)h;
  int32_t nof_read_samples = 0;
  bool enable_dropping = false;

  // Set priority to USRP read samples thread.
  uhd_set_thread_priority(1.0, true);

#if(ENABLE_STICKING_THREADS_TO_CORES==1)
  int ret = helpers_stick_this_thread_to_core(25);
  if(ret != 0) {
    HYPER_RX_ERROR("--------> Set USRP read thread to CPU ret: %d\n", ret);
  }
#endif

  HYPER_RX_PRINT("nof USRP samples to read: %d\n", hyper_rx_handle_ptr->nof_samples_to_read);

  // USRP read samples loop.
  while(hyper_rx_handle_ptr->run_channelizer_thread) {

#if(MEASURE_USRP_BUFFER_FULL_TIME==1)
    static struct timespec rd_timestamp;
    if(hyper_rx_handle_ptr->usrp_read_samples_buffer_wr_cnt == 0) {
      clock_gettime(CLOCK_REALTIME, &rd_timestamp);
    }
#endif

    // Read IQ Samples either from USRP or from channel emulator.
    nof_read_samples = hypervisor_rx_radio_recv(hyper_rx_handle_ptr, hyper_rx_handle_ptr->usrp_read_buffer[hyper_rx_handle_ptr->usrp_read_samples_buffer_wr_cnt], hyper_rx_handle_ptr->nof_samples_to_read, true, NULL, NULL, PHY_CHANNEL);
#ifndef ENABLE_CH_EMULATOR
    if(nof_read_samples <= 0) {
      HYPER_RX_ERROR("Problem reading USRP samples: %d\n", nof_read_samples);
    }
#endif

    if(nof_read_samples > 0 && hyper_rx_handle_ptr->vphy_rx_running) {

#if(WRITE_RX_USRP_READ_INTO_FILE==1)
      static unsigned int dump_cnt = 0;
      char output_file_name[200];
      sprintf(output_file_name, "rx_side_usrp_read_assessment_%d.dat",dump_cnt);
      srslte_filesink_t file_sink;
      if(dump_cnt < 10) {
        filesink_init(&file_sink, output_file_name, SRSLTE_COMPLEX_FLOAT_BIN);
        // Write samples into file.
        filesink_write(&file_sink, hyper_rx_handle_ptr->usrp_read_buffer[hyper_rx_handle_ptr->usrp_read_samples_buffer_wr_cnt], hyper_rx_handle_ptr->nof_samples_to_read);
        // Close file.
        filesink_free(&file_sink);
        dump_cnt++;
        HYPER_RX_PRINT("File dumped: %d.\n",dump_cnt);
      }
#endif

#if(ENABLE_FREQ_SHIFT_CORRECTION==1 && ENABLE_LOCAL_RX_FREQ_CORRECTION==1)
      srslte_vec_prod_ccc(hypervisor_rx_handle->freq_shift_waveform, hyper_rx_handle_ptr->usrp_read_buffer[hyper_rx_handle_ptr->usrp_read_samples_buffer_wr_cnt], hyper_rx_handle_ptr->usrp_read_buffer[hyper_rx_handle_ptr->usrp_read_samples_buffer_wr_cnt], hyper_rx_handle_ptr->nof_samples_to_read);

      #if(WRITE_RX_FREQ_SHIFTED_USRP_READ_INTO_FILE==1)
        static unsigned int dump_cnt2 = 0;
        char output_file_name2[200];
        sprintf(output_file_name2, "rx_side_freq_shifted_usrp_read_assessment_%d.dat",dump_cnt2);
        srslte_filesink_t file_sink2;
        if(dump_cnt2 < 10) {
          filesink_init(&file_sink2, output_file_name2, SRSLTE_COMPLEX_FLOAT_BIN);
          // Write samples into file.
          filesink_write(&file_sink2, hyper_rx_handle_ptr->usrp_read_buffer[hyper_rx_handle_ptr->usrp_read_samples_buffer_wr_cnt], hyper_rx_handle_ptr->nof_samples_to_read);
          // Close file.
          filesink_free(&file_sink2);
          dump_cnt2++;
          HYPER_RX_PRINT("File dumped: %d.\n",dump_cnt2);
        }
      #endif
#endif

      // Push the input IQ sample counter to the container.
      enable_dropping = hypervisor_rx_push_usrp_read_buffer_context_to_container_with_dropping(hyper_rx_handle_ptr->usrp_read_samples_buffer_wr_cnt);

#if(MEASURE_USRP_BUFFER_FULL_TIME==1)
      if(hyper_rx_handle_ptr->usrp_read_samples_buffer_wr_cnt == (NOF_USRP_READ_BUFFERS-1)) {
        PHY_PROFILLING_AVG5("Avg. USRP read time: %f [ms] - min: %f [ms] - max: %f [ms] - max counter %d - diff >= 5000 [ms]: %d - total counter: %d - perc: %f\n", helpers_profiling_diff_time(rd_timestamp), 5000.0, 1);
      }
#endif

#if(DEBUG_USRP_READ_BUFFER_COUNTER==1)
      printf("USRP CB size: %d - usrp_read_samples_buffer_wr_cnt: %d\n", input_buffer_ctx_cb_size(hyper_rx_handle_ptr->usrp_read_cb_buffer_handle), hyper_rx_handle_ptr->usrp_read_samples_buffer_wr_cnt);
#endif

      // Increment IQ samples input buffer counter if not dropping buffers.
      if(!enable_dropping) {
        hyper_rx_handle_ptr->usrp_read_samples_buffer_wr_cnt = (hyper_rx_handle_ptr->usrp_read_samples_buffer_wr_cnt + 1) % NOF_USRP_READ_BUFFERS;
      }

    }

  } // End of USRP read samples loop.

  HYPER_RX_DEBUG("Leaving USRP read samples thread.\n", 0);
  // Exit thread with result code NULL.
  pthread_exit(NULL);
}

// Push input IQ samples buffer context into container.
void hypervisor_rx_push_usrp_read_buffer_context_to_container(uint32_t usrp_read_samples_buffer_wr_cnt) {
  // Lock mutex so that we can push slot control to container.
  pthread_mutex_lock(&hypervisor_rx_handle->usrp_read_mutex);
  // Push input buffer read counter into circular buffer.
  input_buffer_ctx_cb_push_back(hypervisor_rx_handle->usrp_read_cb_buffer_handle, usrp_read_samples_buffer_wr_cnt);
#if(DEBUG_USRP_READ_BUFFER_COUNTER==1)
  printf("---------------------------> [USRP buffer WR] - Size: %d - CB Back: %d - CB front: %d\n", input_buffer_ctx_cb_size(hypervisor_rx_handle->usrp_read_cb_buffer_handle), usrp_read_samples_buffer_wr_cnt, input_buffer_ctx_cb_front(hypervisor_rx_handle->usrp_read_cb_buffer_handle));
#endif
  // Unlock mutex so that function can do other things.
  pthread_mutex_unlock(&hypervisor_rx_handle->usrp_read_mutex);
  // Notify other thread that input IQ sample counter was pushed into container.
  pthread_cond_signal(&hypervisor_rx_handle->usrp_read_cond_var);
}

// Push input IQ samples buffer context into container if the size is not bigger than the maximum.
bool hypervisor_rx_push_usrp_read_buffer_context_to_container_with_dropping(uint32_t usrp_read_samples_buffer_wr_cnt) {
  // Flag controlling the dropping state.
  static bool enable_dropping = false;

  // Lock mutex so that we can push slot control to container.
  pthread_mutex_lock(&hypervisor_rx_handle->usrp_read_mutex);

  // Verify if dropping is to be enabled or not.
  int size = input_buffer_ctx_cb_size(hypervisor_rx_handle->usrp_read_cb_buffer_handle);
  if(enable_dropping == false && size >= (NOF_USRP_READ_BUFFERS-10)) {
    // Enable dropping until container size is below or equal to 100.
    enable_dropping = true;
  } else if(enable_dropping == true && size <= 100) {
    // Disable dropping.
    enable_dropping = false;
  }

  // If dropping is not enabled, then push reader index into circular buffer.
  if(!enable_dropping) {
    // Push input buffer read counter into circular buffer.
    input_buffer_ctx_cb_push_back(hypervisor_rx_handle->usrp_read_cb_buffer_handle, usrp_read_samples_buffer_wr_cnt);
    // If plotting is enabled, then we push current index into the circular buffer.
#ifdef ENABLE_PLOT_RX_SPECTRUM
    spectrum_plot_push_buffer_index(usrp_read_samples_buffer_wr_cnt);
#endif
  }

#if(DEBUG_USRP_READ_BUFFER_COUNTER==1)
  printf("[USRP buffer drop] enable_dropping: %d - usrp_read_samples_buffer_wr_cnt: %d\n", enable_dropping, usrp_read_samples_buffer_wr_cnt);
  if(!enable_dropping) {
    printf("---------------------------> [USRP buffer WR] - Size: %d - CB Back: %d - CB front: %d\n", input_buffer_ctx_cb_size(hypervisor_rx_handle->usrp_read_cb_buffer_handle), usrp_read_samples_buffer_wr_cnt, input_buffer_ctx_cb_front(hypervisor_rx_handle->usrp_read_cb_buffer_handle));
  } else {
    printf("######################################### Dropping USRP read samples buffer write. Size: %d\n", input_buffer_ctx_cb_size(hypervisor_rx_handle->usrp_read_cb_buffer_handle));
  }
#endif

  // Unlock mutex so that function can do other things.
  pthread_mutex_unlock(&hypervisor_rx_handle->usrp_read_mutex);
  // Notify other thread that input IQ sample counter was pushed into container.
  if(!enable_dropping) {
    pthread_cond_signal(&hypervisor_rx_handle->usrp_read_cond_var);
  }

  return enable_dropping;
}

// Check if container is not empty, if so, wait until it is not empty and get the context, incrementing the counter.
bool hypervisor_rx_timedwait_and_get_usrp_buffer_context(uint32_t* usrp_read_samples_buffer_rd_cnt) {
  bool ret = true, is_cb_empty = true;
  struct timespec timeout;
  // Lock mutex.
  pthread_mutex_lock(&hypervisor_rx_handle->usrp_read_mutex);
  // Wait for conditional variable only if container is empty.
  if(input_buffer_ctx_cb_empty(hypervisor_rx_handle->usrp_read_cb_buffer_handle)) {
    do {
      // Timeout in 0.5 ms.
      helpers_get_timeout(50000, &timeout);
      // Timed wait for conditional variable to be true.
      pthread_cond_timedwait(&hypervisor_rx_handle->usrp_read_cond_var, &hypervisor_rx_handle->usrp_read_mutex, &timeout);
      // Check status of the circular buffer again.
      is_cb_empty = input_buffer_ctx_cb_empty(hypervisor_rx_handle->usrp_read_cb_buffer_handle);
      // Check if the threads are still running, if not, then leave with false.
      if(!hypervisor_rx_handle->run_channelizer_thread || !phy_is_running()) {
        ret = false;
      }
    } while(is_cb_empty && ret);
  }

  // Only retrieve the context if the thread is still running.
  if(ret) {
    // Retrieve input IQ samples buffer context element from circular buffer.
    *usrp_read_samples_buffer_rd_cnt = input_buffer_ctx_cb_front(hypervisor_rx_handle->usrp_read_cb_buffer_handle);
    // Remove input IQ samples buffer contextelement from  circular buffer.
    input_buffer_ctx_cb_pop_front(hypervisor_rx_handle->usrp_read_cb_buffer_handle);
  }

#if(DEBUG_USRP_READ_BUFFER_COUNTER==1)
  if(ret) {
    printf("<--------------------------- [USRP buffer RD] - Size: %d - Read: %d - CB Front: %d\n", input_buffer_ctx_cb_size(hypervisor_rx_handle->usrp_read_cb_buffer_handle), *usrp_read_samples_buffer_rd_cnt, input_buffer_ctx_cb_front(hypervisor_rx_handle->usrp_read_cb_buffer_handle));
  }
#endif

  // Unlock mutex.
  pthread_mutex_unlock(&hypervisor_rx_handle->usrp_read_mutex);
  return ret;
}

#endif
//******************************************************************************
// ----------------------- USRP Read Samples (END) ---------------------------
//******************************************************************************

//******************************************************************************
// --------------------------- Channelizer (BEGIN) -----------------------------
//******************************************************************************
#if(ENABLE_CHANNELIZER_THREAD==1)
void hypervisor_rx_initialize_channelizer(transceiver_args_t* const args) {
  // Calculate number of channels.
  hypervisor_rx_handle->channelizer_nof_channels = (uint32_t)(helpers_get_bw_from_nprb(args->radio_nof_prb)/helpers_get_bw_from_nprb(args->nof_prb));
  // Calculate number of frames.
  hypervisor_rx_handle->channelizer_nof_frames = hypervisor_rx_handle->nof_samples_to_read/hypervisor_rx_handle->channelizer_nof_channels;
  // Filter delay.
  hypervisor_rx_handle->channelizer_filter_delay = args->channelizer_filter_delay;
  // Stop-band attenuation
  hypervisor_rx_handle->channelizer_stop_band_att = 60;

  // Create prototype filter.
  unsigned int h_len = 2*hypervisor_rx_handle->channelizer_nof_channels*hypervisor_rx_handle->channelizer_filter_delay + 1;
  float h[h_len];
  liquid_firdes_kaiser(h_len, 0.5f/(float)hypervisor_rx_handle->channelizer_nof_channels, hypervisor_rx_handle->channelizer_stop_band_att, 0.0f, h);

  // Create filterbank channelizer object using external filter coefficients.
  hypervisor_rx_handle->channelizer = firpfbch_crcf_create(LIQUID_ANALYZER, hypervisor_rx_handle->channelizer_nof_channels, 2*hypervisor_rx_handle->channelizer_filter_delay, h);

  // Allocate memory for output buffers.
  for(uint32_t i = 0; i < NOF_CHANNELIZER_BUFFERS; i++) {
    hypervisor_rx_handle->channelizer_buffer[i] = (cf_t*)srslte_vec_malloc(sizeof(cf_t)*hypervisor_rx_handle->nof_samples_to_read);
    if(hypervisor_rx_handle->channelizer_buffer[i] == NULL) {
      HYPER_RX_ERROR("Error when allocating memory for channelizer_buffer", 0);
      exit(-1);
    }
    // Intialize channelizer buffer memory.
    bzero(hypervisor_rx_handle->channelizer_buffer[i], sizeof(cf_t)*hypervisor_rx_handle->nof_samples_to_read);
  }

  // Set channelizer buffer circular write counter to 0.
  hypervisor_rx_handle->channelizer_buffer_wr_cnt = 0;
}

void hypervisor_rx_uninitialize_channelizer() {
  // Destroy channelizer object.
  firpfbch_crcf_destroy(hypervisor_rx_handle->channelizer);
  // Free memory used to store channelizer buffer base band IQ samples.
  for(uint32_t i = 0; i < NOF_CHANNELIZER_BUFFERS; i++) {
    if(hypervisor_rx_handle->channelizer_buffer[i]) {
      free(hypervisor_rx_handle->channelizer_buffer[i]);
      hypervisor_rx_handle->channelizer_buffer[i] = NULL;
    }
  }
}

void *hypervisor_rx_channelizer_work(void *h) {
  // Cast handle to hypervisor Rx context.
  hypervisor_rx_t *hyper_rx_handle_ptr = (hypervisor_rx_t*)h;
  channel_buffer_context_t channel_buffer_ctx;
  uint32_t usrp_read_samples_buffer_rd_cnt = 0;
  bool status = false;

  // Set priority to Channelizer thread.
  uhd_set_thread_priority(1.0, true);

#if(ENABLE_STICKING_THREADS_TO_CORES==1)
  int ret = helpers_stick_this_thread_to_core(26);
  if(ret != 0) {
    HYPER_RX_ERROR("--------> Set channelizer thread to CPU ret: %d\n", ret);
  }
#endif

  HYPER_RX_PRINT("nof channelizer channels: %d - nof frames: %d - nof Rx vPHYs: %d\n", hyper_rx_handle_ptr->nof_samples_to_read,  hyper_rx_handle_ptr->channelizer_nof_channels, hyper_rx_handle_ptr->channelizer_nof_frames, hyper_rx_handle_ptr->num_of_rx_vphys);

  while(hyper_rx_handle_ptr->run_channelizer_thread) {

    // Check if there is a new context, if none is available, wait until it is not empty and retrieve the front element.
    status = hypervisor_rx_timedwait_and_get_usrp_buffer_context(&usrp_read_samples_buffer_rd_cnt);

    // Only run channelizer after all the vPHY Rx threads have been started and are waiting for samples.
    // Check status and if the vPHY Rx are running.
    if(status && hyper_rx_handle_ptr->vphy_rx_running) {

#if(MEASURE_CHANN_BUFFER_FULL_TIME==1)
      static uint32_t chann_cnt = 0;
      static struct timespec channelizer_timestamp;
      if(chann_cnt == 0) {
        clock_gettime(CLOCK_REALTIME, &channelizer_timestamp);
      }
#endif

      // Execute analysis filter bank, i.e., feed read base band samples into channelizer.
      for(uint32_t i = 0; i < hyper_rx_handle_ptr->channelizer_nof_frames; i++) {
        firpfbch_crcf_analyzer_execute(hyper_rx_handle_ptr->channelizer, &hyper_rx_handle_ptr->usrp_read_buffer[usrp_read_samples_buffer_rd_cnt][i*hyper_rx_handle_ptr->channelizer_nof_channels], &hyper_rx_handle_ptr->channelizer_buffer[hyper_rx_handle_ptr->channelizer_buffer_wr_cnt][i*hyper_rx_handle_ptr->channelizer_nof_channels]);
      }

      // Post that an ouput channel buffer is available to all expecting threads.
      channel_buffer_ctx.channel_buffer_rd_pos = hyper_rx_handle_ptr->channelizer_buffer_wr_cnt;
      // Add the context to all virtual Rx PHYs waiting for samples.
      for(uint32_t vphy_id = 0; vphy_id < hyper_rx_handle_ptr->num_of_rx_vphys; vphy_id++) {
        hypervisor_rx_push_channel_buffer_context_to_container(vphy_id, &channel_buffer_ctx);
      }

#if(MEASURE_CHANN_BUFFER_FULL_TIME==1)
      if(chann_cnt == (NOF_CHANNELIZER_BUFFERS-1)) {
        PHY_PROFILLING_AVG5("Channelizer execution time: %f [ms] - min: %f [ms] - max: %f [ms] - max counter %d - diff >= 5000 [ms]: %d - total counter: %d - perc: %f\n", helpers_profiling_diff_time(channelizer_timestamp), 5000.0, 1);
      }
      chann_cnt++;
      if(chann_cnt == NOF_CHANNELIZER_BUFFERS) {
        chann_cnt = 0;
      }
#endif

#if(DEBUG_CHANNELIZER_BUFFER_COUNTER==1)
      printf("channelizer_buffer_wr_cnt: %d - CB size: %d - usrp_read_samples_buffer_rd_cnt: %d\n", hyper_rx_handle_ptr->channelizer_buffer_wr_cnt, input_buffer_ctx_cb_size(hyper_rx_handle_ptr->usrp_read_cb_buffer_handle), usrp_read_samples_buffer_rd_cnt);
      if(hyper_rx_handle_ptr->usrp_read_samples_buffer_wr_cnt == usrp_read_samples_buffer_rd_cnt) {
        printf("channelizer_buffer_wr_cnt: %d - usrp_read_samples_buffer_rd_cnt: %d !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n", hyper_rx_handle_ptr->channelizer_buffer_wr_cnt, usrp_read_samples_buffer_rd_cnt);
      }
#endif

      // Increment IQ samples output buffer counter.
      hyper_rx_handle_ptr->channelizer_buffer_wr_cnt = (hyper_rx_handle_ptr->channelizer_buffer_wr_cnt + 1) % NOF_CHANNELIZER_BUFFERS;
    }

  }

  HYPER_RX_DEBUG("Leaving channelizer thread.\n", 0);
  // Exit thread with result code.
  pthread_exit(NULL);
}

// This function is used by the vPHY Rx threads to retrieve samples coming from the selected channels.
int hypervisor_rx_recv(void *h, void *data, uint32_t nof_samples) {

  vphy_reader_t *vphy_reader = (vphy_reader_t*)h;
  uint32_t channel = 0, nof_samples_to_read = 0, total_nof_read_samples = 0, curr_nof_samples = nof_samples;
  channel_buffer_context_t channel_buffer_ctx;
  cf_t *samples = (cf_t*)data;
  bool status;

  do {
    // Every time it reaches zero, then we retrieve from the circular buffer again.
    if(vphy_reader->nof_read_samples == 0) {
      // Check if there is a new context, if none is available, wait until it is not empty and retrieve the front element.
      status = hypervisor_rx_timedwait_and_get_channel_buffer_context(vphy_reader->vphy_id, &channel_buffer_ctx);
      // Check status.
      if(status) {
        // Update reader to structure to hold the current buffer to be read.
        vphy_reader->current_buffer_idx = channel_buffer_ctx.channel_buffer_rd_pos;
        // Number of remaining samples must be equal to the number of frames.
        vphy_reader->nof_remaining_samples = hypervisor_rx_handle->channelizer_nof_frames;
      } else {
        return 0;
      }
    }

    // Retrieve current channel for this vPHY.
    channel = vphy_reader->vphy_channel;
    if(channel >= hypervisor_rx_handle->channelizer_nof_channels) {
      HYPER_RX_ERROR("Channel is greater than number of available channels: %d, using 0 instead.\n", channel);
      channel = 0;
    }

    // Calculate how many samples we can read from the current buffer to attend the function's request.
    if(curr_nof_samples > vphy_reader->nof_remaining_samples) {
      nof_samples_to_read = vphy_reader->nof_remaining_samples;
    } else {
      nof_samples_to_read = curr_nof_samples;
    }

    // Read specified number of samples from specific channel buffer.
    for(uint32_t k = vphy_reader->nof_read_samples; k < (vphy_reader->nof_read_samples + nof_samples_to_read); k++) {
      *samples = hypervisor_rx_handle->channelizer_buffer[vphy_reader->current_buffer_idx][k*hypervisor_rx_handle->channelizer_nof_channels + channel];
      samples++;
    }

    // Update number of remaining samples.
    vphy_reader->nof_remaining_samples -= nof_samples_to_read;

    // Update number of read samples.
    vphy_reader->nof_read_samples += nof_samples_to_read;
    // If read the whole buffer content, then we should go to the next channel buffer position.
    if(vphy_reader->nof_read_samples >= hypervisor_rx_handle->channelizer_nof_frames) {
      vphy_reader->nof_read_samples = 0;
    }

    curr_nof_samples -= nof_samples_to_read;

    total_nof_read_samples += nof_samples_to_read;

  } while(total_nof_read_samples < nof_samples);

  return total_nof_read_samples;
}

// These functions have the same priority for message passing.
// Get channel buffer context from container. It must be a blocking function.
void hypervisor_rx_get_channel_buffer_context_inc_nof_reads(uint32_t vphy_id, channel_buffer_context_t* const channel_buffer_ctx) {
  // Lock mutex so that we can push slot control to container.
  pthread_mutex_lock(&hypervisor_rx_handle->channelizer_mutex[vphy_id]);
  // Retrieve channel buffer context element from circular buffer.
  channel_buffer_ctx_cb_front(hypervisor_rx_handle->channelizer_cb_buffer_handle[vphy_id], channel_buffer_ctx);
  // Remove channel buffer contextelement from  circular buffer.
  channel_buffer_ctx_cb_pop_front(hypervisor_rx_handle->channelizer_cb_buffer_handle[vphy_id]);
  // Unlock mutex.
  pthread_mutex_unlock(&hypervisor_rx_handle->channelizer_mutex[vphy_id]);
}

// Push channel buffer context into container.
void hypervisor_rx_push_channel_buffer_context_to_container(uint32_t vphy_id, channel_buffer_context_t* const channel_buffer_ctx) {
  // Lock mutex so that we can push slot control to container.
  pthread_mutex_lock(&hypervisor_rx_handle->channelizer_mutex[vphy_id]);
  // Push phy control into circular buffer.
  channel_buffer_ctx_cb_push_back(hypervisor_rx_handle->channelizer_cb_buffer_handle[vphy_id], channel_buffer_ctx);

#if(DEBUG_CHANNELIZER_BUFFER_COUNTER==1)
  channel_buffer_context_t channel_buffer_ctx_aux;
  channel_buffer_ctx_cb_front(hypervisor_rx_handle->channelizer_cb_buffer_handle[vphy_id], &channel_buffer_ctx_aux);
  printf("-------------------------------> [Chann buffer WR] - ID: %d - Size: %d - CB Back: %d - CB front: %d\n", vphy_id, channel_buffer_ctx_cb_size(hypervisor_rx_handle->channelizer_cb_buffer_handle[vphy_id]), channel_buffer_ctx->channel_buffer_rd_pos, channel_buffer_ctx_aux.channel_buffer_rd_pos);
#endif

  // Unlock mutex so that function can do other things.
  pthread_mutex_unlock(&hypervisor_rx_handle->channelizer_mutex[vphy_id]);
  // Notify other thread that slot control was pushed into container.
  pthread_cond_signal(&hypervisor_rx_handle->channelizer_cond_var[vphy_id]);
}

// Check if container is not empty, if so, wait until it is not empty and get the context, incrementing the counter.
bool hypervisor_rx_wait_and_get_channel_buffer_context(uint32_t vphy_id, channel_buffer_context_t* const channel_buffer_ctx) {
  bool ret = true;
  // Lock mutex.
  pthread_mutex_lock(&hypervisor_rx_handle->channelizer_mutex[vphy_id]);
  // Wait for conditional variable only if container is empty.
  if(channel_buffer_ctx_cb_empty(hypervisor_rx_handle->channelizer_cb_buffer_handle[vphy_id])) {
    // Wait for conditional variable to be true.
    pthread_cond_wait(&hypervisor_rx_handle->channelizer_cond_var[vphy_id], &hypervisor_rx_handle->channelizer_mutex[vphy_id]);
    if(!hypervisor_rx_handle->run_channelizer_thread) {
      ret = false;
    }
  }
  // Only retrieve the context if the thread is still running.
  if(ret) {
    // Retrieve channel buffer context element from circular buffer.
    channel_buffer_ctx_cb_front(hypervisor_rx_handle->channelizer_cb_buffer_handle[vphy_id], channel_buffer_ctx);
    // Remove channel buffer contextelement from  circular buffer.
    channel_buffer_ctx_cb_pop_front(hypervisor_rx_handle->channelizer_cb_buffer_handle[vphy_id]);
  }
  // Unlock mutex.
  pthread_mutex_unlock(&hypervisor_rx_handle->channelizer_mutex[vphy_id]);
  return ret;
}

// Check if container is not empty, if so, wait until it is not empty and get the context, incrementing the counter.
bool hypervisor_rx_timedwait_and_get_channel_buffer_context(uint32_t vphy_id, channel_buffer_context_t* const channel_buffer_ctx) {
  bool ret = true, is_cb_empty = true;
  struct timespec timeout;
  // Lock mutex.
  pthread_mutex_lock(&hypervisor_rx_handle->channelizer_mutex[vphy_id]);
  // Wait for conditional variable only if container is empty.
  if(channel_buffer_ctx_cb_empty(hypervisor_rx_handle->channelizer_cb_buffer_handle[vphy_id])) {
    do {
      // Timeout in 0.5 ms.
      helpers_get_timeout(50000, &timeout);
      // Timed wait for conditional variable to be true.
      pthread_cond_timedwait(&hypervisor_rx_handle->channelizer_cond_var[vphy_id], &hypervisor_rx_handle->channelizer_mutex[vphy_id], &timeout);
      // Check status of the circular buffer again.
      is_cb_empty = channel_buffer_ctx_cb_empty(hypervisor_rx_handle->channelizer_cb_buffer_handle[vphy_id]);
      // Check if the threads are still running, if not, then leave with false.
      if(!hypervisor_rx_handle->run_channelizer_thread || !phy_is_running()) {
        ret = false;
      }
    } while(is_cb_empty && ret);
  }

  // Only retrieve the context if the thread is still running.
  if(ret) {
    // Retrieve channel buffer context element from circular buffer.
    channel_buffer_ctx_cb_front(hypervisor_rx_handle->channelizer_cb_buffer_handle[vphy_id], channel_buffer_ctx);
    // Remove channel buffer contextelement from  circular buffer.
    channel_buffer_ctx_cb_pop_front(hypervisor_rx_handle->channelizer_cb_buffer_handle[vphy_id]);
  }

#if(DEBUG_CHANNELIZER_BUFFER_COUNTER==1)
  if(ret) {
    channel_buffer_context_t channel_buffer_ctx_aux;
    channel_buffer_ctx_cb_front(hypervisor_rx_handle->channelizer_cb_buffer_handle[vphy_id], &channel_buffer_ctx_aux);
    printf("<------------------------- [Chann buffer RD] - ID: %d Size: %d - Read: %d - CB Front: %d\n", vphy_id, channel_buffer_ctx_cb_size(hypervisor_rx_handle->channelizer_cb_buffer_handle[vphy_id]), channel_buffer_ctx->channel_buffer_rd_pos, channel_buffer_ctx_aux.channel_buffer_rd_pos);
  }
#endif

  // Unlock mutex.
  pthread_mutex_unlock(&hypervisor_rx_handle->channelizer_mutex[vphy_id]);
  return ret;
}

#else
  int hypervisor_rx_recv(void *h, void *data, uint32_t nof_samples) {
    return 0;
  }
#endif
//******************************************************************************
// --------------------------- Channelizer (END) -----------------------------
//******************************************************************************

void hypervisor_rx_set_channel(uint32_t vphy_id, uint32_t channel) {
  // Lock mutex so that we can update channel list.
  pthread_mutex_lock(&hypervisor_rx_handle->hypervisor_rx_channel_list_mutex);
  hypervisor_rx_handle->channel_list[vphy_id] = (channel + (hypervisor_rx_handle->channelizer_nof_channels/2)) % hypervisor_rx_handle->channelizer_nof_channels;
  // Unlock mutex so that function can do other things.
  pthread_mutex_unlock(&hypervisor_rx_handle->hypervisor_rx_channel_list_mutex);
}

uint32_t hypervisor_rx_get_channel(uint32_t vphy_id) {
  uint32_t channel = 0;
  // Lock mutex so that we can update channel list.
  pthread_mutex_lock(&hypervisor_rx_handle->hypervisor_rx_channel_list_mutex);
  channel = hypervisor_rx_handle->channel_list[vphy_id];
  // Unlock mutex so that function can do other things.
  pthread_mutex_unlock(&hypervisor_rx_handle->hypervisor_rx_channel_list_mutex);
  return channel;
}

void hypervisor_rx_set_vphy_rx_running_flag(bool vphy_rx_running) {
  hypervisor_rx_handle->vphy_rx_running = vphy_rx_running;
}

void hypervisor_rx_set_last_gain(uint32_t rx_gain) {
  // Lock a mutex prior to using the slot control object.
  pthread_mutex_lock(&hypervisor_rx_handle->hypervisor_rx_radio_params_mutex);
  hypervisor_rx_handle->last_rx_hypervisor_ctrl.gain = rx_gain;
  // Unlock mutex upon using the slot control object.
  pthread_mutex_unlock(&hypervisor_rx_handle->hypervisor_rx_radio_params_mutex);
}

uint32_t hypervisor_rx_get_last_gain() {
  uint32_t rx_gain;
  // Lock a mutex prior to using the slot control object.
  pthread_mutex_lock(&hypervisor_rx_handle->hypervisor_rx_radio_params_mutex);
  rx_gain = hypervisor_rx_handle->last_rx_hypervisor_ctrl.gain;
  // Unlock mutex upon using the slot control object.
  pthread_mutex_unlock(&hypervisor_rx_handle->hypervisor_rx_radio_params_mutex);
  return rx_gain;
}

int hypervisor_rx_radio_recv(hypervisor_rx_t* hyper_rx_handle, void *data, uint32_t nof_samples, bool blocking, time_t *full_secs, double *frac_secs, size_t channel) {
  return srslte_rf_recv_with_time(hyper_rx_handle->hypervisor_rx_rf, data, nof_samples, blocking, full_secs, frac_secs, channel);
}
