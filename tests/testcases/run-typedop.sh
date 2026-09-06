#!/bin/sh

#
# Typed-ops test suite (docs/design/07-typed-ops.md).
#
# Runs every case at optimize level 0, 1, and 2, with the interpreter and
# the JIT.  All six runs must match the golden output byte-for-byte:
# typed opcodes must never change observable behavior.
#
# A case may provide NAME.noct.out2 as the level-2 golden when its
# output contains error line numbers (debug info present at level 0,
# omitted at level >= 1).
#
# Cases listed in MUST_EMIT must additionally report at least one
# typed-op emission via NOCT_TYPED_DEBUG=1 at level 2 (a golden test
# alone cannot catch the optimization silently dying).
#

NOCT=${NOCT:-../../build-static/noct}

MUST_EMIT="arith farith abce_region lattice shift_edges"

echo 'Typed-ops tests:'

FAILED=0
for tc in typedop/*.noct; do
    for lvl in "-O0" "-O1" "-O2"; do
        golden="$tc.out"
        if [ "$lvl" != "-O0" ] && [ -f "$tc.out2" ]; then
            golden="$tc.out2"
        fi
        for jit in "-j0" "-j"; do
            $NOCT $jit $lvl "$tc" > out 2>&1
            if ! diff -q "$golden" out > /dev/null 2>&1; then
                echo "FAIL $tc ($jit $lvl)"
                diff "$golden" out | head -5
                FAILED=1
            fi
        done
    done
    echo "PASS $tc"
done

# Emission assertions (level 1, interpreter is enough: emission is
# decided at LIR build time, before the backend choice).
for name in $MUST_EMIT; do
    tc="typedop/$name.noct"
    if [ ! -f "$tc" ]; then
        continue
    fi
    n=$(NOCT_TYPED_DEBUG=1 $NOCT -j0 -O1 "$tc" 2>&1 \
        | grep -c '^TYPED: .*emitted=[1-9]')
    if [ "$n" -eq 0 ]; then
        echo "FAIL $tc (no typed emission reported)"
        FAILED=1
    else
        echo "PASS $tc (typed emission reported)"
    fi
done

# Variable proven-int division must select the checked typed opcode, while a
# statically safe literal divisor stays on the existing unchecked fast path.
n=$(NOCT_TYPED_DEBUG=1 $NOCT -j0 -O1 typedop/checked_div.noct 2>&1 \
    | grep '^TYPED: checked_div:' | grep -c 'checked_div=[1-9]')
if [ "$n" -eq 0 ]; then
    echo "FAIL typedop/checked_div.noct (checked division not emitted)"
    FAILED=1
else
    echo "PASS typedop/checked_div.noct (checked division emitted)"
fi

n=$(NOCT_TYPED_DEBUG=1 $NOCT -j0 -O1 typedop/arith.noct 2>&1 \
    | grep '^TYPED: f:' | grep -c 'checked_div=0')
if [ "$n" -eq 0 ]; then
    echo "FAIL typedop/arith.noct (safe literal used checked division)"
    FAILED=1
else
    echo "PASS typedop/arith.noct (safe literal stayed unchecked)"
fi

# Persist and reload the new append-only opcodes.  Cross builds use the same
# bytecode path, so source-only execution is not sufficient coverage.
work=".tmp-typedop-$$"
mkdir "$work"
cp typedop/checked_div.noct "$work/checked_div.noct"
$NOCT --compile -O1 "$work/checked_div.noct" >/dev/null
for jit in -j0 -j; do
    $NOCT "$jit" "$work/checked_div.nbc" > "$work/out" 2>&1
    if ! diff -q typedop/checked_div.noct.out "$work/out" >/dev/null 2>&1; then
        echo "FAIL typedop/checked_div.noct (persisted opcode, $jit)"
        diff typedop/checked_div.noct.out "$work/out" | head -5
        FAILED=1
    fi
done
rm -rf "$work"
echo "PASS typedop/checked_div.noct (persisted opcode)"

# Persist and reload OP_MATERIALIZE_TYPE as well.  The optimized golden
# observes long/double widening and every dynamic escape boundary.
work=".tmp-tag-materialize-$$"
mkdir "$work"
cp typedop/tag-materialize.noct "$work/tag-materialize.noct"
$NOCT --compile -O1 "$work/tag-materialize.noct" >/dev/null
for jit in -j0 -j; do
    $NOCT "$jit" "$work/tag-materialize.nbc" > "$work/out" 2>&1
    if ! diff -q typedop/tag-materialize.noct.out2 "$work/out" >/dev/null 2>&1; then
        echo "FAIL typedop/tag-materialize.noct (persisted materialize, $jit)"
        diff typedop/tag-materialize.noct.out2 "$work/out" | head -5
        FAILED=1
    fi
done
rm -rf "$work"
echo "PASS typedop/tag-materialize.noct (persisted materialize opcode)"

n=$(NOCT_TAGSTORE_DEBUG=1 $NOCT -j0 -O1 typedop/tag-materialize.noct 2>&1 \
    | grep '^TAGSTORE: main:' | grep -c 'materialize_ops=[1-9]')
if [ "$n" -eq 0 ]; then
    echo "FAIL typedop/tag-materialize.noct (materialize emission not reported)"
    FAILED=1
else
    echo "PASS typedop/tag-materialize.noct (materialize emission reported)"
fi

# Note on abce_w64.noct: there is deliberately NO "zero emission"
# assertion for it.  Stage B legitimately types the ABCE guard
# arithmetic ($lo >= 0 etc.) even for a 64-bit-bet loop, so the
# per-function counter is nonzero.  The invariant that matters --
# the long-contaminated accumulator inside the w64 fast body must
# never see a typed int op -- is enforced by the golden output
# (the sum only comes out right through the generic long path).

rm -f out

if [ "$FAILED" -ne 0 ]; then
    echo 'Typed-ops tests FAILED.'
    exit 1
fi
echo 'All typed-ops tests passed.'
