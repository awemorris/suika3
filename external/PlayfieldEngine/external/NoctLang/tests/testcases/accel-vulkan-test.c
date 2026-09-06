/* -*- coding: utf-8; tab-width: 8; indent-tabs-mode: t; -*- */

/*
 * Noct Programming Language
 * Copyright (c) 2025, 2026, Awe Morris
 */

/*
 * Device-independent Vulkan accelerator contract tests.
 */

#include "accel_vulkan.h"
#include "accel_vulkan_shader.h"
#include "runtime.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ACCEL_TEST_SPIRV_OP_ATOMIC_IADD	234U

enum mock_mode {
	MOCK_SUCCESS,
	MOCK_VERSION_MISSING,
	MOCK_VERSION_FAILURE,
	MOCK_VERSION_1_1,
	MOCK_PROPERTIES_MISSING,
	MOCK_IDLE_FAILURE
};

struct mock_state {
	int mode;
	uint32_t get_proc_count;
	uint32_t create_instance_count;
	uint32_t destroy_instance_count;
	uint32_t enumerate_device_count;
	uint32_t create_device_count;
	uint32_t destroy_device_count;
	uint32_t device_wait_idle_count;
};

static struct mock_state mock;

static PFN_vkVoidFunction VKAPI_PTR mock_get_instance_proc_addr(VkInstance instance, const char *name);
static VkResult VKAPI_PTR mock_enumerate_instance_version(uint32_t *version);
static VkResult VKAPI_PTR mock_create_instance(const VkInstanceCreateInfo *create_info, const VkAllocationCallbacks *allocator, VkInstance *instance);
static void VKAPI_PTR mock_destroy_instance(VkInstance instance, const VkAllocationCallbacks *allocator);
static VkResult VKAPI_PTR mock_enumerate_physical_devices(VkInstance instance, uint32_t *count, VkPhysicalDevice *device);
static void VKAPI_PTR mock_get_physical_device_properties2(VkPhysicalDevice device, VkPhysicalDeviceProperties2 *properties);
static void VKAPI_PTR mock_get_queue_properties(VkPhysicalDevice device, uint32_t *count, VkQueueFamilyProperties *properties);
static void VKAPI_PTR mock_get_memory_properties(VkPhysicalDevice device, VkPhysicalDeviceMemoryProperties *properties);
static VkResult VKAPI_PTR mock_create_device(VkPhysicalDevice physical_device, const VkDeviceCreateInfo *create_info, const VkAllocationCallbacks *allocator, VkDevice *device);
static void VKAPI_PTR mock_destroy_device(VkDevice device, const VkAllocationCallbacks *allocator);
static void VKAPI_PTR mock_get_device_queue(VkDevice device, uint32_t family, uint32_t index, VkQueue *queue);
static VkResult VKAPI_PTR mock_device_wait_idle(VkDevice device);
static void mock_reset(int mode);
static void mock_make_api(struct accel_vulkan_api *api);
static bool test_missing_required_function(struct rt_env *env);
static bool test_version_cases(struct rt_env *env);
static bool test_properties_resolution(struct rt_env *env);
static bool test_device_enumeration(void);
static bool test_device_selection(struct rt_env *env);
static bool append_shader_instruction(struct accel_ir_builder *builder, int opcode, int result_type, uint32_t reference, uint32_t operand0);
static bool spirv_has_opcode(const struct accel_vulkan_spirv *spirv, uint32_t requested_opcode);
static bool test_scalar_result_shader(void);
static bool expect_failure(struct rt_env *env, struct accel_vulkan_api *api, const char *gpu_name);

