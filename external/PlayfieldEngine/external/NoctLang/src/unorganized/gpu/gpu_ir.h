/* -*- coding: utf-8; tab-width: 8; indent-tabs-mode: t; -*- */

/*
 * Noct Programming Language
 * Copyright (c) 2025, 2026, Awe Morris
 */

#ifndef NOCT_GPU_IR_H
#define NOCT_GPU_IR_H

#include "accel.h"

#define GPU_IR_VERSION 1
#define GPU_IR_SOURCE_MAX (1024u * 1024u)

enum gpu_ir_param_kind {
	GPU_IR_PARAM_SCALAR,
	GPU_IR_PARAM_BUFFER
};

struct gpu_ir_param {
	int kind;
	int scalar_type;
	int element_type;
	int binding;
	unsigned int effect;
};

/*
 * Backend-neutral kernel contract.  The structured source stream is owned by
 * this object; it contains only code already type-checked by the AST/HIR
 * adapter.  Backend emitters must validate this contract before consuming it.
 */
struct gpu_ir_kernel {
	uint32_t version;
	uint32_t param_count;
	struct gpu_ir_param param[NOCT_ARG_MAX];
	uint32_t local_size_x;
	bool local_size_specialized;
	bool uses_shared;
	bool uses_barrier;
	char *structured_source;
	size_t structured_source_size;
};

void gpu_ir_kernel_init(struct gpu_ir_kernel *ir);
void gpu_ir_kernel_free(struct gpu_ir_kernel *ir);
bool gpu_ir_kernel_build(struct gpu_ir_kernel *ir,
			 const struct accel_kernel *kernel,
			 const char *source, size_t source_size,
			 char *error, size_t error_size);
bool gpu_ir_kernel_validate(const struct gpu_ir_kernel *ir,
			    char *error, size_t error_size);
bool gpu_glsl_emit(const struct gpu_ir_kernel *ir,
		   char **source, size_t *source_size, uint32_t *content_hash,
		   char *error, size_t error_size);

/* Common final boundary used by raw __gpu func and generated accel kernels. */
bool gpu_ir_finalize_kernel(struct accel_kernel *kernel,
			    const char *source, size_t source_size,
			    char *error, size_t error_size);

#endif
