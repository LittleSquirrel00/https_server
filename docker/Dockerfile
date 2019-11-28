FROM ubuntu:latest
ARG USERNAME=ubuntu
ADD sources.list /etc/apt/
RUN apt-get update \
    && apt-get -y install --no-install-recommends apt-utils dialog 2>&1 \
    && apt-get -y install build-essential cmake openssl libssl-dev libevent-dev libhttp-parser-dev

EXPOSE 443
EXPOSE 8080
