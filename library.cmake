add_library(rio-lib INTERFACE)

target_sources(rio-lib INTERFACE
    src/engine.c
    src/gen.c
    src/lex.c
    src/parse.c
    src/util.c
)
