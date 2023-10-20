# profile=debug
# cargo build && \
profile=release
cargo build --release && \
ls -lh target/$profile/rio && \
RUST_BACKTRACE=1 /usr/bin/time -v target/$profile/rio run examples/hi.rio --dump examples/trees && \
RUST_BACKTRACE=1 /usr/bin/time -v target/$profile/rio run examples/crazy.rio --dump examples/trees
