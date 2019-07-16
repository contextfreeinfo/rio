#include <stdint.h>
#include <stdio.h>

int main();
void run();

const char* const message = "Hello";

int main() {
  run();
}

void run() {
  const char* const name = "world";
  int32_t const age = 75;
  printf("%s, %s!\n", message, name);
  printf("You are %d years old.\n", age);
}
