/* -*- coding: utf-8; tab-width: 8; indent-tabs-mode: t; -*- */

/*
 * Noct Programming Language
 * Copyright (C) 2025, 2026 Awe Morris
 *
 * SPDX-License-Identifier: Zlib
 */

/*
 * Backend-neutral private accelerator runtime tests.
 */

#include "accel_context.h"
#include "accel_runtime.h"
#include "runtime.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#if !defined(NOCT_TARGET_WINDOWS)
#include <pthread.h>
#endif

enum fake_mode {
	FAKE_SUCCESS,
	FAKE_DISPATCH_FAILURE,
	FAKE_FINISH_FAILURE,
	FAKE_ABANDON,
	FAKE_PUBLICATION_FAILURE,
	FAKE_DRAIN,
	FAKE_FINALIZER_DRAIN,
	FAKE_RESULT_SUCCESS,
	FAKE_RESULT_NO_PUBLICATION,
	FAKE_RESULT_PUBLICATION_FAILURE,
	FAKE_RESULT_MISSING,
	FAKE_RESULT_TYPE,
	FAKE_RESULT_IDENTITY,
	FAKE_DEVICE_SUCCESS,
	FAKE_DEVICE_ZERO,
	FAKE_DEVICE_NEGATIVE,
	FAKE_DEVICE_OVERFLOW,
	FAKE_DEVICE_ARITY
};

struct fake_drain_control {
	struct accel_mutex mutex;
	struct accel_condition condition;
	bool callback_active;
	bool destroy_started;
	bool destroy_completed;
	bool destroy_during_callback;
	bool block_execution_destroy;
	bool execution_destroy_started;
	bool release_execution_destroy;
	bool backend_destroy_during_callback;
};

struct fake_destroy_task {
	struct accel_context *context;
	struct fake_drain_control *control;
};

struct fake_finalizer_task {
	void *native_pointer;
	void (*native_finalizer)(void *native_pointer);
};

struct fake_backend {
	int mode;
	uint32_t validate_count;
	uint32_t create_count;
	uint32_t dispatch_count;
	uint32_t finish_count;
	uint32_t destroy_count;
	uint32_t backend_destroy_count;
	uint32_t buffer_count;
	uint32_t result_word_count;
	struct fake_drain_control *drain;
};

struct fake_prepared {
	struct accel_program *program;
};

struct fake_execution {
	struct fake_backend *backend;
	int32_t value[2][4];
	uint32_t result_word[2];
	uint32_t result_word_count;
};

static struct fake_backend *fake_registering_backend;

static const struct accel_executor_ops *fake_get_executor_ops(void);
static const struct accel_backend_ops *fake_get_backend_ops(void);
static enum accel_compile_status fake_prepare_program(void *backend_state, const struct accel_program *program, struct accel_prepared_program *result);
static void fake_destroy_prepared_program(void *backend_state, struct accel_prepared_program *program);
static bool fake_register_runtime(struct accel_context *context, struct rt_env *env);
static void fake_destroy_backend_state(void *backend_state);
static const struct accel_program *fake_get_program(const struct accel_prepared_program *prepared);
static bool fake_validate_dispatch_limit(void *backend_state, const struct accel_prepared_program *prepared, uint32_t kernel_index, uint32_t start, uint32_t trip, char *error, size_t error_size);
static bool fake_create_execution(void *backend_state, const struct accel_prepared_program *prepared, uint32_t scalar_word_count, const uint32_t scalar_word[], uint32_t result_word_count, const uint32_t result_word[], uint32_t buffer_count, const struct accel_runtime_buffer buffer[], void **execution, char *error, size_t error_size);
static bool fake_dispatch_execution(void *execution, uint32_t kernel_index, uint32_t start, uint32_t trip, char *error, size_t error_size);
static bool fake_finish_execution(void *execution, uint32_t result_word_count, uint32_t result_word[], uint32_t buffer_count, struct accel_runtime_buffer buffer[], char *error, size_t error_size);
static void fake_destroy_execution(void *execution);
static void fake_set_error(char *error, size_t error_size, const char *message);
static bool fake_is_device_mode(int mode);
static struct accel_program *fake_create_program(uint32_t buffer_count);
static struct accel_program *fake_create_device_program(bool overflow_extent);
static bool fake_add_results(struct accel_program *program, uint32_t buffer_count, uint32_t result_word_count, bool cpu_publication);
static bool fake_publish_program(struct accel_context *context, struct accel_program *program, uint32_t *program_id);
static bool fake_release_session(struct rt_env *env, struct rt_value *session);
static bool fake_take_session_finalizer(struct rt_env *env, struct rt_value *session, void **native_pointer, void (**native_finalizer)(void *native_pointer));
static bool fake_call_begin(struct rt_env *env, uint32_t program_id, struct rt_value *runtime_args, struct rt_value *session);
static bool fake_call_dispatch(struct rt_env *env, struct rt_value *session);
static bool fake_call_dispatch_index(struct rt_env *env, struct rt_value *session, uint32_t kernel_index);
static bool fake_call_finish(struct rt_env *env, struct rt_value *session, struct rt_value *runtime_args);
static bool fake_drain_context(struct rt_env *env, struct rt_value *session, struct accel_context *context, struct fake_backend *backend);
static bool fake_drain_finalizer(struct rt_env *env, struct rt_value *session, struct accel_context *context, struct fake_backend *backend);
static void fake_delay_for_destroy(void);
#if defined(NOCT_TARGET_WINDOWS)
static DWORD WINAPI fake_destroy_context_thread(LPVOID userdata);
static DWORD WINAPI fake_session_finalizer_thread(LPVOID userdata);
#else
static void *fake_destroy_context_thread(void *userdata);
static void *fake_session_finalizer_thread(void *userdata);
#endif
static bool fake_run_case(int mode);
static bool fake_run_device_case(int mode);

/*
 * Runs the backend-neutral runtime ownership tests.
 */
int
main(
	int argc,
	char *argv[])
{
	bool success;

	UNUSED_PARAMETER(argc);
	UNUSED_PARAMETER(argv);

	/* Runs successful and every terminal ownership path independently. */
	success = fake_run_case(FAKE_SUCCESS);
	if (success)
		success = fake_run_case(FAKE_DISPATCH_FAILURE);
	if (success)
		success = fake_run_case(FAKE_FINISH_FAILURE);
	if (success)
		success = fake_run_case(FAKE_ABANDON);
	if (success)
		success = fake_run_case(FAKE_PUBLICATION_FAILURE);
	if (success)
		success = fake_run_case(FAKE_DRAIN);
	if (success)
		success = fake_run_case(FAKE_FINALIZER_DRAIN);
	if (success)
		success = fake_run_case(FAKE_RESULT_SUCCESS);
	if (success)
		success = fake_run_case(FAKE_RESULT_NO_PUBLICATION);
	if (success)
		success = fake_run_case(FAKE_RESULT_PUBLICATION_FAILURE);
	if (success)
		success = fake_run_case(FAKE_RESULT_MISSING);
	if (success)
		success = fake_run_case(FAKE_RESULT_TYPE);
	if (success)
		success = fake_run_case(FAKE_RESULT_IDENTITY);
	if (success)
		success = fake_run_device_case(FAKE_DEVICE_SUCCESS);
	if (success)
		success = fake_run_device_case(FAKE_DEVICE_ZERO);
	if (success)
		success = fake_run_device_case(FAKE_DEVICE_NEGATIVE);
	if (success)
		success = fake_run_device_case(FAKE_DEVICE_OVERFLOW);
	if (success)
		success = fake_run_device_case(FAKE_DEVICE_ARITY);

	/* Reports the first failed runtime contract. */
	if (!success)
		return 1;

	puts("Accelerator runtime tests passed.");

	return 0;
}

/* Return the immutable fake executor operation table. */
static const struct accel_executor_ops *
fake_get_executor_ops(
	void)
{
	static const struct accel_executor_ops ops = {
		"Fake",
		fake_get_program,
		fake_validate_dispatch_limit,
		fake_create_execution,
		fake_dispatch_execution,
		fake_finish_execution,
		fake_destroy_execution
	};

	/* Return the process-lifetime executor contract. */
	return &ops;
}

/* Return the immutable fake backend operation table. */
static const struct accel_backend_ops *
fake_get_backend_ops(
	void)
{
	static const struct accel_backend_ops ops = {
		fake_prepare_program,
		fake_destroy_prepared_program,
		fake_register_runtime,
		fake_destroy_backend_state
	};

	/* Return the process-lifetime backend contract. */
	return &ops;
}

/* Decline the compiler path unused by the focused registry fixture. */
static enum accel_compile_status
fake_prepare_program(
	void *backend_state,
	const struct accel_program *program,
	struct accel_prepared_program *result)
{
	UNUSED_PARAMETER(backend_state);
	UNUSED_PARAMETER(program);
	UNUSED_PARAMETER(result);

	return ACCEL_COMPILE_DECLINED;
}

/* Destroy one fake prepared program owned by the context registry. */
static void
fake_destroy_prepared_program(
	void *backend_state,
	struct accel_prepared_program *program)
{
	struct fake_prepared *prepared;

