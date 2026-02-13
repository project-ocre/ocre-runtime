# @copyright Copyright (c) contributors to Project Ocre,
# which has been established as Project Ocre a Series of LF Projects, LLC
#
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

set(THREADS_PREFER_PTHREAD_FLAG TRUE)
find_package(Threads)

add_executable(ocred
    ocred.c
    handlers.c
    waiters.c
    ../zcbor_helpers.c
    ../zcbor/src/zcbor_encode.c
    ../zcbor/src/zcbor_decode.c
    ../zcbor/src/zcbor_common.c
)

target_link_libraries(ocred
    PRIVATE
    OcreCore
    Threads::Threads
)

target_include_directories(ocred
    PRIVATE ../zcbor/include
)
