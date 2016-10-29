#!/bin/sh

CONT_IP=$1
HOST_IP=$2

ping -c 1 -q -W 3 $CONT_IP && \
ping -c 1 -q -W 3 $HOST_IP && \
ip link set eth0 up && \
echo 'ok' && exit 0 ||\
{ echo 'net test failed' ; exit 1; }
