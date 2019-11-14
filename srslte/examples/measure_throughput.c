#include "measure_throughput.h"

static bool go_exit = false;

void parse_args(tput_test_args_t *args, int argc, char **argv) {
  int opt;
  default_args(args);
  while((opt = getopt(argc, argv, "bgimnprst0123456789")) != -1) {
    switch(opt) {
    case 'b':
      args->tx_gain = atoi(argv[optind]);
      printf("Tx gain: %d\n", args->tx_gain);
      break;
    case 'g':
      args->rx_gain = atoi(argv[optind]);
      printf("Rx gain: %d\n", args->rx_gain);
      break;
    case 'i':
      args->interval = atoi(argv[optind]);
      printf("Tput interval: %d\n", args->interval);
      break;
    case 'm':
      args->mcs = atoi(argv[optind]);
      printf("MCS: %d\n", args->mcs);
      break;
    case 'n':
      args->nof_slots_to_tx = atoi(argv[optind]);
      printf("nof_slots_to_tx: %d\n", args->nof_slots_to_tx);
      break;
    case 'p':
      args->phy_bw_idx = helpers_get_bw_index_from_prb(atoi(argv[optind]));
      printf("PHY BW in PRB: %d - Mapped index: %d\n", atoi(argv[optind]), args->phy_bw_idx);
      break;
    case 'r':
      args->rx_channel = atoi(argv[optind]);
      printf("Rx channel: %d\n", args->rx_channel);
      break;
    case 's':
      args->tx_side = false;
      printf("Tx side: %d\n", args->tx_side);
      break;
    case 't':
      args->tx_channel = atoi(argv[optind]);
      printf("Tx channel: %d\n", args->tx_channel);
      break;
    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9':
      break;
    default:
      printf("Error parsing arguments...\n");
      exit(-1);
    }
  }
}

void sig_int_handler(int signo) {
  if(signo == SIGINT) {
    go_exit = true;
    printf("SIGINT received. Exiting...\n",0);
  }
}

void initialize_signal_handler() {
  sigset_t sigset;
  sigemptyset(&sigset);
  sigaddset(&sigset, SIGINT);
  sigprocmask(SIG_UNBLOCK, &sigset, NULL);
  signal(SIGINT, sig_int_handler);
}

void createPhyControl(phy_ctrl_t *phy_ctrl, trx_flag_e trx_flag, uint64_t seq_num, uint32_t bw_idx, uint32_t ch, uint64_t timestamp, uint32_t mcs, int32_t gain, uint32_t length, uchar *data) {
  phy_ctrl->trx_flag = trx_flag;
  phy_ctrl->slot_ctrl[0].seq_number = seq_num;
  phy_ctrl->slot_ctrl[0].bw_idx = bw_idx;
  phy_ctrl->slot_ctrl[0].ch = ch;
  phy_ctrl->slot_ctrl[0].timestamp = timestamp;
  phy_ctrl->slot_ctrl[0].mcs = mcs;
  phy_ctrl->hypervisor_ctrl.gain = gain;
  phy_ctrl->slot_ctrl[0].length = length;
  if(data == NULL) {
    phy_ctrl->slot_ctrl[0].data = NULL;
  } else {
    phy_ctrl->slot_ctrl[0].data = data;
  }
}

void default_args(tput_test_args_t *args) {
  args->phy_bw_idx = BW_IDX_Five;
  args->mcs = 0;
  args->rx_channel = 0;
  args->rx_gain = 10;
  args->tx_channel = 0;
  args->tx_gain = 10;
  args->tx_side = true;
  args->nof_slots_to_tx = 1;
  args->interval = 10;
}

int main(int argc, char *argv[]) {

  char module_name[] = "MODULE_MAC";
  char target_name[] = "MODULE_PHY";
  tput_context_t tput_context;

  // Initialize signal handler.
  initialize_signal_handler();

  parse_args(&tput_context.args, argc, argv);

  // Create communicator.
  communicator_make(module_name, target_name, &tput_context.handle);

  sleep(2);

  if(tput_context.args.tx_side) {
    printf("Starting Tx side...\n");
    tx_side(&tput_context);
  } else {
    printf("Starting Rx side...\n");
    rx_side(&tput_context);
  }

  printf("Leaving the application in 2 seconds...\n");
  sleep(2);
  // Free communicator related objects.
  communicator_free(&tput_context.handle);

  return 0;
}

void rx_side(tput_context_t *tput_context) {
  phy_ctrl_t phy_ctrl;
  phy_stat_t phy_rx_stat;
  uchar data[10000];
  bool ret, is_1st_packet = true;
  uint64_t nof_rec_bytes = 0, tput_avg_cnt = 0;
  struct timespec time_1st_packet;
  double time_diff = 0.0;
  double tput = 0, tput_avg = 0;
  double tput_interval = ((double)tput_context->args.interval)*1000.0;
  // Assign address of data vector to PHY Rx stat structure.
  phy_rx_stat.stat.rx_stat.data = data;

  clock_gettime(CLOCK_REALTIME, &time_1st_packet);

  // Create basic control message to control Tx chain.
  createPhyControl(&phy_ctrl, TRX_RX_ST, 66, tput_context->args.phy_bw_idx, tput_context->args.rx_channel, 0, 0, tput_context->args.rx_gain, 1, NULL);

  // Send RX PHY control to PHY.
  communicator_send_phy_control(tput_context->handle, &phy_ctrl);

  // Loop until otherwise said.
  while(!go_exit) {

    // Retrieve message sent by PHY.
    // Try to retrieve a message from the QUEUE. It waits for a specified amount of time before timing out.
    ret = communicator_get_high_queue_wait_for(tput_context->handle, 1000, (void * const)&phy_rx_stat, NULL);
    // If message is properly retrieved and parsed, then relay it to the correct module.
    if(!go_exit && ret) {

      if(phy_rx_stat.status == PHY_SUCCESS) {

        if(phy_rx_stat.seq_number == 66) {

          nof_rec_bytes += phy_rx_stat.stat.rx_stat.length;

          if(is_1st_packet) {
            is_1st_packet = false;
            clock_gettime(CLOCK_REALTIME, &time_1st_packet);
          } else {
            time_diff = profiling_diff_time(&time_1st_packet);

            if(time_diff >= tput_interval) {

              tput = (nof_rec_bytes*8)/(time_diff/1000.0);

              tput_avg += tput;
              tput_avg_cnt++;

              is_1st_packet = true;
              nof_rec_bytes = 0;

              printf("Local tput: %f [bps]\n",tput);
            }

          }
        }
      }
    }
  }
  printf("Average tput: %f [bps]\n",tput_avg/tput_avg_cnt);
}

