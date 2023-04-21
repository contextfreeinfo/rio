<img style="float: right; height: 96px" align="right" height="96" src="rio-logo.svg">

# Rio Programming Language


## Goal

A script-feeling, safe, naturally compatible replacement for C, with no runtime nor std lib of its own.

See also my [YouTube channel called Context Free](https://www.youtube.com/channel/UCS4FAVeYW_IaZqAbqhlvxlA) where I discuss programming languages, including things I think about while working on Rio.


## To do

- A mode that spits out all inferred types, effects, locked imports, and so on.


## Description

For an example, here's a Rio script:

```ruby
use "c"

do
  # Assignment here declares new *const* values
  name = "Alice"
  age = 40
  show_prices([
    # Shorthand field names work like JS or Rust
    {age, name},
    {name: "Bernie", age: 14},
    {name: "Clara", age: 9},
    {name: "Darlene", age: 68},
  ])
end do

Person = struct
  name as string
  age as int
end

show_prices = for(persons as [Person])
  persons each for person
    printf("%s pays $%.2f\n", person.name, ticket_price(person.age))
  end
end

ticket_price = for(age: int): float
  if
  case age <= 11
    6.75
  case age >= 62
    7.00
  else
    if age < 18
      printf("You think age %d gets child prices here?\n", age)
    end
    11.00
  end if
end ticket_price
```

That compiles to this C file (with manual whitespace added to the struct array in `main` for readability):

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


## Notes

```
time (time nim c --nimcache:nimcache -d:release -d:useMalloc --mm:orc nim/main.nim && time ./nim/main tests/fib-simpler.rio tests/trees/ && time wasm2wat tests/trees/fib-simpler.wasm -o tests/trees/fib-simpler.wat)
# Older
time (time nim c --nimcache:nimcache -d:release -d:useMalloc --mm:orc nim/main.nim && time ./nim/main tests/fib-simpler.rio tests/trees/)
nim c --nimcache:nimcache -d:release --mm:orc nim/main.nim
nim r --nimcache:nimcache -d:release --mm:orc nim/main.nim
nim r --nimcache:nimcache -d:release --mm:orc nim/main.nim tests/fib-simpler.rio > tests/trees/
```

```
2022-10-02T15:29:33Z tjpalmer@pop-os:~/projects/rio
$ nim c --nimcache:nimcache -d:release -d:useMalloc --mm:orc nim/main.nim && valgrind nim/main tests/fib-simpler.rio > tests/trees/fib-simpler.lexed.txt
Hint: used config file '/home/tjpalmer/apps/nim-1.6.6/config/nim.cfg' [Conf]
Hint: used config file '/home/tjpalmer/apps/nim-1.6.6/config/config.nims' [Conf]
............................................................................................
CC: main.nim
Hint:  [Link]
Hint: gc: orc; opt: speed; options: -d:release
51728 lines; 0.704s; 75.207MiB peakmem; proj: /home/tjpalmer/projects/rio/nim/main.nim; out: /home/tjpalmer/projects/rio/nim/main [SuccessX]
==76541== Memcheck, a memory error detector
==76541== Copyright (C) 2002-2017, and GNU GPL'd, by Julian Seward et al.
==76541== Using Valgrind-3.18.1 and LibVEX; rerun with -h for copyright info
==76541== Command: nim/main tests/fib-simpler.rio
==76541== 
==76541== 
==76541== HEAP SUMMARY:
==76541==     in use at exit: 16,384 bytes in 1 blocks
==76541==   total heap usage: 351 allocs, 350 frees, 39,421 bytes allocated
==76541== 
==76541== LEAK SUMMARY:
==76541==    definitely lost: 0 bytes in 0 blocks
==76541==    indirectly lost: 0 bytes in 0 blocks
==76541==      possibly lost: 0 bytes in 0 blocks
==76541==    still reachable: 16,384 bytes in 1 blocks
==76541==         suppressed: 0 bytes in 0 blocks
==76541== Rerun with --leak-check=full to see details of leaked memory
==76541== 
==76541== For lists of detected and suppressed errors, rerun with: -s
==76541== ERROR SUMMARY: 0 errors from 0 contexts (suppressed: 0 from 0)
2022-10-02T15:29:55Z tjpalmer@pop-os:~/projects/rio
```
