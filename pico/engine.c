#include "engine.h"
#include <stdio.h>

void rio_reportEngine(rio_Engine* engine) {
    printf("Data:\n");
    rio_Span_Byte data = engine->data.span;
    size_t used = engine->data.used;
    for (size_t index = 0; index < used; index += 1) {
        printf("%zu: %d\n", index, data.items[index]);
    }
}
