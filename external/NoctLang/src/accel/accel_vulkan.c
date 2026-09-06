/* -*- coding: utf-8; tab-width: 8; indent-tabs-mode: t; -*- */

/*
 * Noct Programming Language
 * Copyright (c) 2025, 2026, Awe Morris
 */

/*
 * Vulkan 1.2 accelerator backend.
 */

#include "accel_vulkan.h"
#include "accel_context.h"
#include "accel_mutex.h"
#include "accel_runtime.h"
#include "accel_vulkan_shader.h"
#include "hir.h"
#include "runtime.h"

#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#define ACCEL_VULKAN_WORKGROUP_SIZE	64U
#define ACCEL_VULKAN_BACKEND_PRIORITY	400U

enum accel_vulkan_submission_state {
	ACCEL_VULKAN_SUBMISSION_NONE,
	ACCEL_VULKAN_SUBMISSION_IN_FLIGHT,
	ACCEL_VULKAN_SUBMISSION_DRAINED,
	ACCEL_VULKAN_SUBMISSION_ABANDONED
};

struct accel_vulkan_execution;
struct accel_vulkan_prepared;

struct accel_vulkan_backend {
	struct accel_vulkan_api api;
	VkInstance instance;
	VkPhysicalDevice physical_device;
	VkDevice device;
	VkQueue queue;
	uint32_t queue_family;
	VkPhysicalDeviceProperties properties;
	VkPhysicalDeviceFloatControlsProperties float_controls;
	VkPhysicalDeviceMemoryProperties memory_properties;
	struct accel_mutex queue_mutex;
	struct accel_vulkan_execution *abandoned_execution_head;
	struct accel_vulkan_prepared *deferred_prepared_head;
	bool poisoned;
	bool device_lost;
};

struct accel_vulkan_diagnostic {
	struct rt_env *env;
	char *message;
	size_t message_size;
};

struct accel_vulkan_kernel {
	VkShaderModule shader_module;
	VkDescriptorSetLayout descriptor_layout;
	VkPipelineLayout pipeline_layout;
	VkPipeline pipeline;
	bool uses_f32;
};

struct accel_vulkan_prepared {
	struct accel_vulkan_prepared *next_deferred;
	struct accel_program *program;
	struct accel_vulkan_kernel *kernel;
	uint32_t kernel_count;
};

struct accel_vulkan_buffer {
	VkBuffer buffer;
	VkDeviceMemory memory;
	void *mapped;
	VkDeviceSize logical_size;
	VkDeviceSize allocation_size;
	uint32_t memory_type;
	bool coherent;
	bool active;
	bool upload;
	bool download;
};

struct accel_vulkan_execution {
	struct accel_vulkan_execution *next_abandoned;
	struct accel_vulkan_backend *backend;
	const struct accel_vulkan_prepared *prepared;
	struct accel_vulkan_buffer *buffer;
	struct accel_vulkan_buffer scalar_buffer;
	struct accel_vulkan_buffer result_buffer;
	VkDescriptorPool descriptor_pool;
	VkDescriptorSet *descriptor_set;
	VkCommandPool command_pool;
	VkCommandBuffer command_buffer;
	VkFence fence;
	uint32_t buffer_count;
	uint32_t scalar_word_count;
	uint32_t result_word_count;
	uint32_t last_kernel;
	enum accel_vulkan_submission_state submission_state;
	bool command_started;
	bool has_commands;
	bool dispatched;
	bool finished;
	bool abandoned_linked;
};

static const struct accel_vulkan_api accel_vulkan_real_api = {
	vkGetInstanceProcAddr,
	vkCreateInstance,
	vkDestroyInstance,
	vkEnumeratePhysicalDevices,
	NULL,
	vkGetPhysicalDeviceQueueFamilyProperties,
	vkGetPhysicalDeviceMemoryProperties,
	vkCreateDevice,
	vkDestroyDevice,
	vkGetDeviceQueue,
	vkDeviceWaitIdle,
	vkCreateShaderModule,
	vkDestroyShaderModule,
	vkCreateDescriptorSetLayout,
	vkDestroyDescriptorSetLayout,
	vkCreatePipelineLayout,
	vkDestroyPipelineLayout,
	vkCreateComputePipelines,
	vkDestroyPipeline,
	vkCreateBuffer,
	vkDestroyBuffer,
	vkGetBufferMemoryRequirements,
	vkAllocateMemory,
	vkFreeMemory,
	vkBindBufferMemory,
	vkMapMemory,
	vkUnmapMemory,
	vkFlushMappedMemoryRanges,
	vkInvalidateMappedMemoryRanges,
	vkCreateDescriptorPool,
	vkDestroyDescriptorPool,
	vkAllocateDescriptorSets,
	vkUpdateDescriptorSets,
	vkCreateCommandPool,
	vkDestroyCommandPool,
	vkAllocateCommandBuffers,
	vkBeginCommandBuffer,
	vkEndCommandBuffer,
	vkCmdBindPipeline,
	vkCmdBindDescriptorSets,
	vkCmdCopyBuffer,
	vkCmdPipelineBarrier,
	vkCmdFillBuffer,
	vkCmdDispatch,
	vkCreateFence,
	vkDestroyFence,
	vkQueueSubmit,
	vkWaitForFences
};

static bool accel_vulkan_enumeration_api_valid(const struct accel_vulkan_api *api);
static bool accel_vulkan_api_valid(const struct accel_vulkan_api *api);
static void accel_vulkan_diagnostic_init(struct accel_vulkan_diagnostic *diagnostic, struct rt_env *env, char *message, size_t message_size);
static void accel_vulkan_diagnostic_error(struct accel_vulkan_diagnostic *diagnostic, const char *message);
static void accel_vulkan_diagnostic_out_of_memory(struct accel_vulkan_diagnostic *diagnostic);
static void accel_vulkan_initialization_error(struct rt_env *env, const char *message);
static bool accel_vulkan_loader_version(struct accel_vulkan_diagnostic *diagnostic, const struct accel_vulkan_api *api);
static bool accel_vulkan_create_instance(struct accel_vulkan_diagnostic *diagnostic, struct accel_vulkan_backend *backend);
static bool accel_vulkan_resolve_instance_api(struct accel_vulkan_diagnostic *diagnostic, struct accel_vulkan_backend *backend);
static bool accel_vulkan_get_physical_devices(struct accel_vulkan_diagnostic *diagnostic, struct accel_vulkan_backend *backend, VkPhysicalDevice **device, uint32_t *device_count);
static bool accel_vulkan_append_devices(struct accel_vulkan_diagnostic *diagnostic, struct accel_vulkan_backend *backend, struct accel_device_list *list);
static void accel_vulkan_rollback_devices(struct accel_device_list *list, uint32_t count);
static bool accel_vulkan_select_device(struct rt_env *env, struct accel_vulkan_backend *backend, const char *gpu_name);
static bool accel_vulkan_device_candidate(struct accel_vulkan_backend *backend, VkPhysicalDevice device, VkPhysicalDeviceProperties *properties, uint32_t *queue_family, uint32_t *score);
static bool accel_vulkan_find_queue_family(struct accel_vulkan_backend *backend, VkPhysicalDevice device, uint32_t *queue_family);
static uint32_t accel_vulkan_device_score(const VkPhysicalDeviceProperties *properties);
static bool accel_vulkan_create_device(struct rt_env *env, struct accel_vulkan_backend *backend);
static const struct accel_backend_ops *accel_vulkan_backend_ops(void);
static const struct accel_executor_ops *accel_vulkan_executor_ops(void);
static enum accel_compile_status accel_vulkan_prepare_program(void *backend_state, const struct accel_program *program, struct accel_prepared_program *result);
static bool accel_vulkan_program_uses_f32(const struct accel_program *program);
static bool accel_vulkan_prepare_kernel(struct accel_vulkan_backend *backend, shaderc_compiler_t compiler, shaderc_compile_options_t options, const struct accel_program *program, uint32_t kernel_index, struct accel_vulkan_kernel *result);
static void accel_vulkan_destroy_kernel(struct accel_vulkan_backend *backend, struct accel_vulkan_kernel *kernel);
static void accel_vulkan_destroy_prepared_program(void *backend_state, struct accel_prepared_program *program);
static bool accel_vulkan_register_runtime(struct accel_context *context, struct rt_env *env);
static void accel_vulkan_destroy_backend_state(void *backend_state);
static const struct accel_program *accel_vulkan_get_program(const struct accel_prepared_program *prepared);
static bool accel_vulkan_validate_dispatch_limit(void *backend_state, const struct accel_prepared_program *prepared, uint32_t kernel_index, uint32_t start, uint32_t trip, char *error, size_t error_size);
static bool accel_vulkan_create_execution(void *backend_state, const struct accel_prepared_program *prepared, uint32_t scalar_word_count, const uint32_t scalar_word[], uint32_t result_word_count, const uint32_t result_word[], uint32_t buffer_count, const struct accel_runtime_buffer buffer[], void **execution, char *error, size_t error_size);
static bool accel_vulkan_validate_execution_inputs(struct accel_vulkan_backend *backend, const struct accel_vulkan_prepared *prepared, uint32_t scalar_word_count, const uint32_t scalar_word[], uint32_t result_word_count, const uint32_t result_word[], uint32_t buffer_count, const struct accel_runtime_buffer buffer[], char *error, size_t error_size);
static bool accel_vulkan_create_execution_resources(struct accel_vulkan_execution *execution, uint32_t scalar_word_count, const uint32_t scalar_word[], uint32_t result_word_count, const uint32_t result_word[], uint32_t buffer_count, const struct accel_runtime_buffer buffer[], char *error, size_t error_size);
static bool accel_vulkan_create_host_buffer(struct accel_vulkan_backend *backend, VkDeviceSize logical_size, VkBufferUsageFlags usage, struct accel_vulkan_buffer *buffer);
static bool accel_vulkan_find_host_memory(const struct accel_vulkan_backend *backend, uint32_t memory_type_bits, uint32_t *memory_type, bool *coherent);
static bool accel_vulkan_round_allocation(VkDeviceSize value, VkDeviceSize atom, VkDeviceSize *result);
static void accel_vulkan_destroy_buffer(struct accel_vulkan_backend *backend, struct accel_vulkan_buffer *buffer);
static bool accel_vulkan_create_descriptors(struct accel_vulkan_execution *execution, char *error, size_t error_size);
static bool accel_vulkan_kernel_uses_buffer(const struct accel_ir_kernel *kernel, uint32_t buffer_index);
static bool accel_vulkan_record_begin(struct accel_vulkan_execution *execution, char *error, size_t error_size);
static bool accel_vulkan_flush_execution(struct accel_vulkan_execution *execution, char *error, size_t error_size);
static bool accel_vulkan_dispatch_execution(void *execution, uint32_t kernel_index, uint32_t start, uint32_t trip, char *error, size_t error_size);
static bool accel_vulkan_finish_execution(void *execution, uint32_t result_word_count, uint32_t result_word[], uint32_t buffer_count, struct accel_runtime_buffer buffer[], char *error, size_t error_size);
static bool accel_vulkan_validate_finish(const struct accel_vulkan_execution *execution, uint32_t result_word_count, const uint32_t result_word[], uint32_t buffer_count, const struct accel_runtime_buffer buffer[], char *error, size_t error_size);
static bool accel_vulkan_submit_and_wait(struct accel_vulkan_execution *execution, char *error, size_t error_size);
static bool accel_vulkan_invalidate_execution(struct accel_vulkan_execution *execution, char *error, size_t error_size);
static void accel_vulkan_copy_execution_results(struct accel_vulkan_execution *execution, uint32_t result_word_count, uint32_t result_word[], uint32_t buffer_count, struct accel_runtime_buffer buffer[]);
static void accel_vulkan_destroy_execution(void *execution);
static void accel_vulkan_destroy_execution_resources(struct accel_vulkan_execution *execution);
static void accel_vulkan_abandon_execution(struct accel_vulkan_execution *execution);
static void accel_vulkan_destroy_abandoned_executions(struct accel_vulkan_backend *backend);
static void accel_vulkan_destroy_prepared_payload(struct accel_vulkan_backend *backend, struct accel_vulkan_prepared *prepared);
static void accel_vulkan_destroy_deferred_prepared(struct accel_vulkan_backend *backend);
static void accel_vulkan_set_runtime_error(char *error, size_t error_size, const char *message);

/*
 * Enumerates suitable Vulkan compute devices without opening logical devices.
 */
bool
accel_vulkan_enumerate(
	struct accel_device_list *list,
	char *error,
	size_t error_size)
{
	bool success;

	/* Enumerate through the process Vulkan loader table. */
	success = accel_vulkan_enumerate_with_api(
		list,
		&accel_vulkan_real_api,
		error,
		error_size);

	/* Report the enumeration result. */
	return success;
}

/*
 * Enumerates suitable Vulkan devices through an injected function table.
 */
bool
accel_vulkan_enumerate_with_api(
	struct accel_device_list *list,
	const struct accel_vulkan_api *api,
	char *error,
	size_t error_size)
{
	struct accel_vulkan_backend backend;
	struct accel_vulkan_diagnostic diagnostic;
	bool success;

	accel_vulkan_diagnostic_init(
		&diagnostic,
		NULL,
		error,
		error_size);

	/* Reject an absent destination before opening the Vulkan loader. */
	if (list == NULL) {
		accel_vulkan_diagnostic_error(
			&diagnostic,
			N_TR("Invalid Vulkan device list."));
		return false;
	}

	/* Validate only the entry points required during enumeration. */
	if (!accel_vulkan_enumeration_api_valid(api)) {
		accel_vulkan_diagnostic_error(
			&diagnostic,
			N_TR("Incomplete Vulkan enumeration function table."));
		return false;
	}

	/* Initialize one temporary instance-only backend. */
	memset(&backend, 0, sizeof(backend));
	backend.api = *api;
	backend.api.get_physical_device_properties2 = NULL;

	/* Require the same Vulkan loader version used for execution. */
	if (!accel_vulkan_loader_version(&diagnostic, &backend.api))
		return false;

	/* Create the temporary enumeration instance. */
	if (!accel_vulkan_create_instance(&diagnostic, &backend))
		return false;

	/* Resolve physical-device properties before testing suitability. */
	if (!accel_vulkan_resolve_instance_api(&diagnostic, &backend)) {
		backend.api.destroy_instance(backend.instance, NULL);
		return false;
	}

	/* Append suitable devices without creating any logical device. */
	success = accel_vulkan_append_devices(
		&diagnostic,
		&backend,
		list);

	/* Close the temporary instance after enumeration completes. */
	backend.api.destroy_instance(backend.instance, NULL);

	/* Report the completed enumeration transaction. */
	return success;
}

