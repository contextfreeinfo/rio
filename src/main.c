/* nib command line host.
   nib file.nib            run top-level code, then main() if present
   nib -c out.c file.nib   compile to a standalone C program instead */
#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "nib.h"
#include "aot.h"

static uint8_t mem[8 << 20];
static NibIns code[65535];
static char src[1 << 20];
static uint8_t scratch[2 << 20];
static Nib vm;

static void out(void *ud, const char *s, int n) { (void)ud; fwrite(s, 1, (size_t)n, stdout); fputc('\n', stdout); }
static void ffi_clock(Nib *v, NibVal *a) { (void)v; a[0].f = (float)clock() / CLOCKS_PER_SEC; }
static void ffi_putc(Nib *v, NibVal *a) { (void)v; fputc(a[0].i, stdout); }
/* the same host functions, as C source for AOT output */
static const char *ffi_src =
  "void nib_ffi_clock(NibVal *a) { a[0].f = (float)clock() / CLOCKS_PER_SEC; }\n"
  "void nib_ffi_putc(NibVal *a) { fputc(a[0].i, stdout); }\n";

int main(int argc, char **argv) {
  FILE *f; size_t n; int fn;
  const char *cout = 0, *path = argv[argc - 1];
  if (argc == 4 && !strcmp(argv[1], "-c")) cout = argv[2];
  else if (argc != 2) { fprintf(stderr, "usage: nib file.nib\n       nib -c out.c file.nib\n"); return 2; }
  if (!(f = fopen(path, "rb"))) { fprintf(stderr, "cannot open %s\n", path); return 2; }
  n = fread(src, 1, sizeof src, f);
  fclose(f);
  if (nib_init(&vm, mem, sizeof mem, code, 65535)) { fprintf(stderr, "init failed\n"); return 1; }
  nib_set_log(&vm, out, 0);
  nib_ffi(&vm, "clock", ">f", ffi_clock);
  nib_ffi(&vm, "putc", "i", ffi_putc);
  /* the C backend reads compiler tables after compiling, so they get their own buffer; running
     compiles in place, with the compiler's state overlaid on script memory */
  if (cout ? nib_compile_scratch(&vm, src, (uint32_t)n, scratch, sizeof scratch) : nib_compile(&vm, src, (uint32_t)n)) {
    fprintf(stderr, "%s: %s\n", path, nib_error(&vm));
    return 1;
  }
  if (cout) {
    int err;
    if (!(f = fopen(cout, "w"))) { fprintf(stderr, "cannot write %s\n", cout); return 2; }
    err = nib_aot(&vm, f, ffi_src);
    if (fclose(f) || err) { fprintf(stderr, "error writing %s\n", cout); return 1; }
    return 0;
  }
  if (nib_run(&vm) || ((fn = nib_func(&vm, "main")) >= 0 && nib_call(&vm, fn))) {
    fflush(stdout);
    fprintf(stderr, "%s: %s\n", path, nib_error(&vm));
    return 1;
  }
  return 0;
}
