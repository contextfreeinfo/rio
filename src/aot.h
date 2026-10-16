#ifndef rio_AOT_H
#define rio_AOT_H
#include <stdio.h>
#include "rio.h"
/* Writes a standalone C program for a compiled vm (compiled with rio_compileScratch: this reads the
   compiler's tables). hostFfi: C definitions of rio_ffi_<name>(rio_Val *a) for the host's ffi functions
   (may be NULL; then the user links them). */
int rio_aot(rio_Vm *vm, FILE *out, const char *hostFfi);
#endif
