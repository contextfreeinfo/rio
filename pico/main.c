#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "engine.h"
#include "gen.h"
#include "parse.h"
#include "sys-std.h"

rio_Err rio_run(int argc, const char** argv) {
    if (argc < 2) return rio_Err_bad;
    rio_Err err = 0;
    const char* path = argv[1];
    FILE* f = fopen(path, "r");
    if (!f) {
        rio_log("Failed to open:");
        rio_log(path);
        return rio_Err_bad;
    }
    size_t dataSize = 4 << 20;
    uint8_t* dataBytes = malloc(dataSize);
    if (!dataBytes) return rio_Err_bad;
    memset(dataBytes, 0, dataSize);
    rio_Engine engine = { .data = { .size = dataSize, .items = dataBytes } };
    rio_StdFile file = { .file = f };
    rio_Parser parser = {
        .engine = &engine,
        .lexer = { .file = &file },
    };
    err = rio_parse(&parser);
    rio_close(&file);
    if (err) goto freeData;
    err = rio_genDemo();
    if (err) goto freeData;
    freeData:
    free(dataBytes);
    return err;
}

int main(int argc, const char** argv) {
    // TODO Better arg parse.
    if (argc > 1) {
        return rio_run(argc, argv);
    } else {
        // TODO Better help.
        printf("Script not specified.\n");
    }
    return 0;
}
