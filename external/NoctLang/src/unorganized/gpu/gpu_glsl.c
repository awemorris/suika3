/* -*- coding: utf-8; tab-width: 8; indent-tabs-mode: t; -*- */

/*
 * Noct Programming Language
 * Copyright (c) 2025, 2026, Awe Morris
 */

#include <noct/noct.h>
#include "gpu_ir.h"

#include <stdlib.h>
#include <string.h>

static uint32_t
gpu_glsl_hash(const char *source, size_t source_size)
{
	uint32_t hash;
	size_t i;

	hash = 2166136261u;
	for (i = 0; i < source_size; i++) {
		hash ^= (unsigned char)source[i];
		hash *= 16777619u;
	}
	return hash;
}

bool
gpu_glsl_emit(const struct gpu_ir_kernel *ir,
		   char **source, size_t *source_size, uint32_t *content_hash,
		   char *error, size_t error_size)
{
	char *output;

	if (source == NULL || source_size == NULL || content_hash == NULL ||
	    !gpu_ir_kernel_validate(ir, error, error_size))
		return false;
	output = noct_malloc(ir->structured_source_size + 1);
	if (output == NULL) {
		if (error != NULL && error_size != 0) {
			strncpy(error, "Out of memory emitting GLSL.", error_size - 1);
			error[error_size - 1] = '\0';
		}
		return false;
	}
	memcpy(output, ir->structured_source, ir->structured_source_size + 1);
	*source = output;
	*source_size = ir->structured_source_size;
	*content_hash = gpu_glsl_hash(output, *source_size);
	return true;
}
