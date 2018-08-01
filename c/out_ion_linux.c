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
typedef struct dirent dirent;
typedef struct tm tm_t;

// Foreign header files
#include <limits.h>
#include <stdint.h>
#include <dirent.h>
#include <ctype.h>
#include <errno.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// Forward declarations
typedef struct TypeInfo TypeInfo;
typedef struct ion_Arena ion_Arena;
typedef struct ion_SrcPos ion_SrcPos;
typedef struct ion_Note ion_Note;
typedef struct ion_Notes ion_Notes;
typedef struct ion_StmtList ion_StmtList;
typedef struct ion_Map ion_Map;
typedef struct ion_Token ion_Token;
typedef struct ion_CompoundField ion_CompoundField;
typedef struct ion_SwitchCasePattern ion_SwitchCasePattern;
typedef struct ion_SwitchCase ion_SwitchCase;
typedef struct ion_EnumItem ion_EnumItem;
typedef struct ion_AggregateItem ion_AggregateItem;
typedef struct ion_FuncParam ion_FuncParam;
typedef struct ion_NoteArg ion_NoteArg;
typedef union ion_Val ion_Val;
typedef struct ion_Sym ion_Sym;
typedef struct ion_Operand ion_Operand;
typedef struct ion_Label ion_Label;
typedef struct ion_StmtCtx ion_StmtCtx;
typedef struct ion_TypeMetrics ion_TypeMetrics;
typedef struct ion_TypeAggregate ion_TypeAggregate;
typedef struct ion_TypeFunc ion_TypeFunc;
typedef struct ion_Type ion_Type;
typedef struct TypeFieldInfo TypeFieldInfo;
typedef struct Any Any;
typedef struct ion_StmtIf ion_StmtIf;
typedef struct ion_StmtWhile ion_StmtWhile;
typedef struct ion_StmtFor ion_StmtFor;
typedef struct ion_StmtSwitch ion_StmtSwitch;
typedef struct ion_StmtAssign ion_StmtAssign;
typedef struct ion_StmtInit ion_StmtInit;
typedef struct ion_Stmt ion_Stmt;
typedef struct ion_TypespecFunc ion_TypespecFunc;
typedef struct ion_Typespec ion_Typespec;
typedef struct ion_ExprParen ion_ExprParen;
typedef struct ion_ExprIntLit ion_ExprIntLit;
typedef struct ion_ExprFloatLit ion_ExprFloatLit;
typedef struct ion_ExprStrLit ion_ExprStrLit;
typedef struct ion_ExprOffsetofField ion_ExprOffsetofField;
typedef struct ion_ExprCompound ion_ExprCompound;
typedef struct ion_ExprCast ion_ExprCast;
typedef struct ion_ExprModify ion_ExprModify;
typedef struct ion_ExprUnary ion_ExprUnary;
typedef struct ion_ExprBinary ion_ExprBinary;
typedef struct ion_ExprTernary ion_ExprTernary;
typedef struct ion_ExprCall ion_ExprCall;
typedef struct ion_ExprIndex ion_ExprIndex;
typedef struct ion_ExprField ion_ExprField;
typedef struct ion_Expr ion_Expr;
typedef struct ion_Decls ion_Decls;
typedef struct ion_DeclEnum ion_DeclEnum;
typedef struct ion_DeclFunc ion_DeclFunc;
typedef struct ion_DeclTypedef ion_DeclTypedef;
typedef struct ion_DeclVar ion_DeclVar;
typedef struct ion_DeclImport ion_DeclImport;
typedef struct ion_Decl ion_Decl;
typedef struct ion_Aggregate ion_Aggregate;
typedef struct ion_ImportItem ion_ImportItem;
typedef struct ion_ElseIf ion_ElseIf;
typedef struct ion_BufHdr ion_BufHdr;
typedef struct ion_Intern ion_Intern;
typedef struct ion_Package ion_Package;
typedef struct ion_TypeField ion_TypeField;
typedef struct ion_DirListIter ion_DirListIter;
typedef struct ion_FlagDefPtr ion_FlagDefPtr;
typedef struct ion_FlagDef ion_FlagDef;
typedef struct ion_CachedArrayType ion_CachedArrayType;
typedef struct ion_CachedFuncType ion_CachedFuncType;

// Sorted declarations
int main(int argc, char const ((*(*argv))));

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

struct ion_Arena {
    char (*ptr);
    char (*end);
    char (*(*blocks));
};

extern ion_Arena ion_ast_arena;

void (*ion_ast_alloc(size_t size));

void (*ion_ast_dup(void const ((*src)), size_t size));

struct ion_SrcPos {
    char const ((*name));
    int line;
};

struct ion_Note {
    ion_SrcPos pos;
    char const ((*name));
    ion_NoteArg (*args);
    size_t num_args;
};

ion_Note ion_new_note(ion_SrcPos pos, char const ((*name)), ion_NoteArg (*args), size_t num_args);

struct ion_Notes {
    ion_Note (*notes);
    size_t num_notes;
};

ion_Notes ion_new_notes(ion_Note (*notes), size_t num_notes);

struct ion_StmtList {
    ion_SrcPos pos;
    ion_Stmt (*(*stmts));
    size_t num_stmts;
};

ion_StmtList ion_new_stmt_list(ion_SrcPos pos, ion_Stmt (*(*stmts)), size_t num_stmts);

typedef int ion_TypespecKind;

ion_Typespec (*ion_new_typespec(ion_TypespecKind kind, ion_SrcPos pos));

ion_Typespec (*ion_new_typespec_name(ion_SrcPos pos, char const ((*name))));

ion_Typespec (*ion_new_typespec_ptr(ion_SrcPos pos, ion_Typespec (*base)));

ion_Typespec (*ion_new_typespec_const(ion_SrcPos pos, ion_Typespec (*base)));

ion_Typespec (*ion_new_typespec_array(ion_SrcPos pos, ion_Typespec (*elem), ion_Expr (*size)));

ion_Typespec (*ion_new_typespec_func(ion_SrcPos pos, ion_Typespec (*(*args)), size_t num_args, ion_Typespec (*ret), bool has_varargs));

ion_Decls (*ion_new_decls(ion_Decl (*(*decls)), size_t num_decls));

typedef int ion_DeclKind;

ion_Decl (*ion_new_decl(ion_DeclKind kind, ion_SrcPos pos, char const ((*name))));

ion_Note (*ion_get_decl_note(ion_Decl (*decl), char const ((*name))));

bool ion_is_decl_foreign(ion_Decl (*decl));

ion_Decl (*ion_new_decl_enum(ion_SrcPos pos, char const ((*name)), ion_Typespec (*type), ion_EnumItem (*items), size_t num_items));

typedef int ion_AggregateKind;

ion_Aggregate (*ion_new_aggregate(ion_SrcPos pos, ion_AggregateKind kind, ion_AggregateItem (*items), size_t num_items));

ion_Decl (*ion_new_decl_aggregate(ion_SrcPos pos, ion_DeclKind kind, char const ((*name)), ion_Aggregate (*aggregate)));

ion_Decl (*ion_new_decl_var(ion_SrcPos pos, char const ((*name)), ion_Typespec (*type), ion_Expr (*expr)));

ion_Decl (*ion_new_decl_func(ion_SrcPos pos, char const ((*name)), ion_FuncParam (*params), size_t num_params, ion_Typespec (*ret_type), bool has_varargs, ion_StmtList block));

ion_Decl (*ion_new_decl_const(ion_SrcPos pos, char const ((*name)), ion_Typespec (*type), ion_Expr (*expr)));

ion_Decl (*ion_new_decl_typedef(ion_SrcPos pos, char const ((*name)), ion_Typespec (*type)));

ion_Decl (*ion_new_decl_note(ion_SrcPos pos, ion_Note note));

ion_Decl (*ion_new_decl_import(ion_SrcPos pos, char const ((*rename_name)), bool is_relative, char const ((*(*names))), size_t num_names, bool import_all, ion_ImportItem (*items), size_t num_items));

typedef int ion_ExprKind;

ion_Expr (*ion_new_expr(ion_ExprKind kind, ion_SrcPos pos));

ion_Expr (*ion_new_expr_paren(ion_SrcPos pos, ion_Expr (*expr)));

ion_Expr (*ion_new_expr_sizeof_expr(ion_SrcPos pos, ion_Expr (*expr)));

ion_Expr (*ion_new_expr_sizeof_type(ion_SrcPos pos, ion_Typespec (*type)));

ion_Expr (*ion_new_expr_typeof_expr(ion_SrcPos pos, ion_Expr (*expr)));

ion_Expr (*ion_new_expr_typeof_type(ion_SrcPos pos, ion_Typespec (*type)));

ion_Expr (*ion_new_expr_alignof_expr(ion_SrcPos pos, ion_Expr (*expr)));

ion_Expr (*ion_new_expr_alignof_type(ion_SrcPos pos, ion_Typespec (*type)));

ion_Expr (*ion_new_expr_offsetof(ion_SrcPos pos, ion_Typespec (*type), char const ((*name))));

typedef int ion_TokenKind;

ion_Expr (*ion_new_expr_modify(ion_SrcPos pos, ion_TokenKind op, bool post, ion_Expr (*expr)));

typedef int ion_TokenMod;

typedef int ion_TokenSuffix;

ion_Expr (*ion_new_expr_int(ion_SrcPos pos, ullong val, ion_TokenMod mod, ion_TokenSuffix suffix));

ion_Expr (*ion_new_expr_float(ion_SrcPos pos, char const ((*start)), char const ((*end)), double val, ion_TokenSuffix suffix));

ion_Expr (*ion_new_expr_str(ion_SrcPos pos, char const ((*val)), ion_TokenMod mod));

ion_Expr (*ion_new_expr_name(ion_SrcPos pos, char const ((*name))));

ion_Expr (*ion_new_expr_compound(ion_SrcPos pos, ion_Typespec (*type), ion_CompoundField (*fields), size_t num_fields));

ion_Expr (*ion_new_expr_cast(ion_SrcPos pos, ion_Typespec (*type), ion_Expr (*expr)));

ion_Expr (*ion_new_expr_call(ion_SrcPos pos, ion_Expr (*expr), ion_Expr (*(*args)), size_t num_args));

ion_Expr (*ion_new_expr_index(ion_SrcPos pos, ion_Expr (*expr), ion_Expr (*index)));

ion_Expr (*ion_new_expr_field(ion_SrcPos pos, ion_Expr (*expr), char const ((*name))));

ion_Expr (*ion_new_expr_unary(ion_SrcPos pos, ion_TokenKind op, ion_Expr (*expr)));

ion_Expr (*ion_new_expr_binary(ion_SrcPos pos, ion_TokenKind op, ion_Expr (*left), ion_Expr (*right)));

ion_Expr (*ion_new_expr_ternary(ion_SrcPos pos, ion_Expr (*cond), ion_Expr (*then_expr), ion_Expr (*else_expr)));

ion_Note (*ion_get_stmt_note(ion_Stmt (*stmt), char const ((*name))));

typedef int ion_StmtKind;

ion_Stmt (*ion_new_stmt(ion_StmtKind kind, ion_SrcPos pos));

ion_Stmt (*ion_new_stmt_label(ion_SrcPos pos, char const ((*label))));

ion_Stmt (*ion_new_stmt_goto(ion_SrcPos pos, char const ((*label))));

ion_Stmt (*ion_new_stmt_note(ion_SrcPos pos, ion_Note note));

ion_Stmt (*ion_new_stmt_decl(ion_SrcPos pos, ion_Decl (*decl)));

ion_Stmt (*ion_new_stmt_return(ion_SrcPos pos, ion_Expr (*expr)));

ion_Stmt (*ion_new_stmt_break(ion_SrcPos pos));

ion_Stmt (*ion_new_stmt_continue(ion_SrcPos pos));

ion_Stmt (*ion_new_stmt_block(ion_SrcPos pos, ion_StmtList block));

ion_Stmt (*ion_new_stmt_if(ion_SrcPos pos, ion_Stmt (*init), ion_Expr (*cond), ion_StmtList then_block, ion_ElseIf (*elseifs), size_t num_elseifs, ion_StmtList else_block));

ion_Stmt (*ion_new_stmt_while(ion_SrcPos pos, ion_Expr (*cond), ion_StmtList block));

ion_Stmt (*ion_new_stmt_do_while(ion_SrcPos pos, ion_Expr (*cond), ion_StmtList block));

ion_Stmt (*ion_new_stmt_for(ion_SrcPos pos, ion_Stmt (*init), ion_Expr (*cond), ion_Stmt (*next), ion_StmtList block));

ion_Stmt (*ion_new_stmt_switch(ion_SrcPos pos, ion_Expr (*expr), ion_SwitchCase (*cases), size_t num_cases));

ion_Stmt (*ion_new_stmt_assign(ion_SrcPos pos, ion_TokenKind op, ion_Expr (*left), ion_Expr (*right)));

ion_Stmt (*ion_new_stmt_init(ion_SrcPos pos, char const ((*name)), ion_Typespec (*type), ion_Expr (*expr)));

ion_Stmt (*ion_new_stmt_expr(ion_SrcPos pos, ion_Expr (*expr)));

#define ION_TYPESPEC_NONE ((ion_TypespecKind)(0))

#define ION_TYPESPEC_NAME ((ion_TypespecKind)((ION_TYPESPEC_NONE) + (1)))

#define ION_TYPESPEC_FUNC ((ion_TypespecKind)((ION_TYPESPEC_NAME) + (1)))

#define ION_TYPESPEC_ARRAY ((ion_TypespecKind)((ION_TYPESPEC_FUNC) + (1)))

#define ION_TYPESPEC_PTR ((ion_TypespecKind)((ION_TYPESPEC_ARRAY) + (1)))

#define ION_TYPESPEC_CONST ((ion_TypespecKind)((ION_TYPESPEC_PTR) + (1)))

typedef int ion_AggregateItemKind;

#define ION_AGGREGATE_ITEM_NONE ((ion_AggregateItemKind)(0))

#define ION_AGGREGATE_ITEM_FIELD ((ion_AggregateItemKind)((ION_AGGREGATE_ITEM_NONE) + (1)))

#define ION_AGGREGATE_ITEM_SUBAGGREGATE ((ion_AggregateItemKind)((ION_AGGREGATE_ITEM_FIELD) + (1)))

#define ION_DECL_NONE ((ion_DeclKind)(0))

#define ION_DECL_ENUM ((ion_DeclKind)((ION_DECL_NONE) + (1)))

#define ION_DECL_STRUCT ((ion_DeclKind)((ION_DECL_ENUM) + (1)))

#define ION_DECL_UNION ((ion_DeclKind)((ION_DECL_STRUCT) + (1)))

#define ION_DECL_VAR ((ion_DeclKind)((ION_DECL_UNION) + (1)))

#define ION_DECL_CONST ((ion_DeclKind)((ION_DECL_VAR) + (1)))

#define ION_DECL_TYPEDEF ((ion_DeclKind)((ION_DECL_CONST) + (1)))

#define ION_DECL_FUNC ((ion_DeclKind)((ION_DECL_TYPEDEF) + (1)))

#define ION_DECL_NOTE ((ion_DeclKind)((ION_DECL_FUNC) + (1)))

#define ION_DECL_IMPORT ((ion_DeclKind)((ION_DECL_NOTE) + (1)))

#define ION_AGGREGATE_NONE ((ion_AggregateKind)(0))

#define ION_AGGREGATE_STRUCT ((ion_AggregateKind)((ION_AGGREGATE_NONE) + (1)))

#define ION_AGGREGATE_UNION ((ion_AggregateKind)((ION_AGGREGATE_STRUCT) + (1)))

#define ION_EXPR_NONE ((ion_ExprKind)(0))

#define ION_EXPR_PAREN ((ion_ExprKind)((ION_EXPR_NONE) + (1)))

#define ION_EXPR_INT ((ion_ExprKind)((ION_EXPR_PAREN) + (1)))

#define ION_EXPR_FLOAT ((ion_ExprKind)((ION_EXPR_INT) + (1)))

#define ION_EXPR_STR ((ion_ExprKind)((ION_EXPR_FLOAT) + (1)))

#define ION_EXPR_NAME ((ion_ExprKind)((ION_EXPR_STR) + (1)))

#define ION_EXPR_CAST ((ion_ExprKind)((ION_EXPR_NAME) + (1)))

#define ION_EXPR_CALL ((ion_ExprKind)((ION_EXPR_CAST) + (1)))

#define ION_EXPR_INDEX ((ion_ExprKind)((ION_EXPR_CALL) + (1)))

#define ION_EXPR_FIELD ((ion_ExprKind)((ION_EXPR_INDEX) + (1)))

#define ION_EXPR_COMPOUND ((ion_ExprKind)((ION_EXPR_FIELD) + (1)))

#define ION_EXPR_UNARY ((ion_ExprKind)((ION_EXPR_COMPOUND) + (1)))

#define ION_EXPR_BINARY ((ion_ExprKind)((ION_EXPR_UNARY) + (1)))

#define ION_EXPR_TERNARY ((ion_ExprKind)((ION_EXPR_BINARY) + (1)))

#define ION_EXPR_MODIFY ((ion_ExprKind)((ION_EXPR_TERNARY) + (1)))

#define ION_EXPR_SIZEOF_EXPR ((ion_ExprKind)((ION_EXPR_MODIFY) + (1)))

#define ION_EXPR_SIZEOF_TYPE ((ion_ExprKind)((ION_EXPR_SIZEOF_EXPR) + (1)))

#define ION_EXPR_TYPEOF_EXPR ((ion_ExprKind)((ION_EXPR_SIZEOF_TYPE) + (1)))

#define ION_EXPR_TYPEOF_TYPE ((ion_ExprKind)((ION_EXPR_TYPEOF_EXPR) + (1)))

#define ION_EXPR_ALIGNOF_EXPR ((ion_ExprKind)((ION_EXPR_TYPEOF_TYPE) + (1)))

#define ION_EXPR_ALIGNOF_TYPE ((ion_ExprKind)((ION_EXPR_ALIGNOF_EXPR) + (1)))

#define ION_EXPR_OFFSETOF ((ion_ExprKind)((ION_EXPR_ALIGNOF_TYPE) + (1)))

typedef int ion_CompoundFieldKind;

#define ION_FIELD_DEFAULT ((ion_CompoundFieldKind)(0))

#define ION_FIELD_NAME ((ion_CompoundFieldKind)((ION_FIELD_DEFAULT) + (1)))

#define ION_FIELD_INDEX ((ion_CompoundFieldKind)((ION_FIELD_NAME) + (1)))

#define ION_STMT_NONE ((ion_StmtKind)(0))

#define ION_STMT_DECL ((ion_StmtKind)((ION_STMT_NONE) + (1)))

#define ION_STMT_RETURN ((ion_StmtKind)((ION_STMT_DECL) + (1)))

#define ION_STMT_BREAK ((ion_StmtKind)((ION_STMT_RETURN) + (1)))

#define ION_STMT_CONTINUE ((ion_StmtKind)((ION_STMT_BREAK) + (1)))

#define ION_STMT_BLOCK ((ion_StmtKind)((ION_STMT_CONTINUE) + (1)))

#define ION_STMT_IF ((ion_StmtKind)((ION_STMT_BLOCK) + (1)))

#define ION_STMT_WHILE ((ion_StmtKind)((ION_STMT_IF) + (1)))

#define ION_STMT_DO_WHILE ((ion_StmtKind)((ION_STMT_WHILE) + (1)))

#define ION_STMT_FOR ((ion_StmtKind)((ION_STMT_DO_WHILE) + (1)))

#define ION_STMT_SWITCH ((ion_StmtKind)((ION_STMT_FOR) + (1)))

#define ION_STMT_ASSIGN ((ion_StmtKind)((ION_STMT_SWITCH) + (1)))

#define ION_STMT_INIT ((ion_StmtKind)((ION_STMT_ASSIGN) + (1)))

#define ION_STMT_EXPR ((ion_StmtKind)((ION_STMT_INIT) + (1)))

#define ION_STMT_NOTE ((ion_StmtKind)((ION_STMT_EXPR) + (1)))

#define ION_STMT_LABEL ((ion_StmtKind)((ION_STMT_NOTE) + (1)))

#define ION_STMT_GOTO ((ion_StmtKind)((ION_STMT_LABEL) + (1)))

size_t ion_min(size_t x, size_t y);

size_t ion_max(size_t x, size_t y);

size_t ion_clamp_max(size_t x, size_t max);

size_t ion_clamp_min(size_t x, size_t min);

size_t ion_is_pow2(size_t x);

uintptr_t ion_align_down(uintptr_t n, size_t a);

uintptr_t ion_align_up(uintptr_t n, size_t a);

void (*ion_align_down_ptr(void (*p), size_t a));

void (*ion_align_up_ptr(void (*p), size_t a));

void ion_fatal(char const ((*fmt)), ...);

void (*ion_xcalloc(size_t num_elems, size_t elem_size));

void (*ion_xrealloc(void (*ptr), size_t num_bytes));

void (*ion_xmalloc(size_t num_bytes));

void (*ion_memdup(void (*src), size_t size));

char (*ion_strf(char const ((*fmt)), ...));

char (*ion_read_file(char const ((*path))));

bool ion_write_file(char const ((*path)), char const ((*buf)), size_t len);

ion_BufHdr (*ion_buf__hdr(void (*b)));

size_t ion_buf_len(void (*b));

size_t ion_buf_cap(void const ((*b)));

void (*ion_buf_end(void (*b), size_t elem_size));

size_t ion_buf_sizeof(void (*b), size_t elem_size);

void ion_buf_free(void (*(*b)));

void ion_buf_fit(void (*(*b)), size_t new_len, size_t elem_size);

void ion_buf_push(void (*(*b)), void (*elem), size_t elem_size);

void (*ion_buf__grow(void const ((*buf)), size_t new_len, size_t elem_size));

void ion_buf_printf(char (*(*buf)), char const ((*fmt)), ...);

extern ion_Arena ion_allocator;

#define ION_ARENA_ALIGNMENT (8)

#define ION_ARENA_BLOCK_SIZE ((1024) * (1024))

void ion_arena_grow(ion_Arena (*arena), size_t min_size);

void (*ion_arena_alloc(ion_Arena (*arena), size_t size));

void ion_arena_free(ion_Arena (*arena));

uint64_t ion_hash_uint64(uint64_t x);

uint64_t ion_hash_ptr(void const ((*ptr)));

uint64_t ion_hash_mix(uint64_t x, uint64_t y);

uint64_t ion_hash_bytes(void const ((*ptr)), size_t len);

uint64_t ion_map_get_uint64_from_uint64(ion_Map (*map), uint64_t key);

void ion_map_grow(ion_Map (*map), size_t new_cap);

void ion_map_put_uint64_from_uint64(ion_Map (*map), uint64_t key, uint64_t val);

void (*ion_map_get(ion_Map (*map), void const ((*key))));

void ion_map_put(ion_Map (*map), void const ((*key)), void (*val));

void (*ion_map_get_from_uint64(ion_Map (*map), uint64_t key));

void ion_map_put_from_uint64(ion_Map (*map), uint64_t key, void (*val));

uint64_t ion_map_get_uint64(ion_Map (*map), void (*key));

void ion_map_put_uint64(ion_Map (*map), void (*key), uint64_t val);

extern ion_Arena ion_intern_arena;

struct ion_Map {
    uint64_t (*keys);
    uint64_t (*vals);
    size_t len;
    size_t cap;
};

extern ion_Map ion_interns;

char const ((*ion_str_intern_range(char const ((*start)), char const ((*end)))));

char const ((*ion_str_intern(char const ((*str)))));

bool ion_str_islower(char const ((*str)));

extern char (*ion_gen_buf);

extern int ion_gen_indent;

extern ion_SrcPos ion_gen_pos;

extern char const ((*(*ion_gen_headers_buf)));

extern char (*ion_gen_preamble_str);

extern char (*ion_gen_postamble_str);

void ion_genln(void);

bool ion_is_incomplete_array_typespec(ion_Typespec (*typespec));

extern char (ion_char_to_escape[256]);

void ion_gen_char(char c);

void ion_gen_str(char const ((*str)), bool multiline);

void ion_gen_sync_pos(ion_SrcPos pos);

char const ((*ion_cdecl_paren(char const ((*str)), char c)));

char const ((*ion_cdecl_name(ion_Type (*type))));

char (*ion_type_to_cdecl(ion_Type (*type), char const ((*str))));

char const ((*ion_gen_expr_str(ion_Expr (*expr))));

extern ion_Map ion_gen_name_map;

char const ((*ion_get_gen_name_or_default(void const ((*ptr)), char const ((*default_name)))));

char const ((*ion_get_gen_name(void const ((*ptr)))));

char (*ion_typespec_to_cdecl(ion_Typespec (*typespec), char const ((*str))));

void ion_gen_func_decl(ion_Decl (*decl));

bool ion_gen_reachable(ion_Sym (*sym));

void ion_gen_forward_decls(void);

void ion_gen_aggregate_items(ion_Aggregate (*aggregate));

void ion_gen_aggregate(ion_Decl (*decl));

void ion_gen_paren_expr(ion_Expr (*expr));

void ion_gen_expr_compound(ion_Expr (*expr));

typedef int ion_CompilerTypeKind;

#define ION_CMPL_TYPE_NONE ((ion_CompilerTypeKind)(0))

#define ION_CMPL_TYPE_INCOMPLETE ((ion_CompilerTypeKind)((ION_CMPL_TYPE_NONE) + (1)))

#define ION_CMPL_TYPE_COMPLETING ((ion_CompilerTypeKind)((ION_CMPL_TYPE_INCOMPLETE) + (1)))

#define ION_CMPL_TYPE_VOID ((ion_CompilerTypeKind)((ION_CMPL_TYPE_COMPLETING) + (1)))

#define ION_CMPL_TYPE_BOOL ((ion_CompilerTypeKind)((ION_CMPL_TYPE_VOID) + (1)))

#define ION_CMPL_TYPE_CHAR ((ion_CompilerTypeKind)((ION_CMPL_TYPE_BOOL) + (1)))

#define ION_CMPL_TYPE_SCHAR ((ion_CompilerTypeKind)((ION_CMPL_TYPE_CHAR) + (1)))

#define ION_CMPL_TYPE_UCHAR ((ion_CompilerTypeKind)((ION_CMPL_TYPE_SCHAR) + (1)))

#define ION_CMPL_TYPE_SHORT ((ion_CompilerTypeKind)((ION_CMPL_TYPE_UCHAR) + (1)))

#define ION_CMPL_TYPE_USHORT ((ion_CompilerTypeKind)((ION_CMPL_TYPE_SHORT) + (1)))

#define ION_CMPL_TYPE_INT ((ion_CompilerTypeKind)((ION_CMPL_TYPE_USHORT) + (1)))

#define ION_CMPL_TYPE_UINT ((ion_CompilerTypeKind)((ION_CMPL_TYPE_INT) + (1)))

#define ION_CMPL_TYPE_LONG ((ion_CompilerTypeKind)((ION_CMPL_TYPE_UINT) + (1)))

#define ION_CMPL_TYPE_ULONG ((ion_CompilerTypeKind)((ION_CMPL_TYPE_LONG) + (1)))

#define ION_CMPL_TYPE_LLONG ((ion_CompilerTypeKind)((ION_CMPL_TYPE_ULONG) + (1)))

#define ION_CMPL_TYPE_ULLONG ((ion_CompilerTypeKind)((ION_CMPL_TYPE_LLONG) + (1)))

#define ION_CMPL_TYPE_ENUM ((ion_CompilerTypeKind)((ION_CMPL_TYPE_ULLONG) + (1)))

#define ION_CMPL_TYPE_FLOAT ((ion_CompilerTypeKind)((ION_CMPL_TYPE_ENUM) + (1)))

#define ION_CMPL_TYPE_DOUBLE ((ion_CompilerTypeKind)((ION_CMPL_TYPE_FLOAT) + (1)))

#define ION_CMPL_TYPE_PTR ((ion_CompilerTypeKind)((ION_CMPL_TYPE_DOUBLE) + (1)))

#define ION_CMPL_TYPE_FUNC ((ion_CompilerTypeKind)((ION_CMPL_TYPE_PTR) + (1)))

#define ION_CMPL_TYPE_ARRAY ((ion_CompilerTypeKind)((ION_CMPL_TYPE_FUNC) + (1)))

#define ION_CMPL_TYPE_STRUCT ((ion_CompilerTypeKind)((ION_CMPL_TYPE_ARRAY) + (1)))

#define ION_CMPL_TYPE_UNION ((ion_CompilerTypeKind)((ION_CMPL_TYPE_STRUCT) + (1)))

#define ION_CMPL_TYPE_CONST ((ion_CompilerTypeKind)((ION_CMPL_TYPE_UNION) + (1)))

#define ION_NUM_CMPL_TYPE_KINDS ((ion_CompilerTypeKind)((ION_CMPL_TYPE_CONST) + (1)))

extern char const ((*(ion_typeid_kind_names[ION_NUM_CMPL_TYPE_KINDS])));

char const ((*ion_typeid_kind_name(ion_Type (*type))));

bool ion_is_excluded_typeinfo(ion_Type (*type));

void ion_gen_typeid(ion_Type (*type));

void ion_gen_expr(ion_Expr (*expr));

void ion_gen_stmt_block(ion_StmtList block);

void ion_gen_simple_stmt(ion_Stmt (*stmt));

bool ion_is_char_lit(ion_Expr (*expr));

void ion_gen_stmt(ion_Stmt (*stmt));

void ion_gen_decl(ion_Sym (*sym));

void ion_gen_sorted_decls(void);

void ion_gen_defs(void);

extern ion_Map ion_gen_foreign_headers_map;

extern char const ((*(*ion_gen_foreign_headers_buf)));

void ion_add_foreign_header(char const ((*name)));

extern char const ((*(*ion_gen_foreign_sources_buf)));

void ion_add_foreign_source(char const ((*name)));

void ion_gen_include(char const ((*path)));

void ion_gen_foreign_headers(void);

void ion_gen_foreign_sources(void);

extern char const ((*(*ion_gen_sources_buf)));

void ion_put_include_path(char (path[MAX_PATH]), ion_Package (*package), char const ((*filename)));

extern char (*ion_gen_preamble_buf);

extern char (*ion_gen_postamble_buf);

void ion_preprocess_package(ion_Package (*package));

void ion_preprocess_packages(void);

void ion_gen_typeinfo_header(char const ((*kind)), ion_Type (*type));

void ion_gen_typeinfo_fields(ion_Type (*type));

void ion_gen_typeinfo(ion_Type (*type));

void ion_gen_typeinfos(void);

void ion_gen_package_external_names(void);

void ion_gen_preamble(void);

void ion_gen_postamble(void);

void ion_gen_all(void);

#define ION_MAX_SEARCH_PATHS ((int)(256))

extern char const ((*(ion_static_package_search_paths[ION_MAX_SEARCH_PATHS])));

extern char const ((*(*ion_package_search_paths)));

extern int ion_num_package_search_paths;

void ion_add_package_search_path(char const ((*path)));

void ion_add_package_search_path_range(char const ((*start)), char const ((*end)));

void ion_init_package_search_paths(void);

void ion_init_compiler(void);

void ion_parse_env_vars(void);

int ion_ion_main(int argc, char const ((*(*argv))), void (*gen_all)(void), char const ((*extension)));

extern char const ((*ion_typedef_keyword));

extern char const ((*ion_enum_keyword));

extern char const ((*ion_struct_keyword));

extern char const ((*ion_union_keyword));

extern char const ((*ion_var_keyword));

extern char const ((*ion_const_keyword));

extern char const ((*ion_func_keyword));

extern char const ((*ion_sizeof_keyword));

extern char const ((*ion_alignof_keyword));

extern char const ((*ion_typeof_keyword));

extern char const ((*ion_offsetof_keyword));

extern char const ((*ion_break_keyword));

extern char const ((*ion_continue_keyword));

extern char const ((*ion_return_keyword));

extern char const ((*ion_if_keyword));

extern char const ((*ion_else_keyword));

extern char const ((*ion_while_keyword));

extern char const ((*ion_do_keyword));

extern char const ((*ion_for_keyword));

extern char const ((*ion_switch_keyword));

extern char const ((*ion_case_keyword));

extern char const ((*ion_default_keyword));

extern char const ((*ion_import_keyword));

extern char const ((*ion_goto_keyword));

extern char const ((*ion_first_keyword));

extern char const ((*ion_last_keyword));

extern char const ((*(*ion_keywords)));

extern char const ((*ion_always_name));

extern char const ((*ion_foreign_name));

extern char const ((*ion_complete_name));

extern char const ((*ion_assert_name));

extern char const ((*ion_declare_note_name));

extern char const ((*ion_static_assert_name));

char const ((*ion_init_keyword(char const ((*keyword)))));

extern bool ion_keywords_inited;

void ion_init_keywords(void);

bool ion_is_keyword_name(char const ((*name)));

#define ION_TOKEN_EOF ((ion_TokenKind)(0))

#define ION_TOKEN_COLON ((ion_TokenKind)((ION_TOKEN_EOF) + (1)))

#define ION_TOKEN_LPAREN ((ion_TokenKind)((ION_TOKEN_COLON) + (1)))

#define ION_TOKEN_RPAREN ((ion_TokenKind)((ION_TOKEN_LPAREN) + (1)))

#define ION_TOKEN_LBRACE ((ion_TokenKind)((ION_TOKEN_RPAREN) + (1)))

#define ION_TOKEN_RBRACE ((ion_TokenKind)((ION_TOKEN_LBRACE) + (1)))

#define ION_TOKEN_LBRACKET ((ion_TokenKind)((ION_TOKEN_RBRACE) + (1)))

#define ION_TOKEN_RBRACKET ((ion_TokenKind)((ION_TOKEN_LBRACKET) + (1)))

#define ION_TOKEN_COMMA ((ion_TokenKind)((ION_TOKEN_RBRACKET) + (1)))

#define ION_TOKEN_DOT ((ion_TokenKind)((ION_TOKEN_COMMA) + (1)))

#define ION_TOKEN_AT ((ion_TokenKind)((ION_TOKEN_DOT) + (1)))

#define ION_TOKEN_POUND ((ion_TokenKind)((ION_TOKEN_AT) + (1)))

#define ION_TOKEN_ELLIPSIS ((ion_TokenKind)((ION_TOKEN_POUND) + (1)))

#define ION_TOKEN_QUESTION ((ion_TokenKind)((ION_TOKEN_ELLIPSIS) + (1)))

#define ION_TOKEN_SEMICOLON ((ion_TokenKind)((ION_TOKEN_QUESTION) + (1)))

#define ION_TOKEN_KEYWORD ((ion_TokenKind)((ION_TOKEN_SEMICOLON) + (1)))

#define ION_TOKEN_INT ((ion_TokenKind)((ION_TOKEN_KEYWORD) + (1)))

#define ION_TOKEN_FLOAT ((ion_TokenKind)((ION_TOKEN_INT) + (1)))

#define ION_TOKEN_STR ((ion_TokenKind)((ION_TOKEN_FLOAT) + (1)))

#define ION_TOKEN_NAME ((ion_TokenKind)((ION_TOKEN_STR) + (1)))

#define ION_TOKEN_NEG ((ion_TokenKind)((ION_TOKEN_NAME) + (1)))

#define ION_TOKEN_NOT ((ion_TokenKind)((ION_TOKEN_NEG) + (1)))

#define ION_TOKEN_FIRST_MUL ((ion_TokenKind)((ION_TOKEN_NOT) + (1)))

#define ION_TOKEN_MUL ((ion_TokenKind)(ION_TOKEN_FIRST_MUL))

#define ION_TOKEN_DIV ((ion_TokenKind)((ION_TOKEN_MUL) + (1)))

#define ION_TOKEN_MOD ((ion_TokenKind)((ION_TOKEN_DIV) + (1)))

#define ION_TOKEN_AND ((ion_TokenKind)((ION_TOKEN_MOD) + (1)))

#define ION_TOKEN_LSHIFT ((ion_TokenKind)((ION_TOKEN_AND) + (1)))

#define ION_TOKEN_RSHIFT ((ion_TokenKind)((ION_TOKEN_LSHIFT) + (1)))

#define ION_TOKEN_LAST_MUL ((ion_TokenKind)(ION_TOKEN_RSHIFT))

#define ION_TOKEN_FIRST_ADD ((ion_TokenKind)((ION_TOKEN_LAST_MUL) + (1)))

#define ION_TOKEN_ADD ((ion_TokenKind)(ION_TOKEN_FIRST_ADD))

#define ION_TOKEN_SUB ((ion_TokenKind)((ION_TOKEN_ADD) + (1)))

#define ION_TOKEN_XOR ((ion_TokenKind)((ION_TOKEN_SUB) + (1)))

#define ION_TOKEN_OR ((ion_TokenKind)((ION_TOKEN_XOR) + (1)))

#define ION_TOKEN_LAST_ADD ((ion_TokenKind)(ION_TOKEN_OR))

#define ION_TOKEN_FIRST_CMP ((ion_TokenKind)((ION_TOKEN_LAST_ADD) + (1)))

#define ION_TOKEN_EQ ((ion_TokenKind)(ION_TOKEN_FIRST_CMP))

#define ION_TOKEN_NOTEQ ((ion_TokenKind)((ION_TOKEN_EQ) + (1)))

#define ION_TOKEN_LT ((ion_TokenKind)((ION_TOKEN_NOTEQ) + (1)))

#define ION_TOKEN_GT ((ion_TokenKind)((ION_TOKEN_LT) + (1)))

#define ION_TOKEN_LTEQ ((ion_TokenKind)((ION_TOKEN_GT) + (1)))

#define ION_TOKEN_GTEQ ((ion_TokenKind)((ION_TOKEN_LTEQ) + (1)))

#define ION_TOKEN_LAST_CMP ((ion_TokenKind)(ION_TOKEN_GTEQ))

#define ION_TOKEN_AND_AND ((ion_TokenKind)((ION_TOKEN_LAST_CMP) + (1)))

#define ION_TOKEN_OR_OR ((ion_TokenKind)((ION_TOKEN_AND_AND) + (1)))

#define ION_TOKEN_FIRST_ASSIGN ((ion_TokenKind)((ION_TOKEN_OR_OR) + (1)))

#define ION_TOKEN_ASSIGN ((ion_TokenKind)(ION_TOKEN_FIRST_ASSIGN))

#define ION_TOKEN_ADD_ASSIGN ((ion_TokenKind)((ION_TOKEN_ASSIGN) + (1)))

#define ION_TOKEN_SUB_ASSIGN ((ion_TokenKind)((ION_TOKEN_ADD_ASSIGN) + (1)))

#define ION_TOKEN_OR_ASSIGN ((ion_TokenKind)((ION_TOKEN_SUB_ASSIGN) + (1)))

#define ION_TOKEN_AND_ASSIGN ((ion_TokenKind)((ION_TOKEN_OR_ASSIGN) + (1)))

#define ION_TOKEN_XOR_ASSIGN ((ion_TokenKind)((ION_TOKEN_AND_ASSIGN) + (1)))

#define ION_TOKEN_LSHIFT_ASSIGN ((ion_TokenKind)((ION_TOKEN_XOR_ASSIGN) + (1)))

#define ION_TOKEN_RSHIFT_ASSIGN ((ion_TokenKind)((ION_TOKEN_LSHIFT_ASSIGN) + (1)))

#define ION_TOKEN_MUL_ASSIGN ((ion_TokenKind)((ION_TOKEN_RSHIFT_ASSIGN) + (1)))

#define ION_TOKEN_DIV_ASSIGN ((ion_TokenKind)((ION_TOKEN_MUL_ASSIGN) + (1)))

#define ION_TOKEN_MOD_ASSIGN ((ion_TokenKind)((ION_TOKEN_DIV_ASSIGN) + (1)))

#define ION_TOKEN_LAST_ASSIGN ((ion_TokenKind)(ION_TOKEN_MOD_ASSIGN))

#define ION_TOKEN_INC ((ion_TokenKind)((ION_TOKEN_LAST_ASSIGN) + (1)))

#define ION_TOKEN_DEC ((ion_TokenKind)((ION_TOKEN_INC) + (1)))

#define ION_TOKEN_COLON_ASSIGN ((ion_TokenKind)((ION_TOKEN_DEC) + (1)))

#define ION_NUM_TOKEN_KINDS ((ion_TokenKind)((ION_TOKEN_COLON_ASSIGN) + (1)))

#define ION_MOD_NONE ((ion_TokenMod)(0))

#define ION_MOD_HEX ((ion_TokenMod)((ION_MOD_NONE) + (1)))

#define ION_MOD_BIN ((ion_TokenMod)((ION_MOD_HEX) + (1)))

#define ION_MOD_OCT ((ion_TokenMod)((ION_MOD_BIN) + (1)))

#define ION_MOD_CHAR ((ion_TokenMod)((ION_MOD_OCT) + (1)))

#define ION_MOD_MULTILINE ((ion_TokenMod)((ION_MOD_CHAR) + (1)))

#define ION_SUFFIX_NONE ((ion_TokenSuffix)(0))

#define ION_SUFFIX_D ((ion_TokenSuffix)((ION_SUFFIX_NONE) + (1)))

#define ION_SUFFIX_U ((ion_TokenSuffix)((ION_SUFFIX_D) + (1)))

#define ION_SUFFIX_L ((ion_TokenSuffix)((ION_SUFFIX_U) + (1)))

#define ION_SUFFIX_UL ((ion_TokenSuffix)((ION_SUFFIX_L) + (1)))

#define ION_SUFFIX_LL ((ion_TokenSuffix)((ION_SUFFIX_UL) + (1)))

#define ION_SUFFIX_ULL ((ion_TokenSuffix)((ION_SUFFIX_LL) + (1)))

extern char const ((*(ion_token_suffix_names[7])));

extern char const ((*(ion_token_kind_names[54])));

char const ((*ion_token_kind_name(ion_TokenKind kind)));

extern ion_TokenKind (ion_assign_token_to_binary_token[ION_NUM_TOKEN_KINDS]);

extern ion_SrcPos ion_pos_builtin;

struct ion_Token {
    ion_TokenKind kind;
    ion_TokenMod mod;
    ion_TokenSuffix suffix;
    ion_SrcPos pos;
    char const ((*start));
    char const ((*end));
    union {
        ullong int_val;
        double float_val;
        char const ((*str_val));
        char const ((*name));
    };
};

extern ion_Token ion_token;

extern char const ((*ion_stream));

extern char const ((*ion_line_start));

void ion_vnotice(char const ((*level)), ion_SrcPos pos, char const ((*fmt)), va_list args);

void ion_warning(ion_SrcPos pos, char const ((*fmt)), ...);

void ion_verror(ion_SrcPos pos, char const ((*fmt)), va_list args);

void ion_error(ion_SrcPos pos, char const ((*fmt)), ...);

void ion_fatal_error(ion_SrcPos pos, char const ((*fmt)), ...);

char const ((*ion_token_info(void)));

extern uint8_t (ion_char_to_digit[256]);

void ion_scan_int(void);

void ion_scan_float(void);

extern char (ion_escape_to_char[256]);

int ion_scan_hex_escape(void);

void ion_scan_char(void);

void ion_scan_str(void);

void ion_next_token(void);

void ion_init_stream(char const ((*name)), char const ((*buf)));

bool ion_is_token(ion_TokenKind kind);

bool ion_is_token_eof(void);

bool ion_is_token_name(char const ((*name)));

bool ion_is_keyword(char const ((*name)));

bool ion_match_keyword(char const ((*name)));

bool ion_match_token(ion_TokenKind kind);

bool ion_expect_token(ion_TokenKind kind);

extern bool ion_flag_verbose;

extern bool ion_flag_lazy;

extern bool ion_flag_nosourcemap;

extern bool ion_flag_notypeinfo;

extern bool ion_flag_fullgen;

void ion_path_normalize(char (*path));

void ion_path_copy(char (path[MAX_PATH]), char const ((*src)));

void ion_path_join(char (path[MAX_PATH]), char const ((*src)));

char (*ion_path_file(char (path[MAX_PATH])));

char (*ion_path_ext(char (path[MAX_PATH])));

bool ion_dir_excluded(ion_DirListIter (*iter));

bool ion_dir_list_subdir(ion_DirListIter (*iter));

char const ((*(*ion_dir_list_buf(char const ((*filespec))))));

typedef int ion_FlagKind;

#define ION_FLAG_BOOL ((ion_FlagKind)(0))

#define ION_FLAG_STR ((ion_FlagKind)((ION_FLAG_BOOL) + (1)))

#define ION_FLAG_ENUM ((ion_FlagKind)((ION_FLAG_STR) + (1)))

extern ion_FlagDef (*ion_flag_defs);

void ion_add_flag_bool(char const ((*name)), bool (*ptr), char const ((*help)));

void ion_add_flag_str(char const ((*name)), char const ((*(*ptr))), char const ((*arg_name)), char const ((*help)));

void ion_add_flag_enum(char const ((*name)), int (*ptr), char const ((*help)), char const ((*(*options))), int num_options);

ion_FlagDef (*ion_get_flag_def(char const ((*name))));

void ion_print_flags_usage(void);

char const ((*ion_parse_flags(int (*argc_ptr), char const ((*(*(*argv_ptr)))))));

void ion_path_absolute(char (path[MAX_PATH]));

void ion_dir_list_free(ion_DirListIter (*iter));

void ion_dir_list_next(ion_DirListIter (*iter));

void ion_dir_list(ion_DirListIter (*iter), char const ((*path)));

ion_Typespec (*ion_parse_type_func_param(void));

ion_Typespec (*ion_parse_type_func(void));

ion_Typespec (*ion_parse_type_base(void));

ion_Typespec (*ion_parse_type(void));

struct ion_CompoundField {
    ion_CompoundFieldKind kind;
    ion_SrcPos pos;
    ion_Expr (*init);
    union {
        char const ((*name));
        ion_Expr (*index);
    };
};

ion_CompoundField ion_parse_expr_compound_field(void);

ion_Expr (*ion_parse_expr_compound(ion_Typespec (*type)));

ion_Expr (*ion_parse_expr_operand(void));

ion_Expr (*ion_parse_expr_base(void));

bool ion_is_unary_op(void);

ion_Expr (*ion_parse_expr_unary(void));

bool ion_is_mul_op(void);

ion_Expr (*ion_parse_expr_mul(void));

bool ion_is_add_op(void);

ion_Expr (*ion_parse_expr_add(void));

bool ion_is_cmp_op(void);

ion_Expr (*ion_parse_expr_cmp(void));

ion_Expr (*ion_parse_expr_and(void));

ion_Expr (*ion_parse_expr_or(void));

ion_Expr (*ion_parse_expr_ternary(void));

ion_Expr (*ion_parse_expr(void));

ion_Expr (*ion_parse_paren_expr(void));

ion_StmtList ion_parse_stmt_block(void);

ion_Stmt (*ion_parse_stmt_if(ion_SrcPos pos));

ion_Stmt (*ion_parse_stmt_while(ion_SrcPos pos));

ion_Stmt (*ion_parse_stmt_do_while(ion_SrcPos pos));

bool ion_is_assign_op(void);

ion_Stmt (*ion_parse_init_stmt(ion_Expr (*left)));

ion_Stmt (*ion_parse_simple_stmt(void));

ion_Stmt (*ion_parse_stmt_for(ion_SrcPos pos));

struct ion_SwitchCasePattern {
    ion_Expr (*start);
    ion_Expr (*end);
};

ion_SwitchCasePattern ion_parse_switch_case_pattern(void);

struct ion_SwitchCase {
    ion_SwitchCasePattern (*patterns);
    size_t num_patterns;
    bool is_default;
    ion_StmtList block;
};

ion_SwitchCase ion_parse_stmt_switch_case(void);

ion_Stmt (*ion_parse_stmt_switch(ion_SrcPos pos));

ion_Stmt (*ion_parse_stmt(void));

char const ((*ion_parse_name(void)));

struct ion_EnumItem {
    ion_SrcPos pos;
    char const ((*name));
    ion_Expr (*init);
};

ion_EnumItem ion_parse_decl_enum_item(void);

ion_Decl (*ion_parse_decl_enum(ion_SrcPos pos));

struct ion_AggregateItem {
    ion_SrcPos pos;
    ion_AggregateItemKind kind;
    union {
        struct {
            char const ((*(*names)));
            size_t num_names;
            ion_Typespec (*type);
        };
        ion_Aggregate (*subaggregate);
    };
};

ion_AggregateItem ion_parse_decl_aggregate_item(void);

ion_Aggregate (*ion_parse_aggregate(ion_AggregateKind kind));

ion_Decl (*ion_parse_decl_aggregate(ion_SrcPos pos, ion_DeclKind kind));

ion_Decl (*ion_parse_decl_var(ion_SrcPos pos));

ion_Decl (*ion_parse_decl_const(ion_SrcPos pos));

ion_Decl (*ion_parse_decl_typedef(ion_SrcPos pos));

struct ion_FuncParam {
    ion_SrcPos pos;
    char const ((*name));
    ion_Typespec (*type);
};

ion_FuncParam ion_parse_decl_func_param(void);

ion_Decl (*ion_parse_decl_func(ion_SrcPos pos));

struct ion_NoteArg {
    ion_SrcPos pos;
    char const ((*name));
    ion_Expr (*expr);
};

ion_NoteArg ion_parse_note_arg(void);

ion_Note ion_parse_note(void);

ion_Notes ion_parse_notes(void);

ion_Decl (*ion_parse_decl_note(ion_SrcPos pos));

ion_Decl (*ion_parse_decl_import(ion_SrcPos pos));

ion_Decl (*ion_parse_decl_opt(void));

ion_Decl (*ion_parse_decl(void));

ion_Decls (*ion_parse_decls(void));

typedef int ion_SymKind;

#define ION_SYM_NONE ((ion_SymKind)(0))

#define ION_SYM_VAR ((ion_SymKind)((ION_SYM_NONE) + (1)))

#define ION_SYM_CONST ((ion_SymKind)((ION_SYM_VAR) + (1)))

#define ION_SYM_FUNC ((ion_SymKind)((ION_SYM_CONST) + (1)))

#define ION_SYM_TYPE ((ion_SymKind)((ION_SYM_FUNC) + (1)))

#define ION_SYM_PACKAGE ((ion_SymKind)((ION_SYM_TYPE) + (1)))

typedef int ion_SymState;

#define ION_SYM_UNRESOLVED ((ion_SymState)(0))

#define ION_SYM_RESOLVING ((ion_SymState)((ION_SYM_UNRESOLVED) + (1)))

#define ION_SYM_RESOLVED ((ion_SymState)((ION_SYM_RESOLVING) + (1)))

#define ION_MAX_LOCAL_SYMS ((int)(1024))

extern ion_Package (*ion_current_package);

extern ion_Package (*ion_builtin_package);

extern ion_Map ion_package_map;

extern ion_Package (*(*ion_package_list));

#define ION_REACHABLE_NONE ((int)(0))

#define ION_REACHABLE_NATURAL ((int)((ION_REACHABLE_NONE) + (1)))

#define ION_REACHABLE_FORCED ((int)((ION_REACHABLE_NATURAL) + (1)))

extern uint8_t ion_reachable_phase;

ion_Sym (*ion_get_package_sym(ion_Package (*package), char const ((*name))));

void ion_add_package(ion_Package (*package));

ion_Package (*ion_enter_package(ion_Package (*new_package)));

void ion_leave_package(ion_Package (*old_package));

extern ion_Sym (*(*ion_reachable_syms));

extern ion_Sym (*(*ion_sorted_syms));

union ion_Val {
    bool b;
    char c;
    uchar uc;
    schar sc;
    short s;
    ushort us;
    int i;
    uint u;
    long l;
    ulong ul;
    llong ll;
    ullong ull;
    uintptr_t p;
};

struct ion_Sym {
    char const ((*name));
    ion_Package (*home_package);
    ion_SymKind kind;
    ion_SymState state;
    uint8_t reachable;
    ion_Decl (*decl);
    char const ((*external_name));
    union {
        struct {
            ion_Type (*type);
            ion_Val val;
        };
        ion_Package (*package);
    };
};

extern ion_Sym (ion_local_syms[ION_MAX_LOCAL_SYMS]);

extern ion_Sym (*ion_local_syms_end);

bool ion_is_local_sym(ion_Sym (*sym));

ion_Sym (*ion_sym_new(ion_SymKind kind, char const ((*name)), ion_Decl (*decl)));

void ion_process_decl_notes(ion_Decl (*decl), ion_Sym (*sym));

ion_Sym (*ion_sym_decl(ion_Decl (*decl)));

ion_Sym (*ion_sym_get_local(char const ((*name))));

ion_Sym (*ion_sym_get(char const ((*name))));

bool ion_sym_push_var(char const ((*name)), ion_Type (*type));

ion_Sym (*ion_sym_enter(void));

void ion_sym_leave(ion_Sym (*sym));

void ion_sym_global_put(char const ((*name)), ion_Sym (*sym));

ion_Sym (*ion_sym_global_type(char const ((*name)), ion_Type (*type)));

ion_Sym (*ion_sym_global_decl(ion_Decl (*decl)));

void ion_put_type_name(char (*(*buf)), ion_Type (*type));

char (*ion_get_type_name(ion_Type (*type)));

struct ion_Operand {
    ion_Type (*type);
    bool is_lvalue;
    bool is_const;
    ion_Val val;
};

extern ion_Operand ion_operand_null;

ion_Operand ion_operand_rvalue(ion_Type (*type));

ion_Operand ion_operand_lvalue(ion_Type (*type));

ion_Operand ion_operand_const(ion_Type (*type), ion_Val val);

ion_Type (*ion_type_decay(ion_Type (*type)));

ion_Operand ion_operand_decay(ion_Operand operand);

bool ion_is_convertible(ion_Operand (*operand), ion_Type (*dest));

bool ion_is_castable(ion_Operand (*operand), ion_Type (*dest));

bool ion_convert_operand(ion_Operand (*operand), ion_Type (*type));

bool ion_is_null_ptr(ion_Operand operand);

void ion_promote_operand(ion_Operand (*operand));

void ion_unify_arithmetic_operands(ion_Operand (*left), ion_Operand (*right));

extern ion_Map ion_resolved_val_map;

ion_Val ion_get_resolved_val(void (*ptr));

void ion_set_resolved_val(void (*ptr), ion_Val val);

extern ion_Map ion_resolved_type_map;

ion_Type (*ion_get_resolved_type(void (*ptr)));

void ion_set_resolved_type(void (*ptr), ion_Type (*type));

extern ion_Map ion_resolved_sym_map;

ion_Sym (*ion_get_resolved_sym(void const ((*ptr))));

void ion_set_resolved_sym(void const ((*ptr)), ion_Sym (*sym));

extern ion_Map ion_resolved_expected_type_map;

ion_Type (*ion_get_resolved_expected_type(ion_Expr (*expr)));

void ion_set_resolved_expected_type(ion_Expr (*expr), ion_Type (*type));

ion_Operand ion_resolve_expr(ion_Expr (*expr));

ion_Operand ion_resolve_expr_rvalue(ion_Expr (*expr));

ion_Operand ion_resolve_expected_expr_rvalue(ion_Expr (*expr), ion_Type (*expected_type));

ion_Type (*ion_resolve_typespec(ion_Typespec (*typespec)));

ion_Type (*ion_complete_aggregate(ion_Type (*type), ion_Aggregate (*aggregate)));

void ion_complete_type(ion_Type (*type));

ion_Type (*ion_resolve_typed_init(ion_SrcPos pos, ion_Type (*type), ion_Expr (*expr)));

ion_Type (*ion_resolve_init(ion_SrcPos pos, ion_Typespec (*typespec), ion_Expr (*expr)));

ion_Type (*ion_resolve_decl_var(ion_Decl (*decl)));

ion_Type (*ion_resolve_decl_const(ion_Decl (*decl), ion_Val (*val)));

ion_Type (*ion_resolve_decl_func(ion_Decl (*decl)));

#define ION_MAX_LABELS (256)

struct ion_Label {
    char const ((*name));
    ion_SrcPos pos;
    bool referenced;
    bool defined;
};

extern ion_Label (ion_labels[ION_MAX_LABELS]);

extern ion_Label (*ion_labels_end);

ion_Label (*ion_get_label(ion_SrcPos pos, char const ((*name))));

void ion_reference_label(ion_SrcPos pos, char const ((*name)));

void ion_define_label(ion_SrcPos pos, char const ((*name)));

void ion_resolve_labels(void);

bool ion_is_cond_operand(ion_Operand operand);

void ion_resolve_cond_expr(ion_Expr (*expr));

struct ion_StmtCtx {
    bool is_break_legal;
    bool is_continue_legal;
};

bool ion_resolve_stmt_block(ion_StmtList block, ion_Type (*ret_type), ion_StmtCtx ctx);

void ion_resolve_stmt_assign(ion_Stmt (*stmt));

void ion_resolve_stmt_init(ion_Stmt (*stmt));

void ion_resolve_static_assert(ion_Note note);

bool ion_resolve_stmt(ion_Stmt (*stmt), ion_Type (*ret_type), ion_StmtCtx ctx);

void ion_resolve_func_body(ion_Sym (*sym));

void ion_resolve_sym(ion_Sym (*sym));

void ion_finalize_sym(ion_Sym (*sym));

ion_Sym (*ion_resolve_name(char const ((*name))));

ion_Package (*ion_try_resolve_package(ion_Expr (*expr)));

ion_Operand ion_resolve_expr_field(ion_Expr (*expr));

llong ion_eval_unary_op_ll(ion_TokenKind op, llong val);

ullong ion_eval_unary_op_ull(ion_TokenKind op, ullong val);

llong ion_eval_binary_op_ll(ion_TokenKind op, llong left, llong right);

ullong ion_eval_binary_op_ull(ion_TokenKind op, ullong left, ullong right);

ion_Val ion_eval_unary_op(ion_TokenKind op, ion_Type (*type), ion_Val val);

ion_Val ion_eval_binary_op(ion_TokenKind op, ion_Type (*type), ion_Val left, ion_Val right);

ion_Operand ion_resolve_name_operand(ion_SrcPos pos, char const ((*name)));

ion_Operand ion_resolve_expr_name(ion_Expr (*expr));

ion_Operand ion_resolve_unary_op(ion_TokenKind op, ion_Operand operand);

ion_Operand ion_resolve_expr_unary(ion_Expr (*expr));

ion_Operand ion_resolve_binary_op(ion_TokenKind op, ion_Operand left, ion_Operand right);

ion_Operand ion_resolve_binary_arithmetic_op(ion_TokenKind op, ion_Operand left, ion_Operand right);

ion_Operand ion_resolve_expr_binary_op(ion_TokenKind op, char const ((*op_name)), ion_SrcPos pos, ion_Operand left, ion_Operand right);

ion_Operand ion_resolve_expr_binary(ion_Expr (*expr));

ion_Operand ion_resolve_expr_compound(ion_Expr (*expr), ion_Type (*expected_type));

ion_Operand ion_resolve_expr_call(ion_Expr (*expr));

ion_Operand ion_resolve_expr_ternary(ion_Expr (*expr), ion_Type (*expected_type));

ion_Operand ion_resolve_expr_index(ion_Expr (*expr));

ion_Operand ion_resolve_expr_cast(ion_Expr (*expr));

ion_Operand ion_resolve_expr_int(ion_Expr (*expr));

ion_Operand ion_resolve_expr_modify(ion_Expr (*expr));

ion_Operand ion_resolve_expected_expr(ion_Expr (*expr), ion_Type (*expected_type));

ion_Operand ion_resolve_const_expr(ion_Expr (*expr));

extern ion_Map ion_decl_note_names;

void ion_init_builtin_syms(void);

void ion_add_package_decls(ion_Package (*package));

bool ion_is_package_dir(char const ((*search_path)), char const ((*package_path)));

bool ion_copy_package_full_path(char (dest[MAX_PATH]), char const ((*package_path)));

ion_Package (*ion_import_package(char const ((*package_path))));

void ion_import_all_package_symbols(ion_Package (*package));

void ion_import_package_symbols(ion_Decl (*decl), ion_Package (*package));

void ion_process_package_imports(ion_Package (*package));

bool ion_parse_package(ion_Package (*package));

bool ion_compile_package(ion_Package (*package));

void ion_resolve_package_syms(ion_Package (*package));

void ion_finalize_reachable_syms(void);

typedef int ion_Os;

#define ION_OS_WIN32 ((ion_Os)(0))

#define ION_OS_LINUX ((ion_Os)((ION_OS_WIN32) + (1)))

#define ION_OS_OSX ((ion_Os)((ION_OS_LINUX) + (1)))

#define ION_NUM_OSES ((ion_Os)((ION_OS_OSX) + (1)))

extern char const ((*(ion_os_names[ION_NUM_OSES])));

typedef int ion_Arch;

#define ION_ARCH_X64 ((ion_Arch)(0))

#define ION_ARCH_X86 ((ion_Arch)((ION_ARCH_X64) + (1)))

#define ION_NUM_ARCHES ((ion_Arch)((ION_ARCH_X86) + (1)))

extern char const ((*(ion_arch_names[ION_NUM_ARCHES])));

extern int ion_target_os;

extern int ion_target_arch;

int ion_get_os(char const ((*name)));

int ion_get_arch(char const ((*name)));

struct ion_TypeMetrics {
    size_t size;
    size_t align;
    bool sign;
    ullong max;
};

void ion_init_default_type_metrics(ion_TypeMetrics (metrics[ION_NUM_CMPL_TYPE_KINDS]));

extern ion_TypeMetrics (ion_win32_x86_metrics[ION_NUM_CMPL_TYPE_KINDS]);

extern ion_TypeMetrics (ion_win32_x64_metrics[ION_NUM_CMPL_TYPE_KINDS]);

extern ion_TypeMetrics (ion_ilp32_metrics[ION_NUM_CMPL_TYPE_KINDS]);

extern ion_TypeMetrics (ion_lp64_metrics[ION_NUM_CMPL_TYPE_KINDS]);

void ion_init_target(void);

bool ion_is_excluded_target_filename(char const ((*name)));

extern ion_TypeMetrics (*ion_type_metrics);

struct ion_TypeAggregate {
    ion_TypeField (*fields);
    size_t num_fields;
};

struct ion_TypeFunc {
    ion_Type (*(*params));
    size_t num_params;
    bool has_varargs;
    ion_Type (*ret);
};

struct ion_Type {
    ion_CompilerTypeKind kind;
    size_t size;
    size_t align;
    ion_Sym (*sym);
    ion_Type (*base);
    int typeid;
    bool nonmodifiable;
    union {
        size_t num_elems;
        ion_TypeAggregate aggregate;
        ion_TypeFunc function;
    };
};

extern ion_Type (*ion_type_void);

extern ion_Type (*ion_type_bool);

extern ion_Type (*ion_type_char);

extern ion_Type (*ion_type_uchar);

extern ion_Type (*ion_type_schar);

extern ion_Type (*ion_type_short);

extern ion_Type (*ion_type_ushort);

extern ion_Type (*ion_type_int);

extern ion_Type (*ion_type_uint);

extern ion_Type (*ion_type_long);

extern ion_Type (*ion_type_ulong);

extern ion_Type (*ion_type_llong);

extern ion_Type (*ion_type_ullong);

extern ion_Type (*ion_type_float);

extern ion_Type (*ion_type_double);

extern int ion_next_typeid;

extern ion_Type (*ion_type_uintptr);

extern ion_Type (*ion_type_usize);

extern ion_Type (*ion_type_ssize);

extern ion_Map ion_typeid_map;

ion_Type (*ion_get_type_from_typeid(int typeid));

void ion_register_typeid(ion_Type (*type));

ion_Type (*ion_type_alloc(TypeKind kind));

bool ion_is_ptr_type(ion_Type (*type));

bool ion_is_func_type(ion_Type (*type));

bool ion_is_ptr_like_type(ion_Type (*type));

bool ion_is_const_type(ion_Type (*type));

bool ion_is_array_type(ion_Type (*type));

bool ion_is_incomplete_array_type(ion_Type (*type));

bool ion_is_integer_type(ion_Type (*type));

bool ion_is_floating_type(ion_Type (*type));

bool ion_is_arithmetic_type(ion_Type (*type));

bool ion_is_scalar_type(ion_Type (*type));

bool ion_is_aggregate_type(ion_Type (*type));

bool ion_is_signed_type(ion_Type (*type));

extern char const ((*(ion_type_names[ION_NUM_CMPL_TYPE_KINDS])));

extern int (ion_type_ranks[ION_NUM_CMPL_TYPE_KINDS]);

int ion_type_rank(ion_Type (*type));

ion_Type (*ion_unsigned_type(ion_Type (*type)));

size_t ion_type_sizeof(ion_Type (*type));

size_t ion_type_alignof(ion_Type (*type));

extern ion_Map ion_cached_ptr_types;

ion_Type (*ion_type_ptr(ion_Type (*base)));

extern ion_Map ion_cached_const_types;

ion_Type (*ion_type_const(ion_Type (*base)));

ion_Type (*ion_unqualify_type(ion_Type (*type)));

extern ion_Map ion_cached_array_types;

ion_Type (*ion_type_array(ion_Type (*base), size_t num_elems));

extern ion_Map ion_cached_func_types;

ion_Type (*ion_type_func(ion_Type (*(*params)), size_t num_params, ion_Type (*ret), bool has_varargs));

bool ion_has_duplicate_fields(ion_Type (*type));

void ion_add_type_fields(ion_TypeField (*(*fields)), ion_Type (*type), size_t offset);

void ion_type_complete_struct(ion_Type (*type), ion_TypeField (*fields), size_t num_fields);

void ion_type_complete_union(ion_Type (*type), ion_TypeField (*fields), size_t num_fields);

ion_Type (*ion_type_incomplete(ion_Sym (*sym)));

ion_Type (*ion_type_enum(ion_Sym (*sym), ion_Type (*base)));

void ion_init_builtin_type(ion_Type (*type));

void ion_init_builtin_types(void);

int ion_aggregate_item_field_index(ion_Type (*type), char const ((*name)));

ion_Type (*ion_aggregate_item_field_type_from_index(ion_Type (*type), int index));

ion_Type (*ion_aggregate_item_field_type_from_name(ion_Type (*type), char const ((*name))));


struct TypeFieldInfo {
    char const ((*name));
    typeid type;
    int offset;
};

struct Any {
    void (*ptr);
    typeid type;
};

struct ion_StmtIf {
    ion_Stmt (*init);
    ion_Expr (*cond);
    ion_StmtList then_block;
    ion_ElseIf (*elseifs);
    size_t num_elseifs;
    ion_StmtList else_block;
};

struct ion_StmtWhile {
    ion_Expr (*cond);
    ion_StmtList block;
};

struct ion_StmtFor {
    ion_Stmt (*init);
    ion_Expr (*cond);
    ion_Stmt (*next);
    ion_StmtList block;
};

struct ion_StmtSwitch {
    ion_Expr (*expr);
    ion_SwitchCase (*cases);
    size_t num_cases;
};

struct ion_StmtAssign {
    ion_TokenKind op;
    ion_Expr (*left);
    ion_Expr (*right);
};

struct ion_StmtInit {
    char const ((*name));
    ion_Typespec (*type);
    ion_Expr (*expr);
};

struct ion_Stmt {
    ion_StmtKind kind;
    ion_Notes notes;
    ion_SrcPos pos;
    union {
        ion_Note note;
        ion_Expr (*expr);
        ion_Decl (*decl);
        ion_StmtIf if_stmt;
        ion_StmtWhile while_stmt;
        ion_StmtFor for_stmt;
        ion_StmtSwitch switch_stmt;
        ion_StmtList block;
        ion_StmtAssign assign;
        ion_StmtInit init;
        char const ((*label));
    };
};

struct ion_TypespecFunc {
    ion_Typespec (*(*args));
    size_t num_args;
    bool has_varargs;
    ion_Typespec (*ret);
};

struct ion_Typespec {
    ion_TypespecKind kind;
    ion_SrcPos pos;
    ion_Typespec (*base);
    union {
        char const ((*name));
        ion_TypespecFunc function;
        ion_Expr (*num_elems);
    };
};

struct ion_ExprParen {
    ion_Expr (*expr);
};

struct ion_ExprIntLit {
    ullong val;
    ion_TokenMod mod;
    ion_TokenSuffix suffix;
};

struct ion_ExprFloatLit {
    char const ((*start));
    char const ((*end));
    double val;
    ion_TokenSuffix suffix;
};

struct ion_ExprStrLit {
    char const ((*val));
    ion_TokenMod mod;
};

struct ion_ExprOffsetofField {
    ion_Typespec (*type);
    char const ((*name));
};

struct ion_ExprCompound {
    ion_Typespec (*type);
    ion_CompoundField (*fields);
    size_t num_fields;
};

struct ion_ExprCast {
    ion_Typespec (*type);
    ion_Expr (*expr);
};

struct ion_ExprModify {
    ion_TokenKind op;
    bool post;
    ion_Expr (*expr);
};

struct ion_ExprUnary {
    ion_TokenKind op;
    ion_Expr (*expr);
};

struct ion_ExprBinary {
    ion_TokenKind op;
    ion_Expr (*left);
    ion_Expr (*right);
};

struct ion_ExprTernary {
    ion_Expr (*cond);
    ion_Expr (*then_expr);
    ion_Expr (*else_expr);
};

struct ion_ExprCall {
    ion_Expr (*expr);
    ion_Expr (*(*args));
    size_t num_args;
};

struct ion_ExprIndex {
    ion_Expr (*expr);
    ion_Expr (*index);
};

struct ion_ExprField {
    ion_Expr (*expr);
    char const ((*name));
};

struct ion_Expr {
    ion_ExprKind kind;
    ion_SrcPos pos;
    union {
        ion_ExprParen paren;
        ion_ExprIntLit int_lit;
        ion_ExprFloatLit float_lit;
        ion_ExprStrLit str_lit;
        char const ((*name));
        ion_Expr (*sizeof_expr);
        ion_Typespec (*sizeof_type);
        ion_Expr (*typeof_expr);
        ion_Typespec (*typeof_type);
        ion_Expr (*alignof_expr);
        ion_Typespec (*alignof_type);
        ion_ExprOffsetofField offsetof_field;
        ion_ExprCompound compound;
        ion_ExprCast cast;
        ion_ExprModify modify;
        ion_ExprUnary unary;
        ion_ExprBinary binary;
        ion_ExprTernary ternary;
        ion_ExprCall call;
        ion_ExprIndex index;
        ion_ExprField field;
    };
};

struct ion_Decls {
    ion_Decl (*(*decls));
    size_t num_decls;
};

struct ion_DeclEnum {
    ion_Typespec (*type);
    ion_EnumItem (*items);
    size_t num_items;
};

struct ion_DeclFunc {
    ion_FuncParam (*params);
    size_t num_params;
    ion_Typespec (*ret_type);
    bool has_varargs;
    ion_StmtList block;
};

struct ion_DeclTypedef {
    ion_Typespec (*type);
};

struct ion_DeclVar {
    ion_Typespec (*type);
    ion_Expr (*expr);
};

struct ion_DeclImport {
    bool is_relative;
    char const ((*(*names)));
    size_t num_names;
    bool import_all;
    ion_ImportItem (*items);
    size_t num_items;
};

struct ion_Decl {
    ion_DeclKind kind;
    ion_SrcPos pos;
    char const ((*name));
    ion_Notes notes;
    bool is_incomplete;
    union {
        ion_Note note;
        ion_DeclEnum enum_decl;
        ion_Aggregate (*aggregate);
        ion_DeclFunc function;
        ion_DeclTypedef typedef_decl;
        ion_DeclVar var_decl;
        ion_DeclVar const_decl;
        ion_DeclImport import_decl;
    };
};

struct ion_Aggregate {
    ion_SrcPos pos;
    ion_AggregateKind kind;
    ion_AggregateItem (*items);
    size_t num_items;
};

struct ion_ImportItem {
    char const ((*name));
    char const ((*rename));
};

struct ion_ElseIf {
    ion_Expr (*cond);
    ion_StmtList block;
};

struct ion_BufHdr {
    size_t len;
    size_t cap;
    char (buf[1]);
};

struct ion_Intern {
    size_t len;
    ion_Intern (*next);
    char (str[1]);
};

struct ion_Package {
    char const ((*path));
    char (full_path[MAX_PATH]);
    ion_Decl (*(*decls));
    size_t num_decls;
    ion_Map syms_map;
    ion_Sym (*(*syms));
    char const ((*external_name));
    bool always_reachable;
};

struct ion_TypeField {
    char const ((*name));
    ion_Type (*type);
    size_t offset;
};

struct ion_DirListIter {
    bool valid;
    bool error;
    char (base[MAX_PATH]);
    char (name[MAX_PATH]);
    size_t size;
    bool is_dir;
    void (*handle);
};

struct ion_FlagDefPtr {
    int (*i);
    bool (*b);
    char const ((*(*s)));
};

struct ion_FlagDef {
    ion_FlagKind kind;
    char const ((*name));
    char const ((*help));
    char const ((*(*options)));
    char const ((*arg_name));
    int num_options;
    ion_FlagDefPtr ptr;
};

struct ion_CachedArrayType {
    ion_Type (*type);
    ion_CachedArrayType (*next);
};

struct ion_CachedFuncType {
    ion_Type (*type);
    ion_CachedFuncType (*next);
};

// Typeinfo
#define TYPEID0(index, kind) ((ullong)(index) | ((ullong)(kind) << 24))
#define TYPEID(index, kind, ...) ((ullong)(index) | ((ullong)sizeof(__VA_ARGS__) << 32) | ((ullong)(kind) << 24))

int num_typeinfos;
const TypeInfo **typeinfos;

// Definitions
int main(int argc, char const ((*(*argv)))) {
    return ion_ion_main(argc, argv, ion_gen_all, "c");
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

ion_Arena ion_ast_arena;
void (*ion_ast_alloc(size_t size)) {
    assert((size) != (0));
    void (*ptr) = ion_arena_alloc(&(ion_ast_arena), size);
    memset(ptr, 0, size);
    return ptr;
}

void (*ion_ast_dup(void const ((*src)), size_t size)) {
    if ((size) == (0)) {
        return NULL;
    }
    void (*ptr) = ion_arena_alloc(&(ion_ast_arena), size);
    memcpy(ptr, src, size);
    return ptr;
}

ion_Note ion_new_note(ion_SrcPos pos, char const ((*name)), ion_NoteArg (*args), size_t num_args) {
    return (ion_Note){.pos = pos, .name = name, .args = ion_ast_dup(args, (num_args) * (sizeof(*(args)))), .num_args = num_args};
}

ion_Notes ion_new_notes(ion_Note (*notes), size_t num_notes) {
    return (ion_Notes){ion_ast_dup(notes, (num_notes) * (sizeof(*(notes)))), num_notes};
}

ion_StmtList ion_new_stmt_list(ion_SrcPos pos, ion_Stmt (*(*stmts)), size_t num_stmts) {
    return (ion_StmtList){pos, ion_ast_dup(stmts, (num_stmts) * (sizeof(*(stmts)))), num_stmts};
}

ion_Typespec (*ion_new_typespec(ion_TypespecKind kind, ion_SrcPos pos)) {
    ion_Typespec (*t) = ion_ast_alloc(sizeof(ion_Typespec));
    t->kind = kind;
    t->pos = pos;
    return t;
}

ion_Typespec (*ion_new_typespec_name(ion_SrcPos pos, char const ((*name)))) {
    ion_Typespec (*t) = ion_new_typespec(ION_TYPESPEC_NAME, pos);
    t->name = name;
    return t;
}

ion_Typespec (*ion_new_typespec_ptr(ion_SrcPos pos, ion_Typespec (*base))) {
    ion_Typespec (*t) = ion_new_typespec(ION_TYPESPEC_PTR, pos);
    t->base = base;
    return t;
}

ion_Typespec (*ion_new_typespec_const(ion_SrcPos pos, ion_Typespec (*base))) {
    ion_Typespec (*t) = ion_new_typespec(ION_TYPESPEC_CONST, pos);
    t->base = base;
    return t;
}

ion_Typespec (*ion_new_typespec_array(ion_SrcPos pos, ion_Typespec (*elem), ion_Expr (*size))) {
    ion_Typespec (*t) = ion_new_typespec(ION_TYPESPEC_ARRAY, pos);
    t->base = elem;
    t->num_elems = size;
    return t;
}

ion_Typespec (*ion_new_typespec_func(ion_SrcPos pos, ion_Typespec (*(*args)), size_t num_args, ion_Typespec (*ret), bool has_varargs)) {
    ion_Typespec (*t) = ion_new_typespec(ION_TYPESPEC_FUNC, pos);
    t->function.args = ion_ast_dup(args, (num_args) * (sizeof(*(args))));
    t->function.num_args = num_args;
    t->function.ret = ret;
    t->function.has_varargs = has_varargs;
    return t;
}

ion_Decls (*ion_new_decls(ion_Decl (*(*decls)), size_t num_decls)) {
    ion_Decls (*d) = ion_ast_alloc(sizeof(ion_Decls));
    d->decls = ion_ast_dup(decls, (num_decls) * (sizeof(*(decls))));
    d->num_decls = num_decls;
    return d;
}

ion_Decl (*ion_new_decl(ion_DeclKind kind, ion_SrcPos pos, char const ((*name)))) {
    ion_Decl (*d) = ion_ast_alloc(sizeof(ion_Decl));
    d->kind = kind;
    d->pos = pos;
    d->name = name;
    return d;
}

ion_Note (*ion_get_decl_note(ion_Decl (*decl), char const ((*name)))) {
    for (size_t i = 0; (i) < (decl->notes.num_notes); (i)++) {
        ion_Note (*note) = (decl->notes.notes) + (i);
        if ((note->name) == (name)) {
            return note;
        }
    }
    return NULL;
}

bool ion_is_decl_foreign(ion_Decl (*decl)) {
    return (ion_get_decl_note(decl, ion_foreign_name)) != (NULL);
}

ion_Decl (*ion_new_decl_enum(ion_SrcPos pos, char const ((*name)), ion_Typespec (*type), ion_EnumItem (*items), size_t num_items)) {
    ion_Decl (*d) = ion_new_decl(ION_DECL_ENUM, pos, name);
    d->enum_decl.type = type;
    d->enum_decl.items = ion_ast_dup(items, (num_items) * (sizeof(*(items))));
    d->enum_decl.num_items = num_items;
    return d;
}

ion_Aggregate (*ion_new_aggregate(ion_SrcPos pos, ion_AggregateKind kind, ion_AggregateItem (*items), size_t num_items)) {
    ion_Aggregate (*aggregate) = ion_ast_alloc(sizeof(ion_Aggregate));
    aggregate->pos = pos;
    aggregate->kind = kind;
    aggregate->items = ion_ast_dup(items, (num_items) * (sizeof(*(items))));
    aggregate->num_items = num_items;
    return aggregate;
}

ion_Decl (*ion_new_decl_aggregate(ion_SrcPos pos, ion_DeclKind kind, char const ((*name)), ion_Aggregate (*aggregate))) {
    assert(((kind) == (ION_DECL_STRUCT)) || ((kind) == (ION_DECL_UNION)));
    ion_Decl (*d) = ion_new_decl(kind, pos, name);
    d->aggregate = aggregate;
    return d;
}

ion_Decl (*ion_new_decl_var(ion_SrcPos pos, char const ((*name)), ion_Typespec (*type), ion_Expr (*expr))) {
    ion_Decl (*d) = ion_new_decl(ION_DECL_VAR, pos, name);
    d->var_decl.type = type;
    d->var_decl.expr = expr;
    return d;
}

ion_Decl (*ion_new_decl_func(ion_SrcPos pos, char const ((*name)), ion_FuncParam (*params), size_t num_params, ion_Typespec (*ret_type), bool has_varargs, ion_StmtList block)) {
    ion_Decl (*d) = ion_new_decl(ION_DECL_FUNC, pos, name);
    d->function.params = ion_ast_dup(params, (num_params) * (sizeof(*(params))));
    d->function.num_params = num_params;
    d->function.ret_type = ret_type;
    d->function.has_varargs = has_varargs;
    d->function.block = block;
    return d;
}

ion_Decl (*ion_new_decl_const(ion_SrcPos pos, char const ((*name)), ion_Typespec (*type), ion_Expr (*expr))) {
    ion_Decl (*d) = ion_new_decl(ION_DECL_CONST, pos, name);
    d->const_decl.type = type;
    d->const_decl.expr = expr;
    return d;
}

ion_Decl (*ion_new_decl_typedef(ion_SrcPos pos, char const ((*name)), ion_Typespec (*type))) {
    ion_Decl (*d) = ion_new_decl(ION_DECL_TYPEDEF, pos, name);
    d->typedef_decl.type = type;
    return d;
}

ion_Decl (*ion_new_decl_note(ion_SrcPos pos, ion_Note note)) {
    ion_Decl (*d) = ion_new_decl(ION_DECL_NOTE, pos, NULL);
    d->note = note;
    return d;
}

ion_Decl (*ion_new_decl_import(ion_SrcPos pos, char const ((*rename_name)), bool is_relative, char const ((*(*names))), size_t num_names, bool import_all, ion_ImportItem (*items), size_t num_items)) {
    ion_Decl (*d) = ion_new_decl(ION_DECL_IMPORT, pos, NULL);
    d->name = rename_name;
    d->import_decl.is_relative = is_relative;
    d->import_decl.names = ion_ast_dup(names, (num_names) * (sizeof(*(names))));
    d->import_decl.num_names = num_names;
    d->import_decl.import_all = import_all;
    d->import_decl.items = ion_ast_dup(items, (num_items) * (sizeof(*(items))));
    d->import_decl.num_items = num_items;
    return d;
}

ion_Expr (*ion_new_expr(ion_ExprKind kind, ion_SrcPos pos)) {
    ion_Expr (*e) = ion_ast_alloc(sizeof(ion_Expr));
    e->kind = kind;
    e->pos = pos;
    return e;
}

ion_Expr (*ion_new_expr_paren(ion_SrcPos pos, ion_Expr (*expr))) {
    ion_Expr (*e) = ion_new_expr(ION_EXPR_PAREN, pos);
    e->paren.expr = expr;
    return e;
}

ion_Expr (*ion_new_expr_sizeof_expr(ion_SrcPos pos, ion_Expr (*expr))) {
    ion_Expr (*e) = ion_new_expr(ION_EXPR_SIZEOF_EXPR, pos);
    e->sizeof_expr = expr;
    return e;
}

ion_Expr (*ion_new_expr_sizeof_type(ion_SrcPos pos, ion_Typespec (*type))) {
    ion_Expr (*e) = ion_new_expr(ION_EXPR_SIZEOF_TYPE, pos);
    e->sizeof_type = type;
    return e;
}

ion_Expr (*ion_new_expr_typeof_expr(ion_SrcPos pos, ion_Expr (*expr))) {
    ion_Expr (*e) = ion_new_expr(ION_EXPR_TYPEOF_EXPR, pos);
    e->typeof_expr = expr;
    return e;
}

ion_Expr (*ion_new_expr_typeof_type(ion_SrcPos pos, ion_Typespec (*type))) {
    ion_Expr (*e) = ion_new_expr(ION_EXPR_TYPEOF_TYPE, pos);
    e->typeof_type = type;
    return e;
}

ion_Expr (*ion_new_expr_alignof_expr(ion_SrcPos pos, ion_Expr (*expr))) {
    ion_Expr (*e) = ion_new_expr(ION_EXPR_ALIGNOF_EXPR, pos);
    e->alignof_expr = expr;
    return e;
}

ion_Expr (*ion_new_expr_alignof_type(ion_SrcPos pos, ion_Typespec (*type))) {
    ion_Expr (*e) = ion_new_expr(ION_EXPR_ALIGNOF_TYPE, pos);
    e->alignof_type = type;
    return e;
}

ion_Expr (*ion_new_expr_offsetof(ion_SrcPos pos, ion_Typespec (*type), char const ((*name)))) {
    ion_Expr (*e) = ion_new_expr(ION_EXPR_OFFSETOF, pos);
    e->offsetof_field.type = type;
    e->offsetof_field.name = name;
    return e;
}

ion_Expr (*ion_new_expr_modify(ion_SrcPos pos, ion_TokenKind op, bool post, ion_Expr (*expr))) {
    ion_Expr (*e) = ion_new_expr(ION_EXPR_MODIFY, pos);
    e->modify.op = op;
    e->modify.post = post;
    e->modify.expr = expr;
    return e;
}

ion_Expr (*ion_new_expr_int(ion_SrcPos pos, ullong val, ion_TokenMod mod, ion_TokenSuffix suffix)) {
    ion_Expr (*e) = ion_new_expr(ION_EXPR_INT, pos);
    e->int_lit.val = val;
    e->int_lit.mod = mod;
    e->int_lit.suffix = suffix;
    return e;
}

ion_Expr (*ion_new_expr_float(ion_SrcPos pos, char const ((*start)), char const ((*end)), double val, ion_TokenSuffix suffix)) {
    ion_Expr (*e) = ion_new_expr(ION_EXPR_FLOAT, pos);
    e->float_lit.start = start;
    e->float_lit.end = end;
    e->float_lit.val = val;
    e->float_lit.suffix = suffix;
    return e;
}

ion_Expr (*ion_new_expr_str(ion_SrcPos pos, char const ((*val)), ion_TokenMod mod)) {
    ion_Expr (*e) = ion_new_expr(ION_EXPR_STR, pos);
    e->str_lit.val = val;
    e->str_lit.mod = mod;
    return e;
}

ion_Expr (*ion_new_expr_name(ion_SrcPos pos, char const ((*name)))) {
    ion_Expr (*e) = ion_new_expr(ION_EXPR_NAME, pos);
    e->name = name;
    return e;
}

ion_Expr (*ion_new_expr_compound(ion_SrcPos pos, ion_Typespec (*type), ion_CompoundField (*fields), size_t num_fields)) {
    ion_Expr (*e) = ion_new_expr(ION_EXPR_COMPOUND, pos);
    e->compound.type = type;
    e->compound.fields = ion_ast_dup(fields, (num_fields) * (sizeof(*(fields))));
    e->compound.num_fields = num_fields;
    return e;
}

ion_Expr (*ion_new_expr_cast(ion_SrcPos pos, ion_Typespec (*type), ion_Expr (*expr))) {
    ion_Expr (*e) = ion_new_expr(ION_EXPR_CAST, pos);
    e->cast.type = type;
    e->cast.expr = expr;
    return e;
}

ion_Expr (*ion_new_expr_call(ion_SrcPos pos, ion_Expr (*expr), ion_Expr (*(*args)), size_t num_args)) {
    ion_Expr (*e) = ion_new_expr(ION_EXPR_CALL, pos);
    e->call.expr = expr;
    e->call.args = ion_ast_dup(args, (num_args) * (sizeof(*(args))));
    e->call.num_args = num_args;
    return e;
}

ion_Expr (*ion_new_expr_index(ion_SrcPos pos, ion_Expr (*expr), ion_Expr (*index))) {
    ion_Expr (*e) = ion_new_expr(ION_EXPR_INDEX, pos);
    e->index.expr = expr;
    e->index.index = index;
    return e;
}

ion_Expr (*ion_new_expr_field(ion_SrcPos pos, ion_Expr (*expr), char const ((*name)))) {
    ion_Expr (*e) = ion_new_expr(ION_EXPR_FIELD, pos);
    e->field.expr = expr;
    e->field.name = name;
    return e;
}

ion_Expr (*ion_new_expr_unary(ion_SrcPos pos, ion_TokenKind op, ion_Expr (*expr))) {
    ion_Expr (*e) = ion_new_expr(ION_EXPR_UNARY, pos);
    e->unary.op = op;
    e->unary.expr = expr;
    return e;
}

ion_Expr (*ion_new_expr_binary(ion_SrcPos pos, ion_TokenKind op, ion_Expr (*left), ion_Expr (*right))) {
    ion_Expr (*e) = ion_new_expr(ION_EXPR_BINARY, pos);
    e->binary.op = op;
    e->binary.left = left;
    e->binary.right = right;
    return e;
}

ion_Expr (*ion_new_expr_ternary(ion_SrcPos pos, ion_Expr (*cond), ion_Expr (*then_expr), ion_Expr (*else_expr))) {
    ion_Expr (*e) = ion_new_expr(ION_EXPR_TERNARY, pos);
    e->ternary.cond = cond;
    e->ternary.then_expr = then_expr;
    e->ternary.else_expr = else_expr;
    return e;
}

ion_Note (*ion_get_stmt_note(ion_Stmt (*stmt), char const ((*name)))) {
    for (size_t i = 0; (i) < (stmt->notes.num_notes); (i)++) {
        ion_Note (*note) = (stmt->notes.notes) + (i);
        if ((note->name) == (name)) {
            return note;
        }
    }
    return NULL;
}

ion_Stmt (*ion_new_stmt(ion_StmtKind kind, ion_SrcPos pos)) {
    ion_Stmt (*s) = ion_ast_alloc(sizeof(ion_Stmt));
    s->kind = kind;
    s->pos = pos;
    return s;
}

ion_Stmt (*ion_new_stmt_label(ion_SrcPos pos, char const ((*label)))) {
    ion_Stmt (*s) = ion_new_stmt(ION_STMT_LABEL, pos);
    s->label = label;
    return s;
}

ion_Stmt (*ion_new_stmt_goto(ion_SrcPos pos, char const ((*label)))) {
    ion_Stmt (*s) = ion_new_stmt(ION_STMT_GOTO, pos);
    s->label = label;
    return s;
}

ion_Stmt (*ion_new_stmt_note(ion_SrcPos pos, ion_Note note)) {
    ion_Stmt (*s) = ion_new_stmt(ION_STMT_NOTE, pos);
    s->note = note;
    return s;
}

ion_Stmt (*ion_new_stmt_decl(ion_SrcPos pos, ion_Decl (*decl))) {
    ion_Stmt (*s) = ion_new_stmt(ION_STMT_DECL, pos);
    s->decl = decl;
    return s;
}

ion_Stmt (*ion_new_stmt_return(ion_SrcPos pos, ion_Expr (*expr))) {
    ion_Stmt (*s) = ion_new_stmt(ION_STMT_RETURN, pos);
    s->expr = expr;
    return s;
}

ion_Stmt (*ion_new_stmt_break(ion_SrcPos pos)) {
    return ion_new_stmt(ION_STMT_BREAK, pos);
}

ion_Stmt (*ion_new_stmt_continue(ion_SrcPos pos)) {
    return ion_new_stmt(ION_STMT_CONTINUE, pos);
}

ion_Stmt (*ion_new_stmt_block(ion_SrcPos pos, ion_StmtList block)) {
    ion_Stmt (*s) = ion_new_stmt(ION_STMT_BLOCK, pos);
    s->block = block;
    return s;
}

ion_Stmt (*ion_new_stmt_if(ion_SrcPos pos, ion_Stmt (*init), ion_Expr (*cond), ion_StmtList then_block, ion_ElseIf (*elseifs), size_t num_elseifs, ion_StmtList else_block)) {
    ion_Stmt (*s) = ion_new_stmt(ION_STMT_IF, pos);
    s->if_stmt.init = init;
    s->if_stmt.cond = cond;
    s->if_stmt.then_block = then_block;
    s->if_stmt.elseifs = ion_ast_dup(elseifs, (num_elseifs) * (sizeof(*(elseifs))));
    s->if_stmt.num_elseifs = num_elseifs;
    s->if_stmt.else_block = else_block;
    return s;
}

ion_Stmt (*ion_new_stmt_while(ion_SrcPos pos, ion_Expr (*cond), ion_StmtList block)) {
    ion_Stmt (*s) = ion_new_stmt(ION_STMT_WHILE, pos);
    s->while_stmt.cond = cond;
    s->while_stmt.block = block;
    return s;
}

ion_Stmt (*ion_new_stmt_do_while(ion_SrcPos pos, ion_Expr (*cond), ion_StmtList block)) {
    ion_Stmt (*s) = ion_new_stmt(ION_STMT_DO_WHILE, pos);
    s->while_stmt.cond = cond;
    s->while_stmt.block = block;
    return s;
}

ion_Stmt (*ion_new_stmt_for(ion_SrcPos pos, ion_Stmt (*init), ion_Expr (*cond), ion_Stmt (*next), ion_StmtList block)) {
    ion_Stmt (*s) = ion_new_stmt(ION_STMT_FOR, pos);
    s->for_stmt.init = init;
    s->for_stmt.cond = cond;
    s->for_stmt.next = next;
    s->for_stmt.block = block;
    return s;
}

ion_Stmt (*ion_new_stmt_switch(ion_SrcPos pos, ion_Expr (*expr), ion_SwitchCase (*cases), size_t num_cases)) {
    ion_Stmt (*s) = ion_new_stmt(ION_STMT_SWITCH, pos);
    s->switch_stmt.expr = expr;
    s->switch_stmt.cases = ion_ast_dup(cases, (num_cases) * (sizeof(*(cases))));
    s->switch_stmt.num_cases = num_cases;
    return s;
}

ion_Stmt (*ion_new_stmt_assign(ion_SrcPos pos, ion_TokenKind op, ion_Expr (*left), ion_Expr (*right))) {
    ion_Stmt (*s) = ion_new_stmt(ION_STMT_ASSIGN, pos);
    s->assign.op = op;
    s->assign.left = left;
    s->assign.right = right;
    return s;
}

ion_Stmt (*ion_new_stmt_init(ion_SrcPos pos, char const ((*name)), ion_Typespec (*type), ion_Expr (*expr))) {
    ion_Stmt (*s) = ion_new_stmt(ION_STMT_INIT, pos);
    s->init.name = name;
    s->init.type = type;
    s->init.expr = expr;
    return s;
}

ion_Stmt (*ion_new_stmt_expr(ion_SrcPos pos, ion_Expr (*expr))) {
    ion_Stmt (*s) = ion_new_stmt(ION_STMT_EXPR, pos);
    s->expr = expr;
    return s;
}

size_t ion_min(size_t x, size_t y) {
    return ((x) <= (y) ? x : y);
}

size_t ion_max(size_t x, size_t y) {
    return ((x) >= (y) ? x : y);
}

size_t ion_clamp_max(size_t x, size_t max) {
    return ion_min(x, max);
}

size_t ion_clamp_min(size_t x, size_t min) {
    return ion_max(x, min);
}

size_t ion_is_pow2(size_t x) {
    return ((x) != (0)) && (((x) & (((x) - (1)))) == (0));
}

uintptr_t ion_align_down(uintptr_t n, size_t a) {
    return (n) & (~(((a) - (1))));
}

uintptr_t ion_align_up(uintptr_t n, size_t a) {
    return ion_align_down(((n) + (a)) - (1), a);
}

void (*ion_align_down_ptr(void (*p), size_t a)) {
    return (void *)(ion_align_down((uintptr_t)(p), a));
}

void (*ion_align_up_ptr(void (*p), size_t a)) {
    return (void *)(ion_align_up((uintptr_t)(p), a));
}

void ion_fatal(char const ((*fmt)), ...) {
    va_list args = {0};
    va_start_ptr(&(args), &(fmt));
    printf("FATAL: ");
    vprintf(fmt, args);
    printf("\n");
    va_end_ptr(&(args));
    exit(1);
}

void (*ion_xcalloc(size_t num_elems, size_t elem_size)) {
    void (*ptr) = calloc(num_elems, elem_size);
    if (!(ptr)) {
        perror("xcalloc failed");
        exit(1);
    }
    return ptr;
}

void (*ion_xrealloc(void (*ptr), size_t num_bytes)) {
    ptr = realloc(ptr, num_bytes);
    if (!(ptr)) {
        perror("xrealloc failed");
        exit(1);
    }
    return ptr;
}

void (*ion_xmalloc(size_t num_bytes)) {
    void (*ptr) = malloc(num_bytes);
    if (!(ptr)) {
        perror("xmalloc failed");
        exit(1);
    }
    return ptr;
}

void (*ion_memdup(void (*src), size_t size)) {
    void (*dest) = ion_xmalloc(size);
    memcpy(dest, src, size);
    return dest;
}

char (*ion_strf(char const ((*fmt)), ...)) {
    va_list args = {0};
    va_start_ptr(&(args), &(fmt));
    int n = (1) + (vsnprintf(NULL, 0, fmt, args));
    va_end_ptr(&(args));
    char (*str) = ion_xmalloc(n);
    va_start_ptr(&(args), &(fmt));
    vsnprintf(str, n, fmt, args);
    va_end_ptr(&(args));
    return str;
}

char (*ion_read_file(char const ((*path)))) {
    FILE (*file) = fopen(path, "rb");
    if (!(file)) {
        return NULL;
    }
    fseek(file, 0, SEEK_END);
    long len = ftell(file);
    fseek(file, 0, SEEK_SET);
    char (*buf) = ion_xmalloc((len) + (1));
    if ((len) && ((fread(buf, len, 1, file)) != (1))) {
        fclose(file);
        free(buf);
        return NULL;
    }
    fclose(file);
    buf[len] = 0;
    return buf;
}

bool ion_write_file(char const ((*path)), char const ((*buf)), size_t len) {
    FILE (*file) = fopen(path, "w");
    if (!(file)) {
        return false;
    }
    ullong n = fwrite(buf, len, 1, file);
    fclose(file);
    return (n) == (1);
}

ion_BufHdr (*ion_buf__hdr(void (*b))) {
    return (ion_BufHdr *)((((char *)(b)) - (offsetof(ion_BufHdr, buf))));
}

size_t ion_buf_len(void (*b)) {
    return (b ? ion_buf__hdr(b)->len : 0);
}

size_t ion_buf_cap(void const ((*b))) {
    return (b ? ion_buf__hdr((void *)(b))->cap : 0);
}

void (*ion_buf_end(void (*b), size_t elem_size)) {
    return ((char *)(b)) + ((elem_size) * (ion_buf_len(b)));
}

size_t ion_buf_sizeof(void (*b), size_t elem_size) {
    return (b ? (ion_buf_len(b)) * (elem_size) : 0);
}

void ion_buf_free(void (*(*b))) {
    if (b) {
        free(ion_buf__hdr(*(b)));
        *(b) = NULL;
    }
}

void ion_buf_fit(void (*(*b)), size_t new_len, size_t elem_size) {
    if ((new_len) > (ion_buf_cap(*(b)))) {
        *(b) = ion_buf__grow(*(b), new_len, elem_size);
    }
}

void ion_buf_push(void (*(*b)), void (*elem), size_t elem_size) {
    ion_buf_fit(b, (1) + (ion_buf_len(*(b))), elem_size);
    memcpy(((char *)(*(b))) + ((elem_size) * ((ion_buf__hdr(*(b))->len)++)), elem, elem_size);
}

void (*ion_buf__grow(void const ((*buf)), size_t new_len, size_t elem_size)) {
    assert((ion_buf_cap(buf)) <= ((((SIZE_MAX) - (1))) / (2)));
    ullong new_cap = ion_clamp_min((2) * (ion_buf_cap(buf)), ion_max(new_len, 16));
    assert((new_len) <= (new_cap));
    assert((new_cap) <= ((((SIZE_MAX) - (offsetof(ion_BufHdr, buf)))) / (elem_size)));
    ullong new_size = (offsetof(ion_BufHdr, buf)) + ((new_cap) * (elem_size));
    ion_BufHdr (*new_hdr) = {0};
    if (buf) {
        new_hdr = ion_xrealloc(ion_buf__hdr((void *)(buf)), new_size);
    } else {
        new_hdr = ion_xmalloc(new_size);
        new_hdr->len = 0;
    }
    new_hdr->cap = new_cap;
    return new_hdr->buf;
}

void ion_buf_printf(char (*(*buf)), char const ((*fmt)), ...) {
    va_list args = {0};
    va_start_ptr(&(args), &(fmt));
    ullong cap = (ion_buf_cap(*(buf))) - (ion_buf_len(*(buf)));
    int n = (1) + (vsnprintf(ion_buf_end(*(buf), 1), cap, fmt, args));
    va_end_ptr(&(args));
    if ((n) > (cap)) {
        ion_buf_fit((void (**))(buf), (n) + (ion_buf_len(*(buf))), 1);
        va_start_ptr(&(args), &(fmt));
        ullong new_cap = (ion_buf_cap(*(buf))) - (ion_buf_len(*(buf)));
        n = (1) + (vsnprintf(ion_buf_end(*(buf), 1), new_cap, fmt, args));
        assert((n) <= (new_cap));
        va_end_ptr(&(args));
    }
    ion_buf__hdr(*(buf))->len += (n) - (1);
}

ion_Arena ion_allocator;
void ion_arena_grow(ion_Arena (*arena), size_t min_size) {
    ullong size = ion_align_up(ion_clamp_min(min_size, ION_ARENA_BLOCK_SIZE), ION_ARENA_ALIGNMENT);
    arena->ptr = ion_xmalloc(size);
    assert((arena->ptr) == (ion_align_down_ptr(arena->ptr, ION_ARENA_ALIGNMENT)));
    arena->end = (arena->ptr) + (size);
    ion_buf_push((void (**))(&(arena->blocks)), &(arena->ptr), sizeof(arena->ptr));
}

void (*ion_arena_alloc(ion_Arena (*arena), size_t size)) {
    if ((size) > ((size_t)(((arena->end) - (arena->ptr))))) {
        ion_arena_grow(arena, size);
        assert((size) <= ((size_t)(((arena->end) - (arena->ptr)))));
    }
    char (*ptr) = arena->ptr;
    arena->ptr = ion_align_up_ptr((arena->ptr) + (size), ION_ARENA_ALIGNMENT);
    assert((arena->ptr) <= (arena->end));
    assert((ptr) == (ion_align_down_ptr(ptr, ION_ARENA_ALIGNMENT)));
    return ptr;
}

void ion_arena_free(ion_Arena (*arena)) {
    void (*end) = ion_buf_end(arena->blocks, sizeof(arena->ptr));
    for (char (*(*it)) = arena->blocks; (it) != (end); (it)++) {
        free(*(it));
    }
    ion_buf_free((void (**))(&(arena->blocks)));
}

uint64_t ion_hash_uint64(uint64_t x) {
    x *= 0xff51afd7ed558ccd;
    x ^= (x) >> (32);
    return x;
}

uint64_t ion_hash_ptr(void const ((*ptr))) {
    return ion_hash_uint64((uintptr_t)(ptr));
}

uint64_t ion_hash_mix(uint64_t x, uint64_t y) {
    x ^= y;
    x *= 0xff51afd7ed558ccd;
    x ^= (x) >> (32);
    return x;
}

uint64_t ion_hash_bytes(void const ((*ptr)), size_t len) {
    uint64_t x = 0xcbf29ce484222325;
    char const ((*buf)) = (char const (*))(ptr);
    for (size_t i = 0; (i) < (len); (i)++) {
        x ^= buf[i];
        x *= 0x100000001b3;
        x ^= (x) >> (32);
    }
    return x;
}

uint64_t ion_map_get_uint64_from_uint64(ion_Map (*map), uint64_t key) {
    if ((map->len) == (0)) {
        return 0;
    }
    assert(ion_is_pow2(map->cap));
    ullong i = (size_t)(ion_hash_uint64(key));
    assert((map->len) < (map->cap));
    for (;;) {
        i &= (map->cap) - (1);
        if ((map->keys[i]) == (key)) {
            return map->vals[i];
        } else if (!(map->keys[i])) {
            return 0;
        }
        (i)++;
    }
    return 0;
}

void ion_map_grow(ion_Map (*map), size_t new_cap) {
    new_cap = ion_clamp_min(new_cap, 16);
    ion_Map new_map = {.keys = ion_xcalloc(new_cap, sizeof(uint64_t)), .vals = ion_xmalloc((new_cap) * (sizeof(uint64_t))), .cap = new_cap};
    for (size_t i = 0; (i) < (map->cap); (i)++) {
        if (map->keys[i]) {
            ion_map_put_uint64_from_uint64(&(new_map), map->keys[i], map->vals[i]);
        }
    }
    free((void *)(map->keys));
    free(map->vals);
    *(map) = new_map;
}

void ion_map_put_uint64_from_uint64(ion_Map (*map), uint64_t key, uint64_t val) {
    assert(key);
    if (!(val)) {
        return;
    }
    if (((2) * (map->len)) >= (map->cap)) {
        ion_map_grow(map, (2) * (map->cap));
    }
    assert(((2) * (map->len)) < (map->cap));
    assert(ion_is_pow2(map->cap));
    ullong i = (size_t)(ion_hash_uint64(key));
    for (;;) {
        i &= (map->cap) - (1);
        if (!(map->keys[i])) {
            (map->len)++;
            map->keys[i] = key;
            map->vals[i] = val;
            return;
        } else if ((map->keys[i]) == (key)) {
            map->vals[i] = val;
            return;
        }
        (i)++;
    }
}

void (*ion_map_get(ion_Map (*map), void const ((*key)))) {
    return (void *)((uintptr_t)(ion_map_get_uint64_from_uint64(map, (uint64_t)((uintptr_t)(key)))));
}

void ion_map_put(ion_Map (*map), void const ((*key)), void (*val)) {
    ion_map_put_uint64_from_uint64(map, (uint64_t)((uintptr_t)(key)), (uint64_t)((uintptr_t)(val)));
}

void (*ion_map_get_from_uint64(ion_Map (*map), uint64_t key)) {
    return (void *)((uintptr_t)(ion_map_get_uint64_from_uint64(map, key)));
}

void ion_map_put_from_uint64(ion_Map (*map), uint64_t key, void (*val)) {
    ion_map_put_uint64_from_uint64(map, key, (uint64_t)((uintptr_t)(val)));
}

uint64_t ion_map_get_uint64(ion_Map (*map), void (*key)) {
    return ion_map_get_uint64_from_uint64(map, (uint64_t)((uintptr_t)(key)));
}

void ion_map_put_uint64(ion_Map (*map), void (*key), uint64_t val) {
    ion_map_put_uint64_from_uint64(map, (uint64_t)((uintptr_t)(key)), val);
}

ion_Arena ion_intern_arena;
ion_Map ion_interns;
char const ((*ion_str_intern_range(char const ((*start)), char const ((*end))))) {
    size_t len = (end) - (start);
    ullong hash = ion_hash_bytes(start, len);
    uint64_t key = (hash ? hash : 1);
    ion_Intern (*intern) = ion_map_get_from_uint64(&(ion_interns), key);
    for (ion_Intern (*it) = intern; it; it = it->next) {
        if (((it->len) == (len)) && ((strncmp(it->str, start, len)) == (0))) {
            return it->str;
        }
    }
    ion_Intern (*new_intern) = ion_arena_alloc(&(ion_intern_arena), ((offsetof(ion_Intern, str)) + (len)) + (1));
    new_intern->len = len;
    new_intern->next = intern;
    memcpy(new_intern->str, start, len);
    new_intern->str[len] = 0;
    ion_map_put_from_uint64(&(ion_interns), key, new_intern);
    return new_intern->str;
}

char const ((*ion_str_intern(char const ((*str))))) {
    return ion_str_intern_range(str, (str) + (strlen(str)));
}

bool ion_str_islower(char const ((*str))) {
    while (*(str)) {
        if ((isalpha(*(str))) && (!(islower(*(str))))) {
            return false;
        }
        (str)++;
    }
    return true;
}

char (*ion_gen_buf) = NULL;
int ion_gen_indent;
ion_SrcPos ion_gen_pos;
char const ((*(*ion_gen_headers_buf)));
char (*ion_gen_preamble_str) = 
    "// Preamble\n"
    "#define __USE_MINGW_ANSI_STDIO 1\n"
    "#ifndef _CRT_SECURE_NO_WARNINGS\n"
    "#define _CRT_SECURE_NO_WARNINGS\n"
    "#endif\n"
    "#ifndef _CRT_NONSTDC_NO_DEPRECATE\n"
    "#define _CRT_NONSTDC_NO_DEPRECATE\n"
    "#endif\n"
    "\n"
    "#if _MSC_VER >= 1900 || __STDC_VERSION__ >= 201112L\n"
    "// Visual Studio 2015 supports enough C99/C11 features for us.\n"
    "#else\n"
    "#error \"C11 support required or Visual Studio 2015 or later\"\n"
    "#endif\n"
    "\n"
    "#ifdef __GNUC__\n"
    "#pragma GCC diagnostic push\n"
    "#pragma GCC diagnostic ignored \"-Wvarargs\"\n"
    "#endif\n"
    "\n"
    "#include <stdbool.h>\n"
    "#include <stdint.h>\n"
    "#include <stddef.h>\n"
    "#include <stdarg.h>\n"
    "#include <assert.h>\n"
    "\n"
    "typedef unsigned char uchar;\n"
    "typedef signed char schar;\n"
    "typedef unsigned short ushort;\n"
    "typedef unsigned int uint;\n"
    "typedef unsigned long ulong;\n"
    "typedef long long llong;\n"
    "typedef unsigned long long ullong;\n"
    "\n"
    "#ifdef _MSC_VER\n"
    "#define alignof(x) __alignof(x)\n"
    "#else\n"
    "#define alignof(x) __alignof__(x)\n"
    "#endif\n"
    "\n"
    "#define va_start_ptr(args, arg) (va_start(*(args), *(arg)))\n"
    "#define va_copy_ptr(dest, src) (va_copy(*(dest), *(src)))\n"
    "#define va_end_ptr(args) (va_end(*(args)))\n"
    "\n"
    "struct Any;\n"
    "static void va_arg_ptr(va_list *args, struct Any any);\n";
char (*ion_gen_postamble_str) = 
    "\n"
    "static void va_arg_ptr(va_list *args, Any any) {\n"
    "    switch (typeid_kind(any.type)) {\n"
    "    case TYPE_BOOL:\n"
    "        *(bool *)any.ptr = (bool)va_arg(*args, int);\n"
    "        break;\n"
    "    case TYPE_CHAR:\n"
    "        *(char *)any.ptr = (char)va_arg(*args, int);\n"
    "        break;\n"
    "    case TYPE_UCHAR:\n"
    "        *(uchar *)any.ptr = (uchar)va_arg(*args, int);\n"
    "        break;\n"
    "    case TYPE_SCHAR:\n"
    "        *(schar *)any.ptr = (schar)va_arg(*args, int);\n"
    "        break;\n"
    "    case TYPE_SHORT:\n"
    "        *(short *)any.ptr = (short)va_arg(*args, int);\n"
    "        break;\n"
    "    case TYPE_USHORT:\n"
    "        *(ushort *)any.ptr = (ushort)va_arg(*args, int);\n"
    "        break;\n"
    "    case TYPE_INT:\n"
    "        *(int *)any.ptr = va_arg(*args, int);\n"
    "        break;\n"
    "    case TYPE_UINT:\n"
    "        *(uint *)any.ptr = va_arg(*args, uint);\n"
    "        break;\n"
    "    case TYPE_LONG:\n"
    "        *(long *)any.ptr = va_arg(*args, long);\n"
    "        break;\n"
    "    case TYPE_ULONG:\n"
    "        *(ulong *)any.ptr = va_arg(*args, ulong);\n"
    "        break;\n"
    "    case TYPE_LLONG:\n"
    "        *(llong *)any.ptr = va_arg(*args, llong);\n"
    "        break;\n"
    "    case TYPE_ULLONG:\n"
    "        *(ullong *)any.ptr = va_arg(*args, ullong);\n"
    "        break;\n"
    "    case TYPE_FLOAT:\n"
    "        *(float *)any.ptr = (float)va_arg(*args, double);\n"
    "        break;\n"
    "    case TYPE_DOUBLE:\n"
    "        *(double *)any.ptr = va_arg(*args, double);\n"
    "        break;\n"
    "    case TYPE_FUNC:\n"
    "    case TYPE_PTR:\n"
    "        *(void **)any.ptr = va_arg(*args, void *);\n"
    "        break;\n"
    "    default:\n"
    "        assert(0 && \"argument type not supported\");\n"
    "        break;\n"
    "    }\n"
    "}\n"
    "\n"
    "#ifdef __GNUC__\n"
    "#pragma GCC diagnostic pop\n"
    "#endif\n";
void ion_genln(void) {
    ion_buf_printf(&(ion_gen_buf), "\n"
    "%.*s", (ion_gen_indent) * (4), "                                                                  ");
    (ion_gen_pos.line)++;
}

bool ion_is_incomplete_array_typespec(ion_Typespec (*typespec)) {
    return ((typespec->kind) == (ION_TYPESPEC_ARRAY)) && (!(typespec->num_elems));
}

char (ion_char_to_escape[256]) = {['\0'] = '0', ['\n'] = 'n', ['\r'] = 'r', ['\t'] = 't', ['\v'] = 'v', ['\b'] = 'b', ['\a'] = 'a', ['\\'] = '\\', ['\"'] = '\"', ['\''] = '\''};
void ion_gen_char(char c) {
    if (ion_char_to_escape[(uchar)(c)]) {
        ion_buf_printf(&(ion_gen_buf), "\'\\%c\'", ion_char_to_escape[(uchar)(c)]);
    } else if (isprint(c)) {
        ion_buf_printf(&(ion_gen_buf), "\'%c\'", c);
    } else {
        ion_buf_printf(&(ion_gen_buf), "\'\\x%X\'", (uchar)(c));
    }
}

void ion_gen_str(char const ((*str)), bool multiline) {
    if (multiline) {
        (ion_gen_indent)++;
        ion_genln();
    }
    ion_buf_printf(&(ion_gen_buf), "\"");
    while (*(str)) {
        char const ((*start)) = str;
        while (((*(str)) && (isprint(*(str)))) && (!(ion_char_to_escape[(uchar)(*(str))]))) {
            (str)++;
        }
        if ((start) != (str)) {
            ion_buf_printf(&(ion_gen_buf), "%.*s", (str) - (start), start);
        }
        if (*(str)) {
            if (ion_char_to_escape[(uchar)(*(str))]) {
                ion_buf_printf(&(ion_gen_buf), "\\%c", ion_char_to_escape[(uchar)(*(str))]);
                if (((str[0]) == ('\n')) && (str[1])) {
                    ion_buf_printf(&(ion_gen_buf), "\"");
                    ion_genln();
                    ion_buf_printf(&(ion_gen_buf), "\"");
                }
            } else {
                assert(!(isprint(*(str))));
                ion_buf_printf(&(ion_gen_buf), "\\x%X", (uchar)(*(str)));
            }
            (str)++;
        }
    }
    ion_buf_printf(&(ion_gen_buf), "\"");
    if (multiline) {
        (ion_gen_indent)--;
    }
}

void ion_gen_sync_pos(ion_SrcPos pos) {
    if (ion_flag_nosourcemap) {
        return;
    }
    if (((ion_gen_pos.line) != (pos.line)) || ((ion_gen_pos.name) != (pos.name))) {
        ion_genln();
        ion_buf_printf(&(ion_gen_buf), "#line %d", pos.line);
        if ((ion_gen_pos.name) != (pos.name)) {
            ion_buf_printf(&(ion_gen_buf), " ");
            ion_gen_str(pos.name, false);
        }
        ion_gen_pos = pos;
    }
}

char const ((*ion_cdecl_paren(char const ((*str)), char c))) {
    return ((c) && ((c) != ('[')) ? (char const (*))(ion_strf("(%s)", str)) : str);
}

char const ((*ion_cdecl_name(ion_Type (*type)))) {
    char const ((*type_name)) = ion_type_names[type->kind];
    if (type_name) {
        return type_name;
    } else {
        assert(type->sym);
        return ion_get_gen_name(type->sym);
    }
}

char (*ion_type_to_cdecl(ion_Type (*type), char const ((*str)))) {
    switch (type->kind) {
    case ION_CMPL_TYPE_PTR: {
        return ion_type_to_cdecl(type->base, ion_cdecl_paren(ion_strf("*%s", str), *(str)));
        break;
    }
    case ION_CMPL_TYPE_CONST: {
        return ion_type_to_cdecl(type->base, ion_strf("const %s", ion_cdecl_paren(str, *(str))));
        break;
    }
    case ION_CMPL_TYPE_ARRAY: {
        if ((type->num_elems) == (0)) {
            return ion_type_to_cdecl(type->base, ion_cdecl_paren(ion_strf("%s[]", str), *(str)));
        } else {
            return ion_type_to_cdecl(type->base, ion_cdecl_paren(ion_strf("%s[%zu]", str, type->num_elems), *(str)));
        }
        break;
    }
    case ION_CMPL_TYPE_FUNC: {
        {
            char (*result) = NULL;
            ion_buf_printf(&(result), "(*%s)(", str);
            if ((type->function.num_params) == (0)) {
                ion_buf_printf(&(result), "void");
            } else {
                for (size_t i = 0; (i) < (type->function.num_params); (i)++) {
                    ion_buf_printf(&(result), "%s%s", ((i) == (0) ? "" : ", "), ion_type_to_cdecl(type->function.params[i], ""));
                }
            }
            if (type->function.has_varargs) {
                ion_buf_printf(&(result), ", ...");
            }
            ion_buf_printf(&(result), ")");
            return ion_type_to_cdecl(type->function.ret, result);
        }
        break;
    }
    default: {
        return ion_strf("%s%s%s", ion_cdecl_name(type), (*(str) ? " " : ""), str);
        break;
    }
    }
}

char const ((*ion_gen_expr_str(ion_Expr (*expr)))) {
    char (*temp) = ion_gen_buf;
    ion_gen_buf = NULL;
    ion_gen_expr(expr);
    char (*result) = ion_gen_buf;
    ion_gen_buf = temp;
    return result;
}

ion_Map ion_gen_name_map;
char const ((*ion_get_gen_name_or_default(void const ((*ptr)), char const ((*default_name))))) {
    char const ((*name)) = ion_map_get(&(ion_gen_name_map), ptr);
    if (!(name)) {
        ion_Sym (*sym) = ion_get_resolved_sym(ptr);
        if (sym) {
            if (sym->external_name) {
                name = sym->external_name;
            } else if (sym->home_package->external_name) {
                char const ((*external_name)) = sym->home_package->external_name;
                char (buf[256]) = {0};
                if ((sym->kind) == (ION_SYM_CONST)) {
                    char (*ptr2) = buf;
                    for (char const ((*str)) = external_name; (*(str)) && ((ptr2) < (((buf) + (sizeof(buf))) - (1))); (str)++) {
                        *(ptr2) = toupper(*(str));
                        (ptr2)++;
                    }
                    *(ptr2) = 0;
                    if ((ptr2) < ((buf) + (sizeof(buf)))) {
                        external_name = buf;
                    }
                }
                name = ion_strf("%s%s", external_name, sym->name);
            } else {
                name = sym->name;
            }
        } else {
            assert(default_name);
            name = default_name;
        }
        ion_map_put(&(ion_gen_name_map), ptr, (void *)(name));
    }
    return name;
}

char const ((*ion_get_gen_name(void const ((*ptr))))) {
    return ion_get_gen_name_or_default(ptr, "ERROR");
}

char (*ion_typespec_to_cdecl(ion_Typespec (*typespec), char const ((*str)))) {
    if (!(typespec)) {
        return ion_strf("void%s%s", (*(str) ? " " : ""), str);
    }
    switch (typespec->kind) {
    case ION_TYPESPEC_NAME: {
        return ion_strf("%s%s%s", ion_get_gen_name_or_default(typespec, typespec->name), (*(str) ? " " : ""), str);
        break;
    }
    case ION_TYPESPEC_PTR: {
        return ion_typespec_to_cdecl(typespec->base, ion_cdecl_paren(ion_strf("*%s", str), *(str)));
        break;
    }
    case ION_TYPESPEC_CONST: {
        return ion_typespec_to_cdecl(typespec->base, ion_strf("const %s", ion_cdecl_paren(str, *(str))));
        break;
    }
    case ION_TYPESPEC_ARRAY: {
        if ((typespec->num_elems) == (0)) {
            return ion_typespec_to_cdecl(typespec->base, ion_cdecl_paren(ion_strf("%s[]", str), *(str)));
        } else {
            return ion_typespec_to_cdecl(typespec->base, ion_cdecl_paren(ion_strf("%s[%s]", str, ion_gen_expr_str(typespec->num_elems)), *(str)));
        }
        break;
    }
    case ION_TYPESPEC_FUNC: {
        {
            char (*result) = NULL;
            ion_buf_printf(&(result), "(*%s)(", str);
            if ((typespec->function.num_args) == (0)) {
                ion_buf_printf(&(result), "void");
            } else {
                for (size_t i = 0; (i) < (typespec->function.num_args); (i)++) {
                    ion_buf_printf(&(result), "%s%s", ((i) == (0) ? "" : ", "), ion_typespec_to_cdecl(typespec->function.args[i], ""));
                }
            }
            if (typespec->function.has_varargs) {
                ion_buf_printf(&(result), ", ...");
            }
            ion_buf_printf(&(result), ")");
            return ion_typespec_to_cdecl(typespec->function.ret, result);
        }
        break;
    }
    default: {
        assert(0);
        return NULL;
        break;
    }
    }
}

void ion_gen_func_decl(ion_Decl (*decl)) {
    assert((decl->kind) == (ION_DECL_FUNC));
    char (*result) = NULL;
    ion_buf_printf(&(result), "%s(", ion_get_gen_name(decl));
    if ((decl->function.num_params) == (0)) {
        ion_buf_printf(&(result), "void");
    } else {
        for (size_t i = 0; (i) < (decl->function.num_params); (i)++) {
            ion_FuncParam param = decl->function.params[i];
            if ((i) != (0)) {
                ion_buf_printf(&(result), ", ");
            }
            ion_buf_printf(&(result), "%s", ion_typespec_to_cdecl(param.type, param.name));
        }
    }
    if (decl->function.has_varargs) {
        ion_buf_printf(&(result), ", ...");
    }
    ion_buf_printf(&(result), ")");
    ion_gen_sync_pos(decl->pos);
    if (decl->function.ret_type) {
        ion_genln();
        ion_buf_printf(&(ion_gen_buf), "%s", ion_typespec_to_cdecl(decl->function.ret_type, result));
    } else {
        ion_genln();
        ion_buf_printf(&(ion_gen_buf), "void %s", result);
    }
}

bool ion_gen_reachable(ion_Sym (*sym)) {
    return (ion_flag_fullgen) || ((sym->reachable) == (ION_REACHABLE_NATURAL));
}

void ion_gen_forward_decls(void) {
    for (ion_Sym (*(*it)) = ion_sorted_syms; (it) != (ion_buf_end(ion_sorted_syms, sizeof(*(ion_sorted_syms)))); (it)++) {
        ion_Sym (*sym) = *(it);
        ion_Decl (*decl) = sym->decl;
        if ((!(decl)) || (!(ion_gen_reachable(sym)))) {
            continue;
        }
        if (ion_is_decl_foreign(decl)) {
            continue;
        }
        switch (decl->kind) {
        case ION_DECL_STRUCT:
        case ION_DECL_UNION: {
            {
                char const ((*name)) = ion_get_gen_name(sym);
                ion_genln();
                ion_buf_printf(&(ion_gen_buf), "typedef %s %s %s;", ((decl->kind) == (ION_DECL_STRUCT) ? "struct" : "union"), name, name);
                break;
            }
            break;
        }
        default: {
            break;
            break;
        }
        }
    }
}

void ion_gen_aggregate_items(ion_Aggregate (*aggregate)) {
    (ion_gen_indent)++;
    for (size_t i = 0; (i) < (aggregate->num_items); (i)++) {
        ion_AggregateItem item = aggregate->items[i];
        if ((item.kind) == (ION_AGGREGATE_ITEM_FIELD)) {
            for (size_t j = 0; (j) < (item.num_names); (j)++) {
                ion_gen_sync_pos(item.pos);
                ion_genln();
                ion_buf_printf(&(ion_gen_buf), "%s;", ion_typespec_to_cdecl(item.type, item.names[j]));
            }
        } else if ((item.kind) == (ION_AGGREGATE_ITEM_SUBAGGREGATE)) {
            ion_genln();
            ion_buf_printf(&(ion_gen_buf), "%s {", ((item.subaggregate->kind) == (ION_AGGREGATE_STRUCT) ? "struct" : "union"));
            ion_gen_aggregate_items(item.subaggregate);
            ion_genln();
            ion_buf_printf(&(ion_gen_buf), "};");
        } else {
            assert(0);
        }
    }
    (ion_gen_indent)--;
}

void ion_gen_aggregate(ion_Decl (*decl)) {
    assert(((decl->kind) == (ION_DECL_STRUCT)) || ((decl->kind) == (ION_DECL_UNION)));
    if (decl->is_incomplete) {
        return;
    }
    ion_genln();
    ion_buf_printf(&(ion_gen_buf), "%s %s {", ((decl->kind) == (ION_DECL_STRUCT) ? "struct" : "union"), ion_get_gen_name(decl));
    ion_gen_aggregate_items(decl->aggregate);
    ion_genln();
    ion_buf_printf(&(ion_gen_buf), "};");
}

void ion_gen_paren_expr(ion_Expr (*expr)) {
    ion_buf_printf(&(ion_gen_buf), "(");
    ion_gen_expr(expr);
    ion_buf_printf(&(ion_gen_buf), ")");
}

void ion_gen_expr_compound(ion_Expr (*expr)) {
    ion_Type (*expected_type) = ion_get_resolved_expected_type(expr);
    if ((expected_type) && (!(ion_is_ptr_type(expected_type)))) {
        ion_buf_printf(&(ion_gen_buf), "{");
    } else if (expr->compound.type) {
        ion_buf_printf(&(ion_gen_buf), "(%s){", ion_typespec_to_cdecl(expr->compound.type, ""));
    } else {
        ion_buf_printf(&(ion_gen_buf), "(%s){", ion_type_to_cdecl(ion_get_resolved_type(expr), ""));
    }
    for (size_t i = 0; (i) < (expr->compound.num_fields); (i)++) {
        if ((i) != (0)) {
            ion_buf_printf(&(ion_gen_buf), ", ");
        }
        ion_CompoundField field = expr->compound.fields[i];
        if ((field.kind) == (ION_FIELD_NAME)) {
            ion_buf_printf(&(ion_gen_buf), ".%s = ", field.name);
        } else if ((field.kind) == (ION_FIELD_INDEX)) {
            ion_buf_printf(&(ion_gen_buf), "[");
            ion_gen_expr(field.index);
            ion_buf_printf(&(ion_gen_buf), "] = ");
        }
        ion_gen_expr(field.init);
    }
    if ((expr->compound.num_fields) == (0)) {
        ion_buf_printf(&(ion_gen_buf), "0");
    }
    ion_buf_printf(&(ion_gen_buf), "}");
}

char const ((*(ion_typeid_kind_names[ION_NUM_CMPL_TYPE_KINDS]))) = {[ION_CMPL_TYPE_NONE] = "TYPE_NONE", [ION_CMPL_TYPE_VOID] = "TYPE_VOID", [ION_CMPL_TYPE_BOOL] = "TYPE_BOOL", [ION_CMPL_TYPE_CHAR] = "TYPE_CHAR", [ION_CMPL_TYPE_UCHAR] = "TYPE_UCHAR", [ION_CMPL_TYPE_SCHAR] = "TYPE_SCHAR", [ION_CMPL_TYPE_SHORT] = "TYPE_SHORT", [ION_CMPL_TYPE_USHORT] = "TYPE_USHORT", [ION_CMPL_TYPE_INT] = "TYPE_INT", [ION_CMPL_TYPE_UINT] = "TYPE_UINT", [ION_CMPL_TYPE_LONG] = "TYPE_LONG", [ION_CMPL_TYPE_ULONG] = "TYPE_ULONG", [ION_CMPL_TYPE_LLONG] = "TYPE_LLONG", [ION_CMPL_TYPE_ULLONG] = "TYPE_ULLONG", [ION_CMPL_TYPE_FLOAT] = "TYPE_FLOAT", [ION_CMPL_TYPE_DOUBLE] = "TYPE_DOUBLE", [ION_CMPL_TYPE_CONST] = "TYPE_CONST", [ION_CMPL_TYPE_PTR] = "TYPE_PTR", [ION_CMPL_TYPE_ARRAY] = "TYPE_ARRAY", [ION_CMPL_TYPE_STRUCT] = "TYPE_STRUCT", [ION_CMPL_TYPE_UNION] = "TYPE_UNION", [ION_CMPL_TYPE_FUNC] = "TYPE_FUNC"};
char const ((*ion_typeid_kind_name(ion_Type (*type)))) {
    if ((type->kind) < (ION_NUM_CMPL_TYPE_KINDS)) {
        char const ((*name)) = ion_typeid_kind_names[type->kind];
        if (name) {
            return name;
        }
    }
    return "TYPE_NONE";
}

bool ion_is_excluded_typeinfo(ion_Type (*type)) {
    while ((((type->kind) == (ION_CMPL_TYPE_ARRAY)) || ((type->kind) == (ION_CMPL_TYPE_CONST))) || ((type->kind) == (ION_CMPL_TYPE_PTR))) {
        type = type->base;
    }
    if (type->sym) {
        return !(ion_gen_reachable(type->sym));
    } else {
        return (!(type->sym)) && ((((type->kind) == (ION_CMPL_TYPE_STRUCT)) || ((type->kind) == (ION_CMPL_TYPE_UNION))));
    }
}

void ion_gen_typeid(ion_Type (*type)) {
    if (((type->size) == (0)) || (ion_is_excluded_typeinfo(type))) {
        ion_buf_printf(&(ion_gen_buf), "TYPEID0(%d, %s)", type->typeid, ion_typeid_kind_name(type));
    } else {
        ion_buf_printf(&(ion_gen_buf), "TYPEID(%d, %s, %s)", type->typeid, ion_typeid_kind_name(type), ion_type_to_cdecl(type, ""));
    }
}

void ion_gen_expr(ion_Expr (*expr)) {
    switch (expr->kind) {
    case ION_EXPR_PAREN: {
        ion_buf_printf(&(ion_gen_buf), "(");
        ion_gen_expr(expr->paren.expr);
        ion_buf_printf(&(ion_gen_buf), ")");
        break;
    }
    case ION_EXPR_INT: {
        {
            char const ((*suffix_name)) = ion_token_suffix_names[expr->int_lit.suffix];
            switch (expr->int_lit.mod) {
            case ION_MOD_BIN:
            case ION_MOD_HEX: {
                ion_buf_printf(&(ion_gen_buf), "0x%llx%s", expr->int_lit.val, suffix_name);
                break;
            }
            case ION_MOD_OCT: {
                ion_buf_printf(&(ion_gen_buf), "0%llo%s", expr->int_lit.val, suffix_name);
                break;
            }
            case ION_MOD_CHAR: {
                ion_gen_char((char)(expr->int_lit.val));
                break;
            }
            default: {
                ion_buf_printf(&(ion_gen_buf), "%llu%s", expr->int_lit.val, suffix_name);
                break;
            }
            }
        }
        break;
    }
    case ION_EXPR_FLOAT: {
        {
            int is_double = (expr->float_lit.suffix) == (ION_SUFFIX_D);
            size_t len = (expr->float_lit.end) - (expr->float_lit.start);
            ion_buf_printf(&(ion_gen_buf), "%.*s%s", (is_double ? (len) - (1) : len), expr->float_lit.start, (is_double ? "" : "f"));
        }
        break;
    }
    case ION_EXPR_STR: {
        ion_gen_str(expr->str_lit.val, (expr->str_lit.mod) == (ION_MOD_MULTILINE));
        break;
    }
    case ION_EXPR_NAME: {
        ion_buf_printf(&(ion_gen_buf), "%s", ion_get_gen_name_or_default(expr, expr->name));
        break;
    }
    case ION_EXPR_CAST: {
        ion_buf_printf(&(ion_gen_buf), "(%s)(", ion_typespec_to_cdecl(expr->cast.type, ""));
        ion_gen_expr(expr->cast.expr);
        ion_buf_printf(&(ion_gen_buf), ")");
        break;
    }
    case ION_EXPR_CALL: {
        {
            ion_Sym (*sym) = ion_get_resolved_sym(expr->call.expr);
            if ((sym) && ((sym->kind) == (ION_SYM_TYPE))) {
                ion_buf_printf(&(ion_gen_buf), "(%s)", ion_get_gen_name(sym));
            } else {
                ion_gen_expr(expr->call.expr);
            }
            ion_buf_printf(&(ion_gen_buf), "(");
            for (size_t i = 0; (i) < (expr->call.num_args); (i)++) {
                if ((i) != (0)) {
                    ion_buf_printf(&(ion_gen_buf), ", ");
                }
                ion_gen_expr(expr->call.args[i]);
            }
            ion_buf_printf(&(ion_gen_buf), ")");
        }
        break;
    }
    case ION_EXPR_INDEX: {
        ion_gen_expr(expr->index.expr);
        ion_buf_printf(&(ion_gen_buf), "[");
        ion_gen_expr(expr->index.index);
        ion_buf_printf(&(ion_gen_buf), "]");
        break;
    }
    case ION_EXPR_FIELD: {
        {
            ion_Sym (*sym) = ion_get_resolved_sym(expr);
            if (sym) {
                ion_buf_printf(&(ion_gen_buf), "(%s)", ion_get_gen_name(sym));
            } else {
                ion_gen_expr(expr->field.expr);
                ion_Type (*type) = ion_unqualify_type(ion_get_resolved_type(expr->field.expr));
                ion_buf_printf(&(ion_gen_buf), "%s%s", ((type->kind) == (ION_CMPL_TYPE_PTR) ? "->" : "."), expr->field.name);
            }
        }
        break;
    }
    case ION_EXPR_COMPOUND: {
        ion_gen_expr_compound(expr);
        break;
    }
    case ION_EXPR_UNARY: {
        ion_buf_printf(&(ion_gen_buf), "%s(", ion_token_kind_name(expr->unary.op));
        ion_gen_expr(expr->unary.expr);
        ion_buf_printf(&(ion_gen_buf), ")");
        break;
    }
    case ION_EXPR_BINARY: {
        ion_buf_printf(&(ion_gen_buf), "(");
        ion_gen_expr(expr->binary.left);
        ion_buf_printf(&(ion_gen_buf), ") %s (", ion_token_kind_name(expr->binary.op));
        ion_gen_expr(expr->binary.right);
        ion_buf_printf(&(ion_gen_buf), ")");
        break;
    }
    case ION_EXPR_TERNARY: {
        ion_buf_printf(&(ion_gen_buf), "(");
        ion_gen_expr(expr->ternary.cond);
        ion_buf_printf(&(ion_gen_buf), " ? ");
        ion_gen_expr(expr->ternary.then_expr);
        ion_buf_printf(&(ion_gen_buf), " : ");
        ion_gen_expr(expr->ternary.else_expr);
        ion_buf_printf(&(ion_gen_buf), ")");
        break;
    }
    case ION_EXPR_SIZEOF_EXPR: {
        ion_buf_printf(&(ion_gen_buf), "sizeof(");
        ion_gen_expr(expr->sizeof_expr);
        ion_buf_printf(&(ion_gen_buf), ")");
        break;
    }
    case ION_EXPR_SIZEOF_TYPE: {
        ion_buf_printf(&(ion_gen_buf), "sizeof(%s)", ion_typespec_to_cdecl(expr->sizeof_type, ""));
        break;
    }
    case ION_EXPR_ALIGNOF_EXPR: {
        ion_buf_printf(&(ion_gen_buf), "alignof(%s)", ion_type_to_cdecl(ion_get_resolved_type(expr->alignof_expr), ""));
        break;
    }
    case ION_EXPR_ALIGNOF_TYPE: {
        ion_buf_printf(&(ion_gen_buf), "alignof(%s)", ion_typespec_to_cdecl(expr->alignof_type, ""));
        break;
    }
    case ION_EXPR_TYPEOF_EXPR: {
        {
            ion_Type (*type) = ion_get_resolved_type(expr->typeof_expr);
            assert(type->typeid);
            ion_gen_typeid(type);
        }
        break;
    }
    case ION_EXPR_TYPEOF_TYPE: {
        {
            ion_Type (*type) = ion_get_resolved_type(expr->typeof_type);
            assert(type->typeid);
            ion_gen_typeid(type);
        }
        break;
    }
    case ION_EXPR_OFFSETOF: {
        ion_buf_printf(&(ion_gen_buf), "offsetof(%s, %s)", ion_typespec_to_cdecl(expr->offsetof_field.type, ""), expr->offsetof_field.name);
        break;
    }
    case ION_EXPR_MODIFY: {
        if (!(expr->modify.post)) {
            ion_buf_printf(&(ion_gen_buf), "%s", ion_token_kind_name(expr->modify.op));
        }
        ion_gen_paren_expr(expr->modify.expr);
        if (expr->modify.post) {
            ion_buf_printf(&(ion_gen_buf), "%s", ion_token_kind_name(expr->modify.op));
        }
        break;
    }
    default: {
        assert(0);
        break;
    }
    }
}

void ion_gen_stmt_block(ion_StmtList block) {
    ion_buf_printf(&(ion_gen_buf), "{");
    (ion_gen_indent)++;
    for (size_t i = 0; (i) < (block.num_stmts); (i)++) {
        ion_gen_stmt(block.stmts[i]);
    }
    (ion_gen_indent)--;
    ion_genln();
    ion_buf_printf(&(ion_gen_buf), "}");
}

void ion_gen_simple_stmt(ion_Stmt (*stmt)) {
    switch (stmt->kind) {
    case ION_STMT_EXPR: {
        ion_gen_expr(stmt->expr);
        break;
    }
    case ION_STMT_INIT: {
        if (stmt->init.type) {
            ion_Typespec (*init_typespec) = stmt->init.type;
            if (ion_is_incomplete_array_typespec(stmt->init.type)) {
                ion_Expr (*size) = ion_new_expr_int(init_typespec->pos, ion_get_resolved_type(stmt->init.expr)->num_elems, 0, 0);
                init_typespec = ion_new_typespec_array(init_typespec->pos, init_typespec->base, size);
            }
            ion_buf_printf(&(ion_gen_buf), "%s = ", ion_typespec_to_cdecl(stmt->init.type, stmt->init.name));
            if (stmt->init.expr) {
                ion_gen_expr(stmt->init.expr);
            } else {
                ion_buf_printf(&(ion_gen_buf), "{0}");
            }
        } else {
            ion_buf_printf(&(ion_gen_buf), "%s = ", ion_type_to_cdecl(ion_unqualify_type(ion_get_resolved_type(stmt->init.expr)), stmt->init.name));
            ion_gen_expr(stmt->init.expr);
        }
        break;
    }
    case ION_STMT_ASSIGN: {
        ion_gen_expr(stmt->assign.left);
        ion_buf_printf(&(ion_gen_buf), " %s ", ion_token_kind_name(stmt->assign.op));
        ion_gen_expr(stmt->assign.right);
        break;
    }
    default: {
        assert(0);
        break;
    }
    }
}

bool ion_is_char_lit(ion_Expr (*expr)) {
    return ((expr->kind) == (ION_EXPR_INT)) && ((expr->int_lit.mod) == (ION_MOD_CHAR));
}

void ion_gen_stmt(ion_Stmt (*stmt)) {
    ion_gen_sync_pos(stmt->pos);
    switch (stmt->kind) {
    case ION_STMT_RETURN: {
        ion_genln();
        ion_buf_printf(&(ion_gen_buf), "return");
        if (stmt->expr) {
            ion_buf_printf(&(ion_gen_buf), " ");
            ion_gen_expr(stmt->expr);
        }
        ion_buf_printf(&(ion_gen_buf), ";");
        break;
    }
    case ION_STMT_BREAK: {
        ion_genln();
        ion_buf_printf(&(ion_gen_buf), "break;");
        break;
    }
    case ION_STMT_CONTINUE: {
        ion_genln();
        ion_buf_printf(&(ion_gen_buf), "continue;");
        break;
    }
    case ION_STMT_BLOCK: {
        ion_genln();
        ion_gen_stmt_block(stmt->block);
        break;
    }
    case ION_STMT_NOTE: {
        if ((stmt->note.name) == (ion_assert_name)) {
            ion_genln();
            ion_buf_printf(&(ion_gen_buf), "assert(");
            assert((stmt->note.num_args) == (1));
            ion_gen_expr(stmt->note.args[0].expr);
            ion_buf_printf(&(ion_gen_buf), ");");
        }
        break;
    }
    case ION_STMT_IF: {
        if (stmt->if_stmt.init) {
            ion_genln();
            ion_buf_printf(&(ion_gen_buf), "{");
            (ion_gen_indent)++;
            ion_gen_stmt(stmt->if_stmt.init);
        }
        ion_gen_sync_pos(stmt->pos);
        ion_genln();
        ion_buf_printf(&(ion_gen_buf), "if (");
        if (stmt->if_stmt.cond) {
            ion_gen_expr(stmt->if_stmt.cond);
        } else {
            ion_buf_printf(&(ion_gen_buf), "%s", stmt->if_stmt.init->init.name);
        }
        ion_buf_printf(&(ion_gen_buf), ") ");
        ion_gen_stmt_block(stmt->if_stmt.then_block);
        for (size_t i = 0; (i) < (stmt->if_stmt.num_elseifs); (i)++) {
            ion_ElseIf elseif = stmt->if_stmt.elseifs[i];
            ion_buf_printf(&(ion_gen_buf), " else if (");
            ion_gen_expr(elseif.cond);
            ion_buf_printf(&(ion_gen_buf), ") ");
            ion_gen_stmt_block(elseif.block);
        }
        if (stmt->if_stmt.else_block.stmts) {
            ion_buf_printf(&(ion_gen_buf), " else ");
            ion_gen_stmt_block(stmt->if_stmt.else_block);
        } else {
            ion_Note (*complete_note) = ion_get_stmt_note(stmt, ion_complete_name);
            if (complete_note) {
                ion_buf_printf(&(ion_gen_buf), " else {");
                (ion_gen_indent)++;
                ion_gen_sync_pos(complete_note->pos);
                ion_genln();
                ion_buf_printf(&(ion_gen_buf), "assert(\"@complete if/elseif chain failed to handle case\" && 0);");
                (ion_gen_indent)--;
                ion_genln();
                ion_buf_printf(&(ion_gen_buf), "}");
            }
        }
        if (stmt->if_stmt.init) {
            (ion_gen_indent)--;
            ion_genln();
            ion_buf_printf(&(ion_gen_buf), "}");
        }
        break;
    }
    case ION_STMT_WHILE: {
        ion_genln();
        ion_buf_printf(&(ion_gen_buf), "while (");
        ion_gen_expr(stmt->while_stmt.cond);
        ion_buf_printf(&(ion_gen_buf), ") ");
        ion_gen_stmt_block(stmt->while_stmt.block);
        break;
    }
    case ION_STMT_DO_WHILE: {
        ion_genln();
        ion_buf_printf(&(ion_gen_buf), "do ");
        ion_gen_stmt_block(stmt->while_stmt.block);
        ion_buf_printf(&(ion_gen_buf), " while (");
        ion_gen_expr(stmt->while_stmt.cond);
        ion_buf_printf(&(ion_gen_buf), ");");
        break;
    }
    case ION_STMT_FOR: {
        ion_genln();
        ion_buf_printf(&(ion_gen_buf), "for (");
        if (stmt->for_stmt.init) {
            ion_gen_simple_stmt(stmt->for_stmt.init);
        }
        ion_buf_printf(&(ion_gen_buf), ";");
        if (stmt->for_stmt.cond) {
            ion_buf_printf(&(ion_gen_buf), " ");
            ion_gen_expr(stmt->for_stmt.cond);
        }
        ion_buf_printf(&(ion_gen_buf), ";");
        if (stmt->for_stmt.next) {
            ion_buf_printf(&(ion_gen_buf), " ");
            ion_gen_simple_stmt(stmt->for_stmt.next);
        }
        ion_buf_printf(&(ion_gen_buf), ") ");
        ion_gen_stmt_block(stmt->for_stmt.block);
        break;
    }
    case ION_STMT_SWITCH: {
        {
            ion_genln();
            ion_buf_printf(&(ion_gen_buf), "switch (");
            ion_gen_expr(stmt->switch_stmt.expr);
            ion_buf_printf(&(ion_gen_buf), ") {");
            bool has_default = false;
            for (size_t i = 0; (i) < (stmt->switch_stmt.num_cases); (i)++) {
                ion_SwitchCase switch_case = stmt->switch_stmt.cases[i];
                for (size_t j = 0; (j) < (switch_case.num_patterns); (j)++) {
                    ion_SwitchCasePattern pattern = switch_case.patterns[j];
                    if (pattern.end) {
                        ion_Val start_val = ion_get_resolved_val(pattern.start);
                        ion_Val end_val = ion_get_resolved_val(pattern.end);
                        if ((ion_is_char_lit(pattern.start)) && (ion_is_char_lit(pattern.end))) {
                            ion_genln();
                            for (int c = (int)(start_val.ll); (c) <= ((int)(end_val.ll)); (c)++) {
                                ion_buf_printf(&(ion_gen_buf), "case ");
                                ion_gen_char(c);
                                ion_buf_printf(&(ion_gen_buf), ": ");
                            }
                        } else {
                            ion_genln();
                            ion_buf_printf(&(ion_gen_buf), "// ");
                            ion_gen_expr(pattern.start);
                            ion_buf_printf(&(ion_gen_buf), "...");
                            ion_gen_expr(pattern.end);
                            ion_genln();
                            for (llong ll = start_val.ll; (ll) <= (end_val.ll); (ll)++) {
                                ion_buf_printf(&(ion_gen_buf), "case %lld: ", ll);
                            }
                        }
                    } else {
                        ion_genln();
                        ion_buf_printf(&(ion_gen_buf), "case ");
                        ion_gen_expr(pattern.start);
                        ion_buf_printf(&(ion_gen_buf), ":");
                    }
                }
                if (switch_case.is_default) {
                    has_default = true;
                    ion_genln();
                    ion_buf_printf(&(ion_gen_buf), "default:");
                }
                ion_buf_printf(&(ion_gen_buf), " ");
                ion_buf_printf(&(ion_gen_buf), "{");
                (ion_gen_indent)++;
                ion_StmtList block = switch_case.block;
                for (size_t j = 0; (j) < (block.num_stmts); (j)++) {
                    ion_gen_stmt(block.stmts[j]);
                }
                ion_genln();
                ion_buf_printf(&(ion_gen_buf), "break;");
                (ion_gen_indent)--;
                ion_genln();
                ion_buf_printf(&(ion_gen_buf), "}");
            }
            if (!(has_default)) {
                ion_Note (*note) = ion_get_stmt_note(stmt, ion_complete_name);
                if (note) {
                    ion_genln();
                    ion_buf_printf(&(ion_gen_buf), "default:");
                    (ion_gen_indent)++;
                    ion_genln();
                    ion_buf_printf(&(ion_gen_buf), "assert(\"@complete switch failed to handle case\" && 0);");
                    ion_genln();
                    ion_buf_printf(&(ion_gen_buf), "break;");
                    (ion_gen_indent)--;
                }
            }
            ion_genln();
            ion_buf_printf(&(ion_gen_buf), "}");
        }
        break;
    }
    case ION_STMT_LABEL: {
        ion_genln();
        ion_buf_printf(&(ion_gen_buf), "%s: ;", stmt->label);
        break;
    }
    case ION_STMT_GOTO: {
        ion_genln();
        ion_buf_printf(&(ion_gen_buf), "goto %s;", stmt->label);
        break;
    }
    default: {
        ion_genln();
        ion_gen_simple_stmt(stmt);
        ion_buf_printf(&(ion_gen_buf), ";");
        break;
    }
    }
}

void ion_gen_decl(ion_Sym (*sym)) {
    ion_Decl (*decl) = sym->decl;
    if ((!(decl)) || (ion_is_decl_foreign(decl))) {
        return;
    }
    ion_gen_sync_pos(decl->pos);
    switch (decl->kind) {
    case ION_DECL_CONST: {
        ion_genln();
        ion_buf_printf(&(ion_gen_buf), "#define %s (", ion_get_gen_name(sym));
        if (decl->const_decl.type) {
            ion_buf_printf(&(ion_gen_buf), "(%s)(", ion_typespec_to_cdecl(decl->const_decl.type, ""));
        }
        ion_gen_expr(decl->const_decl.expr);
        if (decl->const_decl.type) {
            ion_buf_printf(&(ion_gen_buf), ")");
        }
        ion_buf_printf(&(ion_gen_buf), ")");
        break;
    }
    case ION_DECL_VAR: {
        ion_genln();
        ion_buf_printf(&(ion_gen_buf), "extern ");
        if ((decl->var_decl.type) && (!(ion_is_incomplete_array_typespec(decl->var_decl.type)))) {
            ion_buf_printf(&(ion_gen_buf), "%s", ion_typespec_to_cdecl(decl->var_decl.type, ion_get_gen_name(sym)));
        } else {
            ion_buf_printf(&(ion_gen_buf), "%s", ion_type_to_cdecl(sym->type, ion_get_gen_name(sym)));
        }
        ion_buf_printf(&(ion_gen_buf), ";");
        break;
    }
    case ION_DECL_FUNC: {
        ion_gen_func_decl(decl);
        ion_buf_printf(&(ion_gen_buf), ";");
        break;
    }
    case ION_DECL_STRUCT:
    case ION_DECL_UNION: {
        ion_gen_aggregate(decl);
        break;
    }
    case ION_DECL_TYPEDEF: {
        ion_genln();
        ion_buf_printf(&(ion_gen_buf), "typedef %s;", ion_typespec_to_cdecl(decl->typedef_decl.type, ion_get_gen_name(sym)));
        break;
    }
    case ION_DECL_ENUM: {
        if (decl->enum_decl.type) {
            ion_genln();
            ion_buf_printf(&(ion_gen_buf), "typedef %s;", ion_typespec_to_cdecl(decl->enum_decl.type, ion_get_gen_name(decl)));
        } else {
            ion_genln();
            ion_buf_printf(&(ion_gen_buf), "typedef int %s;", ion_get_gen_name(decl));
        }
        break;
    }
    case ION_DECL_IMPORT: {
        break;
        break;
    }
    default: {
        assert(0);
        break;
    }
    }
    ion_genln();
}

void ion_gen_sorted_decls(void) {
    for (size_t i = 0; (i) < (ion_buf_len(ion_sorted_syms)); (i)++) {
        if ((ion_sorted_syms[i]->reachable) == (ION_REACHABLE_NATURAL)) {
            ion_gen_decl(ion_sorted_syms[i]);
        }
    }
}

void ion_gen_defs(void) {
    for (ion_Sym (*(*it)) = ion_sorted_syms; (it) != (ion_buf_end(ion_sorted_syms, sizeof(*(ion_sorted_syms)))); (it)++) {
        ion_Sym (*sym) = *(it);
        ion_Decl (*decl) = sym->decl;
        if ((((((sym->state) != (ION_SYM_RESOLVED)) || (!(decl))) || (ion_is_decl_foreign(decl))) || (decl->is_incomplete)) || ((sym->reachable) != (ION_REACHABLE_NATURAL))) {
            continue;
        }
        if ((decl->kind) == (ION_DECL_FUNC)) {
            ion_gen_func_decl(decl);
            ion_buf_printf(&(ion_gen_buf), " ");
            ion_gen_stmt_block(decl->function.block);
            ion_genln();
        } else if ((decl->kind) == (ION_DECL_VAR)) {
            if ((decl->var_decl.type) && (!(ion_is_incomplete_array_typespec(decl->var_decl.type)))) {
                ion_genln();
                ion_buf_printf(&(ion_gen_buf), "%s", ion_typespec_to_cdecl(decl->var_decl.type, ion_get_gen_name(sym)));
            } else {
                ion_genln();
                ion_buf_printf(&(ion_gen_buf), "%s", ion_type_to_cdecl(sym->type, ion_get_gen_name(sym)));
            }
            if (decl->var_decl.expr) {
                ion_buf_printf(&(ion_gen_buf), " = ");
                ion_gen_expr(decl->var_decl.expr);
            }
            ion_buf_printf(&(ion_gen_buf), ";");
        }
    }
}

ion_Map ion_gen_foreign_headers_map;
char const ((*(*ion_gen_foreign_headers_buf)));
void ion_add_foreign_header(char const ((*name))) {
    name = ion_str_intern(name);
    if (!(ion_map_get(&(ion_gen_foreign_headers_map), name))) {
        ion_map_put(&(ion_gen_foreign_headers_map), name, (void *)(1));
        ion_buf_push((void (**))(&(ion_gen_foreign_headers_buf)), &(name), sizeof(name));
    }
}

char const ((*(*ion_gen_foreign_sources_buf)));
void ion_add_foreign_source(char const ((*name))) {
    char const ((*interned)) = ion_str_intern(name);
    ion_buf_push((void (**))(&(ion_gen_foreign_sources_buf)), &(interned), sizeof(interned));
}

void ion_gen_include(char const ((*path))) {
    ion_genln();
    ion_buf_printf(&(ion_gen_buf), "#include ");
    if ((*(path)) == ('<')) {
        ion_buf_printf(&(ion_gen_buf), "%s", path);
    } else {
        ion_gen_str(path, false);
    }
}

void ion_gen_foreign_headers(void) {
    if (ion_gen_foreign_headers_buf) {
        ion_genln();
        ion_buf_printf(&(ion_gen_buf), "// Foreign header files");
        for (size_t i = 0; (i) < (ion_buf_len(ion_gen_foreign_headers_buf)); (i)++) {
            ion_gen_include(ion_gen_foreign_headers_buf[i]);
        }
    }
}

void ion_gen_foreign_sources(void) {
    for (size_t i = 0; (i) < (ion_buf_len(ion_gen_foreign_sources_buf)); (i)++) {
        ion_gen_include(ion_gen_foreign_sources_buf[i]);
    }
}

char const ((*(*ion_gen_sources_buf)));
void ion_put_include_path(char (path[MAX_PATH]), ion_Package (*package), char const ((*filename))) {
    if ((*(filename)) == ('<')) {
        ion_path_copy(path, filename);
    } else {
        ion_path_copy(path, package->full_path);
        ion_path_join(path, filename);
        ion_path_absolute(path);
    }
}

char (*ion_gen_preamble_buf);
char (*ion_gen_postamble_buf);
void ion_preprocess_package(ion_Package (*package)) {
    if (!(package->external_name)) {
        char (*external_name) = NULL;
        for (char const ((*ptr)) = package->path; *(ptr); (ptr)++) {
            ion_buf_printf(&(external_name), "%c", ((*(ptr)) == ('/') ? '_' : *(ptr)));
        }
        ion_buf_printf(&(external_name), "_");
        package->external_name = ion_str_intern(external_name);
    }
    char const ((*header_name)) = ion_str_intern("header");
    char const ((*source_name)) = ion_str_intern("source");
    char const ((*preamble_name)) = ion_str_intern("preamble");
    char const ((*postamble_name)) = ion_str_intern("postamble");
    for (size_t i = 0; (i) < (package->num_decls); (i)++) {
        ion_Decl (*decl) = package->decls[i];
        if ((decl->kind) != (ION_DECL_NOTE)) {
            continue;
        }
        ion_Note note = decl->note;
        if ((note.name) == (ion_foreign_name)) {
            for (size_t k = 0; (k) < (note.num_args); (k)++) {
                ion_NoteArg arg = note.args[k];
                ion_Expr (*expr) = note.args[k].expr;
                if ((expr->kind) != (ION_EXPR_STR)) {
                    ion_fatal_error(decl->pos, "#foreign argument must be a string");
                }
                char const ((*str)) = expr->str_lit.val;
                if ((arg.name) == (header_name)) {
                    char (path[MAX_PATH]) = {0};
                    ion_put_include_path(path, package, str);
                    ion_add_foreign_header(path);
                } else if ((arg.name) == (source_name)) {
                    char (path[MAX_PATH]) = {0};
                    ion_put_include_path(path, package, str);
                    ion_add_foreign_source(path);
                } else if ((arg.name) == (preamble_name)) {
                    ion_buf_printf(&(ion_gen_preamble_buf), "%s\n", str);
                } else if ((arg.name) == (postamble_name)) {
                    ion_buf_printf(&(ion_gen_postamble_buf), "%s\n", str);
                } else {
                    ion_fatal_error(decl->pos, "Unknown #foreign named argument \'%s\'", arg.name);
                }
            }
        }
    }
}

void ion_preprocess_packages(void) {
    for (size_t i = 0; (i) < (ion_buf_len(ion_package_list)); (i)++) {
        ion_preprocess_package(ion_package_list[i]);
    }
}

void ion_gen_typeinfo_header(char const ((*kind)), ion_Type (*type)) {
    if ((ion_type_sizeof(type)) == (0)) {
        ion_buf_printf(&(ion_gen_buf), "&(TypeInfo){%s, .size = 0, .align = 0", kind);
    } else {
        char (*ctype) = ion_type_to_cdecl(type, "");
        ion_buf_printf(&(ion_gen_buf), "&(TypeInfo){%s, .size = sizeof(%s), .align = alignof(%s)", kind, ctype, ctype);
    }
}

void ion_gen_typeinfo_fields(ion_Type (*type)) {
    (ion_gen_indent)++;
    for (size_t i = 0; (i) < (type->aggregate.num_fields); (i)++) {
        ion_TypeField field = type->aggregate.fields[i];
        ion_genln();
        ion_buf_printf(&(ion_gen_buf), "{");
        ion_gen_str(field.name, false);
        ion_buf_printf(&(ion_gen_buf), ", .type = ");
        ion_gen_typeid(field.type);
        ion_buf_printf(&(ion_gen_buf), ", .offset = offsetof(%s, %s)},", ion_get_gen_name(type->sym), field.name);
    }
    (ion_gen_indent)--;
}

void ion_gen_typeinfo(ion_Type (*type)) {
    switch (type->kind) {
    case ION_CMPL_TYPE_BOOL: {
        ion_buf_printf(&(ion_gen_buf), "&(TypeInfo){TYPE_BOOL, .size = sizeof(bool), .align = sizeof(bool), .name = ");
        ion_gen_str("bool", false);
        ion_buf_printf(&(ion_gen_buf), "},");
        break;
    }
    case ION_CMPL_TYPE_CHAR: {
        ion_buf_printf(&(ion_gen_buf), "&(TypeInfo){TYPE_CHAR, .size = sizeof(char), .align = sizeof(char), .name = ");
        ion_gen_str("char", false);
        ion_buf_printf(&(ion_gen_buf), "},");
        break;
    }
    case ION_CMPL_TYPE_UCHAR: {
        ion_buf_printf(&(ion_gen_buf), "&(TypeInfo){TYPE_UCHAR, .size = sizeof(uchar), .align = sizeof(uchar), .name = ");
        ion_gen_str("uchar", false);
        ion_buf_printf(&(ion_gen_buf), "},");
        break;
    }
    case ION_CMPL_TYPE_SCHAR: {
        ion_buf_printf(&(ion_gen_buf), "&(TypeInfo){TYPE_SCHAR, .size = sizeof(schar), .align = sizeof(schar), .name = ");
        ion_gen_str("schar", false);
        ion_buf_printf(&(ion_gen_buf), "},");
        break;
    }
    case ION_CMPL_TYPE_SHORT: {
        ion_buf_printf(&(ion_gen_buf), "&(TypeInfo){TYPE_SHORT, .size = sizeof(short), .align = sizeof(short), .name = ");
        ion_gen_str("short", false);
        ion_buf_printf(&(ion_gen_buf), "},");
        break;
    }
    case ION_CMPL_TYPE_USHORT: {
        ion_buf_printf(&(ion_gen_buf), "&(TypeInfo){TYPE_USHORT, .size = sizeof(ushort), .align = sizeof(ushort), .name = ");
        ion_gen_str("ushort", false);
        ion_buf_printf(&(ion_gen_buf), "},");
        break;
    }
    case ION_CMPL_TYPE_INT: {
        ion_buf_printf(&(ion_gen_buf), "&(TypeInfo){TYPE_INT, .size = sizeof(int), .align = sizeof(int), .name = ");
        ion_gen_str("int", false);
        ion_buf_printf(&(ion_gen_buf), "},");
        break;
    }
    case ION_CMPL_TYPE_UINT: {
        ion_buf_printf(&(ion_gen_buf), "&(TypeInfo){TYPE_UINT, .size = sizeof(uint), .align = sizeof(uint), .name = ");
        ion_gen_str("uint", false);
        ion_buf_printf(&(ion_gen_buf), "},");
        break;
    }
    case ION_CMPL_TYPE_LONG: {
        ion_buf_printf(&(ion_gen_buf), "&(TypeInfo){TYPE_LONG, .size = sizeof(long), .align = sizeof(long), .name = ");
        ion_gen_str("long", false);
        ion_buf_printf(&(ion_gen_buf), "},");
        break;
    }
    case ION_CMPL_TYPE_ULONG: {
        ion_buf_printf(&(ion_gen_buf), "&(TypeInfo){TYPE_ULONG, .size = sizeof(ulong), .align = sizeof(ulong), .name = ");
        ion_gen_str("ulong", false);
        ion_buf_printf(&(ion_gen_buf), "},");
        break;
    }
    case ION_CMPL_TYPE_LLONG: {
        ion_buf_printf(&(ion_gen_buf), "&(TypeInfo){TYPE_LLONG, .size = sizeof(llong), .align = sizeof(llong), .name = ");
        ion_gen_str("llong", false);
        ion_buf_printf(&(ion_gen_buf), "},");
        break;
    }
    case ION_CMPL_TYPE_ULLONG: {
        ion_buf_printf(&(ion_gen_buf), "&(TypeInfo){TYPE_ULLONG, .size = sizeof(ullong), .align = sizeof(ullong), .name = ");
        ion_gen_str("ullong", false);
        ion_buf_printf(&(ion_gen_buf), "},");
        break;
    }
    case ION_CMPL_TYPE_FLOAT: {
        ion_buf_printf(&(ion_gen_buf), "&(TypeInfo){TYPE_FLOAT, .size = sizeof(float), .align = sizeof(float), .name = ");
        ion_gen_str("float", false);
        ion_buf_printf(&(ion_gen_buf), "},");
        break;
    }
    case ION_CMPL_TYPE_DOUBLE: {
        ion_buf_printf(&(ion_gen_buf), "&(TypeInfo){TYPE_DOUBLE, .size = sizeof(double), .align = sizeof(double), .name = ");
        ion_gen_str("double", false);
        ion_buf_printf(&(ion_gen_buf), "},");
        break;
    }
    case ION_CMPL_TYPE_VOID: {
        ion_buf_printf(&(ion_gen_buf), "&(TypeInfo){TYPE_VOID, .name = \"void\", .size = 0, .align = 0},");
        break;
    }
    case ION_CMPL_TYPE_PTR: {
        ion_buf_printf(&(ion_gen_buf), "&(TypeInfo){TYPE_PTR, .size = sizeof(void *), .align = alignof(void *), .base = ");
        ion_gen_typeid(type->base);
        ion_buf_printf(&(ion_gen_buf), "},");
        break;
    }
    case ION_CMPL_TYPE_CONST: {
        ion_gen_typeinfo_header("TYPE_CONST", type);
        ion_buf_printf(&(ion_gen_buf), ", .base = ");
        ion_gen_typeid(type->base);
        ion_buf_printf(&(ion_gen_buf), "},");
        break;
    }
    case ION_CMPL_TYPE_ARRAY: {
        if (ion_is_incomplete_array_type(type)) {
            ion_buf_printf(&(ion_gen_buf), "NULL, // Incomplete array type");
        } else {
            ion_gen_typeinfo_header("TYPE_ARRAY", type);
            ion_buf_printf(&(ion_gen_buf), ", .base = ");
            ion_gen_typeid(type->base);
            ion_buf_printf(&(ion_gen_buf), ", .count = %d},", type->num_elems);
        }
        break;
    }
    case ION_CMPL_TYPE_STRUCT:
    case ION_CMPL_TYPE_UNION: {
        ion_gen_typeinfo_header(((type->kind) == (ION_CMPL_TYPE_STRUCT) ? "TYPE_STRUCT" : "TYPE_UNION"), type);
        ion_buf_printf(&(ion_gen_buf), ", .name = ");
        ion_gen_str(ion_get_gen_name(type->sym), false);
        ion_buf_printf(&(ion_gen_buf), ", .num_fields = %d, .fields = (TypeFieldInfo[]) {", type->aggregate.num_fields);
        ion_gen_typeinfo_fields(type);
        ion_genln();
        ion_buf_printf(&(ion_gen_buf), "}},");
        break;
    }
    case ION_CMPL_TYPE_FUNC: {
        ion_buf_printf(&(ion_gen_buf), "NULL, // Func");
        break;
    }
    case ION_CMPL_TYPE_ENUM: {
        ion_buf_printf(&(ion_gen_buf), "NULL, // Enum");
        break;
    }
    case ION_CMPL_TYPE_INCOMPLETE: {
        ion_buf_printf(&(ion_gen_buf), "NULL, // Incomplete: %s", ion_get_gen_name(type->sym));
        break;
    }
    default: {
        ion_buf_printf(&(ion_gen_buf), "NULL, // Unhandled");
        break;
    }
    }
}

void ion_gen_typeinfos(void) {
    ion_genln();
    ion_buf_printf(&(ion_gen_buf), "#define TYPEID0(index, kind) ((ullong)(index) | ((ullong)(kind) << 24))");
    ion_genln();
    ion_buf_printf(&(ion_gen_buf), "#define TYPEID(index, kind, ...) ((ullong)(index) | ((ullong)sizeof(__VA_ARGS__) << 32) | ((ullong)(kind) << 24))");
    ion_genln();
    if (ion_flag_notypeinfo) {
        ion_genln();
        ion_buf_printf(&(ion_gen_buf), "int num_typeinfos;");
        ion_genln();
        ion_buf_printf(&(ion_gen_buf), "const TypeInfo **typeinfos;");
    } else {
        int num_typeinfos = ion_next_typeid;
        ion_genln();
        ion_buf_printf(&(ion_gen_buf), "const TypeInfo *typeinfo_table[%d] = {", num_typeinfos);
        (ion_gen_indent)++;
        for (int typeid = 0; (typeid) < (num_typeinfos); (typeid)++) {
            ion_genln();
            ion_buf_printf(&(ion_gen_buf), "[%d] = ", typeid);
            ion_Type (*type) = ion_get_type_from_typeid(typeid);
            if ((type) && (!(ion_is_excluded_typeinfo(type)))) {
                ion_gen_typeinfo(type);
            } else {
                ion_buf_printf(&(ion_gen_buf), "NULL, // No associated type");
            }
        }
        (ion_gen_indent)--;
        ion_genln();
        ion_buf_printf(&(ion_gen_buf), "};");
        ion_genln();
        ion_genln();
        ion_buf_printf(&(ion_gen_buf), "int num_typeinfos = %d;", num_typeinfos);
        ion_genln();
        ion_buf_printf(&(ion_gen_buf), "const TypeInfo **typeinfos = (const TypeInfo **)typeinfo_table;");
    }
}

void ion_gen_package_external_names(void) {
    for (size_t i = 0; (i) < (ion_buf_len(ion_package_list)); (i)++) {
    }
}

void ion_gen_preamble(void) {
    ion_buf_printf(&(ion_gen_buf), "%s", ion_gen_preamble_str);
    if (ion_gen_preamble_buf) {
        ion_genln();
        ion_genln();
        ion_buf_printf(&(ion_gen_buf), "// Foreign preamble");
        ion_genln();
        ion_buf_printf(&(ion_gen_buf), "%s", ion_gen_preamble_buf);
    }
}

void ion_gen_postamble(void) {
    ion_buf_printf(&(ion_gen_buf), "%s", ion_gen_postamble_str);
    if (ion_gen_postamble_buf) {
        ion_genln();
        ion_genln();
        ion_buf_printf(&(ion_gen_buf), "// Foreign postamble");
        ion_genln();
        ion_buf_printf(&(ion_gen_buf), "%s", ion_gen_postamble_buf);
    }
}

void ion_gen_all(void) {
    ion_preprocess_packages();
    ion_gen_buf = NULL;
    ion_gen_preamble();
    ion_gen_foreign_headers();
    ion_genln();
    ion_genln();
    ion_buf_printf(&(ion_gen_buf), "// Forward declarations");
    ion_gen_forward_decls();
    ion_genln();
    ion_genln();
    ion_buf_printf(&(ion_gen_buf), "// Sorted declarations");
    ion_gen_sorted_decls();
    ion_genln();
    ion_buf_printf(&(ion_gen_buf), "// Typeinfo");
    ion_gen_typeinfos();
    ion_genln();
    ion_genln();
    ion_buf_printf(&(ion_gen_buf), "// Definitions");
    ion_gen_defs();
    ion_genln();
    ion_buf_printf(&(ion_gen_buf), "// Foreign source files");
    ion_gen_foreign_sources();
    ion_genln();
    ion_gen_postamble();
}

char const ((*(ion_static_package_search_paths[ION_MAX_SEARCH_PATHS])));
char const ((*(*ion_package_search_paths))) = ion_static_package_search_paths;
int ion_num_package_search_paths;
void ion_add_package_search_path(char const ((*path))) {
    if (ion_flag_verbose) {
        printf("Adding package search path %s\n", path);
    }
    ion_package_search_paths[(ion_num_package_search_paths)++] = ion_str_intern(path);
}

void ion_add_package_search_path_range(char const ((*start)), char const ((*end))) {
    char (path[MAX_PATH]) = {0};
    size_t len = ion_clamp_max((end) - (start), (MAX_PATH) - (1));
    memcpy(path, start, len);
    path[len] = 0;
    ion_add_package_search_path(path);
}

void ion_init_package_search_paths(void) {
    char (*ionhome_var) = getenv("IONHOME");
    if (!(ionhome_var)) {
        printf("error: Set the environment variable IONHOME to the Ion home directory (where system_packages is located)\n");
        exit(1);
    }
    char (path[MAX_PATH]) = {0};
    ion_path_copy(path, ionhome_var);
    ion_path_join(path, "system_packages");
    ion_add_package_search_path(path);
    ion_add_package_search_path(".");
    char (*ionpath_var) = getenv("IONPATH");
    if (ionpath_var) {
        char (*start) = ionpath_var;
        for (char (*ptr) = ionpath_var; *(ptr); (ptr)++) {
            if ((*(ptr)) == (';')) {
                ion_add_package_search_path_range(start, ptr);
                start = (ptr) + (1);
            }
        }
        if (*(start)) {
            ion_add_package_search_path(start);
        }
    }
}

void ion_init_compiler(void) {
    ion_init_target();
    ion_init_package_search_paths();
    ion_init_keywords();
    ion_init_builtin_types();
    ion_map_put(&(ion_decl_note_names), ion_declare_note_name, (void *)(1));
}

void ion_parse_env_vars(void) {
    char (*ionos_var) = getenv("IONOS");
    if (ionos_var) {
        int os = ion_get_os(ionos_var);
        if ((os) == (-(1))) {
            printf("Unknown target operating system in IONOS environment variable: %s\n", ionos_var);
        } else {
            ion_target_os = os;
        }
    }
    char (*ionarch_var) = getenv("IONARCH");
    if (ionarch_var) {
        int arch = ion_get_arch(ionarch_var);
        if ((arch) == (-(1))) {
            printf("Unknown target architecture in IONARCH environment variable: %s\n", ionarch_var);
        } else {
            ion_target_arch = arch;
        }
    }
}

int ion_ion_main(int argc, char const ((*(*argv))), void (*gen_all)(void), char const ((*extension))) {
    ion_parse_env_vars();
    char const ((*output_name)) = {0};
    bool flag_check = false;
    ion_add_flag_str("o", &(output_name), "file", "Output file (default: out_<main-package>.c)");
    ion_add_flag_enum("os", &(ion_target_os), "Target operating system", ion_os_names, ION_NUM_OSES);
    ion_add_flag_enum("arch", &(ion_target_arch), "Target machine architecture", ion_arch_names, ION_NUM_ARCHES);
    ion_add_flag_bool("check", &(flag_check), "Semantic checking with no code generation");
    ion_add_flag_bool("lazy", &(ion_flag_lazy), "Only compile what\'s reachable from the main package");
    ion_add_flag_bool("nosourcemap", &(ion_flag_nosourcemap), "Don\'t generate any source map information");
    ion_add_flag_bool("notypeinfo", &(ion_flag_notypeinfo), "Don\'t generate any typeinfo tables");
    ion_add_flag_bool("fullgen", &(ion_flag_fullgen), "Force full code generation even for non-reachable symbols");
    ion_add_flag_bool("verbose", &(ion_flag_verbose), "Extra diagnostic information");
    char const ((*program_name)) = ion_parse_flags(&(argc), &(argv));
    if ((argc) != (1)) {
        printf("Usage: %s [flags] <main-package>\n", program_name);
        ion_print_flags_usage();
        return 1;
    }
    char const ((*package_name)) = argv[0];
    if (ion_flag_verbose) {
        printf("Target operating system: %s\n", ion_os_names[ion_target_os]);
        printf("Target architecture: %s\n", ion_arch_names[ion_target_arch]);
    }
    ion_init_compiler();
    ion_builtin_package = ion_import_package("builtin");
    if (!(ion_builtin_package)) {
        printf("error: Failed to compile package \'builtin\'.\n");
        return 1;
    }
    ion_builtin_package->external_name = ion_str_intern("");
    ion_Package (*main_package) = ion_import_package(package_name);
    if (!(main_package)) {
        printf("error: Failed to compile package \'%s\'\n", package_name);
        return 1;
    }
    char const ((*main_name)) = ion_str_intern("main");
    ion_Sym (*main_sym) = ion_get_package_sym(main_package, main_name);
    if (!(main_sym)) {
        printf("error: No \'main\' entry point defined in package \'%s\'\n", package_name);
        return 1;
    }
    main_sym->external_name = main_name;
    ion_reachable_phase = ION_REACHABLE_NATURAL;
    ion_resolve_sym(main_sym);
    for (size_t i = 0; (i) < (ion_buf_len(ion_package_list)); (i)++) {
        if (ion_package_list[i]->always_reachable) {
            ion_resolve_package_syms(ion_package_list[i]);
        }
    }
    ion_finalize_reachable_syms();
    if (ion_flag_verbose) {
        printf("Reached %d symbols in %d packages from %s/main\n", (int)(ion_buf_len(ion_reachable_syms)), (int)(ion_buf_len(ion_package_list)), package_name);
    }
    if (!(ion_flag_lazy)) {
        ion_reachable_phase = ION_REACHABLE_FORCED;
        for (size_t i = 0; (i) < (ion_buf_len(ion_package_list)); (i)++) {
            ion_resolve_package_syms(ion_package_list[i]);
        }
        ion_finalize_reachable_syms();
    }
    printf("Processed %d symbols in %d packages\n", (int)(ion_buf_len(ion_reachable_syms)), (int)(ion_buf_len(ion_package_list)));
    if (!(flag_check)) {
        char (c_path[MAX_PATH]) = {0};
        if (output_name) {
            ion_path_copy(c_path, output_name);
        } else {
            snprintf(c_path, sizeof(c_path), "out_%s.%s", package_name, extension);
        }
        gen_all();
        char (*c_code) = ion_gen_buf;
        ion_gen_buf = NULL;
        if (!(ion_write_file(c_path, c_code, ion_buf_len(c_code)))) {
            printf("error: Failed to write file: %s\n", c_path);
            return 1;
        }
        printf("Generated %s\n", c_path);
    }
    return 0;
}

char const ((*ion_typedef_keyword));
char const ((*ion_enum_keyword));
char const ((*ion_struct_keyword));
char const ((*ion_union_keyword));
char const ((*ion_var_keyword));
char const ((*ion_const_keyword));
char const ((*ion_func_keyword));
char const ((*ion_sizeof_keyword));
char const ((*ion_alignof_keyword));
char const ((*ion_typeof_keyword));
char const ((*ion_offsetof_keyword));
char const ((*ion_break_keyword));
char const ((*ion_continue_keyword));
char const ((*ion_return_keyword));
char const ((*ion_if_keyword));
char const ((*ion_else_keyword));
char const ((*ion_while_keyword));
char const ((*ion_do_keyword));
char const ((*ion_for_keyword));
char const ((*ion_switch_keyword));
char const ((*ion_case_keyword));
char const ((*ion_default_keyword));
char const ((*ion_import_keyword));
char const ((*ion_goto_keyword));
char const ((*ion_first_keyword));
char const ((*ion_last_keyword));
char const ((*(*ion_keywords)));
char const ((*ion_always_name));
char const ((*ion_foreign_name));
char const ((*ion_complete_name));
char const ((*ion_assert_name));
char const ((*ion_declare_note_name));
char const ((*ion_static_assert_name));
char const ((*ion_init_keyword(char const ((*keyword))))) {
    keyword = ion_str_intern(keyword);
    ion_buf_push((void (**))(&(ion_keywords)), (void *)(&(keyword)), sizeof(keyword));
    return keyword;
}

bool ion_keywords_inited = false;
void ion_init_keywords(void) {
    if (ion_keywords_inited) {
        return;
    }
    ion_typedef_keyword = ion_init_keyword("typedef");
    char (*arena_end) = ion_intern_arena.end;
    ion_enum_keyword = ion_init_keyword("enum");
    ion_struct_keyword = ion_init_keyword("struct");
    ion_union_keyword = ion_init_keyword("union");
    ion_const_keyword = ion_init_keyword("const");
    ion_var_keyword = ion_init_keyword("var");
    ion_func_keyword = ion_init_keyword("func");
    ion_import_keyword = ion_init_keyword("import");
    ion_goto_keyword = ion_init_keyword("goto");
    ion_sizeof_keyword = ion_init_keyword("sizeof");
    ion_alignof_keyword = ion_init_keyword("alignof");
    ion_typeof_keyword = ion_init_keyword("typeof");
    ion_offsetof_keyword = ion_init_keyword("offsetof");
    ion_break_keyword = ion_init_keyword("break");
    ion_continue_keyword = ion_init_keyword("continue");
    ion_return_keyword = ion_init_keyword("return");
    ion_if_keyword = ion_init_keyword("if");
    ion_else_keyword = ion_init_keyword("else");
    ion_while_keyword = ion_init_keyword("while");
    ion_do_keyword = ion_init_keyword("do");
    ion_for_keyword = ion_init_keyword("for");
    ion_switch_keyword = ion_init_keyword("switch");
    ion_case_keyword = ion_init_keyword("case");
    ion_default_keyword = ion_init_keyword("default");
    assert((ion_intern_arena.end) == (arena_end));
    ion_first_keyword = ion_typedef_keyword;
    ion_last_keyword = ion_default_keyword;
    ion_always_name = ion_str_intern("always");
    ion_foreign_name = ion_str_intern("foreign");
    ion_complete_name = ion_str_intern("complete");
    ion_assert_name = ion_str_intern("assert");
    ion_declare_note_name = ion_str_intern("declare_note");
    ion_static_assert_name = ion_str_intern("static_assert");
    ion_keywords_inited = true;
}

bool ion_is_keyword_name(char const ((*name))) {
    return ((ion_first_keyword) <= (name)) && ((name) <= (ion_last_keyword));
}

char const ((*(ion_token_suffix_names[7]))) = {[ION_SUFFIX_NONE] = "", [ION_SUFFIX_D] = "d", [ION_SUFFIX_U] = "u", [ION_SUFFIX_L] = "l", [ION_SUFFIX_UL] = "ul", [ION_SUFFIX_LL] = "ll", [ION_SUFFIX_ULL] = "ull"};
char const ((*(ion_token_kind_names[54]))) = {[ION_TOKEN_EOF] = "EOF", [ION_TOKEN_COLON] = ":", [ION_TOKEN_LPAREN] = "(", [ION_TOKEN_RPAREN] = ")", [ION_TOKEN_LBRACE] = "{", [ION_TOKEN_RBRACE] = "}", [ION_TOKEN_LBRACKET] = "[", [ION_TOKEN_RBRACKET] = "]", [ION_TOKEN_COMMA] = ",", [ION_TOKEN_DOT] = ".", [ION_TOKEN_AT] = "@", [ION_TOKEN_POUND] = "#", [ION_TOKEN_ELLIPSIS] = "...", [ION_TOKEN_QUESTION] = "?", [ION_TOKEN_SEMICOLON] = ";", [ION_TOKEN_KEYWORD] = "keyword", [ION_TOKEN_INT] = "int", [ION_TOKEN_FLOAT] = "float", [ION_TOKEN_STR] = "string", [ION_TOKEN_NAME] = "name", [ION_TOKEN_NEG] = "~", [ION_TOKEN_NOT] = "!", [ION_TOKEN_MUL] = "*", [ION_TOKEN_DIV] = "/", [ION_TOKEN_MOD] = "%", [ION_TOKEN_AND] = "&", [ION_TOKEN_LSHIFT] = "<<", [ION_TOKEN_RSHIFT] = ">>", [ION_TOKEN_ADD] = "+", [ION_TOKEN_SUB] = "-", [ION_TOKEN_OR] = "|", [ION_TOKEN_XOR] = "^", [ION_TOKEN_EQ] = "==", [ION_TOKEN_NOTEQ] = "!=", [ION_TOKEN_LT] = "<", [ION_TOKEN_GT] = ">", [ION_TOKEN_LTEQ] = "<=", [ION_TOKEN_GTEQ] = ">=", [ION_TOKEN_AND_AND] = "&&", [ION_TOKEN_OR_OR] = "||", [ION_TOKEN_ASSIGN] = "=", [ION_TOKEN_ADD_ASSIGN] = "+=", [ION_TOKEN_SUB_ASSIGN] = "-=", [ION_TOKEN_OR_ASSIGN] = "|=", [ION_TOKEN_AND_ASSIGN] = "&=", [ION_TOKEN_XOR_ASSIGN] = "^=", [ION_TOKEN_MUL_ASSIGN] = "*=", [ION_TOKEN_DIV_ASSIGN] = "/=", [ION_TOKEN_MOD_ASSIGN] = "%=", [ION_TOKEN_LSHIFT_ASSIGN] = "<<=", [ION_TOKEN_RSHIFT_ASSIGN] = ">>=", [ION_TOKEN_INC] = "++", [ION_TOKEN_DEC] = "--", [ION_TOKEN_COLON_ASSIGN] = ":="};
char const ((*ion_token_kind_name(ion_TokenKind kind))) {
    if ((kind) < ((sizeof(ion_token_kind_names)) / (sizeof(*(ion_token_kind_names))))) {
        return ion_token_kind_names[kind];
    } else {
        return "<unknown>";
    }
}

ion_TokenKind (ion_assign_token_to_binary_token[ION_NUM_TOKEN_KINDS]) = {[ION_TOKEN_ADD_ASSIGN] = ION_TOKEN_ADD, [ION_TOKEN_SUB_ASSIGN] = ION_TOKEN_SUB, [ION_TOKEN_OR_ASSIGN] = ION_TOKEN_OR, [ION_TOKEN_AND_ASSIGN] = ION_TOKEN_AND, [ION_TOKEN_XOR_ASSIGN] = ION_TOKEN_XOR, [ION_TOKEN_LSHIFT_ASSIGN] = ION_TOKEN_LSHIFT, [ION_TOKEN_RSHIFT_ASSIGN] = ION_TOKEN_RSHIFT, [ION_TOKEN_MUL_ASSIGN] = ION_TOKEN_MUL, [ION_TOKEN_DIV_ASSIGN] = ION_TOKEN_DIV, [ION_TOKEN_MOD_ASSIGN] = ION_TOKEN_MOD};
ion_SrcPos ion_pos_builtin = {.name = "<builtin>"};
ion_Token ion_token;
char const ((*ion_stream));
char const ((*ion_line_start));
void ion_vnotice(char const ((*level)), ion_SrcPos pos, char const ((*fmt)), va_list args) {
    if ((pos.name) == (NULL)) {
        pos = ion_pos_builtin;
    }
    printf("%s(%d): %s: ", pos.name, pos.line, level);
    vprintf(fmt, args);
    printf("\n");
}

void ion_warning(ion_SrcPos pos, char const ((*fmt)), ...) {
    va_list args = {0};
    va_start_ptr(&(args), &(fmt));
    ion_vnotice("warning", pos, fmt, args);
    va_end_ptr(&(args));
}

void ion_verror(ion_SrcPos pos, char const ((*fmt)), va_list args) {
    ion_vnotice("error", pos, fmt, args);
}

void ion_error(ion_SrcPos pos, char const ((*fmt)), ...) {
    va_list args = {0};
    va_start_ptr(&(args), &(fmt));
    ion_verror(pos, fmt, args);
    va_end_ptr(&(args));
}

void ion_fatal_error(ion_SrcPos pos, char const ((*fmt)), ...) {
    va_list args = {0};
    va_start_ptr(&(args), &(fmt));
    ion_verror(pos, fmt, args);
    va_end_ptr(&(args));
    exit(1);
}

char const ((*ion_token_info(void))) {
    if (((ion_token.kind) == (ION_TOKEN_NAME)) || ((ion_token.kind) == (ION_TOKEN_KEYWORD))) {
        return ion_token.name;
    } else {
        return ion_token_kind_name(ion_token.kind);
    }
}

uint8_t (ion_char_to_digit[256]) = {['0'] = 0, ['1'] = 1, ['2'] = 2, ['3'] = 3, ['4'] = 4, ['5'] = 5, ['6'] = 6, ['7'] = 7, ['8'] = 8, ['9'] = 9, ['a'] = 10, ['A'] = 10, ['b'] = 11, ['B'] = 11, ['c'] = 12, ['C'] = 12, ['d'] = 13, ['D'] = 13, ['e'] = 14, ['E'] = 14, ['f'] = 15, ['F'] = 15};
void ion_scan_int(void) {
    int base = 10;
    char const ((*start_digits)) = ion_stream;
    if ((*(ion_stream)) == ('0')) {
        (ion_stream)++;
        if ((tolower(*(ion_stream))) == ('x')) {
            (ion_stream)++;
            ion_token.mod = ION_MOD_HEX;
            base = 16;
            start_digits = ion_stream;
        } else if ((tolower(*(ion_stream))) == ('b')) {
            (ion_stream)++;
            ion_token.mod = ION_MOD_BIN;
            base = 2;
            start_digits = ion_stream;
        } else if (isdigit(*(ion_stream))) {
            ion_token.mod = ION_MOD_OCT;
            base = 8;
            start_digits = ion_stream;
        }
    }
    ullong val = 0;
    for (;;) {
        if ((*(ion_stream)) == ('_')) {
            (ion_stream)++;
            continue;
        }
        uchar digit = ion_char_to_digit[(uchar)(*(ion_stream))];
        if (((digit) == (0)) && ((*(ion_stream)) != ('0'))) {
            break;
        }
        if ((digit) >= (base)) {
            ion_error(ion_token.pos, "Digit \'%c\' out of range for base %d", *(ion_stream), base);
            digit = 0;
        }
        if ((val) > ((((ULLONG_MAX) - (digit))) / (base))) {
            ion_error(ion_token.pos, "Integer literal overflow");
            while (isdigit(*(ion_stream))) {
                (ion_stream)++;
            }
            val = 0;
            break;
        }
        val = ((val) * (base)) + (digit);
        (ion_stream)++;
    }
    if ((ion_stream) == (start_digits)) {
        ion_error(ion_token.pos, "Expected base %d digit, got \'%c\'", base, *(ion_stream));
    }
    ion_token.kind = ION_TOKEN_INT;
    ion_token.int_val = val;
    if ((tolower(*(ion_stream))) == ('u')) {
        ion_token.suffix = ION_SUFFIX_U;
        (ion_stream)++;
        if ((tolower(*(ion_stream))) == ('l')) {
            ion_token.suffix = ION_SUFFIX_UL;
            (ion_stream)++;
            if ((tolower(*(ion_stream))) == ('l')) {
                ion_token.suffix = ION_SUFFIX_ULL;
                (ion_stream)++;
            }
        }
    } else if ((tolower(*(ion_stream))) == ('l')) {
        ion_token.suffix = ION_SUFFIX_L;
        (ion_stream)++;
        if ((tolower(*(ion_stream))) == ('l')) {
            ion_token.suffix = ION_SUFFIX_LL;
            (ion_stream)++;
        }
    }
}

void ion_scan_float(void) {
    char const ((*start)) = ion_stream;
    while (isdigit(*(ion_stream))) {
        (ion_stream)++;
    }
    if ((*(ion_stream)) == ('.')) {
        (ion_stream)++;
    }
    while (isdigit(*(ion_stream))) {
        (ion_stream)++;
    }
    if ((tolower(*(ion_stream))) == ('e')) {
        (ion_stream)++;
        if (((*(ion_stream)) == ('+')) || ((*(ion_stream)) == ('-'))) {
            (ion_stream)++;
        }
        if (!(isdigit(*(ion_stream)))) {
            ion_error(ion_token.pos, "Expected digit after float literal exponent, found \'%c\'.", *(ion_stream));
        }
        while (isdigit(*(ion_stream))) {
            (ion_stream)++;
        }
    }
    double val = strtod(start, NULL);
    if ((val) == (HUGE_VAL)) {
        ion_error(ion_token.pos, "Float literal overflow");
    }
    ion_token.kind = ION_TOKEN_FLOAT;
    ion_token.float_val = val;
    if ((tolower(*(ion_stream))) == ('d')) {
        ion_token.suffix = ION_SUFFIX_D;
        (ion_stream)++;
    }
}

char (ion_escape_to_char[256]) = {['0'] = '\0', ['\''] = '\'', ['\"'] = '\"', ['\\'] = '\\', ['n'] = '\n', ['r'] = '\r', ['t'] = '\t', ['v'] = '\v', ['b'] = '\b', ['a'] = '\a'};
int ion_scan_hex_escape(void) {
    assert((*(ion_stream)) == ('x'));
    (ion_stream)++;
    uchar val = ion_char_to_digit[(uchar)(*(ion_stream))];
    if ((!(val)) && ((*(ion_stream)) != ('0'))) {
        ion_error(ion_token.pos, "\\x needs at least 1 hex digit");
    }
    (ion_stream)++;
    uchar digit = ion_char_to_digit[(uchar)(*(ion_stream))];
    if ((digit) || ((*(ion_stream)) == ('0'))) {
        val *= 16;
        val += digit;
        if ((val) > (0xff)) {
            ion_error(ion_token.pos, "\\x argument out of range");
            val = 0xff;
        }
        (ion_stream)++;
    }
    return val;
}

void ion_scan_char(void) {
    assert((*(ion_stream)) == ('\''));
    (ion_stream)++;
    int val = 0;
    if ((*(ion_stream)) == ('\'')) {
        ion_error(ion_token.pos, "Char literal cannot be empty");
        (ion_stream)++;
    } else if ((*(ion_stream)) == ('\n')) {
        ion_error(ion_token.pos, "Char literal cannot contain newline");
    } else if ((*(ion_stream)) == ('\\')) {
        (ion_stream)++;
        if ((*(ion_stream)) == ('x')) {
            val = ion_scan_hex_escape();
        } else {
            val = ion_escape_to_char[(uchar)(*(ion_stream))];
            if (((val) == (0)) && ((*(ion_stream)) != ('0'))) {
                ion_error(ion_token.pos, "Invalid char literal escape \'\\%c\'", *(ion_stream));
            }
            (ion_stream)++;
        }
    } else {
        val = *(ion_stream);
        (ion_stream)++;
    }
    if ((*(ion_stream)) != ('\'')) {
        ion_error(ion_token.pos, "Expected closing char quote, got \'%c\'", *(ion_stream));
    } else {
        (ion_stream)++;
    }
    ion_token.kind = ION_TOKEN_INT;
    ion_token.int_val = val;
    ion_token.mod = ION_MOD_CHAR;
}

void ion_scan_str(void) {
    assert((*(ion_stream)) == ('\"'));
    (ion_stream)++;
    char (*str) = NULL;
    if (((ion_stream[0]) == ('\"')) && ((ion_stream[1]) == ('\"'))) {
        ion_stream += 2;
        while (*(ion_stream)) {
            if ((((ion_stream[0]) == ('\"')) && ((ion_stream[1]) == ('\"'))) && ((ion_stream[2]) == ('\"'))) {
                ion_stream += 3;
                break;
            }
            if ((*(ion_stream)) != ('\r')) {
                ion_buf_push((void (**))(&(str)), (void *)(ion_stream), 1);
            }
            if ((*(ion_stream)) == ('\n')) {
                (ion_token.pos.line)++;
            }
            (ion_stream)++;
        }
        if (!(*(ion_stream))) {
            ion_error(ion_token.pos, "Unexpected end of file within multi-line string literal");
        }
        ion_token.mod = ION_MOD_MULTILINE;
    } else {
        while ((*(ion_stream)) && ((*(ion_stream)) != ('\"'))) {
            char val = *(ion_stream);
            if ((val) == ('\n')) {
                ion_error(ion_token.pos, "String literal cannot contain newline");
                break;
            } else if ((val) == ('\\')) {
                (ion_stream)++;
                if ((*(ion_stream)) == ('x')) {
                    val = ion_scan_hex_escape();
                } else {
                    val = ion_escape_to_char[(uchar)(*(ion_stream))];
                    if (((val) == (0)) && ((*(ion_stream)) != ('0'))) {
                        ion_error(ion_token.pos, "Invalid string literal escape \'\\%c\'", *(ion_stream));
                    }
                    (ion_stream)++;
                }
            } else {
                (ion_stream)++;
            }
            ion_buf_push((void (**))(&(str)), &(val), 1);
        }
        if (*(ion_stream)) {
            (ion_stream)++;
        } else {
            ion_error(ion_token.pos, "Unexpected end of file within string literal");
        }
    }
    int nul = '\0';
    ion_buf_push((void (**))(&(str)), &(nul), 1);
    ion_token.kind = ION_TOKEN_STR;
    ion_token.str_val = str;
}

void ion_next_token(void) {
    repeat: ;
    ion_token.start = ion_stream;
    ion_token.mod = 0;
    ion_token.suffix = 0;
    switch (*(ion_stream)) {
    case ' ':
    case '\n':
    case '\r':
    case '\t':
    case '\v': {
        while (isspace(*(ion_stream))) {
            if ((*((ion_stream)++)) == ('\n')) {
                ion_line_start = ion_stream;
                (ion_token.pos.line)++;
            }
        }
        goto repeat;
        break;
    }
    case '\'': {
        ion_scan_char();
        break;
    }
    case '\"': {
        ion_scan_str();
        break;
    }
    case '.': {
        if (isdigit(ion_stream[1])) {
            ion_scan_float();
        } else if (((ion_stream[1]) == ('.')) && ((ion_stream[2]) == ('.'))) {
            ion_token.kind = ION_TOKEN_ELLIPSIS;
            ion_stream += 3;
        } else {
            ion_token.kind = ION_TOKEN_DOT;
            (ion_stream)++;
        }
        break;
    }
    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9': {
        {
            while (isdigit(*(ion_stream))) {
                (ion_stream)++;
            }
            char c = *(ion_stream);
            ion_stream = ion_token.start;
            if (((c) == ('.')) || ((tolower(c)) == ('e'))) {
                ion_scan_float();
            } else {
                ion_scan_int();
            }
        }
        break;
    }
    case 'a':
    case 'b':
    case 'c':
    case 'd':
    case 'e':
    case 'f':
    case 'g':
    case 'h':
    case 'i':
    case 'j':
    case 'k':
    case 'l':
    case 'm':
    case 'n':
    case 'o':
    case 'p':
    case 'q':
    case 'r':
    case 's':
    case 't':
    case 'u':
    case 'v':
    case 'w':
    case 'x':
    case 'y':
    case 'z':
    case 'A':
    case 'B':
    case 'C':
    case 'D':
    case 'E':
    case 'F':
    case 'G':
    case 'H':
    case 'I':
    case 'J':
    case 'K':
    case 'L':
    case 'M':
    case 'N':
    case 'O':
    case 'P':
    case 'Q':
    case 'R':
    case 'S':
    case 'T':
    case 'U':
    case 'V':
    case 'W':
    case 'X':
    case 'Y':
    case 'Z':
    case '_': {
        while ((isalnum(*(ion_stream))) || ((*(ion_stream)) == ('_'))) {
            (ion_stream)++;
        }
        ion_token.name = ion_str_intern_range(ion_token.start, ion_stream);
        ion_token.kind = (ion_is_keyword_name(ion_token.name) ? ION_TOKEN_KEYWORD : ION_TOKEN_NAME);
        break;
    }
    case '<': {
        ion_token.kind = ION_TOKEN_LT;
        (ion_stream)++;
        if ((*(ion_stream)) == ('<')) {
            ion_token.kind = ION_TOKEN_LSHIFT;
            (ion_stream)++;
            if ((*(ion_stream)) == ('=')) {
                ion_token.kind = ION_TOKEN_LSHIFT_ASSIGN;
                (ion_stream)++;
            }
        } else if ((*(ion_stream)) == ('=')) {
            ion_token.kind = ION_TOKEN_LTEQ;
            (ion_stream)++;
        }
        break;
    }
    case '>': {
        ion_token.kind = ION_TOKEN_GT;
        (ion_stream)++;
        if ((*(ion_stream)) == ('>')) {
            ion_token.kind = ION_TOKEN_RSHIFT;
            (ion_stream)++;
            if ((*(ion_stream)) == ('=')) {
                ion_token.kind = ION_TOKEN_RSHIFT_ASSIGN;
                (ion_stream)++;
            }
        } else if ((*(ion_stream)) == ('=')) {
            ion_token.kind = ION_TOKEN_GTEQ;
            (ion_stream)++;
        }
        break;
    }
    case '/': {
        ion_token.kind = ION_TOKEN_DIV;
        (ion_stream)++;
        if ((*(ion_stream)) == ('=')) {
            ion_token.kind = ION_TOKEN_DIV_ASSIGN;
            (ion_stream)++;
        } else if ((*(ion_stream)) == ('/')) {
            (ion_stream)++;
            while ((*(ion_stream)) && ((*(ion_stream)) != ('\n'))) {
                (ion_stream)++;
            }
            goto repeat;
        } else if ((*(ion_stream)) == ('*')) {
            (ion_stream)++;
            int level = 1;
            while ((*(ion_stream)) && ((level) > (0))) {
                if (((ion_stream[0]) == ('/')) && ((ion_stream[1]) == ('*'))) {
                    (level)++;
                    ion_stream += 2;
                } else if (((ion_stream[0]) == ('*')) && ((ion_stream[1]) == ('/'))) {
                    (level)--;
                    ion_stream += 2;
                } else {
                    if ((*(ion_stream)) == ('\n')) {
                        (ion_token.pos.line)++;
                    }
                    (ion_stream)++;
                }
            }
            goto repeat;
        }
        break;
    }
    case '\0': {
        ion_token.kind = ION_TOKEN_EOF;
        (ion_stream)++;
        break;
    }
    case '(': {
        ion_token.kind = ION_TOKEN_LPAREN;
        (ion_stream)++;
        break;
    }
    case ')': {
        ion_token.kind = ION_TOKEN_RPAREN;
        (ion_stream)++;
        break;
    }
    case '{': {
        ion_token.kind = ION_TOKEN_LBRACE;
        (ion_stream)++;
        break;
    }
    case '}': {
        ion_token.kind = ION_TOKEN_RBRACE;
        (ion_stream)++;
        break;
    }
    case '[': {
        ion_token.kind = ION_TOKEN_LBRACKET;
        (ion_stream)++;
        break;
    }
    case ']': {
        ion_token.kind = ION_TOKEN_RBRACKET;
        (ion_stream)++;
        break;
    }
    case ',': {
        ion_token.kind = ION_TOKEN_COMMA;
        (ion_stream)++;
        break;
    }
    case '@': {
        ion_token.kind = ION_TOKEN_AT;
        (ion_stream)++;
        break;
    }
    case '#': {
        ion_token.kind = ION_TOKEN_POUND;
        (ion_stream)++;
        break;
    }
    case '?': {
        ion_token.kind = ION_TOKEN_QUESTION;
        (ion_stream)++;
        break;
    }
    case ';': {
        ion_token.kind = ION_TOKEN_SEMICOLON;
        (ion_stream)++;
        break;
    }
    case '~': {
        ion_token.kind = ION_TOKEN_NEG;
        (ion_stream)++;
        break;
    }
    case '!': {
        ion_token.kind = ION_TOKEN_NOT;
        (ion_stream)++;
        if ((*(ion_stream)) == ('=')) {
            ion_token.kind = ION_TOKEN_NOTEQ;
            (ion_stream)++;
        }
        break;
    }
    case ':': {
        ion_token.kind = ION_TOKEN_COLON;
        (ion_stream)++;
        if ((*(ion_stream)) == ('=')) {
            ion_token.kind = ION_TOKEN_COLON_ASSIGN;
            (ion_stream)++;
        }
        break;
    }
    case '=': {
        ion_token.kind = ION_TOKEN_ASSIGN;
        (ion_stream)++;
        if ((*(ion_stream)) == ('=')) {
            ion_token.kind = ION_TOKEN_EQ;
            (ion_stream)++;
        }
        break;
    }
    case '^': {
        ion_token.kind = ION_TOKEN_XOR;
        (ion_stream)++;
        if ((*(ion_stream)) == ('=')) {
            ion_token.kind = ION_TOKEN_XOR_ASSIGN;
            (ion_stream)++;
        }
        break;
    }
    case '*': {
        ion_token.kind = ION_TOKEN_MUL;
        (ion_stream)++;
        if ((*(ion_stream)) == ('=')) {
            ion_token.kind = ION_TOKEN_MUL_ASSIGN;
            (ion_stream)++;
        }
        break;
    }
    case '%': {
        ion_token.kind = ION_TOKEN_MOD;
        (ion_stream)++;
        if ((*(ion_stream)) == ('=')) {
            ion_token.kind = ION_TOKEN_MOD_ASSIGN;
            (ion_stream)++;
        }
        break;
    }
    case '+': {
        ion_token.kind = ION_TOKEN_ADD;
        (ion_stream)++;
        if ((*(ion_stream)) == ('=')) {
            ion_token.kind = ION_TOKEN_ADD_ASSIGN;
            (ion_stream)++;
        } else if ((*(ion_stream)) == ('+')) {
            ion_token.kind = ION_TOKEN_INC;
            (ion_stream)++;
        }
        break;
    }
    case '-': {
        ion_token.kind = ION_TOKEN_SUB;
        (ion_stream)++;
        if ((*(ion_stream)) == ('=')) {
            ion_token.kind = ION_TOKEN_SUB_ASSIGN;
            (ion_stream)++;
        } else if ((*(ion_stream)) == ('-')) {
            ion_token.kind = ION_TOKEN_DEC;
            (ion_stream)++;
        }
        break;
    }
    case '&': {
        ion_token.kind = ION_TOKEN_AND;
        (ion_stream)++;
        if ((*(ion_stream)) == ('=')) {
            ion_token.kind = ION_TOKEN_AND_ASSIGN;
            (ion_stream)++;
        } else if ((*(ion_stream)) == ('&')) {
            ion_token.kind = ION_TOKEN_AND_AND;
            (ion_stream)++;
        }
        break;
    }
    case '|': {
        ion_token.kind = ION_TOKEN_OR;
        (ion_stream)++;
        if ((*(ion_stream)) == ('=')) {
            ion_token.kind = ION_TOKEN_OR_ASSIGN;
            (ion_stream)++;
        } else if ((*(ion_stream)) == ('|')) {
            ion_token.kind = ION_TOKEN_OR_OR;
            (ion_stream)++;
        }
        break;
    }
    default: {
        ion_error(ion_token.pos, "Invalid \'%c\' token, skipping", *(ion_stream));
        (ion_stream)++;
        goto repeat;
        break;
    }
    }
    ion_token.end = ion_stream;
}

void ion_init_stream(char const ((*name)), char const ((*buf))) {
    ion_stream = buf;
    ion_line_start = ion_stream;
    ion_token.pos.name = (name ? name : (char const (*))("<string>"));
    ion_token.pos.line = 1;
    ion_next_token();
}

bool ion_is_token(ion_TokenKind kind) {
    return (ion_token.kind) == (kind);
}

bool ion_is_token_eof(void) {
    return (ion_token.kind) == (ION_TOKEN_EOF);
}

bool ion_is_token_name(char const ((*name))) {
    return ((ion_token.kind) == (ION_TOKEN_NAME)) && ((ion_token.name) == (name));
}

bool ion_is_keyword(char const ((*name))) {
    return (ion_is_token(ION_TOKEN_KEYWORD)) && ((ion_token.name) == (name));
}

bool ion_match_keyword(char const ((*name))) {
    if (ion_is_keyword(name)) {
        ion_next_token();
        return true;
    } else {
        return false;
    }
}

bool ion_match_token(ion_TokenKind kind) {
    if (ion_is_token(kind)) {
        ion_next_token();
        return true;
    } else {
        return false;
    }
}

bool ion_expect_token(ion_TokenKind kind) {
    if (ion_is_token(kind)) {
        ion_next_token();
        return true;
    } else {
        ion_fatal_error(ion_token.pos, "Expected token %s, got %s", ion_token_kind_name(kind), ion_token_info());
        return false;
    }
}

bool ion_flag_verbose;
bool ion_flag_lazy;
bool ion_flag_nosourcemap;
bool ion_flag_notypeinfo;
bool ion_flag_fullgen;
void ion_path_normalize(char (*path)) {
    char (*ptr) = {0};
    for (ptr = path; *(ptr); (ptr)++) {
        if ((*(ptr)) == ('\\')) {
            *(ptr) = '/';
        }
    }
    if (((ptr) != (path)) && ((ptr[-(1)]) == ('/'))) {
        ptr[-(1)] = 0;
    }
}

void ion_path_copy(char (path[MAX_PATH]), char const ((*src))) {
    strncpy(path, src, MAX_PATH);
    path[(MAX_PATH) - (1)] = 0;
    ion_path_normalize(path);
}

void ion_path_join(char (path[MAX_PATH]), char const ((*src))) {
    char (*ptr) = (path) + (strlen(path));
    if (((ptr) != (path)) && ((ptr[-(1)]) == ('/'))) {
        (ptr)--;
    }
    if ((*(src)) == ('/')) {
        (src)++;
    }
    snprintf(ptr, ((path) + (MAX_PATH)) - (ptr), "/%s", src);
}

char (*ion_path_file(char (path[MAX_PATH]))) {
    ion_path_normalize(path);
    for (char (*ptr) = (path) + (strlen(path)); (ptr) != (path); (ptr)--) {
        if ((ptr[-(1)]) == ('/')) {
            return ptr;
        }
    }
    return path;
}

char (*ion_path_ext(char (path[MAX_PATH]))) {
    for (char (*ptr) = (path) + (strlen(path)); (ptr) != (path); (ptr)--) {
        if ((ptr[-(1)]) == ('.')) {
            return ptr;
        }
    }
    return path;
}

bool ion_dir_excluded(ion_DirListIter (*iter)) {
    return (iter->valid) && ((((strcmp(iter->name, ".")) == (0)) || ((strcmp(iter->name, "..")) == (0))));
}

bool ion_dir_list_subdir(ion_DirListIter (*iter)) {
    if ((!(iter->valid)) || (!(iter->is_dir))) {
        return false;
    }
    ion_DirListIter subdir_iter = {0};
    ion_path_join(iter->base, iter->name);
    ion_dir_list(&(subdir_iter), iter->base);
    ion_dir_list_free(iter);
    *(iter) = subdir_iter;
    return true;
}

char const ((*(*ion_dir_list_buf(char const ((*filespec)))))) {
    char const ((*(*buf))) = NULL;
    ion_DirListIter iter = {0};
    for (ion_dir_list(&(iter), filespec); iter.valid; ion_dir_list_next(&(iter))) {
        char const ((*name)) = strdup(iter.name);
        ion_buf_push((void (**))(&(buf)), &(name), sizeof(name));
    }
    return buf;
}

ion_FlagDef (*ion_flag_defs);
void ion_add_flag_bool(char const ((*name)), bool (*ptr), char const ((*help))) {
    ion_FlagDef def = {.kind = ION_FLAG_BOOL, .name = name, .help = help, .ptr = {.b = ptr}};
    ion_buf_push((void (**))(&(ion_flag_defs)), &(def), sizeof(def));
}

void ion_add_flag_str(char const ((*name)), char const ((*(*ptr))), char const ((*arg_name)), char const ((*help))) {
    ion_FlagDef def = {.kind = ION_FLAG_STR, .name = name, .help = help, .arg_name = arg_name, .ptr = {.s = ptr}};
    ion_buf_push((void (**))(&(ion_flag_defs)), &(def), sizeof(def));
}

void ion_add_flag_enum(char const ((*name)), int (*ptr), char const ((*help)), char const ((*(*options))), int num_options) {
    ion_FlagDef def = {.kind = ION_FLAG_ENUM, .name = name, .help = help, .ptr = {.i = ptr}, .options = options, .num_options = num_options};
    ion_buf_push((void (**))(&(ion_flag_defs)), &(def), sizeof(def));
}

ion_FlagDef (*ion_get_flag_def(char const ((*name)))) {
    for (size_t i = 0; (i) < (ion_buf_len(ion_flag_defs)); (i)++) {
        if ((strcmp(ion_flag_defs[i].name, name)) == (0)) {
            return &(ion_flag_defs[i]);
        }
    }
    return NULL;
}

void ion_print_flags_usage(void) {
    printf("Flags:\n");
    for (size_t i = 0; (i) < (ion_buf_len(ion_flag_defs)); (i)++) {
        ion_FlagDef flag = ion_flag_defs[i];
        char (note[256]) = {0};
        char (format[256]) = {0};
        switch (flag.kind) {
        case ION_FLAG_STR: {
            snprintf(format, sizeof(format), "%s <%s>", flag.name, (flag.arg_name ? flag.arg_name : (char const (*))("value")));
            if (*(flag.ptr.s)) {
                snprintf(note, sizeof(note), "(default: %s)", *(flag.ptr.s));
            }
            break;
        }
        case ION_FLAG_ENUM: {
            {
                char (*end) = (format) + (sizeof(format));
                char (*ptr) = format;
                ptr += snprintf(ptr, (end) - (ptr), "%s <", flag.name);
                for (int k = 0; (k) < (flag.num_options); (k)++) {
                    ptr += snprintf(ptr, (end) - (ptr), "%s%s", ((k) == (0) ? "" : "|"), flag.options[k]);
                    if ((k) == (*(flag.ptr.i))) {
                        snprintf(note, sizeof(note), " (default: %s)", flag.options[k]);
                    }
                }
                snprintf(ptr, (end) - (ptr), ">");
            }
            break;
        }
        case ION_FLAG_BOOL:
        default: {
            snprintf(format, sizeof(format), "%s", flag.name);
            break;
        }
        }
        printf(" -%-32s %s%s\n", format, (flag.help ? flag.help : (char const (*))("")), note);
    }
}

char const ((*ion_parse_flags(int (*argc_ptr), char const ((*(*(*argv_ptr))))))) {
    int argc = *(argc_ptr);
    char const ((*(*argv))) = *(argv_ptr);
    int i = {0};
    for (i = 1; (i) < (argc); (i)++) {
        char const ((*arg)) = argv[i];
        char const ((*name)) = arg;
        if ((*(name)) == ('-')) {
            (name)++;
            if ((*(name)) == ('-')) {
                (name)++;
            }
            ion_FlagDef (*flag) = ion_get_flag_def(name);
            if (!(flag)) {
                printf("Unknown flag %s\n", arg);
                continue;
            }
            switch (flag->kind) {
            case ION_FLAG_BOOL: {
                *(flag->ptr.b) = true;
                break;
            }
            case ION_FLAG_STR: {
                if (((i) + (1)) < (argc)) {
                    (i)++;
                    *(flag->ptr.s) = argv[i];
                } else {
                    printf("No value argument after -%s\n", arg);
                }
                break;
            }
            case ION_FLAG_ENUM: {
                {
                    char const ((*option)) = {0};
                    if (((i) + (1)) < (argc)) {
                        (i)++;
                        option = argv[i];
                    } else {
                        printf("No value after %s\n", arg);
                        break;
                    }
                    bool found = false;
                    for (int k = 0; (k) < (flag->num_options); (k)++) {
                        if ((strcmp(flag->options[k], option)) == (0)) {
                            *(flag->ptr.i) = k;
                            found = true;
                            break;
                        }
                    }
                    if (!(found)) {
                        printf("Invalid value \'%s\' for %s\n", option, arg);
                    }
                }
                break;
            }
            default: {
                printf("Unhandled flag kind\n");
                break;
            }
            }
        } else {
            break;
        }
    }
    *(argc_ptr) = (argc) - (i);
    *(argv_ptr) = (argv) + (i);
    return ion_path_file(strdup(argv[0]));
}

void ion_path_absolute(char (path[MAX_PATH])) {
    char (rel_path[MAX_PATH]) = {0};
    ion_path_copy(rel_path, path);
    realpath(rel_path, path);
}

void ion_dir_list_free(ion_DirListIter (*iter)) {
    if (iter->valid) {
        iter->valid = false;
        iter->error = false;
        closedir(iter->handle);
    }
}

void ion_dir_list_next(ion_DirListIter (*iter)) {
    if (!(iter->valid)) {
        return;
    }
    do {
        dirent (*entry) = readdir(iter->handle);
        if (!(entry)) {
            ion_dir_list_free(iter);
            return;
        }
        ion_path_copy(iter->name, entry->d_name);
        iter->is_dir = (entry->d_type) & (DT_DIR);
    } while (ion_dir_excluded(iter));
}

void ion_dir_list(ion_DirListIter (*iter), char const ((*path))) {
    memset(iter, 0, sizeof(*(iter)));
    void (*dir) = opendir(path);
    if (!(dir)) {
        iter->valid = false;
        iter->error = true;
        return;
    }
    iter->handle = dir;
    ion_path_copy(iter->base, path);
    iter->valid = true;
    ion_dir_list_next(iter);
}

ion_Typespec (*ion_parse_type_func_param(void)) {
    ion_Typespec (*type) = ion_parse_type();
    if (ion_match_token(ION_TOKEN_COLON)) {
        if ((type->kind) != (ION_TYPESPEC_NAME)) {
            ion_error(ion_token.pos, "Colons in parameters of func types must be preceded by names.");
        }
        type = ion_parse_type();
    }
    return type;
}

ion_Typespec (*ion_parse_type_func(void)) {
    ion_SrcPos pos = ion_token.pos;
    ion_Typespec (*(*args)) = NULL;
    bool has_varargs = false;
    ion_expect_token(ION_TOKEN_LPAREN);
    while (!(ion_is_token(ION_TOKEN_RPAREN))) {
        if (ion_match_token(ION_TOKEN_ELLIPSIS)) {
            if (has_varargs) {
                ion_error(ion_token.pos, "Multiple ellipsis instances in function type");
            }
            has_varargs = true;
        } else {
            if (has_varargs) {
                ion_error(ion_token.pos, "Ellipsis must be last parameter in function type");
            }
            ion_Typespec (*param) = ion_parse_type_func_param();
            ion_buf_push((void (**))(&(args)), &(param), sizeof(param));
        }
        if (!(ion_match_token(ION_TOKEN_COMMA))) {
            break;
        }
    }
    ion_expect_token(ION_TOKEN_RPAREN);
    ion_Typespec (*ret) = NULL;
    if (ion_match_token(ION_TOKEN_COLON)) {
        ret = ion_parse_type();
    }
    return ion_new_typespec_func(pos, args, ion_buf_len(args), ret, has_varargs);
}

ion_Typespec (*ion_parse_type_base(void)) {
    if (ion_is_token(ION_TOKEN_NAME)) {
        ion_SrcPos pos = ion_token.pos;
        char const ((*name)) = ion_token.name;
        ion_next_token();
        return ion_new_typespec_name(pos, name);
    } else if (ion_match_keyword(ion_func_keyword)) {
        return ion_parse_type_func();
    } else if (ion_match_token(ION_TOKEN_LPAREN)) {
        ion_Typespec (*type) = ion_parse_type();
        ion_expect_token(ION_TOKEN_RPAREN);
        return type;
    } else {
        ion_fatal_error(ion_token.pos, "Unexpected token %s in type", ion_token_info());
        return NULL;
    }
}

ion_Typespec (*ion_parse_type(void)) {
    ion_Typespec (*type) = ion_parse_type_base();
    ion_SrcPos pos = ion_token.pos;
    while (((ion_is_token(ION_TOKEN_LBRACKET)) || (ion_is_token(ION_TOKEN_MUL))) || (ion_is_keyword(ion_const_keyword))) {
        if (ion_match_token(ION_TOKEN_LBRACKET)) {
            ion_Expr (*size) = NULL;
            if (!(ion_is_token(ION_TOKEN_RBRACKET))) {
                size = ion_parse_expr();
            }
            ion_expect_token(ION_TOKEN_RBRACKET);
            type = ion_new_typespec_array(pos, type, size);
        } else if (ion_match_keyword(ion_const_keyword)) {
            type = ion_new_typespec_const(pos, type);
        } else {
            assert(ion_is_token(ION_TOKEN_MUL));
            ion_next_token();
            type = ion_new_typespec_ptr(pos, type);
        }
    }
    return type;
}

ion_CompoundField ion_parse_expr_compound_field(void) {
    ion_SrcPos pos = ion_token.pos;
    if (ion_match_token(ION_TOKEN_LBRACKET)) {
        ion_Expr (*index) = ion_parse_expr();
        ion_expect_token(ION_TOKEN_RBRACKET);
        ion_expect_token(ION_TOKEN_ASSIGN);
        return (ion_CompoundField){ION_FIELD_INDEX, pos, ion_parse_expr(), .index = index};
    } else {
        ion_Expr (*expr) = ion_parse_expr();
        if (ion_match_token(ION_TOKEN_ASSIGN)) {
            if ((expr->kind) != (ION_EXPR_NAME)) {
                ion_fatal_error(ion_token.pos, "Named initializer in compound literal must be preceded by field name");
            }
            return (ion_CompoundField){ION_FIELD_NAME, pos, ion_parse_expr(), .name = expr->name};
        } else {
            return (ion_CompoundField){ION_FIELD_DEFAULT, pos, expr};
        }
    }
}

ion_Expr (*ion_parse_expr_compound(ion_Typespec (*type))) {
    ion_SrcPos pos = ion_token.pos;
    ion_expect_token(ION_TOKEN_LBRACE);
    ion_CompoundField (*fields) = NULL;
    while (!(ion_is_token(ION_TOKEN_RBRACE))) {
        ion_CompoundField field = ion_parse_expr_compound_field();
        ion_buf_push((void (**))(&(fields)), &(field), sizeof(field));
        if (!(ion_match_token(ION_TOKEN_COMMA))) {
            break;
        }
    }
    ion_expect_token(ION_TOKEN_RBRACE);
    return ion_new_expr_compound(pos, type, fields, ion_buf_len(fields));
}

ion_Expr (*ion_parse_expr_operand(void)) {
    ion_SrcPos pos = ion_token.pos;
    if (ion_is_token(ION_TOKEN_INT)) {
        ullong val = ion_token.int_val;
        ion_TokenMod mod = ion_token.mod;
        ion_TokenSuffix suffix = ion_token.suffix;
        ion_next_token();
        return ion_new_expr_int(pos, val, mod, suffix);
    } else if (ion_is_token(ION_TOKEN_FLOAT)) {
        char const ((*start)) = ion_token.start;
        char const ((*end)) = ion_token.end;
        double val = ion_token.float_val;
        ion_TokenSuffix suffix = ion_token.suffix;
        ion_next_token();
        return ion_new_expr_float(pos, start, end, val, suffix);
    } else if (ion_is_token(ION_TOKEN_STR)) {
        char const ((*val)) = ion_token.str_val;
        ion_TokenMod mod = ion_token.mod;
        ion_next_token();
        return ion_new_expr_str(pos, val, mod);
    } else if (ion_is_token(ION_TOKEN_NAME)) {
        char const ((*name)) = ion_token.name;
        ion_next_token();
        if (ion_is_token(ION_TOKEN_LBRACE)) {
            return ion_parse_expr_compound(ion_new_typespec_name(pos, name));
        } else {
            return ion_new_expr_name(pos, name);
        }
    } else if (ion_match_keyword(ion_sizeof_keyword)) {
        ion_expect_token(ION_TOKEN_LPAREN);
        if (ion_match_token(ION_TOKEN_COLON)) {
            ion_Typespec (*type) = ion_parse_type();
            ion_expect_token(ION_TOKEN_RPAREN);
            return ion_new_expr_sizeof_type(pos, type);
        } else {
            ion_Expr (*expr) = ion_parse_expr();
            ion_expect_token(ION_TOKEN_RPAREN);
            return ion_new_expr_sizeof_expr(pos, expr);
        }
    } else if (ion_match_keyword(ion_alignof_keyword)) {
        ion_expect_token(ION_TOKEN_LPAREN);
        if (ion_match_token(ION_TOKEN_COLON)) {
            ion_Typespec (*type) = ion_parse_type();
            ion_expect_token(ION_TOKEN_RPAREN);
            return ion_new_expr_alignof_type(pos, type);
        } else {
            ion_Expr (*expr) = ion_parse_expr();
            ion_expect_token(ION_TOKEN_RPAREN);
            return ion_new_expr_alignof_expr(pos, expr);
        }
    } else if (ion_match_keyword(ion_typeof_keyword)) {
        ion_expect_token(ION_TOKEN_LPAREN);
        if (ion_match_token(ION_TOKEN_COLON)) {
            ion_Typespec (*type) = ion_parse_type();
            ion_expect_token(ION_TOKEN_RPAREN);
            return ion_new_expr_typeof_type(pos, type);
        } else {
            ion_Expr (*expr) = ion_parse_expr();
            ion_expect_token(ION_TOKEN_RPAREN);
            return ion_new_expr_typeof_expr(pos, expr);
        }
    } else if (ion_match_keyword(ion_offsetof_keyword)) {
        ion_expect_token(ION_TOKEN_LPAREN);
        ion_Typespec (*type) = ion_parse_type();
        ion_expect_token(ION_TOKEN_COMMA);
        char const ((*name)) = ion_parse_name();
        ion_expect_token(ION_TOKEN_RPAREN);
        return ion_new_expr_offsetof(pos, type, name);
    } else if (ion_is_token(ION_TOKEN_LBRACE)) {
        return ion_parse_expr_compound(NULL);
    } else if (ion_match_token(ION_TOKEN_LPAREN)) {
        if (ion_match_token(ION_TOKEN_COLON)) {
            ion_Typespec (*type) = ion_parse_type();
            ion_expect_token(ION_TOKEN_RPAREN);
            if (ion_is_token(ION_TOKEN_LBRACE)) {
                return ion_parse_expr_compound(type);
            } else {
                return ion_new_expr_cast(pos, type, ion_parse_expr_unary());
            }
        } else {
            ion_Expr (*expr) = ion_parse_expr();
            ion_expect_token(ION_TOKEN_RPAREN);
            return ion_new_expr_paren(pos, expr);
        }
    } else {
        ion_fatal_error(ion_token.pos, "Unexpected token %s in expression", ion_token_info());
        return NULL;
    }
}

ion_Expr (*ion_parse_expr_base(void)) {
    ion_Expr (*expr) = ion_parse_expr_operand();
    while (((((ion_is_token(ION_TOKEN_LPAREN)) || (ion_is_token(ION_TOKEN_LBRACKET))) || (ion_is_token(ION_TOKEN_DOT))) || (ion_is_token(ION_TOKEN_INC))) || (ion_is_token(ION_TOKEN_DEC))) {
        ion_SrcPos pos = ion_token.pos;
        if (ion_match_token(ION_TOKEN_LPAREN)) {
            ion_Expr (*(*args)) = NULL;
            while (!(ion_is_token(ION_TOKEN_RPAREN))) {
                ion_Expr (*arg) = ion_parse_expr();
                ion_buf_push((void (**))(&(args)), &(arg), sizeof(arg));
                if (!(ion_match_token(ION_TOKEN_COMMA))) {
                    break;
                }
            }
            ion_expect_token(ION_TOKEN_RPAREN);
            expr = ion_new_expr_call(pos, expr, args, ion_buf_len(args));
        } else if (ion_match_token(ION_TOKEN_LBRACKET)) {
            ion_Expr (*index) = ion_parse_expr();
            ion_expect_token(ION_TOKEN_RBRACKET);
            expr = ion_new_expr_index(pos, expr, index);
        } else if (ion_is_token(ION_TOKEN_DOT)) {
            ion_next_token();
            char const ((*field)) = ion_token.name;
            ion_expect_token(ION_TOKEN_NAME);
            expr = ion_new_expr_field(pos, expr, field);
        } else {
            assert((ion_is_token(ION_TOKEN_INC)) || (ion_is_token(ION_TOKEN_DEC)));
            ion_TokenKind op = ion_token.kind;
            ion_next_token();
            expr = ion_new_expr_modify(pos, op, true, expr);
        }
    }
    return expr;
}

bool ion_is_unary_op(void) {
    return (((((((ion_is_token(ION_TOKEN_ADD)) || (ion_is_token(ION_TOKEN_SUB))) || (ion_is_token(ION_TOKEN_MUL))) || (ion_is_token(ION_TOKEN_AND))) || (ion_is_token(ION_TOKEN_NEG))) || (ion_is_token(ION_TOKEN_NOT))) || (ion_is_token(ION_TOKEN_INC))) || (ion_is_token(ION_TOKEN_DEC));
}

ion_Expr (*ion_parse_expr_unary(void)) {
    if (ion_is_unary_op()) {
        ion_SrcPos pos = ion_token.pos;
        ion_TokenKind op = ion_token.kind;
        ion_next_token();
        if (((op) == (ION_TOKEN_INC)) || ((op) == (ION_TOKEN_DEC))) {
            return ion_new_expr_modify(pos, op, false, ion_parse_expr_unary());
        } else {
            return ion_new_expr_unary(pos, op, ion_parse_expr_unary());
        }
    } else {
        return ion_parse_expr_base();
    }
}

bool ion_is_mul_op(void) {
    return ((ION_TOKEN_FIRST_MUL) <= (ion_token.kind)) && ((ion_token.kind) <= (ION_TOKEN_LAST_MUL));
}

ion_Expr (*ion_parse_expr_mul(void)) {
    ion_Expr (*expr) = ion_parse_expr_unary();
    while (ion_is_mul_op()) {
        ion_SrcPos pos = ion_token.pos;
        ion_TokenKind op = ion_token.kind;
        ion_next_token();
        expr = ion_new_expr_binary(pos, op, expr, ion_parse_expr_unary());
    }
    return expr;
}

bool ion_is_add_op(void) {
    return ((ION_TOKEN_FIRST_ADD) <= (ion_token.kind)) && ((ion_token.kind) <= (ION_TOKEN_LAST_ADD));
}

ion_Expr (*ion_parse_expr_add(void)) {
    ion_Expr (*expr) = ion_parse_expr_mul();
    while (ion_is_add_op()) {
        ion_SrcPos pos = ion_token.pos;
        ion_TokenKind op = ion_token.kind;
        ion_next_token();
        expr = ion_new_expr_binary(pos, op, expr, ion_parse_expr_mul());
    }
    return expr;
}

bool ion_is_cmp_op(void) {
    return ((ION_TOKEN_FIRST_CMP) <= (ion_token.kind)) && ((ion_token.kind) <= (ION_TOKEN_LAST_CMP));
}

ion_Expr (*ion_parse_expr_cmp(void)) {
    ion_Expr (*expr) = ion_parse_expr_add();
    while (ion_is_cmp_op()) {
        ion_SrcPos pos = ion_token.pos;
        ion_TokenKind op = ion_token.kind;
        ion_next_token();
        expr = ion_new_expr_binary(pos, op, expr, ion_parse_expr_add());
    }
    return expr;
}

ion_Expr (*ion_parse_expr_and(void)) {
    ion_Expr (*expr) = ion_parse_expr_cmp();
    while (ion_match_token(ION_TOKEN_AND_AND)) {
        ion_SrcPos pos = ion_token.pos;
        expr = ion_new_expr_binary(pos, ION_TOKEN_AND_AND, expr, ion_parse_expr_cmp());
    }
    return expr;
}

ion_Expr (*ion_parse_expr_or(void)) {
    ion_Expr (*expr) = ion_parse_expr_and();
    while (ion_match_token(ION_TOKEN_OR_OR)) {
        ion_SrcPos pos = ion_token.pos;
        expr = ion_new_expr_binary(pos, ION_TOKEN_OR_OR, expr, ion_parse_expr_and());
    }
    return expr;
}

ion_Expr (*ion_parse_expr_ternary(void)) {
    ion_SrcPos pos = ion_token.pos;
    ion_Expr (*expr) = ion_parse_expr_or();
    if (ion_match_token(ION_TOKEN_QUESTION)) {
        ion_Expr (*then_expr) = ion_parse_expr_ternary();
        ion_expect_token(ION_TOKEN_COLON);
        ion_Expr (*else_expr) = ion_parse_expr_ternary();
        expr = ion_new_expr_ternary(pos, expr, then_expr, else_expr);
    }
    return expr;
}

ion_Expr (*ion_parse_expr(void)) {
    return ion_parse_expr_ternary();
}

ion_Expr (*ion_parse_paren_expr(void)) {
    ion_expect_token(ION_TOKEN_LPAREN);
    ion_Expr (*expr) = ion_parse_expr();
    ion_expect_token(ION_TOKEN_RPAREN);
    return expr;
}

ion_StmtList ion_parse_stmt_block(void) {
    ion_SrcPos pos = ion_token.pos;
    ion_expect_token(ION_TOKEN_LBRACE);
    ion_Stmt (*(*stmts)) = NULL;
    while ((!(ion_is_token_eof())) && (!(ion_is_token(ION_TOKEN_RBRACE)))) {
        ion_Stmt (*stmt) = ion_parse_stmt();
        ion_buf_push((void (**))(&(stmts)), &(stmt), sizeof(stmt));
    }
    ion_expect_token(ION_TOKEN_RBRACE);
    return ion_new_stmt_list(pos, stmts, ion_buf_len(stmts));
}

ion_Stmt (*ion_parse_stmt_if(ion_SrcPos pos)) {
    ion_expect_token(ION_TOKEN_LPAREN);
    ion_Expr (*cond) = ion_parse_expr();
    ion_Stmt (*init) = ion_parse_init_stmt(cond);
    if (init) {
        if (ion_match_token(ION_TOKEN_SEMICOLON)) {
            cond = ion_parse_expr();
        } else {
            cond = NULL;
        }
    }
    ion_expect_token(ION_TOKEN_RPAREN);
    ion_StmtList then_block = ion_parse_stmt_block();
    ion_StmtList else_block = {{NULL, 0}, NULL, 0};
    ion_ElseIf (*elseifs) = NULL;
    while (ion_match_keyword(ion_else_keyword)) {
        if (!(ion_match_keyword(ion_if_keyword))) {
            else_block = ion_parse_stmt_block();
            break;
        }
        ion_Expr (*elseif_cond) = ion_parse_paren_expr();
        ion_StmtList elseif_block = ion_parse_stmt_block();
        ion_ElseIf elseif = {elseif_cond, elseif_block};
        ion_buf_push((void (**))(&(elseifs)), &(elseif), sizeof(elseif));
    }
    return ion_new_stmt_if(pos, init, cond, then_block, elseifs, ion_buf_len(elseifs), else_block);
}

ion_Stmt (*ion_parse_stmt_while(ion_SrcPos pos)) {
    ion_Expr (*cond) = ion_parse_paren_expr();
    return ion_new_stmt_while(pos, cond, ion_parse_stmt_block());
}

ion_Stmt (*ion_parse_stmt_do_while(ion_SrcPos pos)) {
    ion_StmtList block = ion_parse_stmt_block();
    if (!(ion_match_keyword(ion_while_keyword))) {
        ion_fatal_error(ion_token.pos, "Expected \'while\' after \'do\' block");
        return NULL;
    }
    ion_Stmt (*stmt) = ion_new_stmt_do_while(pos, ion_parse_paren_expr(), block);
    ion_expect_token(ION_TOKEN_SEMICOLON);
    return stmt;
}

bool ion_is_assign_op(void) {
    return ((ION_TOKEN_FIRST_ASSIGN) <= (ion_token.kind)) && ((ion_token.kind) <= (ION_TOKEN_LAST_ASSIGN));
}

ion_Stmt (*ion_parse_init_stmt(ion_Expr (*left))) {
    if (ion_match_token(ION_TOKEN_COLON_ASSIGN)) {
        if ((left->kind) != (ION_EXPR_NAME)) {
            ion_fatal_error(ion_token.pos, ":= must be preceded by a name");
            return NULL;
        }
        return ion_new_stmt_init(left->pos, left->name, NULL, ion_parse_expr());
    } else if (ion_match_token(ION_TOKEN_COLON)) {
        if ((left->kind) != (ION_EXPR_NAME)) {
            ion_fatal_error(ion_token.pos, ": must be preceded by a name");
            return NULL;
        }
        char const ((*name)) = left->name;
        ion_Typespec (*type) = ion_parse_type();
        ion_Expr (*expr) = NULL;
        if (ion_match_token(ION_TOKEN_ASSIGN)) {
            expr = ion_parse_expr();
        }
        return ion_new_stmt_init(left->pos, name, type, expr);
    } else {
        return NULL;
    }
}

ion_Stmt (*ion_parse_simple_stmt(void)) {
    ion_SrcPos pos = ion_token.pos;
    ion_Expr (*expr) = ion_parse_expr();
    ion_Stmt (*stmt) = ion_parse_init_stmt(expr);
    if (!(stmt)) {
        if (ion_is_assign_op()) {
            ion_TokenKind op = ion_token.kind;
            ion_next_token();
            stmt = ion_new_stmt_assign(pos, op, expr, ion_parse_expr());
        } else {
            stmt = ion_new_stmt_expr(pos, expr);
        }
    }
    return stmt;
}

ion_Stmt (*ion_parse_stmt_for(ion_SrcPos pos)) {
    ion_expect_token(ION_TOKEN_LPAREN);
    ion_Stmt (*init) = NULL;
    if (!(ion_is_token(ION_TOKEN_SEMICOLON))) {
        init = ion_parse_simple_stmt();
    }
    ion_expect_token(ION_TOKEN_SEMICOLON);
    ion_Expr (*cond) = NULL;
    if (!(ion_is_token(ION_TOKEN_SEMICOLON))) {
        cond = ion_parse_expr();
    }
    ion_Stmt (*next) = NULL;
    if (ion_match_token(ION_TOKEN_SEMICOLON)) {
        if (!(ion_is_token(ION_TOKEN_RPAREN))) {
            next = ion_parse_simple_stmt();
            if ((next->kind) == (ION_STMT_INIT)) {
                ion_error(ion_token.pos, "Init statements not allowed in for-statement\'s next clause");
            }
        }
    }
    ion_expect_token(ION_TOKEN_RPAREN);
    return ion_new_stmt_for(pos, init, cond, next, ion_parse_stmt_block());
}

ion_SwitchCasePattern ion_parse_switch_case_pattern(void) {
    ion_Expr (*start) = ion_parse_expr();
    ion_Expr (*end) = NULL;
    if (ion_match_token(ION_TOKEN_ELLIPSIS)) {
        end = ion_parse_expr();
    }
    return (ion_SwitchCasePattern){start, end};
}

ion_SwitchCase ion_parse_stmt_switch_case(void) {
    ion_SwitchCasePattern (*patterns) = NULL;
    bool is_default = false;
    bool is_first_case = true;
    while ((ion_is_keyword(ion_case_keyword)) || (ion_is_keyword(ion_default_keyword))) {
        if (ion_match_keyword(ion_case_keyword)) {
            if (!(is_first_case)) {
                ion_error(ion_token.pos, "Use comma-separated expressions to match multiple values with one case label");
                is_first_case = false;
            }
            ion_SwitchCasePattern pattern = ion_parse_switch_case_pattern();
            ion_buf_push((void (**))(&(patterns)), &(pattern), sizeof(pattern));
            while (ion_match_token(ION_TOKEN_COMMA)) {
                pattern = ion_parse_switch_case_pattern();
                ion_buf_push((void (**))(&(patterns)), &(pattern), sizeof(pattern));
            }
        } else {
            assert(ion_is_keyword(ion_default_keyword));
            ion_next_token();
            if (is_default) {
                ion_error(ion_token.pos, "Duplicate default labels in same switch clause");
            }
            is_default = true;
        }
        ion_expect_token(ION_TOKEN_COLON);
    }
    ion_SrcPos pos = ion_token.pos;
    ion_Stmt (*(*stmts)) = {0};
    while ((((!(ion_is_token_eof())) && (!(ion_is_token(ION_TOKEN_RBRACE)))) && (!(ion_is_keyword(ion_case_keyword)))) && (!(ion_is_keyword(ion_default_keyword)))) {
        ion_Stmt (*stmt) = ion_parse_stmt();
        ion_buf_push((void (**))(&(stmts)), &(stmt), sizeof(stmt));
    }
    return (ion_SwitchCase){patterns, ion_buf_len(patterns), is_default, ion_new_stmt_list(pos, stmts, ion_buf_len(stmts))};
}

ion_Stmt (*ion_parse_stmt_switch(ion_SrcPos pos)) {
    ion_Expr (*expr) = ion_parse_paren_expr();
    ion_SwitchCase (*cases) = {0};
    ion_expect_token(ION_TOKEN_LBRACE);
    while ((!(ion_is_token_eof())) && (!(ion_is_token(ION_TOKEN_RBRACE)))) {
        ion_SwitchCase case_stmt = ion_parse_stmt_switch_case();
        ion_buf_push((void (**))(&(cases)), &(case_stmt), sizeof(case_stmt));
    }
    ion_expect_token(ION_TOKEN_RBRACE);
    return ion_new_stmt_switch(pos, expr, cases, ion_buf_len(cases));
}

ion_Stmt (*ion_parse_stmt(void)) {
    ion_Notes notes = ion_parse_notes();
    ion_SrcPos pos = ion_token.pos;
    ion_Stmt (*stmt) = {0};
    if (ion_match_keyword(ion_if_keyword)) {
        stmt = ion_parse_stmt_if(pos);
    } else if (ion_match_keyword(ion_while_keyword)) {
        stmt = ion_parse_stmt_while(pos);
    } else if (ion_match_keyword(ion_do_keyword)) {
        stmt = ion_parse_stmt_do_while(pos);
    } else if (ion_match_keyword(ion_for_keyword)) {
        stmt = ion_parse_stmt_for(pos);
    } else if (ion_match_keyword(ion_switch_keyword)) {
        stmt = ion_parse_stmt_switch(pos);
    } else if (ion_is_token(ION_TOKEN_LBRACE)) {
        stmt = ion_new_stmt_block(pos, ion_parse_stmt_block());
    } else if (ion_match_keyword(ion_break_keyword)) {
        ion_expect_token(ION_TOKEN_SEMICOLON);
        stmt = ion_new_stmt_break(pos);
    } else if (ion_match_keyword(ion_continue_keyword)) {
        ion_expect_token(ION_TOKEN_SEMICOLON);
        stmt = ion_new_stmt_continue(pos);
    } else if (ion_match_keyword(ion_return_keyword)) {
        ion_Expr (*expr) = {0};
        if (!(ion_is_token(ION_TOKEN_SEMICOLON))) {
            expr = ion_parse_expr();
        }
        ion_expect_token(ION_TOKEN_SEMICOLON);
        stmt = ion_new_stmt_return(pos, expr);
    } else if (ion_match_token(ION_TOKEN_POUND)) {
        ion_Note note = ion_parse_note();
        ion_expect_token(ION_TOKEN_SEMICOLON);
        stmt = ion_new_stmt_note(pos, note);
    } else if (ion_match_token(ION_TOKEN_COLON)) {
        stmt = ion_new_stmt_label(pos, ion_parse_name());
    } else if (ion_match_keyword(ion_goto_keyword)) {
        stmt = ion_new_stmt_goto(pos, ion_parse_name());
        ion_expect_token(ION_TOKEN_SEMICOLON);
    } else {
        stmt = ion_parse_simple_stmt();
        ion_expect_token(ION_TOKEN_SEMICOLON);
    }
    stmt->notes = notes;
    return stmt;
}

char const ((*ion_parse_name(void))) {
    char const ((*name)) = ion_token.name;
    ion_expect_token(ION_TOKEN_NAME);
    return name;
}

ion_EnumItem ion_parse_decl_enum_item(void) {
    ion_SrcPos pos = ion_token.pos;
    char const ((*name)) = ion_parse_name();
    ion_Expr (*init) = NULL;
    if (ion_match_token(ION_TOKEN_ASSIGN)) {
        init = ion_parse_expr();
    }
    return (ion_EnumItem){pos, name, init};
}

ion_Decl (*ion_parse_decl_enum(ion_SrcPos pos)) {
    char const ((*name)) = NULL;
    if (ion_is_token(ION_TOKEN_NAME)) {
        name = ion_parse_name();
    }
    ion_Typespec (*type) = NULL;
    if (ion_match_token(ION_TOKEN_ASSIGN)) {
        type = ion_parse_type();
    }
    ion_expect_token(ION_TOKEN_LBRACE);
    ion_EnumItem (*items) = NULL;
    while (!(ion_is_token(ION_TOKEN_RBRACE))) {
        ion_EnumItem item = ion_parse_decl_enum_item();
        ion_buf_push((void (**))(&(items)), &(item), sizeof(item));
        if (!(ion_match_token(ION_TOKEN_COMMA))) {
            break;
        }
    }
    ion_expect_token(ION_TOKEN_RBRACE);
    return ion_new_decl_enum(pos, name, type, items, ion_buf_len(items));
}

ion_AggregateItem ion_parse_decl_aggregate_item(void) {
    ion_SrcPos pos = ion_token.pos;
    if (ion_match_keyword(ion_struct_keyword)) {
        return (ion_AggregateItem){.pos = pos, .kind = ION_AGGREGATE_ITEM_SUBAGGREGATE, .subaggregate = ion_parse_aggregate(ION_AGGREGATE_STRUCT)};
    } else if (ion_match_keyword(ion_union_keyword)) {
        return (ion_AggregateItem){.pos = pos, .kind = ION_AGGREGATE_ITEM_SUBAGGREGATE, .subaggregate = ion_parse_aggregate(ION_AGGREGATE_UNION)};
    } else {
        char const ((*(*names))) = NULL;
        char const ((*name)) = ion_parse_name();
        ion_buf_push((void (**))(&(names)), &(name), sizeof(name));
        while (ion_match_token(ION_TOKEN_COMMA)) {
            name = ion_parse_name();
            ion_buf_push((void (**))(&(names)), &(name), sizeof(name));
        }
        ion_expect_token(ION_TOKEN_COLON);
        ion_Typespec (*type) = ion_parse_type();
        ion_expect_token(ION_TOKEN_SEMICOLON);
        return (ion_AggregateItem){.pos = pos, .kind = ION_AGGREGATE_ITEM_FIELD, .names = names, .num_names = ion_buf_len(names), .type = type};
    }
}

ion_Aggregate (*ion_parse_aggregate(ion_AggregateKind kind)) {
    ion_SrcPos pos = ion_token.pos;
    ion_expect_token(ION_TOKEN_LBRACE);
    ion_AggregateItem (*items) = NULL;
    while ((!(ion_is_token_eof())) && (!(ion_is_token(ION_TOKEN_RBRACE)))) {
        ion_AggregateItem item = ion_parse_decl_aggregate_item();
        ion_buf_push((void (**))(&(items)), &(item), sizeof(item));
    }
    ion_expect_token(ION_TOKEN_RBRACE);
    return ion_new_aggregate(pos, kind, items, ion_buf_len(items));
}

ion_Decl (*ion_parse_decl_aggregate(ion_SrcPos pos, ion_DeclKind kind)) {
    assert(((kind) == (ION_DECL_STRUCT)) || ((kind) == (ION_DECL_UNION)));
    char const ((*name)) = ion_parse_name();
    ion_AggregateKind aggregate_kind = ((kind) == (ION_DECL_STRUCT) ? ION_AGGREGATE_STRUCT : ION_AGGREGATE_UNION);
    if (ion_match_token(ION_TOKEN_SEMICOLON)) {
        ion_Decl (*decl) = ion_new_decl_aggregate(pos, kind, name, ion_new_aggregate(pos, aggregate_kind, NULL, 0));
        decl->is_incomplete = true;
        return decl;
    } else {
        return ion_new_decl_aggregate(pos, kind, name, ion_parse_aggregate(aggregate_kind));
    }
}

ion_Decl (*ion_parse_decl_var(ion_SrcPos pos)) {
    char const ((*name)) = ion_parse_name();
    if (ion_match_token(ION_TOKEN_ASSIGN)) {
        ion_Expr (*expr) = ion_parse_expr();
        ion_expect_token(ION_TOKEN_SEMICOLON);
        return ion_new_decl_var(pos, name, NULL, expr);
    } else if (ion_match_token(ION_TOKEN_COLON)) {
        ion_Typespec (*type) = ion_parse_type();
        ion_Expr (*expr) = NULL;
        if (ion_match_token(ION_TOKEN_ASSIGN)) {
            expr = ion_parse_expr();
        }
        ion_expect_token(ION_TOKEN_SEMICOLON);
        return ion_new_decl_var(pos, name, type, expr);
    } else {
        ion_fatal_error(ion_token.pos, "Expected : or = after var, got %s", ion_token_info());
        return NULL;
    }
}

ion_Decl (*ion_parse_decl_const(ion_SrcPos pos)) {
    char const ((*name)) = ion_parse_name();
    ion_Typespec (*type) = NULL;
    if (ion_match_token(ION_TOKEN_COLON)) {
        type = ion_parse_type();
    }
    ion_expect_token(ION_TOKEN_ASSIGN);
    ion_Expr (*expr) = ion_parse_expr();
    ion_expect_token(ION_TOKEN_SEMICOLON);
    return ion_new_decl_const(pos, name, type, expr);
}

ion_Decl (*ion_parse_decl_typedef(ion_SrcPos pos)) {
    char const ((*name)) = ion_parse_name();
    ion_expect_token(ION_TOKEN_ASSIGN);
    ion_Typespec (*type) = ion_parse_type();
    ion_expect_token(ION_TOKEN_SEMICOLON);
    return ion_new_decl_typedef(pos, name, type);
}

ion_FuncParam ion_parse_decl_func_param(void) {
    ion_SrcPos pos = ion_token.pos;
    char const ((*name)) = ion_parse_name();
    ion_expect_token(ION_TOKEN_COLON);
    ion_Typespec (*type) = ion_parse_type();
    return (ion_FuncParam){pos, name, type};
}

ion_Decl (*ion_parse_decl_func(ion_SrcPos pos)) {
    char const ((*name)) = ion_parse_name();
    ion_expect_token(ION_TOKEN_LPAREN);
    ion_FuncParam (*params) = NULL;
    bool has_varargs = false;
    while (!(ion_is_token(ION_TOKEN_RPAREN))) {
        if (ion_match_token(ION_TOKEN_ELLIPSIS)) {
            if (has_varargs) {
                ion_error(ion_token.pos, "Multiple ellipsis in function declaration");
            }
            has_varargs = true;
        } else {
            if (has_varargs) {
                ion_error(ion_token.pos, "Ellipsis must be last parameter in function declaration");
            }
            ion_FuncParam param = ion_parse_decl_func_param();
            ion_buf_push((void (**))(&(params)), &(param), sizeof(param));
        }
        if (!(ion_match_token(ION_TOKEN_COMMA))) {
            break;
        }
    }
    ion_expect_token(ION_TOKEN_RPAREN);
    ion_Typespec (*ret_type) = NULL;
    if (ion_match_token(ION_TOKEN_COLON)) {
        ret_type = ion_parse_type();
    }
    ion_StmtList block = {0};
    bool is_incomplete = {0};
    if (ion_match_token(ION_TOKEN_SEMICOLON)) {
        is_incomplete = true;
    } else {
        block = ion_parse_stmt_block();
        is_incomplete = false;
    }
    ion_Decl (*decl) = ion_new_decl_func(pos, name, params, ion_buf_len(params), ret_type, has_varargs, block);
    decl->is_incomplete = is_incomplete;
    return decl;
}

ion_NoteArg ion_parse_note_arg(void) {
    ion_SrcPos pos = ion_token.pos;
    ion_Expr (*expr) = ion_parse_expr();
    char const ((*name)) = NULL;
    if (ion_match_token(ION_TOKEN_ASSIGN)) {
        if ((expr->kind) != (ION_EXPR_NAME)) {
            ion_fatal_error(ion_token.pos, "Left of: operand = in note argument must be a name");
        }
        name = expr->name;
        expr = ion_parse_expr();
    }
    return (ion_NoteArg){.pos = pos, .name = name, .expr = expr};
}

ion_Note ion_parse_note(void) {
    ion_SrcPos pos = ion_token.pos;
    char const ((*name)) = ion_parse_name();
    ion_NoteArg (*args) = NULL;
    if (ion_match_token(ION_TOKEN_LPAREN)) {
        ion_NoteArg arg = ion_parse_note_arg();
        ion_buf_push((void (**))(&(args)), &(arg), sizeof(arg));
        while (ion_match_token(ION_TOKEN_COMMA)) {
            arg = ion_parse_note_arg();
            ion_buf_push((void (**))(&(args)), &(arg), sizeof(arg));
        }
        ion_expect_token(ION_TOKEN_RPAREN);
    }
    return ion_new_note(pos, name, args, ion_buf_len(args));
}

ion_Notes ion_parse_notes(void) {
    ion_Note (*notes) = NULL;
    while (ion_match_token(ION_TOKEN_AT)) {
        ion_Note note = ion_parse_note();
        ion_buf_push((void (**))(&(notes)), &(note), sizeof(note));
    }
    return ion_new_notes(notes, ion_buf_len(notes));
}

ion_Decl (*ion_parse_decl_note(ion_SrcPos pos)) {
    return ion_new_decl_note(pos, ion_parse_note());
}

ion_Decl (*ion_parse_decl_import(ion_SrcPos pos)) {
    char const ((*rename_name)) = {0};
    bool is_relative = {0};
    repeat: ;
    is_relative = false;
    if (ion_match_token(ION_TOKEN_DOT)) {
        is_relative = true;
    }
    char const ((*name)) = ion_token.name;
    ion_expect_token(ION_TOKEN_NAME);
    if ((!(is_relative)) && (ion_match_token(ION_TOKEN_ASSIGN))) {
        if (rename_name) {
            ion_fatal_error(pos, "Only one import assignment is allowed");
        }
        rename_name = name;
        goto repeat;
    }
    char const ((*(*names))) = NULL;
    ion_buf_push((void (**))(&(names)), &(name), sizeof(name));
    while (ion_match_token(ION_TOKEN_DOT)) {
        ion_buf_push((void (**))(&(names)), &(ion_token.name), sizeof(ion_token.name));
        ion_expect_token(ION_TOKEN_NAME);
    }
    bool import_all = false;
    ion_ImportItem (*items) = {0};
    if (ion_match_token(ION_TOKEN_LBRACE)) {
        while (!(ion_is_token(ION_TOKEN_RBRACE))) {
            if (ion_match_token(ION_TOKEN_ELLIPSIS)) {
                import_all = true;
            } else {
                char const ((*item_name)) = ion_parse_name();
                if (ion_match_token(ION_TOKEN_ASSIGN)) {
                    ion_ImportItem item = {.name = ion_parse_name(), .rename = item_name};
                    ion_buf_push((void (**))(&(items)), &(item), sizeof(item));
                } else {
                    ion_ImportItem item = {.name = item_name};
                    ion_buf_push((void (**))(&(items)), &(item), sizeof(item));
                }
                if (!(ion_match_token(ION_TOKEN_COMMA))) {
                    break;
                }
            }
        }
        ion_expect_token(ION_TOKEN_RBRACE);
    }
    return ion_new_decl_import(pos, rename_name, is_relative, names, ion_buf_len(names), import_all, items, ion_buf_len(items));
}

ion_Decl (*ion_parse_decl_opt(void)) {
    ion_SrcPos pos = ion_token.pos;
    if (ion_match_keyword(ion_enum_keyword)) {
        return ion_parse_decl_enum(pos);
    } else if (ion_match_keyword(ion_struct_keyword)) {
        return ion_parse_decl_aggregate(pos, ION_DECL_STRUCT);
    } else if (ion_match_keyword(ion_union_keyword)) {
        return ion_parse_decl_aggregate(pos, ION_DECL_UNION);
    } else if (ion_match_keyword(ion_const_keyword)) {
        return ion_parse_decl_const(pos);
    } else if (ion_match_keyword(ion_typedef_keyword)) {
        return ion_parse_decl_typedef(pos);
    } else if (ion_match_keyword(ion_func_keyword)) {
        return ion_parse_decl_func(pos);
    } else if (ion_match_keyword(ion_var_keyword)) {
        return ion_parse_decl_var(pos);
    } else if (ion_match_keyword(ion_import_keyword)) {
        return ion_parse_decl_import(pos);
    } else if (ion_match_token(ION_TOKEN_POUND)) {
        return ion_parse_decl_note(pos);
    } else {
        return NULL;
    }
}

ion_Decl (*ion_parse_decl(void)) {
    ion_Notes notes = ion_parse_notes();
    ion_Decl (*decl) = ion_parse_decl_opt();
    if (!(decl)) {
        ion_fatal_error(ion_token.pos, "Expected declaration keyword, got %s", ion_token_info());
    }
    decl->notes = notes;
    return decl;
}

ion_Decls (*ion_parse_decls(void)) {
    ion_Decl (*(*decls)) = NULL;
    while (!(ion_is_token(ION_TOKEN_EOF))) {
        ion_Decl (*decl) = ion_parse_decl();
        ion_buf_push((void (**))(&(decls)), &(decl), sizeof(decl));
    }
    return ion_new_decls(decls, ion_buf_len(decls));
}

ion_Package (*ion_current_package);
ion_Package (*ion_builtin_package);
ion_Map ion_package_map;
ion_Package (*(*ion_package_list));
uint8_t ion_reachable_phase = ION_REACHABLE_NATURAL;
ion_Sym (*ion_get_package_sym(ion_Package (*package), char const ((*name)))) {
    return ion_map_get(&(package->syms_map), name);
}

void ion_add_package(ion_Package (*package)) {
    ion_Package (*old_package) = ion_map_get(&(ion_package_map), package->path);
    if ((old_package) != (package)) {
        assert(!(old_package));
        ion_map_put(&(ion_package_map), package->path, package);
        ion_buf_push((void (**))(&(ion_package_list)), &(package), sizeof(package));
    }
}

ion_Package (*ion_enter_package(ion_Package (*new_package))) {
    ion_Package (*old_package) = ion_current_package;
    ion_current_package = new_package;
    return old_package;
}

void ion_leave_package(ion_Package (*old_package)) {
    ion_current_package = old_package;
}

ion_Sym (*(*ion_reachable_syms));
ion_Sym (*(*ion_sorted_syms));
ion_Sym (ion_local_syms[ION_MAX_LOCAL_SYMS]);
ion_Sym (*ion_local_syms_end) = ion_local_syms;
bool ion_is_local_sym(ion_Sym (*sym)) {
    return ((ion_local_syms) <= (sym)) && ((sym) < (ion_local_syms_end));
}

ion_Sym (*ion_sym_new(ion_SymKind kind, char const ((*name)), ion_Decl (*decl))) {
    ion_Sym (*sym) = ion_xcalloc(1, sizeof(ion_Sym));
    sym->kind = kind;
    sym->name = name;
    sym->decl = decl;
    sym->home_package = ion_current_package;
    ion_set_resolved_sym(sym, sym);
    return sym;
}

void ion_process_decl_notes(ion_Decl (*decl), ion_Sym (*sym)) {
    ion_Note (*foreign_note) = ion_get_decl_note(decl, ion_foreign_name);
    if (foreign_note) {
        if ((foreign_note->num_args) > (1)) {
            ion_fatal_error(decl->pos, "@foreign takes 0 or 1 argument");
        }
        char const ((*external_name)) = {0};
        if ((foreign_note->num_args) == (0)) {
            external_name = sym->name;
        } else {
            ion_Expr (*arg) = foreign_note->args[0].expr;
            if ((arg->kind) != (ION_EXPR_STR)) {
                ion_fatal_error(decl->pos, "@foreign argument 1 must be a string literal");
            }
            external_name = arg->str_lit.val;
        }
        sym->external_name = external_name;
    }
}

ion_Sym (*ion_sym_decl(ion_Decl (*decl))) {
    ion_SymKind kind = ION_SYM_NONE;
    switch (decl->kind) {
    case ION_DECL_STRUCT:
    case ION_DECL_UNION:
    case ION_DECL_TYPEDEF:
    case ION_DECL_ENUM: {
        kind = ION_SYM_TYPE;
        break;
    }
    case ION_DECL_VAR: {
        kind = ION_SYM_VAR;
        break;
    }
    case ION_DECL_CONST: {
        kind = ION_SYM_CONST;
        break;
    }
    case ION_DECL_FUNC: {
        kind = ION_SYM_FUNC;
        break;
    }
    default: {
        assert(0);
        break;
    }
    }
    ion_Sym (*sym) = ion_sym_new(kind, decl->name, decl);
    ion_set_resolved_sym(decl, sym);
    ion_process_decl_notes(decl, sym);
    return sym;
}

ion_Sym (*ion_sym_get_local(char const ((*name)))) {
    for (ion_Sym (*it) = ion_local_syms_end; (it) != (ion_local_syms); (it)--) {
        ion_Sym (*sym) = (it) - (1);
        if ((sym->name) == (name)) {
            return sym;
        }
    }
    return NULL;
}

ion_Sym (*ion_sym_get(char const ((*name)))) {
    ion_Sym (*sym) = ion_sym_get_local(name);
    return (sym ? sym : ion_get_package_sym(ion_current_package, name));
}

bool ion_sym_push_var(char const ((*name)), ion_Type (*type)) {
    if (ion_sym_get_local(name)) {
        return false;
    }
    if ((ion_local_syms_end) == ((ion_local_syms) + (ION_MAX_LOCAL_SYMS))) {
        ion_fatal("Too many local symbols");
    }
    *((ion_local_syms_end)++) = (ion_Sym){.name = name, .kind = ION_SYM_VAR, .state = ION_SYM_RESOLVED, .type = type};
    return true;
}

ion_Sym (*ion_sym_enter(void)) {
    return ion_local_syms_end;
}

void ion_sym_leave(ion_Sym (*sym)) {
    ion_local_syms_end = sym;
}

void ion_sym_global_put(char const ((*name)), ion_Sym (*sym)) {
    ion_Sym (*old_sym) = ion_map_get(&(ion_current_package->syms_map), name);
    if (old_sym) {
        if ((sym) == (old_sym)) {
            return;
        }
        if ((((sym->kind) == (ION_SYM_PACKAGE)) && ((old_sym->kind) == (ION_SYM_PACKAGE))) && ((sym->package) == (old_sym->package))) {
            return;
        }
        ion_SrcPos pos = (sym->decl ? sym->decl->pos : ion_pos_builtin);
        if (old_sym->decl) {
            ion_warning(old_sym->decl->pos, "Previous definition of \'%s\'", name);
        }
        ion_fatal_error(pos, "Duplicate definition of global symbol \'%s\'.", name);
    }
    ion_map_put(&(ion_current_package->syms_map), name, sym);
    ion_buf_push((void (**))(&(ion_current_package->syms)), &(sym), sizeof(sym));
}

ion_Sym (*ion_sym_global_type(char const ((*name)), ion_Type (*type))) {
    name = ion_str_intern(name);
    ion_Sym (*sym) = ion_sym_new(ION_SYM_TYPE, name, NULL);
    sym->state = ION_SYM_RESOLVED;
    sym->type = type;
    sym->external_name = name;
    ion_sym_global_put(name, sym);
    return sym;
}

ion_Sym (*ion_sym_global_decl(ion_Decl (*decl))) {
    ion_Sym (*sym) = NULL;
    if (decl->name) {
        sym = ion_sym_decl(decl);
        ion_sym_global_put(sym->name, sym);
    }
    if ((decl->kind) == (ION_DECL_ENUM)) {
        ion_Typespec (*enum_typespec) = ion_new_typespec_name(decl->pos, (sym ? sym->name : ion_str_intern("int")));
        char const ((*prev_item_name)) = NULL;
        for (size_t i = 0; (i) < (decl->enum_decl.num_items); (i)++) {
            ion_EnumItem item = decl->enum_decl.items[i];
            ion_Expr (*init) = {0};
            if (item.init) {
                init = item.init;
            } else if (prev_item_name) {
                init = ion_new_expr_binary(item.pos, ION_TOKEN_ADD, ion_new_expr_name(item.pos, prev_item_name), ion_new_expr_int(item.pos, 1, 0, 0));
            } else {
                init = ion_new_expr_int(item.pos, 0, 0, 0);
            }
            ion_Decl (*item_decl) = ion_new_decl_const(item.pos, item.name, enum_typespec, init);
            item_decl->notes = decl->notes;
            ion_sym_global_decl(item_decl);
            prev_item_name = item.name;
        }
    }
    return sym;
}

void ion_put_type_name(char (*(*buf)), ion_Type (*type)) {
    char const ((*type_name)) = ion_type_names[type->kind];
    if (type_name) {
        ion_buf_printf(buf, "%s", type_name);
    } else {
        switch (type->kind) {
        case ION_CMPL_TYPE_STRUCT:
        case ION_CMPL_TYPE_UNION:
        case ION_CMPL_TYPE_ENUM:
        case ION_CMPL_TYPE_INCOMPLETE: {
            assert(type->sym);
            ion_buf_printf(buf, "%s", type->sym->name);
            break;
        }
        case ION_CMPL_TYPE_CONST: {
            ion_put_type_name(buf, type->base);
            ion_buf_printf(buf, " const");
            break;
        }
        case ION_CMPL_TYPE_PTR: {
            ion_put_type_name(buf, type->base);
            ion_buf_printf(buf, "*");
            break;
        }
        case ION_CMPL_TYPE_ARRAY: {
            ion_put_type_name(buf, type->base);
            ion_buf_printf(buf, "[%zu]", type->num_elems);
            break;
        }
        case ION_CMPL_TYPE_FUNC: {
            ion_buf_printf(buf, "func(");
            for (size_t i = 0; (i) < (type->function.num_params); (i)++) {
                if ((i) != (0)) {
                    ion_buf_printf(buf, ", ");
                }
                ion_put_type_name(buf, type->function.params[i]);
            }
            if (type->function.has_varargs) {
                ion_buf_printf(buf, "...");
            }
            ion_buf_printf(buf, ")");
            if ((type->function.ret) != (ion_type_void)) {
                ion_buf_printf(buf, ": ");
                ion_put_type_name(buf, type->function.ret);
            }
            break;
        }
        default: {
            assert(0);
            break;
        }
        }
    }
}

char (*ion_get_type_name(ion_Type (*type))) {
    char (*buf) = NULL;
    ion_put_type_name(&(buf), type);
    return buf;
}

ion_Operand ion_operand_null;
ion_Operand ion_operand_rvalue(ion_Type (*type)) {
    return (ion_Operand){.type = ion_unqualify_type(type)};
}

ion_Operand ion_operand_lvalue(ion_Type (*type)) {
    return (ion_Operand){.type = type, .is_lvalue = true};
}

ion_Operand ion_operand_const(ion_Type (*type), ion_Val val) {
    return (ion_Operand){.type = ion_unqualify_type(type), .is_const = true, .val = val};
}

ion_Type (*ion_type_decay(ion_Type (*type))) {
    type = ion_unqualify_type(type);
    if ((type->kind) == (ION_CMPL_TYPE_ARRAY)) {
        type = ion_type_ptr(type->base);
    }
    return type;
}

ion_Operand ion_operand_decay(ion_Operand operand) {
    operand.type = ion_type_decay(operand.type);
    operand.is_lvalue = false;
    return operand;
}

bool ion_is_convertible(ion_Operand (*operand), ion_Type (*dest)) {
    dest = ion_unqualify_type(dest);
    ion_Type (*src) = ion_unqualify_type(operand->type);
    if ((dest) == (src)) {
        return true;
    } else if ((ion_is_arithmetic_type(dest)) && (ion_is_arithmetic_type(src))) {
        return true;
    } else if ((ion_is_ptr_like_type(dest)) && (ion_is_null_ptr(*(operand)))) {
        return true;
    } else if ((ion_is_ptr_type(dest)) && (ion_is_ptr_type(src))) {
        if ((ion_is_const_type(dest->base)) && (ion_is_const_type(src->base))) {
            return (((dest->base->base) == (src->base->base)) || ((dest->base->base) == (ion_type_void))) || ((src->base->base) == (ion_type_void));
        } else {
            ion_Type (*unqual_dest_base) = ion_unqualify_type(dest->base);
            if ((unqual_dest_base) == (src->base)) {
                return true;
            } else if ((unqual_dest_base) == (ion_type_void)) {
                return (ion_is_const_type(dest->base)) || (!(ion_is_const_type(src->base)));
            } else {
                return (src->base) == (ion_type_void);
            }
        }
    } else {
        return false;
    }
}

bool ion_is_castable(ion_Operand (*operand), ion_Type (*dest)) {
    ion_Type (*src) = operand->type;
    if (ion_is_convertible(operand, dest)) {
        return true;
    } else if (ion_is_integer_type(dest)) {
        return ion_is_ptr_like_type(src);
    } else if (ion_is_integer_type(src)) {
        return ion_is_ptr_like_type(dest);
    } else if ((ion_is_ptr_like_type(dest)) && (ion_is_ptr_like_type(src))) {
        return true;
    } else {
        return false;
    }
}

bool ion_convert_operand(ion_Operand (*operand), ion_Type (*type)) {
    if (ion_is_convertible(operand, type)) {
        cast_operand(operand, type);
        operand->type = ion_unqualify_type(operand->type);
        operand->is_lvalue = false;
        return true;
    }
    return false;
}

bool ion_is_null_ptr(ion_Operand operand) {
    if ((operand.is_const) && (((ion_is_ptr_type(operand.type)) || (ion_is_integer_type(operand.type))))) {
        cast_operand(&(operand), ion_type_ullong);
        return (operand.val.ull) == (0);
    } else {
        return false;
    }
}

void ion_promote_operand(ion_Operand (*operand)) {
    switch (operand->type->kind) {
    case ION_CMPL_TYPE_BOOL:
    case ION_CMPL_TYPE_CHAR:
    case ION_CMPL_TYPE_SCHAR:
    case ION_CMPL_TYPE_UCHAR:
    case ION_CMPL_TYPE_SHORT:
    case ION_CMPL_TYPE_USHORT:
    case ION_CMPL_TYPE_ENUM: {
        cast_operand(operand, ion_type_int);
        break;
    }
    default: {
        break;
    }
    }
}

void ion_unify_arithmetic_operands(ion_Operand (*left), ion_Operand (*right)) {
    if ((left->type) == (ion_type_double)) {
        cast_operand(right, ion_type_double);
    } else if ((right->type) == (ion_type_double)) {
        cast_operand(left, ion_type_double);
    } else if ((left->type) == (ion_type_float)) {
        cast_operand(right, ion_type_float);
    } else if ((right->type) == (ion_type_float)) {
        cast_operand(left, ion_type_float);
    } else {
        assert(ion_is_integer_type(left->type));
        assert(ion_is_integer_type(right->type));
        ion_promote_operand(left);
        ion_promote_operand(right);
        if ((left->type) != (right->type)) {
            if ((ion_is_signed_type(left->type)) == (ion_is_signed_type(right->type))) {
                if ((ion_type_rank(left->type)) <= (ion_type_rank(right->type))) {
                    cast_operand(left, right->type);
                } else {
                    cast_operand(right, left->type);
                }
            } else if ((ion_is_signed_type(left->type)) && ((ion_type_rank(right->type)) >= (ion_type_rank(left->type)))) {
                cast_operand(left, right->type);
            } else if ((ion_is_signed_type(right->type)) && ((ion_type_rank(left->type)) >= (ion_type_rank(right->type)))) {
                cast_operand(right, left->type);
            } else if ((ion_is_signed_type(left->type)) && ((ion_type_sizeof(left->type)) > (ion_type_sizeof(right->type)))) {
                cast_operand(right, left->type);
            } else if ((ion_is_signed_type(right->type)) && ((ion_type_sizeof(right->type)) > (ion_type_sizeof(left->type)))) {
                cast_operand(left, right->type);
            } else {
                ion_Type (*type) = ion_unsigned_type((ion_is_signed_type(left->type) ? left->type : right->type));
                cast_operand(left, type);
                cast_operand(right, type);
            }
        }
    }
    assert((left->type) == (right->type));
}

ion_Map ion_resolved_val_map;
ion_Val ion_get_resolved_val(void (*ptr)) {
    uint64_t u64 = ion_map_get_uint64(&(ion_resolved_val_map), ptr);
    ion_Val val = {0};
    assert((sizeof(val)) == (sizeof(u64)));
    memcpy(&(val), &(u64), sizeof(u64));
    return val;
}

void ion_set_resolved_val(void (*ptr), ion_Val val) {
    uint64_t u64 = {0};
    assert((sizeof(val)) == (sizeof(u64)));
    memcpy(&(u64), &(val), sizeof(val));
    ion_map_put_uint64(&(ion_resolved_val_map), ptr, u64);
}

ion_Map ion_resolved_type_map;
ion_Type (*ion_get_resolved_type(void (*ptr))) {
    return ion_map_get(&(ion_resolved_type_map), ptr);
}

void ion_set_resolved_type(void (*ptr), ion_Type (*type)) {
    ion_map_put(&(ion_resolved_type_map), ptr, type);
}

ion_Map ion_resolved_sym_map;
ion_Sym (*ion_get_resolved_sym(void const ((*ptr)))) {
    return ion_map_get(&(ion_resolved_sym_map), ptr);
}

void ion_set_resolved_sym(void const ((*ptr)), ion_Sym (*sym)) {
    if (!(ion_is_local_sym(sym))) {
        ion_map_put(&(ion_resolved_sym_map), ptr, sym);
    }
}

ion_Map ion_resolved_expected_type_map;
ion_Type (*ion_get_resolved_expected_type(ion_Expr (*expr))) {
    return ion_map_get(&(ion_resolved_expected_type_map), expr);
}

void ion_set_resolved_expected_type(ion_Expr (*expr), ion_Type (*type)) {
    if ((expr) && (type)) {
        ion_map_put(&(ion_resolved_expected_type_map), expr, type);
    }
}

ion_Operand ion_resolve_expr(ion_Expr (*expr)) {
    return ion_resolve_expected_expr(expr, NULL);
}

ion_Operand ion_resolve_expr_rvalue(ion_Expr (*expr)) {
    return ion_operand_decay(ion_resolve_expr(expr));
}

ion_Operand ion_resolve_expected_expr_rvalue(ion_Expr (*expr), ion_Type (*expected_type)) {
    return ion_operand_decay(ion_resolve_expected_expr(expr, expected_type));
}

ion_Type (*ion_resolve_typespec(ion_Typespec (*typespec))) {
    if (!(typespec)) {
        return ion_type_void;
    }
    ion_Type (*result) = NULL;
    switch (typespec->kind) {
    case ION_TYPESPEC_NAME: {
        {
            ion_Sym (*sym) = ion_resolve_name(typespec->name);
            if (!(sym)) {
                ion_fatal_error(typespec->pos, "Unresolved type name \'%s\'", typespec->name);
            }
            if ((sym->kind) != (ION_SYM_TYPE)) {
                ion_fatal_error(typespec->pos, "%s must denote a type", typespec->name);
                return NULL;
            }
            ion_set_resolved_sym(typespec, sym);
            result = sym->type;
        }
        break;
    }
    case ION_TYPESPEC_CONST: {
        result = ion_type_const(ion_resolve_typespec(typespec->base));
        break;
    }
    case ION_TYPESPEC_PTR: {
        result = ion_type_ptr(ion_resolve_typespec(typespec->base));
        break;
    }
    case ION_TYPESPEC_ARRAY: {
        {
            int size = 0;
            if (typespec->num_elems) {
                ion_Operand operand = ion_resolve_const_expr(typespec->num_elems);
                if (!(ion_is_integer_type(operand.type))) {
                    ion_fatal_error(typespec->pos, "Array size constant expression must have integer type");
                }
                cast_operand(&(operand), ion_type_int);
                size = operand.val.i;
                if ((size) <= (0)) {
                    ion_fatal_error(typespec->num_elems->pos, "Non-positive array size");
                }
            }
            result = ion_type_array(ion_resolve_typespec(typespec->base), size);
        }
        break;
    }
    case ION_TYPESPEC_FUNC: {
        {
            ion_Type (*(*args)) = NULL;
            for (size_t i = 0; (i) < (typespec->function.num_args); (i)++) {
                ion_Type (*arg) = ion_resolve_typespec(typespec->function.args[i]);
                if ((arg) == (ion_type_void)) {
                    ion_fatal_error(typespec->pos, "Function parameter type cannot be void");
                }
                ion_buf_push((void (**))(&(args)), &(arg), sizeof(arg));
            }
            ion_Type (*ret) = ion_type_void;
            if (typespec->function.ret) {
                ret = ion_resolve_typespec(typespec->function.ret);
            }
            if (ion_is_array_type(ret)) {
                ion_fatal_error(typespec->pos, "Function return type cannot be array");
            }
            result = ion_type_func(args, ion_buf_len(args), ret, false);
        }
        break;
    }
    default: {
        assert(0);
        return NULL;
        break;
    }
    }
    ion_set_resolved_type(typespec, result);
    return result;
}

ion_Type (*ion_complete_aggregate(ion_Type (*type), ion_Aggregate (*aggregate))) {
    ion_TypeField (*fields) = {0};
    for (size_t i = 0; (i) < (aggregate->num_items); (i)++) {
        ion_AggregateItem item = aggregate->items[i];
        if ((item.kind) == (ION_AGGREGATE_ITEM_FIELD)) {
            ion_Type (*item_type) = ion_resolve_typespec(item.type);
            ion_complete_type(item_type);
            if ((ion_type_sizeof(item_type)) == (0)) {
                ion_fatal_error(item.pos, "Field type of size 0 is not allowed");
            }
            for (size_t j = 0; (j) < (item.num_names); (j)++) {
                ion_TypeField type_field = {item.names[j], item_type};
                ion_buf_push((void (**))(&(fields)), &(type_field), sizeof(type_field));
            }
        } else {
            assert((item.kind) == (ION_AGGREGATE_ITEM_SUBAGGREGATE));
            ion_Type (*item_type) = ion_complete_aggregate(NULL, item.subaggregate);
            ion_TypeField type_field = {NULL, item_type};
            ion_buf_push((void (**))(&(fields)), &(type_field), sizeof(type_field));
        }
    }
    if (!(type)) {
        type = ion_type_incomplete(NULL);
        type->kind = ION_CMPL_TYPE_COMPLETING;
    }
    if ((aggregate->kind) == (ION_AGGREGATE_STRUCT)) {
        ion_type_complete_struct(type, fields, ion_buf_len(fields));
    } else {
        assert((aggregate->kind) == (ION_AGGREGATE_UNION));
        ion_type_complete_union(type, fields, ion_buf_len(fields));
    }
    if ((type->aggregate.num_fields) == (0)) {
        ion_fatal_error(aggregate->pos, "No fields");
    }
    if (ion_has_duplicate_fields(type)) {
        ion_fatal_error(aggregate->pos, "Duplicate fields");
    }
    return type;
}

void ion_complete_type(ion_Type (*type)) {
    if ((type->kind) == (ION_CMPL_TYPE_COMPLETING)) {
        ion_fatal_error(type->sym->decl->pos, "Type completion cycle");
        return;
    } else if ((type->kind) != (ION_CMPL_TYPE_INCOMPLETE)) {
        return;
    }
    ion_Sym (*sym) = type->sym;
    ion_Package (*old_package) = ion_enter_package(sym->home_package);
    ion_Decl (*decl) = sym->decl;
    if (decl->is_incomplete) {
        ion_fatal_error(decl->pos, "Trying to use incomplete type as complete type");
    }
    type->kind = ION_CMPL_TYPE_COMPLETING;
    assert(((decl->kind) == (ION_DECL_STRUCT)) || ((decl->kind) == (ION_DECL_UNION)));
    ion_complete_aggregate(type, decl->aggregate);
    ion_buf_push((void (**))(&(ion_sorted_syms)), &(type->sym), sizeof(type->sym));
    ion_leave_package(old_package);
}

ion_Type (*ion_resolve_typed_init(ion_SrcPos pos, ion_Type (*type), ion_Expr (*expr))) {
    ion_Type (*expected_type) = ion_unqualify_type(type);
    ion_Operand operand = ion_resolve_expected_expr(expr, expected_type);
    if (((ion_is_incomplete_array_type(type)) && (ion_is_array_type(operand.type))) && ((type->base) == (operand.type->base))) {
    } else {
        if ((type) && (ion_is_ptr_type(type))) {
            operand = ion_operand_decay(operand);
        }
        if (!(ion_convert_operand(&(operand), expected_type))) {
            return NULL;
        }
    }
    ion_set_resolved_expected_type(expr, operand.type);
    return operand.type;
}

ion_Type (*ion_resolve_init(ion_SrcPos pos, ion_Typespec (*typespec), ion_Expr (*expr))) {
    ion_Type (*type) = {0};
    if (typespec) {
        ion_Type (*declared_type) = ion_resolve_typespec(typespec);
        type = declared_type;
        if (expr) {
            type = ion_resolve_typed_init(pos, declared_type, expr);
            if (!(type)) {
                ion_fatal_error(pos, "Invalid type in initialization. Expected %s", ion_get_type_name(declared_type));
            }
        }
    } else {
        assert(expr);
        type = ion_unqualify_type(ion_resolve_expr(expr).type);
        if ((ion_is_array_type(type)) && ((expr->kind) != (ION_EXPR_COMPOUND))) {
            type = ion_type_decay(type);
            ion_set_resolved_type(expr, type);
        }
        ion_set_resolved_expected_type(expr, type);
    }
    ion_complete_type(type);
    if ((type->size) == (0)) {
        ion_fatal_error(pos, "Cannot declare variable of size 0");
    }
    return type;
}

ion_Type (*ion_resolve_decl_var(ion_Decl (*decl))) {
    assert((decl->kind) == (ION_DECL_VAR));
    return ion_resolve_init(decl->pos, decl->var_decl.type, decl->var_decl.expr);
}

ion_Type (*ion_resolve_decl_const(ion_Decl (*decl), ion_Val (*val))) {
    assert((decl->kind) == (ION_DECL_CONST));
    ion_Operand result = ion_resolve_const_expr(decl->const_decl.expr);
    if (!(ion_is_scalar_type(result.type))) {
        ion_fatal_error(decl->pos, "Const declarations must have scalar type");
    }
    if (decl->const_decl.type) {
        ion_Type (*type) = ion_resolve_typespec(decl->const_decl.type);
        if (!(ion_convert_operand(&(result), type))) {
            ion_fatal_error(decl->pos, "Invalid type in constant declaration. Expected %s, got %s", ion_get_type_name(type), ion_get_type_name(result.type));
        }
    }
    *(val) = result.val;
    return result.type;
}

ion_Type (*ion_resolve_decl_func(ion_Decl (*decl))) {
    assert((decl->kind) == (ION_DECL_FUNC));
    ion_Type (*(*params)) = NULL;
    for (size_t i = 0; (i) < (decl->function.num_params); (i)++) {
        ion_Type (*param) = ion_resolve_typespec(decl->function.params[i].type);
        ion_complete_type(param);
        if ((param) == (ion_type_void)) {
            ion_fatal_error(decl->pos, "Function parameter type cannot be void");
        }
        ion_buf_push((void (**))(&(params)), &(param), sizeof(param));
    }
    ion_Type (*ret_type) = ion_type_void;
    if (decl->function.ret_type) {
        ret_type = ion_resolve_typespec(decl->function.ret_type);
        ion_complete_type(ret_type);
    }
    if (ion_is_array_type(ret_type)) {
        ion_fatal_error(decl->pos, "Function return type cannot be array");
    }
    return ion_type_func(params, ion_buf_len(params), ret_type, decl->function.has_varargs);
}

ion_Label (ion_labels[ION_MAX_LABELS]);
ion_Label (*ion_labels_end) = ion_labels;
ion_Label (*ion_get_label(ion_SrcPos pos, char const ((*name)))) {
    ion_Label (*label) = {0};
    for (label = ion_labels; (label) != (ion_labels_end); (label)++) {
        if ((label->name) == (name)) {
            return label;
        }
    }
    if ((label) == ((ion_labels) + (ION_MAX_LABELS))) {
        ion_fatal_error(pos, "Too many labels");
    }
    *(label) = (ion_Label){.name = name, .pos = pos};
    (ion_labels_end)++;
    return label;
}

void ion_reference_label(ion_SrcPos pos, char const ((*name))) {
    ion_Label (*label) = ion_get_label(pos, name);
    label->referenced = true;
}

void ion_define_label(ion_SrcPos pos, char const ((*name))) {
    ion_Label (*label) = ion_get_label(pos, name);
    if (label->defined) {
        ion_fatal_error(pos, "Multiple definitions of label \'%s\'", name);
    }
    label->defined = true;
}

void ion_resolve_labels(void) {
    for (ion_Label (*label) = ion_labels; (label) != (ion_labels_end); (label)++) {
        if ((label->referenced) && (!(label->defined))) {
            ion_fatal_error(label->pos, "Label \'%s\' referenced but not defined", label->name);
        }
        if ((label->defined) && (!(label->referenced))) {
            ion_warning(label->pos, "Label \'%s\' defined but not referenced", label->name);
        }
    }
    ion_labels_end = ion_labels;
}

bool ion_is_cond_operand(ion_Operand operand) {
    operand = ion_operand_decay(operand);
    return ion_is_scalar_type(operand.type);
}

void ion_resolve_cond_expr(ion_Expr (*expr)) {
    ion_Operand cond = ion_resolve_expr_rvalue(expr);
    if (!(ion_is_cond_operand(cond))) {
        ion_fatal_error(expr->pos, "Conditional expression must have scalar type");
    }
}

bool ion_resolve_stmt_block(ion_StmtList block, ion_Type (*ret_type), ion_StmtCtx ctx) {
    ion_Sym (*scope) = ion_sym_enter();
    bool returns = false;
    for (size_t i = 0; (i) < (block.num_stmts); (i)++) {
        returns = (ion_resolve_stmt(block.stmts[i], ret_type, ctx)) || (returns);
    }
    ion_sym_leave(scope);
    return returns;
}

void ion_resolve_stmt_assign(ion_Stmt (*stmt)) {
    assert((stmt->kind) == (ION_STMT_ASSIGN));
    ion_Operand left = ion_resolve_expr(stmt->assign.left);
    if (!(left.is_lvalue)) {
        ion_fatal_error(stmt->pos, "Cannot assign to non-lvalue");
    }
    if (ion_is_array_type(left.type)) {
        ion_fatal_error(stmt->pos, "Cannot assign to array");
    }
    if (left.type->nonmodifiable) {
        ion_fatal_error(stmt->pos, "Left-hand side of assignment has non-modifiable type");
    }
    char const ((*assign_op_name)) = ion_token_kind_name(stmt->assign.op);
    ion_TokenKind binary_op = ion_assign_token_to_binary_token[stmt->assign.op];
    ion_Operand right = ion_resolve_expected_expr_rvalue(stmt->assign.right, left.type);
    ion_Operand result = {0};
    if ((stmt->assign.op) == (ION_TOKEN_ASSIGN)) {
        result = right;
    } else if (((stmt->assign.op) == (ION_TOKEN_ADD_ASSIGN)) || ((stmt->assign.op) == (ION_TOKEN_SUB_ASSIGN))) {
        if (((left.type->kind) == (ION_CMPL_TYPE_PTR)) && (ion_is_integer_type(right.type))) {
            result = ion_operand_rvalue(left.type);
        } else if ((ion_is_arithmetic_type(left.type)) && (ion_is_arithmetic_type(right.type))) {
            result = ion_resolve_expr_binary_op(binary_op, assign_op_name, stmt->pos, left, right);
        } else {
            ion_fatal_error(stmt->pos, "Invalid operand types for %s", assign_op_name);
        }
    } else {
        result = ion_resolve_expr_binary_op(binary_op, assign_op_name, stmt->pos, left, right);
    }
    if (!(ion_convert_operand(&(result), left.type))) {
        ion_fatal_error(stmt->pos, "Invalid type in assignment. Expected %s, got %s", ion_get_type_name(left.type), ion_get_type_name(result.type));
    }
}

void ion_resolve_stmt_init(ion_Stmt (*stmt)) {
    assert((stmt->kind) == (ION_STMT_INIT));
    ion_Type (*type) = ion_resolve_init(stmt->pos, stmt->init.type, stmt->init.expr);
    if (!(ion_sym_push_var(stmt->init.name, type))) {
        ion_fatal_error(stmt->pos, "Shadowed definition of local symbol");
    }
}

void ion_resolve_static_assert(ion_Note note) {
    if ((note.num_args) != (1)) {
        ion_fatal_error(note.pos, "#static_assert takes 1 argument");
    }
    ion_Operand operand = ion_resolve_const_expr(note.args[0].expr);
    if (!(operand.val.ull)) {
        ion_fatal_error(note.pos, "#static_assert failed");
    }
}

bool ion_resolve_stmt(ion_Stmt (*stmt), ion_Type (*ret_type), ion_StmtCtx ctx) {
    switch (stmt->kind) {
    case ION_STMT_RETURN: {
        if (stmt->expr) {
            ion_Operand operand = ion_resolve_expected_expr_rvalue(stmt->expr, ret_type);
            if (!(ion_convert_operand(&(operand), ret_type))) {
                ion_fatal_error(stmt->pos, "Invalid type in return expression. Expected %s, got %s", ion_get_type_name(ret_type), ion_get_type_name(operand.type));
            }
        } else if ((ret_type) != (ion_type_void)) {
            ion_fatal_error(stmt->pos, "Empty return expression for function with non-void return type");
        }
        return true;
        break;
    }
    case ION_STMT_BREAK: {
        if (!(ctx.is_break_legal)) {
            ion_fatal_error(stmt->pos, "Illegal break");
        }
        return false;
        break;
    }
    case ION_STMT_CONTINUE: {
        if (!(ctx.is_continue_legal)) {
            ion_fatal_error(stmt->pos, "Illegal continue");
        }
        return false;
        break;
    }
    case ION_STMT_BLOCK: {
        return ion_resolve_stmt_block(stmt->block, ret_type, ctx);
        break;
    }
    case ION_STMT_NOTE: {
        if ((stmt->note.name) == (ion_assert_name)) {
            if ((stmt->note.num_args) != (1)) {
                ion_fatal_error(stmt->pos, "#assert takes 1 argument");
            }
            ion_resolve_cond_expr(stmt->note.args[0].expr);
        } else if ((stmt->note.name) == (ion_static_assert_name)) {
            ion_resolve_static_assert(stmt->note);
        } else {
            ion_warning(stmt->pos, "Unknown statement #directive \'%s\'", stmt->note.name);
        }
        return false;
        break;
    }
    case ION_STMT_IF: {
        {
            ion_Sym (*scope) = ion_sym_enter();
            if (stmt->if_stmt.init) {
                ion_resolve_stmt_init(stmt->if_stmt.init);
            }
            if (stmt->if_stmt.cond) {
                ion_resolve_cond_expr(stmt->if_stmt.cond);
            } else if (!(ion_is_cond_operand(ion_resolve_name_operand(stmt->pos, stmt->if_stmt.init->init.name)))) {
                ion_fatal_error(stmt->pos, "Conditional expression must have scalar type");
            }
            bool returns = ion_resolve_stmt_block(stmt->if_stmt.then_block, ret_type, ctx);
            for (size_t i = 0; (i) < (stmt->if_stmt.num_elseifs); (i)++) {
                ion_ElseIf elseif = stmt->if_stmt.elseifs[i];
                ion_resolve_cond_expr(elseif.cond);
                returns = (ion_resolve_stmt_block(elseif.block, ret_type, ctx)) && (returns);
            }
            if (stmt->if_stmt.else_block.stmts) {
                returns = (ion_resolve_stmt_block(stmt->if_stmt.else_block, ret_type, ctx)) && (returns);
            } else {
                returns = false;
            }
            ion_sym_leave(scope);
            return returns;
        }
        break;
    }
    case ION_STMT_WHILE:
    case ION_STMT_DO_WHILE: {
        ion_resolve_cond_expr(stmt->while_stmt.cond);
        ctx.is_break_legal = true;
        ctx.is_continue_legal = true;
        ion_resolve_stmt_block(stmt->while_stmt.block, ret_type, ctx);
        return false;
        break;
    }
    case ION_STMT_FOR: {
        {
            ion_Sym (*scope) = ion_sym_enter();
            if (stmt->for_stmt.init) {
                ion_resolve_stmt(stmt->for_stmt.init, ret_type, ctx);
            }
            if (stmt->for_stmt.cond) {
                ion_resolve_cond_expr(stmt->for_stmt.cond);
            }
            if (stmt->for_stmt.next) {
                ion_resolve_stmt(stmt->for_stmt.next, ret_type, ctx);
            }
            ctx.is_break_legal = true;
            ctx.is_continue_legal = true;
            ion_resolve_stmt_block(stmt->for_stmt.block, ret_type, ctx);
            ion_sym_leave(scope);
            return false;
        }
        break;
    }
    case ION_STMT_SWITCH: {
        {
            ion_Operand operand = ion_resolve_expr_rvalue(stmt->switch_stmt.expr);
            if (!(ion_is_integer_type(operand.type))) {
                ion_fatal_error(stmt->pos, "Switch expression must have integer type");
            }
            ctx.is_break_legal = true;
            bool returns = true;
            bool has_default = false;
            for (size_t i = 0; (i) < (stmt->switch_stmt.num_cases); (i)++) {
                ion_SwitchCase switch_case = stmt->switch_stmt.cases[i];
                for (size_t j = 0; (j) < (switch_case.num_patterns); (j)++) {
                    ion_SwitchCasePattern pattern = switch_case.patterns[j];
                    ion_Expr (*start_expr) = pattern.start;
                    ion_Operand start_operand = ion_resolve_const_expr(start_expr);
                    if (!(ion_convert_operand(&(start_operand), operand.type))) {
                        ion_fatal_error(start_expr->pos, "Invalid type in switch case expression. Expected %s, got %s", ion_get_type_name(operand.type), ion_get_type_name(start_operand.type));
                    }
                    ion_Expr (*end_expr) = pattern.end;
                    if (end_expr) {
                        ion_Operand end_operand = ion_resolve_const_expr(end_expr);
                        if (!(ion_convert_operand(&(end_operand), operand.type))) {
                            ion_fatal_error(end_expr->pos, "Invalid type in switch case expression. Expected %s, got %s", ion_get_type_name(operand.type), ion_get_type_name(end_operand.type));
                        }
                        ion_convert_operand(&(start_operand), ion_type_llong);
                        ion_set_resolved_val(start_expr, start_operand.val);
                        ion_convert_operand(&(end_operand), ion_type_llong);
                        ion_set_resolved_val(end_expr, end_operand.val);
                        if ((end_operand.val.ll) < (start_operand.val.ll)) {
                            ion_fatal_error(start_expr->pos, "Case range end value cannot be less thn start value");
                        }
                        if (((end_operand.val.ll) - (start_operand.val.ll)) >= (256)) {
                            ion_fatal_error(start_expr->pos, "Case range cannot span more than 256 values");
                        }
                    }
                }
                if (switch_case.is_default) {
                    if (has_default) {
                        ion_fatal_error(stmt->pos, "Switch statement has multiple default clauses");
                    }
                    has_default = true;
                }
                if ((switch_case.block.num_stmts) > (1)) {
                    ion_Stmt (*last_stmt) = switch_case.block.stmts[(switch_case.block.num_stmts) - (1)];
                    if ((last_stmt->kind) == (ION_STMT_BREAK)) {
                        ion_warning(last_stmt->pos, "Case blocks already end with an implicit break");
                    }
                }
                returns = (ion_resolve_stmt_block(switch_case.block, ret_type, ctx)) && (returns);
            }
            return (returns) && (has_default);
        }
        break;
    }
    case ION_STMT_ASSIGN: {
        ion_resolve_stmt_assign(stmt);
        return false;
        break;
    }
    case ION_STMT_INIT: {
        ion_resolve_stmt_init(stmt);
        return false;
        break;
    }
    case ION_STMT_EXPR: {
        ion_resolve_expr(stmt->expr);
        return false;
        break;
    }
    case ION_STMT_LABEL: {
        ion_define_label(stmt->pos, stmt->label);
        return false;
        break;
    }
    case ION_STMT_GOTO: {
        ion_reference_label(stmt->pos, stmt->label);
        return false;
        break;
    }
    default: {
        assert(0);
        return false;
        break;
    }
    }
}

void ion_resolve_func_body(ion_Sym (*sym)) {
    ion_Decl (*decl) = sym->decl;
    assert((decl->kind) == (ION_DECL_FUNC));
    assert((sym->state) == (ION_SYM_RESOLVED));
    if (decl->is_incomplete) {
        return;
    }
    ion_Package (*old_package) = ion_enter_package(sym->home_package);
    ion_Sym (*scope) = ion_sym_enter();
    for (size_t i = 0; (i) < (decl->function.num_params); (i)++) {
        ion_FuncParam param = decl->function.params[i];
        ion_Type (*param_type) = ion_resolve_typespec(param.type);
        if (ion_is_array_type(param_type)) {
            param_type = ion_type_ptr(param_type->base);
        }
        ion_sym_push_var(param.name, param_type);
    }
    ion_Type (*ret_type) = ion_resolve_typespec(decl->function.ret_type);
    assert(!(ion_is_array_type(ret_type)));
    bool returns = ion_resolve_stmt_block(decl->function.block, ret_type, (ion_StmtCtx){0});
    ion_resolve_labels();
    ion_sym_leave(scope);
    if (((ret_type) != (ion_type_void)) && (!(returns))) {
        ion_fatal_error(decl->pos, "Not all control paths return values");
    }
    ion_leave_package(old_package);
}

void ion_resolve_sym(ion_Sym (*sym)) {
    if ((sym->state) == (ION_SYM_RESOLVED)) {
        return;
    } else if ((sym->state) == (ION_SYM_RESOLVING)) {
        ion_fatal_error(sym->decl->pos, "Cyclic dependency");
        return;
    }
    assert((sym->state) == (ION_SYM_UNRESOLVED));
    assert(!(sym->reachable));
    if (!(ion_is_local_sym(sym))) {
        ion_buf_push((void (**))(&(ion_reachable_syms)), &(sym), sizeof(sym));
        sym->reachable = ion_reachable_phase;
    }
    sym->state = ION_SYM_RESOLVING;
    ion_Decl (*decl) = sym->decl;
    ion_Package (*old_package) = ion_enter_package(sym->home_package);
    switch (sym->kind) {
    case ION_SYM_TYPE: {
        if ((decl) && ((decl->kind) == (ION_DECL_TYPEDEF))) {
            sym->type = ion_resolve_typespec(decl->typedef_decl.type);
        } else if ((decl->kind) == (ION_DECL_ENUM)) {
            ion_Type (*base) = (decl->enum_decl.type ? ion_resolve_typespec(decl->enum_decl.type) : ion_type_int);
            if (!(ion_is_integer_type(base))) {
                ion_fatal_error(decl->pos, "Base type of enum must be integer type");
            }
            sym->type = ion_type_enum(sym, base);
        } else {
            sym->type = ion_type_incomplete(sym);
        }
        break;
    }
    case ION_SYM_VAR: {
        sym->type = ion_resolve_decl_var(decl);
        break;
    }
    case ION_SYM_CONST: {
        sym->type = ion_resolve_decl_const(decl, &(sym->val));
        break;
    }
    case ION_SYM_FUNC: {
        sym->type = ion_resolve_decl_func(decl);
        break;
    }
    case ION_SYM_PACKAGE: {
        break;
        break;
    }
    default: {
        assert(0);
        break;
    }
    }
    ion_leave_package(old_package);
    sym->state = ION_SYM_RESOLVED;
    if ((decl->is_incomplete) || ((((decl->kind) != (ION_DECL_STRUCT)) && ((decl->kind) != (ION_DECL_UNION))))) {
        ion_buf_push((void (**))(&(ion_sorted_syms)), &(sym), sizeof(sym));
    }
}

void ion_finalize_sym(ion_Sym (*sym)) {
    assert((sym->state) == (ION_SYM_RESOLVED));
    if (((sym->decl) && (!(ion_is_decl_foreign(sym->decl)))) && (!(sym->decl->is_incomplete))) {
        if ((sym->kind) == (ION_SYM_TYPE)) {
            ion_complete_type(sym->type);
        } else if ((sym->kind) == (ION_SYM_FUNC)) {
            ion_resolve_func_body(sym);
        }
    }
}

ion_Sym (*ion_resolve_name(char const ((*name)))) {
    ion_Sym (*sym) = ion_sym_get(name);
    if (!(sym)) {
        return NULL;
    }
    ion_resolve_sym(sym);
    return sym;
}

ion_Package (*ion_try_resolve_package(ion_Expr (*expr))) {
    if ((expr->kind) == (ION_EXPR_NAME)) {
        ion_Sym (*sym) = ion_resolve_name(expr->name);
        if ((sym) && ((sym->kind) == (ION_SYM_PACKAGE))) {
            return sym->package;
        }
    } else if ((expr->kind) == (ION_EXPR_FIELD)) {
        ion_Package (*package) = ion_try_resolve_package(expr->field.expr);
        if (package) {
            ion_Sym (*sym) = ion_get_package_sym(package, expr->field.name);
            if ((sym) && ((sym->kind) == (ION_SYM_PACKAGE))) {
                return sym->package;
            }
        }
    }
    return NULL;
}

ion_Operand ion_resolve_expr_field(ion_Expr (*expr)) {
    assert((expr->kind) == (ION_EXPR_FIELD));
    ion_Package (*package) = ion_try_resolve_package(expr->field.expr);
    if (package) {
        ion_Package (*old_package) = ion_enter_package(package);
        ion_Sym (*sym) = ion_resolve_name(expr->field.name);
        ion_Operand operand = ion_resolve_name_operand(expr->pos, expr->field.name);
        ion_leave_package(old_package);
        ion_set_resolved_sym(expr, sym);
        return operand;
    }
    ion_Operand operand = ion_resolve_expr(expr->field.expr);
    bool was_const_type = ion_is_const_type(operand.type);
    ion_Type (*type) = ion_unqualify_type(operand.type);
    ion_complete_type(type);
    if (ion_is_ptr_type(type)) {
        operand = ion_operand_lvalue(type->base);
        was_const_type = ion_is_const_type(operand.type);
        type = ion_unqualify_type(operand.type);
        ion_complete_type(type);
    }
    if (((type->kind) != (ION_CMPL_TYPE_STRUCT)) && ((type->kind) != (ION_CMPL_TYPE_UNION))) {
        ion_fatal_error(expr->pos, "Can only access fields on aggregates or pointers to aggregates");
        return ion_operand_null;
    }
    for (size_t i = 0; (i) < (type->aggregate.num_fields); (i)++) {
        ion_TypeField field = type->aggregate.fields[i];
        if ((field.name) == (expr->field.name)) {
            ion_Operand field_operand = (operand.is_lvalue ? ion_operand_lvalue(field.type) : ion_operand_rvalue(field.type));
            if (was_const_type) {
                field_operand.type = ion_type_const(field_operand.type);
            }
            return field_operand;
        }
    }
    ion_fatal_error(expr->pos, "No field named \'%s\'", expr->field.name);
    return ion_operand_null;
}

llong ion_eval_unary_op_ll(ion_TokenKind op, llong val) {
    switch (op) {
    case ION_TOKEN_ADD: {
        return +(val);
        break;
    }
    case ION_TOKEN_SUB: {
        return -(val);
        break;
    }
    case ION_TOKEN_NEG: {
        return ~(val);
        break;
    }
    case ION_TOKEN_NOT: {
        return !(val);
        break;
    }
    default: {
        assert(0);
        break;
    }
    }
    return 0;
}

ullong ion_eval_unary_op_ull(ion_TokenKind op, ullong val) {
    switch (op) {
    case ION_TOKEN_ADD: {
        return +(val);
        break;
    }
    case ION_TOKEN_SUB: {
        return (0ull) - (val);
        break;
    }
    case ION_TOKEN_NEG: {
        return ~(val);
        break;
    }
    case ION_TOKEN_NOT: {
        return !(val);
        break;
    }
    default: {
        assert(0);
        break;
    }
    }
    return 0;
}

llong ion_eval_binary_op_ll(ion_TokenKind op, llong left, llong right) {
    switch (op) {
    case ION_TOKEN_MUL: {
        return (left) * (right);
        break;
    }
    case ION_TOKEN_DIV: {
        return ((right) != (0) ? (left) / (right) : 0);
        break;
    }
    case ION_TOKEN_MOD: {
        return ((right) != (0) ? (left) % (right) : 0);
        break;
    }
    case ION_TOKEN_AND: {
        return (left) & (right);
        break;
    }
    case ION_TOKEN_LSHIFT: {
        return (left) << (right);
        break;
    }
    case ION_TOKEN_RSHIFT: {
        return (left) >> (right);
        break;
    }
    case ION_TOKEN_ADD: {
        return (left) + (right);
        break;
    }
    case ION_TOKEN_SUB: {
        return (left) - (right);
        break;
    }
    case ION_TOKEN_OR: {
        return (left) | (right);
        break;
    }
    case ION_TOKEN_XOR: {
        return (left) ^ (right);
        break;
    }
    case ION_TOKEN_EQ: {
        return (left) == (right);
        break;
    }
    case ION_TOKEN_NOTEQ: {
        return (left) != (right);
        break;
    }
    case ION_TOKEN_LT: {
        return (left) < (right);
        break;
    }
    case ION_TOKEN_LTEQ: {
        return (left) <= (right);
        break;
    }
    case ION_TOKEN_GT: {
        return (left) > (right);
        break;
    }
    case ION_TOKEN_GTEQ: {
        return (left) >= (right);
        break;
    }
    default: {
        assert(0);
        break;
    }
    }
    return 0;
}

ullong ion_eval_binary_op_ull(ion_TokenKind op, ullong left, ullong right) {
    switch (op) {
    case ION_TOKEN_MUL: {
        return (left) * (right);
        break;
    }
    case ION_TOKEN_DIV: {
        return ((right) != (0) ? (left) / (right) : 0);
        break;
    }
    case ION_TOKEN_MOD: {
        return ((right) != (0) ? (left) % (right) : 0);
        break;
    }
    case ION_TOKEN_AND: {
        return (left) & (right);
        break;
    }
    case ION_TOKEN_LSHIFT: {
        return (left) << (right);
        break;
    }
    case ION_TOKEN_RSHIFT: {
        return (left) >> (right);
        break;
    }
    case ION_TOKEN_ADD: {
        return (left) + (right);
        break;
    }
    case ION_TOKEN_SUB: {
        return (left) - (right);
        break;
    }
    case ION_TOKEN_OR: {
        return (left) | (right);
        break;
    }
    case ION_TOKEN_XOR: {
        return (left) ^ (right);
        break;
    }
    case ION_TOKEN_EQ: {
        return (left) == (right);
        break;
    }
    case ION_TOKEN_NOTEQ: {
        return (left) != (right);
        break;
    }
    case ION_TOKEN_LT: {
        return (left) < (right);
        break;
    }
    case ION_TOKEN_LTEQ: {
        return (left) <= (right);
        break;
    }
    case ION_TOKEN_GT: {
        return (left) > (right);
        break;
    }
    case ION_TOKEN_GTEQ: {
        return (left) >= (right);
        break;
    }
    default: {
        assert(0);
        break;
    }
    }
    return 0;
}

ion_Val ion_eval_unary_op(ion_TokenKind op, ion_Type (*type), ion_Val val) {
    if (ion_is_integer_type(type)) {
        ion_Operand operand = ion_operand_const(type, val);
        if (ion_is_signed_type(type)) {
            cast_operand(&(operand), ion_type_llong);
            operand.val.ll = ion_eval_unary_op_ll(op, operand.val.ll);
        } else {
            cast_operand(&(operand), ion_type_ullong);
            operand.val.ll = ion_eval_unary_op_ull(op, operand.val.ull);
        }
        cast_operand(&(operand), type);
        return operand.val;
    } else {
        return (ion_Val){0};
    }
}

ion_Val ion_eval_binary_op(ion_TokenKind op, ion_Type (*type), ion_Val left, ion_Val right) {
    if (ion_is_integer_type(type)) {
        ion_Operand left_operand = ion_operand_const(type, left);
        ion_Operand right_operand = ion_operand_const(type, right);
        ion_Operand result_operand = {0};
        if (ion_is_signed_type(type)) {
            cast_operand(&(left_operand), ion_type_llong);
            cast_operand(&(right_operand), ion_type_llong);
            result_operand = ion_operand_const(ion_type_llong, (ion_Val){.ll = ion_eval_binary_op_ll(op, left_operand.val.ll, right_operand.val.ll)});
        } else {
            cast_operand(&(left_operand), ion_type_ullong);
            cast_operand(&(right_operand), ion_type_ullong);
            result_operand = ion_operand_const(ion_type_ullong, (ion_Val){.ull = ion_eval_binary_op_ull(op, left_operand.val.ull, right_operand.val.ull)});
        }
        cast_operand(&(result_operand), type);
        return result_operand.val;
    } else {
        return (ion_Val){0};
    }
}

ion_Operand ion_resolve_name_operand(ion_SrcPos pos, char const ((*name))) {
    ion_Sym (*sym) = ion_resolve_name(name);
    if (!(sym)) {
        ion_fatal_error(pos, "Unresolved name \'%s\'", name);
    }
    if ((sym->kind) == (ION_SYM_VAR)) {
        ion_Operand operand = ion_operand_lvalue(sym->type);
        if (ion_is_array_type(operand.type)) {
            operand = ion_operand_decay(operand);
        }
        return operand;
    } else if ((sym->kind) == (ION_SYM_CONST)) {
        return ion_operand_const(sym->type, sym->val);
    } else if ((sym->kind) == (ION_SYM_FUNC)) {
        return ion_operand_rvalue(sym->type);
    } else {
        ion_fatal_error(pos, "%s must be a var or const", name);
        return ion_operand_null;
    }
}

ion_Operand ion_resolve_expr_name(ion_Expr (*expr)) {
    assert((expr->kind) == (ION_EXPR_NAME));
    return ion_resolve_name_operand(expr->pos, expr->name);
}

ion_Operand ion_resolve_unary_op(ion_TokenKind op, ion_Operand operand) {
    ion_promote_operand(&(operand));
    if (operand.is_const) {
        return ion_operand_const(operand.type, ion_eval_unary_op(op, operand.type, operand.val));
    } else {
        return operand;
    }
}

ion_Operand ion_resolve_expr_unary(ion_Expr (*expr)) {
    ion_Operand operand = ion_resolve_expr_rvalue(expr->unary.expr);
    ion_Type (*type) = operand.type;
    switch (expr->unary.op) {
    case ION_TOKEN_MUL: {
        if (!(ion_is_ptr_type(type))) {
            ion_fatal_error(expr->pos, "Cannot deref non-ptr type");
        }
        return ion_operand_lvalue(type->base);
        break;
    }
    case ION_TOKEN_ADD:
    case ION_TOKEN_SUB: {
        if (!(ion_is_arithmetic_type(type))) {
            ion_fatal_error(expr->pos, "Can only use unary %s with arithmetic types", ion_token_kind_name(expr->unary.op));
        }
        return ion_resolve_unary_op(expr->unary.op, operand);
        break;
    }
    case ION_TOKEN_NEG: {
        if (!(ion_is_integer_type(type))) {
            ion_fatal_error(expr->pos, "Can only use ~ with integer types");
        }
        return ion_resolve_unary_op(expr->unary.op, operand);
        break;
    }
    case ION_TOKEN_NOT: {
        if (!(ion_is_scalar_type(type))) {
            ion_fatal_error(expr->pos, " Can only use ! with scalar types");
        }
        return ion_resolve_unary_op(expr->unary.op, operand);
        break;
    }
    default: {
        assert(0);
        break;
    }
    }
    return (ion_Operand){0};
}

ion_Operand ion_resolve_binary_op(ion_TokenKind op, ion_Operand left, ion_Operand right) {
    if ((left.is_const) && (right.is_const)) {
        return ion_operand_const(left.type, ion_eval_binary_op(op, left.type, left.val, right.val));
    } else {
        return ion_operand_rvalue(left.type);
    }
}

ion_Operand ion_resolve_binary_arithmetic_op(ion_TokenKind op, ion_Operand left, ion_Operand right) {
    ion_unify_arithmetic_operands(&(left), &(right));
    return ion_resolve_binary_op(op, left, right);
}

ion_Operand ion_resolve_expr_binary_op(ion_TokenKind op, char const ((*op_name)), ion_SrcPos pos, ion_Operand left, ion_Operand right) {
    switch (op) {
    case ION_TOKEN_MUL:
    case ION_TOKEN_DIV: {
        if (!(ion_is_arithmetic_type(left.type))) {
            ion_fatal_error(pos, "Left operand of %s must have arithmetic type", op_name);
        }
        if (!(ion_is_arithmetic_type(right.type))) {
            ion_fatal_error(pos, "Right operand of %s must have arithmetic type", op_name);
        }
        return ion_resolve_binary_arithmetic_op(op, left, right);
        break;
    }
    case ION_TOKEN_MOD: {
        if (!(ion_is_integer_type(left.type))) {
            ion_fatal_error(pos, "Left operand of %% must have integer type");
        }
        if (!(ion_is_integer_type(right.type))) {
            ion_fatal_error(pos, "Right operand of %% must have integer type");
        }
        return ion_resolve_binary_arithmetic_op(op, left, right);
        break;
    }
    case ION_TOKEN_ADD: {
        if ((ion_is_arithmetic_type(left.type)) && (ion_is_arithmetic_type(right.type))) {
            return ion_resolve_binary_arithmetic_op(op, left, right);
        } else if ((ion_is_ptr_type(left.type)) && (ion_is_integer_type(right.type))) {
            ion_complete_type(left.type->base);
            if ((ion_type_sizeof(left.type->base)) == (0)) {
                ion_fatal_error(pos, "Cannot do pointer arithmetic with size 0 base type");
            }
            return ion_operand_rvalue(left.type);
        } else if ((ion_is_ptr_type(right.type)) && (ion_is_integer_type(left.type))) {
            ion_complete_type(right.type->base);
            if ((ion_type_sizeof(right.type->base)) == (0)) {
                ion_fatal_error(pos, "Cannot do pointer arithmetic with size 0 base type");
            }
            return ion_operand_rvalue(right.type);
        } else {
            ion_fatal_error(pos, "Operands of + must both have arithmetic type, or pointer and integer type");
        }
        break;
    }
    case ION_TOKEN_SUB: {
        if ((ion_is_arithmetic_type(left.type)) && (ion_is_arithmetic_type(right.type))) {
            return ion_resolve_binary_arithmetic_op(op, left, right);
        } else if ((ion_is_ptr_type(left.type)) && (ion_is_integer_type(right.type))) {
            return ion_operand_rvalue(left.type);
        } else if ((ion_is_ptr_type(left.type)) && (ion_is_ptr_type(right.type))) {
            if ((left.type->base) != (right.type->base)) {
                ion_fatal_error(pos, "Cannot subtract pointers to different types");
            }
            return ion_operand_rvalue(ion_type_ssize);
        } else {
            ion_fatal_error(pos, "Operands of - must both have arithmetic type, pointer and integer type, or compatible pointer types");
        }
        break;
    }
    case ION_TOKEN_LSHIFT:
    case ION_TOKEN_RSHIFT: {
        if ((ion_is_integer_type(left.type)) && (ion_is_integer_type(right.type))) {
            ion_promote_operand(&(left));
            ion_promote_operand(&(right));
            ion_Type (*result_type) = left.type;
            ion_Operand result = {0};
            if (ion_is_signed_type(left.type)) {
                cast_operand(&(left), ion_type_llong);
                cast_operand(&(right), ion_type_llong);
            } else {
                cast_operand(&(left), ion_type_ullong);
                cast_operand(&(right), ion_type_ullong);
            }
            result = ion_resolve_binary_op(op, left, right);
            cast_operand(&(result), result_type);
            return result;
        } else {
            ion_fatal_error(pos, "Operands of %s must both have integer type", op_name);
        }
        break;
    }
    case ION_TOKEN_EQ:
    case ION_TOKEN_NOTEQ: {
        if ((ion_is_arithmetic_type(left.type)) && (ion_is_arithmetic_type(right.type))) {
            ion_Operand result = ion_resolve_binary_arithmetic_op(op, left, right);
            cast_operand(&(result), ion_type_int);
            return result;
        } else if ((ion_is_ptr_type(left.type)) && (ion_is_ptr_type(right.type))) {
            ion_Type (*unqual_left_base) = ion_unqualify_type(left.type->base);
            ion_Type (*unqual_right_base) = ion_unqualify_type(right.type->base);
            if ((((unqual_left_base) != (unqual_right_base)) && ((unqual_left_base) != (ion_type_void))) && ((unqual_right_base) != (ion_type_void))) {
                ion_fatal_error(pos, "Cannot compare pointers to different types");
            }
            return ion_operand_rvalue(ion_type_int);
        } else if ((((ion_is_null_ptr(left)) && (ion_is_ptr_type(right.type)))) || (((ion_is_null_ptr(right)) && (ion_is_ptr_type(left.type))))) {
            return ion_operand_rvalue(ion_type_int);
        } else {
            ion_fatal_error(pos, "Operands of %s must be arithmetic types or compatible pointer types", op_name);
        }
        break;
    }
    case ION_TOKEN_LT:
    case ION_TOKEN_LTEQ:
    case ION_TOKEN_GT:
    case ION_TOKEN_GTEQ: {
        if ((ion_is_arithmetic_type(left.type)) && (ion_is_arithmetic_type(right.type))) {
            ion_Operand result = ion_resolve_binary_arithmetic_op(op, left, right);
            cast_operand(&(result), ion_type_int);
            return result;
        } else if ((ion_is_ptr_type(left.type)) && (ion_is_ptr_type(right.type))) {
            if ((ion_unqualify_type(left.type->base)) != (ion_unqualify_type(right.type->base))) {
                ion_fatal_error(pos, "Cannot compare pointers to different types");
            }
            return ion_operand_rvalue(ion_type_int);
        } else if ((((ion_is_null_ptr(left)) && (ion_is_ptr_type(right.type)))) || (((ion_is_null_ptr(right)) && (ion_is_ptr_type(left.type))))) {
            return ion_operand_rvalue(ion_type_int);
        } else {
            ion_fatal_error(pos, "Operands of %s must be arithmetic types or compatible pointer types", op_name);
        }
        break;
    }
    case ION_TOKEN_AND:
    case ION_TOKEN_XOR:
    case ION_TOKEN_OR: {
        if ((ion_is_integer_type(left.type)) && (ion_is_integer_type(right.type))) {
            return ion_resolve_binary_arithmetic_op(op, left, right);
        } else {
            ion_fatal_error(pos, "Operands of %s must have arithmetic types", op_name);
        }
        break;
    }
    case ION_TOKEN_AND_AND:
    case ION_TOKEN_OR_OR: {
        if ((ion_is_scalar_type(left.type)) && (ion_is_scalar_type(right.type))) {
            if ((left.is_const) && (right.is_const)) {
                cast_operand(&(left), ion_type_bool);
                cast_operand(&(right), ion_type_bool);
                int i = {0};
                if ((op) == (ION_TOKEN_AND_AND)) {
                    i = (left.val.b) && (right.val.b);
                } else {
                    assert((op) == (ION_TOKEN_OR_OR));
                    i = (left.val.b) || (right.val.b);
                }
                return ion_operand_const(ion_type_int, (ion_Val){.i = i});
            } else {
                return ion_operand_rvalue(ion_type_int);
            }
        } else {
            ion_fatal_error(pos, "Operands of %s must have scalar types", op_name);
        }
        break;
    }
    default: {
        assert(0);
        break;
    }
    }
    return (ion_Operand){0};
}

ion_Operand ion_resolve_expr_binary(ion_Expr (*expr)) {
    assert((expr->kind) == (ION_EXPR_BINARY));
    ion_Operand left = ion_resolve_expr_rvalue(expr->binary.left);
    ion_Operand right = ion_resolve_expr_rvalue(expr->binary.right);
    ion_TokenKind op = expr->binary.op;
    char const ((*op_name)) = ion_token_kind_name(op);
    return ion_resolve_expr_binary_op(op, op_name, expr->pos, left, right);
}

ion_Operand ion_resolve_expr_compound(ion_Expr (*expr), ion_Type (*expected_type)) {
    assert((expr->kind) == (ION_EXPR_COMPOUND));
    if ((!(expected_type)) && (!(expr->compound.type))) {
        ion_fatal_error(expr->pos, "Implicitly typed compound literals used in context without expected type");
    }
    ion_Type (*type) = NULL;
    if (expr->compound.type) {
        type = ion_resolve_typespec(expr->compound.type);
    } else {
        type = expected_type;
    }
    ion_complete_type(type);
    bool is_const = ion_is_const_type(type);
    type = ion_unqualify_type(type);
    if (((type->kind) == (ION_CMPL_TYPE_STRUCT)) || ((type->kind) == (ION_CMPL_TYPE_UNION))) {
        int index = 0;
        for (size_t i = 0; (i) < (expr->compound.num_fields); (i)++) {
            ion_CompoundField field = expr->compound.fields[i];
            if ((field.kind) == (ION_FIELD_INDEX)) {
                ion_fatal_error(field.pos, "Index field initializer not allowed for struct/union compound literal");
            } else if ((field.kind) == (ION_FIELD_NAME)) {
                index = ion_aggregate_item_field_index(type, field.name);
                if ((index) == (-(1))) {
                    ion_fatal_error(field.pos, "Named field in compound literal does not exist");
                }
            }
            if ((index) >= ((int)(type->aggregate.num_fields))) {
                ion_fatal_error(field.pos, "Field initializer in struct/union compound literal out of range");
            }
            ion_Type (*field_type) = type->aggregate.fields[index].type;
            if (!(ion_resolve_typed_init(field.pos, field_type, field.init))) {
                ion_fatal_error(field.pos, "Invalid type in compound literal initializer for aggregate type. Expected %s", ion_get_type_name(field_type));
            }
            (index)++;
        }
    } else if ((type->kind) == (ION_CMPL_TYPE_ARRAY)) {
        size_t index = 0;
        size_t max_index = 0;
        for (size_t i = 0; (i) < (expr->compound.num_fields); (i)++) {
            ion_CompoundField field = expr->compound.fields[i];
            if ((field.kind) == (ION_FIELD_NAME)) {
                ion_fatal_error(field.pos, "Named field initializer not allowed for array compound literals");
            } else if ((field.kind) == (ION_FIELD_INDEX)) {
                ion_Operand operand = ion_resolve_const_expr(field.index);
                if (!(ion_is_integer_type(operand.type))) {
                    ion_fatal_error(field.pos, "Field initializer index expression must have type int");
                }
                if (!(cast_operand(&(operand), ion_type_int))) {
                    ion_fatal_error(field.pos, "Invalid type in field initializer index. Expected integer type");
                }
                if ((operand.val.i) < (0)) {
                    ion_fatal_error(field.pos, "Field initializer index cannot be negative");
                }
                index = operand.val.i;
            }
            if ((type->num_elems) && ((index) >= ((int)(type->num_elems)))) {
                ion_fatal_error(field.pos, "Field initializer in array compound literal out of range");
            }
            if (!(ion_resolve_typed_init(field.pos, type->base, field.init))) {
                ion_fatal_error(field.pos, "Invalid type in compound literal initializer for array type. Expected %s", ion_get_type_name(type->base));
            }
            max_index = ion_max(max_index, index);
            (index)++;
        }
        if ((type->num_elems) == (0)) {
            type = ion_type_array(type->base, (max_index) + (1));
        }
    } else {
        assert(ion_is_scalar_type(type));
        if ((expr->compound.num_fields) > (1)) {
            ion_fatal_error(expr->pos, "Compound literal for scalar type cannot have more than one operand");
        }
        if ((expr->compound.num_fields) == (1)) {
            ion_CompoundField field = expr->compound.fields[0];
            ion_Operand init = ion_resolve_expected_expr_rvalue(field.init, type);
            if (!(ion_convert_operand(&(init), type))) {
                ion_fatal_error(field.pos, "Invalid type in compound literal initializer. Expected %s, got %s", ion_get_type_name(type), ion_get_type_name(init.type));
            }
        }
    }
    return ion_operand_lvalue((is_const ? ion_type_const(type) : type));
}

ion_Operand ion_resolve_expr_call(ion_Expr (*expr)) {
    assert((expr->kind) == (ION_EXPR_CALL));
    if ((expr->call.expr->kind) == (ION_EXPR_NAME)) {
        ion_Sym (*sym) = ion_resolve_name(expr->call.expr->name);
        if ((sym) && ((sym->kind) == (ION_SYM_TYPE))) {
            if ((expr->call.num_args) != (1)) {
                ion_fatal_error(expr->pos, "Type conversion operator takes 1 argument");
            }
            ion_Operand operand = ion_resolve_expr_rvalue(expr->call.args[0]);
            if (!(cast_operand(&(operand), sym->type))) {
                ion_fatal_error(expr->pos, "Invalid type cast from %s to %s", ion_get_type_name(operand.type), ion_get_type_name(sym->type));
            }
            ion_set_resolved_sym(expr->call.expr, sym);
            return operand;
        }
    }
    ion_Operand function = ion_resolve_expr_rvalue(expr->call.expr);
    if ((function.type->kind) != (ION_CMPL_TYPE_FUNC)) {
        ion_fatal_error(expr->pos, "Cannot call non-function value");
    }
    size_t num_params = function.type->function.num_params;
    if ((expr->call.num_args) < (num_params)) {
        ion_fatal_error(expr->pos, "Function call with too few arguments");
    }
    if (((expr->call.num_args) > (num_params)) && (!(function.type->function.has_varargs))) {
        ion_fatal_error(expr->pos, "Function call with too many arguments");
    }
    for (size_t i = 0; (i) < (num_params); (i)++) {
        ion_Type (*param_type) = function.type->function.params[i];
        ion_Operand arg = ion_resolve_expected_expr_rvalue(expr->call.args[i], param_type);
        if (ion_is_array_type(param_type)) {
            param_type = ion_type_ptr(param_type->base);
        }
        if (!(ion_convert_operand(&(arg), param_type))) {
            ion_fatal_error(expr->call.args[i]->pos, "Invalid type in function call argument. Expected %s, got %s", ion_get_type_name(param_type), ion_get_type_name(arg.type));
        }
    }
    for (size_t i = num_params; (i) < (expr->call.num_args); (i)++) {
        ion_resolve_expr_rvalue(expr->call.args[i]);
    }
    return ion_operand_rvalue(function.type->function.ret);
}

ion_Operand ion_resolve_expr_ternary(ion_Expr (*expr), ion_Type (*expected_type)) {
    assert((expr->kind) == (ION_EXPR_TERNARY));
    ion_Operand cond = ion_resolve_expr_rvalue(expr->ternary.cond);
    if (!(ion_is_scalar_type(cond.type))) {
        ion_fatal_error(expr->pos, "Ternary conditional must have scalar type");
    }
    ion_Operand left = ion_resolve_expected_expr_rvalue(expr->ternary.then_expr, expected_type);
    ion_Operand right = ion_resolve_expected_expr_rvalue(expr->ternary.else_expr, expected_type);
    if ((left.type) == (right.type)) {
        return ion_operand_rvalue(left.type);
    } else if ((ion_is_arithmetic_type(left.type)) && (ion_is_arithmetic_type(right.type))) {
        ion_unify_arithmetic_operands(&(left), &(right));
        if (((cond.is_const) && (left.is_const)) && (right.is_const)) {
            return ion_operand_const(left.type, (cond.val.i ? left.val : right.val));
        } else {
            return ion_operand_rvalue(left.type);
        }
    } else if ((ion_is_ptr_type(left.type)) && (ion_is_null_ptr(right))) {
        return ion_operand_rvalue(left.type);
    } else if ((ion_is_ptr_type(right.type)) && (ion_is_null_ptr(left))) {
        return ion_operand_rvalue(right.type);
    } else {
        ion_fatal_error(expr->pos, "Left and right operands of ternary expression must have arithmetic types or identical types");
    }
    return (ion_Operand){0};
}

ion_Operand ion_resolve_expr_index(ion_Expr (*expr)) {
    assert((expr->kind) == (ION_EXPR_INDEX));
    ion_Operand operand = ion_resolve_expr_rvalue(expr->index.expr);
    if (!(ion_is_ptr_type(operand.type))) {
        ion_fatal_error(expr->pos, "Can only index arrays and pointers");
    }
    ion_Operand index = ion_resolve_expr_rvalue(expr->index.index);
    if (!(ion_is_integer_type(index.type))) {
        ion_fatal_error(expr->pos, "Index must have integer type");
    }
    return ion_operand_lvalue(operand.type->base);
}

ion_Operand ion_resolve_expr_cast(ion_Expr (*expr)) {
    assert((expr->kind) == (ION_EXPR_CAST));
    ion_Type (*type) = ion_resolve_typespec(expr->cast.type);
    ion_Operand operand = ion_resolve_expr_rvalue(expr->cast.expr);
    if (!(cast_operand(&(operand), type))) {
        ion_fatal_error(expr->pos, "Invalid type cast from %s to %s", ion_get_type_name(operand.type), ion_get_type_name(type));
    }
    return operand;
}

ion_Operand ion_resolve_expr_int(ion_Expr (*expr)) {
    assert((expr->kind) == (ION_EXPR_INT));
    ullong int_max = ion_type_metrics[ION_CMPL_TYPE_INT].max;
    ullong uint_max = ion_type_metrics[ION_CMPL_TYPE_UINT].max;
    ullong long_max = ion_type_metrics[ION_CMPL_TYPE_LONG].max;
    ullong ulong_max = ion_type_metrics[ION_CMPL_TYPE_ULONG].max;
    ullong llong_max = ion_type_metrics[ION_CMPL_TYPE_LLONG].max;
    ullong val = expr->int_lit.val;
    ion_Operand operand = ion_operand_const(ion_type_ullong, (ion_Val){.ull = val});
    ion_Type (*type) = ion_type_ullong;
    if ((expr->int_lit.mod) == (ION_MOD_NONE)) {
        bool overflow = false;
        switch (expr->int_lit.suffix) {
        case ION_SUFFIX_NONE: {
            type = ion_type_int;
            if ((val) > (int_max)) {
                type = ion_type_long;
                if ((val) > (long_max)) {
                    type = ion_type_llong;
                    overflow = (val) > (llong_max);
                }
            }
            break;
        }
        case ION_SUFFIX_U: {
            type = ion_type_uint;
            if ((val) > (uint_max)) {
                type = ion_type_ulong;
                if ((val) > (ulong_max)) {
                    type = ion_type_ullong;
                }
            }
            break;
        }
        case ION_SUFFIX_L: {
            type = ion_type_long;
            if ((val) > (long_max)) {
                type = ion_type_llong;
                overflow = (val) > (llong_max);
            }
            break;
        }
        case ION_SUFFIX_UL: {
            type = ion_type_ulong;
            if ((val) > (ulong_max)) {
                type = ion_type_ullong;
            }
            break;
        }
        case ION_SUFFIX_LL: {
            type = ion_type_llong;
            overflow = (val) > (llong_max);
            break;
        }
        case ION_SUFFIX_ULL: {
            type = ion_type_ullong;
            break;
        }
        default: {
            assert(0);
            break;
        }
        }
        if (overflow) {
            ion_fatal_error(expr->pos, "Integer literal overflow");
        }
    } else {
        switch (expr->int_lit.suffix) {
        case ION_SUFFIX_NONE: {
            type = ion_type_int;
            if ((val) > (int_max)) {
                type = ion_type_uint;
                if ((val) > (uint_max)) {
                    type = ion_type_long;
                    if ((val) > (long_max)) {
                        type = ion_type_ulong;
                        if ((val) > (ulong_max)) {
                            type = ion_type_llong;
                            if ((val) > (llong_max)) {
                                type = ion_type_ullong;
                            }
                        }
                    }
                }
            }
            break;
        }
        case ION_SUFFIX_U: {
            type = ion_type_uint;
            if ((val) > (uint_max)) {
                type = ion_type_ulong;
                if ((val) > (ulong_max)) {
                    type = ion_type_ullong;
                }
            }
            break;
        }
        case ION_SUFFIX_L: {
            type = ion_type_long;
            if ((val) > (long_max)) {
                type = ion_type_ulong;
                if ((val) > (ulong_max)) {
                    type = ion_type_llong;
                    if ((val) > (llong_max)) {
                        type = ion_type_ullong;
                    }
                }
            }
            break;
        }
        case ION_SUFFIX_UL: {
            type = ion_type_ulong;
            if ((val) > (ulong_max)) {
                type = ion_type_ullong;
            }
            break;
        }
        case ION_SUFFIX_LL: {
            type = ion_type_llong;
            if ((val) > (llong_max)) {
                type = ion_type_ullong;
            }
            break;
        }
        case ION_SUFFIX_ULL: {
            type = ion_type_ullong;
            break;
        }
        default: {
            assert(0);
            break;
        }
        }
    }
    cast_operand(&(operand), type);
    return operand;
}

ion_Operand ion_resolve_expr_modify(ion_Expr (*expr)) {
    ion_Operand operand = ion_resolve_expr(expr->modify.expr);
    ion_Type (*type) = operand.type;
    ion_complete_type(type);
    if (!(operand.is_lvalue)) {
        ion_fatal_error(expr->pos, "Cannot modify non-lvalue");
    }
    if (type->nonmodifiable) {
        ion_fatal_error(expr->pos, "Cannot modify non-modifiable type");
    }
    if (!(((ion_is_integer_type(type)) || ((type->kind) == (ION_CMPL_TYPE_PTR))))) {
        ion_fatal_error(expr->pos, "%s only valid for integer and pointer types", ion_token_kind_name(expr->modify.op));
    }
    return ion_operand_rvalue(type);
}

ion_Operand ion_resolve_expected_expr(ion_Expr (*expr), ion_Type (*expected_type)) {
    ion_Operand result = {0};
    switch (expr->kind) {
    case ION_EXPR_PAREN: {
        result = ion_resolve_expected_expr(expr->paren.expr, expected_type);
        break;
    }
    case ION_EXPR_INT: {
        result = ion_resolve_expr_int(expr);
        break;
    }
    case ION_EXPR_FLOAT: {
        result = ion_operand_const(((expr->float_lit.suffix) == (ION_SUFFIX_D) ? ion_type_double : ion_type_float), (ion_Val){0});
        break;
    }
    case ION_EXPR_STR: {
        result = ion_operand_rvalue(ion_type_array(ion_type_char, (strlen(expr->str_lit.val)) + (1)));
        break;
    }
    case ION_EXPR_NAME: {
        result = ion_resolve_expr_name(expr);
        ion_set_resolved_sym(expr, ion_resolve_name(expr->name));
        break;
    }
    case ION_EXPR_CAST: {
        result = ion_resolve_expr_cast(expr);
        break;
    }
    case ION_EXPR_CALL: {
        result = ion_resolve_expr_call(expr);
        break;
    }
    case ION_EXPR_INDEX: {
        result = ion_resolve_expr_index(expr);
        break;
    }
    case ION_EXPR_FIELD: {
        result = ion_resolve_expr_field(expr);
        break;
    }
    case ION_EXPR_COMPOUND: {
        result = ion_resolve_expr_compound(expr, expected_type);
        break;
    }
    case ION_EXPR_UNARY: {
        if ((expr->unary.op) == (ION_TOKEN_AND)) {
            ion_Operand operand = {0};
            if ((expected_type) && (ion_is_ptr_type(expected_type))) {
                operand = ion_resolve_expected_expr(expr->unary.expr, expected_type->base);
            } else {
                operand = ion_resolve_expr(expr->unary.expr);
            }
            if (!(operand.is_lvalue)) {
                ion_fatal_error(expr->pos, "Cannot take address of non-lvalue");
            }
            result = ion_operand_rvalue(ion_type_ptr(operand.type));
        } else {
            result = ion_resolve_expr_unary(expr);
        }
        break;
    }
    case ION_EXPR_BINARY: {
        result = ion_resolve_expr_binary(expr);
        break;
    }
    case ION_EXPR_TERNARY: {
        result = ion_resolve_expr_ternary(expr, expected_type);
        break;
    }
    case ION_EXPR_SIZEOF_EXPR: {
        {
            if ((expr->sizeof_expr->kind) == (ION_EXPR_NAME)) {
                ion_Sym (*sym) = ion_resolve_name(expr->sizeof_expr->name);
                if ((sym) && ((sym->kind) == (ION_SYM_TYPE))) {
                    ion_complete_type(sym->type);
                    result = ion_operand_const(ion_type_usize, (ion_Val){.ull = ion_type_sizeof(sym->type)});
                    ion_set_resolved_type(expr->sizeof_expr, sym->type);
                    ion_set_resolved_sym(expr->sizeof_expr, sym);
                    break;
                }
            }
            ion_Type (*type) = ion_resolve_expr(expr->sizeof_expr).type;
            ion_complete_type(type);
            result = ion_operand_const(ion_type_usize, (ion_Val){.ull = ion_type_sizeof(type)});
        }
        break;
    }
    case ION_EXPR_SIZEOF_TYPE: {
        {
            ion_Type (*type) = ion_resolve_typespec(expr->sizeof_type);
            ion_complete_type(type);
            result = ion_operand_const(ion_type_usize, (ion_Val){.ull = ion_type_sizeof(type)});
        }
        break;
    }
    case ION_EXPR_ALIGNOF_EXPR: {
        {
            if ((expr->sizeof_expr->kind) == (ION_EXPR_NAME)) {
                ion_Sym (*sym) = ion_resolve_name(expr->alignof_expr->name);
                if ((sym) && ((sym->kind) == (ION_SYM_TYPE))) {
                    ion_complete_type(sym->type);
                    result = ion_operand_const(ion_type_usize, (ion_Val){.ull = ion_type_alignof(sym->type)});
                    ion_set_resolved_type(expr->alignof_expr, sym->type);
                    ion_set_resolved_sym(expr->alignof_expr, sym);
                    break;
                }
            }
            ion_Type (*type) = ion_resolve_expr(expr->alignof_expr).type;
            ion_complete_type(type);
            result = ion_operand_const(ion_type_usize, (ion_Val){.ull = ion_type_alignof(type)});
        }
        break;
    }
    case ION_EXPR_ALIGNOF_TYPE: {
        {
            ion_Type (*type) = ion_resolve_typespec(expr->alignof_type);
            ion_complete_type(type);
            result = ion_operand_const(ion_type_usize, (ion_Val){.ull = ion_type_alignof(type)});
        }
        break;
    }
    case ION_EXPR_TYPEOF_TYPE: {
        {
            ion_Type (*type) = ion_resolve_typespec(expr->typeof_type);
            result = ion_operand_const(ion_type_ullong, (ion_Val){.ull = type->typeid});
        }
        break;
    }
    case ION_EXPR_TYPEOF_EXPR: {
        {
            if ((expr->typeof_expr->kind) == (ION_EXPR_NAME)) {
                ion_Sym (*sym) = ion_resolve_name(expr->typeof_expr->name);
                if ((sym) && ((sym->kind) == (ION_SYM_TYPE))) {
                    result = ion_operand_const(ion_type_ullong, (ion_Val){.ull = sym->type->typeid});
                    ion_set_resolved_type(expr->typeof_expr, sym->type);
                    ion_set_resolved_sym(expr->typeof_expr, sym);
                    break;
                }
            }
            ion_Type (*type) = ion_resolve_expr(expr->typeof_expr).type;
            result = ion_operand_const(ion_type_ullong, (ion_Val){.ull = type->typeid});
        }
        break;
    }
    case ION_EXPR_OFFSETOF: {
        {
            ion_Type (*type) = ion_resolve_typespec(expr->offsetof_field.type);
            ion_complete_type(type);
            if (((type->kind) != (ION_CMPL_TYPE_STRUCT)) && ((type->kind) != (ION_CMPL_TYPE_UNION))) {
                ion_fatal_error(expr->pos, "offsetof can only be used with struct/union types");
            }
            int field = ion_aggregate_item_field_index(type, expr->offsetof_field.name);
            if ((field) < (0)) {
                ion_fatal_error(expr->pos, "No field \'%s\' in type", expr->offsetof_field.name);
            }
            result = ion_operand_const(ion_type_usize, (ion_Val){.ull = type->aggregate.fields[field].offset});
        }
        break;
    }
    case ION_EXPR_MODIFY: {
        result = ion_resolve_expr_modify(expr);
        break;
    }
    default: {
        assert(0);
        result = ion_operand_null;
        break;
    }
    }
    ion_set_resolved_type(expr, result.type);
    return result;
}

ion_Operand ion_resolve_const_expr(ion_Expr (*expr)) {
    ion_Operand operand = ion_resolve_expr(expr);
    if (!(operand.is_const)) {
        ion_fatal_error(expr->pos, "Expected constant expression");
    }
    return operand;
}

ion_Map ion_decl_note_names;
void ion_init_builtin_syms(void) {
    assert(ion_current_package);
    ion_sym_global_type("void", ion_type_void);
    ion_sym_global_type("bool", ion_type_bool);
    ion_sym_global_type("char", ion_type_char);
    ion_sym_global_type("schar", ion_type_schar);
    ion_sym_global_type("uchar", ion_type_uchar);
    ion_sym_global_type("short", ion_type_short);
    ion_sym_global_type("ushort", ion_type_ushort);
    ion_sym_global_type("int", ion_type_int);
    ion_sym_global_type("uint", ion_type_uint);
    ion_sym_global_type("long", ion_type_long);
    ion_sym_global_type("ulong", ion_type_ulong);
    ion_sym_global_type("llong", ion_type_llong);
    ion_sym_global_type("ullong", ion_type_ullong);
    ion_sym_global_type("float", ion_type_float);
    ion_sym_global_type("double", ion_type_double);
}

void ion_add_package_decls(ion_Package (*package)) {
    for (size_t i = 0; (i) < (package->num_decls); (i)++) {
        ion_Decl (*decl) = package->decls[i];
        if ((decl->kind) == (ION_DECL_NOTE)) {
            if (!(ion_map_get(&(ion_decl_note_names), decl->note.name))) {
                ion_warning(decl->pos, "Unknown declaration #directive \'%s\'", decl->note.name);
            }
            if ((decl->note.name) == (ion_declare_note_name)) {
                if ((decl->note.num_args) != (1)) {
                    ion_fatal_error(decl->pos, "#declare_note takes 1 argument");
                }
                ion_Expr (*arg) = decl->note.args[0].expr;
                if ((arg->kind) != (ION_EXPR_NAME)) {
                    ion_fatal_error(decl->pos, "#declare_note argument must be name");
                }
                ion_map_put(&(ion_decl_note_names), arg->name, (void *)(1));
            } else if ((decl->note.name) == (ion_static_assert_name)) {
                if (!(ion_flag_lazy)) {
                    ion_resolve_static_assert(decl->note);
                }
            }
        } else if ((decl->kind) == (ION_DECL_IMPORT)) {
        } else {
            ion_sym_global_decl(decl);
        }
    }
}

bool ion_is_package_dir(char const ((*search_path)), char const ((*package_path))) {
    char (path[MAX_PATH]) = {0};
    ion_path_copy(path, search_path);
    ion_path_join(path, package_path);
    ion_DirListIter iter = {0};
    for (ion_dir_list(&(iter), path); iter.valid; ion_dir_list_next(&(iter))) {
        char const ((*ext)) = ion_path_ext(iter.name);
        if (((ext) != (iter.name)) && ((strcmp(ext, "ion")) == (0))) {
            ion_dir_list_free(&(iter));
            return true;
        }
    }
    return false;
}

bool ion_copy_package_full_path(char (dest[MAX_PATH]), char const ((*package_path))) {
    for (int i = 0; (i) < (ion_num_package_search_paths); (i)++) {
        if (ion_is_package_dir(ion_package_search_paths[i], package_path)) {
            ion_path_copy(dest, ion_package_search_paths[i]);
            ion_path_join(dest, package_path);
            return true;
        }
    }
    return false;
}

ion_Package (*ion_import_package(char const ((*package_path)))) {
    package_path = ion_str_intern(package_path);
    ion_Package (*package) = ion_map_get(&(ion_package_map), package_path);
    if (!(package)) {
        package = ion_xcalloc(1, sizeof(ion_Package));
        package->path = package_path;
        if (ion_flag_verbose) {
            printf("Importing %s\n", package_path);
        }
        char (full_path[MAX_PATH]) = {0};
        if (!(ion_copy_package_full_path(full_path, package_path))) {
            return NULL;
        }
        strcpy(package->full_path, full_path);
        ion_add_package(package);
        ion_compile_package(package);
    }
    return package;
}

void ion_import_all_package_symbols(ion_Package (*package)) {
    char const ((*main_name)) = ion_str_intern("main");
    for (size_t i = 0; (i) < (ion_buf_len(package->syms)); (i)++) {
        if (((package->syms[i]->home_package) == (package)) && ((package->syms[i]->name) != (main_name))) {
            ion_sym_global_put(package->syms[i]->name, package->syms[i]);
        }
    }
}

void ion_import_package_symbols(ion_Decl (*decl), ion_Package (*package)) {
    for (size_t i = 0; (i) < (decl->import_decl.num_items); (i)++) {
        ion_ImportItem item = decl->import_decl.items[i];
        ion_Sym (*sym) = ion_get_package_sym(package, item.name);
        if (!(sym)) {
            ion_fatal_error(decl->pos, "Symbol \'%s\' does not exist in package \'%s\'", item.name, package->path);
        }
        ion_sym_global_put((item.rename ? item.rename : item.name), sym);
    }
}

void ion_process_package_imports(ion_Package (*package)) {
    for (size_t i = 0; (i) < (package->num_decls); (i)++) {
        ion_Decl (*decl) = package->decls[i];
        if ((decl->kind) == (ION_DECL_NOTE)) {
            if ((decl->note.name) == (ion_always_name)) {
                package->always_reachable = true;
            }
        } else if ((decl->kind) == (ION_DECL_IMPORT)) {
            char (*path_buf) = NULL;
            if (decl->import_decl.is_relative) {
                ion_buf_printf(&(path_buf), "%s/", package->path);
            }
            for (size_t k = 0; (k) < (decl->import_decl.num_names); (k)++) {
                if (!(ion_str_islower(decl->import_decl.names[k]))) {
                    ion_fatal_error(decl->pos, "Import name must be lower case: \'%s\'", decl->import_decl.names[k]);
                }
                ion_buf_printf(&(path_buf), "%s%s", ((k) == (0) ? "" : "/"), decl->import_decl.names[k]);
            }
            ion_Package (*imported_package) = ion_import_package(path_buf);
            if (!(imported_package)) {
                ion_fatal_error(decl->pos, "Failed to import package \'%s\'", path_buf);
            }
            ion_buf_free((void (**))(&(path_buf)));
            ion_import_package_symbols(decl, imported_package);
            if (decl->import_decl.import_all) {
                ion_import_all_package_symbols(imported_package);
            }
            char const ((*sym_name)) = (decl->name ? decl->name : decl->import_decl.names[(decl->import_decl.num_names) - (1)]);
            ion_Sym (*sym) = ion_sym_new(ION_SYM_PACKAGE, sym_name, decl);
            sym->package = imported_package;
            ion_sym_global_put(sym_name, sym);
        }
    }
}

bool ion_parse_package(ion_Package (*package)) {
    ion_Decl (*(*decls)) = NULL;
    ion_DirListIter iter = {0};
    for (ion_dir_list(&(iter), package->full_path); iter.valid; ion_dir_list_next(&(iter))) {
        if (((iter.is_dir) || ((iter.name[0]) == ('_'))) || ((iter.name[0]) == ('.'))) {
            continue;
        }
        char (name[MAX_PATH]) = {0};
        ion_path_copy(name, iter.name);
        char (*ext) = ion_path_ext(name);
        if (((ext) == (name)) || ((strcmp(ext, "ion")) != (0))) {
            continue;
        }
        ext[-(1)] = 0;
        if (ion_is_excluded_target_filename(name)) {
            continue;
        }
        char (path[MAX_PATH]) = {0};
        ion_path_copy(path, iter.base);
        ion_path_join(path, iter.name);
        ion_path_absolute(path);
        char const ((*code)) = ion_read_file(path);
        if (!(code)) {
            ion_fatal_error((ion_SrcPos){.name = path}, "Failed to read source file");
        }
        ion_init_stream(ion_str_intern(path), code);
        ion_Decls (*file_decls) = ion_parse_decls();
        for (size_t i = 0; (i) < (file_decls->num_decls); (i)++) {
            ion_buf_push((void (**))(&(decls)), &(file_decls->decls[i]), sizeof(file_decls->decls[i]));
        }
    }
    package->decls = decls;
    package->num_decls = (int)(ion_buf_len(decls));
    return (package) != (NULL);
}

bool ion_compile_package(ion_Package (*package)) {
    if (!(ion_parse_package(package))) {
        return false;
    }
    ion_Package (*old_package) = ion_enter_package(package);
    if ((ion_buf_len(ion_package_list)) == (1)) {
        ion_init_builtin_syms();
    }
    if (ion_builtin_package) {
        ion_import_all_package_symbols(ion_builtin_package);
    }
    ion_add_package_decls(package);
    ion_process_package_imports(package);
    ion_leave_package(old_package);
    return true;
}

void ion_resolve_package_syms(ion_Package (*package)) {
    ion_Package (*old_package) = ion_enter_package(package);
    for (size_t i = 0; (i) < (ion_buf_len(package->syms)); (i)++) {
        if ((package->syms[i]->home_package) == (package)) {
            ion_resolve_sym(package->syms[i]);
        }
    }
    ion_leave_package(old_package);
}

void ion_finalize_reachable_syms(void) {
    if (ion_flag_verbose) {
        printf("Finalizing reachable symbols\n");
    }
    size_t prev_num_reachable = 0;
    ullong num_reachable = ion_buf_len(ion_reachable_syms);
    for (size_t i = 0; (i) < (num_reachable); (i)++) {
        ion_finalize_sym(ion_reachable_syms[i]);
        if ((i) == ((num_reachable) - (1))) {
            if (ion_flag_verbose) {
                printf("New reachable symbols:");
                for (size_t k = prev_num_reachable; (k) < (num_reachable); (k)++) {
                    printf(" %s/%s", ion_reachable_syms[k]->home_package->path, ion_reachable_syms[k]->name);
                }
                printf("\n");
            }
            prev_num_reachable = num_reachable;
            num_reachable = ion_buf_len(ion_reachable_syms);
        }
    }
}

char const ((*(ion_os_names[ION_NUM_OSES]))) = {[ION_OS_WIN32] = "win32", [ION_OS_LINUX] = "linux", [ION_OS_OSX] = "osx"};
char const ((*(ion_arch_names[ION_NUM_ARCHES]))) = {[ION_ARCH_X64] = "x64", [ION_ARCH_X86] = "x86"};
int ion_target_os;
int ion_target_arch;
int ion_get_os(char const ((*name))) {
    for (int i = 0; (i) < (ION_NUM_OSES); (i)++) {
        if ((strcmp(ion_os_names[i], name)) == (0)) {
            return i;
        }
    }
    return -(1);
}

int ion_get_arch(char const ((*name))) {
    for (int i = 0; (i) < (ION_NUM_ARCHES); (i)++) {
        if ((strcmp(ion_arch_names[i], name)) == (0)) {
            return i;
        }
    }
    return -(1);
}

void ion_init_default_type_metrics(ion_TypeMetrics (metrics[ION_NUM_CMPL_TYPE_KINDS])) {
    metrics[ION_CMPL_TYPE_BOOL] = (ion_TypeMetrics){.size = 1, .align = 1};
    metrics[ION_CMPL_TYPE_CHAR] = (ion_TypeMetrics){.size = 1, .align = 1, .max = 0x7f, .sign = true};
    metrics[ION_CMPL_TYPE_SCHAR] = (ion_TypeMetrics){.size = 1, .align = 1, .max = 0x7f, .sign = true};
    metrics[ION_CMPL_TYPE_UCHAR] = (ion_TypeMetrics){.size = 1, .align = 1, .max = 0xff};
    metrics[ION_CMPL_TYPE_SHORT] = (ion_TypeMetrics){.size = 2, .align = 2, .max = 0x7fff, .sign = true};
    metrics[ION_CMPL_TYPE_USHORT] = (ion_TypeMetrics){.size = 2, .align = 2, .max = 0xffff};
    metrics[ION_CMPL_TYPE_INT] = (ion_TypeMetrics){.size = 4, .align = 4, .max = 0x7fffffff, .sign = true};
    metrics[ION_CMPL_TYPE_UINT] = (ion_TypeMetrics){.size = 4, .align = 4, .max = 0xffffffff};
    metrics[ION_CMPL_TYPE_LLONG] = (ion_TypeMetrics){.size = 8, .align = 8, .max = 0x7fffffffffffffff, .sign = true};
    metrics[ION_CMPL_TYPE_ULLONG] = (ion_TypeMetrics){.size = 8, .align = 8, .max = 0xffffffffffffffff};
    metrics[ION_CMPL_TYPE_FLOAT] = (ion_TypeMetrics){.size = 4, .align = 4};
    metrics[ION_CMPL_TYPE_DOUBLE] = (ion_TypeMetrics){.size = 8, .align = 8};
}

ion_TypeMetrics (ion_win32_x86_metrics[ION_NUM_CMPL_TYPE_KINDS]) = {[ION_CMPL_TYPE_PTR] = {.size = 4, .align = 4}, [ION_CMPL_TYPE_LONG] = {.size = 4, .align = 4, .max = 0x7fffffff, .sign = true}, [ION_CMPL_TYPE_ULONG] = {.size = 4, .align = 4, .max = 0x7fffffff, .sign = true}};
ion_TypeMetrics (ion_win32_x64_metrics[ION_NUM_CMPL_TYPE_KINDS]) = {[ION_CMPL_TYPE_PTR] = {.size = 8, .align = 8}, [ION_CMPL_TYPE_LONG] = {.size = 4, .align = 4, .max = 0x7fffffff, .sign = true}, [ION_CMPL_TYPE_ULONG] = {.size = 4, .align = 4, .max = 0x7fffffff, .sign = true}};
ion_TypeMetrics (ion_ilp32_metrics[ION_NUM_CMPL_TYPE_KINDS]) = {[ION_CMPL_TYPE_PTR] = {.size = 4, .align = 4}, [ION_CMPL_TYPE_LONG] = {.size = 4, .align = 4, .max = 0x7fffffff, .sign = true}, [ION_CMPL_TYPE_ULONG] = {.size = 4, .align = 4, .max = 0x7fffffff, .sign = true}};
ion_TypeMetrics (ion_lp64_metrics[ION_NUM_CMPL_TYPE_KINDS]) = {[ION_CMPL_TYPE_PTR] = {.size = 8, .align = 8}, [ION_CMPL_TYPE_LONG] = {.size = 8, .align = 8, .max = 0x7fffffffffffffff, .sign = true}, [ION_CMPL_TYPE_ULONG] = {.size = 8, .align = 8, .max = 0xffffffffffffffff, .sign = true}};
void ion_init_target(void) {
    ion_type_metrics = NULL;
    switch (ion_target_os) {
    case ION_OS_WIN32: {
        switch (ion_target_arch) {
        case ION_ARCH_X86: {
            ion_type_metrics = ion_win32_x86_metrics;
            break;
        }
        case ION_ARCH_X64: {
            ion_type_metrics = ion_win32_x64_metrics;
            break;
        }
        default: {
            break;
        }
        }
        break;
    }
    case ION_OS_LINUX: {
        switch (ion_target_arch) {
        case ION_ARCH_X86: {
            ion_type_metrics = ion_ilp32_metrics;
            break;
        }
        case ION_ARCH_X64: {
            ion_type_metrics = ion_lp64_metrics;
            break;
        }
        default: {
            break;
        }
        }
        break;
    }
    case ION_OS_OSX: {
        switch (ion_target_arch) {
        case ION_ARCH_X64: {
            ion_type_metrics = ion_lp64_metrics;
            break;
        }
        default: {
            break;
        }
        }
        break;
    }
    default: {
        break;
    }
    }
    if (!(ion_type_metrics)) {
        printf("Unsupported os/arch combination: %s/%s\n", ion_os_names[ion_target_os], ion_arch_names[ion_target_arch]);
        exit(1);
    }
    ion_init_default_type_metrics(ion_type_metrics);
    if ((ion_type_metrics[ION_CMPL_TYPE_PTR].size) == (4)) {
        ion_type_uintptr = ion_type_uint;
        ion_type_usize = ion_type_uint;
        ion_type_ssize = ion_type_int;
    } else {
        assert((ion_type_metrics[ION_CMPL_TYPE_PTR].size) == (8));
        ion_type_uintptr = ion_type_ullong;
        ion_type_usize = ion_type_ullong;
        ion_type_ssize = ion_type_llong;
    }
}

bool ion_is_excluded_target_filename(char const ((*name))) {
    char const ((*end)) = (name) + (strlen(name));
    char const ((*ptr1)) = end;
    while (((ptr1) != (name)) && ((ptr1[-(1)]) != ('_'))) {
        (ptr1)--;
    }
    char (str1[MAX_PATH]) = {0};
    if ((ptr1) == (name)) {
        str1[0] = 0;
    } else {
        memcpy(str1, ptr1, (end) - (ptr1));
        str1[(end) - (ptr1)] = 0;
        (ptr1)--;
    }
    char const ((*ptr2)) = ptr1;
    while (((ptr2) != (name)) && ((ptr2[-(1)]) != ('_'))) {
        (ptr2)--;
    }
    char (str2[MAX_PATH]) = {0};
    if ((ptr2) == (name)) {
        str2[0] = 0;
    } else {
        memcpy(str2, ptr2, (ptr1) - (ptr2));
        str2[(ptr1) - (ptr2)] = 0;
    }
    int os1 = ion_get_os(str1);
    int arch1 = ion_get_arch(str1);
    int os2 = ion_get_os(str2);
    int arch2 = ion_get_arch(str2);
    if (((arch1) != (-(1))) && ((os2) != (-(1)))) {
        return ((arch1) != (ion_target_arch)) || ((os2) != (ion_target_os));
    } else if (((arch2) != (-(1))) && ((os1) != (-(1)))) {
        return ((arch2) != (ion_target_arch)) || ((os1) != (ion_target_os));
    } else if ((os1) != (-(1))) {
        return (os1) != (ion_target_os);
    } else if ((arch1) != (-(1))) {
        return (arch1) != (ion_target_arch);
    } else {
        return false;
    }
}

ion_TypeMetrics (*ion_type_metrics);
ion_Type (*ion_type_void) = &((ion_Type){ION_CMPL_TYPE_VOID});
ion_Type (*ion_type_bool) = &((ion_Type){ION_CMPL_TYPE_BOOL});
ion_Type (*ion_type_char) = &((ion_Type){ION_CMPL_TYPE_CHAR});
ion_Type (*ion_type_uchar) = &((ion_Type){ION_CMPL_TYPE_UCHAR});
ion_Type (*ion_type_schar) = &((ion_Type){ION_CMPL_TYPE_SCHAR});
ion_Type (*ion_type_short) = &((ion_Type){ION_CMPL_TYPE_SHORT});
ion_Type (*ion_type_ushort) = &((ion_Type){ION_CMPL_TYPE_USHORT});
ion_Type (*ion_type_int) = &((ion_Type){ION_CMPL_TYPE_INT});
ion_Type (*ion_type_uint) = &((ion_Type){ION_CMPL_TYPE_UINT});
ion_Type (*ion_type_long) = &((ion_Type){ION_CMPL_TYPE_LONG});
ion_Type (*ion_type_ulong) = &((ion_Type){ION_CMPL_TYPE_ULONG});
ion_Type (*ion_type_llong) = &((ion_Type){ION_CMPL_TYPE_LLONG});
ion_Type (*ion_type_ullong) = &((ion_Type){ION_CMPL_TYPE_ULLONG});
ion_Type (*ion_type_float) = &((ion_Type){ION_CMPL_TYPE_FLOAT});
ion_Type (*ion_type_double) = &((ion_Type){ION_CMPL_TYPE_DOUBLE});
int ion_next_typeid = 1;
ion_Type (*ion_type_uintptr);
ion_Type (*ion_type_usize);
ion_Type (*ion_type_ssize);
ion_Map ion_typeid_map;
ion_Type (*ion_get_type_from_typeid(int typeid)) {
    if ((typeid) == (0)) {
        return NULL;
    }
    return ion_map_get(&(ion_typeid_map), (void *)((uintptr_t)(typeid)));
}

void ion_register_typeid(ion_Type (*type)) {
    ion_map_put(&(ion_typeid_map), (void *)((uintptr_t)(type->typeid)), type);
}

ion_Type (*ion_type_alloc(TypeKind kind)) {
    ion_Type (*type) = ion_xcalloc(1, sizeof(ion_Type));
    type->kind = kind;
    type->typeid = (ion_next_typeid)++;
    ion_register_typeid(type);
    return type;
}

bool ion_is_ptr_type(ion_Type (*type)) {
    return (type->kind) == (ION_CMPL_TYPE_PTR);
}

bool ion_is_func_type(ion_Type (*type)) {
    return (type->kind) == (ION_CMPL_TYPE_FUNC);
}

bool ion_is_ptr_like_type(ion_Type (*type)) {
    return ((type->kind) == (ION_CMPL_TYPE_PTR)) || ((type->kind) == (ION_CMPL_TYPE_FUNC));
}

bool ion_is_const_type(ion_Type (*type)) {
    return (type->kind) == (ION_CMPL_TYPE_CONST);
}

bool ion_is_array_type(ion_Type (*type)) {
    return (type->kind) == (ION_CMPL_TYPE_ARRAY);
}

bool ion_is_incomplete_array_type(ion_Type (*type)) {
    return (ion_is_array_type(type)) && ((type->num_elems) == (0));
}

bool ion_is_integer_type(ion_Type (*type)) {
    return ((ION_CMPL_TYPE_BOOL) <= (type->kind)) && ((type->kind) <= (ION_CMPL_TYPE_ENUM));
}

bool ion_is_floating_type(ion_Type (*type)) {
    return ((ION_CMPL_TYPE_FLOAT) <= (type->kind)) && ((type->kind) <= (ION_CMPL_TYPE_DOUBLE));
}

bool ion_is_arithmetic_type(ion_Type (*type)) {
    return ((ION_CMPL_TYPE_BOOL) <= (type->kind)) && ((type->kind) <= (ION_CMPL_TYPE_DOUBLE));
}

bool ion_is_scalar_type(ion_Type (*type)) {
    return ((ION_CMPL_TYPE_BOOL) <= (type->kind)) && ((type->kind) <= (ION_CMPL_TYPE_FUNC));
}

bool ion_is_aggregate_type(ion_Type (*type)) {
    return ((type->kind) == (ION_CMPL_TYPE_STRUCT)) || ((type->kind) == (ION_CMPL_TYPE_UNION));
}

bool ion_is_signed_type(ion_Type (*type)) {
    switch (type->kind) {
    case ION_CMPL_TYPE_CHAR: {
        return ion_type_metrics[ION_CMPL_TYPE_CHAR].sign;
        break;
    }
    case ION_CMPL_TYPE_SCHAR:
    case ION_CMPL_TYPE_SHORT:
    case ION_CMPL_TYPE_INT:
    case ION_CMPL_TYPE_LONG:
    case ION_CMPL_TYPE_LLONG: {
        return true;
        break;
    }
    default: {
        return false;
        break;
    }
    }
}

char const ((*(ion_type_names[ION_NUM_CMPL_TYPE_KINDS]))) = {[ION_CMPL_TYPE_VOID] = "void", [ION_CMPL_TYPE_BOOL] = "bool", [ION_CMPL_TYPE_CHAR] = "char", [ION_CMPL_TYPE_SCHAR] = "schar", [ION_CMPL_TYPE_UCHAR] = "uchar", [ION_CMPL_TYPE_SHORT] = "short", [ION_CMPL_TYPE_USHORT] = "ushort", [ION_CMPL_TYPE_INT] = "int", [ION_CMPL_TYPE_UINT] = "uint", [ION_CMPL_TYPE_LONG] = "long", [ION_CMPL_TYPE_ULONG] = "ulong", [ION_CMPL_TYPE_LLONG] = "llong", [ION_CMPL_TYPE_ULLONG] = "ullong", [ION_CMPL_TYPE_FLOAT] = "float", [ION_CMPL_TYPE_DOUBLE] = "double"};
int (ion_type_ranks[ION_NUM_CMPL_TYPE_KINDS]) = {[ION_CMPL_TYPE_BOOL] = 1, [ION_CMPL_TYPE_CHAR] = 2, [ION_CMPL_TYPE_SCHAR] = 2, [ION_CMPL_TYPE_UCHAR] = 2, [ION_CMPL_TYPE_SHORT] = 3, [ION_CMPL_TYPE_USHORT] = 3, [ION_CMPL_TYPE_INT] = 4, [ION_CMPL_TYPE_UINT] = 4, [ION_CMPL_TYPE_LONG] = 5, [ION_CMPL_TYPE_ULONG] = 5, [ION_CMPL_TYPE_LLONG] = 6, [ION_CMPL_TYPE_ULLONG] = 6};
int ion_type_rank(ion_Type (*type)) {
    int rank = ion_type_ranks[type->kind];
    assert((rank) != (0));
    return rank;
}

ion_Type (*ion_unsigned_type(ion_Type (*type))) {
    switch (type->kind) {
    case ION_CMPL_TYPE_BOOL: {
        return ion_type_bool;
        break;
    }
    case ION_CMPL_TYPE_CHAR:
    case ION_CMPL_TYPE_SCHAR:
    case ION_CMPL_TYPE_UCHAR: {
        return ion_type_uchar;
        break;
    }
    case ION_CMPL_TYPE_SHORT:
    case ION_CMPL_TYPE_USHORT: {
        return ion_type_ushort;
        break;
    }
    case ION_CMPL_TYPE_INT:
    case ION_CMPL_TYPE_UINT: {
        return ion_type_uint;
        break;
    }
    case ION_CMPL_TYPE_LONG:
    case ION_CMPL_TYPE_ULONG: {
        return ion_type_ulong;
        break;
    }
    case ION_CMPL_TYPE_LLONG:
    case ION_CMPL_TYPE_ULLONG: {
        return ion_type_ullong;
        break;
    }
    default: {
        assert(0);
        return NULL;
        break;
    }
    }
}

size_t ion_type_sizeof(ion_Type (*type)) {
    assert((type->kind) > (ION_CMPL_TYPE_COMPLETING));
    return type->size;
}

size_t ion_type_alignof(ion_Type (*type)) {
    assert((type->kind) > (ION_CMPL_TYPE_COMPLETING));
    return type->align;
}

ion_Map ion_cached_ptr_types;
ion_Type (*ion_type_ptr(ion_Type (*base))) {
    ion_Type (*type) = ion_map_get(&(ion_cached_ptr_types), base);
    if (!(type)) {
        type = ion_type_alloc(ION_CMPL_TYPE_PTR);
        type->size = ion_type_metrics[ION_CMPL_TYPE_PTR].size;
        type->align = ion_type_metrics[ION_CMPL_TYPE_PTR].align;
        type->base = base;
        ion_map_put(&(ion_cached_ptr_types), base, type);
    }
    return type;
}

ion_Map ion_cached_const_types;
ion_Type (*ion_type_const(ion_Type (*base))) {
    if ((base->kind) == (ION_CMPL_TYPE_CONST)) {
        return base;
    }
    ion_Type (*type) = ion_map_get(&(ion_cached_const_types), base);
    if (!(type)) {
        ion_complete_type(base);
        type = ion_type_alloc(ION_CMPL_TYPE_CONST);
        type->nonmodifiable = true;
        type->size = base->size;
        type->align = base->align;
        type->base = base;
        ion_map_put(&(ion_cached_const_types), base, type);
    }
    return type;
}

ion_Type (*ion_unqualify_type(ion_Type (*type))) {
    if ((type->kind) == (ION_CMPL_TYPE_CONST)) {
        return type->base;
    } else {
        return type;
    }
}

ion_Map ion_cached_array_types;
ion_Type (*ion_type_array(ion_Type (*base), size_t num_elems)) {
    ullong hash = ion_hash_mix(ion_hash_ptr(base), ion_hash_uint64(num_elems));
    uint64_t key = (hash ? hash : 1);
    ion_CachedArrayType (*cached) = ion_map_get_from_uint64(&(ion_cached_array_types), key);
    for (ion_CachedArrayType (*it) = cached; it; it = it->next) {
        ion_Type (*type) = it->type;
        if (((type->base) == (base)) && ((type->num_elems) == (num_elems))) {
            return type;
        }
    }
    ion_complete_type(base);
    ion_Type (*type) = ion_type_alloc(ION_CMPL_TYPE_ARRAY);
    type->nonmodifiable = base->nonmodifiable;
    type->size = (num_elems) * (ion_type_sizeof(base));
    type->align = ion_type_alignof(base);
    type->base = base;
    type->num_elems = num_elems;
    ion_CachedArrayType (*new_cached) = ion_xmalloc(sizeof(ion_CachedArrayType));
    new_cached->type = type;
    new_cached->next = cached;
    ion_map_put_from_uint64(&(ion_cached_array_types), key, new_cached);
    return type;
}

ion_Map ion_cached_func_types;
ion_Type (*ion_type_func(ion_Type (*(*params)), size_t num_params, ion_Type (*ret), bool has_varargs)) {
    ullong params_size = (num_params) * (sizeof(*(params)));
    ullong hash = ion_hash_mix(ion_hash_bytes(params, params_size), ion_hash_ptr(ret));
    uint64_t key = (hash ? hash : 1);
    ion_CachedFuncType (*cached) = ion_map_get_from_uint64(&(ion_cached_func_types), key);
    for (ion_CachedFuncType (*it) = cached; it; it = it->next) {
        ion_Type (*type) = it->type;
        if ((((type->function.num_params) == (num_params)) && ((type->function.ret) == (ret))) && ((type->function.has_varargs) == (has_varargs))) {
            if ((memcmp(type->function.params, params, params_size)) == (0)) {
                return type;
            }
        }
    }
    ion_Type (*type) = ion_type_alloc(ION_CMPL_TYPE_FUNC);
    type->size = ion_type_metrics[ION_CMPL_TYPE_PTR].size;
    type->align = ion_type_metrics[ION_CMPL_TYPE_PTR].align;
    type->function.params = ion_memdup(params, params_size);
    type->function.num_params = num_params;
    type->function.has_varargs = has_varargs;
    type->function.ret = ret;
    ion_CachedFuncType (*new_cached) = ion_xmalloc(sizeof(ion_CachedFuncType));
    new_cached->type = type;
    new_cached->next = cached;
    ion_map_put_from_uint64(&(ion_cached_func_types), key, new_cached);
    return type;
}

bool ion_has_duplicate_fields(ion_Type (*type)) {
    for (size_t i = 0; (i) < (type->aggregate.num_fields); (i)++) {
        for (size_t j = (i) + (1); (j) < (type->aggregate.num_fields); (j)++) {
            if ((type->aggregate.fields[i].name) == (type->aggregate.fields[j].name)) {
                return true;
            }
        }
    }
    return false;
}

void ion_add_type_fields(ion_TypeField (*(*fields)), ion_Type (*type), size_t offset) {
    assert(((type->kind) == (ION_CMPL_TYPE_STRUCT)) || ((type->kind) == (ION_CMPL_TYPE_UNION)));
    for (size_t i = 0; (i) < (type->aggregate.num_fields); (i)++) {
        ion_TypeField (*field) = &(type->aggregate.fields[i]);
        ion_TypeField new_field = {field->name, field->type, (field->offset) + (offset)};
        ion_buf_push((void (**))(fields), &(new_field), sizeof(new_field));
    }
}

void ion_type_complete_struct(ion_Type (*type), ion_TypeField (*fields), size_t num_fields) {
    assert((type->kind) == (ION_CMPL_TYPE_COMPLETING));
    type->kind = ION_CMPL_TYPE_STRUCT;
    type->size = 0;
    type->align = 0;
    bool nonmodifiable = false;
    ion_TypeField (*new_fields) = {0};
    for (ion_TypeField (*it) = fields; (it) != ((fields) + (num_fields)); (it)++) {
        assert(ion_is_pow2(ion_type_alignof(it->type)));
        if (it->name) {
            it->offset = type->size;
            ion_buf_push((void (**))(&(new_fields)), it, sizeof(*(it)));
        } else {
            ion_add_type_fields(&(new_fields), it->type, type->size);
        }
        type->align = ion_max(type->align, ion_type_alignof(it->type));
        type->size = (ion_type_sizeof(it->type)) + (ion_align_up(type->size, ion_type_alignof(it->type)));
        nonmodifiable = (it->type->nonmodifiable) || (nonmodifiable);
    }
    type->size = ion_align_up(type->size, type->align);
    type->aggregate.fields = new_fields;
    type->aggregate.num_fields = ion_buf_len(new_fields);
    type->nonmodifiable = nonmodifiable;
}

void ion_type_complete_union(ion_Type (*type), ion_TypeField (*fields), size_t num_fields) {
    assert((type->kind) == (ION_CMPL_TYPE_COMPLETING));
    type->kind = ION_CMPL_TYPE_UNION;
    type->size = 0;
    type->align = 0;
    bool nonmodifiable = false;
    ion_TypeField (*new_fields) = {0};
    for (ion_TypeField (*it) = fields; (it) != ((fields) + (num_fields)); (it)++) {
        assert((it->type->kind) > (ION_CMPL_TYPE_COMPLETING));
        if (it->name) {
            it->offset = type->size;
            ion_buf_push((void (**))(&(new_fields)), it, sizeof(*(it)));
        } else {
            ion_add_type_fields(&(new_fields), it->type, 0);
        }
        type->size = ion_max(type->size, ion_type_sizeof(it->type));
        type->align = ion_max(type->align, ion_type_alignof(it->type));
        nonmodifiable = (it->type->nonmodifiable) || (nonmodifiable);
    }
    type->size = ion_align_up(type->size, type->align);
    type->aggregate.fields = new_fields;
    type->aggregate.num_fields = ion_buf_len(new_fields);
    type->nonmodifiable = nonmodifiable;
}

ion_Type (*ion_type_incomplete(ion_Sym (*sym))) {
    ion_Type (*type) = ion_type_alloc(ION_CMPL_TYPE_INCOMPLETE);
    type->sym = sym;
    return type;
}

ion_Type (*ion_type_enum(ion_Sym (*sym), ion_Type (*base))) {
    ion_Type (*type) = ion_type_alloc(ION_CMPL_TYPE_ENUM);
    type->sym = sym;
    type->base = base;
    type->size = ion_type_int->size;
    type->align = ion_type_int->align;
    return type;
}

void ion_init_builtin_type(ion_Type (*type)) {
    type->typeid = (ion_next_typeid)++;
    ion_register_typeid(type);
    type->size = ion_type_metrics[type->kind].size;
    type->align = ion_type_metrics[type->kind].align;
}

void ion_init_builtin_types(void) {
    ion_init_builtin_type(ion_type_void);
    ion_init_builtin_type(ion_type_bool);
    ion_init_builtin_type(ion_type_char);
    ion_init_builtin_type(ion_type_uchar);
    ion_init_builtin_type(ion_type_schar);
    ion_init_builtin_type(ion_type_short);
    ion_init_builtin_type(ion_type_ushort);
    ion_init_builtin_type(ion_type_int);
    ion_init_builtin_type(ion_type_uint);
    ion_init_builtin_type(ion_type_long);
    ion_init_builtin_type(ion_type_ulong);
    ion_init_builtin_type(ion_type_llong);
    ion_init_builtin_type(ion_type_ullong);
    ion_init_builtin_type(ion_type_float);
    ion_init_builtin_type(ion_type_double);
}

int ion_aggregate_item_field_index(ion_Type (*type), char const ((*name))) {
    assert(ion_is_aggregate_type(type));
    for (size_t i = 0; (i) < (type->aggregate.num_fields); (i)++) {
        if ((type->aggregate.fields[i].name) == (name)) {
            return (int)(i);
        }
    }
    return -(1);
}

ion_Type (*ion_aggregate_item_field_type_from_index(ion_Type (*type), int index)) {
    assert(ion_is_aggregate_type(type));
    assert(((0) <= (index)) && ((index) < ((int)(type->aggregate.num_fields))));
    return type->aggregate.fields[index].type;
}

ion_Type (*ion_aggregate_item_field_type_from_name(ion_Type (*type), char const ((*name)))) {
    assert(ion_is_aggregate_type(type));
    int index = ion_aggregate_item_field_index(type, name);
    if ((index) < (0)) {
        return NULL;
    }
    return ion_aggregate_item_field_type_from_index(type, index);
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
