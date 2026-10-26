# Source layout

```
src/rio.h          the public API (and the fixed-size state struct)
src/internal.h     what Rio's files share: the instruction set, symbol and type kinds, runtime tables
the VM (all a host needs to run images):
  src/vm.c         the interpreter
  src/runtime.c    setting up a vm, finding procs and globals, errors, the line table
  src/image.c      saving and loading images
  src/common.c     number formatting, error messages, math builtins
the compiler (single pass, no recursion; left out with rio_NO_COMPILER):
  src/compiler.h   its state and tables, and what its files share
  src/lexer.c      tokens, and reporting compile errors
  src/tables.c     names, symbols, types and fields
  src/codegen.c    emitting code; slots, constants and memory; moving values; peepholes
  src/operators.c  unary and binary operators, element-by-element arithmetic, unions' `is`
  src/expr.c       the expression parser (shunting-yard): calls, indexing, members, literals
  src/builtins.c   log, math, assert, and the methods of sequences, builders, enums and numbers
  src/decls.c      types as written; procs, methods, structs, unions, enums and variables
  src/statements.c statements and their blocks: if, loops, switch, return, assignment
  src/modules.c    import and include
  src/host.c       @host declarations, checked; modules of host procs
  src/compile.c    limits, setup, and finishing: frame sharing, the line table, final layout
  src/repl.c       REPL sessions
src/jit.c          native loops on ARM Thumb-2 (elsewhere, rio_jit does nothing)
src/core.rio       the builtins, declared with @host (compiled by the tests, not by programs)
src/aot.c          bytecode to C (CLI only; not needed for embedding)
src/main.c         the CLI host (host procs clock and putc; loads modules from files and -L paths)
tests/             ctest runs these, in the VM and through C:
  test.rio         the main suite
  errors.c         compile and runtime errors: kind, line, column, width and message
  docs.py          every example in docs/
  ...              modules, host procs, the REPL, images, exports, fuzzing, VM-vs-C differences
docs/              this documentation: SUMMARY.md sets the order; mdbook builds it as a site
bench/             Rio vs Lua benchmarks
```

Everything Rio defines in C starts with `rio_`: functions are `rio_camelCase`, types `rio_PascalCase`,
enum values `rio_EnumType_value`, and macros `rio_UPPER_CASE` (the CMake options keep CMake's `RIO_`
style). `library.cmake` lists the sources (`RIO_VM_SOURCES`, `RIO_COMPILER_SOURCES`) for builds that pick
them. Comments in the source point to the spec section a piece of code implements, as
`docs/spec/lexical.md`.
