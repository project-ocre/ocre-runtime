# @copyright Copyright (c) contributors to Project Ocre,
# which has been established as Project Ocre a Series of LF Projects, LLC
#
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

project(ocre LANGUAGES C)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_C_STANDARD 99)

set(WAMR_BUILD_PLATFORM linux)

include (src/samples/demo/demo_containers.cmake)

# uthash
add_subdirectory(src/uthash)

# core
add_subdirectory(src/common)
add_subdirectory(src/ocre)
add_subdirectory(src/runtime)

# runtime
include (cmake/wamr.cmake)
add_subdirectory(src/runtime/wamr-wasip1)

# platform
add_subdirectory(src/platform/posix)

# platform
add_subdirectory(src/shell)

# samples
add_subdirectory(src/samples/mini/posix)
add_subdirectory(src/samples/demo/posix)

# this sample is useful for static code analysis on POSIX
add_subdirectory(src/samples/static_checks/posix)
