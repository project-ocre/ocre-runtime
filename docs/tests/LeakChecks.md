# Memory leak checks

The memory leak checks will leverage the address sanitizer features provided in clang
to build an instrumented version of Ocre and execute the following binaries:

- [mini sample](../samples/mini.md)
- [demo sample](../samples/demo.md)
- [System Tests](SystemTests.md)

Currently, these are only available for the POSIX platform.

Because Ocre will need to be built with a different set of compile options, the entry point is separate:

## Build and run

Create a build directory in the root of the repository (or anywhere else) and navigate to it:

```sh
mkdir tests/leaks/build
cd tests/leaks/build
```

Configure the cmake project. Note that `..` points to `tests/leaks` in the Ocre source tree:

```sh
cmake ..
```

To build and run all the tests and samples, execute:

```sh
make
```

Any memory leaks detected by clang address sanitizer will be printed and the command will fail.
