#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdbool.h>
#define __STDC_FORMAT_MACROS
#include <inttypes.h>
#include <signal.h>
#include <errno.h>

#include <uhd.h>
#include "srslte/srslte.h"

typedef _Complex float cf_t;

bool go_exit = false;

void sigIntHandler(int signo);
void initializeSignalHandler();
void changeProcessPriority();
int vphyRecv(int fd, cf_t *samples, uint32_t nof_samples);

int main() {
  int fd1, ret;
  cf_t samples[1920];
  char nfifo_name[100];

  //changeProcessPriority();

  // Create named pipe name.
  uint32_t vphy_id = 0;
  sprintf(nfifo_name, "/tmp/nfifo_vphy_%d", vphy_id);

  // Creating the named file(FIFO)
  mkfifo(nfifo_name, 0666);

  printf("sizeof(cf_t)): %ld\n",sizeof(cf_t));

  // First open in read only and read
  fd1 = open(nfifo_name, O_RDONLY);

  while(!go_exit) {

    ret = vphyRecv(fd1, samples, 1920);

    if(ret > 0) {
      if(ret != 1920) {
        printf("Problem: %d !!!!!!!!!!!!!!!\n", ret);
      }
    } else {
      printf("Error reading samples from PHY: %d.\n", ret);
    }
  }

  close(fd1);

  return 0;
}

int vphyRecv(int fd, cf_t *samples, uint32_t nof_samples) {
  int ret;
  int nof_reads = 0;
  int nof_to_read = nof_samples*sizeof(cf_t);

  do {
    ret = read(fd, &samples[nof_reads], sizeof(cf_t));
    if(ret > 0) {
      nof_reads += ret/sizeof(cf_t);
      nof_to_read -= ret;
    }
  } while(nof_reads < nof_samples);
  return nof_reads;
}

void sigIntHandler(int signo) {
  if(signo == SIGINT) {
    go_exit = true;
    printf("SIGINT received. Exiting...\n");
  }
}

void initializeSignalHandler() {
  sigset_t sigset;
  sigemptyset(&sigset);
  sigaddset(&sigset, SIGINT);
  sigprocmask(SIG_UNBLOCK, &sigset, NULL);
  signal(SIGINT, sigIntHandler);
}

void changeProcessPriority() {
  errno = 0;
  // Set priority of main thread.
  uhd_set_thread_priority(1.0, true);
  // Set nice to highest priority.
  if(nice(-20) == -1) {
    if(errno != 0) {
      printf("Something went wrong with nice(): %s - Perhaps you should run it as root.\n",strerror(errno));
    }
  }
}
