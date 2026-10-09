#ifndef NIB_AOT_H
#define NIB_AOT_H
#include <stdio.h>
#include "nib.h"
/* write a standalone C program for a compiled vm. host_ffi: C definitions of nib_ffi_<name>(NibVal *a)
   for the host's ffi functions (may be NULL; then the user links them). */
int nib_aot(Nib *vm, FILE *out, const char *host_ffi);
#endif
