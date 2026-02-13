# @copyright Copyright (c) contributors to Project Ocre,
# which has been established as Project Ocre a Series of LF Projects, LLC
#
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

# set(CMAKE_C_COMPILER /usr/bin/clang)

list(APPEND OCRE_SDK_PRELOADED_IMAGES
    "return0.wasm"
    "return1.wasm"
    "sleep_5_return_0.wasm"
)

if (SANITIZER)
    string(APPEND CMAKE_C_FLAGS
        "-fsanitize=address"
)
endif()

project(OcreSystemTestPosix)

add_subdirectory(../../.. ocre)

add_library(Unity STATIC
    ../../Unity/src/unity.c
)

target_include_directories(Unity PUBLIC
    ../../Unity/src
)

list(APPEND OCRE_TESTS
    lib
    ocre
    context
    container
)

file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/src/ocre/var/lib/ocre/images)
file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/src/ocre/var/lib/ocre/containers)

foreach(test ${OCRE_TESTS})
    add_executable(test_${test}
        ../${test}.c
    )

    set_source_files_properties(${CMAKE_CURRENT_LIST_DIR}/../ocre.c PROPERTIES COMPILE_FLAGS
        -DOCRE_TEST_OTHER_CONTEXT_PATH=\\"./othercontext\\"
    )

    target_link_libraries(test_${test}
        OcreCommon
        OcreCore
        Unity
    )

    add_custom_command(
        OUTPUT test_${test}.log
        COMMAND cd ocre && LLVM_PROFILE_FILE=../test_${test}.profraw ../test_${test} > ../test_${test}.log
        COMMAND rm -f test_${test}.testpass test_${test}.testfail
        COMMAND sh -c "[ \"$(tail -1 test_${test}.log)\" = \"OK\" ] && cp test_${test}.log test_${test}.testpass || cp test_${test}.log test_${test}.testfail"
        DEPENDS
            test_${test}
        VERBATIM
    )
endforeach()

add_custom_target(run-systests
    COMMAND python3 ${CMAKE_CURRENT_LIST_DIR}/../../Unity/auto/unity_test_summary.py ${CMAKE_CURRENT_BINARY_DIR}
    DEPENDS
        test_lib.log
        test_ocre.log
        test_context.log
        test_container.log
)
