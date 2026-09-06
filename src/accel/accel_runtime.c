/* -*- coding: utf-8; tab-width: 8; indent-tabs-mode: t; -*- */

/*
 * Noct Programming Language
 * Copyright (C) 2025, 2026 Awe Morris
 *
 * SPDX-License-Identifier: Zlib
 */

/*
 * Backend-neutral private accelerator runtime.
 */

#include "accel_runtime.h"
#include "accel_context.h"
#include "objectmodel.h"
#include "runtime.h"

#include <assert.h>
#include <limits.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#define ACCEL_RUNTIME_BINDING_MAGIC	0x4e414252U
#define ACCEL_RUNTIME_SESSION_MAGIC	0x4e415253U

enum accel_runtime_session_state {
	ACCEL_RUNTIME_CREATING,
	ACCEL_RUNTIME_RECORDING,
	ACCEL_RUNTIME_DISPATCHING,
	ACCEL_RUNTIME_SUBMITTING,
	ACCEL_RUNTIME_COPY_READY,
	ACCEL_RUNTIME_FINISHED,
	ACCEL_RUNTIME_FAILED,
	ACCEL_RUNTIME_ORPHANED
};

enum accel_runtime_create_status {
	ACCEL_RUNTIME_CREATE_OK,
	ACCEL_RUNTIME_CREATE_CHANGED,
	ACCEL_RUNTIME_CREATE_LIMIT,
	ACCEL_RUNTIME_CREATE_BACKEND
};

struct accel_runtime_binding {
	uint32_t magic;
	struct accel_context *context;
	struct accel_executor_ops ops;
};

struct accel_runtime_result_binding {
	uint32_t args_slot;
	bool cpu_publication;
};

struct accel_runtime_session {
	uint32_t magic;
	struct rt_vm *vm;
	struct accel_context *owner_context;
	struct accel_context *context;
	struct accel_live_session live;
	enum accel_runtime_session_state state;
	struct accel_executor_ops ops;
	const struct accel_prepared_program *prepared;
	void *execution;
	uint32_t program_id;
	uint32_t next_kernel;
	uint32_t kernel_count;
	uint32_t buffer_count;
	uint32_t scalar_word_count;
	uint32_t result_word_count;
	int64_t *scalar_value;
	uint32_t *scalar_word;
	uint32_t *result_word;
	struct accel_runtime_result_binding *result;
	uint32_t *kernel_start;
	uint32_t *kernel_trip;
	bool *kernel_active;
	struct accel_runtime_buffer *buffer;
};

static const char *accel_runtime_begin_parameter[] = {
	"programId",
	"args"
};

static const char *accel_runtime_dispatch_parameter[] = {
	"session",
	"kernelIndex"
};

static const char *accel_runtime_finish_parameter[] = {
	"session",
	"args"
};

static bool accel_runtime_ops_valid(const struct accel_executor_ops *ops);
static bool accel_runtime_publish_package(struct accel_runtime_binding *binding, struct rt_env *env, struct rt_value *package, bool *native_installed, bool *published);
static void accel_runtime_binding_finalizer(void *native_pointer);
static bool accel_runtime_begin(struct rt_env *env);
static bool accel_runtime_dispatch(struct rt_env *env);
static bool accel_runtime_finish(struct rt_env *env);
static bool accel_runtime_begin_work(struct rt_env *env, struct rt_value *program_value, struct rt_value *args, struct rt_value *element, struct rt_value *returned, struct accel_runtime_session **created_session, bool *installed);
static bool accel_runtime_dispatch_work(struct rt_env *env, struct rt_value *session_value, struct rt_value *kernel_value);
static bool accel_runtime_finish_work(struct rt_env *env, struct rt_value *session_value, struct rt_value *args, struct rt_value *element);
static bool accel_runtime_begin_claimed(struct rt_env *env, struct accel_runtime_binding *binding, uint32_t program_id, struct rt_value *args, struct rt_value *element, struct rt_value *returned, struct accel_runtime_session **created_session, bool *installed);
static bool accel_runtime_dispatch_claimed(struct rt_env *env, struct accel_runtime_binding *binding, struct accel_runtime_session *session, uint32_t kernel_index);
static bool accel_runtime_finish_claimed(struct rt_env *env, struct accel_runtime_binding *binding, struct accel_runtime_session *session, struct rt_value *args, struct rt_value *element);
static bool accel_runtime_current_binding(struct rt_env *env, struct accel_runtime_binding **result);
static bool accel_runtime_lookup_program(struct rt_env *env, struct accel_runtime_binding *binding, uint32_t program_id, const struct accel_prepared_program **prepared, const struct accel_program **program);
static bool accel_runtime_allocate_metadata(struct rt_env *env, const struct accel_program *program, struct accel_runtime_session *session);
static bool accel_runtime_read_metadata(struct rt_env *env, struct rt_value *args, struct rt_value *element, const struct accel_program *program, struct accel_runtime_session *session);
static bool accel_runtime_validate_argument_count(struct rt_env *env, size_t args_size, const struct accel_program *program);
static bool accel_runtime_read_scalars(struct rt_env *env, struct rt_value *args, struct rt_value *element, size_t args_size, const struct accel_program *program, struct accel_runtime_session *session);
static bool accel_runtime_read_results(struct rt_env *env, struct rt_value *args, struct rt_value *element, size_t args_size, const struct accel_program *program, struct accel_runtime_session *session);
static bool accel_runtime_read_device_extents(struct rt_env *env, const struct accel_program *program, struct accel_runtime_session *session);
static bool accel_runtime_evaluate_kernels(struct rt_env *env, const struct accel_program *program, struct accel_runtime_session *session);
static bool accel_runtime_read_buffers(struct rt_env *env, struct rt_value *args, struct rt_value *element, size_t args_size, const struct accel_program *program, struct accel_runtime_session *session);
static bool accel_runtime_plan_buffer(struct rt_env *env, const struct accel_program *program, struct accel_runtime_session *session, uint32_t buffer_index);
static enum accel_runtime_create_status accel_runtime_create_and_link(struct rt_env *env, struct accel_runtime_binding *binding, const struct accel_prepared_program *prepared, struct accel_runtime_session *session, char *error, size_t error_size);
static bool accel_runtime_install_session(struct rt_env *env, struct accel_runtime_session *session, struct rt_value *returned, bool *installed);
static bool accel_runtime_get_session_argument(struct rt_env *env, struct rt_value *value, struct accel_runtime_session **session);
static bool accel_runtime_copy_results(struct rt_env *env, struct rt_value *args, struct rt_value *element, struct accel_runtime_session *session);
static void accel_runtime_backend_error(struct rt_env *env, const char *backend_name, const char *operation, const char *error);
static bool accel_runtime_pin_values(struct rt_env *env, struct rt_value *value[], uint32_t value_count, uint32_t *pinned_count);
static bool accel_runtime_unpin_values(struct rt_env *env, struct rt_value *value[], uint32_t *pinned_count);
static void accel_runtime_discard_session(struct rt_env *env, struct accel_runtime_session *session);
static void *accel_runtime_fail_session_locked(struct accel_runtime_session *session);
static void *accel_runtime_session_orphan_locked(struct accel_live_session *live);
static void accel_runtime_session_finalizer(void *native_pointer);
static void accel_runtime_destroy_session(struct accel_runtime_session *session);

/*
 * Registers the backend-neutral private accelerator package.
 *
 * The operation table and context association live in the private package's
 * native metadata.  This keeps the core context ABI independent of executor
 * details and gives the metadata the same lifetime as the owning VM.
 */
bool
accel_runtime_register(
	struct accel_context *context,
	struct rt_env *env,
	const struct accel_executor_ops *ops)
{
	struct accel_runtime_binding *binding;
	struct rt_value package;
	bool native_installed;
	bool published;
	bool pinned;
	bool success;

	/* Rejects incomplete runtime owners. */
	if (context == NULL || env == NULL)
		return false;

	/* Rejects an incomplete executor contract. */
	if (!accel_runtime_ops_valid(ops))
		return false;

	/* Allocates VM-owned private package metadata. */
	binding = noct_calloc(1, sizeof(*binding));
	if (binding == NULL) {
		rt_out_of_memory(env);
		return false;
	}
	if (!accel_context_retain(context)) {
		noct_free(binding);
		rt_error(env, N_TR("Accelerator context reference limit reached."));
		return false;
	}

	binding->magic = ACCEL_RUNTIME_BINDING_MAGIC;
	binding->context = context;
	binding->ops = *ops;

	/* Roots the package while its functions and native metadata are built. */
	memset(&package, 0, sizeof(package));
	native_installed = false;
	published = false;
	pinned = rt_pin_local(env, &package);
	if (!pinned) {
		accel_runtime_binding_finalizer(binding);
		return false;
	}

	/* Publishes the immutable private package. */
	success = accel_runtime_publish_package(
		binding,
		env,
		&package,
		&native_installed,
		&published);

	/* Reclaims metadata when publication failed before acquiring a VM root. */
	if (!published && native_installed) {
		if (rt_set_dict_native_pointer(env, &package, NULL, NULL)) {
			native_installed = false;
			accel_runtime_binding_finalizer(binding);
		}
	} else if (!published && !native_installed) {
		accel_runtime_binding_finalizer(binding);
	}

	/* Releases the construction root on every exit. */
	if (!rt_unpin_local(env, &package))
		success = false;

	return success;
}

