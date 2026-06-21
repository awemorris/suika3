/* -*- coding: utf-8; tab-width: 8; indent-tabs-mode: t; -*- */

/*
 * Noct Programming Language
 * Copyright (c) 2025, 2026, Awe Morris
 */

/*
 * Runtime
 */

#ifndef NOCT_RUNTIME_H
#define NOCT_RUNTIME_H

#include <noct/noct.h>
#include "gc.h"

/*
 * Maximum number of the stack depth.
 */
#define RT_FRAME_MAX		32

/*
 * Maximum number of the tmpvar in a stack.
 */
#define RT_TMPVAR_MAX		128

/*
 * Maximum number of the C pinned variables.
 */
#define RT_GLOBAL_PIN_MAX	64
#define RT_LOCAL_PIN_MAX	32

/*
 * Forward declaration.
 */
struct rt_vm;
struct rt_env;
struct rt_frame;
struct rt_value;
struct rt_object_header;
struct rt_string;
struct rt_array;
struct rt_dict;
struct rt_packed;
struct rt_func;
struct rt_bindglobal;

/*
 * String object.
 */
struct rt_string {
	struct rt_gc_object head;

	/* UTF-8 data. */
	char *data;

	/* Length including the tail NUL character. */
	size_t len;

	/* Hash. */
	uint32_t hash;
};

/*
 * Array object.
 */
struct rt_array {
	struct rt_gc_object head;

	/* Allocation size. */
	size_t alloc_size;

	/* Current size. */
	size_t size;

	/* Value table. */
	struct rt_value *table;

	/* Copy-On-Resize forwarding. (RCU-style) */
	struct rt_array *newer;

#if defined(NOCT_USE_MULTITHREAD)
	/* Creator thread. */
	struct rt_env *creator;

	/* Shared flag. */
	int shared;

	/* Write lock. */
	int write_lock;

	/* SeqLock */
	int seqlock;
#endif
};

/*
 * Dictionary object.
 */
struct rt_dict {
	struct rt_gc_object head;

	/* Allocation size. */
	size_t alloc_size;

	/* Current used elements. */
	size_t size;

	/* Key table. */
	struct rt_value *key;

	/* Value table. */
	struct rt_value *value;

	/* Copy-On-Resize forwarding. (RCU-style) */
	struct rt_dict *newer;

	/* Native object pointer. */
	void *native_pointer;

	/* Native object finalizer. */
	void (*native_finalizer)(void *native_pointer);

#if defined(NOCT_USE_MULTITHREAD)
	/* Creator thread. */
	struct rt_env *creator;

	/* Shared flag. */
	int shared;

	/* Write lock. */
	int write_lock;

	/* SeqLock */
	int seqlock;
#endif
};

/*
 * Packed (Buffer) object.
 */
struct rt_packed {
	struct rt_gc_object head;

	/* Primitive type. */
	int type;

	/* Allocated size in bytes. (0 if using a preallocated buffer.) */
	size_t size;

	/* Element count. */
	size_t elem_size;

	/* Packed type. */
	int packed_typed;

	/* Buffer pointer. */
	void *packed_buffer;
};

/*
 * Function object.
 */
struct rt_func {
	struct rt_gc_object head;

	char *name;
	size_t param_count;
	char *param_name[NOCT_ARG_MAX];

	char *file_name;

	/* Bytecode for a function. (if not a cfunc) */
	uint32_t bytecode_size;
	uint8_t *bytecode;
	uint32_t tmpvar_size;

	/* JIT-generated code. */
	bool (CDECL *jit_code)(struct rt_env *env);
	int call_count;

	/* Function pointer. (if a cfunc) */
	bool (*cfunc)(struct rt_env *env);

	/* Next. */
	struct rt_func *next;
};

/*
 * Global variable entry.
 */
struct rt_bindglobal {
	/* Symbol name. */
	char *name;

	/* Hash cache for the symbol name. */
	uint32_t name_len;
	uint32_t name_hash;

	/* Value. */
	struct rt_value val;

	/* Removed flag for linear search. */
	bool is_removed;
};

/*
 * Calling frame.
 */
struct rt_frame {
	/*
	 * tmpvar pointer.
	 *  - Do not move. JIT assumes its offset.
	 */
	struct rt_value *tmpvar;

