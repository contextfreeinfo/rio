#include <stdbool.h>

int addTwo(int a, int b) {
    return a + b;
}

int addThree(int a, int b, int c) {
    if (!c) {
        return addTwo(a, b);
    }
    return addTwo(addTwo(a, b), c);
}

int mulTwo(int a, int b) {
    return a * b;
}

bool check(int a, int b, int c) {
    return b >= a && b <= c;
}

void spin(int max, int step) {
    for (int i = 0; i < max; i += step) {}
}

void spinf(float max, float step) {
    for (float i = 0; i < max; i += step) {}
}

int main(void) {
    int sum = addTwo(3, 4);
    int prod = mulTwo(sum, 5);
    spin(10000000, 1);
    spinf(10000000.0f, 1.0f);
    // while(1);
    return prod;
}
