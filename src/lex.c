#include "lex.h"
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

// Char kinds.

bool rio_isDigit(uint8_t c) {
    return c >= '0' && c <= '9';
}

bool rio_isNameStart(uint8_t c) {
    return
        c == '_' ||
        (c >= 'A' && c <= 'Z') ||
        (c >= 'a' && c <= 'z') ||
        // Not all of these are good for names, but unicode processing is big.
        c > 127
    ;
}

// Helpers.

rio_Err rio_lexFinishToken(rio_Lexer* lexer, rio_Err err, size_t size) {
    rio_Token* token = &lexer->token;
    token->text[size] = 0;
    token->end = token->start + size;
    if (err && !size) {
        return err;
    }
    return 0;
}

rio_Err rio_lexRead(rio_Lexer* lexer, uint8_t* c) {
    if (lexer->pending) {
        *c = lexer->pending;
        lexer->pending = 0;
        return 0;
    } else {
        return rio_read(lexer->file, c);
    }
}

// Token kinds.

rio_Err rio_lexComment(rio_Lexer* lexer, uint8_t start) {
    rio_Err err;
    rio_Token* token = &lexer->token;
    token->kind = rio_TokenKind_comment;
    size_t size = 0;
    token->text[size++] = start;
    while (size < sizeof(token->text) - 1) {
        uint8_t c;
        err = rio_lexRead(lexer, &c);
        if (err) goto token_done;
        switch (c) {
        case '\r':
        case '\n':
            lexer->pending = c;
            goto token_done;
        default:
            break;
        }
        token->text[size++] = c;
    }
    token_done:
    return rio_lexFinishToken(lexer, err, size);
}

rio_Err rio_lexEndLine(rio_Lexer* lexer, uint8_t start) {
    rio_Err err = 0;
    rio_Token* token = &lexer->token;
    token->kind = rio_TokenKind_endLine;
    size_t size = 0;
    token->text[size++] = start;
    if (start == '\r') {
        uint8_t c;
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

rio_Err rio_lexName(rio_Lexer* lexer, uint8_t start) {
    rio_Err err;
    rio_Token* token = &lexer->token;
    token->kind = rio_TokenKind_name;
    size_t size = 0;
    token->text[size++] = start;
    while (size < sizeof(token->text) - 1) {
        uint8_t c;
        err = rio_lexRead(lexer, &c);
        if (err) goto token_done;
        if (!(rio_isNameStart(c) || rio_isDigit(c))) {
            lexer->pending = c;
            goto token_done;
        }
        token->text[size++] = c;
    }
    token_done:
    err = rio_lexFinishToken(lexer, err, size);
    // Find keywords. Switch first char maybe speeds up a little.
    switch (token->text[0]) {
    case 'd':
        if (!strcmp(token->text + 1, "eclare")) {
            token->kind = rio_TokenKind_declare;
        }
        break;
    case 'e':
        if (!strcmp(token->text + 1, "nd")) {
            token->kind = rio_TokenKind_end;
        }
        break;
    case 'f':
        if (!strcmp(token->text + 1, "or")) {
            token->kind = rio_TokenKind_for;
        }
        break;
    case 'p':
        if (!strcmp(token->text + 1, "roc")) {
            token->kind = rio_TokenKind_proc;
        }
        break;
    }
    return err;
}

rio_Err rio_lexNumber(rio_Lexer* lexer, uint8_t start) {
    rio_Err err = 0;
    rio_Token* token = &lexer->token;
    size_t size = 0;
    token->text[size++] = start;
    // Start by just consuming all chars that could possibly be part of a number
    // literal, even hex and other letters and float parts. Parse after that.
    // But purposely don't try to stop early in things like 123abc. It's written
    // as if one literal, so treat it as such, but error as needed.
    while (size < sizeof(token->text) - 1) {
        uint8_t c;
        err = rio_lexRead(lexer, &c);
        if (err) goto textDone;
        if (!(
            rio_isNameStart(c) || rio_isDigit(c) ||
            c == '.' || c == '+' || c == '-'
        )) {
            lexer->pending = c;
            goto textDone;
        }
        token->text[size++] = c;
    }
    textDone:
    // Get the text locked in.
    if ((err = rio_lexFinishToken(lexer, err, size))) return err;
    // But go ahead and get the numeric value now, too.
    // Handle int first.
    // TODO Check for starting 0b or 0x. (Or 0o???)
    // printf(">>>------> number text: %s\n", token->text);
    int32_t intValue = 0;
    for (size_t i = 0; i < size; i += 1) {
        uint8_t c = token->text[i];
        // TODO Also allow a/A through f/F if hex.
        // TODO Track errors.
        // Meanwhile, this does already correctly skip underscores.
        if (rio_isDigit(c)) {
            int32_t digit = c - '0';
            intValue = 10 * intValue + digit;
        } // else TODO End int part on dot or exponent.
    }
    // intDone:
    // TODO More parsing.
    token->kind = rio_TokenKind_int;
    token->intValue = intValue;
    // printf(">>>-------------------------L> int: %d vs %d\n", intValue, token->intValue);
    // tokenDone:
    return err;
}

rio_Err rio_lexSpace(rio_Lexer* lexer, uint8_t start) {
    rio_Err err;
    rio_Token* token = &lexer->token;
    token->kind = rio_TokenKind_space;
    size_t size = 0;
    token->text[size++] = start;
    while (size < sizeof(token->text) - 1) {
        uint8_t c;
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

// Lex modes.

rio_Err rio_lexModeString(rio_Lexer* lexer) {
    rio_Err err;
    rio_Token* token = &lexer->token;
    token->kind = rio_TokenKind_stringText;
    size_t size = 0;
    while (size < sizeof(token->text) - 1) {
        uint8_t c;
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
                token->kind = rio_TokenKind_stringClose;
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
    *token = (rio_Token){ .start = token->end };
    // Switch mode.
    switch (lexer->mode) {
    case rio_LexMode_default:
        break; // to below
    case rio_LexMode_string:
        return rio_lexModeString(lexer);
    }
    // Default mode.
    size_t size = 0;
    uint8_t c;
    err = rio_lexRead(lexer, &c);
    if (err) goto token_done;
    // Switch char.
    switch (c) {
    case ' ':
    case '\t':
        return rio_lexSpace(lexer, c);
    case '\r':
    case '\n':
        return rio_lexEndLine(lexer, c);
    case '#':
        return rio_lexComment(lexer, c);
    case '"':
        lexer->mode = rio_LexMode_string;
        token->kind = rio_TokenKind_stringOpen;
        break;
    case ':':
        token->kind = rio_TokenKind_colon;
        break;
    case ',':
        token->kind = rio_TokenKind_comma;
        break;
    case '=':
        token->kind = rio_TokenKind_eq;
        break;
    case '(':
        token->kind = rio_TokenKind_roundOpen;
        break;
    case ')':
        token->kind = rio_TokenKind_roundClose;
        break;
    default:
        if (rio_isDigit(c)) {
            return rio_lexNumber(lexer, c);
        } else if (rio_isNameStart(c)) {
            return rio_lexName(lexer, c);
        }
    }
    // Keep the char.
    token->text[size++] = c;
    token_done:
    return rio_lexFinishToken(lexer, err, size);
}
