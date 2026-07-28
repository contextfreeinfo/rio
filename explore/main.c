int addTwo(int a, int b) {
    return a + b;
}

int addThree(int a, int b, int c) {
    return addTwo(addTwo(a, b), c);
}

int mulTwo(int a, int b) {
    return a * b;
}

void spin(int max, int step) {
    for (int i = 0; i < max; i += step) {}
}

int main(void) {
    int sum = addTwo(3, 4);
    int prod = mulTwo(sum, 5);
    spin(10000000, 1);
    // while(1);
    return prod;
}
