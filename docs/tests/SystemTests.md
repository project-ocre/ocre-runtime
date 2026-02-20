# System tests

System Tests are tests designed to test the entire system as a black box,
including all components and dependencies.
We usually build it in release mode, so we test exactly what we are delivering.

Currently, these are available for POSIX and Zephyr platforms.

However, these are also sometimes reused for memory [leak checks](LeakChecks.md) and [source code coverage generation](CoverageReports.md).
In these cases, Ocre needs to be built with instrumentation so it is not testing the release build.
For more information on those, refer to their specific documentation.

These tests are configured in the CI workflows as a quality gate. All PRs must pass all of them.

## Linux

### Build and run

Create a build directory and navigate to it:

```sh
mkdir tests/system/posix/build
cd tests/system/posix/build
```

Configure and build the cmake project. Note that `..` points to `tests/system/posix`:

Alternatively, the IPC system tests can be build in the directory `tests/system/posix-ipc`. Note that only `test_context` and `test_container` are available in IPC mode.

```sh
cmake ..
make
```

To run all the tests, execute:

```sh
make run-systests
```

The last lines of the output will be something like:

```
--------------------------
OVERALL UNITY TEST SUMMARY
--------------------------
51 TOTAL TESTS 0 TOTAL FAILURES 0 IGNORED
```

You can also run the individual test executable binaries `test_*` in the build directory.

Note that the tests must be run from the build directory, because they look for images in the current working directory + `./ocre/src/ocre/var/lib/ocre/images/`.

### Details

The individual test binaries:

- `test_lib`
- `test_ocre`
- `test_context`
- `test_container`

Follow a similar pattern. `test_lib` is testing the general library initialization functions.
`test_ocre` initializes the Ocre library, and tests the functionality of management of contexts.
`test_context` instantiates a context and tests its functionality, creating and managing the lifetime of the containers.
`test_container` tests the specific functionality of a specific container.

Please, refer to their source code for more details.

## Zephyr

### Build and run

To build the Zephyr system tests for the `native_sim_64` board:

```sh
west build -p always -b native_sim/native/64 tests/system/zephyr/context
```

Replace `context` with `container`, `lib`, or `ocre` to build different tests:

- `tests/system/zephyr/lib` - Tests the general library initialization functions
- `tests/system/zephyr/ocre` - Tests the Ocre library and management of contexts
- `tests/system/zephyr/context` - Tests context functionality, creating and managing the lifetime of containers
- `tests/system/zephyr/container` - Tests the specific functionality of a container

To run the test:

```sh
west build -t run
```

The last lines of the output will be something like:

```
-----------------------
25 Tests 0 Failures 0 Ignored
OK
```

### Details

Each test must be run individually. Run the build command for one test at a time, then execute `west build -t run` to run that specific test.
