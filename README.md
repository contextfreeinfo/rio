# rio

A tiny, statically typed, embeddable scripting language. It's written in C99; the core is about 1,300 lines, and the optional C backend about 270 more.

- Odin-like syntax, with blocks closed by `end`
- Static types, inferred from the right-hand side
- Types: `Int` (i32), `Float` (f32), `Bool`, `String`, `Blob`, structs, slices (`[]T`), fixed arrays (`[N]T`, stored as contiguous structs), and fixed-capacity lists (`[..N]T`)
- **No dynamic allocation.** You give it one memory buffer and one code buffer; the compiler and VM never call `malloc`
- **No recursion**, either in the implementation (the parser uses explicit stacks) or in the language. Because procs can't recurse, each proc gets one static frame, and bytecode operands are absolute slots, so there is no stack and no frame pointer
- A register VM with typed opcodes, fused compare-and-branch, fused index+load/store, and bottom-tested loops. It uses computed goto on GCC/Clang and `switch` elsewhere
- **Ahead-of-time compilation to C** (`rio -c out.c file.rio`) for scripts known at build time
- No standard library: only math builtins and `log`. Anything else comes from the host through FFI

## Build

```
cmake -S . -B build
cmake --build build --config Release
ctest --test-dir build -C Release
build/rio tests/test.rio        # (build/Release/rio.exe with MSVC)
```

It needs only a C99 compiler and libm, and works on Linux, macOS and Windows.

On Windows with Visual Studio, CMake uses the ClangCL toolset when it's installed (the "C++ Clang tools for Windows" component in the VS installer), because clang supports the faster computed-goto dispatch. Otherwise it falls back to MSVC with `switch` dispatch. `-T <toolset>` overrides this, `-DRIO_PREFER_CLANGCL=OFF` disables it, and an existing build directory keeps the toolset it was first configured with.

## Language tour

```odin
// constants (folded at compile time)
N :: 1000
GRAVITY :: 9.8
NAME :: "rio"

// structs: fields are laid out contiguously
Vec :: struct
  x, y: Float
end
Particle :: struct
  pos, vel: Vec
  life: Int
  tag: Blob[8]          // inline 8 bytes
end

parts: [N]Particle      // global storage: one contiguous block
count := 0              // inferred Int
ready := false          // Bool
scale : Float = 2       // Int literal coerces to Float

// procs: must be defined before use, so recursion is impossible
step :: proc(ps: []Particle, dt: Float)
  for i in 0..<len(ps)
    ps[i].vel.y -= GRAVITY * dt
    ps[i].pos.x += ps[i].vel.x * dt
    ps[i].pos.y += ps[i].vel.y * dt
  end
end

length :: proc(v: Vec) -> Float
  return sqrt(v.x * v.x + v.y * v.y)
end

main :: proc()
  for i in 0..<N
    parts[i] = {pos = {x = Float(i), y = 0}, vel = {x = 1, y = 0}, life = 100}
  end
  step(parts, 0.016)              // [N]T converts to []T
  step(parts[10:20], 0.016)       // sub-slice (bounds-checked)
  log("speed", length(parts[0].vel), "name", NAME)

  i := 0
  for i < 10                      // while
    i += 3
  end
  for                             // infinite loop
    break
  end
  if i > 10 && count == 0
    log("big")
  else if i == 10
    log("ten")
  else
    log("small")
  end
end
```

The top-level code runs first, then the host (or the `rio` CLI) calls `main`.

**Rules that keep it small and safe**

- Values are copied: assigning or passing a struct copies it, and slices are views (address, length).
- Struct literals name their fields: `Vec{x = 1, y = 2}`. Order doesn't matter, fields you leave out are zero (`Vec{}` is all zeros), and a trailing comma is fine. There is no positional form; write a small proc like `vec :: proc(x: Float, y: Float) -> Vec` if you want one.
- When the struct type is already decided by where the value goes, leave the type off: `v: Vec = {x = 1}`, `p.vel = {y = 2}`, `dot({x = 1}, {y = 1})`, `return {x = x}`, `Particle{pos = {x = 3}}`, `ships.push({hp = 3})`. Where nothing decides it (`v := {x = 1}`), write `Type{...}`.
- A name or dotted path can stand in for a field of the same name: `{x, y}` means `{x = x, y = y}`, and `{src.pos, self.hp, life = 3}` means `{pos = src.pos, hp = self.hp, life = 3}`, so the last name in the path picks the field. Anything else, such as `xs[i]`, a call or arithmetic, needs `field = value`.
- A `String` is a read-only byte view. A `Blob` is a writable byte view, and `String(b)` turns a `Blob` into a `String`.
- A struct or array that contains slices or strings must be a **global**. It can't be a local, a parameter, or a return value. Slices themselves can be locals and parameters.
- Every index and slice is bounds-checked at runtime. Integer arithmetic wraps, and integer division by zero is a runtime error.
- There are no implicit conversions between `Int` and `Float` except for literals. Use `Float(x)` and `Int(x)` (which truncates).
- `Bool` is its own type. Comparisons and `! && ||` produce `Bool`, and `if`/`for` conditions must be `Bool` (`if n` is an error; write `if n != 0`). `true` and `false` are literals, and `Int(b)` (gives 0 or 1) and `Bool(n)` (`n != 0`) convert between the two.
- Statements end at a newline (`;` also works). Inside brackets, an expression can span several lines.

