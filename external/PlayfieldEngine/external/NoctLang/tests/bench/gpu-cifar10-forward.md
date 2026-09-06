# CIFAR-10 CNN CPU/JIT and raw GPU benchmark

## Reference architecture

The topology follows the CNN in the official
[PyTorch CIFAR-10 tutorial](https://docs.pytorch.org/tutorials/beginner/blitz/cifar10_tutorial.html):

```
32x32x3
  -> Conv2d(3, 6, 5) -> ReLU -> MaxPool2d(2, 2)
  -> Conv2d(6, 16, 5) -> ReLU -> MaxPool2d(2, 2)
  -> Linear(400, 120) -> ReLU
  -> Linear(120, 84) -> ReLU
  -> Linear(84, 10)
```

The inference path uses no sigmoid, softmax, exponential, or other currently
unsupported activation. Class selection from logits does not require softmax.
The benchmark uses deterministic synthetic input and sparse weights so CPU and
GPU outputs have an exact `[1, 2, ..., 10]` oracle. All specified multiply-adds
still execute; this is a performance-path and correctness test, not a trained
CIFAR-10 accuracy test.

## Work ratio

The network executes 651,720 multiply-adds per forward:

| layer | multiply-adds |
|---|---:|
| Conv 3x5x5, 6x28x28 outputs | 352,800 |
| Conv 6x5x5, 16x10x10 outputs | 240,000 |
| Linear 400x120 | 48,000 |
| Linear 120x84 | 10,080 |
| Linear 84x10 | 840 |

The original 8x8 CNN executes 702 multiply-adds, making this case 928.38 times
larger by that metric. Fixed GPU dot products are expanded in the Noct source
because the current raw `__gpu func` subset has no source-level loop statement.

## Steady-state benchmark

From the repository root:

```sh
TARGET_SECONDS=30 WARMUP_SECONDS=30 SAMPLES=3 \
    bench/run-gpu-cifar10-bench.sh
```

The shared embedded-VM harness finishes CPU JIT compilation and all OpenGL
pipeline compilation before timing. It calibrates CPU and GPU iteration counts
independently, performs a 30-second untimed warm-up for each mode, then records
three approximately 30-second samples. Exact logits are verified after every
warm-up and sample.

A short functional smoke run is:

```sh
TARGET_SECONDS=0.2 WARMUP_SECONDS=0.2 SAMPLES=1 \
    bench/run-gpu-cifar10-bench.sh
```

## WSL2 reference result (2026-08-11)

The default protocol produced the following result on an Intel Core Ultra 5
228V (8 WSL2 CPUs) and Mesa D3D12 OpenGL 4.6 on an Intel Arc 130V GPU:

| mode | iterations/sample | best | median | worst | median/forward |
|---|---:|---:|---:|---:|---:|
| CPU ordinary func/JIT | 1,762 | 30.003 s | 30.321 s | 30.669 s | 17.208 ms |
| GPU raw func/OpenGL | 11,467 | 29.965 s | 30.204 s | 30.482 s | 2.634 ms |

The measured `gpu_speedup` was `6.533165`. Unlike the 8x8 launch-overhead
case, this network has enough arithmetic per forward for the GPU to be about
6.53 times faster than CPU/JIT despite seven synchronous dispatches.
