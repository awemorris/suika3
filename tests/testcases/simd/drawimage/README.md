# Draw-image SIMD essence cases

These files reduce the inner pixel loops from `/home/awe/drawimage.h` to
small Noct programs.  They are optimizer probes, not production-compatible
image APIs.  Clipping, row strides, scan conversion, notifications, and the
PC-98/PC/AT alpha-table specialization are deliberately outside the cases.

Source snapshot: SHA-256
`75dbc7ed68a92309116b8ea4335bd3ab9cbbd2586f7425fd4791bbbf1df43ee3`
(2026-08-12).

| Case | Source kernel represented |
|---|---|
| `blend-alpha.noct` | `DRAW_IMAGE_ALPHA`; canonical test and benchmark kernel |
| `blend-copy.noct` | `DRAW_IMAGE_COPY` |
| `blend-dim.noct` | `DRAW_IMAGE_DIM`; arithmetic part of `3D_DIM` |
| `blend-glyph.noct` | Common non-PC98 body of `GLYPH` and `EMOJI` |
| `blend-add.noct` | `DRAW_IMAGE_ADD`; arithmetic part of `3D_ADD` |
| `blend-sub.noct` | `DRAW_IMAGE_SUB`; arithmetic part of `3D_SUB` |
| `blend-rule.noct` | `DRAW_IMAGE_RULE` conditional store |
| `blend-melt.noct` | `DRAW_IMAGE_MELT` clamp and blend |
| `blend-cross.noct` | `DRAW_IMAGE_CROSS` after bounds are pre-clipped |
| `blend-3d-alpha.noct` | Affine coordinates plus one texture gather |
| `blend-3d-cross.noct` | Two rasterized texture gathers and cross blend |

`blend-alpha-multigroup.noct` is an additional live-range/remainder regression
for the same ALPHA kernel and is not counted as a separate draw-image mode.

Inspect the current optimizer decision with, for example:

```sh
NOCT_SIMD_DEBUG=1 build-static/noct -j0 -O2 \
  tests/testcases/simd/drawimage/blend-dim.noct
```

## Measured vectorization (updated 2026-08-12)

All eleven canonical probes are admitted as architecture-neutral x4 bytecode.
On the measured SysV x86_64 AVX/SSE4.1 host, all eleven also use the packed native tier;
`NOCT_JIT_VECTOR_DEBUG=1` reports the following resource plan.

| Case | Logical/peak vregs | Physical | Scratch | Spill | Native cursor |
|---|---:|---:|---:|---:|---|
| alpha | 10 | 10 | 1 | 0 | register |
| copy | 1 | 1 | 1 | 0 | register |
| dim | 11 | 11 | 1 | 0 | register |
| glyph | 15 | 15 | 1 | 0 | register |
| add | 11 | 11 | 1 | 0 | register |
| sub | 12 | 12 | 1 | 0 | register |
| rule | 5 | 5 | 1 | 0 | memory |
| melt | 15 | 15 | 1 | 0 | memory |
| cross | 11 | 11 | 1 | 0 | memory |
| 3d-alpha | 13 | 13 | 1 | 0 | register |
| 3d-cross | 11 | 11 | 1 | 0 | memory |

The native cursor column describes the PBASE/index addressing hint, not
whether packed SIMD is used.  `memory` still means packed native arithmetic;
it reloads one or more base/index values because the current hint keeps only
two packed bases in GPRs.

## x86_64 instruction audit

Configuration:

```text
Noct: build-static/noct -j -O2
GCC:  gcc 14.2 -O3 -mavx -msse4.1 -mno-avx2 -ffp-contract=off
Width: x4 / 128 bit, FMA disabled
```

Noct bytes were captured with `NOCT_JIT_DUMP_DIR`, after branch patching and
before slab commit, then decoded with `objdump -D -b binary -m i386:x86-64
-M intel`.  Counts cover the vector header through its backward `jne`.  The
3D regions include inline cold checked-gather failure blocks, so their count
is deliberately not a dynamic successful-path count.

