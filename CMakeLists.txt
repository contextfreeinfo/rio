cmake_minimum_required(VERSION 3.10)

# On Windows with Visual Studio, prefer the ClangCL toolset when it's installed: clang supports
# computed-goto dispatch (MSVC only gets the slower switch loop). -T <toolset> still wins.
option(RIO_PREFER_CLANGCL "Use the ClangCL toolset with Visual Studio generators if installed" ON)
set(RIO_CLANGCL_COMPONENT "Microsoft.VisualStudio.Component.VC.Llvm.Clang;Microsoft.VisualStudio.Component.VC.Llvm.ClangToolset"
    CACHE STRING "VS components that provide the ClangCL toolset (all required)")
mark_as_advanced(RIO_CLANGCL_COMPONENT)
if(RIO_PREFER_CLANGCL AND CMAKE_GENERATOR MATCHES "^Visual Studio" AND NOT CMAKE_GENERATOR_TOOLSET)
  set(_rio_pf86 "ProgramFiles(x86)")
  set(_rio_vswhere "$ENV{${_rio_pf86}}/Microsoft Visual Studio/Installer/vswhere.exe")
  set(_rio_clangcl "")
  if(EXISTS "${_rio_vswhere}")
    execute_process(COMMAND "${_rio_vswhere}" -latest -products * -requires ${RIO_CLANGCL_COMPONENT}
                    -property installationPath OUTPUT_VARIABLE _rio_clangcl OUTPUT_STRIP_TRAILING_WHITESPACE)
  endif()
  if(_rio_clangcl)
    set(CMAKE_GENERATOR_TOOLSET ClangCL)
    message(STATUS "rio: using ClangCL toolset (computed-goto dispatch)")
  else()
    message(STATUS "rio: ClangCL not installed; using MSVC (switch dispatch)")
  endif()
endif()

project(rio C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Release)
endif()

# the embeddable core: compiler + vm, no allocation, no stdio
add_library(riocore STATIC src/rio.c)
target_include_directories(riocore PUBLIC src)
if(MSVC)
  target_compile_options(riocore PRIVATE /W4 /O2 /wd4324)
else()
  target_compile_options(riocore PRIVATE -Wall -Wextra -O2)
  target_link_libraries(riocore PUBLIC m)
endif()

# cli: run scripts, or translate them to C with -c
add_executable(rio src/main.c src/aot.c)
target_link_libraries(rio PRIVATE riocore)

enable_testing()
add_test(NAME rio_tests COMMAND rio ${CMAKE_SOURCE_DIR}/tests/test.rio)
set_tests_properties(rio_tests PROPERTIES FAIL_REGULAR_EXPRESSION "FAIL")

# -D overrides of top-level constants
add_test(NAME rio_defines_default COMMAND rio ${CMAKE_SOURCE_DIR}/tests/defines.rio)
set_tests_properties(rio_defines_default PROPERTIES
  PASS_REGULAR_EXPRESSION "^N 10 len 10 total 45 SCALE 3.0 NAME default")
add_test(NAME rio_defines_override COMMAND rio -D N=20 -DSCALE=3 -D NAME=custom -D VERBOSE ${CMAKE_SOURCE_DIR}/tests/defines.rio)
set_tests_properties(rio_defines_override PROPERTIES
  PASS_REGULAR_EXPRESSION "verbose on[\r\n]+N 20 len 20 total 190 SCALE 6.0 NAME custom")

# the same suite compiled ahead of time to C
add_custom_command(OUTPUT ${CMAKE_BINARY_DIR}/test_aot.c
  COMMAND rio -c ${CMAKE_BINARY_DIR}/test_aot.c ${CMAKE_SOURCE_DIR}/tests/test.rio
  DEPENDS rio ${CMAKE_SOURCE_DIR}/tests/test.rio)
add_executable(test_aot ${CMAKE_BINARY_DIR}/test_aot.c)
if(NOT MSVC)
  target_link_libraries(test_aot PRIVATE m)
endif()
add_test(NAME rio_aot_tests COMMAND test_aot)
set_tests_properties(rio_aot_tests PROPERTIES FAIL_REGULAR_EXPRESSION "FAIL" PASS_REGULAR_EXPRESSION "all tests passed")
set_tests_properties(rio_tests PROPERTIES PASS_REGULAR_EXPRESSION "all tests passed")
