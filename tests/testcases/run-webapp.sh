#!/bin/sh

set -eu

NOCT=${NOCT:-../../build-mt-debug/noct}
FRAMEWORK=../../apps/webapp/framework.noct

echo 'NoctLang WebApp Tests'
echo

mkdir -p webapp/build
for tc in webapp/*.noct; do
    name=$(basename "$tc" .noct)
    cat "$FRAMEWORK" "$tc" > "webapp/build/$name.noct"
done

echo "(Interpreter)"
for tc in webapp/*.noct; do
    name=$(basename "$tc" .noct)
    echo "$tc";
    $NOCT -j0 "webapp/build/$name.noct" > out || true;
    diff "$tc.out" out;
done
echo "(JIT)"
for tc in webapp/*.noct; do
    name=$(basename "$tc" .noct)
    echo "$tc";
    $NOCT -j "webapp/build/$name.noct" > out || true;
    diff "$tc.out" out;
done
echo 'All WebApp tests passed.'
