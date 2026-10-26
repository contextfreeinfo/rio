/* host: host procs in modules, auto-imports, and declarations checked against the host and the
   builtins. argv[1]: core.rio, which must compile and declare every builtin */
#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "compiler.h" /* (rio_builtinNames) */

static unsigned char mem[1 << 20];
static rio_Instr code[8192];
static rio_Vm vm;
static char out[1024];
static size_t outn;
static int fails, n, cleared;

/* package modules the loader has: host modules written out, and ones that use them */
static const char *files[][2] = {
  {"hm", "add* :: host proc(a, b: Int) -> Int\ncls* :: host proc(color: Int)\n"},
  {"hbad", "add* :: host proc(a: Int, b: Float) -> Int\ncls* :: host proc(color: Int)\n"},
  {"hres", "add* :: host proc(a, b: Int)\ncls* :: host proc(color: Int)\n"},
  {"hmiss", "add* :: host proc(a, b: Int) -> Int\n"},
  {"hpriv", "add :: host proc(a, b: Int) -> Int\ncls* :: host proc(color: Int)\n"},
  {"hnest", "f :: proc()\n  add :: host proc(a, b: Int) -> Int\nend\n"},
  {"user", "import pc\ntwiceSum* :: proc(x: Int) -> Int\n  return pc.add(x, x)\nend\n"},
  {"hsrc", "Color* :: enum\n  black, red\nend\nTWO* :: 2\nadd* :: host proc(a, b: Int) -> Int\ncls* :: host proc(color: Color)\n"},
  {"hsbad", "add* :: host proc(a, b: Int) -> Int\ncls* :: host proc(color: Float)\n"},
  {"user2", "double* :: proc(x: Int) -> Int\n  return hsrc.add(x, hsrc.TWO) + x - hsrc.TWO\nend\n"},
  {"mathy","sqrt* :: host proc(x: Float) -> Float\nInt* :: host struct\nhalf* :: proc(x: Int) -> Int\n  return x / 2\nend\n"},
};
static int loader(void *ud, const char *path, int kind, rio_Source *o) {
  size_t i; (void)ud;
  if (!(kind & rio_LoadKind_package)) return 1;
  for (i = 0; i < sizeof files / sizeof files[0]; i++) {
    if (strcmp(files[i][0], path)) continue;
    o->src = files[i][1]; o->len = (uint32_t)strlen(files[i][1]); o->isDir = 0; o->name = 0;
    return 0;
  }
  return 1;
}
static void logfn(void *ud, const char *s, int len) { (void)ud; memcpy(out + outn, s, (size_t)len); outn += (size_t)len; out[outn++] = '\n'; out[outn] = 0; }
static void add(rio_Vm *v, rio_Val *a) { (void)v; a[0].i = a[0].i + a[1].i; }
static void cls(rio_Vm *v, rio_Val *a) { (void)v; cleared = a[0].i; }
static void scale(rio_Vm *v, rio_Val *a) { (void)v; a[0].f = a[0].f * (float)a[1].i; }

/* compile and run src (auto: a module to auto-import, or NULL); expect output (NULL: don't check), or
   an error: kind, file (NULL = main), line, message */
static void t(const char *auto_, const char *src, const char *printed, int kind, const char *file, int line, const char *msg) {
  static const char *const hostModules[] = {"pc", "hm", "hbad", "hres", "hmiss", "hpriv", "hnest", "hsrc", "hsbad"};
  rio_Error e;
  size_t i;
  n++; outn = 0; out[0] = 0; cleared = -1;
  rio_init(&vm, mem, sizeof mem, code, 8192);
  rio_setLog(&vm, logfn, 0);
  rio_setLoader(&vm, loader, 0);
  for (i = 0; i < sizeof hostModules / sizeof hostModules[0]; i++) { /* (names must outlive the compile) */
    static char names[16][2][32];
    snprintf(names[i][0], sizeof names[i][0], "%s.add", hostModules[i]);
    snprintf(names[i][1], sizeof names[i][1], "%s.cls", hostModules[i]);
    rio_addFfi(&vm, names[i][0], "ii>i", add);
    rio_addFfi(&vm, names[i][1], "i", cls);
  }
  rio_addFfi(&vm, "scale", "fi>f", scale); /* a global one */
  if (auto_) rio_autoImport(&vm, auto_);
  if (!rio_compile(&vm, src, (uint32_t)strlen(src))) rio_run(&vm);
  e = rio_errorInfo(&vm);
  if (e.kind != kind || (printed && strcmp(out, printed)) || (kind && (e.line != line || !strstr(e.msg, msg) ||
      (file ? !e.file || strcmp(e.file, file) : e.file != 0)))) {
    printf("FAIL %d: \"%s\": got kind %d file %s line %d \"%s\" printed \"%s\"\n", n, src, e.kind, e.file ? e.file : "(main)", e.line, e.msg, out);
    fails++;
  }
}
#define OK(src, printed) t(0, src, printed, rio_ErrorKind_none, 0, 0, 0)
#define BAD(src, file, line, msg) t(0, src, 0, rio_ErrorKind_compile, file, line, msg)

