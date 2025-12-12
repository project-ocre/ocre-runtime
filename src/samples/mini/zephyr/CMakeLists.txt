# This file is to be used to build a zephyr firmware including the ocre demo
# automatically uses the ocre module in the parent directory

cmake_minimum_required(VERSION 3.20.0)

# comment out to use the ocre module provided by Zephyr
list(APPEND ZEPHYR_EXTRA_MODULES ${CMAKE_CURRENT_LIST_DIR}/../../../..)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project (ocre_sample_mini)

include(../input_file.cmake)

target_sources(app
    PRIVATE
    ../main.c
    input_file.c
)

target_link_libraries(app
    PUBLIC
    OcreCore
)
