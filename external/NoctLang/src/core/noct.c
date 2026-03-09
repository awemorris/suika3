/* -*- coding: utf-8; tab-width: 8; indent-tabs-mode: t; -*- */

/*
 * Copyright (c) 2025, Awe Morris. All rights reserved.
 */

/*
 * NoctVM Public Interface
 */

#include <noct/noct.h>
#include "runtime.h"

#include <stdio.h>
#include <stdarg.h>
#include <assert.h>

NOCT_DLL
bool
noct_create_vm(
	NoctVM **vm,
	NoctEnv **default_env)
{
	assert(vm != NULL);
	assert(default_env != NULL);

	if (!rt_create_vm(vm, default_env))
		return false;

	return true;
}

NOCT_DLL
bool
noct_destroy_vm(
	NoctVM *vm)
{
	assert(vm != NULL);

	if (!rt_destroy_vm(vm))
		return false;

	return true;
}

NOCT_DLL
bool
noct_register_source(
	NoctEnv *env,
	const char *file_name,
	const char *source_text)
{
	assert(env != NULL);
	assert(file_name != NULL);
	assert(source_text != NULL);

	if (!rt_register_source(env, file_name, source_text))
		return false;

	return true;
}

NOCT_DLL
bool
noct_register_bytecode(
	NoctEnv *env,
	uint8_t *data,
	size_t size)
{
	assert(env != NULL);
	assert(size > 0);
	assert(data != NULL);

	if (!rt_register_bytecode(env, size, data))
		return false;

	return true;
}

NOCT_DLL
bool
noct_register_cfunc(
	NoctEnv *env,
	const char *name,
	int param_count,
	const char *param_name[],
	bool (*cfunc)(NoctEnv *env),
	NoctFunc **ret_func)
{
	assert(env != NULL);
	assert(name != NULL);
	assert(param_count >= 0);
	assert(cfunc != NULL);

	if (!rt_register_cfunc(env, name, param_count, param_name, cfunc, ret_func))
		return false;

	return true;
}

#if defined(NOCT_USE_MULTITHREAD)
NOCT_DLL
bool
noct_create_thread_env(
	NoctEnv *prev_env,
	NoctEnv **new_env)
{
	assert(prev_env != NULL);
	assert(new_env != NULL);

	if (!rt_create_thread_env(prev_env, new_env))
		return false;

	return true;
}
#endif

NOCT_DLL
bool
noct_enter_vm(
	NoctEnv *env,
	const char *func_name,
	int arg_count,
	NoctValue *arg,
	NoctValue *ret)
{
	assert(env != NULL);
	assert(func_name != NULL);
	assert(arg_count >= 0);

	if (!rt_call_with_name(env, func_name, arg_count, arg, ret))
		return false;

	return true;
}

NOCT_DLL
bool
noct_get_error_file(
	NoctEnv *env,
	const char **msg)
{
	assert(env != NULL);
	assert(msg != NULL);

	*msg = rt_get_error_file(env);

	return true;
}

NOCT_DLL
bool
noct_get_error_line(
	NoctEnv *env,
	int *line)
{
	assert(env != NULL);
	assert(line != NULL);

	*line = rt_get_error_line(env);

	return true;	
}

NOCT_DLL
bool
noct_get_error_message(
	NoctEnv *env,
	const char **msg)
{
	assert(env != NULL);
	assert(msg != NULL);

	*msg = rt_get_error_message(env);

	return true;
}

NOCT_DLL
bool
noct_call(
	NoctEnv *env,
	NoctFunc *func,
	int arg_count,
	NoctValue *arg,
	NoctValue *ret)
{
	assert(env != NULL);
	assert(func != NULL);
	assert(arg_count >= 0);

	if (!rt_call(env, func, arg_count, arg, ret))
		return false;

	return true;
}

NOCT_DLL
bool
noct_make_int(
	NoctEnv *env,
	NoctValue *val,
	int i)
{
	assert(env != NULL);
	assert(val != NULL);

	val->type = NOCT_VALUE_INT;
	val->val.i = i;

	return true;
}

NOCT_DLL
bool
noct_make_float(
	NoctEnv *env,
	NoctValue *val,
	float f)
{
	assert(env != NULL);
	assert(val != NULL);

	val->type = NOCT_VALUE_FLOAT;
	val->val.f = f;

	return true;
}

