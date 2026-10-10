/* error reporting tests: each snippet must fail with exactly this kind, position and message */
#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <string.h>
#include "rio.h"

static unsigned char mem[1 << 20];
static RioIns code[4096];
static Rio vm;

typedef struct {
  const char *src, *def, *defval; /* optional -D */
  uint32_t memsize;               /* 0 = all of mem */
  int kind, line, col, len;       /* line/col/len -1 = don't check */
  const char *msg;                /* substring of the message */
} Case;

static const Case cases[] = {
  /* compile errors point at the token at fault */
  {"x := 1\ny := x + nope\n", 0, 0, 0, RIO_ECOMPILE, 2, 10, 4, "undefined name"},
  {"x: Nope = 1\n", 0, 0, 0, RIO_ECOMPILE, 1, 4, 4, "type expected"},
  {"v := 3\nif v\nend\n", 0, 0, 0, RIO_ECOMPILE, 2, 4, 1, "condition must be Bool"},
  {"s := \"abc\"\ns[0] = 1\n", 0, 0, 0, RIO_ECOMPILE, 2, 6, 1, "cannot assign"},
  {"f :: proc(n: Int) -> Int\n  return f(n)\nend\n", 0, 0, 0, RIO_ECOMPILE, 2, 10, 1, "recursion is not allowed"},
  {"V :: struct\n  x: Float\nend\nv := V{1}\n", 0, 0, 0, RIO_ECOMPILE, 4, 8, 1, "must be named"},
  {"V :: struct\n  x: Float\nend\nv := V{x = 1, x = 2}\n", 0, 0, 0, RIO_ECOMPILE, 4, 15, 1, "field set twice"},
  {"for i in 0..<3\n  break\nend\nbreak\n", 0, 0, 0, RIO_ECOMPILE, 4, 1, 5, "not inside a loop"},
  {"for c in \"abc\"\n  c = 1\nend\n", 0, 0, 0, RIO_ECOMPILE, 2, 5, 1, "cannot assign"},
  {"for x, i in 0..<3\nend\n", 0, 0, 0, RIO_ECOMPILE, 1, -1, -1, "a range loop has no index"},
  {"n := 3\nfor x in n\nend\n", 0, 0, 0, RIO_ECOMPILE, 2, -1, -1, "for needs a range, array, slice or list"},
  {"xs: [2]Int\nfor x, in xs\nend\n", 0, 0, 0, RIO_ECOMPILE, 2, 8, 2, "index name expected"},
  {"xs: [2]Int\nfor x, i on xs\nend\n", 0, 0, 0, RIO_ECOMPILE, 2, 10, 2, "'in' expected"},
  {"B :: struct\n  h: Int\nend\nf :: proc(b: &B)\nend\nv: B\nf(v)\n", 0, 0, 0, RIO_ECOMPILE, 7, 4, 1, "this parameter is a reference: pass &x"},
  {"B :: struct\n  h: Int\nend\nf :: proc(b: &B)\nend\nv: B\nf(&v.h)\n", 0, 0, 0, RIO_ECOMPILE, 7, 7, 1, "type mismatch"},
  {"g :: proc() -> Int\n  return 1\nend\nr := &g()\n", 0, 0, 0, RIO_ECOMPILE, 4, -1, -1, "& needs a variable, field or element that can be changed"},
  {"s := \"abc\"\nr := &s[0]\n", 0, 0, 0, RIO_ECOMPILE, 2, -1, -1, "& needs a variable, field or element that can be changed"},
  {"K :: 5\nk := &K\n", 0, 0, 0, RIO_ECOMPILE, 2, -1, -1, "& needs a variable, field or element that can be changed"},
  {"v := 1\nlog(&v)\n", 0, 0, 0, RIO_ECOMPILE, 2, -1, -1, "&x only goes to a & parameter"},
  {"v := 1\nw := v\nw = &v\n", 0, 0, 0, RIO_ECOMPILE, 3, -1, -1, "&x only goes to a & parameter"},
  {"v := 1\nh :: proc() -> Int\n  return &v\nend\n", 0, 0, 0, RIO_ECOMPILE, 3, -1, -1, "&x only goes to a & parameter"},
  {"v := 1\nw: Int = &v\n", 0, 0, 0, RIO_ECOMPILE, 2, -1, -1, "a reference takes its type from its place"},
  {"v := 1\nr* := &v\n", 0, 0, 0, RIO_ECOMPILE, 2, -1, -1, "a reference can't be exported"},
  {"a: [3]Float\nb: [3]Float\ns: []Float = a + b\n", 0, 0, 0, RIO_ECOMPILE, 3, -1, -1, "a computed array has no home to view"},
  {"a: [3]Float\nfor x in a + a\nend\n", 0, 0, 0, RIO_ECOMPILE, 2, -1, -1, "a computed array has no home to view"},
  {"a: [3]Float\nt := (a + a)[0:2]\n", 0, 0, 0, RIO_ECOMPILE, 2, -1, -1, "a computed array has no home to view"},
  {"M :: struct\n  x: Float\n  n: Int\nend\nm: M\nm2 := m + m\n", 0, 0, 0, RIO_ECOMPILE, 6, -1, -1, "arrays and structs made of one number type"},
  {"a: [3]Float\nb: [2]Float\nc := a + b\n", 0, 0, 0, RIO_ECOMPILE, 3, -1, -1, "type mismatch"},
  {"a: [3]Float\ni := 1\nc := a * i\n", 0, 0, 0, RIO_ECOMPILE, 3, -1, -1, "type mismatch"},
  {"a: [3]Float\nc := a % a\n", 0, 0, 0, RIO_ECOMPILE, 2, -1, -1, "integer operator on floats"},
  {"a: [3]Float\nw := a == a\n", 0, 0, 0, RIO_ECOMPILE, 2, -1, -1, "only do + - * / %"},
  {"a: [20]Float\nc := a + a\n", 0, 0, 0, RIO_ECOMPILE, 2, -1, -1, "too big for a temporary"},
  {"v: [2]Float\nlog(v.z)\n", 0, 0, 0, RIO_ECOMPILE, 2, 7, 1, "no such element: the array is shorter"},
  {"v: [2]Float\nlog(v.xr)\n", 0, 0, 0, RIO_ECOMPILE, 2, 7, 2, "use xyzw or rgba, not both"},
  {"v: [5]Float\nlog(v.x)\n", 0, 0, 0, RIO_ECOMPILE, 2, -1, -1, "swizzles like .x .zy .rgba need 1-4 letters"},
  {"v: [2]Float\nv.xy = v\n", 0, 0, 0, RIO_ECOMPILE, 2, -1, -1, "cannot assign"},
  {"big: [100]Int\nc := big\n", 0, 0, 0, RIO_ECOMPILE, 2, -1, -1, "this copies a big array (over 16 words)"},
  {"B :: struct\n  xs: [20]Int\nend\nb: B\nc := b\n", 0, 0, 0, RIO_ECOMPILE, 5, -1, -1, "this copies a big struct (over 16 words)"},
  {"f :: proc(g: [17]Int)\nend\n", 0, 0, 0, RIO_ECOMPILE, 1, -1, -1, "a big value (over 16 words) goes by reference"},
  {"f :: proc() -> [17]Int\nend\n", 0, 0, 0, RIO_ECOMPILE, 1, -1, -1, "a big value (over 16 words) can't be returned"},
  {"buf: [4]Int\nf :: proc(o: [..]Int)\nend\nf(buf)\n", 0, 0, 0, RIO_ECOMPILE, 4, -1, -1, "a builder needs somewhere to keep its length"},
  {"t: Blob[..] = \"abc\"\n", 0, 0, 0, RIO_ECOMPILE, 1, -1, -1, "can't build on read-only data"},
  {"fs: [4]Float\nb: [..]Int = fs\n", 0, 0, 0, RIO_ECOMPILE, 2, -1, -1, "type mismatch"},
  {"buf: [4]Int\nb: [..]Int = buf\nlog(b[0])\n", 0, 0, 0, RIO_ERUNTIME, 3, 0, 0, "index out of bounds"},
  /* views of a proc's own memory: the next call rewrites it, so they can't be returned or kept */
  {"f :: proc() -> []Int\n  a: [4]Int\n  return a[:]\nend\n", 0, 0, 0, RIO_ECOMPILE, 3, -1, -1, "this returns a view of the proc's own memory"},
  {"f :: proc() -> []Int\n  a: [4]Int\n  return a\nend\n", 0, 0, 0, RIO_ECOMPILE, 3, -1, -1, "this returns a view of the proc's own memory"},
  {"f :: proc(id: Int) -> String\n  buf: Blob[16]\n  b: Blob[..] = buf\n  b.format(\"p \", id)\n  return b[:].asString()\nend\n", 0, 0, 0, RIO_ECOMPILE, 5, -1, -1, "this returns a view of the proc's own memory"},
  {"f :: proc() -> [..]Int\n  xs: [..4]Int\n  return xs\nend\n", 0, 0, 0, RIO_ECOMPILE, 3, -1, -1, "this returns a view of the proc's own memory"},
  {"ga: [4]Int\nf :: proc() -> [..]Int\n  b: [..]Int = ga\n  return b\nend\n", 0, 0, 0, RIO_ECOMPILE, 4, -1, -1, "this returns a view of the proc's own memory"}, /* its length lives here */
  {"g: []Int\nf :: proc()\n  a: [4]Int\n  g = a[:]\nend\n", 0, 0, 0, RIO_ECOMPILE, 4, -1, -1, "this view of the proc's own memory would outlive the call"},
  {"g: []Int\nf :: proc()\n  a: [4]Int\n  s := a[1:3]\n  t := s\n  g = t\nend\n", 0, 0, 0, RIO_ECOMPILE, 6, -1, -1, "this view of the proc's own memory would outlive the call"},
  {"f :: proc(out: &[]Int)\n  a: [4]Int\n  out = a[:]\nend\n", 0, 0, 0, RIO_ECOMPILE, 3, -1, -1, "this view of the proc's own memory would outlive the call"},
  /* views of globals and parameters, and local views passed down or replaced, are fine */
  {"g: []Int\nga: [4]Int\nf :: proc(v: []Int) -> []Int\n  a: [4]Int\n  s := a[:]\n  n := len(s)\n  s = ga[:]\n  g = s\n  return v[1:]\nend\nsum :: proc(v: []Int) -> Int\n  return len(v)\nend\nh :: proc() -> Int\n  a: [4]Int\n  return sum(a[:])\nend\nlog(len(f(ga)), h())\n", 0, 0, 0, RIO_ENONE, 0, 0, 0, ""},
  {"x := 1\nelse\n", 0, 0, 0, RIO_ECOMPILE, 2, 1, 4, "'else' without 'if'"},
  {"x := 1\nend\n", 0, 0, 0, RIO_ECOMPILE, 2, 1, 3, "'end' without block"},
  {"x := 1\nreturn\n", 0, 0, 0, RIO_ECOMPILE, 2, 1, 6, "return outside proc"},
  {"xs: [..2]Int\nxs.format(1)\n",0, 0, 0, RIO_ECOMPILE, 2, 10, 1, "format needs a Blob list"},
  {"S :: struct\n  x: Int\nend\nS.f :: proc()\n  self.f()\nend\n", 0, 0, 0, RIO_ECOMPILE, 5, 8, 1, "recursion is not allowed"},
  {"S :: struct\n  x: Int\nend\nS.x :: proc()\nend\n", 0, 0, 0, RIO_ECOMPILE, 4, 3, 1, "same name as a field"},
  {"S :: struct\n  x: Int\nend\nS.f :: proc()\nend\nS.f :: proc()\nend\n", 0, 0, 0, RIO_ECOMPILE, 6, 3, 1, "method already defined"},
  {"S :: struct\n  x: Int\nend\nS.f :: proc()\nend\ns: S\ng := s.f\n", 0, 0, 0, RIO_ECOMPILE, 7, 8, 1, "method calls need ()"},
  {"S :: struct\n  x: Int\nend\ns: S\ns.nope()\n", 0, 0, 0, RIO_ECOMPILE, 5, 3, 4, "no such field or method"},
  {"xs: [..2]Int\npush(xs, 1)\n", 0, 0, 0, RIO_ECOMPILE, 2, 1, 4, "undefined name"},
  {"V :: struct\n  x: Float\nend\nv := {x = 1}\n", 0, 0, 0, RIO_ECOMPILE, 4, 6, 1, "can't tell which struct"},
  {"V :: struct\n  x: Float\nend\nlog({x = 1})\n", 0, 0, 0, RIO_ECOMPILE, 4, 5, 1, "can't tell which struct"},
  {"V :: struct\n  x: Float\nend\nn: Int = {x = 1}\n", 0, 0, 0, RIO_ECOMPILE, 4, 10, 1, "can't tell which struct"},
  {"V :: struct\n  x, y: Int\nend\nxs: [2]Int\nv := V{xs[0]}\n", 0, 0, 0, RIO_ECOMPILE, 5, 13, 1, "only a name or a.b path"},
  {"V :: struct\n  x, y: Int\nend\nz := 1\nv := V{z}\n", 0, 0, 0, RIO_ECOMPILE, 5, 9, 1, "no such field"},
  {"V :: struct\n  x, y: Int\nend\nx := 1\nv := V{x, x = 2}\n", 0, 0, 0, RIO_ECOMPILE, 5, 11, 1, "field set twice"},
  {"V :: struct\n  x, y: Int\nend\nv: V\n{x} = v\n", 0, 0, 0, RIO_ECOMPILE, 5, 5, 1, "':=' expected"},
  {"V :: struct\n  x, y: Int\nend\nv: V\n{x as} := v\n", 0, 0, 0, RIO_ECOMPILE, 5, 6, 1, "name expected after 'as'"},
  {"{x} := 5\n", 0, 0, 0, RIO_ECOMPILE, 1, 8, 1, "only a struct can be destructured"},
  {"V :: struct\n  x, y: Int\nend\nv: V\n{x, z} := v\n", 0, 0, 0, RIO_ECOMPILE, 5, 11, 1, "no such field to destructure"},
  {"x := Float(3)\n", 0, 0, 0, RIO_ECOMPILE, 1, 11, 1, "convert with x.toInt()"},
  {"s := \"a\".toFloat()\n", 0, 0, 0, RIO_ECOMPILE, 1, 18, 1, "no such conversion"},
  {"Int.toFloat :: proc() -> Float\n  return 0\nend\n", 0, 0, 0, RIO_ECOMPILE, 1, 5, 7, "that conversion is built in"},
  /* ...or at the last token of an expression that can only be judged once it's complete */
  {"x := 1\n  y := 2.0 + \"s\"\n", 0, 0, 0, RIO_ECOMPILE, 2, 14, 3, "type mismatch"},
  {"x := (1 + 2\n", 0, 0, 0, RIO_ECOMPILE, 1, 11, 1, "unclosed bracket"},
  {"if true\n  x := 1\n", 0, 0, 0, RIO_ECOMPILE, 2, 8, 1, "missing 'end'"},
  {"s := \"abc\n", 0, 0, 0, RIO_ECOMPILE, -1, -1, -1, "unterminated string"},
  /* runtime errors report the line */
  {"xs: [4]Int\ni := 2\nf :: proc(n: Int) -> Int\n  return xs[n * 3]\nend\nlog(f(i))\n", 0, 0, 0, RIO_ERUNTIME, 4, 0, 0, "index out of bounds"},
  {"xs: [..2]Int\nfor j in 0..<3\n  xs.push(j)\nend\nlog(xs.pop(), xs.pop(), xs.pop())\n", 0, 0, 0, RIO_ERUNTIME, 5, 0, 0, "pop from empty list"},
  {"main :: proc()\n  a := 0\n  b := 5\n  log(b / a)\nend\n", 0, 0, 0, RIO_ERUNTIME, 4, 0, 0, "division by zero"},
  {"a: [3]Int\nn := 5\nb := a[1:n]\n", 0, 0, 0, RIO_ERUNTIME, 3, 0, 0, "slice out of bounds"},
  {"xs: [2]Int\ni := 0\nfor xs[i] == 0\n  i += 1\nend\n", 0, 0, 0, RIO_ERUNTIME, 3, 0, 0, "index out of bounds"}, /* rotated loop condition */
  {"a: [3]Int\na[0] = 1\na[2] = 4\nb: [3]Int\nb[0] = 1\nb[2] = 1\nc := a / b\n", 0, 0, 0, RIO_ERUNTIME, 7, 0, 0, "division by zero"},
  /* a view of a block's array outlives the block: those slots must not be reused for t, or writing
     through s would forge t's address and length */
  {"s: []Int\nif true\n  a: [2]Int\n  s = a[:]\nend\nif true\n  t: []Int = s\n  s[1] = 2000000000\n  s[0] = 4\n  log(t[100000000])\nend\n", 0, 0, 0, RIO_ERUNTIME, 10, 0, 0, "index out of bounds"},
  {"keep :: proc(v: []Int) -> []Int\n  return v\nend\nf :: proc() -> Int\n  s: []Int\n  if true\n    a: [2]Int\n    s = keep(a[:])\n  end\n  if true\n    t: []Int = s\n    s[1] = 2000000000\n    return t[100000000]\n  end\n  return 0\nend\nlog(f())\n", 0, 0, 0, RIO_ERUNTIME, 13, 0, 0, "index out of bounds"},
  {"main :: proc()\n  again()\nend\n", 0, 0, 0, RIO_ERUNTIME, 2, 0, 0, "rio is already running"},
  {"main :: proc()\n  log(\"x\")\n  boom()\nend\n", 0, 0, 0, RIO_ERUNTIME, 3, 0, 0, "the host said no"},
  /* errors with no source position */
  {"N :: 10\n", "N", "2.5", 0, RIO_ECOMPILE, 0, 0, 0, "-D N: value doesn't fit Int"},
  {"N :: 10\n", "9x", "1", 0, RIO_ECOMPILE, 0, 0, 0, "-D: bad name 9x"},
  {"x := 1\n", 0, 0, 4096, RIO_ECOMPILE, 0, 0, 0, "not enough memory for the compiler"},
  /* success clears the previous error */
  {"x := 1\n", 0, 0, 0, RIO_ENONE, 0, 0, 0, ""},
};

