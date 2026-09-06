#!/bin/sh

# Build and run the pure zedBSD BeUI evdev state engine on the host.  The
# production source is linked directly so this covers the same translation,
# packet-boundary, synchronization, detach, and cleanup logic used by Noct.

set -eu

root=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
zedbsd_root=${ZEDBSD_SOURCE_DIR:-}
if test $# -gt 1; then
	echo "usage: $0 [zedBSD-source-directory]" >&2
	exit 2
fi
if test $# -eq 1; then
	zedbsd_root=$1
fi
if test -z "$zedbsd_root" &&
	test -f "$root/../../include/uapi/zedbsd/input.h"
then
	zedbsd_root=$(CDPATH= cd -- "$root/../.." && pwd)
fi
if test -z "$zedbsd_root" ||
	test ! -f "$zedbsd_root/include/uapi/zedbsd/input.h"
then
	echo "set ZEDBSD_SOURCE_DIR or pass the zedBSD source directory" >&2
	exit 2
fi

cc=${CC:-cc}
output=$(mktemp -d "${TMPDIR:-/tmp}/noct-beui-zedbsd.XXXXXX")
trap 'rm -rf -- "$output"' EXIT HUP INT TERM

common_flags="-std=c11 -Wall -Wextra -Werror"
sanitize_flags=
if test "${NOCT_TEST_SANITIZERS:-1}" != 0; then
	sanitize_flags="-fsanitize=address,undefined -fno-omit-frame-pointer"
fi

# shellcheck disable=SC2086
"$cc" $common_flags $sanitize_flags \
	-I"$root/include" -I"$root/src/api" \
	-I"$zedbsd_root/include/uapi" \
	"$root/tests/testcases/beui-zedbsd-input-test.c" \
	-o "$output/beui-zedbsd-input-test"
if test "${NOCT_TEST_SANITIZERS:-1}" != 0; then
	ASAN_OPTIONS=${ASAN_OPTIONS:-detect_leaks=0}
	export ASAN_OPTIONS
fi
"$output/beui-zedbsd-input-test"

# Wiring/source audit: the target owns one complete backend, the CLI uses the
# sole public registration function, and the implementation uses evdev rather
# than the legacy console event/key-state interface.
grep -q 'option(NOCT_ENABLE_API_BEUI ' "$root/CMakeLists.txt"
grep -q 'src/api/api-beui-zedbsd.c' "$root/CMakeLists.txt"
grep -q '"NOCT_ENABLE_API_BEUI": "ON"' "$root/CMakePresets.json"
grep -q 'noct_register_api_beui(env)' "$root/src/cli/cli-run.c"
grep -q 'noct_register_api_beui(env)' "$root/src/cli/cli-repl.c"
test "$(grep -c '^noct_register_api_beui($' \
	"$root/include/noct/noct.h")" -eq 1
if grep -E 'noct_beui_(bind|init|image_|bmp_)|struct noct_beui_|enum noct_beui_' \
	"$root/include/noct/noct.h" >/dev/null
then
	echo "private BeUI implementation contract found in public header" >&2
	exit 1
fi
for source in cli-run.c cli-repl.c; do
	grep -q '#include <noct/noct.h>' "$root/src/cli/$source"
	if grep -q 'beui-internal.h' "$root/src/cli/$source"; then
		echo "CLI includes private BeUI contract: $source" >&2
		exit 1
	fi
done
for source in api-beui-pc98.c api-beui-sdl2.c api-beui-zedbsd.c; do
	test "$(grep -c '^noct_register_api_beui($' \
		"$root/src/api/$source")" -eq 1
done
grep -q '^  if(NOCT_TARGET_ZEDBSD)$' "$root/CMakeLists.txt"
grep -q '^  elseif(NOCT_TARGET_PC98DOS)$' "$root/CMakeLists.txt"
test ! -e "$root/src/api/api-beui.c"
test ! -e "$root/src/api/api-beui-backend.c"
test ! -e "$root/src/api/beui-internal.h"
test ! -e "$root/src/api/beui-core.c"
test ! -e "$root/src/api/beui-image.c"
test ! -e "$root/include/noct/beui.h"
test ! -e "$root/src/api/beui-zedbsd-input.c"
test ! -e "$root/src/api/beui-zedbsd-input.h"
obsolete_prefix=noct_register_api_beui_
obsolete_registrars="${obsolete_prefix}with_hal\|${obsolete_prefix}zedbsd"
obsolete_registrars="$obsolete_registrars\|${obsolete_prefix}sdl2"
obsolete_registrars="$obsolete_registrars\|${obsolete_prefix}pc98dos"
if grep -R "$obsolete_registrars" \
	"$root/include/noct/noct.h" "$root/src" "$root/docs" >/dev/null
then
	echo "obsolete BeUI registration interface found" >&2
	exit 1
fi
if grep -E 'ZEDBSD_CONSOLE_(POLL_EVENT|READ_EVENT|KEY_STATE|DRAIN_INPUT)' \
	"$root/src/api/api-beui-zedbsd.c" >/dev/null
then
	echo "legacy zedBSD console input API found in BeUI backend" >&2
	exit 1
fi

echo 'BeUI zedBSD wiring audit: OK'
