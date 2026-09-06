/* -*- coding: utf-8; tab-width: 8; indent-tabs-mode: t; -*- */

/*
 * Noct Programming Language
 * Copyright (c) 2025, 2026, Awe Morris
 */

/*
 * CLI-owned accelerator initialization transaction.
 */

#include "accel_cli.h"
#include "accel_context.h"
#include "accel_device.h"
#if defined(NOCT_ACCEL_BACKEND_DX12)
#include "accel_dx12.h"
#endif
#if defined(NOCT_ACCEL_BACKEND_METAL)
#include "accel_metal.h"
#endif
#if defined(NOCT_ACCEL_BACKEND_OPENGLES)
#include "accel_opengles.h"
#endif
#if defined(NOCT_ACCEL_BACKEND_VULKAN)
#include "accel_vulkan.h"
#endif
#include "runtime.h"

#include <stdlib.h>
#include <string.h>

#define ACCEL_ENUMERATION_ERROR_SIZE	256U

static bool accel_enumerate_all(struct accel_device_list *list, char *error, size_t error_size);
static void accel_save_enumeration_error(char *error, size_t error_size, const char *backend_error);
static void accel_clear_error(char *error, size_t error_size);

/*
 * Enumerates suitable devices without creating a VM accelerator.
 *
 * The visitor receives each canonical selector in stable backend order.
 */
bool
accel_list_devices(
	bool (*visitor)(const char *selector, void *userdata),
	void *userdata,
	char *error,
	size_t error_size,
	size_t *device_count)
{
	struct accel_device_list list;
	uint32_t i;

	/* Clear caller-owned outputs before validating the request. */
	accel_clear_error(error, error_size);
	if (device_count != NULL)
		*device_count = 0;

	/* Require both result consumers before allocating enumeration state. */
	if (visitor == NULL || device_count == NULL) {
		accel_save_enumeration_error(
			error,
			error_size,
			N_TR("Invalid accelerator device-list request."));
		return false;
	}

	/* Enumerate every backend into one deep-owned registry. */
	accel_device_list_init(&list);
	if (!accel_enumerate_all(&list, error, error_size)) {
		accel_device_list_destroy(&list);
		return false;
	}

	/* Publish each borrowed selector synchronously to the CLI visitor. */
	for (i = 0; i < list.count; i++) {
		if (!visitor(list.device[i].selector, userdata)) {
			accel_save_enumeration_error(
				error,
				error_size,
				N_TR("Accelerator device-list visitor failed."));
			accel_device_list_destroy(&list);
			return false;
		}
	}

	/* Publish the completed count before releasing borrowed selectors. */
	*device_count = (size_t)list.count;
	accel_device_list_destroy(&list);

	return true;
}

/*
 * Initializes the selected accelerator for one VM.
 *
 * A NULL selector chooses the default suitable device.  A non-NULL selector
 * may be canonical or an unambiguous UTF-8 display name.
 */
bool
accel_initialize(
	NoctVM *vm,
	NoctEnv *env,
	const char *gpu_selector)
{
	const struct accel_backend_ops *ops;
	const struct accel_device *device;
	struct accel_context *context;
	struct accel_device_list list;
	void *backend_state;
	char error[ACCEL_ENUMERATION_ERROR_SIZE];
	enum accel_device_resolve_status resolve_status;
	bool created;

	ops = NULL;
	device = NULL;
	context = NULL;
	backend_state = NULL;
	created = false;
	accel_device_list_init(&list);

	if (vm == NULL || env == NULL)
		return false;
	if (env->vm != vm) {
		rt_error(env, N_TR("Accelerator VM and environment do not match."));
		return false;
	}
	if (vm->accel_optimize_func != NULL ||
	    vm->accel_optimize_userdata != NULL) {
		rt_error(env, N_TR("An accelerator is already attached to this VM."));
		return false;
	}

	/* Enumerate once so selection policy remains backend-neutral. */
	if (!accel_enumerate_all(&list, error, sizeof(error))) {
		rt_error(env, N_TR("%s"), error);
		accel_device_list_destroy(&list);
		return false;
	}

	/* Resolve a default, canonical selector, or unique legacy plain name. */
	resolve_status = accel_device_list_resolve(
		&list,
		gpu_selector,
		&device);
	if (resolve_status == ACCEL_DEVICE_EMPTY) {
		rt_error(env, N_TR("No suitable GPU device is available."));
		accel_device_list_destroy(&list);
		return false;
	}
	if (resolve_status == ACCEL_DEVICE_NOT_FOUND) {
		rt_error(env, N_TR("The requested GPU device was not found."));
		accel_device_list_destroy(&list);
		return false;
	}
	if (resolve_status == ACCEL_DEVICE_AMBIGUOUS) {
		rt_error(env, N_TR("The requested GPU name is ambiguous; use a backend-qualified selector."));
		accel_device_list_destroy(&list);
		return false;
	}

	/* Create only the selected backend; do not hide initialization failure. */
	switch (device->backend) {
#if defined(NOCT_ACCEL_BACKEND_VULKAN)
	case ACCEL_BACKEND_VULKAN:
		created = accel_vulkan_create_selected(
			env,
			device,
			&ops,
			&backend_state);
		break;
#endif
#if defined(NOCT_ACCEL_BACKEND_OPENGLES)
	case ACCEL_BACKEND_OPENGLES:
		created = accel_opengles_create_selected(
			env,
			device,
			&ops,
			&backend_state);
		break;
#endif
#if defined(NOCT_ACCEL_BACKEND_DX12)
	case ACCEL_BACKEND_D3D12:
		created = accel_dx12_create_selected(
			env,
			device,
			&ops,
			&backend_state);
		break;
#endif
#if defined(NOCT_ACCEL_BACKEND_METAL)
	case ACCEL_BACKEND_METAL:
		created = accel_metal_create_selected(
			env,
			device,
			&ops,
			&backend_state);
		break;
#endif
	default:
		rt_error(env, N_TR("The selected GPU backend is unavailable."));
		break;
	}
	accel_device_list_destroy(&list);
	if (!created)
		return false;

	if (!accel_context_create(vm, ops, backend_state, &context)) {
		ops->destroy_backend_state(backend_state);
		rt_out_of_memory(env);
		return false;
	}
	backend_state = NULL;

	if (!accel_context_register_runtime(context, env)) {
		accel_context_destroy(context);
		return false;
	}

	accel_context_attach(context);

	return true;
}

