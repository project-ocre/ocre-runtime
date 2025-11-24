# This file allows OCRE to be included as a Zephyr module.

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

set (WAMR_BUILD_PLATFORM "zephyr")

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_C_STANDARD 99)
set(CMAKE_CXX_STANDARD 17)

include (../cmake/build_information.cmake)
include (wamr.cmake)

add_subdirectory(../src/messaging OcreMessaging)
add_subdirectory(../src/ocre OcreComponent)
add_subdirectory(../src/shared/platform/zephyr OcreLibrary)
