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
    // Code.
    // Because typing, we can keep function pointers/ids separate from data
    // pointers. This is also nice for wasm.
    uint8_t* codeBytes = malloc(rio_codeSize);
    if (!codeBytes) return rio_Err_bad;
    memset(codeBytes, 0, rio_codeSize);
    // Data.
    // TODO Ensure full pages for marking as read-only.
    uint8_t* dataBytes = malloc(rio_dataSize);
    if (!dataBytes) goto freeCode;
    memset(dataBytes, 0, rio_dataSize);
    // Names.
    uint8_t* namesBytes = malloc(rio_namesSize);
    if (!namesBytes) goto freeData;
    memset(namesBytes, 0, rio_namesSize);
    // Name Starts.
    // If each name averages 7 bytes, that's 8 with null char, meaning bytes / 4
    // still is likely to be half empty.
    size_t nameStartsLen = rio_namesSize / 4;
    size_t nameStartsBytesSize = nameStartsLen * sizeof(rio_UInt16);
    rio_UInt16* nameStarts = malloc(nameStartsBytesSize);
    if (!nameStarts) goto freeNames;
    memset(nameStarts, 0, nameStartsBytesSize);
    // Engine.
    rio_Engine engine = {
        // Use up each first 4-byte word so nil pointers aren't useful.
        .code = {{.size = rio_codeSize, .items = codeBytes}, .used = 4},
        .data = {{.size = rio_dataSize, .items = dataBytes}, .used = 4},
    };
    rio_StdFile file = {.file = f};
    rio_Parser parser = {
        .engine = &engine,
        .gen = {.code = &engine.code},
        .lexer = {.file = &file},
        .names = {
            .starts = {.size = nameStartsLen, .items = nameStarts},
            .strings = {
                .span = {.size = rio_namesSize, .items = namesBytes},
                // This makes index 0 always be an invalid empty name.
                .used = 2,
            },
        },
    };
    err = rio_parse(&parser);
    rio_close(&file);
    if (err) goto done;
    rio_reportParser(&parser);
    err = rio_genDemo();
    if (err) goto done;
    done:;
    // freeNameStarts:;
    free(nameStarts);
    freeNames:
    free(namesBytes);
    freeData:
    free(dataBytes);
    freeCode:
    free(codeBytes);
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
