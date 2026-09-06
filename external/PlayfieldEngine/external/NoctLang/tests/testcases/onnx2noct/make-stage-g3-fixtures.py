#!/usr/bin/env python3
"""Build deterministic Stage-G.3 pooling models and float32 oracles."""

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


def pool2d(values, shape, kernel, pads, strides, mode, include_pad=0):
    batches, channels, height, width = shape
    out_h = (height + pads[0] + pads[2] - kernel[0]) // strides[0] + 1
    out_w = (width + pads[1] + pads[3] - kernel[1]) // strides[1] + 1
    result = []
    for batch in range(batches):
        for channel in range(channels):
            for oy in range(out_h):
                for ox in range(out_w):
                    window = []
                    for ky in range(kernel[0]):
                        for kx in range(kernel[1]):
                            iy = oy * strides[0] + ky - pads[0]
                            ix = ox * strides[1] + kx - pads[1]
                            if 0 <= iy < height and 0 <= ix < width:
                                index = ((batch * channels + channel) * height + iy) * width + ix
                                window.append(values[index])
                    if mode == "max":
                        result.append(max(window))
                    else:
                        divisor = kernel[0] * kernel[1] if include_pad else len(window)
                        result.append(sum(window) / divisor)
    return result, (batches, channels, out_h, out_w)


def write_fixture(root, name, model, input_values, output_values):
    (root / f"{name}.onnx").write_bytes(model)
    (root / f"{name}.input.f32le").write_bytes(
        struct.pack(f"<{len(input_values)}f", *input_values)
    )
    (root / f"{name}.output.f32le").write_bytes(
        struct.pack(f"<{len(output_values)}f", *output_values)
    )


def main():
    root = Path(sys.argv[1])
    root.mkdir(parents=True, exist_ok=True)

    max_input = [-5.0, -2.0, -7.0, -4.0, 1.0, -3.0,
                 -8.0, -6.0, 2.0, -9.0, -1.0, -10.0]
    max_attrs = (
        G1.attr_ints("kernel_shape", [2, 3]),
        G1.attr_ints("pads", [1, 1, 0, 1]),
        G1.attr_ints("strides", [1, 2]),
    )
    max_model = R.model(R.graph(
        nodes=[R.node(("x",), ("y",), "MaxPool", attrs=max_attrs)],
        inputs=[R.value_info("x", (1, 1, 3, 4))],
        outputs=[R.value_info("y", (1, 1, 3, 2))],
    ))
    max_output, _ = pool2d(max_input, (1, 1, 3, 4), (2, 3),
                           (1, 1, 0, 1), (1, 2), "max")
    write_fixture(root, "maxpool-padding", max_model, max_input, max_output)

    average_input = [1.0, 2.0, 3.0, 4.0]
    common = (
        G1.attr_ints("kernel_shape", [2, 2]),
        G1.attr_ints("pads", [1, 1, 0, 0]),
        G1.attr_ints("strides", [1, 1]),
    )
    exclude = R.node(("x",), ("exclude",), "AveragePool", attrs=common + (
        attr_int("count_include_pad", 0),
    ))
    include = R.node(("x",), ("include",), "AveragePool", attrs=common + (
        attr_int("count_include_pad", 1),
    ))
    average_model = R.model(R.graph(
        nodes=[exclude, include, R.node(("exclude", "include"), ("y",), "Add")],
        inputs=[R.value_info("x", (1, 1, 2, 2))],
        outputs=[R.value_info("y", (1, 1, 2, 2))],
    ))
    excluded, _ = pool2d(average_input, (1, 1, 2, 2), (2, 2),
                         (1, 1, 0, 0), (1, 1), "average", 0)
    included, _ = pool2d(average_input, (1, 1, 2, 2), (2, 2),
                         (1, 1, 0, 0), (1, 1), "average", 1)
    average_output = [left + right for left, right in zip(excluded, included)]
    write_fixture(root, "averagepool-count", average_model,
                  average_input, average_output)

    global_input = [1.0, 2.0, 3.0, 4.0, 5.0, 6.0,
                    -1.0, 0.0, 1.0, 2.0, 3.0, 4.0]
    global_model = R.model(R.graph(
        nodes=[R.node(("x",), ("y",), "GlobalAveragePool")],
        inputs=[R.value_info("x", (1, 2, 2, 3))],
        outputs=[R.value_info("y", (1, 2, 1, 1))],
    ))
    global_output = [sum(global_input[:6]) / 6.0, sum(global_input[6:]) / 6.0]
    write_fixture(root, "global-averagepool", global_model,
                  global_input, global_output)

    transposed = R.node(("x",), ("xt",), "Transpose", attrs=(
        G1.attr_ints("perm", [0, 1, 3, 2]),
    ))
    bad_pool = R.node(("xt",), ("product",), "MaxPool", attrs=(
        G1.attr_ints("kernel_shape", [2, 2]),
    ))
    restore = R.node(("product",), ("y",), "Transpose", attrs=(
        G1.attr_ints("perm", [0, 1, 3, 2]),
    ))
    (root / "unsupported-strided-pool.onnx").write_bytes(R.model(R.graph(
        nodes=[transposed, bad_pool, restore],
        inputs=[R.value_info("x", (1, 1, 3, 4))],
        outputs=[R.value_info("y", (1, 1, 2, 3))],
    )))


if __name__ == "__main__":
    main()
