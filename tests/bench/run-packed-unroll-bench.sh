#!/bin/sh

set -eu

bench_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
repo_dir=$(CDPATH= cd -- "$bench_dir/.." && pwd)
BUILD_DIR=${BUILD_DIR:-"$repo_dir/build-static"}
CC=${CC:-cc}
SAMPLES=${SAMPLES:-11}

if [ ! -f "$BUILD_DIR/libnoct.a" ] ||
   [ ! -f "$BUILD_DIR/libnoctapi.a" ]; then
    echo "Noct libraries not found in $BUILD_DIR" >&2
    exit 2
fi

tmp_dir=$(mktemp -d)
trap 'rm -rf -- "$tmp_dir"' EXIT HUP INT TERM
harness="$tmp_dir/packed-unroll-bench"
system_libs=-lm
case "$(uname -s)" in
Linux)
    system_libs="$system_libs -lutil"
    if command -v pkg-config >/dev/null 2>&1 &&
       pkg-config --exists egl glesv2; then
        system_libs="$system_libs $(pkg-config --libs egl glesv2)"
    fi
    ;;
esac

# This general function-boundary harness accepts CPU function names in both
# slots. No accelerator call or transfer is part of either measurement.
# shellcheck disable=SC2086
"$CC" -O2 -std=c11 -Wall -Wextra -Werror -I"$repo_dir/include" \
    "$bench_dir/gpu-cnn-call-bench.c" \
    "$BUILD_DIR/libnoct.a" "$BUILD_DIR/libnoctapi.a" \
    $system_libs -o "$harness"

run_one()
{
    mode=$1
    shift
    "$@" NOCT_BENCH_SINGLE_INVOCATION=1 "$harness" \
        0.001 0.001 "$SAMPLES" "$bench_dir/packed-unroll.noct" \
        pu_setup pu_run pu_verify pu_run pu_verify 2>/dev/null | \
        awk -F, -v mode="$mode" \
            '$1 == "cpu-jit" { print mode "," $4 "," $5 "," $6 }'
}

echo 'mode,best_ms,median_ms,worst_ms'
run_one default env
