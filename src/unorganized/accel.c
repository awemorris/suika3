/* -*- coding: utf-8; tab-width: 8; indent-tabs-mode: t; -*- */

/*
 * Noct Programming Language
 * Copyright (c) 2025, 2026, Awe Morris
 */

/* Backend-neutral accelerator descriptor and GPU-only managed execution. */

#include "../core/runtime.h"
#include "../core/objectmodel.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/* OpenWatcom 1.9's stdint.h does not define the C99 limit macros. */
#define ACCEL_API_INT64_MAX ((int64_t)(((uint64_t)-1) >> 1))

static bool rt_intrin_Accel_call(struct rt_env *env);
static bool rt_intrin_Accel_dispatchAsync(struct rt_env *env);
static bool rt_intrin_Accel_dispatchSync(struct rt_env *env);
static bool rt_intrin_Accel_join(struct rt_env *env);
static bool rt_intrin_Accel_copyToAccel(struct rt_env *env);
static bool rt_intrin_Accel_copyToAccelAsync(struct rt_env *env);
static bool rt_intrin_Accel_copyFromAccel(struct rt_env *env);
static bool rt_intrin_Accel_copyFromAccelAsync(struct rt_env *env);
static bool rt_intrin_Accel_int8(struct rt_env *env);
static bool rt_intrin_Accel_int16(struct rt_env *env);
static bool rt_intrin_Accel_int32(struct rt_env *env);
static bool rt_intrin_Accel_int64(struct rt_env *env);
static bool rt_intrin_Accel_uint8(struct rt_env *env);
static bool rt_intrin_Accel_uint16(struct rt_env *env);
static bool rt_intrin_Accel_uint32(struct rt_env *env);
static bool rt_intrin_Accel_uint64(struct rt_env *env);
static bool rt_intrin_Accel_float32(struct rt_env *env);
static bool rt_intrin_Accel_float64(struct rt_env *env);
static bool accel_reserve_event(struct rt_env *env, uint32_t *event_id,
				struct accel_event **ret_event);
static int accel_backend_dispatch(struct rt_env *env, struct rt_func *func,
				  uint32_t arg_count, struct rt_value *arg);
static const char *accel_backend_name(struct rt_env *env);
static int accel_backend_copy(struct rt_env *env, bool to_accel,
			      struct rt_packed *resource, size_t offset,
			      size_t size);
static bool accel_preflight_call(struct rt_env *env, struct rt_func *func,
				 uint32_t arg_count, struct rt_value *arg);

struct accel_copy_op {
	struct rt_value source;
	struct rt_value destination;
	size_t source_offset;
	size_t destination_offset;
	size_t length;
};

#if defined(NOCT_USE_ACCEL_VULKAN)
static const struct accel_backend_ops vulkan_backend = {
	ACCEL_BACKEND_INTERFACE_VERSION, NOCT_ACCEL_BACKEND_VULKAN,
	"Vulkan", 0, accel_vulkan_list_devices,
	accel_vulkan_dispatch, NULL, NULL, NULL,
	accel_vulkan_copy_to, accel_vulkan_copy_from, NULL,
	accel_vulkan_cleanup
};
#endif
#if defined(NOCT_USE_ACCEL_OPENGL)
static const struct accel_backend_ops opengl_backend = {
	ACCEL_BACKEND_INTERFACE_VERSION, NOCT_ACCEL_BACKEND_OPENGL,
	"OpenGL ES", 0, accel_opengl_list_devices,
	accel_opengl_dispatch, accel_opengl_dispatch_raw_async,
	accel_opengl_join, accel_opengl_copy_async,
	accel_opengl_copy_to, accel_opengl_copy_from,
	accel_opengl_sync_cpu, accel_opengl_cleanup
};
#endif
#if defined(NOCT_USE_ACCEL_DX12)
static const struct accel_backend_ops dx12_backend = {
	ACCEL_BACKEND_INTERFACE_VERSION, NOCT_ACCEL_BACKEND_DX12,
	"DirectX 12", 0, accel_dx12_list_devices,
	accel_dx12_dispatch, accel_dx12_dispatch_raw,
	accel_dx12_join, accel_dx12_copy_async,
	accel_dx12_copy_to, accel_dx12_copy_from, NULL,
	accel_dx12_cleanup
};
#endif

bool
accel_register_backend(struct rt_vm *vm,
		       const struct accel_backend_ops *backend)
{
	uint32_t i;

	if (backend == NULL ||
	    backend->interface_version != ACCEL_BACKEND_INTERFACE_VERSION ||
	    backend->id <= NOCT_ACCEL_BACKEND_NONE || backend->name == NULL ||
	    backend->name[0] == '\0' || backend->list_devices == NULL ||
	    backend->dispatch == NULL || vm->accel_backend_count >= ACCEL_BACKEND_MAX)
		return false;
	for (i = 0; i < vm->accel_backend_count; i++)
		if (vm->accel_backend[i]->id == backend->id ||
		    strcmp(vm->accel_backend[i]->name, backend->name) == 0)
			return false;
	vm->accel_backend[vm->accel_backend_count++] = backend;
	return true;
}

