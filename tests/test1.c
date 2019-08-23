#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

typedef double rio_float;
typedef ptrdiff_t rio_int;

// tests/test1-other.rio

const char* const tests_test1_other_message = "Hello";

// tests/test1.rio

typedef struct tests_test1_Person {
  const char* name;
  rio_int age;
} tests_test1_Person;

typedef struct rio_Span_int {
  rio_int* items;
  rio_int len;
} rio_Span_int;

typedef struct rio_Span_string {
  const char** items;
  rio_int len;
} rio_Span_string;

void tests_test1_greet(const char* const name, rio_int const age);
void tests_test1_greet_person(tests_test1_Person const person);
void tests_test1_report_scores(rio_Span_int const scores);

int main() {
  const char* const name = "world";
  rio_int const age = 75;
  rio_float const score = 4.5;
  rio_Span_int const scores = {(rio_int[]){45, 63, 22, -8}, 4};
  tests_test1_report_scores(scores);
  rio_Span_int const mores = {(rio_int[]){1, -5}, 2};
  rio_Span_string const words = {(const char*[]){"hi", "there"}, 2};
  {
    rio_Span_string rio_span = words;
    for (rio_int rio_index = 0; rio_index < rio_span.len; rio_index += 1) {
      const char* word = rio_span.items[rio_index];
      printf("word: %s\n", word);
    }
  }
  {
    rio_Span_string rio_span = words;
    for (rio_int rio_index = 0; rio_index < rio_span.len; rio_index += 1) {
      printf("hi\n");
    }
  }
  tests_test1_greet(name, age);
  tests_test1_greet_person((tests_test1_Person){.name = name, .age = 80});
}

void tests_test1_greet(const char* const name, rio_int const age) {
  printf("%s, %s!\n", tests_test1_other_message, name);
  printf("You are %d years old.\n", age);
}

void tests_test1_greet_person(tests_test1_Person const person) {
  printf("%s is %d years old.\n", person.name, person.age);
}

void tests_test1_report_scores(rio_Span_int const scores) {
  {
    rio_Span_int rio_span = scores;
    for (rio_int rio_index = 0; rio_index < rio_span.len; rio_index += 1) {
      rio_int score = rio_span.items[rio_index];
      printf("score: %d\n", score);
    }
  }
}
