# @copyright Copyright (c) contributors to Project Ocre,
# which has been established as Project Ocre a Series of LF Projects, LLC
#
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

project (ocre_supervisor)

set(THREADS_PREFER_PTHREAD_FLAG TRUE)
find_package(Threads)

add_executable(ocre_cli
    ocre.c
    download.c
)

target_link_libraries(ocre_cli
    PRIVATE
    OcreShell
)

add_library(OcreClient)

target_sources(OcreClient
    PRIVATE
    client/client.c
    client/context.c
    zcbor_helpers.c
    zcbor/src/zcbor_encode.c
    zcbor/src/zcbor_decode.c
    zcbor/src/zcbor_common.c
)

target_include_directories(OcreClient
    PUBLIC ../../../ocre/include
    PUBLIC ../../../runtime/include
    PRIVATE zcbor/include
    PRIVATE .
)


target_link_libraries(OcreClient
    PUBLIC
    OcreCommon
)

# target_link_libraries(OcreCore PUBLIC
#     OcrePlatform
#     OcreRuntime
#     OcreRuntimeWamr
# )

# target_include_directories(OcreCore
#     PRIVATE
#     ${CMAKE_CURRENT_BINARY_DIR}/include
#     PUBLIC
#     include
# )

# include (../../cmake/state_information.cmake)

# if(OCRE_IMAGES)
#     add_dependencies(OcreCore ${OCRE_IMAGES})
# endif()

# install(TARGETS OcreCore DESTINATION lib)
# install(FILES include/ocre/container.h DESTINATION include/ocre)
# install(FILES include/ocre/library.h DESTINATION include/ocre)
# install(FILES include/ocre/context.h DESTINATION include/ocre)
# install(FILES include/ocre/ocre.h DESTINATION include/ocre)

# ######

target_link_libraries(ocre_cli
    PUBLIC
    OcreClient
    Threads::Threads
)

# target_include_directories(ocre_cli PRIVATE ../../../ocre/include)
# target_include_directories(ocre_cli PRIVATE ../../../runtime/include)
# target_include_directories(ocre_cli PRIVATE ../../../shell/include)


# target_include_directories(ocre_cli PRIVATE zcbor/include)

# target_link_libraries(OcreShell
#     PUBLIC
#     OcreCore
# )




# add_definitions(-DZCBOR_VERBOSE)

# add_definitions(-DZCBOR_CANONICAL)
add_subdirectory(server)
