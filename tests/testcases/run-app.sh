#!/bin/sh

set -eu

NOCT=${NOCT:-../../build-mt-debug/noct}
tmp="app/.tmp-app-$$"
mkdir "$tmp"
trap 'rm -rf "$tmp"' EXIT HUP INT TERM

"$NOCT" --compile --app -O2 "$tmp/test.nap" \
    app/main.noct app/second.noct
head -n 1 "$tmp/test.nap" | grep -Fx '#!/usr/bin/noct'
test -x "$tmp/test.nap"
"$NOCT" -j0 "$tmp/test.nap" > "$tmp/interpreter.out"
diff app/app.out "$tmp/interpreter.out"
"$NOCT" -j "$tmp/test.nap" > "$tmp/jit.out"
diff app/app.out "$tmp/jit.out"

if "$NOCT" --compile --app "$tmp/duplicate.nap" \
    app/main.noct app/duplicate.noct > "$tmp/duplicate.log" 2>&1; then
    echo 'duplicate public symbol was accepted' >&2
    exit 1
fi
grep -F 'Duplicate public symbol "from_a"' "$tmp/duplicate.log"

if "$NOCT" --compile --app "$tmp/repeated.nap" \
    app/main.noct app/main.noct > "$tmp/repeated.log" 2>&1; then
    echo 'duplicate input was accepted' >&2
    exit 1
fi
grep -F 'Duplicate Noct App input' "$tmp/repeated.log"

if "$NOCT" --compile --app "$tmp/absolute.nap" \
    "$PWD/app/main.noct" > "$tmp/absolute.log" 2>&1; then
    echo 'absolute input path was accepted' >&2
    exit 1
fi

printf 'preserve-me\n' > "$tmp/preserved.nap"
if "$NOCT" --compile --app "$tmp/preserved.nap" \
    app/no-main.noct > "$tmp/no-main.log" 2>&1; then
    echo 'application without main was accepted' >&2
    exit 1
fi
grep -Fx 'preserve-me' "$tmp/preserved.nap"

# Runtime source loading and .nap linking share require resolution.  The leaf
# initializer must run once, followed by its importer and finally the root.
"$NOCT" -j0 --path=app/require/modules \
    app/require/main.noct > "$tmp/require-runtime.out"
diff app/require/require.out "$tmp/require-runtime.out"

"$NOCT" --compile --app --path=app/require/modules \
    "$tmp/require.nap" app/require/main.noct
"$NOCT" -j0 "$tmp/require.nap" > "$tmp/require-app.out"
diff app/require/require.out "$tmp/require-app.out"
"$NOCT" -j "$tmp/require.nap" > "$tmp/require-app-jit.out"
diff app/require/require.out "$tmp/require-app-jit.out"
if grep -F "$PWD" "$tmp/require.nap" >/dev/null; then
    echo 'Noct App leaked an absolute require path' >&2
    exit 1
fi

if "$NOCT" --compile --app --path=app/require/modules:app/require \
    "$tmp/cycle.nap" app/require/cycle-a.noct \
    > "$tmp/cycle.log" 2>&1; then
    echo 'circular require was accepted' >&2
    exit 1
fi
grep -F 'Circular require' "$tmp/cycle.log"

if "$NOCT" -j0 --path=app/require/modules \
    app/require/missing.noct > "$tmp/missing.log" 2>&1; then
    echo 'missing required module was accepted' >&2
    exit 1
fi
grep -F "Cannot resolve required module 'absent_module'" "$tmp/missing.log"

cp app/require/main.noct "$tmp/standalone.noct"
if "$NOCT" --compile "$tmp/standalone.noct" \
    > "$tmp/standalone.log" 2>&1; then
    echo 'standalone bytecode accepted require without bundling' >&2
    exit 1
fi
grep -F 'require is supported by --compile --app' "$tmp/standalone.log"

# Installed packages live below HOME, have a path-stable logical name, run
# package_init() after their synthesized top-level initializer, and expose a
# NAME_main entry through `noct -m NAME`.
mkdir -p "$tmp/home/.noct/packages/demo"
cp app/package/demo.noct "$tmp/home/.noct/packages/demo/demo.noct"
HOME="$tmp/home" NOCT_PACKAGE_CACHE_DEBUG=1 \
    "$NOCT" -j0 -m demo hello > "$tmp/package-main.out" \
    2> "$tmp/package-cache-first.log"
diff app/package/package-main.out "$tmp/package-main.out"
grep -F 'noct-package-cache: demo: rebuilt' "$tmp/package-cache-first.log"
cache_file=$(find "$tmp/home/.noct/cache/packages/demo" -name '*.nbp' \
    -type f | head -n 1)
test -n "$cache_file"
printf 'truncated\n' > "$cache_file"
HOME="$tmp/home" NOCT_PACKAGE_CACHE_DEBUG=1 \
    "$NOCT" -j0 app/package/require-demo.noct \
    > "$tmp/package-require.out" 2> "$tmp/package-cache-rebuild.log"
