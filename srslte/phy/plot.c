#include "plot.h"

#include "hypervisor_rx.h"

/**********************************************************************
 *  Plotting Functions
 **********************************************************************/
static ue_decoding_plot_context_t *ue_decoding_plot_ctx = NULL;

// Structure holding plotting object and parameters.
static spectrum_plot_context_t *spectrum_plot_ctx = NULL;

int init_ue_decoding_plot_object() {
  // Allocate memory for a new plot handle object.
  ue_decoding_plot_ctx = (ue_decoding_plot_context_t*)srslte_vec_malloc(sizeof(ue_decoding_plot_context_t));
  // Check if memory allocation was correctly done.
  if(ue_decoding_plot_ctx == NULL) {
    printf("Error allocating memory for plot context.\n");
    return -1;
  }
  // Initialize structure with zeros.
  bzero(ue_decoding_plot_ctx, sizeof(ue_decoding_plot_context_t));

  sdrgui_init();

  plot_scatter_init(&ue_decoding_plot_ctx->pscatequal);
  plot_scatter_setTitle(&ue_decoding_plot_ctx->pscatequal, "PDSCH - Equalized Symbols");
  plot_scatter_setXAxisScale(&ue_decoding_plot_ctx->pscatequal, -2, 2);
  plot_scatter_setYAxisScale(&ue_decoding_plot_ctx->pscatequal, -2, 2);

  plot_scatter_addToWindowGrid(&ue_decoding_plot_ctx->pscatequal, (char*)"pdsch_ue", 0, 0);

  plot_real_init(&ue_decoding_plot_ctx->pce);
  plot_real_setTitle(&ue_decoding_plot_ctx->pce, "Channel Response - Magnitude");
  plot_real_setLabels(&ue_decoding_plot_ctx->pce, "Index", "dB");
  plot_real_setYAxisScale(&ue_decoding_plot_ctx->pce, -40, 40);

  plot_real_init(&ue_decoding_plot_ctx->p_sync);
  plot_real_setTitle(&ue_decoding_plot_ctx->p_sync, "PSS Cross-Corr abs value");
  plot_real_setYAxisScale(&ue_decoding_plot_ctx->p_sync, 0, 1);

  plot_scatter_init(&ue_decoding_plot_ctx->pscatequal_pdcch);
  plot_scatter_setTitle(&ue_decoding_plot_ctx->pscatequal_pdcch, "PDCCH - Equalized Symbols");
  plot_scatter_setXAxisScale(&ue_decoding_plot_ctx->pscatequal_pdcch, -2, 2);
  plot_scatter_setYAxisScale(&ue_decoding_plot_ctx->pscatequal_pdcch, -2, 2);

  plot_real_addToWindowGrid(&ue_decoding_plot_ctx->pce, (char*)"pdsch_ue",    0, 1);
  plot_real_addToWindowGrid(&ue_decoding_plot_ctx->pscatequal_pdcch, (char*)"pdsch_ue", 1, 0);
  plot_real_addToWindowGrid(&ue_decoding_plot_ctx->p_sync, (char*)"pdsch_ue", 1, 1);
  // Everything went well.
  return 0;
}

void free_ue_decoding_plot_object() {
  // Free memory used to store plot context object.
  if(ue_decoding_plot_ctx) {
    free(ue_decoding_plot_ctx);
    ue_decoding_plot_ctx = NULL;
  }
}