/* Validate a complete backend executor operation table. */
static bool
accel_runtime_ops_valid(
	const struct accel_executor_ops *ops)
{
	/* Requires the operation-table wrapper. */
	if (ops == NULL)
		return false;

	/* Requires a stable backend name for runtime diagnostics. */
	if (ops->backend_display_name == NULL)
		return false;

	/* Rejects an empty backend name. */
	if (ops->backend_display_name[0] == '\0')
		return false;

	/* Requires prepared-program metadata access. */
	if (ops->get_program == NULL)
		return false;

	/* Requires synchronous execution creation. */
	if (ops->create_execution == NULL)
		return false;

	/* Requires ordered kernel dispatch. */
	if (ops->dispatch_execution == NULL)
		return false;

	/* Requires synchronous execution completion. */
	if (ops->finish_execution == NULL)
		return false;

	/* Requires exact backend execution cleanup. */
	if (ops->destroy_execution == NULL)
		return false;

	return true;
}

/* Build and publish the immutable private runtime package. */
static bool
accel_runtime_publish_package(
	struct accel_runtime_binding *binding,
	struct rt_env *env,
	struct rt_value *package,
	bool *native_installed,
	bool *published)
{
	struct rt_func *begin;
	struct rt_func *dispatch;
	struct rt_func *finish;
	struct rt_value value;

	assert(binding != NULL);
	assert(env != NULL);
	assert(package != NULL);
	assert(native_installed != NULL);
	assert(published != NULL);

	/* Creates the package and transfers native metadata ownership to it. */
	if (!rt_make_empty_dict(env, package))
		return false;
	if (!rt_set_dict_native_pointer(
		env,
		package,
		binding,
		accel_runtime_binding_finalizer)) {
		return false;
	}
	*native_installed = true;

	/* Registers the three private protocol functions. */
	if (!rt_register_cfunc(
		env,
		"__Accel.begin",
		2,
		accel_runtime_begin_parameter,
		accel_runtime_begin,
		&begin)) {
		return false;
	}
	if (!rt_register_cfunc(
		env,
		"__Accel.dispatch",
		2,
		accel_runtime_dispatch_parameter,
		accel_runtime_dispatch,
		&dispatch)) {
		return false;
	}
	if (!rt_register_cfunc(
		env,
		"__Accel.finish",
		2,
		accel_runtime_finish_parameter,
		accel_runtime_finish,
		&finish)) {
		return false;
	}

	/* Adds the private functions in source-visible call order. */
	memset(&value, 0, sizeof(value));
	value.type = NOCT_VALUE_FUNC;
	value.val.func = begin;
	if (!rt_set_dict_elem_cstr(env, package, "begin", &value))
		return false;

	value.val.func = dispatch;
	if (!rt_set_dict_elem_cstr(env, package, "dispatch", &value))
		return false;

	value.val.func = finish;
	if (!rt_set_dict_elem_cstr(env, package, "finish", &value))
		return false;

	/* Freezes and roots the complete private package. */
	if (!om_freeze_dict(env, package))
		return false;
	if (!rt_set_global(env, "__Accel", package))
		return false;
	*published = true;

	if (!rt_mark_global_const(env, "__Accel"))
		return false;

	return true;
}

/* Release private package metadata after the owning dictionary dies. */
static void
accel_runtime_binding_finalizer(
	void *native_pointer)
{
	struct accel_runtime_binding *binding;
	struct accel_context *context;

	binding = native_pointer;
	if (binding == NULL)
		return;

	context = binding->context;
	binding->magic = 0;
	binding->context = NULL;
	memset(&binding->ops, 0, sizeof(binding->ops));
	noct_free(binding);

	if (context != NULL)
		accel_context_release(context);
}

/* Create and return one backend-neutral recording session. */
static bool
accel_runtime_begin(
	struct rt_env *env)
{
	struct accel_runtime_session *session;
	struct rt_value program_value;
	struct rt_value args;
	struct rt_value element;
	struct rt_value returned;
	struct rt_value *pinned_value[4];
	uint32_t pinned_count;
	bool installed;
	bool success;

	/* Initializes every GC-visible local before pinning it. */
	memset(&program_value, 0, sizeof(program_value));
	memset(&args, 0, sizeof(args));
	memset(&element, 0, sizeof(element));
	memset(&returned, 0, sizeof(returned));
	pinned_value[0] = &program_value;
	pinned_value[1] = &args;
	pinned_value[2] = &element;
	pinned_value[3] = &returned;
	pinned_count = 0;
	session = NULL;
	installed = false;

	/* Pins all values used across argument and result helper calls. */
	if (!accel_runtime_pin_values(
		env,
		pinned_value,
		4,
		&pinned_count)) {
		return false;
	}

	/* Builds and installs the recording session. */
	success = accel_runtime_begin_work(
		env,
		&program_value,
		&args,
		&element,
		&returned,
		&session,
		&installed);

	/* Reclaims sessions that were not transferred to a native dictionary. */
	if (!installed && session != NULL)
		accel_runtime_discard_session(env, session);

	/* Releases all temporary GC roots in reverse order. */
	if (!accel_runtime_unpin_values(
		env,
		pinned_value,
		&pinned_count)) {
		success = false;
	}

	return success;
}

/* Record one ordered backend-neutral kernel dispatch. */
static bool
accel_runtime_dispatch(
	struct rt_env *env)
{
	struct rt_value session_value;
	struct rt_value kernel_value;
	struct rt_value *pinned_value[2];
	uint32_t pinned_count;
	bool success;

	/* Initializes every GC-visible local before pinning it. */
	memset(&session_value, 0, sizeof(session_value));
	memset(&kernel_value, 0, sizeof(kernel_value));
	pinned_value[0] = &session_value;
	pinned_value[1] = &kernel_value;
	pinned_count = 0;

	/* Pins both arguments across native dictionary inspection. */
	if (!accel_runtime_pin_values(
		env,
		pinned_value,
		2,
		&pinned_count)) {
		return false;
	}

	/* Validates and records the requested dispatch. */
	success = accel_runtime_dispatch_work(
		env,
		&session_value,
		&kernel_value);

	/* Releases both argument roots in reverse order. */
	if (!accel_runtime_unpin_values(
		env,
		pinned_value,
		&pinned_count)) {
		success = false;
	}

	return success;
}

/* Finish one session and synchronously publish host-visible results. */
static bool
accel_runtime_finish(
	struct rt_env *env)
{
	struct rt_value session_value;
	struct rt_value args;
	struct rt_value element;
	struct rt_value *pinned_value[3];
	uint32_t pinned_count;
	bool success;

	/* Initializes every GC-visible local before pinning it. */
	memset(&session_value, 0, sizeof(session_value));
	memset(&args, 0, sizeof(args));
	memset(&element, 0, sizeof(element));
	pinned_value[0] = &session_value;
	pinned_value[1] = &args;
	pinned_value[2] = &element;
	pinned_count = 0;

	/* Pins every value used during result revalidation and publication. */
	if (!accel_runtime_pin_values(
		env,
		pinned_value,
		3,
		&pinned_count)) {
		return false;
	}

	/* Completes the backend execution and copies its plain snapshots. */
	success = accel_runtime_finish_work(
		env,
		&session_value,
		&args,
		&element);

	/* Releases every argument root in reverse order. */
	if (!accel_runtime_unpin_values(
		env,
		pinned_value,
		&pinned_count)) {
		success = false;
	}

	return success;
}

/* Validate arguments, snapshot inputs, and create one execution. */
static bool
accel_runtime_begin_work(
	struct rt_env *env,
	struct rt_value *program_value,
	struct rt_value *args,
	struct rt_value *element,
	struct rt_value *returned,
	struct accel_runtime_session **created_session,
	bool *installed)
{
	struct accel_runtime_binding *binding;
	size_t program_id;
	bool success;

	assert(created_session != NULL);
	assert(installed != NULL);
	assert(program_value != NULL);
	assert(args != NULL);
	assert(element != NULL);
	assert(returned != NULL);

	/* Reads and validates the private protocol arguments. */
	if (!noct_get_arg_check_int_long(env, 0, program_value, &program_id))
		return false;
	if (program_id == 0 || program_id > UINT32_MAX) {
		rt_error(env, N_TR("Invalid accelerator program ID."));
		return false;
	}
	if (!noct_get_arg_check_array(env, 1, args))
		return false;

	/* Recovers this VM's immutable private executor binding. */
	if (!accel_runtime_current_binding(env, &binding))
		return false;

	/* Claims every context and backend pointer used by this begin operation. */
	if (!accel_context_begin_operation(binding->context)) {
		rt_error(env, N_TR("Detached accelerator context."));
		return false;
	}

	success = accel_runtime_begin_claimed(
		env,
		binding,
		(uint32_t)program_id,
		args,
		element,
		returned,
		created_session,
		installed);

	/* Reclaims uninstalled sessions before releasing the context lifetime. */
	if (!success &&
	    !*installed &&
	    *created_session != NULL) {
		accel_runtime_discard_session(env, *created_session);
		*created_session = NULL;
	}

	accel_context_end_operation(binding->context);

	return success;
}

