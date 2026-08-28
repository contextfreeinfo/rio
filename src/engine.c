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

rio_Err rio_runLog(rio_Blob* message) {
    size_t size = (size_t)message->size;
    size_t written = fwrite(message->items, size, 1, stdout);
    if (puts("") < 0) return rio_Err_bad;
    return written == size ? 0 : rio_Err_bad;
}