/* Run the device-independent Vulkan initialization contract tests. */
int
main(
	int argc,
	char *argv[])
{
	struct rt_vm *vm;
	struct rt_env *env;
	struct rt_config config;
	bool success;

	UNUSED_PARAMETER(argc);
	UNUSED_PARAMETER(argv);

	/* Exercise device listing before any VM or environment exists. */
	success = test_device_enumeration();
	if (!success)
		return 1;

	/* Create the environment required by backend creation diagnostics. */
	noct_set_default_config(&config);
	config.jit_enable = 0;
	if (!noct_create_vm(&vm, &env, &config)) {
		fprintf(stderr, "failed to create test VM\n");
		return 1;
	}

	/* Run every environment-backed initialization contract. */
	success = test_missing_required_function(env);
	if (success)
		success = test_version_cases(env);
	if (success)
		success = test_properties_resolution(env);
	if (success)
		success = test_device_selection(env);
	if (success)
		success = test_scalar_result_shader();

	if (!noct_destroy_vm(vm))
		success = false;

	if (!success)
		return 1;

	printf("Vulkan accelerator plan tests passed.\n");

	return 0;
}

/* Return one injected loader or instance function. */
static PFN_vkVoidFunction VKAPI_PTR
mock_get_instance_proc_addr(
	VkInstance instance,
	const char *name)
{
	mock.get_proc_count++;
	if (instance == VK_NULL_HANDLE &&
	    strcmp(name, "vkEnumerateInstanceVersion") == 0) {
		if (mock.mode == MOCK_VERSION_MISSING)
			return NULL;

		return (PFN_vkVoidFunction)mock_enumerate_instance_version;
	}
	if (instance != VK_NULL_HANDLE &&
	    strcmp(name, "vkGetPhysicalDeviceProperties2") == 0) {
		if (mock.mode == MOCK_PROPERTIES_MISSING)
			return NULL;

		return (PFN_vkVoidFunction)mock_get_physical_device_properties2;
	}

	return NULL;
}

/* Return the selected mock loader version. */
static VkResult VKAPI_PTR
mock_enumerate_instance_version(
	uint32_t *version)
{
	if (mock.mode == MOCK_VERSION_FAILURE)
		return VK_ERROR_INITIALIZATION_FAILED;
	if (mock.mode == MOCK_VERSION_1_1)
		*version = VK_API_VERSION_1_1;
	else
		*version = VK_API_VERSION_1_2;

	return VK_SUCCESS;
}

/* Create one opaque mock instance. */
static VkResult VKAPI_PTR
mock_create_instance(
	const VkInstanceCreateInfo *create_info,
	const VkAllocationCallbacks *allocator,
	VkInstance *instance)
{
	UNUSED_PARAMETER(allocator);

	if (create_info->pApplicationInfo->apiVersion != VK_API_VERSION_1_2)
		return VK_ERROR_INITIALIZATION_FAILED;

	mock.create_instance_count++;
	*instance = (VkInstance)(uintptr_t)1;

	return VK_SUCCESS;
}

/* Destroy one opaque mock instance. */
static void VKAPI_PTR
mock_destroy_instance(
	VkInstance instance,
	const VkAllocationCallbacks *allocator)
{
	UNUSED_PARAMETER(instance);
	UNUSED_PARAMETER(allocator);

	mock.destroy_instance_count++;
}

/* Enumerate one opaque mock physical device. */
static VkResult VKAPI_PTR
mock_enumerate_physical_devices(
	VkInstance instance,
	uint32_t *count,
	VkPhysicalDevice *device)
{
	UNUSED_PARAMETER(instance);

	mock.enumerate_device_count++;
	if (device == NULL) {
		*count = 1;
		return VK_SUCCESS;
	}

	if (*count == 0)
		return VK_INCOMPLETE;

	device[0] = (VkPhysicalDevice)(uintptr_t)2;
	*count = 1;

	return VK_SUCCESS;
}

