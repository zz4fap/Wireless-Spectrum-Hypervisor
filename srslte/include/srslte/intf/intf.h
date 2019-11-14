#ifndef _INTF_H_
#define _INTF_H_

#define __STDC_FORMAT_MACROS
#include <inttypes.h>
#include <stdint.h>

// MF-TDMA Design: (May be changed later according to DARPA reference design)
// Basic resource unit in the air: (ch, slot). ch -- channel number; slot -- slot number
// One scheduling period is defined as a frame. For instance, slot 0 ~ slot N-1 compose a frame which contains N slots.
// Scheduler can be updated frame by frame. Frame number can be defined from 0 to max_frame_number.
// When a (ch, slot) is scheduled, MAC send multiple TB(transportation block) to PHY, PHY will split a TB to many CB(code block) if a TB is too long.
// CB size has upper limit (in LTE it is 6144 bits). If multiple CBs are generated for one TB, except the last CB, all previous CB must have 6144 bits.
// Before MAC send data to PHY, MAC should query PHY info: current (ch, slot), target (ch, slot) abliity (how many bits can be carried under specific MCS)
// PHY CB HARQ won't be supported in the first step because of potential timing performance.
// MAC TB ARQ may be defined in MAC layer. Network and application layer may design their own ARQ scheme.

// Revision History
// 2017 Feb. 21: Drafting some interfaces/design to be exposed to AI, not MAC-PHY interface/design (maybe some overlapping and reuse in the future, not intentional).
// 2017 Feb. 21: Addition of more statistics parameters to PHY Rx Statistics struct.
// 2017 Feb. 22: Removal of BLER and CRC from PHY Rx Statistics struct. Addition of CRC to MAC statistics. Modification of signed integers to unsigned integers. Some code alignment.

// Maximum number of concurrent vPHYs running in parallel.
#define MAX_NUM_CONCURRENT_VPHYS 12
// Define the maximum number of TBs that can be sent in a single slot message.
#define MAX_NUMBER_OF_TBS_IN_A_SLOT 20
// Size of the current user data, i.e., one row in the user data buffer has this length.
#define USER_DATA_BUFFER_LEN MAX_NUMBER_OF_TBS_IN_A_SLOT*549 // 100 TB * TB size for 1.4 MHz PHY BW and MCS 28, i.e., 549 bytes.
// Macro defining the maximum MCS value.
#define MAX_MCS_VALUE 28

typedef enum {PHY_UNKNOWN_ST=0, TRX_RX_ST=1, TRX_TX_ST=2, TRX_RADIO_RX_ST=3, TRX_RADIO_TX_ST=4} trx_flag_e;

typedef enum {RX_STAT=0, TX_STAT=1, SENSING_STAT=2} stat_e;

typedef enum {PHY_UNKNOWN=0, PHY_SUCCESS=100, PHY_ERROR=101, PHY_TIMEOUT=102, PHY_LBT_TIMEOUT=103} phy_status_e;

typedef enum {BW_IDX_UNKNOWN=0, BW_IDX_OneDotFour=1, BW_IDX_Three=2, BW_IDX_Five=3, BW_IDX_Ten=4, BW_IDX_Fifteen=5, BW_IDX_Twenty=6, BW_IDX_Fourty=7, BW_IDX_Eighty=8} bw_index_e;

typedef enum {MODULE_UNKNOWN=0, MODULE_PHY=1, MODULE_MAC=2, MODULE_AI=3, MODULE_RF_MON=4, MODULE_APP=5, MODULE_PHY_DEBUG_1=6, MODULE_PHY_DEBUG_2=7, MODULE_MAC_DEBUG_1=8, MODULE_MAC_DEBUG_2=9} module_index_e;

typedef enum {WINDOW_UNKNOWN=0, WINDOW_HANN=1, WINDOW_KAISER=2, WINDOW_LIQUID=3} window_e;

typedef unsigned char uchar;

