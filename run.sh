cargo build --release && \
ls -lh target/release/rio && \
/usr/bin/time -v target/release/rio run examples/hi.rio --dump examples/trees
