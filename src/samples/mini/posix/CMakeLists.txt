cmake_minimum_required(VERSION 3.20.0)


project (ocre_sample_mini)

add_executable(ocre_mini
    ../main.c
)

target_link_libraries(ocre_mini
    PUBLIC
    OcreCore
)

target_link_libraries(ocre_mini
    PRIVATE
    -lpthread
)
