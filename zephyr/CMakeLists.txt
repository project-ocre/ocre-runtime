# This file allows OCRE to be included as a Zephyr module.

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

set (WAMR_BUILD_PLATFORM "zephyr")

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_C_STANDARD 99)
set(CMAKE_CXX_STANDARD 17)

include (wamr.cmake)

add_subdirectory(../src/ocre "ocre-core")
add_subdirectory(../src/runtime "ocre-runtime")
add_subdirectory(../src/runtime/wamr "ocre-runtime-wamr")
add_subdirectory(../src/platform/zephyr "ocre-zephyr")
if(CONFIG_OCRE_SHELL)
    message(STATUS "Shell is enabled")
    add_subdirectory(../src/shell "ocre-shell")
endif()

if(CONFIG_OCRE_STORAGE_PARTITION)
    include(storage_partition.cmake)
endif()
