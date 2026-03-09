/* -*- coding: utf-8; tab-width: 8; indent-tabs-mode: t; -*- */

/*
 * Copyright (c) 2025, Awe Morris. All rights reserved.
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

struct rt_env;
struct rt_frame;
struct rt_value;
struct rt_object_header;
struct rt_string;
struct rt_array;
struct rt_dict;
struct rt_func;
struct rt_bindglobal;

/*
 * String object.
 */
struct rt_string {
	struct rt_gc_object head;

	char *data;
	size_t len;	/* Including the tail NUL character*/
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
	/* Atomic counter. */
	int counter;
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

#if defined(NOCT_USE_MULTITHREAD)
	/* Atomic counter. */
	int counter;
#endif
};

#define RT_DICT_KEY_REMOVED ((struct rt_value *)((intptr_t)-1))

/*
 * Function object.
 */
struct rt_func {
	struct rt_gc_object head;

	char *name;
	uint32_t param_count;
	char *param_name[NOCT_ARG_MAX];

	char *file_name;

	/* Bytecode for a function. (if not a cfunc) */
	uint32_t bytecode_size;
	uint8_t *bytecode;
	uint32_t tmpvar_size;

	/* JIT-generated code. */
	bool (*jit_code)(struct rt_env *env);
	uint32_t call_count;

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
	int tmpvar_size;

	/*
	 * Current running function.
	 */
	struct rt_func *func;

	/*
	 * Pinned C local variables.
	 */
	struct rt_value *pinned[RT_LOCAL_PIN_MAX];
	int pinned_count;

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
	/* Atomic counter for GC. */
	int gc_in_progress_counter;
#endif
};

/*
 * VM.
 */
struct rt_vm {
	/* Global symbols. */
	int global_alloc_size;
	int global_size;
	struct rt_bindglobal *global;

	/* Function list. */
	struct rt_func *func_list;

	/* GC. */
	struct rt_gc_info gc;

	/* Env list. */
	struct rt_env *env_list;

	/* Pinned C global variables. */
	struct rt_value *pinned[RT_GLOBAL_PIN_MAX];
	int pinned_count;

	/* Is JIT code written and not commited? */
	bool is_jit_dirty;

#if defined(NOCT_USE_MULTITHREAD)
	/* In-flight counter for GC exclusion. */
	int in_flight_counter;

	/* GC stop-the-world counter. */
	int gc_stw_counter;

	/* Atomic counter for global variables. */
	int global_var_counter;
#endif
};

/*
 * Initialization
 */

/* Create a runtime environment. */
bool rt_create_vm(struct rt_vm **vm, struct rt_env **default_env);

/* Destroy a runtime environment. */
bool rt_destroy_vm(struct rt_vm *vm);

/* Create an environment for the current thread. */
bool rt_create_thread_env(struct rt_env *prev_env, struct rt_env **new_env);

/*
 * Compilation
 */

/* Register functions from a souce text. */
bool rt_register_source(struct rt_env *env, const char *file_name, const char *source_text);

/* Register functions from bytecode data. */
bool rt_register_bytecode(struct rt_env *env, size_t size, uint8_t *data);

/* Register an FFI C function. */
bool rt_register_cfunc(struct rt_env *env, const char *name, int param_count, const char *param_name[], bool (*cfunc)(struct rt_env *env), struct rt_func **ret_func);

/*
 * Call
 */

/* Call a function with a name. */
bool rt_call_with_name(struct rt_env *env, const char *func_name, int arg_count, struct rt_value *arg, struct rt_value *ret);

/* Call a function. */
bool rt_call(struct rt_env *env, struct rt_func *func, int arg_count, struct rt_value *arg, struct rt_value *ret);

/*
 * String
 */

/* Make a string value. */
bool rt_make_string(struct rt_env *env, struct rt_value *val, const char *data);

/* Make a string value. (hash version) */
bool rt_make_string_with_hash(struct rt_env *env, struct rt_value *val, const char *data, size_t len, uint32_t hash);

/* Cache the hash of a string. */
void rt_cache_string_hash(struct rt_string *rts);

/*
 * Array and Dictionary
 */

/* Make an empty array. */
bool rt_make_empty_array(struct rt_env *env, struct rt_value *val);

