#!/bin/sh

set -eu

: "${NOCT:?Set NOCT to the DirectX 12-enabled Windows noct executable.}"

check_dx12()
{
	grep -F 'ACCEL: DirectX 12 device:' accel.log >/dev/null
	if grep -F 'CPU fallback' accel.log >/dev/null; then
		echo "FAIL $1 used CPU fallback"
		exit 1
	fi
}

echo 'Accelerator DirectX 12 tests:'

# The portable --gpu policy option must select the linked DX12 runtime.  Keep
# this separate from the explicit --accel=dx12 cases so CLI/backend wiring
# regressions cannot be hidden by the backend-specific option.
NOCT_DX12_DEBUG=${NOCT_DX12_DEBUG:-1} \
	"$NOCT" --gpu --accel-info -j0 -O2 accel/multi-dosum.noct \
	> out 2> accel.log
diff accel/multi-dosum.noct.out out
grep -F 'accelerator kernel generated' accel.log >/dev/null
check_dx12 'multi-dosum (--gpu)'
echo 'PASS multi-dosum (--gpu)'

for tc in accel/cpu-call.noct accel/vulkan-wide.noct \
	  accel/vulkan-float.noct accel/vulkan-int.noct \
	  accel/vulkan-resource.noct accel/branch.noct \
	  accel/neighbor.noct accel/convert.noct accel/unsafe-index.noct \
	  accel/grid-stride.noct accel/multi-doall.noct \
	  accel/multi-doall-local.noct accel/dosum.noct \
	  accel/doall-dosum.noct accel/doall-dosum-doall.noct \
	  accel/multi-dosum.noct; do
	for mode in '-j0' '-j'; do
		NOCT_DX12_DEBUG=${NOCT_DX12_DEBUG:-1} \
			"$NOCT" --accel=dx12 --accel-info \
			$mode -O2 "$tc" > out 2> accel.log
		diff "$tc.out" out
		grep -F 'accelerator kernel generated' accel.log >/dev/null
		check_dx12 "$tc ($mode)"
	done
	echo "PASS $tc"
done

for tc in accel/gpu-dispatch.noct accel/gpu-launch.noct \
	  accel/gpu-shared.noct accel/gpu-loop-accumulator.noct \
	  accel/gpu-math-mnist.noct accel/gpu-float32-bits.noct \
	  accel/resource-copy.noct accel/async-copy-zero.noct; do
	for mode in '-j0' '-j'; do
		"$NOCT" --accel=dx12 --accel-info $mode "$tc" \
			> out 2> accel.log
		diff "$tc.out" out
		check_dx12 "$tc ($mode)"
	done
	echo "PASS $tc"
done

tmp_dir=$(mktemp -d)
trap 'rm -rf "$tmp_dir"; rm -f out accel.log' EXIT HUP INT TERM
for tc in cpu-call multi-doall doall-dosum-doall multi-dosum gpu-shared; do
	cp "accel/$tc.noct" "$tmp_dir/$tc.noct"
	"$NOCT" --compile -O2 "$tmp_dir/$tc.noct"
	grep -a -F 'HLSL Size' "$tmp_dir/$tc.nb" >/dev/null
	"$NOCT" --accel=dx12 --accel-info -j "$tmp_dir/$tc.nb" \
		> out 2> accel.log
	diff "accel/$tc.noct.out" out
	check_dx12 "$tc bytecode"
done

app_path="$tmp_dir/dx12-app.nap"
"$NOCT" --compile --app -O2 "$app_path" accel/cpu-call.noct
"$NOCT" --accel=dx12 --accel-info -j "$app_path" > out 2> accel.log
diff accel/cpu-call.noct.out out
check_dx12 'cpu-call app'

rm -rf "$tmp_dir"
rm -f out accel.log
trap - EXIT HUP INT TERM
echo 'All accelerator DirectX 12 tests passed.'