/* a REPL session with pc auto-imported: there in every input */
static void repl(void) {
  static unsigned char scratch[256 << 10];
  static const char *const inputs[][2] = {
    {"pc.cls(4)\n", ""}, {"pc.add(1, 2)\n", "3\n"}, {"f :: proc() -> Int\n  return pc.add(5, 5)\nend\n", ""}, {"f()\n", "10\n"},
    {"import pc.{add}\n", ""}, {"add(2, 2)\n", "4\n"},
  };
  size_t i;
  rio_init(&vm, mem, sizeof mem, code, 8192);
  rio_setLog(&vm, logfn, 0);
  rio_addFfi(&vm, "pc.add", "ii>i", add);
  rio_addFfi(&vm, "pc.cls", "i", cls);
  rio_autoImport(&vm, "pc");
  n++;
  if (rio_replBegin(&vm, scratch, sizeof scratch, 0)) { printf("FAIL: repl: %s\n", rio_error(&vm)); fails++; return; }
  for (i = 0; i < sizeof inputs / sizeof inputs[0]; i++) {
    outn = 0; out[0] = 0; cleared = -1;
    n++;
    if (rio_replEval(&vm, inputs[i][0], (uint32_t)strlen(inputs[i][0])) || strcmp(out, inputs[i][1])) {
      printf("FAIL: repl \"%s\": printed \"%s\" (%s)\n", inputs[i][0], out, rio_error(&vm));
      fails++;
    }
    if (i == 0 && cleared != 4) { printf("FAIL: repl: pc.cls wasn't called\n"); fails++; }
  }
}

/* a REPL session with hsrc auto-imported from its source: its enum and constant there in every input */
static void replSource(void) {
  static unsigned char scratch[256 << 10];
  static const char *const inputs[][2] = {
    {"c := hsrc.Color.red\n", ""}, {"hsrc.cls(c)\n", ""}, {"log(c, hsrc.add(hsrc.TWO, 2))\n", "red 4\n"},
    {"f :: proc() -> hsrc.Color\n  return hsrc.Color.black\nend\n", ""}, {"log(f())\n", "black\n"},
  };
  size_t i;
  rio_init(&vm, mem, sizeof mem, code, 8192);
  rio_setLog(&vm, logfn, 0);
  rio_setLoader(&vm, loader, 0);
  rio_addFfi(&vm, "hsrc.add", "ii>i", add);
  rio_addFfi(&vm, "hsrc.cls", "i", cls);
  rio_autoImport(&vm, "hsrc");
  n++;
  if (rio_replBegin(&vm, scratch, sizeof scratch, 0)) { printf("FAIL: repl from source: %s\n", rio_error(&vm)); fails++; return; }
  for (i = 0; i < sizeof inputs / sizeof inputs[0]; i++) {
    outn = 0; out[0] = 0; cleared = -1;
    n++;
    if (rio_replEval(&vm, inputs[i][0], (uint32_t)strlen(inputs[i][0])) || strcmp(out, inputs[i][1])) {
      printf("FAIL: repl \"%s\": printed \"%s\" (%s)\n", inputs[i][0], out, rio_error(&vm));
      fails++;
    }
    if (i == 1 && cleared != 1) { printf("FAIL: repl from source: hsrc.cls wasn't called with red\n"); fails++; }
  }
}

