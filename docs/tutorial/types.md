# Methods, enums and unions

## Methods

A method is a proc on a type. Inside, `self` is what it was called on, and for a struct, changes to
`self` change the real thing:

```rio
Ship :: struct
  x, vx: Float
end
Ship.move :: proc(dt: Float)
  self.x += self.vx * dt
end
s := Ship{vx = 4}
s.move(0.5)
log(s.x)
```

```output
2.0
```

## Enums

An enum is a set of named values:

```rio
Dir :: enum
  north, east, south, west
end
turn :: proc(d: Dir) -> Dir
  switch d
  case .north
    return .east
  case .east
    return .south
  case .south
    return .west
  case .west
    return .north
  end
end
d := Dir.north
d = turn(turn(d))
log(d)
```

```output
south
```

`.south` is enough where Rio knows the type. A `switch` on an enum has to handle every value (or have
an `else`), so adding a value later points you at every switch to update.

## Unions

A union holds one of several types, and knows which:

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
    return 3.14 * s.r * s.r   # here, s is the Circle
  case Box
    return s.w * s.h
  end
end
shapes: [2]Shape
shapes[0] = Circle{r = 1}
shapes[1] = Box{w = 2, h = 3}
for s in shapes
  log(area(s))
end
```

```output
3.14
6.0
```

`if s is Box` works too, and inside that `if`, `s` is the `Box`.

## Proc values

A proc can be passed around and called later:

```rio
twice :: proc(n: Int) -> Int
  return n * 2
end
each :: proc(xs: []Int, f: proc(Int) -> Int)
  for x in xs
    x = f(x)
  end
end
nums: [3]Int
nums[2] = 5
each(nums, twice)
log(nums[2])
```

```output
10
```

Next: [modules](modules.md).
