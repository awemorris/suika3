#!/bin/sh

set -eu

case_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
root=$(CDPATH= cd -- "$case_dir/../.." && pwd)
build_arg=${1:-build-accel-vulkan}

case "$build_arg" in
/*)
	build_dir=$build_arg
	;;
*)
	build_dir=$root/$build_arg
	;;
esac

tmp_root=${TMPDIR:-"$build_dir/test-tmp"}

mkdir -p "$tmp_root"
work=$(mktemp -d "$tmp_root/accel-vulkan-plan.XXXXXX")
trap 'rm -rf "$work"' EXIT HUP INT TERM

${CMAKE:-cmake} --build "$build_dir" \
	--target noctcli noct-test-accel-vulkan-plan
"$build_dir/noct-test-accel-vulkan-plan"

if grep -E 'vk[A-Z][A-Za-z0-9_]*[[:space:]]*\(' \
	"$root/src/accel/accel_vulkan.c" >/dev/null; then
	echo 'Vulkan backend contains a raw Vulkan function call.' >&2
	exit 1
fi

VK_ICD_FILENAMES=$work/missing-icd.json \
	"$build_dir/noct" -j0 \
	"$root/tests/testcases/accel-hint-cpu/basic.noct" \
	> "$work/no-gpu.out"
diff -u "$root/tests/testcases/accel-hint-cpu/basic.noct.out" \
	"$work/no-gpu.out"

echo 'Vulkan accelerator integration tests passed.'
