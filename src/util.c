#include "util.h"
#include <stdio.h>
#include <string.h>

int rio_verbosity = 0;

static intptr_t stackBase;
static intptr_t deepestStack;
static intptr_t maxStack = 0x10000000;

void rio_initStackCheck(intptr_t maxStack_) {
    uint8_t mark;
    stackBase = (intptr_t)&mark;
    maxStack = maxStack_;
}

rio_Err rio_checkStack(void) {
    uint8_t markVar;
    intptr_t mark = (intptr_t)&markVar;
    if (mark < deepestStack) {
        deepestStack = mark;
    }
    intptr_t depth = stackBase - mark;
    if (rio_verbosity) {
        printf(
            "rio_checkStack: %d (%d), %d (%d)\n",
            mark, depth, deepestStack, stackBase - deepestStack
        );
    }
    if (depth > maxStack) {
        printf("Stack overflow: %d\n", depth);
        return rio_Err_bad;
    }
    return 0;
}

rio_Err rio_pushBytes(rio_Buffer_Byte* buffer, rio_Span_Byte bytes) {
    size_t remaining = buffer->span.size - buffer->used;
    if (remaining < bytes.size) return rio_Err_bad;
    memcpy(buffer->span.items + buffer->used, bytes.items, bytes.size);
    buffer->used += bytes.size;
    return 0;
}

rio_Err rio_pushBytesByte(rio_Buffer_Byte* buffer, rio_Byte value) {
    rio_Span_Byte span = {.size = sizeof(value), .items = (rio_Byte*)&value};
    return rio_pushBytes(buffer, span);
}

rio_Err rio_pushBytesInt16Pre(rio_Buffer_Byte* buffer, int16_t value) {
    rio_Span_Byte span = {.size = sizeof(value), .items = (rio_Byte*)&value};
    return rio_pushBytes(buffer, span);
}

rio_Err rio_pushBytesInt32Pre(rio_Buffer_Byte* buffer, int32_t value) {
    rio_Span_Byte span = {.size = sizeof(value), .items = (rio_Byte*)&value};
    return rio_pushBytes(buffer, span);
}

rio_Err rio_pushBytesInt64Pre(rio_Buffer_Byte* buffer, int64_t value) {
    rio_Span_Byte span = {.size = sizeof(value), .items = (rio_Byte*)&value};
    return rio_pushBytes(buffer, span);
}

rio_Err rio_pushBytesInt16(rio_Buffer_Byte* buffer, int16_t value) {
    rio_pushBytesPad16(buffer);
    return rio_pushBytesInt16Pre(buffer, value);
}

rio_Err rio_pushBytesInt32(rio_Buffer_Byte* buffer, int32_t value) {
    rio_pushBytesPad32(buffer);
    return rio_pushBytesInt32Pre(buffer, value);
}

rio_Err rio_pushBytesInt64(rio_Buffer_Byte* buffer, int64_t value) {
    rio_pushBytesPad64(buffer);
    return rio_pushBytesInt64Pre(buffer, value);
}

rio_Err rio_pushBytesPad16(rio_Buffer_Byte* buffer) {
    rio_Err err = 0;
    // Write zeros even though we've likely precleared.
    // TODO Could optimize this into single increment and memset, but meh?
    while (buffer->used % 2) {
        if ((err = rio_pushBytesByte(buffer, 0))) return err;
    }
    return 0;
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

rio_Err rio_pushBytesPad64(rio_Buffer_Byte* buffer) {
    rio_Err err = 0;
    // Write zeros even though we've likely precleared.
    // TODO Could optimize this into single increment and memset, but meh?
    while (buffer->used % 8) {
        if ((err = rio_pushBytesByte(buffer, 0))) return err;
    }
    return 0;
}

uint32_t rio_hash(rio_Byte* string) {
    // Just use fnv-1a.
    uint32_t hash = 0x811c9dc5u;
    while (*string) {
        hash ^= *string++;
        hash *= 0x01000193u;
    }
    return hash;
}

rio_Err rio_table(rio_Table* table, rio_Byte* string, rio_Intern* intern) {
    rio_Err err = 0;
    if (!*string) {
        if (intern) *intern = 0;
        return 0;
    }
    uint32_t indexStart = rio_hash(string) % table->starts.size;
    uint32_t index = indexStart;
    do {
        rio_UInt16 maybeStart = table->starts.items[index];
        if (maybeStart) {
            // Try to match it.
            char* maybeString = (char*)(table->strings.span.items + maybeStart);
            if (!strcmp((char*)string, maybeString)) {
                // Found it.
                // TODO Verify that it's in range?
                if (intern) *intern = (rio_Intern)index;
                return 0;
            }
        } else {
            // Try to fit it.
            maybeStart = (rio_UInt16)table->strings.used;
            size_t size = strlen((char*)string) + 1;
            rio_Span_Byte span = {.size = size, .items = string};
            if ((err = rio_pushBytes(&table->strings, span))) return err;
            table->starts.items[index] = maybeStart;
            // Fit it.
            if (intern) *intern = index;
            return 0;
        }
        index = (index + 1) % table->starts.size;
    } while (index != indexStart);
    // We neither found nor fit it.
    return rio_Err_bad;
}

rio_Err rio_tabled(rio_Table* table, rio_Intern intern, rio_Byte** string) {
    rio_Err err = 0;
    // The only valid empty string intern is 0.
    if (!intern) {
        *string = table->strings.span.items;
        return 0;
    }
    // Out of bounds is bad.
    if (intern >= (rio_Intern)table->starts.size) {
        return rio_Err_bad;
    }
    // See what we have, where index 0 now means no entry.
    rio_UInt16 start = table->starts.items[intern];
    if (!start) return rio_Err_bad;
    if (string) {
        *string = table->strings.span.items + start;
    }
    return err;
}
