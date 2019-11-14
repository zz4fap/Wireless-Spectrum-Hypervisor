/**
 *
 * \section COPYRIGHT
 *
 * Copyright 2013-2015 Software Radio Systems Limited
 *
 * \section LICENSE
 *
 * This file is part of the srsLTE library.
 *
 * srsLTE is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of
 * the License, or (at your option) any later version.
 *
 * srsLTE is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * A copy of the GNU Affero General Public License can be found in
 * the LICENSE file in the top-level directory of this distribution
 * and at http://www.gnu.org/licenses/.
 *
 */

#include <strings.h>
#include <string.h>
#include <stdlib.h>
#include <complex.h>
#include <math.h>

#include "srslte/sync/pss.h"
#include "srslte/dft/dft.h"
#include "srslte/utils/vector.h"
#include "srslte/utils/convolution.h"
#include "srslte/utils/debug.h"

int srslte_pss_synch_init_N_id_2(cf_t *pss_signal_freq, cf_t *pss_signal_time,
                                 uint32_t N_id_2, uint32_t fft_size, int cfo_i) {
  srslte_dft_plan_t plan;
  cf_t pss_signal_pad[2048];
  int ret = SRSLTE_ERROR_INVALID_INPUTS;

  if (srslte_N_id_2_isvalid(N_id_2)    &&
      fft_size                  <= 2048)
  {

    srslte_pss_generate(pss_signal_freq, N_id_2);

    bzero(pss_signal_pad, fft_size * sizeof(cf_t));
    bzero(pss_signal_time, fft_size * sizeof(cf_t));
    memcpy(&pss_signal_pad[(fft_size-SRSLTE_PSS_LEN)/2+cfo_i], pss_signal_freq, SRSLTE_PSS_LEN * sizeof(cf_t));

    /* Convert signal into the time domain */
    if (srslte_dft_plan(&plan, fft_size, SRSLTE_DFT_BACKWARD, SRSLTE_DFT_COMPLEX)) {
      return SRSLTE_ERROR;
    }

    srslte_dft_plan_set_mirror(&plan, true);
    srslte_dft_plan_set_dc(&plan, true);
    srslte_dft_plan_set_norm(&plan, true);
    srslte_dft_run_c(&plan, pss_signal_pad, pss_signal_time);

    srslte_vec_conj_cc(pss_signal_time, pss_signal_time, fft_size);
    srslte_vec_sc_prod_cfc(pss_signal_time, 1.0/SRSLTE_PSS_LEN, pss_signal_time, fft_size);

    srslte_dft_plan_free(&plan);

    ret = SRSLTE_SUCCESS;
  }
  return ret;
}

/* Initializes the PSS synchronization object with fft_size=128
 */
int srslte_pss_synch_init(srslte_pss_synch_t *q, uint32_t frame_size) {
  return srslte_pss_synch_init_fft(q, frame_size, 128);
}

int srslte_pss_synch_init_fft(srslte_pss_synch_t *q, uint32_t frame_size, uint32_t fft_size) {
  return srslte_pss_synch_init_fft_offset(q, frame_size, fft_size, 0);
}

int srslte_pss_synch_init_fft_vphy(srslte_pss_synch_t *q, uint32_t frame_size, uint32_t fft_size, uint32_t vphy_id, uint32_t vphy_id_to_plot) {
  return srslte_pss_synch_init_fft_offset_vphy(q, frame_size, fft_size, 0, vphy_id, vphy_id_to_plot);
}

/* Initializes the PSS synchronization object.
 *
 * It correlates a signal of frame_size samples with the PSS sequence in the frequency
 * domain. The PSS sequence is transformed using fft_size samples.
 */
int srslte_pss_synch_init_fft_offset(srslte_pss_synch_t *q, uint32_t frame_size, uint32_t fft_size, int offset) {
  return srslte_pss_synch_init_fft_offset_vphy(q, frame_size, fft_size, offset, 0, 0);
}

