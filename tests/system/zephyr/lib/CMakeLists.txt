# @copyright Copyright (c) contributors to Project Ocre,
# which has been established as Project Ocre a Series of LF Projects, LLC
#
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

list(APPEND ZEPHYR_EXTRA_MODULES ${CMAKE_CURRENT_LIST_DIR}/../../../..)

list(APPEND EXTRA_DTC_OVERLAY_FILE fstab.overlay)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project (ocre_system_test_lib)

add_library(Unity STATIC
    ../../../../tests/Unity/src/unity.c
)

target_include_directories(Unity PUBLIC
    ../../../../tests/Unity/src
)

target_sources(app
    PRIVATE
    ../../lib.c
)

target_link_libraries(app
    PUBLIC
    OcreCore
    Unity
)
