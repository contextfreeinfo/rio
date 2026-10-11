/* a host built with RIO_NO_COMPILER: runs an image made by rio -i (with the CLI's ffi functions) */
#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <time.h>
#include "rio.h"

static unsigned char mem[8 << 20], img[4 << 20];
static RioIns code[65535];
static Rio vm;

static void out(void *ud, const char *s, int n) { (void)ud; fwrite(s, 1, (size_t)n, stdout); fputc('\n', stdout); }
static void ffi_clock(Rio *v, RioVal *a) { (void)v; a[0].f = (float)clock() / CLOCKS_PER_SEC; }
static void ffi_putc(Rio *v, RioVal *a) { (void)v; fputc(a[0].i, stdout); }

int main(int argc, char **argv) {
  FILE *f; size_t n; int fn;
  if (argc != 2 || !(f = fopen(argv[1], "rb"))) { fprintf(stderr, "usage: rio_vm file.img\n"); return 2; }
  n = fread(img, 1, sizeof img, f); fclose(f);
  rio_init(&vm, mem, sizeof mem, code, 65535);
  rio_set_log(&vm, out, 0);
  rio_ffi(&vm, "clock", ">f", ffi_clock);
  rio_ffi(&vm, "putc", "i", ffi_putc);
  if (rio_image_load(&vm, img, (uint32_t)n) || rio_run(&vm) || ((fn = rio_func(&vm, "main")) >= 0 && rio_call(&vm, fn))) {
    fflush(stdout); fprintf(stderr, "%s: %s\n", argv[1], rio_error(&vm)); return 1;
  }
  return 0;
}
