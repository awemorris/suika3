#!/bin/sh

#
# CSE test suite (docs/design/05-cse.md).
#
# Runs every case at optimize level 0 and 2, with the interpreter and
# the JIT.  All four runs must match the golden output byte-for-byte:
# CSE must never change observable behavior.
#

NOCT=${NOCT:-../../build-static/noct}

echo 'CSE tests:'

FAILED=0
for tc in cse/*.noct; do
    for lvl in "-O0" "-O2"; do
        # Error-message line numbers are debug info: they exist at
        # level 0 and are omitted at level >= 1 (OP_LINEINFO).  A case
        # may therefore provide NAME.noct.out2 as the level-2 golden.
        golden="$tc.out"
        if [ "$lvl" = "-O2" ] && [ -f "$tc.out2" ]; then
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
    rm -f out
    echo "PASS $tc"
done

#
# R4 regression (docs/design/05-cse.md): the pass must fire on the
# parameter-based flagship shape.  Only meaningful when the binary was
# built with NOCT_ENABLE_OPTIMIZER, so skip if the pass reports nothing
# at all across the whole suite run above.
#
if NOCT_CSE_DEBUG=1 $NOCT -j0 -O1 \
        cse/r4_param_flagship.noct 2>&1 | grep -q '^\[cse\]'; then
    if NOCT_CSE_DEBUG=1 $NOCT -j0 -O1 \
            cse/r4_param_flagship.noct 2>&1 | \
            grep -q '\[cse\] .*:f: [1-9][0-9]* captures'; then
        echo 'PASS cse debug counter (R4)'
    else
        echo 'FAIL cse debug counter (R4): pass did not fire on f(d){d.k+d.k}'
        FAILED=1
    fi
fi

if [ "$FAILED" -ne 0 ]; then
    echo 'CSE tests failed.'
    exit 1
fi

echo 'All CSE tests passed.'
