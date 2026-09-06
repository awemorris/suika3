#!/bin/sh

set -eu

bench_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
repo_dir=$(CDPATH= cd -- "$bench_dir/.." && pwd)
BUILD_DIR=${BUILD_DIR:-"$repo_dir/build-static"}
CC=${CC:-cc}
SAMPLES=${SAMPLES:-11}

test -f "$BUILD_DIR/libnoct.a" || {
    echo "Noct static library not found: $BUILD_DIR/libnoct.a" >&2
    exit 2
}

tmp_dir=$(mktemp -d)
trap 'rm -rf -- "$tmp_dir"' EXIT HUP INT TERM
harness="$tmp_dir/objectmodel-bench"
system_libs="-lm"
case "$(uname -s)" in
Linux)
    system_libs="$system_libs -lutil -pthread"
    if command -v pkg-config >/dev/null 2>&1 &&
       pkg-config --exists egl glesv2; then
        system_libs="$system_libs $(pkg-config --libs egl glesv2)"
    fi
    ;;
Darwin) system_libs="$system_libs -pthread" ;;
esac

# gpu-cnn-call-bench is also a general two-function VM boundary timer. Both
# slots point to the same CPU function here; accelerator work is skipped.
# shellcheck disable=SC2086
"$CC" -O2 -std=c11 -Wall -Wextra -Werror -I"$repo_dir/include" \
    "$bench_dir/gpu-cnn-call-bench.c" \
    "$BUILD_DIR/libnoct.a" "$BUILD_DIR/libnoctapi.a" \
    $system_libs -o "$harness"

run_model()
{
    model=$1
    NOCT_BENCH_OBJECT_MODEL="$model" \
    NOCT_BENCH_SINGLE_INVOCATION=1 \
    "$harness" 0.001 0.001 "$SAMPLES" "$bench_dir/objectmodel.noct" \
        om_setup om_run om_verify om_run om_verify 2>/dev/null | \
        awk -F, -v model="m$model" \
            '$1 == "cpu-jit" { print model "," $4 "," $5 "," $6 }'
}

echo 'model,best_ms,median_ms,worst_ms'
run_model 0
run_model 1
