/* -*- coding: utf-8; tab-width: 8; indent-tabs-mode: t; -*- */

/*
 * Noct Programming Language
 * Copyright (c) 2025, 2026, Awe Morris
 */

/*
 * VM-local accelerator context and program registry.
 */

#include "accel_context.h"
#include "runtime.h"

#include <assert.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>

struct accel_registry_entry {
	uint32_t program_id;
	struct accel_prepared_program program;
};

struct accel_registry_reservation {
	struct accel_context *context;
	uint32_t count;
	struct accel_registry_entry **entry;
};

static bool accel_context_ops_valid(const struct accel_backend_ops *ops);
static bool accel_context_reserve_locked(struct accel_context *context, uint32_t count, struct accel_registry_reservation *reservation);
static bool accel_context_reservation_range(struct accel_context *context, uint32_t count, uint32_t *last_id, uint32_t *required_capacity);
static bool accel_context_grow_registry_locked(struct accel_context *context, uint32_t required_capacity);
static void accel_context_free_reservation(struct accel_registry_reservation *reservation);
static bool accel_context_reservation_valid_locked(const struct accel_context *context, const struct accel_registry_reservation *reservation);

/*
 * Creates a detached VM-local accelerator context.
 */
bool
accel_context_create(
	struct rt_vm *vm,
	const struct accel_backend_ops *ops,
	void *backend_state,
	struct accel_context **result)
{
	struct accel_context *context;

	if (result == NULL)
		return false;

	*result = NULL;

	if (vm == NULL)
		return false;
	if (!accel_context_ops_valid(ops))
		return false;

	context = noct_calloc(1, sizeof(*context));
	if (context == NULL)
		return false;

	context->vm = vm;
	context->ops = *ops;
	context->backend_state = backend_state;
	context->next_program_id = 1;
	context->reference_count = 1;

	if (!accel_mutex_init(&context->state_mutex)) {
		noct_free(context);
		return false;
	}
	if (!accel_condition_init(&context->state_condition)) {
		accel_mutex_destroy(&context->state_mutex);
		noct_free(context);
		return false;
	}

	*result = context;

	return true;
}

/*
 * Reserves stable program IDs and no-fail publication storage.
 */
bool
accel_context_reserve_programs(
	struct accel_context *context,
	uint32_t count,
	struct accel_registry_reservation **result)
{
	struct accel_registry_reservation *reservation;
	bool success;

	if (result == NULL)
		return false;

	*result = NULL;

	if (context == NULL)
		return false;
	if (count == 0)
		return false;

	reservation = noct_calloc(1, sizeof(*reservation));
	if (reservation == NULL)
		return false;

	reservation->entry = noct_calloc(count, sizeof(*reservation->entry));
	if (reservation->entry == NULL) {
		noct_free(reservation);
		return false;
	}

	reservation->context = context;
	reservation->count = count;

	/* Reserves all IDs and wrappers atomically with respect to shutdown. */
	accel_mutex_lock(&context->state_mutex);
	success = false;
	if (context->attached && !context->shutting_down) {
		success = accel_context_reserve_locked(
			context,
			count,
			reservation);
	}
	accel_mutex_unlock(&context->state_mutex);

	if (!success) {
		accel_context_free_reservation(reservation);
		return false;
	}

	*result = reservation;

	return true;
}

/*
 * Returns one stable program ID from a registry reservation.
 */
uint32_t
accel_registry_reservation_get_id(
	const struct accel_registry_reservation *reservation,
	uint32_t index)
{
	if (reservation == NULL)
		return 0;
	if (index >= reservation->count)
		return 0;
	if (reservation->entry[index] == NULL)
		return 0;

	return reservation->entry[index]->program_id;
}

/*
 * Cancels an unpublished registry reservation.
 */
void
accel_context_cancel_reservation(
	struct accel_context *context,
	struct accel_registry_reservation *reservation)
{
	if (reservation == NULL)
		return;