int srslte_pss_synch_init_fft_offset_vphy(srslte_pss_synch_t *q, uint32_t frame_size, uint32_t fft_size, int offset, uint32_t vphy_id, uint32_t vphy_id_to_plot) {
  int ret = SRSLTE_ERROR_INVALID_INPUTS;

  if (q != NULL) {

    ret = SRSLTE_ERROR;

    uint32_t N_id_2;
    uint32_t buffer_size;
    bzero(q, sizeof(srslte_pss_synch_t));

    q->N_id_2 = 10;
    q->fft_size = fft_size;
    q->frame_size = frame_size;
    q->ema_alpha = 0.2;
    q->vphy_id = vphy_id;
    q->vphy_id_to_plot = vphy_id_to_plot;
    q->conv_output_avg = NULL;

    buffer_size = fft_size + frame_size + 1;

    if(srslte_dft_plan(&q->dftp_input, fft_size, SRSLTE_DFT_FORWARD, SRSLTE_DFT_COMPLEX)) {
      fprintf(stderr, "Error creating DFT plan \n");
      goto clean_and_exit;
    }
    srslte_dft_plan_set_mirror(&q->dftp_input, true);
    srslte_dft_plan_set_dc(&q->dftp_input, true);
    srslte_dft_plan_set_norm(&q->dftp_input, true);

    q->tmp_input = (cf_t*)srslte_vec_malloc(buffer_size * sizeof(cf_t));
    if(!q->tmp_input) {
      fprintf(stderr, "Error allocating memory\n");
      goto clean_and_exit;
    }
    bzero(&q->tmp_input[q->frame_size], q->fft_size * sizeof(cf_t));

    q->conv_output = (cf_t*)srslte_vec_malloc(buffer_size * sizeof(cf_t));
    if(!q->conv_output) {
      fprintf(stderr, "Error allocating memory\n");
      goto clean_and_exit;
    }
    bzero(q->conv_output, sizeof(cf_t) * buffer_size);

    q->conv_output_avg = (float*)srslte_vec_malloc(buffer_size * sizeof(float));
    if(!q->conv_output_avg) {
      fprintf(stderr, "Error allocating memory\n");
      goto clean_and_exit;
    }
    bzero(q->conv_output_avg, sizeof(float) * buffer_size);

#if(ENBALE_VPHY_RX_INFO_PLOT==1)
    if(q->vphy_id == q->vphy_id_to_plot) {
      q->conv_output_avg_plot = (float*)srslte_vec_malloc(buffer_size * sizeof(float));
      if (!q->conv_output_avg_plot) {
        fprintf(stderr, "Error allocating memory\n");
        goto clean_and_exit;
      }
      bzero(q->conv_output_avg_plot, sizeof(float) * buffer_size);
    } else {
      q->conv_output_avg_plot = NULL;
    }
#endif

#ifdef SRSLTE_PSS_ACCUMULATE_ABS
    q->conv_output_abs = (float*)srslte_vec_malloc(buffer_size * sizeof(float));
    if(!q->conv_output_abs) {
      fprintf(stderr, "Error allocating memory\n");
      goto clean_and_exit;
    }
    bzero(q->conv_output_abs, sizeof(float)*buffer_size);
#endif

    for(N_id_2 = 0; N_id_2 < 3; N_id_2++) {
      q->pss_signal_time[N_id_2] = (cf_t*)srslte_vec_malloc(buffer_size*sizeof(cf_t));
      if(!q->pss_signal_time[N_id_2]) {
        fprintf(stderr, "Error allocating memory\n");
        goto clean_and_exit;
      }
      // The PSS is translated into the time domain for each N_id_2.
      if(srslte_pss_synch_init_N_id_2(q->pss_signal_freq[N_id_2], q->pss_signal_time[N_id_2], N_id_2, fft_size, offset)) {
        fprintf(stderr, "Error initiating PSS detector for N_id_2=%d fft_size=%d\n", N_id_2, fft_size);
        goto clean_and_exit;
      }
      // Zero-pad the remaining part of the vector.
      bzero(&q->pss_signal_time[N_id_2][q->fft_size], q->frame_size*sizeof(cf_t));
    }

#ifdef CONVOLUTION_FFT
    if(srslte_conv_fft_cc_init(&q->conv_fft, frame_size, fft_size)) {
      fprintf(stderr, "Error initiating convolution FFT\n");
      goto clean_and_exit;
    }
#endif

    // Allocate memory for holding frequency domain received PSS sequence.
    q->rx_pss_fft = (cf_t*)srslte_vec_malloc(fft_size*sizeof(cf_t));
    if(!q->rx_pss_fft) {
      fprintf(stderr, "Error allocating memory for freq. domain PSS sequence.\n");
      goto clean_and_exit;
    }
    bzero(q->rx_pss_fft, fft_size*sizeof(cf_t));

    srslte_pss_synch_reset(q);

    ret = SRSLTE_SUCCESS;
  }

clean_and_exit:
  if(ret == SRSLTE_ERROR) {
    srslte_pss_synch_free(q);
  }
  return ret;
}

