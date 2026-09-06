#!/bin/sh

set -eu

NOCT=${NOCT:-../../build-static/noct}
NOCT_NOOPT=${NOCT_NOOPT:-}
FAST_EXPECT_OPTIMIZER=${FAST_EXPECT_OPTIMIZER:-1}
case "$NOCT" in
/*) ;;
*) NOCT=$(CDPATH= cd -- "$(dirname -- "$NOCT")" && pwd)/$(basename -- "$NOCT") ;;
esac
if [ -n "$NOCT_NOOPT" ]; then
    case "$NOCT_NOOPT" in
    /*) ;;
    *) NOCT_NOOPT=$(CDPATH= cd -- "$(dirname -- "$NOCT_NOOPT")" && pwd)/$(basename -- "$NOCT_NOOPT") ;;
    esac
fi
TMP=$(mktemp -d)
trap 'rm -rf "$TMP"' EXIT HUP INT TERM

case "$FAST_EXPECT_OPTIMIZER" in
0) primary_suite=no-optimizer-build ;;
1) primary_suite=optimizer-build ;;
*)
    echo 'FAST_EXPECT_OPTIMIZER must be 0 or 1.' >&2
    exit 2
    ;;
esac

run_ok()
{
    test_noct=$1
    source=$2
    expected=$3
    shift 3

    for options in "$@"; do
        # shellcheck disable=SC2086
        "$test_noct" $options "$source" > "$TMP/out"
        diff -u "$expected" "$TMP/out"
    done
}

check_message()
{
    test_noct=$1
    source=$2
    message=$3
    shift 3

    if "$test_noct" "$@" "$source" > "$TMP/error" 2>&1; then
        echo "Expected $source to fail." >&2
        cat "$TMP/error"
        exit 1
    fi
    if ! grep -F "$message" "$TMP/error" >/dev/null; then
        cat "$TMP/error"
        exit 1
    fi
}

check_runtime_oob()
{
    test_noct=$1
    shift

    check_message \
        "$test_noct" \
        fast/static-oob.noct \
        'out-of-range' \
        "$@"
    if ! grep -F 'entered static-oob main' "$TMP/error" >/dev/null; then
        cat "$TMP/error"
        echo 'The checked out-of-bounds failure did not occur at runtime.' >&2
        exit 1
    fi
}

check_compile_oob()
{
    test_noct=$1
    shift

    check_message \
        "$test_noct" \
        fast/static-oob.noct \
        'provably out of bounds' \
        "$@"
    if grep -F 'entered static-oob main' "$TMP/error" >/dev/null; then
        cat "$TMP/error"
        echo 'The statically out-of-bounds function was executed.' >&2
        exit 1
    fi
}

check_bytecode_rejected()
{
    test_noct=$1
    bytecode=$2

    if "$test_noct" -j0 "$bytecode" > "$TMP/error" 2>&1; then
        echo "Expected $bytecode to be rejected." >&2
        exit 1
    fi
    if ! grep -E \
        'Failed to load bytecode data.|Malformed bytecode module' \
        "$TMP/error" >/dev/null; then
        cat "$TMP/error"
        exit 1
    fi
}

check_counter_reassign_oob()
{
    test_noct=$1
    shift

    check_message \
        "$test_noct" \
        fast/counter-reassign-oob.noct \
        'Index out of bounds.' \
        "$@"
    if ! grep -F 'entered counter-reassign-oob main' "$TMP/error" >/dev/null; then
        cat "$TMP/error"
        echo 'The reassigned-counter failure did not occur at runtime.' >&2
        exit 1
    fi
}

run_functional_suite()
{
    test_noct=$1
    suite_name=$2

    run_ok "$test_noct" fast/exact-shape.noct \
        fast/exact-shape.noct.out \
        '-j0 -O0' '-j -O0' '-j0 -O2' '-j -O2'
    run_ok "$test_noct" fast/intrinsics.noct \
        fast/intrinsics.noct.out \
        '-j0 -O0' '-j -O0' '-j0 -O2' '-j -O2'
    run_ok "$test_noct" fast/guarded-safe.noct \
        fast/guarded-safe.noct.out \
        '-j0 -O0' '-j -O0' '-j0 -O2' '-j -O2'
    run_ok "$test_noct" fast/control-bounded-safe.noct \
        fast/control-bounded-safe.noct.out \
        '-j0 -O0' '-j -O0' \
        '-j0 -O1' '-j -O1' \
        '-j0 -O2' '-j -O2'
    run_ok "$test_noct" fast/simd.noct \
        fast/simd.noct.out \
        '-j0 -O0' '-j -O0' '-j0 -O2' '-j -O2'
    run_ok "$test_noct" fast/restricted-distinct.noct \
        fast/restricted-distinct.noct.out \
        '-j0 -O0' '-j -O0' '-j0 -O2' '-j -O2'
    run_ok "$test_noct" fast/multi-index-compound.noct \
        fast/multi-index-compound.noct.out \
        '-j0 -O0' '-j -O0' '-j0 -O2' '-j -O2'

    echo "PASS __fast execution ($suite_name)"
}

run_require_suite()
{
    test_noct=$1
    suite_name=$2
    expect_optimizer=$3

    for options in '-j0 -O0' '-j -O0' '-j0 -O2' '-j -O2'; do
        # Prototype scanning must not execute the dependency initializer.
        # shellcheck disable=SC2086
        "$test_noct" $options --path=fast/modules \
            fast/required-call.noct > "$TMP/required.out"
        diff -u fast/required-call.noct.out "$TMP/required.out"
    done

    "$test_noct" -j0 -O0 --path=fast/modules \
        fast/required-cycle.noct > "$TMP/required-cycle.out"
    test ! -s "$TMP/required-cycle.out"

    if [ "$expect_optimizer" = 1 ]; then
        check_message \
            "$test_noct" \
            fast/required-cycle.noct \
            'Direct or mutually recursive __fast calls are not supported.' \
            -j0 -O2 --path=fast/modules
    else
        "$test_noct" -j0 -O2 --path=fast/modules \
            fast/required-cycle.noct > "$TMP/required-cycle.out"
        test ! -s "$TMP/required-cycle.out"
    fi

    echo "PASS __fast require prototypes ($suite_name)"
}

run_checked_suite()
{
    test_noct=$1
    suite_name=$2

    for jit in -j0 -j; do
        check_message "$test_noct" fast/axis-oob.noct \
            'Index out of bounds.' "$jit" -O0
        check_message "$test_noct" fast/shape-mismatch.noct \
            'does not match the exact shape' "$jit" -O0
        check_message "$test_noct" fast/wrong-primitive.noct \
            'argument type mismatch' "$jit" -O0
        check_message "$test_noct" fast/wrong-element.noct \
            'argument type mismatch' "$jit" -O0
        check_message "$test_noct" fast/dynamic-zero.noct \
            'shape extents must be positive' "$jit" -O0
        check_message "$test_noct" fast/versioning-oob.noct \
            'out-of-range' "$jit" -O2

        for level in 0 1 2; do
            check_counter_reassign_oob \
                "$test_noct" "$jit" "-O$level"
        done
    done

    echo "PASS __fast checked failures ($suite_name)"
}

run_validation_suite()
{
    test_noct=$1
    suite_name=$2
    expect_optimizer=$3

    check_message "$test_noct" fast/extent-overflow.noct \
        'Decimal integer literal is too large' -j0 -O0
    check_message "$test_noct" fast/long-overflow.noct \
        'Decimal integer literal is too large' -j0 -O0

    for level in 0 1 2 3; do
        if [ "$expect_optimizer" = 1 ] && [ "$level" -ge 1 ]; then
            check_message "$test_noct" fast/bad-global.noct \
                'is not available inside __fast func' -j0 "-O$level"
            check_message "$test_noct" fast/bad-local.noct \
                'requires an exact primitive type annotation' -j0 "-O$level"
            check_message "$test_noct" fast/view-mismatch.noct \
                'shape rank does not match' -j0 "-O$level"
            check_message "$test_noct" fast/mutual-recursion.noct \
                'mutually recursive __fast calls' -j0 "-O$level"
            check_message "$test_noct" fast/fallthrough.noct \
                'must return a value' -j0 "-O$level"
        else
            "$test_noct" -j0 "-O$level" fast/bad-global.noct \
                > "$TMP/bad-global.out"
            printf '5\n' | diff -u - "$TMP/bad-global.out"
            "$test_noct" -j0 "-O$level" fast/bad-local.noct \
                > "$TMP/bad-local.out"
            printf '2\n' | diff -u - "$TMP/bad-local.out"
            "$test_noct" -j0 "-O$level" fast/view-mismatch.noct \
                > "$TMP/view-mismatch.out"
            printf '0\n' | diff -u - "$TMP/view-mismatch.out"
            check_message "$test_noct" fast/mutual-recursion.noct \
                'Stack overflow.' -j0 "-O$level"
            "$test_noct" -j0 "-O$level" fast/fallthrough.noct \
                > "$TMP/fallthrough.out"
            printf '1\n' | diff -u - "$TMP/fallthrough.out"
        fi
        check_message "$test_noct" \
            fast/internal-index-package-immutable.noct \
            'Dictionary is frozen.' -j0 "-O$level"
        check_message "$test_noct" \
            fast/internal-math-package-immutable.noct \
            'Dictionary is frozen.' -j0 "-O$level"
    done

    echo "PASS __fast validation ($suite_name)"
}

run_bytecode_suite()
{
    test_noct=$1
    suite_name=$2
    expect_optimizer=$3
    bytecode_dir="$TMP/bytecode-$suite_name"
    checked_dir="$TMP/bytecode-checked-$suite_name"
    checked_metadata="$TMP/fast-checked-metadata-$suite_name"
    metadata="$TMP/fast-metadata-$suite_name"

    mkdir "$checked_dir"
    cp fast/exact-shape.noct "$checked_dir/exact-shape.noct"
    (cd "$checked_dir" && \
        "$test_noct" --compile -O0 exact-shape.noct)
    sed -n '1,/^Temporary Size$/p' \
        "$checked_dir/exact-shape.nbc" > "$checked_metadata"
    checked_kind=$(awk \
        'previous == "Function Kind" { print; exit } { previous = $0 }' \
        "$checked_metadata")
    checked_signature=$(awk \
        'previous == "Fast Signature" { print; exit } { previous = $0 }' \
        "$checked_metadata")
    if [ -n "$checked_kind" ] || [ -n "$checked_signature" ]; then
        cat "$checked_metadata"
        echo '-O0 bytecode unexpectedly contains fast metadata.' >&2
        exit 1
    fi

    "$test_noct" -j0 "$checked_dir/exact-shape.nbc" \
        > "$TMP/checked-exact-$suite_name.out"
    diff -u fast/exact-shape.noct.out \
        "$TMP/checked-exact-$suite_name.out"

    if [ "$expect_optimizer" = 1 ] && [ -n "$NOCT_NOOPT" ]; then
        "$NOCT_NOOPT" -j0 "$checked_dir/exact-shape.nbc" \
            > "$TMP/noopt-checked-exact.out"
        diff -u fast/exact-shape.noct.out \
            "$TMP/noopt-checked-exact.out"
    fi

    mkdir "$bytecode_dir"
    cp fast/exact-shape.noct "$bytecode_dir/exact-shape.noct"
    cp fast/intrinsics.noct "$bytecode_dir/intrinsics.noct"
    cp fast/restricted-distinct.noct \
        "$bytecode_dir/restricted-distinct.noct"
    (cd "$bytecode_dir" && \
        "$test_noct" --compile -O1 \
            exact-shape.noct \
            intrinsics.noct \
            restricted-distinct.noct)

    sed -n '1,/^Temporary Size$/p' \
        "$bytecode_dir/exact-shape.nbc" > "$metadata"
    function_kind=$(awk \
        'previous == "Function Kind" { print; exit } { previous = $0 }' \
        "$metadata")
    signature_version=$(awk \
        'previous == "Fast Signature" { print; exit } { previous = $0 }' \
        "$metadata")
    void_return=$(awk \
        'previous == "Return Type" { print; exit } { previous = $0 }' \
        "$bytecode_dir/restricted-distinct.nbc")
    if [ "$expect_optimizer" = 1 ]; then
        if [ "$function_kind" != 1 ] || [ "$signature_version" != 1 ]; then
            cat "$metadata"
            echo 'The bytecode does not contain compact kind-1 fast metadata.' >&2
            exit 1
        fi
        if [ "$void_return" != -2 ]; then
            echo 'Fast void bytecode does not contain canonical return metadata.' >&2
            exit 1
        fi
    else
        if [ -n "$function_kind" ] || [ -n "$signature_version" ]; then
            cat "$metadata"
            echo 'Unoptimized bytecode unexpectedly contains fast metadata.' >&2
            exit 1
        fi
        if [ "$void_return" = -2 ]; then
            echo 'Unoptimized bytecode unexpectedly contains fast return metadata.' >&2
            exit 1
        fi
    fi

    "$test_noct" -j0 "$bytecode_dir/exact-shape.nbc" > "$TMP/exact.out"
    "$test_noct" -j0 "$bytecode_dir/intrinsics.nbc" > "$TMP/intrinsics.out"
    "$test_noct" -j0 "$bytecode_dir/restricted-distinct.nbc" \
        > "$TMP/restricted.out"
    diff -u fast/exact-shape.noct.out "$TMP/exact.out"
    diff -u fast/intrinsics.noct.out "$TMP/intrinsics.out"
    diff -u fast/restricted-distinct.noct.out "$TMP/restricted.out"

    if [ "$expect_optimizer" = 1 ] && [ -n "$NOCT_NOOPT" ]; then
        if "$NOCT_NOOPT" -j0 "$bytecode_dir/exact-shape.nbc" \
            > "$TMP/noopt-optimized-bytecode.out" 2>&1; then
            echo 'Optimizer-free runtime accepted optimized __fast bytecode.' >&2
            exit 1
        fi
        if ! grep -F \
            'Optimized __fast bytecode requires optimizer support.' \
            "$TMP/noopt-optimized-bytecode.out" >/dev/null; then
            cat "$TMP/noopt-optimized-bytecode.out"
            exit 1
        fi
    fi

    if [ "$expect_optimizer" = 1 ]; then
        malformed_dir="$bytecode_dir/malformed"
        python3 fast/make-malformed-bytecode.py \
            "$bytecode_dir/exact-shape.nbc" \
            "$malformed_dir"

        for malformed in "$malformed_dir"/*.nbc; do
            check_bytecode_rejected "$test_noct" "$malformed"
        done
    fi

    echo "PASS __fast bytecode ($suite_name)"
}

run_functional_suite "$NOCT" "$primary_suite"
run_require_suite "$NOCT" "$primary_suite" "$FAST_EXPECT_OPTIMIZER"
run_checked_suite "$NOCT" "$primary_suite"
run_validation_suite "$NOCT" "$primary_suite" "$FAST_EXPECT_OPTIMIZER"
run_bytecode_suite "$NOCT" "$primary_suite" "$FAST_EXPECT_OPTIMIZER"

check_runtime_oob "$NOCT" -j0 -O0
check_runtime_oob "$NOCT" -j -O0
if [ "$FAST_EXPECT_OPTIMIZER" = 1 ]; then
    for level in 1 2; do
        check_compile_oob "$NOCT" -j0 "-O$level"
        check_compile_oob "$NOCT" -j "-O$level"
    done
else
    for level in 1 2; do
        check_runtime_oob "$NOCT" -j0 "-O$level"
        check_runtime_oob "$NOCT" -j "-O$level"
    done
fi

if [ "${FAST_EXPECT_SIMD:-0}" = 1 ]; then
    simd_info=$("$NOCT" --simd-info -j0 -O2 fast/simd.noct 2>&1)
    test "$(echo "$simd_info" | grep -c 'vectorized')" -ge 2 || {
        echo "$simd_info"
        echo '__fast contiguous loops were not vectorized' >&2
        exit 1
    }
fi

if [ -n "$NOCT_NOOPT" ]; then
    run_functional_suite "$NOCT_NOOPT" secondary-no-optimizer-build
    run_require_suite "$NOCT_NOOPT" secondary-no-optimizer-build 0
    run_checked_suite "$NOCT_NOOPT" secondary-no-optimizer-build
    run_validation_suite "$NOCT_NOOPT" secondary-no-optimizer-build 0
    run_bytecode_suite "$NOCT_NOOPT" secondary-no-optimizer-build 0

    for level in 0 1 2; do
        check_runtime_oob "$NOCT_NOOPT" -j0 "-O$level"
        check_runtime_oob "$NOCT_NOOPT" -j "-O$level"
    done
fi

echo 'All __fast func tests passed.'
