# Rio roadmap

The goal is a Rio 1.0 that's robust, nearly complete, and specified well enough that anyone could
implement it from the [spec](docs/spec/language.md). After 1.0, only bug fixes and implementation
improvements, with no 1.1, so each item below is decided as now or never. Releases may stay 0.x, or a
1.0 beta, until Rio has more users.

This is the list of everything still being considered. Each item says where it stands and which way
it leans. Rio means to stay small, so some of these will end up as no. Ideas already turned down,
with the reasons, are in [REJECTED.md](REJECTED.md).

## Decide first: changes that would break existing source

- **`_` not a name:** `_ := 3` and `_ :: 3` are legal today, and `[_]` already means a literal sized by
  its values. Making `_` illegal as a name keeps it free for that and for any later "ignore this"
  use (`for _ in 0..<n`, say).

Each other change here was decided against, keeping Rio as it is: `a[]` for a whole slice,
`a[i..]` the one spelling of an open-ended slice, `end` for the last index (not C#'s `^1`), and
`format` staying, with no formatters. [REJECTED.md](REJECTED.md) has the reasons. A
change that would break existing source goes here first, since programs already written, such as
Rinc's and Studio's apps, would change with it.

## Next

Nothing open: see Leaning yes.

## Leaning yes

- **Generic procs, the rest:** the first pass is done (see Done recently). Still to come:
  - **More on a T:** `for x in xs` and slicing over a `[]T`, `[..]T` builder views (push, pop),
    returning a T, and a T by value in a proc type (`less: proc(a, b: T) -> Bool`).
  - **The descriptor:** each call already passes the address of (T's size, its descriptor), with the
    descriptor 0 for now. Builtins that read it would work on a T: `log` and `format` print it, `==`
    compares Strings by their text, `pack` and `unpack` take it (a T that can't be packed is a compile
    error at the call, where T is known), and a hash builtin would serve a generic map.
  - **Generic methods:** `([..]T).push :: proc[T](value: T) -> Bool`, the receiver's T bound by the
    proc's list. The receiver comes first in the source, before T is named, so it needs reading after.
  - **No constraints and no traits:** anything type-specific, like `<`, comes in as a proc argument:
    `sort(xs[], less)`.
  - **Builtins declared for real:** this would let core.rio declare its generic builtins as
    `([]T).len` and `([..]T).push`. Their rules, like what can be packed or printed, stay in comments.
  - **Sort and find:** these could then be written in Rio. Without generics, they'd be host builtins
    taking a less proc.
- **Returning any value:** a proc returns a value of up to 16 words, views in it or not (host procs
  return no views). What's left, when something needs it:
  - **Big values** (over 16 words): the caller passes an address for the proc to fill in, and `return`
    writes through it. `x := f()` and `x = f()` fill x itself, so nothing is copied (and no type need
    be written to copy it); elsewhere, the result goes in a temporary kept in the caller's frame, one
    for each such call.
- **Comparing a maybe with `==`:** `x == 4` for a `?Int`, false when x is nil, and `x == nil`. Today
  both are errors, and `x is nil` is the way to test for nil.

## Maybe

- **Void as a unit type,** with generics: a zero-sized value, so a Bool is a `?Void`, `proc() -> Void`
  a proc with no result, and a branch with no value a `Void` (as JavaScript's `undefined` or Python's
  `None`). With it, `??T`: a program writing the inner nil, and `is` and printing telling the two
  apart. Until then, an empty struct names a second nothing in a union.
- **Generic types,** in brackets as generic procs are: `Pair :: struct[T]`, used as `Pair[Int]`, only if
  they stay simple. They're what generic maps would need. Some of a type's parameters may be constant
  Ints rather than types, as the builtins' already are (`Blob[16]`, `[..N]T`). Unsure for now.
- **Reflection, through generics:** not now, but designed so generic procs leave room for it. A generic
  proc's hidden descriptor is all reflection needs, so it works on any value at any call, locals
  included, with no stored `Any`: the caller knows the type, as with any generic call.
  - **No recursion:** Rio has none, so a nested value isn't walked by a proc calling itself. A builtin
    `for` walks it instead, a leaf at a time, depth first: `for leaf in leaves(x)`, each leaf a
    builtin union of what can be one (`Int`, `Float`, `Bool`, `String`, an enum as its number and
    name, `nil` for an empty union), with its field's name and depth.
  - **What it would give:** printing and text serialization written in Rio (a writer of JSON, say),
    equality and hashing of whole values, and reading values back from text, field by field.
  - **Writing back** through `&x`, only leaf by leaf, from a leaf of the same kind: a view only from a
    view, an enum checked as `E.fromInt` is, so bytes never become an address.
  - **Costs:** descriptors kept for every type passed as a T, field names too (bytes in the image); a
    slower walk than code written for one type; and a reflection API frozen in the 1.0 spec, which
    every implementation must build. So it waits for a real use, such as a serializer, and builtins
    on descriptors (printing, `==`, pack, hashing) may cover most of what it would.
- **A very limited lifetime marker on `&` parameters:** one marker, nothing more, saying a `&T`
  parameter may point at the caller's locals, so views read through it count as borrowed. Unmarked
  `&T` keeps today's meaning (views read through it are global), so nothing breaks. It would let `&`
  reach locals holding views (Strings, slices, builder views, structs) without changing every `&T`.
  A method's `self` would need the same choice.
- **Layout checks for packed data:** `layoutOf(T)`, a constant hash of a struct's layout (each field's
  name and what it is, as host procs' sigs give it), to keep in a file's header, so reading records
  saved by another version of a program is an error, not misread fields. `sizeOf(T)` in the header
  covers only a change of size.
- **Compile-time `when`:** today, `if CONST` still type-checks a body it never runs. Only if
  differences between hosts need it.
- **A `sys` module for scripting,** in the Rio repo but not standard Rio: a separate `rio` build whose
  host module has files, directories, running programs, the environment and arguments, so scripts
  (tools such as `tools/stats.py`) can be Rio. Built on hosts registering their modules' sources.
- **Host procs calling back into Rio while they run:** a sort with a comparison, a parser handing back
  each element, a physics step calling back on each collision. Today a host proc is a leaf: it can't
  call into the program while it runs. What allowing it would take:
  - **No recursion still:** a host proc marked as calling back counts as calling through a proc value
    for its callers, so they can't be proc values, and no callback can come back around to them (each
    proc's one frame stays safe), by the rule proc values already follow.
  - **The VM entered again** from inside a host proc, nested on the C stack (a cap on how deep, for
    boards with a small stack), with a runtime error in a callback coming back out through the host,
    and the JIT's loops and the C output handling it too.
  - **What it's worth:** mock up the parser and physics cases first, against what already works for
    them: results as data (an array of events), iterator structs, sort in Rio once generics come, and
    proc values the host keeps and calls between runs (`rio_callValue`).

## Before 1.0

- **Memory spec:** what a program can see of memory, so that two implementations agree. Cached images
  needn't be portable between implementations. Source is the portable form.
- **The full spec:** every rule written as an example that the tests run.
- **Audits and cleanup:** every compile error reachable and tested.
- **Keywords:** a last pass makes as many keywords contextual as can be, without hurting error
  messages or highlighting. Declaration words, which only follow `::` (`struct`, `enum`, `union`), are
  the easy ones. Control words at the start of a statement (`if`, `for`, `return`) are riskier, since
  `return (x)` and a call `return(x)` would look alike.
- **Better printing of everything:** every value should print, and print usefully, in `log`,
  `print` and `format` alike. Known gaps:
  - **Procs:** a proc's name as an argument, `log(f)`, is "not a value", though
    `g: proc(x: Int) -> Int = f` works. A proc value prints only `proc`, not which proc; its name would
    mean keeping proc names in images, as enum names are kept.
  - **Floats:** printed to about 7 significant digits, so `1.0 / 3.0` prints `0.333333` and doesn't read
    back as the same Float. The shortest digits that read back exactly would print `0.33333334`.
  - **Types:** `log(Int)` is "not a value". A type's name could print, which would help in generic
    procs if they come.

## Done recently

- **Generic procs, a first pass:** `swap :: proc[T](a, b: &T)`, `find :: proc[T](xs: []T, x: T) -> Int`,
  each T found from the arguments at every call (no `find[Int](...)`, which would read like indexing).
  One compiled copy for every T: a call passes, as a hidden argument, the address of the constant
  (T's size, descriptor), and three instructions take a size from a slot (`idxv`, `copyv`, `eqv`; images
  are version 14). In the body a T is copied, taken from a `[]T`, compared with `==` by its bytes, and
  passed on; a call checks a compared T holds no views and a T kept in a variable is at most 16 words.
  `tests/map.rio`'s second map is generic procs over arrays the caller keeps.

- **Literals sized and typed by their values:** `[_]T{...}` takes its size from its values, `[_]{...}`
  its element type from the first too, and slice literals, `[]T{...}`, `[]{...}`, and `{...}` where a
  slice goes, view an array kept as a hidden variable's would be (a top-level one's starting data, or
  a proc's own memory). The literal grows a value at a time, so nothing reads ahead.

- **Literals of constants built while compiling:** a literal whose values are all constants (a table
  of Ints or Strings, nested arrays, structs) is run as it's compiled, its code dropped, and its words
  kept with the constants: one instruction copies them in where it goes. A top-level variable from one
  starts as those words, kept with the constants in memory and images, so nothing runs to set it. A
  64-Int table at the top level costs 284 bytes over an empty program, not 1,316. Seven of rinc's and
  studio's apps, unchanged, take 10.6 KB less RAM (colors 4.6 KB, sound 2.6 KB, themes 1.7 KB), once
  rinc moves to this Rio.

- **Returning values holding views:** a struct with a `String` field, a `?String`, an array or union
  of views, returned as a `String` is, so an iterator's `next` can give text. Holding a view of the
  proc's own memory is an error, and the caller's copy counts as viewing what went in. `self` or a `&`
  argument counts as its own memory only when there's an array or builder in it to view (a struct of
  views and numbers is what it holds). Returning a parameter's view from inside a loop works now too.

- **Stats as charts:** `bench/stats.svg`, drawn from `bench/stats.csv` by `tools/stats.py` (`--record`
  redraws it; CI fails when it's stale), shown in the README. Earlier commits backfilled with
  `--backfill`, each built and measured in a worktree. A row from another machine shows hollow.

- **Static procs on types:** `Rect.of :: static proc(...) -> Rect`, called as `Rect.of(...)`, with no
  `self`, and a proc value as any proc. `static` is a word only there, so it reserves nothing. Methods
  keep their implicit `self`: the marker goes on the rare kind.

- **Narrowing when a test is false:** for a maybe or a union of two types, `not n`, `n is nil` and
  `not (u is A)` narrow on the right of `or`, in the branches after (`case`, `else`), and after an `if`
  whose branch leaves (`if not n do return`: n its value from there, as Swift's `guard let`).

- **Bool as a maybe of nothing:** maybes work where Bools do: `if x` (x its value inside), `not x`, `x and e`
  (e if x is there, x its value in e; a maybe of e, unless e is a Bool: `t and t.x`), and `x or y` (x's
  value, else y: `n := s.toInt() or 0`). A `?Bool` isn't a condition
  (there, or true?). Only Bools and maybes are conditions.

- **if and switch as values:** right after `:=`, `=` or `return`, each branch's last line its value;
  an `else` required, except in a switch with a case for every value of an enum or type of a union.
  Never part of an expression.

- **No more arithmetic on structs:** only arrays of numbers do arithmetic as wholes (a struct's is a
  compile error pointing to `[2]Float` and its swizzles), so a vector has one way to be written.

- **Math on arrays:** the math builtins, `min` and `max`, and `toInt` and `toFloat`, element by element
  on arrays of numbers, a number spread over every element, and `clamp(x, lo, hi)` for numbers and
  arrays alike (`clamp(p, 0, hi)`). An array literal
  with no type takes its partner's in arithmetic (`p + {1, 0}`, `p *= {2, 1}`). Arithmetic on arrays
  of up to 4 numbers compiles to an instruction an element, so native loops have
  it (on the Fruit Jam, 256 balls' physics with `[2]Float` points: 0.52 ms a frame before, 0.25 after,
  against 0.19 with separate x and y).

- **Iterator structs:** `for x in it` (and `for x, i in it`, the turn from 0) over any struct with a
  method `next() -> ?T`, until it gives nil: a variable stepped in place, so a `break` leaves it where it
  stopped; anything else kept for the loop. A `next` can give text (`-> ?String`, the next word of a
  line) since values holding views can be returned.

- **Text as numbers:** `s.toInt()`, `s.toInt(base)` (2 to 36) and `s.toFloat()`, on Strings, Blobs and
  byte builders, each a maybe: nil when the text isn't a number (images are version 13). A Float reads
  as its literal would.

- **`>>>`:** an unsigned right shift, and `>>>=` (images are version 12). Unsigned compare, divide and
  remainder, if they come, would be builtins rather than operators.

- **Proc type aliases:** `Less :: (proc(a, b: Int) -> Bool)`, in parentheses, since a proc type alone
  after `::` starts a proc.

- **`end` in index brackets:** the last index of what's indexed, `xs[end]`, `xs[end - 2..=end]`: a
  constant for an array, the length less 1 for the rest; not for an `[E]T`, whose indexes are named.

- **Unicode escapes:** `\u{1F600}`, its code point's bytes in UTF-8 in a string, and the code point
  itself in a character literal; past 10FFFF and surrogates are errors.

- **No more `diverse union`:** any union is allowed, whatever its types' sizes; each value takes its
  biggest type's room, a cost in memory like a big array's (`rio -m` shows it). `diverse` is an
  ordinary name again.

- **Enums by name in layouts:** `Enum{game,ui,none}` (a bitfield's `Enum(2){...}`), as a struct's field
  and as a host proc's parameter in its sig, so an image made before an enum's values were reordered
  doesn't fit, while a parameter's values added at its end do. A plain `i` for an enum still works.

- **`core`, a module:** the builtins are module `core`, there everywhere unqualified as before, and as
  `core.name`, which reaches a builtin a program's own name hides. (`src/core.rio` stays its
  declarations, checked by the tests, not a source compiled into programs: that would cost every
  compile.)

- **Hosts giving their modules' sources:** `rio_addSource(vm, "pc", src, len)`, taken before the
  loader, so declarations go with the code they declare, and given back by `rio_hostSource` for help
  or an editor. `rio -H` writes the source (`pc_SOURCE`, `pc_HASH`) and the host procs as an X macro,
  `pc_HOST_PROCS(X)`, beside the structs and sigs. Images keep each source's name and hash, named in
  the message when a host proc doesn't fit: a name for what an image was made with, not a gate, since
  images are meant to survive what only adds. Images are version 11.

- **Host procs as proc values:** `f := pc.cls`, and as an argument or element. Each one used as a value
  gets a small proc of its own that calls it, made at the end of compiling, so calls through values
  stay as they are and the host call does the converting (structs by address, results placed). A host
  proc can't call back into the program, so it can always be a value. With this, what one kind of proc
  can do, the other can.

- **Host procs returning structs and maybes** (`>{...}`, `>Union{nil|Int}` in a sig): the call passes
  where the result goes first, and the host writes it with `rio_putStruct`, so a struct result may
  gain fields at its end and older images still get what they hold.

- **Host procs taking `&T`:** an `Int`, a `Float` or a struct the host may change (`&i`, `&f`,
  `&{...}` in its sig), written back with `rio_putStruct`, no more than the place holds.

- **Optional trailing parameters:** `style?: Style`, left out of a call, is zero, as a variable is: in
  procs, methods, proc values and host procs alike. Only zero, the rule images already follow for host
  procs that gained parameters at their end, so a host proc can gain options once its API is frozen
  (Rinc's `pc`) without breaking programs, in source or compiled.

- **Long argument lists, as struct arguments:** `text("hi", 3, 4, {color = 8, scale = 2})`, what's
  left out zero, as for variables. Structs holding views (a `String` field) can be locals and by-value
  parameters, borrowed like a `String` parameter's (tests/viewtest.py checks the rules with random
  programs), and host procs take structs, by address, with their layouts in the sig; `rio -H` writes
  the matching C from the module's source. Images bind only the host procs they call, and a host proc
  may gain parameters at its end, or a struct fields at its end, which older images give as zeros.

- **`sizeOf(T)`, `offsetOf(S, .field)` and `bitOffsetOf(S, .field)`:** constants, for packed records
  and binary formats.

- **Enums and Bools in bitfields:** `on: Bool(1)`, and `kind: Shape(2)`, with a compile-time check
  that 2 bits hold every Shape. They print by name. Unpacking a value its type doesn't have (an enum,
  a Bool or a union's tag, bitfield or not) is a runtime error, as with `E.fromInt`.

- **Narrowing through `and`:** in `if x is Int and x > 2`, x is an Int after the `and`, not just in
  the if's body.
- **Packing:** `pack`, `unpack`, `getInt` and `setInt`.
- **Hex escapes,** with logs escaping control bytes.
- **Bitfields:** `Int(n)` and `Int(-n)`.
- **Enums:** always 0-based and contiguous.
- **Proc values as elements.**
- **Ranges as values,** with range slicing instead of `:`.
- **Names:** `[..N]T` is a builder and `[..]T` a builder view.
- **A map in Rio:** tests/map.rio, a hash table in user code, String to Int and Int to a struct. Keys
  are copied into an arena of the map's own, since views passed in are borrowed. Each key and value
  type needs its own copy of the code until generic procs, which it's to be tried again with.
- **`do`:** one-line blocks (`if done do return`, `case .red do stop()`), and `do ... end` blocks
  for names.
- **No `@`:** `cls* :: host proc(c: Int)`. `host` is an ordinary name anywhere else.
