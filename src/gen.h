#pragma once

#include "engine.h"
#include "util.h"

#if UINTPTR_MAX == 0xffffffffffffffffULL
    #define rio_ptrSize 8
#elif UINTPTR_MAX == 0xffffffffU
    #define rio_ptrSize 4
#endif

#define rio_maxArgs 3

// TODO Include wasm gen everywhere, including pico.
// TODO Run on wasm most places.
// TODO Wasm gen good on pico so we can deploy directly to itch or wherever?
// TODO Except we might be relying on local network file servers anyway???
// TODO Run on native code on pico still.
// TODO Maybe just have a hardcoded function to call that might or might not
// TODO also generate wasm?

// Function pointers allow for changing arch based on command line args.
typedef struct rio_Gen {
    rio_Buffer_Byte* code;
    int state;

    // rio_Err (*argFloat)(rio_Buffer_Byte* code, int index, float value);
    // rio_Err (*argInt)(rio_Buffer_Byte* code, int index, int32_t value);
    // rio_Err (*argPtr)(rio_Buffer_Byte* code, int index, intptr_t value);
    // rio_Err (*call)(rio_Buffer_Byte* code, intptr_t proc);
    // // Track code used before this call.
    // rio_Err (*procStart)(rio_Buffer_Byte* code);
    // // Pass the used index from before start as the start.
    // rio_Err (*procEnd)(rio_Buffer_Byte* code, size_t start);
    // rio_Err (*ret)(rio_Buffer_Byte* code);
} rio_Gen;

// If target is null, get target from stack.
rio_Err rio_genCall(rio_Gen* gen, intptr_t target, size_t arity);

rio_Err rio_genIntAdd(rio_Gen* gen);

// TODO Separate options for pushing/popping shadow stack?
rio_Err rio_genPopAsArgs(rio_Gen* gen, size_t count);

rio_Err rio_genProcBegin(rio_Gen* gen);

rio_Err rio_genProcEnd(rio_Gen* gen);

// Always push intptr_t to keep things simple, even if non-pointer types are
// smaller on 64-bit systems.
rio_Err rio_genPush(rio_Gen* gen, intptr_t value);

rio_Err rio_genUnusedPush(rio_Gen* gen);

rio_Err rio_memPushPtr(rio_Buffer_Byte* buffer, size_t offset);

uint8_t* rio_addrForExec(uint8_t* addr);
