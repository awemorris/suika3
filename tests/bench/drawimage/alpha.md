# DRAW_IMAGE_ALPHA function-call benchmark

Date: 2026-08-10

`tests/testcases/simd/drawimage/blend-alpha.noct`のアルファブレンド式を、連続する
1000 pixel に適用する一回の関数呼び出しについて測定した。

## 測定境界

- JIT は threshold 0 で source 登録時に生成する。
- 最初に 1-pixel の呼び出しを行い、JIT code の commit を完了する。
- 続いて測定対象と同じ 1000-pixel 呼び出しを5回 warmup する。
- 入力バッファの初期化と各回の復元は測定区間外とする。
- 単調時計を `blend_alpha()` の呼び出し直前と復帰直後だけで読む。
- O0 と O2 をそれぞれ50回測り、最速値と通常の偶数標本中央値
  （25番目と26番目の平均）を求める。
- x86_64 はCPU 63へ固定した。M5はmacOS上で通常実行した。
- 両アーキでO0/O2のchecksumは `7225778` で一致した。
- O2では同じloopが `i32x4` mixed conversion loopとしてvectorizeされた。

これは「720p frameを1000回処理」ではなく、「同じalpha-blend kernelを
1000 pixel反復する一回の関数呼び出し」の測定である。

## 結果

| architecture | level | fastest | median | O0からの高速化（fastest） | O0からの高速化（median） |
|---|---:|---:|---:|---:|---:|
| Intel Xeon Gold 6130 x86_64 | O0 | 1.136730 ms | 1.166826 ms | 1.00x | 1.00x |
| Intel Xeon Gold 6130 x86_64 | O2 | 0.004029 ms | 0.004124 ms | 282.14x | 282.94x |
| Apple M5 arm64 | O0 | 0.496500 ms | 0.765479 ms | 1.00x | 1.00x |
| Apple M5 arm64 | O2 | 0.002458 ms | 0.002542 ms | 201.99x | 301.13x |

O0は動的なpacked access、数値演算、`Float.from` / `Int.from` の処理を
各pixelで行う。O2ではABCE、typed operation、invariant index normalization、
mixed i32/f32 SIMDが組み合わさるため、倍率は単なるSIMD幅4より大幅に大きい。

## 再現用ファイル

- `tests/testcases/simd/drawimage/blend-alpha.noct`: テストとベンチで共有するalpha kernel。
- `bench/drawimage/alpha-call-bench.c`: VM/JIT warmup、関数境界計時、標本集計。
- `bench/drawimage/run-alpha.sh`: ビルドとO2/O3測定をまとめたrunner。

Linux x86_64ではrelease buildの`libnoct.a`/`libnoctapi.a`へリンクし、
macOS arm64では`macos-arm64` presetへ`NOCT_ENABLE_OPTIMIZER=ON`を追加した
release buildへリンクした。

## O3 FMA follow-up (2026-08-11)

The O3 implementation was measured with the same harness after extending it
to accept level 3.  To reduce timer noise, this follow-up uses 1,000,000
pixels per call, 5 warmups, and 50 measured calls.  JIT compilation remains
outside the measured interval.  O2 and O3 produced the same checksum,
`6236317`.

| architecture | level | fastest | median | speedup from O2 (fastest) | speedup from O2 (median) |
|---|---:|---:|---:|---:|---:|
| Intel Xeon Gold 6130 x86_64 | O2 | 1.788048 ms | 1.797989 ms | 1.000x | 1.000x |
| Intel Xeon Gold 6130 x86_64 | O3 FMA | 1.487437 ms | 1.488708 ms | 1.202x | 1.208x |
| Apple M5 arm64 | O2 | 0.502542 ms | 0.675562 ms | 1.000x | 1.000x |
| Apple M5 arm64 | O3 FMA | 0.462000 ms | 0.493354 ms | 1.088x | 1.369x |

The x86_64 native dump contains three `vfmadd231ps` instructions, direct
`movdqu (base,rdi,4)` loads/stores, the preheader `xmm15` opaque-alpha
constant, and a two-instruction `addq $4,rdi` / `jne` latch.  Native and
capability-forced portable results matched on both x86_64 and Apple M5.

## x86_64 GCC-parity implementation result (2026-08-11)

Design 18 was implemented on top of revision `cc6eda2` plus the uncommitted
Design 17 work.  The host was an Intel Xeon Gold 6130 and the reference
compiler was GCC 14.2.0.  This section is a code-shape result, not a new
runtime-performance claim; the development host was not used for a fresh
timing run.

The vector planner reported:

~~~text
noct-lir-vfor: max=13 homes=3 candidates=10 caches=4 stack=7
~~~

Thus the source packed load, destination packed load, `pix_a`, and
`pix_inv_a` are all materialized once and kept live.  Logical vector
registers 0..12 map to xmm0..xmm12; xmm13/xmm14 remain SSE2 integer-multiply
scratch and xmm15 owns the opaque-alpha invariant.

The final recurrent Noct loop spans native offsets `0x150b..0x15d4`: 202
bytes and 39 instructions including the latch.

