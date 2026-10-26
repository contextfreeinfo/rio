# Statements

A block is the statements up to its `end` (or the next `else` or `case`). Names declared in a block
end with it.

## Expressions as statements

Any expression can be a statement, usually a call. Its value, if any, is dropped (the REPL prints
it).

## Assignment

`place = value` stores a value, copying it. `place op= value` is `place = place op value`, with
the place worked out once, for `+ - * / % & | ^ << >>`. There's no `++`.

```rio
xs: [3]Int
i := 0
xs[i] = 5
xs[i] += 2
xs[i] <<= 1
log(xs[0])
```

```output
14
```

Constants, `String` bytes and call results can't be assigned:

```rio
N :: 5
N = 6  # error: cannot assign to this
```

## Destructuring

`{a, b} := value` declares new variables from a struct's fields, by name, in any order; `as` renames.
The value is evaluated once, and the variables are copies. Patterns are one level deep:

```rio
QR :: struct
  q, r: Int
end
divmod :: proc(a, b: Int) -> QR
  return {q = a / b, r = a % b}
end
{q, r} := divmod(17, 5)
{r as rest} := divmod(9, 4)
log(q, r, rest)
```

```output
3 2 1
```

## if

```rio
n := 7
if n > 10
  log("big")
case n > 5
  log("medium")
else
  log("small")
end
```

```output
medium
```

The condition is a `Bool`. `if a` is short for `switch` and then `case a`: a switch with nothing after
it tests each case's condition in turn and runs the first that's true, or else its `else`, so a chain of
conditions reads the same either way:

```rio
n := 7
switch
case n > 10
  log("big")
case n > 5
  log("medium")
end
```

```output
medium
```

```rio
n := 7
if n > 10
  log("big")
else if n > 5  # error: write case
  log("medium")
end
```

```rio
n := 1
if n  # error: condition must be Bool
end
```

## for

`for` has four forms:

```rio
for i in 0..<3            # a range: 0, 1, 2
  log("range", i)
end
for i in 1..=2            # including the end: 1, 2
  log("through", i)
end
n := 0
for n < 2                 # while the condition holds
  n += 1
end
for                       # forever, until break or return
  break
end
log(n)
```

```output
range 0
range 1
range 2
through 1
through 2
2
```

- **Ranges:** `for i in a..<b` sets `i` to `a`, and runs while `i < b`, adding 1 after each time.
  `b` is evaluated once, before the loop. `a..=b` is `a..<b + 1`. `i` is a new `Int` variable; changing
  it changes the next value.
- **Sequences:** `for x in xs` visits each element of an array, slice, builder, `String` or `Blob`, and
  `for x, i in xs` adds the index. `x` is the element itself, not a copy, so changing it changes `xs`
  (unless `xs` is a `String`, which is read-only). Over a builder, the loop visits the elements it had when
  it started.
- **Enums:** `for d in E` visits the enum's values in the order listed (`for d, i in E` adds the
  index). Over an `[E]T` array, the index is the enum value.

```rio
xs: [3]Int
for x, i in xs
  x = i * 10
end
total := 0
for x in xs
  total += x
end
Dir :: enum
  north, east
end
for d in Dir
  log(d)
end
log(total)
```

```output
north
east
30
```

## break and continue

`break` leaves the innermost loop, and `continue` goes on to its next round. A label on a loop
(`name: for ...`) lets them reach an outer one:

```rio
found := -1
rows: for y in 0..<4
  for x in 0..<4
    if x * y == 6
      found = y * 10 + x
      break rows
    end
  end
end
log(found)
```

```output
23
```

A label can be used again once its loop has ended.

## switch

`switch` picks the first `case` that lists the value, with no fallthrough. It works on `Int`s,
enums, unions and `String`s. Case values are constants (or, for a union, its types). `else` comes last,
and takes what no case did:

```rio
kind :: proc(n: Int) -> String
  switch n % 3
  case 0
    return "zero"
  case 1, 2
    return "other"
  end
  return "?"
end
log(kind(3), kind(4))
```

```output
zero other
```

On a `String`, cases are string constants, compared as `==` compares them:

```rio
command :: proc(s: String) -> Int
  switch s
  case "ls", "list"
    return 1
  case "home"
    return 2
  else
    return 0
  end
end
log(command("list"), command("home"), command("hom"))
```

```output
1 2 0
```

Without an `else`, a switch on an enum or union must have a case for every value (or type, `nil`
included). Each value can appear once:

```rio
Dir :: enum
  north, east, south
end
d: Dir
switch d
case .north, .east
  log("up")
end  # error: this switch has no case for .south
```

On a union variable, a case naming one type narrows the variable to that type inside the case, as
`is` does in an `if`. `switch name := value` declares a variable for the switch, so any value can be
narrowed:

```rio
Circle :: struct
  r: Float
end
Box :: struct
  w, h: Float
end
Shape :: union
  Circle, Box
end
area :: proc(s: Shape) -> Float
  switch s
  case Circle
    return 3.0 * s.r * s.r
  case Box
    return s.w * s.h
  end
end
log(area(Box{w = 2, h = 3}), area(Circle{r = 1}))
```

```output
6.0 3.0
```

`break` and `continue` in a switch are about the loop around it.

## One-line blocks

`do` puts a block's one statement on the line that starts it, and the block ends with the line. It
works after `if`, `for` and a proc's header, and after `case` and `else`:

```rio
double :: proc(n: Int) -> Int do return n * 2
total := 0
for i in 1..=4 do total += i
if total > 5 do log("big", double(total))
light := 1
switch light
case 0 do log("stop")
case 1 do log("go")
else do log("wait")
end
```

```output
big 20
go
```

A one-line `case` or `else` is just its line, so `case`, `else` or `end` comes next. A one-line `if`
has no `case` or `else`: one with branches is written out, with its end. The statement after `do`
can't open a block, one-line or not, so they don't nest:

```rio
xs := [3]Int{1, 2, 3}
for x in xs do if x > 1 do log(x)  # error: one-line blocks don't nest
```

```rio
n := 1
if n > 0 do log(n); log(2)  # error: a one-line block holds one statement
```

## do blocks

`do` on a line of its own starts a block that only holds names: those declared inside end with it.

```rio
do
  total := 0
  for x in 1..=3 do total += x
  log(total)
end
total := "free again"
log(total)
```

```output
6
free again
```

## return

`return` leaves the proc, with a value when the proc has a result. See
[procs](declarations.md#procs) for which paths must return.
