#include <stdbool.h>
#include <stdio.h>
#include "gen.h"

uint8_t* rio_addrForExec(uint8_t* addr) {
    // Offset by 1 for thumb.
    return addr + 1;
}

rio_Err rio_memPadPtr(rio_Buffer_Byte* buffer) {
    return rio_pushBytesPad32(buffer);
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

#define rio_genThumbDump false
// #define rio_genThumbDump true

static rio_Err pushCode(rio_Buffer_Byte* buffer, uint16_t code) {
    #if rio_genThumbDump
        printf("-------------- code[%04x] -> 0x%04x\n", buffer->used, code);
    #endif
    return rio_pushBytesInt16(buffer, code);
}

static rio_Err pushCodes(
    rio_Buffer_Byte* buffer, uint16_t* codes, size_t count
) {
    #if rio_genThumbDump
        for (size_t i = 0; i < count; i += 1) {
            size_t at = buffer->used + i * 2;
            printf("-------------- code[%04x] -> 0x%04x\n", at, codes[i]);
        }
    #endif
    rio_Span_Byte bytes = {.size = count * 2, .items = (uint8_t*)codes};
    return rio_pushBytes(buffer, bytes);
}

rio_Err rio_genMovT(rio_Buffer_Byte* code, uint8_t rd, uint16_t imm16) {
    // Get bit regions.
    uint16_t imm4 = (imm16 >> 12) & 0x0f;
    uint16_t i = (imm16 >> 11) & 0x01;
    uint16_t imm3 = (imm16 >> 8) & 0x07;
    uint16_t imm8 = imm16 & 0xff;
    // Build instruction halves.
    // TODO Merge with movw, since only this constant is different?
    uint16_t codes[] = {
        0xf2c0 | (i << 10) | imm4,
        (imm3 << 12) | ((rd & 0xf) << 8) | imm8,
    };
    // Push instructions.
    rio_Err err = 0;
    if ((err = pushCodes(code, codes, 2))) return err;
    return 0;
}

rio_Err rio_genMovW(rio_Buffer_Byte* code, uint8_t rd, uint16_t imm16) {
    // Get bit regions.
    uint16_t imm4 = (imm16 >> 12) & 0x0f;
    uint16_t i = (imm16 >> 11) & 0x01;
    uint16_t imm3 = (imm16 >> 8) & 0x07;
    uint16_t imm8 = imm16 & 0xff;
    // Build instruction halves.
    uint16_t codes[] = {
        0xf240 | (i << 10) | imm4,
        (imm3 << 12) | ((rd & 0xf) << 8) | imm8,
    };
    // Push instructions.
    rio_Err err = 0;
    if ((err = pushCodes(code, codes, 2))) return err;
    return 0;
}

rio_Err checkPushR0(rio_Gen* gen) {
    if (gen->state) {
        // printf("---------------------> pushR0\n");
        gen->state = 0;
        return pushCode(gen->code, (int16_t)0xb401);
    }
    return 0;
}

static rio_Err putReg(rio_Gen* gen, uint8_t rd, intptr_t value) {
    rio_Err err = 0;
    // TODO Cycle regs.
    uint32_t val = (uint32_t)value;
    uint16_t low = (uint16_t)(val & 0xffff);
    if (low <= 0xff && value >= 0) { // TODO MVNS for negative values
        uint16_t mov = 0x2000 | (rd << 8) | low;
        // Sets flags, but meh.
        if ((err = pushCode(gen->code, mov))) return err;
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

static rio_Err genBranchW(
    rio_Buffer_Byte* code, uint16_t lower, int32_t offset
) {
    rio_Err err = 0;
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
    // Push two halves.
    uint16_t codes[] = {
        0xf000 | (s << 10) | imm10,
        lower | (j1 << 13) | (j2 << 11) | imm11,
    };
    if ((err = pushCodes(code, codes, 2))) return err;
    // TODO If non-void, push return value.
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
        return pushCode(gen->code, 0x47e0);
    }
    // bl
    int32_t offset = (int32_t)offsetBig;
    if ((err = genBranchW(gen->code, 0xd000, offset))) return err;
    // TODO If non-void, push return value.
    return 0;
}

rio_Err rio_genPopAsArgs(rio_Gen* gen, size_t count) {
    if (!count) return 0;
    rio_Err err = 0;
    // Max of 3 args.
    // printf("---------------------> genPopAsArgs was %d, %d\n", gen->state, count);
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
            if ((err = pushCode(gen->code, bits))) return err;
        }
    }
    return 0;
}

