/* -*- coding: utf-8; tab-width: 8; indent-tabs-mode: t; -*- */

/*
 * Copyright (c) 2025, Awe Morris. All rights reserved.
 */

/*
 * Noct Virtual Machine Public Interface
 */

#ifndef NOCT_NOCT_H
#define NOCT_NOCT_H

#include <noct/c89compat.h>

/*
 * Definition of the import/export keyword.
 */
#if defined(USE_DLL)
#if defined(__GNUC__)
#define NOCT_DLL		__attribute__((visibility("default")))
#elif defined(_MSC_VER)
#if defined(DLL_IMPL)
#define NOCT_DLL		__declspec(dllexport)
#else
#define NOCT_DLL		__declspec(dllimport)
#endif
#endif
#else
#define NOCT_DLL
#endif

/*
 * Maximum arguments of a call.
 */
#define NOCT_ARG_MAX		32

/*
 * Bytecode File Header
 */
#define NOCT_BYTECODE_HEADER	"Noct Bytecode"

/*
 * Forward declaration of internal structs.
 */
struct rt_runtime;
struct rt_env;
struct rt_frame;
struct rt_gc_object;
struct rt_value;
struct rt_string;
struct rt_array;
struct rt_dict;
struct rt_func;

/*
 * Friendly-names.
 */
typedef struct rt_vm     NoctVM;
typedef struct rt_env    NoctEnv;
typedef struct rt_value  NoctValue;
typedef struct rt_func   NoctFunc;

/*
 * Value type.
 */
enum noct_value_type {
	NOCT_VALUE_INT    = 0,
	NOCT_VALUE_FLOAT  = 1,
	NOCT_VALUE_STRING = 2,
	NOCT_VALUE_ARRAY  = 3,
	NOCT_VALUE_DICT   = 4,
	NOCT_VALUE_FUNC   = 5,
};

/*
 * Zero value.
 */
#define NOCT_ZERO	{0, 0}

/*
 * NoctValue implementation.
 *  - If a value is zero-cleared, it shows an integer zero.
 *  - This struct has 8 bytes on 32-bit architecture and 16 bytes on 64-bit architecture.
 */
struct rt_value {
	/* Offset 0: */
	int type;

	/* 32-bit padding for 64-bit architecture excluding MIPS64. */
#if defined(ARCH_ARM64) || defined(ARCH_X86_64) || defined(ARCH_PPC64)
	int padding;
#endif

	/* Offset 4 in 32-bit, 8 in 64-bit: */
	union {
		int i;
		float f;
		struct rt_string *str;
		struct rt_array *arr;
		struct rt_dict *dict;
		struct rt_func *func;
		struct rt_gc_object *obj;
	} val;
};

/*
 * Allocator
 */

#define noct_malloc	malloc
#define noct_calloc	calloc
#define noct_strdup	strdup
#define noct_free	free

/*
 * Core Functions
 */

/*
 * Creates a new VM instance.
 *
 * Also returns a pointer to an environment for the calling thread.
 */
NOCT_DLL
bool
noct_create_vm(
	NoctVM **vm,
	NoctEnv **default_env);

/*
 * Destroys the given VM instance and releases associated resources.
 */
NOCT_DLL
bool
noct_destroy_vm(
	NoctVM *vm);

/*
 * Creates a thread-local environment for the current thread.
 */
#if defined(NOCT_USE_MULTITHREAD)
NOCT_DLL
bool
noct_create_thread_env(
	NoctEnv *prev_evm,
	NoctEnv **new_env);
#endif

/*
 * Registers functions from a source code text.
 */
NOCT_DLL
bool
noct_register_source(
	NoctEnv *env,
	const char *file_name,
	const char *source_text);

/*
 * Registers functions from compiled bytecode data.
 */
NOCT_DLL
bool
noct_register_bytecode(
	NoctEnv *env,
	uint8_t *data,
	size_t size);

/*
 * Registers an FFI function.
 */
NOCT_DLL
bool
noct_register_cfunc(
	NoctEnv *env,
	const char *name,
	int param_count,
	const char *param_name[],
	bool (*cfunc)(NoctEnv *env),
	NoctFunc **ret_func);

/*
 * Enters the VM in the current thread and invokes a function by name.
 *
 * Executes the specified function with the given arguments and stores
 * the results in the provided return value slot.
 */
