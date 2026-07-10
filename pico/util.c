#include "util.h"
#include <string.h>

rio_Err rio_pushBytes(rio_Buffer_Byte* buffer, rio_Span_Byte bytes) {
    size_t remaining = buffer->span.size - buffer->used;
    if (remaining < bytes.size) return rio_Err_bad;
    memcpy(buffer->span.items, bytes.items, bytes.size);
    buffer->used += bytes.size;
    return 0;
}
