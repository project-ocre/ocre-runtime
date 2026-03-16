# @copyright Copyright (c) contributors to Project Ocre,
# which has been established as Project Ocre a Series of LF Projects, LLC
#
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

add_library(OcreClient)

target_sources(OcreClient
    PRIVATE
    client.c
    context.c
    ../zcbor_helpers.c
    ../zcbor/src/zcbor_encode.c
    ../zcbor/src/zcbor_decode.c
    ../zcbor/src/zcbor_common.c
)

target_include_directories(OcreClient
    PUBLIC
    ../../../../ocre/include
    ../../../../runtime/include
    PRIVATE
    ../zcbor/include
    ..
)

target_link_libraries(OcreClient
    PUBLIC
    OcreCommon
)
