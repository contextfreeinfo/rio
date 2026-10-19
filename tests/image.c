/* images: compile, save, load into a fresh vm, run; ffi binding by name; bad images */
#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <string.h>
#include "rio.h"

static unsigned char mem[1 << 18], mem2[1 << 19], img[1 << 16];
static rio_Instr code[4096], code2[4096];
static int fails, ntests;
static char logged[256];

static void out(void *ud, const char *s, int n) { (void)ud; if (strlen(logged) + (size_t)n + 2 < sizeof logged) { strncat(logged, s, (size_t)n); strcat(logged, "|"); } }
static void f_add(rio_Vm *v, rio_Val *a) { (void)v; a[0].i = a[0].i + a[1].i; }
static void f_half(rio_Vm *v, rio_Val *a) { (void)v; a[0].f = a[0].f / 2; }
static void f_other(rio_Vm *v, rio_Val *a) { (void)v; (void)a; }

/* an image going to and from img2 a part at a time: a write fails at stopAt (0: none); a read gets at
   most "most" bytes a call (it asks again for the rest), and fails past len */
static unsigned char img2[1 << 16];
static struct { uint32_t at, stopAt, len, most; } stream;
static int toStream(void *ud, const void *bytes, uint32_t n) {
  (void)ud;
  if (stream.stopAt && stream.at + n > stream.stopAt) return 1;
  memcpy(img + stream.at, bytes, n);
  stream.at += n;
  return 0;
}
static int fromStream(void *ud, void *bytes, uint32_t n) {
  uint32_t got = 0;
  (void)ud;
  while (got < n) {
    uint32_t k = n - got < stream.most ? n - got : stream.most;
    if (stream.at + k > stream.len) return 1;
    memcpy((unsigned char *)bytes + got, img2 + stream.at, k);
    stream.at += k;
    got += k;
  }
  return 0;
}

static void check(int ok, const char *what, rio_Vm *vm) {
  ntests++;
  if (!ok) { fails++; printf("FAIL: %s (%s)\n", what, vm ? rio_error(vm) : ""); }
}
/* compile src in a vm of memsize bytes with add and half, save the image; 0 on failure */
static uint32_t save(const char *src, uint32_t memsize) {
  rio_Vm vm; uint32_t n;
  rio_init(&vm, mem, memsize, code, 4096);
  rio_addFfi(&vm, "add", "ii>i", f_add); rio_addFfi(&vm, "half", "f>f", f_half); rio_addFfi(&vm, "other", "", f_other);
  if (rio_compile(&vm, src, (uint32_t)strlen(src))) { printf("FAIL: compile: %s\n", rio_error(&vm)); fails++; return 0; }
  n = rio_saveImage(&vm, 0, 0);
  check(n > 0 && rio_saveImage(&vm, img, n - 1) == 0, "save needs room for the whole image", &vm);
  check(rio_saveImage(&vm, img, sizeof img) == n, "save writes the size it reported", &vm);
  return n;
}

