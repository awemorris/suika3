#!/usr/bin/env python3
"""Compare raw float32 model output, report errors, and optionally check argmax."""

import math
import struct
import sys
from pathlib import Path


actual_bytes = Path(sys.argv[1]).read_bytes()
expected_bytes = Path(sys.argv[2]).read_bytes()
abs_tolerance = float(sys.argv[3])
rel_tolerance = float(sys.argv[4])
check_argmax = len(sys.argv) == 6 and sys.argv[5] == "argmax"
if len(actual_bytes) != len(expected_bytes) or len(actual_bytes) % 4:
    raise SystemExit("model output byte count mismatch")
count = len(actual_bytes) // 4
actual = struct.unpack(f"<{count}f", actual_bytes)
expected = struct.unpack(f"<{count}f", expected_bytes)
max_abs = 0.0
max_rel = 0.0
for index, (got, want) in enumerate(zip(actual, expected)):
    if math.isnan(want):
        if not math.isnan(got):
            raise SystemExit(f"finite/NaN mismatch at {index}")
        continue
    if math.isinf(want):
        if got != want:
            raise SystemExit(f"infinity mismatch at {index}")
        continue
    if not math.isfinite(got):
        raise SystemExit(f"nonfinite mismatch at {index}")
    absolute = abs(got - want)
    relative = absolute / abs(want) if want else absolute
    max_abs = max(max_abs, absolute)
    max_rel = max(max_rel, relative)
    if absolute > abs_tolerance + rel_tolerance * abs(want):
        raise SystemExit(
            f"tolerance exceeded at {index}: got={got} want={want} "
            f"abs={absolute} rel={relative}"
        )
actual_argmax = max(range(count), key=actual.__getitem__)
expected_argmax = max(range(count), key=expected.__getitem__)
if check_argmax and actual_argmax != expected_argmax:
    raise SystemExit(
        f"argmax mismatch: got={actual_argmax} want={expected_argmax}"
    )
print(
    f"compare-ok elements={count} max_abs={max_abs:.9g} "
    f"max_rel={max_rel:.9g} argmax={actual_argmax}"
)
