#!/bin/sh
set -eu
NOCT=${NOCT:-../../build-mt-debug/noct}
echo 'Process API tests'
for tc in process/*.noct; do
    echo "$tc"
    "$NOCT" -j0 "$tc" > out
    diff "$tc.out" out
done
rm -f out
echo 'All Process tests passed.'