void update_ue_decoding_plot(srslte_ue_dl_t* const ue_dl, srslte_ue_sync_t* const ue_sync) {
  bool plot_track = false;
  bool disable_plots_except_constellation = false;
  int i;
  uint32_t nof_re = SRSLTE_SF_LEN_RE(ue_dl->cell.nof_prb, ue_dl->cell.cp);

  uint32_t nof_symbols = ue_dl->pdsch_cfg.nbits.nof_re;
  if(!disable_plots_except_constellation) {
    for(i = 0; i < nof_re; i++) {
      ue_decoding_plot_ctx->tmp_plot[i] = 20 * log10f(cabsf(ue_dl->sf_symbols[i]));
      if(isinf(ue_decoding_plot_ctx->tmp_plot[i])) {
        ue_decoding_plot_ctx->tmp_plot[i] = -80;
      }
    }
    int sz = srslte_symbol_sz(ue_dl->cell.nof_prb);
    bzero(ue_decoding_plot_ctx->tmp_plot2, sizeof(float)*sz);
    int g = (sz - 12*ue_dl->cell.nof_prb)/2;
    for(i = 0; i < 12*ue_dl->cell.nof_prb; i++) {
      ue_decoding_plot_ctx->tmp_plot2[g+i] = 20 * log10(cabs(ue_dl->ce[0][i]));
      if(isinf(ue_decoding_plot_ctx->tmp_plot2[g+i])) {
        ue_decoding_plot_ctx->tmp_plot2[g+i] = -80;
      }
    }
    plot_real_setNewData(&ue_decoding_plot_ctx->pce, ue_decoding_plot_ctx->tmp_plot2, sz);

    if(plot_track) {
      srslte_pss_synch_t *pss_obj = srslte_sync_get_cur_pss_obj(&ue_sync->strack);
      int max = srslte_vec_max_fi(pss_obj->conv_output_avg_plot, pss_obj->frame_size+pss_obj->fft_size-1);
      srslte_vec_sc_prod_fff(pss_obj->conv_output_avg_plot,
                             1/pss_obj->conv_output_avg_plot[max],
                             ue_decoding_plot_ctx->tmp_plot2,
                             pss_obj->frame_size+pss_obj->fft_size-1);
      plot_real_setNewData(&ue_decoding_plot_ctx->p_sync, ue_decoding_plot_ctx->tmp_plot2, pss_obj->frame_size);
    } else {
      int max = srslte_vec_max_fi(ue_sync->sfind.pss.conv_output_avg_plot, ue_sync->sfind.pss.frame_size+ue_sync->sfind.pss.fft_size-1);
      srslte_vec_sc_prod_fff(ue_sync->sfind.pss.conv_output_avg_plot,
                             1/ue_sync->sfind.pss.conv_output_avg_plot[max],
                             ue_decoding_plot_ctx->tmp_plot2,
                             ue_sync->sfind.pss.frame_size+ue_sync->sfind.pss.fft_size-1);
      plot_real_setNewData(&ue_decoding_plot_ctx->p_sync, ue_decoding_plot_ctx->tmp_plot2, ue_sync->sfind.pss.frame_size);
    }

    plot_scatter_setNewData(&ue_decoding_plot_ctx->pscatequal_pdcch, ue_dl->pdcch.d, 36*ue_dl->pdcch.nof_cce);
  }

  plot_scatter_setNewData(&ue_decoding_plot_ctx->pscatequal, ue_dl->pdsch.d, nof_symbols);
}

