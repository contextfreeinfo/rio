#include "lex.h"
#include <stdbool.h>

bool rio_isDigit(char c) {
    return c >= '0' && c <= '9';
}

bool rio_isNameStart(char c) {
    return
        c == '_' ||
        (c >= 'A' && c <= 'Z') ||
        (c >= 'a' && c <= 'z') ||
        // Not all of these are good for names, but unicode processing is big.
        c > 127
    ;
}

rio_Err rio_lexRead(rio_Lexer* lexer, char* c) {
    if (lexer->pending) {
        *c = lexer->pending;
        lexer->pending = 0;
        return 0;
    } else {
        return rio_read(lexer->file, c);
    }
}

rio_Err rio_lexFinishToken(rio_Lexer* lexer, rio_Err err, size_t size) {
    rio_Token* token = &lexer->token;
    token->text[size] = 0;
    token->end = token->start + size;
    if (err && !size) {
        return err;
    }
    return 0;
}

rio_Err rio_lexName(rio_Lexer* lexer, char start) {
    rio_Err err;
    rio_Token* token = &lexer->token;
    size_t size = 0;
    token->text[size++] = start;
    while (size < sizeof(token->text) - 1) {
        char c;
        err = rio_lexRead(lexer, &c);
        if (err) goto token_done;
        if (!(rio_isNameStart(c) || rio_isDigit(c))) {
            lexer->pending = c;
            goto token_done;
        }
        token->text[size++] = c;
    }
    token_done:
    return rio_lexFinishToken(lexer, err, size);
}

rio_Err rio_lexNewline(rio_Lexer* lexer, char start) {
    rio_Err err;
    rio_Token* token = &lexer->token;
    size_t size = 0;
    token->text[size++] = start;
    if (start == '\r') {
        char c;
        err = rio_lexRead(lexer, &c);
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
    return rio_lexFinishToken(lexer, err, size);
}

rio_Err rio_lexSpace(rio_Lexer* lexer, char start) {
    rio_Err err;
    rio_Token* token = &lexer->token;
    size_t size = 0;
    token->text[size++] = start;
    while (size < sizeof(token->text) - 1) {
        char c;
        err = rio_lexRead(lexer, &c);
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
    return rio_lexFinishToken(lexer, err, size);
}

rio_Err rio_lexModeString(rio_Lexer* lexer) {
    rio_Err err;
    rio_Token* token = &lexer->token;
    size_t size = 0;
    while (size < sizeof(token->text) - 1) {
        char c;
        err = rio_lexRead(lexer, &c);
        if (err) goto token_done;
        switch (c) {
        // TODO case '\\':
        // TODO case '\r': case '\n':
        case '"':
            if (size) {
                lexer->pending = c;
            } else {
                token->text[size++] = c;
                lexer->mode = rio_LexMode_default;
                token->kind = rio_TokenKind_stringOpen;
            }
            goto token_done;
        }
        token->text[size++] = c;
    }
    token_done:
    return rio_lexFinishToken(lexer, err, size);
}

rio_Err rio_lexNext(rio_Lexer* lexer) {
    rio_Err err;
    rio_Token* token = &lexer->token;
    token->start = token->end;
    switch (lexer->mode) {
    case rio_LexMode_default:
        break; // to below
    case rio_LexMode_string:
        return rio_lexModeString(lexer);
    }
    size_t size = 0;
    char c;
    err = rio_lexRead(lexer, &c);
    if (err) goto token_done;
    switch (c) {
    case ' ':
    case '\t':
        return rio_lexSpace(lexer, c);
    case '\r':
    case '\n':
        return rio_lexNewline(lexer, c);
    case '"':
        lexer->mode = rio_LexMode_string;
        token->kind = rio_TokenKind_stringOpen;
        break;
    default:
        if (rio_isNameStart(c)) {
            return rio_lexName(lexer, c);
        }
    }
    token->text[size++] = c;
    token_done:
    return rio_lexFinishToken(lexer, err, size);
}
