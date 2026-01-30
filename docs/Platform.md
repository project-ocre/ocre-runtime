# Ocre Runtime platform requirements

Ocre targets a POSIX-like operating system and makes heavy use of the POSIX API.

We specifically use `pthread(3)` for creating the runtime engine threads (which are used by the runtime engine).

Ocre Runtime also exposes a thread-safe API for managing the runtime engine and the lifecycle of the containers.
This functionality requires `pthread_mutex(3)` and `pthread_cond(3)`.

We explicitly do not make use of `pthread_kill(3)` because it is undefined and unreliable behavior.

This subset of the POSIX API is available in most modern RTOS.

## WAMR

Ocre Runtime by default uses WebAssembly Micro-Runtime (WAMR) for running WASM containers. The integration between WAMR and the underlying platform is done by WAMR and is out of scope of Ocre Runtime.

Check [WAMR porting guide](https://github.com/bytecodealliance/wasm-micro-runtime/blob/main/doc/port_wamr.md) for more information about WAMR platform requirements.

## Container loading

For loading the container into the memory, each platform can provide its own implementation.

The following functions must be available in the platform:

- `void *ocre_load_file(const char *path, size_t *size)`: should make the contents of the file specified in `path` available in a linear memory. This is usually implemented as `mmap(2)` or `open(2)` then `malloc(3)` and `read(2)`
- `int ocre_unload_file(void *buffer, size_t size)`: should release whatever was acquired by `ocre_load_file`. This is usually implemented as `munmap(2)` or `free(3)`

Check the files in `src/platform/posix/file_alloc_*.c` for reference implementations.

## User memory

Ocre Runtime allows the "application memory" (container memory) to be allocated differently depending on the platform.

The following functions must be available in the platform:

- `void *user_malloc(size_t size)`: should allocate memory of the specified size. This is usually implemented as `malloc(3)`
- `void user_free(void *p)`: should release the memory allocated by `user_malloc`. This is usually implemented as `free(3)`
- `void *user_realloc(void *p, size_t size)`: should reallocate the memory allocated by `user_malloc`. This is usually implemented as `realloc(3)`

Check the files in `src/platform/*/memory.c` for reference implementations.

## Logging system

Logging API is inspired by Zephyr's logging API. This requires the following macros to be defined and used by every module that uses logging:

- `LOG_MODULE_REGISTER(module, ...)`: should register the module with the logging system
- `LOG_MODULE_DECLARE(module, ...)`: should declare the module with the logging system

In Zephyr, these functions behave differently. Ocre just expects any one of these to be called exactly once on each module, defining the name of the module, that will be part of the loggin output.

Also, the following macros or functions should be defined:

- `LOG_ERR(fmt, ...)`: should log an error message
- `LOG_WRN(fmt, ...)`: should log a warning message
- `LOG_INF(fmt, ...)`: should log an informational message
- `LOG_DBG(fmt, ...)`: should log a debug message

These functions are expected to behave like `printf(3)`.

Check Zephyr's [Logging](https://docs.zephyrproject.org/latest/services/logging/index.html) documentation for more information.

## config.h

This file, in the platform is required to define the following macro:

- `CONFIG_OCRE_DEFAULT_WORKING_DIRECTORY`: the default working directory for OCRE. This is usually set to `/var/lib/ocre` on POSIX production systems; `src/ocre/var/lib/ocre` for testing; or `/lfs/ocre` in Zephyr.

Additional configuration options and platform overrides can be defined here as well.

This file usually just sets `CONFIG_OCRE_DEFAULT_WORKING_DIRECTORY` and imports some other file (i.e. `autoconf.h`) so that Ocre can be configured and integrated in another build system.

# Current Platform list:

Ocre Runtime currently supports the following platforms:

- [POSIX](PlatformPosix.md)
- [Zephyr](PlatformZephyr.md)

Please refer to the platform-specific documentation for more information about their implementation.
