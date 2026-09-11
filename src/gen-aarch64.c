#include "gen.h"

uint8_t* rio_addrForExec(uint8_t* addr) {
    return addr;
}

rio_Err rio_memPushPtr(rio_Buffer_Byte* buffer, size_t offset) {
    rio_Err err = 0;
    size_t old = buffer->used;
    if ((err = rio_pushBytesPad64(buffer))) return err;
    offset += buffer->used - old;
    // Get an actual memory address.
    int64_t addr = (int64_t)((uint64_t)buffer->span.items + offset);
    return rio_pushBytesInt64Pre(buffer, addr);
}

rio_Err rio_genCall(rio_Gen* gen, intptr_t target, size_t arity) {
    (void)gen;
    (void)target;
    (void)arity;
    return 0;
}

rio_Err rio_genPopAsArgs(rio_Gen* gen, size_t count) {
    (void)gen;
    (void)count;
    return 0;
}

rio_Err rio_genProcBegin(rio_Gen* gen) {
    (void)gen;
    return 0;
}

rio_Err rio_genProcEnd(rio_Gen* gen) {
    (void)gen;
    return 0;
}

rio_Err rio_genPush(rio_Gen* gen, intptr_t value) {
    (void)gen;
    (void)value;
    return 0;
}

rio_Err rio_genUnusedPush(rio_Gen* gen) {
    gen->state = 0;
    return 0;
}
