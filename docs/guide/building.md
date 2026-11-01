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

The tests include `tests/docs.rio`, which runs every example in these docs. With zig on the PATH,
`rio tools/zigbuild.rio` (run from the repo's folder) cross-builds the rio program for other systems
into `build-zig`: `linux-x86_64` and `linux-arm64` (static), and `macos-arm64` (not signed).

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
rio fmt game.rio ...     the files laid out the one way, printed; -w rewrites the ones that change,
                         -l lists them (exiting 1 if any)
```

`rio fmt` changes only white space: the indent at each line's start (2 for each block it's in), the
gaps between tokens (`x := a[i] + f(b, -c)`), spaces at line ends (gone), line ends (all as the
file's first, `\n` or `\r\n`), and line breaks inside brackets where Rio reads them as spaces. The one
token it adds is a trailing comma, where a list stays over lines. Before it gives code back it checks
that the tokens are the same but for those commas, so the program means what it did. It doesn't wrap
long lines: no rule depends on a line's length.

A list in `()`, `[]` or `{}` with commas is laid out by its trailing comma, and by whether a line break
follows its opener:

```
f(a,           f(a, b, c)      no trailing comma, first item on the opener's line: one line
  b, c)

f(             f(              no trailing comma, a break after the opener: the items on a line of
  a,             a, b, c       their own (and so back to one line by joining that line up and
  b, c)        )               taking the comma off)

f(a, b, c,)    f(              a trailing comma, first item on the opener's line: an item a line
                 a,
                 b,
                 c,
               )

f(             f(              a trailing comma, a break after the opener: the lines as they are,
  a, b,          a, b,         for tables
  c,             c,
)              )
```

A comment, or a string over lines, in a list keeps it over lines (with a trailing comma added). A
list over lines that's the last item of another can sit on its line: `f(a, [` ... `])`. Parentheses
without commas (grouping) keep their lines as written.

It keeps what lines things up: runs of spaces before `::`, `:=` or `=` that line up with the lines
around them, comments in the column they were in, and lines going on (after an operator, or in
grouping parentheses) where they were from their statement's start. Lines from a `# nofmt` comment
line to a `# /nofmt` one (or the file's end) are left as they are, but for spaces at their ends; the
two comments are indented as usual, and can have a note after a space (`# nofmt (a table)`). Code
needn't compile to be laid out, and hosts get it as `rio_format` (in `rio.h`), which needs no VM.

Errors print as `game.rio:12:7: type mismatch`, which terminals and editors can turn into links.
Ctrl-C stops a running program. Arguments after the program's file are the program's (`rio
tool.rio a b`: `os.arg`). The CLI's host procs are in its module [os](os.md), for programs that
`import os`.

In the REPL, globals, procs, types and methods carry over from one input to the next, and a bare
expression prints its value. An unfinished input gets a `.` prompt for more, as wide as `>` so the
lines line up; a blank line drops it. A compile error leaves the session as it was. Lines are edited
as they're typed, as `os.readLine`'s are ([os](os.md)): the arrows, Home and End, Ctrl+Left and Right
by words, Ctrl+K, Ctrl+U, Ctrl+W and Ctrl+Y, and Up and Down for the lines typed before, which are kept
in `~/.rio_history`. Ctrl+C drops what's typed, and Ctrl+D quits.
