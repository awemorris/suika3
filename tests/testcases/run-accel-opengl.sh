#!/bin/sh

set -eu

NOCT=${NOCT:-../build-opengl/noct}
NOCT_OPENGL_RENDERER_PATTERN=${NOCT_OPENGL_RENDERER_PATTERN:-}

check_hardware_renderer()
{
	grep -F 'ACCEL: OpenGL device:' accel.log >/dev/null
	if grep -E 'ACCEL: OpenGL device:.*(llvmpipe|softpipe|Software Rasterizer)' \
		accel.log >/dev/null; then
		echo 'FAIL OpenGL accelerator selected a software renderer'
		exit 1
	fi
	if [ -n "$NOCT_OPENGL_RENDERER_PATTERN" ]; then
		grep -E "$NOCT_OPENGL_RENDERER_PATTERN" accel.log >/dev/null
	fi
}

echo 'Accelerator OpenGL tests:'

for tc in accel/cpu-call.noct accel/vulkan-wide.noct \
	  accel/vulkan-float.noct accel/vulkan-int.noct \
	  accel/vulkan-resource.noct \
	  accel/branch.noct accel/neighbor.noct accel/convert.noct \
	  accel/unsafe-index.noct accel/grid-stride.noct accel/multi-doall.noct \
	  accel/multi-doall-local.noct accel/dosum.noct \
	  accel/doall-dosum.noct accel/doall-dosum-doall.noct \
	  accel/multi-dosum.noct; do
	for mode in '-j0' '-j'; do
		MESA_DEBUG=context \
			"$NOCT" --accel=opengl --accel-info \
			$mode -O2 "$tc" > out 2> accel.log
		diff "$tc.out" out
		grep -F 'accelerator kernel generated' accel.log >/dev/null
		check_hardware_renderer
		if grep -F 'CPU fallback' accel.log >/dev/null; then
			echo "FAIL $tc ($mode used CPU fallback)"
			exit 1
		fi
	done
	echo "PASS $tc"
done

if "$NOCT" --accel=opengl --accel-info \
	-j0 -O2 accel/non-doall-ptr.noct \
	> out 2> accel.log; then
	echo 'FAIL dependent accel loop compiled under GPU-only semantics'
	exit 1
fi
grep -F 'class=dependent class-reason=memory-raw' accel.log >/dev/null
grep -F "GPU-only __accel func 'prefix' cannot be lowered" out >/dev/null

for tc in accel/gpu-dispatch.noct accel/gpu-launch.noct \
	  accel/gpu-shared.noct accel/gpu-cnn-forward.noct \
	  accel/gpu-cifar10-forward.noct accel/gpu-loop-accumulator.noct \
	  accel/gpu-math-mnist.noct accel/gpu-float32-bits.noct; do
	for mode in '-j0' '-j'; do
		"$NOCT" --accel=opengl $mode "$tc" \
			> out 2> accel.log
		diff "$tc.out" out
	done
	echo "PASS $tc"
done

for level in 0 1 2; do
	for tc in accel/gpu-loop-accumulator.noct accel/gpu-math-mnist.noct \
		  accel/gpu-float32-bits.noct; do
		"$NOCT" --accel=opengl -j0 -O"$level" \
			"$tc" > out 2> accel.log
		diff "$tc.out" out
	done
done

for tc in accel/resource-copy.noct accel/async-copy-zero.noct; do
	for mode in '-j0' '-j'; do
		"$NOCT" --accel=opengl $mode "$tc" \
			> out 2> accel.log
		diff "$tc.out" out
	done
	echo "PASS $tc"
done

# VM teardown completes an unjoined download and releases its pinned Packed.
NOCT_ACCEL_DEBUG=1 \
	"$NOCT" --accel=opengl -j0 \
	accel/async-copy-no-join.noct > out 2> accel.log
diff accel/async-copy-no-join.noct.out out
grep -F 'OpenGL asynchronous download queued' accel.log >/dev/null
grep -F 'OpenGL asynchronous download completed' accel.log >/dev/null

"$NOCT" --accel=opengl --accel-info \
	-j0 -O2 accel/resource-persistent.noct \
	> out 2> accel.log
diff accel/resource-persistent.noct.out out
test "$(grep -c 'OpenGL persistent resource allocated' accel.log)" -eq 2

"$NOCT" --accel=opengl --accel-info \
	-j0 accel/typed-resources.noct > out 2> accel.log
diff accel/typed-resources.noct.out out
test "$(grep -c 'OpenGL persistent resource allocated' accel.log)" -eq 10

# A software OpenGL context is not accepted as a GPU accelerator.
env -u GALLIUM_DRIVER LIBGL_ALWAYS_SOFTWARE=1 \
	"$NOCT" --accel=opengl --accel-info \
	-j0 -O2 accel/cpu-call.noct \
	> out 2> accel.log && {
	echo 'FAIL software renderer triggered a CPU fallback'
	exit 1
}
grep -F 'OpenGL device rejected: llvmpipe' accel.log >/dev/null
grep -F 'backend is unavailable or rejected the GPU program.' out >/dev/null