static void boom(Rio *v, RioVal *a) { (void)a; rio_trap(v, "the host said no"); }
/* calls back into rio, which is refused: procs that never run at the same time share memory */
static void again(Rio *v, RioVal *a) {
  char m[128]; const char *e; size_t i;
  (void)a;
  if (rio_call(v, rio_func(v, "main")) == -1) {
    for (e = rio_error(v), i = 0; e[i] && i < sizeof m - 1; i++) m[i] = e[i];
    m[i] = 0; rio_trap(v, m);
  }
}

int main(void) {
  int i, fails = 0, n = (int)(sizeof cases / sizeof cases[0]);
  for (i = 0; i < n; i++) {
    const Case *c = &cases[i]; RioError e; int f;
    rio_init(&vm, mem, c->memsize ? c->memsize : sizeof mem, code, 4096);
    rio_ffi(&vm, "boom", "", boom);
    rio_ffi(&vm, "again", "", again);
    if (c->def) rio_define(&vm, c->def, c->defval);
    if (!rio_compile(&vm, c->src, (uint32_t)strlen(c->src)) && !rio_run(&vm)) {
      if ((f = rio_func(&vm, "main")) >= 0) rio_call(&vm, f);
    }
    e = rio_error_info(&vm);
    if (e.kind != c->kind || (c->line >= 0 && e.line != c->line) || (c->col >= 0 && e.col != c->col) ||
        (c->len >= 0 && e.len != c->len) || !strstr(e.msg, c->msg)) {
      printf("FAIL case %d: got kind %d %d:%d len %d \"%s\"; want kind %d %d:%d len %d \"%s\"\n",
             i, e.kind, e.line, e.col, e.len, e.msg, c->kind, c->line, c->col, c->len, c->msg);
      fails++;
    }
  }
  { /* a line of over 4096 instructions: the packed line table takes long steps; the error is on line 4 */
    static char big[40000]; static RioIns bigcode[8192]; int k, len = sprintf(big, "xs: [2]Int\nx := 1\ni := 5\ny := x");
    RioError e;
    for (k = 0; k < 5000; k++) len += sprintf(big + len, " + x");
    len += sprintf(big + len, " + xs[i]\n");
    rio_init(&vm, mem, sizeof mem, bigcode, 8192);
    n++;
    if (rio_compile(&vm, big, (uint32_t)len)) { printf("FAIL long line: %s\n", rio_error(&vm)); fails++; }
    else {
      rio_run(&vm); e = rio_error_info(&vm);
      if (e.kind != RIO_ERUNTIME || e.line != 4) { printf("FAIL long line: kind %d line %d \"%s\"\n", e.kind, e.line, e.msg); fails++; }
    }
  }
  { /* explicit limits: tiny tables fail cleanly, and a scratch buffer too small for them is refused */
    static unsigned char scratch[1 << 16];
    static const char *many = "a := 1\nb := 2\nc := 3\nd := 4\ne := 5\nf := 6\ng := 7\nh := 8\n";
    RioLimits l = rio_limits_for(sizeof scratch);
    RioError e;
    l.syms = 33; /* the builtins take 28, so the 6th global is one too many */
    rio_init(&vm, mem, sizeof mem, code, 4096);
    if (!rio_compile_ex(&vm, many, (uint32_t)strlen(many), scratch, sizeof scratch, &l) ||
        !strstr((e = rio_error_info(&vm)).msg, "too many symbols") || e.line != 6) {
      printf("FAIL explicit limits: \"%s\"\n", rio_error(&vm)); fails++; n++;
    } else n++;
    l = rio_limits_for(1 << 20);
    rio_init(&vm, mem, sizeof mem, code, 4096);
    if (!rio_compile_ex(&vm, many, (uint32_t)strlen(many), scratch, sizeof scratch, &l) ||
        !strstr(rio_error(&vm), "scratch too small for these limits")) {
      printf("FAIL limits bigger than scratch: \"%s\"\n", rio_error(&vm)); fails++; n++;
    } else n++;
  }
  if (!fails) printf("all %d error tests passed\n", n);
  return fails != 0;
}
