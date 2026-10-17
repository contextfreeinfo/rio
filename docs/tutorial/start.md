# Getting started

This tutorial shows Rio by example, a page at a time. Each example is a whole program you can run.
For the exact rules, see the [spec](../spec/language.md).

## Get Rio

Download a build from the
[nightly release](https://github.com/contextfreeinfo/rio/releases/tag/nightly), or build it, with a C
compiler and CMake:

```sh
cmake -S . -B build
cmake --build build --config Release
```

## Hello

Save this as `hello.rio`:

```rio
log("hello from rio")
```

and run it:

```sh
rio hello.rio
```

```output
hello from rio
```

`log` prints its arguments on a line, with spaces between them. The program's top-level code runs
first; then, if there's a proc named `main`, `rio` calls it:

```rio
greeting := "hi"
main :: proc()
  log(greeting, "from main")
end
```

```output
hi from main
```

## The REPL

`rio` on its own starts a REPL, where each input runs as you enter it, and a bare expression shows
its value. Names stay around from one input to the next:

```
> x := 6 * 7
> x
42
> sq :: proc(n: Int) -> Int
.   return n * n
. end
> sq(x)
1764
```

An unfinished input, such as an open block, gets a `.` prompt for more. Ctrl-C stops a program that's
running.

Next: [values and procs](basics.md).
