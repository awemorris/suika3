#!/bin/sh

set -eu

NOCT=${NOCT:-../../build-static/noct}
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
NOCT=$(CDPATH= cd -- "$(dirname "$NOCT")" && pwd)/$(basename "$NOCT")
TMP=$(mktemp -d)
APP_TMP=$(mktemp -d "$ROOT/tests/testcases/.onnx-stage-d.XXXXXX")
trap 'rm -rf "$TMP" "$APP_TMP"' EXIT HUP INT TERM

echo 'ONNX Stage-D structural reader tests:'
python3 "$ROOT/tests/testcases/onnx2noct/make-reader-fixtures.py" "$TMP/generated"
python3 "$ROOT/tests/testcases/onnx2noct/make-reader-fixtures.py" "$TMP/generated-2"
diff -qr "$TMP/generated" "$TMP/generated-2" >/dev/null
python3 "$ROOT/tests/testcases/onnx2noct/make-normalize-fixtures.py" "$TMP/normalized"
python3 "$ROOT/tests/testcases/onnx2noct/make-normalize-fixtures.py" "$TMP/normalized-2"
diff -qr "$TMP/normalized" "$TMP/normalized-2" >/dev/null
python3 "$ROOT/tests/testcases/onnx2noct/make-stage-f-fixtures.py" "$TMP/stage-f"
python3 "$ROOT/tests/testcases/onnx2noct/make-stage-f-fixtures.py" "$TMP/stage-f-2"
diff -qr "$TMP/stage-f" "$TMP/stage-f-2" >/dev/null
python3 "$ROOT/tests/testcases/onnx2noct/make-stage-g1-fixtures.py" "$TMP/stage-g1"
python3 "$ROOT/tests/testcases/onnx2noct/make-stage-g1-fixtures.py" "$TMP/stage-g1-2"
diff -qr "$TMP/stage-g1" "$TMP/stage-g1-2" >/dev/null
python3 "$ROOT/tests/testcases/onnx2noct/make-stage-g2-fixtures.py" "$TMP/stage-g2"
python3 "$ROOT/tests/testcases/onnx2noct/make-stage-g2-fixtures.py" "$TMP/stage-g2-2"
diff -qr "$TMP/stage-g2" "$TMP/stage-g2-2" >/dev/null
python3 "$ROOT/tests/testcases/onnx2noct/make-stage-g3-fixtures.py" "$TMP/stage-g3"
python3 "$ROOT/tests/testcases/onnx2noct/make-stage-g3-fixtures.py" "$TMP/stage-g3-2"
diff -qr "$TMP/stage-g3" "$TMP/stage-g3-2" >/dev/null
python3 "$ROOT/tests/testcases/onnx2noct/make-stage-g4-fixtures.py" "$TMP/stage-g4"
python3 "$ROOT/tests/testcases/onnx2noct/make-stage-g4-fixtures.py" "$TMP/stage-g4-2"
diff -qr "$TMP/stage-g4" "$TMP/stage-g4-2" >/dev/null
python3 "$ROOT/tests/testcases/onnx2noct/make-stage-g5-fixtures.py" "$TMP/stage-g5"
python3 "$ROOT/tests/testcases/onnx2noct/make-stage-g5-fixtures.py" "$TMP/stage-g5-2"
diff -qr "$TMP/stage-g5" "$TMP/stage-g5-2" >/dev/null
python3 "$ROOT/tests/testcases/onnx2noct/make-stage-g6-fixtures.py" "$TMP/stage-g6"
python3 "$ROOT/tests/testcases/onnx2noct/make-stage-g6-fixtures.py" "$TMP/stage-g6-2"
diff -qr "$TMP/stage-g6" "$TMP/stage-g6-2" >/dev/null

run_reader() {
	mode=$1
	path=$2
	"$NOCT" "$mode" --path="$ROOT/tools/onnx2noct" \
		"$ROOT/tests/testcases/onnx2noct/reader.noct" "$path"
}

check_valid() {
	path=$1
	expected=$2
	for mode in -j0 -j; do
		run_reader "$mode" "$path" > "$TMP/out"
		printf '%s\n' "$expected" > "$TMP/expected"
		diff "$TMP/expected" "$TMP/out"
	done
}

FIX="$ROOT/tests/testcases/onnx2noct/fixtures/models"
check_valid "$FIX/identity-opset12.onnx" \
	'ir=7 opset=12 nodes=1 initializers=0 input=x output=y'
check_valid "$FIX/add-broadcast-opset12.onnx" \
	'ir=7 opset=12 nodes=1 initializers=1 input=x output=y'
check_valid "$FIX/conv2d-opset12.onnx" \
	'ir=7 opset=12 nodes=1 initializers=2 input=x output=y'
