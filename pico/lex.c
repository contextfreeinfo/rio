#include "lex.h"

rio_Err rio_lex_read(rio_Lexer* lexer, char* c) {
    if (lexer->pending) {
        *c = lexer->pending;
        lexer->pending = 0;
        return 0;
    } else {
        return rio_read(lexer->file, c);
    }
}

rio_Err rio_lex_next(rio_Lexer* lexer) {
    rio_Err err;
    rio_Token* token = &lexer->token;
    token->start = token->end;
    size_t size = 0;
    while (size < sizeof(token->text) - 1) {
        char c;
        err = rio_lex_read(lexer, &c);
        if (err) break;
        switch (c) {
        case ' ':
        case '\t':
        case '\r':
        case '\n':
            // TODO Something different with newlines.
            if (size) {
                lexer->pending = c;
            } else {
                token->text[size++] = c;
            }
            goto token_done;
        }
        token->text[size++] = c;
    }
    token_done:
    token->text[size] = 0;
    token->end = token->start + size;
    if (err && !size) {
        return err;
    }
    return 0;
}
