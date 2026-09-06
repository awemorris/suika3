/* -*- coding: utf-8; tab-width: 8; indent-tabs-mode: t; -*- */

/*
 * Noct Programming Language
 * Copyright (c) 2025, 2026, Awe Morris
 */

/* Owned, backend-neutral accelerator program descriptors. */

#include "accel.h"

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ACCEL_INT64_MAX ((int64_t)(((uint64_t)-1) >> 1))
#define ACCEL_INT64_MIN (-ACCEL_INT64_MAX - 1)

static bool
accel_program_error(char *error, size_t size, const char *message)
{
	if (error != NULL && size != 0) {
		strncpy(error, message, size - 1);
		error[size - 1] = '\0';
	}
	return false;
}

void
accel_program_free(
	struct accel_program *program)
{
	uint32_t i;

	if (program == NULL)
		return;
	noct_free(program->name);
	noct_free(program->source_name);
	for (i = 0; i < program->buffer_count; i++)
		noct_free(program->buffer[i].name);
	for (i = 0; i < program->kernel_count; i++)
		accel_kernel_free(program->kernel[i]);
	noct_free(program->expr);
	noct_free(program->buffer);
	noct_free(program->kernel);
	noct_free(program->step);
	noct_free(program);
}

struct accel_program *
accel_program_clone(
	const struct accel_program *src)
{
	struct accel_program *dst;
	uint32_t i;

	if (src == NULL)
		return NULL;
	dst = noct_calloc(1, sizeof(*dst));
	if (dst == NULL)
		return NULL;
	*dst = *src;
	dst->name = NULL;
	dst->source_name = NULL;
	dst->expr = NULL;
	dst->buffer = NULL;
	dst->kernel = NULL;
	dst->step = NULL;
	if (src->name != NULL) {
		dst->name = noct_strdup(src->name);
		if (dst->name == NULL)
			goto failed;
	}
	if (src->source_name != NULL) {
		dst->source_name = noct_strdup(src->source_name);
		if (dst->source_name == NULL)
			goto failed;
	}
	if (src->expr_count != 0) {
		dst->expr = noct_malloc(sizeof(*dst->expr) * src->expr_count);
		if (dst->expr == NULL)
			goto failed;
		memcpy(dst->expr, src->expr,
		       sizeof(*dst->expr) * src->expr_count);
	}
	if (src->buffer_count != 0) {
		dst->buffer = noct_calloc(src->buffer_count, sizeof(*dst->buffer));
		if (dst->buffer == NULL)
			goto failed;
		for (i = 0; i < src->buffer_count; i++) {
			dst->buffer[i] = src->buffer[i];
			dst->buffer[i].name = NULL;
			if (src->buffer[i].name != NULL) {
				dst->buffer[i].name = noct_strdup(src->buffer[i].name);
				if (dst->buffer[i].name == NULL)
					goto failed;
			}
		}
	}
	if (src->kernel_count != 0) {
		dst->kernel = noct_calloc(src->kernel_count, sizeof(*dst->kernel));
		if (dst->kernel == NULL)
			goto failed;
		for (i = 0; i < src->kernel_count; i++) {
			dst->kernel[i] = accel_kernel_clone(src->kernel[i]);
			if (dst->kernel[i] == NULL)
				goto failed;
		}
	}
	if (src->step_count != 0) {
		dst->step = noct_malloc(sizeof(*dst->step) * src->step_count);
		if (dst->step == NULL)
			goto failed;
		memcpy(dst->step, src->step,
		       sizeof(*dst->step) * src->step_count);
	}
	return dst;

failed:
	accel_program_free(dst);
	return NULL;
}

