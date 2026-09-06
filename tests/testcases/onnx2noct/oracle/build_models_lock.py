#!/usr/bin/env python3
"""Build the reviewed Stage-A model lock from exact local artifacts."""

from __future__ import annotations

import argparse
import json
import pathlib
import sys
from typing import Any

from inventory import inventory


CATALOG: list[dict[str, Any]] = [
    {
        "id": "mnist-12",
        "filename": "mnist-12.onnx",
        "expected_byte_size": 26143,
        "expected_sha256": "5c688690f8bacf667d4c2074af5ad0646ca328d7ab03eccf944a65b320171bdd",
        "source": {
            "kind": "external",
            "revision": "1cd752f4a3d818fa215aa621c78b8a10a4a1a3a5",
            "url": "https://huggingface.co/onnxmodelzoo/mnist-12/resolve/1cd752f4a3d818fa215aa621c78b8a10a4a1a3a5/mnist-12.onnx",
            "page": "https://huggingface.co/onnxmodelzoo/mnist-12",
            "license_evidence": [
                {"claim": "Apache-2.0", "location": "Hugging Face repository metadata"},
                {"claim": "MIT", "location": "migrated ONNX Model Zoo model-card License section"},
            ],
            "notice": "Migrated ONNX Model Zoo artifact; preserve both conflicting published license claims pending owner/legal review.",
        },
        "acceptance": {
            "fixed_shape_v1": True,
            "blockers": [],
            "output_meaning": "raw 1x10 logits before softmax",
        },
    },
    {
        "id": "project-cifar-opset12",
        "filename": "project-cifar-opset12.onnx",
        "expected_byte_size": 248967,
        "expected_sha256": "919bf61c686a19c22404636db9003b1cbe651cf1fd01250500a1c2a3a72d64bf",
        "source": {
            "kind": "repository",
            "path": "tests/testcases/onnx2noct/fixtures/models/project-cifar-opset12.onnx",
            "generator": "tests/testcases/onnx2noct/oracle/generate_fixtures.py",
            "topology_reference": "https://docs.pytorch.org/tutorials/beginner/blitz/cifar10_tutorial.html",
            "license_evidence": [
                {"claim": "zlib", "location": "NoctLang repository LICENSE for project-owned fixture"}
            ],
            "notice": "Project-owned deterministic sparse-weight export of the existing Noct CIFAR benchmark topology.",
        },
        "acceptance": {
            "fixed_shape_v1": True,
            "blockers": [],
            "output_meaning": "raw 1x10 synthetic logits",
        },
    },
    {
        "id": "squeezenet1.1-7",
        "filename": "squeezenet1.1-7.onnx",
        "expected_byte_size": 4956208,
        "expected_sha256": "1eeff551a67ae8d565ca33b572fc4b66e3ef357b0eb2863bb9ff47a918cc4088",
        "source": {
            "kind": "external",
            "revision": "61e525224ad479521059f4586bcacf50ad3627ca",
            "url": "https://huggingface.co/onnxmodelzoo/squeezenet1.1-7/resolve/61e525224ad479521059f4586bcacf50ad3627ca/squeezenet1.1-7.onnx",
            "page": "https://huggingface.co/onnxmodelzoo/squeezenet1.1-7",
            "paper": "https://arxiv.org/abs/1602.07360",
            "license_evidence": [
                {"claim": "Apache-2.0", "location": "Hugging Face repository metadata and model-card License section"}
            ],
            "notice": "Migrated ONNX Model Zoo SqueezeNet 1.1 artifact.",
        },
        "acceptance": {
            "fixed_shape_v1": True,
            "blockers": [],
            "output_meaning": "raw 1x1000 classification scores",
        },
    },
    {
        "id": "tinyyolov2-8",
        "filename": "tinyyolov2-8.onnx",
        "expected_byte_size": 63480982,
        "expected_sha256": "583fb7fdc948435ceac9fa82efc7708701efe8382a859a3dd46526b155f5f2ae",
        "source": {
            "kind": "external",
            "revision": "869707e16e57006f97d98af54cfdc8a1d388ae61",
            "url": "https://huggingface.co/onnxmodelzoo/tinyyolov2-8/resolve/869707e16e57006f97d98af54cfdc8a1d388ae61/tinyyolov2-8.onnx",
            "page": "https://huggingface.co/onnxmodelzoo/tinyyolov2-8",
            "paper": "https://arxiv.org/abs/1612.08242",
            "license_evidence": [
                {"claim": "Apache-2.0", "location": "Hugging Face repository metadata"},
                {"claim": "MIT", "location": "migrated ONNX Model Zoo model-card License section"},
            ],
            "notice": "Migrated ONNX Model Zoo artifact; preserve both conflicting published license claims pending owner/legal review.",
        },
        "acceptance": {
            "fixed_shape_v1": False,
            "blockers": [
                "The exact artifact declares symbolic batch dimension 'None' on input and output; design 18 v1 requires static batch 1. Do not silently rewrite it."
            ],
            "output_meaning": "raw batchx125x13x13 YOLO grid tensor; no decode or NMS",
        },
    },
]


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--repository", type=pathlib.Path, required=True)
    parser.add_argument("--external-cache", type=pathlib.Path, required=True)
    parser.add_argument("--oracles", type=pathlib.Path, required=True)
    parser.add_argument("--output", type=pathlib.Path, required=True)
    args = parser.parse_args()
    repository = args.repository.resolve()
    external_cache = args.external_cache.resolve()
    oracle_lock = json.loads(args.oracles.read_text(encoding="utf-8"))
    oracle_by_id = {record["id"]: record for record in oracle_lock["models"]}
    models = []
    for catalog in CATALOG:
        source = catalog["source"]
        if source["kind"] == "external":
            path = external_cache / catalog["filename"]
        else:
            path = repository / source["path"]
        result = inventory(path)
        if result["byte_size"] != catalog["expected_byte_size"]:
            raise RuntimeError(f"{catalog['id']}: byte-size mismatch")
        if result["sha256"] != catalog["expected_sha256"]:
            raise RuntimeError(f"{catalog['id']}: SHA-256 mismatch")
        result["graph"].pop("initializers")
        result["graph"].pop("nodes")
        oracle = oracle_by_id.get(catalog["id"])
        if oracle is None or oracle["model_sha256"] != result["sha256"]:
            raise RuntimeError(f"{catalog['id']}: missing or mismatched oracle")
        models.append({
            "id": catalog["id"],
            "artifact": {
                "filename": catalog["filename"],
                "byte_size": result["byte_size"],
                "sha256": result["sha256"],
            },
            "source": source,
            "inventory": {
                key: value for key, value in result.items()
                if key not in {"file", "byte_size", "sha256"}
            },
            "oracle": {
                "lock": "tests/testcases/onnx2noct/oracle-data/model-oracles.lock",
                "input": oracle["input"],
                "output": oracle["output"],
            },
            "acceptance": catalog["acceptance"],
        })
    lock = {
        "schema": 1,
        "authority": "docs/design/18-onnx-gpu-source-codegen.md Stage A",
        "large_external_artifacts_are_not_committed": True,
        "generated_by": "tests/testcases/onnx2noct/oracle/build_models_lock.py",
        "models": models,
    }
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(
        json.dumps(lock, indent=2, sort_keys=True) + "\n", encoding="utf-8"
    )


if __name__ == "__main__":
    try:
        main()
    except Exception as error:
        print(f"build_models_lock.py: {error}", file=sys.stderr)
        raise
