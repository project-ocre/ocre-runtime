cmake_minimum_required(VERSION 3.20)

add_library(OcreRuntime
    INTERFACE
    include/ocre/runtime/vtable.h
)

target_include_directories(OcreRuntime
    INTERFACE
    include
)
