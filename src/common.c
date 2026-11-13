/* common.c - what the compiler and the vm both use: number formatting, error messages, and the math
   builtins. */
#include <math.h>
#include "internal.h"

/* ------------------------------------------------------------------ math builtins */

static float rio_sqrt(float x) { return sqrtf(x); }
static float rio_sin(float x) { return sinf(x); }
static float rio_cos(float x) { return cosf(x); }
static float rio_tan(float x) { return tanf(x); }
static float rio_asin(float x) { return asinf(x); }
static float rio_acos(float x) { return acosf(x); }
static float rio_atan(float x) { return atanf(x); }
static float rio_exp(float x) { return expf(x); }
static float rio_ln(float x) { return logf(x); }
static float rio_floor(float x) { return floorf(x); }
static float rio_ceil(float x) { return ceilf(x); }
static float rio_round(float x) { return x < 0 ? -floorf(-x + 0.5f) : floorf(x + 0.5f); } /* half away from 0 */
static float rio_abs(float x) { return fabsf(x); }
static float rio_atan2(float y, float x) { return atan2f(y, x); }
static float rio_pow(float x, float y) { return powf(x, y); }
static float rio_fmod(float x, float y) { return fmodf(x, y); }

float (*const rio_math1[rio_Math1_count])(float) = {
  rio_sqrt, rio_sin, rio_cos, rio_tan, rio_asin, rio_acos, rio_atan, rio_exp, rio_ln,
  rio_floor, rio_ceil, rio_round, rio_abs,
};
float (*const rio_math2[rio_Math2_count])(float, float) = {rio_atan2, rio_pow, rio_fmod};

/* ------------------------------------------------------------------ text */

uint32_t rio_hashText(const char *s, uint32_t n) { /* (FNV-1a, 32 bits; \r skipped, so line ends don't count) */
  uint32_t h = 2166136261u, i;
  for (i = 0; i < n; i++)
    if (s[i] != '\r') h = (h ^ (uint8_t)s[i]) * 16777619u;
  return h;
}

int rio_formatInt(char *out, int32_t v) {
  char digits[12];
  int n = 0, len = 0;
  uint32_t u = v < 0 ? 0u - (uint32_t)v : (uint32_t)v;
  do {
    digits[n++] = (char)('0' + u % 10);
  } while (u /= 10);
  if (v < 0) out[len++] = '-';
  while (n) out[len++] = digits[--n];
  return len;
}

/* Floats as text (docs/spec/builtins.md#log): about 7 significant digits, with trailing zeros dropped
   (but at least one decimal): 1.5, 0.333333, 1234567.0. Outside 1e-4 to 1e9, scientific: 1.5e12. */
int rio_formatFloat(char *out, float f) {
  int len = 0, exponent = 0, intDigits = 1, decimals, k;
  double d = f;
  uint32_t intPart, fracPart, scale = 1, t;
  if (f != f) {
    memcpy(out, "nan", 3);
    return 3;
  }
  if (d < 0) {
    out[len++] = '-';
    d = -d;
  }
  if (d > 3.5e38) {
    memcpy(out + len, "inf", 3);
    return len + 3;
  }
  if (d != 0 && (d >= 1e9 || d < 1e-4)) {
    while (d >= 10) {
      d /= 10;
      exponent++;
    }
    while (d < 1) {
      d *= 10;
      exponent--;
    }
  }
  intPart = (uint32_t)d;
  for (t = intPart; t >= 10; t /= 10) intDigits++;
  decimals = 7 - intDigits;
  if (decimals < 1) decimals = 1;
  if (decimals > 6) decimals = 6;
  for (k = 0; k < decimals; k++) scale *= 10;
  fracPart = (uint32_t)((d - intPart) * scale + 0.5);
  if (fracPart >= scale) { /* rounded up to the next integer */
    intPart++;
    fracPart -= scale;
    if (exponent && intPart == 10) { /* 9.9999996e-6 is 1.0e-5, not 10.0e-6 */
      intPart = 1;
      exponent++;
    }
  }
  len += rio_formatInt(out + len, (int32_t)intPart);
  out[len++] = '.';
  for (k = decimals - 1; k >= 0; k--) {
    out[len + k] = (char)('0' + fracPart % 10);
    fracPart /= 10;
  }
  while (decimals > 1 && out[len + decimals - 1] == '0') decimals--;
  len += decimals;
  if (exponent) {
    out[len++] = 'e';
    len += rio_formatInt(out + len, exponent);
  }
  return len;
}

int rio_appendText(char *out, int len, const char *text) {
  while (*text && len < rio_ERROR_BUF_SIZE - 1) out[len++] = *text++;
  return len;
}

/* ------------------------------------------------------------------ errors */

