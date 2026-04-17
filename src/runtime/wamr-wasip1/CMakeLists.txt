# @copyright Copyright (c) contributors to Project Ocre,
# which has been established as Project Ocre a Series of LF Projects, LLC
#
# SPDX-License-Identifier: Apache-2.0

add_library(OcreRuntimeWamr)

target_sources(OcreRuntimeWamr
    PRIVATE
    wamr.c
)

target_include_directories(OcreRuntimeWamr
    PUBLIC
    include
)

target_link_libraries(OcreRuntimeWamr
    PRIVATE
    OcreRuntime
    OcrePlatform
    OcreRuntimeAPI
    vmlib
)

# Ensure generated headers are ready before compiling
if(TARGET zephyr_generated_headers)
    add_dependencies(OcreRuntimeWamr zephyr_generated_headers)
endif()

add_subdirectory(ocre_api)
