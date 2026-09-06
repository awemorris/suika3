#!/bin/sh

set -eu

NOCT=${NOCT:-../../build-linux-vulkan/noct}

echo 'Accelerator Vulkan tests:'

for tc in accel/cpu-call.noct accel/vulkan-wide.noct \
	  accel/vulkan-float.noct accel/vulkan-int.noct \
	  accel/vulkan-resource.noct accel/branch.noct \
	  accel/neighbor.noct accel/convert.noct accel/unsafe-index.noct \
	  accel/grid-stride.noct \
	  accel/multi-doall.noct accel/multi-doall-local.noct \
	  accel/dosum.noct accel/doall-dosum.noct \
	  accel/doall-dosum-doall.noct accel/multi-dosum.noct; do
	for mode in '-j0' '-j'; do
		NOCT_VULKAN_VALIDATION=1 "$NOCT" --accel=vulkan --accel-info \
			$mode -O2 "$tc" > out 2> accel.log
		diff "$tc.out" out
		grep -F 'accelerator kernel generated' accel.log >/dev/null
		grep -F 'ACCEL: Vulkan device:' accel.log >/dev/null
		if grep -F 'CPU fallback' accel.log >/dev/null; then
			echo "FAIL $tc ($mode used CPU fallback)"
			exit 1
		fi
	done
	echo "PASS $tc"
done

if NOCT_ACCEL_DEBUG=1 "$NOCT" --disable-accel --accel-info \
	-j0 -O2 accel/cpu-call.noct \
	> out 2> accel.log; then
	echo 'FAIL GPU-only accelerator function ran with acceleration disabled'
	exit 1
fi
grep -F 'has no CPU fallback.' out >/dev/null
awk '/^#version 450/{copy=1} copy && /^ACCEL: kernel/{exit} copy{print}' \
	accel.log > accel.comp
glslc -fshader-stage=compute accel.comp -o accel.spv
spirv-val accel.spv

tmp_dir=$(mktemp -d)
cp accel/cpu-call.noct "$tmp_dir/cpu-call.noct"
"$NOCT" --compile -O2 "$tmp_dir/cpu-call.noct"
NOCT_VULKAN_VALIDATION=1 "$NOCT" --accel=vulkan --accel-info \
	-j "$tmp_dir/cpu-call.nb" > out 2> accel.log
diff accel/cpu-call.noct.out out
grep -F 'compiling Vulkan pipeline' accel.log >/dev/null
if grep -F 'CPU fallback' accel.log >/dev/null; then
	echo 'FAIL accelerator bytecode used CPU fallback'
	exit 1
fi
for tc in multi-doall doall-dosum-doall multi-dosum; do
	cp "accel/$tc.noct" "$tmp_dir/$tc.noct"
	"$NOCT" --compile -O2 "$tmp_dir/$tc.noct"
	NOCT_VULKAN_VALIDATION=1 "$NOCT" --accel=vulkan --accel-info \
		-j "$tmp_dir/$tc.nb" > out 2> accel.log
	diff "accel/$tc.noct.out" out
	grep -F 'compiling Vulkan pipeline' accel.log >/dev/null
	if grep -F 'CPU fallback' accel.log >/dev/null; then
		echo "FAIL $tc Vulkan bytecode used CPU fallback"
		exit 1
	fi
done
rm -rf "$tmp_dir"

app_path="accel/.tmp-vulkan-resource-$$.nap"
"$NOCT" --compile --app -O2 "$app_path" \
	accel/vulkan-resource.noct
NOCT_VULKAN_VALIDATION=1 "$NOCT" --accel=vulkan --accel-info \
	-j "$app_path" > out 2> accel.log
diff accel/vulkan-resource.noct.out out
grep -F 'compiling Vulkan pipeline' accel.log >/dev/null
if grep -F 'CPU fallback' accel.log >/dev/null; then
	echo 'FAIL accelerator .nap resource app used CPU fallback'
	exit 1
fi
rm -f "$app_path"

rm -f out accel.log accel.comp accel.spv
echo 'All accelerator Vulkan tests passed.'
