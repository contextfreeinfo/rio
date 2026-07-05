.syntax unified
.cpu cortex-m33
.thumb

.global _start
_start:
    movs r0, #3
    movs r1, #4
    bl add_two
    loop_forever:
        b loop_forever

.global add_two
.type add_two, %function
add_two:
    add r0, r0, r1
    bx lr
