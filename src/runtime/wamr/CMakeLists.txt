add_library(OcreRuntimeWamr)

target_sources(OcreRuntimeWamr
    PRIVATE
    wamr.c
)

target_include_directories(OcreRuntimeWamr
    PUBLIC
    include
)

target_link_libraries(OcreRuntimeWamr
    PUBLIC
    OcreRuntime
    OcrePlatform
    OcreRuntimeAPI
    vmlib
)

add_subdirectory(ocre_api)