check_valid "$FIX/maxpool-opset12.onnx" \
	'ir=7 opset=12 nodes=1 initializers=0 input=x output=y'
check_valid "$FIX/reduce-sum-opset12.onnx" \
	'ir=7 opset=12 nodes=1 initializers=0 input=x output=y'
check_valid "$FIX/reshape-opset12.onnx" \
	'ir=7 opset=12 nodes=1 initializers=1 input=x output=y'
check_valid "$FIX/sigmoid-opset12.onnx" \
	'ir=7 opset=12 nodes=1 initializers=0 input=x output=y'
check_valid "$FIX/project-cifar-opset12.onnx" \
	'ir=7 opset=12 nodes=12 initializers=10 input=input output=logits'
check_valid "$TMP/generated/valid-unpacked.onnx" \
	'ir=7 opset=12 nodes=1 initializers=0 input=x output=y'
check_valid "$TMP/generated/valid-packed.onnx" \
	'ir=7 opset=12 nodes=1 initializers=1 input=x output=y'
check_valid "$TMP/generated/valid-unknown-fields.onnx" \
	'ir=7 opset=12 nodes=1 initializers=0 input=x output=y'
check_valid "$TMP/generated/valid-type-metadata.onnx" \
	'ir=7 opset=12 nodes=1 initializers=0 input=x output=y'
check_valid "$TMP/generated/valid-ai-onnx-domain.onnx" \
	'ir=7 opset=12 nodes=1 initializers=0 input=x output=y'
check_valid "$TMP/generated/valid-old-ir-initializer-input.onnx" \
	'ir=7 opset=12 nodes=1 initializers=1 input=x output=y'
check_valid "$TMP/generated/valid-optional-empty-input.onnx" \
	'ir=7 opset=12 nodes=1 initializers=0 input=x output=y'
check_valid "$TMP/generated/valid-float-bits.onnx" \
	'ir=7 opset=12 nodes=1 initializers=1 input=x output=y'

"$NOCT" -j0 --path="$ROOT/tools/onnx2noct" \
	"$ROOT/tests/testcases/onnx2noct/float-bits.noct" \
	"$TMP/generated/valid-float-bits.onnx" > "$TMP/bits.out"
printf '%s\n' 1 2147483648 2143289345 4294967295 > "$TMP/bits.expected"
diff "$TMP/bits.expected" "$TMP/bits.out"

check_error() {
	path=$1
	message=$2
	if run_reader -j0 "$path" > "$TMP/error.out" 2>&1; then
		echo "FAIL malformed ONNX accepted: $path"
		exit 1
	fi
	grep -F -- "$message" "$TMP/error.out" >/dev/null || {
		echo "FAIL unexpected diagnostic: $path"
		cat "$TMP/error.out"
		exit 1
	}
}

check_error "$FIX/error-custom-domain-opset12.onnx" 'custom opset domain'
check_error "$FIX/error-dynamic-shape-opset12.onnx" 'symbolic dimension'
check_error "$FIX/error-external-data-opset12.onnx" 'unsupported external data'
check_error "$FIX/error-truncated-protobuf.onnx" 'truncated ModelProto.opset_import'

truncate -s 536870913 "$TMP/too-large.onnx"
check_error "$TMP/too-large.onnx" 'exceeds the 512 MiB limit'

MODEL_CACHE=${ONNX_MODEL_CACHE:-${XDG_CACHE_HOME:-$HOME/.cache}/noct-onnx-models}
if [ -f "$MODEL_CACHE/mnist-12.onnx" ]; then
	check_valid "$MODEL_CACHE/mnist-12.onnx" \
		'ir=7 opset=12 nodes=12 initializers=8 input=Input3 output=Plus214_Output_0'
fi
if [ -f "$MODEL_CACHE/squeezenet1.1-7.onnx" ]; then
	check_valid "$MODEL_CACHE/squeezenet1.1-7.onnx" \
		'ir=3 opset=7 nodes=66 initializers=53 input=data output=squeezenet0_flatten0_reshape0'
fi
if [ -f "$MODEL_CACHE/tinyyolov2-8.onnx" ]; then
	check_error "$MODEL_CACHE/tinyyolov2-8.onnx" "symbolic dimension 'None'"
fi

tab=$(printf '\t')
while IFS="$tab" read -r name message; do
	check_error "$TMP/generated/$name" "$message"
done < "$TMP/generated/errors.tsv"

"$NOCT" -j0 --path="$ROOT/tools/onnx2noct" \
	"$ROOT/tests/testcases/onnx2noct/protobuf-depth.noct" \
	"$TMP/generated/depth-32.pb"

if "$NOCT" -j0 --path="$ROOT/tools/onnx2noct" \
	"$ROOT/tests/testcases/onnx2noct/protobuf-depth.noct" \
	"$TMP/generated/depth-33.pb" > "$TMP/error.out" 2>&1; then
	echo 'FAIL protobuf depth 33 accepted'
	exit 1
