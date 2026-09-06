#!/usr/bin/env python3
"""Build deterministic Stage-G.2 Gemm/MatMul models and test NWT1 packs."""

import importlib.util
import struct
import sys
from pathlib import Path


HERE = Path(__file__).resolve().parent
SPEC = importlib.util.spec_from_file_location(
    "stage_g1_fixtures", HERE / "make-stage-g1-fixtures.py"
)
G1 = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(G1)
R = G1.R


def attr_int(name, value):
    return R.s(1, name) + R.v(20, 2) + R.v(3, value)


def attr_float(name, bits):
    return R.s(1, name) + R.v(20, 1) + R.f32(2, bits)


def matmul(a, a_shape, b, b_shape):
    m, k = a_shape
    bk, n = b_shape
    assert k == bk
    result = []
    for row in range(m):
        for col in range(n):
            total = 0.0
            for inner in range(k):
                total += a[row * k + inner] * b[inner * n + col]
            result.append(total)
    return result


def gemm(a, a_shape, b, b_shape, c, alpha, beta, trans_a, trans_b):
    a_rows, a_cols = a_shape
    b_rows, b_cols = b_shape
    m = a_cols if trans_a else a_rows
    k = a_rows if trans_a else a_cols
    bk = b_cols if trans_b else b_rows
    n = b_rows if trans_b else b_cols
    assert k == bk and len(c) == n
    result = []
    for row in range(m):
        for col in range(n):
            total = 0.0
            for inner in range(k):
                ai = inner * a_cols + row if trans_a else row * a_cols + inner
                bi = col * b_cols + inner if trans_b else inner * b_cols + col
                total += a[ai] * b[bi]
            result.append(alpha * total + beta * c[col])
    return result, (m, n)


def main():
    root = Path(sys.argv[1])
    root.mkdir(parents=True, exist_ok=True)

    gemm_input = [1.0, 3.0, 5.0]
    gemm_weight = [
        1.0, 0.0, 1.0,
        0.0, 1.0, 1.0,
        1.0, 1.0, 0.0,
        -1.0, 1.0, 0.5,
    ]
    gemm_bias = [1.0, -1.0, 0.5, 2.0]
    b_tensor, b_raw = G1.float_tensor("gemm_b", (4, 3), gemm_weight)
    c_tensor, c_raw = G1.float_tensor("gemm_c", (4,), gemm_bias)
    gemm_transpose = R.node(("x",), ("a",), "Transpose", attrs=(
        G1.attr_ints("perm", [1, 0]),
    ))
    gemm_node = R.node(("a", "gemm_b", "gemm_c"), ("y",), "Gemm", attrs=(
        attr_float("alpha", 0x3F000000),
        attr_float("beta", 0xC0000000),
        attr_int("transA", 1),
        attr_int("transB", 1),
    ))
    gemm_model = R.model(R.graph(
        nodes=[gemm_transpose, gemm_node], initializers=[b_tensor, c_tensor],
        inputs=[R.value_info("x", (1, 3))],
        outputs=[R.value_info("y", (1, 4))],
    ))
    gemm_output, gemm_shape = gemm(
        gemm_input, (3, 1), gemm_weight, (4, 3), gemm_bias,
        0.5, -2.0, True, True,
    )
    assert gemm_shape == (1, 4)
    G1.write_fixture(root, "gemm-transpose-broadcast", gemm_model,
                     gemm_input, gemm_output, [
        ("gemm_b", (4, 3), b_raw),
        ("gemm_c", (4,), c_raw),
    ])
    wrong_name_pack = G1.make_nwt1(gemm_model, [
        ("gemm_b", (4, 3), b_raw),
        ("wrong_c", (4,), c_raw),
    ])
    (root / "gemm-transpose-broadcast.wrong-name.weights").write_bytes(
        wrong_name_pack
    )
    corrupt_pack = bytearray(
        (root / "gemm-transpose-broadcast.weights").read_bytes()
    )
    corrupt_pack[-1] ^= 1
    (root / "gemm-transpose-broadcast.corrupt.weights").write_bytes(corrupt_pack)

    no_bias_weight = [1.0, -1.0, 0.5, 2.0, 2.0, 0.0]
    no_bias_tensor, no_bias_raw = G1.float_tensor(
        "gemm_no_bias_b", (3, 2), no_bias_weight
    )
    no_bias_node = R.node(("x", "gemm_no_bias_b"), ("y",), "Gemm")
    no_bias_model = R.model(R.graph(
        nodes=[no_bias_node], initializers=[no_bias_tensor],
        inputs=[R.value_info("x", (1, 3))],
        outputs=[R.value_info("y", (1, 2))],
    ))
    no_bias_output = matmul(gemm_input, (1, 3), no_bias_weight, (3, 2))
    G1.write_fixture(root, "gemm-no-bias", no_bias_model, gemm_input,
                     no_bias_output, [
        ("gemm_no_bias_b", (3, 2), no_bias_raw),
    ])

    matmul_input = [1.0, 2.0, 3.0]
    matmul_weight = [1.0, -1.0, 0.5, 2.0, 2.0, 0.0]
    mm_tensor, mm_raw = G1.float_tensor("matmul_b", (3, 2), matmul_weight)
    matmul_nodes = [
        R.node(("x", "matmul_b"), ("left",), "MatMul"),
        R.node(("x", "matmul_b"), ("right",), "MatMul"),
        R.node(("left", "right"), ("y",), "Add"),
    ]
    matmul_model = R.model(R.graph(
        nodes=matmul_nodes, initializers=[mm_tensor],
        inputs=[R.value_info("x", (1, 3))],
        outputs=[R.value_info("y", (1, 2))],
    ))
    matmul_once = matmul(matmul_input, (1, 3), matmul_weight, (3, 2))
    matmul_output = [value + value for value in matmul_once]
    G1.write_fixture(root, "matmul-reuse", matmul_model, matmul_input,
                     matmul_output, [("matmul_b", (3, 2), mm_raw)])

    negative_weight = [1.0, -1.0]
    negative_tensor, _ = G1.float_tensor("negative_b", (1, 2), negative_weight)
    transposed = R.node(("negative_b",), ("bt",), "Transpose", attrs=(
        G1.attr_ints("perm", [1, 0]),
    ))
    transposed_weight_matmul = R.node(("x", "bt"), ("y",), "MatMul")
    (root / "unsupported-transposed-weight.onnx").write_bytes(R.model(R.graph(
        nodes=[transposed, transposed_weight_matmul],
        initializers=[negative_tensor],
        inputs=[R.value_info("x", (1, 2))],
        outputs=[R.value_info("y", (1, 1))],
    )))


if __name__ == "__main__":
    main()
