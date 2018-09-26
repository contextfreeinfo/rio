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
typedef struct test1_More test1_More;
typedef struct test1_ConstVector test1_ConstVector;
typedef struct test1_Slorce_int test1_Slorce_int;
typedef struct test1_Slirce test1_Slirce;
typedef struct test1_Slorce_ref_int test1_Slorce_ref_int;
typedef struct test1_Slorce_Bar test1_Slorce_Bar;
typedef struct test1_Slorce_Hey test1_Slorce_Hey;
typedef struct test1_Slorce_Slorce_Hey test1_Slorce_Slorce_Hey;
typedef struct test1_Result_float_TestErr test1_Result_float_TestErr;
typedef struct test1_UartCtrl test1_UartCtrl;
typedef union test1_IntOrPtr test1_IntOrPtr;

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

void test1_test_ref(void);

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

void test1_test_type_path(void);

void test1_test_generics(void);

void test1_test_err(void);

void test1_test_for_each(void);



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

#define test1_Color_None ((test1_Color)(0))

#define test1_Color_Red ((test1_Color)((test1_Color_None) + (1)))

#define test1_Color_Green ((test1_Color)((test1_Color_Red) + (1)))

#define test1_Color_Blue ((test1_Color)((test1_Color_Green) + (1)))

#define test1_Color_Num ((test1_Color)((test1_Color_Blue) + (1)))

extern char const ((*(test1_color_names[(test1_Color_Num)])));

#define test1_FOO ((int)(0))

#define test1_BAR ((int)((test1_FOO) + (1)))

typedef int8_t test1_TypedEnum;

#define test1_BAZ ((test1_TypedEnum)(0))

#define test1_QUUX ((test1_TypedEnum)((test1_BAZ) + (1)))

typedef int test1_More_Kind;

struct test1_More {
  test1_More_Kind kind;
  float prob;
  union {
    // void;
    float amount;
    int years;
    struct {
      char const ((*description));
      float value;
    };
  };
};

#define test1_More_None ((test1_More_Kind)(0))

#define test1_More_MoreNone ((test1_More_Kind)((test1_More_None) + (1)))

#define test1_More_Happy ((test1_More_Kind)((test1_More_MoreNone) + (1)))

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

#define test1_IS_DEBUG (true)

void test1_println_any(Any any);

void test1_println_type(typeid type);

void test1_println_typeinfo(typeid type);

typedef time_t test1_my_time_t;


typedef test1_Slorce_int test1_Slarce;

struct test1_Slorce_int {
  size_t length;
  int (*items);
};

struct test1_Slirce {
  test1_Slarce slarce;
};

struct test1_Slorce_ref_int {
  size_t length;
  int (*(*items));
};

struct test1_Slorce_Bar {
  size_t length;
  test1_Bar (*items);
};

typedef float test1_Hey;

struct test1_Slorce_Hey {
  size_t length;
  test1_Hey (*items);
};

struct test1_Slorce_Slorce_Hey {
  size_t length;
  test1_Slorce_Hey (*items);
};

void test1_test_generic_functions(void);

typedef int test1_TestErr;

struct test1_Result_float_TestErr {
  Result_Kind kind;
  union {
    float val;
    test1_TestErr err;
  };
};

test1_Result_float_TestErr test1_maybe_math(float x);

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

#define test1_Ptr (959595)

// Typeinfo
#define TYPEID0(index, kind) ((ullong)(index) | ((ullong)(kind) << 24))
#define TYPEID(index, kind, ...) ((ullong)(index) | ((ullong)sizeof(__VA_ARGS__) << 32) | ((ullong)(kind) << 24))