NOCT_DLL
bool
noct_enter_vm(
	NoctEnv *env,
	const char *func_name,
	int arg_count,
	NoctValue *arg,
	NoctValue *ret);

/*
 * Retrieves the file name where the last error occurred.
 */
NOCT_DLL
bool
noct_get_error_file(
	NoctEnv *env,
	const char **msg);

/*
 * Retrieves the line number where the last error occurred.
 */
NOCT_DLL
bool
noct_get_error_line(
	NoctEnv *env,
	int *line);

/*
 * Retrieves the error message where the last error occurred.
 */
NOCT_DLL
bool
noct_get_error_message(
	NoctEnv *env,
	const char **msg);

/*
 * Calls a function.
 */
NOCT_DLL
bool
noct_call(
	NoctEnv *env,
	NoctFunc *func,
	int arg_count,
	NoctValue *arg,
	NoctValue *ret);

/*
 * Makes an integer value.
 */
NOCT_DLL
bool
noct_make_int(
	NoctEnv *env,
	NoctValue *val,
	int i);

/*
 * Makes a float value.
 */
NOCT_DLL
bool
noct_make_float(
	NoctEnv *env,
	NoctValue *val,
	float f);

/*
 * Makes a string value.
 */
NOCT_DLL
bool
noct_make_string(
	NoctEnv *env,
	NoctValue *val,
	const char *data);

/*
 * Makes a string value with a format.
 */
NOCT_DLL
bool
noct_make_string_format(
	NoctEnv *env,
	NoctValue *val,
	const char *s,
	...);

/*
 * Makes an empty array value.
 */
NOCT_DLL
bool
noct_make_empty_array(
	NoctEnv *env,
	NoctValue *val);

/*
 * Makes an empty dictionary value.
 */
NOCT_DLL
bool
noct_make_empty_dict(
	NoctEnv *env,
	NoctValue *val);

/*
 * Retrieves the type tag of a value.
 *
 * The result is one of the NOCT_VALUE_* constants.
 */
NOCT_DLL
bool
noct_get_value_type(
	NoctEnv *env,
	NoctValue *val,
	int *type);

/*
 * Retrieves an integer from a value with type checking.
 *
 * Fails if the given value is not of integer type.
 */
NOCT_DLL
bool
noct_get_int(
	NoctEnv *env,
	NoctValue *val,
	int *i);

/*
 * Retrieves a float from a value with type checking.
 *
 * Fails if the given value is not of float type.
 */
NOCT_DLL
bool
noct_get_float(
	NoctEnv *env,
	NoctValue *val,
	float *f);

/*
 * Retrieves the length of the string in a value with type checking.
 *
 * Fails if the given value is not of string type.
 */
NOCT_DLL
bool
noct_get_string_len(
	NoctEnv *env,
	NoctValue *val,
	size_t *len);

/*
 * Retrieves the string from a value with type checking.
 *
 * Fails if the given value is not of string type.
 */
NOCT_DLL
bool
noct_get_string(
	NoctEnv *env,
	NoctValue *val,
	const char **s);

/*
 * Retrieves the function from a value with type checking.
 *
 * Fails if the given value is not of function type.
 */
NOCT_DLL
bool
noct_get_func(
	NoctEnv *env,
	NoctValue *val,
	NoctFunc **ret);

/*
 * Arrays
 */

/*
 * Retrieves the number of elements in an array.
 *
 * Fails if the given value is not an array.
 */
NOCT_DLL
bool
noct_get_array_size(
	NoctEnv *env,
	NoctValue *val,
	uint32_t *size);

/*
 * Retrieves an element from an array without type checking.
 *
 * The element at the given index is returned as a NoctValue,
 * regardless of its actual type.
 */
NOCT_DLL
bool
noct_get_array_elem(
	NoctEnv *env,
	NoctValue *array,
	uint32_t index,
	NoctValue *val);

/*
 * Sets an element in an array at the specified index.
 *
 * The value is passed as a NoctValue and may be of any type.
 * If the index is out of bounds, the array is automatically expanded.
 * The existing element at the index, if any, will be overwritten.
 */
NOCT_DLL
bool
noct_set_array_elem(
	NoctEnv *env,
	NoctValue *array,
	uint32_t index,
	NoctValue *val);

