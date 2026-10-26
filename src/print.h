/* print.h - values as text, by their types' descriptors: log and format for structs, arrays, slices,
   builders, builder views and unions (docs/spec/builtins.md#log). The compiler writes a descriptor of each type
   printed into the constants, the first time it's printed (rio_descriptor, builtins.c), and one
   instruction prints a value through it (logv, pushv), so a struct takes one instruction, not one a
   field. The printer is written once, here, as a macro: vm.c compiles it, and aot.c puts its text in
   the C it writes (so it uses no names but its own, memcpy and rio_formatFloat, and only these
   comments).

   A descriptor is constant words, starting with its kind (a rio_Print_ value, + rio_Print_text when its
   elements are bytes, printed as text), then by kind:
     int, byte, float, bool, proc   nothing more
     enum                           its names' first constant (values are 0 to n - 1), how many
     struct                         its name (a string constant: address, length), its field count, then
                                    each field's name, offset and descriptor
     array                          its length, the element size and descriptor
     slice, builder view            the element size and descriptor
     builder                        the element size and descriptor, and its capacity
     union                          how many types it lists, then each one's descriptor (0: nil)
     bits                           a struct's bitfield: its first bit in the word, its bits (negative:
                                    signed)
     range                          nothing more (a Range: first..=last)
   Constants are slots: a descriptor names its strings and other descriptors by slot number. */
#ifndef rio_PRINT_H
#define rio_PRINT_H

#define rio_PRINT_KINDS                                                                                  \
  enum { rio_Print_int = 1, rio_Print_byte, rio_Print_float, rio_Print_bool, rio_Print_proc, rio_Print_enum, \
         rio_Print_struct, rio_Print_array, rio_Print_slice, rio_Print_builder, rio_Print_builderView, rio_Print_union, rio_Print_bits, rio_Print_range, \
         rio_Print_text = 256, rio_Print_shown = 10, rio_Print_textShown = 40, rio_Print_depth = 8 };

/* rio_Out: where the text goes (out, from len, up to cap); capped: a log line's, so long sequences show
   their first rio_Print_shown elements and long nested text its first rio_Print_textShown bytes; full:
   it ran out of room (the rest is dropped); spec: format's settings' address, or 0. The value is at an address in M (structs, arrays, builders,
   unions), or is the words w0, w1, w2 themselves (a slice: address, length; a builder view: the address of
   its length, of its data, its capacity) */
#define rio_PRINTER(X) X(                                                                                \
typedef struct { uint8_t *out; uint32_t len, cap, spec; int capped, full; } rio_Out;                           \
static uint32_t rio_printWord(const uint8_t *M, uint32_t p) { uint32_t v; memcpy(&v, M + p, 4); return v; } \
static void rio_outBytes(rio_Out *o, const void *s, uint32_t n) {                                         \
  if (n > o->cap - o->len) { n = o->cap - o->len; o->full = 1; }                                         \
  memcpy(o->out + o->len, s, n);                                                                         \
  o->len += n;                                                                                           \
}                                                                                                        \
static void rio_outInt(rio_Out *o, int32_t v) {                                                          \
  char b[12]; int n = 0; uint32_t u = v < 0 ? 0u - (uint32_t)v : (uint32_t)v;                            \
  do { b[11 - n++] = (char)('0' + u % 10); u /= 10; } while (u);                                          \
  if (v < 0) b[11 - n++] = '-';                                                                          \
  rio_outBytes(o, b + 12 - n, (uint32_t)n);                                                              \
}                                                                                                        \
/* how many bytes the UTF-8 character at s is (n left), or 0 if they don't make one */                   \
static uint32_t rio_utf8Len(const uint8_t *s, uint32_t n) {                                              \
  uint32_t k, i; uint8_t c = s[0], lo = 0x80, hi = 0xBF;                                                 \
  if (c < 0x80) return 1;                                                                                \
  k = c >= 0xC2 && c <= 0xDF ? 2 : c >= 0xE0 && c <= 0xEF ? 3 : c >= 0xF0 && c <= 0xF4 ? 4 : 0;          \
  if (!k || k > n) return 0;                                                                             \
  if (c == 0xE0) lo = 0xA0; else if (c == 0xED) hi = 0x9F; else if (c == 0xF0) lo = 0x90; else if (c == 0xF4) hi = 0x8F; \
  if (s[1] < lo || s[1] > hi) return 0;                                                                  \
  for (i = 2; i < k; i++) if (s[i] < 0x80 || s[i] > 0xBF) return 0;                                      \
  return k;                                                                                              \
}                                                                                                        \
/* text as shown, not as bytes: control bytes, 127 and bytes that aren't UTF-8 as \xHH (and in quoted    \
   text, as Rio writes it: \n \t \r \0 \" \\; else line breaks, tabs and \r stay as they are) */          \
