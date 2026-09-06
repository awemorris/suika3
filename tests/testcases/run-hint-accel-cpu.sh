#!/bin/sh

set -eu

root=$(CDPATH= cd -- "$(dirname -- "$0")/../.." && pwd)
build_dir=${1:-build-static}
case "$build_dir" in
/*) ;;
*) build_dir="$root/$build_dir" ;;
esac

noct=${NOCT:-"$build_dir/noct"}
cc=${CC:-cc}
tmp_root=${TMPDIR:-"$build_dir/test-tmp"}

mkdir -p "$tmp_root"
TMPDIR=$tmp_root
export TMPDIR
work=$(mktemp -d "$tmp_root/hint-accel-cpu.XXXXXX")
trap 'rm -rf "$work"' EXIT HUP INT TERM

cd "$root"

${CMAKE:-cmake} --build "$build_dir" \
    --target noctcli noct-test-accel-hint-cpu
"$build_dir/noct-test-accel-hint-cpu"

run_case()
{
    source=$1
    expected=$2

    for options in '-j0 -O0' '-j -O0' \
                   '-j0 -O1' '-j -O1' \
                   '-j0 -O2' '-j -O2'; do
        # shellcheck disable=SC2086
        "$noct" $options "$source" > "$work/actual"
        diff -u "$expected" "$work/actual"
    done
}

reject_source()
{
    source=$1

    if "$noct" -j0 "$source" > "$work/rejected.out" 2>&1; then
        echo "Expected syntax rejection: $source" >&2
        exit 1
    fi
}

basic=tests/testcases/accel-hint-cpu/basic.noct
basic_out=tests/testcases/accel-hint-cpu/basic.noct.out
static=tests/testcases/accel-hint-cpu/static.noct
static_out=tests/testcases/accel-hint-cpu/static.noct.out

run_case "$basic" "$basic_out"
run_case "$static" "$static_out"

for rejected in reject-var.noct reject-let.noct \
                reject-internal-package.noct reject-modifiers.noct; do
    reject_source "$root/tests/testcases/accel-hint-cpu/$rejected"
done

printf '%s\n' '__fast __accel func invalid(): void { return; }' \
    > "$work/reject-fast-accel.noct"
printf '%s\n' 'static __inline __accel func invalid(): void { return; }' \
    > "$work/reject-inline-accel.noct"
printf '%s\n' '__gpu func invalid(): void { return; }' \
    > "$work/reject-gpu.noct"
printf '%s\n' '__accel func invalid(data: rpackedfloat_in): void { return; }' \
    > "$work/reject-transport-type.noct"
reject_source "$work/reject-fast-accel.noct"
reject_source "$work/reject-inline-accel.noct"
reject_source "$work/reject-gpu.noct"
reject_source "$work/reject-transport-type.noct"

cp "$basic" "$work/basic.noct"
"$noct" --compile "$work/basic.noct"
test -f "$work/basic.nbc"
if grep -a -E '__accel|__Accel|GPU' "$work/basic.nbc" >/dev/null; then
    echo 'Accelerator metadata leaked into CPU bytecode.' >&2
    exit 1
fi
"$noct" -j0 "$work/basic.nbc" > "$work/basic-nbc-j0.out"
diff -u "$basic_out" "$work/basic-nbc-j0.out"
"$noct" -j "$work/basic.nbc" > "$work/basic-nbc-jit.out"
diff -u "$basic_out" "$work/basic-nbc-jit.out"

"$noct" --compile --app "$work/basic.nap" "$work/basic.noct"
if grep -a -E '__accel|__Accel|GPU' "$work/basic.nap" >/dev/null; then
    echo 'Accelerator metadata leaked into the CPU app.' >&2
    exit 1
fi
"$noct" -j0 "$work/basic.nap" > "$work/basic-nap-j0.out"
diff -u "$basic_out" "$work/basic-nap-j0.out"
"$noct" -j "$work/basic.nap" > "$work/basic-nap-jit.out"
diff -u "$basic_out" "$work/basic-nap-jit.out"

"$noct" --ansic -O2 "$work/basic.c" "$basic"
if grep -E '__accel|__Accel|GPU' "$work/basic.c" >/dev/null; then
    echo 'Accelerator metadata leaked into generated C.' >&2
    exit 1
fi
"$cc" -I"$root/include" \
    "$root/tests/testcases/ctrans-test.c" "$work/basic.c" \
    "$build_dir/libnoctapi.a" "$build_dir/libnoct.a" -lm \
    -o "$work/basic-aot"
"$work/basic-aot" > "$work/basic-aot.out"
diff -u "$basic_out" "$work/basic-aot.out"

echo 'All accelerator CPU hint tests passed.'
