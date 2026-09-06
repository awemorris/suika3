#!/bin/sh

#
# Block scoping / let / TDZ test suite (docs/design/04-scoping.md).
# Golden-diff; error cases assert the exact compile error message.
#

NOCT=${NOCT:-../../build-static/noct}

echo 'Scoping tests:'

FAILED=0
for tc in scoping/*.noct; do
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
    echo 'Scoping tests failed.'
    exit 1
fi
echo 'All scoping tests passed.'
