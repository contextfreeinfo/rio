# Methods, enums, unions and maybes

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

## Maybes

Some things may not be there: a search that finds nothing, an enemy with no target yet. `?T` is either
nil or a `T`, and it starts as nil:

```rio
Item :: struct
  name: String
  price: Int
  sale: ?Int     # a sale price, if there is one
end
items: [3]Item
items[0] = Item{name = "lamp", price = 30}
items[1] = Item{name = "rug", price = 80, sale = 60}
items[2] = Item{name = "vase", price = 45}
cost :: proc(it: &Item) -> Int
  sale := it.sale
  if sale is Int
    return sale    # here, sale is the Int
  end
  return it.price
end
cheapest :: proc() -> ?Int # which item costs least under 50, if any does
  best: ?Int
  bestCost := 50
  for i in 0..<3
    if cost(&items[i]) < bestCost
      best = i
      bestCost = cost(&items[i])
    end
  end
  return best
end
log(cost(&items[0]), cost(&items[1]))
pick := cheapest()
switch pick
case nil
  log("nothing under 50")
case Int
  log(items[pick].name)
end
log(items[1].sale, items[2].sale)
```

```output
30 60
lamp
60 nil
```

Rio won't let a `?Int` be used as an `Int` until it's checked, so a missing value can't slip through
as a 0 by mistake.

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
