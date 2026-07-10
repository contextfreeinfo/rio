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
    Type* items; \
} rio_Span_##Type

#define rio_defineBuffer(Type) typedef struct rio_Buffer_##Type { \
    rio_Span_##Type span; \
    size_t used; \
} rio_Buffer_##Type

typedef uint8_t Byte;

rio_defineSpan(Byte);
rio_defineBuffer(Byte);

rio_Err rio_pushBytes(rio_Buffer_Byte* engine, rio_Span_Byte bytes);
