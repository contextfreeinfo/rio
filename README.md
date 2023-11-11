# Rio

A programming language someday maybe.

Try this for now:

```sh
cargo run -- run examples/hi.rio
```

Or for more fun, which cargo installs wasm-tools and wasmi_cli:

```sh
python run.py
```

Or maybe soon just this? It abusively builds rio separately and runs it. It also
cargo installs additional binaries such as wasm-tools and/or wasmi_cli.

TODO Just download third-party binaries directly into rio's own app space if not
already present on the system? Should rio itself do this for some options? Maybe
offer to do so?

```sh
cargo run --example bin-test
```
