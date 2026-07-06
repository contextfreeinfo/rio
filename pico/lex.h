#pragma once

#include <stdlib.h>
#include "sys.h"

typedef enum rio_TokenKind {
    rio_TokenKind_unknown,
    rio_TokenKind_colon,
    rio_TokenKind_endLine,
    rio_TokenKind_name,
    rio_TokenKind_roundOpen,
    rio_TokenKind_roundClose,
    rio_TokenKind_space,
    rio_TokenKind_stringOpen,
    rio_TokenKind_stringText,
    rio_TokenKind_stringEscape,
    rio_TokenKind_stringClose,
} rio_TokenKind;

typedef struct rio_Token {
    rio_TokenKind kind;
    size_t start;
    size_t end;
    char text[256];
} rio_Token;

typedef enum rio_LexMode {
    rio_LexMode_default,
    rio_LexMode_string,
} rio_LexMode;

typedef struct rio_Lexer {
    rio_File file;
    rio_LexMode mode;
    char pending; // Null char means none.
    rio_Token token;
} rio_Lexer;

rio_Err rio_lexNext(rio_Lexer* lexer);