	UNUSED_PARAMETER(backend_state);

	/* Releases a published fake payload exactly once. */
	if (program == NULL || program->payload == NULL)
		return;

	prepared = program->payload;
	accel_program_destroy(prepared->program);
	noct_free(prepared);
	program->payload = NULL;
}

/* Register the generic runtime against the current fake backend. */
static bool
fake_register_runtime(
	struct accel_context *context,
	struct rt_env *env)
{
	const struct accel_executor_ops *ops;
	bool success;

	/* Require the registering fixture before exposing private functions. */
	if (fake_registering_backend == NULL)
		return false;

	/* Register the immutable fake executor contract. */
	ops = fake_get_executor_ops();
	success = accel_runtime_register(context, env, ops);

	/* Report the runtime registration result. */
	return success;
}

/* Leave caller-owned fake backend counters intact after context cleanup. */
static void
fake_destroy_backend_state(
	void *backend_state)
{
	struct fake_backend *backend;
	struct fake_drain_control *drain;

	backend = backend_state;
	drain = backend->drain;
	if (drain != NULL) {
		accel_mutex_lock(&drain->mutex);
		if (drain->callback_active)
			drain->backend_destroy_during_callback = true;
		backend->backend_destroy_count++;
		accel_mutex_unlock(&drain->mutex);
	} else {
		backend->backend_destroy_count++;
	}
}

/* Borrow the immutable program stored in one fake prepared payload. */
static const struct accel_program *
fake_get_program(
	const struct accel_prepared_program *prepared)
{
	const struct fake_prepared *payload;

	if (prepared == NULL || prepared->payload == NULL)
		return NULL;

	payload = prepared->payload;

	return payload->program;
}

/* Validate the one deterministic four-element fake dispatch. */
static bool
fake_validate_dispatch_limit(
	void *backend_state,
	const struct accel_prepared_program *prepared,
	uint32_t kernel_index,
	uint32_t start,
	uint32_t trip,
	char *error,
	size_t error_size)
{
	struct fake_backend *backend;

	UNUSED_PARAMETER(prepared);

	backend = backend_state;
	backend->validate_count++;

	/* Accepts the exact positive device-only producer range. */
	if (fake_is_device_mode(backend->mode)) {
		if (backend->mode != FAKE_DEVICE_SUCCESS ||
		    kernel_index > 1 ||
		    start != 0 ||
		    trip != 4) {
			fake_set_error(error, error_size, "invalid fake device dispatch range");
			return false;
		}

		return true;
	}

	/* Requires the exact checked range produced by the fixture. */
	if (kernel_index != 0 ||
	    start != 0 ||
	    trip != 4) {
		fake_set_error(error, error_size, "invalid fake dispatch range");
		return false;
	}

	return true;
}

/* Snapshot one host buffer into a plain fake execution. */
static bool
fake_create_execution(
	void *backend_state,
	const struct accel_prepared_program *prepared,
	uint32_t scalar_word_count,
	const uint32_t scalar_word[],
	uint32_t result_word_count,
	const uint32_t result_word[],
	uint32_t buffer_count,
	const struct accel_runtime_buffer buffer[],
	void **execution,
	char *error,
	size_t error_size)
{
	struct fake_backend *backend;
	struct fake_execution *created;
	uint32_t i;

	UNUSED_PARAMETER(prepared);

	backend = backend_state;
	backend->create_count++;
	*execution = NULL;

	/* Verifies that device-only metadata has no host transfer representation. */
	if (fake_is_device_mode(backend->mode)) {
		if (backend->mode != FAKE_DEVICE_SUCCESS ||
		    scalar_word_count != 5 ||
		    scalar_word == NULL ||
		    scalar_word[0] != 4 ||
		    scalar_word[1] != 0 ||
		    scalar_word[2] != 4 ||
		    scalar_word[3] != 0 ||
		    scalar_word[4] != 4 ||
		    result_word_count != 0 ||
		    result_word != NULL ||
		    buffer_count != 1 ||
		    buffer == NULL) {
			fake_set_error(error, error_size, "invalid fake device metadata");
			return false;
		}
		if (buffer[0].origin != ACCEL_BUFFER_LOCAL_DEVICE ||
		    buffer[0].args_slot != ACCEL_ARGS_SLOT_NONE ||
		    buffer[0].element_kind != NOCT_PACKED_INT32 ||
		    buffer[0].element_width != 4 ||
		    buffer[0].element_count != 4 ||
		    buffer[0].byte_count != 16 ||
		    !buffer[0].active ||
		    buffer[0].upload ||
		    buffer[0].download ||
		    buffer[0].snapshot != NULL) {
			fake_set_error(error, error_size, "fake device buffer used host storage");
			return false;
		}

		/* Allocates only an opaque backend execution for the focused fixture. */
		created = noct_calloc(1, sizeof(*created));
		if (created == NULL) {
			fake_set_error(error, error_size, "out of memory");
			return false;
		}
		created->backend = backend;
		*execution = created;

		return true;
	}

	/* Verifies target-neutral scalar and dispatch words. */
	if (scalar_word_count != 2 ||
	    scalar_word == NULL ||
	    scalar_word[0] != 0 ||
	    scalar_word[1] != 4) {
		fake_set_error(error, error_size, "invalid fake scalar words");
		return false;
	}

	/* Verifies exact optional identity ownership at the private ABI boundary. */
	if (result_word_count != backend->result_word_count) {
		fake_set_error(error, error_size, "invalid fake result word count");
		return false;
	}
	if (result_word_count == 0 && result_word != NULL) {
		fake_set_error(error, error_size, "unexpected fake result words");
		return false;
	}
	if (result_word_count != 0 && result_word == NULL) {
		fake_set_error(error, error_size, "missing fake result words");
		return false;
	}

	/* Validate every initial scalar-result identity word. */
	for (i = 0; i < result_word_count; i++) {
		if (result_word[i] != 0) {
			fake_set_error(error, error_size, "invalid fake result identity");
			return false;
		}
	}

	/* Verifies the complete conservative snapshot set for this test mode. */
	if (buffer_count != backend->buffer_count || buffer == NULL) {
		fake_set_error(error, error_size, "invalid fake buffer metadata");
		return false;
	}

	/* Validate every host buffer snapshot descriptor. */
	for (i = 0; i < buffer_count; i++) {
		if (!buffer[i].active ||
		    !buffer[i].upload ||
		    !buffer[i].download ||
		    buffer[i].byte_count != sizeof(created->value[i]) ||
		    buffer[i].snapshot == NULL) {
			fake_set_error(error, error_size, "invalid fake buffer metadata");
			return false;
		}
	}

	/* Allocates and fills backend-owned execution storage synchronously. */
	created = noct_calloc(1, sizeof(*created));
	if (created == NULL) {
		fake_set_error(error, error_size, "out of memory");
		return false;
	}

	/* Initialize the execution ownership metadata. */
	created->backend = backend;
	created->result_word_count = result_word_count;

	/* Copy every initial scalar-result word. */
	for (i = 0; i < result_word_count; i++)
		created->result_word[i] = result_word[i];

	/* Snapshot every host buffer for the fake execution. */
	for (i = 0; i < buffer_count; i++) {
		memcpy(
			created->value[i],
			buffer[i].snapshot,
			sizeof(created->value[i]));
	}
	*execution = created;

	return true;
}

/* Execute or fail the one ordered fake kernel. */
static bool
fake_dispatch_execution(
	void *execution,
	uint32_t kernel_index,
	uint32_t start,
	uint32_t trip,
	char *error,
	size_t error_size)
{
	struct fake_execution *current;
	uint32_t buffer_index;
	uint32_t i;

	current = execution;
	current->backend->dispatch_count++;

	/* Accepts the exact device-only producer dispatch without host bytes. */
	if (fake_is_device_mode(current->backend->mode)) {
		if (kernel_index > 1 ||
		    start != 0 ||
		    trip != 4) {
			fake_set_error(error, error_size, "changed fake device dispatch range");
			return false;
		}

		return true;
	}

	/* Injects one terminal dispatch failure when requested. */
	if (current->backend->mode == FAKE_DISPATCH_FAILURE) {
		fake_set_error(error, error_size, "injected dispatch failure");
		return false;
	}

	/* Requires the same checked range passed to limit validation. */
	if (kernel_index != 0 ||
	    start != 0 ||
	    trip != 4) {
		fake_set_error(error, error_size, "changed fake dispatch range");
		return false;
	}

	/* Applies one deterministic in-place result to every fake buffer. */
	for (buffer_index = 0;
	     buffer_index < current->backend->buffer_count;
	     buffer_index++) {
		/* Update every element in this fake buffer. */
		for (i = 0; i < 4; i++)
			current->value[buffer_index][i] += 10;
	}

	return true;
}