/*
 * Creates the production Vulkan backend for one selected device record.
 *
 * The backend re-resolves the deep-owned name because enumeration-session
 * identity values are not retained after device listing.
 */
bool
accel_vulkan_create_selected(
	struct rt_env *env,
	const struct accel_device *device,
	const struct accel_backend_ops **ops,
	void **backend_state)
{
	bool success;

	/* Clear ownership outputs before validating the selected record. */
	if (ops != NULL)
		*ops = NULL;
	if (backend_state != NULL)
		*backend_state = NULL;

	/* Reject an invalid creation request without dereferencing it. */
	if (env == NULL ||
	    ops == NULL ||
	    backend_state == NULL) {
		return false;
	}

	/* Require a record produced by the Vulkan enumerator. */
	if (device == NULL ||
	    device->backend != ACCEL_BACKEND_VULKAN ||
	    device->name == NULL ||
	    device->name[0] == '\0') {
		rt_error(env, N_TR("Invalid selected Vulkan device."));
		return false;
	}

	/* Re-resolve the selected display name in a fresh Vulkan instance. */
	success = accel_vulkan_create(
		env,
		device->name,
		ops,
		backend_state);

	/* Report the backend creation result. */
	return success;
}

/*
 * Creates the production Vulkan backend with the process loader table.
 */
bool
accel_vulkan_create(
	struct rt_env *env,
	const char *gpu_name,
	const struct accel_backend_ops **ops,
	void **backend_state)
{
	bool success;

	/* Create through the process Vulkan loader table. */
	success = accel_vulkan_create_with_api(
		env,
		gpu_name,
		&accel_vulkan_real_api,
		ops,
		backend_state);

	/* Report the backend creation result. */
	return success;
}

/*
 * Creates a Vulkan backend through an injected function table.
 *
 * The function table is copied into the returned backend state.  Tests may
 * therefore release the input table after this call returns.
 */
bool
accel_vulkan_create_with_api(
	struct rt_env *env,
	const char *gpu_name,
	const struct accel_vulkan_api *api,
	const struct accel_backend_ops **ops,
	void **backend_state)
{
	struct accel_vulkan_backend *backend;
	struct accel_vulkan_diagnostic diagnostic;

	accel_vulkan_diagnostic_init(&diagnostic, env, NULL, 0);

	/* Clear ownership outputs before validating the creation request. */
	if (ops != NULL)
		*ops = NULL;
	if (backend_state != NULL)
		*backend_state = NULL;

	/* Reject an incomplete ownership or environment request. */
	if (env == NULL ||
	    ops == NULL ||
	    backend_state == NULL) {
		return false;
	}

	/* Require every entry point needed after backend publication. */
	if (!accel_vulkan_api_valid(api)) {
		accel_vulkan_diagnostic_error(
			&diagnostic,
			N_TR("Incomplete Vulkan function table."));
		return false;
	}

	/* Allocate the backend before opening Vulkan resources. */
	backend = noct_calloc(1, sizeof(*backend));
	if (backend == NULL) {
		accel_vulkan_diagnostic_out_of_memory(&diagnostic);
		return false;
	}

	/* Copy the injected table and force instance-level property resolution. */
	backend->api = *api;
	backend->api.get_physical_device_properties2 = NULL;

	/* Require the production Vulkan loader contract. */
	if (!accel_vulkan_loader_version(&diagnostic, &backend->api)) {
		accel_vulkan_destroy_backend_state(backend);
		return false;
	}

	/* Create the private execution instance. */
	if (!accel_vulkan_create_instance(&diagnostic, backend)) {
		accel_vulkan_destroy_backend_state(backend);
		return false;
	}

	/* Resolve physical-device properties from the created instance. */
	if (!accel_vulkan_resolve_instance_api(&diagnostic, backend)) {
		accel_vulkan_destroy_backend_state(backend);
		return false;
	}

	/* Select and inspect one suitable compute device. */
	if (!accel_vulkan_select_device(env, backend, gpu_name)) {
		accel_vulkan_destroy_backend_state(backend);
		return false;
	}

	/* Open the selected logical device and compute queue. */
	if (!accel_vulkan_create_device(env, backend)) {
		accel_vulkan_destroy_backend_state(backend);
		return false;
	}

	/* Initialize serialization for the published queue. */
	if (!accel_mutex_init(&backend->queue_mutex)) {
		accel_vulkan_initialization_error(
			env,
			N_TR("Failed to initialize the Vulkan queue mutex."));
		accel_vulkan_destroy_backend_state(backend);
		return false;
	}

	/* Transfer the complete backend to the caller. */
	*ops = accel_vulkan_backend_ops();
	*backend_state = backend;

	/* Report successful backend publication. */
	return true;
}

/* Return the immutable Vulkan backend operation table. */
static const struct accel_backend_ops *
accel_vulkan_backend_ops(void)
{
	static const struct accel_backend_ops ops = {
		accel_vulkan_prepare_program,
		accel_vulkan_destroy_prepared_program,
		accel_vulkan_register_runtime,
		accel_vulkan_destroy_backend_state
	};

	/* Return the process-lifetime backend operations. */
	return &ops;
}

/* Return the immutable Vulkan executor operation table. */
static const struct accel_executor_ops *
accel_vulkan_executor_ops(void)
{
	static const struct accel_executor_ops ops = {
		"Vulkan",
		accel_vulkan_get_program,
		accel_vulkan_validate_dispatch_limit,
		accel_vulkan_create_execution,
		accel_vulkan_dispatch_execution,
		accel_vulkan_finish_execution,
		accel_vulkan_destroy_execution
	};

	/* Return the process-lifetime executor operations. */
	return &ops;
}

/* Validate the bootstrap entry points needed for device enumeration. */
static bool
accel_vulkan_enumeration_api_valid(
	const struct accel_vulkan_api *api)
{
	/* Require every function used before logical-device creation. */
	if (api == NULL)
		return false;
	if (api->get_instance_proc_addr == NULL)
		return false;
	if (api->create_instance == NULL)
		return false;
	if (api->destroy_instance == NULL)
		return false;
	if (api->enumerate_physical_devices == NULL)
		return false;
	if (api->get_physical_device_queue_family_properties == NULL)
		return false;

	/* Report a complete enumeration table. */
	return true;
}

/* Validate every required bootstrap and runtime Vulkan entry point. */
static bool
accel_vulkan_api_valid(
	const struct accel_vulkan_api *api)
{
	/* Require the shared enumeration table first. */
	if (!accel_vulkan_enumeration_api_valid(api))
		return false;

	/* Require every function used by published runtime programs. */
	if (api->get_physical_device_memory_properties == NULL)
		return false;
	if (api->create_device == NULL)
		return false;
	if (api->destroy_device == NULL)
		return false;
	if (api->get_device_queue == NULL)
		return false;
	if (api->device_wait_idle == NULL)
		return false;
	if (api->create_shader_module == NULL)
		return false;
	if (api->destroy_shader_module == NULL)
		return false;
	if (api->create_descriptor_set_layout == NULL)
		return false;
	if (api->destroy_descriptor_set_layout == NULL)
		return false;
	if (api->create_pipeline_layout == NULL)
		return false;
	if (api->destroy_pipeline_layout == NULL)
		return false;
	if (api->create_compute_pipelines == NULL)
		return false;
	if (api->destroy_pipeline == NULL)
		return false;
	if (api->create_buffer == NULL)
		return false;
	if (api->destroy_buffer == NULL)
		return false;
	if (api->get_buffer_memory_requirements == NULL)
		return false;
	if (api->allocate_memory == NULL)
		return false;
	if (api->free_memory == NULL)
		return false;
	if (api->bind_buffer_memory == NULL)
		return false;
	if (api->map_memory == NULL)
		return false;
	if (api->unmap_memory == NULL)
		return false;
	if (api->flush_mapped_memory_ranges == NULL)
		return false;
	if (api->invalidate_mapped_memory_ranges == NULL)
		return false;
	if (api->create_descriptor_pool == NULL)
		return false;
	if (api->destroy_descriptor_pool == NULL)
		return false;
	if (api->allocate_descriptor_sets == NULL)
		return false;
	if (api->update_descriptor_sets == NULL)
		return false;
	if (api->create_command_pool == NULL)
		return false;
	if (api->destroy_command_pool == NULL)
		return false;
	if (api->allocate_command_buffers == NULL)
		return false;
	if (api->begin_command_buffer == NULL)
		return false;
	if (api->end_command_buffer == NULL)
		return false;
	if (api->cmd_bind_pipeline == NULL)
		return false;
	if (api->cmd_bind_descriptor_sets == NULL)
		return false;
	if (api->cmd_copy_buffer == NULL)
		return false;
	if (api->cmd_pipeline_barrier == NULL)
		return false;
	if (api->cmd_fill_buffer == NULL)
		return false;
	if (api->cmd_dispatch == NULL)
		return false;
	if (api->create_fence == NULL)
		return false;
	if (api->destroy_fence == NULL)
		return false;
	if (api->queue_submit == NULL)
		return false;
	if (api->wait_for_fences == NULL)
		return false;

	return true;
}

/* Initialize one runtime-backed or buffer-backed diagnostic sink. */
static void
accel_vulkan_diagnostic_init(
	struct accel_vulkan_diagnostic *diagnostic,
	struct rt_env *env,
	char *message,
	size_t message_size)
{
	diagnostic->env = env;
	diagnostic->message = message;
	diagnostic->message_size = message_size;

	/* Clear a caller-provided error buffer before the first operation. */
	if (message != NULL && message_size > 0)
		message[0] = '\0';
}

/* Publish one constant initialization diagnostic to the configured sink. */
static void
accel_vulkan_diagnostic_error(
	struct accel_vulkan_diagnostic *diagnostic,
	const char *message)
{
	size_t copy_size;

	/* Publish the runtime error when an environment owns the operation. */
	if (diagnostic->env != NULL)
		rt_error(diagnostic->env, N_TR("%s"), message);

	/* Ignore an absent optional text buffer. */
	if (diagnostic->message == NULL || diagnostic->message_size == 0)
		return;

	copy_size = strlen(message);

	/* Truncate while preserving one terminated diagnostic string. */
	if (copy_size >= diagnostic->message_size)
		copy_size = diagnostic->message_size - 1;

	memcpy(diagnostic->message, message, copy_size);
	diagnostic->message[copy_size] = '\0';
}

/* Publish an allocation failure through the configured diagnostic sink. */
static void
accel_vulkan_diagnostic_out_of_memory(
	struct accel_vulkan_diagnostic *diagnostic)
{
	/* Preserve the runtime's canonical allocation error behavior. */
	if (diagnostic->env != NULL)
		rt_out_of_memory(diagnostic->env);

	/* Publish a standalone message for no-VM enumeration. */
	if (diagnostic->message != NULL && diagnostic->message_size > 0) {
		accel_vulkan_diagnostic_error(
			diagnostic,
			N_TR("Out of memory while enumerating Vulkan devices."));
	}
}

/* Set one constant Vulkan initialization diagnostic. */
static void
accel_vulkan_initialization_error(
	struct rt_env *env,
	const char *message)
{
	rt_error(env, N_TR("%s"), message);
}

/* Require a working Vulkan 1.2 loader before creating an instance. */
static bool
accel_vulkan_loader_version(
	struct accel_vulkan_diagnostic *diagnostic,
	const struct accel_vulkan_api *api)
{
	PFN_vkEnumerateInstanceVersion enumerate_version;
	VkResult vk_result;
	uint32_t version;

	enumerate_version = (PFN_vkEnumerateInstanceVersion)
		api->get_instance_proc_addr(
			VK_NULL_HANDLE,
			"vkEnumerateInstanceVersion");
	if (enumerate_version == NULL) {
		accel_vulkan_diagnostic_error(
			diagnostic,
			N_TR("Vulkan 1.2 is required, but the loader only exposes Vulkan 1.0."));
		return false;
	}

	version = 0;
	vk_result = enumerate_version(&version);
	if (vk_result != VK_SUCCESS) {
		accel_vulkan_diagnostic_error(
			diagnostic,
			N_TR("Failed to query the Vulkan loader version."));
		return false;
	}
	if (version < VK_API_VERSION_1_2) {
		accel_vulkan_diagnostic_error(
			diagnostic,
			N_TR("Vulkan 1.2 or newer is required."));
		return false;
	}

	return true;
}

/* Create the private Vulkan 1.2 instance. */
static bool
accel_vulkan_create_instance(
	struct accel_vulkan_diagnostic *diagnostic,
	struct accel_vulkan_backend *backend)
{
	VkApplicationInfo application_info;
	VkInstanceCreateInfo create_info;
	VkResult result;

	memset(&application_info, 0, sizeof(application_info));
	application_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	application_info.pApplicationName = "Noct";
	application_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	application_info.pEngineName = "Noct Accel";
	application_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	application_info.apiVersion = VK_API_VERSION_1_2;

	memset(&create_info, 0, sizeof(create_info));
	create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	create_info.pApplicationInfo = &application_info;

	result = backend->api.create_instance(
		&create_info,
		NULL,
		&backend->instance);
	if (result != VK_SUCCESS) {
		accel_vulkan_diagnostic_error(
			diagnostic,
			N_TR("Failed to create a Vulkan 1.2 instance."));
		return false;
	}

	return true;
}

/* Resolve the required Vulkan 1.2 physical-device property entry point. */
static bool
accel_vulkan_resolve_instance_api(
	struct accel_vulkan_diagnostic *diagnostic,
	struct accel_vulkan_backend *backend)
{
	backend->api.get_physical_device_properties2 =
		(PFN_vkGetPhysicalDeviceProperties2)
		backend->api.get_instance_proc_addr(
			backend->instance,
			"vkGetPhysicalDeviceProperties2");
	if (backend->api.get_physical_device_properties2 == NULL) {
		accel_vulkan_diagnostic_error(
			diagnostic,
			N_TR("The Vulkan loader did not expose Vulkan 1.2 physical-device properties."));
		return false;
	}

	return true;
}

