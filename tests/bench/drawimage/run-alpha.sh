#!/bin/sh

# Function-boundary benchmark for the canonical DRAW_IMAGE_ALPHA Noct source.
# JIT compilation, warmup and buffer restoration are outside each sample.

set -eu

bench_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
repo_dir=$(CDPATH= cd -- "$bench_dir/../.." && pwd)
build_dir=${NOCT_BUILD:-"$repo_dir/build-static"}
cc=${CC:-cc}
samples=${SAMPLES:-50}
pixels=${PIXELS:-1000000}
levels=${LEVELS:-"2 3"}
cpu=${CPU:-}

case "$samples:$pixels" in
*[!0-9:]*|0:*|*:0) echo "SAMPLES and PIXELS must be positive integers" >&2; exit 2 ;;
esac
if [ -n "$cpu" ] && ! command -v taskset >/dev/null 2>&1; then
    echo "CPU was specified but taskset is unavailable" >&2
    exit 2
fi
for library in libnoctapi.a libnoct.a; do
    if [ ! -f "$build_dir/$library" ]; then
        echo "Noct static library not found: $build_dir/$library" >&2
        exit 2
    fi
done

tmp_dir=$(mktemp -d)
trap 'rm -rf -- "$tmp_dir"' EXIT HUP INT TERM
binary="$tmp_dir/alpha-call-bench"
source="$repo_dir/tests/testcases/simd/drawimage/blend-alpha.noct"

"$cc" -O2 -I"$repo_dir/include" -I"$repo_dir/src/core" \
    "$bench_dir/alpha-call-bench.c" \
    "$build_dir/libnoctapi.a" "$build_dir/libnoct.a" -lm -o "$binary"

invoke()
{
    if [ -n "$cpu" ]; then
        taskset -c "$cpu" "$@"
    else
        "$@"
    fi
}

for level in $levels; do
    case "$level" in
    0|2|3) ;;
    *) echo "LEVELS accepts only 0, 2 and 3" >&2; exit 2 ;;
    esac
    invoke "$binary" "$level" "$samples" "$pixels" "$source"
done
