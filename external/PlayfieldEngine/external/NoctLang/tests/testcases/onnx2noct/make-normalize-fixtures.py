#!/usr/bin/env python3
"""Build deterministic Stage-E ONNX fixtures without an ONNX dependency."""

import importlib.util
import struct
import sys
from pathlib import Path


HERE = Path(__file__).resolve().parent
SPEC = importlib.util.spec_from_file_location(
    "reader_fixtures", HERE / "make-reader-fixtures.py"
)
R = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(R)


def attr_int(name, value):
    return R.s(1, name) + R.v(20, 2) + R.v(3, value)


def attr_float(name, bits):
    return R.s(1, name) + R.v(20, 1) + R.f32(2, bits)


def attr_ints(name, values):
    return R.s(1, name) + R.v(20, 7) + R.b(
        8, b"".join(R.varint(x) for x in values)
    )


def attr_floats(name, values):
    return R.s(1, name) + R.v(20, 6) + R.b(
        7, b"".join(struct.pack("<I", x) for x in values)
    )


def attr_string(name, value):
    return R.s(1, name) + R.v(20, 3) + R.s(4, value)


def attr_tensor(name, value):
    return R.s(1, name) + R.v(20, 4) + R.b(5, value)


def int64_tensor(name, values):
    return R.tensor(
        name, dims=(len(values),), dtype=7, storage="raw",
        data=b"".join(struct.pack("<q", x) for x in values),
    )


def write(root, name, payload):
    (root / name).write_bytes(payload)


def error(root, errors, name, payload, message):
    write(root, name, payload)
    errors.append((name, message))


