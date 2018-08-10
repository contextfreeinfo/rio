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
        case rio_CMPL_TYPE_BOOL: \
            operand->val.b = (bool)operand->val.t; \
            break; \
        case rio_CMPL_TYPE_CHAR: \
            operand->val.c = (char)operand->val.t; \
            break; \
        case rio_CMPL_TYPE_UCHAR: \
            operand->val.uc = (unsigned char)operand->val.t; \
            break; \
        case rio_CMPL_TYPE_SCHAR: \
            operand->val.sc = (signed char)operand->val.t; \
            break; \
        case rio_CMPL_TYPE_SHORT: \
            operand->val.s = (short)operand->val.t; \
            break; \
        case rio_CMPL_TYPE_USHORT: \
            operand->val.us = (unsigned short)operand->val.t; \
            break; \
        case rio_CMPL_TYPE_INT: \
        case rio_CMPL_TYPE_ENUM: \
            operand->val.i = (int)operand->val.t; \
            break; \
        case rio_CMPL_TYPE_UINT: \
            operand->val.u = (unsigned)operand->val.t; \
            break; \
        case rio_CMPL_TYPE_LONG: \
            operand->val.l = (long)operand->val.t; \
            break; \
        case rio_CMPL_TYPE_ULONG: \
            operand->val.ul = (unsigned long)operand->val.t; \
            break; \
        case rio_CMPL_TYPE_LLONG: \
            operand->val.ll = (long long)operand->val.t; \
            break; \
        case rio_CMPL_TYPE_ULLONG: \
            operand->val.ull = (unsigned long long)operand->val.t; \
            break; \
        case rio_CMPL_TYPE_PTR: \
            operand->val.p = (uintptr_t)operand->val.t; \
            break; \
        case rio_CMPL_TYPE_FLOAT: \
        case rio_CMPL_TYPE_DOUBLE: \
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
                if (type->kind == rio_CMPL_TYPE_ENUM) {
                    type = type->base;
                }
                rio_Type *operand_type = operand->type;
                if (operand_type->kind == rio_CMPL_TYPE_ENUM) {
                    operand_type = operand_type->base;
                }
                switch (operand_type->kind) {
                CASE(rio_CMPL_TYPE_BOOL, b)
                CASE(rio_CMPL_TYPE_CHAR, c)
                CASE(rio_CMPL_TYPE_UCHAR, uc)
                CASE(rio_CMPL_TYPE_SCHAR, sc)
                CASE(rio_CMPL_TYPE_SHORT, s)
                CASE(rio_CMPL_TYPE_USHORT, us)
                CASE(rio_CMPL_TYPE_INT, i)
                CASE(rio_CMPL_TYPE_UINT, u)
                CASE(rio_CMPL_TYPE_LONG, l)
                CASE(rio_CMPL_TYPE_ULONG, ul)
                CASE(rio_CMPL_TYPE_LLONG, ll)
                CASE(rio_CMPL_TYPE_ULLONG, ull)
                CASE(rio_CMPL_TYPE_PTR, p)
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
