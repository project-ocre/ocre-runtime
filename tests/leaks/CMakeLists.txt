# @copyright Copyright (c) contributors to Project Ocre,
# which has been established as Project Ocre a Series of LF Projects, LLC
#
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

set(CMAKE_C_COMPILER /usr/bin/clang)

string(APPEND CMAKE_C_FLAGS
    "-fsanitize=address"
)

project(OcreLeaksCheckPosix)

add_subdirectory(../system/posix OcreSystemTests)

add_custom_target(always_run ALL
    DEPENDS run-systests run-demo run-mini
)