void srslte_pss_synch_free(srslte_pss_synch_t *q) {
  uint32_t i;

  if (q) {
    for (i=0;i<3;i++) {
      if (q->pss_signal_time[i]) {
        free(q->pss_signal_time[i]);
      }
    }
#ifdef CONVOLUTION_FFT
    srslte_conv_fft_cc_free(&q->conv_fft);
#endif
    if (q->tmp_input) {
      free(q->tmp_input);
    }
    if (q->conv_output) {
      free(q->conv_output);
    }
#ifdef SRSLTE_PSS_ACCUMULATE_ABS
    if (q->conv_output_abs) {
      free(q->conv_output_abs);
    }
#endif
    if (q->conv_output_avg) {
      free(q->conv_output_avg);
    }
#if(ENBALE_VPHY_RX_INFO_PLOT==1)
    if(q->vphy_id == q->vphy_id_to_plot) {
      if(q->conv_output_avg_plot) {
        free(q->conv_output_avg_plot);
      }
    }
#endif
    if(q->rx_pss_fft) {
      free(q->rx_pss_fft);
    }

    srslte_dft_plan_free(&q->dftp_input);

    bzero(q, sizeof(srslte_pss_synch_t));
  }
}

void srslte_pss_synch_reset(srslte_pss_synch_t *q) {
  uint32_t buffer_size = q->fft_size + q->frame_size + 1;
#if(ENBALE_VPHY_RX_INFO_PLOT==1)
  if(q->vphy_id == q->vphy_id_to_plot) {
    memcpy(q->conv_output_avg_plot, q->conv_output_avg, sizeof(float)*buffer_size);
  }
#endif
  bzero(q->conv_output_avg, sizeof(float) * buffer_size);
}

/**
 * This function calculates the Zadoff-Chu sequence.
 * @param signal Output array.
 */
int srslte_pss_generate(cf_t *signal, uint32_t N_id_2) {
  int i;
  float arg;
  const float root_value[] = { 25.0, 29.0, 34.0 };
  int root_idx;

  int sign = -1;

  if (N_id_2 > 2) {
    fprintf(stderr, "Invalid N_id_2 %d\n", N_id_2);
    return -1;
  }

  root_idx = N_id_2;

  for (i = 0; i < SRSLTE_PSS_LEN / 2; i++) {
    arg = (float) sign * M_PI * root_value[root_idx]
        * ((float) i * ((float) i + 1.0)) / 63.0;
    __real__ signal[i] = cosf(arg);
    __imag__ signal[i] = sinf(arg);
  }
  for (i = SRSLTE_PSS_LEN / 2; i < SRSLTE_PSS_LEN; i++) {
    arg = (float) sign * M_PI * root_value[root_idx]
        * (((float) i + 2.0) * ((float) i + 1.0)) / 63.0;
    __real__ signal[i] = cosf(arg);
    __imag__ signal[i] = sinf(arg);
  }
  return 0;
}

/** 36.211 10.3 section 6.11.1.2
 */
void srslte_pss_put_slot(cf_t *pss_signal, cf_t *slot, uint32_t nof_prb, srslte_cp_t cp) {
  int k;
  k = (SRSLTE_CP_NSYMB(cp) - 1) * nof_prb * SRSLTE_NRE + nof_prb * SRSLTE_NRE / 2 - 31;
  memset(&slot[k - 5], 0, 5 * sizeof(cf_t));
  memcpy(&slot[k], pss_signal, SRSLTE_PSS_LEN * sizeof(cf_t));
  memset(&slot[k + SRSLTE_PSS_LEN], 0, 5 * sizeof(cf_t));
}

void srslte_pss_get_slot(cf_t *slot, cf_t *pss_signal, uint32_t nof_prb, srslte_cp_t cp) {
  int k;
  k = (SRSLTE_CP_NSYMB(cp) - 1) * nof_prb * SRSLTE_NRE + nof_prb * SRSLTE_NRE / 2 - 31;
  memcpy(pss_signal, &slot[k], SRSLTE_PSS_LEN * sizeof(cf_t));
}