//******************************************************************************
// Plot spectrum.
//******************************************************************************
int init_spectrum_plot_object(uint32_t fft_size, uint32_t nof_samples, plot_t plot_type, cf_t **usrp_buffer) {
  // Allocate memory for a new plot handle object.
  spectrum_plot_ctx = (spectrum_plot_context_t*)srslte_vec_malloc(sizeof(spectrum_plot_context_t));
  // Check if memory allocation was correctly done.
  if(spectrum_plot_ctx == NULL) {
    PLOT_ERROR("Error allocating memory for plot context.\n",0);
    return -1;
  }
  // Clearing structure.
  bzero(spectrum_plot_ctx, sizeof(spectrum_plot_context_t));
  // Assign fft size with valid value.
  spectrum_plot_ctx->fft_size = fft_size;
  // Number of samples to plot.
  spectrum_plot_ctx->nof_samples = nof_samples;
  // Allocate memroy for PSD.
  spectrum_plot_ctx->psd = (float*)srslte_vec_malloc(sizeof(float)*spectrum_plot_ctx->fft_size);
  // Check if memory allocation was correctly done.
  if(spectrum_plot_ctx->psd == NULL) {
    PLOT_ERROR("Error allocating memory for PSD.\n",0);
    return -1;
  }
  // Initialize GUI.
  sdrgui_init();
  // Initialize the correct plot type.
  spectrum_plot_ctx->plot_type = plot_type;
  switch(spectrum_plot_ctx->plot_type) {
    case PLOT_PSD:
      init_psd_plot(spectrum_plot_ctx);
      break;
    case PLOT_WATERFALL:
      init_waterfall_plot(spectrum_plot_ctx);
      break;
    case PLOT_PERIODOGRAM:
      init_periodogram_plot(spectrum_plot_ctx);
      break;
    default:
      PLOT_ERROR("Invalid plot type.\n", 0);
      return -1;
  }
  // Register USRP buffer containing IQ samples that will be ploted.
  if(usrp_buffer != NULL) {
    spectrum_plot_ctx->usrp_buffer = usrp_buffer;
  } else {
    return -1;
  }
  // Instantiate USRP read circular buffer object.
  input_buffer_ctx_cb_make(&spectrum_plot_ctx->buffer_index_cb_handle, NOF_USRP_READ_BUFFERS);
  // Initialize buffer index mutex.
  if(pthread_mutex_init(&spectrum_plot_ctx->buffer_index_mutex, NULL) != 0) {
    PLOT_ERROR("Initialization of buffer index mutex failed.\n",0);
    return -1;
  }
  // Initialize buffer index conditional variable.
  if(pthread_cond_init(&spectrum_plot_ctx->buffer_index_cv, NULL)) {
    PLOT_ERROR("Initialization of buffer index conditional variable failed.\n",0);
    return -1;
  }
  // Enable plotting thread to run.
  spectrum_plot_ctx->run_plot_thread = true;
  // Start plotting thread.
  pthread_attr_init(&spectrum_plot_ctx->plot_thread_attr);
  pthread_attr_setdetachstate(&spectrum_plot_ctx->plot_thread_attr, PTHREAD_CREATE_JOINABLE);
  // Spawn plotting thread.
  int ret = pthread_create(&spectrum_plot_ctx->plot_thread_id, &spectrum_plot_ctx->plot_thread_attr, spectrum_plot_work, (void*)spectrum_plot_ctx);
  if(ret) {
    PLOT_ERROR("Error spwaning plotting thread. Error: %d\n", ret);
    return -1;
  }
  // Everything went well.
  return 0;
}

int free_spectrum_plot_object() {
  // Uninitialize plotting thread.
  spectrum_plot_ctx->run_plot_thread = false;
  // Notify thread.
  pthread_cond_signal(&spectrum_plot_ctx->buffer_index_cv);
  // Destroy and join plotting thread.
  pthread_attr_destroy(&spectrum_plot_ctx->plot_thread_attr);
  int ret = pthread_join(spectrum_plot_ctx->plot_thread_id, NULL);
  if(ret) {
    PLOT_ERROR("Error joining plotting thread. Error: %d\n", ret);
    return -1;
  }
  // Free circular buffer holding read index.
  input_buffer_ctx_cb_free(&spectrum_plot_ctx->buffer_index_cb_handle);
  // Destroy mutex.
  pthread_mutex_destroy(&spectrum_plot_ctx->buffer_index_mutex);
  // Destory conditional variable.
  if(pthread_cond_destroy(&spectrum_plot_ctx->buffer_index_cv) != 0) {
    HYPER_RX_ERROR("Buffer index conditional variable destruction failed.\n",0);
    return -1;
  }
  // Destroy periodogram object.
  if(spectrum_plot_ctx->plot_type == PLOT_PERIODOGRAM) {
    destroy_periodogram_plot(spectrum_plot_ctx);
  }
  // Free memory used to store PSD.
  if(spectrum_plot_ctx->psd) {
    free(spectrum_plot_ctx->psd);
    spectrum_plot_ctx->psd = NULL;
  } else {
    PLOT_ERROR("Error freeing PSD memory.",0);
    return -1;
  }
  // Free memory used to store plot context object.
  if(spectrum_plot_ctx) {
    free(spectrum_plot_ctx);
    spectrum_plot_ctx = NULL;
  } else {
    PLOT_ERROR("Error freeing Plot context memory.",0);
    return -1;
  }
  // Exit GUI.
  sdrgui_exit();
  // Everything went well.
  return 0;
}

