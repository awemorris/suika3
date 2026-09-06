#!/usr/bin/env python3
"""Download and verify external models named by tests/testcases/dnn/models.lock.

This helper uses only the Python standard library.  It does not make Python a
dependency of the production Noct converter or generated model.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import os
import pathlib
import sys
import urllib.request


def digest(path: pathlib.Path) -> str:
    result = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            result.update(block)
    return result.hexdigest()


def verify(path: pathlib.Path, size: int, sha256: str) -> None:
    if not path.is_file():
        raise RuntimeError(f"missing file: {path}")
    actual_size = path.stat().st_size
    if actual_size != size:
        raise RuntimeError(f"{path}: expected {size} bytes, found {actual_size}")
    actual_sha256 = digest(path)
    if actual_sha256 != sha256:
        raise RuntimeError(
            f"{path}: expected SHA-256 {sha256}, found {actual_sha256}"
        )


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--lock", type=pathlib.Path, default="tests/testcases/dnn/models.lock")
    parser.add_argument("--cache", type=pathlib.Path, required=True)
    parser.add_argument("--repository", type=pathlib.Path, default=".")
    parser.add_argument("--only", action="append", default=[])
    args = parser.parse_args()
    lock = json.loads(args.lock.read_text(encoding="utf-8"))
    cache = args.cache.resolve()
    repository = args.repository.resolve()
    cache.mkdir(parents=True, exist_ok=True)
    selected = set(args.only)
    known = {model["id"] for model in lock["models"]}
    unknown = selected - known
    if unknown:
        raise RuntimeError(f"unknown model IDs: {', '.join(sorted(unknown))}")
    for model in lock["models"]:
        model_id = model["id"]
        if selected and model_id not in selected:
            continue
        artifact = model["artifact"]
        source = model["source"]
        filename = artifact["filename"]
        if pathlib.PurePath(filename).name != filename:
            raise RuntimeError(f"unsafe artifact filename in lock: {filename!r}")
        if source["kind"] == "repository":
            path = repository / source["path"]
            verify(path, artifact["byte_size"], artifact["sha256"])
            print(f"verified repository model {model_id}: {path}")
            continue
        destination = cache / filename
        if destination.exists():
            verify(destination, artifact["byte_size"], artifact["sha256"])
            print(f"verified cached model {model_id}: {destination}")
            continue
        partial = cache / f".{filename}.part"
        request = urllib.request.Request(
            source["url"], headers={"User-Agent": "NoctLang-model-lock/1"}
        )
        print(f"downloading {model_id}: {source['url']}")
        try:
            with urllib.request.urlopen(request) as response, partial.open("wb") as stream:
                while True:
                    block = response.read(1024 * 1024)
                    if not block:
                        break
                    stream.write(block)
            verify(partial, artifact["byte_size"], artifact["sha256"])
            os.replace(partial, destination)
        except Exception:
            try:
                partial.unlink()
            except FileNotFoundError:
                pass
            raise
        print(f"downloaded and verified {model_id}: {destination}")


if __name__ == "__main__":
    try:
        main()
    except Exception as error:
        print(f"download_models.py: {error}", file=sys.stderr)
        raise SystemExit(1)
