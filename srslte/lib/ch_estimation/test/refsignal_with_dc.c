#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <unistd.h>
#include <complex.h>

#include "srslte/srslte.h"

srslte_cell_t cell = {
  6,                    // nof_prb
  1,                    // nof_ports
  0,                    // bw_idx
  0,                    // cell_id
  SRSLTE_CP_NORM        // cyclic prefix
};

void usage(char *prog) {
  printf("Usage: %s [recov]\n", prog);
  printf("\t-r nof_prb [Default %d]\n", cell.nof_prb);
  printf("\t-e extended cyclic prefix [Default normal]\n");
  printf("\t-c cell_id (1000 tests all). [Default %d]\n", cell.id);
  printf("\t-v increase verbosity\n");
}

void parse_args(int argc, char **argv) {
  int opt;
  while ((opt = getopt(argc, argv, "recov")) != -1) {
    switch(opt) {
    case 'r':
      cell.nof_prb = atoi(argv[optind]);
      break;
    case 'e':
      cell.cp = SRSLTE_CP_EXT;
      break;
    case 'c':
      cell.id = atoi(argv[optind]);
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
  srslte_chest_dl_t est;
  cf_t *input = NULL;
  int n_port = 0, sf_idx = 5, cid = 0, num_re;
  int ret = -1;
  uint32_t radio_nof_prb = 100;
  uint32_t vphy_nof_prb = 6;
  uint32_t radio_fft_len = srslte_symbol_sz(radio_nof_prb);
  uint32_t vphy_fft_len = srslte_symbol_sz(vphy_nof_prb);
  uint32_t channel = 0;
  int dc_pos = channel*vphy_fft_len;

  parse_args(argc,argv);

  num_re = 2 * SRSLTE_CP_NSYMB(cell.cp) * srslte_symbol_sz(radio_nof_prb);

  printf("num_re: %d\n",num_re);

  input = srslte_vec_malloc(num_re * sizeof(cf_t));
  if(!input) {
    perror("srslte_vec_malloc");
    goto do_exit;
  }

  if(srslte_chest_dl_init(&est, cell)) {
    fprintf(stderr, "Error initializing equalizer.\n");
    goto do_exit;
  }

  for(n_port = 0; n_port < cell.nof_ports; n_port++) {
    bzero(input, sizeof(cf_t)*num_re);
    // Insert CRS to resource grid.
    srslte_insert_refsignal_into_subframe_with_dc(cell, radio_fft_len, dc_pos, n_port, est.csr_signal.pilots[0][sf_idx], input);
  }

  // Print OFDM symbols.
  for(int symb = 0; symb < 2*SRSLTE_CP_NSYMB(cell.cp); symb++) {
    printf("########### Symbol %d ###########\n",symb);
    for(int sc = 0; sc < radio_fft_len; sc++) {
      printf("value[%d]: %f,%f\n", sc, __real__ input[radio_fft_len*symb + sc], __imag__ input[radio_fft_len*symb + sc]);
    }
    printf("----------------------------------------\n\n");
  }

  srslte_chest_dl_free(&est);

  ret = 0;

do_exit:
  if(input) {
    free(input);
  }

  if(!ret) {
    printf("OK\n");
  } else {
    printf("Error at cid = %d, slot = %d, port = %d\n",cid, sf_idx, n_port);
  }

  exit(ret);
}
