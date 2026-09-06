/* -*- coding: utf-8; tab-width: 8; indent-tabs-mode: t; -*- */

/*
 * Noct Programming Language
 * Copyright (c) 2025, 2026, Awe Morris
 */

/* Headless synchronous DirectX 12 compute backend. */

#include "../core/runtime.h"

#define CINTERFACE
#define COBJMACROS
#include <windows.h>
#include <dxgi1_6.h>
#include <directx/d3d12.h>
#include <d3dcompiler.h>

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ACCEL_DX12_MAX_DESCRIPTORS \
	(ACCEL_PROGRAM_MAX_STEPS * NOCT_ARG_MAX * 8U)
#define ACCEL_DX12_MAX_GROUPS 65535U

struct accel_dx12_resource;

struct accel_dx12_runtime {
	IDXGIFactory1 *factory;
	IDXGIAdapter1 *adapter;
	ID3D12Device *device;
	ID3D12CommandQueue *queue;
	ID3D12Fence *fence;
	HANDLE fence_event;
	UINT64 next_fence;
	char device_name[256];
	struct accel_dx12_resource *resources;
	bool unavailable;
};

struct accel_dx12_pipeline {
	ID3D12RootSignature *root_signature;
	ID3D12PipelineState *pipeline;
	uint32_t descriptor_count;
	uint32_t root_constant_count;
	uint32_t descriptor_root;
	uint32_t constant_root;
	uint32_t local_size;
	struct accel_dx12_pipeline *next;
};

struct accel_dx12_buffer {
	ID3D12Resource *resource;
	UINT64 size;
	uint32_t elements;
	D3D12_RESOURCE_STATES state;
};

struct accel_dx12_resource {
	struct accel_dx12_buffer storage;
	struct accel_dx12_resource *next;
};

struct accel_dx12_command {
	ID3D12CommandAllocator *allocator;
	ID3D12GraphicsCommandList *list;
	ID3D12DescriptorHeap *heap;
	uint32_t descriptor_cursor;
	uint32_t descriptor_capacity;
	UINT descriptor_increment;
};

struct accel_dx12_submission {
	struct accel_dx12_command command;
	UINT64 fence_value;
	struct accel_dx12_buffer staging;
	struct rt_packed *source;
	struct rt_packed *destination;
	size_t source_offset;
	size_t destination_offset;
	size_t size;
	bool download;
};

static void accel_dx12_destroy_runtime(struct accel_dx12_runtime *dx);
static struct accel_dx12_runtime *accel_dx12_get_runtime(struct rt_env *env);
static bool accel_dx12_make_pipeline(struct rt_env *env,
				     struct accel_dx12_runtime *dx,
				     struct accel_kernel *kernel,
				     uint32_t local_size);
static bool accel_dx12_make_buffer(struct accel_dx12_runtime *dx,
				   UINT64 size, D3D12_HEAP_TYPE heap_type,
				   D3D12_RESOURCE_STATES state,
				   bool unordered_access,
				   struct accel_dx12_buffer *buffer);
static void accel_dx12_free_buffer(struct accel_dx12_buffer *buffer);
static bool accel_dx12_begin_command(struct accel_dx12_runtime *dx,
				     uint32_t descriptors,
				     struct accel_dx12_command *command);
static void accel_dx12_free_command(struct accel_dx12_command *command);
static bool accel_dx12_execute(struct accel_dx12_runtime *dx,
			       struct accel_dx12_command *command);
static bool accel_dx12_submit(struct accel_dx12_runtime *dx,
			      struct accel_dx12_command *command,
			      UINT64 *fence_value);
static bool accel_dx12_wait(struct accel_dx12_runtime *dx,
			    UINT64 fence_value);

static size_t
accel_dx12_element_width(
	int type)
{
	switch (type) {
	case NOCT_PACKED_INT8:
	case NOCT_PACKED_UINT8:
		return 1;
	case NOCT_PACKED_INT16:
	case NOCT_PACKED_UINT16:
		return 2;
	case NOCT_PACKED_INT32:
	case NOCT_PACKED_UINT32:
	case NOCT_PACKED_FLOAT32:
		return 4;
	case NOCT_PACKED_INT64:
	case NOCT_PACKED_UINT64:
	case NOCT_PACKED_FLOAT64:
		return 8;
	default:
		return 0;
	}
}

static void
accel_dx12_utf8_name(
	const WCHAR *source,
	char *destination,
	size_t destination_size)
{
	int result;

	if (destination_size == 0)
		return;
	result = WideCharToMultiByte(CP_UTF8, 0, source, -1,
				     destination, (int)destination_size,
				     NULL, NULL);
	if (result == 0) {
		strncpy(destination, "DirectX 12 adapter", destination_size - 1);
		destination[destination_size - 1] = '\0';
	}
}

bool
accel_dx12_list_devices(void)
{
	IDXGIFactory1 *factory;
	IDXGIAdapter1 *adapter;
	ID3D12Device *device;
	DXGI_ADAPTER_DESC1 desc;
	char name[256];
	UINT index;
	bool found;

	factory = NULL;
	if (FAILED(CreateDXGIFactory1(&IID_IDXGIFactory1, (void **)&factory)))
		return false;
	found = false;
	for (index = 0; ; index++) {
		adapter = NULL;
		if (IDXGIFactory1_EnumAdapters1(factory, index, &adapter) ==
		    DXGI_ERROR_NOT_FOUND)
			break;
		if (adapter == NULL)
			continue;
		memset(&desc, 0, sizeof(desc));
		if (SUCCEEDED(IDXGIAdapter1_GetDesc1(adapter, &desc))) {
			device = NULL;
			if (SUCCEEDED(D3D12CreateDevice((IUnknown *)adapter,
				D3D_FEATURE_LEVEL_11_0, &IID_ID3D12Device,
				(void **)&device))) {
				accel_dx12_utf8_name(desc.Description, name,
					sizeof(name));
				printf("%u  %s  %llu MiB%s\n", index, name,
					(unsigned long long)
					(desc.DedicatedVideoMemory / (1024 * 1024)),
					(desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) != 0 ?
					"  software" : "");
				ID3D12Device_Release(device);
				found = true;
			}
		}
		IDXGIAdapter1_Release(adapter);
	}
	IDXGIFactory1_Release(factory);
	return found;
}

static void
accel_dx12_destroy_runtime(
	struct accel_dx12_runtime *dx)
{
	struct accel_dx12_resource *resource;
	struct accel_dx12_resource *next;

	if (dx == NULL)
		return;
	resource = dx->resources;
	while (resource != NULL) {
		next = resource->next;
		accel_dx12_free_buffer(&resource->storage);
		noct_free(resource);
		resource = next;
	}
	if (dx->fence_event != NULL)
		CloseHandle(dx->fence_event);
	if (dx->fence != NULL)
		ID3D12Fence_Release(dx->fence);
	if (dx->queue != NULL)
		ID3D12CommandQueue_Release(dx->queue);
	if (dx->device != NULL)
		ID3D12Device_Release(dx->device);
	if (dx->adapter != NULL)
		IDXGIAdapter1_Release(dx->adapter);
	if (dx->factory != NULL)
		IDXGIFactory1_Release(dx->factory);
	noct_free(dx);
}

