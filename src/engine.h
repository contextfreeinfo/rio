#pragma once

#include <stdbool.h>
#include "sys.h"

#define rio_codeSize 0x100000
#define rio_dataSize 0x200000
#define rio_defsSize 0x2000
#define rio_typesSize 0x20000

typedef enum rio_Type {
    rio_Type_int,
    rio_Type_float,
    rio_Type_bool,
    rio_Type_composite, // TODO Subdivide?
} rio_Type;

// Currently 12 bytes on thumb2 and 24 bytes on arm64.
typedef struct rio_Def {
    uint16_t name; // TODO Make this 32 bits on arm64?
    bool constant : 1;
    bool local : 1; // If true, the address is frame relative???
    uint16_t reserved : 14; // Reserved. Basic types go here?
    uint8_t* type; // 0 i32, 1 f32, 2 bool, else composite desc address?
    union {
        // Only values for constants should appear here.
        bool boolVal;
        float f32Val;
        int32_t i32Val;
        // For a constant function, the actual address of the function.
        // If not constant, either a global or frame relative address.
        // For a function variable, the *pointer* is stored at this address.
        intptr_t ptrVal;
    };
} rio_Def;

rio_defineSpan(Def);
rio_defineBuffer(Def);

rio_Def* rio_findDef(rio_Buffer_Def* defs, int32_t name);

typedef struct rio_Proc {
    // TODO Params
    // TODO Return type
    intptr_t addr;
} rio_Proc;

// rio_defineSpan(Proc);
// rio_defineBuffer(Proc);

typedef struct rio_Engine {
    // At runtime, we only need to keep code and data/memory.
    // TODO If we do only indirect or relative calls, this could be movable.
    rio_Buffer_Byte code;
    // TODO Globals can't go here. Is the space wasted?
    // TODO Shadow stack this counting down from the end of data?
    rio_Buffer_Def defs;
    // TODO Separate constant data from runtime memory?
    // TODO Can we compile to wasm without that?
    rio_Buffer_Byte data;
    rio_Buffer_Byte memory;
    // Types also include procedure signatures.
    // It would be nice to store type defs in memory for rtti, but if we keep
    // them there, people might be afraid to define them and waste memory.
    // So this is for data that we discard at runtime.
    // TODO Globals can't go here. Is the space wasted?
    // TODO Same concern applies to source being compiled.
    // TODO Or *can* globals go here if zero init?
    // TODO Maybe zero-init arrays/globals are tracked from the back.
    // TODO Every zero-init global shrinks the size of the data span?
    rio_Buffer_Byte types;
    // Zero space is contiguous with data/memory, so zeroStart can't go below
    // the *end* of the data buffer used size, or vice versa.
    uint8_t* zeroStart;
    uint8_t* zeroEnd;
    // TODO This could instead easily be an index into the code buffer.
    intptr_t main;
} rio_Engine;

void rio_reportEngine(rio_Engine* engine);

typedef struct rio_Blob {
    // Items go first because pointer might be bigger than size.
    uint8_t* items;
    // For string literals, char data can squeeze in right after size.
    int32_t size;
} rio_Blob;

rio_Err rio_runLog(rio_Blob* message);
