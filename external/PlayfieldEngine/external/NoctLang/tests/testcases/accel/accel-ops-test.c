/* -*- coding: utf-8; tab-width: 8; indent-tabs-mode: t; -*- */

/*
 * Noct Programming Language
 * Copyright (c) 2025, 2026, Awe Morris
 */

/* Stable ID and capability audit for src/core/accel_ops.def. */

#include "accel_ops.h"

#include <stdio.h>
#include <string.h>

struct expected_math {
	int id;
	const char *source;
	int arity;
};

int
main(void)
{
	static const struct expected_math expected[] = {
		{ 1, "Accel.abs", 1 }, { 2, "Accel.neg", 1 },
		{ 3, "Accel.add", 2 }, { 4, "Accel.sub", 2 },
		{ 5, "Accel.mul", 2 }, { 6, "Accel.div", 2 },
		{ 7, "Accel.min", 2 }, { 8, "Accel.max", 2 },
		{ 9, "Accel.clip", 3 }, { 10, "Accel.sigmoid", 1 },
		{ 11, "Accel.relu", 1 }, { 12, "Accel.leakyRelu", 2 },
		{ 13, "Accel.tanh", 1 }, { 14, "Accel.exp", 1 },
		{ 15, "Accel.log", 1 }, { 16, "Accel.sqrt", 1 },
		{ 17, "Accel.pow", 2 }, { 18, "Accel.fma", 3 },
		{ 19, "Accel.softplus", 1 }, { 20, "Accel.silu", 1 },
		{ 21, "Accel.geluErf", 1 }, { 22, "Accel.geluTanh", 1 },
		{ 23, "Accel.erf", 1 }
	};
	const struct accel_op_desc *op;
	size_t i;
	if (ACCEL_MATH != 1 || ACCEL_REDUCE != 2 || ACCEL_TENSOR != 3)
		return 1;
	for (i = 0; i < sizeof(expected) / sizeof(expected[0]); i++) {
		op = accel_math_lookup_id(expected[i].id);
		if (op == NULL || op->op_class != ACCEL_MATH ||
		    strcmp(op->source_spelling, expected[i].source) != 0 ||
		    op->arity != expected[i].arity ||
		    accel_math_lookup_source(expected[i].source) != op ||
		    accel_math_lookup_member(expected[i].source + 6) != op)
			return 2;
		if (op->function_id == ACCEL_MATH_SIGMOID ||
		    op->function_id == ACCEL_MATH_RELU) {
			if ((op->capabilities & ACCEL_OP_CAP_RAW_GLSL) == 0 ||
			    (op->capabilities & ACCEL_OP_CAP_OPENGL) == 0 ||
			    (op->capabilities & ACCEL_OP_CAP_VULKAN) != 0 ||
			    accel_op_glsl_helper(op) == NULL ||
			    accel_op_glsl_function(op) == NULL)
				return 3;
		} else if (op->function_id == ACCEL_MATH_EXP ||
			   op->function_id == ACCEL_MATH_LOG ||
			   op->function_id == ACCEL_MATH_SQRT) {
			if ((op->capabilities & ACCEL_OP_CAP_RAW_GLSL) == 0 ||
			    (op->capabilities & ACCEL_OP_CAP_OPENGL) == 0 ||
			    (op->capabilities & ACCEL_OP_CAP_VULKAN) != 0 ||
			    accel_op_glsl_helper(op) != NULL ||
			    accel_op_glsl_function(op) == NULL)
				return 3;
		} else if (op->capabilities != 0 ||
			   accel_op_glsl_helper(op) != NULL ||
			   accel_op_glsl_function(op) != NULL) {
			return 4;
		}
	}
	if (accel_math_lookup_id(0) != NULL ||
	    accel_math_lookup_id(24) != NULL ||
	    accel_math_lookup_source("Accel.notRegistered") != NULL ||
	    accel_math_lookup_member("notRegistered") != NULL)
		return 5;
	puts("accelerator operation registry passed");
	return 0;
}