/* Enumerate physical-device handles owned by one live Vulkan instance. */
static bool
accel_vulkan_get_physical_devices(
	struct accel_vulkan_diagnostic *diagnostic,
	struct accel_vulkan_backend *backend,
	VkPhysicalDevice **device,
	uint32_t *device_count)
{
	VkPhysicalDevice *found_device;
	VkResult result;
	size_t byte_size;
	uint32_t count;

	*device = NULL;
	*device_count = 0;
	count = 0;

	/* Query the immutable number of physical devices. */
	result = backend->api.enumerate_physical_devices(
		backend->instance,
		&count,
		NULL);
	if (result != VK_SUCCESS) {
		accel_vulkan_diagnostic_error(
			diagnostic,
			N_TR("Failed to enumerate Vulkan physical devices."));
		return false;
	}

	/* Report a successful empty enumeration without allocating storage. */
	if (count == 0)
		return true;

	/* Check and allocate the physical-device handle table. */
	byte_size = (size_t)count * sizeof(*found_device);
	if (byte_size / sizeof(*found_device) != (size_t)count) {
		accel_vulkan_diagnostic_out_of_memory(diagnostic);
		return false;
	}

	found_device = noct_malloc(byte_size);
	if (found_device == NULL) {
		accel_vulkan_diagnostic_out_of_memory(diagnostic);
		return false;
	}

	/* Fill every handle without creating a logical device. */
	result = backend->api.enumerate_physical_devices(
		backend->instance,
		&count,
		found_device);
	if (result != VK_SUCCESS) {
		noct_free(found_device);
		accel_vulkan_diagnostic_error(
			diagnostic,
			N_TR("Failed to enumerate Vulkan physical devices."));
		return false;
	}

	*device = found_device;
	*device_count = count;

	/* Transfer the complete handle table to the caller. */
	return true;
}

/* Append every suitable Vulkan compute device to the shared registry. */
static bool
accel_vulkan_append_devices(
	struct accel_vulkan_diagnostic *diagnostic,
	struct accel_vulkan_backend *backend,
	struct accel_device_list *list)
{
	VkPhysicalDevice *device;
	VkPhysicalDeviceProperties properties;
	uint32_t original_count;
	uint32_t device_count;
	uint32_t queue_family;
	uint32_t score;
	uint32_t i;
	bool suitable;

	original_count = list->count;

	/* Acquire all physical handles before changing the shared registry. */
	if (!accel_vulkan_get_physical_devices(
		diagnostic,
		backend,
		&device,
		&device_count)) {
		return false;
	}

	/* Record every device that satisfies the execution requirements. */
	for (i = 0; i < device_count; i++) {
		suitable = accel_vulkan_device_candidate(
			backend,
			device[i],
			&properties,
			&queue_family,
			&score);
		if (!suitable)
			continue;

		if (!accel_device_list_append(
			list,
			ACCEL_BACKEND_VULKAN,
			properties.deviceName,
			ACCEL_VULKAN_BACKEND_PRIORITY,
			score,
			(uintptr_t)device[i])) {
			noct_free(device);
			accel_vulkan_rollback_devices(list, original_count);
			accel_vulkan_diagnostic_out_of_memory(diagnostic);
			return false;
		}
	}

	noct_free(device);

	/* Report a complete, possibly empty, backend enumeration. */
	return true;
}