static struct accel_dx12_runtime *
accel_dx12_get_runtime(
	struct rt_env *env)
{
	struct accel_dx12_runtime *dx;
	IDXGIAdapter1 *candidate;
	ID3D12Device *probe;
	DXGI_ADAPTER_DESC1 desc;
	D3D12_COMMAND_QUEUE_DESC queue_desc;
	SIZE_T best_memory;
	UINT index;
	char candidate_name[256];
	bool allow_software;
	HRESULT hr;

	if (env->vm->accel_runtime != NULL) {
		dx = env->vm->accel_runtime;
		return dx->unavailable ? NULL : dx;
	}
	dx = noct_calloc(1, sizeof(*dx));
	if (dx == NULL) {
		rt_out_of_memory(env);
		return NULL;
	}
	env->vm->accel_runtime = dx;
	if (getenv("NOCT_DX12_DEBUG") != NULL) {
		ID3D12Debug *debug;
		debug = NULL;
		if (SUCCEEDED(D3D12GetDebugInterface(&IID_ID3D12Debug,
						 (void **)&debug))) {
			ID3D12Debug_EnableDebugLayer(debug);
			ID3D12Debug_Release(debug);
		}
	}
	hr = CreateDXGIFactory1(&IID_IDXGIFactory1, (void **)&dx->factory);
	if (FAILED(hr))
		goto unavailable;
	best_memory = 0;
	allow_software = getenv("NOCT_DX12_ALLOW_SOFTWARE") != NULL;
	for (index = 0; ; index++) {
		candidate = NULL;
		if (IDXGIFactory1_EnumAdapters1(dx->factory, index,
						 &candidate) == DXGI_ERROR_NOT_FOUND)
			break;
		if (candidate == NULL)
			continue;
		memset(&desc, 0, sizeof(desc));
		if (FAILED(IDXGIAdapter1_GetDesc1(candidate, &desc)) ||
		    (!allow_software &&
		     (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) != 0)) {
			IDXGIAdapter1_Release(candidate);
			continue;
		}
		accel_dx12_utf8_name(desc.Description, candidate_name,
				       sizeof(candidate_name));
		if (env->vm->config.gpu_name != NULL &&
		    strcmp(env->vm->config.gpu_name, candidate_name) != 0) {
			IDXGIAdapter1_Release(candidate);
			continue;
		}
		probe = NULL;
		if (FAILED(D3D12CreateDevice((IUnknown *)candidate,
					     D3D_FEATURE_LEVEL_11_0,
					     &IID_ID3D12Device,
					     (void **)&probe))) {
			IDXGIAdapter1_Release(candidate);
			continue;
		}
		ID3D12Device_Release(probe);
		if (dx->adapter == NULL || env->vm->config.gpu_name != NULL ||
		    desc.DedicatedVideoMemory > best_memory) {
			if (dx->adapter != NULL)
				IDXGIAdapter1_Release(dx->adapter);
			dx->adapter = candidate;
			best_memory = desc.DedicatedVideoMemory;
			strncpy(dx->device_name, candidate_name,
				sizeof(dx->device_name) - 1);
			if (env->vm->config.gpu_name != NULL)
				break;
		} else {
			IDXGIAdapter1_Release(candidate);
		}
	}
	if (dx->adapter == NULL)
		goto unavailable;
	if (FAILED(D3D12CreateDevice((IUnknown *)dx->adapter,
				     D3D_FEATURE_LEVEL_11_0,
				     &IID_ID3D12Device,
				     (void **)&dx->device)))
		goto unavailable;
	memset(&queue_desc, 0, sizeof(queue_desc));
	queue_desc.Type = D3D12_COMMAND_LIST_TYPE_COMPUTE;
	queue_desc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
	if (FAILED(ID3D12Device_CreateCommandQueue(dx->device, &queue_desc,
						    &IID_ID3D12CommandQueue,
						    (void **)&dx->queue)))
		goto unavailable;
	if (FAILED(ID3D12Device_CreateFence(dx->device, 0,
					    D3D12_FENCE_FLAG_NONE,
					    &IID_ID3D12Fence,
					    (void **)&dx->fence)))
		goto unavailable;
	dx->fence_event = CreateEventW(NULL, FALSE, FALSE, NULL);
	if (dx->fence_event == NULL)
		goto unavailable;
	dx->next_fence = 1;
	if (env->vm->config.accel_info)
		fprintf(stderr, "ACCEL: DirectX 12 device: %s\n",
			dx->device_name);
	return dx;

unavailable:
	dx->unavailable = true;
	return NULL;
}

