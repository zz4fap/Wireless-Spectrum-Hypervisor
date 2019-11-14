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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>

#include "srslte/srslte.h"

#define SRSLTE_NOF_PORTS 1

srslte_cell_t cell = {
  6,                  // nof_prb
  1,                  // nof_ports
  0,                  // BW index
  0,                  // cell_id
  SRSLTE_CP_NORM,     // cyclic prefix
  SRSLTE_PHICH_NORM,  // PHICH length
  SRSLTE_PHICH_R_1    // PHICH resources
};

void usage(char *prog) {
  printf("Usage: %s [cpnv]\n", prog);
  printf("\t-c cell id [Default %d]\n", cell.id);
  printf("\t-p nof_ports [Default %d]\n", cell.nof_ports);
  printf("\t-n nof_prb [Default %d]\n", cell.nof_prb);
  printf("\t-v [set srslte_verbose to debug, default none]\n");
}

void parse_args(int argc, char **argv) {
  int opt;
  while ((opt = getopt(argc, argv, "cpnv")) != -1) {
    switch(opt) {
    case 'p':
      cell.nof_ports = atoi(argv[optind]);
      break;
    case 'n':
      cell.nof_prb = atoi(argv[optind]);
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
  srslte_pcfich_t pcfich;
  srslte_regs_t regs;
  int i;
  int nof_re;
  cf_t *slot_symbols[SRSLTE_NOF_PORTS];
  uint32_t cfi = 1;
  uint32_t nsf = 5;
  uint32_t radio_nof_prb = 100;
  uint32_t vphy_nof_prb = 6;
  uint32_t radio_fft_len = srslte_symbol_sz(radio_nof_prb);
  uint32_t vphy_fft_len = srslte_symbol_sz(vphy_nof_prb);
  uint32_t channel = 0;
  int dc_pos = channel*vphy_fft_len;

  parse_args(argc,argv);

  nof_re = SRSLTE_CP_NSYMB(cell.cp)*srslte_symbol_sz(radio_nof_prb);

  printf("nof_re: %d - SRSLTE_CP_NSYMB(cell.cp): %d\n", nof_re, SRSLTE_CP_NSYMB(cell.cp));

  // init memory
  for(i = 0; i < SRSLTE_NOF_PORTS; i++) {
    slot_symbols[i] = malloc(sizeof(cf_t)*nof_re);
    if(!slot_symbols[i]) {
      perror("malloc");
      exit(-1);
    }
    bzero(slot_symbols[i], sizeof(cf_t)*nof_re);
  }

  if(srslte_regs_init(&regs, cell)) {
    fprintf(stderr, "Error initiating regs\n");
    exit(-1);
  }

  if(srslte_pcfich_init(&pcfich, &regs, cell)) {
    fprintf(stderr, "Error creating PBCH object\n");
    exit(-1);
  }

  srslte_encode_and_map_pcfich_with_dc(&pcfich, cfi, slot_symbols, nsf, radio_fft_len, dc_pos);

  // Print OFDM symbols.
  for(int symb = 0; symb < SRSLTE_CP_NSYMB(cell.cp); symb++) {
    printf("########### Symbol %d ###########\n",symb);
    for(int sc = 0; sc < radio_fft_len; sc++) {
      printf("value[%d]: %f,%f\n", sc, __real__ slot_symbols[0][radio_fft_len*symb + sc], __imag__ slot_symbols[0][radio_fft_len*symb + sc]);
    }
    printf("----------------------------------------\n\n");
  }

  srslte_pcfich_free(&pcfich);
  srslte_regs_free(&regs);

  for(i = 0; i < SRSLTE_NOF_PORTS; i++) {
    free(slot_symbols[i]);
  }

  printf("OK\n");
  exit(0);
}
