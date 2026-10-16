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

A name is a letter or `_`, then letters, digits and `_`: `x`, `hp2`, `_tmp`, `fromHex`. Case
matters. By convention, types are `PascalCase` and everything else, enum values included, is
`camelCase`; constants are often `UPPER_CASE`.

Keywords can't be names:

```
and break case continue else end enum for if import in include is nil not or proc
return struct switch union
```

A few names mean something only in one place, and are ordinary names elsewhere: `self` in methods,
`as` in imports and destructuring, and annotation names after `@`. The builtin names (`Int`,
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

A character literal is one byte, as an `Int`: `'a'` is 97. A string literal is `"..."`, and can span
lines. Both take these escapes, and any other backslash is an error:

| escape | byte |
|---|---|
| `\n` `\t` `\r` `\0` | newline, tab, carriage return, zero |
| `\\` `\"` `\'` | the character itself |

```rio
log('a', '\n', "say \"hi\"", "two
lines")
```

```output
97 10 say "hi" two
lines
```

```rio
log("\q")  # error: unknown escape
```

## Operators and punctuation

```
+  -  *  /  %  &  |  ^  ~  <<  >>        arithmetic and bits
== != <  <= >  >=  and  or  not  is      comparison and logic
=  :=  :  ::  +=  -=  *=  /=  %=  &=  |=  ^=  <<=  >>=
( ) [ ] { } , . ; ..< ..= ->  &  *  @
```

`&&`, `||` and `!` are errors that name the word to use instead (`and`, `or`, `not`).
