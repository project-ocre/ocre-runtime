# mini sample

This sample will create `hello-world.wasm` from the state information directory (`/lfs/ocre/images/hellow-world.wasm` if it does not exist. Then it will execute this container, which prints a nice ASCII art.

It does not require the preloading of any other containers in the state information
directory because the hello-world.wasm container is hardcoded.

It is possible to customize what container is hardcoded and created. Check the [Zephyr build system](../BuildSystemZephyr.md) document for more information.

## Building and running

Instructions for building on the different platforms are provided below:

### Linux

Make sure you are using the [Devcontainer](../Devcontainers.md) or have the necessary
tools for Linux described in the [Get Started with Linux](../GetStartedLinux.md) guide.

In Linux, the mini sample is part of the main library build.

From the root of the repository, or from anywhere else, reate build directory:

```sh
mkdir build
cd build
```

Configure the Ocre Library build:

```sh
cmake ..
```

Make sure `..` points to the root of the ocre-runtime source tree.

Build ocre and the samples:

```sh
make
```

Run the mini sample:

```sh
./src/samples/mini/posix/ocre_mini
```

Note that this command should be run from the build directory
(i.e. the directory that contains `./src/ocre/var/lib/`).

### Zephyr

Make sure you are using the [Devcontainer](../Devcontainers.md) or have the necessary
tools for Linux described in the [Get Started with Zephyr](../GetStartedZephyr.md) guide.

You should have done at least the `west init` and `west update` commands to proceed.

Build the firmware image with:

```sh
west build -p always -b b_u585i_iot02a src/samples/mini/zephyr/
```

You can also replace `b_u585i_iot02a` with another board.

#### Flashing

```sh
west flash
```

If you monitor the UART or console from your Zephyr board, you should get the execution output.

## Customization

It is possible to customize the container executed by mini sample through the `OCRE_INPUT_FILE_NAME` variable.

For Zephyr:

```sh
west build -b <board> src/samples/mini/zephyr -- -DOCRE_INPUT_FILE_NAME=/absolute/path/to/my_container.wasm
```

For Linux:

```sh
cmake .. -DOCRE_INPUT_FILE_NAME=/absolute/path/to/my_container.wasm
```

Note that the path to the container file must be absolute.

And this container will be executed by the sample application.

Also, please note that if the container required additional runtime engine capabilities, they should be added
to the source code of this sample.

For more information about build customization, please refer to the [Linux Build System](../BuildSystemLinux.md) or [Zephyr Build System](../BuildSystemZephyr.md) documentation.