// Extended Basic control and scheduler definition
// Extended Basic control -- extended basic control header for both Tx and Rx
typedef struct {
  trx_flag_e trx_flag;	// 0 -- Rx; 1 -- Tx; may have more status in the future.
  uint64_t seq_number;  // Sequence number used by upper layer to track the response of PHY, i.e., correlates one basic_control message with a phy_stat message.
  uint32_t send_to;     // This field gives the SRN ID of the radio the we are sending a packet to.
  uint32_t intf_id;     // Interface ID. For McF-TDMA it is the vPHY ID you want to transmit to.
  uint32_t bw_idx;      // Channel BW: 1.4, 3, 5 and 10 MHz.
  uint32_t ch; 			    // To Tx or Rx at this channel
  uint32_t frame;       // Frame number
  uint32_t slot; 		    // To Tx or Rx at this slot
  uint64_t timestamp;   // Time at which slot should be sent given in milliseconds.
  uint32_t mcs; 		    // Set mcs for Tx; When in Rx state it is automatically found by the receiver.
  int32_t gain; 		    // Tx or Rx gain. dB. For Rx, -1 means AGC mode.
  double rf_boost;      // RF boost amplification
  uint32_t length;      // During Tx state, it indicates number of bytes after this header. It must be an integer times the TB size. During Rx indicates number of expected slots to be received.
  uchar *data;          // Data to be transmitted.
} basic_ctrl_t;

typedef struct {
  uint32_t largest_nof_tbs_in_slot;                                 // Largest number of TBs in a slot comming in a PHY control message.
  uint32_t nof_active_vphys_per_slot[MAX_NUMBER_OF_TBS_IN_A_SLOT];  // Number of active vPHYs in a given 1 ms period.
} slot_info_t;

// Define PHY control message. This new message is used to allow McF-TDMA operation between MAC and PHY.
typedef struct {
  uint32_t vphy_id;                 // Indicates which vPHY this control will be forwarded to.
  uint64_t timestamp;               // Time at which the fisrt subframe should be sent given in microseconds.
  slot_info_t slot_info;            // Information on the slots to be sent.
  uint64_t seq_number;              // Sequence number used by upper layer to track the response of PHY, i.e., correlates one basic_control message with a phy_stat message.
  uint32_t send_to;                 // This field gives the ID of radio the we are sending a packet to.
  uint32_t intf_id;                 // Interface ID. For McF-TDMA it is the vPHY ID you want to transmit to.
  uint32_t bw_idx;                  // Channel BW: 1.4, 3, 5 and 10 MHz.
  uint32_t ch; 			                // To Tx or Rx at this channel
  uint32_t frame;                   // Frame number
  uint32_t slot; 		                // To Tx or Rx at this slot
  uint32_t mcs; 		                // Set MCS for Tx. When in Rx state it is automatically detected by the receiver.
  double freq_boost;                // Gain given in the frequency domain to the modulation symbols before being mapped into subcarriers.
  uint32_t nof_tb_in_slot;          // Number of TBs in this slot control message.
  uint32_t length;                  // During Tx state, it indicates number of bytes after this header. It must be an integer times the TB size. During Rx indicates number of expected slots to be received.
  uchar *data;                      // Data to be transmitted.
} slot_ctrl_t;

typedef struct {
  uint32_t radio_id;              // Defines which one of the two radios should have its configuration changed.
  double radio_center_frequency;  // Defines the competition center frequency.
  int32_t gain; 		              // Tx or Rx gain. given in dB. For Rx, -1 means AGC mode.
  uint32_t radio_nof_prb;         // Defines the sampling rate for the hypervisor to work at.
  uint32_t vphy_nof_prb;          // Defines the number of Resource blocks for each one of the vPHYs.
  double rf_boost;                // RF boost amplification given to the IFFT signal just before it is sent to the USRP.
} hypervisor_ctrl_t;

typedef struct {
  trx_flag_e trx_flag;	                            // 0 -- Rx; 1 -- Tx; may have more status in the future.
  uint32_t nof_slots;                               // Specify the number of elements in the struct slot_ctrl.
  slot_ctrl_t slot_ctrl[MAX_NUM_CONCURRENT_VPHYS];  // Structure containing up to MAX_NUM_CONCURRENT_VPHYS slot parameters. MAX_NUM_CONCURRENT_VPHYS equals the number of channels.
  hypervisor_ctrl_t hypervisor_ctrl;                // Hypervisor/Radio control parameters.
} phy_ctrl_t;

