#! /bin/bash

mkdir -p bin
g++ -std=c++11 -I./src/include src/util.cpp src/aucont_start.cpp -o bin/aucont_start
g++ -std=c++11 -I./src/include src/util.cpp src/aucont_stop.cpp -o bin/aucont_stop
g++ -std=c++11 -I./src/include src/util.cpp src/aucont_list.cpp -o bin/aucont_list
g++ -std=c++11 -I./src/include src/util.cpp src/aucont_exec.cpp -o bin/aucont_exec
