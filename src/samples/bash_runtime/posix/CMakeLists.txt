cmake_minimum_required(VERSION 3.20.0)

project (ocre_sample_bash_runtime)

add_subdirectory(bash)

add_executable(ocre_bash
    main.c
)

target_link_libraries(ocre_bash
    PUBLIC
    OcreCore
    OcreRuntime
    OcreRuntimeBash
)

target_link_libraries(ocre_bash
    PRIVATE
    -lpthread
)