/* Complete or fail the fake execution and fill its download snapshot. */
static bool
fake_finish_execution(
	void *execution,
	uint32_t result_word_count,
	uint32_t result_word[],
	uint32_t buffer_count,
	struct accel_runtime_buffer buffer[],
	char *error,
	size_t error_size)
{
	struct fake_execution *current;
	uint32_t i;

	current = execution;
	current->backend->finish_count++;

	/* Accepts completion without any host-visible device-local output. */
	if (fake_is_device_mode(current->backend->mode)) {
		if (result_word_count != 0 ||
		    result_word != NULL ||
		    buffer_count != 1 ||
		    buffer == NULL ||
		    buffer[0].origin != ACCEL_BUFFER_LOCAL_DEVICE ||
		    buffer[0].upload ||
		    buffer[0].download ||
		    buffer[0].snapshot != NULL) {
			fake_set_error(error, error_size, "changed fake device output metadata");
			return false;
		}

		return true;
	}

	/* Injects one terminal finish failure when requested. */
	if (current->backend->mode == FAKE_FINISH_FAILURE) {
		fake_set_error(error, error_size, "injected finish failure");
		return false;
	}

	/* Requires every runtime-owned output snapshot for completion. */
	if (buffer_count != current->backend->buffer_count || buffer == NULL) {
		fake_set_error(error, error_size, "missing fake output snapshot");
		return false;
	}
	if (result_word_count != current->result_word_count) {
		fake_set_error(error, error_size, "changed fake result word count");
		return false;
	}
	if (result_word_count == 0 && result_word != NULL) {
		fake_set_error(error, error_size, "unexpected fake finish result words");
		return false;
	}
	if (result_word_count != 0 && result_word == NULL) {
		fake_set_error(error, error_size, "missing fake finish result words");
		return false;
	}

	/* Validate every required output snapshot. */
	for (i = 0; i < buffer_count; i++) {
		if (!buffer[i].download || buffer[i].snapshot == NULL) {
			fake_set_error(error, error_size, "missing fake output snapshot");
			return false;
		}
	}

	/* Publishes one positive and one negative raw Int32 result word. */
	if (result_word_count != 0)
		result_word[0] = 123U;
	if (result_word_count > 1)
		result_word[1] = (uint32_t)(int32_t)-123;

	/* Copy every completed buffer into its publication snapshot. */
	for (i = 0; i < buffer_count; i++) {
		memcpy(
			buffer[i].snapshot,
			current->value[i],
			sizeof(current->value[i]));
	}

	return true;
}

/* Release one fake execution and count exact ownership transfer. */
static void
fake_destroy_execution(
	void *execution)
{
	struct fake_execution *current;
	struct fake_drain_control *drain;

	current = execution;
	if (current == NULL)
		return;

	drain = current->backend->drain;
	if (drain != NULL) {
		accel_mutex_lock(&drain->mutex);
		if (drain->callback_active)
			drain->destroy_during_callback = true;
		if (drain->block_execution_destroy) {
			drain->callback_active = true;
			drain->execution_destroy_started = true;
			accel_condition_wake_all(&drain->condition);

			/* Wait for permission to finish execution destruction. */
			while (!drain->release_execution_destroy) {
				accel_condition_wait(
					&drain->condition,
					&drain->mutex);
			}
			drain->callback_active = false;
		}
		accel_mutex_unlock(&drain->mutex);
	}

	current->backend->destroy_count++;
	noct_free(current);
}

/* Copy one deterministic callback diagnostic into caller-owned storage. */
static void
fake_set_error(
	char *error,
	size_t error_size,
	const char *message)
{
	size_t length;

	if (error == NULL || error_size == 0)
		return;

	length = strlen(message);
	if (length >= error_size)
		length = error_size - 1;
	memcpy(error, message, length);
	error[length] = '\0';
}

/* Report whether one focused case uses a device-only local buffer. */
static bool
fake_is_device_mode(
	int mode)
{
	if (mode < FAKE_DEVICE_SUCCESS)
		return false;
	if (mode > FAKE_DEVICE_ARITY)
		return false;

	return true;
}

/* Build one four-element read/write accelerator program. */
static struct accel_program *
fake_create_program(
	uint32_t buffer_count)
{
	struct accel_program *program;
	struct accel_ir_kernel *ir;
	struct accel_size_expression expression;
	struct accel_buffer_binding buffer;
	struct accel_kernel_plan kernel;
	char buffer_name[2][6];
	uint32_t start_expression;
	uint32_t trip_expression;
	uint32_t ignored;
	uint32_t buffer_index;
	uint32_t i;

	strcpy(buffer_name[0], "data0");
	strcpy(buffer_name[1], "data1");

	if (buffer_count == 0 || buffer_count > 2)
		return NULL;

	program = accel_program_create(
		"accel-runtime-test",
		"fake",
		1,
		0,
		buffer_count,
		0,
		1,
		1);
	if (program == NULL)
		return NULL;

	/* Adds constant zero and four size nodes. */
	memset(&expression, 0, sizeof(expression));
	expression.opcode = ACCEL_SIZE_CONSTANT;
	expression.left = ACCEL_PROGRAM_INDEX_NONE;
	expression.right = ACCEL_PROGRAM_INDEX_NONE;
	expression.reference = ACCEL_PROGRAM_INDEX_NONE;
	expression.value = 0;
	if (!accel_program_add_size_expression(
		program,
		&expression,
		&start_expression)) {
		accel_program_destroy(program);
		return NULL;
	}

	expression.value = 4;
	if (!accel_program_add_size_expression(
		program,
		&expression,
		&trip_expression)) {
		accel_program_destroy(program);
		return NULL;
	}

	/* Adds every host-visible read/write Int32 buffer binding. */
	for (buffer_index = 0; buffer_index < buffer_count; buffer_index++) {
		memset(&buffer, 0, sizeof(buffer));
		buffer.name = buffer_name[buffer_index];
		buffer.source_line = 1;
		buffer.element_kind = NOCT_PACKED_INT32;
		buffer.element_width = 4;
		buffer.origin = ACCEL_BUFFER_PARAMETER;
		buffer.args_slot = buffer_index;
		buffer.device_binding = buffer_index;
		buffer.required_first_expression = start_expression;
		buffer.required_end_expression = trip_expression;
		buffer.required_byte_end_expression = trip_expression;
		buffer.host_visible = true;

		/* Clear every per-kernel required-range slot. */
		for (i = 0; i < ACCEL_MAX_KERNELS; i++) {
			buffer.kernel_required_first_expression[i] =
				ACCEL_PROGRAM_INDEX_NONE;
			buffer.kernel_required_end_expression[i] =
				ACCEL_PROGRAM_INDEX_NONE;
		}
		buffer.kernel_required_first_expression[0] = start_expression;
		buffer.kernel_required_end_expression[0] = trip_expression;
		buffer.effect[0].read = true;
		buffer.effect[0].write = true;
		buffer.effect[0].read_before_write = true;
		if (!accel_program_add_buffer(program, &buffer, &ignored)) {
			accel_program_destroy(program);
			return NULL;
		}
	}

	/* Adds one typed placeholder IR kernel used only as immutable metadata. */
	ir = accel_ir_kernel_create("fake.kernel", 1, 1, 0, buffer_count);
	if (ir == NULL) {
		accel_program_destroy(program);
		return NULL;
	}

	memset(&kernel, 0, sizeof(kernel));
	kernel.source_line = 1;
	kernel.loop_block_id = 1;
	kernel.start_expression = start_expression;
	kernel.stop_expression = trip_expression;
	kernel.trip_expression = trip_expression;
	kernel.ir = ir;
	if (!accel_program_add_kernel(program, &kernel, &ignored)) {
		accel_ir_kernel_destroy(ir);
		accel_program_destroy(program);
		return NULL;
	}

	return program;
}

