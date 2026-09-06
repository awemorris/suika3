#!/bin/sh

set -eu

NOCT=${NOCT:-../../build-static/noct}

echo 'GPU-only accelerator compiler tests:'

if "$NOCT" --accel=vulkan accel/syntax-ok.noct > out 2>&1; then
	diff accel/syntax-ok.noct.out out
else
	grep -F 'Vulkan accelerator support is not available in this build.' out
fi
if "$NOCT" --accel=opengl accel/syntax-ok.noct > out 2>&1; then
	diff accel/syntax-ok.noct.out out
else
	grep -F 'OpenGL accelerator support is not available in this build.' out
fi

if "$NOCT" -j0 accel/legacy-resource-var.noct > out 2>&1; then
	echo 'FAIL legacy accel var spelling was accepted'
	exit 1
fi
grep -F 'accel var is no longer accepted; use __accel var.' out
if "$NOCT" -j0 accel/legacy-resource-let.noct > out 2>&1; then
	echo 'FAIL legacy accel let spelling was accepted'
	exit 1
fi
grep -F 'accel let is no longer accepted; use __accel let.' out
if "$NOCT" -j0 accel/resource-let-reassign.noct > out 2>&1; then
	echo 'FAIL __accel let resource was rebound'
	exit 1
fi
grep -F 'Cannot assign to constant "device".' out

"$NOCT" -j0 accel/syntax-ok.noct > out
diff accel/syntax-ok.noct.out out
"$NOCT" -j -O2 accel/syntax-ok.noct > out
diff accel/syntax-ok.noct.out out

for mode in '-j0' '-j'; do
	"$NOCT" $mode accel/typed-resources.noct > out
	diff accel/typed-resources.noct.out out
	"$NOCT" $mode accel/resource-copy.noct > out
	diff accel/resource-copy.noct.out out
	"$NOCT" $mode accel/async-copy-zero.noct > out
	diff accel/async-copy-zero.noct.out out
done

compile_dir=$(mktemp -d)
for tc in ptr-cpu branch neighbor convert unsafe-index cpu-call \
	  multi-doall multi-doall-local dosum doall-dosum \
	  doall-dosum-doall multi-dosum; do
	cp "accel/$tc.noct" "$compile_dir/$tc.noct"
	"$NOCT" --compile -O0 "$compile_dir/$tc.noct"
	test -s "$compile_dir/$tc.nb"
done
rm -rf "$compile_dir"
if "$NOCT" -j0 accel/ptr-host-error.noct > out 2>&1; then
	echo 'FAIL ordinary Packed accepted for _ptr'
	exit 1
fi
grep -F '_ptr argument is not an Accel resource.' out
if "$NOCT" -j0 accel/ptr-alias-error.noct > out 2>&1; then
	echo 'FAIL aliased accelerator buffers were accepted'
	exit 1
fi
grep -F 'restricted buffer arguments must not alias.' out
if "$NOCT" -j0 accel/copy-contract-error.noct > out 2>&1; then
	echo 'FAIL Accel resource was accepted by _in/_out transport'
	exit 1
fi
grep -F '_in/_out require host Packed storage; use _ptr' out
if "$NOCT" -j0 -O2 accel/bounds-short.noct > out 2>&1; then
	echo 'FAIL short managed buffer passed required-range guard'
	exit 1
fi
grep -F 'shorter than the managed kernel required range.' out
if "$NOCT" -j0 accel/untyped-resource.noct > out 2>&1; then
	echo 'FAIL untyped accelerator resource was accepted'
	exit 1
fi
grep -F "Unknown accelerator element type 'data'." out
if "$NOCT" -j0 accel/zero-resource.noct > out 2>&1; then
	echo 'FAIL zero-sized accelerator resource was accepted'
	exit 1
fi
grep -F 'Accelerator element count must be positive.' out
if "$NOCT" -j0 accel/overflow-resource.noct > out 2>&1; then
	echo 'FAIL overflowing accelerator resource was accepted'
	exit 1
fi
grep -F 'element count is too large.' out
if "$NOCT" -j0 accel/host-subscript.noct > out 2>&1; then
	echo 'FAIL host subscript of accelerator resource was accepted'
	exit 1
fi
grep -F 'Accelerator resources cannot be subscripted by host code.' out
if "$NOCT" -j0 accel/direct-resource-error.noct > out 2>&1; then
	echo 'FAIL direct __accel var dependency was accepted in __accel func'
	exit 1
