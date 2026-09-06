#!/usr/bin/env python3
"""Build deterministic Stage-G.1 Conv models, NWT1 packs, and f32 oracles."""

import hashlib
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


def align(value, boundary):
    return (value + boundary - 1) // boundary * boundary


def attr_ints(name, values):
    return R.s(1, name) + R.v(20, 7) + R.b(
        8, b"".join(R.varint(value) for value in values)
    )


def float_tensor(name, shape, values):
    raw = struct.pack(f"<{len(values)}f", *values)
    return R.tensor(name, dims=shape, data=raw), raw


def nwt1_entry(name, shape, offset, byte_length):
    raw_name = name.encode("utf-8")
    size = align(28 + 8 * len(shape) + len(raw_name), 8)
    body = bytearray(size)
    struct.pack_into("<IHBBIQQ", body, 0, size, len(raw_name), 1,
                     len(shape), 0, offset, byte_length)
    for index, dimension in enumerate(shape):
        struct.pack_into("<Q", body, 28 + index * 8, dimension)
    begin = 28 + 8 * len(shape)
    body[begin:begin + len(raw_name)] = raw_name
    return body


def make_nwt1(model_bytes, tensors):
    ordered = sorted(tensors, key=lambda item: item[0].encode("utf-8"))
    entries = []
    payload = bytearray()
    for name, shape, raw in ordered:
        offset = align(len(payload), 64)
        payload.extend(bytes(offset - len(payload)))
        payload.extend(raw)
        entries.append(nwt1_entry(name, shape, offset, len(raw)))
    directory = b"".join(entries)
    payload_start = align(104 + len(directory), 64)
    pack = bytearray(payload_start + len(payload))
    pack[:8] = b"NOCTWGT\0"
    struct.pack_into("<HHIIIQQ", pack, 8, 1, 0, 104, 0, len(ordered),
                     len(directory), len(payload))
    pack[40:72] = hashlib.sha256(model_bytes).digest()
    pack[72:104] = hashlib.sha256(payload).digest()
    pack[104:104 + len(directory)] = directory
    pack[payload_start:] = payload
    return bytes(pack)


def conv2d(values, input_shape, weights, weight_shape, bias, pads, strides):
    n_count, in_channels, in_height, in_width = input_shape
    out_channels, weight_channels, kernel_h, kernel_w = weight_shape
    assert n_count == 1 and in_channels == weight_channels
    out_height = (in_height + pads[0] + pads[2] - kernel_h) // strides[0] + 1
    out_width = (in_width + pads[1] + pads[3] - kernel_w) // strides[1] + 1
    result = [0.0] * (out_channels * out_height * out_width)
    for oc in range(out_channels):
        for oy in range(out_height):
            for ox in range(out_width):
                total = 0.0 if bias is None else bias[oc]
                for ic in range(in_channels):
                    for ky in range(kernel_h):
                        for kx in range(kernel_w):
                            iy = oy * strides[0] + ky - pads[0]
                            ix = ox * strides[1] + kx - pads[1]
                            if 0 <= iy < in_height and 0 <= ix < in_width:
                                input_index = ic * in_height * in_width + iy * in_width + ix
                                weight_index = ((oc * in_channels + ic) * kernel_h + ky) * kernel_w + kx
                                total += values[input_index] * weights[weight_index]
                result[(oc * out_height + oy) * out_width + ox] = total
    return result, (1, out_channels, out_height, out_width)


def write_f32(path, values):
    path.write_bytes(struct.pack(f"<{len(values)}f", *values))


def write_fixture(root, name, model_bytes, input_values, output_values, tensors):
    (root / f"{name}.onnx").write_bytes(model_bytes)
    (root / f"{name}.weights").write_bytes(make_nwt1(model_bytes, tensors))
    write_f32(root / f"{name}.input.f32le", input_values)
    write_f32(root / f"{name}.output.f32le", output_values)