/** Sets the current N_id_2 value. Returns -1 on error, 0 otherwise
 */
int srslte_pss_synch_set_N_id_2(srslte_pss_synch_t *q, uint32_t N_id_2) {
  if (!srslte_N_id_2_isvalid((N_id_2))) {
    fprintf(stderr, "Invalid N_id_2 %d\n", N_id_2);
    return -1;
  } else {
    q->N_id_2 = N_id_2;
    return 0;
  }
}

/* Sets the weight factor alpha for the exponential moving average of the PSS correlation output
 */
void srslte_pss_synch_set_ema_alpha(srslte_pss_synch_t *q, float alpha) {
  q->ema_alpha = alpha;
}

/** Performs time-domain PSS correlation.
 * Returns the index of the PSS correlation peak in a subframe.
 * The frame starts at corr_peak_pos-subframe_size/2.
 * The value of the correlation is stored in corr_peak_value.
 *
 * Input buffer must be subframe_size long.
 */
int srslte_pss_synch_find_pss(srslte_pss_synch_t *q, cf_t *input, float *corr_peak_value)
{
  int ret = SRSLTE_ERROR_INVALID_INPUTS;

  if (q                 != NULL  &&
      input             != NULL)
  {

    uint32_t corr_peak_pos = 0;
    uint32_t conv_output_len = 0;

    if(!srslte_N_id_2_isvalid(q->N_id_2)) {
      fprintf(stderr, "Error finding PSS peak, Must set N_id_2 first\n");
      return SRSLTE_ERROR;
    }

    /* Correlate input with PSS sequence
     *
     * We do not reverse time-domain PSS signal because it's conjugate is symmetric.
     * The conjugate operation on pss_signal_time has been done in srslte_pss_synch_init_N_id_2
     * This is why we can use FFT-based convolution
     */
    if(q->frame_size >= q->fft_size) {
#ifdef CONVOLUTION_FFT
      memcpy(q->tmp_input, input, q->frame_size*sizeof(cf_t));
      conv_output_len = srslte_conv_fft_cc_run(&q->conv_fft, q->tmp_input, q->pss_signal_time[q->N_id_2], q->conv_output);
#else
      conv_output_len = srslte_conv_cc(input, q->pss_signal_time[q->N_id_2], q->conv_output, q->frame_size, q->fft_size);
#endif
    } else {
      for(int i = 0; i < q->frame_size; i++) {
        q->conv_output[i] = srslte_vec_dot_prod_ccc(q->pss_signal_time[q->N_id_2], &input[i], q->fft_size);
      }
      conv_output_len = q->frame_size;
    }

#ifdef SRSLTE_PSS_ABS_SQUARE
       // Compute modulus square.
      srslte_vec_abs_square_cf(q->conv_output, q->conv_output_abs, conv_output_len-1);
#else
      srslte_vec_abs_cf(q->conv_output, q->conv_output_abs, conv_output_len-1);
#endif

    // If enabled, average the absolute value from previous calls.
    if(q->ema_alpha < 1.0 && q->ema_alpha > 0.0) {
      srslte_vec_sc_prod_fff(q->conv_output_abs, q->ema_alpha, q->conv_output_abs, conv_output_len-1);
      srslte_vec_sc_prod_fff(q->conv_output_avg, 1-q->ema_alpha, q->conv_output_avg, conv_output_len-1);

      srslte_vec_sum_fff(q->conv_output_abs, q->conv_output_avg, q->conv_output_avg, conv_output_len-1);
    } else {
      memcpy(q->conv_output_avg, q->conv_output_abs, sizeof(float)*(conv_output_len-1));
    }
    // Find maximum of the absolute value of the correlation.
    corr_peak_pos = srslte_vec_max_fi(q->conv_output_avg, conv_output_len-1);

    // Save absolute value.
    q->peak_value = q->conv_output_avg[corr_peak_pos];

#ifdef SRSLTE_PSS_RETURN_PSR
    // Find second side lobe.

    // Find end of peak lobe to the right.
    int pl_ub = corr_peak_pos+1;
    while(q->conv_output_avg[pl_ub+1] <= q->conv_output_avg[pl_ub] && pl_ub < conv_output_len) {
      pl_ub ++;
    }
    // Find end of peak lobe to the left.
    int pl_lb;
    if(corr_peak_pos > 2) {
      pl_lb = corr_peak_pos-1;
        while(q->conv_output_avg[pl_lb-1] <= q->conv_output_avg[pl_lb] && pl_lb > 1) {
        pl_lb --;
      }
    } else {
      pl_lb = 0;
    }

    int sl_distance_right = conv_output_len-1-pl_ub;
    if(sl_distance_right < 0) {
      sl_distance_right = 0;
    }
    int sl_distance_left = pl_lb;

    int sl_right = pl_ub+srslte_vec_max_fi(&q->conv_output_avg[pl_ub], sl_distance_right);
    int sl_left = srslte_vec_max_fi(q->conv_output_avg, sl_distance_left);
    float side_lobe_value = SRSLTE_MAX(q->conv_output_avg[sl_right], q->conv_output_avg[sl_left]);
    if(corr_peak_value) {
      *corr_peak_value = q->conv_output_avg[corr_peak_pos]/side_lobe_value;

      if(*corr_peak_value < 10)
        DEBUG("peak_pos=%2d, pl_ub=%2d, pl_lb=%2d, sl_right: %2d, sl_left: %2d, PSR: %.2f/%.2f=%.2f\n", corr_peak_pos, pl_ub, pl_lb,
             sl_right,sl_left, q->conv_output_avg[corr_peak_pos], side_lobe_value,*corr_peak_value);
    }
#else
    if(corr_peak_value) {
      *corr_peak_value = q->conv_output_avg[corr_peak_pos];
    }
#endif

    if(q->frame_size >= q->fft_size) {
      ret = (int) corr_peak_pos;
    } else {
      ret = (int) corr_peak_pos + q->fft_size;
    }
  }
  return ret;
}

