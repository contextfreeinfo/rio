#pragma once

#include "util.h"

typedef void* rio_File;

rio_Err rio_close(rio_File file);
rio_Err rio_log(const char* message);
rio_Err rio_read(rio_File file, char* c);
