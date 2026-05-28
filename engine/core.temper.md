# Core library

Define the core library in Rio source. Although we could consider caching some,
if we're fast enough and core is small enough, it won't matter too much for now.

For now, the core library is loaded in an Engine.

## Core library source

It would be nice to have this as a separate file, but we don't currently have
file data include in Temper.

    let coreSource = """
      "union Claim = Maybe[Void]
      "
      "union Either[YesValue, NoValue]
      "    Yes[YesValue]
      "    No[NoValue]
      "end
      "
      "struct Error
      "    message Text
      "end
      "
      "fun error(message Text) Error
      "    Error { message }
      "end
      "
      "var false = null[Void]
      "
      "struct Float
      "end
      "
      "struct Int
      "end
      "
      "class List[Item]
      "end
      "
      "fun log(message Text) Void
      "end
      "
      "union Maybe[Value] = Either[Value, Void]
      "
      "struct No[YesValue, NoValue]
      "    value NoValue
      "end
      "
      "fun no[YesValue, NoValue](value NoValue) No[YesValue, NoValue]
      "    No { value }
      "end
      "
      "var null[Value] = no[Value, _](void)
      "
      "class Text
      "end
      "
      "var true = yes(void)
      "
      "union Try[Value] = Either[Value, Error]
      "
      "struct Void
      "end
      "
      "var void Void
      "
      "struct Yes[Value]
      "    value Value
      "end
      "
      "fun yes[Value](value Value) Yes[Value]
      "    Yes { value }
      "end
    ;
