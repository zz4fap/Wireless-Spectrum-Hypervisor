#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/time.h>
#define __STDC_FORMAT_MACROS
#include <inttypes.h>
#include <time.h>
#include <semaphore.h>
#include <assert.h>
#include <signal.h>
#include <pthread.h>
#include <string.h>

#include "srslte/srslte.h"
#include "srslte/intf/intf.h"
#include "../phy/helpers.h"
#include "../../../../communicator/cpp/communicator_wrapper.h"

#define SIZE_TEST 32

#ifdef LV_HAVE_AVX
#include <immintrin.h>

typedef struct SRSLTE_API {
  cf_t y[2][SRSLTE_SSS_N];
  unsigned int aux[4];
} aligned_cf_t;

typedef float complex lv_32fc_t;

typedef struct SRSLTE_API {
  float z1[SRSLTE_SSS_N][SRSLTE_SSS_N];
  //float c[2][SRSLTE_SSS_N];
  float *c;
  float s[SRSLTE_SSS_N][SRSLTE_SSS_N];
  float sd[SRSLTE_SSS_N][SRSLTE_SSS_N-1];
} sss_fc_tables_t;

void my_volk_32fc_32f_multiply_32fc_a_avx(lv_32fc_t* cVector, const lv_32fc_t* aVector, const float* bVector, unsigned int num_points) {
  unsigned int number = 0;
  const unsigned int eighthPoints = num_points / 8;

  lv_32fc_t* cPtr = cVector;
  const lv_32fc_t* aPtr = aVector;
  const float* bPtr=  bVector;

  __m256 aVal1, aVal2, bVal, bVal1, bVal2, cVal1, cVal2;

  __m256i permute_mask = _mm256_set_epi32(3, 3, 2, 2, 1, 1, 0, 0);

  for(;number < eighthPoints; number++){

    aVal1 = _mm256_load_ps((float *)aPtr);
    aPtr += 4;

    aVal2 = _mm256_load_ps((float *)aPtr);
    aPtr += 4;

    bVal = _mm256_load_ps(bPtr); // b0|b1|b2|b3|b4|b5|b6|b7
    bPtr += 8;

    bVal1 = _mm256_permute2f128_ps(bVal, bVal, 0x00); // b0|b1|b2|b3|b0|b1|b2|b3
    bVal2 = _mm256_permute2f128_ps(bVal, bVal, 0x11); // b4|b5|b6|b7|b4|b5|b6|b7

    bVal1 = _mm256_permutevar_ps(bVal1, permute_mask); // b0|b0|b1|b1|b2|b2|b3|b3
    bVal2 = _mm256_permutevar_ps(bVal2, permute_mask); // b4|b4|b5|b5|b6|b6|b7|b7

    cVal1 = _mm256_mul_ps(aVal1, bVal1);
    cVal2 = _mm256_mul_ps(aVal2, bVal2);

    _mm256_store_ps((float*)cPtr,cVal1); // Store the results back into the C container
    cPtr += 4;

    _mm256_store_ps((float*)cPtr,cVal2); // Store the results back into the C container
    cPtr += 4;
  }

  number = eighthPoints * 8;
  for(;number < num_points; ++number){
    *cPtr++ = (*aPtr++) * (*bPtr++);
  }
}
#endif /* LV_HAVE_AVX */

#ifdef LV_HAVE_SSE
#include <xmmintrin.h>

