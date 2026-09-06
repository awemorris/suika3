#!/bin/sh

set -eu

case_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
root=$(CDPATH= cd -- "$case_dir/../.." && pwd)
build_dir=${1:-"$root/build-static"}
case "$build_dir" in
/*) ;;
*) build_dir="$root/$build_dir" ;;
esac
noct=${NOCT:-"$build_dir/noct"}
cc=${CC:-cc}
tmp=${TMPDIR:-/tmp}/noct-mmap-test.$$
system_libs="-lm"
case "$(uname -s)" in
Linux) system_libs="$system_libs -lutil -pthread" ;;
Darwin) system_libs="$system_libs -pthread" ;;
esac
if command -v pkg-config >/dev/null 2>&1 &&
   pkg-config --exists egl glesv2; then
    system_libs="$system_libs $(pkg-config --libs egl glesv2)"
fi

cleanup()
{
    rm -rf "$tmp"
}
trap cleanup EXIT HUP INT TERM
mkdir -p "$tmp"

make_data()
{
    dd if=/dev/zero of="$tmp/mmap-data.bin" bs=64 count=1 2>/dev/null
}

run_basic()
{
    label=$1
    shift
    make_data
    echo "  $label"
    (cd "$tmp" && "$noct" "$@" "$case_dir/fileutil/mmap-basic.noct") \
        >"$tmp/out" 2>"$tmp/err"
    diff "$case_dir/fileutil/mmap-basic.noct.out" "$tmp/out"
}

expect_error()
{
    label=$1
    options=$2
    source=$3
    expected=$4
    make_data
    echo "  $label"
    if (cd "$tmp" && "$noct" $options "$case_dir/fileutil/$source") \
        >"$tmp/out" 2>&1; then
        echo "Expected failure: $source" >&2
        exit 1
    fi
    grep -F "$expected" "$tmp/out" >/dev/null || {
        cat "$tmp/out" >&2
        echo "Missing expected error: $expected" >&2
        exit 1
    }
}

echo 'FileUtil mmap tests'
run_basic 'interpreter' -j0
run_basic 'JIT + O2' -O2

expect_error 'double munmap' '-j0' mmap-double-unmap.noct 'Packed is unmapped.'
expect_error 'closed alias through optimized loop' '-O2' mmap-closed.noct 'Packed is unmapped.'
expect_error 'typed alignment' '-j0' mmap-invalid-alignment.noct 'not aligned to the element size'
expect_error 'write-only permission' '-j0' mmap-invalid-permission.noct 'Write-only file mappings are not supported.'
expect_error 'negative offset' '-j0' mmap-negative.noct 'must not be negative'
expect_error 'range past EOF' '-j0' mmap-past-eof.noct 'exceeds the file size'
expect_error 'ordinary Packed munmap' '-j0' mmap-ordinary.noct 'Packed is not a file mapping.'

# shellcheck disable=SC2086
"$cc" -I"$root/include" "$case_dir/packed-finalizer-test.c" \
    "$build_dir/libnoct.a" $system_libs -o "$tmp/packed-finalizer-test"
"$tmp/packed-finalizer-test"

echo 'All FileUtil mmap tests passed.'