	/*
	 * Size of the tmpvar table.
	 */
	uint32_t tmpvar_size;

	/*
	 * Current running function.
	 */
	struct rt_func *func;

	/*
	 * Pinned C local variables.
	 */
	struct rt_value *pinned[RT_LOCAL_PIN_MAX];
	uint32_t pinned_count;

	/*
	 * tmpvar body.
	 */
	struct rt_value tmpvar_alloc[RT_TMPVAR_MAX];
};

/*
 * Runtime environment.
 */
struct rt_env {
	/*
	 * [Stack Pointer]
	 *  - Do not move this. JIT codegen assumes its offset.
	 */
	struct rt_frame *frame;

	/*
	 * [Execution Line]
	 *  - Do not move. JIT codegen assumes the offset.
	 */
	int line;

	/*
	 * Do not move. (8-byte alisgnment)
	 */
	int _dummy;

	/*
	 * Reference to VM.
	 */
	struct rt_vm *vm;

	/*
	 * Stack allocation table, referenced by the "frame" field.
	 */
	struct rt_frame frame_alloc[RT_FRAME_MAX];
	int cur_frame_index;

	/*
	 * Execution file name. Set by "rt_call()".
	 */
	char file_name[256];

	/*
	 * Error message. Set by "rt_error()".
	 */
	char error_message[1024];

	/*
	 * Env linked list.
	 */
	struct rt_env *next;

#if defined(NOCT_USE_MULTITHREAD)
	/*
	 * Is this thread in-flight?
	 */
	bool is_in_flight;

	/*
	 * Is this thread raising STW request?
	 */
	bool is_stw_raised;

	/*
	 * Is this thread the STW executor?
	 */
	bool is_stw_executor;
#endif
};

/*
 * VM.
 */
struct rt_vm {
	/* Global symbols. */
	uint32_t global_alloc_size;
	uint32_t global_size;
	struct rt_bindglobal *global;

	/* Function list. */
	struct rt_func *func_list;

	/* GC. */
	struct rt_gc_info gc;

	/* Env list. */
	struct rt_env *env_list;

	/* Pinned C global variables. */
	struct rt_value *pinned[RT_GLOBAL_PIN_MAX];
	uint32_t pinned_count;

	/* Is JIT code written and not commited? */
	bool is_jit_dirty;

	/* Config. */
	struct rt_config config;

	/* GC nest counter. */
	int gc_in_progress_counter;

	/* GC level. */
	int gc_level;

#if defined(NOCT_USE_MULTITHREAD)
	/*
	 * Number of in-flight threads.
	 *  - See objectmodel.c
	 */
	int in_flight_counter;

	/*
	 * STW requests.
	 *  - Indicates the number of the threads that are raising requests.
	 *  - See objectmodel.c
	 */
	int stw_request_counter;

	/*
	 * STW executor lock.
	 *  - Reading 0 by RMW means the thread is promoted to STW executor.
	 *  - See objectmodel.c
	 */
	int stw_executor_lock;

	/*
	 * Lock for global variables.
	 */
	int global_var_counter;
#endif
};

/*
 * Initialization
 */

/* Create a runtime environment. */
bool
rt_create_vm(
	struct rt_vm **vm,
	struct rt_env **default_env,
	struct rt_config *config);

/* Destroy a runtime environment. */
bool
rt_destroy_vm(
	struct rt_vm *vm);

/* Create an environment for the current thread. */
bool
rt_create_thread_env(
	struct rt_env *prev_env,
	struct rt_env **new_env);

/*
 * Compilation
 */

/* Register functions from a souce text. */
bool
rt_register_source(
	struct rt_env *env,
	const char *file_name,
	const char *source_text);

/* Register functions from bytecode data. */
bool
rt_register_bytecode(
	struct rt_env *env,
	size_t size,
	uint8_t *data);

/* Register a native function. */
bool
rt_register_cfunc(
	struct rt_env *env,
	const char *name,
	size_t param_count,
	const char *param_name[],
	bool (*cfunc)(struct rt_env *env),
	struct rt_func **ret_func);

/*
 * Call
 */