NOCT_DLL
bool
noct_make_string(
	NoctEnv *env,
	NoctValue *val,
	const char *data)
{
	assert(env != NULL);
	assert(val != NULL);
	assert(data != NULL);

	if (!rt_make_string(env, val, data))
		return false;

	return true;
}

NOCT_DLL
bool
noct_make_empty_array(
	NoctEnv *env,
	NoctValue *val)
{
	assert(env != NULL);
	assert(val != NULL);

	if (!rt_make_empty_array(env, val))
		return false;

	return true;
}

NOCT_DLL
bool
noct_make_empty_dict(
	NoctEnv *env,
	NoctValue *val)
{
	assert(env != NULL);
	assert(val != NULL);

	if (!rt_make_empty_dict(env, val))
		return false;

	return true;
}

NOCT_DLL
bool
noct_get_value_type(
	NoctEnv *env,
	NoctValue *val,
	int *type)
{
	assert(env != NULL);
	assert(val != NULL);
	assert(type != NULL);

	*type = val->type;

	return true;
}

NOCT_DLL
bool
noct_get_int(
	NoctEnv *env,
	NoctValue *val,
	int *i)
{
	assert(env != NULL);
	assert(val != NULL);
	assert(i != NULL);

	/* Check the type. */
	if (val->type != NOCT_VALUE_INT) {
		rt_error(env, N_TR("Value is not an integer."));
		return false;
	}

	/* Get the value. */
	*i = val->val.i;

	return true;
}

NOCT_DLL
bool
noct_get_float(
	NoctEnv *env,
	NoctValue *val,
	float *f)
{
	assert(env != NULL);
	assert(val != NULL);
	assert(f != NULL);

	/* Check the type. */
	if (val->type != NOCT_VALUE_FLOAT) {
		rt_error(env, N_TR("Value is not a float."));
		return false;
	}

	/* Get the value. */
	*f = val->val.f;

	return true;
}

NOCT_DLL
bool
noct_get_string_len(
	NoctEnv *env,
	NoctValue *val,
	size_t *len)
{
	assert(env != NULL);
	assert(val != NULL);
	assert(len != NULL);

	/* Check the type. */
	if (val->type != NOCT_VALUE_STRING) {
		rt_error(env, N_TR("Value is not a string."));
		return false;
	}

	/* Get the size. */
	*len = val->val.str->len;

	return true;
}

NOCT_DLL
bool
noct_get_string(
	NoctEnv *env,
	NoctValue *val,
	const char **s)
{
	assert(env != NULL);
	assert(val != NULL);

	/* Check the type. */
	if (val->type != NOCT_VALUE_STRING) {
		rt_error(env, N_TR("Value is not a string."));
		return false;
	}

	/* Get the pointer. */
	*s = val->val.str->data;

	return true;
}

NOCT_DLL
bool
noct_get_func(
	NoctEnv *env,
	NoctValue *val,
	NoctFunc **f)
{
	assert(env != NULL);
	assert(val != NULL);
	assert(f != NULL);

	/* Check the type. */
	if (val->type != NOCT_VALUE_FUNC) {
		rt_error(env, N_TR("Value is not a function."));
		return false;
	}

	/* Get the function. */
	*f = val->val.func;

	return true;
}

NOCT_DLL
bool
noct_get_array_size(
	NoctEnv *env,
	NoctValue *val,
	uint32_t *size)
{
	assert(env != NULL);
	assert(val != NULL);
	assert(size != NULL);

	/* Check the type. */
	if (val->type != NOCT_VALUE_ARRAY) {
		rt_error(env, N_TR("Value is not an array."));
		return false;
	}

	/* Get the array size. */
	if (!rt_get_array_size(env, val->val.arr, size))
		return false;

	return true;
}

NOCT_DLL
bool
noct_get_array_elem(
	NoctEnv *env,
	NoctValue *array,
	uint32_t index,
	NoctValue *val)
{
	assert(env != NULL);
	assert(array != NULL);
	assert(index >= 0);
	assert(val != NULL);

	/* Check the type. */
	if (array->type != NOCT_VALUE_ARRAY) {
		rt_error(env, N_TR("Value is not an array."));
		return false;
	}

	/* Get the array element. */
	if (!rt_get_array_elem(env, array->val.arr, index, val))
		return false;

	return true;
}