void tx_side(tput_context_t *tput_context) {
  phy_ctrl_t phy_ctrl;
  uint32_t nof_tx_slots = 0;
  unsigned int tb_size = get_tb_size((tput_context->args.phy_bw_idx-1), tput_context->args.mcs);
  int numOfBytes = tput_context->args.nof_slots_to_tx*tb_size;
  uint64_t timestamp;
  // Create some data.
  uchar data[numOfBytes];
  generateData(numOfBytes, data);

  uint64_t time_offset = (uint64_t)(tput_context->args.nof_slots_to_tx*1000.0 + 500.0);

  printf("[Tx side] timestamp offset: %" PRIu64 "\n", time_offset);

  // Create basic control message to control Tx chain.
  createPhyControl(&phy_ctrl, TRX_TX_ST, 66, tput_context->args.phy_bw_idx, tput_context->args.tx_channel, 0, tput_context->args.mcs, tput_context->args.tx_gain, numOfBytes, data);

  // Retrieve current time from host PC.
  timestamp = get_host_time_now_us();

  //printf("time now: %" PRIu64 "\n", timestamp);

  while(!go_exit && nof_tx_slots < MAX_TX_SLOTS) {

    // Add some time to the current time.
    timestamp += time_offset;
    phy_ctrl.slot_ctrl[0].timestamp = timestamp;

    communicator_send_phy_control(tput_context->handle, &phy_ctrl);

    usleep(500*tput_context->args.nof_slots_to_tx);

    nof_tx_slots++;
  }
}

// This function returns timestamp with microseconds precision.
inline uint64_t get_host_time_now_us() {
  struct timespec host_timestamp;
  // Retrieve current time from host PC.
  clock_gettime(CLOCK_REALTIME, &host_timestamp);
  return (uint64_t)(host_timestamp.tv_sec*1000000LL) + (uint64_t)((double)host_timestamp.tv_nsec/1000LL);
}

inline double profiling_diff_time(struct timespec *timestart) {
  struct timespec timeend;
  clock_gettime(CLOCK_REALTIME, &timeend);
  return time_diff(timestart, &timeend);
}

inline double time_diff(struct timespec *start, struct timespec *stop) {
  struct timespec diff;
  if(stop->tv_nsec < start->tv_nsec) {
    diff.tv_sec = stop->tv_sec - start->tv_sec - 1;
    diff.tv_nsec = stop->tv_nsec - start->tv_nsec + 1000000000;
  } else {
    diff.tv_sec = stop->tv_sec - start->tv_sec;
    diff.tv_nsec = stop->tv_nsec - start->tv_nsec;
  }
  return (double)(diff.tv_sec*1000) + (double)(diff.tv_nsec/1.0e6);
}

static const unsigned int tbs_table[6][29] = {{19,26,32,41,51,63,75,89,101,117,117,129,149,169,193,217,225,225,241,269,293,325,349,373,405,437,453,469,549}, // Values for 1.4 MHz BW
{49,65,81,109,133,165,193,225,261,293,293,333,373,421,485,533,573,573,621,669,749,807,871,935,999,1063,1143,1191,1383}, // Values for 3 MHz BW
{85,113,137,177,225,277,325,389,437,501,501,549,621,717,807,903,967,967,999,1143,1239,1335,1431,1572,1692,1764,1908,1980,2292}, // Values for 5 MHz BW
{173,225,277,357,453,549,645,775,871,999,999,1095,1239,1431,1620,1764,1908,1908,2052,2292,2481,2673,2865,3182,3422,3542,3822,3963,4587}, // Values for 10 MHz BW
{261,341,421,549,669,839,967,1143,1335,1479,1479,1620,1908,2124,2385,2673,2865,2865,3062,3422,3662,4107,4395,4736,5072,5477,5669,5861,6882}, // Values for 15 MHz BW
{349,453,573,717,903,1095,1287,1527,1764,1980,1980,2196,2481,2865,3182,3542,3822,3822,4107,4587,4904,5477,5861,6378,6882,7167,7708,7972,9422}}; // Values for 20 MHz BW

unsigned int get_tb_size(uint32_t prb, uint32_t mcs) {
  return tbs_table[prb][mcs];
}

void generateData(uint32_t numOfBytes, uchar *data) {
  // Create some data.
  printf("Creating %d data bytes\n",numOfBytes);
  for(int i = 0; i < numOfBytes; i++) {
    data[i] = (uchar)(rand() % 256);
  }
}
