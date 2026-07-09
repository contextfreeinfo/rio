#include "parse.h"
#include <stdio.h>
#include "lex.h"

uint8_t rio_archcode[rio_archcode_size] = {0};
uint8_t rio_globals[rio_globals_size] = {0};
uint16_t rio_names[rio_names_size] = {0};
uint8_t rio_text[rio_text_size] = {0};

typedef struct rio_Parser {
    rio_Lexer lexer;
} rio_Parser;

rio_Err rio_parserAdvance(rio_Parser* parser) {
    rio_Err err = 0;
    while (!(err = rio_lexNext(&parser->lexer))) {
        rio_Token token = parser->lexer.token;
        printf(
            "%s (%d); %zu..%zu\n",
            token.text,
            token.kind,
            token.start,
            token.end
        );
        if (token.kind != rio_TokenKind_space) goto done;
    }
    done:
    return err;
}

rio_Err rio_parseNameStatement(rio_Parser* parser) {
    rio_Err err = 0;
    printf("Got name: %s\n", parser->lexer.token.text);
    while (!(err = rio_parserAdvance(parser))) {
        rio_Token token = parser->lexer.token;
        if (token.kind == rio_TokenKind_endLine) goto statementDone;
    }
    statementDone:
    printf("Name statement done\n");
    return err;
}

rio_Err rio_parseStatement(rio_Parser* parser) {
    rio_Err err = 0;
    err = rio_parserAdvance(parser);
    if (err) return err;
    switch (parser->lexer.token.kind) {
    case rio_TokenKind_name:
        return rio_parseNameStatement(parser);
    default:
        break;
    }
    while (!(err = rio_parserAdvance(parser))) {
        rio_Token token = parser->lexer.token;
        if (token.kind == rio_TokenKind_endLine) goto statementDone;
    }
    statementDone:
    printf("Statement done\n");
    return err;
}

rio_Err rio_parse(rio_File file) {
    rio_Parser parser = { .lexer = { .file = file } };
    while (!rio_parseStatement(&parser)) {
        // Keep going.
    }
    return 0;
}
