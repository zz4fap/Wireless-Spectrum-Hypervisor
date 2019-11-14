#ifndef _HELPERS_H_
#define _HELPERS_H_

#include <sys/time.h>
#define __STDC_FORMAT_MACROS
#include <inttypes.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

#include "srslte/srslte.h"
#include "srslte/intf/intf.h"
#include "../../../../communicator/cpp/communicator_wrapper.h"

#define PHY_CHANNEL 0
#define RF_MONITOR_CHANNEL 1

#define ENABLE_HELPERS_PRINTS 1

#define HELPERS_PROFILING_ENABLED 0

#define HELPERS_DEBUG_MODE 0

#define HELPERS_PRINT(_fmt, ...) if(scatter_verbose_level >= 0 && ENABLE_HELPERS_PRINTS) \
  fprintf(stdout, "[HELPERS PRINT]: " _fmt, __VA_ARGS__)

#define HELPERS_DEBUG(_fmt, ...) if(scatter_verbose_level >= SRSLTE_VERBOSE_DEBUG && ENABLE_HELPERS_PRINTS) \
  fprintf(stdout, "[HELPERS DEBUG]: " _fmt, __VA_ARGS__)

#define HELPERS_INFO(_fmt, ...) if(scatter_verbose_level >= SRSLTE_VERBOSE_INFO && ENABLE_HELPERS_PRINTS) \
  fprintf(stdout, "[HELPERS INFO]: " _fmt, __VA_ARGS__)

#define HELPERS_ERROR(_fmt, ...) do { fprintf(stdout, "[HELPERS ERROR]: " _fmt, __VA_ARGS__); } while(0)

#define SCATTER_PROFILING_GETTIME(timestamp) if(SCATTER_PROFILING_ENABLED) \
  clock_gettime(CLOCK_REALTIME, &timestamp)

#define SCATTER_PROFILING_DIFFTIME(timestart, timeend) if(SCATTER_PROFILING_ENABLED) \
  profiling_diff_time(timestart, timeend)

#define HELPERS_PRINT_PHY_CONTROL(phy_control) do { if(HELPERS_DEBUG_MODE) \
  helpers_print_phy_control(phy_control); } while(0)

#define HELPERS_PRINT_CONTROL_MSG(phy_control) do { if(HELPERS_DEBUG_MODE) \
  helpers_print_control_msg(phy_control); } while(0)

#define HELPERS_PRINT_RX_STATS(phy_stat) do { if(HELPERS_DEBUG_MODE) \
  helpers_print_rx_statistics(phy_stat); } while(0)

#define HELPERS_PRINT_TX_STATS(phy_stat) do { if(HELPERS_DEBUG_MODE) \
  helpers_print_tx_statistics(phy_stat); } while(0)

#define TRX_MODE(flag) flag==PHY_UNKNOWN_ST ? "TRX_UNKNOWN":(flag==TRX_RX_ST ? "RX_MODE":(flag==TRX_TX_ST ? "TX_MODE":(flag==TRX_RADIO_RX_ST ? "RADIO_RX":(flag==TRX_RADIO_TX_ST ? "RADIO_TX":"INVALID"))))

#define BW_STRING(bw_idx) bw_idx==BW_IDX_UNKNOWN ? "BW_UNKNOWN":(bw_idx==BW_IDX_OneDotFour ? "BW_1DOT4_MHZ":(bw_idx==BW_IDX_Three ? "BW_3_MHZ":(bw_idx==BW_IDX_Five ? "BW_5_MHZ":(bw_idx==BW_IDX_Ten ? "BW_10_MHZ":"BW_INVALID"))))

#define FRAME_DURATION 10 // given in milliseconds.
#define SLOT_DURATION 1   // given in milliseconds.

#define FRAME_LENGTH 1024 // Number of
#define SLOT_LENGTH 10    // Number of slots in a frame.

