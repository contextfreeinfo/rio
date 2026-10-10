/* module errors and edge cases through the API, with an in-memory loader */
#include <stdio.h>
#include <string.h>
#include "rio.h"

static unsigned char mem[1 << 20], scratch[256 << 10];
static RioIns code[8192];
static Rio vm;
static char out[1024];
static size_t outn;
static int fails, n;

/* "path" for local modules, "@path" for packages */
static const char *files[][2] = {
  {"a", "x* := 1\nhidden := 2\n"},
  {"b", "import .a\ny* := a.x + 1\n"},
  {"cyc1", "import .cyc2\n"},
  {"cyc2", "import .cyc1\n"},
  {"me", "x* := 1\nimport .me\n"},
  {"r1", "import .r2\n"},
  {"r2", "import .r3\n"},
  {"r3", "y := 1\nimport .r1\n"},
  {"err", "x* := 1\ny := nope\n"},
  {"rt", "xs: [2]Int\nboom* :: proc(i: Int) -> Int\n  return xs[i]\nend\n"},
  {"m", "V* :: struct\n  a: Int\nend\nV.pub* :: proc() -> Int\n  return 1\nend\nV.priv :: proc() -> Int\n  return 2\nend\n"},
  {"@pkg", "p* := 7\n"},
  {"@two", "t* := 2\n"},
};
static int loader(void *ud, const char *path, int pkg, RioSource *o) {
  size_t i; (void)ud;
  if (pkg && !strcmp(path, "two")) return 2; /* pretend it's in two library paths */
  for (i = 0; i < sizeof files / sizeof files[0]; i++) {
    const char *k = files[i][0];
    if ((k[0] == '@') == pkg && !strcmp(k + (k[0] == '@'), path)) {
      o->src = files[i][1]; o->len = (uint32_t)strlen(files[i][1]); o->isdir = 0; o->name = 0;
      return 0;
    }
  }
  return 1;
}
static void logfn(void *ud, const char *s, int len) { (void)ud; memcpy(out + outn, s, (size_t)len); outn += (size_t)len; out[outn++] = '\n'; out[outn] = 0; }

/* compile and run src; expect output (NULL: don't check), or an error: kind, file (NULL = main), line, message */
static void t(const char *src, const char *printed, int kind, const char *file, int line, const char *msg) {
  RioError e; int f;
  n++; outn = 0; out[0] = 0;
  rio_init(&vm, mem, sizeof mem, code, 8192);
  rio_set_log(&vm, logfn, 0);
  rio_set_loader(&vm, loader, 0);
  if (!rio_compile(&vm, src, (uint32_t)strlen(src)) && !rio_run(&vm)) {
    if ((f = rio_func(&vm, "main")) >= 0) rio_call(&vm, f);
  }
  e = rio_error_info(&vm);
  if (e.kind != kind || (printed && strcmp(out, printed)) || (kind && (e.line != line || !strstr(e.msg, msg) ||
      (file ? !e.file || strcmp(e.file, file) : e.file != 0)))) {
    printf("FAIL %d: \"%s\": got kind %d file %s line %d \"%s\" printed \"%s\"\n", n, src, e.kind, e.file ? e.file : "(main)", e.line, e.msg, out);
    fails++;
  }
}

int main(void) {
  t("import .a\nlog(a.x)\n", "1\n", RIO_ENONE, 0, 0, 0);
  t("import .b\nimport .a\nlog(b.y, a.x)\n", "2 1\n", RIO_ENONE, 0, 0, 0);           /* a is compiled once */
  t("import pkg.{p as q}\nlog(q)\n", "7\n", RIO_ENONE, 0, 0, 0);
  t("import .a\nlog(a.hidden)\n", 0, RIO_ECOMPILE, 0, 2, "not exported by that module");
  t("import .a.{hidden}\n", 0, RIO_ECOMPILE, 0, 1, "not exported by that module");
  t("import .nope\n", 0, RIO_ECOMPILE, 0, 1, "module not found: nope");
  t("import two\n", 0, RIO_ECOMPILE, 0, 1, "module found in more than one library path: two");
  t("import .cyc1\n", 0, RIO_ECOMPILE, "cyc2.rio", 1, "import cycle");
  t("import .me\n", 0, RIO_ECOMPILE, "me.rio", 2, "import cycle");                 /* importing yourself */
  t("import .r1\n", 0, RIO_ECOMPILE, "r3.rio", 2, "import cycle");                 /* a longer loop */
  t("import .err\n", 0, RIO_ECOMPILE, "err.rio", 2, "undefined name");
  t("x := 1\nimport .rt\nlog(rt.boom(5))\n", 0, RIO_ERUNTIME, "rt.rio", 3, "index out of bounds");
  t("import pkg*\n", 0, RIO_ECOMPILE, 0, 1, "only local modules");
  t("f :: proc()\n  x* := 1\nend\n", 0, RIO_ECOMPILE, 0, 2, "only top-level names can be exported");
  t("f :: proc()\n  import .a\nend\n", 0, RIO_ECOMPILE, 0, 2, "imports must be at the top level");
  t("import .m\nv: m.V\nlog(v.pub())\n", "1\n", RIO_ENONE, 0, 0, 0);
  t("import .m\nv: m.V\nlog(v.priv())\n", 0, RIO_ECOMPILE, 0, 3, "no such field or method");
  t("x :: 1\nlog(x)\n", "1\n", RIO_ENONE, 0, 0, 0);
  t("import .a\nlog(x)\n", 0, RIO_ECOMPILE, 0, 2, "undefined name");                  /* only through a. */

  { /* modules in a REPL session */
    const char *steps[] = {"import .a\n", "a.x + 1\n", "import .a.{x as ax}\n", "ax\n"};
    const char *want[] = {"", "2\n", "", "1\n"};
    int i;
    rio_init(&vm, mem, sizeof mem, code, 8192);
    rio_set_log(&vm, logfn, 0);
    rio_set_loader(&vm, loader, 0);
    rio_repl_begin(&vm, scratch, sizeof scratch, 0);
    for (i = 0; i < 4; i++) {
      n++; outn = 0; out[0] = 0;
      if (rio_repl_eval(&vm, steps[i], (uint32_t)strlen(steps[i])) || strcmp(out, want[i])) {
        printf("FAIL repl step %d: \"%s\" printed \"%s\" error \"%s\"\n", i, steps[i], out, rio_error(&vm)); fails++;
      }
    }
    n++;
    if (rio_repl_eval(&vm, "import .err\n", 12) != -1 || !rio_error_info(&vm).file) { printf("FAIL repl bad import\n"); fails++; }
    n++; outn = 0; out[0] = 0;
    if (rio_repl_eval(&vm, "a.x\n", 4) || strcmp(out, "1\n")) { printf("FAIL repl after bad import: %s\n", rio_error(&vm)); fails++; }
  }
  if (!fails) printf("all %d module tests passed\n", n);
  return fails != 0;
}
