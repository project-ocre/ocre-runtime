cmake_minimum_required(VERSION 3.20.0)

project(ocre LANGUAGES C ASM)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_C_STANDARD 99)
set(CMAKE_CXX_STANDARD 17)

include (cmake/build_information.cmake)
include (cmake/wamr.cmake)

add_subdirectory(src/messaging)
add_subdirectory(src/ocre)
add_subdirectory(src/ocre/utils/c-smf)
add_subdirectory(src/shared/platform/posix)

add_subdirectory(src/samples-mini/posix)
