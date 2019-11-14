#include "srslte/channel/channel_emulator.h"
#include "gauss.h"

int channel_emulator_initialization(channel_emulator_t* chann_emulator) {
  // Create named pipe.
  if(channel_emulator_create_named_pipe(CH_EMULATOR_NP_FILENAME) < 0) {
    CH_EMULATOR_ERROR("Error creating named pipe.\n",0);
    return -1;
  }
  // Get reading named pipe.
  chann_emulator->rd_fd = channel_emulator_get_named_pipe_for_reading(CH_EMULATOR_NP_FILENAME);
  if(chann_emulator->rd_fd < 0) {
    CH_EMULATOR_ERROR("Error opening channel emulator writer.\n",0);
    return -1;
  }
  // Get writing named pipe.
  chann_emulator->wr_fd = channel_emulator_get_named_pipe_for_writing(CH_EMULATOR_NP_FILENAME);
  if(chann_emulator->wr_fd < 0) {
    CH_EMULATOR_ERROR("Error opening channel emulator writer.\n",0);
    return -1;
  }
  // Initialize counters.
  chann_emulator->nof_reads = 0;
  // Initialize subframe length.
  chann_emulator->subframe_length = DEFAULT_SUBFRAME_LEN;
  // Initialize flag for channel impairments.
  chann_emulator->enable_channel_impairments = false;
  // Create channel.
  chann_emulator->chan.channel = channel_cccf_create();
  // Set Channel emulator's Callbacks.
  chann_emulator->chan.add_awgn_ptr = &channel_emulator_add_awgn;
  chann_emulator->chan.add_carrier_offset_ptr = &channel_emulator_add_carrier_offset;
  chann_emulator->chan.add_multipath_ptr = &channel_emulator_add_multipath;
  chann_emulator->chan.add_shadowing_ptr = &channel_emulator_add_shadowing;
  chann_emulator->chan.print_channel_ptr = &channel_emulator_print_channel;
  chann_emulator->chan.estimate_psd_ptr = &channel_emulator_estimate_psd;
  chann_emulator->chan.create_psd_script = &channel_emulator_create_psd_script;
  chann_emulator->chan.set_cfo_freq = &channel_emulator_set_cfo_freq;
  // Create CFO generator.
  if(srslte_cfo_init_finer(&chann_emulator->chan.cfocorr, DEFAULT_SUBFRAME_LEN)) {
    fprintf(stderr, "Error initiating CFO\n");
    exit(-1);
  }
  // Set the number of FFT bins used.
  chann_emulator->chan.fft_size = srslte_symbol_sz(DEFAULT_NOF_PRB);
  srslte_cfo_set_fft_size_finer(&chann_emulator->chan.cfocorr, chann_emulator->chan.fft_size);
  // Set default value for CFO frequency.
  chann_emulator->chan.cfo_freq = 0.0;
  // Allocate and initialize memory for random transmission delay.
#if(ENABLE_WRITING_RANDOM_ZEROS_SUFFIX==1 || ENABLE_WRITING_RANDOM_ZEROS_PREFIX==1)
  uint32_t nof_subframes = 6;
  chann_emulator->null_sample_vector = (cf_t*)srslte_vec_malloc(nof_subframes*DEFAULT_SUBFRAME_LEN*sizeof(cf_t));
  // Check if memory allocation was correctly done.
  if(chann_emulator->null_sample_vector == NULL) {
    CH_EMULATOR_ERROR("Error allocating memory for NULL Sample vector.\n",0);
    exit(-1);
  }
  bzero(chann_emulator->null_sample_vector, sizeof(cf_t)*nof_subframes*DEFAULT_SUBFRAME_LEN);
#endif

  return 0;
}

