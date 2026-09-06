#!/usr/bin/env python3
import hashlib
import pathlib
import struct
import sys

OUT = pathlib.Path(sys.argv[1])
OUT.mkdir(parents=True, exist_ok=True)
(OUT / "three-bytes.bin").write_bytes(b"abc")
(OUT / "short.f32le").write_bytes(b"\0" * 3)
(OUT / "trailing.f32le").write_bytes(b"\0" * 5)


def align(value, n):
    return (value + n - 1) // n * n


def entry(name, dims, offset, payload):
    raw_name = name.encode("utf-8")
    size = align(28 + 8 * len(dims) + len(raw_name), 8)
    body = bytearray(size)
    struct.pack_into("<IHBBIQQ", body, 0, size, len(raw_name), 1,
                     len(dims), 0, offset, len(payload))
    for i, dim in enumerate(dims):
        struct.pack_into("<Q", body, 28 + i * 8, dim)
    body[28 + 8 * len(dims):28 + 8 * len(dims) + len(raw_name)] = raw_name
    return body


def make(entries):
    directory = b"".join(entry(*item) for item in entries)
    payload_size = max((off + len(data) for _, _, off, data in entries),
                       default=0)
    payload = bytearray(payload_size)
    for _, _, off, data in entries:
        payload[off:off + len(data)] = data
    payload_start = align(104 + len(directory), 64)
    pack = bytearray(payload_start + len(payload))
    pack[:8] = b"NOCTWGT\0"
    struct.pack_into("<HHIIIQQ", pack, 8, 1, 0, 104, 0, len(entries),
                     len(directory), len(payload))
    pack[40:72] = hashlib.sha256(b"model").digest()
    pack[72:104] = hashlib.sha256(payload).digest()
    pack[104:104 + len(directory)] = directory
    pack[payload_start:] = payload
    return pack


def digest(data):
    return hashlib.sha256(data).hexdigest()


cases = []


def add(name, data, message, expected=None):
    path = OUT / name
    path.write_bytes(data)
    cases.append((name, digest(data) if expected is None else expected, message))


one = make([("a", [2], 0, struct.pack("<ff", 1.0, -2.5))])
two = make([
    ("a", [1], 0, struct.pack("<f", 1.0)),
    ("b", [1], 64, struct.pack("<f", 2.0)),
])
(OUT / "valid.nwt1").write_bytes(one)
(OUT / "valid.sha256").write_text(digest(one) + "\n")
(OUT / "valid-two.nwt1").write_bytes(two)

bad = bytearray(one); bad[0] ^= 1
add("bad-magic.nwt1", bad, "NWT1 magic is invalid")
bad = bytearray(one); struct.pack_into("<H", bad, 8, 2)
add("bad-version.nwt1", bad, "header version or reserved fields")
bad = bytearray(one); struct.pack_into("<I", bad, 12, 103)
add("bad-header-size.nwt1", bad, "header version or reserved fields")
bad = bytearray(one); struct.pack_into("<I", bad, 16, 1)
add("bad-header-flags.nwt1", bad, "header version or reserved fields")
bad = bytearray(one); struct.pack_into("<Q", bad, 24, 0xffffffffffffffff)
add("bad-section-overflow.nwt1", bad, "section sizes are invalid")
bad = bytearray(one); struct.pack_into("<Q", bad, 24, 48)
add("bad-directory-size.nwt1", bad, "directory byte count is inconsistent")
bad = bytearray(one); struct.pack_into("<I", bad, 20, 2)
add("bad-entry-count.nwt1", bad, "entry count exceeds the directory size")
bad = bytearray(one); struct.pack_into("<I", bad, 104, 39)
add("bad-entry-size.nwt1", bad, "entry size is invalid")
bad = bytearray(one); struct.pack_into("<H", bad, 108, 0)
add("bad-empty-name.nwt1", bad, "entry metadata is invalid")
bad = bytearray(one); bad[110] = 2
add("bad-dtype.nwt1", bad, "entry metadata is invalid")
bad = bytearray(one); bad[111] = 0
add("bad-rank-zero.nwt1", bad, "entry metadata is invalid")
bad = bytearray(one); bad[111] = 9
add("bad-rank-nine.nwt1", bad, "entry metadata is invalid")
bad = bytearray(one); struct.pack_into("<I", bad, 112, 1)
add("bad-entry-flags.nwt1", bad, "entry metadata is invalid")
bad = bytearray(one); struct.pack_into("<Q", bad, 132, 0)
add("bad-zero-dimension.nwt1", bad, "tensor shape is out-of-range")
bad = bytearray(one); struct.pack_into("<Q", bad, 132, 0x80000000)
add("bad-product.nwt1", bad, "tensor shape is out-of-range")
bad = bytearray(one); struct.pack_into("<Q", bad, 124, 4)
add("bad-byte-length.nwt1", bad, "tensor payload range is invalid")
bad = bytearray(one); struct.pack_into("<Q", bad, 116, 1)
add("bad-alignment.nwt1", bad, "tensor payload range is invalid")
bad = bytearray(one); struct.pack_into("<Q", bad, 116, 64)
add("bad-payload-range.nwt1", bad, "tensor payload range is invalid")
bad = bytearray(one); bad[140] = 0xff
add("bad-utf8.nwt1", bad, "name is not valid UTF-8")
bad = bytearray(one); bad[140] = 0
add("bad-name-nul.nwt1", bad, "name is not valid UTF-8")
bad = bytearray(one); bad[141] = 1
add("bad-entry-padding.nwt1", bad, "entry padding is not zero")
bad = bytearray(one); bad[160] = 1
add("bad-section-padding.nwt1", bad, "section padding is not zero")
bad = bytearray(one); bad[72] ^= 1
add("bad-payload-hash.nwt1", bad, "payload SHA-256 mismatch")
bad = bytearray(one); bad[192] ^= 1
add("bad-payload-data.nwt1", bad, "payload SHA-256 mismatch")
bad = bytearray(one) + b"x"
add("bad-trailing.nwt1", bad, "section sizes are invalid")
add("bad-pack-hash.nwt1", one, "pack SHA-256 mismatch", "0" * 64)

# Two-entry directory: each entry is 40 bytes at 104 and 144.
bad = bytearray(two); bad[140] = ord("b"); bad[180] = ord("a")
add("bad-name-order.nwt1", bad, "duplicate or out-of-order")
bad = bytearray(two); bad[180] = ord("a")
add("bad-name-duplicate.nwt1", bad, "duplicate or out-of-order")
bad = bytearray(two); struct.pack_into("<Q", bad, 156, 0)
add("bad-overlap.nwt1", bad, "payloads overlap")
bad = bytearray(two); bad[200] = 1
bad[72:104] = hashlib.sha256(bad[192:]).digest()
add("bad-payload-padding.nwt1", bad, "payload padding is not zero")

for size in range(len(one)):
    data = one[:size]
    add(f"truncated-{size:03d}.nwt1", data, "NWT1")

with (OUT / "errors.tsv").open("w", newline="\n") as f:
    for name, sha, message in cases:
        f.write(f"{name}\t{sha}\t{message}\n")
