cmake_minimum_required(VERSION 3.20.0)

# Allow user to set CMAKE_SYSTEM_NAME via -DCMAKE_SYSTEM_NAME=Zephyr or Linux
if(NOT DEFINED TARGET_PLATFORM_NAME)
    set(TARGET_PLATFORM_NAME "Zephyr")
endif()

if(TARGET_PLATFORM_NAME STREQUAL "Linux")
    project(ocre LANGUAGES C ASM)
elseif(TARGET_PLATFORM_NAME STREQUAL "Zephyr")
    find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
    project(ocre VERSION ${APP_VERSION_MAJOR}.${APP_VERSION_MINOR}.${APP_PATCHLEVEL}.${APP_VERSION_TWEAK} LANGUAGES C ASM)
else()
    message(FATAL_ERROR "Unsupported TARGET_PLATFORM_NAME: ${TARGET_PLATFORM_NAME}")
endif()

# ----------- COMMON SECTION -----------
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_C_STANDARD 99)
set(CMAKE_CXX_STANDARD 17)

# Version and build info (common)
execute_process(
    COMMAND git describe --long --dirty --always
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE RAW_GIT_VERSION
    OUTPUT_STRIP_TRAILING_WHITESPACE
)
string(REPLACE "-dirty" "+" BUILD_INFO ${RAW_GIT_VERSION})
string(TIMESTAMP BUILD_DATE "%d-%m-%Y, %H:%M" UTC )
cmake_host_system_information(RESULT BUILD_MACHINE QUERY HOSTNAME)
string(PREPEND BUILD_INFO dev-)
string(REPLACE "-" ";" GIT_INFO_LIST ${RAW_GIT_VERSION})
list(GET GIT_INFO_LIST 0 VERSION_NUMBER)

# Message generation (common)
set(OCRE_ROOT_DIR ${CMAKE_CURRENT_LIST_DIR})
set(MSG_INPUT_FILES ${OCRE_ROOT_DIR}/src/ocre/components/container_supervisor/component_supervisor.yaml )
set(MSG_GENERATED_FILE ${CMAKE_CURRENT_LIST_DIR}/src/messaging/messages.g)

# Zephyr toolchain usually provides PYTHON_EXECUTABLE
if(NOT DEFINED PYTHON_EXECUTABLE)
    find_package(Python3 QUIET REQUIRED Interpreter Development)
    if(NOT Python3_FOUND)
        message(FATAL_ERROR "Python3 interpreter not found. Please install Python3 or set PYTHON_EXECUTABLE to the path of your Python interpreter.")
    endif()
    set(PYTHON_EXECUTABLE ${Python3_EXECUTABLE})
endif()

add_custom_command(
    OUTPUT ${MSG_GENERATED_FILE}
    COMMAND ${PYTHON_EXECUTABLE} ${OCRE_ROOT_DIR}/tools/automsg ${MSG_INPUT_FILES} ${MSG_GENERATED_FILE}
    DEPENDS ${MSG_INPUT_FILES}
    COMMENT "Running Python script to generate ${MSG_GENERATED_FILE}"
)
add_custom_target(generate_messages DEPENDS ${MSG_GENERATED_FILE})

# Include and process WASM binaries
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
include(embed_wasm_binary)
process_wasm_binaries()

if(TARGET_PLATFORM_NAME STREQUAL "Linux")
    include(${CMAKE_CURRENT_LIST_DIR}/src/shared/platform/posix/ocre_internal.cmake)
elseif(TARGET_PLATFORM_NAME STREQUAL "Zephyr")
    include(${CMAKE_CURRENT_LIST_DIR}/src/shared/platform/zephyr/ocre_internal.cmake)
endif()
