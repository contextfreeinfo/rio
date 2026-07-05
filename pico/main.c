#include <stdio.h>
#include <stdlib.h>
#include "engine.h"
#include "gen.h"
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
    rio_StdFile file = { .file = f };
    rio_Lexer lexer = { .file = &file };
    while (!rio_lex_next(&lexer)) {
        rio_Token token = lexer.token;
        printf("%s; %zu..%zu\n", token.text, token.start, token.end);
    }
    rio_close(&file);
    if ((err = rio_gen_demo())) return err;
    return 0;
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
