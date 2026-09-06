/* -*- coding: utf-8; tab-width: 8; indent-tabs-mode: t; -*- */

/*
 * Noct Programming Language
 * Copyright (C) 2025, 2026 Awe Morris
 *
 * SPDX-License-Identifier: Zlib
 */

/*
 * Headless synchronous Direct3D 12 accelerator backend.
 */

#include "accel_dx12.h"
#include "accel_context.h"
#include "accel_mutex.h"
#include "accel_runtime.h"
#include "accel_shader_source.h"
#include "hir.h"
#include "runtime.h"

#define CINTERFACE
#define COBJMACROS
#if defined(__MINGW32__)
#define WIDL_C_INLINE_WRAPPERS
#endif
#include <windows.h>
#include <dxgi1_6.h>
#include <d3d12.h>
#include <d3dcompiler.h>

#include <assert.h>
#include <limits.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

/* Missing macro */
#ifndef D3D12_PS_CS_UAV_REGISTER_COUNT
#define D3D12_PS_CS_UAV_REGISTER_COUNT 8U
#endif

#define ACCEL_DX12_WORKGROUP_SIZE	64U
#define ACCEL_DX12_MAX_GROUPS		65535U
#define ACCEL_DX12_BACKEND_PRIORITY	400U
#define ACCEL_DX12_DRAIN_ATTEMPTS	3U
#define ACCEL_DX12_WAIT_POLL_MS		1000U

enum accel_dx12_submission_state {
	ACCEL_DX12_SUBMISSION_NONE,
	ACCEL_DX12_SUBMISSION_IN_FLIGHT,
	ACCEL_DX12_SUBMISSION_DRAINED,
	ACCEL_DX12_SUBMISSION_ABANDONED
};

struct accel_dx12_backend {
	IDXGIFactory1 *factory;
	IDXGIAdapter1 *adapter;
	ID3D12Device *device;
	ID3D12CommandQueue *queue;
	ID3D12Fence *fence;
	ID3D12Fence *drain_fence;
	UINT64 next_fence;
	struct accel_mutex queue_mutex;
	struct accel_dx12_execution *abandoned_execution_head;
};

struct accel_dx12_kernel {
	ID3D12RootSignature *root_signature;
	ID3D12PipelineState *pipeline;
};

struct accel_dx12_prepared {
	struct accel_program *program;
	struct accel_dx12_kernel *kernel;
	uint32_t kernel_count;
};

struct accel_dx12_buffer {
	ID3D12Resource *resource;
	UINT64 allocation_size;
	D3D12_RESOURCE_STATES state;
};

struct accel_dx12_execution_buffer {
	struct accel_dx12_buffer device;
	struct accel_dx12_buffer upload;
	struct accel_dx12_buffer readback;
	uint32_t args_slot;
	int element_kind;
	uint32_t element_width;
	size_t element_count;
	size_t byte_count;
	int origin;
	bool active;
	bool upload_required;
	bool download;
};

struct accel_dx12_execution {
	struct accel_dx12_backend *backend;
	ID3D12Device *retained_device;
	ID3D12CommandQueue *retained_queue;
	ID3D12Fence *retained_fence;
	struct accel_dx12_kernel *kernel;
	ID3D12CommandAllocator *allocator;
	ID3D12GraphicsCommandList *command_list;
	ID3D12DescriptorHeap *descriptor_heap;
	struct accel_dx12_execution_buffer *buffer;
	struct accel_dx12_buffer scalar_device;
	struct accel_dx12_buffer scalar_upload;
	struct accel_dx12_buffer result_device;
	struct accel_dx12_buffer result_upload;
	struct accel_dx12_buffer result_readback;
	HANDLE fence_event[ACCEL_DX12_DRAIN_ATTEMPTS];
	UINT64 fence_value;
	uint32_t kernel_count;
	uint32_t buffer_count;
	uint32_t result_word_count;
	enum accel_dx12_submission_state submission_state;
	bool fence_signalled;
	bool recording;
	bool has_active_dispatch;
	bool abandoned_linked;
	struct accel_dx12_execution *next_abandoned;
};

static bool accel_dx12_error(char *error, size_t error_size, const char *message);
static char *accel_dx12_utf8_name(const WCHAR *source);
static void accel_dx12_rollback_devices(struct accel_device_list *list, uint32_t count);
static uint32_t accel_dx12_device_score(const DXGI_ADAPTER_DESC1 *description);
static bool accel_dx12_adapter_suitable(IDXGIAdapter1 *adapter, DXGI_ADAPTER_DESC1 *description);
static IDXGIAdapter1 *accel_dx12_find_adapter(IDXGIFactory1 *factory, const char *name);
static bool accel_dx12_create_backend_objects(struct rt_env *env, struct accel_dx12_backend *backend);
static const struct accel_backend_ops *accel_dx12_backend_ops(void);
static const struct accel_executor_ops *accel_dx12_executor_ops(void);
static enum accel_compile_status accel_dx12_prepare_program(void *backend_state, const struct accel_program *program, struct accel_prepared_program *result);
static bool accel_dx12_program_uses_f32(const struct accel_program *program);
static bool accel_dx12_prepare_kernel(struct accel_dx12_backend *backend, const struct accel_program *program, uint32_t kernel_index, struct accel_dx12_kernel *result);
static void accel_dx12_destroy_kernel(struct accel_dx12_kernel *kernel);
static void accel_dx12_destroy_prepared_program(void *backend_state, struct accel_prepared_program *program);
static bool accel_dx12_register_runtime(struct accel_context *context, struct rt_env *env);
static void accel_dx12_destroy_backend_state(void *backend_state);
static const struct accel_program *accel_dx12_get_program(const struct accel_prepared_program *prepared);
static bool accel_dx12_validate_dispatch_limit(void *backend_state, const struct accel_prepared_program *prepared, uint32_t kernel_index, uint32_t start, uint32_t trip, char *error, size_t error_size);
static bool accel_dx12_create_execution(void *backend_state, const struct accel_prepared_program *prepared, uint32_t scalar_word_count, const uint32_t scalar_word[], uint32_t result_word_count, const uint32_t result_word[], uint32_t buffer_count, const struct accel_runtime_buffer buffer[], void **execution, char *error, size_t error_size);
static bool accel_dx12_dispatch_execution(void *execution, uint32_t kernel_index, uint32_t start, uint32_t trip, char *error, size_t error_size);
static bool accel_dx12_finish_execution(void *execution, uint32_t result_word_count, uint32_t result_word[], uint32_t buffer_count, struct accel_runtime_buffer buffer[], char *error, size_t error_size);
static void accel_dx12_destroy_execution(void *execution);
static bool accel_dx12_retain_execution_objects(struct accel_dx12_execution *execution, const struct accel_dx12_prepared *prepared, char *error, size_t error_size);
static void accel_dx12_release_execution_objects(struct accel_dx12_execution *execution);
static void accel_dx12_abandon_execution_objects(struct accel_dx12_execution *execution);
static void accel_dx12_destroy_safe_execution(struct accel_dx12_execution *execution);
static bool accel_dx12_drain_backend(struct accel_dx12_backend *backend);
static bool accel_dx12_backend_device_removed(struct accel_dx12_backend *backend);
static void accel_dx12_release_backend_drain(struct accel_dx12_backend *backend);
static void accel_dx12_destroy_abandoned_executions(struct accel_dx12_backend *backend);
static bool accel_dx12_recheck_submission(struct accel_dx12_execution *execution);
static bool accel_dx12_create_command(struct accel_dx12_execution *execution, char *error, size_t error_size);
static bool accel_dx12_has_active_dispatch(const struct accel_program *program, const uint32_t scalar_word[]);
static bool accel_dx12_create_execution_metadata(struct accel_dx12_execution *execution, const struct accel_program *program, uint32_t buffer_count, const struct accel_runtime_buffer buffer[], char *error, size_t error_size);
static bool accel_dx12_read_data_buffer_metadata(bool has_active_dispatch, struct accel_dx12_execution_buffer *result, const struct accel_runtime_buffer *runtime_buffer, char *error, size_t error_size);
static bool accel_dx12_validate_finish_metadata(const struct accel_dx12_execution *execution, uint32_t buffer_count, const struct accel_runtime_buffer buffer[], char *error, size_t error_size);
static bool accel_dx12_create_execution_buffers(struct accel_dx12_execution *execution, uint32_t scalar_word_count, const uint32_t scalar_word[], uint32_t result_word_count, const uint32_t result_word[], uint32_t buffer_count, const struct accel_runtime_buffer buffer[], char *error, size_t error_size);
static bool accel_dx12_create_data_buffer(struct accel_dx12_execution *execution, uint32_t buffer_index, const struct accel_runtime_buffer *runtime_buffer, char *error, size_t error_size);
static bool accel_dx12_create_scalar_buffer(struct accel_dx12_execution *execution, uint32_t scalar_word_count, const uint32_t scalar_word[], char *error, size_t error_size);
static bool accel_dx12_create_result_buffer(struct accel_dx12_execution *execution, uint32_t result_word_count, const uint32_t result_word[], char *error, size_t error_size);
static bool accel_dx12_create_descriptors(struct accel_dx12_execution *execution, char *error, size_t error_size);
static bool accel_dx12_write_descriptor(struct accel_dx12_execution *execution, uint32_t index, const struct accel_dx12_buffer *buffer, char *error, size_t error_size);
static bool accel_dx12_create_buffer(struct accel_dx12_backend *backend, size_t byte_count, D3D12_HEAP_TYPE heap_type, D3D12_RESOURCE_STATES state, bool unordered_access, struct accel_dx12_buffer *result, char *error, size_t error_size);
static void accel_dx12_destroy_buffer(struct accel_dx12_buffer *buffer);
static bool accel_dx12_upload_buffer(struct accel_dx12_execution *execution, struct accel_dx12_buffer *destination, struct accel_dx12_buffer *upload, const void *data, size_t byte_count, char *error, size_t error_size);
static void accel_dx12_transition(struct accel_dx12_execution *execution, struct accel_dx12_buffer *buffer, D3D12_RESOURCE_STATES state);
static void accel_dx12_uav_barrier(struct accel_dx12_execution *execution);
static bool accel_dx12_record_downloads(struct accel_dx12_execution *execution, uint32_t result_word_count, uint32_t result_word[], uint32_t buffer_count, struct accel_runtime_buffer buffer[], char *error, size_t error_size);
static bool accel_dx12_submit_and_wait(struct accel_dx12_execution *execution, char *error, size_t error_size);
static bool accel_dx12_signal_submission(struct accel_dx12_execution *execution);
static bool accel_dx12_wait_submission(struct accel_dx12_execution *execution);
static bool accel_dx12_copy_downloads(struct accel_dx12_execution *execution, uint32_t result_word_count, uint32_t result_word[], uint32_t buffer_count, struct accel_runtime_buffer buffer[], char *error, size_t error_size);

/*
 * Enumerates suitable non-software Direct3D 12 adapters.
 */
