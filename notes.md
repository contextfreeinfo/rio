- Top level source files works. Auto include of `src` dirs, too.
- Prohibit libstdc++ for faster compile.
- Cast like either `type expr` or `type(expr)`?
- All private until explicit private section?
- Parse `fun name` syntax for reporting specific error?
- Use `name fun` as shorthand for `name => fun`?
- Use `*type` as null-terminated and uglier (like `[*]type`) for raw?
- Or keep `string` and require uglies (like `*type..=0`) for null-terminated?
- Separate `fun` from `proc`? Does it matter with default const? Side effects???

`
./test -g > tests/test1.c-mem.txt 2>&1
./test > tests/test1.c.txt 2>&1
./test > tests/test1.c
`

Comments to be made into issues:
`
# # Untyped constants/macros?
# def pi = 3.14
# # def macro(x) = x + 1
# # Optional "shape" types on macro args????
# def macro(x) {
#   x + 1
# }
# Raw: '\_(..)_/'
# Char ints: 'A'u8 or 'A'u32 (or u8 'A' or u8('A')? Also u8 56 vs 56u8?).
# Interpolate just refs at first: $"Hello, $name!" -> ("Hello, ", name, "!")
# Also: $"Hi, $(name)san!" -> ("Hi, ", name, "san!")
# No line breaks nor quotes in $(expr) for now.
# "Goodbye!\n" -> printf
# nonlocal x
# var x = 3
# x := 4
# x += 1 # <- Lower priority
# Cache serialization ids for structs, unions, etc.
`
