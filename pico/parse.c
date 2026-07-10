#include "parse.h"
#include <stdbool.h>
#include <stdio.h>
#include "lex.h"

uint8_t rio_archcode[rio_archcode_size] = {0};
uint8_t rio_globals[rio_globals_size] = {0};
uint16_t rio_names[rio_names_size] = {0};
uint8_t rio_data[rio_data_size] = {0};

typedef struct rio_Parser {
    rio_Lexer lexer;
} rio_Parser;

rio_Err rio_parserAdvance(rio_Parser* parser, bool skipEndLines) {
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
        switch (token.kind) {
        case rio_TokenKind_space:
            goto next;
        // case rio_TokenKind_comment:
        case rio_TokenKind_endLine:
            if (skipEndLines) goto next;
            break;
        default:
            break;
        }
        goto done;
        next:;
    }
    done:
    return err;
}

rio_Err rio_eatEndLines(rio_Parser* parser) {
    switch (parser->lexer.token.kind) {
    // case rio_TokenKind_comment:
    case rio_TokenKind_endLine:
    case rio_TokenKind_space:
        return rio_parserAdvance(parser, true);
    default:
        return 0;
    }
}

rio_Err rio_parserEnsureAdvance(rio_Parser* parser, size_t oldStart) {
    if (parser->lexer.token.start == oldStart) {
        printf("Had to advance\n");
        return rio_parserAdvance(parser, false);
    }
    return 0;
}

rio_Err rio_parseExpression(rio_Parser* parser);

rio_Err rio_parseTupleContent(rio_Parser* parser) {
    rio_Err err = 0;
    if ((err = rio_parserAdvance(parser, true))) return err;
    while (parser->lexer.token.kind != rio_TokenKind_roundClose) {
        size_t oldStart = parser->lexer.token.start;
        if ((err = rio_parseExpression(parser))) return err;
        if ((err = rio_parserEnsureAdvance(parser, oldStart))) return err;
        // TODO Check comma.
        if ((err = rio_eatEndLines(parser))) return err;
    }
    if ((err = rio_parserAdvance(parser, false))) return err;
    return err;
}

rio_Err rio_parseBlock(rio_Parser* parser) {
    rio_Err err = 0;
    if ((err = rio_eatEndLines(parser))) return err;
    while (parser->lexer.token.kind != rio_TokenKind_end) {
        size_t oldStart = parser->lexer.token.start;
        if ((err = rio_parseExpression(parser))) return err;
        if ((err = rio_parserEnsureAdvance(parser, oldStart))) return err;
        // TODO Check comma.
        if ((err = rio_eatEndLines(parser))) return err;
    }
    if ((err = rio_parserAdvance(parser, false))) return err;
    printf("Block end\n");
    return err;
}

rio_Err rio_parseProc(rio_Parser* parser) {
    rio_Err err = 0;
    if ((err = rio_parserAdvance(parser, true))) return err;
    if (parser->lexer.token.kind == rio_TokenKind_roundOpen) {
        printf("Params start\n");
        if ((err = rio_parseTupleContent(parser))) return err;
        printf("Params end\n");
    }
    if ((err = rio_parseBlock(parser))) return err;
    return err;
}

rio_Err rio_parseAtom(rio_Parser* parser) {
    rio_Err err = 0;
    switch (parser->lexer.token.kind) {
    case rio_TokenKind_name:
        printf("Name\n");
        if ((err = rio_parserAdvance(parser, false))) return err;
        // TODO Put the name somewhere.
        break;
    case rio_TokenKind_proc:
        return rio_parseProc(parser);
    case rio_TokenKind_roundOpen:
        // TODO Parse parenthesized.
        break;
    case rio_TokenKind_stringOpen:
        // TODO Parse string.
        break;
    default:
        break;
    }
    return err;
}

rio_Err rio_parseCall(rio_Parser* parser) {
    rio_Err err = rio_parseAtom(parser);
    if (err) return err;
    while (parser->lexer.token.kind == rio_TokenKind_roundOpen) {
        printf("Call start\n");
        if ((err = rio_parseTupleContent(parser))) return err;
        printf("Call end\n");
    }
    return err;
}

rio_Err rio_parseColon(rio_Parser* parser) {
    rio_Err err = rio_parseCall(parser);
    if (err) return err;
    printf("Checking for colon\n");
    if (parser->lexer.token.kind != rio_TokenKind_colon) return err;
    if ((err = rio_parserAdvance(parser, true))) return err;
    // Type or control flow.
    printf("Type or control flow\n");
    // TODO Eat newlines.
    // TODO Retain any name from earlier for definition.
    err = rio_parseCall(parser);
    if (err) return err;
    switch (parser->lexer.token.kind) {
    case rio_TokenKind_colon:
        if ((err = rio_parserAdvance(parser, true))) return err;
        break;
    // TODO case rio_TokenKind_eq:
    default:
        return err;
    }
    // Value.
    // TODO Eat newlines.
    printf("Value\n");
    err = rio_parseCall(parser);
    if (err) return err;
    // TODO Apply value.
    return rio_eatEndLines(parser);
}

rio_Err rio_parseExpression(rio_Parser* parser) {
    return rio_parseColon(parser);
}

rio_Err rio_parse(rio_File file) {
    rio_Parser parser = { .lexer = { .file = file } };
    // Prime the pump.
    rio_Err err = rio_parserAdvance(&parser, true);
    if (err) return 0; // Presume empty for now.
    // Go until eof.
    size_t oldStart = parser.lexer.token.start;
    while (!rio_parseColon(&parser)) {
        if ((err = rio_parserEnsureAdvance(&parser, oldStart))) goto done;
        oldStart = parser.lexer.token.start;
    }
    done:
    return 0;
}
