# The Rio language

Rio is a small, statically typed language for embedding in other programs, down to microcontrollers.
This spec says what Rio programs mean, for people writing them and for people implementing Rio. It
leans on examples, and the examples are tests: `tests/docs.py` runs every one against the Rio in this
repository.

## Design in brief

- **Static types,** mostly inferred: `x := 1` is an `Int`.
- **No hidden allocation.** A program's memory is known when it compiles: globals, one frame per
  proc, and fixed-size arrays and builders.
- **No recursion.** Everything is declared before it's used, so a proc can't call itself, directly or
  through others. That's what lets each proc have one fixed frame.
- **Values are copied, views are explicit.** Structs and arrays are values, copied when assigned.
  Slices, `String`s, `Blob`s and builder views are views of memory, with types that say so. `&` lets a
  proc change the caller's things.
- **Safe at runtime.** Indexes are checked, integers wrap, and errors stop the program with a line
  number, never with undefined behavior.
- **The host provides the rest.** There's no standard library beyond math and text. Anything else,
  such as drawing or files, comes from the program that embeds Rio as host procs.

## Reading the examples

A `rio` block is a whole program. When it logs something, the block after it shows the output:

```rio
x := 6 * 7
log("x is", x)
```

```output
x is 42
```

A comment `# error: ...` ends a line that must fail to compile with that message, and
`# runtime error: ...` a line that must stop the program when it runs:

```rio
x := 1
x = "one"  # error: type mismatch
```

```rio
xs: [3]Int
i := 3
log(xs[i])  # runtime error: index out of bounds
```

The messages are the reference implementation's. Another implementation needs the same errors at
the same places, not the same words.

## Words used here

- **Compile error:** the program is rejected before any of it runs.
- **Runtime error:** the program stops, reporting the line. Nothing after it runs.
- **Word:** 4 bytes, the size of an `Int`.
- **View:** a value that refers to memory it doesn't own: a slice, `String`, `Blob` or builder view.

## Not yet in Rio

Generics, recursion (on purpose), closures, heap allocation (on purpose), and a standard library
(the host's job). Where something is planned, the spec says so.
