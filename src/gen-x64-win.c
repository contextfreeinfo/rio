#include "gen.h"
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

static const uint8_t instructions[] = {
    0x8d, 0x04, 0x11, // lea eax, [rcx + rdx]
    0xc3,             // ret
};

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
