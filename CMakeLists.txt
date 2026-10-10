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

option(RIO_EXPORTS_ALL "The host can look up every top-level proc and global, not just main and name*" OFF)
option(RIO_SANITIZE "Build everything with AddressSanitizer and UndefinedBehaviorSanitizer (GCC/clang)" OFF)
if(RIO_SANITIZE AND NOT MSVC)
  # any finding aborts the test that hit it, so ctest fails
  add_compile_options(-fsanitize=address,undefined -fno-omit-frame-pointer -fno-sanitize-recover=all -g)
  add_link_options(-fsanitize=address,undefined)
endif()

# the embeddable core: compiler + vm, no allocation, no stdio
add_library(riocore STATIC src/rio.c)
target_include_directories(riocore PUBLIC src)
if(RIO_EXPORTS_ALL)
  target_compile_definitions(riocore PUBLIC RIO_EXPORTS_ALL)
endif()
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

# errors: kind, position and message through the API, and as the CLI prints them
add_executable(rio_errors tests/errors.c)
target_link_libraries(rio_errors PRIVATE riocore)
add_test(NAME rio_error_info COMMAND rio_errors)
set_tests_properties(rio_error_info PROPERTIES PASS_REGULAR_EXPRESSION "all [0-9]+ error tests passed")
add_test(NAME rio_cli_compile_error COMMAND rio ${CMAKE_SOURCE_DIR}/tests/bad.rio)
set_tests_properties(rio_cli_compile_error PROPERTIES PASS_REGULAR_EXPRESSION "bad\\.rio:3:10: undefined name near 'nope'")
add_test(NAME rio_cli_runtime_error COMMAND rio ${CMAKE_SOURCE_DIR}/tests/runtime_error.rio)
set_tests_properties(rio_cli_runtime_error PROPERTIES PASS_REGULAR_EXPRESSION "runtime_error\\.rio:4: runtime error: index out of bounds")

# REPL sessions and break, through the API
add_executable(rio_repl_test tests/repl.c)
target_link_libraries(rio_repl_test PRIVATE riocore)
add_test(NAME rio_repl COMMAND rio_repl_test)
set_tests_properties(rio_repl PROPERTIES PASS_REGULAR_EXPRESSION "all [0-9]+ repl tests passed")

# modules: real files through the CLI loader, and edge cases through the API
add_test(NAME rio_modules COMMAND rio -L ${CMAKE_SOURCE_DIR}/tests/modules/lib ${CMAKE_SOURCE_DIR}/tests/modules/main.rio)
set_tests_properties(rio_modules PROPERTIES PASS_REGULAR_EXPRESSION "all module tests passed" FAIL_REGULAR_EXPRESSION "FAIL")
add_executable(rio_modules_test tests/modules.c)
target_link_libraries(rio_modules_test PRIVATE riocore)
add_test(NAME rio_modules_api COMMAND rio_modules_test)
set_tests_properties(rio_modules_api PROPERTIES PASS_REGULAR_EXPRESSION "all [0-9]+ module tests passed")

# host lookups by name: main and name* by default, every top-level name with RIO_EXPORTS_ALL
add_executable(rio_exports_test tests/exports.c)
target_link_libraries(rio_exports_test PRIVATE riocore)
add_test(NAME rio_exports COMMAND rio_exports_test)
set_tests_properties(rio_exports PROPERTIES PASS_REGULAR_EXPRESSION "all export tests passed")
add_executable(rio_exports_all_test tests/exports.c src/rio.c)
target_include_directories(rio_exports_all_test PRIVATE src)
target_compile_definitions(rio_exports_all_test PRIVATE RIO_EXPORTS_ALL)
if(NOT MSVC)
  target_link_libraries(rio_exports_all_test PRIVATE m)
endif()
add_test(NAME rio_exports_all COMMAND rio_exports_all_test)
set_tests_properties(rio_exports_all PROPERTIES PASS_REGULAR_EXPRESSION "all export tests passed .every name.")

# memory report
add_test(NAME rio_memory_report COMMAND rio -m ${CMAKE_SOURCE_DIR}/tests/test.rio)
set_tests_properties(rio_memory_report PROPERTIES PASS_REGULAR_EXPRESSION "heaviest call chain: [0-9]+ bytes")

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

# generated C reports the same line for a runtime error
add_custom_command(OUTPUT ${CMAKE_BINARY_DIR}/runtime_error_aot.c
  COMMAND rio -c ${CMAKE_BINARY_DIR}/runtime_error_aot.c ${CMAKE_SOURCE_DIR}/tests/runtime_error.rio
  DEPENDS rio ${CMAKE_SOURCE_DIR}/tests/runtime_error.rio)
add_executable(runtime_error_aot ${CMAKE_BINARY_DIR}/runtime_error_aot.c)
if(NOT MSVC)
  target_link_libraries(runtime_error_aot PRIVATE m)
endif()
add_test(NAME rio_aot_runtime_error COMMAND runtime_error_aot)
# an error late in a line that becomes many statements still reports that line
add_test(NAME rio_cli_runtime_error_late COMMAND rio ${CMAKE_SOURCE_DIR}/tests/runtime_error_late.rio)
set_tests_properties(rio_cli_runtime_error_late PROPERTIES PASS_REGULAR_EXPRESSION "runtime_error_late\\.rio:6: runtime error: index out of bounds")
add_custom_command(OUTPUT ${CMAKE_BINARY_DIR}/runtime_error_late_aot.c
  COMMAND rio -c ${CMAKE_BINARY_DIR}/runtime_error_late_aot.c ${CMAKE_SOURCE_DIR}/tests/runtime_error_late.rio
  DEPENDS rio ${CMAKE_SOURCE_DIR}/tests/runtime_error_late.rio)
add_executable(runtime_error_late_aot ${CMAKE_BINARY_DIR}/runtime_error_late_aot.c)
if(NOT MSVC)
  target_link_libraries(runtime_error_late_aot PRIVATE m)
endif()
add_test(NAME rio_aot_runtime_error_late COMMAND runtime_error_late_aot)
set_tests_properties(rio_aot_runtime_error_late PROPERTIES PASS_REGULAR_EXPRESSION "^6: runtime error: index out of bounds")
set_tests_properties(rio_aot_runtime_error PROPERTIES PASS_REGULAR_EXPRESSION "^4: runtime error: index out of bounds")
