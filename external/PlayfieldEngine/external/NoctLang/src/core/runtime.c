/* -*- coding: utf-8; tab-width: 8; indent-tabs-mode: t; -*- */

/*
 * Noct Programming Language
 * Copyright (c) 2025, 2026, Awe Morris
 */

/*
 * Language Runtime
 */

#include <noct/noct.h>
#include "ast.h"
#include "hir.h"
#include "lir.h"
#include "runtime.h"
#include "intrinsics.h"
#include "interpreter.h"
#include "jit.h"
#include "gc.h"

#if defined(NOCT_USE_MULTITHREAD)
#include "atomic.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>
#include <assert.h>

/* False assertion */
#define NOT_IMPLEMENTED		0
#define NEVER_COME_HERE		0
#define PINNED_VAR_NOT_FOUND	0

/* Dictionary key. */
#define IS_DICT_KEY_EMPTY(k)	(k.type == NOCT_VALUE_INT)
#define IS_DICT_KEY_REMOVED(k)	(k.type == NOCT_VALUE_FLOAT)
#define REMOVE_DICT_KEY(k)	do { k.type = NOCT_VALUE_FLOAT; } while (0)

#if !defined(NOCT_USE_MULTITHREAD)

#define ACQUIRE_OBJ(obj, real_obj)				\
	/* Get the newer reference. */				\
	real_obj = (obj);					\
	while (real_obj->newer != NULL)				\
		real_obj = real_obj->newer;

#define RELEASE_OBJ(real_obj)

#define ACQUIRE_OBJ2(obj1, real_obj1, obj2, real_obj2)		\
	/* Get the newer reference of obj1. */			\
	real_obj1 = (obj1);					\
	while (real_obj1->newer != NULL)			\
		real_obj1 = real_obj1->newer;			\
	/* Get the newer reference of obj2. */			\
	real_obj2 = (obj2);					\
	while (real_obj2->newer != NULL)			\
		real_obj2 = real_obj2->newer;

#define RELEASE_OBJ2(real_obj1, real_obj2)

#else

#define ACQUIRE_OBJ(obj, real_obj)								\
	/* Acquire the array. */								\
	while (1) {										\
		/* Get the newer reference. */							\
		real_obj = atomic_load_relaxed_ptr((void**)&(obj));				\
		while (atomic_load_relaxed_ptr((void **)&real_obj->newer) != NULL)		\
			real_obj = atomic_load_relaxed_ptr((void **)&real_obj->newer);		\
												\
		/* Try acquire. */								\
		int old = atomic_fetch_add_acquire(&real_obj->counter, 1);			\
		if (old == 0 && atomic_load_acquire_ptr((void **)&real_obj->newer) == NULL)	\
			break;									\
												\
		/* Failed, release. */								\
		atomic_fetch_sub_release(&real_obj->counter, 1);				\
												\
		/* Allow GC in other threads because they may cause GC. */			\
		while (1) {									\
			atomic_fetch_sub_release(&env->vm->in_flight_counter, 1);		\
			while (atomic_load_acquire(&env->vm->gc_stw_counter) > 0)		\
				cpu_relax();							\
			atomic_fetch_add_acquire(&env->vm->in_flight_counter, 1);		\
			if (atomic_load_acquire(&env->vm->gc_stw_counter) == 0)			\
				break;								\
		}										\
	}

#define ACQUIRE_OBJ2(obj1, real_obj1, obj2, real_obj2)						\
	/* Acquire two objects atomically. obj1 and obj2 must differ. */			\
	while (1) {										\
		/* Get the newer reference of obj1. */						\
		real_obj1 = atomic_load_relaxed_ptr((void **)&(obj1));				\
		while (atomic_load_relaxed_ptr((void **)&real_obj1->newer) != NULL)		\
			real_obj1 = atomic_load_relaxed_ptr((void **)&real_obj1->newer);	\
												\
		/* Try acquire obj1. */								\
		int _old1 = atomic_fetch_add_acquire(&real_obj1->counter, 1);			\
		if (_old1 != 0 ||								\
		    atomic_load_acquire_ptr((void **)&real_obj1->newer) != NULL) {		\
			/* Failed to acquire obj1, release and wait for GC. */			\
			atomic_fetch_sub_release(&real_obj1->counter, 1);			\
			while (1) {								\
				atomic_fetch_sub_release(&env->vm->in_flight_counter, 1);	\
				while (atomic_load_acquire(&env->vm->gc_stw_counter) > 0)	\
					cpu_relax();						\
				atomic_fetch_add_acquire(&env->vm->in_flight_counter, 1);	\
				if (atomic_load_acquire(&env->vm->gc_stw_counter) == 0)		\
					break;							\
			}									\
			continue;								\
		}										\
												\
		/* obj1 acquired. Now try acquire obj2. */					\
		real_obj2 = atomic_load_relaxed_ptr((void **)&(obj2));				\
		while (atomic_load_relaxed_ptr((void **)&real_obj2->newer) != NULL)		\
			real_obj2 = atomic_load_relaxed_ptr((void **)&real_obj2->newer);	\
												\
		int _old2 = atomic_fetch_add_acquire(&real_obj2->counter, 1);			\
		if (_old2 == 0 &&								\
		    atomic_load_acquire_ptr((void **)&real_obj2->newer) == NULL)		\
			break; /* Both acquired. */						\
												\
		/* Failed to acquire obj2.  Release obj2 and obj1, then wait. */		\
		atomic_fetch_sub_release(&real_obj2->counter, 1);				\
		atomic_fetch_sub_release(&real_obj1->counter, 1);				\
		while (1) {									\
			atomic_fetch_sub_release(						\
				&env->vm->in_flight_counter, 1);				\
			while (atomic_load_acquire(						\
				&env->vm->gc_stw_counter) > 0)					\
				cpu_relax();							\
			atomic_fetch_add_acquire(						\
				&env->vm->in_flight_counter, 1);				\
			if (atomic_load_acquire(						\
				&env->vm->gc_stw_counter) == 0)					\
				break;								\
		}										\
	}

#define RELEASE_OBJ(real_obj)									\
	/* Failed, release. */									\
	atomic_fetch_sub_release(&real_obj->counter, 1);

#define RELEASE_OBJ2(real_obj1, real_obj2)							\
	/* Release in reverse acquisition order. */						\
	atomic_fetch_sub_release(&real_obj2->counter, 1);					\
	atomic_fetch_sub_release(&real_obj1->counter, 1);

#endif

/* Forward declarations. */
static void rt_free_func(struct rt_env *rt, struct rt_func *func);
static bool rt_register_lir(struct rt_env *rt, struct lir_func *lir);
static bool rt_register_bytecode_function(struct rt_env *rt, uint8_t *data, size_t size, uint32_t *pos, char *file_name);
static const char *rt_read_bytecode_line(uint8_t *data, size_t size, uint32_t *pos);
static bool rt_enter_frame(struct rt_env *env, struct rt_func *func);
static void rt_leave_frame(struct rt_env *env);
static bool rt_expand_array(struct rt_env *env, struct rt_array *old_arr, struct rt_array **new_arr_pp, size_t size);
static bool rt_expand_dict(struct rt_env *env, struct rt_dict *old_dict, struct rt_dict **new_dict_pp);
static bool rt_init_global(struct rt_env *env);
static void rt_cleanup_global(struct rt_env *env);
static bool rt_expand_global(struct rt_env *env);

/*
 * Initialization
 */

/*
 * Create a virtual machine.
 */
bool
rt_create_vm(
	struct rt_vm **vm,
	struct rt_env **default_env,
	struct rt_config *config)
{
	/* Allocate a struct rt_vm. */
	*vm = noct_malloc(sizeof(struct rt_vm));
	if (*vm == NULL) {
		*default_env = NULL;
		return false;
	}
	memset(*vm, 0, sizeof(struct rt_vm));

	/* Copy the config if specified. */
	if (config != NULL)
		memcpy(&(*vm)->config, config, sizeof(struct rt_config));
	else
		noct_set_default_config(&(*vm)->config);

	/* Allocate a struct rt_env. */
	*default_env = noct_malloc(sizeof(struct rt_env));
	if (*default_env == NULL) {
		noct_free(*vm);
		*vm = NULL;
		return false;
	}
	memset(*default_env, 0, sizeof(struct rt_env));
	(*default_env)->vm = *vm;
	(*vm)->env_list = *default_env;

