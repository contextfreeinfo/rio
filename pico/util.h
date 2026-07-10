#pragma once

#include <stdint.h>
#include <stdlib.h>

#define rio_defineSpan(Type) typedef struct rio_Span_##Type { \
    size_t size; \
    Type* items; \
} rio_Span_##Type

rio_defineSpan(uint8_t);
typedef rio_Span_uint8_t rio_Bytes;
