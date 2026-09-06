#!/bin/sh

set -eu

ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
NOCT=${NOCT:-$ROOT/build-static/noct}
ONNX_ORACLE_PYTHON=${ONNX_ORACLE_PYTHON:-/tmp/noct-onnx-oracle/bin/python}
MODEL_CACHE=${ONNX_MODEL_CACHE:-${XDG_CACHE_HOME:-$HOME/.cache}/noct-onnx-models}
MODEL="$MODEL_CACHE/tinyyolov2-8.onnx"
INPUT="$ROOT/tests/testcases/onnx2noct/oracle-data/tinyyolov2-8.input.f32le"
EXPECTED="$ROOT/tests/testcases/onnx2noct/oracle-data/tinyyolov2-8.output.f32le"
TMP=$(mktemp -d)
trap 'rm -rf "$TMP"' EXIT HUP INT TERM

echo 'ONNX Stage-I.4 locked Tiny YOLOv2 static-shape blocker tests:'
[ -x "$ONNX_ORACLE_PYTHON" ] || {
	echo "Missing pinned ONNX oracle Python: $ONNX_ORACLE_PYTHON"
	exit 1
}
[ -f "$MODEL" ] || {
	echo "Missing locked Tiny YOLOv2 model: $MODEL"
	exit 1
}
python3 "$ROOT/tests/testcases/onnx2noct/verify-locked-model.py" \
	"$ROOT/tests/testcases/dnn/models.lock" tinyyolov2-8 "$MODEL" >/dev/null
"$ONNX_ORACLE_PYTHON" "$ROOT/tests/testcases/onnx2noct/verify-onnxruntime.py" \
	"$MODEL" "$INPUT" "$EXPECTED" --allow-symbolic-batch-one \
	> "$TMP/ort.out" 2> "$TMP/ort.err"
grep -F 'onnxruntime-ok version=1.22.1 elements=21125' \
	"$TMP/ort.out" >/dev/null

for mode in -j0 -j; do
	output="$TMP/rejected-${mode#--}"
	if "$NOCT" "$mode" --path="$ROOT/tools/onnx2noct" \
		"$ROOT/tools/onnx2noct/main.noct" --output="$output" "$MODEL" \
		> "$TMP/error.out" 2>&1; then
		echo "FAIL locked Tiny YOLOv2 symbolic batch accepted in $mode"
		exit 1
	fi
	grep -F "symbolic dimension 'None'" "$TMP/error.out" >/dev/null
	[ ! -e "$output" ]
done

echo 'All ONNX Stage-I.4 locked Tiny YOLOv2 blocker tests passed.'
