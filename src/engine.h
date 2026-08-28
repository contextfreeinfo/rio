#pragma once

#include "sys.h"

#define rio_codeSize 0x100000
#define rio_dataSize 0x200000

typedef struct rio_Proc {
    // TODO Params
    // TODO Return type
    size_t addr;
} rio_Proc;

rio_defineSpan(Proc);
rio_defineBuffer(Proc);

typedef struct rio_Engine {
    // TODO Constant data in one place.
    // TODO Separate space for writable memory.
    // TODO How to init writable memory?
    // TODO Any type descriptions here need available also at runtime.
    // TODO So keep them in the data.
    // TODO Separate table of procedures.
    // TODO Also separate table of types?
    rio_Buffer_Byte code;
    rio_Buffer_Byte data;
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