/* Build one validated device-only producer with a dynamic or huge extent. */
static struct accel_program *
fake_create_device_program(
	bool overflow_extent)
{
	struct accel_program *program;
	struct accel_ir_kernel *ir;
	struct accel_ir_kernel *consumer_ir;
	struct accel_ir_builder builder;
	struct accel_ir_instruction instruction;
	struct accel_size_expression expression;
	struct accel_scalar_binding scalar;
	struct accel_buffer_binding buffer;
	struct accel_kernel_plan kernel;
	char validation_error[128];
	uint32_t scalar_index;
	uint32_t zero_expression;
	uint32_t extent_expression;
	uint32_t difference_expression;
	uint32_t trip_expression;
	uint32_t byte_expression;
	uint32_t index_value;
	uint32_t stored_value;
	uint32_t loaded_value;
	uint32_t ignored;
	uint32_t i;

	/* Creates the exact generated-argument prefix for this extent form. */
	program = accel_program_create(
		"accel-runtime-test",
		"fake-device",
		1,
		0,
		overflow_extent ? 0 : 1,
		0,
		1,
		2);
	if (program == NULL)
		return NULL;

	/* Adds one typed dynamic extent when the fixture is not constant. */
	if (!overflow_extent) {
		memset(&scalar, 0, sizeof(scalar));
		scalar.name = "extent";
		scalar.args_slot = 0;
		scalar.value_type = ACCEL_IR_I32;
		if (!accel_program_add_scalar(
			program,
			&scalar,
			&scalar_index)) {
			accel_program_destroy(program);
			return NULL;
		}
		if (scalar_index != 0) {
			accel_program_destroy(program);
			return NULL;
		}
	}

	/* Adds the source loop's constant zero expression. */
	memset(&expression, 0, sizeof(expression));
	expression.opcode = ACCEL_SIZE_CONSTANT;
	expression.left = ACCEL_PROGRAM_INDEX_NONE;
	expression.right = ACCEL_PROGRAM_INDEX_NONE;
	expression.reference = ACCEL_PROGRAM_INDEX_NONE;
	expression.value = 0;
	if (!accel_program_add_size_expression(
		program,
		&expression,
		&zero_expression)) {
		accel_program_destroy(program);
		return NULL;
	}

	/* Adds either the scalar extent or one multiplication-overflowing literal. */
	if (overflow_extent) {
		expression.opcode = ACCEL_SIZE_CONSTANT;
		expression.reference = ACCEL_PROGRAM_INDEX_NONE;
		expression.value = (int64_t)(((uint64_t)-1) >> 1);
	} else {
		expression.opcode = ACCEL_SIZE_SCALAR;
		expression.reference = 0;
		expression.value = 0;
	}
	if (!accel_program_add_size_expression(
		program,
		&expression,
		&extent_expression)) {
		accel_program_destroy(program);
		return NULL;
	}

	/* Builds the canonical max-zero trip expression for the producer. */
	memset(&expression, 0, sizeof(expression));
	expression.opcode = ACCEL_SIZE_SUB;
	expression.left = extent_expression;
	expression.right = zero_expression;
	expression.reference = ACCEL_PROGRAM_INDEX_NONE;
	if (!accel_program_add_size_expression(
		program,
		&expression,
		&difference_expression)) {
		accel_program_destroy(program);
		return NULL;
	}

	memset(&expression, 0, sizeof(expression));
	expression.opcode = ACCEL_SIZE_MAX_ZERO;
	expression.left = difference_expression;
	expression.right = ACCEL_PROGRAM_INDEX_NONE;
	expression.reference = ACCEL_PROGRAM_INDEX_NONE;
	if (!accel_program_add_size_expression(
		program,
		&expression,
		&trip_expression)) {
		accel_program_destroy(program);
		return NULL;
	}

	/* Builds the constructor-equivalent checked byte-count expression. */
	memset(&expression, 0, sizeof(expression));
	expression.opcode = ACCEL_SIZE_MUL_CONSTANT;
	expression.left = extent_expression;
	expression.right = ACCEL_PROGRAM_INDEX_NONE;
	expression.reference = ACCEL_PROGRAM_INDEX_NONE;
	expression.value = 4;
	if (!accel_program_add_size_expression(
		program,
		&expression,
		&byte_expression)) {
		accel_program_destroy(program);
		return NULL;
	}

	/* Adds one nonescaping device-only Int32 binding with no argument slot. */
	memset(&buffer, 0, sizeof(buffer));
	buffer.name = "temporary";
	buffer.source_line = 1;
	buffer.element_kind = NOCT_PACKED_INT32;
	buffer.element_width = 4;
	buffer.origin = ACCEL_BUFFER_LOCAL_DEVICE;
	buffer.args_slot = ACCEL_ARGS_SLOT_NONE;
	buffer.device_binding = 0;
	buffer.required_first_expression = zero_expression;
	buffer.required_end_expression = extent_expression;
	buffer.required_byte_end_expression = byte_expression;
	buffer.extent_expression = extent_expression;

	/* Clear every per-kernel required-range slot. */
	for (i = 0; i < ACCEL_MAX_KERNELS; i++) {
		buffer.kernel_required_first_expression[i] =
			ACCEL_PROGRAM_INDEX_NONE;
		buffer.kernel_required_end_expression[i] =
			ACCEL_PROGRAM_INDEX_NONE;
	}
	buffer.kernel_required_first_expression[0] = zero_expression;
	buffer.kernel_required_end_expression[0] = extent_expression;
	buffer.effect[0].write = true;
	buffer.effect[0].full_overwrite = true;
	buffer.kernel_required_first_expression[1] = zero_expression;
	buffer.kernel_required_end_expression[1] = extent_expression;
	buffer.effect[1].read = true;
	if (!accel_program_add_buffer(program, &buffer, &ignored)) {
		accel_program_destroy(program);
		return NULL;
	}

	/* Builds one unconditional invocation-indexed store into the device local. */
	ir = accel_ir_kernel_create(
		"fake.device.kernel",
		1,
		1,
		program->scalar_count,
		program->buffer_count);
	if (ir == NULL) {
		accel_program_destroy(program);
		return NULL;
	}
	if (!accel_ir_kernel_set_buffer_type(ir, 0, ACCEL_IR_I32)) {
		accel_ir_kernel_destroy(ir);
		accel_program_destroy(program);
		return NULL;
	}
	accel_ir_builder_init(&builder, ir);

	memset(&instruction, 0, sizeof(instruction));
	instruction.opcode = ACCEL_IR_GLOBAL_INDEX;
	instruction.result_type = ACCEL_IR_INDEX_U32;
	instruction.operand[0] = ACCEL_IR_VALUE_NONE;
	instruction.operand[1] = ACCEL_IR_VALUE_NONE;
	instruction.operand[2] = ACCEL_IR_VALUE_NONE;
	instruction.reference = ACCEL_IR_REFERENCE_NONE;
	if (!accel_ir_builder_append(&builder, &instruction, &index_value)) {
		accel_ir_kernel_destroy(ir);
		accel_program_destroy(program);
		return NULL;
	}

	memset(&instruction, 0, sizeof(instruction));
	instruction.opcode = ACCEL_IR_CONST_I32;
	instruction.result_type = ACCEL_IR_I32;
	instruction.operand[0] = ACCEL_IR_VALUE_NONE;
	instruction.operand[1] = ACCEL_IR_VALUE_NONE;
	instruction.operand[2] = ACCEL_IR_VALUE_NONE;
	instruction.reference = ACCEL_IR_REFERENCE_NONE;
	instruction.literal_bits = 7;
	if (!accel_ir_builder_append(&builder, &instruction, &stored_value)) {
		accel_ir_kernel_destroy(ir);
		accel_program_destroy(program);
		return NULL;
	}

	memset(&instruction, 0, sizeof(instruction));
	instruction.opcode = ACCEL_IR_BUFFER_STORE;
	instruction.result_type = ACCEL_IR_VOID;
	instruction.operand[0] = index_value;
	instruction.operand[1] = stored_value;
	instruction.operand[2] = ACCEL_IR_VALUE_NONE;
	instruction.reference = 0;
	if (!accel_ir_builder_append(&builder, &instruction, NULL)) {
		accel_ir_kernel_destroy(ir);
		accel_program_destroy(program);
		return NULL;
	}

	/* Transfers the complete producer kernel into the immutable program. */
	memset(&kernel, 0, sizeof(kernel));
	kernel.source_line = 1;
	kernel.loop_block_id = 1;
	kernel.start_expression = zero_expression;
	kernel.stop_expression = extent_expression;
	kernel.trip_expression = trip_expression;
	kernel.ir = ir;
	if (!accel_program_add_kernel(program, &kernel, &ignored)) {
		accel_ir_kernel_destroy(ir);
		accel_program_destroy(program);
		return NULL;
	}

	/* Builds a later consumer which reads the complete private allocation. */
	consumer_ir = accel_ir_kernel_create(
		"fake.device.consumer",
		1,
		2,
		program->scalar_count,
		program->buffer_count);
	if (consumer_ir == NULL) {
		accel_program_destroy(program);
		return NULL;
	}
	if (!accel_ir_kernel_set_buffer_type(consumer_ir, 0, ACCEL_IR_I32)) {
		accel_ir_kernel_destroy(consumer_ir);
		accel_program_destroy(program);
		return NULL;
	}
	accel_ir_builder_init(&builder, consumer_ir);

	memset(&instruction, 0, sizeof(instruction));
	instruction.opcode = ACCEL_IR_GLOBAL_INDEX;
	instruction.result_type = ACCEL_IR_INDEX_U32;
	instruction.operand[0] = ACCEL_IR_VALUE_NONE;
	instruction.operand[1] = ACCEL_IR_VALUE_NONE;
	instruction.operand[2] = ACCEL_IR_VALUE_NONE;
	instruction.reference = ACCEL_IR_REFERENCE_NONE;
	if (!accel_ir_builder_append(&builder, &instruction, &index_value)) {
		accel_ir_kernel_destroy(consumer_ir);
		accel_program_destroy(program);
		return NULL;
	}

	memset(&instruction, 0, sizeof(instruction));
	instruction.opcode = ACCEL_IR_BUFFER_LOAD;
	instruction.result_type = ACCEL_IR_I32;
	instruction.operand[0] = index_value;
	instruction.operand[1] = ACCEL_IR_VALUE_NONE;
	instruction.operand[2] = ACCEL_IR_VALUE_NONE;
	instruction.reference = 0;
	if (!accel_ir_builder_append(&builder, &instruction, &loaded_value)) {
		accel_ir_kernel_destroy(consumer_ir);
		accel_program_destroy(program);
		return NULL;
	}
	if (loaded_value == ACCEL_IR_VALUE_NONE) {
		accel_ir_kernel_destroy(consumer_ir);
		accel_program_destroy(program);
		return NULL;
	}

	/* Transfers the later consumer into the same synchronous region. */
	memset(&kernel, 0, sizeof(kernel));
	kernel.source_line = 1;
	kernel.loop_block_id = 2;
	kernel.start_expression = zero_expression;
	kernel.stop_expression = extent_expression;
	kernel.trip_expression = trip_expression;
	kernel.ir = consumer_ir;
	if (!accel_program_add_kernel(program, &kernel, &ignored)) {
		accel_ir_kernel_destroy(consumer_ir);
		accel_program_destroy(program);
		return NULL;
	}

	/* Ensures the fixture itself obeys the production device-only contract. */
	validation_error[0] = '\0';
	if (!accel_program_validate(
		program,
		validation_error,
		sizeof(validation_error))) {
		fprintf(stderr, "invalid fake device program: %s\n", validation_error);
		accel_program_destroy(program);
		return NULL;
	}

	return program;
}