/* Create one execution while holding an external context lifetime claim. */
static bool
accel_runtime_begin_claimed(
	struct rt_env *env,
	struct accel_runtime_binding *binding,
	uint32_t program_id,
	struct rt_value *args,
	struct rt_value *element,
	struct rt_value *returned,
	struct accel_runtime_session **created_session,
	bool *installed)
{
	const struct accel_prepared_program *prepared;
	const struct accel_program *program;
	struct accel_runtime_session *session;
	char error[ACCEL_RUNTIME_ERROR_SIZE];
	enum accel_runtime_create_status create_status;

	assert(env != NULL);
	assert(binding != NULL);
	assert(binding->context != NULL);
	assert(args != NULL);
	assert(element != NULL);
	assert(returned != NULL);
	assert(created_session != NULL);
	assert(installed != NULL);

	/* Borrows the published program while the context remains attached. */
	if (!accel_runtime_lookup_program(
		env,
		binding,
		program_id,
		&prepared,
		&program)) {
		return false;
	}

	/* Allocates the plain session wrapper and its fixed metadata. */
	session = noct_calloc(1, sizeof(*session));
	if (session == NULL) {
		rt_out_of_memory(env);
		return false;
	}
	if (!accel_context_retain(binding->context)) {
		noct_free(session);
		rt_error(env, N_TR("Accelerator context reference limit reached."));
		return false;
	}
	*created_session = session;

	session->magic = ACCEL_RUNTIME_SESSION_MAGIC;
	session->vm = env->vm;
	session->owner_context = binding->context;
	session->context = binding->context;
	session->live.orphan_locked = accel_runtime_session_orphan_locked;
	session->live.destroy_orphan = binding->ops.destroy_execution;
	session->state = ACCEL_RUNTIME_CREATING;
	session->ops = binding->ops;
	session->prepared = prepared;
	session->program_id = program_id;
	session->kernel_count = program->kernel_count;
	session->buffer_count = program->buffer_count;

	/* Converts all Noct inputs into runtime-owned plain memory. */
	if (!accel_runtime_allocate_metadata(env, program, session))
		return false;
	if (!accel_runtime_read_metadata(
		env,
		args,
		element,
		program,
		session)) {
		return false;
	}

	/* Creates backend resources and links their exact lifetime to the context. */
	error[0] = '\0';
	create_status = accel_runtime_create_and_link(
		env,
		binding,
		prepared,
		session,
		error,
		sizeof(error));
	if (create_status == ACCEL_RUNTIME_CREATE_CHANGED) {
		rt_error(
			env,
			N_TR("%s accelerator program changed during begin."),
			binding->ops.backend_display_name);
		return false;
	}
	if (create_status == ACCEL_RUNTIME_CREATE_LIMIT) {
		accel_runtime_backend_error(
			env,
			binding->ops.backend_display_name,
			N_TR("rejected a dispatch range"),
			error);
		return false;
	}
	if (create_status == ACCEL_RUNTIME_CREATE_BACKEND) {
		accel_runtime_backend_error(
			env,
			binding->ops.backend_display_name,
			N_TR("failed to create an execution"),
			error);
		return false;
	}

	/* Transfers session ownership to the returned private dictionary. */
	if (!accel_runtime_install_session(
		env,
		session,
		returned,
		installed)) {
		return false;
	}

	return true;
}

/* Validate and submit one dispatch while protecting the live execution. */
static bool
accel_runtime_dispatch_work(
	struct rt_env *env,
	struct rt_value *session_value,
	struct rt_value *kernel_value)
{
	struct accel_runtime_binding *binding;
	struct accel_runtime_session *session;
	size_t kernel_index;
	bool success;

	/* Retrieves both protocol arguments without retaining GC object pointers. */
	if (!accel_runtime_get_session_argument(env, session_value, &session))
		return false;
	if (!noct_get_arg_check_int_long(env, 1, kernel_value, &kernel_index))
		return false;
	if (kernel_index > UINT32_MAX) {
		rt_error(env, N_TR("Invalid accelerator kernel index."));
		return false;
	}

	/* Recovers the still-attached context for this private session. */
	if (!accel_runtime_current_binding(env, &binding))
		return false;

	/* Claims context, session, and backend lifetime across the dispatch. */
	if (!accel_context_begin_operation(binding->context)) {
		rt_error(env, N_TR("Detached accelerator context."));
		return false;
	}

	success = accel_runtime_dispatch_claimed(
		env,
		binding,
		session,
		(uint32_t)kernel_index);
	accel_context_end_operation(binding->context);

	return success;
}

/* Submit one dispatch while holding an external context lifetime claim. */
static bool
accel_runtime_dispatch_claimed(
	struct rt_env *env,
	struct accel_runtime_binding *binding,
	struct accel_runtime_session *session,
	uint32_t kernel_index)
{
	void *execution;
	void *failed_execution;
	char error[ACCEL_RUNTIME_ERROR_SIZE];
	uint32_t start;
	uint32_t trip;
	bool active;
	bool backend_success;

	assert(env != NULL);
	assert(binding != NULL);
	assert(binding->context != NULL);
	assert(session != NULL);

	/* Claims this ordered dispatch under the short-lived state mutex. */
	error[0] = '\0';
	noct_enter_blocking(env);
	accel_context_state_lock(binding->context);

	if (session->magic != ACCEL_RUNTIME_SESSION_MAGIC ||
	    session->vm != env->vm ||
	    session->context != binding->context ||
	    !session->live.linked ||
	    session->state != ACCEL_RUNTIME_RECORDING ||
	    session->execution == NULL ||
	    session->next_kernel != kernel_index ||
	    kernel_index >= session->kernel_count) {
		accel_context_state_unlock(binding->context);
		noct_leave_blocking(env);
		rt_error(
			env,
			N_TR("Invalid or out-of-order %s accelerator dispatch."),
			binding->ops.backend_display_name);
		return false;
	}

	session->state = ACCEL_RUNTIME_DISPATCHING;
	execution = session->execution;
	start = session->kernel_start[kernel_index];
	trip = session->kernel_trip[kernel_index];
	active = session->kernel_active[kernel_index];
	accel_context_state_unlock(binding->context);

	/* Performs backend serialization without holding the context state mutex. */
	backend_success = true;
	if (active) {
		backend_success = session->ops.dispatch_execution(
			execution,
			kernel_index,
			start,
			trip,
			error,
			sizeof(error));
		error[sizeof(error) - 1] = '\0';
	}

	/* Reacquires state ownership and verifies the claimed session. */
	accel_context_state_lock(binding->context);
	if (session->magic != ACCEL_RUNTIME_SESSION_MAGIC ||
	    session->vm != env->vm ||
	    session->context != binding->context ||
	    !session->live.linked ||
	    session->state != ACCEL_RUNTIME_DISPATCHING ||
	    session->next_kernel != kernel_index ||
	    session->execution != execution) {
		accel_context_state_unlock(binding->context);
		noct_leave_blocking(env);
		rt_error(
			env,
			N_TR("%s accelerator session changed during dispatch."),
			binding->ops.backend_display_name);
		return false;
	}

	/* Claims terminal ownership after the first backend dispatch failure. */
	if (!backend_success) {
		failed_execution = accel_runtime_fail_session_locked(session);
		accel_context_state_unlock(binding->context);
		if (failed_execution != NULL)
			session->ops.destroy_execution(failed_execution);
		noct_leave_blocking(env);
		accel_runtime_backend_error(
			env,
			binding->ops.backend_display_name,
			N_TR("failed to dispatch a kernel"),
			error);
		return false;
	}

	session->next_kernel++;
	session->state = ACCEL_RUNTIME_RECORDING;
	accel_context_state_unlock(binding->context);
	noct_leave_blocking(env);

	return true;
}

/* Finish an execution, detach it, and copy plain output snapshots. */
static bool
accel_runtime_finish_work(
	struct rt_env *env,
	struct rt_value *session_value,
	struct rt_value *args,
	struct rt_value *element)
{
	struct accel_runtime_binding *binding;
	struct accel_runtime_session *session;
	bool success;

