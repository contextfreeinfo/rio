#include <stdint.h>
#include <stdio.h>

void tests_test1_greet();

int main() {
  const char* const name = "world";
  int32_t const age = 75;
  // TODO This needs namespaced.
  greet(name, age);
}

// TODO This needs namespaced.
const char* const message = "Hello";

void tests_test1_greet(const char* const name, int32_t const age) {
  // These need to stay top-level.
  printf("%s, %s!\n", message, name);
  printf("You are %d years old.\n", age);
}
