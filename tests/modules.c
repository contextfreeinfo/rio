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
  {"@lib", "import .pub*\nimport .priv\ninclude \"part.rio\"\nv* := pub.x + priv.y + w\n"},
  {"@lib/pub", "x* := 1\n"},
  {"@lib/priv", "y* := 2\n"},
  {"@lib/part.rio", "w := 10\n"},
  {"t", "T* :: struct\n  q: Int\nend\n"},
  {"al", "import .t\nTT* :: t.T\nf* :: proc() -> Int\n  return 4\nend\ng* :: f\n"},
  {"part.rio", "helper :: proc(x: Int) -> Int\n  return x * secret\nend\n"},
  {"dir/inner.rio", "y := 3\n"},
  {"dir/m", "include \"inner.rio\"\nz* := y + 1\n"},
  {"bad.rio", "q := 1\nr := nope\n"},
  {"rtpart.rio", "xs: [2]Int\nget :: proc(i: Int) -> Int\n  return xs[i]\nend\n"},
  {"cyca.rio", "a := 1\ninclude \"cycb.rio\"\n"},
  {"cycb.rio", "include \"cyca.rio\"\n"},
  {"sub/.hidden.rio", "h := 1\n"},
  {"k", "N* :: 4\nHALF* :: 0.5\nNAME* :: \"rio\"\nON* :: true\nSECRET :: 9\n"
         "P* :: struct\n  x, y: Int\nend\nHid :: struct\n  a: Int\nend\n"
         "P.sum* :: proc() -> Int\n  return self.x + self.y\nend\n"
         "mk* :: proc(x: Int) -> P\n  return {x, y = N}\nend\n"
         "total* :: proc(ps: []P) -> Int\n  t := 0\n  for i in 0..<len(ps)\n    t += ps[i].sum()\n  end\n  return t\nend\n"},
  {"rf", "R* :: struct\n  n: Int\nend\ninc* :: proc(r: &R, by: Int)\n  r.n += by\nend\n"},
  {"@two", "t* := 2\n"},
};
static int loader(void *ud, const char *path, int kind, RioSource *o) {
  size_t i; char want[128]; (void)ud;
  if ((kind & RIO_LOAD_PKG) && !strcmp(path, "two")) return 2; /* pretend it's in two library paths */
  snprintf(want, sizeof want, "%s%s", kind & RIO_LOAD_PKG ? "@" : "", path); /* includes ask for "x.rio" itself */
  for (i = 0; i < sizeof files / sizeof files[0]; i++) {
    const char *k = files[i][0];
    if (!strcmp(k, want)) {
      size_t j, wl = strlen(want);
      o->src = files[i][1]; o->len = (uint32_t)strlen(files[i][1]); o->isdir = 0; o->name = 0;
      for (j = 0; j < sizeof files / sizeof files[0]; j++) /* a module with files under it is a directory */
        if (!strncmp(files[j][0], want, wl) && files[j][0][wl] == '/') o->isdir = 1;
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
  t("import two\n", 0, RIO_ECOMPILE, 0, 1, "found in more than one library path: two");
  t("import .cyc1\n", 0, RIO_ECOMPILE, "cyc2.rio", 1, "import cycle");
  t("import .me\n", 0, RIO_ECOMPILE, "me.rio", 2, "import cycle");                 /* importing yourself */
  t("import .r1\n", 0, RIO_ECOMPILE, "r3.rio", 2, "import cycle");                 /* a longer loop */
  t("import .err\n", 0, RIO_ECOMPILE, "err.rio", 2, "undefined name");
  t("x := 1\nimport .rt\nlog(rt.boom(5))\n", 0, RIO_ERUNTIME, "rt.rio", 3, "index out of bounds");
  t("import pkg*\n", 0, RIO_ECOMPILE, 0, 1, "only your own submodules can be published");
  /* packages: only what the root publishes is reachable from outside */
  t("import lib.pub\nlog(pub.x)\n", "1\n", RIO_ENONE, 0, 0, 0);
  t("import lib\nlog(lib.v, lib.pub.x)\n", "13 1\n", RIO_ENONE, 0, 0, 0);
  t("import lib.priv\n", 0, RIO_ECOMPILE, 0, 1, "that package doesn't publish priv");
  t("import lib\nlog(lib.priv)\n", 0, RIO_ECOMPILE, 0, 2, "not exported by that module");
  /* exporting an imported name is an explicit alias, not part of import */
  t("import .a.{x*}\n", 0, RIO_ECOMPILE, 0, 1, "to export an imported name");
  t("import .al\nv: al.TT\nv.q = 5\nlog(al.g(), v.q)\n", "4 5\n", RIO_ENONE, 0, 0, 0);
  /* include: a file becomes part of this module */
  t("secret := 5\ninclude \"part.rio\"\nlog(helper(2))\n", "10\n", RIO_ENONE, 0, 0, 0);
  t("import .dir.m\nlog(m.z)\n", "4\n", RIO_ENONE, 0, 0, 0);                         /* relative to the including file */
  t("secret := 1\ninclude \"part.rio\"\ninclude \"part.rio\"\n", 0, RIO_ECOMPILE, 0, 3, "file already included");
  t("include \"../x.rio\"\n", 0, RIO_ECOMPILE, 0, 1, "stay inside this file's directory");
  t("include \"part\"\n", 0, RIO_ECOMPILE, 0, 1, "include paths name a .rio file");
  t("include \"nope.rio\"\n", 0, RIO_ECOMPILE, 0, 1, "file not found: nope.rio");
  t("include \"bad.rio\"\n", 0, RIO_ECOMPILE, "bad.rio", 2, "undefined name");
  t("include \"rtpart.rio\"\nlog(get(9))\n", 0, RIO_ERUNTIME, "rtpart.rio", 3, "index out of bounds");
  t("f :: proc()\n  include \"part.rio\"\nend\n", 0, RIO_ECOMPILE, 0, 2, "includes must be at the top level");
  /* bad include paths: one spelling per file, always inside this file's directory */
  t("include \"/part.rio\"\n", 0, RIO_ECOMPILE, 0, 1, "stay inside this file's directory");
  t("include \"a\\\\part.rio\"\n", 0, RIO_ECOMPILE, 0, 1, "stay inside this file's directory");
  t("include \"c:part.rio\"\n", 0, RIO_ECOMPILE, 0, 1, "stay inside this file's directory");
  t("include \"./part.rio\"\n", 0, RIO_ECOMPILE, 0, 1, "stay inside this file's directory");
  t("include \"sub//part.rio\"\n", 0, RIO_ECOMPILE, 0, 1, "stay inside this file's directory");
  t("include \"sub/../part.rio\"\n", 0, RIO_ECOMPILE, 0, 1, "stay inside this file's directory");
  t("include \"sub/./part.rio\"\n", 0, RIO_ECOMPILE, 0, 1, "stay inside this file's directory");
  t("include \"sub/.hidden.rio\"\n", "", RIO_ENONE, 0, 0, 0);                      /* dot files are just names */
  t("include part.rio\n", 0, RIO_ECOMPILE, 0, 1, "include needs a file path in quotes");
  t("secret := 1\ninclude \"part.rio\" log(1)\n", 0, RIO_ECOMPILE, 0, 2, "expected end of statement");
  t("include \"cyca.rio\"\n", 0, RIO_ECOMPILE, "cycb.rio", 1, "file already included"); /* a cycle of includes */
  /* imported constants: usable wherever a constant is needed */
  t("import .k\nxs: [k.N]Int\nlog(len(xs))\n", "4\n", RIO_ENONE, 0, 0, 0);
  t("import .k.{N}\nM :: N * 2\nys: [M]Int\nlog(M, len(ys))\n", "8 8\n", RIO_ENONE, 0, 0, 0);
  t("import .k\nlog(k.HALF * 2.0, k.NAME, k.ON)\n", "1.0 rio true\n", RIO_ENONE, 0, 0, 0);
  t("import .k.{NAME as nm}\nlog(nm)\n", "rio\n", RIO_ENONE, 0, 0, 0);
  t("N :: 4\nN = 5\n", 0, RIO_ECOMPILE, 0, 2, "cannot assign to this");          /* same as a local constant */
  t("import .k\nk.N = 5\n", 0, RIO_ECOMPILE, 0, 2, "cannot assign to this");
  t("import .k.{N}\nN = 5\n", 0, RIO_ECOMPILE, 0, 2, "cannot assign to this");
  t("import .k\nlog(k.SECRET)\n", 0, RIO_ECOMPILE, 0, 2, "not exported by that module");
  /* imported types: in fields, arrays, lists, slices, literals, params and returns */
  t("import .k\nQ :: struct\n  p: k.P\n  ps: [2]k.P\nend\nq: Q\nq.p = {x = 1, y = 2}\nq.ps[1].x = 7\nlog(q.p.sum(), q.ps[1].sum())\n", "3 7\n", RIO_ENONE, 0, 0, 0);
  t("import .k\nps: [..4]k.P\nps.push({x = 1, y = 1})\nps.push(k.mk(2))\nlog(k.total(ps[:]), len(ps))\n", "8 2\n", RIO_ENONE, 0, 0, 0);
  t("import .k.{P, mk}\n{x, y as b} := mk(3)\nlog(x, b)\n", "3 4\n", RIO_ENONE, 0, 0, 0);
  t("import .k.{P}\nf :: proc(p: P) -> P\n  return {x = p.y, y = p.x}\nend\nlog(f({x = 1, y = 2}).x)\n", "2\n", RIO_ENONE, 0, 0, 0);
  t("import .k\nv := k.P{x = 1, y = 2}\nlog(v.sum())\n", "3\n", RIO_ENONE, 0, 0, 0);
  t("import .k\nv: k.P\nv.z = 1\n", 0, RIO_ECOMPILE, 0, 3, "no such field");
  t("import .k\nv: k.Hid\n", 0, RIO_ECOMPILE, 0, 2, "not exported by that module");
  t("import .k\nv: k.mk\n", 0, RIO_ECOMPILE, 0, 2, "type expected");             /* exported, but not a type */
  t("import .k.{Hid}\n", 0, RIO_ECOMPILE, 0, 1, "not exported by that module");
  t("import .rf\nv: rf.R\nrf.inc(&v, 2)\nw := &v\nrf.inc(&w, 3)\nlog(v.n)\n", "5\n", RIO_ENONE, 0, 0, 0); /* & across modules */
  t("import .k\nv: k.P = 1\n", 0, RIO_ECOMPILE, 0, 2, "type");
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
