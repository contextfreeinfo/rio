# Rio

## Goal

A script-feeling, safe, naturally compatible replacement for C, with no runtime nor std lib of its own.

## Description

Just a slightly fancy hello world for now here:

```ruby
use "c"

do
  # Simple assignment-looking statements *declare* constants.
  name = "world"
  age = 75
  greet(name, age)
end

message = "Hello"

greet proc(name: string, age: i32)
  printf("%s, %s!\n", message, name)
  printf("You are %d years old.\n", age)
end
```

Compiles to this C file:

```c
#include <stdint.h>
#include <stdio.h>

void tests_test1_greet();

const char* const tests_test1_message = "Hello";

int main() {
  const char* const name = "world";
  int32_t const age = 75;
  tests_test1_greet(name, age);
}

void tests_test1_greet(const char* const name, int32_t const age) {
  printf("%s, %s!\n", tests_test1_message, name);
  printf("You are %d years old.\n", age);
}
```

## Status

- A few things work (like the hello world above!).
- No error checking.
- Coding ground up with language server in mind.
- Uses arenas, interning, and so on with the hopes of being efficient.
- Coded in C++ for now but *without* the C++ std lib for faster compiling and fewer dependencies later down the road.
