cmake_minimum_required(VERSION 3.20.0)

project(ocre LANGUAGES C ASM)

set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_C_FLAGS "-Wall")

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_VERBOSE_MAKEFILE ON)

set(CMAKE_C_STANDARD 99)
set(CMAKE_CXX_STANDARD 17)

set(WAMR_BUILD_PLATFORM linux)
# set(WAMR_BUILD_PLATFORM darwin)



# core
add_subdirectory(src/ocre)
add_subdirectory(src/runtime)

# runtime
include (cmake/wamr.cmake)
add_subdirectory(src/runtime/wamr)

# platform
add_subdirectory(src/platform/posix)

# shell
#add_subdirectory(src/shell)

# samples
add_subdirectory(src/samples/mini/posix)
add_subdirectory(src/samples/demo/posix)

include (src/samples/demo/demo_containers.cmake)

# TODO move these to testing
list(APPEND OCRE_SDK_PRELOADED_IMAGES
    "return0.wasm"
    "return1.wasm"
    "sleep_5_return_0.wasm"
)

include (cmake/sample_containers.cmake)

# /var/lib/ocre directory
include (cmake/state_information.cmake)

add_subdirectory(src/samples/bash_runtime/posix)
