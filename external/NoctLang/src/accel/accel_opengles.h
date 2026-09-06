/* -*- coding: utf-8; tab-width: 8; indent-tabs-mode: t; -*- */

/*
 * Noct Programming Language
 * Copyright (C) 2025, 2026 Awe Morris
 *
 * SPDX-License-Identifier: Zlib
 */

/*
 * Private OpenGL ES accelerator backend.
 */

#ifndef NOCT_ACCEL_OPENGLES_H
#define NOCT_ACCEL_OPENGLES_H

#include "accel_backend.h"
#include "accel_device.h"

struct rt_env;

/*
 * Enumerates the suitable OpenGL ES compute device exposed by EGL.
 */
bool
accel_opengles_enumerate(
	struct accel_device_list *list,
	char *error,
	size_t error_size);

/*
 * Creates the OpenGL ES backend for one selected device record.
 */
bool
accel_opengles_create_selected(
	struct rt_env *env,
	const struct accel_device *device,
	const struct accel_backend_ops **ops,
	void **backend_state);

#endif
