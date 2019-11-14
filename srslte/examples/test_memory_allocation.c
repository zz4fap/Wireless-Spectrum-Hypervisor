#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <unistd.h>
#include <sys/time.h>
#include <string.h>

#include "srslte/srslte.h"

#include "liquid/liquid.h"

#define NOF_SUBFRAME_BUFFERS 10

cf_t *subframe_buffer[NOF_SUBFRAME_BUFFERS];

cf_t** const hypervisor_tx_get_subframe_buffer() {
  return subframe_buffer;
}

int main(int argc, char *argv[]) {

  cf_t** subframe_buffer_test_ptr;

  for(uint32_t i = 0; i < NOF_SUBFRAME_BUFFERS; i++) {
    subframe_buffer[i] = (cf_t*)srslte_vec_malloc(sizeof(cf_t)*NOF_SUBFRAME_BUFFERS);
    printf("subframe_buffer[%d]: %x\n", i, subframe_buffer[i]);
  }
  printf("---------------------------------------------\n\n");

  subframe_buffer_test_ptr = hypervisor_tx_get_subframe_buffer();
  for(uint32_t i = 0; i < NOF_SUBFRAME_BUFFERS; i++) {
    printf("subframe_buffer_test_ptr[%d]: %x\n", i, subframe_buffer_test_ptr[i]);
  }

  for(uint32_t i = 0; i < NOF_SUBFRAME_BUFFERS; i++) {
    free(subframe_buffer[i]);
  }

  return 0;
}
