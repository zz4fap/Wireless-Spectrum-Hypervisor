#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <unistd.h>
#include <sys/time.h>
#include <string.h>

#include "srslte/srslte.h"

#define NOF_USRP_READ_BUFFERS_TEST 10

int main(int argc, char *argv[]) {

  in_buf_ctx_cb_handle usrp_read_cb_buffer_handle;

  // Instantiate USRP read circular buffer object.
  input_buffer_ctx_cb_make(&usrp_read_cb_buffer_handle, NOF_USRP_READ_BUFFERS_TEST);

  // Push input buffer read counter into circular buffer.
  for(uint32_t i = 0; i < 10; i++) {
    input_buffer_ctx_cb_push_back(usrp_read_cb_buffer_handle, i);
  }

  for(uint32_t i = 0; i < 10; i++) {
    printf("data[%d]: %d\n", i, input_buffer_ctx_cb_read(usrp_read_cb_buffer_handle, i));
  }

  printf("Getting the front.\n");
  for(uint32_t i = 0; i < 10; i++) {
    printf("data[%d]: %d\n", i, input_buffer_ctx_cb_front(usrp_read_cb_buffer_handle));
  }
  printf("Size: %d\n", input_buffer_ctx_cb_size(usrp_read_cb_buffer_handle));

  printf("Pushing back more values.\n");
  printf("Front data: %d\n", input_buffer_ctx_cb_front(usrp_read_cb_buffer_handle));
  for(uint32_t i = 10; i < 20; i++) {
    input_buffer_ctx_cb_push_back(usrp_read_cb_buffer_handle, i);
    printf("Front data: %d\n", input_buffer_ctx_cb_front(usrp_read_cb_buffer_handle));
  }

  printf("\nSize: %d\n", input_buffer_ctx_cb_size(usrp_read_cb_buffer_handle));
  for(uint32_t i = 0; i < 10; i++) {
    printf("data[%d]: %d\n", i, input_buffer_ctx_cb_read(usrp_read_cb_buffer_handle, i));
  }

  // Free circular buffer holding inpit IQ samples buffer context structures.
  input_buffer_ctx_cb_free(&usrp_read_cb_buffer_handle);

  return 0;
}
