# Check if we are in a git repository
execute_process(
    COMMAND git rev-parse --git-dir
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    OUTPUT_VARIABLE GIT_DIR
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_QUIET
)

if(GIT_DIR)
    message(STATUS "Detected git repository. Automatic commit ID tracking is enabled.")

    # Generate commit ID header file in source dir if we are in a git repository
    add_custom_command(
        OUTPUT commit_id.h
        COMMAND sh -c "echo '/* Auto-generated file. DO NOT EDIT */' > commit_id.h"
        COMMAND sh -c "echo \"#define GIT_COMMIT_ID \\\"$(git describe --always --abbrev=0 --dirty)\\\"\" >> commit_id.h"
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        VERBATIM
        DEPENDS always_rebuild
    )
endif()

file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/include/ocre)

# Generate build info header file in binary dir for every build
add_custom_command(
    OUTPUT include/ocre/build_info.h
    COMMAND sh -c "echo \"#define OCRE_BUILD_HOST_INFO \\\"$ENV{USER} @ $(uname -a) - $(date +'%Y-%m-%d %H:%M:%S %Z')\\\"\"" > include/ocre/build_info.h
    VERBATIM
    DEPENDS always_rebuild
)

# dummy command used to make the custom commands be re-run on every build (not just configure)
add_custom_command(
    OUTPUT always_rebuild
    COMMAND cmake -E echo
)

add_library(OcreCore)

# if we are not in a git repository, just fail automatically if we are missing commit_id.h
target_sources(OcreCore
    PRIVATE
    container.c
    context.c
    unique_random_id.c
    ocre.c
    util/string_array.c
    util/rm_rf.c
    commit_id.h
    include/ocre/build_info.h
)

target_link_libraries(OcreCore PUBLIC
    OcrePlatform
    OcreRuntime
    OcreRuntimeWamr
)

target_include_directories(OcreCore
    PRIVATE
    ${CMAKE_CURRENT_BINARY_DIR}/include
    PUBLIC
    include
)
