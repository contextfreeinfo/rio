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
typedef struct rio_Arena rio_Arena;
typedef struct rio_SrcPos rio_SrcPos;
typedef struct rio_Note rio_Note;
typedef struct rio_Notes rio_Notes;
typedef struct rio_StmtList rio_StmtList;
typedef struct rio_Map rio_Map;
typedef struct rio_Token rio_Token;
typedef struct rio_CompoundField rio_CompoundField;
typedef struct rio_SwitchCasePattern rio_SwitchCasePattern;
typedef struct rio_SwitchCase rio_SwitchCase;
typedef struct rio_EnumItem rio_EnumItem;
typedef struct rio_AggregateItem rio_AggregateItem;
typedef struct rio_FuncParam rio_FuncParam;
typedef struct rio_NoteArg rio_NoteArg;
typedef union rio_Val rio_Val;
typedef struct rio_Sym rio_Sym;
typedef struct rio_Operand rio_Operand;
typedef struct rio_Label rio_Label;
typedef struct rio_StmtCtx rio_StmtCtx;
typedef struct rio_TypeMetrics rio_TypeMetrics;
typedef struct rio_TypeAggregate rio_TypeAggregate;
typedef struct rio_TypeFunc rio_TypeFunc;
typedef struct rio_Type rio_Type;
typedef struct TypeFieldInfo TypeFieldInfo;
typedef struct Any Any;
typedef struct rio_StmtIf rio_StmtIf;
typedef struct rio_StmtWhile rio_StmtWhile;
typedef struct rio_StmtFor rio_StmtFor;
typedef struct rio_StmtSwitch rio_StmtSwitch;
typedef struct rio_StmtAssign rio_StmtAssign;
typedef struct rio_StmtInit rio_StmtInit;
typedef struct rio_Stmt rio_Stmt;
typedef struct rio_TypespecFunc rio_TypespecFunc;
typedef struct rio_Typespec rio_Typespec;
typedef struct rio_ExprParen rio_ExprParen;
typedef struct rio_ExprIntLit rio_ExprIntLit;
typedef struct rio_ExprFloatLit rio_ExprFloatLit;
typedef struct rio_ExprStrLit rio_ExprStrLit;
typedef struct rio_ExprOffsetofField rio_ExprOffsetofField;
typedef struct rio_ExprCompound rio_ExprCompound;
typedef struct rio_ExprCast rio_ExprCast;
typedef struct rio_ExprModify rio_ExprModify;
typedef struct rio_ExprUnary rio_ExprUnary;
typedef struct rio_ExprBinary rio_ExprBinary;
typedef struct rio_ExprTernary rio_ExprTernary;
typedef struct rio_ExprCall rio_ExprCall;
typedef struct rio_ExprIndex rio_ExprIndex;
typedef struct rio_ExprField rio_ExprField;
typedef struct rio_Expr rio_Expr;
typedef struct rio_Decls rio_Decls;
typedef struct rio_DeclEnum rio_DeclEnum;
typedef struct rio_DeclFunc rio_DeclFunc;
typedef struct rio_DeclTypedef rio_DeclTypedef;
typedef struct rio_DeclVar rio_DeclVar;
typedef struct rio_DeclImport rio_DeclImport;
typedef struct rio_Decl rio_Decl;
typedef struct rio_Aggregate rio_Aggregate;
typedef struct rio_ImportItem rio_ImportItem;
typedef struct rio_ElseIf rio_ElseIf;
typedef struct rio_BufHdr rio_BufHdr;
typedef struct rio_Intern rio_Intern;
typedef struct rio_Package rio_Package;
typedef struct rio_TypeField rio_TypeField;
typedef struct rio_DirListIter rio_DirListIter;
typedef struct rio_FlagDefPtr rio_FlagDefPtr;
typedef struct rio_FlagDef rio_FlagDef;
typedef struct rio_CachedArrayType rio_CachedArrayType;
typedef struct rio_CachedFuncType rio_CachedFuncType;

// Sorted declarations
int main(int argc, char const ((*(*argv))));

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

struct rio_Arena {
    char (*ptr);
    char (*end);
    char (*(*blocks));
};

extern rio_Arena rio_ast_arena;

void (*rio_ast_alloc(size_t size));

void (*rio_ast_dup(void const ((*src)), size_t size));

struct rio_SrcPos {
    char const ((*name));
    int line;
};

struct rio_Note {
    rio_SrcPos pos;
    char const ((*name));
    rio_NoteArg (*args);
    size_t num_args;
};

rio_Note rio_new_note(rio_SrcPos pos, char const ((*name)), rio_NoteArg (*args), size_t num_args);

struct rio_Notes {
    rio_Note (*notes);
    size_t num_notes;
};

rio_Notes rio_new_notes(rio_Note (*notes), size_t num_notes);

struct rio_StmtList {
    rio_SrcPos pos;
    rio_Stmt (*(*stmts));
    size_t num_stmts;
};

rio_StmtList rio_new_stmt_list(rio_SrcPos pos, rio_Stmt (*(*stmts)), size_t num_stmts);

typedef int rio_TypespecKind;

rio_Typespec (*rio_new_typespec(rio_TypespecKind kind, rio_SrcPos pos));

rio_Typespec (*rio_new_typespec_name(rio_SrcPos pos, char const ((*name))));

rio_Typespec (*rio_new_typespec_ptr(rio_SrcPos pos, rio_Typespec (*base)));

rio_Typespec (*rio_new_typespec_const(rio_SrcPos pos, rio_Typespec (*base)));

rio_Typespec (*rio_new_typespec_array(rio_SrcPos pos, rio_Typespec (*elem), rio_Expr (*size)));

rio_Typespec (*rio_new_typespec_func(rio_SrcPos pos, rio_Typespec (*(*args)), size_t num_args, rio_Typespec (*ret), bool has_varargs));

rio_Decls (*rio_new_decls(rio_Decl (*(*decls)), size_t num_decls));

typedef int rio_DeclKind;

rio_Decl (*rio_new_decl(rio_DeclKind kind, rio_SrcPos pos, char const ((*name))));

rio_Note (*rio_get_decl_note(rio_Decl (*decl), char const ((*name))));

bool rio_is_decl_foreign(rio_Decl (*decl));

rio_Decl (*rio_new_decl_enum(rio_SrcPos pos, char const ((*name)), rio_Typespec (*type), rio_EnumItem (*items), size_t num_items));

typedef int rio_AggregateKind;

rio_Aggregate (*rio_new_aggregate(rio_SrcPos pos, rio_AggregateKind kind, rio_AggregateItem (*items), size_t num_items));

rio_Decl (*rio_new_decl_aggregate(rio_SrcPos pos, rio_DeclKind kind, char const ((*name)), rio_Aggregate (*aggregate)));

rio_Decl (*rio_new_decl_var(rio_SrcPos pos, char const ((*name)), rio_Typespec (*type), rio_Expr (*expr)));

rio_Decl (*rio_new_decl_func(rio_SrcPos pos, char const ((*name)), rio_FuncParam (*params), size_t num_params, rio_Typespec (*ret_type), bool has_varargs, rio_StmtList block));

rio_Decl (*rio_new_decl_const(rio_SrcPos pos, char const ((*name)), rio_Typespec (*type), rio_Expr (*expr)));

rio_Decl (*rio_new_decl_typedef(rio_SrcPos pos, char const ((*name)), rio_Typespec (*type)));

rio_Decl (*rio_new_decl_note(rio_SrcPos pos, rio_Note note));

rio_Decl (*rio_new_decl_import(rio_SrcPos pos, char const ((*rename_name)), bool is_relative, char const ((*(*names))), size_t num_names, bool import_all, rio_ImportItem (*items), size_t num_items));

typedef int rio_ExprKind;

rio_Expr (*rio_new_expr(rio_ExprKind kind, rio_SrcPos pos));

rio_Expr (*rio_new_expr_paren(rio_SrcPos pos, rio_Expr (*expr)));

rio_Expr (*rio_new_expr_sizeof_expr(rio_SrcPos pos, rio_Expr (*expr)));

rio_Expr (*rio_new_expr_sizeof_type(rio_SrcPos pos, rio_Typespec (*type)));

rio_Expr (*rio_new_expr_typeof_expr(rio_SrcPos pos, rio_Expr (*expr)));

rio_Expr (*rio_new_expr_typeof_type(rio_SrcPos pos, rio_Typespec (*type)));

rio_Expr (*rio_new_expr_alignof_expr(rio_SrcPos pos, rio_Expr (*expr)));

rio_Expr (*rio_new_expr_alignof_type(rio_SrcPos pos, rio_Typespec (*type)));

rio_Expr (*rio_new_expr_offsetof(rio_SrcPos pos, rio_Typespec (*type), char const ((*name))));

typedef int rio_TokenKind;

rio_Expr (*rio_new_expr_modify(rio_SrcPos pos, rio_TokenKind op, bool post, rio_Expr (*expr)));

typedef int rio_TokenMod;

typedef int rio_TokenSuffix;

rio_Expr (*rio_new_expr_int(rio_SrcPos pos, ullong val, rio_TokenMod mod, rio_TokenSuffix suffix));

rio_Expr (*rio_new_expr_float(rio_SrcPos pos, char const ((*start)), char const ((*end)), double val, rio_TokenSuffix suffix));

rio_Expr (*rio_new_expr_str(rio_SrcPos pos, char const ((*val)), rio_TokenMod mod));

rio_Expr (*rio_new_expr_name(rio_SrcPos pos, char const ((*name))));

rio_Expr (*rio_new_expr_compound(rio_SrcPos pos, rio_Typespec (*type), rio_CompoundField (*fields), size_t num_fields));

rio_Expr (*rio_new_expr_cast(rio_SrcPos pos, rio_Typespec (*type), rio_Expr (*expr)));

rio_Expr (*rio_new_expr_call(rio_SrcPos pos, rio_Expr (*expr), rio_Expr (*(*args)), size_t num_args));

rio_Expr (*rio_new_expr_index(rio_SrcPos pos, rio_Expr (*expr), rio_Expr (*index)));

rio_Expr (*rio_new_expr_field(rio_SrcPos pos, rio_Expr (*expr), char const ((*name))));

rio_Expr (*rio_new_expr_unary(rio_SrcPos pos, rio_TokenKind op, rio_Expr (*expr)));

rio_Expr (*rio_new_expr_binary(rio_SrcPos pos, rio_TokenKind op, rio_Expr (*left), rio_Expr (*right)));

rio_Expr (*rio_new_expr_ternary(rio_SrcPos pos, rio_Expr (*cond), rio_Expr (*then_expr), rio_Expr (*else_expr)));

rio_Note (*rio_get_stmt_note(rio_Stmt (*stmt), char const ((*name))));

typedef int rio_StmtKind;

rio_Stmt (*rio_new_stmt(rio_StmtKind kind, rio_SrcPos pos));

rio_Stmt (*rio_new_stmt_label(rio_SrcPos pos, char const ((*label))));

rio_Stmt (*rio_new_stmt_goto(rio_SrcPos pos, char const ((*label))));

rio_Stmt (*rio_new_stmt_note(rio_SrcPos pos, rio_Note note));

rio_Stmt (*rio_new_stmt_decl(rio_SrcPos pos, rio_Decl (*decl)));

rio_Stmt (*rio_new_stmt_return(rio_SrcPos pos, rio_Expr (*expr)));

rio_Stmt (*rio_new_stmt_break(rio_SrcPos pos));

rio_Stmt (*rio_new_stmt_continue(rio_SrcPos pos));

rio_Stmt (*rio_new_stmt_block(rio_SrcPos pos, rio_StmtList block));

rio_Stmt (*rio_new_stmt_if(rio_SrcPos pos, rio_Stmt (*init), rio_Expr (*cond), rio_StmtList then_block, rio_ElseIf (*elseifs), size_t num_elseifs, rio_StmtList else_block));

rio_Stmt (*rio_new_stmt_while(rio_SrcPos pos, rio_Expr (*cond), rio_StmtList block));

rio_Stmt (*rio_new_stmt_do_while(rio_SrcPos pos, rio_Expr (*cond), rio_StmtList block));

rio_Stmt (*rio_new_stmt_for(rio_SrcPos pos, rio_Stmt (*init), rio_Expr (*cond), rio_Stmt (*next), rio_StmtList block));

rio_Stmt (*rio_new_stmt_switch(rio_SrcPos pos, rio_Expr (*expr), rio_SwitchCase (*cases), size_t num_cases));

rio_Stmt (*rio_new_stmt_assign(rio_SrcPos pos, rio_TokenKind op, rio_Expr (*left), rio_Expr (*right)));

rio_Stmt (*rio_new_stmt_init(rio_SrcPos pos, char const ((*name)), bool is_mut, rio_Typespec (*type), rio_Expr (*expr)));

rio_Stmt (*rio_new_stmt_expr(rio_SrcPos pos, rio_Expr (*expr)));

#define RIO_TYPESPEC_NONE ((rio_TypespecKind)(0))

#define RIO_TYPESPEC_NAME ((rio_TypespecKind)((RIO_TYPESPEC_NONE) + (1)))

#define RIO_TYPESPEC_FUNC ((rio_TypespecKind)((RIO_TYPESPEC_NAME) + (1)))

#define RIO_TYPESPEC_ARRAY ((rio_TypespecKind)((RIO_TYPESPEC_FUNC) + (1)))

#define RIO_TYPESPEC_PTR ((rio_TypespecKind)((RIO_TYPESPEC_ARRAY) + (1)))

#define RIO_TYPESPEC_CONST ((rio_TypespecKind)((RIO_TYPESPEC_PTR) + (1)))

typedef int rio_AggregateItemKind;

#define RIO_AGGREGATE_ITEM_NONE ((rio_AggregateItemKind)(0))

#define RIO_AGGREGATE_ITEM_FIELD ((rio_AggregateItemKind)((RIO_AGGREGATE_ITEM_NONE) + (1)))

#define RIO_AGGREGATE_ITEM_SUBAGGREGATE ((rio_AggregateItemKind)((RIO_AGGREGATE_ITEM_FIELD) + (1)))

#define RIO_DECL_NONE ((rio_DeclKind)(0))

#define RIO_DECL_ENUM ((rio_DeclKind)((RIO_DECL_NONE) + (1)))

#define RIO_DECL_STRUCT ((rio_DeclKind)((RIO_DECL_ENUM) + (1)))

#define RIO_DECL_UNION ((rio_DeclKind)((RIO_DECL_STRUCT) + (1)))

#define RIO_DECL_VAR ((rio_DeclKind)((RIO_DECL_UNION) + (1)))

#define RIO_DECL_CONST ((rio_DeclKind)((RIO_DECL_VAR) + (1)))

#define RIO_DECL_TYPEDEF ((rio_DeclKind)((RIO_DECL_CONST) + (1)))

#define RIO_DECL_FUNC ((rio_DeclKind)((RIO_DECL_TYPEDEF) + (1)))

#define RIO_DECL_NOTE ((rio_DeclKind)((RIO_DECL_FUNC) + (1)))

#define RIO_DECL_IMPORT ((rio_DeclKind)((RIO_DECL_NOTE) + (1)))

#define RIO_AGGREGATE_NONE ((rio_AggregateKind)(0))

#define RIO_AGGREGATE_STRUCT ((rio_AggregateKind)((RIO_AGGREGATE_NONE) + (1)))

#define RIO_AGGREGATE_UNION ((rio_AggregateKind)((RIO_AGGREGATE_STRUCT) + (1)))

#define RIO_EXPR_NONE ((rio_ExprKind)(0))

#define RIO_EXPR_PAREN ((rio_ExprKind)((RIO_EXPR_NONE) + (1)))

#define RIO_EXPR_INT ((rio_ExprKind)((RIO_EXPR_PAREN) + (1)))

#define RIO_EXPR_FLOAT ((rio_ExprKind)((RIO_EXPR_INT) + (1)))

#define RIO_EXPR_STR ((rio_ExprKind)((RIO_EXPR_FLOAT) + (1)))

#define RIO_EXPR_NAME ((rio_ExprKind)((RIO_EXPR_STR) + (1)))

#define RIO_EXPR_CAST ((rio_ExprKind)((RIO_EXPR_NAME) + (1)))

#define RIO_EXPR_CALL ((rio_ExprKind)((RIO_EXPR_CAST) + (1)))

#define RIO_EXPR_INDEX ((rio_ExprKind)((RIO_EXPR_CALL) + (1)))

#define RIO_EXPR_FIELD ((rio_ExprKind)((RIO_EXPR_INDEX) + (1)))

#define RIO_EXPR_COMPOUND ((rio_ExprKind)((RIO_EXPR_FIELD) + (1)))

#define RIO_EXPR_UNARY ((rio_ExprKind)((RIO_EXPR_COMPOUND) + (1)))

#define RIO_EXPR_BINARY ((rio_ExprKind)((RIO_EXPR_UNARY) + (1)))

#define RIO_EXPR_TERNARY ((rio_ExprKind)((RIO_EXPR_BINARY) + (1)))

#define RIO_EXPR_MODIFY ((rio_ExprKind)((RIO_EXPR_TERNARY) + (1)))

#define RIO_EXPR_SIZEOF_EXPR ((rio_ExprKind)((RIO_EXPR_MODIFY) + (1)))

#define RIO_EXPR_SIZEOF_TYPE ((rio_ExprKind)((RIO_EXPR_SIZEOF_EXPR) + (1)))

#define RIO_EXPR_TYPEOF_EXPR ((rio_ExprKind)((RIO_EXPR_SIZEOF_TYPE) + (1)))

#define RIO_EXPR_TYPEOF_TYPE ((rio_ExprKind)((RIO_EXPR_TYPEOF_EXPR) + (1)))

#define RIO_EXPR_ALIGNOF_EXPR ((rio_ExprKind)((RIO_EXPR_TYPEOF_TYPE) + (1)))

#define RIO_EXPR_ALIGNOF_TYPE ((rio_ExprKind)((RIO_EXPR_ALIGNOF_EXPR) + (1)))

#define RIO_EXPR_OFFSETOF ((rio_ExprKind)((RIO_EXPR_ALIGNOF_TYPE) + (1)))

typedef int rio_CompoundFieldKind;

#define RIO_FIELD_DEFAULT ((rio_CompoundFieldKind)(0))

#define RIO_FIELD_NAME ((rio_CompoundFieldKind)((RIO_FIELD_DEFAULT) + (1)))

#define RIO_FIELD_INDEX ((rio_CompoundFieldKind)((RIO_FIELD_NAME) + (1)))

#define RIO_STMT_NONE ((rio_StmtKind)(0))

#define RIO_STMT_DECL ((rio_StmtKind)((RIO_STMT_NONE) + (1)))

#define RIO_STMT_RETURN ((rio_StmtKind)((RIO_STMT_DECL) + (1)))

#define RIO_STMT_BREAK ((rio_StmtKind)((RIO_STMT_RETURN) + (1)))

#define RIO_STMT_CONTINUE ((rio_StmtKind)((RIO_STMT_BREAK) + (1)))

#define RIO_STMT_BLOCK ((rio_StmtKind)((RIO_STMT_CONTINUE) + (1)))

#define RIO_STMT_IF ((rio_StmtKind)((RIO_STMT_BLOCK) + (1)))

#define RIO_STMT_WHILE ((rio_StmtKind)((RIO_STMT_IF) + (1)))

#define RIO_STMT_DO_WHILE ((rio_StmtKind)((RIO_STMT_WHILE) + (1)))

#define RIO_STMT_FOR ((rio_StmtKind)((RIO_STMT_DO_WHILE) + (1)))

#define RIO_STMT_SWITCH ((rio_StmtKind)((RIO_STMT_FOR) + (1)))

#define RIO_STMT_ASSIGN ((rio_StmtKind)((RIO_STMT_SWITCH) + (1)))

#define RIO_STMT_INIT ((rio_StmtKind)((RIO_STMT_ASSIGN) + (1)))

#define RIO_STMT_EXPR ((rio_StmtKind)((RIO_STMT_INIT) + (1)))

#define RIO_STMT_NOTE ((rio_StmtKind)((RIO_STMT_EXPR) + (1)))

#define RIO_STMT_LABEL ((rio_StmtKind)((RIO_STMT_NOTE) + (1)))

#define RIO_STMT_GOTO ((rio_StmtKind)((RIO_STMT_LABEL) + (1)))

size_t rio_min(size_t x, size_t y);

size_t rio_max(size_t x, size_t y);

size_t rio_clamp_max(size_t x, size_t max);

size_t rio_clamp_min(size_t x, size_t min);

size_t rio_is_pow2(size_t x);

uintptr_t rio_align_down(uintptr_t n, size_t a);

uintptr_t rio_align_up(uintptr_t n, size_t a);

void (*rio_align_down_ptr(void (*p), size_t a));

void (*rio_align_up_ptr(void (*p), size_t a));

void rio_fatal(char const ((*fmt)), ...);

void (*rio_xcalloc(size_t num_elems, size_t elem_size));

void (*rio_xrealloc(void (*ptr), size_t num_bytes));

void (*rio_xmalloc(size_t num_bytes));

void (*rio_memdup(void (*src), size_t size));

char (*rio_strf(char const ((*fmt)), ...));

char (*rio_read_file(char const ((*path))));

bool rio_write_file(char const ((*path)), char const ((*buf)), size_t len);

rio_BufHdr (*rio_buf__hdr(void (*b)));

size_t rio_buf_len(void (*b));

size_t rio_buf_cap(void const ((*b)));

void (*rio_buf_end(void (*b), size_t elem_size));

size_t rio_buf_sizeof(void (*b), size_t elem_size);

void rio_buf_free(void (*(*b)));

void rio_buf_fit(void (*(*b)), size_t new_len, size_t elem_size);

void rio_buf_push(void (*(*b)), void (*elem), size_t elem_size);

void (*rio_buf__grow(void const ((*buf)), size_t new_len, size_t elem_size));

void rio_buf_printf(char (*(*buf)), char const ((*fmt)), ...);

extern rio_Arena rio_allocator;

#define RIO_ARENA_ALIGNMENT (8)

#define RIO_ARENA_BLOCK_SIZE ((1024) * (1024))

void rio_arena_grow(rio_Arena (*arena), size_t min_size);

void (*rio_arena_alloc(rio_Arena (*arena), size_t size));

void rio_arena_free(rio_Arena (*arena));

uint64_t rio_hash_uint64(uint64_t x);

uint64_t rio_hash_ptr(void const ((*ptr)));

uint64_t rio_hash_mix(uint64_t x, uint64_t y);

uint64_t rio_hash_bytes(void const ((*ptr)), size_t len);

uint64_t rio_map_get_uint64_from_uint64(rio_Map (*map), uint64_t key);

void rio_map_grow(rio_Map (*map), size_t new_cap);

void rio_map_put_uint64_from_uint64(rio_Map (*map), uint64_t key, uint64_t val);

void (*rio_map_get(rio_Map (*map), void const ((*key))));

void rio_map_put(rio_Map (*map), void const ((*key)), void (*val));

void (*rio_map_get_from_uint64(rio_Map (*map), uint64_t key));

void rio_map_put_from_uint64(rio_Map (*map), uint64_t key, void (*val));

uint64_t rio_map_get_uint64(rio_Map (*map), void (*key));

void rio_map_put_uint64(rio_Map (*map), void (*key), uint64_t val);

extern rio_Arena rio_intern_arena;

struct rio_Map {
    uint64_t (*keys);
    uint64_t (*vals);
    size_t len;
    size_t cap;
};

extern rio_Map rio_interns;

char const ((*rio_str_intern_range(char const ((*start)), char const ((*end)))));

char const ((*rio_str_intern(char const ((*str)))));

bool rio_str_islower(char const ((*str)));

extern char (*rio_gen_buf);

extern int rio_gen_indent;

extern rio_SrcPos rio_gen_pos;

extern char const ((*(*rio_gen_headers_buf)));

extern char (*rio_gen_preamble_str);

extern char (*rio_gen_postamble_str);

void rio_genln(void);

bool rio_is_incomplete_array_typespec(rio_Typespec (*typespec));

extern char (rio_char_to_escape[256]);

void rio_gen_char(char c);

void rio_gen_str(char const ((*str)), bool multiline);

void rio_gen_sync_pos(rio_SrcPos pos);

char const ((*rio_cdecl_paren(char const ((*str)), char c)));

char const ((*rio_cdecl_name(rio_Type (*type))));

char (*rio_type_to_cdecl(rio_Type (*type), char const ((*str))));

char const ((*rio_gen_expr_str(rio_Expr (*expr))));

extern rio_Map rio_gen_name_map;

char const ((*rio_get_gen_name_or_default(void const ((*ptr)), char const ((*default_name)))));

char const ((*rio_get_gen_name(void const ((*ptr)))));

char (*rio_typespec_to_cdecl(rio_Typespec (*typespec), char const ((*str))));

void rio_gen_func_decl(rio_Decl (*decl));

bool rio_gen_reachable(rio_Sym (*sym));

void rio_gen_forward_decls(void);

void rio_gen_aggregate_items(rio_Aggregate (*aggregate));

void rio_gen_aggregate(rio_Decl (*decl));

void rio_gen_paren_expr(rio_Expr (*expr));

void rio_gen_expr_compound(rio_Expr (*expr));

typedef int rio_CompilerTypeKind;

#define RIO_CMPL_TYPE_NONE ((rio_CompilerTypeKind)(0))

#define RIO_CMPL_TYPE_INCOMPLETE ((rio_CompilerTypeKind)((RIO_CMPL_TYPE_NONE) + (1)))

#define RIO_CMPL_TYPE_COMPLETING ((rio_CompilerTypeKind)((RIO_CMPL_TYPE_INCOMPLETE) + (1)))

#define RIO_CMPL_TYPE_VOID ((rio_CompilerTypeKind)((RIO_CMPL_TYPE_COMPLETING) + (1)))

#define RIO_CMPL_TYPE_BOOL ((rio_CompilerTypeKind)((RIO_CMPL_TYPE_VOID) + (1)))

#define RIO_CMPL_TYPE_CHAR ((rio_CompilerTypeKind)((RIO_CMPL_TYPE_BOOL) + (1)))

#define RIO_CMPL_TYPE_SCHAR ((rio_CompilerTypeKind)((RIO_CMPL_TYPE_CHAR) + (1)))

#define RIO_CMPL_TYPE_UCHAR ((rio_CompilerTypeKind)((RIO_CMPL_TYPE_SCHAR) + (1)))

#define RIO_CMPL_TYPE_SHORT ((rio_CompilerTypeKind)((RIO_CMPL_TYPE_UCHAR) + (1)))

#define RIO_CMPL_TYPE_USHORT ((rio_CompilerTypeKind)((RIO_CMPL_TYPE_SHORT) + (1)))

#define RIO_CMPL_TYPE_INT ((rio_CompilerTypeKind)((RIO_CMPL_TYPE_USHORT) + (1)))

#define RIO_CMPL_TYPE_UINT ((rio_CompilerTypeKind)((RIO_CMPL_TYPE_INT) + (1)))

#define RIO_CMPL_TYPE_LONG ((rio_CompilerTypeKind)((RIO_CMPL_TYPE_UINT) + (1)))

#define RIO_CMPL_TYPE_ULONG ((rio_CompilerTypeKind)((RIO_CMPL_TYPE_LONG) + (1)))

#define RIO_CMPL_TYPE_LLONG ((rio_CompilerTypeKind)((RIO_CMPL_TYPE_ULONG) + (1)))

#define RIO_CMPL_TYPE_ULLONG ((rio_CompilerTypeKind)((RIO_CMPL_TYPE_LLONG) + (1)))

#define RIO_CMPL_TYPE_ENUM ((rio_CompilerTypeKind)((RIO_CMPL_TYPE_ULLONG) + (1)))

#define RIO_CMPL_TYPE_FLOAT ((rio_CompilerTypeKind)((RIO_CMPL_TYPE_ENUM) + (1)))

#define RIO_CMPL_TYPE_DOUBLE ((rio_CompilerTypeKind)((RIO_CMPL_TYPE_FLOAT) + (1)))

#define RIO_CMPL_TYPE_PTR ((rio_CompilerTypeKind)((RIO_CMPL_TYPE_DOUBLE) + (1)))

#define RIO_CMPL_TYPE_FUNC ((rio_CompilerTypeKind)((RIO_CMPL_TYPE_PTR) + (1)))

#define RIO_CMPL_TYPE_ARRAY ((rio_CompilerTypeKind)((RIO_CMPL_TYPE_FUNC) + (1)))

#define RIO_CMPL_TYPE_STRUCT ((rio_CompilerTypeKind)((RIO_CMPL_TYPE_ARRAY) + (1)))

#define RIO_CMPL_TYPE_UNION ((rio_CompilerTypeKind)((RIO_CMPL_TYPE_STRUCT) + (1)))

#define RIO_CMPL_TYPE_CONST ((rio_CompilerTypeKind)((RIO_CMPL_TYPE_UNION) + (1)))

#define RIO_NUM_CMPL_TYPE_KINDS ((rio_CompilerTypeKind)((RIO_CMPL_TYPE_CONST) + (1)))

extern char const ((*(rio_typeid_kind_names[RIO_NUM_CMPL_TYPE_KINDS])));

char const ((*rio_typeid_kind_name(rio_Type (*type))));

bool rio_is_excluded_typeinfo(rio_Type (*type));

void rio_gen_typeid(rio_Type (*type));

void rio_gen_expr(rio_Expr (*expr));

void rio_gen_stmt_block(rio_StmtList block);

void rio_gen_simple_stmt(rio_Stmt (*stmt));

bool rio_is_char_lit(rio_Expr (*expr));

void rio_gen_stmt(rio_Stmt (*stmt));

void rio_gen_decl(rio_Sym (*sym));

void rio_gen_sorted_decls(void);

void rio_gen_defs(void);

extern rio_Map rio_gen_foreign_headers_map;

extern char const ((*(*rio_gen_foreign_headers_buf)));

void rio_add_foreign_header(char const ((*name)));

extern char const ((*(*rio_gen_foreign_sources_buf)));

void rio_add_foreign_source(char const ((*name)));

void rio_gen_include(char const ((*path)));

void rio_gen_foreign_headers(void);

void rio_gen_foreign_sources(void);

extern char const ((*(*rio_gen_sources_buf)));

void rio_put_include_path(char (path[MAX_PATH]), rio_Package (*package), char const ((*filename)));

extern char (*rio_gen_preamble_buf);

extern char (*rio_gen_postamble_buf);

void rio_preprocess_package(rio_Package (*package));

void rio_preprocess_packages(void);

void rio_gen_typeinfo_header(char const ((*kind)), rio_Type (*type));

void rio_gen_typeinfo_fields(rio_Type (*type));

void rio_gen_typeinfo(rio_Type (*type));

void rio_gen_typeinfos(void);

void rio_gen_package_external_names(void);

void rio_gen_preamble(void);

void rio_gen_postamble(void);

void rio_gen_all(void);

#define RIO_MAX_SEARCH_PATHS ((int)(256))

extern char const ((*(rio_static_package_search_paths[RIO_MAX_SEARCH_PATHS])));

extern char const ((*(*rio_package_search_paths)));

extern int rio_num_package_search_paths;

void rio_add_package_search_path(char const ((*path)));

void rio_add_package_search_path_range(char const ((*start)), char const ((*end)));

void rio_init_package_search_paths(void);

void rio_init_compiler(void);

void rio_parse_env_vars(void);

int rio_rio_main(int argc, char const ((*(*argv))), void (*gen_all)(void), char const ((*extension)));

extern char const ((*rio_typedef_keyword));

extern char const ((*rio_enum_keyword));

extern char const ((*rio_struct_keyword));

extern char const ((*rio_union_keyword));

extern char const ((*rio_let_keyword));

extern char const ((*rio_mut_keyword));

extern char const ((*rio_const_keyword));

extern char const ((*rio_fn_keyword));

extern char const ((*rio_sizeof_keyword));

extern char const ((*rio_alignof_keyword));

extern char const ((*rio_typeof_keyword));

extern char const ((*rio_offsetof_keyword));

extern char const ((*rio_break_keyword));

extern char const ((*rio_continue_keyword));

extern char const ((*rio_return_keyword));

extern char const ((*rio_if_keyword));

extern char const ((*rio_else_keyword));

extern char const ((*rio_while_keyword));

extern char const ((*rio_do_keyword));

extern char const ((*rio_for_keyword));

extern char const ((*rio_switch_keyword));

extern char const ((*rio_case_keyword));

extern char const ((*rio_default_keyword));

extern char const ((*rio_import_keyword));

extern char const ((*rio_goto_keyword));

extern char const ((*rio_first_keyword));

extern char const ((*rio_last_keyword));

extern char const ((*(*rio_keywords)));

extern char const ((*rio_always_name));

extern char const ((*rio_foreign_name));

extern char const ((*rio_complete_name));

extern char const ((*rio_assert_name));

extern char const ((*rio_declare_note_name));

extern char const ((*rio_static_assert_name));

char const ((*rio_init_keyword(char const ((*keyword)))));

extern bool rio_keywords_inited;

void rio_init_keywords(void);

bool rio_is_keyword_name(char const ((*name)));

#define RIO_TOKEN_EOF ((rio_TokenKind)(0))

#define RIO_TOKEN_COLON ((rio_TokenKind)((RIO_TOKEN_EOF) + (1)))

#define RIO_TOKEN_LPAREN ((rio_TokenKind)((RIO_TOKEN_COLON) + (1)))

#define RIO_TOKEN_RPAREN ((rio_TokenKind)((RIO_TOKEN_LPAREN) + (1)))

#define RIO_TOKEN_LBRACE ((rio_TokenKind)((RIO_TOKEN_RPAREN) + (1)))

#define RIO_TOKEN_RBRACE ((rio_TokenKind)((RIO_TOKEN_LBRACE) + (1)))

#define RIO_TOKEN_LBRACKET ((rio_TokenKind)((RIO_TOKEN_RBRACE) + (1)))

#define RIO_TOKEN_RBRACKET ((rio_TokenKind)((RIO_TOKEN_LBRACKET) + (1)))

#define RIO_TOKEN_COMMA ((rio_TokenKind)((RIO_TOKEN_RBRACKET) + (1)))

#define RIO_TOKEN_DOT ((rio_TokenKind)((RIO_TOKEN_COMMA) + (1)))

#define RIO_TOKEN_AT ((rio_TokenKind)((RIO_TOKEN_DOT) + (1)))

#define RIO_TOKEN_POUND ((rio_TokenKind)((RIO_TOKEN_AT) + (1)))

#define RIO_TOKEN_ELLIPSIS ((rio_TokenKind)((RIO_TOKEN_POUND) + (1)))

#define RIO_TOKEN_QUESTION ((rio_TokenKind)((RIO_TOKEN_ELLIPSIS) + (1)))

#define RIO_TOKEN_SEMICOLON ((rio_TokenKind)((RIO_TOKEN_QUESTION) + (1)))

#define RIO_TOKEN_KEYWORD ((rio_TokenKind)((RIO_TOKEN_SEMICOLON) + (1)))

#define RIO_TOKEN_INT ((rio_TokenKind)((RIO_TOKEN_KEYWORD) + (1)))

#define RIO_TOKEN_FLOAT ((rio_TokenKind)((RIO_TOKEN_INT) + (1)))

#define RIO_TOKEN_STR ((rio_TokenKind)((RIO_TOKEN_FLOAT) + (1)))

#define RIO_TOKEN_NAME ((rio_TokenKind)((RIO_TOKEN_STR) + (1)))

#define RIO_TOKEN_NEG ((rio_TokenKind)((RIO_TOKEN_NAME) + (1)))

#define RIO_TOKEN_NOT ((rio_TokenKind)((RIO_TOKEN_NEG) + (1)))

#define RIO_TOKEN_FIRST_MUL ((rio_TokenKind)((RIO_TOKEN_NOT) + (1)))

#define RIO_TOKEN_MUL ((rio_TokenKind)(RIO_TOKEN_FIRST_MUL))

#define RIO_TOKEN_DIV ((rio_TokenKind)((RIO_TOKEN_MUL) + (1)))

#define RIO_TOKEN_MOD ((rio_TokenKind)((RIO_TOKEN_DIV) + (1)))

#define RIO_TOKEN_AND ((rio_TokenKind)((RIO_TOKEN_MOD) + (1)))

#define RIO_TOKEN_LSHIFT ((rio_TokenKind)((RIO_TOKEN_AND) + (1)))

#define RIO_TOKEN_RSHIFT ((rio_TokenKind)((RIO_TOKEN_LSHIFT) + (1)))

#define RIO_TOKEN_LAST_MUL ((rio_TokenKind)(RIO_TOKEN_RSHIFT))

#define RIO_TOKEN_FIRST_ADD ((rio_TokenKind)((RIO_TOKEN_LAST_MUL) + (1)))

#define RIO_TOKEN_ADD ((rio_TokenKind)(RIO_TOKEN_FIRST_ADD))

#define RIO_TOKEN_SUB ((rio_TokenKind)((RIO_TOKEN_ADD) + (1)))

#define RIO_TOKEN_XOR ((rio_TokenKind)((RIO_TOKEN_SUB) + (1)))

#define RIO_TOKEN_OR ((rio_TokenKind)((RIO_TOKEN_XOR) + (1)))

#define RIO_TOKEN_LAST_ADD ((rio_TokenKind)(RIO_TOKEN_OR))

#define RIO_TOKEN_FIRST_CMP ((rio_TokenKind)((RIO_TOKEN_LAST_ADD) + (1)))

#define RIO_TOKEN_EQ ((rio_TokenKind)(RIO_TOKEN_FIRST_CMP))

#define RIO_TOKEN_NOTEQ ((rio_TokenKind)((RIO_TOKEN_EQ) + (1)))

#define RIO_TOKEN_LT ((rio_TokenKind)((RIO_TOKEN_NOTEQ) + (1)))

#define RIO_TOKEN_GT ((rio_TokenKind)((RIO_TOKEN_LT) + (1)))

#define RIO_TOKEN_LTEQ ((rio_TokenKind)((RIO_TOKEN_GT) + (1)))

#define RIO_TOKEN_GTEQ ((rio_TokenKind)((RIO_TOKEN_LTEQ) + (1)))

#define RIO_TOKEN_LAST_CMP ((rio_TokenKind)(RIO_TOKEN_GTEQ))

#define RIO_TOKEN_AND_AND ((rio_TokenKind)((RIO_TOKEN_LAST_CMP) + (1)))

#define RIO_TOKEN_OR_OR ((rio_TokenKind)((RIO_TOKEN_AND_AND) + (1)))

#define RIO_TOKEN_FIRST_ASSIGN ((rio_TokenKind)((RIO_TOKEN_OR_OR) + (1)))

#define RIO_TOKEN_ASSIGN ((rio_TokenKind)(RIO_TOKEN_FIRST_ASSIGN))

#define RIO_TOKEN_ADD_ASSIGN ((rio_TokenKind)((RIO_TOKEN_ASSIGN) + (1)))

#define RIO_TOKEN_SUB_ASSIGN ((rio_TokenKind)((RIO_TOKEN_ADD_ASSIGN) + (1)))

#define RIO_TOKEN_OR_ASSIGN ((rio_TokenKind)((RIO_TOKEN_SUB_ASSIGN) + (1)))

#define RIO_TOKEN_AND_ASSIGN ((rio_TokenKind)((RIO_TOKEN_OR_ASSIGN) + (1)))

#define RIO_TOKEN_XOR_ASSIGN ((rio_TokenKind)((RIO_TOKEN_AND_ASSIGN) + (1)))

#define RIO_TOKEN_LSHIFT_ASSIGN ((rio_TokenKind)((RIO_TOKEN_XOR_ASSIGN) + (1)))

#define RIO_TOKEN_RSHIFT_ASSIGN ((rio_TokenKind)((RIO_TOKEN_LSHIFT_ASSIGN) + (1)))

#define RIO_TOKEN_MUL_ASSIGN ((rio_TokenKind)((RIO_TOKEN_RSHIFT_ASSIGN) + (1)))

#define RIO_TOKEN_DIV_ASSIGN ((rio_TokenKind)((RIO_TOKEN_MUL_ASSIGN) + (1)))

#define RIO_TOKEN_MOD_ASSIGN ((rio_TokenKind)((RIO_TOKEN_DIV_ASSIGN) + (1)))

#define RIO_TOKEN_LAST_ASSIGN ((rio_TokenKind)(RIO_TOKEN_MOD_ASSIGN))

#define RIO_TOKEN_INC ((rio_TokenKind)((RIO_TOKEN_LAST_ASSIGN) + (1)))

#define RIO_TOKEN_DEC ((rio_TokenKind)((RIO_TOKEN_INC) + (1)))

#define RIO_NUM_TOKEN_KINDS ((rio_TokenKind)((RIO_TOKEN_DEC) + (1)))

#define RIO_MOD_NONE ((rio_TokenMod)(0))

#define RIO_MOD_HEX ((rio_TokenMod)((RIO_MOD_NONE) + (1)))

#define RIO_MOD_BIN ((rio_TokenMod)((RIO_MOD_HEX) + (1)))

#define RIO_MOD_OCT ((rio_TokenMod)((RIO_MOD_BIN) + (1)))

#define RIO_MOD_CHAR ((rio_TokenMod)((RIO_MOD_OCT) + (1)))

#define RIO_MOD_MULTILINE ((rio_TokenMod)((RIO_MOD_CHAR) + (1)))

#define RIO_SUFFIX_NONE ((rio_TokenSuffix)(0))

#define RIO_SUFFIX_D ((rio_TokenSuffix)((RIO_SUFFIX_NONE) + (1)))

#define RIO_SUFFIX_U ((rio_TokenSuffix)((RIO_SUFFIX_D) + (1)))

#define RIO_SUFFIX_L ((rio_TokenSuffix)((RIO_SUFFIX_U) + (1)))

#define RIO_SUFFIX_UL ((rio_TokenSuffix)((RIO_SUFFIX_L) + (1)))

#define RIO_SUFFIX_LL ((rio_TokenSuffix)((RIO_SUFFIX_UL) + (1)))

#define RIO_SUFFIX_ULL ((rio_TokenSuffix)((RIO_SUFFIX_LL) + (1)))

extern char const ((*(rio_token_suffix_names[7])));

extern char const ((*(rio_token_kind_names[53])));

char const ((*rio_token_kind_name(rio_TokenKind kind)));

extern rio_TokenKind (rio_assign_token_to_binary_token[RIO_NUM_TOKEN_KINDS]);

extern rio_SrcPos rio_pos_builtin;

struct rio_Token {
    rio_TokenKind kind;
    rio_TokenMod mod;
    rio_TokenSuffix suffix;
    rio_SrcPos pos;
    char const ((*start));
    char const ((*end));
    union {
        ullong int_val;
        double float_val;
        char const ((*str_val));
        char const ((*name));
    };
};

extern rio_Token rio_token;

extern char const ((*rio_stream));

extern char const ((*rio_line_start));

void rio_vnotice(char const ((*level)), rio_SrcPos pos, char const ((*fmt)), va_list args);

void rio_warning(rio_SrcPos pos, char const ((*fmt)), ...);

void rio_verror(rio_SrcPos pos, char const ((*fmt)), va_list args);

void rio_error(rio_SrcPos pos, char const ((*fmt)), ...);

void rio_fatal_error(rio_SrcPos pos, char const ((*fmt)), ...);

char const ((*rio_token_info(void)));

extern uint8_t (rio_char_to_digit[256]);

void rio_scan_int(void);

void rio_scan_float(void);

extern char (rio_escape_to_char[256]);

int rio_scan_hex_escape(void);

void rio_scan_char(void);

void rio_scan_str(void);

void rio_next_token(void);

void rio_init_stream(char const ((*name)), char const ((*buf)));

bool rio_is_token(rio_TokenKind kind);

bool rio_is_token_eof(void);

bool rio_is_token_name(char const ((*name)));

bool rio_is_keyword(char const ((*name)));

bool rio_match_keyword(char const ((*name)));

bool rio_match_token(rio_TokenKind kind);

bool rio_expect_token(rio_TokenKind kind);

extern bool rio_flag_verbose;

extern bool rio_flag_lazy;

extern bool rio_flag_nosourcemap;

extern bool rio_flag_notypeinfo;

extern bool rio_flag_fullgen;

void rio_path_normalize(char (*path));

void rio_path_copy(char (path[MAX_PATH]), char const ((*src)));

void rio_path_join(char (path[MAX_PATH]), char const ((*src)));

char (*rio_path_file(char (path[MAX_PATH])));

char (*rio_path_ext(char (path[MAX_PATH])));

bool rio_dir_excluded(rio_DirListIter (*iter));

bool rio_dir_list_subdir(rio_DirListIter (*iter));

char const ((*(*rio_dir_list_buf(char const ((*filespec))))));

typedef int rio_FlagKind;

#define RIO_FLAG_BOOL ((rio_FlagKind)(0))

#define RIO_FLAG_STR ((rio_FlagKind)((RIO_FLAG_BOOL) + (1)))

#define RIO_FLAG_ENUM ((rio_FlagKind)((RIO_FLAG_STR) + (1)))

extern rio_FlagDef (*rio_flag_defs);

void rio_add_flag_bool(char const ((*name)), bool (*ptr), char const ((*help)));

void rio_add_flag_str(char const ((*name)), char const ((*(*ptr))), char const ((*arg_name)), char const ((*help)));

void rio_add_flag_enum(char const ((*name)), int (*ptr), char const ((*help)), char const ((*(*options))), int num_options);

rio_FlagDef (*rio_get_flag_def(char const ((*name))));

void rio_print_flags_usage(void);

char const ((*rio_parse_flags(int (*argc_ptr), char const ((*(*(*argv_ptr)))))));

void rio_path_absolute(char (path[MAX_PATH]));

void rio_dir_list_free(rio_DirListIter (*iter));

void rio_dir_list_next(rio_DirListIter (*iter));

void rio_dir_list(rio_DirListIter (*iter), char const ((*path)));

rio_Typespec (*rio_parse_type_func_param(void));

rio_Typespec (*rio_parse_type_func(void));

rio_Typespec (*rio_parse_type_base(void));

rio_Typespec (*rio_parse_type(void));

struct rio_CompoundField {
    rio_CompoundFieldKind kind;
    rio_SrcPos pos;
    rio_Expr (*init);
    union {
        char const ((*name));
        rio_Expr (*index);
    };
};

rio_CompoundField rio_parse_expr_compound_field(void);

rio_Expr (*rio_parse_expr_compound(rio_Typespec (*type)));

rio_Expr (*rio_parse_expr_operand(void));

rio_Expr (*rio_parse_expr_base(void));

bool rio_is_unary_op(void);

rio_Expr (*rio_parse_expr_unary(void));

bool rio_is_mul_op(void);

rio_Expr (*rio_parse_expr_mul(void));

bool rio_is_add_op(void);

rio_Expr (*rio_parse_expr_add(void));

bool rio_is_cmp_op(void);

rio_Expr (*rio_parse_expr_cmp(void));

rio_Expr (*rio_parse_expr_and(void));

rio_Expr (*rio_parse_expr_or(void));

rio_Expr (*rio_parse_expr_ternary(void));

rio_Expr (*rio_parse_expr(void));

rio_Expr (*rio_parse_paren_expr(void));

rio_StmtList rio_parse_stmt_block(void);

rio_Stmt (*rio_parse_stmt_if(rio_SrcPos pos));

rio_Stmt (*rio_parse_stmt_while(rio_SrcPos pos));

rio_Stmt (*rio_parse_stmt_do_while(rio_SrcPos pos));

bool rio_is_assign_op(void);

rio_Stmt (*rio_parse_let_stmt(rio_SrcPos pos));

rio_Stmt (*rio_parse_simple_stmt(void));

rio_Stmt (*rio_parse_stmt_for(rio_SrcPos pos));

struct rio_SwitchCasePattern {
    rio_Expr (*start);
    rio_Expr (*end);
};

rio_SwitchCasePattern rio_parse_switch_case_pattern(void);

struct rio_SwitchCase {
    rio_SwitchCasePattern (*patterns);
    size_t num_patterns;
    bool is_default;
    rio_StmtList block;
};

rio_SwitchCase rio_parse_stmt_switch_case(void);

rio_Stmt (*rio_parse_stmt_switch(rio_SrcPos pos));

rio_Stmt (*rio_parse_stmt(void));

char const ((*rio_parse_name(void)));

struct rio_EnumItem {
    rio_SrcPos pos;
    char const ((*name));
    rio_Expr (*init);
};

rio_EnumItem rio_parse_decl_enum_item(void);

rio_Decl (*rio_parse_decl_enum(rio_SrcPos pos));

struct rio_AggregateItem {
    rio_SrcPos pos;
    rio_AggregateItemKind kind;
    union {
        struct {
            char const ((*(*names)));
            size_t num_names;
            rio_Typespec (*type);
        };
        rio_Aggregate (*subaggregate);
    };
};

rio_AggregateItem rio_parse_decl_aggregate_item(void);

rio_Aggregate (*rio_parse_aggregate(rio_AggregateKind kind));

rio_Decl (*rio_parse_decl_aggregate(rio_SrcPos pos, rio_DeclKind kind));

rio_Decl (*rio_parse_decl_var(rio_SrcPos pos));

rio_Decl (*rio_parse_decl_const(rio_SrcPos pos));

rio_Decl (*rio_parse_decl_typedef(rio_SrcPos pos));

struct rio_FuncParam {
    rio_SrcPos pos;
    char const ((*name));
    rio_Typespec (*type);
};

rio_FuncParam rio_parse_decl_func_param(void);

rio_Decl (*rio_parse_decl_func(rio_SrcPos pos));

struct rio_NoteArg {
    rio_SrcPos pos;
    char const ((*name));
    rio_Expr (*expr);
};

rio_NoteArg rio_parse_note_arg(void);

rio_Note rio_parse_note(void);

rio_Notes rio_parse_notes(void);

rio_Decl (*rio_parse_decl_note(rio_SrcPos pos));

rio_Decl (*rio_parse_decl_import(rio_SrcPos pos));

rio_Decl (*rio_parse_decl_opt(void));

rio_Decl (*rio_parse_decl(void));

rio_Decls (*rio_parse_decls(void));

typedef int rio_SymKind;

#define RIO_SYM_NONE ((rio_SymKind)(0))

#define RIO_SYM_VAR ((rio_SymKind)((RIO_SYM_NONE) + (1)))

#define RIO_SYM_CONST ((rio_SymKind)((RIO_SYM_VAR) + (1)))

#define RIO_SYM_FUNC ((rio_SymKind)((RIO_SYM_CONST) + (1)))

#define RIO_SYM_TYPE ((rio_SymKind)((RIO_SYM_FUNC) + (1)))

#define RIO_SYM_PACKAGE ((rio_SymKind)((RIO_SYM_TYPE) + (1)))

typedef int rio_SymState;

#define RIO_SYM_UNRESOLVED ((rio_SymState)(0))

#define RIO_SYM_RESOLVING ((rio_SymState)((RIO_SYM_UNRESOLVED) + (1)))

#define RIO_SYM_RESOLVED ((rio_SymState)((RIO_SYM_RESOLVING) + (1)))

#define RIO_MAX_LOCAL_SYMS ((int)(1024))

extern rio_Package (*rio_current_package);

extern rio_Package (*rio_builtin_package);

extern rio_Map rio_package_map;

extern rio_Package (*(*rio_package_list));

#define RIO_REACHABLE_NONE ((int)(0))

#define RIO_REACHABLE_NATURAL ((int)((RIO_REACHABLE_NONE) + (1)))

#define RIO_REACHABLE_FORCED ((int)((RIO_REACHABLE_NATURAL) + (1)))

extern uint8_t rio_reachable_phase;

rio_Sym (*rio_get_package_sym(rio_Package (*package), char const ((*name))));

void rio_add_package(rio_Package (*package));

rio_Package (*rio_enter_package(rio_Package (*new_package)));

void rio_leave_package(rio_Package (*old_package));

extern rio_Sym (*(*rio_reachable_syms));

extern rio_Sym (*(*rio_sorted_syms));

union rio_Val {
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

struct rio_Sym {
    char const ((*name));
    rio_Package (*home_package);
    rio_SymKind kind;
    rio_SymState state;
    uint8_t reachable;
    rio_Decl (*decl);
    char const ((*external_name));
    union {
        struct {
            rio_Type (*type);
            rio_Val val;
        };
        rio_Package (*package);
    };
};

extern rio_Sym (rio_local_syms[RIO_MAX_LOCAL_SYMS]);

extern rio_Sym (*rio_local_syms_end);

bool rio_is_local_sym(rio_Sym (*sym));

rio_Sym (*rio_sym_new(rio_SymKind kind, char const ((*name)), rio_Decl (*decl)));

void rio_process_decl_notes(rio_Decl (*decl), rio_Sym (*sym));

rio_Sym (*rio_sym_decl(rio_Decl (*decl)));

rio_Sym (*rio_sym_get_local(char const ((*name))));

rio_Sym (*rio_sym_get(char const ((*name))));

bool rio_sym_push_var(char const ((*name)), rio_Type (*type));

rio_Sym (*rio_sym_enter(void));

void rio_sym_leave(rio_Sym (*sym));

void rio_sym_global_put(char const ((*name)), rio_Sym (*sym));

rio_Sym (*rio_sym_global_type(char const ((*name)), rio_Type (*type)));

rio_Sym (*rio_sym_global_decl(rio_Decl (*decl)));

void rio_put_type_name(char (*(*buf)), rio_Type (*type));

char (*rio_get_type_name(rio_Type (*type)));

struct rio_Operand {
    rio_Type (*type);
    bool is_lvalue;
    bool is_const;
    rio_Val val;
};

extern rio_Operand rio_operand_null;

rio_Operand rio_operand_rvalue(rio_Type (*type));

rio_Operand rio_operand_lvalue(rio_Type (*type));

rio_Operand rio_operand_const(rio_Type (*type), rio_Val val);

rio_Type (*rio_type_decay(rio_Type (*type)));

rio_Operand rio_operand_decay(rio_Operand operand);

bool rio_is_convertible(rio_Operand (*operand), rio_Type (*dest));

bool rio_is_castable(rio_Operand (*operand), rio_Type (*dest));

bool rio_convert_operand(rio_Operand (*operand), rio_Type (*type));

bool rio_is_null_ptr(rio_Operand operand);

void rio_promote_operand(rio_Operand (*operand));

void rio_unify_arithmetic_operands(rio_Operand (*left), rio_Operand (*right));

extern rio_Map rio_resolved_val_map;

rio_Val rio_get_resolved_val(void (*ptr));

void rio_set_resolved_val(void (*ptr), rio_Val val);

extern rio_Map rio_resolved_type_map;

rio_Type (*rio_get_resolved_type(void (*ptr)));

void rio_set_resolved_type(void (*ptr), rio_Type (*type));

extern rio_Map rio_resolved_sym_map;

rio_Sym (*rio_get_resolved_sym(void const ((*ptr))));

void rio_set_resolved_sym(void const ((*ptr)), rio_Sym (*sym));

extern rio_Map rio_resolved_expected_type_map;

rio_Type (*rio_get_resolved_expected_type(rio_Expr (*expr)));

void rio_set_resolved_expected_type(rio_Expr (*expr), rio_Type (*type));

rio_Operand rio_resolve_expr(rio_Expr (*expr));

rio_Operand rio_resolve_expr_rvalue(rio_Expr (*expr));

rio_Operand rio_resolve_expected_expr_rvalue(rio_Expr (*expr), rio_Type (*expected_type));

rio_Type (*rio_resolve_typespec(rio_Typespec (*typespec)));

rio_Type (*rio_complete_aggregate(rio_Type (*type), rio_Aggregate (*aggregate)));

void rio_complete_type(rio_Type (*type));

rio_Type (*rio_resolve_typed_init(rio_SrcPos pos, rio_Type (*type), rio_Expr (*expr)));

rio_Type (*rio_resolve_init(rio_SrcPos pos, rio_Typespec (*typespec), rio_Expr (*expr)));

rio_Type (*rio_resolve_decl_var(rio_Decl (*decl)));

rio_Type (*rio_resolve_decl_const(rio_Decl (*decl), rio_Val (*val)));

rio_Type (*rio_resolve_decl_func(rio_Decl (*decl)));

#define RIO_MAX_LABELS (256)

struct rio_Label {
    char const ((*name));
    rio_SrcPos pos;
    bool referenced;
    bool defined;
};

extern rio_Label (rio_labels[RIO_MAX_LABELS]);

extern rio_Label (*rio_labels_end);

rio_Label (*rio_get_label(rio_SrcPos pos, char const ((*name))));

void rio_reference_label(rio_SrcPos pos, char const ((*name)));

void rio_define_label(rio_SrcPos pos, char const ((*name)));

void rio_resolve_labels(void);

bool rio_is_cond_operand(rio_Operand operand);

void rio_resolve_cond_expr(rio_Expr (*expr));

struct rio_StmtCtx {
    bool is_break_legal;
    bool is_continue_legal;
};

bool rio_resolve_stmt_block(rio_StmtList block, rio_Type (*ret_type), rio_StmtCtx ctx);

void rio_resolve_stmt_assign(rio_Stmt (*stmt));

void rio_resolve_stmt_init(rio_Stmt (*stmt));

void rio_resolve_static_assert(rio_Note note);

bool rio_resolve_stmt(rio_Stmt (*stmt), rio_Type (*ret_type), rio_StmtCtx ctx);

void rio_resolve_func_body(rio_Sym (*sym));

void rio_resolve_sym(rio_Sym (*sym));

void rio_finalize_sym(rio_Sym (*sym));

rio_Sym (*rio_resolve_name(char const ((*name))));

rio_Package (*rio_try_resolve_package(rio_Expr (*expr)));

rio_Operand rio_resolve_expr_field(rio_Expr (*expr));

llong rio_eval_unary_op_ll(rio_TokenKind op, llong val);

ullong rio_eval_unary_op_ull(rio_TokenKind op, ullong val);

llong rio_eval_binary_op_ll(rio_TokenKind op, llong left, llong right);

ullong rio_eval_binary_op_ull(rio_TokenKind op, ullong left, ullong right);

rio_Val rio_eval_unary_op(rio_TokenKind op, rio_Type (*type), rio_Val val);

rio_Val rio_eval_binary_op(rio_TokenKind op, rio_Type (*type), rio_Val left, rio_Val right);

rio_Operand rio_resolve_name_operand(rio_SrcPos pos, char const ((*name)));

rio_Operand rio_resolve_expr_name(rio_Expr (*expr));

rio_Operand rio_resolve_unary_op(rio_TokenKind op, rio_Operand operand);

rio_Operand rio_resolve_expr_unary(rio_Expr (*expr));

rio_Operand rio_resolve_binary_op(rio_TokenKind op, rio_Operand left, rio_Operand right);

rio_Operand rio_resolve_binary_arithmetic_op(rio_TokenKind op, rio_Operand left, rio_Operand right);

rio_Operand rio_resolve_expr_binary_op(rio_TokenKind op, char const ((*op_name)), rio_SrcPos pos, rio_Operand left, rio_Operand right);

rio_Operand rio_resolve_expr_binary(rio_Expr (*expr));

rio_Operand rio_resolve_expr_compound(rio_Expr (*expr), rio_Type (*expected_type));

rio_Operand rio_resolve_expr_call(rio_Expr (*expr));

rio_Operand rio_resolve_expr_ternary(rio_Expr (*expr), rio_Type (*expected_type));

rio_Operand rio_resolve_expr_index(rio_Expr (*expr));

rio_Operand rio_resolve_expr_cast(rio_Expr (*expr));

rio_Operand rio_resolve_expr_int(rio_Expr (*expr));

rio_Operand rio_resolve_expr_modify(rio_Expr (*expr));

rio_Operand rio_resolve_expected_expr(rio_Expr (*expr), rio_Type (*expected_type));

rio_Operand rio_resolve_const_expr(rio_Expr (*expr));

extern rio_Map rio_decl_note_names;

void rio_init_builtin_syms(void);

void rio_add_package_decls(rio_Package (*package));

bool rio_is_package_dir(char const ((*search_path)), char const ((*package_path)));

bool rio_copy_package_full_path(char (dest[MAX_PATH]), char const ((*package_path)));

rio_Package (*rio_import_package(char const ((*package_path))));

void rio_import_all_package_symbols(rio_Package (*package));

void rio_import_package_symbols(rio_Decl (*decl), rio_Package (*package));

void rio_process_package_imports(rio_Package (*package));

bool rio_parse_package(rio_Package (*package));

bool rio_compile_package(rio_Package (*package));

void rio_resolve_package_syms(rio_Package (*package));

void rio_finalize_reachable_syms(void);

typedef int rio_Os;

#define RIO_OS_WIN32 ((rio_Os)(0))

#define RIO_OS_LINUX ((rio_Os)((RIO_OS_WIN32) + (1)))

#define RIO_OS_OSX ((rio_Os)((RIO_OS_LINUX) + (1)))

#define RIO_NUM_OSES ((rio_Os)((RIO_OS_OSX) + (1)))

extern char const ((*(rio_os_names[RIO_NUM_OSES])));

typedef int rio_Arch;

#define RIO_ARCH_X64 ((rio_Arch)(0))

#define RIO_ARCH_X86 ((rio_Arch)((RIO_ARCH_X64) + (1)))

#define RIO_NUM_ARCHES ((rio_Arch)((RIO_ARCH_X86) + (1)))

extern char const ((*(rio_arch_names[RIO_NUM_ARCHES])));

extern int rio_target_os;

extern int rio_target_arch;

int rio_get_os(char const ((*name)));

int rio_get_arch(char const ((*name)));

struct rio_TypeMetrics {
    size_t size;
    size_t align;
    bool sign;
    ullong max;
};

void rio_init_default_type_metrics(rio_TypeMetrics (metrics[RIO_NUM_CMPL_TYPE_KINDS]));

extern rio_TypeMetrics (rio_win32_x86_metrics[RIO_NUM_CMPL_TYPE_KINDS]);

extern rio_TypeMetrics (rio_win32_x64_metrics[RIO_NUM_CMPL_TYPE_KINDS]);

extern rio_TypeMetrics (rio_ilp32_metrics[RIO_NUM_CMPL_TYPE_KINDS]);

extern rio_TypeMetrics (rio_lp64_metrics[RIO_NUM_CMPL_TYPE_KINDS]);

void rio_init_target(void);

bool rio_is_excluded_target_filename(char const ((*name)));

extern rio_TypeMetrics (*rio_type_metrics);

struct rio_TypeAggregate {
    rio_TypeField (*fields);
    size_t num_fields;
};

struct rio_TypeFunc {
    rio_Type (*(*params));
    size_t num_params;
    bool has_varargs;
    rio_Type (*ret);
};

struct rio_Type {
    rio_CompilerTypeKind kind;
    size_t size;
    size_t align;
    rio_Sym (*sym);
    rio_Type (*base);
    int typeid;
    bool nonmodifiable;
    union {
        size_t num_elems;
        rio_TypeAggregate aggregate;
        rio_TypeFunc function;
    };
};

extern rio_Type (*rio_type_void);

extern rio_Type (*rio_type_bool);

extern rio_Type (*rio_type_char);

extern rio_Type (*rio_type_uchar);

extern rio_Type (*rio_type_schar);

extern rio_Type (*rio_type_short);

extern rio_Type (*rio_type_ushort);

extern rio_Type (*rio_type_int);

extern rio_Type (*rio_type_uint);

extern rio_Type (*rio_type_long);

extern rio_Type (*rio_type_ulong);

extern rio_Type (*rio_type_llong);

extern rio_Type (*rio_type_ullong);

extern rio_Type (*rio_type_float);

extern rio_Type (*rio_type_double);

extern int rio_next_typeid;

extern rio_Type (*rio_type_uintptr);

extern rio_Type (*rio_type_usize);

extern rio_Type (*rio_type_ssize);

extern rio_Map rio_typeid_map;

rio_Type (*rio_get_type_from_typeid(int typeid));

void rio_register_typeid(rio_Type (*type));

rio_Type (*rio_type_alloc(TypeKind kind));

bool rio_is_ptr_type(rio_Type (*type));

bool rio_is_func_type(rio_Type (*type));

bool rio_is_ptr_like_type(rio_Type (*type));

bool rio_is_const_type(rio_Type (*type));

bool rio_is_array_type(rio_Type (*type));

bool rio_is_incomplete_array_type(rio_Type (*type));

bool rio_is_integer_type(rio_Type (*type));

bool rio_is_floating_type(rio_Type (*type));

bool rio_is_arithmetic_type(rio_Type (*type));

bool rio_is_scalar_type(rio_Type (*type));

bool rio_is_aggregate_type(rio_Type (*type));

bool rio_is_signed_type(rio_Type (*type));

extern char const ((*(rio_type_names[RIO_NUM_CMPL_TYPE_KINDS])));

extern int (rio_type_ranks[RIO_NUM_CMPL_TYPE_KINDS]);

int rio_type_rank(rio_Type (*type));

rio_Type (*rio_unsigned_type(rio_Type (*type)));

size_t rio_type_sizeof(rio_Type (*type));

size_t rio_type_alignof(rio_Type (*type));

extern rio_Map rio_cached_ptr_types;

rio_Type (*rio_type_ptr(rio_Type (*base)));

extern rio_Map rio_cached_const_types;

rio_Type (*rio_type_const(rio_Type (*base)));

rio_Type (*rio_unqualify_type(rio_Type (*type)));

extern rio_Map rio_cached_array_types;

rio_Type (*rio_type_array(rio_Type (*base), size_t num_elems));

extern rio_Map rio_cached_func_types;

rio_Type (*rio_type_func(rio_Type (*(*params)), size_t num_params, rio_Type (*ret), bool has_varargs));

bool rio_has_duplicate_fields(rio_Type (*type));

void rio_add_type_fields(rio_TypeField (*(*fields)), rio_Type (*type), size_t offset);

void rio_type_complete_struct(rio_Type (*type), rio_TypeField (*fields), size_t num_fields);

void rio_type_complete_union(rio_Type (*type), rio_TypeField (*fields), size_t num_fields);

rio_Type (*rio_type_incomplete(rio_Sym (*sym)));

rio_Type (*rio_type_enum(rio_Sym (*sym), rio_Type (*base)));

void rio_init_builtin_type(rio_Type (*type));

void rio_init_builtin_types(void);

int rio_aggregate_item_field_index(rio_Type (*type), char const ((*name)));

rio_Type (*rio_aggregate_item_field_type_from_index(rio_Type (*type), int index));

rio_Type (*rio_aggregate_item_field_type_from_name(rio_Type (*type), char const ((*name))));


struct TypeFieldInfo {
    char const ((*name));
    typeid type;
    int offset;
};

struct Any {
    void (*ptr);
    typeid type;
};

struct rio_StmtIf {
    rio_Stmt (*init);
    rio_Expr (*cond);
    rio_StmtList then_block;
    rio_ElseIf (*elseifs);
    size_t num_elseifs;
    rio_StmtList else_block;
};

struct rio_StmtWhile {
    rio_Expr (*cond);
    rio_StmtList block;
};

struct rio_StmtFor {
    rio_Stmt (*init);
    rio_Expr (*cond);
    rio_Stmt (*next);
    rio_StmtList block;
};

struct rio_StmtSwitch {
    rio_Expr (*expr);
    rio_SwitchCase (*cases);
    size_t num_cases;
};

struct rio_StmtAssign {
    rio_TokenKind op;
    rio_Expr (*left);
    rio_Expr (*right);
};

struct rio_StmtInit {
    char const ((*name));
    bool is_mut;
    rio_Typespec (*type);
    rio_Expr (*expr);
};

struct rio_Stmt {
    rio_StmtKind kind;
    rio_Notes notes;
    rio_SrcPos pos;
    union {
        rio_Note note;
        rio_Expr (*expr);
        rio_Decl (*decl);
        rio_StmtIf if_stmt;
        rio_StmtWhile while_stmt;
        rio_StmtFor for_stmt;
        rio_StmtSwitch switch_stmt;
        rio_StmtList block;
        rio_StmtAssign assign;
        rio_StmtInit init;
        char const ((*label));
    };
};

struct rio_TypespecFunc {
    rio_Typespec (*(*args));
    size_t num_args;
    bool has_varargs;
    rio_Typespec (*ret);
};

struct rio_Typespec {
    rio_TypespecKind kind;
    rio_SrcPos pos;
    rio_Typespec (*base);
    union {
        char const ((*name));
        rio_TypespecFunc function;
        rio_Expr (*num_elems);
    };
};

struct rio_ExprParen {
    rio_Expr (*expr);
};

struct rio_ExprIntLit {
    ullong val;
    rio_TokenMod mod;
    rio_TokenSuffix suffix;
};

struct rio_ExprFloatLit {
    char const ((*start));
    char const ((*end));
    double val;
    rio_TokenSuffix suffix;
};

struct rio_ExprStrLit {
    char const ((*val));
    rio_TokenMod mod;
};

struct rio_ExprOffsetofField {
    rio_Typespec (*type);
    char const ((*name));
};

struct rio_ExprCompound {
    rio_Typespec (*type);
    rio_CompoundField (*fields);
    size_t num_fields;
};

struct rio_ExprCast {
    rio_Typespec (*type);
    rio_Expr (*expr);
};

struct rio_ExprModify {
    rio_TokenKind op;
    bool post;
    rio_Expr (*expr);
};

struct rio_ExprUnary {
    rio_TokenKind op;
    rio_Expr (*expr);
};

struct rio_ExprBinary {
    rio_TokenKind op;
    rio_Expr (*left);
    rio_Expr (*right);
};

struct rio_ExprTernary {
    rio_Expr (*cond);
    rio_Expr (*then_expr);
    rio_Expr (*else_expr);
};

struct rio_ExprCall {
    rio_Expr (*expr);
    rio_Expr (*(*args));
    size_t num_args;
};

struct rio_ExprIndex {
    rio_Expr (*expr);
    rio_Expr (*index);
};

struct rio_ExprField {
    rio_Expr (*expr);
    char const ((*name));
};

struct rio_Expr {
    rio_ExprKind kind;
    rio_SrcPos pos;
    union {
        rio_ExprParen paren;
        rio_ExprIntLit int_lit;
        rio_ExprFloatLit float_lit;
        rio_ExprStrLit str_lit;
        char const ((*name));
        rio_Expr (*sizeof_expr);
        rio_Typespec (*sizeof_type);
        rio_Expr (*typeof_expr);
        rio_Typespec (*typeof_type);
        rio_Expr (*alignof_expr);
        rio_Typespec (*alignof_type);
        rio_ExprOffsetofField offsetof_field;
        rio_ExprCompound compound;
        rio_ExprCast cast;
        rio_ExprModify modify;
        rio_ExprUnary unary;
        rio_ExprBinary binary;
        rio_ExprTernary ternary;
        rio_ExprCall call;
        rio_ExprIndex index;
        rio_ExprField field;
    };
};

struct rio_Decls {
    rio_Decl (*(*decls));
    size_t num_decls;
};

struct rio_DeclEnum {
    rio_Typespec (*type);
    rio_EnumItem (*items);
    size_t num_items;
};

struct rio_DeclFunc {
    rio_FuncParam (*params);
    size_t num_params;
    rio_Typespec (*ret_type);
    bool has_varargs;
    rio_StmtList block;
};

struct rio_DeclTypedef {
    rio_Typespec (*type);
};

struct rio_DeclVar {
    rio_Typespec (*type);
    rio_Expr (*expr);
};

struct rio_DeclImport {
    bool is_relative;
    char const ((*(*names)));
    size_t num_names;
    bool import_all;
    rio_ImportItem (*items);
    size_t num_items;
};

struct rio_Decl {
    rio_DeclKind kind;
    rio_SrcPos pos;
    char const ((*name));
    rio_Notes notes;
    bool is_incomplete;
    union {
        rio_Note note;
        rio_DeclEnum enum_decl;
        rio_Aggregate (*aggregate);
        rio_DeclFunc function;
        rio_DeclTypedef typedef_decl;
        rio_DeclVar var_decl;
        rio_DeclVar const_decl;
        rio_DeclImport import_decl;
    };
};

struct rio_Aggregate {
    rio_SrcPos pos;
    rio_AggregateKind kind;
    rio_AggregateItem (*items);
    size_t num_items;
};

struct rio_ImportItem {
    char const ((*name));
    char const ((*rename));
};

struct rio_ElseIf {
    rio_Expr (*cond);
    rio_StmtList block;
};

struct rio_BufHdr {
    size_t len;
    size_t cap;
    char (buf[1]);
};

struct rio_Intern {
    size_t len;
    rio_Intern (*next);
    char (str[1]);
};

struct rio_Package {
    char const ((*path));
    char (full_path[MAX_PATH]);
    rio_Decl (*(*decls));
    size_t num_decls;
    rio_Map syms_map;
    rio_Sym (*(*syms));
    char const ((*external_name));
    bool always_reachable;
};

struct rio_TypeField {
    char const ((*name));
    rio_Type (*type);
    size_t offset;
};

struct rio_DirListIter {
    bool valid;
    bool error;
    char (base[MAX_PATH]);
    char (name[MAX_PATH]);
    size_t size;
    bool is_dir;
    void (*handle);
};

struct rio_FlagDefPtr {
    int (*i);
    bool (*b);
    char const ((*(*s)));
};

struct rio_FlagDef {
    rio_FlagKind kind;
    char const ((*name));
    char const ((*help));
    char const ((*(*options)));
    char const ((*arg_name));
    int num_options;
    rio_FlagDefPtr ptr;
};

struct rio_CachedArrayType {
    rio_Type (*type);
    rio_CachedArrayType (*next);
};

struct rio_CachedFuncType {
    rio_Type (*type);
    rio_CachedFuncType (*next);
};

// Typeinfo
#define TYPEID0(index, kind) ((ullong)(index) | ((ullong)(kind) << 24))
#define TYPEID(index, kind, ...) ((ullong)(index) | ((ullong)sizeof(__VA_ARGS__) << 32) | ((ullong)(kind) << 24))

int num_typeinfos;
const TypeInfo **typeinfos;

// Definitions
int main(int argc, char const ((*(*argv)))) {
    return rio_rio_main(argc, argv, rio_gen_all, "c");
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

rio_Arena rio_ast_arena;
void (*rio_ast_alloc(size_t size)) {
    assert((size) != (0));
    void (*ptr) = rio_arena_alloc(&(rio_ast_arena), size);
    memset(ptr, 0, size);
    return ptr;
}

void (*rio_ast_dup(void const ((*src)), size_t size)) {
    if ((size) == (0)) {
        return NULL;
    }
    void (*ptr) = rio_arena_alloc(&(rio_ast_arena), size);
    memcpy(ptr, src, size);
    return ptr;
}

rio_Note rio_new_note(rio_SrcPos pos, char const ((*name)), rio_NoteArg (*args), size_t num_args) {
    return (rio_Note){.pos = pos, .name = name, .args = rio_ast_dup(args, (num_args) * (sizeof(*(args)))), .num_args = num_args};
}

rio_Notes rio_new_notes(rio_Note (*notes), size_t num_notes) {
    return (rio_Notes){rio_ast_dup(notes, (num_notes) * (sizeof(*(notes)))), num_notes};
}

rio_StmtList rio_new_stmt_list(rio_SrcPos pos, rio_Stmt (*(*stmts)), size_t num_stmts) {
    return (rio_StmtList){pos, rio_ast_dup(stmts, (num_stmts) * (sizeof(*(stmts)))), num_stmts};
}

rio_Typespec (*rio_new_typespec(rio_TypespecKind kind, rio_SrcPos pos)) {
    rio_Typespec (*t) = rio_ast_alloc(sizeof(rio_Typespec));
    t->kind = kind;
    t->pos = pos;
    return t;
}

rio_Typespec (*rio_new_typespec_name(rio_SrcPos pos, char const ((*name)))) {
    rio_Typespec (*t) = rio_new_typespec(RIO_TYPESPEC_NAME, pos);
    t->name = name;
    return t;
}

rio_Typespec (*rio_new_typespec_ptr(rio_SrcPos pos, rio_Typespec (*base))) {
    rio_Typespec (*t) = rio_new_typespec(RIO_TYPESPEC_PTR, pos);
    t->base = base;
    return t;
}

rio_Typespec (*rio_new_typespec_const(rio_SrcPos pos, rio_Typespec (*base))) {
    rio_Typespec (*t) = rio_new_typespec(RIO_TYPESPEC_CONST, pos);
    t->base = base;
    return t;
}

rio_Typespec (*rio_new_typespec_array(rio_SrcPos pos, rio_Typespec (*elem), rio_Expr (*size))) {
    rio_Typespec (*t) = rio_new_typespec(RIO_TYPESPEC_ARRAY, pos);
    t->base = elem;
    t->num_elems = size;
    return t;
}

rio_Typespec (*rio_new_typespec_func(rio_SrcPos pos, rio_Typespec (*(*args)), size_t num_args, rio_Typespec (*ret), bool has_varargs)) {
    rio_Typespec (*t) = rio_new_typespec(RIO_TYPESPEC_FUNC, pos);
    t->function.args = rio_ast_dup(args, (num_args) * (sizeof(*(args))));
    t->function.num_args = num_args;
    t->function.ret = ret;
    t->function.has_varargs = has_varargs;
    return t;
}

rio_Decls (*rio_new_decls(rio_Decl (*(*decls)), size_t num_decls)) {
    rio_Decls (*d) = rio_ast_alloc(sizeof(rio_Decls));
    d->decls = rio_ast_dup(decls, (num_decls) * (sizeof(*(decls))));
    d->num_decls = num_decls;
    return d;
}

rio_Decl (*rio_new_decl(rio_DeclKind kind, rio_SrcPos pos, char const ((*name)))) {
    rio_Decl (*d) = rio_ast_alloc(sizeof(rio_Decl));
    d->kind = kind;
    d->pos = pos;
    d->name = name;
    return d;
}

rio_Note (*rio_get_decl_note(rio_Decl (*decl), char const ((*name)))) {
    for (size_t i = 0; (i) < (decl->notes.num_notes); (i)++) {
        rio_Note (*note) = (decl->notes.notes) + (i);
        if ((note->name) == (name)) {
            return note;
        }
    }
    return NULL;
}

bool rio_is_decl_foreign(rio_Decl (*decl)) {
    return (rio_get_decl_note(decl, rio_foreign_name)) != (NULL);
}

rio_Decl (*rio_new_decl_enum(rio_SrcPos pos, char const ((*name)), rio_Typespec (*type), rio_EnumItem (*items), size_t num_items)) {
    rio_Decl (*d) = rio_new_decl(RIO_DECL_ENUM, pos, name);
    d->enum_decl.type = type;
    d->enum_decl.items = rio_ast_dup(items, (num_items) * (sizeof(*(items))));
    d->enum_decl.num_items = num_items;
    return d;
}

rio_Aggregate (*rio_new_aggregate(rio_SrcPos pos, rio_AggregateKind kind, rio_AggregateItem (*items), size_t num_items)) {
    rio_Aggregate (*aggregate) = rio_ast_alloc(sizeof(rio_Aggregate));
    aggregate->pos = pos;
    aggregate->kind = kind;
    aggregate->items = rio_ast_dup(items, (num_items) * (sizeof(*(items))));
    aggregate->num_items = num_items;
    return aggregate;
}

rio_Decl (*rio_new_decl_aggregate(rio_SrcPos pos, rio_DeclKind kind, char const ((*name)), rio_Aggregate (*aggregate))) {
    assert(((kind) == (RIO_DECL_STRUCT)) || ((kind) == (RIO_DECL_UNION)));
    rio_Decl (*d) = rio_new_decl(kind, pos, name);
    d->aggregate = aggregate;
    return d;
}

rio_Decl (*rio_new_decl_var(rio_SrcPos pos, char const ((*name)), rio_Typespec (*type), rio_Expr (*expr))) {
    rio_Decl (*d) = rio_new_decl(RIO_DECL_VAR, pos, name);
    d->var_decl.type = type;
    d->var_decl.expr = expr;
    return d;
}

rio_Decl (*rio_new_decl_func(rio_SrcPos pos, char const ((*name)), rio_FuncParam (*params), size_t num_params, rio_Typespec (*ret_type), bool has_varargs, rio_StmtList block)) {
    rio_Decl (*d) = rio_new_decl(RIO_DECL_FUNC, pos, name);
    d->function.params = rio_ast_dup(params, (num_params) * (sizeof(*(params))));
    d->function.num_params = num_params;
    d->function.ret_type = ret_type;
    d->function.has_varargs = has_varargs;
    d->function.block = block;
    return d;
}

rio_Decl (*rio_new_decl_const(rio_SrcPos pos, char const ((*name)), rio_Typespec (*type), rio_Expr (*expr))) {
    rio_Decl (*d) = rio_new_decl(RIO_DECL_CONST, pos, name);
    d->const_decl.type = type;
    d->const_decl.expr = expr;
    return d;
}

rio_Decl (*rio_new_decl_typedef(rio_SrcPos pos, char const ((*name)), rio_Typespec (*type))) {
    rio_Decl (*d) = rio_new_decl(RIO_DECL_TYPEDEF, pos, name);
    d->typedef_decl.type = type;
    return d;
}

rio_Decl (*rio_new_decl_note(rio_SrcPos pos, rio_Note note)) {
    rio_Decl (*d) = rio_new_decl(RIO_DECL_NOTE, pos, NULL);
    d->note = note;
    return d;
}

rio_Decl (*rio_new_decl_import(rio_SrcPos pos, char const ((*rename_name)), bool is_relative, char const ((*(*names))), size_t num_names, bool import_all, rio_ImportItem (*items), size_t num_items)) {
    rio_Decl (*d) = rio_new_decl(RIO_DECL_IMPORT, pos, NULL);
    d->name = rename_name;
    d->import_decl.is_relative = is_relative;
    d->import_decl.names = rio_ast_dup(names, (num_names) * (sizeof(*(names))));
    d->import_decl.num_names = num_names;
    d->import_decl.import_all = import_all;
    d->import_decl.items = rio_ast_dup(items, (num_items) * (sizeof(*(items))));
    d->import_decl.num_items = num_items;
    return d;
}

rio_Expr (*rio_new_expr(rio_ExprKind kind, rio_SrcPos pos)) {
    rio_Expr (*e) = rio_ast_alloc(sizeof(rio_Expr));
    e->kind = kind;
    e->pos = pos;
    return e;
}

rio_Expr (*rio_new_expr_paren(rio_SrcPos pos, rio_Expr (*expr))) {
    rio_Expr (*e) = rio_new_expr(RIO_EXPR_PAREN, pos);
    e->paren.expr = expr;
    return e;
}

rio_Expr (*rio_new_expr_sizeof_expr(rio_SrcPos pos, rio_Expr (*expr))) {
    rio_Expr (*e) = rio_new_expr(RIO_EXPR_SIZEOF_EXPR, pos);
    e->sizeof_expr = expr;
    return e;
}

rio_Expr (*rio_new_expr_sizeof_type(rio_SrcPos pos, rio_Typespec (*type))) {
    rio_Expr (*e) = rio_new_expr(RIO_EXPR_SIZEOF_TYPE, pos);
    e->sizeof_type = type;
    return e;
}

rio_Expr (*rio_new_expr_typeof_expr(rio_SrcPos pos, rio_Expr (*expr))) {
    rio_Expr (*e) = rio_new_expr(RIO_EXPR_TYPEOF_EXPR, pos);
    e->typeof_expr = expr;
    return e;
}

rio_Expr (*rio_new_expr_typeof_type(rio_SrcPos pos, rio_Typespec (*type))) {
    rio_Expr (*e) = rio_new_expr(RIO_EXPR_TYPEOF_TYPE, pos);
    e->typeof_type = type;
    return e;
}

rio_Expr (*rio_new_expr_alignof_expr(rio_SrcPos pos, rio_Expr (*expr))) {
    rio_Expr (*e) = rio_new_expr(RIO_EXPR_ALIGNOF_EXPR, pos);
    e->alignof_expr = expr;
    return e;
}

rio_Expr (*rio_new_expr_alignof_type(rio_SrcPos pos, rio_Typespec (*type))) {
    rio_Expr (*e) = rio_new_expr(RIO_EXPR_ALIGNOF_TYPE, pos);
    e->alignof_type = type;
    return e;
}

rio_Expr (*rio_new_expr_offsetof(rio_SrcPos pos, rio_Typespec (*type), char const ((*name)))) {
    rio_Expr (*e) = rio_new_expr(RIO_EXPR_OFFSETOF, pos);
    e->offsetof_field.type = type;
    e->offsetof_field.name = name;
    return e;
}

rio_Expr (*rio_new_expr_modify(rio_SrcPos pos, rio_TokenKind op, bool post, rio_Expr (*expr))) {
    rio_Expr (*e) = rio_new_expr(RIO_EXPR_MODIFY, pos);
    e->modify.op = op;
    e->modify.post = post;
    e->modify.expr = expr;
    return e;
}

rio_Expr (*rio_new_expr_int(rio_SrcPos pos, ullong val, rio_TokenMod mod, rio_TokenSuffix suffix)) {
    rio_Expr (*e) = rio_new_expr(RIO_EXPR_INT, pos);
    e->int_lit.val = val;
    e->int_lit.mod = mod;
    e->int_lit.suffix = suffix;
    return e;
}

rio_Expr (*rio_new_expr_float(rio_SrcPos pos, char const ((*start)), char const ((*end)), double val, rio_TokenSuffix suffix)) {
    rio_Expr (*e) = rio_new_expr(RIO_EXPR_FLOAT, pos);
    e->float_lit.start = start;
    e->float_lit.end = end;
    e->float_lit.val = val;
    e->float_lit.suffix = suffix;
    return e;
}

rio_Expr (*rio_new_expr_str(rio_SrcPos pos, char const ((*val)), rio_TokenMod mod)) {
    rio_Expr (*e) = rio_new_expr(RIO_EXPR_STR, pos);
    e->str_lit.val = val;
    e->str_lit.mod = mod;
    return e;
}

rio_Expr (*rio_new_expr_name(rio_SrcPos pos, char const ((*name)))) {
    rio_Expr (*e) = rio_new_expr(RIO_EXPR_NAME, pos);
    e->name = name;
    return e;
}

rio_Expr (*rio_new_expr_compound(rio_SrcPos pos, rio_Typespec (*type), rio_CompoundField (*fields), size_t num_fields)) {
    rio_Expr (*e) = rio_new_expr(RIO_EXPR_COMPOUND, pos);
    e->compound.type = type;
    e->compound.fields = rio_ast_dup(fields, (num_fields) * (sizeof(*(fields))));
    e->compound.num_fields = num_fields;
    return e;
}

rio_Expr (*rio_new_expr_cast(rio_SrcPos pos, rio_Typespec (*type), rio_Expr (*expr))) {
    rio_Expr (*e) = rio_new_expr(RIO_EXPR_CAST, pos);
    e->cast.type = type;
    e->cast.expr = expr;
    return e;
}

rio_Expr (*rio_new_expr_call(rio_SrcPos pos, rio_Expr (*expr), rio_Expr (*(*args)), size_t num_args)) {
    rio_Expr (*e) = rio_new_expr(RIO_EXPR_CALL, pos);
    e->call.expr = expr;
    e->call.args = rio_ast_dup(args, (num_args) * (sizeof(*(args))));
    e->call.num_args = num_args;
    return e;
}

rio_Expr (*rio_new_expr_index(rio_SrcPos pos, rio_Expr (*expr), rio_Expr (*index))) {
    rio_Expr (*e) = rio_new_expr(RIO_EXPR_INDEX, pos);
    e->index.expr = expr;
    e->index.index = index;
    return e;
}

rio_Expr (*rio_new_expr_field(rio_SrcPos pos, rio_Expr (*expr), char const ((*name)))) {
    rio_Expr (*e) = rio_new_expr(RIO_EXPR_FIELD, pos);
    e->field.expr = expr;
    e->field.name = name;
    return e;
}

rio_Expr (*rio_new_expr_unary(rio_SrcPos pos, rio_TokenKind op, rio_Expr (*expr))) {
    rio_Expr (*e) = rio_new_expr(RIO_EXPR_UNARY, pos);
    e->unary.op = op;
    e->unary.expr = expr;
    return e;
}

rio_Expr (*rio_new_expr_binary(rio_SrcPos pos, rio_TokenKind op, rio_Expr (*left), rio_Expr (*right))) {
    rio_Expr (*e) = rio_new_expr(RIO_EXPR_BINARY, pos);
    e->binary.op = op;
    e->binary.left = left;
    e->binary.right = right;
    return e;
}

rio_Expr (*rio_new_expr_ternary(rio_SrcPos pos, rio_Expr (*cond), rio_Expr (*then_expr), rio_Expr (*else_expr))) {
    rio_Expr (*e) = rio_new_expr(RIO_EXPR_TERNARY, pos);
    e->ternary.cond = cond;
    e->ternary.then_expr = then_expr;
    e->ternary.else_expr = else_expr;
    return e;
}

rio_Note (*rio_get_stmt_note(rio_Stmt (*stmt), char const ((*name)))) {
    for (size_t i = 0; (i) < (stmt->notes.num_notes); (i)++) {
        rio_Note (*note) = (stmt->notes.notes) + (i);
        if ((note->name) == (name)) {
            return note;
        }
    }
    return NULL;
}

rio_Stmt (*rio_new_stmt(rio_StmtKind kind, rio_SrcPos pos)) {
    rio_Stmt (*s) = rio_ast_alloc(sizeof(rio_Stmt));
    s->kind = kind;
    s->pos = pos;
    return s;
}

rio_Stmt (*rio_new_stmt_label(rio_SrcPos pos, char const ((*label)))) {
    rio_Stmt (*s) = rio_new_stmt(RIO_STMT_LABEL, pos);
    s->label = label;
    return s;
}

rio_Stmt (*rio_new_stmt_goto(rio_SrcPos pos, char const ((*label)))) {
    rio_Stmt (*s) = rio_new_stmt(RIO_STMT_GOTO, pos);
    s->label = label;
    return s;
}

rio_Stmt (*rio_new_stmt_note(rio_SrcPos pos, rio_Note note)) {
    rio_Stmt (*s) = rio_new_stmt(RIO_STMT_NOTE, pos);
    s->note = note;
    return s;
}

rio_Stmt (*rio_new_stmt_decl(rio_SrcPos pos, rio_Decl (*decl))) {
    rio_Stmt (*s) = rio_new_stmt(RIO_STMT_DECL, pos);
    s->decl = decl;
    return s;
}

rio_Stmt (*rio_new_stmt_return(rio_SrcPos pos, rio_Expr (*expr))) {
    rio_Stmt (*s) = rio_new_stmt(RIO_STMT_RETURN, pos);
    s->expr = expr;
    return s;
}

rio_Stmt (*rio_new_stmt_break(rio_SrcPos pos)) {
    return rio_new_stmt(RIO_STMT_BREAK, pos);
}

rio_Stmt (*rio_new_stmt_continue(rio_SrcPos pos)) {
    return rio_new_stmt(RIO_STMT_CONTINUE, pos);
}

rio_Stmt (*rio_new_stmt_block(rio_SrcPos pos, rio_StmtList block)) {
    rio_Stmt (*s) = rio_new_stmt(RIO_STMT_BLOCK, pos);
    s->block = block;
    return s;
}

rio_Stmt (*rio_new_stmt_if(rio_SrcPos pos, rio_Stmt (*init), rio_Expr (*cond), rio_StmtList then_block, rio_ElseIf (*elseifs), size_t num_elseifs, rio_StmtList else_block)) {
    rio_Stmt (*s) = rio_new_stmt(RIO_STMT_IF, pos);
    s->if_stmt.init = init;
    s->if_stmt.cond = cond;
    s->if_stmt.then_block = then_block;
    s->if_stmt.elseifs = rio_ast_dup(elseifs, (num_elseifs) * (sizeof(*(elseifs))));
    s->if_stmt.num_elseifs = num_elseifs;
    s->if_stmt.else_block = else_block;
    return s;
}

rio_Stmt (*rio_new_stmt_while(rio_SrcPos pos, rio_Expr (*cond), rio_StmtList block)) {
    rio_Stmt (*s) = rio_new_stmt(RIO_STMT_WHILE, pos);
    s->while_stmt.cond = cond;
    s->while_stmt.block = block;
    return s;
}

rio_Stmt (*rio_new_stmt_do_while(rio_SrcPos pos, rio_Expr (*cond), rio_StmtList block)) {
    rio_Stmt (*s) = rio_new_stmt(RIO_STMT_DO_WHILE, pos);
    s->while_stmt.cond = cond;
    s->while_stmt.block = block;
    return s;
}

rio_Stmt (*rio_new_stmt_for(rio_SrcPos pos, rio_Stmt (*init), rio_Expr (*cond), rio_Stmt (*next), rio_StmtList block)) {
    rio_Stmt (*s) = rio_new_stmt(RIO_STMT_FOR, pos);
    s->for_stmt.init = init;
    s->for_stmt.cond = cond;
    s->for_stmt.next = next;
    s->for_stmt.block = block;
    return s;
}

rio_Stmt (*rio_new_stmt_switch(rio_SrcPos pos, rio_Expr (*expr), rio_SwitchCase (*cases), size_t num_cases)) {
    rio_Stmt (*s) = rio_new_stmt(RIO_STMT_SWITCH, pos);
    s->switch_stmt.expr = expr;
    s->switch_stmt.cases = rio_ast_dup(cases, (num_cases) * (sizeof(*(cases))));
    s->switch_stmt.num_cases = num_cases;
    return s;
}

rio_Stmt (*rio_new_stmt_assign(rio_SrcPos pos, rio_TokenKind op, rio_Expr (*left), rio_Expr (*right))) {
    rio_Stmt (*s) = rio_new_stmt(RIO_STMT_ASSIGN, pos);
    s->assign.op = op;
    s->assign.left = left;
    s->assign.right = right;
    return s;
}

rio_Stmt (*rio_new_stmt_init(rio_SrcPos pos, char const ((*name)), bool is_mut, rio_Typespec (*type), rio_Expr (*expr))) {
    rio_Stmt (*s) = rio_new_stmt(RIO_STMT_INIT, pos);
    s->init.name = name;
    s->init.is_mut = is_mut;
    s->init.type = type;
    s->init.expr = expr;
    return s;
}

rio_Stmt (*rio_new_stmt_expr(rio_SrcPos pos, rio_Expr (*expr))) {
    rio_Stmt (*s) = rio_new_stmt(RIO_STMT_EXPR, pos);
    s->expr = expr;
    return s;
}

size_t rio_min(size_t x, size_t y) {
    return ((x) <= (y) ? x : y);
}

size_t rio_max(size_t x, size_t y) {
    return ((x) >= (y) ? x : y);
}

size_t rio_clamp_max(size_t x, size_t max) {
    return rio_min(x, max);
}

size_t rio_clamp_min(size_t x, size_t min) {
    return rio_max(x, min);
}

size_t rio_is_pow2(size_t x) {
    return ((x) != (0)) && (((x) & (((x) - (1)))) == (0));
}

uintptr_t rio_align_down(uintptr_t n, size_t a) {
    return (n) & (~(((a) - (1))));
}

uintptr_t rio_align_up(uintptr_t n, size_t a) {
    return rio_align_down(((n) + (a)) - (1), a);
}

void (*rio_align_down_ptr(void (*p), size_t a)) {
    return (void *)(rio_align_down((uintptr_t)(p), a));
}

void (*rio_align_up_ptr(void (*p), size_t a)) {
    return (void *)(rio_align_up((uintptr_t)(p), a));
}

void rio_fatal(char const ((*fmt)), ...) {
    va_list args = {0};
    va_start_ptr(&(args), &(fmt));
    printf("FATAL: ");
    vprintf(fmt, args);
    printf("\n");
    va_end_ptr(&(args));
    exit(1);
}

void (*rio_xcalloc(size_t num_elems, size_t elem_size)) {
    void (*ptr) = calloc(num_elems, elem_size);
    if (!(ptr)) {
        perror("xcalloc failed");
        exit(1);
    }
    return ptr;
}

void (*rio_xrealloc(void (*ptr), size_t num_bytes)) {
    ptr = realloc(ptr, num_bytes);
    if (!(ptr)) {
        perror("xrealloc failed");
        exit(1);
    }
    return ptr;
}

void (*rio_xmalloc(size_t num_bytes)) {
    void (*ptr) = malloc(num_bytes);
    if (!(ptr)) {
        perror("xmalloc failed");
        exit(1);
    }
    return ptr;
}

void (*rio_memdup(void (*src), size_t size)) {
    void (*dest) = rio_xmalloc(size);
    memcpy(dest, src, size);
    return dest;
}

char (*rio_strf(char const ((*fmt)), ...)) {
    va_list args = {0};
    va_start_ptr(&(args), &(fmt));
    int n = (1) + (vsnprintf(NULL, 0, fmt, args));
    va_end_ptr(&(args));
    char (*str) = rio_xmalloc(n);
    va_start_ptr(&(args), &(fmt));
    vsnprintf(str, n, fmt, args);
    va_end_ptr(&(args));
    return str;
}

char (*rio_read_file(char const ((*path)))) {
    FILE (*file) = fopen(path, "rb");
    if (!(file)) {
        return NULL;
    }
    fseek(file, 0, SEEK_END);
    long len = ftell(file);
    fseek(file, 0, SEEK_SET);
    char (*buf) = rio_xmalloc((len) + (1));
    if ((len) && ((fread(buf, len, 1, file)) != (1))) {
        fclose(file);
        free(buf);
        return NULL;
    }
    fclose(file);
    buf[len] = 0;
    return buf;
}

bool rio_write_file(char const ((*path)), char const ((*buf)), size_t len) {
    FILE (*file) = fopen(path, "w");
    if (!(file)) {
        return false;
    }
    ullong n = fwrite(buf, len, 1, file);
    fclose(file);
    return (n) == (1);
}

rio_BufHdr (*rio_buf__hdr(void (*b))) {
    return (rio_BufHdr *)((((char *)(b)) - (offsetof(rio_BufHdr, buf))));
}

size_t rio_buf_len(void (*b)) {
    return (b ? rio_buf__hdr(b)->len : 0);
}

size_t rio_buf_cap(void const ((*b))) {
    return (b ? rio_buf__hdr((void *)(b))->cap : 0);
}

void (*rio_buf_end(void (*b), size_t elem_size)) {
    return ((char *)(b)) + ((elem_size) * (rio_buf_len(b)));
}

size_t rio_buf_sizeof(void (*b), size_t elem_size) {
    return (b ? (rio_buf_len(b)) * (elem_size) : 0);
}

void rio_buf_free(void (*(*b))) {
    if (b) {
        free(rio_buf__hdr(*(b)));
        *(b) = NULL;
    }
}

void rio_buf_fit(void (*(*b)), size_t new_len, size_t elem_size) {
    if ((new_len) > (rio_buf_cap(*(b)))) {
        *(b) = rio_buf__grow(*(b), new_len, elem_size);
    }
}

void rio_buf_push(void (*(*b)), void (*elem), size_t elem_size) {
    rio_buf_fit(b, (1) + (rio_buf_len(*(b))), elem_size);
    memcpy(((char *)(*(b))) + ((elem_size) * ((rio_buf__hdr(*(b))->len)++)), elem, elem_size);
}

void (*rio_buf__grow(void const ((*buf)), size_t new_len, size_t elem_size)) {
    assert((rio_buf_cap(buf)) <= ((((SIZE_MAX) - (1))) / (2)));
    ullong new_cap = rio_clamp_min((2) * (rio_buf_cap(buf)), rio_max(new_len, 16));
    assert((new_len) <= (new_cap));
    assert((new_cap) <= ((((SIZE_MAX) - (offsetof(rio_BufHdr, buf)))) / (elem_size)));
    ullong new_size = (offsetof(rio_BufHdr, buf)) + ((new_cap) * (elem_size));
    rio_BufHdr (*new_hdr) = {0};
    if (buf) {
        new_hdr = rio_xrealloc(rio_buf__hdr((void *)(buf)), new_size);
    } else {
        new_hdr = rio_xmalloc(new_size);
        new_hdr->len = 0;
    }
    new_hdr->cap = new_cap;
    return new_hdr->buf;
}

void rio_buf_printf(char (*(*buf)), char const ((*fmt)), ...) {
    va_list args = {0};
    va_start_ptr(&(args), &(fmt));
    ullong cap = (rio_buf_cap(*(buf))) - (rio_buf_len(*(buf)));
    int n = (1) + (vsnprintf(rio_buf_end(*(buf), 1), cap, fmt, args));
    va_end_ptr(&(args));
    if ((n) > (cap)) {
        rio_buf_fit((void (**))(buf), (n) + (rio_buf_len(*(buf))), 1);
        va_start_ptr(&(args), &(fmt));
        ullong new_cap = (rio_buf_cap(*(buf))) - (rio_buf_len(*(buf)));
        n = (1) + (vsnprintf(rio_buf_end(*(buf), 1), new_cap, fmt, args));
        assert((n) <= (new_cap));
        va_end_ptr(&(args));
    }
    rio_buf__hdr(*(buf))->len += (n) - (1);
}

rio_Arena rio_allocator;
void rio_arena_grow(rio_Arena (*arena), size_t min_size) {
    ullong size = rio_align_up(rio_clamp_min(min_size, RIO_ARENA_BLOCK_SIZE), RIO_ARENA_ALIGNMENT);
    arena->ptr = rio_xmalloc(size);
    assert((arena->ptr) == (rio_align_down_ptr(arena->ptr, RIO_ARENA_ALIGNMENT)));
    arena->end = (arena->ptr) + (size);
    rio_buf_push((void (**))(&(arena->blocks)), &(arena->ptr), sizeof(arena->ptr));
}

void (*rio_arena_alloc(rio_Arena (*arena), size_t size)) {
    if ((size) > ((size_t)(((arena->end) - (arena->ptr))))) {
        rio_arena_grow(arena, size);
        assert((size) <= ((size_t)(((arena->end) - (arena->ptr)))));
    }
    char (*ptr) = arena->ptr;
    arena->ptr = rio_align_up_ptr((arena->ptr) + (size), RIO_ARENA_ALIGNMENT);
    assert((arena->ptr) <= (arena->end));
    assert((ptr) == (rio_align_down_ptr(ptr, RIO_ARENA_ALIGNMENT)));
    return ptr;
}

void rio_arena_free(rio_Arena (*arena)) {
    void (*end) = rio_buf_end(arena->blocks, sizeof(arena->ptr));
    for (char (*(*it)) = arena->blocks; (it) != (end); (it)++) {
        free(*(it));
    }
    rio_buf_free((void (**))(&(arena->blocks)));
}

uint64_t rio_hash_uint64(uint64_t x) {
    x *= 0xff51afd7ed558ccd;
    x ^= (x) >> (32);
    return x;
}

uint64_t rio_hash_ptr(void const ((*ptr))) {
    return rio_hash_uint64((uintptr_t)(ptr));
}

uint64_t rio_hash_mix(uint64_t x, uint64_t y) {
    x ^= y;
    x *= 0xff51afd7ed558ccd;
    x ^= (x) >> (32);
    return x;
}

uint64_t rio_hash_bytes(void const ((*ptr)), size_t len) {
    uint64_t x = 0xcbf29ce484222325;
    char const ((*buf)) = (char const (*))(ptr);
    for (size_t i = 0; (i) < (len); (i)++) {
        x ^= buf[i];
        x *= 0x100000001b3;
        x ^= (x) >> (32);
    }
    return x;
}

uint64_t rio_map_get_uint64_from_uint64(rio_Map (*map), uint64_t key) {
    if ((map->len) == (0)) {
        return 0;
    }
    assert(rio_is_pow2(map->cap));
    ullong i = (size_t)(rio_hash_uint64(key));
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

void rio_map_grow(rio_Map (*map), size_t new_cap) {
    new_cap = rio_clamp_min(new_cap, 16);
    rio_Map new_map = {.keys = rio_xcalloc(new_cap, sizeof(uint64_t)), .vals = rio_xmalloc((new_cap) * (sizeof(uint64_t))), .cap = new_cap};
    for (size_t i = 0; (i) < (map->cap); (i)++) {
        if (map->keys[i]) {
            rio_map_put_uint64_from_uint64(&(new_map), map->keys[i], map->vals[i]);
        }
    }
    free((void *)(map->keys));
    free(map->vals);
    *(map) = new_map;
}

void rio_map_put_uint64_from_uint64(rio_Map (*map), uint64_t key, uint64_t val) {
    assert(key);
    if (!(val)) {
        return;
    }
    if (((2) * (map->len)) >= (map->cap)) {
        rio_map_grow(map, (2) * (map->cap));
    }
    assert(((2) * (map->len)) < (map->cap));
    assert(rio_is_pow2(map->cap));
    ullong i = (size_t)(rio_hash_uint64(key));
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

void (*rio_map_get(rio_Map (*map), void const ((*key)))) {
    return (void *)((uintptr_t)(rio_map_get_uint64_from_uint64(map, (uint64_t)((uintptr_t)(key)))));
}

void rio_map_put(rio_Map (*map), void const ((*key)), void (*val)) {
    rio_map_put_uint64_from_uint64(map, (uint64_t)((uintptr_t)(key)), (uint64_t)((uintptr_t)(val)));
}

void (*rio_map_get_from_uint64(rio_Map (*map), uint64_t key)) {
    return (void *)((uintptr_t)(rio_map_get_uint64_from_uint64(map, key)));
}

void rio_map_put_from_uint64(rio_Map (*map), uint64_t key, void (*val)) {
    rio_map_put_uint64_from_uint64(map, key, (uint64_t)((uintptr_t)(val)));
}

uint64_t rio_map_get_uint64(rio_Map (*map), void (*key)) {
    return rio_map_get_uint64_from_uint64(map, (uint64_t)((uintptr_t)(key)));
}

void rio_map_put_uint64(rio_Map (*map), void (*key), uint64_t val) {
    rio_map_put_uint64_from_uint64(map, (uint64_t)((uintptr_t)(key)), val);
}

rio_Arena rio_intern_arena;
rio_Map rio_interns;
char const ((*rio_str_intern_range(char const ((*start)), char const ((*end))))) {
    size_t len = (end) - (start);
    ullong hash = rio_hash_bytes(start, len);
    uint64_t key = (hash ? hash : 1);
    rio_Intern (*intern) = rio_map_get_from_uint64(&(rio_interns), key);
    for (rio_Intern (*it) = intern; it; it = it->next) {
        if (((it->len) == (len)) && ((strncmp(it->str, start, len)) == (0))) {
            return it->str;
        }
    }
    rio_Intern (*new_intern) = rio_arena_alloc(&(rio_intern_arena), ((offsetof(rio_Intern, str)) + (len)) + (1));
    new_intern->len = len;
    new_intern->next = intern;
    memcpy(new_intern->str, start, len);
    new_intern->str[len] = 0;
    rio_map_put_from_uint64(&(rio_interns), key, new_intern);
    return new_intern->str;
}

char const ((*rio_str_intern(char const ((*str))))) {
    return rio_str_intern_range(str, (str) + (strlen(str)));
}

bool rio_str_islower(char const ((*str))) {
    while (*(str)) {
        if ((isalpha(*(str))) && (!(islower(*(str))))) {
            return false;
        }
        (str)++;
    }
    return true;
}

char (*rio_gen_buf) = NULL;
int rio_gen_indent;
rio_SrcPos rio_gen_pos;
char const ((*(*rio_gen_headers_buf)));
char (*rio_gen_preamble_str) = 
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
char (*rio_gen_postamble_str) = 
    "\n"
    "static void va_arg_ptr(va_list *args, Any any) {\n"
    "  switch (typeid_kind(any.type)) {\n"
    "  case TYPE_BOOL:\n"
    "    *(bool *)any.ptr = (bool)va_arg(*args, int);\n"
    "    break;\n"
    "  case TYPE_CHAR:\n"
    "    *(char *)any.ptr = (char)va_arg(*args, int);\n"
    "    break;\n"
    "  case TYPE_UCHAR:\n"
    "    *(uchar *)any.ptr = (uchar)va_arg(*args, int);\n"
    "    break;\n"
    "  case TYPE_SCHAR:\n"
    "    *(schar *)any.ptr = (schar)va_arg(*args, int);\n"
    "    break;\n"
    "  case TYPE_SHORT:\n"
    "    *(short *)any.ptr = (short)va_arg(*args, int);\n"
    "    break;\n"
    "  case TYPE_USHORT:\n"
    "    *(ushort *)any.ptr = (ushort)va_arg(*args, int);\n"
    "    break;\n"
    "  case TYPE_INT:\n"
    "    *(int *)any.ptr = va_arg(*args, int);\n"
    "    break;\n"
    "  case TYPE_UINT:\n"
    "    *(uint *)any.ptr = va_arg(*args, uint);\n"
    "    break;\n"
    "  case TYPE_LONG:\n"
    "    *(long *)any.ptr = va_arg(*args, long);\n"
    "    break;\n"
    "  case TYPE_ULONG:\n"
    "    *(ulong *)any.ptr = va_arg(*args, ulong);\n"
    "    break;\n"
    "  case TYPE_LLONG:\n"
    "    *(llong *)any.ptr = va_arg(*args, llong);\n"
    "    break;\n"
    "  case TYPE_ULLONG:\n"
    "    *(ullong *)any.ptr = va_arg(*args, ullong);\n"
    "    break;\n"
    "  case TYPE_FLOAT:\n"
    "    *(float *)any.ptr = (float)va_arg(*args, double);\n"
    "    break;\n"
    "  case TYPE_DOUBLE:\n"
    "    *(double *)any.ptr = va_arg(*args, double);\n"
    "    break;\n"
    "  case TYPE_FUNC:\n"
    "  case TYPE_PTR:\n"
    "    *(void **)any.ptr = va_arg(*args, void *);\n"
    "    break;\n"
    "  default:\n"
    "    assert(0 && \"argument type not supported\");\n"
    "    break;\n"
    "  }\n"
    "}\n"
    "\n"
    "#ifdef __GNUC__\n"
    "#pragma GCC diagnostic pop\n"
    "#endif\n";
void rio_genln(void) {
    rio_buf_printf(&(rio_gen_buf), "\n"
    "%.*s", (rio_gen_indent) * (2), "                                  ");
    (rio_gen_pos.line)++;
}

bool rio_is_incomplete_array_typespec(rio_Typespec (*typespec)) {
    return ((typespec->kind) == (RIO_TYPESPEC_ARRAY)) && (!(typespec->num_elems));
}

char (rio_char_to_escape[256]) = {['\0'] = '0', ['\n'] = 'n', ['\r'] = 'r', ['\t'] = 't', ['\v'] = 'v', ['\b'] = 'b', ['\a'] = 'a', ['\\'] = '\\', ['\"'] = '\"', ['\''] = '\''};
void rio_gen_char(char c) {
    if (rio_char_to_escape[(uchar)(c)]) {
        rio_buf_printf(&(rio_gen_buf), "\'\\%c\'", rio_char_to_escape[(uchar)(c)]);
    } else if (isprint(c)) {
        rio_buf_printf(&(rio_gen_buf), "\'%c\'", c);
    } else {
        rio_buf_printf(&(rio_gen_buf), "\'\\x%X\'", (uchar)(c));
    }
}

void rio_gen_str(char const ((*str)), bool multiline) {
    if (multiline) {
        (rio_gen_indent)++;
        rio_genln();
    }
    rio_buf_printf(&(rio_gen_buf), "\"");
    while (*(str)) {
        char const ((*start)) = str;
        while (((*(str)) && (isprint(*(str)))) && (!(rio_char_to_escape[(uchar)(*(str))]))) {
            (str)++;
        }
        if ((start) != (str)) {
            rio_buf_printf(&(rio_gen_buf), "%.*s", (str) - (start), start);
        }
        if (*(str)) {
            if (rio_char_to_escape[(uchar)(*(str))]) {
                rio_buf_printf(&(rio_gen_buf), "\\%c", rio_char_to_escape[(uchar)(*(str))]);
                if (((str[0]) == ('\n')) && (str[1])) {
                    rio_buf_printf(&(rio_gen_buf), "\"");
                    rio_genln();
                    rio_buf_printf(&(rio_gen_buf), "\"");
                }
            } else {
                assert(!(isprint(*(str))));
                rio_buf_printf(&(rio_gen_buf), "\\x%X", (uchar)(*(str)));
            }
            (str)++;
        }
    }
    rio_buf_printf(&(rio_gen_buf), "\"");
    if (multiline) {
        (rio_gen_indent)--;
    }
}

void rio_gen_sync_pos(rio_SrcPos pos) {
    if (rio_flag_nosourcemap) {
        return;
    }
    if (((rio_gen_pos.line) != (pos.line)) || ((rio_gen_pos.name) != (pos.name))) {
        rio_genln();
        rio_buf_printf(&(rio_gen_buf), "#line %d", pos.line);
        if ((rio_gen_pos.name) != (pos.name)) {
            rio_buf_printf(&(rio_gen_buf), " ");
            rio_gen_str(pos.name, false);
        }
        rio_gen_pos = pos;
    }
}

char const ((*rio_cdecl_paren(char const ((*str)), char c))) {
    return ((c) && ((c) != ('[')) ? (char const (*))(rio_strf("(%s)", str)) : str);
}

char const ((*rio_cdecl_name(rio_Type (*type)))) {
    char const ((*type_name)) = rio_type_names[type->kind];
    if (type_name) {
        return type_name;
    } else {
        assert(type->sym);
        return rio_get_gen_name(type->sym);
    }
}

char (*rio_type_to_cdecl(rio_Type (*type), char const ((*str)))) {
    switch (type->kind) {
    case RIO_CMPL_TYPE_PTR: {
        return rio_type_to_cdecl(type->base, rio_cdecl_paren(rio_strf("*%s", str), *(str)));
        break;
    }
    case RIO_CMPL_TYPE_CONST: {
        return rio_type_to_cdecl(type->base, rio_strf("const %s", rio_cdecl_paren(str, *(str))));
        break;
    }
    case RIO_CMPL_TYPE_ARRAY: {
        if ((type->num_elems) == (0)) {
            return rio_type_to_cdecl(type->base, rio_cdecl_paren(rio_strf("%s[]", str), *(str)));
        } else {
            return rio_type_to_cdecl(type->base, rio_cdecl_paren(rio_strf("%s[%zu]", str, type->num_elems), *(str)));
        }
        break;
    }
    case RIO_CMPL_TYPE_FUNC: {
        {
            char (*result) = NULL;
            rio_buf_printf(&(result), "(*%s)(", str);
            if ((type->function.num_params) == (0)) {
                rio_buf_printf(&(result), "void");
            } else {
                for (size_t i = 0; (i) < (type->function.num_params); (i)++) {
                    rio_buf_printf(&(result), "%s%s", ((i) == (0) ? "" : ", "), rio_type_to_cdecl(type->function.params[i], ""));
                }
            }
            if (type->function.has_varargs) {
                rio_buf_printf(&(result), ", ...");
            }
            rio_buf_printf(&(result), ")");
            return rio_type_to_cdecl(type->function.ret, result);
        }
        break;
    }
    default: {
        return rio_strf("%s%s%s", rio_cdecl_name(type), (*(str) ? " " : ""), str);
        break;
    }
    }
}

char const ((*rio_gen_expr_str(rio_Expr (*expr)))) {
    char (*temp) = rio_gen_buf;
    rio_gen_buf = NULL;
    rio_gen_expr(expr);
    char (*result) = rio_gen_buf;
    rio_gen_buf = temp;
    return result;
}

rio_Map rio_gen_name_map;
char const ((*rio_get_gen_name_or_default(void const ((*ptr)), char const ((*default_name))))) {
    char const ((*name)) = rio_map_get(&(rio_gen_name_map), ptr);
    if (!(name)) {
        rio_Sym (*sym) = rio_get_resolved_sym(ptr);
        if (sym) {
            if (sym->external_name) {
                name = sym->external_name;
            } else if (sym->home_package->external_name) {
                char const ((*external_name)) = sym->home_package->external_name;
                char (buf[256]) = {0};
                if ((sym->kind) == (RIO_SYM_CONST)) {
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
                name = rio_strf("%s%s", external_name, sym->name);
            } else {
                name = sym->name;
            }
        } else {
            assert(default_name);
            name = default_name;
        }
        rio_map_put(&(rio_gen_name_map), ptr, (void *)(name));
    }
    return name;
}

char const ((*rio_get_gen_name(void const ((*ptr))))) {
    return rio_get_gen_name_or_default(ptr, "ERROR");
}

char (*rio_typespec_to_cdecl(rio_Typespec (*typespec), char const ((*str)))) {
    if (!(typespec)) {
        return rio_strf("void%s%s", (*(str) ? " " : ""), str);
    }
    switch (typespec->kind) {
    case RIO_TYPESPEC_NAME: {
        return rio_strf("%s%s%s", rio_get_gen_name_or_default(typespec, typespec->name), (*(str) ? " " : ""), str);
        break;
    }
    case RIO_TYPESPEC_PTR: {
        return rio_typespec_to_cdecl(typespec->base, rio_cdecl_paren(rio_strf("*%s", str), *(str)));
        break;
    }
    case RIO_TYPESPEC_CONST: {
        return rio_typespec_to_cdecl(typespec->base, rio_strf("const %s", rio_cdecl_paren(str, *(str))));
        break;
    }
    case RIO_TYPESPEC_ARRAY: {
        if ((typespec->num_elems) == (0)) {
            return rio_typespec_to_cdecl(typespec->base, rio_cdecl_paren(rio_strf("%s[]", str), *(str)));
        } else {
            return rio_typespec_to_cdecl(typespec->base, rio_cdecl_paren(rio_strf("%s[%s]", str, rio_gen_expr_str(typespec->num_elems)), *(str)));
        }
        break;
    }
    case RIO_TYPESPEC_FUNC: {
        {
            char (*result) = NULL;
            rio_buf_printf(&(result), "(*%s)(", str);
            if ((typespec->function.num_args) == (0)) {
                rio_buf_printf(&(result), "void");
            } else {
                for (size_t i = 0; (i) < (typespec->function.num_args); (i)++) {
                    rio_buf_printf(&(result), "%s%s", ((i) == (0) ? "" : ", "), rio_typespec_to_cdecl(typespec->function.args[i], ""));
                }
            }
            if (typespec->function.has_varargs) {
                rio_buf_printf(&(result), ", ...");
            }
            rio_buf_printf(&(result), ")");
            return rio_typespec_to_cdecl(typespec->function.ret, result);
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

void rio_gen_func_decl(rio_Decl (*decl)) {
    assert((decl->kind) == (RIO_DECL_FUNC));
    char (*result) = NULL;
    rio_buf_printf(&(result), "%s(", rio_get_gen_name(decl));
    if ((decl->function.num_params) == (0)) {
        rio_buf_printf(&(result), "void");
    } else {
        for (size_t i = 0; (i) < (decl->function.num_params); (i)++) {
            rio_FuncParam param = decl->function.params[i];
            if ((i) != (0)) {
                rio_buf_printf(&(result), ", ");
            }
            rio_buf_printf(&(result), "%s", rio_typespec_to_cdecl(param.type, param.name));
        }
    }
    if (decl->function.has_varargs) {
        rio_buf_printf(&(result), ", ...");
    }
    rio_buf_printf(&(result), ")");
    rio_gen_sync_pos(decl->pos);
    if (decl->function.ret_type) {
        rio_genln();
        rio_buf_printf(&(rio_gen_buf), "%s", rio_typespec_to_cdecl(decl->function.ret_type, result));
    } else {
        rio_genln();
        rio_buf_printf(&(rio_gen_buf), "void %s", result);
    }
}

bool rio_gen_reachable(rio_Sym (*sym)) {
    return (rio_flag_fullgen) || ((sym->reachable) == (RIO_REACHABLE_NATURAL));
}

void rio_gen_forward_decls(void) {
    for (rio_Sym (*(*it)) = rio_sorted_syms; (it) != (rio_buf_end(rio_sorted_syms, sizeof(*(rio_sorted_syms)))); (it)++) {
        rio_Sym (*sym) = *(it);
        rio_Decl (*decl) = sym->decl;
        if ((!(decl)) || (!(rio_gen_reachable(sym)))) {
            continue;
        }
        if (rio_is_decl_foreign(decl)) {
            continue;
        }
        switch (decl->kind) {
        case RIO_DECL_STRUCT:
        case RIO_DECL_UNION: {
            {
                char const ((*name)) = rio_get_gen_name(sym);
                rio_genln();
                rio_buf_printf(&(rio_gen_buf), "typedef %s %s %s;", ((decl->kind) == (RIO_DECL_STRUCT) ? "struct" : "union"), name, name);
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

void rio_gen_aggregate_items(rio_Aggregate (*aggregate)) {
    (rio_gen_indent)++;
    for (size_t i = 0; (i) < (aggregate->num_items); (i)++) {
        rio_AggregateItem item = aggregate->items[i];
        if ((item.kind) == (RIO_AGGREGATE_ITEM_FIELD)) {
            for (size_t j = 0; (j) < (item.num_names); (j)++) {
                rio_gen_sync_pos(item.pos);
                rio_genln();
                rio_buf_printf(&(rio_gen_buf), "%s;", rio_typespec_to_cdecl(item.type, item.names[j]));
            }
        } else if ((item.kind) == (RIO_AGGREGATE_ITEM_SUBAGGREGATE)) {
            rio_genln();
            rio_buf_printf(&(rio_gen_buf), "%s {", ((item.subaggregate->kind) == (RIO_AGGREGATE_STRUCT) ? "struct" : "union"));
            rio_gen_aggregate_items(item.subaggregate);
            rio_genln();
            rio_buf_printf(&(rio_gen_buf), "};");
        } else {
            assert(0);
        }
    }
    (rio_gen_indent)--;
}

void rio_gen_aggregate(rio_Decl (*decl)) {
    assert(((decl->kind) == (RIO_DECL_STRUCT)) || ((decl->kind) == (RIO_DECL_UNION)));
    if (decl->is_incomplete) {
        return;
    }
    rio_genln();
    rio_buf_printf(&(rio_gen_buf), "%s %s {", ((decl->kind) == (RIO_DECL_STRUCT) ? "struct" : "union"), rio_get_gen_name(decl));
    rio_gen_aggregate_items(decl->aggregate);
    rio_genln();
    rio_buf_printf(&(rio_gen_buf), "};");
}

void rio_gen_paren_expr(rio_Expr (*expr)) {
    rio_buf_printf(&(rio_gen_buf), "(");
    rio_gen_expr(expr);
    rio_buf_printf(&(rio_gen_buf), ")");
}

void rio_gen_expr_compound(rio_Expr (*expr)) {
    rio_Type (*expected_type) = rio_get_resolved_expected_type(expr);
    if ((expected_type) && (!(rio_is_ptr_type(expected_type)))) {
        rio_buf_printf(&(rio_gen_buf), "{");
    } else if (expr->compound.type) {
        rio_buf_printf(&(rio_gen_buf), "(%s){", rio_typespec_to_cdecl(expr->compound.type, ""));
    } else {
        rio_buf_printf(&(rio_gen_buf), "(%s){", rio_type_to_cdecl(rio_get_resolved_type(expr), ""));
    }
    for (size_t i = 0; (i) < (expr->compound.num_fields); (i)++) {
        if ((i) != (0)) {
            rio_buf_printf(&(rio_gen_buf), ", ");
        }
        rio_CompoundField field = expr->compound.fields[i];
        if ((field.kind) == (RIO_FIELD_NAME)) {
            rio_buf_printf(&(rio_gen_buf), ".%s = ", field.name);
        } else if ((field.kind) == (RIO_FIELD_INDEX)) {
            rio_buf_printf(&(rio_gen_buf), "[");
            rio_gen_expr(field.index);
            rio_buf_printf(&(rio_gen_buf), "] = ");
        }
        rio_gen_expr(field.init);
    }
    if ((expr->compound.num_fields) == (0)) {
        rio_buf_printf(&(rio_gen_buf), "0");
    }
    rio_buf_printf(&(rio_gen_buf), "}");
}

char const ((*(rio_typeid_kind_names[RIO_NUM_CMPL_TYPE_KINDS]))) = {[RIO_CMPL_TYPE_NONE] = "TYPE_NONE", [RIO_CMPL_TYPE_VOID] = "TYPE_VOID", [RIO_CMPL_TYPE_BOOL] = "TYPE_BOOL", [RIO_CMPL_TYPE_CHAR] = "TYPE_CHAR", [RIO_CMPL_TYPE_UCHAR] = "TYPE_UCHAR", [RIO_CMPL_TYPE_SCHAR] = "TYPE_SCHAR", [RIO_CMPL_TYPE_SHORT] = "TYPE_SHORT", [RIO_CMPL_TYPE_USHORT] = "TYPE_USHORT", [RIO_CMPL_TYPE_INT] = "TYPE_INT", [RIO_CMPL_TYPE_UINT] = "TYPE_UINT", [RIO_CMPL_TYPE_LONG] = "TYPE_LONG", [RIO_CMPL_TYPE_ULONG] = "TYPE_ULONG", [RIO_CMPL_TYPE_LLONG] = "TYPE_LLONG", [RIO_CMPL_TYPE_ULLONG] = "TYPE_ULLONG", [RIO_CMPL_TYPE_FLOAT] = "TYPE_FLOAT", [RIO_CMPL_TYPE_DOUBLE] = "TYPE_DOUBLE", [RIO_CMPL_TYPE_CONST] = "TYPE_CONST", [RIO_CMPL_TYPE_PTR] = "TYPE_PTR", [RIO_CMPL_TYPE_ARRAY] = "TYPE_ARRAY", [RIO_CMPL_TYPE_STRUCT] = "TYPE_STRUCT", [RIO_CMPL_TYPE_UNION] = "TYPE_UNION", [RIO_CMPL_TYPE_FUNC] = "TYPE_FUNC"};
char const ((*rio_typeid_kind_name(rio_Type (*type)))) {
    if ((type->kind) < (RIO_NUM_CMPL_TYPE_KINDS)) {
        char const ((*name)) = rio_typeid_kind_names[type->kind];
        if (name) {
            return name;
        }
    }
    return "TYPE_NONE";
}

bool rio_is_excluded_typeinfo(rio_Type (*type)) {
    while ((((type->kind) == (RIO_CMPL_TYPE_ARRAY)) || ((type->kind) == (RIO_CMPL_TYPE_CONST))) || ((type->kind) == (RIO_CMPL_TYPE_PTR))) {
        type = type->base;
    }
    if (type->sym) {
        return !(rio_gen_reachable(type->sym));
    } else {
        return (!(type->sym)) && ((((type->kind) == (RIO_CMPL_TYPE_STRUCT)) || ((type->kind) == (RIO_CMPL_TYPE_UNION))));
    }
}

void rio_gen_typeid(rio_Type (*type)) {
    if (((type->size) == (0)) || (rio_is_excluded_typeinfo(type))) {
        rio_buf_printf(&(rio_gen_buf), "TYPEID0(%d, %s)", type->typeid, rio_typeid_kind_name(type));
    } else {
        rio_buf_printf(&(rio_gen_buf), "TYPEID(%d, %s, %s)", type->typeid, rio_typeid_kind_name(type), rio_type_to_cdecl(type, ""));
    }
}

void rio_gen_expr(rio_Expr (*expr)) {
    switch (expr->kind) {
    case RIO_EXPR_PAREN: {
        rio_buf_printf(&(rio_gen_buf), "(");
        rio_gen_expr(expr->paren.expr);
        rio_buf_printf(&(rio_gen_buf), ")");
        break;
    }
    case RIO_EXPR_INT: {
        {
            char const ((*suffix_name)) = rio_token_suffix_names[expr->int_lit.suffix];
            switch (expr->int_lit.mod) {
            case RIO_MOD_BIN:
            case RIO_MOD_HEX: {
                rio_buf_printf(&(rio_gen_buf), "0x%llx%s", expr->int_lit.val, suffix_name);
                break;
            }
            case RIO_MOD_OCT: {
                rio_buf_printf(&(rio_gen_buf), "0%llo%s", expr->int_lit.val, suffix_name);
                break;
            }
            case RIO_MOD_CHAR: {
                rio_gen_char((char)(expr->int_lit.val));
                break;
            }
            default: {
                rio_buf_printf(&(rio_gen_buf), "%llu%s", expr->int_lit.val, suffix_name);
                break;
            }
            }
        }
        break;
    }
    case RIO_EXPR_FLOAT: {
        {
            int is_double = (expr->float_lit.suffix) == (RIO_SUFFIX_D);
            size_t len = (expr->float_lit.end) - (expr->float_lit.start);
            rio_buf_printf(&(rio_gen_buf), "%.*s%s", (is_double ? (len) - (1) : len), expr->float_lit.start, (is_double ? "" : "f"));
        }
        break;
    }
    case RIO_EXPR_STR: {
        rio_gen_str(expr->str_lit.val, (expr->str_lit.mod) == (RIO_MOD_MULTILINE));
        break;
    }
    case RIO_EXPR_NAME: {
        rio_buf_printf(&(rio_gen_buf), "%s", rio_get_gen_name_or_default(expr, expr->name));
        break;
    }
    case RIO_EXPR_CAST: {
        rio_buf_printf(&(rio_gen_buf), "(%s)(", rio_typespec_to_cdecl(expr->cast.type, ""));
        rio_gen_expr(expr->cast.expr);
        rio_buf_printf(&(rio_gen_buf), ")");
        break;
    }
    case RIO_EXPR_CALL: {
        {
            rio_Sym (*sym) = rio_get_resolved_sym(expr->call.expr);
            if ((sym) && ((sym->kind) == (RIO_SYM_TYPE))) {
                rio_buf_printf(&(rio_gen_buf), "(%s)", rio_get_gen_name(sym));
            } else {
                rio_gen_expr(expr->call.expr);
            }
            rio_buf_printf(&(rio_gen_buf), "(");
            for (size_t i = 0; (i) < (expr->call.num_args); (i)++) {
                if ((i) != (0)) {
                    rio_buf_printf(&(rio_gen_buf), ", ");
                }
                rio_gen_expr(expr->call.args[i]);
            }
            rio_buf_printf(&(rio_gen_buf), ")");
        }
        break;
    }
    case RIO_EXPR_INDEX: {
        rio_gen_expr(expr->index.expr);
        rio_buf_printf(&(rio_gen_buf), "[");
        rio_gen_expr(expr->index.index);
        rio_buf_printf(&(rio_gen_buf), "]");
        break;
    }
    case RIO_EXPR_FIELD: {
        {
            rio_Sym (*sym) = rio_get_resolved_sym(expr);
            if (sym) {
                rio_buf_printf(&(rio_gen_buf), "(%s)", rio_get_gen_name(sym));
            } else {
                rio_gen_expr(expr->field.expr);
                rio_Type (*type) = rio_unqualify_type(rio_get_resolved_type(expr->field.expr));
                rio_buf_printf(&(rio_gen_buf), "%s%s", ((type->kind) == (RIO_CMPL_TYPE_PTR) ? "->" : "."), expr->field.name);
            }
        }
        break;
    }
    case RIO_EXPR_COMPOUND: {
        rio_gen_expr_compound(expr);
        break;
    }
    case RIO_EXPR_UNARY: {
        rio_buf_printf(&(rio_gen_buf), "%s(", rio_token_kind_name(expr->unary.op));
        rio_gen_expr(expr->unary.expr);
        rio_buf_printf(&(rio_gen_buf), ")");
        break;
    }
    case RIO_EXPR_BINARY: {
        rio_buf_printf(&(rio_gen_buf), "(");
        rio_gen_expr(expr->binary.left);
        rio_buf_printf(&(rio_gen_buf), ") %s (", rio_token_kind_name(expr->binary.op));
        rio_gen_expr(expr->binary.right);
        rio_buf_printf(&(rio_gen_buf), ")");
        break;
    }
    case RIO_EXPR_TERNARY: {
        rio_buf_printf(&(rio_gen_buf), "(");
        rio_gen_expr(expr->ternary.cond);
        rio_buf_printf(&(rio_gen_buf), " ? ");
        rio_gen_expr(expr->ternary.then_expr);
        rio_buf_printf(&(rio_gen_buf), " : ");
        rio_gen_expr(expr->ternary.else_expr);
        rio_buf_printf(&(rio_gen_buf), ")");
        break;
    }
    case RIO_EXPR_SIZEOF_EXPR: {
        rio_buf_printf(&(rio_gen_buf), "sizeof(");
        rio_gen_expr(expr->sizeof_expr);
        rio_buf_printf(&(rio_gen_buf), ")");
        break;
    }
    case RIO_EXPR_SIZEOF_TYPE: {
        rio_buf_printf(&(rio_gen_buf), "sizeof(%s)", rio_typespec_to_cdecl(expr->sizeof_type, ""));
        break;
    }
    case RIO_EXPR_ALIGNOF_EXPR: {
        rio_buf_printf(&(rio_gen_buf), "alignof(%s)", rio_type_to_cdecl(rio_get_resolved_type(expr->alignof_expr), ""));
        break;
    }
    case RIO_EXPR_ALIGNOF_TYPE: {
        rio_buf_printf(&(rio_gen_buf), "alignof(%s)", rio_typespec_to_cdecl(expr->alignof_type, ""));
        break;
    }
    case RIO_EXPR_TYPEOF_EXPR: {
        {
            rio_Type (*type) = rio_get_resolved_type(expr->typeof_expr);
            assert(type->typeid);
            rio_gen_typeid(type);
        }
        break;
    }
    case RIO_EXPR_TYPEOF_TYPE: {
        {
            rio_Type (*type) = rio_get_resolved_type(expr->typeof_type);
            assert(type->typeid);
            rio_gen_typeid(type);
        }
        break;
    }
    case RIO_EXPR_OFFSETOF: {
        rio_buf_printf(&(rio_gen_buf), "offsetof(%s, %s)", rio_typespec_to_cdecl(expr->offsetof_field.type, ""), expr->offsetof_field.name);
        break;
    }
    case RIO_EXPR_MODIFY: {
        if (!(expr->modify.post)) {
            rio_buf_printf(&(rio_gen_buf), "%s", rio_token_kind_name(expr->modify.op));
        }
        rio_gen_paren_expr(expr->modify.expr);
        if (expr->modify.post) {
            rio_buf_printf(&(rio_gen_buf), "%s", rio_token_kind_name(expr->modify.op));
        }
        break;
    }
    default: {
        assert(0);
        break;
    }
    }
}

void rio_gen_stmt_block(rio_StmtList block) {
    rio_buf_printf(&(rio_gen_buf), "{");
    (rio_gen_indent)++;
    for (size_t i = 0; (i) < (block.num_stmts); (i)++) {
        rio_gen_stmt(block.stmts[i]);
    }
    (rio_gen_indent)--;
    rio_genln();
    rio_buf_printf(&(rio_gen_buf), "}");
}

void rio_gen_simple_stmt(rio_Stmt (*stmt)) {
    switch (stmt->kind) {
    case RIO_STMT_EXPR: {
        rio_gen_expr(stmt->expr);
        break;
    }
    case RIO_STMT_INIT: {
        if (stmt->init.type) {
            rio_Typespec (*init_typespec) = stmt->init.type;
            if (rio_is_incomplete_array_typespec(stmt->init.type)) {
                rio_Expr (*size) = rio_new_expr_int(init_typespec->pos, rio_get_resolved_type(stmt->init.expr)->num_elems, 0, 0);
                init_typespec = rio_new_typespec_array(init_typespec->pos, init_typespec->base, size);
            }
            rio_buf_printf(&(rio_gen_buf), "%s = ", rio_typespec_to_cdecl(stmt->init.type, stmt->init.name));
            if (stmt->init.expr) {
                rio_gen_expr(stmt->init.expr);
            } else {
                rio_buf_printf(&(rio_gen_buf), "{0}");
            }
        } else {
            rio_buf_printf(&(rio_gen_buf), "%s = ", rio_type_to_cdecl(rio_unqualify_type(rio_get_resolved_type(stmt->init.expr)), stmt->init.name));
            rio_gen_expr(stmt->init.expr);
        }
        break;
    }
    case RIO_STMT_ASSIGN: {
        rio_gen_expr(stmt->assign.left);
        rio_buf_printf(&(rio_gen_buf), " %s ", rio_token_kind_name(stmt->assign.op));
        rio_gen_expr(stmt->assign.right);
        break;
    }
    default: {
        assert(0);
        break;
    }
    }
}

bool rio_is_char_lit(rio_Expr (*expr)) {
    return ((expr->kind) == (RIO_EXPR_INT)) && ((expr->int_lit.mod) == (RIO_MOD_CHAR));
}

void rio_gen_stmt(rio_Stmt (*stmt)) {
    rio_gen_sync_pos(stmt->pos);
    switch (stmt->kind) {
    case RIO_STMT_RETURN: {
        rio_genln();
        rio_buf_printf(&(rio_gen_buf), "return");
        if (stmt->expr) {
            rio_buf_printf(&(rio_gen_buf), " ");
            rio_gen_expr(stmt->expr);
        }
        rio_buf_printf(&(rio_gen_buf), ";");
        break;
    }
    case RIO_STMT_BREAK: {
        rio_genln();
        rio_buf_printf(&(rio_gen_buf), "break;");
        break;
    }
    case RIO_STMT_CONTINUE: {
        rio_genln();
        rio_buf_printf(&(rio_gen_buf), "continue;");
        break;
    }
    case RIO_STMT_BLOCK: {
        rio_genln();
        rio_gen_stmt_block(stmt->block);
        break;
    }
    case RIO_STMT_NOTE: {
        if ((stmt->note.name) == (rio_assert_name)) {
            rio_genln();
            rio_buf_printf(&(rio_gen_buf), "assert(");
            assert((stmt->note.num_args) == (1));
            rio_gen_expr(stmt->note.args[0].expr);
            rio_buf_printf(&(rio_gen_buf), ");");
        }
        break;
    }
    case RIO_STMT_IF: {
        if (stmt->if_stmt.init) {
            rio_genln();
            rio_buf_printf(&(rio_gen_buf), "{");
            (rio_gen_indent)++;
            rio_gen_stmt(stmt->if_stmt.init);
        }
        rio_gen_sync_pos(stmt->pos);
        rio_genln();
        rio_buf_printf(&(rio_gen_buf), "if (");
        if (stmt->if_stmt.cond) {
            rio_gen_expr(stmt->if_stmt.cond);
        } else {
            rio_buf_printf(&(rio_gen_buf), "%s", stmt->if_stmt.init->init.name);
        }
        rio_buf_printf(&(rio_gen_buf), ") ");
        rio_gen_stmt_block(stmt->if_stmt.then_block);
        for (size_t i = 0; (i) < (stmt->if_stmt.num_elseifs); (i)++) {
            rio_ElseIf elseif = stmt->if_stmt.elseifs[i];
            rio_buf_printf(&(rio_gen_buf), " else if (");
            rio_gen_expr(elseif.cond);
            rio_buf_printf(&(rio_gen_buf), ") ");
            rio_gen_stmt_block(elseif.block);
        }
        if (stmt->if_stmt.else_block.stmts) {
            rio_buf_printf(&(rio_gen_buf), " else ");
            rio_gen_stmt_block(stmt->if_stmt.else_block);
        } else {
            rio_Note (*complete_note) = rio_get_stmt_note(stmt, rio_complete_name);
            if (complete_note) {
                rio_buf_printf(&(rio_gen_buf), " else {");
                (rio_gen_indent)++;
                rio_gen_sync_pos(complete_note->pos);
                rio_genln();
                rio_buf_printf(&(rio_gen_buf), "assert(\"@complete if/elseif chain failed to handle case\" && 0);");
                (rio_gen_indent)--;
                rio_genln();
                rio_buf_printf(&(rio_gen_buf), "}");
            }
        }
        if (stmt->if_stmt.init) {
            (rio_gen_indent)--;
            rio_genln();
            rio_buf_printf(&(rio_gen_buf), "}");
        }
        break;
    }
    case RIO_STMT_WHILE: {
        rio_genln();
        rio_buf_printf(&(rio_gen_buf), "while (");
        rio_gen_expr(stmt->while_stmt.cond);
        rio_buf_printf(&(rio_gen_buf), ") ");
        rio_gen_stmt_block(stmt->while_stmt.block);
        break;
    }
    case RIO_STMT_DO_WHILE: {
        rio_genln();
        rio_buf_printf(&(rio_gen_buf), "do ");
        rio_gen_stmt_block(stmt->while_stmt.block);
        rio_buf_printf(&(rio_gen_buf), " while (");
        rio_gen_expr(stmt->while_stmt.cond);
        rio_buf_printf(&(rio_gen_buf), ");");
        break;
    }
    case RIO_STMT_FOR: {
        rio_genln();
        rio_buf_printf(&(rio_gen_buf), "for (");
        if (stmt->for_stmt.init) {
            rio_gen_simple_stmt(stmt->for_stmt.init);
        }
        rio_buf_printf(&(rio_gen_buf), ";");
        if (stmt->for_stmt.cond) {
            rio_buf_printf(&(rio_gen_buf), " ");
            rio_gen_expr(stmt->for_stmt.cond);
        }
        rio_buf_printf(&(rio_gen_buf), ";");
        if (stmt->for_stmt.next) {
            rio_buf_printf(&(rio_gen_buf), " ");
            rio_gen_simple_stmt(stmt->for_stmt.next);
        }
        rio_buf_printf(&(rio_gen_buf), ") ");
        rio_gen_stmt_block(stmt->for_stmt.block);
        break;
    }
    case RIO_STMT_SWITCH: {
        {
            rio_genln();
            rio_buf_printf(&(rio_gen_buf), "switch (");
            rio_gen_expr(stmt->switch_stmt.expr);
            rio_buf_printf(&(rio_gen_buf), ") {");
            bool has_default = false;
            for (size_t i = 0; (i) < (stmt->switch_stmt.num_cases); (i)++) {
                rio_SwitchCase switch_case = stmt->switch_stmt.cases[i];
                for (size_t j = 0; (j) < (switch_case.num_patterns); (j)++) {
                    rio_SwitchCasePattern pattern = switch_case.patterns[j];
                    if (pattern.end) {
                        rio_Val start_val = rio_get_resolved_val(pattern.start);
                        rio_Val end_val = rio_get_resolved_val(pattern.end);
                        if ((rio_is_char_lit(pattern.start)) && (rio_is_char_lit(pattern.end))) {
                            rio_genln();
                            for (int c = (int)(start_val.ll); (c) <= ((int)(end_val.ll)); (c)++) {
                                rio_buf_printf(&(rio_gen_buf), "case ");
                                rio_gen_char(c);
                                rio_buf_printf(&(rio_gen_buf), ": ");
                            }
                        } else {
                            rio_genln();
                            rio_buf_printf(&(rio_gen_buf), "// ");
                            rio_gen_expr(pattern.start);
                            rio_buf_printf(&(rio_gen_buf), "...");
                            rio_gen_expr(pattern.end);
                            rio_genln();
                            for (llong ll = start_val.ll; (ll) <= (end_val.ll); (ll)++) {
                                rio_buf_printf(&(rio_gen_buf), "case %lld: ", ll);
                            }
                        }
                    } else {
                        rio_genln();
                        rio_buf_printf(&(rio_gen_buf), "case ");
                        rio_gen_expr(pattern.start);
                        rio_buf_printf(&(rio_gen_buf), ":");
                    }
                }
                if (switch_case.is_default) {
                    has_default = true;
                    rio_genln();
                    rio_buf_printf(&(rio_gen_buf), "default:");
                }
                rio_buf_printf(&(rio_gen_buf), " ");
                rio_buf_printf(&(rio_gen_buf), "{");
                (rio_gen_indent)++;
                rio_StmtList block = switch_case.block;
                for (size_t j = 0; (j) < (block.num_stmts); (j)++) {
                    rio_gen_stmt(block.stmts[j]);
                }
                rio_genln();
                rio_buf_printf(&(rio_gen_buf), "break;");
                (rio_gen_indent)--;
                rio_genln();
                rio_buf_printf(&(rio_gen_buf), "}");
            }
            if (!(has_default)) {
                rio_Note (*note) = rio_get_stmt_note(stmt, rio_complete_name);
                if (note) {
                    rio_genln();
                    rio_buf_printf(&(rio_gen_buf), "default:");
                    (rio_gen_indent)++;
                    rio_genln();
                    rio_buf_printf(&(rio_gen_buf), "assert(\"@complete switch failed to handle case\" && 0);");
                    rio_genln();
                    rio_buf_printf(&(rio_gen_buf), "break;");
                    (rio_gen_indent)--;
                }
            }
            rio_genln();
            rio_buf_printf(&(rio_gen_buf), "}");
        }
        break;
    }
    case RIO_STMT_LABEL: {
        rio_genln();
        rio_buf_printf(&(rio_gen_buf), "%s: ;", stmt->label);
        break;
    }
    case RIO_STMT_GOTO: {
        rio_genln();
        rio_buf_printf(&(rio_gen_buf), "goto %s;", stmt->label);
        break;
    }
    default: {
        rio_genln();
        rio_gen_simple_stmt(stmt);
        rio_buf_printf(&(rio_gen_buf), ";");
        break;
    }
    }
}

void rio_gen_decl(rio_Sym (*sym)) {
    rio_Decl (*decl) = sym->decl;
    if ((!(decl)) || (rio_is_decl_foreign(decl))) {
        return;
    }
    rio_gen_sync_pos(decl->pos);
    switch (decl->kind) {
    case RIO_DECL_CONST: {
        rio_genln();
        rio_buf_printf(&(rio_gen_buf), "#define %s (", rio_get_gen_name(sym));
        if (decl->const_decl.type) {
            rio_buf_printf(&(rio_gen_buf), "(%s)(", rio_typespec_to_cdecl(decl->const_decl.type, ""));
        }
        rio_gen_expr(decl->const_decl.expr);
        if (decl->const_decl.type) {
            rio_buf_printf(&(rio_gen_buf), ")");
        }
        rio_buf_printf(&(rio_gen_buf), ")");
        break;
    }
    case RIO_DECL_VAR: {
        rio_genln();
        rio_buf_printf(&(rio_gen_buf), "extern ");
        if ((decl->var_decl.type) && (!(rio_is_incomplete_array_typespec(decl->var_decl.type)))) {
            rio_buf_printf(&(rio_gen_buf), "%s", rio_typespec_to_cdecl(decl->var_decl.type, rio_get_gen_name(sym)));
        } else {
            rio_buf_printf(&(rio_gen_buf), "%s", rio_type_to_cdecl(sym->type, rio_get_gen_name(sym)));
        }
        rio_buf_printf(&(rio_gen_buf), ";");
        break;
    }
    case RIO_DECL_FUNC: {
        rio_gen_func_decl(decl);
        rio_buf_printf(&(rio_gen_buf), ";");
        break;
    }
    case RIO_DECL_STRUCT:
    case RIO_DECL_UNION: {
        rio_gen_aggregate(decl);
        break;
    }
    case RIO_DECL_TYPEDEF: {
        rio_genln();
        rio_buf_printf(&(rio_gen_buf), "typedef %s;", rio_typespec_to_cdecl(decl->typedef_decl.type, rio_get_gen_name(sym)));
        break;
    }
    case RIO_DECL_ENUM: {
        if (decl->enum_decl.type) {
            rio_genln();
            rio_buf_printf(&(rio_gen_buf), "typedef %s;", rio_typespec_to_cdecl(decl->enum_decl.type, rio_get_gen_name(decl)));
        } else {
            rio_genln();
            rio_buf_printf(&(rio_gen_buf), "typedef int %s;", rio_get_gen_name(decl));
        }
        break;
    }
    case RIO_DECL_IMPORT: {
        break;
        break;
    }
    default: {
        assert(0);
        break;
    }
    }
    rio_genln();
}

void rio_gen_sorted_decls(void) {
    for (size_t i = 0; (i) < (rio_buf_len(rio_sorted_syms)); (i)++) {
        if ((rio_sorted_syms[i]->reachable) == (RIO_REACHABLE_NATURAL)) {
            rio_gen_decl(rio_sorted_syms[i]);
        }
    }
}

void rio_gen_defs(void) {
    for (rio_Sym (*(*it)) = rio_sorted_syms; (it) != (rio_buf_end(rio_sorted_syms, sizeof(*(rio_sorted_syms)))); (it)++) {
        rio_Sym (*sym) = *(it);
        rio_Decl (*decl) = sym->decl;
        if ((((((sym->state) != (RIO_SYM_RESOLVED)) || (!(decl))) || (rio_is_decl_foreign(decl))) || (decl->is_incomplete)) || ((sym->reachable) != (RIO_REACHABLE_NATURAL))) {
            continue;
        }
        if ((decl->kind) == (RIO_DECL_FUNC)) {
            rio_gen_func_decl(decl);
            rio_buf_printf(&(rio_gen_buf), " ");
            rio_gen_stmt_block(decl->function.block);
            rio_genln();
        } else if ((decl->kind) == (RIO_DECL_VAR)) {
            if ((decl->var_decl.type) && (!(rio_is_incomplete_array_typespec(decl->var_decl.type)))) {
                rio_genln();
                rio_buf_printf(&(rio_gen_buf), "%s", rio_typespec_to_cdecl(decl->var_decl.type, rio_get_gen_name(sym)));
            } else {
                rio_genln();
                rio_buf_printf(&(rio_gen_buf), "%s", rio_type_to_cdecl(sym->type, rio_get_gen_name(sym)));
            }
            if (decl->var_decl.expr) {
                rio_buf_printf(&(rio_gen_buf), " = ");
                rio_gen_expr(decl->var_decl.expr);
            }
            rio_buf_printf(&(rio_gen_buf), ";");
        }
    }
}

rio_Map rio_gen_foreign_headers_map;
char const ((*(*rio_gen_foreign_headers_buf)));
void rio_add_foreign_header(char const ((*name))) {
    name = rio_str_intern(name);
    if (!(rio_map_get(&(rio_gen_foreign_headers_map), name))) {
        rio_map_put(&(rio_gen_foreign_headers_map), name, (void *)(1));
        rio_buf_push((void (**))(&(rio_gen_foreign_headers_buf)), &(name), sizeof(name));
    }
}

char const ((*(*rio_gen_foreign_sources_buf)));
void rio_add_foreign_source(char const ((*name))) {
    char const ((*interned)) = rio_str_intern(name);
    rio_buf_push((void (**))(&(rio_gen_foreign_sources_buf)), &(interned), sizeof(interned));
}

void rio_gen_include(char const ((*path))) {
    rio_genln();
    rio_buf_printf(&(rio_gen_buf), "#include ");
    if ((*(path)) == ('<')) {
        rio_buf_printf(&(rio_gen_buf), "%s", path);
    } else {
        rio_gen_str(path, false);
    }
}

void rio_gen_foreign_headers(void) {
    if (rio_gen_foreign_headers_buf) {
        rio_genln();
        rio_buf_printf(&(rio_gen_buf), "// Foreign header files");
        for (size_t i = 0; (i) < (rio_buf_len(rio_gen_foreign_headers_buf)); (i)++) {
            rio_gen_include(rio_gen_foreign_headers_buf[i]);
        }
    }
}

void rio_gen_foreign_sources(void) {
    for (size_t i = 0; (i) < (rio_buf_len(rio_gen_foreign_sources_buf)); (i)++) {
        rio_gen_include(rio_gen_foreign_sources_buf[i]);
    }
}

char const ((*(*rio_gen_sources_buf)));
void rio_put_include_path(char (path[MAX_PATH]), rio_Package (*package), char const ((*filename))) {
    if ((*(filename)) == ('<')) {
        rio_path_copy(path, filename);
    } else {
        rio_path_copy(path, package->full_path);
        rio_path_join(path, filename);
        rio_path_absolute(path);
    }
}

char (*rio_gen_preamble_buf);
char (*rio_gen_postamble_buf);
void rio_preprocess_package(rio_Package (*package)) {
    if (!(package->external_name)) {
        char (*external_name) = NULL;
        for (char const ((*ptr)) = package->path; *(ptr); (ptr)++) {
            rio_buf_printf(&(external_name), "%c", ((*(ptr)) == ('/') ? '_' : *(ptr)));
        }
        rio_buf_printf(&(external_name), "_");
        package->external_name = rio_str_intern(external_name);
    }
    char const ((*header_name)) = rio_str_intern("header");
    char const ((*source_name)) = rio_str_intern("source");
    char const ((*preamble_name)) = rio_str_intern("preamble");
    char const ((*postamble_name)) = rio_str_intern("postamble");
    for (size_t i = 0; (i) < (package->num_decls); (i)++) {
        rio_Decl (*decl) = package->decls[i];
        if ((decl->kind) != (RIO_DECL_NOTE)) {
            continue;
        }
        rio_Note note = decl->note;
        if ((note.name) == (rio_foreign_name)) {
            for (size_t k = 0; (k) < (note.num_args); (k)++) {
                rio_NoteArg arg = note.args[k];
                rio_Expr (*expr) = note.args[k].expr;
                if ((expr->kind) != (RIO_EXPR_STR)) {
                    rio_fatal_error(decl->pos, "#foreign argument must be a string");
                }
                char const ((*str)) = expr->str_lit.val;
                if ((arg.name) == (header_name)) {
                    char (path[MAX_PATH]) = {0};
                    rio_put_include_path(path, package, str);
                    rio_add_foreign_header(path);
                } else if ((arg.name) == (source_name)) {
                    char (path[MAX_PATH]) = {0};
                    rio_put_include_path(path, package, str);
                    rio_add_foreign_source(path);
                } else if ((arg.name) == (preamble_name)) {
                    rio_buf_printf(&(rio_gen_preamble_buf), "%s\n", str);
                } else if ((arg.name) == (postamble_name)) {
                    rio_buf_printf(&(rio_gen_postamble_buf), "%s\n", str);
                } else {
                    rio_fatal_error(decl->pos, "Unknown #foreign named argument \'%s\'", arg.name);
                }
            }
        }
    }
}

void rio_preprocess_packages(void) {
    for (size_t i = 0; (i) < (rio_buf_len(rio_package_list)); (i)++) {
        rio_preprocess_package(rio_package_list[i]);
    }
}

void rio_gen_typeinfo_header(char const ((*kind)), rio_Type (*type)) {
    if ((rio_type_sizeof(type)) == (0)) {
        rio_buf_printf(&(rio_gen_buf), "&(TypeInfo){%s, .size = 0, .align = 0", kind);
    } else {
        char (*ctype) = rio_type_to_cdecl(type, "");
        rio_buf_printf(&(rio_gen_buf), "&(TypeInfo){%s, .size = sizeof(%s), .align = alignof(%s)", kind, ctype, ctype);
    }
}

void rio_gen_typeinfo_fields(rio_Type (*type)) {
    (rio_gen_indent)++;
    for (size_t i = 0; (i) < (type->aggregate.num_fields); (i)++) {
        rio_TypeField field = type->aggregate.fields[i];
        rio_genln();
        rio_buf_printf(&(rio_gen_buf), "{");
        rio_gen_str(field.name, false);
        rio_buf_printf(&(rio_gen_buf), ", .type = ");
        rio_gen_typeid(field.type);
        rio_buf_printf(&(rio_gen_buf), ", .offset = offsetof(%s, %s)},", rio_get_gen_name(type->sym), field.name);
    }
    (rio_gen_indent)--;
}

void rio_gen_typeinfo(rio_Type (*type)) {
    switch (type->kind) {
    case RIO_CMPL_TYPE_BOOL: {
        rio_buf_printf(&(rio_gen_buf), "&(TypeInfo){TYPE_BOOL, .size = sizeof(bool), .align = sizeof(bool), .name = ");
        rio_gen_str("bool", false);
        rio_buf_printf(&(rio_gen_buf), "},");
        break;
    }
    case RIO_CMPL_TYPE_CHAR: {
        rio_buf_printf(&(rio_gen_buf), "&(TypeInfo){TYPE_CHAR, .size = sizeof(char), .align = sizeof(char), .name = ");
        rio_gen_str("char", false);
        rio_buf_printf(&(rio_gen_buf), "},");
        break;
    }
    case RIO_CMPL_TYPE_UCHAR: {
        rio_buf_printf(&(rio_gen_buf), "&(TypeInfo){TYPE_UCHAR, .size = sizeof(uchar), .align = sizeof(uchar), .name = ");
        rio_gen_str("uchar", false);
        rio_buf_printf(&(rio_gen_buf), "},");
        break;
    }
    case RIO_CMPL_TYPE_SCHAR: {
        rio_buf_printf(&(rio_gen_buf), "&(TypeInfo){TYPE_SCHAR, .size = sizeof(schar), .align = sizeof(schar), .name = ");
        rio_gen_str("schar", false);
        rio_buf_printf(&(rio_gen_buf), "},");
        break;
    }
    case RIO_CMPL_TYPE_SHORT: {
        rio_buf_printf(&(rio_gen_buf), "&(TypeInfo){TYPE_SHORT, .size = sizeof(short), .align = sizeof(short), .name = ");
        rio_gen_str("short", false);
        rio_buf_printf(&(rio_gen_buf), "},");
        break;
    }
    case RIO_CMPL_TYPE_USHORT: {
        rio_buf_printf(&(rio_gen_buf), "&(TypeInfo){TYPE_USHORT, .size = sizeof(ushort), .align = sizeof(ushort), .name = ");
        rio_gen_str("ushort", false);
        rio_buf_printf(&(rio_gen_buf), "},");
        break;
    }
    case RIO_CMPL_TYPE_INT: {
        rio_buf_printf(&(rio_gen_buf), "&(TypeInfo){TYPE_INT, .size = sizeof(int), .align = sizeof(int), .name = ");
        rio_gen_str("int", false);
        rio_buf_printf(&(rio_gen_buf), "},");
        break;
    }
    case RIO_CMPL_TYPE_UINT: {
        rio_buf_printf(&(rio_gen_buf), "&(TypeInfo){TYPE_UINT, .size = sizeof(uint), .align = sizeof(uint), .name = ");
        rio_gen_str("uint", false);
        rio_buf_printf(&(rio_gen_buf), "},");
        break;
    }
    case RIO_CMPL_TYPE_LONG: {
        rio_buf_printf(&(rio_gen_buf), "&(TypeInfo){TYPE_LONG, .size = sizeof(long), .align = sizeof(long), .name = ");
        rio_gen_str("long", false);
        rio_buf_printf(&(rio_gen_buf), "},");
        break;
    }
    case RIO_CMPL_TYPE_ULONG: {
        rio_buf_printf(&(rio_gen_buf), "&(TypeInfo){TYPE_ULONG, .size = sizeof(ulong), .align = sizeof(ulong), .name = ");
        rio_gen_str("ulong", false);
        rio_buf_printf(&(rio_gen_buf), "},");
        break;
    }
    case RIO_CMPL_TYPE_LLONG: {
        rio_buf_printf(&(rio_gen_buf), "&(TypeInfo){TYPE_LLONG, .size = sizeof(llong), .align = sizeof(llong), .name = ");
        rio_gen_str("llong", false);
        rio_buf_printf(&(rio_gen_buf), "},");
        break;
    }
    case RIO_CMPL_TYPE_ULLONG: {
        rio_buf_printf(&(rio_gen_buf), "&(TypeInfo){TYPE_ULLONG, .size = sizeof(ullong), .align = sizeof(ullong), .name = ");
        rio_gen_str("ullong", false);
        rio_buf_printf(&(rio_gen_buf), "},");
        break;
    }
    case RIO_CMPL_TYPE_FLOAT: {
        rio_buf_printf(&(rio_gen_buf), "&(TypeInfo){TYPE_FLOAT, .size = sizeof(float), .align = sizeof(float), .name = ");
        rio_gen_str("float", false);
        rio_buf_printf(&(rio_gen_buf), "},");
        break;
    }
    case RIO_CMPL_TYPE_DOUBLE: {
        rio_buf_printf(&(rio_gen_buf), "&(TypeInfo){TYPE_DOUBLE, .size = sizeof(double), .align = sizeof(double), .name = ");
        rio_gen_str("double", false);
        rio_buf_printf(&(rio_gen_buf), "},");
        break;
    }
    case RIO_CMPL_TYPE_VOID: {
        rio_buf_printf(&(rio_gen_buf), "&(TypeInfo){TYPE_VOID, .name = \"void\", .size = 0, .align = 0},");
        break;
    }
    case RIO_CMPL_TYPE_PTR: {
        rio_buf_printf(&(rio_gen_buf), "&(TypeInfo){TYPE_PTR, .size = sizeof(void *), .align = alignof(void *), .base = ");
        rio_gen_typeid(type->base);
        rio_buf_printf(&(rio_gen_buf), "},");
        break;
    }
    case RIO_CMPL_TYPE_CONST: {
        rio_gen_typeinfo_header("TYPE_CONST", type);
        rio_buf_printf(&(rio_gen_buf), ", .base = ");
        rio_gen_typeid(type->base);
        rio_buf_printf(&(rio_gen_buf), "},");
        break;
    }
    case RIO_CMPL_TYPE_ARRAY: {
        if (rio_is_incomplete_array_type(type)) {
            rio_buf_printf(&(rio_gen_buf), "NULL, // Incomplete array type");
        } else {
            rio_gen_typeinfo_header("TYPE_ARRAY", type);
            rio_buf_printf(&(rio_gen_buf), ", .base = ");
            rio_gen_typeid(type->base);
            rio_buf_printf(&(rio_gen_buf), ", .count = %d},", type->num_elems);
        }
        break;
    }
    case RIO_CMPL_TYPE_STRUCT:
    case RIO_CMPL_TYPE_UNION: {
        rio_gen_typeinfo_header(((type->kind) == (RIO_CMPL_TYPE_STRUCT) ? "TYPE_STRUCT" : "TYPE_UNION"), type);
        rio_buf_printf(&(rio_gen_buf), ", .name = ");
        rio_gen_str(rio_get_gen_name(type->sym), false);
        rio_buf_printf(&(rio_gen_buf), ", .num_fields = %d, .fields = (TypeFieldInfo[]) {", type->aggregate.num_fields);
        rio_gen_typeinfo_fields(type);
        rio_genln();
        rio_buf_printf(&(rio_gen_buf), "}},");
        break;
    }
    case RIO_CMPL_TYPE_FUNC: {
        rio_buf_printf(&(rio_gen_buf), "NULL, // Function");
        break;
    }
    case RIO_CMPL_TYPE_ENUM: {
        rio_buf_printf(&(rio_gen_buf), "NULL, // Enum");
        break;
    }
    case RIO_CMPL_TYPE_INCOMPLETE: {
        rio_buf_printf(&(rio_gen_buf), "NULL, // Incomplete: %s", rio_get_gen_name(type->sym));
        break;
    }
    default: {
        rio_buf_printf(&(rio_gen_buf), "NULL, // Unhandled");
        break;
    }
    }
}

void rio_gen_typeinfos(void) {
    rio_genln();
    rio_buf_printf(&(rio_gen_buf), "#define TYPEID0(index, kind) ((ullong)(index) | ((ullong)(kind) << 24))");
    rio_genln();
    rio_buf_printf(&(rio_gen_buf), "#define TYPEID(index, kind, ...) ((ullong)(index) | ((ullong)sizeof(__VA_ARGS__) << 32) | ((ullong)(kind) << 24))");
    rio_genln();
    if (rio_flag_notypeinfo) {
        rio_genln();
        rio_buf_printf(&(rio_gen_buf), "int num_typeinfos;");
        rio_genln();
        rio_buf_printf(&(rio_gen_buf), "const TypeInfo **typeinfos;");
    } else {
        int num_typeinfos = rio_next_typeid;
        rio_genln();
        rio_buf_printf(&(rio_gen_buf), "const TypeInfo *typeinfo_table[%d] = {", num_typeinfos);
        (rio_gen_indent)++;
        for (int typeid = 0; (typeid) < (num_typeinfos); (typeid)++) {
            rio_genln();
            rio_buf_printf(&(rio_gen_buf), "[%d] = ", typeid);
            rio_Type (*type) = rio_get_type_from_typeid(typeid);
            if ((type) && (!(rio_is_excluded_typeinfo(type)))) {
                rio_gen_typeinfo(type);
            } else {
                rio_buf_printf(&(rio_gen_buf), "NULL, // No associated type");
            }
        }
        (rio_gen_indent)--;
        rio_genln();
        rio_buf_printf(&(rio_gen_buf), "};");
        rio_genln();
        rio_genln();
        rio_buf_printf(&(rio_gen_buf), "int num_typeinfos = %d;", num_typeinfos);
        rio_genln();
        rio_buf_printf(&(rio_gen_buf), "const TypeInfo **typeinfos = (const TypeInfo **)typeinfo_table;");
    }
}

void rio_gen_package_external_names(void) {
    for (size_t i = 0; (i) < (rio_buf_len(rio_package_list)); (i)++) {
    }
}

void rio_gen_preamble(void) {
    rio_buf_printf(&(rio_gen_buf), "%s", rio_gen_preamble_str);
    if (rio_gen_preamble_buf) {
        rio_genln();
        rio_genln();
        rio_buf_printf(&(rio_gen_buf), "// Foreign preamble");
        rio_genln();
        rio_buf_printf(&(rio_gen_buf), "%s", rio_gen_preamble_buf);
    }
}

void rio_gen_postamble(void) {
    rio_buf_printf(&(rio_gen_buf), "%s", rio_gen_postamble_str);
    if (rio_gen_postamble_buf) {
        rio_genln();
        rio_genln();
        rio_buf_printf(&(rio_gen_buf), "// Foreign postamble");
        rio_genln();
        rio_buf_printf(&(rio_gen_buf), "%s", rio_gen_postamble_buf);
    }
}

void rio_gen_all(void) {
    rio_preprocess_packages();
    rio_gen_buf = NULL;
    rio_gen_preamble();
    rio_gen_foreign_headers();
    rio_genln();
    rio_genln();
    rio_buf_printf(&(rio_gen_buf), "// Forward declarations");
    rio_gen_forward_decls();
    rio_genln();
    rio_genln();
    rio_buf_printf(&(rio_gen_buf), "// Sorted declarations");
    rio_gen_sorted_decls();
    rio_genln();
    rio_buf_printf(&(rio_gen_buf), "// Typeinfo");
    rio_gen_typeinfos();
    rio_genln();
    rio_genln();
    rio_buf_printf(&(rio_gen_buf), "// Definitions");
    rio_gen_defs();
    rio_genln();
    rio_buf_printf(&(rio_gen_buf), "// Foreign source files");
    rio_gen_foreign_sources();
    rio_genln();
    rio_gen_postamble();
}

char const ((*(rio_static_package_search_paths[RIO_MAX_SEARCH_PATHS])));
char const ((*(*rio_package_search_paths))) = rio_static_package_search_paths;
int rio_num_package_search_paths;
void rio_add_package_search_path(char const ((*path))) {
    if (rio_flag_verbose) {
        printf("Adding package search path %s\n", path);
    }
    rio_package_search_paths[(rio_num_package_search_paths)++] = rio_str_intern(path);
}

void rio_add_package_search_path_range(char const ((*start)), char const ((*end))) {
    char (path[MAX_PATH]) = {0};
    size_t len = rio_clamp_max((end) - (start), (MAX_PATH) - (1));
    memcpy(path, start, len);
    path[len] = 0;
    rio_add_package_search_path(path);
}

void rio_init_package_search_paths(void) {
    char (*riohome_var) = getenv("RIOHOME");
    if (!(riohome_var)) {
        printf("error: Set the environment variable RIOHOME to the Ion home directory (where system_packages is located)\n");
        exit(1);
    }
    char (path[MAX_PATH]) = {0};
    rio_path_copy(path, riohome_var);
    rio_path_join(path, "system_packages");
    rio_add_package_search_path(path);
    rio_add_package_search_path(".");
    char (*riopath_var) = getenv("RIOPATH");
    if (riopath_var) {
        char (*start) = riopath_var;
        for (char (*ptr) = riopath_var; *(ptr); (ptr)++) {
            if ((*(ptr)) == (';')) {
                rio_add_package_search_path_range(start, ptr);
                start = (ptr) + (1);
            }
        }
        if (*(start)) {
            rio_add_package_search_path(start);
        }
    }
}

void rio_init_compiler(void) {
    rio_init_target();
    rio_init_package_search_paths();
    rio_init_keywords();
    rio_init_builtin_types();
    rio_map_put(&(rio_decl_note_names), rio_declare_note_name, (void *)(1));
}

void rio_parse_env_vars(void) {
    char (*rioos_var) = getenv("RIOOS");
    if (rioos_var) {
        int os = rio_get_os(rioos_var);
        if ((os) == (-(1))) {
            printf("Unknown target operating system in RIOOS environment variable: %s\n", rioos_var);
        } else {
            rio_target_os = os;
        }
    }
    char (*rioarch_var) = getenv("RIOARCH");
    if (rioarch_var) {
        int arch = rio_get_arch(rioarch_var);
        if ((arch) == (-(1))) {
            printf("Unknown target architecture in RIOARCH environment variable: %s\n", rioarch_var);
        } else {
            rio_target_arch = arch;
        }
    }
}

int rio_rio_main(int argc, char const ((*(*argv))), void (*gen_all)(void), char const ((*extension))) {
    rio_parse_env_vars();
    char const ((*output_name)) = {0};
    bool flag_check = false;
    rio_add_flag_str("o", &(output_name), "file", "Output file (default: out_<main-package>.c)");
    rio_add_flag_enum("os", &(rio_target_os), "Target operating system", rio_os_names, RIO_NUM_OSES);
    rio_add_flag_enum("arch", &(rio_target_arch), "Target machine architecture", rio_arch_names, RIO_NUM_ARCHES);
    rio_add_flag_bool("check", &(flag_check), "Semantic checking with no code generation");
    rio_add_flag_bool("lazy", &(rio_flag_lazy), "Only compile what\'s reachable from the main package");
    rio_add_flag_bool("nosourcemap", &(rio_flag_nosourcemap), "Don\'t generate any source map information");
    rio_add_flag_bool("notypeinfo", &(rio_flag_notypeinfo), "Don\'t generate any typeinfo tables");
    rio_add_flag_bool("fullgen", &(rio_flag_fullgen), "Force full code generation even for non-reachable symbols");
    rio_add_flag_bool("verbose", &(rio_flag_verbose), "Extra diagnostic information");
    char const ((*program_name)) = rio_parse_flags(&(argc), &(argv));
    if ((argc) != (1)) {
        printf("Usage: %s [flags] <main-package>\n", program_name);
        rio_print_flags_usage();
        return 1;
    }
    char const ((*package_name)) = argv[0];
    if (rio_flag_verbose) {
        printf("Target operating system: %s\n", rio_os_names[rio_target_os]);
        printf("Target architecture: %s\n", rio_arch_names[rio_target_arch]);
    }
    rio_init_compiler();
    rio_builtin_package = rio_import_package("builtin");
    if (!(rio_builtin_package)) {
        printf("error: Failed to compile package \'builtin\'.\n");
        return 1;
    }
    rio_builtin_package->external_name = rio_str_intern("");
    rio_Package (*main_package) = rio_import_package(package_name);
    if (!(main_package)) {
        printf("error: Failed to compile package \'%s\'\n", package_name);
        return 1;
    }
    char const ((*main_name)) = rio_str_intern("main");
    rio_Sym (*main_sym) = rio_get_package_sym(main_package, main_name);
    if (!(main_sym)) {
        printf("error: No \'main\' entry point defined in package \'%s\'\n", package_name);
        return 1;
    }
    main_sym->external_name = main_name;
    rio_reachable_phase = RIO_REACHABLE_NATURAL;
    rio_resolve_sym(main_sym);
    for (size_t i = 0; (i) < (rio_buf_len(rio_package_list)); (i)++) {
        if (rio_package_list[i]->always_reachable) {
            rio_resolve_package_syms(rio_package_list[i]);
        }
    }
    rio_finalize_reachable_syms();
    if (rio_flag_verbose) {
        printf("Reached %d symbols in %d packages from %s/main\n", (int)(rio_buf_len(rio_reachable_syms)), (int)(rio_buf_len(rio_package_list)), package_name);
    }
    if (!(rio_flag_lazy)) {
        rio_reachable_phase = RIO_REACHABLE_FORCED;
        for (size_t i = 0; (i) < (rio_buf_len(rio_package_list)); (i)++) {
            rio_resolve_package_syms(rio_package_list[i]);
        }
        rio_finalize_reachable_syms();
    }
    printf("Processed %d symbols in %d packages\n", (int)(rio_buf_len(rio_reachable_syms)), (int)(rio_buf_len(rio_package_list)));
    if (!(flag_check)) {
        char (c_path[MAX_PATH]) = {0};
        if (output_name) {
            rio_path_copy(c_path, output_name);
        } else {
            snprintf(c_path, sizeof(c_path), "out_%s.%s", package_name, extension);
        }
        gen_all();
        char (*c_code) = rio_gen_buf;
        rio_gen_buf = NULL;
        if (!(rio_write_file(c_path, c_code, rio_buf_len(c_code)))) {
            printf("error: Failed to write file: %s\n", c_path);
            return 1;
        }
        printf("Generated %s\n", c_path);
    }
    return 0;
}

char const ((*rio_typedef_keyword));
char const ((*rio_enum_keyword));
char const ((*rio_struct_keyword));
char const ((*rio_union_keyword));
char const ((*rio_let_keyword));
char const ((*rio_mut_keyword));
char const ((*rio_const_keyword));
char const ((*rio_fn_keyword));
char const ((*rio_sizeof_keyword));
char const ((*rio_alignof_keyword));
char const ((*rio_typeof_keyword));
char const ((*rio_offsetof_keyword));
char const ((*rio_break_keyword));
char const ((*rio_continue_keyword));
char const ((*rio_return_keyword));
char const ((*rio_if_keyword));
char const ((*rio_else_keyword));
char const ((*rio_while_keyword));
char const ((*rio_do_keyword));
char const ((*rio_for_keyword));
char const ((*rio_switch_keyword));
char const ((*rio_case_keyword));
char const ((*rio_default_keyword));
char const ((*rio_import_keyword));
char const ((*rio_goto_keyword));
char const ((*rio_first_keyword));
char const ((*rio_last_keyword));
char const ((*(*rio_keywords)));
char const ((*rio_always_name));
char const ((*rio_foreign_name));
char const ((*rio_complete_name));
char const ((*rio_assert_name));
char const ((*rio_declare_note_name));
char const ((*rio_static_assert_name));
char const ((*rio_init_keyword(char const ((*keyword))))) {
    keyword = rio_str_intern(keyword);
    rio_buf_push((void (**))(&(rio_keywords)), (void *)(&(keyword)), sizeof(keyword));
    return keyword;
}

bool rio_keywords_inited = false;
void rio_init_keywords(void) {
    if (rio_keywords_inited) {
        return;
    }
    rio_typedef_keyword = rio_init_keyword("typedef");
    char (*arena_end) = rio_intern_arena.end;
    rio_enum_keyword = rio_init_keyword("enum");
    rio_struct_keyword = rio_init_keyword("struct");
    rio_union_keyword = rio_init_keyword("union");
    rio_const_keyword = rio_init_keyword("const");
    rio_let_keyword = rio_init_keyword("let");
    rio_mut_keyword = rio_init_keyword("mut");
    rio_fn_keyword = rio_init_keyword("fn");
    rio_import_keyword = rio_init_keyword("import");
    rio_goto_keyword = rio_init_keyword("goto");
    rio_sizeof_keyword = rio_init_keyword("sizeof");
    rio_alignof_keyword = rio_init_keyword("alignof");
    rio_typeof_keyword = rio_init_keyword("typeof");
    rio_offsetof_keyword = rio_init_keyword("offsetof");
    rio_break_keyword = rio_init_keyword("break");
    rio_continue_keyword = rio_init_keyword("continue");
    rio_return_keyword = rio_init_keyword("return");
    rio_if_keyword = rio_init_keyword("if");
    rio_else_keyword = rio_init_keyword("else");
    rio_while_keyword = rio_init_keyword("while");
    rio_do_keyword = rio_init_keyword("do");
    rio_for_keyword = rio_init_keyword("for");
    rio_switch_keyword = rio_init_keyword("switch");
    rio_case_keyword = rio_init_keyword("case");
    rio_default_keyword = rio_init_keyword("default");
    assert((rio_intern_arena.end) == (arena_end));
    rio_first_keyword = rio_typedef_keyword;
    rio_last_keyword = rio_default_keyword;
    rio_always_name = rio_str_intern("always");
    rio_foreign_name = rio_str_intern("foreign");
    rio_complete_name = rio_str_intern("complete");
    rio_assert_name = rio_str_intern("assert");
    rio_declare_note_name = rio_str_intern("declare_note");
    rio_static_assert_name = rio_str_intern("static_assert");
    rio_keywords_inited = true;
}

bool rio_is_keyword_name(char const ((*name))) {
    return ((rio_first_keyword) <= (name)) && ((name) <= (rio_last_keyword));
}

char const ((*(rio_token_suffix_names[7]))) = {[RIO_SUFFIX_NONE] = "", [RIO_SUFFIX_D] = "d", [RIO_SUFFIX_U] = "u", [RIO_SUFFIX_L] = "l", [RIO_SUFFIX_UL] = "ul", [RIO_SUFFIX_LL] = "ll", [RIO_SUFFIX_ULL] = "ull"};
char const ((*(rio_token_kind_names[53]))) = {[RIO_TOKEN_EOF] = "EOF", [RIO_TOKEN_COLON] = ":", [RIO_TOKEN_LPAREN] = "(", [RIO_TOKEN_RPAREN] = ")", [RIO_TOKEN_LBRACE] = "{", [RIO_TOKEN_RBRACE] = "}", [RIO_TOKEN_LBRACKET] = "[", [RIO_TOKEN_RBRACKET] = "]", [RIO_TOKEN_COMMA] = ",", [RIO_TOKEN_DOT] = ".", [RIO_TOKEN_AT] = "@", [RIO_TOKEN_POUND] = "#", [RIO_TOKEN_ELLIPSIS] = "...", [RIO_TOKEN_QUESTION] = "?", [RIO_TOKEN_SEMICOLON] = ";", [RIO_TOKEN_KEYWORD] = "keyword", [RIO_TOKEN_INT] = "int", [RIO_TOKEN_FLOAT] = "float", [RIO_TOKEN_STR] = "string", [RIO_TOKEN_NAME] = "name", [RIO_TOKEN_NEG] = "~", [RIO_TOKEN_NOT] = "!", [RIO_TOKEN_MUL] = "*", [RIO_TOKEN_DIV] = "/", [RIO_TOKEN_MOD] = "%", [RIO_TOKEN_AND] = "&", [RIO_TOKEN_LSHIFT] = "<<", [RIO_TOKEN_RSHIFT] = ">>", [RIO_TOKEN_ADD] = "+", [RIO_TOKEN_SUB] = "-", [RIO_TOKEN_OR] = "|", [RIO_TOKEN_XOR] = "^", [RIO_TOKEN_EQ] = "==", [RIO_TOKEN_NOTEQ] = "!=", [RIO_TOKEN_LT] = "<", [RIO_TOKEN_GT] = ">", [RIO_TOKEN_LTEQ] = "<=", [RIO_TOKEN_GTEQ] = ">=", [RIO_TOKEN_AND_AND] = "&&", [RIO_TOKEN_OR_OR] = "||", [RIO_TOKEN_ASSIGN] = "=", [RIO_TOKEN_ADD_ASSIGN] = "+=", [RIO_TOKEN_SUB_ASSIGN] = "-=", [RIO_TOKEN_OR_ASSIGN] = "|=", [RIO_TOKEN_AND_ASSIGN] = "&=", [RIO_TOKEN_XOR_ASSIGN] = "^=", [RIO_TOKEN_MUL_ASSIGN] = "*=", [RIO_TOKEN_DIV_ASSIGN] = "/=", [RIO_TOKEN_MOD_ASSIGN] = "%=", [RIO_TOKEN_LSHIFT_ASSIGN] = "<<=", [RIO_TOKEN_RSHIFT_ASSIGN] = ">>=", [RIO_TOKEN_INC] = "++", [RIO_TOKEN_DEC] = "--"};
char const ((*rio_token_kind_name(rio_TokenKind kind))) {
    if ((kind) < ((sizeof(rio_token_kind_names)) / (sizeof(*(rio_token_kind_names))))) {
        return rio_token_kind_names[kind];
    } else {
        return "<unknown>";
    }
}

rio_TokenKind (rio_assign_token_to_binary_token[RIO_NUM_TOKEN_KINDS]) = {[RIO_TOKEN_ADD_ASSIGN] = RIO_TOKEN_ADD, [RIO_TOKEN_SUB_ASSIGN] = RIO_TOKEN_SUB, [RIO_TOKEN_OR_ASSIGN] = RIO_TOKEN_OR, [RIO_TOKEN_AND_ASSIGN] = RIO_TOKEN_AND, [RIO_TOKEN_XOR_ASSIGN] = RIO_TOKEN_XOR, [RIO_TOKEN_LSHIFT_ASSIGN] = RIO_TOKEN_LSHIFT, [RIO_TOKEN_RSHIFT_ASSIGN] = RIO_TOKEN_RSHIFT, [RIO_TOKEN_MUL_ASSIGN] = RIO_TOKEN_MUL, [RIO_TOKEN_DIV_ASSIGN] = RIO_TOKEN_DIV, [RIO_TOKEN_MOD_ASSIGN] = RIO_TOKEN_MOD};
rio_SrcPos rio_pos_builtin = {.name = "<builtin>"};
rio_Token rio_token;
char const ((*rio_stream));
char const ((*rio_line_start));
void rio_vnotice(char const ((*level)), rio_SrcPos pos, char const ((*fmt)), va_list args) {
    if ((pos.name) == (NULL)) {
        pos = rio_pos_builtin;
    }
    printf("%s(%d): %s: ", pos.name, pos.line, level);
    vprintf(fmt, args);
    printf("\n");
}

void rio_warning(rio_SrcPos pos, char const ((*fmt)), ...) {
    va_list args = {0};
    va_start_ptr(&(args), &(fmt));
    rio_vnotice("warning", pos, fmt, args);
    va_end_ptr(&(args));
}

void rio_verror(rio_SrcPos pos, char const ((*fmt)), va_list args) {
    rio_vnotice("error", pos, fmt, args);
}

void rio_error(rio_SrcPos pos, char const ((*fmt)), ...) {
    va_list args = {0};
    va_start_ptr(&(args), &(fmt));
    rio_verror(pos, fmt, args);
    va_end_ptr(&(args));
}

void rio_fatal_error(rio_SrcPos pos, char const ((*fmt)), ...) {
    va_list args = {0};
    va_start_ptr(&(args), &(fmt));
    rio_verror(pos, fmt, args);
    va_end_ptr(&(args));
    exit(1);
}

char const ((*rio_token_info(void))) {
    if (((rio_token.kind) == (RIO_TOKEN_NAME)) || ((rio_token.kind) == (RIO_TOKEN_KEYWORD))) {
        return rio_token.name;
    } else {
        return rio_token_kind_name(rio_token.kind);
    }
}

uint8_t (rio_char_to_digit[256]) = {['0'] = 0, ['1'] = 1, ['2'] = 2, ['3'] = 3, ['4'] = 4, ['5'] = 5, ['6'] = 6, ['7'] = 7, ['8'] = 8, ['9'] = 9, ['a'] = 10, ['A'] = 10, ['b'] = 11, ['B'] = 11, ['c'] = 12, ['C'] = 12, ['d'] = 13, ['D'] = 13, ['e'] = 14, ['E'] = 14, ['f'] = 15, ['F'] = 15};
void rio_scan_int(void) {
    int base = 10;
    char const ((*start_digits)) = rio_stream;
    if ((*(rio_stream)) == ('0')) {
        (rio_stream)++;
        if ((tolower(*(rio_stream))) == ('x')) {
            (rio_stream)++;
            rio_token.mod = RIO_MOD_HEX;
            base = 16;
            start_digits = rio_stream;
        } else if ((tolower(*(rio_stream))) == ('b')) {
            (rio_stream)++;
            rio_token.mod = RIO_MOD_BIN;
            base = 2;
            start_digits = rio_stream;
        } else if (isdigit(*(rio_stream))) {
            rio_token.mod = RIO_MOD_OCT;
            base = 8;
            start_digits = rio_stream;
        }
    }
    ullong val = 0;
    for (;;) {
        if ((*(rio_stream)) == ('_')) {
            (rio_stream)++;
            continue;
        }
        uchar digit = rio_char_to_digit[(uchar)(*(rio_stream))];
        if (((digit) == (0)) && ((*(rio_stream)) != ('0'))) {
            break;
        }
        if ((digit) >= (base)) {
            rio_error(rio_token.pos, "Digit \'%c\' out of range for base %d", *(rio_stream), base);
            digit = 0;
        }
        if ((val) > ((((ULLONG_MAX) - (digit))) / (base))) {
            rio_error(rio_token.pos, "Integer literal overflow");
            while (isdigit(*(rio_stream))) {
                (rio_stream)++;
            }
            val = 0;
            break;
        }
        val = ((val) * (base)) + (digit);
        (rio_stream)++;
    }
    if ((rio_stream) == (start_digits)) {
        rio_error(rio_token.pos, "Expected base %d digit, got \'%c\'", base, *(rio_stream));
    }
    rio_token.kind = RIO_TOKEN_INT;
    rio_token.int_val = val;
    if ((tolower(*(rio_stream))) == ('u')) {
        rio_token.suffix = RIO_SUFFIX_U;
        (rio_stream)++;
        if ((tolower(*(rio_stream))) == ('l')) {
            rio_token.suffix = RIO_SUFFIX_UL;
            (rio_stream)++;
            if ((tolower(*(rio_stream))) == ('l')) {
                rio_token.suffix = RIO_SUFFIX_ULL;
                (rio_stream)++;
            }
        }
    } else if ((tolower(*(rio_stream))) == ('l')) {
        rio_token.suffix = RIO_SUFFIX_L;
        (rio_stream)++;
        if ((tolower(*(rio_stream))) == ('l')) {
            rio_token.suffix = RIO_SUFFIX_LL;
            (rio_stream)++;
        }
    }
}

void rio_scan_float(void) {
    char const ((*start)) = rio_stream;
    while (isdigit(*(rio_stream))) {
        (rio_stream)++;
    }
    if ((*(rio_stream)) == ('.')) {
        (rio_stream)++;
    }
    while (isdigit(*(rio_stream))) {
        (rio_stream)++;
    }
    if ((tolower(*(rio_stream))) == ('e')) {
        (rio_stream)++;
        if (((*(rio_stream)) == ('+')) || ((*(rio_stream)) == ('-'))) {
            (rio_stream)++;
        }
        if (!(isdigit(*(rio_stream)))) {
            rio_error(rio_token.pos, "Expected digit after float literal exponent, found \'%c\'.", *(rio_stream));
        }
        while (isdigit(*(rio_stream))) {
            (rio_stream)++;
        }
    }
    double val = strtod(start, NULL);
    if ((val) == (HUGE_VAL)) {
        rio_error(rio_token.pos, "Float literal overflow");
    }
    rio_token.kind = RIO_TOKEN_FLOAT;
    rio_token.float_val = val;
    if ((tolower(*(rio_stream))) == ('d')) {
        rio_token.suffix = RIO_SUFFIX_D;
        (rio_stream)++;
    }
}

char (rio_escape_to_char[256]) = {['0'] = '\0', ['\''] = '\'', ['\"'] = '\"', ['\\'] = '\\', ['n'] = '\n', ['r'] = '\r', ['t'] = '\t', ['v'] = '\v', ['b'] = '\b', ['a'] = '\a'};
int rio_scan_hex_escape(void) {
    assert((*(rio_stream)) == ('x'));
    (rio_stream)++;
    uchar val = rio_char_to_digit[(uchar)(*(rio_stream))];
    if ((!(val)) && ((*(rio_stream)) != ('0'))) {
        rio_error(rio_token.pos, "\\x needs at least 1 hex digit");
    }
    (rio_stream)++;
    uchar digit = rio_char_to_digit[(uchar)(*(rio_stream))];
    if ((digit) || ((*(rio_stream)) == ('0'))) {
        val *= 16;
        val += digit;
        if ((val) > (0xff)) {
            rio_error(rio_token.pos, "\\x argument out of range");
            val = 0xff;
        }
        (rio_stream)++;
    }
    return val;
}

void rio_scan_char(void) {
    assert((*(rio_stream)) == ('\''));
    (rio_stream)++;
    int val = 0;
    if ((*(rio_stream)) == ('\'')) {
        rio_error(rio_token.pos, "Char literal cannot be empty");
        (rio_stream)++;
    } else if ((*(rio_stream)) == ('\n')) {
        rio_error(rio_token.pos, "Char literal cannot contain newline");
    } else if ((*(rio_stream)) == ('\\')) {
        (rio_stream)++;
        if ((*(rio_stream)) == ('x')) {
            val = rio_scan_hex_escape();
        } else {
            val = rio_escape_to_char[(uchar)(*(rio_stream))];
            if (((val) == (0)) && ((*(rio_stream)) != ('0'))) {
                rio_error(rio_token.pos, "Invalid char literal escape \'\\%c\'", *(rio_stream));
            }
            (rio_stream)++;
        }
    } else {
        val = *(rio_stream);
        (rio_stream)++;
    }
    if ((*(rio_stream)) != ('\'')) {
        rio_error(rio_token.pos, "Expected closing char quote, got \'%c\'", *(rio_stream));
    } else {
        (rio_stream)++;
    }
    rio_token.kind = RIO_TOKEN_INT;
    rio_token.int_val = val;
    rio_token.mod = RIO_MOD_CHAR;
}

void rio_scan_str(void) {
    assert((*(rio_stream)) == ('\"'));
    (rio_stream)++;
    char (*str) = NULL;
    if (((rio_stream[0]) == ('\"')) && ((rio_stream[1]) == ('\"'))) {
        rio_stream += 2;
        while (*(rio_stream)) {
            if ((((rio_stream[0]) == ('\"')) && ((rio_stream[1]) == ('\"'))) && ((rio_stream[2]) == ('\"'))) {
                rio_stream += 3;
                break;
            }
            if ((*(rio_stream)) != ('\r')) {
                rio_buf_push((void (**))(&(str)), (void *)(rio_stream), 1);
            }
            if ((*(rio_stream)) == ('\n')) {
                (rio_token.pos.line)++;
            }
            (rio_stream)++;
        }
        if (!(*(rio_stream))) {
            rio_error(rio_token.pos, "Unexpected end of file within multi-line string literal");
        }
        rio_token.mod = RIO_MOD_MULTILINE;
    } else {
        while ((*(rio_stream)) && ((*(rio_stream)) != ('\"'))) {
            char val = *(rio_stream);
            if ((val) == ('\n')) {
                rio_error(rio_token.pos, "String literal cannot contain newline");
                break;
            } else if ((val) == ('\\')) {
                (rio_stream)++;
                if ((*(rio_stream)) == ('x')) {
                    val = rio_scan_hex_escape();
                } else {
                    val = rio_escape_to_char[(uchar)(*(rio_stream))];
                    if (((val) == (0)) && ((*(rio_stream)) != ('0'))) {
                        rio_error(rio_token.pos, "Invalid string literal escape \'\\%c\'", *(rio_stream));
                    }
                    (rio_stream)++;
                }
            } else {
                (rio_stream)++;
            }
            rio_buf_push((void (**))(&(str)), &(val), 1);
        }
        if (*(rio_stream)) {
            (rio_stream)++;
        } else {
            rio_error(rio_token.pos, "Unexpected end of file within string literal");
        }
    }
    int nul = '\0';
    rio_buf_push((void (**))(&(str)), &(nul), 1);
    rio_token.kind = RIO_TOKEN_STR;
    rio_token.str_val = str;
}

void rio_next_token(void) {
    repeat: ;
    rio_token.start = rio_stream;
    rio_token.mod = 0;
    rio_token.suffix = 0;
    switch (*(rio_stream)) {
    case ' ':
    case '\n':
    case '\r':
    case '\t':
    case '\v': {
        while (isspace(*(rio_stream))) {
            if ((*((rio_stream)++)) == ('\n')) {
                rio_line_start = rio_stream;
                (rio_token.pos.line)++;
            }
        }
        goto repeat;
        break;
    }
    case '\'': {
        rio_scan_char();
        break;
    }
    case '\"': {
        rio_scan_str();
        break;
    }
    case '.': {
        if (isdigit(rio_stream[1])) {
            rio_scan_float();
        } else if (((rio_stream[1]) == ('.')) && ((rio_stream[2]) == ('.'))) {
            rio_token.kind = RIO_TOKEN_ELLIPSIS;
            rio_stream += 3;
        } else {
            rio_token.kind = RIO_TOKEN_DOT;
            (rio_stream)++;
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
            while (isdigit(*(rio_stream))) {
                (rio_stream)++;
            }
            char c = *(rio_stream);
            rio_stream = rio_token.start;
            if (((c) == ('.')) || ((tolower(c)) == ('e'))) {
                rio_scan_float();
            } else {
                rio_scan_int();
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
        while ((isalnum(*(rio_stream))) || ((*(rio_stream)) == ('_'))) {
            (rio_stream)++;
        }
        rio_token.name = rio_str_intern_range(rio_token.start, rio_stream);
        rio_token.kind = (rio_is_keyword_name(rio_token.name) ? RIO_TOKEN_KEYWORD : RIO_TOKEN_NAME);
        break;
    }
    case '<': {
        rio_token.kind = RIO_TOKEN_LT;
        (rio_stream)++;
        if ((*(rio_stream)) == ('<')) {
            rio_token.kind = RIO_TOKEN_LSHIFT;
            (rio_stream)++;
            if ((*(rio_stream)) == ('=')) {
                rio_token.kind = RIO_TOKEN_LSHIFT_ASSIGN;
                (rio_stream)++;
            }
        } else if ((*(rio_stream)) == ('=')) {
            rio_token.kind = RIO_TOKEN_LTEQ;
            (rio_stream)++;
        }
        break;
    }
    case '>': {
        rio_token.kind = RIO_TOKEN_GT;
        (rio_stream)++;
        if ((*(rio_stream)) == ('>')) {
            rio_token.kind = RIO_TOKEN_RSHIFT;
            (rio_stream)++;
            if ((*(rio_stream)) == ('=')) {
                rio_token.kind = RIO_TOKEN_RSHIFT_ASSIGN;
                (rio_stream)++;
            }
        } else if ((*(rio_stream)) == ('=')) {
            rio_token.kind = RIO_TOKEN_GTEQ;
            (rio_stream)++;
        }
        break;
    }
    case '/': {
        rio_token.kind = RIO_TOKEN_DIV;
        (rio_stream)++;
        if ((*(rio_stream)) == ('=')) {
            rio_token.kind = RIO_TOKEN_DIV_ASSIGN;
            (rio_stream)++;
        } else if ((*(rio_stream)) == ('/')) {
            (rio_stream)++;
            while ((*(rio_stream)) && ((*(rio_stream)) != ('\n'))) {
                (rio_stream)++;
            }
            goto repeat;
        } else if ((*(rio_stream)) == ('*')) {
            (rio_stream)++;
            int level = 1;
            while ((*(rio_stream)) && ((level) > (0))) {
                if (((rio_stream[0]) == ('/')) && ((rio_stream[1]) == ('*'))) {
                    (level)++;
                    rio_stream += 2;
                } else if (((rio_stream[0]) == ('*')) && ((rio_stream[1]) == ('/'))) {
                    (level)--;
                    rio_stream += 2;
                } else {
                    if ((*(rio_stream)) == ('\n')) {
                        (rio_token.pos.line)++;
                    }
                    (rio_stream)++;
                }
            }
            goto repeat;
        }
        break;
    }
    case '\0': {
        rio_token.kind = RIO_TOKEN_EOF;
        (rio_stream)++;
        break;
    }
    case '(': {
        rio_token.kind = RIO_TOKEN_LPAREN;
        (rio_stream)++;
        break;
    }
    case ')': {
        rio_token.kind = RIO_TOKEN_RPAREN;
        (rio_stream)++;
        break;
    }
    case '{': {
        rio_token.kind = RIO_TOKEN_LBRACE;
        (rio_stream)++;
        break;
    }
    case '}': {
        rio_token.kind = RIO_TOKEN_RBRACE;
        (rio_stream)++;
        break;
    }
    case '[': {
        rio_token.kind = RIO_TOKEN_LBRACKET;
        (rio_stream)++;
        break;
    }
    case ']': {
        rio_token.kind = RIO_TOKEN_RBRACKET;
        (rio_stream)++;
        break;
    }
    case ',': {
        rio_token.kind = RIO_TOKEN_COMMA;
        (rio_stream)++;
        break;
    }
    case '@': {
        rio_token.kind = RIO_TOKEN_AT;
        (rio_stream)++;
        break;
    }
    case '#': {
        rio_token.kind = RIO_TOKEN_POUND;
        (rio_stream)++;
        break;
    }
    case '?': {
        rio_token.kind = RIO_TOKEN_QUESTION;
        (rio_stream)++;
        break;
    }
    case ';': {
        rio_token.kind = RIO_TOKEN_SEMICOLON;
        (rio_stream)++;
        break;
    }
    case '~': {
        rio_token.kind = RIO_TOKEN_NEG;
        (rio_stream)++;
        break;
    }
    case '!': {
        rio_token.kind = RIO_TOKEN_NOT;
        (rio_stream)++;
        if ((*(rio_stream)) == ('=')) {
            rio_token.kind = RIO_TOKEN_NOTEQ;
            (rio_stream)++;
        }
        break;
    }
    case ':': {
        rio_token.kind = RIO_TOKEN_COLON;
        (rio_stream)++;
        break;
    }
    case '=': {
        rio_token.kind = RIO_TOKEN_ASSIGN;
        (rio_stream)++;
        if ((*(rio_stream)) == ('=')) {
            rio_token.kind = RIO_TOKEN_EQ;
            (rio_stream)++;
        }
        break;
    }
    case '^': {
        rio_token.kind = RIO_TOKEN_XOR;
        (rio_stream)++;
        if ((*(rio_stream)) == ('=')) {
            rio_token.kind = RIO_TOKEN_XOR_ASSIGN;
            (rio_stream)++;
        }
        break;
    }
    case '*': {
        rio_token.kind = RIO_TOKEN_MUL;
        (rio_stream)++;
        if ((*(rio_stream)) == ('=')) {
            rio_token.kind = RIO_TOKEN_MUL_ASSIGN;
            (rio_stream)++;
        }
        break;
    }
    case '%': {
        rio_token.kind = RIO_TOKEN_MOD;
        (rio_stream)++;
        if ((*(rio_stream)) == ('=')) {
            rio_token.kind = RIO_TOKEN_MOD_ASSIGN;
            (rio_stream)++;
        }
        break;
    }
    case '+': {
        rio_token.kind = RIO_TOKEN_ADD;
        (rio_stream)++;
        if ((*(rio_stream)) == ('=')) {
            rio_token.kind = RIO_TOKEN_ADD_ASSIGN;
            (rio_stream)++;
        } else if ((*(rio_stream)) == ('+')) {
            rio_token.kind = RIO_TOKEN_INC;
            (rio_stream)++;
        }
        break;
    }
    case '-': {
        rio_token.kind = RIO_TOKEN_SUB;
        (rio_stream)++;
        if ((*(rio_stream)) == ('=')) {
            rio_token.kind = RIO_TOKEN_SUB_ASSIGN;
            (rio_stream)++;
        } else if ((*(rio_stream)) == ('-')) {
            rio_token.kind = RIO_TOKEN_DEC;
            (rio_stream)++;
        }
        break;
    }
    case '&': {
        rio_token.kind = RIO_TOKEN_AND;
        (rio_stream)++;
        if ((*(rio_stream)) == ('=')) {
            rio_token.kind = RIO_TOKEN_AND_ASSIGN;
            (rio_stream)++;
        } else if ((*(rio_stream)) == ('&')) {
            rio_token.kind = RIO_TOKEN_AND_AND;
            (rio_stream)++;
        }
        break;
    }
    case '|': {
        rio_token.kind = RIO_TOKEN_OR;
        (rio_stream)++;
        if ((*(rio_stream)) == ('=')) {
            rio_token.kind = RIO_TOKEN_OR_ASSIGN;
            (rio_stream)++;
        } else if ((*(rio_stream)) == ('|')) {
            rio_token.kind = RIO_TOKEN_OR_OR;
            (rio_stream)++;
        }
        break;
    }
    default: {
        rio_error(rio_token.pos, "Invalid \'%c\' token, skipping", *(rio_stream));
        (rio_stream)++;
        goto repeat;
        break;
    }
    }
    rio_token.end = rio_stream;
}

void rio_init_stream(char const ((*name)), char const ((*buf))) {
    rio_stream = buf;
    rio_line_start = rio_stream;
    rio_token.pos.name = (name ? name : (char const (*))("<string>"));
    rio_token.pos.line = 1;
    rio_next_token();
}

bool rio_is_token(rio_TokenKind kind) {
    return (rio_token.kind) == (kind);
}

bool rio_is_token_eof(void) {
    return (rio_token.kind) == (RIO_TOKEN_EOF);
}

bool rio_is_token_name(char const ((*name))) {
    return ((rio_token.kind) == (RIO_TOKEN_NAME)) && ((rio_token.name) == (name));
}

bool rio_is_keyword(char const ((*name))) {
    return (rio_is_token(RIO_TOKEN_KEYWORD)) && ((rio_token.name) == (name));
}

bool rio_match_keyword(char const ((*name))) {
    if (rio_is_keyword(name)) {
        rio_next_token();
        return true;
    } else {
        return false;
    }
}

bool rio_match_token(rio_TokenKind kind) {
    if (rio_is_token(kind)) {
        rio_next_token();
        return true;
    } else {
        return false;
    }
}

bool rio_expect_token(rio_TokenKind kind) {
    if (rio_is_token(kind)) {
        rio_next_token();
        return true;
    } else {
        rio_fatal_error(rio_token.pos, "Expected token %s, got %s", rio_token_kind_name(kind), rio_token_info());
        return false;
    }
}

bool rio_flag_verbose;
bool rio_flag_lazy;
bool rio_flag_nosourcemap;
bool rio_flag_notypeinfo;
bool rio_flag_fullgen;
void rio_path_normalize(char (*path)) {
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

void rio_path_copy(char (path[MAX_PATH]), char const ((*src))) {
    strncpy(path, src, MAX_PATH);
    path[(MAX_PATH) - (1)] = 0;
    rio_path_normalize(path);
}

void rio_path_join(char (path[MAX_PATH]), char const ((*src))) {
    char (*ptr) = (path) + (strlen(path));
    if (((ptr) != (path)) && ((ptr[-(1)]) == ('/'))) {
        (ptr)--;
    }
    if ((*(src)) == ('/')) {
        (src)++;
    }
    snprintf(ptr, ((path) + (MAX_PATH)) - (ptr), "/%s", src);
}

char (*rio_path_file(char (path[MAX_PATH]))) {
    rio_path_normalize(path);
    for (char (*ptr) = (path) + (strlen(path)); (ptr) != (path); (ptr)--) {
        if ((ptr[-(1)]) == ('/')) {
            return ptr;
        }
    }
    return path;
}

char (*rio_path_ext(char (path[MAX_PATH]))) {
    for (char (*ptr) = (path) + (strlen(path)); (ptr) != (path); (ptr)--) {
        if ((ptr[-(1)]) == ('.')) {
            return ptr;
        }
    }
    return path;
}

bool rio_dir_excluded(rio_DirListIter (*iter)) {
    return (iter->valid) && ((((strcmp(iter->name, ".")) == (0)) || ((strcmp(iter->name, "..")) == (0))));
}

bool rio_dir_list_subdir(rio_DirListIter (*iter)) {
    if ((!(iter->valid)) || (!(iter->is_dir))) {
        return false;
    }
    rio_DirListIter subdir_iter = {0};
    rio_path_join(iter->base, iter->name);
    rio_dir_list(&(subdir_iter), iter->base);
    rio_dir_list_free(iter);
    *(iter) = subdir_iter;
    return true;
}

char const ((*(*rio_dir_list_buf(char const ((*filespec)))))) {
    char const ((*(*buf))) = NULL;
    rio_DirListIter iter = {0};
    for (rio_dir_list(&(iter), filespec); iter.valid; rio_dir_list_next(&(iter))) {
        char const ((*name)) = strdup(iter.name);
        rio_buf_push((void (**))(&(buf)), &(name), sizeof(name));
    }
    return buf;
}

rio_FlagDef (*rio_flag_defs);
void rio_add_flag_bool(char const ((*name)), bool (*ptr), char const ((*help))) {
    rio_FlagDef def = {.kind = RIO_FLAG_BOOL, .name = name, .help = help, .ptr = {.b = ptr}};
    rio_buf_push((void (**))(&(rio_flag_defs)), &(def), sizeof(def));
}

void rio_add_flag_str(char const ((*name)), char const ((*(*ptr))), char const ((*arg_name)), char const ((*help))) {
    rio_FlagDef def = {.kind = RIO_FLAG_STR, .name = name, .help = help, .arg_name = arg_name, .ptr = {.s = ptr}};
    rio_buf_push((void (**))(&(rio_flag_defs)), &(def), sizeof(def));
}

void rio_add_flag_enum(char const ((*name)), int (*ptr), char const ((*help)), char const ((*(*options))), int num_options) {
    rio_FlagDef def = {.kind = RIO_FLAG_ENUM, .name = name, .help = help, .ptr = {.i = ptr}, .options = options, .num_options = num_options};
    rio_buf_push((void (**))(&(rio_flag_defs)), &(def), sizeof(def));
}

rio_FlagDef (*rio_get_flag_def(char const ((*name)))) {
    for (size_t i = 0; (i) < (rio_buf_len(rio_flag_defs)); (i)++) {
        if ((strcmp(rio_flag_defs[i].name, name)) == (0)) {
            return &(rio_flag_defs[i]);
        }
    }
    return NULL;
}

void rio_print_flags_usage(void) {
    printf("Flags:\n");
    for (size_t i = 0; (i) < (rio_buf_len(rio_flag_defs)); (i)++) {
        rio_FlagDef flag = rio_flag_defs[i];
        char (note[256]) = {0};
        char (format[256]) = {0};
        switch (flag.kind) {
        case RIO_FLAG_STR: {
            snprintf(format, sizeof(format), "%s <%s>", flag.name, (flag.arg_name ? flag.arg_name : (char const (*))("value")));
            if (*(flag.ptr.s)) {
                snprintf(note, sizeof(note), "(default: %s)", *(flag.ptr.s));
            }
            break;
        }
        case RIO_FLAG_ENUM: {
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
        case RIO_FLAG_BOOL:
        default: {
            snprintf(format, sizeof(format), "%s", flag.name);
            break;
        }
        }
        printf(" -%-32s %s%s\n", format, (flag.help ? flag.help : (char const (*))("")), note);
    }
}

char const ((*rio_parse_flags(int (*argc_ptr), char const ((*(*(*argv_ptr))))))) {
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
            rio_FlagDef (*flag) = rio_get_flag_def(name);
            if (!(flag)) {
                printf("Unknown flag %s\n", arg);
                continue;
            }
            switch (flag->kind) {
            case RIO_FLAG_BOOL: {
                *(flag->ptr.b) = true;
                break;
            }
            case RIO_FLAG_STR: {
                if (((i) + (1)) < (argc)) {
                    (i)++;
                    *(flag->ptr.s) = argv[i];
                } else {
                    printf("No value argument after -%s\n", arg);
                }
                break;
            }
            case RIO_FLAG_ENUM: {
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
    return rio_path_file(strdup(argv[0]));
}

void rio_path_absolute(char (path[MAX_PATH])) {
    char (rel_path[MAX_PATH]) = {0};
    rio_path_copy(rel_path, path);
    realpath(rel_path, path);
}

void rio_dir_list_free(rio_DirListIter (*iter)) {
    if (iter->valid) {
        iter->valid = false;
        iter->error = false;
        closedir(iter->handle);
    }
}

void rio_dir_list_next(rio_DirListIter (*iter)) {
    if (!(iter->valid)) {
        return;
    }
    do {
        dirent (*entry) = readdir(iter->handle);
        if (!(entry)) {
            rio_dir_list_free(iter);
            return;
        }
        rio_path_copy(iter->name, entry->d_name);
        iter->is_dir = (entry->d_type) & (DT_DIR);
    } while (rio_dir_excluded(iter));
}

void rio_dir_list(rio_DirListIter (*iter), char const ((*path))) {
    memset(iter, 0, sizeof(*(iter)));
    void (*dir) = opendir(path);
    if (!(dir)) {
        iter->valid = false;
        iter->error = true;
        return;
    }
    iter->handle = dir;
    rio_path_copy(iter->base, path);
    iter->valid = true;
    rio_dir_list_next(iter);
}

rio_Typespec (*rio_parse_type_func_param(void)) {
    rio_Typespec (*type) = rio_parse_type();
    if (rio_match_token(RIO_TOKEN_COLON)) {
        if ((type->kind) != (RIO_TYPESPEC_NAME)) {
            rio_error(rio_token.pos, "Colons in parameters of fn types must be preceded by names.");
        }
        type = rio_parse_type();
    }
    return type;
}

rio_Typespec (*rio_parse_type_func(void)) {
    rio_SrcPos pos = rio_token.pos;
    rio_Typespec (*(*args)) = NULL;
    bool has_varargs = false;
    rio_expect_token(RIO_TOKEN_LPAREN);
    while (!(rio_is_token(RIO_TOKEN_RPAREN))) {
        if (rio_match_token(RIO_TOKEN_ELLIPSIS)) {
            if (has_varargs) {
                rio_error(rio_token.pos, "Multiple ellipsis instances in function type");
            }
            has_varargs = true;
        } else {
            if (has_varargs) {
                rio_error(rio_token.pos, "Ellipsis must be last parameter in function type");
            }
            rio_Typespec (*param) = rio_parse_type_func_param();
            rio_buf_push((void (**))(&(args)), &(param), sizeof(param));
        }
        if (!(rio_match_token(RIO_TOKEN_COMMA))) {
            break;
        }
    }
    rio_expect_token(RIO_TOKEN_RPAREN);
    rio_Typespec (*ret) = NULL;
    if (rio_match_token(RIO_TOKEN_COLON)) {
        ret = rio_parse_type();
    }
    return rio_new_typespec_func(pos, args, rio_buf_len(args), ret, has_varargs);
}

rio_Typespec (*rio_parse_type_base(void)) {
    if (rio_is_token(RIO_TOKEN_NAME)) {
        rio_SrcPos pos = rio_token.pos;
        char const ((*name)) = rio_token.name;
        rio_next_token();
        return rio_new_typespec_name(pos, name);
    } else if (rio_match_keyword(rio_fn_keyword)) {
        return rio_parse_type_func();
    } else if (rio_match_token(RIO_TOKEN_LPAREN)) {
        rio_Typespec (*type) = rio_parse_type();
        rio_expect_token(RIO_TOKEN_RPAREN);
        return type;
    } else {
        rio_fatal_error(rio_token.pos, "Unexpected token %s in type", rio_token_info());
        return NULL;
    }
}

rio_Typespec (*rio_parse_type(void)) {
    rio_Typespec (*type) = rio_parse_type_base();
    rio_SrcPos pos = rio_token.pos;
    while (((rio_is_token(RIO_TOKEN_LBRACKET)) || (rio_is_token(RIO_TOKEN_MUL))) || (rio_is_keyword(rio_const_keyword))) {
        if (rio_match_token(RIO_TOKEN_LBRACKET)) {
            rio_Expr (*size) = NULL;
            if (!(rio_is_token(RIO_TOKEN_RBRACKET))) {
                size = rio_parse_expr();
            }
            rio_expect_token(RIO_TOKEN_RBRACKET);
            type = rio_new_typespec_array(pos, type, size);
        } else if (rio_match_keyword(rio_const_keyword)) {
            type = rio_new_typespec_const(pos, type);
        } else {
            assert(rio_is_token(RIO_TOKEN_MUL));
            rio_next_token();
            type = rio_new_typespec_ptr(pos, type);
        }
    }
    return type;
}

rio_CompoundField rio_parse_expr_compound_field(void) {
    rio_SrcPos pos = rio_token.pos;
    if (rio_match_token(RIO_TOKEN_LBRACKET)) {
        rio_Expr (*index) = rio_parse_expr();
        rio_expect_token(RIO_TOKEN_RBRACKET);
        rio_expect_token(RIO_TOKEN_ASSIGN);
        return (rio_CompoundField){RIO_FIELD_INDEX, pos, rio_parse_expr(), .index = index};
    } else {
        rio_Expr (*expr) = rio_parse_expr();
        if (rio_match_token(RIO_TOKEN_ASSIGN)) {
            if ((expr->kind) != (RIO_EXPR_NAME)) {
                rio_fatal_error(rio_token.pos, "Named initializer in compound literal must be preceded by field name");
            }
            return (rio_CompoundField){RIO_FIELD_NAME, pos, rio_parse_expr(), .name = expr->name};
        } else {
            return (rio_CompoundField){RIO_FIELD_DEFAULT, pos, expr};
        }
    }
}

rio_Expr (*rio_parse_expr_compound(rio_Typespec (*type))) {
    rio_SrcPos pos = rio_token.pos;
    rio_expect_token(RIO_TOKEN_LBRACE);
    rio_CompoundField (*fields) = NULL;
    while (!(rio_is_token(RIO_TOKEN_RBRACE))) {
        rio_CompoundField field = rio_parse_expr_compound_field();
        rio_buf_push((void (**))(&(fields)), &(field), sizeof(field));
        if (!(rio_match_token(RIO_TOKEN_COMMA))) {
            break;
        }
    }
    rio_expect_token(RIO_TOKEN_RBRACE);
    return rio_new_expr_compound(pos, type, fields, rio_buf_len(fields));
}

rio_Expr (*rio_parse_expr_operand(void)) {
    rio_SrcPos pos = rio_token.pos;
    if (rio_is_token(RIO_TOKEN_INT)) {
        ullong val = rio_token.int_val;
        rio_TokenMod mod = rio_token.mod;
        rio_TokenSuffix suffix = rio_token.suffix;
        rio_next_token();
        return rio_new_expr_int(pos, val, mod, suffix);
    } else if (rio_is_token(RIO_TOKEN_FLOAT)) {
        char const ((*start)) = rio_token.start;
        char const ((*end)) = rio_token.end;
        double val = rio_token.float_val;
        rio_TokenSuffix suffix = rio_token.suffix;
        rio_next_token();
        return rio_new_expr_float(pos, start, end, val, suffix);
    } else if (rio_is_token(RIO_TOKEN_STR)) {
        char const ((*val)) = rio_token.str_val;
        rio_TokenMod mod = rio_token.mod;
        rio_next_token();
        return rio_new_expr_str(pos, val, mod);
    } else if (rio_is_token(RIO_TOKEN_NAME)) {
        char const ((*name)) = rio_token.name;
        rio_next_token();
        if (rio_is_token(RIO_TOKEN_LBRACE)) {
            return rio_parse_expr_compound(rio_new_typespec_name(pos, name));
        } else {
            return rio_new_expr_name(pos, name);
        }
    } else if (rio_match_keyword(rio_sizeof_keyword)) {
        rio_expect_token(RIO_TOKEN_LPAREN);
        if (rio_match_token(RIO_TOKEN_COLON)) {
            rio_Typespec (*type) = rio_parse_type();
            rio_expect_token(RIO_TOKEN_RPAREN);
            return rio_new_expr_sizeof_type(pos, type);
        } else {
            rio_Expr (*expr) = rio_parse_expr();
            rio_expect_token(RIO_TOKEN_RPAREN);
            return rio_new_expr_sizeof_expr(pos, expr);
        }
    } else if (rio_match_keyword(rio_alignof_keyword)) {
        rio_expect_token(RIO_TOKEN_LPAREN);
        if (rio_match_token(RIO_TOKEN_COLON)) {
            rio_Typespec (*type) = rio_parse_type();
            rio_expect_token(RIO_TOKEN_RPAREN);
            return rio_new_expr_alignof_type(pos, type);
        } else {
            rio_Expr (*expr) = rio_parse_expr();
            rio_expect_token(RIO_TOKEN_RPAREN);
            return rio_new_expr_alignof_expr(pos, expr);
        }
    } else if (rio_match_keyword(rio_typeof_keyword)) {
        rio_expect_token(RIO_TOKEN_LPAREN);
        if (rio_match_token(RIO_TOKEN_COLON)) {
            rio_Typespec (*type) = rio_parse_type();
            rio_expect_token(RIO_TOKEN_RPAREN);
            return rio_new_expr_typeof_type(pos, type);
        } else {
            rio_Expr (*expr) = rio_parse_expr();
            rio_expect_token(RIO_TOKEN_RPAREN);
            return rio_new_expr_typeof_expr(pos, expr);
        }
    } else if (rio_match_keyword(rio_offsetof_keyword)) {
        rio_expect_token(RIO_TOKEN_LPAREN);
        rio_Typespec (*type) = rio_parse_type();
        rio_expect_token(RIO_TOKEN_COMMA);
        char const ((*name)) = rio_parse_name();
        rio_expect_token(RIO_TOKEN_RPAREN);
        return rio_new_expr_offsetof(pos, type, name);
    } else if (rio_is_token(RIO_TOKEN_LBRACE)) {
        return rio_parse_expr_compound(NULL);
    } else if (rio_match_token(RIO_TOKEN_LPAREN)) {
        if (rio_match_token(RIO_TOKEN_COLON)) {
            rio_Typespec (*type) = rio_parse_type();
            rio_expect_token(RIO_TOKEN_RPAREN);
            if (rio_is_token(RIO_TOKEN_LBRACE)) {
                return rio_parse_expr_compound(type);
            } else {
                return rio_new_expr_cast(pos, type, rio_parse_expr_unary());
            }
        } else {
            rio_Expr (*expr) = rio_parse_expr();
            rio_expect_token(RIO_TOKEN_RPAREN);
            return rio_new_expr_paren(pos, expr);
        }
    } else {
        rio_fatal_error(rio_token.pos, "Unexpected token %s in expression", rio_token_info());
        return NULL;
    }
}

rio_Expr (*rio_parse_expr_base(void)) {
    rio_Expr (*expr) = rio_parse_expr_operand();
    while (((((rio_is_token(RIO_TOKEN_LPAREN)) || (rio_is_token(RIO_TOKEN_LBRACKET))) || (rio_is_token(RIO_TOKEN_DOT))) || (rio_is_token(RIO_TOKEN_INC))) || (rio_is_token(RIO_TOKEN_DEC))) {
        rio_SrcPos pos = rio_token.pos;
        if (rio_match_token(RIO_TOKEN_LPAREN)) {
            rio_Expr (*(*args)) = NULL;
            while (!(rio_is_token(RIO_TOKEN_RPAREN))) {
                rio_Expr (*arg) = rio_parse_expr();
                rio_buf_push((void (**))(&(args)), &(arg), sizeof(arg));
                if (!(rio_match_token(RIO_TOKEN_COMMA))) {
                    break;
                }
            }
            rio_expect_token(RIO_TOKEN_RPAREN);
            expr = rio_new_expr_call(pos, expr, args, rio_buf_len(args));
        } else if (rio_match_token(RIO_TOKEN_LBRACKET)) {
            rio_Expr (*index) = rio_parse_expr();
            rio_expect_token(RIO_TOKEN_RBRACKET);
            expr = rio_new_expr_index(pos, expr, index);
        } else if (rio_is_token(RIO_TOKEN_DOT)) {
            rio_next_token();
            char const ((*field)) = rio_token.name;
            rio_expect_token(RIO_TOKEN_NAME);
            expr = rio_new_expr_field(pos, expr, field);
        } else {
            assert((rio_is_token(RIO_TOKEN_INC)) || (rio_is_token(RIO_TOKEN_DEC)));
            rio_TokenKind op = rio_token.kind;
            rio_next_token();
            expr = rio_new_expr_modify(pos, op, true, expr);
        }
    }
    return expr;
}

bool rio_is_unary_op(void) {
    return (((((((rio_is_token(RIO_TOKEN_ADD)) || (rio_is_token(RIO_TOKEN_SUB))) || (rio_is_token(RIO_TOKEN_MUL))) || (rio_is_token(RIO_TOKEN_AND))) || (rio_is_token(RIO_TOKEN_NEG))) || (rio_is_token(RIO_TOKEN_NOT))) || (rio_is_token(RIO_TOKEN_INC))) || (rio_is_token(RIO_TOKEN_DEC));
}

rio_Expr (*rio_parse_expr_unary(void)) {
    if (rio_is_unary_op()) {
        rio_SrcPos pos = rio_token.pos;
        rio_TokenKind op = rio_token.kind;
        rio_next_token();
        if (((op) == (RIO_TOKEN_INC)) || ((op) == (RIO_TOKEN_DEC))) {
            return rio_new_expr_modify(pos, op, false, rio_parse_expr_unary());
        } else {
            return rio_new_expr_unary(pos, op, rio_parse_expr_unary());
        }
    } else {
        return rio_parse_expr_base();
    }
}

bool rio_is_mul_op(void) {
    return ((RIO_TOKEN_FIRST_MUL) <= (rio_token.kind)) && ((rio_token.kind) <= (RIO_TOKEN_LAST_MUL));
}

rio_Expr (*rio_parse_expr_mul(void)) {
    rio_Expr (*expr) = rio_parse_expr_unary();
    while (rio_is_mul_op()) {
        rio_SrcPos pos = rio_token.pos;
        rio_TokenKind op = rio_token.kind;
        rio_next_token();
        expr = rio_new_expr_binary(pos, op, expr, rio_parse_expr_unary());
    }
    return expr;
}

bool rio_is_add_op(void) {
    return ((RIO_TOKEN_FIRST_ADD) <= (rio_token.kind)) && ((rio_token.kind) <= (RIO_TOKEN_LAST_ADD));
}

rio_Expr (*rio_parse_expr_add(void)) {
    rio_Expr (*expr) = rio_parse_expr_mul();
    while (rio_is_add_op()) {
        rio_SrcPos pos = rio_token.pos;
        rio_TokenKind op = rio_token.kind;
        rio_next_token();
        expr = rio_new_expr_binary(pos, op, expr, rio_parse_expr_mul());
    }
    return expr;
}

bool rio_is_cmp_op(void) {
    return ((RIO_TOKEN_FIRST_CMP) <= (rio_token.kind)) && ((rio_token.kind) <= (RIO_TOKEN_LAST_CMP));
}

rio_Expr (*rio_parse_expr_cmp(void)) {
    rio_Expr (*expr) = rio_parse_expr_add();
    while (rio_is_cmp_op()) {
        rio_SrcPos pos = rio_token.pos;
        rio_TokenKind op = rio_token.kind;
        rio_next_token();
        expr = rio_new_expr_binary(pos, op, expr, rio_parse_expr_add());
    }
    return expr;
}

rio_Expr (*rio_parse_expr_and(void)) {
    rio_Expr (*expr) = rio_parse_expr_cmp();
    while (rio_match_token(RIO_TOKEN_AND_AND)) {
        rio_SrcPos pos = rio_token.pos;
        expr = rio_new_expr_binary(pos, RIO_TOKEN_AND_AND, expr, rio_parse_expr_cmp());
    }
    return expr;
}

rio_Expr (*rio_parse_expr_or(void)) {
    rio_Expr (*expr) = rio_parse_expr_and();
    while (rio_match_token(RIO_TOKEN_OR_OR)) {
        rio_SrcPos pos = rio_token.pos;
        expr = rio_new_expr_binary(pos, RIO_TOKEN_OR_OR, expr, rio_parse_expr_and());
    }
    return expr;
}

rio_Expr (*rio_parse_expr_ternary(void)) {
    rio_SrcPos pos = rio_token.pos;
    rio_Expr (*expr) = rio_parse_expr_or();
    if (rio_match_token(RIO_TOKEN_QUESTION)) {
        rio_Expr (*then_expr) = rio_parse_expr_ternary();
        rio_expect_token(RIO_TOKEN_COLON);
        rio_Expr (*else_expr) = rio_parse_expr_ternary();
        expr = rio_new_expr_ternary(pos, expr, then_expr, else_expr);
    }
    return expr;
}

rio_Expr (*rio_parse_expr(void)) {
    return rio_parse_expr_ternary();
}

rio_Expr (*rio_parse_paren_expr(void)) {
    rio_expect_token(RIO_TOKEN_LPAREN);
    rio_Expr (*expr) = rio_parse_expr();
    rio_expect_token(RIO_TOKEN_RPAREN);
    return expr;
}

rio_StmtList rio_parse_stmt_block(void) {
    rio_SrcPos pos = rio_token.pos;
    rio_expect_token(RIO_TOKEN_LBRACE);
    rio_Stmt (*(*stmts)) = NULL;
    while ((!(rio_is_token_eof())) && (!(rio_is_token(RIO_TOKEN_RBRACE)))) {
        rio_Stmt (*stmt) = rio_parse_stmt();
        rio_buf_push((void (**))(&(stmts)), &(stmt), sizeof(stmt));
    }
    rio_expect_token(RIO_TOKEN_RBRACE);
    return rio_new_stmt_list(pos, stmts, rio_buf_len(stmts));
}

rio_Stmt (*rio_parse_stmt_if(rio_SrcPos pos)) {
    rio_expect_token(RIO_TOKEN_LPAREN);
    rio_Expr (*cond) = {0};
    rio_Stmt (*init) = {0};
    if (rio_match_keyword(rio_let_keyword)) {
        init = rio_parse_let_stmt(pos);
        if (rio_match_token(RIO_TOKEN_SEMICOLON)) {
            cond = rio_parse_expr();
        }
    } else {
        cond = rio_parse_expr();
    }
    rio_expect_token(RIO_TOKEN_RPAREN);
    rio_StmtList then_block = rio_parse_stmt_block();
    rio_StmtList else_block = {{NULL, 0}, NULL, 0};
    rio_ElseIf (*elseifs) = {0};
    while (rio_match_keyword(rio_else_keyword)) {
        if (!(rio_match_keyword(rio_if_keyword))) {
            else_block = rio_parse_stmt_block();
            break;
        }
        rio_Expr (*elseif_cond) = rio_parse_paren_expr();
        rio_StmtList elseif_block = rio_parse_stmt_block();
        rio_ElseIf elseif = {elseif_cond, elseif_block};
        rio_buf_push((void (**))(&(elseifs)), &(elseif), sizeof(elseif));
    }
    return rio_new_stmt_if(pos, init, cond, then_block, elseifs, rio_buf_len(elseifs), else_block);
}

rio_Stmt (*rio_parse_stmt_while(rio_SrcPos pos)) {
    rio_Expr (*cond) = rio_parse_paren_expr();
    return rio_new_stmt_while(pos, cond, rio_parse_stmt_block());
}

rio_Stmt (*rio_parse_stmt_do_while(rio_SrcPos pos)) {
    rio_StmtList block = rio_parse_stmt_block();
    if (!(rio_match_keyword(rio_while_keyword))) {
        rio_fatal_error(rio_token.pos, "Expected \'while\' after \'do\' block");
        return NULL;
    }
    rio_Stmt (*stmt) = rio_new_stmt_do_while(pos, rio_parse_paren_expr(), block);
    rio_expect_token(RIO_TOKEN_SEMICOLON);
    return stmt;
}

bool rio_is_assign_op(void) {
    return ((RIO_TOKEN_FIRST_ASSIGN) <= (rio_token.kind)) && ((rio_token.kind) <= (RIO_TOKEN_LAST_ASSIGN));
}

rio_Stmt (*rio_parse_let_stmt(rio_SrcPos pos)) {
    bool is_mut = rio_match_keyword(rio_mut_keyword);
    char const ((*name)) = rio_parse_name();
    if (rio_match_token(RIO_TOKEN_ASSIGN)) {
        return rio_new_stmt_init(pos, name, is_mut, NULL, rio_parse_expr());
    } else if (rio_match_token(RIO_TOKEN_COLON)) {
        rio_Typespec (*type) = rio_parse_type();
        rio_Expr (*expr) = {0};
        if (rio_match_token(RIO_TOKEN_ASSIGN)) {
            expr = rio_parse_expr();
        } else {
            is_mut = true;
        }
        return rio_new_stmt_init(pos, name, is_mut, type, expr);
    } else {
        rio_fatal_error(rio_token.pos, "Expected : or = after let, got %s", rio_token_info());
        return NULL;
    }
}

rio_Stmt (*rio_parse_simple_stmt(void)) {
    rio_SrcPos pos = rio_token.pos;
    rio_Stmt (*stmt) = {0};
    if (rio_match_keyword(rio_let_keyword)) {
        stmt = rio_parse_let_stmt(pos);
    } else {
        rio_Expr (*expr) = rio_parse_expr();
        if (rio_is_assign_op()) {
            rio_TokenKind op = rio_token.kind;
            rio_next_token();
            stmt = rio_new_stmt_assign(pos, op, expr, rio_parse_expr());
        } else {
            stmt = rio_new_stmt_expr(pos, expr);
        }
    }
    return stmt;
}

rio_Stmt (*rio_parse_stmt_for(rio_SrcPos pos)) {
    rio_expect_token(RIO_TOKEN_LPAREN);
    rio_Stmt (*init) = NULL;
    if (!(rio_is_token(RIO_TOKEN_SEMICOLON))) {
        init = rio_parse_simple_stmt();
    }
    rio_expect_token(RIO_TOKEN_SEMICOLON);
    rio_Expr (*cond) = NULL;
    if (!(rio_is_token(RIO_TOKEN_SEMICOLON))) {
        cond = rio_parse_expr();
    }
    rio_Stmt (*next) = NULL;
    if (rio_match_token(RIO_TOKEN_SEMICOLON)) {
        if (!(rio_is_token(RIO_TOKEN_RPAREN))) {
            next = rio_parse_simple_stmt();
            if ((next->kind) == (RIO_STMT_INIT)) {
                rio_error(rio_token.pos, "Init statements not allowed in for-statement\'s next clause");
            }
        }
    }
    rio_expect_token(RIO_TOKEN_RPAREN);
    return rio_new_stmt_for(pos, init, cond, next, rio_parse_stmt_block());
}

rio_SwitchCasePattern rio_parse_switch_case_pattern(void) {
    rio_Expr (*start) = rio_parse_expr();
    rio_Expr (*end) = NULL;
    if (rio_match_token(RIO_TOKEN_ELLIPSIS)) {
        end = rio_parse_expr();
    }
    return (rio_SwitchCasePattern){start, end};
}

rio_SwitchCase rio_parse_stmt_switch_case(void) {
    rio_SwitchCasePattern (*patterns) = NULL;
    bool is_default = false;
    bool is_first_case = true;
    while ((rio_is_keyword(rio_case_keyword)) || (rio_is_keyword(rio_default_keyword))) {
        if (rio_match_keyword(rio_case_keyword)) {
            if (!(is_first_case)) {
                rio_error(rio_token.pos, "Use comma-separated expressions to match multiple values with one case label");
                is_first_case = false;
            }
            rio_SwitchCasePattern pattern = rio_parse_switch_case_pattern();
            rio_buf_push((void (**))(&(patterns)), &(pattern), sizeof(pattern));
            while (rio_match_token(RIO_TOKEN_COMMA)) {
                pattern = rio_parse_switch_case_pattern();
                rio_buf_push((void (**))(&(patterns)), &(pattern), sizeof(pattern));
            }
        } else {
            assert(rio_is_keyword(rio_default_keyword));
            rio_next_token();
            if (is_default) {
                rio_error(rio_token.pos, "Duplicate default labels in same switch clause");
            }
            is_default = true;
        }
        rio_expect_token(RIO_TOKEN_COLON);
    }
    rio_SrcPos pos = rio_token.pos;
    rio_Stmt (*(*stmts)) = {0};
    while ((((!(rio_is_token_eof())) && (!(rio_is_token(RIO_TOKEN_RBRACE)))) && (!(rio_is_keyword(rio_case_keyword)))) && (!(rio_is_keyword(rio_default_keyword)))) {
        rio_Stmt (*stmt) = rio_parse_stmt();
        rio_buf_push((void (**))(&(stmts)), &(stmt), sizeof(stmt));
    }
    return (rio_SwitchCase){patterns, rio_buf_len(patterns), is_default, rio_new_stmt_list(pos, stmts, rio_buf_len(stmts))};
}

rio_Stmt (*rio_parse_stmt_switch(rio_SrcPos pos)) {
    rio_Expr (*expr) = rio_parse_paren_expr();
    rio_SwitchCase (*cases) = {0};
    rio_expect_token(RIO_TOKEN_LBRACE);
    while ((!(rio_is_token_eof())) && (!(rio_is_token(RIO_TOKEN_RBRACE)))) {
        rio_SwitchCase case_stmt = rio_parse_stmt_switch_case();
        rio_buf_push((void (**))(&(cases)), &(case_stmt), sizeof(case_stmt));
    }
    rio_expect_token(RIO_TOKEN_RBRACE);
    return rio_new_stmt_switch(pos, expr, cases, rio_buf_len(cases));
}

rio_Stmt (*rio_parse_stmt(void)) {
    rio_Notes notes = rio_parse_notes();
    rio_SrcPos pos = rio_token.pos;
    rio_Stmt (*stmt) = {0};
    if (rio_match_keyword(rio_if_keyword)) {
        stmt = rio_parse_stmt_if(pos);
    } else if (rio_match_keyword(rio_while_keyword)) {
        stmt = rio_parse_stmt_while(pos);
    } else if (rio_match_keyword(rio_do_keyword)) {
        stmt = rio_parse_stmt_do_while(pos);
    } else if (rio_match_keyword(rio_for_keyword)) {
        stmt = rio_parse_stmt_for(pos);
    } else if (rio_match_keyword(rio_switch_keyword)) {
        stmt = rio_parse_stmt_switch(pos);
    } else if (rio_is_token(RIO_TOKEN_LBRACE)) {
        stmt = rio_new_stmt_block(pos, rio_parse_stmt_block());
    } else if (rio_match_keyword(rio_break_keyword)) {
        rio_expect_token(RIO_TOKEN_SEMICOLON);
        stmt = rio_new_stmt_break(pos);
    } else if (rio_match_keyword(rio_continue_keyword)) {
        rio_expect_token(RIO_TOKEN_SEMICOLON);
        stmt = rio_new_stmt_continue(pos);
    } else if (rio_match_keyword(rio_return_keyword)) {
        rio_Expr (*expr) = {0};
        if (!(rio_is_token(RIO_TOKEN_SEMICOLON))) {
            expr = rio_parse_expr();
        }
        rio_expect_token(RIO_TOKEN_SEMICOLON);
        stmt = rio_new_stmt_return(pos, expr);
    } else if (rio_match_token(RIO_TOKEN_POUND)) {
        rio_Note note = rio_parse_note();
        rio_expect_token(RIO_TOKEN_SEMICOLON);
        stmt = rio_new_stmt_note(pos, note);
    } else if (rio_match_token(RIO_TOKEN_COLON)) {
        stmt = rio_new_stmt_label(pos, rio_parse_name());
    } else if (rio_match_keyword(rio_goto_keyword)) {
        stmt = rio_new_stmt_goto(pos, rio_parse_name());
        rio_expect_token(RIO_TOKEN_SEMICOLON);
    } else {
        stmt = rio_parse_simple_stmt();
        rio_expect_token(RIO_TOKEN_SEMICOLON);
    }
    stmt->notes = notes;
    return stmt;
}

char const ((*rio_parse_name(void))) {
    char const ((*name)) = rio_token.name;
    rio_expect_token(RIO_TOKEN_NAME);
    return name;
}

rio_EnumItem rio_parse_decl_enum_item(void) {
    rio_SrcPos pos = rio_token.pos;
    char const ((*name)) = rio_parse_name();
    rio_Expr (*init) = NULL;
    if (rio_match_token(RIO_TOKEN_ASSIGN)) {
        init = rio_parse_expr();
    }
    return (rio_EnumItem){pos, name, init};
}

rio_Decl (*rio_parse_decl_enum(rio_SrcPos pos)) {
    char const ((*name)) = NULL;
    if (rio_is_token(RIO_TOKEN_NAME)) {
        name = rio_parse_name();
    }
    rio_Typespec (*type) = NULL;
    if (rio_match_token(RIO_TOKEN_ASSIGN)) {
        type = rio_parse_type();
    }
    rio_expect_token(RIO_TOKEN_LBRACE);
    rio_EnumItem (*items) = NULL;
    while (!(rio_is_token(RIO_TOKEN_RBRACE))) {
        rio_EnumItem item = rio_parse_decl_enum_item();
        rio_buf_push((void (**))(&(items)), &(item), sizeof(item));
        if (!(rio_match_token(RIO_TOKEN_COMMA))) {
            break;
        }
    }
    rio_expect_token(RIO_TOKEN_RBRACE);
    return rio_new_decl_enum(pos, name, type, items, rio_buf_len(items));
}

rio_AggregateItem rio_parse_decl_aggregate_item(void) {
    rio_SrcPos pos = rio_token.pos;
    if (rio_match_keyword(rio_struct_keyword)) {
        return (rio_AggregateItem){.pos = pos, .kind = RIO_AGGREGATE_ITEM_SUBAGGREGATE, .subaggregate = rio_parse_aggregate(RIO_AGGREGATE_STRUCT)};
    } else if (rio_match_keyword(rio_union_keyword)) {
        return (rio_AggregateItem){.pos = pos, .kind = RIO_AGGREGATE_ITEM_SUBAGGREGATE, .subaggregate = rio_parse_aggregate(RIO_AGGREGATE_UNION)};
    } else {
        char const ((*(*names))) = NULL;
        char const ((*name)) = rio_parse_name();
        rio_buf_push((void (**))(&(names)), &(name), sizeof(name));
        while (rio_match_token(RIO_TOKEN_COMMA)) {
            name = rio_parse_name();
            rio_buf_push((void (**))(&(names)), &(name), sizeof(name));
        }
        rio_expect_token(RIO_TOKEN_COLON);
        rio_Typespec (*type) = rio_parse_type();
        rio_expect_token(RIO_TOKEN_SEMICOLON);
        return (rio_AggregateItem){.pos = pos, .kind = RIO_AGGREGATE_ITEM_FIELD, .names = names, .num_names = rio_buf_len(names), .type = type};
    }
}

rio_Aggregate (*rio_parse_aggregate(rio_AggregateKind kind)) {
    rio_SrcPos pos = rio_token.pos;
    rio_expect_token(RIO_TOKEN_LBRACE);
    rio_AggregateItem (*items) = NULL;
    while ((!(rio_is_token_eof())) && (!(rio_is_token(RIO_TOKEN_RBRACE)))) {
        rio_AggregateItem item = rio_parse_decl_aggregate_item();
        rio_buf_push((void (**))(&(items)), &(item), sizeof(item));
    }
    rio_expect_token(RIO_TOKEN_RBRACE);
    return rio_new_aggregate(pos, kind, items, rio_buf_len(items));
}

rio_Decl (*rio_parse_decl_aggregate(rio_SrcPos pos, rio_DeclKind kind)) {
    assert(((kind) == (RIO_DECL_STRUCT)) || ((kind) == (RIO_DECL_UNION)));
    char const ((*name)) = rio_parse_name();
    rio_AggregateKind aggregate_kind = ((kind) == (RIO_DECL_STRUCT) ? RIO_AGGREGATE_STRUCT : RIO_AGGREGATE_UNION);
    if (rio_match_token(RIO_TOKEN_SEMICOLON)) {
        rio_Decl (*decl) = rio_new_decl_aggregate(pos, kind, name, rio_new_aggregate(pos, aggregate_kind, NULL, 0));
        decl->is_incomplete = true;
        return decl;
    } else {
        return rio_new_decl_aggregate(pos, kind, name, rio_parse_aggregate(aggregate_kind));
    }
}

rio_Decl (*rio_parse_decl_var(rio_SrcPos pos)) {
    char const ((*name)) = rio_parse_name();
    if (rio_match_token(RIO_TOKEN_ASSIGN)) {
        rio_Expr (*expr) = rio_parse_expr();
        rio_expect_token(RIO_TOKEN_SEMICOLON);
        return rio_new_decl_var(pos, name, NULL, expr);
    } else if (rio_match_token(RIO_TOKEN_COLON)) {
        rio_Typespec (*type) = rio_parse_type();
        rio_Expr (*expr) = NULL;
        if (rio_match_token(RIO_TOKEN_ASSIGN)) {
            expr = rio_parse_expr();
        }
        rio_expect_token(RIO_TOKEN_SEMICOLON);
        return rio_new_decl_var(pos, name, type, expr);
    } else {
        rio_fatal_error(rio_token.pos, "Expected : or = after let, got %s", rio_token_info());
        return NULL;
    }
}

rio_Decl (*rio_parse_decl_const(rio_SrcPos pos)) {
    char const ((*name)) = rio_parse_name();
    rio_Typespec (*type) = NULL;
    if (rio_match_token(RIO_TOKEN_COLON)) {
        type = rio_parse_type();
    }
    rio_expect_token(RIO_TOKEN_ASSIGN);
    rio_Expr (*expr) = rio_parse_expr();
    rio_expect_token(RIO_TOKEN_SEMICOLON);
    return rio_new_decl_const(pos, name, type, expr);
}

rio_Decl (*rio_parse_decl_typedef(rio_SrcPos pos)) {
    char const ((*name)) = rio_parse_name();
    rio_expect_token(RIO_TOKEN_ASSIGN);
    rio_Typespec (*type) = rio_parse_type();
    rio_expect_token(RIO_TOKEN_SEMICOLON);
    return rio_new_decl_typedef(pos, name, type);
}

rio_FuncParam rio_parse_decl_func_param(void) {
    rio_SrcPos pos = rio_token.pos;
    char const ((*name)) = rio_parse_name();
    rio_expect_token(RIO_TOKEN_COLON);
    rio_Typespec (*type) = rio_parse_type();
    return (rio_FuncParam){pos, name, type};
}

rio_Decl (*rio_parse_decl_func(rio_SrcPos pos)) {
    char const ((*name)) = rio_parse_name();
    rio_expect_token(RIO_TOKEN_LPAREN);
    rio_FuncParam (*params) = NULL;
    bool has_varargs = false;
    while (!(rio_is_token(RIO_TOKEN_RPAREN))) {
        if (rio_match_token(RIO_TOKEN_ELLIPSIS)) {
            if (has_varargs) {
                rio_error(rio_token.pos, "Multiple ellipsis in function declaration");
            }
            has_varargs = true;
        } else {
            if (has_varargs) {
                rio_error(rio_token.pos, "Ellipsis must be last parameter in function declaration");
            }
            rio_FuncParam param = rio_parse_decl_func_param();
            rio_buf_push((void (**))(&(params)), &(param), sizeof(param));
        }
        if (!(rio_match_token(RIO_TOKEN_COMMA))) {
            break;
        }
    }
    rio_expect_token(RIO_TOKEN_RPAREN);
    rio_Typespec (*ret_type) = NULL;
    if (rio_match_token(RIO_TOKEN_COLON)) {
        ret_type = rio_parse_type();
    }
    rio_StmtList block = {0};
    bool is_incomplete = {0};
    if (rio_match_token(RIO_TOKEN_SEMICOLON)) {
        is_incomplete = true;
    } else {
        block = rio_parse_stmt_block();
        is_incomplete = false;
    }
    rio_Decl (*decl) = rio_new_decl_func(pos, name, params, rio_buf_len(params), ret_type, has_varargs, block);
    decl->is_incomplete = is_incomplete;
    return decl;
}

rio_NoteArg rio_parse_note_arg(void) {
    rio_SrcPos pos = rio_token.pos;
    rio_Expr (*expr) = rio_parse_expr();
    char const ((*name)) = NULL;
    if (rio_match_token(RIO_TOKEN_ASSIGN)) {
        if ((expr->kind) != (RIO_EXPR_NAME)) {
            rio_fatal_error(rio_token.pos, "Left of: operand = in note argument must be a name");
        }
        name = expr->name;
        expr = rio_parse_expr();
    }
    return (rio_NoteArg){.pos = pos, .name = name, .expr = expr};
}

rio_Note rio_parse_note(void) {
    rio_SrcPos pos = rio_token.pos;
    char const ((*name)) = rio_parse_name();
    rio_NoteArg (*args) = NULL;
    if (rio_match_token(RIO_TOKEN_LPAREN)) {
        rio_NoteArg arg = rio_parse_note_arg();
        rio_buf_push((void (**))(&(args)), &(arg), sizeof(arg));
        while (rio_match_token(RIO_TOKEN_COMMA)) {
            arg = rio_parse_note_arg();
            rio_buf_push((void (**))(&(args)), &(arg), sizeof(arg));
        }
        rio_expect_token(RIO_TOKEN_RPAREN);
    }
    return rio_new_note(pos, name, args, rio_buf_len(args));
}

rio_Notes rio_parse_notes(void) {
    rio_Note (*notes) = NULL;
    while (rio_match_token(RIO_TOKEN_AT)) {
        rio_Note note = rio_parse_note();
        rio_buf_push((void (**))(&(notes)), &(note), sizeof(note));
    }
    return rio_new_notes(notes, rio_buf_len(notes));
}

rio_Decl (*rio_parse_decl_note(rio_SrcPos pos)) {
    return rio_new_decl_note(pos, rio_parse_note());
}

rio_Decl (*rio_parse_decl_import(rio_SrcPos pos)) {
    char const ((*rename_name)) = {0};
    bool is_relative = {0};
    repeat: ;
    is_relative = false;
    if (rio_match_token(RIO_TOKEN_DOT)) {
        is_relative = true;
    }
    char const ((*name)) = rio_token.name;
    rio_expect_token(RIO_TOKEN_NAME);
    if ((!(is_relative)) && (rio_match_token(RIO_TOKEN_ASSIGN))) {
        if (rename_name) {
            rio_fatal_error(pos, "Only one import assignment is allowed");
        }
        rename_name = name;
        goto repeat;
    }
    char const ((*(*names))) = NULL;
    rio_buf_push((void (**))(&(names)), &(name), sizeof(name));
    while (rio_match_token(RIO_TOKEN_DOT)) {
        rio_buf_push((void (**))(&(names)), &(rio_token.name), sizeof(rio_token.name));
        rio_expect_token(RIO_TOKEN_NAME);
    }
    bool import_all = false;
    rio_ImportItem (*items) = {0};
    if (rio_match_token(RIO_TOKEN_LBRACE)) {
        while (!(rio_is_token(RIO_TOKEN_RBRACE))) {
            if (rio_match_token(RIO_TOKEN_ELLIPSIS)) {
                import_all = true;
            } else {
                char const ((*item_name)) = rio_parse_name();
                if (rio_match_token(RIO_TOKEN_ASSIGN)) {
                    rio_ImportItem item = {.name = rio_parse_name(), .rename = item_name};
                    rio_buf_push((void (**))(&(items)), &(item), sizeof(item));
                } else {
                    rio_ImportItem item = {.name = item_name};
                    rio_buf_push((void (**))(&(items)), &(item), sizeof(item));
                }
                if (!(rio_match_token(RIO_TOKEN_COMMA))) {
                    break;
                }
            }
        }
        rio_expect_token(RIO_TOKEN_RBRACE);
    }
    return rio_new_decl_import(pos, rename_name, is_relative, names, rio_buf_len(names), import_all, items, rio_buf_len(items));
}

rio_Decl (*rio_parse_decl_opt(void)) {
    rio_SrcPos pos = rio_token.pos;
    if (rio_match_keyword(rio_enum_keyword)) {
        return rio_parse_decl_enum(pos);
    } else if (rio_match_keyword(rio_struct_keyword)) {
        return rio_parse_decl_aggregate(pos, RIO_DECL_STRUCT);
    } else if (rio_match_keyword(rio_union_keyword)) {
        return rio_parse_decl_aggregate(pos, RIO_DECL_UNION);
    } else if (rio_match_keyword(rio_const_keyword)) {
        return rio_parse_decl_const(pos);
    } else if (rio_match_keyword(rio_typedef_keyword)) {
        return rio_parse_decl_typedef(pos);
    } else if (rio_match_keyword(rio_fn_keyword)) {
        return rio_parse_decl_func(pos);
    } else if (rio_match_keyword(rio_let_keyword)) {
        return rio_parse_decl_var(pos);
    } else if (rio_match_keyword(rio_import_keyword)) {
        return rio_parse_decl_import(pos);
    } else if (rio_match_token(RIO_TOKEN_POUND)) {
        return rio_parse_decl_note(pos);
    } else {
        return NULL;
    }
}

rio_Decl (*rio_parse_decl(void)) {
    rio_Notes notes = rio_parse_notes();
    rio_Decl (*decl) = rio_parse_decl_opt();
    if (!(decl)) {
        rio_fatal_error(rio_token.pos, "Expected declaration keyword, got %s", rio_token_info());
    }
    decl->notes = notes;
    return decl;
}

rio_Decls (*rio_parse_decls(void)) {
    rio_Decl (*(*decls)) = NULL;
    while (!(rio_is_token(RIO_TOKEN_EOF))) {
        rio_Decl (*decl) = rio_parse_decl();
        rio_buf_push((void (**))(&(decls)), &(decl), sizeof(decl));
    }
    return rio_new_decls(decls, rio_buf_len(decls));
}

rio_Package (*rio_current_package);
rio_Package (*rio_builtin_package);
rio_Map rio_package_map;
rio_Package (*(*rio_package_list));
uint8_t rio_reachable_phase = RIO_REACHABLE_NATURAL;
rio_Sym (*rio_get_package_sym(rio_Package (*package), char const ((*name)))) {
    return rio_map_get(&(package->syms_map), name);
}

void rio_add_package(rio_Package (*package)) {
    rio_Package (*old_package) = rio_map_get(&(rio_package_map), package->path);
    if ((old_package) != (package)) {
        assert(!(old_package));
        rio_map_put(&(rio_package_map), package->path, package);
        rio_buf_push((void (**))(&(rio_package_list)), &(package), sizeof(package));
    }
}

rio_Package (*rio_enter_package(rio_Package (*new_package))) {
    rio_Package (*old_package) = rio_current_package;
    rio_current_package = new_package;
    return old_package;
}

void rio_leave_package(rio_Package (*old_package)) {
    rio_current_package = old_package;
}

rio_Sym (*(*rio_reachable_syms));
rio_Sym (*(*rio_sorted_syms));
rio_Sym (rio_local_syms[RIO_MAX_LOCAL_SYMS]);
rio_Sym (*rio_local_syms_end) = rio_local_syms;
bool rio_is_local_sym(rio_Sym (*sym)) {
    return ((rio_local_syms) <= (sym)) && ((sym) < (rio_local_syms_end));
}

rio_Sym (*rio_sym_new(rio_SymKind kind, char const ((*name)), rio_Decl (*decl))) {
    rio_Sym (*sym) = rio_xcalloc(1, sizeof(rio_Sym));
    sym->kind = kind;
    sym->name = name;
    sym->decl = decl;
    sym->home_package = rio_current_package;
    rio_set_resolved_sym(sym, sym);
    return sym;
}

void rio_process_decl_notes(rio_Decl (*decl), rio_Sym (*sym)) {
    rio_Note (*foreign_note) = rio_get_decl_note(decl, rio_foreign_name);
    if (foreign_note) {
        if ((foreign_note->num_args) > (1)) {
            rio_fatal_error(decl->pos, "@foreign takes 0 or 1 argument");
        }
        char const ((*external_name)) = {0};
        if ((foreign_note->num_args) == (0)) {
            external_name = sym->name;
        } else {
            rio_Expr (*arg) = foreign_note->args[0].expr;
            if ((arg->kind) != (RIO_EXPR_STR)) {
                rio_fatal_error(decl->pos, "@foreign argument 1 must be a string literal");
            }
            external_name = arg->str_lit.val;
        }
        sym->external_name = external_name;
    }
}

rio_Sym (*rio_sym_decl(rio_Decl (*decl))) {
    rio_SymKind kind = RIO_SYM_NONE;
    switch (decl->kind) {
    case RIO_DECL_STRUCT:
    case RIO_DECL_UNION:
    case RIO_DECL_TYPEDEF:
    case RIO_DECL_ENUM: {
        kind = RIO_SYM_TYPE;
        break;
    }
    case RIO_DECL_VAR: {
        kind = RIO_SYM_VAR;
        break;
    }
    case RIO_DECL_CONST: {
        kind = RIO_SYM_CONST;
        break;
    }
    case RIO_DECL_FUNC: {
        kind = RIO_SYM_FUNC;
        break;
    }
    default: {
        assert(0);
        break;
    }
    }
    rio_Sym (*sym) = rio_sym_new(kind, decl->name, decl);
    rio_set_resolved_sym(decl, sym);
    rio_process_decl_notes(decl, sym);
    return sym;
}

rio_Sym (*rio_sym_get_local(char const ((*name)))) {
    for (rio_Sym (*it) = rio_local_syms_end; (it) != (rio_local_syms); (it)--) {
        rio_Sym (*sym) = (it) - (1);
        if ((sym->name) == (name)) {
            return sym;
        }
    }
    return NULL;
}

rio_Sym (*rio_sym_get(char const ((*name)))) {
    rio_Sym (*sym) = rio_sym_get_local(name);
    return (sym ? sym : rio_get_package_sym(rio_current_package, name));
}

bool rio_sym_push_var(char const ((*name)), rio_Type (*type)) {
    if (rio_sym_get_local(name)) {
        return false;
    }
    if ((rio_local_syms_end) == ((rio_local_syms) + (RIO_MAX_LOCAL_SYMS))) {
        rio_fatal("Too many local symbols");
    }
    *((rio_local_syms_end)++) = (rio_Sym){.name = name, .kind = RIO_SYM_VAR, .state = RIO_SYM_RESOLVED, .type = type};
    return true;
}

rio_Sym (*rio_sym_enter(void)) {
    return rio_local_syms_end;
}

void rio_sym_leave(rio_Sym (*sym)) {
    rio_local_syms_end = sym;
}

void rio_sym_global_put(char const ((*name)), rio_Sym (*sym)) {
    rio_Sym (*old_sym) = rio_map_get(&(rio_current_package->syms_map), name);
    if (old_sym) {
        if ((sym) == (old_sym)) {
            return;
        }
        if ((((sym->kind) == (RIO_SYM_PACKAGE)) && ((old_sym->kind) == (RIO_SYM_PACKAGE))) && ((sym->package) == (old_sym->package))) {
            return;
        }
        rio_SrcPos pos = (sym->decl ? sym->decl->pos : rio_pos_builtin);
        if (old_sym->decl) {
            rio_warning(old_sym->decl->pos, "Previous definition of \'%s\'", name);
        }
        rio_fatal_error(pos, "Duplicate definition of global symbol \'%s\'.", name);
    }
    rio_map_put(&(rio_current_package->syms_map), name, sym);
    rio_buf_push((void (**))(&(rio_current_package->syms)), &(sym), sizeof(sym));
}

rio_Sym (*rio_sym_global_type(char const ((*name)), rio_Type (*type))) {
    name = rio_str_intern(name);
    rio_Sym (*sym) = rio_sym_new(RIO_SYM_TYPE, name, NULL);
    sym->state = RIO_SYM_RESOLVED;
    sym->type = type;
    sym->external_name = name;
    rio_sym_global_put(name, sym);
    return sym;
}

rio_Sym (*rio_sym_global_decl(rio_Decl (*decl))) {
    rio_Sym (*sym) = NULL;
    if (decl->name) {
        sym = rio_sym_decl(decl);
        rio_sym_global_put(sym->name, sym);
    }
    if ((decl->kind) == (RIO_DECL_ENUM)) {
        rio_Typespec (*enum_typespec) = rio_new_typespec_name(decl->pos, (sym ? sym->name : rio_str_intern("int")));
        char const ((*prev_item_name)) = NULL;
        for (size_t i = 0; (i) < (decl->enum_decl.num_items); (i)++) {
            rio_EnumItem item = decl->enum_decl.items[i];
            rio_Expr (*init) = {0};
            if (item.init) {
                init = item.init;
            } else if (prev_item_name) {
                init = rio_new_expr_binary(item.pos, RIO_TOKEN_ADD, rio_new_expr_name(item.pos, prev_item_name), rio_new_expr_int(item.pos, 1, 0, 0));
            } else {
                init = rio_new_expr_int(item.pos, 0, 0, 0);
            }
            rio_Decl (*item_decl) = rio_new_decl_const(item.pos, item.name, enum_typespec, init);
            item_decl->notes = decl->notes;
            rio_sym_global_decl(item_decl);
            prev_item_name = item.name;
        }
    }
    return sym;
}

void rio_put_type_name(char (*(*buf)), rio_Type (*type)) {
    char const ((*type_name)) = rio_type_names[type->kind];
    if (type_name) {
        rio_buf_printf(buf, "%s", type_name);
    } else {
        switch (type->kind) {
        case RIO_CMPL_TYPE_STRUCT:
        case RIO_CMPL_TYPE_UNION:
        case RIO_CMPL_TYPE_ENUM:
        case RIO_CMPL_TYPE_INCOMPLETE: {
            assert(type->sym);
            rio_buf_printf(buf, "%s", type->sym->name);
            break;
        }
        case RIO_CMPL_TYPE_CONST: {
            rio_put_type_name(buf, type->base);
            rio_buf_printf(buf, " const");
            break;
        }
        case RIO_CMPL_TYPE_PTR: {
            rio_put_type_name(buf, type->base);
            rio_buf_printf(buf, "*");
            break;
        }
        case RIO_CMPL_TYPE_ARRAY: {
            rio_put_type_name(buf, type->base);
            rio_buf_printf(buf, "[%zu]", type->num_elems);
            break;
        }
        case RIO_CMPL_TYPE_FUNC: {
            rio_buf_printf(buf, "fn(");
            for (size_t i = 0; (i) < (type->function.num_params); (i)++) {
                if ((i) != (0)) {
                    rio_buf_printf(buf, ", ");
                }
                rio_put_type_name(buf, type->function.params[i]);
            }
            if (type->function.has_varargs) {
                rio_buf_printf(buf, "...");
            }
            rio_buf_printf(buf, ")");
            if ((type->function.ret) != (rio_type_void)) {
                rio_buf_printf(buf, ": ");
                rio_put_type_name(buf, type->function.ret);
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

char (*rio_get_type_name(rio_Type (*type))) {
    char (*buf) = NULL;
    rio_put_type_name(&(buf), type);
    return buf;
}

rio_Operand rio_operand_null;
rio_Operand rio_operand_rvalue(rio_Type (*type)) {
    return (rio_Operand){.type = rio_unqualify_type(type)};
}

rio_Operand rio_operand_lvalue(rio_Type (*type)) {
    return (rio_Operand){.type = type, .is_lvalue = true};
}

rio_Operand rio_operand_const(rio_Type (*type), rio_Val val) {
    return (rio_Operand){.type = rio_unqualify_type(type), .is_const = true, .val = val};
}

rio_Type (*rio_type_decay(rio_Type (*type))) {
    type = rio_unqualify_type(type);
    if ((type->kind) == (RIO_CMPL_TYPE_ARRAY)) {
        type = rio_type_ptr(type->base);
    }
    return type;
}

rio_Operand rio_operand_decay(rio_Operand operand) {
    operand.type = rio_type_decay(operand.type);
    operand.is_lvalue = false;
    return operand;
}

bool rio_is_convertible(rio_Operand (*operand), rio_Type (*dest)) {
    dest = rio_unqualify_type(dest);
    rio_Type (*src) = rio_unqualify_type(operand->type);
    if ((dest) == (src)) {
        return true;
    } else if ((rio_is_arithmetic_type(dest)) && (rio_is_arithmetic_type(src))) {
        return true;
    } else if ((rio_is_ptr_like_type(dest)) && (rio_is_null_ptr(*(operand)))) {
        return true;
    } else if ((rio_is_ptr_type(dest)) && (rio_is_ptr_type(src))) {
        if ((rio_is_const_type(dest->base)) && (rio_is_const_type(src->base))) {
            return (((dest->base->base) == (src->base->base)) || ((dest->base->base) == (rio_type_void))) || ((src->base->base) == (rio_type_void));
        } else {
            rio_Type (*unqual_dest_base) = rio_unqualify_type(dest->base);
            if ((unqual_dest_base) == (src->base)) {
                return true;
            } else if ((unqual_dest_base) == (rio_type_void)) {
                return (rio_is_const_type(dest->base)) || (!(rio_is_const_type(src->base)));
            } else {
                return (src->base) == (rio_type_void);
            }
        }
    } else {
        return false;
    }
}

bool rio_is_castable(rio_Operand (*operand), rio_Type (*dest)) {
    rio_Type (*src) = operand->type;
    if (rio_is_convertible(operand, dest)) {
        return true;
    } else if (rio_is_integer_type(dest)) {
        return rio_is_ptr_like_type(src);
    } else if (rio_is_integer_type(src)) {
        return rio_is_ptr_like_type(dest);
    } else if ((rio_is_ptr_like_type(dest)) && (rio_is_ptr_like_type(src))) {
        return true;
    } else {
        return false;
    }
}

bool rio_convert_operand(rio_Operand (*operand), rio_Type (*type)) {
    if (rio_is_convertible(operand, type)) {
        cast_operand(operand, type);
        operand->type = rio_unqualify_type(operand->type);
        operand->is_lvalue = false;
        return true;
    }
    return false;
}

bool rio_is_null_ptr(rio_Operand operand) {
    if ((operand.is_const) && (((rio_is_ptr_type(operand.type)) || (rio_is_integer_type(operand.type))))) {
        cast_operand(&(operand), rio_type_ullong);
        return (operand.val.ull) == (0);
    } else {
        return false;
    }
}

void rio_promote_operand(rio_Operand (*operand)) {
    switch (operand->type->kind) {
    case RIO_CMPL_TYPE_BOOL:
    case RIO_CMPL_TYPE_CHAR:
    case RIO_CMPL_TYPE_SCHAR:
    case RIO_CMPL_TYPE_UCHAR:
    case RIO_CMPL_TYPE_SHORT:
    case RIO_CMPL_TYPE_USHORT:
    case RIO_CMPL_TYPE_ENUM: {
        cast_operand(operand, rio_type_int);
        break;
    }
    default: {
        break;
    }
    }
}

void rio_unify_arithmetic_operands(rio_Operand (*left), rio_Operand (*right)) {
    if ((left->type) == (rio_type_double)) {
        cast_operand(right, rio_type_double);
    } else if ((right->type) == (rio_type_double)) {
        cast_operand(left, rio_type_double);
    } else if ((left->type) == (rio_type_float)) {
        cast_operand(right, rio_type_float);
    } else if ((right->type) == (rio_type_float)) {
        cast_operand(left, rio_type_float);
    } else {
        assert(rio_is_integer_type(left->type));
        assert(rio_is_integer_type(right->type));
        rio_promote_operand(left);
        rio_promote_operand(right);
        if ((left->type) != (right->type)) {
            if ((rio_is_signed_type(left->type)) == (rio_is_signed_type(right->type))) {
                if ((rio_type_rank(left->type)) <= (rio_type_rank(right->type))) {
                    cast_operand(left, right->type);
                } else {
                    cast_operand(right, left->type);
                }
            } else if ((rio_is_signed_type(left->type)) && ((rio_type_rank(right->type)) >= (rio_type_rank(left->type)))) {
                cast_operand(left, right->type);
            } else if ((rio_is_signed_type(right->type)) && ((rio_type_rank(left->type)) >= (rio_type_rank(right->type)))) {
                cast_operand(right, left->type);
            } else if ((rio_is_signed_type(left->type)) && ((rio_type_sizeof(left->type)) > (rio_type_sizeof(right->type)))) {
                cast_operand(right, left->type);
            } else if ((rio_is_signed_type(right->type)) && ((rio_type_sizeof(right->type)) > (rio_type_sizeof(left->type)))) {
                cast_operand(left, right->type);
            } else {
                rio_Type (*type) = rio_unsigned_type((rio_is_signed_type(left->type) ? left->type : right->type));
                cast_operand(left, type);
                cast_operand(right, type);
            }
        }
    }
    assert((left->type) == (right->type));
}

rio_Map rio_resolved_val_map;
rio_Val rio_get_resolved_val(void (*ptr)) {
    uint64_t u64 = rio_map_get_uint64(&(rio_resolved_val_map), ptr);
    rio_Val val = {0};
    assert((sizeof(val)) == (sizeof(u64)));
    memcpy(&(val), &(u64), sizeof(u64));
    return val;
}

void rio_set_resolved_val(void (*ptr), rio_Val val) {
    uint64_t u64 = {0};
    assert((sizeof(val)) == (sizeof(u64)));
    memcpy(&(u64), &(val), sizeof(val));
    rio_map_put_uint64(&(rio_resolved_val_map), ptr, u64);
}

rio_Map rio_resolved_type_map;
rio_Type (*rio_get_resolved_type(void (*ptr))) {
    return rio_map_get(&(rio_resolved_type_map), ptr);
}

void rio_set_resolved_type(void (*ptr), rio_Type (*type)) {
    rio_map_put(&(rio_resolved_type_map), ptr, type);
}

rio_Map rio_resolved_sym_map;
rio_Sym (*rio_get_resolved_sym(void const ((*ptr)))) {
    return rio_map_get(&(rio_resolved_sym_map), ptr);
}

void rio_set_resolved_sym(void const ((*ptr)), rio_Sym (*sym)) {
    if (!(rio_is_local_sym(sym))) {
        rio_map_put(&(rio_resolved_sym_map), ptr, sym);
    }
}

rio_Map rio_resolved_expected_type_map;
rio_Type (*rio_get_resolved_expected_type(rio_Expr (*expr))) {
    return rio_map_get(&(rio_resolved_expected_type_map), expr);
}

void rio_set_resolved_expected_type(rio_Expr (*expr), rio_Type (*type)) {
    if ((expr) && (type)) {
        rio_map_put(&(rio_resolved_expected_type_map), expr, type);
    }
}

rio_Operand rio_resolve_expr(rio_Expr (*expr)) {
    return rio_resolve_expected_expr(expr, NULL);
}

rio_Operand rio_resolve_expr_rvalue(rio_Expr (*expr)) {
    return rio_operand_decay(rio_resolve_expr(expr));
}

rio_Operand rio_resolve_expected_expr_rvalue(rio_Expr (*expr), rio_Type (*expected_type)) {
    return rio_operand_decay(rio_resolve_expected_expr(expr, expected_type));
}

rio_Type (*rio_resolve_typespec(rio_Typespec (*typespec))) {
    if (!(typespec)) {
        return rio_type_void;
    }
    rio_Type (*result) = NULL;
    switch (typespec->kind) {
    case RIO_TYPESPEC_NAME: {
        {
            rio_Sym (*sym) = rio_resolve_name(typespec->name);
            if (!(sym)) {
                rio_fatal_error(typespec->pos, "Unresolved type name \'%s\'", typespec->name);
            }
            if ((sym->kind) != (RIO_SYM_TYPE)) {
                rio_fatal_error(typespec->pos, "%s must denote a type", typespec->name);
                return NULL;
            }
            rio_set_resolved_sym(typespec, sym);
            result = sym->type;
        }
        break;
    }
    case RIO_TYPESPEC_CONST: {
        result = rio_type_const(rio_resolve_typespec(typespec->base));
        break;
    }
    case RIO_TYPESPEC_PTR: {
        result = rio_type_ptr(rio_resolve_typespec(typespec->base));
        break;
    }
    case RIO_TYPESPEC_ARRAY: {
        {
            int size = 0;
            if (typespec->num_elems) {
                rio_Operand operand = rio_resolve_const_expr(typespec->num_elems);
                if (!(rio_is_integer_type(operand.type))) {
                    rio_fatal_error(typespec->pos, "Array size constant expression must have integer type");
                }
                cast_operand(&(operand), rio_type_int);
                size = operand.val.i;
                if ((size) <= (0)) {
                    rio_fatal_error(typespec->num_elems->pos, "Non-positive array size");
                }
            }
            result = rio_type_array(rio_resolve_typespec(typespec->base), size);
        }
        break;
    }
    case RIO_TYPESPEC_FUNC: {
        {
            rio_Type (*(*args)) = NULL;
            for (size_t i = 0; (i) < (typespec->function.num_args); (i)++) {
                rio_Type (*arg) = rio_resolve_typespec(typespec->function.args[i]);
                if ((arg) == (rio_type_void)) {
                    rio_fatal_error(typespec->pos, "Function parameter type cannot be void");
                }
                rio_buf_push((void (**))(&(args)), &(arg), sizeof(arg));
            }
            rio_Type (*ret) = rio_type_void;
            if (typespec->function.ret) {
                ret = rio_resolve_typespec(typespec->function.ret);
            }
            if (rio_is_array_type(ret)) {
                rio_fatal_error(typespec->pos, "Function return type cannot be array");
            }
            result = rio_type_func(args, rio_buf_len(args), ret, false);
        }
        break;
    }
    default: {
        assert(0);
        return NULL;
        break;
    }
    }
    rio_set_resolved_type(typespec, result);
    return result;
}

rio_Type (*rio_complete_aggregate(rio_Type (*type), rio_Aggregate (*aggregate))) {
    rio_TypeField (*fields) = {0};
    for (size_t i = 0; (i) < (aggregate->num_items); (i)++) {
        rio_AggregateItem item = aggregate->items[i];
        if ((item.kind) == (RIO_AGGREGATE_ITEM_FIELD)) {
            rio_Type (*item_type) = rio_resolve_typespec(item.type);
            rio_complete_type(item_type);
            if ((rio_type_sizeof(item_type)) == (0)) {
                rio_fatal_error(item.pos, "Field type of size 0 is not allowed");
            }
            for (size_t j = 0; (j) < (item.num_names); (j)++) {
                rio_TypeField type_field = {item.names[j], item_type};
                rio_buf_push((void (**))(&(fields)), &(type_field), sizeof(type_field));
            }
        } else {
            assert((item.kind) == (RIO_AGGREGATE_ITEM_SUBAGGREGATE));
            rio_Type (*item_type) = rio_complete_aggregate(NULL, item.subaggregate);
            rio_TypeField type_field = {NULL, item_type};
            rio_buf_push((void (**))(&(fields)), &(type_field), sizeof(type_field));
        }
    }
    if (!(type)) {
        type = rio_type_incomplete(NULL);
        type->kind = RIO_CMPL_TYPE_COMPLETING;
    }
    if ((aggregate->kind) == (RIO_AGGREGATE_STRUCT)) {
        rio_type_complete_struct(type, fields, rio_buf_len(fields));
    } else {
        assert((aggregate->kind) == (RIO_AGGREGATE_UNION));
        rio_type_complete_union(type, fields, rio_buf_len(fields));
    }
    if ((type->aggregate.num_fields) == (0)) {
        rio_fatal_error(aggregate->pos, "No fields");
    }
    if (rio_has_duplicate_fields(type)) {
        rio_fatal_error(aggregate->pos, "Duplicate fields");
    }
    return type;
}

void rio_complete_type(rio_Type (*type)) {
    if ((type->kind) == (RIO_CMPL_TYPE_COMPLETING)) {
        rio_fatal_error(type->sym->decl->pos, "Type completion cycle");
        return;
    } else if ((type->kind) != (RIO_CMPL_TYPE_INCOMPLETE)) {
        return;
    }
    rio_Sym (*sym) = type->sym;
    rio_Package (*old_package) = rio_enter_package(sym->home_package);
    rio_Decl (*decl) = sym->decl;
    if (decl->is_incomplete) {
        rio_fatal_error(decl->pos, "Trying to use incomplete type as complete type");
    }
    type->kind = RIO_CMPL_TYPE_COMPLETING;
    assert(((decl->kind) == (RIO_DECL_STRUCT)) || ((decl->kind) == (RIO_DECL_UNION)));
    rio_complete_aggregate(type, decl->aggregate);
    rio_buf_push((void (**))(&(rio_sorted_syms)), &(type->sym), sizeof(type->sym));
    rio_leave_package(old_package);
}

rio_Type (*rio_resolve_typed_init(rio_SrcPos pos, rio_Type (*type), rio_Expr (*expr))) {
    rio_Type (*expected_type) = rio_unqualify_type(type);
    rio_Operand operand = rio_resolve_expected_expr(expr, expected_type);
    if (((rio_is_incomplete_array_type(type)) && (rio_is_array_type(operand.type))) && ((type->base) == (operand.type->base))) {
    } else {
        if ((type) && (rio_is_ptr_type(type))) {
            operand = rio_operand_decay(operand);
        }
        if (!(rio_convert_operand(&(operand), expected_type))) {
            return NULL;
        }
    }
    rio_set_resolved_expected_type(expr, operand.type);
    return operand.type;
}

rio_Type (*rio_resolve_init(rio_SrcPos pos, rio_Typespec (*typespec), rio_Expr (*expr))) {
    rio_Type (*type) = {0};
    if (typespec) {
        rio_Type (*declared_type) = rio_resolve_typespec(typespec);
        type = declared_type;
        if (expr) {
            type = rio_resolve_typed_init(pos, declared_type, expr);
            if (!(type)) {
                rio_fatal_error(pos, "Invalid type in initialization. Expected %s", rio_get_type_name(declared_type));
            }
        }
    } else {
        assert(expr);
        type = rio_unqualify_type(rio_resolve_expr(expr).type);
        if ((rio_is_array_type(type)) && ((expr->kind) != (RIO_EXPR_COMPOUND))) {
            type = rio_type_decay(type);
            rio_set_resolved_type(expr, type);
        }
        rio_set_resolved_expected_type(expr, type);
    }
    rio_complete_type(type);
    if ((type->size) == (0)) {
        rio_fatal_error(pos, "Cannot declare variable of size 0");
    }
    return type;
}

rio_Type (*rio_resolve_decl_var(rio_Decl (*decl))) {
    assert((decl->kind) == (RIO_DECL_VAR));
    return rio_resolve_init(decl->pos, decl->var_decl.type, decl->var_decl.expr);
}

rio_Type (*rio_resolve_decl_const(rio_Decl (*decl), rio_Val (*val))) {
    assert((decl->kind) == (RIO_DECL_CONST));
    rio_Operand result = rio_resolve_const_expr(decl->const_decl.expr);
    if (!(rio_is_scalar_type(result.type))) {
        rio_fatal_error(decl->pos, "Const declarations must have scalar type");
    }
    if (decl->const_decl.type) {
        rio_Type (*type) = rio_resolve_typespec(decl->const_decl.type);
        if (!(rio_convert_operand(&(result), type))) {
            rio_fatal_error(decl->pos, "Invalid type in constant declaration. Expected %s, got %s", rio_get_type_name(type), rio_get_type_name(result.type));
        }
    }
    *(val) = result.val;
    return result.type;
}

rio_Type (*rio_resolve_decl_func(rio_Decl (*decl))) {
    assert((decl->kind) == (RIO_DECL_FUNC));
    rio_Type (*(*params)) = NULL;
    for (size_t i = 0; (i) < (decl->function.num_params); (i)++) {
        rio_Type (*param) = rio_resolve_typespec(decl->function.params[i].type);
        rio_complete_type(param);
        if ((param) == (rio_type_void)) {
            rio_fatal_error(decl->pos, "Function parameter type cannot be void");
        }
        rio_buf_push((void (**))(&(params)), &(param), sizeof(param));
    }
    rio_Type (*ret_type) = rio_type_void;
    if (decl->function.ret_type) {
        ret_type = rio_resolve_typespec(decl->function.ret_type);
        rio_complete_type(ret_type);
    }
    if (rio_is_array_type(ret_type)) {
        rio_fatal_error(decl->pos, "Function return type cannot be array");
    }
    return rio_type_func(params, rio_buf_len(params), ret_type, decl->function.has_varargs);
}

rio_Label (rio_labels[RIO_MAX_LABELS]);
rio_Label (*rio_labels_end) = rio_labels;
rio_Label (*rio_get_label(rio_SrcPos pos, char const ((*name)))) {
    rio_Label (*label) = {0};
    for (label = rio_labels; (label) != (rio_labels_end); (label)++) {
        if ((label->name) == (name)) {
            return label;
        }
    }
    if ((label) == ((rio_labels) + (RIO_MAX_LABELS))) {
        rio_fatal_error(pos, "Too many labels");
    }
    *(label) = (rio_Label){.name = name, .pos = pos};
    (rio_labels_end)++;
    return label;
}

void rio_reference_label(rio_SrcPos pos, char const ((*name))) {
    rio_Label (*label) = rio_get_label(pos, name);
    label->referenced = true;
}

void rio_define_label(rio_SrcPos pos, char const ((*name))) {
    rio_Label (*label) = rio_get_label(pos, name);
    if (label->defined) {
        rio_fatal_error(pos, "Multiple definitions of label \'%s\'", name);
    }
    label->defined = true;
}

void rio_resolve_labels(void) {
    for (rio_Label (*label) = rio_labels; (label) != (rio_labels_end); (label)++) {
        if ((label->referenced) && (!(label->defined))) {
            rio_fatal_error(label->pos, "Label \'%s\' referenced but not defined", label->name);
        }
        if ((label->defined) && (!(label->referenced))) {
            rio_warning(label->pos, "Label \'%s\' defined but not referenced", label->name);
        }
    }
    rio_labels_end = rio_labels;
}

bool rio_is_cond_operand(rio_Operand operand) {
    operand = rio_operand_decay(operand);
    return rio_is_scalar_type(operand.type);
}

void rio_resolve_cond_expr(rio_Expr (*expr)) {
    rio_Operand cond = rio_resolve_expr_rvalue(expr);
    if (!(rio_is_cond_operand(cond))) {
        rio_fatal_error(expr->pos, "Conditional expression must have scalar type");
    }
}

bool rio_resolve_stmt_block(rio_StmtList block, rio_Type (*ret_type), rio_StmtCtx ctx) {
    rio_Sym (*scope) = rio_sym_enter();
    bool returns = false;
    for (size_t i = 0; (i) < (block.num_stmts); (i)++) {
        returns = (rio_resolve_stmt(block.stmts[i], ret_type, ctx)) || (returns);
    }
    rio_sym_leave(scope);
    return returns;
}

void rio_resolve_stmt_assign(rio_Stmt (*stmt)) {
    assert((stmt->kind) == (RIO_STMT_ASSIGN));
    rio_Operand left = rio_resolve_expr(stmt->assign.left);
    if (!(left.is_lvalue)) {
        rio_fatal_error(stmt->pos, "Cannot assign to non-lvalue");
    }
    if (rio_is_array_type(left.type)) {
        rio_fatal_error(stmt->pos, "Cannot assign to array");
    }
    if (left.type->nonmodifiable) {
        rio_fatal_error(stmt->pos, "Left-hand side of assignment has non-modifiable type");
    }
    char const ((*assign_op_name)) = rio_token_kind_name(stmt->assign.op);
    rio_TokenKind binary_op = rio_assign_token_to_binary_token[stmt->assign.op];
    rio_Operand right = rio_resolve_expected_expr_rvalue(stmt->assign.right, left.type);
    rio_Operand result = {0};
    if ((stmt->assign.op) == (RIO_TOKEN_ASSIGN)) {
        result = right;
    } else if (((stmt->assign.op) == (RIO_TOKEN_ADD_ASSIGN)) || ((stmt->assign.op) == (RIO_TOKEN_SUB_ASSIGN))) {
        if (((left.type->kind) == (RIO_CMPL_TYPE_PTR)) && (rio_is_integer_type(right.type))) {
            result = rio_operand_rvalue(left.type);
        } else if ((rio_is_arithmetic_type(left.type)) && (rio_is_arithmetic_type(right.type))) {
            result = rio_resolve_expr_binary_op(binary_op, assign_op_name, stmt->pos, left, right);
        } else {
            rio_fatal_error(stmt->pos, "Invalid operand types for %s", assign_op_name);
        }
    } else {
        result = rio_resolve_expr_binary_op(binary_op, assign_op_name, stmt->pos, left, right);
    }
    if (!(rio_convert_operand(&(result), left.type))) {
        rio_fatal_error(stmt->pos, "Invalid type in assignment. Expected %s, got %s", rio_get_type_name(left.type), rio_get_type_name(result.type));
    }
}

void rio_resolve_stmt_init(rio_Stmt (*stmt)) {
    assert((stmt->kind) == (RIO_STMT_INIT));
    rio_Type (*type) = rio_resolve_init(stmt->pos, stmt->init.type, stmt->init.expr);
    if (!(stmt->init.is_mut)) {
    }
    if (!(rio_sym_push_var(stmt->init.name, type))) {
        rio_fatal_error(stmt->pos, "Shadowed definition of local symbol");
    }
}

void rio_resolve_static_assert(rio_Note note) {
    if ((note.num_args) != (1)) {
        rio_fatal_error(note.pos, "#static_assert takes 1 argument");
    }
    rio_Operand operand = rio_resolve_const_expr(note.args[0].expr);
    if (!(operand.val.ull)) {
        rio_fatal_error(note.pos, "#static_assert failed");
    }
}

bool rio_resolve_stmt(rio_Stmt (*stmt), rio_Type (*ret_type), rio_StmtCtx ctx) {
    switch (stmt->kind) {
    case RIO_STMT_RETURN: {
        if (stmt->expr) {
            rio_Operand operand = rio_resolve_expected_expr_rvalue(stmt->expr, ret_type);
            if (!(rio_convert_operand(&(operand), ret_type))) {
                rio_fatal_error(stmt->pos, "Invalid type in return expression. Expected %s, got %s", rio_get_type_name(ret_type), rio_get_type_name(operand.type));
            }
        } else if ((ret_type) != (rio_type_void)) {
            rio_fatal_error(stmt->pos, "Empty return expression for function with non-void return type");
        }
        return true;
        break;
    }
    case RIO_STMT_BREAK: {
        if (!(ctx.is_break_legal)) {
            rio_fatal_error(stmt->pos, "Illegal break");
        }
        return false;
        break;
    }
    case RIO_STMT_CONTINUE: {
        if (!(ctx.is_continue_legal)) {
            rio_fatal_error(stmt->pos, "Illegal continue");
        }
        return false;
        break;
    }
    case RIO_STMT_BLOCK: {
        return rio_resolve_stmt_block(stmt->block, ret_type, ctx);
        break;
    }
    case RIO_STMT_NOTE: {
        if ((stmt->note.name) == (rio_assert_name)) {
            if ((stmt->note.num_args) != (1)) {
                rio_fatal_error(stmt->pos, "#assert takes 1 argument");
            }
            rio_resolve_cond_expr(stmt->note.args[0].expr);
        } else if ((stmt->note.name) == (rio_static_assert_name)) {
            rio_resolve_static_assert(stmt->note);
        } else {
            rio_warning(stmt->pos, "Unknown statement #directive \'%s\'", stmt->note.name);
        }
        return false;
        break;
    }
    case RIO_STMT_IF: {
        {
            rio_Sym (*scope) = rio_sym_enter();
            if (stmt->if_stmt.init) {
                rio_resolve_stmt_init(stmt->if_stmt.init);
            }
            if (stmt->if_stmt.cond) {
                rio_resolve_cond_expr(stmt->if_stmt.cond);
            } else if (!(rio_is_cond_operand(rio_resolve_name_operand(stmt->pos, stmt->if_stmt.init->init.name)))) {
                rio_fatal_error(stmt->pos, "Conditional expression must have scalar type");
            }
            bool returns = rio_resolve_stmt_block(stmt->if_stmt.then_block, ret_type, ctx);
            for (size_t i = 0; (i) < (stmt->if_stmt.num_elseifs); (i)++) {
                rio_ElseIf elseif = stmt->if_stmt.elseifs[i];
                rio_resolve_cond_expr(elseif.cond);
                returns = (rio_resolve_stmt_block(elseif.block, ret_type, ctx)) && (returns);
            }
            if (stmt->if_stmt.else_block.stmts) {
                returns = (rio_resolve_stmt_block(stmt->if_stmt.else_block, ret_type, ctx)) && (returns);
            } else {
                returns = false;
            }
            rio_sym_leave(scope);
            return returns;
        }
        break;
    }
    case RIO_STMT_WHILE:
    case RIO_STMT_DO_WHILE: {
        rio_resolve_cond_expr(stmt->while_stmt.cond);
        ctx.is_break_legal = true;
        ctx.is_continue_legal = true;
        rio_resolve_stmt_block(stmt->while_stmt.block, ret_type, ctx);
        return false;
        break;
    }
    case RIO_STMT_FOR: {
        {
            rio_Sym (*scope) = rio_sym_enter();
            if (stmt->for_stmt.init) {
                rio_resolve_stmt(stmt->for_stmt.init, ret_type, ctx);
            }
            if (stmt->for_stmt.cond) {
                rio_resolve_cond_expr(stmt->for_stmt.cond);
            }
            if (stmt->for_stmt.next) {
                rio_resolve_stmt(stmt->for_stmt.next, ret_type, ctx);
            }
            ctx.is_break_legal = true;
            ctx.is_continue_legal = true;
            rio_resolve_stmt_block(stmt->for_stmt.block, ret_type, ctx);
            rio_sym_leave(scope);
            return false;
        }
        break;
    }
    case RIO_STMT_SWITCH: {
        {
            rio_Operand operand = rio_resolve_expr_rvalue(stmt->switch_stmt.expr);
            if (!(rio_is_integer_type(operand.type))) {
                rio_fatal_error(stmt->pos, "Switch expression must have integer type");
            }
            ctx.is_break_legal = true;
            bool returns = true;
            bool has_default = false;
            for (size_t i = 0; (i) < (stmt->switch_stmt.num_cases); (i)++) {
                rio_SwitchCase switch_case = stmt->switch_stmt.cases[i];
                for (size_t j = 0; (j) < (switch_case.num_patterns); (j)++) {
                    rio_SwitchCasePattern pattern = switch_case.patterns[j];
                    rio_Expr (*start_expr) = pattern.start;
                    rio_Operand start_operand = rio_resolve_const_expr(start_expr);
                    if (!(rio_convert_operand(&(start_operand), operand.type))) {
                        rio_fatal_error(start_expr->pos, "Invalid type in switch case expression. Expected %s, got %s", rio_get_type_name(operand.type), rio_get_type_name(start_operand.type));
                    }
                    rio_Expr (*end_expr) = pattern.end;
                    if (end_expr) {
                        rio_Operand end_operand = rio_resolve_const_expr(end_expr);
                        if (!(rio_convert_operand(&(end_operand), operand.type))) {
                            rio_fatal_error(end_expr->pos, "Invalid type in switch case expression. Expected %s, got %s", rio_get_type_name(operand.type), rio_get_type_name(end_operand.type));
                        }
                        rio_convert_operand(&(start_operand), rio_type_llong);
                        rio_set_resolved_val(start_expr, start_operand.val);
                        rio_convert_operand(&(end_operand), rio_type_llong);
                        rio_set_resolved_val(end_expr, end_operand.val);
                        if ((end_operand.val.ll) < (start_operand.val.ll)) {
                            rio_fatal_error(start_expr->pos, "Case range end value cannot be less thn start value");
                        }
                        if (((end_operand.val.ll) - (start_operand.val.ll)) >= (256)) {
                            rio_fatal_error(start_expr->pos, "Case range cannot span more than 256 values");
                        }
                    }
                }
                if (switch_case.is_default) {
                    if (has_default) {
                        rio_fatal_error(stmt->pos, "Switch statement has multiple default clauses");
                    }
                    has_default = true;
                }
                if ((switch_case.block.num_stmts) > (1)) {
                    rio_Stmt (*last_stmt) = switch_case.block.stmts[(switch_case.block.num_stmts) - (1)];
                    if ((last_stmt->kind) == (RIO_STMT_BREAK)) {
                        rio_warning(last_stmt->pos, "Case blocks already end with an implicit break");
                    }
                }
                returns = (rio_resolve_stmt_block(switch_case.block, ret_type, ctx)) && (returns);
            }
            return (returns) && (has_default);
        }
        break;
    }
    case RIO_STMT_ASSIGN: {
        rio_resolve_stmt_assign(stmt);
        return false;
        break;
    }
    case RIO_STMT_INIT: {
        rio_resolve_stmt_init(stmt);
        return false;
        break;
    }
    case RIO_STMT_EXPR: {
        rio_resolve_expr(stmt->expr);
        return false;
        break;
    }
    case RIO_STMT_LABEL: {
        rio_define_label(stmt->pos, stmt->label);
        return false;
        break;
    }
    case RIO_STMT_GOTO: {
        rio_reference_label(stmt->pos, stmt->label);
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

void rio_resolve_func_body(rio_Sym (*sym)) {
    rio_Decl (*decl) = sym->decl;
    assert((decl->kind) == (RIO_DECL_FUNC));
    assert((sym->state) == (RIO_SYM_RESOLVED));
    if (decl->is_incomplete) {
        return;
    }
    rio_Package (*old_package) = rio_enter_package(sym->home_package);
    rio_Sym (*scope) = rio_sym_enter();
    for (size_t i = 0; (i) < (decl->function.num_params); (i)++) {
        rio_FuncParam param = decl->function.params[i];
        rio_Type (*param_type) = rio_resolve_typespec(param.type);
        if (rio_is_array_type(param_type)) {
            param_type = rio_type_ptr(param_type->base);
        }
        rio_sym_push_var(param.name, param_type);
    }
    rio_Type (*ret_type) = rio_resolve_typespec(decl->function.ret_type);
    assert(!(rio_is_array_type(ret_type)));
    bool returns = rio_resolve_stmt_block(decl->function.block, ret_type, (rio_StmtCtx){0});
    rio_resolve_labels();
    rio_sym_leave(scope);
    if (((ret_type) != (rio_type_void)) && (!(returns))) {
        rio_fatal_error(decl->pos, "Not all control paths return values");
    }
    rio_leave_package(old_package);
}

void rio_resolve_sym(rio_Sym (*sym)) {
    if ((sym->state) == (RIO_SYM_RESOLVED)) {
        return;
    } else if ((sym->state) == (RIO_SYM_RESOLVING)) {
        rio_fatal_error(sym->decl->pos, "Cyclic dependency");
        return;
    }
    assert((sym->state) == (RIO_SYM_UNRESOLVED));
    assert(!(sym->reachable));
    if (!(rio_is_local_sym(sym))) {
        rio_buf_push((void (**))(&(rio_reachable_syms)), &(sym), sizeof(sym));
        sym->reachable = rio_reachable_phase;
    }
    sym->state = RIO_SYM_RESOLVING;
    rio_Decl (*decl) = sym->decl;
    rio_Package (*old_package) = rio_enter_package(sym->home_package);
    switch (sym->kind) {
    case RIO_SYM_TYPE: {
        if ((decl) && ((decl->kind) == (RIO_DECL_TYPEDEF))) {
            sym->type = rio_resolve_typespec(decl->typedef_decl.type);
        } else if ((decl->kind) == (RIO_DECL_ENUM)) {
            rio_Type (*base) = (decl->enum_decl.type ? rio_resolve_typespec(decl->enum_decl.type) : rio_type_int);
            if (!(rio_is_integer_type(base))) {
                rio_fatal_error(decl->pos, "Base type of enum must be integer type");
            }
            sym->type = rio_type_enum(sym, base);
        } else {
            sym->type = rio_type_incomplete(sym);
        }
        break;
    }
    case RIO_SYM_VAR: {
        sym->type = rio_resolve_decl_var(decl);
        break;
    }
    case RIO_SYM_CONST: {
        sym->type = rio_resolve_decl_const(decl, &(sym->val));
        break;
    }
    case RIO_SYM_FUNC: {
        sym->type = rio_resolve_decl_func(decl);
        break;
    }
    case RIO_SYM_PACKAGE: {
        break;
        break;
    }
    default: {
        assert(0);
        break;
    }
    }
    rio_leave_package(old_package);
    sym->state = RIO_SYM_RESOLVED;
    if ((decl->is_incomplete) || ((((decl->kind) != (RIO_DECL_STRUCT)) && ((decl->kind) != (RIO_DECL_UNION))))) {
        rio_buf_push((void (**))(&(rio_sorted_syms)), &(sym), sizeof(sym));
    }
}

void rio_finalize_sym(rio_Sym (*sym)) {
    assert((sym->state) == (RIO_SYM_RESOLVED));
    if (((sym->decl) && (!(rio_is_decl_foreign(sym->decl)))) && (!(sym->decl->is_incomplete))) {
        if ((sym->kind) == (RIO_SYM_TYPE)) {
            rio_complete_type(sym->type);
        } else if ((sym->kind) == (RIO_SYM_FUNC)) {
            rio_resolve_func_body(sym);
        }
    }
}

rio_Sym (*rio_resolve_name(char const ((*name)))) {
    rio_Sym (*sym) = rio_sym_get(name);
    if (!(sym)) {
        return NULL;
    }
    rio_resolve_sym(sym);
    return sym;
}

rio_Package (*rio_try_resolve_package(rio_Expr (*expr))) {
    if ((expr->kind) == (RIO_EXPR_NAME)) {
        rio_Sym (*sym) = rio_resolve_name(expr->name);
        if ((sym) && ((sym->kind) == (RIO_SYM_PACKAGE))) {
            return sym->package;
        }
    } else if ((expr->kind) == (RIO_EXPR_FIELD)) {
        rio_Package (*package) = rio_try_resolve_package(expr->field.expr);
        if (package) {
            rio_Sym (*sym) = rio_get_package_sym(package, expr->field.name);
            if ((sym) && ((sym->kind) == (RIO_SYM_PACKAGE))) {
                return sym->package;
            }
        }
    }
    return NULL;
}

rio_Operand rio_resolve_expr_field(rio_Expr (*expr)) {
    assert((expr->kind) == (RIO_EXPR_FIELD));
    rio_Package (*package) = rio_try_resolve_package(expr->field.expr);
    if (package) {
        rio_Package (*old_package) = rio_enter_package(package);
        rio_Sym (*sym) = rio_resolve_name(expr->field.name);
        rio_Operand operand = rio_resolve_name_operand(expr->pos, expr->field.name);
        rio_leave_package(old_package);
        rio_set_resolved_sym(expr, sym);
        return operand;
    }
    rio_Operand operand = rio_resolve_expr(expr->field.expr);
    bool was_const_type = rio_is_const_type(operand.type);
    rio_Type (*type) = rio_unqualify_type(operand.type);
    rio_complete_type(type);
    if (rio_is_ptr_type(type)) {
        operand = rio_operand_lvalue(type->base);
        was_const_type = rio_is_const_type(operand.type);
        type = rio_unqualify_type(operand.type);
        rio_complete_type(type);
    }
    if (((type->kind) != (RIO_CMPL_TYPE_STRUCT)) && ((type->kind) != (RIO_CMPL_TYPE_UNION))) {
        rio_fatal_error(expr->pos, "Can only access fields on aggregates or pointers to aggregates");
        return rio_operand_null;
    }
    for (size_t i = 0; (i) < (type->aggregate.num_fields); (i)++) {
        rio_TypeField field = type->aggregate.fields[i];
        if ((field.name) == (expr->field.name)) {
            rio_Operand field_operand = (operand.is_lvalue ? rio_operand_lvalue(field.type) : rio_operand_rvalue(field.type));
            if (was_const_type) {
                field_operand.type = rio_type_const(field_operand.type);
            }
            return field_operand;
        }
    }
    rio_fatal_error(expr->pos, "No field named \'%s\'", expr->field.name);
    return rio_operand_null;
}

llong rio_eval_unary_op_ll(rio_TokenKind op, llong val) {
    switch (op) {
    case RIO_TOKEN_ADD: {
        return +(val);
        break;
    }
    case RIO_TOKEN_SUB: {
        return -(val);
        break;
    }
    case RIO_TOKEN_NEG: {
        return ~(val);
        break;
    }
    case RIO_TOKEN_NOT: {
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

ullong rio_eval_unary_op_ull(rio_TokenKind op, ullong val) {
    switch (op) {
    case RIO_TOKEN_ADD: {
        return +(val);
        break;
    }
    case RIO_TOKEN_SUB: {
        return (0ull) - (val);
        break;
    }
    case RIO_TOKEN_NEG: {
        return ~(val);
        break;
    }
    case RIO_TOKEN_NOT: {
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

llong rio_eval_binary_op_ll(rio_TokenKind op, llong left, llong right) {
    switch (op) {
    case RIO_TOKEN_MUL: {
        return (left) * (right);
        break;
    }
    case RIO_TOKEN_DIV: {
        return ((right) != (0) ? (left) / (right) : 0);
        break;
    }
    case RIO_TOKEN_MOD: {
        return ((right) != (0) ? (left) % (right) : 0);
        break;
    }
    case RIO_TOKEN_AND: {
        return (left) & (right);
        break;
    }
    case RIO_TOKEN_LSHIFT: {
        return (left) << (right);
        break;
    }
    case RIO_TOKEN_RSHIFT: {
        return (left) >> (right);
        break;
    }
    case RIO_TOKEN_ADD: {
        return (left) + (right);
        break;
    }
    case RIO_TOKEN_SUB: {
        return (left) - (right);
        break;
    }
    case RIO_TOKEN_OR: {
        return (left) | (right);
        break;
    }
    case RIO_TOKEN_XOR: {
        return (left) ^ (right);
        break;
    }
    case RIO_TOKEN_EQ: {
        return (left) == (right);
        break;
    }
    case RIO_TOKEN_NOTEQ: {
        return (left) != (right);
        break;
    }
    case RIO_TOKEN_LT: {
        return (left) < (right);
        break;
    }
    case RIO_TOKEN_LTEQ: {
        return (left) <= (right);
        break;
    }
    case RIO_TOKEN_GT: {
        return (left) > (right);
        break;
    }
    case RIO_TOKEN_GTEQ: {
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

ullong rio_eval_binary_op_ull(rio_TokenKind op, ullong left, ullong right) {
    switch (op) {
    case RIO_TOKEN_MUL: {
        return (left) * (right);
        break;
    }
    case RIO_TOKEN_DIV: {
        return ((right) != (0) ? (left) / (right) : 0);
        break;
    }
    case RIO_TOKEN_MOD: {
        return ((right) != (0) ? (left) % (right) : 0);
        break;
    }
    case RIO_TOKEN_AND: {
        return (left) & (right);
        break;
    }
    case RIO_TOKEN_LSHIFT: {
        return (left) << (right);
        break;
    }
    case RIO_TOKEN_RSHIFT: {
        return (left) >> (right);
        break;
    }
    case RIO_TOKEN_ADD: {
        return (left) + (right);
        break;
    }
    case RIO_TOKEN_SUB: {
        return (left) - (right);
        break;
    }
    case RIO_TOKEN_OR: {
        return (left) | (right);
        break;
    }
    case RIO_TOKEN_XOR: {
        return (left) ^ (right);
        break;
    }
    case RIO_TOKEN_EQ: {
        return (left) == (right);
        break;
    }
    case RIO_TOKEN_NOTEQ: {
        return (left) != (right);
        break;
    }
    case RIO_TOKEN_LT: {
        return (left) < (right);
        break;
    }
    case RIO_TOKEN_LTEQ: {
        return (left) <= (right);
        break;
    }
    case RIO_TOKEN_GT: {
        return (left) > (right);
        break;
    }
    case RIO_TOKEN_GTEQ: {
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

rio_Val rio_eval_unary_op(rio_TokenKind op, rio_Type (*type), rio_Val val) {
    if (rio_is_integer_type(type)) {
        rio_Operand operand = rio_operand_const(type, val);
        if (rio_is_signed_type(type)) {
            cast_operand(&(operand), rio_type_llong);
            operand.val.ll = rio_eval_unary_op_ll(op, operand.val.ll);
        } else {
            cast_operand(&(operand), rio_type_ullong);
            operand.val.ll = rio_eval_unary_op_ull(op, operand.val.ull);
        }
        cast_operand(&(operand), type);
        return operand.val;
    } else {
        return (rio_Val){0};
    }
}

rio_Val rio_eval_binary_op(rio_TokenKind op, rio_Type (*type), rio_Val left, rio_Val right) {
    if (rio_is_integer_type(type)) {
        rio_Operand left_operand = rio_operand_const(type, left);
        rio_Operand right_operand = rio_operand_const(type, right);
        rio_Operand result_operand = {0};
        if (rio_is_signed_type(type)) {
            cast_operand(&(left_operand), rio_type_llong);
            cast_operand(&(right_operand), rio_type_llong);
            result_operand = rio_operand_const(rio_type_llong, (rio_Val){.ll = rio_eval_binary_op_ll(op, left_operand.val.ll, right_operand.val.ll)});
        } else {
            cast_operand(&(left_operand), rio_type_ullong);
            cast_operand(&(right_operand), rio_type_ullong);
            result_operand = rio_operand_const(rio_type_ullong, (rio_Val){.ull = rio_eval_binary_op_ull(op, left_operand.val.ull, right_operand.val.ull)});
        }
        cast_operand(&(result_operand), type);
        return result_operand.val;
    } else {
        return (rio_Val){0};
    }
}

rio_Operand rio_resolve_name_operand(rio_SrcPos pos, char const ((*name))) {
    rio_Sym (*sym) = rio_resolve_name(name);
    if (!(sym)) {
        rio_fatal_error(pos, "Unresolved name \'%s\'", name);
    }
    if ((sym->kind) == (RIO_SYM_VAR)) {
        rio_Operand operand = rio_operand_lvalue(sym->type);
        if (rio_is_array_type(operand.type)) {
            operand = rio_operand_decay(operand);
        }
        return operand;
    } else if ((sym->kind) == (RIO_SYM_CONST)) {
        return rio_operand_const(sym->type, sym->val);
    } else if ((sym->kind) == (RIO_SYM_FUNC)) {
        return rio_operand_rvalue(sym->type);
    } else {
        rio_fatal_error(pos, "%s must be a let or const", name);
        return rio_operand_null;
    }
}

rio_Operand rio_resolve_expr_name(rio_Expr (*expr)) {
    assert((expr->kind) == (RIO_EXPR_NAME));
    return rio_resolve_name_operand(expr->pos, expr->name);
}

rio_Operand rio_resolve_unary_op(rio_TokenKind op, rio_Operand operand) {
    rio_promote_operand(&(operand));
    if (operand.is_const) {
        return rio_operand_const(operand.type, rio_eval_unary_op(op, operand.type, operand.val));
    } else {
        return operand;
    }
}

rio_Operand rio_resolve_expr_unary(rio_Expr (*expr)) {
    rio_Operand operand = rio_resolve_expr_rvalue(expr->unary.expr);
    rio_Type (*type) = operand.type;
    switch (expr->unary.op) {
    case RIO_TOKEN_MUL: {
        if (!(rio_is_ptr_type(type))) {
            rio_fatal_error(expr->pos, "Cannot deref non-ptr type");
        }
        return rio_operand_lvalue(type->base);
        break;
    }
    case RIO_TOKEN_ADD:
    case RIO_TOKEN_SUB: {
        if (!(rio_is_arithmetic_type(type))) {
            rio_fatal_error(expr->pos, "Can only use unary %s with arithmetic types", rio_token_kind_name(expr->unary.op));
        }
        return rio_resolve_unary_op(expr->unary.op, operand);
        break;
    }
    case RIO_TOKEN_NEG: {
        if (!(rio_is_integer_type(type))) {
            rio_fatal_error(expr->pos, "Can only use ~ with integer types");
        }
        return rio_resolve_unary_op(expr->unary.op, operand);
        break;
    }
    case RIO_TOKEN_NOT: {
        if (!(rio_is_scalar_type(type))) {
            rio_fatal_error(expr->pos, " Can only use ! with scalar types");
        }
        return rio_resolve_unary_op(expr->unary.op, operand);
        break;
    }
    default: {
        assert(0);
        break;
    }
    }
    return (rio_Operand){0};
}

rio_Operand rio_resolve_binary_op(rio_TokenKind op, rio_Operand left, rio_Operand right) {
    if ((left.is_const) && (right.is_const)) {
        return rio_operand_const(left.type, rio_eval_binary_op(op, left.type, left.val, right.val));
    } else {
        return rio_operand_rvalue(left.type);
    }
}

rio_Operand rio_resolve_binary_arithmetic_op(rio_TokenKind op, rio_Operand left, rio_Operand right) {
    rio_unify_arithmetic_operands(&(left), &(right));
    return rio_resolve_binary_op(op, left, right);
}

rio_Operand rio_resolve_expr_binary_op(rio_TokenKind op, char const ((*op_name)), rio_SrcPos pos, rio_Operand left, rio_Operand right) {
    switch (op) {
    case RIO_TOKEN_MUL:
    case RIO_TOKEN_DIV: {
        if (!(rio_is_arithmetic_type(left.type))) {
            rio_fatal_error(pos, "Left operand of %s must have arithmetic type", op_name);
        }
        if (!(rio_is_arithmetic_type(right.type))) {
            rio_fatal_error(pos, "Right operand of %s must have arithmetic type", op_name);
        }
        return rio_resolve_binary_arithmetic_op(op, left, right);
        break;
    }
    case RIO_TOKEN_MOD: {
        if (!(rio_is_integer_type(left.type))) {
            rio_fatal_error(pos, "Left operand of %% must have integer type");
        }
        if (!(rio_is_integer_type(right.type))) {
            rio_fatal_error(pos, "Right operand of %% must have integer type");
        }
        return rio_resolve_binary_arithmetic_op(op, left, right);
        break;
    }
    case RIO_TOKEN_ADD: {
        if ((rio_is_arithmetic_type(left.type)) && (rio_is_arithmetic_type(right.type))) {
            return rio_resolve_binary_arithmetic_op(op, left, right);
        } else if ((rio_is_ptr_type(left.type)) && (rio_is_integer_type(right.type))) {
            rio_complete_type(left.type->base);
            if ((rio_type_sizeof(left.type->base)) == (0)) {
                rio_fatal_error(pos, "Cannot do pointer arithmetic with size 0 base type");
            }
            return rio_operand_rvalue(left.type);
        } else if ((rio_is_ptr_type(right.type)) && (rio_is_integer_type(left.type))) {
            rio_complete_type(right.type->base);
            if ((rio_type_sizeof(right.type->base)) == (0)) {
                rio_fatal_error(pos, "Cannot do pointer arithmetic with size 0 base type");
            }
            return rio_operand_rvalue(right.type);
        } else {
            rio_fatal_error(pos, "Operands of + must both have arithmetic type, or pointer and integer type");
        }
        break;
    }
    case RIO_TOKEN_SUB: {
        if ((rio_is_arithmetic_type(left.type)) && (rio_is_arithmetic_type(right.type))) {
            return rio_resolve_binary_arithmetic_op(op, left, right);
        } else if ((rio_is_ptr_type(left.type)) && (rio_is_integer_type(right.type))) {
            return rio_operand_rvalue(left.type);
        } else if ((rio_is_ptr_type(left.type)) && (rio_is_ptr_type(right.type))) {
            if ((left.type->base) != (right.type->base)) {
                rio_fatal_error(pos, "Cannot subtract pointers to different types");
            }
            return rio_operand_rvalue(rio_type_ssize);
        } else {
            rio_fatal_error(pos, "Operands of - must both have arithmetic type, pointer and integer type, or compatible pointer types");
        }
        break;
    }
    case RIO_TOKEN_LSHIFT:
    case RIO_TOKEN_RSHIFT: {
        if ((rio_is_integer_type(left.type)) && (rio_is_integer_type(right.type))) {
            rio_promote_operand(&(left));
            rio_promote_operand(&(right));
            rio_Type (*result_type) = left.type;
            rio_Operand result = {0};
            if (rio_is_signed_type(left.type)) {
                cast_operand(&(left), rio_type_llong);
                cast_operand(&(right), rio_type_llong);
            } else {
                cast_operand(&(left), rio_type_ullong);
                cast_operand(&(right), rio_type_ullong);
            }
            result = rio_resolve_binary_op(op, left, right);
            cast_operand(&(result), result_type);
            return result;
        } else {
            rio_fatal_error(pos, "Operands of %s must both have integer type", op_name);
        }
        break;
    }
    case RIO_TOKEN_EQ:
    case RIO_TOKEN_NOTEQ: {
        if ((rio_is_arithmetic_type(left.type)) && (rio_is_arithmetic_type(right.type))) {
            rio_Operand result = rio_resolve_binary_arithmetic_op(op, left, right);
            cast_operand(&(result), rio_type_int);
            return result;
        } else if ((rio_is_ptr_type(left.type)) && (rio_is_ptr_type(right.type))) {
            rio_Type (*unqual_left_base) = rio_unqualify_type(left.type->base);
            rio_Type (*unqual_right_base) = rio_unqualify_type(right.type->base);
            if ((((unqual_left_base) != (unqual_right_base)) && ((unqual_left_base) != (rio_type_void))) && ((unqual_right_base) != (rio_type_void))) {
                rio_fatal_error(pos, "Cannot compare pointers to different types");
            }
            return rio_operand_rvalue(rio_type_int);
        } else if ((((rio_is_null_ptr(left)) && (rio_is_ptr_type(right.type)))) || (((rio_is_null_ptr(right)) && (rio_is_ptr_type(left.type))))) {
            return rio_operand_rvalue(rio_type_int);
        } else {
            rio_fatal_error(pos, "Operands of %s must be arithmetic types or compatible pointer types", op_name);
        }
        break;
    }
    case RIO_TOKEN_LT:
    case RIO_TOKEN_LTEQ:
    case RIO_TOKEN_GT:
    case RIO_TOKEN_GTEQ: {
        if ((rio_is_arithmetic_type(left.type)) && (rio_is_arithmetic_type(right.type))) {
            rio_Operand result = rio_resolve_binary_arithmetic_op(op, left, right);
            cast_operand(&(result), rio_type_int);
            return result;
        } else if ((rio_is_ptr_type(left.type)) && (rio_is_ptr_type(right.type))) {
            if ((rio_unqualify_type(left.type->base)) != (rio_unqualify_type(right.type->base))) {
                rio_fatal_error(pos, "Cannot compare pointers to different types");
            }
            return rio_operand_rvalue(rio_type_int);
        } else if ((((rio_is_null_ptr(left)) && (rio_is_ptr_type(right.type)))) || (((rio_is_null_ptr(right)) && (rio_is_ptr_type(left.type))))) {
            return rio_operand_rvalue(rio_type_int);
        } else {
            rio_fatal_error(pos, "Operands of %s must be arithmetic types or compatible pointer types", op_name);
        }
        break;
    }
    case RIO_TOKEN_AND:
    case RIO_TOKEN_XOR:
    case RIO_TOKEN_OR: {
        if ((rio_is_integer_type(left.type)) && (rio_is_integer_type(right.type))) {
            return rio_resolve_binary_arithmetic_op(op, left, right);
        } else {
            rio_fatal_error(pos, "Operands of %s must have arithmetic types", op_name);
        }
        break;
    }
    case RIO_TOKEN_AND_AND:
    case RIO_TOKEN_OR_OR: {
        if ((rio_is_scalar_type(left.type)) && (rio_is_scalar_type(right.type))) {
            if ((left.is_const) && (right.is_const)) {
                cast_operand(&(left), rio_type_bool);
                cast_operand(&(right), rio_type_bool);
                int i = {0};
                if ((op) == (RIO_TOKEN_AND_AND)) {
                    i = (left.val.b) && (right.val.b);
                } else {
                    assert((op) == (RIO_TOKEN_OR_OR));
                    i = (left.val.b) || (right.val.b);
                }
                return rio_operand_const(rio_type_int, (rio_Val){.i = i});
            } else {
                return rio_operand_rvalue(rio_type_int);
            }
        } else {
            rio_fatal_error(pos, "Operands of %s must have scalar types", op_name);
        }
        break;
    }
    default: {
        assert(0);
        break;
    }
    }
    return (rio_Operand){0};
}

rio_Operand rio_resolve_expr_binary(rio_Expr (*expr)) {
    assert((expr->kind) == (RIO_EXPR_BINARY));
    rio_Operand left = rio_resolve_expr_rvalue(expr->binary.left);
    rio_Operand right = rio_resolve_expr_rvalue(expr->binary.right);
    rio_TokenKind op = expr->binary.op;
    char const ((*op_name)) = rio_token_kind_name(op);
    return rio_resolve_expr_binary_op(op, op_name, expr->pos, left, right);
}

rio_Operand rio_resolve_expr_compound(rio_Expr (*expr), rio_Type (*expected_type)) {
    assert((expr->kind) == (RIO_EXPR_COMPOUND));
    if ((!(expected_type)) && (!(expr->compound.type))) {
        rio_fatal_error(expr->pos, "Implicitly typed compound literals used in context without expected type");
    }
    rio_Type (*type) = NULL;
    if (expr->compound.type) {
        type = rio_resolve_typespec(expr->compound.type);
    } else {
        type = expected_type;
    }
    rio_complete_type(type);
    bool is_const = rio_is_const_type(type);
    type = rio_unqualify_type(type);
    if (((type->kind) == (RIO_CMPL_TYPE_STRUCT)) || ((type->kind) == (RIO_CMPL_TYPE_UNION))) {
        int index = 0;
        for (size_t i = 0; (i) < (expr->compound.num_fields); (i)++) {
            rio_CompoundField field = expr->compound.fields[i];
            if ((field.kind) == (RIO_FIELD_INDEX)) {
                rio_fatal_error(field.pos, "Index field initializer not allowed for struct/union compound literal");
            } else if ((field.kind) == (RIO_FIELD_NAME)) {
                index = rio_aggregate_item_field_index(type, field.name);
                if ((index) == (-(1))) {
                    rio_fatal_error(field.pos, "Named field in compound literal does not exist");
                }
            }
            if ((index) >= ((int)(type->aggregate.num_fields))) {
                rio_fatal_error(field.pos, "Field initializer in struct/union compound literal out of range");
            }
            rio_Type (*field_type) = type->aggregate.fields[index].type;
            if (!(rio_resolve_typed_init(field.pos, field_type, field.init))) {
                rio_fatal_error(field.pos, "Invalid type in compound literal initializer for aggregate type. Expected %s", rio_get_type_name(field_type));
            }
            (index)++;
        }
    } else if ((type->kind) == (RIO_CMPL_TYPE_ARRAY)) {
        size_t index = 0;
        size_t max_index = 0;
        for (size_t i = 0; (i) < (expr->compound.num_fields); (i)++) {
            rio_CompoundField field = expr->compound.fields[i];
            if ((field.kind) == (RIO_FIELD_NAME)) {
                rio_fatal_error(field.pos, "Named field initializer not allowed for array compound literals");
            } else if ((field.kind) == (RIO_FIELD_INDEX)) {
                rio_Operand operand = rio_resolve_const_expr(field.index);
                if (!(rio_is_integer_type(operand.type))) {
                    rio_fatal_error(field.pos, "Field initializer index expression must have type int");
                }
                if (!(cast_operand(&(operand), rio_type_int))) {
                    rio_fatal_error(field.pos, "Invalid type in field initializer index. Expected integer type");
                }
                if ((operand.val.i) < (0)) {
                    rio_fatal_error(field.pos, "Field initializer index cannot be negative");
                }
                index = operand.val.i;
            }
            if ((type->num_elems) && ((index) >= ((int)(type->num_elems)))) {
                rio_fatal_error(field.pos, "Field initializer in array compound literal out of range");
            }
            if (!(rio_resolve_typed_init(field.pos, type->base, field.init))) {
                rio_fatal_error(field.pos, "Invalid type in compound literal initializer for array type. Expected %s", rio_get_type_name(type->base));
            }
            max_index = rio_max(max_index, index);
            (index)++;
        }
        if ((type->num_elems) == (0)) {
            type = rio_type_array(type->base, (max_index) + (1));
        }
    } else {
        assert(rio_is_scalar_type(type));
        if ((expr->compound.num_fields) > (1)) {
            rio_fatal_error(expr->pos, "Compound literal for scalar type cannot have more than one operand");
        }
        if ((expr->compound.num_fields) == (1)) {
            rio_CompoundField field = expr->compound.fields[0];
            rio_Operand init = rio_resolve_expected_expr_rvalue(field.init, type);
            if (!(rio_convert_operand(&(init), type))) {
                rio_fatal_error(field.pos, "Invalid type in compound literal initializer. Expected %s, got %s", rio_get_type_name(type), rio_get_type_name(init.type));
            }
        }
    }
    return rio_operand_lvalue((is_const ? rio_type_const(type) : type));
}

rio_Operand rio_resolve_expr_call(rio_Expr (*expr)) {
    assert((expr->kind) == (RIO_EXPR_CALL));
    if ((expr->call.expr->kind) == (RIO_EXPR_NAME)) {
        rio_Sym (*sym) = rio_resolve_name(expr->call.expr->name);
        if ((sym) && ((sym->kind) == (RIO_SYM_TYPE))) {
            if ((expr->call.num_args) != (1)) {
                rio_fatal_error(expr->pos, "Type conversion operator takes 1 argument");
            }
            rio_Operand operand = rio_resolve_expr_rvalue(expr->call.args[0]);
            if (!(cast_operand(&(operand), sym->type))) {
                rio_fatal_error(expr->pos, "Invalid type cast from %s to %s", rio_get_type_name(operand.type), rio_get_type_name(sym->type));
            }
            rio_set_resolved_sym(expr->call.expr, sym);
            return operand;
        }
    }
    rio_Operand function = rio_resolve_expr_rvalue(expr->call.expr);
    if ((function.type->kind) != (RIO_CMPL_TYPE_FUNC)) {
        rio_fatal_error(expr->pos, "Cannot call non-function value");
    }
    size_t num_params = function.type->function.num_params;
    if ((expr->call.num_args) < (num_params)) {
        rio_fatal_error(expr->pos, "Function call with too few arguments");
    }
    if (((expr->call.num_args) > (num_params)) && (!(function.type->function.has_varargs))) {
        rio_fatal_error(expr->pos, "Function call with too many arguments");
    }
    for (size_t i = 0; (i) < (num_params); (i)++) {
        rio_Type (*param_type) = function.type->function.params[i];
        rio_Operand arg = rio_resolve_expected_expr_rvalue(expr->call.args[i], param_type);
        if (rio_is_array_type(param_type)) {
            param_type = rio_type_ptr(param_type->base);
        }
        if (!(rio_convert_operand(&(arg), param_type))) {
            rio_fatal_error(expr->call.args[i]->pos, "Invalid type in function call argument. Expected %s, got %s", rio_get_type_name(param_type), rio_get_type_name(arg.type));
        }
    }
    for (size_t i = num_params; (i) < (expr->call.num_args); (i)++) {
        rio_resolve_expr_rvalue(expr->call.args[i]);
    }
    return rio_operand_rvalue(function.type->function.ret);
}

rio_Operand rio_resolve_expr_ternary(rio_Expr (*expr), rio_Type (*expected_type)) {
    assert((expr->kind) == (RIO_EXPR_TERNARY));
    rio_Operand cond = rio_resolve_expr_rvalue(expr->ternary.cond);
    if (!(rio_is_scalar_type(cond.type))) {
        rio_fatal_error(expr->pos, "Ternary conditional must have scalar type");
    }
    rio_Operand left = rio_resolve_expected_expr_rvalue(expr->ternary.then_expr, expected_type);
    rio_Operand right = rio_resolve_expected_expr_rvalue(expr->ternary.else_expr, expected_type);
    if ((left.type) == (right.type)) {
        return rio_operand_rvalue(left.type);
    } else if ((rio_is_arithmetic_type(left.type)) && (rio_is_arithmetic_type(right.type))) {
        rio_unify_arithmetic_operands(&(left), &(right));
        if (((cond.is_const) && (left.is_const)) && (right.is_const)) {
            return rio_operand_const(left.type, (cond.val.i ? left.val : right.val));
        } else {
            return rio_operand_rvalue(left.type);
        }
    } else if ((rio_is_ptr_type(left.type)) && (rio_is_null_ptr(right))) {
        return rio_operand_rvalue(left.type);
    } else if ((rio_is_ptr_type(right.type)) && (rio_is_null_ptr(left))) {
        return rio_operand_rvalue(right.type);
    } else {
        rio_fatal_error(expr->pos, "Left and right operands of ternary expression must have arithmetic types or identical types");
    }
    return (rio_Operand){0};
}

rio_Operand rio_resolve_expr_index(rio_Expr (*expr)) {
    assert((expr->kind) == (RIO_EXPR_INDEX));
    rio_Operand operand = rio_resolve_expr_rvalue(expr->index.expr);
    if (!(rio_is_ptr_type(operand.type))) {
        rio_fatal_error(expr->pos, "Can only index arrays and pointers");
    }
    rio_Operand index = rio_resolve_expr_rvalue(expr->index.index);
    if (!(rio_is_integer_type(index.type))) {
        rio_fatal_error(expr->pos, "Index must have integer type");
    }
    return rio_operand_lvalue(operand.type->base);
}

rio_Operand rio_resolve_expr_cast(rio_Expr (*expr)) {
    assert((expr->kind) == (RIO_EXPR_CAST));
    rio_Type (*type) = rio_resolve_typespec(expr->cast.type);
    rio_Operand operand = rio_resolve_expr_rvalue(expr->cast.expr);
    if (!(cast_operand(&(operand), type))) {
        rio_fatal_error(expr->pos, "Invalid type cast from %s to %s", rio_get_type_name(operand.type), rio_get_type_name(type));
    }
    return operand;
}

rio_Operand rio_resolve_expr_int(rio_Expr (*expr)) {
    assert((expr->kind) == (RIO_EXPR_INT));
    ullong int_max = rio_type_metrics[RIO_CMPL_TYPE_INT].max;
    ullong uint_max = rio_type_metrics[RIO_CMPL_TYPE_UINT].max;
    ullong long_max = rio_type_metrics[RIO_CMPL_TYPE_LONG].max;
    ullong ulong_max = rio_type_metrics[RIO_CMPL_TYPE_ULONG].max;
    ullong llong_max = rio_type_metrics[RIO_CMPL_TYPE_LLONG].max;
    ullong val = expr->int_lit.val;
    rio_Operand operand = rio_operand_const(rio_type_ullong, (rio_Val){.ull = val});
    rio_Type (*type) = rio_type_ullong;
    if ((expr->int_lit.mod) == (RIO_MOD_NONE)) {
        bool overflow = false;
        switch (expr->int_lit.suffix) {
        case RIO_SUFFIX_NONE: {
            type = rio_type_int;
            if ((val) > (int_max)) {
                type = rio_type_long;
                if ((val) > (long_max)) {
                    type = rio_type_llong;
                    overflow = (val) > (llong_max);
                }
            }
            break;
        }
        case RIO_SUFFIX_U: {
            type = rio_type_uint;
            if ((val) > (uint_max)) {
                type = rio_type_ulong;
                if ((val) > (ulong_max)) {
                    type = rio_type_ullong;
                }
            }
            break;
        }
        case RIO_SUFFIX_L: {
            type = rio_type_long;
            if ((val) > (long_max)) {
                type = rio_type_llong;
                overflow = (val) > (llong_max);
            }
            break;
        }
        case RIO_SUFFIX_UL: {
            type = rio_type_ulong;
            if ((val) > (ulong_max)) {
                type = rio_type_ullong;
            }
            break;
        }
        case RIO_SUFFIX_LL: {
            type = rio_type_llong;
            overflow = (val) > (llong_max);
            break;
        }
        case RIO_SUFFIX_ULL: {
            type = rio_type_ullong;
            break;
        }
        default: {
            assert(0);
            break;
        }
        }
        if (overflow) {
            rio_fatal_error(expr->pos, "Integer literal overflow");
        }
    } else {
        switch (expr->int_lit.suffix) {
        case RIO_SUFFIX_NONE: {
            type = rio_type_int;
            if ((val) > (int_max)) {
                type = rio_type_uint;
                if ((val) > (uint_max)) {
                    type = rio_type_long;
                    if ((val) > (long_max)) {
                        type = rio_type_ulong;
                        if ((val) > (ulong_max)) {
                            type = rio_type_llong;
                            if ((val) > (llong_max)) {
                                type = rio_type_ullong;
                            }
                        }
                    }
                }
            }
            break;
        }
        case RIO_SUFFIX_U: {
            type = rio_type_uint;
            if ((val) > (uint_max)) {
                type = rio_type_ulong;
                if ((val) > (ulong_max)) {
                    type = rio_type_ullong;
                }
            }
            break;
        }
        case RIO_SUFFIX_L: {
            type = rio_type_long;
            if ((val) > (long_max)) {
                type = rio_type_ulong;
                if ((val) > (ulong_max)) {
                    type = rio_type_llong;
                    if ((val) > (llong_max)) {
                        type = rio_type_ullong;
                    }
                }
            }
            break;
        }
        case RIO_SUFFIX_UL: {
            type = rio_type_ulong;
            if ((val) > (ulong_max)) {
                type = rio_type_ullong;
            }
            break;
        }
        case RIO_SUFFIX_LL: {
            type = rio_type_llong;
            if ((val) > (llong_max)) {
                type = rio_type_ullong;
            }
            break;
        }
        case RIO_SUFFIX_ULL: {
            type = rio_type_ullong;
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

rio_Operand rio_resolve_expr_modify(rio_Expr (*expr)) {
    rio_Operand operand = rio_resolve_expr(expr->modify.expr);
    rio_Type (*type) = operand.type;
    rio_complete_type(type);
    if (!(operand.is_lvalue)) {
        rio_fatal_error(expr->pos, "Cannot modify non-lvalue");
    }
    if (type->nonmodifiable) {
        rio_fatal_error(expr->pos, "Cannot modify non-modifiable type");
    }
    if (!(((rio_is_integer_type(type)) || ((type->kind) == (RIO_CMPL_TYPE_PTR))))) {
        rio_fatal_error(expr->pos, "%s only valid for integer and pointer types", rio_token_kind_name(expr->modify.op));
    }
    return rio_operand_rvalue(type);
}

rio_Operand rio_resolve_expected_expr(rio_Expr (*expr), rio_Type (*expected_type)) {
    rio_Operand result = {0};
    switch (expr->kind) {
    case RIO_EXPR_PAREN: {
        result = rio_resolve_expected_expr(expr->paren.expr, expected_type);
        break;
    }
    case RIO_EXPR_INT: {
        result = rio_resolve_expr_int(expr);
        break;
    }
    case RIO_EXPR_FLOAT: {
        result = rio_operand_const(((expr->float_lit.suffix) == (RIO_SUFFIX_D) ? rio_type_double : rio_type_float), (rio_Val){0});
        break;
    }
    case RIO_EXPR_STR: {
        result = rio_operand_rvalue(rio_type_array(rio_type_char, (strlen(expr->str_lit.val)) + (1)));
        break;
    }
    case RIO_EXPR_NAME: {
        result = rio_resolve_expr_name(expr);
        rio_set_resolved_sym(expr, rio_resolve_name(expr->name));
        break;
    }
    case RIO_EXPR_CAST: {
        result = rio_resolve_expr_cast(expr);
        break;
    }
    case RIO_EXPR_CALL: {
        result = rio_resolve_expr_call(expr);
        break;
    }
    case RIO_EXPR_INDEX: {
        result = rio_resolve_expr_index(expr);
        break;
    }
    case RIO_EXPR_FIELD: {
        result = rio_resolve_expr_field(expr);
        break;
    }
    case RIO_EXPR_COMPOUND: {
        result = rio_resolve_expr_compound(expr, expected_type);
        break;
    }
    case RIO_EXPR_UNARY: {
        if ((expr->unary.op) == (RIO_TOKEN_AND)) {
            rio_Operand operand = {0};
            if ((expected_type) && (rio_is_ptr_type(expected_type))) {
                operand = rio_resolve_expected_expr(expr->unary.expr, expected_type->base);
            } else {
                operand = rio_resolve_expr(expr->unary.expr);
            }
            if (!(operand.is_lvalue)) {
                rio_fatal_error(expr->pos, "Cannot take address of non-lvalue");
            }
            result = rio_operand_rvalue(rio_type_ptr(operand.type));
        } else {
            result = rio_resolve_expr_unary(expr);
        }
        break;
    }
    case RIO_EXPR_BINARY: {
        result = rio_resolve_expr_binary(expr);
        break;
    }
    case RIO_EXPR_TERNARY: {
        result = rio_resolve_expr_ternary(expr, expected_type);
        break;
    }
    case RIO_EXPR_SIZEOF_EXPR: {
        {
            if ((expr->sizeof_expr->kind) == (RIO_EXPR_NAME)) {
                rio_Sym (*sym) = rio_resolve_name(expr->sizeof_expr->name);
                if ((sym) && ((sym->kind) == (RIO_SYM_TYPE))) {
                    rio_complete_type(sym->type);
                    result = rio_operand_const(rio_type_usize, (rio_Val){.ull = rio_type_sizeof(sym->type)});
                    rio_set_resolved_type(expr->sizeof_expr, sym->type);
                    rio_set_resolved_sym(expr->sizeof_expr, sym);
                    break;
                }
            }
            rio_Type (*type) = rio_resolve_expr(expr->sizeof_expr).type;
            rio_complete_type(type);
            result = rio_operand_const(rio_type_usize, (rio_Val){.ull = rio_type_sizeof(type)});
        }
        break;
    }
    case RIO_EXPR_SIZEOF_TYPE: {
        {
            rio_Type (*type) = rio_resolve_typespec(expr->sizeof_type);
            rio_complete_type(type);
            result = rio_operand_const(rio_type_usize, (rio_Val){.ull = rio_type_sizeof(type)});
        }
        break;
    }
    case RIO_EXPR_ALIGNOF_EXPR: {
        {
            if ((expr->sizeof_expr->kind) == (RIO_EXPR_NAME)) {
                rio_Sym (*sym) = rio_resolve_name(expr->alignof_expr->name);
                if ((sym) && ((sym->kind) == (RIO_SYM_TYPE))) {
                    rio_complete_type(sym->type);
                    result = rio_operand_const(rio_type_usize, (rio_Val){.ull = rio_type_alignof(sym->type)});
                    rio_set_resolved_type(expr->alignof_expr, sym->type);
                    rio_set_resolved_sym(expr->alignof_expr, sym);
                    break;
                }
            }
            rio_Type (*type) = rio_resolve_expr(expr->alignof_expr).type;
            rio_complete_type(type);
            result = rio_operand_const(rio_type_usize, (rio_Val){.ull = rio_type_alignof(type)});
        }
        break;
    }
    case RIO_EXPR_ALIGNOF_TYPE: {
        {
            rio_Type (*type) = rio_resolve_typespec(expr->alignof_type);
            rio_complete_type(type);
            result = rio_operand_const(rio_type_usize, (rio_Val){.ull = rio_type_alignof(type)});
        }
        break;
    }
    case RIO_EXPR_TYPEOF_TYPE: {
        {
            rio_Type (*type) = rio_resolve_typespec(expr->typeof_type);
            result = rio_operand_const(rio_type_ullong, (rio_Val){.ull = type->typeid});
        }
        break;
    }
    case RIO_EXPR_TYPEOF_EXPR: {
        {
            if ((expr->typeof_expr->kind) == (RIO_EXPR_NAME)) {
                rio_Sym (*sym) = rio_resolve_name(expr->typeof_expr->name);
                if ((sym) && ((sym->kind) == (RIO_SYM_TYPE))) {
                    result = rio_operand_const(rio_type_ullong, (rio_Val){.ull = sym->type->typeid});
                    rio_set_resolved_type(expr->typeof_expr, sym->type);
                    rio_set_resolved_sym(expr->typeof_expr, sym);
                    break;
                }
            }
            rio_Type (*type) = rio_resolve_expr(expr->typeof_expr).type;
            result = rio_operand_const(rio_type_ullong, (rio_Val){.ull = type->typeid});
        }
        break;
    }
    case RIO_EXPR_OFFSETOF: {
        {
            rio_Type (*type) = rio_resolve_typespec(expr->offsetof_field.type);
            rio_complete_type(type);
            if (((type->kind) != (RIO_CMPL_TYPE_STRUCT)) && ((type->kind) != (RIO_CMPL_TYPE_UNION))) {
                rio_fatal_error(expr->pos, "offsetof can only be used with struct/union types");
            }
            int field = rio_aggregate_item_field_index(type, expr->offsetof_field.name);
            if ((field) < (0)) {
                rio_fatal_error(expr->pos, "No field \'%s\' in type", expr->offsetof_field.name);
            }
            result = rio_operand_const(rio_type_usize, (rio_Val){.ull = type->aggregate.fields[field].offset});
        }
        break;
    }
    case RIO_EXPR_MODIFY: {
        result = rio_resolve_expr_modify(expr);
        break;
    }
    default: {
        assert(0);
        result = rio_operand_null;
        break;
    }
    }
    rio_set_resolved_type(expr, result.type);
    return result;
}

rio_Operand rio_resolve_const_expr(rio_Expr (*expr)) {
    rio_Operand operand = rio_resolve_expr(expr);
    if (!(operand.is_const)) {
        rio_fatal_error(expr->pos, "Expected constant expression");
    }
    return operand;
}

rio_Map rio_decl_note_names;
void rio_init_builtin_syms(void) {
    assert(rio_current_package);
    rio_sym_global_type("void", rio_type_void);
    rio_sym_global_type("bool", rio_type_bool);
    rio_sym_global_type("char", rio_type_char);
    rio_sym_global_type("schar", rio_type_schar);
    rio_sym_global_type("uchar", rio_type_uchar);
    rio_sym_global_type("short", rio_type_short);
    rio_sym_global_type("ushort", rio_type_ushort);
    rio_sym_global_type("int", rio_type_int);
    rio_sym_global_type("uint", rio_type_uint);
    rio_sym_global_type("long", rio_type_long);
    rio_sym_global_type("ulong", rio_type_ulong);
    rio_sym_global_type("llong", rio_type_llong);
    rio_sym_global_type("ullong", rio_type_ullong);
    rio_sym_global_type("float", rio_type_float);
    rio_sym_global_type("double", rio_type_double);
}

void rio_add_package_decls(rio_Package (*package)) {
    for (size_t i = 0; (i) < (package->num_decls); (i)++) {
        rio_Decl (*decl) = package->decls[i];
        if ((decl->kind) == (RIO_DECL_NOTE)) {
            if (!(rio_map_get(&(rio_decl_note_names), decl->note.name))) {
                rio_warning(decl->pos, "Unknown declaration #directive \'%s\'", decl->note.name);
            }
            if ((decl->note.name) == (rio_declare_note_name)) {
                if ((decl->note.num_args) != (1)) {
                    rio_fatal_error(decl->pos, "#declare_note takes 1 argument");
                }
                rio_Expr (*arg) = decl->note.args[0].expr;
                if ((arg->kind) != (RIO_EXPR_NAME)) {
                    rio_fatal_error(decl->pos, "#declare_note argument must be name");
                }
                rio_map_put(&(rio_decl_note_names), arg->name, (void *)(1));
            } else if ((decl->note.name) == (rio_static_assert_name)) {
                if (!(rio_flag_lazy)) {
                    rio_resolve_static_assert(decl->note);
                }
            }
        } else if ((decl->kind) == (RIO_DECL_IMPORT)) {
        } else {
            rio_sym_global_decl(decl);
        }
    }
}

bool rio_is_package_dir(char const ((*search_path)), char const ((*package_path))) {
    char (path[MAX_PATH]) = {0};
    rio_path_copy(path, search_path);
    rio_path_join(path, package_path);
    rio_DirListIter iter = {0};
    for (rio_dir_list(&(iter), path); iter.valid; rio_dir_list_next(&(iter))) {
        char const ((*ext)) = rio_path_ext(iter.name);
        if (((ext) != (iter.name)) && (!(strcmp(ext, "rio")))) {
            rio_dir_list_free(&(iter));
            return true;
        }
    }
    return false;
}

bool rio_copy_package_full_path(char (dest[MAX_PATH]), char const ((*package_path))) {
    for (int i = 0; (i) < (rio_num_package_search_paths); (i)++) {
        if (rio_is_package_dir(rio_package_search_paths[i], package_path)) {
            rio_path_copy(dest, rio_package_search_paths[i]);
            rio_path_join(dest, package_path);
            return true;
        }
    }
    return false;
}

rio_Package (*rio_import_package(char const ((*package_path)))) {
    package_path = rio_str_intern(package_path);
    rio_Package (*package) = rio_map_get(&(rio_package_map), package_path);
    if (!(package)) {
        package = rio_xcalloc(1, sizeof(rio_Package));
        package->path = package_path;
        if (rio_flag_verbose) {
            printf("Importing %s\n", package_path);
        }
        char (full_path[MAX_PATH]) = {0};
        if (!(rio_copy_package_full_path(full_path, package_path))) {
            return NULL;
        }
        strcpy(package->full_path, full_path);
        rio_add_package(package);
        rio_compile_package(package);
    }
    return package;
}

void rio_import_all_package_symbols(rio_Package (*package)) {
    char const ((*main_name)) = rio_str_intern("main");
    for (size_t i = 0; (i) < (rio_buf_len(package->syms)); (i)++) {
        if (((package->syms[i]->home_package) == (package)) && ((package->syms[i]->name) != (main_name))) {
            rio_sym_global_put(package->syms[i]->name, package->syms[i]);
        }
    }
}

void rio_import_package_symbols(rio_Decl (*decl), rio_Package (*package)) {
    for (size_t i = 0; (i) < (decl->import_decl.num_items); (i)++) {
        rio_ImportItem item = decl->import_decl.items[i];
        rio_Sym (*sym) = rio_get_package_sym(package, item.name);
        if (!(sym)) {
            rio_fatal_error(decl->pos, "Symbol \'%s\' does not exist in package \'%s\'", item.name, package->path);
        }
        rio_sym_global_put((item.rename ? item.rename : item.name), sym);
    }
}

void rio_process_package_imports(rio_Package (*package)) {
    for (size_t i = 0; (i) < (package->num_decls); (i)++) {
        rio_Decl (*decl) = package->decls[i];
        if ((decl->kind) == (RIO_DECL_NOTE)) {
            if ((decl->note.name) == (rio_always_name)) {
                package->always_reachable = true;
            }
        } else if ((decl->kind) == (RIO_DECL_IMPORT)) {
            char (*path_buf) = NULL;
            if (decl->import_decl.is_relative) {
                rio_buf_printf(&(path_buf), "%s/", package->path);
            }
            for (size_t k = 0; (k) < (decl->import_decl.num_names); (k)++) {
                if (!(rio_str_islower(decl->import_decl.names[k]))) {
                    rio_fatal_error(decl->pos, "Import name must be lower case: \'%s\'", decl->import_decl.names[k]);
                }
                rio_buf_printf(&(path_buf), "%s%s", ((k) == (0) ? "" : "/"), decl->import_decl.names[k]);
            }
            rio_Package (*imported_package) = rio_import_package(path_buf);
            if (!(imported_package)) {
                rio_fatal_error(decl->pos, "Failed to import package \'%s\'", path_buf);
            }
            rio_buf_free((void (**))(&(path_buf)));
            rio_import_package_symbols(decl, imported_package);
            if (decl->import_decl.import_all) {
                rio_import_all_package_symbols(imported_package);
            }
            char const ((*sym_name)) = (decl->name ? decl->name : decl->import_decl.names[(decl->import_decl.num_names) - (1)]);
            rio_Sym (*sym) = rio_sym_new(RIO_SYM_PACKAGE, sym_name, decl);
            sym->package = imported_package;
            rio_sym_global_put(sym_name, sym);
        }
    }
}

bool rio_parse_package(rio_Package (*package)) {
    rio_Decl (*(*decls)) = NULL;
    rio_DirListIter iter = {0};
    for (rio_dir_list(&(iter), package->full_path); iter.valid; rio_dir_list_next(&(iter))) {
        if (((iter.is_dir) || ((iter.name[0]) == ('_'))) || ((iter.name[0]) == ('.'))) {
            continue;
        }
        char (name[MAX_PATH]) = {0};
        rio_path_copy(name, iter.name);
        char (*ext) = rio_path_ext(name);
        if (((ext) == (name)) || (strcmp(ext, "rio"))) {
            continue;
        }
        ext[-(1)] = 0;
        if (rio_is_excluded_target_filename(name)) {
            continue;
        }
        char (path[MAX_PATH]) = {0};
        rio_path_copy(path, iter.base);
        rio_path_join(path, iter.name);
        rio_path_absolute(path);
        char const ((*code)) = rio_read_file(path);
        if (!(code)) {
            rio_fatal_error((rio_SrcPos){.name = path}, "Failed to read source file");
        }
        rio_init_stream(rio_str_intern(path), code);
        rio_Decls (*file_decls) = rio_parse_decls();
        for (size_t i = 0; (i) < (file_decls->num_decls); (i)++) {
            rio_buf_push((void (**))(&(decls)), &(file_decls->decls[i]), sizeof(file_decls->decls[i]));
        }
    }
    package->decls = decls;
    package->num_decls = (int)(rio_buf_len(decls));
    return (package) != (NULL);
}

bool rio_compile_package(rio_Package (*package)) {
    if (!(rio_parse_package(package))) {
        return false;
    }
    rio_Package (*old_package) = rio_enter_package(package);
    if ((rio_buf_len(rio_package_list)) == (1)) {
        rio_init_builtin_syms();
    }
    if (rio_builtin_package) {
        rio_import_all_package_symbols(rio_builtin_package);
    }
    rio_add_package_decls(package);
    rio_process_package_imports(package);
    rio_leave_package(old_package);
    return true;
}

void rio_resolve_package_syms(rio_Package (*package)) {
    rio_Package (*old_package) = rio_enter_package(package);
    for (size_t i = 0; (i) < (rio_buf_len(package->syms)); (i)++) {
        if ((package->syms[i]->home_package) == (package)) {
            rio_resolve_sym(package->syms[i]);
        }
    }
    rio_leave_package(old_package);
}

void rio_finalize_reachable_syms(void) {
    if (rio_flag_verbose) {
        printf("Finalizing reachable symbols\n");
    }
    size_t prev_num_reachable = 0;
    ullong num_reachable = rio_buf_len(rio_reachable_syms);
    for (size_t i = 0; (i) < (num_reachable); (i)++) {
        rio_finalize_sym(rio_reachable_syms[i]);
        if ((i) == ((num_reachable) - (1))) {
            if (rio_flag_verbose) {
                printf("New reachable symbols:");
                for (size_t k = prev_num_reachable; (k) < (num_reachable); (k)++) {
                    printf(" %s/%s", rio_reachable_syms[k]->home_package->path, rio_reachable_syms[k]->name);
                }
                printf("\n");
            }
            prev_num_reachable = num_reachable;
            num_reachable = rio_buf_len(rio_reachable_syms);
        }
    }
}

char const ((*(rio_os_names[RIO_NUM_OSES]))) = {[RIO_OS_WIN32] = "win32", [RIO_OS_LINUX] = "linux", [RIO_OS_OSX] = "osx"};
char const ((*(rio_arch_names[RIO_NUM_ARCHES]))) = {[RIO_ARCH_X64] = "x64", [RIO_ARCH_X86] = "x86"};
int rio_target_os;
int rio_target_arch;
int rio_get_os(char const ((*name))) {
    for (int i = 0; (i) < (RIO_NUM_OSES); (i)++) {
        if ((strcmp(rio_os_names[i], name)) == (0)) {
            return i;
        }
    }
    return -(1);
}

int rio_get_arch(char const ((*name))) {
    for (int i = 0; (i) < (RIO_NUM_ARCHES); (i)++) {
        if ((strcmp(rio_arch_names[i], name)) == (0)) {
            return i;
        }
    }
    return -(1);
}

void rio_init_default_type_metrics(rio_TypeMetrics (metrics[RIO_NUM_CMPL_TYPE_KINDS])) {
    metrics[RIO_CMPL_TYPE_BOOL] = (rio_TypeMetrics){.size = 1, .align = 1};
    metrics[RIO_CMPL_TYPE_CHAR] = (rio_TypeMetrics){.size = 1, .align = 1, .max = 0x7f, .sign = true};
    metrics[RIO_CMPL_TYPE_SCHAR] = (rio_TypeMetrics){.size = 1, .align = 1, .max = 0x7f, .sign = true};
    metrics[RIO_CMPL_TYPE_UCHAR] = (rio_TypeMetrics){.size = 1, .align = 1, .max = 0xff};
    metrics[RIO_CMPL_TYPE_SHORT] = (rio_TypeMetrics){.size = 2, .align = 2, .max = 0x7fff, .sign = true};
    metrics[RIO_CMPL_TYPE_USHORT] = (rio_TypeMetrics){.size = 2, .align = 2, .max = 0xffff};
    metrics[RIO_CMPL_TYPE_INT] = (rio_TypeMetrics){.size = 4, .align = 4, .max = 0x7fffffff, .sign = true};
    metrics[RIO_CMPL_TYPE_UINT] = (rio_TypeMetrics){.size = 4, .align = 4, .max = 0xffffffff};
    metrics[RIO_CMPL_TYPE_LLONG] = (rio_TypeMetrics){.size = 8, .align = 8, .max = 0x7fffffffffffffff, .sign = true};
    metrics[RIO_CMPL_TYPE_ULLONG] = (rio_TypeMetrics){.size = 8, .align = 8, .max = 0xffffffffffffffff};
    metrics[RIO_CMPL_TYPE_FLOAT] = (rio_TypeMetrics){.size = 4, .align = 4};
    metrics[RIO_CMPL_TYPE_DOUBLE] = (rio_TypeMetrics){.size = 8, .align = 8};
}

rio_TypeMetrics (rio_win32_x86_metrics[RIO_NUM_CMPL_TYPE_KINDS]) = {[RIO_CMPL_TYPE_PTR] = {.size = 4, .align = 4}, [RIO_CMPL_TYPE_LONG] = {.size = 4, .align = 4, .max = 0x7fffffff, .sign = true}, [RIO_CMPL_TYPE_ULONG] = {.size = 4, .align = 4, .max = 0x7fffffff, .sign = true}};
rio_TypeMetrics (rio_win32_x64_metrics[RIO_NUM_CMPL_TYPE_KINDS]) = {[RIO_CMPL_TYPE_PTR] = {.size = 8, .align = 8}, [RIO_CMPL_TYPE_LONG] = {.size = 4, .align = 4, .max = 0x7fffffff, .sign = true}, [RIO_CMPL_TYPE_ULONG] = {.size = 4, .align = 4, .max = 0x7fffffff, .sign = true}};
rio_TypeMetrics (rio_ilp32_metrics[RIO_NUM_CMPL_TYPE_KINDS]) = {[RIO_CMPL_TYPE_PTR] = {.size = 4, .align = 4}, [RIO_CMPL_TYPE_LONG] = {.size = 4, .align = 4, .max = 0x7fffffff, .sign = true}, [RIO_CMPL_TYPE_ULONG] = {.size = 4, .align = 4, .max = 0x7fffffff, .sign = true}};
rio_TypeMetrics (rio_lp64_metrics[RIO_NUM_CMPL_TYPE_KINDS]) = {[RIO_CMPL_TYPE_PTR] = {.size = 8, .align = 8}, [RIO_CMPL_TYPE_LONG] = {.size = 8, .align = 8, .max = 0x7fffffffffffffff, .sign = true}, [RIO_CMPL_TYPE_ULONG] = {.size = 8, .align = 8, .max = 0xffffffffffffffff, .sign = true}};
void rio_init_target(void) {
    rio_type_metrics = NULL;
    switch (rio_target_os) {
    case RIO_OS_WIN32: {
        switch (rio_target_arch) {
        case RIO_ARCH_X86: {
            rio_type_metrics = rio_win32_x86_metrics;
            break;
        }
        case RIO_ARCH_X64: {
            rio_type_metrics = rio_win32_x64_metrics;
            break;
        }
        default: {
            break;
        }
        }
        break;
    }
    case RIO_OS_LINUX: {
        switch (rio_target_arch) {
        case RIO_ARCH_X86: {
            rio_type_metrics = rio_ilp32_metrics;
            break;
        }
        case RIO_ARCH_X64: {
            rio_type_metrics = rio_lp64_metrics;
            break;
        }
        default: {
            break;
        }
        }
        break;
    }
    case RIO_OS_OSX: {
        switch (rio_target_arch) {
        case RIO_ARCH_X64: {
            rio_type_metrics = rio_lp64_metrics;
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
    if (!(rio_type_metrics)) {
        printf("Unsupported os/arch combination: %s/%s\n", rio_os_names[rio_target_os], rio_arch_names[rio_target_arch]);
        exit(1);
    }
    rio_init_default_type_metrics(rio_type_metrics);
    if ((rio_type_metrics[RIO_CMPL_TYPE_PTR].size) == (4)) {
        rio_type_uintptr = rio_type_uint;
        rio_type_usize = rio_type_uint;
        rio_type_ssize = rio_type_int;
    } else {
        assert((rio_type_metrics[RIO_CMPL_TYPE_PTR].size) == (8));
        rio_type_uintptr = rio_type_ullong;
        rio_type_usize = rio_type_ullong;
        rio_type_ssize = rio_type_llong;
    }
}

bool rio_is_excluded_target_filename(char const ((*name))) {
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
    int os1 = rio_get_os(str1);
    int arch1 = rio_get_arch(str1);
    int os2 = rio_get_os(str2);
    int arch2 = rio_get_arch(str2);
    if (((arch1) != (-(1))) && ((os2) != (-(1)))) {
        return ((arch1) != (rio_target_arch)) || ((os2) != (rio_target_os));
    } else if (((arch2) != (-(1))) && ((os1) != (-(1)))) {
        return ((arch2) != (rio_target_arch)) || ((os1) != (rio_target_os));
    } else if ((os1) != (-(1))) {
        return (os1) != (rio_target_os);
    } else if ((arch1) != (-(1))) {
        return (arch1) != (rio_target_arch);
    } else {
        return false;
    }
}

rio_TypeMetrics (*rio_type_metrics);
rio_Type (*rio_type_void) = &((rio_Type){RIO_CMPL_TYPE_VOID});
rio_Type (*rio_type_bool) = &((rio_Type){RIO_CMPL_TYPE_BOOL});
rio_Type (*rio_type_char) = &((rio_Type){RIO_CMPL_TYPE_CHAR});
rio_Type (*rio_type_uchar) = &((rio_Type){RIO_CMPL_TYPE_UCHAR});
rio_Type (*rio_type_schar) = &((rio_Type){RIO_CMPL_TYPE_SCHAR});
rio_Type (*rio_type_short) = &((rio_Type){RIO_CMPL_TYPE_SHORT});
rio_Type (*rio_type_ushort) = &((rio_Type){RIO_CMPL_TYPE_USHORT});
rio_Type (*rio_type_int) = &((rio_Type){RIO_CMPL_TYPE_INT});
rio_Type (*rio_type_uint) = &((rio_Type){RIO_CMPL_TYPE_UINT});
rio_Type (*rio_type_long) = &((rio_Type){RIO_CMPL_TYPE_LONG});
rio_Type (*rio_type_ulong) = &((rio_Type){RIO_CMPL_TYPE_ULONG});
rio_Type (*rio_type_llong) = &((rio_Type){RIO_CMPL_TYPE_LLONG});
rio_Type (*rio_type_ullong) = &((rio_Type){RIO_CMPL_TYPE_ULLONG});
rio_Type (*rio_type_float) = &((rio_Type){RIO_CMPL_TYPE_FLOAT});
rio_Type (*rio_type_double) = &((rio_Type){RIO_CMPL_TYPE_DOUBLE});
int rio_next_typeid = 1;
rio_Type (*rio_type_uintptr);
rio_Type (*rio_type_usize);
rio_Type (*rio_type_ssize);
rio_Map rio_typeid_map;
rio_Type (*rio_get_type_from_typeid(int typeid)) {
    if ((typeid) == (0)) {
        return NULL;
    }
    return rio_map_get(&(rio_typeid_map), (void *)((uintptr_t)(typeid)));
}

void rio_register_typeid(rio_Type (*type)) {
    rio_map_put(&(rio_typeid_map), (void *)((uintptr_t)(type->typeid)), type);
}

rio_Type (*rio_type_alloc(TypeKind kind)) {
    rio_Type (*type) = rio_xcalloc(1, sizeof(rio_Type));
    type->kind = kind;
    type->typeid = (rio_next_typeid)++;
    rio_register_typeid(type);
    return type;
}

bool rio_is_ptr_type(rio_Type (*type)) {
    return (type->kind) == (RIO_CMPL_TYPE_PTR);
}

bool rio_is_func_type(rio_Type (*type)) {
    return (type->kind) == (RIO_CMPL_TYPE_FUNC);
}

bool rio_is_ptr_like_type(rio_Type (*type)) {
    return ((type->kind) == (RIO_CMPL_TYPE_PTR)) || ((type->kind) == (RIO_CMPL_TYPE_FUNC));
}

bool rio_is_const_type(rio_Type (*type)) {
    return (type->kind) == (RIO_CMPL_TYPE_CONST);
}

bool rio_is_array_type(rio_Type (*type)) {
    return (type->kind) == (RIO_CMPL_TYPE_ARRAY);
}

bool rio_is_incomplete_array_type(rio_Type (*type)) {
    return (rio_is_array_type(type)) && ((type->num_elems) == (0));
}

bool rio_is_integer_type(rio_Type (*type)) {
    return ((RIO_CMPL_TYPE_BOOL) <= (type->kind)) && ((type->kind) <= (RIO_CMPL_TYPE_ENUM));
}

bool rio_is_floating_type(rio_Type (*type)) {
    return ((RIO_CMPL_TYPE_FLOAT) <= (type->kind)) && ((type->kind) <= (RIO_CMPL_TYPE_DOUBLE));
}

bool rio_is_arithmetic_type(rio_Type (*type)) {
    return ((RIO_CMPL_TYPE_BOOL) <= (type->kind)) && ((type->kind) <= (RIO_CMPL_TYPE_DOUBLE));
}

bool rio_is_scalar_type(rio_Type (*type)) {
    return ((RIO_CMPL_TYPE_BOOL) <= (type->kind)) && ((type->kind) <= (RIO_CMPL_TYPE_FUNC));
}

bool rio_is_aggregate_type(rio_Type (*type)) {
    return ((type->kind) == (RIO_CMPL_TYPE_STRUCT)) || ((type->kind) == (RIO_CMPL_TYPE_UNION));
}

bool rio_is_signed_type(rio_Type (*type)) {
    switch (type->kind) {
    case RIO_CMPL_TYPE_CHAR: {
        return rio_type_metrics[RIO_CMPL_TYPE_CHAR].sign;
        break;
    }
    case RIO_CMPL_TYPE_SCHAR:
    case RIO_CMPL_TYPE_SHORT:
    case RIO_CMPL_TYPE_INT:
    case RIO_CMPL_TYPE_LONG:
    case RIO_CMPL_TYPE_LLONG: {
        return true;
        break;
    }
    default: {
        return false;
        break;
    }
    }
}

char const ((*(rio_type_names[RIO_NUM_CMPL_TYPE_KINDS]))) = {[RIO_CMPL_TYPE_VOID] = "void", [RIO_CMPL_TYPE_BOOL] = "bool", [RIO_CMPL_TYPE_CHAR] = "char", [RIO_CMPL_TYPE_SCHAR] = "schar", [RIO_CMPL_TYPE_UCHAR] = "uchar", [RIO_CMPL_TYPE_SHORT] = "short", [RIO_CMPL_TYPE_USHORT] = "ushort", [RIO_CMPL_TYPE_INT] = "int", [RIO_CMPL_TYPE_UINT] = "uint", [RIO_CMPL_TYPE_LONG] = "long", [RIO_CMPL_TYPE_ULONG] = "ulong", [RIO_CMPL_TYPE_LLONG] = "llong", [RIO_CMPL_TYPE_ULLONG] = "ullong", [RIO_CMPL_TYPE_FLOAT] = "float", [RIO_CMPL_TYPE_DOUBLE] = "double"};
int (rio_type_ranks[RIO_NUM_CMPL_TYPE_KINDS]) = {[RIO_CMPL_TYPE_BOOL] = 1, [RIO_CMPL_TYPE_CHAR] = 2, [RIO_CMPL_TYPE_SCHAR] = 2, [RIO_CMPL_TYPE_UCHAR] = 2, [RIO_CMPL_TYPE_SHORT] = 3, [RIO_CMPL_TYPE_USHORT] = 3, [RIO_CMPL_TYPE_INT] = 4, [RIO_CMPL_TYPE_UINT] = 4, [RIO_CMPL_TYPE_LONG] = 5, [RIO_CMPL_TYPE_ULONG] = 5, [RIO_CMPL_TYPE_LLONG] = 6, [RIO_CMPL_TYPE_ULLONG] = 6};
int rio_type_rank(rio_Type (*type)) {
    int rank = rio_type_ranks[type->kind];
    assert((rank) != (0));
    return rank;
}

rio_Type (*rio_unsigned_type(rio_Type (*type))) {
    switch (type->kind) {
    case RIO_CMPL_TYPE_BOOL: {
        return rio_type_bool;
        break;
    }
    case RIO_CMPL_TYPE_CHAR:
    case RIO_CMPL_TYPE_SCHAR:
    case RIO_CMPL_TYPE_UCHAR: {
        return rio_type_uchar;
        break;
    }
    case RIO_CMPL_TYPE_SHORT:
    case RIO_CMPL_TYPE_USHORT: {
        return rio_type_ushort;
        break;
    }
    case RIO_CMPL_TYPE_INT:
    case RIO_CMPL_TYPE_UINT: {
        return rio_type_uint;
        break;
    }
    case RIO_CMPL_TYPE_LONG:
    case RIO_CMPL_TYPE_ULONG: {
        return rio_type_ulong;
        break;
    }
    case RIO_CMPL_TYPE_LLONG:
    case RIO_CMPL_TYPE_ULLONG: {
        return rio_type_ullong;
        break;
    }
    default: {
        assert(0);
        return NULL;
        break;
    }
    }
}

size_t rio_type_sizeof(rio_Type (*type)) {
    assert((type->kind) > (RIO_CMPL_TYPE_COMPLETING));
    return type->size;
}

size_t rio_type_alignof(rio_Type (*type)) {
    assert((type->kind) > (RIO_CMPL_TYPE_COMPLETING));
    return type->align;
}

rio_Map rio_cached_ptr_types;
rio_Type (*rio_type_ptr(rio_Type (*base))) {
    rio_Type (*type) = rio_map_get(&(rio_cached_ptr_types), base);
    if (!(type)) {
        type = rio_type_alloc(RIO_CMPL_TYPE_PTR);
        type->size = rio_type_metrics[RIO_CMPL_TYPE_PTR].size;
        type->align = rio_type_metrics[RIO_CMPL_TYPE_PTR].align;
        type->base = base;
        rio_map_put(&(rio_cached_ptr_types), base, type);
    }
    return type;
}

rio_Map rio_cached_const_types;
rio_Type (*rio_type_const(rio_Type (*base))) {
    if ((base->kind) == (RIO_CMPL_TYPE_CONST)) {
        return base;
    }
    rio_Type (*type) = rio_map_get(&(rio_cached_const_types), base);
    if (!(type)) {
        rio_complete_type(base);
        type = rio_type_alloc(RIO_CMPL_TYPE_CONST);
        type->nonmodifiable = true;
        type->size = base->size;
        type->align = base->align;
        type->base = base;
        rio_map_put(&(rio_cached_const_types), base, type);
    }
    return type;
}

rio_Type (*rio_unqualify_type(rio_Type (*type))) {
    if ((type->kind) == (RIO_CMPL_TYPE_CONST)) {
        return type->base;
    } else {
        return type;
    }
}

rio_Map rio_cached_array_types;
rio_Type (*rio_type_array(rio_Type (*base), size_t num_elems)) {
    ullong hash = rio_hash_mix(rio_hash_ptr(base), rio_hash_uint64(num_elems));
    uint64_t key = (hash ? hash : 1);
    rio_CachedArrayType (*cached) = rio_map_get_from_uint64(&(rio_cached_array_types), key);
    for (rio_CachedArrayType (*it) = cached; it; it = it->next) {
        rio_Type (*type) = it->type;
        if (((type->base) == (base)) && ((type->num_elems) == (num_elems))) {
            return type;
        }
    }
    rio_complete_type(base);
    rio_Type (*type) = rio_type_alloc(RIO_CMPL_TYPE_ARRAY);
    type->nonmodifiable = base->nonmodifiable;
    type->size = (num_elems) * (rio_type_sizeof(base));
    type->align = rio_type_alignof(base);
    type->base = base;
    type->num_elems = num_elems;
    rio_CachedArrayType (*new_cached) = rio_xmalloc(sizeof(rio_CachedArrayType));
    new_cached->type = type;
    new_cached->next = cached;
    rio_map_put_from_uint64(&(rio_cached_array_types), key, new_cached);
    return type;
}

rio_Map rio_cached_func_types;
rio_Type (*rio_type_func(rio_Type (*(*params)), size_t num_params, rio_Type (*ret), bool has_varargs)) {
    ullong params_size = (num_params) * (sizeof(*(params)));
    ullong hash = rio_hash_mix(rio_hash_bytes(params, params_size), rio_hash_ptr(ret));
    uint64_t key = (hash ? hash : 1);
    rio_CachedFuncType (*cached) = rio_map_get_from_uint64(&(rio_cached_func_types), key);
    for (rio_CachedFuncType (*it) = cached; it; it = it->next) {
        rio_Type (*type) = it->type;
        if ((((type->function.num_params) == (num_params)) && ((type->function.ret) == (ret))) && ((type->function.has_varargs) == (has_varargs))) {
            if ((memcmp(type->function.params, params, params_size)) == (0)) {
                return type;
            }
        }
    }
    rio_Type (*type) = rio_type_alloc(RIO_CMPL_TYPE_FUNC);
    type->size = rio_type_metrics[RIO_CMPL_TYPE_PTR].size;
    type->align = rio_type_metrics[RIO_CMPL_TYPE_PTR].align;
    type->function.params = rio_memdup(params, params_size);
    type->function.num_params = num_params;
    type->function.has_varargs = has_varargs;
    type->function.ret = ret;
    rio_CachedFuncType (*new_cached) = rio_xmalloc(sizeof(rio_CachedFuncType));
    new_cached->type = type;
    new_cached->next = cached;
    rio_map_put_from_uint64(&(rio_cached_func_types), key, new_cached);
    return type;
}

bool rio_has_duplicate_fields(rio_Type (*type)) {
    for (size_t i = 0; (i) < (type->aggregate.num_fields); (i)++) {
        for (size_t j = (i) + (1); (j) < (type->aggregate.num_fields); (j)++) {
            if ((type->aggregate.fields[i].name) == (type->aggregate.fields[j].name)) {
                return true;
            }
        }
    }
    return false;
}

void rio_add_type_fields(rio_TypeField (*(*fields)), rio_Type (*type), size_t offset) {
    assert(((type->kind) == (RIO_CMPL_TYPE_STRUCT)) || ((type->kind) == (RIO_CMPL_TYPE_UNION)));
    for (size_t i = 0; (i) < (type->aggregate.num_fields); (i)++) {
        rio_TypeField (*field) = &(type->aggregate.fields[i]);
        rio_TypeField new_field = {field->name, field->type, (field->offset) + (offset)};
        rio_buf_push((void (**))(fields), &(new_field), sizeof(new_field));
    }
}

void rio_type_complete_struct(rio_Type (*type), rio_TypeField (*fields), size_t num_fields) {
    assert((type->kind) == (RIO_CMPL_TYPE_COMPLETING));
    type->kind = RIO_CMPL_TYPE_STRUCT;
    type->size = 0;
    type->align = 0;
    bool nonmodifiable = false;
    rio_TypeField (*new_fields) = {0};
    for (rio_TypeField (*it) = fields; (it) != ((fields) + (num_fields)); (it)++) {
        assert(rio_is_pow2(rio_type_alignof(it->type)));
        if (it->name) {
            it->offset = type->size;
            rio_buf_push((void (**))(&(new_fields)), it, sizeof(*(it)));
        } else {
            rio_add_type_fields(&(new_fields), it->type, type->size);
        }
        type->align = rio_max(type->align, rio_type_alignof(it->type));
        type->size = (rio_type_sizeof(it->type)) + (rio_align_up(type->size, rio_type_alignof(it->type)));
        nonmodifiable = (it->type->nonmodifiable) || (nonmodifiable);
    }
    type->size = rio_align_up(type->size, type->align);
    type->aggregate.fields = new_fields;
    type->aggregate.num_fields = rio_buf_len(new_fields);
    type->nonmodifiable = nonmodifiable;
}

void rio_type_complete_union(rio_Type (*type), rio_TypeField (*fields), size_t num_fields) {
    assert((type->kind) == (RIO_CMPL_TYPE_COMPLETING));
    type->kind = RIO_CMPL_TYPE_UNION;
    type->size = 0;
    type->align = 0;
    bool nonmodifiable = false;
    rio_TypeField (*new_fields) = {0};
    for (rio_TypeField (*it) = fields; (it) != ((fields) + (num_fields)); (it)++) {
        assert((it->type->kind) > (RIO_CMPL_TYPE_COMPLETING));
        if (it->name) {
            it->offset = type->size;
            rio_buf_push((void (**))(&(new_fields)), it, sizeof(*(it)));
        } else {
            rio_add_type_fields(&(new_fields), it->type, 0);
        }
        type->size = rio_max(type->size, rio_type_sizeof(it->type));
        type->align = rio_max(type->align, rio_type_alignof(it->type));
        nonmodifiable = (it->type->nonmodifiable) || (nonmodifiable);
    }
    type->size = rio_align_up(type->size, type->align);
    type->aggregate.fields = new_fields;
    type->aggregate.num_fields = rio_buf_len(new_fields);
    type->nonmodifiable = nonmodifiable;
}

rio_Type (*rio_type_incomplete(rio_Sym (*sym))) {
    rio_Type (*type) = rio_type_alloc(RIO_CMPL_TYPE_INCOMPLETE);
    type->sym = sym;
    return type;
}

rio_Type (*rio_type_enum(rio_Sym (*sym), rio_Type (*base))) {
    rio_Type (*type) = rio_type_alloc(RIO_CMPL_TYPE_ENUM);
    type->sym = sym;
    type->base = base;
    type->size = rio_type_int->size;
    type->align = rio_type_int->align;
    return type;
}

void rio_init_builtin_type(rio_Type (*type)) {
    type->typeid = (rio_next_typeid)++;
    rio_register_typeid(type);
    type->size = rio_type_metrics[type->kind].size;
    type->align = rio_type_metrics[type->kind].align;
}

void rio_init_builtin_types(void) {
    rio_init_builtin_type(rio_type_void);
    rio_init_builtin_type(rio_type_bool);
    rio_init_builtin_type(rio_type_char);
    rio_init_builtin_type(rio_type_uchar);
    rio_init_builtin_type(rio_type_schar);
    rio_init_builtin_type(rio_type_short);
    rio_init_builtin_type(rio_type_ushort);
    rio_init_builtin_type(rio_type_int);
    rio_init_builtin_type(rio_type_uint);
    rio_init_builtin_type(rio_type_long);
    rio_init_builtin_type(rio_type_ulong);
    rio_init_builtin_type(rio_type_llong);
    rio_init_builtin_type(rio_type_ullong);
    rio_init_builtin_type(rio_type_float);
    rio_init_builtin_type(rio_type_double);
}

int rio_aggregate_item_field_index(rio_Type (*type), char const ((*name))) {
    assert(rio_is_aggregate_type(type));
    for (size_t i = 0; (i) < (type->aggregate.num_fields); (i)++) {
        if ((type->aggregate.fields[i].name) == (name)) {
            return (int)(i);
        }
    }
    return -(1);
}

rio_Type (*rio_aggregate_item_field_type_from_index(rio_Type (*type), int index)) {
    assert(rio_is_aggregate_type(type));
    assert(((0) <= (index)) && ((index) < ((int)(type->aggregate.num_fields))));
    return type->aggregate.fields[index].type;
}

rio_Type (*rio_aggregate_item_field_type_from_name(rio_Type (*type), char const ((*name)))) {
    assert(rio_is_aggregate_type(type));
    int index = rio_aggregate_item_field_index(type, name);
    if ((index) < (0)) {
        return NULL;
    }
    return rio_aggregate_item_field_type_from_index(type, index);
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
