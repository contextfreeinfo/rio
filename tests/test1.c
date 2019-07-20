#include <stdint.h>
#include <stdio.h>

int main();

void greet();

const char* const message = "Hello";

int main() {
  const char* const name = "world";
  int32_t const age = 75;
  greet(name, age);
}

void greet(const char* const name, int32_t const age) {
  printf("%s, %s!\n", message, name);
  printf("You are %d years old.\n", age);
}
