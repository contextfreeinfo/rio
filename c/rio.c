// This file contains code that can't easily be expressed in rio at present.
// Ideally, this file goes away sometime.

#if _MSC_VER >= 1900 || __STDC_VERSION__ >= 201112L
// Visual Studio 2015 supports enough C99/C11 features for us.
#else
#error "C11 support required or Visual Studio 2015 or later"
#endif

#define __USE_MINGW_ANSI_STDIO 1
#define _CRT_SECURE_NO_WARNINGS

#include <limits.h>
#include <stdbool.h>
#include <stdlib.h>

#ifndef MAX_PATH
#   if defined _MAX_PATH
#       define MAX_PATH _MAX_PATH
#   elif defined PATH_MAX
#       define MAX_PATH PATH_MAX
#   else
#       error "No suitable MAX_PATH surrogate"
#   endif
#endif

struct rio_Operand;
struct rio_Type;

bool cast_operand(struct rio_Operand *operand, struct rio_Type *type);

#ifdef _WIN32
#   include "out_rio_win32.c"
#else
#   include "out_rio_linux.c"
#endif

#define CASE(k, t) \
    case k: \
        switch (type->kind) { \
        case rio_CompilerTypeKind_Bool: \
            operand->val.b = (bool)operand->val.t; \
            break; \
        case rio_CompilerTypeKind_Char: \
            operand->val.c = (char)operand->val.t; \
            break; \
        case rio_CompilerTypeKind_UChar: \
            operand->val.uc = (unsigned char)operand->val.t; \
            break; \
        case rio_CompilerTypeKind_SChar: \
            operand->val.sc = (signed char)operand->val.t; \
            break; \
        case rio_CompilerTypeKind_Short: \
            operand->val.s = (short)operand->val.t; \
            break; \
        case rio_CompilerTypeKind_UShort: \
            operand->val.us = (unsigned short)operand->val.t; \
            break; \
        case rio_CompilerTypeKind_Int: \
        case rio_CompilerTypeKind_Enum: \
            operand->val.i = (int)operand->val.t; \
            break; \
        case rio_CompilerTypeKind_UInt: \
            operand->val.u = (unsigned)operand->val.t; \
            break; \
        case rio_CompilerTypeKind_Long: \
            operand->val.l = (long)operand->val.t; \
            break; \
        case rio_CompilerTypeKind_ULong: \
            operand->val.ul = (unsigned long)operand->val.t; \
            break; \
        case rio_CompilerTypeKind_LLong: \
            operand->val.ll = (long long)operand->val.t; \
            break; \
        case rio_CompilerTypeKind_ULLong: \
            operand->val.ull = (unsigned long long)operand->val.t; \
            break; \
        case rio_CompilerTypeKind_Ptr: \
        case rio_CompilerTypeKind_Ref: \
            operand->val.p = (uintptr_t)operand->val.t; \
            break; \
        case rio_CompilerTypeKind_Float: \
        case rio_CompilerTypeKind_Double: \
            break; \
        default: \
            operand->is_const = false; \
            break; \
        } \
        break;

bool cast_operand(rio_Operand *operand, rio_Type *type) {
    rio_Type *qual_type = type;
    type = rio_unqualify_type(type);
    operand->type = rio_unqualify_type(operand->type);
    if (operand->type != type) {
        if (!rio_is_castable(operand, type)) {
            return false;
        }
        if (operand->is_const) {
            if (rio_is_floating_type(operand->type)) {
                operand->is_const = !rio_is_integer_type(type);
            } else {
                if (type->kind == rio_CompilerTypeKind_Enum) {
                    type = type->base;
                }
                rio_Type *operand_type = operand->type;
                if (operand_type->kind == rio_CompilerTypeKind_Enum) {
                    operand_type = operand_type->base;
                }
                switch (operand_type->kind) {
                CASE(rio_CompilerTypeKind_Bool, b)
                CASE(rio_CompilerTypeKind_Char, c)
                CASE(rio_CompilerTypeKind_UChar, uc)
                CASE(rio_CompilerTypeKind_SChar, sc)
                CASE(rio_CompilerTypeKind_Short, s)
                CASE(rio_CompilerTypeKind_UShort, us)
                CASE(rio_CompilerTypeKind_Int, i)
                CASE(rio_CompilerTypeKind_UInt, u)
                CASE(rio_CompilerTypeKind_Long, l)
                CASE(rio_CompilerTypeKind_ULong, ul)
                CASE(rio_CompilerTypeKind_LLong, ll)
                CASE(rio_CompilerTypeKind_ULLong, ull)
                CASE(rio_CompilerTypeKind_Ptr, p)
                CASE(rio_CompilerTypeKind_Ref, p)
                default:
                    operand->is_const = false;
                    break;
                }
            }
        }
    }
    operand->type = qual_type;
    return true;
}

#undef CASE