int channel_emulator_uninitialization(channel_emulator_t* chann_emulator) {
  // Close channel emulator writing pipe.
  if(channel_emulator_close_writing_pipe(chann_emulator->wr_fd) < 0) {
    return -1;
  }
  // Close channel emulator reading pipe.
  if(channel_emulator_close_reading_pipe(chann_emulator->rd_fd) < 0) {
    return -1;
  }
  // Destroy channel.
  channel_cccf_destroy(chann_emulator->chan.channel);
  // Destroy all CFO related structures.
  srslte_cfo_free_finer(&chann_emulator->chan.cfocorr);
  // Free memory used to store Application context object.
#if(ENABLE_WRITING_RANDOM_ZEROS_SUFFIX==1 || ENABLE_WRITING_RANDOM_ZEROS_PREFIX==1)
  if(chann_emulator->null_sample_vector) {
    free(chann_emulator->null_sample_vector);
    chann_emulator->null_sample_vector = NULL;
  }
#endif

  return 0;
}
// Add AWGN impairment.
void channel_emulator_add_awgn(void *h, float noise_floor, float SNRdB) {
  channel_emulator_t *ch_emulator = (channel_emulator_t*)h;
  channel_cccf_add_awgn(ch_emulator->chan.channel, noise_floor, SNRdB);
}

// Add carrier offset impairment.
void channel_emulator_add_carrier_offset(void *h, float dphi, float phi) {
  channel_emulator_t *ch_emulator = (channel_emulator_t*)h;
  channel_cccf_add_carrier_offset(ch_emulator->chan.channel, dphi, phi);
}

// Add multipath impairment.
void channel_emulator_add_multipath(void *h, cf_t *hc, unsigned int hc_len) {
  channel_emulator_t *ch_emulator = (channel_emulator_t*)h;
  channel_cccf_add_multipath(ch_emulator->chan.channel, hc, hc_len);
}

// Add shadowing impairment.
void channel_emulator_add_shadowing(void *h, float sigma, float fd) {
  channel_emulator_t *ch_emulator = (channel_emulator_t*)h;
  channel_cccf_add_shadowing(ch_emulator->chan.channel, sigma, fd);
}

// Print channel internals.
void channel_emulator_print_channel(void * h) {
  channel_emulator_t *ch_emulator = (channel_emulator_t*)h;
  channel_cccf_print(ch_emulator->chan.channel);
}

// Estimate spectrum.
void channel_emulator_estimate_psd(cf_t *sig, uint32_t nof_samples, unsigned int nfft, float *psd) {
  spgramcf_estimate_psd(nfft, sig, nof_samples, psd);
}

void channel_emulator_create_psd_script(unsigned int nfft, float *psd) {
  FILE * fid = fopen(OUTPUT_FILENAME, "w");
  // power spectral density estimate
  fprintf(fid,"nfft = %u;\n", nfft);
  fprintf(fid,"f=[0:(nfft-1)]/nfft - 0.5;\n");
  fprintf(fid,"psd = zeros(1,nfft);\n");
  for(uint32_t i = 0; i < nfft; i++) {
    fprintf(fid,"psd(%3u) = %12.8f;\n", i+1, psd[i]);
  }

  fprintf(fid,"  plot(f, psd, 'LineWidth',1.5,'Color',[0 0.5 0.2]);\n");
  fprintf(fid,"  grid on;\n");
  fprintf(fid,"  pmin = 10*floor(0.1*min(psd - 5));\n");
  fprintf(fid,"  pmax = 10*ceil (0.1*max(psd + 5));\n");
  fprintf(fid,"  axis([-0.5 0.5 pmin pmax]);\n");
  fprintf(fid,"  xlabel('Normalized Frequency [f/F_s]');\n");
  fprintf(fid,"  ylabel('Power Spectral Density [dB]');\n");

  fclose(fid);
  printf("results written to %s.\n", OUTPUT_FILENAME);
}

int channel_emulator_set_subframe_length(channel_emulator_t* chann_emulator, double freq) {
  int subframe_length = -1;
  if(freq > 0) {
    subframe_length = (int)(freq*0.001);
    chann_emulator->subframe_length = subframe_length;
  }
  return subframe_length;
}

void channel_emulator_set_channel_impairments(void* h, bool flag) {
  channel_emulator_t* chann_emulator = (channel_emulator_t*)h;
  chann_emulator->enable_channel_impairments = flag;
}

