#pragma once

#include "util.h"

typedef void* rio_File;

rio_Err rio_close(rio_File file);
rio_Err rio_log(const char* message);
rio_Err rio_read(rio_File file, uint8_t* c);

rio_Err rio_allocPages(size_t sizeBytes, uint8_t** memOut);
rio_Err rio_enableExec(uint8_t* mem, size_t sizeBytes);
rio_Err rio_freePages(uint8_t* mem, size_t sizeBytes);
