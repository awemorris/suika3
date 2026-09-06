/* -*- coding: utf-8; tab-width: 8; indent-tabs-mode: t; -*- */

/*
 * Noct Programming Language
 * Copyright (c) 2025, 2026, Awe Morris
 */

/* Headless synchronous Vulkan compute backend. */

#include "../core/runtime.h"

#include <vulkan/vulkan.h>
#include <shaderc/shaderc.h>

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct accel_vk_resource;

struct accel_vk_runtime {
	VkInstance instance;
	VkPhysicalDevice physical_device;
	VkDevice device;
	VkQueue queue;
	uint32_t queue_family;
	VkCommandPool command_pool;
	char device_name[VK_MAX_PHYSICAL_DEVICE_NAME_SIZE];
	struct accel_vk_resource *resources;
	bool unavailable;
};

struct accel_vk_pipeline {
	VkShaderModule shader_module;
	VkDescriptorSetLayout descriptor_layout;
	VkPipelineLayout pipeline_layout;
	VkPipeline pipeline;
	uint32_t push_size;
};

struct accel_vk_buffer {
	VkBuffer buffer;
	VkDeviceMemory memory;
	void *mapped;
	VkDeviceSize size;
};

struct accel_vk_resource {
	struct accel_vk_buffer storage;
	struct accel_vk_resource *next;
};

static void accel_vk_destroy_runtime(struct accel_vk_runtime *vk);
static struct accel_vk_runtime *accel_vk_get_runtime(struct rt_env *env);
static bool accel_vk_make_pipeline(struct rt_env *env,
				   struct accel_vk_runtime *vk,
				   struct accel_kernel *kernel,
				   const char *display_name);
static uint32_t accel_vk_find_memory(struct accel_vk_runtime *vk,
				     uint32_t bits, VkMemoryPropertyFlags flags);
static bool accel_vk_make_buffer(struct accel_vk_runtime *vk,
				 VkDeviceSize size, struct accel_vk_buffer *buffer);
static void accel_vk_free_buffer(struct accel_vk_runtime *vk,
				 struct accel_vk_buffer *buffer);
static struct accel_vk_resource *accel_vk_get_resource(
	struct rt_env *env, struct rt_packed *packed);

static void
accel_vk_destroy_runtime(
	struct accel_vk_runtime *vk)
{
	struct accel_vk_resource *resource;
	struct accel_vk_resource *next;

	if (vk == NULL)
		return;
	if (vk->device != VK_NULL_HANDLE) {
		resource = vk->resources;
		while (resource != NULL) {
			next = resource->next;
			accel_vk_free_buffer(vk, &resource->storage);
			noct_free(resource);
			resource = next;
		}
		if (vk->command_pool != VK_NULL_HANDLE)
			vkDestroyCommandPool(vk->device, vk->command_pool, NULL);
		vkDestroyDevice(vk->device, NULL);
	}
	if (vk->instance != VK_NULL_HANDLE)
		vkDestroyInstance(vk->instance, NULL);
	noct_free(vk);
}

static bool
accel_vk_has_validation_layer(void)
{
	VkLayerProperties *properties;
	uint32_t count;
	uint32_t i;
	bool found;

	count = 0;
	if (vkEnumerateInstanceLayerProperties(&count, NULL) != VK_SUCCESS ||
	    count == 0)
		return false;
	properties = noct_malloc(sizeof(*properties) * count);
	if (properties == NULL)
		return false;
	if (vkEnumerateInstanceLayerProperties(&count, properties) != VK_SUCCESS) {
		noct_free(properties);
		return false;
	}
	found = false;
	for (i = 0; i < count; i++) {
		if (strcmp(properties[i].layerName,
			   "VK_LAYER_KHRONOS_validation") == 0) {
			found = true;
			break;
		}
	}
	noct_free(properties);
	return found;
}

bool
accel_vulkan_list_devices(void)
{
	VkApplicationInfo app;
	VkInstanceCreateInfo instance_info;
	VkInstance instance;
	VkPhysicalDevice *devices;
	VkPhysicalDeviceProperties properties;
	VkPhysicalDeviceMemoryProperties memory;
	VkQueueFamilyProperties *families;
	uint32_t device_count;
	uint32_t family_count;
	uint32_t i;
	uint32_t j;
	uint32_t heap;
	uint64_t device_memory;
	const char *type;
	bool compute;
	bool found;

	memset(&app, 0, sizeof(app));
	app.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	app.pApplicationName = "Noct";
	app.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	app.pEngineName = "Noct Accel";
	app.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	app.apiVersion = VK_API_VERSION_1_1;
	memset(&instance_info, 0, sizeof(instance_info));
	instance_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	instance_info.pApplicationInfo = &app;
	instance = VK_NULL_HANDLE;
	if (vkCreateInstance(&instance_info, NULL, &instance) != VK_SUCCESS)
		return false;
	device_count = 0;
	if (vkEnumeratePhysicalDevices(instance, &device_count, NULL) != VK_SUCCESS ||
	    device_count == 0) {
		vkDestroyInstance(instance, NULL);
		return false;
	}
	devices = noct_malloc(sizeof(*devices) * device_count);
	if (devices == NULL) {
		vkDestroyInstance(instance, NULL);
		return false;
	}
	if (vkEnumeratePhysicalDevices(instance, &device_count, devices) != VK_SUCCESS) {
		noct_free(devices);
		vkDestroyInstance(instance, NULL);
		return false;
	}
	found = false;
	for (i = 0; i < device_count; i++) {
		family_count = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(devices[i], &family_count, NULL);
		if (family_count == 0)
			continue;
		families = noct_malloc(sizeof(*families) * family_count);
		if (families == NULL)
			continue;
		vkGetPhysicalDeviceQueueFamilyProperties(devices[i], &family_count, families);
		compute = false;
		for (j = 0; j < family_count; j++) {
			if ((families[j].queueFlags & VK_QUEUE_COMPUTE_BIT) != 0) {
				compute = true;
				break;
			}
		}
		noct_free(families);
		if (!compute)
			continue;
		vkGetPhysicalDeviceProperties(devices[i], &properties);
		vkGetPhysicalDeviceMemoryProperties(devices[i], &memory);
		device_memory = 0;
		for (heap = 0; heap < memory.memoryHeapCount; heap++) {
			if ((memory.memoryHeaps[heap].flags &
			     VK_MEMORY_HEAP_DEVICE_LOCAL_BIT) != 0)
				device_memory += memory.memoryHeaps[heap].size;
		}
		switch (properties.deviceType) {
		case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
			type = "discrete";
			break;
		case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
			type = "integrated";
			break;
		case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:
			type = "virtual";
			break;
		case VK_PHYSICAL_DEVICE_TYPE_CPU:
			type = "software";
			break;
		default:
			type = "other";
			break;
		}
		printf("%u  %s  %llu MiB  %s\n", i, properties.deviceName,
		       (unsigned long long)(device_memory / (1024 * 1024)), type);
		found = true;
	}
	noct_free(devices);
	vkDestroyInstance(instance, NULL);
	return found;
}

