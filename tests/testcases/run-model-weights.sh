#!/bin/sh

set -eu

NOCT=${NOCT:-../../build-static/noct}
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
NOCT=$(CDPATH= cd -- "$(dirname "$NOCT")" && pwd)/$(basename "$NOCT")
TMP=$(mktemp -d)
trap 'rm -rf "$TMP"' EXIT HUP INT TERM

echo 'Binary/Hash/NWT1 tests:'
python3 "$ROOT/tests/testcases/model-weights/make-nwt1-fixtures.py" "$TMP/fixtures"
python3 "$ROOT/tests/testcases/model-weights/make-nwt1-fixtures.py" "$TMP/fixtures-2"
cmp "$TMP/fixtures/valid.nwt1" "$TMP/fixtures-2/valid.nwt1"
cmp "$TMP/fixtures/errors.tsv" "$TMP/fixtures-2/errors.tsv"

for mode in '-j0' '-j'; do
	"$NOCT" "$mode" "$ROOT/tests/testcases/model-weights/binary-ok.noct" \
		"$TMP/floats.f32le" "$TMP/exact.bin" > "$TMP/binary.out"
	diff "$ROOT/tests/testcases/model-weights/binary-ok.noct.out" "$TMP/binary.out"
	"$NOCT" "$mode" "$ROOT/tests/testcases/model-weights/nwt1-write-read.noct" \
		"$TMP/written.nwt1" > "$TMP/nwt1.out"
	diff "$ROOT/tests/testcases/model-weights/nwt1-write-read.noct.out" "$TMP/nwt1.out"
done

cp "$ROOT/tests/testcases/model-weights/nwt1-write-read.noct" "$TMP/roundtrip.noct"
"$NOCT" --compile "$TMP/roundtrip.noct"
"$NOCT" -j0 "$TMP/roundtrip.nb" "$TMP/from-nb.nwt1" \
	> "$TMP/roundtrip-nb.out"
diff "$ROOT/tests/testcases/model-weights/nwt1-write-read.noct.out" \
	"$TMP/roundtrip-nb.out"
(
	cd "$TMP"
	"$NOCT" --compile --app roundtrip.nap roundtrip.noct
)
"$NOCT" -j0 "$TMP/roundtrip.nap" "$TMP/from-nap.nwt1" \
	> "$TMP/roundtrip-nap.out"
diff "$ROOT/tests/testcases/model-weights/nwt1-write-read.noct.out" \
	"$TMP/roundtrip-nap.out"

hash=$(sed -n '1p' "$TMP/fixtures/valid.sha256")
"$NOCT" -j0 "$ROOT/tests/testcases/model-weights/weights-gc.noct" \
	"$TMP/fixtures/valid.nwt1" "$hash" > "$TMP/gc.out"
diff "$ROOT/tests/testcases/model-weights/weights-gc.noct.out" "$TMP/gc.out"

tab=$(printf '\t')
while IFS="$tab" read -r name expected message; do
	if "$NOCT" -j0 \
		"$ROOT/tests/testcases/model-weights/weights-open-error.noct" \
		"$TMP/fixtures/$name" "$expected" > "$TMP/error.out" 2>&1; then
		echo "FAIL malformed NWT1 accepted: $name"
		exit 1
	fi
	grep -F "$message" "$TMP/error.out" >/dev/null || {
		echo "FAIL unexpected diagnostic for $name"
		cat "$TMP/error.out"
		exit 1
	}
done < "$TMP/fixtures/errors.tsv"

for spec in \
	'wrong-weight-kind|Weights handle kind mismatch' \
	'wrong-file-kind|File handle kind mismatch' \
	'closed|Weights handle is closed' \
	'index|Weights entry index is out-of-range' \
	'name|Weights entry name mismatch' \
	'shape|Weights entry shape mismatch' \
	'handle-frozen|Dictionary is frozen' \
	'namespace-frozen|Dictionary is frozen'; do
	mode=${spec%%|*}
	message=${spec#*|}
	if "$NOCT" -j0 "$ROOT/tests/testcases/model-weights/weights-errors.noct" \
		"$TMP/fixtures/valid.nwt1" "$hash" "$mode" \
		> "$TMP/error.out" 2>&1; then
		echo "FAIL Weights error case accepted: $mode"
		exit 1
	fi
	grep -F "$message" "$TMP/error.out" >/dev/null
done

for spec in \
	'varint-truncated|Truncated varint' \
	'varint-wide|Varint exceeds 64 bits' \
	'varint-overlong|Overlong varint encoding' \
	'varint-eleven|Varint exceeds 64 bits' \
	'cursor-negative|must be non-negative' \
	'cursor-order|cursor range is out-of-bounds' \
	'fixed-bounds|fixed-width access is out-of-bounds' \
	'u32-range|writeU32LE value is out-of-range' \
	'hash-type|is not a packed' \
	'read-short|Unexpected end of file' \
	'read-negative|byte count must be positive' \
	'file-closed|File is closed' \
	'write-range|writeAll range is out-of-bounds' \
	'float-short|shorter than expected' \
	'float-trailing|trailing bytes' \
	'float-count|element count is out-of-range' \
	'directory-existing|Output directory already exists'; do
	mode=${spec%%|*}
	message=${spec#*|}
	case "$mode" in
		read-short|read-negative|file-closed) path="$TMP/fixtures/three-bytes.bin" ;;
		float-short) path="$TMP/fixtures/short.f32le" ;;
		float-trailing) path="$TMP/fixtures/trailing.f32le" ;;
		directory-existing) path="$TMP/new-output" ;;
		*) path="$TMP/binary-error.bin" ;;
	esac
	if "$NOCT" -j0 "$ROOT/tests/testcases/model-weights/binary-errors.noct" \
		"$mode" "$path" > "$TMP/error.out" 2>&1; then
		echo "FAIL Binary/File error case accepted: $mode"
		exit 1
	fi
	grep -F "$message" "$TMP/error.out" >/dev/null
done

${CC:-cc} ${TEST_CFLAGS:-} -I"$ROOT/include" -Wall -Wextra -Werror \
	"$ROOT/tests/testcases/model-weights/weights-vm-test.c" \
	"$(dirname "$NOCT")/libnoctapi.a" \
	"$(dirname "$NOCT")/libnoct.a" -lm ${TEST_LIBS:-} ${TEST_LDFLAGS:-} \
	-o "$TMP/weights-vm-test"
"$TMP/weights-vm-test" "$TMP/fixtures/valid.nwt1" "$hash"

echo 'All Binary/Hash/NWT1 tests passed.'
