#!/bin/sh

set -eu

NOCT=${NOCT:-../../build-static/noct}
case "$NOCT" in
/*) ;;
*) NOCT=$(CDPATH= cd -- "$(dirname -- "$NOCT")" && pwd)/$(basename -- "$NOCT") ;;
esac
TMP=$(mktemp -d)
trap 'rm -rf "$TMP"' EXIT HUP INT TERM

# Exercise transitive loading, duplicate suppression, .nct fallback, and
# dependency-first initialization in both execution engines.
"$NOCT" -j0 --path=app/require/modules \
    app/require/main.noct > "$TMP/interpreter.out"
diff -u app/require/require.out "$TMP/interpreter.out"

"$NOCT" -j --path=app/require/modules \
    app/require/main.noct > "$TMP/jit.out"
diff -u app/require/require.out "$TMP/jit.out"

# A missing module must be diagnosed by the CLI resolver boundary.
if "$NOCT" -j0 --path=app/require/modules \
    app/require/missing.noct > "$TMP/missing.log" 2>&1; then
    echo 'missing required module was accepted' >&2
    exit 1
fi
grep -F "Cannot resolve required module 'absent_module'" \
    "$TMP/missing.log" >/dev/null

# Re-entering a module that is still loading must fail deterministically.
if "$NOCT" -j0 --path=app/require/modules \
    app/require/cycle-a.noct > "$TMP/cycle.log" 2>&1; then
    echo 'circular require was accepted' >&2
    exit 1
fi
grep -F 'Circular require' "$TMP/cycle.log" >/dev/null

# Search one directory completely before the next directory, prefer .noct
# within a directory, and always search the current directory first.
mkdir "$TMP/current" "$TMP/first" "$TMP/second"
printf '%s\n' \
    'require choice;' \
    'func main() { print(chosen()); }' \
    > "$TMP/current/main.noct"
printf '%s\n' \
    'static func local_choice(): string { return "first-nct"; }' \
    'func chosen(): string { return local_choice(); }' \
    > "$TMP/first/choice.nct"
printf '%s\n' \
    'func chosen(): string { return "second-noct"; }' \
    > "$TMP/second/choice.noct"

(cd "$TMP/current" && \
    "$NOCT" -j0 --path="$TMP/first:$TMP/second" main.noct) \
    > "$TMP/path-order.out"
printf '%s\n' 'first-nct' > "$TMP/path-order.expected"
diff -u "$TMP/path-order.expected" "$TMP/path-order.out"

printf '%s\n' \
    'func chosen(): string { return "first-noct"; }' \
    > "$TMP/first/choice.noct"
(cd "$TMP/current" && \
    "$NOCT" -j0 --path="$TMP/first:$TMP/second" main.noct) \
    > "$TMP/suffix-order.out"
printf '%s\n' 'first-noct' > "$TMP/suffix-order.expected"
diff -u "$TMP/suffix-order.expected" "$TMP/suffix-order.out"

printf '%s\n' \
    'func chosen(): string { return "current"; }' \
    > "$TMP/current/choice.nct"
(cd "$TMP/current" && \
    "$NOCT" -j0 --path="$TMP/first:$TMP/second" main.noct) \
    > "$TMP/current-order.out"
printf '%s\n' 'current' > "$TMP/current-order.expected"
diff -u "$TMP/current-order.expected" "$TMP/current-order.out"

# The prototype scan must not impose an arbitrary limit on ordinary require
# graphs. This chain is intentionally longer than the old fixed table.
mkdir "$TMP/deep"
i=0
while [ "$i" -lt 300 ]; do
    module=$(printf 'm%03d' "$i")
    if [ "$i" -lt 299 ]; then
        next_index=$((i + 1))
        next_module=$(printf 'm%03d' "$next_index")
        printf 'require %s;\n' "$next_module" \
            > "$TMP/deep/$module.noct"
    else
        printf '%s\n' \
            'func deep_value(): string { return "deep"; }' \
            > "$TMP/deep/$module.noct"
    fi
    i=$((i + 1))
done
printf '%s\n' \
    'require m000;' \
    'func main() { print(deep_value()); }' \
    > "$TMP/deep-main.noct"
"$NOCT" -j0 --path="$TMP/deep" "$TMP/deep-main.noct" \
    > "$TMP/deep.out"
printf '%s\n' 'deep' > "$TMP/deep.expected"
diff -u "$TMP/deep.expected" "$TMP/deep.out"

# Empty search-path options are rejected before VM creation.
if "$NOCT" --path= app/require/main.noct \
    > "$TMP/empty-path.log" 2>&1; then
    echo 'empty --path option was accepted' >&2
    exit 1
fi
grep -F 'Invalid --path option' "$TMP/empty-path.log" >/dev/null

echo 'PASS require resolver'