**Operators:** `+ - * / % & | ^ << >> == != < <= > >= && || ! ~ - =` and `+= -= *= /= %= &= |= ^= <<= >>=`.

**Builtins:** `log(...)`, `len(x)`, `cap(x)`, `min`, `max`, `abs`, `sqrt`, `sin`, `cos`, `tan`, `asin`, `acos`, `atan`, `atan2`, `exp`, `ln`, `pow`, `fmod`, `floor`, `ceil` and `round`, plus the list builtins below.

## Lists and text builders

A list has a fixed capacity and a current length, with no allocation. `[..N]T` is the storage, and `[..]T` is a view of one, the way `[N]T` relates to `[]T`:

| | storage (owns memory) | view (what procs take) |
|---|---|---|
| fixed length | `[N]T` | `[]T` |
| growable up to a limit | `[..N]T` | `[..]T` |
| bytes | `Blob[N]` / `Blob[..N]` | `Blob` / `Blob[..]` |

```odin
Enemy :: struct
  x, hp: Int
end
enemies: [..64]Enemy                     // up to 64; starts empty

spawn :: proc(list: [..]Enemy, x: Int) -> Bool
  return list.push({x = x, hp = 3})        // false when full
end
for i in 0..<3
  spawn(enemies, i * 10)                 // the view appends to the caller's list
end

for i in 0..<len(enemies)                // indexing is checked against the current length
  enemies[i].hp -= 1
end
enemies.swapRemove(0)                   // O(1): the last element takes its place
enemies.remove(0)                       // keeps order, O(n)
e := enemies.pop()
step(enemies[:])                         // [:] is a []Enemy of the live elements
enemies.clear()

enemies.pushAll(wave[:])                // many at once: a slice, array or list

title: Blob[..32]                        // a Blob list is a text builder
title.format("score: ", 42, " x", 1.5)  // appends values as text; all or nothing
title.push('!')                         // push is always one element: here, one byte
draw(String(title[:]))
```

- **Methods:** `xs.push(x) -> Bool` (one element), `xs.pushAll(ys) -> Bool` (every element of a slice, array or list with the same element type; a `String` works for Blob lists), `xs.pop()`, `xs.clear()`, `xs.remove(i)` and `xs.swapRemove(i)`. `len(xs)` and `cap(xs)` stay functions, since they work on every kind of sequence. A push that doesn't fit changes nothing and returns `false`; popping an empty list or removing past the end is a runtime error.
- **Text:** `b.format(...) -> Bool` appends Strings, Blobs, Ints, Floats and Bools to the end of a Blob list as text, formatted like `log` but without spaces between arguments. There is no template string: the arguments are the pieces, in order. If any of it doesn't fit, the list is left as it was. `b.push(x)` on a Blob list adds the single byte `x`, just as on any other list.
- **Views share the length:** a `[..]T` points at the list's length word, so pushing through any view changes the one real list. Assigning a list with `:=` also makes a view; use `[..N]T` explicitly for a separate copy.
- **Storage rules match arrays:** `[..N]T` can't be a parameter or result (pass `[..]T`), it can be a struct field (inline), and like slices a `[..]T` can be stored in a struct only if the struct is global.
- **Layout:** a 4-byte length followed by the N elements, contiguous like everything else.

## Methods

A method is a proc declared on a type with `Type.name`. Inside it, `self` is the value it was called on:

```odin
Ship :: struct
  x, vx: Float
  hp: Int
end
Ship.move :: proc(dt: Float)
  self.x += self.vx * dt
end
Ship.hurt :: proc(n: Int) -> Bool
  self.hp -= n
  return self.hp <= 0
end

fleet[i].move(dt)                // updates fleet[i] itself
if boss.hurt(3)
  log("boss down")
end
Int.double :: proc() -> Int      // methods work on Int, Float, Bool, String and Blob too
  return self * 2
end
```

