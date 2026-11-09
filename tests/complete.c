/* rio_complete's tests: a REPL session with some names in it, then text with the cursor at '|', and the
   names offered there. want: each name offered, in order, with its kind's letter (k keyword, v var, c
   const, t type, p proc, m method, f field, M module, e enum value) and detail when it has one
   (name:k or name:k:detail), space-separated; or with has, just some that must be among them (and
   !name: must not be) */
#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <string.h>
#include "rio.h"

static unsigned char mem[1 << 20], scratch[256 << 10];
static rio_Instr code[8192];
static rio_Vm vm;
static char got[8192];
static size_t gotLen;
static int fails, n;

static void twice(rio_Vm *v, rio_Val *a) {
  (void)v;
  a[0].f = a[0].f * 2;
}

static const char *const files[][2] = {
  {"shapes", "Box* :: struct\n  w, h: Int\nend\narea* :: proc(b: Box) -> Int\n  return b.w * b.h\nend\nSIDES* :: 4\nhidden :: 1\n"},
};
static int loader(void *ud, const char *path, int kind, rio_Source *o) {
  size_t i;
  (void)ud;
  (void)kind;
  for (i = 0; i < sizeof files / sizeof files[0]; i++)
    if (!strcmp(files[i][0], path)) {
      o->src = files[i][1];
      o->len = (uint32_t)strlen(files[i][1]);
      o->isDir = 0;
      o->name = 0;
      return 0;
    }
  return 1;
}

static void item(void *ud, const char *name, int nameLen, int kind, const char *detail, int detailLen) {
  (void)ud;
  if (gotLen > sizeof got - 256) return; /* (full: the rest left out) */
  gotLen +=(size_t)snprintf(got + gotLen, sizeof got - gotLen, "%s%.*s:%c%s%.*s", gotLen ? " " : "", nameLen, name, "kvctpmfMe"[kind],
                             detailLen ? ":" : "", detailLen, detail);
}

/* rio_lookupAt's first: "name detail line textAt" */
static char found[256];
static void foundAt(void *ud, const char *name, int nameLen, int kind, const char *detail, int detailLen, const char *file, int line, int textAt) {
  (void)ud;
  (void)kind;
  (void)file;
  if (!found[0]) snprintf(found, sizeof found, "%.*s %.*s %d %d", nameLen, name, detailLen, detail, line, textAt);
}

/* the names offered at text's '|' (where the word starts: at the word's start, with its length) */
static int complete(const char *text) {
  static char t[1024];
  const char *bar = strchr(text, '|');
  size_t at = (size_t)(bar - text);
  memcpy(t, text, at);
  strcpy(t + at, bar + 1);
  gotLen = 0;
  got[0] = 0;
  return rio_complete(&vm, t, (uint32_t)at, item, 0);
}

static void exact(const char *text, const char *want) {
  n++;
  complete(text);
  if (strcmp(got, want)) {
    printf("FAIL %d: \"%s\"\n  want %s\n  got  %s\n", n, text, want, got);
    fails++;
  }
}

/* has a name (name:k or name:k:detail, the whole item, or just name: as a name's start) */
static int offered(const char *item) {
  const char *p = got;
  size_t len = strlen(item);
  while ((p = strstr(p, item)) != 0) {
    if ((p == got || p[-1] == ' ') && (p[len] == 0 || p[len] == ' ' || p[len] == ':' || item[len - 1] == ':')) return 1;
    p += len;
  }
  return 0;
}

static void has(const char *text, const char *want) {
  char w[512], *s;
  n++;
  complete(text);
  strcpy(w, want);
  for (s = strtok(w, " "); s; s = strtok(0, " ")) {
    int no = *s == '!';
    if (offered(s + no) == !no) continue;
    printf("FAIL %d: \"%s\": %s%s\n  got %s\n", n, text, s + no, no ? " offered" : " not offered", got);
    fails++;
    return;
  }
}

static void eval(const char *src) {
  if (rio_replEval(&vm, src, (uint32_t)strlen(src))) {
    printf("FAIL setup: %s: %s\n", src, rio_error(&vm));
    fails++;
  }
}