bool
accel_dx12_enumerate(
	struct accel_device_list *list,
	char *error,
	size_t error_size)
{
	IDXGIFactory1 *factory;
	IDXGIAdapter1 *adapter;
	DXGI_ADAPTER_DESC1 description;
	char *name;
	HRESULT result;
	UINT index;
	uint32_t initial_count;
	bool appended;

	/* Reject an invalid destination before opening DXGI. */
	if (list == NULL)
		return accel_dx12_error(error, error_size, N_TR("invalid Direct3D 12 device list"));

	initial_count = list->count;
	factory = NULL;

	/* Create one temporary factory for adapter enumeration. */
	result = CreateDXGIFactory1(
		&IID_IDXGIFactory1,
		(void **)&factory);
	if (FAILED(result))
		return accel_dx12_error(error, error_size, N_TR("failed to create the DXGI factory"));

	/* Append every hardware adapter that can create a D3D12 device. */
	for (index = 0; ; index++) {
		adapter = NULL;
		result = IDXGIFactory1_EnumAdapters1(factory, index, &adapter);
		if (result == DXGI_ERROR_NOT_FOUND)
			break;
		if (FAILED(result) || adapter == NULL) {
			accel_dx12_rollback_devices(list, initial_count);
			IDXGIFactory1_Release(factory);
			return accel_dx12_error(error, error_size, N_TR("failed to enumerate DXGI adapters"));
		}

		/* Ignore software and non-D3D12 adapters. */
		if (!accel_dx12_adapter_suitable(adapter, &description)) {
			IDXGIAdapter1_Release(adapter);
			continue;
		}

		/* Convert and deep-copy the exact DXGI display name. */
		name = accel_dx12_utf8_name(description.Description);
		if (name == NULL) {
			IDXGIAdapter1_Release(adapter);
			accel_dx12_rollback_devices(list, initial_count);
			IDXGIFactory1_Release(factory);
			return accel_dx12_error(error, error_size, N_TR("failed to convert a DXGI adapter name"));
		}

		appended = accel_device_list_append(
			list,
			ACCEL_BACKEND_D3D12,
			name,
			ACCEL_DX12_BACKEND_PRIORITY,
			accel_dx12_device_score(&description),
			(uintptr_t)index);
		noct_free(name);
		IDXGIAdapter1_Release(adapter);
		if (!appended) {
			accel_dx12_rollback_devices(list, initial_count);
			IDXGIFactory1_Release(factory);
			return accel_dx12_error(error, error_size, N_TR("out of memory while recording DXGI adapters"));
		}
	}

	/* Close the temporary factory after all records are deep-owned. */
	IDXGIFactory1_Release(factory);

	/* Clear an optional stale diagnostic on success. */
	if (error != NULL && error_size != 0)
		error[0] = '\0';

	/* Report a complete enumeration, including an empty result. */
	return true;
}

/*
 * Creates a Direct3D 12 backend for one selected adapter record.
 */
bool
accel_dx12_create_selected(
	struct rt_env *env,
	const struct accel_device *device,
	const struct accel_backend_ops **ops,
	void **backend_state)
{
	struct accel_dx12_backend *backend;
	HRESULT result;

	/* Clear ownership outputs before validating the request. */
	if (ops != NULL)
		*ops = NULL;
	if (backend_state != NULL)
		*backend_state = NULL;

	/* Reject an incomplete ownership request. */
	if (env == NULL ||
	    ops == NULL ||
	    backend_state == NULL) {
		return false;
	}

	/* Require a deep-owned record from the D3D12 enumerator. */
	if (device == NULL ||
	    device->backend != ACCEL_BACKEND_D3D12 ||
	    device->name == NULL ||
	    device->name[0] == '\0') {
		rt_error(env, N_TR("Invalid selected Direct3D 12 device."));
		return false;
	}

	/* Allocate the complete backend owner before opening DXGI objects. */
	backend = noct_calloc(1, sizeof(*backend));
	if (backend == NULL) {
		rt_out_of_memory(env);
		return false;
	}

	/* Initialize serialization before any path can use common cleanup. */
	if (!accel_mutex_init(&backend->queue_mutex)) {
		rt_error(env, N_TR("Failed to initialize the Direct3D 12 queue mutex."));
		noct_free(backend);
		return false;
	}

	/* Create the retained factory used to re-resolve the selected name. */
	result = CreateDXGIFactory1(
		&IID_IDXGIFactory1,
		(void **)&backend->factory);
	if (FAILED(result)) {
		rt_error(env, N_TR("Failed to create the Direct3D 12 DXGI factory."));
		accel_dx12_destroy_backend_state(backend);
		return false;
	}

	/* Select the exact hardware adapter in the fresh factory session. */
	backend->adapter = accel_dx12_find_adapter(
		backend->factory,
		device->name);
	if (backend->adapter == NULL) {
		rt_error(env, N_TR("The selected Direct3D 12 device is unavailable."));
		accel_dx12_destroy_backend_state(backend);
		return false;
	}

	/* Create the selected device, compute queue, and completion fence. */
	if (!accel_dx12_create_backend_objects(env, backend)) {
		accel_dx12_destroy_backend_state(backend);
		return false;
	}

	/* Transfer the complete backend to the owning context. */
	*ops = accel_dx12_backend_ops();
	*backend_state = backend;

	/* Report successful backend publication. */
	return true;
}

/* Return the immutable Direct3D 12 backend operation table. */
static const struct accel_backend_ops *
accel_dx12_backend_ops(void)
{
	static const struct accel_backend_ops ops = {
		accel_dx12_prepare_program,
		accel_dx12_destroy_prepared_program,
		accel_dx12_register_runtime,
		accel_dx12_destroy_backend_state
	};

	/* Return the process-lifetime backend operations. */
	return &ops;
}

/* Return the immutable Direct3D 12 executor operation table. */
static const struct accel_executor_ops *
accel_dx12_executor_ops(void)
{
	static const struct accel_executor_ops ops = {
		"Direct3D 12",
		accel_dx12_get_program,
		accel_dx12_validate_dispatch_limit,
		accel_dx12_create_execution,
		accel_dx12_dispatch_execution,
		accel_dx12_finish_execution,
		accel_dx12_destroy_execution
	};

	/* Return the process-lifetime executor operations. */
	return &ops;
}

/* Copy one stable diagnostic into the caller's optional buffer. */
static bool
accel_dx12_error(
	char *error,
	size_t error_size,
	const char *message)
{
	/* Publish a terminated diagnostic when storage is available. */
	if (error != NULL && error_size != 0) {
		strncpy(error, message, error_size - 1);
		error[error_size - 1] = '\0';
	}

	/* Report the failed operation. */
	return false;
}

/* Convert one DXGI UTF-16 description into owned UTF-8. */
static char *
accel_dx12_utf8_name(
	const WCHAR *source)
{
	char *name;
	int length;
	int converted;

	/* Determine the exact terminated UTF-8 byte count. */
	length = WideCharToMultiByte(
		CP_UTF8,
		WC_ERR_INVALID_CHARS,
		source,
		-1,
		NULL,
		0,
		NULL,
		NULL);
	if (length <= 1)
		return NULL;

	/* Allocate the exact deep-owned adapter name. */
	name = noct_malloc((size_t)length);
	if (name == NULL)
		return NULL;

	/* Convert the complete string and reject partial failure. */
	converted = WideCharToMultiByte(
		CP_UTF8,
		WC_ERR_INVALID_CHARS,
		source,
		-1,
		name,
		length,
		NULL,
		NULL);
	if (converted != length) {
		noct_free(name);
		return NULL;
	}

	/* Return the exact owned UTF-8 name. */
	return name;
}

/* Remove only records appended by a failed D3D12 enumeration. */
static void
accel_dx12_rollback_devices(
	struct accel_device_list *list,
	uint32_t count)
{
	/* Release newly appended deep-owned records in reverse order. */
	while (list->count > count) {
		list->count--;
		noct_free(list->device[list->count].selector);
		noct_free(list->device[list->count].name);
		memset(
			&list->device[list->count],
			0,
			sizeof(list->device[list->count]));
	}
}

/* Score one suitable adapter by dedicated local memory. */
static uint32_t
accel_dx12_device_score(
	const DXGI_ADAPTER_DESC1 *description)
{
	UINT64 mebibytes;

	mebibytes = description->DedicatedVideoMemory / (1024U * 1024U);

	/* Saturate the backend-local score to its public word. */
	if (mebibytes > (uint32_t)-1)
		return (uint32_t)-1;

	/* Preserve exact memory ordering for ordinary adapters. */
	return (uint32_t)mebibytes;
}

/* Test one adapter without retaining a probe device. */
static bool
accel_dx12_adapter_suitable(
	IDXGIAdapter1 *adapter,
	DXGI_ADAPTER_DESC1 *description)
{
	ID3D12Device *device;
	HRESULT result;

	/* Read the immutable adapter description. */
	memset(description, 0, sizeof(*description));
	result = IDXGIAdapter1_GetDesc1(adapter, description);
	if (FAILED(result))
		return false;

	/* Reject software rasterizers from production selection. */
	if ((description->Flags & DXGI_ADAPTER_FLAG_SOFTWARE) != 0)
		return false;

	/* Probe the minimum D3D12 feature level used by this backend. */
	device = NULL;
	result = D3D12CreateDevice(
		(IUnknown *)adapter,
		D3D_FEATURE_LEVEL_11_0,
		&IID_ID3D12Device,
		(void **)&device);
	if (FAILED(result) || device == NULL) {
		if (device != NULL)
			ID3D12Device_Release(device);
		return false;
	}

	/* Release the probe immediately after proving suitability. */
	ID3D12Device_Release(device);

	/* Report a hardware D3D12 compute candidate. */
	return true;
}

/* Re-resolve one selected UTF-8 adapter name in a fresh factory. */
static IDXGIAdapter1 *
accel_dx12_find_adapter(
	IDXGIFactory1 *factory,
	const char *name)
{
	IDXGIAdapter1 *adapter;
	DXGI_ADAPTER_DESC1 description;
	char *candidate_name;
	HRESULT result;
	UINT index;
	bool matches;

	/* Inspect every suitable hardware adapter in factory order. */
	for (index = 0; ; index++) {
		adapter = NULL;
		result = IDXGIFactory1_EnumAdapters1(factory, index, &adapter);
		if (result == DXGI_ERROR_NOT_FOUND)
			return NULL;
		if (FAILED(result) || adapter == NULL)
			return NULL;

		/* Skip adapters outside the enumerator's suitability contract. */
		if (!accel_dx12_adapter_suitable(adapter, &description)) {
			IDXGIAdapter1_Release(adapter);
			continue;
		}

		/* Compare exact deep UTF-8 display names. */
		candidate_name = accel_dx12_utf8_name(description.Description);
		if (candidate_name == NULL) {
			IDXGIAdapter1_Release(adapter);
			return NULL;
		}

		matches = strcmp(candidate_name, name) == 0;
		noct_free(candidate_name);
		if (matches)
			return adapter;

		IDXGIAdapter1_Release(adapter);
	}
}

/* Create immutable device-wide execution objects. */
static bool
accel_dx12_create_backend_objects(
	struct rt_env *env,
	struct accel_dx12_backend *backend)
{
	D3D12_COMMAND_QUEUE_DESC queue_description;
	HRESULT result;

	/* Create the retained logical device for the selected adapter. */
	result = D3D12CreateDevice(
		(IUnknown *)backend->adapter,
		D3D_FEATURE_LEVEL_11_0,
		&IID_ID3D12Device,
		(void **)&backend->device);
	if (FAILED(result)) {
		rt_error(env, N_TR("Failed to create the selected Direct3D 12 device."));
		return false;
	}

	/* Create one compute-only command queue. */
	memset(&queue_description, 0, sizeof(queue_description));
	queue_description.Type = D3D12_COMMAND_LIST_TYPE_COMPUTE;
	queue_description.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
	result = ID3D12Device_CreateCommandQueue(
		backend->device,
		&queue_description,
		&IID_ID3D12CommandQueue,
		(void **)&backend->queue);
	if (FAILED(result)) {
		rt_error(env, N_TR("Failed to create the Direct3D 12 compute queue."));
		return false;
	}

	/* Create the monotonically signalled queue fence. */
	result = ID3D12Device_CreateFence(
		backend->device,
		0,
		D3D12_FENCE_FLAG_NONE,
		&IID_ID3D12Fence,
		(void **)&backend->fence);
	if (FAILED(result)) {
		rt_error(env, N_TR("Failed to create the Direct3D 12 queue fence."));
		return false;
	}

	backend->next_fence = 1;

	/* Report a complete device-wide execution context. */
	return true;
}

/* Prepare every immutable D3D12 compute pipeline for one program. */
static enum accel_compile_status
accel_dx12_prepare_program(
	void *backend_state,
	const struct accel_program *program,
	struct accel_prepared_program *result)
{
	struct accel_dx12_backend *backend;
	struct accel_dx12_prepared *prepared;
	char validation_error[128];
	uint32_t descriptor_count;
	uint32_t i;

	/* Clear the opaque publication slot before validating inputs. */
	if (result == NULL)
		return ACCEL_COMPILE_ERROR;