static bool
accel_dx12_make_pipeline(
	struct rt_env *env,
	struct accel_dx12_runtime *dx,
	struct accel_kernel *kernel,
	uint32_t local_size)
{
	struct accel_dx12_pipeline *pipeline;
	struct accel_dx12_pipeline *candidate;
	struct accel_dx12_pipeline *previous;
	ID3DBlob *shader;
	ID3DBlob *errors;
	ID3DBlob *signature;
	ID3DBlob *signature_errors;
	D3D12_DESCRIPTOR_RANGE ranges[NOCT_ARG_MAX];
	D3D12_ROOT_PARAMETER root[2];
	D3D12_ROOT_SIGNATURE_DESC root_desc;
	D3D12_COMPUTE_PIPELINE_STATE_DESC pipeline_desc;
	uint32_t descriptor_count;
	uint32_t scalar_count;
	uint32_t root_count;
	uint32_t i;
	D3D_SHADER_MACRO defines[2];
	char local_size_text[16];
	HRESULT hr;

	previous = NULL;
	candidate = kernel->backend_data;
	while (candidate != NULL) {
		if (candidate->local_size == local_size) {
			if (previous != NULL) {
				previous->next = candidate->next;
				candidate->next = kernel->backend_data;
				kernel->backend_data = candidate;
			}
			return true;
		}
		previous = candidate;
		candidate = candidate->next;
	}
	if (kernel->hlsl == NULL || kernel->hlsl_size == 0)
		return false;
	if (env->vm->config.accel_info)
		fprintf(stderr,
			"ACCEL: kernel %s: compiling DirectX 12 pipeline\n",
			kernel->name);
	shader = NULL;
	errors = NULL;
	if (local_size == 0 || local_size > 1024)
		return false;
	snprintf(local_size_text, sizeof(local_size_text), "%u", local_size);
	defines[0].Name = "NOCT_LOCAL_SIZE_X";
	defines[0].Definition = local_size_text;
	defines[1].Name = NULL;
	defines[1].Definition = NULL;
	hr = D3DCompile(kernel->hlsl, kernel->hlsl_size, kernel->name,
			defines, NULL, "main", "cs_5_1",
			D3DCOMPILE_ENABLE_STRICTNESS, 0, &shader, &errors);
	if (FAILED(hr)) {
		if (env->vm->config.accel_info || getenv("NOCT_ACCEL_DEBUG") != NULL)
			fprintf(stderr, "ACCEL: kernel %s: HLSL compilation failed: %s\n",
				kernel->name, errors != NULL ?
				(const char *)ID3D10Blob_GetBufferPointer(errors) :
				"D3DCompile failed");
		if (errors != NULL) ID3D10Blob_Release(errors);
		if (shader != NULL) ID3D10Blob_Release(shader);
		return false;
	}
	if (errors != NULL) {
		if (getenv("NOCT_ACCEL_DEBUG") != NULL)
			fprintf(stderr, "ACCEL: kernel %s: %s\n", kernel->name,
				(const char *)ID3D10Blob_GetBufferPointer(errors));
		ID3D10Blob_Release(errors);
	}
	pipeline = noct_calloc(1, sizeof(*pipeline));
	if (pipeline == NULL) {
		ID3D10Blob_Release(shader);
		return false;
	}
	descriptor_count = 0;
	scalar_count = 0;
	memset(ranges, 0, sizeof(ranges));
	for (i = 0; i < kernel->param_count; i++) {
		if (kernel->param_transport[i] == ACCEL_TRANSPORT_SCALAR) {
			scalar_count++;
			continue;
		}
		ranges[descriptor_count].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
		ranges[descriptor_count].NumDescriptors = 1;
		ranges[descriptor_count].BaseShaderRegister = i;
		ranges[descriptor_count].RegisterSpace = 0;
		ranges[descriptor_count].OffsetInDescriptorsFromTableStart =
			descriptor_count;
		descriptor_count++;
	}
	memset(root, 0, sizeof(root));
	root_count = 0;
	pipeline->descriptor_root = UINT32_MAX;
	if (descriptor_count != 0) {
		pipeline->descriptor_root = root_count;
		root[root_count].ParameterType =
			D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		root[root_count].DescriptorTable.NumDescriptorRanges = descriptor_count;
		root[root_count].DescriptorTable.pDescriptorRanges = ranges;
		root[root_count].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
		root_count++;
	}
	pipeline->constant_root = root_count;
	pipeline->root_constant_count = 2 + scalar_count;
	root[root_count].ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
	root[root_count].Constants.ShaderRegister = 0;
	root[root_count].Constants.RegisterSpace = 0;
	root[root_count].Constants.Num32BitValues = pipeline->root_constant_count;
	root[root_count].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
	root_count++;
	memset(&root_desc, 0, sizeof(root_desc));
	root_desc.NumParameters = root_count;
	root_desc.pParameters = root;
	root_desc.Flags = D3D12_ROOT_SIGNATURE_FLAG_NONE;
	signature = NULL;
	signature_errors = NULL;
	hr = D3D12SerializeRootSignature(&root_desc,
					 D3D_ROOT_SIGNATURE_VERSION_1,
					 &signature, &signature_errors);
	if (FAILED(hr))
		goto failed;
	if (FAILED(ID3D12Device_CreateRootSignature(dx->device, 0,
			ID3D10Blob_GetBufferPointer(signature),
			ID3D10Blob_GetBufferSize(signature),
			&IID_ID3D12RootSignature,
			(void **)&pipeline->root_signature)))
		goto failed;
	memset(&pipeline_desc, 0, sizeof(pipeline_desc));
	pipeline_desc.pRootSignature = pipeline->root_signature;
	pipeline_desc.CS.pShaderBytecode = ID3D10Blob_GetBufferPointer(shader);
	pipeline_desc.CS.BytecodeLength = ID3D10Blob_GetBufferSize(shader);
	if (FAILED(ID3D12Device_CreateComputePipelineState(dx->device,
						    &pipeline_desc,
						    &IID_ID3D12PipelineState,
						    (void **)&pipeline->pipeline)))
		goto failed;
	pipeline->descriptor_count = descriptor_count;
	pipeline->local_size = local_size;
	pipeline->next = kernel->backend_data;
	kernel->backend_data = pipeline;
	ID3D10Blob_Release(shader);
	ID3D10Blob_Release(signature);
	if (signature_errors != NULL) ID3D10Blob_Release(signature_errors);
	return true;

failed:
	if (signature_errors != NULL && getenv("NOCT_ACCEL_DEBUG") != NULL)
		fprintf(stderr, "ACCEL: root signature failed: %s\n",
			(const char *)ID3D10Blob_GetBufferPointer(signature_errors));
	if (signature_errors != NULL) ID3D10Blob_Release(signature_errors);
	if (signature != NULL) ID3D10Blob_Release(signature);
	if (shader != NULL) ID3D10Blob_Release(shader);
	if (pipeline->pipeline != NULL)
		ID3D12PipelineState_Release(pipeline->pipeline);
	if (pipeline->root_signature != NULL)
		ID3D12RootSignature_Release(pipeline->root_signature);
	noct_free(pipeline);
	return false;
}

static bool
accel_dx12_make_buffer(
	struct accel_dx12_runtime *dx,
	UINT64 size,
	D3D12_HEAP_TYPE heap_type,
	D3D12_RESOURCE_STATES state,
	bool unordered_access,
	struct accel_dx12_buffer *buffer)
{
	D3D12_HEAP_PROPERTIES heap;
	D3D12_RESOURCE_DESC desc;

	memset(buffer, 0, sizeof(*buffer));
	memset(&heap, 0, sizeof(heap));
	heap.Type = heap_type;
	heap.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	heap.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	heap.CreationNodeMask = 1;
	heap.VisibleNodeMask = 1;
	memset(&desc, 0, sizeof(desc));
	desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	desc.Width = size != 0 ? size : 4;
	desc.Height = 1;
	desc.DepthOrArraySize = 1;
	desc.MipLevels = 1;
	desc.Format = DXGI_FORMAT_UNKNOWN;
	desc.SampleDesc.Count = 1;
	desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	desc.Flags = unordered_access ?
		D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS :
		D3D12_RESOURCE_FLAG_NONE;
	if (FAILED(ID3D12Device_CreateCommittedResource(dx->device, &heap,
			D3D12_HEAP_FLAG_NONE, &desc, state, NULL,
			&IID_ID3D12Resource, (void **)&buffer->resource)))
		return false;
	buffer->size = desc.Width;
	buffer->elements = (uint32_t)(desc.Width / 4);
	buffer->state = state;
	return true;
}

static void
accel_dx12_free_buffer(
	struct accel_dx12_buffer *buffer)
{
	if (buffer->resource != NULL)
		ID3D12Resource_Release(buffer->resource);
	memset(buffer, 0, sizeof(*buffer));
}

static bool
accel_dx12_begin_command(
	struct accel_dx12_runtime *dx,
	uint32_t descriptors,
	struct accel_dx12_command *command)
{
	D3D12_DESCRIPTOR_HEAP_DESC heap_desc;

