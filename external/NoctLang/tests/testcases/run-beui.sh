#!/bin/sh

# BeUI host tests.  Core and PC-98 implementation contracts are private, so
# their suites compile the owning sources directly.  An SDL2-enabled CLI, when
# present, exercises the sole public registration path.

set -eu

root=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
build_dir=${1:-"$root/build-static"}
case "$build_dir" in
/*) ;;
*) build_dir="$root/$build_dir" ;;
esac
cc=${CC:-cc}
api="$root/src/api"

test -f "$build_dir/libnoctapi.a" || {
	echo "Noct API static library not found: $build_dir/libnoctapi.a" >&2
	echo "Configure with -DNOCT_ENABLE_API_BEUI=ON and build first." >&2
	exit 1
}

"$cc" -I"$root/include" -I"$api" "$root/tests/testcases/beui-test.c" \
	-lm \
	-o "$build_dir/noct-beui-test"
"$build_dir/noct-beui-test"

if test -x "$build_dir/noct" &&
	grep -q '^NOCT_ENABLE_API_BEUI:BOOL=ON$' "$build_dir/CMakeCache.txt" &&
	! grep -q '^NOCT_TARGET_PC98DOS:BOOL=ON$' "$build_dir/CMakeCache.txt" &&
	! grep -q '^NOCT_TARGET_ZEDBSD:BOOL=ON$' "$build_dir/CMakeCache.txt"
then
	SDL_VIDEODRIVER=${SDL_VIDEODRIVER:-dummy}
	SDL_AUDIODRIVER=${SDL_AUDIODRIVER:-dummy}
	export SDL_VIDEODRIVER SDL_AUDIODRIVER
	"$build_dir/noct" -O0 "$root/tests/testcases/beui-sdl2.noct"
	echo 'BeUI SDL2 tests: OK'
fi

"$cc" -I"$root/include" -I"$api" "$root/tests/testcases/beui-pc98-gdc-test.c" \
	-o "$build_dir/noct-beui-pc98-gdc-test"
"$build_dir/noct-beui-pc98-gdc-test"
echo 'BeUI PC-98 GDC tests: OK'

"$cc" -I"$root/include" -I"$api" "$root/tests/testcases/beui-pc98-cirrus-test.c" \
	-o "$build_dir/noct-beui-pc98-cirrus-test"
"$build_dir/noct-beui-pc98-cirrus-test"
echo 'BeUI PC-98 Cirrus tests: OK'