int main(void) {
  int at;
  rio_init(&vm, mem, sizeof mem, code, 8192);
  rio_setLoader(&vm, loader, 0);
  rio_addFfi(&vm, "twice", "f>f", twice);
  if (rio_replBegin(&vm, scratch, sizeof scratch, 0)) {
    printf("FAIL begin: %s\n", rio_error(&vm));
    return 1;
  }
  eval("Point :: struct\n  x: Float\n  y: Float\nend\n");
  eval("Point.len2 :: proc() -> Float\n  return self.x * self.x + self.y * self.y\nend\n");
  eval("Point.at :: static proc(x: Float, y?: Float) -> Point\n  return Point{x = x, y = y}\nend\n");
  eval("Dir :: enum\n  north, east, south, west\nend\n");
  eval("Line :: struct\n  from, to: Point\n  dir: Dir\nend\n");
  eval("pts: [..8]Point\nline: Line\ncounts: [Dir]Int\nbytes: Blob[..64]\nfixed: Blob[4]\nname :=\"rio\"\nr := 0..<10\nflag := true\n");
  eval("mid :: callback proc(a: Point, b: &Point) -> Point\n  return Point{x = (a.x + b.x) / 2, y = (a.y + b.y) / 2}\nend\n");
  eval("pointer := mid\nposition := Point{x = 1, y = 2}\n");
  eval("import shapes\nN :: 16\n");

  /* x.: fields, then methods, then the built-in ones */
  exact("position.|", "x:f:Float y:f:Float len2:m:proc() -> Float");
  exact("position.l|", "len2:m:proc() -> Float");
  exact("line.|", "from:f:Point to:f:Point dir:f:Dir");
  exact("line.from.|", "x:f:Float y:f:Float len2:m:proc() -> Float");
  exact("line.dir.|", "toInt:m:built in");
  exact("pts[0].|", "x:f:Float y:f:Float len2:m:proc() -> Float");
  exact("mid(position, position).|", "x:f:Float y:f:Float len2:m:proc() -> Float");
  exact("pointer(position, position).y|", "y:f:Float");
  exact("Point.at(1).|", "x:f:Float y:f:Float len2:m:proc() -> Float");
  exact("pts.|", "len:m:built in cap:m:built in swap:m:built in push:m:built in pop:m:built in clear:m:built in remove:m:built in "
                 "swapRemove:m:built in pushAll:m:built in");
  exact("pts.p|", "push:m:built in pop:m:built in pushAll:m:built in");
  exact("pts[1..<3].|", "len:m:built in cap:m:built in swap:m:built in");
  exact("pts.pop().|", "x:f:Float y:f:Float len2:m:proc() -> Float");
  has("bytes.|", "len:m push:m format:m print:m pack:m getInt:m toInt:m toFloat:m asString:m !toBool:");
  exact("fixed.as|", "asString:m:built in"); /* (a Blob[N]) */
  has("fixed.|", "len:m swap:m pack:m asString:m !toInt: !push:");
  has("name.|","len:m swap:m pack:m toInt:m asString:m !push:");
  has("name[0].|", "toInt:m toFloat:m toBool:m");
  has("counts.|", "len:m toInt:m toFloat:m !push:");
  exact("r.|", "first:f:Int last:f:Int len:m:built in has:m:built in");
  exact("flag.|", "toInt:m:built in toBool:m:built in");
  exact("position.x.to|", "toInt:m:built in toFloat:m:built in");
  exact("(position).|", "");
  exact("nothing.|", "");
  exact("3.|", "");
  has("x := 1..|", "nil:k position:v !len:m !x:"); /* (and not x, being declared) */ /* (a range: not a dot) */

  /* Type.: an enum's values, a type's static procs; module.: its exported names */
  exact("Dir.|", "north:e:Dir east:e:Dir south:e:Dir west:e:Dir fromInt:m:built in has:m:built in");
  exact("Dir.s|", "south:e:Dir");
  exact("Point.|", "at:m:proc(Float, ?Float) -> Point");
  exact("shapes.|", "SIDES:c:Int area:p:proc(Box) -> Int Box:t:struct");
  exact("shapes.Box.|", "");
  has("core.|", "log:p sqrt:p true:c !Point: !shapes:");

  /* names in scope, the newest of each, and the words that fit */
  has("po|", "position:v:Point pointer:v:proc(Point, &Point) -> Point !pts:");
  has("|", "if:k for:k switch:k do:k return:k import:k include:k Point:t:struct mid:p:proc(Point, &Point) -> Point twice:p:proc(Float) -> Float "
           "N:c:Int shapes:M:module sqrt:p:built in true:c:Bool !end: !else: !case: !and: !nil:");
  has("x := |", "nil:k not:k if:k switch:k position:v N:c !for: !return: !import:");
  has("x := position |", "and:k or:k is:k !in: !do: !position:");
  has("if position.x > 1 |", "and:k do:k !in:");
  has("for i in 0..<N |", "by:k do:k in:k and:k");
  has("x := counts[|", "end:k nil:k counts:v");
  has("x := counts[(|", "nil:k !end:");
  has("x: |", "Point:t Dir:t Int:t String:t shapes:M !position: !if: !nil:");
  has("x: []|", "Point:t !position:");
  has("x: [..8]P|", "Point:t !position:");
  has("f :: proc(a: &|", "Point:t !position:");
  has("f :: proc() -> |", "Point:t !position:");
  has("T :: |", "proc:k struct:k enum:k union:k static:k callback:k host:k N:c");
  has("T.f :: static |", "proc:k host:k callback:k !struct:");
  has("T :: host |", "proc:k struct:k");
  has("if flag do |", "return:k break:k continue:k !end: !and: position:v");
  has("x := \"po|", "!position:");
  has("# po|", "!position:");
  exact("x := 1|", "");
  has("for item in pts do log(it|", "item:v");

  /* inside a block: its words; and names the text declares that aren't compiled yet */
  has("if flag\n  |", "end:k else:k case:k break:k continue:k !import:");
  has("f :: proc(width: Int, other?: Point)\n  wi|", "width:v");
  has("f :: proc(width: Int, other?: Point)\n  |", "width:v other:v position:v end:k");
  has("for item, index in pts\n  it|", "item:v");
  has("f :: proc(left, top, wide: Float)\n  |", "left:v top:v wide:v");
  has("f :: proc()\n  log(left, top)\n  |", "!left: !top:");
  has("for item, index in pts\n  in|", "index:v");
  has("total := 0\nsum :: proc() -> Int\nShape :: struct\n|", "total:v sum:p Shape:t");
  exact("position := 4\nposi|", "position:v:Point"); /* (compiled: once, as the symbol) */
  exact("f :: proc(p: Point)\n  p.|", "x:f:Float y:f:Float len2:m:proc() -> Float");
  exact("f :: proc(p: &Point)\n  p.y|", "y:f:Float");
  exact("q := Point{x = 1, y = 2}\nq.x|", "x:f:Float");
  exact("f :: proc(b: Blob[..])\n  b.as|", "asString:m:built in");
  exact("f :: proc(b: Blob[..16], c: Blob[2])\n  c.as|", "asString:m:built in");
  exact("f :: proc(ps: [..8]Point)\n  ps[0].y|", "y:f:Float");
  exact("f :: proc(ps: []Point)\n  ps.l|", "len:m:built in");
  exact("Point.sum :: proc() -> Float\n  return self.|", "x:f:Float y:f:Float len2:m:proc() -> Float");

  /* where the word starts */
  n++;
  if ((at = complete("x := position.le|")) != 14) {
    printf("FAIL %d: the word starts at %d\n", n, at);
    fails++;
  }
  { /* anywhere at all in some text, broken or not: no trouble (the sanitizers' builds watch) */
    static const char *const texts[] = {
      "f :: proc(p: &Point, xs?: []Int) -> [..4]Point\n  for a, b in xs do log(a.)\n  q := pts[end - 1..].pop().len2() # x.\n"
      "  s := \"a.b\\\" .c\" + 'x' + name[0].to\n  return ((((]]]] .. ..< x..=y .\nend\n",
      ".", "..", "x.", ").", "].", "(.", "[.", "a[.", "a(.", "\"", "'", "#", "::", ": ", "-> ", "&", "?:", "]]]].", "))).", "a.b.c.d.e.",
    };
    size_t t, i;
    for (t = 0; t < sizeof texts / sizeof texts[0]; t++)
      for (i = 0; i <= strlen(texts[t]); i++) {
        gotLen = 0;
        rio_complete(&vm, texts[t], (uint32_t)i, item, 0);
        rio_complete(0, texts[t], (uint32_t)i, item, 0);
      }
    n++;
  }

  { /* a check (rio_check), as an editor's: a small vm, a program with far more memory than it has, cut
       at the start of the cursor's line; then completing there, from its tables */
    static unsigned char small[16 << 10], room[48 << 10], many[96 << 10];
    static char src[64 << 10];
    static const char *const cut = "Point :: struct\n  x, y: Float\nend\nbig: [200000]Int\nhuge: Blob[300000]\n"
                                   "f :: proc(p: Point, n: Int) -> Float\n  q := p\n  for i in 0..<n\n    w := big[i]\n";
    rio_Vm cv;
    size_t len = 0;
    int i, r;
    rio_init(&cv, small, sizeof small, 0, 0);
    r = rio_check(&cv, cut, (uint32_t)strlen(cut), 0, 0, room, sizeof room);
    n++;
    if (r != -1 || !strstr(rio_error(&cv), "missing 'end'")) {
      printf("FAIL %d: a check cut in a proc: %d %s\n", n, r, rio_error(&cv));
      fails++;
    }
    gotLen = 0;
    got[0] = 0;
    {
      static char text[1024];
      snprintf(text, sizeof text, "    q."); /* (the cursor's line: the rest is in the tables) */
      rio_complete(&cv, text, (uint32_t)strlen(text), item, 0);
    }
    n++;
    if (strcmp(got, "x:f:Float y:f:Float")) {
      printf("FAIL %d: completing after a check: %s\n", n, got);
      fails++;
    }
    gotLen = 0;
    got[0] = 0;
    {
      static char text[1024];
      snprintf(text, sizeof text, "    ");
      rio_complete(&cv, text, (uint32_t)strlen(text), item, 0);
    }
    n++;
    if (!offered("w:v:Int") || !offered("i:v:Int") || !offered("q:v:Point") || !offered("huge:v:Blob[300000]") || !offered("end:k") || offered("x:")) {
      printf("FAIL %d: a check's scope: %s\n", n, got);
      fails++;
    }
    r = rio_check(&cv, "big: [200000]Int\nlog(big[3])\n", 29, 0, 0, room, sizeof room); /* (whole: nothing to run, nothing written) */
    n++;
    if (r || rio_run(&cv) == 0) {
      printf("FAIL %d: a whole check: %d, and runnable after\n", n, r);
      fails++;
    }
    for (i = 0; i < 60; i++) { /* (far more code than the scratch holds: kept a statement at a time) */
      int j;
      len += (size_t)snprintf(src + len, sizeof src - len, "p%d :: proc(a: Int) -> Int\n  b := a\n", i);
      for (j = 0; j < 20; j++) len += (size_t)snprintf(src + len, sizeof src - len, "  if b > %d\n    b = b * 3 - a / %d\n  end\n", j, j + 1);
      len += (size_t)snprintf(src + len, sizeof src - len, "  return b\nend\n");
    }
    rio_init(&cv, small, sizeof small, 0, 0);
    r = rio_compileScratch(&cv, src, (uint32_t)len, many, sizeof many);
    n++;
    if (r != -1 || !strstr(rio_error(&cv), "out of memory")) { /* (the code fits scratch, taking the string pool's room, but not mem) */
      printf("FAIL %d: a long program's code should outgrow mem when compiled: %d %s\n", n, r, rio_error(&cv));
      fails++;
    }
    rio_init(&cv, small, sizeof small, 0, 0);
    r = rio_check(&cv, src, (uint32_t)len, 0, 0, many, sizeof many);
    n++;
    if (r) {
      printf("FAIL %d: a check of a long program: %s\n", n, rio_error(&cv));
      fails++;
    }
  }

  { /* rio_lookupAt: the name exactly, and where it's declared (the main source: no file; its line) */
    static unsigned char small[16 << 10], room[48 << 10];
    static const char *const src = "Point :: struct\n  across: Int\n  down: Int\nend\nfar :: proc(p: Point) -> Int\n  q := p\n";
    rio_Vm cv;
    rio_init(&cv, small, sizeof small, 0, 0);
    (void)rio_check(&cv, src, (uint32_t)strlen(src), 0, 0, room, sizeof room);
    found[0] = 0;
    rio_lookupAt(&cv, "  q.down", 8, foundAt, 0);
    n++;
    if (strcmp(found, "down Int 3 -1")) {
      printf("FAIL %d: lookup of a field: %s\n", n, found);
      fails++;
    }
    found[0] = 0;
    rio_lookupAt(&cv, "  x := far", 10, foundAt, 0);
    n++;
    if (strcmp(found, "far proc(Point) -> Int 5 -1")) {
      printf("FAIL %d: lookup of a proc: %s\n", n, found);
      fails++;
    }
    found[0] = 0;
    rio_lookupAt(&cv, "  x := far(q).acr", 17, foundAt, 0);
    n++;
    if (found[0]) {
      printf("FAIL %d: no lookup of part of a name: %s\n", n, found);
      fails++;
    }
  }

  /* no compiler: just the words and the text's names */
  n++;
  gotLen = 0;
  got[0] = 0;
  rio_complete(0, "abc := 1\na", 10, item, 0);
  if (strcmp(got, "abc:v")) {
    printf("FAIL %d: no vm: %s\n", n, got);
    fails++;
  }

  if (!fails) printf("all %d completion tests passed\n", n);
  return fails != 0;
}
