# embed_wasm_binary.cmake
# Embeds a WASM binary file into the application as a linkable object

# Helper function to detect the correct objcopy format based on architecture
function(detect_objcopy_format OUT_FORMAT OUT_ARCH_NAME)
    set(FORMAT "")
    set(ARCH "unknown")

    if(CONFIG_ARM64 OR CONFIG_AARCH64)
        set(FORMAT "elf64-littleaarch64")
        set(ARCH "ARM64")
    elseif(CONFIG_ARM)
        set(FORMAT "elf32-littlearm")
        set(ARCH "ARM")
    elseif(CONFIG_RISCV)
        if(CONFIG_64BIT)
            set(FORMAT "elf64-littleriscv")
            set(ARCH "RISC-V 64")
        else()
            set(FORMAT "elf32-littleriscv")
            set(ARCH "RISC-V 32")
        endif()
    elseif(CONFIG_X86_64)
        set(FORMAT "elf64-x86-64")
        set(ARCH "x86-64")
    elseif(CONFIG_X86 OR CONFIG_ARCH_POSIX)
        # POSIX/native boards are typically x86 32-bit
        # Check if 64-bit is explicitly set
        if(CONFIG_64BIT OR CMAKE_SIZEOF_VOID_P EQUAL 8)
            set(FORMAT "elf64-x86-64")
            set(ARCH "x86-64")
        else()
            set(FORMAT "elf32-i386")
            set(ARCH "x86")
        endif()
    elseif(CONFIG_XTENSA)
        set(FORMAT "elf32-xtensa-le")
        set(ARCH "Xtensa")
    elseif(CONFIG_ARC)
        set(FORMAT "elf32-littlearc")
        set(ARCH "ARC")
    elseif(CONFIG_NIOS2)
        set(FORMAT "elf32-littlenios2")
        set(ARCH "NIOS2")
    else()
        # Fallback: try to use default
        set(FORMAT "default")
        set(ARCH "unknown")
        message(WARNING "Could not detect architecture for objcopy, using 'default' format")
    endif()

    # Return values to caller
    set(${OUT_FORMAT} ${FORMAT} PARENT_SCOPE)
    set(${OUT_ARCH_NAME} ${ARCH} PARENT_SCOPE)
endfunction()

