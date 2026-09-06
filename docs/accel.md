__accel func
============

`__accel` marks an ordinary function as a candidate for synchronous
GPU optimization.  It is a hint, not a separate function kind: the
function is called by its normal name and always has a complete CPU
implementation.  If GPU optimization is unavailable or declines the
function, the CPU body keeps its ordinary semantics.

The modifier is accepted in exactly these positions:

```
__accel func transform(data: rpackedint32, count: int): void {
    for (i in 0..count) {
        data[i] = data[i] * 2;
    }
}

static __accel func clear(data: rpackedint32, count: int): void {
    for (i in 0..count) {
        data[i] = 0;
    }
}
```

Thus the two forms are `__accel func` and `static __accel func`.  The
modifier applies only to functions and cannot be combined with
`__fast` or `__inline`.  The function body otherwise uses ordinary
Noct syntax, annotations, calls, and return rules.

GPU rewriting is attempted only in an accelerator-enabled build, at
optimization level 1 or above, while source is run with `--gpu`.  An
offloaded function must return `void`.  Its GPU-visible buffer
parameters use exact `rpackedint32`, `rpackeduint32`, or
`rpackedfloat` annotations, and its scalar parameters use the `int` or
`float` runtime type.  Other legal signatures keep the CPU
implementation.

The current GPU subset recognizes suitable top-level, one-dimensional
ranged loops.  Independent loops become DOALL kernels.  Straight-line
numeric arithmetic, comparisons, and Packed loads and stores inside
those loops can be moved to the selected backend.  Adjacent eligible
loops may share one synchronous GPU session.  Ordinary CPU statements
separate sessions: one GPU region completes before the CPU statements
run, and a later region starts for the next eligible group.  Regions
are source-ordered parts of one function, not different physical GPUs.

Code outside the subset remains valid CPU code.  Unsupported control
flow, calls, types, or dependence patterns cause GPU optimization to
decline rather than making the source invalid.  A DOSUM reduction may
be offloaded when its accumulator is `int`/`i32` or `u32`, starts at
additive zero, and is updated by addition.  Floating-point, minimum,
maximum, and product reductions currently remain on the CPU.

Local Packed buffers use ordinary declarations and constructors, for
example `let temporary: packedfloat = Packed.float32(count)`.
Normally the object is CPU-owned and whole-buffer transfers surround
the GPU region.  The optimizer may omit the host object when it proves
a conservative device-only local: the matching constructor immediately
precedes one region, the local does not escape or participate in CPU
code, and the first kernel completely initializes it before any read.
Otherwise the CPU-backed representation is retained or the region is
declined.  Asynchronous execution and persistence across regions or
calls are not part of the source contract.

Compiled `.nbc` and `.nap` files, `--compile`, `--app`, and the source
transpilers preserve only the ordinary CPU body.  Once a source run
has committed an eligible region and GPU execution has started, a GPU
failure ends that invocation; the replaced CPU loop is not replayed.

`--gpu-list` reports selectable devices.  Canonical selectors have the
forms `vulkan:NAME`, `opengles:NAME`, `d3d12:NAME`, and `metal:NAME`;
an unqualified exact name is accepted only when it is unique.  `--gpu`
chooses a default suitable device, while `--gpu=SELECTOR` requests one
explicitly.
