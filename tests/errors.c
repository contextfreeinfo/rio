/* error reporting tests: each snippet must fail with exactly this kind, position and message */
#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <string.h>
#include "rio.h"

static unsigned char mem[1 << 20];
static rio_Instr code[4096];
static rio_Vm vm;

typedef struct {
  const char *src, *def, *defval; /* optional -D */
  uint32_t memsize;               /* 0 = all of mem */
  int kind, line, col, len;       /* line/col/len -1 = don't check */
  const char *msg;                /* substring of the message */
} Case;

static const Case cases[] = {
  /* compile errors point at the token at fault */
  {"x := 1\ny := x + nope\n", 0, 0, 0, rio_ErrorKind_compile, 2, 10, 4, "undefined name"},
  {"x: Nope = 1\n", 0, 0, 0, rio_ErrorKind_compile, 1, 4, 4, "type expected"},
  {"v := 3\nif v\nend\n", 0, 0, 0, rio_ErrorKind_compile, 2, 4, 1, "condition must be Bool"},
  {"s := \"abc\"\ns[0] = 1\n", 0, 0, 0, rio_ErrorKind_compile, 2, 6, 1, "cannot assign"},
  {"f :: proc(n: Int) -> Int\n  return f(n)\nend\n", 0, 0, 0, rio_ErrorKind_compile, 2, 10, 1, "recursion is not allowed"},
  {"b := hexBlob(\"abc\")\n", 0, 0, 0, rio_ErrorKind_compile, 1, -1, -1, "odd number of hex digits"},
  {"b := hexBlob(\"0g\")\n", 0, 0, 0, rio_ErrorKind_compile, 1, -1, -1, "only hex digits"},
  {"s := \"ab\"\nb := hexBlob(s)\n", 0, 0, 0, rio_ErrorKind_compile, 2, -1, -1, "string literal"},
  {"V :: struct\n  x: Float\nend\nv := V{1}\n", 0, 0, 0, rio_ErrorKind_compile, 4, 8, 1, "must be named"},
  {"V :: struct\n  x: Float\nend\nv := V{x = 1, x = 2}\n", 0, 0, 0, rio_ErrorKind_compile, 4, 15, 1, "field set twice"},
  {"for i in 0..<3\n  break\nend\nbreak\n", 0, 0, 0, rio_ErrorKind_compile, 4, 1, 5, "not inside a loop"},
  {"a: for i in 0..<3\n  for j in 0..<3\n    break b\n  end\nend\n", 0, 0, 0, rio_ErrorKind_compile, 3, 11, 1, "no loop with this label"},
  {"a: for i in 0..<3\n  a: for j in 0..<3\n  end\nend\n", 0, 0, 0, rio_ErrorKind_compile, 2, 3, 1, "a loop with this label is already open"},
  {"a: for i in 0..<3\nend\nf :: proc()\n  for\n    break a\n  end\nend\n", 0, 0, 0, rio_ErrorKind_compile, 5, 11, 1, "no loop with this label"},
  {"for c in \"abc\"\n  c = 1\nend\n", 0, 0, 0, rio_ErrorKind_compile, 2, 5, 1, "cannot assign"},
  {"for x, i in 0..<3\nend\n", 0, 0, 0, rio_ErrorKind_compile, 1, -1, -1, "a range loop has no index"},
  {"n := 3\nfor x in n\nend\n", 0, 0, 0, rio_ErrorKind_compile, 2, -1, -1, "for needs a range, array, slice or list"},
  {"xs: [2]Int\nfor x, in xs\nend\n", 0, 0, 0, rio_ErrorKind_compile, 2, 8, 2, "index name expected"},
  {"xs: [2]Int\nfor x, i on xs\nend\n", 0, 0, 0, rio_ErrorKind_compile, 2, 10, 2, "'in' expected"},
  {"B :: struct\n  h: Int\nend\nf :: proc(b: &B)\nend\nv: B\nf(v)\n", 0, 0, 0, rio_ErrorKind_compile, 7, 4, 1, "this parameter is a reference: pass &x"},
  {"L :: struct\n  t: String\nend\nf :: proc(l: L)\nend\n", 0, 0, 0, rio_ErrorKind_compile, 4, 15, 1, "holding slices or strings goes by reference: use &T"},
  /* maybes: ?T */
  {"x: ??Int\n", 0, 0, 0, rio_ErrorKind_compile, 1, 6, -1, "this can already be nil"},
  {"U :: union\n  nil\n  Int\nend\nx: ?U\n", 0, 0, 0, rio_ErrorKind_compile, 5, 5, -1, "this can already be nil"},
  {"x: ?Int = 2\nlog(x + 1)\n", 0, 0, 0, rio_ErrorKind_compile, 2, 10, -1, "this may be nil: check first"},
  {"f :: proc(n: Int) -> Int\n  return n\nend\nx: ?Int = 2\nlog(f(x))\n", 0, 0, 0, rio_ErrorKind_compile, 5, 8, -1, "this may be nil: check first"},
  {"x: ?Int\nswitch x\ncase nil\n  log(1)\nend\n", 0, 0, 0, rio_ErrorKind_compile, 5, 1, -1, "this switch has no case for Int"},
  {"x: ?Int = 3.5\n", 0, 0, 0, rio_ErrorKind_compile, 1, 11, -1, "type mismatch"},
  /* views passed in are borrowed: kept past the call, they'd maybe view the caller's own memory; a call's result views what went in */
  {"g := \"\"\nkeep :: proc(s: String)\n  g = s\nend\n", 0, 0, 0, rio_ErrorKind_compile, 3, 7, -1, "came in as a parameter"},
  {"g := \"\"\nkeep :: proc(s: String)\n  t := s\n  g = t\nend\n", 0, 0, 0, rio_ErrorKind_compile, 4, 7, -1, "came in as a parameter"},
  {"g := \"\"\nid :: proc(s: String) -> String\n  return s[1:]\nend\nf :: proc()\n  b: Blob[8]\n  g = id(b[:].asString())\nend\n", 0, 0, 0, rio_ErrorKind_compile, 7, 25, -1, "view of the proc's own memory would outlive the call"},
  {"g := \"\"\nid :: proc(s: String) -> String\n  return s[1:]\nend\nf :: proc(s: String)\n  g = id(s)\nend\n", 0, 0, 0, rio_ErrorKind_compile, 6, 11, -1, "came in as a parameter"},
  {"g: []Int\nkeep :: proc(xs: []Int)\n  g = xs\nend\n", 0, 0, 0, rio_ErrorKind_compile, 3, 7, -1, "came in as a parameter"},
  {"B :: struct\n  h: Int\nend\nf :: proc(b: &B)\nend\nv: B\nf(&v.h)\n", 0, 0, 0, rio_ErrorKind_compile, 7, 7, 1, "type mismatch"},
  {"g :: proc() -> Int\n  return 1\nend\nr := &g()\n", 0, 0, 0, rio_ErrorKind_compile, 4, -1, -1, "& needs a variable, field or element that can be changed"},
  {"s := \"abc\"\nr := &s[0]\n", 0, 0, 0, rio_ErrorKind_compile, 2, -1, -1, "& needs a variable, field or element that can be changed"},
  {"K :: 5\nk := &K\n", 0, 0, 0, rio_ErrorKind_compile, 2, -1, -1, "& needs a variable, field or element that can be changed"},
  {"v := 1\nlog(&v)\n", 0, 0, 0, rio_ErrorKind_compile, 2, -1, -1, "&x only goes to a & parameter"},
  {"v := 1\nw := v\nw = &v\n", 0, 0, 0, rio_ErrorKind_compile, 3, -1, -1, "&x only goes to a & parameter"},
  {"v := 1\nh :: proc() -> Int\n  return &v\nend\n", 0, 0, 0, rio_ErrorKind_compile, 3, -1, -1, "&x only goes to a & parameter"},
  {"v := 1\nw: Int = &v\n", 0, 0, 0, rio_ErrorKind_compile, 2, -1, -1, "a reference takes its type from its place"},
  {"v := 1\nr* := &v\n", 0, 0, 0, rio_ErrorKind_compile, 2, -1, -1, "a reference can't be exported"},
  {"a: [3]Float\nb: [3]Float\ns: []Float = a + b\n", 0, 0, 0, rio_ErrorKind_compile, 3, -1, -1, "a computed array has no home to view"},
  {"a: [3]Float\nfor x in a + a\nend\n", 0, 0, 0, rio_ErrorKind_compile, 2, -1, -1, "a computed array has no home to view"},
  {"a: [3]Float\nt := (a + a)[0:2]\n", 0, 0, 0, rio_ErrorKind_compile, 2, -1, -1, "a computed array has no home to view"},
  {"M :: struct\n  x: Float\n  n: Int\nend\nm: M\nm2 := m + m\n", 0, 0, 0, rio_ErrorKind_compile, 6, -1, -1, "arrays and structs made of one number type"},
  {"a: [3]Float\nb: [2]Float\nc := a + b\n", 0, 0, 0, rio_ErrorKind_compile, 3, -1, -1, "type mismatch"},
  {"a: [3]Float\ni := 1\nc := a * i\n", 0, 0, 0, rio_ErrorKind_compile, 3, -1, -1, "type mismatch"},
  {"a: [3]Float\nc := a % a\n", 0, 0, 0, rio_ErrorKind_compile, 2, -1, -1, "integer operator on floats"},
  {"a: [3]Float\nw := a == a\n", 0, 0, 0, rio_ErrorKind_compile, 2, -1, -1, "only do + - * / %"},
  {"a: [20]Float\nc := a + a\n", 0, 0, 0, rio_ErrorKind_compile, 2, -1, -1, "too big for a temporary"},
  {"v: [2]Float\nlog(v.z)\n", 0, 0, 0, rio_ErrorKind_compile, 2, 7, 1, "no such element: the array is shorter"},
  {"v: [2]Float\nlog(v.xr)\n", 0, 0, 0, rio_ErrorKind_compile, 2, 7, 2, "use xyzw or rgba, not both"},
  {"v: [5]Float\nlog(v.x)\n", 0, 0, 0, rio_ErrorKind_compile, 2, -1, -1, "swizzles like .x .zy .rgba need 1-4 letters"},
  {"v: [2]Float\nv.xy = v\n", 0, 0, 0, rio_ErrorKind_compile, 2, -1, -1, "cannot assign"},
  {"big: [100]Int\nc := big\n", 0, 0, 0, rio_ErrorKind_compile, 2, -1, -1, "this copies a big array (over 16 words)"},
  {"B :: struct\n  xs: [20]Int\nend\nb: B\nc := b\n", 0, 0, 0, rio_ErrorKind_compile, 5, -1, -1, "this copies a big struct (over 16 words)"},
  {"f :: proc(g: [17]Int)\nend\n", 0, 0, 0, rio_ErrorKind_compile, 1, -1, -1, "a big value (over 16 words) goes by reference"},
  {"f :: proc() -> [17]Int\nend\n", 0, 0, 0, rio_ErrorKind_compile, 1, -1, -1, "a big value (over 16 words) can't be returned"},
  {"buf: [4]Int\nf :: proc(o: [..]Int)\nend\nf(buf)\n", 0, 0, 0, rio_ErrorKind_compile, 4, -1, -1, "a builder needs somewhere to keep its length"},
  {"t: Blob[..] = \"abc\"\n", 0, 0, 0, rio_ErrorKind_compile, 1, -1, -1, "can't build on read-only data"},
  {"fs: [4]Float\nb: [..]Int = fs\n", 0, 0, 0, rio_ErrorKind_compile, 2, -1, -1, "type mismatch"},
  {"buf: [4]Int\nb: [..]Int = buf\nlog(b[0])\n", 0, 0, 0, rio_ErrorKind_runtime, 3, 0, 0, "index out of bounds"},
  /* views of a proc's own memory: the next call rewrites it, so they can't be returned or kept */
  {"f :: proc() -> []Int\n  a: [4]Int\n  return a[:]\nend\n", 0, 0, 0, rio_ErrorKind_compile, 3, -1, -1, "this returns a view of the proc's own memory"},
  {"f :: proc() -> []Int\n  a: [4]Int\n  return a\nend\n", 0, 0, 0, rio_ErrorKind_compile, 3, -1, -1, "this returns a view of the proc's own memory"},
  {"f :: proc(id: Int) -> String\n  buf: Blob[16]\n  b: Blob[..] = buf\n  b.print(\"p \", id)\n  return b[:].asString()\nend\n", 0, 0, 0, rio_ErrorKind_compile, 5, -1, -1, "this returns a view of the proc's own memory"},
  {"f :: proc() -> [..]Int\n  xs: [..4]Int\n  return xs\nend\n", 0, 0, 0, rio_ErrorKind_compile, 3, -1, -1, "this returns a view of the proc's own memory"},
  {"ga: [4]Int\nf :: proc() -> [..]Int\n  b: [..]Int = ga\n  return b\nend\n", 0, 0, 0, rio_ErrorKind_compile, 4, -1, -1, "this returns a view of the proc's own memory"}, /* its length lives here */
  {"g: []Int\nf :: proc()\n  a: [4]Int\n  g = a[:]\nend\n", 0, 0, 0, rio_ErrorKind_compile, 4, -1, -1, "this view of the proc's own memory would outlive the call"},
  {"g: []Int\nf :: proc()\n  a: [4]Int\n  s := a[1:3]\n  t := s\n  g = t\nend\n", 0, 0, 0, rio_ErrorKind_compile, 6, -1, -1, "this view of the proc's own memory would outlive the call"},
  {"f :: proc(out: &[]Int)\n  a: [4]Int\n  out = a[:]\nend\n", 0, 0, 0, rio_ErrorKind_compile, 3, -1, -1, "this view of the proc's own memory would outlive the call"},
  /* views of globals and parameters, and local views passed down or replaced, are fine */
  {"g: []Int\nga: [4]Int\nf :: proc(v: []Int) -> []Int\n  a: [4]Int\n  s := a[:]\n  n := s.len()\n  s = ga[:]\n  g = s\n  return v[1:]\nend\nsum :: proc(v: []Int) -> Int\n  return v.len()\nend\nh :: proc() -> Int\n  a: [4]Int\n  return sum(a[:])\nend\nlog(f(ga).len(), h())\n", 0, 0, 0, rio_ErrorKind_none, 0, 0, 0, ""},
  /* the logical operators are words: the C forms point at themselves */
  {"a := true\nb := a && false\n", 0, 0, 0, rio_ErrorKind_compile, 2, 8, 2, "write and instead of &&"},
  {"a := true\nb := a || false\n", 0, 0, 0, rio_ErrorKind_compile, 2, 8, 2, "write or instead of ||"},
  {"a := true\nb := !a\n", 0, 0, 0, rio_ErrorKind_compile, 2, 6, 1, "write not instead of !"},
  {"x := 3\nb := not x\n", 0, 0, 0, rio_ErrorKind_compile, 2, -1, -1, "bad operand"},
  /* number literals: the error points at the whole number */
  {"x := 1_000_\n", 0, 0, 0, rio_ErrorKind_compile, 1, 6, 6, "_ goes between digits"},
  {"x := 1__0\n", 0, 0, 0, rio_ErrorKind_compile, 1, 6, 4, "_ goes between digits"},
  {"x := 0x\n", 0, 0, 0, rio_ErrorKind_compile, 1, 6, 2, "hex digits expected after 0x"},
  {"x := 0b102\n", 0, 0, 0, rio_ErrorKind_compile, 1, 6, 5, "binary digits are 0 and 1"},
  {"x := 0xFG\n", 0, 0, 0, rio_ErrorKind_compile, 1, 6, 4, "bad number"},
  {"x := 12px\n", 0, 0, 0, rio_ErrorKind_compile, 1, 6, 4, "bad number"},
  {"x := 2e\n", 0, 0, 0, rio_ErrorKind_compile, 1, 6, 2, "exponent digits expected"},
  {"x := 99_999_999_999\n", 0, 0, 0, rio_ErrorKind_compile, 1, 6, 14, "number too big for Int"},
  {"x := 0x1_0000_0000\n", 0, 0, 0, rio_ErrorKind_compile, 1, 6, 13, "number too big for Int"},
  {"x := 2147483648\n", 0, 0, 0, rio_ErrorKind_compile, 1, 6, 10, "the most is 2147483647"},
  {"x := 4_294_967_295\n", 0, 0, 0, rio_ErrorKind_compile, 1, 6, 13, "the most is 2147483647"},
  /* comments start with #: a C-style comment is an error that points at itself */
  {"x := 1 // note\n", 0, 0, 0, rio_ErrorKind_compile, 1, 8, 2, "comments start with #"},
  {"/* note */\nx := 1\n", 0, 0, 0, rio_ErrorKind_compile, 1, 1, 2, "comments start with #"},
  /* escapes: only \n \t \r \0 \\ \" \' (the backslash and its letter are the error) */
  {"s := \"ok\\n\"\nt := \"a\\qb\"\n", 0, 0, 0, rio_ErrorKind_compile, 2, 8, 2, "unknown escape"},
  {"c := '\\x'\n", 0, 0, 0, rio_ErrorKind_compile, 1, 6, -1, "unknown escape"},
  /* a proc with a result returns on every path: the error is at its end */
  {"f :: proc(n: Int) -> Int\n  if n > 0\n    return 1\n  end\nend\n", 0, 0, 0, rio_ErrorKind_compile, 5, 1, 3, "missing return"},
  {"f :: proc() -> Int\n  for\n    break\n  end\nend\n", 0, 0, 0, rio_ErrorKind_compile, 5, 1, 3, "missing return"},
  {"#!/usr/bin/env rio\n# a script\nx := 1 # note\nlog(x, \"a # b\", '#') #\n", 0, 0, 0, rio_ErrorKind_none, 0, 0, 0, ""},
  /* enums */
  {"E :: enum north, east\n", 0, 0, 0, rio_ErrorKind_compile, 1, 11, -1, "list an enum's values on the lines below it"},
  {"E :: enum\n  a = 1\n  b\nend\n", 0, 0, 0, rio_ErrorKind_compile, 4, -1, -1, "an enum needs a value that's 0"},
  {"E :: enum\n  a, b = 0\nend\n", 0, 0, 0, rio_ErrorKind_compile, 2, -1, -1, "values increase down the list"},
  {"E :: enum\n  a, b = 5\n  c = 3\nend\n", 0, 0, 0, rio_ErrorKind_compile, 3, -1, -1, "values increase down the list"},
  {"E :: enum\n  low = -1, mid, high\nend\nlog(E.mid.toInt())\n", 0, 0, 0, rio_ErrorKind_none, 0, 0, 0, ""},
  {"E :: enum\n  a\n  nil\nend\n", 0, 0, 0, rio_ErrorKind_compile, 3, -1, -1, "nil can only come first"},
  {"Dir :: enum\n  north, east\nend\nd: Dir\nd = 1\n", 0, 0, 0, rio_ErrorKind_compile, 5, -1, -1, "type mismatch"},
  {"Dir :: enum\n  north, east\nend\nd: Dir\nx := .north\n", 0, 0, 0, rio_ErrorKind_compile, 5, -1, -1, "can't tell which enum this is"},
  {"Dir :: enum\n  north, east\nend\nd: Dir\nlog(d < .east)\n", 0, 0, 0, rio_ErrorKind_compile, 5, -1, -1, "enums compare only with == and !="},
  {"Dir :: enum\n  north, east\nend\nd: Dir\nx := Dir.fromInt(9)\n", 0, 0, 0, rio_ErrorKind_compile, 5, -1, -1, "not a value of this enum"},
  {"Dir :: enum\n  north, east\nend\nd: Dir\nk := 7\nx := Dir.fromInt(k)\n", 0, 0, 0, rio_ErrorKind_runtime, 6, 0, 0, "not a value of that enum"},
  /* switch */
  {"Dir :: enum\n  north, east\nend\nd: Dir\nswitch d\ncase .north\nend\n", 0, 0, 0, rio_ErrorKind_compile, 7, -1, -1, "this switch has no case for .east"},
  {"Dir :: enum\n  north, east\nend\nd: Dir\nswitch d\ncase .north\ncase .north, .east\nend\n", 0, 0, 0, rio_ErrorKind_compile, 7, -1, -1, "this value already has a case"},
  {"Dir :: enum\n  north, east\nend\nd: Dir\nswitch d\nlog(1)\nend\n", 0, 0, 0, rio_ErrorKind_compile, 6, -1, -1, "a switch's body starts with case"},
  {"n := 1\nswitch n\ncase n\nend\n", 0, 0, 0, rio_ErrorKind_compile, 3, -1, -1, "case values are constants"},
  {"n := 1\nswitch n\nelse\ncase 1\nend\n", 0, 0, 0, rio_ErrorKind_compile, 4, -1, -1, "the else of a switch comes last"},
  {"case 1\n", 0, 0, 0, rio_ErrorKind_compile, 1, -1, -1, "'case' outside an if or a switch"},
  {"n := 1\nif n == 1\nelse if n == 2\nend\n", 0, 0, 0, rio_ErrorKind_compile, 3, -1, -1, "write case for another condition"},
  {"n := 1\nif n == 1\nelse\ncase n == 2\nend\n", 0, 0, 0, rio_ErrorKind_compile, 4, -1, -1, "the else comes last"},
  {"switch\nn := 1\nend\n", 0, 0, 0, rio_ErrorKind_compile, 2, -1, -1, "a switch's body starts with case"},
  {"n := 1\nswitch\ncase n\nend\n", 0, 0, 0, rio_ErrorKind_compile, 3, -1, -1, "condition must be Bool"},
  /* unions */
  {"A :: struct\n  x: Int\nend\nV :: union\n  A\n  A\nend\n", 0, 0, 0, rio_ErrorKind_compile, 6, -1, -1, "type listed twice"},
  {"A :: struct\n  x: Int\nend\nBig :: struct\n  d: [20]Int\nend\nV :: union\n  A, Big\nend\n", 0, 0, 0, rio_ErrorKind_compile, 9, -1, -1, "differ in size by more than 64 bytes"},
  {"A :: struct\n  x: Int\nend\nBig :: struct\n  d: [20]Int\nend\nV :: union @diverse\n  A, Big\nend\nv: V\nlog(v is A)\n", 0, 0, 0, rio_ErrorKind_none, 0, 0, 0, ""},
  {"A :: struct\n  x: Int\nend\nB :: struct\n  y: Float\nend\nU :: union\n  A, B\nend\nu: U\nu = 5\n", 0, 0, 0, rio_ErrorKind_compile, 11, -1, -1, "type mismatch"},
  {"A :: struct\n  x: Int\nend\nB :: struct\n  y: Float\nend\nU :: union\n  A, B\nend\nu: U\nu = nil\n", 0, 0, 0, rio_ErrorKind_compile, 11, -1, -1, "this union doesn't list nil"},
  {"A :: struct\n  x: Int\nend\nB :: struct\n  y: Float\nend\nU :: union\n  A, B\nend\nu: U\nlog(u is Int)\n", 0, 0, 0, rio_ErrorKind_compile, 11, -1, -1, "not one of this union's types"},
  {"A :: struct\n  x: Int\nend\nB :: struct\n  y: Float\nend\nU :: union\n  A, B\nend\nu: U\nlog(u.x)\n", 0, 0, 0, rio_ErrorKind_compile, 11, -1, -1, "a union's value is reachable inside switch"},
  {"A :: struct\n  x: Int\nend\nB :: struct\n  y: Float\nend\nU :: union\n  A, B\nend\nu: U\nswitch u\ncase A\n  u.x = 3\ncase B\nend\n", 0, 0, 0, rio_ErrorKind_compile, 13, -1, -1, "assign the whole union to change it"},
  {"A :: struct\n  x: Int\nend\nB :: struct\n  y: Float\nend\nU :: union\n  A, B\nend\nu: U\nswitch u\ncase A\nend\n", 0, 0, 0, rio_ErrorKind_compile, 13, -1, -1, "this switch has no case for B"},
  {"S :: union\n  String, Int\nend\nf :: proc()\n  s: S\nend\n", 0, 0, 0, rio_ErrorKind_compile, 5, -1, -1, "unions holding slices or strings must be global"},
  /* only x is T parts joined by && narrow: not under ||, !, or a comparison */
  {"A :: struct\n  x: Int\nend\nB :: struct\n  y: Float\nend\nU :: union\n  A, B\nend\nu: U\nt := true\nif t or u is A\n  log(u.x)\nend\n", 0, 0, 0, rio_ErrorKind_compile, 13, -1, -1, "a union's value is reachable"},
  {"A :: struct\n  x: Int\nend\nB :: struct\n  y: Float\nend\nU :: union\n  A, B\nend\nu: U\nt := true\nif not (u is B)\n  log(u.x)\nend\n", 0, 0, 0, rio_ErrorKind_compile, 13, -1, -1, "a union's value is reachable"},
  {"A :: struct\n  x: Int\nend\nB :: struct\n  y: Float\nend\nU :: union\n  A, B\nend\nu: U\nt := true\nif (u is A) == true\n  log(u.x)\nend\n", 0, 0, 0, rio_ErrorKind_compile, 13, -1, -1, "a union's value is reachable"},
  {"A :: struct\n  x: Int\nend\nB :: struct\n  y: Float\nend\nU :: union\n  A, B\nend\nu: U\nt := true\nif (u is A and t) or t\n  log(u.x)\nend\n", 0, 0, 0, rio_ErrorKind_compile, 13, -1, -1, "a union's value is reachable"},
  /* [Enum]T */
  {"Dir :: enum\n  north, east\nend\nTile :: enum\n  nil\n  wall = 3\nend\nc: [Dir]Int\nt: [Tile]Int\n", 0, 0, 0, rio_ErrorKind_compile, 9, -1, -1, "an enum used as an index needs the values 0, 1, 2"},
  {"L :: enum\n  a = -1, b, c\nend\nx: [L]Int\n", 0, 0, 0, rio_ErrorKind_compile, 4, -1, -1, "an enum used as an index needs the values 0, 1, 2"},
  {"Dir :: enum\n  north, east\nend\nTile :: enum\n  nil\n  wall = 3\nend\nc: [Dir]Int\nlog(c[0])\n", 0, 0, 0, rio_ErrorKind_compile, 9, -1, -1, "index this array with its enum"},
  {"Dir :: enum\n  north, east\nend\nTile :: enum\n  nil\n  wall = 3\nend\nc: [Dir]Int\nlog(c[Tile.wall])\n", 0, 0, 0, rio_ErrorKind_compile, 9, -1, -1, "index this array with its enum"},
  {"Dir :: enum\n  north, east\nend\nTile :: enum\n  nil\n  wall = 3\nend\nc: [Dir]Int\np: [2]Int\np = c\n", 0, 0, 0, rio_ErrorKind_compile, 10, -1, -1, "type mismatch"},
  {"Dir :: enum\n  north, east\nend\nTile :: enum\n  nil\n  wall = 3\nend\nc: [Dir]Int\ns := c[1:2]\n", 0, 0, 0, rio_ErrorKind_compile, 9, -1, -1, "slice this array with its enum's values"},
  {"Dir :: enum\n  north, east\nend\nTile :: enum\n  nil\n  wall = 3\nend\nc: [Dir]Int\ns := c[0:]\n", 0, 0, 0, rio_ErrorKind_compile, 9, -1, -1, "slice this array with its enum's values"},
  {"Dir :: enum\n  north, east\nend\nTile :: enum\n  nil\n  wall = 3\nend\nc: [Dir]Int\ns := c[.north:.east]\nlog(s.len())\n", 0, 0, 0, rio_ErrorKind_none, 0, 0, 0, ""},
  {"Dir :: enum\n  north, east\nend\nTile :: enum\n  nil\n  wall = 3\nend\nc: [Dir]Int\ns := c[:]\nlog(s.len())\n", 0, 0, 0, rio_ErrorKind_none, 0, 0, 0, ""},
  {"x := 3\nassert(x == 3)\nassert(x < 2, \"x too big\")\n", 0, 0, 0, rio_ErrorKind_runtime, 3, 0, 0, "assertion failed: x too big"},
  {"assert(1 == 2)\n", 0, 0, 0, rio_ErrorKind_runtime, 1, 0, 0, "assertion failed"},
  {"assert(1)\n", 0, 0, 0, rio_ErrorKind_compile, 1, -1, -1, "condition must be Bool"},
  {"a: [3]Int\nlog(len(a))\n", 0, 0, 0, rio_ErrorKind_compile, 2, -1, -1, "len is a method: write x.len()"},
  {"a: [..3]Int\nlog(cap(a))\n", 0, 0, 0, rio_ErrorKind_compile, 2, -1, -1, "cap is a method: write x.cap()"},
  /* proc values: none that calls through a proc value, so calls through them can't recurse */
  {"inc :: proc(n: Int) -> Int\n  return n + 1\nend\napply :: proc(f: proc(Int) -> Int, n: Int) -> Int\n  return f(n)\nend\nw :: proc(n: Int) -> Int\n  return apply(inc, n)\nend\nlog(apply(w, 1))\n", 0, 0, 0, rio_ErrorKind_compile, 10, -1, -1, "can't be a value itself (that could recurse)"},
  {"inc :: proc(n: Int) -> Int\n  return n + 1\nend\napply :: proc(f: proc(Int) -> Int, n: Int) -> Int\n  return f(n)\nend\ng: proc(Int) -> Int\nf :: proc(n: Int) -> Int\n  return g(n)\nend\ng = f\n", 0, 0, 0, rio_ErrorKind_compile, 11, -1, -1, "can't be a value itself (that could recurse)"},
  {"inc :: proc(n: Int) -> Int\n  return n + 1\nend\napply :: proc(f: proc(Int) -> Int, n: Int) -> Int\n  return f(n)\nend\ntwo :: proc(a: Int, b: Int) -> Int\n  return a\nend\nlog(apply(two, 1))\n", 0, 0, 0, rio_ErrorKind_compile, 10, -1, -1, "this proc's parameters or result don't match"},
  {"inc :: proc(n: Int) -> Int\n  return n + 1\nend\napply :: proc(f: proc(Int) -> Int, n: Int) -> Int\n  return f(n)\nend\nr :: proc(n: &Int) -> Int\n  return n\nend\nx := r\n", 0, 0, 0, rio_ErrorKind_compile, 10, -1, -1, "a proc with & parameters can't be a value"},
  {"x: proc(proc(Int)) -> Int\n", 0, 0, 0, rio_ErrorKind_compile, 1, -1, -1, "a proc type can't take or return procs"},
  {"inc :: proc(n: Int) -> Int\n  return n + 1\nend\napply :: proc(f: proc(Int) -> Int, n: Int) -> Int\n  return f(n)\nend\nf: proc(Int) -> Int\nlog(f(1))\n", 0, 0, 0, rio_ErrorKind_runtime, 8, 0, 0, "this proc value isn't set"},
  {"x := 1\nelse\n", 0, 0, 0, rio_ErrorKind_compile, 2, 1, 4, "'else' without 'if'"},
  {"x := 1\nend\n", 0, 0, 0, rio_ErrorKind_compile, 2, 1, 3, "'end' without block"},
  {"x := 1\nreturn\n", 0, 0, 0, rio_ErrorKind_compile, 2, 1, 6, "return outside proc"},
  {"xs: [..2]Int\nxs.print(1)\n",0, 0, 0, rio_ErrorKind_compile, 2, 9, 1, "print needs a Blob list"},
  {"b: Blob[..8]\nok := b.format(1, 2)\n", 0, 0, 0, rio_ErrorKind_compile, 2, -1, -1, "format needs its settings first: b.format({width = 4}, ...), or b.print(...)"},
  {"b: Blob[..8]\nok := b.format()\n", 0, 0, 0, rio_ErrorKind_compile, 2, -1, -1, "format needs its settings first: b.format({width = 4}, ...), or b.print(...)"},
  /* packing */
  {"b: Blob[8]\nn := 0\nb.unpack(0, n)\n", 0, 0, 0, rio_ErrorKind_compile, 3, -1, -1, "unpack changes what it's given: pass &x (or a slice, whose elements it fills)"},
  {"S :: struct\n  name: String\nend\ns: S\nb: Blob[16]\nb.pack(0, s)\n", 0, 0, 0, rio_ErrorKind_compile, 6, -1, -1, "only plain data packs: no views or proc values in it"},
  {"b: Blob[8]\nn := b.getInt(0, 12)\n", 0, 0, 0, rio_ErrorKind_compile, 2, -1, -1, "the bits are 8, 16, 24 or 32 (negative: signed)"},
  {"b: Blob[8]\nb.pack(1)\n", 0, 0, 0, rio_ErrorKind_compile, 2, -1, -1, "pack(value) appends to a byte list: a Blob takes pack(at, value)"},
  {"\"abcd\".setInt(0, 8, 1)\n", 0, 0, 0, rio_ErrorKind_compile, 1, -1, -1, "a String is read-only: pack and setInt need a Blob"},
  {"xs: [4]Int\nn := xs.getInt(0, 8)\n", 0, 0, 0, rio_ErrorKind_compile, 2, -1, -1, "no such method"},
  {"b: Blob[8]\nn := 5\nb.pack(n, n)\n", 0, 0, 0, rio_ErrorKind_runtime, 3, 0, 0, "pack out of bounds"},
  {"b: Blob[8]\nn := -1\nb.setInt(n, 8, 1)\n", 0, 0, 0, rio_ErrorKind_runtime, 3, 0, 0, "setInt out of bounds"},
  {"b: Blob[8]\nw := 12\nlog(b.getInt(0, w))\n", 0, 0, 0, rio_ErrorKind_runtime, 3, 0, 0, "getInt takes 8, 16, 24 or 32 bits"},
  {"b: Blob[..8]\nb.push(1)\nn := 0\nb.unpack(0, &n)\n", 0, 0, 0, rio_ErrorKind_runtime, 4, 0, 0, "unpack out of bounds"},
  {"b: Blob[..8]\nok := b.format({wide = 4}, 2)\n", 0, 0, 0, rio_ErrorKind_compile, 2, -1, -1, "no such field"},
  {"S :: struct\n  x: Int\nend\nS.f :: proc()\n  self.f()\nend\n", 0, 0, 0, rio_ErrorKind_compile, 5, 8, 1, "recursion is not allowed"},
  {"S :: struct\n  x: Int\nend\nS.x :: proc()\nend\n", 0, 0, 0, rio_ErrorKind_compile, 4, 3, 1, "same name as a field"},
  {"S :: struct\n  x: Int\nend\nS.f :: proc()\nend\nS.f :: proc()\nend\n", 0, 0, 0, rio_ErrorKind_compile, 6, 3, 1, "method already defined"},
  {"S :: struct\n  x: Int\nend\nS.f :: proc()\nend\ns: S\ng := s.f\n", 0, 0, 0, rio_ErrorKind_compile, 7, 8, 1, "method calls need ()"},
  {"S :: struct\n  x: Int\nend\ns: S\ns.nope()\n", 0, 0, 0, rio_ErrorKind_compile, 5, 3, 4, "no such field or method"},
  {"xs: [..2]Int\npush(xs, 1)\n", 0, 0, 0, rio_ErrorKind_compile, 2, 1, 4, "undefined name"},
  {"V :: struct\n  x: Float\nend\nv := {x = 1}\n", 0, 0, 0, rio_ErrorKind_compile, 4, 6, 1, "can't tell which struct"},
  {"V :: struct\n  x: Float\nend\nlog({x = 1})\n", 0, 0, 0, rio_ErrorKind_compile, 4, 5, 1, "can't tell which struct"},
  {"V :: struct\n  x: Float\nend\nn: Int = {x = 1}\n", 0, 0, 0, rio_ErrorKind_compile, 4, 10, 1, "can't tell which struct"},
  {"V :: struct\n  x, y: Int\nend\nxs: [2]Int\nv := V{xs[0]}\n", 0, 0, 0, rio_ErrorKind_compile, 5, 13, 1, "only a name or a.b path"},
  {"V :: struct\n  x, y: Int\nend\nz := 1\nv := V{z}\n", 0, 0, 0, rio_ErrorKind_compile, 5, 9, 1, "no such field"},
  {"V :: struct\n  x, y: Int\nend\nx := 1\nv := V{x, x = 2}\n", 0, 0, 0, rio_ErrorKind_compile, 5, 11, 1, "field set twice"},
  {"V :: struct\n  x, y: Int\nend\nv: V\n{x} = v\n", 0, 0, 0, rio_ErrorKind_compile, 5, 5, 1, "':=' expected"},
  {"V :: struct\n  x, y: Int\nend\nv: V\n{x as} := v\n", 0, 0, 0, rio_ErrorKind_compile, 5, 6, 1, "name expected after 'as'"},
  {"{x} := 5\n", 0, 0, 0, rio_ErrorKind_compile, 1, 8, 1, "only a struct can be destructured"},
  {"V :: struct\n  x, y: Int\nend\nv: V\n{x, z} := v\n", 0, 0, 0, rio_ErrorKind_compile, 5, 11, 1, "no such field to destructure"},
  {"x := Float(3)\n", 0, 0, 0, rio_ErrorKind_compile, 1, 11, 1, "convert with x.toInt()"},
  {"s := \"a\".toFloat()\n", 0, 0, 0, rio_ErrorKind_compile, 1, 18, 1, "no such conversion"},
  {"Int.toFloat :: proc() -> Float\n  return 0\nend\n", 0, 0, 0, rio_ErrorKind_compile, 1, 5, 7, "that conversion is built in"},
  /* ...or at the last token of an expression that can only be judged once it's complete */
  {"x := 1\n  y := 2.0 + \"s\"\n", 0, 0, 0, rio_ErrorKind_compile, 2, 14, 3, "type mismatch"},
  {"x := (1 + 2\n", 0, 0, 0, rio_ErrorKind_compile, 1, 11, 1, "unclosed bracket"},
  {"if true\n  x := 1\n", 0, 0, 0, rio_ErrorKind_compile, 2, 8, 1, "missing 'end'"},
  {"log(\"abc)\n", 0, 0, 0, rio_ErrorKind_compile, -1, -1, -1, "unclosed bracket"}, /* (a closing quote left out: the string ends with its line) */
  /* runtime errors report the line */
  {"xs: [4]Int\ni := 2\nf :: proc(n: Int) -> Int\n  return xs[n * 3]\nend\nlog(f(i))\n", 0, 0, 0, rio_ErrorKind_runtime, 4, 0, 0, "index out of bounds"},
  {"xs: [..2]Int\nfor j in 0..<3\n  xs.push(j)\nend\nlog(xs.pop(), xs.pop(), xs.pop())\n", 0, 0, 0, rio_ErrorKind_runtime, 5, 0, 0, "pop from empty list"},
  {"main :: proc()\n  a := 0\n  b := 5\n  log(b / a)\nend\n", 0, 0, 0, rio_ErrorKind_runtime, 4, 0, 0, "division by zero"},
  {"a: [3]Int\nn := 5\nb := a[1:n]\n", 0, 0, 0, rio_ErrorKind_runtime, 3, 0, 0, "slice out of bounds"},
  {"xs: [2]Int\ni := 0\nfor xs[i] == 0\n  i += 1\nend\n", 0, 0, 0, rio_ErrorKind_runtime, 3, 0, 0, "index out of bounds"}, /* rotated loop condition */
  {"a: [3]Int\na[0] = 1\na[2] = 4\nb: [3]Int\nb[0] = 1\nb[2] = 1\nc := a / b\n", 0, 0, 0, rio_ErrorKind_runtime, 7, 0, 0, "division by zero"},
  /* a view of a block's array outlives the block: those slots must not be reused for t, or writing
     through s would forge t's address and length */
  {"s: []Int\nif true\n  a: [2]Int\n  s = a[:]\nend\nif true\n  t: []Int = s\n  s[1] = 2000000000\n  s[0] = 4\n  log(t[100000000])\nend\n", 0, 0, 0, rio_ErrorKind_runtime, 10, 0, 0, "index out of bounds"},
  {"keep :: proc(v: []Int) -> []Int\n  return v\nend\nf :: proc() -> Int\n  s: []Int\n  if true\n    a: [2]Int\n    s = keep(a[:])\n  end\n  if true\n    t: []Int = s\n    s[1] = 2000000000\n    return t[100000000]\n  end\n  return 0\nend\nlog(f())\n", 0, 0, 0, rio_ErrorKind_runtime, 13, 0, 0, "index out of bounds"},
  {"main :: proc()\n  again()\nend\n", 0, 0, 0, rio_ErrorKind_runtime, 2, 0, 0, "rio is already running"},
  {"main :: proc()\n  log(\"x\")\n  boom()\nend\n", 0, 0, 0, rio_ErrorKind_runtime, 3, 0, 0, "the host said no"},
  /* errors with no source position */
  {"N :: 10\n", "N", "2.5", 0, rio_ErrorKind_compile, 0, 0, 0, "-D N: value doesn't fit Int"},
  {"N :: 10\n", "9x", "1", 0, rio_ErrorKind_compile, 0, 0, 0, "-D: bad name 9x"},
  {"x := 1\n", 0, 0, 4096, rio_ErrorKind_compile, 0, 0, 0, "not enough memory for the compiler"},
  /* success clears the previous error */
  {"x := 1\n", 0, 0, 0, rio_ErrorKind_none, 0, 0, 0, ""},
};

