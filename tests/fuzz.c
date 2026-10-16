/* libFuzzer target: any input must either compile or fail with an error, never crash or misbehave,
   and whatever compiles must translate to C. Programs aren't run (random ones loop forever easily);
   tests/difftest.py runs generated programs instead. Build (clang) with every source in src but main.c
   and jit.c, then run:
     clang -g -O1 -fsanitize=fuzzer,address,undefined -Isrc <sources> tests/fuzz.c -o fuzz
     ./fuzz -dict=tests/fuzz.dict corpus/ tests/ bench/ */
#include <stdio.h>
#include <string.h>
#include "rio.h"
#include "aot.h"

static unsigned char mem[1 << 20], scratch[1 << 19];
static rio_Instr code[16384];

/* the input's size picks how: with a code buffer, or the code in mem, compiled in scratch (and to C)
   or overlaid on mem */
int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
  rio_Vm vm; int how = (int)(size % 3);
  if (size > (1 << 16)) return 0;
  rio_init(&vm, mem, sizeof mem, how ? 0 : code, 16384);
  if (how == 2) { rio_compile(&vm, (const char *)data, (uint32_t)size); return 0; }
  if (!rio_compileScratch(&vm, (const char *)data, (uint32_t)size, scratch, sizeof scratch)) {
    FILE *f = fopen("/dev/null", "w");
    if (f) { rio_aot(&vm, f, ""); fclose(f); }
  }
  return 0;
}
