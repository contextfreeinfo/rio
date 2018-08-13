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

// Sorted declarations
int main(void);

extern char const ((*RIOOS));

extern char const ((*RIOARCH));

typedef ullong typeid;

typedef int TypeKind;

#define TYPE_NONE ((TypeKind)(0))

#define TypeKind_TYPE_NONE ((TypeKind)(0))

#define TYPE_VOID ((TypeKind)((TYPE_NONE) + (1)))

#define TypeKind_TYPE_VOID ((TypeKind)((TYPE_NONE) + (1)))

#define TYPE_BOOL ((TypeKind)((TYPE_VOID) + (1)))

#define TypeKind_TYPE_BOOL ((TypeKind)((TYPE_VOID) + (1)))

#define TYPE_CHAR ((TypeKind)((TYPE_BOOL) + (1)))

#define TypeKind_TYPE_CHAR ((TypeKind)((TYPE_BOOL) + (1)))

#define TYPE_UCHAR ((TypeKind)((TYPE_CHAR) + (1)))

#define TypeKind_TYPE_UCHAR ((TypeKind)((TYPE_CHAR) + (1)))

#define TYPE_SCHAR ((TypeKind)((TYPE_UCHAR) + (1)))

#define TypeKind_TYPE_SCHAR ((TypeKind)((TYPE_UCHAR) + (1)))

#define TYPE_SHORT ((TypeKind)((TYPE_SCHAR) + (1)))

#define TypeKind_TYPE_SHORT ((TypeKind)((TYPE_SCHAR) + (1)))

#define TYPE_USHORT ((TypeKind)((TYPE_SHORT) + (1)))

#define TypeKind_TYPE_USHORT ((TypeKind)((TYPE_SHORT) + (1)))

#define TYPE_INT ((TypeKind)((TYPE_USHORT) + (1)))

#define TypeKind_TYPE_INT ((TypeKind)((TYPE_USHORT) + (1)))

#define TYPE_UINT ((TypeKind)((TYPE_INT) + (1)))

#define TypeKind_TYPE_UINT ((TypeKind)((TYPE_INT) + (1)))

#define TYPE_LONG ((TypeKind)((TYPE_UINT) + (1)))

#define TypeKind_TYPE_LONG ((TypeKind)((TYPE_UINT) + (1)))

#define TYPE_ULONG ((TypeKind)((TYPE_LONG) + (1)))

#define TypeKind_TYPE_ULONG ((TypeKind)((TYPE_LONG) + (1)))

#define TYPE_LLONG ((TypeKind)((TYPE_ULONG) + (1)))

#define TypeKind_TYPE_LLONG ((TypeKind)((TYPE_ULONG) + (1)))

#define TYPE_ULLONG ((TypeKind)((TYPE_LLONG) + (1)))

#define TypeKind_TYPE_ULLONG ((TypeKind)((TYPE_LLONG) + (1)))

#define TYPE_FLOAT ((TypeKind)((TYPE_ULLONG) + (1)))

#define TypeKind_TYPE_FLOAT ((TypeKind)((TYPE_ULLONG) + (1)))

#define TYPE_DOUBLE ((TypeKind)((TYPE_FLOAT) + (1)))

#define TypeKind_TYPE_DOUBLE ((TypeKind)((TYPE_FLOAT) + (1)))

#define TYPE_CONST ((TypeKind)((TYPE_DOUBLE) + (1)))

#define TypeKind_TYPE_CONST ((TypeKind)((TYPE_DOUBLE) + (1)))

#define TYPE_PTR ((TypeKind)((TYPE_CONST) + (1)))

#define TypeKind_TYPE_PTR ((TypeKind)((TYPE_CONST) + (1)))

#define TYPE_REF ((TypeKind)((TYPE_PTR) + (1)))

#define TypeKind_TYPE_REF ((TypeKind)((TYPE_PTR) + (1)))

#define TYPE_ARRAY ((TypeKind)((TYPE_REF) + (1)))

#define TypeKind_TYPE_ARRAY ((TypeKind)((TYPE_REF) + (1)))

#define TYPE_STRUCT ((TypeKind)((TYPE_ARRAY) + (1)))

#define TypeKind_TYPE_STRUCT ((TypeKind)((TYPE_ARRAY) + (1)))

#define TYPE_UNION ((TypeKind)((TYPE_STRUCT) + (1)))

#define TypeKind_TYPE_UNION ((TypeKind)((TYPE_STRUCT) + (1)))

#define TYPE_FUNC ((TypeKind)((TYPE_UNION) + (1)))

#define TypeKind_TYPE_FUNC ((TypeKind)((TYPE_UNION) + (1)))

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

#define ULONG_MIN ((ulong)(INT32_MIN))

#define USIZE_MIN (UINT64_MIN)

#define UINTPTR_MIN (UINT64_MIN)

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

int num_typeinfos;
const TypeInfo **typeinfos;

// Definitions
int main(void) {
  int i = 1;
  int const (j) = 2;
  int k = 3;
  printf("Hello, %d!\n", i);
  return 0;
}

char const ((*RIOOS)) = "win32";
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