	/* Enter the initial stack frame. */
	(*default_env)->cur_frame_index = 0;
	(*default_env)->frame = &(*default_env)->frame_alloc[0];
	(*default_env)->frame->tmpvar = &(*default_env)->frame->tmpvar_alloc[0];
	(*default_env)->frame->tmpvar_size = RT_TMPVAR_MAX;
	memset((*default_env)->frame->tmpvar, 0, sizeof(struct rt_value) * RT_TMPVAR_MAX);

#if defined(NOCT_USE_MULTITHREAD)
	/* Initialize for GC. */
	rt_gc_init_env(*default_env);
#endif

	/* Initialize the global variables. */
	if (!rt_init_global(*default_env)) {
		noct_free(*default_env);
		noct_free(*vm);
		return false;
	}

	/* Initialize the garbage collector. */
	if (!rt_gc_init(*vm)) {
		rt_cleanup_global(*default_env);
		noct_free(*default_env);
		noct_free(*vm);
		return false;
	}

	/* Register the intrinsics. */
	if (!rt_register_intrinsics(*default_env)) {
		rt_cleanup_global(*default_env);
		rt_gc_cleanup(*vm);
		noct_free(*default_env);
		noct_free(*vm);
		return false;
	}

	return true;
}

/*
 * Destroy a virtual machine.
 */
bool
rt_destroy_vm(
	struct rt_vm *vm)
{
	struct rt_env *env, *next_env;
	struct rt_func *func, *next_func;

	/* Free the JIT region. */
	if (vm->config.jit_enable)
		jit_free(vm->env_list);

	/* Free global variables. */
	rt_cleanup_global(vm->env_list);

	/* Free thread environments. */
	env = vm->env_list;
	while (env != NULL) {
		next_env = env->next;
		noct_free(env);
		env = next_env;
	}

	/* Cleanup the garbage collector. */
	rt_gc_cleanup(vm);

	/* Free functions. */
	func = vm->func_list;
	while (func != NULL) {
		next_func = func->next;
		rt_free_func(vm->env_list, func);
		func = next_func;
	}

	/* Free rt_env. */
	noct_free(vm);

	return true;
}

/* Free a function. */
static void
rt_free_func(
	struct rt_env *env,
	struct rt_func *func)
{
	int i;

	UNUSED_PARAMETER(env);

	noct_free(func->name);
	func->name = NULL;
	for (i = 0; i < NOCT_ARG_MAX; i++) {
		if (func->param_name[i] != NULL) {
			noct_free(func->param_name[i]);
			func->param_name[i] = NULL;
		}
	}
	noct_free(func->file_name);
	noct_free(func->bytecode);

	if (func->jit_code != NULL)
		func->jit_code = NULL;
}

#if defined(NOCT_USE_MULTITHREAD)
/*
 * Create an environment for the current thread.
 */
bool
rt_create_thread_env(
	struct rt_env *prev_env,
	struct rt_env **new_env)
{
	struct rt_env *env;

	/* Allocate a struct rt_env. */
	env = noct_malloc(sizeof(struct rt_env));
	if (env == NULL) {
		rt_out_of_memory(prev_env);
		return false;
	}
	memset(env, 0, sizeof(struct rt_env));
	env->vm = prev_env->vm;

	/* Link. */
	env->next = prev_env->vm->env_list;
	prev_env->vm->env_list = env;

	/* Enter the initial stack frame. */
	env->cur_frame_index = 0;
	env->frame = &env->frame_alloc[0];
	env->frame->tmpvar = &env->frame->tmpvar_alloc[0];
	env->frame->tmpvar_size = RT_TMPVAR_MAX;

	/* Initialize for GC. */
	rt_gc_init_env(env);

	/* Succeeded. */
	*new_env = env;
	return true;
}
#endif

/*
 * Compilation
 */

/*
 * Register functions from a souce text.
 */
bool
rt_register_source(
	struct rt_env *env,
	const char *file_name,
	const char *source_text)
{
	struct hir_block *hfunc;
	struct lir_func *lfunc;
	uint32_t i, func_count;
	bool is_succeeded;

	is_succeeded = false;
	do {
		/* Do parse and build AST. */
		if (!ast_build(file_name, source_text)) {
			strncpy(env->file_name, ast_get_file_name(), sizeof(env->file_name) - 1);
			env->line = ast_get_error_line();
			rt_error(env, "%s", ast_get_error_message());
			break;
		}

		/* Transform AST to HIR. */
		if (!hir_build()) {
			strncpy(env->file_name, hir_get_file_name(), sizeof(env->file_name) - 1);
			env->line = hir_get_error_line();
			rt_error(env, "%s", hir_get_error_message());
			break;
		}

		/* For each function. */
		func_count = hir_get_function_count();
		for (i = 0; i < func_count; i++) {
			/* Transform HIR to LIR (bytecode). */
			hfunc = hir_get_function(i);
			if (!lir_build(hfunc, &lfunc)) {
				strncpy(env->file_name, lir_get_file_name(), sizeof(env->file_name) - 1);
				env->line = lir_get_error_line();
				rt_error(env, "%s", lir_get_error_message());
				break;
			}

			/* Make a function object. */
			if (!rt_register_lir(env, lfunc))
				break;

			/* Free a LIR. */
			lir_cleanup(lfunc);
		}
		if (i != func_count)
			break;

		is_succeeded = true;
	} while (0);

	/* Free intermediates. */
	hir_cleanup();
	ast_cleanup();

	/* If failed. */
	if (!is_succeeded)
		return false;

	/* Succeeded. */
	return true;
}

/* Register a function from LIR. */
static bool
rt_register_lir(
	struct rt_env *env,
	struct lir_func *lir)
{
	struct rt_func *func;
	struct rt_value global;
	uint32_t i;

	func = noct_malloc(sizeof(struct rt_func));
	if (func == NULL) {
		rt_out_of_memory(env);
		return false;
	}
	memset(func, 0, sizeof(struct rt_func));

	func->name = strdup(lir->func_name);
	if (func->name == NULL) {
		rt_out_of_memory(env);
		return false;
	}
	func->param_count = lir->param_count;
	for (i = 0; i < lir->param_count; i++) {
		func->param_name[i] = strdup(lir->param_name[i]);
		if (func->param_name[i] == NULL) {
			rt_out_of_memory(env);
			return false;
		}
	}
	func->bytecode_size = lir->bytecode_size;
	if (func->bytecode_size != 0) {
		func->bytecode = noct_malloc((size_t)lir->bytecode_size);
		if (func->bytecode == NULL) {
			rt_out_of_memory(env);
			return false;
		}
		memcpy(func->bytecode, lir->bytecode, (size_t)lir->bytecode_size);
	}
	func->tmpvar_size = lir->tmpvar_size;
	func->file_name = strdup(lir->file_name);
	if (func->file_name == NULL) {
		rt_out_of_memory(env);
		return false;
	}

	/* Insert a global variable. */
	global.type = NOCT_VALUE_FUNC;
	global.val.func = func;
	if (!rt_set_global(env, func->name, &global))
		return false;

	/* Do JIT compilation */
	if (env->vm->config.jit_enable) {
		if (env->vm->config.jit_threshold == 0) {
			/* Write code. */
			if (!jit_build(env, func)) {
				/* -1 means JIT failed. */
				func->call_count = -1;
			}

			/* Need to commit before call. */
			env->vm->is_jit_dirty = true;
		}
	}

	/* Link. */
	func->next = env->vm->func_list;
	env->vm->func_list = func;

	return true;
}

/*
 * Register functions from bytecode data.
 */
bool
rt_register_bytecode(
	struct rt_env *env,
	size_t size,
	uint8_t *data)
{
	char *file_name;
	const char *line;
	uint32_t pos, func_count, i;
	bool succeeded;

	pos = 0;
	file_name = NULL;
	succeeded = false;
	do {
		/* Check "CScript Bytecode". */
		line = rt_read_bytecode_line(data, size, &pos);
		if (line == NULL || strcmp(line, "Noct Bytecode 1.0") != 0)
			break;

		/* Check "Source". */
		line = rt_read_bytecode_line(data, size, &pos);
		if (line == NULL || strcmp(line, "Source") != 0)
			break;

		/* Get a source file name. */
		line = rt_read_bytecode_line(data, size, &pos);
		if (line == NULL)
			break;
		file_name = strdup(line);
		if (file_name == NULL)
			break;

		/* Check "Number Of Functions". */
		line = rt_read_bytecode_line(data, size, &pos);
		if (line == NULL || strcmp(line, "Number Of Functions") != 0)
			break;

		/* Get a number of functions. */
		line = rt_read_bytecode_line(data, size, &pos);
		if (line == NULL)
			break;
		func_count = (uint32_t)atoi(line);

		/* Read functions. */
		for (i = 0; i < func_count; i++) {
			if (!rt_register_bytecode_function(env, data, size, &pos, file_name))
				break;
		}

		succeeded = true;
	} while (0);

	if (file_name != NULL)
		noct_free(file_name);

	if (!succeeded) {
		rt_error(env, N_TR("Failed to load bytecode."));
		return false;
	}

	return true;
}