void
accel_register_builtin_backends(struct rt_vm *vm)
{
	uint32_t i;

	vm->accel_backend_count = 0;
	vm->selected_accel_backend = NULL;
#if defined(NOCT_USE_ACCEL_DX12)
	(void)accel_register_backend(vm, &dx12_backend);
#endif
#if defined(NOCT_USE_ACCEL_VULKAN)
	(void)accel_register_backend(vm, &vulkan_backend);
#endif
#if defined(NOCT_USE_ACCEL_OPENGL)
	(void)accel_register_backend(vm, &opengl_backend);
#endif
	if (!vm->config.accel_enable)
		return;
	for (i = 0; i < vm->accel_backend_count; i++) {
		if (vm->config.accel_backend == NOCT_ACCEL_BACKEND_NONE ||
		    vm->config.accel_backend == NOCT_ACCEL_BACKEND_AUTO ||
		    vm->accel_backend[i]->id == vm->config.accel_backend) {
			vm->selected_accel_backend = vm->accel_backend[i];
			vm->config.accel_backend =
				(uint8_t)vm->accel_backend[i]->id;
			break;
		}
	}
}

const struct accel_backend_ops *
accel_get_backend(struct rt_vm *vm)
{
	return vm->selected_accel_backend;
}

bool
accel_list_devices(void)
{
	bool found;

	found = false;
#if defined(NOCT_USE_ACCEL_DX12)
	printf("Backend: %s\n", dx12_backend.name);
	found = dx12_backend.list_devices() || found;
#endif
#if defined(NOCT_USE_ACCEL_VULKAN)
	printf("Backend: %s\n", vulkan_backend.name);
	found = vulkan_backend.list_devices() || found;
#endif
#if defined(NOCT_USE_ACCEL_OPENGL)
	printf("Backend: %s\n", opengl_backend.name);
	found = opengl_backend.list_devices() || found;
#endif
	return found;
}

bool
rt_register_accel_intrinsics(
	struct rt_env *env)
{
	struct accel_resource_intrin {
		const char *link_name;
		bool (*cfunc)(struct rt_env *env);
	};
	static const struct accel_resource_intrin resource_intrin[] = {
		{ "$Accel.int8", rt_intrin_Accel_int8 },
		{ "$Accel.int16", rt_intrin_Accel_int16 },
		{ "$Accel.int32", rt_intrin_Accel_int32 },
		{ "$Accel.int64", rt_intrin_Accel_int64 },
		{ "$Accel.uint8", rt_intrin_Accel_uint8 },
		{ "$Accel.uint16", rt_intrin_Accel_uint16 },
		{ "$Accel.uint32", rt_intrin_Accel_uint32 },
		{ "$Accel.uint64", rt_intrin_Accel_uint64 },
		{ "$Accel.float32", rt_intrin_Accel_float32 },
		{ "$Accel.float64", rt_intrin_Accel_float64 },
	};
	static const char *param[] = { "kernel" };
	static const char *length_param[] = { "length" };
	struct rt_func *func;
	struct rt_func *dispatch_async_func;
	struct rt_func *dispatch_sync_func;
	struct rt_func *join_func;
	struct rt_func *copy_to_func;
	struct rt_func *copy_to_async_func;
	struct rt_func *copy_from_func;
	struct rt_func *copy_from_async_func;
	struct rt_func *resource_func;
	struct rt_value pkg;
	struct rt_value val;
	static const char *join_param[] = { "event" };
	static const char *copy_to_param[] = {
		"hostSource", "hostByteOffset", "accelDestination",
		"accelByteOffset", "byteLength",
	};
	static const char *copy_from_param[] = {
		"accelSource", "accelByteOffset", "hostDestination",
		"hostByteOffset", "byteLength",
	};
	size_t i;

	if (!noct_register_cfunc(env, "Accel.call", 1, param,
				 rt_intrin_Accel_call, &func))
		return false;
	func->cfunc_variadic = true;
	func->tmpvar_size = NOCT_ARG_MAX;
	{
		static const char *dispatch_param[] = { "kernel", "grid", "block" };
		if (!noct_register_cfunc(env, "Accel.dispatchAsync", 3,
				 dispatch_param, rt_intrin_Accel_dispatchAsync,
				 &dispatch_async_func))
			return false;
		dispatch_async_func->cfunc_variadic = true;
		dispatch_async_func->tmpvar_size = NOCT_ARG_MAX;
		if (!noct_register_cfunc(env, "$Accel.dispatchSync", 3,
				 dispatch_param, rt_intrin_Accel_dispatchSync,
				 &dispatch_sync_func))
			return false;
		dispatch_sync_func->cfunc_variadic = true;
		dispatch_sync_func->tmpvar_size = NOCT_ARG_MAX;
	}
	if (!noct_register_cfunc(env, "Accel.join", 1, join_param,
				 rt_intrin_Accel_join, &join_func))
		return false;
	if (!noct_register_cfunc(env, "Accel.copyToAccel", 5, copy_to_param,
				 rt_intrin_Accel_copyToAccel, &copy_to_func) ||
	    !noct_register_cfunc(env, "Accel.copyToAccelAsync", 5,
				 copy_to_param, rt_intrin_Accel_copyToAccelAsync,
				 &copy_to_async_func) ||
	    !noct_register_cfunc(env, "Accel.copyFromAccel", 5, copy_from_param,
				 rt_intrin_Accel_copyFromAccel,
				 &copy_from_func) ||
	    !noct_register_cfunc(env, "Accel.copyFromAccelAsync", 5,
				 copy_from_param,
				 rt_intrin_Accel_copyFromAccelAsync,
				 &copy_from_async_func))
		return false;
	if (!rt_make_empty_dict(env, &pkg))
		return false;
	val.type = NOCT_VALUE_FUNC;
	val.val.func = func;
	if (!rt_set_dict_elem_cstr(env, &pkg, "call", &val) ||
	    !rt_set_global(env, "Accel", &pkg))
		return false;
	val.val.func = dispatch_async_func;
	if (!rt_set_dict_elem_cstr(env, &pkg, "dispatchAsync", &val))
		return false;
	val.val.func = join_func;
	if (!rt_set_dict_elem_cstr(env, &pkg, "join", &val))
		return false;
	val.val.func = copy_to_func;
	if (!rt_set_dict_elem_cstr(env, &pkg, "copyToAccel", &val))
		return false;
	val.val.func = copy_to_async_func;
	if (!rt_set_dict_elem_cstr(env, &pkg, "copyToAccelAsync", &val))
		return false;
	val.val.func = copy_from_func;
	if (!rt_set_dict_elem_cstr(env, &pkg, "copyFromAccel", &val))
		return false;
	val.val.func = copy_from_async_func;
	if (!rt_set_dict_elem_cstr(env, &pkg, "copyFromAccelAsync", &val))
		return false;
	for (i = 0; i < sizeof(resource_intrin) / sizeof(resource_intrin[0]); i++) {
		if (!noct_register_cfunc(env, resource_intrin[i].link_name, 1,
					 length_param, resource_intrin[i].cfunc,
					 &resource_func))
			return false;
	}
	if (!om_freeze_dict(env, &pkg) ||
	    !rt_mark_global_const(env, "Accel"))
		return false;
	return true;
}