	/* Retrieves and validates the two protocol arguments. */
	if (!accel_runtime_get_session_argument(env, session_value, &session))
		return false;
	if (!noct_get_arg_check_array(env, 1, args))
		return false;

	/* Recovers the still-attached context for this private session. */
	if (!accel_runtime_current_binding(env, &binding))
		return false;

	/* Claims context, session, and backend lifetime through result publication. */
	if (!accel_context_begin_operation(binding->context)) {
		rt_error(env, N_TR("Detached accelerator context."));
		return false;
	}

	success = accel_runtime_finish_claimed(
		env,
		binding,
		session,
		args,
		element);
	accel_context_end_operation(binding->context);

	return success;
}

/* Finish one execution while holding an external context lifetime claim. */
static bool
accel_runtime_finish_claimed(
	struct rt_env *env,
	struct accel_runtime_binding *binding,
	struct accel_runtime_session *session,
	struct rt_value *args,
	struct rt_value *element)
{
	void *execution;
	void *finished_execution;
	char error[ACCEL_RUNTIME_ERROR_SIZE];
	bool backend_success;

	assert(env != NULL);
	assert(binding != NULL);
	assert(binding->context != NULL);
	assert(session != NULL);
	assert(args != NULL);
	assert(element != NULL);

	/* Claims this finish under the short-lived context state mutex. */
	error[0] = '\0';
	noct_enter_blocking(env);
	accel_context_state_lock(binding->context);

	if (session->magic != ACCEL_RUNTIME_SESSION_MAGIC ||
	    session->vm != env->vm ||
	    session->context != binding->context ||
	    !session->live.linked ||
	    session->state != ACCEL_RUNTIME_RECORDING ||
	    session->execution == NULL ||
	    session->next_kernel != session->kernel_count) {
		accel_context_state_unlock(binding->context);
		noct_leave_blocking(env);
		rt_error(
			env,
			N_TR("Invalid or incomplete %s accelerator finish."),
			binding->ops.backend_display_name);
		return false;
	}

	session->state = ACCEL_RUNTIME_SUBMITTING;
	execution = session->execution;
	accel_context_state_unlock(binding->context);

	/* Submits and waits without holding the context state mutex. */
	backend_success = session->ops.finish_execution(
		execution,
		session->result_word_count,
		session->result_word,
		session->buffer_count,
		session->buffer,
		error,
		sizeof(error));
	error[sizeof(error) - 1] = '\0';

	/* Reacquires state ownership and verifies the claimed session. */
	accel_context_state_lock(binding->context);
	if (session->magic != ACCEL_RUNTIME_SESSION_MAGIC ||
	    session->vm != env->vm ||
	    session->context != binding->context ||
	    !session->live.linked ||
	    session->state != ACCEL_RUNTIME_SUBMITTING ||
	    session->execution != execution) {
		accel_context_state_unlock(binding->context);
		noct_leave_blocking(env);
		rt_error(
			env,
			N_TR("%s accelerator session changed during finish."),
			binding->ops.backend_display_name);
		return false;
	}

	/* Claims terminal ownership after a backend completion failure. */
	if (!backend_success) {
		finished_execution = accel_runtime_fail_session_locked(session);
		accel_context_state_unlock(binding->context);
		if (finished_execution != NULL)
			session->ops.destroy_execution(finished_execution);
		noct_leave_blocking(env);
		accel_runtime_backend_error(
			env,
			binding->ops.backend_display_name,
			N_TR("failed to finish an execution"),
			error);
		return false;
	}

	/* Makes completed plain snapshots available while retaining live ownership. */
	session->state = ACCEL_RUNTIME_COPY_READY;
	accel_context_state_unlock(binding->context);
	noct_leave_blocking(env);

	/* Revalidates output objects and copies only from runtime-owned snapshots. */
	if (!accel_runtime_copy_results(env, args, element, session)) {
		noct_enter_blocking(env);
		accel_context_state_lock(binding->context);
		if (session->context == binding->context &&
		    session->live.linked &&
		    session->state == ACCEL_RUNTIME_COPY_READY &&
		    session->execution == execution) {
			finished_execution = accel_runtime_fail_session_locked(session);
		} else {
			finished_execution = NULL;
		}
		accel_context_state_unlock(binding->context);
		if (finished_execution != NULL)
			session->ops.destroy_execution(finished_execution);
		noct_leave_blocking(env);
		return false;
	}

	/* Claims and closes the completed session after host publication. */
	noct_enter_blocking(env);
	accel_context_state_lock(binding->context);
	if (session->magic != ACCEL_RUNTIME_SESSION_MAGIC ||
	    session->vm != env->vm ||
	    session->context != binding->context ||
	    !session->live.linked ||
	    session->state != ACCEL_RUNTIME_COPY_READY ||
	    session->execution != execution) {
		accel_context_state_unlock(binding->context);
		noct_leave_blocking(env);
		rt_error(
			env,
			N_TR("%s accelerator session changed while publishing results."),
			binding->ops.backend_display_name);
		return false;
	}

	accel_context_unlink_session_locked(binding->context, &session->live);
	finished_execution = session->execution;
	session->execution = NULL;
	session->prepared = NULL;
	session->state = ACCEL_RUNTIME_FINISHED;
	session->context = NULL;
	accel_context_state_unlock(binding->context);

	/* Releases drained backend resources without holding the state mutex. */
	if (finished_execution != NULL)
		session->ops.destroy_execution(finished_execution);
	noct_leave_blocking(env);

	return true;
}

/* Recover and validate the private package binding for the current VM. */
static bool
accel_runtime_current_binding(
	struct rt_env *env,
	struct accel_runtime_binding **result)
{
	struct accel_runtime_binding *binding;
	struct rt_value package;
	void *native_pointer;
	void (*native_finalizer)(void *native_pointer);

	assert(result != NULL);

	*result = NULL;

	/* Rejects calls without an active accelerator context. */
	if (env == NULL ||
	    env->vm == NULL ||
	    env->vm->accel_optimize_userdata == NULL) {
		rt_error(env, N_TR("Accelerator context is unavailable."));
		return false;
	}

	/* Borrows private native metadata from the VM-rooted package dictionary. */
	memset(&package, 0, sizeof(package));
	if (!rt_get_global(env, "__Accel", &package))
		return false;
	if (!rt_get_dict_native_pointer(
		env,
		&package,
		&native_pointer,
		&native_finalizer)) {
		return false;
	}

	/* Verifies that no user dictionary can impersonate the private package. */
	if (native_pointer == NULL ||
	    native_finalizer != accel_runtime_binding_finalizer) {
		rt_error(env, N_TR("Invalid private accelerator package."));
		return false;
	}

	binding = native_pointer;
	if (binding->magic != ACCEL_RUNTIME_BINDING_MAGIC ||
	    binding->context != env->vm->accel_optimize_userdata ||
	    !accel_runtime_ops_valid(&binding->ops)) {
		rt_error(env, N_TR("Private accelerator package is detached."));
		return false;
	}

	*result = binding;

	return true;
}

/* Borrow one immutable published program from an attached context. */
static bool
accel_runtime_lookup_program(
	struct rt_env *env,
	struct accel_runtime_binding *binding,
	uint32_t program_id,
	const struct accel_prepared_program **prepared,
	const struct accel_program **program)
{
	const struct accel_prepared_program *entry;
	const struct accel_program *entry_program;
	bool attached;

	assert(binding != NULL);
	assert(prepared != NULL);
	assert(program != NULL);

	*prepared = NULL;
	*program = NULL;

	/* Looks up immutable registry data while context destruction is excluded. */
	noct_enter_blocking(env);
	accel_context_state_lock(binding->context);
	attached = accel_context_is_attached_locked(binding->context);
	entry = NULL;
	entry_program = NULL;
	if (attached) {
		entry = accel_context_lookup_program_locked(
			binding->context,
			program_id);
		if (entry != NULL && entry->payload != NULL)
			entry_program = binding->ops.get_program(entry);
	}
	accel_context_state_unlock(binding->context);
	noct_leave_blocking(env);

	/* Publishes precise lookup failures after returning to the mutator. */
	if (!attached) {
		rt_error(
			env,
			N_TR("%s accelerator context is detached."),
			binding->ops.backend_display_name);
		return false;
	}
	if (entry == NULL ||
	    entry->payload == NULL ||
	    entry_program == NULL) {
		rt_error(
			env,
			N_TR("%s accelerator program is not published."),
			binding->ops.backend_display_name);
		return false;
	}

	*prepared = entry;
	*program = entry_program;

	return true;
}

/* Allocate all fixed-size plain metadata for one session. */
static bool
accel_runtime_allocate_metadata(
	struct rt_env *env,
	const struct accel_program *program,
	struct accel_runtime_session *session)
{
	uint32_t scalar_word_count;

	assert(program != NULL);
	assert(session != NULL);

