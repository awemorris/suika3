#!/bin/sh

set -eu

CC=${CC:-cc}
tmp=${TMPDIR:-/tmp}/noct-accel-program-$$
trap 'rm -f "$tmp"' EXIT HUP INT TERM

"$CC" -std=gnu89 -Wall -Wextra -Werror -DHAVE_STDINT_H=1 \
	-I../../include -I../../src/core \
	accel/unit/accel-program.c ../../src/core/accel_program.c -o "$tmp"
"$tmp"
