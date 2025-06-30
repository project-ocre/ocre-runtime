cmake_minimum_required(VERSION 3.20)
project(smf LANGUAGES C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

option(BUILD_TESTS "Build test program" OFF)

add_subdirectory(smf)

if(BUILD_TESTS)
    add_subdirectory(test)
    target_link_libraries(test smf)
endif(BUILD_TESTS)
