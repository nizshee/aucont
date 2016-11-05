FROM eabatalov/aucont16-test-base
MAINTAINER Roman Vasilev


# RUN apt-get update
# RUN apt-get install -y g++
# RUN apt-get install -y cmake
# RUN apt-get install -y python3

# RUN chown dev:dev -R /test

# USER dev

COPY task_stuff/rootfs /test/rootfs
COPY task_stuff/scripts /test/scripts
COPY tools /test/aucont

# RUN chown 1000:1000 -R /test

#RUN /test/aucont/build.sh

#RUN cd /test/aucont/ && cmake .
#RUN mkdir /test/aucont/bin
#RUN ls /test/aucont/
#RUN cd /test/aucont/ && make


