#pragma once

#include "sys.h"

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
