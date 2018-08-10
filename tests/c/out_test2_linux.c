// Preamble
#define __USE_MINGW_ANSI_STDIO 1
#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif
#ifndef _CRT_NONSTDC_NO_DEPRECATE
#define _CRT_NONSTDC_NO_DEPRECATE
#endif

#if _MSC_VER >= 1900 || __STDC_VERSION__ >= 201112L
// Visual Studio 2015 supports enough C99/C11 features for us.
#else
#error "C11 support required or Visual Studio 2015 or later"
#endif

#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wvarargs"
#endif

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <stdarg.h>
#include <assert.h>

typedef unsigned char uchar;
typedef signed char schar;
typedef unsigned short ushort;
typedef unsigned int uint;
typedef unsigned long ulong;
typedef long long llong;
typedef unsigned long long ullong;

#ifdef _MSC_VER
#define alignof(x) __alignof(x)
#else
#define alignof(x) __alignof__(x)
#endif

#define va_start_ptr(args, arg) (va_start(*(args), *(arg)))
#define va_copy_ptr(dest, src) (va_copy(*(dest), *(src)))
#define va_end_ptr(args) (va_end(*(args)))

struct Any;
static void va_arg_ptr(va_list *args, struct Any any);


// Foreign preamble
typedef struct tm tm_t;

// Foreign header files
#include <limits.h>
#include <stdint.h>
#include <ctype.h>
#include <errno.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// Forward declarations
typedef struct TypeInfo TypeInfo;
typedef struct test2_vec2 test2_vec2;
typedef struct TypeFieldInfo TypeFieldInfo;
typedef struct Any Any;

// Sorted declarations
int main(int argc, char (*(*argv)));

extern char const ((*RIOOS));

extern char const ((*RIOARCH));

typedef ullong typeid;

typedef int TypeKind;

#define TYPE_NONE ((TypeKind)(0))

#define TYPE_VOID ((TypeKind)((TYPE_NONE) + (1)))

#define TYPE_BOOL ((TypeKind)((TYPE_VOID) + (1)))

#define TYPE_CHAR ((TypeKind)((TYPE_BOOL) + (1)))

#define TYPE_UCHAR ((TypeKind)((TYPE_CHAR) + (1)))

#define TYPE_SCHAR ((TypeKind)((TYPE_UCHAR) + (1)))

#define TYPE_SHORT ((TypeKind)((TYPE_SCHAR) + (1)))

#define TYPE_USHORT ((TypeKind)((TYPE_SHORT) + (1)))

#define TYPE_INT ((TypeKind)((TYPE_USHORT) + (1)))

#define TYPE_UINT ((TypeKind)((TYPE_INT) + (1)))

#define TYPE_LONG ((TypeKind)((TYPE_UINT) + (1)))

#define TYPE_ULONG ((TypeKind)((TYPE_LONG) + (1)))

#define TYPE_LLONG ((TypeKind)((TYPE_ULONG) + (1)))

#define TYPE_ULLONG ((TypeKind)((TYPE_LLONG) + (1)))

#define TYPE_FLOAT ((TypeKind)((TYPE_ULLONG) + (1)))

#define TYPE_DOUBLE ((TypeKind)((TYPE_FLOAT) + (1)))

#define TYPE_CONST ((TypeKind)((TYPE_DOUBLE) + (1)))

#define TYPE_PTR ((TypeKind)((TYPE_CONST) + (1)))

#define TYPE_PTR1 ((TypeKind)((TYPE_PTR) + (1)))

#define TYPE_ARRAY ((TypeKind)((TYPE_PTR1) + (1)))

#define TYPE_STRUCT ((TypeKind)((TYPE_ARRAY) + (1)))

#define TYPE_UNION ((TypeKind)((TYPE_STRUCT) + (1)))

#define TYPE_FUNC ((TypeKind)((TYPE_UNION) + (1)))

struct TypeInfo {
  TypeKind kind;
  int size;
  int align;
  char const ((*name));
  int count;
  typeid base;
  TypeFieldInfo (*fields);
  int num_fields;
};

TypeKind typeid_kind(typeid type);

int typeid_index(typeid type);

size_t typeid_size(typeid type);

TypeInfo const ((*get_typeinfo(typeid type)));

#define UCHAR_MIN ((uchar)(0))

#define USHORT_MIN ((short)(0))

#define UINT_MIN ((uint)(0))

#define ULLONG_MIN ((ullong)(0))

#define UINT8_MIN (UCHAR_MIN)

#define UINT16_MIN (USHORT_MIN)

#define UINT32_MIN (UINT_MIN)

