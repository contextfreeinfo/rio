#!/usr/bin/env bash

run-dump() {
    # RUST_LOG=debug \
    RUST_BACKTRACE=1 /usr/bin/time -v \
        target/$profile/rio run examples/$1.rio --dump examples/trees
}

# profile=debug
# cargo build && \
profile=release
cargo build --release && \
ls -l target/$profile/rio && \
run-dump hi && \
run-dump wild

# cargo build --profile release-lto && ls -l ./target/release-lto/rio
