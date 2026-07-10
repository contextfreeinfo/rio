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

typedef struct rio_Parser {
    rio_Engine* engine;
    rio_Lexer lexer;
} rio_Parser;

rio_Err rio_parse(rio_Parser* parser);
