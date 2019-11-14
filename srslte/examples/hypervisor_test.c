#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <unistd.h>
#include <sys/time.h>
#include <string.h>

#include "srslte/srslte.h"

#include "liquid/liquid.h"

#define MAX_SHORT_VALUE 32767.0

#define FILENAME "/home/zz4fap/work/mcf_tdma/scatter/build/phy/srslte/examples/scatter_iq_dump_plus_sensing_node_id_15_20180417_135234_970961.dat"

#define MATLAB_SCRIPT_NAME "channelizer.m"

#define SAMPLES_FILENAME "channelized_iq_samples.dat"

void convert_to_complex_float(_Complex short * input, _Complex float *output, unsigned long int length) {
  float re_f, im_f;
  short re_s, im_s;

  for(int i = 0; i < length; i++) {
    re_s =  __real__ input[i];
    re_f = (float)re_s/MAX_SHORT_VALUE;
    im_s = __imag__ input[i];
    im_f = (float)im_s/MAX_SHORT_VALUE;
    output[i] = re_f + (im_f * _Complex_I);
  }
}

void print_samples(_Complex float *samples, unsigned long int length) {
  for(int i = 0; i < length; i++) {
    printf("sample[%d]: (%f,%f)\n",i, __real__ samples[i], __imag__ samples[i]);
  }
}

void print_channelized_samples(_Complex float *y, unsigned int num_frames, unsigned int num_channels) {
  // Print channelized output signals.
  for(int i = 0; i < num_frames; i++) {
      for(int k = 0; k < num_channels; k++) {
          float complex v = y[i*num_channels + k];
          printf("y(%3u,%6u) = %12.4e + 1i*%12.4e\n", k+1, i+1, crealf(v), cimagf(v));
      }
  }
}

// export results to matlab script.
void create_matlab_file(_Complex float *y, unsigned int num_frames, unsigned int num_channels) {

  FILE *fid = fopen(MATLAB_SCRIPT_NAME,"w");
  fprintf(fid,"%% %s: auto-generated file\n\n", MATLAB_SCRIPT_NAME);
  fprintf(fid,"clear all;\n");
  fprintf(fid,"close all;\n");
  fprintf(fid,"num_channels = %u;\n", num_channels);
  fprintf(fid,"num_frames   = %u;\n", num_frames);

  fprintf(fid,"y = zeros(num_channels, num_frames);\n");

  // Save channelized output signals.
  for(int i = 0; i < num_frames; i++) {
    for(int k = 0; k < num_channels; k++) {
      float complex v = y[i*num_channels + k];
      fprintf(fid,"  y(%3u,%6u) = %12.4e + 1i*%12.4e;\n", k+1, i+1, crealf(v), cimagf(v));
    }
  }

  fclose(fid);
  printf("results written to %s\n", MATLAB_SCRIPT_NAME);
}

void create_file_with_samples(_Complex float *output, unsigned long int number_of_read_samples, char *output_file_name) {
  FILE *output_file;
  output_file = fopen(output_file_name, "w");

  for(int i = 0; i < number_of_read_samples; i++) {
    fprintf(output_file, "%f\t%f\n", __real__ output[i], __imag__ output[i]);

    printf("%d\t%f\t%f\n", i, __real__ output[i], __imag__ output[i]);
  }

  fclose(output_file);
  printf("results written to %s\n", output_file_name);
}

void create_file_with_channel_samples(_Complex float *output, unsigned int num_channels, unsigned long int number_of_frames, unsigned int channel, char *output_file_name) {
  FILE *output_file;
  output_file = fopen(output_file_name, "w");

  for(int i = 0; i < number_of_frames; i++) {
    fprintf(output_file, "%1.12f\t%1.12f\n", __real__ output[i*num_channels + channel], __imag__ output[i*num_channels + channel]);
  }

  fclose(output_file);
  printf("results written to %s\n", output_file_name);
}

void create_channel_files(_Complex float *output, unsigned int num_channels, unsigned long int number_of_frames) {
  char filename[200];
  for(int channel = 0; channel < num_channels; channel++) {
    sprintf(filename,"channelized_iq_samples_channel%d.dat",channel);
    create_file_with_channel_samples(output, num_channels, number_of_frames, channel, filename);
  }
}

void save_prototype_filter_into_file(float *h, unsigned int h_len, char *output_file_name) {
  FILE *fid;
  fid = fopen(output_file_name, "w");

  // Save prototype filter.
  for(int i = 0; i < h_len; i++) {
    fprintf(fid,"%12.4e\n",h[i]);
  }

  fclose(fid);
}

int main(int argc, char **argv) {

  srslte_filesource_t fsrc;
  unsigned int data_type = SRSLTE_COMPLEX_SHORT_BIN;
  unsigned long int number_of_read_samples;
  unsigned long int number_of_samples = 11520000;
  _Complex short *input_short;
  _Complex float *input_float;
  _Complex float *output;

  // Channelizer options.
  unsigned int num_channels = 12;     // number of channels
  unsigned int m            =  4;     // filter delay
  float        As           = 60;     // stop-band attenuation

  // Create prototype filter.
  unsigned int h_len = 2*num_channels*m + 1;
  float h[h_len];
  liquid_firdes_kaiser(h_len, 0.5f/(float)num_channels, As, 0.0f, h);

  // Create filterbank channelizer object using external filter coefficients.
  firpfbch_crcf q = firpfbch_crcf_create(LIQUID_ANALYZER, num_channels, 2*m, h);

  input_short = (_Complex short*)malloc(number_of_samples*sizeof(_Complex short));
  if(!input_short) {
    perror("malloc failed to allocate input_short.");
    exit(-1);
  }

  input_float = (_Complex float*)malloc(number_of_samples*sizeof(_Complex float));
  if(!input_float) {
    perror("malloc failed to allocate input_float.");
    exit(-1);
  }

  output = (_Complex float*)malloc(number_of_samples*sizeof(_Complex float));
  if(!output) {
    perror("malloc failed to allocate output.");
    exit(-1);
  }

  if(filesource_init(&fsrc, FILENAME, data_type)) {
    fprintf(stderr, "Error opening file %s\n", FILENAME);
    exit(-1);
  }

  // Read all the file.
  number_of_read_samples = filesource_read(&fsrc, input_short, number_of_samples);

  // Convert complex short into complex float.
  convert_to_complex_float(input_short, input_float, number_of_read_samples);

  // Channelize input data.
  unsigned int num_frames = number_of_read_samples/num_channels;  // number of frames
  printf("number_of_read_samples: %d - num_frames: %d\n",number_of_read_samples,num_frames);
  for(int i = 0; i < num_frames; i++) {
    // Execute analysis filter bank.
    firpfbch_crcf_analyzer_execute(q, &input_float[i*num_channels], &output[i*num_channels]);
  }

  //create_channel_files(output, num_channels, num_frames);

  save_prototype_filter_into_file(h, h_len, "prototype_fir_filter.dat");

  //print_channelized_samples(output, num_frames, num_channels);

  //create_file_with_samples(output, number_of_read_samples, SAMPLES_FILENAME);

  //create_matlab_file(output, num_frames, num_channels);

  // destroy channelizer object
  firpfbch_crcf_destroy(q);

  // Free source file.
  filesource_free(&fsrc);

  // Free vectors.
  free(input_short);
  free(input_float);
  free(output);

  return 0;
}
