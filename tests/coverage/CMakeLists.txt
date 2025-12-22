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

add_custom_target(coverage ALL
    COMMAND llvm-profdata merge -sparse
        OcreSystemTests/test_lib.profraw
        OcreSystemTests/test_ocre.profraw
        OcreSystemTests/test_context.profraw
        OcreSystemTests/test_container.profraw
        -o default.profdata
    COMMAND llvm-cov show
        -format=html -output-dir=report
        -instr-profile=default.profdata
        --ignore-filename-regex=$wasm-micro-runtime/
        --ignore-filename-regex=$test/
        --object=OcreSystemTests/test_lib
        --object=OcreSystemTests/test_ocre
        --object=OcreSystemTests/test_context
        --object=OcreSystemTests/test_container
    DEPENDS
    run-systests
)
