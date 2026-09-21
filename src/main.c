#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "engine.h"
#include "gen.h"
#include "parse.h"
#include "sys-std.h"

rio_Err rio_run(const char* path) {
    rio_Err err = 0;
    FILE* f = fopen(path, "r");
    if (!f) {
        rio_log("Failed to open:");
        rio_log(path);
        return rio_Err_bad;
    }
    // Code.
    // Because typing, we can keep function pointers/ids separate from data
    // pointers. This is also nice for wasm.
    uint8_t* codeBytes = NULL;
    if ((err = rio_allocPages(rio_codeSize, &codeBytes))) return err;
    memset(codeBytes, 0, rio_codeSize);
    err = rio_Err_bad;
    // Data. See later for some of the usage of this space.
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
    // Defs.
    rio_Def* defs = malloc(rio_defsSize * sizeof(rio_Def));
    if (!defs) goto freeNameStarts;
    // Types.
    uint8_t* typesBytes = malloc(rio_typesSize);
    if (!typesBytes) goto freeDefs;
    memset(typesBytes, 0, rio_typesSize);
    // Engine.
    uint8_t* zeroEnd = dataBytes + rio_dataSize;
    rio_Engine engine = {
        .code = {
            .span = {.size = rio_codeSize, .items = codeBytes},
            // Use up each first word so nil pointers aren't useful.
            .used = rio_ptrSize,
        },
        // Initialized data space grows from bottom up.
        .data = {
            .span = {.size = rio_dataSize, .items = dataBytes},
            .used = rio_ptrSize,
        },
        // Zero-init allocations are empty to start with and grow down.
        .zeroStart = zeroEnd,
        .zeroEnd = zeroEnd,
        // Defs are logically separate from data/memory.
        // In Spry, these and more inhabit the zero space.
        .defs = {{.size = rio_defsSize, .items = defs}, .used = 1},
        .names = {
            .starts = {.size = nameStartsLen, .items = nameStarts},
            .strings = {
                .span = {.size = rio_namesSize, .items = namesBytes},
                // This makes index 0 always be an invalid empty name.
                .used = 2,
            },
        },
        .types = {
            .span = {.size = rio_typesSize, .items = typesBytes},
            // Give some extra start here for more primitive types.
            .used = 2 * rio_ptrSize,
        },
    };
    // TODO Combine memory with data for local running but not wasm?
    engine.memory = engine.data;
    rio_StdFile file = {.file = f};
    rio_Parser parser = {
        .engine = &engine,
        .gen = {.code = &engine.code},
        .lexer = {.file = &file},
    };
    // Prefill defs such as `log`.
    // And we know initial defs won't overflow.
    rio_Intern logName;
    rio_table(&engine.names, (rio_Byte*)"log", &logName);
    rio_pushDef(&engine.defs, (rio_Def){
        .name = logName,
        .constant = true,
        .ptrVal = (intptr_t)rio_runLog,
    });
    rio_Def logDef = engine.defs.span.items[engine.defs.used - 1];
    if (rio_verbosity) {
        printf("log fun %d at %p\n", logDef.name, (void*)logDef.ptrVal);
    }
    // Parse/process.
    err = rio_parse(&parser);
    rio_close(&file);
    if (err) goto done;
    if (rio_verbosity) rio_reportParser(&parser);
    if ((err = rio_enableExec(codeBytes, rio_codeSize))) goto done;
    if (engine.main) {
        void (*codeMain)(void) = (void (*)(void))engine.main;
        if (rio_verbosity) {
            printf("Wanting to call main at: %p\n", (void*)(intptr_t)codeMain);
        }
        #if defined(__thumb2__)
            codeMain();
        #endif
    }
    // This log was a test of string layout, but we'd want to get the actual
    // address of the message from the defs.
    // rio_runLog((rio_Blob*)(engine.memory.span.items + rio_ptrSize));
    // printf("def size: %zu\n", sizeof(rio_Def));
    if (err) goto done;
    done:;
    // freeTypes:
    free(typesBytes);
    freeDefs:
    free(defs);
    freeNameStarts:
    free(nameStarts);
    freeNames:
    free(namesBytes);
    freeData:
    free(dataBytes);
    freeCode:
    rio_freePages(codeBytes, rio_codeSize);
    return err;
}

int main(int argc, const char** argv) {
    // TODO Better arg parse then call run with info struct.
    int scriptArg = 1;
    if (argc > 1) {
        if (!strcmp(argv[1], "--verbose")) {
            scriptArg += 1;
            rio_verbosity = 1;
        }
    }
    if (scriptArg >= argc) {
        fprintf(stderr, "No script given.\n");
        return rio_Err_bad;
    }
    const char* path = argv[scriptArg];
    return rio_run(path);
}
