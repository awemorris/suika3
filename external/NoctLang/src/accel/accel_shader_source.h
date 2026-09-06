/* -*- coding: utf-8; tab-width: 8; indent-tabs-mode: t; -*- */

/*
 * Noct Programming Language
 * Copyright (C) 2025, 2026 Awe Morris
 *
 * SPDX-License-Identifier: Zlib
 */

/*
 * Target-neutral textual compute-shader generation.
 */

#ifndef NOCT_ACCEL_SHADER_SOURCE_H
#define NOCT_ACCEL_SHADER_SOURCE_H

#include "accel_program.h"

enum accel_shader_source_dialect {
	ACCEL_SHADER_SOURCE_GLSL_ES_310,
	ACCEL_SHADER_SOURCE_HLSL,
	ACCEL_SHADER_SOURCE_MSL
};

struct accel_shader_source {
	char *data;
	size_t length;
};

/*
 * Generates one deterministic textual compute shader.
 *
 * Numeric SSA values and storage-buffer elements remain raw 32-bit words.
 * The generated shader converts words only at typed operation boundaries.
 */
bool
accel_shader_source_generate(
	int dialect,
	const struct accel_program *program,
	uint32_t kernel_index,
	struct accel_shader_source *result,
	char *error,
	size_t error_size);

/*
 * Releases one generated shader source and clears its fields.
 */
void
accel_shader_source_cleanup(
	struct accel_shader_source *source);

#endif
