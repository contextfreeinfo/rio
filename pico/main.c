#include <stdio.h>
#include <stdlib.h>
#include "engine.h"
#include "sys-std.h"

int main(int argc, const char** argv) {
    if (argc > 1) {
        const char* path = argv[1];
        FILE* f = fopen(path, "r");
        if (!f) {
            rio_log("Failed to open:");
            rio_log(path);
            return rio_Err_bad;
        }
        rio_File file;
        if (rio_file_make(f, &file)) {
            fclose(f);
            return rio_Err_bad;
        }
        rio_Lexer lexer = { .file = file };
        while (!rio_lex_next(&lexer)) {
            rio_log(lexer.token.text);
        }
        rio_close(file);
    }
    return 0;
}
