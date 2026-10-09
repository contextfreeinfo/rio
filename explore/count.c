// gcc -std=c2x -O0 count.c && time ./a.out

#include <stdint.h>

int32_t main(void) {
    for (int32_t i = 0; i < 1'000'000'000; i += 1) {}
}
