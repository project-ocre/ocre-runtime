# Get Started with Zephyr

The samples we currently provide for Zephyr are the following. Check their specific instruction for
detailed explanation for each sample:

- [`src/samples/mini/zephyr`](samples/mini.md)
- [`src/samples/demo/zephyr`](samples/demo.md)
- [`src/samples/supervisor/zephyr`](samples/supervisor.md)

Refer to the [Zephyr build system](BuildSystemZephyr.md) documentation for more information.

## Set up instructions

### Devcontainer

The devcontainer can be build and started in VS Code (using the devcontainer extension) or using the devcontainer-cli and requires a working docker environment. Please refer to the [devcontainer section](Devcontainers.md) for information on how to set up the devcontainer.

### Native host development setup

The Zephyr build can be done in two ways. We provide a west project with the minimal amount of Zephyr modules,
targeting our standard boards; and a more traditional Zephyr-centric development, where the whole Zephyr modules and toolchains are installed. These options are described below.

#### Ocre West project

Use this if you are developing for Ocre and do not care too much about Zephyr development or adding new boards.
The Ocre West project is minimalistic and focuses on a few boards, and includes only the necessary Zephyr
modules. If you are looking for developing Zephyr or other custom applications, modules, or boards, you should
set up a standard Zephyr West project, described below.

Zephyr and its modules will be checked alongside ocre-runtime repository. In the devcontainer, this is
`/workspace/zephyr`, `/workspace/modules` and so on. In the host, we recommend ocre-runtime to be checked-out in a subdirectory of a workspace, for example:

```sh
mkdir ~/zephyrproject
cd ~/zephyrproject
git clone --recurse-submodules https://github.com/project-ocre/ocre-runtime.git
```

If you open the repository in devcontainer, you have already cloned the repository and can continue with the instructions.

If you need, make sure you have all the submodules checked out:

```sh
git submodule update --init --recursive
```

Initialize Zephyr build using Ocre West Project Manifest. From the ocre repository root:

```sh
west init -l .
west update
```

Proceed to the general instructions for Zephyr development below.

#### Ocre Zephyr module

Use this if you need to add new boards, or need to extensively change the Zephyr build configuration.
This is the standard recommended point of integration with Zephyr. All the modules and boards support
will be checked out.

The base instructions are based on [Getting Started Guide](https://docs.zephyrproject.org/latest/getting_started/index.html).

```sh
mkdir ~/zephyrproject
cd ~/zephyrproject
west init
west update
```

Proceed to the general instructions for Zephyr development below.

## General instructions

Build a sample:

```sh
west build -p always -b native_sim/native/64 src/samples/supervisor/zephyr
```

Make sure you are in a directory inside the Zephyr workspace and the path to the Ocre sample is correct.
This will create a `build` directory in the current directory. It is possible to use multiple different build
directories for each sample, just by switching to a different directory.

You can replace `native_sim/native/64` with any other supported board.
Please refer to the board specific instructions for more details:

- `native_sim`
- `pico_plus2/rp2350b/m33`
- `b_u585i_iot02a`

You can replace `src/samples/supervisor` with any other supported sample.
Please refer to the sample specific instructions for more details:

- `src/samples/mini/zephyr`
- `src/samples/demo/zephyr`
- `src/samples/supervisor/zephyr`

For the `native_sim` targets, run the sample as:

```sh
west build -t run
```

For a physical target, you need to flash the image to the board:

```sh
west flash
```

If you flashed the board, monitor the serial port and check the output. Please refer to the zephyr documentation for more details:

- [Zephyr Documentation](https://docs.zephyrproject.org/latest/index.html)
