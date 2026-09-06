#!/usr/bin/env python3
"""Build tiny ONNX protobufs without importing onnx (tests only)."""

import struct
import sys
from pathlib import Path


def varint(value):
    value &= (1 << 64) - 1
    out = bytearray()
    while value >= 0x80:
        out.append((value & 0x7f) | 0x80)
        value >>= 7
    out.append(value)
    return bytes(out)


def key(field, wire):
    return varint((field << 3) | wire)


def v(field, value):
    return key(field, 0) + varint(value)


def f32(field, bits):
    return key(field, 5) + struct.pack("<I", bits)


def f64(field, bits):
    return key(field, 1) + struct.pack("<Q", bits)


def b(field, data):
    return key(field, 2) + varint(len(data)) + data


def s(field, text):
    return b(field, text.encode("utf-8"))


def dimension(value=1, param=None):
    return s(2, param) if param is not None else v(1, value)


def value_info(name, dims=(1, 4), elem=1, type_extra=b""):
    shape = b"".join(b(1, dimension(d)) for d in dims)
    tensor_type = v(1, elem) + b(2, shape)
    return s(1, name) + b(2, b(1, tensor_type) + type_extra)


def symbolic_value_info(name):
    shape = b(1, dimension(1)) + b(1, dimension(param="width"))
    return s(1, name) + b(2, b(1, v(1, 1) + b(2, shape)))


def node(inputs=("x",), outputs=("y",), op="Identity", domain="", attrs=()):
    out = b"".join(s(1, x) for x in inputs)
    out += b"".join(s(2, x) for x in outputs)
    out += s(4, op)
    out += b"".join(b(5, x) for x in attrs)
    if domain:
        out += s(7, domain)
    return out


def attribute(name="axis", extra=b""):
    return s(1, name) + extra


def tensor(name="w", dims=(1, 4), dtype=1, storage="raw", data=None,
           packed_dims=True, extra=b""):
    if packed_dims:
        dims_field = b(1, b"".join(varint(x) for x in dims))
    else:
        dims_field = b"".join(v(1, x) for x in dims)
    out = dims_field + v(2, dtype) + s(8, name)
    count = 1
    for d in dims:
        count *= d
    if data is None and storage != "none":
        if dtype in (1, 6):
            data = bytes(count * 4)
        else:
            data = bytes(count * 8)
    if storage == "raw":
        out += b(9, data)
    elif storage == "float-packed":
        out += b(4, data)
    elif storage == "float-unpacked":
        out += b"".join(f32(4, 0) for _ in range(count))
    elif storage == "int32-packed":
        out += b(5, b"".join(varint(i) for i in range(count)))
    elif storage == "int64-unpacked":
        out += b"".join(v(7, i) for i in range(count))
    elif storage == "none":
        pass
    return out + extra


def graph(nodes=None, initializers=(), inputs=None, outputs=None,
          value_infos=(), extra=b""):
    if nodes is None:
        nodes = [node()]
    if inputs is None:
        inputs = [value_info("x")]
    if outputs is None:
        outputs = [value_info("y")]
    out = b"".join(b(1, x) for x in nodes)
    out += b"".join(b(5, x) for x in initializers)
    out += b"".join(b(11, x) for x in inputs)
    out += b"".join(b(12, x) for x in outputs)
    out += b"".join(b(13, x) for x in value_infos)
    return out + extra


def model(g=None, opset=12, domain="", extra=b"", ir=7):
    if g is None:
        g = graph()
    opset_message = (s(1, domain) if domain else b"") + v(2, opset)
    return v(1, ir) + b(7, g) + b(8, opset_message) + extra


def write_case(root, name, payload, message, errors):
    (root / name).write_bytes(payload)
    errors.append((name, message))


