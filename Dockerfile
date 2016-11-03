FROM debian
MAINTAINER Roman Vasilev

RUN apt-get update
RUN apt-get install -y g++
RUN apt-get install -y cmake
RUN apt-get install -y python3

COPY task_stuff/rootfs /test/rootfs
COPY task_stuff/scripts /test/scripts
COPY tools /test/aucont

RUN cd /test/aucont/ && cmake .
RUN cd /test/aucont/ && make


