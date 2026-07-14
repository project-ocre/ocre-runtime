# @copyright Copyright (c) contributors to Project Ocre,
# which has been established as Project Ocre a Series of LF Projects, LLC
#
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

add_subdirectory(server)

add_subdirectory(client)

add_executable(ocre_cli
    ocre.c
    download.c
)

target_link_libraries(ocre_cli
    PRIVATE
    OcreShell
    OcreClient
)