- **Struct receivers are passed by reference,** so `self.hp -= n` changes the real ship. `self = Ship{...}` replaces it. For `Int`, `Float`, `Bool`, `String` and `Blob` receivers, `self` is a copy.
- **Names are per type:** `Ship.update`, `Bullet.update` and a plain `update` proc can all coexist. There's no other overloading.
- **One way to call:** `x.name(...)`, with the parentheses required. Fields are always `self.x`; a bare `x` inside a method is never the field, so globals stay unambiguous.
- **Rules:** like procs, methods are declared before use and can't recurse. A method can't share its name with one of its type's fields.
- **List methods:** list operations use the same syntax: `xs.push(e)`, `xs.pop()`, `b.format(...)`.

## Compile-time defines (`-D`)

Like `gcc -D`, you can set top-level constants from outside the script:

```odin
N :: 1000          // defaults, used when nothing is passed
SCALE :: 1.5
NAME :: "rio"
DEBUG :: false
```

```
rio -D N=20 -D SCALE=3 -D NAME=custom -D DEBUG game.rio
rio -c game.c -D N=20 game.rio          # works for the C backend too
```

- The value is parsed as the declared constant's type: `-D SCALE=3` gives `3.0`, `-D N=0x10` works for an `Int`, and a `Bool` accepts `true`/`false`/`0`/`1`. A bare `-D DEBUG` means `true`. Strings take the text as-is (surrounding quotes are optional).
- A define the script never declares is still usable, with its type inferred from the text (`7` → `Int`, `6.28` → `Float`, `true` or bare → `Bool`, anything else → `String`). The script then won't compile without it, just as in C.
- Only top-level `::` constants are overridden. A value that doesn't fit the declared type is a compile error, e.g. `-D N: value doesn't fit Int`.
- From C, call `rio_define(vm, "N", "20")` before `rio_compile`.

## Embedding

```c
#include "rio.h"
static uint8_t mem[1 << 20];          // script data, and the compiler's working memory while compiling
static RioIns code[16384];            // bytecode (max 65535)
static Rio vm;                        // runtime state (~1-12 KB depending on limits)

static void host_print(void *ud, const char *s, int n) { fwrite(s, 1, n, stdout); putchar('\n'); }
static void host_rand(Rio *vm, RioVal *a) { a[0].i = rand(); }  // args in a[], result to a[0]

rio_init(&vm, mem, sizeof mem, code, 16384);
rio_set_log(&vm, host_print, NULL);
rio_ffi(&vm, "rand", ">i", host_rand);   // sig: i f s b params, '>' result
rio_define(&vm, "LEVEL", "3");           // optional: like -D LEVEL=3
if (rio_compile(&vm, src, len) || rio_run(&vm)) puts(rio_error(&vm));

int f = rio_func(&vm, "update");
rio_args(&vm, f)[0].f = 0.016f;        // params are consecutive words (slices 2, structs n)
rio_call(&vm, f);
float r = rio_ret(&vm, f)->f;          // if it returns something
int *score = rio_global(&vm, "score"); // direct access to globals
```

`String` or `Blob` FFI arguments take two words (address, length); use `rio_ptr(vm, &a[i])` to get the bytes. An FFI function can abort the script with `rio_trap(vm, "msg")`. FFI functions must not call back into the VM.

You can change the limits (symbols, procs, constants and so on) with `-DRIO_MAX_...`. See `rio.h`.

### Errors

`rio_error(vm)` is a ready-to-print string in compiler style: `"12:7: type mismatch"` for compile errors and `"40: runtime error: index out of bounds"` for runtime errors. The CLI prefixes the file name (`game.rio:12:7: ...`), which terminals and editors can turn into links. For an editor, use the structured form:

```c
RioError e = rio_error_info(&vm);  // after a failed rio_compile / rio_run / rio_call
// e.kind: RIO_ECOMPILE or RIO_ERUNTIME
// e.line, e.col: 1-based (0 = unknown); e.len: width of the offending token, for underlining
// e.msg: the message without the position, e.g. "undefined name near 'nope'"
```

- Compile errors point at the token at fault. When an error is only detectable at the end of an expression (a type mismatch, say), they point at the expression's last token.
- Runtime errors report the line, from a compact pc-to-line table built while compiling: one 4-byte entry per source line that produces code, about 10% of the bytecode size. Columns aren't tracked at runtime. `rio_pc_line(vm, pc)` exposes the same lookup.
- A message from `rio_trap` in an FFI function becomes a runtime error at the line of the call.
- Code from `rio -c` reports the same lines: it emits `#line` directives, so each runtime check knows its rio line.

## Memory and microcontrollers

The library has no static RAM of its own. Everything lives in three buffers you provide: `Rio`, `mem` and `code`.

