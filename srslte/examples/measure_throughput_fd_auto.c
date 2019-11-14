#include "measure_throughput_fd_auto.h"

static tput_context_t *tput_context;

int main(int argc, char *argv[]) {

  char source_module[] = "MODULE_MAC";
  char target_module[] = "MODULE_PHY";

  change_process_priority(-20);

  // Allocate memory for a new Tput context object.
  tput_context = (tput_context_t*)srslte_vec_malloc(sizeof(tput_context_t));
  // Check if memory allocation was correctly done.
  if(tput_context == NULL) {
    printf("Error allocating memory for Tput context object.\n");
    exit(-1);
  }

  tput_context->go_exit = false;

  // Initialize signal handler.
  initialize_signal_handler();

  // Retrieve input arguments.
  parse_args(&tput_context->args, argc, argv);

  // Create communicator.
  communicator_make(source_module, target_module, &tput_context->handle);

  // Wait some time before starting anything.
  sleep(2);

  // Start Rx thread.
  printf("[Main] Starting Tx side thread...\n");
  start_tx_side_thread(tput_context);

  // Run Tx side loop.
  printf("[Main] Starting Rx side...\n");
  rx_side(tput_context);

  // Wait some time so that all packets are transmitted.
  printf("[Main] Leaving the application in 2 seconds...\n");
  sleep(2);

  // Stop Rx side thread.
  printf("[Main] Stopping Tx side thread...\n");
  stop_tx_side_thread(tput_context);

  // Free communicator related objects.
  communicator_free(&tput_context->handle);

  // Free memory used to store Tput context object.
  if(tput_context) {
    free(tput_context);
    tput_context = NULL;
  }

  return 0;
}

void rx_side(tput_context_t *tput_context) {
  phy_ctrl_t phy_ctrl;
  phy_stat_t phy_rx_stat;
  uchar data[10000];
  bool ret, is_1st_packet = true, phy_stats = false;
  uint64_t nof_rec_bytes = 0, tput_avg_cnt = 0;
  uint32_t errors = 0, nof_errors = 0, pkt_cnt = 0;
  float prr = 0.0, rssi = 0.0;
  struct timespec time_1st_packet;
  double time_diff = 0.0;
  double tput = 0, tput_avg = 0;
  double min_tput = DBL_MAX, max_tput = 0.0;
  double tput_interval = ((double)tput_context->args.interval)*1000.0;

  // Open log file.
  char tput_file_name[200];
  sprintf(tput_file_name,"tput_prb_%d_mcs_%d_slots_%d.txt",get_prb_index_from_bw_idx(tput_context->args.phy_bw_idx),tput_context->args.mcs,tput_context->args.nof_slots_to_tx);
  FILE *f = fopen(tput_file_name, "w");

  // Assign address of data vector to PHY Rx stat structure.
  phy_rx_stat.stat.rx_stat.data = data;

  // Set priority to RX thread.
  uhd_set_thread_priority(1.0, true);

  clock_gettime(CLOCK_REALTIME, &time_1st_packet);

  // Create basic control message to control Tx chain.
  createPhyControl(&phy_ctrl, TRX_RX_ST, 66, tput_context->args.phy_bw_idx, tput_context->args.rx_channel, 0, 0, tput_context->args.rx_gain, 1, NULL);

  // Send RX PHY control to PHY.
  communicator_send_phy_control(tput_context->handle, &phy_ctrl);

  // Loop until otherwise said.
  printf("[Rx side] Starting Rx side thread loop...\n");
  while(!tput_context->go_exit && tput_avg_cnt < 6) {

    // Retrieve message sent by PHY.
    // Try to retrieve a message from the QUEUE. It waits for a specified amount of time before timing out.
    ret = communicator_get_high_queue_wait_for(tput_context->handle, 500, (void * const)&phy_rx_stat, NULL);

    // If message is properly retrieved and parsed, then relay it to the correct module.
    if(!tput_context->go_exit && ret) {

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

              if(tput < min_tput) {
                min_tput = tput;
              }

              if(tput > max_tput) {
                max_tput = tput;
              }

              nof_errors = errors;
              printf("[Rx side] MCS: %d - RSSI: %1.2f - PRR: %1.2f - # errors: %d - Local tput: %f [bps]\n", phy_rx_stat.mcs, rssi, prr, nof_errors, tput);

              fprintf(f, "%f\t%f\t%f\t%d\t%f\n", min_tput, max_tput, tput, nof_errors, prr);
            }
          }

          if(pkt_cnt >= 500) {
            // Store errors for each one of the PHYs.
            errors = (phy_rx_stat.stat.rx_stat.detection_errors+phy_rx_stat.stat.rx_stat.decoding_errors);
            // Store PRR for each one of the PHYs.
            prr = phy_rx_stat.num_cb_total/phy_rx_stat.stat.rx_stat.total_packets_synchronized;
            // Store RSSI for each one of the PHYs.
            rssi = phy_rx_stat.stat.rx_stat.rssi;
            // Set PHY stats flag.
            phy_stats = true;
            // Reset flags and counters.
            if(phy_stats) {
              // Reset both phy stats flags.
              phy_stats = false;
              // Reset packet counter.
              pkt_cnt = 0;
        }
          } else {
            // Increment packet counter.
            pkt_cnt++;
      }
    }
  }
    }
  }
  printf("[Rx side] Min tput: %f - Max tput: %f - Average tput: %f [bps]\n",min_tput,max_tput,(tput_avg/tput_avg_cnt));

  fprintf(f, "%f\t%f\t%f\t%d\t%f\n", min_tput, max_tput, (tput_avg/tput_avg_cnt), nof_errors, prr);

  // Close log file.
  fclose(f);
}

