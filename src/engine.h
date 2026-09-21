#pragma once

#include <stdbool.h>
#include "sys.h"

#define rio_codeSize 0x100000
#define rio_dataSize 0x200000
#define rio_defsSize 0x2000
#define rio_typesSize 0x20000

// Usable directly as type ids.
typedef enum rio_CoreType {
    // Primitive types.
    rio_CoreType_none,
    rio_CoreType_void,
    rio_CoreType_bool,
    rio_CoreType_float,
    rio_CoreType_int,
    // Semi-primitive types.
    rio_CoreType_blob,
    rio_CoreType_string,
    // Any value >= end requires a description in memory.
    rio_CoreType_end,
} rio_CoreType;

// Kinds of aggregate types requiring descriptions in memory.
typedef enum rio_TypeKind {
    rio_TypeKind_none,
    rio_TypeKind_proc,
    rio_TypeKind_span,
    rio_TypeKind_struct,
    rio_TypeKind_union,
} rio_TypeKind;

// Currently 12 bytes on thumb2 and 24 bytes on arm64.
typedef struct rio_Def {
    rio_Intern name;
    bool constant : 1;
    bool local : 1; // If true, the address is frame relative???
    uint16_t reserved : 14; // Reserved. Basic types go here?
    // Wastes 32 bits here on 64-bit arch.
    uint8_t* type; // Core type or address.
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

// Currently 8 bytes on arch32 and 16 bytes on arch64.
typedef struct rio_Field {
    uint16_t name;
    uint16_t offset;
    uint8_t* type; // Core type or address.
} rio_Field;

rio_defineSpan(Field);
rio_defineBuffer(Field);

// Should be 8 bytes on arch32 or 16 bytes on arch64.
typedef struct rio_ProcType {
    uint8_t typeKind; // Always value rio_TypeKind_proc.
    // Expected to immediate precede in memory?
    uint8_t paramCount;
    uint16_t reserved;
    // Wastes 32 bits here on arch64.
    // rio_Span_Def params;
    uint8_t* returnType;
} rio_ProcType;

typedef struct rio_CommonNames {
    rio_Intern typeBool;
    rio_Intern typeFloat;
    rio_Intern typeInt;
    rio_Intern typeString;
} rio_CommonNames;

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
    // For efficient reference to names.
    rio_Table names;
    rio_CommonNames commonNames;
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

rio_Err rio_engineInit(rio_Engine* engine);

rio_Err rio_enginePadDataPtr(rio_Engine* engine);
rio_Err rio_enginePushDataPtr(rio_Engine* engine, intptr_t ptr);
rio_Err rio_enginePushDataInt32(rio_Engine* engine, int32_t i);
rio_Err rio_enginePadZeroPtrArray(rio_Engine* engine, size_t count);
rio_Err rio_enginePadZeroIntArray(rio_Engine* engine, size_t count);

void rio_reportEngine(rio_Engine* engine);

typedef struct rio_Blob {
    // Items go first because pointer might be bigger than size.
    uint8_t* items;
    // For string literals, char data can squeeze in right after size.
    int32_t size;
} rio_Blob;

rio_Err rio_runLog(rio_Blob* message);