int main(void) {
  const char *src =
    "msg := \"hi\"\ncount* := 0\n"
    "twice :: proc(x: Int) -> Int\n  return add(x, x)\nend\n"
    "bump* :: proc(by: Int)\n  count += by\nend\n"
    "main :: proc()\n  log(msg, twice(20), half(3.0))\n  xs: [2]Int\n  i := count\n  log(xs[i])\nend\n";
  rio_Vm vm; uint32_t n; int fn; unsigned char ver;
  /* round trip, with the ffi functions registered in another order */
  n = save(src, sizeof mem);
  rio_init(&vm, mem2, sizeof mem2, code2, 4096);
  rio_setLog(&vm, out, 0);
  rio_addFfi(&vm, "half", "f>f", f_half); rio_addFfi(&vm, "add", "ii>i", f_add);
  check(rio_loadImage(&vm, img, n) == 0, "load", &vm);
  check(rio_run(&vm) == 0 && (fn = rio_findProc(&vm, "main")) >= 0 && rio_call(&vm, fn) == 0, "run main", &vm);
  check(!strcmp(logged, "hi 40 1.5|0|"), "output after binding ffi by name", &vm);
  check(rio_findProc(&vm, "bump") >= 0 && rio_findGlobal(&vm, "count") != 0, "exports after load", &vm);
  fn = rio_findProc(&vm, "bump"); rio_procArgs(&vm, fn)[0].i = 5;
  check(rio_call(&vm, fn) == 0 && *(int32_t *)rio_findGlobal(&vm, "count") == 5, "call with args after load", &vm);
  check(rio_call(&vm, rio_findProc(&vm, "main")) != 0 && !strcmp(rio_error(&vm), "13: runtime error: index out of bounds"), "runtime error line", &vm);
  /* a fresh vm reloads the same image again from scratch */
  check(rio_loadImage(&vm, img, n) == 0 && rio_call(&vm, rio_findProc(&vm, "main")) == 0, "reload resets memory", &vm);
  /* ffi binding */
  rio_init(&vm, mem2, sizeof mem2, code2, 4096);
  rio_addFfi(&vm, "add", "ii>i", f_add);
  check(rio_loadImage(&vm, img, n) == -1 && strstr(rio_error(&vm), "didn't register"), "a called ffi function is missing", &vm);
  rio_init(&vm, mem2, sizeof mem2, code2, 4096);
  rio_addFfi(&vm, "add", "ii>i", f_add); rio_addFfi(&vm, "half", "i>f", f_half);
  check(rio_loadImage(&vm, img, n) == -1 && strstr(rio_error(&vm), "sig differs"), "an ffi sig differs", &vm);
  /* memory and code room */
  rio_init(&vm, mem2, sizeof mem / 2, code2, 4096);
  rio_addFfi(&vm, "add", "ii>i", f_add); rio_addFfi(&vm, "half", "f>f", f_half);
  check(rio_loadImage(&vm, img, n) == -1 && strstr(rio_error(&vm), "needs more memory"), "too little memory", &vm);
  rio_init(&vm, mem2, sizeof mem2, code2, 16);
  rio_addFfi(&vm, "add", "ii>i", f_add); rio_addFfi(&vm, "half", "f>f", f_half);
  check(rio_loadImage(&vm, img, n) == -1 && strstr(rio_error(&vm), "code too large"), "too little code room", &vm);
  /* damaged images */
  rio_init(&vm, mem2, sizeof mem2, code2, 4096);
  rio_addFfi(&vm, "add", "ii>i", f_add); rio_addFfi(&vm, "half", "f>f", f_half);
  check(rio_loadImage(&vm, img, n - 1) == -1 && strstr(rio_error(&vm), "wrong size"), "truncated", &vm);
  check(rio_loadImage(&vm, img, 10) == -1 && strstr(rio_error(&vm), "too short"), "too short", &vm);
  img[0] = 'X';
  check(rio_loadImage(&vm, img, n) == -1 && strstr(rio_error(&vm), "not a rio image"), "bad magic", &vm);
  img[0] = 'R'; ver = img[4]; img[4] = 99;
  check(rio_loadImage(&vm, img, n) == -1 && strstr(rio_error(&vm), "unsupported version"), "bad version", &vm);
  img[4] = ver; img[52]++;
  check(rio_loadImage(&vm, img, n) == -1 && strstr(rio_error(&vm), "bad header"), "inconsistent header", &vm);
  img[52]--;
  check(rio_loadImage(&vm, img, n) == 0 && rio_run(&vm) == 0, "restored image loads", &vm);
  /* code in mem: no code buffer when compiling, loading, or either */
  {
    rio_Vm a; static unsigned char ma[1 << 18]; uint32_t m;
    rio_init(&a, ma, sizeof ma, 0, 0);
    rio_addFfi(&a, "add", "ii>i", f_add); rio_addFfi(&a, "half", "f>f", f_half);
    check(rio_compile(&a, src, (uint32_t)strlen(src)) == 0, "compile with the code in mem", &a);
    m = rio_saveImage(&a, img, sizeof img);
    check(m > 0 && m < n, "an image with the code in mem (the code is part of the block)", &a);
    logged[0] = 0;
    rio_init(&vm, mem2, sizeof mem2, 0, 0);
    rio_setLog(&vm, out, 0); rio_addFfi(&vm, "add", "ii>i", f_add); rio_addFfi(&vm, "half", "f>f", f_half);
    check(rio_loadImage(&vm, img, m) == 0 && rio_run(&vm) == 0 && rio_call(&vm, rio_findProc(&vm, "main")) == 0 &&
          !strcmp(logged, "hi 40 1.5|0|"), "load and run with the code in mem", &vm);
    logged[0] = 0;
    rio_init(&vm, mem2, sizeof mem2, code2, 4096);
    rio_setLog(&vm, out, 0); rio_addFfi(&vm, "add", "ii>i", f_add); rio_addFfi(&vm, "half", "f>f", f_half);
    check(rio_loadImage(&vm, img, m) == 0 && rio_run(&vm) == 0 && rio_call(&vm, rio_findProc(&vm, "main")) == 0 && !strcmp(logged, "hi 40 1.5|0|"),
          "a vm with a code buffer runs an image whose code is in mem", &vm);
    n = save(src, sizeof mem); /* code buffer */
    rio_init(&vm, mem2, sizeof mem2, 0, 0);
    rio_addFfi(&vm, "add", "ii>i", f_add); rio_addFfi(&vm, "half", "f>f", f_half);
    check(rio_loadImage(&vm, img, n) == -1 && strstr(rio_error(&vm), "separate buffer"), "an image with its code apart needs a code buffer", &vm);
    rio_init(&vm, mem2, sizeof mem2, code2, 4096);
    rio_addFfi(&vm, "add", "ii>i", f_add); rio_addFfi(&vm, "half", "f>f", f_half);
    check(rio_loadImage(&vm, img, n) == 0 && rio_run(&vm) == 0, "back to a code buffer", &vm);
  }
  { /* the source in mem itself, at its top (where a host with no other room reads a file): the same image
       as from a source elsewhere, and room for the program's memory too, down to the source's last byte */
    static unsigned char img2[1 << 16];
    static const char big[] = "a: [16000]Int\na[15999] = 7\nlog(a[15999])\n"; /* (64 KB of the 128: over the source too) */
    uint32_t len = (uint32_t)strlen(src), k, n2;
    char *at;
    rio_init(&vm, mem2, 1 << 17, 0, 0);
    rio_addFfi(&vm, "add", "ii>i", f_add); rio_addFfi(&vm, "half", "f>f", f_half);
    check(rio_compile(&vm, src, len) == 0, "compile from outside mem", &vm);
    n = rio_saveImage(&vm, img2, sizeof img2);
    memset(mem2, 0, sizeof mem2);
    rio_init(&vm, mem2, 1 << 17, 0, 0);
    rio_addFfi(&vm, "add", "ii>i", f_add); rio_addFfi(&vm, "half", "f>f", f_half);
    at = (char *)mem2 + (1 << 17) - len;
    memcpy(at, src, len);
    check(rio_compile(&vm, at, len) == 0, "compile from the top of mem", &vm);
    n2 = rio_saveImage(&vm, img, sizeof img);
    check(n2 == n && !memcmp(img, img2, n), "the same image", &vm);
    logged[0] = 0;
    rio_setLog(&vm, out, 0);
    check(rio_run(&vm) == 0 && rio_call(&vm, rio_findProc(&vm, "main")) == 0 && !strcmp(logged, "hi 40 1.5|0|"), "and it runs", &vm);
    k = (uint32_t)strlen(big);
    rio_init(&vm, mem2, 1 << 17, 0, 0);
    at = (char *)mem2 + (1 << 17) - k;
    memcpy(at, big, k);
    logged[0] = 0;
    rio_setLog(&vm, out, 0);
    check(rio_compile(&vm, at, k) == 0 && rio_run(&vm) == 0 && !strcmp(logged, "7|"), "a program's memory over where its source was", &vm);
    memcpy(mem2, big, k); /* (at the bottom: no room below it) */
    rio_init(&vm, mem2, 1 << 17, 0, 0);
    check(rio_compile(&vm, (char *)mem2 + 16, k) != 0 && strstr(rio_error(&vm), "not enough memory"), "a source at the bottom of mem", &vm);
  }
  /* saving */
  check(rio_saveImage(&vm, img, sizeof img) == 0, "no saving after running", &vm);
  n = save("x := 1\n", sizeof mem);
  check(n > 0, "a tiny program", 0);
  { /* an image a part at a time (as to and from a file), the code in mem (as a host short of memory has
       it): written in the parts it comes in, read back in odd sizes */
    uint32_t size = 1 << 17, k;
    rio_init(&vm, mem2, size, 0, 0);
    rio_addFfi(&vm, "add", "ii>i", f_add); rio_addFfi(&vm, "half", "f>f", f_half);
    check(rio_compile(&vm, src, (uint32_t)strlen(src)) == 0, "compile, the code in mem", &vm);
    stream.at = 0; stream.stopAt = 0;
    n = rio_writeImage(&vm, toStream, &stream);
    check(n > 0 && n == stream.at && n == rio_saveImage(&vm, 0, 0), "write an image in parts", &vm);
    memcpy(img2, img, n);
    check(rio_saveImage(&vm, img, sizeof img) == n && !memcmp(img, img2, n), "the same bytes as saving it whole", &vm);
    stream.at = 0; stream.stopAt = 40;
    check(rio_writeImage(&vm, toStream, &stream) == 0, "a write that fails stops it", &vm);
    for (k = 1; k <= 7; k += 6) {
      rio_init(&vm, mem2, size, 0, 0);
      rio_setLog(&vm, out, 0);
      rio_addFfi(&vm, "half", "f>f", f_half); rio_addFfi(&vm, "add", "ii>i", f_add);
      stream.at = 0; stream.len = n; stream.most = k == 1 ? n : 7;
      logged[0] = 0;
      check(rio_readImage(&vm, fromStream, &stream, n) == 0 && rio_run(&vm) == 0 && rio_call(&vm, rio_findProc(&vm, "main")) == 0 &&
            !strcmp(logged, "hi 40 1.5|0|"), "read an image in parts, and run it", &vm);
    }
    stream.at = 0; stream.len = n - 1; stream.most = n;
    check(rio_readImage(&vm, fromStream, &stream, n) == -1 && strstr(rio_error(&vm), "can't read"), "an image cut short", &vm);
    stream.at = 0; stream.len = n;
    check(rio_readImage(&vm, fromStream, &stream, n + 4) == -1 && strstr(rio_error(&vm), "wrong size"), "the wrong length", &vm);
    rio_init(&vm, mem2, size, 0, 0);
    rio_addFfi(&vm, "add", "ii>i", f_add);
    stream.at = 0;
    check(rio_readImage(&vm, fromStream, &stream, n) == -1 && strstr(rio_error(&vm), "didn't register"), "a called ffi function is missing, read in parts", &vm);
  }
  if (!fails) printf("all %d image tests passed\n", ntests);
  return fails != 0;
}
