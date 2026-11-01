/* rio_format tests: each snippet must come out as given (and that, formatted again, the same); then
   random token soups, which it must take without changing a token (never -2) and lay out to stay */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "rio.h"

typedef struct {
  const char *src, *want;
} Case;

static const Case cases[] = {
  /* spaces between tokens */
  {"x:=1+2*y\n", "x := 1 + 2 * y\n"},
  {"f( a,b )\n", "f(a, b)\n"},
  {"a [ i ] = xs [i+1]\n", "a[i] = xs[i + 1]\n"},
  {"x = a-b\ny = -1\nz = f(-x)\nw = a - -b\n", "x = a - b\ny = -1\nz = f(-x)\nw = a - -b\n"},
  {"return -1\ncase -2\nfor i in -3..<3 by -1\n", "return -1\ncase -2\nfor i in -3..<3 by -1\n"},
  {"p :: proc(q: & Pos, r: &[3]Int, s: &?Int) -> ?Int\n", "p :: proc(q: &Pos, r: &[3]Int, s: &?Int) -> ?Int\n"},
  {"b := bits&~held\nc := &crew [3]\n", "b := bits & ~held\nc := &crew[3]\n"},
  {"Shape * :: 3\ncount *:= 1\nimport .extra*\nn := a *\n  b\n", "Shape* :: 3\ncount* := 1\nimport .extra*\nn := a *\n  b\n"},
  {"v := {1, 2} *k\nw := [2]Int {1,2}\nu := [4] ?Int{}\n", "v := {1, 2} * k\nw := [2]Int{1, 2}\nu := [4]?Int{}\n"},
  {"draw :: host proc(id: Int, o ?: Opts)\n", "draw :: host proc(id: Int, o?: Opts)\n"},
  {"x := a . b ( ) . c\ns := \"abc\" .len()\n", "x := a.b().c\ns := \"abc\".len()\n"},
  {"case .north\nd = if a do .north else .south\n", "case .north\nd = if a do .north else .south\n"},
  {"y := xs [end-1]\nz := xs[0 ..< n]\n", "y := xs[end - 1]\nz := xs[0..<n]\n"},
  {"outer : for i in 0..<3\n  break outer\nend\n", "outer: for i in 0..<3\n  break outer\nend\n"},
  {"m := Map [Int, String] {}\nt :: struct [K, V]\nend\n", "m := Map[Int, String]{}\nt :: struct[K, V]\nend\n"},
  {"if not(a) and b or c\nend\n", "if not (a) and b or c\nend\n"},
  {"x := 1 .5\ny := 1 . 5\n", "x := 1 .5\ny := 1 .5\n"},
  /* indents: blocks open and close */
  {"f :: proc(x: Int) -> Int\nif x > 0\nreturn 1\nelse\nreturn 2\nend\nend\n",
   "f :: proc(x: Int) -> Int\n  if x > 0\n    return 1\n  else\n    return 2\n  end\nend\n"},
  {"switch k\ncase 1\na()\ncase 2, 3\nb()\nelse\nc()\nend\n", "switch k\ncase 1\n  a()\ncase 2, 3\n  b()\nelse\n  c()\nend\n"},
  {"P :: struct\nx: Int\nend\nE :: enum\na\nend\nU :: union\nInt\nend\n",
   "P :: struct\n  x: Int\nend\nE :: enum\n  a\nend\nU :: union\n  Int\nend\n"},
  {"g :: static proc()\nend\nh :: callback proc()\nend\nk :: host proc()\nn := 1\n",
   "g :: static proc()\nend\nh :: callback proc()\nend\nk :: host proc()\nn := 1\n"},
  {"if a do b()\nfor i in 0..<3 do log(i)\nn := 1\n", "if a do b()\nfor i in 0..<3 do log(i)\nn := 1\n"},
  {"x := if a\n1\nelse\n2\nend\ny := switch k\ncase 1\n2\nend\n", "x := if a\n  1\nelse\n  2\nend\ny := switch k\ncase 1\n  2\nend\n"},
  {"r :: proc() -> Int do return 1\nn := 1\n", "r :: proc() -> Int do return 1\nn := 1\n"},
  {"      x := 1\n   end\nend\n", "x := 1\nend\nend\n"},
  /* lines going on keep their place from their statement, but not left of it */
  {"    xs := [3]Int{\n      1, 2,\n      3}\n", "xs := [3]Int{\n  1, 2, 3\n}\n"},
  {"xs := [3]Int{\n1, 2,\n3,\n}\n", "xs := [3]Int{\n  1, 2,\n  3,\n}\n"},
  {"if a and\n      b\n  x()\nend\n", "if a and\n      b\n  x()\nend\n"},
  {"s := \"one\n  \"two\"\n", "s := \"one\n  \"two\"\n"},
  {"if a\n    s := \"one\n        \"two\"\nend\n", "if a\n  s := \"one\n      \"two\"\nend\n"},
  /* lists over lines: a trailing comma, and a line break after the opener, say how */
  {"f(a,\n  b)\n", "f(a, b)\n"},
  {"p := Pos{x = 1,\n  y = 2}\n", "p := Pos{x = 1, y = 2}\n"},
  {"f(\n  a,\n      b)\n", "f(\n  a, b\n)\n"},
  {"f(a, b,)\n", "f(\n  a,\n  b,\n)\n"},
  {"xs := [4]Int{\n  1, 2,\n\n  3, 4,\n}\n", "xs := [4]Int{\n  1, 2,\n\n  3, 4,\n}\n"},
  {"v := Vec{\n  x,  # one\n  y   # two\n}\n", "v := Vec{\n  x,  # one\n  y,  # two\n}\n"},
  {"f(a, # one\n  b)\n", "f(\n  a, # one\n  b,\n)\n"},
  {"f(a, [1,\n  2,])\n", "f(a, [\n  1,\n  2,\n])\n"},
  {"f([1, 2,], b)\n", "f(\n  [\n    1,\n    2,\n  ],\n  b,\n)\n"},
  {"x := (a or\n      b)\n", "x := (a or\n      b)\n"},
  {"if f(a,\n  b)\n  g()\nend\n", "if f(a, b)\n  g()\nend\n"},
  {"s := f(\"one\n  \"two\", b)\n", "s := f(\n  \"one\n  \"two\",\n  b,\n)\n"},
  /* # nofmt to # /nofmt: left as it is (but spaces at line ends), the markers indented */
  {"if a\n# nofmt (a table)\n   x  = 1   \n   # /nofmt\ny:=2\nend\n", "if a\n  # nofmt (a table)\n   x  = 1\n  # /nofmt\n  y := 2\nend\n"},
  {"# nofmt\nf(a,\n  b)\n", "# nofmt\nf(a,\n  b)\n"},
  {"# nofmt:no\nx  = 1\n", "# nofmt:no\nx = 1\n"},
  /* comments: kept, lined up as they were */
  {"x := 1   # one\ny := 22  # two\n", "x := 1   # one\ny := 22  # two\n"},
  {"if a\n      x := 1        # one\n                    # more\nend\n", "if a\n  x := 1        # one\n                # more\nend\n"},
  {"x:=1# tight\n", "x := 1# tight\n"},
  {"  # alone\nx := 1\n", "# alone\nx := 1\n"},
  /* runs of spaces before ::, := and = line up with the lines around them, else go */
  {"bg*     :: 16\ncodeBg* :: 0\ntext*   :: 9\n", "bg*     :: 16\ncodeBg* :: 0\ntext*   :: 9\n"},
  {"a    := 1\nbb   := 2\n", "a    := 1\nbb   := 2\n"},
  {"a  := 1\nbbbbb := 2\n", "a := 1\nbbbbb := 2\n"},
  {"a [ i ]  = 1\nbb[i]    = 2\n", "a[i]     = 1\nbb[i]    = 2\n"},
  {"x   = 1\n", "x = 1\n"},
  /* line ends, blank lines, trailing spaces, no final newline */
  {"x:=1\r\n\r\ny:=2\r\n", "x := 1\r\n\r\ny := 2\r\n"},
  {"x:=1\r\ny:=2\nz:=3\n", "x := 1\r\ny := 2\r\nz := 3\r\n"},
  {"x := 1   \n   \n\n\ny := 2", "x := 1\n\n\n\ny := 2"},
  {"", ""},
};

