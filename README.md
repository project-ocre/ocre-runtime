![Project Ocre logo](ocre_logo.jpg "Project Ocre")
# Project Ocre
Ocre is a container runtime for constrained devices.  It leverages WebAssembly and Zephyr to support OCI-type application containers in a footprint up to 2,000 times smaller than traditional Linux-based container runtimes. Our mission is to modernize the embedded applications by making it as easy to develop and securely deploy apps on constrained edge devices as it is in the cloud.


# Getting Started 
There is a sample application in `./src/main.c` that will demonstrate how to use Project Ocre.  The sample writes a simple hello world application to flash and directs the Ocre runtime to load and execute it.  

1. **Install Zephyr SDK and WEST** - Follow the [Zephyr Getting Started Guide](https://docs.zephyrproject.org/latest/develop/getting_started/index.html) to install tools and prerequisites. There are several steps that must be performed if this is the first time you’ve developed for Zephyr on your machine.  In general, builds should be done on a Linux machine or on Windows using WSL2.
2. **Initialize the workspace** - This will checkout the project code and configure the Zephyr workspace.  This is done as follows:
```
> mkdir runtime

> cd runtime

> west init -m git@github.com:project-ocre/ocre-runtime.git

> west update
```

3. **Build the application** - The build follows the normal Zephyr build process.  The following will build the firmware for the `native_sim` target which will allow you to run Project Ocre on a Linux-based host.  
```
> west build -b native_sim ./application -d build -- -DMODULE_EXT_ROOT=`pwd`/application
```
4.  **Flash the application to your device** - If you are using a real board, flash the application to your device using `west flash` or however you normally flash your device.
5.  **Run the application** - If using a real device, restart/reset your board to run the application.  If using the simulator, run the following command:
```
> ./build/zephyr/zephyr.exe
```

# Current Status
Project Ocre is a work in progress.  Here is a list of what is currently implemented and planned:
- [X] Core container execution
- [X] Timer support
- [X] Container Supervisor state machine
- [ ] Ocre containerization image format
- [ ] Inter container messaging
- [ ] Networking APIs
- [ ] Multi-threading
- [ ] Support for WASI embedded APIs
