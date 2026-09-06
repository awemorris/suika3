/* -*- coding: utf-8; tab-width: 8; indent-tabs-mode: t; -*- */

/*
 * Noct Programming Language
 * Copyright (C) 2025, 2026 Awe Morris
 *
 * SPDX-License-Identifier: Zlib
 */

/*
 * Synchronous Metal accelerator backend.
 */

#include "accel_metal.h"
#include "accel_context.h"
#include "accel_runtime.h"
#include "accel_shader_source.h"
#include "hir.h"
#include "runtime.h"

#import <Foundation/Foundation.h>
#import <Metal/Metal.h>

#include <limits.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#define ACCEL_METAL_WORKGROUP_SIZE	64U
#define ACCEL_METAL_BACKEND_PRIORITY	400U
#define ACCEL_METAL_DEVICE_SCORE		400U
#define ACCEL_METAL_BUFFER_ARGUMENT_LIMIT	31U

struct accel_metal_backend {
	id<MTLDevice> device;
	id<MTLCommandQueue> queue;
};

struct accel_metal_kernel {
	id<MTLComputePipelineState> pipeline;
};

struct accel_metal_prepared {
	struct accel_program *program;
	struct accel_metal_kernel *kernel;
	uint32_t kernel_count;
};

struct accel_metal_buffer {
	id<MTLBuffer> buffer;
	int origin;
	uint32_t args_slot;
	int element_kind;
	uint32_t element_width;
	size_t element_count;
	size_t byte_count;
	bool active;
	bool upload;
	bool download;
};

struct accel_metal_execution {
	struct accel_metal_backend *backend;
	const struct accel_metal_prepared *prepared;
	id<MTLCommandBuffer> command_buffer;
	struct accel_metal_buffer *buffer;
	id<MTLBuffer> scalar_buffer;
	id<MTLBuffer> result_buffer;
	uint32_t buffer_count;
	uint32_t scalar_word_count;
	uint32_t result_word_count;
	uint32_t expected_dispatch_count;
	uint32_t dispatch_count;
	uint32_t last_kernel;
	bool has_active_dispatch;
	bool dispatched;
	bool submitted;
	bool completed;
};

static bool accel_metal_error(char *error, size_t error_size, const char *message);
static void accel_metal_rollback_devices(struct accel_device_list *list, uint32_t count);
static bool accel_metal_enumerate_impl(struct accel_device_list *list, char *error, size_t error_size);
static id<MTLDevice> accel_metal_find_device(NSArray *devices, const char *name);
static bool accel_metal_create_selected_impl(struct rt_env *env, const struct accel_device *device, const struct accel_backend_ops **ops, void **backend_state);
static const struct accel_backend_ops *accel_metal_backend_ops(void);
static const struct accel_executor_ops *accel_metal_executor_ops(void);
static enum accel_compile_status accel_metal_prepare_program(void *backend_state, const struct accel_program *program, struct accel_prepared_program *result);
static enum accel_compile_status accel_metal_prepare_program_impl(void *backend_state, const struct accel_program *program, struct accel_prepared_program *result);
static bool accel_metal_program_uses_f32(const struct accel_program *program);
static bool accel_metal_prepare_kernel(struct accel_metal_backend *backend, const struct accel_program *program, uint32_t kernel_index, struct accel_metal_kernel *result);
static void accel_metal_destroy_prepared(struct accel_metal_prepared *prepared);
static void accel_metal_destroy_prepared_program(void *backend_state, struct accel_prepared_program *program);
static bool accel_metal_register_runtime(struct accel_context *context, struct rt_env *env);
static void accel_metal_destroy_backend_state(void *backend_state);
static const struct accel_program *accel_metal_get_program(const struct accel_prepared_program *prepared);
static bool accel_metal_validate_dispatch_limit(void *backend_state, const struct accel_prepared_program *prepared, uint32_t kernel_index, uint32_t start, uint32_t trip, char *error, size_t error_size);
static uint32_t accel_metal_count_active_dispatches(const struct accel_program *program, const uint32_t scalar_word[]);
static bool accel_metal_create_execution(void *backend_state, const struct accel_prepared_program *prepared, uint32_t scalar_word_count, const uint32_t scalar_word[], uint32_t result_word_count, const uint32_t result_word[], uint32_t buffer_count, const struct accel_runtime_buffer buffer[], void **execution, char *error, size_t error_size);
static bool accel_metal_create_execution_impl(void *backend_state, const struct accel_prepared_program *prepared, uint32_t scalar_word_count, const uint32_t scalar_word[], uint32_t result_word_count, const uint32_t result_word[], uint32_t buffer_count, const struct accel_runtime_buffer buffer[], void **execution, char *error, size_t error_size);
static bool accel_metal_validate_execution_inputs(struct accel_metal_backend *backend, const struct accel_metal_prepared *prepared, uint32_t scalar_word_count, const uint32_t scalar_word[], uint32_t result_word_count, const uint32_t result_word[], uint32_t buffer_count, const struct accel_runtime_buffer buffer[], char *error, size_t error_size);
static bool accel_metal_create_buffer(struct accel_metal_backend *backend, size_t byte_count, const void *data, id<MTLBuffer> *result, char *error, size_t error_size);
static bool accel_metal_dispatch_execution(void *execution, uint32_t kernel_index, uint32_t start, uint32_t trip, char *error, size_t error_size);
static bool accel_metal_dispatch_execution_impl(void *execution, uint32_t kernel_index, uint32_t start, uint32_t trip, char *error, size_t error_size);
static bool accel_metal_finish_execution(void *execution, uint32_t result_word_count, uint32_t result_word[], uint32_t buffer_count, struct accel_runtime_buffer buffer[], char *error, size_t error_size);
static bool accel_metal_finish_execution_impl(void *execution, uint32_t result_word_count, uint32_t result_word[], uint32_t buffer_count, struct accel_runtime_buffer buffer[], char *error, size_t error_size);
static bool accel_metal_validate_finish(const struct accel_metal_execution *execution, uint32_t result_word_count, const uint32_t result_word[], uint32_t buffer_count, const struct accel_runtime_buffer buffer[], char *error, size_t error_size);
static void accel_metal_destroy_execution(void *execution);

