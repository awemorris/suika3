#!/bin/sh

set -eu

SCRIPT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
REPOSITORY_ROOT=$(CDPATH= cd -- "$SCRIPT_DIR/../.." && pwd)
BUILD_ARGUMENT=${1:-build-static}

case "$BUILD_ARGUMENT" in
/*)
	BUILD_DIR=$BUILD_ARGUMENT
	;;
*)
	BUILD_DIR=$REPOSITORY_ROOT/$BUILD_ARGUMENT
	;;
esac

BUILD_DIR=$(CDPATH= cd -- "$BUILD_DIR" && pwd)
NOCT=$BUILD_DIR/noct
TEST_TMP_ROOT=$BUILD_DIR/test-tmp
mkdir -p "$TEST_TMP_ROOT"
TMP=$(mktemp -d "$TEST_TMP_ROOT/bytecode-require.XXXXXX")
trap 'rm -rf "$TMP"' EXIT HUP INT TERM
export TMPDIR=$TMP

${CMAKE:-cmake} --build "$BUILD_DIR" \
	--target noctcli noct-test-require-core

"$BUILD_DIR/noct-test-require-core"

cp -R "$SCRIPT_DIR/app/require" "$TMP/require"
MODULES=$TMP/require/modules

# Build dependency bytecode first, then the root while --path resolves source.
"$NOCT" --compile --path="$MODULES" "$MODULES/leaf.nct"
"$NOCT" --compile --path="$MODULES" "$MODULES/middle.noct"
"$NOCT" --compile --path="$MODULES" "$TMP/require/main.noct"

test -f "$MODULES/leaf.nbc"
test -f "$MODULES/middle.nbc"
test -f "$TMP/require/main.nbc"
grep -F 'Noct Bytecode 1.1' "$TMP/require/main.nbc" >/dev/null

# Exercise a source root with source dependencies.
"$NOCT" -j0 --path="$MODULES" "$TMP/require/main.noct" \
	> "$TMP/source-source.out"
diff -u "$TMP/require/require.out" "$TMP/source-source.out"

# Exercise a bytecode root with source dependencies.
"$NOCT" -j0 --path="$MODULES" "$TMP/require/main.nbc" \
	> "$TMP/bytecode-source.out"
diff -u "$TMP/require/require.out" "$TMP/bytecode-source.out"

# Exercise the same root with only bytecode dependency artifacts visible.
mkdir "$TMP/cache"
cp "$MODULES/leaf.nbc" "$TMP/cache/leaf.nbc"
cp "$MODULES/middle.nbc" "$TMP/cache/middle.nbc"
"$NOCT" -j0 --path="$TMP/cache" "$TMP/require/main.noct" \
	> "$TMP/source-bytecode.out"
diff -u "$TMP/require/require.out" "$TMP/source-bytecode.out"
"$NOCT" -j0 --path="$TMP/cache" "$TMP/require/main.nbc" \
	> "$TMP/bytecode-bytecode.out"
diff -u "$TMP/require/require.out" "$TMP/bytecode-bytecode.out"

# Package source and bytecode roots independently into self-contained apps.
"$NOCT" --compile --app --path="$MODULES" \
	"$TMP/source.nap" "$TMP/require/main.noct"
"$NOCT" --compile --app --path="$TMP/cache" \
	"$TMP/bytecode.nap" "$TMP/require/main.nbc"

mkdir "$TMP/isolated"
cp "$TMP/source.nap" "$TMP/isolated/source.nap"
cp "$TMP/bytecode.nap" "$TMP/isolated/bytecode.nap"

(cd "$TMP/isolated" && "$NOCT" -j0 source.nap) \
	> "$TMP/source-app-interpreter.out"
(cd "$TMP/isolated" && "$NOCT" -j bytecode.nap) \
	> "$TMP/bytecode-app-jit.out"
diff -u "$TMP/require/require.out" "$TMP/source-app-interpreter.out"
diff -u "$TMP/require/require.out" "$TMP/bytecode-app-jit.out"

# Preserve an already portable Source identity when packaging static links.
mkdir "$TMP/static"
cp "$SCRIPT_DIR/bytecode-require/static.noct" "$TMP/static/static.noct"
(
	cd "$TMP/static"
	"$NOCT" --compile static.noct
	"$NOCT" --compile --app static.nap static.nbc
	"$NOCT" -j0 static.nap > static.out
)
diff -u "$SCRIPT_DIR/bytecode-require/static.noct.out" \
	"$TMP/static/static.out"

# Compile mode must recognize and explicitly reject accelerator selection.
if "$NOCT" --compile --gpu "$TMP/require/main.noct" \
	> "$TMP/gpu.log" 2>&1; then
	echo 'compile mode accepted --gpu' >&2
	exit 1
fi
grep -F 'GPU acceleration is available only when running Noct source.' \
	"$TMP/gpu.log" >/dev/null

# Detect output-name collisions before creating either mapped output.
cp "$TMP/require/main.noct" "$TMP/collision.noct"
cp "$TMP/require/main.noct" "$TMP/collision.nct"
if "$NOCT" --compile "$TMP/collision.noct" "$TMP/collision.nct" \
	> "$TMP/collision.log" 2>&1; then
	echo 'duplicate mapped output was accepted' >&2
	exit 1
fi
grep -F 'map to the same output' "$TMP/collision.log" >/dev/null
test ! -e "$TMP/collision.nbc"

# An application output must not replace any transitive graph input.
if "$NOCT" --compile --app --path="$MODULES" \
	"$MODULES/leaf.nct" "$TMP/require/main.noct" \
	> "$TMP/app-collision.log" 2>&1; then
	echo 'application output replaced a transitive input' >&2
	exit 1
fi
grep -F 'collides with input' "$TMP/app-collision.log" >/dev/null
cmp "$SCRIPT_DIR/app/require/modules/leaf.nct" "$MODULES/leaf.nct"

# Reject a closure-wide link-name collision before opening the writer.
cp -R "$SCRIPT_DIR/bytecode-require/duplicate" "$TMP/duplicate"
if "$NOCT" --compile --path="$TMP/duplicate/modules" \
	"$TMP/duplicate/main.noct" > "$TMP/duplicate.log" 2>&1; then
	echo 'duplicate closure function was accepted' >&2
	exit 1
fi
grep -F 'Duplicate function in module closure' "$TMP/duplicate.log" \
	>/dev/null
test ! -e "$TMP/duplicate/main.nbc"

echo 'PASS bytecode require and CPU app'