NOCT_DLL
bool
noct_set_array_elem(
	NoctEnv *env,
	NoctValue *array,
	uint32_t index,
	NoctValue *val)
{
	assert(env != NULL);
	assert(array != NULL);
	assert(index >= 0);
	assert(val != NULL);

	/* Check the type. */
	if (array->type != NOCT_VALUE_ARRAY) {
		rt_error(env, N_TR("Not an array."));
		return false;
	}

	/* Set the array element. */
	if (!rt_set_array_elem(env, &array->val.arr, index, val))
		return false;

	return true;
}

NOCT_DLL
bool
noct_resize_array(
	NoctEnv *env,
	NoctValue *array,
	uint32_t size)
{
	assert(env != NULL);
	assert(array != NULL);

	/* Check the type. */
	if (array->type != NOCT_VALUE_ARRAY) {
		rt_error(env, N_TR("Not an array."));
		return false;
	}

	/* Resize the array. */
	if (!rt_resize_array(env, &array->val.arr, size))
		return false;

	return true;
}

NOCT_DLL
bool
noct_make_array_copy(
	NoctEnv *env,
	NoctValue *dst,
	NoctValue *src)
{
	assert(env != NULL);
	assert(dst != NULL);
	assert(src != NULL);

	/* Check the type. */
	if (src->type != NOCT_VALUE_ARRAY) {
		rt_error(env, N_TR("Not an array."));
		return false;
	}

	dst->type = NOCT_VALUE_ARRAY;

	/* Make a shallow copy of the source array. */
	if (!rt_make_array_copy(env, &dst->val.arr, src->val.arr)) {
		/* Failed. Invalidate the value for safety. */
		dst->type = NOCT_VALUE_INT;
		return false;
	}

	return true;
}

NOCT_DLL
bool
noct_get_dict_size(
	NoctEnv *env,
	NoctValue *dict,
	uint32_t *size)
{
	assert(env != NULL);
	assert(dict != NULL);
	assert(size != NULL);

	/* Check the type. */
	if (dict->type != NOCT_VALUE_DICT) {
		rt_error(env, N_TR("Not a dictionary."));
		return false;
	}

	/* Get the dictionary size. */
	if (!rt_get_dict_size(env, dict->val.dict, size))
		return false;

	return true;
}

NOCT_DLL
bool
noct_get_dict_key_by_index(
	NoctEnv *env,
	NoctValue *dict,
	uint32_t index,
	NoctValue *key)
{
	assert(env != NULL);
	assert(dict != NULL);
	assert(index >= 0);
	assert(key != NULL);
	
	/* Check the type. */
	if (dict->type != NOCT_VALUE_DICT) {
		rt_error(env, N_TR("Not a dictionary."));
		return false;
	}

	/* Load the key. */
	if (!rt_get_dict_key_by_index(env, dict->val.dict, index, key))
		return false;

	return true;
}

NOCT_DLL
bool
noct_get_dict_value_by_index(
	NoctEnv *env,
	NoctValue *dict,
	uint32_t index,
	NoctValue *val)
{
	assert(env != NULL);
	assert(dict != NULL);
	assert(index >= 0);
	assert(val != NULL);
	
	/* Check the type. */
	if (dict->type != NOCT_VALUE_DICT) {
		rt_error(env, N_TR("Not a dictionary."));
		return false;
	}

	/* Load the value. */
	if (!rt_get_dict_value_by_index(env, dict->val.dict, index, val))
		return false;

	return true;
}

NOCT_DLL
bool
noct_check_dict_key(
	NoctEnv *env,
	NoctValue *dict,
	const char *key,
	bool *ret)
{
	assert(env != NULL);
	assert(dict != NULL);
	assert(key != NULL);
	assert(ret != NULL);
	
	/* Check the type. */
	if (dict->type != NOCT_VALUE_DICT) {
		rt_error(env, N_TR("Not a dictionary."));
		return false;
	}

	/* Search the key. */
	if (!rt_check_dict_key(env, dict->val.dict, key, ret))
		return false;

	return true;
}

NOCT_DLL
bool
noct_get_dict_elem(
	NoctEnv *env,
	NoctValue *dict,
	const char *key,
	NoctValue *val)
{
	assert(env != NULL);
	assert(dict != NULL);
	assert(key != NULL);
	assert(val != NULL);
	
	/* Check the type. */
	if (dict->type != NOCT_VALUE_DICT) {
		rt_error(env, N_TR("Not a dictionary."));
		return false;
	}

	/* Get. */
	if (!rt_get_dict_elem(env, dict->val.dict, key, val))
		return false;

	return true;
}