/*
 * Enumerates suitable hardware Metal devices.
 */
bool
accel_metal_enumerate(
	struct accel_device_list *list,
	char *error,
	size_t error_size)
{
	bool success;

	/* Bounds all temporary Foundation objects to this enumeration call. */
	@autoreleasepool {
		success = accel_metal_enumerate_impl(
			list,
			error,
			error_size);
	}

	/* Report the complete enumeration transaction. */
	return success;
}

/*
 * Creates a Metal backend for one selected device record.
 */
bool
accel_metal_create_selected(
	struct rt_env *env,
	const struct accel_device *device,
	const struct accel_backend_ops **ops,
	void **backend_state)
{
	bool success;

	/* Bounds temporary Foundation objects to device selection. */
	@autoreleasepool {
		success = accel_metal_create_selected_impl(
			env,
			device,
			ops,
			backend_state);
	}

	/* Report the ownership-transfer result. */
	return success;
}

/* Return the immutable Metal backend operation table. */
static const struct accel_backend_ops *
accel_metal_backend_ops(void)
{
	static const struct accel_backend_ops ops = {
		accel_metal_prepare_program,
		accel_metal_destroy_prepared_program,
		accel_metal_register_runtime,
		accel_metal_destroy_backend_state
	};

	/* Return the process-lifetime backend operations. */
	return &ops;
}

/* Return the immutable Metal executor operation table. */
static const struct accel_executor_ops *
accel_metal_executor_ops(void)
{
	static const struct accel_executor_ops ops = {
		"Metal",
		accel_metal_get_program,
		accel_metal_validate_dispatch_limit,
		accel_metal_create_execution,
		accel_metal_dispatch_execution,
		accel_metal_finish_execution,
		accel_metal_destroy_execution
	};

	/* Return the process-lifetime executor operations. */
	return &ops;
}

