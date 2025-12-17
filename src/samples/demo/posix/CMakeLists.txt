cmake_minimum_required(VERSION 3.20.0)

project (ocre_sample_demo)

add_executable(ocre_demo
    ../main.c
)

target_link_libraries(ocre_demo
    PUBLIC
    OcreCore
)

set(THREADS_PREFER_PTHREAD_FLAG TRUE)
find_package(Threads)
target_link_libraries(ocre_demo PRIVATE Threads::Threads)
