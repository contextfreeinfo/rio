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
typedef struct TypeFieldInfo TypeFieldInfo;
typedef struct Any Any;
typedef struct test1_Bar test1_Bar;
typedef struct test1_Foo test1_Foo;
typedef struct test1_Thing test1_Thing;
typedef struct test1_BufHdr test1_BufHdr;
typedef struct test1_Vector test1_Vector;
typedef struct test1_Ints test1_Ints;
typedef struct test1_ConstVector test1_ConstVector;
typedef struct test1_UartCtrl test1_UartCtrl;
typedef union test1_IntOrPtr test1_IntOrPtr;

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

void test1_test_const_ptr_deref(void);

void test1_test_limits(void);

void test1_test_va_list(char const ((*fmt)), ...);

void test1_test_os_arch(void);

void test1_test_packages(void);

void test1_test_if(void);

void test1_test_modify(void);

void test1_test_lvalue(void);

void test1_test_alignof(void);

void test1_test_offsetof(void);

void test1_test_complete(void);

void test1_test_compound_literals(void);

void test1_test_loops(void);

void test1_test_sizeof(void);

void test1_test_assign(void);

void test1_test_enum(void);

void test1_test_arrays(void);

void test1_test_cast(void);

void test1_test_init(void);

void test1_test_lits(void);

void test1_test_const(void);

void test1_test_bool(void);

void test1_test_ops(void);

void test1_test_typeinfo(void);

void test1_test_reachable(void);

struct TypeFieldInfo {
    char const ((*name));
    typeid type;
    int offset;
};

struct Any {
    void (*ptr);
    typeid type;
};

struct test1_Bar {
    int rc;
};

struct test1_Foo {
    test1_Bar (*const (bar));
};

int test1_subtest1_func1(void);

test1_Thing (*test1_returns_ptr(void));

struct test1_Thing {
    int a;
};

test1_Thing const ((*test1_returns_ptr_to_const(void)));

struct test1_BufHdr {
    size_t cap;
    size_t len;
    char (buf[1]);
};

struct test1_Vector {
    int x;
    int y;
};

struct test1_Ints {
    int num_ints;
    int (*int_ptr);
    int (int_arr[3]);
};

typedef int test1_Color;

#define TEST1_COLOR_NONE ((test1_Color)(0))

#define TEST1_COLOR_RED ((test1_Color)((TEST1_COLOR_NONE) + (1)))

#define TEST1_COLOR_GREEN ((test1_Color)((TEST1_COLOR_RED) + (1)))

#define TEST1_COLOR_BLUE ((test1_Color)((TEST1_COLOR_GREEN) + (1)))

#define TEST1_FOO ((int)(0))

#define TEST1_BAR ((int)((TEST1_FOO) + (1)))

typedef int8_t test1_TypedEnum;

#define TEST1_BAZ ((test1_TypedEnum)(0))

#define TEST1_QUUX ((test1_TypedEnum)((TEST1_BAZ) + (1)))

void test1_f10(wchar_t (a[3]));

struct test1_ConstVector {
    int const (x);
    int const (y);
};

extern test1_Vector const (test1_cv);

extern char const ((test1_escape_to_char[256]));

extern char const ((test1_char_to_escape[256]));

extern char (*test1_esc_test_str);

void test1_f4(char const ((*x)));

#define TEST1_IS_DEBUG (true)

void test1_println_any(Any any);

void test1_println_type(typeid type);

void test1_println_typeinfo(typeid type);

void test1_subtest1_func2(void);

extern test1_Thing test1_thing;

void test1_print_any(Any any);

void test1_print_type(typeid type);

struct test1_UartCtrl {
    bool tx_enable;
    bool rx_enable;
};

void test1_print_typeinfo(typeid type);

union test1_IntOrPtr {
    int i;
    int (*p);
};

void test1_subtest1_func3(void);

void test1_subtest1_func4(void);

void test1_print_typeid(typeid type);

// Typeinfo
#define TYPEID0(index, kind) ((ullong)(index) | ((ullong)(kind) << 24))
#define TYPEID(index, kind, ...) ((ullong)(index) | ((ullong)sizeof(__VA_ARGS__) << 32) | ((ullong)(kind) << 24))

