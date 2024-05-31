# Rio

A programming language someday maybe.

Try this for now:

```sh
cargo run -- build examples/hi.rio
```

Or for more fun, which cargo installs wasm-tools and wasmi_cli then builds and
runs rio and so on:

```sh
cargo run --example bin-test
```

Or for smaller binary size and maybe faster, though I don't have large enough
rio examples yet to know:

```sh
cargo run --example bin-test -- --profile release-lto
```

Example usage of wasm-opt:

```sh
wasm-opt -O3 --enable-multivalue --enable-bulk-memory \
    examples/out/branch.wasm -o examples/out/branch-opt.wasm
```
