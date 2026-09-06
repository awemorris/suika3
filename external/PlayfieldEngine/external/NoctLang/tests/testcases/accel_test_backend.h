/* -*- coding: utf-8; tab-width: 8; indent-tabs-mode: t; -*- */

/*
 * Noct Programming Language
 * Copyright (c) 2025, 2026, Awe Morris
 */

#ifndef NOCT_ACCEL_TEST_BACKEND_H
#define NOCT_ACCEL_TEST_BACKEND_H

#include "accel_backend.h"

struct accel_test_backend_observer {
	enum accel_compile_status prepare_status;
	uint32_t prepare_count;
	uint32_t destroy_program_count;
	uint32_t register_count;
	uint32_t destroy_state_count;
};

/*
 * Creates fake backend state and its complete private operation table.
 */
bool
accel_test_backend_create(
	struct accel_test_backend_observer *observer,
	struct accel_backend_ops *ops,
	void **backend_state);

/*
 * Borrows the deep-copied program stored in one fake prepared payload.
 */
const struct accel_program *
accel_test_backend_get_program(
	const struct accel_prepared_program *prepared);

#endif
