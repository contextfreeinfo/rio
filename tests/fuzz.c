/* libFuzzer target: any input must either compile or fail with an error, never crash or misbehave,
   and whatever compiles must translate to C. Programs aren't run (random ones loop forever easily);
   tests/difftest.py runs generated programs instead. Build and run (clang):
     clang -g -O1 -fsanitize=fuzzer,address,undefined -Isrc src/rio.c src/aot.c tests/fuzz.c -o fuzz
     ./fuzz -dict=tests/fuzz.dict corpus/ tests/ bench/ */
#include <stdio.h>
#include <string.h>
#include "rio.h"
#include "aot.h"

static unsigned char mem[1 << 20], scratch[1 << 19];
static RioIns code[16384];

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
  Rio vm;
  if (size > (1 << 16)) return 0;
  rio_init(&vm, mem, sizeof mem, code, 16384);
  if (!rio_compile_scratch(&vm, (const char *)data, (uint32_t)size, scratch, sizeof scratch)) {
    FILE *f = fopen("/dev/null", "w");
    if (f) { rio_aot(&vm, f, ""); fclose(f); }
  }
  return 0;
}