/*
 * Resizes an array to the specified size.
 *
 * If the array is shrunk, elements beyond the new size are removed.
 * If the array is expanded, new elements are initialized to integer zero.
 */
NOCT_DLL
bool
noct_resize_array(
	NoctEnv *env,
	NoctValue *arr,
	uint32_t size);

/*
 * Creates a shallow copy of an array.
 */
NOCT_DLL
bool
noct_make_array_copy(
	NoctEnv *env,
	NoctValue *dst,
	NoctValue *src);

/*
 * Retrieves the number of key-value pairs in a dictionary.
 */
NOCT_DLL
bool
noct_get_dict_size(
	NoctEnv *env,
	NoctValue *dict,
	uint32_t *size);

/*
 * Retrieves a dictionary key by index.
 *
 * This function can be used to traverse dictionary entries in order.
 */
NOCT_DLL
bool
noct_get_dict_key_by_index(
	NoctEnv *env,
	NoctValue *dict,
	uint32_t index,
	NoctValue *key);

/*
 * Retrieves a dictionary value by index.
 *
 * This function can be used to traverse dictionary entries in order.
 */
NOCT_DLL
bool
noct_get_dict_value_by_index(
	NoctEnv *env,
	NoctValue *dict,
	uint32_t index,
	NoctValue *val);

/*
 * Checks whether a key exists in a dictionary.
 */
NOCT_DLL
bool
noct_check_dict_key(
	NoctEnv *env,
	NoctValue *dict,
	const char *key,
	bool *ret);

/*
 * Retrieves a value associated with a key in a dictionary without type checking.
 *
 * The returned value is wrapped by NoctValue and may be of any type.
 * Fails if the key does not exist.
 */
NOCT_DLL
bool
noct_get_dict_elem(
	NoctEnv *env,
	NoctValue *dict,
	const char *key,
	NoctValue *val);

/*
 * Sets a value for a key in a dictionary.
 */
NOCT_DLL
bool
noct_set_dict_elem(
	NoctEnv *env,
	NoctValue *dict,
	const char *key,
	NoctValue *val);

/*
 * Removes a key-value pair from a dictionary by key.
 */
NOCT_DLL
bool
noct_remove_dict_elem(
	NoctEnv *env,
	NoctValue *dict,
	const char *key);

/*
 * Creates a shallow copy of a dictionary.
 */
NOCT_DLL
bool
noct_make_dict_copy(
	NoctEnv *env,
	NoctValue *dst,
	NoctValue *src);

/*
 * Retrieves a function argument from the current stack frame.
 */
NOCT_DLL
bool
noct_get_arg(
	NoctEnv *env,
	uint32_t index,
	NoctValue *val);

/*
 * Sets the return value for the current stack frame.
 */
NOCT_DLL
bool
noct_set_return(
	NoctEnv *env,
	NoctValue *val);

/*
 * Checks wheter a global variable exists or not.
 *
 * Fails if the global variable with the given name does not exist.
 */
NOCT_DLL
bool
noct_check_global(
	NoctEnv *env,
	const char *name);

/*
 * Retrieves the value of a global variable by name.
 *
 * Fails if the global variable with the given name does not exist.
 */
NOCT_DLL
bool
noct_get_global(
	NoctEnv *env,
	const char *name,
	NoctValue *val);

/*
 * Sets a global variable by name.
 */
NOCT_DLL
bool
noct_set_global(
	NoctEnv *env,
	const char *name,
	NoctValue *val);

/*
 * Declares a native global variable for use within an FFI function.
 *
 * This informs the GC that the given NoctValue variable is
 * in use and should not be collected during GC.
 */
NOCT_DLL
bool
noct_pin_global(
	NoctEnv *env,
	NoctValue *val);

/*
 * Undeclares a native global variable for use within an FFI function.
 */
NOCT_DLL
bool
noct_unpin_global(
	NoctEnv *env,
	NoctValue *val);

/*
 * Declares native local variables for use within an FFI function.
 *
 * This informs the GC that the given stack-local NoctValue variables
 * are in use and should not be collected during GC. Returning from an
 * FFI function undeclares the local variables in the current stack.
 *
 * This function should be called with the number of variables,
 * followed by that many NoctValue* arguments.
 */
NOCT_DLL
bool
noct_pin_local(
	NoctEnv *env,
	uint32_t count,
	...);

