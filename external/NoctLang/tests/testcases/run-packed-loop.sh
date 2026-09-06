#!/bin/sh

set -eu

NOCT=${NOCT:-../../build-debug/noct}
tmp_dir=$(mktemp -d)
trap 'rm -rf -- "$tmp_dir"' EXIT HUP INT TERM

echo 'Packed-loop/JIT register-cache tests:'

"$NOCT" -j0 -O0 packed-loop/regcache.noct >"$tmp_dir/o0"
diff packed-loop/regcache.noct.out "$tmp_dir/o0"
echo 'PASS packed-loop/regcache.noct (-j0 -O0)'

"$NOCT" -j -O2 packed-loop/regcache.noct >"$tmp_dir/o2"
diff packed-loop/regcache.noct.out "$tmp_dir/o2"
echo 'PASS packed-loop/regcache.noct (-j -O2)'

NOCT_JIT_GPR_LIMIT=0 "$NOCT" -j -O2 \
    packed-loop/regcache.noct >"$tmp_dir/gpr0.out"
diff packed-loop/regcache.noct.out "$tmp_dir/gpr0.out"
echo 'PASS packed-loop/regcache.noct (base/index only; GPR limit 0)'

NOCT_JIT_GPR_LIMIT=1 "$NOCT" -j -O2 \
    packed-loop/regcache.noct >"$tmp_dir/gpr1.out"
diff packed-loop/regcache.noct.out "$tmp_dir/gpr1.out"
echo 'PASS packed-loop/regcache.noct (forced spill; GPR limit 1)'

NOCT_JIT_REGCACHE_DEBUG=1 "$NOCT" -j -O2 \
    packed-loop/regcache.noct >"$tmp_dir/debug.out" 2>"$tmp_dir/debug.err"
diff packed-loop/regcache.noct.out "$tmp_dir/debug.out"
grep -F 'func=kernel' "$tmp_dir/debug.err" | \
    grep -F 'flags=0x5' | grep -F 'accepted=1' >/dev/null
grep -F 'func=kernel' "$tmp_dir/debug.err" | grep -F 'hits=' >/dev/null
grep -F 'proven-div=1' "$tmp_dir/debug.err" >/dev/null
echo 'PASS packed-loop/regcache.noct (cursor and GPR cache accepted)'

cp packed-loop/regcache.noct "$tmp_dir/regcache.noct"
"$NOCT" --compile -O2 "$tmp_dir/regcache.noct" >/dev/null
"$NOCT" -j "$tmp_dir/regcache.nbc" >"$tmp_dir/roundtrip.out"
diff packed-loop/regcache.noct.out "$tmp_dir/roundtrip.out"
echo 'PASS packed-loop/regcache.noct (optimized bytecode round trip)'

"$NOCT" -j0 -O0 packed-loop/three-base.noct >"$tmp_dir/three-o0"
diff packed-loop/three-base.noct.out "$tmp_dir/three-o0"
NOCT_JIT_GPR_LIMIT=1 "$NOCT" -j -O2 \
    packed-loop/three-base.noct >"$tmp_dir/three-gpr1"
diff packed-loop/three-base.noct.out "$tmp_dir/three-gpr1"
"$NOCT" -j -O2 packed-loop/three-base.noct >"$tmp_dir/three-o2"
diff packed-loop/three-base.noct.out "$tmp_dir/three-o2"
echo 'PASS packed-loop/three-base.noct (nonzero start, three bases, spill)'

for tc in unroll-width16 unroll-offset-tail; do
    "$NOCT" -j0 -O0 "packed-loop/$tc.noct" >"$tmp_dir/$tc-o0"
    diff "packed-loop/$tc.noct.out" "$tmp_dir/$tc-o0"
    "$NOCT" -j0 -O2 \
        "packed-loop/$tc.noct" >"$tmp_dir/$tc-int"
    diff "packed-loop/$tc.noct.out" "$tmp_dir/$tc-int"
    "$NOCT" -j -O2 \
        "packed-loop/$tc.noct" >"$tmp_dir/$tc-jit"
    diff "packed-loop/$tc.noct.out" "$tmp_dir/$tc-jit"
    NOCT_JIT_REGCACHE_DEBUG=1 \
        "$NOCT" -j -O2 "packed-loop/$tc.noct" \
        >"$tmp_dir/$tc-debug.out" 2>"$tmp_dir/$tc-debug.err"
    diff "packed-loop/$tc.noct.out" "$tmp_dir/$tc-debug.out"
    grep -F 'flags=0x15' "$tmp_dir/$tc-debug.err" | \
        grep -F 'accepted=1' >/dev/null
    cp "packed-loop/$tc.noct" "$tmp_dir/$tc.noct"
    "$NOCT" --compile -O2 \
        "$tmp_dir/$tc.noct" >/dev/null
    "$NOCT" -j "$tmp_dir/$tc.nbc" >"$tmp_dir/$tc-roundtrip"
    diff "packed-loop/$tc.noct.out" "$tmp_dir/$tc-roundtrip"
    echo "PASS packed-loop/$tc.noct (unroll4 interpreter/JIT/roundtrip)"
done

echo 'PASS variable-division profitability guard'

"$NOCT" -j -O2 abce/width16.noct >"$tmp_dir/carried.out"
diff abce/width16.noct.out "$tmp_dir/carried.out"
echo 'PASS loop-carried scalar compatibility'

# Exercise the dynamic PC/branch tables with one function that has well over
# the former 2048-entry ceiling.  Keep the generated source out of the tree.
awk 'BEGIN {
    print "func main() {";
    print "    var x = 0;";
    for (i = 0; i < 2300; i++)
        print "    x = x + 1;";
    print "    print(x);";
    print "}";
}' >"$tmp_dir/large-pc.noct"
NOCT_JIT_CODEGEN_DEBUG=1 "$NOCT" -j -O0 \
    "$tmp_dir/large-pc.noct" >"$tmp_dir/large-pc.out" \
    2>"$tmp_dir/large-pc.err"
test "$(cat "$tmp_dir/large-pc.out")" = 2300
if grep -F 'func=main' "$tmp_dir/large-pc.err" >/dev/null; then
    awk '
        /func=main/ {
            for (i = 1; i <= NF; i++) {
                if ($i ~ /^pc_entries=/) {
                    split($i, a, "=");
                    if (a[2] > 2048) found = 1;
                }
            }
        }
        END { exit found ? 0 : 1 }
    ' "$tmp_dir/large-pc.err"
fi
echo 'PASS dynamic JIT PC table (>2048 entries)'

echo 'All Packed-loop tests passed.'