/* Computes frequency-domain channel estimation of the PSS symbol
 * input signal is in the time-domain.
 * ce is the returned frequency-domain channel estimates.
 */
int srslte_pss_synch_chest(srslte_pss_synch_t *q, cf_t *input, cf_t ce[SRSLTE_PSS_LEN]) {
  int ret = SRSLTE_ERROR_INVALID_INPUTS;

  if(q != NULL && input != NULL) {
    // Check if Nid2 is a valid number.
    if (!srslte_N_id_2_isvalid(q->N_id_2)) {
      fprintf(stderr, "Error finding PSS peak, Must set N_id_2 first\n");
      return SRSLTE_ERROR;
    }
    // Transform to frequency-domain.
    srslte_dft_run_c(&q->dftp_input, input, q->rx_pss_fft);
    // Compute channel estimate taking the PSS sequence as reference.
    srslte_vec_prod_conj_ccc(&q->rx_pss_fft[(q->fft_size-SRSLTE_PSS_LEN)/2], q->pss_signal_freq[q->N_id_2], ce, SRSLTE_PSS_LEN);
    ret = SRSLTE_SUCCESS;
  }
  return ret;
}

/* Returns the CFO estimation given a PSS received sequence
 *
 * Source: An Efﬁcient CFO Estimation Algorithm for the Downlink of 3GPP-LTE
 *       Feng Wang and Yu Zhu
 */
float srslte_pss_synch_cfo_compute(srslte_pss_synch_t* q, cf_t *pss_recv) {
  cf_t y0, y1;

  y0 = srslte_vec_dot_prod_ccc(q->pss_signal_time[q->N_id_2], pss_recv, q->fft_size/2);
  y1 = srslte_vec_dot_prod_ccc(&q->pss_signal_time[q->N_id_2][q->fft_size/2], &pss_recv[q->fft_size/2], q->fft_size/2);

  // The carg functions seems more realible than atan2f.
  return cargf(conjf(y0) * y1)/M_PI;
}

