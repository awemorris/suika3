#!/bin/sh

# Run the mixed alpha-blend SIMD/JIT test under qemu-user.
# Usage: ./run-simd-qemu.sh arm64 /path/to/noct [/path/to/sysroot]
# NOCT_HOST may select the optimizer-enabled host compiler.

set -eu

if [ "$#" -lt 2 ] || [ "$#" -gt 3 ]; then
    echo "usage: $0 {x86|arm32|arm64|ppc32|ppc64|mips32|mips64|riscv32|riscv64} NOCT_BINARY [SYSROOT]" >&2
    exit 2
fi

arch=$1
noct=$2
sysroot=${3:-}

case "$noct" in
/*) ;;
*) noct=$(CDPATH= cd -- "$(dirname -- "$noct")" && pwd)/$(basename -- "$noct") ;;
esac
if [ -n "$sysroot" ]; then
    sysroot=$(CDPATH= cd -- "$sysroot" && pwd)
fi

test_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
repo_dir=$(CDPATH= cd -- "$test_dir/../.." && pwd)
host_noct=${NOCT_HOST:-"$repo_dir/build-static/noct"}
if [ ! -x "$host_noct" ]; then
    echo "optimizer-enabled host Noct not found: $host_noct" >&2
    exit 2
fi
cd "$test_dir"

case "$arch" in
x86) qemu=${QEMU:-qemu-i386}; cpu=${QEMU_CPU:-max}; tiers="scalar sse2 sse3 sse41" ;;
arm32) qemu=${QEMU:-qemu-arm}; cpu=${QEMU_CPU:-cortex-a15}; tiers="scalar neon" ;;
arm64) qemu=${QEMU:-qemu-aarch64}; cpu=${QEMU_CPU:-max}; tiers="scalar neon" ;;
ppc32) qemu=${QEMU:-qemu-ppc}; cpu=${QEMU_CPU:-g4}; tiers="scalar altivec" ;;
ppc64) qemu=${QEMU:-qemu-ppc64le}; cpu=${QEMU_CPU:-POWER8}; tiers="scalar altivec" ;;
mips32) qemu=${QEMU:-qemu-mips}; cpu=${QEMU_CPU:-24Kf}; tiers="scalar" ;;
mips64) qemu=${QEMU:-qemu-mips64}; cpu=${QEMU_CPU:-MIPS64R2-generic}; tiers="scalar" ;;
riscv32) qemu=${QEMU:-qemu-riscv32}; cpu=${QEMU_CPU:-rv32}; tiers="scalar" ;;
riscv64) qemu=${QEMU:-qemu-riscv64}; cpu=${QEMU_CPU:-rv64}; tiers="scalar" ;;
*) echo "unsupported architecture: $arch" >&2; exit 2 ;;
esac

qemu_args="-cpu $cpu"
if [ -n "$sysroot" ]; then
    qemu_args="$qemu_args -L $sysroot"
fi

tmp_dir=$(mktemp -d)
trap 'rm -rf -- "$tmp_dir"' EXIT HUP INT TERM
cp simd/drawimage/blend-alpha.noct "$tmp_dir/blend-alpha.noct"
"$host_noct" --compile -O2 "$tmp_dir/blend-alpha.noct" >/dev/null 2>&1
alpha_nb="$tmp_dir/blend-alpha.nbc"

run_case() {
    ceiling=$1
    env NOCT_JIT_SIMD_MAX=$ceiling \
        "$qemu" $qemu_args "$noct" -j -O2 \
        "$alpha_nb" > "$tmp_dir/$ceiling.out" 2>&1
    diff -u simd/drawimage/blend-alpha.noct.out "$tmp_dir/$ceiling.out"
}

for ceiling in $tiers; do
    run_case "$ceiling"
done

native_tier=${tiers##* }

env NOCT_JIT_SIMD_MAX=$native_tier NOCT_JIT_SIMD_DEBUG=1 \
    "$qemu" $qemu_args "$noct" -j -O2 \
    "$alpha_nb" > /dev/null 2> "$tmp_dir/caps"
if ! grep -q 'vector=1' "$tmp_dir/caps"; then
    echo "JIT did not report vector bytecode for $arch" >&2
    cat "$tmp_dir/caps" >&2
    exit 1
fi

echo "qemu-user SIMD test passed: $arch ($tiers)"
