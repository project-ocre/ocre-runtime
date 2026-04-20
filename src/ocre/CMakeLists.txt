# @copyright Copyright (c) contributors to Project Ocre,
# which has been established as Project Ocre a Series of LF Projects, LLC
#
# SPDX-License-Identifier: Apache-2.0

add_library(OcreCore)

target_sources(OcreCore
    PRIVATE
    container.c
    context.c
    ocre.c
    util/rm_rf.c
    util/string_array.c
    util/unique_random_id.c
)

target_link_libraries(OcreCore
    PRIVATE
    uthash
    OcrePlatform
    OcreRuntimeWamr
    PUBLIC
    OcreRuntime
    OcreCommon
)

target_include_directories(OcreCore
    PRIVATE
    ${CMAKE_CURRENT_BINARY_DIR}/include
    PUBLIC
    include
)

include (../../cmake/state_information.cmake)

if(OCRE_IMAGES)
    add_dependencies(OcreCore ${OCRE_IMAGES})
endif()

install(TARGETS OcreCore DESTINATION lib)
install(FILES include/ocre/container.h DESTINATION include/ocre)
install(FILES include/ocre/library.h DESTINATION include/ocre)
install(FILES include/ocre/context.h DESTINATION include/ocre)
install(FILES include/ocre/ocre.h DESTINATION include/ocre)