// Thread used to plot.
void *spectrum_plot_work(void *h) {
  uint32_t buffer_index;
  // Run plotting thread.
  while(spectrum_plot_timedwait_and_get_buffer_index(&buffer_index) && spectrum_plot_ctx->run_plot_thread) {
    // Select which type of plot should be used.
    switch(spectrum_plot_ctx->plot_type) {
      case PLOT_PSD:
        update_psd_plot2(buffer_index);
        break;
      case PLOT_PERIODOGRAM:
        update_periodgram_plot2(buffer_index);
        break;
      case PLOT_WATERFALL:
      default:
        update_waterfall_plot3(buffer_index);
    }
    // Sleep for a while before ploting again.
    usleep(20000);
  }

  PLOT_PRINT("Leaving plotting thread.\n", 0);
  // Exit thread with result code NULL.
  pthread_exit(NULL);
}

// ------------------------ Power Spectrum Density ------------------------
int init_psd_plot(spectrum_plot_context_t *q) {
  // Set initial parameters.
  plot_real_init(&q->spectrum);
  plot_real_setTitle(&q->spectrum, "Spectrum");
  plot_real_setLabels(&q->spectrum, "Normalized Frequency [f/Fs]", "Power Spectral Density [dB]");
  plot_real_setYAxisScale(&q->spectrum, -70, 0);
  plot_real_setXAxisRange(&q->spectrum, -0.5, 0.5);
  plot_real_addToWindow(&q->spectrum, (char*)"psd");
  // Everything went well.
  return 0;
}

void update_psd_plot(spectrum_plot_context_t *q, cf_t *sig, uint32_t nof_samples) {
  // Calculate the Power spectrum density of the input signal.
  spgramcf_estimate_psd(q->fft_size, sig, nof_samples, q->psd);
  // Update window with the current PSD value.
  plot_real_setNewData(&q->spectrum, q->psd, q->fft_size);
}

void update_psd_plot2(uint32_t buffer_index) {
  // Calculate the Power spectrum density of the input signal.
  spgramcf_estimate_psd(spectrum_plot_ctx->fft_size, spectrum_plot_ctx->usrp_buffer[buffer_index], spectrum_plot_ctx->nof_samples, spectrum_plot_ctx->psd);
  // Update window with the current PSD value.
  plot_real_setNewData(&spectrum_plot_ctx->spectrum, spectrum_plot_ctx->psd, spectrum_plot_ctx->fft_size);
}

// ------------------------ Waterfall ------------------------
int init_waterfall_plot(spectrum_plot_context_t *q) {
  // Set initial parameters.
  plot_waterfall_init(&q->waterfall, q->fft_size, q->fft_size);
  plot_waterfall_setTitle(&q->waterfall, "Waterfall");
  plot_waterfall_setSpectrogramXLabel(&q->waterfall, "Frequency");
  plot_waterfall_setSpectrogramYLabel(&q->waterfall, "Time");
  plot_waterfall_addToWindow(&q->waterfall, (char*)"waterfall");
  // Everything went well.
  return 0;
}

void update_waterfall_plot(spectrum_plot_context_t *q, cf_t *sig, uint32_t nof_samples) {
  // Calculate the Power spectrum density of the input signal.
  spgramcf_estimate_psd(q->fft_size, sig, nof_samples, q->psd);
  // Update window with the current PSD value.
  plot_waterfall_appendNewData(&q->waterfall, q->psd, q->fft_size);
}

void update_waterfall_plot2(spectrum_plot_context_t *q, cf_t *sig, uint32_t nof_samples) {
  uint32_t nof_frames = nof_samples/q->fft_size;
  for(uint32_t i = 0; i < nof_frames; i++) {
    // Calculate the Power spectrum density of the input signal.
    spgramcf_estimate_psd(q->fft_size, &sig[i*q->fft_size], q->fft_size, q->psd);
    // Update window with the current PSD value.
    plot_waterfall_appendNewData(&q->waterfall, q->psd, q->fft_size);
  }
}

void update_waterfall_plot3(uint32_t buffer_index) {
  // Calculate the Power spectrum density of the input signal.
  spgramcf_estimate_psd(spectrum_plot_ctx->fft_size, spectrum_plot_ctx->usrp_buffer[buffer_index], spectrum_plot_ctx->nof_samples, spectrum_plot_ctx->psd);
  // Update window with the current PSD value.
  plot_waterfall_appendNewData(&spectrum_plot_ctx->waterfall, spectrum_plot_ctx->psd, spectrum_plot_ctx->fft_size);
}