/* Fill Vulkan 1.2 limits and strict Float32 properties. */
static void VKAPI_PTR
mock_get_physical_device_properties2(
	VkPhysicalDevice device,
	VkPhysicalDeviceProperties2 *properties)
{
	VkPhysicalDeviceFloatControlsProperties *float_controls;

	UNUSED_PARAMETER(device);

	properties->properties.apiVersion = VK_API_VERSION_1_2;
	properties->properties.deviceType = VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU;
	properties->properties.limits.maxComputeWorkGroupInvocations = 256;
	properties->properties.limits.maxComputeWorkGroupSize[0] = 256;
	properties->properties.limits.maxComputeWorkGroupCount[0] = 65535;
	properties->properties.limits.maxPerStageDescriptorStorageBuffers = 64;
	properties->properties.limits.maxDescriptorSetStorageBuffers = 64;
	properties->properties.limits.maxStorageBufferRange = 1U << 24;
	properties->properties.limits.nonCoherentAtomSize = 64;
	(void)strcpy(properties->properties.deviceName, "Mock GPU");

	float_controls = properties->pNext;
	if (float_controls != NULL) {
		float_controls->shaderSignedZeroInfNanPreserveFloat32 = VK_TRUE;
		float_controls->shaderDenormPreserveFloat32 = VK_TRUE;
		float_controls->shaderRoundingModeRTEFloat32 = VK_TRUE;
	}
}

/* Expose one compute-capable queue family. */
static void VKAPI_PTR
mock_get_queue_properties(
	VkPhysicalDevice device,
	uint32_t *count,
	VkQueueFamilyProperties *properties)
{
	UNUSED_PARAMETER(device);

	if (properties == NULL) {
		*count = 1;
		return;
	}

	memset(&properties[0], 0, sizeof(properties[0]));
	properties[0].queueFlags = VK_QUEUE_COMPUTE_BIT;
	properties[0].queueCount = 1;
	*count = 1;
}

/* Expose one host-visible coherent memory type. */
static void VKAPI_PTR
mock_get_memory_properties(
	VkPhysicalDevice device,
	VkPhysicalDeviceMemoryProperties *properties)
{
	UNUSED_PARAMETER(device);

	memset(properties, 0, sizeof(*properties));
	properties->memoryHeapCount = 1;
	properties->memoryHeaps[0].size = 1U << 24;
	properties->memoryTypeCount = 1;
	properties->memoryTypes[0].heapIndex = 0;
	properties->memoryTypes[0].propertyFlags =
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
		VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
}

/* Create one opaque mock logical device. */
static VkResult VKAPI_PTR
mock_create_device(
	VkPhysicalDevice physical_device,
	const VkDeviceCreateInfo *create_info,
	const VkAllocationCallbacks *allocator,
	VkDevice *device)
{
	UNUSED_PARAMETER(physical_device);
	UNUSED_PARAMETER(create_info);
	UNUSED_PARAMETER(allocator);

	mock.create_device_count++;
	*device = (VkDevice)(uintptr_t)3;

	return VK_SUCCESS;
}

/* Destroy one opaque mock logical device. */
static void VKAPI_PTR
mock_destroy_device(
	VkDevice device,
	const VkAllocationCallbacks *allocator)
{
	UNUSED_PARAMETER(device);
	UNUSED_PARAMETER(allocator);

	mock.destroy_device_count++;
}

/* Return one opaque mock compute queue. */
static void VKAPI_PTR
mock_get_device_queue(
	VkDevice device,
	uint32_t family,
	uint32_t index,
	VkQueue *queue)
{
	UNUSED_PARAMETER(device);
	UNUSED_PARAMETER(family);
	UNUSED_PARAMETER(index);

	*queue = (VkQueue)(uintptr_t)4;
}

/* Complete mock device cleanup without waiting on hardware. */
static VkResult VKAPI_PTR
mock_device_wait_idle(
	VkDevice device)
{
	UNUSED_PARAMETER(device);

	/* Count every device-wide lifetime proof requested by teardown. */
	mock.device_wait_idle_count++;

	/* Preserve the selected backend when a final drain cannot be proved. */
	if (mock.mode == MOCK_IDLE_FAILURE)
		return VK_ERROR_OUT_OF_HOST_MEMORY;

	/* Report a proved-idle mock device. */
	return VK_SUCCESS;
}

/* Reset all call counters and select one injected behavior. */
static void
mock_reset(
	int mode)
{
	memset(&mock, 0, sizeof(mock));
	mock.mode = mode;
}

