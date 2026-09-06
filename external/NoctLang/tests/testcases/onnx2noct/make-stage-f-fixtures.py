#!/usr/bin/env python3
"""Build deterministic Stage-F pointwise/view models and float32 oracles."""

import importlib.util
import math
import struct
import sys
from pathlib import Path


HERE = Path(__file__).resolve().parent
SPEC = importlib.util.spec_from_file_location(
    "reader_fixtures", HERE / "make-reader-fixtures.py"
)
R = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(R)


def attr_ints(name, values):
    return R.s(1, name) + R.v(20, 7) + R.b(
        8, b"".join(R.varint(x) for x in values)
    )


def int64_tensor(name, values):
    return R.tensor(
        name, dims=(len(values),), dtype=7, storage="raw",
        data=b"".join(struct.pack("<q", value) for value in values),
    )


def write_f32(path, values):
    path.write_bytes(b"".join(struct.pack("<f", value) for value in values))


def main():
    root = Path(sys.argv[1])
    root.mkdir(parents=True, exist_ok=True)

    unary_nodes = [
        R.node(("x",), ("sigmoid",), "Sigmoid"),
        R.node(("sigmoid",), ("relu1",), "Relu"),
        R.node(("relu1",), ("y",), "Relu"),
    ]
    (root / "unary.onnx").write_bytes(R.model(R.graph(
        nodes=unary_nodes, inputs=[R.value_info("x", (1, 8))],
        outputs=[R.value_info("y", (1, 8))],
    )))
    unary_input = [-8.0, -4.0, -1.0, -0.0, 0.0, 1.0, 4.0, 8.0]
    unary_output = [1.0 / (1.0 + math.exp(-value)) for value in unary_input]
    write_f32(root / "unary.input.f32le", unary_input)
    write_f32(root / "unary.output.f32le", unary_output)

    reshape_shape = int64_tensor("shape", [1, 3, 1])
    broadcast_nodes = [
        R.node(("x", "shape"), ("column",), "Reshape"),
        R.node(("x", "column"), ("y",), "Add"),
    ]
    (root / "broadcast-alias.onnx").write_bytes(R.model(R.graph(
        nodes=broadcast_nodes, initializers=[reshape_shape],
        inputs=[R.value_info("x", (1, 3))],
        outputs=[R.value_info("y", (1, 3, 3))],
    )))
    broadcast_input = [1.0, 2.0, 3.0]
    broadcast_output = [left + right for left in broadcast_input
                        for right in broadcast_input]
    write_f32(root / "broadcast-alias.input.f32le", broadcast_input)
    write_f32(root / "broadcast-alias.output.f32le", broadcast_output)

    transpose = R.node(("x",), ("y",), "Transpose", attrs=(
        attr_ints("perm", [0, 2, 1]),
    ))
    (root / "transpose-copy.onnx").write_bytes(R.model(R.graph(
        nodes=[transpose], inputs=[R.value_info("x", (1, 2, 3))],
        outputs=[R.value_info("y", (1, 3, 2))],
    )))
    transpose_input = [1.0, 2.0, 3.0, 4.0, 5.0, 6.0]
    transpose_output = [1.0, 4.0, 2.0, 5.0, 3.0, 6.0]
    write_f32(root / "transpose-copy.input.f32le", transpose_input)
    write_f32(root / "transpose-copy.output.f32le", transpose_output)

    (root / "unsupported-exp.onnx").write_bytes(R.model(R.graph(
        nodes=[R.node(("x",), ("y",), "Exp")],
        inputs=[R.value_info("x", (1, 3))],
        outputs=[R.value_info("y", (1, 3))],
    )))
    weight = R.tensor("w", dims=(3,), data=struct.pack("<3f", 1.0, 2.0, 3.0))
    (root / "unsupported-weight.onnx").write_bytes(R.model(R.graph(
        nodes=[R.node(("x", "w"), ("y",), "Add")], initializers=[weight],
        inputs=[R.value_info("x", (1, 3))],
        outputs=[R.value_info("y", (1, 3))],
    )))
    unused = R.tensor("unused", dims=(1,), data=struct.pack("<f", 7.0))
    (root / "unsupported-unused-weight.onnx").write_bytes(R.model(R.graph(
        nodes=[R.node(("x",), ("y",), "Identity")], initializers=[unused],
        inputs=[R.value_info("x", (1, 3))],
        outputs=[R.value_info("y", (1, 3))],
    )))


if __name__ == "__main__":
    main()