/* Register a function from bytecode file data. */
static bool
rt_register_bytecode_function(
	struct rt_env *env,
	uint8_t *data,
	size_t size,
	uint32_t *pos,
	char *file_name)
{
	struct lir_func lfunc;
	const char *line;
	uint32_t i;
	bool succeeded;

	memset(&lfunc, 0, sizeof(lfunc));
	lfunc.file_name = file_name;

	succeeded = false;
	do {
		/* Check "Begin Function". */
		line = rt_read_bytecode_line(data, size, pos);
		if (line == NULL || strcmp(line, "Begin Function") != 0)
			break;

		/* Check "Name". */
		line = rt_read_bytecode_line(data, size, pos);
		if (line == NULL || strcmp(line, "Name") != 0)
			break;

		/* Get a function name. */
		line = rt_read_bytecode_line(data, size, pos);
		if (line == NULL)
			break;
		lfunc.func_name = strdup(line);
		if (lfunc.func_name == NULL)
			break;

		/* Check "Parameters". */
		line = rt_read_bytecode_line(data, size, pos);
		if (line == NULL || strcmp(line, "Parameters") != 0)
			break;

		/* Get number of parameters. */
		line = rt_read_bytecode_line(data, size, pos);
		if (line == NULL)
			break;
		lfunc.param_count = (uint32_t)atoi(line);

		/* Get parameters. */
		for (i = 0; i < lfunc.param_count; i++) {
			line = rt_read_bytecode_line(data, size, pos);
			if (line == NULL)
				break;
			lfunc.param_name[i] = strdup(line);
			if (lfunc.param_name[i] == NULL)
				break;
		}
		if (i != lfunc.param_count)
			break;

		/* Check "Temporary Size". */
		line = rt_read_bytecode_line(data, size, pos);
		if (line == NULL || strcmp(line, "Temporary Size") != 0)
			break;

		/* Get a local size. */
		line = rt_read_bytecode_line(data, size, pos);
		if (line == NULL)
			break;
		lfunc.tmpvar_size = (uint32_t)atoi(line);

		/* Check "Bytecode Size". */
		line = rt_read_bytecode_line(data, size, pos);
		if (line == NULL || strcmp(line, "Bytecode Size") != 0)
			break;

		/* Get a bytecode size. */
		line = rt_read_bytecode_line(data, size, pos);
		if (line == NULL)
			break;
		lfunc.bytecode_size = (uint32_t)atoi(line);

		/* Load LIR. */
		lfunc.bytecode = data + *pos;
		if (!rt_register_lir(env, &lfunc))
			break;

		/* Check "End Function". */
		(*pos) += lfunc.bytecode_size + 1;
		line = rt_read_bytecode_line(data, size, pos);
		if (line == NULL || strcmp(line, "End Function") != 0)
			break;

		succeeded = true;
	} while (0);

	if (lfunc.func_name != NULL)
		noct_free(lfunc.func_name);

	for (i = 0; i < NOCT_ARG_MAX; i++) {
		if (lfunc.param_name[i] != NULL)
			noct_free(lfunc.param_name[i]);
	}

	if (!succeeded) {
		noct_error(env, N_TR("Failed to load bytecode data."));
		return false;
	}

	return true;
}

/* Read a line from bytecode file data. */
static const char *
rt_read_bytecode_line(
	uint8_t *data,
	size_t size,
	uint32_t *pos)
{
	static char line[1024];
	uint32_t i;

	for (i = 0; i < sizeof(line); i++) {
		if (*pos >= size)
			return NULL;

		line[i] = (char)data[*pos];
		(*pos)++;
		if (line[i] == '\n') {
			line[i] = '\0';
			return line;
		}
	}
	return NULL;
}

/*
 * Register a native function.
 */
bool
rt_register_cfunc(
	struct rt_env *env,
	const char *name,
	uint32_t param_count,
	const char *param_name[],
	bool (*cfunc)(struct rt_env *env),
	struct rt_func **ret_func)
{
	struct rt_func *func;
	struct rt_value global;
	uint32_t i;

	func = noct_malloc(sizeof(struct rt_func));
	if (func == NULL) {
		rt_out_of_memory(env);
		return false;
	}
	memset(func, 0, sizeof(struct rt_func));

	func->name = strdup(name);
	if (func->name == NULL) {
		rt_out_of_memory(env);
		return false;
	}
	func->param_count = param_count;
	for (i = 0; i < param_count; i++) {
		func->param_name[i] = strdup(param_name[i]);
		if (func->param_name[i] == NULL) {
			rt_out_of_memory(env);
			return false;
		}
	}
	func->cfunc = cfunc;
	func->tmpvar_size = param_count + 1;

	global.type = NOCT_VALUE_FUNC;
	global.val.func = func;
	if (!rt_set_global(env, name, &global))
		return false;

	if (ret_func != NULL)
		*ret_func = func;

	return true;
}

/*
 * Call
 */

/*
 * Call a function with a name.
 */
bool
rt_call_with_name(
	struct rt_env *env,
	const char *func_name,
	uint32_t arg_count,
	struct rt_value *arg,
	struct rt_value *ret)
{
	struct rt_value global;
	struct rt_func *func;
	bool func_ok;

	/* Search a function. */
	func_ok = false;
	do {
		if (!rt_check_global(env, func_name))
			break;
		if (!rt_get_global(env, func_name, &global))
			break;
		if (global.type != NOCT_VALUE_FUNC)
			break;
		func_ok = true;
	} while (0);
	if (!func_ok) {
		noct_error(env, N_TR("Cannot find function %s."), func_name);
		return false;
	}
	func = global.val.func;

	/* Call. */
	if (!rt_call(env, func, arg_count, arg, ret))
		return false;

	return true;
}

/*
 * Call a function.
 */
bool
rt_call(
	struct rt_env *env,
	struct rt_func *func,
	uint32_t arg_count,
	struct rt_value *arg,
	struct rt_value *ret)
{
	char old_file_name[256];
	uint32_t i;

#if defined(NOCT_USE_MULTITHREAD)
	/* Make a GC safe point. */
	rt_gc_safepoint(env);
#endif

	/* Do JIT compilation if needed. */
	if (env->vm->config.jit_enable &&
	    func->jit_code == NULL &&
	    func->call_count != -1) {
		func->call_count++;
		if (func->call_count == env->vm->config.jit_threshold) {
			if (!jit_build(env, func)) {
				/* -1 means JIT failed. */
				func->call_count = -1;
			}

			/* Need to commit before call. */
			env->vm->is_jit_dirty = true;
		}
	}

	/* Commit JIT-compiled code for the first time compilation. */
	if (env->vm->config.jit_enable && env->vm->is_jit_dirty) {
		jit_commit(env);
		env->vm->is_jit_dirty = false;
	}

	/* Allocate a frame for this call. */
	if (!rt_enter_frame(env, func))
		return false;

	/* Pass args. */
	if (arg_count != func->param_count) {
		noct_error(env, N_TR("%s(): Function arguments not match."), func->name);
		return false;
	}
	for (i = 0; i < arg_count; i++)
		env->frame->tmpvar[i] = arg[i];

	/* Run. */
	if (func->cfunc != NULL) {
		/* Call an intrinsic or an FFI function implemented in C. */
		if (!func->cfunc(env))
			return false;
	} else {
		/* Backup the old file name. */
		strncpy(old_file_name, env->file_name, sizeof(old_file_name) - 1);

		/* Set the new file name. */
		strncpy(env->file_name, env->frame->func->file_name, sizeof(env->file_name) - 1);

		if (func->jit_code != NULL) {
			/* Call a JIT-generated code. */
			if (!func->jit_code(env)) {
				/*printf("Returned from JIT code (false).\n");*/
				return false;
			}
			/*printf("Returned from JIT code (true).\n");*/
			/*printf("%d: %d\n", env->frame->tmpvar[0].type, env->frame->tmpvar[0].val.i);*/
		} else {
			/* Call the bytecode interpreter. */
			if (!rt_visit_bytecode(env, func))
				return false;
		}

		/* Restore the old file name. */
		strncpy(env->file_name, old_file_name, sizeof(env->file_name) - 1);
	}

