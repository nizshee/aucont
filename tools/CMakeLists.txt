cmake_minimum_required(VERSION 3.0)
project(aucont)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

set(SOURCE_FILES src/main.cpp src/util.h)
set(START src/aucont_start.cpp src/util.h)

add_executable(aucont_start ${START})
add_executable(aucont ${SOURCE_FILES})
