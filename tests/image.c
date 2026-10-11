/* images: compile, save, load into a fresh vm, run; ffi binding by name; bad images */
#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <string.h>
#include "rio.h"

static unsigned char mem[1 << 18], mem2[1 << 19], img[1 << 16];
static RioIns code[4096], code2[4096];
static int fails, ntests;
static char logged[256];

static void out(void *ud, const char *s, int n) { (void)ud; if (strlen(logged) + (size_t)n + 2 < sizeof logged) { strncat(logged, s, (size_t)n); strcat(logged, "|"); } }
static void f_add(Rio *v, RioVal *a) { (void)v; a[0].i = a[0].i + a[1].i; }
static void f_half(Rio *v, RioVal *a) { (void)v; a[0].f = a[0].f / 2; }
static void f_other(Rio *v, RioVal *a) { (void)v; (void)a; }

static void check(int ok, const char *what, Rio *vm) {
  ntests++;
  if (!ok) { fails++; printf("FAIL: %s (%s)\n", what, vm ? rio_error(vm) : ""); }
}
/* compile src in a vm of memsize bytes with add and half, save the image; 0 on failure */
static uint32_t save(const char *src, uint32_t memsize) {
  Rio vm; uint32_t n;
  rio_init(&vm, mem, memsize, code, 4096);
  rio_ffi(&vm, "add", "ii>i", f_add); rio_ffi(&vm, "half", "f>f", f_half); rio_ffi(&vm, "other", "", f_other);
  if (rio_compile(&vm, src, (uint32_t)strlen(src))) { printf("FAIL: compile: %s\n", rio_error(&vm)); fails++; return 0; }
  n = rio_image_save(&vm, 0, 0);
  check(n > 0 && rio_image_save(&vm, img, n - 1) == 0, "save needs room for the whole image", &vm);
  check(rio_image_save(&vm, img, sizeof img) == n, "save writes the size it reported", &vm);
  return n;
}

