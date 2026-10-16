# Performance

From `bench/`, against Lua 5.5 on the same Windows machine. Seconds, best of 5.

| benchmark | VM (clang, computed goto) | VM (MSVC, switch) | C output (clang -O2) | Lua 5.5 |
|---|---|---|---|---|
| loop: 100M int ops | 0.46 | 0.55–0.64 | 0.006 | 1.5–1.7 |
| calls: 30M proc calls | 0.36 | 0.34 | 0.010 | 1.3–1.5 |
| particles: 10k structs × 1000 steps | 0.28 | 0.39 | 0.014 | 0.9–1.0 |
| sieve: 2M, ×10 | 0.48 | 0.49 | 0.14 | 1.4–1.6 |

- **VM:** about 3–4× faster than Lua here. On Linux with a faster Lua build, particles measured 0.29 s
  vs 0.59 s, about 2×, so expect roughly 2–4× depending on the Lua build. The speed comes from static
  types (no tag checks), absolute-slot operands (static frames), constants preloaded in memory,
  compare-and-branch fusion, fused index+load/store, bottom-tested `while` loops, a dedicated `for` loop
  op, and destination retargeting that removes most moves.
- **C output:** sieve is about 3.5× faster than the VM. The other three run in milliseconds because the
  C compiler sees through those loops entirely (inlining, vectorizing or folding them), so treat them as
  an upper bound rather than typical.
- The particles sums differ from Lua only because Rio's `Float` is 32-bit: Lua with 32-bit rounding
  emulated gives the identical 43926.92.
- `switch` dispatch on MSVC is sensitive to code layout: the same VM source has measured 0.34 s or 0.65 s
  on `calls` depending only on how unrelated code shifted. Expect build-to-build variation with MSVC;
  ClangCL avoids it.
