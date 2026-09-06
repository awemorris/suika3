#!/usr/bin/env python3
"""Print a canonical, bounded inventory for an exact ONNX file.

This is test/oracle tooling.  The production converter must not import Python,
onnx, NumPy, or ONNX Runtime.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import math
import pathlib
import struct
from collections import Counter, defaultdict
from typing import Any

import onnx
from onnx import AttributeProto, TensorProto, helper, numpy_helper


ATTRIBUTE_TYPE_NAMES = {
    value: name
    for name, value in AttributeProto.AttributeType.items()
}


def sha256_file(path: pathlib.Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def float32_bits(value: float) -> str:
    return f"0x{struct.unpack('<I', struct.pack('<f', value))[0]:08x}"


def shape_of(value_info: onnx.ValueInfoProto) -> list[int | str | None]:
    tensor_type = value_info.type.tensor_type
    result: list[int | str | None] = []
    for dim in tensor_type.shape.dim:
        if dim.HasField("dim_value"):
            result.append(int(dim.dim_value))
        elif dim.HasField("dim_param"):
            result.append(dim.dim_param)
        else:
            result.append(None)
    return result


def value_info_record(value_info: onnx.ValueInfoProto) -> dict[str, Any]:
    tensor_type = value_info.type.tensor_type
    return {
        "name": value_info.name,
        "dtype": TensorProto.DataType.Name(tensor_type.elem_type),
        "shape": shape_of(value_info),
    }


def tensor_record(tensor: onnx.TensorProto) -> dict[str, Any]:
    raw = tensor.raw_data
    if raw:
        data_sha256 = hashlib.sha256(raw).hexdigest()
        encoded_bytes = len(raw)
    else:
        array = numpy_helper.to_array(tensor)
        canonical = array.astype(array.dtype.newbyteorder("<"), copy=False).tobytes()
        data_sha256 = hashlib.sha256(canonical).hexdigest()
        encoded_bytes = len(canonical)
    return {
        "name": tensor.name,
        "dtype": TensorProto.DataType.Name(tensor.data_type),
        "shape": [int(d) for d in tensor.dims],
        "payload_bytes": encoded_bytes,
        "payload_sha256": data_sha256,
        "data_location": TensorProto.DataLocation.Name(tensor.data_location),
    }


def scalar_json(value: Any) -> Any:
    if isinstance(value, bytes):
        return {"utf8_or_hex": value.decode("utf-8", "backslashreplace")}
    if isinstance(value, float):
        if math.isnan(value):
            text = "nan"
        elif math.isinf(value):
            text = "+inf" if value > 0 else "-inf"
        else:
            text = repr(value)
        return {"float32_bits": float32_bits(value), "display": text}
    if isinstance(value, (int, str, bool)) or value is None:
        return value
    return value


def attribute_record(attribute: onnx.AttributeProto) -> dict[str, Any]:
    value = helper.get_attribute_value(attribute)
    if isinstance(value, onnx.TensorProto):
        canonical_value: Any = tensor_record(value)
    elif isinstance(value, onnx.GraphProto):
        canonical_value = {
            "graph_name": value.name,
            "node_count": len(value.node),
        }
    elif isinstance(value, (list, tuple)):
        canonical_value = [
            tensor_record(item) if isinstance(item, onnx.TensorProto)
            else scalar_json(item)
            for item in value
        ]
    else:
        canonical_value = scalar_json(value)
    return {
        "name": attribute.name,
        "type": ATTRIBUTE_TYPE_NAMES.get(attribute.type, str(attribute.type)),
        "value": canonical_value,
    }


def canonical_node_attributes(node: onnx.NodeProto) -> list[dict[str, Any]]:
    return sorted(
        (attribute_record(attribute) for attribute in node.attribute),
        key=lambda record: record["name"],
    )


def inventory(path: pathlib.Path) -> dict[str, Any]:
    model = onnx.load(path, load_external_data=False)
    checker_result = "ok"
    try:
        onnx.checker.check_model(model, full_check=True)
    except Exception as error:  # inventory must expose, not hide, checker failures
        checker_result = f"error: {type(error).__name__}: {error}"

    initializer_names = {initializer.name for initializer in model.graph.initializer}
    graph_inputs = [
        value_info_record(value)
        for value in model.graph.input
        if value.name not in initializer_names
    ]
    graph_outputs = [value_info_record(value) for value in model.graph.output]
    counts: Counter[tuple[str, str]] = Counter()
    attribute_sets: dict[tuple[str, str], dict[str, dict[str, Any]]] = defaultdict(dict)
    nodes: list[dict[str, Any]] = []
    for index, node in enumerate(model.graph.node):
        domain = node.domain or "ai.onnx"
        key = (domain, node.op_type)
        attributes = canonical_node_attributes(node)
        counts[key] += 1
        attribute_key = json.dumps(attributes, sort_keys=True, separators=(",", ":"))
        attribute_sets[key][attribute_key] = {
            "count": attribute_sets[key].get(attribute_key, {}).get("count", 0) + 1,
            "attributes": attributes,
        }
        nodes.append({
            "index": index,
            "name": node.name,
            "domain": domain,
            "op_type": node.op_type,
            "inputs": list(node.input),
            "outputs": list(node.output),
            "attributes": attributes,
        })

    operators = []
    for key in sorted(counts):
        variants = sorted(
            attribute_sets[key].values(),
            key=lambda variant: json.dumps(
                variant["attributes"], sort_keys=True, separators=(",", ":")
            ),
        )
        operators.append({
            "domain": key[0],
            "op_type": key[1],
            "count": counts[key],
            "attribute_variants": variants,
        })

    return {
        "file": path.name,
        "byte_size": path.stat().st_size,
        "sha256": sha256_file(path),
        "ir_version": int(model.ir_version),
        "producer_name": model.producer_name,
        "producer_version": model.producer_version,
        "model_version": int(model.model_version),
        "checker": checker_result,
        "opsets": [
            {"domain": opset.domain or "ai.onnx", "version": int(opset.version)}
            for opset in sorted(model.opset_import, key=lambda item: item.domain)
        ],
        "graph": {
            "name": model.graph.name,
            "inputs": graph_inputs,
            "outputs": graph_outputs,
            "node_count": len(model.graph.node),
            "initializer_count": len(model.graph.initializer),
            "initializers": sorted(
                (tensor_record(initializer) for initializer in model.graph.initializer),
                key=lambda record: record["name"],
            ),
            "operators": operators,
            "nodes": nodes,
        },
    }


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("model", type=pathlib.Path)
    parser.add_argument("--summary", action="store_true")
    args = parser.parse_args()
    result = inventory(args.model)
    if args.summary:
        result["graph"].pop("initializers")
        result["graph"].pop("nodes")
    print(json.dumps(result, indent=2, sort_keys=True, ensure_ascii=True))


if __name__ == "__main__":
    main()
