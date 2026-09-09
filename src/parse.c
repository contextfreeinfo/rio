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
        case rio_TokenKind_comment:
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
    case rio_TokenKind_comment:
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
    rio_Err err = 0;
    printf("Name\n");
    int32_t index = 0;
    // All our token texts also are null-terminated.
    rio_Byte* text = (rio_Byte*)parser->lexer.token.text;
    if ((err = rio_table(&parser->names, text, &index))) return err;
    parser->node = (rio_Node){
        .kind = rio_NodeKind_name,
        .value = {.name = {.name = index}},
    };
    // if (!hold) {
    //     // TODO Push value to code gen.
    // }
    rio_Def* def = rio_findDef(&parser->engine->defs, index);
    if (def) {
        printf("---> found: %d\n", index);
        if (def->constant) {
            // TODO Handle different types.
            if ((err = rio_genPush(&parser->gen, def->ptrVal))) return err;
        } else if (def->local) {
            // TODO Handle different types.
            // TODO Get value from frame-relative address.
        } else {
            // TODO Handle different types.
            // TODO Get value from address.
        }
    }
    return rio_parserAdvance(parser, false);
}

rio_Err rio_parseProcProto(rio_Parser* parser) {
    rio_Err err = 0;
    if ((err = rio_parserAdvance(parser, true))) return err;
    if (parser->lexer.token.kind == rio_TokenKind_roundOpen) {
        printf("Params start\n");
        if ((err = rio_parseTupleContent(parser))) return err;
        printf("Params end\n");
    }
    parser->node = (rio_Node){ .kind = rio_NodeKind_proc };
    return err;
}

rio_Err rio_parseProc(rio_Parser* parser) {
    rio_Err err = 0;
    size_t start = parser->engine->code.used;
    // if ((err = parser->gen.procStart(&parser->engine->code))) return err;
    if ((err = rio_parseProcProto(parser))) return err;
    if ((err = rio_parseBlock(parser))) return err;
    // if ((err = parser->gen.procEnd(&parser->engine->code, start))) return err;
    if ((err = rio_genRet(&parser->gen))) return err;
    parser->node = (rio_Node){ .kind = rio_NodeKind_proc, .start = start };
    return err;
}

rio_Err rio_parseDeclare(rio_Parser* parser) {
    rio_Err err = 0;
    if ((err = rio_parserAdvance(parser, true))) return err;
    parser->node = (rio_Node){ .kind = rio_NodeKind_nil };
    // We can declare either procs or types without defining them.
    switch (parser->lexer.token.kind) {
    case rio_TokenKind_proc:
        return rio_parseProcProto(parser);
    // case rio_TokenKind_struct:
    //     // TODO Just consume the struct token?
    //     return err;
    default:
        return err;
    }
}

rio_Err rio_parseString(rio_Parser* parser) {
    rio_Err err = 0;
    rio_Buffer_Byte* buffer = &parser->engine->data;
    // Push address now.
    if ((err = rio_memPushPtr(buffer, buffer->used + rio_ptrSize + 4))) {
        return err;
    }
    // Remember where we were for size later.
    rio_Buffer_Byte sizeBuffer = *buffer;
    if ((err = rio_pushBytesInt32(buffer, 0))) return err;
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
    intptr_t address = (intptr_t)(buffer->span.items + start);
    if ((err = rio_genPush(&parser->gen, address))) return err;
    return err;
}

rio_Err rio_parseAtom(rio_Parser* parser) {
    rio_Err err = 0;
    parser->node = (rio_Node){ .kind = rio_NodeKind_nil };
    switch (parser->lexer.token.kind) {
    case rio_TokenKind_declare:
        return rio_parseDeclare(parser);
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
    int32_t name = 0;
    if (parser->node.kind == rio_NodeKind_name) {
        name = parser->node.value.name.name;
        // TODO If we get a constant call address, avoid instructions for
        // TODO pushing callee address in the first place?
    }
    while (parser->lexer.token.kind == rio_TokenKind_roundOpen) {
        // TODO Validate callee type. Get arity from it.
        // TODO From rio, always describe types through handles to prevent mut?
        size_t arity = 0;
        printf("Call %d start\n", name);
        if ((err = rio_parseTupleContent(parser))) return err;
        printf("Call %d end\n", name);
        parser->node = (rio_Node){ .kind = rio_NodeKind_call };
        rio_Def* def = rio_findDef(&parser->engine->defs, name);
        intptr_t target = def ? def->ptrVal : 0;
        if ((err = rio_genCall(&parser->gen, target, arity))) return err;
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
    rio_Node nameNode = parser->node;
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
    if (nameNode.kind == rio_NodeKind_name) {
        rio_Byte* name;
        rio_tabled(&parser->names, nameNode.value.name.name, &name);
        // TODO If top-level, add to tops table.
        // TODO Handle whatever for the specific value node we got.
        rio_Def def = {
            .name = nameNode.value.name.name,
            .constant = true,
            .type = NULL, // TODO Actual type description pointer.
        };
        switch (parser->node.kind) {
        case rio_NodeKind_proc:
            def.ptrVal = (intptr_t)(
                rio_addrForExec(
                    parser->node.start + parser->engine->code.span.items
                )
            );
            if ((err = rio_pushDef(&parser->engine->defs, def))) return err;
            printf(
                "Defined proc: %s (%d) %zu at 0x%x -> %p\n",
                name,
                def.name,
                parser->engine->defs.used - 1,
                (uint32_t)parser->node.start,
                (void*)def.ptrVal
            );
            break;
        default:;
        }
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
    rio_Span_UInt16 starts = parser->names.starts;
    rio_Span_Byte strings = parser->names.strings.span;
    // TODO Report names as strings with start.
    for (size_t index = 0; index < starts.size; index += 1) {
        size_t start = starts.items[index];
        if (start) {
            printf("%zu@%zu: %s\n", index, start, strings.items + start);
        }
    }
    printf("\n");
    rio_reportEngine(parser->engine);
}
