#!/bin/sh

set -eu

ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
NOCT=${NOCT:-$ROOT/build-linux-opengl/noct}
CONVERTER_NOCT=${CONVERTER_NOCT:-$ROOT/build-static/noct}
NOCT_OPENGL_RENDERER_PATTERN=${NOCT_OPENGL_RENDERER_PATTERN:-Intel}
EGL_PLATFORM=${EGL_PLATFORM:-surfaceless}
ONNX_ORACLE_PYTHON=${ONNX_ORACLE_PYTHON:-/tmp/noct-onnx-oracle/bin/python}
export EGL_PLATFORM

TMP=$(mktemp -d)
APP_TMP=$(mktemp -d "$ROOT/tests/testcases/.onnx-stage-h-package.XXXXXX")
trap 'rm -rf "$TMP" "$APP_TMP"' EXIT HUP INT TERM
APP_REL=${APP_TMP#"$ROOT/"}

echo 'ONNX Stage-H complete package OpenGL tests:'
[ -x "$ONNX_ORACLE_PYTHON" ] || {
	echo "Missing pinned ONNX oracle Python: $ONNX_ORACLE_PYTHON"
	exit 1
}
python3 "$ROOT/tests/testcases/onnx2noct/make-stage-g6-fixtures.py" "$TMP/fixtures"

for label in a b; do
	"$CONVERTER_NOCT" -j0 --path="$ROOT/tools/onnx2noct" \
		"$ROOT/tools/onnx2noct/main.noct" \
		--output="$TMP/package-$label" \
		"$TMP/fixtures/batchnorm-reference.onnx" >/dev/null
done
diff -qr "$TMP/package-a" "$TMP/package-b" >/dev/null
python3 "$ROOT/tests/testcases/onnx2noct/verify-package.py" "$TMP/package-a" \
	"$TMP/fixtures/batchnorm-reference.onnx" >/dev/null
cmp "$TMP/package-a/model.weights" \
	"$TMP/fixtures/batchnorm-reference.weights"

"$ONNX_ORACLE_PYTHON" "$ROOT/tests/testcases/onnx2noct/verify-onnxruntime.py" \
	"$TMP/fixtures/batchnorm-reference.onnx" \
	"$TMP/fixtures/batchnorm-reference.input.f32le" \
	"$TMP/fixtures/batchnorm-reference.output.f32le" > "$TMP/ort.out"
grep -F 'onnxruntime-ok version=1.22.1' "$TMP/ort.out" >/dev/null

mkdir "$TMP/relocated"
cp -R "$TMP/package-a/." "$TMP/relocated/"

check_renderer() {
	grep -F 'ACCEL: OpenGL device:' "$TMP/accel.log" >/dev/null
	if grep -E 'ACCEL: OpenGL device:.*(llvmpipe|softpipe|Software Rasterizer)' \
		"$TMP/accel.log" >/dev/null; then
		echo 'FAIL Stage-H package selected a software renderer'
		exit 1
	fi
	if [ -n "$NOCT_OPENGL_RENDERER_PATTERN" ]; then
		grep -E "$NOCT_OPENGL_RENDERER_PATTERN" "$TMP/accel.log" >/dev/null
	fi
	if grep -F 'CPU fallback' "$TMP/accel.log" >/dev/null; then
		echo 'FAIL Stage-H package used CPU fallback'
		exit 1
	fi
	test "$(grep -c 'compiling OpenGL pipeline' "$TMP/accel.log")" -eq 1
}

for mode in -j0 -j; do
	actual="$TMP/source-${mode#--}.f32le"
	NOCT_ACCEL_DEBUG=1 "$NOCT" --accel=opengl --accel-info "$mode" \
		--path="$TMP/relocated/gpu" "$TMP/relocated/gpu/main.noct" \
		"$TMP/relocated/model.weights" \
		"$TMP/fixtures/batchnorm-reference.input.f32le" "$actual" \
		> "$TMP/run.out" 2> "$TMP/accel.log"
	check_renderer
	python3 "$ROOT/tests/testcases/onnx2noct/compare-f32.py" "$actual" \
		"$TMP/fixtures/batchnorm-reference.output.f32le"
done

(
	cd "$ROOT"
	cp "$TMP/package-a/gpu/main.noct" "$APP_TMP/main.noct"
	"$NOCT" --compile --app --path="$TMP/package-a/gpu" \
		"$APP_REL/model.nap" "$APP_REL/main.noct"
)
cp "$APP_TMP/model.nap" "$TMP/relocated/model.nap"
for mode in -j0 -j; do
	actual="$TMP/app-${mode#--}.f32le"
	NOCT_ACCEL_DEBUG=1 "$NOCT" --accel=opengl --accel-info "$mode" \
		"$TMP/relocated/model.nap" "$TMP/relocated/model.weights" \
		"$TMP/fixtures/batchnorm-reference.input.f32le" "$actual" \
		> "$TMP/run.out" 2> "$TMP/accel.log"
	check_renderer
	python3 "$ROOT/tests/testcases/onnx2noct/compare-f32.py" "$actual" \
		"$TMP/fixtures/batchnorm-reference.output.f32le"
done

expect_preflight_failure() {
	name=$1
	message=$2
	weights=$3
	input=$4
	output="$TMP/rejected-$name.f32le"
	if "$NOCT" --accel=opengl -j0 \
		--path="$TMP/relocated/gpu" "$TMP/relocated/gpu/main.noct" \
		"$weights" "$input" "$output" > "$TMP/rejected.log" 2>&1; then
		echo "FAIL Stage-H package accepted $name"
		exit 1
	fi
	grep -F "$message" "$TMP/rejected.log" >/dev/null
	[ ! -e "$output" ]
}

cp "$TMP/relocated/model.weights" "$TMP/corrupt.weights"
printf '\001' | dd of="$TMP/corrupt.weights" bs=1 \
	seek=128 count=1 conv=notrunc status=none
dd if="$TMP/fixtures/batchnorm-reference.input.f32le" \
	of="$TMP/short.input.f32le" bs=1 count=28 status=none
expect_preflight_failure missing 'Cannot open file' \
	"$TMP/missing.weights" \
	"$TMP/fixtures/batchnorm-reference.input.f32le"
expect_preflight_failure corrupt 'NWT1 pack SHA-256 mismatch' \
	"$TMP/corrupt.weights" \
	"$TMP/fixtures/batchnorm-reference.input.f32le"
expect_preflight_failure short-input 'Float32 file is shorter than expected' \
	"$TMP/relocated/model.weights" "$TMP/short.input.f32le"

if "$NOCT" -j0 --path="$TMP/relocated/gpu" \
	"$TMP/relocated/gpu/main.noct" "$TMP/relocated/model.weights" \
	"$TMP/fixtures/batchnorm-reference.input.f32le" \
	"$TMP/rejected-disabled.f32le" > "$TMP/disabled.log" 2>&1; then
	echo 'FAIL Stage-H package ran with acceleration disabled'
	exit 1
fi
grep -F '__gpu func requires the OpenGL backend' "$TMP/disabled.log" >/dev/null
[ ! -e "$TMP/rejected-disabled.f32le" ]

echo 'All ONNX Stage-H complete package OpenGL tests passed.'
