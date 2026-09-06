#!/bin/sh

set -eu

root=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
build_dir=${1:-"$root/build-static"}
case "$build_dir" in
/*) ;;
*) build_dir="$root/$build_dir" ;;
esac
cc=${CC:-cc}
test_bin="$build_dir/noct-api-public-test"
system_libs="-lm"
case "$(uname -s)" in
Linux) system_libs="$system_libs -lutil -pthread" ;;
Darwin) system_libs="$system_libs -pthread" ;;
esac
if command -v pkg-config >/dev/null 2>&1 &&
   pkg-config --exists egl glesv2; then
	system_libs="$system_libs $(pkg-config --libs egl glesv2)"
fi

test -f "$build_dir/libnoct.a" || {
	echo "Noct static library not found: $build_dir/libnoct.a" >&2
	exit 1
}
test -f "$build_dir/libnoctapi.a" || {
	echo "Noct API static library not found: $build_dir/libnoctapi.a" >&2
	exit 1
}

# Exercise only the maintained public registrars.  Target injection backends
# are deliberately not part of the Noct API.
# shellcheck disable=SC2086
"$cc" -I"$root/include" "$root/tests/testcases/api-public-test.c" \
	"$build_dir/libnoctapi.a" "$build_dir/libnoct.a" \
	$system_libs -o "$test_bin"
"$test_bin"