NOCT_DLL
bool
noct_set_dict_elem(
	NoctEnv *env,
	NoctValue *dict,
	const char *key,
	NoctValue *val)
{
	assert(env != NULL);
	assert(dict != NULL);
	assert(key != NULL);
	assert(val != NULL);
	
	/* Check the type. */
	if (dict->type != NOCT_VALUE_DICT) {
		rt_error(env, N_TR("Not a dictionary."));
		return false;
	}

	/* Set a value. */
	if (!rt_set_dict_elem(env, &dict->val.dict, key, val))
		return false;

	return true;
}

/*
 * The following is not thread-safe.
 */
#if !defined(NOCT_USE_MULTITHREAD)
NOCT_DLL
bool
noct_remove_dict_elem(
	NoctEnv *env,
	NoctValue *dict,
	const char *key)
{
	assert(env != NULL);
	assert(dict != NULL);
	assert(key != NULL);
	
	/* Check the type. */
	if (dict->type != NOCT_VALUE_DICT) {
		rt_error(env, N_TR("Not a dictionary."));
		return false;
	}

	/* Remove the element. */
	if (!rt_remove_dict_elem(env, dict->val.dict, key))
		return false;

	return true;
}
#endif

NOCT_DLL
bool
noct_make_dict_copy(
	NoctEnv *env,
	NoctValue *dst,
	NoctValue *src)
{
	assert(env != NULL);
	assert(src != NULL);
	assert(dst != NULL);

	/* Check the type. */
	if (src->type != NOCT_VALUE_DICT) {
		rt_error(env, N_TR("Not a dictionary."));
		return false;
	}

	dst->type = NOCT_VALUE_DICT;

	if (!rt_make_dict_copy(env, &dst->val.dict, src->val.dict)) {
		/* Failed. Invalidate the value for safety. */
		dst->type = NOCT_VALUE_INT;
		return false;
	}

	return true;
}

NOCT_DLL
bool
noct_get_arg(
	NoctEnv *env,
	uint32_t index,
	NoctValue *val)
{
	assert(env != NULL);
	assert(index >= 0);
	assert(val != NULL);

	*val = env->frame->tmpvar[index];

	return true;
}

NOCT_DLL
bool
noct_set_return(
	NoctEnv *env,
	NoctValue *val)
{
	assert(env != NULL);
	assert(val != NULL);

	env->frame->tmpvar[0] = *val;

	return true;
}

NOCT_DLL
bool
noct_check_global(
	NoctEnv *env,
	const char *name)
{
	assert(env != NULL);
	assert(name != NULL);

	if (!rt_check_global(env, name))
		return false;

	return true;
}

NOCT_DLL
bool
noct_get_global(
	NoctEnv *env,
	const char *name,
	NoctValue *val)
{
	assert(env != NULL);
	assert(name != NULL);
	assert(val != NULL);

	if (!rt_get_global(env, name, val))
		return false;

	return true;
}

NOCT_DLL
bool
noct_set_global(
	NoctEnv *env,
	const char *name,
	NoctValue *val)
{
	assert(env != NULL);
	assert(name != NULL);
	assert(val != NULL);

	if (!rt_set_global(env, name, val))
		return false;

	return true;
}

NOCT_DLL
bool
noct_pin_global(
	NoctEnv *env,
	NoctValue *val)
{
	assert(env != NULL);
	assert(val != NULL);

	if (!rt_pin_global(env, val))
		return false;

	return true;
}

NOCT_DLL
bool
noct_unpin_global(
	NoctEnv *env,
	struct rt_value *val)
{
	assert(env != NULL);
	assert(val != NULL);

	if (!rt_unpin_global(env, val))
		return false;

	return true;
}

NOCT_DLL
bool
noct_pin_local(
	NoctEnv *env,
	uint32_t count,
	...)
{
	va_list ap;
	uint32_t i;
	struct rt_value *val;

	assert(env != NULL);
	assert(count > 0);

	va_start(ap, count);
	for (i = 0; i < count; i++) {
		val = va_arg(ap, struct rt_value *);
		if (!rt_pin_local(env, val))
			return false;
	}
	va_end(ap);

	return true;
}

NOCT_DLL
bool
noct_unpin_local(
	NoctEnv *env,
	uint32_t count,
	...)
{
	va_list ap;
	uint32_t i;
	struct rt_value *val;

	assert(env != NULL);
	assert(count > 0);

	va_start(ap, count);
	for (i = 0; i < count; i++) {
		val = va_arg(ap, struct rt_value *);
		if (!rt_unpin_local(env, val))
			return false;
	}
	va_end(ap);

	return true;
}