/* core.rio compiles, and names every builtin: as `name*` or `.name*` (some only in its comments) */
static void core(const char *path) {
  FILE *f = fopen(path, "rb");
  static char src[1 << 16];
  size_t len;
  int i;
  n++;
  if (!f) { printf("FAIL: can't read %s\n", path); fails++; return; }
  len = fread(src, 1, sizeof src - 1, f);
  fclose(f);
  src[len] = 0;
  rio_init(&vm, mem, sizeof mem, code, 8192);
  if (rio_compile(&vm, src, (uint32_t)len)) { printf("FAIL: core.rio: %s\n", rio_error(&vm)); fails++; return; }
  for (i = 0; i < rio_Builtin_count; i++) {
    const char *name = rio_builtinNames[i], *p = src;
    size_t nl = strlen(name);
    int found = 0;
    n++;
    while (!found && (p = strstr(p, name))) {
      found = (p == src || !(rio_isAlpha(p[-1]) || rio_isDigit(p[-1]))) && p[nl] == '*';
      p += nl;
    }
    if (!found) { printf("FAIL: core.rio doesn't declare the builtin %s\n", name); fails++; }
  }
  /* its built-in structs, declared as plain ones: the same fields, in order, of the same types, as a
     zeroed Format prints them (0, 0.0, false, ...), and a Range packs (it prints as first..=last) */
  {
    static const char use[] = "\nlog(Format{})\nrangeBytes: Blob[8]\nrangeBytes.pack(0, Range{first = 3, last = 9})\n"
                              "log(rangeBytes.getInt(0, 32), rangeBytes.getInt(4, 32))\n";
    char builtin[256];
    n++;
    outn = 0; out[0] = 0;
    rio_init(&vm, mem, sizeof mem, code, 8192);
    rio_setLog(&vm, logfn, 0);
    if (rio_compile(&vm, use, (uint32_t)strlen(use)) || rio_run(&vm)) { printf("FAIL: the builtin Format: %s\n", rio_error(&vm)); fails++; return; }
    snprintf(builtin, sizeof builtin, "%s", out);
    if (len + sizeof use > sizeof src) { printf("FAIL: core.rio is too long to check\n"); fails++; return; }
    memcpy(src + len, use, sizeof use);
    outn = 0; out[0] = 0;
    rio_init(&vm, mem, sizeof mem, code, 8192);
    rio_setLog(&vm, logfn, 0);
    if (rio_compile(&vm, src, (uint32_t)strlen(src)) || rio_run(&vm)) { printf("FAIL: core.rio's Format: %s\n", rio_error(&vm)); fails++; return; }
    if (strcmp(out, builtin)) { printf("FAIL: core.rio's Format isn't the builtin's: %s vs %s", out, builtin); fails++; }
  }
}