/* Call a function with a name. */
bool
rt_call_with_name(
	struct rt_env *env,
	const char *func_name,
	uint32_t arg_count,
	struct rt_value *arg,
	struct rt_value *ret);

/* Call a function. */
bool
rt_call(
	struct rt_env *env,
	struct rt_func *func,
	uint32_t arg_count,
	struct rt_value *arg,
	struct rt_value *ret);

/*
 * String
 */

/* Make a string value. */
bool
rt_make_string(
	struct rt_env *env,
	struct rt_value *val,
	const char *data);

/* Make a string value. (hash version) */
bool
rt_make_string_with_hash(
	struct rt_env *env,
	struct rt_value *val,
	const char *data,
	size_t len,
	uint32_t hash);

/* Cache the hash of a string. */
void
rt_cache_string_hash(
	struct rt_string *rts);

/* Get a string hash. */
uint32_t
rt_string_hash(
	const char *s);

/* Get a string hash and length. */
void
rt_string_hash_and_len(
	const char *s,
	uint32_t *hash,
	uint32_t *len);

/*
 * Array, Dictionary, and Packed
 */

/* Make an empty array. */
bool
rt_make_empty_array(
	struct rt_env *env,
	struct rt_value *val);

/* Get the size of an array. */
bool
rt_get_array_size(
	struct rt_env *env,
	struct rt_value *arr,
	size_t *size);

/* Retrieves an array element. */
bool
rt_get_array_elem(
	struct rt_env *env,
	struct rt_value *arr,
	size_t index,
	struct rt_value *val);

/* Stores an value to an array. */
bool
rt_set_array_elem(
	struct rt_env *env,
	struct rt_value *arr,
	size_t index,
	struct rt_value *val);

/* Resizes an array. */
bool
rt_resize_array(
	struct rt_env *env,
	struct rt_value *arr,
	size_t size);

/* Make a shallow copy of an array. */
bool
rt_make_array_copy(
	struct rt_env *env,
	struct rt_value *dst,
	struct rt_value *src);

/* Make an empty dictionary value. */
bool
rt_make_empty_dict(
	struct rt_env *env,
	struct rt_value *val);

/* Get the size of a dictionary. */
bool
rt_get_dict_size(
	struct rt_env *env,
	struct rt_value *dict,
	size_t *size);

/* Get the allocation size of a dictionary. */
bool
rt_get_dict_alloc_size(
	struct rt_env *env,
	struct rt_value *dict,
	size_t *size);

/* Checks if a key exists in a dictionary. */
bool
rt_check_dict_key(
	struct rt_env *env,
	struct rt_value *dict,
	struct rt_value *key,
	bool *ret);

/* Checks if a key exists in a dictionary. */
bool
rt_check_dict_key_cstr(
	struct rt_env *env,
	struct rt_value *dict,
	const char *key,
	bool *ret);

/* Get a dictionary key by index. */
bool
rt_get_dict_by_index(
	struct rt_env *env,
	struct rt_value *dict,
	size_t index,
	struct rt_value *key,
	struct rt_value *val);

/* Retrieves the value by a key in a dictionary. */
bool
rt_get_dict_elem(
	struct rt_env *env,
	struct rt_value *dict,
	struct rt_value *key,
	struct rt_value *val);

/* Retrieves the value by a key in a dictionary. */
bool
rt_get_dict_elem_cstr(
	struct rt_env *env,
	struct rt_value *dict,
	const char *key,
	struct rt_value *val);

/* Retrieves the value by a key in a dictionary. */
bool
rt_get_dict_elem_with_hash(
	struct rt_env *env,
	struct rt_value *dict,
	const char *key,
	size_t len,
	uint32_t hash,
	struct rt_value *val);

/* Stores a key-value-pair to a dictionary. */
bool
rt_set_dict_elem(
	struct rt_env *env,
	struct rt_value *dict,
	struct rt_value *key,
	struct rt_value *val);

/* Stores a key-value-pair to a dictionary. */
bool
rt_set_dict_elem_cstr(
	struct rt_env *env,
	struct rt_value *dict,
	const char *key,
	struct rt_value *val);

