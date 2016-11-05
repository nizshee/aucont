cmake_minimum_required(VERSION 3.0)
project(aucont1)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

set(START src/aucont_start.cpp src/util.h)
set(STOP src/aucont_stop.cpp src/util.h)
set(LIST src/aucont_list.cpp src/util.h)
set(EXEC src/aucont_exec.cpp src/util.h)

add_executable(aucont_start ${START})
add_executable(aucont_stop ${STOP})
add_executable(aucont_list ${LIST})
add_executable(aucont_exec ${EXEC})
