#include <stdint.h>
#include <stdio.h>

void tests_test1_greet();

const char* const tests_test1_message = "Hello";

int main() {
  const char* const name = "world";
  int32_t const age = 75;
  tests_test1_greet(name, age);
}

void tests_test1_greet(const char* const name, int32_t const age) {
  printf("%s, %s!\n", tests_test1_message, name);
  printf("You are %d years old.\n", age);
}