// ------------------------ Periodogram ------------------------
int init_periodogram_plot(spectrum_plot_context_t *q) {
  init_waterfall_plot(q);
  q->periodogram = spgramcf_create_default(q->fft_size);
  // Everything went well.
  return 0;
}

int destroy_periodogram_plot(spectrum_plot_context_t *q) {
  spgramcf_destroy(q->periodogram);
  // Everything went well.
  return 0;
}

void update_periodgram_plot(spectrum_plot_context_t *q, cf_t *sig, uint32_t nof_samples) {
  // Push resulting sample through periodogram.
  spgramcf_write(q->periodogram, sig, nof_samples);
  // Compute power spectral density output
  spgramcf_get_psd(q->periodogram, q->psd);
  // Update window with the current PSD value.
  plot_waterfall_appendNewData(&q->waterfall, q->psd, q->fft_size);
  // Soft reset of internal state, counters
  spgramcf_clear(q->periodogram);
}

void update_periodgram_plot2(uint32_t buffer_index) {
  // Push resulting sample through periodogram.
  spgramcf_write(spectrum_plot_ctx->periodogram, spectrum_plot_ctx->usrp_buffer[buffer_index], spectrum_plot_ctx->nof_samples);
  // Compute power spectral density output
  spgramcf_get_psd(spectrum_plot_ctx->periodogram, spectrum_plot_ctx->psd);
  // Update window with the current PSD value.
  plot_waterfall_appendNewData(&spectrum_plot_ctx->waterfall, spectrum_plot_ctx->psd, spectrum_plot_ctx->fft_size);
  // Soft reset of internal state, counters
  spgramcf_clear(spectrum_plot_ctx->periodogram);
}

// --------------------- Circular buffer push/pop functions --------------------
void spectrum_plot_push_buffer_index(uint32_t index) {
  // Lock mutex.
  pthread_mutex_lock(&spectrum_plot_ctx->buffer_index_mutex);
  // Push buffer index into the circular buffer.
  input_buffer_ctx_cb_push_back(spectrum_plot_ctx->buffer_index_cb_handle, index);
  // Unlock mutex.
  pthread_mutex_unlock(&spectrum_plot_ctx->buffer_index_mutex);
  // Notify other thread that input IQ sample counter was pushed into container.
  pthread_cond_signal(&spectrum_plot_ctx->buffer_index_cv);
}

// Check if container is not empty, if so, wait until it is not empty and get the context, incrementing the counter.
bool spectrum_plot_timedwait_and_get_buffer_index(uint32_t* index) {
  bool ret = true, is_cb_empty = true;
  struct timespec timeout;
  // Lock mutex.
  pthread_mutex_lock(&spectrum_plot_ctx->buffer_index_mutex);
  // Wait for conditional variable only if container is empty.
  if(input_buffer_ctx_cb_empty(spectrum_plot_ctx->buffer_index_cb_handle)) {
    do {
      // Timeout in 10 ms.
      helpers_get_timeout(1000000, &timeout);
      // Timed wait for conditional variable to be true.
      pthread_cond_timedwait(&spectrum_plot_ctx->buffer_index_cv, &spectrum_plot_ctx->buffer_index_mutex, &timeout);
      // Check status of the circular buffer again.
      is_cb_empty = input_buffer_ctx_cb_empty(spectrum_plot_ctx->buffer_index_cb_handle);
      // Check if the threads are still running, if not, then leave with false.
      if(!spectrum_plot_ctx->run_plot_thread) {
        ret = false;
      }
    } while(is_cb_empty && ret);
  }
  // Only retrieve the context if the thread is still running.
  if(ret) {
    // Retrieve input IQ samples buffer context element from circular buffer.
    *index = input_buffer_ctx_cb_front(spectrum_plot_ctx->buffer_index_cb_handle);
    // Remove input IQ samples buffer contextelement from  circular buffer.
    input_buffer_ctx_cb_pop_front(spectrum_plot_ctx->buffer_index_cb_handle);
  }
  // Unlock mutex.
  pthread_mutex_unlock(&spectrum_plot_ctx->buffer_index_mutex);
  return ret;
}
