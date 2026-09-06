#!/bin/sh

set -eu

root=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
build_dir=${1:-build-accel-common}

case "$build_dir" in
/*)
	;;
*)
	build_dir=$root/$build_dir
	;;
esac

${CMAKE:-cmake} --build "$build_dir" --target noct-test-accel-rewrite
"$build_dir/noct-test-accel-rewrite" "$root/tests/testcases/accel-rewrite"
