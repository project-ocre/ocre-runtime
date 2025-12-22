# @copyright Copyright (c) contributors to Project Ocre,
# which has been established as Project Ocre a Series of LF Projects, LLC
#
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

project (ocre_sample_mini)

include(../input_file.cmake)

add_executable(ocre_mini
    ../main.c
    input_file.c
)

target_link_libraries(ocre_mini
    PUBLIC
    OcreCore
)

set(THREADS_PREFER_PTHREAD_FLAG TRUE)
find_package(Threads)
target_link_libraries(ocre_mini PRIVATE Threads::Threads)

add_custom_target(run-mini
    COMMAND pwd
    COMMAND ${CMAKE_CURRENT_BINARY_DIR}/ocre_mini
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/../../../..
    DEPENDS ocre_mini
)