/* Get the size of an array. */
bool rt_get_array_size(struct rt_env *env, struct rt_array *arr, uint32_t *size);

/* Retrieves an array element. */
bool rt_get_array_elem(struct rt_env *env, struct rt_array *arr, int index, struct rt_value *val);

/* Stores an value to an array. */
bool rt_set_array_elem(struct rt_env *env, struct rt_array **arr, int index, struct rt_value *val);

/* Resizes an array. */
bool rt_resize_array(struct rt_env *env, struct rt_array **arr, int size);

/* Make a shallow copy of an array. */
bool rt_make_array_copy(struct rt_env *env, struct rt_array **dst, struct rt_array *src);

/* Make an empty dictionary value. */
bool rt_make_empty_dict(struct rt_env *env, struct rt_value *val);

/* Get the size of a dictionary. */
bool rt_get_dict_size(struct rt_env *env, struct rt_dict *dict, uint32_t *size);

/* Checks if a key exists in a dictionary. */
bool rt_check_dict_key(struct rt_env *env, struct rt_dict *dict, const char *key, bool *ret);

/* Get a dictionary key by index. */
bool rt_get_dict_key_by_index(struct rt_env *env, struct rt_dict *dict, int index, struct rt_value *key);

/* Get a dictionary value by index. */
bool rt_get_dict_value_by_index(struct rt_env *env, struct rt_dict *dict, int index, struct rt_value *val);

/* Retrieves the value by a key in a dictionary. */
bool rt_get_dict_elem(struct rt_env *env, struct rt_dict *dict, const char *key, struct rt_value *val);

/* Retrieves the value by a key in a dictionary. (hash version) */
bool rt_get_dict_elem_with_hash(struct rt_env *env, struct rt_dict *dict, const char *key, size_t len, uint32_t hash, struct rt_value *val);

/* Stores a key-value-pair to a dictionary. */
bool rt_set_dict_elem(struct rt_env *env, struct rt_dict **dict, const char *key, struct rt_value *val);

/* Stores a key-value-pair to a dictionary. (hash version) */
bool rt_set_dict_elem_with_hash(struct rt_env *env, struct rt_dict **dict, const char *key, size_t len, uint32_t hash, struct rt_value *val);

/* Remove a dictionary key. */
bool rt_remove_dict_elem(struct rt_env *env, struct rt_dict *dict, const char *key);

/* Remove a dictionary key. (hash version) */
bool rt_remove_dict_elem_with_hash(struct rt_env *env, struct rt_dict *dict, const char *key, size_t len, uint32_t hash);

/* Make a shallow copy of a dictionary. */
bool rt_make_dict_copy(struct rt_env *env, struct rt_dict **dst, struct rt_dict *src);

/*
 * Global Variable
 */

/* Check if a global variable exists. */
bool rt_check_global(struct rt_env *env, const char *name);

/* Get a global variable. */
bool rt_get_global(struct rt_env *env, const char *name, struct rt_value *val);

/* Get a global variable. (hash version) */
bool rt_get_global_with_hash(struct rt_env *env, const char *name, size_t len, uint32_t hash, struct rt_value *val);

/* Set a global variable. */
bool rt_set_global(struct rt_env *env, const char *name, struct rt_value *val);

/* Set a global variable. (hash version) */
bool rt_set_global_with_hash(struct rt_env *env, const char *name, size_t len, uint32_t hash, struct rt_value *val);

/*
 * FFI Pin
 */

/* Pin a C global variable. */
bool rt_pin_global(struct rt_env *env, struct rt_value *val);

/* Pin a C global variable. */
bool rt_unpin_global(struct rt_env *env, struct rt_value *val);

/* Pin a C local variable. */
bool rt_pin_local(struct rt_env *env, struct rt_value *val);

/* Unpin a C local variable. */
bool rt_unpin_local(struct rt_env *env, struct rt_value *val);

/*
 * Error Handling
 */

/* Get an error message. */
const char *rt_get_error_message(struct rt_env *env);

/* Get an error file name. */
const char *rt_get_error_file(struct rt_env *env);

/* Get an error line number. */
int rt_get_error_line(struct rt_env *env);

/* Output an error message.*/
void rt_error(struct rt_env *env, const char *msg, ...);

/* Output an out-of-memory message. */
void rt_out_of_memory(struct rt_env *env);

#endif