int rio_setError(rio_Vm *vm, int kind, int line, int col, int len, const char *message) {
  char *out = vm->errorText;
  int n = 0;
  if (line > 0) {
    n += rio_formatInt(out, line);
    if (col > 0) {
      out[n++] = ':';
      n += rio_formatInt(out + n, col);
    }
    n = rio_appendText(out, n, ": ");
  }
  vm->errorKind = kind;
  vm->errorLine = line;
  vm->errorCol = col;
  vm->errorLen = len;
  vm->errorMsgAt = n;
  vm->errorFile = 0;
  if (kind == rio_ErrorKind_runtime) n = rio_appendText(out, n, "runtime error: ");
  n = rio_appendText(out, n, message);
  out[n] = 0;
  return n;
}

/* ------------------------------------------------------------------ frames (internal.h) */

void rio_codeFrames(rio_Vm *vm, int run) {
  uint32_t from = run ? vm->framedAt / 2 : 0, len = run ? vm->codeLen : vm->codeLen / 2, i, base;
  int k = run ? vm->framedProcs : 0, j;
  if (!run && !vm->framedAt) return; /* (in the compiled form already) */
  if (!run) /* the procs, back to instructions (all of them) */
    for (j = 0; j < vm->procCount; j++) { vm->procs[j].entry /= 2; vm->procs[j].end /= 2; }
  for (i = from; i < len; i++) { /* (procs are in the code's order: k follows them) */
    rio_Instr *in = &vm->code[i];
    int which = rio_slotOperands(in->op);
    uint16_t shift;
    while (k < vm->procCount && vm->procs[k].end <= i) k++;
    base = k < vm->procCount && vm->procs[k].entry <= i ? vm->procs[k].frameStart : 0;
    shift = (uint16_t)(rio_FRAME_BIAS - base);
    if (!run) shift = (uint16_t)-shift;
    if (which & 1) in->a = (uint16_t)(in->a + shift);
    if (which & 2) in->b = (uint16_t)(in->b + shift);
    if (which & 4) in->c = (uint16_t)(in->c + shift);
    if (rio_isJump(in->op)) in->c = (uint16_t)(run ? in->c * 2 : in->c / 2);
    if (in->op != rio_Op_call) continue; /* calls: by proc number, or by entry */
    if (!run) in->c = vm->procs[in->c].entry;
    else {
      for (j = 0; j < vm->procCount && (j < vm->framedProcs ? vm->procs[j].entry / 2u : vm->procs[j].entry) != in->c; j++) {}
      in->c = (uint16_t)j;
    }
  }
  if (run) { /* the new procs, into units */
    for (j = vm->framedProcs; j < vm->procCount; j++) { vm->procs[j].entry *= 2; vm->procs[j].end *= 2; }
    vm->codeLen = vm->framedAt = len * 2;
    vm->framedProcs = vm->procCount;
  } else {
    vm->codeLen = len;
    vm->framedAt = 0;
    vm->framedProcs = 0;
  }
}

/* ------------------------------------------------------------------ short forms (internal.h) */

#define rio_SHORT_WIDE(o, k) rio_Op_##o,
#define rio_SHORT_WIDE_MORE(o, w, k) rio_Op_##w,
#define rio_SHORT_KINDS(o, k) k,
#define rio_SHORT_KINDS_MORE(o, w, k) k,
#define rio_SHORT_OF(o, k) [rio_Op_##o] = rio_Op_s_##o,
const uint8_t rio_wideOf[rio_Op_s_end - 128] = {rio_SHORT_OPS(rio_SHORT_WIDE) rio_SHORT_MORE(rio_SHORT_WIDE_MORE)};
const char rio_shortKinds[rio_Op_s_end - 128][4] = {rio_SHORT_OPS(rio_SHORT_KINDS) rio_SHORT_MORE(rio_SHORT_KINDS_MORE)};
const uint8_t rio_shortOf[rio_Op_count] = {rio_SHORT_OPS(rio_SHORT_OF)};

/* where operand k (0 a, 1 b, 2 c) of a short instruction is: its byte (0 a, 1 b, 2 c), or 1 for a G (b and
   c together), whose other operand is in a */
int rio_shortField(const char *kinds, int k) {
  if (!strchr(kinds, 'G')) return k;
  return kinds[k] == 'G' ? 1 : 0;
}

/* the frame start of the proc whose code pc is in (procs in the code's order; entry and end as the code
   counts), or 0 (top-level code) */
static uint32_t rio_frameAtPc(const rio_Vm *vm, uint32_t pc) {
  int lo = 0, hi = vm->procCount;
  while (lo < hi) { /* (the first proc ending past pc) */
    int m = (lo + hi) / 2;
    if (vm->procs[m].end <= pc) lo = m + 1;
    else hi = m;
  }
  return lo < vm->procCount && vm->procs[lo].entry <= pc ? vm->procs[lo].frameStart : 0;
}

uint32_t rio_unitIndex(const rio_Vm *vm, uint32_t pc) {
  const uint32_t *units = (const uint32_t *)(const void *)vm->code;
  uint32_t at = 0, i = 0, n;
  while (at < pc && at < vm->codeLen) {
    n = rio_unitsAt(units, at, vm->codeLen);
    i += n == 4 ? 2 : 1; /* (a data word is an instruction of the compiler's) */
    at += n;
  }
  return i;
}

