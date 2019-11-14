#include "helpers.h"

uint32_t helpers_get_prb_from_bw_index(uint32_t bw_idx) {
  uint32_t nof_prb;
  switch(bw_idx) {
    case BW_IDX_OneDotFour:
      nof_prb = 6;
      break;
    case BW_IDX_Three:
      nof_prb = 15;
      break;
    case BW_IDX_Five:
      nof_prb = 25;
      break;
    case BW_IDX_Ten:
      nof_prb = 50;
      break;
    case BW_IDX_Fifteen:
      nof_prb = 75;
      break;
    case BW_IDX_Twenty:
      nof_prb = 100;
      break;
    case BW_IDX_Fourty:
      nof_prb = 200;
      break;
    case BW_IDX_Eighty:
      nof_prb = 400;
      break;
    default:
      nof_prb = 25;
  }
  return nof_prb;
}

void helpers_get_data_time_string(char* date_time_str) {
  struct timeval tmnow;
  struct tm *tm;
  char usec_buf[20];
  gettimeofday(&tmnow, NULL);
  tm = localtime(&tmnow.tv_sec);
  strftime(date_time_str,30,"[%d/%m/%Y %H:%M:%S", tm);
  strcat(date_time_str,".");
  sprintf(usec_buf,"%06ld]",tmnow.tv_usec);
  strcat(date_time_str,usec_buf);
}

float helpers_get_bandwidth(uint32_t index, uint32_t *bw_idx) {

  float bw;

  switch(index) {
    case BW_IDX_OneDotFour:
      bw = 128.0*15000.0;
      break;
    case BW_IDX_Three:
      bw = 256.0*15000.0;
      break;
    case BW_IDX_Five:
      bw = 384.0*15000.0;
      break;
    case BW_IDX_Ten:
      bw = 768.0*15000.0;
      break;
    case BW_IDX_Fifteen:
      bw = 1024.0*15000.0;
      break;
    case BW_IDX_Twenty:
      bw = 1536.0*15000.0;
      break;
    case BW_IDX_Fourty:
      bw = 3072.0*15000.0;
      break;
    case BW_IDX_Eighty:
      bw = 6144.0*15000.0;
      break;
    case BW_IDX_UNKNOWN:
    default:
      bw = -1.0;
  }

  *bw_idx = communicator_get_bw_index(index);

  return bw;
}

float helpers_get_bandwidth_float(uint32_t index) {

  float bw;

  switch(index) {
    case BW_IDX_OneDotFour:
      bw = 128.0*15000.0;
      break;
    case BW_IDX_Three:
      bw = 256.0*15000.0;
      break;
    case BW_IDX_Five:
      bw = 384.0*15000.0;
      break;
    case BW_IDX_Ten:
      bw = 768.0*15000.0;
      break;
    case BW_IDX_Fifteen:
      bw = 1024.0*15000.0;
      break;
    case BW_IDX_Twenty:
      bw = 1536.0*15000.0;
      break;
    case BW_IDX_Fourty:
      bw = 3072.0*15000.0;
      break;
    case BW_IDX_Eighty:
      bw = 6144.0*15000.0;
      break;
    case BW_IDX_UNKNOWN:
    default:
      bw = -1.0;
  }

  return bw;
}

uint32_t helpers_get_bw_index_from_prb(uint32_t nof_prb) {
  uint32_t bw_idx;
  switch(nof_prb) {
    case 6:
      bw_idx = BW_IDX_OneDotFour;
      break;
    case 15:
      bw_idx = BW_IDX_Three;
      break;
    case 25:
      bw_idx = BW_IDX_Five;
      break;
    case 50:
      bw_idx = BW_IDX_Ten;
      break;
    case 75:
      bw_idx = BW_IDX_Fifteen;
      break;
    case 100:
      bw_idx = BW_IDX_Twenty;
      break;
    case 200:
      bw_idx = BW_IDX_Fourty;
      break;
    case 400:
      bw_idx = BW_IDX_Eighty;
      break;
    default:
      bw_idx = 0;
  }
  return bw_idx;
}

void helpers_print_control_msg(phy_ctrl_t* const phy_ctrl) {
  switch(phy_ctrl->trx_flag) {
    case TRX_TX_ST:
    case TRX_RX_ST:
      helpers_print_slot_control(phy_ctrl);
      break;
    case TRX_RADIO_TX_ST:
    case TRX_RADIO_RX_ST:
      helpers_print_hypervisor_control(phy_ctrl);
      break;
    default:
      HELPERS_ERROR("Invalid TRX flag!!\n",0);
  }
}

