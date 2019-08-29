#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

typedef double rio_float;
typedef ptrdiff_t rio_int;

// tests/test1-other.rio

const char* const tests_test1_other_message = "Hello";

// tests/test1.rio

typedef struct rio_Span_float {
  rio_float* items;
  rio_int len;
} rio_Span_float;

typedef struct rio_Span_int {
  rio_int* items;
  rio_int len;
} rio_Span_int;

typedef struct rio_Span_string {
  const char** items;
  rio_int len;
} rio_Span_string;

typedef struct tests_test1_Person {
  const char* name;
  rio_int age;
  rio_Span_float scores;
} tests_test1_Person;

typedef struct rio_Span_tests_test1_Person {
  tests_test1_Person* items;
  rio_int len;
} rio_Span_tests_test1_Person;

void tests_test1_greet(const char* const name, rio_int const age);
void tests_test1_report_scores(rio_Span_float const scores);
void tests_test1_show_person(tests_test1_Person const person);
void tests_test1_show_persons(rio_Span_tests_test1_Person const persons);

int main() {
  const char* const name = "world";
  rio_int const age = 75;
  rio_float const score = 4.5;
  rio_Span_float const scores = (rio_Span_float){(rio_float[]){45.0, 63.1, 22.2, -8.3}, 4};
  rio_Span_int const mores = (rio_Span_int){(rio_int[]){1, -5}, 2};
  rio_Span_string const words = (rio_Span_string){(const char*[]){"hi", "there"}, 2};
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
  tests_test1_show_persons((rio_Span_tests_test1_Person){(tests_test1_Person[]){(tests_test1_Person){.name = name, .age = 80, .scores = scores}}, 1});
}

void tests_test1_greet(const char* const name, rio_int const age) {
  printf("%s, %s!\n", tests_test1_other_message, name);
  printf("You are %d years old.\n", age);
}

void tests_test1_report_scores(rio_Span_float const scores) {
  {
    rio_Span_float rio_span = scores;
    for (rio_int rio_index = 0; rio_index < rio_span.len; rio_index += 1) {
      rio_float score = rio_span.items[rio_index];
      printf("score: %f\n", score);
    }
  }
}

void tests_test1_show_person(tests_test1_Person const person) {
  printf("%s is %d years old.\n", person.name, person.age);
  tests_test1_report_scores(person.scores);
}

void tests_test1_show_persons(rio_Span_tests_test1_Person const persons) {
  {
    rio_Span_tests_test1_Person rio_span = persons;
    for (rio_int rio_index = 0; rio_index < rio_span.len; rio_index += 1) {
      tests_test1_Person person = rio_span.items[rio_index];
      tests_test1_show_person(person);
    }
  }
}
