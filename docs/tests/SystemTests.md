# System tests

System Tests are tests designed to test the entire system as a black box,
including all components and dependencies.
We usually build it in release mode, so we test exactly what we are delivering.

Currently, these are only available for the POSIX platform.

However, these are also sometimes reused for memory [leak checks](LeakChecks.md) and [source code coverage generation](CoverageReports.md).
In these cases, Ocre needs to be built with instrumentation so it is not testing the release build.
For more information on those, refer to their specific documentation.

These tests are configured in the CI workflows as a quality gate. All PRs must pass all of them.

## Build and run

Create a build directory and navigate to it:

```sh
mkdir tests/system/posix/build
cd tests/system/posix/build
```

Configure and build the cmake project. Note that `..` points to `tests/system/posix`:

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

## Details

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