**The compiler borrows script memory.** While compiling, the compiler writes only two things into `mem`: constants (at the bottom) and string literals. Proc frames, globals and arrays are just address ranges that stay zero until the program runs. So `rio_compile` puts its working state (symbol tables, expression stacks, the string pool) on top of those ranges. When it finishes, it moves the strings and a small export table (for `rio_func`/`rio_global`) to their final place and zeroes the rest. Peak RAM is *max(compiling, running)*, not their sum.

```
compiling:  [consts][ compiler state + string pool ..................................]
running:    [consts][ frames/globals | return stack ][ ... ][strings+exports][arrays ]
```

`rio_compile_scratch` puts the compiler state in a separate buffer instead, which then stays readable after compiling. The C backend needs that.

**MCU limits:** build with `-DRIO_SMALL` for MCU-sized limits (128 symbols, 32 procs, 256 constants, 4K slots, …), or set each `RIO_MAX_*` yourself. Measured with `RIO_SMALL`:

| | 64-bit | 32-bit |
|---|---|---|
| `Rio` (runtime state) | 1,248 B | 968 B |
| compiler state (overlaid on `mem` while compiling) | 8,720 B | 8,504 B |
| smallest `mem` that compiles and runs a 64-ball physics demo | 9,853 B | 9,629 B |
| smallest `mem` for the full test suite | 11,021 B | 10,797 B |
| bytecode | 8 B per instruction (~1 B per source byte) | same |
| C stack while running | a few hundred bytes (return addresses live in `mem`) | same |

On a Cortex-M4, the core compiles to about 24.6 KB of flash at `-Os` (22.6 KB code, 2.1 KB read-only tables), plus libm and `memcpy`/`memset`/`setjmp` from your libc. Peak memory comes from compiling, dominated by the compiler state. If `mem` is too small, `rio_compile` fails cleanly with "out of compiler memory" or "out of memory".

## Ahead-of-time compilation to C

```
rio -c game.c game.rio      # writes a standalone C program
cc -O2 game.c -lm -o game   # any C99 compiler; MSVC works too
```

The output is one C file with no dependency on rio. Because rio has no recursion, each proc becomes a plain C function, and its frame slots become C locals unless their address is taken. Constants are written inline, and each bytecode op becomes one C statement. Globals, arrays and strings live in a static memory image identical to the VM's, so behavior is the same, including bounds checks and wrapping integer math. The C compiler then optimizes the result like any other C code.

FFI functions are called as `void rio_ffi_<name>(RioVal *a)`. The CLI includes definitions for its own `clock` and `putc`; for other FFI functions, link your own definitions (and build with `-DRIO_NO_HOST_FFI` to drop the CLI's). ctest runs the full test suite both in the VM and compiled through C.

## Performance

These are from `bench/`, comparing against Lua 5.5 on the same Windows machine. Times are in seconds, best of 5.

| benchmark | rio VM (clang, computed goto) | rio VM (MSVC, switch) | rio → C (clang -O2) | Lua 5.5 |
|---|---|---|---|---|
| loop: 100M int ops | 0.46 | 0.55–0.64 | 0.006 | 1.5–1.7 |
| calls: 30M proc calls | 0.36 | 0.34 | 0.010 | 1.3–1.5 |
| particles: 10k structs × 1000 steps | 0.28 | 0.39 | 0.014 | 0.9–1.0 |
| sieve: 2M, ×10 | 0.48 | 0.49 | 0.14 | 1.4–1.6 |

- **VM:** about 3–4× faster than Lua here. On Linux with a faster Lua build, particles measured 0.29s vs 0.59s, about 2×, so expect roughly 2–4× depending on the Lua build. The speed comes from static types (no tag checks), absolute-slot operands (static frames), constants preloaded in memory, compare-and-branch fusion, fused index+load/store, bottom-tested `while` loops, a dedicated `for` loop op, and destination retargeting that removes most moves.
- **AOT:** sieve is about 3.5× faster than the VM. The other three run in milliseconds because the C compiler can see through those loops entirely (inlining, vectorizing, or folding them), so treat them as an upper bound rather than typical.
- The particles sums differ from Lua only because rio's `Float` is f32: Lua with f32 rounding emulated gives the identical 43926.92.
- `switch` dispatch on MSVC is sensitive to code layout. The same VM source has measured 0.34s or 0.65s on `calls` depending only on how unrelated code changes shifted the build. The cause hasn't been pinned down, so expect build-to-build variation with MSVC; ClangCL (see Build) avoids it.

## Layout

```
src/rio.h     public API (+ the fixed-size state struct)
src/rio.c     lexer, single-pass compiler, VM
src/aot.c     bytecode -> C translator (CLI only; not needed for embedding)
src/main.c    CLI host (adds ffi: clock, putc)
tests/        test suite (run by ctest, in the VM and compiled through C), plus error tests:
              errors.c checks kind/line/col/width/message for ~26 failing snippets
bench/        rio vs lua benchmarks
```