/* Fill one complete table while replacing only bootstrap calls with mocks. */
static void
mock_make_api(
	struct accel_vulkan_api *api)
{
	memset(api, 0, sizeof(*api));
	api->get_instance_proc_addr = mock_get_instance_proc_addr;
	api->create_instance = mock_create_instance;
	api->destroy_instance = mock_destroy_instance;
	api->enumerate_physical_devices = mock_enumerate_physical_devices;
	api->get_physical_device_properties2 = mock_get_physical_device_properties2;
	api->get_physical_device_queue_family_properties = mock_get_queue_properties;
	api->get_physical_device_memory_properties = mock_get_memory_properties;
	api->create_device = mock_create_device;
	api->destroy_device = mock_destroy_device;
	api->get_device_queue = mock_get_device_queue;
	api->device_wait_idle = mock_device_wait_idle;
	api->create_shader_module = vkCreateShaderModule;
	api->destroy_shader_module = vkDestroyShaderModule;
	api->create_descriptor_set_layout = vkCreateDescriptorSetLayout;
	api->destroy_descriptor_set_layout = vkDestroyDescriptorSetLayout;
	api->create_pipeline_layout = vkCreatePipelineLayout;
	api->destroy_pipeline_layout = vkDestroyPipelineLayout;
	api->create_compute_pipelines = vkCreateComputePipelines;
	api->destroy_pipeline = vkDestroyPipeline;
	api->create_buffer = vkCreateBuffer;
	api->destroy_buffer = vkDestroyBuffer;
	api->get_buffer_memory_requirements = vkGetBufferMemoryRequirements;
	api->allocate_memory = vkAllocateMemory;
	api->free_memory = vkFreeMemory;
	api->bind_buffer_memory = vkBindBufferMemory;
	api->map_memory = vkMapMemory;
	api->unmap_memory = vkUnmapMemory;
	api->flush_mapped_memory_ranges = vkFlushMappedMemoryRanges;
	api->invalidate_mapped_memory_ranges = vkInvalidateMappedMemoryRanges;
	api->create_descriptor_pool = vkCreateDescriptorPool;
	api->destroy_descriptor_pool = vkDestroyDescriptorPool;
	api->allocate_descriptor_sets = vkAllocateDescriptorSets;
	api->update_descriptor_sets = vkUpdateDescriptorSets;
	api->create_command_pool = vkCreateCommandPool;
	api->destroy_command_pool = vkDestroyCommandPool;
	api->allocate_command_buffers = vkAllocateCommandBuffers;
	api->begin_command_buffer = vkBeginCommandBuffer;
	api->end_command_buffer = vkEndCommandBuffer;
	api->cmd_bind_pipeline = vkCmdBindPipeline;
	api->cmd_bind_descriptor_sets = vkCmdBindDescriptorSets;
	api->cmd_copy_buffer = vkCmdCopyBuffer;
	api->cmd_pipeline_barrier = vkCmdPipelineBarrier;
	api->cmd_fill_buffer = vkCmdFillBuffer;
	api->cmd_dispatch = vkCmdDispatch;
	api->create_fence = vkCreateFence;
	api->destroy_fence = vkDestroyFence;
	api->queue_submit = vkQueueSubmit;
	api->wait_for_fences = vkWaitForFences;
}

/* Reject an incomplete table before making the first Vulkan call. */
static bool
test_missing_required_function(
	struct rt_env *env)
{
	struct accel_vulkan_api api;

	mock_reset(MOCK_SUCCESS);
	mock_make_api(&api);
	api.create_buffer = NULL;
	if (!expect_failure(env, &api, NULL))
		return false;
	if (mock.get_proc_count != 0 || mock.create_instance_count != 0)
		return false;

	return true;
}

/* Reject missing, failed, and pre-1.2 loader version queries. */
static bool
test_version_cases(
	struct rt_env *env)
{
	struct accel_vulkan_api api;

	mock_reset(MOCK_VERSION_MISSING);
	mock_make_api(&api);
	if (!expect_failure(env, &api, NULL))
		return false;
	if (mock.create_instance_count != 0)
		return false;

