run-case() {
    date -u --iso-8601=ns
    time (zig build test $1)
    time (RIO_TEST_SCALE=10_000 zig build test $1)
    time (zig build $1 && gzip -kf zig-out/bin/rio && wc -c zig-out/bin/rio zig-out/bin/rio.gz)
    time (zig build $1 -Dtarget=wasm32-wasi && wasm-opt -O4 zig-out/bin/rio.wasm -o zig-out/bin/rio-opt.wasm && gzip -kf zig-out/bin/rio-opt.wasm && wc -c zig-out/bin/rio*.wasm*)
}

run-all() {
    date -u --iso-8601=ns
    git rev-parse HEAD
    git status --untracked-files=no
    wc src/*.zig
    wc tests/*.rio
    lscpu
    rm -rf zig-cache zig-out
    run-case
    run-case -Drelease-fast
    run-case -Drelease-small
}

run() {
    set -x
    time run-all
}

> report.txt
run "$@" 2>&1 | tee -a report.txt
