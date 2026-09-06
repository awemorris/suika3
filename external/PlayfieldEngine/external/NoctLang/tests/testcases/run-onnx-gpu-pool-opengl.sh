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
APP_TMP=$(mktemp -d "$ROOT/tests/testcases/.onnx-stage-g3-opengl.XXXXXX")
trap 'rm -rf "$TMP" "$APP_TMP"' EXIT HUP INT TERM
APP_REL=${APP_TMP#"$ROOT/"}

echo 'ONNX Stage-G3 generated pooling OpenGL tests:'
[ -x "$ONNX_ORACLE_PYTHON" ] || {
	echo "Missing pinned ONNX oracle Python: $ONNX_ORACLE_PYTHON"
	exit 1
}
python3 "$ROOT/tests/testcases/onnx2noct/make-stage-g3-fixtures.py" "$TMP/fixtures"

check_renderer() {
	grep -F 'ACCEL: OpenGL device:' "$TMP/accel.log" >/dev/null
	if grep -E 'ACCEL: OpenGL device:.*(llvmpipe|softpipe|Software Rasterizer)' \
		"$TMP/accel.log" >/dev/null; then
		echo 'FAIL Stage-G3 pooling selected a software renderer'
		exit 1
	fi
	if [ -n "$NOCT_OPENGL_RENDERER_PATTERN" ]; then
		grep -E "$NOCT_OPENGL_RENDERER_PATTERN" "$TMP/accel.log" >/dev/null
	fi
	if grep -F 'CPU fallback' "$TMP/accel.log" >/dev/null; then
		echo 'FAIL Stage-G3 generated pooling used CPU fallback'
		exit 1
	fi
}

for name in maxpool-padding averagepool-count global-averagepool; do
	expected_pipelines=1
	if [ "$name" = averagepool-count ]; then expected_pipelines=3; fi
	"$ONNX_ORACLE_PYTHON" "$ROOT/tests/testcases/onnx2noct/verify-onnxruntime.py" \
		"$TMP/fixtures/$name.onnx" "$TMP/fixtures/$name.input.f32le" \
		"$TMP/fixtures/$name.output.f32le" > "$TMP/ort.out"
	grep -F 'onnxruntime-ok version=1.22.1' "$TMP/ort.out" >/dev/null
	generated="$TMP/generated-$name"
	"$CONVERTER_NOCT" -j0 --path="$ROOT/tools/onnx2noct" \
		"$ROOT/tools/onnx2noct/main.noct" --output="$generated" \
		"$TMP/fixtures/$name.onnx" >/dev/null

	for mode in -j0 -j; do
		actual="$TMP/$name-source-${mode#--}.f32le"
		NOCT_ACCEL_DEBUG=1 "$NOCT" --accel=opengl --accel-info "$mode" \
			--path="$generated/gpu" "$ROOT/tests/testcases/onnx2noct/stage-f-run.noct" \
			"$generated/model.weights" \
			"$TMP/fixtures/$name.input.f32le" "$actual" \
			> "$TMP/run.out" 2> "$TMP/accel.log"
		check_renderer
		test "$(grep -c 'compiling OpenGL pipeline' "$TMP/accel.log")" \
			-eq "$expected_pipelines"
		python3 "$ROOT/tests/testcases/onnx2noct/compare-f32.py" "$actual" \
			"$TMP/fixtures/$name.output.f32le"
	done

	(
		cd "$ROOT"
		"$NOCT" --compile --app --path="$generated/gpu" \
			"$APP_REL/$name.nap" tests/testcases/onnx2noct/stage-f-run.noct
	)
	for mode in -j0 -j; do
		actual="$TMP/$name-app-${mode#--}.f32le"
		"$NOCT" --accel=opengl --accel-info "$mode" \
			"$APP_TMP/$name.nap" "$generated/model.weights" \
			"$TMP/fixtures/$name.input.f32le" \
			"$actual" \
			> "$TMP/run.out" 2> "$TMP/accel.log"
		check_renderer
		test "$(grep -c 'compiling OpenGL pipeline' "$TMP/accel.log")" \
			-eq "$expected_pipelines"
		python3 "$ROOT/tests/testcases/onnx2noct/compare-f32.py" "$actual" \
			"$TMP/fixtures/$name.output.f32le"
	done
	echo "PASS $name source/.nap interpreter/JIT"
done

if "$NOCT" -j0 --path="$TMP/generated-maxpool-padding/gpu" \
	"$ROOT/tests/testcases/onnx2noct/stage-f-run.noct" \
	"$TMP/generated-maxpool-padding/model.weights" \
	"$TMP/fixtures/maxpool-padding.input.f32le" "$TMP/disabled-output.f32le" \
	> "$TMP/disabled.log" 2>&1; then
	echo 'FAIL Stage-G3 pooling ran with the accelerator disabled'
	exit 1
fi
grep -F '__gpu func requires the OpenGL backend' "$TMP/disabled.log" >/dev/null
[ ! -e "$TMP/disabled-output.f32le" ]

echo 'All ONNX Stage-G3 generated pooling OpenGL tests passed.'