	/* Rejects count arithmetic that cannot fit the executor ABI. */
	if (program->kernel_count >
	    (UINT32_MAX - program->scalar_count) / 2) {
		rt_error(env, N_TR("Accelerator scalar metadata is too large."));
		return false;
	}
	scalar_word_count = program->scalar_count + program->kernel_count * 2;
	session->scalar_word_count = scalar_word_count;

	/* Allocates signed scalar values used by checked size expressions. */
	if (program->scalar_count != 0) {
		session->scalar_value = noct_calloc(
			program->scalar_count,
			sizeof(*session->scalar_value));
		if (session->scalar_value == NULL) {
			rt_out_of_memory(env);
			return false;
		}
	}

	/* Allocates raw scalar and per-kernel dispatch words. */
	if (scalar_word_count != 0) {
		session->scalar_word = noct_calloc(
			scalar_word_count,
			sizeof(*session->scalar_word));
		if (session->scalar_word == NULL) {
			rt_out_of_memory(env);
			return false;
		}
	}

	/* Allocates deep-owned result descriptors and identity words. */
	if (program->scalar_result_count != 0) {
		if (program->scalar_result_count > ACCEL_MAX_SCALAR_BINDINGS) {
			rt_error(env, N_TR("Accelerator scalar result metadata is too large."));
			return false;
		}

		session->result = noct_calloc(
			program->scalar_result_count,
			sizeof(*session->result));
		if (session->result == NULL) {
			rt_out_of_memory(env);
			return false;
		}

		session->result_word = noct_calloc(
			program->scalar_result_count,
			sizeof(*session->result_word));
		if (session->result_word == NULL) {
			rt_out_of_memory(env);
			return false;
		}
	}
	session->result_word_count = program->scalar_result_count;

	/* Allocates every checked kernel dispatch table. */
	if (program->kernel_count != 0) {
		session->kernel_start = noct_calloc(
			program->kernel_count,
			sizeof(*session->kernel_start));
		if (session->kernel_start == NULL) {
			rt_out_of_memory(env);
			return false;
		}

		session->kernel_trip = noct_calloc(
			program->kernel_count,
			sizeof(*session->kernel_trip));
		if (session->kernel_trip == NULL) {
			rt_out_of_memory(env);
			return false;
		}

		session->kernel_active = noct_calloc(
			program->kernel_count,
			sizeof(*session->kernel_active));
		if (session->kernel_active == NULL) {
			rt_out_of_memory(env);
			return false;
		}
	}

	/* Allocates one runtime-owned transfer descriptor per buffer. */
	if (program->buffer_count != 0) {
		session->buffer = noct_calloc(
			program->buffer_count,
			sizeof(*session->buffer));
		if (session->buffer == NULL) {
			rt_out_of_memory(env);
			return false;
		}
	}

	return true;
}

/* Convert every runtime argument into plain session metadata. */
static bool
accel_runtime_read_metadata(
	struct rt_env *env,
	struct rt_value *args,
	struct rt_value *element,
	const struct accel_program *program,
	struct accel_runtime_session *session)
{
	size_t args_size;

	/* Reads the stable argument-array extent once for all bindings. */
	if (!noct_get_array_size(env, args, &args_size))
		return false;
	if (!accel_runtime_validate_argument_count(env, args_size, program))
		return false;

	/* Converts scalars before evaluating all dependent size expressions. */
	if (!accel_runtime_read_scalars(
		env,
		args,
		element,
		args_size,
		program,
		session)) {
		return false;
	}

	/* Validates every rewritten scalar-result identity and output slot. */
	if (!accel_runtime_read_results(
		env,
		args,
		element,
		args_size,
		program,
		session)) {
		return false;
	}

	/* Validates removed constructors before evaluating any dependent loop. */
	if (!accel_runtime_read_device_extents(env, program, session))
		return false;

	/* Resolves exact per-kernel start and trip values. */
	if (!accel_runtime_evaluate_kernels(env, program, session))
		return false;

	/* Validates Packed arguments and snapshots every active host buffer. */
	if (!accel_runtime_read_buffers(
		env,
		args,
		element,
		args_size,
		program,
		session)) {
		return false;
	}

	return true;
}

/* Validate the exact generated argument Array namespace. */
static bool
accel_runtime_validate_argument_count(
	struct rt_env *env,
	size_t args_size,
	const struct accel_program *program)
{
	size_t expected_count;
	uint32_t i;

	expected_count = program->parameter_count;

	/* Counts only host locals after the complete source-parameter prefix. */
	for (i = 0; i < program->buffer_count; i++) {
		if (program->buffer[i].origin == ACCEL_BUFFER_LOCAL_HOST)
			expected_count++;
	}

	/* Adds every scalar-result placeholder with an overflow guard. */
	if (program->scalar_result_count > (size_t)-1 - expected_count) {
		rt_error(env, N_TR("Accelerator argument count is too large."));
		return false;
	}
	expected_count += program->scalar_result_count;

	/* Rejects stale rewrites, including fake device-local placeholders. */
	if (args_size != expected_count) {
		rt_error(env, N_TR("Accelerator argument count changed."));
		return false;
	}

	return true;
}

/* Convert source scalars into checked values and raw executor words. */
static bool
accel_runtime_read_scalars(
	struct rt_env *env,
	struct rt_value *args,
	struct rt_value *element,
	size_t args_size,
	const struct accel_program *program,
	struct accel_runtime_session *session)
{
	const struct accel_scalar_binding *binding;
	float float_value;
	int int_value;
	uint32_t raw;
	uint32_t i;

	/* Converts each immutable source scalar in binding order. */
	for (i = 0; i < program->scalar_count; i++) {
		binding = &program->scalar[i];

		/* Rejects a stale or incomplete rewritten argument array. */
		if (binding->args_slot >= args_size) {
			rt_error(env, N_TR("Accelerator scalar argument is missing."));
			return false;
		}
		if (!noct_get_array_elem(
			env,
			args,
			binding->args_slot,
			element)) {
			return false;
		}

		/* Preserves the target-neutral 32-bit word representation. */
		if (binding->value_type == ACCEL_IR_I32) {
			if (!noct_get_int(env, element, &int_value))
				return false;
			session->scalar_value[i] = int_value;
			session->scalar_word[i] = (uint32_t)(int32_t)int_value;
		} else if (binding->value_type == ACCEL_IR_F32) {
			if (!noct_get_float(env, element, &float_value))
				return false;
			memcpy(&raw, &float_value, sizeof(raw));
			session->scalar_value[i] = 0;
			session->scalar_word[i] = raw;
		} else {
			rt_error(env, N_TR("Invalid accelerator scalar type."));
			return false;
		}
	}

	return true;
}

/* Validate and deep-copy every scalar-result descriptor and identity. */
static bool
accel_runtime_read_results(
	struct rt_env *env,
	struct rt_value *args,
	struct rt_value *element,
	size_t args_size,
	const struct accel_program *program,
	struct accel_runtime_session *session)
{
	const struct accel_scalar_result *result;
	int identity;
	uint32_t i;
	uint32_t j;

	/* Rejects an incomplete prepared-program result table. */
	if (program->scalar_result_count != 0 &&
	    program->scalar_result == NULL) {
		rt_error(env, N_TR("Accelerator scalar result metadata is missing."));
		return false;
	}

	/* Converts identities and keeps publication metadata runtime-private. */
	for (i = 0; i < program->scalar_result_count; i++) {
		result = &program->scalar_result[i];

		/* Requires the dense Int32 result ABI accepted by every backend. */
		if (result->result_entry_id != i ||
		    result->value_type != ACCEL_IR_I32) {
			rt_error(env, N_TR("Invalid accelerator scalar result metadata."));
			return false;
		}

		/* Rejects a duplicate destination before retaining its slot. */
		for (j = 0; j < i; j++) {
			if (session->result[j].args_slot == result->args_slot) {
				rt_error(env, N_TR("Duplicate accelerator scalar result argument."));
				return false;
			}
		}

		/* Requires the rewritten identity placeholder at the exact slot. */
		if (result->args_slot >= args_size) {
			rt_error(env, N_TR("Accelerator scalar result argument is missing."));
			return false;
		}
		if (!noct_get_array_elem(
			env,
			args,
			result->args_slot,
			element)) {
			return false;
		}
		if (!noct_get_int(env, element, &identity))
			return false;
		if ((uint32_t)(int32_t)identity != result->identity_bits) {
			rt_error(env, N_TR("Accelerator scalar result identity changed."));
			return false;
		}

		session->result[i].args_slot = result->args_slot;
		session->result[i].cpu_publication = result->cpu_publication;
		session->result_word[i] = result->identity_bits;
	}

	return true;
}

/* Evaluate every device-only constructor extent without touching Noct objects. */
static bool
accel_runtime_read_device_extents(
	struct rt_env *env,
	const struct accel_program *program,
	struct accel_runtime_session *session)
{
	const struct accel_buffer_binding *binding;
	struct accel_runtime_buffer *buffer;
	int64_t extent;
	uint32_t i;