	result->payload = NULL;
	backend = backend_state;

	/* Reject an invalid compiler-to-backend boundary. */
	if (backend == NULL || program == NULL) {
		hir_error(0, N_TR("Invalid Direct3D 12 program preparation request."));
		return ACCEL_COMPILE_ERROR;
	}

	/* Validate the complete target-neutral program before translation. */
	if (!accel_program_validate(
		program,
		validation_error,
		sizeof(validation_error))) {
		hir_error(
			program->source_line,
			N_TR("Invalid accelerator program reached the Direct3D 12 backend."));
		return ACCEL_COMPILE_ERROR;
	}

	/* Restrict the current UAV-only ABI to the shader-model slot limit. */
	descriptor_count = program->buffer_count + 1;
	if (program->scalar_result_count != 0)
		descriptor_count++;
	if (descriptor_count > D3D12_PS_CS_UAV_REGISTER_COUNT)
		return ACCEL_COMPILE_DECLINED;

	/* Decline Float32 until device controls prove strict binary32 behavior. */
	if (accel_dx12_program_uses_f32(program))
		return ACCEL_COMPILE_DECLINED;

	/* Allocate the complete prepared-program owner. */
	prepared = noct_calloc(1, sizeof(*prepared));
	if (prepared == NULL) {
		hir_out_of_memory();
		return ACCEL_COMPILE_ERROR;
	}

	/* Clone target-neutral metadata retained by the runtime. */
	prepared->program = accel_program_clone(program);
	if (prepared->program == NULL) {
		noct_free(prepared);
		hir_out_of_memory();
		return ACCEL_COMPILE_ERROR;
	}

	/* Allocate one immutable pipeline owner per typed kernel. */
	prepared->kernel_count = program->kernel_count;
	prepared->kernel = noct_calloc(
		prepared->kernel_count,
		sizeof(*prepared->kernel));
	if (prepared->kernel == NULL) {
		accel_program_destroy(prepared->program);
		noct_free(prepared);
		hir_out_of_memory();
		return ACCEL_COMPILE_ERROR;
	}

	/* Compile every kernel before publishing any prepared payload. */
	for (i = 0; i < prepared->kernel_count; i++) {
		if (!accel_dx12_prepare_kernel(
			backend,
			program,
			i,
			&prepared->kernel[i])) {
			result->payload = prepared;
			accel_dx12_destroy_prepared_program(backend, result);
			return ACCEL_COMPILE_ERROR;
		}
	}

	/* Publish the fully prepared immutable program atomically. */
	result->payload = prepared;

	/* Report a backend-applied optimization. */
	return ACCEL_COMPILE_APPLIED;
}

/* Detect whether any binding or instruction uses Float32. */
static bool
accel_dx12_program_uses_f32(
	const struct accel_program *program)
{
	const struct accel_ir_kernel *kernel;
	uint32_t i;
	uint32_t j;

	/* Inspect every typed kernel and its complete binding namespace. */
	for (i = 0; i < program->kernel_count; i++) {
		kernel = program->kernel[i].ir;

		/* Check each buffer's raw-word interpretation. */
		for (j = 0; j < kernel->buffer_binding_count; j++) {
			if (kernel->buffer_value_type[j] == ACCEL_IR_F32)
				return true;
		}

		/* Check every SSA result type. */
		for (j = 0; j < kernel->instruction_count; j++) {
			if (kernel->instruction[j].result_type == ACCEL_IR_F32)
				return true;
		}
	}

	/* Report a word-exact integer program. */
	return false;
}

/* Compile one HLSL source and create its root signature and pipeline. */
static bool
accel_dx12_prepare_kernel(
	struct accel_dx12_backend *backend,
	const struct accel_program *program,
	uint32_t kernel_index,
	struct accel_dx12_kernel *result)
{
	struct accel_shader_source source;
	ID3DBlob *shader;
	ID3DBlob *compiler_error;
	ID3DBlob *signature;
	ID3DBlob *signature_error;
	D3D12_DESCRIPTOR_RANGE range;
	D3D12_ROOT_PARAMETER parameter;
	D3D12_ROOT_SIGNATURE_DESC root_description;
	D3D12_COMPUTE_PIPELINE_STATE_DESC pipeline_description;
	char source_error[128];
	const char *diagnostic;
	HRESULT hresult;
	uint32_t descriptor_count;

	/* Initialize every temporary and destination owner. */
	memset(&source, 0, sizeof(source));
	memset(result, 0, sizeof(*result));
	shader = NULL;
	compiler_error = NULL;
	signature = NULL;
	signature_error = NULL;

	/* Generate deterministic HLSL from the target-neutral typed kernel. */
	if (!accel_shader_source_generate(
		ACCEL_SHADER_SOURCE_HLSL,
		program,
		kernel_index,
		&source,
		source_error,
		sizeof(source_error))) {
		hir_error(program->kernel[kernel_index].source_line, source_error);
		return false;
	}

	/* Compile a broadly supported shader-model 5.1 compute entry point. */
	hresult = D3DCompile(
		source.data,
		source.length,
		program->kernel[kernel_index].ir->name,
		NULL,
		NULL,
		"main",
		"cs_5_1",
		D3DCOMPILE_ENABLE_STRICTNESS,
		0,
		&shader,
		&compiler_error);
	accel_shader_source_cleanup(&source);
	if (FAILED(hresult) || shader == NULL) {
		diagnostic = N_TR("Failed to compile a Direct3D 12 compute shader.");
		if (compiler_error != NULL)
			diagnostic = ID3D10Blob_GetBufferPointer(compiler_error);
		hir_error(program->kernel[kernel_index].source_line, diagnostic);
		if (compiler_error != NULL)
			ID3D10Blob_Release(compiler_error);
		if (shader != NULL)
			ID3D10Blob_Release(shader);
		return false;
	}

	/* Discard nonfatal compiler diagnostics after successful compilation. */
	if (compiler_error != NULL) {
		ID3D10Blob_Release(compiler_error);
		compiler_error = NULL;
	}

	/* Describe one contiguous UAV table for buffers and scalar words. */
	descriptor_count = program->buffer_count + 1;
	if (program->scalar_result_count != 0)
		descriptor_count++;
	memset(&range, 0, sizeof(range));
	range.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
	range.NumDescriptors = descriptor_count;
	range.BaseShaderRegister = 0;
	range.RegisterSpace = 0;
	range.OffsetInDescriptorsFromTableStart = 0;

	memset(&parameter, 0, sizeof(parameter));
	parameter.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	parameter.DescriptorTable.NumDescriptorRanges = 1;
	parameter.DescriptorTable.pDescriptorRanges = &range;
	parameter.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	memset(&root_description, 0, sizeof(root_description));
	root_description.NumParameters = 1;
	root_description.pParameters = &parameter;
	root_description.Flags = D3D12_ROOT_SIGNATURE_FLAG_NONE;

	/* Serialize the exact descriptor-table root signature. */
	hresult = D3D12SerializeRootSignature(
		&root_description,
		D3D_ROOT_SIGNATURE_VERSION_1,
		&signature,
		&signature_error);
	if (FAILED(hresult) || signature == NULL) {
		diagnostic = N_TR("Failed to serialize a Direct3D 12 root signature.");
		if (signature_error != NULL)
			diagnostic = ID3D10Blob_GetBufferPointer(signature_error);
		hir_error(program->kernel[kernel_index].source_line, diagnostic);
		if (signature_error != NULL)
			ID3D10Blob_Release(signature_error);
		if (signature != NULL)
			ID3D10Blob_Release(signature);
		ID3D10Blob_Release(shader);
		return false;
	}

	/* Discard nonfatal root-signature diagnostics. */
	if (signature_error != NULL) {
		ID3D10Blob_Release(signature_error);
		signature_error = NULL;
	}

	/* Create the immutable root-signature object. */
	hresult = ID3D12Device_CreateRootSignature(
		backend->device,
		0,
		ID3D10Blob_GetBufferPointer(signature),
		ID3D10Blob_GetBufferSize(signature),
		&IID_ID3D12RootSignature,
		(void **)&result->root_signature);
	ID3D10Blob_Release(signature);
	if (FAILED(hresult)) {
		hir_error(
			program->kernel[kernel_index].source_line,
			N_TR("Failed to create a Direct3D 12 root signature."));
		ID3D10Blob_Release(shader);
		accel_dx12_destroy_kernel(result);
		return false;
	}

	/* Create the immutable compute pipeline from the compiled bytecode. */
	memset(&pipeline_description, 0, sizeof(pipeline_description));
	pipeline_description.pRootSignature = result->root_signature;
	pipeline_description.CS.pShaderBytecode =
		ID3D10Blob_GetBufferPointer(shader);
	pipeline_description.CS.BytecodeLength =
		ID3D10Blob_GetBufferSize(shader);
	hresult = ID3D12Device_CreateComputePipelineState(
		backend->device,
		&pipeline_description,
		&IID_ID3D12PipelineState,
		(void **)&result->pipeline);
	ID3D10Blob_Release(shader);
	if (FAILED(hresult)) {
		hir_error(
			program->kernel[kernel_index].source_line,
			N_TR("Failed to create a Direct3D 12 compute pipeline."));
		accel_dx12_destroy_kernel(result);
		return false;
	}

	/* Report a complete immutable kernel pipeline. */
	return true;
}

/* Destroy one partially or fully prepared kernel. */
static void
accel_dx12_destroy_kernel(
	struct accel_dx12_kernel *kernel)
{
	/* Release the pipeline before its root signature. */
	if (kernel->pipeline != NULL)
		ID3D12PipelineState_Release(kernel->pipeline);
	if (kernel->root_signature != NULL)
		ID3D12RootSignature_Release(kernel->root_signature);

	/* Clear all stale COM handles. */
	memset(kernel, 0, sizeof(*kernel));
}

/* Destroy one backend-prepared program and clear its opaque slot. */
static void
accel_dx12_destroy_prepared_program(
	void *backend_state,
	struct accel_prepared_program *program)
{
	struct accel_dx12_prepared *prepared;
	uint32_t i;

	UNUSED_PARAMETER(backend_state);

	/* Accept cleanup of an optional or already-cleared owner. */
	if (program == NULL || program->payload == NULL)
		return;

	prepared = program->payload;

	/* Release every immutable pipeline before program metadata. */
	for (i = 0; i < prepared->kernel_count; i++)
		accel_dx12_destroy_kernel(&prepared->kernel[i]);

	accel_program_destroy(prepared->program);
	noct_free(prepared->kernel);
	noct_free(prepared);
	program->payload = NULL;
}

/* Register the backend-neutral private accelerator runtime package. */
static bool
accel_dx12_register_runtime(
	struct accel_context *context,
	struct rt_env *env)
{
	bool success;

	/* Register the copied executor table with the owning VM context. */
	success = accel_runtime_register(
		context,
		env,
		accel_dx12_executor_ops());

	/* Report the runtime registration result. */
	return success;
}

/* Destroy the selected D3D12 device-wide execution state. */
static void
accel_dx12_destroy_backend_state(
	void *backend_state)
{
	struct accel_dx12_backend *backend;
	bool drained;

	backend = backend_state;

	/* Accept cleanup of an optional backend. */
	if (backend == NULL)
		return;

	/* Drain submissions retained after an unprovable callback failure. */
	drained = accel_dx12_drain_backend(backend);
	if (!drained) {
		/*
		 * Retain the complete backend when D3D12 cannot prove that queued
		 * commands stopped referencing their COM object graph.
		 */
		return;
	}

	/* Release every execution made safe by the device-wide drain. */
	accel_dx12_destroy_abandoned_executions(backend);

	/* Release synchronization before its queue and device. */
	accel_mutex_destroy(&backend->queue_mutex);
	if (backend->fence != NULL)
		ID3D12Fence_Release(backend->fence);
	if (backend->queue != NULL)
		ID3D12CommandQueue_Release(backend->queue);
	if (backend->device != NULL)
		ID3D12Device_Release(backend->device);
	if (backend->adapter != NULL)
		IDXGIAdapter1_Release(backend->adapter);
	if (backend->factory != NULL)
		IDXGIFactory1_Release(backend->factory);

	/* Release the final plain backend owner. */
	noct_free(backend);
}