	/* Get a return value. */
	if (ret != NULL)
		*ret = env->frame->tmpvar[0];

	/* Commit JIT-compiled code for dynamically imported inside the function. */
	if (env->vm->config.jit_enable && env->vm->is_jit_dirty) {
		jit_commit(env);
		env->vm->is_jit_dirty = false;
	}

	/* Succeeded. */
	rt_leave_frame(env);

	return true;
}

/* Enter a new calling frame. */
static bool
rt_enter_frame(
	struct rt_env *env,
	struct rt_func *func)
{
	struct rt_frame *frame;

	if (++env->cur_frame_index >= RT_FRAME_MAX) {
		rt_error(env, N_TR("Stack overflow."));
		return false;
	}

	frame = &env->frame_alloc[env->cur_frame_index];
	env->frame = frame;
	frame->func = func;
	frame->tmpvar = &frame->tmpvar_alloc[0];
	frame->tmpvar_size = func->tmpvar_size;
	frame->pinned_count = 0;

	/* We can't remove this due to GC. */
	memset(frame->tmpvar, 0, sizeof(struct rt_value) * (size_t)frame->tmpvar_size);

	return true;
}

/* Leave the current calling frame. */
static void
rt_leave_frame(
	struct rt_env *env)
{
	if (--env->cur_frame_index < 0) {
		rt_error(env, N_TR("Stack underflow."));
		abort();
	}

	env->frame = &env->frame_alloc[env->cur_frame_index];
}

/*
 * String
 */

/*
 * Make a string value.
 */
bool
rt_make_string(
	struct rt_env *env,
	struct rt_value *val,
	const char *data)
{
	size_t len;
	uint32_t hash;
	struct rt_string *rts;

	len = strlen(data) + 1;
	hash = 0;
	if (!rt_make_string_with_hash(env, val, data, len, hash))
		return false;

	return true;
}

/*
 * Make a string value. (hash version)
 */
bool
rt_make_string_with_hash(
	struct rt_env *env,
	struct rt_value *val,
	const char *data,
	size_t len,		/* Including NUL */
	uint32_t hash)
{
	struct rt_string *rts;

	/* Allocate a string. */
	rts = rt_gc_alloc_string(env, data, len, hash);
	if (rts == NULL) {
		rt_out_of_memory(env);
		return false;
	}

	/*
	 * Here, this thread is "in-fligth" and GC won't be executed
	 * in other threads.
	 */

	/* Setup a value. */
	val->type = NOCT_VALUE_STRING;
	val->val.str = rts;

	return true;
}

/*
 * Cache the hash of a string.
 */
void
rt_cache_string_hash(
	struct rt_string *rts)
{
	if (rts->hash == 0)
		rts->hash = noct_string_hash(rts->data);
}

/*
 * Get a string hash. (FNV-1a)
 */
uint32_t
rt_string_hash(
	const char *s)
{
	uint32_t hash = 2166136261u;
	while (*s) {
		hash ^= (uint8_t)*s++;
		hash *= 16777619u;
	}
	return hash;
}

/*
 * Get a string hash and a length. (FNV-1a)
 */
void
rt_string_hash_and_len(
	const char *s,
	uint32_t *hash,
	uint32_t *len)
{
	*len = 0;
	*hash = 2166136261u;
	while (*s) {
		*hash ^= (uint8_t)*s++;
		*hash *= 16777619u;
		*len = *len + 1;
	}
}

/*
 * Arrays and Dictionaries
 */

/*
 * Make an empty array.
 */
bool
rt_make_empty_array(
	struct rt_env *env,
	struct rt_value *val)
{
	struct rt_array *arr;
	const uint32_t START_SIZE = 16;

	/* Allocate an array. */
	arr = rt_gc_alloc_array(env, START_SIZE);
	if (arr == NULL) {
		rt_out_of_memory(env);
		return false;
	}

	/*
	 * Here, this thread is "in-fligth" and GC won't be executed
	 * in other threads.
	 */

	/* Setup a value. */
	val->type = NOCT_VALUE_ARRAY;
	val->val.arr = arr;

	return true;
}

/*
 * Get the size of an array.
 */
bool
rt_get_array_size(
	struct rt_env *env,
	struct rt_array *arr,
	uint32_t *size)
{
	struct rt_array *real_arr;

	UNUSED_PARAMETER(env);

	assert(env != NULL);
	assert(arr != NULL);
	assert(size != NULL);

	/* Acquire the object with GC cooperation. */
	ACQUIRE_OBJ(arr, real_arr);

	/* Get the size. */
	*size = (uint32_t)real_arr->size;

	RELEASE_OBJ(real_arr);

	return true;
}

/*
 * Retrieves an array element.
 */
bool
rt_get_array_elem(
	struct rt_env *env,
	struct rt_array *arr,
	uint32_t index,
	struct rt_value *val)
{
	struct rt_array *real_arr;

	assert(env != NULL);
	assert(arr != NULL);
	assert(val != NULL);

	ACQUIRE_OBJ(arr, real_arr);

	/* Check the array boundary. */
	if (index >= real_arr->size) {
		RELEASE_OBJ(real_arr);

		rt_error(env, N_TR("Array index %d is out-of-range."), index);
		return false;
	}

	/* Load. */
	*val = real_arr->table[index];

	RELEASE_OBJ(real_arr);

	return true;
}

/*
 * Stores an value to an array.
 */
bool
rt_set_array_elem(
	struct rt_env *env,
	struct rt_array **arr,
	uint32_t index,
	NoctValue *val)
{
	struct rt_array *real_arr;

	assert(env != NULL);
	assert(arr != NULL);
	assert(*arr != NULL);
	assert(val != NULL);

	ACQUIRE_OBJ(*arr, real_arr);

	/* Expand the array if needed.. */
	if (index >= real_arr->alloc_size) {
		struct rt_array *new_arr;

		/* Reallocate an array. */
		if (!rt_expand_array(env, real_arr, arr, index + 1)) {
			RELEASE_OBJ(real_arr);
			return false;
		}

		/* Get the new array which is only visible to this thread. */
		new_arr = *arr;

		/* Set the new size. */
		new_arr->size = index + 1;

		/* Store. */
		new_arr->table[index] = *val;

		/* GC: Write barrier for the remember set. */
		if (val->type == NOCT_VALUE_STRING ||
		    val->type == NOCT_VALUE_ARRAY ||
		    val->type == NOCT_VALUE_DICT)
			rt_gc_array_write_barrier(env, new_arr, index, val);

		/* Publication is done by a release to the old array. */
		RELEASE_OBJ(real_arr);
		return true;
	}

	/* Store. */
	real_arr->table[index] = *val;
	if (index >= real_arr->size)
		real_arr->size = index + 1;

	/* GC: Write barrier for the remember set. */
	if (val->type == NOCT_VALUE_STRING ||
	    val->type == NOCT_VALUE_ARRAY ||
	    val->type == NOCT_VALUE_DICT)
		rt_gc_array_write_barrier(env, real_arr, index, val);

	RELEASE_OBJ(real_arr);
	return true;
}

/*
 * Resizes an array.
 */
bool
rt_resize_array(
	struct rt_env *env,
	struct rt_array **arr,
	uint32_t size)
{
	struct rt_array *real_arr;

	assert(env != NULL);
	assert(arr != NULL);

	ACQUIRE_OBJ(*arr, real_arr);

	if (size > real_arr->alloc_size) {
		struct rt_array *new_arr;

		/* Reallocate an array. */
		if (!rt_expand_array(env, real_arr, arr, size)) {
			RELEASE_OBJ(real_arr);
			return false;
		}

		/* Get the new array which is only visible to this thread.. */
		new_arr = *arr;

		/* Set the element count. */
		new_arr->size = size;

		/* Publication is done by a release to the old array. */
		RELEASE_OBJ(real_arr);
	} else {
		/* Remove (zero-fill) the reminder. */
		memset(&real_arr->table[size], 0, sizeof(struct rt_value) * (size_t)(real_arr->alloc_size - size));

		/* Set the element count. */
		real_arr->size = size;

		RELEASE_OBJ(real_arr);
	}

	return true;
}

