# Values and procs

## Variables

`:=` declares a variable, with its type from its value. `:` writes the type instead, and the
variable starts at zero unless you give it a value:

```rio
count := 0            # Int: 32 bits
speed := 2.5          # Float: 32 bits
name := "ann"         # String
ready := false        # Bool
hp: Int               # 0
scale: Float = 2      # a number literal fits a Float
count += 3
log(count, speed, name, ready, hp, scale)
```

```output
3 2.5 ann false 0 2.0
```

Each variable keeps its type. Rio doesn't mix `Int` and `Float` without being asked:

```rio
n := 3
x := 1.5
log(x * n.toFloat(), (x * 2.0).toInt())
```

```output
4.5 3
```

`::` declares a constant, a value known when compiling:

```rio
SIZE :: 8
HALF :: SIZE / 2
log(HALF)
```

```output
4
```

## Procs

A proc takes parameters, and maybe gives a result with `-> Type`:

```rio
area :: proc(w, h: Int) -> Int
  return w * h
end
shout :: proc(text: String)
  log(text, "!")
end
shout("hey")
log(area(3, 4))
```

```output
hey !
12
```

Everything is declared before it's used, so a proc can only call procs above it. That rules out
recursion, and Rio uses that: each proc gets one fixed piece of memory for its locals, with no stack
to overflow.

## Choices and loops

```rio
for n in 1..=15
  if n % 15 == 0
    log("fizzbuzz")
  else if n % 3 == 0
    log("fizz")
  else if n % 5 == 0
    log("buzz")
  else if n > 12
    log(n)
  end
end
```

```output
fizz
buzz
fizz
fizz
buzz
fizz
13
14
fizzbuzz
```

Conditions are `Bool`s, joined with `and`, `or` and `not`. `for` also loops while a condition holds,
or forever until `break`:

```rio
n := 1
for n < 100
  n *= 3
end
steps := 0
for
  steps += 1
  if steps == 4
    break
  end
end
log(n, steps)
```

```output
243 4
```

## When things go wrong

Mistakes the compiler can see stop the program before it runs, with a line and column. Mistakes it
can't see, such as an index out of range, stop the program where they happen, with a line:

```rio
xs: [3]Int
i := 5
log(xs[i])  # runtime error: index out of bounds
```

Integer math wraps around instead of failing, and dividing by zero is an error.

Next: [data](data.md).
