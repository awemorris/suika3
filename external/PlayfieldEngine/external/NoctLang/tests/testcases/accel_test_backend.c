/* -*- coding: utf-8; tab-width: 8; indent-tabs-mode: t; -*- */

/*
 * Noct Programming Language
 * Copyright (c) 2025, 2026, Awe Morris
 */

/*
 * Fake target-neutral accelerator backend for focused tests.
 */

#include "accel_test_backend.h"
#include "accel_context.h"

#include <stdlib.h>
#include <string.h>

struct accel_test_backend_state {
	struct accel_test_backend_observer *observer;
};

static enum accel_compile_status accel_test_prepare_program(void *backend_state, const struct accel_program *program, struct accel_prepared_program *result);
static void accel_test_destroy_prepared_program(void *backend_state, struct accel_prepared_program *program);
static bool accel_test_register_runtime(struct accel_context *context, struct rt_env *env);
static void accel_test_destroy_backend_state(void *backend_state);

/*
 * Creates fake backend state and its complete private operation table.
 */
bool
accel_test_backend_create(
	struct accel_test_backend_observer *observer,
	struct accel_backend_ops *ops,
	void **backend_state)
{
	struct accel_test_backend_state *state;

	if (observer == NULL)
		return false;
	if (ops == NULL)
		return false;
	if (backend_state == NULL)
		return false;

	*backend_state = NULL;
	memset(ops, 0, sizeof(*ops));
	memset(observer, 0, sizeof(*observer));
	observer->prepare_status = ACCEL_COMPILE_APPLIED;

	state = noct_calloc(1, sizeof(*state));
	if (state == NULL)
		return false;

	state->observer = observer;
	ops->prepare_program = accel_test_prepare_program;
	ops->destroy_prepared_program = accel_test_destroy_prepared_program;
	ops->register_runtime = accel_test_register_runtime;
	ops->destroy_backend_state = accel_test_destroy_backend_state;
	*backend_state = state;

	return true;
}

/*
 * Borrows the deep-copied program stored in one fake prepared payload.
 */
const struct accel_program *
accel_test_backend_get_program(
	const struct accel_prepared_program *prepared)
{
	if (prepared == NULL)
		return NULL;

	return prepared->payload;
}

/* Deep-copy one plan or return the configured non-applied status. */
static enum accel_compile_status
accel_test_prepare_program(
	void *backend_state,
	const struct accel_program *program,
	struct accel_prepared_program *result)
{
	struct accel_test_backend_state *state;
	struct accel_program *clone;

	if (backend_state == NULL)
		return ACCEL_COMPILE_ERROR;
	if (program == NULL)
		return ACCEL_COMPILE_ERROR;
	if (result == NULL)
		return ACCEL_COMPILE_ERROR;

	state = backend_state;
	result->payload = NULL;
	state->observer->prepare_count++;
	if (state->observer->prepare_status != ACCEL_COMPILE_APPLIED)
		return state->observer->prepare_status;

	clone = accel_program_clone(program);
	if (clone == NULL)
		return ACCEL_COMPILE_ERROR;

	result->payload = clone;

	return ACCEL_COMPILE_APPLIED;
}

/* Destroy one fake payload and clear the transferred owner field. */
static void
accel_test_destroy_prepared_program(
	void *backend_state,
	struct accel_prepared_program *program)
{
	struct accel_test_backend_state *state;

	if (backend_state == NULL)
		return;
	if (program == NULL)
		return;
	if (program->payload == NULL)
		return;

	state = backend_state;
	accel_program_destroy(program->payload);
	program->payload = NULL;
	state->observer->destroy_program_count++;
}

/* Record registration without creating a production runtime package. */
static bool
accel_test_register_runtime(
	struct accel_context *context,
	struct rt_env *env)
{
	struct accel_test_backend_state *state;

	UNUSED_PARAMETER(env);

	if (context == NULL)
		return false;

	state = accel_context_get_backend_state(context);
	if (state == NULL)
		return false;

	state->observer->register_count++;

	return true;
}

/* Destroy fake backend state while retaining caller-owned observations. */
static void
accel_test_destroy_backend_state(
	void *backend_state)
{
	struct accel_test_backend_state *state;

	if (backend_state == NULL)
		return;

	state = backend_state;
	state->observer->destroy_state_count++;
	noct_free(state);
}
