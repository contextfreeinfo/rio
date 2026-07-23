#pragma once

#include <stdlib.h>
#include "sys.h"

typedef struct rio_StdFile {
    FILE* file;
    size_t start;
    size_t end;
    char buffer[64];
} rio_StdFile;