#define UINT64_MIN (ULLONG_MIN)

#define ULONG_MIN ((ulong)(INT64_MIN))

#define USIZE_MIN (UINT64_MIN)

#define UINTPTR_MIN (UINT64_MIN)

struct test2_vec2 {
  float x;
  float y;
};

test2_vec2 test2_rot2(float a, test2_vec2 b);

test2_vec2 test2_add2(test2_vec2 a, test2_vec2 b);

struct TypeFieldInfo {
  char const ((*name));
  typeid type;
  int offset;
};

struct Any {
  void (*ptr);
  typeid type;
};

// Typeinfo
#define TYPEID0(index, kind) ((ullong)(index) | ((ullong)(kind) << 24))
#define TYPEID(index, kind, ...) ((ullong)(index) | ((ullong)sizeof(__VA_ARGS__) << 32) | ((ullong)(kind) << 24))

const TypeInfo *typeinfo_table[179] = {
  [0] = NULL, // No associated type
  [1] = &(TypeInfo){TYPE_VOID, .name = "void", .size = 0, .align = 0},
  [2] = &(TypeInfo){TYPE_BOOL, .size = sizeof(bool), .align = sizeof(bool), .name = "bool"},
  [3] = &(TypeInfo){TYPE_CHAR, .size = sizeof(char), .align = sizeof(char), .name = "char"},
  [4] = &(TypeInfo){TYPE_UCHAR, .size = sizeof(uchar), .align = sizeof(uchar), .name = "uchar"},
  [5] = &(TypeInfo){TYPE_SCHAR, .size = sizeof(schar), .align = sizeof(schar), .name = "schar"},
  [6] = &(TypeInfo){TYPE_SHORT, .size = sizeof(short), .align = sizeof(short), .name = "short"},
  [7] = &(TypeInfo){TYPE_USHORT, .size = sizeof(ushort), .align = sizeof(ushort), .name = "ushort"},
  [8] = &(TypeInfo){TYPE_INT, .size = sizeof(int), .align = sizeof(int), .name = "int"},
  [9] = &(TypeInfo){TYPE_UINT, .size = sizeof(uint), .align = sizeof(uint), .name = "uint"},
  [10] = &(TypeInfo){TYPE_LONG, .size = sizeof(long), .align = sizeof(long), .name = "long"},
  [11] = &(TypeInfo){TYPE_ULONG, .size = sizeof(ulong), .align = sizeof(ulong), .name = "ulong"},
  [12] = &(TypeInfo){TYPE_LLONG, .size = sizeof(llong), .align = sizeof(llong), .name = "llong"},
  [13] = &(TypeInfo){TYPE_ULLONG, .size = sizeof(ullong), .align = sizeof(ullong), .name = "ullong"},
  [14] = &(TypeInfo){TYPE_FLOAT, .size = sizeof(float), .align = sizeof(float), .name = "float"},
  [15] = &(TypeInfo){TYPE_DOUBLE, .size = sizeof(double), .align = sizeof(double), .name = "double"},
  [16] = &(TypeInfo){TYPE_PTR1, .size = sizeof(void *), .align = alignof(void *), .base = TYPEID0(1, TYPE_VOID)},
  [17] = &(TypeInfo){TYPE_PTR, .size = sizeof(void *), .align = alignof(void *), .base = TYPEID(3, TYPE_CHAR, char)},
  [18] = &(TypeInfo){TYPE_PTR, .size = sizeof(void *), .align = alignof(void *), .base = TYPEID(17, TYPE_PTR, char *)},
  [19] = NULL, // Function
  [20] = &(TypeInfo){TYPE_CONST, .size = sizeof(char const ), .align = alignof(char const ), .base = TYPEID(3, TYPE_CHAR, char)},
  [21] = &(TypeInfo){TYPE_PTR, .size = sizeof(void *), .align = alignof(void *), .base = TYPEID(20, TYPE_CONST, char const )},
  [22] = &(TypeInfo){TYPE_ARRAY, .size = sizeof(char [6]), .align = alignof(char [6]), .base = TYPEID(3, TYPE_CHAR, char), .count = 6},
  [23] = &(TypeInfo){TYPE_ARRAY, .size = sizeof(char [4]), .align = alignof(char [4]), .base = TYPEID(3, TYPE_CHAR, char), .count = 4},
  [24] = NULL, // Enum
  [25] = &(TypeInfo){TYPE_STRUCT, .size = sizeof(TypeFieldInfo), .align = alignof(TypeFieldInfo), .name = "TypeFieldInfo", .num_fields = 3, .fields = (TypeFieldInfo[]) {
    {"name", .type = TYPEID(21, TYPE_PTR, char const (*)), .offset = offsetof(TypeFieldInfo, name)},
    {"type", .type = TYPEID(13, TYPE_ULLONG, ullong), .offset = offsetof(TypeFieldInfo, type)},
    {"offset", .type = TYPEID(8, TYPE_INT, int), .offset = offsetof(TypeFieldInfo, offset)},
  }},
  [26] = &(TypeInfo){TYPE_STRUCT, .size = sizeof(TypeInfo), .align = alignof(TypeInfo), .name = "TypeInfo", .num_fields = 8, .fields = (TypeFieldInfo[]) {
    {"kind", .type = TYPEID(24, TYPE_NONE, TypeKind), .offset = offsetof(TypeInfo, kind)},
    {"size", .type = TYPEID(8, TYPE_INT, int), .offset = offsetof(TypeInfo, size)},
    {"align", .type = TYPEID(8, TYPE_INT, int), .offset = offsetof(TypeInfo, align)},
    {"name", .type = TYPEID(21, TYPE_PTR, char const (*)), .offset = offsetof(TypeInfo, name)},
    {"count", .type = TYPEID(8, TYPE_INT, int), .offset = offsetof(TypeInfo, count)},
    {"base", .type = TYPEID(13, TYPE_ULLONG, ullong), .offset = offsetof(TypeInfo, base)},
    {"fields", .type = TYPEID(27, TYPE_PTR, TypeFieldInfo *), .offset = offsetof(TypeInfo, fields)},
    {"num_fields", .type = TYPEID(8, TYPE_INT, int), .offset = offsetof(TypeInfo, num_fields)},
  }},
  [27] = &(TypeInfo){TYPE_PTR, .size = sizeof(void *), .align = alignof(void *), .base = TYPEID(25, TYPE_STRUCT, TypeFieldInfo)},
  [28] = &(TypeInfo){TYPE_CONST, .size = sizeof(TypeInfo const ), .align = alignof(TypeInfo const ), .base = TYPEID(26, TYPE_STRUCT, TypeInfo)},
  [29] = &(TypeInfo){TYPE_PTR, .size = sizeof(void *), .align = alignof(void *), .base = TYPEID(28, TYPE_CONST, TypeInfo const )},
  [30] = &(TypeInfo){TYPE_PTR, .size = sizeof(void *), .align = alignof(void *), .base = TYPEID(29, TYPE_PTR, TypeInfo const (*))},
  [31] = NULL, // Function
  [32] = NULL, // Function
  [33] = NULL, // Function
  [34] = NULL, // Function
  [35] = &(TypeInfo){TYPE_STRUCT, .size = sizeof(Any), .align = alignof(Any), .name = "Any", .num_fields = 2, .fields = (TypeFieldInfo[]) {
    {"ptr", .type = TYPEID(16, TYPE_PTR1, void *), .offset = offsetof(Any, ptr)},
    {"type", .type = TYPEID(13, TYPE_ULLONG, ullong), .offset = offsetof(Any, type)},
  }},
  [36] = &(TypeInfo){TYPE_PTR, .size = sizeof(void *), .align = alignof(void *), .base = TYPEID0(1, TYPE_VOID)},
  [37] = NULL, // Function
  [38] = &(TypeInfo){TYPE_ARRAY, .size = sizeof(char [15]), .align = alignof(char [15]), .base = TYPEID(3, TYPE_CHAR, char), .count = 15},
  [39] = &(TypeInfo){TYPE_STRUCT, .size = sizeof(test2_vec2), .align = alignof(test2_vec2), .name = "test2_vec2", .num_fields = 2, .fields = (TypeFieldInfo[]) {
    {"x", .type = TYPEID(14, TYPE_FLOAT, float), .offset = offsetof(test2_vec2, x)},
    {"y", .type = TYPEID(14, TYPE_FLOAT, float), .offset = offsetof(test2_vec2, y)},
  }},
  [40] = NULL, // Function
  [41] = NULL, // Function
  [42] = NULL, // Function
  [43] = NULL, // Function
  [44] = NULL, // Function
  [45] = NULL, // Function
  [46] = NULL, // Function
  [47] = NULL, // Function
  [48] = NULL, // Function
  [49] = NULL, // Function
  [50] = NULL, // Function
  [51] = &(TypeInfo){TYPE_PTR, .size = sizeof(void *), .align = alignof(void *), .base = TYPEID(8, TYPE_INT, int)},
  [52] = NULL, // Function
  [53] = NULL, // Function
  [54] = NULL, // Function
  [55] = NULL, // Function
  [56] = NULL, // Function
  [57] = NULL, // Function
  [58] = &(TypeInfo){TYPE_PTR, .size = sizeof(void *), .align = alignof(void *), .base = TYPEID(14, TYPE_FLOAT, float)},
  [59] = NULL, // Function
  [60] = &(TypeInfo){TYPE_PTR, .size = sizeof(void *), .align = alignof(void *), .base = TYPEID(15, TYPE_DOUBLE, double)},
  [61] = NULL, // Function
  [62] = NULL, // Function
  [63] = NULL, // Function
  [64] = NULL, // Function
  [65] = NULL, // Function
  [66] = NULL, // Function
  [67] = NULL, // Function
  [68] = NULL, // Function
  [69] = NULL, // Function
  [70] = NULL, // Function
  [71] = NULL, // Function
  [72] = NULL, // Function
  [73] = NULL, // Function
  [74] = &(TypeInfo){TYPE_CONST, .size = 0, .align = 0, .base = TYPEID0(1, TYPE_VOID)},
  [75] = &(TypeInfo){TYPE_PTR, .size = sizeof(void *), .align = alignof(void *), .base = TYPEID0(74, TYPE_CONST)},
  [76] = NULL, // Function
  [77] = NULL, // Function
  [78] = NULL, // Function
  [79] = &(TypeInfo){TYPE_CONST, .size = sizeof(char (*const )), .align = alignof(char (*const )), .base = TYPEID(17, TYPE_PTR, char *)},
  [80] = &(TypeInfo){TYPE_PTR, .size = sizeof(void *), .align = alignof(void *), .base = TYPEID(79, TYPE_CONST, char (*const ))},
  [81] = NULL, // Function
  [82] = NULL, // No associated type
  [83] = NULL, // No associated type
  [84] = NULL, // No associated type
  [85] = NULL, // Function
  [86] = NULL, // Function
  [87] = NULL, // Function
  [88] = NULL, // Function
  [89] = NULL, // Function
  [90] = NULL, // Function
  [91] = NULL, // Function
  [92] = NULL, // Function
  [93] = NULL, // Function
  [94] = NULL, // Function
  [95] = NULL, // Function
  [96] = NULL, // Function
  [97] = NULL, // Function
  [98] = NULL, // Function
  [99] = NULL, // Function
  [100] = NULL, // Function
  [101] = NULL, // Function
  [102] = NULL, // Function
  [103] = NULL, // Function
  [104] = NULL, // Function
  [105] = NULL, // Function
  [106] = NULL, // Function
  [107] = NULL, // Function
  [108] = NULL, // Function
  [109] = NULL, // No associated type
  [110] = NULL, // Function
  [111] = NULL, // Function
  [112] = NULL, // Function
  [113] = NULL, // Function
  [114] = NULL, // Function
  [115] = NULL, // No associated type
  [116] = NULL, // No associated type
  [117] = NULL, // No associated type
  [118] = NULL, // Function
  [119] = NULL, // Function
  [120] = NULL, // Function
  [121] = NULL, // Function
  [122] = NULL, // Function
  [123] = NULL, // Function
  [124] = NULL, // Function
  [125] = NULL, // Function
  [126] = NULL, // Function
  [127] = NULL, // Function
  [128] = NULL, // Function
  [129] = NULL, // Function
  [130] = NULL, // Function
  [131] = NULL, // Function
  [132] = NULL, // Function
  [133] = NULL, // Function
  [134] = NULL, // Function
  [135] = NULL, // Function
  [136] = NULL, // Function
  [137] = NULL, // Function
  [138] = NULL, // Function
  [139] = NULL, // Function
  [140] = NULL, // Function
  [141] = NULL, // Function
  [142] = NULL, // Function
  [143] = NULL, // Function
  [144] = &(TypeInfo){TYPE_PTR, .size = sizeof(void *), .align = alignof(void *), .base = TYPEID(6, TYPE_SHORT, short)},
  [145] = NULL, // Function
  [146] = NULL, // Function
  [147] = NULL, // Function
  [148] = &(TypeInfo){TYPE_CONST, .size = sizeof(short const ), .align = alignof(short const ), .base = TYPEID(6, TYPE_SHORT, short)},
  [149] = &(TypeInfo){TYPE_PTR, .size = sizeof(void *), .align = alignof(void *), .base = TYPEID(148, TYPE_CONST, short const )},
  [150] = NULL, // Function
  [151] = NULL, // Function
  [152] = NULL, // Function
  [153] = NULL, // Function
  [154] = NULL, // Function
  [155] = NULL, // Function
  [156] = NULL, // Function
  [157] = NULL, // Function
  [158] = NULL, // Function
  [159] = NULL, // Function
  [160] = NULL, // Function
  [161] = NULL, // Function
  [162] = NULL, // Function
  [163] = NULL, // Function
  [164] = NULL, // No associated type
  [165] = NULL, // Function
  [166] = NULL, // Function
  [167] = NULL, // No associated type
  [168] = NULL, // Function
  [169] = &(TypeInfo){TYPE_PTR, .size = sizeof(void *), .align = alignof(void *), .base = TYPEID(10, TYPE_LONG, long)},
  [170] = NULL, // Function
  [171] = NULL, // No associated type
  [172] = NULL, // No associated type
  [173] = NULL, // Function
  [174] = &(TypeInfo){TYPE_CONST, .size = sizeof(long const ), .align = alignof(long const ), .base = TYPEID(10, TYPE_LONG, long)},
  [175] = &(TypeInfo){TYPE_PTR, .size = sizeof(void *), .align = alignof(void *), .base = TYPEID(174, TYPE_CONST, long const )},
  [176] = NULL, // Function
  [177] = NULL, // Function
  [178] = NULL, // Function
};

