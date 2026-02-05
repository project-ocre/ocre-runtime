# Set up host to build Ocre

This document provides instructions on how to build the Ocre Runtime from source without requiring Docker. These instructions also work inside a docker container, however this is provided as a reference for whose who need the development enviroment available in the host.

For quick instructions on how to build the Ocre Runtime (potentially using docker), see [Devcontainers](Devcontainers.md), [Getting Started with Linux](GetStartedLinux.md) and [Getting Started with Zephyr](GetStartedZephyr.md) documents.

We detail the procedure to set up the host system prerequisites, and build a native Linux version of the Ocre Runtime, and also how to set up a Zephyr development environment locally, which includes Ocre.

## General Prerequisites

These prerequisites are necessary both for Linux and Zephyr build.

Our reference system is a Ubuntu 24.04 LTS, however these instructions can be easily adapted for other Linux distributions, as well as other POSIX based operating systems.

### Build Tools

Ocre requires CMake and a C compiler such as GCC. For full testing support, we also use clang.
If we are checking out the Ocre repository, we also need Git to clone Ocre Runtime, and Wget to unpack the WASI-SDK. Install all with:

```sh
sudo apt update -y
sudo apt install -y apt install build-essential git cmake clang wget
```

### WASI-SDK

For building WASM/WASI containers (including the test containers), we need the WASI-SDK.
Download a WASI-P1 compatible WASI-SDK from [the WASI SDK releases page](https://github.com/WebAssembly/wasi-sdk/releases) for your platform. The current recommended version is `wasi-sdk-29`.

This is usually installed under `/opt/wasi-sdk` for default easy usage without the need to set environment variables.

Download, unpack and install WASI SDK. Note that these are Linux binary packages. Make sure you replace `amd_64` with `arm64` in case your host is ARM64 based:

```sh
wget https://github.com/WebAssembly/wasi-sdk/releases/download/wasi-sdk-29/wasi-sdk-29.0-amd_64-linux.tar.gz
tar -xzf wasi-sdk-29.0-amd_64-linux.tar.gz
sudo mv wasi-sdk-29.0-amd_64-linux /opt/wasi-sdk
```

You can check that WASI SDK is properly installed by checking the WASI clang version:

```sh
/opt/wasi-sdk/bin/clang --version
```

The output should be similar to:

```
clang version 21.1.4-wasi-sdk (https://github.com/llvm/llvm-project 222fc11f2b8f25f6a0f4976272ef1bb7bf49521d)
Target: wasm32-unknown-wasi
Thread model: posix
InstalledDir: /opt/wasi-sdk/bin
Configuration file: /opt/wasi-sdk/bin/clang.cfg
```

## Linux

This environment is enough to build and run Ocre on Linux:

```sh
git clone --recursive https://github.com/project-ocre/ocre-runtime.git
cd ocre-runtime
mkdir build
cd build
cmake ..
make
```

Check the [Getting Started with Linux](GetStartedLinux.md) and [Linux Build System](BuildSystemLinux.md) for more information.

## Zephyr

As stated in the Zephyr [Getting Started Guide](https://docs.zephyrproject.org/latest/getting_started/index.html),
Zephyr requires additional tools such as `python3` among others. We can install them with:

```sh
sudo apt update -y
sudo apt install --no-install-recommends git cmake ninja-build gperf \
  ccache dfu-util device-tree-compiler wget python3-dev python3-venv python3-tk \
  xz-utils file make gcc gcc-multilib g++-multilib libsdl2-dev libmagic1
```

Then we need `west`. This is usually done in the venv because we might need some more Python packages and we do not want to tamper with the system Python packages (which are outside of the virtual environment).

Create a virtual environment:

```sh
python3 -m venv .venv
source venv/bin/activate
```

Install `west`:

```sh
pip install west
```

Now you can proceed to build your Zephyr application as detailed in Zephyr's [Getting Started Guide](https://docs.zephyrproject.org/latest/getting_started/index.html).

Alternatively you can check [Get Started with Zephyr](GetStartedZephyr.md) and [Custom Zephyr Application](CustomZephyrApplication.md) to build a custom application or the Ocre samples.

You can also check the [Zephyr Build System](BuildSystemZephyr.md) for more information about the build configuration.
