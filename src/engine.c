#include "engine.h"
#include <assert.h>
#include <stdio.h>

void rio_reportEngine(rio_Engine* engine) {
    printf("Data:\n");
    rio_Span_Byte data = engine->data.span;
    size_t used = engine->data.used;
    for (size_t index = 0; index < used; index += 1) {
        printf("%zu: %d\n", index, data.items[index]);
    }
}

rio_Err rio_runLog(rio_Engine* engine, int32_t message) {
    // Check input.
    rio_Span_Byte span = engine->memory.span;
    assert(span.size >= 8);
    if (message < 0 || (size_t)message >= span.size - 8) return rio_Err_bad;
    int32_t* words = message + (int32_t*)span.items;
    int32_t offset = words[0];
    int32_t len = words[1];
    if (offset < 0 || len < 0) return rio_Err_bad;
    if (span.size - (size_t)offset <= (size_t)len + 1) return rio_Err_bad;
    // Exit early if empty.
    if (!len) return 0;
    // Adjust, write, and check output.
    uint8_t* addr = span.items + offset;
    size_t size = (size_t)len;
    size_t written = fwrite(addr, size, 1, stdout);
    if (puts("") < 0) return rio_Err_bad;
    return written == size ? 0 : rio_Err_bad;
}
