# Rio

## Goal

A script-feeling, safe, naturally compatible replacement for C, with no runtime nor std lib of its own.

## Description

For a sample, this Rio script:

```ruby
use "c"

do
  # Assignment here declares new *const* values
  name = "Alice"
  age = 40
  show_prices([
    # Shorthand field names work like JS or Rust
    {age, name},
    {name = "Bernie", age = 14},
    {name = "Clara", age = 9},
    {name = "Darlene", age = 68},
  ])
end

Person struct
  name: string
  age: int
end

show_prices proc(persons: [Person])
  for person in persons
    printf("%s pays $%.2f\n", person.name, ticket_price(person.age))
  end
end

ticket_price proc(age: int): float
  if
    case age <= 11
      6.75
    end
    case age >= 62
      7.00
    end
    else
      if age < 18
        printf("You think age %d gets child prices here?\n", age)
      end
      11.00
    end
  end
end
```

Compiles to this C file (with manual whitespace added to the struct array in `main` for readability):

```c
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
  sample2_show_prices((rio_Span_sample2_Person){(sample2_Person[4]){
    (sample2_Person){.age = age, .name = name},
    (sample2_Person){.name = "Bernie", .age = 14},
    (sample2_Person){.name = "Clara", .age = 9},
    (sample2_Person){.name = "Darlene", .age = 68}
  }, 4});
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
      printf("You think age %d gets child prices here?\n", age)
    }
    return 11.00;
  }
}
```

## Status

- A few things work (like the example above!).
- Many other things don't yet.
- No error checking at all yet.
- Supports relative path module includes (for combining multiple files) and uses (for separate modules).
- Coding ground up with language server in mind.
- Uses arenas, interning, and so on with the hopes of being efficient.
- Coded in C++ for now but *without* the C++ std lib for faster compiling and fewer dependencies later down the road.
- Current strategy: Support enough features I can port the compiler to Rio, then port, then finish and polish.
- Testing output in tcc, since high speed compilation is great. (And testing in gcc, too.)
