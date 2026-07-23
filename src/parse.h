#pragma once

#include <stdint.h>
#include "engine.h"
#include "lex.h"
#include "sys.h"

// TODO Dynamically allocate these arrays at start?
// #define rio_archcode_size 0x100000
// #define rio_data_size 0x100000
// #define rio_globals_size 0x1000
// #define rio_names_size 0x10000
// extern uint8_t rio_archcode[rio_archcode_size];
// extern uint8_t rio_globals[rio_globals_size]; // TODO Mix with names?
// // If each module top-level or even field def ends up here, are we good?
// extern uint16_t rio_names[rio_names_size];
// extern uint8_t rio_data[rio_data_size];

#define rio_namesSize 0x10000

typedef enum rio_NodeKind {
    rio_NodeKind_nil,
    rio_NodeKind_call,
    rio_NodeKind_name,
    rio_NodeKind_proc,
} rio_NodeKind;

typedef struct rio_Node_Name {
    int32_t name;
} rio_Node_Name;

typedef struct rio_Node {
    rio_NodeKind kind;
    union {
        rio_Node_Name name;
    } value;
} rio_Node;

typedef struct rio_Parser {
    rio_Engine* engine;
    rio_Lexer lexer;
    rio_Table names;
    rio_Node node; // For returning up the parse stack.
} rio_Parser;

rio_Err rio_parse(rio_Parser* parser);

void rio_reportParser(rio_Parser* parser);