const TypeInfo *typeinfo_table[301] = {
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
  [41] = &(TypeInfo){TypeKind_Array, .size = sizeof(char [14]), .align = alignof(char [14]), .base = TYPEID(3, TypeKind_Char, char), .count = 14},
  [42] = NULL, // Function
  [43] = NULL, // Function
  [44] = &(TypeInfo){TypeKind_Array, .size = sizeof(char [9]), .align = alignof(char [9]), .base = TYPEID(3, TypeKind_Char, char), .count = 9},
  [45] = NULL, // No associated type
  [46] = &(TypeInfo){TypeKind_Ref, .size = sizeof(void *), .align = alignof(void *), .base = TYPEID(8, TypeKind_Int, int)},
  [47] = &(TypeInfo){TypeKind_Ptr, .size = sizeof(void *), .align = alignof(void *), .base = TYPEID(8, TypeKind_Int, int)},
  [48] = &(TypeInfo){TypeKind_Struct, .size = sizeof(test1_Bar), .align = alignof(test1_Bar), .name = "test1_Bar", .num_fields = 1, .fields = (TypeFieldInfo[]) {
    {"rc", .type = TYPEID(8, TypeKind_Int, int), .offset = offsetof(test1_Bar, rc)},
  }},
  [49] = &(TypeInfo){TypeKind_Struct, .size = sizeof(test1_Foo), .align = alignof(test1_Foo), .name = "test1_Foo", .num_fields = 1, .fields = (TypeFieldInfo[]) {
    {"bar", .type = TYPEID(51, TypeKind_Const, test1_Bar (*const )), .offset = offsetof(test1_Foo, bar)},
  }},
  [50] = &(TypeInfo){TypeKind_Ptr, .size = sizeof(void *), .align = alignof(void *), .base = TYPEID(48, TypeKind_Struct, test1_Bar)},
  [51] = &(TypeInfo){TypeKind_Const, .size = sizeof(test1_Bar (*const )), .align = alignof(test1_Bar (*const )), .base = TYPEID(50, TypeKind_Ptr, test1_Bar *)},
  [52] = &(TypeInfo){TypeKind_Ref, .size = sizeof(void *), .align = alignof(void *), .base = TYPEID(48, TypeKind_Struct, test1_Bar)},
  [53] = &(TypeInfo){TypeKind_Const, .size = 0, .align = 0, .base = TYPEID0(1, TypeKind_Void)},
  [54] = &(TypeInfo){TypeKind_Ref, .size = sizeof(void *), .align = alignof(void *), .base = TYPEID0(53, TypeKind_Const)},
  [55] = NULL, // Function
  [56] = &(TypeInfo){TypeKind_Ref, .size = sizeof(void *), .align = alignof(void *), .base = TYPEID(17, TypeKind_Ptr, char *)},
  [57] = &(TypeInfo){TypeKind_Ref, .size = sizeof(void *), .align = alignof(void *), .base = TYPEID(24, TypeKind_Ptr, char const (*))},
  [58] = &(TypeInfo){TypeKind_Const, .size = sizeof(char (*const )), .align = alignof(char (*const )), .base = TYPEID(17, TypeKind_Ptr, char *)},
  [59] = &(TypeInfo){TypeKind_Ptr, .size = sizeof(void *), .align = alignof(void *), .base = TYPEID(58, TypeKind_Const, char (*const ))},
  [60] = NULL, // Function
  [61] = NULL, // Function
  [62] = NULL, // Function
  [63] = &(TypeInfo){TypeKind_Ref, .size = sizeof(void *), .align = alignof(void *), .base = TYPEID(3, TypeKind_Char, char)},
  [64] = &(TypeInfo){TypeKind_Ref, .size = sizeof(void *), .align = alignof(void *), .base = TYPEID(12, TypeKind_LLong, llong)},
  [65] = &(TypeInfo){TypeKind_Array, .size = sizeof(char [19]), .align = alignof(char [19]), .base = TYPEID(3, TypeKind_Char, char), .count = 19},
  [66] = &(TypeInfo){TypeKind_Array, .size = sizeof(char [29]), .align = alignof(char [29]), .base = TYPEID(3, TypeKind_Char, char), .count = 29},
  [67] = &(TypeInfo){TypeKind_Array, .size = sizeof(char [33]), .align = alignof(char [33]), .base = TYPEID(3, TypeKind_Char, char), .count = 33},
  [68] = NULL, // Function
  [69] = &(TypeInfo){TypeKind_Array, .size = sizeof(int [16]), .align = alignof(int [16]), .base = TYPEID(8, TypeKind_Int, int), .count = 16},
  [70] = &(TypeInfo){TypeKind_Struct, .size = sizeof(test1_Thing), .align = alignof(test1_Thing), .name = "test1_Thing", .num_fields = 1, .fields = (TypeFieldInfo[]) {
    {"a", .type = TYPEID(8, TypeKind_Int, int), .offset = offsetof(test1_Thing, a)},
  }},
  [71] = &(TypeInfo){TypeKind_Ref, .size = sizeof(void *), .align = alignof(void *), .base = TYPEID(70, TypeKind_Struct, test1_Thing)},
  [72] = NULL, // Function
  [73] = &(TypeInfo){TypeKind_Const, .size = sizeof(test1_Thing const ), .align = alignof(test1_Thing const ), .base = TYPEID(70, TypeKind_Struct, test1_Thing)},
  [74] = &(TypeInfo){TypeKind_Ref, .size = sizeof(void *), .align = alignof(void *), .base = TYPEID(73, TypeKind_Const, test1_Thing const )},
  [75] = NULL, // Function
  [76] = &(TypeInfo){TypeKind_Struct, .size = sizeof(test1_BufHdr), .align = alignof(test1_BufHdr), .name = "test1_BufHdr", .num_fields = 3, .fields = (TypeFieldInfo[]) {
    {"cap", .type = TYPEID(13, TypeKind_ULLong, ullong), .offset = offsetof(test1_BufHdr, cap)},
    {"len", .type = TYPEID(13, TypeKind_ULLong, ullong), .offset = offsetof(test1_BufHdr, len)},
    {"buf", .type = TYPEID(77, TypeKind_Array, char [1]), .offset = offsetof(test1_BufHdr, buf)},
  }},
  [77] = &(TypeInfo){TypeKind_Array, .size = sizeof(char [1]), .align = alignof(char [1]), .base = TYPEID(3, TypeKind_Char, char), .count = 1},
  [78] = &(TypeInfo){TypeKind_Struct, .size = sizeof(test1_Vector), .align = alignof(test1_Vector), .name = "test1_Vector", .num_fields = 2, .fields = (TypeFieldInfo[]) {
    {"x", .type = TYPEID(8, TypeKind_Int, int), .offset = offsetof(test1_Vector, x)},
    {"y", .type = TYPEID(8, TypeKind_Int, int), .offset = offsetof(test1_Vector, y)},
  }},
  [79] = &(TypeInfo){TypeKind_Ptr, .size = sizeof(void *), .align = alignof(void *), .base = TYPEID(78, TypeKind_Struct, test1_Vector)},
  [80] = &(TypeInfo){TypeKind_Ref, .size = sizeof(void *), .align = alignof(void *), .base = TYPEID(78, TypeKind_Struct, test1_Vector)},
  [81] = NULL, // Incomplete array type
  [82] = &(TypeInfo){TypeKind_Array, .size = sizeof(int [3]), .align = alignof(int [3]), .base = TYPEID(8, TypeKind_Int, int), .count = 3},
  [83] = &(TypeInfo){TypeKind_Const, .size = sizeof(Any const ), .align = alignof(Any const ), .base = TYPEID(38, TypeKind_Struct, Any)},
  [84] = &(TypeInfo){TypeKind_Struct, .size = sizeof(test1_Ints), .align = alignof(test1_Ints), .name = "test1_Ints", .num_fields = 3, .fields = (TypeFieldInfo[]) {
    {"num_ints", .type = TYPEID(8, TypeKind_Int, int), .offset = offsetof(test1_Ints, num_ints)},
    {"int_ptr", .type = TYPEID(47, TypeKind_Ptr, int *), .offset = offsetof(test1_Ints, int_ptr)},
    {"int_arr", .type = TYPEID(82, TypeKind_Array, int [3]), .offset = offsetof(test1_Ints, int_arr)},
  }},
  [85] = NULL, // Incomplete array type
  [86] = &(TypeInfo){TypeKind_Array, .size = sizeof(int [2]), .align = alignof(int [2]), .base = TYPEID(8, TypeKind_Int, int), .count = 2},
  [87] = &(TypeInfo){TypeKind_Array, .size = sizeof(test1_Ints [2]), .align = alignof(test1_Ints [2]), .base = TYPEID(84, TypeKind_Struct, test1_Ints), .count = 2},
  [88] = NULL, // Enum
  [89] = &(TypeInfo){TypeKind_Array, .size = sizeof(char [13]), .align = alignof(char [13]), .base = TYPEID(3, TypeKind_Char, char), .count = 13},
  [90] = &(TypeInfo){TypeKind_Array, .size = sizeof(char const (*[4])), .align = alignof(char const (*[4])), .base = TYPEID(24, TypeKind_Ptr, char const (*)), .count = 4},
  [91] = &(TypeInfo){TypeKind_Array, .size = sizeof(char [5]), .align = alignof(char [5]), .base = TYPEID(3, TypeKind_Char, char), .count = 5},
  [92] = &(TypeInfo){TypeKind_Ptr, .size = sizeof(void *), .align = alignof(void *), .base = TYPEID(24, TypeKind_Ptr, char const (*))},
  [93] = NULL, // Enum
  [94] = &(TypeInfo){TypeKind_Struct, .size = sizeof(test1_More), .align = alignof(test1_More), .name = "test1_More", .num_fields = 6, .fields = (TypeFieldInfo[]) {
    {"kind", .type = TYPEID(95, TypeKind_None, test1_More_Kind), .offset = offsetof(test1_More, kind)},
    {"prob", .type = TYPEID(14, TypeKind_Float, float), .offset = offsetof(test1_More, prob)},
    {"amount", .type = TYPEID(14, TypeKind_Float, float), .offset = offsetof(test1_More, amount)},
    {"years", .type = TYPEID(8, TypeKind_Int, int), .offset = offsetof(test1_More, years)},
    {"description", .type = TYPEID(24, TypeKind_Ptr, char const (*)), .offset = offsetof(test1_More, description)},
    {"value", .type = TYPEID(14, TypeKind_Float, float), .offset = offsetof(test1_More, value)},
  }},
  [95] = NULL, // Enum
  [96] = NULL, // No associated type
  [97] = NULL, // No associated type
  [98] = NULL, // Incomplete array type
  [99] = &(TypeInfo){TypeKind_Array, .size = sizeof(uint [3]), .align = alignof(uint [3]), .base = TYPEID(9, TypeKind_UInt, uint), .count = 3},
  [100] = NULL, // Function
  [101] = &(TypeInfo){TypeKind_Ptr, .size = sizeof(void *), .align = alignof(void *), .base = TYPEID(9, TypeKind_UInt, uint)},
  [102] = &(TypeInfo){TypeKind_Const, .size = sizeof(int const ), .align = alignof(int const ), .base = TYPEID(8, TypeKind_Int, int)},
  [103] = &(TypeInfo){TypeKind_Array, .size = sizeof(int [4]), .align = alignof(int [4]), .base = TYPEID(8, TypeKind_Int, int), .count = 4},
  [104] = &(TypeInfo){TypeKind_Struct, .size = sizeof(test1_ConstVector), .align = alignof(test1_ConstVector), .name = "test1_ConstVector", .num_fields = 2, .fields = (TypeFieldInfo[]) {
    {"x", .type = TYPEID(102, TypeKind_Const, int const ), .offset = offsetof(test1_ConstVector, x)},
    {"y", .type = TYPEID(102, TypeKind_Const, int const ), .offset = offsetof(test1_ConstVector, y)},
  }},
  [105] = &(TypeInfo){TypeKind_Const, .size = sizeof(test1_Vector const ), .align = alignof(test1_Vector const ), .base = TYPEID(78, TypeKind_Struct, test1_Vector)},
  [106] = &(TypeInfo){TypeKind_Array, .size = sizeof(char const [256]), .align = alignof(char const [256]), .base = TYPEID(23, TypeKind_Const, char const ), .count = 256},
  [107] = &(TypeInfo){TypeKind_Array, .size = sizeof(char [27]), .align = alignof(char [27]), .base = TYPEID(3, TypeKind_Char, char), .count = 27},
  [108] = NULL, // Function
  [109] = NULL, // Function
  [110] = &(TypeInfo){TypeKind_Ref, .size = sizeof(void *), .align = alignof(void *), .base = TYPEID(14, TypeKind_Float, float)},
  [111] = &(TypeInfo){TypeKind_Ref, .size = sizeof(void *), .align = alignof(void *), .base = TYPEID(39, TypeKind_Ptr, void *)},
  [112] = NULL, // Function
  [113] = &(TypeInfo){TypeKind_Ptr, .size = sizeof(void *), .align = alignof(void *), .base = TYPEID(102, TypeKind_Const, int const )},
  [114] = &(TypeInfo){TypeKind_Array, .size = sizeof(int const (*[42])), .align = alignof(int const (*[42])), .base = TYPEID(113, TypeKind_Ptr, int const (*)), .count = 42},
  [115] = &(TypeInfo){TypeKind_Struct, .size = sizeof(test1_UartCtrl), .align = alignof(test1_UartCtrl), .name = "test1_UartCtrl", .num_fields = 2, .fields = (TypeFieldInfo[]) {
    {"tx_enable", .type = TYPEID(2, TypeKind_Bool, bool), .offset = offsetof(test1_UartCtrl, tx_enable)},
    {"rx_enable", .type = TYPEID(2, TypeKind_Bool, bool), .offset = offsetof(test1_UartCtrl, rx_enable)},
  }},
  [116] = &(TypeInfo){TypeKind_Union, .size = sizeof(test1_IntOrPtr), .align = alignof(test1_IntOrPtr), .name = "test1_IntOrPtr", .num_fields = 2, .fields = (TypeFieldInfo[]) {
    {"i", .type = TYPEID(8, TypeKind_Int, int), .offset = offsetof(test1_IntOrPtr, i)},
    {"p", .type = TYPEID(47, TypeKind_Ptr, int *), .offset = offsetof(test1_IntOrPtr, p)},
  }},
  [117] = &(TypeInfo){TypeKind_Ptr, .size = sizeof(void *), .align = alignof(void *), .base = TYPEID(116, TypeKind_Union, test1_IntOrPtr)},
  [118] = &(TypeInfo){TypeKind_Ref, .size = sizeof(void *), .align = alignof(void *), .base = TYPEID(116, TypeKind_Union, test1_IntOrPtr)},
  [119] = &(TypeInfo){TypeKind_Ref, .size = sizeof(void *), .align = alignof(void *), .base = TYPEID(10, TypeKind_Long, long)},
  [120] = NULL, // Function
  [121] = &(TypeInfo){TypeKind_Struct, .size = sizeof(test1_Slirce), .align = alignof(test1_Slirce), .name = "test1_Slirce", .num_fields = 1, .fields = (TypeFieldInfo[]) {
    {"slarce", .type = TYPEID(123, TypeKind_Struct, test1_Slorce_int), .offset = offsetof(test1_Slirce, slarce)},
  }},
  [122] = &(TypeInfo){TypeKind_Struct, .size = 0, .align = 0, .name = "test1_Slorce", .num_fields = 2, .fields = (TypeFieldInfo[]) {
    {"length", .type = TYPEID(13, TypeKind_ULLong, ullong)},
    {"items", .type = TYPEID(39, TypeKind_Ptr, void *)},
  }},
  [123] = &(TypeInfo){TypeKind_Struct, .size = sizeof(test1_Slorce_int), .align = alignof(test1_Slorce_int), .name = "test1_Slorce_int", .num_fields = 2, .fields = (TypeFieldInfo[]) {
    {"length", .type = TYPEID(13, TypeKind_ULLong, ullong), .offset = offsetof(test1_Slorce_int, length)},
    {"items", .type = TYPEID(47, TypeKind_Ptr, int *), .offset = offsetof(test1_Slorce_int, items)},
  }},
  [124] = &(TypeInfo){TypeKind_Struct, .size = sizeof(test1_Slorce_ref_int), .align = alignof(test1_Slorce_ref_int), .name = "test1_Slorce_ref_int", .num_fields = 2, .fields = (TypeFieldInfo[]) {
    {"length", .type = TYPEID(13, TypeKind_ULLong, ullong), .offset = offsetof(test1_Slorce_ref_int, length)},
    {"items", .type = TYPEID(125, TypeKind_Ptr, int (**)), .offset = offsetof(test1_Slorce_ref_int, items)},
  }},
  [125] = &(TypeInfo){TypeKind_Ptr, .size = sizeof(void *), .align = alignof(void *), .base = TYPEID(46, TypeKind_Ref, int *)},
  [126] = NULL, // Incomplete array type
  [127] = &(TypeInfo){TypeKind_Array, .size = sizeof(int *[1]), .align = alignof(int *[1]), .base = TYPEID(46, TypeKind_Ref, int *), .count = 1},
  [128] = &(TypeInfo){TypeKind_Array, .size = sizeof(char [17]), .align = alignof(char [17]), .base = TYPEID(3, TypeKind_Char, char), .count = 17},
  [129] = &(TypeInfo){TypeKind_Struct, .size = sizeof(test1_Slorce_Bar), .align = alignof(test1_Slorce_Bar), .name = "test1_Slorce_Bar", .num_fields = 2, .fields = (TypeFieldInfo[]) {
    {"length", .type = TYPEID(13, TypeKind_ULLong, ullong), .offset = offsetof(test1_Slorce_Bar, length)},
    {"items", .type = TYPEID(50, TypeKind_Ptr, test1_Bar *), .offset = offsetof(test1_Slorce_Bar, items)},
  }},
  [130] = &(TypeInfo){TypeKind_Struct, .size = sizeof(test1_Slorce_Hey), .align = alignof(test1_Slorce_Hey), .name = "test1_Slorce_Hey", .num_fields = 2, .fields = (TypeFieldInfo[]) {
    {"length", .type = TYPEID(13, TypeKind_ULLong, ullong), .offset = offsetof(test1_Slorce_Hey, length)},
    {"items", .type = TYPEID(131, TypeKind_Ptr, float *), .offset = offsetof(test1_Slorce_Hey, items)},
  }},
  [131] = &(TypeInfo){TypeKind_Ptr, .size = sizeof(void *), .align = alignof(void *), .base = TYPEID(14, TypeKind_Float, float)},
  [132] = &(TypeInfo){TypeKind_Struct, .size = sizeof(test1_Slorce_Slorce_Hey), .align = alignof(test1_Slorce_Slorce_Hey), .name = "test1_Slorce_Slorce_Hey", .num_fields = 2, .fields = (TypeFieldInfo[]) {
    {"length", .type = TYPEID(13, TypeKind_ULLong, ullong), .offset = offsetof(test1_Slorce_Slorce_Hey, length)},
    {"items", .type = TYPEID(133, TypeKind_Ptr, test1_Slorce_Hey *), .offset = offsetof(test1_Slorce_Slorce_Hey, items)},
  }},
  [133] = &(TypeInfo){TypeKind_Ptr, .size = sizeof(void *), .align = alignof(void *), .base = TYPEID(130, TypeKind_Struct, test1_Slorce_Hey)},
  [134] = &(TypeInfo){TypeKind_Struct, .size = sizeof(test1_Result_float_TestErr), .align = alignof(test1_Result_float_TestErr), .name = "test1_Result_float_TestErr", .num_fields = 3, .fields = (TypeFieldInfo[]) {
    {"kind", .type = TYPEID(21, TypeKind_None, Result_Kind), .offset = offsetof(test1_Result_float_TestErr, kind)},
    {"val", .type = TYPEID(14, TypeKind_Float, float), .offset = offsetof(test1_Result_float_TestErr, val)},
    {"err", .type = TYPEID(135, TypeKind_None, test1_TestErr), .offset = offsetof(test1_Result_float_TestErr, err)},
  }},
  [135] = NULL, // Enum
  [136] = NULL, // No associated type
  [137] = NULL, // Function
  [138] = NULL, // Incomplete array type
  [139] = &(TypeInfo){TypeKind_Array, .size = sizeof(char const (*[3])), .align = alignof(char const (*[3])), .base = TYPEID(24, TypeKind_Ptr, char const (*)), .count = 3},
  [140] = &(TypeInfo){TypeKind_Array, .size = sizeof(char [12]), .align = alignof(char [12]), .base = TYPEID(3, TypeKind_Char, char), .count = 12},
  [141] = &(TypeInfo){TypeKind_Array, .size = sizeof(char [2]), .align = alignof(char [2]), .base = TYPEID(3, TypeKind_Char, char), .count = 2},
  [142] = &(TypeInfo){TypeKind_Array, .size = sizeof(char [3]), .align = alignof(char [3]), .base = TYPEID(3, TypeKind_Char, char), .count = 3},
  [143] = &(TypeInfo){TypeKind_Const, .size = sizeof(float const ), .align = alignof(float const ), .base = TYPEID(14, TypeKind_Float, float)},
  [144] = &(TypeInfo){TypeKind_Ptr, .size = sizeof(void *), .align = alignof(void *), .base = TYPEID(143, TypeKind_Const, float const )},
  [145] = &(TypeInfo){TypeKind_Array, .size = sizeof(char [10]), .align = alignof(char [10]), .base = TYPEID(3, TypeKind_Char, char), .count = 10},
  [146] = &(TypeInfo){TypeKind_Const, .size = sizeof(TypeKind const ), .align = alignof(TypeKind const ), .base = TYPEID(27, TypeKind_None, TypeKind)},
  [147] = &(TypeInfo){TypeKind_Array, .size = sizeof(char [23]), .align = alignof(char [23]), .base = TYPEID(3, TypeKind_Char, char), .count = 23},
  [148] = &(TypeInfo){TypeKind_Const, .size = sizeof(ullong const ), .align = alignof(ullong const ), .base = TYPEID(13, TypeKind_ULLong, ullong)},
  [149] = &(TypeInfo){TypeKind_Array, .size = sizeof(char [7]), .align = alignof(char [7]), .base = TYPEID(3, TypeKind_Char, char), .count = 7},
  [150] = &(TypeInfo){TypeKind_Const, .size = sizeof(char const ((*const ))), .align = alignof(char const ((*const ))), .base = TYPEID(24, TypeKind_Ptr, char const (*))},
  [151] = &(TypeInfo){TypeKind_Array, .size = sizeof(char [18]), .align = alignof(char [18]), .base = TYPEID(3, TypeKind_Char, char), .count = 18},
  [152] = &(TypeInfo){TypeKind_Const, .size = sizeof(TypeFieldInfo (*const )), .align = alignof(TypeFieldInfo (*const )), .base = TYPEID(30, TypeKind_Ptr, TypeFieldInfo *)},
  [153] = &(TypeInfo){TypeKind_Array, .size = sizeof(char [11]), .align = alignof(char [11]), .base = TYPEID(3, TypeKind_Char, char), .count = 11},
  [154] = NULL, // No associated type
  [155] = NULL, // No associated type
  [156] = NULL, // Function
  [157] = NULL, // Function
  [158] = NULL, // Function
  [159] = NULL, // Function
  [160] = &(TypeInfo){TypeKind_Array, .size = sizeof(char [99]), .align = alignof(char [99]), .base = TYPEID(3, TypeKind_Char, char), .count = 99},
  [161] = NULL, // No associated type
  [162] = NULL, // No associated type
  [163] = NULL, // Function
  [164] = NULL, // Function
  [165] = NULL, // Function
  [166] = NULL, // Function
  [167] = NULL, // Function
  [168] = &(TypeInfo){TypeKind_Array, .size = sizeof(int [11]), .align = alignof(int [11]), .base = TYPEID(8, TypeKind_Int, int), .count = 11},
  [169] = NULL, // Function
  [170] = NULL, // Function
  [171] = NULL, // No associated type
  [172] = NULL, // No associated type
  [173] = NULL, // No associated type
  [174] = NULL, // No associated type
  [175] = NULL, // Function
  [176] = NULL, // Function
  [177] = NULL, // Function
  [178] = NULL, // No associated type
  [179] = NULL, // No associated type
  [180] = NULL, // No associated type
  [181] = NULL, // Function
  [182] = NULL, // Function
  [183] = NULL, // Function
  [184] = NULL, // Function
  [185] = NULL, // Function
  [186] = NULL, // Function
  [187] = NULL, // Function
  [188] = NULL, // Function
  [189] = NULL, // Function
  [190] = NULL, // Function
  [191] = NULL, // Function
  [192] = NULL, // Function
  [193] = &(TypeInfo){TypeKind_Ptr, .size = sizeof(void *), .align = alignof(void *), .base = TYPEID(15, TypeKind_Double, double)},
  [194] = NULL, // Function
  [195] = NULL, // Function
  [196] = NULL, // Function
  [197] = NULL, // Function
  [198] = NULL, // Function
  [199] = NULL, // Function
  [200] = NULL, // Function
  [201] = NULL, // Function
  [202] = NULL, // Function
  [203] = NULL, // Function
  [204] = NULL, // Function
  [205] = NULL, // Function
  [206] = NULL, // Function
  [207] = NULL, // No associated type
  [208] = NULL, // No associated type
  [209] = NULL, // No associated type
  [210] = NULL, // Function
  [211] = NULL, // Function
  [212] = NULL, // Function
  [213] = NULL, // Function
  [214] = NULL, // Function
  [215] = NULL, // Function
  [216] = NULL, // Function
  [217] = NULL, // Function
  [218] = NULL, // Function
  [219] = NULL, // Function
  [220] = NULL, // Function
  [221] = NULL, // Function
  [222] = NULL, // Function
  [223] = NULL, // Function
  [224] = NULL, // Function
  [225] = NULL, // Function
  [226] = NULL, // Function
  [227] = NULL, // Function
  [228] = NULL, // Function
  [229] = NULL, // Function
  [230] = NULL, // Function
  [231] = NULL, // Function
  [232] = NULL, // Function
  [233] = NULL, // No associated type
  [234] = NULL, // Function
  [235] = NULL, // Function
  [236] = NULL, // Function
  [237] = NULL, // Function
  [238] = NULL, // No associated type
  [239] = NULL, // No associated type
  [240] = NULL, // No associated type
  [241] = NULL, // Function
  [242] = NULL, // Function
  [243] = NULL, // Function
  [244] = NULL, // Function
  [245] = NULL, // Function
  [246] = NULL, // Function
  [247] = NULL, // Function
  [248] = NULL, // Function
  [249] = NULL, // Function
  [250] = NULL, // Function
  [251] = NULL, // Function
  [252] = NULL, // Function
  [253] = NULL, // Function
  [254] = NULL, // Function
  [255] = NULL, // Function
  [256] = NULL, // Function
  [257] = NULL, // Function
  [258] = NULL, // Function
  [259] = NULL, // Function
  [260] = NULL, // Function
  [261] = NULL, // Function
  [262] = NULL, // Function
  [263] = NULL, // Function
  [264] = NULL, // Function
  [265] = &(TypeInfo){TypeKind_Ptr, .size = sizeof(void *), .align = alignof(void *), .base = TYPEID(6, TypeKind_Short, short)},
  [266] = NULL, // Function
  [267] = NULL, // Function
  [268] = NULL, // Function
  [269] = &(TypeInfo){TypeKind_Const, .size = sizeof(short const ), .align = alignof(short const ), .base = TYPEID(6, TypeKind_Short, short)},
  [270] = &(TypeInfo){TypeKind_Ptr, .size = sizeof(void *), .align = alignof(void *), .base = TYPEID(269, TypeKind_Const, short const )},
  [271] = NULL, // Function
  [272] = NULL, // Function
  [273] = NULL, // Function
  [274] = NULL, // Function
  [275] = NULL, // Function
  [276] = NULL, // Function
  [277] = NULL, // Function
  [278] = NULL, // Function
  [279] = NULL, // Function
  [280] = NULL, // Function
  [281] = NULL, // Function
  [282] = NULL, // Function
  [283] = NULL, // Function
  [284] = NULL, // Function
  [285] = NULL, // No associated type
  [286] = NULL, // Function
  [287] = NULL, // Function
  [288] = NULL, // No associated type
  [289] = NULL, // Function
  [290] = NULL, // No associated type
  [291] = NULL, // No associated type
  [292] = NULL, // Function
  [293] = &(TypeInfo){TypeKind_Const, .size = sizeof(long const ), .align = alignof(long const ), .base = TYPEID(10, TypeKind_Long, long)},
  [294] = &(TypeInfo){TypeKind_Ref, .size = sizeof(void *), .align = alignof(void *), .base = TYPEID(293, TypeKind_Const, long const )},
  [295] = NULL, // Function
  [296] = NULL, // Function
  [297] = NULL, // Function
  [298] = &(TypeInfo){TypeKind_Array, .size = sizeof(int [9]), .align = alignof(int [9]), .base = TYPEID(8, TypeKind_Int, int), .count = 9},
  [299] = NULL, // No associated type
  [300] = &(TypeInfo){TypeKind_Array, .size = sizeof(char [15]), .align = alignof(char [15]), .base = TYPEID(3, TypeKind_Char, char), .count = 15},
};