	memset(command, 0, sizeof(*command));
	if (FAILED(ID3D12Device_CreateCommandAllocator(dx->device,
			D3D12_COMMAND_LIST_TYPE_COMPUTE,
			&IID_ID3D12CommandAllocator,
			(void **)&command->allocator)))
		goto failed;
	if (FAILED(ID3D12Device_CreateCommandList(dx->device, 0,
			D3D12_COMMAND_LIST_TYPE_COMPUTE, command->allocator,
			NULL, &IID_ID3D12GraphicsCommandList,
			(void **)&command->list)))
		goto failed;
	if (descriptors != 0) {
		memset(&heap_desc, 0, sizeof(heap_desc));
		heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		heap_desc.NumDescriptors = descriptors;
		heap_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
		if (FAILED(ID3D12Device_CreateDescriptorHeap(dx->device, &heap_desc,
				&IID_ID3D12DescriptorHeap,
				(void **)&command->heap)))
			goto failed;
		command->descriptor_capacity = descriptors;
		command->descriptor_increment =
			ID3D12Device_GetDescriptorHandleIncrementSize(dx->device,
				D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	}
	return true;

failed:
	accel_dx12_free_command(command);
	return false;
}

static void
accel_dx12_free_command(
	struct accel_dx12_command *command)
{
	if (command->heap != NULL)
		ID3D12DescriptorHeap_Release(command->heap);
	if (command->list != NULL)
		ID3D12GraphicsCommandList_Release(command->list);
	if (command->allocator != NULL)
		ID3D12CommandAllocator_Release(command->allocator);
	memset(command, 0, sizeof(*command));
}

static bool
accel_dx12_submit(
	struct accel_dx12_runtime *dx,
	struct accel_dx12_command *command,
	UINT64 *fence_value)
{
	ID3D12CommandList *lists[1];
	UINT64 value;

	if (FAILED(ID3D12GraphicsCommandList_Close(command->list)))
		return false;
	lists[0] = (ID3D12CommandList *)command->list;
	ID3D12CommandQueue_ExecuteCommandLists(dx->queue, 1, lists);
	value = dx->next_fence++;
	if (FAILED(ID3D12CommandQueue_Signal(dx->queue, dx->fence, value)))
		return false;
	*fence_value = value;
	return true;
}

static bool
accel_dx12_wait(
	struct accel_dx12_runtime *dx,
	UINT64 fence_value)
{
	if (ID3D12Fence_GetCompletedValue(dx->fence) < fence_value) {
		if (FAILED(ID3D12Fence_SetEventOnCompletion(dx->fence, fence_value,
						      dx->fence_event)))
			return false;
		if (WaitForSingleObject(dx->fence_event, INFINITE) != WAIT_OBJECT_0)
			return false;
	}
	return SUCCEEDED(ID3D12Device_GetDeviceRemovedReason(dx->device));
}

static bool
accel_dx12_execute(
	struct accel_dx12_runtime *dx,
	struct accel_dx12_command *command)
{
	UINT64 fence_value;

	return accel_dx12_submit(dx, command, &fence_value) &&
		accel_dx12_wait(dx, fence_value);
}

static void
accel_dx12_transition(
	struct accel_dx12_command *command,
	struct accel_dx12_buffer *buffer,
	D3D12_RESOURCE_STATES state)
{
	D3D12_RESOURCE_BARRIER barrier;

	if (buffer->state == state)
		return;
	memset(&barrier, 0, sizeof(barrier));
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Transition.pResource = buffer->resource;
	barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	barrier.Transition.StateBefore = buffer->state;
	barrier.Transition.StateAfter = state;
	ID3D12GraphicsCommandList_ResourceBarrier(command->list, 1, &barrier);
	buffer->state = state;
}

static void
accel_dx12_uav_barrier(
	struct accel_dx12_command *command)
{
	D3D12_RESOURCE_BARRIER barrier;

	memset(&barrier, 0, sizeof(barrier));
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
	barrier.UAV.pResource = NULL;
	ID3D12GraphicsCommandList_ResourceBarrier(command->list, 1, &barrier);
}

static bool
accel_dx12_copy_range(
	struct accel_dx12_runtime *dx,
	struct accel_dx12_buffer *storage,
	void *host,
	size_t offset,
	size_t size,
	bool upload)
{
	struct accel_dx12_buffer staging;
	struct accel_dx12_command command;
	D3D12_RANGE range;
	void *mapped;
	bool ok;

	if (size == 0)
		return true;
	memset(&staging, 0, sizeof(staging));
	memset(&command, 0, sizeof(command));
	if (!accel_dx12_make_buffer(dx, (UINT64)size,
			upload ? D3D12_HEAP_TYPE_UPLOAD : D3D12_HEAP_TYPE_READBACK,
			upload ? D3D12_RESOURCE_STATE_GENERIC_READ :
				 D3D12_RESOURCE_STATE_COPY_DEST,
			false, &staging) ||
	    !accel_dx12_begin_command(dx, 0, &command)) {
		accel_dx12_free_buffer(&staging);
		accel_dx12_free_command(&command);
		return false;
	}
	if (upload) {
		range.Begin = 0;
		range.End = 0;
		mapped = NULL;
		if (FAILED(ID3D12Resource_Map(staging.resource, 0, &range,
					     &mapped)))
			goto failed;
		memcpy(mapped, host, size);
		ID3D12Resource_Unmap(staging.resource, 0, NULL);
		accel_dx12_transition(&command, storage,
			D3D12_RESOURCE_STATE_COPY_DEST);
		ID3D12GraphicsCommandList_CopyBufferRegion(command.list,
			storage->resource, (UINT64)offset, staging.resource, 0,
			(UINT64)size);
	} else {
		accel_dx12_transition(&command, storage,
			D3D12_RESOURCE_STATE_COPY_SOURCE);
		ID3D12GraphicsCommandList_CopyBufferRegion(command.list,
			staging.resource, 0, storage->resource, (UINT64)offset,
			(UINT64)size);
	}
	if (!accel_dx12_execute(dx, &command))
		goto failed;
	if (!upload) {
		range.Begin = 0;
		range.End = size;
		mapped = NULL;
		if (FAILED(ID3D12Resource_Map(staging.resource, 0, &range,
					     &mapped)))
			goto failed;
		memcpy(host, mapped, size);
		range.Begin = 0;
		range.End = 0;
		ID3D12Resource_Unmap(staging.resource, 0, &range);
	}
	ok = true;
	goto cleanup;

failed:
	ok = false;
cleanup:
	accel_dx12_free_command(&command);
	accel_dx12_free_buffer(&staging);
	return ok;
}

static struct accel_dx12_resource *
accel_dx12_get_resource(
	struct rt_env *env,
	struct rt_packed *packed)
{
	struct accel_dx12_runtime *dx;
	struct accel_dx12_resource *resource;
	size_t width;
	size_t size;