/*
 * Undeclares a native local variables for use within an FFI function.
 */
NOCT_DLL
bool
noct_unpin_local(
	NoctEnv *env,
	uint32_t count,
	...);

/*
 * Triggers a fast garbage collection pass, typically during a
 * periodic pause such as VSync.
 *
 * This performs a collection of the young generation only.
 */
NOCT_DLL
void
noct_fast_gc(
	NoctEnv *env);

/*
 * Triggers a full, stop-the-world garbage collection.
 *
 * This collects both young and old generations, but does not perform
 * memory compaction.
 */
NOCT_DLL
void
noct_full_gc(
	NoctEnv *env);

/*
 * Triggers a full garbage collection followed by memory compaction.
 *
 * This pass eliminates fragmentation by sliding live objects to
 * create a contiguous free space in the old generation.
 */
NOCT_DLL
void
noct_compact_gc(
	NoctEnv *env);

/*
 * Retrieves the current heap usage, in bytes.
 */
NOCT_DLL
bool
noct_get_heap_usage(
	NoctEnv *env,
	size_t *ret);

/*
 * Writes an error message to the internal buffer.
 *
 * The message can be retrieved later using noct_get_error_message().
 */
NOCT_DLL
void
noct_error(
	NoctEnv *env,
	const char *msg,
	...);

/*
 * Writes the out-of-memory error message to the internal buffer.
 *
 * The message can be retrieved later using noct_get_error_message().
 */
NOCT_DLL
void
noct_out_of_memory(
	NoctEnv *env);

/*
 * Convenience Functions
 */

/* Array Getters */

/*
 * Convenience function to retrieve an integer element from an array
 * with type checking.
 *
 * The `backing_val` parameter must point to a variable previously
 * declared via `noct_declare_c_local()`. This variable serves as a
 * pinned storage location: during the call, it temporarily holds the
 * element's value so that, even if a parallel GC is triggered midway,
 * the value remains protected and stable.
 *
 * Fails if the element at the given index is not of integer type.
 */
NOCT_DLL
bool
noct_get_array_elem_check_int(
	NoctEnv *env,
	NoctValue *array,
	uint32_t index,
	NoctValue *val,
	int *i);

/*
 * Convenience function to retrieve a float element from an array with
 * type checking.
 *
 * The `backing_val` parameter must point to a variable previously
 * declared via `noct_declare_c_local()`. This variable serves as a
 * pinned storage location: during the call, it temporarily holds the
 * element's value so that, even if a parallel GC is triggered midway,
 * the value remains protected and stable.
 *
 * Fails if the element at the given index is not of float type.
 */
NOCT_DLL
bool
noct_get_array_elem_check_float(
	NoctEnv *env,
	NoctValue *array,
	uint32_t index,
	NoctValue *val,
	float *f);


/*
 * Convenience function to retrieve a string element from an array
 * with type checking.
 *
 * The `backing_val` parameter must point to a variable previously
 * declared via `noct_declare_c_local()`. This variable serves as a
 * pinned storage location: during the call, it temporarily holds the
 * element's value so that, even if a parallel GC is triggered midway,
 * the value remains protected and stable.
 *
 * Fails if the element at the given index is not of string type.
 */
NOCT_DLL
bool
noct_get_array_elem_check_string(
	NoctEnv *env,
	NoctValue *array,
	uint32_t index,
	NoctValue *val,
	const char **data);

/*
 * Convenience function to retrieve an array element from an array
 * with type checking.
 *
 * The `backing_val` parameter must point to a variable previously
 * declared via `noct_declare_c_local()`. This variable serves as a
 * pinned storage location: during the call, it temporarily holds the
 * element's value so that, even if a parallel GC is triggered midway,
 * the value remains protected and stable.
 *
 * Fails if the element at the given index is not of array type.
 */
NOCT_DLL
bool
noct_get_array_elem_check_array(
	NoctEnv *env,
	NoctValue *array,
	uint32_t index,
	NoctValue *val);

/*
 * Convenience function to retrieve an array element from a dictionary
 * with type checking.
 *
 * The `backing_val` parameter must point to a variable previously
 * declared via `noct_declare_c_local()`. This variable serves as a
 * pinned storage location: during the call, it temporarily holds the
 * element's value so that, even if a parallel GC is triggered midway,
 * the value remains protected and stable.
 *
 * Fails if the element at the given index is not of dictionary type.
 */
