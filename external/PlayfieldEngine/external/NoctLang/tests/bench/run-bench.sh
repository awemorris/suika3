#!/bin/sh
# ABCE synthetic benchmarks: optimize-level 0 vs 2, interpreter and JIT.
bench_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
repo_dir=$(CDPATH= cd -- "$bench_dir/.." && pwd)
NOCT=${NOCT:-"$repo_dir/build-static/noct"}
cd "$bench_dir"
for tc in b*.noct; do
    printf "%-22s" "$tc"
    for mode in "-j0" "-j"; do
        for lvl in "-O0" "-O2"; do
            t0=$(date +%s.%N)
            $NOCT $mode $lvl "$tc" > /dev/null 2>&1
            t1=$(date +%s.%N)
            printf " %8.3f" "$(echo "$t1 - $t0" | bc)"
        done
    done
    echo
done
echo "columns: interp-L0 interp-L2 jit-L0 jit-L2 (seconds)"
