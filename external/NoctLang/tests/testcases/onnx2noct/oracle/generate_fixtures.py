#!/usr/bin/env python3
"""Generate deterministic project-owned ONNX fixtures and float32 oracles."""

from __future__ import annotations

import argparse
import hashlib
import json
import pathlib
from typing import Iterable

import numpy as np
import onnx
import onnxruntime as ort
from onnx import TensorProto, helper, numpy_helper


PRODUCER = "NoctLang Stage A fixture generator"
PRODUCER_VERSION = "1"


def model(graph: onnx.GraphProto, *, opset: int = 12,
          extra_opsets: Iterable[onnx.OperatorSetIdProto] = ()) -> onnx.ModelProto:
    imports = [helper.make_opsetid("", opset), *extra_opsets]
    result = helper.make_model(
        graph,
        producer_name=PRODUCER,
        producer_version=PRODUCER_VERSION,
        opset_imports=imports,
        ir_version=7,
    )
    result.model_version = 1
    return result


def save_model(path: pathlib.Path, value: onnx.ModelProto, *, check: bool = True) -> None:
    if check:
        onnx.checker.check_model(value, full_check=True)
    path.write_bytes(value.SerializeToString(deterministic=True))


def f32(path: pathlib.Path, value: np.ndarray) -> None:
    array = np.asarray(value, dtype="<f4")
    path.write_bytes(array.tobytes(order="C"))


def value(name: str, shape: list[int | str | None]) -> onnx.ValueInfoProto:
    return helper.make_tensor_value_info(name, TensorProto.FLOAT, shape)


def initializer(name: str, array: np.ndarray) -> onnx.TensorProto:
    return numpy_helper.from_array(np.asarray(array), name=name)


def run_reference(model_path: pathlib.Path, input_value: np.ndarray) -> np.ndarray:
    session = ort.InferenceSession(
        str(model_path), providers=["CPUExecutionProvider"]
    )
    input_name = session.get_inputs()[0].name
    output = session.run(None, {input_name: np.asarray(input_value, dtype=np.float32)})
    if len(output) != 1:
        raise RuntimeError(f"{model_path.name}: expected one output")
    return np.asarray(output[0], dtype=np.float32)


def write_valid(root: pathlib.Path, name: str, graph: onnx.GraphProto,
                input_value: np.ndarray) -> None:
    model_path = root / "models" / f"{name}.onnx"
    save_model(model_path, model(graph))
    output_value = run_reference(model_path, input_value)
    f32(root / "data" / f"{name}.input.f32le", input_value)
    f32(root / "data" / f"{name}.output.f32le", output_value)


