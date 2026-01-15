# Ocre Build System for Zephyr

Ocre build system is tightly integrated with Zephyr's build system, allowing developers to leverage the power of Zephyr's build system while also providing a more streamlined and user-friendly experience for developing Ocre itself.

Since Zephyr does not support multiple applications, we need to build each sample separately. For this reason, we recommend the usage of multiple `build` directories with west.

For more information about west and Zephyr, please refer to the [Zephyr documentation](https://docs.zephyrproject.org/latest/guides/west/index.html).

## Build integration

We provide several points of integration with Zephyr:

- Ocre West Project
- Ocre Zephyr Module
- Ocre Sample Applications
    - mini
    - demo
    - supervisor

The general usage of these points of integration are described below.

### Ocre West Project

The Ocre West Project is a lightweight way of getting started with Ocre development.
It should just check out the very few modules and tools necessary for Ocre development on a supported board.

It is defined by the `west.yml` file in the root directory of the project.

There are two main possibilities to use it. If the Ocre runtime source code is already checked out, you can use the following command to initialize the project (run from the root of the source tree):

```sh
west init -l .
```

Here, `-l .` tells west to initialize the project using the manifest (`west.yml` in the current directory).

This will create a `.west` directory in `..` (i.e. side by side with the ocre-runtime directory). And when `west update` is run, Zephyr and its modules will also be checked out in `..`. 

This is the recommended way to work with the [Devcontainers](./Devcontainers.md).

If the Ocre runtime source code is not checked out, you use a remote path to check out the Ocre West Project:

```sh
west init https://github.com/project-ocre/ocre-runtime.git
```

This will create a `.west` directory in the current directory, and when `west update` is run, Ocre, Zephyr and its modules will be checked out in the current directory.

With this project initialized, the following command is to actually check out all the code:

```sh
west update
```

And then you can proceed to building one of the sample applications below.

### Ocre Zephyr Module

Ocre is provided as a Zephyr module. This makes it easy to integrate with existing Zephyr projects.

The modules is defined by the `zephyr` directory in the root of the project. This adds some configuration variables
and kconfig options to Zephyr. Ocre is also built as a library, as it does not implement the function `main()`.
The user is free to implement it on their own projects.

The module is usually included in a Zephyr project through the use of `ZEPHYR_EXTRA_MODULES` CMake variable of the `EXTRA_ZEPHYR_MODULES` environment variable.

The samples described below already do this, so no extra configuration is required.

### Ocre Sample Applications

The provided sample applications use the Ocre Zephyr Module and can be used for demonstration
or a starting point for your own projects.

These applications already include the Zephyr module on their `CMakeLists.txt` file, by setting the variable `ZEPHYR_EXTRA_MODULES`.

### Configuration

This sections describes the build-time configuration required for Ocre to work.

#### Version

There are several components of the Ocre version that gets compiled in the project.

The file `src/ocre/version.h` includes the Ocre Library version string.
While the file `commit_id.h` includes the commit ID of the Ocre Library.
If this file is not present in the source tree, the file is generated during the build process and is stored in the build directory.
If the file does not exist, and the source tree is not a valid git repository, the build will fail.

Note that these are unrelated to the Zephyr version and the Zephyr application version mechanism (i.e. throught he use of the VERSION file in the Zephyr application directory).

#### Kconfig

There are several Kconfig options that can be used to configure Ocre.
They are described in the `zephyr/Kconfig` file in this repository.

The easiest way to configure them is to first build one of the Ocre sample applications, and then do:

```sh
west built -t menuconfig
```

Then navigate to Zephyr -> Modules -> Ocre and look for their help pages.

The Kconfig system is also used to extract the variables `CONFIG_OCRE_STORAGE_PARTITION_ADDR`, `CONFIG_OCRE_STORAGE_PARTITION_SIZE` and `CONFIG_OCRE_STORAGE_PARTITION_BLOCK_SIZE`. These parameters are used to generate the littlefs storage partition `merged.hex` file described below.

#### build-time variables

Some relevant build-time variables are described below:

- `OCRE_INPUT_FILE_NAME`: Absolute path to the container file to be executed by the mini sample application.
- `OCRE_PRELOADED_IMAGES`: List of absolute paths images to be added to the state information directory.
- `OCRE_SDK_PRELOADED_IMAGES`: List of ocre-sdk submodule target images to be added to the state information directory.

#### Storage partition

In Zephyr, the [state information](StateInformation.md) is usually stored in a separate littlefs flash partition,
defined in the device tree. We use the widely used `storage_partition` device-tree labelled partition mounted at
`/lfs`. This is usually done with a `fstab.overlay` file (already included in our demos for the supported boards).

Ocre will use a directory `ocre` inside this filesystem. For more information, see [StateInformation.md](StateInformation.md).

The `fstab.overlay` file is included by appending to the list `EXTRA_DTC_OVERLAY_FILE` in the samples `CMakeLists.txt` files.

#### merged.hex

On platforms that support, if the storage partition related variables are set (potentially to their auto-detected values) in Kconfig, and when the flash programmer runner supports it, it is possible to flash the generated `merged.hex` file which includes the Zephyr application and the storage partition.
