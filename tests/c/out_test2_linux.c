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

typedef int Result_Kind;

#define Result_Ok ((Result_Kind)(0))

#define Result_Err ((Result_Kind)((Result_Ok) + (1)))

extern char const ((*RIOOS));

extern char const ((*RIOARCH));

typedef ullong typeid;

typedef int TypeKind;

#define TypeKind_None ((TypeKind)(0))

#define TypeKind_Void ((TypeKind)((TypeKind_None) + (1)))

#define TypeKind_Bool ((TypeKind)((TypeKind_Void) + (1)))

#define TypeKind_Char ((TypeKind)((TypeKind_Bool) + (1)))

#define TypeKind_UChar ((TypeKind)((TypeKind_Char) + (1)))

#define TypeKind_SChar ((TypeKind)((TypeKind_UChar) + (1)))

#define TypeKind_Short ((TypeKind)((TypeKind_SChar) + (1)))

#define TypeKind_UShort ((TypeKind)((TypeKind_Short) + (1)))

#define TypeKind_Int ((TypeKind)((TypeKind_UShort) + (1)))

#define TypeKind_UInt ((TypeKind)((TypeKind_Int) + (1)))

#define TypeKind_Long ((TypeKind)((TypeKind_UInt) + (1)))

#define TypeKind_ULong ((TypeKind)((TypeKind_Long) + (1)))

#define TypeKind_LLong ((TypeKind)((TypeKind_ULong) + (1)))

#define TypeKind_ULLong ((TypeKind)((TypeKind_LLong) + (1)))

#define TypeKind_Float ((TypeKind)((TypeKind_ULLong) + (1)))

#define TypeKind_Double ((TypeKind)((TypeKind_Float) + (1)))

#define TypeKind_Const ((TypeKind)((TypeKind_Double) + (1)))

#define TypeKind_Ptr ((TypeKind)((TypeKind_Const) + (1)))

#define TypeKind_Ref ((TypeKind)((TypeKind_Ptr) + (1)))

#define TypeKind_Array ((TypeKind)((TypeKind_Ref) + (1)))

#define TypeKind_Struct ((TypeKind)((TypeKind_Array) + (1)))

#define TypeKind_Union ((TypeKind)((TypeKind_Struct) + (1)))

#define TypeKind_Func ((TypeKind)((TypeKind_Union) + (1)))

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