/* Stores a key-value-pair to a dictionary. */
bool
rt_set_dict_elem_with_hash(
	struct rt_env *env,
	struct rt_value *dict,
	const char *key,
	size_t len,
	uint32_t hash,
	struct rt_value *val);

/* Remove a dictionary key. */
bool
rt_remove_dict_elem(
	struct rt_env *env,
	struct rt_value *dict,
	struct rt_value *key);

/* Remove a dictionary key. (hash version) */
bool
rt_remove_dict_elem_cstr(
	struct rt_env *env,
	struct rt_value *dict,
	const char *key);

/* Make a shallow copy of a dictionary. */
bool
rt_make_dict_copy(
	struct rt_env *env,
	struct rt_value *dst,
	struct rt_value *src);

/* Merges a dictionary. */
bool
rt_merge_dict(
	struct rt_env *env,
	struct rt_value *dst,
	struct rt_value *src1,
	struct rt_value *src2);

/* Sets the native pointers to a dictionary. */
bool
rt_set_dict_native_pointer(
	struct rt_env *env,
	struct rt_value *dict,
	void *native_pointer,
	void (*native_finalizer)(void *native_pointer));

/* Gets the native pointer from a dictionary. */
bool
rt_get_dict_native_pointer(
	struct rt_env *env,
	struct rt_value *dict,
	void **native_pointer,
	void (**native_finalizer)(void *native_pointer));

/* Make a packed. */
bool
rt_make_packed(
	struct rt_env *env,
	struct rt_value *val,
	int type,
	size_t size,
	size_t elem_size,
	void *preallocated);

/* Get the element type of a packed. */
bool
rt_get_packed_type(
	struct rt_env *env,
	struct rt_value *packed,
	int *type);

/* Get the element count of a packed. */
bool
rt_get_packed_size(
	struct rt_env *env,
	struct rt_value *packed,
	size_t *size);

/* Retrieves an int8 packed element. */
bool
rt_get_packed_elem(
	struct rt_env *env,
	struct rt_value *packed,
	size_t index,
	struct rt_value *val);

/* Stores an value to a packed. */
bool
rt_set_packed_elem(
	struct rt_env *env,
	struct rt_value *packed,
	size_t index,
	struct rt_value *val);

/* Make a copy of a packed. */
bool
rt_make_packed_copy(
	struct rt_env *env,
	struct rt_value *dst,
	struct rt_value *src);

/*
 * Global Variable
 */

/* Check if a global variable exists. */
bool
rt_check_global(
	struct rt_env *env,
	const char *name);

/* Get a global variable. */
bool
rt_get_global(
	struct rt_env *env,
	const char *name,
	struct rt_value *val);

/* Get a global variable. (hash version) */
bool
rt_get_global_with_hash(
	struct rt_env *env,
	const char *name,
	size_t len,
	uint32_t hash,
	struct rt_value *val);

/* Set a global variable. */
bool
rt_set_global(
	struct rt_env *env,
	const char *name,
	struct rt_value *val);

/* Set a global variable. (hash version) */
bool
rt_set_global_with_hash(
	struct rt_env *env,
	const char *name,
	size_t len,
	uint32_t hash,
	struct rt_value *val);

/*
 * FFI Pin
 */

/* Pin a C global variable. */
bool
rt_pin_global(
	struct rt_env *env,
	struct rt_value *val);

/* Pin a C global variable. */
bool
rt_unpin_global(
	struct rt_env *env,
	struct rt_value *val);

/* Pin a C local variable. */
bool
rt_pin_local(
	struct rt_env *env,
	struct rt_value *val);

/* Unpin a C local variable. */
bool
rt_unpin_local(
	struct rt_env *env,
	struct rt_value *val);

/* Make a safepoint. */
bool
rt_safepoint(
	struct rt_env *env);

/*
 * Error Handling
 */

/* Get an error message. */
const char *
rt_get_error_message(
	struct rt_env *env);

/* Get an error file name. */
const char *
rt_get_error_file(
	struct rt_env *env);

/* Get an error line number. */
int
rt_get_error_line(
	struct rt_env *env);

/* Output an error message.*/
void
rt_error(
	struct rt_env *env,
	const char *msg,
	...);

/* Output an out-of-memory message. */
void
rt_out_of_memory(
	struct rt_env *env);

#endif
