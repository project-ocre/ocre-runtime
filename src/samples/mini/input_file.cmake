# @copyright Copyright (c) contributors to Project Ocre,
# which has been established as Project Ocre a Series of LF Projects, LLC
#
# SPDX-License-Identifier: Apache-2.0

if(OCRE_INPUT_FILE)
    message(STATUS "Using user input file: ${OCRE_INPUT_FILE}")
else()
    message(STATUS "Using default input file: hello-world.wasm")
    set(OCRE_INPUT_FILE "${CMAKE_CURRENT_LIST_DIR}/hello-world.wasm")
endif()

add_custom_command(
    OUTPUT input_file.hex
    COMMAND od -v -An -tx1 "${OCRE_INPUT_FILE}" > input_file.hex
    DEPENDS ${OCRE_INPUT_FILE}
    COMMENT "Generating hex file from ${OCRE_INPUT_FILE}"
)

add_custom_command(
    OUTPUT input_file.c
    COMMAND awk -f "${CMAKE_CURRENT_LIST_DIR}/../../../scripts/c_array.awk" input_file.hex > input_file.c
    DEPENDS
        input_file.hex
        ${CMAKE_CURRENT_LIST_DIR}/../../../scripts/c_array.awk
    COMMENT "Generating C array"
)

add_custom_target(input_file DEPENDS input_file.c)
