# Source code test coverage reports

The source code coverage report generation coverage mapping features provided in clang
to build an instrumented version of Ocre and execute the [System Tests](SystemTests.md).

Currently, these are only available for the POSIX platform.

Because Ocre will need to be built with a different set of compile options, the entry point is separate:

## Build and run

Create a build directory in the root of the repository (or anywhere else) and navigate to it:

```sh
mkdir tests/coverage/build
cd tests/coverage/build
```

Configure the cmake project. Note that `..` points to `tests/coverage` in the Ocre source tree:

```sh
cmake ..
make
```

To build, run the tests and generate the report:

```sh
make
```

If the command was successful, the coverage report is inside `report` in the `build` directory.
