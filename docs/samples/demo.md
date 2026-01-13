# demo sample

The demo sample will run a few scenarios in a sequential demonstration.

Currently it will run the following scenarios in order:

1. `hello-world.wasm`
   - Prints a nice ASCII art

2. `blinky.wasm` runs for 2 seconds
   - Prints some logs to stdout

3. `subscriber.wasm` and `publisher.wasm` run alongside for 4 seconds
   - Exchange messages between the containers

It requires the [state information](../StateInformation.md) directory
to load the images from.

## Building and running

Instructions for building on the different platforms are provided below:

### Linux

Make sure you are using the [Devcontainer](../Devcontainers.md) or have the necessary
tools for Linux described in the [Get Started with Linux](../GetStartedLinux.md) guide.

In Linux, the demo sample is part of the main library build.

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

Run the demo sample:

```sh
./src/samples/demo/posix/ocre_demo
```

Note that this command should be run from the build directory
(i.e. the directory that contains files in the relative path `./src/ocre/var/lib/ocre/images/`).

### Zephyr

Make sure you are using the [Devcontainer](../Devcontainers.md) or have the necessary
tools for Linux described in the [Get Started with Zephyr](../GetStartedZephyr.md) guide.

You should have done at least the `west init` and `west update` commands to proceed.

Build the firmware image with:

```sh
west build -p always -b b_u585i_iot02a src/samples/demo/zephyr/
```

You can also replace `b_u585i_iot02a` with another board.

#### Flashing

This sample will load the container images from the state information directory,
which in Zephyr is `/lfs/ocre` by default. This directory should have a `images`
subdirectory preloaded with the required images.

If we just use `west flash` here, it will flash the main app partition, but if the
images are not in the filesystem, the demo will fail.

As part of the build artifacts, on supported platforms, is `merged.hex` which is
a hex file providing the application and the storage_partition with the state information data with the required preloaded images for the demo.

To flash this hex, use a command like:

```sh
west flash --hex-file build/zephyr/merged.hex
```

Note that the `--hex-file` option might be different depending on your runner,
and the `merged.hex` file might not have been built for your board because of
missing configuration.
Check the [Zephyr build system](../BuildSystemZephyr.md) document for more information.

Depending on the size, it might take a long time to flash it.
For this reason we recommend doing this the first time, so it will preload the images,
and for subsequent development and flashing of Ocre, we can then just do:

```sh
west flash
```

which should not erase the storage partition.

If you monitor the UART or console from your Zephyr board, you should get the execution output.

If it is not possible to flash the storage_partition with west, as a workaround,
it is possible to use the [Supervisor] with `ocre pull` to populate the images
directory, and then flash just this demo with the command above.
