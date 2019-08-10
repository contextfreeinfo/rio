#include <stdint.h>
#include <stdio.h>

// tests/test1-other.rio

const char* const tests_test1_other_message = "Hello";

// tests/test1.rio

typedef struct tests_test1_Person {
  const char* name;
  int32_t age;
} tests_test1_Person;

void tests_test1_greet(const char* const name, int32_t const age);
void tests_test1_greet_person(tests_test1_Person const person);

int main() {
  const char* const name = "world";
  int32_t const age = 75;
  tests_test1_greet(name, age);
  tests_test1_greet_person((tests_test1_Person){.name = name, .age = 80});
}

void tests_test1_greet(const char* const name, int32_t const age) {
  printf("%s, %s!\n", tests_test1_other_message, name);
  printf("You are %d years old.\n", age);
}

void tests_test1_greet_person(tests_test1_Person const person) {
  printf("%s is %d years old.\n", person.name, person.age);
}
