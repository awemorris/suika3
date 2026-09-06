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
noct=$build_dir/noct

mkdir -p "$tmp_root"
work=$(mktemp -d "$tmp_root/gpu-vulkan.XXXXXX")
trap 'rm -rf "$work"' EXIT HUP INT TERM

${CMAKE:-cmake} --build "$build_dir" --target noctcli

gpu_selector=${NOCT_GPU_SELECTOR:-${NOCT_GPU_NAME:-}}
if test -n "$gpu_selector"; then
	gpu_option="--gpu=$gpu_selector"
else
	gpu_option=--gpu
fi

if ! "$noct" --gpu-list > "$work/gpu-list.out" 2> "$work/gpu-list.err"; then
	if grep -E 'No suitable GPU|GPU enumeration failed|no suitable.*device' \
		"$work/gpu-list.out" "$work/gpu-list.err" >/dev/null; then
		echo 'SKIP: no suitable accelerator device is available.'
		exit 0
	fi
	cat "$work/gpu-list.out" "$work/gpu-list.err" >&2
	exit 1
fi
if test -n "$gpu_selector"; then
	grep -F -x "$gpu_selector" "$work/gpu-list.out" >/dev/null
fi

first=$root/tests/testcases/gpu-vulkan/basic-int32.noct
if ! "$noct" -j0 -O1 "$gpu_option" "$first" \
	> "$work/probe.out" 2> "$work/probe.err"; then
	if grep -E 'Vulkan 1\.2|Vulkan device|compute device|Vulkan loader|No suitable GPU|OpenGL ES|Direct3D 12|Metal' \
		"$work/probe.out" "$work/probe.err" >/dev/null; then
		echo 'SKIP: the selected accelerator device is unavailable.'
		exit 0
	fi
	cat "$work/probe.out" "$work/probe.err" >&2
	exit 1
fi
diff -u "$first.out" "$work/probe.out"

for case_name in basic-int32 basic-uint32 basic-float \
		 two-kernel partial-write zero-trip multi-region dosum \
		 dosum-multiple dosum-uint dosum-zero-trip; do
	source=$root/tests/testcases/gpu-vulkan/$case_name.noct
	expected=$source.out

	"$noct" -j0 -O0 "$gpu_option" "$source" > "$work/$case_name-o0.out"
	diff -u "$expected" "$work/$case_name-o0.out"

	"$noct" -j0 -O1 "$gpu_option" "$source" > "$work/$case_name-j0.out"
	diff -u "$expected" "$work/$case_name-j0.out"

	"$noct" -j -O1 "$gpu_option" "$source" > "$work/$case_name-jit.out"
	diff -u "$expected" "$work/$case_name-jit.out"
done

for case_name in cpu-backed device-only; do
	source=$root/tests/testcases/gpu-local/$case_name.noct
	expected=$source.out

	"$noct" -j0 -O0 "$gpu_option" "$source" > "$work/local-$case_name-o0.out"
	diff -u "$expected" "$work/local-$case_name-o0.out"

	"$noct" -j0 -O1 "$gpu_option" "$source" > "$work/local-$case_name-j0.out"
	diff -u "$expected" "$work/local-$case_name-j0.out"

	"$noct" -j -O1 "$gpu_option" "$source" > "$work/local-$case_name-jit.out"
	diff -u "$expected" "$work/local-$case_name-jit.out"
done

if "$noct" -j0 -O1 --gpu=noct-missing:Noct-Missing-GPU "$first" \
	> "$work/missing.out" 2>&1; then
	echo 'Missing accelerator device selector was accepted.' >&2
	exit 1
fi
grep -F "The requested GPU device was not found." \
	"$work/missing.out" >/dev/null

echo 'Accelerator hardware execution tests passed.'
