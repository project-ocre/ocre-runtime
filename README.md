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

| Feature                | Zephyr (native_sim, b_u585i_iot02a) | Linux x86_64  |
|------------------------|:-----------------------------------:|:-------------:|
| Ocre Runtime           | ✅                                  | ✅           |
| Container Messaging    | ✅                                  | ❌           |
| GPIO                   | ✅                                  | ❌           |
| Timers                 | ✅                                  | ❌           |
| Sensors                | ✅                                  | ❌           |
| Networking             | ❌                                  | ✅           |
| Interactive Shell      | ✅                                  | ❌           |

- **Zephyr**: Full feature set, including hardware integration and shell.
- **Linux x86_64**: Core runtime and networking only; hardware and shell features are not available.

---

## Getting Started

This guide will help you build and run Ocre on both Zephyr (simulated and hardware) and Linux.

### Prerequisites

- **Zephyr SDK** (for Zephyr targets)
- **Python 3** (for build tooling)
- **CMake** and **make** (for Linux builds)
- **west** (Zephyr meta-tool)
- **git**

---

### Setup

#### Zephyr

Follow the [Zephyr Getting Started Guide](https://docs.zephyrproject.org/3.7.0/develop/getting_started/index.html) for your OS, including:
- [Install dependencies](https://docs.zephyrproject.org/3.7.0/develop/getting_started/index.html#install-dependencies)
- [Install the Zephyr SDK](https://docs.zephyrproject.org/3.7.0/develop/getting_started/index.html#install-the-zephyr-sdk)

#### Python Virtual Environment (Recommended)

```sh
mkdir runtime && cd runtime
python3 -m venv .venv
source .venv/bin/activate
```
You may need to install `python3-venv` on your system.

#### Install West

Install the [west](https://docs.zephyrproject.org/latest/develop/west/index.html) CLI tool, which is needed to build, run and manage Zephyr applications.

```sh
pip install west
```

#### Initialize and Update Zephyr Workspace

```sh
west init -m git@github.com:project-ocre/ocre-runtime.git
west update
```

#### Install Zephyr Python Requirements

```sh
pip install -r zephyr/scripts/requirements.txt
```

---

#### Linux

#### Clone the Repository

```sh
git clone git@github.com:project-ocre/ocre-runtime.git
cd ocre-runtime
```

#### Initialize submodules
```sh
git submodule update --init --recursive
```

---

## Building and Running

### Using the `build.sh` Script (Recommended)

Ocre provides a convenient `build.sh` script to simplify building and running for both Zephyr and Linux targets.

#### Usage

```sh
./build.sh -t <target> [-r] [-f <file1> [file2 ...]] [-b] [-h]
```

- `-t <target>`: **Required**. `z` for Zephyr, `l` for Linux.
- `-r`: Run after build (optional).
- `-f <file(s)>`: Specify one or more input files (optional).
- `-b`: (Zephyr only) Build for `b_u585i_iot02a` board instead of `native_sim`.
- `-h`: Show help.

#### Examples

**Build and run for Zephyr native_sim:**
```sh
./build.sh -t z -r
```

**Build for Zephyr b_u585i_iot02a board:**
```sh
./build.sh -t z -b
```

**Build and run for Linux with a WASM file:**
```sh
./build.sh -t l -r -f your_file.wasm
```

---

### Manual Build Steps

#### Zephyr (native_sim)

```sh
west build -b native_sim ./application -d build -- -DMODULE_EXT_ROOT=`pwd`/application
./build/zephyr/zephyr.exe
```

#### Zephyr (b_u585i_iot02a)

```sh
west build -b b_u585i_iot02a ./application -d build -- -DMODULE_EXT_ROOT=`pwd`/application
# Flash to board (requires hardware)
west flash
```

#### Linux x86_64

```sh
mkdir -p build
cd build
cmake .. -DTARGET_PLATFORM_NAME=Linux
make
./app your_file.wasm
```

---

## Additional Notes

- The `build.sh` script will automatically detect the target and handle build/run steps, including passing input files and selecting the correct Zephyr board.
- For Zephyr, only the first file specified with `-f` is passed as the input file (see script for details).
- For Linux, you can pass multiple WASM files as arguments to the built application.
- The script checks build success and only runs the application if the build completes successfully.
- The `mini-samples` directory contains a "Hello World" container, which is hardcoded as a C array. When Ocre is run without input file arguments, it executes this sample container by default. If an input file is provided, Zephyr will convert that file into a C array at build time and run it as a container. On Linux, this conversion does not occur — instead, Ocre simply opens and reads the provided file(s) directly from the filesystem.

---

## License

Distributed under the Apache License 2.0. See [LICENSE](https://github.com/project-ocre/ocre-runtime/blob/main/LICENSE) for more information.

---

## More Info
* **[Website](https://lfedge.org/projects/ocre/)**: For a high-level overview of the Ocre project, and its goals, visit our website.
* **[Docs](https://docs.project-ocre.org/)**: For more detailed information about the Ocre runtime, visit Ocre docs.
* **[Wiki](https://lf-edge.atlassian.net/wiki/spaces/OCRE/overview?homepageId=14909442)**: For a full project overview, FAQs, project roadmap, and governance information, visit the Ocre Wiki.
* **[Slack](https://lfedge.slack.com/archives/C07F190CC3X)**: If you need support or simply want to discuss all things Ocre, head on over to our Slack channel (`#ocre`) on LFEdge's Slack org.
* **[Mailing list](https://lists.lfedge.org/g/Ocre-TSC)**: Join the Ocre Technical Steering Committee (TSC) mailing list to stay up to date with important project announcements, discussions, and meetings.
