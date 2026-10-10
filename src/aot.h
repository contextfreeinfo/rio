#ifndef RIO_AOT_H
#define RIO_AOT_H
#include <stdio.h>
#include "rio.h"
/* write a standalone C program for a compiled vm. host_ffi: C definitions of rio_ffi_<name>(RioVal *a)
   for the host's ffi functions (may be NULL; then the user links them). */
int rio_aot(Rio *vm, FILE *out, const char *host_ffi);
#endif