/* rio_instrAt, for code in a proc whose frame starts at base */
static uint32_t rio_instrIn(const rio_Vm *vm, uint32_t pc, uint32_t base, rio_Instr *out) {
  const uint32_t *units = (const uint32_t *)(const void *)vm->code;
  uint32_t n, k;
  if (pc >= vm->codeLen) return 0;
  n = rio_unitsAt(units, pc, vm->codeLen);
  if (n > 1) { memcpy(out, units + pc, sizeof *out); return n; }
  { /* a short one, in the wide form: slots biased as wide ones are (K and G from mem's start, so from
       the frame's too), a jump's target absolute */
    uint8_t sh[4];
    const char *kinds;
    uint16_t v[3];
    memcpy(sh, units + pc, sizeof sh);
    kinds = rio_shortKinds[sh[0] - 128];
    for (k = 0; k < 3; k++) {
      int f = rio_shortField(kinds, (int)k);
      uint32_t x = kinds[k] == 'G' ? (uint32_t)sh[2] | (uint32_t)sh[3] << 8 : sh[f + 1];
      v[k] = (uint16_t)(kinds[k] == 'S' ? x - rio_SHORT_BIAS + rio_FRAME_BIAS : kinds[k] == 'K' || kinds[k] == 'G' ? x - base + rio_FRAME_BIAS :
                        kinds[k] == 'J' ? pc + (uint32_t)(int8_t)x : kinds[k] == '-' ? 0 : x);
    }
    out->op = rio_wideOf[sh[0] - 128]; out->x = 0; out->a = v[0]; out->b = v[1]; out->c = v[2];
  }
  return 1;
}

uint32_t rio_instrAt(const rio_Vm *vm, uint32_t pc, rio_Instr *out) { return rio_instrIn(vm, pc, rio_frameAtPc(vm, pc), out); }

/* the instruction whose first unit is pc (unitOf ascending: n of them) */
static uint32_t rio_indexOfUnit(const uint16_t *unitOf, uint32_t n, uint32_t pc) {
  uint32_t lo = 0, hi = n;
  while (lo < hi) {
    uint32_t m = (lo + hi) / 2;
    if (unitOf[m] < pc) lo = m + 1;
    else hi = m;
  }
  return lo;
}

uint32_t rio_unpackInto(rio_Vm *vm, rio_Instr *buf, uint16_t *unitOf, uint16_t (*kept)[2]) {
  uint32_t pc, k, n = 0, i;
  int j;
  for (pc = 0, j = 0; pc < vm->codeLen; pc = k) { /* each instruction, wide, as the vm runs it; then its data word */
    while (j < vm->procCount && vm->procs[j].end <= pc) j++;
    k = pc + rio_instrIn(vm, pc, j < vm->procCount && vm->procs[j].entry <= pc ? vm->procs[j].frameStart : 0, &buf[n]);
    unitOf[n++] = (uint16_t)pc;
    if (k - pc == 4) {
      memcpy(&buf[n], (const uint32_t *)(const void *)vm->code + pc + 2, sizeof *buf);
      unitOf[n++] = (uint16_t)(pc + 2);
    }
  }
  unitOf[n] = (uint16_t)vm->codeLen;
  for (j = 0; j < vm->procCount; j++) {
    rio_Proc *p = &vm->procs[j];
    kept[j][0] = p->entry;
    kept[j][1] = p->end;
    p->entry = (uint16_t)rio_indexOfUnit(unitOf, n + 1, p->entry);
    p->end = (uint16_t)rio_indexOfUnit(unitOf, n + 1, p->end);
  }
  for (i = 0, j = 0; i < n; i++) { /* slots from mem's start, jumps by instruction, calls by entry (procs in the code's order) */
    rio_Instr *in = &buf[i];
    int which = rio_slotOperands(in->op);
    uint16_t shift;
    uint32_t base;
    if (in->op == rio_Op_data) continue;
    while (j < vm->procCount && vm->procs[j].end <= i) j++;
    base = j < vm->procCount && vm->procs[j].entry <= i ? vm->procs[j].frameStart : 0;
    shift = (uint16_t)(rio_FRAME_BIAS - base);
    if (which & 1) in->a = (uint16_t)(in->a - shift);
    if (which & 2) in->b = (uint16_t)(in->b - shift);
    if (which & 4) in->c = (uint16_t)(in->c - shift);
    if (rio_isJump(in->op)) in->c = (uint16_t)rio_indexOfUnit(unitOf, n + 1, in->c);
    if (in->op == rio_Op_call) in->c = vm->procs[in->c].entry;
  }
  return n;
}

void rio_procsInUnits(rio_Vm *vm, const uint16_t (*kept)[2]) {
  int j;
  for (j = 0; j < vm->procCount; j++) { vm->procs[j].entry = kept[j][0]; vm->procs[j].end = kept[j][1]; }
}