	/* Validates device allocations even when every dependent loop is empty. */
	for (i = 0; i < program->buffer_count; i++) {
		binding = &program->buffer[i];
		if (binding->origin != ACCEL_BUFFER_LOCAL_DEVICE)
			continue;

		buffer = &session->buffer[i];
		buffer->origin = binding->origin;
		buffer->args_slot = binding->args_slot;
		buffer->element_kind = binding->element_kind;
		buffer->element_width = binding->element_width;

		/* Rejects a device descriptor that leaked into the host namespace. */
		if (binding->args_slot != ACCEL_ARGS_SLOT_NONE ||
		    binding->host_visible ||
		    binding->upload_required ||
		    binding->download_required ||
		    binding->materialization_required) {
			rt_error(env, N_TR("Invalid accelerator device-local buffer."));
			return false;
		}

		/* Evaluates the removed Packed constructor's extent in source order. */
		if (!accel_program_evaluate_size(
			program,
			binding->extent_expression,
			program->scalar_count,
			session->scalar_value,
			&extent)) {
			rt_error(env, N_TR("Accelerator device buffer size overflowed."));
			return false;
		}
		if (extent <= 0) {
			rt_error(env, N_TR("Accelerator device buffer size is not positive."));
			return false;
		}
		if ((uint64_t)extent > (uint64_t)((size_t)-1)) {
			rt_error(env, N_TR("Accelerator device buffer size is too large."));
			return false;
		}
		if (binding->element_width == 0 ||
		    (size_t)extent > (size_t)-1 / binding->element_width) {
			rt_error(env, N_TR("Accelerator device buffer size is too large."));
			return false;
		}

		/* Publishes only plain allocation metadata for the selected backend. */
		buffer->element_count = (size_t)extent;
		buffer->byte_count = (size_t)extent * binding->element_width;
	}

	return true;
}

/* Evaluate all checked dynamic kernel ranges. */
static bool
accel_runtime_evaluate_kernels(
	struct rt_env *env,
	const struct accel_program *program,
	struct accel_runtime_session *session)
{
	const struct accel_kernel_plan *kernel;
	int64_t start;
	int64_t trip;
	uint32_t word_index;
	uint32_t i;

	/* Resolves every dispatch before any backend resource is created. */
	for (i = 0; i < program->kernel_count; i++) {
		kernel = &program->kernel[i];

		/* Evaluates the signed source loop start with overflow checks. */
		if (!accel_program_evaluate_size(
			program,
			kernel->start_expression,
			program->scalar_count,
			session->scalar_value,
			&start)) {
			rt_error(env, N_TR("Accelerator loop start overflowed."));
			return false;
		}

		/* Evaluates the nonnegative trip count with overflow checks. */
		if (!accel_program_evaluate_size(
			program,
			kernel->trip_expression,
			program->scalar_count,
			session->scalar_value,
			&trip)) {
			rt_error(env, N_TR("Accelerator trip count overflowed."));
			return false;
		}

		/* Restricts dispatch words to the target-neutral 32-bit ABI. */
		if (start < 0 ||
		    start > INT32_MAX ||
		    trip < 0 ||
		    trip > UINT32_MAX) {
			rt_error(env, N_TR("Accelerator dispatch range is too large."));
			return false;
		}

		session->kernel_start[i] = (uint32_t)start;
		session->kernel_trip[i] = (uint32_t)trip;
		session->kernel_active[i] = trip != 0;
		word_index = program->scalar_count + i * 2;
		session->scalar_word[word_index] = (uint32_t)start;
		session->scalar_word[word_index + 1] = (uint32_t)trip;
	}

	return true;
}

/* Validate host buffers and take complete runtime-owned snapshots. */
static bool
accel_runtime_read_buffers(
	struct rt_env *env,
	struct rt_value *args,
	struct rt_value *element,
	size_t args_size,
	const struct accel_program *program,
	struct accel_runtime_session *session)
{
	const struct accel_buffer_binding *binding;
	struct accel_runtime_buffer *buffer;
	void *pointer;
	size_t element_count;
	size_t byte_count;
	int packed_type;
	uint32_t i;

	/* Validates each descriptor even when every dependent kernel is empty. */
	for (i = 0; i < program->buffer_count; i++) {
		binding = &program->buffer[i];
		buffer = &session->buffer[i];

		/* Records immutable binding metadata needed again at finish. */
		buffer->origin = binding->origin;
		buffer->args_slot = binding->args_slot;
		buffer->element_kind = binding->element_kind;
		buffer->element_width = binding->element_width;

		/* Plans device-only storage without reading an Array or Packed value. */
		if (binding->origin == ACCEL_BUFFER_LOCAL_DEVICE) {
			if (!accel_runtime_plan_buffer(env, program, session, i))
				return false;
			if (buffer->upload ||
			    buffer->download ||
			    buffer->snapshot != NULL) {
				rt_error(env, N_TR("Invalid accelerator device-local transfer."));
				return false;
			}
			continue;
		}

		/* Resolves the current host Packed object without retaining it. */
		if ((binding->origin != ACCEL_BUFFER_PARAMETER &&
		     binding->origin != ACCEL_BUFFER_LOCAL_HOST) ||
		    !binding->host_visible ||
		    binding->args_slot == ACCEL_ARGS_SLOT_NONE ||
		    binding->args_slot >= args_size) {
			rt_error(env, N_TR("Accelerator Packed argument is missing."));
			return false;
		}
		if (!noct_get_array_elem(
			env,
			args,
			binding->args_slot,
			element)) {
			return false;
		}
		if (!noct_get_packed_type(env, element, &packed_type))
			return false;
		if (packed_type != binding->element_kind) {
			rt_error(
				env,
				N_TR("Accelerator Packed element type does not match."));
			return false;
		}
		if (!noct_get_packed_size(env, element, &element_count))
			return false;

		/* Computes the full host extent before any plain allocation. */
		if (binding->element_width == 0 ||
		    element_count > (size_t)-1 / binding->element_width) {
			rt_error(env, N_TR("Accelerator buffer size overflowed."));
			return false;
		}
		byte_count = element_count * binding->element_width;
		buffer->element_count = element_count;
		buffer->byte_count = byte_count;

		/* Folds active effects and validates their exact dynamic ranges. */
		if (!accel_runtime_plan_buffer(env, program, session, i))
			return false;

		/* Allocates plain publication storage for every host transfer. */
		if ((!buffer->upload &&
		     !buffer->download) ||
		    byte_count == 0)
			continue;

		buffer->snapshot = noct_malloc(byte_count);
		if (buffer->snapshot == NULL) {
			rt_out_of_memory(env);
			return false;
		}

		/* Copies the current host bytes only when an upload is required. */
		if (buffer->upload) {
			if (!noct_get_packed_pointer(env, element, &pointer))
				return false;
			memcpy(buffer->snapshot, pointer, byte_count);
		}
	}

	return true;
}

/* Fold active effects and validate one buffer's dynamic access range. */
static bool
accel_runtime_plan_buffer(
	struct rt_env *env,
	const struct accel_program *program,
	struct accel_runtime_session *session,
	uint32_t buffer_index)
{
	const struct accel_buffer_binding *binding;
	const struct accel_buffer_effect *effect;
	struct accel_runtime_buffer *buffer;
	int64_t first;
	int64_t end;
	uint32_t i;
	bool device_defined;
	bool any_use;

	binding = &program->buffer[buffer_index];
	buffer = &session->buffer[buffer_index];
	device_defined = false;
	any_use = false;

	/* Folds only nonempty kernels in source order. */
	for (i = 0; i < program->kernel_count; i++) {
		if (!session->kernel_active[i])
			continue;

		effect = &binding->effect[i];
		if (!effect->read && !effect->write)
			continue;

		any_use = true;

		/* Supplies initial host bytes or rejects an uninitialized device read. */
		if (!device_defined &&
		    (effect->read || effect->read_before_write)) {
			if (binding->origin == ACCEL_BUFFER_LOCAL_DEVICE) {
				rt_error(env, N_TR("Accelerator device buffer is read before definition."));
				return false;
			}
			buffer->upload = true;
		}

		/* Records writes without turning device-only storage into a transfer. */
		if (effect->write) {
			if (binding->origin != ACCEL_BUFFER_LOCAL_DEVICE &&
			    !effect->full_overwrite &&
			    !device_defined) {
				buffer->upload = true;
			}
			if (binding->origin != ACCEL_BUFFER_LOCAL_DEVICE)
				buffer->download = true;
			device_defined = true;
		}

		/* Evaluates and validates this kernel's exact element interval. */
		if (!accel_program_evaluate_size(
			program,
			binding->kernel_required_first_expression[i],
			program->scalar_count,
			session->scalar_value,
			&first)) {
			rt_error(env, N_TR("Accelerator buffer range overflowed."));
			return false;
		}
		if (!accel_program_evaluate_size(
			program,
			binding->kernel_required_end_expression[i],
			program->scalar_count,
			session->scalar_value,
			&end)) {
			rt_error(env, N_TR("Accelerator buffer range overflowed."));
			return false;
		}
		if (first < 0 ||
		    end < first ||
		    (uint64_t)end > buffer->element_count) {
			rt_error(env, N_TR("Accelerator buffer access is out of range."));
			return false;
		}
	}

	buffer->active = any_use;
	if (!any_use)
		return true;

	/* Leaves device-only storage entirely outside the host transfer path. */
	if (binding->origin == ACCEL_BUFFER_LOCAL_DEVICE)
		return true;

	/* Preserves untouched host contents until a full-extent proof exists. */
	buffer->upload = true;

	return true;
}

