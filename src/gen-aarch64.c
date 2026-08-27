#include "gen.h"
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

static const uint32_t instructions[] = {
    0x0b010000, // add w0, w0, w1
    0xd65f03c0, // ret
};

uint8_t* rio_addrForExec(uint8_t* addr) {
    return addr;
}

// rio_Gen rio_a64Gen(void) {
//     return (rio_Gen){
//         .argFloat = rio_a64ArgFloat,
//         .argInt = rio_a64ArgInt,
//         .argPtr = rio_a64ArgPtr,
//         .call = rio_a64Call,
//         .procStart = rio_a64ProcStart,
//         .procEnd = rio_a64ProcEnd,
//         .ret = rio_a64Ret,
//     };
// }

// rio_Err rio_a64ArgFloat(rio_Buffer_Byte* code, int index, float value) {
//     (void)code;
//     (void)index;
//     (void)value;
//     return 0;
// }

// rio_Err rio_a64ArgInt(rio_Buffer_Byte* code, int index, int32_t value) {
//     (void)code;
//     (void)index;
//     (void)value;
//     return 0;
// }

// rio_Err rio_a64ArgPtr(rio_Buffer_Byte* code, int index, intptr_t value) {
//     (void)code;
//     (void)index;
//     (void)value;
//     return 0;
// }

// rio_Err rio_a64Call(rio_Buffer_Byte* code, intptr_t proc) {
//     (void)code;
//     (void)proc;
//     return 0;
// }

// rio_Err rio_a64ProcStart(rio_Buffer_Byte* code) {
//     printf("----------> rio_a64ProcStart\n");
//     (void)code;
//     return 0;
// }

// rio_Err rio_a64ProcEnd(rio_Buffer_Byte* code, size_t start) {
//     printf("----------> rio_a64ProcEnd: %zu\n", start);
//     (void)code;
//     (void)start;
//     return 0;
// }

// rio_Err rio_a64Ret(rio_Buffer_Byte* code) {
//     // TODO
//     // >>> import re
//     // >>> re.sub(r"(.{4})(?=.)", r"\1_", f"{0xd65f03c0:032b}")
//     // '1101_0110_0101_1111_0000_00-11_110-0_0000'
//     (void)code;
//     return 0;
// }