// This function returns time in Milliseconds
inline double fpga_time_diff(time_t full_secs_start, double frac_secs_start, time_t full_secs_end, double frac_secs_end) {

  double diff_full_secs, diff_frac_secs;

  if(frac_secs_end < frac_secs_start) {
      diff_full_secs = full_secs_end - full_secs_start - 1.0;
      diff_frac_secs = frac_secs_end - frac_secs_start + 1.0;
  } else {
      diff_full_secs = full_secs_end - full_secs_start;
      diff_frac_secs = frac_secs_end - frac_secs_start;
  }

  return (diff_full_secs*1000.0 + diff_frac_secs*1000.0);
}

// This function returns time in Milliseconds
inline double helpers_time_diff(struct timespec start, struct timespec stop) {

  struct timespec diff;

  if(stop.tv_nsec < start.tv_nsec) {
      diff.tv_sec = stop.tv_sec - start.tv_sec - 1;
      diff.tv_nsec = stop.tv_nsec - start.tv_nsec + 1000000000;
  } else {
      diff.tv_sec = stop.tv_sec - start.tv_sec;
      diff.tv_nsec = stop.tv_nsec - start.tv_nsec;
  }

  return (double)(diff.tv_sec*1000) + (double)(diff.tv_nsec/1.0e6);
}

// Function used to verify if this is time to timeout reception.
inline bool helpers_is_timeout(struct timespec time_start, double timeout) {
  bool ret  = false;
  struct timespec time_now;
  clock_gettime(CLOCK_REALTIME, &time_now);
  double diff = helpers_time_diff(time_start, time_now);
  if(diff >= timeout) {
    ret = true;
  }
  return ret;
}

// Retrieve host PC time value now
inline uint64_t helpers_get_host_time_now() {
  struct timespec host_timestamp;
  // Retrieve current time from host PC.
  clock_gettime(CLOCK_REALTIME, &host_timestamp);
  return (uint64_t)host_timestamp.tv_sec*1000000LL + (uint64_t)(host_timestamp.tv_nsec/1000LL);
}

// Convert to a timestamp in microseconds.
inline uint64_t helpers_convert_host_time_now(struct timespec *host_timestamp) {
  return (uint64_t)host_timestamp->tv_sec*1000000LL + (uint64_t)((double)host_timestamp->tv_nsec/1000LL);
}

// Convert to timestamp in nanoseconds.
inline uint64_t helpers_convert_host_timestamp(struct timespec *timestamp) {
  return (uint64_t)timestamp->tv_sec*1000000000LL + (uint64_t)(timestamp->tv_nsec/1LL);
}

// Retrieve FPGA time value now
inline uint64_t helpers_get_fpga_time_now(srslte_rf_t *rf) {
  time_t secs;
  double frac_secs;
  // Retrieve current time from USRP hardware.
  srslte_rf_get_time(rf, &secs, &frac_secs);
  return (secs*1000000 + frac_secs*1000); 	// FPGA internal counter for global time slot synchronization
}

// Retrieve FPGA time value now with precision in microseconds.
inline uint64_t helpers_convert_fpga_time_into_int(time_t full_secs, double frac_secs) {
  return (full_secs*1000000 + frac_secs*1000000); 	// FPGA internal counter for global time slot synchronization
}

// Retrieve FPGA time value now with precision in nanoseconds.
inline uint64_t helpers_convert_fpga_time_into_uint64_nanoseconds(time_t full_secs, double frac_secs) {
  return (full_secs*1000000000 + frac_secs*1000000000); 	// FPGA internal counter for global time slot synchronization
}

inline uint64_t helpers_convert_fpga_time_into_int2(srslte_timestamp_t timestamp) {
  return (timestamp.full_secs*1000000 + timestamp.frac_secs*1000); 	// FPGA internal counter for global time slot synchronization
}

// Return difference in milliseconds.
inline double helpers_profiling_diff_time(struct timespec timestart) {
  struct timespec timeend;
  clock_gettime(CLOCK_REALTIME, &timeend);
  return helpers_time_diff(timestart, timeend);
}

