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
- 20171228 After individual allocations/unique_ptr, down to 0.96G.
  Time at about 2.3 seconds.
- 20171228b After string data, about 2.45 seconds and 0.97G.
- 20171229 After function names (and generation that breaks for big which has
  bogus content), still about 2.45s and 0.97G.
- 20170115 Could be different state that slower right now, but about 3s and
  still 0.97G. I don't think anything meaningful for speed has changed.
  The high memory use still bothers me.

Maybe should actually profile sometime.
I'm also concerned about the high ram usage for the large file parse.
I'm not really sure how that breaks down.