/* Add deterministic scalar-result descriptors to one fake program. */
static bool
fake_add_results(
	struct accel_program *program,
	uint32_t buffer_count,
	uint32_t result_word_count,
	bool cpu_publication)
{
	struct accel_scalar_result result;
	char result_name[2][8];
	uint32_t result_entry_id;
	uint32_t i;

	if (result_word_count > 2)
		return false;

	strcpy(result_name[0], "result0");
	strcpy(result_name[1], "result1");

	/* Adds dense zero-identity Int32 results after every buffer argument. */
	for (i = 0; i < result_word_count; i++) {
		memset(&result, 0, sizeof(result));
		result.name = result_name[i];
		result.result_entry_id = i;
		result.args_slot = buffer_count + i;
		result.value_type = ACCEL_IR_I32;
		result.identity_bits = 0;
		result.producer_kernel = 0;
		result.cpu_publication = cpu_publication;
		if (!accel_program_add_scalar_result(
			program,
			&result,
			&result_entry_id)) {
			return false;
		}
		if (result_entry_id != i)
			return false;
	}

	return true;
}

/* Publish one fake prepared program and return its stable ID. */
static bool
fake_publish_program(
	struct accel_context *context,
	struct accel_program *program,
	uint32_t *program_id)
{
	struct accel_registry_reservation *reservation;
	struct accel_registry_commit_guard guard;
	struct accel_prepared_program prepared[1];
	struct fake_prepared *payload;

	reservation = NULL;
	memset(&guard, 0, sizeof(guard));
	memset(prepared, 0, sizeof(prepared));

	/* Allocates the fake prepared wrapper before reserving publication. */
	payload = noct_calloc(1, sizeof(*payload));
	if (payload == NULL)
		return false;
	payload->program = program;
	prepared[0].payload = payload;

	/* Publishes the payload through the normal transactional registry API. */
	if (!accel_context_reserve_programs(context, 1, &reservation)) {
		noct_free(payload);
		return false;
	}
	*program_id = accel_registry_reservation_get_id(reservation, 0);
	if (*program_id == 0) {
		accel_context_cancel_reservation(context, reservation);
		noct_free(payload);
		return false;
	}
	if (!accel_context_lock_commit(context, reservation, &guard)) {
		accel_context_cancel_reservation(context, reservation);
		noct_free(payload);
		return false;
	}
	accel_context_publish_programs_locked(&guard, prepared);
	accel_context_unlock_commit(&guard);

	return true;
}

/* Explicitly finalize one private session dictionary in the test mutator. */
static bool
fake_release_session(
	struct rt_env *env,
	struct rt_value *session)
{
	void *native_pointer;
	void (*native_finalizer)(void *native_pointer);

	if (!fake_take_session_finalizer(
		env,
		session,
		&native_pointer,
		&native_finalizer)) {
		return false;
	}

	native_finalizer(native_pointer);

	return true;
}

/* Detach one session wrapper and return its native finalizer ownership. */
static bool
fake_take_session_finalizer(
	struct rt_env *env,
	struct rt_value *session,
	void **native_pointer,
	void (**native_finalizer)(void *native_pointer))
{
	assert(native_pointer != NULL);
	assert(native_finalizer != NULL);

	*native_pointer = NULL;
	*native_finalizer = NULL;

	if (!rt_get_dict_native_pointer(
		env,
		session,
		native_pointer,
		native_finalizer)) {
		return false;
	}
	if (*native_pointer == NULL || *native_finalizer == NULL)
		return false;
	if (!rt_set_dict_native_pointer(env, session, NULL, NULL))
		return false;

	return true;
}

/* Call private begin with one runtime argument array. */
static bool
fake_call_begin(
	struct rt_env *env,
	uint32_t program_id,
	struct rt_value *runtime_args,
	struct rt_value *session)
{
	struct rt_value argument[2];

	memset(argument, 0, sizeof(argument));
	if (!noct_make_int_long(env, &argument[0], program_id))
		return false;
	argument[1] = *runtime_args;

	return rt_call_with_name(
		env,
		"__Accel.begin",
		2,
		argument,
		session);
}

/* Call private dispatch for the fixture's only kernel. */
static bool
fake_call_dispatch(
	struct rt_env *env,
	struct rt_value *session)
{
	bool success;

	/* Dispatches the first kernel used by every original runtime fixture. */
	success = fake_call_dispatch_index(env, session, 0);

	return success;
}

/* Call private dispatch for one explicit zero-origin kernel index. */
static bool
fake_call_dispatch_index(
	struct rt_env *env,
	struct rt_value *session,
	uint32_t kernel_index)
{
	struct rt_value argument[2];

	memset(argument, 0, sizeof(argument));
	argument[0] = *session;
	if (!noct_make_int_long(env, &argument[1], kernel_index))
		return false;

	return rt_call_with_name(
		env,
		"__Accel.dispatch",
		2,
		argument,
		NULL);
}

/* Call private finish with the original runtime argument array. */
static bool
fake_call_finish(
	struct rt_env *env,
	struct rt_value *session,
	struct rt_value *runtime_args)
{
	struct rt_value argument[2];

	memset(argument, 0, sizeof(argument));
	argument[0] = *session;
	argument[1] = *runtime_args;

	return rt_call_with_name(
		env,
		"__Accel.finish",
		2,
		argument,
		NULL);
}

/* Drain one active operation before orphaning and finalizing its session. */
static bool
fake_drain_context(
	struct rt_env *env,
	struct rt_value *session,
	struct accel_context *context,
	struct fake_backend *backend)
{
	struct fake_drain_control control;
	struct fake_destroy_task task;
#if defined(NOCT_TARGET_WINDOWS)
	HANDLE thread;
	DWORD wait_result;
#else
	pthread_t thread;
	int thread_result;
#endif
	bool released;
	bool success;

	memset(&control, 0, sizeof(control));
	memset(&task, 0, sizeof(task));

	/* Initializes synchronization before exposing it to backend cleanup. */
	if (!accel_mutex_init(&control.mutex))
		return false;
	if (!accel_condition_init(&control.condition)) {
		accel_mutex_destroy(&control.mutex);
		return false;
	}

	/* Holds one simulated backend callback across asynchronous destruction. */
	backend->drain = &control;
	if (!accel_context_begin_operation(context)) {
		backend->drain = NULL;
		accel_condition_destroy(&control.condition);
		accel_mutex_destroy(&control.mutex);
		return false;
	}
	accel_mutex_lock(&control.mutex);
	control.callback_active = true;
	accel_mutex_unlock(&control.mutex);

