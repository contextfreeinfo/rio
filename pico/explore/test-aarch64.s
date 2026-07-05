.global _start
.global add_two

add_two:
    add w0, w0, w1
    ret

_start:
    mov w0, #3
    mov w1, #4
    bl add_two
    mov x8, #93 // linux exit system call number?
    svc #0 // call the kernel
