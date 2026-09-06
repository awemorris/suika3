/* -*- coding: utf-8; tab-width: 8; indent-tabs-mode: t; -*- */

/*
 * Noct Programming Language
 * Copyright (c) 2025, 2026, Awe Morris
 */

/*
 * Private CLI accelerator initialization boundary.
 */

#ifndef NOCT_ACCEL_CLI_H
#define NOCT_ACCEL_CLI_H

#include "noct.h"

#include <stddef.h>

/*
 * Enumerates suitable devices without creating a VM accelerator.
 *
 * The visitor borrows each canonical selector only for the duration of the
 * call.  A false visitor result stops enumeration and reports failure.
 */
bool
accel_list_devices(
	bool (*visitor)(const char *selector, void *userdata),
	void *userdata,
	char *error,
	size_t error_size,
	size_t *device_count);

/*
 * Initializes the selected accelerator for one VM.
 *
 * A NULL device name selects the default suitable device.  A non-NULL
 * name is matched exactly against the backend's UTF-8 display name.
 */
bool
accel_initialize(
	NoctVM *vm,
	NoctEnv *env,
	const char *gpu_name);

/* Detaches and releases the accelerator attached by accel_initialize(). */
void
accel_finalize(
	NoctVM *vm);

#endif
