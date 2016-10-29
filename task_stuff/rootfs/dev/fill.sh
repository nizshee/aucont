#!/bin/bash

mkdir mqueue
mkdir shm
sudo mknod null c 1 3
sudo mknod urandom c 1 9
sudo mknod zero c 1 5
sudo chown 1000:1000 ./*