static bool
accel_make_typed_resource(
	struct rt_env *env,
	int type,
	size_t element_width,
	const char *type_name)
{
	struct rt_value length_value;
	struct rt_value ret;
	size_t length;

	noct_pin_local(env, 2, &length_value, &ret);
	if (!noct_get_arg_check_int_long(env, 0, &length_value, &length))
		return false;
	if (length == 0) {
		rt_error(env, "Accel.%s(): element count is 0.", type_name);
		return false;
	}
	if (length > SIZE_MAX / element_width) {
		rt_error(env, "Accel.%s(): element count is too large.", type_name);
		return false;
	}
	if (!rt_make_packed(env, &ret, type, length * element_width,
			    length, NULL, NULL, NULL))
		return false;
	ret.val.packed->is_accel_resource = true;
	if (!noct_set_return(env, &ret))
		return false;
	noct_unpin_local(env, 2, &length_value, &ret);
	return true;
}

#define DEFINE_ACCEL_RESOURCE_INTRIN(name, packed_type, width) \
	static bool \
	rt_intrin_Accel_##name(struct rt_env *env) \
	{ \
		return accel_make_typed_resource(env, packed_type, width, #name); \
	}

DEFINE_ACCEL_RESOURCE_INTRIN(int8, NOCT_PACKED_INT8, 1)
DEFINE_ACCEL_RESOURCE_INTRIN(int16, NOCT_PACKED_INT16, 2)
DEFINE_ACCEL_RESOURCE_INTRIN(int32, NOCT_PACKED_INT32, 4)
DEFINE_ACCEL_RESOURCE_INTRIN(int64, NOCT_PACKED_INT64, 8)
DEFINE_ACCEL_RESOURCE_INTRIN(uint8, NOCT_PACKED_UINT8, 1)
DEFINE_ACCEL_RESOURCE_INTRIN(uint16, NOCT_PACKED_UINT16, 2)
DEFINE_ACCEL_RESOURCE_INTRIN(uint32, NOCT_PACKED_UINT32, 4)
DEFINE_ACCEL_RESOURCE_INTRIN(uint64, NOCT_PACKED_UINT64, 8)
DEFINE_ACCEL_RESOURCE_INTRIN(float32, NOCT_PACKED_FLOAT32, 4)
DEFINE_ACCEL_RESOURCE_INTRIN(float64, NOCT_PACKED_FLOAT64, 8)

#undef DEFINE_ACCEL_RESOURCE_INTRIN

static size_t
accel_packed_element_width(
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

static bool
accel_get_packed_bytes(
	struct rt_env *env,
	struct rt_value *value,
	const char *argument_name,
	bool require_accel,
	size_t *byte_size)
{
	size_t width;

	if (value->type != NOCT_VALUE_PACKED || value->val.packed == NULL) {
		rt_error(env, "%s is not Packed storage.", argument_name);
		return false;
	}
	if (value->val.packed->is_accel_resource != require_accel) {
		rt_error(env, "%s has the wrong accelerator storage kind.",
			 argument_name);
		return false;
	}
	width = accel_packed_element_width(value->val.packed->type);
	if (width == 0 || value->val.packed->elem_size > SIZE_MAX / width) {
		rt_error(env, "%s has an invalid Packed element layout.",
			 argument_name);
		return false;
	}
	*byte_size = value->val.packed->elem_size * width;
	return true;
}

static bool
accel_prepare_copy(
	struct rt_env *env,
	bool to_accel,
	struct accel_copy_op *op)
{
	struct rt_value source_offset_value;
	struct rt_value destination_offset_value;
	struct rt_value length_value;
	size_t source_size;
	size_t destination_size;

	if (!noct_get_arg(env, 0, &op->source) ||
	    !noct_get_arg_check_int_long(env, 1, &source_offset_value,
					 &op->source_offset) ||
	    !noct_get_arg(env, 2, &op->destination) ||
	    !noct_get_arg_check_int_long(env, 3, &destination_offset_value,
					 &op->destination_offset) ||
	    !noct_get_arg_check_int_long(env, 4, &length_value, &op->length))
		return false;
	if (!accel_get_packed_bytes(env, &op->source, "copy source", !to_accel,
				    &source_size) ||
	    !accel_get_packed_bytes(env, &op->destination, "copy destination",
				    to_accel, &destination_size))
		return false;
	if (op->source_offset > source_size ||
	    op->length > source_size - op->source_offset ||
	    op->destination_offset > destination_size ||
	    op->length > destination_size - op->destination_offset) {
		rt_error(env, "Accelerator copy range is out-of-bounds.");
		return false;
	}
	return true;
}

static bool
accel_execute_copy(
	struct rt_env *env,
	bool to_accel,
	struct accel_copy_op *op)
{
	if (!to_accel) {
		int copy_result;
		copy_result = accel_backend_copy(env, false, op->source.val.packed,
					 op->source_offset, op->length);
		if (copy_result == ACCEL_DISPATCH_ERROR)
			return false;
	}
	memmove((char *)op->destination.val.packed->packed_buffer +
		 op->destination_offset,
		(char *)op->source.val.packed->packed_buffer + op->source_offset,
		op->length);
	if (to_accel) {
		int copy_result;
		copy_result = accel_backend_copy(env, true,
					 op->destination.val.packed,
					 op->destination_offset, op->length);
		if (copy_result == ACCEL_DISPATCH_ERROR)
			return false;
	}
	return true;
}

static bool
accel_copy(
	struct rt_env *env,
	bool to_accel)
{
	struct accel_copy_op op;
	struct rt_value ret;

	if (!accel_prepare_copy(env, to_accel, &op) ||
	    !accel_execute_copy(env, to_accel, &op))
		return false;
	ret.type = NOCT_VALUE_INT;
	ret.val.i = 0;
	return noct_set_return(env, &ret);
}

static bool
rt_intrin_Accel_copyToAccel(
	struct rt_env *env)
{
	return accel_copy(env, true);
}

static bool
rt_intrin_Accel_copyFromAccel(
	struct rt_env *env)
{
	return accel_copy(env, false);
}

static bool
accel_copy_async(
	struct rt_env *env,
	bool to_accel)
{
	struct accel_copy_op op;
	struct rt_value ret;
	struct accel_event *event;
	uint32_t event_id;
	int copy_result;
	const struct accel_backend_ops *backend;

	if (!accel_prepare_copy(env, to_accel, &op) ||
	    !accel_reserve_event(env, &event_id, &event))
		return false;
	copy_result = ACCEL_DISPATCH_FALLBACK;
	backend = accel_get_backend(env->vm);
	if (env->vm->config.accel_enable && backend != NULL &&
	    backend->copy_async != NULL) {
		event->backend = backend;
		copy_result = backend->copy_async(
			env, to_accel, op.source.val.packed, op.source_offset,
			op.destination.val.packed, op.destination_offset,
			op.length, event);
	}
	if (copy_result == ACCEL_DISPATCH_ERROR) {
		event->state = ACCEL_EVENT_FREE;
		return false;
	}
	if (copy_result == ACCEL_DISPATCH_FALLBACK) {
		if (!accel_execute_copy(env, to_accel, &op)) {
			event->state = ACCEL_EVENT_FREE;
			return false;
		}
		event->state = ACCEL_EVENT_COMPLETE;
	} else {
		event->state = event->backend_data != NULL ?
			ACCEL_EVENT_SUBMITTED : ACCEL_EVENT_COMPLETE;
	}
	ret.type = NOCT_VALUE_INT;
	ret.val.i = (int)event_id;
	return noct_set_return(env, &ret);
}

static bool
rt_intrin_Accel_copyToAccelAsync(
	struct rt_env *env)
{
	return accel_copy_async(env, true);
}

static bool
rt_intrin_Accel_copyFromAccelAsync(
	struct rt_env *env)
{
	return accel_copy_async(env, false);
}

static bool
rt_intrin_Accel_call(
	struct rt_env *env)
{
	struct rt_value kernel_value;
	struct rt_value arg[NOCT_ARG_MAX];
	struct rt_value ignored;
	uint32_t argc;
	uint32_t i;
	int dispatch_result;

	argc = env->frame->arg_count;
	if (argc < 1 || argc > NOCT_ARG_MAX) {
		rt_error(env, "Accel.call(): invalid argument count.");
		return false;
	}
	kernel_value = env->frame->tmpvar[0];
	if (kernel_value.type != NOCT_VALUE_FUNC ||
	    kernel_value.val.func->func_kind != NOCT_FUNC_ACCEL) {
		rt_error(env, "Accel.call(): first argument is not an accelerator function.");
		return false;
	}
	for (i = 1; i < argc; i++)
		arg[i - 1] = env->frame->tmpvar[i];
	if (!accel_preflight_call(env, kernel_value.val.func, argc - 1, arg))
		return false;
	if (!env->vm->config.accel_enable) {
		rt_error(env,
			 "Accel.call(): GPU accelerator is disabled; __accel func has no CPU fallback.");
		return false;
	}
	if (kernel_value.val.func->accel_program == NULL ||
	    kernel_value.val.func->accel_kernel == NULL ||
	    !kernel_value.val.func->accel_kernel->eligible) {
		rt_error(env,
			 "Accel.call(): accelerator function has no executable GPU program.");
		return false;
	}
	dispatch_result = accel_backend_dispatch(env, kernel_value.val.func,
						 argc - 1, arg);
	if (dispatch_result == ACCEL_DISPATCH_ERROR)
		return false;
	if (dispatch_result != ACCEL_DISPATCH_OK) {
		rt_error(env,
			 "Accel.call(): %s backend is unavailable or rejected the GPU program.",
			 accel_backend_name(env));
		return false;
	}
	ignored.type = NOCT_VALUE_INT;
	ignored.val.i = 0;
	return noct_set_return(env, &ignored);
}

static bool
accel_preflight_call(
	struct rt_env *env,
	struct rt_func *func,
	uint32_t arg_count,
	struct rt_value *arg)
{
	uint32_t i;
	uint32_t j;
	struct accel_kernel *kernel;
	int64_t count;
	int64_t required_end;

	if (arg_count != func->param_count) {
		rt_error(env, "Accel.call(): function arguments do not match.");
		return false;
	}
	for (i = 0; i < arg_count; i++) {
		int transport;
		transport = func->param_accel_transport[i];
		if (transport == ACCEL_TRANSPORT_SCALAR) {
			if (func->param_type[i] >= 0 && arg[i].type != func->param_type[i]) {
				rt_error(env, "Accel.call(): scalar argument type does not match.");
				return false;
			}
			continue;
		}
		if (arg[i].type != NOCT_VALUE_PACKED ||
		    arg[i].val.packed == NULL) {
			rt_error(env, "Accel.call(): buffer argument is not Packed storage.");
			return false;
		}
		if (transport == ACCEL_TRANSPORT_DEVICE_PTR &&
		    !arg[i].val.packed->is_accel_resource) {
			rt_error(env, "Accel.call(): _ptr argument is not an Accel resource.");
			return false;
		}
		if ((transport == ACCEL_TRANSPORT_COPY_IN ||
		     transport == ACCEL_TRANSPORT_COPY_OUT) &&
		    arg[i].val.packed->is_accel_resource) {
			rt_error(env, "Accel.call(): _in/_out require host Packed storage; use _ptr for an Accel resource.");
			return false;
		}
		if (func->param_packed_type[i] >= 0 &&
		    arg[i].val.packed->type != func->param_packed_type[i]) {
			rt_error(env, "Accel.call(): buffer element type does not match.");
			return false;
		}
		for (j = 0; j < i; j++) {
			if (func->param_accel_transport[j] != ACCEL_TRANSPORT_SCALAR &&
			    arg[j].type == NOCT_VALUE_PACKED &&
			    arg[j].val.packed == arg[i].val.packed) {
				rt_error(env, "Accel.call(): restricted buffer arguments must not alias.");
				return false;
			}
		}
	}
	kernel = func->accel_kernel;
	if (kernel == NULL || !kernel->eligible || kernel->dispatch_param < 0)
		return true;
	if (arg[kernel->dispatch_param].type != NOCT_VALUE_INT ||
	    arg[kernel->dispatch_param].val.i < 0) {
		rt_error(env, "Accel.call(): managed loop range must be non-negative.");
		return false;
	}
	count = arg[kernel->dispatch_param].val.i;
	if (count == 0)
		return true;
	for (i = 0; i < arg_count; i++) {
		struct accel_param_range *range;
		range = &kernel->param_range[i];
		if (range->status != ACCEL_RANGE_COMPLETE || !range->has_access)
			continue;
		if (range->min_offset < 0 ||
		    (range->max_offset > 0 &&
		     count > ACCEL_API_INT64_MAX - range->max_offset)) {
			rt_error(env, "Accel.call(): required buffer range overflows.");
			return false;
		}
		required_end = count + range->max_offset;
		if (required_end < 0 || arg[i].type != NOCT_VALUE_PACKED ||
		    (uint64_t)required_end > (uint64_t)arg[i].val.packed->elem_size) {
			rt_error(env, "Accel.call(): buffer is shorter than the managed kernel required range.");
			return false;
		}
	}
	return true;
}

static const char *
accel_backend_name(
	struct rt_env *env)
{
	const struct accel_backend_ops *backend;
	backend = accel_get_backend(env->vm);
	return backend != NULL ? backend->name : "disabled";
}

static int
accel_backend_dispatch(
	struct rt_env *env,
	struct rt_func *func,
	uint32_t arg_count,
	struct rt_value *arg)
{
	const struct accel_backend_ops *backend;
	backend = accel_get_backend(env->vm);
	if (backend == NULL || backend->dispatch == NULL)
		return ACCEL_DISPATCH_FALLBACK;
	return backend->dispatch(env, func, arg_count, arg);
}

static int
accel_backend_copy(
	struct rt_env *env,
	bool to_accel,
	struct rt_packed *resource,
	size_t offset,
	size_t size)
{
	const struct accel_backend_ops *backend;

	if (!env->vm->config.accel_enable)
		return ACCEL_DISPATCH_FALLBACK;
	backend = accel_get_backend(env->vm);
	if (backend == NULL)
		return ACCEL_DISPATCH_FALLBACK;
	if (to_accel && backend->copy_to != NULL)
		return backend->copy_to(env, resource, offset, size);
	if (!to_accel && backend->copy_from != NULL)
		return backend->copy_from(env, resource, offset, size);
	return ACCEL_DISPATCH_FALLBACK;
}

static bool
accel_reserve_event(
	struct rt_env *env,
	uint32_t *event_id,
	struct accel_event **ret_event)
{
	uint32_t i;
	struct accel_event *event;

	for (i = 0; i < ACCEL_EVENT_MAX; i++) {
		event = &env->vm->accel_event[i];
		if (event->state != ACCEL_EVENT_FREE &&
		    event->state != ACCEL_EVENT_JOINED)
			continue;
		event->generation++;
		if (event->generation == 0)
			event->generation = 1;
		event->state = ACCEL_EVENT_RESERVED;
		event->output.type = NOCT_VALUE_INT;
		event->output.val.i = 0;
		event->output_pinned = false;
		event->retained_count = 0;
		event->backend_data = NULL;
		event->backend = NULL;
		*event_id = event->generation * ACCEL_EVENT_MAX + i;
		*ret_event = event;
		return true;
	}
	rt_error(env, "Accelerator event table is full.");
	return false;
}

static bool
rt_intrin_Accel_dispatchAsync(
	struct rt_env *env)
{
	struct rt_value kernel_value;
	struct rt_value grid_value;
	struct rt_value block_value;
	struct rt_value arg[NOCT_ARG_MAX];
	struct rt_value ret;
	struct rt_func *func;
	struct accel_event *event;
	uint32_t argc;
	uint32_t arg_count;
	uint32_t event_id;
	uint32_t i;
	uint32_t j;
	int result;
	const struct accel_backend_ops *backend;

	argc = env->frame->arg_count;
	if (argc < 3 || argc > NOCT_ARG_MAX) {
		rt_error(env, "Accel.dispatchAsync(): invalid argument count.");
		return false;
	}
	kernel_value = env->frame->tmpvar[0];
	grid_value = env->frame->tmpvar[1];
	block_value = env->frame->tmpvar[2];
	if (kernel_value.type != NOCT_VALUE_FUNC ||
	    kernel_value.val.func->func_kind != NOCT_FUNC_GPU) {
		rt_error(env, "Accel.dispatchAsync(): first argument is not a gpu function.");
		return false;
	}
	if (grid_value.type != NOCT_VALUE_INT || grid_value.val.i <= 0 ||
	    block_value.type != NOCT_VALUE_INT || block_value.val.i <= 0) {
		rt_error(env, "Accel.dispatchAsync(): grid and block must be positive integers.");
		return false;
	}
	func = kernel_value.val.func;
	arg_count = argc - 3;
	if (arg_count != func->param_count) {
		rt_error(env, "Accel.dispatchAsync(): function arguments do not match.");
		return false;
	}
	for (i = 0; i < arg_count; i++) {
		arg[i] = env->frame->tmpvar[i + 3];
		if (func->param_accel_transport[i] == ACCEL_TRANSPORT_SCALAR) {
			if (arg[i].type != func->param_type[i]) {
				rt_error(env, "Accel.dispatchAsync(): scalar argument type does not match.");
				return false;
			}
			continue;
		}
		if (func->param_accel_transport[i] != ACCEL_TRANSPORT_DEVICE_PTR ||
		    arg[i].type != NOCT_VALUE_PACKED || arg[i].val.packed == NULL ||
		    !arg[i].val.packed->is_accel_resource ||
		    arg[i].val.packed->type != func->param_packed_type[i]) {
			rt_error(env, "Accel.dispatchAsync(): _ptr argument type/storage does not match.");
			return false;
		}
		for (j = 0; j < i; j++) {
			if (func->param_accel_transport[j] == ACCEL_TRANSPORT_DEVICE_PTR &&
			    arg[j].val.packed == arg[i].val.packed) {
				rt_error(env, "Accel.dispatchAsync(): accelerator buffer arguments must not alias.");
				return false;
			}
		}
	}
	backend = accel_get_backend(env->vm);
	if (!env->vm->config.accel_enable || backend == NULL ||
	    backend->dispatch_raw_async == NULL || backend->join == NULL) {
		rt_error(env, "Accel.dispatchAsync(): __gpu func requires the OpenGL backend or DirectX 12 backend.");
		return false;
	}
	if (!accel_reserve_event(env, &event_id, &event)) return false;
	event->backend = backend;
	event->retained[0] = kernel_value;
	for (i = 0; i < arg_count; i++) event->retained[i + 1] = arg[i];
	event->retained_count = arg_count + 1;
	result = backend->dispatch_raw_async(env, func,
			(uint32_t)grid_value.val.i, (uint32_t)block_value.val.i,
			arg_count, arg, event);
	if (result != ACCEL_DISPATCH_OK) {
		event->retained_count = 0;
		event->state = ACCEL_EVENT_FREE;
		return false;
	}
	event->state = ACCEL_EVENT_SUBMITTED;
	ret.type = NOCT_VALUE_INT;
	ret.val.i = (int)event_id;
	return noct_set_return(env, &ret);
}

static bool
rt_intrin_Accel_dispatchSync(
	struct rt_env *env)
{
	struct rt_value event_value;
	struct rt_value ret;
	struct accel_event *event;
	uint32_t id;
	uint32_t index;
	uint32_t generation;
	bool ok;

	if (!rt_intrin_Accel_dispatchAsync(env))
		return false;
	event_value = env->frame->tmpvar[0];
	if (event_value.type != NOCT_VALUE_INT || event_value.val.i <= 0) {
		rt_error(env, "Synchronous gpu launch did not produce an event.");
		return false;
	}
	id = (uint32_t)event_value.val.i;
	index = id % ACCEL_EVENT_MAX;
	generation = id / ACCEL_EVENT_MAX;
	event = &env->vm->accel_event[index];
	if (event->generation != generation ||
	    event->state != ACCEL_EVENT_SUBMITTED) {
		rt_error(env, "Synchronous gpu launch produced a stale event.");
		return false;
	}
	ok = event->backend != NULL && event->backend->join != NULL &&
		event->backend->join(env, event);
	event->retained_count = 0;
	event->state = ACCEL_EVENT_JOINED;
	if (!ok)
		return false;
	ret.type = NOCT_VALUE_INT;
	ret.val.i = 0;
	return noct_set_return(env, &ret);
}

static bool
rt_intrin_Accel_join(
	struct rt_env *env)
{
	struct rt_value id_value;
	struct rt_value ret;
	struct accel_event *event;
	uint32_t id;
	uint32_t index;
	uint32_t generation;
	bool ok;

	if (!noct_get_arg(env, 0, &id_value) ||
	    id_value.type != NOCT_VALUE_INT || id_value.val.i <= 0) {
		rt_error(env, "Accel.join(): invalid event ID.");
		return false;
	}
	id = (uint32_t)id_value.val.i;
	index = id % ACCEL_EVENT_MAX;
	generation = id / ACCEL_EVENT_MAX;
	event = &env->vm->accel_event[index];
	if (event->generation != generation ||
	    (event->state != ACCEL_EVENT_COMPLETE &&
	     event->state != ACCEL_EVENT_SUBMITTED)) {
		rt_error(env, "Accel.join(): stale or already consumed event ID.");
		return false;
	}
	ok = true;
	if (event->state == ACCEL_EVENT_SUBMITTED) {
		if (event->backend == NULL || event->backend->join == NULL) {
			rt_error(env, "Accel.join(): event backend is unavailable.");
			ok = false;
		} else {
			ok = event->backend->join(env, event);
		}
	}
	/* A join always consumes the event, including a failed device wait. */
	event->state = ACCEL_EVENT_JOINED;
	event->retained_count = 0;
	if (!ok)
		return false;
	ret.type = NOCT_VALUE_INT;
	ret.val.i = 0;
	return noct_set_return(env, &ret);
}

#if !defined(NOCT_USE_ACCEL_VULKAN)
int
accel_vulkan_dispatch(
	struct rt_env *env,
	struct rt_func *func,
	uint32_t arg_count,
	struct rt_value *arg)
{
	UNUSED_PARAMETER(env);
	UNUSED_PARAMETER(func);
	UNUSED_PARAMETER(arg_count);
	UNUSED_PARAMETER(arg);
	return ACCEL_DISPATCH_FALLBACK;
}

int
accel_vulkan_copy_to(
	struct rt_env *env,
	struct rt_packed *resource,
	size_t offset,
	size_t size)
{
	UNUSED_PARAMETER(env);
	UNUSED_PARAMETER(resource);
	UNUSED_PARAMETER(offset);
	UNUSED_PARAMETER(size);
	return ACCEL_DISPATCH_FALLBACK;
}

int
accel_vulkan_copy_from(
	struct rt_env *env,
	struct rt_packed *resource,
	size_t offset,
	size_t size)
{
	UNUSED_PARAMETER(env);
	UNUSED_PARAMETER(resource);
	UNUSED_PARAMETER(offset);
	UNUSED_PARAMETER(size);
	return ACCEL_DISPATCH_FALLBACK;
}

void
accel_vulkan_cleanup(
	struct rt_vm *vm)
{
	UNUSED_PARAMETER(vm);
}
#endif

#if !defined(NOCT_USE_ACCEL_DX12)
int
accel_dx12_dispatch(
	struct rt_env *env,
	struct rt_func *func,
	uint32_t arg_count,
	struct rt_value *arg)
{
	UNUSED_PARAMETER(env);
	UNUSED_PARAMETER(func);
	UNUSED_PARAMETER(arg_count);
	UNUSED_PARAMETER(arg);
	return ACCEL_DISPATCH_FALLBACK;
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
	UNUSED_PARAMETER(env);
	UNUSED_PARAMETER(func);
	UNUSED_PARAMETER(grid_size);
	UNUSED_PARAMETER(block_size);
	UNUSED_PARAMETER(arg_count);
	UNUSED_PARAMETER(arg);
	UNUSED_PARAMETER(event);
	return ACCEL_DISPATCH_FALLBACK;
}

bool
accel_dx12_join(
	struct rt_env *env,
	struct accel_event *event)
{
	UNUSED_PARAMETER(env);
	UNUSED_PARAMETER(event);
	return false;
}

bool
accel_dx12_list_devices(void)
{
	return false;
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
	UNUSED_PARAMETER(env);
	UNUSED_PARAMETER(to_accel);
	UNUSED_PARAMETER(source);
	UNUSED_PARAMETER(source_offset);
	UNUSED_PARAMETER(destination);
	UNUSED_PARAMETER(destination_offset);
	UNUSED_PARAMETER(size);
	UNUSED_PARAMETER(event);
	return ACCEL_DISPATCH_FALLBACK;
}

int
accel_dx12_copy_to(
	struct rt_env *env,
	struct rt_packed *resource,
	size_t offset,
	size_t size)
{
	UNUSED_PARAMETER(env);
	UNUSED_PARAMETER(resource);
	UNUSED_PARAMETER(offset);
	UNUSED_PARAMETER(size);
	return ACCEL_DISPATCH_FALLBACK;
}

int
accel_dx12_copy_from(
	struct rt_env *env,
	struct rt_packed *resource,
	size_t offset,
	size_t size)
{
	UNUSED_PARAMETER(env);
	UNUSED_PARAMETER(resource);
	UNUSED_PARAMETER(offset);
	UNUSED_PARAMETER(size);
	return ACCEL_DISPATCH_FALLBACK;
}

void
accel_dx12_cleanup(
	struct rt_vm *vm)
{
	UNUSED_PARAMETER(vm);
}
#endif

#if !defined(NOCT_USE_ACCEL_OPENGL)
int
accel_opengl_dispatch(
	struct rt_env *env,
	struct rt_func *func,
	uint32_t arg_count,
	struct rt_value *arg)
{
	UNUSED_PARAMETER(env);
	UNUSED_PARAMETER(func);
	UNUSED_PARAMETER(arg_count);
	UNUSED_PARAMETER(arg);
	return ACCEL_DISPATCH_FALLBACK;
}

int
accel_opengl_dispatch_async(
	struct rt_env *env,
	struct rt_func *func,
	uint32_t arg_count,
	struct rt_value *arg,
	struct accel_event *event)
{
	UNUSED_PARAMETER(env);
	UNUSED_PARAMETER(func);
	UNUSED_PARAMETER(arg_count);
	UNUSED_PARAMETER(arg);
	UNUSED_PARAMETER(event);
	return ACCEL_DISPATCH_FALLBACK;
}

int
accel_opengl_dispatch_raw_async(
	struct rt_env *env,
	struct rt_func *func,
	uint32_t grid_size,
	uint32_t block_size,
	uint32_t arg_count,
	struct rt_value *arg,
	struct accel_event *event)
{
	UNUSED_PARAMETER(env);
	UNUSED_PARAMETER(func);
	UNUSED_PARAMETER(grid_size);
	UNUSED_PARAMETER(block_size);
	UNUSED_PARAMETER(arg_count);
	UNUSED_PARAMETER(arg);
	UNUSED_PARAMETER(event);
	return ACCEL_DISPATCH_ERROR;
}

bool
accel_opengl_join(
	struct rt_env *env,
	struct accel_event *event)
{
	UNUSED_PARAMETER(event);
	rt_error(env, "OpenGL asynchronous accelerator support is not available.");
	return false;
}

int
accel_opengl_copy_to(
	struct rt_env *env,
	struct rt_packed *resource,
	size_t offset,
	size_t size)
{
	UNUSED_PARAMETER(env);
	UNUSED_PARAMETER(resource);
	UNUSED_PARAMETER(offset);
	UNUSED_PARAMETER(size);
	return ACCEL_DISPATCH_FALLBACK;
}

int
accel_opengl_copy_from(
	struct rt_env *env,
	struct rt_packed *resource,
	size_t offset,
	size_t size)
{
	UNUSED_PARAMETER(env);
	UNUSED_PARAMETER(resource);
	UNUSED_PARAMETER(offset);
	UNUSED_PARAMETER(size);
	return ACCEL_DISPATCH_FALLBACK;
}

int
accel_opengl_copy_async(
	struct rt_env *env,
	bool to_accel,
	struct rt_packed *source,
	size_t source_offset,
	struct rt_packed *destination,
	size_t destination_offset,
	size_t size,
	struct accel_event *event)
{
	UNUSED_PARAMETER(env);
	UNUSED_PARAMETER(to_accel);
	UNUSED_PARAMETER(source);
	UNUSED_PARAMETER(source_offset);
	UNUSED_PARAMETER(destination);
	UNUSED_PARAMETER(destination_offset);
	UNUSED_PARAMETER(size);
	UNUSED_PARAMETER(event);
	return ACCEL_DISPATCH_FALLBACK;
}

bool
accel_opengl_sync_cpu(
	struct rt_env *env,
	struct rt_func *func,
	uint32_t arg_count,
	struct rt_value *arg,
	bool before_call)
{
	UNUSED_PARAMETER(env);
	UNUSED_PARAMETER(func);
	UNUSED_PARAMETER(arg_count);
	UNUSED_PARAMETER(arg);
	UNUSED_PARAMETER(before_call);
	return true;
}

void
accel_opengl_cleanup(
	struct rt_vm *vm)
{
	UNUSED_PARAMETER(vm);
}
#endif

void
accel_runtime_cleanup(
	struct rt_vm *vm)
{
	const struct accel_backend_ops *backend;
	backend = accel_get_backend(vm);
	if (backend != NULL && backend->cleanup != NULL)
		backend->cleanup(vm);
}

struct accel_kernel *
accel_kernel_clone(
	const struct accel_kernel *src)
{
	struct accel_kernel *dst;

	if (src == NULL)
		return NULL;
	dst = noct_malloc(sizeof(*dst));
	if (dst == NULL)
		return NULL;
	memcpy(dst, src, sizeof(*dst));
	dst->name = NULL;
	dst->source_name = NULL;
	dst->glsl = NULL;
	dst->hlsl = NULL;
	dst->backend_data = NULL;
	if (src->name != NULL) {
		dst->name = noct_strdup(src->name);
		if (dst->name == NULL)
			goto failed;
	}
	if (src->source_name != NULL) {
		dst->source_name = noct_strdup(src->source_name);
		if (dst->source_name == NULL)
			goto failed;
	}
	if (src->glsl != NULL) {
		dst->glsl = noct_malloc(src->glsl_size + 1);
		if (dst->glsl == NULL)
			goto failed;
		memcpy(dst->glsl, src->glsl, src->glsl_size);
		dst->glsl[src->glsl_size] = '\0';
	}
	if (src->hlsl != NULL) {
		dst->hlsl = noct_malloc(src->hlsl_size + 1);
		if (dst->hlsl == NULL)
			goto failed;
		memcpy(dst->hlsl, src->hlsl, src->hlsl_size);
		dst->hlsl[src->hlsl_size] = '\0';
	}
	return dst;

failed:
	accel_kernel_free(dst);
	return NULL;
}

void
accel_kernel_free(
	struct accel_kernel *kernel)
{
	if (kernel == NULL)
		return;
	noct_free(kernel->name);
	noct_free(kernel->source_name);
	noct_free(kernel->glsl);
	noct_free(kernel->hlsl);
	noct_free(kernel);
}