const TypeInfo *typeinfo_table[258] = {
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
    [37] = &(TypeInfo){TYPE_ARRAY, .size = sizeof(char [14]), .align = alignof(char [14]), .base = TYPEID(3, TYPE_CHAR, char), .count = 14},
    [38] = NULL, // Func
    [39] = NULL, // Func
    [40] = &(TypeInfo){TYPE_ARRAY, .size = sizeof(char [9]), .align = alignof(char [9]), .base = TYPEID(3, TYPE_CHAR, char), .count = 9},
    [41] = NULL, // Func
    [42] = &(TypeInfo){TYPE_STRUCT, .size = sizeof(test1_Bar), .align = alignof(test1_Bar), .name = "test1_Bar", .num_fields = 1, .fields = (TypeFieldInfo[]) {
        {"rc", .type = TYPEID(8, TYPE_INT, int), .offset = offsetof(test1_Bar, rc)},
    }},
    [43] = &(TypeInfo){TYPE_STRUCT, .size = sizeof(test1_Foo), .align = alignof(test1_Foo), .name = "test1_Foo", .num_fields = 1, .fields = (TypeFieldInfo[]) {
        {"bar", .type = TYPEID(45, TYPE_CONST, test1_Bar (*const )), .offset = offsetof(test1_Foo, bar)},
    }},
    [44] = &(TypeInfo){TYPE_PTR, .size = sizeof(void *), .align = alignof(void *), .base = TYPEID(42, TYPE_STRUCT, test1_Bar)},
    [45] = &(TypeInfo){TYPE_CONST, .size = sizeof(test1_Bar (*const )), .align = alignof(test1_Bar (*const )), .base = TYPEID(44, TYPE_PTR, test1_Bar *)},
    [46] = &(TypeInfo){TYPE_CONST, .size = 0, .align = 0, .base = TYPEID0(1, TYPE_VOID)},
    [47] = &(TypeInfo){TYPE_PTR, .size = sizeof(void *), .align = alignof(void *), .base = TYPEID0(46, TYPE_CONST)},
    [48] = NULL, // Func
    [49] = &(TypeInfo){TYPE_PTR, .size = sizeof(void *), .align = alignof(void *), .base = TYPEID(21, TYPE_PTR, char const (*))},
    [50] = &(TypeInfo){TYPE_CONST, .size = sizeof(char (*const )), .align = alignof(char (*const )), .base = TYPEID(17, TYPE_PTR, char *)},
    [51] = &(TypeInfo){TYPE_PTR, .size = sizeof(void *), .align = alignof(void *), .base = TYPEID(50, TYPE_CONST, char (*const ))},
    [52] = NULL, // Func
    [53] = NULL, // Func
    [54] = NULL, // Func
    [55] = &(TypeInfo){TYPE_PTR, .size = sizeof(void *), .align = alignof(void *), .base = TYPEID(8, TYPE_INT, int)},
    [56] = &(TypeInfo){TYPE_PTR, .size = sizeof(void *), .align = alignof(void *), .base = TYPEID(12, TYPE_LLONG, llong)},
    [57] = &(TypeInfo){TYPE_ARRAY, .size = sizeof(char [19]), .align = alignof(char [19]), .base = TYPEID(3, TYPE_CHAR, char), .count = 19},
    [58] = &(TypeInfo){TYPE_ARRAY, .size = sizeof(char [29]), .align = alignof(char [29]), .base = TYPEID(3, TYPE_CHAR, char), .count = 29},
    [59] = &(TypeInfo){TYPE_ARRAY, .size = sizeof(char [33]), .align = alignof(char [33]), .base = TYPEID(3, TYPE_CHAR, char), .count = 33},
    [60] = &(TypeInfo){TYPE_ARRAY, .size = sizeof(int [16]), .align = alignof(int [16]), .base = TYPEID(8, TYPE_INT, int), .count = 16},
    [61] = &(TypeInfo){TYPE_STRUCT, .size = sizeof(test1_Thing), .align = alignof(test1_Thing), .name = "test1_Thing", .num_fields = 1, .fields = (TypeFieldInfo[]) {
        {"a", .type = TYPEID(8, TYPE_INT, int), .offset = offsetof(test1_Thing, a)},
    }},
    [62] = &(TypeInfo){TYPE_PTR, .size = sizeof(void *), .align = alignof(void *), .base = TYPEID(61, TYPE_STRUCT, test1_Thing)},
    [63] = NULL, // Func
    [64] = &(TypeInfo){TYPE_CONST, .size = sizeof(test1_Thing const ), .align = alignof(test1_Thing const ), .base = TYPEID(61, TYPE_STRUCT, test1_Thing)},
    [65] = &(TypeInfo){TYPE_PTR, .size = sizeof(void *), .align = alignof(void *), .base = TYPEID(64, TYPE_CONST, test1_Thing const )},
    [66] = NULL, // Func
    [67] = &(TypeInfo){TYPE_STRUCT, .size = sizeof(test1_BufHdr), .align = alignof(test1_BufHdr), .name = "test1_BufHdr", .num_fields = 3, .fields = (TypeFieldInfo[]) {
        {"cap", .type = TYPEID(13, TYPE_ULLONG, ullong), .offset = offsetof(test1_BufHdr, cap)},
        {"len", .type = TYPEID(13, TYPE_ULLONG, ullong), .offset = offsetof(test1_BufHdr, len)},
        {"buf", .type = TYPEID(68, TYPE_ARRAY, char [1]), .offset = offsetof(test1_BufHdr, buf)},
    }},
    [68] = &(TypeInfo){TYPE_ARRAY, .size = sizeof(char [1]), .align = alignof(char [1]), .base = TYPEID(3, TYPE_CHAR, char), .count = 1},
    [69] = &(TypeInfo){TYPE_STRUCT, .size = sizeof(test1_Vector), .align = alignof(test1_Vector), .name = "test1_Vector", .num_fields = 2, .fields = (TypeFieldInfo[]) {
        {"x", .type = TYPEID(8, TYPE_INT, int), .offset = offsetof(test1_Vector, x)},
        {"y", .type = TYPEID(8, TYPE_INT, int), .offset = offsetof(test1_Vector, y)},
    }},
    [70] = &(TypeInfo){TYPE_PTR, .size = sizeof(void *), .align = alignof(void *), .base = TYPEID(69, TYPE_STRUCT, test1_Vector)},
    [71] = NULL, // Incomplete array type
    [72] = &(TypeInfo){TYPE_ARRAY, .size = sizeof(int [3]), .align = alignof(int [3]), .base = TYPEID(8, TYPE_INT, int), .count = 3},
    [73] = &(TypeInfo){TYPE_CONST, .size = sizeof(Any const ), .align = alignof(Any const ), .base = TYPEID(35, TYPE_STRUCT, Any)},
    [74] = &(TypeInfo){TYPE_STRUCT, .size = sizeof(test1_Ints), .align = alignof(test1_Ints), .name = "test1_Ints", .num_fields = 3, .fields = (TypeFieldInfo[]) {
        {"num_ints", .type = TYPEID(8, TYPE_INT, int), .offset = offsetof(test1_Ints, num_ints)},
        {"int_ptr", .type = TYPEID(55, TYPE_PTR, int *), .offset = offsetof(test1_Ints, int_ptr)},
        {"int_arr", .type = TYPEID(72, TYPE_ARRAY, int [3]), .offset = offsetof(test1_Ints, int_arr)},
    }},
    [75] = NULL, // Incomplete array type
    [76] = &(TypeInfo){TYPE_ARRAY, .size = sizeof(int [2]), .align = alignof(int [2]), .base = TYPEID(8, TYPE_INT, int), .count = 2},
    [77] = &(TypeInfo){TYPE_ARRAY, .size = sizeof(test1_Ints [2]), .align = alignof(test1_Ints [2]), .base = TYPEID(74, TYPE_STRUCT, test1_Ints), .count = 2},
    [78] = NULL, // Enum
    [79] = &(TypeInfo){TYPE_ARRAY, .size = sizeof(char [13]), .align = alignof(char [13]), .base = TYPEID(3, TYPE_CHAR, char), .count = 13},
    [80] = NULL, // Enum
    [81] = NULL, // Incomplete array type
    [82] = &(TypeInfo){TYPE_ARRAY, .size = sizeof(uint [3]), .align = alignof(uint [3]), .base = TYPEID(9, TYPE_UINT, uint), .count = 3},
    [83] = NULL, // Func
    [84] = &(TypeInfo){TYPE_PTR, .size = sizeof(void *), .align = alignof(void *), .base = TYPEID(9, TYPE_UINT, uint)},
    [85] = &(TypeInfo){TYPE_CONST, .size = sizeof(int const ), .align = alignof(int const ), .base = TYPEID(8, TYPE_INT, int)},
    [86] = &(TypeInfo){TYPE_ARRAY, .size = sizeof(int [4]), .align = alignof(int [4]), .base = TYPEID(8, TYPE_INT, int), .count = 4},
    [87] = &(TypeInfo){TYPE_STRUCT, .size = sizeof(test1_ConstVector), .align = alignof(test1_ConstVector), .name = "test1_ConstVector", .num_fields = 2, .fields = (TypeFieldInfo[]) {
        {"x", .type = TYPEID(85, TYPE_CONST, int const ), .offset = offsetof(test1_ConstVector, x)},
        {"y", .type = TYPEID(85, TYPE_CONST, int const ), .offset = offsetof(test1_ConstVector, y)},
    }},
    [88] = &(TypeInfo){TYPE_CONST, .size = sizeof(test1_Vector const ), .align = alignof(test1_Vector const ), .base = TYPEID(69, TYPE_STRUCT, test1_Vector)},
    [89] = &(TypeInfo){TYPE_ARRAY, .size = sizeof(char const [256]), .align = alignof(char const [256]), .base = TYPEID(20, TYPE_CONST, char const ), .count = 256},
    [90] = &(TypeInfo){TYPE_ARRAY, .size = sizeof(char [27]), .align = alignof(char [27]), .base = TYPEID(3, TYPE_CHAR, char), .count = 27},
    [91] = NULL, // Func
    [92] = NULL, // Func
    [93] = &(TypeInfo){TYPE_PTR, .size = sizeof(void *), .align = alignof(void *), .base = TYPEID(14, TYPE_FLOAT, float)},
    [94] = &(TypeInfo){TYPE_PTR, .size = sizeof(void *), .align = alignof(void *), .base = TYPEID(16, TYPE_PTR, void *)},
    [95] = NULL, // Func
    [96] = &(TypeInfo){TYPE_PTR, .size = sizeof(void *), .align = alignof(void *), .base = TYPEID(85, TYPE_CONST, int const )},
    [97] = &(TypeInfo){TYPE_ARRAY, .size = sizeof(int const (*[42])), .align = alignof(int const (*[42])), .base = TYPEID(96, TYPE_PTR, int const (*)), .count = 42},
    [98] = &(TypeInfo){TYPE_STRUCT, .size = sizeof(test1_UartCtrl), .align = alignof(test1_UartCtrl), .name = "test1_UartCtrl", .num_fields = 2, .fields = (TypeFieldInfo[]) {
        {"tx_enable", .type = TYPEID(2, TYPE_BOOL, bool), .offset = offsetof(test1_UartCtrl, tx_enable)},
        {"rx_enable", .type = TYPEID(2, TYPE_BOOL, bool), .offset = offsetof(test1_UartCtrl, rx_enable)},
    }},
    [99] = &(TypeInfo){TYPE_UNION, .size = sizeof(test1_IntOrPtr), .align = alignof(test1_IntOrPtr), .name = "test1_IntOrPtr", .num_fields = 2, .fields = (TypeFieldInfo[]) {
        {"i", .type = TYPEID(8, TYPE_INT, int), .offset = offsetof(test1_IntOrPtr, i)},
        {"p", .type = TYPEID(55, TYPE_PTR, int *), .offset = offsetof(test1_IntOrPtr, p)},
    }},
    [100] = &(TypeInfo){TYPE_PTR, .size = sizeof(void *), .align = alignof(void *), .base = TYPEID(99, TYPE_UNION, test1_IntOrPtr)},
    [101] = &(TypeInfo){TYPE_ARRAY, .size = sizeof(char [2]), .align = alignof(char [2]), .base = TYPEID(3, TYPE_CHAR, char), .count = 2},
    [102] = &(TypeInfo){TYPE_ARRAY, .size = sizeof(char [3]), .align = alignof(char [3]), .base = TYPEID(3, TYPE_CHAR, char), .count = 3},
    [103] = &(TypeInfo){TYPE_CONST, .size = sizeof(float const ), .align = alignof(float const ), .base = TYPEID(14, TYPE_FLOAT, float)},
    [104] = &(TypeInfo){TYPE_PTR, .size = sizeof(void *), .align = alignof(void *), .base = TYPEID(103, TYPE_CONST, float const )},
    [105] = &(TypeInfo){TYPE_ARRAY, .size = sizeof(char [10]), .align = alignof(char [10]), .base = TYPEID(3, TYPE_CHAR, char), .count = 10},
    [106] = &(TypeInfo){TYPE_CONST, .size = sizeof(TypeKind const ), .align = alignof(TypeKind const ), .base = TYPEID(24, TYPE_NONE, TypeKind)},
    [107] = &(TypeInfo){TYPE_CONST, .size = sizeof(ullong const ), .align = alignof(ullong const ), .base = TYPEID(13, TYPE_ULLONG, ullong)},
    [108] = &(TypeInfo){TYPE_ARRAY, .size = sizeof(char [7]), .align = alignof(char [7]), .base = TYPEID(3, TYPE_CHAR, char), .count = 7},
    [109] = &(TypeInfo){TYPE_ARRAY, .size = sizeof(char [5]), .align = alignof(char [5]), .base = TYPEID(3, TYPE_CHAR, char), .count = 5},
    [110] = &(TypeInfo){TYPE_CONST, .size = sizeof(char const ((*const ))), .align = alignof(char const ((*const ))), .base = TYPEID(21, TYPE_PTR, char const (*))},
    [111] = &(TypeInfo){TYPE_ARRAY, .size = sizeof(char [18]), .align = alignof(char [18]), .base = TYPEID(3, TYPE_CHAR, char), .count = 18},
    [112] = &(TypeInfo){TYPE_CONST, .size = sizeof(TypeFieldInfo (*const )), .align = alignof(TypeFieldInfo (*const )), .base = TYPEID(27, TYPE_PTR, TypeFieldInfo *)},
    [113] = &(TypeInfo){TYPE_ARRAY, .size = sizeof(char [17]), .align = alignof(char [17]), .base = TYPEID(3, TYPE_CHAR, char), .count = 17},
    [114] = &(TypeInfo){TYPE_ARRAY, .size = sizeof(char [11]), .align = alignof(char [11]), .base = TYPEID(3, TYPE_CHAR, char), .count = 11},
    [115] = NULL, // No associated type
    [116] = NULL, // No associated type
    [117] = NULL, // Func
    [118] = NULL, // Func
    [119] = NULL, // Func
    [120] = NULL, // Func
    [121] = &(TypeInfo){TYPE_ARRAY, .size = sizeof(char [103]), .align = alignof(char [103]), .base = TYPEID(3, TYPE_CHAR, char), .count = 103},
    [122] = NULL, // No associated type
    [123] = NULL, // No associated type
    [124] = NULL, // Func
    [125] = NULL, // Func
    [126] = NULL, // Func
    [127] = NULL, // Func
    [128] = NULL, // Func
    [129] = &(TypeInfo){TYPE_ARRAY, .size = sizeof(int [11]), .align = alignof(int [11]), .base = TYPEID(8, TYPE_INT, int), .count = 11},
    [130] = NULL, // Func
    [131] = NULL, // Func
    [132] = NULL, // No associated type
    [133] = NULL, // No associated type
    [134] = &(TypeInfo){TYPE_ARRAY, .size = sizeof(char const (*[4])), .align = alignof(char const (*[4])), .base = TYPEID(21, TYPE_PTR, char const (*)), .count = 4},
    [135] = NULL, // Func
    [136] = NULL, // Func
    [137] = NULL, // Func
    [138] = NULL, // Func
    [139] = NULL, // Func
    [140] = NULL, // Func
    [141] = NULL, // Func
    [142] = NULL, // Func
    [143] = NULL, // Func
    [144] = NULL, // Func
    [145] = NULL, // Func
    [146] = NULL, // Func
    [147] = NULL, // Func
    [148] = NULL, // Func
    [149] = &(TypeInfo){TYPE_PTR, .size = sizeof(void *), .align = alignof(void *), .base = TYPEID(15, TYPE_DOUBLE, double)},
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
    [164] = NULL, // No associated type
    [165] = NULL, // No associated type
    [166] = NULL, // Func
    [167] = NULL, // Func
    [168] = NULL, // Func
    [169] = NULL, // Func
    [170] = NULL, // Func
    [171] = NULL, // Func
    [172] = NULL, // Func
    [173] = NULL, // Func
    [174] = NULL, // Func
    [175] = NULL, // Func
    [176] = NULL, // Func
    [177] = NULL, // Func
    [178] = NULL, // Func
    [179] = NULL, // Func
    [180] = NULL, // Func
    [181] = NULL, // Func
    [182] = NULL, // Func
    [183] = NULL, // Func
    [184] = NULL, // Func
    [185] = NULL, // Func
    [186] = NULL, // Func
    [187] = NULL, // Func
    [188] = NULL, // Func
    [189] = NULL, // No associated type
    [190] = NULL, // Func
    [191] = NULL, // Func
    [192] = NULL, // Func
    [193] = NULL, // Func
    [194] = NULL, // No associated type
    [195] = NULL, // No associated type
    [196] = NULL, // No associated type
    [197] = NULL, // Func
    [198] = NULL, // Func
    [199] = NULL, // Func
    [200] = NULL, // Func
    [201] = NULL, // Func
    [202] = NULL, // Func
    [203] = NULL, // Func
    [204] = NULL, // Func
    [205] = NULL, // Func
    [206] = NULL, // Func
    [207] = NULL, // Func
    [208] = NULL, // Func
    [209] = NULL, // Func
    [210] = NULL, // Func
    [211] = NULL, // Func
    [212] = NULL, // Func
    [213] = NULL, // Func
    [214] = NULL, // Func
    [215] = NULL, // Func
    [216] = NULL, // Func
    [217] = NULL, // Func
    [218] = NULL, // Func
    [219] = NULL, // Func
    [220] = NULL, // Func
    [221] = &(TypeInfo){TYPE_PTR, .size = sizeof(void *), .align = alignof(void *), .base = TYPEID(6, TYPE_SHORT, short)},
    [222] = NULL, // Func
    [223] = NULL, // Func
    [224] = NULL, // Func
    [225] = &(TypeInfo){TYPE_CONST, .size = sizeof(short const ), .align = alignof(short const ), .base = TYPEID(6, TYPE_SHORT, short)},
    [226] = &(TypeInfo){TYPE_PTR, .size = sizeof(void *), .align = alignof(void *), .base = TYPEID(225, TYPE_CONST, short const )},
    [227] = NULL, // Func
    [228] = NULL, // Func
    [229] = NULL, // Func
    [230] = NULL, // Func
    [231] = NULL, // Func
    [232] = NULL, // Func
    [233] = NULL, // Func
    [234] = NULL, // Func
    [235] = NULL, // Func
    [236] = NULL, // Func
    [237] = NULL, // Func
    [238] = NULL, // Func
    [239] = NULL, // Func
    [240] = NULL, // Func
    [241] = NULL, // No associated type
    [242] = NULL, // Func
    [243] = NULL, // Func
    [244] = NULL, // No associated type
    [245] = NULL, // Func
    [246] = &(TypeInfo){TYPE_PTR, .size = sizeof(void *), .align = alignof(void *), .base = TYPEID(10, TYPE_LONG, long)},
    [247] = NULL, // Func
    [248] = NULL, // No associated type
    [249] = NULL, // No associated type
    [250] = NULL, // Func
    [251] = &(TypeInfo){TYPE_CONST, .size = sizeof(long const ), .align = alignof(long const ), .base = TYPEID(10, TYPE_LONG, long)},
    [252] = &(TypeInfo){TYPE_PTR, .size = sizeof(void *), .align = alignof(void *), .base = TYPEID(251, TYPE_CONST, long const )},
    [253] = NULL, // Func
    [254] = NULL, // Func
    [255] = NULL, // Func
    [256] = &(TypeInfo){TYPE_ARRAY, .size = sizeof(int [9]), .align = alignof(int [9]), .base = TYPEID(8, TYPE_INT, int), .count = 9},
    [257] = &(TypeInfo){TYPE_ARRAY, .size = sizeof(char [15]), .align = alignof(char [15]), .base = TYPEID(3, TYPE_CHAR, char), .count = 15},
};