bool
accel_program_validate(
	const struct accel_program *program,
	char *error,
	size_t error_size)
{
	uint32_t i;
	uint32_t j;
	const struct accel_expr *expr;
	const struct accel_buffer_desc *buffer;
	const struct accel_program_step *step;

	if (program == NULL)
		return accel_program_error(error, error_size, "null program");
	if (program->descriptor_version != ACCEL_PROGRAM_VERSION)
		return accel_program_error(error, error_size,
					   "unsupported program version");
	if (program->outer_param_count > NOCT_ARG_MAX ||
	    program->expr_count > ACCEL_PROGRAM_MAX_EXPRS ||
	    program->buffer_count > ACCEL_PROGRAM_MAX_BUFFERS ||
	    program->kernel_count > ACCEL_PROGRAM_MAX_KERNELS ||
	    program->step_count > ACCEL_PROGRAM_MAX_STEPS)
		return accel_program_error(error, error_size, "program limit exceeded");
	if ((program->expr_count != 0 && program->expr == NULL) ||
	    (program->buffer_count != 0 && program->buffer == NULL) ||
	    (program->kernel_count != 0 && program->kernel == NULL) ||
	    (program->step_count != 0 && program->step == NULL))
		return accel_program_error(error, error_size, "missing program table");
	for (i = 0; i < program->expr_count; i++) {
		expr = &program->expr[i];
		switch (expr->op) {
		case ACCEL_EXPR_CONST:
			if (expr->value < 0)
				return accel_program_error(error, error_size,
							   "negative constant");
			break;
		case ACCEL_EXPR_SCALAR_ARG:
			if (expr->ref < 0 ||
			    (uint32_t)expr->ref >= program->outer_param_count)
				return accel_program_error(error, error_size,
							   "bad scalar argument");
			break;
		case ACCEL_EXPR_BUFFER_LENGTH:
			if (expr->ref < 0 ||
			    (uint32_t)expr->ref >= program->buffer_count)
				return accel_program_error(error, error_size,
							   "bad buffer reference");
			break;
		case ACCEL_EXPR_ADD_CONST:
		case ACCEL_EXPR_MUL_CONST:
		case ACCEL_EXPR_CEIL_DIV_CONST:
			if (expr->left < 0 || (uint32_t)expr->left >= i ||
			    ((expr->op == ACCEL_EXPR_MUL_CONST ||
			      expr->op == ACCEL_EXPR_CEIL_DIV_CONST) &&
			     expr->value <= 0))
				return accel_program_error(error, error_size,
							   "bad unary expression");
			break;
		case ACCEL_EXPR_MIN:
		case ACCEL_EXPR_MAX:
			if (expr->left < 0 || expr->right < 0 ||
			    (uint32_t)expr->left >= i ||
			    (uint32_t)expr->right >= i)
				return accel_program_error(error, error_size,
							   "bad binary expression");
			break;
		default:
			return accel_program_error(error, error_size,
						   "bad expression opcode");
		}
	}
	for (i = 0; i < program->buffer_count; i++) {
		buffer = &program->buffer[i];
		if (buffer->id != (int)i || buffer->name == NULL ||
		    buffer->element_width <= 0 || buffer->length_expr < 0 ||
		    (uint32_t)buffer->length_expr >= program->expr_count)
			return accel_program_error(error, error_size,
						   "bad buffer descriptor");
		if (buffer->outer_param >= 0 &&
		    (uint32_t)buffer->outer_param >= program->outer_param_count)
			return accel_program_error(error, error_size,
						   "bad buffer argument");
	}
	for (i = 0; i < program->kernel_count; i++) {
		if (program->kernel[i] == NULL ||
		    program->kernel[i]->func_kind != NOCT_FUNC_GPU)
			return accel_program_error(error, error_size,
						   "bad internal kernel");
	}
	for (i = 0; i < program->step_count; i++) {
		step = &program->step[i];
		if (step->binding_count > NOCT_ARG_MAX)
			return accel_program_error(error, error_size,
						   "too many bindings");
		if (step->kind == ACCEL_STEP_DOALL_DISPATCH) {
			if (step->kernel < 0 ||
			    (uint32_t)step->kernel >= program->kernel_count ||
			    step->trip_expr < 0 ||
			    (uint32_t)step->trip_expr >= program->expr_count ||
			    step->block_size == 0)
				return accel_program_error(error, error_size,
							   "bad DOALL step");
		} else if (step->kind == ACCEL_STEP_DOSUM_REDUCTION) {
			if (step->kernel < 0 || step->fold_kernel < 0 ||
			    (uint32_t)step->kernel >= program->kernel_count ||
			    (uint32_t)step->fold_kernel >= program->kernel_count ||
			    step->trip_expr < 0 ||
			    (uint32_t)step->trip_expr >= program->expr_count ||
			    step->result_buffer < 0 ||
			    step->scratch_buffer < 0 ||
			    step->scratch_buffer2 < 0 ||
			    (uint32_t)step->result_buffer >= program->buffer_count ||
			    (uint32_t)step->scratch_buffer >= program->buffer_count ||
			    (uint32_t)step->scratch_buffer2 >= program->buffer_count ||
			    step->result_buffer == step->scratch_buffer ||
			    step->result_buffer == step->scratch_buffer2 ||
			    step->scratch_buffer == step->scratch_buffer2 ||
			    step->reduction_operator != ACCEL_REDUCTION_ADD ||
			    (step->reduction_type != NOCT_PACKED_INT32 &&
			     step->reduction_type != NOCT_PACKED_UINT32 &&
			     step->reduction_type != NOCT_PACKED_FLOAT32) ||
			    step->block_size == 0)
				return accel_program_error(error, error_size,
							   "bad DOSUM step");
		} else if (step->kind != ACCEL_STEP_DEVICE_COPY) {
			return accel_program_error(error, error_size, "bad step kind");
		}
		for (j = 0; j < step->binding_count; j++) {
			if (step->binding[j].kernel_param < 0 ||
			    step->binding[j].kernel_param >= NOCT_ARG_MAX)
				return accel_program_error(error, error_size,
							   "bad kernel binding");
			if (step->binding[j].kind == ACCEL_BIND_BUFFER) {
				if (step->binding[j].value < 0 ||
				    (uint32_t)step->binding[j].value >=
					program->buffer_count)
					return accel_program_error(error, error_size,
								   "bad buffer binding");
			} else if (step->binding[j].kind ==
				   ACCEL_BIND_SCALAR_EXPR) {
				if (step->binding[j].value < 0 ||
				    (uint32_t)step->binding[j].value >=
					program->expr_count)
					return accel_program_error(error, error_size,
								   "bad scalar binding");
			} else {
				return accel_program_error(error, error_size,
							   "bad binding kind");
			}
		}
	}
	if (error != NULL && error_size != 0)
		error[0] = '\0';
	return true;
}