/* Borrow target-neutral metadata from one prepared D3D12 payload. */
static const struct accel_program *
accel_dx12_get_program(
	const struct accel_prepared_program *prepared)
{
	const struct accel_dx12_prepared *payload;

	/* Reject an absent or invalid opaque payload. */
	if (prepared == NULL || prepared->payload == NULL)
		return NULL;

	payload = prepared->payload;

	/* Return immutable program metadata to the common runtime. */
	return payload->program;
}

/* Validate one checked one-dimensional D3D12 dispatch range. */
static bool
accel_dx12_validate_dispatch_limit(
	void *backend_state,
	const struct accel_prepared_program *prepared,
	uint32_t kernel_index,
	uint32_t start,
	uint32_t trip,
	char *error,
	size_t error_size)
{
	const struct accel_dx12_prepared *payload;
	uint32_t group_count;

	UNUSED_PARAMETER(backend_state);
	UNUSED_PARAMETER(start);

	/* Require a published kernel payload. */
	if (prepared == NULL || prepared->payload == NULL)
		return accel_dx12_error(error, error_size, N_TR("missing Direct3D 12 program"));

	payload = prepared->payload;

	/* Reject a stale kernel index. */
	if (kernel_index >= payload->kernel_count)
		return accel_dx12_error(error, error_size, N_TR("invalid Direct3D 12 kernel index"));

	/* Empty ranges require no hardware dispatch. */
	if (trip == 0)
		return true;

	/* Bound the exact 64-lane group count to D3D12's X dimension. */
	group_count = (trip - 1) / ACCEL_DX12_WORKGROUP_SIZE + 1;
	if (group_count > ACCEL_DX12_MAX_GROUPS)
		return accel_dx12_error(error, error_size, N_TR("Direct3D 12 dispatch exceeds 65535 workgroups"));

	/* Report a representable one-dimensional dispatch. */
	return true;
}

/* Create one checked execution and its optional command recording. */
static bool
accel_dx12_create_execution(
	void *backend_state,
	const struct accel_prepared_program *prepared,
	uint32_t scalar_word_count,
	const uint32_t scalar_word[],
	uint32_t result_word_count,
	const uint32_t result_word[],
	uint32_t buffer_count,
	const struct accel_runtime_buffer buffer[],
	void **execution,
	char *error,
	size_t error_size)
{
	struct accel_dx12_backend *backend;
	const struct accel_dx12_prepared *payload;
	struct accel_dx12_execution *created;
	uint32_t expected_scalar_count;
	bool has_active_dispatch;

	/* Clear the ownership result before validating inputs. */
	if (execution == NULL)
		return accel_dx12_error(error, error_size, N_TR("missing Direct3D 12 execution output"));

	*execution = NULL;
	backend = backend_state;

	/* Validate the immutable prepared-program boundary. */
	if (backend == NULL ||
	    prepared == NULL ||
	    prepared->payload == NULL) {
		return accel_dx12_error(error, error_size, N_TR("invalid Direct3D 12 execution request"));
	}

	payload = prepared->payload;
	if (payload->program == NULL || payload->kernel == NULL)
		return accel_dx12_error(error, error_size, N_TR("incomplete Direct3D 12 prepared program"));

	/* Match all common-runtime arrays to cloned program metadata. */
	if (buffer_count != payload->program->buffer_count)
		return accel_dx12_error(error, error_size, N_TR("Direct3D 12 buffer table mismatch"));
	if (buffer_count != 0 && buffer == NULL)
		return accel_dx12_error(error, error_size, N_TR("missing Direct3D 12 buffer table"));
	if (payload->program->kernel_count >
	    ((uint32_t)-1 - payload->program->scalar_count) / 2U) {
		return accel_dx12_error(error, error_size, N_TR("Direct3D 12 scalar table overflow"));
	}

	expected_scalar_count = payload->program->scalar_count +
		payload->program->kernel_count * 2U;
	if (scalar_word_count != expected_scalar_count)
		return accel_dx12_error(error, error_size, N_TR("Direct3D 12 scalar table mismatch"));
	if (scalar_word_count != 0 && scalar_word == NULL)
		return accel_dx12_error(error, error_size, N_TR("missing Direct3D 12 scalar words"));
	if (result_word_count != payload->program->scalar_result_count)
		return accel_dx12_error(error, error_size, N_TR("Direct3D 12 result table mismatch"));
	if (result_word_count == 0 && result_word != NULL)
		return accel_dx12_error(error, error_size, N_TR("unexpected Direct3D 12 result words"));
	if (result_word_count != 0 && result_word == NULL)
		return accel_dx12_error(error, error_size, N_TR("missing Direct3D 12 result words"));

	/* Fold all validated dispatch ranges into one execution activity flag. */
	has_active_dispatch = accel_dx12_has_active_dispatch(
		payload->program,
		scalar_word);

	/* Allocate the execution owner before opening session resources. */
	created = noct_calloc(1, sizeof(*created));
	if (created == NULL)
		return accel_dx12_error(error, error_size, N_TR("out of memory creating a Direct3D 12 execution"));

	created->backend = backend;
	created->kernel_count = payload->kernel_count;
	created->buffer_count = buffer_count;
	created->result_word_count = result_word_count;
	created->has_active_dispatch = has_active_dispatch;

	/* Validate and retain plain metadata before allocating any GPU object. */
	if (!accel_dx12_create_execution_metadata(
		created,
		payload->program,
		buffer_count,
		buffer,
		error,
		error_size)) {
		accel_dx12_destroy_execution(created);
		return false;
	}

	/* Publish an identity-only execution without opening Direct3D 12 objects. */
	if (!created->has_active_dispatch) {
		*execution = created;
		if (error != NULL && error_size != 0)
			error[0] = '\0';
		return true;
	}

	/* Retain every COM object a submitted command list may reference. */
	if (!accel_dx12_retain_execution_objects(
		created,
		payload,
		error,
		error_size)) {
		accel_dx12_destroy_execution(created);
		return false;
	}

	/* Create the independent allocator and recording command list. */
	if (!accel_dx12_create_command(created, error, error_size)) {
		accel_dx12_destroy_execution(created);
		return false;
	}

	/* Create and upload every data and scalar resource. */
	if (!accel_dx12_create_execution_buffers(
		created,
		scalar_word_count,
		scalar_word,
		result_word_count,
		result_word,
		buffer_count,
		buffer,
		error,
		error_size)) {
		accel_dx12_destroy_execution(created);
		return false;
	}

	/* Create one stable shader-visible UAV table for all kernels. */
	if (!accel_dx12_create_descriptors(created, error, error_size)) {
		accel_dx12_destroy_execution(created);
		return false;
	}

	/* Transfer the complete recording session to the common runtime. */
	*execution = created;

	/* Report a complete execution owner. */
	return true;
}

/* Record one ordered 64-lane compute dispatch. */
static bool
accel_dx12_dispatch_execution(
	void *execution,
	uint32_t kernel_index,
	uint32_t start,
	uint32_t trip,
	char *error,
	size_t error_size)
{
	struct accel_dx12_execution *current;
	const struct accel_dx12_kernel *kernel;
	ID3D12DescriptorHeap *heap[1];
	D3D12_GPU_DESCRIPTOR_HANDLE table;
	uint32_t group_count;

	UNUSED_PARAMETER(start);

	current = execution;

	/* Require a live active recording and one published pipeline. */
	if (current == NULL ||
	    !current->has_active_dispatch ||
	    !current->recording) {
		return accel_dx12_error(error, error_size, N_TR("Direct3D 12 execution is not recording"));
	}
	if (kernel_index >= current->kernel_count)
		return accel_dx12_error(error, error_size, N_TR("invalid Direct3D 12 kernel index"));
	if (trip == 0)
		return accel_dx12_error(error, error_size, N_TR("empty Direct3D 12 dispatch reached the backend"));

	/* Revalidate the group count at the backend callback boundary. */
	group_count = (trip - 1) / ACCEL_DX12_WORKGROUP_SIZE + 1;
	if (group_count > ACCEL_DX12_MAX_GROUPS)
		return accel_dx12_error(error, error_size, N_TR("Direct3D 12 dispatch exceeds 65535 workgroups"));

	kernel = &current->kernel[kernel_index];

	/* Bind the immutable pipeline and shared session descriptor table. */
	heap[0] = current->descriptor_heap;
	ID3D12GraphicsCommandList_SetDescriptorHeaps(
		current->command_list,
		1,
		heap);
	ID3D12GraphicsCommandList_SetComputeRootSignature(
		current->command_list,
		kernel->root_signature);
	ID3D12GraphicsCommandList_SetPipelineState(
		current->command_list,
		kernel->pipeline);
#if defined(__MINGW32__)
	table = ID3D12DescriptorHeap_GetGPUDescriptorHandleForHeapStart(
		current->descriptor_heap);
#else
	ID3D12DescriptorHeap_GetGPUDescriptorHandleForHeapStart(
		current->descriptor_heap,
		&table);
#endif
	ID3D12GraphicsCommandList_SetComputeRootDescriptorTable(
		current->command_list,
		0,
		table);

	/* Record the one-dimensional dispatch and order following UAV access. */
	ID3D12GraphicsCommandList_Dispatch(
		current->command_list,
		group_count,
		1,
		1);
	accel_dx12_uav_barrier(current);

	/* Report a successfully recorded dispatch. */
	return true;
}

/* Complete one execution and synchronously fill any download snapshots. */
static bool
accel_dx12_finish_execution(
	void *execution,
	uint32_t result_word_count,
	uint32_t result_word[],
	uint32_t buffer_count,
	struct accel_runtime_buffer buffer[],
	char *error,
	size_t error_size)
{
	struct accel_dx12_execution *current;

	current = execution;

	/* Validate the live execution and borrowed output table. */
	if (current == NULL)
		return accel_dx12_error(error, error_size, N_TR("missing Direct3D 12 execution"));
	if (buffer_count != current->buffer_count)
		return accel_dx12_error(error, error_size, N_TR("Direct3D 12 output table mismatch"));
	if (buffer_count != 0 && buffer == NULL)
		return accel_dx12_error(error, error_size, N_TR("missing Direct3D 12 output table"));
	if (result_word_count != current->result_word_count)
		return accel_dx12_error(error, error_size, N_TR("Direct3D 12 result table changed"));
	if (result_word_count == 0 && result_word != NULL)
		return accel_dx12_error(error, error_size, N_TR("unexpected Direct3D 12 result words"));
	if (result_word_count != 0 && result_word == NULL)
		return accel_dx12_error(error, error_size, N_TR("missing Direct3D 12 result words"));

	/* Revalidate every immutable buffer field before changing execution state. */
	if (!accel_dx12_validate_finish_metadata(
		current,
		buffer_count,
		buffer,
		error,
		error_size)) {
		return false;
	}

	/* Preserve scalar identities without submitting an all-empty execution. */
	if (!current->has_active_dispatch) {
		if (current->recording ||
		    current->submission_state != ACCEL_DX12_SUBMISSION_NONE) {
			return accel_dx12_error(error, error_size, N_TR("invalid empty Direct3D 12 execution state"));
		}

		current->submission_state = ACCEL_DX12_SUBMISSION_DRAINED;
		if (error != NULL && error_size != 0)
			error[0] = '\0';

		/* Report identity-only completion without touching result words. */
		return true;
	}

	/* Require a command recording for an execution with active dispatches. */
	if (!current->recording)
		return accel_dx12_error(error, error_size, N_TR("Direct3D 12 execution is not recording"));

	/* Record every requested device-to-readback copy. */
	if (!accel_dx12_record_downloads(
		current,
		result_word_count,
		result_word,
		buffer_count,
		buffer,
		error,
		error_size)) {
		return false;
	}

	/* Close, submit, and synchronously wait on the shared queue. */
	if (!accel_dx12_submit_and_wait(current, error, error_size))
		return false;

	/* Copy completed plain bytes into runtime-owned snapshots. */
	if (!accel_dx12_copy_downloads(
		current,
		result_word_count,
		result_word,
		buffer_count,
		buffer,
		error,
		error_size)) {
		return false;
	}

	/* Report a synchronously completed execution. */
	return true;
}

