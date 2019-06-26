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
typedef struct Slice_TypeFieldInfo Slice_TypeFieldInfo;
typedef struct TypeInfo TypeInfo;
typedef struct nio_Slice_String nio_Slice_String;
typedef struct nio_Options nio_Options;
typedef struct TypeFieldInfo TypeFieldInfo;
typedef struct Any Any;
typedef struct nio_Slice_char nio_Slice_char;
typedef struct nio_Lex nio_Lex;
typedef struct nio_Pos nio_Pos;
typedef struct nio_Token nio_Token;

// Sorted declarations
typedef char (*nio_String);

int main(int argc, nio_String (*argv));

typedef int Result_Kind;

#define Result_Ok ((Result_Kind)(0))

#define Result_Err ((Result_Kind)((Result_Ok) + (1)))

typedef int Err;

typedef void Item;

typedef int Try_Kind;

#define Try_Ok ((Try_Kind)(0))

#define Try_Err ((Try_Kind)((Try_Ok) + (1)))

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


struct Slice_TypeFieldInfo {
  TypeFieldInfo (*items);
  size_t length;
};

struct TypeInfo {
  TypeKind kind;
  int size;
  int align;
  char const ((*name));
  int count;
  typeid base;
  Slice_TypeFieldInfo fields;
};

TypeKind typeid_kind(typeid type);

int typeid_index(typeid type);

size_t typeid_size(typeid type);

TypeInfo const ((*get_typeinfo(typeid type)));

#define UCHAR_MIN ((uchar)(0))

#define USHORT_MIN ((ushort)(0))

#define UINT_MIN ((uint)(0))

#define ULLONG_MIN ((ullong)(0))

#define UINT8_MIN (UCHAR_MIN)

#define UINT16_MIN (USHORT_MIN)

#define UINT32_MIN (UINT_MIN)

#define UINT64_MIN (ULLONG_MIN)

#define ULONG_MIN ((ulong)(INT64_MIN))

#define USIZE_MIN (UINT64_MIN)

#define UINTPTR_MIN (UINT64_MIN)

struct nio_Slice_String {
  nio_String (*items);
  size_t length;
};

struct nio_Options {
  nio_String in;
  nio_String out;
};

nio_Options nio_parse_args(nio_Slice_String args);

void nio_run(nio_Options options);



struct TypeFieldInfo {
  char const ((*name));
  typeid type;
  int offset;
};

struct Any {
  void (*ptr);
  typeid type;
};

typedef int nio_OptionKey;

#define nio_OptionKey_None ((nio_OptionKey)(0))

#define nio_OptionKey_Out ((nio_OptionKey)((nio_OptionKey_None) + (1)))

void nio_parse(nio_String in);

void nio_gen(nio_String path);

struct nio_Slice_char {
  char (*items);
  size_t length;
};

struct nio_Lex {
  nio_Slice_char code;
  int col;
  int index;
  nio_String path;
  int row;
};

nio_Lex nio_lex_init(nio_String path);

typedef int nio_Token_Kind;

struct nio_Pos {
  int col;
  int row;
};

struct nio_Token {
  nio_Token_Kind kind;
  nio_Pos begin;
  nio_Pos end;
  nio_String file;
  union {
    // void;
    // void;
    // void;
    // void;
    nio_String text;
    // void;
    // void;
    // void;
    // void;
  };
};

nio_Token nio_lex_next(nio_Lex (*lex));

#define nio_Token_Comma ((nio_Token_Kind)(0))

#define nio_Token_CurlyBegin ((nio_Token_Kind)((nio_Token_Comma) + (1)))

#define nio_Token_CurlyEnd ((nio_Token_Kind)((nio_Token_CurlyBegin) + (1)))

#define nio_Token_Def ((nio_Token_Kind)((nio_Token_CurlyEnd) + (1)))

#define nio_Token_Eof ((nio_Token_Kind)((nio_Token_Def) + (1)))

nio_Slice_char nio_read_file(char (*path));

nio_Token nio_lex_next_id(nio_Lex (*lex));

void (*nio_xmalloc(size_t num_bytes));

// Typeinfo
#define TYPEID0(index, kind) ((ullong)(index) | ((ullong)(kind) << 24))
#define TYPEID(index, kind, ...) ((ullong)(index) | ((ullong)sizeof(__VA_ARGS__) << 32) | ((ullong)(kind) << 24))

int num_typeinfos;
const TypeInfo **typeinfos;

