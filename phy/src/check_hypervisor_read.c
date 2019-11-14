#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/time.h>
#define __STDC_FORMAT_MACROS
#include <inttypes.h>
#include <time.h>
#include <semaphore.h>
#include <assert.h>
#include <signal.h>
#include <pthread.h>
#include <string.h>

#include "srslte/srslte.h"
#include "srslte/intf/intf.h"
#include "../phy/helpers.h"
#include "../../../../communicator/cpp/communicator_wrapper.h"
#include "../phy/hypervisor_rx.h"

#define MAX_VALUE 23040

#if(ENABLE_CHANNELIZER_THREAD==1)

static hypervisor_rx_t* handle = NULL;

cf_t *data[NOF_CHANNELIZER_BUFFERS];

int recv_check(void *h, void *data, uint32_t nof_samples);
void set_channel(uint32_t vphy_id, uint32_t channel);
uint32_t get_channel(uint32_t vphy_id);
void print_channel_buffer();

bool go_exit = false;

void sig_int_handler(int signo) {
  if(signo == SIGINT) {
    go_exit = true;
    printf("SIGINT received. Exiting...\n",0);
  }
}

void initialize_signal_handler() {
  sigset_t sigset;
  sigemptyset(&sigset);
  sigaddset(&sigset, SIGINT);
  sigprocmask(SIG_UNBLOCK, &sigset, NULL);
  signal(SIGINT, sig_int_handler);
}

void initialize_test_resources() {
  // Allocate memory for a hypervisor Rx object;
  handle = (hypervisor_rx_t*)srslte_vec_malloc(sizeof(hypervisor_rx_t));
  if(handle == NULL) {
    printf("Error when allocating memory for handle\n");
    exit(-1);
  }

  handle->num_of_rx_vphys = 1;
  // Set all channels in the list to zero.
  bzero(handle->channel_list, MAX_NUM_CONCURRENT_VPHYS*sizeof(uint32_t));
  // Number of samples to read.
  handle->nof_samples_to_read = 12*1920;
  handle->channelizer_nof_channels = 12;
  handle->channelizer_nof_frames = 1920;

  // Allocate memory for output buffers.
  for(uint32_t k = 0; k < NOF_CHANNELIZER_BUFFERS; k++) {
    handle->channelizer_buffer[k] = (cf_t*)srslte_vec_malloc(sizeof(cf_t)*handle->nof_samples_to_read);
    if(handle->channelizer_buffer[k] == NULL) {
      printf("Error when allocating memory for channelizer_buffer", 0);
      exit(-1);
    }
  }

  // Initialize channel buffers.
  uint32_t value = 0;
  for(uint32_t ch = 0; ch < NOF_CHANNELIZER_BUFFERS; ch++) {
    for(uint32_t k = 0; k < handle->nof_samples_to_read; k++) {
      value = (ch*handle->nof_samples_to_read + k) % MAX_VALUE;
      handle->channelizer_buffer[ch][k] = ((1.0 + 1.0*I)*((float)value));
    }
  }

  // Allocate memory for data buffer.
  for(uint32_t k = 0; k < NOF_CHANNELIZER_BUFFERS; k++) {
    data[k] = (cf_t*)srslte_vec_malloc(sizeof(cf_t)*handle->nof_samples_to_read);
    if(data[k] == NULL) {
      printf("Error when allocating memory for data", 0);
      exit(-1);
    }
  }
}

void free_test_resources() {
  // Free memory used to store channelizer_buffer base band IQ samples.
  for(uint32_t k = 0; k < NOF_CHANNELIZER_BUFFERS; k++) {
    if(handle->channelizer_buffer[k]) {
      free(handle->channelizer_buffer[k]);
      handle->channelizer_buffer[k] = NULL;
    }
  }

  // Free memory used to store channelizer_buffer base band IQ samples.
  for(uint32_t k = 0; k < NOF_CHANNELIZER_BUFFERS; k++) {
    if(data[k]) {
      free(data[k]);
      data[k] = NULL;
    }
  }

  // Free memory used to store hypervisor Rx object.
  if(handle) {
    free(handle);
    handle = NULL;
  }
}

void print_channel_buffer() {
  for(uint32_t k = 0; k < NOF_CHANNELIZER_BUFFERS; k++) {
    for(uint32_t j = 0; j < handle->nof_samples_to_read; j++) {
      printf("buffer[%d][%d]: %f,%f\n", k, j, __real__ (handle->channelizer_buffer[k][j]), __imag__ (handle->channelizer_buffer[k][j]));
    }
  }
  printf("--------------------\n");
}

void print_data(cf_t *data, uint32_t nof_samples) {
  printf("---------------------------------------\n");
  for(uint32_t k = 0; k < nof_samples; k++) {
    printf("data[%d]: %f,%f\n", k, __real__ (data[k]), __imag__ (data[k]));
  }
  printf("---------------------------------------\n");
}

