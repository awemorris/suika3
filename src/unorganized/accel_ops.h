/* -*- coding: utf-8; tab-width: 8; indent-tabs-mode: t; -*- */

/*
 * Noct Programming Language
 * Copyright (c) 2025, 2026, Awe Morris
 */

#ifndef NOCT_ACCEL_OPS_H
#define NOCT_ACCEL_OPS_H

#include <noct/noct.h>

enum accel_op_class {
	ACCEL_OP_CLASS_NONE = 0,
	ACCEL_MATH = 1,
	ACCEL_REDUCE = 2,
	ACCEL_TENSOR = 3
};

enum accel_scalar_type_mask {
	ACCEL_SCALAR_FLOAT32 = 1
};

enum accel_op_capability {
	ACCEL_OP_CAP_RAW_GLSL = 1,
	ACCEL_OP_CAP_OPENGL = 2,
	ACCEL_OP_CAP_VULKAN = 4
};

enum accel_op_lowering {
	ACCEL_LOWER_NONE = 0,
	ACCEL_LOWER_SIGMOID,
	ACCEL_LOWER_RELU,
	ACCEL_LOWER_EXP,
	ACCEL_LOWER_LOG,
	ACCEL_LOWER_SQRT
};

enum accel_exception_policy {
	ACCEL_EXCEPTION_IEEE = 1,
	ACCEL_EXCEPTION_NAN_ZERO_EXPLICIT,
	ACCEL_EXCEPTION_SIGMOID_STABLE
};

enum accel_math_id {
	ACCEL_MATH_NONE = 0,
#define ACCEL_MATH_OP(symbol, id, source, arity, scalar_types, capabilities, lowering, exceptional_policy, absolute_tolerance, relative_tolerance) \
	ACCEL_MATH_##symbol = id,
#include "accel_ops.def"
#undef ACCEL_MATH_OP
	ACCEL_MATH_ID_LIMIT = 24
};

struct accel_op_desc {
	int op_class;
	int function_id;
	const char *source_spelling;
	int arity;
	unsigned int scalar_types;
	unsigned int capabilities;
	int lowering;
	int exceptional_policy;
	double absolute_tolerance;
	double relative_tolerance;
};

const struct accel_op_desc *accel_math_lookup_source(const char *source);
const struct accel_op_desc *accel_math_lookup_member(const char *member);
const struct accel_op_desc *accel_math_lookup_id(int function_id);
const char *accel_op_glsl_helper(const struct accel_op_desc *op);
const char *accel_op_glsl_function(const struct accel_op_desc *op);

#endif
