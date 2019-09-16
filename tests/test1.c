#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

typedef double rio_float;
typedef ptrdiff_t rio_int;
typedef const char* rio_string;

// tests/test1-other.rio

bool tests_test1_other_has_child_ticket_price(rio_int const age);

rio_string const tests_test1_other_message = "Hello";

bool tests_test1_other_has_child_ticket_price(rio_int const age) {
  if (age < 18) {
    printf("You'd like to think that, wouldn't you?\n");
  }
  return age < 13;
}

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
  rio_string* items;
  rio_int len;
} rio_Span_string;

typedef struct tests_test1_Person {
  rio_string name;
  rio_int age;
  rio_Span_float scores;
} tests_test1_Person;

typedef struct rio_Span_tests_test1_Person {
  tests_test1_Person* items;
  rio_int len;
} rio_Span_tests_test1_Person;

void tests_test1_greet(rio_string const name, rio_int const age);
void tests_test1_report_scores(rio_Span_float const scores);
void tests_test1_show_person(tests_test1_Person const person);
void tests_test1_show_persons(rio_Span_tests_test1_Person const persons);

int main() {
  rio_string const name = "world";
  rio_int const age = 75;
  rio_float const score = 4;
  rio_Span_float const scores = (rio_Span_float){(rio_float[4]){45.0, 63.1, 22.2, -8.3}, 4};
  rio_Span_int const mores = (rio_Span_int){(rio_int[2]){1, -5}, 2};
  rio_Span_int const cores = (rio_Span_int){0};
  rio_Span_string const words = (rio_Span_string){(rio_string[2]){"hi", "there"}, 2};
  {
    rio_Span_string rio_span = words;
    for (rio_int rio_index = 0; rio_index < rio_span.len; rio_index += 1) {
      rio_string word = rio_span.items[rio_index];
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
  tests_test1_Person const person = (tests_test1_Person){.name = name, .age = 80, .scores = scores};
  tests_test1_show_persons((rio_Span_tests_test1_Person){(tests_test1_Person[2]){person, (tests_test1_Person){.age = 5, .name = "Me", .scores = (rio_Span_float){(rio_float[1]){2.5}, 1}}}, 2});
}

void tests_test1_greet(rio_string const name, rio_int const age) {
  printf("%s, %s!\n", tests_test1_other_message, name);
  printf("You are %td years old.\n", age);
}

void tests_test1_report_scores(rio_Span_float const scores) {
  {
    rio_Span_float rio_span = scores;
    for (rio_int rio_index = 0; rio_index < rio_span.len; rio_index += 1) {
      rio_float score = rio_span.items[rio_index];
      printf("score: %f\n", score);
      if (score < 1) {
        printf("(pretty low)\n");
      }
      else if (score > 4) {
        printf("(pretty high)\n");
      }
    }
  }
}

void tests_test1_show_person(tests_test1_Person const person) {
  // This is manually edited to be closer to what we need as the transform target.
  rio_string temp;
  if (person.age >= 65) {
    temp = "senior";
  }
  else if (person.age < 18) {
    temp = "minor";
  }
  else {
    printf("This is a large category.\n");
    temp = "adult";
  }
  // This is the statement that we extracted the temp from.
  printf("%s is %td years old (%s)\n", person.name, person.age, temp);
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
