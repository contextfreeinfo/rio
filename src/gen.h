#pragma once

#include "util.h"

// TODO Include wasm gen everywhere, including pico.
// TODO Run on wasm most places.
// TODO Wasm gen good on pico so we can deploy directly to itch or wherever.
// TODO Run on native code

rio_Err rio_genDemo(void);

// Function pointers allow for changing arch based on command line args.
typedef struct rio_Gen {
    rio_Err (*argFloat)(rio_Buffer_Byte* code, int index, float value);

    rio_Err (*argInt)(rio_Buffer_Byte* code, int index, int32_t value);

    rio_Err (*argPtr)(rio_Buffer_Byte* code, int index, intptr_t value);

    rio_Err (*call)(rio_Buffer_Byte* code, intptr_t proc);

    // Track code used before this call.
    rio_Err (*procStart)(rio_Buffer_Byte* code);

    // Pass the used index from before start as the start.
    rio_Err (*procEnd)(rio_Buffer_Byte* code, size_t start);

    rio_Err (*ret)(rio_Buffer_Byte* code);
} rio_Gen;