int num_typeinfos = 179;
const TypeInfo **typeinfos = (const TypeInfo **)typeinfo_table;

// Definitions
int main(int argc, char (*(*argv))) {
  printf("Hello, world!\n");
  int a = 0;
  test2_vec2 v = {1, 0};
  test2_vec2 w = {0, 1};
  v = test2_rot2((3.14f) / (4), v);
  w = test2_add2(w, (test2_vec2){0.1f, 0.2f});
  return 0;
}

char const ((*RIOOS)) = "linux";
char const ((*RIOARCH)) = "x64";
TypeKind typeid_kind(typeid type) {
  return (TypeKind)((((type) >> (24))) & (0xff));
}

int typeid_index(typeid type) {
  return (int)((type) & (0xffffff));
}

size_t typeid_size(typeid type) {
  return (size_t)((type) >> (32));
}

TypeInfo const ((*get_typeinfo(typeid type))) {
  int index = typeid_index(type);
  if ((typeinfos) && ((index) < (num_typeinfos))) {
    return typeinfos[index];
  } else {
    return NULL;
  }
}

test2_vec2 test2_rot2(float a, test2_vec2 b) {
  float c = cosf(a);
  float s = sinf(a);
  return (test2_vec2){((c) * (b.x)) - ((s) * (b.y)), ((s) * (b.x)) + ((c) * (b.y))};
}

