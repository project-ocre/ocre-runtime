# @copyright Copyright (c) contributors to Project Ocre,
# which has been established as Project Ocre a Series of LF Projects, LLC
#
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

add_library(OcreShell)

target_sources(OcreShell
    PRIVATE
    shell.c
    image.c
    image/ls.c
    image/pull.c
    image/sha256_file.c
    image/rm.c
    container.c
    container/create.c
    container/kill.c
    container/pause.c
    container/ps.c
    container/rm.c
    container/start.c
    container/stop.c
    container/unpause.c
    container/wait.c
    sha256/sha256.c
)

target_include_directories(OcreShell
    PRIVATE
    ../common/include
    ../ocre/include
    ../runtime/include
    PUBLIC
    include
)
