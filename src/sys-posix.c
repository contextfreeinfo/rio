#include <sys/mman.h>
#include <unistd.h>
#include "sys.h"

rio_Err rio_allocPages(size_t sizeBytes, uint8_t** memOut) {
    // We expect page size to be something common, but check just in case.
    // printf("rio_allocPages %zu, %lu\n", sizeBytes, sysconf(_SC_PAGESIZE));
    static int pageSize = 0;
    if (!pageSize) pageSize = sysconf(_SC_PAGESIZE);
    if (sizeBytes % pageSize) return rio_Err_bad;
    *memOut = NULL;
    uint8_t* mem = mmap( // (void*)-1 on fail then errno
        NULL, // addr where NULL means kernel chooses page-aligned address
        sizeBytes, // length
        PROT_READ | PROT_WRITE, // prot can't be write and exec at the same time
        MAP_PRIVATE | MAP_ANONYMOUS, // flags: process-local, not-file-backed zero-init
        -1, // fd ignored for anonymous
        0 // offset in file must be multiple of page size
    );
    if (mem == MAP_FAILED) return rio_Err_bad;
    *memOut = mem;
    return 0;
}

rio_Err rio_enableExec(uint8_t* mem, size_t sizeBytes) {
    int protect_bad = mprotect( // -1 for bad then errno
        mem, // addr
        sizeBytes, // len
        PROT_READ | PROT_EXEC // prot
    );
    if (protect_bad) return rio_Err_bad;
    return 0;
}

rio_Err rio_freePages(uint8_t* mem, size_t sizeBytes) {
    // TODO Use errno for details.
    if (munmap(mem, sizeBytes)) return rio_Err_bad;
    return 0;
}