	if (!packed->is_accel_resource)
		return NULL;
	if (packed->accel_backend_data != NULL)
		return packed->accel_backend_data;
	dx = accel_dx12_get_runtime(env);
	if (dx == NULL)
		return NULL;
	width = accel_dx12_element_width(packed->type);
	if (width == 0 || packed->elem_size > SIZE_MAX / width)
		return NULL;
	size = packed->elem_size * width;
	resource = noct_calloc(1, sizeof(*resource));
	if (resource == NULL) {
		rt_out_of_memory(env);
		return NULL;
	}
	if (!accel_dx12_make_buffer(dx, (UINT64)size,
			D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_COMMON,
			true, &resource->storage) ||
	    !accel_dx12_copy_range(dx, &resource->storage,
			packed->packed_buffer, 0, size, true)) {
		accel_dx12_free_buffer(&resource->storage);
		noct_free(resource);
		return NULL;
	}
	resource->storage.elements = (uint32_t)packed->elem_size;
	resource->next = dx->resources;
	dx->resources = resource;
	packed->accel_backend_data = resource;
	if (env->vm->config.accel_info)
		fprintf(stderr,
			"ACCEL: DirectX 12 persistent resource allocated (%lu bytes)\n",
			(unsigned long)size);
	return resource;
}

static bool
accel_dx12_record_dispatch(
	struct accel_dx12_runtime *dx,
	struct accel_dx12_command *command,
	struct accel_kernel *kernel,
	struct accel_dx12_buffer **binding,
	const uint32_t *constants,
	uint32_t group_count)
{
	struct accel_dx12_pipeline *pipeline;
	D3D12_CPU_DESCRIPTOR_HANDLE cpu;
	D3D12_GPU_DESCRIPTOR_HANDLE gpu;
	D3D12_UNORDERED_ACCESS_VIEW_DESC uav;
	ID3D12DescriptorHeap *heaps[1];
	uint32_t first;
	uint32_t i;

	pipeline = kernel->backend_data;
	if (pipeline == NULL ||
	    command->descriptor_cursor + pipeline->descriptor_count >
		command->descriptor_capacity)
		return false;
	first = command->descriptor_cursor;
	if (pipeline->descriptor_count != 0) {
		ID3D12DescriptorHeap_GetCPUDescriptorHandleForHeapStart(
			command->heap, &cpu);
		ID3D12DescriptorHeap_GetGPUDescriptorHandleForHeapStart(
			command->heap, &gpu);
		cpu.ptr += (SIZE_T)first * command->descriptor_increment;
		gpu.ptr += (UINT64)first * command->descriptor_increment;
		for (i = 0; i < kernel->param_count; i++) {
			if (kernel->param_transport[i] == ACCEL_TRANSPORT_SCALAR)
				continue;
			if (binding[i] == NULL)
				return false;
			memset(&uav, 0, sizeof(uav));
			uav.Format = DXGI_FORMAT_UNKNOWN;
			uav.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
			uav.Buffer.NumElements = binding[i]->elements;
			uav.Buffer.StructureByteStride = 4;
			ID3D12Device_CreateUnorderedAccessView(dx->device,
				binding[i]->resource, NULL, &uav, cpu);
			cpu.ptr += command->descriptor_increment;
			command->descriptor_cursor++;
		}
		heaps[0] = command->heap;
		ID3D12GraphicsCommandList_SetDescriptorHeaps(command->list, 1,
						       heaps);
	}
	ID3D12GraphicsCommandList_SetComputeRootSignature(command->list,
		pipeline->root_signature);
	ID3D12GraphicsCommandList_SetPipelineState(command->list,
		pipeline->pipeline);
	if (pipeline->descriptor_count != 0) {
		ID3D12DescriptorHeap_GetGPUDescriptorHandleForHeapStart(
			command->heap, &gpu);
		gpu.ptr += (UINT64)first * command->descriptor_increment;
		ID3D12GraphicsCommandList_SetComputeRootDescriptorTable(
			command->list, pipeline->descriptor_root, gpu);
	}
	ID3D12GraphicsCommandList_SetComputeRoot32BitConstants(command->list,
		pipeline->constant_root, pipeline->root_constant_count,
		constants, 0);
	ID3D12GraphicsCommandList_Dispatch(command->list, group_count, 1, 1);
	return true;
}

static int
accel_dx12_dispatch_program(
	struct rt_env *env,
	struct rt_func *func,
	uint32_t arg_count,
	struct rt_value *arg)
{
	struct accel_program *program;
	struct accel_dx12_runtime *dx;
	struct accel_dx12_buffer buffers[ACCEL_PROGRAM_MAX_BUFFERS];
	struct accel_dx12_buffer uploads[ACCEL_PROGRAM_MAX_BUFFERS];
	struct accel_dx12_buffer readbacks[ACCEL_PROGRAM_MAX_BUFFERS];
	struct accel_dx12_resource *resources[ACCEL_PROGRAM_MAX_BUFFERS];
	struct accel_dx12_buffer *binding[NOCT_ARG_MAX];
	struct accel_dx12_command command;
	int64_t scalar_arg[NOCT_ARG_MAX];
	int64_t buffer_length[ACCEL_PROGRAM_MAX_BUFFERS];
	uint32_t trip_count[ACCEL_PROGRAM_MAX_STEPS];
	uint32_t constants[NOCT_ARG_MAX + 2];
	struct accel_program_step *step;
	struct accel_kernel *kernel;
	uint32_t group_count;
	uint32_t next_group_count;
	uint32_t current_buffer;
	uint32_t next_buffer;
	uint32_t bound_buffer;
	uint32_t constant_count;
	uint32_t descriptor_count;
	uint32_t i;
	uint32_t j;
	uint32_t k;
	int64_t evaluated;
	size_t byte_size;
	void *mapped;
	D3D12_RANGE range;
	char validation_error[128];
	int result;

