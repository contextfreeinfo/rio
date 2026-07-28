#pragma once

#include "gen.h"

rio_Gen rio_a64Gen(void);

rio_Err rio_a64ArgFloat(rio_Buffer_Byte* code, int index, float value);

rio_Err rio_a64ArgInt(rio_Buffer_Byte* code, int index, int32_t value);

rio_Err rio_a64ArgPtr(rio_Buffer_Byte* code, int index, intptr_t value);

rio_Err rio_a64Call(rio_Buffer_Byte* code, intptr_t proc);

rio_Err rio_a64ProcStart(rio_Buffer_Byte* code);

rio_Err rio_a64ProcEnd(rio_Buffer_Byte* code, size_t start);

rio_Err rio_a64Ret(rio_Buffer_Byte* code);
