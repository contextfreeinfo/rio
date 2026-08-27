cmake_minimum_required(VERSION 3.13)

project(rio)

add_executable(rio
    src/main.c
    src/engine.c
    src/lex.c
    src/parse.c
    src/util.c
    src/gen.c
    src/sys-std.c
)

if(CMAKE_C_COMPILER_ID MATCHES "GNU" OR CMAKE_C_COMPILER_ID MATCHES "Clang")
    target_compile_options(rio PRIVATE
        -Wall -Wextra -Wpedantic -pedantic-errors -Werror
    )
endif()

