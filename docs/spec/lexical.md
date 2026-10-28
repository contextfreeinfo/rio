# Source text

A source file is bytes, normally UTF-8. Names and keywords are ASCII; strings and comments can hold
any bytes.

## Lines and statements

A statement ends at the end of its line, or at `;`. An expression goes on to the next line inside
brackets (`()`, `[]`, `{}`), or after an operator that ends a line, but not before one that starts a
line:

```rio
a := 1; b := 2
total := a +
  b
sum := (a
  + b)
log(total, sum)
```

```output
3 3
```

```rio
a := 1
b := a
  + 1  # error: expected expression
```

Indentation means nothing to the compiler. Blocks end with `end`.

## Comments

`#` starts a comment, to the end of the line (so `#!` works on the first line). There are no block
comments, and `//` is an error that says to use `#`:

```rio
x := 1  # one
y := "# is just a character here"
log(y)
```

```output
# is just a character here
```

```rio
x := 1  // one  # error: comments start with #
```

## Names

A name is a letter or `_`, then letters, digits and `_`: `x`, `hp2`, `_tmp`, `fromHex`, up to 255
characters long. Case matters. By convention, types are `PascalCase` and everything else, enum values included, is
`camelCase`; constants are often `UPPER_CASE`.

Keywords can't be names:

```
and break case continue do else end enum for if import in include is nil not or proc
return struct switch union
```

A few names mean something only in one place, and are ordinary names elsewhere: `self` in methods,
`as` in imports and destructuring, `by` in for loops, and `host` before `proc` or `struct` in a
declaration. The builtin names (`Int`,
`true`, `sqrt`, ...) are declared outside every module, so a declaration with the same name hides
them.

## Numbers

Integer literals are decimal, hex (`0x`) or binary (`0b`), and `_` can go between any two digits.
A decimal literal can be at most 2147483647 (and 2147483648 right after `-`). Hex and binary give
any 32 bits:

```rio
log(1_000_000, 0xFF, 0b1010, 0xFFFF_FFFF, -2147483648)
```

```output
1000000 255 10 -1 -2147483648
```

```rio
x := 2147483648  # error: number too big for Int
```

A float literal has a decimal point with digits on both sides, or an exponent, or both: `1.5`,
`2e10`, `1.5e-3`. `1.` and `.5` aren't numbers. Floats are 32-bit, so a literal rounds to the
nearest `Float`:

```rio
log(1.5, 2e3, 1_000.25, 123456789.0)
```

```output
1.5 2000.0 1000.25 123456792.0
```

## Characters and strings

A character literal is one byte, as an `Int`: `'a'` is 97. A string literal is `"..."`. Both take
these escapes, and any other backslash is an error:

| escape | byte |
|---|---|
| `\n` `\t` `\r` `\0` | newline, tab, carriage return, zero |
| `\\` `\"` `\'` | the character itself |
| `\xHH` | the byte of those two hex digits (`\x1b` is 27) |
| `\u{1F600}` | a code point, by its 1 to 6 hex digits: in a string, its bytes in UTF-8; as a character, the code point itself (`'\u{E9}'` is 233) |

```rio
log('a', '\n', "say \"hi\"", '\xff', "\x48\x69".len())
log("caf\u{E9}", "caf\u{E9}".len(), "\u{1F600}".len(), '\u{E9}')
```

```output
97 10 say "hi" 255 2
café 5 4 233
```

A code point past 10FFFF, or a surrogate (D800 to DFFF, which UTF-8 doesn't have), is an error:

```rio
log("\u{D800}")  # error: \u{...} takes 1 to 6 hex digits, a code point up to 10FFFF and not a surrogate
```

```rio
log("\q")  # error: unknown escape
```

```rio
log("\x4")  # error: \x takes two hex digits
```

### Strings over more lines

A line with no closing quote ends its string with a line break, and the string goes on if the next line
starts with a quote. Each line's indent comes before its quote, so it isn't part of the string, and
neither are the spaces and tabs at the end of an open line. Close the last line for no line break at
the end:

```rio
keys :=
  "Keys:
  "  arrows  move
  "  Z       jump"
log(keys)
```

```output
Keys:
  arrows  move
  Z       jump
```

Left open, the last line keeps its line break too, and a line of just a quote is an empty one. A string
ends with its line unless the next one starts with a quote, so a quote left open by mistake goes no
further:

```rio
s :=
  "one
  "
  "three
log(s.len(), s == "one\n\nthree\n")
```

```output
11 true
```

### Strings side by side

Strings side by side are one string: on one line, and over more lines inside brackets. That splits a
long line without breaking it, and keeps spaces at the end of a line:

```rio
log("abc" "def")
long := (
  "a long line, "
  "split in two"
)
log(long)
log("two spaces:  " "|")
```

```output
abcdef
a long line, split in two
two spaces:  |
```

## Operators and punctuation

```
+  -  *  /  %  &  |  ^  ~  <<  >>        arithmetic and bits
== != <  <= >  >=  and  or  not  is      comparison and logic
=  :=  :  ::  +=  -=  *=  /=  %=  &=  |=  ^=  <<=  >>=
( ) [ ] { } , . ; ..< ..= ->  &  *
```

`&&`, `||` and `!` are errors that name the word to use instead (`and`, `or`, `not`).
