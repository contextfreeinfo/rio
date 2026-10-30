# Rejected for Rio

Ideas that Rio has turned down, with the reasons, so they needn't be argued again. Some are features
other languages have and people may expect. What's still open is in [ROADMAP.md](ROADMAP.md).

- **`**`:** `pow` covers Floats.
- **Coroutines:** each needs a saved frame, which costs memory and is hard to specify.
- **Traits and constraints on generics.**
- **Odin's `$T` for generics:** a parameter's first `$T` would quietly declare T. `proc[T]` lists
  them up front.
- **Annotations, `@`:** programs don't define their own, since one the compiler checks would need a
  language for what to check, and one it doesn't check is a comment a typo can break. Tools that want
  markers can read comments.
- **JIT requests in the language:** native code is an implementation's choice. A host short on code
  space picks which loops get it, by how often they run and how big they are.
- **Named-only parameters** (`text(s: String; color: Int)`, called `text("hi", color = 8)`): struct
  arguments do the job with no new syntax, and keep options values, to keep, reuse and forward. Proc
  types would have needed them too.
- **Struct arguments without braces** (`text("hi", 3, 4, scale = 2)` for a last struct parameter):
  the braces show where the options are.
- **Host procs with no marker:** a proc with no body would read exactly like a proc type alias, so
  `host` says the body is elsewhere.
- **A builtin Map or Dict:** without generic types it would be a special case, and Rio never
  allocates, so a map's capacity and layout are the program's business. Programs write tables for
  their own keys: `[E]T` by enum, a sorted array, or a hash table over arrays.
- **Truthiness:** conditions stay Bool. (Maybes may be conditions too, if Bool becomes a maybe of
  nothing: see the roadmap.)
- **Recursion, garbage collection, and allocation while running:** each proc has one fixed frame and
  a program's memory is known before it runs. That's what makes Rio predictable on small boards.
- **Enum values of their own (`= n`):** enums are always 0, 1, 2... in order, so they index arrays and
  need no value tables. Other numbers are constants or an `[E]Int` table.
- **`a[i:j]` slices:** ranges took their place (`a[i..<j]`), so there's one way to write a span.
- **`a[..]` for a whole slice:** it looks too much like a builder view type (`Blob[..]`). `a[]`
  matches the type `[]T`. Nor `a[..=]`: `a[]` hasn't misled in practice.
- **More spellings of an open-ended slice** (`a[i..<]`, `a[i..=]`): both would mean `a[i..]`, since
  nothing past the end is taken, and one spelling leaves nothing for a formatter to standardize.
- **C#'s `^1` for indexes from the end:** `xs[end]` is the last index instead. `^` is xor too, `^0`
  (past the end) surprises, and `xs[end - i]` in a loop reads better than `xs[^(i + 1)]`. A wrong
  guess at what `end` means is a bounds error, not a misread.
- **Formatters in place of `format`:** `b.format(settings, ...)` stays; formatters as values may come
  beside it.
- **A table of host procs with no declarations:** hosts give their modules' sources (`rio_addSource`)
  and `rio -H` writes the C to match, so declaring each host proc costs nothing and is checked.
- **`else if`:** `case` continues an if, as in a switch with no value.
- **`c ? a : b`:** a branch is an if. If anything, if and switch might give values when setting a
  variable (see the roadmap).
- **`&&`, `||` and `!`:** the words `and`, `or` and `not` read better, and the symbols are errors that
  name the word to use.