/* Expand an array. */
static bool
rt_expand_array(
	struct rt_env *env,
	struct rt_array *old_arr,
	struct rt_array **new_arr_pp,
	size_t size)
{
	struct rt_array *new_arr;
	size_t old_size;
	uint32_t i;

	assert(env != NULL);
	assert(old_arr->newer == NULL);
	assert(old_arr->alloc_size < size);

	old_size = old_arr->alloc_size;

	/* Get the next size. */
	if (size < old_size * 2)
		size = old_size * 2;
	else
		size = size * 2;

	/* Allocate the new array. */
	new_arr = rt_gc_alloc_array(env, size);
	if (new_arr == NULL) {
		rt_out_of_memory(env);
		return false;
	}

	/* Copy the values with write barrier. */
	new_arr->size = old_arr->size;
	for (i = 0; i < old_arr->size; i++) {
		/* Copy. */
		new_arr->table[i] = old_arr->table[i];

		/* Write barrier. */
		rt_gc_array_write_barrier(env, new_arr, i, &new_arr->table[i]);
	}
	
	/* Set the forwaring pointer. */
	old_arr->newer = new_arr;

	/* Set the result. */
	*new_arr_pp = new_arr;

	return true;
}

/*
 * Make a shallow copy of an array.
 */
bool
rt_make_array_copy(
	struct rt_env *env,
	struct rt_array **dst,
	struct rt_array *src)
{
	struct rt_array *src_real;
	uint32_t i;

	assert(env != NULL);
	assert(dst != NULL);
	assert(src != NULL);

	ACQUIRE_OBJ(src, src_real);

	/* Allocate an array. */
	*dst = rt_gc_alloc_array(env, src_real->size);
	if (*dst == NULL) {
		RELEASE_OBJ(src_real);
		return false;
	}

	/*
	 * In this section, it is guaranteed that GC is not executed
	 * in other threads because this thread is "in-flight" and
	 * a GC execution waits for all threads become not in-flight.
	 */

	/* Copy the array with write-barrier. */
	(*dst)->size = src_real->size;
	for (i = 0; i < src_real->size; i++) {
		/* Copy. */
		(*dst)->table[i] = src_real->table[i];

		/* Write barrier. */
		rt_gc_array_write_barrier(env, *dst, i, &(*dst)->table[i]);
	}

	RELEASE_OBJ(src_real);

	return true;
}

/*
 * Make an empty dictionary.
 */
bool
rt_make_empty_dict(
	struct rt_env *env,
	struct rt_value *val)
{
	struct rt_dict *dict;

	const uint32_t START_SIZE = 2;

	/* Allocate a dictionary. */
	dict = rt_gc_alloc_dict(env, START_SIZE);
	if (dict == NULL) {
		rt_out_of_memory(env);
		return false;
	}

	/*
	 * Here, this thread is "in-fligth" and GC won't be executed
	 * in other threads.
	 */

	/* Setup a value. */
	val->type = NOCT_VALUE_DICT;
	val->val.dict = dict;

	return true;
}

/*
 * Get the size of a dictionary.
 */
bool
rt_get_dict_size(
	struct rt_env *env,
	struct rt_dict *dict,
	uint32_t *size)
{
	struct rt_dict *real_dict;

	UNUSED_PARAMETER(env);

	assert(env != NULL);
	assert(dict != NULL);
	assert(size != NULL);

	ACQUIRE_OBJ(dict, real_dict);

	/* Get the size. */
	*size = (uint32_t)real_dict->size;

	RELEASE_OBJ(real_dict);

	return true;
}

/*
 * Checks if a key exists in a dictionary.
 */
bool
rt_check_dict_key(
	struct rt_env *env,
	struct rt_dict *dict,
	const char *key,
	bool *ret)
{
	struct rt_dict *real_dict;
	size_t len;
	uint32_t hash, i, index;

	UNUSED_PARAMETER(env);

	ACQUIRE_OBJ(dict, real_dict);

	len = strlen(key) + 1; /* +1 for NUL */
	hash = rt_string_hash(key);
	index = hash & ((uint32_t)real_dict->alloc_size - 1);

	/* Search the key. */
	for (i = index;
	     i != ((index - 1 + (uint32_t)real_dict->alloc_size) & ((uint32_t)real_dict->alloc_size - 1));
	     i = (i + 1) & ((uint32_t)real_dict->alloc_size - 1)) {
		if (IS_DICT_KEY_REMOVED(real_dict->key[i]) ||
		    IS_DICT_KEY_EMPTY(real_dict->key[i]))
			continue;

		/* Make a hash cache. */
		if (real_dict->key[i].val.str->hash == 0)
			real_dict->key[i].val.str->hash = rt_string_hash(real_dict->key[i].val.str->data);

		if (real_dict->key[i].val.str->len == len &&
		    real_dict->key[i].val.str->hash == hash &&
		    strcmp(real_dict->key[i].val.str->data, key) == 0) {
			/* Found. */
			RELEASE_OBJ(real_dict);
			*ret = true;
			return true;
		}
	}

	/* Not found. */
	RELEASE_OBJ(real_dict);
	*ret = false;

	/* Note: this is not an error, so just return true. */
	return true;
}

/*
 * Get a dictionary key by index.
 */
bool
rt_get_dict_key_by_index(
	struct rt_env *env,
	struct rt_dict *dict,
	uint32_t index,
	struct rt_value *key)
{
	struct rt_dict *real_dict;
	uint32_t count, i;

	assert(env != NULL);
	assert(dict != NULL);
	assert(key != NULL);
	
	ACQUIRE_OBJ(dict, real_dict);

	/* Check the boundary. */
	if (index >= real_dict->size) {
		RELEASE_OBJ(real_dict);

		rt_error(env, N_TR("Dictionary index %d is out-of-range."), index);
		return false;
	}

	count = 0;
	for (i = 0; i < real_dict->alloc_size; i++) {
		if (IS_DICT_KEY_REMOVED(real_dict->key[i]) ||
		    IS_DICT_KEY_EMPTY(real_dict->key[i]))
			continue;
		if (count == index) {
			/* Load the key. */
			*key = real_dict->key[i];
			RELEASE_OBJ(real_dict);
			return true;
		}
		count++;
	}

	assert(NEVER_COME_HERE);

	RELEASE_OBJ(real_dict);

	return false;
}

/*
 * Get a dictionary value by index.
 */
bool
rt_get_dict_value_by_index(
	struct rt_env *env,
	struct rt_dict *dict,
	uint32_t index,
	struct rt_value *val)
{
	struct rt_dict *real_dict;
	uint32_t count, i;

	assert(env != NULL);
	assert(dict != NULL);
	assert(val != NULL);
	
	ACQUIRE_OBJ(dict, real_dict);

	/* Check the boundary. */
	if (index >= real_dict->size) {
		RELEASE_OBJ(real_dict);

		rt_error(env, N_TR("Dictionary index %d is out-of-range."), index);
		return false;
	}

	count = 0;
	for (i = 0; i < real_dict->alloc_size; i++) {
		if (IS_DICT_KEY_REMOVED(real_dict->key[i]) ||
		    IS_DICT_KEY_EMPTY(real_dict->key[i]))
			continue;
		if (count == index) {
			/* Load the value. */
			*val = real_dict->value[i];
			RELEASE_OBJ(real_dict);
			return true;
		}
		count++;
	}

	assert(NEVER_COME_HERE);

	RELEASE_OBJ(real_dict);

	return true;
}

/*
 * Retrieves the value by a key in a dictionary.
 */
bool
rt_get_dict_elem(
	struct rt_env *env,
	struct rt_dict *dict,
	const char *key,
	struct rt_value *val)
{
	struct rt_dict *real_dict;
	size_t len;
	uint32_t hash;

	/* This function is a wrapper and doesn't need a acquire/release. */

	len = strlen(key) + 1;
	hash = rt_string_hash(key);
	if (!rt_get_dict_elem_with_hash(env, dict, key, len, hash, val))
		return false;

	return true;
}

/*
 * Retrieves the value by a key in a dictionary.
 */
bool
rt_get_dict_elem_with_hash(
	struct rt_env *env,
	struct rt_dict *dict,
	const char *key,
	size_t len,
	uint32_t hash,
	struct rt_value *val)
{
	struct rt_dict *real_dict;
	uint32_t index, i;

	assert(env != NULL);
	assert(dict != NULL);
	assert(key != NULL);
	assert(val != NULL);
	assert(hash != 0);

	ACQUIRE_OBJ(dict, real_dict);

	index = hash & (uint32_t)(real_dict->alloc_size - 1);
	for (i = index;
	     i != ((index - 1 + real_dict->alloc_size) & (real_dict->alloc_size - 1));
	     i = (i + 1) & ((uint32_t)real_dict->alloc_size - 1)) {
		if (IS_DICT_KEY_REMOVED(real_dict->key[i]))
			continue;
		if (IS_DICT_KEY_EMPTY(real_dict->key[i]))
			break;

		/* Make a hash cache. */
		if (real_dict->key[i].val.str->hash == 0)
			real_dict->key[i].val.str->hash = rt_string_hash(real_dict->key[i].val.str->data);
		
		if (real_dict->key[i].val.str->len == len &&
		    real_dict->key[i].val.str->hash == hash &&
		    strcmp(real_dict->key[i].val.str->data, key) == 0) {
			/* Succeeded. */
			*val = real_dict->value[i];
			RELEASE_OBJ(real_dict);
			return true;
		}
	}

