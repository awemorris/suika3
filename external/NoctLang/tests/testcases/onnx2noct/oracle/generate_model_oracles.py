#!/usr/bin/env python3
"""Generate deterministic float32 model inputs and ONNX Runtime outputs."""

from __future__ import annotations

import argparse
import hashlib
import json
import math
import pathlib
from typing import Any

import numpy as np
import onnx
import onnxruntime as ort


def sha256(path: pathlib.Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def concrete_shape(shape: list[Any]) -> list[int]:
    result = []
    for index, dimension in enumerate(shape):
        if isinstance(dimension, int) and dimension > 0:
            result.append(dimension)
        elif index == 0:
            result.append(1)
        else:
            raise ValueError(f"non-batch dynamic dimension is unsupported: {shape!r}")
    return result


def deterministic_input(model_id: str, shape: list[int]) -> np.ndarray:
    count = math.prod(shape)
    salt = sum(model_id.encode("utf-8")) % 257
    indices = np.arange(count, dtype=np.int64)
    values = (((indices * 37 + salt) % 257) - 128).astype(np.float32) / np.float32(128.0)
    return values.reshape(shape)


def write_f32(path: pathlib.Path, array: np.ndarray) -> None:
    path.write_bytes(np.asarray(array, dtype="<f4").tobytes(order="C"))


def stats(array: np.ndarray) -> dict[str, Any]:
    flat = np.asarray(array, dtype=np.float32).reshape(-1)
    finite = flat[np.isfinite(flat)]
    return {
        "elements": int(flat.size),
        "finite_elements": int(finite.size),
        "nan_elements": int(np.isnan(flat).sum()),
        "positive_inf_elements": int(np.isposinf(flat).sum()),
        "negative_inf_elements": int(np.isneginf(flat).sum()),
        "minimum": None if finite.size == 0 else float(finite.min()),
        "maximum": None if finite.size == 0 else float(finite.max()),
        "mean": None if finite.size == 0 else float(finite.astype(np.float64).mean()),
        "argmax": None if flat.size == 0 or np.isnan(flat).any() else int(np.argmax(flat)),
        "first_values": [float(value) for value in flat[:10]],
    }


def parse_model(value: str) -> tuple[str, pathlib.Path]:
    if "=" not in value:
        raise argparse.ArgumentTypeError("model must be ID=PATH")
    model_id, path = value.split("=", 1)
    if not model_id:
        raise argparse.ArgumentTypeError("model ID is empty")
    return model_id, pathlib.Path(path)


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("output", type=pathlib.Path)
    parser.add_argument("--model", action="append", type=parse_model, required=True)
    args = parser.parse_args()
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=True)
    records = []
    for model_id, model_path in sorted(args.model):
        model_path = model_path.resolve()
        model = onnx.load(model_path, load_external_data=False)
        initializer_names = {item.name for item in model.graph.initializer}
        graph_inputs = [item for item in model.graph.input if item.name not in initializer_names]
        if len(graph_inputs) != 1 or len(model.graph.output) != 1:
            raise RuntimeError(f"{model_id}: expected one graph input/output")
        session = ort.InferenceSession(str(model_path), providers=["CPUExecutionProvider"])
        if len(session.get_inputs()) != 1 or len(session.get_outputs()) != 1:
            raise RuntimeError(f"{model_id}: expected one runtime input/output")
        runtime_input = session.get_inputs()[0]
        runtime_output = session.get_outputs()[0]
        input_shape = concrete_shape(list(runtime_input.shape))
        input_array = deterministic_input(model_id, input_shape)
        outputs = session.run([runtime_output.name], {runtime_input.name: input_array})
        output_array = np.asarray(outputs[0], dtype=np.float32)
        input_path = output / f"{model_id}.input.f32le"
        output_path = output / f"{model_id}.output.f32le"
        write_f32(input_path, input_array)
        write_f32(output_path, output_array)
        records.append({
            "id": model_id,
            "model_file": model_path.name,
            "model_byte_size": model_path.stat().st_size,
            "model_sha256": sha256(model_path),
            "input": {
                "name": runtime_input.name,
                "declared_shape": list(runtime_input.shape),
                "concrete_shape": input_shape,
                "file": input_path.name,
                "byte_size": input_path.stat().st_size,
                "sha256": sha256(input_path),
                "stats": stats(input_array),
            },
            "output": {
                "name": runtime_output.name,
                "declared_shape": list(runtime_output.shape),
                "concrete_shape": list(output_array.shape),
                "file": output_path.name,
                "byte_size": output_path.stat().st_size,
                "sha256": sha256(output_path),
                "stats": stats(output_array),
            },
        })
    lock = {
        "schema": 1,
        "generator": "tests/testcases/onnx2noct/oracle/generate_model_oracles.py",
        "input_rule": "float32((((linear_index * 37 + utf8_id_sum) % 257) - 128) / 128)",
        "oracle": {
            "python": ".".join(map(str, __import__("sys").version_info[:3])),
            "numpy": np.__version__,
            "onnx": onnx.__version__,
            "onnxruntime": ort.__version__,
            "providers": ["CPUExecutionProvider"],
        },
        "models": records,
    }
    (output / "model-oracles.lock").write_text(
        json.dumps(lock, indent=2, sort_keys=True) + "\n", encoding="utf-8"
    )


if __name__ == "__main__":
    main()
