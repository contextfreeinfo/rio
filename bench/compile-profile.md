# Where compiling's time goes, on the RP2350

Profiled 2026-11-02, Rio at `3d57d93` in rinc's firmware on the Fruit Jam (RP2350 at 252 MHz, the
compiler running from flash through its 16 KB cache, sources read from the SD card). The work: rinc's
editor checking itself (`pc.checkRio`: `rio_check` of `system/edit/edit.rio`, ~2,600 lines in 9 files,
69 KB, the last part cut short as an editor cuts it), and a check of an empty file, which is just the
`pc` module (an auto-import, its source in memory) that every rinc compile compiles first.

## How

A copy of Rio's source, never committed: each function below renamed `_raw` and wrapped in one that adds
the Cortex-M33's cycle counter (DWT `CYCCNT`) around the call into a bucket; the two calls of a text's
`read` (refilling the source window) wrapped the same way; `rio_check` zeroing the buckets and printing
them, in microseconds, after each check. Built into a separate rinc firmware (`RIO_DIR` pointing at the
copy), flashed, and a small app timing checks with `pc.time` around them. The buckets nest: reading is
inside lexing (the lexer refills the window), and a recursive `rio_lookupMethod` counts twice.

## Results

| | the editor | `pc` alone |
|---|---|---|
| whole check | 763 ms | 30 ms |
| lexing (`rio_lex`, reading included) | 300 ms (39%) | 13.8 ms (46%) |
| reading the source (the window's refills) | 150 ms (19%) | 0 (in memory) |
| name lookups (`rio_lookup`, `rio_lookupExport`, `rio_lookupMethod`) | 55 ms (7%) | 2.5 ms (8%) |
| constants (`rio_findConsts`) | 24 ms (3%) | 0.02 ms |
| emitting (`rio_emit`) | 11 ms (1%) | 0.15 ms |
| symbols added, types made | 3 ms | 0.4 ms |
| the rest (parsing, types, codegen: not yet split) | ~370 ms (48%) | ~13 ms |

The same checks on a desktop: 4.2 ms and 0.4 ms. Reading the 9 files whole, outside a check, takes
78 ms on the board.

## What it says

- The lexer is the biggest single cost: ~150 ms of its own for the editor, and for `pc`, from memory,
  ~250 cycles a byte. Suspects: each name compared with every keyword (`strlen` and `memcmp`, 22 of
  them), the operator tables scanned the same way, and its code in flash, missing the cache. A switch
  on a name's length and first letter, and the lexer's hot loop in SRAM (as the interpreter's is), are
  the first things to try.
- Reading is a fifth, twice what reading the files whole costs: 2 KB windows, refilled a part at a
  time, and a file reopened (a FAT cluster walk to seek) at each include and back. Bigger windows, files
  kept open, or (later) sources cached in PSRAM.
- Lookups are only 7%: hashing them is worth less than thought, and comes after the lexer.
- Half the time is in the rest of the compiler, to be split by a finer profile (statements, expressions,
  declarations, the operator stack) before choosing what's next there.
- pc is 30 ms of every rinc compile and check: keeping its compiled tables, and copying them in, would
  save that much each time (most of a small app's compile).