/* Destroy one partial or complete session execution exactly once. */
static void
accel_dx12_destroy_execution(
	void *execution)
{
	struct accel_dx12_execution *current;

	current = execution;

	/* Accept cleanup of an optional execution. */
	if (current == NULL)
		return;

	/* Recheck a signalled submission without waiting in the destroy callback. */
	if (current->submission_state == ACCEL_DX12_SUBMISSION_IN_FLIGHT) {
		if (!accel_dx12_recheck_submission(current))
			current->submission_state = ACCEL_DX12_SUBMISSION_ABANDONED;
	}

	/* Preserve every object an unproved in-flight command may reference. */
	if (current->submission_state == ACCEL_DX12_SUBMISSION_ABANDONED) {
		accel_dx12_abandon_execution_objects(current);
		return;
	}

	/* Release only unsubmitted or synchronously drained ownership. */
	accel_dx12_destroy_safe_execution(current);
}

/* Retain every shared COM object referenced by one execution recording. */
static bool
accel_dx12_retain_execution_objects(
	struct accel_dx12_execution *execution,
	const struct accel_dx12_prepared *prepared,
	char *error,
	size_t error_size)
{
	struct accel_dx12_backend *backend;
	uint32_t i;

	backend = execution->backend;

	/* Require complete backend and prepared owners before retaining them. */
	if (backend == NULL ||
	    backend->device == NULL ||
	    backend->queue == NULL ||
	    backend->fence == NULL ||
	    prepared == NULL ||
	    (prepared->kernel_count != 0 && prepared->kernel == NULL)) {
		return accel_dx12_error(error, error_size, N_TR("incomplete Direct3D 12 execution objects"));
	}

	/* Keep the device-wide objects alive across an unrecoverable submission. */
	execution->retained_device = backend->device;
	execution->retained_queue = backend->queue;
	execution->retained_fence = backend->fence;
	ID3D12Device_AddRef(execution->retained_device);
	ID3D12CommandQueue_AddRef(execution->retained_queue);
	ID3D12Fence_AddRef(execution->retained_fence);

	/* Allocate one execution-owned COM reference pair per kernel. */
	execution->kernel_count = prepared->kernel_count;
	if (execution->kernel_count != 0) {
		execution->kernel = noct_calloc(
			execution->kernel_count,
			sizeof(*execution->kernel));
		if (execution->kernel == NULL) {
			return accel_dx12_error(error, error_size, N_TR("out of memory retaining Direct3D 12 kernels"));
		}
	}

	/* Retain immutable pipelines and root signatures until execution drains. */
	for (i = 0; i < execution->kernel_count; i++) {
		if (prepared->kernel[i].root_signature == NULL ||
		    prepared->kernel[i].pipeline == NULL) {
			return accel_dx12_error(error, error_size, N_TR("incomplete Direct3D 12 kernel objects"));
		}

		execution->kernel[i].root_signature =
			prepared->kernel[i].root_signature;
		execution->kernel[i].pipeline = prepared->kernel[i].pipeline;
		ID3D12RootSignature_AddRef(
			execution->kernel[i].root_signature);
		ID3D12PipelineState_AddRef(execution->kernel[i].pipeline);
	}

	/* Report a complete independent lifetime envelope. */
	return true;
}

/* Release every COM object owned or retained by one safe execution. */
static void
accel_dx12_release_execution_objects(
	struct accel_dx12_execution *execution)
{
	uint32_t i;

	/* Release all allocated per-buffer resources in ownership order. */
	if (execution->buffer != NULL) {
		for (i = 0; i < execution->buffer_count; i++) {
			accel_dx12_destroy_buffer(&execution->buffer[i].readback);
			accel_dx12_destroy_buffer(&execution->buffer[i].upload);
			accel_dx12_destroy_buffer(&execution->buffer[i].device);
		}
	}

	accel_dx12_destroy_buffer(&execution->scalar_upload);
	accel_dx12_destroy_buffer(&execution->scalar_device);
	accel_dx12_destroy_buffer(&execution->result_readback);
	accel_dx12_destroy_buffer(&execution->result_upload);
	accel_dx12_destroy_buffer(&execution->result_device);

	/* Release command objects after all submitted work is known complete. */
	if (execution->descriptor_heap != NULL)
		ID3D12DescriptorHeap_Release(execution->descriptor_heap);
	if (execution->command_list != NULL)
		ID3D12GraphicsCommandList_Release(execution->command_list);
	if (execution->allocator != NULL)
		ID3D12CommandAllocator_Release(execution->allocator);

	/* Close only events whose registered submission is known complete. */
	for (i = 0; i < ACCEL_DX12_DRAIN_ATTEMPTS; i++) {
		if (execution->fence_event[i] != NULL)
			CloseHandle(execution->fence_event[i]);
	}

	/* Release retained immutable kernel objects in binding order. */
	if (execution->kernel != NULL) {
		for (i = 0; i < execution->kernel_count; i++)
			accel_dx12_destroy_kernel(&execution->kernel[i]);
	}

	/* Release the execution's device-wide lifetime references last. */
	if (execution->retained_fence != NULL)
		ID3D12Fence_Release(execution->retained_fence);
	if (execution->retained_queue != NULL)
		ID3D12CommandQueue_Release(execution->retained_queue);
	if (execution->retained_device != NULL)
		ID3D12Device_Release(execution->retained_device);
}

/* Abandon only the COM graph of one submission whose drain is unproved. */
static void
accel_dx12_abandon_execution_objects(
	struct accel_dx12_execution *execution)
{
	struct accel_dx12_backend *backend;

	/*
	 * D3D12 exposes no safe cancellation primitive after ExecuteCommandLists.
	 * Dropping any referenced COM object before a proved fence completion can
	 * cause GPU use-after-free.  Retain the complete execution behind its
	 * backend until a final queue drain or device removal proves cleanup safe.
	 */
	backend = execution->backend;
	accel_mutex_lock(&backend->queue_mutex);
	if (!execution->abandoned_linked) {
		execution->next_abandoned = backend->abandoned_execution_head;
		backend->abandoned_execution_head = execution;
		execution->abandoned_linked = true;
	}
	accel_mutex_unlock(&backend->queue_mutex);
}

/* Destroy one execution after proving that no queued command references it. */
static void
accel_dx12_destroy_safe_execution(
	struct accel_dx12_execution *execution)
{
	/* Release the complete safe COM graph in dependency order. */
	accel_dx12_release_execution_objects(execution);
	noct_free(execution->kernel);
	noct_free(execution->buffer);

	/* Clear stale owners and release the plain execution wrapper. */
	memset(execution, 0, sizeof(*execution));
	noct_free(execution);
}

/* Drain all prior queue submissions with one backend-owned terminal fence. */
static bool
accel_dx12_drain_backend(
	struct accel_dx12_backend *backend)
{
	UINT64 completed;
	HRESULT result;
	bool drained;

	/* Skip the device-wide drain when no execution was retained. */
	if (backend->abandoned_execution_head == NULL)
		return true;

	/* Serialize terminal synchronization with every shared-queue operation. */
	drained = false;
	accel_mutex_lock(&backend->queue_mutex);

	/* Create a private fence that follows every previously submitted command. */
	result = ID3D12Device_CreateFence(
		backend->device,
		0,
		D3D12_FENCE_FLAG_NONE,
		&IID_ID3D12Fence,
		(void **)&backend->drain_fence);
	if (FAILED(result)) {
		drained = accel_dx12_backend_device_removed(backend);
		if (drained)
			accel_dx12_release_backend_drain(backend);
		accel_mutex_unlock(&backend->queue_mutex);
		return drained;
	}

	/* Enqueue one completion marker strictly after every abandoned submission. */
	result = ID3D12CommandQueue_Signal(
		backend->queue,
		backend->drain_fence,
		1);
	if (FAILED(result)) {
		drained = accel_dx12_backend_device_removed(backend);
		if (drained)
			accel_dx12_release_backend_drain(backend);
		accel_mutex_unlock(&backend->queue_mutex);
		return drained;
	}

	/* Block on the terminal value without adding an application event owner. */
	result = ID3D12Fence_SetEventOnCompletion(
		backend->drain_fence,
		1,
		NULL);
	if (SUCCEEDED(result)) {
		drained = true;
	} else {
		/* Prove completion even when the blocking API reported a failure. */
		completed = ID3D12Fence_GetCompletedValue(backend->drain_fence);
		if (completed == (UINT64)-1 || completed >= 1) {
			drained = true;
		} else {
			drained = accel_dx12_backend_device_removed(backend);
		}
	}

	/* Release terminal synchronization only after cleanup is proved safe. */
	if (drained)
		accel_dx12_release_backend_drain(backend);
	accel_mutex_unlock(&backend->queue_mutex);

	/* Report whether all retained execution ownership can now be released. */
	return drained;
}

/* Test whether device removal makes every queued reference terminal. */
static bool
accel_dx12_backend_device_removed(
	struct accel_dx12_backend *backend)
{
	HRESULT result;

	/* Query the retained logical device for its terminal removal reason. */
	result = ID3D12Device_GetDeviceRemovedReason(backend->device);

	/* Report a terminal device-loss state. */
	if (FAILED(result))
		return true;

	/* Report an operational device whose queued lifetime remains unproved. */
	return false;
}

/* Release the backend's optional terminal-drain objects exactly once. */
static void
accel_dx12_release_backend_drain(
	struct accel_dx12_backend *backend)
{
	/* Release the terminal fence after the queue no longer references it. */
	if (backend->drain_fence != NULL) {
		ID3D12Fence_Release(backend->drain_fence);
		backend->drain_fence = NULL;
	}
}

/* Destroy every execution made safe by the terminal backend drain. */
static void
accel_dx12_destroy_abandoned_executions(
	struct accel_dx12_backend *backend)
{
	struct accel_dx12_execution *execution;

	/* Pop and release each retained execution without taking the queue mutex. */
	while (backend->abandoned_execution_head != NULL) {
		execution = backend->abandoned_execution_head;
		backend->abandoned_execution_head = execution->next_abandoned;
		execution->next_abandoned = NULL;
		execution->abandoned_linked = false;
		execution->submission_state = ACCEL_DX12_SUBMISSION_DRAINED;
		accel_dx12_destroy_safe_execution(execution);
	}
}

/* Recheck one signalled submission without blocking backend destruction. */
static bool
accel_dx12_recheck_submission(
	struct accel_dx12_execution *execution)
{
	UINT64 completed;

	/* An unsignalled submission has no completion marker to prove its drain. */
	if (!execution->fence_signalled || execution->fence_value == 0)
		return false;

	/* Serialize the final fence query with device-wide queue operations. */
	accel_mutex_lock(&execution->backend->queue_mutex);
	completed = ID3D12Fence_GetCompletedValue(execution->retained_fence);
	accel_mutex_unlock(&execution->backend->queue_mutex);

	/* Treat the device-removed sentinel as an unproved drain. */
	if (completed == (UINT64)-1 || completed < execution->fence_value)
		return false;

	execution->submission_state = ACCEL_DX12_SUBMISSION_DRAINED;

	/* Report a completion value that safely covers this submission. */
	return true;
}

/* Create one independent compute command allocator and recording list. */
static bool
accel_dx12_create_command(
	struct accel_dx12_execution *execution,
	char *error,
	size_t error_size)
{
	HRESULT result;

	/* Create the session-owned compute allocator. */
	result = ID3D12Device_CreateCommandAllocator(
		execution->backend->device,
		D3D12_COMMAND_LIST_TYPE_COMPUTE,
		&IID_ID3D12CommandAllocator,
		(void **)&execution->allocator);
	if (FAILED(result))
		return accel_dx12_error(error, error_size, N_TR("failed to create a Direct3D 12 command allocator"));

	/* Open an independent command list without a default pipeline. */
	result = ID3D12Device_CreateCommandList(
		execution->backend->device,
		0,
		D3D12_COMMAND_LIST_TYPE_COMPUTE,
		execution->allocator,
		NULL,
		&IID_ID3D12GraphicsCommandList,
		(void **)&execution->command_list);
	if (FAILED(result))
		return accel_dx12_error(error, error_size, N_TR("failed to create a Direct3D 12 command list"));

	execution->recording = true;

	/* Report a live session command recording. */
	return true;
}

