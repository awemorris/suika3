/* -*- coding: utf-8; tab-width: 8; indent-tabs-mode: t; -*- */

/*
 * Noct Programming Language
 * Copyright (C) 2025, 2026 Awe Morris
 *
 * SPDX-License-Identifier: Zlib
 */

/*
 * Private Metal accelerator backend.
 */

#ifndef NOCT_ACCEL_METAL_H
#define NOCT_ACCEL_METAL_H

#include "accel_backend.h"
#include "accel_device.h"

/*
 * Enumerates suitable hardware Metal devices.
 */
bool
accel_metal_enumerate(
	struct accel_device_list *list,
	char *error,
	size_t error_size);

/*
 * Creates a Metal backend for one selected device record.
 */
bool
accel_metal_create_selected(
	struct rt_env *env,
	const struct accel_device *device,
	const struct accel_backend_ops **ops,
	void **backend_state);

#endif
