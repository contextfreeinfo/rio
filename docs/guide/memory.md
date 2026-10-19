# Memory and limits

The library has no static RAM of its own. Everything lives in three buffers the host provides:
`rio_Vm`, `mem` and `code`.

## Code and data share one buffer

Unless the host passes a code buffer, the code lives in `mem` too. While compiling, it grows in the
compiler's working memory (a quarter of it by default, `rio_Limits.code`), and when compiling ends it
moves to just below the strings and tables at the top of memory, taking exactly its own size (8 bytes an
instruction). So one buffer is the whole budget, split between code and data however each program needs.
A REPL with the code in `mem` keeps a quarter of it for code for the whole session.

## The compiler borrows the program's memory

While compiling, the compiler writes only constants (at the bottom of `mem`) and string literals into
`mem`. Frames, globals and arrays are just address ranges that stay zero until the program runs, so the
compiler keeps its working state (symbol tables, expression stacks, the string pool) on top of them.
When it finishes, it packs frames and globals down right after the constants actually used, lets procs
that never run at the same time share frame memory, moves the strings, the export table and the line
table to their final place, and zeroes the rest. Peak RAM is *max(compiling, running)*, not their sum.

```
compiling:  [consts][ compiler state + string pool ..................................]
running:    [consts][ frames/globals | return stack ][ ... ][strings+exports][arrays ]
```

`rio_compileScratch` keeps the compiler state in a separate buffer instead, which stays readable after
compiling. The C backend needs that.

**The source can be in `mem` too.** A host with no room to spare, like a microcontroller reading a file,
can put the source at the top of `mem` and call `rio_compile` on it there. That memory is the program's
globals and arrays, still zero, so the compiler keeps its state below the source, and the program's final
layout takes it all back once compiling is done. Compiling then needs the source, the compiler's tables,
the constants and the strings to fit in `mem` together.

**Or the source can stay in its file.** `rio_compileStream(vm, read, ud)` reads the source a part at a
time, through a window the compiler keeps at the top of its own memory (2 KB by default;
`rio_setSourceWindow` sets another size before compiling). `read(ud, buf, at, n)` fills `buf` with up to
`n` bytes from byte `at` and returns how many it read, `0` at the end, or `-1` if it can't. It's
positional, so the compiler can go back a little, which it needs to look ahead now and then. String
literals go into the string pool as they're read, so only the window and the strings take room, not the
whole source. A loader can hand back its sources the same way (see [embedding](embedding.md#modules)).

```c
static int readFile(void *ud, char *buf, uint32_t at, uint32_t n) {
  FILE *f = ud;
  if (fseek(f, at, SEEK_SET)) return -1;
  return (int)fread(buf, 1, n, f);
}

rio_compileStream(&vm, readFile, file);
```

Half the window bounds what has to be read at once: a name or number (at most 255 characters
anyway), or one line of a type method's declaration. Something longer is an error that names
`rio_setSourceWindow`.

## How near the limits

With overlapping like this, what fits isn't obvious, so Rio reports it. `rio_trackUsage(vm, &usage)`
(after `rio_init`) has each compile fill in a `rio_Usage`: how full each of the compiler's tables got
against its capacity, the string pool and the source, and, for a program that compiled, where its
memory goes when running. A compile that fails reports as far as it got, which shows what ran out.
`rio_usageText` makes it readable:

```
  compiler room     234496 bytes
  source in mem     14336 bytes
  symbols           78 of 1664: 4%
  names             422 of 19968: 2%
  ...
  string pool       11131 of 22964: 48%
  constants         336 bytes
  data              82872 bytes
  code              2112 bytes
  strings, tables   11620 bytes
  free              165204 bytes
```

The tables are sized to fill the compiler's room, so whichever comes nearest 100% would run out first.
When a big value (an array or struct over 16 words) is what didn't fit, the report ends with it: its
size (`big value`), and the room there was for it (`room for it`).
`rio -m` starts its report with this, from compiling in place as running would.

## Limits are chosen per compile

The compiler's tables (symbols, names, types, fields, procs, params, constants, nesting depth) are carved
out of its working memory at the start of each compile, scaled to the space available: `rio_compile`
scales to `mem`, and `rio_compileScratch` to the buffer given. For exact numbers, fill in a `rio_Limits`:

```c
rio_Limits lim = rio_limitsFor(16 << 10);   // a starting point, scaled to 16 KB
lim.procs = 64;                             // adjust what you need
rio_compileWith(&vm, src, len, scratch, sizeof scratch, &lim);   // scratch NULL: overlay mem instead
uint32_t need = rio_limitsSize(&lim);       // bytes those tables take (strings need some on top)
```

`rio_limitsFor(n)` sizes the tables to about 60% of `n` and leaves the rest for string literals.
Running out of anything is a clean compile error ("too many symbols", "scratch too small for these
limits", "not enough memory for the compiler").

| budget | symbols | procs | constants | tables |
|---|---|---|---|---|
| 8 KB | 64 | 16 | 128 | 5.6 KB |
| 16 KB | 144 | 36 | 288 | 8.7 KB |
| 64 KB | 698 | 174 | 1,396 | 33.9 KB |
| 512 KB | 6,044 | 1,511 | 12,088 | 261 KB |

The only build-time caps are for what the host registers: `rio_MAX_FFI`, `rio_MAX_DEFINES`,
`rio_MAX_AUTO_IMPORTS`, and the log and error buffers. `-Drio_SMALL` shrinks those.

## Measured

| | 64-bit | 32-bit |
|---|---|---|
| `rio_Vm` (runtime state, `rio_SMALL`) | 968 B | 624 B |
| smallest `mem` that compiles and runs a 64-ball physics demo | 6,570 B | 6,314 B |
| smallest `mem` for the full test suite (automatic limits) | 30.6 KB | 30.2 KB |
| bytecode | 8 B per instruction | same |
| C stack while running | a few hundred bytes (return addresses live in `mem`) | same |

On a Cortex-M33, the library compiles to about 68 KB of flash at `-Os` (clang), plus libm and
`memcpy`/`memset`; the VM alone (see [images](images.md)) is about 11 KB. The compiler doesn't use
`setjmp`: an error makes the rest of the source read as its end, so it unwinds by returning, and the
whole thing builds for plain wasm (compiler and VM: 112 KB, 46 KB gzipped).

## Shared frames

With no recursion, the compiler knows exactly which procs can be running at once: a proc and
everything it can call. Procs that never run together share frame memory, so frames cost the heaviest
chain of calls rather than the sum of all procs, and the return stack is sized by the longest chain.
A proc keeps its own frame if any of its slots has its address taken (by `&`, a view of a local array,
a `for` over one, and so on), since a view of that memory may outlive the call. Because of the
sharing, a program can't be started again while it runs: `rio_run` and `rio_call` from inside a host
proc return an error.

`rio -m game.rio` reports where a program's memory goes: constants, globals, the shared frame area and
the procs with their own frames, the call stack and the runtime tables, then the biggest procs and the
heaviest call chain.
