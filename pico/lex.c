#include "lex.h"
#include <stdbool.h>

bool rio_is_digit(char c) {
    return c >= '0' && c <= '9';
}

bool rio_is_name_start(char c) {
    return
        c == '_' ||
        (c >= 'A' && c <= 'Z') ||
        (c >= 'a' && c <= 'z') ||
        // Not all of these are good for names, but unicode processing is big.
        c > 127
    ;
}

rio_Err rio_lex_read(rio_Lexer* lexer, char* c) {
    if (lexer->pending) {
        *c = lexer->pending;
        lexer->pending = 0;
        return 0;
    } else {
        return rio_read(lexer->file, c);
    }
}

rio_Err rio_lex_finish_token(rio_Lexer* lexer, rio_Err err, size_t size) {
    rio_Token* token = &lexer->token;
    token->text[size] = 0;
    token->end = token->start + size;
    if (err && !size) {
        return err;
    }
    return 0;
}

rio_Err rio_lex_name(rio_Lexer* lexer, char start) {
    rio_Err err;
    rio_Token* token = &lexer->token;
    size_t size = 0;
    token->text[size++] = start;
    while (size < sizeof(token->text) - 1) {
        char c;
        err = rio_lex_read(lexer, &c);
        if (err) goto token_done;
        if (!(rio_is_name_start(c) || rio_is_digit(c))) {
            lexer->pending = c;
            goto token_done;
        }
        token->text[size++] = c;
    }
    token_done:
    return rio_lex_finish_token(lexer, err, size);
}

rio_Err rio_lex_newline(rio_Lexer* lexer, char start) {
    rio_Err err;
    rio_Token* token = &lexer->token;
    size_t size = 0;
    token->text[size++] = start;
    if (start == '\r') {
        char c;
        err = rio_lex_read(lexer, &c);
        if (err) goto token_done;
        switch (c) {
        case '\n':
            break;
        default:
            lexer->pending = c;
            goto token_done;
        }
        token->text[size++] = c;
    }
    token_done:
    return rio_lex_finish_token(lexer, err, size);
}

rio_Err rio_lex_space(rio_Lexer* lexer, char start) {
    rio_Err err;
    rio_Token* token = &lexer->token;
    size_t size = 0;
    token->text[size++] = start;
    while (size < sizeof(token->text) - 1) {
        char c;
        err = rio_lex_read(lexer, &c);
        if (err) goto token_done;
        switch (c) {
        case ' ':
        case '\t':
            break;
        default:
            lexer->pending = c;
            goto token_done;
        }
        token->text[size++] = c;
    }
    token_done:
    return rio_lex_finish_token(lexer, err, size);
}

rio_Err rio_lex_next(rio_Lexer* lexer) {
    rio_Err err;
    rio_Token* token = &lexer->token;
    token->start = token->end;
    size_t size = 0;
    char c;
    err = rio_lex_read(lexer, &c);
    if (err) goto token_done;
    switch (c) {
    case ' ':
    case '\t':
        return rio_lex_space(lexer, c);
    case '\r':
    case '\n':
        return rio_lex_newline(lexer, c);
    default:
        if (rio_is_name_start(c)) {
            return rio_lex_name(lexer, c);
        }
    }
    token->text[size++] = c;
    token_done:
    return rio_lex_finish_token(lexer, err, size);
}
