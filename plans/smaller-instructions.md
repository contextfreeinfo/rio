# Smaller instructions: a plan, for review

Status: step 1 done (2026-11-13: operands from each proc's frame); step 2 (4-byte instructions) through 2.3a (35% smaller). ROADMAP: "Leaning yes: Smaller
instructions".

## Why

An instruction is 8 bytes today (`rio_Instr`: op, x, and three 16-bit operands a, b, c). Code is most of
an app's memory once it's big: Rinc's Cloudlamp has 5,555 instructions (43 KB), studio's Maps 12,553
(98 KB). Half the bytes would let the same app space hold about twice the code, within the same limits
(constraints stay; this is about doing more within them).

## What the code looks like now (measured)

Seven real apps compiled to images (Cloudlamp, Maps, Paint, Sound, Music, the editor, Files: 42,523
instructions in all), their operands counted:

| all three operands fit in | share of instructions |
|---|---|
| 8 bits | 10% |
| 10 bits | 26% |
| 11 bits | 68% |
| 12 bits | 92% |

- `x` is used by almost nothing (the math ops' function number): it can go into the op or an operand.
- 76 distinct ops are used; `mov` alone is 23% of all instructions.
- The operands are big because they're absolute: a slot is a word of memory counted from its start
  (`R` is `mem`), each proc's frame at a fixed place ([frameStart, frameEnd)), after the globals and
  constants. In a 1,000-slot app, nearly every operand is in the hundreds or thousands, though each
  proc's own frame is usually small.

So a plain 4-byte form (op and three 8-bit operands) fits only 10% today: halving needs operands that
are small, not just fewer bits.

## The options

### A. Frame-relative operands, 4 bytes, with an 8-byte wide form (halves most code)

- Operands count from the running proc's frame (`R = mem + frameStart`, set on call and ret, the caller's
  kept on the call stack beside its return address). A proc's locals and temporaries are then small
  numbers: most fit 8 bits.
- Globals and constants, which aren't in the frame, are reached by a wide form: an 8-byte instruction
  (a prefix op, then today's layout), or a few ops of their own (load or store a global, by a 16-bit
  number), whichever measures better.
- Common form: `op` 8 bits, `a`, `b`, `c` 8 bits each (4 bytes). Jumps and big constants use the wide
  form, or `b` and `c` together as one 16-bit field.

Estimate: if 70 to 85% of instructions fit the short form, code shrinks by 35 to 45%. The real number
needs one more measurement (below): how many operands are in their own proc's frame.

Speed:
- Decode is byte loads instead of halfword loads: the same work.
- A call or ret does one more load and store, for the frame base.
- Wide instructions cost an extra dispatch, or a slightly slower op.
- Smaller code fits caches better on the desktop; on the RP2350 the code is in SRAM, so no gain there.
- Expected: within about 10% either way. The bench (`bench/*.rio`: loop, calls, sieve, particles) must
  show it on the Fruit Jam, and a slowdown over about 5% on `loop` or `particles` is a reason to stop.

Cost: a large change to codegen (every operand), the VM (`R` per frame), the JIT and AOT (they read
operands), images (a new version), and the debugger-facing tables (lines, procs). Recursion and shared
frames need care. Several days of work, done behind the image version bump.

### B. 6-byte instructions, 13-bit operands (a quarter off, little risk)

- `op` 8 bits (x folded in), `a`, `b`, `c` 13 bits each: 47 bits, in 6 bytes.
- Operands stay absolute, so codegen barely changes: 8,192 slots is enough for every app measured (12
  bits already fits 92%); a bigger one uses the wide form, as in A.
- Decode: a 32-bit and a 16-bit load (unaligned on the RP2350's Cortex-M33, which allows it), then
  shifts and masks: a few more ALU instructions per dispatch, maybe 3 to 8% slower on tight loops.

Saves 25%: less than A, but days less work, and much less risk.

### C. Leave it

Memory is managed by keeping apps small (Maps made smaller on its own first). This fits the
constraints-first approach, and costs nothing.

## Measured: A's short share (2026-11-10)

`tools/opwidths.py` (the same seven images) sorts each operand: in its own proc's frame and under 256
from its start; under 256 anyway (an immediate, an offset, a low constant); a jump within 127
instructions; or far. An instruction is short with no far operand.

| A, as designed | short | bytes an instruction, on average | code saved |
|---|---|---|---|
| frame-relative operands only | 56% | 5.8 | 28% |
| and calls by proc number (every app has under 256 procs: 90 to 247) | 68% | 5.3 | 34% |
| and windowed frames (a callee's frame starts where its caller's ends, so arguments and results are near) | 81% | 4.8 | 40% |
| and constants reached short (a const-load op, or the most used constants numbered low) | about 87% | 4.5 | about 44% |

What's left long: globals (about 11% of instructions: they live past the frames), constants (6%),
top-level code (3%, run once). Jumps are almost all near (within 127): far jumps are under 1%.
Separately, the 5% that are data words (an op's extra word, op 255) could be 4 bytes too.

So A is worth it: about 40% off, as much as 44%. The user's limit (2026-11-10): up to a 10% slowdown
is acceptable for this much more code in the same space (and native Thumb-2, from the JIT or AOT, can
still serve an app's tight loops).

## Step 1 done (2026-11-13): operands from each proc's frame

Frames were already fixed places (no recursion: rio_placeFrames stacks each proc's frame above those of
everything it calls), so no new layout was needed: with today's layout and signed 8-bit offsets, 77% of
instructions already fit short (81% with a relayout). What changed:

- Every slot operand is now its slot less its proc's frameStart, plus rio_FRAME_BIAS (32768), in the
  same 16 bits; top-level code counts from 0 the same way. The vm's R is mem + frameStart - the bias,
  moved by call and ret (the return stack keeps the caller's R beside its pc: two words a call), and
  `call` names its proc by number. rio_codeFrames converts (the compiler, once frames are placed; the
  REPL, a piece at a time) and converts back for the JIT and the C backend, which still read absolute
  slots (the JIT's native code gets mem's start; its helper calls get R moved to the instruction's
  frame). A data word's slot stays absolute (read from mem's start), as do descriptors handed to the
  printer and checker.
- Operands name only the slots below 128 KB now (rio_SLOTS_END: was 256 KB), so any of them is in
  reach of a frame's biased offset; statics above it are reached by address, as those above 256 KB
  were (tests/highstatic.rio, at -M 256K as Rinc runs apps).
- Images: version 16. Rinc's board firmware: the vm in RAM grew by about 100 bytes, past what was free;
  RAM found elsewhere (below), so apps keep their 256 KB and the big stack its 6 KB.

Speed, on the Fruit Jam (bench.rio, seconds, the same both runs):

| | before | after |
|---|---|---|
| with the JIT: loop, calls, particles, pset | 0.0897, 0.0896, 0.0157, 0.0196 | the same |
| with the JIT: sin 100k (a helper call each) | 0.0785 | 0.0793 (+1%) |
| bytecode alone (RINC_NO_JIT): loop 1M | 0.2969 | 0.2930 (1% faster) |
| calls 300k | 0.1768 | 0.1881 (+6%: the frame base saved and restored) |
| particles, sin, pset | 0.0442, 0.0935, 0.0429 | 0.0445, 0.0943, 0.0429 (within 1%) |

No code is smaller yet: that's step 2.

## Step 2's design question (the RAM the vm runs in)

On the Fruit Jam the interpreter runs from SRAM (9.3 KB of it), and the firmware has no SRAM to spare
(step 1's hundred bytes needed the big stack trimmed). A 4-byte short form read at full speed needs
handlers that read 8-bit operands directly: decoding every instruction into 16-bit fields first would
cost far more than 10%. So the short form wants its own handlers for the ops it covers, beside the
8-byte ones. Ways to do it:

- Short handlers for the common ops only (`mov` is 23% of all instructions; about 30 ops cover most of
  the rest), every other op wide only. Perhaps 1.5 to 2 KB more SRAM for the vm. Not from the app
  space (the user, 2026-11-13: apps keep 256 KB): found in Rinc's firmware instead. PIO-USB (the USB-A
  ports) keeps its code in RAM, and GCC had made three copies of its packet encoder, one per constant
  argument (5.6 KB); -fno-ipa-cp-clone for its sources keeps one, freeing 3.7 KB (the PIO times the
  bits, so the encoder's speed doesn't matter). 3.7 KB of RAM is free now.
- Pcs counted in 4-byte units, a wide instruction taking two, so jumps, entries, line tables and the
  call stack stay indexes; jumps relative (8 bits short, 16 wide). The JIT and C backend read through a
  decoding helper.
- The cold handlers (rare ops, wide forms) could move to flash, to keep the vm's SRAM where it is now.

## Step 2's layouts (measured 2026-11-13, after step 1)

Measured on the frame-relative code (the same seven apps: 40,212 instructions, and 2,468 data words),
a plain short form (three 8-bit operands, all from the frame) fits only 30%: the earlier estimate had
counted operands under 256 as near, which includes the constants, low in memory, now far from a frame.
Counting each operand as F (its frame, within 128), K (a constant: an absolute slot under 256; every
app has 500 to 1,350 constant slots, the hot ones low), G (any other: a global), with the operands an
op doesn't use (zero) left out:

| operands | share of instructions |
|---|---|
| frame only, or none (jumps, calls, ret) | 55% |
| frame and one constant | 21% |
| frame and one global | 15% |
| others | 9% |

So three short layouts, 4 bytes each, each read through a base of its own (no choosing per operand,
which would cost too much in every instruction):

- S3: op, a, b, c, each 8 bits: from the frame (R, biased by 128), or small numbers (offsets, sizes,
  counts; a jump's target relative, in units; a call's proc number).
- SK: op, a, b from the frame, k a constant (mem's start + k): for `add a, b, 1` and its kin.
- SG: op, f from the frame, then a 16-bit absolute slot: `mov` to or from a global (most of the 15%).

An instruction that fits none stays as today: 8 bytes, two units, then its data word (8 bytes) if it
has one. With about 87% short, the average is about 4.5 bytes: about 43% smaller. Short handlers for
the ops that matter (about 30 in S3, 10 in SK, 4 in SG) and a 256-entry dispatch table: about 1.5 to 2
KB of RAM on the Fruit Jam, inside the 3.7 KB found.

Plumbing: pcs count 4-byte units (a wide instruction two, its data word two more), so the compiler's
pcs (jumps, proc entries and ends, lines) are remapped once, when a packer rewrites the finished code
in place (the compiler keeps emitting 8-byte instructions, so its fusing and patching don't change).
The REPL's pieces stay wide (two units each, so its pcs just double). The JIT and the C backend read
the code through a decoder (or, for the C backend on the computer, an unpacked copy).

## Milestones measured (2026-11-13)

Seven apps' code (Cloudlamp, Maps, Paint, Sound, Music, the editor, Files; tools/packstats.py), and
bench.rio on the Fruit Jam (seconds; the bytecode alone with RINC_NO_JIT; the same in both runs):

| | code | loop | calls | particles | sin | pset |
|---|---|---|---|---|---|---|
| before (8158967) | 100% | 0.2969 | 0.1768 | 0.0442 | 0.0935 | 0.0429 |
| step 1: frames | 100% | 0.2930 | 0.1881 | 0.0445 | 0.0943 | 0.0429 |
| 2.1: units | 100% | 0.2972 | 0.1893 | 0.0447 | 0.0939 | 0.0432 |
| 2.2: S3 packed | 76% (24% smaller; Maps 100 KB to 76) | 0.3511 | 0.2091 | 0.0478 | 0.0981 | 0.0477 |
| 2.3a: SG and SK | 65% (35% smaller; Maps 100 KB to 66) | 0.3388 | 0.2045 | 0.0473 | 0.0993 | 0.0474 |

With the JIT (what apps run on the board), every step: loop 0.0897, calls 0.0896, particles 0.0157,
sin 0.0793 to 0.0797, pset 0.0197: the same.

What 2.2 found:
- Short handlers in the interpreter made every wide one 15% slower, even with no short code running:
  GCC's global common subexpressions merged the handlers' computed gotos into one shared dispatch (a
  branch more an instruction) once the function grew. optimize("no-gcse") on rio_interpret, GCC's own
  advice for interpreters, took that back to within 2%.
- A second base pointer for short operands (S) cost registers: wide operands now take the same bias
  as short ones (rio_FRAME_BIAS 128), read as signed 16 bits, so R serves both.
- What's left (loop +18%, calls +18%, pset +11% over before) is in running packed code: the same
  three instructions a loop, short handlers the same ARM instructions as wide ones, so most likely
  code and data layout in the shared SRAM (aligning the handlers' labels moved it about 3% either way,
  sin the other way): unexplained yet.
- The vm's RAM: 11.8 KB of interpreter and a 1 KB dispatch table (2.5 + 0.5 KB more); 812 bytes of
  the firmware's RAM left, not enough for step 2.3's handlers (SK, SG) without finding more.
- RAM found for 2.3 (rinc's src/rp2350/link, through the SDK's linker-script overrides): PIO-USB's
  setup (run once) and libgcc's code (64-bit division, doubles) kept in flash, not RAM: 3.3 KB, so
  about 4.1 KB free. Bench, sprites and USB the same. (newlib's malloc stays: PIO-USB's timer pool is
  made with it, and changing that would touch USB's timing.)

What 2.3a found and did (2026-11-13):
- Why the 48% still wide were wide (packed images, each wide instruction's operands sorted): 25% of all
  instructions held a constant (almost always one under 256), 15% a global, 10% an op with no short
  form (slice, lview, pushs, ldxb, idx, not, div...); far jumps 0.7%.
- Two more layouts, as more forms of existing ops (internal.h: rio_SHORT_MORE; the packer tries an
  op's forms in order, and swaps a commuting op's operands to reach one): SG, a frame slot and any slot
  (16 bits from mem's start), for mov and mov2 either way (globals, and constants of any size); SK, the
  last operand a constant under 256, for 16 ops (add, sub, mul, and, or, eq, ne, lt, le, imin, imax,
  fadd, fsub, fmul, flt, fle). 74% of instructions short now: code 35% smaller than all wide.
- The interpreter's M (mem's start) was a register already, so a K or G operand is one load, no new
  base. Bytecode alone came out a little faster than 2.2 (more of it short). The vm's RAM: 856 bytes
  more, about 3.3 KB of the firmware's left. With the JIT, the same as ever.
- tests/shortforms.rio and .py: each form chosen (read from the image) and its results checked (worked
  out apart); the test pads globals so a proc's frame is past the constants' and globals' reach.
- Left, measured: the constant as the second operand (lt and le as gt and ge, compare-and-jumps,
  ldw/stw from a constant base) would make about 1,500 more short (3.7% of instructions), about 2.5%
  smaller again, for perhaps 16 more handlers (some 700 bytes of RAM). jz and jnz on a global: 100.

Decided (2026-11-13): 2.2's speed accepted for its size (Maps 24 KB smaller), since apps on the board
run hot code through the JIT. Drawing barely depends on the vm: on the Fruit Jam (rinc's
apps/drawbench.rio; rinc's docs/guide/fruit-jam.md has the table), 8x8 rectangles drew as many
with the JIT as without it (2,960 in 10 ms), and sprites came within 4% on 2.2's bytecode and before
step 1. Logic does:
falling sand on a 240x240 grid steps in 13.4 ms with the JIT, 54.7 ms on bytecode alone.

## Next steps

1. Windowed frames first, on their own, with today's 8-byte instructions: each proc's frame is placed
   by the callee's depth, not at a fixed address, and `R` is set per frame on call and ret. This is
   the deep change (codegen's frame layout, calls, recursion and shared frames, the JIT and AOT's
   reading of slots), and it can be tested fully (every test, the bench) before any encoding changes.
   Measure its speed alone.
2. Then the short form: `op` 8 bits, `a`, `b`, `c` 8 bits each; jumps relative; calls by proc number;
   an 8-byte wide form (a prefix op, then today's layout) for the rest. The image version goes up.
3. Then constants reached short, if the numbers still say so.
4. Bench on the desktop and the Fruit Jam at each step (`bench/*.rio`): stop if past 10% slower.

Either way, the instruction layout lives in `rio.h` (`rio_Instr`), `internal.h` (the op list), `vm.c`
(dispatch), `codegen.c`, `jit.c`, `aot.c` and `image.c`: the image version goes up, and Rinc's cached
images are made again (they're checked by version already).

## How the numbers were made

Each app compiled with `rinc app.rio -i app.rioi` (Rinc's Cloudlamp, studio's Maps, Paint, Sound and
Music, and Rinc's editor and Files), then `python tools/opwidths.py *.rioi`, which reads each image's
header, proc table and code. The later rows of the table came from the same reading, with calls and
other procs' frames counted as near (a few lines more in that script, when step 1 starts).
