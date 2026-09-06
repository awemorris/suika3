/* -*- coding: utf-8; tab-width: 8; indent-tabs-mode: t; -*- */

/*
 * Noct Programming Language
 * Copyright (c) 2025, 2026, Awe Morris
 */

#include <noct/noct.h>
#include "gpu_ir.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static bool
gpu_ir_error(char *error, size_t error_size, const char *message)
{
	if (error != NULL && error_size != 0) {
		strncpy(error, message, error_size - 1);
		error[error_size - 1] = '\0';
	}
	return false;
}

void
gpu_ir_kernel_init(struct gpu_ir_kernel *ir)
{
	uint32_t i;

	memset(ir, 0, sizeof(*ir));
	ir->version = GPU_IR_VERSION;
	for (i = 0; i < NOCT_ARG_MAX; i++) {
		ir->param[i].scalar_type = -1;
		ir->param[i].element_type = -1;
		ir->param[i].binding = -1;
	}
}

void
gpu_ir_kernel_free(struct gpu_ir_kernel *ir)
{
	if (ir == NULL)
		return;
	noct_free(ir->structured_source);
	gpu_ir_kernel_init(ir);
}

bool
gpu_ir_kernel_build(struct gpu_ir_kernel *ir,
			 const struct accel_kernel *kernel,
			 const char *source, size_t source_size,
			 char *error, size_t error_size)
{
	struct gpu_ir_param *param;
	uint32_t i;

	if (ir == NULL || kernel == NULL || source == NULL)
		return gpu_ir_error(error, error_size,
				    "Invalid GPU IR build argument.");
	gpu_ir_kernel_init(ir);
	if (kernel->param_count > NOCT_ARG_MAX)
		return gpu_ir_error(error, error_size,
				    "GPU IR parameter limit exceeded.");
	if (source_size == 0 || source_size > GPU_IR_SOURCE_MAX ||
	    memchr(source, '\0', source_size) != NULL)
		return gpu_ir_error(error, error_size,
				    "GPU IR source stream has an invalid size.");
	ir->structured_source = noct_malloc(source_size + 1);
	if (ir->structured_source == NULL)
		return gpu_ir_error(error, error_size,
				    "Out of memory building GPU IR.");
	memcpy(ir->structured_source, source, source_size);
	ir->structured_source[source_size] = '\0';
	ir->structured_source_size = source_size;
	ir->param_count = kernel->param_count;
	ir->local_size_x = 1;
	ir->local_size_specialized =
		strstr(source, "__NOCT_LOCAL_SIZE_X__") == NULL;
	ir->uses_shared = strstr(source, "shared ") != NULL;
	ir->uses_barrier = strstr(source, "barrier()") != NULL;
	for (i = 0; i < kernel->param_count; i++) {
		param = &ir->param[i];
		param->binding = (int)i;
		param->effect = kernel->param_effect[i];
		if (kernel->param_transport[i] == ACCEL_TRANSPORT_SCALAR) {
			param->kind = GPU_IR_PARAM_SCALAR;
			param->scalar_type = kernel->param_type[i];
		} else {
			param->kind = GPU_IR_PARAM_BUFFER;
			param->element_type = kernel->param_packed_type[i];
		}
	}
	if (!gpu_ir_kernel_validate(ir, error, error_size)) {
		gpu_ir_kernel_free(ir);
		return false;
	}
	return true;
}

bool
gpu_ir_kernel_validate(const struct gpu_ir_kernel *ir,
			    char *error, size_t error_size)
{
	const struct gpu_ir_param *param;
	uint32_t i;

	if (ir == NULL || ir->version != GPU_IR_VERSION ||
	    ir->param_count > NOCT_ARG_MAX || ir->structured_source == NULL ||
	    ir->structured_source_size == 0 ||
	    ir->structured_source_size > GPU_IR_SOURCE_MAX)
		return gpu_ir_error(error, error_size,
				    "Invalid GPU IR kernel descriptor.");
	if (strncmp(ir->structured_source, "#version 450", 12) != 0 ||
	    strstr(ir->structured_source, "local_size_x") == NULL ||
	    strstr(ir->structured_source, "layout(push_constant)") == NULL ||
	    strstr(ir->structured_source, "void main()") == NULL)
		return gpu_ir_error(error, error_size,
				    "GPU IR kernel is missing a required entry contract.");
	for (i = 0; i < ir->param_count; i++) {
		param = &ir->param[i];
		if (param->binding != (int)i)
			return gpu_ir_error(error, error_size,
					    "GPU IR parameter binding is not deterministic.");
		if (param->kind == GPU_IR_PARAM_SCALAR) {
			if (param->scalar_type != NOCT_VALUE_INT &&
			    param->scalar_type != NOCT_VALUE_FLOAT)
				return gpu_ir_error(error, error_size,
						    "GPU IR has an unsupported scalar type.");
		} else if (param->kind == GPU_IR_PARAM_BUFFER) {
			if (param->element_type != NOCT_PACKED_INT32 &&
			    param->element_type != NOCT_PACKED_UINT32 &&
			    param->element_type != NOCT_PACKED_FLOAT32)
				return gpu_ir_error(error, error_size,
						    "GPU IR has an unsupported buffer type.");
		} else {
			return gpu_ir_error(error, error_size,
					    "GPU IR has an invalid parameter kind.");
		}
	}
	if (error != NULL && error_size != 0)
		error[0] = '\0';
	return true;
}

bool
gpu_ir_finalize_kernel(struct accel_kernel *kernel,
			    const char *source, size_t source_size,
			    char *error, size_t error_size)
{
	struct gpu_ir_kernel ir;
	char *glsl;
	size_t glsl_size;
	uint32_t content_hash;

	if (!gpu_ir_kernel_build(&ir, kernel, source, source_size,
				 error, error_size))
		return false;
	glsl = NULL;
	if (!gpu_glsl_emit(&ir, &glsl, &glsl_size, &content_hash,
			   error, error_size)) {
		gpu_ir_kernel_free(&ir);
		return false;
	}
	gpu_ir_kernel_free(&ir);
	noct_free(kernel->glsl);
	kernel->glsl = glsl;
	kernel->glsl_size = glsl_size;
	kernel->content_hash = content_hash;
	return true;
}
