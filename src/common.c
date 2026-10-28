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

uint32_t rio_hashText(const char *s, uint32_t n) { /* (FNV-1a, 32 bits) */
  uint32_t h = 2166136261u, i;
  for (i = 0; i < n; i++) h = (h ^ (uint8_t)s[i]) * 16777619u;
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