void volk_32fc_32f_multiply_32fc_a_sse(lv_32fc_t* cVector, const lv_32fc_t* aVector,
                                  const float* bVector, unsigned int num_points) {
  unsigned int number = 0;
  const unsigned int quarterPoints = num_points / 4;

  lv_32fc_t* cPtr = cVector;
  const lv_32fc_t* aPtr = aVector;
  const float* bPtr = bVector;

  __m128 aVal1, aVal2, bVal, bVal1, bVal2, cVal;
  for(;number < quarterPoints; number++){

    printf("number 0: %d\n",number);

    aVal1 = _mm_load_ps((const float*)aPtr);
    aPtr += 2;

    printf("number 1: %d\n",number);

    aVal2 = _mm_load_ps((const float*)aPtr);
    aPtr += 2;

    printf("number 2: %d\n",number);

    bVal = _mm_load_ps(bPtr);
    bPtr += 4;

    bVal1 = _mm_shuffle_ps(bVal, bVal, _MM_SHUFFLE(1,1,0,0));
    bVal2 = _mm_shuffle_ps(bVal, bVal, _MM_SHUFFLE(3,3,2,2));

    cVal = _mm_mul_ps(aVal1, bVal1);

    _mm_store_ps((float*)cPtr,cVal); // Store the results back into the C container
    cPtr += 2;

    cVal = _mm_mul_ps(aVal2, bVal2);

    _mm_store_ps((float*)cPtr,cVal); // Store the results back into the C container

    cPtr += 2;
  }

  number = quarterPoints * 4;
  for(;number < num_points; number++){
    *cPtr++ = (*aPtr++) * (*bPtr);
    bPtr++;
  }
}
#endif /* LV_HAVE_SSE */

void vec_prod_cfc(cf_t *x, float *y, cf_t *z, uint32_t len) {
  //my_volk_32fc_32f_multiply_32fc_a_avx(z,x,y,len);
  volk_32fc_32f_multiply_32fc_a_sse(z,x,y,len);
}

int main(int argc, char *argv[]) {

  printf("sizeof(aligned_cf_t): %d\n", sizeof(aligned_cf_t));

  cf_t *y = NULL;
  y = (cf_t*)srslte_vec_malloc(2*SIZE_TEST*sizeof(cf_t));
  if(y == NULL) {
    printf("Error when allocating memory for hypervisor_rx_handle\n", 0);
    return -1;
  }

  sss_fc_tables_t fc_tables[3];
  fc_tables[0].c = (float*)srslte_vec_malloc(2*SIZE_TEST*sizeof(float));
  if(fc_tables[0].c == NULL) {
      printf("Error when allocating memory for hypervisor_rx_handle\n", 0);
      return -1;
  }

  srslte_vec_prod_cfc(y, &fc_tables[0].c[0], y, SIZE_TEST);
  srslte_vec_prod_cfc(&y[SIZE_TEST], &fc_tables[0].c[SIZE_TEST], &y[SIZE_TEST], SIZE_TEST);

  if(y) {
    free(y);
    y = NULL;
  }

  if(fc_tables[0].c) {
    free(fc_tables[0].c);
    fc_tables[0].c = NULL;
  }

  return 0;
}

/*int main(int argc, char *argv[]) {

  cf_t *y = NULL;
  float *c = NULL;

  y = (cf_t*)srslte_vec_malloc(2*SIZE_TEST*sizeof(cf_t));
  if(y == NULL) {
    printf("Error when allocating memory for hypervisor_rx_handle\n", 0);
    return -1;
  }

  c = (float*)srslte_vec_malloc(2*SIZE_TEST*sizeof(float));
  if(c == NULL) {
    printf("Error when allocating memory for hypervisor_rx_handle\n", 0);
    return -1;
  }

  printf("1st time.\n");
  vec_prod_cfc(y, c, y, SIZE_TEST);
  printf("2nd time.\n");
  vec_prod_cfc(&y[SIZE_TEST], &c[SIZE_TEST], &y[SIZE_TEST], SIZE_TEST);

  printf("1st time.\n");
  srslte_vec_prod_cfc(y, c, y, SIZE_TEST);
  printf("2nd time.\n");
  srslte_vec_prod_cfc(&y[SIZE_TEST], &c[SIZE_TEST], &y[SIZE_TEST], SIZE_TEST);


  if(y) {
    free(y);
    y = NULL;
  }

  if(c) {
    free(c);
    c = NULL;
  }

  return 0;
}*/

