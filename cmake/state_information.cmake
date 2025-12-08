cmake_minimum_required(VERSION 3.20.0)

file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/var/lib/ocre/images)
file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/var/lib/ocre/volumes)
file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/var/lib/ocre/containers)

if (OCRE_INPUT_FILE_NAME)
    message(DEPRECATION "Adding '${OCRE_INPUT_FILE_NAME}' to preloaded images. Please use OCRE_PRELOADED_IMAGES instead.")
    list(APPEND OCRE_PRELOADED_IMAGES ${OCRE_INPUT_FILE_NAME})
endif()

foreach(image IN ITEMS ${OCRE_PRELOADED_IMAGES})
    message(STATUS "Adding '${image}' to preloaded images")
    file(COPY ${image} DESTINATION var/lib/ocre/images/)
    cmake_path(GET image FILENAME filename)
    list(APPEND OCRE_IMAGES ${CMAKE_CURRENT_BINARY_DIR}/var/lib/ocre/images/${filename})
endforeach()
