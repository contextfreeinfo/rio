#include <stdio.h>
#include <stdlib.h>
#include "sys-std.h"

rio_Err rio_close(rio_File file) {
    rio_StdioFile* f = file;
    if (f->file) {
        fclose(f->file);
    }
    free(f);
    return 0;
}

rio_Err rio_log(const char* message) {
    puts(message);
    return 0;
}

rio_Err rio_file_make(FILE* file, rio_File* result) {
    rio_StdioFile* f = malloc(sizeof(rio_StdioFile));
    if (!f) {
        return rio_Err_bad;
    }
    f->file = file;
    f->start = 0;
    f->end = 0;
    *result = f;
    return 0;
}

rio_Err rio_read(rio_File file, char* c) {
    rio_StdioFile* f = file;
    if (f->start >= f->end && f->file) {
        size_t bytes = fread(f->buffer, 1, sizeof(f->buffer), f->file);
        if (bytes < sizeof(f->buffer)) {
            // TODO Check for error other than eof?
            fclose(f->file);
            f->file = NULL;
        }
        f->start = 0;
        f->end = bytes;
    }
    if (f->start >= f->end) {
        return rio_Err_eof;
    }
    *c = f->buffer[f->start++];
    return 0;
}
