/* -*- coding: utf-8; tab-width: 8; indent-tabs-mode: t; -*- */

/*
 * Noct Programming Language
 * Copyright (C) 2025, 2026 Awe Morris
 *
 * SPDX-License-Identifier: Zlib
 */

/*
 * Private Direct3D 12 accelerator backend.
 */

#ifndef NOCT_ACCEL_DX12_H
#define NOCT_ACCEL_DX12_H

#include "accel_backend.h"
#include "accel_device.h"

/*
 * Enumerates suitable non-software Direct3D 12 adapters.
 */
bool
accel_dx12_enumerate(
	struct accel_device_list *list,
	char *error,
	size_t error_size);

/*
 * Creates a Direct3D 12 backend for one selected adapter record.
 */
bool
accel_dx12_create_selected(
	struct rt_env *env,
	const struct accel_device *device,
	const struct accel_backend_ops **ops,
	void **backend_state);

#endif
