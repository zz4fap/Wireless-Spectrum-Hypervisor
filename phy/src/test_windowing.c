#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <unistd.h>
#include <sys/time.h>
#include <string.h>

#include "srslte/srslte.h"

#include "liquid/liquid.h"

int main(int argc, char *argv[]) {

  int N = 12;
  int L = 73;

  for(int i = 0; i < (L+2*N); i++) {
    //printf("%d: %f\n",i,hann(i,72)); // Go to zero.
    printf("%d: %f\n",i,liquid_rcostaper_windowf(i,N,(L+2*N))); // Do not go to zero, but close to it.
  }
  printf("\n\n");

}