int main(int argc, char **argv) {
  /* host procs registered under a module name: a module made from them, with nothing to parse */
  OK("import pc\nlog(pc.add(2, 3))\npc.cls(7)\nlog(pc.add(pc.add(1, 1), 0))\n", "5\n2\n");
  OK("import pc.{add, cls as clear}\nclear(add(4, 5))\n", "");
  OK("import user\nlog(user.twiceSum(21))\n", "42\n");                                  /* used by another module */
  BAD("log(add(1, 2))\n", 0, 1, "undefined name");                                       /* not global */
  BAD("log(pc.add(1, 2))\n", 0, 1, "undefined name");                                    /* not without an import */
  BAD("import pc.{nope}\n", 0, 1, "not exported by that module");
  BAD("import nohost\n", 0, 1, "module not found: nohost");
  /* auto-imports: everywhere, as if imported */
  t("pc", "log(pc.add(1, 2))\n", "3\n", rio_ErrorKind_none, 0, 0, 0);
  t("pc", "import user\nlog(user.twiceSum(2))\n", "4\n", rio_ErrorKind_none, 0, 0, 0);
  t("nope", "log(1)\n", 0, rio_ErrorKind_compile, 0, 0, "auto-import nope: the host registered no procs under that name");
  /* an auto-import the loader has a source for: compiled from it first (its enums, constants, and its host
     procs as declared, an enum for an Int), then everywhere */
  t("hsrc", "c := hsrc.Color.red\nhsrc.cls(c)\nlog(c, hsrc.add(hsrc.TWO, 1))\n", "red 3\n", rio_ErrorKind_none, 0, 0, 0);
  if (cleared != 1) { printf("FAIL: hsrc.cls got %d\n", cleared); fails++; }
  t("hsrc", "import user2\nlog(user2.double(4))\n", "8\n", rio_ErrorKind_none, 0, 0, 0);
  t("hsrc", "log(1)\nnope()\n", 0, rio_ErrorKind_compile, 0, 2, "undefined name");
  t("hsrc", "hsrc.cls(1)\n", 0, rio_ErrorKind_compile, 0, 1, "");
  t("hsbad", "log(1)\n", 0, rio_ErrorKind_compile, "hsbad.rio", 2, "doesn't match the host's hsbad.cls: proc(Int)");
  /* a module source for host procs: its declarations are checked against what's registered */
  OK("import hm\nlog(hm.add(20, 22))\nhm.cls(3)\n", "42\n");
  BAD("import hbad\n", "hbad.rio", 1, "doesn't match the host's hbad.add: proc(Int, Int) -> Int");
  BAD("import hres\n", "hres.rio", 1, "doesn't match the host's hres.add: proc(Int, Int) -> Int");
  BAD("import hmiss\n", "hmiss.rio", 1, "this module doesn't declare the host proc hmiss.cls");
  BAD("import hpriv\nlog(hpriv.add(1, 2))\n", 0, 2, "not exported by that module");
  BAD("import hnest\n", "hnest.rio", 2, "host declarations go at the top level");
  /* a global host proc, declared in the main module */
  OK("scale* :: host proc(x: Float, times: Int) -> Float\nlog(scale(1.5, 4))\n", "6.0\n");
  BAD("scale :: host proc(x: Float) -> Float\n", 0, 1, "doesn't match the host's scale: proc(Float, Int) -> Float");
  /* builtins: declared with their parameters and results */
  OK("abs* :: host proc(x: Int) -> Int\nmin :: host proc(a, b: Float) -> Float\nlog(abs(-3), min(1.5, 2.5))\n", "3 1.5\n");
  OK("assert* :: host proc(condition: Bool, why: String)\nhexBlob :: host proc(d: String) -> Blob\n", "");
  OK("Int.toFloat* :: host proc() -> Float\n(Blob[..]).push :: host proc(byte: Int) -> Bool\n(Blob[..]).clear :: host proc()\nlog(3.toFloat())\n", "3.0\n");
  OK("import mathy\nlog(mathy.sqrt(16.0), mathy.half(9))\nx: mathy.Int = 4\nlog(x)\n", "4.0 4\n4\n"); /* re-exported */
  BAD("abs :: host proc(x: String) -> Int\n", 0, 1, "doesn't match the builtin: type mismatch");
  BAD("abs :: host proc(x: Int) -> Float\n", 0, 1, "doesn't match the builtin: it gives Int");
  BAD("sqrt :: host proc(x: Float)\n", 0, 1, "doesn't match the builtin: it gives Float");
  BAD("min :: host proc(a, b, c: Int) -> Int\n", 0, 1, "doesn't match the builtin: wrong number of arguments");
  BAD("nope :: host proc(x: Int)\n", 0, 1, "the host has no proc by this name, and no builtin has it");
  BAD("Int.toFloat :: host proc() -> Int\n", 0, 1, "doesn't match the builtin: it gives Float");
  BAD("Int.nope :: host proc() -> Int\n", 0, 1, "doesn't match the builtin: no such method");
  BAD("(Blob[..]).push :: host proc(s: String) -> Bool\n", 0, 1, "doesn't match the builtin: type mismatch");
  BAD("x := 1\nf :: proc()\n  abs :: host proc(x: Int) -> Int\nend\n", 0, 3, "host declarations go at the top level");
  /* builtin types */
  OK("Int* :: host struct\nFloat* :: host struct\nx: Int = 2\nlog(x)\n", "2\n");
  BAD("Nope :: host struct\n", 0, 1, "the host has no type by this name");
  BAD("P :: struct\n  x: Int\nend\nQ :: host struct\n", 0, 4, "the host has no type by this name");
  BAD("Int.x :: host struct\n", 0, 1, "'proc' expected");
  /* host and diverse mean something only before proc or struct, and union: elsewhere they're names */
  OK("host := 1\ndiverse := 2\nlog(host + diverse)\n", "3\n");
  BAD("f :: @host proc()\n", 0, 1, "expected expression");
  BAD("x := 1 + host proc()\n", 0, 1, "undefined name");
  BAD("(Int).f :: proc()\nend\n", 0, 1, "a method on a type without a name is a builtin");
  /* proc types name their parameters, grouped as in proc definitions (names are for reading) */
  OK("sum :: proc(a, b: Int, c: Float) -> Int\n  return a + b + c.toInt()\nend\nf: proc(x, y: Int, z: Float) -> Int = sum\nlog(f(1, 2, 3.5))\n", "6\n");
  repl();
  replSource();
  if (argc > 1) core(argv[1]);
  else { printf("FAIL: no core.rio given\n"); fails++; }
  if (fails) printf("%d of %d host tests failed\n", fails, n);
  else printf("all %d host tests passed\n", n);
  return fails ? 1 : 0;
}
