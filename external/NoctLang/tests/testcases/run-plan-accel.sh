#!/bin/sh

set -eu

case_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
root=$(CDPATH= cd -- "$case_dir/../.." && pwd)
build_arg=${1:-build-accel-common}

case "$build_arg" in
/*)
	build_dir=$build_arg
	;;
*)
	build_dir=$root/$build_arg
	;;
esac

${CMAKE:-cmake} --build "$build_dir" --target noct-test-accel-plan
"$build_dir/noct-test-accel-plan" "$root/tests/testcases/accel-plan"
