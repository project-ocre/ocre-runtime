cmake_minimum_required(VERSION 3.20.0)

project (ocre_sample_mini)

add_subdirectory(../../.. mybin)

add_executable(ocre main.c)

target_link_libraries(ocre PUBLIC OcreComponent)

target_link_libraries(ocre PRIVATE -lpthread)
