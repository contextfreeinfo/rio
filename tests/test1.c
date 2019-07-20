#include <stdint.h>
#include <stdio.h>

int main();

const char* const message = "Hello";

int main() {
  const char* const name = "world";
  int32_t const age = 75;
  greet(name, age);
}
