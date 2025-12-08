# This file is to be used to build a zephyr firmware including the ocre demo
# automatically uses the ocre module in the parent directory

cmake_minimum_required(VERSION 3.20.0)

# comment out to use the ocre module provided by zephyr
list(APPEND ZEPHYR_EXTRA_MODULES ${CMAKE_CURRENT_LIST_DIR}/../../../..)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project (ocre_sample_supervisor)

target_sources(app
    PRIVATE
    service.c
    shell.c
)

target_link_libraries(app
    PUBLIC
    OcreCore
    OcreShell
)

target_link_libraries(OcreShell
    PUBLIC
    zephyr_interface
    OcreCore
)