void *tx_side_work(void *h) {
  tput_context_t *tput_context = (tput_context_t*)h;
  phy_ctrl_t phy_ctrl;
  uint64_t timestamp, time_now;
  int64_t packet_cnt = 0;
  unsigned int tb_size = get_tb_size((tput_context->args.phy_bw_idx-1), tput_context->args.mcs);
  int numOfBytes = tput_context->args.nof_slots_to_tx*tb_size;
  unsigned int usecs = 1000*tput_context->args.nof_slots_to_tx;
  // Create some data.
  uchar data[numOfBytes];
  generateData(numOfBytes, data);

  // Set priority to RX thread.
  uhd_set_thread_priority(1.0, true);

  uint64_t time_offset = (uint64_t)(tput_context->args.nof_slots_to_tx*1000.0 + 1000.0);

  printf("[Tx side] timestamp offset: %" PRIu64 "\n", time_offset);

  // Create phy control message to control Tx chain.
  createPhyControl(&phy_ctrl, TRX_TX_ST, 66, tput_context->args.phy_bw_idx, tput_context->args.tx_channel, 0, tput_context->args.mcs, tput_context->args.tx_gain, numOfBytes, data);

  // Retrieve time now.
  time_now = get_host_time_now_us();
  // First frame will be always sent some ms in advance.
  timestamp = time_now + 2000;

  while(tput_context->run_tx_side_thread && packet_cnt != tput_context->args.nof_packets_to_tx) {

    //printf("timestamp: %" PRIu64 "\n", basic_ctrl.timestamp);

    // Send data to specific PHY.
    phy_ctrl.slot_ctrl[0].timestamp = timestamp; // Set timestamp
    communicator_send_phy_control(tput_context->handle, &phy_ctrl);

    // Wait for the duration of the frame.
    usleep(usecs);

    // Add some time to the current time.
    timestamp += time_offset;

    packet_cnt++;
  }

  printf("[Tx side] Leaving Tx side thread.\n",0);
  // Exit thread with result code.
  pthread_exit(NULL);
}

