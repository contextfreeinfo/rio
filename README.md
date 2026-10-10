# nib

A tiny, statically typed, embeddable scripting language. It's written in C99; the core is about 1,300 lines, and the optional C backend about 270 more.

- Odin-like syntax, with blocks closed by `end`
- Static types, inferred from the right-hand side
- Types: `i32`, `f32`, `bool`, `string`, `blob`, structs, slices (`[]T`), and fixed arrays (`[N]T`, stored as contiguous structs)
- **No dynamic allocation.** You give it one memory buffer and one code buffer; the compiler and VM never call `malloc`
- **No recursion**, either in the implementation (the parser uses explicit stacks) or in the language. Because procs can't recurse, each proc gets one static frame, and bytecode operands are absolute slots, so there is no stack and no frame pointer
- A register VM with typed opcodes, fused compare-and-branch, fused index+load/store, and bottom-tested loops. It uses computed goto on GCC/Clang and `switch` elsewhere
- **Ahead-of-time compilation to C** (`nib -c out.c file.nib`) for scripts known at build time
- No standard library: only math builtins and `log`. Anything else comes from the host through FFI

## Build

```
cmake -S . -B build
cmake --build build --config Release
ctest --test-dir build -C Release
build/nib tests/test.nib        # (build/Release/nib.exe with MSVC)
```

It needs only a C99 compiler and libm, and works on Linux, macOS and Windows.

On Windows with Visual Studio, CMake uses the ClangCL toolset when it's installed (the "C++ Clang tools for Windows" component in the VS installer), because clang supports the faster computed-goto dispatch. Otherwise it falls back to MSVC with `switch` dispatch. `-T <toolset>` overrides this, `-DNIB_PREFER_CLANGCL=OFF` disables it, and an existing build directory keeps the toolset it was first configured with.

## Language tour

```odin
// constants (folded at compile time)
N :: 1000
GRAVITY :: 9.8
NAME :: "nib"

// structs: fields are laid out contiguously
Vec :: struct
  x, y: f32
end
Particle :: struct
  pos, vel: Vec
  life: i32
  tag: blob[8]          // inline 8 bytes
end

parts: [N]Particle      // global storage: one contiguous block
count := 0              // inferred i32
ready := false          // bool
scale : f32 = 2         // int literal coerces to f32

// procs: must be defined before use, so recursion is impossible
step :: proc(ps: []Particle, dt: f32)
  for i in 0..<len(ps)
    ps[i].vel.y -= GRAVITY * dt
    ps[i].pos.x += ps[i].vel.x * dt
    ps[i].pos.y += ps[i].vel.y * dt
  end
end

length :: proc(v: Vec) -> f32
  return sqrt(v.x * v.x + v.y * v.y)
end

main :: proc()
  for i in 0..<N
    parts[i] = Particle{Vec{f32(i), 0}, Vec{1, 0}, 100}
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

The top-level code runs first, then the host (or the `nib` CLI) calls `main`.

**Rules that keep it small and safe**

- Values are copied: assigning or passing a struct copies it, and slices are views (address, length).
- Strings are read-only byte views. A `blob` is a writable byte view, and `string(b)` turns a blob into a string.
- A struct or array that contains slices or strings must be a **global**. It can't be a local, a parameter, or a return value. Slices themselves can be locals and parameters.
- Every index and slice is bounds-checked at runtime. Integer arithmetic wraps, and integer division by zero is a runtime error.
- There are no implicit conversions between `i32` and `f32` except for literals. Use `f32(x)` and `i32(x)` (which truncates).
- `bool` is its own type. Comparisons and `! && ||` produce `bool`, and `if`/`for` conditions must be `bool` (`if n` is an error; write `if n != 0`). `true` and `false` are literals, and `i32(b)` (gives 0 or 1) and `bool(n)` (`n != 0`) convert between the two.
- Statements end at a newline (`;` also works). Inside brackets, an expression can span several lines.

**Operators:** `+ - * / % & | ^ << >> == != < <= > >= && || ! ~ - =` and `+= -= *= /= %= &= |= ^= <<= >>=`.

**Builtins:** `log(...)`, `len(x)`, `min`, `max`, `abs`, `sqrt`, `sin`, `cos`, `tan`, `asin`, `acos`, `atan`, `atan2`, `exp`, `ln`, `pow`, `fmod`, `floor`, `ceil` and `round`.

## Embedding

```c
#include "nib.h"
static uint8_t mem[1 << 20];          // script data, and the compiler's working memory while compiling
static NibIns code[16384];            // bytecode (max 65535)
static Nib vm;                        // runtime state (~1-12 KB depending on limits)

static void host_print(void *ud, const char *s, int n) { fwrite(s, 1, n, stdout); putchar('\n'); }
static void host_rand(Nib *vm, NibVal *a) { a[0].i = rand(); }  // args in a[], result to a[0]

nib_init(&vm, mem, sizeof mem, code, 16384);
nib_set_log(&vm, host_print, NULL);
nib_ffi(&vm, "rand", ">i", host_rand);   // sig: i f s b params, '>' result
if (nib_compile(&vm, src, len) || nib_run(&vm)) puts(nib_error(&vm));