fi
grep -F 'must be passed through a _ptr parameter' out
if "$NOCT" -j0 accel/constructor-outside-marker.noct > out 2>&1; then
	echo 'FAIL accelerator constructor was callable outside __accel var'
	exit 1
fi
grep -F 'uint32' out >/dev/null

if "$NOCT" -j0 accel/missing-void.noct > out 2>&1; then
	echo 'FAIL accelerator function without void was accepted'
	exit 1
fi
grep -F 'An accelerator function must declare a void return type.' out

for mode in '-j0' '-j'; do
	if "$NOCT" $mode accel/cpu-call.noct > out 2>&1; then
		echo 'FAIL __accel func executed without a GPU backend'
		exit 1
	fi
	grep -F 'GPU accelerator is disabled; __accel func has no CPU fallback.' \
		out >/dev/null
done

info=$("$NOCT" --disable-accel --accel-info -j0 \
	-O2 accel/cpu-call.noct 2>&1 || true)
printf '%s\n' "$info" | grep -F 'accelerator kernel generated (uint32, 1D)'
printf '%s\n' "$info" | grep -F 'has no CPU fallback.'

if "$NOCT" -j0 accel/direct-call.noct > out 2>&1; then
	echo 'FAIL direct accelerator call was accepted'
	exit 1
fi
grep -F "must be invoked with Accel.call()" out

if "$NOCT" -j0 accel/gpu-syntax.noct > out 2>&1; then
	echo 'FAIL direct CPU execution of __gpu func was accepted'
	exit 1
fi
grep -F "GPU function 'empty' cannot execute on the CPU." out

if "$NOCT" -j0 accel/gpu-dispatch.noct > out 2>&1; then
	echo 'FAIL gpu dispatch used a CPU fallback without OpenGL'
	exit 1
fi
grep -F '__gpu func requires the OpenGL backend' out

if "$NOCT" -j0 accel/gpu-barrier-divergent.noct > out 2>&1; then
	echo 'FAIL divergent raw GPU barrier was accepted'
	exit 1
fi
grep -F 'syncthreads() must be at top-level uniform control flow.' out
if "$NOCT" -j0 accel/gpu-barrier-return.noct > out 2>&1; then
	echo 'FAIL early return with raw GPU barrier was accepted'
	exit 1
fi
grep -F 'containing syncthreads() cannot return early.' out

gpu_compile_dir=$(mktemp -d)
cp accel/gpu-loop-accumulator.noct "$gpu_compile_dir/gpu-loop-accumulator.noct"
cp accel/gpu-math-mnist.noct "$gpu_compile_dir/gpu-math-mnist.noct"
cp accel/gpu-float32-bits.noct "$gpu_compile_dir/gpu-float32-bits.noct"
cp accel/gpu-barrier-final-return.noct "$gpu_compile_dir/gpu-barrier-final-return.noct"
for level in 0 1 2; do
	"$NOCT" --compile -O"$level" \
		"$gpu_compile_dir/gpu-loop-accumulator.noct"
	"$NOCT" --compile -O"$level" \
		"$gpu_compile_dir/gpu-math-mnist.noct"
	"$NOCT" --compile -O"$level" \
		"$gpu_compile_dir/gpu-float32-bits.noct"
done
"$NOCT" --compile "$gpu_compile_dir/gpu-barrier-final-return.noct"

for backend in ansic elisp scheme; do
	if "$NOCT" "--$backend" "$gpu_compile_dir/out.$backend" \
		"$gpu_compile_dir/gpu-math-mnist.noct" > out 2>&1; then
		echo "FAIL $backend transpiler accepted __gpu func"
		exit 1
	fi
	grep -F "__gpu func is not supported by the" out >/dev/null
	grep -F "transpiler." out >/dev/null
done
rm -rf "$gpu_compile_dir"