static struct accel_vk_runtime *
accel_vk_get_runtime(
	struct rt_env *env)
{
	struct accel_vk_runtime *vk;
	VkApplicationInfo app;
	VkInstanceCreateInfo instance_info;
	VkPhysicalDevice *devices;
	VkPhysicalDeviceProperties properties;
	VkQueueFamilyProperties *families;
	VkDeviceQueueCreateInfo queue_info;
	VkDeviceCreateInfo device_info;
	VkCommandPoolCreateInfo pool_info;
	uint32_t device_count;
	uint32_t family_count;
	uint32_t i;
	uint32_t j;
	uint32_t best_score;
	uint32_t score;
	float priority;
	const char *validation_layer;

	if (env->vm->accel_runtime != NULL) {
		vk = env->vm->accel_runtime;
		return vk->unavailable ? NULL : vk;
	}
	vk = noct_calloc(1, sizeof(*vk));
	if (vk == NULL) {
		rt_out_of_memory(env);
		return NULL;
	}
	env->vm->accel_runtime = vk;
	memset(&app, 0, sizeof(app));
	app.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	app.pApplicationName = "Noct";
	app.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	app.pEngineName = "Noct Accel";
	app.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	app.apiVersion = VK_API_VERSION_1_1;
	memset(&instance_info, 0, sizeof(instance_info));
	instance_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	instance_info.pApplicationInfo = &app;
	validation_layer = "VK_LAYER_KHRONOS_validation";
	if (getenv("NOCT_VULKAN_VALIDATION") != NULL &&
	    accel_vk_has_validation_layer()) {
		instance_info.enabledLayerCount = 1;
		instance_info.ppEnabledLayerNames = &validation_layer;
	}
	if (vkCreateInstance(&instance_info, NULL, &vk->instance) != VK_SUCCESS)
		goto unavailable;

