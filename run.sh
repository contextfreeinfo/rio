set -e
time odin build rio # -o:size
ls -lh
time ./rio.bin examples/hi