/* Copy one stable Metal diagnostic into optional caller storage. */
static bool
accel_metal_error(
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

/* Remove device records appended by an incomplete Metal enumeration. */
static void
accel_metal_rollback_devices(
	struct accel_device_list *list,
	uint32_t count)
{
	struct accel_device *device;

	/* Release deep-owned records in reverse append order. */
	while (list->count > count) {
		list->count--;
		device = &list->device[list->count];
		noct_free(device->selector);
		noct_free(device->name);
		memset(device, 0, sizeof(*device));
	}
}

/* Enumerate Metal devices while one autorelease pool is active. */
static bool
accel_metal_enumerate_impl(
	struct accel_device_list *list,
	char *error,
	size_t error_size)
{
	NSArray *devices;
	id<MTLDevice> candidate;
	NSString *device_name;
	const char *name;
	NSUInteger count;
	NSUInteger i;
	uint32_t initial_count;
	bool appended;

	/* Clear an optional stale diagnostic before opening Metal. */
	if (error != NULL && error_size != 0)
		error[0] = '\0';

	/* Reject an absent shared device registry. */
	if (list == NULL)
		return accel_metal_error(error, error_size, "Invalid Metal device list.");

	/* Acquire one retained snapshot of all hardware Metal devices. */
	devices = MTLCopyAllDevices();
	if (devices == nil)
		return accel_metal_error(error, error_size, "Failed to enumerate Metal devices.");

	initial_count = list->count;
	count = [devices count];

	/* Append each named hardware device in Metal's preferred order. */
	for (i = 0; i < count; i++) {
		candidate = [devices objectAtIndex:i];
		device_name = [candidate name];
		name = [device_name UTF8String];
		if (candidate == nil ||
		    name == NULL ||
		    name[0] == '\0') {
			continue;
		}

		appended = accel_device_list_append(
			list,
			ACCEL_BACKEND_METAL,
			name,
			ACCEL_METAL_BACKEND_PRIORITY,
			ACCEL_METAL_DEVICE_SCORE,
			(uintptr_t)i);
		if (!appended) {
			accel_metal_rollback_devices(list, initial_count);
			[devices release];
			return accel_metal_error(error, error_size, "Out of memory while enumerating Metal devices.");
		}
	}

	/* Release the enumeration snapshot after all names are deep-owned. */
	[devices release];

	/* Report a complete enumeration, including an empty result. */
	return true;
}

/* Find one exact device name in a retained enumeration snapshot. */
static id<MTLDevice>
accel_metal_find_device(
	NSArray *devices,
	const char *name)
{
	id<MTLDevice> candidate;
	const char *candidate_name;
	NSUInteger count;
	NSUInteger i;

	/* Reject an incomplete selection key. */
	if (devices == nil || name == NULL)
		return nil;

	/* Match the canonical record's exact UTF-8 hardware name. */
	count = [devices count];
	for (i = 0; i < count; i++) {
		candidate = [devices objectAtIndex:i];
		candidate_name = [[candidate name] UTF8String];
		if (candidate_name != NULL && strcmp(candidate_name, name) == 0)
			return candidate;
	}

	/* Report that the selected device disappeared. */
	return nil;
}

/* Create a selected backend while one autorelease pool is active. */
static bool
accel_metal_create_selected_impl(
	struct rt_env *env,
	const struct accel_device *device,
	const struct accel_backend_ops **ops,
	void **backend_state)
{
	struct accel_metal_backend *backend;
	NSArray *devices;
	id<MTLDevice> selected;

	/* Clear ownership outputs before validating the request. */
	if (ops != NULL)
		*ops = NULL;
	if (backend_state != NULL)
		*backend_state = NULL;

	/* Reject incomplete ownership and runtime arguments. */
	if (env == NULL ||
	    ops == NULL ||
	    backend_state == NULL) {
		return false;
	}

	/* Require a record produced by the Metal enumerator. */
	if (device == NULL ||
	    device->backend != ACCEL_BACKEND_METAL ||
	    device->name == NULL ||
	    device->name[0] == '\0') {
		rt_error(env, N_TR("Invalid selected Metal device."));
		return false;
	}

	/* Re-enumerate and resolve the selected hardware by its exact name. */
	devices = MTLCopyAllDevices();
	if (devices == nil) {
		rt_error(env, N_TR("Failed to enumerate Metal devices."));
		return false;
	}
	selected = accel_metal_find_device(devices, device->name);
	if (selected == nil) {
		[devices release];
		rt_error(env, N_TR("The selected Metal device is no longer available."));
		return false;
	}

	/* Allocate and retain the selected device owner. */
	backend = noct_calloc(1, sizeof(*backend));
	if (backend == NULL) {
		[devices release];
		rt_out_of_memory(env);
		return false;
	}
	backend->device = [selected retain];
	[devices release];

	/* Create one reusable command queue before publishing the backend. */
	backend->queue = [backend->device newCommandQueue];
	if (backend->queue == nil) {
		accel_metal_destroy_backend_state(backend);
		rt_error(env, N_TR("Failed to create a Metal command queue."));
		return false;
	}

	/* Transfer the complete backend to the accelerator context. */
	*ops = accel_metal_backend_ops();
	*backend_state = backend;

	/* Report successful backend publication. */
	return true;
}

/* Prepare one program inside an autorelease pool. */
static enum accel_compile_status
accel_metal_prepare_program(
	void *backend_state,
	const struct accel_program *program,
	struct accel_prepared_program *result)
{
	enum accel_compile_status status;

	/* Bounds compiler-created Foundation objects to this transaction. */
	@autoreleasepool {
		status = accel_metal_prepare_program_impl(
			backend_state,
			program,
			result);
	}

	/* Report the transactional compilation result. */
	return status;
}

/* Prepare every immutable Metal pipeline for one accelerator region. */
static enum accel_compile_status
accel_metal_prepare_program_impl(
	void *backend_state,
	const struct accel_program *program,
	struct accel_prepared_program *result)
{
	struct accel_metal_backend *backend;
	struct accel_metal_prepared *prepared;
	char validation_error[128];
	uint32_t binding_count;
	uint32_t i;

	/* Clear the opaque publication slot before validation. */
	if (result == NULL)
		return ACCEL_COMPILE_ERROR;
	result->payload = NULL;
	backend = backend_state;

	/* Reject an incomplete compiler-to-backend request. */
	if (backend == NULL ||
	    backend->device == nil ||
	    program == NULL) {
		hir_error(0, N_TR("Invalid Metal program preparation request."));
		return ACCEL_COMPILE_ERROR;
	}

	/* Validate the complete target-neutral plan before device compilation. */
	if (!accel_program_validate(
		program,
		validation_error,
		sizeof(validation_error))) {
		hir_error(program->source_line, N_TR("Invalid accelerator program reached the Metal backend."));
		return ACCEL_COMPILE_ERROR;
	}

	/* Decline Float32 until strict Noct controls are guaranteed by Metal. */
	if (accel_metal_program_uses_f32(program))
		return ACCEL_COMPILE_DECLINED;

	/* Reserve one final Metal buffer argument for immutable scalar words. */
	binding_count = program->buffer_count + 1;
	if (program->scalar_result_count != 0)
		binding_count++;
	if (binding_count > ACCEL_METAL_BUFFER_ARGUMENT_LIMIT)
		return ACCEL_COMPILE_DECLINED;

	/* Allocate the deep-owned prepared-program wrapper. */
	prepared = noct_calloc(1, sizeof(*prepared));
	if (prepared == NULL) {
		hir_out_of_memory();
		return ACCEL_COMPILE_ERROR;
	}
	prepared->kernel_count = program->kernel_count;

	/* Clone all target-neutral metadata before creating Metal objects. */
	prepared->program = accel_program_clone(program);
	if (prepared->program == NULL) {
		noct_free(prepared);
		hir_out_of_memory();
		return ACCEL_COMPILE_ERROR;
	}

	/* Allocate one immutable pipeline slot per typed kernel. */
	if (prepared->kernel_count != 0) {
		prepared->kernel = noct_calloc(
			prepared->kernel_count,
			sizeof(*prepared->kernel));
		if (prepared->kernel == NULL) {
			accel_program_destroy(prepared->program);
			noct_free(prepared);
			hir_out_of_memory();
			return ACCEL_COMPILE_ERROR;
		}
	}

	/* Generate and compile every kernel before publishing any payload. */
	for (i = 0; i < prepared->kernel_count; i++) {
		if (!accel_metal_prepare_kernel(
			backend,
			program,
			i,
			&prepared->kernel[i])) {
			accel_metal_destroy_prepared(prepared);
			return ACCEL_COMPILE_ERROR;
		}
	}

	/* Publish the complete immutable backend payload. */
	result->payload = prepared;

	/* Report transactional preparation success. */
	return ACCEL_COMPILE_APPLIED;
}

/* Detect every Float32 use that lacks an explicit strict Metal contract. */
static bool
accel_metal_program_uses_f32(
	const struct accel_program *program)
{
	const struct accel_ir_kernel *kernel;
	uint32_t i;
	uint32_t j;

	/* Inspect all uniform scalar declarations. */
	for (i = 0; i < program->scalar_count; i++) {
		if (program->scalar[i].value_type == ACCEL_IR_F32)
			return true;
	}

	/* Inspect every buffer and instruction type in every kernel. */
	for (i = 0; i < program->kernel_count; i++) {
		kernel = program->kernel[i].ir;
		for (j = 0; j < kernel->buffer_binding_count; j++) {
			if (kernel->buffer_value_type[j] == ACCEL_IR_F32)
				return true;
		}
		for (j = 0; j < kernel->instruction_count; j++) {
			if (kernel->instruction[j].result_type == ACCEL_IR_F32)
				return true;
		}
	}

	/* Report an integer-only program. */
	return false;
}

/* Generate MSL and create one immutable compute pipeline. */
static bool
accel_metal_prepare_kernel(
	struct accel_metal_backend *backend,
	const struct accel_program *program,
	uint32_t kernel_index,
	struct accel_metal_kernel *result)
{
	struct accel_shader_source source;
	char source_error[128];
	NSString *text;
	id<MTLLibrary> library;
	id<MTLFunction> function;
	id<MTLComputePipelineState> pipeline;
	NSError *metal_error;
	bool generated;

	memset(&source, 0, sizeof(source));
	result->pipeline = nil;
	text = nil;
	library = nil;
	function = nil;
	pipeline = nil;
	metal_error = nil;

	/* Generate deterministic MSL from the typed target-neutral kernel. */
	generated = accel_shader_source_generate(
		ACCEL_SHADER_SOURCE_MSL,
		program,
		kernel_index,
		&source,
		source_error,
		sizeof(source_error));
	if (!generated) {
		hir_error(program->kernel[kernel_index].source_line, N_TR("Failed to generate a Metal compute shader."));
		return false;
	}

	/* Convert exact generated UTF-8 bytes into one retained source string. */
	text = [[NSString alloc]
		initWithBytes:source.data
		length:source.length
		encoding:NSUTF8StringEncoding];
	accel_shader_source_cleanup(&source);
	if (text == nil) {
		hir_error(program->kernel[kernel_index].source_line, N_TR("Failed to encode a Metal compute shader."));
		return false;
	}

	/* Compile one retained Metal library from the generated source. */
	library = [backend->device
		newLibraryWithSource:text
		options:nil
		error:&metal_error];
	[text release];
	if (library == nil) {
		hir_error(program->kernel[kernel_index].source_line, N_TR("Failed to compile a Metal compute shader."));
		return false;
	}

	/* Resolve the fixed private entry point from the compiled library. */
	function = [library newFunctionWithName:@"noct_main"];
	if (function == nil) {
		[library release];
		hir_error(program->kernel[kernel_index].source_line, N_TR("Failed to resolve the Metal compute entry point."));
		return false;
	}

	/* Create the complete immutable compute pipeline transactionally. */
	metal_error = nil;
	pipeline = [backend->device
		newComputePipelineStateWithFunction:function
		error:&metal_error];
	[function release];
	[library release];
	if (pipeline == nil) {
		hir_error(program->kernel[kernel_index].source_line, N_TR("Failed to create a Metal compute pipeline."));
		return false;
	}

	/* Require the fixed target-neutral 64-lane workgroup contract. */
	if ([pipeline maxTotalThreadsPerThreadgroup] <
	    (NSUInteger)ACCEL_METAL_WORKGROUP_SIZE) {
		[pipeline release];
		hir_error(program->kernel[kernel_index].source_line, N_TR("The Metal device cannot execute 64-lane accelerator workgroups."));
		return false;
	}

	/* Publish the complete retained pipeline. */
	result->pipeline = pipeline;

	/* Report successful kernel preparation. */
	return true;
}

/* Destroy one partially or completely prepared Metal payload. */
static void
accel_metal_destroy_prepared(
	struct accel_metal_prepared *prepared)
{
	uint32_t i;

	/* Accept cleanup of an optional transaction. */
	if (prepared == NULL)
		return;

	/* Release every retained pipeline in binding order. */
	if (prepared->kernel != NULL) {
		for (i = 0; i < prepared->kernel_count; i++) {
			[prepared->kernel[i].pipeline release];
			prepared->kernel[i].pipeline = nil;
		}
	}

	/* Release all target-neutral and wrapper ownership. */
	accel_program_destroy(prepared->program);
	noct_free(prepared->kernel);
	noct_free(prepared);
}

/* Destroy one backend-prepared program and clear its opaque slot. */
static void
accel_metal_destroy_prepared_program(
	void *backend_state,
	struct accel_prepared_program *program)
{
	struct accel_metal_prepared *prepared;

	UNUSED_PARAMETER(backend_state);

	/* Accept cleanup of an absent or already-cleared publication slot. */
	if (program == NULL || program->payload == NULL)
		return;

	/* Detach and destroy the complete retained payload. */
	prepared = program->payload;
	program->payload = NULL;
	accel_metal_destroy_prepared(prepared);
}

/* Register the shared private accelerator runtime with Metal callbacks. */
static bool
accel_metal_register_runtime(
	struct accel_context *context,
	struct rt_env *env)
{
	bool success;

	/* Copy the target-specific executor table into VM-owned metadata. */
	success = accel_runtime_register(
		context,
		env,
		accel_metal_executor_ops());

	/* Report the shared package publication result. */
	return success;
}

/* Destroy the selected device after all programs and executions. */
static void
accel_metal_destroy_backend_state(
	void *backend_state)
{
	struct accel_metal_backend *backend;

	backend = backend_state;

	/* Accept cleanup of an absent backend. */
	if (backend == NULL)
		return;

	/* Release retained Metal objects in reverse ownership order. */
	[backend->queue release];
	[backend->device release];
	backend->queue = nil;
	backend->device = nil;
	noct_free(backend);
}

/* Borrow target-neutral metadata from one prepared Metal payload. */
static const struct accel_program *
accel_metal_get_program(
	const struct accel_prepared_program *prepared)
{
	const struct accel_metal_prepared *payload;

	/* Reject an absent opaque publication slot. */
	if (prepared == NULL || prepared->payload == NULL)
		return NULL;

	payload = prepared->payload;

	/* Return the immutable deep-owned program plan. */
	return payload->program;
}

/* Validate one checked dispatch against the Metal ABI limits. */
static bool
accel_metal_validate_dispatch_limit(
	void *backend_state,
	const struct accel_prepared_program *prepared,
	uint32_t kernel_index,
	uint32_t start,
	uint32_t trip,
	char *error,
	size_t error_size)
{
	const struct accel_metal_prepared *payload;

	UNUSED_PARAMETER(start);

	/* Validate backend and program ownership before applying limits. */
	if (backend_state == NULL ||
	    prepared == NULL ||
	    prepared->payload == NULL) {
		return accel_metal_error(error, error_size, "Invalid Metal dispatch limit request.");
	}
	payload = prepared->payload;
	if (kernel_index >= payload->kernel_count)
		return accel_metal_error(error, error_size, "Invalid Metal kernel index.");

	/* Empty source loops require no device dispatch. */
	if (trip == 0)
		return true;

	/* Every checked 32-bit trip count is representable by Metal grids. */
	return true;
}

/* Count nonempty dispatch ranges in one validated scalar block. */
static uint32_t
accel_metal_count_active_dispatches(
	const struct accel_program *program,
	const uint32_t scalar_word[])
{
	uint32_t active_count;
	uint32_t range_word;
	uint32_t i;

	active_count = 0;

	/* Counts each nonzero trip word in immutable source order. */
	for (i = 0; i < program->kernel_count; i++) {
		range_word = program->scalar_count + i * 2;

		/* Records only ranges that require a device dispatch. */
		if (scalar_word[range_word + 1] != 0)
			active_count++;
	}

	/* Return the exact number of dispatch callbacks expected at finish. */
	return active_count;
}

/* Create one execution inside an autorelease pool. */
static bool
accel_metal_create_execution(
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
	bool success;

	/* Bounds command-buffer and Foundation temporaries to this callback. */
	@autoreleasepool {
		success = accel_metal_create_execution_impl(
			backend_state,
			prepared,
			scalar_word_count,
			scalar_word,
			result_word_count,
			result_word,
			buffer_count,
			buffer,
			execution,
			error,
			error_size);
	}

	/* Report the complete resource-creation transaction. */
	return success;
}

/* Create all shared Metal resources from runtime-owned snapshots. */
static bool
accel_metal_create_execution_impl(
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
	struct accel_metal_backend *backend;
	const struct accel_metal_prepared *payload;
	struct accel_metal_execution *created;
	const void *upload_data;
	size_t allocation_byte_count;
	size_t scalar_byte_count;
	uint32_t i;

	/* Clear the opaque output before validating runtime snapshots. */
	if (execution != NULL)
		*execution = NULL;
	if (execution == NULL)
		return accel_metal_error(error, error_size, "Missing Metal execution output.");

	backend = backend_state;

	/* Recover immutable program metadata from the published payload. */
	if (prepared == NULL || prepared->payload == NULL)
		return accel_metal_error(error, error_size, "Missing Metal prepared program.");
	payload = prepared->payload;

	/* Validate all plain input arrays before allocating Metal objects. */
	if (!accel_metal_validate_execution_inputs(
		backend,
		payload,
		scalar_word_count,
		scalar_word,
		result_word_count,
		result_word,
		buffer_count,
		buffer,
		error,
		error_size)) {
		return false;
	}

	/* Allocate one execution wrapper and its buffer ownership table. */
	created = noct_calloc(1, sizeof(*created));
	if (created == NULL)
		return accel_metal_error(error, error_size, "Out of memory while creating a Metal execution.");
	created->backend = backend;
	created->prepared = payload;
	created->buffer_count = buffer_count;
	created->scalar_word_count = scalar_word_count;
	created->result_word_count = result_word_count;
	created->expected_dispatch_count =
		accel_metal_count_active_dispatches(
			payload->program,
			scalar_word);
	created->has_active_dispatch =
		created->expected_dispatch_count != 0;

	/* Allocate immutable metadata storage for every data binding. */
	if (buffer_count != 0) {
		created->buffer = noct_calloc(
			buffer_count,
			sizeof(*created->buffer));
		if (created->buffer == NULL) {
			noct_free(created);
			return accel_metal_error(error, error_size, "Out of memory while creating Metal buffers.");
		}
	}

	/* Snapshot the immutable metadata for every declared data binding. */
	for (i = 0; i < buffer_count; i++) {
		created->buffer[i].origin = buffer[i].origin;
		created->buffer[i].args_slot = buffer[i].args_slot;
		created->buffer[i].element_kind = buffer[i].element_kind;
		created->buffer[i].element_width = buffer[i].element_width;
		created->buffer[i].element_count = buffer[i].element_count;
		created->buffer[i].byte_count = buffer[i].byte_count;
		created->buffer[i].active = buffer[i].active;
		created->buffer[i].upload = buffer[i].upload;
		created->buffer[i].download = buffer[i].download;
	}

	/* Publish an all-empty execution without creating any GPU resource. */
	if (!created->has_active_dispatch) {
		*execution = created;
		return true;
	}

	/* Create each data buffer or one legal dummy for an inactive declaration. */
	for (i = 0; i < buffer_count; i++) {
		allocation_byte_count = buffer[i].active ?
			buffer[i].byte_count : 0;
		upload_data = buffer[i].active && buffer[i].upload ?
			buffer[i].snapshot : NULL;
		if (!accel_metal_create_buffer(
			backend,
			allocation_byte_count,
			upload_data,
			&created->buffer[i].buffer,
			error,
			error_size)) {
			accel_metal_destroy_execution(created);
			return false;
		}
	}

	/* Create the immutable scalar and dispatch-range buffer last. */
	scalar_byte_count = (size_t)scalar_word_count * sizeof(*scalar_word);
	if (!accel_metal_create_buffer(
		backend,
		scalar_byte_count,
		scalar_word,
		&created->scalar_buffer,
		error,
		error_size)) {
		accel_metal_destroy_execution(created);
		return false;
	}

	/* Create the mutable shared result buffer only when the ABI declares it. */
	if (result_word_count != 0) {
		if (!accel_metal_create_buffer(
			backend,
			(size_t)result_word_count * sizeof(*result_word),
			result_word,
			&created->result_buffer,
			error,
			error_size)) {
			accel_metal_destroy_execution(created);
			return false;
		}
	}

	/* Retain one command buffer for source-ordered kernel encoders. */
	created->command_buffer = [[backend->queue commandBuffer] retain];
	if (created->command_buffer == nil) {
		accel_metal_destroy_execution(created);
		return accel_metal_error(error, error_size, "Failed to create a Metal command buffer.");
	}

	/* Publish the plain backend execution to the shared runtime. */
	*execution = created;

	/* Report successful resource creation. */
	return true;
}

/* Validate execution arrays, exact ABI word counts, and Metal sizes. */
static bool
accel_metal_validate_execution_inputs(
	struct accel_metal_backend *backend,
	const struct accel_metal_prepared *prepared,
	uint32_t scalar_word_count,
	const uint32_t scalar_word[],
	uint32_t result_word_count,
	const uint32_t result_word[],
	uint32_t buffer_count,
	const struct accel_runtime_buffer buffer[],
	char *error,
	size_t error_size)
{
	const struct accel_buffer_binding *binding;
	size_t expected_scalar_count;
	size_t result_byte_count;
	size_t max_buffer_length;
	uint32_t i;
	bool has_active_dispatch;

	/* Require the selected backend and immutable target-neutral plan. */
	if (backend == NULL ||
	    backend->device == nil ||
	    prepared == NULL ||
	    prepared->program == NULL) {
		return accel_metal_error(error, error_size, "Invalid Metal execution request.");
	}

	/* Match every data buffer to the compiled binding namespace. */
	if (buffer_count != prepared->program->buffer_count)
		return accel_metal_error(error, error_size, "Metal buffer table does not match the prepared program.");
	if (buffer_count != 0 && buffer == NULL)
		return accel_metal_error(error, error_size, "Missing Metal buffer descriptors.");

	/* Match scalar values followed by two range words per kernel. */
	expected_scalar_count = prepared->program->scalar_count;
	expected_scalar_count += (size_t)prepared->program->kernel_count * 2;
	if ((size_t)scalar_word_count != expected_scalar_count)
		return accel_metal_error(error, error_size, "Metal scalar block does not match the prepared program.");
	if (scalar_word_count != 0 && scalar_word == NULL)
		return accel_metal_error(error, error_size, "Missing Metal scalar words.");

	/* Match the optional result block and preserve the zero-count ABI. */
	if (result_word_count != prepared->program->scalar_result_count)
		return accel_metal_error(error, error_size, "Metal result block does not match the prepared program.");
	if (result_word_count == 0 && result_word != NULL)
		return accel_metal_error(error, error_size, "Unexpected Metal result words.");
	if (result_word_count != 0 && result_word == NULL)
		return accel_metal_error(error, error_size, "Missing Metal result words.");
	has_active_dispatch =
		accel_metal_count_active_dispatches(
			prepared->program,
			scalar_word) != 0;

	max_buffer_length = (size_t)[backend->device maxBufferLength];

	/* Validate every raw-word snapshot and only active allocations. */
	for (i = 0; i < buffer_count; i++) {
		binding = &prepared->program->buffer[i];

		/* Match immutable binding identity to the prepared program. */
		if (buffer[i].origin != binding->origin ||
		    buffer[i].args_slot != binding->args_slot ||
		    buffer[i].element_kind != binding->element_kind ||
		    buffer[i].element_width != binding->element_width) {
			return accel_metal_error(error, error_size, "Metal buffer metadata does not match the prepared program.");
		}

		/* Validate the runtime extent and dynamic transfer plan. */
		if (buffer[i].element_width != sizeof(uint32_t))
			return accel_metal_error(error, error_size, "Metal supports only 32-bit accelerator buffers.");
		if (buffer[i].element_count >
		    (size_t)-1 / buffer[i].element_width ||
		    buffer[i].element_count * buffer[i].element_width !=
		    buffer[i].byte_count) {
			return accel_metal_error(error, error_size, "Metal buffer extent metadata is inconsistent.");
		}
		if (!buffer[i].active &&
		    (buffer[i].upload || buffer[i].download)) {
			return accel_metal_error(error, error_size, "Inactive Metal buffers cannot request host transfers.");
		}
		if (buffer[i].active &&
		    buffer[i].byte_count > max_buffer_length) {
			return accel_metal_error(error, error_size, "Metal buffer exceeds the device allocation limit.");
		}
		if (buffer[i].upload &&
		    buffer[i].byte_count != 0 &&
		    buffer[i].snapshot == NULL) {
			return accel_metal_error(error, error_size, "Missing Metal upload snapshot.");
		}
		if (buffer[i].download &&
		    buffer[i].byte_count != 0 &&
		    buffer[i].snapshot == NULL) {
			return accel_metal_error(error, error_size, "Missing Metal download snapshot.");
		}
	}

	/* Validate the scalar byte extent before allocating its shared buffer. */
	if (expected_scalar_count > (size_t)-1 / sizeof(uint32_t))
		return accel_metal_error(error, error_size, "Metal scalar block size overflowed.");
	if (has_active_dispatch &&
	    expected_scalar_count * sizeof(uint32_t) > max_buffer_length) {
		return accel_metal_error(error, error_size, "Metal scalar block exceeds the device allocation limit.");
	}

	/* Validate the optional result extent against the same device limit. */
	result_byte_count = (size_t)result_word_count * sizeof(uint32_t);
	if (result_word_count != 0 &&
	    result_byte_count / sizeof(uint32_t) != result_word_count) {
		return accel_metal_error(error, error_size, "Metal result block size overflowed.");
	}
	if (has_active_dispatch && result_byte_count > max_buffer_length)
		return accel_metal_error(error, error_size, "Metal result block exceeds the device allocation limit.");

	/* Report a complete and representable execution snapshot. */
	return true;
}

/* Allocate and initialize one shared Metal buffer. */
static bool
accel_metal_create_buffer(
	struct accel_metal_backend *backend,
	size_t byte_count,
	const void *data,
	id<MTLBuffer> *result,
	char *error,
	size_t error_size)
{
	size_t allocation_size;
	NSUInteger metal_size;
	id<MTLBuffer> buffer;

	*result = nil;
	allocation_size = byte_count;

	/* Satisfy globally declared but unused shader bindings with one raw word. */
	if (allocation_size == 0)
		allocation_size = sizeof(uint32_t);

	/* Reject an extent the Objective-C API cannot represent exactly. */
	metal_size = (NSUInteger)allocation_size;
	if ((size_t)metal_size != allocation_size)
		return accel_metal_error(error, error_size, "Metal buffer size is not representable.");

	/* Allocate shared storage visible to both CPU and selected GPU. */
	buffer = [backend->device
		newBufferWithLength:metal_size
		options:MTLResourceStorageModeShared];
	if (buffer == nil)
		return accel_metal_error(error, error_size, "Failed to allocate shared Metal buffer memory.");

	/* Copy the exact runtime-owned upload snapshot into shared storage. */
	if (data != NULL && byte_count != 0)
		memcpy([buffer contents], data, byte_count);
	else
		memset([buffer contents], 0, allocation_size);

	/* Publish the complete retained buffer. */
	*result = buffer;

	/* Report a complete shared allocation. */
	return true;
}

/* Dispatch one kernel inside an autorelease pool. */
static bool
accel_metal_dispatch_execution(
	void *execution,
	uint32_t kernel_index,
	uint32_t start,
	uint32_t trip,
	char *error,
	size_t error_size)
{
	bool success;

	/* Bounds the temporary compute encoder to this callback. */
	@autoreleasepool {
		success = accel_metal_dispatch_execution_impl(
			execution,
			kernel_index,
			start,
			trip,
			error,
			error_size);
	}

	/* Report the complete encoding result. */
	return success;
}

/* Encode one active kernel in source order. */
static bool
accel_metal_dispatch_execution_impl(
	void *execution,
	uint32_t kernel_index,
	uint32_t start,
	uint32_t trip,
	char *error,
	size_t error_size)
{
	struct accel_metal_execution *active;
	const struct accel_program *program;
	id<MTLComputeCommandEncoder> encoder;
	const uint32_t *scalar_word;
	uint32_t range_word;
	NSUInteger group_count;
	NSUInteger i;
	MTLSize groups;
	MTLSize threads;

	active = execution;

	/* Validate the shared runtime's published execution and kernel index. */
	if (active == NULL || active->prepared == NULL)
		return accel_metal_error(error, error_size, "Invalid Metal execution.");
	if (!active->has_active_dispatch ||
	    active->command_buffer == nil ||
	    active->scalar_buffer == nil) {
		return accel_metal_error(error, error_size, "Metal execution has no active dispatch resources.");
	}
	if (kernel_index >= active->prepared->kernel_count)
		return accel_metal_error(error, error_size, "Invalid Metal kernel index.");
	if (trip == 0)
		return accel_metal_error(error, error_size, "Metal received an empty active dispatch.");
	if (active->submitted)
		return accel_metal_error(error, error_size, "Metal execution was already submitted.");
	if (active->dispatched && kernel_index <= active->last_kernel)
		return accel_metal_error(error, error_size, "Metal kernels were dispatched out of order.");
	if (active->dispatch_count >= active->expected_dispatch_count)
		return accel_metal_error(error, error_size, "Metal received too many active dispatches.");

	program = active->prepared->program;
	range_word = program->scalar_count + kernel_index * 2;

	/* Validate the duplicated callback range against immutable scalar words. */
	if (range_word + 1 >= active->scalar_word_count)
		return accel_metal_error(error, error_size, "Metal dispatch range is missing from the scalar block.");
	scalar_word = [active->scalar_buffer contents];
	if (scalar_word[range_word] != start || scalar_word[range_word + 1] != trip)
		return accel_metal_error(error, error_size, "Metal dispatch range changed after execution creation.");

	/* Open one encoder so Metal orders resources between kernel boundaries. */
	encoder = [active->command_buffer computeCommandEncoder];
	if (encoder == nil)
		return accel_metal_error(error, error_size, "Failed to create a Metal compute encoder.");

	/* Bind the immutable pipeline and every deterministic buffer argument. */
	[encoder setComputePipelineState:active->prepared->kernel[kernel_index].pipeline];
	for (i = 0; i < (NSUInteger)active->buffer_count; i++) {
		[encoder
			setBuffer:active->buffer[i].buffer
			offset:0
			atIndex:i];
	}
	[encoder
		setBuffer:active->scalar_buffer
		offset:0
		atIndex:(NSUInteger)active->buffer_count];
	if (active->result_word_count != 0) {
		[encoder
			setBuffer:active->result_buffer
			offset:0
			atIndex:(NSUInteger)active->buffer_count + 1];
	}

	/* Round the checked trip count to fixed 64-lane threadgroups. */
	group_count = (NSUInteger)(trip / ACCEL_METAL_WORKGROUP_SIZE);
	if (trip % ACCEL_METAL_WORKGROUP_SIZE != 0)
		group_count++;
	groups = MTLSizeMake(group_count, 1, 1);
	threads = MTLSizeMake((NSUInteger)ACCEL_METAL_WORKGROUP_SIZE, 1, 1);
	[encoder dispatchThreadgroups:groups threadsPerThreadgroup:threads];
	[encoder endEncoding];

	/* Remember increasing source order before the next callback. */
	active->last_kernel = kernel_index;
	active->dispatch_count++;
	active->dispatched = true;

	/* Report a complete ordered encoding. */
	return true;
}

/* Finish one execution inside an autorelease pool. */
static bool
accel_metal_finish_execution(
	void *execution,
	uint32_t result_word_count,
	uint32_t result_word[],
	uint32_t buffer_count,
	struct accel_runtime_buffer buffer[],
	char *error,
	size_t error_size)
{
	bool success;

	/* Bounds completion diagnostics to this synchronous callback. */
	@autoreleasepool {
		success = accel_metal_finish_execution_impl(
			execution,
			result_word_count,
			result_word,
			buffer_count,
			buffer,
			error,
			error_size);
	}

	/* Report synchronous completion and readback. */
	return success;
}

/* Submit, wait, and copy all requested shared-buffer downloads. */
static bool
accel_metal_finish_execution_impl(
	void *execution,
	uint32_t result_word_count,
	uint32_t result_word[],
	uint32_t buffer_count,
	struct accel_runtime_buffer buffer[],
	char *error,
	size_t error_size)
{
	struct accel_metal_execution *active;
	MTLCommandBufferStatus status;
	uint32_t i;

	active = execution;

	/* Validate all mutable output descriptors before submitting work. */
	if (!accel_metal_validate_finish(
		active,
		result_word_count,
		result_word,
		buffer_count,
		buffer,
		error,
		error_size)) {
		return false;
	}

	/* Complete an all-empty execution without touching the Metal device. */
	if (!active->has_active_dispatch) {
		active->completed = true;
		return true;
	}

	/* Submit and synchronously wait for every encoded active kernel. */
	active->submitted = true;
	[active->command_buffer commit];
	[active->command_buffer waitUntilCompleted];
	active->completed = true;
	status = [active->command_buffer status];
	if (status != MTLCommandBufferStatusCompleted)
		return accel_metal_error(error, error_size, "Metal command execution failed.");

	/* Copy completed shared storage into runtime-owned output snapshots. */
	for (i = 0; i < buffer_count; i++) {
		if (!buffer[i].download || buffer[i].byte_count == 0)
			continue;
		memcpy(
			buffer[i].snapshot,
			[active->buffer[i].buffer contents],
			buffer[i].byte_count);
	}

	/* Copies completed shared scalar-result words into runtime ownership. */
	if (result_word_count != 0) {
		memcpy(
			result_word,
			[active->result_buffer contents],
			(size_t)result_word_count * sizeof(*result_word));
	}

	/* Report a complete plain readback boundary. */
	return true;
}

/* Validate output snapshots against immutable execution metadata. */
static bool
accel_metal_validate_finish(
	const struct accel_metal_execution *execution,
	uint32_t result_word_count,
	const uint32_t result_word[],
	uint32_t buffer_count,
	const struct accel_runtime_buffer buffer[],
	char *error,
	size_t error_size)
{
	uint32_t i;

	/* Require the complete execution and exact buffer table. */
	if (execution == NULL || execution->backend == NULL)
		return accel_metal_error(error, error_size, "Invalid Metal finish request.");
	if (execution->completed || execution->submitted)
		return accel_metal_error(error, error_size, "Metal execution was already finished.");
	if (buffer_count != execution->buffer_count)
		return accel_metal_error(error, error_size, "Metal finish buffer table changed.");
	if (buffer_count != 0 && buffer == NULL)
		return accel_metal_error(error, error_size, "Missing Metal finish buffers.");
	if (result_word_count != execution->result_word_count)
		return accel_metal_error(error, error_size, "Metal finish result table changed.");
	if (result_word_count == 0 && result_word != NULL)
		return accel_metal_error(error, error_size, "Unexpected Metal finish result words.");
	if (result_word_count != 0 && result_word == NULL)
		return accel_metal_error(error, error_size, "Missing Metal finish result words.");
	if (execution->has_active_dispatch &&
	    execution->dispatch_count != execution->expected_dispatch_count) {
		return accel_metal_error(error, error_size, "Metal active dispatch sequence is incomplete.");
	}
	if (execution->has_active_dispatch &&
	    result_word_count != 0 &&
	    execution->result_buffer == nil) {
		return accel_metal_error(error, error_size, "Missing Metal result buffer.");
	}

	/* Match every output extent and snapshot to its retained buffer. */
	for (i = 0; i < buffer_count; i++) {
		if (buffer[i].origin != execution->buffer[i].origin)
			return accel_metal_error(error, error_size, "Metal output buffer origin changed.");
		if (buffer[i].args_slot != execution->buffer[i].args_slot)
			return accel_metal_error(error, error_size, "Metal output buffer slot changed.");
		if (buffer[i].element_kind != execution->buffer[i].element_kind)
			return accel_metal_error(error, error_size, "Metal output buffer element type changed.");
		if (buffer[i].element_width != execution->buffer[i].element_width)
			return accel_metal_error(error, error_size, "Metal output buffer element width changed.");
		if (buffer[i].element_count != execution->buffer[i].element_count)
			return accel_metal_error(error, error_size, "Metal output buffer extent changed.");
		if (buffer[i].byte_count != execution->buffer[i].byte_count)
			return accel_metal_error(error, error_size, "Metal output buffer size changed.");
		if (buffer[i].active != execution->buffer[i].active)
			return accel_metal_error(error, error_size, "Metal output buffer activity changed.");
		if (buffer[i].upload != execution->buffer[i].upload)
			return accel_metal_error(error, error_size, "Metal output upload plan changed.");
		if (buffer[i].download != execution->buffer[i].download)
			return accel_metal_error(error, error_size, "Metal output transfer plan changed.");
		if (buffer[i].download &&
		    buffer[i].byte_count != 0 &&
		    buffer[i].snapshot == NULL) {
			return accel_metal_error(error, error_size, "Missing Metal output snapshot.");
		}
	}

	/* Report a stable output publication boundary. */
	return true;
}

/* Destroy one drained or unsubmitted execution without backend locking. */
static void
accel_metal_destroy_execution(
	void *execution)
{
	struct accel_metal_execution *active;
	uint32_t i;

	active = execution;

	/* Accept cleanup of an absent execution. */
	if (active == NULL)
		return;

	/* Release command and buffer objects without submitting or waiting. */
	[active->command_buffer release];
	[active->result_buffer release];
	[active->scalar_buffer release];
	if (active->buffer != NULL) {
		for (i = 0; i < active->buffer_count; i++) {
			[active->buffer[i].buffer release];
			active->buffer[i].buffer = nil;
		}
	}
	active->command_buffer = nil;
	active->result_buffer = nil;
	active->scalar_buffer = nil;

	/* Release only backend-owned plain metadata. */
	noct_free(active->buffer);
	noct_free(active);
}
