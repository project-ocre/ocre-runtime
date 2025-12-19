# @copyright Copyright (c) contributors to Project Ocre,
# which has been established as Project Ocre a Series of LF Projects, LLC
#
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/var/lib/ocre/images)
file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/var/lib/ocre/containers)

if(OCRE_SDK_PRELOADED_IMAGES)
    include(ExternalProject)
    ExternalProject_Add(
        OcreSampleContainers
        PREFIX "${CMAKE_CURRENT_BINARY_DIR}/OcreSampleContainers"
        BINARY_DIR "${CMAKE_CURRENT_BINARY_DIR}/OcreSampleContainers/build"
        BUILD_ALWAYS TRUE
        INSTALL_COMMAND ""
        SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}/../ocre-sdk"
        CMAKE_ARGS "-DWAMR_ROOT_DIR=${CMAKE_CURRENT_LIST_DIR}/../wasm-micro-runtime" "-DCMAKE_VERBOSE_MAKEFILE=ON"
    )
endif()

if (OCRE_INPUT_FILE_NAME)
    message(STATUS "Adding user provided '${OCRE_INPUT_FILE_NAME}' to preloaded images")
    list(APPEND OCRE_PRELOADED_IMAGES ${OCRE_INPUT_FILE_NAME})
endif()

# populate SDK provided sample images
foreach(image IN ITEMS ${OCRE_SDK_PRELOADED_IMAGES})
    message(STATUS "Adding sdk sample '${image}' to preloaded images")
    add_custom_target(${image}
        COMMAND ${CMAKE_COMMAND} -E copy
            ${CMAKE_CURRENT_BINARY_DIR}/OcreSampleContainers/build/${image}
            ${CMAKE_CURRENT_BINARY_DIR}/var/lib/ocre/images/${image}
        DEPENDS OcreSampleContainers
    )
    list(APPEND OCRE_IMAGES ${image})
endforeach()

# populate user provided sample files
foreach(file IN ITEMS ${OCRE_PRELOADED_IMAGES})
    message(STATUS "Adding user provided '${file}' to preloaded images")
    cmake_path(GET file FILENAME image)
    add_custom_target(${image}
        COMMAND ${CMAKE_COMMAND} -E copy
            ${file}
            ${CMAKE_CURRENT_BINARY_DIR}/var/lib/ocre/images/${image}
    )
    list(APPEND OCRE_IMAGES ${image})
endforeach()
