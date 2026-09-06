/* -*- coding: utf-8; tab-width: 8; indent-tabs-mode: t; -*- */

/*
 * Noct Programming Language
 * Copyright (C) 2025, 2026 Awe Morris
 *
 * SPDX-License-Identifier: Zlib
 */

/*
 * Private deterministic SPIR-V assembly compiler.
 */

#ifndef NOCT_ACCEL_VULKAN_SHADER_H
#define NOCT_ACCEL_VULKAN_SHADER_H

#include "accel_program.h"

#include <shaderc/shaderc.h>

/* shaderc includes stdbool.h even when Noct supplied its C89 bool typedef. */
#if defined(BOOL_DEF) && defined(bool)
#undef bool
#endif

struct accel_vulkan_spirv {
	uint32_t *word;
	size_t word_count;
};

/*
 * Compiles one target-neutral kernel into owned Vulkan SPIR-V words.
 */
enum accel_compile_status
accel_vulkan_shader_compile(
	shaderc_compiler_t compiler,
	shaderc_compile_options_t options,
	const struct accel_program *program,
	uint32_t kernel_index,
	struct accel_vulkan_spirv *result);

/*
 * Releases an owned SPIR-V result and clears its fields.
 */
void
accel_vulkan_shader_cleanup(
	struct accel_vulkan_spirv *spirv);

#endif
