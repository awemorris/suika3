#!/bin/sh

set -eu

ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
NOCT=${NOCT:-$ROOT/build-linux-opengl/noct}
CONVERTER_NOCT=${CONVERTER_NOCT:-$ROOT/build-static/noct}
NOCT_OPENGL_RENDERER_PATTERN=${NOCT_OPENGL_RENDERER_PATTERN:-Intel}
EGL_PLATFORM=${EGL_PLATFORM:-surfaceless}
ONNX_ORACLE_PYTHON=${ONNX_ORACLE_PYTHON:-/tmp/noct-onnx-oracle/bin/python}
MODEL_CACHE=${ONNX_MODEL_CACHE:-${XDG_CACHE_HOME:-$HOME/.cache}/noct-onnx-models}
export EGL_PLATFORM

MODEL_ID=${MODEL_ID:-mnist-12}
case "$MODEL_ID" in
mnist-12)
	STAGE_LABEL=I.1
	MODEL="$MODEL_CACHE/mnist-12.onnx"
	EXPECTED_KERNELS=10
	EXPECTED_WEIGHTS=6
	EXPECTED_ELEMENTS=10
	EXPECTED_SHAPE=1,10
	;;
project-cifar-opset12)
	STAGE_LABEL=I.2
	MODEL="$ROOT/tests/testcases/onnx2noct/fixtures/models/project-cifar-opset12.onnx"
	EXPECTED_KERNELS=11
	EXPECTED_WEIGHTS=10
	EXPECTED_ELEMENTS=10
	EXPECTED_SHAPE=1,10
	;;
squeezenet1.1-7)
	STAGE_LABEL=I.3
	MODEL="$MODEL_CACHE/squeezenet1.1-7.onnx"
	EXPECTED_KERNELS=36
	EXPECTED_WEIGHTS=52
	EXPECTED_ELEMENTS=1000
	EXPECTED_SHAPE=1,1000
	;;
*)
	echo "Unknown supported model gate: $MODEL_ID"
	exit 2
	;;