inline uintmax_t helpers_get_current_frame_slot(unsigned int *frame, unsigned int *slot) {
   struct timespec host_time_now;
   // Retrieve current time from host PC.
   clock_gettime(CLOCK_REALTIME, &host_time_now);
   uintmax_t time_now_in_milliseconds = (uintmax_t)(host_time_now.tv_sec*1000LL) + (uintmax_t)host_time_now.tv_nsec/1000000LL;
   *frame = (time_now_in_milliseconds / FRAME_DURATION) % FRAME_LENGTH;
   *slot = (time_now_in_milliseconds / SLOT_DURATION) % SLOT_LENGTH;
   return time_now_in_milliseconds;
}

inline uintmax_t helpers_get_time_in_future(unsigned int future_frame, unsigned int future_slot, unsigned int current_frame, unsigned int current_slot) {

   if(future_slot < current_slot) {
      current_frame = future_frame - current_frame - 1;
      current_slot = future_slot - current_slot + 10;
   } else {
      current_frame = future_frame - current_frame;
      current_slot = future_slot - current_slot;
   }

   return (current_frame*FRAME_DURATION + current_slot*SLOT_DURATION);
}

inline void helpers_increment_frame_subframe(unsigned int *frame, unsigned int *slot) {
   if(*slot == SLOT_LENGTH-1) {
      *slot = 0;
      if(*frame == FRAME_LENGTH-1) {
         *frame = 0;
      } else {
         *frame = *frame + 1;
      }
   } else {
      *slot = *slot + 1;
   }
}

// convert into miliseconds.
inline void helpers_convert_host_timestamp_into_uhd_timestamp_ms(uint64_t timestamp, time_t *full_secs, double *frac_secs) {
  *full_secs = (time_t)(timestamp/1000LL);
  uint64_t aux = timestamp - ((*full_secs)*1000LL);
  *frac_secs = aux/1000.0;
  HELPERS_DEBUG("timestamp: %" PRIu64 " - full_secs: %ld - frac_secs: %1.9f\n",timestamp, *full_secs, *frac_secs);
}

// convert timestamp coming from upper layers into microseconds.
inline void helpers_convert_host_timestamp_into_uhd_timestamp_us(uint64_t timestamp, time_t *full_secs, double *frac_secs) {
  *full_secs = (time_t)(timestamp/1000000LL);
  uint64_t aux = timestamp - ((*full_secs)*1000000LL);
  *frac_secs = aux/1000000.0;
  HELPERS_DEBUG("timestamp: %" PRIu64 " - full_secs: %ld - frac_secs: %1.9f\n",timestamp, *full_secs, *frac_secs);
}

// convert into nanoseconds.
inline void helpers_convert_host_timestamp_into_uhd_timestamp_ns(uint64_t timestamp, time_t *full_secs, double *frac_secs) {
  *full_secs = (time_t)(timestamp/1000000000LL);
  uint64_t aux = timestamp - ((*full_secs)*1000000000LL);
  *frac_secs = aux/1000000000.0;
  HELPERS_DEBUG("timestamp: %" PRIu64 " - full_secs: %ld - frac_secs: %1.9f\n",timestamp, *full_secs, *frac_secs);
}

// This function returns timestamp with microseconds precision.
inline uint64_t helpers_get_host_timestamp_us() {
  struct timespec host_timestamp;
  // Retrieve current time from host PC.
  clock_gettime(CLOCK_REALTIME, &host_timestamp);
  return (uintmax_t)(host_timestamp.tv_sec*1000000LL) + (uintmax_t)host_timestamp.tv_nsec/1000LL;
}

// This function returns FPGA timestamp with miliseconds precision.
inline uint64_t helpers_get_fpga_timestamp_ms(srslte_rf_t *rf) {
  time_t secs;
  double frac_secs;
  // Retrieve current time from USRP hardware.
  srslte_rf_get_time(rf, &secs, &frac_secs);
  return (secs*1000 + frac_secs*1000);
}

// This function returns FPGA timestamp with microseconds precision.
inline uint64_t helpers_get_fpga_timestamp_us(srslte_rf_t* const rf) {
  time_t full_secs;
  double frac_secs;
  // Retrieve current time from USRP hardware.
  srslte_rf_get_time(rf, &full_secs, &frac_secs);
  return (full_secs*1000000 + frac_secs*1000000);
}

