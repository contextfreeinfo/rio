#pragma once

#include "sys.h"

typedef struct rio_Engine {
    // TODO Any type descriptions here need available also at runtime.
    // TODO So keep them in the data.
    rio_Buffer_Byte data;
} rio_Engine;

void rio_reportEngine(rio_Engine* engine);
