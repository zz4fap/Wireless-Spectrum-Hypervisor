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

uint32_t cfi = 1;
bool print_dci_table;
int nof_dcis = 1;

void usage(char *prog) {
  printf("Usage: %s [cfpndv]\n", prog);
  printf("\t-c cell id [Default %d]\n", cell.id);
  printf("\t-f cfi [Default %d]\n", cfi);
  printf("\t-p cell.nof_ports [Default %d]\n", cell.nof_ports);
  printf("\t-n cell.nof_prb [Default %d]\n", cell.nof_prb);
  printf("\t-d Print DCI table [Default %s]\n", print_dci_table?"yes":"no");
  printf("\t-v [set srslte_verbose to debug, default none]\n");
}

void parse_args(int argc, char **argv) {
  int opt;
  while ((opt = getopt(argc, argv, "cfpndv")) != -1) {
    switch (opt) {
    case 'p':
      cell.nof_ports = atoi(argv[optind]);
      break;
    case 'f':
      cfi = atoi(argv[optind]);
      break;
    case 'n':
      cell.nof_prb = atoi(argv[optind]);
      break;
    case 'c':
      cell.id = atoi(argv[optind]);
      break;
    case 'd':
      print_dci_table = true;
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

unsigned int reverse_bits(register unsigned int x) {
  x = (((x & 0xaaaaaaaa) >> 1) | ((x & 0x55555555) << 1));
  x = (((x & 0xcccccccc) >> 2) | ((x & 0x33333333) << 2));
  x = (((x & 0xf0f0f0f0) >> 4) | ((x & 0x0f0f0f0f) << 4));
  x = (((x & 0xff00ff00) >> 8) | ((x & 0x00ff00ff) << 8));
  return((x >> 16) | (x << 16));
}

uint32_t prbset_to_bitmask(uint32_t nof_prb) {
  uint32_t mask = 0;
  int nb = (int) ceilf((float) nof_prb / srslte_ra_type0_P(nof_prb));
  for(int i = 0; i < nb; i++) {
    if(i >= 0 && i < nb) {
      mask = mask | (0x1<<i);
    }
  }
  return reverse_bits(mask)>>(32-nb);
}

int main(int argc, char **argv) {
  srslte_pdcch_t pdcch;
  srslte_dci_msg_t dci_tx[nof_dcis];
  srslte_dci_location_t dci_locations[nof_dcis];
  srslte_ra_dl_dci_t ra_dl;
  srslte_regs_t regs;
  int i;
  int nof_re;
  cf_t *slot_symbols[SRSLTE_NOF_PORTS];

  uint16_t rnti = 0x1234;
  uint32_t nsf = 5;
  uint32_t radio_nof_prb = 100;
  uint32_t vphy_nof_prb = 6;
  uint32_t radio_fft_len = srslte_symbol_sz(radio_nof_prb);
  uint32_t vphy_fft_len = srslte_symbol_sz(vphy_nof_prb);
  uint32_t channel = 0;
  int dc_pos = channel*vphy_fft_len;

  int ret = -1;

  parse_args(argc, argv);

  nof_re = SRSLTE_CP_NSYMB(cell.cp)*srslte_symbol_sz(radio_nof_prb);

  // init memory.
  for(i = 0; i < SRSLTE_NOF_PORTS; i++) {
    slot_symbols[i] = malloc(sizeof(cf_t)*nof_re);
    if(!slot_symbols[i]) {
      perror("malloc");
      exit(-1);
    }
    // Clean all the slot buffer.
    bzero(slot_symbols[i],sizeof(cf_t)*nof_re);
  }

  if(srslte_regs_init(&regs, cell)) {
    fprintf(stderr, "Error initiating regs\n");
    exit(-1);
  }

  if(srslte_regs_set_cfi(&regs, cfi)) {
    fprintf(stderr, "Error setting CFI\n");
    exit(-1);
  }

  if(srslte_pdcch_init(&pdcch, &regs, cell)) {
    fprintf(stderr, "Error creating PDCCH object\n");
    exit(-1);
  }

  bzero(&ra_dl, sizeof(srslte_ra_dl_dci_t));
  ra_dl.harq_process = 0;
  ra_dl.mcs_idx = 0;
  ra_dl.ndi = 0;
  ra_dl.rv_idx = 0;
  ra_dl.alloc_type = SRSLTE_RA_ALLOC_TYPE0;
  ra_dl.type0_alloc.rbg_bitmask = prbset_to_bitmask(vphy_nof_prb);

  srslte_dci_msg_pack_pdsch(&ra_dl, SRSLTE_DCI_FORMAT1, &dci_tx[0], cell.nof_prb, false);
  srslte_dci_location_set(&dci_locations[0], 0, 0);

  for(i = 0; i < nof_dcis; i++) {
    if(srslte_encode_and_map_pdcch_with_dc(&pdcch, &dci_tx[i], dci_locations[i], rnti, slot_symbols, nsf, cfi, radio_fft_len, dc_pos)) {
      fprintf(stderr, "Error encoding DCI message.\n");
      goto quit;
    }
  }

  // Print OFDM symbols.
  for(int symb = 0; symb < SRSLTE_CP_NSYMB(cell.cp); symb++) {
    printf("########### Symbol %d ###########\n",symb);
    for(int sc = 0; sc < radio_fft_len; sc++) {
      printf("idx: %d - value[%d]: %f,%f\n", (symb*radio_fft_len + sc), sc, __real__ slot_symbols[0][radio_fft_len*symb + sc], __imag__ slot_symbols[0][radio_fft_len*symb + sc]);
    }
    printf("----------------------------------------\n\n");
  }

  ret = 0;

quit:
  srslte_pdcch_free(&pdcch);
  srslte_regs_free(&regs);

  for(i = 0; i < SRSLTE_NOF_PORTS; i++) {
    free(slot_symbols[i]);
  }
  if(ret) {
    printf("Error\n");
  } else {
    printf("Ok\n");
  }
  exit(ret);
}
