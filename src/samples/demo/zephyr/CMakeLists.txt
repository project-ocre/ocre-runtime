# @copyright Copyright (c) contributors to Project Ocre,
# which has been established as Project Ocre a Series of LF Projects, LLC
#
# SPDX-License-Identifier: Apache-2.0

# This file is to be used to build a zephyr firmware including the ocre demo
# automatically uses the ocre module in the parent directory

cmake_minimum_required(VERSION 3.20.0)

# comment out to use the ocre module provided by Zephyr
list(APPEND ZEPHYR_EXTRA_MODULES ${CMAKE_CURRENT_LIST_DIR}/../../../..)

# append the fstab overlay file to the list of DTC overlay files
list(APPEND EXTRA_DTC_OVERLAY_FILE fstab.overlay)

include(../demo_containers.cmake)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project (ocre_sample_mini)

target_sources(app
    PRIVATE
    ../main.c
)

target_link_libraries(app
    PUBLIC
    OcreCore
)

add_dependencies(app
    OcreSampleContainers
)