int f = nib_func(&vm, "update");
nib_args(&vm, f)[0].f = 0.016f;        // params are consecutive words (slices 2, structs n)
nib_call(&vm, f);
float r = nib_ret(&vm, f)->f;          // if it returns something
int *score = nib_global(&vm, "score"); // direct access to globals
```

String or blob FFI arguments take two words (address, length); use `nib_ptr(vm, &a[i])` to get the bytes. An FFI function can abort the script with `nib_trap(vm, "msg")`. FFI functions must not call back into the VM.

You can change the limits (symbols, procs, constants and so on) with `-DNIB_MAX_...`. See `nib.h`.

## Memory and microcontrollers

The library has no static RAM of its own. Everything lives in three buffers you provide: `Nib`, `mem` and `code`.

**The compiler borrows script memory.** While compiling, the compiler writes only two things into `mem`: constants (at the bottom) and string literals. Proc frames, globals and arrays are just address ranges that stay zero until the program runs. So `nib_compile` puts its working state (symbol tables, expression stacks, the string pool) on top of those ranges. When it finishes, it moves the strings and a small export table (for `nib_func`/`nib_global`) to their final place and zeroes the rest. Peak RAM is *max(compiling, running)*, not their sum.

```
compiling:  [consts][ compiler state + string pool ..................................]
running:    [consts][ frames/globals | return stack ][ ... ][strings+exports][arrays ]
```

`nib_compile_scratch` puts the compiler state in a separate buffer instead, which then stays readable after compiling. The C backend needs that.

**MCU limits:** build with `-DNIB_SMALL` for MCU-sized limits (128 symbols, 32 procs, 256 constants, 4K slots, …), or set each `NIB_MAX_*` yourself. Measured with `NIB_SMALL`:

| | 64-bit | 32-bit |
|---|---|---|
| `Nib` (runtime state) | 1,248 B | 968 B |
| compiler state (overlaid on `mem` while compiling) | 8,720 B | 8,504 B |
| smallest `mem` that compiles and runs a 64-ball physics demo | 9,853 B | 9,629 B |
| smallest `mem` for the full test suite | 11,021 B | 10,797 B |
| bytecode | 8 B per instruction (~1 B per source byte) | same |
| C stack while running | a few hundred bytes (return addresses live in `mem`) | same |

On a Cortex-M4, the core compiles to about 24.6 KB of flash at `-Os` (22.6 KB code, 2.1 KB read-only tables), plus libm and `memcpy`/`memset`/`setjmp` from your libc. Peak memory comes from compiling, dominated by the compiler state. If `mem` is too small, `nib_compile` fails cleanly with "out of compiler memory" or "out of memory".

## Ahead-of-time compilation to C

```
nib -c game.c game.nib      # writes a standalone C program
cc -O2 game.c -lm -o game   # any C99 compiler; MSVC works too
```

The output is one C file with no dependency on nib. Because nib has no recursion, each proc becomes a plain C function, and its frame slots become C locals unless their address is taken. Constants are written inline, and each bytecode op becomes one C statement. Globals, arrays and strings live in a static memory image identical to the VM's, so behavior is the same, including bounds checks and wrapping integer math. The C compiler then optimizes the result like any other C code.

FFI functions are called as `void nib_ffi_<name>(NibVal *a)`. The CLI includes definitions for its own `clock` and `putc`; for other FFI functions, link your own definitions (and build with `-DNIB_NO_HOST_FFI` to drop the CLI's). ctest runs the full test suite both in the VM and compiled through C.

## Performance

These are from `bench/`, comparing against Lua 5.5 on the same Windows machine. Times are in seconds, best of 5.

| benchmark | nib VM (clang, computed goto) | nib VM (MSVC, switch) | nib → C (clang -O2) | Lua 5.5 |
|---|---|---|---|---|
| loop: 100M int ops | 0.46 | 0.55–0.64 | 0.006 | 1.5–1.7 |
| calls: 30M proc calls | 0.36 | 0.34 | 0.010 | 1.3–1.5 |
| particles: 10k structs × 1000 steps | 0.28 | 0.39 | 0.014 | 0.9–1.0 |
| sieve: 2M, ×10 | 0.48 | 0.49 | 0.14 | 1.4–1.6 |

- **VM:** about 3–4× faster than Lua here. On Linux with a faster Lua build, particles measured 0.29s vs 0.59s, about 2×, so expect roughly 2–4× depending on the Lua build. The speed comes from static types (no tag checks), absolute-slot operands (static frames), constants preloaded in memory, compare-and-branch fusion, fused index+load/store, bottom-tested `while` loops, a dedicated `for` loop op, and destination retargeting that removes most moves.
- **AOT:** sieve is about 3.5× faster than the VM. The other three run in milliseconds because the C compiler can see through those loops entirely (inlining, vectorizing, or folding them), so treat them as an upper bound rather than typical.
- The particles sums differ from Lua only because nib floats are f32: Lua with f32 rounding emulated gives the identical 43926.92.
- `switch` dispatch on MSVC is sensitive to code layout. The same VM source has measured 0.34s or 0.65s on `calls` depending only on how unrelated code changes shifted the build. The cause hasn't been pinned down, so expect build-to-build variation with MSVC; ClangCL (see Build) avoids it.

## Layout

```
src/nib.h     public API (+ the fixed-size state struct)
src/nib.c     lexer, single-pass compiler, VM
src/aot.c     bytecode -> C translator (CLI only; not needed for embedding)
src/main.c    CLI host (adds ffi: clock, putc)
tests/        test suite (run by ctest, in the VM and compiled through C)
bench/        nib vs lua benchmarks
```
