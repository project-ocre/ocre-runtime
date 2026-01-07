# @copyright Copyright (c) contributors to Project Ocre,
# which has been established as Project Ocre a Series of LF Projects, LLC
#
# SPDX-License-Identifier: Apache-2.0

add_library(OcrePlatform)

target_sources(OcrePlatform
    PRIVATE
    memory.c
    log.c
    file_mmap.c
    # file_alloc_read.c
)

target_include_directories(OcrePlatform
    PUBLIC
    include
    ../include
)
