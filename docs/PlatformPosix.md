# POSIX Platform

In Linux, pthreads are enabled by compiling with `-pthread` and linking with `-lpthread`. This might not be required for other POSIX platforms. The required build flags are automatically selected by the cmake build system.

## WAMR

WAMR has built-in support for most POSIX operating systems, including Linux. The WAMR integration is out of scope of the Ocre Posix platform.

Check [WAMR porting guide](https://github.com/bytecodealliance/wasm-micro-runtime/blob/main/doc/port_wamr.md) for more information about WAMR platform requirements.

## container loading

For loading the container into the memory, we use `mmap(2)` by default. This is the recommended way as this can provide XIP functionality.

We also provide other alternative implementations in case the system does not support `mmap(2)`:

- Using POSIX syscalls
- Using file streams

Check the files in `src/platform/posix/file_alloc_*.c` for reference implementations.

## user memory

Since in POSIX or Linux there is no standard tiered memory API, these are implemented as `malloc(3)`, `realloc(3)` and `free(3)`

## logging

`LOG_MODULE_REGISTER(module, ...)` and `LOG_MODULE_DECLARE(module, ...)` will just define some `static const` variable with the module name; while the `LOG_*(fmt, ...)` macros will use `fprintf(3)` to log the message to `stderr`.

## config.h

This file, in the platform is required to define the following macro:

- `CONFIG_OCRE_DEFAULT_WORKING_DIRECTORY` is set to `src/ocre/var/lib/ocre`, since we currently do not build a production build.

All Ocre specific optionals are enabled. Since in Linux the memory impact of these features are negligible, we enable them by default.