fi
grep -F 'message nesting exceeds 32' "$TMP/error.out" >/dev/null

size=$(wc -c < "$TMP/generated/valid-unpacked.onnx")
i=0
while [ "$i" -lt "$size" ]; do
	dd if="$TMP/generated/valid-unpacked.onnx" \
		of="$TMP/truncated.onnx" bs=1 count="$i" status=none
	if run_reader -j0 "$TMP/truncated.onnx" \
		> "$TMP/error.out" 2>&1; then
		echo "FAIL truncation at byte $i accepted"
		exit 1
	fi
	i=$((i + 1))
done

run_normalize() {
	mode=$1
	path=$2
	"$NOCT" "$mode" --path="$ROOT/tools/onnx2noct" \
		"$ROOT/tests/testcases/onnx2noct/normalize.noct" "$path"
}

check_normalized() {
	path=$1
	golden=$2
	for mode in -j0 -j; do
		run_normalize "$mode" "$path" > "$TMP/normalized.out"
		diff "$ROOT/tests/testcases/onnx2noct/golden/stage-e/$golden.norm" \
			"$TMP/normalized.out"
	done
}

check_normalized "$FIX/identity-opset12.onnx" identity-opset12
check_normalized "$FIX/add-broadcast-opset12.onnx" add-broadcast-opset12
check_normalized "$FIX/sigmoid-opset12.onnx" sigmoid-opset12
check_normalized "$FIX/reduce-sum-opset12.onnx" reduce-sum-opset12
check_normalized "$FIX/conv2d-opset12.onnx" conv2d-opset12
check_normalized "$FIX/maxpool-opset12.onnx" maxpool-opset12
check_normalized "$FIX/reshape-opset12.onnx" reshape-opset12
check_normalized "$FIX/project-cifar-opset12.onnx" project-cifar-opset12
check_normalized "$TMP/normalized/valid-average-pool.onnx" valid-average-pool
check_normalized "$TMP/normalized/valid-maxpool-opset7.onnx" valid-maxpool-opset7
check_normalized "$TMP/normalized/valid-maxpool-opset8.onnx" valid-maxpool-opset8
check_normalized "$TMP/normalized/valid-fold-chain.onnx" valid-fold-chain
check_normalized "$TMP/normalized/valid-pointwise-family.onnx" valid-pointwise-family
check_normalized "$TMP/normalized/valid-reduction-family.onnx" valid-reduction-family
check_normalized "$TMP/normalized/valid-squeeze-unsqueeze.onnx" valid-squeeze-unsqueeze
check_normalized "$TMP/normalized/valid-transpose-name.onnx" valid-transpose-name

check_normalize_error() {
	path=$1
	message=$2
	if run_normalize -j0 "$path" > "$TMP/error.out" 2>&1; then
		echo "FAIL invalid normalized graph accepted: $path"
		exit 1
	fi
	grep -F "$message" "$TMP/error.out" >/dev/null || {
		echo "FAIL unexpected normalize diagnostic: $path"
		cat "$TMP/error.out"
		exit 1
	}
}

while IFS="$tab" read -r name message; do
	check_normalize_error "$TMP/normalized/$name" "$message"
done < "$TMP/normalized/errors.tsv"

check_real_normalized() {
	path=$1
	header=$2
	run_normalize -j0 "$path" > "$TMP/real-normalized-a.out"
	run_normalize -j "$path" > "$TMP/real-normalized-b.out"
	diff "$TMP/real-normalized-a.out" "$TMP/real-normalized-b.out"
	head -n 1 "$TMP/real-normalized-a.out" | grep -F "$header" >/dev/null
}

if [ -f "$MODEL_CACHE/mnist-12.onnx" ]; then
	check_real_normalized "$MODEL_CACHE/mnist-12.onnx" \
		'graph opset=12 tensors=21 nodes=12 input=t0 output=t20'
fi
if [ -f "$MODEL_CACHE/squeezenet1.1-7.onnx" ]; then
	check_real_normalized "$MODEL_CACHE/squeezenet1.1-7.onnx" \
		'graph opset=7 tensors=120 nodes=66 input=t0 output=t119'
fi

run_stage_f_converter() {
	mode=$1
	output=$2
	model=$3
	shift 3
	"$NOCT" "$mode" --path="$ROOT/tools/onnx2noct" \
		"$ROOT/tools/onnx2noct/main.noct" --output="$output" "$@" "$model"
}