	device_count = 0;
	if (vkEnumeratePhysicalDevices(vk->instance, &device_count, NULL) != VK_SUCCESS ||
	    device_count == 0)
		goto unavailable;
	devices = noct_malloc(sizeof(*devices) * device_count);
	if (devices == NULL) {
		rt_out_of_memory(env);
		goto unavailable;
	}
	if (vkEnumeratePhysicalDevices(vk->instance, &device_count, devices) != VK_SUCCESS) {
		noct_free(devices);
		goto unavailable;
	}
	best_score = 0;
	for (i = 0; i < device_count; i++) {
		family_count = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(devices[i], &family_count, NULL);
		if (family_count == 0)
			continue;
		families = noct_malloc(sizeof(*families) * family_count);
		if (families == NULL)
			continue;
		vkGetPhysicalDeviceQueueFamilyProperties(devices[i], &family_count, families);
		for (j = 0; j < family_count; j++) {
			if ((families[j].queueFlags & VK_QUEUE_COMPUTE_BIT) == 0)
				continue;
			vkGetPhysicalDeviceProperties(devices[i], &properties);
			score = 1;
			if (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU)
				score = 2;
			if (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
				score = 3;
			if (score > best_score) {
				best_score = score;
				vk->physical_device = devices[i];
				vk->queue_family = j;
				strncpy(vk->device_name, properties.deviceName,
					sizeof(vk->device_name) - 1);
			}
			break;
		}
		noct_free(families);
	}
	noct_free(devices);
	if (vk->physical_device == VK_NULL_HANDLE)
		goto unavailable;
	priority = 1.0f;
	memset(&queue_info, 0, sizeof(queue_info));
	queue_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	queue_info.queueFamilyIndex = vk->queue_family;
	queue_info.queueCount = 1;
	queue_info.pQueuePriorities = &priority;
	memset(&device_info, 0, sizeof(device_info));
	device_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	device_info.queueCreateInfoCount = 1;
	device_info.pQueueCreateInfos = &queue_info;
	if (vkCreateDevice(vk->physical_device, &device_info, NULL,
			   &vk->device) != VK_SUCCESS)
		goto unavailable;
	vkGetDeviceQueue(vk->device, vk->queue_family, 0, &vk->queue);
	memset(&pool_info, 0, sizeof(pool_info));
	pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	pool_info.queueFamilyIndex = vk->queue_family;
	if (vkCreateCommandPool(vk->device, &pool_info, NULL,
				&vk->command_pool) != VK_SUCCESS)
		goto unavailable;
	if (env->vm->config.accel_info)
		fprintf(stderr, "ACCEL: Vulkan device: %s\n", vk->device_name);
	return vk;

unavailable:
	vk->unavailable = true;
	return NULL;
}

static bool
accel_vk_make_pipeline(
	struct rt_env *env,
	struct accel_vk_runtime *vk,
	struct accel_kernel *kernel,
	const char *display_name)
{
	struct accel_vk_pipeline *pipeline;
	shaderc_compiler_t compiler;
	shaderc_compile_options_t options;
	shaderc_compilation_result_t result;
	VkShaderModuleCreateInfo shader_info;
	VkDescriptorSetLayoutBinding bindings[NOCT_ARG_MAX];
	VkDescriptorSetLayoutCreateInfo descriptor_info;
	VkPushConstantRange push_range;
	VkPipelineLayoutCreateInfo layout_info;
	VkPipelineShaderStageCreateInfo stage_info;
	VkComputePipelineCreateInfo compute_info;
	uint32_t binding_count;
	uint32_t scalar_count;
	uint32_t i;
	const char *error;

	if (kernel->backend_data != NULL)
		return true;
	if (env->vm->config.accel_info)
		fprintf(stderr, "ACCEL: kernel %s: compiling Vulkan pipeline\n",
			display_name);
	compiler = shaderc_compiler_initialize();
	options = shaderc_compile_options_initialize();
	if (compiler == NULL || options == NULL)
		return false;
	shaderc_compile_options_set_target_env(options, shaderc_target_env_vulkan,
					       shaderc_env_version_vulkan_1_1);
	result = shaderc_compile_into_spv(compiler, kernel->glsl,
					 kernel->glsl_size,
					 shaderc_compute_shader,
					 kernel->name, "main", options);
	shaderc_compile_options_release(options);
	shaderc_compiler_release(compiler);
	if (result == NULL ||
	    shaderc_result_get_compilation_status(result) !=
		shaderc_compilation_status_success) {
		error = result == NULL ? "shaderc failed" :
			shaderc_result_get_error_message(result);
		if (env->vm->config.accel_info || getenv("NOCT_ACCEL_DEBUG") != NULL)
			fprintf(stderr, "ACCEL: kernel %s: shader compilation failed: %s\n",
				display_name, error);
		if (result != NULL) shaderc_result_release(result);
		return false;
	}
	pipeline = noct_calloc(1, sizeof(*pipeline));
	if (pipeline == NULL) {
		shaderc_result_release(result);
		return false;
	}
	memset(&shader_info, 0, sizeof(shader_info));
	shader_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	shader_info.codeSize = shaderc_result_get_length(result);
	shader_info.pCode = (const uint32_t *)shaderc_result_get_bytes(result);
	if (vkCreateShaderModule(vk->device, &shader_info, NULL,
				 &pipeline->shader_module) != VK_SUCCESS) {
		shaderc_result_release(result);
		noct_free(pipeline);
		return false;
	}
	shaderc_result_release(result);
	binding_count = 0;
	scalar_count = 0;
	memset(bindings, 0, sizeof(bindings));
	for (i = 0; i < kernel->param_count; i++) {
		if (kernel->param_transport[i] == ACCEL_TRANSPORT_SCALAR) {
			scalar_count++;
			continue;
		}
		bindings[binding_count].binding = i;
		bindings[binding_count].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		bindings[binding_count].descriptorCount = 1;
		bindings[binding_count].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
		binding_count++;
	}
	memset(&descriptor_info, 0, sizeof(descriptor_info));
	descriptor_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	descriptor_info.bindingCount = binding_count;
	descriptor_info.pBindings = bindings;
	if (vkCreateDescriptorSetLayout(vk->device, &descriptor_info, NULL,
					&pipeline->descriptor_layout) != VK_SUCCESS)
		goto failed;
	pipeline->push_size = (1 + scalar_count) * 4;
	if (pipeline->push_size > 128)
		goto failed;
	memset(&push_range, 0, sizeof(push_range));
	push_range.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
	push_range.size = pipeline->push_size;
	memset(&layout_info, 0, sizeof(layout_info));
	layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	layout_info.setLayoutCount = 1;
	layout_info.pSetLayouts = &pipeline->descriptor_layout;
	layout_info.pushConstantRangeCount = 1;
	layout_info.pPushConstantRanges = &push_range;
	if (vkCreatePipelineLayout(vk->device, &layout_info, NULL,
				   &pipeline->pipeline_layout) != VK_SUCCESS)
		goto failed;
	memset(&stage_info, 0, sizeof(stage_info));
	stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	stage_info.stage = VK_SHADER_STAGE_COMPUTE_BIT;
	stage_info.module = pipeline->shader_module;
	stage_info.pName = "main";
	memset(&compute_info, 0, sizeof(compute_info));
	compute_info.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
	compute_info.stage = stage_info;
	compute_info.layout = pipeline->pipeline_layout;
	if (vkCreateComputePipelines(vk->device, VK_NULL_HANDLE, 1,
				     &compute_info, NULL,
				     &pipeline->pipeline) != VK_SUCCESS)
		goto failed;
	kernel->backend_data = pipeline;
	return true;

failed:
	if (pipeline->pipeline_layout != VK_NULL_HANDLE)
		vkDestroyPipelineLayout(vk->device, pipeline->pipeline_layout, NULL);
	if (pipeline->descriptor_layout != VK_NULL_HANDLE)
		vkDestroyDescriptorSetLayout(vk->device, pipeline->descriptor_layout, NULL);
	if (pipeline->shader_module != VK_NULL_HANDLE)
		vkDestroyShaderModule(vk->device, pipeline->shader_module, NULL);
	noct_free(pipeline);
	return false;
}

static uint32_t
accel_vk_find_memory(
	struct accel_vk_runtime *vk,
	uint32_t bits,
	VkMemoryPropertyFlags flags)
{
	VkPhysicalDeviceMemoryProperties properties;
	uint32_t i;

	vkGetPhysicalDeviceMemoryProperties(vk->physical_device, &properties);
	for (i = 0; i < properties.memoryTypeCount; i++) {
		if ((bits & (1U << i)) != 0 &&
		    (properties.memoryTypes[i].propertyFlags & flags) == flags)
			return i;
	}
	return UINT32_MAX;
}

static bool
accel_vk_make_buffer(
	struct accel_vk_runtime *vk,
	VkDeviceSize size,
	struct accel_vk_buffer *buffer)
{
	VkBufferCreateInfo buffer_info;
	VkMemoryRequirements requirements;
	VkMemoryAllocateInfo alloc_info;
	uint32_t memory_type;