/* Detect whether any validated kernel range contains one invocation. */
static bool
accel_dx12_has_active_dispatch(
	const struct accel_program *program,
	const uint32_t scalar_word[])
{
	uint32_t word_index;
	uint32_t i;

	/* Inspect each trailing start/trip pair in deterministic kernel order. */
	for (i = 0; i < program->kernel_count; i++) {
		word_index = program->scalar_count + i * 2U + 1U;
		if (scalar_word[word_index] != 0)
			return true;
	}

	/* Report an execution whose kernels are all empty. */
	return false;
}

/* Allocate and validate the execution's CPU-only buffer metadata. */
static bool
accel_dx12_create_execution_metadata(
	struct accel_dx12_execution *execution,
	const struct accel_program *program,
	uint32_t buffer_count,
	const struct accel_runtime_buffer buffer[],
	char *error,
	size_t error_size)
{
	const struct accel_buffer_binding *binding;
	uint32_t i;

	/* Allocate one plain metadata owner per program binding. */
	if (buffer_count != 0) {
		execution->buffer = noct_calloc(
			buffer_count,
			sizeof(*execution->buffer));
		if (execution->buffer == NULL) {
			return accel_dx12_error(
				error,
				error_size,
				N_TR("out of memory creating Direct3D 12 buffer metadata"));
		}
	}

	/* Validate every binding without opening a Direct3D 12 resource. */
	for (i = 0; i < buffer_count; i++) {
		binding = &program->buffer[i];

		/* Match immutable runtime identity to the prepared program. */
		if (buffer[i].origin != binding->origin ||
		    buffer[i].args_slot != binding->args_slot ||
		    buffer[i].element_kind != binding->element_kind ||
		    buffer[i].element_width != binding->element_width) {
			return accel_dx12_error(error, error_size, N_TR("Direct3D 12 buffer metadata does not match the program"));
		}

		if (!accel_dx12_read_data_buffer_metadata(
			execution->has_active_dispatch,
			&execution->buffer[i],
			&buffer[i],
			error,
			error_size)) {
			return false;
		}
	}

	/* Report a complete immutable CPU metadata table. */
	return true;
}

/* Validate and copy one backend-neutral buffer metadata record. */
static bool
accel_dx12_read_data_buffer_metadata(
	bool has_active_dispatch,
	struct accel_dx12_execution_buffer *result,
	const struct accel_runtime_buffer *runtime_buffer,
	char *error,
	size_t error_size)
{
	size_t calculated_byte_count;
	size_t word_count;

	/* Clear the plain destination before validating the borrowed record. */
	memset(result, 0, sizeof(*result));

	/* Validate the plain extent and raw-word ABI at the backend boundary. */
	if (runtime_buffer->element_width == 0 ||
	    runtime_buffer->element_count >
	    (size_t)-1 / runtime_buffer->element_width) {
		return accel_dx12_error(error, error_size, N_TR("Direct3D 12 buffer size overflow"));
	}
	calculated_byte_count = runtime_buffer->element_count *
		runtime_buffer->element_width;
	if (calculated_byte_count != runtime_buffer->byte_count)
		return accel_dx12_error(error, error_size, N_TR("Direct3D 12 buffer extent mismatch"));
	if (runtime_buffer->element_width != sizeof(uint32_t))
		return accel_dx12_error(error, error_size, N_TR("Direct3D 12 supports only 32-bit buffers"));
	if (runtime_buffer->byte_count % sizeof(uint32_t) != 0)
		return accel_dx12_error(error, error_size, N_TR("Direct3D 12 buffer is not word aligned"));

	/* Reject unknown origins and invalid host/device argument namespaces. */
	if (runtime_buffer->origin != ACCEL_BUFFER_PARAMETER &&
	    runtime_buffer->origin != ACCEL_BUFFER_LOCAL_HOST &&
	    runtime_buffer->origin != ACCEL_BUFFER_LOCAL_DEVICE) {
		return accel_dx12_error(error, error_size, N_TR("invalid Direct3D 12 buffer origin"));
	}
	if (runtime_buffer->origin == ACCEL_BUFFER_LOCAL_DEVICE) {
		if (runtime_buffer->args_slot != ACCEL_ARGS_SLOT_NONE) {
			return accel_dx12_error(error, error_size, N_TR("device-only Direct3D 12 buffer has a host argument"));
		}
	} else {
		if (runtime_buffer->args_slot == ACCEL_ARGS_SLOT_NONE) {
			return accel_dx12_error(error, error_size, N_TR("host Direct3D 12 buffer is missing its argument"));
		}
	}

	/* Reject an active binding when every kernel dispatch is empty. */
	if (!has_active_dispatch && runtime_buffer->active) {
		return accel_dx12_error(error, error_size, N_TR("empty Direct3D 12 execution has an active buffer"));
	}

	/* Reject mutable transfer metadata for an inactive binding. */
	if (!runtime_buffer->active &&
	    (runtime_buffer->upload || runtime_buffer->download)) {
		return accel_dx12_error(error, error_size, N_TR("inactive Direct3D 12 buffer requests a transfer"));
	}

	/* Match plain snapshot ownership to the declared transfer contract. */
	if (runtime_buffer->byte_count != 0 &&
	    (runtime_buffer->upload || runtime_buffer->download) &&
	    runtime_buffer->snapshot == NULL) {
		return accel_dx12_error(error, error_size, N_TR("missing Direct3D 12 buffer snapshot"));
	}
	if ((!runtime_buffer->upload && !runtime_buffer->download) &&
	    runtime_buffer->snapshot != NULL) {
		return accel_dx12_error(error, error_size, N_TR("unexpected Direct3D 12 buffer snapshot"));
	}

	/* Keep device-only storage outside every host-transfer path. */
	if (runtime_buffer->origin == ACCEL_BUFFER_LOCAL_DEVICE &&
	    (runtime_buffer->byte_count == 0 ||
	     runtime_buffer->upload ||
	     runtime_buffer->download ||
	     runtime_buffer->snapshot != NULL)) {
		return accel_dx12_error(error, error_size, N_TR("invalid Direct3D 12 device-only buffer"));
	}

	/* Bound every active structured UAV to its UINT descriptor field. */
	word_count = runtime_buffer->byte_count / sizeof(uint32_t);
	if (runtime_buffer->active && word_count > UINT_MAX)
		return accel_dx12_error(error, error_size, N_TR("Direct3D 12 buffer is too large for one UAV"));

	/* Retain every immutable field needed by finish validation. */
	result->args_slot = runtime_buffer->args_slot;
	result->element_kind = runtime_buffer->element_kind;
	result->element_width = runtime_buffer->element_width;
	result->element_count = runtime_buffer->element_count;
	result->byte_count = runtime_buffer->byte_count;
	result->origin = runtime_buffer->origin;
	result->active = runtime_buffer->active;
	result->upload_required = runtime_buffer->upload;
	result->download = runtime_buffer->download;

	/* Report a valid plain metadata record. */
	return true;
}

/* Revalidate every borrowed finish record against creation metadata. */
static bool
accel_dx12_validate_finish_metadata(
	const struct accel_dx12_execution *execution,
	uint32_t buffer_count,
	const struct accel_runtime_buffer buffer[],
	char *error,
	size_t error_size)
{
	const struct accel_dx12_execution_buffer *owned;
	struct accel_dx12_execution_buffer candidate;
	uint32_t i;

	/* Rebuild and compare each plain record in binding order. */
	for (i = 0; i < buffer_count; i++) {
		if (!accel_dx12_read_data_buffer_metadata(
			execution->has_active_dispatch,
			&candidate,
			&buffer[i],
			error,
			error_size)) {
			return false;
		}

		owned = &execution->buffer[i];
		if (owned->args_slot != candidate.args_slot ||
		    owned->element_kind != candidate.element_kind ||
		    owned->element_width != candidate.element_width ||
		    owned->element_count != candidate.element_count ||
		    owned->byte_count != candidate.byte_count ||
		    owned->origin != candidate.origin ||
		    owned->active != candidate.active ||
		    owned->upload_required != candidate.upload_required ||
		    owned->download != candidate.download) {
			return accel_dx12_error(error, error_size, N_TR("Direct3D 12 buffer metadata changed"));
		}
	}

	/* Report an unchanged complete output table. */
	return true;
}

/* Create all device, upload, and readback resources for one active session. */
static bool
accel_dx12_create_execution_buffers(
	struct accel_dx12_execution *execution,
	uint32_t scalar_word_count,
	const uint32_t scalar_word[],
	uint32_t result_word_count,
	const uint32_t result_word[],
	uint32_t buffer_count,
	const struct accel_runtime_buffer buffer[],
	char *error,
	size_t error_size)
{
	uint32_t i;

	/* Require metadata and a command recording before creating GPU objects. */
	if (!execution->has_active_dispatch || !execution->recording)
		return accel_dx12_error(error, error_size, N_TR("invalid active Direct3D 12 execution"));
	if (buffer_count != 0 && execution->buffer == NULL)
		return accel_dx12_error(error, error_size, N_TR("missing Direct3D 12 buffer metadata"));

	/* Create every buffer in deterministic binding order. */
	for (i = 0; i < buffer_count; i++) {
		if (!accel_dx12_create_data_buffer(
			execution,
			i,
			&buffer[i],
			error,
			error_size)) {
			return false;
		}
	}

	/* Create and upload the trailing scalar-word resource. */
	if (!accel_dx12_create_scalar_buffer(
		execution,
		scalar_word_count,
		scalar_word,
		error,
		error_size)) {
		return false;
	}

	/* Creates mutable scalar results only when the ABI declares them. */
	if (result_word_count != 0) {
		if (!accel_dx12_create_result_buffer(
			execution,
			result_word_count,
			result_word,
			error,
			error_size)) {
			return false;
		}
	}

	/* Report a complete resource set. */
	return true;
}

/* Create one program buffer and its required staging resources. */
static bool
accel_dx12_create_data_buffer(
	struct accel_dx12_execution *execution,
	uint32_t buffer_index,
	const struct accel_runtime_buffer *runtime_buffer,
	char *error,
	size_t error_size)
{
	struct accel_dx12_execution_buffer *buffer;
	D3D12_RESOURCE_STATES initial_state;
	size_t allocation_byte_count;
	bool has_upload;

	/* Recover this binding's execution-owned resource record. */
	buffer = &execution->buffer[buffer_index];

	/* Select active staging and physical allocation requirements. */
	has_upload = buffer->active &&
		buffer->upload_required &&
		buffer->byte_count != 0;
	initial_state = has_upload ?
		D3D12_RESOURCE_STATE_COPY_DEST :
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
	allocation_byte_count = buffer->active ?
		buffer->byte_count : 0;

	/* Create a full active UAV or one valid dummy inactive binding. */
	if (!accel_dx12_create_buffer(
		execution->backend,
		allocation_byte_count,
		D3D12_HEAP_TYPE_DEFAULT,
		initial_state,
		true,
		&buffer->device,
		error,
		error_size)) {
		return false;
	}

	/* Create and record an upload when the common runtime supplied a snapshot. */
	if (has_upload) {
		if (runtime_buffer->snapshot == NULL)
			return accel_dx12_error(error, error_size, N_TR("missing Direct3D 12 upload snapshot"));
		if (!accel_dx12_create_buffer(
			execution->backend,
			buffer->byte_count,
			D3D12_HEAP_TYPE_UPLOAD,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			false,
			&buffer->upload,
			error,
			error_size)) {
			return false;
		}
		if (!accel_dx12_upload_buffer(
			execution,
			&buffer->device,
			&buffer->upload,
			runtime_buffer->snapshot,
			buffer->byte_count,
			error,
			error_size)) {
			return false;
		}
	}

	/* Create the readback destination before any command submission. */
	if (buffer->download && buffer->byte_count != 0) {
		if (!accel_dx12_create_buffer(
			execution->backend,
			buffer->byte_count,
			D3D12_HEAP_TYPE_READBACK,
			D3D12_RESOURCE_STATE_COPY_DEST,
			false,
			&buffer->readback,
			error,
			error_size)) {
			return false;
		}
	}

	/* Report a complete data-buffer owner. */
	return true;
}