def main():
    root = Path(sys.argv[1])
    root.mkdir(parents=True, exist_ok=True)

    reuse_input = [float(index - 8) / 4.0 for index in range(16)]
    reuse_weight = [0.25, 0.0, -0.25, 0.5, 1.0, -0.5, 0.25, 0.0, -0.25]
    reuse_bias = [0.125]
    weight_tensor, weight_raw = float_tensor("conv_w", (1, 1, 3, 3), reuse_weight)
    bias_tensor, bias_raw = float_tensor("conv_b", (1,), reuse_bias)
    conv_attrs = (
        attr_ints("kernel_shape", [3, 3]),
        attr_ints("pads", [1, 1, 1, 1]),
        attr_ints("strides", [1, 1]),
    )
    reuse_nodes = [
        R.node(("x", "conv_w", "conv_b"), ("hidden",), "Conv", attrs=conv_attrs),
        R.node(("hidden", "conv_w", "conv_b"), ("y",), "Conv", attrs=conv_attrs),
    ]
    reuse_model = R.model(R.graph(
        nodes=reuse_nodes, initializers=[weight_tensor, bias_tensor],
        inputs=[R.value_info("x", (1, 1, 4, 4))],
        outputs=[R.value_info("y", (1, 1, 4, 4))],
    ))
    reuse_once, reuse_shape = conv2d(
        reuse_input, (1, 1, 4, 4), reuse_weight, (1, 1, 3, 3),
        reuse_bias, (1, 1, 1, 1), (1, 1),
    )
    reuse_twice, _ = conv2d(
        reuse_once, reuse_shape, reuse_weight, (1, 1, 3, 3),
        reuse_bias, (1, 1, 1, 1), (1, 1),
    )
    write_fixture(root, "conv-reuse", reuse_model, reuse_input, reuse_twice, [
        ("conv_w", (1, 1, 3, 3), weight_raw),
        ("conv_b", (1,), bias_raw),
    ])
    wrong_name_pack = make_nwt1(reuse_model, [
        ("conv_w", (1, 1, 3, 3), weight_raw),
        ("wrong_bias", (1,), bias_raw),
    ])
    (root / "conv-reuse.wrong-name.weights").write_bytes(wrong_name_pack)
    corrupt_pack = bytearray((root / "conv-reuse.weights").read_bytes())
    corrupt_pack[-1] ^= 1
    (root / "conv-reuse.corrupt.weights").write_bytes(corrupt_pack)

    stride_input = [float((index * 7) % 19 - 9) / 5.0 for index in range(50)]
    stride_weight = [float((index * 5) % 13 - 6) / 8.0 for index in range(36)]
    stride_tensor, stride_raw = float_tensor("stride_w", (2, 2, 3, 3), stride_weight)
    stride_conv = R.node(("x", "stride_w"), ("conv",), "Conv", attrs=(
        attr_ints("kernel_shape", [3, 3]),
        attr_ints("pads", [1, 1, 1, 1]),
        attr_ints("strides", [2, 2]),
    ))
    stride_relu = R.node(("conv",), ("y",), "Relu")
    stride_model = R.model(R.graph(
        nodes=[stride_conv, stride_relu], initializers=[stride_tensor],
        inputs=[R.value_info("x", (1, 2, 5, 5))],
        outputs=[R.value_info("y", (1, 2, 3, 3))],
    ))
    stride_output, _ = conv2d(
        stride_input, (1, 2, 5, 5), stride_weight, (2, 2, 3, 3),
        None, (1, 1, 1, 1), (2, 2),
    )
    stride_output = [max(0.0, value) for value in stride_output]
    write_fixture(root, "conv-stride-relu", stride_model, stride_input,
                  stride_output, [("stride_w", (2, 2, 3, 3), stride_raw)])

    transposed = R.node(("x",), ("xt",), "Transpose", attrs=(
        attr_ints("perm", [0, 1, 3, 2]),
    ))
    strided_conv = R.node(("xt", "conv_w"), ("y",), "Conv", attrs=(
        attr_ints("kernel_shape", [3, 3]),
    ))
    (root / "unsupported-strided-conv.onnx").write_bytes(R.model(R.graph(
        nodes=[transposed, strided_conv], initializers=[weight_tensor],
        inputs=[R.value_info("x", (1, 1, 4, 4))],
        outputs=[R.value_info("y", (1, 1, 2, 2))],
    )))
    dilated_conv = R.node(("x", "conv_w"), ("y",), "Conv", attrs=(
        attr_ints("kernel_shape", [3, 3]),
        attr_ints("dilations", [2, 1]),
    ))
    (root / "unsupported-dilated-conv.onnx").write_bytes(R.model(R.graph(
        nodes=[dilated_conv], initializers=[weight_tensor],
        inputs=[R.value_info("x", (1, 1, 5, 5))],
        outputs=[R.value_info("y", (1, 1, 1, 3))],
    )))


if __name__ == "__main__":
    main()
