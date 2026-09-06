#!/bin/sh

set -eu

root=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
build_dir=${1:-"$root/build-debug"}
case "$build_dir" in
/*) ;;
*) build_dir="$root/$build_dir" ;;
esac
cc=${CC:-cc}
out="$build_dir/fma-helper-test"
cppflags=

if grep -q '^NOCT_ENABLE_MULTITHREAD:BOOL=ON$' \
	"$build_dir/CMakeCache.txt"; then
	cppflags=-DNOCT_USE_MULTITHREAD
fi

"$cc" $cppflags -I"$root/include" -I"$root/src/core" \
    "$root/tests/testcases/fma-helper-test.c" \
    "$build_dir/libnoctapi.a" "$build_dir/libnoct.a" -lm -o "$out"
"$out"