	memset(buffer, 0, sizeof(*buffer));
	buffer->size = size;
	memset(&buffer_info, 0, sizeof(buffer_info));
	buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	buffer_info.size = size;
	buffer_info.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
		VK_BUFFER_USAGE_TRANSFER_DST_BIT;
	buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	if (vkCreateBuffer(vk->device, &buffer_info, NULL,
			   &buffer->buffer) != VK_SUCCESS)
		return false;
	vkGetBufferMemoryRequirements(vk->device, buffer->buffer, &requirements);
	memory_type = accel_vk_find_memory(vk, requirements.memoryTypeBits,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
		VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	if (memory_type == UINT32_MAX)
		goto failed;
	memset(&alloc_info, 0, sizeof(alloc_info));
	alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	alloc_info.allocationSize = requirements.size;
	alloc_info.memoryTypeIndex = memory_type;
	if (vkAllocateMemory(vk->device, &alloc_info, NULL,
			     &buffer->memory) != VK_SUCCESS)
		goto failed;
	if (vkBindBufferMemory(vk->device, buffer->buffer,
			       buffer->memory, 0) != VK_SUCCESS)
		goto failed;
	if (vkMapMemory(vk->device, buffer->memory, 0, size, 0,
			&buffer->mapped) != VK_SUCCESS)
		goto failed;
	return true;

failed:
	accel_vk_free_buffer(vk, buffer);
	return false;
}

static void
accel_vk_free_buffer(
	struct accel_vk_runtime *vk,
	struct accel_vk_buffer *buffer)
{
	if (buffer->mapped != NULL)
		vkUnmapMemory(vk->device, buffer->memory);
	if (buffer->buffer != VK_NULL_HANDLE)
		vkDestroyBuffer(vk->device, buffer->buffer, NULL);
	if (buffer->memory != VK_NULL_HANDLE)
		vkFreeMemory(vk->device, buffer->memory, NULL);
	memset(buffer, 0, sizeof(*buffer));
}

static size_t
accel_vk_element_width(
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

static struct accel_vk_resource *
accel_vk_get_resource(
	struct rt_env *env,
	struct rt_packed *packed)
{
	struct accel_vk_runtime *vk;
	struct accel_vk_resource *resource;
	size_t width;
	size_t size;

	if (!packed->is_accel_resource)
		return NULL;
	if (packed->accel_backend_data != NULL)
		return packed->accel_backend_data;
	vk = accel_vk_get_runtime(env);
	if (vk == NULL)
		return NULL;
	width = accel_vk_element_width(packed->type);
	if (width == 0 || packed->elem_size > SIZE_MAX / width)
		return NULL;
	size = packed->elem_size * width;
	resource = noct_calloc(1, sizeof(*resource));
	if (resource == NULL) {
		rt_out_of_memory(env);
		return NULL;
	}
	if (!accel_vk_make_buffer(vk, (VkDeviceSize)size,
				  &resource->storage)) {
		noct_free(resource);
		return NULL;
	}
	memcpy(resource->storage.mapped, packed->packed_buffer, size);
	resource->next = vk->resources;
	vk->resources = resource;
	packed->accel_backend_data = resource;
	if (env->vm->config.accel_info)
		fprintf(stderr,
			"ACCEL: Vulkan persistent resource allocated (%lu bytes)\n",
			(unsigned long)size);
	return resource;
}

int
accel_vulkan_copy_to(
	struct rt_env *env,
	struct rt_packed *packed,
	size_t offset,
	size_t size)
{
	struct accel_vk_resource *resource;

	resource = accel_vk_get_resource(env, packed);
	if (resource == NULL)
		return ACCEL_DISPATCH_FALLBACK;
	if (offset > resource->storage.size ||
	    size > resource->storage.size - offset) {
		rt_error(env, "Vulkan accelerator upload range is out-of-bounds.");
		return ACCEL_DISPATCH_ERROR;
	}
	if (size != 0)
		memcpy((char *)resource->storage.mapped + offset,
		       (char *)packed->packed_buffer + offset, size);
	return ACCEL_DISPATCH_OK;
}

int
accel_vulkan_copy_from(
	struct rt_env *env,
	struct rt_packed *packed,
	size_t offset,
	size_t size)
{
	struct accel_vk_resource *resource;

	resource = accel_vk_get_resource(env, packed);
	if (resource == NULL)
		return ACCEL_DISPATCH_FALLBACK;
	if (offset > resource->storage.size ||
	    size > resource->storage.size - offset) {
		rt_error(env, "Vulkan accelerator download range is out-of-bounds.");
		return ACCEL_DISPATCH_ERROR;
	}
	if (size != 0)
		memcpy((char *)packed->packed_buffer + offset,
		       (char *)resource->storage.mapped + offset, size);
	return ACCEL_DISPATCH_OK;
}

static uint32_t
accel_vk_descriptor_count(
	const struct accel_kernel *kernel)
{
	uint32_t count;
	uint32_t i;

	count = 0;
	for (i = 0; i < kernel->param_count; i++)
		if (kernel->param_transport[i] != ACCEL_TRANSPORT_SCALAR)
			count++;
	return count;
}

static bool
accel_vk_record_dispatch(
	struct accel_vk_runtime *vk,
	VkDescriptorPool descriptor_pool,
	VkCommandBuffer command,
	struct accel_kernel *kernel,
	struct accel_vk_pipeline *pipeline,
	struct accel_vk_buffer **binding,
	const uint32_t *push,
	uint32_t group_count)
{
	VkDescriptorSetAllocateInfo set_info;
	VkDescriptorSet descriptor_set;
	VkDescriptorBufferInfo buffer_info[NOCT_ARG_MAX];
	VkWriteDescriptorSet writes[NOCT_ARG_MAX];
	uint32_t descriptor_count;
	uint32_t i;

	descriptor_count = 0;
	memset(buffer_info, 0, sizeof(buffer_info));
	memset(writes, 0, sizeof(writes));
	for (i = 0; i < kernel->param_count; i++) {
		if (kernel->param_transport[i] == ACCEL_TRANSPORT_SCALAR)
			continue;
		if (binding[i] == NULL)
			return false;
		buffer_info[descriptor_count].buffer = binding[i]->buffer;
		buffer_info[descriptor_count].range = binding[i]->size;
		writes[descriptor_count].sType =
			VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		writes[descriptor_count].dstBinding = i;
		writes[descriptor_count].descriptorCount = 1;
		writes[descriptor_count].descriptorType =
			VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		writes[descriptor_count].pBufferInfo =
			&buffer_info[descriptor_count];
		descriptor_count++;
	}
	memset(&set_info, 0, sizeof(set_info));
	set_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	set_info.descriptorPool = descriptor_pool;
	set_info.descriptorSetCount = 1;
	set_info.pSetLayouts = &pipeline->descriptor_layout;
	if (vkAllocateDescriptorSets(vk->device, &set_info,
				     &descriptor_set) != VK_SUCCESS)
		return false;
	for (i = 0; i < descriptor_count; i++)
		writes[i].dstSet = descriptor_set;
	vkUpdateDescriptorSets(vk->device, descriptor_count, writes, 0, NULL);
	vkCmdBindPipeline(command, VK_PIPELINE_BIND_POINT_COMPUTE,
			  pipeline->pipeline);
	vkCmdBindDescriptorSets(command, VK_PIPELINE_BIND_POINT_COMPUTE,
				pipeline->pipeline_layout, 0, 1,
				&descriptor_set, 0, NULL);
	vkCmdPushConstants(command, pipeline->pipeline_layout,
			   VK_SHADER_STAGE_COMPUTE_BIT, 0,
			   pipeline->push_size, push);
	vkCmdDispatch(command, group_count, 1, 1);
	return true;
}

static void
accel_vk_shader_barrier(
	VkCommandBuffer command)
{
	VkMemoryBarrier barrier;

	memset(&barrier, 0, sizeof(barrier));
	barrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
	barrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
	barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT |
		VK_ACCESS_SHADER_WRITE_BIT;
	vkCmdPipelineBarrier(command, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
			     VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0,
			     1, &barrier, 0, NULL, 0, NULL);
}

static int
accel_vk_dispatch_program(
	struct rt_env *env,
	struct rt_func *func,
	uint32_t arg_count,
	struct rt_value *arg)
{
	struct accel_program *program;
	struct accel_vk_runtime *vk;
	struct accel_vk_buffer buffers[ACCEL_PROGRAM_MAX_BUFFERS];
	struct accel_vk_resource *resources[ACCEL_PROGRAM_MAX_BUFFERS];
	struct accel_vk_buffer *binding[NOCT_ARG_MAX];
	int64_t scalar_arg[NOCT_ARG_MAX];
	int64_t buffer_length[ACCEL_PROGRAM_MAX_BUFFERS];
	uint32_t trip_count[ACCEL_PROGRAM_MAX_STEPS];
	struct accel_program_step *step;
	struct accel_kernel *kernel;
	struct accel_vk_pipeline *pipeline;
	VkPhysicalDeviceProperties properties;
	VkDescriptorPoolSize pool_size;
	VkDescriptorPoolCreateInfo pool_info;
	VkDescriptorPool descriptor_pool;
	VkCommandBufferAllocateInfo command_alloc;
	VkCommandBuffer command;
	VkCommandBufferBeginInfo begin_info;
	VkMemoryBarrier barrier;
	VkFenceCreateInfo fence_info;
	VkFence fence;
	VkSubmitInfo submit_info;
	uint32_t push[NOCT_ARG_MAX + 1];
	uint32_t dispatch_count;
	uint32_t descriptor_count;
	uint32_t push_count;
	uint32_t group_count;
	uint32_t next_group_count;
	uint32_t current_buffer;
	uint32_t next_buffer;
	uint32_t bound_buffer;
	uint32_t i;
	uint32_t j;
	uint32_t k;
	int64_t evaluated;
	size_t byte_size;
	char validation_error[128];
	bool submitted;
	int result;

	program = func->accel_program;
	if (program == NULL || arg_count != program->outer_param_count)
		return ACCEL_DISPATCH_FALLBACK;
	if (!accel_program_validate(program, validation_error,
				    sizeof(validation_error))) {
		if (env->vm->config.accel_info)
			fprintf(stderr, "ACCEL: Vulkan program %s rejected: %s\n",
				func->name, validation_error);
		return ACCEL_DISPATCH_FALLBACK;
	}
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
						 program->buffer[i].length_expr,
						 arg_count, scalar_arg,
						 buffer_length, &evaluated))
				return ACCEL_DISPATCH_FALLBACK;
			buffer_length[i] = evaluated;
		}
		if (buffer_length[i] < 0 ||
		    (uint64_t)buffer_length[i] > SIZE_MAX /
			(size_t)program->buffer[i].element_width)
			return ACCEL_DISPATCH_FALLBACK;
	}
	vk = accel_vk_get_runtime(env);
	if (vk == NULL)
		return ACCEL_DISPATCH_FALLBACK;
	vkGetPhysicalDeviceProperties(vk->physical_device, &properties);
	dispatch_count = 0;
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
		if (trip_count[i] == 0)
			continue;
		group_count = (trip_count[i] + step->block_size - 1U) /
			step->block_size;
		if (group_count > properties.limits.maxComputeWorkGroupCount[0]) {
			if (step->kind != ACCEL_STEP_DOALL_DISPATCH)
				return ACCEL_DISPATCH_FALLBACK;
			group_count = properties.limits.maxComputeWorkGroupCount[0];
		}
		kernel = program->kernel[step->kernel];
		if (!accel_vk_make_pipeline(env, vk, kernel, kernel->name))
			return ACCEL_DISPATCH_FALLBACK;
		dispatch_count++;
		descriptor_count += accel_vk_descriptor_count(kernel);
		if (step->kind != ACCEL_STEP_DOSUM_REDUCTION)
			continue;
		kernel = program->kernel[step->fold_kernel];
		if (!accel_vk_make_pipeline(env, vk, kernel, kernel->name))
			return ACCEL_DISPATCH_FALLBACK;
		while (group_count > 1) {
			group_count = (group_count + step->block_size - 1U) /
				step->block_size;
			dispatch_count++;
			descriptor_count += accel_vk_descriptor_count(kernel);
		}
	}
	memset(buffers, 0, sizeof(buffers));
	memset(resources, 0, sizeof(resources));
	descriptor_pool = VK_NULL_HANDLE;
	command = VK_NULL_HANDLE;
	fence = VK_NULL_HANDLE;
	submitted = false;
	result = ACCEL_DISPATCH_FALLBACK;
	for (i = 0; i < program->buffer_count; i++) {
		struct accel_buffer_desc *desc;
		desc = &program->buffer[i];
		byte_size = (size_t)buffer_length[i] *
			(size_t)desc->element_width;
		if (desc->origin == ACCEL_BUFFER_DEVICE_PTR) {
			resources[i] = accel_vk_get_resource(
				env, arg[desc->outer_param].val.packed);
			if (resources[i] == NULL ||
			    resources[i]->storage.size != (VkDeviceSize)byte_size)
				goto cleanup;
			continue;
		}
		if (!accel_vk_make_buffer(vk, byte_size != 0 ? byte_size : 4,
					  &buffers[i]))
			goto cleanup;
		if (desc->upload && byte_size != 0)
			memcpy(buffers[i].mapped,
			       arg[desc->outer_param].val.packed->packed_buffer,
			       byte_size);
	}
	if (dispatch_count != 0) {
		memset(&pool_size, 0, sizeof(pool_size));
		pool_size.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		pool_size.descriptorCount = descriptor_count;
		memset(&pool_info, 0, sizeof(pool_info));
		pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		pool_info.maxSets = dispatch_count;
		pool_info.poolSizeCount = 1;
		pool_info.pPoolSizes = &pool_size;
		if (vkCreateDescriptorPool(vk->device, &pool_info, NULL,
					   &descriptor_pool) != VK_SUCCESS)
			goto cleanup;
	}
	memset(&command_alloc, 0, sizeof(command_alloc));
	command_alloc.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	command_alloc.commandPool = vk->command_pool;
	command_alloc.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	command_alloc.commandBufferCount = 1;
	if (vkAllocateCommandBuffers(vk->device, &command_alloc,
				     &command) != VK_SUCCESS)
		goto cleanup;
	memset(&begin_info, 0, sizeof(begin_info));
	begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	if (vkBeginCommandBuffer(command, &begin_info) != VK_SUCCESS)
		goto cleanup;
	memset(&barrier, 0, sizeof(barrier));
	barrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
	barrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
	barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT |
		VK_ACCESS_SHADER_WRITE_BIT;
	vkCmdPipelineBarrier(command, VK_PIPELINE_STAGE_HOST_BIT,
			     VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0,
			     1, &barrier, 0, NULL, 0, NULL);
	for (i = 0; i < program->step_count; i++) {
		step = &program->step[i];
		if (trip_count[i] == 0) {
			if (step->kind == ACCEL_STEP_DOSUM_REDUCTION) {
				struct accel_vk_buffer *zero_buffer;
				zero_buffer = resources[step->result_buffer] != NULL ?
					&resources[step->result_buffer]->storage :
					&buffers[step->result_buffer];
				memset(&barrier, 0, sizeof(barrier));
				barrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
				barrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
				barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
				vkCmdPipelineBarrier(command,
					VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
					VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
					1, &barrier, 0, NULL, 0, NULL);
				vkCmdFillBuffer(command,
					zero_buffer->buffer,
					0, 4, 0);
				memset(&barrier, 0, sizeof(barrier));
				barrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
				barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
				barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
				vkCmdPipelineBarrier(command,
					VK_PIPELINE_STAGE_TRANSFER_BIT,
					VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0,
					1, &barrier, 0, NULL, 0, NULL);
			}
			continue;
		}
		kernel = program->kernel[step->kernel];
		pipeline = kernel->backend_data;
		group_count = (trip_count[i] + step->block_size - 1U) /
			step->block_size;
		if (group_count > properties.limits.maxComputeWorkGroupCount[0])
			group_count = properties.limits.maxComputeWorkGroupCount[0];
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
		push_count = 0;
		push[push_count++] = trip_count[i];
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
				memcpy(&push[push_count], &arg[outer].val.f, 4);
			} else {
				if (!accel_expr_evaluate(program,
						 step->binding[k].value,
						 arg_count, scalar_arg,
						 buffer_length, &evaluated) ||
				    evaluated < INT_MIN || evaluated > INT_MAX)
					goto cleanup;
				push[push_count] = (uint32_t)evaluated;
			}
			push_count++;
		}
		if (!accel_vk_record_dispatch(vk, descriptor_pool, command,
					      kernel, pipeline, binding,
					      push, group_count))
			goto cleanup;
		accel_vk_shader_barrier(command);
		if (step->kind != ACCEL_STEP_DOSUM_REDUCTION || group_count == 1)
			continue;
		current_buffer = (uint32_t)step->scratch_buffer;
		kernel = program->kernel[step->fold_kernel];
		pipeline = kernel->backend_data;
		while (group_count > 1) {
			next_group_count = (group_count + step->block_size - 1U) /
				step->block_size;
			if (next_group_count == 1)
				next_buffer = (uint32_t)step->result_buffer;
			else if (current_buffer == (uint32_t)step->scratch_buffer)
				next_buffer = (uint32_t)step->scratch_buffer2;
			else
				next_buffer = (uint32_t)step->scratch_buffer;
			memset(binding, 0, sizeof(binding));
			binding[0] = resources[current_buffer] != NULL ?
				&resources[current_buffer]->storage : &buffers[current_buffer];
			binding[1] = resources[next_buffer] != NULL ?
				&resources[next_buffer]->storage : &buffers[next_buffer];
			push[0] = group_count;
			push[1] = group_count;
			if (!accel_vk_record_dispatch(vk, descriptor_pool, command,
						      kernel, pipeline, binding,
						      push, next_group_count))
				goto cleanup;
			accel_vk_shader_barrier(command);
			current_buffer = next_buffer;
			group_count = next_group_count;
		}
	}
	memset(&barrier, 0, sizeof(barrier));
	barrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
	barrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT |
		VK_ACCESS_TRANSFER_WRITE_BIT;
	barrier.dstAccessMask = VK_ACCESS_HOST_READ_BIT;
	vkCmdPipelineBarrier(command,
		VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT |
		VK_PIPELINE_STAGE_TRANSFER_BIT,
		VK_PIPELINE_STAGE_HOST_BIT, 0, 1, &barrier, 0, NULL, 0, NULL);
	if (vkEndCommandBuffer(command) != VK_SUCCESS)
		goto cleanup;
	memset(&fence_info, 0, sizeof(fence_info));
	fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	if (vkCreateFence(vk->device, &fence_info, NULL, &fence) != VK_SUCCESS)
		goto cleanup;
	memset(&submit_info, 0, sizeof(submit_info));
	submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submit_info.commandBufferCount = 1;
	submit_info.pCommandBuffers = &command;
	if (vkQueueSubmit(vk->queue, 1, &submit_info, fence) != VK_SUCCESS) {
		rt_error(env, "Vulkan queue submission failed for program '%s'.",
			 func->name);
		result = ACCEL_DISPATCH_ERROR;
		goto cleanup;
	}
	submitted = true;
	if (vkWaitForFences(vk->device, 1, &fence, VK_TRUE,
			    UINT64_MAX) != VK_SUCCESS) {
		rt_error(env, "Vulkan execution failed for program '%s'.",
			 func->name);
		result = ACCEL_DISPATCH_ERROR;
		goto cleanup;
	}
	for (i = 0; i < program->buffer_count; i++) {
		struct accel_buffer_desc *desc;
		desc = &program->buffer[i];
		if (!desc->download)
			continue;
		byte_size = (size_t)buffer_length[i] *
			(size_t)desc->element_width;
		if (byte_size != 0)
			memcpy(arg[desc->outer_param].val.packed->packed_buffer,
			       buffers[i].mapped, byte_size);
	}
	result = ACCEL_DISPATCH_OK;

