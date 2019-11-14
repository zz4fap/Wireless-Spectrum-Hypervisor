message("Executing compileProto.sh")
execute_process(COMMAND /bin/bash compileProto.sh WORKING_DIRECTORY ${PROJECT_SOURCE_DIR} RESULT_VARIABLE proto_res)
message("Output: '${proto_res}'")

cmake_minimum_required(VERSION 2.8)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -Wall")

SET(LIBS protobuf zmq m boost_thread pthread)

add_definitions(-O3)
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

add_subdirectory(communicator)
add_subdirectory(srslte)
add_subdirectory(phy)
