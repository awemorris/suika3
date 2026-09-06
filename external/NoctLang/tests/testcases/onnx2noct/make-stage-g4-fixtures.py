#!/usr/bin/env python3
"""Build deterministic Stage-G.4 Concat fixtures."""

import importlib.util
import struct
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
SPEC = importlib.util.spec_from_file_location(
    "stage_g1_fixtures", HERE / "make-stage-g1-fixtures.py"
)
G1 = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(G1)
R = G1.R


def attr_int(name, value):
    return R.s(1, name) + R.v(20, 2) + R.v(3, value)


def write_fixture(root, name, model, values, expected):
    (root / f"{name}.onnx").write_bytes(model)
    (root / f"{name}.input.f32le").write_bytes(struct.pack(f"<{len(values)}f", *values))
    (root / f"{name}.output.f32le").write_bytes(struct.pack(f"<{len(expected)}f", *expected))


def main():
    root = Path(sys.argv[1]); root.mkdir(parents=True, exist_ok=True)
    values = [1.0, 2.0, 3.0, 4.0]
    transpose = R.node(("x",), ("xt",), "Transpose", attrs=(
        G1.attr_ints("perm", [0, 2, 1]),
    ))
    concat = R.node(("x", "xt", "x"), ("y",), "Concat", attrs=(
        attr_int("axis", 1),
    ))
    model = R.model(R.graph(
        nodes=[transpose, concat],
        inputs=[R.value_info("x", (1, 2, 2))],
        outputs=[R.value_info("y", (1, 6, 2))],
    ))
    expected = values + [1.0, 3.0, 2.0, 4.0] + values
    write_fixture(root, "concat-strided-alias", model, values, expected)

    last = R.node(("x", "x"), ("y",), "Concat", attrs=(attr_int("axis", -1),))
    last_model = R.model(R.graph(
        nodes=[last], inputs=[R.value_info("x", (1, 2, 2))],
        outputs=[R.value_info("y", (1, 2, 4))],
    ))
    last_expected = [1.0, 2.0, 1.0, 2.0, 3.0, 4.0, 3.0, 4.0]
    write_fixture(root, "concat-axis-last", last_model, values, last_expected)

    identity_nodes = []
    names = []
    for index in range(33):
        name = f"v{index}"
        identity_nodes.append(R.node(("x",), (name,), "Identity"))
        names.append(name)
    too_many = R.node(tuple(names), ("y",), "Concat", attrs=(attr_int("axis", 1),))
    (root / "unsupported-concat-33.onnx").write_bytes(R.model(R.graph(
        nodes=identity_nodes + [too_many],
        inputs=[R.value_info("x", (1, 1, 1))],
        outputs=[R.value_info("y", (1, 33, 1))],
    )))


if __name__ == "__main__": main()
