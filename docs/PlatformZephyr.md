# Zephyr Platform

In Zephyr, the POSIX API is selected by the `CONFIG_POSIX_API` configuration option.

## WAMR

WAMR has built-in support for Zephyr. The WAMR integration is out of scope of the Ocre Zephyr platform.

Check [WAMR porting guide](https://github.com/bytecodealliance/wasm-micro-runtime/blob/main/doc/port_wamr.md) for more information about WAMR platform requirements.

## container loading

For loading the container into the memory, we use `open(2)`, `malloc(3)` and `read(2)`. Zephyr does not support `mmap(2)` and file streams are currently broken in Zephyr 4.3.0.

Check the files in `src/platform/posix/file_alloc_read.c` for implementation.

## user memory

If `CONFIG_SHARED_MULTI_HEAP` is disabled, there is no tiered memory API, these are implemented as `malloc(3)`, `realloc(3)` and `free(3)`.

If `CONFIG_SHARED_MULTI_HEAP` is enabled, then these are implemented as `shared_multi_heap_aligned_alloc()` with `SMH_REG_ATTR_EXTERNAL` and `shared_multi_heap_free()`.

Check Zephyr's [Shared Multi Heap](https://docs.zephyrproject.org/latest/kernel/memory_management/shared_multi_heap.html) documentation or [Adding a Zephyr board](AddZephyrBoard.md) documentation for more details about shared multi heap.

## logging

`log.h` just includes Zephyr's logging system, as it is transparent.

## config.h

`config.h` does not include anything, as Zephyr's `autoconf.h` is automatically included (by command line) to every `.c` file in Zephyr build.
