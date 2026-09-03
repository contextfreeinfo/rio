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

time cmake --build $dir
ls -l $dir/rio
$dir/rio "$args"
time $dir/rio "$args" > /dev/null
# /usr/bin/time -v $dir/rio "$args" > /dev/null
