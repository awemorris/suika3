#!/bin/sh

# Convert run-simd-bench.sh CSV into a compact Markdown speedup table.
# Usage: ./report-simd-bench.sh [simd-bench.csv]
# With no file argument, CSV is read from standard input.

set -eu

if [ "$#" -gt 1 ]; then
    echo "usage: $0 [simd-bench.csv]" >&2
    exit 2
fi

input=${1:-/dev/stdin}

awk -F, '
BEGIN {
    print "| case | mode | median (ms) | vs no-SIMD | vs vector-scalar |"
    print "|---|---|---:|---:|---:|"
}
$1 == "case" { next }
NF != 5 {
    printf "invalid CSV at line %d\n", NR > "/dev/stderr"
    exit 2
}
{
    key = $1 SUBSEP $2
    if (!(key in seen)) {
        seen[key] = 1
        row_case[++rows] = $1
        row_mode[rows] = $2
    }
    median[key] = $3 + 0
}
END {
    if (rows == 0)
        exit 0
    for (i = 1; i <= rows; i++) {
        c = row_case[i]
        m = row_mode[i]
        value = median[c SUBSEP m]
        no_simd = median[c SUBSEP "l2-nosimd"]
        vscalar = median[c SUBSEP "l2-vscalar"]
        if (no_simd > 0)
            s1 = sprintf("%.2fx", no_simd / value)
        else
            s1 = "-"
        if (vscalar > 0)
            s2 = sprintf("%.2fx", vscalar / value)
        else
            s2 = "-"
        printf "| %s | %s | %.3f | %s | %s |\n", c, m, value, s1, s2
    }
}
' "$input"
