FROM debian:bookworm

RUN apt-get update 
RUN apt-get upgrade -y
RUN apt-get install -y nasm
RUN apt-get install -y xorriso
RUN apt-get install -y grub-common
RUN apt-cache search grub-pc



VOLUME /root/env
WORKDIR /root/env
