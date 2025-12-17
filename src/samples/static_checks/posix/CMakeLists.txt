cmake_minimum_required(VERSION 3.20.0)

project (ocre_cmd)

add_executable(ocre_cmd
    ocre.c
    download_file.c
)

target_link_libraries(ocre_cmd
    PUBLIC
    OcreCore
    OcreShell
)

target_link_libraries(OcreShell
    PUBLIC
    OcreCore
)

set(THREADS_PREFER_PTHREAD_FLAG TRUE)
find_package(Threads)
target_link_libraries(ocre_cmd PRIVATE Threads::Threads)
