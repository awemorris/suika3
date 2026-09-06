#!/usr/bin/env python3
"""Verify a checked f32le oracle against ONNX Runtime CPU execution."""

import sys
from pathlib import Path

import numpy as np
import onnxruntime as ort


model_path, input_path, expected_path = sys.argv[1:4]
allow_symbolic_batch = (
    len(sys.argv) == 5 and sys.argv[4] == "--allow-symbolic-batch-one"
)
session = ort.InferenceSession(model_path, providers=["CPUExecutionProvider"])
input_meta = session.get_inputs()[0]
shape = []
for axis, dimension in enumerate(input_meta.shape):
    if isinstance(dimension, int):
        shape.append(dimension)
    elif allow_symbolic_batch and axis == 0:
        shape.append(1)
    else:
        raise SystemExit(
            f"symbolic input dimension requires explicit batch-one opt-in: {dimension!r}"
        )
shape = tuple(shape)
values = np.fromfile(input_path, dtype="<f4").reshape(shape)
actual = session.run(None, {input_meta.name: values})[0].astype(np.float32).ravel()
expected = np.fromfile(expected_path, dtype="<f4")
if actual.shape != expected.shape:
    raise SystemExit(f"shape differs: {actual.shape} != {expected.shape}")
if not np.allclose(actual, expected, rtol=2e-5, atol=1e-4, equal_nan=True):
    index = int(np.argmax(np.abs(actual - expected)))
    raise SystemExit(
        f"ONNX Runtime mismatch at {index}: {actual[index]} != {expected[index]}"
    )
print(f"onnxruntime-ok version={ort.__version__} elements={actual.size}")