cleanup:
	if (submitted && result == ACCEL_DISPATCH_FALLBACK)
		result = ACCEL_DISPATCH_ERROR;
	if (fence != VK_NULL_HANDLE)
		vkDestroyFence(vk->device, fence, NULL);
	if (command != VK_NULL_HANDLE)
		vkFreeCommandBuffers(vk->device, vk->command_pool, 1, &command);
	if (descriptor_pool != VK_NULL_HANDLE)
		vkDestroyDescriptorPool(vk->device, descriptor_pool, NULL);
	for (i = 0; i < program->buffer_count; i++)
		if (resources[i] == NULL)
			accel_vk_free_buffer(vk, &buffers[i]);
	return result;
}

int
accel_vulkan_dispatch(
	struct rt_env *env,
	struct rt_func *func,
	uint32_t arg_count,
	struct rt_value *arg)
{
	struct accel_kernel *kernel;
	struct accel_vk_runtime *vk;
	struct accel_vk_pipeline *pipeline;
	struct accel_vk_buffer buffers[NOCT_ARG_MAX];
	struct accel_vk_buffer *binding[NOCT_ARG_MAX];
	struct accel_vk_resource *resource;
	VkDescriptorPoolSize pool_size;
	VkDescriptorPoolCreateInfo pool_info;
	VkDescriptorPool descriptor_pool;
	VkDescriptorSetAllocateInfo set_info;
	VkDescriptorSet descriptor_set;
	VkDescriptorBufferInfo buffer_info[NOCT_ARG_MAX];
	VkWriteDescriptorSet writes[NOCT_ARG_MAX];
	VkCommandBufferAllocateInfo command_alloc;
	VkCommandBuffer command;
	VkCommandBufferBeginInfo begin_info;
	VkMemoryBarrier before_barrier;
	VkMemoryBarrier after_barrier;
	VkPhysicalDeviceProperties properties;
	VkFenceCreateInfo fence_info;
	VkFence fence;
	VkSubmitInfo submit_info;
	uint32_t push[NOCT_ARG_MAX + 1];
	uint32_t descriptor_count;
	uint32_t push_count;
	uint32_t count;
	uint32_t group_count;
	uint32_t i;
	uint32_t j;
	size_t packed_size;
	size_t byte_size;
	bool submitted;
	int result;

