# Programs and hosts

A Rio program runs inside a host: a program that embeds Rio, such as the `rio` command line tool or
a game console. The host gives the program its memory, its log, its host procs and its modules,
and decides when the program's procs run. How a host does that in C is in the
[guide](../guide/embedding.md).

## Running

1. The host compiles the main file. It's rejected as a whole if anything in it, or in what it imports,
   doesn't compile.
2. The top-level code runs, from the top. Imported modules run where they're first imported.
3. The host calls the procs it wants: the CLI calls `main` if there is one; a game console might call
   `update` and `draw` each frame. Globals keep their values between calls.

A runtime error stops what's running and reports its line (and file, outside the main file). The
errors are: an index or slice out of bounds, division by zero, `pop` from an empty builder, a value
that isn't one of an enum's, calling an unset proc value, a failed `assert`, an error from a host proc,
and being stopped by the host.

## What the host can find

The host can find `main`, and the main file's top-level procs and globals marked with `*`, the same
mark modules use for exports:

```rio fragment
score* := 0                 # the host can read and change it
update* :: proc()           # the host can call it
  score += 1
end
```

## Host procs

A host proc is a proc the host provides, written in the host's language. Its parameters and result
can be `Int`, `Float`, `String` and `Blob`, and its parameters structs. A host proc has a plain name, global like a builtin, or a
module name and a name, such as `pc.cls`. Host procs in a module are used through that module: with
`import pc`, or everywhere if the host auto-imports `pc`. If the host has no source for the module,
the module is just its host procs, all exported.

```rio fragment
import pc                   # a module of host procs (a host that has one)
pc.cls(0)
```

The CLI has two global host procs: `clock() -> Float`, seconds of processor time, and `putc(c: Int)`,
one byte to its output.

A host proc can't call back into the program while it runs. It can stop the program with a runtime
error.

## Declaring host procs

`host` declares what the host provides, and the compiler checks the declaration against it. Such
declarations document a host's API in Rio, and since they're checked, they can't drift:

```rio
clock* :: host proc() -> Float     # the CLI's clock: checked against it
start := clock()
log(start >= 0)
```

```output
true
```

```rio
clock :: host proc() -> Int  # error: doesn't match the host's clock: proc() -> Float
```

- **Host procs:** `name :: host proc(...) -> R`, a proc type, as for proc values: its parameter and
  result types must be exactly the host's, except that an enum or a `Bool` may stand for an `Int`,
  which it is underneath, and a struct parameter for its fields (below); calls then take the declared
  types. Its last parameters may be optional (`t?: Text`), and the host gets zeros for those a call
  leaves out, just as an image made before a host proc gained parameters at its end passes zeros for
  them. So a host proc can gain options without breaking programs, compiled or not. In a module, `name` is the host proc of the module's
  name (`cls` in `pc.rio` is `pc.cls`), else a global one. When the host has a source for a module of
  host procs, that source is the module (auto-imported ones too), and it must declare every one of
  the module's host procs. So a module can give its host procs types of its own:

  ```rio fragment
  Pointer* :: enum            # in pc.rio
    game, ui, none
  end
  pointer* :: host proc(mode: Pointer)   # the host's pc.pointer takes an Int
  ```