static void boom(rio_Vm *v, rio_Val *a) { (void)a; rio_trap(v, "the host said no"); }
/* calls back into rio, which is refused: procs that never run at the same time share memory */
static void again(rio_Vm *v, rio_Val *a) {
  char m[128]; const char *e; size_t i;
  (void)a;
  if (rio_call(v, rio_findProc(v, "main")) == -1) {
    for (e = rio_error(v), i = 0; e[i] && i < sizeof m - 1; i++) m[i] = e[i];
    m[i] = 0; rio_trap(v, m);
  }
}

int main(void) {
  int i, fails = 0, n = (int)(sizeof cases / sizeof cases[0]), pass;
  for (pass = 0; pass < 2; pass++) /* with a code buffer, then with the code in mem */
  for (i = 0; i < n; i++) {
    const Case *c = &cases[i]; rio_Error e; int f;
    rio_init(&vm, mem, c->memsize ? c->memsize : sizeof mem, pass ? 0 : code, 4096);
    rio_addFfi(&vm, "boom", "", boom);
    rio_addFfi(&vm, "again", "", again);
    if (c->def) rio_define(&vm, c->def, c->defval);
    if (!rio_compile(&vm, c->src, (uint32_t)strlen(c->src)) && !rio_run(&vm)) {
      if ((f = rio_findProc(&vm, "main")) >= 0) rio_call(&vm, f);
    }
    e = rio_errorInfo(&vm);
    if (e.kind != c->kind || (c->line >= 0 && e.line != c->line) || (c->col >= 0 && e.col != c->col) ||
        (c->len >= 0 && e.len != c->len) || !strstr(e.msg, c->msg)) {
      printf("FAIL case %d%s: got kind %d %d:%d len %d \"%s\"; want kind %d %d:%d len %d \"%s\"\n",
             i, pass ? " (code in mem)" : "", e.kind, e.line, e.col, e.len, e.msg, c->kind, c->line, c->col, c->len, c->msg);
      fails++;
    }
  }
  n *= 2;
  { /* a line of over 4096 instructions: the packed line table takes long steps; the error is on line 4 */
    static char big[40000]; static rio_Instr bigcode[8192]; int k, len = sprintf(big, "xs: [2]Int\nx := 1\ni := 5\ny := x");
    rio_Error e;
    for (k = 0; k < 5000; k++) len += sprintf(big + len, " + x");
    len += sprintf(big + len, " + xs[i]\n");
    rio_init(&vm, mem, sizeof mem, bigcode, 8192);
    n++;
    if (rio_compile(&vm, big, (uint32_t)len)) { printf("FAIL long line: %s\n", rio_error(&vm)); fails++; }
    else {
      rio_run(&vm); e = rio_errorInfo(&vm);
      if (e.kind != rio_ErrorKind_runtime || e.line != 4) { printf("FAIL long line: kind %d line %d \"%s\"\n", e.kind, e.line, e.msg); fails++; }
    }
  }
  { /* explicit limits: tiny tables fail cleanly, and a scratch buffer too small for them is refused */
    static unsigned char scratch[1 << 16];
    static const char *many = "a := 1\nb := 2\nc := 3\nd := 4\ne := 5\nf := 6\ng := 7\nh := 8\n";
    rio_Limits l = rio_limitsFor(sizeof scratch);
    rio_Error e;
    l.symbols = 36; /* the builtins take 31, so the 6th global is one too many */
    rio_init(&vm, mem, sizeof mem, code, 4096);
    if (!rio_compileWith(&vm, many, (uint32_t)strlen(many), scratch, sizeof scratch, &l) ||
        !strstr((e = rio_errorInfo(&vm)).msg, "too many symbols") || e.line != 6) {
      printf("FAIL explicit limits: \"%s\"\n", rio_error(&vm)); fails++; n++;
    } else n++;
    l = rio_limitsFor(1 << 20);
    rio_init(&vm, mem, sizeof mem, code, 4096);
    if (!rio_compileWith(&vm, many, (uint32_t)strlen(many), scratch, sizeof scratch, &l) ||
        !strstr(rio_error(&vm), "scratch too small for these limits")) {
      printf("FAIL limits bigger than scratch: \"%s\"\n", rio_error(&vm)); fails++; n++;
    } else n++;
  }
  { /* REPL indenting: open blocks and brackets, minus ends (a string ends with its line) */
    static const struct { const char *src; int want; } ind[] = {
      {"f :: proc()\n", 1}, {"f :: proc()\n  if x\n", 2}, {"f :: proc()\n  if x\n  end\n", 1},
      {"V :: struct\n  x: Int\n", 1}, {"switch s\ncase A\n", 1}, {"if a\nelse\n", 1},
      {"x := f(1,\n", 1}, {"s := (\"a(bc\n", 1}, {"s := \"a)b(\"\nif (x\n", 2}, {"# if x\nif y # (\n", 1},
      {"outer: for i in 0..<3\n", 1}, {"for x in xs\n  for y in ys\n", 2}, {"T.m :: proc()\n", 1}, {"end\n", 0}};
    size_t i;
    for (i = 0; i < sizeof ind / sizeof ind[0]; i++, n++) {
      int got = rio_replIndent(ind[i].src, (uint32_t)strlen(ind[i].src));
      if (got != ind[i].want) { printf("FAIL indent %d (want %d): %s\n", got, ind[i].want, ind[i].src); fails++; }
    }
    n++;
    if (!rio_replOutdent("end", 3) || !rio_replOutdent("else", 4) || !rio_replOutdent("case", 4) || rio_replOutdent("if", 2)) {
      printf("FAIL outdent\n"); fails++;
    }
  }
  if (!fails) printf("all %d error tests passed\n", n);
  return fails != 0;
}
