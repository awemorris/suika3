/* -*- coding: utf-8; tab-width: 8; indent-tabs-mode: t; -*- */

/*
 * Noct Programming Language
 * Copyright (c) 2025, 2026, Awe Morris
 */

/*
 * Transactional accelerator optimizer callback.
 */

#include "accel_context.h"
#include "accel_lir_budget.h"
#include "accel_rewrite.h"
#include "hir.h"

#include <stdlib.h>
#include <string.h>

static bool accel_callback_optimize_claimed(void *func_data, void *userdata);
static void accel_callback_destroy_prepared(struct accel_context *context, struct accel_prepared_program prepared[], uint32_t count);
static bool accel_callback_error(struct hir_block *func_block, const char *message);

/*
 * Optimizes one accelerator-hinted function transactionally.
 */
bool
accel_optimize_callback(
	void *func_data,
	void *userdata)
{
	struct accel_context *context;
	struct hir_block *func_block;
	bool success;

	context = userdata;
	func_block = func_data;

	/* Rejects incomplete callback data before claiming shared state. */
	if (func_block == NULL)
		return false;
	if (context == NULL)
		return accel_callback_error(func_block, N_TR("Missing accelerator context."));

	/* Claims the registry and backend across the complete optimizer callback. */
	if (!accel_context_begin_operation(context)) {
		return accel_callback_error(
			func_block,
			N_TR("Detached accelerator context."));
	}

	success = accel_callback_optimize_claimed(func_data, userdata);
	accel_context_end_operation(context);

	return success;
}