int num_typeinfos = 301;
const TypeInfo **typeinfos = (const TypeInfo **)typeinfo_table;

// Definitions
int main(int argc, char (*(*argv))) {
  if ((argv) == (0)) {
    printf("argv is null\n");
  }
  test1_test_ref();
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
  test1_test_type_path();
  test1_test_generics();
  test1_test_err();
  test1_test_for_each();
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

void test1_test_ref(void) {
  int i = 1;
  int (*p) = &(i);
  int (*q) = &(i);
  int (*s) = (q) + (1);
  *(p) = 2;
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
  va_arg_ptr(&(args), (Any){&(c), TYPEID(3, TypeKind_Char, char)});
  int i = {0};
  va_arg_ptr(&(args), (Any){&(i), TYPEID(8, TypeKind_Int, int)});
  llong ll = {0};
  va_arg_ptr(&(args), (Any){&(ll), TYPEID(12, TypeKind_LLong, llong)});
  printf("c=%d i=%d ll=%lld\n", c, i, ll);
  va_end_ptr(&(args));
}

void test1_test_os_arch(void) {
  printf("Target operating system: %s\n", RIOOS);
  printf("Target machine architecture: %s\n", RIOARCH);
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
  // 1...3
  case 1: case 2: case 3:  {
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
  Any const (x) = {&(i), TYPEID(8, TypeKind_Int, int)};
  Any y = {&(i), TYPEID(8, TypeKind_Int, int)};
  test1_Ints v = {.num_ints = 3, .int_ptr = (int []){1, 2, 3}, .int_arr = {1, 2, 3}};
  test1_Ints (ints_of_ints[]) = {{.num_ints = 3, .int_arr = {1, 2, 3}}, {.num_ints = 2, .int_ptr = (int [2]){-(1), -(2)}}};
}

void test1_test_loops(void) {
  switch (0) {
  case 1: {
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
  test1_Color a = (test1_Color_Red);
  test1_Color b = (test1_Color_Red);
  int c = (a) + (b);
  int i = a;
  a = i;
  printf("%d %d %d %d\n", (test1_Color_None), (test1_Color_Red), (test1_Color_Green), (test1_Color_Blue));
  printf("No color: %s\n", test1_color_names[(test1_Color_None)]);
  int d = test1_BAR;
  test1_TypedEnum e = test1_QUUX;
  test1_TypedEnum f = {0};
  f = test1_BAZ;
  test1_More more = {.prob = 0.5f, .amount = 0.9f};
  test1_More_Kind more_kind = (test1_More_Happy);
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
  i = test1_IS_DEBUG;
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
  test1_println_any((Any){&(i), TYPEID(8, TypeKind_Int, int)});
  test1_println_any((Any){&(f), TYPEID(14, TypeKind_Float, float)});
  test1_println_any((Any){&(p), TYPEID(39, TypeKind_Ptr, void *)});
  test1_println_type(TYPEID(8, TypeKind_Int, int));
  test1_println_type(TYPEID(113, TypeKind_Ptr, int const (*)));
  test1_println_type(TYPEID(114, TypeKind_Array, int const (*[42])));
  test1_println_type(TYPEID(115, TypeKind_Struct, test1_UartCtrl));
  test1_println_typeinfo(TYPEID(8, TypeKind_Int, int));
  test1_println_typeinfo(TYPEID(115, TypeKind_Struct, test1_UartCtrl));
  test1_println_typeinfo(TYPEID(117, TypeKind_Ptr, test1_IntOrPtr *));
  test1_println_typeinfo(TYPEID(118, TypeKind_Ref, test1_IntOrPtr *));
  test1_println_typeinfo(TYPEID(116, TypeKind_Union, test1_IntOrPtr));
}

void test1_test_reachable(void) {
}

void test1_test_type_path(void) {
  long t1 = time(NULL);
  time_t t2 = time(NULL);
  test1_my_time_t t3 = time(NULL);
}

void test1_test_generics(void) {
  test1_Slirce slirce = {0};
  test1_Slorce_int slorce = {.items = (int []){1, 2, 3}, .length = 3};
  test1_Slorce_ref_int slorce_ref = {.items = (int *[]){slorce.items}, .length = 1};
  printf("Generically, %d\n", *(slorce_ref.items[0]));
  test1_Slorce_Bar slarb = {0};
  test1_Slarce slarce = slorce;
  test1_Slorce_Hey slorce_hey = {0};
  test1_Slorce_Slorce_Hey slorce_slorce_hey = {0};
  test1_test_generic_functions();
}

void test1_test_err(void) {
  test1_maybe_math(4.5f);
}

void test1_test_for_each(void) {
  char const ((*(friends[]))) = {"Larry", "Moe", "Curly"};
  {
    char const ((*(*items__))) = friends;
    for (size_t i__ = 0; i__ < 3; ++i__) {
      char const ((*friend)) = items__[i__];
      printf("friend: %s\n", friend);
    }
  }
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

char const ((*(test1_color_names[(test1_Color_Num)]))) = {[(test1_Color_None)] = "none", [(test1_Color_Red)] = "red", [(test1_Color_Green)] = "green", [(test1_Color_Blue)] = "blue"};
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

void test1_test_generic_functions(void) {
}

test1_Result_float_TestErr test1_maybe_math(float x) {
  return (test1_Result_float_TestErr){.kind = (Result_Ok), .val = (x) + (1)};
}

void test1_subtest1_func2(void) {
  test1_subtest1_func3();
  test1_subtest1_func4();
}

test1_Thing test1_thing;
void test1_print_any(Any any) {
  switch (any.type) {
  case TYPEID(8, TypeKind_Int, int): {
    printf("%d", *((int const (*))(any.ptr)));
    break;
  }
  case TYPEID(14, TypeKind_Float, float): {
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
  case (test1_Ptr): {
    printf("This is very unlikely.");
    break;
  }
  case TypeKind_Ptr: {
    test1_print_type(typeinfo->base);
    printf("*");
    break;
  }
  case TypeKind_Ref: {
    test1_print_type(typeinfo->base);
    printf("&");
    break;
  }
  case TypeKind_Const: {
    test1_print_type(typeinfo->base);
    printf(" const");
    break;
  }
  case TypeKind_Array: {
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
  case TypeKind_Struct:
  case TypeKind_Union: {
    printf(" %s={ ", ((typeinfo->kind) == ((TypeKind_Struct)) ? "struct" : "union"));
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