/* Validate backend limits, create resources, and link one live session. */
static enum accel_runtime_create_status
accel_runtime_create_and_link(
	struct rt_env *env,
	struct accel_runtime_binding *binding,
	const struct accel_prepared_program *prepared,
	struct accel_runtime_session *session,
	char *error,
	size_t error_size)
{
	const struct accel_prepared_program *current;
	void *backend_state;
	void *execution;
	uint32_t i;
	bool valid;
	bool created;
	bool unchanged;

	assert(binding != NULL);
	assert(env != NULL);
	assert(prepared != NULL);
	assert(session != NULL);
	assert(error != NULL);
	assert(error_size != 0);

	/* Claims a live creating session and a stable backend-state pointer. */
	noct_enter_blocking(env);
	accel_context_state_lock(binding->context);
	current = accel_context_lookup_program_locked(
		binding->context,
		session->program_id);
	if (!accel_context_is_attached_locked(binding->context) ||
	    current != prepared) {
		accel_context_state_unlock(binding->context);
		noct_leave_blocking(env);
		return ACCEL_RUNTIME_CREATE_CHANGED;
	}

	backend_state = accel_context_get_backend_state(binding->context);
	accel_context_link_session_locked(binding->context, &session->live);
	accel_context_state_unlock(binding->context);

	/* Applies optional device limits without holding the context state mutex. */
	if (binding->ops.validate_dispatch_limit != NULL) {
		for (i = 0; i < session->kernel_count; i++) {
			if (!session->kernel_active[i])
				continue;

			error[0] = '\0';
			valid = binding->ops.validate_dispatch_limit(
				backend_state,
				prepared,
				i,
				session->kernel_start[i],
				session->kernel_trip[i],
				error,
				error_size);
			error[error_size - 1] = '\0';
			if (!valid) {
				accel_context_state_lock(binding->context);
				if (session->context == binding->context &&
				    session->live.linked &&
				    session->state == ACCEL_RUNTIME_CREATING) {
					accel_context_unlink_session_locked(
						binding->context,
						&session->live);
					session->state = ACCEL_RUNTIME_FAILED;
					session->context = NULL;
				}
				accel_context_state_unlock(binding->context);
				noct_leave_blocking(env);
				return ACCEL_RUNTIME_CREATE_LIMIT;
			}
		}
	}

	/* Creates an execution using only immutable plans and plain snapshots. */
	error[0] = '\0';
	execution = NULL;
	created = binding->ops.create_execution(
		backend_state,
		prepared,
		session->scalar_word_count,
		session->scalar_word,
		session->result_word_count,
		session->result_word,
		session->buffer_count,
		session->buffer,
		&execution,
		error,
		error_size);
	error[error_size - 1] = '\0';

	/* Reclaims an incomplete execution without holding the state mutex. */
	if (!created || execution == NULL) {
		if (execution != NULL)
			binding->ops.destroy_execution(execution);

		accel_context_state_lock(binding->context);
		if (session->context == binding->context &&
		    session->live.linked &&
		    session->state == ACCEL_RUNTIME_CREATING) {
			accel_context_unlink_session_locked(
				binding->context,
				&session->live);
			session->state = ACCEL_RUNTIME_FAILED;
			session->context = NULL;
		}
		accel_context_state_unlock(binding->context);
		noct_leave_blocking(env);
		return ACCEL_RUNTIME_CREATE_BACKEND;
	}

	/* Revalidates the creation claim before publishing backend ownership. */
	accel_context_state_lock(binding->context);
	current = accel_context_lookup_program_locked(
		binding->context,
		session->program_id);
	unchanged = accel_context_is_attached_locked(binding->context) &&
		session->context == binding->context &&
		session->live.linked &&
		session->state == ACCEL_RUNTIME_CREATING &&
		current == prepared;
	if (!unchanged) {
		if (session->context == binding->context &&
		    session->live.linked &&
		    session->state == ACCEL_RUNTIME_CREATING) {
			accel_context_unlink_session_locked(
				binding->context,
				&session->live);
			session->state = ACCEL_RUNTIME_FAILED;
			session->context = NULL;
		}
		accel_context_state_unlock(binding->context);
		binding->ops.destroy_execution(execution);
		noct_leave_blocking(env);
		return ACCEL_RUNTIME_CREATE_CHANGED;
	}

	session->execution = execution;
	session->state = ACCEL_RUNTIME_RECORDING;
	accel_context_state_unlock(binding->context);
	noct_leave_blocking(env);

	return ACCEL_RUNTIME_CREATE_OK;
}

/* Install one session in an opaque private dictionary and return it. */
static bool
accel_runtime_install_session(
	struct rt_env *env,
	struct accel_runtime_session *session,
	struct rt_value *returned,
	bool *installed)
{
	assert(session != NULL);
	assert(returned != NULL);
	assert(installed != NULL);

	/* Creates the opaque native dictionary returned to rewritten HIR. */
	if (!rt_make_empty_dict(env, returned))
		return false;
	if (!rt_set_dict_native_pointer(
		env,
		returned,
		session,
		accel_runtime_session_finalizer)) {
		return false;
	}
	*installed = true;

	/* Publishes the dictionary while preserving finalizer ownership on failure. */
	if (!noct_set_return(env, returned)) {
		if (rt_set_dict_native_pointer(env, returned, NULL, NULL))
			*installed = false;
		return false;
	}

	return true;
}

/* Retrieve and validate an opaque private session dictionary argument. */
static bool
accel_runtime_get_session_argument(
	struct rt_env *env,
	struct rt_value *value,
	struct accel_runtime_session **session)
{
	void *native_pointer;
	void (*native_finalizer)(void *native_pointer);

	assert(session != NULL);

	*session = NULL;

	/* Reads the native owner only from a validated dictionary argument. */
	if (!noct_get_arg_check_dict(env, 0, value))
		return false;
	if (!rt_get_dict_native_pointer(
		env,
		value,
		&native_pointer,
		&native_finalizer)) {
		return false;
	}

	/* Prevents arbitrary native dictionaries from impersonating sessions. */
	if (native_pointer == NULL ||
	    native_finalizer != accel_runtime_session_finalizer) {
		rt_error(env, N_TR("Invalid accelerator session."));
		return false;
	}

	*session = native_pointer;

	return true;
}

/* Revalidate all output objects and copy completed plain snapshots. */
static bool
accel_runtime_copy_results(
	struct rt_env *env,
	struct rt_value *args,
	struct rt_value *element,
	struct accel_runtime_session *session)
{
	struct accel_runtime_buffer *buffer;
	struct rt_value previous_result[ACCEL_MAX_SCALAR_BINDINGS];
	struct rt_value published_result[ACCEL_MAX_SCALAR_BINDINGS];
	void *pointer;
	size_t args_size;
	size_t element_count;
	int result_value;
	int32_t signed_result;
	int packed_type;
	uint32_t i;
	uint32_t j;

	memset(previous_result, 0, sizeof(previous_result));
	memset(published_result, 0, sizeof(published_result));

	/* Reads the current argument-array extent before reacquiring outputs. */
	if (!noct_get_array_size(env, args, &args_size))
		return false;

	/* Validates every destination before publishing any completed snapshot. */
	for (i = 0; i < session->buffer_count; i++) {
		buffer = &session->buffer[i];
		if (!buffer->download)
			continue;

		/* Rejects argument replacement while the backend was executing. */
		if (buffer->args_slot >= args_size) {
			rt_error(env, N_TR("Accelerator output argument is missing."));
			return false;
		}
		if (!noct_get_array_elem(
			env,
			args,
			buffer->args_slot,
			element)) {
			return false;
		}
		if (!noct_get_packed_type(env, element, &packed_type))
			return false;
		if (packed_type != buffer->element_kind) {
			rt_error(env, N_TR("Accelerator output type changed."));
			return false;
		}
		if (!noct_get_packed_size(env, element, &element_count))
			return false;
		if (element_count != buffer->element_count) {
			rt_error(env, N_TR("Accelerator output size changed."));
			return false;
		}

		/* Requires every nonempty download to own a completed snapshot. */
		if (buffer->byte_count == 0)
			continue;
		if (buffer->snapshot == NULL) {
			rt_error(env, N_TR("Accelerator output snapshot is missing."));
			return false;
		}
	}

