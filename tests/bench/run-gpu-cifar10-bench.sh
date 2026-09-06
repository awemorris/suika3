#!/bin/sh

# Steady-state CPU/JIT versus raw __gpu func benchmark for the CIFAR-10 CNN.

set -eu

bench_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
repo_dir=$(CDPATH= cd -- "$bench_dir/.." && pwd)
BUILD_DIR=${BUILD_DIR:-"$repo_dir/build-linux-opengl"}
CC=${CC:-cc}
TARGET_SECONDS=${TARGET_SECONDS:-30}
WARMUP_SECONDS=${WARMUP_SECONDS:-30}
SAMPLES=${SAMPLES:-3}
SOURCE="$repo_dir/tests/accel/gpu-cifar10-forward.noct"

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
benchmark="$tmp_dir/gpu-cifar10-call-bench"

"$CC" -O2 -std=c11 -Wall -Wextra -Werror \
	-I"$repo_dir/include" \
	"$bench_dir/gpu-cnn-call-bench.c" \
	"$BUILD_DIR/libnoct.a" "$BUILD_DIR/libnoctapi.a" \
	-lm -lutil $(pkg-config --libs egl glesv2) -o "$benchmark"

echo "TARGET_SECONDS=$TARGET_SECONDS WARMUP_SECONDS=$WARMUP_SECONDS SAMPLES=$SAMPLES" >&2
echo "Expected default duration: about 4 minutes plus calibration." >&2
"$benchmark" \
	"$TARGET_SECONDS" "$WARMUP_SECONDS" "$SAMPLES" "$SOURCE" \
	cifar_setup cifar_cpu_forward cifar_verify_cpu \
	cifar_gpu_forward cifar_verify_gpu
