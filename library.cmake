add_library(rio-lib INTERFACE)

target_sources(rio-lib INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}/src/engine.c
    ${CMAKE_CURRENT_LIST_DIR}/src/gen.c
    ${CMAKE_CURRENT_LIST_DIR}/src/lex.c
    ${CMAKE_CURRENT_LIST_DIR}/src/parse.c
    ${CMAKE_CURRENT_LIST_DIR}/src/util.c
)
