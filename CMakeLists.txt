cmake_minimum_required(VERSION 3.10)
project(nib C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Release)
endif()

add_library(nibcore STATIC src/nib.c)
target_include_directories(nibcore PUBLIC src)
if(MSVC)
  target_compile_options(nibcore PRIVATE /W4 /O2 /wd4324)
else()
  target_compile_options(nibcore PRIVATE -Wall -Wextra -O2)
  target_link_libraries(nibcore PUBLIC m)
endif()

add_executable(nib src/main.c)
target_link_libraries(nib PRIVATE nibcore)

enable_testing()
add_test(NAME nib_tests COMMAND nib ${CMAKE_SOURCE_DIR}/tests/test.nib)
set_tests_properties(nib_tests PROPERTIES FAIL_REGULAR_EXPRESSION "FAIL")