	mock_reset(MOCK_VERSION_FAILURE);
	mock_make_api(&api);
	if (!expect_failure(env, &api, NULL))
		return false;
	if (mock.create_instance_count != 0)
		return false;

	mock_reset(MOCK_VERSION_1_1);
	mock_make_api(&api);
	if (!expect_failure(env, &api, NULL))
		return false;
	if (mock.create_instance_count != 0)
		return false;

	return true;
}

/* Resolve properties through the instance and clean up on lookup failure. */
static bool
test_properties_resolution(
	struct rt_env *env)
{
	struct accel_vulkan_api api;

	mock_reset(MOCK_PROPERTIES_MISSING);
	mock_make_api(&api);
	if (!expect_failure(env, &api, NULL))
		return false;
	if (mock.create_instance_count != 1)
		return false;
	if (mock.destroy_instance_count != 1)
		return false;
	if (mock.enumerate_device_count != 0)
		return false;

	return true;
}

/* Enumerate a suitable device without creating its logical device. */
static bool
test_device_enumeration(void)
{
	struct accel_vulkan_api api;
	struct accel_device_list list;
	char error[128];
	bool success;

	mock_reset(MOCK_SUCCESS);
	mock_make_api(&api);
	accel_device_list_init(&list);

	/* Enumerate through the no-VM diagnostic path. */
	success = accel_vulkan_enumerate_with_api(
		&list,
		&api,
		error,
		sizeof(error));

	/* Require one deep-owned canonical Vulkan record. */
	if (success && error[0] != '\0')
		success = false;
	if (success && list.count != 1)
		success = false;
	if (success && strcmp(list.device[0].name, "Mock GPU") != 0)
		success = false;
	if (success &&
	    strcmp(list.device[0].selector, "vulkan:Mock GPU") != 0) {
		success = false;
	}

	/* Prohibit logical-device creation during listing. */
	if (success && mock.create_device_count != 0)
		success = false;
	if (success && mock.destroy_device_count != 0)
		success = false;
	if (success && mock.destroy_instance_count != 1)
		success = false;

	accel_device_list_destroy(&list);

	/* Exercise one no-VM loader failure and its standalone diagnostic. */
	if (success) {
		mock_reset(MOCK_PROPERTIES_MISSING);
		mock_make_api(&api);
		accel_device_list_init(&list);
		if (accel_vulkan_enumerate_with_api(
			&list,
			&api,
			error,
			sizeof(error))) {
			success = false;
		}
		if (success && error[0] == '\0')
			success = false;
		if (success && list.count != 0)
			success = false;
		if (success && mock.create_device_count != 0)
			success = false;
		if (success && mock.destroy_instance_count != 1)
			success = false;
		accel_device_list_destroy(&list);
	}

	/* Report the complete enumeration contract result. */
	return success;
}

/* Select an exact mock device and release all owned backend resources. */
static bool
test_device_selection(
	struct rt_env *env)
{
	struct accel_vulkan_api api;
	const struct accel_backend_ops *ops;
	void *backend_state;

	mock_reset(MOCK_SUCCESS);
	mock_make_api(&api);
	if (!expect_failure(env, &api, "Other GPU"))
		return false;
	if (mock.create_device_count != 0)
		return false;

	mock_reset(MOCK_SUCCESS);
	mock_make_api(&api);
	ops = NULL;
	backend_state = NULL;
	if (!accel_vulkan_create_with_api(
		env,
		"Mock GPU",
		&api,
		&ops,
		&backend_state)) {
		return false;
	}
	if (ops == NULL || backend_state == NULL)
		return false;
	if (mock.create_device_count != 1)
		return false;

	/* Release the ordinary backend through one successful final drain. */
	ops->destroy_backend_state(backend_state);

	/* Require exactly one ordinary final drain. */
	if (mock.device_wait_idle_count != 1)
		return false;

	/* Require exactly one logical-device release. */
	if (mock.destroy_device_count != 1)
		return false;

