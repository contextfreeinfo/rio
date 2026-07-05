#pragma once

#include "sys.h"

typedef enum rio_TokenKind {
    rio_TokenKind_unknown,
} rio_TokenKind;

typedef struct rio_Token {
    rio_TokenKind kind;
} rio_Token;

rio_Err rio_lex(rio_File file);
