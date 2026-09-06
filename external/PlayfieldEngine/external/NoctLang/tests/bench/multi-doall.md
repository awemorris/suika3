# Managed multi-DOALL CPU/GPU benchmark

`multi-doall.noct` compares one large CPU/JIT invocation with one managed GPU
invocation on both OpenGL ES and Vulkan. One call executes eight ordered DOALL
loops. Every DOALL processes 4,194,304 `uint32` elements, for 33,554,432
element iterations per call. Each iteration contains dependent integer
arithmetic, bit operations, and an input-dependent unsigned division.

Each invocation evaluates:

```text
value = value ^ round_constant
value = value + input
value = value * 3
value = value / ((input & 7) + 1)
value = (value + round_index) & 16777215
```

The generator emits eight top-level ranged loops, so the managed accelerator
compiler creates eight real kernels and eight ordered dispatch steps. Two logical
intermediate buffers are reused in ping-pong order and remain on the GPU for
the whole call.

The DOALL shader uses a grid-stride loop. Its 65,536 logical workgroups exceed
Vulkan's portable 65,535-workgroup minimum by one; the backend caps physical
dispatch while preserving the full logical trip count, without splitting one
DOALL into multiple host calls. Regenerate the checked-in source after
changing the constants in `generate-multi-doall.py`.

GPU measurements deliberately include the complete synchronous operation:
host-to-device input transfer, one `Accel.call` containing eight dispatches, and
device-to-host output transfer. Setup, initial JIT generation, initial shader/pipeline compilation,
and correctness checks are outside measured samples.

Run on a machine with both `linux-opengl` and `linux-vulkan` builds:

```sh
bench/bench.sh multi-doall
```

The default is five measured samples per mode. Every sample is exactly one CPU
function call or one accelerator call; the harness does not repeat a smaller
workload to reach a target duration. Override the odd sample count with
`SAMPLES`. `OPENGL_BUILD_DIR` and `VULKAN_BUILD_DIR` select other builds.