inline double helpers_get_bw_from_nprb(uint32_t nof_prb) {
  double bw;
  switch(nof_prb) {
    case 6:
      bw = 128.0*15000.0;
      break;
    case 15:
      bw = 256.0*15000.0;
      break;
    case 25:
      bw = 384.0*15000.0;
      break;
    case 50:
      bw = 768.0*15000.0;
      break;
    case 75:
      bw = 1024.0*15000.0;
      break;
    case 100:
      bw = 1536.0*15000.0;
      break;
    case 200:
      bw = 3072.0*15000.0;
      break;
    case 400:
      bw = 6144.0*15000.0;
      break;
    default:
      bw = -1.0;
  }
  return bw;
}

inline uint32_t helpers_get_prb_from_bw(uint32_t bw) {
  uint32_t prb;
  switch(bw) {
    case 1400000:
      prb = 6;
      break;
    case 3000000:
      prb = 15;
      break;
    case 5000000:
      prb = 25;
      break;
    case 10000000:
      prb = 50;
      break;
    case 15000000:
      prb = 75;
      break;
    case 20000000:
      prb = 100;
      break;
    case 40000000:
      prb = 200;
      break;
    case 80000000:
      prb = 400;
      break;
    default:
      prb = 25;
  }
  return prb;
}

inline int helpers_get_fft_size(uint32_t nof_prb) {
  if(nof_prb <= 0) {
    return SRSLTE_ERROR;
  }
  if(nof_prb <= 6) {
    return 64;
  } else if(nof_prb <= 15) {
    return 128;
  } else if(nof_prb <= 25) {
    return 192;
  } else if(nof_prb <= 50) {
    return 384;
  } else if(nof_prb <= 75) {
    return 512;
  } else if(nof_prb <= 100) {
    return 768;
  } else if(nof_prb <= 200) {
    return 1536;
  } else if(nof_prb <= 400) {
    return 3072;
  } else {
    return SRSLTE_ERROR;
  }
}

inline int helpers_non_std_sampling_freq_hz(uint32_t nof_prb) {
  int n = helpers_get_fft_size(nof_prb);
  return 15000 * n;
}

// Calculate central frequency for the vPHY channel.
inline double helpers_calculate_vphy_channel_center_frequency(double center_frequency, double radio_sampling_rate, double vphy_sampling_rate, uint32_t channel) {
  double channel_center_freq = (center_frequency - (radio_sampling_rate/2.0) + channel*vphy_sampling_rate);
  return channel_center_freq;
}

// This fucntion is be used to measure the number of TX or RX packets per second.
static inline void helpers_measure_packets_per_second(char *str) {
  // Timestamp.
  struct timespec packet_transmitted_timenow;
  uint64_t packet_current_timestamp;
  static uint64_t packet_measurement_start_timestamp = 0, packet_counter = 0, max_value = 0, min_value = 100000, avg_packet_counter = 0, avg_counter = 0;

  clock_gettime(CLOCK_REALTIME, &packet_transmitted_timenow);
  packet_current_timestamp = helpers_convert_host_timestamp(&packet_transmitted_timenow);

  if(packet_measurement_start_timestamp == 0) {
    packet_measurement_start_timestamp = packet_current_timestamp;
  }
  if(packet_current_timestamp <= (packet_measurement_start_timestamp+1000000000)) {
    packet_counter++;
  } else {
    avg_packet_counter += packet_counter;
    avg_counter++;
    if(packet_counter > max_value) max_value = packet_counter;
    if(packet_counter < min_value) min_value = packet_counter;
    packet_measurement_start_timestamp = 0;
    packet_counter = 0;
    if(avg_counter%10==0)
      printf("Avg. number of %s packets in 1 [s]: %0.2f - min: %lu - max: %lu\n", str, ((double)avg_packet_counter/avg_counter), min_value, max_value);
  }
}

