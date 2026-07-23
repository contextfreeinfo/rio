#pragma once

#include <stdint.h>
#include <stdlib.h>

typedef enum rio_Err {
    rio_Err_none,
    rio_Err_bad,
    rio_Err_eof,
} rio_Err;

#define rio_defineSpan(Type) typedef struct rio_Span_##Type { \
    size_t size; \
    rio_##Type* items; \
} rio_Span_##Type

#define rio_defineBuffer(Type) typedef struct rio_Buffer_##Type { \
    rio_Span_##Type span; \
    size_t used; \
} rio_Buffer_##Type

typedef uint8_t rio_Byte;
typedef uint16_t rio_UInt16;

rio_defineSpan(Byte);
rio_defineSpan(UInt16);
rio_defineBuffer(Byte);
rio_defineBuffer(UInt16);

rio_Err rio_pushBytes(rio_Buffer_Byte* buffer, rio_Span_Byte bytes);
rio_Err rio_pushBytesByte(rio_Buffer_Byte* buffer, rio_Byte value);
rio_Err rio_pushBytesInt32(rio_Buffer_Byte* buffer, int32_t value);
rio_Err rio_pushBytesPad32(rio_Buffer_Byte* buffer);

typedef struct rio_Table {
    rio_Span_UInt16 starts;
    rio_Buffer_Byte strings;
} rio_Table;

uint32_t rio_hash(rio_Byte* string);

// The intern is an index into the `starts` span.
// Focus on the intern instead of the string start itself because we expert
// fewer of them, so side lookup tables also can be smaller.
rio_Err rio_table(rio_Table* table, rio_Byte* string, int32_t* intern);
rio_Err rio_tabled(rio_Table* table, int32_t intern, rio_Byte** string);
