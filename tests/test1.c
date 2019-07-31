#include <stdint.h>
#include <stdio.h>

// tests/test1-more.rio
void tests_test1_greet(const char* const name, int32_t const age);

// tests/test1-other.rio
const char* const tests_test1_other_message = "Hello";

// tests/test1.rio

int main() {
  const char* const name = "world";
  int32_t const age = 75;
  (!!! TYPE !!!) const scores = {(int32_t)[]){45, 63, 22, -8}, 4};
  tests_test1_greet(name, age);
}

// tests/test1-more.rio

void tests_test1_greet(const char* const name, int32_t const age) {
  printf("%s, %s!\n", tests_test1_other_message, name);
  printf("You are %d years old.\n", age);
}