check_stage_f_source() {
	name=$1
	kernels=$2
	storages=$3
	for label in interp jit; do
		mode=-j0
		if [ "$label" = jit ]; then mode=-j; fi
		output="$TMP/stage-f-$name-$label"
		run_stage_f_converter "$mode" "$output" \
			"$TMP/stage-f/$name.onnx" > "$TMP/stage-f-summary.out"
		printf 'Generated Stage-F GPU model: kernels=%s storages=%s\n' \
			"$kernels" "$storages" > "$TMP/stage-f-summary.expected"
		diff "$TMP/stage-f-summary.expected" "$TMP/stage-f-summary.out"
		diff "$ROOT/tests/testcases/onnx2noct/golden/stage-f/$name.model.noct" \
			"$output/gpu/model.noct"
		if grep -E '__accel func|DNN\.|DPL1|#version|layout\(' \
			"$output/gpu/model.noct" >/dev/null; then
			echo "FAIL forbidden generated source construct: $name"
			exit 1
		fi
		if grep -F '/home/' "$output/gpu/model.noct" >/dev/null; then
			echo "FAIL absolute path in generated source: $name"
			exit 1
		fi
	done
	diff "$TMP/stage-f-$name-interp/gpu/model.noct" \
		"$TMP/stage-f-$name-jit/gpu/model.noct"
}

check_stage_f_source unary 2 4
check_stage_f_source broadcast-alias 1 2
check_stage_f_source transpose-copy 1 2
test "$(grep -c '^__gpu func ' "$TMP/stage-f-unary-interp/gpu/model.noct")" -eq 2
test "$(grep -c 'k_pointwise_unary_1<<<' "$TMP/stage-f-unary-interp/gpu/model.noct")" -eq 2
test "$(grep -c 'p0: rpackedfloat_ptr' "$TMP/stage-f-broadcast-alias-interp/gpu/model.noct")" -eq 1
grep -F 'classes=[0,0]' "$TMP/stage-f-broadcast-alias-interp/gpu/model.noct" >/dev/null
grep -F 'family=copy-strided' "$TMP/stage-f-transpose-copy-interp/gpu/model.noct" >/dev/null
test "$(grep -c 'Accel.copyToAccel' "$TMP/stage-f-unary-interp/gpu/model.noct")" -eq 1
test "$(grep -c 'Accel.copyFromAccel' "$TMP/stage-f-unary-interp/gpu/model.noct")" -eq 1

check_stage_f_contract_error() {
	case_name=$1
	message=$2
	if "$NOCT" -j0 --path="$TMP/stage-f-unary-interp/gpu" \
		"$ROOT/tests/testcases/onnx2noct/stage-f-contract.noct" "$case_name" \
		"$TMP/stage-f-unary-interp/model.weights" \
		> "$TMP/error.out" 2>&1; then
		echo "FAIL Stage-F model contract case accepted: $case_name"
		exit 1
	fi
	grep -F "$message" "$TMP/error.out" >/dev/null || {
		cat "$TMP/error.out"
		exit 1
	}
}

check_stage_f_contract_error uninitialized 'Model is not initialized.'
check_stage_f_contract_error double-initialize 'Model is already initialized.'
check_stage_f_contract_error weights 'Cannot open file unexpected.weights.'
check_stage_f_contract_error short-input 'Model input element count mismatch.'
check_stage_f_contract_error short-output 'Model output element count mismatch.'
check_stage_f_contract_error alias 'Model input and output must be distinct.'

check_stage_g1_source() {
	name=$1
	kernels=$2
	storages=$3
	for label in interp jit; do
		mode=-j0
		if [ "$label" = jit ]; then mode=-j; fi
		output="$TMP/stage-g1-$name-$label"
		run_stage_f_converter "$mode" "$output" \
			"$TMP/stage-g1/$name.onnx" > "$TMP/stage-g1-summary.out"
		printf 'Generated Stage-G1 GPU model: kernels=%s storages=%s\n' \
			"$kernels" "$storages" > "$TMP/stage-g1-summary.expected"
		diff "$TMP/stage-g1-summary.expected" "$TMP/stage-g1-summary.out"
		diff "$ROOT/tests/testcases/onnx2noct/golden/stage-g1/$name.model.noct" \
			"$output/gpu/model.noct"
		test "$(find "$output" -maxdepth 1 -type f | wc -l)" -eq 2
		test -f "$output/gpu/model.noct"
		test -f "$output/gpu/main.noct"
		if grep -E '__accel func|DNN\.|DPL1|#version|layout\(' \
			"$output/gpu/model.noct" >/dev/null; then
			echo "FAIL forbidden Stage-G1 generated source construct: $name"
			exit 1
		fi
	done
	diff "$TMP/stage-g1-$name-interp/gpu/model.noct" \
		"$TMP/stage-g1-$name-jit/gpu/model.noct"
}

check_stage_g1_source conv-reuse 1 5
check_stage_g1_source conv-stride-relu 2 4
test "$(grep -c '^__gpu func k_conv2d_reference' \
	"$TMP/stage-g1-conv-reuse-interp/gpu/model.noct")" -eq 1
