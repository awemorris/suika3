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

#include "accel_device.h"

#include <stdlib.h>
#include <string.h>

#define ACCEL_DEVICE_INITIAL_CAPACITY	4U

static char *accel_device_duplicate(const char *text);
static char *accel_device_make_selector(const char *backend_name, const char *device_name);
static bool accel_device_list_reserve(struct accel_device_list *list);
static bool accel_device_has_canonical_prefix(const char *requested);

/*
 * Returns the canonical lower-case name of one accelerator backend.
 */
const char *
accel_backend_name(
	int backend)
{
	/* Select the stable CLI spelling for the backend. */
	switch (backend) {
	case ACCEL_BACKEND_VULKAN:
		return "vulkan";
	case ACCEL_BACKEND_OPENGLES:
		return "opengles";
	case ACCEL_BACKEND_D3D12:
		return "d3d12";
	case ACCEL_BACKEND_METAL:
		return "metal";
	default:
		return NULL;
	}
}

/*
 * Initializes an empty accelerator device list.
 */
void
accel_device_list_init(
	struct accel_device_list *list)
{
	/* Ignore an absent optional list. */
	if (list == NULL)
		return;

	/* Initialize every list field. */
	list->count = 0;
	list->capacity = 0;
	list->device = NULL;
}

/*
 * Destroys all strings and records owned by an accelerator device list.
 */
void
accel_device_list_destroy(
	struct accel_device_list *list)
{
	uint32_t i;

	/* Ignore an absent optional list. */
	if (list == NULL)
		return;

	/* Release every deep-owned record. */
	for (i = 0; i < list->count; i++) {
		noct_free(list->device[i].selector);
		noct_free(list->device[i].name);
	}

	/* Return the list to its initialized state. */
	noct_free(list->device);
	list->count = 0;
	list->capacity = 0;
	list->device = NULL;
}

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
	uintptr_t identity)
{
	const char *backend_name;
	struct accel_device record;

	/* Reject incomplete records before taking ownership. */
	if (list == NULL ||
	    name == NULL ||
	    name[0] == '\0')
		return false;

	backend_name = accel_backend_name(backend);

	/* Reject unknown backend identifiers. */
	if (backend_name == NULL)
		return false;

	/* Deep-copy the backend-local display name. */
	record.name = accel_device_duplicate(name);
	if (record.name == NULL)
		return false;

	/* Build the stable backend-qualified CLI selector. */
	record.selector = accel_device_make_selector(backend_name, name);
	if (record.selector == NULL) {
		noct_free(record.name);
		return false;
	}

	/* Grow the owning table before publishing the record. */
	if (!accel_device_list_reserve(list)) {
		noct_free(record.selector);
		noct_free(record.name);
		return false;
	}

	/* Publish the complete record in stable enumeration order. */
	record.backend = backend;
	record.backend_priority = backend_priority;
	record.score = score;
	record.identity = identity;
	list->device[list->count] = record;
	list->count++;

	/* Report successful ownership transfer. */
	return true;
}

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
	const struct accel_device **result)
{
	const struct accel_device *selected;
	uint32_t match_count;
	uint32_t i;
	bool canonical;

	/* Clear the borrowed result before validating the request. */
	if (result != NULL)
		*result = NULL;

	/* Treat an invalid list request as an unmatched selector. */
	if (list == NULL || result == NULL)
		return ACCEL_DEVICE_NOT_FOUND;

	/* Distinguish a valid empty registry from a missing name. */
	if (list->count == 0)
		return ACCEL_DEVICE_EMPTY;

	selected = NULL;

	/* Select the strongest default while preserving order for ties. */
	if (requested == NULL) {
		selected = &list->device[0];

		/* Compare every remaining default candidate. */
		for (i = 1; i < list->count; i++) {
			if (list->device[i].backend_priority >
			    selected->backend_priority) {
				selected = &list->device[i];
				continue;
			}
			if (list->device[i].backend_priority <
			    selected->backend_priority) {
				continue;
			}
			if (list->device[i].score > selected->score)
				selected = &list->device[i];
		}

		*result = selected;

		/* Report the selected default record. */
		return ACCEL_DEVICE_RESOLVED;
	}

	canonical = accel_device_has_canonical_prefix(requested);
	match_count = 0;

	/* Match either one canonical selector or one plain display name. */
	for (i = 0; i < list->count; i++) {
		if (canonical) {
			if (strcmp(list->device[i].selector, requested) != 0)
				continue;
		} else {
			if (strcmp(list->device[i].name, requested) != 0)
				continue;
		}

		selected = &list->device[i];
		match_count++;
	}

	/* Report a selector with no suitable device. */
	if (match_count == 0)
		return ACCEL_DEVICE_NOT_FOUND;

	/* Reject duplicate names or duplicate canonical selectors. */
	if (match_count > 1)
		return ACCEL_DEVICE_AMBIGUOUS;

	*result = selected;

	/* Report the unique selected record. */
	return ACCEL_DEVICE_RESOLVED;
}

