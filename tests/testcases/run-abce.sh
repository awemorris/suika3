#!/bin/sh

#
# ABCE test suite (docs/design/01-abce.md).
#
# Runs every case at optimize level 0 and 2, with the interpreter and
# the JIT.  All four runs must match the golden output byte-for-byte:
# ABCE must never change observable behavior.
#

NOCT=${NOCT:-../../build-static/noct}

echo 'ABCE tests:'

FAILED=0
for tc in abce/*.noct; do
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

if [ "$FAILED" -ne 0 ]; then
    echo 'ABCE tests failed.'
    exit 1
fi
echo 'All ABCE tests passed.'
