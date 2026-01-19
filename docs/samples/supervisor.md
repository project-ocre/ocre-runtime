# Supervisor

The Supervisor will run "ocre daemon" in background, being controlled by the ocre-cli command line (shell) tool. It by default will preload the following images,
but will not start them. Instead it will provide a command line controller for managing these. Check the [Ocre CLI](../OcreCli.md) documentation for details.

It also preloads the following images:

- `blinky.wasm`
- `filesystem-full.wasm`
- `filesystem.wasm`
- `hello-world.wasm`
- `publisher.wasm`
- `subscriber.wasm`
- `webserver-complex.wasm`
- `webserver.wasm`

Currently it only supports Zephyr.

## Building and running on Zephyr

### Building

Make sure you are using the [Devcontainer](../Devcontainers.md) or have the necessary
tools for Linux described in the [Get Started with Zephyr](../GetStartedZephyr.md) guide.

You should have done at least the `west init` and `west update` commands to proceed.

Build the firmware image with:

```sh
west build -p always -b b_u585i_iot02a src/samples/supervisor/zephyr/
```

You can also replace `b_u585i_iot02a` with another board.

### Flashing

This sample will load the container images from the state information directory,
which in Zephyr is `/lfs/ocre` by default. This directory could have a `images`
subdirectory preloaded with images. Additional images can be pulled with `ocre pull` command.

If we just use `west flash` here, it will flash the main app partition, but if the
images are not in the filesystem, there will be no preloaded images.

As part of the build artifacts, on supported platforms, is `merged.hex` which is
a hex file providing the application and the storage_partition with the state information data with the preloaded images for the sample.

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

## Using ocre-cli (shell)

Quick usage of ocre client is described below. Detailed usage information can be found on the [Ocre CLI](../OcreCli.md) documentation.

Get help:

```
ocre
```

List local images:

```
ocre image ls
```

It should display the local images:

```
SHA-256									                                          SIZE	NAME
d9d2984172d74b157cbcd27ff53ce5b5e07c1b8f9aa06facd16a59f66ddd0afb	22772	blinky.wasm
fdeffaf2240bd6b3541fccbb5974c72f03cbf4bdd0970ea7e0a5647f08b7b50a	58601	filesystem-full.wasm
a8042be335fd733ecf4c48b76e6c00a43b274ad9b0d9a6d3c662c5f0c36d4a40	41545	filesystem.wasm
4a42158ff5b0a4d0a65d9cf8a3d2bb411d846434a236ca84b483e05b2f1dff99	5026	hello-world.wasm
c7b29c38bd91f67e69771fbe83db4ae84d515a6038a77ee6823ae377c55eac3c	23111	publisher.wasm
5f94ea4678c4c1ab42a3302e190ffe61c58b8db4fcf4919e1c5a576a1b8dcd3b	22944	subscriber.wasm
496ab513d6b1b586f846834fd8d17e0360c053bc614f2c2418ef84a87fbcd384	98082	webserver-complex.wasm
0a8cd55cb93c995d71500381c11bab1f2536c66282b8cab324b42c35817fba57	81647	webserver.wasm
```

If there are no images, before you proceed, you can use `ocre pull` to populate the images.
Check the Ocre SDK and [Ocre CLI](../OcreCli.md) documentation for details.

Start a container with the the `hello-world.wasm` image:

```
ocre run hello-world.wasm
```

Start a background container named `my_blinky` with the `blinky.wasm` image:

```
ocre run -n my_blinky -d -k ocre:api blinky.wasm
```

Show container statuses:

```
ocre ps
```

Kill the `my_blinky` container:

```
ocre kill my_blinky
```

Remove the `my_blinky` container:

```
ocre rm my_blinky
```

Please, refer to [Ocre CLI](../OcreCli.md) documentation and help messages for details

## Customization

It is possible to customize the preloaded container in the demo sample [state information](../StateInformation.md) directory through the following variables.

- `OCRE_PRELOADED_IMAGES`: List of absolute paths images to be added to the state information directory.
- `OCRE_SDK_PRELOADED_IMAGES`: List of ocre-sdk submodule target images to be added to the state information directory.

Note that for `OCRE_PRELOADED_IMAGES` the path to the container files must be absolute. And `OCRE_SDK_PRELOADED_IMAGES` list targets for the ocre-sdk build that should be added to the state information directory. Both variables can be included. For example, in Linux, we could have done:

```sh
west build -p always -b b_u585i_iot02a src/samples/supervisor/zephyr/ -- "-DOCRE_SDK_PRELOADED_IMAGES=webserver.wasm;filesystem.wasm" "-DOCRE_PRELOADED_IMAGES=/absolute/path/to/image1.wasm;/absolute/path/to/image2.wasm"
```

Note the `--` is required for west to pass these arguments to CMake and the `"` are required because we have `;` in the lists.

And these containers will be added to the state information directory.

For more information about build customization, please refer to the [Linux Build System](../BuildSystemLinux.md) or [Zephyr Build System](../BuildSystemZephyr.md) documentation.
