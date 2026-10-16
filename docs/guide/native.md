# Native loops on ARM

On ARM with Thumb-2, such as the RP2350's Cortex-M33, `rio_jit(vm, buf, size)` turns on a small
ahead-of-time compiler: from then on, everything that compiles or loads (programs, REPL inputs and
images) also has its simple loops and procs compiled to machine code in `buf` before running. `buf`
must be memory the CPU can run code from (any SRAM on the RP2350).

- **Which loops:** ones made of integer and float math (math builtins included), compares, branches
  (nested loops, `break`, `continue` and labels, and `for` over arrays), array and field reads and
  writes, `log`, and calls to host procs and to native procs. Others stay bytecode (calls through proc
  values, for one).
- **Which procs:** whole procs made of the same things, except memory reads and writes (a proc with
  those does better with only its loops compiled, which keep values in registers). Native code calls a
  native proc directly; with one frame per proc, a call is just a branch. `sqrt`, `abs`, `floor` and
  `ceil` are single instructions (`floor` and `ceil` on ARMv8, such as the M33); the other math builtins
  and `log` call into C.
- **Same behavior:** values stay in the VM's memory (a loop's busiest values and constants sit in
  registers while it runs), so the interpreter and native code mix freely. A runtime error in native
  code is reported with its line, as if the VM had run it, and `rio_interrupt` stops native code too.
- **Measured** on the Fruit Jam at 252 MHz, native vs the VM: `for n < 10_000_000 / n += 1` 0.33 s vs
  1.75 s; a million `n += i % 7` 0.09 s vs 0.28 s; 300,000 calls to a small proc 0.087 s vs 0.168 s;
  500 particles (structs of floats) for 100 steps 0.015 s vs 0.042 s; filling the screen with `pset`
  0.019 s vs 0.040 s; 100,000 `sin` calls 0.078 s vs 0.090 s (mostly `sin` itself).
- **Size:** native code is about 4.3 times the bytecode it replaces, but only loops and qualifying procs
  get it (the 1,100-line test suite: 9.5 KB for 47 loops and procs). When `buf` runs out, the rest stays
  bytecode.

Elsewhere, `rio_jit` returns -1 and changes nothing. `rio -j` uses it on ARM Linux, which is how it's
tested: under `qemu-arm`, as ARMv7 and ARMv8, the test suite, `tests/jit*.rio` and the VM-vs-C
differential tester all agree.