int main() {

  vphy_reader_t vphy_reader;
  int ret = 0;
  uint32_t nof_samples = 0, channel = 11, idx = 0, trial = 0, expected_value = channel, vphy_id = 0;

  vphy_reader.vphy_channel = channel;
  vphy_reader.current_buffer_idx = 0;
  vphy_reader.nof_remaining_samples = 1920;
  vphy_reader.nof_read_samples = 0;

  // Initialize signal handler.
  go_exit = false;
  initialize_signal_handler();

  // Initialize everyhting needed for testing.
  initialize_test_resources();

  // Set channel number.
  set_channel(vphy_id, channel);

  printf("channel: %d - expected value: %d\n", channel, expected_value);

  // Loop forever until error occurs or control+c.
  while(!go_exit) {

    nof_samples = (rand() % 3*1920) + 1;

    ret = recv_check((void *)&vphy_reader, (void *)&data[idx][0], nof_samples);

    // Check if function returned the expected values.
    for(uint32_t k = 0; k < nof_samples; k++) {
      if((data[idx][k]) != (expected_value*(1.0 + 1.0*I))) {
        printf("Trial: %d - Wrong value... nof_samples: %d - expected: %f,%f - actual: %f,%f - current_buffer_idx: %d\n", trial, nof_samples, __real__ (expected_value*(1.0 + 1.0*I)), __imag__ (expected_value*(1.0 + 1.0*I)), __real__ (data[idx][k]), __imag__ (data[idx][k]), vphy_reader.current_buffer_idx);
        exit(-1);
      }
      expected_value = (expected_value + handle->channelizer_nof_channels) % MAX_VALUE;
    }

    bzero(&data[idx][0], sizeof(cf_t)*handle->channelizer_nof_frames);

    // Check if function returned the expected number of samples.
    if(ret != nof_samples) {
      printf("Trial: %d - Error reading.... exptected: %d - actual: %d\n", trial, nof_samples, ret);
      exit(-1);
    }

    if(trial % 10000 == 0) {
      printf("Trial #: %d\n",trial);
    }

    trial++;
  }

  // Free all resources allocated for testing.
  free_test_resources();

  return 0;
}


void set_channel(uint32_t vphy_id, uint32_t channel) {
  handle->channel_list[vphy_id] = channel;
}

uint32_t get_channel(uint32_t vphy_id) {
  uint32_t channel = 0;
  channel = handle->channel_list[vphy_id];
  return channel;
}

bool wait_container_not_empty() {
  return true;
}

void get_channel_buffer_context_inc_nof_reads(channel_buffer_context_t* const channel_buffer_ctx) {
  static uint32_t rd_idx = 0;
  channel_buffer_ctx->channel_buffer_rd_pos = rd_idx;
  rd_idx = (rd_idx + 1) % NOF_CHANNELIZER_BUFFERS;
}

int recv_check(void *h, void *data, uint32_t nof_samples) {

  vphy_reader_t *vphy_reader = (vphy_reader_t*)h;
  uint32_t channel = 0, nof_samples_to_read = 0, total_nof_read_samples = 0, curr_nof_samples = nof_samples;
  channel_buffer_context_t channel_buffer_ctx;
  cf_t *samples = (cf_t*)data;

  do {

    if(vphy_reader->nof_read_samples == 0) {
      // Wait until container is not empty.
      wait_container_not_empty();
      // Retrieve the front element.
      get_channel_buffer_context_inc_nof_reads(&channel_buffer_ctx);
      // Update reader to structure to hold the current buffer to be read.
      vphy_reader->current_buffer_idx = channel_buffer_ctx.channel_buffer_rd_pos;
      // Number of remaining samples must be equal to the number of frames.
      vphy_reader->nof_remaining_samples = handle->channelizer_nof_frames;
    }

    // Retrieve current channel for this vPHY.
    channel = vphy_reader->vphy_channel;
    if(channel > handle->channelizer_nof_channels) {
      channel = 0;
    }

    // Calculate how many samples can we read from the current buffer to attend the function's request.
    if(curr_nof_samples > vphy_reader->nof_remaining_samples) {
      nof_samples_to_read = vphy_reader->nof_remaining_samples;
    } else {
      nof_samples_to_read = curr_nof_samples;
    }

    // Read specified number of samples from specific channel buffer.
    for(uint32_t k = vphy_reader->nof_read_samples; k < (vphy_reader->nof_read_samples + nof_samples_to_read); k++) {
      *samples = handle->channelizer_buffer[vphy_reader->current_buffer_idx][k*handle->channelizer_nof_channels + channel];
      samples++;
    }

    // Update number of remaining samples.
    vphy_reader->nof_remaining_samples -= nof_samples_to_read;

    // Update number of read samples.
    vphy_reader->nof_read_samples += nof_samples_to_read;
    // If read the whole buffer content, then we should go to the next channel buffer position.
    if(vphy_reader->nof_read_samples >= handle->channelizer_nof_frames) {
      vphy_reader->nof_read_samples = 0;
    }

    curr_nof_samples -= nof_samples_to_read;

    total_nof_read_samples += nof_samples_to_read;

  } while(total_nof_read_samples < nof_samples);

  return total_nof_read_samples;
}

#endif