int main(void) {
  const char *src =
    "msg := \"hi\"\ncount* := 0\n"
    "twice :: proc(x: Int) -> Int\n  return add(x, x)\nend\n"
    "bump* :: proc(by: Int)\n  count += by\nend\n"
    "main :: proc()\n  log(msg, twice(20), half(3.0))\n  xs: [2]Int\n  i := count\n  log(xs[i])\nend\n";
  Rio vm; uint32_t n; int fn; unsigned char ver;
  /* round trip, with the ffi functions registered in another order */
  n = save(src, sizeof mem);
  rio_init(&vm, mem2, sizeof mem2, code2, 4096);
  rio_set_log(&vm, out, 0);
  rio_ffi(&vm, "half", "f>f", f_half); rio_ffi(&vm, "add", "ii>i", f_add);
  check(rio_image_load(&vm, img, n) == 0, "load", &vm);
  check(rio_run(&vm) == 0 && (fn = rio_func(&vm, "main")) >= 0 && rio_call(&vm, fn) == 0, "run main", &vm);
  check(!strcmp(logged, "hi 40 1.5|0|"), "output after binding ffi by name", &vm);
  check(rio_func(&vm, "bump") >= 0 && rio_global(&vm, "count") != 0, "exports after load", &vm);
  fn = rio_func(&vm, "bump"); rio_args(&vm, fn)[0].i = 5;
  check(rio_call(&vm, fn) == 0 && *(int32_t *)rio_global(&vm, "count") == 5, "call with args after load", &vm);
  check(rio_call(&vm, rio_func(&vm, "main")) != 0 && !strcmp(rio_error(&vm), "13: runtime error: index out of bounds"), "runtime error line", &vm);
  /* a fresh vm reloads the same image again from scratch */
  check(rio_image_load(&vm, img, n) == 0 && rio_call(&vm, rio_func(&vm, "main")) == 0, "reload resets memory", &vm);
  /* ffi binding */
  rio_init(&vm, mem2, sizeof mem2, code2, 4096);
  rio_ffi(&vm, "add", "ii>i", f_add);
  check(rio_image_load(&vm, img, n) == -1 && strstr(rio_error(&vm), "didn't register"), "a called ffi function is missing", &vm);
  rio_init(&vm, mem2, sizeof mem2, code2, 4096);
  rio_ffi(&vm, "add", "ii>i", f_add); rio_ffi(&vm, "half", "i>f", f_half);
  check(rio_image_load(&vm, img, n) == -1 && strstr(rio_error(&vm), "sig differs"), "an ffi sig differs", &vm);
  /* memory and code room */
  rio_init(&vm, mem2, sizeof mem / 2, code2, 4096);
  rio_ffi(&vm, "add", "ii>i", f_add); rio_ffi(&vm, "half", "f>f", f_half);
  check(rio_image_load(&vm, img, n) == -1 && strstr(rio_error(&vm), "needs more memory"), "too little memory", &vm);
  rio_init(&vm, mem2, sizeof mem2, code2, 16);
  rio_ffi(&vm, "add", "ii>i", f_add); rio_ffi(&vm, "half", "f>f", f_half);
  check(rio_image_load(&vm, img, n) == -1 && strstr(rio_error(&vm), "code too large"), "too little code room", &vm);
  /* damaged images */
  rio_init(&vm, mem2, sizeof mem2, code2, 4096);
  rio_ffi(&vm, "add", "ii>i", f_add); rio_ffi(&vm, "half", "f>f", f_half);
  check(rio_image_load(&vm, img, n - 1) == -1 && strstr(rio_error(&vm), "wrong size"), "truncated", &vm);
  check(rio_image_load(&vm, img, 10) == -1 && strstr(rio_error(&vm), "too short"), "too short", &vm);
  img[0] = 'X';
  check(rio_image_load(&vm, img, n) == -1 && strstr(rio_error(&vm), "not a rio image"), "bad magic", &vm);
  img[0] = 'R'; ver = img[4]; img[4] = 99;
  check(rio_image_load(&vm, img, n) == -1 && strstr(rio_error(&vm), "unsupported version"), "bad version", &vm);
  img[4] = ver; img[52]++;
  check(rio_image_load(&vm, img, n) == -1 && strstr(rio_error(&vm), "bad header"), "inconsistent header", &vm);
  img[52]--;
  check(rio_image_load(&vm, img, n) == 0 && rio_run(&vm) == 0, "restored image loads", &vm);
  /* code in mem: no code buffer when compiling, loading, or either */
  {
    Rio a; static unsigned char ma[1 << 18]; uint32_t m;
    rio_init(&a, ma, sizeof ma, 0, 0);
    rio_ffi(&a, "add", "ii>i", f_add); rio_ffi(&a, "half", "f>f", f_half);
    check(rio_compile(&a, src, (uint32_t)strlen(src)) == 0, "compile with the code in mem", &a);
    m = rio_image_save(&a, img, sizeof img);
    check(m > 0 && m < n, "an image with the code in mem (the code is part of the block)", &a);
    logged[0] = 0;
    rio_init(&vm, mem2, sizeof mem2, 0, 0);
    rio_set_log(&vm, out, 0); rio_ffi(&vm, "add", "ii>i", f_add); rio_ffi(&vm, "half", "f>f", f_half);
    check(rio_image_load(&vm, img, m) == 0 && rio_run(&vm) == 0 && rio_call(&vm, rio_func(&vm, "main")) == 0 &&
          !strcmp(logged, "hi 40 1.5|0|"), "load and run with the code in mem", &vm);
    logged[0] = 0;
    rio_init(&vm, mem2, sizeof mem2, code2, 4096);
    rio_set_log(&vm, out, 0); rio_ffi(&vm, "add", "ii>i", f_add); rio_ffi(&vm, "half", "f>f", f_half);
    check(rio_image_load(&vm, img, m) == 0 && rio_run(&vm) == 0 && rio_call(&vm, rio_func(&vm, "main")) == 0 && !strcmp(logged, "hi 40 1.5|0|"),
          "a vm with a code buffer runs an image whose code is in mem", &vm);
    n = save(src, sizeof mem); /* code buffer */
    rio_init(&vm, mem2, sizeof mem2, 0, 0);
    rio_ffi(&vm, "add", "ii>i", f_add); rio_ffi(&vm, "half", "f>f", f_half);
    check(rio_image_load(&vm, img, n) == -1 && strstr(rio_error(&vm), "separate buffer"), "an image with its code apart needs a code buffer", &vm);
    rio_init(&vm, mem2, sizeof mem2, code2, 4096);
    rio_ffi(&vm, "add", "ii>i", f_add); rio_ffi(&vm, "half", "f>f", f_half);
    check(rio_image_load(&vm, img, n) == 0 && rio_run(&vm) == 0, "back to a code buffer", &vm);
  }
  /* saving */
  check(rio_image_save(&vm, img, sizeof img) == 0, "no saving after running", &vm);
  n = save("x := 1\n", sizeof mem);
  check(n > 0, "a tiny program", 0);
  if (!fails) printf("all %d image tests passed\n", ntests);
  return fails != 0;
}