diff app/package/package-require.out "$tmp/package-require.out"
grep -F 'noct-package-cache: demo: rebuilt' "$tmp/package-cache-rebuild.log"
HOME="$tmp/home" NOCT_PACKAGE_CACHE_DEBUG=1 \
    "$NOCT" -j0 -m demo hello > "$tmp/package-hit.out" \
    2> "$tmp/package-cache-hit.log"
diff app/package/package-main.out "$tmp/package-hit.out"
grep -F 'noct-package-cache: demo: hit' "$tmp/package-cache-hit.log"
if grep -F "$tmp/home" "$cache_file" >/dev/null; then
    echo 'package cache leaked an absolute home path' >&2
    exit 1
fi
printf '\n// invalidate content-addressed cache\n' >> \
    "$tmp/home/.noct/packages/demo/demo.noct"
HOME="$tmp/home" NOCT_PACKAGE_CACHE_DEBUG=1 \
    "$NOCT" -j0 -m demo hello > "$tmp/package-invalidated.out" \
    2> "$tmp/package-cache-invalidated.log"
diff app/package/package-main.out "$tmp/package-invalidated.out"
grep -F 'noct-package-cache: demo: rebuilt' \
    "$tmp/package-cache-invalidated.log"
test "$(find "$tmp/home/.noct/cache/packages/demo" -name '*.nbp' \
    -type f | wc -l | tr -d ' ')" = 2

# A transitive package cache preserves dependency initializer order.
for package in base middle graphapp; do
    mkdir -p "$tmp/home/.noct/packages/$package"
    cp "app/package/$package.noct" \
        "$tmp/home/.noct/packages/$package/$package.noct"
done
HOME="$tmp/home" NOCT_PACKAGE_CACHE_DEBUG=1 \
    "$NOCT" -j0 -m graphapp value > "$tmp/graph-first.out" \
    2> "$tmp/graph-first.log"
diff app/package/graph.out "$tmp/graph-first.out"
grep -F 'noct-package-cache: graphapp: rebuilt' "$tmp/graph-first.log"
HOME="$tmp/home" NOCT_PACKAGE_CACHE_DEBUG=1 \
    "$NOCT" -j0 -m graphapp value > "$tmp/graph-hit.out" \
    2> "$tmp/graph-hit.log"
diff app/package/graph.out "$tmp/graph-hit.out"
grep -F 'noct-package-cache: graphapp: hit' "$tmp/graph-hit.log"
HOME="$tmp/home" "$NOCT" --compile --app "$tmp/package.nap" \
    app/package/require-demo.noct
HOME="$tmp/no-package-home" "$NOCT" -j0 "$tmp/package.nap" \
    > "$tmp/package-nap.out"
diff app/package/package-require.out "$tmp/package-nap.out"
if grep -F "$tmp/home" "$tmp/package.nap" >/dev/null; then
    echo 'Noct App leaked an absolute package path' >&2
    exit 1
fi

mkdir -p "$tmp/home/.noct/packages/badinit"
cp app/package/badinit.noct \
    "$tmp/home/.noct/packages/badinit/badinit.noct"
if HOME="$tmp/home" "$NOCT" -j0 -m badinit \
    > "$tmp/badinit.log" 2>&1; then
    echo 'invalid package_init signature was accepted' >&2
    exit 1
fi
grep -F "package_init() must have no parameters and declare ': void'." \
    "$tmp/badinit.log"

mkdir -p "$tmp/home/.noct/packages/noentry" \
    "$tmp/home/.noct/packages/badarity"
cp app/package/noentry.noct \
    "$tmp/home/.noct/packages/noentry/noentry.noct"
cp app/package/badarity.noct \
    "$tmp/home/.noct/packages/badarity/badarity.noct"
if HOME="$tmp/home" "$NOCT" -m noentry > "$tmp/noentry.log" 2>&1; then
    echo 'package without NAME_main was accepted' >&2
    exit 1
fi
grep -F 'noentry_main() is not defined' "$tmp/noentry.log"
if HOME="$tmp/home" "$NOCT" -m badarity > "$tmp/badarity.log" 2>&1; then
    echo 'package entry with two parameters was accepted' >&2
    exit 1
fi
grep -F 'badarity_main() must have zero or one parameter' "$tmp/badarity.log"
if "$NOCT" -m > "$tmp/missing-package-name.log" 2>&1; then
    echo 'missing package name was accepted' >&2
    exit 1
fi
grep -F 'Specify a package name after -m.' "$tmp/missing-package-name.log"
if "$NOCT" -mexample > "$tmp/joined-package-option.log" 2>&1; then
    echo 'joined -mNAME option was accepted' >&2
    exit 1
fi
grep -F 'Unknown option -mexample.' "$tmp/joined-package-option.log"
if HOME="$tmp/home" "$NOCT" -m demo -m demo \
    > "$tmp/double-package.log" 2>&1; then
    echo 'double package mode was accepted' >&2
    exit 1
fi
grep -F 'Package mode cannot be combined' "$tmp/double-package.log"

echo 'Noct App tests passed.'
