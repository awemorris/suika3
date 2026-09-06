#!/bin/sh

set -eu

NOCT=${NOCT:-../../build-mt-debug/noct}
repeats=${NOCT_THREAD_STRESS_REPEATS:-100}

case "$repeats" in
    ''|*[!0-9]*|0)
        echo "NOCT_THREAD_STRESS_REPEATS must be a positive integer." >&2
        exit 2
        ;;
esac

echo "NoctLang Thread Stress Tests ($repeats repetitions)"

for mode in -j0 -j; do
    echo "Mode: $mode"
    for tc in thread/07-doop-threads.noct \
              thread/10-shared-dict-expand.noct \
              thread/11-promotion-race.noct; do
        i=0
        while [ "$i" -lt "$repeats" ]; do
            "$NOCT" "$mode" "$tc" > out
            cmp "$tc.out" out
            i=$((i + 1))
        done
        echo "PASS $tc"
    done
done

echo "All thread stress tests passed."