void sig_int_handler(int signo) {
  if(signo == SIGINT) {
    tput_context->go_exit = true;
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
  args->nof_packets_to_tx = -1;
}

int start_tx_side_thread(tput_context_t *tput_context) {
  // Enable Rx side thread.
  tput_context->run_tx_side_thread = true;
  // Create thread attr and Id.
  pthread_attr_init(&tput_context->tx_side_thread_attr);
  pthread_attr_setdetachstate(&tput_context->tx_side_thread_attr, PTHREAD_CREATE_JOINABLE);
  // Create thread.
  int rc = pthread_create(&tput_context->tx_side_thread_id, &tput_context->tx_side_thread_attr, tx_side_work, (void *)tput_context);
  if(rc) {
    printf("[Tx side] Return code from Tx side pthread_create() is %d\n", rc);
    return -1;
  }
  return 0;
}

int stop_tx_side_thread(tput_context_t *tput_context) {
  tput_context->run_tx_side_thread = false; // Stop Tx side thread.
  pthread_attr_destroy(&tput_context->tx_side_thread_attr);
  int rc = pthread_join(tput_context->tx_side_thread_id, NULL);
  if(rc) {
    printf("[Tx side] Return code from Tx side pthread_join() is %d\n", rc);
    return -1;
  }
  return 0;
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

void parse_args(tput_test_args_t *args, int argc, char **argv) {
  int opt;
  default_args(args);
  while((opt = getopt(argc, argv, "bgikmnprst0123456789")) != -1) {
    switch(opt) {
    case 'b':
      args->tx_gain = atoi(argv[optind]);
      printf("[Input argument] Tx gain: %d\n", args->tx_gain);
      break;
    case 'g':
      args->rx_gain = atoi(argv[optind]);
      printf("[Input argument] Rx gain: %d\n", args->rx_gain);
      break;
    case 'i':
      args->interval = atoi(argv[optind]);
      printf("[Input argument] Tput interval: %d\n", args->interval);
      break;
    case 'k':
      args->nof_packets_to_tx = atoi(argv[optind]);
      printf("[Input argument] Number of packets to transmit: %d\n", args->nof_packets_to_tx);
      break;
    case 'm':
      args->mcs = atoi(argv[optind]);
      printf("[Input argument] MCS: %d\n", args->mcs);
      break;
    case 'n':
      args->nof_slots_to_tx = atoi(argv[optind]);
      printf("[Input argument] Number of consecutive slots to be transmitted: %d\n", args->nof_slots_to_tx);
      break;
    case 'p':
      args->phy_bw_idx = helpers_get_bw_index_from_prb(atoi(argv[optind]));
      printf("[Input argument] PHY BW in PRB: %d - Mapped index: %d\n", atoi(argv[optind]), args->phy_bw_idx);
      break;
    case 'r':
      args->rx_channel = atoi(argv[optind]);
      printf("[Input argument] Rx channel: %d\n", args->rx_channel);
      break;
    case 's':
      args->tx_side = false;
      printf("[Input argument] Tx side: %d\n", args->tx_side);
      break;
    case 't':
      args->tx_channel = atoi(argv[optind]);
      printf("[Input argument] Tx channel: %d\n", args->tx_channel);
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

uint32_t get_prb_index_from_bw_idx(uint32_t bw_idx) {
  uint32_t prb;
  switch(bw_idx) {
    case BW_IDX_OneDotFour:
      prb = 6;
      break;
    case BW_IDX_Three:
      prb = 15;
      break;
    case BW_IDX_Five:
      prb = 25;
      break;
    case BW_IDX_Ten:
      prb = 50;
      break;
    case BW_IDX_Fifteen:
      prb = 75;
      break;
    case BW_IDX_Twenty:
      prb = 100;
      break;
    default:
      prb = 25;
  }
  return prb;
}

void change_process_priority(int inc) {
  errno = 0;
  if(nice(inc) == -1) {
    if(errno != 0) {
      printf("Something went wrong with nice(): %s - Perhaps you should run it as root.\n",strerror(errno));
    }
  }
  // Set priority to RX thread.
  uhd_set_thread_priority(1.0, true);
}