| Case | Noct before | Noct now (x4) | GCC count | GCC form/count basis | Noct/GCC | Key current lowering |
|---|---:|---:|---:|---|---:|---|
| alpha | 44 regression | 42 O2 / 39 O3 | 42 | packed x4, FMA disabled | 1.00x | two cached loads; O3 uses 3 FMA |
| copy | 4 | 4 | -- | `memcpy` wrapper, no comparable loop | -- | load, store, add/jne |
| dim | 48 | 46 | 59 | packed x4 loop | 0.78x | packed AVX x4 |
| glyph | 511 scalar-expanded | 51 | 49/pixel | scalar loop | -- | packed compare/select |
| add | 65 | 41 | 54 | packed x4 loop | 0.76x | 3 `vpminsd` |
| sub | 65 | 41 | 39 | packed x4 loop | 1.05x | 3 `vpmaxsd` |
| rule | 57 scalar-expanded | 20 | 11 | packed x4 loop | 1.82x | `vmaskmovps` |
| melt | 512 scalar-expanded | 70 | 47/pixel | scalar loop | -- | packed clamp/select |
| cross | 112 | 98 | 76 | packed x4 loop | 1.29x | 1 div, 10 converts, 11 multiplies |
| 3d-alpha | 642 scalar-expanded | 515 incl. cold | 52/pixel | scalar loop | -- | exact induction + checked gather |
| 3d-cross | 1293 scalar-expanded | 442 incl. cold | 97 | packed x4 loop, no inline cold paths | 4.56x* | two checked gathers |

`Noct/GCC` is a static instruction-count ratio, not a speed ratio.  A value
below 1 means fewer static instructions.  Ratios are shown only where both
sides are x4 loops.  The starred 3D-CROSS ratio is not like-for-like because
the Noct range includes inline checked-gather failure blocks while the GCC
count does not.

The important CROSS common-value target is met even though address/pack code
still leaves total count above GCC: `vdivps=1`, `vcvtdq2ps=10`, and
`vmulps=11`.  ADD intentionally uses signed minimum because Noct's `alpha`
is an `int`; the negative-alpha regression prevents replacing it with GCC's
unsigned pixel-specific minimum.

The cache planner uses marginal rather than independent expression benefit:
after selecting a repeated parent expression, repetitions eliminated from its
children no longer compete for another cache slot.  This fixed the ALPHA
packed-load regression and reduced or preserved the instruction count of all
eleven draw-image probes.  Opaque-alpha vector constants use a spare physical
register and are materialized in the vector preheader when one is available.

Representative compiler lowerings are `pminud`/`umin.4s` for add clamp,
`pmaxsd`/`smax.4s` for subtract clamp, `pblendvb` or `fcmgt.4s` plus
`bsl.16b` for glyph selection, and `minps`/`maxps` or compare plus
`bit.16b` for melt clamp.  SSE constructs a four-lane gather with `pinsrd`
and `punpcklqdq`; NEON uses scalar-lane `ld1.s` loads.

The Noct copy hot loop is already minimal: one `movdqu` load, one `movdqu`
store, `add index, 4`, and the back-edge `jne`.

## DRAW_IMAGE_ALPHA benchmark

The canonical ALPHA source is shared directly with the function-call harness;
there is no benchmark-only copy of the Noct kernel.  From the repository root:

```sh
./bench/bench.sh drawimage-alpha
CPU=63 SAMPLES=50 PIXELS=1000000 LEVELS="2 3" \
  ./bench/bench.sh drawimage-alpha
```

Historical measurements and the 39-instruction O3 dump are recorded in
`bench/drawimage/alpha.md`.

## Source questions noticed during extraction

- The PC-98/PC/AT `DRAW_IMAGE_GLYPH` path appears to use `dst_a` as an
  `alphatable` index before assigning it.
- The PC-98/PC/AT ALPHA and GLYPH bodies do not appear to apply the function's
  global `alpha` argument after `check_draw_image`.
- `DRAW_IMAGE_CROSS` computes `src2_a` from source 1 alpha, while
  `DRAW_IMAGE_3D_CROSS` uses source 2 alpha.  The Noct cross case preserves
  the non-3D C behavior so this discrepancy remains visible.
