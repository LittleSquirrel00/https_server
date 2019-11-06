FROM ubuntu:bionic

ENV DEBIAN_FRONTEND=noninteractive

ARG USERNAME=ubuntu
ARG USER_UID=1000
ARG USER_GID=$USER_UID

ADD sources.list /etc/apt/

RUN apt-get update \
    && apt-get -y install --no-install-recommends apt-utils dialog 2>&1 \
    && apt-get -y install git iproute2 procps lsb-release \
    && apt-get -y install build-essential cmake cppcheck valgrind openssl libssl-dev \
    && apt-get -y install pkg-config automake wget gawk libtool libsysfs-dev mininet \
    && apt-get -y install libevent-2.1-6 libevent-core-2.1-6 libevent-dev libevent-openssl-2.1-6 \
    # && cd ~ && git clone https://github.com/ppelletli/https-example.git \
    # && cd ~/https-example && ./autogen.sh && ./configure && make \
    && groupadd --gid $USER_GID $USERNAME \
    && useradd -s /bin/bash --uid $USER_UID --gid $USER_GID -m $USERNAME \
    && apt-get install -y sudo \
    && echo $USERNAME ALL=\(root\) NOPASSWD:ALL > /etc/sudoers.d/$USERNAME\
    && chmod 0440 /etc/sudoers.d/$USERNAME \
    && apt-get autoremove -y \
    && apt-get clean -y \
    && rm -rf /var/lib/apt/lists/*

ENV DEBIAN_FRONTEND=
