![Ocre logo](ocre_logo.jpg "Ocre")
# Ocre
Ocre is a container runtime for constrained devices. It leverages [WebAssembly](https://www.webassembly.org) and [Zephyr](https://www.zephyrproject.org/) to support OCI-type application containers in a footprint up to 2,000 times smaller than traditional Linux-based container runtimes. Our mission is to modernize the embedded applications by making it as easy to develop and securely deploy apps on constrained edge devices as it is in the cloud.


# Getting Started 
There is a sample application in `./src/main.c` that will demonstrate how to use the Ocre runtime. The sample writes a simple hello world application to flash and directs the Ocre runtime to load and execute it.

1. **Install Dependencies and Zephyr SDK**

Complete the [Install dependencies](https://docs.zephyrproject.org/latest/develop/getting_started/index.html#install-dependencies) and the [Install the Zephyr SDK](https://docs.zephyrproject.org/latest/develop/getting_started/index.html#install-the-zephyr-sdk) sections from the Zephyr [Getting Started Guide](https://docs.zephyrproject.org/latest/develop/getting_started/index.html#getting-started-guide). There are several steps that must be performed if this is the first time you’ve developed for Zephyr on your machine.
In general, builds should be done on a Linux machine or on Windows using WSL2.

**Note:** For the following steps we recommend using a Python virutual environment like [venv](https://docs.python.org/3/library/venv.html).

2. **Install WEST**

Install the [west](https://docs.zephyrproject.org/latest/develop/west/index.html) CLI tool, which is needed to build, run and manage Zephyr applications.

```
$ pip install west
```

3. **Initialize the workspace**

This will checkout the project code and configure the Zephyr workspace.
```
$ cd

$ mkdir runtime

$ cd runtime

$ west init -m git@github.com:project-ocre/ocre-runtime.git

$ west update
```

4. **Install Additional Zephyr (pip) requirements**

In order to build the Ocre runtime properly, you'll need to install a few remaining requirements for Zephyr.

```
pip install -r zephyr/scripts/requirements.txt
```

**Note:** This step is only possible after updating the repo with `west update`.

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

## More Info
* **[Website](https://lfedge.org/projects/ocre/)**: For a high-level overview of the Ocre project, and its goals visit our website.
* **[Docs](https://docs.project-ocre.org/)**: For more detailed information about Ocre, visit Ocre docs.
* **[Wiki](https://lf-edge.atlassian.net/wiki/spaces/OCRE/overview?homepageId=14909442)**: For a full project overview, FAQs, project roadmap, and governance information, visit the Ocre Wiki.
* **[Slack](https://lfedge.slack.com/archives/C07F190CC3X)**: If you need support, or simply want to discuss all things Ocre head on over to our Slack channel (`#ocre`) on LFEdge's Slack org.