inline void helpers_print_resource_grid(cf_t* const vector, uint32_t length, uint32_t nof_re_in_one_ofdm_symbol) {
  uint32_t symbol_cnt = 0;
  for(uint32_t idx = 0; idx < length; idx++) {
    if((idx%nof_re_in_one_ofdm_symbol) == 0) {
      if(idx > 0) {
        printf("\n\n\n");
      }
      printf("****** OFDM Symbol #%d ******\n",symbol_cnt);
      symbol_cnt++;
    }
    printf("sample[%d]: (%f,%f)\n", idx, __real__ vector[idx], __imag__ vector[idx]);
  }
}

inline void helpers_print_resource_grid_gt_zero(cf_t* const vector, uint32_t length) {
  for(uint32_t idx = 0; idx < length; idx++) {
    if(fabs(crealf(vector[idx])) > 0.0 || fabs(cimagf(vector[idx])) > 0.0)
      printf("sample[%d]: (%f,%f)\n", idx, __real__ vector[idx], __imag__ vector[idx]);
  }
}

inline cf_t helpers_random_modulator() {
  cf_t ret;
  int value = rand() % 5;
  switch(value) {
    case 0:
      ret = (1.0/sqrtf(2.0)) + (1.0/sqrtf(2.0))*I;
      break;
    case 1:
      ret = (1.0/sqrtf(2.0)) - (1.0/sqrtf(2.0))*I;
      break;
    case 2:
      ret = -(1.0/sqrtf(2.0)) + (1.0/sqrtf(2.0))*I;
      break;
    case 3:
      ret = -(1.0/sqrtf(2.0)) - (1.0/sqrtf(2.0))*I;
      break;
    case 4:
      ret = 0.0 + 0.0*I;
      break;
    default:
      ret = 0.0 + 0.0*I;
      printf("Invalid value!!!!\n");
  }
  return ret;
}

inline void helpers_get_error_string(int rc, char * error) {
  switch(rc) {
    case EBUSY:
      sprintf(error,"EBUSY");
      break;
    case EINVAL:
      sprintf(error,"EINVAL");
      break;
    case EAGAIN:
      sprintf(error,"EAGAIN");
      break;
    case ENOMEM:
      sprintf(error,"ENOMEM");
      break;
    case EEXIST:
      sprintf(error,"EEXIST");
      break;
    case EACCES:
      sprintf(error,"EACCES");
      break;
    case ELOOP:
      sprintf(error,"ELOOP");
      break;
    default:
      sprintf(error,"Error not mapped");
  }
}

inline void helpers_get_timeout(long timeout, struct timespec *timeout_spec) {
  struct timespec now;
  clock_gettime(CLOCK_REALTIME, &now);
  if((now.tv_nsec + timeout) > 999999999) {
    timeout_spec->tv_sec = now.tv_sec + 1;
    timeout_spec->tv_nsec = (now.tv_nsec + timeout) - 1000000000;
  } else {
    timeout_spec->tv_sec = now.tv_sec;
    timeout_spec->tv_nsec = now.tv_nsec + timeout;
  }
}

void helpers_get_data_time_string(char* date_time_str);

void helpers_print_control_msg(phy_ctrl_t* const phy_ctrl);

void helpers_print_phy_control(phy_ctrl_t* const bc);

void helpers_print_slot_control(phy_ctrl_t* const bc);

void helpers_print_hypervisor_control(phy_ctrl_t* const bc);

void helpers_print_rx_statistics(phy_stat_t* const phy_stat);

void helpers_print_tx_statistics(phy_stat_t* const phy_stat);

uint32_t helpers_get_prb_from_bw_index(uint32_t bw_idx);

float helpers_get_bandwidth(uint32_t index, uint32_t *bw_idx);

uint32_t helpers_get_bw_index_from_prb(uint32_t nof_prb);

float helpers_get_bandwidth_float(uint32_t index);

void helpers_print_subframe(cf_t* const subframe, int num_of_samples, bool start_of_burst, bool end_of_burst);

int helpers_stick_this_thread_to_core(int core_id);

#endif // _HELPERS_H_
