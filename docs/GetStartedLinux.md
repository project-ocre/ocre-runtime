# Get Started with Linux

The samples we currently provide for Linux are the following. Check their specific instruction for
detailed explanation for each sample:

- [`src/samples/mini/linux`](samples/mini.md)
- [`src/samples/demo/linux`](samples/demo.md)

We also provide tests. Please refer to their specific instructions for detailed explanation:

- [`tests/system`: System tests](tests/SystemTests.md)
- [`tests/leaks`: Memory leak checks](tests/LeakChecks.md)

We can also generate source code coverage reports and render the documentation:

- [`tests/coverage`: Source code coverage reports](tests/CoverageReports.md)
- [`docs`: Render documentation](RenderDocs.md)

## Set up instructions

### Devcontainer

The devcontainer can be build and started in VS Code (using the devcontainer extension) or using the devcontainer-cli and requires a working docker environment. Please refer to the [devcontainer section](Devcontainers.md) for information on how to set up the devcontainer.

### Native host development setup

If you prefer to use native builds, you need:

- Linux distribution: Ubuntu 24.04 is recommended.
- Git 2.43 or higher
- CMake 3.20.0 or higher
- A C99 compatible compiler (GCC or Clang recommended)

To run the leak checks, `clang-18` is required. To build the Doxygen documentation, `doxygen`, and for the
automatic formatting, `clang-format` is required.

Install everything in Ubuntu 24.04 with:

```sh
sudo apt install git cmake build-essential clang-18 doxygen clang-format
```

## General instructions

The build system is based on cmake. There are several "entry points" to the build system:

- `/`: Main Ocre Library and samples
- `/tests/system/posix`: POSIX System Tests
- `/tests/leaks`: Memory Leak Checks
- `/tests/coverage`: Source code coverage generation
- `/docs`: Documentation

The building of Ocre with the main entry point `/` is described below. The building of the other entry points is described on each of their documentation pages, linked above.

## Main Ocre Library and samples

This will build the main ocre library and the mini and demo samples in release mode.

Go to the root of this repository and do:

```sh
mkdir build
cd build
cmake ..
make
```

If you need to build the library in debug mode, pass the argument `-DCMAKE_BUILD_TYPE=Debug` to
`cmake ..` command above.

To run the `mini` sample, from the `build` directory, execute:

```sh
./src/samples/mini/posix/ocre_mini
```

For more information about the `mini` sample, refer to the [mini sample documentation](samples/mini.md).

To run the `demo` sample, from the `build` directory, execute:

```sh
./src/samples/demo/posix/ocre_demo
```

The working directory should be the `build` directory. `ocre_demo` requires the state information files in
`./var/lib/ocre`.

For more information about the `demo` sample, refer to the [demo sample documentation](samples/demo.md).
