/* -*- coding: utf-8; tab-width: 8; indent-tabs-mode: t; -*- */

/*
 * Noct Programming Language
 * Copyright (c) 2025, 2026, Awe Morris
 */

/*
 * Private accelerator compiler contracts.
 */

#ifndef NOCT_ACCEL_PRIVATE_H
#define NOCT_ACCEL_PRIVATE_H

#include "hir.h"

#define ACCEL_MAX_SCALAR_BINDINGS	HIR_PARAM_SIZE
#define ACCEL_MAX_BUFFER_BINDINGS	64
#define ACCEL_MAX_KERNELS		32
#define ACCEL_MAX_LOCALS		256
#define ACCEL_MAX_IR_VALUES		1024
#define ACCEL_MAX_IR_INSTRUCTIONS	1024
#define ACCEL_MAX_SIZE_EXPRESSIONS	512

enum accel_compile_status {
	ACCEL_COMPILE_APPLIED,
	ACCEL_COMPILE_DECLINED,
	ACCEL_COMPILE_ERROR
};

enum accel_decline_reason {
	ACCEL_DECLINE_NONE,
	ACCEL_DECLINE_NOT_ACCEL,
	ACCEL_DECLINE_RETURN_TYPE,
	ACCEL_DECLINE_PARAMETER_TYPE,
	ACCEL_DECLINE_PARAMETER_ALIAS,
	ACCEL_DECLINE_LIMIT,
	ACCEL_DECLINE_MEMORY_CATALOG,
	ACCEL_DECLINE_CONTROL_FLOW,
	ACCEL_DECLINE_LOOP_ANALYSIS,
	ACCEL_DECLINE_NOT_DOALL,
	ACCEL_DECLINE_LOCAL_BUFFER,
	ACCEL_DECLINE_RANGE,
	ACCEL_DECLINE_EXPRESSION,
	ACCEL_DECLINE_BUFFER_ESCAPE
};

struct accel_function_plan;

struct accel_compile_context {
	struct hir_block *func_block;
	struct accel_function_plan *plan;
	int decline_reason;
	bool error_reported;
};

/*
 * Compiles one accelerator-hinted function into a private function plan.
 */
enum accel_compile_status
accel_compile_func(
	struct hir_block *func_block,
	struct accel_function_plan **result);

#endif