static bool
accel_add_checked(int64_t left, int64_t right, int64_t *result)
{
	if ((right > 0 && left > ACCEL_INT64_MAX - right) ||
	    (right < 0 && left < ACCEL_INT64_MIN - right))
		return false;
	*result = left + right;
	return true;
}

static bool
accel_mul_checked(int64_t left, int64_t right, int64_t *result)
{
	if (left < 0 || right < 0)
		return false;
	if (left != 0 && right > ACCEL_INT64_MAX / left)
		return false;
	*result = left * right;
	return true;
}

bool
accel_expr_evaluate(
	const struct accel_program *program,
	int expr_index,
	uint32_t arg_count,
	const int64_t *scalar_arg,
	const int64_t *buffer_length,
	int64_t *result)
{
	int64_t value[ACCEL_PROGRAM_MAX_EXPRS];
	const struct accel_expr *expr;
	uint32_t i;
	int64_t numerator;

	if (program == NULL || result == NULL || expr_index < 0 ||
	    (uint32_t)expr_index >= program->expr_count ||
	    program->expr_count > ACCEL_PROGRAM_MAX_EXPRS)
		return false;
	for (i = 0; i <= (uint32_t)expr_index; i++) {
		expr = &program->expr[i];
		switch (expr->op) {
		case ACCEL_EXPR_CONST:
			value[i] = expr->value;
			break;
		case ACCEL_EXPR_SCALAR_ARG:
			if (expr->ref < 0 || (uint32_t)expr->ref >= arg_count ||
			    scalar_arg == NULL)
				return false;
			value[i] = scalar_arg[expr->ref];
			break;
		case ACCEL_EXPR_BUFFER_LENGTH:
			if (expr->ref < 0 ||
			    (uint32_t)expr->ref >= program->buffer_count ||
			    buffer_length == NULL)
				return false;
			value[i] = buffer_length[expr->ref];
			break;
		case ACCEL_EXPR_ADD_CONST:
			if (!accel_add_checked(value[expr->left], expr->value,
					       &value[i]))
				return false;
			break;
		case ACCEL_EXPR_MUL_CONST:
			if (!accel_mul_checked(value[expr->left], expr->value,
					       &value[i]))
				return false;
			break;
		case ACCEL_EXPR_MIN:
			value[i] = value[expr->left] < value[expr->right] ?
				value[expr->left] : value[expr->right];
			break;
		case ACCEL_EXPR_MAX:
			value[i] = value[expr->left] > value[expr->right] ?
				value[expr->left] : value[expr->right];
			break;
		case ACCEL_EXPR_CEIL_DIV_CONST:
			if (value[expr->left] < 0 || expr->value <= 0 ||
			    !accel_add_checked(value[expr->left], expr->value - 1,
					       &numerator))
				return false;
			value[i] = numerator / expr->value;
			break;
		default:
			return false;
		}
	}
	*result = value[expr_index];
	return true;
}
