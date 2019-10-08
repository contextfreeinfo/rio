#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

typedef double rio_float;
typedef ptrdiff_t rio_int;
typedef const char* rio_string;

// sample2.rio

typedef struct sample2_Person {
  rio_string name;
  rio_int age;
} sample2_Person;

typedef struct rio_Span_sample2_Person {
  sample2_Person* items;
  rio_int len;
} rio_Span_sample2_Person;

void sample2_show_prices(rio_Span_sample2_Person const persons);
rio_float sample2_ticket_price(rio_int const age);

int main() {
  rio_string const name = "Alice";
  rio_int const age = 40;
  sample2_show_prices((rio_Span_sample2_Person){(sample2_Person[4]){(sample2_Person){.age = age, .name = name}, (sample2_Person){.name = "Bernie", .age = 14}, (sample2_Person){.name = "Clara", .age = 9}, (sample2_Person){.name = "Darlene", .age = 68}}, 4});
}

void sample2_show_prices(rio_Span_sample2_Person const persons) {
  {
    rio_Span_sample2_Person rio_span = persons;
    for (rio_int rio_index = 0; rio_index < rio_span.len; rio_index += 1) {
      sample2_Person person = rio_span.items[rio_index];
      printf("%s pays $%.2f\n", person.name, sample2_ticket_price(person.age));
    }
  }
}

rio_float sample2_ticket_price(rio_int const age) {
  if (age <= 11) {
    return 6.75;
  }
  else if (age >= 62) {
    return 7.00;
  }
  else {
    if (age < 18) {
      printf("You think age %d gets child prices here?\n", age);
    }
    return 11.00;
  }
}