test "$(grep -c 'k_conv2d_reference_0<<<' \
	"$TMP/stage-g1-conv-reuse-interp/gpu/model.noct")" -eq 2
test "$(grep -c 'for (' "$TMP/stage-g1-conv-reuse-interp/gpu/model.noct")" -eq 3
test "$(grep -c 'Accel.copyToAccel(host_weight_' \
	"$TMP/stage-g1-conv-reuse-interp/gpu/model.noct")" -eq 2
grep -F 'family=conv2d-reference|family-version=1' \
	"$TMP/stage-g1-conv-reuse-interp/gpu/model.noct" >/dev/null
grep -F 'Weights.loadFloat32(weight_handle, 0, "conv_b", [1])' \
	"$TMP/stage-g1-conv-reuse-interp/gpu/model.noct" >/dev/null
grep -F 'Weights.loadFloat32(weight_handle, 1, "conv_w", [1,1,3,3])' \
	"$TMP/stage-g1-conv-reuse-interp/gpu/model.noct" >/dev/null
grep -F 'var sum: float = 0.0;' \
	"$TMP/stage-g1-conv-stride-relu-interp/gpu/model.noct" >/dev/null

check_stage_g2_source() {
	name=$1
	kernels=$2
	storages=$3
	for label in interp jit; do
		mode=-j0
		if [ "$label" = jit ]; then mode=-j; fi
		output="$TMP/stage-g2-$name-$label"
		run_stage_f_converter "$mode" "$output" \
			"$TMP/stage-g2/$name.onnx" > "$TMP/stage-g2-summary.out"
		printf 'Generated Stage-G2 GPU model: kernels=%s storages=%s\n' \
			"$kernels" "$storages" > "$TMP/stage-g2-summary.expected"
		diff "$TMP/stage-g2-summary.expected" "$TMP/stage-g2-summary.out"
		diff "$ROOT/tests/testcases/onnx2noct/golden/stage-g2/$name.model.noct" \
			"$output/gpu/model.noct"
		test "$(find "$output" -maxdepth 1 -type f | wc -l)" -eq 2
		test -f "$output/gpu/model.noct"
		test -f "$output/gpu/main.noct"
		if grep -E '__accel func|DNN\.|DPL1|#version|layout\(' \
			"$output/gpu/model.noct" >/dev/null; then
			echo "FAIL forbidden Stage-G2 generated source construct: $name"
			exit 1
		fi
	done
	diff "$TMP/stage-g2-$name-interp/gpu/model.noct" \
		"$TMP/stage-g2-$name-jit/gpu/model.noct"
}

check_stage_g2_source gemm-transpose-broadcast 1 4
check_stage_g2_source gemm-no-bias 1 3
check_stage_g2_source matmul-reuse 2 5
grep -F 'family=gemm-rank2|family-version=1' \
	"$TMP/stage-g2-gemm-transpose-broadcast-interp/gpu/model.noct" >/dev/null
grep -F 'Accel.float32FromBits(1056964608) * sum' \
	"$TMP/stage-g2-gemm-transpose-broadcast-interp/gpu/model.noct" >/dev/null
grep -F 'Accel.float32FromBits(3221225472)' \
	"$TMP/stage-g2-gemm-transpose-broadcast-interp/gpu/model.noct" >/dev/null
grep -F 'Weights.loadFloat32(weight_handle, 0, "gemm_b", [4,3])' \
	"$TMP/stage-g2-gemm-transpose-broadcast-interp/gpu/model.noct" >/dev/null
grep -F 'Weights.loadFloat32(weight_handle, 1, "gemm_c", [4])' \
	"$TMP/stage-g2-gemm-transpose-broadcast-interp/gpu/model.noct" >/dev/null
grep -F 'out[i] = Accel.float32FromBits(1065353216) * sum;' \
	"$TMP/stage-g2-gemm-no-bias-interp/gpu/model.noct" >/dev/null
test "$(grep -c '^__gpu func k_matmul_reference' \
	"$TMP/stage-g2-matmul-reuse-interp/gpu/model.noct")" -eq 1
test "$(grep -c 'k_matmul_reference_0<<<' \
	"$TMP/stage-g2-matmul-reuse-interp/gpu/model.noct")" -eq 2
test "$(grep -c 'for (kk in ' \
	"$TMP/stage-g2-matmul-reuse-interp/gpu/model.noct")" -eq 1

