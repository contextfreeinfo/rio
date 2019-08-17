#include <stdint.h>
#include <stdio.h>

// tests/test1-other.rio

const char* const tests_test1_other_message = "Hello";

// tests/test1.rio

typedef struct tests_test1_Person {
  const char* name;
  int32_t age;
} tests_test1_Person;

typedef struct rio_Span_i32 {
  int32_t* items;
  size_t len;
} rio_Span_i32;

typedef struct rio_Span_string {
  const char** items;
  size_t len;
} rio_Span_string;

void tests_test1_greet(const char* const name, int32_t const age);
void tests_test1_greet_person(tests_test1_Person const person);

int main() {
  const char* const name = "world";
  int32_t const age = 75;
  rio_Span_i32 const scores = {(int32_t[]){45, 63, 22, -8}, 4};
  rio_Span_i32 const mores = {(int32_t[]){1}, 1};
  rio_Span_string const words = {(const char*[]){"hi", "there"}, 2};
  {
    auto rio_for_list = words;
    for (size_t rio_for_index = 0; rio_for_index < rio_for_list.len; rio_for_index += 1) {
      auto word = rio_for_list[rio_for_index];
      printf("word: %s\n", word);
    }
  }
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
