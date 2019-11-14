#include <stdio.h>
#define __STDC_FORMAT_MACROS
#include <inttypes.h>
#include <stdint.h>
#include <complex.h>    /* Standard Library of Complex Numbers */
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

typedef _Complex float cf_t;

#define SRSLTE_NRE 12

void prb_cp_with_dc(cf_t **input, cf_t **output, int nof_prb, uint32_t radio_fft_len, uint32_t init_pos) {

  int nof_to_copy = 0;

  if((init_pos + nof_prb*SRSLTE_NRE) > radio_fft_len) {
    nof_to_copy = radio_fft_len - init_pos;
    memcpy(*output, *input, sizeof(cf_t)*(nof_to_copy));
    *output -= init_pos;
    memcpy(*output, &((*input)[nof_to_copy]), sizeof(cf_t)*(nof_prb*SRSLTE_NRE - nof_to_copy));
    *output += (nof_prb*SRSLTE_NRE - nof_to_copy);
  } else {
    memcpy(*output, *input, sizeof(cf_t)*(nof_prb*SRSLTE_NRE));
    if((init_pos + nof_prb*SRSLTE_NRE) >= radio_fft_len) {
      *output -= init_pos;
    } else {
      *output += nof_prb*SRSLTE_NRE;
    }
  }
  *input += nof_prb*SRSLTE_NRE;
}

void prb_cp_with_dc2(cf_t **input, cf_t **output, int nof_prb, uint32_t radio_fft_len, uint32_t init_pos) {

  int nof_to_copy = 0;

  if((init_pos + nof_prb*SRSLTE_NRE) <= radio_fft_len) {
    memcpy(*output, *input, sizeof(cf_t)*(nof_prb*SRSLTE_NRE));
    if((init_pos + nof_prb*SRSLTE_NRE) >= radio_fft_len) {
      *output -= init_pos;
    } else {
      *output += nof_prb*SRSLTE_NRE;
    }
  } else {
    nof_to_copy = radio_fft_len - init_pos;
    memcpy(*output, *input, sizeof(cf_t)*(nof_to_copy));
    *output -= init_pos;
    memcpy(*output, &((*input)[nof_to_copy]), sizeof(cf_t)*(nof_prb*SRSLTE_NRE - nof_to_copy));
    *output += (nof_prb*SRSLTE_NRE - nof_to_copy);
  }
  *input += nof_prb*SRSLTE_NRE;
}

void prb_cp_ref_with_dc(cf_t **input, cf_t **output, int offset, int nof_refs, int nof_intervals, bool advance_output, uint32_t nof_prb, uint32_t radio_fft_len, uint32_t init_pos) {
  int i, j, nof_to_copy;

  int ref_interval = ((SRSLTE_NRE/nof_refs) - 1);

  if((init_pos + nof_prb*SRSLTE_NRE) > radio_fft_len) {
    nof_to_copy = radio_fft_len - init_pos;
    for(j = 0; j < nof_to_copy; j++) {
      if(((offset + j) % (SRSLTE_NRE/2)) != 0) {
        (*output)[0] = (*input)[0];
        (*input)++;
      }
      if(j+1 == nof_to_copy) {
        *output -= (init_pos + j);
      } else {
        (*output)++;
      }
    }
    for(; j < nof_prb*SRSLTE_NRE; j++) {
      if(((offset + j) % (SRSLTE_NRE/2)) != 0) {
        (*output)[0] = (*input)[0];
        (*input)++;
      }
      (*output)++;
    }
  } else {
    memcpy(*output, *input, offset*sizeof(cf_t));
    *input += offset;
    *output += offset;
    for(i = 0; i < nof_intervals - 1; i++) {
      if(advance_output) {
        (*output)++;
      } else {
        (*input)++;
      }
      memcpy(*output, *input, ref_interval*sizeof(cf_t));
      *output += ref_interval;
      *input += ref_interval;
    }
    if(ref_interval - offset > 0) {
      if(advance_output) {
        (*output)++;
      } else {
        (*input)++;
      }
      memcpy(*output, *input, (ref_interval - offset)*sizeof(cf_t));
      if((init_pos + nof_prb*SRSLTE_NRE) >= radio_fft_len) {
        *output -= (init_pos + offset + (nof_intervals - 1)*ref_interval + nof_refs);
      } else {
        *output += (ref_interval - offset);
      }
      *input += (ref_interval - offset);
    }
  }
}

void prb_cp_ref_with_dc2(cf_t **input, cf_t **output, int offset, int nof_refs, int nof_intervals, bool advance_output, uint32_t nof_prb, uint32_t radio_fft_len, uint32_t init_pos) {
  int i, j, nof_to_copy;

  int ref_interval = ((SRSLTE_NRE/nof_refs) - 1);

  if((init_pos + nof_prb*SRSLTE_NRE) <= radio_fft_len) {
    memcpy(*output, *input, offset*sizeof(cf_t));
    *input += offset;
    *output += offset;
    for(i = 0; i < nof_intervals - 1; i++) {
      if(advance_output) {
        (*output)++;
      } else {
        (*input)++;
      }
      memcpy(*output, *input, ref_interval*sizeof(cf_t));
      *output += ref_interval;
      *input += ref_interval;
    }
    if(ref_interval - offset > 0) {
      if(advance_output) {
        (*output)++;
      } else {
        (*input)++;
      }
      memcpy(*output, *input, (ref_interval - offset)*sizeof(cf_t));
      if((init_pos + nof_prb*SRSLTE_NRE) >= radio_fft_len) {
        *output -= (init_pos + offset + (nof_intervals - 1)*ref_interval + nof_refs);
      } else {
        *output += (ref_interval - offset);
      }
      *input += (ref_interval - offset);
    }
  } else {
    nof_to_copy = radio_fft_len - init_pos;
    for(j = 0; j < nof_to_copy; j++) {
      if(((offset + j) % (SRSLTE_NRE/2)) != 0) {
        (*output)[0] = (*input)[0];
        (*input)++;
      }
      if(j+1 == nof_to_copy) {
        *output -= (init_pos + j);
      } else {
        (*output)++;
      }
    }
    for(; j < nof_prb*SRSLTE_NRE; j++) {
      if(((offset + j) % (SRSLTE_NRE/2)) != 0) {
        (*output)[0] = (*input)[0];
        (*input)++;
      }
      (*output)++;
    }
  }
}

void prb_cp_half_with_dc(cf_t **input, cf_t **output, int nof_prb, uint32_t radio_fft_len, uint32_t init_pos) {

  int nof_to_copy = 0;

  if((init_pos + (nof_prb*SRSLTE_NRE/2)) > radio_fft_len) {
    nof_to_copy = radio_fft_len - init_pos;
    memcpy(*output, *input, sizeof(cf_t)*nof_to_copy);
    *output -= init_pos;
    memcpy(*output, &((*input)[nof_to_copy]), sizeof(cf_t)*((nof_prb*SRSLTE_NRE/2) - nof_to_copy));
    *output += ((nof_prb*SRSLTE_NRE/2) - nof_to_copy);
  } else {
    memcpy(*output, *input, sizeof(cf_t)*(nof_prb*SRSLTE_NRE/2));
    if((init_pos + (nof_prb*SRSLTE_NRE/2)) >= radio_fft_len) {
      *output -= init_pos;
    } else {
      *output += nof_prb*SRSLTE_NRE/2;
    }
  }
  *input += nof_prb*SRSLTE_NRE/2;
}

void prb_cp_half_with_dc2(cf_t **input, cf_t **output, int nof_prb, uint32_t radio_fft_len, uint32_t init_pos) {

  int nof_to_copy = 0;

  if((init_pos + (nof_prb*SRSLTE_NRE/2)) <= radio_fft_len) {
    memcpy(*output, *input, sizeof(cf_t)*(nof_prb*SRSLTE_NRE/2));
    if((init_pos + (nof_prb*SRSLTE_NRE/2)) >= radio_fft_len) {
      *output -= init_pos;
    } else {
      *output += nof_prb*SRSLTE_NRE/2;
    }
  } else {
    nof_to_copy = radio_fft_len - init_pos;
    memcpy(*output, *input, sizeof(cf_t)*nof_to_copy);
    *output -= init_pos;
    memcpy(*output, &((*input)[nof_to_copy]), sizeof(cf_t)*((nof_prb*SRSLTE_NRE/2) - nof_to_copy));
    *output += ((nof_prb*SRSLTE_NRE/2) - nof_to_copy);
  }
  *input += nof_prb*SRSLTE_NRE/2;
}

