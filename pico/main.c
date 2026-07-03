#include <stdio.h>
#ifdef PICO_BOARD
    #include "pico/stdlib.h"
    // #include "tusb.h"
#endif

int main() {
#ifdef PICO_BOARD
    stdio_init_all();
#endif
    printf("Hi there!\n");
    printf("Bye y'all!\n");
    return 0;
}