NOCT_DLL
void
noct_fast_gc(
	NoctEnv *env)
{
	rt_gc_level1_gc(env);
}

NOCT_DLL
void
noct_full_gc(
	NoctEnv *env)
{
	rt_gc_level2_gc(env);
}

NOCT_DLL
void
noct_compact_gc(
	NoctEnv *env)
{
	rt_gc_level3_gc(env);
}

NOCT_DLL
bool
noct_get_heap_usage(
	NoctEnv *env,
	size_t *ret)
{
	/* TODO: */
	*ret = 0;

	return true;
}

NOCT_DLL
void
noct_error(
	NoctEnv *env,
	const char *msg,
	...)
{
	va_list ap;
	char tmp[1024];

	va_start(ap, msg);
	vsnprintf(tmp, sizeof(tmp), msg, ap);
	va_end(ap);

	rt_error(env, "%s", tmp);
}

NOCT_DLL
void
noct_out_of_memory(
	NoctEnv *env)
{
	noct_error(env, "Out-of-memory.");
}

/*
 * Convenience Functions
 */

/*
 * String
 */

NOCT_DLL
bool
noct_make_string_format(
	NoctEnv *env,
	NoctValue *val,
	const char *s,
	...)
{
	va_list ap;
	char tmp[1024];

	assert(env != NULL);
	assert(val != NULL);
	assert(s != NULL);

	va_start(ap, s);
	vsnprintf(tmp, sizeof(tmp), s, ap);
	va_end(ap);

	if (!rt_make_string(env, val, tmp))
		return false;

	return true;
}

/*
 * Array Getters
 */

NOCT_DLL
bool
noct_get_array_elem_check_int(
	NoctEnv *env,
	NoctValue *array,
	uint32_t index,
	NoctValue *val,
	int *i)
{
	int type;

	assert(env != NULL);
	assert(array != NULL);
	assert(index >= 0);
	assert(i != NULL);

	/* Get the element. */
	if (!noct_get_array_elem(env, array, index, val))
		return false;

	/* Check the element value type. */
	if (!noct_get_value_type(env, val, &type))
		return false;
	if (type != NOCT_VALUE_INT) {
		rt_error(env, N_TR("Element %d is not an integer."), index);
		return false;
	}

	/* Get the value. */
	*i = val->val.i;

	return true;
}

NOCT_DLL
bool
noct_get_array_elem_check_float(
	NoctEnv *env,
	NoctValue *array,
	uint32_t index,
	NoctValue *val,
	float *f)
{
	int type;

	assert(env != NULL);
	assert(array != NULL);
	assert(index >= 0);
	assert(f != NULL);

	/* Get the element. */
	if (!noct_get_array_elem(env, array, index, val))
		return false;

	/* Check the element value type. */
	if (!noct_get_value_type(env, val, &type))
		return false;
	if (type != NOCT_VALUE_FLOAT) {
		rt_error(env, N_TR("Element %d is not a float."), index);
		return false;
	}

	/* Get the value. */
	*f = val->val.f;

	return true;
}

NOCT_DLL
bool
noct_get_array_elem_check_string(
	NoctEnv *env,
	NoctValue *array,
	uint32_t index,
	NoctValue *val,
	const char **data)
{
	int type;

	assert(env != NULL);
	assert(array != NULL);
	assert(index >= 0);
	assert(val != NULL);
	assert(data != NULL);

	/* Get the element. */
	if (!noct_get_array_elem(env, array, index, val))
		return false;

	/* Check the element value type. */
	if (!noct_get_value_type(env, val, &type))
		return false;
	if (type != NOCT_VALUE_STRING) {
		rt_error(env, N_TR("Element %d is not a string."), index);
		return false;
	}

	/* Get the value. */
	*data = val->val.str->data;

	return true;
}

NOCT_DLL
bool
noct_get_array_elem_check_array(
	NoctEnv *env,
	NoctValue *array,
	uint32_t index,
	NoctValue *val)
{
	assert(env != NULL);
	assert(array != NULL);
	assert(index >= 0);
	assert(val != NULL);

	/* Get the element. */
	if (!noct_get_array_elem(env, array, index, val))
		return false;

	/* Check the element value type. */
	if (val->type != NOCT_VALUE_ARRAY) {
		rt_error(env, N_TR("Element %d is not an array."), index);
		return false;
	}

	return true;
}

