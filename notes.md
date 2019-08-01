- Top level source files works. Auto include of `src` dirs, too.
- Prohibit libstdc++ in compiler for faster compile.
- Compile to subset safe for c99 (so tcc works) and c++ (for c++ interop).
- Cast like `expr: type`.
- All private until explicit private section?
- Parse `fun name` syntax for reporting specific error?
- Use `name fun` as shorthand for `name => fun`?
- Use `&type..0` as null-terminated and `&type..*` for raw.
- Allow `&type..+` as safe deref with hint of more or just stick to `&type`?
- Default string literals to `[u8..0]` (or `[u8, 0]`?) as null-terminated slice.
- Also allow string literals as `&u8..0`, `[u8]`, `u8` (if 1 u8), or other ints.
- Still keep `string` keyword as equivalent to `[u8..0]` (or `[u8, 0]`?)?
- Range types as `u8..` with pointer as `&(u8..)` and slice `[u8..]`.
- Separate `fun` from `proc`? Does it matter with default const? Side effects???
- Change unqualified `use package` to external packages, not siblings
- Treat all name conflicts as errors, or let local package override others?
- Definitely conflicts between externals count as errors
- Use/run a dir gets all in dir, use/run a file gets just that file
- For data files (always [u8])?: data = include "..."

```bash
./test -g > tests/test1.c-mem.txt 2>&1
./test > tests/test1.c.txt 2>&1
./test > tests/test1.c
```

Comments to be made into issues:
```ruby
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
```

Imports:
```ruby
# Just use all non-conflicting from c.
use 'c'

# Use c as a wrapper around the imported items.
c => use 'c'

# Use c as a wrapper and import non-conflicting.
c => use 'c' as *

# Use c as wrapper, import non-conflicting, and use explicit aliases.
c => use 'c' as
  *
  c_strlen => strlen
end c

# Maybe `as` also for destructuring locals?
get_some_value() as
  # No `*` for locals.
  # Same name.
  something
  # Local var `other` from `that` in the return struct.
  other = that
end
```

Strings:
```ruby
escaping = "This is \"escaping\": \(value)\n"
raw = 'This is raw, with \ and all'
concat = 'I have raw \ content with a single quote: ' "'" '. <- See?'
multi =
  "I can \"mix and match\" across lines here: \(value)
  'Here is a raw \ line
  'I can terminate the lines to avoid a newline '
  'just like that.
  """I can also use "triple-quotes" to ensure always full lines: \(value)
  '''And avoid \ escapes if I want, too.
final_newline =
  '''Probably best to use triples for every line of a group, though.
  """With "triple double-quote" \(name) can still avoid \
  """the newline if you want.
  '''If you want a newline at the end, you need an empty string at the end.
  ''
simple_full = '''Just a single-line string sans newline with no terminator.
tuple = "Of course, every string literal with embedded \(value) is a tuple."
# Finally, if you really want a lot of data, use a file embedding macro.
# How should that look?
```

Map literals?
```ruby
# If array of tuples expected, interpret differently?
{} with
  name
  message = 'that'
  (other) = 'more'
end

# Above equivalent to:
[
  ('name', name),
  ('message', 'that'),
  (other, 'more'),
]
```
