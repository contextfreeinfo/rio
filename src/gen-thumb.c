#include "gen.h"

static const uint16_t instructions[] = {
    0x4408, // add r0, r1
    0x4770, // bx lr
};

uint8_t* rio_addrForExec(uint8_t* addr) {
    // Offset by 1 for thumb.
    return addr + 1;
}

rio_Err rio_memPushPtr(rio_Buffer_Byte* buffer, size_t offset) {
    rio_Err err = 0;
    size_t old = buffer->used;
    if ((err = rio_pushBytesPad32(buffer))) return err;
    offset += buffer->used - old;
    // Get an actual memory address.
    int32_t addr = (int32_t)((uint32_t)buffer->span.items + offset);
    return rio_pushBytesInt32Pre(buffer, addr);
}

rio_Err rio_genCall(rio_Gen* gen, intptr_t target) {
    intptr_t source = (intptr_t)(gen->code->span.items + gen->code->used + 4);
    intptr_t offsetBig = target - source;
    // Assert range limits, approximately 16Mi.
    if (offsetBig < -(16 << 20)) return rio_Err_bad;
    if (offsetBig > (16 << 20) - 2) return rio_Err_bad;
    int32_t offset = (int32_t)offsetBig;
    // // Clear the thumb bit.
    // offset &= ~1;
    // Convert to half-word offset.
    offset >>= 1;
    // Split bits.
    uint32_t s = (offset >> 23) & 1;
    uint32_t i1 = (offset >> 22) & 1;
    uint32_t i2 = (offset >> 21) & 1;
    uint32_t imm10 = (offset >> 11) & 0x3ff;
    uint32_t imm11 = offset & 0x7ff;
    // Calculate j1 and j2 using "arm scramble formula".
    uint32_t j1 = i1 ^ s ^ 1;
    uint32_t j2 = i2 ^ s ^ 1;
    // Build two halves then full.
    uint16_t upper = 0xF000 | (s << 10) | imm10;
    uint16_t lower = 0xD000 | (j1 << 13) | (1 << 12) | (j2 << 11) | imm11;
    // Push instructions.
    rio_Err err = 0;
    if ((err = rio_pushBytesInt16(gen->code, upper))) return err;
    if ((err = rio_pushBytesInt16(gen->code, lower))) return err;
    return 0;
}