# Main function to embed WASM binary
function(embed_wasm_binary)
    set(options "")
    set(oneValueArgs INPUT_FILE OUTPUT_NAME TARGET_NAME SYMBOL_NAME)
    set(multiValueArgs "")
    cmake_parse_arguments(WASM "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    if(NOT WASM_INPUT_FILE)
        message(FATAL_ERROR "embed_wasm_binary: INPUT_FILE is required")
    endif()

    if(NOT WASM_OUTPUT_NAME)
        set(WASM_OUTPUT_NAME "app.wasm")
    endif()

    if(NOT WASM_SYMBOL_NAME)
        get_filename_component(WASM_SYMBOL_NAME "${WASM_OUTPUT_NAME}" NAME_WE)
    endif()

    message(STATUS "Embedding WASM binary: ${WASM_INPUT_FILE}")

    # Stage the WASM file with a fixed name for consistent symbol names
    set(WASM_STAGED_FILE ${CMAKE_CURRENT_BINARY_DIR}/${WASM_OUTPUT_NAME})

    add_custom_command(
        OUTPUT ${WASM_STAGED_FILE}
        COMMAND ${CMAKE_COMMAND} -E copy ${WASM_INPUT_FILE} ${WASM_STAGED_FILE}
        DEPENDS ${WASM_INPUT_FILE}
        COMMENT "Staging WASM file: ${WASM_INPUT_FILE}"
    )

    # Detect the correct objcopy format for the target architecture
    detect_objcopy_format(OBJCOPY_FORMAT ARCH_NAME)

    message(STATUS "Detected architecture: ${ARCH_NAME}, using objcopy format: ${OBJCOPY_FORMAT}")

    # Convert WASM to object file (use symbol name for unique files)
    set(WASM_OBJECT_FILE ${CMAKE_CURRENT_BINARY_DIR}/wasm_${WASM_SYMBOL_NAME}.o)

    add_custom_command(
        OUTPUT ${WASM_OBJECT_FILE}
        COMMAND objcopy
            -I binary
            -O ${OBJCOPY_FORMAT}
            --rename-section .data=.rodata.wasm,alloc,load,readonly,data,contents
            ${WASM_STAGED_FILE}
            ${WASM_OBJECT_FILE}
        DEPENDS ${WASM_STAGED_FILE}
        COMMENT "Converting to object file (${OBJCOPY_FORMAT} for ${ARCH_NAME})"
    )

    # Create a custom target
    add_custom_target(${WASM_TARGET_NAME} DEPENDS ${WASM_OBJECT_FILE})

    # Export the object file path to parent scope with symbol name
    set(WASM_BINARY_OBJECT_${WASM_SYMBOL_NAME} ${WASM_OBJECT_FILE} PARENT_SCOPE)

    message(STATUS "WASM binary will be embedded as: ${WASM_OBJECT_FILE}")
endfunction()

# Generate a manifest header file using template and Python script
function(generate_wasm_manifest WASM_NAMES_LIST)
    if(NOT WASM_NAMES_LIST)
        message(WARNING "No WASM names provided for manifest generation")
        return()
    endif()

    set(TEMPLATE_FILE ${CMAKE_CURRENT_SOURCE_DIR}/src/ocre/wasm_manifest.h.in)
    set(MANIFEST_FILE ${CMAKE_CURRENT_BINARY_DIR}/wasm_manifest.h)

    # Find Python executable (reuse from earlier in CMakeLists)
    if(NOT DEFINED PYTHON_EXECUTABLE)
        find_package(Python3 QUIET REQUIRED Interpreter)
        set(PYTHON_EXECUTABLE ${Python3_EXECUTABLE})
    endif()

    # Generate manifest using Python script with template
    execute_process(
        COMMAND ${PYTHON_EXECUTABLE}
            ${CMAKE_CURRENT_SOURCE_DIR}/tools/generate_wasm_manifest.py
            ${WASM_NAMES_LIST}
            -t ${TEMPLATE_FILE}
            -o ${MANIFEST_FILE}
        RESULT_VARIABLE RESULT
        OUTPUT_VARIABLE OUTPUT
        ERROR_VARIABLE ERROR
    )

    if(NOT RESULT EQUAL 0)
        message(FATAL_ERROR "Failed to generate WASM manifest: ${ERROR}")
    endif()

    message(STATUS "${OUTPUT}")
endfunction()

# High-level function to process WASM files from build script
function(process_wasm_binaries)
    if(NOT DEFINED OCRE_WASM_FILES OR "${OCRE_WASM_FILES}" STREQUAL "")
        message(STATUS "No WASM files specified")
        return()
    endif()

    message(STATUS "Processing WASM files: ${OCRE_WASM_FILES}")

    set(WASM_NAMES "")

    foreach(WASM_PATH ${OCRE_WASM_FILES})
        if(EXISTS "${WASM_PATH}")
            get_filename_component(WASM_NAME "${WASM_PATH}" NAME_WE)
            message(STATUS "  → Embedding: ${WASM_NAME} from ${WASM_PATH}")

            embed_wasm_binary(
                INPUT_FILE "${WASM_PATH}"
                OUTPUT_NAME "${WASM_NAME}.wasm"
                TARGET_NAME "generate_${WASM_NAME}_wasm"
                SYMBOL_NAME "${WASM_NAME}"
            )

            list(APPEND WASM_NAMES ${WASM_NAME})
        else()
            message(WARNING "WASM file not found: ${WASM_PATH}")
        endif()
    endforeach()

    if(WASM_NAMES)
        # Export to parent scope and cache for use in other cmake files
        set(WASM_MANIFEST_ENTRIES ${WASM_NAMES} PARENT_SCOPE)
        set(WASM_MANIFEST_ENTRIES ${WASM_NAMES} CACHE INTERNAL "List of embedded WASM names")

        list(LENGTH WASM_NAMES WASM_COUNT)
        message(STATUS "Successfully configured ${WASM_COUNT} WASM file(s): ${WASM_NAMES}")

        # Generate the manifest header
        generate_wasm_manifest("${WASM_NAMES}")

        add_definitions(-DHAS_GENERATED_INPUT)
        add_definitions(-DHAS_WASM_MANIFEST)
    else()
        message(WARNING "No valid WASM files were processed")
    endif()
endfunction()