	assert(context != NULL);
	assert(reservation->context == context);

	if (context == NULL)
		return;
	if (reservation->context != context)
		return;

	accel_mutex_lock(&context->state_mutex);
	accel_context_free_reservation(reservation);
	accel_mutex_unlock(&context->state_mutex);
}

/*
 * Locks and revalidates a registry commit reservation.
 */
bool
accel_context_lock_commit(
	struct accel_context *context,
	struct accel_registry_reservation *reservation,
	struct accel_registry_commit_guard *guard)
{
	if (guard == NULL)
		return false;
	if (guard->context != NULL)
		return false;
	if (guard->reservation != NULL)
		return false;
	if (guard->locked)
		return false;
	if (context == NULL)
		return false;
	if (reservation == NULL)
		return false;
	if (reservation->context != context)
		return false;

	accel_mutex_lock(&context->state_mutex);
	if (!accel_context_reservation_valid_locked(context, reservation)) {
		accel_mutex_unlock(&context->state_mutex);
		return false;
	}

	guard->context = context;
	guard->reservation = reservation;
	guard->locked = true;

	return true;
}

/*
 * Publishes prepared programs without allocation or failure.
 */
void
accel_context_publish_programs_locked(
	struct accel_registry_commit_guard *guard,
	struct accel_prepared_program program[])
{
	struct accel_context *context;
	struct accel_registry_reservation *reservation;
	struct accel_registry_entry *entry;
	uint32_t i;

	assert(guard != NULL);
	assert(guard->locked);
	assert(guard->context != NULL);
	assert(guard->reservation != NULL);
	assert(program != NULL);

	context = guard->context;
	reservation = guard->reservation;

	/* Move every prepared payload into its stable registry wrapper. */
	for (i = 0; i < reservation->count; i++) {
		entry = reservation->entry[i];
		assert(entry != NULL);
		assert(entry->program_id < context->registry_capacity);
		assert(context->registry[entry->program_id] == NULL);

		entry->program = program[i];
		program[i].payload = NULL;
		context->registry[entry->program_id] = entry;
		reservation->entry[i] = NULL;
	}

	noct_free(reservation->entry);
	reservation->entry = NULL;
	reservation->context = NULL;
	reservation->count = 0;
	noct_free(reservation);
	guard->reservation = NULL;
}

/*
 * Unlocks and clears a registry commit guard.
 */
void
accel_context_unlock_commit(
	struct accel_registry_commit_guard *guard)
{
	struct accel_context *context;

	if (guard == NULL)
		return;
	if (!guard->locked)
		return;

	assert(guard->context != NULL);

	context = guard->context;
	guard->context = NULL;
	guard->reservation = NULL;
	guard->locked = false;

	accel_mutex_unlock(&context->state_mutex);
}

/*
 * Locks the backend-neutral context state.
 */
void
accel_context_state_lock(
	struct accel_context *context)
{
	assert(context != NULL);

	accel_mutex_lock(&context->state_mutex);
}

/*
 * Unlocks the backend-neutral context state.
 */
void
accel_context_state_unlock(
	struct accel_context *context)
{
	assert(context != NULL);

	accel_mutex_unlock(&context->state_mutex);
}

/*
 * Reports whether a state-locked context is attached to its VM.
 */
bool
accel_context_is_attached_locked(
	const struct accel_context *context)
{
	assert(context != NULL);

	return context->attached;
}

/*
 * Claims the context until one external operation has finished.
 *
 * Destruction closes the claim gate before waiting for every accepted
 * operation.  A successful caller may therefore borrow context, registry,
 * and backend state until it releases this claim.
 */
bool
accel_context_begin_operation(
	struct accel_context *context)
{
	bool claimed;

	/* Rejects an absent context before acquiring its state mutex. */
	if (context == NULL)
		return false;

	claimed = false;
	accel_mutex_lock(&context->state_mutex);

