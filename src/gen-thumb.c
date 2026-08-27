static const uint16_t instructions[] = {
    0x4408, // add r0, r1
    0x4770, // bx lr
};

uint8_t* rio_addrForExec(uint8_t* addr) {
    // Offset by 1 for thumb.
    return addr + 1;
}
