#!/bin/sh

#
# Type-annotation test suite (docs/design/02-typing.md).
# Annotations are inert at -O0; -O/-O1 adds typed entry checks, while
# return-value trust/checking remains level 2. A case may provide out2 as
# golden (used by the intentional violation case).
#

NOCT=${NOCT:-../../build-static/noct}

echo 'Typing tests:'

FAILED=0
TMP_DIR=$(mktemp -d)
OUT="$TMP_DIR/out"
trap 'rm -rf -- "$TMP_DIR"' EXIT HUP INT TERM
for tc in typing/*.noct; do
    for lvl in "-O0" "-O2"; do
        golden="$tc.out"
        if [ "$lvl" = "-O2" ] && [ -f "$tc.out2" ]; then
            golden="$tc.out2"
        fi
        for jit in "-j0" "-j"; do
            $NOCT $jit $lvl "$tc" > "$OUT" 2>&1
            if ! diff -q "$golden" "$OUT" > /dev/null 2>&1; then
                echo "FAIL $tc ($jit $lvl)"
                diff "$golden" "$OUT" | head -5
                FAILED=1
            fi
        done
    done
    echo "PASS $tc"
done

# Exercise the strict bytecode metadata reader with the new packed/restrict
# parameter sections.  The compiler places the .nbc beside its input.
cp typing/anno_packed_types.noct "$TMP_DIR/packed_roundtrip.noct"
$NOCT --compile "$TMP_DIR/packed_roundtrip.noct"
$NOCT -j0 "$TMP_DIR/packed_roundtrip.nbc" > "$OUT" 2>&1
if ! diff -q typing/anno_packed_types.noct.out "$OUT" > /dev/null 2>&1; then
    echo "FAIL packed/restrict bytecode round trip"
    diff typing/anno_packed_types.noct.out "$OUT" | head -5
    FAILED=1
else
    echo "PASS packed/restrict bytecode round trip"
fi

# Return annotations use an optional bytecode section.  Compile and reload it
# explicitly so the strict line-sequence reader stays backward compatible.
cp typing/return_annotation.noct "$TMP_DIR/return_roundtrip.noct"
$NOCT --compile "$TMP_DIR/return_roundtrip.noct"
$NOCT -j0 "$TMP_DIR/return_roundtrip.nbc" > "$OUT" 2>&1
if ! diff -q typing/return_annotation.noct.out "$OUT" > /dev/null 2>&1; then
    echo "FAIL return annotation bytecode round trip"
    diff typing/return_annotation.noct.out "$OUT" | head -5
    FAILED=1
else
    echo "PASS return annotation bytecode round trip"
fi

cp typing/return_unknown.noct "$TMP_DIR/return_checked_roundtrip.noct"
$NOCT --compile -O2 "$TMP_DIR/return_checked_roundtrip.noct"
$NOCT -j0 "$TMP_DIR/return_checked_roundtrip.nbc" > "$OUT" 2>&1
if ! diff -q typing/return_unknown.noct.out2 "$OUT" > /dev/null 2>&1; then
    echo "FAIL checked return bytecode round trip"
    diff typing/return_unknown.noct.out2 "$OUT" | head -5
    FAILED=1
else
    echo "PASS checked return bytecode round trip"
fi

if [ "$FAILED" -ne 0 ]; then
    echo 'Typing tests failed.'
    exit 1
fi
echo 'All typing tests passed.'
