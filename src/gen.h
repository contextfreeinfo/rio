#pragma once

#include "util.h"

// TODO Include wasm gen everywhere, including pico.
// TODO Run on wasm most places.
// TODO Wasm gen good on pico so we can deploy directly to itch or wherever.
// TODO Run on native code

rio_Err rio_genDemo(void);

// Function pointers allow for changing arch based on command line args.
typedef struct rio_Gen {
    rio_Err (*arg)(rio_Buffer_Byte code, int32_t index, int32_t value);

    rio_Err (*call)(rio_Buffer_Byte code, int32_t proc);

    rio_Err (*ret)(rio_Buffer_Byte code);
} rio_Gen;
