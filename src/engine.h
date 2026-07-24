#pragma once

#include "sys.h"

#define rio_codeSize 0x100000
#define rio_dataSize 0x200000

typedef struct rio_Engine {
    // TODO Constant data in one place.
    // TODO Separate space for writable memory.
    // TODO How to init writable memory?
    // TODO Any type descriptions here need available also at runtime.
    // TODO So keep them in the data.
    rio_Buffer_Byte code;
    rio_Buffer_Byte data;
    rio_Buffer_Byte memory;
} rio_Engine;

void rio_reportEngine(rio_Engine* engine);