	/* Not found. */
	RELEASE_OBJ(real_dict);

	rt_error(env, N_TR("Dictionary key \"%s\" not found."), key);

	return false;
}

/*
 * Stores a key-value-pair to a dictionary.
 */
bool
rt_set_dict_elem(
	struct rt_env *env,
	struct rt_dict **dict,
	const char *key,
	struct rt_value *val)
{
	size_t len;
	uint32_t hash;

	/* This function is a wrapper and doesn't need a acquire/release. */

	len = strlen(key) + 1;	/* Including NUL. */
	hash = rt_string_hash(key);
	if (!rt_set_dict_elem_with_hash(env, dict, key, len, hash, val))
		return false;

	return true;
}

/*
 * Stores a key-value-pair to a dictionary. (hash version)
 */
bool
rt_set_dict_elem_with_hash(
	struct rt_env *env,
	struct rt_dict **dict,
	const char *key,
	size_t len,
	uint32_t hash,
	struct rt_value *val)
{
	struct rt_dict *real_dict, *append_dict;
	uint32_t index, i;

	assert(env != NULL);
	assert(dict != NULL);
	assert(*dict != NULL);
	assert(key != NULL);
	assert(val != NULL);
	assert(hash != 0);

	ACQUIRE_OBJ(*dict, real_dict);

	/* Search for the key to replace the value. */
	index = hash & ((uint32_t)real_dict->alloc_size - 1);
	for (i = index;
	     i != ((index - 1 + real_dict->alloc_size) & (real_dict->alloc_size - 1));
	     i = (i + 1) & ((uint32_t)real_dict->alloc_size - 1)) {
		if (IS_DICT_KEY_REMOVED(real_dict->key[i]) ||
		    IS_DICT_KEY_EMPTY(real_dict->key[i]))
			break;

		/* Make a hash cache. */
		if (real_dict->key[i].val.str->hash == 0)
			real_dict->key[i].val.str->hash = rt_string_hash(real_dict->key[i].val.str->data);

		if (real_dict->key[i].val.str->len == len &&
		    real_dict->key[i].val.str->hash == hash &&
		    strcmp(real_dict->key[i].val.str->data, key) == 0) {
			/* Found, replace the value. */
			real_dict->value[i] = *val;

			/* GC: Write barrier for the remember set. */
			if (val->type == NOCT_VALUE_STRING ||
			    val->type == NOCT_VALUE_ARRAY ||
			    val->type == NOCT_VALUE_DICT)
				rt_gc_dict_write_barrier(env, real_dict, val);

			RELEASE_OBJ(real_dict);

			return true;
		}
	}

	/* Key doesn't exist. Add new one. */

	/* Expand the size if 75% is used. */
	if (real_dict->size >= real_dict->alloc_size / 4 * 3) {
		/* Reallocate a dictionary. */
		if (!rt_expand_dict(env, real_dict, dict)) {
			RELEASE_OBJ(real_dict);
			return false;
		}

		/* Get the new dictionary which is only visible to this thread until a publication. */
		append_dict = *dict;
	} else {
		append_dict = real_dict;
	}
	assert(append_dict->size < append_dict->alloc_size);

	/* Append. */
	index = hash & ((uint32_t)append_dict->alloc_size - 1);
	for (i = index;
	     i != ((index - 1 + append_dict->alloc_size) & (append_dict->alloc_size - 1));
	     i = (i + 1) & ((uint32_t)append_dict->alloc_size - 1)) {
		if (IS_DICT_KEY_REMOVED(append_dict->key[i]) ||
		    IS_DICT_KEY_EMPTY(append_dict->key[i])) {
			/* Make a key value. */
			if (!rt_make_string_with_hash(env, &append_dict->key[i], key, len, hash)) {
				RELEASE_OBJ(real_dict);
				return false;
			}

			append_dict->value[i] = *val;

			/* GC: Write barrier for the remember set. */
			if (val->type == NOCT_VALUE_STRING ||
			    val->type == NOCT_VALUE_ARRAY ||
			    val->type == NOCT_VALUE_DICT) {
				rt_gc_dict_write_barrier(env, append_dict, &append_dict->key[i]);
				rt_gc_dict_write_barrier(env, append_dict, val);
			}

			append_dict->size++;
			break;
		}
	}

	/*
	 * Release real_dict, also do publication for append_dict.
	 * In case of expand, the new dictionaty will appear to other threads.
	 */
	RELEASE_OBJ(real_dict);

	return true;
}

/* Expand an array. */
static bool
rt_expand_dict(
	struct rt_env *env,
	struct rt_dict *old_dict,
	struct rt_dict **new_dict_pp)
{
	struct rt_dict *new_dict;
	size_t old_size, new_size;
	uint32_t index, i, j;

	assert(env != NULL);
	assert(old_dict != NULL);
	assert(old_dict->newer == NULL);
	assert(new_dict_pp != NULL);

	old_size = old_dict->alloc_size;
	new_size = old_size * 2;

	/* Allocate the new array. */
	new_dict = rt_gc_alloc_dict(env, new_size);
	if (new_dict == NULL) {
		rt_out_of_memory(env);
		return false;
	}

	/* Rehash. (Copy the values with write barrier.) */
	for (i = 0; i < old_size; i++) {
		if (IS_DICT_KEY_REMOVED(old_dict->key[i]) || IS_DICT_KEY_EMPTY(old_dict->key[i]))
			continue;

		index = rt_string_hash(old_dict->key[i].val.str->data) & ((uint32_t)new_dict->alloc_size - 1);
		for (j = index;
		     j != ((index - 1 + new_dict->alloc_size) & (new_dict->alloc_size - 1));
		     j = (j + 1) & ((uint32_t)new_dict->alloc_size - 1)) {
			if (IS_DICT_KEY_EMPTY(new_dict->key[j])) {
				/* Copy the key and values. */
				new_dict->key[j] = old_dict->key[i];
				new_dict->value[j] = old_dict->value[i];

				/* Write barrier. */
				rt_gc_dict_write_barrier(env, new_dict, &new_dict->key[j]);
				rt_gc_dict_write_barrier(env, new_dict, &new_dict->value[j]);
				break;
			}
		}
	}
	new_dict->size = old_dict->size;

	/* Set the forwarding pointer. */
	old_dict->newer = new_dict;

	/* Set the result */
	*new_dict_pp = new_dict;

	return true;
}

/*
 * Remove a dictionary key.
 */
bool
rt_remove_dict_elem(
	struct rt_env *env,
	struct rt_dict *dict,
	const char *key)
{
	size_t len;
	uint32_t hash;

	/* This function is a wrapper and doesn't need a acquire/release. */

	len = strlen(key) + 1;	/* Including NUL. */
	hash = rt_string_hash(key);
	if (!rt_remove_dict_elem_with_hash(env, dict, key, len, hash))
		return false;

	return true;
}

/*
 * Remove a dictionary key. (hash version)
 */
bool
rt_remove_dict_elem_with_hash(
	struct rt_env *env,
	struct rt_dict *dict,
	const char *key,
	size_t len,
	uint32_t hash)
{
	struct rt_dict *real_dict;
	uint32_t index, i;

	assert(env != NULL);
	assert(dict != NULL);
	assert(key != NULL);
	assert(hash != 0);

	ACQUIRE_OBJ(dict, real_dict);

	/* Search for the key. */
	index = hash & ((uint32_t)real_dict->alloc_size - 1);
	for (i = index;
	     i != ((index - 1 + real_dict->alloc_size) & (real_dict->alloc_size - 1));
	     i = (i + 1) & ((uint32_t)real_dict->alloc_size - 1)) {
		if (IS_DICT_KEY_REMOVED(real_dict->key[i]))
			continue;
		if (IS_DICT_KEY_EMPTY(real_dict->key[i]))
			break;

		/* Make a hash cache. */
		if (real_dict->key[i].val.str->hash == 0)
			real_dict->key[i].val.str->hash = rt_string_hash(real_dict->key[i].val.str->data);
		
		if (real_dict->key[i].val.str->len == len &&
		    real_dict->key[i].val.str->hash == hash &&
		    strcmp(real_dict->key[i].val.str->data, key) == 0) {
			REMOVE_DICT_KEY(real_dict->key[i]);
			real_dict->value[i].type = NOCT_VALUE_INT;
			real_dict->value[i].val.i = 0;

			/* Succeeded. */
			RELEASE_OBJ(real_dict);
			return true;
		}
	}

	/* Not found. */
	RELEASE_OBJ(real_dict);
	rt_error(env, N_TR("Dictionary key \"%s\" not found."), key);
	return false;
}

