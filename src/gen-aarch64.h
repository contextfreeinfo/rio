#pragma once

#include "gen.h"

rio_Gen rio_a64Gen(void);

rio_Err rio_a64Arg(rio_Buffer_Byte code, int32_t index, int32_t value);

rio_Err rio_a64Call(rio_Buffer_Byte code, int32_t proc);

rio_Err rio_a64ProcStart(rio_Buffer_Byte code);

rio_Err rio_a64ProcEnd(rio_Buffer_Byte code, size_t start);

rio_Err rio_a64Ret(rio_Buffer_Byte code);