NOCT_DLL
bool
noct_get_array_elem_check_dict(
	NoctEnv *env,
	NoctValue *array,
	uint32_t index,
	NoctValue *val);

/*
 * Convenience function to retrieve a function element from a
 * dictionary with type checking.
 *
 * The `backing_val` parameter must point to a variable previously
 * declared via `noct_declare_c_local()`. This variable serves as a
 * pinned storage location: during the call, it temporarily holds the
 * element's value so that, even if a parallel GC is triggered midway,
 * the value remains protected and stable.
 *
 * Fails if the element at the given index is not of function type.
 */
NOCT_DLL
bool
noct_get_array_elem_check_func(
	NoctEnv *env,
	NoctValue *array,
	uint32_t index,
	NoctValue *val,
	NoctFunc **f);

/*
 * Convenience function to set an integer element in an array.
 *
 * The `backing_val` parameter must point to a variable previously
 * declared via `noct_declare_c_local()`. This variable serves as a
 * pinned storage location: during the call, it temporarily holds the
 * element's value so that, even if a parallel GC is triggered midway,
 * the value remains protected and stable.
 *
 * Semantically equivalent to wrapping the integer in a NoctValue
 * and calling noct_set_array_elem().
 */
NOCT_DLL
bool
noct_set_array_elem_make_int(
	NoctEnv *env,
	NoctValue *array,
	int index,
	NoctValue *val,
	int i);

/*
 * Convenience function to set a float element in an array.
 *
 * The `backing_val` parameter must point to a variable previously
 * declared via `noct_declare_c_local()`. This variable serves as a
 * pinned storage location: during the call, it temporarily holds the
 * element's value so that, even if a parallel GC is triggered midway,
 * the value remains protected and stable.
 *
 * Semantically equivalent to wrapping the float in a NoctValue
 * and calling noct_set_array_elem().
 */
NOCT_DLL
bool
noct_set_array_elem_make_float(
	NoctEnv *env,
	NoctValue *array,
	int index,
	NoctValue *val,
	float f);

/*
 * Convenience function to set a string element in an array.
 *
 * The `backing_val` parameter must point to a variable previously
 * declared via `noct_declare_c_local()`. This variable serves as a
 * pinned storage location: during the call, it temporarily holds the
 * element's value so that, even if a parallel GC is triggered midway,
 * the value remains protected and stable.
 *
 * Semantically equivalent to creating a string NoctValue with
 * noct_make_string() and passing it to noct_set_array_elem().
 */
NOCT_DLL
bool
noct_set_array_elem_make_string(
	NoctEnv *env,
	NoctValue *array,
	int index,
	NoctValue *val,
	const char *data);

/* Dictionaries */

/*
 * Convenience function to retrieve an integer value associated with a
 * key in a dictionary with type checking.
 *
 * The `backing_val` parameter must point to a variable previously
 * declared via `noct_declare_c_local()`. This variable serves as a
 * pinned storage location: during the call, it temporarily holds the
 * element's value so that, even if a parallel GC is triggered midway,
 * the value remains protected and stable.
 *
 * Fails if the key does not exist.
 * Fails if the associated value is not of integer type.
 */
NOCT_DLL
bool
noct_get_dict_elem_check_int(
	NoctEnv *env,
	NoctValue *dict,
	const char *key,
	NoctValue *val,
	int *i);

/*
 * Convenience function to retrieve a float value associated with a
 * key in a dictionary with type checking.
 *
 * The `backing_val` parameter must point to a variable previously
 * declared via `noct_declare_c_local()`. This variable serves as a
 * pinned storage location: during the call, it temporarily holds the
 * element's value so that, even if a parallel GC is triggered midway,
 * the value remains protected and stable.
 *
 * Fails if the key does not exist.
 * Fails if the associated value is not of float type.
 */
NOCT_DLL
bool
noct_get_dict_elem_check_float(
	NoctEnv *env,
	NoctValue *dict,
	const char *key,
	NoctValue *val,
	float *f);

/*
 * Convenience function to retrieve a string value associated with a
 * key in a dictionary with type checking.
 *
 * The `backing_val` parameter must point to a variable previously
 * declared via `noct_declare_c_local()`. This variable serves as a
 * pinned storage location: during the call, it temporarily holds the
 * element's value so that, even if a parallel GC is triggered midway,
 * the value remains protected and stable.
 *
 * Fails if the key does not exist.
 * Fails if the associated value is not of string type.
 */
