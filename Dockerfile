FROM ubuntu:22.04

# for apt to be noninteractive
ENV DEBIAN_FRONTEND=noninteractive
ENV DEBCONF_NONINTERACTIVE_SEEN=true

RUN truncate -s0 /tmp/preseed.cfg; \
    echo "tzdata tzdata/Areas select Europe" >> /tmp/preseed.cfg; \
    echo "tzdata tzdata/Zones/Europe select Madrid" >> /tmp/preseed.cfg; \
    debconf-set-selections /tmp/preseed.cfg && \
    rm -f /etc/timezone /etc/localtime

RUN apt-get -q update && apt-get install --no-install-recommends -y\
    build-essential\
    ccache\
    cmake\
    device-tree-compiler\
    dfu-util\
    file\
    gcc\
    g++\
    git\
    gperf\
    libmagic1\
    locales\
    make\
    ninja-build\
    openssh-server\
    python3\
    python3-dev\
    python3-venv\
    sudo\
    supervisor\
    tzdata\
    wget\
    xz-utils

RUN /usr/sbin/locale-gen en_US.UTF-8
ENV LANG=en_US.UTF-8

RUN mkdir -p /var/run/sshd /var/log/supervisor

#COPY supervisord.conf /etc/supervisor/conf.d/supervisord.conf

RUN cat << EOF > /etc/supervisor/conf.d/supervisord.conf
[supervisord]
nodaemon=true
user=root

[program:sshd]
command=/usr/sbin/sshd -D
autostart=true
autorestart=true
EOF

EXPOSE 22
CMD ["/usr/bin/supervisord", "-c", "/etc/supervisor/conf.d/supervisord.conf"]

# create users
RUN echo "[color]\n\tui = false\n[user]\n\tname = Marco\n\temail = marco.casaroli@gmail.com" > /etc/skel/.gitconfig

ARG USER_NAME=marco

# ===== create user/setup environment =====
# this is for ubuntu user (uid=1000, gid=1000)
RUN useradd --create-home --shell /bin/bash --groups sudo ${USER_NAME}
RUN echo "${USER_NAME} ALL=(ALL) NOPASSWD: ALL" > /etc/sudoers.d/${USER_NAME}

RUN \
    mkdir -p /home/${USER_NAME}/.ssh/ &&\
    echo "ssh-ed25519 AAAAC3NzaC1lZDI1NTE5AAAAIHOQDCuzFBPW2VeYBGhcmJgoecMM99Uea/C4gExUqZPn marco@Marcos-Silver-MacBook-Pro.local" > /home/${USER_NAME}/.ssh/authorized_keys &&\
    chown -R ${USER_NAME}:${USER_NAME} /home/${USER_NAME}/.ssh

# use bash as shell
SHELL ["/bin/bash", "-c"]

# working directory
#USER ${USER_NAME}
#WORKDIR /home/${USER_NAME}