esac
INPUT="$ROOT/tests/testcases/onnx2noct/oracle-data/$MODEL_ID.input.f32le"
EXPECTED="$ROOT/tests/testcases/onnx2noct/oracle-data/$MODEL_ID.output.f32le"
TMP=$(mktemp -d)
APP_TMP=$(mktemp -d "$ROOT/tests/testcases/.onnx-mnist-opengl.XXXXXX")
trap 'rm -rf "$TMP" "$APP_TMP"' EXIT HUP INT TERM
APP_REL=${APP_TMP#"$ROOT/"}

echo "ONNX Stage-$STAGE_LABEL locked $MODEL_ID OpenGL tests:"
[ -x "$ONNX_ORACLE_PYTHON" ] || {
	echo "Missing pinned ONNX oracle Python: $ONNX_ORACLE_PYTHON"
	exit 1
}
[ -f "$MODEL" ] || {
	echo "Missing locked model: $MODEL"
	exit 1
}
python3 "$ROOT/tests/testcases/onnx2noct/verify-locked-model.py" \
	"$ROOT/tests/testcases/dnn/models.lock" "$MODEL_ID" "$MODEL" >/dev/null
"$ONNX_ORACLE_PYTHON" "$ROOT/tests/testcases/onnx2noct/verify-onnxruntime.py" \
	"$MODEL" "$INPUT" "$EXPECTED" > "$TMP/ort.out"
grep -F "onnxruntime-ok version=1.22.1 elements=$EXPECTED_ELEMENTS" \
	"$TMP/ort.out" >/dev/null

for label in a b; do
	"$CONVERTER_NOCT" -j0 --path="$ROOT/tools/onnx2noct" \
		"$ROOT/tools/onnx2noct/main.noct" \
		--output="$TMP/package-$label" "$MODEL" >/dev/null
done
diff -qr "$TMP/package-a" "$TMP/package-b" >/dev/null
python3 "$ROOT/tests/testcases/onnx2noct/verify-package.py" "$TMP/package-a" \
	"$MODEL" >/dev/null
test "$(jq -r '.output.shape | @csv' "$TMP/package-a/manifest.json")" \
	= "$EXPECTED_SHAPE"
test "$(jq -r '.kernels | length' "$TMP/package-a/manifest.json")" \
	-eq "$EXPECTED_KERNELS"
test "$(jq -r '.weights.entries' "$TMP/package-a/manifest.json")" \
	-eq "$EXPECTED_WEIGHTS"

mkdir "$TMP/relocated"
cp -R "$TMP/package-a/." "$TMP/relocated/"

check_renderer() {
	grep -F 'ACCEL: OpenGL device:' "$TMP/accel.log" >/dev/null
	if grep -E 'ACCEL: OpenGL device:.*(llvmpipe|softpipe|Software Rasterizer)' \
		"$TMP/accel.log" >/dev/null; then
		echo "FAIL locked $MODEL_ID selected a software renderer"
		exit 1
	fi
	if [ -n "$NOCT_OPENGL_RENDERER_PATTERN" ]; then
		grep -E "$NOCT_OPENGL_RENDERER_PATTERN" "$TMP/accel.log" >/dev/null
	fi
	if grep -F 'CPU fallback' "$TMP/accel.log" >/dev/null; then
		echo "FAIL locked $MODEL_ID used CPU fallback"
		exit 1
	fi
	test "$(grep -c 'compiling OpenGL pipeline' "$TMP/accel.log")" \
		-eq "$EXPECTED_KERNELS"
}

for mode in -j0 -j; do
	actual="$TMP/source-${mode#--}.f32le"
	NOCT_ACCEL_DEBUG=1 "$NOCT" --accel=opengl --accel-info "$mode" \
		--path="$TMP/relocated/gpu" "$TMP/relocated/gpu/main.noct" \
		"$TMP/relocated/model.weights" "$INPUT" "$actual" \
		> "$TMP/run.out" 2> "$TMP/accel.log"
	check_renderer
	python3 "$ROOT/tests/testcases/onnx2noct/compare-model.py" "$actual" "$EXPECTED" \
		0.0001 0.00002 argmax
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
		"$INPUT" "$actual" > "$TMP/run.out" 2> "$TMP/accel.log"
	check_renderer
	python3 "$ROOT/tests/testcases/onnx2noct/compare-model.py" "$actual" "$EXPECTED" \
		0.0001 0.00002 argmax
done

warm_begin=$(date +%s%N)
"$NOCT" --accel=opengl -j --path="$TMP/relocated/gpu" \
	"$ROOT/tests/testcases/onnx2noct/model-benchmark.noct" \
	"$TMP/relocated/model.weights" "$INPUT" "$TMP/warm.f32le" 0 >/dev/null
warm_end=$(date +%s%N)
loop_begin=$(date +%s%N)
"$NOCT" --accel=opengl -j --path="$TMP/relocated/gpu" \
	"$ROOT/tests/testcases/onnx2noct/model-benchmark.noct" \
	"$TMP/relocated/model.weights" "$INPUT" "$TMP/steady.f32le" 5 >/dev/null
loop_end=$(date +%s%N)
warm_ns=$((warm_end - warm_begin))
loop_ns=$((loop_end - loop_begin))
steady_ns=$(((loop_ns - warm_ns) / 5))
if [ "$steady_ns" -le 0 ]; then steady_ns=$((loop_ns / 5)); fi
test "$warm_ns" -gt 0
test "$warm_ns" -lt 30000000000
test "$steady_ns" -gt 0
test "$steady_ns" -lt 5000000000
python3 "$ROOT/tests/testcases/onnx2noct/compare-model.py" \
	"$TMP/steady.f32le" "$EXPECTED" 0.0001 0.00002 argmax >/dev/null
echo "timing $MODEL_ID warmup_ns=$warm_ns steady_estimate_ns=$steady_ns"

echo "All ONNX Stage-$STAGE_LABEL locked $MODEL_ID OpenGL tests passed."