	program = func->accel_program;
	if (program == NULL || arg_count != program->outer_param_count)
		return ACCEL_DISPATCH_FALLBACK;
	if (!accel_program_validate(program, validation_error,
				    sizeof(validation_error)))
		return ACCEL_DISPATCH_FALLBACK;
	memset(scalar_arg, 0, sizeof(scalar_arg));
	memset(buffer_length, 0, sizeof(buffer_length));
	for (i = 0; i < arg_count; i++)
		if (arg[i].type == NOCT_VALUE_INT)
			scalar_arg[i] = arg[i].val.i;
	for (i = 0; i < program->buffer_count; i++) {
		struct accel_buffer_desc *desc;
		desc = &program->buffer[i];
		if (desc->outer_param >= 0) {
			j = (uint32_t)desc->outer_param;
			if (j >= arg_count || arg[j].type != NOCT_VALUE_PACKED ||
			    arg[j].val.packed->type != desc->element_kind)
				return ACCEL_DISPATCH_FALLBACK;
			buffer_length[i] = (int64_t)arg[j].val.packed->elem_size;
		}
	}
	for (i = 0; i < program->buffer_count; i++) {
		if (program->buffer[i].outer_param < 0) {
			if (!accel_expr_evaluate(program,
				program->buffer[i].length_expr, arg_count,
				scalar_arg, buffer_length, &evaluated))
				return ACCEL_DISPATCH_FALLBACK;
			buffer_length[i] = evaluated;
		}
		if (buffer_length[i] < 0 ||
		    (uint64_t)buffer_length[i] > SIZE_MAX /
			(size_t)program->buffer[i].element_width ||
		    (uint64_t)buffer_length[i] > UINT32_MAX)
			return ACCEL_DISPATCH_FALLBACK;
	}
	dx = accel_dx12_get_runtime(env);
	if (dx == NULL)
		return ACCEL_DISPATCH_FALLBACK;
	descriptor_count = 0;
	for (i = 0; i < program->step_count; i++) {
		step = &program->step[i];
		if (step->kind != ACCEL_STEP_DOALL_DISPATCH &&
		    step->kind != ACCEL_STEP_DOSUM_REDUCTION)
			return ACCEL_DISPATCH_FALLBACK;
		if (!accel_expr_evaluate(program, step->trip_expr, arg_count,
				scalar_arg, buffer_length, &evaluated) ||
		    evaluated < 0 || evaluated > UINT32_MAX)
			return ACCEL_DISPATCH_FALLBACK;
		trip_count[i] = (uint32_t)evaluated;
		if (!accel_dx12_make_pipeline(env, dx,
				program->kernel[step->kernel], step->block_size))
			return ACCEL_DISPATCH_FALLBACK;
		descriptor_count += NOCT_ARG_MAX;
		if (step->kind == ACCEL_STEP_DOSUM_REDUCTION) {
			if (!accel_dx12_make_pipeline(env, dx,
					program->kernel[step->fold_kernel],
					step->block_size))
				return ACCEL_DISPATCH_FALLBACK;
			descriptor_count += NOCT_ARG_MAX * 8U;
		}
	}
	if (descriptor_count > ACCEL_DX12_MAX_DESCRIPTORS)
		return ACCEL_DISPATCH_FALLBACK;
	memset(buffers, 0, sizeof(buffers));
	memset(uploads, 0, sizeof(uploads));
	memset(readbacks, 0, sizeof(readbacks));
	memset(resources, 0, sizeof(resources));
	memset(&command, 0, sizeof(command));
	result = ACCEL_DISPATCH_FALLBACK;
	if (!accel_dx12_begin_command(dx, descriptor_count, &command))
		goto cleanup;
	for (i = 0; i < program->buffer_count; i++) {
		struct accel_buffer_desc *desc;
		desc = &program->buffer[i];
		byte_size = (size_t)buffer_length[i] * (size_t)desc->element_width;
		if (desc->origin == ACCEL_BUFFER_DEVICE_PTR) {
			struct rt_packed *packed;
			packed = arg[desc->outer_param].val.packed;
			resources[i] = accel_dx12_get_resource(env, packed);
			if (resources[i] == NULL ||
			    resources[i]->storage.size != (UINT64)(byte_size != 0 ?
				byte_size : 4))
				goto cleanup;
			continue;
		}
		if (!accel_dx12_make_buffer(dx, byte_size,
			D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_COMMON,
			true, &buffers[i]))
			goto cleanup;
		buffers[i].elements = (uint32_t)buffer_length[i];
		if (desc->upload && byte_size != 0) {
			if (!accel_dx12_make_buffer(dx, byte_size,
				D3D12_HEAP_TYPE_UPLOAD,
				D3D12_RESOURCE_STATE_GENERIC_READ, false,
				&uploads[i]))
				goto cleanup;
			range.Begin = 0;
			range.End = 0;
			mapped = NULL;
			if (FAILED(ID3D12Resource_Map(uploads[i].resource, 0,
						 &range, &mapped)))
				goto cleanup;
			memcpy(mapped,
			       arg[desc->outer_param].val.packed->packed_buffer,
			       byte_size);
			ID3D12Resource_Unmap(uploads[i].resource, 0, NULL);
			accel_dx12_transition(&command, &buffers[i],
				D3D12_RESOURCE_STATE_COPY_DEST);
			ID3D12GraphicsCommandList_CopyBufferRegion(command.list,
				buffers[i].resource, 0, uploads[i].resource, 0,
				byte_size);
		}
		accel_dx12_transition(&command, &buffers[i],
			D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
	}
	for (i = 0; i < program->step_count; i++) {
		step = &program->step[i];
		if (trip_count[i] == 0) {
			if (step->kind == ACCEL_STEP_DOSUM_REDUCTION) {
				struct accel_dx12_buffer *zero_buffer;
				uint32_t zero;
				uint32_t result_index;

				result_index = (uint32_t)step->result_buffer;
				zero_buffer = resources[result_index] != NULL ?
					&resources[result_index]->storage :
					&buffers[result_index];
				if (uploads[result_index].resource == NULL &&
				    !accel_dx12_make_buffer(dx, 4,
					D3D12_HEAP_TYPE_UPLOAD,
					D3D12_RESOURCE_STATE_GENERIC_READ, false,
					&uploads[result_index]))
					goto cleanup;
				zero = 0;
				range.Begin = 0;
				range.End = 0;
				mapped = NULL;
				if (FAILED(ID3D12Resource_Map(
					uploads[result_index].resource, 0,
					&range, &mapped)))
					goto cleanup;
				memcpy(mapped, &zero, sizeof(zero));
				ID3D12Resource_Unmap(
					uploads[result_index].resource, 0, NULL);
				accel_dx12_transition(&command, zero_buffer,
					D3D12_RESOURCE_STATE_COPY_DEST);
				ID3D12GraphicsCommandList_CopyBufferRegion(
					command.list, zero_buffer->resource, 0,
					uploads[result_index].resource, 0, 4);
				accel_dx12_transition(&command, zero_buffer,
					D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
			}
			continue;
		}
		kernel = program->kernel[step->kernel];
		group_count = (trip_count[i] + step->block_size - 1U) /
			step->block_size;
		if (group_count > ACCEL_DX12_MAX_GROUPS) {
			group_count = ACCEL_DX12_MAX_GROUPS;
		}
		memset(binding, 0, sizeof(binding));
		for (j = 0; j < step->binding_count; j++) {
			if (step->binding[j].kind != ACCEL_BIND_BUFFER)
				continue;
			bound_buffer = (uint32_t)step->binding[j].value;
			if (step->kind == ACCEL_STEP_DOSUM_REDUCTION &&
			    step->binding[j].kernel_param ==
				(int)kernel->param_count - 1)
				bound_buffer = group_count == 1 ?
					(uint32_t)step->result_buffer :
					(uint32_t)step->scratch_buffer;
			binding[step->binding[j].kernel_param] =
				resources[bound_buffer] != NULL ?
				&resources[bound_buffer]->storage : &buffers[bound_buffer];
		}
		constant_count = 0;
		constants[constant_count++] = trip_count[i];
		constants[constant_count++] = group_count * step->block_size;
		for (j = 0; j < kernel->param_count; j++) {
			const struct accel_expr *binding_expr;
			if (kernel->param_transport[j] != ACCEL_TRANSPORT_SCALAR)
				continue;
			for (k = 0; k < step->binding_count; k++)
				if (step->binding[k].kernel_param == (int)j)
					break;
			if (k == step->binding_count ||
			    step->binding[k].kind != ACCEL_BIND_SCALAR_EXPR)
				goto cleanup;
			binding_expr = &program->expr[step->binding[k].value];
			if (binding_expr->op == ACCEL_EXPR_SCALAR_ARG &&
			    kernel->param_type[j] == NOCT_VALUE_FLOAT) {
				uint32_t outer;
				outer = (uint32_t)binding_expr->ref;
				memcpy(&constants[constant_count], &arg[outer].val.f, 4);
			} else {
				if (!accel_expr_evaluate(program,
					step->binding[k].value, arg_count,
					scalar_arg, buffer_length, &evaluated) ||
				    evaluated < INT_MIN || evaluated > INT_MAX)
					goto cleanup;
				constants[constant_count] = (uint32_t)evaluated;
			}
			constant_count++;
		}
		if (!accel_dx12_record_dispatch(dx, &command, kernel, binding,
						 constants, group_count))
			goto cleanup;
		accel_dx12_uav_barrier(&command);
		if (step->kind != ACCEL_STEP_DOSUM_REDUCTION || group_count == 1)
			continue;
		current_buffer = (uint32_t)step->scratch_buffer;
		kernel = program->kernel[step->fold_kernel];
		while (group_count > 1) {
			next_group_count = (group_count + step->block_size - 1U) /
				step->block_size;
			next_buffer = next_group_count == 1 ?
				(uint32_t)step->result_buffer :
				current_buffer == (uint32_t)step->scratch_buffer ?
				(uint32_t)step->scratch_buffer2 :
				(uint32_t)step->scratch_buffer;
			memset(binding, 0, sizeof(binding));
			binding[0] = resources[current_buffer] != NULL ?
				&resources[current_buffer]->storage : &buffers[current_buffer];
			binding[1] = resources[next_buffer] != NULL ?
				&resources[next_buffer]->storage : &buffers[next_buffer];
			constants[0] = group_count;
			constants[1] = next_group_count * step->block_size;
			constants[2] = group_count;
			if (!accel_dx12_record_dispatch(dx, &command, kernel,
					binding, constants, next_group_count))
				goto cleanup;
			accel_dx12_uav_barrier(&command);
			current_buffer = next_buffer;
			group_count = next_group_count;
		}
	}
	for (i = 0; i < program->buffer_count; i++) {
		struct accel_buffer_desc *desc;
		desc = &program->buffer[i];
		if (!desc->download)
			continue;
		byte_size = (size_t)buffer_length[i] * (size_t)desc->element_width;
		if (!accel_dx12_make_buffer(dx, byte_size,
			D3D12_HEAP_TYPE_READBACK, D3D12_RESOURCE_STATE_COPY_DEST,
			false, &readbacks[i]))
			goto cleanup;
		accel_dx12_transition(&command, &buffers[i],
			D3D12_RESOURCE_STATE_COPY_SOURCE);
		if (byte_size != 0)
			ID3D12GraphicsCommandList_CopyBufferRegion(command.list,
				readbacks[i].resource, 0, buffers[i].resource, 0,
				byte_size);
	}
	if (!accel_dx12_execute(dx, &command)) {
		rt_error(env, "DirectX 12 execution failed for program '%s'.",
			 func->name);
		result = ACCEL_DISPATCH_ERROR;
		goto cleanup;
	}
	for (i = 0; i < program->buffer_count; i++) {
		struct accel_buffer_desc *desc;
		desc = &program->buffer[i];
		if (!desc->download)
			continue;
		byte_size = (size_t)buffer_length[i] * (size_t)desc->element_width;
		if (byte_size == 0)
			continue;
		range.Begin = 0;
		range.End = byte_size;
		mapped = NULL;
		if (FAILED(ID3D12Resource_Map(readbacks[i].resource, 0,
						 &range, &mapped)))
			goto cleanup;
		memcpy(arg[desc->outer_param].val.packed->packed_buffer,
		       mapped, byte_size);
		range.Begin = 0;
		range.End = 0;
		ID3D12Resource_Unmap(readbacks[i].resource, 0, &range);
	}
	result = ACCEL_DISPATCH_OK;

cleanup:
	accel_dx12_free_command(&command);
	for (i = 0; i < program->buffer_count; i++) {
		accel_dx12_free_buffer(&readbacks[i]);
		accel_dx12_free_buffer(&uploads[i]);
		if (resources[i] == NULL)
			accel_dx12_free_buffer(&buffers[i]);
	}
	return result;
}

int
accel_dx12_dispatch(
	struct rt_env *env,
	struct rt_func *func,
	uint32_t arg_count,
	struct rt_value *arg)
{
	if (func->accel_program == NULL)
		return ACCEL_DISPATCH_FALLBACK;
	return accel_dx12_dispatch_program(env, func, arg_count, arg);
}

int
accel_dx12_dispatch_raw(
	struct rt_env *env,
	struct rt_func *func,
	uint32_t grid_size,
	uint32_t block_size,
	uint32_t arg_count,
	struct rt_value *arg,
	struct accel_event *event)
{
	struct accel_dx12_runtime *dx;
	struct accel_dx12_pipeline *pipeline;
	struct accel_dx12_command command;
	struct accel_dx12_buffer *binding[NOCT_ARG_MAX];
	struct accel_dx12_resource *resource;
	struct accel_dx12_submission *submission;
	uint32_t constants[NOCT_ARG_MAX + 2];
	uint32_t constant_count;
	uint32_t i;
	int result;

	if (func->accel_kernel == NULL ||
	    func->accel_kernel->func_kind != NOCT_FUNC_GPU ||
	    arg_count != func->accel_kernel->param_count ||
	    grid_size == 0 || grid_size > ACCEL_DX12_MAX_GROUPS ||
	    block_size == 0 || block_size > 1024)
		return ACCEL_DISPATCH_FALLBACK;
	dx = accel_dx12_get_runtime(env);
	if (dx == NULL || !accel_dx12_make_pipeline(env, dx,
			func->accel_kernel, block_size))
		return ACCEL_DISPATCH_FALLBACK;
	pipeline = func->accel_kernel->backend_data;
	memset(binding, 0, sizeof(binding));
	memset(&command, 0, sizeof(command));
	submission = NULL;
	if (!accel_dx12_begin_command(dx, pipeline->descriptor_count, &command))
		return ACCEL_DISPATCH_FALLBACK;
	result = ACCEL_DISPATCH_FALLBACK;
	constant_count = 0;
	constants[constant_count++] = grid_size;
	constants[constant_count++] = block_size;
	for (i = 0; i < arg_count; i++) {
		if (func->accel_kernel->param_transport[i] ==
		    ACCEL_TRANSPORT_SCALAR) {
			if (arg[i].type == NOCT_VALUE_FLOAT)
				memcpy(&constants[constant_count], &arg[i].val.f, 4);
			else
				constants[constant_count] = (uint32_t)arg[i].val.i;
			constant_count++;
			continue;
		}
		resource = accel_dx12_get_resource(env, arg[i].val.packed);
		if (resource == NULL)
			goto cleanup;
		binding[i] = &resource->storage;
		accel_dx12_transition(&command, binding[i],
			D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
	}
	if (!accel_dx12_record_dispatch(dx, &command, func->accel_kernel,
			binding, constants, grid_size))
		goto cleanup;
	accel_dx12_uav_barrier(&command);
	if (event != NULL) {
		submission = noct_calloc(1, sizeof(*submission));
		if (submission == NULL) {
			rt_out_of_memory(env);
			result = ACCEL_DISPATCH_ERROR;
			goto cleanup;
		}
		submission->command = command;
		memset(&command, 0, sizeof(command));
		if (!accel_dx12_submit(dx, &submission->command,
				&submission->fence_value)) {
			accel_dx12_free_command(&submission->command);
			noct_free(submission);
			submission = NULL;
			goto cleanup;
		}
		event->backend_data = submission;
	} else if (!accel_dx12_execute(dx, &command)) {
		rt_error(env, "DirectX 12 raw GPU execution failed for '%s'.",
			 func->name);
		result = ACCEL_DISPATCH_ERROR;
		goto cleanup;
	}
	result = ACCEL_DISPATCH_OK;

cleanup:
	accel_dx12_free_command(&command);
	return result;
}

bool
accel_dx12_join(
	struct rt_env *env,
	struct accel_event *event)
{
	struct accel_dx12_runtime *dx;
	struct accel_dx12_submission *submission;
	bool ok;

	submission = event->backend_data;
	if (submission == NULL)
		return true;
	dx = accel_dx12_get_runtime(env);
	ok = dx != NULL && accel_dx12_wait(dx, submission->fence_value);
	if (ok && submission->download && submission->size != 0) {
		D3D12_RANGE range;
		void *mapped;

		range.Begin = 0;
		range.End = submission->size;
		mapped = NULL;
		if (FAILED(ID3D12Resource_Map(submission->staging.resource, 0,
					     &range, &mapped))) {
			ok = false;
		} else {
			memcpy((char *)submission->source->packed_buffer +
				submission->source_offset, mapped, submission->size);
			memmove((char *)submission->destination->packed_buffer +
				submission->destination_offset, mapped,
				submission->size);
			range.Begin = 0;
			range.End = 0;
			ID3D12Resource_Unmap(submission->staging.resource, 0,
				&range);
		}
	}
	accel_dx12_free_command(&submission->command);
	accel_dx12_free_buffer(&submission->staging);
	noct_free(submission);
	event->backend_data = NULL;
	if (!ok)
		rt_error(env, "DirectX 12 accelerator event wait failed.");
	return ok;
}

int
accel_dx12_copy_async(
	struct rt_env *env,
	bool to_accel,
	struct rt_packed *source,
	size_t source_offset,
	struct rt_packed *destination,
	size_t destination_offset,
	size_t size,
	struct accel_event *event)
{
	struct accel_dx12_runtime *dx;
	struct accel_dx12_resource *resource;
	struct accel_dx12_submission *submission;
	struct rt_packed *device_packed;
	D3D12_RANGE range;
	void *mapped;

	device_packed = to_accel ? destination : source;
	resource = accel_dx12_get_resource(env, device_packed);
	if (resource == NULL)
		return ACCEL_DISPATCH_FALLBACK;
	if (size == 0)
		return ACCEL_DISPATCH_OK;
	dx = accel_dx12_get_runtime(env);
	if (dx == NULL)
		return ACCEL_DISPATCH_FALLBACK;
	submission = noct_calloc(1, sizeof(*submission));
	if (submission == NULL) {
		rt_out_of_memory(env);
		return ACCEL_DISPATCH_ERROR;
	}
	submission->source = source;
	submission->destination = destination;
	submission->source_offset = source_offset;
	submission->destination_offset = destination_offset;
	submission->size = size;
	submission->download = !to_accel;
	if (!accel_dx12_make_buffer(dx, (UINT64)size,
			to_accel ? D3D12_HEAP_TYPE_UPLOAD :
				D3D12_HEAP_TYPE_READBACK,
			to_accel ? D3D12_RESOURCE_STATE_GENERIC_READ :
				D3D12_RESOURCE_STATE_COPY_DEST,
			false, &submission->staging) ||
	    !accel_dx12_begin_command(dx, 0, &submission->command))
		goto failed;
	if (to_accel) {
		memmove((char *)destination->packed_buffer + destination_offset,
			(char *)source->packed_buffer + source_offset, size);
		range.Begin = 0;
		range.End = 0;
		mapped = NULL;
		if (FAILED(ID3D12Resource_Map(submission->staging.resource, 0,
					     &range, &mapped)))
			goto failed;
		memcpy(mapped, (char *)source->packed_buffer + source_offset, size);
		ID3D12Resource_Unmap(submission->staging.resource, 0, NULL);
		accel_dx12_transition(&submission->command, &resource->storage,
			D3D12_RESOURCE_STATE_COPY_DEST);
		ID3D12GraphicsCommandList_CopyBufferRegion(
			submission->command.list, resource->storage.resource,
			(UINT64)destination_offset, submission->staging.resource,
			0, (UINT64)size);
	} else {
		accel_dx12_transition(&submission->command, &resource->storage,
			D3D12_RESOURCE_STATE_COPY_SOURCE);
		ID3D12GraphicsCommandList_CopyBufferRegion(
			submission->command.list, submission->staging.resource, 0,
			resource->storage.resource, (UINT64)source_offset,
			(UINT64)size);
	}
	if (!accel_dx12_submit(dx, &submission->command,
			&submission->fence_value))
		goto failed;
	event->backend_data = submission;
	event->retained[0].type = NOCT_VALUE_PACKED;
	event->retained[0].val.packed = source;
	event->retained[1].type = NOCT_VALUE_PACKED;
	event->retained[1].val.packed = destination;
	event->retained_count = 2;
	return ACCEL_DISPATCH_OK;

failed:
	accel_dx12_free_command(&submission->command);
	accel_dx12_free_buffer(&submission->staging);
	noct_free(submission);
	return ACCEL_DISPATCH_ERROR;
}

int
accel_dx12_copy_to(
	struct rt_env *env,
	struct rt_packed *resource,
	size_t offset,
	size_t size)
{
	struct accel_dx12_runtime *dx;
	struct accel_dx12_resource *dx_resource;

	dx_resource = accel_dx12_get_resource(env, resource);
	if (dx_resource == NULL)
		return ACCEL_DISPATCH_FALLBACK;
	if (offset > dx_resource->storage.size ||
	    size > dx_resource->storage.size - offset) {
		rt_error(env,
			 "DirectX 12 accelerator upload range is out-of-bounds.");
		return ACCEL_DISPATCH_ERROR;
	}
	dx = accel_dx12_get_runtime(env);
	if (dx == NULL || !accel_dx12_copy_range(dx, &dx_resource->storage,
			(char *)resource->packed_buffer + offset,
			offset, size, true)) {
		rt_error(env, "DirectX 12 accelerator upload failed.");
		return ACCEL_DISPATCH_ERROR;
	}
	return ACCEL_DISPATCH_OK;
}

int
accel_dx12_copy_from(
	struct rt_env *env,
	struct rt_packed *resource,
	size_t offset,
	size_t size)
{
	struct accel_dx12_runtime *dx;
	struct accel_dx12_resource *dx_resource;

	dx_resource = accel_dx12_get_resource(env, resource);
	if (dx_resource == NULL)
		return ACCEL_DISPATCH_FALLBACK;
	if (offset > dx_resource->storage.size ||
	    size > dx_resource->storage.size - offset) {
		rt_error(env,
			 "DirectX 12 accelerator download range is out-of-bounds.");
		return ACCEL_DISPATCH_ERROR;
	}
	dx = accel_dx12_get_runtime(env);
	if (dx == NULL || !accel_dx12_copy_range(dx, &dx_resource->storage,
			(char *)resource->packed_buffer + offset,
			offset, size, false)) {
		rt_error(env, "DirectX 12 accelerator download failed.");
		return ACCEL_DISPATCH_ERROR;
	}
	return ACCEL_DISPATCH_OK;
}

void
accel_dx12_cleanup(
	struct rt_vm *vm)
{
	struct rt_func *func;
	struct accel_dx12_pipeline *pipeline;
	struct accel_dx12_pipeline *next_pipeline;
	uint32_t i;

	if (vm->accel_runtime == NULL)
		return;
	if (vm->env_list != NULL) {
		for (i = 0; i < ACCEL_EVENT_MAX; i++) {
			if (vm->accel_event[i].state == ACCEL_EVENT_SUBMITTED &&
			    vm->accel_event[i].backend_data != NULL) {
				(void)accel_dx12_join(vm->env_list,
					&vm->accel_event[i]);
				vm->accel_event[i].state = ACCEL_EVENT_JOINED;
			}
		}
	}
	func = vm->func_list;
	while (func != NULL) {
		if (func->accel_kernel != NULL &&
		    func->accel_kernel->backend_data != NULL) {
			pipeline = func->accel_kernel->backend_data;
			while (pipeline != NULL) {
				next_pipeline = pipeline->next;
				if (pipeline->pipeline != NULL)
					ID3D12PipelineState_Release(pipeline->pipeline);
				if (pipeline->root_signature != NULL)
					ID3D12RootSignature_Release(
						pipeline->root_signature);
				noct_free(pipeline);
				pipeline = next_pipeline;
			}
			func->accel_kernel->backend_data = NULL;
		}
		if (func->accel_program != NULL) {
			for (i = 0; i < func->accel_program->kernel_count; i++) {
				if (func->accel_program->kernel[i]->backend_data == NULL)
					continue;
				pipeline = func->accel_program->kernel[i]->backend_data;
				while (pipeline != NULL) {
					next_pipeline = pipeline->next;
					if (pipeline->pipeline != NULL)
						ID3D12PipelineState_Release(
							pipeline->pipeline);
					if (pipeline->root_signature != NULL)
						ID3D12RootSignature_Release(
							pipeline->root_signature);
					noct_free(pipeline);
					pipeline = next_pipeline;
				}
				func->accel_program->kernel[i]->backend_data = NULL;
			}
		}
		func = func->next;
	}
	accel_dx12_destroy_runtime(vm->accel_runtime);
	vm->accel_runtime = NULL;
}