	/* Accepts work only while the context is attached and below its limit. */
	if (context->attached &&
	    !context->shutting_down &&
	    context->active_operation_count != UINT32_MAX) {
		context->active_operation_count++;
		claimed = true;
	}

	accel_mutex_unlock(&context->state_mutex);

	return claimed;
}

/*
 * Releases one previously claimed external operation.
 */
void
accel_context_end_operation(
	struct accel_context *context)
{
	assert(context != NULL);

	accel_mutex_lock(&context->state_mutex);
	assert(context->active_operation_count != 0);

	/* Releases the claim and wakes a destructor waiting for the last one. */
	if (context->active_operation_count == 0) {
		accel_mutex_unlock(&context->state_mutex);
		abort();
	}
	context->active_operation_count--;
	if (context->active_operation_count == 0)
		accel_condition_wake_all(&context->state_condition);

	accel_mutex_unlock(&context->state_mutex);
}

/*
 * Claims backend cleanup for one still-linked session finalizer.
 *
 * A finalizer arriving before context destruction joins the normal operation
 * drain.  One arriving after destruction has claimed the live list waits until
 * that destructor detaches the session payload instead.
 */
bool
accel_context_begin_session_cleanup(
	struct accel_context *context,
	struct accel_live_session *session)
{
	bool claimed;

	/* Rejects incomplete finalizer state before acquiring its context. */
	if (context == NULL || session == NULL)
		return false;

	claimed = false;
	accel_mutex_lock(&context->state_mutex);

	/* Waits for the destructor to detach a session it already owns. */
	while (context->destroying && session->linked) {
		accel_condition_wait(
			&context->state_condition,
			&context->state_mutex);
	}

	/* Joins the drain before unlinking and destroying a live payload. */
	if (session->linked) {
		if (context->resources_destroyed ||
		    context->active_operation_count == UINT32_MAX) {
			accel_mutex_unlock(&context->state_mutex);
			abort();
		}
		context->active_operation_count++;
		claimed = true;
	}

	accel_mutex_unlock(&context->state_mutex);

	return claimed;
}

/*
 * Retains the context shell for one non-owner wrapper.
 */
bool
accel_context_retain(
	struct accel_context *context)
{
	bool retained;

	/* Rejects an absent context before acquiring its state mutex. */
	if (context == NULL)
		return false;

	retained = false;
	accel_mutex_lock(&context->state_mutex);

	/* Retains only a live reference counter with available capacity. */
	if (!context->destroying &&
	    !context->resources_destroyed &&
	    context->reference_count != 0 &&
	    context->reference_count != UINT32_MAX) {
		context->reference_count++;
		retained = true;
	}

	accel_mutex_unlock(&context->state_mutex);

	return retained;
}

/*
 * Releases one context-shell reference.
 *
 * Backend resources are destroyed by the owner before it releases its
 * reference.  Binding and session references keep the synchronization shell
 * alive until no finalizer can still be waiting for its state mutex.
 */
void
accel_context_release(
	struct accel_context *context)
{
	bool destroy_shell;

	assert(context != NULL);

	destroy_shell = false;
	accel_mutex_lock(&context->state_mutex);
	assert(context->reference_count != 0);

	/* Releases this reference and claims terminal shell ownership at zero. */
	if (context->reference_count == 0) {
		accel_mutex_unlock(&context->state_mutex);
		abort();
	}
	context->reference_count--;
	if (context->reference_count == 0) {
		assert(context->resources_destroyed);
		if (!context->resources_destroyed) {
			accel_mutex_unlock(&context->state_mutex);
			abort();
		}
		destroy_shell = true;
	}

	accel_mutex_unlock(&context->state_mutex);

	/* Destroys synchronization only after every possible waiter released it. */
	if (destroy_shell) {
		accel_condition_destroy(&context->state_condition);
		accel_mutex_destroy(&context->state_mutex);
		noct_free(context);
	}
}

/*
 * Borrows a prepared program while the context state is locked.
 */