static void rio_outShown(rio_Out *o, const uint8_t *s, uint32_t n, int quoted) {                         \
  uint32_t i = 0, k;                                                                                     \
  while (i < n && !o->full) {                                                                            \
    char e[4]; uint8_t c = s[i];                                                                         \
    e[0] = 92;                                                                                           \
    if (quoted && (c == 10 || c == 9 || c == 13 || c == 0 || c == 34 || c == 92)) {                      \
      e[1] = c == 10 ? 'n' : c == 9 ? 't' : c == 13 ? 'r' : c == 0 ? '0' : (char)c;                      \
      rio_outBytes(o, e, 2); i++;                                                                        \
    } else if (c == 10 || c == 9 || c == 13 || (c >= 32 && c < 127)) { rio_outBytes(o, &c, 1); i++; }    \
    else if (c >= 0x80 && (k = rio_utf8Len(s + i, n - i)) != 0) { rio_outBytes(o, s + i, k); i += k; }   \
    else {                                                                                               \
      e[1] = 'x'; e[2] = "0123456789abcdef"[c >> 4]; e[3] = "0123456789abcdef"[c & 15];                  \
      rio_outBytes(o, e, 4); i++;                                                                        \
    }                                                                                                    \
  }                                                                                                      \
}                                                                                                        \
/* text inside a value: quoted, escaped as Rio writes it */                                              \
static void rio_outQuoted(rio_Out *o, const uint8_t *s, uint32_t n) {                                    \
  uint32_t most = o->capped && n > rio_Print_textShown ? rio_Print_textShown : n;                         \
  rio_outBytes(o, "\"", 1);                                                                              \
  rio_outShown(o, s, most, 1);                                                                           \
  if (most < n) rio_outBytes(o, "...", 3);                                                               \
  rio_outBytes(o, "\"", 1);                                                                              \
}                                                                                                        \
static void rio_printAt(const uint8_t *M, rio_Out *o, uint32_t d, uint32_t p, int depth);                \
static void rio_outFormatted(const uint8_t *M, rio_Out *o, uint32_t spec, int kind, uint32_t w0, uint32_t w1); \
/* n elements of descriptor d's element type from address at: text, or [a, b, ...] */                    \
static void rio_printElements(const uint8_t *M, rio_Out *o, uint32_t d, uint32_t at, uint32_t n, int depth) { \
  uint32_t k = rio_printWord(M, d * 4), array = (k & 255) == rio_Print_array, i, shown;                  \
  uint32_t size = rio_printWord(M, (d + 1 + array) * 4), e = rio_printWord(M, (d + 2 + array) * 4);       \
  if (k & rio_Print_text) {                                                                              \
    if (depth) rio_outQuoted(o, M + at, n);                                                              \
    else if (o->spec) rio_outFormatted(M, o, o->spec, 3, at, n);                    \
    else if (o->capped) rio_outShown(o, M + at, n, 0); /* a log: as shown */                             \
    else rio_outBytes(o, M + at, n);                                                                     \
    return;                                                                                              \
  }                                                                                                      \
  shown = o->capped && n > rio_Print_shown ? rio_Print_shown : n;                                         \
  rio_outBytes(o, "[", 1);                                                                               \
  for (i = 0; i < shown && !o->full; i++) {                                                              \
    if (i) rio_outBytes(o, ", ", 2);                                                                     \
    rio_printAt(M, o, e, at + i * size, depth + 1);                                                      \
  }                                                                                                      \
  if (shown < n) { rio_outBytes(o, ", ... ", 6); rio_outInt(o, (int32_t)(n - shown)); rio_outBytes(o, " more", 5); } \
  rio_outBytes(o, "]", 1);                                                                               \
}                                                                                                        \
/* the value at address p, of descriptor d */                                                            \
static void rio_printAt(const uint8_t *M, rio_Out *o, uint32_t d, uint32_t p, int depth) {               \
  uint32_t k = rio_printWord(M, d * 4), n, i;                                                            \
  if (o->full) return;                                                                                   \
  if (depth > rio_Print_depth) { rio_outBytes(o, "...", 3); return; }                                    \
  switch (k & 255) {                                                                                     \
  case rio_Print_int: if (o->spec) rio_outFormatted(M, o, o->spec, 0, rio_printWord(M, p), 0); else rio_outInt(o, (int32_t)rio_printWord(M, p)); return;                               \
  case rio_Print_byte: if (o->spec) rio_outFormatted(M, o, o->spec, 0, M[p], 0); else rio_outInt(o, M[p]); return;                                                      \
  case rio_Print_range: { /* first..=last */                                                             \
    uint32_t first = rio_printWord(M, p), last = rio_printWord(M, p + 4);                                \
    if (o->spec) rio_outFormatted(M, o, o->spec, 0, first, 0); else rio_outInt(o, (int32_t)first);       \
    rio_outBytes(o, "..=", 3);                                                                           \
    if (o->spec) rio_outFormatted(M, o, o->spec, 0, last, 0); else rio_outInt(o, (int32_t)last);         \
    return;                                                                                              \
  }                                                                                                      \
  case rio_Print_bits: { /* (its bits, shifted down: a signed one's top bit spread) */                   \
    uint32_t at = rio_printWord(M, d * 4 + 4), w = rio_printWord(M, p);                                  \
    int32_t bits = (int32_t)rio_printWord(M, d * 4 + 8), width = bits < 0 ? -bits : bits;                \
    w = (w >> at) & ((1u << width) - 1);                                                                 \
    if (bits < 0 && (w >> (width - 1) & 1)) w |= ~0u << width;                                           \
    if (o->spec) rio_outFormatted(M, o, o->spec, 0, w, 0); else rio_outInt(o, (int32_t)w);              \
    return;                                                                                              \
  }                                                                                                      \
  case rio_Print_float: {                                                                                \
    char b[40]; float f; uint32_t w = rio_printWord(M, p);                                               \
    if (o->spec) { rio_outFormatted(M, o, o->spec, 1, w, 0); return; }                \
    memcpy(&f, &w, 4);                                                                                   \
    rio_outBytes(o, b, (uint32_t)rio_formatFloat(b, f));                                                 \
    return;                                                                                              \
  }                                                                                                      \
  case rio_Print_bool: if (o->spec) rio_outFormatted(M, o, o->spec, 2, rio_printWord(M, p), 0); else if (rio_printWord(M, p)) rio_outBytes(o, "true", 4); else rio_outBytes(o, "false", 5); return; \
  case rio_Print_proc: rio_outBytes(o, "proc", 4); return;                                               \
  case rio_Print_enum: {                                                                                 \
    int32_t v = (int32_t)rio_printWord(M, p);                                                            \
    uint32_t names = rio_printWord(M, (d + 1) * 4);                                                      \
    n = rio_printWord(M, (d + 2) * 4);                                                                   \
    i = (uint32_t)v; /* (values are 0 to n - 1) */                                                       \
    if (i < n && o->spec) rio_outFormatted(M, o, o->spec, 3, rio_printWord(M, (names + 2 * i) * 4), rio_printWord(M, (names + 2 * i + 1) * 4)); \
    else if (i < n) rio_outBytes(o, M + rio_printWord(M, (names + 2 * i) * 4), rio_printWord(M, (names + 2 * i + 1) * 4)); \
    else rio_outInt(o, v);                                                                               \
    return;                                                                                              \
  }                                                                                                      \
  case rio_Print_struct: {                                                                               \
    uint32_t name = rio_printWord(M, (d + 1) * 4);                                                       \
    n = rio_printWord(M, (d + 2) * 4);                                                                   \
    rio_outBytes(o, M + rio_printWord(M, name * 4), rio_printWord(M, (name + 1) * 4));                   \
    rio_outBytes(o, "{", 1);                                                                             \
    for (i = 0; i < n && !o->full; i++) {                                                                \
      uint32_t f = d + 3 + 3 * i, fieldName = rio_printWord(M, f * 4);                                   \
      if (i) rio_outBytes(o, ", ", 2);                                                                   \
      rio_outBytes(o, M + rio_printWord(M, fieldName * 4), rio_printWord(M, (fieldName + 1) * 4));       \
      rio_outBytes(o, " = ", 3);                                                                         \
      rio_printAt(M, o, rio_printWord(M, (f + 2) * 4), p + rio_printWord(M, (f + 1) * 4), depth + 1);    \
    }                                                                                                    \
    rio_outBytes(o, "}", 1);                                                                             \
    return;                                                                                              \
  }                                                                                                      \
  case rio_Print_union: {                                                                                \
    uint32_t tag = rio_printWord(M, p), member;                                                          \
    n = rio_printWord(M, (d + 1) * 4);                                                                   \
    member = tag < n ? rio_printWord(M, (d + 2 + tag) * 4) : 0;                                          \
    if (member) rio_printAt(M, o, member, p + 4, depth);                                                 \
    else rio_outBytes(o, "nil", 3);                                                                      \
    return;                                                                                              \
  }                                                                                                      \
  case rio_Print_array: rio_printElements(M, o, d, p, rio_printWord(M, (d + 1) * 4), depth); return;     \
  case rio_Print_slice: rio_printElements(M, o, d, rio_printWord(M, p), rio_printWord(M, p + 4), depth); return; \
  case rio_Print_builder: {                                                                                 \
    uint32_t cap = rio_printWord(M, (d + 3) * 4);                                                        \
    n = rio_printWord(M, p);                                                                             \
    rio_printElements(M, o, d, p + 4, n < cap ? n : cap, depth);                                         \
    return;                                                                                              \
  }                                                                                                      \
  case rio_Print_builderView: {                                                                              \
    uint32_t cap = rio_printWord(M, p + 8);                                                              \
    n = rio_printWord(M, rio_printWord(M, p));                                                           \
    rio_printElements(M, o, d, rio_printWord(M, p + 4), n < cap ? n : cap, depth);                       \
    return;                                                                                              \
  }                                                                                                      \
  }                                                                                                      \
}                                                                                                        \
/* a value, as logv and pushv have it: a slice's or builder view's words themselves, else its address */      \
static void rio_printValue(const uint8_t *M, rio_Out *o, uint32_t d, uint32_t w0, uint32_t w1, uint32_t w2) { \
  uint32_t k = rio_printWord(M, d * 4) & 255, n;                                                         \
  if (k == rio_Print_slice) rio_printElements(M, o, d, w0, w1, 0);                                        \
  else if (k == rio_Print_builderView) { n = rio_printWord(M, w0); rio_printElements(M, o, d, w1, n < w2 ? n : w2, 0); } \
  else rio_printAt(M, o, d, w0, 0);                                                                      \
}                                                                                                         \
/* a value as format's settings say (pushf; docs/spec/builtins.md#format), at the address spec: width,    \
   digits, places, base and textWidth, each 0 for as print does it (width for numbers, textWidth for      \
   the rest). kind: 0 Int (w0), 1 Float (its bits, w0),                                                   \
   2 Bool (w0), 3 text (address w0, length w1) */                                                         \