	/* Starts context destruction after closing the operation claim gate. */
	accel_context_detach(context);
	task.context = context;
	task.control = &control;
#if defined(NOCT_TARGET_WINDOWS)
	thread = CreateThread(
		NULL,
		0,
		fake_destroy_context_thread,
		&task,
		0,
		NULL);
	if (thread == NULL) {
#else
	thread_result = pthread_create(
		&thread,
		NULL,
		fake_destroy_context_thread,
		&task);
	if (thread_result != 0) {
#endif
		accel_mutex_lock(&control.mutex);
		control.callback_active = false;
		accel_mutex_unlock(&control.mutex);
		accel_context_end_operation(context);
		accel_context_destroy(context);
		backend->drain = NULL;
		(void)fake_release_session(env, session);
		accel_condition_destroy(&control.condition);
		accel_mutex_destroy(&control.mutex);
		return false;
	}

	/* Waits until the destroy thread has entered its context call. */
	accel_mutex_lock(&control.mutex);
	while (!control.destroy_started)
		accel_condition_wait(&control.condition, &control.mutex);
	accel_mutex_unlock(&control.mutex);

	/* Verifies destruction cannot pass the outstanding callback claim. */
	fake_delay_for_destroy();
	accel_mutex_lock(&control.mutex);
	success = !control.destroy_completed;
	if (control.destroy_during_callback)
		success = false;
	control.callback_active = false;
	accel_mutex_unlock(&control.mutex);

	/* Releases the claim and waits for destruction to finish exactly once. */
	accel_context_end_operation(context);
#if defined(NOCT_TARGET_WINDOWS)
	wait_result = WaitForSingleObject(thread, INFINITE);
	if (wait_result != WAIT_OBJECT_0)
		success = false;
	if (!CloseHandle(thread))
		success = false;
#else
	thread_result = pthread_join(thread, NULL);
	if (thread_result != 0)
		success = false;
#endif

	/* Checks post-drain state before removing the backend's observer. */
	accel_mutex_lock(&control.mutex);
	if (!control.destroy_completed ||
	    control.destroy_during_callback ||
	    control.backend_destroy_during_callback ||
	    backend->backend_destroy_count != 1) {
		success = false;
	}
	accel_mutex_unlock(&control.mutex);
	backend->drain = NULL;

	/* Finalizes the detached wrapper and rejects a second execution destroy. */
	released = fake_release_session(env, session);
	if (!released)
		success = false;

	accel_condition_destroy(&control.condition);
	accel_mutex_destroy(&control.mutex);

	return success;
}

/* Drain a finalizer-owned execution before destroying backend state. */
static bool
fake_drain_finalizer(
	struct rt_env *env,
	struct rt_value *session,
	struct accel_context *context,
	struct fake_backend *backend)
{
	struct fake_drain_control control;
	struct fake_destroy_task destroy_task;
	struct fake_finalizer_task finalizer_task;
	void *native_pointer;
	void (*native_finalizer)(void *native_pointer);
#if defined(NOCT_TARGET_WINDOWS)
	HANDLE destroy_thread;
	HANDLE finalizer_thread;
	DWORD wait_result;
#else
	pthread_t destroy_thread;
	pthread_t finalizer_thread;
	int thread_result;
#endif
	bool success;

	memset(&control, 0, sizeof(control));
	memset(&destroy_task, 0, sizeof(destroy_task));
	memset(&finalizer_task, 0, sizeof(finalizer_task));
	native_pointer = NULL;
	native_finalizer = NULL;
	success = true;

	/* Initializes the backend cleanup rendezvous and takes wrapper ownership. */
	if (!accel_mutex_init(&control.mutex))
		return false;
	if (!accel_condition_init(&control.condition)) {
		accel_mutex_destroy(&control.mutex);
		return false;
	}
	backend->drain = &control;
	control.block_execution_destroy = true;
	if (!fake_take_session_finalizer(
		env,
		session,
		&native_pointer,
		&native_finalizer)) {
		backend->drain = NULL;
		accel_condition_destroy(&control.condition);
		accel_mutex_destroy(&control.mutex);
		return false;
	}

