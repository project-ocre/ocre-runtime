cmake_minimum_required(VERSION 3.20.0)

project(OcreMessaging)

set(MSG_INPUT_FILES ${CMAKE_CURRENT_SOURCE_DIR}/../ocre/components/container_supervisor/component_supervisor.yaml )

make_directory(${CMAKE_CURRENT_BINARY_DIR}/messaging)

set(MSG_GENERATED_FILE ${CMAKE_CURRENT_BINARY_DIR}/messaging/messages.g)

find_package(Python3 QUIET REQUIRED Interpreter Development)
if(NOT Python3_FOUND)
    message(FATAL_ERROR "Python3 interpreter not found. Please install Python3 or set PYTHON_EXECUTABLE to the path of your Python interpreter.")
endif()
message(STATUS "Using Python Executable: ${Python3_EXECUTABLE}")

add_custom_command(
    OUTPUT ${MSG_GENERATED_FILE}
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/../../tools/automsg ${MSG_INPUT_FILES} ${MSG_GENERATED_FILE}
    DEPENDS ${MSG_INPUT_FILES}
    COMMENT "Running Python script to generate ${MSG_GENERATED_FILE}"
    VERBATIM
)

add_library(OcreMessages INTERFACE ${MSG_GENERATED_FILE})
target_sources(OcreMessages
  INTERFACE ${MSG_GENERATED_FILE}
)

target_include_directories(OcreMessages INTERFACE ${CMAKE_CURRENT_BINARY_DIR} ..)
