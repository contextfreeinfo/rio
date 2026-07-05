#pragma once

#include <stdlib.h>
#include "sys.h"

typedef enum rio_TokenKind {
    rio_TokenKind_unknown,
} rio_TokenKind;

typedef struct rio_Token {
    rio_TokenKind kind;
    size_t start;
    size_t end;
    char text[256];
} rio_Token;

typedef struct rio_Lexer {
    rio_File file;
    char pending; // Null char means none.
    rio_Token token;
} rio_Lexer;

rio_Err rio_lex_next(rio_Lexer* lexer);