/* Optimize one function while holding an external context lifetime claim. */
static bool
accel_callback_optimize_claimed(
	void *func_data,
	void *userdata)
{
	struct accel_context *context;
	struct hir_block *func_block;
	struct accel_function_plan *plan;
	struct accel_prepared_program *prepared;
	struct accel_registry_reservation *reservation;
	struct accel_registry_commit_guard guard;
	struct accel_rewrite *rewrite;
	const struct accel_program *program;
	enum accel_compile_status status;
	uint32_t serialized_tmpvar_size;
	uint32_t region_count;
	uint32_t prepared_count;
	uint32_t i;

	context = userdata;
	func_block = func_data;
	plan = NULL;
	prepared = NULL;
	reservation = NULL;
	rewrite = NULL;
	serialized_tmpvar_size = 0;
	prepared_count = 0;
	memset(&guard, 0, sizeof(guard));

	status = accel_compile_func(func_block, &plan);
	if (status == ACCEL_COMPILE_DECLINED) {
		accel_function_plan_destroy(plan);
		return true;
	}
	if (status == ACCEL_COMPILE_ERROR) {
		accel_function_plan_destroy(plan);
		return false;
	}
	if (status != ACCEL_COMPILE_APPLIED || plan == NULL) {
		accel_function_plan_destroy(plan);
		return accel_callback_error(
			func_block,
			N_TR("Invalid accelerator compiler result."));
	}

	region_count = accel_function_plan_get_region_count(plan);
	if (region_count == 0) {
		accel_function_plan_destroy(plan);
		return accel_callback_error(
			func_block,
			N_TR("Empty accelerator function plan."));
	}

	prepared = noct_calloc(region_count, sizeof(*prepared));
	if (prepared == NULL) {
		accel_function_plan_destroy(plan);
		hir_out_of_memory();
		return false;
	}

	/* Prepare every backend program before reserving or mutating HIR. */
	for (i = 0; i < region_count; i++) {
		program = accel_function_plan_get_region(plan, i);
		if (program == NULL) {
			accel_callback_destroy_prepared(
				context,
				prepared,
				prepared_count);
			noct_free(prepared);
			accel_function_plan_destroy(plan);
			return accel_callback_error(
				func_block,
				N_TR("Invalid accelerator region plan."));
		}

		status = context->ops.prepare_program(
			context->backend_state,
			program,
			&prepared[i]);
		if (status == ACCEL_COMPILE_APPLIED) {
			if (prepared[i].payload == NULL) {
				accel_callback_destroy_prepared(
					context,
					prepared,
					prepared_count + 1);
				noct_free(prepared);
				accel_function_plan_destroy(plan);
				return accel_callback_error(
					func_block,
					N_TR("Accelerator backend returned no program."));
			}
			prepared_count++;
			continue;
		}

		if (prepared[i].payload != NULL) {
			context->ops.destroy_prepared_program(
				context->backend_state,
				&prepared[i]);
		}

		accel_callback_destroy_prepared(
			context,
			prepared,
			prepared_count);
		noct_free(prepared);
		accel_function_plan_destroy(plan);

		if (status == ACCEL_COMPILE_DECLINED)
			return true;
		if (status == ACCEL_COMPILE_ERROR) {
			if (hir_get_error_message()[0] != '\0')
				return false;
			return accel_callback_error(
				func_block,
				N_TR("Accelerator backend failed to prepare a program."));
		}

		return accel_callback_error(
			func_block,
			N_TR("Invalid accelerator backend status."));
	}

	if (!accel_context_reserve_programs(
		context,
		region_count,
		&reservation)) {
		accel_callback_destroy_prepared(
			context,
			prepared,
			prepared_count);
		noct_free(prepared);
		accel_function_plan_destroy(plan);
		return accel_callback_error(
			func_block,
			N_TR("Failed to reserve accelerator programs."));
	}

	status = accel_lir_budget_check(
		func_block,
		plan,
		&serialized_tmpvar_size);
	if (status != ACCEL_COMPILE_APPLIED) {
		accel_context_cancel_reservation(context, reservation);
		accel_callback_destroy_prepared(
			context,
			prepared,
			prepared_count);
		noct_free(prepared);
		accel_function_plan_destroy(plan);

		if (status == ACCEL_COMPILE_DECLINED)
			return true;
		return false;
	}

	status = accel_rewrite_stage(
		func_block,
		plan,
		reservation,
		&rewrite);
	if (status != ACCEL_COMPILE_APPLIED) {
		accel_context_cancel_reservation(context, reservation);
		accel_callback_destroy_prepared(
			context,
			prepared,
			prepared_count);
		noct_free(prepared);
		accel_function_plan_destroy(plan);

		if (status == ACCEL_COMPILE_DECLINED)
			return true;
		return false;
	}

	if (!accel_context_lock_commit(context, reservation, &guard)) {
		accel_rewrite_destroy(rewrite);
		accel_context_cancel_reservation(context, reservation);
		accel_callback_destroy_prepared(
			context,
			prepared,
			prepared_count);
		noct_free(prepared);
		accel_function_plan_destroy(plan);
		return accel_callback_error(
			func_block,
			N_TR("Failed to lock accelerator rewrite commit."));
	}

	if (!accel_rewrite_add_locals(rewrite)) {
		accel_context_unlock_commit(&guard);
		accel_context_cancel_reservation(context, reservation);
		accel_rewrite_destroy(rewrite);
		accel_callback_destroy_prepared(
			context,
			prepared,
			prepared_count);
		noct_free(prepared);
		accel_function_plan_destroy(plan);
		return false;
	}

	accel_context_publish_programs_locked(&guard, prepared);
	reservation = NULL;
	prepared_count = 0;
	accel_rewrite_commit(rewrite);
	accel_context_unlock_commit(&guard);

	accel_rewrite_destroy(rewrite);
	noct_free(prepared);
	accel_function_plan_destroy(plan);

	return true;
}

/* Destroy every prepared payload still owned by the callback. */
static void
accel_callback_destroy_prepared(
	struct accel_context *context,
	struct accel_prepared_program prepared[],
	uint32_t count)
{
	uint32_t i;

	if (prepared == NULL)
		return;

	/* Release each successfully prepared region exactly once. */
	for (i = 0; i < count; i++) {
		context->ops.destroy_prepared_program(
			context->backend_state,
			&prepared[i]);
	}
}

/* Report one deterministic callback-level hard compiler failure. */
static bool
accel_callback_error(
	struct hir_block *func_block,
	const char *message)
{
	int line;

	line = 0;
	if (func_block != NULL)
		line = func_block->line;

	hir_error(line, message);

	return false;
}
