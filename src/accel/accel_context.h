/* -*- coding: utf-8; tab-width: 8; indent-tabs-mode: t; -*- */

/*
 * Noct Programming Language
 * Copyright (c) 2025, 2026, Awe Morris
 */

/*
 * VM-local accelerator context and program registry.
 */

#ifndef NOCT_ACCEL_CONTEXT_H
#define NOCT_ACCEL_CONTEXT_H

#include "accel_backend.h"
#include "accel_mutex.h"

struct accel_registry_entry;
struct accel_registry_reservation;
struct rt_vm;

struct accel_live_session {
	struct accel_live_session *prev;
	struct accel_live_session *next;
	void *(*orphan_locked)(struct accel_live_session *session);
	void (*destroy_orphan)(void *payload);
	bool linked;
};

struct accel_context {
	struct rt_vm *vm;
	struct accel_backend_ops ops;
	void *backend_state;
	struct accel_mutex state_mutex;
	struct accel_condition state_condition;
	struct accel_registry_entry **registry;
	uint32_t registry_capacity;
	uint32_t next_program_id;
	uint32_t reference_count;
	uint32_t active_operation_count;
	struct accel_live_session *live_session_head;
	struct accel_live_session *live_session_tail;
	bool attached;
	bool shutting_down;
	bool destroying;
	bool resources_destroyed;
};

struct accel_registry_commit_guard {
	struct accel_context *context;
	struct accel_registry_reservation *reservation;
	bool locked;
};

/*
 * Creates a detached VM-local accelerator context.
 */
bool
accel_context_create(
	struct rt_vm *vm,
	const struct accel_backend_ops *ops,
	void *backend_state,
	struct accel_context **result);

/*
 * Reserves stable program IDs and no-fail publication storage.
 */
bool
accel_context_reserve_programs(
	struct accel_context *context,
	uint32_t count,
	struct accel_registry_reservation **result);

/*
 * Returns one stable program ID from a registry reservation.
 */
uint32_t
accel_registry_reservation_get_id(
	const struct accel_registry_reservation *reservation,
	uint32_t index);

/*
 * Cancels an unpublished registry reservation.
 */
void
accel_context_cancel_reservation(
	struct accel_context *context,
	struct accel_registry_reservation *reservation);

/*
 * Locks and revalidates a registry commit reservation.
 */
bool
accel_context_lock_commit(
	struct accel_context *context,
	struct accel_registry_reservation *reservation,
	struct accel_registry_commit_guard *guard);

/*
 * Publishes prepared programs without allocation or failure.
 */
void
accel_context_publish_programs_locked(
	struct accel_registry_commit_guard *guard,
	struct accel_prepared_program program[]);

/*
 * Unlocks and clears a registry commit guard.
 */
void
accel_context_unlock_commit(
	struct accel_registry_commit_guard *guard);

/*
 * Locks the backend-neutral context state.
 */
void
accel_context_state_lock(
	struct accel_context *context);

/*
 * Unlocks the backend-neutral context state.
 */
void
accel_context_state_unlock(
	struct accel_context *context);

/*
 * Reports whether a state-locked context is attached to its VM.
 */
bool
accel_context_is_attached_locked(
	const struct accel_context *context);

/*
 * Claims the context until one external operation has finished.
 */
bool
accel_context_begin_operation(
	struct accel_context *context);

/*
 * Releases one previously claimed external operation.
 */
void
accel_context_end_operation(
	struct accel_context *context);

/*
 * Claims backend cleanup for one still-linked session finalizer.
 */
bool
accel_context_begin_session_cleanup(
	struct accel_context *context,
	struct accel_live_session *session);

/*
 * Retains the context shell for one non-owner wrapper.
 */
bool
accel_context_retain(
	struct accel_context *context);

/*
 * Releases one context-shell reference.
 */
void
accel_context_release(
	struct accel_context *context);

/*
 * Borrows a prepared program while the context state is locked.
 */
const struct accel_prepared_program *
accel_context_lookup_program_locked(
	const struct accel_context *context,
	uint32_t program_id);

/*
 * Borrows a stable prepared program from the registry.
 */
const struct accel_prepared_program *
accel_context_lookup_program(
	struct accel_context *context,
	uint32_t program_id);

/*
 * Borrows the selected backend's private state.
 */
void *
accel_context_get_backend_state(
	struct accel_context *context);

/*
 * Registers the selected backend's private runtime package.
 */
bool
accel_context_register_runtime(
	struct accel_context *context,
	struct rt_env *env);

/*
 * Attaches the accelerator optimizer to the owning VM.
 */
void
accel_context_attach(
	struct accel_context *context);

/*
 * Detaches the accelerator optimizer from the owning VM.
 */
void
accel_context_detach(
	struct accel_context *context);

/*
 * Links a backend session while the context state is locked.
 */
void
accel_context_link_session_locked(
	struct accel_context *context,
	struct accel_live_session *session);

/*
 * Unlinks a backend session while the context state is locked.
 */
void
accel_context_unlink_session_locked(
	struct accel_context *context,
	struct accel_live_session *session);

/*
 * Destroys a detached context and all backend-owned resources.
 */
void
accel_context_destroy(
	struct accel_context *context);

/*
 * Optimizes one accelerator-hinted function transactionally.
 */
bool
accel_optimize_callback(
	void *func_data,
	void *userdata);

#endif
