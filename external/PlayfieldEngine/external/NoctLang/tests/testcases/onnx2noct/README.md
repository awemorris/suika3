# ONNX-to-Noct Stage A test assets

This directory contains test-only ONNX fixtures and CPU oracle data for
[design 18](../../../docs/design/18-onnx-gpu-source-codegen.md).  None of the
Python packages in `oracle/requirements.txt` are production dependencies of the
Noct compiler, runtime, converter, or generated models.

## Recreate the pinned environment

```sh
python3 -m venv /tmp/noct-onnx-oracle
/tmp/noct-onnx-oracle/bin/python -m pip install \
  -r tests/testcases/onnx2noct/oracle/requirements.txt
/tmp/noct-onnx-oracle/bin/python -m pip check
```

## Obtain locked third-party models

Large third-party models are deliberately not stored in the repository.
`tests/testcases/dnn/models.lock` pins their immutable URL, byte size, and SHA-256.

```sh
python3 tests/testcases/onnx2noct/oracle/download_models.py \
  --repository . \
  --cache "$HOME/.cache/noct-onnx-models"
```

The helper downloads through a temporary file and publishes it only after both
size and SHA-256 match.  Use `--only MODEL_ID` to fetch one model.

## Recreate project fixtures

```sh
/tmp/noct-onnx-oracle/bin/python \
  tests/testcases/onnx2noct/oracle/generate_fixtures.py \
  tests/testcases/onnx2noct/fixtures
```

The valid micro-models cover identity, broadcasting, sigmoid, reduction,
Conv, MaxPool, and Reshape.  The negative corpus covers a custom domain,
symbolic shape, external data, and truncated protobuf.  The larger
`project-cifar-opset12.onnx` is project-owned and deterministic.

## Recreate model oracles and lock inventory

```sh
/tmp/noct-onnx-oracle/bin/python \
  tests/testcases/onnx2noct/oracle/generate_model_oracles.py \
  tests/testcases/onnx2noct/oracle-data \
  --model mnist-12="$HOME/.cache/noct-onnx-models/mnist-12.onnx" \
  --model project-cifar-opset12=tests/testcases/onnx2noct/fixtures/models/project-cifar-opset12.onnx \
  --model squeezenet1.1-7="$HOME/.cache/noct-onnx-models/squeezenet1.1-7.onnx" \
  --model tinyyolov2-8="$HOME/.cache/noct-onnx-models/tinyyolov2-8.onnx"

/tmp/noct-onnx-oracle/bin/python \
  tests/testcases/onnx2noct/oracle/build_models_lock.py \
  --repository . \
  --external-cache "$HOME/.cache/noct-onnx-models" \
  --oracles tests/testcases/onnx2noct/oracle-data/model-oracles.lock \
  --output tests/testcases/dnn/models.lock
```

`generate_model_oracles.py` concretizes a symbolic batch dimension to one only
to create a test vector.  It does not change the source ONNX model.  The exact
Tiny YOLOv2 artifact remains blocked for design-18 v1 conversion because its
input and output declare symbolic batch `None`; this is recorded in
`tests/testcases/dnn/models.lock` and must not be normalized away silently.

## Run the Stage D/E reader and Stage F/G source generator

The production reader is written in Noct; Python is used only to create the
small deterministic malformed-test corpus.

```sh
NOCT=./build-static/noct sh tests/test.sh onnx2noct

build-static/noct --path=tools/onnx2noct \
  tools/onnx2noct/main.noct --output=GENERATED_DIR MODEL.onnx
```

The command validates and normalizes the model, creates a new exclusive output
directory, and publishes a deterministic GPU-only package.  The committed
artifacts are `model.weights`, `gpu/model.noct`, optional `gpu/main.noct`, and
the manifest-last `manifest.json`.  Stage F/G source goldens track the bound
`gpu/model.noct` artifact.  Stage F covers
metadata views, strided COPY, Relu/Sigmoid, and Add/Sub/Mul/Div static
broadcasting.  Stage G.1 additionally covers the reference Conv2D family:
rank-4 contiguous offset-zero NCHW input/output, contiguous OIHW float32
initializer weights, optional contiguous bias, group 1, dilation 1, static
stride and padding.  Each output element is one global invocation with direct
`ic/ky/kx` loops.  Other normalized operators and Conv regimes fail with
node/opset/shape/attribute capability context.

Stage G.2 adds correctness-first rank-2 Gemm and MatMul.  B must be a direct
contiguous float32 initializer; optional Gemm C must also be a direct
contiguous initializer and may statically broadcast to the output.  Gemm
supports canonical `alpha`, `beta`, `transA`, and `transB`; A may be a checked
strided view.  One invocation owns one output element and performs a constant
`k` loop.  Exact float32 attributes use the raw-GPU-only
`Accel.float32FromBits(bits)` constructor, which is not an ACCEL_MATH function
or a runtime API.

Stage G.3 adds rank-4 NCHW MaxPool, AveragePool, and GlobalAveragePool.
Inputs/outputs are contiguous offset-zero views, local pooling supports static
kernel/stride/padding with ceil mode 0 and dilation 1, and AveragePool preserves
the exact `count_include_pad` divisor rule.  Each output element is owned by
one invocation and uses constant `ky/kx` loops.