~~~asm
150b movdqu       (%rbx,%rdi,4), %xmm3
1510 movdqu       (%rsi,%rdi,4), %xmm4
1515 vpsrld       $24, %xmm3, %xmm5
151b vcvtdq2ps    %xmm5, %xmm5
1520 vmulps       %xmm2, %xmm5, %xmm5
1525 vsubps       %xmm5, %xmm1, %xmm6
152a vpsrld       $16, %xmm4, %xmm7
1530 vpand        %xmm0, %xmm7, %xmm7
1535 vcvtdq2ps    %xmm7, %xmm7
153a vmulps       %xmm6, %xmm7, %xmm7
153f vpsrld       $16, %xmm3, %xmm8
1545 vpand        %xmm0, %xmm8, %xmm8
154a vcvtdq2ps    %xmm8, %xmm8
154f vfmadd231ps  %xmm5, %xmm8, %xmm7
1554 vcvttps2dq   %xmm7, %xmm7
1559 vpslld       $16, %xmm7, %xmm7
155f vpor         %xmm15, %xmm7, %xmm7
1564 vpsrld       $8, %xmm4, %xmm8
156a vpand        %xmm0, %xmm8, %xmm8
156f vcvtdq2ps    %xmm8, %xmm8
1574 vmulps       %xmm6, %xmm8, %xmm8
1579 vpsrld       $8, %xmm3, %xmm9
157f vpand        %xmm0, %xmm9, %xmm9
1584 vcvtdq2ps    %xmm9, %xmm9
1589 vfmadd231ps  %xmm5, %xmm9, %xmm8
158e vcvttps2dq   %xmm8, %xmm8
1593 vpslld       $8, %xmm8, %xmm8
1599 vpor         %xmm8, %xmm7, %xmm7
159e vpand        %xmm0, %xmm4, %xmm8
15a3 vcvtdq2ps    %xmm8, %xmm8
15a8 vmulps       %xmm6, %xmm8, %xmm8
15ad vpand        %xmm0, %xmm3, %xmm9
15b2 vcvtdq2ps    %xmm9, %xmm9
15b7 vfmadd231ps  %xmm5, %xmm9, %xmm8
15bc vcvttps2dq   %xmm8, %xmm8
15c1 vpor         %xmm8, %xmm7, %xmm7
15c6 movdqu       %xmm7, (%rsi,%rdi,4)
15cb add          $4, %rdi
15cf jne          0x150b
~~~

The mnemonic histogram is: `movdqu=3`, `vpsrld=5`, `vpslld=2`, `vpand=6`,
`vcvtdq2ps=7`, `vcvttps2dq=3`, `vmulps=4`, `vsubps=1`,
`vfmadd231ps=3`, `vpor=3`, `add=1`, and `jne=1`.  No YMM or ZMM register is
present.

| recurrent-loop category | GCC native x4 | Noct x4 |
|---|---:|---:|
| packed loads/stores | 3 | 3 |
| two-address adaptation copies | 0 | 0 |
| channel extraction shifts/masks | 11 | 11 |
| integer/float conversions | 10 | 10 |
| floating-point arithmetic | 8 | 8 |
| channel packing | 4 | 5 |
| loop control | 3 | 2 |
| **total** | **39** | **39** |

GCC's native reference saves one packing instruction with 128-bit
`vpternlogd`, but uses `add/cmp/jne` for loop control.  Noct deliberately does
not require AVX-512VL: its three `vpor` packing operations are offset by the
existing negative-index `add/jne` latch.

The GCC reference was regenerated from the signed-`int32_t` conversion
kernel with:

~~~sh
gcc -O3 -march=native -mprefer-vector-width=128 \
    -fno-unroll-loops -ffp-contract=fast
gcc -O3 -march=haswell -mprefer-vector-width=128 \
    -fno-unroll-loops -ffp-contract=fast
~~~

Verification completed:

- native x86_64 debug build and the full `tests/test.sh all` suite;
- `run-simd.sh`, including scalar/SSE2/SSE3/SSE4.1/AVX ceilings, O2/O3 FMA
  metadata, portable fallback, and the four-cache/native-JIT assertion;
- `run-fma-helper.sh`, `run-cse.sh`, `run-abce.sh`, `run-typing.sh`,
  `run-syntax.sh`, and `run-cli-options.sh`;
- MinGW x86 and MinGW x86_64 compile-only builds; and
- Apple M5 Arm64 mt-debug build plus the complete SIMD suite.

The pre-existing Android Arm64 build directory could not be regenerated on
the development host because its cached NDK toolchain path was `/build/...`;
the native M5 build/test above provides the Arm64 regression gate instead.
AVX2 x8 and wider loops remain future work.

## Cache-ranking regression fix (2026-08-12)

The draw-image follow-up initially changed the ALPHA cache selection from
two packed loads plus `pix_a`/`pix_inv_a` to a nested parent/child set.  That
caused two extra destination loads.  The wider AVX register map also rebuilt
the opaque-alpha constant inside every iteration.  Together these changed the
recurrent loop from 39 to 44 instructions and made the immediate pre-fix build
about 6--7% slower than revision `97deff4` in an interleaved same-host test.

Cache ranking now uses marginal benefit: repetitions removed by an already
selected parent are subtracted from its child candidate.  The planner again
selects both packed loads.  On AVX, an otherwise-unused physical vector
register holds the opaque-alpha constant from the preheader while xmm15
remains instruction-local scratch.  The recurrent O3 loop is back to 39
instructions.

The comparison below used CPU 63, 1,000,000 pixels, five warmups, 50 samples,
and excluded JIT compilation.  It compares the clean `97deff4` worktree and
the fixed worktree in one interleaved run; all checksums were `6236317`.

| build | level | fastest | median |
|---|---:|---:|---:|
| `97deff4` | O2 | 1.051779 ms | 1.089400 ms |
| fixed | O2 | 1.053492 ms | 1.080560 ms |
| `97deff4` | O3 | 0.972543 ms | 1.006331 ms |
| fixed | O3 | 0.966599 ms | 0.994790 ms |

The fixed build is therefore at performance parity with the clean baseline
(within about 1% in this short, shared-server measurement), while removing
the reproducible 6--7% pre-fix regression.  The SIMD suite now asserts two
selected packed-load caches and preheader immediate materialization so the
same code-shape regression cannot pass on output correctness alone.
