#include "gen.h"
#include "gen-aarch64.h"
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

rio_Gen rio_a64Gen(void) {
    return (rio_Gen){
        .arg = rio_a64Arg,
        .call = rio_a64Call,
        .ret = rio_a64Ret,
    };
}

rio_Err rio_a64Arg(rio_Buffer_Byte code, int32_t index, int32_t value) {
    (void)code;
    (void)index;
    (void)value;
    return 0;
}

rio_Err rio_a64Call(rio_Buffer_Byte code, int32_t proc) {
    (void)code;
    (void)proc;
    return 0;
}

rio_Err rio_a64Ret(rio_Buffer_Byte code) {
    (void)code;
    return 0;
}

static const uint32_t instructions[] = {
    0x0b010000, // add w0, w0, w1
    0xd65f03c0 // ret
};

rio_Err rio_genDemo(void) {
    rio_Err err = 0;
    long page_size = sysconf(_SC_PAGESIZE);
    if (page_size < 0) return rio_Err_bad;
    void* exec_mem = mmap( // (void*)-1 on fail then errno
        NULL, // addr where NULL means kernel chooses page-aligned address
        page_size, // length
        PROT_READ | PROT_WRITE, // prot can't be write and exec at the same time
        MAP_PRIVATE | MAP_ANONYMOUS, // flags: process-local, not-file-backed zero-init
        -1, // fd ignored for anonymous
        0 // offset in file must be multiple of page size
    );
    if (exec_mem == MAP_FAILED) return rio_Err_bad;
    memcpy(exec_mem, instructions, sizeof(instructions));
    int protect_bad = mprotect( // -1 for bad then errno
        exec_mem, // addr
        page_size, // len
        PROT_READ | PROT_EXEC // prot
    );
    if (protect_bad) {
        err = rio_Err_bad;
        goto done;
    }
    int (*add)(int, int) = (int (*)(int, int))(intptr_t)exec_mem;
    int result = add(3, 4);
    printf("Add result: %d\n", result);
    done:
    munmap(exec_mem, page_size);
    return err;
}
