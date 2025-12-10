cmake_minimum_required(VERSION 3.20.0)

set(OCRE_DEFAULT_WORKING_DIRECTORY "/var/lib/ocre")

project (ocre_sample_demo)

add_executable(ocre_demo
    ../main.c
)

target_link_libraries(ocre_demo
    PUBLIC
    OcreCore
)

target_link_libraries(ocre_demo
    PRIVATE
    -lpthread
)

add_dependencies(ocre_demo
    OcreSampleContainers
    OcrePreloadedImages
)
