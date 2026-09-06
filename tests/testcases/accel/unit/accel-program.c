/* -*- coding: utf-8; tab-width: 8; indent-tabs-mode: t; -*- */

/*
 * Noct Programming Language
 * Copyright (c) 2025, 2026, Awe Morris
 */

#include "accel.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct accel_kernel *
accel_kernel_clone(const struct accel_kernel *src)
{
	struct accel_kernel *dst;
	if (src == NULL) return NULL;
	dst = malloc(sizeof(*dst));
	if (dst != NULL) memcpy(dst, src, sizeof(*dst));
	return dst;
}

void
accel_kernel_free(struct accel_kernel *kernel)
{
	free(kernel);
}

int
main(void)
{
	struct accel_program program;
	struct accel_program *clone;
	struct accel_expr expr[2];
	struct accel_buffer_desc buffer;
	struct accel_kernel kernel;
	struct accel_kernel *kernel_table[1];
	struct accel_program_step step;
	int64_t value;
	char error[128];

	memset(&program, 0, sizeof(program));
	memset(expr, 0, sizeof(expr));
	memset(&buffer, 0, sizeof(buffer));
	memset(&kernel, 0, sizeof(kernel));
	memset(&step, 0, sizeof(step));
	program.descriptor_version = ACCEL_PROGRAM_VERSION;
	program.name = "unit";
	program.source_name = "unit.noct";
	program.outer_param_count = 1;
	program.expr_count = 2;
	program.expr = expr;
	expr[0].op = ACCEL_EXPR_CONST;
	expr[0].value = 17;
	expr[1].op = ACCEL_EXPR_CEIL_DIV_CONST;
	expr[1].left = 0;
	expr[1].value = 8;
	program.buffer_count = 1;
	program.buffer = &buffer;
	buffer.id = 0;
	buffer.name = "tmp";
	buffer.origin = ACCEL_BUFFER_LOCAL;
	buffer.outer_param = -1;
	buffer.element_width = 4;
	buffer.length_expr = 0;
	program.kernel_count = 1;
	program.kernel = kernel_table;
	kernel_table[0] = &kernel;
	kernel.func_kind = NOCT_FUNC_GPU;
	program.step_count = 1;
	program.step = &step;
	step.kind = ACCEL_STEP_DOALL_DISPATCH;
	step.kernel = 0;
	step.trip_expr = 0;
	step.block_size = 64;
	step.binding_count = 1;
	step.binding[0].kind = ACCEL_BIND_BUFFER;
	step.binding[0].kernel_param = 0;
	step.binding[0].value = 0;
	if (!accel_program_validate(&program, error, sizeof(error))) {
		fprintf(stderr, "valid descriptor rejected: %s\n", error);
		return 1;
	}
	if (!accel_expr_evaluate(&program, 1, 0, NULL, NULL, &value) ||
	    value != 3)
		return 2;
	clone = accel_program_clone(&program);
	if (clone == NULL || clone->buffer == program.buffer ||
	    clone->kernel == program.kernel || clone->kernel[0] == &kernel)
		return 3;
	accel_program_free(clone);
	expr[1].left = 1;
	if (accel_program_validate(&program, error, sizeof(error)))
		return 4;
	puts("accel program descriptor passed");
	return 0;
}