for level in 0 1 2; do
	for spec in \
		"gpu-loop-dynamic-error.noct|bounds must be compile-time int constants" \
		"gpu-loop-negative-error.noct|bounds must be non-negative" \
		"gpu-loop-overflow-error.noct|bound exceeds the int range" \
		"gpu-loop-collection-error.noct|Collection iteration is unsupported" \
		"gpu-loop-scope-error.noct|temporary" \
		"gpu-loop-type-error.noct|type does not match" \
		"gpu-loop-depth-error.noct|loop-nesting limit exceeded" \
		"gpu-loop-barrier-error.noct|top-level uniform control flow" \
		"gpu-math-context-error.noct|valid only inside __gpu func" \
		"gpu-math-accel-context-error.noct|valid only inside __gpu func" \
		"gpu-math-firstclass-error.noct|must be called directly inside __gpu func" \
		"gpu-math-arity-error.noct|expects 1 float32 argument" \
		"gpu-math-type-error.noct|type does not match" \
		"gpu-math-unsupported-error.noct|registered but not supported" \
		"gpu-math-unknown-error.noct|Unknown GPU math operation" \
		"gpu-math-shadow-error.noct|reserved name inside __gpu func" \
		"gpu-float32-bits-context-error.noct|valid only inside __gpu func" \
		"gpu-float32-bits-firstclass-error.noct|must be called directly inside __gpu func" \
		"gpu-float32-bits-arity-error.noct|expects 1 int32 argument" \
		"gpu-float32-bits-type-error.noct|requires a 32-bit integer literal" \
		"gpu-float32-bits-range-error.noct|literal is out of range"; do
		tc=${spec%%|*}
		expected=${spec#*|}
		if "$NOCT" -j0 -O"$level" \
			"accel/$tc" > out 2>&1; then
			echo "FAIL $tc was accepted at optimization level $level"
			exit 1
		fi
		grep -F "$expected" out >/dev/null
	done
done

registry_dir=$(mktemp -d)
${CC:-cc} -I../../include -I../../src/core -Wall -Wextra -Werror \
	accel/accel-ops-test.c ../../src/core/accel_ops.c -o "$registry_dir/accel-ops-test"
"$registry_dir/accel-ops-test"
rm -rf "$registry_dir"

if "$NOCT" -j0 accel/async.noct > out 2>&1; then
	echo 'FAIL removed Accel.callAsync API was accepted'
	exit 1
fi
grep -F 'callAsync' out

tmp_dir=$(mktemp -d)
cp accel/cpu-call.noct "$tmp_dir/cpu-call.noct"
"$NOCT" --compile -O2 "$tmp_dir/cpu-call.noct"
grep -a -F 'Accelerator' "$tmp_dir/cpu-call.nb" >/dev/null
grep -a -F 'GLSL Size' "$tmp_dir/cpu-call.nb" >/dev/null
grep -a -F 'HLSL Size' "$tmp_dir/cpu-call.nb" >/dev/null
grep -a -F 'RWStructuredBuffer<uint>' "$tmp_dir/cpu-call.nb" >/dev/null
if "$NOCT" --disable-accel -j0 "$tmp_dir/cpu-call.nb" \
	> out 2>&1; then
	echo 'FAIL accelerator bytecode executed on the CPU'
	exit 1
fi
grep -F 'has no CPU fallback.' out >/dev/null
cp accel/multi-doall-local.noct "$tmp_dir/multi-doall-local.noct"
"$NOCT" --compile -O0 "$tmp_dir/multi-doall-local.noct"
grep -a -F 'Accelerator Program' "$tmp_dir/multi-doall-local.nb" >/dev/null
if "$NOCT" --disable-accel -j "$tmp_dir/multi-doall-local.nb" \
	> out 2>&1; then
	echo 'FAIL multi-kernel bytecode executed on the CPU'
	exit 1
fi
grep -F 'has no CPU fallback.' out >/dev/null
cp accel/dosum.noct "$tmp_dir/dosum.noct"
"$NOCT" --compile -O0 "$tmp_dir/dosum.noct"
grep -a -F 'Accelerator Program' "$tmp_dir/dosum.nb" >/dev/null
if "$NOCT" --disable-accel -j0 "$tmp_dir/dosum.nb" \
	> out 2>&1; then
	echo 'FAIL DOSUM bytecode executed on the CPU'
	exit 1
fi
grep -F 'has no CPU fallback.' out >/dev/null
rm -rf "$tmp_dir"

app_path="accel/.tmp-resource-$$.nap"
"$NOCT" --compile --app -O2 "$app_path" \
	accel/resource-copy.noct
"$NOCT" -j0 "$app_path" > out
diff accel/resource-copy.noct.out out
"$NOCT" -j "$app_path" > out
diff accel/resource-copy.noct.out out
rm -f "$app_path"

for backend in ansic elisp scheme; do
	if "$NOCT" "--$backend" out.transpiled \
		accel/vulkan-resource.noct > out 2>&1; then
		echo "FAIL $backend transpiler accepted GPU-only __accel func"
		exit 1
	fi
	grep -F '__accel func is GPU-only' out >/dev/null
done
rm -f out.transpiled

rm -f out accel.err
echo 'All GPU-only accelerator compiler tests passed.'