static char out[1 << 16], again[1 << 16], soup[1 << 12];
static uint32_t seed = 12345;

static uint32_t next(void) {
  seed ^= seed << 13;
  seed ^= seed >> 17;
  seed ^= seed << 5;
  return seed;
}

/* a run of Rio's tokens (and some that aren't), with random spaces and lines */
static uint32_t makeSoup(void) {
  static const char *const words[] = {"x", "Int", "end", "if", "else", "for", "in", "do", "switch", "case", "proc",
    "struct", "enum", "union", "return", "not", "and", "or", "is", "nil", "static", "callback", "host", "by", "import",
    "1", "0x1F", "2.5", "1e-3", "1_000", "'a'", "'\\n'", "'", "\"s\"", "\"a b\"", "\"open", "\"\\\"", "# note", "::",
    ":=", "=", "==", "!=", "<", "<=", ">", ">=", "<<", ">>", ">>>", "<<=", ">>=", ">>>=", "->", "+", "-", "*", "/",
    "%", "&", "|", "^", "~", "!", "&&", "||", "+=", "-=", "*=", ".", "..<", "..=", "..", ",", ":", ";", "?", "(", ")",
    "[", "]", "{", "}", "@", "$", "\t", "\r", "é"};
  uint32_t n = 0, count = next() % 200;
  while (count-- && n < sizeof soup - 16) {
    uint32_t r = next() % 100;
    const char *w;
    if (r < 10) {
      soup[n++] = '\n';
      for (r = next() % 6; r--;) soup[n++] = ' ';
      continue;
    }
    w = words[next() % (sizeof words / sizeof words[0])];
    if (r < 55) soup[n++] = ' ';
    if (r < 60) soup[n++] = ' ';
    memcpy(soup + n, w, strlen(w));
    n += (uint32_t)strlen(w);
  }
  return n;
}