	/* Validates every published scalar destination before copying any result. */
	for (i = 0; i < session->result_word_count; i++) {
		if (!session->result[i].cpu_publication)
			continue;

		if (session->result[i].args_slot >= args_size) {
			rt_error(env, N_TR("Accelerator scalar result argument is missing."));
			return false;
		}
		if (!noct_get_array_elem(
			env,
			args,
			session->result[i].args_slot,
			&previous_result[i])) {
			return false;
		}
		if (!noct_get_int(env, &previous_result[i], &result_value))
			return false;

		/* Preflights array mutability with an observably identical write. */
		if (!noct_set_array_elem(
			env,
			args,
			session->result[i].args_slot,
			&previous_result[i])) {
			return false;
		}

		/* Converts the raw GPU word through its exact signed representation. */
		memcpy(
			&signed_result,
			&session->result_word[i],
			sizeof(signed_result));
		if (!noct_make_int(
			env,
			&published_result[i],
			(int)signed_result)) {
			return false;
		}
	}

	/* Publishes all snapshots only after the complete validation pass. */
	for (i = 0; i < session->buffer_count; i++) {
		buffer = &session->buffer[i];
		if (!buffer->download || buffer->byte_count == 0)
			continue;

		/* Reacquires each movable destination only for its immediate copy. */
		if (!noct_get_array_elem(
			env,
			args,
			buffer->args_slot,
			element)) {
			return false;
		}
		if (!noct_get_packed_pointer(env, element, &pointer))
			return false;
		memcpy(pointer, buffer->snapshot, buffer->byte_count);
	}

	/* Publishes every requested scalar only after all destination validation. */
	for (i = 0; i < session->result_word_count; i++) {
		if (!session->result[i].cpu_publication)
			continue;

		if (!noct_set_array_elem(
			env,
			args,
			session->result[i].args_slot,
			&published_result[i])) {
			/* Restores earlier scalar slots when an unexpected write fails. */
			for (j = 0; j < i; j++) {
				if (!session->result[j].cpu_publication)
					continue;
				(void)noct_set_array_elem(
					env,
					args,
					session->result[j].args_slot,
					&previous_result[j]);
			}
			return false;
		}
	}

	return true;
}

/* Publish one backend callback failure through the runtime error channel. */
static void
accel_runtime_backend_error(
	struct rt_env *env,
	const char *backend_name,
	const char *operation,
	const char *error)
{
	assert(env != NULL);
	assert(backend_name != NULL);
	assert(operation != NULL);
	assert(error != NULL);

	/* Preserves a backend diagnostic when it supplied one. */
	if (error[0] != '\0') {
		rt_error(
			env,
			N_TR("%s accelerator %s: %s"),
			backend_name,
			operation,
			error);
		return;
	}

	/* Supplies a deterministic fallback for empty backend diagnostics. */
	rt_error(
		env,
		N_TR("%s accelerator %s."),
		backend_name,
		operation);
}

/* Pin an ordered group of GC-visible local values. */
static bool
accel_runtime_pin_values(
	struct rt_env *env,
	struct rt_value *value[],
	uint32_t value_count,
	uint32_t *pinned_count)
{
	assert(env != NULL);
	assert(value != NULL);
	assert(pinned_count != NULL);
	assert(*pinned_count == 0);

	/* Pins values in declaration order. */
	while (*pinned_count < value_count) {
		if (!rt_pin_local(env, value[*pinned_count])) {
			(void)accel_runtime_unpin_values(
				env,
				value,
				pinned_count);
			return false;
		}
		(*pinned_count)++;
	}

	return true;
}

/* Unpin an ordered group of local values in reverse order. */
static bool
accel_runtime_unpin_values(
	struct rt_env *env,
	struct rt_value *value[],
	uint32_t *pinned_count)
{
	bool success;

	assert(env != NULL);
	assert(value != NULL);
	assert(pinned_count != NULL);

	success = true;

	/* Releases the most recently acquired root first. */
	while (*pinned_count != 0) {
		if (!rt_unpin_local(env, value[*pinned_count - 1])) {
			success = false;
			break;
		}
		(*pinned_count)--;
	}

	return success;
}

/* Close and destroy one session not owned by a native dictionary. */
static void
accel_runtime_discard_session(
	struct rt_env *env,
	struct accel_runtime_session *session)
{
	struct accel_context *context;
	void *execution;

	if (session == NULL)
		return;

	context = session->context;
	execution = NULL;

	/* Releases any backend execution while its context state remains live. */
	if (context != NULL) {
		noct_enter_blocking(env);
		accel_context_state_lock(context);
		if (session->context == context) {
			if (session->live.linked)
				accel_context_unlink_session_locked(context, &session->live);
			execution = session->execution;
			session->execution = NULL;
			session->prepared = NULL;
			session->state = ACCEL_RUNTIME_FAILED;
			session->context = NULL;
		}
		accel_context_state_unlock(context);
		if (execution != NULL)
			session->ops.destroy_execution(execution);
		noct_leave_blocking(env);
	}

	/* Releases the detached plain wrapper and all owned snapshots. */
	accel_runtime_destroy_session(session);
}

/* Unlink and close one terminal backend failure under the context mutex. */
static void *
accel_runtime_fail_session_locked(
	struct accel_runtime_session *session)
{
	void *execution;

	assert(session != NULL);
	assert(session->context != NULL);

	if (session->live.linked) {
		accel_context_unlink_session_locked(
			session->context,
			&session->live);
	}
	execution = session->execution;
	session->execution = NULL;
	session->prepared = NULL;
	session->state = ACCEL_RUNTIME_FAILED;
	session->context = NULL;

	return execution;
}

/* Detach one context-owned payload for destruction outside the state mutex. */
static void *
accel_runtime_session_orphan_locked(
	struct accel_live_session *live)
{
	struct accel_runtime_session *session;
	void *execution;

	assert(live != NULL);

	session = (struct accel_runtime_session *)((char *)live -
		offsetof(struct accel_runtime_session, live));
	execution = session->execution;
	session->execution = NULL;
	session->prepared = NULL;
	session->state = ACCEL_RUNTIME_ORPHANED;
	session->context = NULL;

	return execution;
}

/* Unlink and release a session after its native dictionary dies. */
static void
accel_runtime_session_finalizer(
	void *native_pointer)
{
	struct accel_context *context;
	struct accel_runtime_session *session;
	void *execution;
	bool cleanup_claimed;

	session = native_pointer;
	if (session == NULL)
		return;

	/* Rejects corrupted wrappers without dereferencing context state. */
	if (session->magic != ACCEL_RUNTIME_SESSION_MAGIC) {
		noct_free(session);
		return;
	}

	/* Orphans a still-live execution before destroying its plain wrapper. */
	context = session->owner_context;
	execution = NULL;
	cleanup_claimed = false;
	if (context != NULL) {
		cleanup_claimed = accel_context_begin_session_cleanup(
			context,
			&session->live);
	}
	if (cleanup_claimed) {
		accel_context_state_lock(context);
		if (session->context == context && session->live.linked) {
			accel_context_unlink_session_locked(context, &session->live);
			execution = session->execution;
			session->execution = NULL;
			session->prepared = NULL;
			session->state = ACCEL_RUNTIME_ORPHANED;
			session->context = NULL;
		}
		accel_context_state_unlock(context);
	}

	/* Releases normal-GC backend resources outside the context state mutex. */
	if (execution != NULL)
		session->ops.destroy_execution(execution);
	if (cleanup_claimed)
		accel_context_end_operation(context);

	accel_runtime_destroy_session(session);
}

/* Release one detached wrapper and every runtime-owned plain allocation. */
static void
accel_runtime_destroy_session(
	struct accel_runtime_session *session)
{
	struct accel_context *owner_context;
	uint32_t i;

	if (session == NULL)
		return;

	/* Defensively closes an execution left by an incomplete create callback. */
	if (session->execution != NULL) {
		session->ops.destroy_execution(session->execution);
		session->execution = NULL;
	}

	/* Releases every complete host snapshot owned by the generic runtime. */
	if (session->buffer != NULL) {
		for (i = 0; i < session->buffer_count; i++)
			noct_free(session->buffer[i].snapshot);
	}

	noct_free(session->buffer);
	noct_free(session->kernel_active);
	noct_free(session->kernel_trip);
	noct_free(session->kernel_start);
	noct_free(session->result);
	noct_free(session->result_word);
	noct_free(session->scalar_word);
	noct_free(session->scalar_value);
	owner_context = session->owner_context;
	session->owner_context = NULL;
	memset(&session->ops, 0, sizeof(session->ops));
	session->magic = 0;
	noct_free(session);

	if (owner_context != NULL)
		accel_context_release(owner_context);
}
