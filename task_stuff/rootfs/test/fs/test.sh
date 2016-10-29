#!/bin/sh

cat /proc/self/cmdline > /dev/null &&\
cat /sys/kernel/vmcoreinfo > /dev/null &&\
dd if=/dev/zero of=/dev/null bs=1 count=100 2>&1 > /dev/null &&\
ls -l /dev/shm/ > /dev/null &&\
ls -l /dev/mqueue > /dev/null &&\
mount -t tmpfs none /mnt && \
echo 'mount list shouldnt contain host only mount points' && \
mount && \
echo 'ok' && exit 0 ||\
{ echo 'fs test failed' ; exit 1; }
