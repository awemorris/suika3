#!/bin/sh

set -eu

root=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
build_dir=${1:-"$root/build-debug"}
case "$build_dir" in
/*) ;;
*) build_dir="$root/$build_dir" ;;
esac
cc=${CC:-cc}
out="$build_dir/jit-slab-test"
failure_out="$build_dir/jit-memory-failure-test"
failure_debug="$build_dir/jit-memory-failure.debug"

"$cc" -I"$root/include" "$root/tests/testcases/jit-slab-test.c" \
	"$build_dir/libnoct.a" -lm -lpthread -o "$out"
"$out"

"$cc" -I"$root/include" \
	"$root/tests/testcases/jit-memory-failure-test.c" \
	"$build_dir/libnoct.a" -lm -lpthread \
	-Wl,--wrap=mprotect -Wl,--wrap=munmap -o "$failure_out"
NOCT_JIT_DEBUG=1 "$failure_out" 2>"$failure_debug"
grep -q '^noct-jit-memory: mprotect-rx size=[0-9][0-9]* status=failed error=[0-9][0-9]*$' \
	"$failure_debug"
grep -q '^noct-jit-lifecycle: publish status=failed$' "$failure_debug"
if grep -q '^noct-jit: protected_value: native-entry$' "$failure_debug"; then
	echo "failed JIT entry was executed" >&2
	exit 1
fi
grep -q '^noct-jit: teardown_value: native-entry$' "$failure_debug"
grep -q '^noct-jit-memory: munmap size=[0-9][0-9]* status=failed error=[0-9][0-9]*$' \
	"$failure_debug"
grep -q '^noct-jit-lifecycle: destroy status=failed$' "$failure_debug"

# On x86_64 O3 the first function plus blend exceed 28KiB, while blend alone
# fits. This exercises whole-function retry on a fresh slab.
"$build_dir/noct" -O3 --jit-code-size=28672 \
	"$root/tests/testcases/simd/drawimage/blend-alpha.noct" > "$build_dir/jit-slab-blend.out"
diff "$root/tests/testcases/simd/drawimage/blend-alpha.noct.out" \
	"$build_dir/jit-slab-blend.out"
