/* -*- coding: utf-8; tab-width: 8; indent-tabs-mode: t; -*- */

/*
 * Noct Programming Language
 * Copyright (C) 2025, 2026 Awe Morris
 *
 * SPDX-License-Identifier: Zlib
 */

/*
 * Backend-neutral accelerator device registry tests.
 */

#include "accel_device.h"

#include <stdio.h>
#include <string.h>

static bool test_empty(void);
static bool test_selection(void);

/*
 * Runs the backend-neutral accelerator device registry tests.
 */
int
main(
	int argc,
	char *argv[])
{
	bool success;

	UNUSED_PARAMETER(argc);
	UNUSED_PARAMETER(argv);

	success = test_empty();

	/* Run the populated selection cases after the empty-list case. */
	if (success)
		success = test_selection();

	/* Report the first failed contract. */
	if (!success)
		return 1;

	printf("Accelerator device registry tests passed.\n");

	/* Report a successful test run. */
	return 0;
}

/* Verify that an initialized registry has a distinct empty result. */
static bool
test_empty(void)
{
	const struct accel_device *selected;
	struct accel_device_list list;
	enum accel_device_resolve_status status;

	accel_device_list_init(&list);
	selected = (const struct accel_device *)(uintptr_t)1;
	status = accel_device_list_resolve(&list, NULL, &selected);

	/* Require the empty result to clear the borrowed pointer. */
	if (status != ACCEL_DEVICE_EMPTY || selected != NULL) {
		accel_device_list_destroy(&list);
		return false;
	}

	accel_device_list_destroy(&list);

	/* Report a successful empty-list case. */
	return true;
}

/* Verify default, canonical, unique, missing, and ambiguous selection. */
static bool
test_selection(void)
{
	const struct accel_device *selected;
	struct accel_device_list list;
	enum accel_device_resolve_status status;
	char mutable_name[] = "Fast GPU";
	bool success;

	accel_device_list_init(&list);
	success = false;

	/* Append devices in stable backend enumeration order. */
	if (!accel_device_list_append(
		&list,
		ACCEL_BACKEND_VULKAN,
		"Shared GPU",
		400,
		100,
		(uintptr_t)1)) {
		accel_device_list_destroy(&list);
		return false;
	}
	if (!accel_device_list_append(
		&list,
		ACCEL_BACKEND_VULKAN,
		mutable_name,
		400,
		500,
		(uintptr_t)2)) {
		accel_device_list_destroy(&list);
		return false;
	}
	if (!accel_device_list_append(
		&list,
		ACCEL_BACKEND_OPENGLES,
		"Shared GPU",
		300,
		900,
		(uintptr_t)3)) {
		accel_device_list_destroy(&list);
		return false;
	}

	/* Prove that the registry owns the mutable input name. */
	mutable_name[0] = 'X';

	/* Choose the highest score within the highest-priority backend. */
	status = accel_device_list_resolve(&list, NULL, &selected);
	if (status != ACCEL_DEVICE_RESOLVED)
		success = false;
	else if (strcmp(selected->name, "Fast GPU") != 0)
		success = false;
	else
		success = true;

	/* Select one backend explicitly despite a duplicate plain name. */
	if (success) {
		status = accel_device_list_resolve(
			&list,
			"opengles:Shared GPU",
			&selected);
		if (status != ACCEL_DEVICE_RESOLVED)
			success = false;
		else if (selected->backend != ACCEL_BACKEND_OPENGLES)
			success = false;
	}

	/* Preserve compatibility for a unique plain display name. */
	if (success) {
		status = accel_device_list_resolve(
			&list,
			"Fast GPU",
			&selected);
		if (status != ACCEL_DEVICE_RESOLVED)
			success = false;
		else if (strcmp(selected->selector, "vulkan:Fast GPU") != 0)
			success = false;
	}

	/* Reject a plain name exposed by more than one backend. */
	if (success) {
		status = accel_device_list_resolve(
			&list,
			"Shared GPU",
			&selected);
		if (status != ACCEL_DEVICE_AMBIGUOUS || selected != NULL)
			success = false;
	}

	/* Reject a missing canonical selector without a borrowed result. */
	if (success) {
		status = accel_device_list_resolve(
			&list,
			"vulkan:Missing GPU",
			&selected);
		if (status != ACCEL_DEVICE_NOT_FOUND || selected != NULL)
			success = false;
	}

	/* Release every record after the last selection check. */
	accel_device_list_destroy(&list);

	/* Report the combined populated-list result. */
	return success;
}
