# Rio

A programming language someday maybe.

Try this for now:

```sh
cargo run -- run examples/hi.rio
```

Or for more fun:

```sh
cargo build --release && \
ls -lh target/release/rio && \
/usr/bin/time -v target/release/rio run examples/hi.rio
```