def make_micro_fixtures(root: pathlib.Path) -> None:
    write_valid(
        root,
        "identity-opset12",
        helper.make_graph(
            [helper.make_node("Identity", ["x"], ["y"], name="identity")],
            "identity-opset12",
            [value("x", [1, 4])],
            [value("y", [1, 4])],
        ),
        np.array([[1.0, -2.0, 3.5, -0.0]], dtype=np.float32),
    )

    add_bias = np.array([[10.0], [-2.0]], dtype=np.float32)
    write_valid(
        root,
        "add-broadcast-opset12",
        helper.make_graph(
            [helper.make_node("Add", ["x", "bias"], ["y"], name="add")],
            "add-broadcast-opset12",
            [value("x", [1, 2, 3])],
            [value("y", [1, 2, 3])],
            [initializer("bias", add_bias)],
        ),
        np.array([[[1, 2, 3], [4, 5, 6]]], dtype=np.float32),
    )

    write_valid(
        root,
        "sigmoid-opset12",
        helper.make_graph(
            [helper.make_node("Sigmoid", ["x"], ["y"], name="sigmoid")],
            "sigmoid-opset12",
            [value("x", [1, 6])],
            [value("y", [1, 6])],
        ),
        np.array([[-4.0, -1.0, -0.0, 0.0, 1.0, 4.0]], dtype=np.float32),
    )

    write_valid(
        root,
        "reduce-sum-opset12",
        helper.make_graph(
            [helper.make_node(
                "ReduceSum", ["x"], ["y"], name="reduce",
                axes=[1], keepdims=1,
            )],
            "reduce-sum-opset12",
            [value("x", [1, 2, 3])],
            [value("y", [1, 1, 3])],
        ),
        np.arange(1, 7, dtype=np.float32).reshape(1, 2, 3),
    )

    conv_weight = np.array(
        [[[[1, 0, -1], [1, 0, -1], [1, 0, -1]]]], dtype=np.float32
    )
    conv_bias = np.array([0.5], dtype=np.float32)
    write_valid(
        root,
        "conv2d-opset12",
        helper.make_graph(
            [helper.make_node(
                "Conv", ["x", "weight", "bias"], ["y"], name="conv",
                kernel_shape=[3, 3], pads=[0, 0, 0, 0], strides=[1, 1],
            )],
            "conv2d-opset12",
            [value("x", [1, 1, 4, 4])],
            [value("y", [1, 1, 2, 2])],
            [initializer("weight", conv_weight), initializer("bias", conv_bias)],
        ),
        np.arange(1, 17, dtype=np.float32).reshape(1, 1, 4, 4),
    )

    write_valid(
        root,
        "maxpool-opset12",
        helper.make_graph(
            [helper.make_node(
                "MaxPool", ["x"], ["y"], name="pool",
                kernel_shape=[2, 2], strides=[2, 2],
            )],
            "maxpool-opset12",
            [value("x", [1, 1, 4, 4])],
            [value("y", [1, 1, 2, 2])],
        ),
        np.array(
            [[[[1, 5, 2, 4], [3, 0, 8, 7], [9, 2, 6, 1], [4, 3, 5, 0]]]],
            dtype=np.float32,
        ),
    )

    reshape_shape = np.array([1, 2, 3], dtype=np.int64)
    write_valid(
        root,
        "reshape-opset12",
        helper.make_graph(
            [helper.make_node("Reshape", ["x", "shape"], ["y"], name="reshape")],
            "reshape-opset12",
            [value("x", [1, 6])],
            [value("y", [1, 2, 3])],
            [initializer("shape", reshape_shape)],
        ),
        np.arange(1, 7, dtype=np.float32).reshape(1, 6),
    )


def make_error_fixtures(root: pathlib.Path) -> None:
    dynamic = model(helper.make_graph(
        [helper.make_node("Identity", ["x"], ["y"], name="identity")],
        "error-dynamic-shape-opset12",
        [value("x", [1, "width"])],
        [value("y", [1, "width"])],
    ))
    save_model(root / "models" / "error-dynamic-shape-opset12.onnx", dynamic)

    external = onnx.TensorProto()
    external.name = "weight"
    external.data_type = TensorProto.FLOAT
    external.dims.extend([1, 4])
    external.data_location = TensorProto.EXTERNAL
    entry = external.external_data.add()
    entry.key = "location"
    entry.value = "forbidden-external-data.bin"
    external_model = model(helper.make_graph(
        [helper.make_node("Add", ["x", "weight"], ["y"], name="add")],
        "error-external-data-opset12",
        [value("x", [1, 4])],
        [value("y", [1, 4])],
        [external],
    ))
    save_model(
        root / "models" / "error-external-data-opset12.onnx",
        external_model,
        check=False,
    )

    custom = model(
        helper.make_graph(
            [helper.make_node(
                "Unsupported", ["x"], ["y"], name="custom",
                domain="com.noct.fixture",
            )],
            "error-custom-domain-opset12",
            [value("x", [1, 4])],
            [value("y", [1, 4])],
        ),
        extra_opsets=[helper.make_opsetid("com.noct.fixture", 1)],
    )
    save_model(root / "models" / "error-custom-domain-opset12.onnx", custom)

    valid = (root / "models" / "identity-opset12.onnx").read_bytes()
    (root / "models" / "error-truncated-protobuf.onnx").write_bytes(valid[:-1])


