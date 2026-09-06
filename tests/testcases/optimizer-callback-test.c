/* -*- coding: utf-8; tab-width: 8; indent-tabs-mode: t; -*- */

/*
 * Noct Programming Language
 * Copyright (c) 2025, 2026, Awe Morris
 */

/*
 * Nullable accelerator optimizer callback test.
 */

#include <noct/noct.h>
#include "hir.h"
#include "runtime.h"

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

enum callback_action {
	CALLBACK_DECLINE,
	CALLBACK_APPLY,
	CALLBACK_ERROR
};

struct callback_state {
	enum callback_action action;
	int call_count;
	bool saw_accel;
	bool valid;
};

/* Ordinary and marked variants of the same test function. */
static const char ordinary_source[] =
	"func target(value: int): int { return value + 1; }\n";
static const char accel_source[] =
	"__accel func target(value: int): int { return value + 1; }\n";

#if defined(NOCT_USE_ACCEL)
static bool accelerator_callback(void *func_data, void *userdata);
#endif
static bool run_registration_case(const char *label, bool is_accel, int optimize_level, enum callback_action action, int expected_calls, bool expected_success);
static bool check_callback_error(NoctEnv *env);

/*
 * Checks callback gating and runtime error propagation.
 */
int
main(
	void)
{
	int optimized_calls;
	bool optimized_error_success;

#if defined(NOCT_USE_OPTIMIZER) && defined(NOCT_USE_ACCEL)
	optimized_calls = 1;
	optimized_error_success = false;
#else
	optimized_calls = 0;
	optimized_error_success = true;
#endif

	if (!run_registration_case(
		"ordinary",
		false,
		2,
		CALLBACK_DECLINE,
		0,
		true))
		return 1;
	if (!run_registration_case(
		"accelerator level zero",
		true,
		0,
		CALLBACK_DECLINE,
		0,
		true))
		return 1;
	if (!run_registration_case(
		"accelerator declined",
		true,
		1,
		CALLBACK_DECLINE,
		optimized_calls,
		true))
		return 1;
	if (!run_registration_case(
		"accelerator applied",
		true,
		2,
		CALLBACK_APPLY,
		optimized_calls,
		true))
		return 1;
	if (!run_registration_case(
		"accelerator error",
		true,
		1,
		CALLBACK_ERROR,
		optimized_calls,
		optimized_error_success))
		return 1;

	puts("Accelerator optimizer callback tests passed.");

	return 0;
}

#if defined(NOCT_USE_ACCEL)
/* Apply, decline, or fail one accelerator optimization request. */
static bool
accelerator_callback(
	void *func_data,
	void *userdata)
{
	struct hir_block *func_block;
	struct callback_state *state;

	func_block = func_data;
	state = userdata;
	state->call_count++;

	if (strcmp(func_block->val.func.name, "target") != 0) {
		state->valid = false;
		hir_error(0, "optimizer callback received the wrong function");
		return false;
	}
	if (!func_block->val.func.is_accel) {
		state->valid = false;
		hir_error(0, "optimizer callback received an unmarked function");
		return false;
	}

	state->saw_accel = true;

	/* Perform the requested callback outcome. */
	switch (state->action) {
	case CALLBACK_DECLINE:
		return true;
	case CALLBACK_APPLY:
		func_block->val.func.is_accel = false;
		return true;
	case CALLBACK_ERROR:
		hir_error(73, "accelerator callback failure");
		return false;
	default:
		state->valid = false;
		hir_error(0, "optimizer callback received an invalid action");
		return false;
	}
}
#endif

/* Register one source and verify its callback behavior. */
static bool
run_registration_case(
	const char *label,
	bool is_accel,
	int optimize_level,
	enum callback_action action,
	int expected_calls,
	bool expected_success)
{
	struct callback_state state;
	NoctConfig config;
	NoctVM *vm;
	NoctEnv *env;
	const char *source;
	const char *message;
	bool registered;
	bool ok;

	memset(&state, 0, sizeof(state));
	state.action = action;
	state.valid = true;

	noct_set_default_config(&config);
	config.jit_enable = false;
	config.optimize_level = optimize_level;

	vm = NULL;
	env = NULL;
	ok = false;
	if (!noct_create_vm(&vm, &env, &config)) {
		fprintf(stderr, "%s: failed to create VM.\n", label);
		return false;
	}

#if defined(NOCT_USE_ACCEL)
	vm->accel_optimize_func = accelerator_callback;
	vm->accel_optimize_userdata = &state;
#endif
	source = is_accel ? accel_source : ordinary_source;
	registered = noct_register_source(
		env,
		"optimizer-callback.noct",
		source);
	if (registered != expected_success) {
		message = "no runtime error";
		if (!noct_get_error_message(env, &message))
			message = "runtime error unavailable";
		fprintf(stderr, "%s: registration result mismatch: %s\n",
			label, message);
		goto cleanup;
	}

	if (state.call_count != expected_calls) {
		fprintf(stderr, "%s: callback count %d, expected %d.\n",
			label, state.call_count, expected_calls);
		goto cleanup;
	}
	if (!state.valid) {
		fprintf(stderr, "%s: callback validation failed.\n", label);
		goto cleanup;
	}
	if (expected_calls != 0 && !state.saw_accel) {
		fprintf(stderr, "%s: callback did not observe the hint.\n", label);
		goto cleanup;
	}
	if (!expected_success && !check_callback_error(env))
		goto cleanup;

	ok = true;

cleanup:
	if (!noct_destroy_vm(vm)) {
		fprintf(stderr, "%s: failed to destroy VM.\n", label);
		return false;
	}

	return ok;
}

/* Check that the callback diagnostic crossed the runtime boundary intact. */
static bool
check_callback_error(
	NoctEnv *env)
{
	const char *file;
	const char *message;
	int line;

	file = NULL;
	message = NULL;
	line = 0;
	if (!noct_get_error_file(env, &file))
		return false;
	if (!noct_get_error_line(env, &line))
		return false;
	if (!noct_get_error_message(env, &message))
		return false;

	if (strcmp(file, "optimizer-callback.noct") != 0) {
		fprintf(stderr, "Unexpected callback error file: %s\n", file);
		return false;
	}
	if (line != 73) {
		fprintf(stderr, "Unexpected callback error line: %d\n", line);
		return false;
	}
	if (strcmp(message, "accelerator callback failure") != 0) {
		fprintf(stderr, "Unexpected callback error message: %s\n", message);
		return false;
	}

	return true;
}
