if(NOT "${OCRE_INPUT_FILE}" STREQUAL "")
    message("Using input file: ${OCRE_INPUT_FILE}")

    # Extract filename without extension for use in software
    get_filename_component(OCRE_INPUT_FILE_NAME ${OCRE_INPUT_FILE} NAME_WE)
    message("Input file name (without extension): ${OCRE_INPUT_FILE_NAME}")
    add_definitions(-DOCRE_INPUT_FILE_NAME="${OCRE_INPUT_FILE_NAME}")

    add_custom_command(
        OUTPUT ${CMAKE_CURRENT_LIST_DIR}/src/ocre/ocre_input_file.g
        COMMAND xxd -n wasm_binary -i ${OCRE_INPUT_FILE} | sed 's/^unsigned/static const unsigned/' > ${OCRE_ROOT_DIR}/src/ocre/ocre_input_file.g
        DEPENDS ${OCRE_INPUT_FILE}
        COMMENT "Generating C header from ${OCRE_INPUT_FILE}"
    )

    add_definitions(-DHAS_GENERATED_INPUT)

    add_custom_target(generate_ocre_file DEPENDS ${CMAKE_CURRENT_LIST_DIR}/src/ocre/ocre_input_file.g)
endif()
