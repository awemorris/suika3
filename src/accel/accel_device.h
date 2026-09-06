/* -*- coding: utf-8; tab-width: 8; indent-tabs-mode: t; -*- */

/*
 * Noct Programming Language
 * Copyright (C) 2025, 2026 Awe Morris
 *
 * SPDX-License-Identifier: Zlib
 */

/*
 * Backend-neutral accelerator device records and selection.
 */

#ifndef NOCT_ACCEL_DEVICE_H
#define NOCT_ACCEL_DEVICE_H

#include <noct/noct.h>

enum accel_backend_kind {
	ACCEL_BACKEND_VULKAN,
	ACCEL_BACKEND_OPENGLES,
	ACCEL_BACKEND_D3D12,
	ACCEL_BACKEND_METAL
};

enum accel_device_resolve_status {
	ACCEL_DEVICE_RESOLVED,
	ACCEL_DEVICE_NOT_FOUND,
	ACCEL_DEVICE_AMBIGUOUS,
	ACCEL_DEVICE_EMPTY
};

struct accel_device {
	int backend;
	char *name;
	char *selector;
	uint32_t backend_priority;
	uint32_t score;
	uintptr_t identity;
};

struct accel_device_list {
	uint32_t count;
	uint32_t capacity;
	struct accel_device *device;
};

/*
 * Returns the canonical lower-case name of one accelerator backend.
 */
const char *
accel_backend_name(
	int backend);

/*
 * Initializes an empty accelerator device list.
 */
void
accel_device_list_init(
	struct accel_device_list *list);

/*
 * Destroys all strings and records owned by an accelerator device list.
 */
void
accel_device_list_destroy(
	struct accel_device_list *list);

/*
 * Appends one deep-copied accelerator device record.
 *
 * Identity is backend-private enumeration-session data.  Callers must not
 * assume that it remains valid after the backend enumeration session ends.
 */
bool
accel_device_list_append(
	struct accel_device_list *list,
	int backend,
	const char *name,
	uint32_t backend_priority,
	uint32_t score,
	uintptr_t identity);

/*
 * Resolves the default, canonical selector, or unique plain device name.
 *
 * A NULL requested selector chooses the highest backend priority and device
 * score, preserving list order for ties.  The returned record is borrowed
 * from the list.
 */
enum accel_device_resolve_status
accel_device_list_resolve(
	const struct accel_device_list *list,
	const char *requested,
	const struct accel_device **result);

#endif
