FROM ubuntu:22.04@sha256:8eab65df33a6de2844c9aefd19efe8ddb87b7df5e9185a4ab73af936225685bb

ENV DEBIAN_FRONTEND noninteractive

RUN apt-get -y update --fix-missing
RUN apt-get -y install xinetd
RUN groupadd -r pwn && useradd -r -g pwn pwn

ADD build/pwn.xinetd /etc/xinetd.d/pwn
ADD build/init.sh /etc/init.sh
ADD build/run.sh /home/pwn/run.sh
RUN chmod 700 /etc/init.sh
RUN chmod 550 /home/pwn/run.sh
RUN chmod 1733 /tmp /var/tmp /dev/shm

WORKDIR /home/pwn
ADD files/chall chall
RUN chmod 550 chall

ARG FLAG
RUN echo "${FLAG}" > flag.txt
RUN chmod 440 flag.txt
RUN mv flag.txt flag-$(md5sum flag.txt | awk '{print $1}').txt

RUN chown -R root:pwn /home/pwn
