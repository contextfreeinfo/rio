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

extern char const ((*IONOS));

extern char const ((*IONARCH));

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

#define TYPE_ARRAY ((TypeKind)((TYPE_PTR) + (1)))

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

const TypeInfo *typeinfo_table[178] = {
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
    [16] = &(TypeInfo){TYPE_PTR, .size = sizeof(void *), .align = alignof(void *), .base = TYPEID0(1, TYPE_VOID)},
    [17] = &(TypeInfo){TYPE_PTR, .size = sizeof(void *), .align = alignof(void *), .base = TYPEID(3, TYPE_CHAR, char)},
    [18] = &(TypeInfo){TYPE_PTR, .size = sizeof(void *), .align = alignof(void *), .base = TYPEID(17, TYPE_PTR, char *)},
    [19] = NULL, // Func
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
    [31] = NULL, // Func
    [32] = NULL, // Func
    [33] = NULL, // Func
    [34] = NULL, // Func
    [35] = &(TypeInfo){TYPE_STRUCT, .size = sizeof(Any), .align = alignof(Any), .name = "Any", .num_fields = 2, .fields = (TypeFieldInfo[]) {
        {"ptr", .type = TYPEID(16, TYPE_PTR, void *), .offset = offsetof(Any, ptr)},
        {"type", .type = TYPEID(13, TYPE_ULLONG, ullong), .offset = offsetof(Any, type)},
    }},
    [36] = NULL, // Func
    [37] = &(TypeInfo){TYPE_ARRAY, .size = sizeof(char [15]), .align = alignof(char [15]), .base = TYPEID(3, TYPE_CHAR, char), .count = 15},
    [38] = &(TypeInfo){TYPE_STRUCT, .size = sizeof(test2_vec2), .align = alignof(test2_vec2), .name = "test2_vec2", .num_fields = 2, .fields = (TypeFieldInfo[]) {
        {"x", .type = TYPEID(14, TYPE_FLOAT, float), .offset = offsetof(test2_vec2, x)},
        {"y", .type = TYPEID(14, TYPE_FLOAT, float), .offset = offsetof(test2_vec2, y)},
    }},
    [39] = NULL, // Func
    [40] = NULL, // Func
    [41] = NULL, // Func
    [42] = NULL, // Func
    [43] = NULL, // Func
    [44] = NULL, // Func
    [45] = NULL, // Func
    [46] = NULL, // Func
    [47] = NULL, // Func
    [48] = NULL, // Func
    [49] = NULL, // Func
    [50] = &(TypeInfo){TYPE_PTR, .size = sizeof(void *), .align = alignof(void *), .base = TYPEID(8, TYPE_INT, int)},
    [51] = NULL, // Func
    [52] = NULL, // Func
    [53] = NULL, // Func
    [54] = NULL, // Func
    [55] = NULL, // Func
    [56] = NULL, // Func
    [57] = &(TypeInfo){TYPE_PTR, .size = sizeof(void *), .align = alignof(void *), .base = TYPEID(14, TYPE_FLOAT, float)},
    [58] = NULL, // Func
    [59] = &(TypeInfo){TYPE_PTR, .size = sizeof(void *), .align = alignof(void *), .base = TYPEID(15, TYPE_DOUBLE, double)},
    [60] = NULL, // Func
    [61] = NULL, // Func
    [62] = NULL, // Func
    [63] = NULL, // Func
    [64] = NULL, // Func
    [65] = NULL, // Func
    [66] = NULL, // Func
    [67] = NULL, // Func
    [68] = NULL, // Func
    [69] = NULL, // Func
    [70] = NULL, // Func
    [71] = NULL, // Func
    [72] = NULL, // Func
    [73] = &(TypeInfo){TYPE_CONST, .size = 0, .align = 0, .base = TYPEID0(1, TYPE_VOID)},
    [74] = &(TypeInfo){TYPE_PTR, .size = sizeof(void *), .align = alignof(void *), .base = TYPEID0(73, TYPE_CONST)},
    [75] = NULL, // Func
    [76] = NULL, // Func
    [77] = NULL, // Func
    [78] = &(TypeInfo){TYPE_CONST, .size = sizeof(char (*const )), .align = alignof(char (*const )), .base = TYPEID(17, TYPE_PTR, char *)},
    [79] = &(TypeInfo){TYPE_PTR, .size = sizeof(void *), .align = alignof(void *), .base = TYPEID(78, TYPE_CONST, char (*const ))},
    [80] = NULL, // Func
    [81] = NULL, // No associated type
    [82] = NULL, // No associated type
    [83] = NULL, // No associated type
    [84] = NULL, // Func
    [85] = NULL, // Func
    [86] = NULL, // Func
    [87] = NULL, // Func
    [88] = NULL, // Func
    [89] = NULL, // Func
    [90] = NULL, // Func
    [91] = NULL, // Func
    [92] = NULL, // Func
    [93] = NULL, // Func
    [94] = NULL, // Func
    [95] = NULL, // Func
    [96] = NULL, // Func
    [97] = NULL, // Func
    [98] = NULL, // Func
    [99] = NULL, // Func
    [100] = NULL, // Func
    [101] = NULL, // Func
    [102] = NULL, // Func
    [103] = NULL, // Func
    [104] = NULL, // Func
    [105] = NULL, // Func
    [106] = NULL, // Func
    [107] = NULL, // Func
    [108] = NULL, // No associated type
    [109] = NULL, // Func
    [110] = NULL, // Func
    [111] = NULL, // Func
    [112] = NULL, // Func
    [113] = NULL, // Func
    [114] = NULL, // No associated type
    [115] = NULL, // No associated type
    [116] = NULL, // No associated type
    [117] = NULL, // Func
    [118] = NULL, // Func
    [119] = NULL, // Func
    [120] = NULL, // Func
    [121] = NULL, // Func
    [122] = NULL, // Func
    [123] = NULL, // Func
    [124] = NULL, // Func
    [125] = NULL, // Func
    [126] = NULL, // Func
    [127] = NULL, // Func
    [128] = NULL, // Func
    [129] = NULL, // Func
    [130] = NULL, // Func
    [131] = NULL, // Func
    [132] = NULL, // Func
    [133] = NULL, // Func
    [134] = NULL, // Func
    [135] = NULL, // Func
    [136] = NULL, // Func
    [137] = NULL, // Func
    [138] = NULL, // Func
    [139] = NULL, // Func
    [140] = NULL, // Func
    [141] = NULL, // Func
    [142] = NULL, // Func
    [143] = &(TypeInfo){TYPE_PTR, .size = sizeof(void *), .align = alignof(void *), .base = TYPEID(6, TYPE_SHORT, short)},
    [144] = NULL, // Func
    [145] = NULL, // Func
    [146] = NULL, // Func
    [147] = &(TypeInfo){TYPE_CONST, .size = sizeof(short const ), .align = alignof(short const ), .base = TYPEID(6, TYPE_SHORT, short)},
    [148] = &(TypeInfo){TYPE_PTR, .size = sizeof(void *), .align = alignof(void *), .base = TYPEID(147, TYPE_CONST, short const )},
    [149] = NULL, // Func
    [150] = NULL, // Func
    [151] = NULL, // Func
    [152] = NULL, // Func
    [153] = NULL, // Func
    [154] = NULL, // Func
    [155] = NULL, // Func
    [156] = NULL, // Func
    [157] = NULL, // Func
    [158] = NULL, // Func
    [159] = NULL, // Func
    [160] = NULL, // Func
    [161] = NULL, // Func
    [162] = NULL, // Func
    [163] = NULL, // No associated type
    [164] = NULL, // Func
    [165] = NULL, // Func
    [166] = NULL, // No associated type
    [167] = NULL, // Func
    [168] = &(TypeInfo){TYPE_PTR, .size = sizeof(void *), .align = alignof(void *), .base = TYPEID(10, TYPE_LONG, long)},
    [169] = NULL, // Func
    [170] = NULL, // No associated type
    [171] = NULL, // No associated type
    [172] = NULL, // Func
    [173] = &(TypeInfo){TYPE_CONST, .size = sizeof(long const ), .align = alignof(long const ), .base = TYPEID(10, TYPE_LONG, long)},
    [174] = &(TypeInfo){TYPE_PTR, .size = sizeof(void *), .align = alignof(void *), .base = TYPEID(173, TYPE_CONST, long const )},
    [175] = NULL, // Func
    [176] = NULL, // Func
    [177] = NULL, // Func
};

int num_typeinfos = 178;
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

char const ((*IONOS)) = "linux";
char const ((*IONARCH)) = "x64";
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