/*int main(int argc, char *argv[]) {

  cf_t *y = NULL;
  float *c = NULL;

  y = (cf_t*)srslte_vec_malloc(2*SIZE_TEST*sizeof(cf_t));
  if(y == NULL) {
    printf("Error when allocating memory for hypervisor_rx_handle\n", 0);
    return -1;
  }

  c = (float*)srslte_vec_malloc(2*SIZE_TEST*sizeof(float));
  if(c == NULL) {
    printf("Error when allocating memory for hypervisor_rx_handle\n", 0);
    return -1;
  }

  //vec_prod_cfc(y[0], c[0], y[0], SIZE_TEST);
  //vec_prod_cfc(y[1], c[1], y[1], SIZE_TEST);

  srslte_vec_prod_cfc(y, c, y, SIZE_TEST);
  srslte_vec_prod_cfc(&y[SIZE_TEST], &c[SIZE_TEST], &y[SIZE_TEST], SIZE_TEST);

  if(y) {
    free(y);
    y = NULL;
  }

  if(c) {
    free(c);
    c = NULL;
  }

  return 0;
}*/

/*int main(int argc, char *argv[]) {

  cf_t y0[SIZE_TEST], y1[SIZE_TEST];
  float c0[SIZE_TEST], c1[SIZE_TEST];

  srslte_vec_prod_cfc(y0, c0, y0, SIZE_TEST);
  srslte_vec_prod_cfc(y1, c1, y1, SIZE_TEST);

  return 0;
}*/

/*int main(int argc, char *argv[]) {

  cf_t y[2][SIZE_TEST], *y0 = NULL, *y1 = NULL;
  float c[2][SIZE_TEST], *c0 = NULL, *c1 = NULL;

  y1 = (cf_t*)y[1];
  c1 = (float*)c[1];

  //srslte_vec_prod_cfc(y[0], c[0], y[0], SIZE_TEST);
  //srslte_vec_prod_cfc(y[1], c[1], y[1], SIZE_TEST);
  srslte_vec_prod_cfc(y1, c1, y1, SIZE_TEST);

  return 0;
}*/

/*int main(int argc, char *argv[]) {

  cf_t y[2][SIZE_TEST], *y0 = NULL, *y1 = NULL;
  float c[2][SIZE_TEST], *c0 = NULL, *c1 = NULL;

  //srslte_vec_prod_cfc(y[0], c[0], y[0], SIZE_TEST);
  srslte_vec_prod_cfc(y[1], c[1], y[1], SIZE_TEST);

  return 0;
}*/

/*int main(int argc, char *argv[]) {

  cf_t *y[2];
  for(int i = 0; i < 2; i++) {
    y[i] = (cf_t*)srslte_vec_malloc(sizeof(cf_t)*SRSLTE_SSS_N);
    if(y[i] == NULL) {
      printf("Error when allocating memory for hypervisor_rx_handle\n", 0);
      return -1;
    }
    // Initialize structure with zeros.
    bzero(y[i], sizeof(cf_t)*SRSLTE_SSS_N);
  }

  srslte_sss_fc_tables_t *fc_tables[3] = {NULL, NULL, NULL};
  for(int i = 0; i < 3; i++) {
    fc_tables[i] = (srslte_sss_fc_tables_t*)srslte_vec_malloc(sizeof(srslte_sss_fc_tables_t));
    if(fc_tables[i] == NULL) {
      printf("Error when allocating memory for hypervisor_rx_handle\n", 0);
      return -1;
    }
    // Initialize structure with zeros.
    bzero(fc_tables[i], sizeof(srslte_sss_fc_tables_t));
  }

  srslte_vec_prod_cfc(y[0], fc_tables[1]->c[0], y[0], SRSLTE_SSS_N);
  srslte_vec_prod_cfc(y[1], fc_tables[0]->c[1], y[1], SRSLTE_SSS_N);

  for(int i = 0; i < 3; i++) {
    if(fc_tables[i]) {
      free(fc_tables[i]);
      fc_tables[i] = NULL;
    }
  }

  for(int i = 0; i < 2; i++) {
    if(y[i]) {
      free(y[i]);
      y[i] = NULL;
    }
  }

  return 0;
}*/
