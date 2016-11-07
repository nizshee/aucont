cmake_minimum_required(VERSION 3.6)
project(my_aucont)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

set(SOURCE_FILES
    task_stuff/scripts/aucont.py
    task_stuff/scripts/run_test.sh
    task_stuff/scripts/test.py
    task_stuff/scripts/test_utils.py
    tools/src/aucont_exec.cpp
    tools/src/aucont_list.cpp
    tools/src/aucont_start.cpp
    tools/src/aucont_stop.cpp
    tools/src/util.cpp
    tools/src/include/util.h
    tools/build.sh)

add_executable(my_aucont ${SOURCE_FILES})