/* Duplicate one string through the configured Noct allocator. */
static char *
accel_device_duplicate(
	const char *text)
{
	char *copy;
	size_t length;

	length = strlen(text);

	/* Reject an impossible size calculation. */
	if (length == (size_t)-1)
		return NULL;

	/* Allocate and copy the complete terminated string. */
	copy = noct_malloc(length + 1);
	if (copy == NULL)
		return NULL;

	memcpy(copy, text, length + 1);

	/* Return the owned copy. */
	return copy;
}

/* Build one backend-qualified device selector. */
static char *
accel_device_make_selector(
	const char *backend_name,
	const char *device_name)
{
	char *selector;
	size_t backend_length;
	size_t device_length;
	size_t length;

	backend_length = strlen(backend_name);
	device_length = strlen(device_name);

	/* Reject a selector length that cannot include punctuation and NUL. */
	if (device_length > (size_t)-1 - 2)
		return NULL;
	if (backend_length > (size_t)-1 - device_length - 2)
		return NULL;

	length = backend_length + device_length + 2;

	/* Allocate and assemble the canonical selector. */
	selector = noct_malloc(length);
	if (selector == NULL)
		return NULL;

	memcpy(selector, backend_name, backend_length);
	selector[backend_length] = ':';
	memcpy(
		selector + backend_length + 1,
		device_name,
		device_length + 1);

	/* Return the owned selector. */
	return selector;
}

/* Reserve one additional device record. */
static bool
accel_device_list_reserve(
	struct accel_device_list *list)
{
	struct accel_device *new_device;
	size_t byte_size;
	uint32_t new_capacity;

	/* Reuse an existing free table slot. */
	if (list->count < list->capacity)
		return true;

	/* Grow geometrically while checking integer and byte counts. */
	if (list->capacity == 0) {
		new_capacity = ACCEL_DEVICE_INITIAL_CAPACITY;
	} else {
		if (list->capacity > (uint32_t)-1 / 2U)
			return false;

		new_capacity = list->capacity * 2U;
	}

	/* Reject a byte count that cannot be represented by size_t. */
	byte_size = (size_t)new_capacity * sizeof(*new_device);
	if (byte_size / sizeof(*new_device) != (size_t)new_capacity)
		return false;

	/* Preserve the old table if allocation fails. */
	new_device = noct_realloc(
		list->device,
		byte_size);
	if (new_device == NULL)
		return false;

	list->device = new_device;
	list->capacity = new_capacity;

	/* Report an available record slot. */
	return true;
}

/* Detect a recognized backend-qualified selector prefix. */
static bool
accel_device_has_canonical_prefix(
	const char *requested)
{
	const char *backend_name;
	size_t length;
	int backend;

	/* Compare every public backend selector prefix. */
	for (backend = ACCEL_BACKEND_VULKAN;
	     backend <= ACCEL_BACKEND_METAL;
	     backend++) {
		backend_name = accel_backend_name(backend);
		length = strlen(backend_name);
		if (strncmp(requested, backend_name, length) != 0)
			continue;
		if (requested[length] == ':')
			return true;
	}

	/* Treat all other text as a backend-local display name. */
	return false;
}
