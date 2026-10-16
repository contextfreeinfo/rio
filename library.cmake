# The embeddable core (compiler + VM) as a CMake library, for projects that use rio:
#   include(path/to/rio/library.cmake)
#   target_link_libraries(myapp PRIVATE riocore)
# Optional, before linking: target_compile_definitions(riocore PUBLIC rio_SMALL) for MCU-sized fixed caps,
# or rio_NO_COMPILER for a VM that only runs images (the compiler's files then compile to nothing).
set(RIO_VM_SOURCES # what runs programs
  ${CMAKE_CURRENT_LIST_DIR}/src/common.c
  ${CMAKE_CURRENT_LIST_DIR}/src/image.c
  ${CMAKE_CURRENT_LIST_DIR}/src/runtime.c
  ${CMAKE_CURRENT_LIST_DIR}/src/vm.c)
set(RIO_COMPILER_SOURCES # what compiles them
  ${CMAKE_CURRENT_LIST_DIR}/src/builtins.c
  ${CMAKE_CURRENT_LIST_DIR}/src/codegen.c
  ${CMAKE_CURRENT_LIST_DIR}/src/compile.c
  ${CMAKE_CURRENT_LIST_DIR}/src/decls.c
  ${CMAKE_CURRENT_LIST_DIR}/src/expr.c
  ${CMAKE_CURRENT_LIST_DIR}/src/lexer.c
  ${CMAKE_CURRENT_LIST_DIR}/src/modules.c
  ${CMAKE_CURRENT_LIST_DIR}/src/operators.c
  ${CMAKE_CURRENT_LIST_DIR}/src/repl.c
  ${CMAKE_CURRENT_LIST_DIR}/src/statements.c
  ${CMAKE_CURRENT_LIST_DIR}/src/tables.c)
set(RIO_CORE_SOURCES ${RIO_VM_SOURCES} ${RIO_COMPILER_SOURCES}
  ${CMAKE_CURRENT_LIST_DIR}/src/jit.c) # jit.c: native loops, on ARM only
if(NOT TARGET riocore)
  add_library(riocore STATIC ${RIO_CORE_SOURCES})
  target_include_directories(riocore PUBLIC ${CMAKE_CURRENT_LIST_DIR}/src)
  if(NOT MSVC)
    target_link_libraries(riocore PUBLIC m)
  endif()
endif()