def main():
    root = Path(sys.argv[1])
    root.mkdir(parents=True, exist_ok=True)
    errors = []

    weird_input = "1 weird/名"
    transpose = R.node(
        (weird_input,), ('out "x"',), "Transpose",
        attrs=(attr_ints("perm", [0, 2, 1]),),
    )
    write(root, "valid-transpose-name.onnx", R.model(R.graph(
        nodes=[transpose], inputs=[R.value_info(weird_input, (1, 2, 3))],
        outputs=[R.value_info('out "x"', (1, 3, 2))],
    )))

    shape_node = R.node(("x",), ("shape",), "Shape")
    constant_node = R.node((), ("index",), "Constant", attrs=(
        attr_tensor("value", int64_tensor("", [1])),
    ))
    gather_node = R.node(("shape", "index"), ("selected",), "Gather")
    cast_node = R.node(("selected",), ("selected64",), "Cast", attrs=(
        attr_int("to", 7),
    ))
    reshape_node = R.node(("x", "shape"), ("y",), "Reshape")
    write(root, "valid-fold-chain.onnx", R.model(R.graph(
        nodes=[shape_node, constant_node, gather_node, cast_node, reshape_node],
        inputs=[R.value_info("x", (1, 2, 3))],
        outputs=[R.value_info("y", (1, 2, 3))],
    )))

    squeeze = R.node(("x",), ("s",), "Squeeze",
                     attrs=(attr_ints("axes", [1, -1]),))
    unsqueeze = R.node(("s",), ("y",), "Unsqueeze",
                       attrs=(attr_ints("axes", [1, 3]),))
    write(root, "valid-squeeze-unsqueeze.onnx", R.model(R.graph(
        nodes=[squeeze, unsqueeze], inputs=[R.value_info("x", (1, 1, 3, 1))],
        outputs=[R.value_info("y", (1, 1, 3, 1))],
    )))

    avg = R.node(("x",), ("y",), "AveragePool", attrs=(
        attr_ints("kernel_shape", [2, 2]),
        attr_ints("strides", [2, 2]),
        attr_int("count_include_pad", 1),
    ))
    write(root, "valid-average-pool.onnx", R.model(R.graph(
        nodes=[avg], inputs=[R.value_info("x", (1, 2, 4, 4))],
        outputs=[R.value_info("y", (1, 2, 2, 2))],
    )))

    maxpool = R.node(("x",), ("y",), "MaxPool", attrs=(
        attr_ints("kernel_shape", [2, 2]), attr_ints("strides", [2, 2]),
    ))
    pool_graph = R.graph(
        nodes=[maxpool], inputs=[R.value_info("x", (1, 1, 4, 4))],
        outputs=[R.value_info("y", (1, 1, 2, 2))],
    )
    write(root, "valid-maxpool-opset7.onnx", R.model(pool_graph, opset=7, ir=3))
    write(root, "valid-maxpool-opset8.onnx", R.model(pool_graph, opset=8, ir=4))

    constant_floats = R.node((), ("w",), "Constant", attrs=(
        attr_floats("value_floats", [0x3F800000, 0x40000000]),
    ))
    pointwise_nodes = [constant_floats]
    previous = "x"
    for index, op_type in enumerate(("Sub", "Div", "Pow")):
        output = f"p{index}"
        pointwise_nodes.append(R.node((previous, "w"), (output,), op_type))
        previous = output
    for index, op_type in enumerate(("Tanh", "Exp", "Log", "Sqrt"), 3):
        output = f"p{index}"
        pointwise_nodes.append(R.node((previous,), (output,), op_type))
        previous = output
    minimum = R.tensor("minimum", dims=(1,), data=struct.pack("<I", 0xBF800000))
    maximum = R.tensor("maximum", dims=(1,), data=struct.pack("<I", 0x3F800000))
    pointwise_nodes.append(R.node((previous, "minimum", "maximum"), ("y",), "Clip"))
    write(root, "valid-pointwise-family.onnx", R.model(R.graph(
        nodes=pointwise_nodes, initializers=[minimum, maximum],
        inputs=[R.value_info("x", (1, 2))],
        outputs=[R.value_info("y", (1, 2))],
    )))

    reductions = [
        R.node(("x",), ("g",), "GlobalAveragePool"),
        R.node(("g",), ("mean",), "ReduceMean", attrs=(
            attr_ints("axes", [3]), attr_int("keepdims", 0),
        )),
        R.node(("mean",), ("max",), "ReduceMax", attrs=(
            attr_ints("axes", [2]),
        )),
        R.node(("max",), ("min",), "ReduceMin", attrs=(
            attr_ints("axes", [2]),
        )),
        R.node(("min",), ("softmax",), "Softmax", attrs=(attr_int("axis", 1),)),
        R.node(("softmax",), ("y",), "LogSoftmax", attrs=(attr_int("axis", 1),)),
    ]
    write(root, "valid-reduction-family.onnx", R.model(R.graph(
        nodes=reductions, inputs=[R.value_info("x", (1, 2, 3, 4))],
        outputs=[R.value_info("y", (1, 2, 1))],
    )))

    mismatch = R.model(R.graph(outputs=[R.value_info("y", (1, 5))]))
    error(root, errors, "shape-mismatch.onnx", mismatch,
          "inferred shape differs from its declaration")

    bad_broadcast = R.tensor("w", dims=(1, 2), data=bytes(8))
    error(root, errors, "bad-broadcast.onnx", R.model(R.graph(
        nodes=[R.node(("x", "w"), ("y",), "Add")],
        initializers=[bad_broadcast], inputs=[R.value_info("x", (1, 3))],
        outputs=[R.value_info("y", (1, 3))],
    )), "incompatible broadcast dimensions")

    bad_reshape_shape = int64_tensor("shape", [1, 5])
    error(root, errors, "reshape-count.onnx", R.model(R.graph(
        nodes=[R.node(("x", "shape"), ("y",), "Reshape")],
        initializers=[bad_reshape_shape], inputs=[R.value_info("x", (1, 6))],
        outputs=[R.value_info("y", (1, 5))],
    )), "reshape changes element count")

    bad_transpose = R.node(("x",), ("y",), "Transpose",
                           attrs=(attr_ints("perm", [0, 0, 1]),))
    error(root, errors, "transpose-perm.onnx", R.model(R.graph(
        nodes=[bad_transpose], inputs=[R.value_info("x", (1, 2, 3))],
        outputs=[R.value_info("y", (1, 2, 3))],
    )), "perm is not a full permutation")

    wrong_attr = R.node(("x",), ("y",), "Flatten",
                        attrs=(attr_float("axis", 0x3f800000),))
    error(root, errors, "wrong-attr-type.onnx", R.model(R.graph(
        nodes=[wrong_attr], inputs=[R.value_info("x", (1, 4))],
        outputs=[R.value_info("y", (1, 4))],
    )), "attribute 'axis' must be INT")

    unknown_attr = R.node(("x",), ("y",), "Relu",
                          attrs=(attr_int("surprise", 1),))
    error(root, errors, "unknown-attribute.onnx", R.model(R.graph(
        nodes=[unknown_attr], inputs=[R.value_info("x", (1, 4))],
        outputs=[R.value_info("y", (1, 4))],
    )), "unsupported attribute 'surprise'")

    unknown_operator = R.node(("x",), ("y",), "NotARealOperator")
    error(root, errors, "unknown-operator.onnx", R.model(R.graph(
        nodes=[unknown_operator], inputs=[R.value_info("x", (1, 4))],
        outputs=[R.value_info("y", (1, 4))],
    )), "no exact opset handler is registered")

    indices_pool = R.node(("x",), ("y", "indices"), "MaxPool", attrs=(
        attr_ints("kernel_shape", [2, 2]),
    ))
    error(root, errors, "maxpool-indices.onnx", R.model(R.graph(
        nodes=[indices_pool], inputs=[R.value_info("x", (1, 1, 4, 4))],
        outputs=[R.value_info("y", (1, 1, 3, 3))],
    )), "optional output position 1 is unsupported")

    dropout_mask = R.node(("x",), ("y", "mask"), "Dropout")
    error(root, errors, "dropout-mask.onnx", R.model(R.graph(
        nodes=[dropout_mask], inputs=[R.value_info("x", (1, 4))],
        outputs=[R.value_info("y", (1, 4))],
    )), "optional output position 1 is unsupported")

    grouped_w = R.tensor("w", dims=(4, 1, 3, 3), data=bytes(4 * 1 * 3 * 3 * 4))
    grouped = R.node(("x", "w"), ("y",), "Conv", attrs=(
        attr_int("group", 2), attr_ints("kernel_shape", [3, 3]),
    ))
    error(root, errors, "conv-group.onnx", R.model(R.graph(
        nodes=[grouped], initializers=[grouped_w],
        inputs=[R.value_info("x", (1, 2, 5, 5))],
        outputs=[R.value_info("y", (1, 4, 3, 3))],
    )), "supports group=1 only")

    axis_reduce = R.node(("x",), ("y",), "ReduceSum", attrs=(
        attr_ints("axes", [3]),
    ))
    error(root, errors, "axis-range.onnx", R.model(R.graph(
        nodes=[axis_reduce], inputs=[R.value_info("x", (1, 2, 3))],
        outputs=[R.value_info("y", (1, 2, 1))],
    )), "axis 3 is out of range")

    old_ceil = R.node(("x",), ("y",), "MaxPool", attrs=(
        attr_ints("kernel_shape", [2, 2]), attr_int("ceil_mode", 0),
    ))
    error(root, errors, "opset8-ceil-mode.onnx", R.model(R.graph(
        nodes=[old_ceil], inputs=[R.value_info("x", (1, 1, 4, 4))],
        outputs=[R.value_info("y", (1, 1, 3, 3))],
    ), opset=8, ir=4), "unsupported attribute 'ceil_mode'")

    new_dropout_ratio = R.node(("x",), ("y",), "Dropout", attrs=(
        attr_float("ratio", 0x3F000000),
    ))
    error(root, errors, "opset12-dropout-ratio-attr.onnx", R.model(R.graph(
        nodes=[new_dropout_ratio], inputs=[R.value_info("x", (1, 4))],
        outputs=[R.value_info("y", (1, 4))],
    )), "unsupported attribute 'ratio'")

    huge_pad = R.node(("x",), ("y",), "MaxPool", attrs=(
        attr_ints("kernel_shape", [2, 2]),
        attr_ints("pads", [2147483648, 0, 0, 0]),
    ))
    error(root, errors, "pad-range.onnx", R.model(R.graph(
        nodes=[huge_pad], inputs=[R.value_info("x", (1, 1, 4, 4))],
        outputs=[R.value_info("y", (1, 1, 3, 3))],
    )), "pad value exceeds INT_MAX")

    with (root / "errors.tsv").open("w", encoding="utf-8", newline="\n") as out:
        for name, message in errors:
            out.write(f"{name}\t{message}\n")


if __name__ == "__main__":
    main()
