#!/usr/bin/env python3
"""Verify one cached model against tests/testcases/dnn/models.lock."""

import hashlib
import json
import sys
from pathlib import Path


lock = json.loads(Path(sys.argv[1]).read_text(encoding="utf-8"))
model_id = sys.argv[2]
path = Path(sys.argv[3])
record = next((item for item in lock["models"] if item["id"] == model_id), None)
if record is None:
    raise SystemExit(f"unknown locked model {model_id}")
data = path.read_bytes()
if len(data) != record["artifact"]["byte_size"]:
    raise SystemExit(f"locked model size mismatch for {model_id}")
digest = hashlib.sha256(data).hexdigest()
if digest != record["artifact"]["sha256"]:
    raise SystemExit(f"locked model SHA-256 mismatch for {model_id}")
print(f"model-lock-ok id={model_id} sha256={digest}")
