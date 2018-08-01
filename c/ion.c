// This file contains code that can't easily be expressed in ion at present.
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

struct ion_Operand;
struct ion_Type;

bool cast_operand(struct ion_Operand *operand, struct ion_Type *type);

#ifdef _WIN32
#   include "out_ion_win32.c"
#else
#   include "out_ion_linux.c"
#endif

#define CASE(k, t) \
    case k: \
        switch (type->kind) { \
        case ION_CMPL_TYPE_BOOL: \
            operand->val.b = (bool)operand->val.t; \
            break; \
        case ION_CMPL_TYPE_CHAR: \
            operand->val.c = (char)operand->val.t; \
            break; \
        case ION_CMPL_TYPE_UCHAR: \
            operand->val.uc = (unsigned char)operand->val.t; \
            break; \
        case ION_CMPL_TYPE_SCHAR: \
            operand->val.sc = (signed char)operand->val.t; \
            break; \
        case ION_CMPL_TYPE_SHORT: \
            operand->val.s = (short)operand->val.t; \
            break; \
        case ION_CMPL_TYPE_USHORT: \
            operand->val.us = (unsigned short)operand->val.t; \
            break; \
        case ION_CMPL_TYPE_INT: \
        case ION_CMPL_TYPE_ENUM: \
            operand->val.i = (int)operand->val.t; \
            break; \
        case ION_CMPL_TYPE_UINT: \
            operand->val.u = (unsigned)operand->val.t; \
            break; \
        case ION_CMPL_TYPE_LONG: \
            operand->val.l = (long)operand->val.t; \
            break; \
        case ION_CMPL_TYPE_ULONG: \
            operand->val.ul = (unsigned long)operand->val.t; \
            break; \
        case ION_CMPL_TYPE_LLONG: \
            operand->val.ll = (long long)operand->val.t; \
            break; \
        case ION_CMPL_TYPE_ULLONG: \
            operand->val.ull = (unsigned long long)operand->val.t; \
            break; \
        case ION_CMPL_TYPE_PTR: \
            operand->val.p = (uintptr_t)operand->val.t; \
            break; \
        case ION_CMPL_TYPE_FLOAT: \
        case ION_CMPL_TYPE_DOUBLE: \
            break; \
        default: \
            operand->is_const = false; \
            break; \
        } \
        break;

bool cast_operand(ion_Operand *operand, ion_Type *type) {
    ion_Type *qual_type = type;
    type = ion_unqualify_type(type);
    operand->type = ion_unqualify_type(operand->type);
    if (operand->type != type) {
        if (!ion_is_castable(operand, type)) {
            return false;
        }
        if (operand->is_const) {
            if (ion_is_floating_type(operand->type)) {
                operand->is_const = !ion_is_integer_type(type);
            } else {
                if (type->kind == ION_CMPL_TYPE_ENUM) {
                    type = type->base;
                }
                ion_Type *operand_type = operand->type;
                if (operand_type->kind == ION_CMPL_TYPE_ENUM) {
                    operand_type = operand_type->base;
                }
                switch (operand_type->kind) {
                CASE(ION_CMPL_TYPE_BOOL, b)
                CASE(ION_CMPL_TYPE_CHAR, c)
                CASE(ION_CMPL_TYPE_UCHAR, uc)
                CASE(ION_CMPL_TYPE_SCHAR, sc)
                CASE(ION_CMPL_TYPE_SHORT, s)
                CASE(ION_CMPL_TYPE_USHORT, us)
                CASE(ION_CMPL_TYPE_INT, i)
                CASE(ION_CMPL_TYPE_UINT, u)
                CASE(ION_CMPL_TYPE_LONG, l)
                CASE(ION_CMPL_TYPE_ULONG, ul)
                CASE(ION_CMPL_TYPE_LLONG, ll)
                CASE(ION_CMPL_TYPE_ULLONG, ull)
                CASE(ION_CMPL_TYPE_PTR, p)
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
