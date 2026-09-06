#!/bin/sh

# x86_64 JIT SIMD microbenchmark.  Output is CSV; diagnostics go to stderr.
#
# Environment:
#   NOCT=path       executable (default: build-static/noct)
#   RUNS=odd-number measured repetitions per case/mode (default: 5)
#   WARMUPS=number  unmeasured repetitions (default: 1)
#   CPU=number      optional taskset CPU (example: CPU=3)
#   MODES="..."     optional subset/order of the modes below

set -eu

bench_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
repo_dir=$(CDPATH= cd -- "$bench_dir/.." && pwd)
NOCT=${NOCT:-"$repo_dir/build-static/noct"}
RUNS=${RUNS:-5}
WARMUPS=${WARMUPS:-1}
CPU=${CPU:-}
MODES=${MODES:-"l2-nosimd l2-vscalar l2-sse2 l2-sse3 l2-sse41"}

case "$RUNS" in
*[!0-9]*|'') echo "RUNS must be a positive odd integer" >&2; exit 2 ;;
esac
if [ "$RUNS" -lt 1 ] || [ $((RUNS % 2)) -eq 0 ]; then
    echo "RUNS must be a positive odd integer" >&2
    exit 2
fi
case "$WARMUPS" in
*[!0-9]*|'') echo "WARMUPS must be a non-negative integer" >&2; exit 2 ;;
esac
if [ ! -x "$NOCT" ]; then
    echo "NOCT executable not found: $NOCT" >&2
    exit 2
fi
if [ -n "$CPU" ] && ! command -v taskset >/dev/null 2>&1; then
    echo "CPU was specified but taskset is unavailable" >&2
    exit 2
fi

tmp_dir=$(mktemp -d)
trap 'rm -rf -- "$tmp_dir"' EXIT HUP INT TERM

invoke()
{
    if [ -n "$CPU" ]; then
        taskset -c "$CPU" "$@"
    else
        "$@"
    fi
}

run_mode()
{
    mode=$1
    file=$2
    output=$3
    case "$mode" in
    jit-l0)
        invoke "$NOCT" -j "$file" >"$output"
        ;;
    l2-nosimd)
        invoke env NOCT_SIMD_DISABLE=1 "$NOCT" -j \
            -O2 "$file" >"$output"
        ;;
    l2-vscalar)
        invoke env NOCT_JIT_SIMD_MAX=scalar "$NOCT" -j \
            -O2 "$file" >"$output"
        ;;
    l2-sse2)
        invoke env NOCT_JIT_SIMD_MAX=sse2 "$NOCT" -j \
            -O2 "$file" >"$output"
        ;;
    l2-sse3)
        invoke env NOCT_JIT_SIMD_MAX=sse3 "$NOCT" -j \
            -O2 "$file" >"$output"
        ;;
    l2-sse41)
        invoke env NOCT_JIT_SIMD_MAX=sse41 "$NOCT" -j \
            -O2 "$file" >"$output"
        ;;
    l2-neon)
        invoke env NOCT_JIT_SIMD_MAX=neon "$NOCT" -j \
            -O2 "$file" >"$output"
        ;;
    l2-altivec)
        invoke env NOCT_JIT_SIMD_MAX=altivec "$NOCT" -j \
            -O2 "$file" >"$output"
        ;;
    *)
        echo "unknown mode: $mode" >&2
        exit 2
        ;;
    esac
}

now_ns()
{
    date +%s%N
}

cpu_name=$(sed -n 's/^model name[[:space:]]*:[[:space:]]*//p' \
    /proc/cpuinfo 2>/dev/null | head -1 || true)
if [ -z "$cpu_name" ] && command -v sysctl >/dev/null 2>&1; then
    cpu_name=$(sysctl -n machdep.cpu.brand_string 2>/dev/null || true)
fi
echo "CPU: ${cpu_name:-unknown}" >&2
echo "NOCT: $NOCT" >&2
echo "RUNS=$RUNS WARMUPS=$WARMUPS CPU=${CPU:-unbound}" >&2

# Confirm that the selected host path contains vector bytecode and report caps.
invoke env NOCT_JIT_SIMD_DEBUG=1 "$NOCT" -j -O2 \
    "$bench_dir/b12_vblend.noct" >/dev/null

printf 'case,mode,median_ms,min_ms,max_ms\n'
for spec in \
    "alpha-blend:$bench_dir/b12_vblend.noct" \
    "f32-affine:$bench_dir/b13_simd_f32_affine.noct" \
    "u32-inplace:$bench_dir/b14_simd_u32_inplace.noct" \
    "u32-3buffer:$bench_dir/b15_simd_u32_mix.noct"
do
    name=${spec%%:*}
    file=${spec#*:}

    # The non-vectorized L2 path is the semantic reference.  Refuse to time
    # mismatching modes.  jit-l0 remains available through MODES when wanted.
    run_mode l2-nosimd "$file" "$tmp_dir/reference"
    for mode in $MODES; do
        run_mode "$mode" "$file" "$tmp_dir/check"
        if ! cmp "$tmp_dir/reference" "$tmp_dir/check"; then
            echo "$name/$mode output mismatch" >&2
            exit 1
        fi

        warmup=0
        while [ "$warmup" -lt "$WARMUPS" ]; do
            run_mode "$mode" "$file" /dev/null
            warmup=$((warmup + 1))
        done

        : >"$tmp_dir/times"
        run=0
        while [ "$run" -lt "$RUNS" ]; do
            t0=$(now_ns)
            run_mode "$mode" "$file" /dev/null
            t1=$(now_ns)
            echo $((t1 - t0)) >>"$tmp_dir/times"
            run=$((run + 1))
        done

        sort -n "$tmp_dir/times" >"$tmp_dir/sorted"
        middle=$((RUNS / 2 + 1))
        min=$(sed -n '1p' "$tmp_dir/sorted")
        med=$(sed -n "${middle}p" "$tmp_dir/sorted")
        max=$(sed -n "${RUNS}p" "$tmp_dir/sorted")
        awk -v n="$name" -v m="$mode" -v med="$med" \
            -v min="$min" -v max="$max" \
            'BEGIN { printf "%s,%s,%.3f,%.3f,%.3f\n", n, m, med/1000000, min/1000000, max/1000000 }'
    done
done
