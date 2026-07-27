int add_two(int a, int b) {
    return a + b;
}

int mul_two(int a, int b) {
    return a * b;
}

void spin(int max, int step) {
    for (int i = 0; i < max; i += step) {}
}

int main(void) {
    int sum = add_two(3, 4);
    int prod = mul_two(sum, 5);
    spin(10000000, 1);
    // while(1);
    return prod;
}
