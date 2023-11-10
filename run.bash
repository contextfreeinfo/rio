#!/usr/bin/env bash

run-dump() {
    # RUST_LOG=debug \
    RUST_BACKTRACE=1 /usr/bin/time -v \
        target/$profile/rio run examples/$1.rio \
            --dump trees --outdir examples/out
    # target/$profile/wasmprinter examples/out/$1.wasm
}

main() {
    install-tools
    # profile=debug
    # cargo build && \
    profile=release
    cargo build --release && \
    ls -l target/$profile/rio && \
    run-dump hi && \
    run-dump wild
    # cargo build --profile release-lto && ls -l ./target/release-lto/rio
}

install-tools() {
    if ! command -v wasm-tools >/dev/null 2>&1; then
        cargo install wasm-tools
    fi
    if ! command -v wasmi_cli >/dev/null 2>&1; then
        cargo install wasmi_cli
    fi
}

main
