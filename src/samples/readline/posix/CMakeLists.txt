cmake_minimum_required(VERSION 3.20.0)

project (ocre_sample_supervisor)

add_executable(ocred
    ocred.c
)

target_link_libraries(ocred
    PUBLIC
    OcreCore
)

target_link_libraries(ocred
    PRIVATE
    -lpthread
)

add_executable(ocre
    ocre.c
)

target_link_libraries(ocre
    PUBLIC
    OcreCore # just link for now, to validate the build
    OcreShell
)

target_link_libraries(OcreShell
    PUBLIC
    OcreCore
)


target_link_libraries(ocre
    PRIVATE
    -lpthread
)
