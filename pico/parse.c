#include "parse.h"
#include <stdbool.h>
#include <stdio.h>
#include "lex.h"

// uint8_t rio_archcode[rio_archcode_size] = {0};
// uint8_t rio_globals[rio_globals_size] = {0};
// uint16_t rio_names[rio_names_size] = {0};
// uint8_t rio_data[rio_data_size] = {0};

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

rio_Err rio_parseName(rio_Parser* parser) {
    printf("Name\n");
    parser->name = parser->lexer.token;
    parser->node = (rio_Node){ .kind = rio_NodeKind_name };
    return rio_parserAdvance(parser, false);
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
    parser->node = (rio_Node){ .kind = rio_NodeKind_proc };
    return err;
}

rio_Err rio_parseString(rio_Parser* parser) {
    rio_Err err = 0;
    rio_Buffer_Byte* buffer = &parser->engine->data;
    // Remember where we were for size later.
    rio_Buffer_Byte sizeBuffer = *buffer;
    if ((err = rio_pushBytesInt32(buffer, 0))) return err;
    // But put in address now.
    // TODO Use trampolines for native function pointers?
    if ((err = rio_pushBytesInt32(buffer, buffer->used + 4))) return err;
    int32_t start = buffer->used;
    rio_Token* token = &parser->lexer.token;
    while (true) {
        // The first advance gets past the open quote.
        if ((err = rio_parserAdvance(parser, false))) return err;
        switch (token->kind) {
        case rio_TokenKind_endLine:
            if ((err = rio_pushBytesByte(buffer, '\n'))) return err;
            goto done;
        case rio_TokenKind_stringClose:
            err = rio_parserAdvance(parser, false);
            goto done;
        case rio_TokenKind_stringEscape:
            // TODO Push unescaped.
            break;
        default:;
            // TODO Assert rio_TokenKind_stringText?
            rio_Span_Byte bytes = {
                .size = token->end - token->start,
                .items = (rio_Byte*)token->text,
            };
            if ((err = rio_pushBytes(buffer, bytes))) return err;
        }
    }
    done:
    // We know there's space here because we got past it.
    rio_pushBytesInt32(&sizeBuffer, buffer->used - start);
    // Pad to 4-byte alignment.
    if ((err = rio_pushBytesPad32(buffer))) return err;
    return err;
}

rio_Err rio_parseAtom(rio_Parser* parser) {
    rio_Err err = 0;
    parser->node = (rio_Node){ .kind = rio_NodeKind_nil };
    switch (parser->lexer.token.kind) {
    case rio_TokenKind_name:
        return rio_parseName(parser);
    case rio_TokenKind_proc:
        return rio_parseProc(parser);
    case rio_TokenKind_roundOpen:
        // TODO Parse parenthesized.
        return err;
    case rio_TokenKind_stringOpen:
        return rio_parseString(parser);
    default:
        return err;
    }
}

rio_Err rio_parseCall(rio_Parser* parser) {
    rio_Err err = rio_parseAtom(parser);
    if (err) return err;
    while (parser->lexer.token.kind == rio_TokenKind_roundOpen) {
        printf("Call start\n");
        if ((err = rio_parseTupleContent(parser))) return err;
        printf("Call end\n");
        parser->node = (rio_Node){ .kind = rio_NodeKind_call };
    }
    return err;
}

rio_Err rio_parseColon(rio_Parser* parser) {
    rio_Err err = rio_parseCall(parser);
    if (err) return err;
    printf("Checking for colon\n");
    if (parser->lexer.token.kind != rio_TokenKind_colon) return err;
    if ((err = rio_parserAdvance(parser, true))) return err;
    // Got a colon, so remember name.
    rio_Token nameToken =
        parser->node.kind == rio_NodeKind_name ? parser->name : (rio_Token){0};
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
    // Apply value.
    rio_Span_Byte name = {
        .size = nameToken.end - nameToken.start,
        .items = (rio_Byte*)nameToken.text,
    };
    if (name.size) {
        rio_Buffer_Byte* buffer = &parser->names;
        if ((err = rio_pushBytes(buffer, name))) return err;
        if ((err = rio_pushBytesByte(buffer, 0))) return err;
        // TODO If top-level, add to tops table.
        // TODO Handle whatever for the specific value node we got.
    }
    return rio_eatEndLines(parser);
}

rio_Err rio_parseExpression(rio_Parser* parser) {
    return rio_parseColon(parser);
}

rio_Err rio_parse(rio_Parser* parser) {
    // Prime the pump.
    rio_Err err = rio_parserAdvance(parser, true);
    if (err) return 0; // Presume empty for now.
    // Go until eof.
    // TODO Distinguish out of memory from end of file.
    size_t oldStart = parser->lexer.token.start;
    while (!rio_parseExpression(parser)) {
        if ((err = rio_parserEnsureAdvance(parser, oldStart))) goto done;
        oldStart = parser->lexer.token.start;
    }
    done:
    return 0;
}

void rio_reportParser(rio_Parser* parser) {
    printf("Names:\n");
    rio_Span_Byte data = parser->names.span;
    size_t used = parser->names.used;
    for (size_t index = 0; index < used; index += 1) {
        printf("%zu: %d\n", index, data.items[index]);
    }
    printf("\n");
    rio_reportEngine(parser->engine);
}
