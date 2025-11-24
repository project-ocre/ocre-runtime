cmake_minimum_required(VERSION 3.20.0)

add_library(OcreLibrary
    core_fs.c
    core_thread.c
    core_mutex.c
    core_mq.c
    core_misc.c
    core_memory.c
    core_timer.c
    core_eventq.c
)

target_include_directories(OcreLibrary PUBLIC ..)
target_link_libraries(OcreLibrary PUBLIC smf zephyr_interface OcreComponent LITTLEFS)
