#include "parse.h"
#include <stdio.h>
#include "lex.h"

uint8_t rio_archcode[rio_archcode_size] = {0};
uint8_t rio_bytecode[rio_bytecode_size] = {0};
uint16_t rio_names[rio_names_size] = {0};
uint8_t rio_text[rio_text_size] = {0};

rio_Err rio_parse(rio_File file) {
    rio_Lexer lexer = { .file = file };
    while (!rio_lexNext(&lexer)) {
        rio_Token token = lexer.token;
        printf(
            "%s (%d); %zu..%zu\n",
            token.text,
            token.kind,
            token.start,
            token.end
        );
    }
    return 0;
}
