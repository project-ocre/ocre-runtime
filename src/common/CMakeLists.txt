# @copyright Copyright (c) contributors to Project Ocre,
# which has been established as Project Ocre a Series of LF Projects, LLC
#
# SPDX-License-Identifier: Apache-2.0

# Check if we are in a git repository
execute_process(
    COMMAND git rev-parse --git-dir
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    OUTPUT_VARIABLE GIT_DIR
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_QUIET
)

if(GIT_DIR)
    message(STATUS "Detected git repository. Automatic commit ID tracking is enabled.")

    # Generate commit ID header file in source dir if we are in a git repository
    add_custom_command(
        OUTPUT include/ocre/commit_id.h
        COMMAND sh -c "mkdir -p ${CMAKE_CURRENT_BINARY_DIR}/include/ocre"
        COMMAND sh -c "echo '/* Auto-generated file. DO NOT EDIT */' > ${CMAKE_CURRENT_BINARY_DIR}/include/ocre/commit_id.h"
        COMMAND sh -c "echo \"#define GIT_COMMIT_ID \\\"$(git describe --always --abbrev=0 --dirty)\\\"\" >> ${CMAKE_CURRENT_BINARY_DIR}/include/ocre/commit_id.h"
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        VERBATIM
        DEPENDS always_rebuild
    )
endif()

file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/include/ocre)

# Generate build info header file in binary dir for every build
add_custom_command(
    OUTPUT include/ocre/build_info.h
    COMMAND sh -c "echo \"#define OCRE_BUILD_HOST_INFO \\\"$ENV{USER} @ $(uname -a)\\\"\" > include/ocre/build_info.h"
    COMMAND sh -c "echo \"#define OCRE_BUILD_DATE \\\"$(date +'%Y-%m-%d %H:%M:%S %Z')\\\"\" >> include/ocre/build_info.h"
    VERBATIM
    DEPENDS always_rebuild
)

# dummy command used to make the custom commands be re-run on every build (not just configure)
add_custom_command(
    OUTPUT always_rebuild
    COMMAND cmake -E echo
)

add_library(OcreCommon INTERFACE)

# if we are not in a git repository, just fail automatically if we are missing commit_id.h
target_sources(OcreCommon
    PRIVATE
    include/ocre/build_info.h
    include/ocre/commit_id.h
)

target_include_directories(OcreCommon
    INTERFACE
    include
    ${CMAKE_CURRENT_BINARY_DIR}/include
)