// Improvement on the previous PSS-based CFO estimation.
float srslte_pss_synch_cfo_compute2(srslte_pss_synch_t* q, cf_t *pss_recv) {
  cf_t y[4];
  float cfo_est[6];

  y[0] = srslte_vec_dot_prod_ccc(&q->pss_signal_time[q->N_id_2][0*q->fft_size/4], &pss_recv[0*q->fft_size/4], q->fft_size/4);
  y[1] = srslte_vec_dot_prod_ccc(&q->pss_signal_time[q->N_id_2][1*q->fft_size/4], &pss_recv[1*q->fft_size/4], q->fft_size/4);
  y[2] = srslte_vec_dot_prod_ccc(&q->pss_signal_time[q->N_id_2][2*q->fft_size/4], &pss_recv[2*q->fft_size/4], q->fft_size/4);
  y[3] = srslte_vec_dot_prod_ccc(&q->pss_signal_time[q->N_id_2][3*q->fft_size/4], &pss_recv[3*q->fft_size/4], q->fft_size/4);

  cfo_est[0] = cargf(conjf(y[0]) * y[1])/(1*(M_PI/2));
  cfo_est[1] = cargf(conjf(y[0]) * y[2])/(2*(M_PI/2));
  cfo_est[2] = cargf(conjf(y[0]) * y[3])/(3*(M_PI/2));
  cfo_est[3] = cargf(conjf(y[1]) * y[2])/(1*(M_PI/2));
  cfo_est[4] = cargf(conjf(y[1]) * y[3])/(2*(M_PI/2));
  cfo_est[5] = cargf(conjf(y[2]) * y[3])/(1*(M_PI/2));

  float ext_cfo_est = srslte_vec_acc_ff(cfo_est, 6);

  // The carg functions seems more realible than atan2f.
  return ext_cfo_est/6;
}

// *****************************************************************************
// Implementations for McF-TDMA.
// *****************************************************************************

// Implementation for McF-TDMA with 20 MHz PHY channels.
// Loosely based on TS 36.211 10.3 section 6.11.1.2
void srslte_insert_pss_into_subframe(cf_t *pss_signal, cf_t *slot, uint32_t radio_nof_prb, uint32_t nof_prb, uint32_t channel, srslte_cp_t cp) {
  int k;
  k = (SRSLTE_CP_NSYMB(cp) - 1) * radio_nof_prb * SRSLTE_NRE + nof_prb * SRSLTE_NRE / 2 - 31 + channel*nof_prb*SRSLTE_NRE;
  memset(&slot[k - 5], 0, 5 * sizeof(cf_t));
  memcpy(&slot[k], pss_signal, SRSLTE_PSS_LEN * sizeof(cf_t));
  memset(&slot[k + SRSLTE_PSS_LEN], 0, 5 * sizeof(cf_t));
}

// Loosely based on TS 36.211 10.3 section 6.11.1.2
// Neither DC nor the null (i.e., unused) subcarriers at the edges of PSS have to be set here as the resource grid is always set to 0 after a transmission.
void srslte_insert_pss_into_subframe_with_dc(cf_t *pss_signal, cf_t *slot, uint32_t radio_fft_len, int dc_pos, srslte_cp_t cp) {
  // Calculate start of lower part.
  int lower_part_idx = dc_pos - SRSLTE_HALF_PSS_LEN;
  if(lower_part_idx < 0) {
    lower_part_idx = lower_part_idx + radio_fft_len;
  }
  lower_part_idx = lower_part_idx + (SRSLTE_CP_NSYMB(cp) - 1)*radio_fft_len;
  // Calculate start of upper part.
  int upper_part_idx = dc_pos + 1 + (SRSLTE_CP_NSYMB(cp) - 1)*radio_fft_len;
  // Insert to grid first half of the PSS sequence.
  memcpy(&slot[lower_part_idx], pss_signal, SRSLTE_HALF_PSS_LEN*sizeof(cf_t));
  // Insert to grid second half of the PSS sequence.
  memcpy(&slot[upper_part_idx], &pss_signal[SRSLTE_HALF_PSS_LEN], SRSLTE_HALF_PSS_LEN*sizeof(cf_t));
}

// Loosely based on TS 36.211 10.3 section 6.11.1.2
// Neither DC nor the null (i.e., unused) subcarriers at the edges of PSS have to be set here as the resource grid is always set to 0 after a transmission.
void srslte_insert_pss_into_subframe_with_dc2(cf_t *pss_signal, cf_t *slot, int lower_part_idx, int upper_part_idx) {
  // Insert to grid first half of the PSS sequence.
  memcpy(&slot[lower_part_idx], pss_signal, SRSLTE_HALF_PSS_LEN*sizeof(cf_t));
  // Insert to grid second half of the PSS sequence.
  memcpy(&slot[upper_part_idx], &pss_signal[SRSLTE_HALF_PSS_LEN], SRSLTE_HALF_PSS_LEN*sizeof(cf_t));
}