void helpers_print_phy_control(phy_ctrl_t* const phy_control) {
  printf("************** PHY Control **************\n");
  printf("trx_flag: %s\n",TRX_MODE(phy_control->trx_flag));
  printf("nof_slots: %d\n",phy_control->nof_slots);
  helpers_print_slot_control(phy_control);
  if(phy_control->trx_flag == TRX_RADIO_RX_ST || phy_control->trx_flag == TRX_RADIO_TX_ST) {
    helpers_print_hypervisor_control(phy_control);
  }
  printf("*****************************************\n\n");
}

void helpers_print_slot_info(slot_info_t* const slot_info) {
  printf("largest_nof_tbs_in_slot: %d\n",slot_info->largest_nof_tbs_in_slot);
  for(uint32_t i = 0; i < slot_info->largest_nof_tbs_in_slot; i++) {
    printf("sf[%d]: %d\n",i,slot_info->nof_active_vphys_per_slot[i]);
  }
}

void helpers_print_slot_control(phy_ctrl_t* const phy_control) {
  for(uint32_t i = 0; i < phy_control->nof_slots; i++) {
    printf("****** Slot control: %d ******\n",i);
    helpers_print_slot_info(&phy_control->slot_ctrl[i].slot_info);
    printf("vphy_id: %d\n",phy_control->slot_ctrl[i].vphy_id);
    printf("timestamp: %" PRIu64 "\n",phy_control->slot_ctrl[i].timestamp);
    printf("seq_number: %d\n",phy_control->slot_ctrl[i].seq_number);
    printf("send_to: %d\n",phy_control->slot_ctrl[i].send_to);
    printf("bw_idx: %s\n",BW_STRING(phy_control->slot_ctrl[i].bw_idx));
    printf("channel: %d\n",phy_control->slot_ctrl[i].ch);
    printf("frame: %d\n",phy_control->slot_ctrl[i].frame);
    printf("slot: %d\n",phy_control->slot_ctrl[i].slot);
    printf("MCS: %d\n",phy_control->slot_ctrl[i].mcs);
    printf("freq_boost: %1.2f\n",phy_control->slot_ctrl[i].freq_boost);
    printf("length: %d\n",phy_control->slot_ctrl[i].length);
    if(phy_control->slot_ctrl[i].data != NULL) {
      printf("Data: ");
      for(uint32_t k = 0; k < phy_control->slot_ctrl[i].length; k++) {
        if(k < phy_control->slot_ctrl[i].length-1) {
          printf("%d, ",phy_control->slot_ctrl[i].data[k]);
        } else {
          printf("%d\n",phy_control->slot_ctrl[i].data[k]);
        }
      }
    }
  }
}

void helpers_print_hypervisor_control(phy_ctrl_t* const phy_control) {
  printf("****** Hypervisor control ******\n");
  printf("radio_center_frequency: %1.4f [MHz]\n",phy_control->hypervisor_ctrl.radio_center_frequency/1e6);
  printf("gain: %d [dB]\n",phy_control->hypervisor_ctrl.gain);
  printf("radio_nof_prb: %d\n",phy_control->hypervisor_ctrl.radio_nof_prb);
  printf("vphy_nof_prb: %d\n",phy_control->hypervisor_ctrl.vphy_nof_prb);
  printf("RF boost: %1.2f\n",phy_control->hypervisor_ctrl.rf_boost);
}

void helpers_print_basic_control(phy_ctrl_t* const phy_ctrl) {
  printf("*************************** PHY Control ***************************\n");
  printf("TRX mode: %s\n",TRX_MODE(phy_ctrl->trx_flag));
  printf("Number of slots: %d\n",phy_ctrl->nof_slots);
  for(uint32_t i = 0; i < phy_ctrl->nof_slots; i++) {
    printf("vphy_id: %d\n",phy_ctrl->slot_ctrl[i].vphy_id);
    printf("timestamp: %" PRIu64 "\n",phy_ctrl->slot_ctrl[i].timestamp);
    printf("Seq. number: %d\n",phy_ctrl->slot_ctrl[i].seq_number);
    printf("BW: %s\n",BW_STRING(phy_ctrl->slot_ctrl[i].bw_idx));
    printf("Channel: %d\n",phy_ctrl->slot_ctrl[i].ch);
    printf("Frame: %d\n",phy_ctrl->slot_ctrl[i].frame);
    printf("Slot: %d\n",phy_ctrl->slot_ctrl[i].slot);
    printf("MCS: %d\n",phy_ctrl->slot_ctrl[i].mcs);
    if(phy_ctrl->trx_flag == TRX_RX_ST) { // RX
      printf("Num of slots: %d\n",phy_ctrl->slot_ctrl[i].length);
    } else if(phy_ctrl->trx_flag == TRX_TX_ST) { // TX
      printf("Length: %d\n",phy_ctrl->slot_ctrl[i].length);
      printf("Data: ");
      for(uint32_t i = 0; i < phy_ctrl->slot_ctrl[i].length; i++) {
        if(i < (phy_ctrl->slot_ctrl[i].length-1)) {
          printf("%d, ", phy_ctrl->slot_ctrl[i].data[i]);
        } else {
          printf("%d\n", phy_ctrl->slot_ctrl[i].data[i]);
        }
      }
    }
  }
  printf("*********************************************************************\n");
}