const struct accel_prepared_program *
accel_context_lookup_program_locked(
	const struct accel_context *context,
	uint32_t program_id)
{
	struct accel_registry_entry *entry;

	assert(context != NULL);

	if (program_id == 0)
		return NULL;
	if (program_id >= context->registry_capacity)
		return NULL;

	entry = context->registry[program_id];
	if (entry == NULL)
		return NULL;

	return &entry->program;
}

/*
 * Borrows a stable prepared program from the registry.
 */
const struct accel_prepared_program *
accel_context_lookup_program(
	struct accel_context *context,
	uint32_t program_id)
{
	const struct accel_prepared_program *program;

	if (context == NULL)
		return NULL;

	accel_mutex_lock(&context->state_mutex);
	program = accel_context_lookup_program_locked(context, program_id);
	accel_mutex_unlock(&context->state_mutex);

	return program;
}

/*
 * Borrows the selected backend's private state.
 */
void *
accel_context_get_backend_state(
	struct accel_context *context)
{
	if (context == NULL)
		return NULL;

	return context->backend_state;
}

/*
 * Registers the selected backend's private runtime package.
 */
bool
accel_context_register_runtime(
	struct accel_context *context,
	struct rt_env *env)
{
	if (context == NULL)
		return false;
	if (env == NULL)
		return false;

	return context->ops.register_runtime(context, env);
}

/*
 * Attaches the accelerator optimizer to the owning VM.
 */
void
accel_context_attach(
	struct accel_context *context)
{
	assert(context != NULL);
	assert(context->vm != NULL);

	accel_mutex_lock(&context->state_mutex);
	assert(!context->attached);
	assert(!context->shutting_down);
	assert(context->vm->accel_optimize_func == NULL);
	assert(context->vm->accel_optimize_userdata == NULL);

	if (context->attached ||
	    context->shutting_down ||
	    context->vm->accel_optimize_func != NULL ||
	    context->vm->accel_optimize_userdata != NULL) {
		accel_mutex_unlock(&context->state_mutex);
		abort();
	}

	context->vm->accel_optimize_func = accel_optimize_callback;
	context->vm->accel_optimize_userdata = context;
	context->attached = true;
	accel_mutex_unlock(&context->state_mutex);
}

/*
 * Detaches the accelerator optimizer from the owning VM.
 */
void
accel_context_detach(
	struct accel_context *context)
{
	assert(context != NULL);
	assert(context->vm != NULL);

	accel_mutex_lock(&context->state_mutex);
	assert(context->attached);
	assert(context->vm->accel_optimize_func == accel_optimize_callback);
	assert(context->vm->accel_optimize_userdata == context);

	if (!context->attached ||
	    context->vm->accel_optimize_func != accel_optimize_callback ||
	    context->vm->accel_optimize_userdata != context) {
		accel_mutex_unlock(&context->state_mutex);
		abort();
	}

	context->vm->accel_optimize_func = NULL;
	context->vm->accel_optimize_userdata = NULL;
	context->attached = false;
	context->shutting_down = true;
	accel_mutex_unlock(&context->state_mutex);
}

/*
 * Links a backend session while the context state is locked.
 */
void
accel_context_link_session_locked(
	struct accel_context *context,
	struct accel_live_session *session)
{
	assert(context != NULL);
	assert(session != NULL);
	assert(session->orphan_locked != NULL);
	assert(session->destroy_orphan != NULL);
	assert(!session->linked);
	assert(session->prev == NULL);
	assert(session->next == NULL);

	if (context == NULL ||
	    session == NULL ||
	    session->orphan_locked == NULL ||
	    session->destroy_orphan == NULL ||
	    session->linked ||
	    session->prev != NULL ||
	    session->next != NULL) {
		abort();
	}

	session->prev = context->live_session_tail;
	if (context->live_session_tail == NULL)
		context->live_session_head = session;
	else
		context->live_session_tail->next = session;

