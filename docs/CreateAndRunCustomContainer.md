# Create and Run Custom Container

This guide will walk you through the process of creating and running a custom Ocre container on your board.

We will build the container using ocre-sdk and then use ocre-runtime to deploy it to the board and run it.

## Set up

The [Devcontainer](Devcontainer.md) provides the easiest and most convenient way to set up your development environment.

It is also possible to configure the build environment as described in [Get started with Linux](GetStartedLinux.md) and [Get started with Zephyr](GetStartedZephyr.md).

Make sure you have a working build environment both for WASM (based on Ocre SDK) and for Ocre Runtime.

## Building the container

If you have checked out the `ocre-runtime` repository, `ocre-sdk` is available as a Git submodule in the root. Make sure it is initialized and checked out:

```sh
git submodule update --init ocre-sdk
```

In fact, you might just want to make sure all submodules are initialized and checked out:

```sh
git submodule update --init --recursive
```

Using `ocre-sdk`, we can build a custom container. Please refer to the Ocre SDK documentation for more details.

For this guide, we will just use the `filesystem` sample from ocre-sdk:

```sh
cd ocre-sdk/generic/filesystem
mkdir build
cmake ..
make
```

This should generate the file `filesystem.wasm` in the current directory. We will call this the "container build directory".

## Deployment to the board

There are at least two easy ways of having the container image available for execution on the board.

We can provide it as a preloaded image, which gets flashed with the board firmware (and Ocre runtime), or we can use `ocre image pull` to retrieve from a web server.

These methods are described in the following sections.

### Preloaded images

When we build Ocre Runtime [Supervisor sample](samples/supervisor.md), we can provide extra images to be preloaded in the `merged.hex` binary on supported boards, using the `OCRE_PRELOADED_IMAGES` cmake variable.

For example, when we build the supervisor sample for Zephyr, we can do (from ocre-runtime root):

```sh
west build -p always -b b_u585i_iot02a src/samples/supervisor/zephyr/ -- -DOCRE_PRELOADED_IMAGES=${PWD}/ocre-sdk/generic/filesystem/build/filesystem.wasm
```

Make sure to flash the `merged.hex` to the board, which includes the storage partition:

```sh
west flash --hex-file=build/zephyr/merged.hex
```

For Linux build we can add custom images also, from `ocre-runtime` repository root:

```sh
mkdir build
cd build
cmake .. -DOCRE_PRELOADED_IMAGES=${PWD}/ocre-sdk/generic/filesystem/build/filesystem.wasm
make
```

And this will build the supervisor sample with the custom filesystem container preloaded.

### ocre image pull

If we already have the supervisor running on the board, connected to a network, we can run a web server on the host, and use Ocre to pull the image into the board.

To do this, in the "container build directory" (i.e. the directory which contains `filesystem.wasm`), run:

```sh
python3 -m http.server 8000
```

This will start a web server on port 8000. Please take a moment to make sure there is no firewall blocking the port, and that the server is accessible from the board.

Also, find the server IP in the local network configuration.

Then, on the board, run:

```sh
ocre image pull http://<host-ip>:8000/filesystem.wasm
```

### Running the container

We can check that the image is available on the board by running:

```
ocre image ls
```

And we can start the container. Note that the `filesystem.wasm` sample container from Ocre SDK requires the `filesystem` runtime capability. Make sure you enable any other capabilities your container might require:

```
ocre run -k filesystem filesystem.wasm
```

Check the [Supervisor sample](samples/supervisor.md) documentation for more information.
