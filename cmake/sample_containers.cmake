include(ExternalProject)

ExternalProject_Add(
    OcreSampleContainers
    PREFIX "${CMAKE_CURRENT_BINARY_DIR}/OcreSampleContainers"
    BINARY_DIR "${CMAKE_CURRENT_BINARY_DIR}/OcreSampleContainers/build"
    BUILD_ALWAYS TRUE
    INSTALL_COMMAND ""
    SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}/../ocre-sdk"
    CMAKE_ARGS "-DWAMR_ROOT_DIR=${CMAKE_CURRENT_LIST_DIR}/../wasm-micro-runtime"
)

set(OCRE_SAMPLE_CONTAINER_LOCATION "${CMAKE_CURRENT_BINARY_DIR}/OcreSampleContainers/build")