	context->live_session_tail = session;
	session->linked = true;
}

/*
 * Unlinks a backend session while the context state is locked.
 */
void
accel_context_unlink_session_locked(
	struct accel_context *context,
	struct accel_live_session *session)
{
	assert(context != NULL);
	assert(session != NULL);
	assert(session->linked);
	assert(session->prev != NULL || context->live_session_head == session);
	assert(session->next != NULL || context->live_session_tail == session);

	if (context == NULL ||
	    session == NULL ||
	    !session->linked)
		abort();
	if (session->prev == NULL && context->live_session_head != session)
		abort();
	if (session->next == NULL && context->live_session_tail != session)
		abort();

	if (session->prev == NULL)
		context->live_session_head = session->next;
	else
		session->prev->next = session->next;

	if (session->next == NULL)
		context->live_session_tail = session->prev;
	else
		session->next->prev = session->prev;

	session->prev = NULL;
	session->next = NULL;
	session->linked = false;
}

/*
 * Destroys a detached context and all backend-owned resources.
 */
void
accel_context_destroy(
	struct accel_context *context)
{
	struct accel_registry_entry *entry;
	struct accel_live_session *session;
	void (*destroy_orphan)(void *payload);
	void *orphan;
	uint32_t i;

	/* Accepts destruction of an optional context. */
	if (context == NULL)
		return;

	/* Closes the claim gate and drains every accepted backend operation. */
	accel_mutex_lock(&context->state_mutex);
	assert(!context->attached);
	if (context->attached) {
		accel_mutex_unlock(&context->state_mutex);
		abort();
	}
	if (context->destroying || context->resources_destroyed) {
		accel_mutex_unlock(&context->state_mutex);
		abort();
	}
	context->destroying = true;
	context->shutting_down = true;
	while (context->active_operation_count != 0) {
		accel_condition_wait(
			&context->state_condition,
			&context->state_mutex);
	}

	/* Detaches each payload before releasing it outside the state mutex. */
	while (context->live_session_head != NULL) {
		session = context->live_session_head;
		destroy_orphan = session->destroy_orphan;
		accel_context_unlink_session_locked(context, session);
		orphan = session->orphan_locked(session);
		accel_condition_wake_all(&context->state_condition);
		accel_mutex_unlock(&context->state_mutex);

		if (orphan != NULL)
			destroy_orphan(orphan);

		accel_mutex_lock(&context->state_mutex);
	}

	accel_mutex_unlock(&context->state_mutex);

	/* Destroy every immutable program payload before backend state. */
	for (i = 0; i < context->registry_capacity; i++) {
		entry = context->registry[i];
		if (entry == NULL)
			continue;

		context->ops.destroy_prepared_program(
			context->backend_state,
			&entry->program);
		noct_free(entry);
	}

	noct_free(context->registry);
	context->ops.destroy_backend_state(context->backend_state);

	/* Publishes backend teardown before releasing the owner's shell reference. */
	accel_mutex_lock(&context->state_mutex);
	context->registry = NULL;
	context->registry_capacity = 0;
	context->backend_state = NULL;
	context->resources_destroyed = true;
	accel_mutex_unlock(&context->state_mutex);

	accel_context_release(context);
}

/* Validate the complete backend operation table. */
static bool
accel_context_ops_valid(
	const struct accel_backend_ops *ops)
{
	if (ops == NULL)
		return false;
	if (ops->prepare_program == NULL)
		return false;
	if (ops->destroy_prepared_program == NULL)
		return false;
	if (ops->register_runtime == NULL)
		return false;
	if (ops->destroy_backend_state == NULL)
		return false;

	return true;
}

/* Allocate publication wrappers and consume one contiguous ID range. */
static bool
accel_context_reserve_locked(
	struct accel_context *context,
	uint32_t count,
	struct accel_registry_reservation *reservation)
{
	uint32_t required_capacity;
	uint32_t last_id;
	uint32_t i;

