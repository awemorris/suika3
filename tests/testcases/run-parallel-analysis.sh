#!/bin/sh

set -eu

case_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
root=$(CDPATH= cd -- "$case_dir/../.." && pwd)
NOCT=${NOCT:-"$root/build-static/noct"}
PARALLEL_EXPECT_SIMD=${PARALLEL_EXPECT_SIMD:-1}
tmp=${TMPDIR:-/tmp}/noct-parallel-analysis.$$

cleanup()
{
    rm -rf "$tmp"
}
trap cleanup EXIT HUP INT TERM
mkdir -p "$tmp"

echo 'Target-neutral loop analysis tests:'
for opt in 0 1 2 3; do
    NOCT_PARALLEL_DEBUG=1 LC_ALL=C "$NOCT" -j0 -O"$opt" \
        "$case_dir/parallel-analysis/basic.noct" \
        >"$tmp/stdout" 2>"$tmp/stderr"
    grep '^parallel-analysis ' "$tmp/stderr" >"$tmp/actual"
    diff "$case_dir/parallel-analysis/basic.noct.out" "$tmp/actual"
done

if [ "$PARALLEL_EXPECT_SIMD" -ne 0 ]; then
    echo 'SIMD/common-analysis integration:'
    LC_ALL=C "$NOCT" --simd-info -j0 -O2 \
        "$case_dir/simd/blend.noct" >"$tmp/simd.stdout" 2>"$tmp/simd.stderr"
    grep 'SIMD: .*blend.noct:11: vectorized (i32x4)$' \
        "$tmp/simd.stderr" >/dev/null
    diff "$case_dir/simd/blend.noct.out" "$tmp/simd.stdout"
else
    echo 'SKIP SIMD/common-analysis integration (optimizer disabled)'
fi
echo 'PASS'
