#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

typedef double rio_float;
typedef ptrdiff_t rio_int;
typedef const char* rio_string;

// tests/test1-other.rio

#ifndef typedef_rio_Range_int
#define typedef_rio_Range_int
typedef struct rio_Range_int {
  rio_int from;
  rio_int to;
  rio_int by;
} rio_Range_int;
#endif  // typedef_rio_Range_int

rio_float tests_test1_other_bother(rio_float const x);
void tests_test1_other_bother_again(rio_float const x);
bool tests_test1_other_has_child_ticket_price(rio_int const age);
rio_int tests_test1_other_sum(rio_int const n);
rio_float tests_test1_other_ticket_price_calc(rio_int const age);

rio_string const tests_test1_other_message = "Hello";

rio_float tests_test1_other_bother(rio_float const x) {
  return x + 1;
}

void tests_test1_other_bother_again(rio_float const x) {
  bool const condition = x + 1 > 2;
  if (condition) {
    printf("Something!\n");
  }
}

bool tests_test1_other_has_child_ticket_price(rio_int const age) {
  if (age < 18) {
    printf("You'd like to think that, wouldn't you?\n");
  }
  return age < 13;
}

rio_int tests_test1_other_sum(rio_int const n) {
  rio_int result = 0;
  {
    rio_Range_int rio_list = (rio_Range_int){1, n + 1, 1};
    for (rio_int i = rio_list.from; i < rio_list.to; i += rio_list.by) {
      result = result + i;
    }
  }
  return result;
}

rio_float tests_test1_other_ticket_price_calc(rio_int const age) {
  if (tests_test1_other_has_child_ticket_price(age)) {
    return 4.50;
  }
  else {
    return 5.50;
  }
}

// tests/things.rio

typedef struct tests_things_Review {
  rio_string message;
  rio_int stars;
} tests_things_Review;

#ifndef typedef_rio_Span_int
#define typedef_rio_Span_int
typedef struct rio_Span_int {
  rio_int* items;
  rio_int length;
} rio_Span_int;
#endif  // typedef_rio_Span_int

#ifndef typedef_rio_Range_int
#define typedef_rio_Range_int
typedef struct rio_Range_int {
  rio_int from;
  rio_int to;
  rio_int by;
} rio_Range_int;
#endif  // typedef_rio_Range_int

rio_Span_int tests_things_range(rio_int const length);

rio_Span_int tests_things_range(rio_int const length) {
  {
    rio_int* items = (rio_int*)(malloc(length * sizeof(rio_int)));
    {
      rio_Range_int rio_list = (rio_Range_int){0, length, 1};
      for (rio_int i = rio_list.from; i < rio_list.to; i += rio_list.by) {
      }
    }
    return (rio_Span_int){.items = items, .length = length};
  }
}

// tests/test1.rio

#ifndef typedef_rio_Span_float
#define typedef_rio_Span_float
typedef struct rio_Span_float {
  rio_float* items;
  rio_int length;
} rio_Span_float;
#endif  // typedef_rio_Span_float

#ifndef typedef_rio_Span_string
#define typedef_rio_Span_string
typedef struct rio_Span_string {
  rio_string* items;
  rio_int length;
} rio_Span_string;
#endif  // typedef_rio_Span_string

typedef struct tests_test1_Person {
  rio_string name;
  rio_int age;
  rio_Span_float scores;
} tests_test1_Person;

#ifndef typedef_rio_Span_tests_test1_Person
#define typedef_rio_Span_tests_test1_Person
typedef struct rio_Span_tests_test1_Person {
  tests_test1_Person* items;
  rio_int length;
} rio_Span_tests_test1_Person;
#endif  // typedef_rio_Span_tests_test1_Person

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
    for (rio_int rio_index = 0; rio_index < rio_span.length; rio_index += 1) {
      rio_string word = rio_span.items[rio_index];
      printf("word: %s\n", word);
    }
  }
  {
    rio_Span_string rio_span = words;
    for (rio_int rio_index = 0; rio_index < rio_span.length; rio_index += 1) {
      printf("hi\n");
    }
  }
  tests_test1_greet(name, age);
  tests_test1_Person const person = (tests_test1_Person){.name = name, .age = 80, .scores = scores};
  rio_string const hey = person.name;
  tests_test1_show_persons((rio_Span_tests_test1_Person){(tests_test1_Person[3]){person, (tests_test1_Person){.age = 5, .name = "Me", .scores = (rio_Span_float){(rio_float[1]){2.5}, 1}}, (tests_test1_Person){.age = 25, .name = "Other", .scores = (rio_Span_float){(rio_float[2]){4.2, 4.5}, 2}}}, 3});
  printf("sum: %d\n", tests_test1_other_sum(5));
}

void tests_test1_greet(rio_string const name, rio_int const age) {
  printf("%s, %s!\n", tests_test1_other_message, name);
  printf("You are %td years old.\n", age);
}

void tests_test1_report_scores(rio_Span_float const scores) {
  {
    rio_Span_float rio_span = scores;
    for (rio_int rio_index = 0; rio_index < rio_span.length; rio_index += 1) {
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
  printf("%s is %td years old (ticket price: %.2f, common category: %s)\n", person.name, person.age, (tests_test1_other_has_child_ticket_price(person.age)) ? (4.50) : (5.50), (person.age >= 65) ? ("senior") : (person.age < 18) ? ("minor") : (printf("This is a large category.\n"), "adult"));
  tests_test1_report_scores(person.scores);
}

void tests_test1_show_persons(rio_Span_tests_test1_Person const persons) {
  {
    rio_Span_tests_test1_Person rio_span = persons;
    for (rio_int rio_index = 0; rio_index < rio_span.length; rio_index += 1) {
      tests_test1_Person person = rio_span.items[rio_index];
      tests_test1_show_person(person);
    }
  }
}
