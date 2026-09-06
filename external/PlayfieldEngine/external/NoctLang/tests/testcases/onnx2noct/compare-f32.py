#!/usr/bin/env python3
"""Compare little-endian float32 vectors with a bounded mixed tolerance."""

import math
import struct
import sys
from pathlib import Path


def read(path):
    data = Path(path).read_bytes()
    if len(data) % 4:
        raise SystemExit(f"{path}: byte count is not divisible by four")
    return struct.unpack(f"<{len(data) // 4}f", data)


actual = read(sys.argv[1])
expected = read(sys.argv[2])
if len(actual) != len(expected):
    raise SystemExit(f"element count differs: {len(actual)} != {len(expected)}")
for index, (got, want) in enumerate(zip(actual, expected)):
    if math.isnan(want):
        ok = math.isnan(got)
    else:
        ok = abs(got - want) <= 2e-6 + 2e-5 * abs(want)
    if not ok:
        raise SystemExit(f"element {index}: got {got!r}, expected {want!r}")
