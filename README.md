## Building

Run:

```sh
./run.sh examples/hi/hi.rio
```

Or for thumb2, if on a system that can run thumb2:

```sh
./run.sh --thumb examples/hi/hi.rio
```

## Thoughts

- No dynamic memory allocation
- Maybe no recursion???
- Refs only in parameters and locals
- Maybe a shadow stack for platforms with limited stack space
- &Reference, ?Maybe, [_]Array, and []Span types
- &mut Ref, []mut Span, else deeply imu?
- Refs can't be nil
- Named args good because can't store refs/spans in structs?
- No numeric address access? Still can compare for equality only?
- Numeric zero, empty slices, and empty arrays are falsy
- Vague inspiration from Lua and Odin
- Override constants from command line args including for global array sizes
- Clean module hierarchy on dots
- Direct native codegen in some cases, such as thumb-2 / rp2350 / Fruit Jam
- Fallback codegen/exec on wasm (and wamr or wasmi???)
- Range type for slicing
- Redefine any constant as command line arg definition
- Nil value of enum or union for false
- Operator `not` always gives Bool, but `and` and `or` generic
- Maybe `or panic()` for awkward failing?
- Maybe awkwardly explicit monomorphizing generics???
- Maybe 1-based indexing for bad zero default ints
- Maybe hash helpers into spans