const TypeInfo *typeinfo_table[183] = {
  [0] = NULL, // No associated type
  [1] = &(TypeInfo){TypeKind_Void, .name = "void", .size = 0, .align = 0},
  [2] = &(TypeInfo){TypeKind_Bool, .size = sizeof(bool), .align = sizeof(bool), .name = "bool"},
  [3] = &(TypeInfo){TypeKind_Char, .size = sizeof(char), .align = sizeof(char), .name = "char"},
  [4] = &(TypeInfo){TypeKind_UChar, .size = sizeof(uchar), .align = sizeof(uchar), .name = "uchar"},
  [5] = &(TypeInfo){TypeKind_SChar, .size = sizeof(schar), .align = sizeof(schar), .name = "schar"},
  [6] = &(TypeInfo){TypeKind_Short, .size = sizeof(short), .align = sizeof(short), .name = "short"},
  [7] = &(TypeInfo){TypeKind_UShort, .size = sizeof(ushort), .align = sizeof(ushort), .name = "ushort"},
  [8] = &(TypeInfo){TypeKind_Int, .size = sizeof(int), .align = sizeof(int), .name = "int"},
  [9] = &(TypeInfo){TypeKind_UInt, .size = sizeof(uint), .align = sizeof(uint), .name = "uint"},
  [10] = &(TypeInfo){TypeKind_Long, .size = sizeof(long), .align = sizeof(long), .name = "long"},
  [11] = &(TypeInfo){TypeKind_ULong, .size = sizeof(ulong), .align = sizeof(ulong), .name = "ulong"},
  [12] = &(TypeInfo){TypeKind_LLong, .size = sizeof(llong), .align = sizeof(llong), .name = "llong"},
  [13] = &(TypeInfo){TypeKind_ULLong, .size = sizeof(ullong), .align = sizeof(ullong), .name = "ullong"},
  [14] = &(TypeInfo){TypeKind_Float, .size = sizeof(float), .align = sizeof(float), .name = "float"},
  [15] = &(TypeInfo){TypeKind_Double, .size = sizeof(double), .align = sizeof(double), .name = "double"},
  [16] = &(TypeInfo){TypeKind_Ref, .size = sizeof(void *), .align = alignof(void *), .base = TYPEID0(1, TypeKind_Void)},
  [17] = &(TypeInfo){TypeKind_Ptr, .size = sizeof(void *), .align = alignof(void *), .base = TYPEID(3, TypeKind_Char, char)},
  [18] = &(TypeInfo){TypeKind_Ptr, .size = sizeof(void *), .align = alignof(void *), .base = TYPEID(17, TypeKind_Ptr, char *)},
  [19] = NULL, // Function
  [20] = &(TypeInfo){TypeKind_Struct, .size = 0, .align = 0, .name = "Result", .num_fields = 3, .fields = (TypeFieldInfo[]) {
    {"kind", .type = TYPEID(21, TypeKind_None, Result_Kind)},
    {"val", .type = TYPEID0(1, TypeKind_Void)},
    {"err", .type = TYPEID0(1, TypeKind_Void)},
  }},
  [21] = NULL, // Enum
  [22] = &(TypeInfo){TypeKind_Struct, .size = 0, .align = 0, .name = "Slice", .num_fields = 2, .fields = (TypeFieldInfo[]) {
    {"items", .type = TYPEID(39, TypeKind_Ptr, void *)},
    {"length", .type = TYPEID(13, TypeKind_ULLong, ullong)},
  }},
  [23] = &(TypeInfo){TypeKind_Const, .size = sizeof(char const ), .align = alignof(char const ), .base = TYPEID(3, TypeKind_Char, char)},
  [24] = &(TypeInfo){TypeKind_Ptr, .size = sizeof(void *), .align = alignof(void *), .base = TYPEID(23, TypeKind_Const, char const )},
  [25] = &(TypeInfo){TypeKind_Array, .size = sizeof(char [6]), .align = alignof(char [6]), .base = TYPEID(3, TypeKind_Char, char), .count = 6},
  [26] = &(TypeInfo){TypeKind_Array, .size = sizeof(char [4]), .align = alignof(char [4]), .base = TYPEID(3, TypeKind_Char, char), .count = 4},
  [27] = NULL, // Enum
  [28] = &(TypeInfo){TypeKind_Struct, .size = sizeof(TypeFieldInfo), .align = alignof(TypeFieldInfo), .name = "TypeFieldInfo", .num_fields = 3, .fields = (TypeFieldInfo[]) {
    {"name", .type = TYPEID(24, TypeKind_Ptr, char const (*)), .offset = offsetof(TypeFieldInfo, name)},
    {"type", .type = TYPEID(13, TypeKind_ULLong, ullong), .offset = offsetof(TypeFieldInfo, type)},
    {"offset", .type = TYPEID(8, TypeKind_Int, int), .offset = offsetof(TypeFieldInfo, offset)},
  }},
  [29] = &(TypeInfo){TypeKind_Struct, .size = sizeof(TypeInfo), .align = alignof(TypeInfo), .name = "TypeInfo", .num_fields = 8, .fields = (TypeFieldInfo[]) {
    {"kind", .type = TYPEID(27, TypeKind_None, TypeKind), .offset = offsetof(TypeInfo, kind)},
    {"size", .type = TYPEID(8, TypeKind_Int, int), .offset = offsetof(TypeInfo, size)},
    {"align", .type = TYPEID(8, TypeKind_Int, int), .offset = offsetof(TypeInfo, align)},
    {"name", .type = TYPEID(24, TypeKind_Ptr, char const (*)), .offset = offsetof(TypeInfo, name)},
    {"count", .type = TYPEID(8, TypeKind_Int, int), .offset = offsetof(TypeInfo, count)},
    {"base", .type = TYPEID(13, TypeKind_ULLong, ullong), .offset = offsetof(TypeInfo, base)},
    {"fields", .type = TYPEID(30, TypeKind_Ptr, TypeFieldInfo *), .offset = offsetof(TypeInfo, fields)},
    {"num_fields", .type = TYPEID(8, TypeKind_Int, int), .offset = offsetof(TypeInfo, num_fields)},
  }},
  [30] = &(TypeInfo){TypeKind_Ptr, .size = sizeof(void *), .align = alignof(void *), .base = TYPEID(28, TypeKind_Struct, TypeFieldInfo)},
  [31] = &(TypeInfo){TypeKind_Const, .size = sizeof(TypeInfo const ), .align = alignof(TypeInfo const ), .base = TYPEID(29, TypeKind_Struct, TypeInfo)},
  [32] = &(TypeInfo){TypeKind_Ptr, .size = sizeof(void *), .align = alignof(void *), .base = TYPEID(31, TypeKind_Const, TypeInfo const )},
  [33] = &(TypeInfo){TypeKind_Ptr, .size = sizeof(void *), .align = alignof(void *), .base = TYPEID(32, TypeKind_Ptr, TypeInfo const (*))},
  [34] = NULL, // Function
  [35] = NULL, // Function
  [36] = NULL, // Function
  [37] = NULL, // Function
  [38] = &(TypeInfo){TypeKind_Struct, .size = sizeof(Any), .align = alignof(Any), .name = "Any", .num_fields = 2, .fields = (TypeFieldInfo[]) {
    {"ptr", .type = TYPEID(16, TypeKind_Ref, void *), .offset = offsetof(Any, ptr)},
    {"type", .type = TYPEID(13, TypeKind_ULLong, ullong), .offset = offsetof(Any, type)},
  }},
  [39] = &(TypeInfo){TypeKind_Ptr, .size = sizeof(void *), .align = alignof(void *), .base = TYPEID0(1, TypeKind_Void)},
  [40] = NULL, // Function
  [41] = &(TypeInfo){TypeKind_Array, .size = sizeof(char [15]), .align = alignof(char [15]), .base = TYPEID(3, TypeKind_Char, char), .count = 15},
  [42] = &(TypeInfo){TypeKind_Struct, .size = sizeof(test2_vec2), .align = alignof(test2_vec2), .name = "test2_vec2", .num_fields = 2, .fields = (TypeFieldInfo[]) {
    {"x", .type = TYPEID(14, TypeKind_Float, float), .offset = offsetof(test2_vec2, x)},
    {"y", .type = TYPEID(14, TypeKind_Float, float), .offset = offsetof(test2_vec2, y)},
  }},
  [43] = NULL, // Function
  [44] = NULL, // Function
  [45] = NULL, // No associated type
  [46] = NULL, // Function
  [47] = NULL, // Function
  [48] = NULL, // Function
  [49] = NULL, // Function
  [50] = NULL, // Function
  [51] = NULL, // Function
  [52] = NULL, // Function
  [53] = NULL, // Function
  [54] = NULL, // Function
  [55] = &(TypeInfo){TypeKind_Ptr, .size = sizeof(void *), .align = alignof(void *), .base = TYPEID(8, TypeKind_Int, int)},
  [56] = NULL, // Function
  [57] = NULL, // Function
  [58] = NULL, // Function
  [59] = NULL, // Function
  [60] = NULL, // Function
  [61] = NULL, // Function
  [62] = &(TypeInfo){TypeKind_Ptr, .size = sizeof(void *), .align = alignof(void *), .base = TYPEID(14, TypeKind_Float, float)},
  [63] = NULL, // Function
  [64] = &(TypeInfo){TypeKind_Ptr, .size = sizeof(void *), .align = alignof(void *), .base = TYPEID(15, TypeKind_Double, double)},
  [65] = NULL, // Function
  [66] = NULL, // Function
  [67] = NULL, // Function
  [68] = NULL, // Function
  [69] = NULL, // Function
  [70] = NULL, // Function
  [71] = NULL, // Function
  [72] = NULL, // Function
  [73] = NULL, // Function
  [74] = NULL, // Function
  [75] = NULL, // Function
  [76] = NULL, // Function
  [77] = NULL, // Function
  [78] = &(TypeInfo){TypeKind_Const, .size = 0, .align = 0, .base = TYPEID0(1, TypeKind_Void)},
  [79] = &(TypeInfo){TypeKind_Ref, .size = sizeof(void *), .align = alignof(void *), .base = TYPEID0(78, TypeKind_Const)},
  [80] = NULL, // Function
  [81] = NULL, // Function
  [82] = NULL, // Function
  [83] = &(TypeInfo){TypeKind_Const, .size = sizeof(char (*const )), .align = alignof(char (*const )), .base = TYPEID(17, TypeKind_Ptr, char *)},
  [84] = &(TypeInfo){TypeKind_Ptr, .size = sizeof(void *), .align = alignof(void *), .base = TYPEID(83, TypeKind_Const, char (*const ))},
  [85] = NULL, // Function
  [86] = NULL, // No associated type
  [87] = NULL, // No associated type
  [88] = NULL, // No associated type
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
  [109] = NULL, // Function
  [110] = NULL, // Function
  [111] = NULL, // Function
  [112] = NULL, // Function
  [113] = NULL, // No associated type
  [114] = NULL, // Function
  [115] = NULL, // Function
  [116] = NULL, // Function
  [117] = NULL, // Function
  [118] = NULL, // Function
  [119] = NULL, // No associated type
  [120] = NULL, // No associated type
  [121] = NULL, // No associated type
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
  [144] = NULL, // Function
  [145] = NULL, // Function
  [146] = NULL, // Function
  [147] = NULL, // Function
  [148] = &(TypeInfo){TypeKind_Ptr, .size = sizeof(void *), .align = alignof(void *), .base = TYPEID(6, TypeKind_Short, short)},
  [149] = NULL, // Function
  [150] = NULL, // Function
  [151] = NULL, // Function
  [152] = &(TypeInfo){TypeKind_Const, .size = sizeof(short const ), .align = alignof(short const ), .base = TYPEID(6, TypeKind_Short, short)},
  [153] = &(TypeInfo){TypeKind_Ptr, .size = sizeof(void *), .align = alignof(void *), .base = TYPEID(152, TypeKind_Const, short const )},
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
  [164] = NULL, // Function
  [165] = NULL, // Function
  [166] = NULL, // Function
  [167] = NULL, // Function
  [168] = NULL, // No associated type
  [169] = NULL, // Function
  [170] = NULL, // Function
  [171] = NULL, // No associated type
  [172] = NULL, // Function
  [173] = &(TypeInfo){TypeKind_Ref, .size = sizeof(void *), .align = alignof(void *), .base = TYPEID(10, TypeKind_Long, long)},
  [174] = NULL, // Function
  [175] = NULL, // No associated type
  [176] = NULL, // No associated type
  [177] = NULL, // Function
  [178] = &(TypeInfo){TypeKind_Const, .size = sizeof(long const ), .align = alignof(long const ), .base = TYPEID(10, TypeKind_Long, long)},
  [179] = &(TypeInfo){TypeKind_Ref, .size = sizeof(void *), .align = alignof(void *), .base = TYPEID(178, TypeKind_Const, long const )},
  [180] = NULL, // Function
  [181] = NULL, // Function
  [182] = NULL, // Function
};

