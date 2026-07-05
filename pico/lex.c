#include "lex.h"

rio_Err rio_lex_next(rio_Lexer* lexer) {
    rio_Err err;
    rio_Token* token = &lexer->token;
    token->start = token->end;
    size_t size = 0;
    while (size < sizeof(token->text) - 1) {
        char c;
        err = rio_read(lexer->file, &c);
        if (err) break;
        token->text[size++] = c;
    }
    token->text[size] = 0;
    token->end = token->start + size;
    if (err && !size) {
        return err;
    }
    return 0;
}