int main(int argc, char **argv) {
  int failed = 0, passed = 0, soups = argc > 1 ? atoi(argv[1]) : 20000, i; /* (rio_fmt_test 1000000 7: more soups, another seed) */
  if (argc > 2) seed = (uint32_t)atoi(argv[2]);
  for (i = 0; i < (int)(sizeof cases / sizeof cases[0]); i++) {
    const Case *c = &cases[i];
    int32_t n = rio_format(c->src, (uint32_t)strlen(c->src), out, sizeof out), m;
    if (n < 0 || strcmp(out, c->want)) {
      printf("FAIL case %d (%d):\n%s\n-- want --\n%s\n-- got --\n%s\n", i, n, c->src, c->want, n < 0 ? "" : out);
      failed++;
      continue;
    }
    m = rio_format(out, (uint32_t)n, again, sizeof again);
    if (m != n || memcmp(out, again, (size_t)n)) {
      printf("FAIL case %d: formatting it again changed it:\n%s\n", i, again);
      failed++;
      continue;
    }
    passed++;
  }
  if (rio_format("x := 1\n", 7, out, 4) != -1) {
    printf("FAIL: too little room should give -1\n");
    failed++;
  } else passed++;
  for (i = 0; i < soups; i++) {
    uint32_t len = makeSoup();
    int32_t n = rio_format(soup, len, out, sizeof out), m;
    if (n < 0) {
      printf("FAIL soup %d gave %d:\n%.*s\n-- formatted --\n%s\n", i, n, (int)len, soup, n == -2 ? out : "");
      failed++;
      break;
    }
    m = rio_format(out, (uint32_t)n, again, sizeof again);
    if (m != n || memcmp(out, again, (size_t)n)) {
      printf("FAIL soup %d: formatting it again changed it:\n%.*s\n-- once --\n%s\n-- twice --\n%s\n", i, (int)len, soup, out, again);
      failed++;
      break;
    }
  }
  if (i == soups) passed++;
  if (failed) return 1;
  printf("all %d fmt tests passed\n", passed);
  return 0;
}
