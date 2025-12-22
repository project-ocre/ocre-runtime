FROM ubuntu:24.04 AS ocre-ci

ARG USERNAME=ocre-dev
ARG USER_UID=1000
ARG USER_GID=$USER_UID

ARG NODE_MAJOR=20
ARG WASI_SDK_VERSION=29

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y --no-install-recommends \
    ca-certificates \
    curl \
    gcc \
    git \
    gpg \
    make \
    build-essential \
    cmake \
    cppcheck \
    clang \
    clang-format \
    clang-tools \
    libclang-rt-dev \
    llvm \
    net-tools \
    sudo \
    jq \
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

FROM ocre-ci AS ocre-dev

# Accomodate the user for devcontainer
RUN (userdel -r $USERNAME ; userdel -r `id -nu $USER_UID` ; groupdel `id -ng $USER_GID`) || true \
    && groupadd --gid $USER_GID $USERNAME \
    && useradd --uid $USER_UID --gid $USER_GID -m $USERNAME -s /bin/bash \
    && echo $USERNAME ALL=\(root\) NOPASSWD:ALL > /etc/sudoers.d/$USERNAME \
    && chmod 0440 /etc/sudoers.d/$USERNAME

USER $USERNAME

WORKDIR /home/$USERNAME
