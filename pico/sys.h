#pragma once

#include "util.h"

typedef enum rio_Err {
    rio_Err_none,
    rio_Err_bad,
    rio_Err_eof,
} rio_Err;

typedef void* rio_File;

rio_Err rio_close(rio_File file);
rio_Err rio_log(const char* message);
rio_Err rio_read(rio_File file, char* c);
