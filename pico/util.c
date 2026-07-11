#include "util.h"
#include <string.h>

rio_Err rio_pushBytes(rio_Buffer_Byte* buffer, rio_Span_Byte bytes) {
    size_t remaining = buffer->span.size - buffer->used;
    if (remaining < bytes.size) return rio_Err_bad;
    memcpy(buffer->span.items + buffer->used, bytes.items, bytes.size);
    buffer->used += bytes.size;
    return 0;
}

rio_Err rio_pushBytesByte(rio_Buffer_Byte* buffer, Byte value) {
    rio_Span_Byte span = { .size = sizeof(value), .items = (Byte*)&value };
    return rio_pushBytes(buffer, span);
}

rio_Err rio_pushBytesInt32(rio_Buffer_Byte* buffer, int32_t value) {
    rio_Span_Byte span = { .size = sizeof(value), .items = (Byte*)&value };
    return rio_pushBytes(buffer, span);
}

rio_Err rio_pushBytesPad32(rio_Buffer_Byte* buffer) {
    rio_Err err = 0;
    // Write zeros even though we've likely precleared.
    // TODO Could optimize this into single increment and memset, but meh?
    while (buffer->used % 4) {
        if ((err = rio_pushBytesByte(buffer, 0))) return err;
    }
    return 0;
}