test2_vec2 test2_add2(test2_vec2 a, test2_vec2 b) {
  return (test2_vec2){(a.x) + (b.x), (a.y) + (b.y)};
}

// Foreign source files

static void va_arg_ptr(va_list *args, Any any) {
  switch (typeid_kind(any.type)) {
  case TYPE_BOOL:
    *(bool *)any.ptr = (bool)va_arg(*args, int);
    break;
  case TYPE_CHAR:
    *(char *)any.ptr = (char)va_arg(*args, int);
    break;
  case TYPE_UCHAR:
    *(uchar *)any.ptr = (uchar)va_arg(*args, int);
    break;
  case TYPE_SCHAR:
    *(schar *)any.ptr = (schar)va_arg(*args, int);
    break;
  case TYPE_SHORT:
    *(short *)any.ptr = (short)va_arg(*args, int);
    break;
  case TYPE_USHORT:
    *(ushort *)any.ptr = (ushort)va_arg(*args, int);
    break;
  case TYPE_INT:
    *(int *)any.ptr = va_arg(*args, int);
    break;
  case TYPE_UINT:
    *(uint *)any.ptr = va_arg(*args, uint);
    break;
  case TYPE_LONG:
    *(long *)any.ptr = va_arg(*args, long);
    break;
  case TYPE_ULONG:
    *(ulong *)any.ptr = va_arg(*args, ulong);
    break;
  case TYPE_LLONG:
    *(llong *)any.ptr = va_arg(*args, llong);
    break;
  case TYPE_ULLONG:
    *(ullong *)any.ptr = va_arg(*args, ullong);
    break;
  case TYPE_FLOAT:
    *(float *)any.ptr = (float)va_arg(*args, double);
    break;
  case TYPE_DOUBLE:
    *(double *)any.ptr = va_arg(*args, double);
    break;
  case TYPE_FUNC:
  case TYPE_PTR:
    *(void **)any.ptr = va_arg(*args, void *);
    break;
  default:
    assert(0 && "argument type not supported");
    break;
  }
}

#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif
