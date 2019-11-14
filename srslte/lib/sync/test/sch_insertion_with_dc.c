#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include <math.h>
#include <time.h>

#include <stdbool.h>

#include "srslte/srslte.h"

int cell_id = 0;
srslte_cp_t cp = SRSLTE_CP_NORM;
uint32_t vphy_nof_prb = 6;

void usage(char *prog) {
  printf("Usage: %s [cpoev]\n", prog);
  printf("\t-c cell_id [Default check for all]\n");
  printf("\t-p vphy_nof_prb [Default %d]\n", vphy_nof_prb);
  printf("\t-e extended CP [Default normal]\n");
  printf("\t-v srslte_verbose\n");
}

void parse_args(int argc, char **argv) {
  int opt;
  while((opt = getopt(argc, argv, "cpoev")) != -1) {
    switch(opt) {
    case 'c':
      cell_id = atoi(argv[optind]);
      break;
    case 'p':
      vphy_nof_prb = atoi(argv[optind]);
      break;
    case 'e':
      cp = SRSLTE_CP_EXT;
      break;
    case 'v':
      srslte_verbose++;
      break;
    default:
      usage(argv[0]);
      exit(-1);
    }
  }
}

int main(int argc, char **argv) {
  cf_t *buffer;
  float sss_signal0[SRSLTE_SSS_LEN]; // for subframe 0
  float sss_signal5[SRSLTE_SSS_LEN]; // for subframe 5
  int sf_idx = 5;
  uint32_t radio_nof_prb = 100;
  uint32_t vphy_nof_prb = 6;
  uint32_t radio_fft_len = srslte_symbol_sz(radio_nof_prb);
  uint32_t vphy_fft_len = srslte_symbol_sz(vphy_nof_prb);
  uint32_t channel = 0;
  int dc_pos = channel*vphy_fft_len;

  parse_args(argc, argv);

  uint32_t nof_re = SRSLTE_CP_NSYMB(cp)*srslte_symbol_sz(radio_nof_prb);

  buffer = malloc(nof_re*sizeof(cf_t));
  if(!buffer) {
    perror("malloc");
    exit(-1);
  }

  // Generate PSS/SSS signals
  srslte_sss_generate(sss_signal0, sss_signal5, cell_id);

  // Clear resource grid.
  memset(buffer, 0, nof_re*sizeof(cf_t));

  // Add SCH signal to resource grid respecting the DC subcarrier for each vPHY.
  uint32_t symbol = 0;
  bool contiguous_subcarriers = true;
  srslte_insert_sch_into_subframe_with_dc_generic(sf_idx ? sss_signal5:sss_signal0, buffer, radio_fft_len, dc_pos, symbol, contiguous_subcarriers);

  // Print OFDM symbols.
  for(int symb = 0; symb < SRSLTE_CP_NSYMB(cp); symb++) {
    printf("########### Symbol %d ###########\n",symb);
    for(int sc = 0; sc < radio_fft_len; sc++) {
      printf("value[%d]: %f,%f\n", sc, __real__ buffer[radio_fft_len*symb + sc], __imag__ buffer[radio_fft_len*symb + sc]);
    }
    printf("----------------------------------------\n\n");
  }

  free(buffer);

  printf("Ok\n");
  exit(0);
}
