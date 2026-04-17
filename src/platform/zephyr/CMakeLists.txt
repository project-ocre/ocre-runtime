# @copyright Copyright (c) contributors to Project Ocre,
# which has been established as Project Ocre a Series of LF Projects, LLC
#
# SPDX-License-Identifier: Apache-2.0

add_library(OcrePlatform)

target_sources(OcrePlatform
    PRIVATE
    memory.c
    ../posix/file_alloc_read.c
    lstat.c
)

add_subdirectory(.. OcrePlatformBase)

target_include_directories(OcrePlatform
    PUBLIC
    include
)

target_link_libraries(OcrePlatform
    PRIVATE
    zephyr_interface
    OcrePlatformBase
)

# Ensure generated headers are ready before compiling
if(TARGET zephyr_generated_headers)
    add_dependencies(OcrePlatform zephyr_generated_headers)
endif()