Stage H extracts every reviewed float32 initializer into canonical NWT1 bytes,
binds the NWT1 header to the exact ONNX SHA-256, and embeds the complete pack
SHA-256 in `gpu/model.noct`.  Initialization validates and loads the complete
pack before any upload, then uploads each initializer once.  The generated
sample main accepts exactly `MODEL.weights INPUT.f32le OUTPUT.f32le` and uses
only explicit paths, so source and `.nap` packages remain usable after
relocation.  `--emit-main=no` omits only the sample main.  The manifest records
source, pack, payload, source-file, operator, kernel, I/O, numeric-policy, and
backend identities and is written last as the package commit marker.

The test runner checks Stage D malformed protobuf/model fixtures, Stage E
normalization failures and 16 normalized goldens, three byte-exact Stage F
generated-source goldens, and two byte-exact Stage G.1 Conv goldens in
interpreter and forced-JIT modes.  It also checks three byte-exact Stage G.2
Gemm/MatMul goldens, including transposition, vector bias broadcasting,
non-default alpha/beta, optional C, and specialization reuse.  Its Python
fixture builders use only the standard library; the production converter is
Noct source and does not depend on Python or the `onnx` package.
`tests/test.sh onnx-package` additionally checks byte-repeated package
generation, identity binding, manifest hashes, source/`.nap` relocation,
interpreter/JIT execution on hardware OpenGL, ONNX Runtime comparison, and
preflight rejection of missing/corrupt weights, short input, and a disabled
accelerator before output publication.
If the locked third-party models are present under
`${ONNX_MODEL_CACHE:-${XDG_CACHE_HOME:-$HOME/.cache}/noct-onnx-models}`, the
test runner checks them as well.

Run the generated source and `.nap` OpenGL oracle gate with:

```sh
EGL_PLATFORM=surfaceless \
NOCT=./build-linux-opengl/noct \
CONVERTER_NOCT=./build-static/noct \
sh tests/test.sh onnx-gpu
```

Run the Stage G.1 Conv gate against the pinned ONNX Runtime oracle with:

```sh
EGL_PLATFORM=surfaceless \
NOCT=./build-linux-opengl/noct \
CONVERTER_NOCT=./build-static/noct \
ONNX_ORACLE_PYTHON=/tmp/noct-onnx-oracle/bin/python \
sh tests/test.sh onnx-conv
```

The Conv gate requires ONNX Runtime 1.22.1, checks source and `.nap` through
interpreter and forced-JIT host paths, rejects software renderers/CPU fallback,
checks specialization reuse by OpenGL pipeline count, and verifies disabled,
corrupt-pack, and wrong-entry-name failures before caller output publication.

## Run the locked model ladder

With the pinned model cache and ONNX Runtime environment present:

```sh
EGL_PLATFORM=surfaceless NOCT=./build-linux-opengl/noct \
CONVERTER_NOCT=./build-static/noct \
ONNX_ORACLE_PYTHON=/tmp/noct-onnx-oracle/bin/python \
sh tests/test.sh onnx-mnist

EGL_PLATFORM=surfaceless NOCT=./build-linux-opengl/noct \
CONVERTER_NOCT=./build-static/noct \
ONNX_ORACLE_PYTHON=/tmp/noct-onnx-oracle/bin/python \
sh tests/test.sh onnx-cifar

EGL_PLATFORM=surfaceless NOCT=./build-linux-opengl/noct \
CONVERTER_NOCT=./build-static/noct \
ONNX_ORACLE_PYTHON=/tmp/noct-onnx-oracle/bin/python \
sh tests/test.sh onnx-squeezenet

NOCT=./build-static/noct \
ONNX_ORACLE_PYTHON=/tmp/noct-onnx-oracle/bin/python \
sh tests/test.sh onnx-tinyyolo
```

MNIST-12, project CIFAR, and SqueezeNet each verify the exact model lock,
ONNX Runtime oracle, two byte-identical packages, manifest identities,
source/`.nap` relocation, interpreter/JIT host paths, hardware Intel OpenGL,
tensor tolerance, argmax, and bounded warmup/steady-state estimates.  The exact
Tiny YOLOv2 artifact intentionally remains a hard conversion rejection because
its declared batch is symbolic `None`; the test-only ORT check explicitly
concretizes only that batch to one and does not alter production conversion.

`tests/test.sh accel-serialization` relocates raw `.nb`/`.nap`
artifacts and ensures truncated descriptor containers fail rather than execute.

Run the Stage G.2 contraction gate with:

```sh
EGL_PLATFORM=surfaceless \
NOCT=./build-linux-opengl/noct \
CONVERTER_NOCT=./build-static/noct \
ONNX_ORACLE_PYTHON=/tmp/noct-onnx-oracle/bin/python \
sh tests/test.sh onnx-contraction
```

This applies the same hardware-renderer, no-fallback, source/`.nap`,
interpreter/JIT, pipeline reuse, disabled-backend, and malformed-weight gates
to the Gemm/MatMul models and ONNX Runtime 1.22.1 oracle.

Run the Stage G.3 pooling gate with:

```sh
EGL_PLATFORM=surfaceless \
NOCT=./build-linux-opengl/noct \
CONVERTER_NOCT=./build-static/noct \
ONNX_ORACLE_PYTHON=/tmp/noct-onnx-oracle/bin/python \
sh tests/test.sh onnx-pool
```