# Accelerator metadata and OpenGL execution survive a standalone .nb.
tmp_dir=$(mktemp -d)
cp accel/cpu-call.noct "$tmp_dir/cpu-call.noct"
"$NOCT" --compile -O2 "$tmp_dir/cpu-call.noct"
"$NOCT" --accel=opengl --accel-info \
	-j "$tmp_dir/cpu-call.nb" > out 2> accel.log
diff accel/cpu-call.noct.out out
grep -F 'compiling OpenGL pipeline' accel.log >/dev/null
if grep -F 'CPU fallback' accel.log >/dev/null; then
	echo 'FAIL accelerator bytecode used CPU fallback'
	exit 1
fi

# Required-range metadata and raw gpu descriptors also survive a .nb.
cp accel/unsafe-index.noct "$tmp_dir/unsafe-index.noct"
"$NOCT" --compile -O2 "$tmp_dir/unsafe-index.noct"
"$NOCT" --accel=opengl -j \
	"$tmp_dir/unsafe-index.nb" > out 2> accel.log
diff accel/unsafe-index.noct.out out
if grep -F 'CPU fallback' accel.log >/dev/null; then
	echo 'FAIL accelerator range metadata bytecode used CPU fallback'
	exit 1
fi

cp accel/gpu-dispatch.noct "$tmp_dir/gpu-dispatch.noct"
"$NOCT" --compile "$tmp_dir/gpu-dispatch.noct"
"$NOCT" --accel=opengl -j \
	"$tmp_dir/gpu-dispatch.nb" > out 2> accel.log
diff accel/gpu-dispatch.noct.out out

cp accel/gpu-cnn-forward.noct "$tmp_dir/gpu-cnn-forward.noct"
"$NOCT" --compile "$tmp_dir/gpu-cnn-forward.noct"
"$NOCT" --accel=opengl -j \
	"$tmp_dir/gpu-cnn-forward.nb" > out 2> accel.log
diff accel/gpu-cnn-forward.noct.out out

cp accel/gpu-cifar10-forward.noct "$tmp_dir/gpu-cifar10-forward.noct"
"$NOCT" --compile "$tmp_dir/gpu-cifar10-forward.noct"
"$NOCT" --accel=opengl -j \
	"$tmp_dir/gpu-cifar10-forward.nb" > out 2> accel.log
diff accel/gpu-cifar10-forward.noct.out out

for tc in gpu-loop-accumulator gpu-math-mnist gpu-float32-bits; do
	cp "accel/$tc.noct" "$tmp_dir/$tc.noct"
	"$NOCT" --compile -O2 "$tmp_dir/$tc.noct"
	"$NOCT" --accel=opengl -j "$tmp_dir/$tc.nb" \
		> out 2> accel.log
	diff "accel/$tc.noct.out" out
done

for tc in dosum doall-dosum doall-dosum-doall multi-dosum; do
	cp "accel/$tc.noct" "$tmp_dir/$tc.noct"
	"$NOCT" --compile -O0 "$tmp_dir/$tc.noct"
	"$NOCT" --accel=opengl --accel-info -j \
		"$tmp_dir/$tc.nb" > out 2> accel.log
	diff "accel/$tc.noct.out" out
	grep -F 'compiling OpenGL pipeline' accel.log >/dev/null
	if grep -F 'CPU fallback' accel.log >/dev/null; then
		echo "FAIL $tc accelerator bytecode used CPU fallback"
		exit 1
	fi
done
rm -rf "$tmp_dir"

# Typed resources and the aggregate initializer survive a .nap.
app_path="accel/.tmp-opengl-resource-$$.nap"
"$NOCT" --compile --app -O2 "$app_path" \
	accel/vulkan-resource.noct
"$NOCT" --accel=opengl --accel-info \
	-j "$app_path" > out 2> accel.log
diff accel/vulkan-resource.noct.out out
grep -F 'compiling OpenGL pipeline' accel.log >/dev/null
if grep -F 'CPU fallback' accel.log >/dev/null; then
	echo 'FAIL accelerator OpenGL .nap used CPU fallback'
	exit 1
fi
rm -f "$app_path"

for tc in gpu-loop-accumulator gpu-math-mnist gpu-float32-bits; do
	app_path="accel/.tmp-$tc-$$.nap"
	"$NOCT" --compile --app -O2 "$app_path" \
		"accel/$tc.noct"
	"$NOCT" --accel=opengl -j "$app_path" \
		> out 2> accel.log
	diff "accel/$tc.noct.out" out
	rm -f "$app_path"
done

rm -f out accel.log
echo 'All accelerator OpenGL tests passed.'