// PHY Tx statistics
typedef struct {
  int32_t gain; 			// Hardware (USRP) Tx gain. Given in dB.
  uint64_t channel_free_cnt;
  uint64_t channel_busy_cnt;
  double free_energy;
  double busy_energy;
  uint64_t total_dropped_slots;
  double coding_time;
  double rf_boost;    // Gain applied to IFFT signal just before being transmitted.
  double freq_boost;  // Gain apllied to frequency domain modulation symbols just before being processed by IFFT.
} phy_tx_stat_t;

// PHY Rx statistics
typedef struct {
  uint32_t nof_slots_in_frame;  // This field indicates the number decoded from SSS, indicating the number of subframes part of a MAC frame.
  uint32_t slot_counter;        // This field indicates the slot number inside of a MAC frame.
  int32_t gain; 		            // Rx gain. dB. For receiver, -1 means AGC mode. No need to contro gain by outside.
  uint32_t cqi; 		            // Channel Quality Indicator. Range: [1, 15]
  float rssi; 			            // Received Signal Strength Indicator. Range: [–2^31, (2^31) - 1]. dBm*10. For example, value -567 means -56.7dBm.
  float rsrp;				            // Reference Signal Received Power. Range: [-1400, -400]. dBm*10. For example, value -567 means -56.7dBm.
  float rsrq;				            // Reference Signal Receive Quality. Range: [-340, -25]. dB*10. For example, value 301 means 30.1 dB.
  float sinr; 			            // Signal to Interference plus Noise Ratio. Range: [–2^31, (2^31) - 1]. dB.
  uint64_t detection_errors;
  uint64_t decoding_errors;
  uint64_t filler_bits_error;                     // Specific error when there is a decoding error.
  uint64_t nof_cbs_exceeds_softbuffer_size_error; // Specific error when there is a decoding error.
  uint64_t rate_matching_error;                   // Specific error when there is a decoding error.
  uint64_t cb_crc_error;                          // Specific error when there is a decoding error.
  uint64_t tb_crc_error;                          // Specific error when there is a decoding error.
  float cfo;
  float peak_value;
  float noise;
  int32_t decoded_cfi;
  bool found_dci;
  int32_t last_noi;
  uint64_t total_packets_synchronized;
  double decoding_time;
  double synch_plus_decoding_time;
  int32_t length;   // How many bytes are after this header. It should be equal to current TB size.
  uchar *data;      // Data received by the PHY.
} phy_rx_stat_t;

typedef struct {
  float frequency;    // Central frequency used for sensing the channel.
  float sample_rate;  // Sample rate used for sensing the channel.
  float gain;         // Rx gain used for sensing the channel.
  float rssi;         // RSSI of the received IQ samples.
  int32_t length;			// How many bytes are after this header. It should be equal to current TB size.
  uchar *data;        // Data received by the PHY.
} phy_sensing_stat_t;

typedef union {
  phy_tx_stat_t tx_stat;
  phy_rx_stat_t rx_stat;
  phy_sensing_stat_t sensing_stat;
} stat_t;

// Statistics
typedef struct {
  uint32_t vphy_id;     // ID of the virtual PHY that handled this message.
  uint64_t seq_number;  // Sequence number used by upper layer to track the response of PHY, i.e., correlates one basic_control message with a phy_stat message.
  uint32_t status;      // Indicates whether there were some kind of problem during Tx or Rx or if it was successful. We must define some error codes.
  uint64_t host_timestamp;
	uint64_t fpga_timestamp;
	uint32_t frame;
  uint32_t slot;
	uint32_t ch;
	uint32_t mcs;
	uint32_t num_cb_total;
	uint32_t num_cb_err;
	uint32_t wrong_decoding_counter;
  stat_t   stat;
} phy_stat_t;

typedef struct {
  uint32_t channel_buffer_rd_pos; // read position within the channel buffers.
} channel_buffer_context_t;

typedef struct {
  // ID of the virtual PHY that handled this message.
  uint32_t vphy_id;
  // Current channel the vPHY will listen to.
  uint32_t vphy_channel;
  // Current buffer being read.
  uint32_t current_buffer_idx; // MUST be initialized to zero.
  // Number of remaining samples in current buffer.
  uint32_t nof_remaining_samples; // MUST be initialized to the maxium number of samples in a buffer, i.e., 1920.
  // Number of samples read from the current buffer.
  uint32_t nof_read_samples; // MUST be initialized to zero.
} vphy_reader_t;

#endif // _INTF_H_
