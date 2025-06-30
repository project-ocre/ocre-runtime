add_library(smf SHARED
    smf.c
)

set_target_properties(smf PROPERTIES PUBLIC_HEADER smf.h)
target_include_directories(smf PUBLIC ..)

target_compile_options(smf PRIVATE
    -fdiagnostics-color=always
    -pedantic-errors
    -Wall
    -Wextra
    -Wconversion
    -Wsign-conversion
    -Werror
)
