#include <stdbool.h>
#include <stdio.h>
#include "gen.h"

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

rio_Err rio_genMovT(rio_Buffer_Byte* code, uint8_t rd, uint16_t imm16) {
    // Get bit regions.
    uint16_t imm4 = (imm16 >> 12) & 0x0f;
    uint16_t i = (imm16 >> 11) & 0x01;
    uint16_t imm3 = (imm16 >> 8) & 0x07;
    uint16_t imm8 = imm16 & 0xff;
    // Build instruction halves.
    // TODO Merge with movw, since only this constant is different?
    uint16_t upper = 0xf2c0 | (i << 10) | imm4;
    uint16_t lower = (imm3 << 12) | ((rd & 0xf) << 8) | imm8;
    // Push instructions.
    rio_Err err = 0;
    if ((err = rio_pushBytesInt16(code, upper))) return err;
    if ((err = rio_pushBytesInt16(code, lower))) return err;
    return 0;
}

rio_Err rio_genMovW(rio_Buffer_Byte* code, uint8_t rd, uint16_t imm16) {
    // Get bit regions.
    uint16_t imm4 = (imm16 >> 12) & 0x0f;
    uint16_t i = (imm16 >> 11) & 0x01;
    uint16_t imm3 = (imm16 >> 8) & 0x07;
    uint16_t imm8 = imm16 & 0xff;
    // Build instruction halves.
    uint16_t upper = 0xf240 | (i << 10) | imm4;
    uint16_t lower = (imm3 << 12) | ((rd & 0xf) << 8) | imm8;
    // Push instructions.
    rio_Err err = 0;
    if ((err = rio_pushBytesInt16(code, upper))) return err;
    if ((err = rio_pushBytesInt16(code, lower))) return err;
    return 0;
}

rio_Err checkPushR0(rio_Gen* gen) {
    if (gen->state) {
        gen->state = 0;
        return rio_pushBytesInt16(gen->code, (int16_t)0xb401);
    }
    return 0;
}

static rio_Err putReg(rio_Gen* gen, uint8_t rd, intptr_t value) {
    rio_Err err = 0;
    // TODO Cycle regs.
    uint32_t val = (uint32_t)value;
    uint16_t low = (uint16_t)(val & 0xffff);
    if (low <= 0xff && false) { // TODO drop false
        // TODO 8-bit mode.
        // TODO When set flags?
    } else if (low <= 0xfff && false) { // TODO drop false
        // TODO 12-bit mode.
        // TODO When set flags?
    } else { // 16-bit mode.
        if ((err = rio_genMovW(gen->code, rd, low))) return err;
    }
    uint16_t high = (uint16_t)(val >> 16);
    if (high) { // Whether any high bits.
        if ((err = rio_genMovT(gen->code, rd, high))) return err;
    }
    return 0;
}

rio_Err rio_genCall(rio_Gen* gen, intptr_t target, size_t arity) {
    rio_Err err = 0;
    if ((err = checkPushR0(gen))) return err;
    // TODO Pop args into place.
    (void)arity;
    intptr_t source = (intptr_t)(gen->code->span.items + gen->code->used + 4);
    intptr_t offsetBig = target ? target - source : 0;
    // Assert range limits, approximately 16Mi.
    if (!target || offsetBig < -(16 << 20) || offsetBig > (16 << 20) - 2) {
        if (target) {
            // BLX register 12. Expect bit 0 set to 1 in advance.
            if ((err = putReg(gen, 12, target))) return err;
        } else {
            // TODO Pop target to register 12.
        }
        return rio_pushBytesInt16(gen->code, 0x47e0);
    }
    int32_t offset = (int32_t)offsetBig;
    // // Clear the thumb bit.
    // // TODO Doesn't matter since we shift it out, anyway?
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
    uint16_t upper = 0xf000 | (s << 10) | imm10;
    uint16_t lower = 0xd000 | (j1 << 13) | (j2 << 11) | imm11;
    // Push instructions.
    if ((err = rio_pushBytesInt16(gen->code, upper))) return err;
    if ((err = rio_pushBytesInt16(gen->code, lower))) return err;
    // TODO If non-void, push return value.
    return 0;
}

rio_Err rio_genPopAsArgs(rio_Gen* gen, size_t count) {
    if (!count) return 0;
    rio_Err err = 0;
    // Max of 3 args.
    if (count > 3) count = 3;
    if (count == 1 && gen->state) {
        gen->state = 0;
    } else if (count) {
        // TODO Instead move last arg in place instead of pushing.
        if ((err = checkPushR0(gen))) return err;
        // Because we push first arg first, we can't pop all at once, because
        // thumb puts those in the wrong order.
        for (; count; count -= 1) {
            int16_t bits = (int16_t)0xbc00 | (1 << (count - 1));
            if ((err = rio_pushBytesInt16(gen->code, bits))) return err;
        }
    }
    return 0;
}

rio_Err rio_genProcBegin(rio_Gen* gen) {
    rio_Err err = 0;
    // push {r7, lr}
    // TODO sub sp, #[locals size] -- need to remember where this is
    // TODO add r7, sp, #0 -- why not mov???
    // TODO movs r[arg], #[local]
    // TODO ...args...
    // TODO b [after the end code]
    if ((err = rio_pushBytesInt16(gen->code, (int16_t)0xb580))) return err;
    return 0;
}

rio_Err rio_genProcEnd(rio_Gen* gen) {
    rio_Err err = 0;
    // TODO adds r7, #[locals size] -- all returns can branch here
    // TODO mov sp, r7
    // pop {r7, pc}
    if ((err = rio_pushBytesInt16(gen->code, (int16_t)0xbd80))) return err;
    return 0;
}

rio_Err rio_genPush(rio_Gen* gen, intptr_t value) {
    rio_Err err = 0;
    // printf("---------------------> genPush was %d\n", gen->state);
    if ((err = checkPushR0(gen))) return err;
    // Put r0 then push r0.
    // TODO Defer push of r0 in case the next instruction is pop r0?
    if ((err = putReg(gen, 0, value))) return err;
    gen->state = 1;
    return 0;
}

rio_Err rio_genUnusedPush(rio_Gen* gen) {
    gen->state = 0;
    return 0;
}