check_stage_g3_source() {
	name=$1; kernels=$2; storages=$3
	for label in interp jit; do
		mode=-j0
		if [ "$label" = jit ]; then mode=-j; fi
		output="$TMP/stage-g3-$name-$label"
		run_stage_f_converter "$mode" "$output" \
			"$TMP/stage-g3/$name.onnx" > "$TMP/stage-g3-summary.out"
		printf 'Generated Stage-G3 GPU model: kernels=%s storages=%s\n' \
			"$kernels" "$storages" > "$TMP/stage-g3-summary.expected"
		diff "$TMP/stage-g3-summary.expected" "$TMP/stage-g3-summary.out"
		diff "$ROOT/tests/testcases/onnx2noct/golden/stage-g3/$name.model.noct" \
			"$output/gpu/model.noct"
		test "$(find "$output" -maxdepth 1 -type f | wc -l)" -eq 2
		test -f "$output/gpu/model.noct"
		test -f "$output/gpu/main.noct"
	done
	diff "$TMP/stage-g3-$name-interp/gpu/model.noct" \
		"$TMP/stage-g3-$name-jit/gpu/model.noct"
}

check_stage_g3_source maxpool-padding 1 2
check_stage_g3_source averagepool-count 3 4
check_stage_g3_source global-averagepool 1 2
grep -F 'family=max-pool2d|family-version=1' \
	"$TMP/stage-g3-maxpool-padding-interp/gpu/model.noct" >/dev/null
grep -F 'candidate != candidate' \
	"$TMP/stage-g3-maxpool-padding-interp/gpu/model.noct" >/dev/null
grep -F 'value / valid_count' \
	"$TMP/stage-g3-averagepool-count-interp/gpu/model.noct" >/dev/null
grep -F 'value / 4.0' \
	"$TMP/stage-g3-averagepool-count-interp/gpu/model.noct" >/dev/null
grep -F 'family=global-average-pool|family-version=1' \
	"$TMP/stage-g3-global-averagepool-interp/gpu/model.noct" >/dev/null

for name in concat-strided-alias concat-axis-last; do
	for label in interp jit; do
		mode=-j0; if [ "$label" = jit ]; then mode=-j; fi
		output="$TMP/stage-g4-$name-$label"
		run_stage_f_converter "$mode" "$output" "$TMP/stage-g4/$name.onnx" \
			> "$TMP/stage-g4-summary.out"
		grep -F 'Generated Stage-G4 GPU model: kernels=1 storages=2' \
			"$TMP/stage-g4-summary.out" >/dev/null
		diff "$ROOT/tests/testcases/onnx2noct/golden/stage-g4/$name.model.noct" \
			"$output/gpu/model.noct"
	done
	diff "$TMP/stage-g4-$name-interp/gpu/model.noct" \
		"$TMP/stage-g4-$name-jit/gpu/model.noct"
done
grep -F 'classes=[0,0,0]' "$TMP/stage-g4-concat-strided-alias-interp/gpu/model.noct" >/dev/null
test "$(grep -c 'p0: rpackedfloat_ptr' \
	"$TMP/stage-g4-concat-strided-alias-interp/gpu/model.noct")" -eq 1

for spec in 'reduce-sum-mean 3 4' 'reduce-extrema 2 3' 'softmax-logsoftmax 3 4'; do
	set -- $spec; name=$1; kernels=$2; storages=$3
	for label in interp jit; do
		mode=-j0; if [ "$label" = jit ]; then mode=-j; fi
		output="$TMP/stage-g5-$name-$label"
		run_stage_f_converter "$mode" "$output" "$TMP/stage-g5/$name.onnx" \
			> "$TMP/stage-g5-summary.out"
		grep -F "Generated Stage-G5 GPU model: kernels=$kernels storages=$storages" \
			"$TMP/stage-g5-summary.out" >/dev/null
		diff "$ROOT/tests/testcases/onnx2noct/golden/stage-g5/$name.model.noct" "$output/gpu/model.noct"
	done
	diff "$TMP/stage-g5-$name-interp/gpu/model.noct" "$TMP/stage-g5-$name-jit/gpu/model.noct"
done
grep -F 'Accel.exp' "$TMP/stage-g5-softmax-logsoftmax-interp/gpu/model.noct" >/dev/null
grep -F 'Accel.log' "$TMP/stage-g5-softmax-logsoftmax-interp/gpu/model.noct" >/dev/null

for label in interp jit; do
	mode=-j0; if [ "$label" = jit ]; then mode=-j; fi
	output="$TMP/stage-g6-batchnorm-$label"
	run_stage_f_converter "$mode" "$output" "$TMP/stage-g6/batchnorm-reference.onnx" \
		> "$TMP/stage-g6-summary.out"
	grep -F 'Generated Stage-G6 GPU model: kernels=1 storages=6' \
		"$TMP/stage-g6-summary.out" >/dev/null
	diff "$ROOT/tests/testcases/onnx2noct/golden/stage-g6/batchnorm-reference.model.noct" \
		"$output/gpu/model.noct"
