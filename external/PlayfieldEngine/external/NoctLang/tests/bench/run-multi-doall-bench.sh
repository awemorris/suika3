#!/bin/sh

# Compare one large CPU/JIT call with equivalent OpenGL ES and Vulkan managed
# __accel calls.  Accelerator results include host input/output transfer.

set -eu

bench_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
repo_dir=$(CDPATH= cd -- "$bench_dir/.." && pwd)
OPENGL_BUILD_DIR=${OPENGL_BUILD_DIR:-"$repo_dir/build-linux-opengl"}
VULKAN_BUILD_DIR=${VULKAN_BUILD_DIR:-"$repo_dir/build-linux-vulkan"}
CC=${CC:-cc}
SAMPLES=${SAMPLES:-5}
SOURCE="$bench_dir/multi-doall.noct"

case "$SAMPLES" in
*[!0-9]*|'') echo "SAMPLES must be a positive odd integer" >&2; exit 2 ;;
esac
if [ "$SAMPLES" -lt 1 ] || [ $((SAMPLES % 2)) -eq 0 ]; then
	echo "SAMPLES must be a positive odd integer" >&2
	exit 2
fi
for build_dir in "$OPENGL_BUILD_DIR" "$VULKAN_BUILD_DIR"; do
	if [ ! -f "$build_dir/libnoct.a" ] ||
	   [ ! -f "$build_dir/libnoctapi.a" ]; then
		echo "accelerator build libraries not found in $build_dir" >&2
		exit 2
	fi
done

tmp_dir=$(mktemp -d)
trap 'rm -rf -- "$tmp_dir"' EXIT HUP INT TERM
opengl_benchmark="$tmp_dir/multi-doall-opengl-bench"
vulkan_benchmark="$tmp_dir/multi-doall-vulkan-bench"
opengl_result="$tmp_dir/opengl.csv"
vulkan_result="$tmp_dir/vulkan.csv"

"$CC" -O2 -std=c11 -Wall -Wextra -Werror \
	-I"$repo_dir/include" \
	"$bench_dir/gpu-cnn-call-bench.c" \
	"$OPENGL_BUILD_DIR/libnoct.a" "$OPENGL_BUILD_DIR/libnoctapi.a" \
	-lm -lutil $(pkg-config --libs egl glesv2) -o "$opengl_benchmark"

"$CC" -O2 -std=c11 -Wall -Wextra -Werror \
	-I"$repo_dir/include" \
	"$bench_dir/gpu-cnn-call-bench.c" \
	"$VULKAN_BUILD_DIR/libnoct.a" "$VULKAN_BUILD_DIR/libnoctapi.a" \
	-lm -lutil $(pkg-config --libs vulkan shaderc) -o "$vulkan_benchmark"

echo "doalls=8 elements=4194304 element_iterations=33554432 variable_division=yes" >&2
echo "SAMPLES=$SAMPLES; every sample is exactly one function/Accel.call" >&2
NOCT_BENCH_SINGLE_INVOCATION=1 \
	EGL_PLATFORM=${EGL_PLATFORM:-surfaceless} \
	"$opengl_benchmark" 1 1 "$SAMPLES" "$SOURCE" \
	md_setup md_cpu_run md_verify_cpu md_gpu_run md_verify_gpu >"$opengl_result"
NOCT_BENCH_SINGLE_INVOCATION=1 NOCT_BENCH_SKIP_CPU=1 \
	NOCT_BENCH_ACCEL_BACKEND=vulkan \
	"$vulkan_benchmark" 1 1 "$SAMPLES" "$SOURCE" \
	md_setup md_cpu_run md_verify_cpu md_gpu_run md_verify_gpu >"$vulkan_result"

echo "mode,calls,samples,best_ms,median_ms,worst_ms,median_ns_per_call,calls_per_second"
awk -F, '$1 == "cpu-jit" || $1 == "gpu-opengl-es"' "$opengl_result"
awk -F, '$1 == "gpu-vulkan"' "$vulkan_result"
echo "comparison,cpu_median_ms,gpu_median_ms,cpu_over_gpu_speedup"
awk -F, '
	$1 == "cpu-jit" { cpu = $5; next }
	$1 == "gpu-opengl-es" || $1 == "gpu-vulkan" {
		printf "cpu-vs-%s,%.6f,%.6f,%.6f\n", $1, cpu, $5, cpu / $5
	}' "$opengl_result" "$vulkan_result"