static void rio_outPad(rio_Out *o, int32_t n) { while (n-- > 0 && !o->full) rio_outBytes(o, " ", 1); }    \
static void rio_outFormatted(const uint8_t *M, rio_Out *o, uint32_t spec, int kind, uint32_t w0, uint32_t w1) { \
  int32_t width = (int32_t)rio_printWord(M, spec), digits = (int32_t)rio_printWord(M, spec + 4);          \
  int32_t places = (int32_t)rio_printWord(M, spec + 8), base = (int32_t)rio_printWord(M, spec + 12), n = 0, k = 0, at; \
  char b[96], t[72]; const uint8_t *s = (const uint8_t *)b; uint32_t len, u;                              \
  if (kind >= 2) width = (int32_t)rio_printWord(M, spec + 16); /* (text, Bools and enums: textWidth) */   \
  if (digits > 64) digits = 64;                                                                           \
  if (kind == 3) { s = M + w0; len = w1; }                                                                \
  else if (kind == 2) { len = w0 ? 4u : 5u; memcpy(b, w0 ? "true" : "false", len); }                      \
  else if (kind == 0) {                                                                                   \
    if (base < 2 || base > 16) base = 10;                                                                 \
    u = base == 10 && (int32_t)w0 < 0 ? 0u - w0 : w0;                                                     \
    if (base == 10 && (int32_t)w0 < 0) b[n++] = '-';                                                      \
    do { t[k++] = "0123456789abcdef"[u % (uint32_t)base]; u /= (uint32_t)base; } while (u);               \
    while (k < digits) t[k++] = '0';                                                                      \
    while (k) b[n++] = t[--k];                                                                            \
    len = (uint32_t)n;                                                                                    \
  } else {                                                                                                \
    float f; double v, scale = 1; memcpy(&f, &w0, 4); v = f < 0 ? -(double)f : (double)f;                 \
    if (places > 9) places = 9;                                                                           \
    for (k = 0; k < places; k++) scale *= 10;                                                             \
    if (places > 0 && f == f && v * scale < 1e18) {                                                       \
      uint64_t q = (uint64_t)(v * scale + 0.5), whole = q / (uint64_t)scale, part = q % (uint64_t)scale;  \
      if (f < 0 && q) b[n++] = '-';                                                                       \
      k = 0;                                                                                              \
      do { t[k++] = (char)('0' + whole % 10); whole /= 10; } while (whole);                               \
      while (k < digits) t[k++] = '0';                                                                    \
      while (k) b[n++] = t[--k];                                                                          \
      b[n++] = '.';                                                                                       \
      for (k = places - 1; k >= 0; k--) { b[n + k] = (char)('0' + part % 10); part /= 10; }               \
      n += places;                                                                                        \
    } else {                                                                                              \
      n = rio_formatFloat(b, f);                                                                          \
      at = b[0] == '-';                                                                                   \
      for (k = at; k < n && b[k] >= '0' && b[k] <= '9'; k++) {}                                           \
      if (k > at && k - at < digits) { /* (zeros before its whole part) */                                \
        int32_t more = digits - (k - at);                                                                 \
        memmove(b + at + more, b + at, (size_t)(n - at));                                                 \
        memset(b + at, '0', (size_t)more);                                                                \
        n += more;                                                                                        \
      }                                                                                                   \
    }                                                                                                     \
    len = (uint32_t)n;                                                                                    \
  }                                                                                                       \
  if (width > 0) rio_outPad(o, width - (int32_t)len);                                                     \
  rio_outBytes(o, s, len);                                                                                \
  if (width < 0) rio_outPad(o, -width - (int32_t)len);                                                    \
}                                                                                                         \
)

#define rio_PRINT_CODE(...) __VA_ARGS__
#define rio_PRINT_TEXT(...) #__VA_ARGS__
#define rio_PRINT_STRING(m) rio_PRINT_TEXT(m)

#endif
