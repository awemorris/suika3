#!/bin/sh

set -eu

case_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
root=$(CDPATH= cd -- "$case_dir/../.." && pwd)
build_dir=${1:-"$root/build-debug"}
case "$build_dir" in
/*) ;;
*) build_dir="$root/$build_dir" ;;
esac
noct=${NOCT:-"$build_dir/noct"}
cc=${CC:-cc}
tmp=${TMPDIR:-/tmp}/noct-dynlib-test.$$

cleanup()
{
	rm -rf "$tmp"
}
trap cleanup EXIT HUP INT TERM
mkdir -p "$tmp"

system_libs='-lm'
case "$(uname -s)" in
Darwin)
	shared_flags='-dynamiclib'
	system_libs="$system_libs -pthread"
	good_name=libgood.dylib
	fail_name=libfail.dylib
	missing_name=libmissing_init.dylib
	;;
MINGW*|MSYS*|CYGWIN*)
	shared_flags='-shared'
	good_name=good.dll
	fail_name=fail.dll
	missing_name=missing_init.dll
	;;
*)
	shared_flags='-shared -fPIC'
	system_libs="$system_libs -ldl -lutil -pthread"
	good_name=libgood.so
	fail_name=libfail.so
	missing_name=libmissing_init.so
	;;
esac
if command -v pkg-config >/dev/null 2>&1 &&
   pkg-config --exists egl glesv2; then
	system_libs="$system_libs $(pkg-config --libs egl glesv2)"
fi

# shellcheck disable=SC2086
"$cc" $shared_flags -I"$root/include" "$case_dir/dynlib/good.c" \
	-o "$tmp/$good_name"
# shellcheck disable=SC2086
"$cc" $shared_flags -I"$root/include" "$case_dir/dynlib/fail.c" \
	-o "$tmp/$fail_name"
# shellcheck disable=SC2086
"$cc" $shared_flags "$case_dir/dynlib/missing-init.c" \
	-o "$tmp/$missing_name"

echo 'Dynamic-library tests'
	(cd "$tmp" && NOCT_DYNLIB_FINALIZER_MARKER="$tmp/finalized" \
		"$noct" "$case_dir/dynlib/load.noct") \
	>"$tmp/out" 2>"$tmp/err"
diff "$case_dir/dynlib/load.noct.out" "$tmp/out"
test "$(cat "$tmp/finalized")" = finalized

mkdir -p "$tmp/package-home/.noct/packages/nativepkg" "$tmp/empty"
cp "$case_dir/dynlib/nativepkg.noct" \
	"$tmp/package-home/.noct/packages/nativepkg/nativepkg.noct"
cp "$tmp/$good_name" \
	"$tmp/package-home/.noct/packages/nativepkg/$good_name"
(cd "$tmp/empty" && HOME="$tmp/package-home" USERPROFILE="$tmp/package-home" \
	NOCT_DYNLIB_FINALIZER_MARKER="$tmp/finalized-package" \
	"$noct" "$case_dir/dynlib/package-native.noct") >"$tmp/package.out"
test "$(cat "$tmp/package.out")" = 42
test "$(cat "$tmp/finalized-package")" = finalized

# The C host keeps one process alive across two VM instances. This verifies
# per-VM reinitialization, process-lifetime handles, transactional rollback,
# and that an optional load does not hide a malformed library.
# shellcheck disable=SC2086
"$cc" -I"$root/include" "$case_dir/dynlib-host-test.c" \
	"$build_dir/libnoct.a" $system_libs -o "$tmp/dynlib-host-test"
	(cd "$tmp" && NOCT_DYNLIB_FINALIZER_MARKER="$tmp/finalized-host" \
		./dynlib-host-test)
test "$(wc -l < "$tmp/finalized-host" | tr -d ' ')" = 2

expect_error()
{
	name=$1
	source=$2
	message=$3
	if (cd "$tmp" && "$noct" "$case_dir/dynlib/$source") \
		>"$tmp/out" 2>&1; then
		echo "Expected failure: $name" >&2
		exit 1
	fi
	grep -F "$message" "$tmp/out" >/dev/null || {
		cat "$tmp/out" >&2
		echo "Missing expected error: $message" >&2
		exit 1
	}
}

expect_error rollback load-fail.noct \
	'intentional native-library initialization failure'
expect_error missing-entry load-missing-init.noct 'noct_library_init'
expect_error invalid-name load-invalid.noct 'Invalid native library name'

echo 'All dynamic-library tests passed.'
