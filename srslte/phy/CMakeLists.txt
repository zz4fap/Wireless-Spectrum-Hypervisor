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
include_directories("../rf_monitor")
link_directories("/usr/local/lib")

#################################################################
# These can be compiled without UHD or graphics support
#################################################################

if(RF_FOUND)
  if(ENBALE_SRS_GUI OR ENABLE_PLOT_RX_SPECTRUM)
    add_executable(phy phy.c helpers.c vphy_tx.c vphy_rx.c hypervisor_control.c hypervisor_tx.c hypervisor_rx.c phy_comm_control.c plot.c)
  else(ENBALE_SRS_GUI OR ENABLE_PLOT_RX_SPECTRUM)
    add_executable(phy phy.c helpers.c vphy_tx.c vphy_rx.c hypervisor_control.c hypervisor_tx.c hypervisor_rx.c phy_comm_control.c)
  endif(ENBALE_SRS_GUI OR ENABLE_PLOT_RX_SPECTRUM)
  target_link_libraries(phy srslte pthread rt communicator protobuf zmq m boost_thread liquid)
  message(STATUS "   PHY will be installed.")
else(RF_FOUND)
  add_definitions(-DDISABLE_RF)
  message(STATUS "   PHY will NOT BE INSTALLED.")
endif(RF_FOUND)

find_package(SRSGUI)

if(SRSGUI_FOUND)
  if(ENBALE_SRS_GUI OR ENABLE_PLOT_RX_SPECTRUM)
    include_directories(${SRSGUI_INCLUDE_DIRS})
    target_link_libraries(phy ${SRSGUI_LIBRARIES})
  endif(ENBALE_SRS_GUI OR ENABLE_PLOT_RX_SPECTRUM)
else(SRSGUI_FOUND)
  add_definitions(-DDISABLE_GRAPHICS)
endif(SRSGUI_FOUND)