NOCT_DLL
bool
noct_get_array_elem_dict(
	NoctEnv *env,
	NoctValue *array,
	int index,
	NoctValue *val)
{
	assert(env != NULL);
	assert(array != NULL);
	assert(index >= 0);
	assert(val != NULL);

	/* Get the element. */
	if (!noct_get_array_elem(env, array, index, val))
		return false;

	/* Check the element value type. */
	if (val->type != NOCT_VALUE_ARRAY) {
		rt_error(env, N_TR("Element %d is not a dictionary."), index);
		return false;
	}

	return true;
}

NOCT_DLL
bool
noct_get_array_elem_check_func(
	NoctEnv *env,
	NoctValue *array,
	uint32_t index,
	NoctValue *val,
	NoctFunc **f)
{
	assert(env != NULL);
	assert(array != NULL);
	assert(index >= 0);
	assert(val != NULL);

	/* Get the element. */
	if (!noct_get_array_elem(env, array, index, val))
		return false;

	/* Check the element value type. */
	if (val->type != NOCT_VALUE_FUNC) {
		rt_error(env, N_TR("Element %d is not a function."), index);
		return false;
	}

	/* Get the value. */
	*f = val->val.func;

	return true;
}

/*
 * Array Setters
 */

NOCT_DLL
bool
noct_set_array_elem_make_int(
	NoctEnv *env,
	NoctValue *array,
	int index,
	NoctValue *val,
	int i)
{
	assert(env != NULL);
	assert(array != NULL);
	assert(index >= 0);
	assert(val != NULL);

	/* Make an integer value. */
	val->type = NOCT_VALUE_INT;
	val->val.i = i;

	/* Get the element. */
	if (!noct_set_array_elem(env, array, index, val))
		return false;

	return true;
}

NOCT_DLL
bool
noct_set_array_elem_make_float(
	NoctEnv *env,
	NoctValue *array,
	int index,
	NoctValue *val,
	float f)
{
	assert(env != NULL);
	assert(array != NULL);
	assert(index >= 0);
	assert(val != NULL);

	/* Make an integer value. */
	val->type = NOCT_VALUE_FLOAT;
	val->val.f = f;

	/* Get the element. */
	if (!noct_set_array_elem(env, array, index, val))
		return false;
	
	return true;
}

NOCT_DLL
bool
noct_set_array_elem_make_string(
	NoctEnv *env,
	NoctValue *array,
	int index,
	NoctValue *val,
	const char *data)
{
	assert(env != NULL);
	assert(array != NULL);
	assert(index >= 0);
	assert(val != NULL);
	assert(data != NULL);

	/* Make an integer value. */
	if (!rt_make_string(env, val, data))
		return false;

	/* Get the element. */
	if (!noct_set_array_elem(env, array, index, val))
		return false;
	
	return true;
}

/*
 * Dictionary Getters
 */

NOCT_DLL
bool
noct_get_dict_elem_check_int(
	NoctEnv *env,
	NoctValue *dict,
	const char *key,
	NoctValue *val,
	int *i)
{
	assert(env != NULL);
	assert(dict != NULL);
	assert(key != NULL);
	assert(i != NULL);
	assert(val != NULL);

	/* Get the element. */
	if (!noct_get_dict_elem(env, dict, key, val))
		return false;

	/* Check the element value type. */
	if (val->type != NOCT_VALUE_INT) {
		rt_error(env, N_TR("Element %d is not an integer."), i);
		return false;
	}

	/* Get the value. */
	*i = val->val.i;

	return true;
}

NOCT_DLL
bool
noct_get_dict_elem_check_float(
	NoctEnv *env,
	NoctValue *dict,
	const char *key,
	NoctValue *val,
	float *f)
{
	assert(env != NULL);
	assert(dict != NULL);
	assert(key != NULL);
	assert(f != NULL);
	assert(val != NULL);

	/* Get the element. */
	if (!noct_get_dict_elem(env, dict, key, val))
		return false;

	/* Check the element value type. */
	if (val->type != NOCT_VALUE_FLOAT) {
		rt_error(env, N_TR("Value for key %s is not a float."), key);
		return false;
	}

	/* Get the value. */
	*f = val->val.f;

	return true;
}