rio_Err rio_genProcBegin(
    rio_Gen* gen, uint8_t paramCount, uint8_t** returnAddress
) {
    rio_Err err = 0;
    // Store the address for branching to for return from the procedure.
    // TODO If a big array push, we could calculate this based on an offset.
    // And the return address is after the first 4 codes below, or 8 bytes.
    *returnAddress = gen->code->span.items + gen->code->used + 8;
    // TODO Move this array to global space?
    uint16_t codes[] = {
        // push {r7, lr}
        0xb580,
        // sub sp, #[frame size in words]
        // Use param count as placeholder for frame size for now.
        // TODO Just say 0 frame size at start?
        // Max frame size of 127 words. TODO Validate this limit in parsing???
        // That's up to 508 bytes, which is about 1/4th of rp2350 stack size.
        0xb080 | paramCount,
        // mov r7, sp
        0x466f,
        // Branch past return code.
        0xe002,
        // Add return logic early, so we already know where to branch to.
        // This logic differs from gcc, which does `adds r7, #` then `mov sp, r7`.
        // But that doesn't get the automatic x4 that we get here.
        // This logic also reflects better the push frame instructions above.
        // TODO Merge sequential writes into single array push?
        // mov sp, r7
        0x46bd,
        // add sp, #[frame size in words]
        // TODO Also always just start with 0?
        0xb000 | paramCount,
        // pop {r7, pc}
        0xbd80,
    };
    if ((err = pushCodes(gen->code, codes, 7))) return err;
    // Here's where we need to branch to on begin.
    // Push args here so all the start and end code above is fixed size.
    for (uint16_t param = 0; param < paramCount; param += 1) {
        // TODO movs rn, [r7, #param]
        // Where p is multiplied by 4 automatically.
        // 0b01100_ppppp_111_nnn
        uint16_t store = 0x6038 | (param << 6) | param;
        if ((err = pushCode(gen->code, store))) return err;
    }
    return 0;
}

rio_Err rio_genProcEnd(rio_Gen* procBeginGen, uint16_t frameSize) {
    rio_Err err = 0;
    // Frame size here is a multiple of 4, so round up.
    frameSize = (frameSize + 3) >> 2;
    // Max of 127 words. TODO Validate this limit in parsing???
    // That's up to 508 bytes, which is about 1/4th of the rp2350 stack size.
    if (frameSize > 0xff) return rio_Err_bad;
    // int16_t sub = 0xb080 | frameSize;
    // sub sp, #[frame size in words]
    (void)err;
    (void)procBeginGen;
    // if ((err = rio_pushBytesInt16(procBeginGen->code, sub))) return err;
    return 0;
}

rio_Err rio_genRet(rio_Gen* gen, uint8_t* returnAddress) {
    rio_Err err = 0;
    // TODO On wasm, this might ignore returnAddress entirely.
    intptr_t source = (intptr_t)(gen->code->span.items + gen->code->used + 4);
    intptr_t offsetBig = (intptr_t)returnAddress - source;
    int32_t offset = (int32_t)offsetBig;
    if (offset >= -2048 && offset <= 2046) {
        // Small branch.
        offset >>= 1;
        uint32_t imm11 = offset & 0x7ff;
        uint16_t branch = 0xe000 | imm11;
        if ((err = pushCode(gen->code, branch))) return err;
        return 0;
    }
    // b.w
    if ((err = genBranchW(gen->code, 0x9000, offset))) return err;
    return 0;
}

rio_Err rio_genPush(rio_Gen* gen, intptr_t value) {
    rio_Err err = 0;
    // printf("---------------------> genPush was %d, %x\n", gen->state, value);
    if ((err = checkPushR0(gen))) return err;
    // Put r0 then push r0.
    // TODO Defer push of r0 in case the next instruction is pop r0?
    if ((err = putReg(gen, 0, value))) return err;
    gen->state = 1;
    return 0;
}

rio_Err rio_genFlushPush(rio_Gen* gen) {
    return checkPushR0(gen);
}

rio_Err rio_genUnusedPush(rio_Gen* gen) {
    gen->state = 0;
    return 0;
}
