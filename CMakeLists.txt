cmake_minimum_required(VERSION 3.13)

project(rio)

if(CMAKE_C_COMPILER_ID MATCHES "GNU" OR CMAKE_C_COMPILER_ID MATCHES "Clang")
    add_compile_options(-Wall -Wextra -Wpedantic -pedantic-errors -Werror)
endif()

add_executable(rio
    src/main.c
    src/engine.c
    src/lex.c
    src/parse.c
    src/util.c
    src/gen-aarch64.c
    src/sys-std.c
)