def make_project_cifar(root: pathlib.Path) -> None:
    conv1_w = np.zeros((6, 3, 5, 5), dtype=np.float32)
    conv1_w.reshape(6, 75)[:, 12] = 1.0
    conv1_b = np.zeros(6, dtype=np.float32)
    conv2_w = np.zeros((16, 6, 5, 5), dtype=np.float32)
    conv2_w.reshape(16, 150)[:, 12] = 1.0
    conv2_b = np.zeros(16, dtype=np.float32)
    fc1_w = np.zeros((120, 400), dtype=np.float32)
    fc1_w[:, 0] = 1.0
    fc1_b = np.zeros(120, dtype=np.float32)
    fc2_w = np.zeros((84, 120), dtype=np.float32)
    fc2_w[:, 0] = 1.0
    fc2_b = np.zeros(84, dtype=np.float32)
    fc3_w = np.zeros((10, 84), dtype=np.float32)
    fc3_w[:, 0] = 1.0
    fc3_b = np.arange(10, dtype=np.float32)

    initializers = [
        initializer("conv1_w", conv1_w), initializer("conv1_b", conv1_b),
        initializer("conv2_w", conv2_w), initializer("conv2_b", conv2_b),
        initializer("fc1_w", fc1_w), initializer("fc1_b", fc1_b),
        initializer("fc2_w", fc2_w), initializer("fc2_b", fc2_b),
        initializer("fc3_w", fc3_w), initializer("fc3_b", fc3_b),
    ]
    nodes = [
        helper.make_node("Conv", ["input", "conv1_w", "conv1_b"], ["conv1"],
                         name="conv1", kernel_shape=[5, 5]),
        helper.make_node("Relu", ["conv1"], ["relu1"], name="relu1"),
        helper.make_node("MaxPool", ["relu1"], ["pool1"], name="pool1",
                         kernel_shape=[2, 2], strides=[2, 2]),
        helper.make_node("Conv", ["pool1", "conv2_w", "conv2_b"], ["conv2"],
                         name="conv2", kernel_shape=[5, 5]),
        helper.make_node("Relu", ["conv2"], ["relu2"], name="relu2"),
        helper.make_node("MaxPool", ["relu2"], ["pool2"], name="pool2",
                         kernel_shape=[2, 2], strides=[2, 2]),
        helper.make_node("Flatten", ["pool2"], ["flat"], name="flatten", axis=1),
        helper.make_node("Gemm", ["flat", "fc1_w", "fc1_b"], ["fc1"],
                         name="fc1", transB=1),
        helper.make_node("Relu", ["fc1"], ["relu3"], name="relu3"),
        helper.make_node("Gemm", ["relu3", "fc2_w", "fc2_b"], ["fc2"],
                         name="fc2", transB=1),
        helper.make_node("Relu", ["fc2"], ["relu4"], name="relu4"),
        helper.make_node("Gemm", ["relu4", "fc3_w", "fc3_b"], ["logits"],
                         name="fc3", transB=1),
    ]
    graph = helper.make_graph(
        nodes,
        "noct-project-cifar-opset12",
        [value("input", [1, 3, 32, 32])],
        [value("logits", [1, 10])],
        initializers,
    )
    model_path = root / "models" / "project-cifar-opset12.onnx"
    save_model(model_path, model(graph))
    input_value = np.ones((1, 3, 32, 32), dtype=np.float32)
    output_value = run_reference(model_path, input_value)
    expected = np.arange(1, 11, dtype=np.float32).reshape(1, 10)
    if not np.array_equal(output_value, expected):
        raise RuntimeError(f"project CIFAR oracle mismatch: {output_value!r}")
    f32(root / "data" / "project-cifar-opset12.input.f32le", input_value)
    f32(root / "data" / "project-cifar-opset12.output.f32le", output_value)


def sha256(path: pathlib.Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def write_lock(root: pathlib.Path) -> None:
    records = []
    for path in sorted(p for p in root.rglob("*") if p.is_file()
                       and p.name != "fixtures.lock"):
        records.append({
            "path": path.relative_to(root).as_posix(),
            "byte_size": path.stat().st_size,
            "sha256": sha256(path),
        })
    payload = {
        "schema": 1,
        "generator": "tests/testcases/onnx2noct/oracle/generate_fixtures.py",
        "files": records,
    }
    (root / "fixtures.lock").write_text(
        json.dumps(payload, indent=2, sort_keys=True) + "\n", encoding="utf-8"
    )


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("output", type=pathlib.Path)
    args = parser.parse_args()
    root = args.output.resolve()
    (root / "models").mkdir(parents=True, exist_ok=True)
    (root / "data").mkdir(parents=True, exist_ok=True)
    for child in (root / "models").iterdir():
        if child.is_file():
            child.unlink()
    for child in (root / "data").iterdir():
        if child.is_file():
            child.unlink()
    make_micro_fixtures(root)
    make_error_fixtures(root)
    make_project_cifar(root)
    write_lock(root)


if __name__ == "__main__":
    main()
