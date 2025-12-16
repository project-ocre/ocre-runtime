FROM ubuntu:24.04 AS base

ARG USERNAME=ocre-dev
ARG USER_UID=1000
ARG USER_GID=$USER_UID

ARG NODE_MAJOR=20
ARG WASI_SDK_VERSION=29

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential \
    ca-certificates \
    ccache \
    clang \
    clang-format \
    clang-tools \
    cmake \
    cppcheck \
    curl \
    device-tree-compiler \
    dfu-util \
    file \
    gcc \
    git \
    gperf \
    gpg \
    jq \
    libmagic1 \
    libsdl2-dev \
    llvm \
    make \
    net-tools \
    ninja-build \
    python3-dev \
    python3-venv \
    python3-tk \
    wget \
    xz-utils \
    && curl -fsSL https://deb.nodesource.com/gpgkey/nodesource-repo.gpg.key | gpg --dearmor -o /etc/apt/keyrings/nodesource.gpg && \
    echo "deb [signed-by=/etc/apt/keyrings/nodesource.gpg] https://deb.nodesource.com/node_$NODE_MAJOR.x nodistro main" | tee /etc/apt/sources.list.d/nodesource.list && \
    apt-get update && apt-get install -y --no-install-recommends \
    nodejs \
    && apt-get clean -y\
    && update-alternatives --install /usr/bin/python python /usr/bin/python3 50

# Install wasi-sdk
RUN mkdir /opt/wasi-sdk && \
    curl -sSL https://github.com/WebAssembly/wasi-sdk/releases/download/wasi-sdk-$WASI_SDK_VERSION/wasi-sdk-$WASI_SDK_VERSION.0-$(uname -m | sed s/aarch64/arm64/)-linux.tar.gz | \
        tar zxvf - --strip-components=1 -C /opt/wasi-sdk

# Accomodate the user for devcontainer
RUN (userdel -r $USERNAME ; userdel -r `id -nu $USER_UID` ; groupdel `id -ng $USER_GID`) || true \
    && groupadd --gid $USER_GID $USERNAME \
    && useradd --uid $USER_UID --gid $USER_GID -m $USERNAME -s /bin/bash \
    && apt-get update \
    && apt-get install -y sudo \
    && echo $USERNAME ALL=\(root\) NOPASSWD:ALL > /etc/sudoers.d/$USERNAME \
    && chmod 0440 /etc/sudoers.d/$USERNAME

USER $USERNAME

WORKDIR /home/ocre-dev

RUN python -m venv .venv && \
    . .venv/bin/activate && \
    pip install \
        west \
        littlefs-python

# Install the SDK on a separate stage to avoid layer cluttering

FROM base AS zephyr-sdk

RUN mkdir -p /tmp/zephyrproject/ocre-runtime

COPY west.yml /tmp/zephyrproject/ocre-runtime/west.yml

RUN . .venv/bin/activate && \
    cd /tmp/zephyrproject && \
    west init -l ocre-runtime && \
    west update -n zephyr && \
    west packages pip --install && \
    west sdk install -t \
        x86_64-zephyr-elf \
        aarch64-zephyr-elf \
        arm-zephyr-eabi

FROM base AS ocre-dev

COPY --from=zephyr-sdk /home/ocre-dev /home/ocre-dev

RUN printf "\n%s\n" '. ~/.venv/bin/activate' >> /home/ocre-dev/.bashrc

WORKDIR /home/ocre-dev/zephyrproject