/*
 * Make a shallow copy of a dictionary.
 */
bool
rt_make_dict_copy(
	struct rt_env *env,
	struct rt_dict **dst,
	struct rt_dict *src)
{
	struct rt_dict *d, *real_d, *src_real;
	int i;

	assert(env != NULL);
	assert(src != NULL);
	assert(dst != NULL);

	/* Acquire src. */
	ACQUIRE_OBJ(src, src_real);

	/* Make a dictionary. */
	d = rt_gc_alloc_dict(env, src_real->alloc_size);
	if (d == NULL) {
		RELEASE_OBJ(src_real);
		return false;
	}

	/*
	 * Here, this thread is "in-fligth" and GC won't be executed
	 * in other threads.
	 */

	/* Copy the array with write-barrier. */
	d->size = src_real->size;
	for (i = 0; i < (int)src_real->alloc_size; i++) {
		if (!IS_DICT_KEY_REMOVED(src_real->key[i]) &&
		    !IS_DICT_KEY_EMPTY(src_real->key[i])) {
			/* Copy the key and value. */
			d->key[i] = src_real->key[i];
			d->value[i] = src_real->value[i];

			/* Write barrier. */
			rt_gc_dict_write_barrier(env, d, &d->key[i]);
			rt_gc_dict_write_barrier(env, d, &d->value[i]);
		}
	}

	RELEASE_OBJ(src_real);

	*dst = d;

	return true;
}

/*
 * Merges a dictionary.
 */
bool
rt_merge_dict(
	struct rt_env *env,
	struct rt_dict *dst,
	struct rt_dict *src)
{
	struct rt_dict *real_src, *real_dst, *orig_real_dst, *new_dict;
	const char *key;
	uint32_t i, j, index, len, hash;
	bool is_replaced;

	/* Acquire 2 dictionaries. */
	ACQUIRE_OBJ2(src, real_src, dst, real_dst);

	/*
	 * Save real_dst because it will be overwritten to a new one
	 * when expanded, and we must do release to the original one.
	 */
	orig_real_dst = real_dst;

	for (i = 0; i < real_src->alloc_size; i++) {
		if (IS_DICT_KEY_REMOVED(real_src->key[i]) ||
		    IS_DICT_KEY_EMPTY(real_src->key[i]))
			continue;

		len = real_src->key[i].val.str->len;
		hash = real_src->key[i].val.str->hash;
		key = real_src->key[i].val.str->data;

		/* Search for the key to replace the value. */
		index = hash & ((uint32_t)real_dst->alloc_size - 1);
		is_replaced = false;
		for (j = index;
		     j != ((index - 1 + real_dst->alloc_size) & (real_dst->alloc_size - 1));
		     j = (j + 1) & ((uint32_t)real_dst->alloc_size - 1)) {
			if (IS_DICT_KEY_REMOVED(real_dst->key[j]) ||
			    IS_DICT_KEY_EMPTY(real_dst->key[j]))
				break;

			/* Make a hash cache. */
			if (real_dst->key[j].val.str->hash == 0)
				real_dst->key[j].val.str->hash = rt_string_hash(real_dst->key[j].val.str->data);

			if (real_dst->key[j].val.str->len == len &&
			    real_dst->key[j].val.str->hash == hash &&
			    strcmp(real_dst->key[j].val.str->data, key) == 0) {
				/* Found, replace the value. */
				real_dst->value[j] = real_src->value[i];

				/* GC: Write barrier for the remember set. */
				if (real_src->value[i].type == NOCT_VALUE_STRING ||
				    real_src->value[i].type == NOCT_VALUE_ARRAY ||
				    real_src->value[i].type == NOCT_VALUE_DICT) {
					rt_gc_dict_write_barrier(env, real_dst, &real_dst->key[j]);
					rt_gc_dict_write_barrier(env, real_dst, &real_dst->value[j]);
				}

				is_replaced = true;
				break;
			}
		}
		if (is_replaced)
			continue;

		/* Key doesn't exist. Add new one. */

		/* Expand the size if 75% is used. */
		if (real_dst->size >= real_dst->alloc_size / 4 * 3) {
			/* Reallocate a dictionary. */
			if (!rt_expand_dict(env, real_dst, &new_dict)) {
				RELEASE_OBJ2(real_src, real_dst);
				return false;
			}
			real_dst = new_dict;
		}

		/* Append. */
		index = hash & ((uint32_t)real_dst->alloc_size - 1);
		for (j = index;
		     j != ((index - 1 + real_dst->alloc_size) & (real_dst->alloc_size - 1));
		     j = (j + 1) & ((uint32_t)real_dst->alloc_size - 1)) {
			if (IS_DICT_KEY_REMOVED(real_dst->key[j]) ||
			    IS_DICT_KEY_EMPTY(real_dst->key[j])) {
				/* Make a key value. */
				if (!rt_make_string_with_hash(env, &real_dst->key[j], key, len, hash)) {
					RELEASE_OBJ2(real_src, real_dst);
					return false;
				}
				real_dst->value[j] = real_src->value[i];

				/* GC: Write barrier for the remember set. */
				if (real_src->value[i].type == NOCT_VALUE_STRING ||
				    real_src->value[i].type == NOCT_VALUE_ARRAY ||
				    real_src->value[i].type == NOCT_VALUE_DICT) {
					rt_gc_dict_write_barrier(env, real_dst, &real_dst->key[j]);
					rt_gc_dict_write_barrier(env, real_dst, &real_dst->value[j]);
				}

				break;
			}
		}
		real_dst->size++;
	}

	/*
	 * Release real_src and real_dst.
	 * In case of expand, the new dictionaty will appear to other threads. (publication)
	 */
	RELEASE_OBJ2(real_src, orig_real_dst);

	return true;
}

/*
 * Sets the native pointers to a dictionary.
 */
bool
rt_set_dict_native_pointer(
	struct rt_env *env,
	struct rt_dict *dict,
	void *native_pointer,
	void (*native_finalizer)(void *native_pointer))
{
	struct rt_dict *real_dict;

	ACQUIRE_OBJ(dict, real_dict);

	real_dict->native_pointer = native_pointer;
	real_dict->native_finalizer = native_finalizer;

	RELEASE_OBJ(real_dict);

	return true;
}

/*
 * Gets the native pointer from a dictionary.
 */
bool
rt_get_dict_native_pointer(
	struct rt_env *env,
	struct rt_dict *dict,
	void **native_pointer,
	void (**native_finalizer)(void *native_pointer))
{
	struct rt_dict *real_dict;

	ACQUIRE_OBJ(dict, real_dict);

	*native_pointer = real_dict->native_pointer;
	*native_finalizer = real_dict->native_finalizer;

	RELEASE_OBJ(real_dict);

	return true;
}

/*
 * Global Variable
 */

#if !defined(NOCT_USE_MULTITHREAD)

#define ACQUIRE_GLOBAL()
#define RELEASE_GLOBAL()

#else

#define ACQUIRE_GLOBAL()									\
	while (1) {										\
		int old = atomic_fetch_add_acquire(&env->vm->global_var_counter, 1);		\
		if (old == 0)									\
			break;									\
		atomic_fetch_sub_release(&env->vm->global_var_counter, 1);			\
	}

#define RELEASE_GLOBAL()									\
	atomic_fetch_sub_release(&env->vm->global_var_counter, 1);

#endif

/* Initialize the global variables. */
static bool
rt_init_global(
	struct rt_env *env)
{
	const uint32_t START_SIZE = 2;

	assert(env->vm->global == NULL);

	/* Allocate the table. */
	env->vm->global = noct_calloc(sizeof(struct rt_bindglobal) * START_SIZE, 1);
	if (env->vm->global == NULL) {
		rt_out_of_memory(env);
		return false;
	}

	env->vm->global_alloc_size = START_SIZE;
	env->vm->global_size = 0;

	return true;
}

/* Cleanup the global variables. */
static void
rt_cleanup_global(
	struct rt_env *env)
{
	uint32_t i;

	assert(env->vm->global != NULL);

