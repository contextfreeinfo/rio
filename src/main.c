/* nib command line host: nib file.nib  -- runs top-level code, then main() if present. */
#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <time.h>
#include "nib.h"

static uint8_t mem[8 << 20];
static NibIns code[65535];
static char src[1 << 20];
static Nib vm;

static void out(void *ud, const char *s, int n) { (void)ud; fwrite(s, 1, (size_t)n, stdout); fputc('\n', stdout); }
static void ffi_clock(Nib *v, NibVal *a) { (void)v; a[0].f = (float)clock() / CLOCKS_PER_SEC; }
static void ffi_putc(Nib *v, NibVal *a) { (void)v; fputc(a[0].i, stdout); }

int main(int argc, char **argv) {
  FILE *f; size_t n; int fn;
  if (argc < 2) { fprintf(stderr, "usage: nib file.nib\n"); return 2; }
  if (!(f = fopen(argv[1], "rb"))) { fprintf(stderr, "cannot open %s\n", argv[1]); return 2; }
  n = fread(src, 1, sizeof src, f);
  fclose(f);
  if (nib_init(&vm, mem, sizeof mem, code, 65535)) { fprintf(stderr, "init failed\n"); return 1; }
  nib_set_log(&vm, out, 0);
  nib_ffi(&vm, "clock", ">f", ffi_clock);
  nib_ffi(&vm, "putc", "i", ffi_putc);
  if (nib_compile(&vm, src, (uint32_t)n) || nib_run(&vm) ||
      ((fn = nib_func(&vm, "main")) >= 0 && nib_call(&vm, fn))) {
    fflush(stdout);
    fprintf(stderr, "%s: %s\n", argv[1], nib_error(&vm));
    return 1;
  }
  return 0;
}
