#!/bin/sh

set -eu

echo 'NoctLang Tests'
echo

echo 'Running bootstrap tests...'
echo "(Interpreter)";
for tc in syntax/*.noct; do
    echo "$tc";
    ../build/noct --disable-jit $tc > out || true;
    diff $tc.out out;
done
echo "(JIT)";
for tc in syntax/*.noct; do
    echo "$tc";
    ../build/noct --force-jit $tc > out || true;
    diff $tc.out out;
done
echo 'All tests passed.'