	/* Require exactly one instance release. */
	if (mock.destroy_instance_count != 1)
		return false;

	/* Retain every backend object after an unproved final device drain. */
	mock_reset(MOCK_IDLE_FAILURE);
	mock_make_api(&api);
	ops = NULL;
	backend_state = NULL;
	if (!accel_vulkan_create_with_api(
		env,
		"Mock GPU",
		&api,
		&ops,
		&backend_state)) {
		return false;
	}
	ops->destroy_backend_state(backend_state);

	/* Require the failed teardown to attempt one final drain. */
	if (mock.device_wait_idle_count != 1)
		return false;

	/* Preserve the logical device while its lifetime is unproved. */
	if (mock.destroy_device_count != 0)
		return false;

	/* Preserve the parent instance with its retained logical device. */
	if (mock.destroy_instance_count != 0)
		return false;

	/* Retry the preserved graph and release it after a proved idle result. */
	mock.mode = MOCK_SUCCESS;
	ops->destroy_backend_state(backend_state);

	/* Require one failed and one successful final drain attempt. */
	if (mock.device_wait_idle_count != 2)
		return false;

	/* Release the retained logical device exactly once. */
	if (mock.destroy_device_count != 1)
		return false;

	/* Release the retained parent instance exactly once. */
	if (mock.destroy_instance_count != 1)
		return false;

	return true;
}

/* Append one fully initialized scalar-result shader instruction. */
static bool
append_shader_instruction(
	struct accel_ir_builder *builder,
	int opcode,
	int result_type,
	uint32_t reference,
	uint32_t operand0)
{
	struct accel_ir_instruction instruction;
	bool success;

	/* Initialize one complete target-neutral instruction. */
	memset(&instruction, 0, sizeof(instruction));
	instruction.opcode = opcode;
	instruction.result_type = result_type;
	instruction.result = ACCEL_IR_VALUE_NONE;
	instruction.operand[0] = operand0;
	instruction.operand[1] = ACCEL_IR_VALUE_NONE;
	instruction.operand[2] = ACCEL_IR_VALUE_NONE;
	instruction.reference = reference;

	/* Append the instruction through the checked IR builder. */
	success = accel_ir_builder_append(builder, &instruction, NULL);

	/* Report whether the builder accepted the instruction. */
	return success;
}

/* Find one opcode in a structurally bounded SPIR-V instruction stream. */
static bool
spirv_has_opcode(
	const struct accel_vulkan_spirv *spirv,
	uint32_t requested_opcode)
{
	size_t offset;
	uint32_t word_count;
	uint32_t opcode;

	if (spirv == NULL || spirv->word == NULL || spirv->word_count < 5)
		return false;

	offset = 5;

	/* Inspect each complete instruction after the five-word header. */
	while (offset < spirv->word_count) {
		word_count = spirv->word[offset] >> 16;
		opcode = spirv->word[offset] & 0xffffU;
		if (word_count == 0 || word_count > spirv->word_count - offset)
			return false;
		if (opcode == requested_opcode)
			return true;
		offset += word_count;
	}

	return false;
}

