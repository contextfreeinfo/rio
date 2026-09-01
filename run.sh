set -e

# Check for and remove
dir=build
args=()
while [[ $# -gt 0 ]]; do
    case "$1" in
        --thumb)
            dir=build-thumb
            toolchain="--toolchain toolchains/thumb2.cmake"
            shift
            ;;
        *)
            args+=("$1")
            shift
            ;;
    esac
done

if [ ! -d "$dir" ]; then
    time cmake -B "$dir" $toolchain
fi

time cmake --build build
ls -l "$dir/rio"
build/rio "$args"
time build/rio "$args" > /dev/null
# /usr/bin/time -v build/rio "$args" > /dev/null
