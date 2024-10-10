![Ocre logo](ocre_logo.jpg "Ocre")
# Ocre
Ocre is a container runtime for constrained devices. It leverages [WebAssembly](https://www.webassembly.org) and [Zephyr](https://www.zephyrproject.org/) to support OCI-type application containers in a footprint up to 2,000 times smaller than traditional Linux-based container runtimes. Our mission is to modernize the embedded applications by making it as easy to develop and securely deploy apps on constrained edge devices as it is in the cloud.


# Getting Started 
There is a sample application in `./src/main.c` that will demonstrate how to use the Ocre runtime. The sample writes a simple hello world application to flash and directs the Ocre runtime to load and execute it.

1. **Install Dependencies and Zephyr SDK**

Complete the [Install dependencies](https://docs.zephyrproject.org/latest/develop/getting_started/index.html#install-dependencies) and the [Install the Zephyr SDK](https://docs.zephyrproject.org/latest/develop/getting_started/index.html#install-the-zephyr-sdk) sections from the Zephyr [Getting Started Guide](https://docs.zephyrproject.org/latest/develop/getting_started/index.html#getting-started-guide). There are several steps that must be performed if this is the first time you’ve developed for Zephyr on your machine.
In general, builds should be done on a Linux machine or on Windows using WSL2.

2. **Install WEST**

Install the [west](https://docs.zephyrproject.org/latest/develop/west/index.html) CLI tool, which is needed to build, run and manage Zephyr applications. Additionally, install other pip packages required by West/Zephyr.

```
$ pip install west pyelftools
```

3. **Initialize the workspace**

This will checkout the project code and configure the Zephyr workspace.
```
$ mkdir runtime

$ cd runtime

$ west init -m git@github.com:project-ocre/ocre-runtime.git

$ west update
```

4. **Build the application**

The following will build the firmware for the *virtual*, `native_sim` target which will allow you to run the Ocre runtime on a simulated device, rather than a physical board.
```
$ west build -b native_sim ./application -d build -- -DMODULE_EXT_ROOT=`pwd`/application
```
5. **Run the application**

Run the following command:
```
$ ./build/zephyr/zephyr.exe
```

---

### Build and Flash for a Physical Device

To build and flash for a physical device, follow these steps:

1. Connect your board to your computer.

2. Build the application for your specific board. Replace `BOARD_NAME` with your board's name:
   ```
   $ west build -b BOARD_NAME ./application -d build -- -DMODULE_EXT_ROOT=`pwd`/application
   ```

3. Flash the application to your device:
   ```
   $ west flash
   ```

4. After flashing, restart/reset your board to run the application.


# Current Status

Project Ocre is a work in progress. Here is a list of what is currently implemented and planned:

- [X] Core container execution
- [X] Timer support
- [X] Container Supervisor state machine
- [ ] Ocre containerization image format
- [ ] Inter container messaging
- [ ] Networking APIs
- [ ] Multi-threading
- [ ] Support for WASI embedded APIs
