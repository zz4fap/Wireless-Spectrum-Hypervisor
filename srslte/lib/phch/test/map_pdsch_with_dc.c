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
#include <sys/time.h>

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
uint32_t mcs = 0;
uint32_t subframe = 5;
uint32_t rv_idx = 0;
uint16_t rnti = 0x1234;

uint8_t *data = NULL;
srslte_ra_dl_grant_t grant;
srslte_pdsch_cfg_t pdsch_cfg;
cf_t *slot_symbols[SRSLTE_NOF_PORTS];
srslte_pdsch_t pdsch;
srslte_ofdm_t ofdm_tx, ofdm_rx;

void usage(char *prog) {
  printf("Usage: %s [fmcsrRFpnv] \n", prog);
  printf("\t-m MCS [Default %d]\n", mcs);
  printf("\t-c cell id [Default %d]\n", cell.id);
  printf("\t-s subframe [Default %d]\n", subframe);
  printf("\t-r rv_idx [Default %d]\n", rv_idx);
  printf("\t-R rnti [Default %d]\n", rnti);
  printf("\t-F cfi [Default %d]\n", cfi);
  printf("\t-p cell.nof_ports [Default %d]\n", cell.nof_ports);
  printf("\t-n cell.nof_prb [Default %d]\n", cell.nof_prb);
  printf("\t-v [set srslte_verbose to debug, default none]\n");
}

void parse_args(int argc, char **argv) {
  int opt;
  while ((opt = getopt(argc, argv, "mcsrRFpnv")) != -1) {
    switch(opt) {
    case 'm':
      mcs = atoi(argv[optind]);
      break;
    case 's':
      subframe = atoi(argv[optind]);
      break;
    case 'r':
      rv_idx = atoi(argv[optind]);
      break;
    case 'R':
      rnti = atoi(argv[optind]);
      break;
    case 'F':
      cfi = atoi(argv[optind]);
      break;
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
  int ret = -1, nof_re = 0;
  srslte_softbuffer_tx_t softbuffer_tx;

  uint32_t radio_nof_prb = 100;
  uint32_t vphy_nof_prb = 6;
  uint32_t radio_fft_len = srslte_symbol_sz(radio_nof_prb);
  uint32_t vphy_fft_len = srslte_symbol_sz(vphy_nof_prb);
  uint32_t channel = 11;
  int dc_pos = channel*vphy_fft_len;

  parse_args(argc,argv);

  bzero(&pdsch, sizeof(srslte_pdsch_t));
  bzero(&pdsch_cfg, sizeof(srslte_pdsch_cfg_t));
  bzero(slot_symbols, sizeof(cf_t*)*SRSLTE_NOF_PORTS);
  bzero(&softbuffer_tx, sizeof(srslte_softbuffer_tx_t));

  srslte_ra_dl_dci_t dci;
  bzero(&dci, sizeof(srslte_ra_dl_dci_t));
  dci.harq_process = 0;
  dci.mcs_idx = mcs;
  dci.ndi = 0;
  dci.rv_idx = rv_idx;
  dci.alloc_type = SRSLTE_RA_ALLOC_TYPE0;
  dci.type0_alloc.rbg_bitmask = prbset_to_bitmask(vphy_nof_prb);
  if(srslte_ra_dl_dci_to_grant(&dci, cell.nof_prb, rnti, &grant)) {
    fprintf(stderr, "Error computing resource allocation\n");
    return ret;
  }

  // Configure PDSCH.
  if(srslte_pdsch_cfg(&pdsch_cfg, cell, &grant, cfi, subframe, rv_idx)) {
    fprintf(stderr, "Error configuring PDSCH\n");
    exit(-1);
  }

  nof_re = 2*SRSLTE_CP_NSYMB(cell.cp)*srslte_symbol_sz(radio_nof_prb);

  // init memory.
  for(uint32_t i = 0; i < SRSLTE_NOF_PORTS; i++) {
    slot_symbols[i] = srslte_vec_malloc(nof_re*sizeof(cf_t));
    if(!slot_symbols[i]) {
      perror("srslte_vec_malloc");
      goto quit;
    }
    // Clean resource grid buffer.
    bzero(slot_symbols[i],nof_re*sizeof(cf_t));
  }

  data = srslte_vec_malloc(sizeof(uint8_t)*grant.mcs.tbs/8);
  if(!data) {
    perror("srslte_vec_malloc");
    goto quit;
  }

  if(srslte_pdsch_init(&pdsch, cell)) {
    fprintf(stderr, "Error creating PDSCH object\n");
    goto quit;
  }

  srslte_pdsch_set_rnti(&pdsch, rnti);

  if(srslte_softbuffer_tx_init(&softbuffer_tx, cell.nof_prb)) {
    fprintf(stderr, "Error initiating TX soft buffer\n");
    goto quit;
  }

  for(uint32_t i = 0; i < grant.mcs.tbs/8; i++) {
    data[i] = rand()%256;
  }

  uint8_t databit[100000];
  srslte_bit_unpack_vector(data, databit, grant.mcs.tbs);

  // Do 1st transmission for rv_idx!=0.
  pdsch_cfg.rv = 0;

  if(srslte_encode_and_map_pdsch_with_dc(&pdsch, &pdsch_cfg, &softbuffer_tx, data, slot_symbols, radio_fft_len, dc_pos)) {
    fprintf(stderr, "Error encoding PDSCH\n");
    goto quit;
  }

#if 0
  // Print OFDM symbols.
  for(int symb = 0; symb < 2*SRSLTE_CP_NSYMB(cell.cp); symb++) {
    printf("########### Symbol %d ###########\n",symb);
    for(int sc = 0; sc < radio_fft_len; sc++) {
      printf("idx: %d - value[%d]: %f,%f\n", (symb*radio_fft_len + sc), sc, __real__ slot_symbols[0][radio_fft_len*symb + sc], __imag__ slot_symbols[0][radio_fft_len*symb + sc]);
    }
    printf("----------------------------------------\n\n");
  }
#endif

#if 1
  // Print OFDM symbols.
  printf("OFDM # ----- 0 ----- ----- 1 ----- ----- 2 ----- ----- 3 ----- ----- 4 ----- ----- 5 ----- ----- 6 ----- ----- 7 ----- ----- 8 ----- ----- 9 ----- ----- 10 ---- ----- 11 ---- ----- 12 ---- ----- 13 ----\n");
  for(int sc = radio_fft_len-1; sc >= 0; sc--) {
    printf("%04d - ",sc);
    for(int symb = 0; symb < 2*SRSLTE_CP_NSYMB(cell.cp); symb++) {
      printf("|%+1.2f,%+1.2f| ", __real__ slot_symbols[0][radio_fft_len*symb + sc], __imag__ slot_symbols[0][radio_fft_len*symb + sc] );
    }
    printf("\n");
  }
  printf("OFDM # ----- 0 ----- ----- 1 ----- ----- 2 ----- ----- 3 ----- ----- 4 ----- ----- 5 ----- ----- 6 ----- ----- 7 ----- ----- 8 ----- ----- 9 ----- ----- 10 ---- ----- 11 ---- ----- 12 ---- ----- 13 ----\n");
#endif

  ret = 0;

quit:
  srslte_pdsch_free(&pdsch);
  srslte_softbuffer_tx_free(&softbuffer_tx);

  for(uint32_t i = 0; i < cell.nof_ports; i++) {
    if(slot_symbols[i]) {
      free(slot_symbols[i]);
    }
  }
  if(data) {
    free(data);
  }
  if(ret) {
    printf("Error\n");
  } else {
    printf("Ok\n");
  }
  exit(ret);
}
