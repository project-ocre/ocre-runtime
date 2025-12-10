cmake_minimum_required(VERSION 3.20.0)

file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/var/lib/ocre/images)
file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/var/lib/ocre/volumes)
file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/var/lib/ocre/containers)

if (OCRE_INPUT_FILE_NAME)
    message(DEPRECATION "Adding '${OCRE_INPUT_FILE_NAME}' to preloaded images. Please use OCRE_PRELOADED_IMAGES instead.")
    list(APPEND OCRE_PRELOADED_IMAGES ${OCRE_INPUT_FILE_NAME})
endif()

# populate user provided preloaded images
foreach(image IN ITEMS ${OCRE_PRELOADED_IMAGES})
    message(STATUS "Adding '${image}' to preloaded images")
    file(COPY ${image} DESTINATION var/lib/ocre/images/)
    cmake_path(GET image FILENAME filename)
    list(APPEND OCRE_IMAGES ${CMAKE_CURRENT_BINARY_DIR}/var/lib/ocre/images/${filename})
endforeach()

# populate SDK provided sample images
foreach(image IN ITEMS ${OCRE_SDK_PRELOADED_IMAGES})
    message(STATUS "Adding '${image}' to preloaded images")
    add_custom_command(
        OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/var/lib/ocre/images/${image}
        COMMAND ${CMAKE_COMMAND} -E copy
            ${OCRE_SAMPLE_CONTAINER_LOCATION}/${image}
            ${CMAKE_CURRENT_BINARY_DIR}/var/lib/ocre/images/${image}
        DEPENDS OcreSampleContainers
    )
    cmake_path(GET image FILENAME filename)
    list(APPEND OCRE_IMAGES ${CMAKE_CURRENT_BINARY_DIR}/var/lib/ocre/images/${filename})
endforeach()

add_custom_target(OcrePreloadedImages
    DEPENDS ${OCRE_IMAGES}
)
