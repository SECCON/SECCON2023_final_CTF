FROM ubuntu:22.04
ENV DEBIAN_FRONTEND noninteractive

RUN apt-get -y update
RUN apt-get -y install \
    libprotobuf-dev    \
    libnl-route-3-dev  \
    build-essential

WORKDIR /app
ADD flag.txt    .
ADD nsjail      .
ADD nsjail.conf .
ADD main.c      .
RUN chmod 444 flag.txt && chmod 555 nsjail

RUN sed -i "s/flag-XXXX/flag-$(md5sum flag.txt | cut -d ' ' -f 1)/" \
    main.c nsjail.conf
RUN gcc main.c -o readflag -static && chmod 111 readflag

CMD ./nsjail --config nsjail.conf
