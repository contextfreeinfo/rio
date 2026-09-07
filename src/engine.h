#pragma once

#include <stdbool.h>
#include "sys.h"

#define rio_codeSize 0x100000
#define rio_dataSize 0x200000
#define rio_defsSize 0x2000

typedef enum rio_Type {
    rio_Type_int,
    rio_Type_float,
    rio_Type_bool,
    rio_Type_composite, // TODO Subdivide?
} rio_Type;

// Currently 12 bytes on thumb2 and 24 bytes on arm64.
typedef struct rio_Def {
    uint16_t name;
    bool constant : 1;
    uint16_t reserved : 15; // Reserved. Basic types go here?
    uint8_t* type; // 0 i32, 1 f32, 2 bool, else composite desc address?
    union {
        // Only values for constants should appear here.
        bool boolVal;
        float f32Val;
        int32_t i32Val;
        intptr_t ptrVal;
    };
} rio_Def;

rio_defineSpan(Def);
rio_defineBuffer(Def);

typedef struct rio_Proc {
    // TODO Params
    // TODO Return type
    intptr_t addr;
} rio_Proc;

// rio_defineSpan(Proc);
// rio_defineBuffer(Proc);

typedef struct rio_Engine {
    // TODO Constant data in one place.
    // TODO Separate space for writable memory.
    // TODO How to init writable memory?
    // TODO Any type descriptions here need available also at runtime.
    // TODO So keep them in the data.
    // TODO Also separate table of types?
    rio_Buffer_Byte code;
    // TODO Defs for active space. Pub module members go into memory.
    rio_Buffer_Def defs;
    rio_Buffer_Byte data; // TODO Combine data with memory.
    rio_Buffer_Byte memory;
} rio_Engine;

void rio_reportEngine(rio_Engine* engine);

typedef struct rio_Blob {
    // Items go first because pointer might be bigger than size.
    uint8_t* items;
    // For string literals, char data can squeeze in right after size.
    int32_t size;
} rio_Blob;

rio_Err rio_runLog(rio_Blob* message);