	/* Computes and allocates every fallible part before consuming an ID. */
	if (!accel_context_reservation_range(
		context,
		count,
		&last_id,
		&required_capacity)) {
		return false;
	}
	if (!accel_context_grow_registry_locked(
		context,
		required_capacity)) {
		return false;
	}
	for (i = 0; i < count; i++) {
		reservation->entry[i] = noct_calloc(
			1,
			sizeof(*reservation->entry[i]));
		if (reservation->entry[i] == NULL)
			return false;
	}

	/* Assigns the stable IDs only after every wrapper is available. */
	for (i = 0; i < count; i++) {
		reservation->entry[i]->program_id =
			context->next_program_id + i;
	}
	context->next_program_id = last_id + 1;

	return true;
}

/* Compute the inclusive ID range and required table size. */
static bool
accel_context_reservation_range(
	struct accel_context *context,
	uint32_t count,
	uint32_t *last_id,
	uint32_t *required_capacity)
{
	uint32_t available;

	assert(context != NULL);
	assert(count != 0);
	assert(last_id != NULL);
	assert(required_capacity != NULL);

	if (context->next_program_id == 0)
		return false;
	if (context->next_program_id > (uint32_t)INT_MAX)
		return false;

	available = (uint32_t)INT_MAX - context->next_program_id + 1;
	if (count > available)
		return false;

	*last_id = context->next_program_id + count - 1;
	*required_capacity = *last_id + 1;

	return true;
}

/* Grow the registry pointer table without moving published entries. */
static bool
accel_context_grow_registry_locked(
	struct accel_context *context,
	uint32_t required_capacity)
{
	struct accel_registry_entry **registry;
	uint32_t capacity;
	size_t size;
	size_t clear_size;

	assert(context != NULL);

	if (required_capacity <= context->registry_capacity)
		return true;

	capacity = context->registry_capacity;
	if (capacity < 8)
		capacity = 8;

	/* Double the table until the reserved ID range fits. */
	while (capacity < required_capacity) {
		if (capacity > (uint32_t)INT_MAX / 2) {
			capacity = required_capacity;
			break;
		}
		capacity *= 2;
	}

	size = (size_t)capacity * sizeof(*registry);
	if (capacity != 0 && size / sizeof(*registry) != capacity)
		return false;

	registry = noct_realloc(context->registry, size);
	if (registry == NULL)
		return false;

	clear_size = (size_t)(capacity - context->registry_capacity) *
		sizeof(*registry);
	memset(
		&registry[context->registry_capacity],
		0,
		clear_size);
	context->registry = registry;
	context->registry_capacity = capacity;

	return true;
}

/* Release every wrapper still owned by an unpublished reservation. */
static void
accel_context_free_reservation(
	struct accel_registry_reservation *reservation)
{
	uint32_t i;

	if (reservation == NULL)
		return;

	/* Release only wrappers not transferred into the registry. */
	for (i = 0; i < reservation->count; i++)
		noct_free(reservation->entry[i]);

	noct_free(reservation->entry);
	noct_free(reservation);
}

/* Revalidate reservation ownership and every unpublished table slot. */
static bool
accel_context_reservation_valid_locked(
	const struct accel_context *context,
	const struct accel_registry_reservation *reservation)
{
	const struct accel_registry_entry *entry;
	uint32_t i;

	if (!context->attached)
		return false;
	if (reservation->context != context)
		return false;
	if (reservation->count == 0)
		return false;
	if (reservation->entry == NULL)
		return false;

	/* Ensure every reserved wrapper still names an empty table slot. */
	for (i = 0; i < reservation->count; i++) {
		entry = reservation->entry[i];
		if (entry == NULL)
			return false;
		if (entry->program_id == 0)
			return false;
		if (entry->program_id >= context->registry_capacity)
			return false;
		if (context->registry[entry->program_id] != NULL)
			return false;
	}

	return true;
}