void channel_emulator_set_cfo_freq(void* h, float freq) {
  channel_emulator_t* chann_emulator = (channel_emulator_t*)h;
  chann_emulator->chan.cfo_freq = freq;
}

int channel_emulator_create_named_pipe(const char *path) {
  int ret = 0;
  // Create named pipe name.
  CH_EMULATOR_INFO("Creating: %s\n", path);
  // Create named pipe.
  ret = mkfifo(path, 0666);
  if(ret < 0) {
    if(errno != EEXIST) {
      char error_str[100];
      get_error_string(errno, error_str);
      CH_EMULATOR_ERROR("Error: %s\n", error_str);
      return -1;
    } else {
      CH_EMULATOR_INFO("The named pipe, %s, already exists.\n", path);
    }
  }

  return 0;
}

int channel_emulator_get_named_pipe_for_writing(const char *path) {
  int ret = 0, fd = -1;
  // Open named pipe for read/write.
  fd = open(path, O_WRONLY);
  if(fd < 0) {
    CH_EMULATOR_ERROR("Error opening writing pipe: %d\n", errno);
    return -1;
  }
  // Increase the named pipe size.
  ret = fcntl(fd, F_SETPIPE_SZ, DEFAULT_SUBFRAME_LEN*sizeof(cf_t));
  if(ret < 0) {
    CH_EMULATOR_ERROR("Error increasing pipe size: %d\n", errno);
    return -1;
  }
  CH_EMULATOR_PRINT("Named pipe increasing to size: %d\n", DEFAULT_SUBFRAME_LEN*sizeof(cf_t));

  return fd;
}

int channel_emulator_get_named_pipe_for_reading(const char *path) {
  int fd = -1;
  // First open in read only mode.
  fd = open(path, O_RDONLY|O_NONBLOCK);
  if(fd < 0) {
    CH_EMULATOR_ERROR("Error opening reading pipe: %d\n", errno);
    return -1;
  }
  return fd;
}

int channel_emulator_close_reading_pipe(int fd) {
  int ret = -1;
  ret = close(fd);
  if(ret < 0) {
    CH_EMULATOR_ERROR("Error closing reading pipe: %d\n", errno);
  }
  return ret;
}

int channel_emulator_close_writing_pipe(int fd) {
  int ret = -1;
  ret = close(fd);
  if(ret < 0) {
    CH_EMULATOR_ERROR("Error closing writing pipe: %d\n", errno);
  }
  return ret;
}

