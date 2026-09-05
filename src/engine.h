#pragma once

#include "sys.h"

#define rio_codeSize 0x100000
#define rio_dataSize 0x200000

#define rio_defsSize 0x2000

// TODO Union procs with structs?
#define rio_procsSize 0x1000

typedef struct rio_Def {
    uint16_t name;
    uint16_t index; // If int32_t or size_t, up to 1/2 MB on thumb for 64k defs.
} rio_Def;

rio_defineSpan(Def);
rio_defineBuffer(Def);

typedef struct rio_Proc {
    // TODO Params
    // TODO Return type
    intptr_t addr;
} rio_Proc;

rio_defineSpan(Proc);
rio_defineBuffer(Proc);

typedef struct rio_Engine {
    // TODO Constant data in one place.
    // TODO Separate space for writable memory.
    // TODO How to init writable memory?
    // TODO Any type descriptions here need available also at runtime.
    // TODO So keep them in the data.
    // TODO Also separate table of types?
    rio_Buffer_Byte code;
    // TODO defs buffer that fills up across modules and stackly in locals?
    // TODO After a module is done, move pubs elsewhere or collapse non-pubs?
    // TODO Then each module gives its range of pub defs.
    // TODO Separate defs for module stack from all module pubs.
    rio_Buffer_Def locals;
    rio_Buffer_Def pubs;
    rio_Buffer_Byte data; // TODO Combine data with memory.
    rio_Buffer_Byte memory;
    rio_Buffer_Proc procs;
} rio_Engine;

void rio_reportEngine(rio_Engine* engine);

typedef struct rio_Blob {
    // Items go first because pointer might be bigger than size.
    uint8_t* items;
    // For string literals, char data can squeeze in right after size.
    int32_t size;
} rio_Blob;

rio_Err rio_runLog(rio_Blob* message);