- **Struct parameters:** a struct goes to the host by address, and the host reads it in place, so it
  can hold anything a struct can, and be any size. The host's sig gives its layout: each field's name
  and what it is to C (an enum or a `Bool` is an `Int`), which the declaration's must be. The host
  only borrows it, as it does views, for the call; it reads it as it is when the call is made:

  ```rio fragment
  Pos* :: struct              # in pc.rio
    x, y: Int
  end
  Text* :: struct
    at: Pos
    color: Int(8)
    big: Bool(1)
  end
  text* :: host proc(s: String, t: Text)   # the host's sig: "s{at:{x:Int,y:Int},color:Int(8),big:Int(1)}"
  pc.text("hi", {at = {x = 3, y = 4}, big = true})   # color 0
  ```

  A field left out of a literal is zero, so a host proc's options can grow without breaking calls
  written before: add fields at the end, and make zero mean what calls did before. Images made before
  then still load: the host reads zeros for the fields they lack. A host writes its side from the
  module's source, which makes C structs and sigs to match: see the
  [embedding guide](../guide/embedding.md#structs).
- **`&T` parameters:** as for a proc, a host proc may take a place, `&T`, and change it: an `Int`, a
  `Float` or a struct (`&i`, `&f`, `&{...}` in its sig). Calls pass `&x`. The host writes no more than
  the place holds, so a struct an older image made with fewer fields stays as it was past them:

  ```rio fragment
  Mouse* :: struct            # in pc.rio
    x, y: Int
    down: Bool
  end
  mouse* :: host proc(m: &Mouse)   # the host fills it in
  m: pc.Mouse
  pc.mouse(&m)
  ```
- **Results:** a host proc returns what a proc may: an `Int`, `Float`, `String` or `Blob`, or a struct,
  union or maybe of up to 16 words with no views in it (`>{...}`, `>Union{...}` in its sig). A struct
  result may gain fields at its end, as a parameter may: an older image gets the fields it holds.

  ```rio fragment
  mouse* :: host proc() -> Mouse          # in pc.rio: rather than taking &Mouse
  keyDown* :: host proc(k: Int) -> ?Int   # nil, or how long it's been down
  ```
- **Builtins:** declared the same way, and checked as calls with those types:
  `sqrt* :: host proc(x: Float) -> Float`. [`src/core.rio`](https://github.com/contextfreeinfo/rio/blob/main/src/core.rio) declares them all.
- **Builtin types:** `Int* :: host struct`.
- **Methods:** on a named type, `Int.toFloat* :: host proc() -> Float`; on any other type, in
  parentheses: `(Blob[..]).push* :: host proc(byte: Int) -> Bool`.

`host` declarations go at the top level; `*` exports them, as for other declarations. A name that's
neither a host proc nor a builtin is an error:

```rio
nope :: host proc(x: Int)  # error: the host has no proc by this name
```

A host can ship such a module only as documentation, and still make its module straight from its host
procs, so nothing is parsed at runtime. Its tests then compile the source against the real host procs.

## Defines

The host can set top-level constants from outside, like `gcc -D`; the CLI takes `-D NAME=value`.
The value is read as the constant's declared type: `-D SCALE=3` gives a `Float` constant `3.0`,
`-D DEBUG` alone means `true`, and a `String` takes the text as it is. A define the program doesn't
declare is still a constant, with its type from its text: `7` an `Int`, `6.28` a `Float`, `true`,
`false` or nothing a `Bool`, and anything else a `String`. A value that doesn't fit is a compile error.

```rio fragment
N :: 1000          # rio -D N=20 game.rio makes N 20
DEBUG :: false     # rio -D DEBUG game.rio makes it true
```

## Memory

A program's memory is all known when it compiles: its globals, one frame for each proc (procs that
never run at the same time can share one), and its strings. Nothing is allocated while it runs.
Programs that don't fit are compile errors, as are programs too big for the compiler's tables.

The host sees values in words of 4 bytes, little-endian:

| type | words |
|---|---|
| `Int`, `Float`, `Bool`, enums, proc values | 1 |
| `String`, `Blob`, `[]T` | 2: where the bytes or elements start, and how many |
| `[..]T`, `Blob[..]` | 3: where the length is, where the elements start, the capacity |
| `[N]T` | N elements, one after another |
| `[..N]T` | the length, then N elements |
| `Blob[N]` | N bytes, padded to whole words |
| structs | the fields in order |
| unions | the type's index (in the order listed), then room for the biggest type |

Positions ("where ... start") are byte offsets in the program's memory. Host procs get their
arguments as these words, one after another.