NOCT_DLL
bool
noct_get_dict_elem_check_string(
	NoctEnv *env,
	NoctValue *dict,
	const char *key,
	NoctValue *val,
	const char **data);

/*
 * Convenience function to retrieve an array value associated with a
 * key in a dictionary with type checking.
 *
 * Fails if the key does not exist.
 * Fails if the associated value is not of array type.
 */
NOCT_DLL
bool
noct_get_dict_elem_check_array(
	NoctEnv *env,
	NoctValue *dict,
	const char *key,
	NoctValue *val);

/*
 * Convenience function to retrieve a dictionary value associated with a
 * key in a dictionary with type checking.
 *
 * Fails if the key does not exist.
 * Fails if the associated value is not of dictionary type.
 */
NOCT_DLL
bool
noct_get_dict_elem_check_dict(
	NoctEnv *env,
	NoctValue *dict,
	const char *key,
	NoctValue *val);

/*
 * Convenience function to retrieve a function value associated with a
 * key in a dictionary with type checking.
 *
 * The `backing_val` parameter must point to a variable previously
 * declared via `noct_declare_c_local()`. This variable serves as a
 * pinned storage location: during the call, it temporarily holds the
 * element's value so that, even if a parallel GC is triggered midway,
 * the value remains protected and stable.
 *
 * Fails if the key does not exist.
 * Fails if the associated value is not of function type.
 */
NOCT_DLL
bool
noct_get_dict_elem_check_func(
	NoctEnv *env,
	NoctValue *dict,
	const char *key,
	NoctValue *val,
	NoctFunc **f);

/*
 * Convenience function to set an integer value for a key in a dictionary.
 *
 * The `backing_val` parameter must point to a variable previously
 * declared via `noct_declare_c_local()`. This variable serves as a
 * pinned storage location: during the call, it temporarily holds the
 * element's value so that, even if a parallel GC is triggered midway,
 * the value remains protected and stable.
 */
NOCT_DLL
bool
noct_set_dict_elem_make_int(
	NoctEnv *env,
	NoctValue *dict,
	const char *key,
	NoctValue *val,
	int i);

/*
 * Convenience function to set a float value for a key in a dictionary.
 *
 * The `backing_val` parameter must point to a variable previously
 * declared via `noct_declare_c_local()`. This variable serves as a
 * pinned storage location: during the call, it temporarily holds the
 * element's value so that, even if a parallel GC is triggered midway,
 * the value remains protected and stable.
 */
NOCT_DLL
bool
noct_set_dict_elem_make_float(
	NoctEnv *env,
	NoctValue *dict,
	const char *key,
	NoctValue *val,
	float f);

/*
 * Convenience function to set a string value for a key in a dictionary.
 *
 * The `backing_val` parameter must point to a variable previously
 * declared via `noct_declare_c_local()`. This variable serves as a
 * pinned storage location: during the call, it temporarily holds the
 * element's value so that, even if a parallel GC is triggered midway,
 * the value remains protected and stable.
 */
NOCT_DLL
bool
noct_set_dict_elem_make_string(
	NoctEnv *env,
	NoctValue *dict,
	const char *key,
	NoctValue *val,
	const char *data);

/* Arguments */

/*
 * Convenience function to retrieve an integer function argument with
 * type checking.
 *
 * The `backing_val` parameter must point to a variable previously
 * declared via `noct_declare_c_local()`. This variable serves as a
 * pinned storage location: during the call, it temporarily holds the
 * element's value so that, even if a parallel GC is triggered midway,
 * the value remains protected and stable.
 */
NOCT_DLL
bool
noct_get_arg_check_int(
	NoctEnv *env,
	int index,
	NoctValue *val,
	int *i);

/*
 * Convenience function to retrieve a float function argument with
 * type checking.
 *
 * The `backing_val` parameter must point to a variable previously
 * declared via `noct_declare_c_local()`. This variable serves as a
 * pinned storage location: during the call, it temporarily holds the
 * element's value so that, even if a parallel GC is triggered midway,
 * the value remains protected and stable.
 */