void srslte_calculate_pss_lower_and_upper_indexes(uint32_t radio_fft_len, uint32_t vphy_fft_len, uint32_t channel, srslte_cp_t cp, int *lower_part_idx, int *upper_part_idx) {
  // Calculate DC subcarrier position.
  int dc_pos = channel*vphy_fft_len;
  // Calculate start of lower part.
  *lower_part_idx = dc_pos - SRSLTE_HALF_PSS_LEN;
  if(*lower_part_idx < 0) {
    *lower_part_idx = *lower_part_idx + radio_fft_len;
  }
  *lower_part_idx = *lower_part_idx + (SRSLTE_CP_NSYMB(cp) - 1)*radio_fft_len;
  // Calculate start of upper part.
  *upper_part_idx = dc_pos + 1 + (SRSLTE_CP_NSYMB(cp) - 1)*radio_fft_len;
}

//---------------------------------------------------------
//******* Scatter System Customized Implementations *******
//---------------------------------------------------------
// Retrieve the accumulated noise power from null subcarriers up and above the PSS.
float srslte_pss_synch_acc_noise(srslte_pss_synch_t *q) {
  float acc_noise_power = 0.0;
  // Accumulate noise power based on the 5 null subcarriers below the PSS.
  acc_noise_power += crealf(srslte_vec_dot_prod_conj_ccc(&q->rx_pss_fft[((q->fft_size-SRSLTE_PSS_LEN)/2) - 5], &q->rx_pss_fft[((q->fft_size-SRSLTE_PSS_LEN)/2) - 5], 5));
  // Accumulate noise power based on the 5 null subcarriers above the PSS.
  acc_noise_power += crealf(srslte_vec_dot_prod_conj_ccc(&q->rx_pss_fft[((q->fft_size+SRSLTE_PSS_LEN)/2)],     &q->rx_pss_fft[((q->fft_size+SRSLTE_PSS_LEN)/2)],     5));
  return acc_noise_power;
}

/* Computes frequency-domain channel estimation based on PSS symbols.
 * input signal is in the time-domain containing a supposedly received PSS signal.
 * ce is the returned frequency-domain channel estimates.
 */
int srslte_pss_synch_chest_generic(srslte_pss_synch_t *q, cf_t *input, cf_t *ce, uint32_t len, uint32_t offset) {
  int ret = SRSLTE_ERROR_INVALID_INPUTS;

  if(q != NULL && input != NULL && ce != NULL) {
    // Check if Nid2 is a valid number.
    if(!srslte_N_id_2_isvalid(q->N_id_2)) {
      fprintf(stderr, "Error finding PSS peak, Must set N_id_2 first\n");
      return SRSLTE_ERROR;
    }

    // Transform received PSS into frequency-domain.
    srslte_dft_run_c(&q->dftp_input, input, q->rx_pss_fft);

#if(0)
    printf("---------------------- Received PSS ---------------------- \n");
    for(int k = -2; k < SRSLTE_PSS_LEN+2; k++) {
      printf("%d - %f,%f\n", k, creal(q->rx_pss_fft[((q->fft_size-SRSLTE_PSS_LEN)/2) + k]), cimag(q->rx_pss_fft[((q->fft_size-SRSLTE_PSS_LEN)/2) + k]));
    }
    printf("\n\n---------------------- Local PSS ---------------------- \n");
    for(int k = 0; k < SRSLTE_PSS_LEN; k++) {
      printf("%d - %f,%f\n", k, creal(q->pss_signal_freq[q->N_id_2][k]), cimag(q->pss_signal_freq[q->N_id_2][k]));
    }
#endif

    // Compute channel estimate taking the PSS sequence as reference.
    srslte_vec_prod_conj_ccc(&q->rx_pss_fft[((q->fft_size-SRSLTE_PSS_LEN)/2) + offset], &q->pss_signal_freq[q->N_id_2][offset], ce, len);

#if(0)
    printf("\n\n---------------------- PSS-based CE ---------------------- \n");
    for(int k = 0; k < len; k++) {
      printf("%d - %f,%f\n", k, creal(ce[k]), cimag(ce[k]));
    }
#endif

    ret = SRSLTE_SUCCESS;
  }
  return ret;
}