int channel_emulator_send(void* h, void *data, int nof_samples, bool blocking, bool is_start_of_burst, bool is_end_of_burst) {

  channel_emulator_t *ch_emulator = (channel_emulator_t*)h;
  int ret = 0;
  int32_t nof_written_samples = 0;
#if(ENABLE_WRITING_ZEROS==1)
  uint32_t additional_samples = 0;
#endif

  // Write a random number of zeros before the subframe in order to emulate real-world transmission.
#if(ENABLE_WRITING_RANDOM_ZEROS_PREFIX==1)
  // Get random number of additional zero samples.
  if(is_start_of_burst) {
    uint32_t nof_random_samples = (rand() % (1*DEFAULT_SUBFRAME_LEN));
    // Sleep for the duration of the random number of samples.
#if(ADD_DELAY_BEFORE_RANDOM_SAMPLES==1)
    uint32_t random_samples_delay = (uint32_t)((((float)nof_random_samples)*1000.0)/((float)DEFAULT_SUBFRAME_LEN));
    usleep(8*random_samples_delay);
#endif
    // Write random number of samples.
    ret = write(ch_emulator->wr_fd, ch_emulator->null_sample_vector, nof_random_samples*sizeof(cf_t));
    if(ret < 0) {
      CH_EMULATOR_ERROR("Write to named pipe returned: %d\n", ret);
      return -1;
    } else {
      nof_written_samples = ret/sizeof(cf_t);
    }
  }

  ret = write(ch_emulator->wr_fd, data, nof_samples*sizeof(cf_t));
  if(ret < 0) {
    CH_EMULATOR_ERROR("Write to named pipe returned: %d\n", ret);
    return -1;
  } else {
    nof_written_samples += ret/sizeof(cf_t);
  }

#else
  ret = write(ch_emulator->wr_fd, data, nof_samples*sizeof(cf_t));
  if(ret < 0) {
    CH_EMULATOR_ERROR("Write to named pipe returned: %d\n", ret);
    return -1;
  } else {
    nof_written_samples = ret/sizeof(cf_t);
  }
#endif

  // Write a random number of zeros to emulate real-world transmission.
#if(ENABLE_WRITING_RANDOM_ZEROS_SUFFIX==1)
  if(ret > 0) {
    // Get random number of additional zero samples.
    uint32_t nof_random_samples = rand() % DEFAULT_SUBFRAME_LEN;
    ret = write(ch_emulator->wr_fd, ch_emulator->null_sample_vector, nof_random_samples*sizeof(cf_t));
    if(ret < 0) {
      CH_EMULATOR_ERROR("Write to named pipe returned: %d\n", ret);
    } else {
      nof_written_samples += ret/sizeof(cf_t);
    }
  }
#endif

#if(ENABLE_WRITING_ZEROS==1)
  // If number of samples is not a integer multiple of subframe_length then write zeros so that we have a multiple of that.
  if(nof_samples > ch_emulator->subframe_length && ret > 0) {
    // Calculate number of additional zero samples.
    additional_samples = ch_emulator->subframe_length - (nof_samples % ch_emulator->subframe_length);
    for(uint32_t i = 0; i < additional_samples; i++) {
      ret = write(ch_emulator->wr_fd, ch_emulator->null_sample_vector, sizeof(cf_t));
      if(ret < 0) {
        CH_EMULATOR_ERROR("Write to named pipe returned: %d\n", ret);
        break;
      } else {
        nof_written_samples++;
      }
    }
  }
#endif

  return nof_written_samples;
}

int channel_emulator_recv(void *h, void *data, uint32_t nof_samples, bool blocking) {

  channel_emulator_t *ch_emulator = (channel_emulator_t*)h;

  // Receive data.
  int ret = recv_samples(h, data, nof_samples);

  //if(ret > 0) CH_EMULATOR_PRINT("Read %d samples\n", ret);

  //if(ret > 0 && ret > ch_emulator->subframe_length) CH_EMULATOR_PRINT("Read %d samples...........................\n", ret);

  if(ret > 0) {
    // Apply channel to input signal if enabled.
    if(ch_emulator->enable_channel_impairments) {
      channel_cccf_execute_block(ch_emulator->chan.channel, data, nof_samples, data);
    } else {
      // Apply CFO to the signal.
      if(ch_emulator->chan.cfo_freq > 0.0) {
        srslte_cfo_correct_finer(&ch_emulator->chan.cfocorr, data, data, ch_emulator->chan.cfo_freq/((float)ch_emulator->chan.fft_size));
        CH_EMULATOR_INFO("Applying CFO of %f [Hz] to the subframe.\n", ch_emulator->chan.cfo_freq*15000.0);
      }
    }
  }

  return ret;
}

int recv_samples(void *h, void *data, uint32_t nof_samples) {
  channel_emulator_t *ch_emulator = (channel_emulator_t*)h;
  int ret;
  cf_t *samples = (cf_t*)data;
  // Read specified number of samples from named pipe.
  do {
    ret = read(ch_emulator->rd_fd, (void*)&samples[ch_emulator->nof_reads], (nof_samples-ch_emulator->nof_reads)*sizeof(cf_t));
    if(ret > 0) {
      ch_emulator->nof_reads += ret/sizeof(cf_t);
    } else {
      if(ret < 0) {
        return ret;
      }
    }
  } while(ch_emulator->nof_reads < nof_samples && ret != 0); // If zero is returned, then the writing side of the named fifo was closed.
  nof_samples = ch_emulator->nof_reads;
  // Reset number of reads counter.
  ch_emulator->nof_reads = 0;

  return nof_samples;
}
