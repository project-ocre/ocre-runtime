# @copyright Copyright (c) contributors to Project Ocre,
# which has been established as Project Ocre a Series of LF Projects, LLC
#
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20)

add_library(OcreRuntime
    INTERFACE
    include/ocre/runtime/vtable.h
)

target_include_directories(OcreRuntime
    INTERFACE
    include
)

install(FILES include/ocre/runtime/vtable.h DESTINATION include/ocre/runtime)
