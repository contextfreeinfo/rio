#include "engine.h"
#include <assert.h>
#include <stdio.h>

static void printBuffer(rio_Buffer_Byte* buffer) {
    rio_Span_Byte bytes = buffer->span;
    size_t used = buffer->used;
    size_t bytesPerLine = 16;
    for (size_t index = 0; index < used; index += bytesPerLine) {
        printf("0x%08x: ", (uint32_t)index);
        size_t bytesThisLine = used - index;
        if (bytesThisLine > bytesPerLine) {
            bytesThisLine = bytesPerLine;
        }
        for (size_t offset = 0; offset < bytesPerLine; offset += 1) {
            if (offset < bytesThisLine) {
                if (offset && !(offset % 4)) printf("_");
                printf("%02x", bytes.items[index + offset]);
            } else {
                if (offset && !(offset % 4)) printf(" ");
                printf("  ");
            }
        }
        printf(" ");
        for (size_t offset = 0; offset < bytesThisLine; offset += 1) {
            uint8_t c = bytes.items[index + offset];
            if (c < 32 || c > 126) c = '_';
            printf("%c", c);
        }
        printf("\n");
    }
}

void rio_reportEngine(rio_Engine* engine) {
    printf("Code:\n");
    printBuffer(&engine->code);
    printf("Data:\n");
    printBuffer(&engine->data);
}

rio_Err rio_runLog(rio_Blob* message) {
    size_t size = (size_t)message->size;
    size_t written = fwrite(message->items, size, 1, stdout);
    if (puts("") < 0) return rio_Err_bad;
    return written == size ? 0 : rio_Err_bad;
}