	if (func->accel_program != NULL)
		return accel_vk_dispatch_program(env, func, arg_count, arg);
	kernel = func->accel_kernel;
	if (kernel == NULL || !kernel->eligible || arg_count != kernel->param_count)
		return ACCEL_DISPATCH_FALLBACK;
	count = 0;
	if (kernel->dispatch_param >= 0) {
		if (arg[kernel->dispatch_param].type != NOCT_VALUE_INT ||
		    arg[kernel->dispatch_param].val.i < 0)
			return ACCEL_DISPATCH_FALLBACK;
		count = (uint32_t)arg[kernel->dispatch_param].val.i;
	}
	for (i = 0; i < arg_count; i++) {
		if (kernel->param_transport[i] == ACCEL_TRANSPORT_SCALAR) {
			if (arg[i].type != kernel->param_type[i])
				return ACCEL_DISPATCH_FALLBACK;
			continue;
		}
		if (arg[i].type != NOCT_VALUE_PACKED ||
		    arg[i].val.packed->type != kernel->param_packed_type[i])
			return ACCEL_DISPATCH_FALLBACK;
		packed_size = arg[i].val.packed->elem_size;
		if ((size_t)count > packed_size)
			return ACCEL_DISPATCH_FALLBACK;
		for (j = 0; j < i; j++) {
			if (kernel->param_transport[j] != ACCEL_TRANSPORT_SCALAR &&
			    arg[j].val.packed == arg[i].val.packed)
				return ACCEL_DISPATCH_FALLBACK;
		}
	}
	if (count == 0)
		return ACCEL_DISPATCH_OK;
	vk = accel_vk_get_runtime(env);
	if (vk == NULL)
		return ACCEL_DISPATCH_FALLBACK;
	if (!accel_vk_make_pipeline(env, vk, kernel, func->name))
		return ACCEL_DISPATCH_FALLBACK;
	pipeline = kernel->backend_data;
	memset(buffers, 0, sizeof(buffers));
	memset(binding, 0, sizeof(binding));
	descriptor_pool = VK_NULL_HANDLE;
	command = VK_NULL_HANDLE;
	fence = VK_NULL_HANDLE;
	submitted = false;
	result = ACCEL_DISPATCH_FALLBACK;
	for (i = 0; i < arg_count; i++) {
		if (kernel->param_transport[i] == ACCEL_TRANSPORT_SCALAR) continue;
		if (kernel->param_transport[i] == ACCEL_TRANSPORT_DEVICE_PTR) {
			resource = accel_vk_get_resource(env, arg[i].val.packed);
			if (resource == NULL)
				goto cleanup;
			binding[i] = &resource->storage;
			continue;
		}
		byte_size = arg[i].val.packed->elem_size * 4;
		if (!accel_vk_make_buffer(vk, (VkDeviceSize)byte_size, &buffers[i]))
			goto cleanup;
		binding[i] = &buffers[i];
		if (kernel->param_transport[i] == ACCEL_TRANSPORT_COPY_IN)
			memcpy(buffers[i].mapped, arg[i].val.packed->packed_buffer, byte_size);
	}
	descriptor_count = 0;
	memset(buffer_info, 0, sizeof(buffer_info));
	memset(writes, 0, sizeof(writes));
	for (i = 0; i < arg_count; i++) {
		if (kernel->param_transport[i] == ACCEL_TRANSPORT_SCALAR) continue;
		buffer_info[descriptor_count].buffer = binding[i]->buffer;
		buffer_info[descriptor_count].range = binding[i]->size;
		writes[descriptor_count].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		writes[descriptor_count].dstBinding = i;
		writes[descriptor_count].descriptorCount = 1;
		writes[descriptor_count].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		writes[descriptor_count].pBufferInfo = &buffer_info[descriptor_count];
		descriptor_count++;
	}
	memset(&pool_size, 0, sizeof(pool_size));
	pool_size.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	pool_size.descriptorCount = descriptor_count;
	memset(&pool_info, 0, sizeof(pool_info));
	pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	pool_info.maxSets = 1;
	pool_info.poolSizeCount = 1;
	pool_info.pPoolSizes = &pool_size;
	if (vkCreateDescriptorPool(vk->device, &pool_info, NULL,
				   &descriptor_pool) != VK_SUCCESS)
		goto cleanup;
	memset(&set_info, 0, sizeof(set_info));
	set_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	set_info.descriptorPool = descriptor_pool;
	set_info.descriptorSetCount = 1;
	set_info.pSetLayouts = &pipeline->descriptor_layout;
	if (vkAllocateDescriptorSets(vk->device, &set_info,
				     &descriptor_set) != VK_SUCCESS)
		goto cleanup;
	for (i = 0; i < descriptor_count; i++)
		writes[i].dstSet = descriptor_set;
	vkUpdateDescriptorSets(vk->device, descriptor_count, writes, 0, NULL);
	push_count = 0;
	push[push_count++] = count;
	for (i = 0; i < arg_count; i++) {
		if (kernel->param_transport[i] != ACCEL_TRANSPORT_SCALAR) continue;
		if (arg[i].type == NOCT_VALUE_FLOAT)
			memcpy(&push[push_count], &arg[i].val.f, 4);
		else
			memcpy(&push[push_count], &arg[i].val.i, 4);
		push_count++;
	}
	memset(&command_alloc, 0, sizeof(command_alloc));
	command_alloc.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	command_alloc.commandPool = vk->command_pool;
	command_alloc.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	command_alloc.commandBufferCount = 1;
	if (vkAllocateCommandBuffers(vk->device, &command_alloc, &command) != VK_SUCCESS)
		goto cleanup;
	memset(&begin_info, 0, sizeof(begin_info));
	begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	if (vkBeginCommandBuffer(command, &begin_info) != VK_SUCCESS)
		goto cleanup;
	memset(&before_barrier, 0, sizeof(before_barrier));
	before_barrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
	before_barrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
	before_barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
	vkCmdPipelineBarrier(command, VK_PIPELINE_STAGE_HOST_BIT,
			     VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0,
			     1, &before_barrier, 0, NULL, 0, NULL);
	vkCmdBindPipeline(command, VK_PIPELINE_BIND_POINT_COMPUTE,
			  pipeline->pipeline);
	vkCmdBindDescriptorSets(command, VK_PIPELINE_BIND_POINT_COMPUTE,
				pipeline->pipeline_layout, 0, 1,
				&descriptor_set, 0, NULL);
	vkCmdPushConstants(command, pipeline->pipeline_layout,
			   VK_SHADER_STAGE_COMPUTE_BIT, 0,
			   pipeline->push_size, push);
	group_count = (count + 63U) / 64U;
	vkGetPhysicalDeviceProperties(vk->physical_device, &properties);
	if (group_count > properties.limits.maxComputeWorkGroupCount[0])
		group_count = properties.limits.maxComputeWorkGroupCount[0];
	vkCmdDispatch(command, group_count, 1, 1);
	memset(&after_barrier, 0, sizeof(after_barrier));
	after_barrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
	after_barrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
	after_barrier.dstAccessMask = VK_ACCESS_HOST_READ_BIT;
	vkCmdPipelineBarrier(command, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
			     VK_PIPELINE_STAGE_HOST_BIT, 0,
			     1, &after_barrier, 0, NULL, 0, NULL);
	if (vkEndCommandBuffer(command) != VK_SUCCESS)
		goto cleanup;
	memset(&fence_info, 0, sizeof(fence_info));
	fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	if (vkCreateFence(vk->device, &fence_info, NULL, &fence) != VK_SUCCESS)
		goto cleanup;
	memset(&submit_info, 0, sizeof(submit_info));
	submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submit_info.commandBufferCount = 1;
	submit_info.pCommandBuffers = &command;
	if (vkQueueSubmit(vk->queue, 1, &submit_info, fence) != VK_SUCCESS) {
		rt_error(env, "Vulkan queue submission failed for kernel '%s'.", func->name);
		result = ACCEL_DISPATCH_ERROR;
		goto cleanup;
	}
	submitted = true;
	if (vkWaitForFences(vk->device, 1, &fence, VK_TRUE, UINT64_MAX) != VK_SUCCESS) {
		rt_error(env, "Vulkan execution failed for kernel '%s'.", func->name);
		result = ACCEL_DISPATCH_ERROR;
		goto cleanup;
	}
	for (i = 0; i < arg_count; i++) {
		if (kernel->param_transport[i] == ACCEL_TRANSPORT_COPY_OUT)
			memcpy(arg[i].val.packed->packed_buffer, buffers[i].mapped,
			       (size_t)count * 4);
	}
	result = ACCEL_DISPATCH_OK;

cleanup:
	if (submitted && result == ACCEL_DISPATCH_FALLBACK)
		result = ACCEL_DISPATCH_ERROR;
	if (fence != VK_NULL_HANDLE)
		vkDestroyFence(vk->device, fence, NULL);
	if (command != VK_NULL_HANDLE)
		vkFreeCommandBuffers(vk->device, vk->command_pool, 1, &command);
	if (descriptor_pool != VK_NULL_HANDLE)
		vkDestroyDescriptorPool(vk->device, descriptor_pool, NULL);
	for (i = 0; i < arg_count; i++)
		accel_vk_free_buffer(vk, &buffers[i]);
	return result;
}

