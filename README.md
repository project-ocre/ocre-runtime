![Ocre logo](ocre_logo.jpg "Ocre")

# Ocre

[![Build](https://github.com/project-ocre/ocre-runtime/actions/workflows/build.yml/badge.svg)](https://github.com/project-ocre/ocre-runtime/actions/workflows/build.yml)
[![OpenSSF Best Practices](https://www.bestpractices.dev/projects/9691/badge)](https://www.bestpractices.dev/projects/9691)
[![License](https://img.shields.io/github/license/project-ocre/ocre-runtime?color=blue)](LICENSE)
[![slack](https://img.shields.io/badge/slack-ocre-brightgreen.svg?logo=slack)](https://lfedge.slack.com/archives/C07F190CC3X)
[![Stars](https://img.shields.io/github/stars/project-ocre/ocre-runtime?style=social)](Stars)

Powered by WebAssembly, the Ocre runtime is available in both Zephyr and Linux variants and supports OCI-like application containers in a footprint up to 2000x lighter than traditional container runtimes like Docker.

With Ocre, developers can run the exact same application container binaries written in choice of programming language on both the Linux and Zephyr-based runtime versions spanning CPU and MCU-based devices.

Our mission is to make it as easy to develop and securely deploy apps for the billions of embedded devices in the physical world as it is in the cloud.

---

## Features & Platform Support

Ocre supports a range of features depending on the platform. The table below summarizes the current support:

| Feature             | Zephyr (native_sim, b_u585i_iot02a) | Linux |
| ------------------- | :---------------------------------: | :---: |
| Ocre Runtime        |                 ✅                  |  ✅   |
| Container Messaging |                 ✅                  |  ✅   |
| GPIO                |                 ✅                  |  ❌   |
| Timers              |                 ✅                  |  ✅   |
| Sensors             |                 ✅                  |  ❌   |
| Networking          |                 ✅                  |  ✅   |
| Filesystem          |                 ✅                  |  ✅   |
| Interactive Shell   |                 ✅                  |  ❌   |

- **Zephyr**: Full feature set, including hardware integration and shell.
- **Linux**: Core runtime and multiple I/O features; hardware and shell features are not available.

---

# Getting Started

## Zephyr

### Devcontainer

We provide an optional devcontainer for building Zephyr with Ocre. However, we suggest Zephyr development to be done in the host outside docker, because it will be easier to flash and debug the board, the devcontainer is provided as a quick and handy way of reproducing the environment we have in the CI.

### Zephyr Integration

Ocre is a collection of a External Zephyr module, a few sample applications and a an optional slim West project for building for a small set of platforms.

More boards are supported in Ocre with a full West Project, however, our optional West project can be used to get started fast and in our CI.

### Quick Zephyr build with our West project

This will very quickly build the images for one of the officially supported boards. Open a terminal in Ubuntu 22.04 or newer and run:

Install Ubuntu dependencies:

```sh
sudo apt update
sudo apt upgrade
sudo apt install --no-install-recommends git cmake ninja-build gperf \
  ccache dfu-util device-tree-compiler wget python3-dev python3-venv python3-tk \
  xz-utils file make gcc gcc-multilib g++-multilib libsdl2-dev libmagic1 curl
```

Install [WASI-SDK](https://github.com/WebAssembly/wasi-sdk/releases):

```sh
mkdir /opt/wasi-sdk && \
    curl -sSL https://github.com/WebAssembly/wasi-sdk/releases/download/wasi-sdk-29/wasi-sdk-29.0-$(uname -m | sed s/aarch64/arm64/)-linux.tar.gz | \
        sudo tar zxvf - --strip-components=1 -C /opt/wasi-sdk
```

Create Zephyr project directory, venv and install python dependencies:

```sh
mkdir ~/zephyrproject
python3 -m venv ~/zephyrproject/.venv
source ~/zephyrproject/.venv/bin/activate
pip install littlefs-python
```

Zephyr workspace initialization:

```sh
cd ~/zephyrproject
west init -m https://github.com/project-ocre/ocre-runtime.git
west update
west zephyr-export
```

Install SDK and Python packages:

```sh
west packages pip --install
west sdk install -t \
    x86_64-zephyr-elf \
    aarch64-zephyr-elf \
    arm-zephyr-eabi
```

Build the sample application:

```sh
west build -p always -b b_u585i_iot02a ocre-runtime/src/samples/demo/zephyr
```

Connect the board and flash the application:

```sh
west flash
```

Alternatively, you can try the `mini` or `supervisor` samples.

### Full Zephyr build from scratch

These instructions work natively for Linux, MacOS and WSL2. If this is used in docker, it might require some special configuration for being able to flash or debug the board.

1. Follow the instructions on the [Zephyr Getting Started Guide](https://docs.zephyrproject.org/latest/develop/getting_started/index.html) for your OS, until you can build a sample application from Zephyr.

2. Install `littlefs-python`:

   ```sh
   pip install littlefs-python
   ```

3. Install [WASI-SDK](https://github.com/WebAssembly/wasi-sdk/releases)

WASI-SDK is expected to be installed in `/opt/wasi-sdk`.

Check-out the ocre-runtime repository alongside the zephyr repositories or anywhere and just build one of our sample applications like:

```sh
west build -p always -b <board> ocre-runtime/src/samples/<sample>/zephyr
```

See below for the available applications and officially supported boards.

You can also change the Kconfig options for Zephyr and ocre with:

```sh
west build -t menuconfig
```

Followed by:

```sh
west build
west flash
```

### Quick Zephyr build with devcontainer

This will very quickly build the images for one of the officially supported boards. The devcontainer already includes the Zephyr SDK, WASI-SDK the Python venv and all the necessary tools.

Build and open the 'zephyr' devcontainer of this repository.

Open a terminal in the devcontainer and run:

```sh
west init .
west update
west build -p always -b <board> ocre-runtime/src/samples/<sample>/zephyr
```

The build artifacts are in the `build/zephyr` directory. You might need to set-up some docker mounts and permissions for flashing the board.

See below for the available applications and officially supported boards.

### Official boards and sample applications

The officially supported boards are listed below:

| Board                    | Name                                                                                            |
| ------------------------ | ----------------------------------------------------------------------------------------------- |
| `native_sim`             | 32-bit native simulator                                                                         |
| `native_sim/native/64`   | 64-bit native simulator                                                                         |
| `pico_plus2/rp2350b/m33` | Pimoroni Pico Plus 2                                                                            |
| `b_u585i_iot02a`         | [B-U585I-IOT02A](https://docs.zephyrproject.org/latest/boards/st/b_u585i_iot02a/doc/index.html) |

Other boards might work. Check the [Zephyr documentation](https://docs.zephyrproject.org/latest/boards/index.html) for a list of supported boards in Zephyr.

The officially supported sample applications are listed below:

| Sample       | Description                                                |
| ------------ | ---------------------------------------------------------- |
| `mini`       | A simple "Hello World" application using minimal resources |
| `demo`       | A more featured sample application                         |
| `supervisor` | Interactive shell control                                  |

The intended usage of the samples are:

- Use the `mini` sample to see how Ocre can work on your board.
- Use the `demo` sample to test some more Ocre features.
- Use the `supervisor` sample to have full interactive control from the shell command line.

## Linux

### Devcontainer

We also provide a devcontainer for Linux. This is the recommended way to get started with Ocre on Linux. However these instructions will work for native Linux builds.

### Prerequisites

- Ubuntu 22.04 or later
- CMake > 3.20
- git > 2.28
- A working C compiler (gcc or clang)
- [WASI-SDK](https://github.com/WebAssembly/wasi-sdk/releases) installed under `/opt/wasi-sdk`

Other Linux distributions might work, but are not actively tested.

You can get this easily on a new Ubuntu by running:

```sh
sudo apt update
sudo apt install -y cmake git build-essential
```

Or you can use the devcontainer, which already has everything preinstalled.

Clone the repository and the submodules:

```sh
git clone https://github.com/project-ocre/ocre-runtime.git
cd ocre-runtime
git submodule update --init --recursive
```

Run the CMake build, like:

```sh
mkdir build && cd build
cmake ..
make
```

The relevant binaries are stored in `build/src/samples/mini/posix` and `build/src/samples/demo/posix`. You can run them directly:

```sh
./build/src/samples/mini/posix/mini
```

## License

Distributed under the Apache License 2.0. See [LICENSE](https://github.com/project-ocre/ocre-runtime/blob/main/LICENSE) for more information.

---

## More Info

- **[Website](https://lfedge.org/projects/ocre/)**: For a high-level overview of the Ocre project, and its goals, visit our website.
- **[Docs](https://docs.project-ocre.org/)**: For more detailed information about the Ocre runtime, visit Ocre docs.
- **[Wiki](https://lf-edge.atlassian.net/wiki/spaces/OCRE/overview?homepageId=14909442)**: For a full project overview, FAQs, project roadmap, and governance information, visit the Ocre Wiki.
- **[Slack](https://lfedge.slack.com/archives/C07F190CC3X)**: If you need support or simply want to discuss all things Ocre, head on over to our Slack channel (`#ocre`) on LFEdge's Slack org.
- **[Mailing list](https://lists.lfedge.org/g/Ocre-TSC)**: Join the Ocre Technical Steering Committee (TSC) mailing list to stay up to date with important project announcements, discussions, and meetings.
