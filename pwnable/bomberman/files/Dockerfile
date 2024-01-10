FROM ubuntu:22.04@sha256:c9cf959fd83770dfdefd8fb42cfef0761432af36a764c077aed54bbc5bb25368

RUN apt update
RUN apt install -y openssh-server gcc

RUN useradd ctf

WORKDIR /home/ctf
ADD game .
RUN chown root:ctf /home/ctf
RUN chown root:ctf game
RUN chmod 111 game

ADD sshd_config /etc/ssh/sshd_config
RUN ssh-keygen -A
RUN mkdir -p /run/sshd
RUN echo 'ctf:password' | chpasswd

CMD ["/usr/sbin/sshd", "-D"]
