#pragma once

#include "sys.h"

typedef struct rio_Engine {
    rio_Bytes data;
    size_t dataSize;
} rio_Engine;

rio_Err rio_pushData(rio_Engine* engine, rio_Bytes bytes);