/*
 * Detaches and destroys the CLI-owned accelerator before VM destruction.
 */
void
accel_finalize(
	NoctVM *vm)
{
	struct accel_context *context;

	if (vm == NULL)
		return;

	context = vm->accel_optimize_userdata;
	if (context == NULL)
		return;

	accel_context_detach(context);
	accel_context_destroy(context);
}

/* Enumerate every compiled backend while preserving the first diagnostic. */
static bool
accel_enumerate_all(
	struct accel_device_list *list,
	char *error,
	size_t error_size)
{
	char backend_error[ACCEL_ENUMERATION_ERROR_SIZE];
	bool any_backend_succeeded;
	bool succeeded;

	any_backend_succeeded = false;
	accel_clear_error(error, error_size);

#if defined(NOCT_ACCEL_BACKEND_VULKAN)
	/* Enumerate Vulkan first so it wins Linux and FreeBSD default ties. */
	backend_error[0] = '\0';
	succeeded = accel_vulkan_enumerate(
		list,
		backend_error,
		sizeof(backend_error));
	if (succeeded)
		any_backend_succeeded = true;
	else
		accel_save_enumeration_error(error, error_size, backend_error);
#endif

#if defined(NOCT_ACCEL_BACKEND_OPENGLES)
	/* Enumerate OpenGL ES after Vulkan as a distinct fallback backend. */
	backend_error[0] = '\0';
	succeeded = accel_opengles_enumerate(
		list,
		backend_error,
		sizeof(backend_error));
	if (succeeded)
		any_backend_succeeded = true;
	else
		accel_save_enumeration_error(error, error_size, backend_error);
#endif

#if defined(NOCT_ACCEL_BACKEND_DX12)
	/* Enumerate the platform-native Direct3D 12 adapters. */
	backend_error[0] = '\0';
	succeeded = accel_dx12_enumerate(
		list,
		backend_error,
		sizeof(backend_error));
	if (succeeded)
		any_backend_succeeded = true;
	else
		accel_save_enumeration_error(error, error_size, backend_error);
#endif

#if defined(NOCT_ACCEL_BACKEND_METAL)
	/* Enumerate the platform-native Metal devices. */
	backend_error[0] = '\0';
	succeeded = accel_metal_enumerate(
		list,
		backend_error,
		sizeof(backend_error));
	if (succeeded)
		any_backend_succeeded = true;
	else
		accel_save_enumeration_error(error, error_size, backend_error);
#endif

	/* Preserve a deterministic diagnostic if every backend failed. */
	if (!any_backend_succeeded) {
		if (error != NULL &&
		    error_size != 0 &&
		    error[0] == '\0') {
			accel_save_enumeration_error(
				error,
				error_size,
				N_TR("No accelerator backend could enumerate devices."));
		}
		return false;
	}

	/* Discard diagnostics from optional backends when one backend succeeded. */
	accel_clear_error(error, error_size);

	return true;
}

/* Preserve the first nonempty backend enumeration diagnostic. */
static void
accel_save_enumeration_error(
	char *error,
	size_t error_size,
	const char *backend_error)
{
	/* Ignore absent storage, an existing error, or an empty diagnostic. */
	if (error == NULL || error_size == 0)
		return;
	if (error[0] != '\0')
		return;
	if (backend_error == NULL || backend_error[0] == '\0')
		return;

	/* Copy a terminated diagnostic without transferring ownership. */
	strncpy(error, backend_error, error_size - 1);
	error[error_size - 1] = '\0';
}

/* Clear one optional diagnostic buffer. */
static void
accel_clear_error(
	char *error,
	size_t error_size)
{
	if (error != NULL && error_size != 0)
		error[0] = '\0';
}
