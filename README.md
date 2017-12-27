## Overview

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


## Running

This command parses the test files to rebuild the cached AST output, for manual
comparison:

```
cargo run --bin testup
```

Once I've stabilized AST, it will make more sense to have unit tests that
actually check the output.

You can also pretend to compile and run, but it's not real yet.
So I won't bother to give that run here.


## Performance

Time log:
- 20171104 I tokenized 23M of file in 20+ seconds debug, 1-2 seconds release
  mode.
  On an i3.
- 20171106 Using keyword hashmap, about (now) 12M in 1 second release mode.
- 2017110? Switched to state machine. Slightly faster.
- 20171111 Parsing. Wasted assigns. About 1.3 seconds.
- 20171111 Wasted spaceds. About 1.5 seconds. Real effect?
- 20171116 Growing steadily with each layer. Maybe 2.2 seconds. Sad.
- 20171130 Maybe back down to about 1.1 seconds with new parser.
  I don't have the whitespace and comments in the ast again yet, though.
  Also, maybe even as low as about 1 second, with 0.1 seconds or more now on
  compiling bogus c output.
- 20171201 Prettier skip fill puts me at over 1.3 seconds rather than over 1.1.
  I don't understand why that would make a difference, though, since I iterate
  to append them either way.
- 20171216 Ported to c++. Taking more like 2.3 seconds now.
  Not sure what was faster about Rust, because I think the logic is pretty
  close.
  Maybe I'm not using my std::moves right?
- 20171216b Added a new std::move, and allocations are down, and time's down to
  1.7 seconds, using msvc in release mode.
- 20171218 Seems gcc gives about 1.5 seconds. I wonder what clang would do.
- 20171220 Still at about 1.5 seconds on gcc.
- 20171222 1.8 seconds for release. 2 seconds for minsizerel.
- 20171223 Still about 2 seconds for minsizerel.
- 20171223 After referents, 2.3 seconds and up from 1.08G to 1.19G
  (says /usr/bin/time).
  How much ram does g++ take in common cases for 13M of source?
- 20171227 After variants, back down to 1.08G. Time at about 2.24 seconds.

Maybe should actually profile sometime.

My concern is that temporary vecs for context of holding parsed kids is wasting
time by allocating and freeing ram.
But I don't know if this is a real effect or not.
Will it go up as I add other binary operator levels?
If so, I'll need to redo the parsing to avoid collecting on possible waste
space.

I'm looking into redoing things to return nodes and keep a common buffer for
intermediate nodes.
Also looking into Pratt parsers.
