#pragma once

#include <stdint.h>
#include "sys.h"

#define rio_archcode_size 0x100000
#define rio_bytecode_size rio_archcode_size
#define rio_names_size 0x10000
#define rio_text_size 0x10000

extern uint8_t rio_archcode[rio_archcode_size];
extern uint8_t rio_bytecode[rio_bytecode_size];
extern uint16_t rio_names[rio_names_size];
extern uint8_t rio_text[rio_text_size];

rio_Err rio_parse(rio_File file);
