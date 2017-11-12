Just some quick plan notes for now:

- No GC
- Compiles fast
- Static typing with templates/generics
- Explicit copy, move, and borrow semantics
- Probably no full memory/type safety
- Convenient exceptions, perhaps still as return types
- More data-oriented than object-oriented
- Still has classes
- Operator overloading
- Tensors (ND arrays)

Time log:
- 20171104 I tokenized 23M of file in 20+ seconds debug, 1-2 seconds release
  mode.
  On an i3.
- 20171106 Using keyword hashmap, about (now) 12M in 1 second release mode.
- 2017110? Switched to state machine. Slightly faster.
- 20171111 Parsing. Wasted assigns. About 1.3 seconds.
- 20171111 Wasted spaceds. About 1.5 seconds. Real effect?

My concern is that temporary vecs for context of holding parsed kids is wasting
time by allocating and freeing ram.
But I don't know if this is a real effect or not.
Will it go up as I add other binary operator levels?
If so, I'll need to redo the parsing to avoid collecting on possible waste
space.
