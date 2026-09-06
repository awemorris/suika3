# CPU/JIT and raw GPU CNN forward benchmark

## Reference

The test is structurally adapted from the forward path in
[`paramhanji/CUDA-CNN`](https://github.com/paramhanji/CUDA-CNN), specifically
the flattened CUDA execution model used by `fp_preact_c1`, `fp_preact_s1`,
and `fp_preact_f` in `layer.cu`.

That repository does not declare a license. No source or weights are copied.
`tests/testcases/accel/gpu-cnn-forward.noct` is an independent, smaller network that
keeps the convolution, subsampling, and dense topology.

## Test network

```
8x8x1 input
  -> valid 3x3 convolution, 2 channels (6x6x2)
  -> ReLU (6x6x2)
  -> 2x2 average pooling, stride 2 (3x3x2)
  -> dense 18-to-3
  -> logits [4, 3.5, 0]
```

The CPU path uses ordinary `func` functions and host `Packed.float32`
tensors. The GPU path uses one `__gpu func` per layer and persistent
`Accel.float32` resources, with no host transfer between layers. Both paths
use the same weights, arithmetic order, and exact-logit regression oracle.

## Correctness

From `tests/`:

```sh
GALLIUM_DRIVER=d3d12 ../build-linux-opengl/noct --accel=opengl \
    -j -O2 accel/gpu-cnn-forward.noct
```

The case also runs from `run-accel-opengl.sh` in interpreter, forced-JIT,
and bytecode reload modes.

## Steady-state benchmark protocol

From the repository root:

```sh
TARGET_SECONDS=30 WARMUP_SECONDS=30 SAMPLES=3 \
    bench/run-gpu-cnn-bench.sh
```

The C harness embeds one Noct VM. It completes ordinary-function JIT and all
four OpenGL pipeline compilations before any timed region, so JIT and shader
compilation time are excluded. CPU and GPU iteration counts are calibrated
independently because this deliberately tiny CNN is dominated by synchronous
GPU launch/fence overhead.

For each mode the runner performs an untimed 30-second warm-up, recalibrates
at the resulting clock and thermal state, and records three runs of about 30
seconds each. Thus the default run takes about four minutes plus calibration.
It verifies the exact logits after warm-up and after every sample.

Output is CSV. `median_ns_per_forward` is the primary result. `gpu_speedup` is
`CPU time / GPU time`, so a value below 1 means the GPU path is slower.
Short smoke measurements are possible, for example:

```sh
TARGET_SECONDS=0.2 WARMUP_SECONDS=0.2 SAMPLES=1 \
    bench/run-gpu-cnn-bench.sh
```

The tiny network is a bring-up and launch-overhead benchmark, not a claim
about throughput on production CNN sizes.

## WSL2 reference result (2026-08-11)

The default protocol produced the following result on an Intel Core Ultra 5
228V (8 WSL2 CPUs) and Mesa D3D12 OpenGL 4.6 on an Intel Arc 130V GPU:

| mode | iterations/sample | best | median | worst | median/forward |
|---|---:|---:|---:|---:|---:|
| CPU ordinary func/JIT | 704,635 | 29.718 s | 31.693 s | 31.749 s | 44.977 us |
| GPU raw func/OpenGL | 19,742 | 29.186 s | 29.223 s | 29.251 s | 1.480 ms |

The measured `gpu_speedup` was `0.030385`: the GPU path took about 32.9 times
as long as CPU/JIT per forward. Four synchronous GPU dispatches and fences per
forward dominate the 8x8 network. This is useful confirmation that the raw
kernel path works end to end, while a meaningful throughput crossover study
will require larger tensors, batching, asynchronous chaining, or kernel
fusion.
