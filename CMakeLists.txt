cmake_minimum_required(VERSION 3.10)

# On Windows with Visual Studio, prefer the ClangCL toolset when it's installed: clang supports
# computed-goto dispatch (MSVC only gets the slower switch loop). -T <toolset> still wins.
option(NIB_PREFER_CLANGCL "Use the ClangCL toolset with Visual Studio generators if installed" ON)
set(NIB_CLANGCL_COMPONENT "Microsoft.VisualStudio.Component.VC.Llvm.Clang;Microsoft.VisualStudio.Component.VC.Llvm.ClangToolset"
    CACHE STRING "VS components that provide the ClangCL toolset (all required)")
mark_as_advanced(NIB_CLANGCL_COMPONENT)
if(NIB_PREFER_CLANGCL AND CMAKE_GENERATOR MATCHES "^Visual Studio" AND NOT CMAKE_GENERATOR_TOOLSET)
  set(_nib_pf86 "ProgramFiles(x86)")
  set(_nib_vswhere "$ENV{${_nib_pf86}}/Microsoft Visual Studio/Installer/vswhere.exe")
  set(_nib_clangcl "")
  if(EXISTS "${_nib_vswhere}")
    execute_process(COMMAND "${_nib_vswhere}" -latest -products * -requires ${NIB_CLANGCL_COMPONENT}
                    -property installationPath OUTPUT_VARIABLE _nib_clangcl OUTPUT_STRIP_TRAILING_WHITESPACE)
  endif()
  if(_nib_clangcl)
    set(CMAKE_GENERATOR_TOOLSET ClangCL)
    message(STATUS "nib: using ClangCL toolset (computed-goto dispatch)")
  else()
    message(STATUS "nib: ClangCL not installed; using MSVC (switch dispatch)")
  endif()
endif()

project(nib C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Release)
endif()

# the embeddable core: compiler + vm, no allocation, no stdio
add_library(nibcore STATIC src/nib.c)
target_include_directories(nibcore PUBLIC src)
if(MSVC)
  target_compile_options(nibcore PRIVATE /W4 /O2 /wd4324)
else()
  target_compile_options(nibcore PRIVATE -Wall -Wextra -O2)
  target_link_libraries(nibcore PUBLIC m)
endif()

# cli: run scripts, or translate them to C with -c
add_executable(nib src/main.c src/aot.c)
target_link_libraries(nib PRIVATE nibcore)

enable_testing()
add_test(NAME nib_tests COMMAND nib ${CMAKE_SOURCE_DIR}/tests/test.nib)
set_tests_properties(nib_tests PROPERTIES FAIL_REGULAR_EXPRESSION "FAIL")

# the same suite compiled ahead of time to C
add_custom_command(OUTPUT ${CMAKE_BINARY_DIR}/test_aot.c
  COMMAND nib -c ${CMAKE_BINARY_DIR}/test_aot.c ${CMAKE_SOURCE_DIR}/tests/test.nib
  DEPENDS nib ${CMAKE_SOURCE_DIR}/tests/test.nib)
add_executable(test_aot ${CMAKE_BINARY_DIR}/test_aot.c)
if(NOT MSVC)
  target_link_libraries(test_aot PRIVATE m)
endif()
add_test(NAME nib_aot_tests COMMAND test_aot)
set_tests_properties(nib_aot_tests PROPERTIES FAIL_REGULAR_EXPRESSION "FAIL" PASS_REGULAR_EXPRESSION "all tests passed")
set_tests_properties(nib_tests PROPERTIES PASS_REGULAR_EXPRESSION "all tests passed")