int num_typeinfos = 258;
const TypeInfo **typeinfos = (const TypeInfo **)typeinfo_table;

// Definitions
int main(int argc, char (*(*argv))) {
    if ((argv) == (0)) {
        printf("argv is null\n");
    }
    test1_test_const_ptr_deref();
    test1_test_limits();
    test1_test_va_list("whatever", (char)(123), (int)(123123), (llong)(123123123123));
    test1_test_os_arch();
    test1_test_packages();
    test1_test_if();
    test1_test_modify();
    test1_test_lvalue();
    test1_test_alignof();
    test1_test_offsetof();
    test1_test_complete();
    test1_test_compound_literals();
    test1_test_loops();
    test1_test_sizeof();
    test1_test_assign();
    test1_test_enum();
    test1_test_arrays();
    test1_test_cast();
    test1_test_init();
    test1_test_lits();
    test1_test_const();
    test1_test_bool();
    test1_test_ops();
    test1_test_typeinfo();
    test1_test_reachable();
    getchar();
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

void test1_test_const_ptr_deref(void) {
    test1_Bar bar = {.rc = 42};
    test1_Foo foo = {.bar = &(bar)};
    int i = foo.bar->rc;
}

void test1_test_limits(void) {
    char char_min = CHAR_MIN;
    char char_max = CHAR_MAX;
    schar schar_min = SCHAR_MIN;
    schar schar_max = SCHAR_MAX;
    uchar uchar_min = UCHAR_MIN;
    uchar uchar_max = UCHAR_MAX;
    short short_min = SHRT_MIN;
    short short_max = SHRT_MAX;
    short ushort_min = USHORT_MIN;
    ushort ushort_max = USHRT_MAX;
    int int_min = INT_MIN;
    int int_max = INT_MAX;
    uint uint_min = UINT_MIN;
    uint uint_max = UINT_MAX;
    long long_min = LONG_MIN;
    long long_max = LONG_MAX;
    ulong ulong_min = ULONG_MIN;
    ulong ulong_max = ULONG_MAX;
    llong llong_min = LLONG_MIN;
    llong llong_max = LLONG_MAX;
    ullong ullong_min = ULLONG_MIN;
    ullong ullong_max = ULLONG_MAX;
    uint wchar_min = WCHAR_MIN;
    uint wchar_max = WCHAR_MAX;
    schar int8_min = INT8_MIN;
    schar int8_max = INT8_MAX;
    uchar uint8_min = UINT8_MIN;
    uchar uint8_max = UINT8_MAX;
    short int16_min = INT16_MIN;
    short int16_max = INT16_MAX;
    short uint16_min = UINT16_MIN;
    ushort uint16_max = UINT16_MAX;
    int int32_min = INT32_MIN;
    int int32_max = INT32_MAX;
    uint uint32_min = UINT32_MIN;
    uint uint32_max = UINT32_MAX;
    llong int64_min = INT64_MIN;
    llong int64_max = INT64_MAX;
    ullong uint64_min = UINT64_MIN;
    ullong uint64_max = UINT64_MAX;
    ullong usize_min = USIZE_MIN;
    ullong usize_max = SIZE_MAX;
    llong ssize_min = PTRDIFF_MIN;
    llong ssize_max = PTRDIFF_MAX;
    ullong uintptr_min = UINTPTR_MIN;
    ullong uintptr_max = UINTPTR_MAX;
    llong intptr_min = INTPTR_MIN;
    llong intptr_max = INTPTR_MAX;
}

void test1_test_va_list(char const ((*fmt)), ...) {
    va_list init_args = {0};
    va_start_ptr(&(init_args), &(fmt));
    va_list args = {0};
    va_copy_ptr(&(args), &(init_args));
    va_end_ptr(&(init_args));
    char c = {0};
    va_arg_ptr(&(args), (Any){&(c), TYPEID(3, TYPE_CHAR, char)});
    int i = {0};
    va_arg_ptr(&(args), (Any){&(i), TYPEID(8, TYPE_INT, int)});
    llong ll = {0};
    va_arg_ptr(&(args), (Any){&(ll), TYPEID(12, TYPE_LLONG, llong)});
    printf("c=%d i=%d ll=%lld\n", c, i, ll);
    va_end_ptr(&(args));
}

void test1_test_os_arch(void) {
    printf("Target operating system: %s\n", IONOS);
    printf("Target machine architecture: %s\n", IONARCH);
}

void test1_test_packages(void) {
    test1_subtest1_func1();
}

void test1_test_if(void) {
    if (1) {
    }
    {
        int x = 42;
        if (x) {
        }
    }
    {
        int x = 42;
        if ((x) >= (0)) {
        }
    }
    {
        int x = 42;
        if ((x) >= (0)) {
        }
    }
}

void test1_test_modify(void) {
    int i = 42;
    int (*p) = &(i);
    (p)--;
    int x = *((p)++);
    assert((x) == (*(--(p))));
    ((*(p)))++;
    ((*(p)))--;
    int (stk[16]) = {0};
    int (*sp) = stk;
    *((sp)++) = 1;
    *((sp)++) = 2;
    x = *(--(sp));
    assert((x) == (2));
    x = *(--(sp));
    assert((x) == (1));
    assert((sp) == (stk));
}

void test1_test_lvalue(void) {
    test1_returns_ptr()->a = 5;
    test1_Thing const ((*p)) = test1_returns_ptr_to_const();
}

void test1_test_alignof(void) {
    int i = 42;
    ullong n1 = alignof(int);
    ullong n2 = alignof(int);
    ullong n3 = alignof(ullong);
    ullong n4 = alignof(int *);
}

void test1_test_offsetof(void) {
    ullong n = offsetof(test1_BufHdr, buf);
}

void test1_test_complete(void) {
    int x = 0;
    int y = 0;
    if ((x) == (0)) {
        y = 1;
    } else if ((x) == (1)) {
        y = 2;
    } else {
        assert("@complete if/elseif chain failed to handle case" && 0);
    }
    x = 1;
    assert((x) >= (0));
    x = 0;
    switch (x) {
    case 0: {
        y = 3;
        break;
    }
    case 1: {
        y = 4;
        break;
    }
    default:
        assert("@complete switch failed to handle case" && 0);
        break;
    }
}

void test1_test_compound_literals(void) {
    test1_Vector (*w) = {0};
    w = &((test1_Vector){1, 2});
    int (a[3]) = {1, 2, 3};
    int i = 42;
    Any const (x) = {&(i), TYPEID(8, TYPE_INT, int)};
    Any y = {&(i), TYPEID(8, TYPE_INT, int)};
    test1_Ints v = {.num_ints = 3, .int_ptr = (int []){1, 2, 3}, .int_arr = {1, 2, 3}};
    test1_Ints (ints_of_ints[]) = {{.num_ints = 3, .int_arr = {1, 2, 3}}, {.num_ints = 2, .int_ptr = (int [2]){-(1), -(2)}}};
}

void test1_test_loops(void) {
    switch (0) {
    case 1: {
        break;
        break;
    }
    default: {
        if (1) {
            break;
        }
        for (;;) {
            continue;
        }
        break;
    }
    }
    while (0) {
    }
    for (int i = 0; (i) < (10); (i)++) {
    }
    for (;;) {
        break;
    }
    for (int i = 0;;) {
        break;
    }
    for (; 0;) {
    }
    for (int i = 0;; (i)++) {
        break;
    }
    int i = 0;
    for (;; (i)++) {
        break;
    }
}

void test1_test_sizeof(void) {
    int i = 0;
    ullong n = sizeof(i);
    n = sizeof(int);
    n = sizeof(int);
    n = sizeof(int *);
}

void test1_test_assign(void) {
    int i = 0;
    float f = 3.14f;
    int (*p) = &(i);
    (i)++;
    (i)--;
    (p)++;
    (p)--;
    p += 1;
    i /= 2;
    i *= 123;
    i %= 3;
    i <<= 1;
    i >>= 2;
    i &= 0xff;
    i |= 0xff00;
    i ^= 0xff0;
}

void test1_test_enum(void) {
    test1_Color a = TEST1_COLOR_RED;
    test1_Color b = TEST1_COLOR_RED;
    int c = (a) + (b);
    int i = a;
    a = i;
    printf("%d %d %d %d\n", TEST1_COLOR_NONE, TEST1_COLOR_RED, TEST1_COLOR_GREEN, TEST1_COLOR_BLUE);
    int d = TEST1_BAR;
    test1_TypedEnum e = TEST1_QUUX;
    test1_TypedEnum f = {0};
    f = TEST1_BAZ;
}

void test1_test_arrays(void) {
    wchar_t (a[]) = {1, 2, 3};
    test1_f10(a);
    uint (*b) = a;
    wchar_t w1 = {0};
    uint w2 = w1;
}

void test1_test_cast(void) {
    int (*p) = 0;
    uint64_t a = 0;
    a = (uint64_t)(p);
    p = (int *)(a);
}

void test1_test_init(void) {
    int x = (int const )(0);
    int y = {0};
    y = 0;
    int z = 42;
    int (a[]) = {1, 2, 3};
    for (ullong i = 0; (i) < (10); (i)++) {
        printf("%llu\n", i);
    }
    int (b[4]) = {1, 2, 3, 4};
    b[0] = a[2];
}

void test1_test_lits(void) {
    float f = 3.14f;
    double d = 3.14;
    int i = 1;
    uint u = 0xffffffffu;
    long l = 1l;
    ulong ul = 1ul;
    llong ll = 0x100000000ll;
    ullong ull = 0xffffffffffffffffull;
    uint x1 = 0xffffffff;
    long x2 = 4294967295;
    ulong x3 = 0xffffffffffffffff;
    int x4 = (0xaa) + (0x55);
}

void test1_test_const(void) {
    test1_ConstVector cv2 = {1, 2};
    int i = 0;
    i = 1;
    int x = test1_cv.x;
    char c = test1_escape_to_char[0];
    c = test1_char_to_escape[c];
    c = test1_esc_test_str[0];
    test1_f4(test1_escape_to_char);
    char const ((*p)) = (char const (*))(0);
    p = (test1_escape_to_char) + (1);
    char (*q) = (char *)(test1_escape_to_char);
    c = q['n'];
    p = (char const (*))(1);
    i = (int)((ullong)(p));
}

void test1_test_bool(void) {
    bool b = false;
    b = true;
    int i = 0;
    i = TEST1_IS_DEBUG;
}

void test1_test_ops(void) {
    float pi = 3.14f;
    float f = 0.0f;
    f = +(pi);
    f = -(pi);
    int n = -(1);
    n = ~(n);
    f = ((f) * (pi)) + (n);
    f = (pi) / (pi);
    n = (3) % (2);
    n = (n) + ((uchar)(1));
    int (*p) = &(n);
    p = (p) + (1);
    n = (int)((((p) + (1))) - (p));
    n = (n) << (1);
    n = (n) >> (1);
    int b = ((p) + (1)) > (p);
    b = ((p) + (1)) >= (p);
    b = ((p) + (1)) < (p);
    b = ((p) + (1)) <= (p);
    b = ((p) + (1)) == (p);
    b = (1) > (2);
    b = (1.23f) <= (pi);
    n = 0xff;
    b = (n) & (~(1));
    b = (n) & (1);
    b = (((n) & (~(1)))) ^ (1);
    b = (p) && (pi);
}

void test1_test_typeinfo(void) {
    int i = 42;
    float f = 3.14f;
    void (*p) = NULL;
    test1_println_any((Any){&(i), TYPEID(8, TYPE_INT, int)});
    test1_println_any((Any){&(f), TYPEID(14, TYPE_FLOAT, float)});
    test1_println_any((Any){&(p), TYPEID(16, TYPE_PTR, void *)});
    test1_println_type(TYPEID(8, TYPE_INT, int));
    test1_println_type(TYPEID(96, TYPE_PTR, int const (*)));
    test1_println_type(TYPEID(97, TYPE_ARRAY, int const (*[42])));
    test1_println_type(TYPEID(98, TYPE_STRUCT, test1_UartCtrl));
    test1_println_typeinfo(TYPEID(8, TYPE_INT, int));
    test1_println_typeinfo(TYPEID(98, TYPE_STRUCT, test1_UartCtrl));
    test1_println_typeinfo(TYPEID(100, TYPE_PTR, test1_IntOrPtr *));
    test1_println_typeinfo(TYPEID(99, TYPE_UNION, test1_IntOrPtr));
}

void test1_test_reachable(void) {
}

int test1_subtest1_func1(void) {
    test1_subtest1_func2();
    return 42;
}

test1_Thing (*test1_returns_ptr(void)) {
    return &(test1_thing);
}

test1_Thing const ((*test1_returns_ptr_to_const(void))) {
    return &(test1_thing);
}

void test1_f10(wchar_t (a[3])) {
    a[1] = 42;
}

test1_Vector const (test1_cv);
char const ((test1_escape_to_char[256])) = {['0'] = '\0', ['\''] = '\'', ['\"'] = '\"', ['\\'] = '\\', ['n'] = '\n', ['r'] = '\r', ['t'] = '\t', ['v'] = '\v', ['b'] = '\b', ['a'] = '\a'};
char const ((test1_char_to_escape[256])) = {['\0'] = '0', ['\n'] = 'n', ['\r'] = 'r', ['\t'] = 't', ['\v'] = 'v', ['\b'] = 'b', ['\a'] = 'a', ['\\'] = '\\', ['\"'] = '\"', ['\''] = '\''};
char (*test1_esc_test_str) = "Hello\n"
"world\n"
"Hex: \x1\x10\xFHello\xFF";
void test1_f4(char const ((*x))) {
}

void test1_println_any(Any any) {
    test1_print_any(any);
    printf("\n");
}

void test1_println_type(typeid type) {
    test1_print_type(type);
    printf("\n");
}

void test1_println_typeinfo(typeid type) {
    test1_print_typeinfo(type);
    printf("\n");
}

void test1_subtest1_func2(void) {
    test1_subtest1_func3();
    test1_subtest1_func4();
}

test1_Thing test1_thing;
void test1_print_any(Any any) {
    switch (any.type) {
    case TYPEID(8, TYPE_INT, int): {
        printf("%d", *((int const (*))(any.ptr)));
        break;
    }
    case TYPEID(14, TYPE_FLOAT, float): {
        printf("%f", *((float const (*))(any.ptr)));
        break;
    }
    default: {
        printf("<unknown>");
        break;
    }
    }
    printf(": ");
    test1_print_type(any.type);
}

void test1_print_type(typeid type) {
    TypeInfo const ((*typeinfo)) = get_typeinfo(type);
    if (!(typeinfo)) {
        test1_print_typeid(type);
        return;
    }
    switch (typeinfo->kind) {
    case TYPE_PTR: {
        test1_print_type(typeinfo->base);
        printf("*");
        break;
    }
    case TYPE_CONST: {
        test1_print_type(typeinfo->base);
        printf(" const");
        break;
    }
    case TYPE_ARRAY: {
        test1_print_type(typeinfo->base);
        printf("[%d]", typeinfo->count);
        break;
    }
    default: {
        if (typeinfo->name) {
            printf("%s", typeinfo->name);
        } else {
            test1_print_typeid(type);
        }
        break;
    }
    }
}

void test1_print_typeinfo(typeid type) {
    TypeInfo const ((*typeinfo)) = get_typeinfo(type);
    if (!(typeinfo)) {
        test1_print_typeid(type);
        return;
    }
    printf("<");
    test1_print_type(type);
    printf(" size=%d align=%d", typeinfo->size, typeinfo->align);
    switch (typeinfo->kind) {
    case TYPE_STRUCT:
    case TYPE_UNION: {
        printf(" %s={ ", ((typeinfo->kind) == (TYPE_STRUCT) ? "struct" : "union"));
        for (int i = 0; (i) < (typeinfo->num_fields); (i)++) {
            TypeFieldInfo field = typeinfo->fields[i];
            printf("@offset(%d) %s: ", field.offset, field.name);
            test1_print_type(field.type);
            printf("; ");
        }
        printf("}");
        break;
    }
    }
    printf(">");
}

void test1_subtest1_func3(void) {
    printf("func3\n");
}

void test1_subtest1_func4(void) {
}

void test1_print_typeid(typeid type) {
    int index = typeid_index(type);
    printf("typeid(%d)", index);
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
