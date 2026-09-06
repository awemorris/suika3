#!/bin/sh

set -eu

root=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
build_dir=${1:-"$root/build-static"}
case "$build_dir" in
/*) ;;
*) build_dir="$root/$build_dir" ;;
esac
cc=${CC:-cc}
test_bin="$build_dir/noct-repl-session-test"

test -x "$build_dir/noct" || {
	echo "Noct CLI not found: $build_dir/noct" >&2
	exit 1
}
test -f "$build_dir/libnoct.a" || {
	echo "Noct static library not found: $build_dir/libnoct.a" >&2
	exit 1
}

"$cc" -I"$root/include" "$root/tests/testcases/repl-session-test.c" \
	"$build_dir/libnoct.a" -lm -o "$test_bin"
"$test_bin"
python3 "$root/tests/testcases/run-repl.py" "$build_dir/noct"
