#
# Copyright 2013-2015 Software Radio Systems Limited
#
# This file is part of the srsLTE library.
#
# srsLTE is free software: you can redistribute it and/or modify
# it under the terms of the GNU Affero General Public License as
# published by the Free Software Foundation, either version 3 of
# the License, or (at your option) any later version.
#
# srsLTE is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU Affero General Public License for more details.
#
# A copy of the GNU Affero General Public License can be found in
# the LICENSE file in the top-level directory of this distribution
# and at http://www.gnu.org/licenses/.
#

#################################################################
# Generate communicator_test executable.
#################################################################
include_directories("/usr/local/include")
include_directories("/usr/local/lib")
include_directories("../../../communicator/cpp")

link_directories("/usr/local/lib")

add_executable(test_communicator test_communicator.c)
target_link_libraries(test_communicator srslte communicator protobuf zmq m boost_thread pthread)

add_executable(mcf_tdma_basic_test mcf_tdma_basic_test.c ../phy/helpers.c)
target_link_libraries(mcf_tdma_basic_test srslte communicator protobuf zmq m boost_thread pthread)

add_executable(read_phy_samples_with_np read_phy_samples_with_np.c)
target_link_libraries(read_phy_samples_with_np srslte communicator protobuf zmq m boost_thread pthread)

add_executable(check_hypervisor_read check_hypervisor_read.c)
target_link_libraries(check_hypervisor_read srslte communicator protobuf zmq m boost_thread pthread)

add_executable(mcf_tdma_check_channel_change mcf_tdma_check_channel_change.c ../phy/helpers.c)
target_link_libraries(mcf_tdma_check_channel_change srslte communicator protobuf zmq m boost_thread pthread)

add_executable(phy_test phy_test.c ../phy/helpers.c)
target_link_libraries(phy_test srslte communicator protobuf zmq m boost_thread pthread)

add_executable(test_vec_prod test_vec_prod.c)
target_link_libraries(test_vec_prod srslte)

add_executable(test_circular_buffer test_circular_buffer.c)
target_link_libraries(test_circular_buffer srslte communicator protobuf zmq m boost_thread pthread)

add_executable(test_memory_allocation test_memory_allocation.c)
target_link_libraries(test_memory_allocation srslte communicator protobuf zmq m boost_thread pthread)

#################################################################
# Applications
#################################################################

add_executable(synch_file synch_file.c)
target_link_libraries(synch_file srslte)

#################################################################
# These can be compiled without UHD or graphics support
#################################################################

add_executable(hypervisor_test hypervisor_test.c)
target_link_libraries(hypervisor_test srslte pthread communicator protobuf zmq m boost_thread liquid)

add_executable(test_windowing test_windowing.c)
target_link_libraries(test_windowing srslte pthread communicator protobuf zmq m boost_thread liquid)

add_executable(read_dumped_file read_dumped_file.c)
target_link_libraries(read_dumped_file srslte pthread)

add_executable(test_complex_short test_complex_short.c)
target_link_libraries(test_complex_short srslte pthread)

add_executable(pdsch_ue_pkt pdsch_ue_pkt.c)
target_link_libraries(pdsch_ue_pkt srslte pthread)

add_executable(pdsch_enodeb_pkt pdsch_enodeb_pkt.c)
target_link_libraries(pdsch_enodeb_pkt srslte pthread)

add_executable(pdsch_ue pdsch_ue.c)
target_link_libraries(pdsch_ue srslte pthread)

add_executable(pdsch_enodeb pdsch_enodeb.c)
target_link_libraries(pdsch_enodeb srslte pthread)

add_executable(test_fft test_fft.c)
target_link_libraries(test_fft srslte pthread boost_thread)

add_executable(measure_throughput measure_throughput.c ../phy/helpers.c)
target_link_libraries(measure_throughput srslte pthread communicator protobuf zmq m boost_thread)

add_executable(measure_throughput_fd_auto measure_throughput_fd_auto.c ../phy/helpers.c)
target_link_libraries(measure_throughput_fd_auto srslte pthread communicator protobuf zmq m boost_thread)

add_executable(decode_subframe decode_subframe.c)
target_link_libraries(decode_subframe srslte pthread communicator protobuf zmq m boost_thread)

add_executable(cfo_check cfo_check.c)
target_link_libraries(cfo_check srslte pthread communicator protobuf zmq m boost_thread)

if(RF_FOUND)

else(RF_FOUND)
  add_definitions(-DDISABLE_RF)
endif(RF_FOUND)

find_package(SRSGUI)

if(SRSGUI_FOUND)
  include_directories(${SRSGUI_INCLUDE_DIRS})
  target_link_libraries(pdsch_ue ${SRSGUI_LIBRARIES})
  target_link_libraries(pdsch_ue_pkt ${SRSGUI_LIBRARIES})
else(SRSGUI_FOUND)
  add_definitions(-DDISABLE_GRAPHICS)
endif(SRSGUI_FOUND)


#################################################################
# These examples need the UHD driver
#################################################################

if(RF_FOUND)

  add_executable(cell_search cell_search.c)
  target_link_libraries(cell_search srslte)

  add_executable(cell_measurement cell_measurement.c)
  target_link_libraries(cell_measurement srslte)

  add_executable(usrp_capture usrp_capture.c)
  target_link_libraries(usrp_capture srslte)

  add_executable(usrp_capture_sync usrp_capture_sync.c)
  target_link_libraries(usrp_capture_sync srslte)

  add_executable(usrp_txrx usrp_txrx.c)
  target_link_libraries(usrp_txrx srslte)

  message(STATUS "   examples will be installed.")

else(RF_FOUND)
  message(STATUS "   examples will NOT BE INSTALLED.")
endif(RF_FOUND)

# Add subdirectories
add_subdirectory(tutorial_examples)
