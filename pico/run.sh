set -e
# First time:
# time cmake -S . -B build

time cmake --build build
ls -l build
build/rio "$@"
time build/rio "$@" > /dev/null
# /usr/bin/time -v build/rio "$@" > /dev/null
