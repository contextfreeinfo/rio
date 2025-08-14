# Rio programming language

## Examples as tests

```
cargo run --example bin-test
cargo run --example bin-test -- --profile release-lto
```

## Thoughts on native imports

Can something like allow passing around import access without requiring making
it an interface with extra indirection?

```rb
# Group of related imports.
io = extern of
    print = for ...
    # ...
end
```

Maybe accept it in a function like this?

```rb
printThings = for(things Things, io extern) be
    # ...
end
```

Maybe allow easy definition of an interface? Alternatively, just use an
interface impl in the first place, but that requires more setup and could
presumably slow down all uses. Might matter in some cases?

```rb
Io = interface of io extern
```
