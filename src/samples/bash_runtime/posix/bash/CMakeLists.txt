cmake_minimum_required(VERSION 3.20.0)

set(OCRE_DEFAULT_WORKING_DIRECTORY "/var/lib/ocre")

add_library(OcreRuntimeBash)

target_sources(OcreRuntimeBash PRIVATE
    bash.c
)

target_include_directories(OcreRuntimeBash PUBLIC
    include
)

target_link_libraries(OcreRuntimeBash PUBLIC
    OcrePlatform
    OcreRuntime
)
