/* -*- coding: utf-8; tab-width: 8; indent-tabs-mode: t; -*- */

/*
 * Noct Programming Language
 * Copyright (c) 2025, 2026, Awe Morris
 */

/* Stable compiler-only raw-GPU operation registry. */

#include "accel_ops.h"

#include <string.h>

static const struct accel_op_desc math_ops[] = {
#define ACCEL_MATH_OP(symbol, id, source, arity, scalar_types, capabilities, lowering, exceptional_policy, absolute_tolerance, relative_tolerance) \
	{ ACCEL_MATH, id, source, arity, scalar_types, capabilities, lowering, \
	  exceptional_policy, absolute_tolerance, relative_tolerance },
#include "accel_ops.def"
#undef ACCEL_MATH_OP
};

const struct accel_op_desc *
accel_math_lookup_source(const char *source)
{
	size_t i;
	if (source == NULL) return NULL;
	for (i = 0; i < sizeof(math_ops) / sizeof(math_ops[0]); i++)
		if (strcmp(math_ops[i].source_spelling, source) == 0)
			return &math_ops[i];
	return NULL;
}

const struct accel_op_desc *
accel_math_lookup_member(const char *member)
{
	size_t i;
	const char *source;
	if (member == NULL) return NULL;
	for (i = 0; i < sizeof(math_ops) / sizeof(math_ops[0]); i++) {
		source = math_ops[i].source_spelling;
		if (strncmp(source, "Accel.", 6) == 0 &&
		    strcmp(source + 6, member) == 0)
			return &math_ops[i];
	}
	return NULL;
}

const struct accel_op_desc *
accel_math_lookup_id(int function_id)
{
	size_t i;
	for (i = 0; i < sizeof(math_ops) / sizeof(math_ops[0]); i++)
		if (math_ops[i].function_id == function_id)
			return &math_ops[i];
	return NULL;
}

const char *
accel_op_glsl_function(const struct accel_op_desc *op)
{
	if (op == NULL) return NULL;
	switch (op->lowering) {
	case ACCEL_LOWER_SIGMOID: return "noct_math_sigmoid";
	case ACCEL_LOWER_RELU: return "noct_math_relu";
	case ACCEL_LOWER_EXP: return "exp";
	case ACCEL_LOWER_LOG: return "log";
	case ACCEL_LOWER_SQRT: return "sqrt";
	default: return NULL;
	}
}

const char *
accel_op_glsl_helper(const struct accel_op_desc *op)
{
	if (op == NULL) return NULL;
	switch (op->lowering) {
	case ACCEL_LOWER_SIGMOID:
		return
			"float noct_math_sigmoid(float x) {\n"
			"    if (isnan(x)) return x;\n"
			"    if (x >= 0.0) return 1.0 / (1.0 + exp(-x));\n"
			"    float z = exp(x);\n"
			"    return z / (1.0 + z);\n"
			"}\n";
	case ACCEL_LOWER_RELU:
		return
			"float noct_math_relu(float x) {\n"
			"    if (isnan(x)) return x;\n"
			"    return x > 0.0 ? x : 0.0;\n"
			"}\n";
	default:
		return NULL;
	}
}
