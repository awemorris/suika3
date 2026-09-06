/* -*- coding: utf-8; tab-width: 8; indent-tabs-mode: t; -*- */

/*
 * Noct Programming Language
 * Copyright (c) 2025, 2026, Awe Morris
 */

/*
 * Private Vulkan backend and device-independent injection seam.
 */

#ifndef NOCT_ACCEL_VULKAN_H
#define NOCT_ACCEL_VULKAN_H

#include "accel_backend.h"
#include "accel_device.h"

#include <vulkan/vulkan.h>

struct accel_vulkan_api {
	PFN_vkGetInstanceProcAddr get_instance_proc_addr;
	PFN_vkCreateInstance create_instance;
	PFN_vkDestroyInstance destroy_instance;
	PFN_vkEnumeratePhysicalDevices enumerate_physical_devices;
	PFN_vkGetPhysicalDeviceProperties2 get_physical_device_properties2;
	PFN_vkGetPhysicalDeviceQueueFamilyProperties get_physical_device_queue_family_properties;
	PFN_vkGetPhysicalDeviceMemoryProperties get_physical_device_memory_properties;
	PFN_vkCreateDevice create_device;
	PFN_vkDestroyDevice destroy_device;
	PFN_vkGetDeviceQueue get_device_queue;
	PFN_vkDeviceWaitIdle device_wait_idle;
	PFN_vkCreateShaderModule create_shader_module;
	PFN_vkDestroyShaderModule destroy_shader_module;
	PFN_vkCreateDescriptorSetLayout create_descriptor_set_layout;
	PFN_vkDestroyDescriptorSetLayout destroy_descriptor_set_layout;
	PFN_vkCreatePipelineLayout create_pipeline_layout;
	PFN_vkDestroyPipelineLayout destroy_pipeline_layout;
	PFN_vkCreateComputePipelines create_compute_pipelines;
	PFN_vkDestroyPipeline destroy_pipeline;
	PFN_vkCreateBuffer create_buffer;
	PFN_vkDestroyBuffer destroy_buffer;
	PFN_vkGetBufferMemoryRequirements get_buffer_memory_requirements;
	PFN_vkAllocateMemory allocate_memory;
	PFN_vkFreeMemory free_memory;
	PFN_vkBindBufferMemory bind_buffer_memory;
	PFN_vkMapMemory map_memory;
	PFN_vkUnmapMemory unmap_memory;
	PFN_vkFlushMappedMemoryRanges flush_mapped_memory_ranges;
	PFN_vkInvalidateMappedMemoryRanges invalidate_mapped_memory_ranges;
	PFN_vkCreateDescriptorPool create_descriptor_pool;
	PFN_vkDestroyDescriptorPool destroy_descriptor_pool;
	PFN_vkAllocateDescriptorSets allocate_descriptor_sets;
	PFN_vkUpdateDescriptorSets update_descriptor_sets;
	PFN_vkCreateCommandPool create_command_pool;
	PFN_vkDestroyCommandPool destroy_command_pool;
	PFN_vkAllocateCommandBuffers allocate_command_buffers;
	PFN_vkBeginCommandBuffer begin_command_buffer;
	PFN_vkEndCommandBuffer end_command_buffer;
	PFN_vkCmdBindPipeline cmd_bind_pipeline;
	PFN_vkCmdBindDescriptorSets cmd_bind_descriptor_sets;
	PFN_vkCmdCopyBuffer cmd_copy_buffer;
	PFN_vkCmdPipelineBarrier cmd_pipeline_barrier;
	PFN_vkCmdFillBuffer cmd_fill_buffer;
	PFN_vkCmdDispatch cmd_dispatch;
	PFN_vkCreateFence create_fence;
	PFN_vkDestroyFence destroy_fence;
	PFN_vkQueueSubmit queue_submit;
	PFN_vkWaitForFences wait_for_fences;
};

/*
 * Enumerates suitable Vulkan compute devices without opening logical devices.
 */
bool
accel_vulkan_enumerate(
	struct accel_device_list *list,
	char *error,
	size_t error_size);

/*
 * Enumerates suitable Vulkan devices through an injected function table.
 */
bool
accel_vulkan_enumerate_with_api(
	struct accel_device_list *list,
	const struct accel_vulkan_api *api,
	char *error,
	size_t error_size);

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
	void **backend_state);

/*
 * Creates the production Vulkan backend with the process loader table.
 */
bool
accel_vulkan_create(
	struct rt_env *env,
	const char *gpu_name,
	const struct accel_backend_ops **ops,
	void **backend_state);

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
	void **backend_state);

#endif
