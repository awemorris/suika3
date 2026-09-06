#!/bin/sh

# Compare ordinary func/JIT and raw __gpu func/OpenGL CNN forward time in one VM.
# The default protocol is intentionally long enough to reach a steady thermal
# and clock state: per mode, 30 s untimed warm-up followed by three ~30 s runs.

set -eu

bench_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
repo_dir=$(CDPATH= cd -- "$bench_dir/.." && pwd)
BUILD_DIR=${BUILD_DIR:-"$repo_dir/build-linux-opengl"}
CC=${CC:-cc}
TARGET_SECONDS=${TARGET_SECONDS:-30}
WARMUP_SECONDS=${WARMUP_SECONDS:-30}
SAMPLES=${SAMPLES:-3}
SOURCE="$repo_dir/tests/accel/gpu-cnn-forward.noct"

case "$SAMPLES" in
*[!0-9]*|'') echo "SAMPLES must be a positive odd integer" >&2; exit 2 ;;
esac
if [ "$SAMPLES" -lt 1 ] || [ $((SAMPLES % 2)) -eq 0 ]; then
	echo "SAMPLES must be a positive odd integer" >&2
	exit 2
fi
if [ ! -f "$BUILD_DIR/libnoct.a" ] ||
   [ ! -f "$BUILD_DIR/libnoctapi.a" ]; then
	echo "OpenGL build libraries not found in $BUILD_DIR" >&2
	exit 2
fi

tmp_dir=$(mktemp -d)
trap 'rm -rf -- "$tmp_dir"' EXIT HUP INT TERM
benchmark="$tmp_dir/gpu-cnn-call-bench"

"$CC" -O2 -std=c11 -Wall -Wextra -Werror \
	-I"$repo_dir/include" \
	"$bench_dir/gpu-cnn-call-bench.c" \
	"$BUILD_DIR/libnoct.a" "$BUILD_DIR/libnoctapi.a" \
	-lm -lutil $(pkg-config --libs egl glesv2) -o "$benchmark"

echo "TARGET_SECONDS=$TARGET_SECONDS WARMUP_SECONDS=$WARMUP_SECONDS SAMPLES=$SAMPLES" >&2
echo "Expected default duration: about 4 minutes plus calibration." >&2
EGL_PLATFORM=${EGL_PLATFORM:-surfaceless} "$benchmark" \
	"$TARGET_SECONDS" "$WARMUP_SECONDS" "$SAMPLES" "$SOURCE" \
	cnn_setup cnn_cpu_forward cnn_verify_cpu \
	cnn_gpu_forward cnn_verify_gpu
