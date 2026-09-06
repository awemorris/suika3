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

#ifndef NOCT_ACCEL_RUNTIME_H
#define NOCT_ACCEL_RUNTIME_H

#include "accel_backend.h"

#define ACCEL_RUNTIME_ERROR_SIZE	256

struct accel_context;
struct rt_env;

struct accel_runtime_buffer {
	int origin;
	uint32_t args_slot;
	int element_kind;
	uint32_t element_width;
	size_t element_count;
	size_t byte_count;
	bool active;
	bool upload;
	bool download;
	void *snapshot;
};

struct accel_executor_ops {
	const char *backend_display_name;
	const struct accel_program *(*get_program)(
		const struct accel_prepared_program *prepared);
	bool (*validate_dispatch_limit)(
		void *backend_state,
		const struct accel_prepared_program *prepared,
		uint32_t kernel_index,
		uint32_t start,
		uint32_t trip,
		char *error,
		size_t error_size);
	bool (*create_execution)(
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
		size_t error_size);
	bool (*dispatch_execution)(
		void *execution,
		uint32_t kernel_index,
		uint32_t start,
		uint32_t trip,
		char *error,
		size_t error_size);
	bool (*finish_execution)(
		void *execution,
		uint32_t result_word_count,
		uint32_t result_word[],
		uint32_t buffer_count,
		struct accel_runtime_buffer buffer[],
		char *error,
		size_t error_size);
	void (*destroy_execution)(void *execution);
};

/*
 * Registers the backend-neutral private accelerator package.
 *
 * The operation table is copied into VM-owned package metadata.  Buffer,
 * scalar, and scalar-result arrays passed to callbacks are borrowed only for
 * the duration of the callback.  Buffer snapshots remain owned by the runtime;
 * create callbacks read upload snapshots and scalar-result identities, and
 * finish callbacks synchronously fill scalar-result words and download
 * snapshots before returning.  Device-only buffers always carry a NULL
 * snapshot with upload and download disabled; active alone requests a device
 * resource.  A zero scalar-result count always carries a NULL array and does
 * not add a backend resource or shader binding.  Create, dispatch, and finish
 * callbacks run without the accelerator context state mutex and may use
 * backend-specific serialization.  Destroy callbacks receive only drained or
 * unsubmitted executions.  They must be short, must not acquire the
 * accelerator context state mutex, and must not print diagnostics.  A backend
 * may acquire its own serialization mutex when its API requires a current
 * context for cleanup; it must release that mutex before returning and must
 * not wait for GPU work.
 */
bool
accel_runtime_register(
	struct accel_context *context,
	struct rt_env *env,
	const struct accel_executor_ops *ops);

#endif
