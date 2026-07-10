Thoughts:

- No dynamic memory allocation
- Pointers only in parameters and locals
- ?Maybe, [_]Array, and []Span types
- Vague inspiration from Lua and Odin
- Clean module hierarchy on dots
- Direct native codegen in some cases, such as thumb-2 / rp2350 / Fruit Jam
- Fallback codegen/exec on wasm (and wamr???)
- Range type for slicing
- Redefine any constant as command line arg definition
- Nil value of enum or union for false
- Operator `not` always gives Bool, but `and` and `or` generic
- Maybe `or panic()` for awkward failing?
- Maybe awkwardly explicit monomorphizing generics???
- Maybe 1-based indexing for bad zero default ints
- Maybe hash helpers into spans
