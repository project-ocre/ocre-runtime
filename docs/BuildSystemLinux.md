# Ocre Build System for Linux

Ocre build system is based on CMake. This provides an easy way of embedding Ocre in diffrerent Linux applications,
as well as easy integration with other projects and generation of target executables.

Since Linux does support multiple applications, we need a single entry point for building the Ocre library and all the sample applications.

However, for [memory leak checks](tests/LeakChecks.md) and [source code test coverage report generation](tests/CoverageReports.md), it is necessary to build Ocre with a different set of build time options. While for [system tests](tests/SystemTests.md), we need to include Unity. Please check their documentation for more information.

This file described the generic configuration that is used by Ocre Library or the builds described above.

### Configuration

This sections describes the build-time configuration honored by samples and tests in Linux.

#### Version

There are several components of the Ocre version that gets compiled in the project.

The file `src/common/version.h` includes the Ocre Library version string.
While the file `commit_id.h` includes the commit ID of the Ocre Library.
If this file is not present in the source tree, the file is generated during the build process and is stored in the build directory.
If the file does not exist, and the source tree is not a valid git repository, the build will fail.

Check [Versioning](Versioning.md) documentation for more information.

#### Ocre build Configuration

The default Ocre build configuration is in the file `src/platform/posix/include/ocre/platform/config.h`.
These options should not be changed by the user and the defaults should be used.

#### build-time variables

Some relevant build-time variables are described below:

- `OCRE_INPUT_FILE_NAME`: Absolute path to the container file to be executed by the mini sample application.
- `OCRE_PRELOADED_IMAGES`: List of absolute paths images to be added to the state information directory.
- `OCRE_SDK_PRELOADED_IMAGES`: List of ocre-sdk submodule target images to be added to the state information directory.

#### State information directory

In Linux, the [state information](StateInformation.md) is usually stored by default in `/var/run/ocre` if Ocre is installed as a system service. For tests, these are usually stored relative to the build directory.

Whenever an Ocre context is created, the workspace directory will be created and used as the state information directory.