done
diff "$TMP/stage-g6-batchnorm-interp/gpu/model.noct" "$TMP/stage-g6-batchnorm-jit/gpu/model.noct"
grep -F 'Accel.sqrt' "$TMP/stage-g6-batchnorm-interp/gpu/model.noct" >/dev/null
test "$(grep -c 'Accel.copyToAccel(host_weight_' \
	"$TMP/stage-g6-batchnorm-interp/gpu/model.noct")" -eq 4
cmp "$TMP/stage-g6/batchnorm-reference.weights" \
	"$TMP/stage-g6-batchnorm-interp/model.weights"
grep -F 'Weights.open(weightsPath, "' \
	"$TMP/stage-g6-batchnorm-interp/gpu/model.noct" >/dev/null
if grep -F 'modelPackHash' \
	"$TMP/stage-g6-batchnorm-interp/gpu/model.noct" >/dev/null; then
	echo 'FAIL Stage-H package model retained self-hash weight binding'
	exit 1
fi
python3 "$ROOT/tests/testcases/onnx2noct/verify-package.py" \
	"$TMP/stage-g6-batchnorm-interp" \
	"$TMP/stage-g6/batchnorm-reference.onnx" >/dev/null

run_stage_f_converter -j0 "$TMP/stage-h-no-main" \
	"$TMP/stage-f/unary.onnx" --emit-main=no >/dev/null
[ ! -e "$TMP/stage-h-no-main/gpu/main.noct" ]
python3 "$ROOT/tests/testcases/onnx2noct/verify-package.py" \
	"$TMP/stage-h-no-main" "$TMP/stage-f/unary.onnx" >/dev/null

check_stage_f_converter_error() {
	message=$1
	shift
	if "$NOCT" -j0 --path="$ROOT/tools/onnx2noct" \
		"$ROOT/tools/onnx2noct/main.noct" "$@" \
		> "$TMP/error.out" 2>&1; then
		echo 'FAIL invalid Stage-F converter invocation accepted'
		exit 1
	fi
	grep -F -- "$message" "$TMP/error.out" >/dev/null || {
		cat "$TMP/error.out"
		exit 1
	}
}

check_stage_f_converter_error 'Usage: onnx2noct --output=DIR MODEL.onnx' \
	"$TMP/stage-f/unary.onnx"
check_stage_f_converter_error 'unknown Stage-F option' --target=cpu \
	--output="$TMP/not-created-option" "$TMP/stage-f/unary.onnx"
[ ! -e "$TMP/not-created-option" ]
check_stage_f_converter_error 'duplicate --output option' \
	--output="$TMP/not-created-duplicate-a" \
	--output="$TMP/not-created-duplicate-b" "$TMP/stage-f/unary.onnx"
[ ! -e "$TMP/not-created-duplicate-a" ]
[ ! -e "$TMP/not-created-duplicate-b" ]
check_stage_f_converter_error '--emit-main must be yes or no' \
	--emit-main=maybe --output="$TMP/not-created-emit-main" \
	"$TMP/stage-f/unary.onnx"
[ ! -e "$TMP/not-created-emit-main" ]
check_stage_f_converter_error 'duplicate --emit-main option' \
	--emit-main=yes --emit-main=no --output="$TMP/not-created-emit-main-2" \
	"$TMP/stage-f/unary.onnx"
[ ! -e "$TMP/not-created-emit-main-2" ]
check_stage_f_converter_error 'multiple model paths were provided' \
	--output="$TMP/not-created-models" "$TMP/stage-f/unary.onnx" \
	"$TMP/stage-f/transpose-copy.onnx"
[ ! -e "$TMP/not-created-models" ]
check_stage_f_converter_error 'no Stage F COPY/view/pointwise kernel family' \
	--output="$TMP/not-created-exp" "$TMP/stage-f/unsupported-exp.onnx"
[ ! -e "$TMP/not-created-exp" ]
run_stage_f_converter -j0 "$TMP/stage-i-pointwise-weight" \
	"$TMP/stage-f/unsupported-weight.onnx" >/dev/null
python3 "$ROOT/tests/testcases/onnx2noct/verify-package.py" \
	"$TMP/stage-i-pointwise-weight" \
	"$TMP/stage-f/unsupported-weight.onnx" >/dev/null
check_stage_f_converter_error 'does not emit unused float initializer' \
	--output="$TMP/not-created-unused-weight" \
	"$TMP/stage-f/unsupported-unused-weight.onnx"
[ ! -e "$TMP/not-created-unused-weight" ]
check_stage_f_converter_error 'requires a contiguous offset-zero NCHW input' \
	--output="$TMP/not-created-strided-conv" \
	"$TMP/stage-g1/unsupported-strided-conv.onnx"
[ ! -e "$TMP/not-created-strided-conv" ]
check_stage_f_converter_error 'supports dilation 1 only' \
	--output="$TMP/not-created-dilated-conv" \
	"$TMP/stage-g1/unsupported-dilated-conv.onnx"
