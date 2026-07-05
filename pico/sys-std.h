#pragma once

#include <stdlib.h>
#include "sys.h"

typedef struct rio_StdFile {
    FILE* file;
    size_t start;
    size_t end;
    char buffer[64];
} rio_StdFile;

rio_Err rio_file_make(FILE* file, rio_File* result);
