#!/bin/sh

set -eu

ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
NOCT=${NOCT:-$ROOT/build-linux-opengl/noct}
NOCT_OPENGL_RENDERER_PATTERN=${NOCT_OPENGL_RENDERER_PATTERN:-Intel}
EGL_PLATFORM=${EGL_PLATFORM:-surfaceless}
export EGL_PLATFORM

BUILD_TMP=$(mktemp -d "$ROOT/tests/testcases/.accel-serialization.XXXXXX")
RELOCATED=$(mktemp -d)
trap 'rm -rf "$BUILD_TMP" "$RELOCATED"' EXIT HUP INT TERM
BUILD_REL=${BUILD_TMP#"$ROOT/"}

echo 'Raw GPU bytecode/app serialization hardening tests:'
cp "$ROOT/tests/testcases/accel/gpu-loop-accumulator.noct" "$BUILD_TMP/model.noct"
(
	cd "$ROOT"
	"$NOCT" --compile -O2 "$BUILD_REL/model.noct"
	"$NOCT" --compile --app -O2 \
		"$BUILD_REL/model.nap" "$BUILD_REL/model.noct"
)
cp "$BUILD_TMP/model.nb" "$RELOCATED/model.nb"
cp "$BUILD_TMP/model.nap" "$RELOCATED/model.nap"

check_run() {
	artifact=$1
	NOCT_ACCEL_DEBUG=1 "$NOCT" --accel=opengl --accel-info -j \
		"$artifact" > "$RELOCATED/out" 2> "$RELOCATED/accel.log"
	diff "$ROOT/tests/testcases/accel/gpu-loop-accumulator.noct.out" "$RELOCATED/out"
	grep -F 'ACCEL: OpenGL device:' "$RELOCATED/accel.log" >/dev/null
	if grep -E 'ACCEL: OpenGL device:.*(llvmpipe|softpipe|Software Rasterizer)' \
		"$RELOCATED/accel.log" >/dev/null; then
		echo 'FAIL relocated raw GPU artifact selected software rendering'
		exit 1
	fi
	if [ -n "$NOCT_OPENGL_RENDERER_PATTERN" ]; then
		grep -E "$NOCT_OPENGL_RENDERER_PATTERN" \
			"$RELOCATED/accel.log" >/dev/null
	fi
	if grep -F 'CPU fallback' "$RELOCATED/accel.log" >/dev/null; then
		echo 'FAIL relocated raw GPU artifact used CPU fallback'
		exit 1
	fi
}

check_run "$RELOCATED/model.nb"
check_run "$RELOCATED/model.nap"

for kind in nb nap; do
	size=$(wc -c < "$RELOCATED/model.$kind")
	dd if="$RELOCATED/model.$kind" of="$RELOCATED/truncated.$kind" \
		bs=1 count=$((size - 1)) status=none
	if "$NOCT" --accel=opengl -j0 "$RELOCATED/truncated.$kind" \
		> "$RELOCATED/rejected.out" 2>&1; then
		echo "FAIL truncated raw GPU .$kind artifact executed"
		exit 1
	fi
done

echo 'All raw GPU bytecode/app serialization hardening tests passed.'
