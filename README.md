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
  show_persons([
    {name, age},
    {name = "Bernie", age = 39},
  ])
end

Person struct
  name: string
  age: int
end

show_persons proc(persons: [Person])
  for persons do(person)
    printf("%s is %td years old\n", person.name, person.age)
  end
end
```

Compiles to this C file (with manual whitespace added to the struct array in `main` for readability):

```c
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

typedef double rio_float;
typedef ptrdiff_t rio_int;

// sample2.rio

typedef struct sample2_Person {
  const char* name;
  rio_int age;
} sample2_Person;

typedef struct rio_Span_sample2_Person {
  sample2_Person* items;
  rio_int len;
} rio_Span_sample2_Person;

void sample2_show_persons(rio_Span_sample2_Person const persons);

int main() {
  const char* const name = "Alice";
  rio_int const age = 40;
  sample2_show_persons(
    (rio_Span_sample2_Person){(sample2_Person[2]){
      (sample2_Person){.name = name, .age = age},
      (sample2_Person){.name = "Bernie", .age = 39}
    }, 2}
  );
}

void sample2_show_persons(rio_Span_sample2_Person const persons) {
  {
    rio_Span_sample2_Person rio_span = persons;
    for (rio_int rio_index = 0; rio_index < rio_span.len; rio_index += 1) {
      sample2_Person person = rio_span.items[rio_index];
      printf("%s is %td years old\n", person.name, person.age);
    }
  }
}
```

## Status

- A few things work (like the example above!).
- Module includes (for combining multiple files) and uses (for separate modules).
- No error checking.
- Coding ground up with language server in mind.
- Uses arenas, interning, and so on with the hopes of being efficient.
- Coded in C++ for now but *without* the C++ std lib for faster compiling and fewer dependencies later down the road.
- Testing output in tcc, since high speed compilation is great. (And testing in gcc, too.)
