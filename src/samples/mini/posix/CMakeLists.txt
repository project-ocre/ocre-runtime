cmake_minimum_required(VERSION 3.20.0)

project (ocre_sample_mini)

include(../input_file.cmake)

add_executable(ocre_mini
    ../main.c
    input_file.c
)

target_link_libraries(ocre_mini
    PUBLIC
    OcreCore
)

target_link_libraries(ocre_mini
    PRIVATE
    -lpthread
)
