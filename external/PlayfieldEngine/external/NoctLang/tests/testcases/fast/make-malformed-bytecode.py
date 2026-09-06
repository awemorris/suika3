#!/usr/bin/env python3

import pathlib
import sys


source = pathlib.Path(sys.argv[1]).read_bytes()
output_dir = pathlib.Path(sys.argv[2])
output_dir.mkdir(parents=True, exist_ok=True)


def replace_one(name, old, new):
    if old not in source:
        raise RuntimeError("bytecode pattern not found: " + name)
    (output_dir / (name + ".nbc")).write_bytes(source.replace(old, new, 1))


signature = (
    b"Fast Signature\n1\n1\n2\n0\n"
    b"7\n5\n1\n2\n1\n2\n1\n3\n"
    b"0\n-1\n0\n0\n"
)

replace_one("missing", signature, b"")
replace_one("duplicate", signature, signature + signature)
replace_one("unknown-kind", b"Function Kind\n1\n", b"Function Kind\n2\n")
replace_one("unknown-version", b"Fast Signature\n1\n", b"Fast Signature\n2\n")
replace_one(
    "invalid-rank",
    b"Fast Signature\n1\n1\n2\n0\n7\n5\n1\n2\n",
    b"Fast Signature\n1\n1\n2\n0\n7\n5\n1\n9\n",
)
replace_one(
    "unknown-extent-kind",
    b"Fast Signature\n1\n1\n2\n0\n7\n5\n1\n2\n1\n2\n",
    b"Fast Signature\n1\n1\n2\n0\n7\n5\n1\n2\n9\n2\n",
)
replace_one(
    "invalid-extent",
    b"Fast Signature\n1\n1\n2\n0\n7\n5\n1\n2\n1\n2\n",
    b"Fast Signature\n1\n1\n2\n0\n7\n5\n1\n2\n1\n0\n",
)
replace_one("missing-param-types", b"Parameter Types\n7\n0\n", b"")
replace_one(
    "missing-packed-types",
    b"Parameter Packed Types\n5\n-1\n",
    b"",
)
replace_one(
    "missing-restricted",
    b"Parameter Restricted\n1\n0\n",
    b"",
)
replace_one(
    "param-type-mismatch",
    b"Parameter Types\n7\n0\n",
    b"Parameter Types\n0\n0\n",
)
replace_one(
    "packed-type-mismatch",
    b"Parameter Packed Types\n5\n-1\n",
    b"Parameter Packed Types\n4\n-1\n",
)
replace_one(
    "restricted-mismatch",
    b"Parameter Restricted\n1\n0\n",
    b"Parameter Restricted\n0\n0\n",
)
replace_one("missing-return-type", b"Return Type\n0\n-1\n1\n", b"")
replace_one(
    "return-type-mismatch",
    b"Return Type\n0\n-1\n1\n",
    b"Return Type\n5\n-1\n1\n",
)
replace_one(
    "param-type-text",
    b"Parameter Types\n7\n0\n",
    b"Parameter Types\n7\n0junk\n",
)
replace_one(
    "packed-type-text",
    b"Parameter Packed Types\n5\n-1\n",
    b"Parameter Packed Types\n5junk\n-1\n",
)
replace_one(
    "restricted-text",
    b"Parameter Restricted\n1\n0\n",
    b"Parameter Restricted\n1junk\n0\n",
)
replace_one(
    "restricted-noncanonical",
    b"Parameter Restricted\n1\n0\n",
    b"Parameter Restricted\n2\n0\n",
)
replace_one(
    "return-type-text",
    b"Return Type\n0\n-1\n1\n",
    b"Return Type\n0junk\n-1\n1\n",
)

truncated = source.index(b"Fast Signature\n") + len(b"Fast Signature\n1\n1\n2\n")
(output_dir / "truncated.nbc").write_bytes(source[:truncated])
