#!/usr/bin/env python3
"""Validate deterministic Stage-H package identities without ONNX packages."""

import hashlib
import json
import struct
import sys
from pathlib import Path


def sha256(data):
    return hashlib.sha256(data).hexdigest()


def fail(message):
    raise SystemExit(f"package verification failed: {message}")


root = Path(sys.argv[1])
onnx_path = Path(sys.argv[2])
manifest_path = root / "manifest.json"
if not manifest_path.is_file():
    fail("manifest commit marker is missing")
raw_manifest = manifest_path.read_bytes()
if not raw_manifest.endswith(b"\n") or b"\r" in raw_manifest:
    fail("manifest is not LF-terminated UTF-8")
manifest = json.loads(raw_manifest)
manifest_text = raw_manifest.decode("utf-8")
for forbidden in ("/home/", "D3D12", "Mesa", "Iris", "llvmpipe", "softpipe"):
    if forbidden in manifest_text:
        fail(f"forbidden manifest environment text {forbidden!r}")
onnx_bytes = onnx_path.read_bytes()
source_hash = sha256(onnx_bytes)
if manifest["manifest_version"] != 1:
    fail("manifest version")
if manifest["source_model"]["sha256"] != source_hash:
    fail("source model hash")

pack = (root / "model.weights").read_bytes()
pack_hash = sha256(pack)
if manifest["weights"]["sha256"] != pack_hash:
    fail("pack hash")
if len(pack) < 104 or pack[:8] != b"NOCTWGT\0":
    fail("NWT1 header")
header_bytes = struct.unpack_from("<I", pack, 12)[0]
directory_bytes = struct.unpack_from("<Q", pack, 24)[0]
payload_bytes = struct.unpack_from("<Q", pack, 32)[0]
payload_start = (header_bytes + directory_bytes + 63) // 64 * 64
payload = pack[payload_start:]
if len(payload) != payload_bytes:
    fail("NWT1 payload size")
if pack[40:72] != hashlib.sha256(onnx_bytes).digest():
    fail("NWT1 source binding")
payload_hash = sha256(payload)
if pack[72:104] != hashlib.sha256(payload).digest():
    fail("NWT1 payload header hash")
if manifest["weights"]["payload_sha256"] != payload_hash:
    fail("manifest payload hash")

artifacts = manifest["artifacts"]
if artifacts.get("model.weights") != pack_hash:
    fail("artifact pack hash")
for relative, expected in artifacts.items():
    artifact = root / relative
    if not artifact.is_file() or sha256(artifact.read_bytes()) != expected:
        fail(f"artifact hash {relative}")

model_source = (root / "gpu" / "model.noct").read_text(encoding="utf-8")
if pack_hash not in model_source or source_hash not in model_source:
    fail("generated model identity binding")
if "modelPackHash" in model_source:
    fail("generated model self-hashes untrusted weights")
for forbidden in ("D3D12", "DNN.", "__accel func", str(root), str(onnx_path)):
    if forbidden and forbidden in model_source:
        fail(f"forbidden generated source text {forbidden!r}")
if manifest["backend"] != {"required": "OpenGL", "cpu_fallback": False}:
    fail("backend policy")
print(f"package-ok source={source_hash} weights={pack_hash}")