/* Create and upload the immutable scalar and dispatch word buffer. */
static bool
accel_dx12_create_scalar_buffer(
	struct accel_dx12_execution *execution,
	uint32_t scalar_word_count,
	const uint32_t scalar_word[],
	char *error,
	size_t error_size)
{
	size_t byte_count;
	bool has_upload;

	/* Check the scalar byte count before allocating GPU resources. */
	byte_count = (size_t)scalar_word_count * sizeof(*scalar_word);

	/* Reject wrapped multiplication on narrow size_t targets. */
	if (scalar_word_count != 0 &&
	    byte_count / sizeof(*scalar_word) != scalar_word_count) {
		return accel_dx12_error(error, error_size, N_TR("Direct3D 12 scalar byte count overflow"));
	}

	has_upload = byte_count != 0;

	/* Create the scalar UAV in its initial copy or execution state. */
	if (!accel_dx12_create_buffer(
		execution->backend,
		byte_count,
		D3D12_HEAP_TYPE_DEFAULT,
		has_upload ?
			D3D12_RESOURCE_STATE_COPY_DEST :
			D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
		true,
		&execution->scalar_device,
		error,
		error_size)) {
		return false;
	}

	/* Create and record the immutable scalar upload. */
	if (has_upload) {
		if (!accel_dx12_create_buffer(
			execution->backend,
			byte_count,
			D3D12_HEAP_TYPE_UPLOAD,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			false,
			&execution->scalar_upload,
			error,
			error_size)) {
			return false;
		}
		if (!accel_dx12_upload_buffer(
			execution,
			&execution->scalar_device,
			&execution->scalar_upload,
			scalar_word,
			byte_count,
			error,
			error_size)) {
			return false;
		}
	}

	/* Report a complete scalar resource. */
	return true;
}

/* Create uploaded, shader-visible, and readback scalar-result resources. */
static bool
accel_dx12_create_result_buffer(
	struct accel_dx12_execution *execution,
	uint32_t result_word_count,
	const uint32_t result_word[],
	char *error,
	size_t error_size)
{
	size_t byte_count;

	/* Converts the nonempty result count without wrapping size_t. */
	byte_count = (size_t)result_word_count * sizeof(*result_word);
	if (byte_count / sizeof(*result_word) != result_word_count)
		return accel_dx12_error(error, error_size, N_TR("Direct3D 12 result byte count overflow"));

	/* Creates the default-heap UAV in its upload destination state. */
	if (!accel_dx12_create_buffer(
		execution->backend,
		byte_count,
		D3D12_HEAP_TYPE_DEFAULT,
		D3D12_RESOURCE_STATE_COPY_DEST,
		true,
		&execution->result_device,
		error,
		error_size)) {
		return false;
	}

	/* Creates and records the exact identity-word upload. */
	if (!accel_dx12_create_buffer(
		execution->backend,
		byte_count,
		D3D12_HEAP_TYPE_UPLOAD,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		false,
		&execution->result_upload,
		error,
		error_size)) {
		return false;
	}
	if (!accel_dx12_upload_buffer(
		execution,
		&execution->result_device,
		&execution->result_upload,
		result_word,
		byte_count,
		error,
		error_size)) {
		return false;
	}

	/* Creates a persistent readback destination before submission. */
	if (!accel_dx12_create_buffer(
		execution->backend,
		byte_count,
		D3D12_HEAP_TYPE_READBACK,
		D3D12_RESOURCE_STATE_COPY_DEST,
		false,
		&execution->result_readback,
		error,
		error_size)) {
		return false;
	}

	/* Report a complete scalar-result resource set. */
	return true;
}

/* Create and populate one shader-visible UAV descriptor table. */
static bool
accel_dx12_create_descriptors(
	struct accel_dx12_execution *execution,
	char *error,
	size_t error_size)
{
	D3D12_DESCRIPTOR_HEAP_DESC description;
	HRESULT result;
	uint32_t descriptor_count;
	uint32_t i;

	descriptor_count = execution->buffer_count + 1;
	if (execution->result_word_count != 0)
		descriptor_count++;

	/* Create one shader-visible table shared by every program kernel. */
	memset(&description, 0, sizeof(description));
	description.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	description.NumDescriptors = descriptor_count;
	description.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	result = ID3D12Device_CreateDescriptorHeap(
		execution->backend->device,
		&description,
		&IID_ID3D12DescriptorHeap,
		(void **)&execution->descriptor_heap);
	if (FAILED(result))
		return accel_dx12_error(error, error_size, N_TR("failed to create a Direct3D 12 descriptor heap"));

	/* Write all program-buffer UAVs in binding order. */
	for (i = 0; i < execution->buffer_count; i++) {
		if (!accel_dx12_write_descriptor(
			execution,
			i,
			&execution->buffer[i].device,
			error,
			error_size)) {
			return false;
		}
	}

	/* Write the scalar-word UAV at the trailing binding. */
	if (!accel_dx12_write_descriptor(
		execution,
		execution->buffer_count,
		&execution->scalar_device,
		error,
		error_size)) {
		return false;
	}

	/* Writes the optional result UAV after the immutable scalar words. */
	if (execution->result_word_count != 0) {
		if (!accel_dx12_write_descriptor(
			execution,
			execution->buffer_count + 1,
			&execution->result_device,
			error,
			error_size)) {
			return false;
		}
	}

	/* Report a complete stable descriptor table. */
	return true;
}

