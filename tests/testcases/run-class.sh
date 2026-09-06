#!/bin/sh

#
# Frozen class dict / top-level declaration test suite
# (docs/design/03-class.md).  Golden-diff over interpreter and JIT.
#

NOCT=${NOCT:-../../build-static/noct}

echo 'Class tests:'

FAILED=0
for tc in class/*.noct; do
    for jit in "-j0" "-j"; do
        $NOCT $jit "$tc" > out 2>&1
        if ! diff -q "$tc.out" out > /dev/null 2>&1; then
            echo "FAIL $tc ($jit)"
            diff "$tc.out" out | head -5
            FAILED=1
        fi
    done
    rm -f out
    echo "PASS $tc"
done

if [ "$FAILED" -ne 0 ]; then
    echo 'Class tests failed.'
    exit 1
fi
echo 'All class tests passed.'
