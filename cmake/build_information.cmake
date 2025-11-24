execute_process(
    COMMAND git describe --long --dirty --always
    WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/..
    OUTPUT_VARIABLE RAW_GIT_VERSION
    OUTPUT_STRIP_TRAILING_WHITESPACE
)
string(REPLACE "-dirty" "+" BUILD_INFO ${RAW_GIT_VERSION})
string(TIMESTAMP BUILD_DATE "%d-%m-%Y, %H:%M" UTC )
cmake_host_system_information(RESULT BUILD_MACHINE QUERY HOSTNAME)
string(PREPEND BUILD_INFO dev-)
string(REPLACE "-" ";" GIT_INFO_LIST ${RAW_GIT_VERSION})
list(GET GIT_INFO_LIST 0 VERSION_NUMBER)

message(STATUS "OCRE build date: " ${BUILD_DATE})
message(STATUS "OCRE version number: " ${VERSION_NUMBER})