// Definitions
int main(int argc, nio_String (*argv)) {
  nio_Options options = nio_parse_args((nio_Slice_String){(argv) + (1), (argc) - (1)});
  nio_run(options);
  return 0;
}

char const ((*RIOOS)) = "linux";
char const ((*RIOARCH)) = "x64";
TypeKind typeid_kind(typeid type) {
  return (((type) >> (24))) & (0xff);
}

int typeid_index(typeid type) {
  return (type) & (0xffffff);
}

size_t typeid_size(typeid type) {
  return (type) >> (32);
}

TypeInfo const ((*get_typeinfo(typeid type))) {
  int index = typeid_index(type);
  if ((typeinfos) && ((index) < (num_typeinfos))) {
    return typeinfos[index];
  } else {
    return NULL;
  }
}

nio_Options nio_parse_args(nio_Slice_String args) {
  nio_OptionKey key = (nio_OptionKey_None);
  nio_Options options = {0};
  {
    nio_Slice_String items__ = args;
    for (size_t i__ = 0; i__ < items__.length; ++i__) {
      char (*arg) = items__.items[i__];
      if (key) {
        switch (key) {
        case nio_OptionKey_Out: {
          options.out = arg;
          break;
        }
        }
        key = (nio_OptionKey_None);
      } else {
        if (!(strcmp(arg, "-o"))) {
          key = (nio_OptionKey_Out);
        } else {
          options.in = arg;
        }
      }
    }
  }
  return options;
}

void nio_run(nio_Options options) {
  printf("in: %s\n", options.in);
  printf("out: %s\n", options.out);
  if ((options.in) && (options.out)) {
    nio_parse(options.in);
    nio_gen(options.out);
  }
}

void nio_parse(nio_String in) {
  nio_Lex lex = nio_lex_init(in);
  bool more = true;
  while (more) {
    nio_Token token = nio_lex_next(&(lex));
    switch (token.kind) {
    case nio_Token_Eof: {
      more = false;
      break;
    }
    }
  }
}

void nio_gen(nio_String path) {
  FILE (*file) = fopen(path, "wb");
  if (!(file)) {
    printf("Failed to write: %s\n", path);
    exit(1);
  }
  fprintf(file, "int main() {\n");
  fprintf(file, "  return 0;\n");
  fprintf(file, "}\n");
  fclose(file);
}

nio_Lex nio_lex_init(nio_String path) {
  nio_Slice_char code = nio_read_file(path);
  return (nio_Lex){.code = code, .path = path};
}

nio_Token nio_lex_next(nio_Lex (*lex)) {
  int next_index = lex->index;
  nio_Slice_char code = lex->code;
  while ((next_index) < (lex->code.length)) {
    char next = code.items[next_index];
    next_index += 1;
    switch (next) {
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F': case 'G': case 'H': case 'I': case 'J': case 'K': case 'L': case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R': case 'S': case 'T': case 'U': case 'V': case 'W': case 'X': case 'Y': case 'Z': 
    case 'a': case 'b': case 'c': case 'd': case 'e': case 'f': case 'g': case 'h': case 'i': case 'j': case 'k': case 'l': case 'm': case 'n': case 'o': case 'p': case 'q': case 'r': case 's': case 't': case 'u': case 'v': case 'w': case 'x': case 'y': case 'z': 
    case '_': {
      return nio_lex_next_id(lex);
      break;
    }
    }
    ++(next_index);
  }
  lex->index = next_index;
  return (nio_Token){(nio_Token_Eof)};
}

nio_Slice_char nio_read_file(char (*path)) {
  FILE (*file) = fopen(path, "rb");
  if (!(file)) {
    return (nio_Slice_char){NULL};
  }
  fseek(file, 0, SEEK_END);
  long len = ftell(file);
  fseek(file, 0, SEEK_SET);
  char (*buf) = nio_xmalloc((len) + (1));
  if ((len) && ((fread(buf, len, 1, file)) != (1))) {
    fclose(file);
    free(buf);
    return (nio_Slice_char){NULL};
  }
  fclose(file);
  buf[len] = 0;
  return (nio_Slice_char){buf, len};
}

nio_Token nio_lex_next_id(nio_Lex (*lex)) {
  return (nio_Token){(nio_Token_Eof)};
}

void (*nio_xmalloc(size_t num_bytes)) {
  void (*ptr) = malloc(num_bytes);
  if (!(ptr)) {
    perror("xmalloc failed");
    exit(1);
  }
  return ptr;
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
