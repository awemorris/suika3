#!/bin/sh

set -eu

root=$(CDPATH= cd -- "$(dirname -- "$0")/../.." && pwd)
NOCT=${NOCT:-"$root/build-mt-debug/noct"}

if [ "$#" -ne 0 ]; then
    echo "usage: $0" >&2
    exit 2
fi
if [ ! -x "$NOCT" ]; then
    echo "Noct CLI not found: $NOCT" >&2
    exit 2
fi

case_dir="$root/tests/testcases"
cd "$case_dir"
log=$(mktemp)
blend_copy_nb=simd/drawimage/blend-copy.nbc
blend_add_nb=simd/drawimage/blend-add.nbc
packed_loop_nb=packed-loop/regcache.nbc
unroll_nb=packed-loop/unroll-width16.nbc
trap 'rm -f -- noct-arch out "$log" "$blend_copy_nb" "$blend_copy_nb.out" "$blend_add_nb" "$blend_add_nb.out" "$packed_loop_nb" "$packed_loop_nb.out" "$unroll_nb" "$unroll_nb.out"' EXIT HUP INT TERM

# Cross builds intentionally do not link the optimizer.  Produce optimized,
# portable bytecode once with the host compiler so every target exercises the
# persisted vector opcodes and its own interpreter/JIT lowering.
"$NOCT" --compile -O2 simd/drawimage/blend-copy.noct >/dev/null 2>&1
"$NOCT" --compile -O2 simd/drawimage/blend-add.noct >/dev/null 2>&1
"$NOCT" --compile -O2 packed-loop/regcache.noct >/dev/null 2>&1
"$NOCT" --compile -O2 \
    packed-loop/unroll-width16.noct >/dev/null 2>&1
cp simd/drawimage/blend-copy.noct.out "$blend_copy_nb.out"
cp simd/drawimage/blend-add.noct.out "$blend_add_nb.out"
cp packed-loop/regcache.noct.out "$packed_loop_nb.out"
cp packed-loop/unroll-width16.noct.out "$unroll_nb.out"

"$NOCT" -j0 multiarch.noct | tee "$log"
if ! grep -Fx 'All available multi-architecture tests passed.' "$log" >/dev/null; then
    echo 'Multi-architecture tests failed.' >&2
    exit 1
fi
