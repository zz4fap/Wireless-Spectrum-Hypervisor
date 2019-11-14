#ifndef _TRANSCEIVER_H_
#define _TRANSCEIVER_H_

#define DEFAULT_CFI 1 // We use only one OFDM symbol for control.

#define DEVNAME_B200 "uhd_b200"

#define DEVNAME_X300 "uhd_x300"

#define MAXIMUM_NUMBER_OF_RADIOS 256 // This is related to the number of IP addresses we can get.

#define MAXIMUM_NUMBER_OF_CELLS 504

// ID of Broadcast messages sent by MAC to all nodes.
#define BROADCAST_ID 255

// Define used to enable timestamps to be added to TX data and measure time it takes to receive packet on RX side.
#define ENABLE_TX_TO_RX_TIME_DIFF 0

// Structure used to store parsed arguments.
typedef struct {
  bool enable_cfo_correction;
  uint16_t rnti;
  uint32_t nof_prb;
  uint32_t nof_ports;
  char *rf_args;
  double radio_center_frequency;
  float initial_rx_gain;
  float initial_tx_gain;
  int node_operation;
  unsigned long single_log_duration;
  unsigned long logging_frequency;
  unsigned long max_number_of_dumps;
  char *path_to_start_file;
  char *path_to_log_files;
  uint32_t node_id;
  srslte_datatype_t data_type;
  float sensing_rx_gain;
  uint32_t radio_id;
  bool use_std_carrier_sep;
  float initial_agc_gain;
  double competition_bw;
  uint32_t default_tx_channel;
  uint32_t default_rx_channel;
  float lbt_threshold;
  uint64_t lbt_timeout;
  uint32_t max_turbo_decoder_noi;
  bool phy_filtering;
  srslte_datatype_t iq_dump_data_type;
  float rf_monitor_rx_sample_rate;
  size_t rf_monitor_channel;
  bool lbt_use_fft_based_pwr;
  bool send_tx_stats_to_mac;
  uint32_t max_backoff_period;
  bool iq_dumping;
  bool immediate_transmission;
  bool add_tx_timestamp;
  uint32_t rf_monitor_option;
  int initial_subframe_index;
  float rf_boost;
  float freq_boost;
  uint32_t radio_nof_prb;
  uint32_t num_of_tx_vphys;
  uint32_t num_of_rx_vphys;
  srslte_cp_t phy_cylic_prefix;
  bool add_preamble_to_front;
  bool enable_ifft_adjust;
  uint32_t modulate_with_zeros;
  uint32_t window_type;
  bool plot_rx_info;
  uint32_t vphy_id_rx_info;
  uint32_t channelizer_filter_delay;
  float pss_peak_threshold;
  bool decode_pdcch;
} transceiver_args_t;

#endif // _TRANSCEIVER_H_
