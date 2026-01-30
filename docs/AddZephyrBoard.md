# Adding support for Zephyr boards

This document will guide you through the process of adding support for Zephyr boards to Ocre.

We assume the board is already supported by Zephyr and that you have a working Zephyr build environment set up,
and that you can build at least the hello-world zephyr application.

For more information, check the [Zephyr Getting Started Guide](https://docs.zephyrproject.org/4.3.0/develop/getting_started/index.html) and if your board is not currently supported by Zephyr, check Zehyr's [Board Porting Guide](https://docs.zephyrproject.org/latest/hardware/porting/board_porting.html).

We also assume you are familiar with the instructions in the [Get Started with Zephyr](GetStartedZephyr.md) document.

## Platform requirements

### WAMR

Zephyr's WAMR integration supports many different architectures, including ARM Cortex-M, ARM Cortex-A, and RISC-V, however most of these are not actively tested.

### Memory

Ocre runtime itself requires very little RAM memory to work (about 16KB). Containers, on the other hand might require much more memory depending on the application.

In some systems, there is an external PSRAM chip with some extra RAM memory mapped to the system's address space. This provides usually a lot more memory than the internal RAM (commonly 8MB, but can do more when there will be PSRAM chips available), however this memory is slow compared to the internal RAM and usually cannot be used for drivers or special functionality (i.e. atomic variables, etc.).

Some of the example boards are:

- [Pimoroni Pico Plus 2](https://docs.zephyrproject.org/latest/boards/pimoroni/pico_plus2/doc/index.html)
- [B-U585I-IOT02A](https://docs.zephyrproject.org/latest/boards/st/b_u585i_iot02a/doc/index.html)

but there are a few more that are still not supported in Zephyr.

On these platforms, it is common for Zephyr to be built with the [Shared Multi Heap](https://docs.zephyrproject.org/latest/kernel/memory_management/shared_multi_heap.html) functionality which allows the application to choose which heap to use for different purposes.

In Zephyr, Ocre Runtime, by default will use the internal RAM (i.e. RAM ARENA -- `malloc(3)` and `free(3)`) for its internal data structures. However for application data (i.e. (WASM linear memory)), will be allocated with Shared Multi Heap with `SMH_REG_ATTR_EXTERNAL` when `CONFIG_SHARED_MULTI_HEAP` is enabled, and regular `malloc(3)` and `free(3)` if `CONFIG_SHARED_MULTI_HEAP` is not enabled.

Make sure that if your board contains multi tiered memory (i.e. an external PSRAM chip), `CONFIG_SHARED_MULTI_HEAP` is properly set up and configured in Zephyr, so we can make use of all the extra RAM available to the system.

For more information, check Zephyr's [Memory Heap](https://docs.zephyrproject.org/latest/kernel/memory_management/heap.html) documentation.

### Storage

Ocre runtime uses a filesystem to store container images and container filesystems and volumes.
This information is referred as the "State Information". In Linux, this is usually stored in `/var/lib/ocre`,
however in Zephyr, we store it in `/lfs/ocre`.

For this reason, we require that a filesystem be mounted in `/lfs` with enough space to store images and directories. This is the standard mount point for littlefs
partitions in Zephyr for most boards, however, Ocre Runtime will make use of fstab functionality to automatically mount the `storage_partition` in `/lfs`.

Note that each littlefs directory requires two littlefs blocks, so make sure that you will have plenty of littlefs blocks in this partition.

In some platforms, the `storage_partition` is in the same (usually internal) Flash memory that has the Zephyr OS and the application firmware itself. On other platforms, the `storage_partition` is in a separate Flash memory partition accessed through a separate SPI flash controller. Ocre supports both scenario, as long as the `storage_partition` is usable in Zephyr.

Make sure you have a `storage_partition` defined in your board's device tree overlay files. And that Zephyr can mount this partition, read and write to it.

For debugging storage issues in Zephyr, it is useful to use the [shell-fs Zephyr sample](https://github.com/zephyrproject-rtos/zephyr/tree/main/samples/subsys/shell/fs).

For more information, check Zephyr's [File Systems](https://docs.zephyrproject.org/latest/services/storage/file_system/index.html) documentation.

#### merged.hex mechanism

In most platforms, the `storage_partition` is in a flash controller that maps the data into the address space of the SoC. If those platforms are configured correctly, there should be a `ranges` entry in the board or soc device tree. For example in for [b_u585i_iot02a](https://github.com/zephyrproject-rtos/zephyr/blob/main/boards/st/b_u585i_iot02a/b_u585i_iot02a-common.dtsi), we have:

```
&octospi2 {
    ...
    ext_flash_ctrl: ospi-flash-controller@0 {
        ...
        ranges = <0x0 0x70000000 DT_SIZE_M(64)>; /* Ext Flash mem-mapped to 0x70000000 */
```

As the comment reads, in this chip, the external flash is mapped to the address `0x70000000` in the SoC. This information allows us to build a unified firmware image `merged.hex` which includes the firmware (Zephyr + Application + Ocre Runtime) and the storage partition. This is useful in case we want to preload containers in ocre (used in the [demo](samples/demo.md) and [supervisor](samples/supervisor.md)).

Even in platforms in which the partition lies in external flash, sometimes it is possible to flash both flash chips with a single `west flash` command. This is the case for `STM32CubeProgrammer` on `b_u585i_iot02a`.

Note that some boards have this information incorrectly placed in `reg` property.

If your board supports mapped flash, make sure the device tree is properly configured and the flash controller has the `ranges` property, so we can generate `merged.hex`.

For more information, check [State Information](StateInformation.md).

### Network

Ocre does not require networking to run. However, it is recommended to have a network interface for running networked containers and pulling new images from a remote server, through the use of the `ocre image pull` command from the [supervisor](samples.supervisor.md) sample.

Ocre does not interfere in the network stack or the connection manager. It does not wait the network to be ready before starting up.

Any network configuration and management is out of scope for Ocre runtime.

## Board-specific configuration and overlay files

We aim to provide at least the tree samples for each board. Check their documentation for their specific usage and requirements:

- [`src/samples/mini/zephyr`](samples/mini.md)
- [`src/samples/demo/zephyr`](samples/demo.md)
- [`src/samples/supervisor/zephyr`](samples/supervisor.md)

Some of these samples might require special configuration for your board. These are added to the overlay files in the samples directories.

This is also the place to put board-configuration specifics that are not included in Zephyr upstream.

Check the ocre-runtime repository for files in `src/samples/*/zephyr/boards` and check some of the currently supported boards.

Add your board files in case you require any special configuration for each of these samples.

You can now proceed to build each of the Ocre samples for your new board. Check their documentation for more information.

If you have your own application, you can add Ocre to it, see [Custom Zephyr Application](CustomZephyrApplication.md) for more information.

## Documentation

We want to keep documentation for each of the well-supported boards. These lies in `docs/boards` directory in Ocre Runtime repository. If you want to submit a pull request with your new board addition to Ocre Runtime, make sure that there is a documentation file for your board similar to the other ones already present.
