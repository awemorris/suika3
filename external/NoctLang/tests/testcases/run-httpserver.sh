#!/bin/sh

set -eu

NOCT=${NOCT:-../../build-mt-debug/noct}

echo 'NoctLang HttpServer Tests'
echo

echo "(Interpreter)"
for tc in httpserver/*.noct; do
    echo "$tc";
    $NOCT -j0 $tc > out || true;
    diff $tc.out out;
done
echo "(JIT)"
for tc in httpserver/*.noct; do
    echo "$tc";
    $NOCT -j $tc > out || true;
    diff $tc.out out;
done
echo 'All HttpServer tests passed.'