/* Write one structured raw-word UAV descriptor. */
static bool
accel_dx12_write_descriptor(
	struct accel_dx12_execution *execution,
	uint32_t index,
	const struct accel_dx12_buffer *buffer,
	char *error,
	size_t error_size)
{
	D3D12_UNORDERED_ACCESS_VIEW_DESC description;
	D3D12_CPU_DESCRIPTOR_HANDLE handle;
	UINT increment;
	UINT64 element_count;

	element_count = buffer->allocation_size / sizeof(uint32_t);

	/* Restrict structured-buffer elements to the descriptor's UINT field. */
	if (element_count == 0 || element_count > UINT_MAX)
		return accel_dx12_error(error, error_size, N_TR("Direct3D 12 buffer is too large for one UAV"));

	/* Compute the exact CPU descriptor slot. */
	increment = ID3D12Device_GetDescriptorHandleIncrementSize(
		execution->backend->device,
		D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
#if defined(__MINGW32__)
	handle = ID3D12DescriptorHeap_GetCPUDescriptorHandleForHeapStart(
		execution->descriptor_heap);
#else
	ID3D12DescriptorHeap_GetCPUDescriptorHandleForHeapStart(
		execution->descriptor_heap,
		&handle);
#endif
	handle.ptr += (SIZE_T)index * increment;

	/* Describe a tightly packed array of raw 32-bit words. */
	memset(&description, 0, sizeof(description));
	description.Format = DXGI_FORMAT_UNKNOWN;
	description.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
	description.Buffer.FirstElement = 0;
	description.Buffer.NumElements = (UINT)element_count;
	description.Buffer.StructureByteStride = sizeof(uint32_t);
	description.Buffer.CounterOffsetInBytes = 0;
	description.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE;
	ID3D12Device_CreateUnorderedAccessView(
		execution->backend->device,
		buffer->resource,
		NULL,
		&description,
		handle);

	/* Report one populated descriptor slot. */
	return true;
}

/* Create one committed buffer with a word-aligned nonzero allocation. */
static bool
accel_dx12_create_buffer(
	struct accel_dx12_backend *backend,
	size_t byte_count,
	D3D12_HEAP_TYPE heap_type,
	D3D12_RESOURCE_STATES state,
	bool unordered_access,
	struct accel_dx12_buffer *result,
	char *error,
	size_t error_size)
{
	D3D12_HEAP_PROPERTIES heap;
	D3D12_RESOURCE_DESC description;
	size_t allocation_size;
	size_t remainder;
	HRESULT hresult;

	/* Initialize the destination before any fallible D3D12 call. */
	memset(result, 0, sizeof(*result));

	/* Round small or partial words to one valid structured-buffer element. */
	allocation_size = byte_count;
	if (allocation_size == 0)
		allocation_size = sizeof(uint32_t);
	remainder = allocation_size % sizeof(uint32_t);
	if (remainder != 0) {
		if (allocation_size >
		    (size_t)-1 - (sizeof(uint32_t) - remainder)) {
			return accel_dx12_error(error, error_size, N_TR("Direct3D 12 buffer size overflow"));
		}
		allocation_size += sizeof(uint32_t) - remainder;
	}

	/* Describe the selected standard heap type. */
	memset(&heap, 0, sizeof(heap));
	heap.Type = heap_type;
	heap.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	heap.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	heap.CreationNodeMask = 1;
	heap.VisibleNodeMask = 1;

	/* Describe one row-major committed buffer. */
	memset(&description, 0, sizeof(description));
	description.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	description.Alignment = 0;
	description.Width = (UINT64)allocation_size;
	description.Height = 1;
	description.DepthOrArraySize = 1;
	description.MipLevels = 1;
	description.Format = DXGI_FORMAT_UNKNOWN;
	description.SampleDesc.Count = 1;
	description.SampleDesc.Quality = 0;
	description.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	description.Flags = unordered_access ?
		D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS :
		D3D12_RESOURCE_FLAG_NONE;

	/* Create the exact committed resource. */
	hresult = ID3D12Device_CreateCommittedResource(
		backend->device,
		&heap,
		D3D12_HEAP_FLAG_NONE,
		&description,
		state,
		NULL,
		&IID_ID3D12Resource,
		(void **)&result->resource);
	if (FAILED(hresult))
		return accel_dx12_error(error, error_size, N_TR("failed to create a Direct3D 12 buffer"));

	result->allocation_size = (UINT64)allocation_size;
	result->state = state;

	/* Report a complete committed buffer owner. */
	return true;
}

/* Destroy one optional committed buffer and clear its state. */
static void
accel_dx12_destroy_buffer(
	struct accel_dx12_buffer *buffer)
{
	/* Release an existing COM resource exactly once. */
	if (buffer->resource != NULL)
		ID3D12Resource_Release(buffer->resource);

	/* Clear every stale resource field. */
	memset(buffer, 0, sizeof(*buffer));
}

/* Fill one upload resource and record its copy into a device UAV. */
static bool
accel_dx12_upload_buffer(
	struct accel_dx12_execution *execution,
	struct accel_dx12_buffer *destination,
	struct accel_dx12_buffer *upload,
	const void *data,
	size_t byte_count,
	char *error,
	size_t error_size)
{
	D3D12_RANGE read_range;
	D3D12_RANGE write_range;
	void *mapped;
	HRESULT result;

	/* Map the write-combined upload heap without a CPU read range. */
	read_range.Begin = 0;
	read_range.End = 0;
	mapped = NULL;
	result = ID3D12Resource_Map(
		upload->resource,
		0,
		&read_range,
		&mapped);
	if (FAILED(result) || mapped == NULL)
		return accel_dx12_error(error, error_size, N_TR("failed to map a Direct3D 12 upload buffer"));

	/* Copy exact plain snapshot bytes and publish their written range. */
	memcpy(mapped, data, byte_count);
	write_range.Begin = 0;
	write_range.End = byte_count;
	ID3D12Resource_Unmap(upload->resource, 0, &write_range);

	/* Record the staging copy followed by the execution-state transition. */
	ID3D12GraphicsCommandList_CopyBufferRegion(
		execution->command_list,
		destination->resource,
		0,
		upload->resource,
		0,
		(UINT64)byte_count);
	accel_dx12_transition(
		execution,
		destination,
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

	/* Report a completely recorded upload. */
	return true;
}

/* Record one exact resource-state transition when needed. */
static void
accel_dx12_transition(
	struct accel_dx12_execution *execution,
	struct accel_dx12_buffer *buffer,
	D3D12_RESOURCE_STATES state)
{
	D3D12_RESOURCE_BARRIER barrier;

	/* Preserve an already-correct resource state. */
	if (buffer->state == state)
		return;

	/* Order every subresource across the requested state transition. */
	memset(&barrier, 0, sizeof(barrier));
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrier.Transition.pResource = buffer->resource;
	barrier.Transition.Subresource =
		D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	barrier.Transition.StateBefore = buffer->state;
	barrier.Transition.StateAfter = state;
	ID3D12GraphicsCommandList_ResourceBarrier(
		execution->command_list,
		1,
		&barrier);
	buffer->state = state;
}

/* Order all prior UAV writes before a following dispatch or copy. */
static void
accel_dx12_uav_barrier(
	struct accel_dx12_execution *execution)
{
	D3D12_RESOURCE_BARRIER barrier;

	/* Record a global UAV barrier for the shared binding table. */
	memset(&barrier, 0, sizeof(barrier));
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
	barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrier.UAV.pResource = NULL;
	ID3D12GraphicsCommandList_ResourceBarrier(
		execution->command_list,
		1,
		&barrier);
}

/* Record every required device-to-readback transfer. */
static bool
accel_dx12_record_downloads(
	struct accel_dx12_execution *execution,
	uint32_t result_word_count,
	uint32_t result_word[],
	uint32_t buffer_count,
	struct accel_runtime_buffer buffer[],
	char *error,
	size_t error_size)
{
	struct accel_dx12_execution_buffer *owned;
	uint32_t i;
	size_t result_byte_count;

	/* Requires complete result resources before recording any output copy. */
	result_byte_count = (size_t)result_word_count * sizeof(*result_word);
	if (result_word_count != 0 &&
	    (execution->result_device.resource == NULL ||
	     execution->result_readback.resource == NULL)) {
		return accel_dx12_error(error, error_size, N_TR("missing Direct3D 12 result resources"));
	}

	/* Validate and record downloads in binding order. */
	for (i = 0; i < buffer_count; i++) {
		owned = &execution->buffer[i];

		/* Require immutable transfer metadata from begin to finish. */
		if (owned->byte_count != buffer[i].byte_count ||
		    owned->origin != buffer[i].origin ||
		    owned->active != buffer[i].active ||
		    owned->upload_required != buffer[i].upload ||
		    owned->download != buffer[i].download) {
			return accel_dx12_error(error, error_size, N_TR("Direct3D 12 buffer metadata changed"));
		}

		/* Skip buffers without a nonempty host result. */
		if (!owned->download || owned->byte_count == 0)
			continue;
		if (buffer[i].snapshot == NULL)
			return accel_dx12_error(error, error_size, N_TR("missing Direct3D 12 download snapshot"));
		if (owned->readback.resource == NULL)
			return accel_dx12_error(error, error_size, N_TR("missing Direct3D 12 readback buffer"));

		/* Order shader writes before copying exact logical bytes. */
		accel_dx12_transition(
			execution,
			&owned->device,
			D3D12_RESOURCE_STATE_COPY_SOURCE);
		ID3D12GraphicsCommandList_CopyBufferRegion(
			execution->command_list,
			owned->readback.resource,
			0,
			owned->device.resource,
			0,
			(UINT64)owned->byte_count);
	}

	/* Records the optional result block after all ordinary output buffers. */
	if (result_word_count != 0) {
		accel_dx12_transition(
			execution,
			&execution->result_device,
			D3D12_RESOURCE_STATE_COPY_SOURCE);
		ID3D12GraphicsCommandList_CopyBufferRegion(
			execution->command_list,
			execution->result_readback.resource,
			0,
			execution->result_device.resource,
			0,
			(UINT64)result_byte_count);
	}

	/* Report a complete ordered download recording. */
	return true;
}

/* Close, submit, signal, and synchronously wait on the shared queue. */
static bool
accel_dx12_submit_and_wait(
	struct accel_dx12_execution *execution,
	char *error,
	size_t error_size)
{
	ID3D12CommandList *command[1];
	struct accel_dx12_backend *backend;
	UINT64 fence_value;
	HRESULT result;

	backend = execution->backend;

	/* Close the independent recording before acquiring queue serialization. */
	result = ID3D12GraphicsCommandList_Close(execution->command_list);
	execution->recording = false;
	if (FAILED(result))
		return accel_dx12_error(error, error_size, N_TR("failed to close a Direct3D 12 command list"));

	/* Serialize submission and the shared monotonically increasing fence. */
	accel_mutex_lock(&backend->queue_mutex);
	fence_value = backend->next_fence;
	if (fence_value == 0 || fence_value == (UINT64)-1) {
		accel_mutex_unlock(&backend->queue_mutex);
		return accel_dx12_error(error, error_size, N_TR("Direct3D 12 fence counter overflowed"));
	}
	backend->next_fence++;

	/* Execute this session's complete command list. */
	command[0] = (ID3D12CommandList *)execution->command_list;
	ID3D12CommandQueue_ExecuteCommandLists(
		execution->retained_queue,
		1,
		command);
	execution->fence_value = fence_value;
	execution->submission_state = ACCEL_DX12_SUBMISSION_IN_FLIGHT;

	/* Retry the queue marker before abandoning an untrackable submission. */
	if (!accel_dx12_signal_submission(execution)) {
		accel_mutex_unlock(&backend->queue_mutex);
		return accel_dx12_error(error, error_size, N_TR("failed to signal the Direct3D 12 queue fence"));
	}
	accel_mutex_unlock(&backend->queue_mutex);

	/* Wait outside queue serialization after publishing the ordered marker. */
	if (!accel_dx12_wait_submission(execution)) {
		return accel_dx12_error(error, error_size, N_TR("failed to drain the Direct3D 12 queue submission"));
	}

	/* Detect asynchronous device removal before exposing output bytes. */
	result = ID3D12Device_GetDeviceRemovedReason(
		execution->retained_device);
	if (FAILED(result))
		return accel_dx12_error(error, error_size, N_TR("the Direct3D 12 device was removed"));

	/* Report a synchronously completed queue submission. */
	return true;
}

/* Retry the fence marker for one already-submitted command list. */
static bool
accel_dx12_signal_submission(
	struct accel_dx12_execution *execution)
{
	HRESULT result;
	uint32_t attempt;

	/* Enqueue the same ordered marker until the queue accepts it. */
	for (attempt = 0; attempt < ACCEL_DX12_DRAIN_ATTEMPTS; attempt++) {
		result = ID3D12CommandQueue_Signal(
			execution->retained_queue,
			execution->retained_fence,
			execution->fence_value);
		if (SUCCEEDED(result)) {
			execution->fence_signalled = true;
			return true;
		}
	}

	/* Leave the submission marked in-flight without a false drain claim. */
	return false;
}

/* Wait for one signalled submission with bounded event-setup retries. */
static bool
accel_dx12_wait_submission(
	struct accel_dx12_execution *execution)
{
	HANDLE event;
	UINT64 completed;
	HRESULT result;
	HRESULT device_result;
	DWORD wait_result;
	uint32_t attempt;

	/* Retry failed event setup while preserving the synchronous wait contract. */
	for (attempt = 0; attempt < ACCEL_DX12_DRAIN_ATTEMPTS; attempt++) {
		completed = ID3D12Fence_GetCompletedValue(
			execution->retained_fence);
		if (completed == (UINT64)-1)
			return false;
		if (completed >= execution->fence_value) {
			execution->submission_state =
				ACCEL_DX12_SUBMISSION_DRAINED;
			return true;
		}

		/* Give every event-registration attempt independent lifetime. */
		event = CreateEventW(NULL, FALSE, FALSE, NULL);
		if (event == NULL)
			continue;
		execution->fence_event[attempt] = event;

		/* Arm this exact ordered fence value before blocking. */
		result = ID3D12Fence_SetEventOnCompletion(
			execution->retained_fence,
			execution->fence_value,
			event);
		if (FAILED(result))
			continue;

		/* Poll a long-running wait so device removal cannot strand cleanup. */
		for (;;) {
			wait_result = WaitForSingleObject(
				event,
				ACCEL_DX12_WAIT_POLL_MS);
			if (wait_result != WAIT_TIMEOUT)
				break;

			/* Accept only an ordinary monotonic completion value. */
			completed = ID3D12Fence_GetCompletedValue(
				execution->retained_fence);
			if (completed == (UINT64)-1)
				return false;
			if (completed >= execution->fence_value) {
				execution->submission_state =
					ACCEL_DX12_SUBMISSION_DRAINED;
				return true;
			}

			/* Stop waiting when the device reports an asynchronous loss. */
			device_result = ID3D12Device_GetDeviceRemovedReason(
				execution->retained_device);
			if (FAILED(device_result))
				return false;
		}
		if (wait_result != WAIT_OBJECT_0)
			continue;

		/* Prove the fence value instead of trusting an event wake alone. */
		completed = ID3D12Fence_GetCompletedValue(
			execution->retained_fence);
		if (completed == (UINT64)-1)
			return false;
		if (completed >= execution->fence_value) {
			execution->submission_state =
				ACCEL_DX12_SUBMISSION_DRAINED;
			return true;
		}
	}

	/* Preserve the in-flight state when completion cannot be proved. */
	return false;
}

/* Copy every completed readback into a common-runtime snapshot. */
static bool
accel_dx12_copy_downloads(
	struct accel_dx12_execution *execution,
	uint32_t result_word_count,
	uint32_t result_word[],
	uint32_t buffer_count,
	struct accel_runtime_buffer buffer[],
	char *error,
	size_t error_size)
{
	struct accel_dx12_execution_buffer *owned;
	D3D12_RANGE read_range;
	D3D12_RANGE write_range;
	void *mapped;
	HRESULT result;
	uint32_t i;
	size_t result_byte_count;

	/* Map and copy each requested readback after the fence wait. */
	for (i = 0; i < buffer_count; i++) {
		owned = &execution->buffer[i];

		/* Skip buffers without a nonempty host result. */
		if (!owned->download || owned->byte_count == 0)
			continue;

		/* Map exactly the initialized logical readback range. */
		read_range.Begin = 0;
		read_range.End = owned->byte_count;
		mapped = NULL;
		result = ID3D12Resource_Map(
			owned->readback.resource,
			0,
			&read_range,
			&mapped);
		if (FAILED(result) || mapped == NULL)
			return accel_dx12_error(error, error_size, N_TR("failed to map a Direct3D 12 readback buffer"));

		/* Fill only the runtime-owned plain snapshot. */
		memcpy(buffer[i].snapshot, mapped, owned->byte_count);
		write_range.Begin = 0;
		write_range.End = 0;
		ID3D12Resource_Unmap(
			owned->readback.resource,
			0,
			&write_range);
	}

	/* Maps and copies the completed scalar-result readback block. */
	if (result_word_count != 0) {
		result_byte_count =
			(size_t)result_word_count * sizeof(*result_word);
		read_range.Begin = 0;
		read_range.End = result_byte_count;
		mapped = NULL;
		result = ID3D12Resource_Map(
			execution->result_readback.resource,
			0,
			&read_range,
			&mapped);
		if (FAILED(result) || mapped == NULL)
			return accel_dx12_error(error, error_size, N_TR("failed to map a Direct3D 12 result buffer"));

		memcpy(result_word, mapped, result_byte_count);
		write_range.Begin = 0;
		write_range.End = 0;
		ID3D12Resource_Unmap(
			execution->result_readback.resource,
			0,
			&write_range);
	}

	/* Clear an optional stale diagnostic on success. */
	if (error != NULL && error_size != 0)
		error[0] = '\0';

	/* Report complete synchronous download publication. */
	return true;
}
