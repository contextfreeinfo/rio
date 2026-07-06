set -e
# First time:
# time cmake -S . -B build

time cmake --build build
ls -l build
build/rio "$@"