	for (i = 0; i < env->vm->global_alloc_size; i++) {
		if (env->vm->global[i].name != NULL) {
			free(env->vm->global[i].name);
			env->vm->global[i].name = NULL;
		}
	}
	free(env->vm->global);
	env->vm->global = NULL;
}

/*
 * Check if a global variable exists.
 */
bool
rt_check_global(
	struct rt_env *env,
	const char *name)
{
	uint32_t index, i, len, hash;

	ACQUIRE_GLOBAL();

	rt_string_hash_and_len(name, &hash, &len);
	len++;	/* Including NUL. */

	index = hash & ((uint32_t)env->vm->global_alloc_size - 1) ;
	for (i = index;
	     i != ((index - 1 + env->vm->global_alloc_size) & (env->vm->global_alloc_size - 1));
	     i = (i + 1) & (env->vm->global_alloc_size - 1)) {
		if (env->vm->global[i].is_removed)
			continue;
		if (env->vm->global[i].name == NULL) {
			/* Not found. */
			RELEASE_GLOBAL();
			return false;
		}
		if (env->vm->global[i].name_len != len)
			continue;
		if (env->vm->global[i].name_hash != hash)
			continue;
		if (strcmp(env->vm->global[i].name, name) != 0)
			continue;

		/* Found. */
		RELEASE_GLOBAL();
		return true;
	}

	/* Not found. */
	RELEASE_GLOBAL();
	return false;
}

/*
 * Get a global variable.
 */
bool
rt_get_global(
	struct rt_env *env,
	const char *name,
	struct rt_value *val)
{
	size_t len;
	uint32_t hash;

	len = strlen(name) + 1;
	hash = rt_string_hash(name);

	if (!rt_get_global_with_hash(env, name, len, hash, val))
		return false;

	return true;
}

/*
 * Get a global variable. (hash version)
 */
bool
rt_get_global_with_hash(
	struct rt_env *env,
	const char *name,
	size_t len,
	uint32_t hash,
	struct rt_value *val)
{
	uint32_t index, i;

	ACQUIRE_GLOBAL();

	index = hash & ((uint32_t)env->vm->global_alloc_size - 1) ;
	for (i = index;
	     i != ((index - 1 + env->vm->global_alloc_size) & (env->vm->global_alloc_size - 1));
	     i = (i + 1) & (env->vm->global_alloc_size - 1)) {
		if (env->vm->global[i].is_removed)
			continue;
		if (env->vm->global[i].name == NULL)
			break;
		if (env->vm->global[i].name_len != len)
			continue;
		if (env->vm->global[i].name_hash != hash)
			continue;
		if (strcmp(env->vm->global[i].name, name) != 0)
			continue;

		/* Found. */
		*val = env->vm->global[i].val;
		RELEASE_GLOBAL();
		return true;
	}

	/* Not found. */
	RELEASE_GLOBAL();
	rt_error(env, N_TR("Symbol \"%s\" not found."), name);
	return false;
}

/*
 * Set a global variable.
 */
bool
rt_set_global(
	struct rt_env *env,
	const char *name,
	struct rt_value *val)
{
	size_t len;
	uint32_t hash;

	len = strlen(name) + 1;	/* Including NUL. */
	hash = rt_string_hash(name);
	if (!rt_set_global_with_hash(env, name, len, hash, val))
		return false;

	return true;
}

/*
 * Set a global variable.
 */
bool
rt_set_global_with_hash(
	struct rt_env *env,
	const char *name,
	size_t len,		/* Including NUL. */
	uint32_t hash,
	struct rt_value *val)
{
	uint32_t index, i;

	ACQUIRE_GLOBAL();

	/* Reisze if 75% is used. */
	if (env->vm->global_size >= env->vm->global_alloc_size / 4 * 3) {
		if (!rt_expand_global(env))
			return false;
	}

	/* Search a place to insert or overwrite. */
	index = hash & ((uint32_t)env->vm->global_alloc_size - 1) ;
	for (i = index;
	     i != ((index - 1 + env->vm->global_alloc_size) & (env->vm->global_alloc_size - 1));
	     i = (i + 1) & (env->vm->global_alloc_size - 1)) {
		/* If found an empty entry. */
		if (env->vm->global[i].is_removed ||
		    env->vm->global[i].name == NULL) {
			/* Insert a new entry. */
			env->vm->global[i].name = strdup(name);
			if (env->vm->global[i].name == NULL) {
				RELEASE_GLOBAL();
				rt_out_of_memory(env);
				return false;
			}
			env->vm->global[i].name_len = (uint32_t)len;
			env->vm->global[i].name_hash = hash;
			env->vm->global[i].val = *val;
			env->vm->global_size++;
			RELEASE_GLOBAL();
			return true;
		}

		/* If found an existing entry. */
		if (env->vm->global[i].name_len != len)
			continue;
		if (env->vm->global[i].name_hash != hash)
			continue;
		if (strcmp(env->vm->global[i].name, name) == 0) {
			/* Overwrite the existing entry value. */
			env->vm->global[i].val = *val;
			RELEASE_GLOBAL();
			return true;
		}
	}

	/* No empty entry. */
	assert(NEVER_COME_HERE);
	RELEASE_GLOBAL();
	return false;
}

/* Expand the global variable table. */
static bool
rt_expand_global(
	struct rt_env *env)
{
	struct rt_bindglobal *old_tbl,*new_tbl;
	uint32_t old_size, new_size, i, j, index;

	/* Allocate the new table. */
	old_size = env->vm->global_alloc_size;
	old_tbl = env->vm->global;
	new_size = old_size * 2;
	new_tbl = noct_calloc(sizeof(struct rt_bindglobal) * new_size, 1);
	if (new_tbl == NULL) {
		rt_out_of_memory(env);
		return false;
	}

	/* Rehash (copy). */
	for (i = 0; i < old_size; i++) {
		if (old_tbl[i].name == NULL || old_tbl[i].is_removed)
			continue;
		index = rt_string_hash(old_tbl[i].name) & (new_size - 1) ;
		for (j = index;
		     j != ((index - 1 + new_size) & (new_size - 1));
		     j = (j + 1) & (new_size - 1)) {
			if (new_tbl[j].name == NULL) {
				new_tbl[j].name = old_tbl[i].name;
				new_tbl[j].name_len = old_tbl[i].name_len;
				new_tbl[j].name_hash = old_tbl[i].name_hash;
				new_tbl[j].val = old_tbl[i].val;
				break;
			}
		}
	}

	free(old_tbl);
	env->vm->global = new_tbl;
	env->vm->global_alloc_size = new_size;

	return true;
}

/*
 * Native API Pin
 */

/*
 * Pins a C global variable.
 */
bool
rt_pin_global(
	struct rt_env *env,
	struct rt_value *val)
{
	assert(env != NULL);
	assert(val != NULL);

	if (!rt_gc_pin_global(env, val))
		return false;

	return true;
}

/*
 * Unpins a C global variable.
 */
bool
rt_unpin_global(
	struct rt_env *env,
	struct rt_value *val)
{
	assert(env != NULL);
	assert(val != NULL);

	if (!rt_gc_unpin_global(env, val))
		return false;

	return true;
}

/*
 * Pin a C local variable.
 */
bool
rt_pin_local(
	struct rt_env *env,
	struct rt_value *val)
{
	assert(env != NULL);
	assert(val != NULL);

	if (!rt_gc_pin_local(env, val))
		return false;

	return true;
}

/*
 * Unpin a C local variable.
 */
bool
rt_unpin_local(
	struct rt_env *env,
	struct rt_value *val)
{
	assert(env != NULL);
	assert(val != NULL);

	if (!rt_gc_unpin_local(env, val))
		return false;

	return true;
}

/*
 * Error Handling
 */

/*
 * Get an error message.
 */
const char *
rt_get_error_message(
	struct rt_env *env)
{
	return &env->error_message[0];
}

/*
 * Get an error file name.
 */
const char *
rt_get_error_file(
	struct rt_env *env)
{
	return &env->file_name[0];
}

/*
 * Get an error line number.
 */
int
rt_get_error_line(
	struct rt_env *env)
{
	return env->line;
}

/*
 * Output an error message.
 */
void
rt_error(
	struct rt_env *env,
	const char *msg,
	...)
{
	va_list ap;

	va_start(ap, msg);
	vsnprintf(env->error_message, sizeof(env->error_message) - 1, msg, ap);
	va_end(ap);
}

/*
 * Output an out-of-memory message.
 */
void
rt_out_of_memory(
	struct rt_env *env)
{
	noct_error(env, N_TR("Out of memory."));
}
