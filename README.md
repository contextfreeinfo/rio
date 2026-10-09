# nib

A tiny, statically typed, embeddable scripting language. It's written in C99, and the core is about 1,200 lines.

- Odin-like syntax, with blocks closed by `end`
- Static types, inferred from the right-hand side
- Types: `i32`, `f32`, `bool`, `string`, `blob`, structs, slices (`[]T`), and fixed arrays (`[N]T`, stored as contiguous structs)
- **No dynamic allocation.** You give it one memory buffer and one code buffer; the compiler and VM never call `malloc`
- **No recursion**, either in the implementation (the parser uses explicit stacks) or in the language. Because procs can't recurse, each proc gets one static frame, and bytecode operands are absolute slots, so there is no stack and no frame pointer
- A register VM with typed opcodes and fused compare-and-branch. It uses computed goto on GCC/Clang and `switch` elsewhere
- No standard library: only math builtins and `log`. Anything else comes from the host through FFI

## Build

```
cmake -S . -B build
cmake --build build --config Release
ctest --test-dir build -C Release
build/nib tests/test.nib        # (build/Release/nib.exe with MSVC)
```

It needs only a C99 compiler and libm, and works on Linux, macOS and Windows.

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
static uint8_t mem[1 << 20];          // all script data
static NibIns code[16384];            // bytecode (max 65535)
static Nib vm;                        // compiler + vm state (fixed size)

static void host_print(void *ud, const char *s, int n) { fwrite(s, 1, n, stdout); putchar('\n'); }
static void host_rand(Nib *vm, NibVal *a) { a[0].i = rand(); }  // args in a[], result to a[0]

nib_init(&vm, mem, sizeof mem, code, 16384);
nib_set_log(&vm, host_print, NULL);
nib_ffi(&vm, "rand", ">i", host_rand);   // sig: i f s b params, '>' result
if (nib_compile(&vm, src, len) || nib_run(&vm)) puts(nib_error(&vm));

int f = nib_func(&vm, "update");
nib_arg(&vm, f, 0)->f = 0.016f;
nib_call(&vm, f);
float r = nib_ret(&vm, f)->f;          // if it returns something
int *score = nib_global(&vm, "score"); // direct access to globals
```

String or blob FFI arguments take two words (address, length); use `nib_ptr(vm, &a[i])` to get the bytes. An FFI function can abort the script with `nib_trap(vm, "msg")`. FFI functions must not call back into the VM.

You can change the limits (symbols, procs, constants and so on) with `-DNIB_MAX_...`. See `nib.h`.

## Performance

These are from `bench/`, comparing against Lua 5.5 on the same machine. Times are in seconds.

| benchmark | nib (clang, computed goto) | nib (MSVC, switch) | Lua 5.5 |
|---|---|---|---|
| loop: 100M int ops | 0.46 | 0.58 | 1.51 |
| calls: 30M proc calls | 0.32 | 0.35 | 1.31 |
| particles: 10k structs × 1000 steps | 0.27 | 0.39 | 0.93 |
| sieve: 2M, ×10 | 0.61 | 0.59 | 1.44 |

nib comes out 2.4–4.2× faster. The speed comes from static types (no tag checks), absolute-slot operands (static frames), constants preloaded in memory, compare-and-branch fusion, a dedicated `for` loop op, and destination retargeting that removes most moves.

## Layout

```
src/nib.h     public API (+ the fixed-size state struct)
src/nib.c     lexer, single-pass compiler, VM
src/main.c    CLI host (adds ffi: clock, putc)
tests/        test suite (run by ctest)
bench/        nib vs lua benchmarks
```
