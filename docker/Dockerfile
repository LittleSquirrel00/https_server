FROM ubuntu
# ENV DEBIAN_FRONTEND=noninteractive
ARG USERNAME=ubuntu
# ARG USER_UID=1000
# ARG USER_GID=$USER_UID

ADD sources.list /etc/apt/
ADD server.tar.gz /root/

RUN apt-get update \
    && apt-get -y install --no-install-recommends apt-utils dialog 2>&1 \
    && apt-get -y install build-essential cmake openssl libssl-dev libevent-dev libhttp-parser-dev
    && cd /root/server/ && make && make cleanobj && rm cclean
    # && groupadd --gid $USER_GID $USERNAME \
    # && useradd -s /bin/bash --uid $USER_UID --gid $USER_GID -m $USERNAME \
    # && apt-get install -y sudo \
    # && echo $USERNAME ALL=\(root\) NOPASSWD:ALL > /etc/sudoers.d/$USERNAME\
    # && chmod 0440 /etc/sudoers.d/$USERNAME \
    # && apt-get autoremove -y \
    # && apt-get clean -y \
    # && rm -rf /var/lib/apt/lists/*

EXPOSE 443
EXPOSE 8080

CMD /root/server/start_server.sh

# ENV DEBIAN_FRONTEND=