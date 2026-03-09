#!/bin/sh

set -eu

echo 'NoctLang Tests'
echo

echo 'Running bootstrap tests...'
for tc in elisp/*.noct; do
    echo "$tc";
    ../build/noct --elisp out $tc || true;
    diff $tc.out out;
done

echo
echo 'All tests passed.'
