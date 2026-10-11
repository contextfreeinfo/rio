# The embeddable core (compiler + VM) as a CMake library, for projects that use rio:
#   include(path/to/rio/library.cmake)
#   target_link_libraries(myapp PRIVATE riocore)
# Optional, before linking: target_compile_definitions(riocore PUBLIC RIO_SMALL) for MCU-sized fixed caps.
if(NOT TARGET riocore)
  add_library(riocore STATIC ${CMAKE_CURRENT_LIST_DIR}/src/rio.c)
  target_include_directories(riocore PUBLIC ${CMAKE_CURRENT_LIST_DIR}/src)
  if(NOT MSVC)
    target_link_libraries(riocore PUBLIC m)
  endif()
endif()
