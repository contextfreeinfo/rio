#include <stdint.h>
#include <stdio.h>

const char* const message = "Hello";
void main() {
  const char* const name = "world";
  const char* const age = 75;
  printf("%s, %s!\n", message, name);
  printf("You are %d years old.\n", age);
}
