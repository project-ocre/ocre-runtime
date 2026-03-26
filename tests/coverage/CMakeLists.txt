# @copyright Copyright (c) contributors to Project Ocre,
# which has been established as Project Ocre a Series of LF Projects, LLC
#
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

set(CMAKE_C_COMPILER /usr/bin/clang)

set(CMAKE_C_FLAGS
    "-fprofile-instr-generate -fcoverage-mapping"
)

project(OcreTestSourceCoverage)

add_subdirectory(../system/posix OcreSystemTests)

add_custom_target(default.profdata
    COMMAND llvm-profdata merge -sparse
    OcreSystemTests/test_lib.profraw
    OcreSystemTests/test_ocre.profraw
    OcreSystemTests/test_context.profraw
    OcreSystemTests/test_container.profraw
    -o default.profdata
    DEPENDS
    OcreSystemTests/test_lib.profraw
    OcreSystemTests/test_ocre.profraw
    OcreSystemTests/test_context.profraw
    OcreSystemTests/test_container.profraw
    run-systests
)

add_custom_target(coverage ALL
    COMMAND llvm-cov show
        -format=html -output-dir=coverage
        -instr-profile=default.profdata
        --sources ${CMAKE_CURRENT_LIST_DIR}/../../src
        --object=OcreSystemTests/test_lib
        --object=OcreSystemTests/test_ocre
        --object=OcreSystemTests/test_context
        --object=OcreSystemTests/test_container
    DEPENDS
        OcreSystemTests/test_lib
        OcreSystemTests/test_ocre
        OcreSystemTests/test_context
        OcreSystemTests/test_container
        default.profdata
)

add_custom_target(report.md
    COMMAND echo '```' > report.md
    COMMAND llvm-cov report
        -instr-profile=default.profdata
        --sources ${CMAKE_CURRENT_LIST_DIR}/../../src
        --object=OcreSystemTests/test_lib
        --object=OcreSystemTests/test_ocre
        --object=OcreSystemTests/test_context
        --object=OcreSystemTests/test_container
        >> report.md
    COMMAND echo '```' >> report.md
    DEPENDS
        OcreSystemTests/test_lib
        OcreSystemTests/test_ocre
        OcreSystemTests/test_context
        OcreSystemTests/test_container
        default.profdata
)