	/* Starts the session finalizer and blocks inside backend execution cleanup. */
	finalizer_task.native_pointer = native_pointer;
	finalizer_task.native_finalizer = native_finalizer;
#if defined(NOCT_TARGET_WINDOWS)
	finalizer_thread = CreateThread(
		NULL,
		0,
		fake_session_finalizer_thread,
		&finalizer_task,
		0,
		NULL);
	if (finalizer_thread == NULL) {
#else
	thread_result = pthread_create(
		&finalizer_thread,
		NULL,
		fake_session_finalizer_thread,
		&finalizer_task);
	if (thread_result != 0) {
#endif
		control.block_execution_destroy = false;
		native_finalizer(native_pointer);
		accel_context_detach(context);
		accel_context_destroy(context);
		backend->drain = NULL;
		accel_condition_destroy(&control.condition);
		accel_mutex_destroy(&control.mutex);
		return false;
	}

	/* Wait until backend execution destruction begins. */
	accel_mutex_lock(&control.mutex);
	while (!control.execution_destroy_started)
		accel_condition_wait(&control.condition, &control.mutex);
	accel_mutex_unlock(&control.mutex);

	/* Starts context destruction while finalizer cleanup owns the backend. */
	accel_context_detach(context);
	destroy_task.context = context;
	destroy_task.control = &control;
#if defined(NOCT_TARGET_WINDOWS)
	destroy_thread = CreateThread(
		NULL,
		0,
		fake_destroy_context_thread,
		&destroy_task,
		0,
		NULL);
	if (destroy_thread == NULL) {
#else
	thread_result = pthread_create(
		&destroy_thread,
		NULL,
		fake_destroy_context_thread,
		&destroy_task);
	if (thread_result != 0) {
#endif
		accel_mutex_lock(&control.mutex);
		control.release_execution_destroy = true;
		accel_condition_wake_all(&control.condition);
		accel_mutex_unlock(&control.mutex);
#if defined(NOCT_TARGET_WINDOWS)
		(void)WaitForSingleObject(finalizer_thread, INFINITE);
		(void)CloseHandle(finalizer_thread);
#else
		(void)pthread_join(finalizer_thread, NULL);
#endif
		accel_context_destroy(context);
		backend->drain = NULL;
		accel_condition_destroy(&control.condition);
		accel_mutex_destroy(&control.mutex);
		return false;
	}

	/* Confirms backend-state destruction waits for finalizer cleanup. */
	accel_mutex_lock(&control.mutex);
	while (!control.destroy_started)
		accel_condition_wait(&control.condition, &control.mutex);
	accel_mutex_unlock(&control.mutex);
	fake_delay_for_destroy();
	accel_mutex_lock(&control.mutex);
	if (control.destroy_completed ||
	    control.backend_destroy_during_callback ||
	    backend->backend_destroy_count != 0) {
		success = false;
	}
	control.release_execution_destroy = true;
	accel_condition_wake_all(&control.condition);
	accel_mutex_unlock(&control.mutex);

	/* Joins finalizer cleanup before waiting for context teardown. */
#if defined(NOCT_TARGET_WINDOWS)
	wait_result = WaitForSingleObject(finalizer_thread, INFINITE);
	if (wait_result != WAIT_OBJECT_0)
		success = false;
	if (!CloseHandle(finalizer_thread))
		success = false;
	wait_result = WaitForSingleObject(destroy_thread, INFINITE);
	if (wait_result != WAIT_OBJECT_0)
		success = false;
	if (!CloseHandle(destroy_thread))
		success = false;
#else
	thread_result = pthread_join(finalizer_thread, NULL);
	if (thread_result != 0)
		success = false;
	thread_result = pthread_join(destroy_thread, NULL);
	if (thread_result != 0)
		success = false;
#endif

	/* Verifies terminal ordering and exact backend-state destruction. */
	accel_mutex_lock(&control.mutex);
	if (!control.destroy_completed ||
	    control.backend_destroy_during_callback ||
	    control.callback_active ||
	    backend->backend_destroy_count != 1) {
		success = false;
	}
	accel_mutex_unlock(&control.mutex);
	backend->drain = NULL;
	accel_condition_destroy(&control.condition);
	accel_mutex_destroy(&control.mutex);
	if (!success) {
		fprintf(
			stderr,
			"finalizer drain failed "
			"(started=%d completed=%d active=%d overlap=%d backend-overlap=%d "
			"execution=%lu backend=%lu)\n",
			control.destroy_started ? 1 : 0,
			control.destroy_completed ? 1 : 0,
			control.callback_active ? 1 : 0,
			control.destroy_during_callback ? 1 : 0,
			control.backend_destroy_during_callback ? 1 : 0,
			(unsigned long)backend->destroy_count,
			(unsigned long)backend->backend_destroy_count);
	}

	return success;
}

/* Give the destroy thread enough portable CPU time to expose early return. */
static void
fake_delay_for_destroy(
	void)
{
	clock_t start;
	clock_t current;
	volatile unsigned long fallback;

	start = clock();
	if (start == (clock_t)-1) {
		/* Burn bounded CPU cycles when the clock is unavailable. */
		for (fallback = 0; fallback < 10000000UL; fallback++)
			;
		return;
	}

	/* Busy waiting avoids non-ANSI sleep APIs in this portable test binary. */
	do {
		current = clock();
		if (current == (clock_t)-1)
			return;
	} while (current - start < CLOCKS_PER_SEC / 20);
}

/* Destroy one context and publish completion to the drain test. */
#if defined(NOCT_TARGET_WINDOWS)
static DWORD WINAPI
fake_destroy_context_thread(
	LPVOID userdata)
#else
static void *
fake_destroy_context_thread(
	void *userdata)
#endif
{
	struct fake_destroy_task *task;
	struct fake_drain_control *control;

	task = userdata;
	control = task->control;

	/* Publishes entry before invoking the potentially blocking destructor. */
	accel_mutex_lock(&control->mutex);
	control->destroy_started = true;
	accel_condition_wake_all(&control->condition);
	accel_mutex_unlock(&control->mutex);

	accel_context_destroy(task->context);

	/* Publishes completion only after backend and context cleanup returned. */
	accel_mutex_lock(&control->mutex);
	control->destroy_completed = true;
	accel_condition_wake_all(&control->condition);
	accel_mutex_unlock(&control->mutex);

	return 0;
}

/* Invoke one detached native session finalizer on a worker thread. */
#if defined(NOCT_TARGET_WINDOWS)
static DWORD WINAPI
fake_session_finalizer_thread(
	LPVOID userdata)
#else
static void *
fake_session_finalizer_thread(
	void *userdata)
#endif
{
	struct fake_finalizer_task *task;

	task = userdata;
	task->native_finalizer(task->native_pointer);

	return 0;
}

/* Run one isolated success, failure, or finalizer ownership case. */
static bool
fake_run_case(
	int mode)
{
	struct rt_vm *vm;
	struct rt_env *env;
	struct accel_context *context;
	struct accel_program *program;
	struct fake_backend backend;
	struct rt_value runtime_args;
	struct rt_value packed;
	struct rt_value packed_second;
	struct rt_value session;
	struct rt_value scratch;
	int32_t value[4];
	int32_t value_second[4];
	uint32_t program_id;
	uint32_t buffer_count;
	uint32_t result_word_count;
	int first_result;
	int second_result;
	bool context_attached;
	bool program_published;
	bool arguments_created;
	bool session_created;
	bool begin_failure_expected;
	bool cpu_publication;
	bool pinned;
	bool success;
	bool call_success;
	uint32_t i;

	vm = NULL;
	env = NULL;
	context = NULL;
	program = NULL;
	memset(&backend, 0, sizeof(backend));
	memset(&runtime_args, 0, sizeof(runtime_args));
	memset(&packed, 0, sizeof(packed));
	memset(&packed_second, 0, sizeof(packed_second));
	memset(&session, 0, sizeof(session));
	memset(&scratch, 0, sizeof(scratch));
	backend.mode = mode;
	buffer_count = mode == FAKE_PUBLICATION_FAILURE ? 2 : 1;
	result_word_count = 0;
	if (mode == FAKE_RESULT_SUCCESS ||
	    mode == FAKE_RESULT_PUBLICATION_FAILURE) {
		result_word_count = 2;
	} else if (mode == FAKE_RESULT_NO_PUBLICATION ||
		   mode == FAKE_RESULT_MISSING ||
		   mode == FAKE_RESULT_TYPE ||
		   mode == FAKE_RESULT_IDENTITY) {
		result_word_count = 1;
	}
	backend.buffer_count = buffer_count;
	backend.result_word_count = result_word_count;
	program_id = 0;
	first_result = 0;
	second_result = 0;
	context_attached = false;
	program_published = false;
	arguments_created = false;
	session_created = false;
	begin_failure_expected = mode == FAKE_RESULT_MISSING ||
		mode == FAKE_RESULT_TYPE ||
		mode == FAKE_RESULT_IDENTITY;
	cpu_publication = mode != FAKE_RESULT_NO_PUBLICATION;
	success = false;

	/* Initialize both deterministic host buffers. */
	for (i = 0; i < 4; i++) {
		value[i] = (int32_t)i + 1;
		value_second[i] = (int32_t)i + 21;
	}

	/* Creates an isolated VM and roots every test-owned Noct value. */
	if (!noct_create_vm(&vm, &env, NULL))
		return false;
	pinned = rt_pin_local(env, &runtime_args);
	if (pinned)
		pinned = rt_pin_local(env, &packed);
	if (pinned)
		pinned = rt_pin_local(env, &packed_second);
	if (pinned)
		pinned = rt_pin_local(env, &session);
	if (pinned)
		pinned = rt_pin_local(env, &scratch);
	if (!pinned) {
		(void)noct_destroy_vm(vm);
		return false;
	}

	/* Creates, registers, and attaches one fake accelerator context. */
	fake_registering_backend = &backend;
	if (!accel_context_create(
		vm,
		fake_get_backend_ops(),
		&backend,
		&context)) {
		fake_registering_backend = NULL;
		(void)noct_destroy_vm(vm);
		return false;
	}
	if (!accel_context_register_runtime(context, env)) {
		fake_registering_backend = NULL;
		accel_context_destroy(context);
		(void)noct_destroy_vm(vm);
		return false;
	}
	fake_registering_backend = NULL;
	accel_context_attach(context);
	context_attached = true;

	/* Builds and publishes one immutable fake program. */
	program = fake_create_program(buffer_count);
	if (program != NULL && result_word_count != 0) {
		if (!fake_add_results(
			program,
			buffer_count,
			result_word_count,
			cpu_publication)) {
			accel_program_destroy(program);
			program = NULL;
		}
	}
	if (program != NULL) {
		program_published = fake_publish_program(
			context,
			program,
			&program_id);
	}

	/* Builds the one Packed argument array when publication succeeded. */
	if (program_published) {
		arguments_created = noct_make_packed(
			env,
			&packed,
			NOCT_PACKED_INT32,
			sizeof(value),
			4,
			value,
			NULL,
			NULL);
		if (arguments_created) {
			arguments_created = noct_make_empty_array(
				env,
				&runtime_args);
		}
		if (arguments_created) {
			arguments_created = noct_set_array_elem(
				env,
				&runtime_args,
				0,
				&packed);
		}
		if (arguments_created && buffer_count == 2) {
			arguments_created = noct_make_packed(
				env,
				&packed_second,
				NOCT_PACKED_INT32,
				sizeof(value_second),
				4,
				value_second,
				NULL,
				NULL);
			if (arguments_created) {
				arguments_created = noct_set_array_elem(
					env,
					&runtime_args,
					1,
					&packed_second);
			}
		}

		/* Appends rewritten scalar-result identity placeholders. */
		if (arguments_created &&
		    result_word_count != 0 &&
		    mode != FAKE_RESULT_MISSING) {
			if (mode == FAKE_RESULT_TYPE) {
				scratch = packed;
			} else {
				arguments_created = noct_make_int(
					env,
					&scratch,
					mode == FAKE_RESULT_IDENTITY ? 1 : 0);
			}
			if (arguments_created) {
				arguments_created = noct_set_array_elem(
					env,
					&runtime_args,
					buffer_count,
					&scratch);
			}
		}
		if (arguments_created && result_word_count > 1) {
			arguments_created = noct_make_int(
				env,
				&scratch,
				0);
			if (arguments_created) {
				arguments_created = noct_set_array_elem(
					env,
					&runtime_args,
					buffer_count + 1,
					&scratch);
			}
		}

		if (arguments_created) {
			session_created = fake_call_begin(
				env,
				program_id,
				&runtime_args,
				&session);
		}
	}
	if (program_published &&
	    !session_created &&
	    !begin_failure_expected) {
		fprintf(
			stderr,
			"begin error: %s\n",
			rt_get_error_message(env));
	}

	/* Runs the requested protocol path after a successful begin. */
	call_success = begin_failure_expected &&
		program_published &&
		arguments_created &&
		!session_created;
	if (!begin_failure_expected && session_created) {
		if (mode == FAKE_ABANDON ||
		    mode == FAKE_DRAIN ||
		    mode == FAKE_FINALIZER_DRAIN) {
			call_success = true;
		} else {
			call_success = fake_call_dispatch(env, &session);
			if (mode == FAKE_DISPATCH_FAILURE)
				call_success = !call_success;
			else if (call_success) {
				if (mode == FAKE_PUBLICATION_FAILURE) {
					call_success = noct_make_int_long(
						env,
						&scratch,
						0);
					if (call_success) {
						call_success = noct_set_array_elem(
							env,
							&runtime_args,
							1,
							&scratch);
					}
				} else if (mode == FAKE_RESULT_PUBLICATION_FAILURE) {
					call_success = noct_set_array_elem(
						env,
						&runtime_args,
						buffer_count + 1,
						&packed);
				}

				if (call_success) {
					call_success = fake_call_finish(
						env,
						&session,
						&runtime_args);
					if (mode == FAKE_FINISH_FAILURE ||
					    mode == FAKE_PUBLICATION_FAILURE ||
					    mode == FAKE_RESULT_PUBLICATION_FAILURE) {
						call_success = !call_success;
					}
				}
			}
		}
	}

	/* Finalizes the opaque wrapper and verifies exact backend destruction. */
	if (session_created &&
	    call_success &&
	    mode == FAKE_DRAIN) {
		call_success = fake_drain_context(
			env,
			&session,
			context,
			&backend);
		context_attached = false;
		context = NULL;
	} else if (session_created &&
		   call_success &&
		   mode == FAKE_FINALIZER_DRAIN) {
		call_success = fake_drain_finalizer(
			env,
			&session,
			context,
			&backend);
		context_attached = false;
		context = NULL;
	} else if (session_created && call_success) {
		call_success = fake_release_session(env, &session);
	}
	if (begin_failure_expected &&
	    call_success &&
	    backend.create_count == 0 &&
	    backend.destroy_count == 0) {
		success = true;
	} else if (session_created &&
		   call_success &&
		   backend.destroy_count == 1) {
		success = true;
	}

	/* Verifies successful output publication and expected callback counts. */
	if (success &&
	    (mode == FAKE_SUCCESS ||
	     mode == FAKE_RESULT_SUCCESS ||
	     mode == FAKE_RESULT_NO_PUBLICATION)) {
		/* Verify every published host word. */
		for (i = 0; i < 4; i++) {
			if (value[i] != (int32_t)i + 11)
				success = false;
		}
	}
	if (success && mode == FAKE_RESULT_SUCCESS) {
		success = noct_get_array_elem(
			env,
			&runtime_args,
			buffer_count,
			&scratch);
		if (success)
			success = noct_get_int(env, &scratch, &first_result);
		if (success) {
			success = noct_get_array_elem(
				env,
				&runtime_args,
				buffer_count + 1,
				&scratch);
		}
		if (success)
			success = noct_get_int(env, &scratch, &second_result);
		if (success &&
		    (first_result != 123 ||
		     second_result != -123))
			success = false;
	}
	if (success && mode == FAKE_RESULT_NO_PUBLICATION) {
		success = noct_get_array_elem(
			env,
			&runtime_args,
			buffer_count,
			&scratch);
		if (success)
			success = noct_get_int(env, &scratch, &first_result);
		if (success && first_result != 0)
			success = false;
	}
	if (success && mode == FAKE_PUBLICATION_FAILURE) {
		/* Verify every untouched primary-buffer word. */
		for (i = 0; i < 4; i++) {
			if (value[i] != (int32_t)i + 1)
				success = false;
		}
	}
	if (success && mode == FAKE_RESULT_PUBLICATION_FAILURE) {
		/* Verify every untouched publication-failure word. */
		for (i = 0; i < 4; i++) {
			if (value[i] != (int32_t)i + 1)
				success = false;
		}
		call_success = noct_get_array_elem(
			env,
			&runtime_args,
			buffer_count,
			&scratch);
		if (call_success)
			call_success = noct_get_int(env, &scratch, &first_result);
		if (!call_success)
			success = false;
		if (success && first_result != 0)
			success = false;
	}
	if (success &&
	    !begin_failure_expected &&
	    backend.create_count != 1)
		success = false;
	if (success &&
	    !begin_failure_expected &&
	    backend.validate_count != 1)
		success = false;
	if (success &&
	    !begin_failure_expected &&
	    mode != FAKE_ABANDON &&
	    mode != FAKE_DRAIN &&
	    mode != FAKE_FINALIZER_DRAIN &&
	    backend.dispatch_count != 1) {
		success = false;
	}
	if (success &&
	    !begin_failure_expected &&
	    (mode == FAKE_ABANDON ||
	     mode == FAKE_DRAIN ||
	     mode == FAKE_FINALIZER_DRAIN) &&
	    backend.dispatch_count != 0) {
		success = false;
	}
	if (success &&
	    !begin_failure_expected &&
	    mode != FAKE_DISPATCH_FAILURE &&
	    mode != FAKE_ABANDON &&
	    mode != FAKE_DRAIN &&
	    mode != FAKE_FINALIZER_DRAIN &&
	    backend.finish_count != 1) {
		success = false;
	}

	/* Releases any unpublished program before context-owned teardown. */
	if (!program_published)
		accel_program_destroy(program);

	/* Destroys the context before its owning VM on every completed setup. */
	if (context_attached)
		accel_context_detach(context);
	if (context != NULL)
		accel_context_destroy(context);
	if (!noct_destroy_vm(vm))
		success = false;

	if (!success) {
		fprintf(
			stderr,
			"accelerator runtime case %d failed "
			"(published=%d session=%d calls=%lu/%lu/%lu/%lu/%lu "
			"values=%ld,%ld,%ld,%ld)\n",
			mode,
			program_published ? 1 : 0,
			session_created ? 1 : 0,
			(unsigned long)backend.validate_count,
			(unsigned long)backend.create_count,
			(unsigned long)backend.dispatch_count,
			(unsigned long)backend.finish_count,
			(unsigned long)backend.destroy_count,
			(long)value[0],
			(long)value[1],
			(long)value[2],
			(long)value[3]);
	}

	return success;
}

/* Run one device-only extent, arity, or transfer-contract case. */
static bool
fake_run_device_case(
	int mode)
{
	struct rt_vm *vm;
	struct rt_env *env;
	struct accel_context *context;
	struct accel_program *program;
	struct fake_backend backend;
	struct rt_value runtime_args;
	struct rt_value session;
	struct rt_value scalar;
	uint32_t program_id;
	int extent;
	bool context_attached;
	bool program_published;
	bool arguments_created;
	bool session_created;
	bool expected_success;
	bool pinned;
	bool success;

	vm = NULL;
	env = NULL;
	context = NULL;
	program = NULL;
	memset(&backend, 0, sizeof(backend));
	memset(&runtime_args, 0, sizeof(runtime_args));
	memset(&session, 0, sizeof(session));
	memset(&scalar, 0, sizeof(scalar));
	backend.mode = mode;
	backend.buffer_count = 1;
	program_id = 0;
	context_attached = false;
	program_published = false;
	arguments_created = false;
	session_created = false;
	expected_success = mode == FAKE_DEVICE_SUCCESS;
	success = false;

	/* Creates one isolated VM and roots the generated argument values. */
	if (!noct_create_vm(&vm, &env, NULL))
		return false;
	pinned = rt_pin_local(env, &runtime_args);
	if (pinned)
		pinned = rt_pin_local(env, &session);
	if (pinned)
		pinned = rt_pin_local(env, &scalar);
	if (!pinned) {
		(void)noct_destroy_vm(vm);
		return false;
	}

	/* Registers and attaches one fake generic accelerator runtime. */
	fake_registering_backend = &backend;
	if (!accel_context_create(
		vm,
		fake_get_backend_ops(),
		&backend,
		&context)) {
		fake_registering_backend = NULL;
		(void)noct_destroy_vm(vm);
		return false;
	}
	if (!accel_context_register_runtime(context, env)) {
		fake_registering_backend = NULL;
		accel_context_destroy(context);
		(void)noct_destroy_vm(vm);
		return false;
	}
	fake_registering_backend = NULL;
	accel_context_attach(context);
	context_attached = true;

	/* Publishes either the dynamic or multiplication-overflowing fixture. */
	program = fake_create_device_program(mode == FAKE_DEVICE_OVERFLOW);
	if (program != NULL) {
		program_published = fake_publish_program(
			context,
			program,
			&program_id);
	}

	/* Builds the exact argument Array without a device-local placeholder. */
	if (program_published && noct_make_empty_array(env, &runtime_args)) {
		arguments_created = true;
		if (mode != FAKE_DEVICE_OVERFLOW) {
			extent = 4;
			if (mode == FAKE_DEVICE_ZERO)
				extent = 0;
			else if (mode == FAKE_DEVICE_NEGATIVE)
				extent = -1;

			arguments_created = noct_make_int(env, &scalar, extent);
			if (arguments_created) {
				arguments_created = noct_set_array_elem(
					env,
					&runtime_args,
					0,
					&scalar);
			}
		}
		if (arguments_created && mode == FAKE_DEVICE_ARITY) {
			arguments_created = noct_set_array_elem(
				env,
				&runtime_args,
				1,
				&scalar);
		}
	}

	/* Begins the session or observes the required pre-backend rejection. */
	if (arguments_created) {
		session_created = fake_call_begin(
			env,
			program_id,
			&runtime_args,
			&session);
	}

	/* Completes the positive device-only session through the normal protocol. */
	if (expected_success && session_created) {
		success = fake_call_dispatch_index(env, &session, 0);
		if (success)
			success = fake_call_dispatch_index(env, &session, 1);
		if (success) {
			success = fake_call_finish(
				env,
				&session,
				&runtime_args);
		}
		if (success)
			success = fake_release_session(env, &session);
	} else if (!expected_success &&
		   program_published &&
		   arguments_created &&
		   !session_created) {
		success = true;
	}

	/* Rejects backend entry on every invalid extent or argument shape. */
	if (success && expected_success) {
		if (backend.validate_count != 2 ||
		    backend.create_count != 1 ||
		    backend.dispatch_count != 2 ||
		    backend.finish_count != 1 ||
		    backend.destroy_count != 1) {
			success = false;
		}
	} else if (success) {
		if (backend.validate_count != 0 ||
		    backend.create_count != 0 ||
		    backend.dispatch_count != 0 ||
		    backend.finish_count != 0 ||
		    backend.destroy_count != 0) {
			success = false;
		}
	}

	/* Releases an unexpected session before destroying its owning context. */
	if (session_created && !expected_success)
		(void)fake_release_session(env, &session);

	/* Releases any unpublished fixture and then all VM-owned state. */
	if (!program_published)
		accel_program_destroy(program);
	if (context_attached)
		accel_context_detach(context);
	if (context != NULL)
		accel_context_destroy(context);
	if (!noct_destroy_vm(vm))
		success = false;

	/* Reports the first failed device-only contract with callback counts. */
	if (!success) {
		fprintf(
			stderr,
			"accelerator device runtime case %d failed "
			"(published=%d arguments=%d session=%d calls=%lu/%lu/%lu/%lu/%lu)\n",
			mode,
			program_published ? 1 : 0,
			arguments_created ? 1 : 0,
			session_created ? 1 : 0,
			(unsigned long)backend.validate_count,
			(unsigned long)backend.create_count,
			(unsigned long)backend.dispatch_count,
			(unsigned long)backend.finish_count,
			(unsigned long)backend.destroy_count);
	}

	return success;
}