/* Assemble valid Vulkan modules for both sides of one scalar result. */
static bool
test_scalar_result_shader(
	void)
{
	struct accel_program program;
	struct accel_scalar_result scalar_result;
	struct accel_kernel_plan kernel[2];
	struct accel_ir_kernel *producer;
	struct accel_ir_kernel *consumer;
	struct accel_ir_builder builder;
	struct accel_vulkan_spirv spirv;
	shaderc_compiler_t compiler;
	shaderc_compile_options_t options;
	enum accel_compile_status status;
	bool success;

	/* Build one static I32 atomic producer. */
	producer = accel_ir_kernel_create("result_producer", 1, 1, 0, 0);
	if (producer == NULL)
		return false;
	accel_ir_builder_init(&builder, producer);
	success = append_shader_instruction(
		&builder,
		ACCEL_IR_CONST_I32,
		ACCEL_IR_I32,
		ACCEL_IR_REFERENCE_NONE,
		ACCEL_IR_VALUE_NONE);
	if (success) {
		producer->instruction[0].literal_bits = 7;
		success = append_shader_instruction(
			&builder,
			ACCEL_IR_ATOMIC_ADD_I32,
			ACCEL_IR_VOID,
			0,
			0);
	}
	if (!success) {
		accel_ir_kernel_destroy(producer);
		return false;
	}

	/* Build one later signed result load. */
	consumer = accel_ir_kernel_create("result_consumer", 2, 2, 0, 0);
	if (consumer == NULL) {
		accel_ir_kernel_destroy(producer);
		return false;
	}
	accel_ir_builder_init(&builder, consumer);
	success = append_shader_instruction(
		&builder,
		ACCEL_IR_LOAD_RESULT_I32,
		ACCEL_IR_I32,
		0,
		ACCEL_IR_VALUE_NONE);
	if (!success) {
		accel_ir_kernel_destroy(consumer);
		accel_ir_kernel_destroy(producer);
		return false;
	}

	/* Bind both kernels and one deterministic scalar-result entry. */
	memset(&program, 0, sizeof(program));
	memset(&scalar_result, 0, sizeof(scalar_result));
	memset(kernel, 0, sizeof(kernel));
	program.scalar_result_count = 1;
	program.scalar_result = &scalar_result;
	program.kernel_count = 2;
	program.kernel = kernel;
	scalar_result.name = "sum";
	scalar_result.value_type = ACCEL_IR_I32;
	scalar_result.producer_kernel = 0;
	scalar_result.gpu_consumer_mask = (uint32_t)1U << 1;
	kernel[0].kernel_index = 0;
	kernel[0].source_line = 1;
	kernel[0].ir = producer;
	kernel[1].kernel_index = 1;
	kernel[1].source_line = 2;
	kernel[1].ir = consumer;

	/* Initialize the same Vulkan 1.2/SPIR-V 1.5 assembler contract. */
	compiler = shaderc_compiler_initialize();
	if (compiler == NULL) {
		accel_ir_kernel_destroy(consumer);
		accel_ir_kernel_destroy(producer);
		return false;
	}
	options = shaderc_compile_options_initialize();
	if (options == NULL) {
		shaderc_compiler_release(compiler);
		accel_ir_kernel_destroy(consumer);
		accel_ir_kernel_destroy(producer);
		return false;
	}
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

	/* Assemble the producer and require one real OpAtomicIAdd. */
	status = accel_vulkan_shader_compile(
		compiler,
		options,
		&program,
		0,
		&spirv);
	success = status == ACCEL_COMPILE_APPLIED;
	if (success)
		success = spirv_has_opcode(
			&spirv,
			ACCEL_TEST_SPIRV_OP_ATOMIC_IADD);
	accel_vulkan_shader_cleanup(&spirv);

	/* Assemble the dependent load kernel through the same result binding. */
	if (success) {
		status = accel_vulkan_shader_compile(
			compiler,
			options,
			&program,
			1,
			&spirv);
		success = status == ACCEL_COMPILE_APPLIED;
		accel_vulkan_shader_cleanup(&spirv);
	}

	shaderc_compile_options_release(options);
	shaderc_compiler_release(compiler);
	accel_ir_kernel_destroy(consumer);
	accel_ir_kernel_destroy(producer);

	return success;
}

/* Require a failed create call to leave both ownership outputs clear. */
static bool
expect_failure(
	struct rt_env *env,
	struct accel_vulkan_api *api,
	const char *gpu_name)
{
	const struct accel_backend_ops *ops;
	void *backend_state;

	ops = (const struct accel_backend_ops *)(uintptr_t)1;
	backend_state = (void *)(uintptr_t)1;
	env->error_message[0] = '\0';
	if (accel_vulkan_create_with_api(
		env,
		gpu_name,
		api,
		&ops,
		&backend_state)) {
		return false;
	}
	if (ops != NULL || backend_state != NULL)
		return false;
	if (env->error_message[0] == '\0')
		return false;

	return true;
}
