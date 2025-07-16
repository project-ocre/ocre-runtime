add_executable(test
    test.c
)

target_compile_options(test PRIVATE
    -fdiagnostics-color=always
    -pedantic-errors
    -Wall
    -Wextra
    -Wconversion
    -Wsign-conversion
    -Werror
)