void helpers_print_rx_statistics(phy_stat_t* const phy_stat) {
  // Print PHY RX Stats Structure.
  printf("******************* PHY RX Statistics *******************\n"\
    "Seq. number: %d\n"\
    "Status: %d\n"\
    "Host Timestamp: %" PRIu64 "\n"\
    "FPGA Timestamp: %d\n"\
    "Channel: %d\n"\
    "MCS: %d\n"\
    "Num. Packets: %d\n"\
    "Num. Errors: %d\n"\
    "Gain: %d\n"\
    "CQI: %d\n"\
    "RSSI: %1.2f\n"\
    "RSRP: %1.2f\n"\
    "RSRQ: %1.2f\n"\
    "SINR: %1.2f\n"\
    "Length: %d\n"\
    "*********************************************************************\n"\
    ,phy_stat->seq_number,phy_stat->status,phy_stat->host_timestamp,phy_stat->fpga_timestamp,phy_stat->ch,phy_stat->mcs,phy_stat->num_cb_total,phy_stat->num_cb_err,phy_stat->stat.rx_stat.gain,phy_stat->stat.rx_stat.cqi,phy_stat->stat.rx_stat.rssi,phy_stat->stat.rx_stat.rsrp,phy_stat->stat.rx_stat.rsrq,phy_stat->stat.rx_stat.sinr,phy_stat->stat.rx_stat.length);
}

void helpers_print_tx_statistics(phy_stat_t* const phy_stat) {
  // Print PHY TX Stats Structure.
  printf("******************* PHY TX Statistics *******************\n"\
    "Seq. number: %d\n"\
    "Status: %d\n"\
    "Host Timestamp: %" PRIu64 "\n"\
    "FPGA Timestamp: %d\n"\
    "Channel: %d\n"\
    "MCS: %d\n"\
    "Num. Packets: %d\n"\
    "Num. Errors: %d\n"\
    "RF boost: %d\n"\
    "Frequency boost: %d\n"\
    "*********************************************************************\n"\
    ,phy_stat->seq_number,phy_stat->status,phy_stat->host_timestamp,phy_stat->fpga_timestamp,phy_stat->ch,phy_stat->mcs,phy_stat->num_cb_total,phy_stat->num_cb_err,phy_stat->stat.tx_stat.rf_boost,phy_stat->stat.tx_stat.freq_boost);
}

void helpers_print_subframe(cf_t* const subframe, int num_of_samples, bool start_of_burst, bool end_of_burst) {
  if(start_of_burst) {
    printf("************** SOB **************\n");
  }
  for(uint32_t i = 0; i < num_of_samples; i++) {
    printf("sample: %d - (%1.3f,%1.3f)\n",i,crealf(subframe[i]),cimagf(subframe[i]));
  }
  if(end_of_burst) {
    printf("************** EOB **************\n");
  } else {
    printf("\n");
  }
}

// Stick a specific thread to a CPU.
// core_id = 0, 1, ... n-1, where n is the system's number of cores.
int helpers_stick_this_thread_to_core(int core_id) {
  int num_cores = sysconf(_SC_NPROCESSORS_ONLN);
  if(core_id < 0 || core_id >= num_cores) {
    return EINVAL;
  }

  cpu_set_t cpuset;
  CPU_ZERO(&cpuset);
  CPU_SET(core_id, &cpuset);

  pthread_t current_thread = pthread_self();
  return pthread_setaffinity_np(current_thread, sizeof(cpu_set_t), &cpuset);
}