def main():
    root = Path(sys.argv[1])
    root.mkdir(parents=True, exist_ok=True)
    errors = []

    valid = model()
    (root / "valid-unpacked.onnx").write_bytes(valid)
    init_packed = tensor(storage="float-packed")
    packed_graph = graph(nodes=[node(("x", "w"), ("y",), "Add")],
                         initializers=[init_packed])
    (root / "valid-packed.onnx").write_bytes(model(packed_graph))
    unknown = key(100, 0) + varint(9) + f64(101, 0) + b(102, b"abc") + f32(103, 0)
    (root / "valid-unknown-fields.onnx").write_bytes(model(extra=unknown))
    metadata_graph = graph(inputs=[value_info("x", type_extra=s(6, "IMAGE"))])
    (root / "valid-type-metadata.onnx").write_bytes(model(metadata_graph))
    (root / "valid-ai-onnx-domain.onnx").write_bytes(model(domain="ai.onnx"))
    old_ir_init = tensor("w")
    old_ir_graph = graph(initializers=[old_ir_init],
                         inputs=[value_info("x"), s(1, "w")])
    (root / "valid-old-ir-initializer-input.onnx").write_bytes(model(old_ir_graph))
    optional_graph = graph(nodes=[node(("x", ""), ("y",))])
    (root / "valid-optional-empty-input.onnx").write_bytes(model(optional_graph))
    exact_bits = struct.pack("<IIII", 0x00000001, 0x80000000,
                             0x7FC00001, 0xFFFFFFFF)
    bits_init = tensor(storage="float-packed", data=exact_bits)
    bits_graph = graph(nodes=[node(("x", "w"), ("y",), "Add")],
                       initializers=[bits_init])
    (root / "valid-float-bits.onnx").write_bytes(model(bits_graph))

    write_case(root, "bad-wire.onnx", valid + key(100, 3),
               "unsupported wire type", errors)
    write_case(root, "field-zero.onnx", valid + b"\x00",
               "field number zero", errors)
    write_case(root, "bad-length.onnx", valid + key(100, 2) + varint(20) + b"x",
               "truncated unknown ModelProto field", errors)
    write_case(root, "overlong-varint.onnx", valid + key(100, 0) + b"\x80\x00",
               "Overlong varint encoding", errors)
    write_case(root, "wide-varint.onnx", valid + key(100, 0) + b"\x80" * 10 + b"\x00",
               "Varint exceeds", errors)
    write_case(root, "duplicate-ir.onnx", model(extra=v(1, 7)),
               "duplicate ModelProto.ir_version", errors)
    write_case(root, "duplicate-graph.onnx", model(extra=b(7, graph())),
               "multiple graphs", errors)
    write_case(root, "training.onnx", model(extra=b(20, b"")),
               "training_info is unsupported", errors)
    write_case(root, "function.onnx", model(extra=b(25, b"")),
               "FunctionProto is unsupported", errors)
    write_case(root, "sparse.onnx", model(graph(extra=b(15, b""))),
               "sparse_initializer is unsupported", errors)
    graph_attr = attribute("body", b(6, b""))
    write_case(root, "control-flow.onnx",
               model(graph(nodes=[node(attrs=(graph_attr,))])),
               "control-flow graph data", errors)
    write_case(root, "custom-opset.onnx", model(domain="com.example"),
               "custom opset domain", errors)
    write_case(root, "custom-node.onnx",
               model(graph(nodes=[node(domain="com.example")])),
               "uses custom domain", errors)
    write_case(root, "unsupported-opset.onnx", model(opset=11),
               "opset must be exactly 7, 8, or 12", errors)
    write_case(root, "missing-opset.onnx", v(1, 7) + b(7, graph()),
               "opset must be exactly 7, 8, or 12", errors)
    write_case(root, "duplicate-opset.onnx",
               model(extra=b(8, v(2, 12))),
               "duplicate default-domain opset import", errors)
    sequence_type = s(1, "x") + b(2, b(4, b""))
    write_case(root, "sequence-input.onnx",
               model(graph(inputs=[sequence_type])),
               "graph input must have tensor_type", errors)
    nul_op = s(1, "x") + s(2, "y") + b(4, b"Bad\x00Op")
    write_case(root, "nul-string.onnx", model(graph(nodes=[nul_op])),
               "contains a NUL byte", errors)
    bad_utf8_op = s(1, "x") + s(2, "y") + b(4, b"\xc0\x80")
    write_case(root, "bad-utf8-string.onnx",
               model(graph(nodes=[bad_utf8_op])),
               "is not valid UTF-8", errors)
    write_case(root, "oversized-string.onnx",
               model(graph(nodes=[node(op="A" * (1048576 + 1))])),
               "exceeds the 1 MiB string limit", errors)
    many_attrs = tuple(attribute(f"a{i}") for i in range(257))
    write_case(root, "too-many-attributes.onnx",
               model(graph(nodes=[node(attrs=many_attrs)])),
               "more than 256 attributes", errors)
    write_case(root, "symbolic.onnx",
               model(graph(inputs=[symbolic_value_info("x")])),
               "symbolic dimension", errors)
    write_case(root, "zero-dimension.onnx",
               model(graph(inputs=[value_info("x", (1, 0))])),
               "dynamic or nonpositive dimension", errors)
    write_case(root, "batch-two.onnx",
               model(graph(inputs=[value_info("x", (2, 4))])),
               "batch dimension must be one", errors)
    write_case(root, "nonfloat-input.onnx",
               model(graph(inputs=[value_info("x", elem=6)])),
               "graph input must be float32", errors)
    write_case(root, "multiple-input.onnx",
               model(graph(inputs=[value_info("x"), value_info("z")])),
               "exactly one non-initializer input", errors)
    write_case(root, "multiple-output.onnx",
               model(graph(outputs=[value_info("y"), value_info("z")])),
               "exactly one output", errors)
    write_case(root, "missing-node-input.onnx",
               model(graph(nodes=[node(("missing",), ("y",))])),
               "missing or not produced earlier", errors)
    cycle_nodes = [node(("b",), ("a",)), node(("a",), ("b",))]
    write_case(root, "cycle.onnx", model(graph(nodes=cycle_nodes,
               outputs=[value_info("b")])), "missing or not produced earlier", errors)
    duplicate_nodes = [node(("x",), ("y",)), node(("x",), ("y",))]
    write_case(root, "duplicate-output.onnx", model(graph(nodes=duplicate_nodes)),
               "duplicate value or node output", errors)
    write_case(root, "undefined-output.onnx",
               model(graph(outputs=[value_info("absent")])),
               "graph output 'absent' is undefined", errors)
    dup_init = tensor("w")
    write_case(root, "duplicate-initializer.onnx",
               model(graph(initializers=[dup_init, dup_init])),
               "duplicate initializer name", errors)
    write_case(root, "empty-initializer-name.onnx",
               model(graph(initializers=[tensor("")])),
               "initializer[0] name must not be empty", errors)
    competing = tensor(extra=b(4, bytes(16)))
    write_case(root, "competing-data.onnx", model(graph(initializers=[competing])),
               "competing data fields", errors)
    segmented = tensor(extra=b(3, b""))
    write_case(root, "segment.onnx", model(graph(initializers=[segmented])),
               "uses unsupported segments", errors)
    wrong_bytes = tensor(data=bytes(12))
    write_case(root, "wrong-byte-count.onnx", model(graph(initializers=[wrong_bytes])),
               "raw_data byte count mismatch", errors)
    write_case(root, "unsupported-dtype.onnx",
               model(graph(initializers=[tensor(dtype=11)])),
               "unsupported data type", errors)
    write_case(root, "rank-nine.onnx",
               model(graph(initializers=[tensor(dims=(1,) * 9)])),
               "rank exceeds eight", errors)
    overflow_tensor = tensor(dims=(50000, 50000), storage="none")
    write_case(root, "element-overflow.onnx",
               model(graph(initializers=[overflow_tensor])),
               "element count exceeds INT_MAX", errors)
    attr1 = attribute("axis", v(3, 0))
    write_case(root, "duplicate-attribute.onnx",
               model(graph(nodes=[node(attrs=(attr1, attr1))])),
               "duplicate attribute", errors)
    write_case(root, "runtime-int-value-info.onnx",
               model(graph(value_infos=[value_info("y", elem=6)])),
               "runtime non-float tensor", errors)

    nested = b""
    for depth in range(1, 34):
        nested = b(1, nested)
        if depth == 32:
            (root / "depth-32.pb").write_bytes(nested)
    (root / "depth-33.pb").write_bytes(nested)

    with (root / "errors.tsv").open("w", encoding="utf-8", newline="\n") as out:
        for name, message in errors:
            out.write(f"{name}\t{message}\n")


if __name__ == "__main__":
    main()
