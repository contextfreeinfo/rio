cmake_minimum_required(VERSION 3.13)

project(rio)

if(CMAKE_C_COMPILER_ID MATCHES "GNU" OR CMAKE_C_COMPILER_ID MATCHES "Clang")
    add_compile_options(-Wall -Wextra -Wpedantic -pedantic-errors -Werror)
endif()

add_executable(rio
    main.c
    engine.c
    lex.c
    parse.c
    util.c
    gen-aarch64.c
    sys-std.c
)
