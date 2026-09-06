#!/bin/sh

set -eu

case_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
root=$(CDPATH= cd -- "$case_dir/../.." && pwd)
build_arg=${1:-build-static}
configuration=${2:-Release}

case "$build_arg" in
/*)
    build_dir=$build_arg
    ;;
*)
    build_dir=$root/$build_arg
    ;;
esac

${CMAKE:-cmake} --build "$build_dir" --config "$configuration" \
    --target noctcli

if test -x "$build_dir/$configuration/noct.exe"; then
    NOCT=${NOCT:-"$build_dir/$configuration/noct.exe"}
elif test -x "$build_dir/noct.exe"; then
    NOCT=${NOCT:-"$build_dir/noct.exe"}
else
    NOCT=${NOCT:-"$build_dir/noct"}
fi

tmp_root=${TMPDIR:-"$build_dir/test-tmp"}
mkdir -p "$tmp_root"
work=$(mktemp -d "$tmp_root/cli-options.XXXXXX")
trap 'rm -rf "$work"' EXIT HUP INT TERM

cd "$case_dir"

echo 'CLI optimization-level tests:'

for option in -O -O0 -O1 -O2 -O3 -O9; do
    $NOCT "$option" -j0 simd/f32.noct > "$work/out"
    diff simd/f32.noct.out "$work/out"
done

# Compile-mode option order is observable through O3-only FMA metadata.
cp simd/drawimage/blend-alpha.noct "$work/blend-alpha.noct"
$NOCT --compile -O3 -O2 "$work/blend-alpha.noct"
if grep -a -q '^FMA Ops$' "$work/blend-alpha.nbc"; then
    echo 'last-option-wins failed for -O3 -O2' >&2
    exit 1
fi
$NOCT --compile -O2 -O3 "$work/blend-alpha.noct"
grep -a -q '^FMA Ops$' "$work/blend-alpha.nbc"

$NOCT --ansic -O3 "$work/f32.c" simd/f32.noct
test -s "$work/f32.c"

# -j is eager: even a once-called entry point is compiled while registering.
NOCT_JIT_DEBUG=1 $NOCT -j -O0 simd/f32.noct \
    > "$work/out" 2> "$work/jit-debug"
grep -q '^noct-jit: .*: compiled$' "$work/jit-debug"
NOCT_JIT_DEBUG=1 $NOCT -j0 -O0 simd/f32.noct \
    > "$work/out" 2> "$work/no-jit-debug"
test ! -s "$work/no-jit-debug"

# Bare -O keeps LINEINFO, while the numbered optimized preset drops it.
$NOCT -j0 -O typing/anno_violate.noct \
    > "$work/lineinfo" 2>&1 || true
grep -q ':5: Error:' "$work/lineinfo"
$NOCT -j0 -O1 typing/anno_violate.noct \
    > "$work/no-lineinfo" 2>&1 || true
grep -q ':0: Error:' "$work/no-lineinfo"

for option in -O4 -O2foo --optimize-level= \
              --optimize-level=-1 --optimize-level=x \
              --optimize-level=999999999999999999999999; do
    if $NOCT "$option" -j0 simd/f32.noct \
        > "$work/invalid.out" 2>&1; then
        echo "invalid option accepted: $option" >&2
        exit 1
    fi
    grep -q 'Invalid optimize-level option' "$work/invalid.out"
done

for option in --disable-jit --force-jit --jit-threshold=0 \
              --optimize-level=2; do
    if $NOCT "$option" -j0 simd/f32.noct > "$work/removed.out" 2>&1; then
        echo "removed/invalid option accepted: $option" >&2
        exit 1
    fi
done

echo 'CLI accelerator option tests:'

printf 'func main() { }\n' > "$work/main.noct"
"$NOCT" --compile "$work/main.noct"
"$NOCT" --compile --app "$work/main.nap" "$work/main.noct"

if "$NOCT" --gpu-list > "$work/gpu-list.out" 2>&1; then
    echo 'accelerator-disabled build accepted --gpu-list' >&2
    exit 1
fi
grep -q 'GPU acceleration is not available in this build.' \
    "$work/gpu-list.out"

if "$NOCT" --gpu "$work/main.noct" > "$work/gpu.out" 2>&1; then
    echo 'accelerator-disabled build accepted --gpu' >&2
    exit 1
fi
grep -q 'GPU acceleration is not available in this build.' "$work/gpu.out"

if "$NOCT" --gpu=Noct-Test "$work/main.noct" \
        > "$work/gpu-name.out" 2>&1; then
    echo 'accelerator-disabled build accepted --gpu=NAME' >&2
    exit 1
fi
grep -q 'GPU acceleration is not available in this build.' \
    "$work/gpu-name.out"

if "$NOCT" --gpu -e 'return' > "$work/gpu-oneliner.out" 2>&1; then
    echo 'accelerator-disabled build accepted --gpu with -e' >&2
    exit 1
fi
grep -q 'GPU acceleration is not available in this build.' \
    "$work/gpu-oneliner.out"

if "$NOCT" --gpu= "$work/main.noct" > "$work/gpu-empty.out" 2>&1; then
    echo 'empty GPU name was accepted' >&2
    exit 1
fi
grep -q 'GPU device name must not be empty.' "$work/gpu-empty.out"

if "$NOCT" --gpu --gpu "$work/main.noct" \
        > "$work/gpu-duplicate.out" 2>&1; then
    echo 'duplicate GPU selection was accepted' >&2
    exit 1
fi
grep -q 'GPU acceleration may be selected only once.' \
    "$work/gpu-duplicate.out"

for input in "$work/main.nbc" "$work/main.nap"; do
    if "$NOCT" --gpu "$input" > "$work/gpu-binary.out" 2>&1; then
        echo "--gpu accepted non-source input: $input" >&2
        exit 1
    fi
    grep -q 'GPU acceleration is available only when running Noct source.' \
        "$work/gpu-binary.out"
done

if "$NOCT" --compile --gpu "$work/main.noct" \
        > "$work/gpu-compile.out" 2>&1; then
    echo '--compile accepted --gpu' >&2
    exit 1
fi
grep -q 'GPU acceleration is available only when running Noct source.' \
    "$work/gpu-compile.out"

if "$NOCT" --compile --gpu-list "$work/main.noct" \
        > "$work/gpu-list-compile.out" 2>&1; then
    echo '--compile accepted --gpu-list' >&2
    exit 1
fi
grep -q 'GPU acceleration is available only when running Noct source.' \
    "$work/gpu-list-compile.out"

if "$NOCT" --compile --app --gpu "$work/rejected.nap" \
        "$work/main.noct" > "$work/gpu-app.out" 2>&1; then
    echo '--compile --app accepted --gpu' >&2
    exit 1
fi
grep -q 'GPU acceleration is available only when running Noct source.' \
    "$work/gpu-app.out"

printf 'Noct Bytecode 9.9\n' > "$work/unknown.nbc"
if "$NOCT" --gpu "$work/unknown.nbc" \
        > "$work/gpu-unknown-bytecode.out" 2>&1; then
    echo '--gpu accepted an unknown bytecode family version' >&2
    exit 1
fi
grep -q 'Unsupported or malformed bytecode version.' \
    "$work/gpu-unknown-bytecode.out"

printf 'Noct App 9.9\n' > "$work/unknown.nap"
if "$NOCT" --gpu "$work/unknown.nap" \
        > "$work/gpu-unknown-app.out" 2>&1; then
    echo '--gpu accepted an unknown application family version' >&2
    exit 1
fi
grep -q 'Unsupported or malformed application version.' \
    "$work/gpu-unknown-app.out"

case $(uname -s 2>/dev/null || echo unknown) in
MINGW*|MSYS*|CYGWIN*)
    ${CMAKE:-cmake} --build "$build_dir" --config "$configuration" \
        --target noct-test-cli-win32-gpu-name
    if test -x "$build_dir/$configuration/noct-test-cli-win32-gpu-name.exe"; then
        "$build_dir/$configuration/noct-test-cli-win32-gpu-name.exe"
    else
        "$build_dir/noct-test-cli-win32-gpu-name.exe"
    fi
    ;;
esac

echo 'All CLI optimization-level tests passed.'
