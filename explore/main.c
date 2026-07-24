int add_two(int a, int b) {
    return a + b;
}

int mul_two(int a, int b) {
    return a * b;
}

int main(void) {
    int sum = add_two(3, 4);
    int prod = mul_two(sum, 5);
    // while(1);
    return prod;
}
