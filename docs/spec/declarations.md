# Declarations

`::` declares something that doesn't change: a constant, a type, a proc, or another name for one.
`:=` and `:` declare variables.

## Before use

Every name must be declared before it's used, in the order the source is read. So a proc can't call
itself, or a proc declared after it, and Rio has no recursion:

```rio
f :: proc()
  g()  # error: undefined name
end
g :: proc()
end
```

## Constants

`NAME :: value` names a value known when compiling: a number, `Bool` or `String`, or operators and
conversions on those. A constant number takes the type its place needs, as a literal does:

```rio
N :: 16
HALF :: N / 2
SCALE :: 1.5
NAME :: "rio"
x: Float = N             # 16.0: N is a number, not just an Int
log(HALF, x, NAME)
```

```output
8 16.0 rio
```

Constants can be declared in procs too. Top-level constants can be set from outside: see
[defines](host.md#defines).

## Variables

```rio
count := 0              # the type from the value: Int
speed: Float = 2        # the type written, and a value
grid: [4][4]Int         # the type written: starts as zero
log(count, speed, grid[3][3])
```

```output
0 2.0 0
```

A variable at the top level is a global, living as long as the program. One in a proc is a local; it
starts again each time its declaration runs. A declaration is visible from there to the end of its
block. Declaring a name again, in the same block or an inner one, makes a new variable that hides the
old one from then on:

```rio
x := 1
x := "one"               # a new x
f :: proc()
  x := 2.5               # a local x, in f only
  log(x)
end
f()
log(x)
```

```output
2.5
one
```

## Copies

Assigning, passing and returning copy values. To keep copies cheap and visible, a copy that isn't
written out is limited to 16 words: a bigger `x := big` is an error. Write the type to copy on
purpose, or take a view (`big[]`) or a reference (`&big`). Parameters and results are limited to 16
words; pass bigger values as `&T` or `[]T`, and fill in a `&T` parameter instead of returning one.
Plain assignment, `b = a`, copies any size.

```rio
Big :: struct
  a: [20]Int
end
g: Big
f :: proc()
  copy: Big = g          # fine: the type says it's a copy
  x := g  # error: this copies a big struct
end
```

## Procs

```rio
add :: proc(a, b: Int) -> Int
  return a + b
end
greet :: proc(name: String)
  log("hi", name)
end
greet("ann")
log(add(2, 3))
```

```output
hi ann
5
```

Procs are declared at the top level. Parameters are values the proc can change without affecting the
caller, except `&T` parameters (see [references](expressions.md#references)). A proc with a result must
end with `return` on every path:

```rio
sign :: proc(n: Int) -> Int
  if n < 0
    return -1
  end
end  # error: missing return
```

A proc can't return a view of its own locals, or store one where it outlives the call (a global, or
through a `&` parameter): the next call reuses that memory.

```rio
f :: proc() -> []Int
  xs: [3]Int
  return xs[]  # error: this returns a view of the proc's own memory
end
```

A view that comes in as a parameter may be one of the caller's own, so it's only borrowed: it can't
be stored where it outlives the call either. To keep what it views, copy the bytes:

```rio
name := ""
rename :: proc(s: String)
  name = s  # error: this view came in as a parameter
end
```

```rio
name: Blob[..32]
rename :: proc(s: String)
  name.clear()
  ok := name.pushAll(s)
end
f :: proc()
  typed: Blob[8]
  typed[0] = 'A'
  rename(typed[0..<1].asString())
end
f()
log(name[].asString())
```

```output
A
```

And a call's result, when it's a view, may view what went in, so it's a view of the caller's own
memory if any argument was. Each rule is about one proc's own code, so changing a proc's body never
changes what its callers may do.

## Methods

`Type.name :: proc` declares a method on a type. Inside, `self` is the value it was called on: for
structs, the caller's own value; for `Int`, `Float`, `Bool`, `String` and `Blob`, a copy.

```rio
Ship :: struct
  x, vx: Float
  hp: Int
end
Ship.move :: proc(dt: Float)
  self.x += self.vx * dt
end
Ship.hurt :: proc(n: Int) -> Bool
  self.hp -= n
  return self.hp <= 0
end
Int.double :: proc() -> Int
  return self * 2
end
s := Ship{vx = 2, hp = 3}
s.move(0.5)
log(s.x, s.hurt(5), 21.double())
```

```output
1.0 true 42
```

Each type has its own method names, apart from plain procs. A method can't have the same name as a
field of its type. Fields are always `self.x`; a plain `x` is never a field.

## Structs, enums and unions

```rio fragment
Vec :: struct
  x, y: Float            # several fields of one type
end
Dir :: enum
  north, east, south, west
end
Shape :: union
  Circle, Box
end
```

Field types, and the types in a union, must already be declared, so no type contains itself. See
[types](types.md) for what they mean. Types are declared at the top level.

## Other names for things

`::` with an existing type, proc or module makes another name for it:

```rio
add :: proc(a, b: Int) -> Int
  return a + b
end
plus :: add
Point :: [2]Float
p: Point
log(plus(1, 2), p.len())
```

```output
3 2
```

## Exports

A `*` after a top-level name exports it: other modules can use it, and the host can find it (see
[modules](modules.md) and [programs](host.md)):

```rio fragment
Vec* :: struct
  x, y: Float
end
score* := 0
update* :: proc()
end
```

## Host procs and diverse unions

Two names change what a declaration means, right before what they apply to:

- `host proc` and `host struct` declare something the host provides: see
  [host procs](host.md#declaring-host-procs).
- `diverse union` allows types of very different sizes in a union: see [unions](types.md#unions).

Elsewhere, `host` and `diverse` are ordinary names.
