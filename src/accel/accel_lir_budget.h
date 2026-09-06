/* -*- coding: utf-8; tab-width: 8; indent-tabs-mode: t; -*- */

/*
 * Noct Programming Language
 * Copyright (c) 2025, 2026, Awe Morris
 */

/*
 * Pure LIR temporary-slot preflight for accelerator rewrites.
 */

#ifndef NOCT_ACCEL_LIR_BUDGET_H
#define NOCT_ACCEL_LIR_BUDGET_H

#include "accel_program.h"

/*
 * Checks the final virtual HIR against the serialized LIR slot limit.
 */
enum accel_compile_status
accel_lir_budget_check(
	struct hir_block *func_block,
	const struct accel_function_plan *plan,
	uint32_t *serialized_tmpvar_size);

#endif
