# Building and the CLI

This guide is about the Rio in this repository: a C99 library (compiler and VM) and the `rio` command
line tool. For the language itself, see the [spec](../spec/language.md).

## Building

```sh
cmake -S . -B build
cmake --build build --config Release
ctest --test-dir build -C Release
build/rio tests/test.rio        # (build/Release/rio.exe with MSVC)
```

It needs only a C99 compiler and libm, and works on Linux, macOS and Windows. Prebuilt binaries are
in the [nightly](https://github.com/contextfreeinfo/rio/releases/tag/nightly) prerelease, rebuilt
each night `main` changes (and on demand: Actions > ci > Run workflow), and in releases for tags like
`v0.1.0`.

On Windows with Visual Studio, CMake uses the ClangCL toolset when it's installed (the "C++ Clang
tools for Windows" component), because clang supports the faster computed-goto dispatch. Otherwise it
falls back to MSVC with `switch` dispatch. `-T <toolset>` overrides this, `-DRIO_PREFER_CLANGCL=OFF`
disables it, and an existing build directory keeps the toolset it was first configured with.

The tests include `tests/docs.py`, which runs every example in these docs (with Python 3, when CMake
finds it).

## The CLI

```
rio                      a REPL
rio game.rio             run the top-level code, then main() if there is one
rio -L libs game.rio     with packages from libs/ (each libs/name.rio or libs/name/name.rio)
rio -D N=20 game.rio     set a top-level constant (or define one), like gcc -D
rio -c game.c game.rio   compile to a standalone C program instead (see C output)
rio -i game.img game.rio compile to an image instead (see Images); rio game.img runs one
rio -m game.rio          how near its limits it comes, and where its memory goes, instead of running it
rio -M 256K ...          the memory the program gets (default 8M, up to 1G), allocated once at the start
rio -j game.rio          compile simple loops to native code first (ARM with Thumb-2 only)
```

Errors print as `game.rio:12:7: type mismatch`, which terminals and editors can turn into links.
Ctrl-C stops a running program. The CLI's host procs are `clock() -> Float` (processor seconds) and
`putc(c: Int)`.

In the REPL, globals, procs, types and methods carry over from one input to the next, and a bare
expression prints its value. An unfinished input gets a `.` prompt for more, as wide as `>` so the
lines line up; a blank line drops it. A compile error leaves the session as it was. For line editing
and history, Windows' console already has them; on Linux and macOS use `rlwrap rio`.
