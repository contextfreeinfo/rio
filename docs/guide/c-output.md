# C output

```sh
rio -c game.c game.rio      # writes a standalone C program
cc -O2 game.c -lm -o game   # any C99 compiler; MSVC works too
```

The output is one C file with no dependency on Rio. Because Rio has no recursion, each proc becomes a
plain C function, and its frame slots become C locals unless their address is taken. Constants are
written inline, and each bytecode op becomes one C statement. Globals, arrays and strings live in a
static memory image identical to the VM's, so behavior is the same, bounds checks and wrapping math
included. The C compiler then optimizes it like any other C code. Runtime errors report Rio lines,
through `#line` directives.

Host procs are called as `void rio_ffi_<name>(rio_Val *a)`, with a dotted name's dots as `_`
(`rio_ffi_pc_cls`). The CLI includes definitions of its own `clock` and `putc`; link your own for other
host procs (and build with `-Drio_NO_HOST_FFI` to drop the CLI's). ctest runs the test suite both in the
VM and through C.
