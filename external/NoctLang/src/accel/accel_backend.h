/* -*- coding: utf-8; tab-width: 8; indent-tabs-mode: t; -*- */

/*
 * Noct Programming Language
 * Copyright (c) 2025, 2026, Awe Morris
 */

/*
 * Private accelerator backend contracts.
 */

#ifndef NOCT_ACCEL_BACKEND_H
#define NOCT_ACCEL_BACKEND_H

#include "accel_program.h"

struct accel_context;
struct rt_env;

struct accel_prepared_program {
	void *payload;
};

struct accel_backend_ops {
	enum accel_compile_status (*prepare_program)(
		void *backend_state,
		const struct accel_program *program,
		struct accel_prepared_program *result);
	void (*destroy_prepared_program)(
		void *backend_state,
		struct accel_prepared_program *program);
	bool (*register_runtime)(
		struct accel_context *context,
		struct rt_env *env);
	void (*destroy_backend_state)(void *backend_state);
};

#endif
