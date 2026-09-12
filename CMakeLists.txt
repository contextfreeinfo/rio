cmake_minimum_required(VERSION 3.13)

project(rio)

include(library.cmake)

add_executable(rio
    src/main.c
    src/sys.c
)

target_link_libraries(rio PRIVATE rio-lib)

if(CMAKE_C_COMPILER_ID MATCHES "GNU" OR CMAKE_C_COMPILER_ID MATCHES "Clang")
    target_compile_options(rio PRIVATE
        -Wall -Wextra -Wpedantic -pedantic-errors -Werror
    )
endif()