void
accel_vulkan_cleanup(
	struct rt_vm *vm)
{
	struct accel_vk_runtime *vk;
	struct rt_func *func;
	struct accel_vk_pipeline *pipeline;
	uint32_t i;

	vk = vm->accel_runtime;
	if (vk == NULL)
		return;
	if (vk->device != VK_NULL_HANDLE) {
		vkDeviceWaitIdle(vk->device);
		func = vm->func_list;
		while (func != NULL) {
			if (func->accel_kernel != NULL &&
			    func->accel_kernel->backend_data != NULL) {
				pipeline = func->accel_kernel->backend_data;
				vkDestroyPipeline(vk->device, pipeline->pipeline, NULL);
				vkDestroyPipelineLayout(vk->device,
						pipeline->pipeline_layout, NULL);
				vkDestroyDescriptorSetLayout(vk->device,
						     pipeline->descriptor_layout, NULL);
				vkDestroyShaderModule(vk->device,
						pipeline->shader_module, NULL);
				noct_free(pipeline);
				func->accel_kernel->backend_data = NULL;
			}
			if (func->accel_program != NULL) {
				for (i = 0; i < func->accel_program->kernel_count; i++) {
					if (func->accel_program->kernel[i] == NULL ||
					    func->accel_program->kernel[i]->backend_data == NULL)
						continue;
					pipeline = func->accel_program->kernel[i]->backend_data;
					vkDestroyPipeline(vk->device, pipeline->pipeline, NULL);
					vkDestroyPipelineLayout(vk->device,
							pipeline->pipeline_layout, NULL);
					vkDestroyDescriptorSetLayout(vk->device,
							     pipeline->descriptor_layout, NULL);
					vkDestroyShaderModule(vk->device,
							pipeline->shader_module, NULL);
					noct_free(pipeline);
					func->accel_program->kernel[i]->backend_data = NULL;
				}
			}
			func = func->next;
		}
	}
	accel_vk_destroy_runtime(vk);
	vm->accel_runtime = NULL;
}
