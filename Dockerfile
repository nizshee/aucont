FROM eabatalov/aucont16-test-base

MAINTAINER Roman Vasiliev

COPY tools /test/aucont
COPY task_stuff/scripts /test/scripts
COPY task_stuff/rootfs /test/rootfs

USER root

RUN chown -R dev:dev /test
RUN chmod -R 777 /test

USER dev