int main() {

  cf_t *input, *output;
  cf_t *out_ptr, *in_ptr;

  uint32_t radio_nof_prb = 100;
  uint32_t vphy_nof_prb = 6;
  uint32_t radio_fft_len = 1536;
  uint32_t vphy_fft_len = 128;
  uint32_t channel = 0;

  output = malloc(radio_fft_len*sizeof(cf_t));
  if (!output) {
    perror("srslte_vec_malloc");
    exit(-1);
  }
  // Clean resource grid buffer.
  bzero(output,radio_fft_len*sizeof(cf_t));

  input = malloc(SRSLTE_NRE*vphy_nof_prb*sizeof(cf_t));
  if (!input) {
    perror("srslte_vec_malloc");
    exit(-1);
  }
  // Clean resource grid buffer.
  bzero(input, SRSLTE_NRE*vphy_nof_prb*sizeof(cf_t));

  for(int i = 1; i <= SRSLTE_NRE*vphy_nof_prb; i++) {
    input[i-1] = i + i*I;
  }

  uint32_t init_pos = 1535;

  in_ptr = &input[0];
  out_ptr = &output[init_pos];

#define TEST_PRB_CP_HALF 1

#if(TEST_PRB_CP==1)
  printf("1st call - init_pos: %d\n",init_pos);
  prb_cp_with_dc2(&in_ptr, &out_ptr, 1, radio_fft_len, init_pos);
  init_pos = (init_pos + SRSLTE_NRE) % radio_fft_len;
  printf("2nd call - init_pos: %d\n",init_pos);
  prb_cp_with_dc2(&in_ptr, &out_ptr, 1, radio_fft_len, init_pos);
  init_pos = (init_pos + SRSLTE_NRE) % radio_fft_len;
  printf("3rd call - init_pos: %d\n",init_pos);
  prb_cp_with_dc2(&in_ptr, &out_ptr, 1, radio_fft_len, init_pos);
#endif

#if(TEST_PRB_CP_REF==1)
  int offset = 3;
  int nof_refs = 2;
  int nof_intervals = 2;
  bool advance_output = true;
  printf("1st call - init_pos: %d\n",init_pos);
  prb_cp_ref_with_dc2(&in_ptr, &out_ptr, offset, nof_refs, nof_intervals, advance_output, 1, radio_fft_len, init_pos);
  init_pos = (init_pos + SRSLTE_NRE) % radio_fft_len;
  printf("2nd call - init_pos: %d\n",init_pos);
  prb_cp_ref_with_dc2(&in_ptr, &out_ptr, offset, nof_refs, nof_intervals, advance_output, 1, radio_fft_len, init_pos);
  init_pos = (init_pos + SRSLTE_NRE) % radio_fft_len;
  printf("3rd call - init_pos: %d\n",init_pos);
  prb_cp_ref_with_dc2(&in_ptr, &out_ptr, offset, nof_refs, nof_intervals, advance_output, 1, radio_fft_len, init_pos);
#endif

#if(TEST_PRB_CP_HALF==1)
  printf("1st call - init_pos: %d\n",init_pos);
  prb_cp_half_with_dc2(&in_ptr, &out_ptr, 1, radio_fft_len, init_pos);
  init_pos = (init_pos + SRSLTE_NRE/2) % radio_fft_len;
  printf("2nd call - init_pos: %d\n",init_pos);
  prb_cp_half_with_dc2(&in_ptr, &out_ptr, 1, radio_fft_len, init_pos);
  init_pos = (init_pos + SRSLTE_NRE/2) % radio_fft_len;
  printf("3rd call - init_pos: %d\n",init_pos);
  prb_cp_half_with_dc2(&in_ptr, &out_ptr, 1, radio_fft_len, init_pos);
#endif


#if(PRINT_INPUT==1)
  // Print OFDM symbols.
  for(int symb = 0; symb < 1; symb++) {
    printf("########### Symbol %d ###########\n",symb);
    for(int sc = 0; sc < SRSLTE_NRE*vphy_nof_prb; sc++) {
      printf("idx: %d - input value[%d]: %f,%f\n", (symb*radio_fft_len + sc), sc, __real__ input[radio_fft_len*symb + sc], __imag__ input[radio_fft_len*symb + sc]);
    }
    printf("----------------------------------------\n\n\n\n");
  }
#endif

  // Print OFDM symbols.
  for(int symb = 0; symb < 1; symb++) {
    printf("########### Symbol %d ###########\n",symb);
    for(int sc = 0; sc < radio_fft_len; sc++) {
      printf("idx: %d - value[%d]: %f,%f\n", (symb*radio_fft_len + sc), sc, __real__ output[radio_fft_len*symb + sc], __imag__ output[radio_fft_len*symb + sc]);
    }
    printf("----------------------------------------\n\n");
  }

  return 0;
}
