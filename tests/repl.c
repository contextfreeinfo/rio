/* REPL and break tests: a scripted session through the API */
#include <stdio.h>
#include <string.h>
#include "rio.h"

static unsigned char mem[1 << 20], scratch[256 << 10];
static RioIns code[8192];
static Rio vm;
static char out[4096];
static size_t outn;
static int fails, n;

static void logfn(void *ud, const char *s, int len) { (void)ud; memcpy(out + outn, s, (size_t)len); outn += (size_t)len; out[outn++] = '\n'; out[outn] = 0; }
static void stop(Rio *v, RioVal *a) { (void)a; rio_interrupt(v); }

/* eval src; expect result r, printed output (NULL = don't check) and error substring (NULL = none) */
static void eval(const char *src, int r, const char *printed, const char *err) {
  int got;
  outn = 0; out[0] = 0;
  got = rio_repl_eval(&vm, src, (uint32_t)strlen(src));
  n++;
  if (got != r || (printed && strcmp(out, printed)) || (err ? !strstr(rio_error(&vm), err) : got < 0)) {
    printf("FAIL %d: eval \"%s\" -> %d, printed \"%s\", error \"%s\"\n", n, src, got, out, rio_error(&vm));
    fails++;
  }
}

int main(void) {
  RioVal *a; int f;
  rio_init(&vm, mem, sizeof mem, code, 8192);
  rio_set_log(&vm, logfn, 0);
  rio_ffi(&vm, "stop", "", stop);
  if (rio_repl_begin(&vm, scratch, sizeof scratch, 0)) { printf("FAIL begin: %s\n", rio_error(&vm)); return 1; }

  eval("x := 40\n", 0, "", 0);
  eval("x + 2\n", 0, "42\n", 0);                                   /* bare expressions print */
  eval("Float(x) / 8.0 == 5\n", 0, "true\n", 0);
  eval("f :: proc(n: Int) -> Int\n", RIO_MORE, "", 0);             /* unfinished: ask for more */
  eval("f :: proc(n: Int) -> Int\n  return n * x\n", RIO_MORE, "", 0);
  eval("f :: proc(n: Int) -> Int\n  return n * x\nend\n", 0, "", 0);
  eval("f(2)\n", 0, "80\n", 0);
  eval("x := 1 +\n", RIO_MORE, "", 0);
  eval("v := (1 + 2\n", RIO_MORE, "", 0);
  eval("log(\"open\n", RIO_MORE, "", 0);
  eval("if x > 0\n", RIO_MORE, "", 0);
  eval("z := nope\n", -1, "", "undefined name");                  /* a compile error... */
  eval("z\n", -1, "", "undefined name");                           /* ...declares nothing */
  eval("if x\nend\n", -1, "", "condition must be Bool");           /* wrong, not unfinished */
  eval("V :: struct\n  a, b: Int\nend\n", 0, "", 0);
  eval("V.sum :: proc() -> Int\n  return self.a + self.b\nend\n", 0, "", 0);
  eval("v := V{a = 1, b = 2}\nv.sum()\n", 0, "3\n", 0);
  eval("s := \"hi\"\n", 0, "", 0);
  eval("s\n", 0, "hi\n", 0);                                       /* strings outlive the piece that made them */
  eval("names: [..4]String\nnames.push(s)\nnames.push(\"there\")\n", 0, "true\ntrue\n", 0); /* push returns a Bool, so it prints */
  eval("log(names[0], names[1])\n", 0, "hi there\n", 0);
  eval("xs: [3]Int\nk := 5\nxs[k] = 1\n", -1, "", "3: runtime error: index out of bounds");
  eval("k\n", 0, "5\n", 0);                                        /* runtime errors keep what already ran */
  eval("for\n  stop()\nend\n", -1, "", "runtime error: interrupted");
  eval("x\n", 0, "40\n", 0);                                       /* and the session carries on */

  /* the host can see and call what the session defined */
  n++;
  if ((f = rio_func(&vm, "f")) < 0) { printf("FAIL: rio_func\n"); fails++; }
  else {
    a = rio_args(&vm, f); a[0].i = 5;
    if (rio_call(&vm, f) || rio_ret(&vm, f)->i != 200) { printf("FAIL: rio_call -> %d\n", rio_ret(&vm, f)->i); fails++; }
  }
  n++;
  if (!rio_global(&vm, "x") || *(int *)rio_global(&vm, "x") != 40) { printf("FAIL: rio_global\n"); fails++; }

  /* break outside a REPL too */
  {
    const char *src = "i := 0\nfor\n  i += 1\n  if i == 1000\n    stop()\n  end\nend\n";
    n++;
    rio_init(&vm, mem, sizeof mem, code, 8192);
    rio_ffi(&vm, "stop", "", stop);
    if (rio_compile(&vm, src, (uint32_t)strlen(src)) || !rio_run(&vm) || !strstr(rio_error(&vm), "interrupted")) {
      printf("FAIL: interrupt a compiled program: \"%s\"\n", rio_error(&vm)); fails++;
    }
  }
  if (!fails) printf("all %d repl tests passed\n", n);
  return fails != 0;
}