NOCT_DLL
bool
noct_get_dict_elem_check_string(
	NoctEnv *env,
	NoctValue *dict,
	const char *key,
	NoctValue *val,
	const char **data)
{
	assert(env != NULL);
	assert(dict != NULL);
	assert(key != NULL);
	assert(val != NULL);
	assert(data != NULL);

	/* Get the element. */
	if (!noct_get_dict_elem(env, dict, key, val))
		return false;

	/* Check the element value type. */
	if (val->type != NOCT_VALUE_STRING) {
		rt_error(env, N_TR("Value for key %s is not a string."), key);
		return false;
	}

	/* Get the value. */
	*data = val->val.str->data;

	return true;
}

NOCT_DLL
bool
noct_get_dict_elem_check_array(
	NoctEnv *env,
	NoctValue *dict,
	const char *key,
	NoctValue *val)
{
	assert(env != NULL);
	assert(dict != NULL);
	assert(key != NULL);
	assert(val != NULL);

	/* Get the element. */
	if (!noct_get_dict_elem(env, dict, key, val))
		return false;

	/* Check the element value type. */
	if (val->type != NOCT_VALUE_ARRAY) {
		rt_error(env, N_TR("Value for key %s is not an array."), key);
		return false;
	}

	return true;
}

NOCT_DLL
bool
noct_get_dict_elem_check_dict(
	NoctEnv *env,
	NoctValue *dict,
	const char *key,
	NoctValue *val)
{
	assert(env != NULL);
	assert(dict != NULL);
	assert(key != NULL);
	assert(val != NULL);

	/* Get the element. */
	if (!noct_get_dict_elem(env, dict, key, val))
		return false;

	/* Check the element value type. */
	if (val->type != NOCT_VALUE_DICT) {
		rt_error(env, N_TR("Value for key %s is not a dictionary."), key);
		return false;
	}

	return true;
}

NOCT_DLL
bool
noct_get_dict_elem_check_func(
	NoctEnv *env,
	NoctValue *dict,
	const char *key,
	NoctValue *val,
	NoctFunc **f)
{
	assert(env != NULL);
	assert(dict != NULL);
	assert(key != NULL);
	assert(f != NULL);
	assert(val != NULL);

	/* Get the element. */
	if (!noct_get_dict_elem(env, dict, key, val))
		return false;

	/* Check the element value type. */
	if (val->type != NOCT_VALUE_FUNC) {
		rt_error(env, N_TR("Value for key %s is not a function."), key);
		return false;
	}

	/* Get the value. */
	*f = val->val.func;

	return true;
}

/*
 * Dictionary Setters
 */

NOCT_DLL
bool
noct_set_dict_elem_make_int(
	NoctEnv *env,
	NoctValue *dict,
	const char *key,
	NoctValue *val,
	int i)
{
	assert(env != NULL);
	assert(dict != NULL);
	assert(key != NULL);
	assert(val != NULL);

	/* Make an integer value. */
	val->type = NOCT_VALUE_INT;
	val->val.i = i;

	/* Set the key-value pair. */
	if (!noct_set_dict_elem(env, dict, key, val))
		return false;

	return true;
}

NOCT_DLL
bool
noct_set_dict_elem_make_float(
	NoctEnv *env,
	NoctValue *dict,
	const char *key,
	NoctValue *val,
	float f)
{
	assert(env != NULL);
	assert(dict != NULL);
	assert(key != NULL);
	assert(val != NULL);

	/* Make a float value. */
	val->type = NOCT_VALUE_FLOAT;
	val->val.f = f;

	/* Set the key-value pair. */
	if (!noct_set_dict_elem(env, dict, key, val))
		return false;

	return true;
}

NOCT_DLL
bool
noct_set_dict_elem_make_string(
	NoctEnv *env,
	NoctValue *dict,
	const char *key,
	NoctValue *val,
	const char *data)
{
	assert(env != NULL);
	assert(dict != NULL);
	assert(key != NULL);
	assert(val != NULL);
	assert(data != NULL);

	/* Make a string value. */
	if (!rt_make_string(env, val, data))
		return false;

	/* Set the key-value pair. */
	if (!noct_set_dict_elem(env, dict, key, val))
		return false;

	return true;
}

/*
 * Argument Getters
 */

NOCT_DLL
bool
noct_get_arg_check_int(
	NoctEnv *env,
	int index,
	NoctValue *val,
	int *i)
{
	assert(env != NULL);
	assert(index >= 0);
	assert(val != NULL);
	assert(i != NULL);

	/* Get the argument. */
	if (!noct_get_arg(env, index, val))
		return false;

	/* Check the value type. */
	if (val->type != NOCT_VALUE_INT) {
		rt_error(env, N_TR("Argument (%d: %s) not an integer."),
			 index,
			 env->frame->func->param_name[index]);
		return false;
	}

	/* Get the integer. */
	*i = val->val.i;

	return true;
}