/* Remove Vulkan records appended by one failed enumeration transaction. */
static void
accel_vulkan_rollback_devices(
	struct accel_device_list *list,
	uint32_t count)
{
	/* Release appended records in reverse enumeration order. */
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

/* Select one exact or highest-scoring suitable compute device. */
static bool
accel_vulkan_select_device(
	struct rt_env *env,
	struct accel_vulkan_backend *backend,
	const char *gpu_name)
{
	const char *requested_name;
	VkPhysicalDevice *device;
	VkPhysicalDevice selected;
	VkPhysicalDeviceProperties properties;
	VkPhysicalDeviceProperties2 property_query;
	struct accel_vulkan_diagnostic diagnostic;
	uint32_t device_count;
	uint32_t queue_family;
	uint32_t selected_queue_family;
	uint32_t score;
	uint32_t selected_score;
	uint32_t match_count;
	uint32_t i;
	bool suitable;

	accel_vulkan_diagnostic_init(&diagnostic, env, NULL, 0);

	/* Acquire all physical devices through the shared enumeration path. */
	if (!accel_vulkan_get_physical_devices(
		&diagnostic,
		backend,
		&device,
		&device_count)) {
		return false;
	}

	/* Require at least one physical device before suitability filtering. */
	if (device_count == 0) {
		accel_vulkan_initialization_error(
			env,
			N_TR("No Vulkan physical device is available."));
		return false;
	}

	/* Accept the canonical Vulkan selector as a direct-create convenience. */
	requested_name = gpu_name;
	if (requested_name != NULL &&
	    strncmp(requested_name, "vulkan:", 7) == 0) {
		requested_name += 7;
	}

	/* Initialize the stable best-candidate selection. */
	selected = VK_NULL_HANDLE;
	selected_score = 0;
	selected_queue_family = 0;
	match_count = 0;

	/* Evaluate every device without opening a logical device. */
	for (i = 0; i < device_count; i++) {
		suitable = accel_vulkan_device_candidate(
			backend,
			device[i],
			&properties,
			&queue_family,
			&score);
		if (!suitable)
			continue;
		if (requested_name != NULL &&
		    strcmp(properties.deviceName, requested_name) != 0) {
			continue;
		}
		if (requested_name != NULL)
			match_count++;

		if (selected == VK_NULL_HANDLE || score > selected_score) {
			selected = device[i];
			selected_score = score;
			selected_queue_family = queue_family;
		}
	}

	noct_free(device);

	/* Diagnose a requested name with no suitable exact match. */
	if (requested_name != NULL && match_count == 0) {
		rt_error(env, N_TR("Vulkan device '%s' was not found."), requested_name);
		return false;
	}

	/* Reject duplicate suitable devices with the same display name. */
	if (requested_name != NULL && match_count > 1) {
		rt_error(env, N_TR("Vulkan device name '%s' is ambiguous."), requested_name);
		return false;
	}

	/* Diagnose a platform with no suitable compute device. */
	if (selected == VK_NULL_HANDLE) {
		accel_vulkan_initialization_error(
			env,
			N_TR("No suitable Vulkan 1.2 compute device is available."));
		return false;
	}

	backend->physical_device = selected;
	backend->queue_family = selected_queue_family;
	memset(&backend->float_controls, 0, sizeof(backend->float_controls));
	backend->float_controls.sType =
		VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FLOAT_CONTROLS_PROPERTIES;
	memset(&backend->properties, 0, sizeof(backend->properties));
	memset(&property_query, 0, sizeof(property_query));
	property_query.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
	property_query.pNext = &backend->float_controls;
	backend->api.get_physical_device_properties2(
		backend->physical_device,
		&property_query);
	backend->properties = property_query.properties;
	backend->api.get_physical_device_memory_properties(
		backend->physical_device,
		&backend->memory_properties);

	return true;
}

/* Check one physical device against immutable initialization requirements. */
static bool
accel_vulkan_device_candidate(
	struct accel_vulkan_backend *backend,
	VkPhysicalDevice device,
	VkPhysicalDeviceProperties *properties,
	uint32_t *queue_family,
	uint32_t *score)
{
	VkPhysicalDeviceProperties2 property_query;
	VkPhysicalDeviceFloatControlsProperties float_controls;

	memset(&float_controls, 0, sizeof(float_controls));
	float_controls.sType =
		VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FLOAT_CONTROLS_PROPERTIES;
	memset(&property_query, 0, sizeof(property_query));
	property_query.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
	property_query.pNext = &float_controls;
	backend->api.get_physical_device_properties2(device, &property_query);
	*properties = property_query.properties;

	/* Require the same Vulkan version and workgroup used by execution. */
	if (properties->apiVersion < VK_API_VERSION_1_2)
		return false;
	if (properties->limits.maxComputeWorkGroupInvocations <
	    ACCEL_VULKAN_WORKGROUP_SIZE) {
		return false;
	}
	if (properties->limits.maxComputeWorkGroupSize[0] <
	    ACCEL_VULKAN_WORKGROUP_SIZE) {
		return false;
	}
	if (!accel_vulkan_find_queue_family(
		backend,
		device,
		queue_family)) {
		return false;
	}

	*score = accel_vulkan_device_score(properties);

	return true;
}

/* Find the first compute-capable queue family for one physical device. */
static bool
accel_vulkan_find_queue_family(
	struct accel_vulkan_backend *backend,
	VkPhysicalDevice device,
	uint32_t *queue_family)
{
	VkQueueFamilyProperties *properties;
	uint32_t count;
	uint32_t i;
	bool found;

	count = 0;
	backend->api.get_physical_device_queue_family_properties(
		device,
		&count,
		NULL);
	if (count == 0)
		return false;

	properties = noct_malloc(sizeof(*properties) * count);
	if (properties == NULL)
		return false;

	backend->api.get_physical_device_queue_family_properties(
		device,
		&count,
		properties);
	found = false;

	/* Select the first family that can execute compute commands. */
	for (i = 0; i < count; i++) {
		if ((properties[i].queueFlags & VK_QUEUE_COMPUTE_BIT) == 0)
			continue;

		*queue_family = i;
		found = true;
		break;
	}

	noct_free(properties);

	return found;
}

/* Give deterministic preference to discrete and integrated GPUs. */
static uint32_t
accel_vulkan_device_score(
	const VkPhysicalDeviceProperties *properties)
{
	/* Rank broad Vulkan device classes before stable enumeration order. */
	switch (properties->deviceType) {
	case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
		return 500;
	case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
		return 400;
	case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:
		return 300;
	case VK_PHYSICAL_DEVICE_TYPE_CPU:
		return 200;
	default:
		return 100;
	}
}

/* Create the selected device and borrow its one compute queue. */
static bool
accel_vulkan_create_device(
	struct rt_env *env,
	struct accel_vulkan_backend *backend)
{
	VkDeviceQueueCreateInfo queue_info;
	VkDeviceCreateInfo create_info;
	VkResult result;
	float priority;

	priority = 1.0f;
	memset(&queue_info, 0, sizeof(queue_info));
	queue_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	queue_info.queueFamilyIndex = backend->queue_family;
	queue_info.queueCount = 1;
	queue_info.pQueuePriorities = &priority;

	memset(&create_info, 0, sizeof(create_info));
	create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	create_info.queueCreateInfoCount = 1;
	create_info.pQueueCreateInfos = &queue_info;

	result = backend->api.create_device(
		backend->physical_device,
		&create_info,
		NULL,
		&backend->device);
	if (result != VK_SUCCESS) {
		accel_vulkan_initialization_error(
			env,
			N_TR("Failed to create the selected Vulkan compute device."));
		return false;
	}

	backend->api.get_device_queue(
		backend->device,
		backend->queue_family,
		0,
		&backend->queue);
	if (backend->queue == VK_NULL_HANDLE) {
		accel_vulkan_initialization_error(
			env,
			N_TR("The selected Vulkan compute queue is unavailable."));
		return false;
	}

	return true;
}

/* Prepare every immutable shader pipeline for one accelerator program. */
static enum accel_compile_status
accel_vulkan_prepare_program(
	void *backend_state,
	const struct accel_program *program,
	struct accel_prepared_program *result)
{
	struct accel_vulkan_backend *backend;
	struct accel_vulkan_prepared *prepared;
	shaderc_compiler_t compiler;
	shaderc_compile_options_t options;
	char validation_error[128];
	uint32_t descriptor_count;
	uint32_t i;
	bool unavailable;

	/* Reject an absent opaque publication output. */
	if (result == NULL)
		return ACCEL_COMPILE_ERROR;

	/* Clear publication and recover the selected backend. */
	result->payload = NULL;
	backend = backend_state;

	/* Reject incomplete preparation inputs before touching the backend. */
	if (backend == NULL || program == NULL) {
		hir_error(0, N_TR("Invalid Vulkan program preparation request."));
		return ACCEL_COMPILE_ERROR;
	}

	/* Reject compilation after a device or synchronization failure. */
	accel_mutex_lock(&backend->queue_mutex);
	unavailable = backend->poisoned || backend->device_lost;
	accel_mutex_unlock(&backend->queue_mutex);
	if (unavailable) {
		hir_error(
			program->source_line,
			N_TR("The Vulkan accelerator device is unavailable."));
		return ACCEL_COMPILE_ERROR;
	}

	/* Validate the complete target-neutral program before device limits. */
	if (!accel_program_validate(
		program,
		validation_error,
		sizeof(validation_error))) {
		hir_error(
			program->source_line,
			N_TR("Invalid accelerator program reached the Vulkan backend."));
		return ACCEL_COMPILE_ERROR;
	}

	/* Count data, scalar, and optional result descriptor bindings. */
	descriptor_count = program->buffer_count + 1;

	/* Include binding B plus one only for a declared result block. */
	if (program->scalar_result_count != 0)
		descriptor_count++;

	/* Decline programs exceeding the per-stage storage descriptor limit. */
	if (descriptor_count >
	    backend->properties.limits.maxPerStageDescriptorStorageBuffers) {
		return ACCEL_COMPILE_DECLINED;
	}

	/* Decline programs exceeding the complete set storage limit. */
	if (descriptor_count >
	    backend->properties.limits.maxDescriptorSetStorageBuffers) {
		return ACCEL_COMPILE_DECLINED;
	}

	/* Require enough invocations for one fixed-size workgroup. */
	if (ACCEL_VULKAN_WORKGROUP_SIZE >
	    backend->properties.limits.maxComputeWorkGroupInvocations) {
		return ACCEL_COMPILE_DECLINED;
	}

	/* Require the fixed workgroup width in the X dimension. */
	if (ACCEL_VULKAN_WORKGROUP_SIZE >
	    backend->properties.limits.maxComputeWorkGroupSize[0]) {
		return ACCEL_COMPILE_DECLINED;
	}

	/* Require every strict Float32 control used by generated SPIR-V. */
	if (accel_vulkan_program_uses_f32(program)) {
		/* Preserve signed zero, infinities, and NaNs exactly. */
		if (!backend->float_controls.shaderSignedZeroInfNanPreserveFloat32)
			return ACCEL_COMPILE_DECLINED;

		/* Preserve Float32 denormal operands and results exactly. */
		if (!backend->float_controls.shaderDenormPreserveFloat32)
			return ACCEL_COMPILE_DECLINED;

		/* Require round-to-nearest-even Float32 arithmetic. */
		if (!backend->float_controls.shaderRoundingModeRTEFloat32)
			return ACCEL_COMPILE_DECLINED;
	}

	/* Allocate the opaque prepared-program owner. */
	prepared = noct_calloc(1, sizeof(*prepared));
	if (prepared == NULL) {
		hir_out_of_memory();
		return ACCEL_COMPILE_ERROR;
	}

	/* Allocate one immutable pipeline record per source kernel. */
	prepared->kernel = noct_calloc(
		program->kernel_count,
		sizeof(*prepared->kernel));
	if (prepared->kernel == NULL) {
		noct_free(prepared);
		hir_out_of_memory();
		return ACCEL_COMPILE_ERROR;
	}

	/* Publish the complete kernel-table extent inside the private owner. */
	prepared->kernel_count = program->kernel_count;

	/* Deep-copy target-neutral metadata retained by runtime executions. */
	prepared->program = accel_program_clone(program);
	if (prepared->program == NULL) {
		noct_free(prepared->kernel);
		noct_free(prepared);
		hir_out_of_memory();
		return ACCEL_COMPILE_ERROR;
	}

	/* Open one shader assembler for this complete program transaction. */
	compiler = shaderc_compiler_initialize();
	if (compiler == NULL) {
		result->payload = prepared;
		accel_vulkan_destroy_prepared_program(backend, result);
		hir_out_of_memory();
		return ACCEL_COMPILE_ERROR;
	}

	/* Allocate the matching deterministic assembler options. */
	options = shaderc_compile_options_initialize();
	if (options == NULL) {
		shaderc_compiler_release(compiler);
		result->payload = prepared;
		accel_vulkan_destroy_prepared_program(backend, result);
		hir_out_of_memory();
		return ACCEL_COMPILE_ERROR;
	}

	/* Select Vulkan 1.2, SPIR-V 1.5, and deterministic unoptimized output. */
	shaderc_compile_options_set_target_env(
		options,
		shaderc_target_env_vulkan,
		shaderc_env_version_vulkan_1_2);
	shaderc_compile_options_set_target_spirv(
		options,
		shaderc_spirv_version_1_5);
	shaderc_compile_options_set_optimization_level(
		options,
		shaderc_optimization_level_zero);

	/* Create every kernel pipeline before publishing the prepared payload. */
	for (i = 0; i < prepared->kernel_count; i++) {
		/* Prepare this kernel before advancing the publication frontier. */
		if (!accel_vulkan_prepare_kernel(
			backend,
			compiler,
			options,
			program,
			i,
			&prepared->kernel[i])) {
			/* Release temporary compiler state and the partial payload. */
			shaderc_compile_options_release(options);
			shaderc_compiler_release(compiler);
			result->payload = prepared;
			accel_vulkan_destroy_prepared_program(backend, result);
			return ACCEL_COMPILE_ERROR;
		}
	}

	/* Release temporary compiler state and publish the prepared owner. */
	shaderc_compile_options_release(options);
	shaderc_compiler_release(compiler);
	result->payload = prepared;

	/* Report a complete accelerator program preparation. */
	return ACCEL_COMPILE_APPLIED;
}

/* Detect whether any kernel requires strict Float32 device controls. */
static bool
accel_vulkan_program_uses_f32(
	const struct accel_program *program)
{
	const struct accel_ir_kernel *kernel;
	uint32_t i;
	uint32_t j;

	/* Inspect every buffer and instruction type in every kernel. */
	for (i = 0; i < program->kernel_count; i++) {
		kernel = program->kernel[i].ir;

		/* Inspect every declared buffer element type. */
		for (j = 0; j < kernel->buffer_binding_count; j++) {
			/* Report the first Float32 storage binding. */
			if (kernel->buffer_value_type[j] == ACCEL_IR_F32)
				return true;
		}

		/* Inspect every typed SSA result. */
		for (j = 0; j < kernel->instruction_count; j++) {
			/* Report the first Float32 value. */
			if (kernel->instruction[j].result_type == ACCEL_IR_F32)
				return true;
		}
	}

	/* Report an integer-only program. */
	return false;
}

/* Compile and create one immutable Vulkan compute pipeline. */
static bool
accel_vulkan_prepare_kernel(
	struct accel_vulkan_backend *backend,
	shaderc_compiler_t compiler,
	shaderc_compile_options_t options,
	const struct accel_program *program,
	uint32_t kernel_index,
	struct accel_vulkan_kernel *result)
{
	struct accel_vulkan_spirv spirv;
	VkShaderModuleCreateInfo shader_info;
	VkDescriptorSetLayoutCreateInfo descriptor_info;
	VkDescriptorSetLayoutBinding *binding;
	VkPipelineLayoutCreateInfo layout_info;
	VkPipelineShaderStageCreateInfo stage_info;
	VkComputePipelineCreateInfo pipeline_info;
	VkResult vk_result;
	uint32_t descriptor_count;
	uint32_t i;
	enum accel_compile_status status;

	/* Clear all temporary assembly and output ownership. */
	memset(&spirv, 0, sizeof(spirv));
	memset(result, 0, sizeof(*result));

	/* Assemble and validate this kernel's deterministic SPIR-V. */
	status = accel_vulkan_shader_compile(
		compiler,
		options,
		program,
		kernel_index,
		&spirv);
	if (status != ACCEL_COMPILE_APPLIED)
		return false;

	/* Create the immutable shader module and release temporary words. */
	memset(&shader_info, 0, sizeof(shader_info));
	shader_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	shader_info.codeSize = spirv.word_count * sizeof(uint32_t);
	shader_info.pCode = spirv.word;
	vk_result = backend->api.create_shader_module(
		backend->device,
		&shader_info,
		NULL,
		&result->shader_module);
	accel_vulkan_shader_cleanup(&spirv);
	if (vk_result != VK_SUCCESS) {
		hir_error(
			program->kernel[kernel_index].source_line,
			N_TR("Failed to create a Vulkan shader module."));
		return false;
	}

	/* Count data, scalar, and optional result descriptor bindings. */
	descriptor_count = program->buffer_count + 1;

	/* Include binding B plus one only for a declared result block. */
	if (program->scalar_result_count != 0)
		descriptor_count++;

	/* Allocate the complete descriptor-layout binding table. */
	binding = noct_calloc(descriptor_count, sizeof(*binding));
	if (binding == NULL) {
		hir_out_of_memory();
		accel_vulkan_destroy_kernel(backend, result);
		return false;
	}

	/* Describe data, scalar, and optional scalar-result storage buffers. */
	for (i = 0; i < descriptor_count; i++) {
		binding[i].binding = i;
		binding[i].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		binding[i].descriptorCount = 1;
		binding[i].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
	}

	/* Create the immutable descriptor-set layout from the dense table. */
	memset(&descriptor_info, 0, sizeof(descriptor_info));
	descriptor_info.sType =
		VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	descriptor_info.bindingCount = descriptor_count;
	descriptor_info.pBindings = binding;
	vk_result = backend->api.create_descriptor_set_layout(
		backend->device,
		&descriptor_info,
		NULL,
		&result->descriptor_layout);
	noct_free(binding);
	if (vk_result != VK_SUCCESS) {
		hir_error(
			program->kernel[kernel_index].source_line,
			N_TR("Failed to create a Vulkan descriptor layout."));
		accel_vulkan_destroy_kernel(backend, result);
		return false;
	}

	/* Create the one-set pipeline layout used by this kernel. */
	memset(&layout_info, 0, sizeof(layout_info));
	layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	layout_info.setLayoutCount = 1;
	layout_info.pSetLayouts = &result->descriptor_layout;
	vk_result = backend->api.create_pipeline_layout(
		backend->device,
		&layout_info,
		NULL,
		&result->pipeline_layout);
	if (vk_result != VK_SUCCESS) {
		hir_error(
			program->kernel[kernel_index].source_line,
			N_TR("Failed to create a Vulkan pipeline layout."));
		accel_vulkan_destroy_kernel(backend, result);
		return false;
	}

	/* Describe the validated compute entry point. */
	memset(&stage_info, 0, sizeof(stage_info));
	stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	stage_info.stage = VK_SHADER_STAGE_COMPUTE_BIT;
	stage_info.module = result->shader_module;
	stage_info.pName = "main";

	/* Create the immutable compute pipeline without a pipeline cache. */
	memset(&pipeline_info, 0, sizeof(pipeline_info));
	pipeline_info.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
	pipeline_info.stage = stage_info;
	pipeline_info.layout = result->pipeline_layout;
	pipeline_info.basePipelineHandle = VK_NULL_HANDLE;
	pipeline_info.basePipelineIndex = -1;
	vk_result = backend->api.create_compute_pipelines(
		backend->device,
		VK_NULL_HANDLE,
		1,
		&pipeline_info,
		NULL,
		&result->pipeline);
	if (vk_result != VK_SUCCESS) {
		hir_error(
			program->kernel[kernel_index].source_line,
			N_TR("Failed to create a Vulkan compute pipeline."));
		accel_vulkan_destroy_kernel(backend, result);
		return false;
	}

	/* Record whether the prepared program requires strict Float32 controls. */
	result->uses_f32 = accel_vulkan_program_uses_f32(program);

	/* Report one complete immutable kernel pipeline. */
	return true;
}

/* Destroy one partially or fully prepared kernel in reverse order. */
static void
accel_vulkan_destroy_kernel(
	struct accel_vulkan_backend *backend,
	struct accel_vulkan_kernel *kernel)
{
	/* Release the compute pipeline before its pipeline layout. */
	if (kernel->pipeline != VK_NULL_HANDLE) {
		backend->api.destroy_pipeline(
			backend->device,
			kernel->pipeline,
			NULL);
	}

	/* Release the pipeline layout before its descriptor-set layout. */
	if (kernel->pipeline_layout != VK_NULL_HANDLE) {
		backend->api.destroy_pipeline_layout(
			backend->device,
			kernel->pipeline_layout,
			NULL);
	}

	/* Release the descriptor-set layout before shader code. */
	if (kernel->descriptor_layout != VK_NULL_HANDLE) {
		backend->api.destroy_descriptor_set_layout(
			backend->device,
			kernel->descriptor_layout,
			NULL);
	}

	/* Release the immutable shader module last. */
	if (kernel->shader_module != VK_NULL_HANDLE) {
		backend->api.destroy_shader_module(
			backend->device,
			kernel->shader_module,
			NULL);
	}

	/* Clear all stale handles in the reusable partial record. */
	memset(kernel, 0, sizeof(*kernel));
}

/* Destroy one prepared program or defer it behind an unproved submission. */
static void
accel_vulkan_destroy_prepared_program(
	void *backend_state,
	struct accel_prepared_program *program)
{
	struct accel_vulkan_backend *backend;
	struct accel_vulkan_prepared *prepared;
	bool deferred;

	/* Accept cleanup of an empty publication slot. */
	if (program == NULL || program->payload == NULL)
		return;

	/* Detach the opaque payload before choosing its cleanup lifetime. */
	backend = backend_state;
	prepared = program->payload;
	program->payload = NULL;

	/* A published prepared program always belongs to a live backend. */
	if (backend == NULL)
		abort();

	/* Preserve pipelines while an unproved command may still reference them. */
	deferred = false;
	accel_mutex_lock(&backend->queue_mutex);
	if (backend->poisoned) {
		prepared->next_deferred = backend->deferred_prepared_head;
		backend->deferred_prepared_head = prepared;
		deferred = true;
	}
	accel_mutex_unlock(&backend->queue_mutex);

	/* Destroy ordinary prepared ownership immediately after all sessions drain. */
	if (!deferred)
		accel_vulkan_destroy_prepared_payload(backend, prepared);
}

/* Register the shared private accelerator runtime with Vulkan callbacks. */
static bool
accel_vulkan_register_runtime(
	struct accel_context *context,
	struct rt_env *env)
{
	bool success;

	/* Copy the target-specific executor table into VM-owned metadata. */
	success = accel_runtime_register(
		context,
		env,
		accel_vulkan_executor_ops());

	/* Report the shared package publication result. */
	return success;
}

/* Destroy the selected device after every context operation has drained. */
static void
accel_vulkan_destroy_backend_state(
	void *backend_state)
{
	struct accel_vulkan_backend *backend;
	struct accel_vulkan_execution *execution;
	VkResult result;

	/* Recover and accept the optional backend owner. */
	backend = backend_state;
	if (backend == NULL)
		return;

	/* Prove all submitted work idle before releasing deferred objects. */
	if (backend->device != VK_NULL_HANDLE &&
	    backend->queue_mutex.initialized) {
		accel_mutex_lock(&backend->queue_mutex);
		result = backend->api.device_wait_idle(backend->device);
		if (result != VK_SUCCESS && result != VK_ERROR_DEVICE_LOST) {
			/*
			 * Vulkan does not permit destroying objects that may still
			 * be in use.  Retain the complete backend when the final
			 * device-wide drain cannot prove their lifetime complete.
			 */
			accel_mutex_unlock(&backend->queue_mutex);
			return;
		}

		/* Record terminal device loss after it establishes cleanup safety. */
		if (result == VK_ERROR_DEVICE_LOST)
			backend->device_lost = true;
		backend->poisoned = false;
		execution = backend->abandoned_execution_head;

		/* Mark every retained execution safe for ordinary cleanup. */
		while (execution != NULL) {
			execution->submission_state =
				ACCEL_VULKAN_SUBMISSION_DRAINED;
			execution = execution->next_abandoned;
		}
		accel_mutex_unlock(&backend->queue_mutex);

		/* Release execution resources before their deferred pipelines. */
		accel_vulkan_destroy_abandoned_executions(backend);
		accel_vulkan_destroy_deferred_prepared(backend);
	}

	/* Close device-wide synchronization and the logical device. */
	if (backend->device != VK_NULL_HANDLE) {
		accel_mutex_destroy(&backend->queue_mutex);
		backend->api.destroy_device(backend->device, NULL);
		backend->device = VK_NULL_HANDLE;
	}

	/* Close the loader instance after every child object is gone. */
	if (backend->instance != VK_NULL_HANDLE) {
		backend->api.destroy_instance(backend->instance, NULL);
		backend->instance = VK_NULL_HANDLE;
	}

	/* Release the empty backend wrapper after every Vulkan owner is gone. */
	noct_free(backend);
}

/* Borrow target-neutral metadata from one prepared Vulkan payload. */
static const struct accel_program *
accel_vulkan_get_program(
	const struct accel_prepared_program *prepared)
{
	const struct accel_vulkan_prepared *payload;

	/* Reject an absent opaque publication slot. */
	if (prepared == NULL || prepared->payload == NULL)
		return NULL;

	/* Recover the immutable payload from its opaque publication slot. */
	payload = prepared->payload;

	/* Return the immutable deep-owned program plan. */
	return payload->program;
}

/* Validate one checked dispatch against immutable Vulkan limits. */
static bool
accel_vulkan_validate_dispatch_limit(
	void *backend_state,
	const struct accel_prepared_program *prepared,
	uint32_t kernel_index,
	uint32_t start,
	uint32_t trip,
	char *error,
	size_t error_size)
{
	struct accel_vulkan_backend *backend;
	const struct accel_vulkan_prepared *payload;
	uint64_t group_count;
	bool unavailable;

	UNUSED_PARAMETER(start);

	/* Recover and validate backend and prepared-program ownership. */
	backend = backend_state;
	if (backend == NULL ||
	    prepared == NULL ||
	    prepared->payload == NULL) {
		accel_vulkan_set_runtime_error(
			error,
			error_size,
			N_TR("Invalid Vulkan dispatch limit request."));
		return false;
	}

	/* Recover the immutable payload and reject a stale kernel index. */
	payload = prepared->payload;
	if (kernel_index >= payload->kernel_count) {
		accel_vulkan_set_runtime_error(
			error,
			error_size,
			N_TR("Invalid Vulkan kernel index."));
		return false;
	}

	/* Reject new work after a device or synchronization failure. */
	accel_mutex_lock(&backend->queue_mutex);
	unavailable = backend->poisoned || backend->device_lost;
	accel_mutex_unlock(&backend->queue_mutex);
	if (unavailable) {
		accel_vulkan_set_runtime_error(
			error,
			error_size,
			N_TR("The Vulkan accelerator device is unavailable."));
		return false;
	}

	/* Empty source loops require no device dispatch. */
	if (trip == 0)
		return true;

	/* Round the source iteration count to fixed 64-lane workgroups. */
	group_count = ((uint64_t)trip + ACCEL_VULKAN_WORKGROUP_SIZE - 1) /
		ACCEL_VULKAN_WORKGROUP_SIZE;
	if (group_count >
	    backend->properties.limits.maxComputeWorkGroupCount[0]) {
		accel_vulkan_set_runtime_error(
			error,
			error_size,
			N_TR("Vulkan dispatch exceeds the device workgroup-count limit."));
		return false;
	}

	/* Report a representable one-dimensional dispatch. */
	return true;
}

/* Create all Vulkan resources from runtime-owned plain snapshots. */
static bool
accel_vulkan_create_execution(
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
	struct accel_vulkan_backend *backend;
	const struct accel_vulkan_prepared *payload;
	struct accel_vulkan_execution *created;

	/* Reject an absent ownership output before writing through it. */
	if (execution == NULL) {
		accel_vulkan_set_runtime_error(
			error,
			error_size,
			N_TR("Missing Vulkan execution output."));
		return false;
	}

	/* Clear the opaque output before validating runtime snapshots. */
	*execution = NULL;

	/* Recover backend state and immutable published program metadata. */
	backend = backend_state;
	if (prepared == NULL || prepared->payload == NULL) {
		accel_vulkan_set_runtime_error(
			error,
			error_size,
			N_TR("Missing Vulkan prepared program."));
		return false;
	}
	payload = prepared->payload;

	/* Validate every borrowed plain-C input before allocating Vulkan objects. */
	if (!accel_vulkan_validate_execution_inputs(
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

	/* Allocate the backend execution without retaining a VM or context. */
	created = noct_calloc(1, sizeof(*created));
	if (created == NULL) {
		accel_vulkan_set_runtime_error(
			error,
			error_size,
			N_TR("Out of memory while creating a Vulkan execution."));
		return false;
	}
	created->backend = backend;
	created->prepared = payload;
	created->buffer_count = buffer_count;
	created->scalar_word_count = scalar_word_count;
	created->result_word_count = result_word_count;
	created->submission_state = ACCEL_VULKAN_SUBMISSION_NONE;

	/* Create mapped buffers, descriptors, and a recording command buffer. */
	if (!accel_vulkan_create_execution_resources(
		created,
		scalar_word_count,
		scalar_word,
		result_word_count,
		result_word,
		buffer_count,
		buffer,
		error,
		error_size)) {
		accel_vulkan_destroy_execution(created);
		return false;
	}

	/* Publish the complete plain backend execution to the shared runtime. */
	*execution = created;

	/* Report successful resource creation. */
	return true;
}

/* Validate execution arrays, exact ABI counts, identities, and buffer plans. */
static bool
accel_vulkan_validate_execution_inputs(
	struct accel_vulkan_backend *backend,
	const struct accel_vulkan_prepared *prepared,
	uint32_t scalar_word_count,
	const uint32_t scalar_word[],
	uint32_t result_word_count,
	const uint32_t result_word[],
	uint32_t buffer_count,
	const struct accel_runtime_buffer buffer[],
	char *error,
	size_t error_size)
{
	const struct accel_program *program;
	size_t expected_scalar_count;
	size_t result_byte_count;
	uint32_t range_word;
	uint32_t i;
	bool any_dispatch;
	bool unavailable;

	/* Require a complete selected backend and immutable program plan. */
	if (backend == NULL ||
	    prepared == NULL ||
	    prepared->program == NULL) {
		accel_vulkan_set_runtime_error(
			error,
			error_size,
			N_TR("Invalid Vulkan execution request."));
		return false;
	}

	/* Borrow the immutable plan after validating its owner. */
	program = prepared->program;

	/* Reject work after a device or synchronization failure. */
	accel_mutex_lock(&backend->queue_mutex);
	unavailable = backend->poisoned || backend->device_lost;
	accel_mutex_unlock(&backend->queue_mutex);
	if (unavailable) {
		accel_vulkan_set_runtime_error(
			error,
			error_size,
			N_TR("The Vulkan accelerator device is unavailable."));
		return false;
	}

	/* Match every data buffer to the compiled binding namespace. */
	if (buffer_count != program->buffer_count) {
		accel_vulkan_set_runtime_error(
			error,
			error_size,
			N_TR("Vulkan buffer table does not match the prepared program."));
		return false;
	}

	/* Require the descriptor array whenever the program declares buffers. */
	if (buffer_count != 0 && buffer == NULL) {
		accel_vulkan_set_runtime_error(
			error,
			error_size,
			N_TR("Missing Vulkan buffer descriptors."));
		return false;
	}

	/* Match scalar values followed by two range words per kernel. */
	expected_scalar_count = program->scalar_count;
	expected_scalar_count += (size_t)program->kernel_count * 2;
	if ((size_t)scalar_word_count != expected_scalar_count) {
		accel_vulkan_set_runtime_error(
			error,
			error_size,
			N_TR("Vulkan scalar block does not match the prepared program."));
		return false;
	}

	/* Require scalar storage whenever the exact ABI count is nonzero. */
	if (scalar_word_count != 0 && scalar_word == NULL) {
		accel_vulkan_set_runtime_error(
			error,
			error_size,
			N_TR("Missing Vulkan scalar words."));
		return false;
	}

	/* Detect whether this execution needs any Vulkan storage or commands. */
	any_dispatch = false;
	for (i = 0; i < program->kernel_count; i++) {
		range_word = program->scalar_count + i * 2;

		/* Stop after finding the first nonempty source-loop range. */
		if (scalar_word[range_word + 1] != 0) {
			any_dispatch = true;
			break;
		}
	}

	/* Apply the scalar storage limit only when a command will use it. */
	if (any_dispatch &&
	    (expected_scalar_count > (size_t)-1 / sizeof(uint32_t) ||
	     expected_scalar_count * sizeof(uint32_t) >
	     backend->properties.limits.maxStorageBufferRange)) {
		accel_vulkan_set_runtime_error(
			error,
			error_size,
			N_TR("Vulkan scalar block exceeds the device storage-buffer limit."));
		return false;
	}

	/* Match scalar-result identities to the compiled result namespace. */
	if (result_word_count != program->scalar_result_count) {
		accel_vulkan_set_runtime_error(
			error,
			error_size,
			N_TR("Vulkan result block does not match the prepared program."));
		return false;
	}

	/* Reject storage that violates the zero-result NULL-array contract. */
	if (result_word_count == 0 && result_word != NULL) {
		accel_vulkan_set_runtime_error(
			error,
			error_size,
			N_TR("Unexpected Vulkan scalar-result identities."));
		return false;
	}

	/* Require identity storage for every declared scalar result. */
	if (result_word_count != 0 && result_word == NULL) {
		accel_vulkan_set_runtime_error(
			error,
			error_size,
			N_TR("Missing Vulkan scalar-result identities."));
		return false;
	}

	/* Match every result word to its immutable zero identity. */
	for (i = 0; i < result_word_count; i++) {
		/* Reject a result identity changed after program preparation. */
		if (result_word[i] != program->scalar_result[i].identity_bits) {
			accel_vulkan_set_runtime_error(
				error,
				error_size,
				N_TR("Vulkan scalar-result identity changed."));
			return false;
		}
	}

	/* Bound the optional result resource only for an active execution. */
	result_byte_count = (size_t)result_word_count * sizeof(uint32_t);
	if (any_dispatch &&
	    result_byte_count >
	    backend->properties.limits.maxStorageBufferRange) {
		accel_vulkan_set_runtime_error(
			error,
			error_size,
			N_TR("Vulkan result block exceeds the device storage-buffer limit."));
		return false;
	}

	/* Validate every runtime buffer against immutable binding metadata. */
	for (i = 0; i < buffer_count; i++) {
		/* Match the stable host and device binding namespaces exactly. */
		if (buffer[i].origin != program->buffer[i].origin ||
		    buffer[i].args_slot != program->buffer[i].args_slot ||
		    buffer[i].element_kind != program->buffer[i].element_kind ||
		    buffer[i].element_width != program->buffer[i].element_width) {
			accel_vulkan_set_runtime_error(
				error,
				error_size,
				N_TR("Vulkan buffer metadata changed after planning."));
			return false;
		}

		/* Restrict this backend to its raw 32-bit storage contract. */
		if (buffer[i].element_width != sizeof(uint32_t)) {
			accel_vulkan_set_runtime_error(
				error,
				error_size,
				N_TR("Vulkan supports only 32-bit accelerator buffers."));
			return false;
		}

		/* Match the byte extent to the checked element extent exactly. */
		if (buffer[i].element_count >
		    (size_t)-1 / buffer[i].element_width ||
		    buffer[i].element_count * buffer[i].element_width !=
		    buffer[i].byte_count) {
			accel_vulkan_set_runtime_error(
				error,
				error_size,
				N_TR("Vulkan buffer extent is inconsistent."));
			return false;
		}

		/* Reject an active binding that cannot back one shader word. */
		if (buffer[i].active && buffer[i].byte_count == 0) {
			accel_vulkan_set_runtime_error(
				error,
				error_size,
				N_TR("An active Vulkan buffer has no storage."));
			return false;
		}

		/* Keep inactive bindings free of host transfer side effects. */
		if (!buffer[i].active &&
		    (buffer[i].upload || buffer[i].download)) {
			accel_vulkan_set_runtime_error(
				error,
				error_size,
				N_TR("An inactive Vulkan buffer requested a transfer."));
			return false;
		}

		/* Apply the storage limit only to a resource that will exist. */
		if (buffer[i].active &&
		    buffer[i].byte_count >
		    backend->properties.limits.maxStorageBufferRange) {
			accel_vulkan_set_runtime_error(
				error,
				error_size,
				N_TR("Vulkan buffer exceeds the device storage-buffer limit."));
			return false;
		}

		/* Require plain storage for every requested host transfer. */
		if ((buffer[i].upload || buffer[i].download) &&
		    buffer[i].byte_count != 0 &&
		    buffer[i].snapshot == NULL) {
			accel_vulkan_set_runtime_error(
				error,
				error_size,
				N_TR("Missing Vulkan buffer snapshot."));
			return false;
		}
	}

	/* Report a complete and representable execution snapshot. */
	return true;
}

/* Create mapped storage, descriptors, and one recording command buffer. */
static bool
accel_vulkan_create_execution_resources(
	struct accel_vulkan_execution *execution,
	uint32_t scalar_word_count,
	const uint32_t scalar_word[],
	uint32_t result_word_count,
	const uint32_t result_word[],
	uint32_t buffer_count,
	const struct accel_runtime_buffer buffer[],
	char *error,
	size_t error_size)
{
	const struct accel_program *program;
	struct accel_vulkan_backend *backend;
	VkDeviceSize byte_count;
	uint32_t range_word;
	uint32_t i;
	bool any_dispatch;

	/* Borrow the immutable program and selected backend. */
	program = execution->prepared->program;
	backend = execution->backend;

	/* Allocate stable metadata for every declared data binding. */
	if (buffer_count != 0) {
		execution->buffer = noct_calloc(
			buffer_count,
			sizeof(*execution->buffer));
		if (execution->buffer == NULL) {
			accel_vulkan_set_runtime_error(
				error,
				error_size,
				N_TR("Out of memory while creating Vulkan buffers."));
			return false;
		}
	}

	/* Copy immutable transfer metadata before creating any Vulkan handle. */
	for (i = 0; i < buffer_count; i++) {
		execution->buffer[i].logical_size =
			(VkDeviceSize)buffer[i].byte_count;
		execution->buffer[i].active = buffer[i].active;
		execution->buffer[i].upload = buffer[i].upload;
		execution->buffer[i].download = buffer[i].download;
	}

	/* Detect whether the checked scalar block contains any active dispatch. */
	any_dispatch = false;
	for (i = 0; i < program->kernel_count; i++) {
		range_word = program->scalar_count + i * 2;

		/* Stop after finding the first nonempty source-loop range. */
		if (range_word + 1 < scalar_word_count &&
		    scalar_word[range_word + 1] != 0) {
			any_dispatch = true;
			break;
		}
	}

	/* Preserve identity and existing snapshots when every source loop is empty. */
	if (!any_dispatch)
		return true;

	/* Allocate every data binding needed by at least one active kernel. */
	for (i = 0; i < buffer_count; i++) {
		/* Leave unused bindings without a Vulkan resource. */
		if (!buffer[i].active)
			continue;

		/* Allocate the exact logical range of this active binding. */
		byte_count = (VkDeviceSize)buffer[i].byte_count;
		if (!accel_vulkan_create_host_buffer(
			backend,
			byte_count,
			VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
			&execution->buffer[i])) {
			accel_vulkan_set_runtime_error(
				error,
				error_size,
				N_TR("Failed to create a Vulkan data buffer."));
			return false;
		}
		execution->buffer[i].upload = buffer[i].upload;
		execution->buffer[i].download = buffer[i].download;

		/* Initialize only bindings whose first device use reads host data. */
		if (buffer[i].upload) {
			memcpy(
				execution->buffer[i].mapped,
				buffer[i].snapshot,
				buffer[i].byte_count);
		}
	}

	/* Allocate and initialize the scalar and dispatch-range storage block. */
	byte_count = (VkDeviceSize)scalar_word_count * sizeof(uint32_t);
	if (!accel_vulkan_create_host_buffer(
		backend,
		byte_count,
		VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
		&execution->scalar_buffer)) {
		accel_vulkan_set_runtime_error(
			error,
			error_size,
			N_TR("Failed to create the Vulkan scalar buffer."));
		return false;
	}
	execution->scalar_buffer.upload = true;
	memcpy(
		execution->scalar_buffer.mapped,
		scalar_word,
		(size_t)byte_count);

	/* Allocate the optional scalar-result block at binding B plus one. */
	if (result_word_count != 0) {
		byte_count = (VkDeviceSize)result_word_count * sizeof(uint32_t);
		if (!accel_vulkan_create_host_buffer(
			backend,
			byte_count,
			VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
			&execution->result_buffer)) {
			accel_vulkan_set_runtime_error(
				error,
				error_size,
				N_TR("Failed to create the Vulkan scalar-result buffer."));
			return false;
		}
		execution->result_buffer.upload = true;
		execution->result_buffer.download = true;
		memcpy(
			execution->result_buffer.mapped,
			result_word,
			(size_t)byte_count);
	}

	/* Publish all non-coherent host initialization before recording compute. */
	if (!accel_vulkan_flush_execution(execution, error, error_size))
		return false;

	/* Bind data 0..B-1, scalars B, and optional results B+1. */
	if (!accel_vulkan_create_descriptors(execution, error, error_size))
		return false;

	/* Begin the one-shot command buffer with a host-to-compute barrier. */
	if (!accel_vulkan_record_begin(execution, error, error_size))
		return false;

	/* Report a complete recording execution. */
	return true;
}

/* Create and map one host-visible Vulkan storage buffer. */
static bool
accel_vulkan_create_host_buffer(
	struct accel_vulkan_backend *backend,
	VkDeviceSize logical_size,
	VkBufferUsageFlags usage,
	struct accel_vulkan_buffer *buffer)
{
	VkBufferCreateInfo buffer_info;
	VkMemoryRequirements requirements;
	VkMemoryAllocateInfo allocate_info;
	VkDeviceSize allocation_size;
	VkDeviceSize atom;
	VkResult result;
	uint32_t memory_type;
	bool coherent;

	/* Reject empty storage because every active binding needs one word. */
	if (logical_size == 0)
		return false;

	/* Create the buffer handle with the exact logical descriptor range. */
	memset(&buffer_info, 0, sizeof(buffer_info));
	buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	buffer_info.size = logical_size;
	buffer_info.usage = usage;
	buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	result = backend->api.create_buffer(
		backend->device,
		&buffer_info,
		NULL,
		&buffer->buffer);
	if (result != VK_SUCCESS)
		return false;

	/* Select a compatible host-visible allocation type. */
	memset(&requirements, 0, sizeof(requirements));
	backend->api.get_buffer_memory_requirements(
		backend->device,
		buffer->buffer,
		&requirements);
	if (!accel_vulkan_find_host_memory(
		backend,
		requirements.memoryTypeBits,
		&memory_type,
		&coherent)) {
		return false;
	}

	/* Cover the driver requirement and complete non-coherent atom range. */
	allocation_size = requirements.size;
	if (allocation_size < logical_size)
		allocation_size = logical_size;
	atom = backend->properties.limits.nonCoherentAtomSize;
	if (!coherent &&
	    !accel_vulkan_round_allocation(
		allocation_size,
		atom,
		&allocation_size)) {
		return false;
	}

	/* Reject an allocation larger than its selected memory heap. */
	if (allocation_size >
	    backend->memory_properties.memoryHeaps[
		backend->memory_properties.memoryTypes[
			memory_type].heapIndex].size) {
		return false;
	}

	/* Allocate and bind one dedicated mapping. */
	memset(&allocate_info, 0, sizeof(allocate_info));
	allocate_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocate_info.allocationSize = allocation_size;
	allocate_info.memoryTypeIndex = memory_type;
	result = backend->api.allocate_memory(
		backend->device,
		&allocate_info,
		NULL,
		&buffer->memory);
	if (result != VK_SUCCESS)
		return false;

	/* Bind the dedicated allocation at its naturally aligned zero offset. */
	result = backend->api.bind_buffer_memory(
		backend->device,
		buffer->buffer,
		buffer->memory,
		0);
	if (result != VK_SUCCESS)
		return false;

	/* Keep the complete allocation persistently mapped for this execution. */
	result = backend->api.map_memory(
		backend->device,
		buffer->memory,
		0,
		allocation_size,
		0,
		&buffer->mapped);
	if (result != VK_SUCCESS)
		return false;

	/* Publish the complete mapped-buffer metadata after every call succeeds. */
	buffer->logical_size = logical_size;
	buffer->allocation_size = allocation_size;
	buffer->memory_type = memory_type;
	buffer->coherent = coherent;
	buffer->active = true;

	/* Report one complete dedicated mapped buffer. */
	return true;
}

/* Select a host-visible memory type, preferring coherent memory. */
static bool
accel_vulkan_find_host_memory(
	const struct accel_vulkan_backend *backend,
	uint32_t memory_type_bits,
	uint32_t *memory_type,
	bool *coherent)
{
	VkMemoryPropertyFlags flags;
	uint32_t fallback;
	uint32_t i;

	/* Prefer a coherent mapping and remember the first visible fallback. */
	fallback = UINT32_MAX;
	for (i = 0; i < backend->memory_properties.memoryTypeCount; i++) {
		/* Skip memory types excluded by this buffer requirement. */
		if ((memory_type_bits & (1U << i)) == 0)
			continue;

		/* Read this compatible memory type's host visibility flags. */
		flags = backend->memory_properties.memoryTypes[i].propertyFlags;

		/* Skip compatible types that the host cannot map. */
		if ((flags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) == 0)
			continue;

		/* Publish the first directly coherent compatible type. */
		if ((flags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) != 0) {
			*memory_type = i;
			*coherent = true;
			return true;
		}

		/* Remember the first non-coherent host-visible fallback. */
		if (fallback == UINT32_MAX)
			fallback = i;
	}

	/* Reject devices without any compatible host-visible type. */
	if (fallback == UINT32_MAX)
		return false;

	/* Publish the selected non-coherent fallback. */
	*memory_type = fallback;
	*coherent = false;

	/* Report the non-coherent fallback selection. */
	return true;
}

/* Round a non-coherent mapped allocation to the device atom size. */
static bool
accel_vulkan_round_allocation(
	VkDeviceSize value,
	VkDeviceSize atom,
	VkDeviceSize *result)
{
	VkDeviceSize remainder;

	/* Reject a malformed device limit. */
	if (atom == 0)
		return false;

	/* Detect whether the allocation already covers complete atoms. */
	remainder = value % atom;
	if (remainder == 0) {
		*result = value;
		return true;
	}

	/* Reject arithmetic overflow while extending the mapped range. */
	if (value > UINT64_MAX - (atom - remainder))
		return false;

	/* Publish the next complete non-coherent atom range. */
	*result = value + atom - remainder;

	/* Report the complete atom-aligned allocation. */
	return true;
}

/* Destroy one mapped buffer and its dedicated allocation. */
static void
accel_vulkan_destroy_buffer(
	struct accel_vulkan_backend *backend,
	struct accel_vulkan_buffer *buffer)
{
	/* Release the persistent mapping before freeing its memory. */
	if (buffer->mapped != NULL && buffer->memory != VK_NULL_HANDLE)
		backend->api.unmap_memory(backend->device, buffer->memory);

	/* Release the dedicated allocation before its buffer handle. */
	if (buffer->memory != VK_NULL_HANDLE)
		backend->api.free_memory(backend->device, buffer->memory, NULL);

	/* Release the logical buffer after all bound memory is gone. */
	if (buffer->buffer != VK_NULL_HANDLE)
		backend->api.destroy_buffer(backend->device, buffer->buffer, NULL);

	/* Clear all stale handles and mapping metadata. */
	memset(buffer, 0, sizeof(*buffer));
}

/* Allocate and populate one descriptor set for each prepared kernel. */
static bool
accel_vulkan_create_descriptors(
	struct accel_vulkan_execution *execution,
	char *error,
	size_t error_size)
{
	const struct accel_program *program;
	struct accel_vulkan_backend *backend;
	VkDescriptorPoolSize pool_size;
	VkDescriptorPoolCreateInfo pool_info;
	VkDescriptorSetLayout *layout;
	VkDescriptorSetAllocateInfo allocate_info;
	VkDescriptorBufferInfo *buffer_info;
	VkWriteDescriptorSet *write;
	VkResult result;
	uint32_t descriptor_count;
	uint32_t write_count;
	uint32_t i;
	uint32_t j;

	/* Recover the immutable binding namespace and selected backend. */
	program = execution->prepared->program;
	backend = execution->backend;
	descriptor_count = program->buffer_count + 1;

	/* Reserve binding B plus one only for a declared result block. */
	if (program->scalar_result_count != 0)
		descriptor_count++;

	/* Create a pool large enough for every declared per-kernel binding. */
	memset(&pool_size, 0, sizeof(pool_size));
	pool_size.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	pool_size.descriptorCount = descriptor_count * program->kernel_count;
	memset(&pool_info, 0, sizeof(pool_info));
	pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	pool_info.maxSets = program->kernel_count;
	pool_info.poolSizeCount = 1;
	pool_info.pPoolSizes = &pool_size;
	result = backend->api.create_descriptor_pool(
		backend->device,
		&pool_info,
		NULL,
		&execution->descriptor_pool);
	if (result != VK_SUCCESS) {
		accel_vulkan_set_runtime_error(
			error,
			error_size,
			N_TR("Failed to create the Vulkan descriptor pool."));
		return false;
	}

	/* Borrow every immutable kernel layout for one allocation transaction. */
	layout = noct_malloc(sizeof(*layout) * program->kernel_count);
	if (layout == NULL) {
		accel_vulkan_set_runtime_error(
			error,
			error_size,
			N_TR("Out of memory while creating Vulkan descriptor sets."));
		return false;
	}

	/* Copy every immutable per-kernel descriptor layout. */
	for (i = 0; i < program->kernel_count; i++)
		layout[i] = execution->prepared->kernel[i].descriptor_layout;

	/* Allocate one stable descriptor-set handle per kernel. */
	execution->descriptor_set = noct_calloc(
		program->kernel_count,
		sizeof(*execution->descriptor_set));
	if (execution->descriptor_set == NULL) {
		noct_free(layout);
		accel_vulkan_set_runtime_error(
			error,
			error_size,
			N_TR("Out of memory while storing Vulkan descriptor sets."));
		return false;
	}
	memset(&allocate_info, 0, sizeof(allocate_info));
	allocate_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocate_info.descriptorPool = execution->descriptor_pool;
	allocate_info.descriptorSetCount = program->kernel_count;
	allocate_info.pSetLayouts = layout;
	result = backend->api.allocate_descriptor_sets(
		backend->device,
		&allocate_info,
		execution->descriptor_set);
	noct_free(layout);
	if (result != VK_SUCCESS) {
		accel_vulkan_set_runtime_error(
			error,
			error_size,
			N_TR("Failed to allocate Vulkan descriptor sets."));
		return false;
	}

	/* Allocate one temporary write table large enough for one kernel. */
	buffer_info = noct_calloc(descriptor_count, sizeof(*buffer_info));
	if (buffer_info == NULL) {
		accel_vulkan_set_runtime_error(
			error,
			error_size,
			N_TR("Out of memory while writing Vulkan descriptors."));
		return false;
	}

	/* Allocate the matching descriptor-write table. */
	write = noct_calloc(descriptor_count, sizeof(*write));
	if (write == NULL) {
		noct_free(buffer_info);
		accel_vulkan_set_runtime_error(
			error,
			error_size,
			N_TR("Out of memory while writing Vulkan descriptors."));
		return false;
	}

	/* Populate statically used data bindings and the fixed ABI blocks. */
	for (i = 0; i < program->kernel_count; i++) {
		/* Start this kernel's dense temporary write table. */
		write_count = 0;

		/* Bind every active data buffer statically used by this kernel. */
		for (j = 0; j < program->buffer_count; j++) {
			/* Leave inactive and statically unused descriptors unbound. */
			if (!execution->buffer[j].active ||
			    !accel_vulkan_kernel_uses_buffer(
				program->kernel[i].ir,
				j)) {
				continue;
			}

			buffer_info[write_count].buffer =
				execution->buffer[j].buffer;
			buffer_info[write_count].offset = 0;
			buffer_info[write_count].range =
				execution->buffer[j].logical_size;
			memset(&write[write_count], 0, sizeof(write[write_count]));
			write[write_count].sType =
				VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			write[write_count].dstSet = execution->descriptor_set[i];
			write[write_count].dstBinding = j;
			write[write_count].descriptorCount = 1;
			write[write_count].descriptorType =
				VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
			write[write_count].pBufferInfo =
				&buffer_info[write_count];
			write_count++;
		}

		/* Bind the fixed scalar and dispatch-range block at binding B. */
		buffer_info[write_count].buffer = execution->scalar_buffer.buffer;
		buffer_info[write_count].offset = 0;
		buffer_info[write_count].range =
			execution->scalar_buffer.logical_size;
		memset(&write[write_count], 0, sizeof(write[write_count]));
		write[write_count].sType =
			VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		write[write_count].dstSet = execution->descriptor_set[i];
		write[write_count].dstBinding = program->buffer_count;
		write[write_count].descriptorCount = 1;
		write[write_count].descriptorType =
			VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		write[write_count].pBufferInfo = &buffer_info[write_count];
		write_count++;

		/* Bind the optional scalar-result resource at binding B plus one. */
		if (program->scalar_result_count != 0) {
			buffer_info[write_count].buffer =
				execution->result_buffer.buffer;
			buffer_info[write_count].offset = 0;
			buffer_info[write_count].range =
				execution->result_buffer.logical_size;
			memset(&write[write_count], 0, sizeof(write[write_count]));
			write[write_count].sType =
				VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			write[write_count].dstSet = execution->descriptor_set[i];
			write[write_count].dstBinding = program->buffer_count + 1;
			write[write_count].descriptorCount = 1;
			write[write_count].descriptorType =
				VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
			write[write_count].pBufferInfo =
				&buffer_info[write_count];
			write_count++;
		}

		/* Publish this complete kernel descriptor set atomically. */
		backend->api.update_descriptor_sets(
			backend->device,
			write_count,
			write,
			0,
			NULL);
	}

	/* Release the temporary descriptor assembly tables. */
	noct_free(write);
	noct_free(buffer_info);

	/* Report a complete descriptor namespace. */
	return true;
}

/* Report whether one typed kernel statically references a data buffer. */
static bool
accel_vulkan_kernel_uses_buffer(
	const struct accel_ir_kernel *kernel,
	uint32_t buffer_index)
{
	const struct accel_ir_instruction *instruction;
	uint32_t i;

	/* Inspect every typed load and store reference. */
	for (i = 0; i < kernel->instruction_count; i++) {
		instruction = &kernel->instruction[i];

		/* Skip instructions that cannot name a data buffer. */
		if (instruction->opcode != ACCEL_IR_BUFFER_LOAD &&
		    instruction->opcode != ACCEL_IR_BUFFER_STORE) {
			continue;
		}

		/* Report the first reference to the requested binding. */
		if (instruction->reference == buffer_index)
			return true;
	}

	/* Report an unused data binding. */
	return false;
}

/* Create and begin the execution command buffer and completion fence. */
static bool
accel_vulkan_record_begin(
	struct accel_vulkan_execution *execution,
	char *error,
	size_t error_size)
{
	struct accel_vulkan_backend *backend;
	VkCommandPoolCreateInfo pool_info;
	VkCommandBufferAllocateInfo allocate_info;
	VkCommandBufferBeginInfo begin_info;
	VkFenceCreateInfo fence_info;
	VkMemoryBarrier barrier;
	VkResult result;

	/* Recover the backend that owns all command objects. */
	backend = execution->backend;

	/* Create one command pool owned exclusively by this execution. */
	memset(&pool_info, 0, sizeof(pool_info));
	pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	pool_info.queueFamilyIndex = backend->queue_family;
	result = backend->api.create_command_pool(
		backend->device,
		&pool_info,
		NULL,
		&execution->command_pool);
	if (result != VK_SUCCESS) {
		accel_vulkan_set_runtime_error(
			error,
			error_size,
			N_TR("Failed to create a Vulkan command pool."));
		return false;
	}

	/* Allocate and begin one primary one-shot command buffer. */
	memset(&allocate_info, 0, sizeof(allocate_info));
	allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocate_info.commandPool = execution->command_pool;
	allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocate_info.commandBufferCount = 1;
	result = backend->api.allocate_command_buffers(
		backend->device,
		&allocate_info,
		&execution->command_buffer);
	if (result != VK_SUCCESS) {
		accel_vulkan_set_runtime_error(
			error,
			error_size,
			N_TR("Failed to allocate a Vulkan command buffer."));
		return false;
	}

	/* Begin the allocated primary buffer for one synchronous submission. */
	memset(&begin_info, 0, sizeof(begin_info));
	begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	result = backend->api.begin_command_buffer(
		execution->command_buffer,
		&begin_info);
	if (result != VK_SUCCESS) {
		accel_vulkan_set_runtime_error(
			error,
			error_size,
			N_TR("Failed to begin a Vulkan command buffer."));
		return false;
	}
	execution->command_started = true;

	/* Make all flushed host identities and uploads visible to compute. */
	memset(&barrier, 0, sizeof(barrier));
	barrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
	barrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
	barrier.dstAccessMask =
		VK_ACCESS_SHADER_READ_BIT |
		VK_ACCESS_SHADER_WRITE_BIT;
	backend->api.cmd_pipeline_barrier(
		execution->command_buffer,
		VK_PIPELINE_STAGE_HOST_BIT,
		VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
		0,
		1,
		&barrier,
		0,
		NULL,
		0,
		NULL);

	/* Create one unsignalled fence for synchronous completion proof. */
	memset(&fence_info, 0, sizeof(fence_info));
	fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	result = backend->api.create_fence(
		backend->device,
		&fence_info,
		NULL,
		&execution->fence);
	if (result != VK_SUCCESS) {
		accel_vulkan_set_runtime_error(
			error,
			error_size,
			N_TR("Failed to create a Vulkan completion fence."));
		return false;
	}

	/* Report a complete recording command owner. */
	return true;
}

/* Flush every non-coherent host write before command submission. */
static bool
accel_vulkan_flush_execution(
	struct accel_vulkan_execution *execution,
	char *error,
	size_t error_size)
{
	struct accel_vulkan_backend *backend;
	VkMappedMemoryRange range;
	VkResult result;
	uint32_t i;

	/* Recover the backend that owns all mapped allocations. */
	backend = execution->backend;

	/* Flush each mapped data buffer that received an upload snapshot. */
	for (i = 0; i < execution->buffer_count; i++) {
		/* Skip absent, device-initialized, and coherent mappings. */
		if (!execution->buffer[i].active ||
		    !execution->buffer[i].upload ||
		    execution->buffer[i].coherent) {
			continue;
		}

		/* Flush this complete atom-aligned mapped allocation. */
		memset(&range, 0, sizeof(range));
		range.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
		range.memory = execution->buffer[i].memory;
		range.offset = 0;
		range.size = execution->buffer[i].allocation_size;
		result = backend->api.flush_mapped_memory_ranges(
			backend->device,
			1,
			&range);
		if (result != VK_SUCCESS) {
			accel_vulkan_set_runtime_error(
				error,
				error_size,
				N_TR("Failed to flush Vulkan data-buffer memory."));
			return false;
		}
	}

	/* Flush the scalar input block when its memory is non-coherent. */
	if (execution->scalar_buffer.active &&
	    !execution->scalar_buffer.coherent) {
		memset(&range, 0, sizeof(range));
		range.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
		range.memory = execution->scalar_buffer.memory;
		range.offset = 0;
		range.size = execution->scalar_buffer.allocation_size;
		result = backend->api.flush_mapped_memory_ranges(
			backend->device,
			1,
			&range);
		if (result != VK_SUCCESS) {
			accel_vulkan_set_runtime_error(
				error,
				error_size,
				N_TR("Failed to flush Vulkan scalar memory."));
			return false;
		}
	}

	/* Flush scalar-result identities before their first atomic update. */
	if (execution->result_buffer.active &&
	    !execution->result_buffer.coherent) {
		memset(&range, 0, sizeof(range));
		range.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
		range.memory = execution->result_buffer.memory;
		range.offset = 0;
		range.size = execution->result_buffer.allocation_size;
		result = backend->api.flush_mapped_memory_ranges(
			backend->device,
			1,
			&range);
		if (result != VK_SUCCESS) {
			accel_vulkan_set_runtime_error(
				error,
				error_size,
				N_TR("Failed to flush Vulkan scalar-result memory."));
			return false;
		}
	}

	/* Report complete host-to-device visibility. */
	return true;
}

/* Record one active kernel in increasing source order. */
static bool
accel_vulkan_dispatch_execution(
	void *execution,
	uint32_t kernel_index,
	uint32_t start,
	uint32_t trip,
	char *error,
	size_t error_size)
{
	struct accel_vulkan_execution *active;
	struct accel_vulkan_backend *backend;
	const struct accel_program *program;
	const struct accel_vulkan_kernel *kernel;
	const uint32_t *scalar_word;
	VkMemoryBarrier barrier;
	uint64_t group_count;
	uint32_t range_word;
	uint32_t i;
	bool unavailable;

	/* Recover the opaque generic execution before validating it. */
	active = execution;
	if (active == NULL ||
	    active->prepared == NULL ||
	    active->finished) {
		accel_vulkan_set_runtime_error(
			error,
			error_size,
			N_TR("Invalid Vulkan execution."));
		return false;
	}

	/* Reject a kernel index outside the immutable prepared table. */
	if (kernel_index >= active->prepared->kernel_count) {
		accel_vulkan_set_runtime_error(
			error,
			error_size,
			N_TR("Invalid Vulkan kernel index."));
		return false;
	}

	/* Keep empty ranges inside the common runtime's no-dispatch path. */
	if (trip == 0) {
		accel_vulkan_set_runtime_error(
			error,
			error_size,
			N_TR("Vulkan received an empty active dispatch."));
		return false;
	}

	/* Enforce strictly increasing source kernel order. */
	if (active->dispatched && kernel_index <= active->last_kernel) {
		accel_vulkan_set_runtime_error(
			error,
			error_size,
			N_TR("Vulkan kernels were dispatched out of order."));
		return false;
	}

	/* Require all command and scalar resources created by active begin. */
	if (!active->command_started || active->scalar_buffer.mapped == NULL) {
		accel_vulkan_set_runtime_error(
			error,
			error_size,
			N_TR("Vulkan execution has no recording command buffer."));
		return false;
	}

	/* Recover the selected backend and immutable program plan. */
	backend = active->backend;
	program = active->prepared->program;

	/* Reject commands after another execution made the device unavailable. */
	accel_mutex_lock(&backend->queue_mutex);
	unavailable = backend->poisoned || backend->device_lost;
	accel_mutex_unlock(&backend->queue_mutex);
	if (unavailable) {
		accel_vulkan_set_runtime_error(
			error,
			error_size,
			N_TR("The Vulkan accelerator device is unavailable."));
		return false;
	}

	/* Match callback ranges to the immutable uploaded scalar block. */
	range_word = program->scalar_count + kernel_index * 2;
	if (range_word + 1 >= active->scalar_word_count) {
		accel_vulkan_set_runtime_error(
			error,
			error_size,
			N_TR("Vulkan dispatch range is missing from the scalar block."));
		return false;
	}

	/* Match the callback values to the immutable uploaded range words. */
	scalar_word = active->scalar_buffer.mapped;
	if (scalar_word[range_word] != start ||
	    scalar_word[range_word + 1] != trip) {
		accel_vulkan_set_runtime_error(
			error,
			error_size,
			N_TR("Vulkan dispatch range changed after execution creation."));
		return false;
	}

	/* Require storage for every data binding statically used by this kernel. */
	for (i = 0; i < active->buffer_count; i++) {
		/* Reject any statically used binding omitted from this execution. */
		if (accel_vulkan_kernel_uses_buffer(
			program->kernel[kernel_index].ir,
			i) &&
		    !active->buffer[i].active) {
			accel_vulkan_set_runtime_error(
				error,
				error_size,
				N_TR("Vulkan active kernel has an inactive data buffer."));
			return false;
		}
	}

	/* Revalidate the one-dimensional device workgroup count. */
	group_count = ((uint64_t)trip + ACCEL_VULKAN_WORKGROUP_SIZE - 1) /
		ACCEL_VULKAN_WORKGROUP_SIZE;
	if (group_count >
	    backend->properties.limits.maxComputeWorkGroupCount[0]) {
		accel_vulkan_set_runtime_error(
			error,
			error_size,
			N_TR("Vulkan dispatch exceeds the device workgroup-count limit."));
		return false;
	}

	/* Order every preceding shader write before this kernel reads or writes. */
	if (active->has_commands) {
		memset(&barrier, 0, sizeof(barrier));
		barrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
		barrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
		barrier.dstAccessMask =
			VK_ACCESS_SHADER_READ_BIT |
			VK_ACCESS_SHADER_WRITE_BIT;
		backend->api.cmd_pipeline_barrier(
			active->command_buffer,
			VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
			VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
			0,
			1,
			&barrier,
			0,
			NULL,
			0,
			NULL);
	}

	/* Bind the immutable pipeline and complete execution descriptor set. */
	kernel = &active->prepared->kernel[kernel_index];
	backend->api.cmd_bind_pipeline(
		active->command_buffer,
		VK_PIPELINE_BIND_POINT_COMPUTE,
		kernel->pipeline);
	backend->api.cmd_bind_descriptor_sets(
		active->command_buffer,
		VK_PIPELINE_BIND_POINT_COMPUTE,
		kernel->pipeline_layout,
		0,
		1,
		&active->descriptor_set[kernel_index],
		0,
		NULL);

	/* Record the checked active workgroups. */
	backend->api.cmd_dispatch(
		active->command_buffer,
		(uint32_t)group_count,
		1,
		1);
	active->last_kernel = kernel_index;
	active->has_commands = true;
	active->dispatched = true;

	/* Report a complete ordered dispatch recording. */
	return true;
}

/* Synchronize compute and fill result words plus download snapshots. */
static bool
accel_vulkan_finish_execution(
	void *execution,
	uint32_t result_word_count,
	uint32_t result_word[],
	uint32_t buffer_count,
	struct accel_runtime_buffer buffer[],
	char *error,
	size_t error_size)
{
	struct accel_vulkan_execution *active;
	struct accel_vulkan_backend *backend;
	VkMemoryBarrier barrier;
	VkResult result;

	/* Recover the opaque generic execution before validating outputs. */
	active = execution;
	if (!accel_vulkan_validate_finish(
		active,
		result_word_count,
		result_word,
		buffer_count,
		buffer,
		error,
		error_size)) {
		return false;
	}

	/* Preserve identities and existing snapshots when no kernel was active. */
	if (!active->has_commands) {
		active->finished = true;
		return true;
	}

	/* Make every shader write available to the host after completion. */
	backend = active->backend;
	memset(&barrier, 0, sizeof(barrier));
	barrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
	barrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
	barrier.dstAccessMask = VK_ACCESS_HOST_READ_BIT;
	backend->api.cmd_pipeline_barrier(
		active->command_buffer,
		VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
		VK_PIPELINE_STAGE_HOST_BIT,
		0,
		1,
		&barrier,
		0,
		NULL,
		0,
		NULL);

	/* Close the complete one-shot recording before queue publication. */
	result = backend->api.end_command_buffer(active->command_buffer);
	if (result != VK_SUCCESS) {
		accel_vulkan_set_runtime_error(
			error,
			error_size,
			N_TR("Failed to finish the Vulkan command buffer."));
		return false;
	}
	active->command_started = false;

	/* Submit and prove completion or preserve all in-flight ownership. */
	if (!accel_vulkan_submit_and_wait(active, error, error_size))
		return false;

	/* Invalidate every requested output before changing any runtime snapshot. */
	if (!accel_vulkan_invalidate_execution(active, error, error_size))
		return false;

	/* Publish all result bytes only after every fallible backend step succeeded. */
	accel_vulkan_copy_execution_results(
		active,
		result_word_count,
		result_word,
		buffer_count,
		buffer);
	active->finished = true;

	/* Report synchronous completion and atomic host publication. */
	return true;
}

/* Validate result destinations against immutable execution metadata. */
static bool
accel_vulkan_validate_finish(
	const struct accel_vulkan_execution *execution,
	uint32_t result_word_count,
	const uint32_t result_word[],
	uint32_t buffer_count,
	const struct accel_runtime_buffer buffer[],
	char *error,
	size_t error_size)
{
	uint32_t i;

	/* Require a live execution that has not crossed its publication boundary. */
	if (execution == NULL ||
	    execution->backend == NULL ||
	    execution->finished) {
		accel_vulkan_set_runtime_error(
			error,
			error_size,
			N_TR("Invalid Vulkan finish request."));
		return false;
	}

	/* Reject a second finish after any submission attempt. */
	if (execution->submission_state != ACCEL_VULKAN_SUBMISSION_NONE) {
		accel_vulkan_set_runtime_error(
			error,
			error_size,
			N_TR("Vulkan execution was already submitted."));
		return false;
	}

	/* Match the optional result block without dereferencing zero-result ABI. */
	if (result_word_count != execution->result_word_count) {
		accel_vulkan_set_runtime_error(
			error,
			error_size,
			N_TR("Vulkan scalar-result table changed."));
		return false;
	}

	/* Reject storage that violates the zero-result NULL-array contract. */
	if (result_word_count == 0 && result_word != NULL) {
		accel_vulkan_set_runtime_error(
			error,
			error_size,
			N_TR("Unexpected Vulkan scalar-result destination."));
		return false;
	}

	/* Require a destination for every declared result word. */
	if (result_word_count != 0 && result_word == NULL) {
		accel_vulkan_set_runtime_error(
			error,
			error_size,
			N_TR("Missing Vulkan scalar-result destination."));
		return false;
	}

	/* Match the complete buffer table before beginning synchronous work. */
	if (buffer_count != execution->buffer_count) {
		accel_vulkan_set_runtime_error(
			error,
			error_size,
			N_TR("Vulkan finish buffer table changed."));
		return false;
	}

	/* Require the buffer table whenever this execution declares bindings. */
	if (buffer_count != 0 && buffer == NULL) {
		accel_vulkan_set_runtime_error(
			error,
			error_size,
			N_TR("Missing Vulkan finish buffers."));
		return false;
	}

	/* Validate every output transfer before submitting any command. */
	for (i = 0; i < buffer_count; i++) {
		/* Match the immutable byte extent and transfer plan. */
		if ((VkDeviceSize)buffer[i].byte_count !=
		    execution->buffer[i].logical_size ||
		    buffer[i].active != execution->buffer[i].active ||
		    buffer[i].download != execution->buffer[i].download) {
			accel_vulkan_set_runtime_error(
				error,
				error_size,
				N_TR("Vulkan output transfer plan changed."));
			return false;
		}

		/* Require plain publication storage for every download. */
		if (buffer[i].download &&
		    buffer[i].byte_count != 0 &&
		    buffer[i].snapshot == NULL) {
			accel_vulkan_set_runtime_error(
				error,
				error_size,
				N_TR("Missing Vulkan output snapshot."));
			return false;
		}
	}

	/* Report a stable all-or-nothing publication boundary. */
	return true;
}

/* Submit one command buffer and prove that every referenced object is idle. */
static bool
accel_vulkan_submit_and_wait(
	struct accel_vulkan_execution *execution,
	char *error,
	size_t error_size)
{
	struct accel_vulkan_backend *backend;
	VkSubmitInfo submit_info;
	VkResult submit_result;
	VkResult wait_result;
	VkResult idle_result;
	bool unavailable;

	/* Build the one-command submission against its selected backend. */
	backend = execution->backend;
	memset(&submit_info, 0, sizeof(submit_info));
	submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submit_info.commandBufferCount = 1;
	submit_info.pCommandBuffers = &execution->command_buffer;

	/* Serialize queue publication and all device-wide drain attempts. */
	accel_mutex_lock(&backend->queue_mutex);
	unavailable = backend->poisoned || backend->device_lost;

	/* Reject this submission after an earlier terminal device failure. */
	if (unavailable) {
		accel_mutex_unlock(&backend->queue_mutex);
		accel_vulkan_set_runtime_error(
			error,
			error_size,
			N_TR("The Vulkan accelerator device is unavailable."));
		return false;
	}

	/* Publish the command buffer and mark ownership in flight only on success. */
	submit_result = backend->api.queue_submit(
		backend->queue,
		1,
		&submit_info,
		execution->fence);
	if (submit_result == VK_SUCCESS) {
		/* Mark the command graph pending before waiting on its fence. */
		execution->submission_state =
			ACCEL_VULKAN_SUBMISSION_IN_FLIGHT;
		wait_result = backend->api.wait_for_fences(
			backend->device,
			1,
			&execution->fence,
			VK_TRUE,
			UINT64_MAX);
	} else {
		/* Preserve the exact submission failure for device-loss handling. */
		wait_result = submit_result;
	}

	/* Convert a successful fence wait into an exact lifetime proof. */
	if (submit_result == VK_SUCCESS && wait_result == VK_SUCCESS) {
		execution->submission_state = ACCEL_VULKAN_SUBMISSION_DRAINED;
		accel_mutex_unlock(&backend->queue_mutex);
		return true;
	}

	/* Retry with a device-wide drain before preserving an uncertain graph. */
	idle_result = backend->api.device_wait_idle(backend->device);
	if (idle_result == VK_SUCCESS || idle_result == VK_ERROR_DEVICE_LOST) {
		execution->submission_state = ACCEL_VULKAN_SUBMISSION_DRAINED;

		/* Preserve terminal device loss after establishing cleanup safety. */
		if (idle_result == VK_ERROR_DEVICE_LOST ||
		    submit_result == VK_ERROR_DEVICE_LOST ||
		    wait_result == VK_ERROR_DEVICE_LOST) {
			backend->device_lost = true;
		}
	} else {
		/* Keep all referenced buffers and pipelines after an unproved drain. */
		execution->submission_state =
			ACCEL_VULKAN_SUBMISSION_ABANDONED;
		backend->poisoned = true;
	}
	accel_mutex_unlock(&backend->queue_mutex);

	/* Never expose any result bytes from a failed submission transaction. */
	accel_vulkan_set_runtime_error(
		error,
		error_size,
		N_TR("Vulkan accelerator submission failed."));
	return false;
}

/* Invalidate every non-coherent output after proved device completion. */
static bool
accel_vulkan_invalidate_execution(
	struct accel_vulkan_execution *execution,
	char *error,
	size_t error_size)
{
	struct accel_vulkan_backend *backend;
	VkMappedMemoryRange range;
	VkResult result;
	uint32_t i;

	/* Recover the backend that owns all completed mappings. */
	backend = execution->backend;

	/* Require the synchronous submission lifetime proof. */
	if (execution->submission_state != ACCEL_VULKAN_SUBMISSION_DRAINED) {
		accel_vulkan_set_runtime_error(
			error,
			error_size,
			N_TR("Vulkan output was requested before completion."));
		return false;
	}

	/* Invalidate each mapped data buffer requested by the runtime. */
	for (i = 0; i < execution->buffer_count; i++) {
		/* Skip absent, non-output, and coherent mappings. */
		if (!execution->buffer[i].active ||
		    !execution->buffer[i].download ||
		    execution->buffer[i].coherent) {
			continue;
		}

		/* Invalidate this complete atom-aligned mapped allocation. */
		memset(&range, 0, sizeof(range));
		range.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
		range.memory = execution->buffer[i].memory;
		range.offset = 0;
		range.size = execution->buffer[i].allocation_size;
		result = backend->api.invalidate_mapped_memory_ranges(
			backend->device,
			1,
			&range);
		if (result != VK_SUCCESS) {
			accel_vulkan_set_runtime_error(
				error,
				error_size,
				N_TR("Failed to invalidate Vulkan output memory."));
			return false;
		}
	}

	/* Invalidate the complete scalar-result block before publication. */
	if (execution->result_buffer.active &&
	    !execution->result_buffer.coherent) {
		memset(&range, 0, sizeof(range));
		range.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
		range.memory = execution->result_buffer.memory;
		range.offset = 0;
		range.size = execution->result_buffer.allocation_size;
		result = backend->api.invalidate_mapped_memory_ranges(
			backend->device,
			1,
			&range);
		if (result != VK_SUCCESS) {
			accel_vulkan_set_runtime_error(
				error,
				error_size,
				N_TR("Failed to invalidate Vulkan scalar-result memory."));
			return false;
		}
	}

	/* Report that every fallible readback operation completed. */
	return true;
}

/* Copy completed result bytes into runtime-owned publication storage. */
static void
accel_vulkan_copy_execution_results(
	struct accel_vulkan_execution *execution,
	uint32_t result_word_count,
	uint32_t result_word[],
	uint32_t buffer_count,
	struct accel_runtime_buffer buffer[])
{
	size_t byte_count;
	uint32_t i;

	/* Copy the complete scalar-result array as one publication paragraph. */
	if (result_word_count != 0) {
		byte_count = (size_t)result_word_count * sizeof(uint32_t);
		memcpy(result_word, execution->result_buffer.mapped, byte_count);
	}

	/* Fill every requested buffer snapshot after scalar readback is infallible. */
	for (i = 0; i < buffer_count; i++) {
		/* Skip inactive and input-only buffers. */
		if (!buffer[i].active || !buffer[i].download)
			continue;
		memcpy(
			buffer[i].snapshot,
			execution->buffer[i].mapped,
			buffer[i].byte_count);
	}
}

/* Destroy one partial or complete execution without waiting in cleanup. */
static void
accel_vulkan_destroy_execution(
	void *execution)
{
	struct accel_vulkan_execution *active;

	/* Recover and accept the optional generic execution owner. */
	active = execution;
	if (active == NULL)
		return;

	/* Preserve every object whose submitted lifetime remains unproved. */
	if (active->submission_state == ACCEL_VULKAN_SUBMISSION_IN_FLIGHT ||
	    active->submission_state == ACCEL_VULKAN_SUBMISSION_ABANDONED) {
		accel_vulkan_abandon_execution(active);
		return;
	}

	/* Release unsubmitted or synchronously drained Vulkan ownership. */
	accel_vulkan_destroy_execution_resources(active);
	noct_free(active->descriptor_set);
	noct_free(active->buffer);

	/* Release the empty execution wrapper after all private metadata. */
	memset(active, 0, sizeof(*active));
	noct_free(active);
}

/* Release all Vulkan handles owned by one proved-idle execution. */
static void
accel_vulkan_destroy_execution_resources(
	struct accel_vulkan_execution *execution)
{
	struct accel_vulkan_backend *backend;
	uint32_t i;

	/* Recover the backend that owns every execution resource. */
	backend = execution->backend;

	/* Release command ownership only after submission drain is proved. */
	if (execution->fence != VK_NULL_HANDLE) {
		backend->api.destroy_fence(
			backend->device,
			execution->fence,
			NULL);
		execution->fence = VK_NULL_HANDLE;
	}

	/* Release command-buffer ownership with its private command pool. */
	if (execution->command_pool != VK_NULL_HANDLE) {
		backend->api.destroy_command_pool(
			backend->device,
			execution->command_pool,
			NULL);
		execution->command_pool = VK_NULL_HANDLE;
		execution->command_buffer = VK_NULL_HANDLE;
	}

	/* Release descriptor ownership before the referenced mapped buffers. */
	if (execution->descriptor_pool != VK_NULL_HANDLE) {
		backend->api.destroy_descriptor_pool(
			backend->device,
			execution->descriptor_pool,
			NULL);
		execution->descriptor_pool = VK_NULL_HANDLE;
	}

	/* Release every dedicated data and fixed-ABI storage allocation. */
	if (execution->buffer != NULL) {
		/* Release every data binding in stable descriptor order. */
		for (i = 0; i < execution->buffer_count; i++) {
			accel_vulkan_destroy_buffer(
				backend,
				&execution->buffer[i]);
		}
	}
	accel_vulkan_destroy_buffer(backend, &execution->result_buffer);
	accel_vulkan_destroy_buffer(backend, &execution->scalar_buffer);

	/* Clear the borrowed prepared-program reference after handle cleanup. */
	execution->prepared = NULL;
}

/* Transfer one unproved in-flight execution to backend destruction. */
static void
accel_vulkan_abandon_execution(
	struct accel_vulkan_execution *execution)
{
	struct accel_vulkan_backend *backend;

	/* Recover the backend that must retain the submitted object graph. */
	backend = execution->backend;

	/* Link the complete object graph exactly once under queue serialization. */
	accel_mutex_lock(&backend->queue_mutex);
	if (!execution->abandoned_linked) {
		execution->submission_state =
			ACCEL_VULKAN_SUBMISSION_ABANDONED;
		execution->next_abandoned = backend->abandoned_execution_head;
		backend->abandoned_execution_head = execution;
		execution->abandoned_linked = true;
		backend->poisoned = true;
	}
	accel_mutex_unlock(&backend->queue_mutex);
}

/* Destroy all backend-owned executions after a successful device-wide drain. */
static void
accel_vulkan_destroy_abandoned_executions(
	struct accel_vulkan_backend *backend)
{
	struct accel_vulkan_execution *execution;

	/* Pop each execution before invoking ordinary drained cleanup. */
	while (backend->abandoned_execution_head != NULL) {
		execution = backend->abandoned_execution_head;
		backend->abandoned_execution_head = execution->next_abandoned;
		execution->next_abandoned = NULL;
		execution->abandoned_linked = false;
		execution->submission_state = ACCEL_VULKAN_SUBMISSION_DRAINED;
		accel_vulkan_destroy_execution(execution);
	}
}

/* Destroy one raw prepared payload after all referencing commands drain. */
static void
accel_vulkan_destroy_prepared_payload(
	struct accel_vulkan_backend *backend,
	struct accel_vulkan_prepared *prepared)
{
	uint32_t i;

	/* Accept cleanup of an optional compiler payload. */
	if (prepared == NULL)
		return;

	/* Release every immutable pipeline before its program metadata. */
	for (i = 0; i < prepared->kernel_count; i++)
		accel_vulkan_destroy_kernel(backend, &prepared->kernel[i]);

	/* Release target-neutral metadata and the emptied payload wrapper. */
	accel_program_destroy(prepared->program);
	noct_free(prepared->kernel);
	memset(prepared, 0, sizeof(*prepared));
	noct_free(prepared);
}

/* Destroy all prepared payloads deferred behind abandoned executions. */
static void
accel_vulkan_destroy_deferred_prepared(
	struct accel_vulkan_backend *backend)
{
	struct accel_vulkan_prepared *prepared;

	/* Pop each payload only after every abandoned command object is gone. */
	while (backend->deferred_prepared_head != NULL) {
		prepared = backend->deferred_prepared_head;
		backend->deferred_prepared_head = prepared->next_deferred;
		prepared->next_deferred = NULL;
		accel_vulkan_destroy_prepared_payload(backend, prepared);
	}
}

/* Copy one backend runtime diagnostic into optional caller storage. */
static void
accel_vulkan_set_runtime_error(
	char *error,
	size_t error_size,
	const char *message)
{
	size_t length;

	/* Preserve a valid zero-result diagnostic ABI. */
	if (error == NULL || error_size == 0)
		return;

	/* Copy at most one complete caller-owned buffer. */
	length = strlen(message);
	if (length >= error_size)
		length = error_size - 1;
	memcpy(error, message, length);
	error[length] = '\0';
}