int num_typeinfos = 183;
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
  case TypeKind_Bool:
    *(bool *)any.ptr = (bool)va_arg(*args, int);
    break;
  case TypeKind_Char:
    *(char *)any.ptr = (char)va_arg(*args, int);
    break;
  case TypeKind_UChar:
    *(uchar *)any.ptr = (uchar)va_arg(*args, int);
    break;
  case TypeKind_SChar:
    *(schar *)any.ptr = (schar)va_arg(*args, int);
    break;
  case TypeKind_Short:
    *(short *)any.ptr = (short)va_arg(*args, int);
    break;
  case TypeKind_UShort:
    *(ushort *)any.ptr = (ushort)va_arg(*args, int);
    break;
  case TypeKind_Int:
    *(int *)any.ptr = va_arg(*args, int);
    break;
  case TypeKind_UInt:
    *(uint *)any.ptr = va_arg(*args, uint);
    break;
  case TypeKind_Long:
    *(long *)any.ptr = va_arg(*args, long);
    break;
  case TypeKind_ULong:
    *(ulong *)any.ptr = va_arg(*args, ulong);
    break;
  case TypeKind_LLong:
    *(llong *)any.ptr = va_arg(*args, llong);
    break;
  case TypeKind_ULLong:
    *(ullong *)any.ptr = va_arg(*args, ullong);
    break;
  case TypeKind_Float:
    *(float *)any.ptr = (float)va_arg(*args, double);
    break;
  case TypeKind_Double:
    *(double *)any.ptr = va_arg(*args, double);
    break;
  case TypeKind_Func:
  case TypeKind_Ptr:
  case TypeKind_Ref:
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