NOCT_DLL
bool
noct_get_arg_check_float(
	NoctEnv *env,
	int index,
	NoctValue *val,
	float *f)
{
	assert(env != NULL);
	assert(index >= 0);
	assert(val != NULL);
	assert(f != NULL);

	/* Get the argument. */
	if (!noct_get_arg(env, index, val))
		return false;

	/* Check the value type. */
	if (val->type != NOCT_VALUE_FLOAT) {
		rt_error(env, N_TR("Argument (%d: %s) not a float."),
			 index,
			 env->frame->func->param_name[index]);
		return false;
	}

	/* Get the float. */
	*f = val->val.f;

	return true;
}

NOCT_DLL
bool
noct_get_arg_check_string(
	NoctEnv *env,
	int index,
	NoctValue *val,
	const char **data)
{
	assert(env != NULL);
	assert(index >= 0);
	assert(val != NULL);
	assert(data != NULL);

	/* Get the argument. */
	if (!noct_get_arg(env, index, val))
		return false;

	/* Check the value type. */
	if (val->type != NOCT_VALUE_STRING) {
		rt_error(env, N_TR("Argument (%d: %s) not a string."),
			 index,
			 env->frame->func->param_name[index]);
		return false;
	}

	/* Get the string. */
	*data = val->val.str->data;

	return true;
}

NOCT_DLL
bool
noct_get_arg_check_array(
	NoctEnv *env,
	int index,
	NoctValue *val)
{
	assert(env != NULL);
	assert(index >= 0);
	assert(val != NULL);

	/* Get the argument. */
	if (!noct_get_arg(env, index, val))
		return false;

	/* Check the value type. */
	if (val->type != NOCT_VALUE_ARRAY) {
		rt_error(env, N_TR("Argument (%d: %s) not an array."),
			 index,
			 env->frame->func->param_name[index]);
		return false;
	}

	return true;
}


NOCT_DLL
bool
noct_get_arg_check_dict(
	NoctEnv *env,
	int index,
	NoctValue *val)
{
	assert(env != NULL);
	assert(index >= 0);
	assert(val != NULL);

	/* Get the argument. */
	if (!noct_get_arg(env, index, val))
		return false;

	/* Check the value type. */
	if (val->type != NOCT_VALUE_DICT) {
		rt_error(env, N_TR("Argument (%d: %s) not a dictionary."),
			 index,
			 env->frame->func->param_name[index]);
		return false;
	}

	return true;
}

NOCT_DLL
bool
noct_get_arg_func(
	NoctEnv *env,
	int index,
	NoctValue *val,
	NoctFunc **f)
{
	assert(env != NULL);
	assert(index >= 0);
	assert(val != NULL);
	assert(f != NULL);

	/* Get the argument. */
	if (!noct_get_arg(env, index, val))
		return false;

	/* Check the value type. */
	if (val->type != NOCT_VALUE_FUNC) {
		rt_error(env, N_TR("Argument (%d: %s) not a function."),
			 index,
			 env->frame->func->param_name[index]);
		return false;
	}

	/* Get the function. */
	*f = val->val.func;

	return true;
}

/*
 * Return Setter
 */

NOCT_DLL
bool
noct_set_return_make_int(
	NoctEnv *env,
	NoctValue *val,
	int i)
{
	assert(env != NULL);
	assert(val != NULL);

	/* Make an integer value. */
	val->type = NOCT_VALUE_INT;
	val->val.i = i;

	/* Set the return value. */
	if (!noct_set_return(env, val))
		return false;

	return true;
}

NOCT_DLL
bool
noct_set_return_make_float(
	NoctEnv *env,
	NoctValue *val,
	float f)
{
	assert(env != NULL);
	assert(val != NULL);

	/* Make a float value. */
	val->type = NOCT_VALUE_FLOAT;
	val->val.f = f;

	/* Set the return value. */
	if (!noct_set_return(env, val))
		return false;

	return true;
}

NOCT_DLL
bool
noct_set_return_make_string(
	NoctEnv *env,
	NoctValue *val,
	const char *data)
{
	assert(env != NULL);
	assert(val != NULL);
	assert(data != NULL);

	/* Make a string value. */
	if (!rt_make_string(env, val, data))
		return false;

	/* Set the return value. */
	if (!noct_set_return(env, val))
		return false;

	return true;
}