[ ! -e "$TMP/not-created-dilated-conv" ]
check_stage_f_converter_error 'rather than a reviewed Stage G weighted family' \
	--output="$TMP/not-created-transposed-weight" \
	"$TMP/stage-g2/unsupported-transposed-weight.onnx"
[ ! -e "$TMP/not-created-transposed-weight" ]
check_stage_f_converter_error 'requires contiguous offset-zero NCHW input' \
	--output="$TMP/not-created-strided-pool" \
	"$TMP/stage-g3/unsupported-strided-pool.onnx"
[ ! -e "$TMP/not-created-strided-pool" ]
check_stage_f_converter_error 'supports 1..32 inputs' \
	--output="$TMP/not-created-concat-33" "$TMP/stage-g4/unsupported-concat-33.onnx"
[ ! -e "$TMP/not-created-concat-33" ]
check_stage_f_converter_error 'rather than a reviewed Stage G weighted family' \
	--output="$TMP/not-created-bn-alias" "$TMP/stage-g6/unsupported-aliased-parameter.onnx"
[ ! -e "$TMP/not-created-bn-alias" ]
mkdir "$TMP/existing-output"
check_stage_f_converter_error 'already exists' \
	--output="$TMP/existing-output" "$TMP/stage-f/unary.onnx"

APP_REL=${APP_TMP#"$ROOT/"}
printf '%s\n' 'ir=7 opset=12 nodes=1 initializers=0 input=x output=y' \
	> "$TMP/expected"
(
	cd "$ROOT"
	"$NOCT" --compile --app --path=tools/onnx2noct \
		"$APP_REL/reader.nap" tests/testcases/onnx2noct/reader.noct
)
"$NOCT" -j0 "$APP_TMP/reader.nap" \
	"$FIX/identity-opset12.onnx" > "$TMP/app.out"
diff "$TMP/expected" "$TMP/app.out"

(
	cd "$ROOT"
	"$NOCT" --compile --app --path=tools/onnx2noct \
		"$APP_REL/normalize.nap" tests/testcases/onnx2noct/normalize.noct
)
"$NOCT" -j0 "$APP_TMP/normalize.nap" \
	"$FIX/identity-opset12.onnx" > "$TMP/app-normalized.out"
diff "$ROOT/tests/testcases/onnx2noct/golden/stage-e/identity-opset12.norm" \
	"$TMP/app-normalized.out"

for name in unary broadcast-alias transpose-copy; do
	(
		cd "$ROOT"
		"$NOCT" --compile --app \
			--path="$TMP/stage-f-$name-interp/gpu" \
			"$APP_REL/stage-f-$name.nap" \
			tests/testcases/onnx2noct/stage-f-run.noct
	)
done

for name in conv-reuse conv-stride-relu; do
	(
		cd "$ROOT"
		"$NOCT" --compile --app \
			--path="$TMP/stage-g1-$name-interp/gpu" \
			"$APP_REL/stage-g1-$name.nap" \
			tests/testcases/onnx2noct/stage-f-run.noct
	)
done

for name in gemm-transpose-broadcast gemm-no-bias matmul-reuse; do
	(
		cd "$ROOT"
		"$NOCT" --compile --app \
			--path="$TMP/stage-g2-$name-interp/gpu" \
			"$APP_REL/stage-g2-$name.nap" \
			tests/testcases/onnx2noct/stage-f-run.noct
	)
done

for name in maxpool-padding averagepool-count global-averagepool; do
	(
		cd "$ROOT"
		"$NOCT" --compile --app \
			--path="$TMP/stage-g3-$name-interp/gpu" \
			"$APP_REL/stage-g3-$name.nap" \
			tests/testcases/onnx2noct/stage-f-run.noct
	)
done

for name in concat-strided-alias concat-axis-last; do
	( cd "$ROOT"; "$NOCT" --compile --app --path="$TMP/stage-g4-$name-interp/gpu" \
		"$APP_REL/stage-g4-$name.nap" tests/testcases/onnx2noct/stage-f-run.noct )
done

for name in reduce-sum-mean reduce-extrema softmax-logsoftmax; do
	( cd "$ROOT"; "$NOCT" --compile --app --path="$TMP/stage-g5-$name-interp/gpu" \
		"$APP_REL/stage-g5-$name.nap" tests/testcases/onnx2noct/stage-f-run.noct )
done

( cd "$ROOT"; "$NOCT" --compile --app --path="$TMP/stage-g6-batchnorm-interp/gpu" \
	"$APP_REL/stage-g6-batchnorm.nap" tests/testcases/onnx2noct/stage-f-run.noct )

echo 'All ONNX Stage-D through Stage-H package-generation tests passed.'
