# Add Ocre Runtime to a custom Linux Application

This document will guide you through the process of embedding Ocre Runtime into a custom Linux application. For integrating Ocre Runtime into a Zephyr application, please refer to the [Custom Zephyr Application](CustomZephyrApplication.md) documentation.

We will provide instructions for a CMake project. Other build systems can be used but are not documented here.

Note that this is not the only way to achieve the integration of Ocre Runtime into a Linux application. As Ocre Runtime is provided as a library, we recommend you just link it statically to your application, as this guide will show.

Alternatives range from building the Ocre Runtime as an ExternalProject and then linking it statically or dynamically to your application.

## Prerequisites

Use the [Devcontainer](Devcontainers.md), follow the [Get started with Linux](GetStartedLinux.md) guide or set up a [Native Build](NativeBuild.md) development environment so you can compile Ocre Runtime.

## CMake Project Integration

Say we have any CMake project with a structure like:

```
.
├── CMakeLists.txt
└── main.c
```

The contents of `CMakeLists.txt` are as follows:

```cmake
cmake_minimum_required(VERSION 3.20)

project(my_project)

add_executable(app main.c)
```

And the contents of `main.c` are as follows:

```c
#include <stdio.h>

int main() {
    printf("Hello, World!\n");
    return 0;
}
```

`Hello, World!` is being printed to the console from native code.

To add Ocre runtime to this project, if you are in a Git repository, we can add it as a git submodule:

```
git submodule add https://github.com/project-ocre/ocre-runtime.git
git submodule update --init --recursive
```

Or if you are not in a Git repository, we can just clone the repository:

```sh
git clone --recursive https://github.com/project-ocre/ocre-runtime.git
```

So we have a directory structure like:

```
.
├── CMakeLists.txt
├── main.c
└── ocre-runtime
    └── ...
```

Then, we can add the following lines to `CMakeLists.txt`:

```cmake
add_subdirectory(ocre-runtime)

target_link_libraries(app PRIVATE OcreCore)
```

And now just try to build the project:

```sh
mkdir build
cd build
cmake ..
make
```

If this works, we can successfully build Ocre Runtime library. We can now proceed to the next section about using Ocre from your application.

## Using Ocre from your application

Now that we have a build working, and we can successfully build Ocre Runtime library and our application, replace the `main.c` contents with:

```c
#include <stdio.h>
#include <stdbool.h>
#include <ocre/ocre.h>

int main() {
  int rc = ocre_initialize(NULL);
	if (rc) {
		fprintf(stderr, "Failed to initialize runtimes\n");
		return 1;
	}

	struct ocre_context *ocre = ocre_create_context("/tmp/ocre");
	if (!ocre) {
		fprintf(stderr, "Failed to create ocre context\n");
		return 1;
	}

	struct ocre_container *hello =
		ocre_context_create_container(ocre, "hello.wasm", "wamr/wasip1", NULL, false, NULL);

	if (!hello) {
		fprintf(stderr, "Failed to create container\n");
		return 1;
	}

	rc = ocre_container_start(hello);
	if (rc) {
		fprintf(stderr, "Failed to start container\n");
		return 1;
	}

    return 0;
}
```

This code will create a new Ocre context with `/tmp/ocre` as the working directory, and then will start a container with the image `hello.wasm`, which we will copy from Ocre SDK.

To use your own container image, please refer to the [Create and Run custom containers](CreateAndRunCustomContainers.md) documentation.

We can now proceed with the build process by running `make`, possibly in the build directory.

We will now create the working directory `/tmp/ocre` and copy the `hello.wasm` file into it:

You can use the following commands from the build directory:

```sh
mkdir -p /tmp/ocre/images
cp ./ocre-runtime/src/ocre/OcreSampleContainers/build/hello-world.wasm /tmp/ocre/images/hello.wasm
```

Note that we "renamed" the `hello-world.wasm` file to `hello.wasm` for simplicity.

Now you can run your application:

```sh
./app
```

And it should run the `hello.wasm` container.

For more information, check the [Linux Build system](BuildSystemLinux.md) documentation.