NOCT_DLL
bool
noct_get_arg_check_float(
	NoctEnv *env,
	int index,
	NoctValue *val,
	float *f);

/*
 * Convenience function to retrieve a string function argument with
 * type checking.
 *
 * The `backing_val` parameter must point to a variable previously
 * declared via `noct_declare_c_local()`. This variable serves as a
 * pinned storage location: during the call, it temporarily holds the
 * element's value so that, even if a parallel GC is triggered midway,
 * the value remains protected and stable.
 */
NOCT_DLL
bool
noct_get_arg_check_string(
	NoctEnv *env,
	int index,
	NoctValue *val,
	const char **data);

/*
 * Convenience function to retrieve an array function argument with
 * type checking.
 */
NOCT_DLL
bool
noct_get_arg_check_array(
	NoctEnv *env,
	int index,
	NoctValue *val);

/*
 * Convenience function to retrieve a dictionary function argument with
 * type checking.
 */
NOCT_DLL
bool
noct_get_arg_check_dict(
	NoctEnv *env,
	int index,
	NoctValue *val);

/*
 * Convenience function to retrieve a function argument of function
 * type with type checking.
 *
 * The `backing_val` parameter must point to a variable previously
 * declared via `noct_declare_c_local()`. This variable serves as a
 * pinned storage location: during the call, it temporarily holds the
 * element's value so that, even if a parallel GC is triggered midway,
 * the value remains protected and stable.
 */
NOCT_DLL
bool
noct_get_arg_check_func(
	NoctEnv *env,
	int index,
	NoctValue *val,
	NoctFunc **f);

/* Return Values */

/*
 * Convenience function to set an integer return value for the current
 * stack frame.
 *
 * The `backing_val` parameter must point to a variable previously
 * declared via `noct_declare_c_local()`. This variable serves as a
 * pinned storage location: during the call, it temporarily holds the
 * element's value so that, even if a parallel GC is triggered midway,
 * the value remains protected and stable.
 */
NOCT_DLL
bool
noct_set_return_make_int(
	NoctEnv *env,
	NoctValue *val,
	int i);

/*
 * Convenience function to set a float return value for the current
 * stack frame.
 *
 * The `backing_val` parameter must point to a variable previously
 * declared via `noct_declare_c_local()`. This variable serves as a
 * pinned storage location: during the call, it temporarily holds the
 * element's value so that, even if a parallel GC is triggered midway,
 * the value remains protected and stable.
 */
NOCT_DLL
bool
noct_set_return_make_float(
	NoctEnv *env,
	NoctValue *val,
	float f);

/*
 * Convenience function to set a string return value for the current
 * stack frame.
 *
 * The `backing_val` parameter must point to a variable previously
 * declared via `noct_declare_c_local()`. This variable serves as a
 * pinned storage location: during the call, it temporarily holds the
 * element's value so that, even if a parallel GC is triggered midway,
 * the value remains protected and stable.
 */
NOCT_DLL
bool
noct_set_return_make_string(
	NoctEnv *env,
	NoctValue *val,
	const char *data);

/*
 * Configuration
 */

/* Enable JIT. */
extern bool noct_conf_jit_enable;

/* JIT threshold. */
extern int noct_conf_jit_threshold;

/* Optimization level. */
extern int noct_conf_optimize;

/* GC nursery region size. */
extern size_t noct_conf_gc_nursery_size;

/* GC graduate region size. */
extern size_t noct_conf_gc_graduate_size;

/* GC tenure region size. */
extern size_t noct_conf_gc_tenure_size;

/* GC large-object-promotion threshold. */
extern size_t noct_conf_gc_lop_threshold;

/* GC tenure-promotion threshold. */
extern size_t noct_conf_gc_promotion_threshold;

/*
 * Library Installation
 */

#define NOCT_REGISTER_ALL_APIS(env)	do { 			\
		if (!noct_register_api_system(env))		\
			return false;				\
		if (!noct_register_api_console(env))		\
			return false;				\
		if (!noct_register_api_math(env))		\
			return false;				\
	} while (0)

/* Register the "System.*" APIs. */
bool noct_register_api_system(NoctEnv *env);

/* Register the "Console.*" APIs. */
bool noct_register_api_console(NoctEnv *env);

/* Register the "Math.*" APIs. */
bool noct_register_api_math(NoctEnv *env);

#endif
