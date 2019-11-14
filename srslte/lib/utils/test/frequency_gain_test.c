#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <unistd.h>
#include <math.h>
#include <time.h>
#include <complex.h>

#include "srslte/srslte.h"

srslte_cell_t cell = {
  6,                  // nof_prb
  1,                  // nof_ports
  0,                  // BW index
  0,                  // cell_id
  SRSLTE_CP_NORM,     // cyclic prefix
  SRSLTE_PHICH_NORM,  // PHICH length
  SRSLTE_PHICH_R_1    // PHICH resources
};

int main(int argc, char **argv) {

  int ret = -1, dc_pos = 0, init_pos = 0, end_pos = 0, idx = 0;
  uint32_t radio_nof_prb = 100;
  uint32_t vphy_nof_prb = 6;
  uint32_t radio_fft_len = srslte_symbol_sz(radio_nof_prb);
  uint32_t vphy_fft_len = srslte_symbol_sz(vphy_nof_prb);
  uint32_t channel = 0;
  float freq_boost = 0.5;

  int nof_re = 2*SRSLTE_CP_NSYMB(cell.cp)*srslte_symbol_sz(radio_nof_prb);
  // init memory.
  cf_t *resource_grid = srslte_vec_malloc(nof_re*sizeof(cf_t));
  if(!resource_grid) {
    perror("srslte_vec_malloc");
    goto quit;
  }
  // Clean resource grid buffer.
  bzero(resource_grid, nof_re*sizeof(cf_t));

  // Calculate DC position.
  dc_pos = channel*vphy_fft_len;

  // Calculate initial position.
  init_pos = dc_pos - ((vphy_nof_prb*SRSLTE_NRE)/2);
  if(init_pos < 0) {
    init_pos += radio_fft_len;
  }

  // Calculate final position.
  end_pos = dc_pos + ((vphy_nof_prb*SRSLTE_NRE)/2);

  // Create a user allocation into the resource grid.
  for(int symb = 0; symb < 2*SRSLTE_CP_NSYMB(cell.cp); symb++) {
    for(int cnt = 0; cnt < (vphy_nof_prb*SRSLTE_NRE); cnt++) {
      idx = (init_pos + cnt) % radio_fft_len;
      if(cnt >= ((vphy_nof_prb*SRSLTE_NRE)/2)) {
        idx++;
      }
      resource_grid[idx + symb*radio_fft_len] = 1.0 + 1.0I;
    }
  }

#if 0
  // Print OFDM symbols.
  printf("OFDM # ----- 0 ----- ----- 1 ----- ----- 2 ----- ----- 3 ----- ----- 4 ----- ----- 5 ----- ----- 6 ----- ----- 7 ----- ----- 8 ----- ----- 9 ----- ----- 10 ---- ----- 11 ---- ----- 12 ---- ----- 13 ----\n");
  for(int sc = radio_fft_len-1; sc >= 0; sc--) {
    printf("%04d - ",sc);
    for(int symb = 0; symb < 2*SRSLTE_CP_NSYMB(cell.cp); symb++) {
      printf("|%+1.2f,%+1.2f| ", __real__ resource_grid[radio_fft_len*symb + sc], __imag__ resource_grid[radio_fft_len*symb + sc] );
    }
    printf("\n");
  }
  printf("OFDM # ----- 0 ----- ----- 1 ----- ----- 2 ----- ----- 3 ----- ----- 4 ----- ----- 5 ----- ----- 6 ----- ----- 7 ----- ----- 8 ----- ----- 9 ----- ----- 10 ---- ----- 11 ---- ----- 12 ---- ----- 13 ----\n");
#endif

  srslte_vec_sc_prod_cfc_with_dc(resource_grid, freq_boost, resource_grid, SRSLTE_CP_NSYMB(cell.cp), radio_fft_len, vphy_fft_len, vphy_nof_prb, channel);

#if 1
  // Print OFDM symbols.
  printf("OFDM # ----- 0 ----- ----- 1 ----- ----- 2 ----- ----- 3 ----- ----- 4 ----- ----- 5 ----- ----- 6 ----- ----- 7 ----- ----- 8 ----- ----- 9 ----- ----- 10 ---- ----- 11 ---- ----- 12 ---- ----- 13 ----\n");
  for(int sc = radio_fft_len-1; sc >= 0; sc--) {
    printf("%04d - ",sc);
    for(int symb = 0; symb < 2*SRSLTE_CP_NSYMB(cell.cp); symb++) {
      printf("|%+1.2f,%+1.2f| ", __real__ resource_grid[radio_fft_len*symb + sc], __imag__ resource_grid[radio_fft_len*symb + sc] );
    }
    printf("\n");
  }
  printf("OFDM # ----- 0 ----- ----- 1 ----- ----- 2 ----- ----- 3 ----- ----- 4 ----- ----- 5 ----- ----- 6 ----- ----- 7 ----- ----- 8 ----- ----- 9 ----- ----- 10 ---- ----- 11 ---- ----- 12 ---- ----- 13 ----\n");
#endif

  // Check results.
  int error = 0;
  for(int symb = 0; symb < 2*SRSLTE_CP_NSYMB(cell.cp); symb++) {
    for(int cnt = 0; cnt < (vphy_nof_prb*SRSLTE_NRE); cnt++) {
      idx = (init_pos + cnt) % radio_fft_len;
      if(cnt >= ((vphy_nof_prb*SRSLTE_NRE)/2)) {
        idx++;
      }
      float re = __real__ resource_grid[idx + symb*radio_fft_len];
      float im = __imag__ resource_grid[idx + symb*radio_fft_len];
      if(re != freq_boost || im != freq_boost) {
        error++;
      }
    }
  }
  printf("# errors within allocation: %d\n",error);

  error = 0;
  for(int symb = 0; symb < 2*SRSLTE_CP_NSYMB(cell.cp); symb++) {
    for(int sc = 0; sc < radio_fft_len; sc++) {
      if(end_pos > init_pos) {
        if(sc < init_pos || sc > end_pos) {
          float re = __real__ resource_grid[sc + symb*radio_fft_len];
          float im = __imag__ resource_grid[sc + symb*radio_fft_len];
          if(re != 0.0 || im != 0.0) {
            error++;
          }
        }
      } else {
        if(sc > end_pos && sc < init_pos) {
          float re = __real__ resource_grid[sc + symb*radio_fft_len];
          float im = __imag__ resource_grid[sc + symb*radio_fft_len];
          if(re != 0.0 || im != 0.0) {
            error++;
          }
        }
      }
    }
  }
  printf("# errors outside allocation: %d\n",error);

  ret = 0;

quit:

    if(resource_grid) {
      free(resource_grid);
    }
    if(ret) {
      printf("Error\n");
    } else {
      printf("Ok\n");
    }
    exit(ret);
}
