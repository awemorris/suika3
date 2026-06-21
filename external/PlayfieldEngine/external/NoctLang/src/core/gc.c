/* -*- coding: utf-8; tab-width: 8; indent-tabs-mode: t; -*- */

/*
 * Noct Programming Language
 * Copyright (c) 2025, 2026, Awe Morris
 */

/*
 * Garbage Collector
 */

#include "runtime.h"
#include "gc.h"
#include "atomic.h"
#include "objectmodel.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <assert.h>

/*
 * False Assertion
 */
#define NEVER_COME_HERE		0
#define PINNED_VAR_NOT_FOUND	0

/*
 * Link an element to a list.
 */
#define INSERT_TO_LIST(elem, list, prev, next)	\
	do {					\
		(elem)->prev = NULL;		\
		(elem)->next = (list);		\
		if ((list) != NULL) {		\
			assert(elem != (list)->next); \
			(list)->prev = (elem);	\
		}				\
		(list) = (elem);		\
	} while (0)

/*
 * Unlink an element from a list.
 */
#define UNLINK_FROM_LIST(elem, list, prev, next)			\
	do {								\
		if ((elem)->prev != NULL) {				\
			(elem)->prev->next = (elem)->next;		\
			if ((elem)->next != NULL)			\
				(elem)->next->prev = (elem)->prev;	\
		} else {						\
			if ((elem)->next != NULL)			\
				(elem)->next->prev = NULL;		\
			(list) = (elem)->next;				\
		}							\
		(elem)->prev = NULL;					\
		(elem)->next = NULL;					\
	} while (0)

/*
 * Check if a value is a reference type.
 */
#define IS_REF_VAL(v)			((v)->type >= NOCT_VALUE_STRING && (v)->type <= NOCT_VALUE_DICT)

/*
 * Check if an object is in the nursery or graduate region.
 */
#define IS_YOUNG_OBJ(o)			((o)->region < RT_GC_REGION_TENURE)

/*
 * Forward declaration.
 */
static struct rt_string *rt_gc_alloc_string_graduate(struct rt_env *env, const char *data, size_t len, uint32_t hash);
static struct rt_string *rt_gc_alloc_string_tenure(struct rt_env *env, const char *data, size_t len, uint32_t hash);
static struct rt_array *rt_gc_alloc_array_graduate(struct rt_env *env, size_t size);
static struct rt_array *rt_gc_alloc_array_tenure(struct rt_env *env, size_t size);
static struct rt_dict *rt_gc_alloc_dict_graduate(struct rt_env *env, size_t size);
static struct rt_dict *rt_gc_alloc_dict_tenure(struct rt_env *env, size_t size);
static struct rt_packed *rt_gc_alloc_packed_graduate(struct rt_env *env, int type, size_t size, size_t elem_size, void *preallocated);
static struct rt_packed *rt_gc_alloc_packed_tenure(struct rt_env *env, int type, size_t size, size_t elem_size, void *preallocated);
static void rt_gc_young_gc(struct rt_env *env);
static void rt_gc_young_gc_body(struct rt_env *env);
static bool rt_gc_copy_young_object_recursively(struct rt_env *env, struct rt_gc_object **obj);
static void rt_gc_array_dict_follow_newer(struct rt_env *env, struct rt_gc_object **obj);
static struct rt_gc_object *rt_gc_promote_object(struct rt_env *env, struct rt_gc_object *obj);
static struct rt_gc_object *rt_gc_promote_string(struct rt_env *env, struct rt_gc_object *obj);
static struct rt_gc_object *rt_gc_promote_array(struct rt_env *env, struct rt_gc_object *obj);
static struct rt_gc_object *rt_gc_promote_dict(struct rt_env *env, struct rt_gc_object *obj);
static struct rt_gc_object *rt_gc_promote_packed(struct rt_env *env, struct rt_gc_object *obj);
static struct rt_gc_object *rt_gc_copy_string_to_graduate(struct rt_env *env, struct rt_string *old_obj);
static struct rt_gc_object *rt_gc_copy_array_to_graduate(struct rt_env *env, struct rt_array *old_obj);
static struct rt_gc_object *rt_gc_copy_dict_to_graduate(struct rt_env *env, struct rt_dict *old_obj);
static struct rt_gc_object *rt_gc_copy_packed_to_graduate(struct rt_env *env, struct rt_packed *old_obj);
static void rt_gc_old_gc(struct rt_env *env);
static void rt_gc_old_gc_body(struct rt_env *env);
static void rt_gc_mark_old_object_recursively(struct rt_env *env, struct rt_gc_object **obj);
static void rt_gc_free_old_object(struct rt_env *env, struct rt_gc_object *obj);
static bool rt_gc_compact_gc(struct rt_env *env);
static void rt_gc_update_tenure_ref(struct rt_env *env, struct rt_gc_object **obj);
static void rt_gc_update_tenure_ref_recursively(struct rt_env *env, struct rt_gc_object **obj);
static void *nursery_alloc(struct rt_env *env, size_t size);
static void *graduate_alloc(struct rt_env *env, size_t size);
static void *rt_gc_tenure_alloc(struct rt_env *env, size_t size);
static void rt_gc_tenure_free(struct rt_env *env, void *p);

/*
 * Initializes the garbage collector and allocate memory regions.
 */
bool
rt_gc_init(
	struct rt_vm *vm)
{
	memset(&vm->gc, 0, sizeof(struct rt_gc_info));

	/* Initialize the nursery allocator. */
	if (!arena_init(&vm->gc.nursery_arena, vm->config.gc_nursery_size))
		return false;

	/* Initialize the graduate allocators. */
	if (!arena_init(&vm->gc.graduate_arena[0], vm->config.gc_graduate_size))
		return false;
	if (!arena_init(&vm->gc.graduate_arena[1], vm->config.gc_graduate_size))
		return false;
	vm->gc.cur_grad_from = 0;
	vm->gc.cur_grad_to = 1;

	/* Initialize the tenure allocator.  */
	vm->gc.tenure_freelist.top = noct_calloc(1, vm->config.gc_tenure_size);
	if (vm->gc.tenure_freelist.top == NULL)
		return false;
	vm->gc.tenure_freelist.end = vm->gc.tenure_freelist.top + vm->config.gc_tenure_size;

	return true;
}

void
rt_gc_cleanup(
	struct rt_vm *vm)
{
	UNUSED_PARAMETER(vm);
}

/*
 * Cleanups the garbage collector and deallocate memory regions.
 */
void env_gc_cleanup(struct rt_vm *vm)
{
	/* Cleanup the nursery allocator. */
	arena_cleanup(&vm->gc.nursery_arena);

	/* Cleanup the graduate allocators. */
	arena_cleanup(&vm->gc.graduate_arena[0]);
	arena_cleanup(&vm->gc.graduate_arena[1]);

	/* Cleanup the tenure allocator. */
	noct_free(vm->gc.tenure_freelist.top);
}

/*
 * Allocates a string object in the appropriate region.
 */
struct rt_string *
rt_gc_alloc_string(
	struct rt_env *env,
	const char *data,
	size_t len,
	uint32_t hash)
{
	struct rt_string *rts;
	char *s;
	int retry;

	/* Check for overflow. */
	if (len > SIZE_MAX - sizeof(struct rt_string)) {
		rt_out_of_memory(env);
		return NULL;
	}

	/*
	 * [Large Object Promotion]
	 *  - If the string is large, allocate in the tenure region.
	 */
	if (len >= env->vm->config.gc_lop_threshold)
		return rt_gc_alloc_string_tenure(env, data, len, hash);

	/* Allocate in the nursery region. */
	for (retry = 0; retry <= 1; retry++) {
		/* Allocate a rt_string buffer. */
		rts = nursery_alloc(env, sizeof(struct rt_string) + len);
		if (rts == NULL) {
			/* Retry. */
			if (retry == 0) {
				rt_gc_young_gc(env);
				continue;
			} else {
				rt_out_of_memory(env);
				return NULL;
			}
		}

		/* Get the string top address. */
		s = (char *)rts + sizeof(struct rt_string);

		/* Copy the string. */
		memcpy(s, data, len);

		/* Setup the struct. */
		memset(&rts->head, 0, sizeof(struct rt_gc_object));
		rts->head.type = RT_GC_TYPE_STRING;
		rts->head.region = RT_GC_REGION_NURSERY;
		rts->head.size = sizeof(struct rt_string) + len;
		INSERT_TO_LIST(&rts->head, env->vm->gc.nursery_list, prev, next);
		rts->data = s;
		rts->len = len;
		rts->hash = hash;

		/* Succeeded. */
		return rts;
	}

	/* Failed. */
	rt_out_of_memory(env);
	return NULL;
}

/* Allocates a string object in the graduate region. */
static struct rt_string *
rt_gc_alloc_string_graduate(
	struct rt_env *env,
	const char *data,
	size_t len,
	uint32_t hash)
{
	struct rt_string *rts;
	char *s;

	/* Check for overflow. */
	if (len > SIZE_MAX - sizeof(struct rt_string)) {
		rt_out_of_memory(env);
		return NULL;
	}

	/*
	 * This function is only called from the young GC,
	 * and thus, we don't use young GC for a retry here.
	 */

	do {
		/* Try allocating a rt_string buffer in the graduate region. */
		rts = graduate_alloc(env, sizeof(struct rt_string) + len);
		if (rts == NULL)
			break;

		/* Get the string top address. */
		s = (char *)rts + sizeof(struct rt_string);

		/* Copy the string. */
		memcpy(s, data, len);

		/* Setup the struct. */
		memset(&rts->head, 0, sizeof(struct rt_gc_object));
		rts->head.type = RT_GC_TYPE_STRING;
		rts->head.region = RT_GC_REGION_GRADUATE;
		rts->head.size = sizeof(struct rt_string) + len;
		INSERT_TO_LIST(&rts->head, env->vm->gc.graduate_new_list, prev, next);
		rts->data = s;
		rts->len = len;
		rts->hash = hash;

		/* Succeeded. (graduate) */
		return rts;
	} while (0);

	/* Failed. Try allocating in the tenure region. */
	rts = rt_gc_alloc_string_tenure(env, data, len, hash);
	if (rts == NULL)
		return NULL;

	/* Succeeded. (tenure) */
	return rts;
}

/* Allocates a large string in the tenure region. */
static struct rt_string *
rt_gc_alloc_string_tenure(
	struct rt_env *env,
	const char *data,
	size_t len,
	uint32_t hash)
{
	struct rt_string *rts;
	char *s;
	int retry;

	/* Check for overflow. */
	if (len > SIZE_MAX - sizeof(struct rt_string)) {
		rt_out_of_memory(env);
		return NULL;
	}

	if (sizeof(struct rt_string) + len >= env->vm->config.gc_tenure_size) {
		rt_out_of_memory(env);
		return NULL;
	}

	/* Allocate in the tenure region. */
	for (retry = 0; retry <= 2; retry++) {
		/* Allocate a rt_string buffer. */
		rts = rt_gc_tenure_alloc(env, sizeof(struct rt_string) + len);
		if (rts == NULL) {
			/* Retry. */
			if (retry == 0) {
				rt_gc_old_gc(env);
				continue;
			} else if (retry == 1) {
				rt_gc_compact_gc(env);
				continue;
			} else {
				rt_out_of_memory(env);
				return NULL;
			}
		}

		/* Get the string top address. */
		s = (char *)rts + sizeof(struct rt_string);

		/* Copy the string. */
		memcpy(s, data, len);

		/* Setup the struct. */
		memset(&rts->head, 0, sizeof(struct rt_gc_object));
		rts->head.type = RT_GC_TYPE_STRING;
		rts->head.region = RT_GC_REGION_TENURE;
		rts->head.size = sizeof(struct rt_string) + len;
		INSERT_TO_LIST(&rts->head, env->vm->gc.tenure_list, prev, next);
		rts->data = s;
		rts->len = len;
		rts->hash = hash;

		/* Succeeded. */
		return rts;
	}

	/* Failed. */
	rt_out_of_memory(env);
	return NULL;
}

/*
 * Allocates an array object in the appropriate region.
 */
struct rt_array *
rt_gc_alloc_array(
	struct rt_env *env,
	size_t size)
{
	struct rt_array *arr;
	size_t len;
	struct rt_value *table;
	int retry;

	assert(env != NULL);
	assert(size > 0);

	/* Check for overflow. */
	if (size >= SIZE_MAX / sizeof(struct rt_value)) {
		rt_out_of_memory(env);
		return NULL;
	}
	if (size > (SIZE_MAX - sizeof(struct rt_array)) / sizeof(struct rt_value)) {
		rt_out_of_memory(env);
		return NULL;
	}

	/*
	 * [Large Object Promotion]
	 *  - If the array is large, allocate in the tenure region.
	 */
	len = size * sizeof(struct rt_value);
	if (len >= env->vm->config.gc_lop_threshold)
		return rt_gc_alloc_array_tenure(env, size);

	/* Allocate in the nursery region. */
	for (retry = 0; retry <= 1; retry++) {
		/* Allocate a rt_array buffer. */
		arr = nursery_alloc(env, sizeof(struct rt_array) + size * sizeof(struct rt_value));
		if (arr == NULL) {
			/* Retry. */
			if (retry == 0) {
				rt_gc_young_gc(env);
				continue;
			} else {
				rt_out_of_memory(env);
				return NULL;
			}
		}

		/* Get the address of the table. */
		table = (struct rt_value *)((char *)arr + sizeof(struct rt_array));

		/* Setup the struct. */
		memset(&arr->head, 0, sizeof(struct rt_gc_object));
		arr->head.type = RT_GC_TYPE_ARRAY;
		arr->head.region = RT_GC_REGION_NURSERY;
		arr->head.size = sizeof(struct rt_array) + size * sizeof(struct rt_value);
		INSERT_TO_LIST(&arr->head, env->vm->gc.nursery_list, prev, next);
		arr->alloc_size = size;
		arr->size = 0;
		arr->table = table;
		arr->newer = NULL;
#if defined(NOCT_USE_MULTITHREAD)
		arr->shared = 0;
		arr->write_lock = 0;
		arr->creator = env;
#endif

		/* Succeeded. */
		return arr;
	}

	/* Failed. */
	rt_out_of_memory(env);
	return NULL;
}

/* Allocates an array object in the graduate region. */
static struct rt_array *
rt_gc_alloc_array_graduate(
	struct rt_env *env,
	size_t size)
{
	struct rt_array *arr;
	struct rt_value *table;

	assert(env != NULL);
	assert(size > 0);

	/* Check for overflow. */
	if (size >= SIZE_MAX / sizeof(struct rt_value)) {
		rt_out_of_memory(env);
		return NULL;
	}
	if (size > (SIZE_MAX - sizeof(struct rt_array)) / sizeof(struct rt_value)) {
		rt_out_of_memory(env);
		return NULL;
	}

	/*
	 * This function is only called from the young GC,
	 * and thus, we don't use young GC for a retry here.
	 */

	/* Try allocating in the graduate region. */
	do {
		/* Allocate a rt_arrary buffer. */
		arr = graduate_alloc(env, sizeof(struct rt_array) + size * sizeof(struct rt_value));
		if (arr == NULL)
			break;

		/* Get the address of the table. */
		table = (struct rt_value *)((char *)arr + sizeof(struct rt_array));

		/* Setup the struct. */
		memset(&arr->head, 0, sizeof(struct rt_gc_object));
		arr->head.type = RT_GC_TYPE_ARRAY;
		arr->head.region = RT_GC_REGION_GRADUATE;
		arr->head.size = sizeof(struct rt_array) + size * sizeof(struct rt_value);
		INSERT_TO_LIST(&arr->head, env->vm->gc.graduate_new_list, prev, next);
		arr->alloc_size = size;
		arr->size = 0;
		arr->table = table;
		arr->newer = NULL;
#if defined(NOCT_USE_MULTITHREAD)
		arr->shared = 0;
		arr->write_lock = 0;
		arr->creator = env;
#endif

		/* Succeeded. (graduate) */
		return arr;
	} while (0);

	/*
	 * Failed to allocate in the graduate region.
	 * Try allocating in the tenure region.
	 */
	arr = rt_gc_alloc_array_tenure(env, size);
	if (arr == NULL)
		return NULL;

	/* Succeeded. (tenure) */
	return arr;
}

/* Allocates a large array in the tenure region. */
static struct rt_array *
rt_gc_alloc_array_tenure(
	struct rt_env *env,
	size_t size)
{
	struct rt_array *arr;
	struct rt_value *table;
	int retry;

	assert(env != NULL);
	assert(size > 0);

	/* Check for overflow. */
	if (size >= SIZE_MAX / sizeof(struct rt_value)) {
		rt_out_of_memory(env);
		return NULL;
	}
	if (size > (SIZE_MAX - sizeof(struct rt_array)) / sizeof(struct rt_value)) {
		rt_out_of_memory(env);
		return NULL;
	}

	/* Allocate in the tenure region. */
	for (retry = 0; retry <= 2; retry++) {
		/* Allocate a rt_array buffer. */
		arr = rt_gc_tenure_alloc(env, sizeof(struct rt_array) + size * sizeof(struct rt_value));
		if (arr == NULL) {
			/* Retry. */
			if (retry == 0) {
				rt_gc_old_gc(env);
				continue;
			} else if (retry == 1) {
				rt_gc_compact_gc(env);
				continue;
			} else {
				rt_out_of_memory(env);
				return NULL;
			}
		}

		/* Get the address of the table. */
		table = (struct rt_value *)((char *)arr + sizeof(struct rt_array));

		/* Setup the struct. */
		memset(&arr->head, 0, sizeof(struct rt_gc_object));
		arr->head.type = RT_GC_TYPE_ARRAY;
		arr->head.region = RT_GC_REGION_TENURE;
		arr->head.size = sizeof(struct rt_array) + size * sizeof(struct rt_value);
		INSERT_TO_LIST(&arr->head, env->vm->gc.tenure_list, prev, next);
		arr->alloc_size = size;
		arr->size = 0;
		arr->table = table;
		arr->newer = NULL;
#if defined(NOCT_USE_MULTITHREAD)
		arr->shared = 0;
		arr->write_lock = 0;
		arr->creator = env;
#endif

		/* Succeeded. */
		return arr;
	}

	/* Failed. */
	rt_out_of_memory(env);
	return NULL;
}

/*
 * Allocates a dictionary object in the appropriate region.
 */
struct rt_dict *
rt_gc_alloc_dict(
	struct rt_env *env,
	size_t size)
{
	struct rt_dict *dict;
	struct rt_value *key_table;
	struct rt_value *value_table;
	int retry;

	assert(env != NULL);
	assert(size > 0);

	/* Check for overflow. */
	if (size >= SIZE_MAX / 2 / sizeof(struct rt_value)) {
		rt_out_of_memory(env);
		return NULL;
	}
	if (size > (SIZE_MAX - sizeof(struct rt_dict)) / 2 / sizeof(struct rt_value)) {
		rt_out_of_memory(env);
		return NULL;
	}

	/*
	 * [Large Object Promotion]
	 *  - If the array is large, allocate in the tenure region.
	 */
	if (2 * size * sizeof(struct rt_value) >= env->vm->config.gc_lop_threshold)
		return rt_gc_alloc_dict_tenure(env, size);

	/* Allocate in the nursery region. */
	for (retry = 0; retry <= 1; retry++) {
		/* Allocate a rt_dict buffer. */
		dict = nursery_alloc(env, sizeof(struct rt_dict) + 2 * size * sizeof(struct rt_value));
		if (dict == NULL) {
			/* Retry. */
			if (retry == 0) {
				rt_gc_young_gc(env);
				continue;
			} else {
				rt_out_of_memory(env);
				return NULL;
			}
		}

		/* Get the address of the key array block. */
		key_table = (struct rt_value *)((char *)dict + sizeof(struct rt_dict));

		/* Get the address of the value array block. */
		value_table = (struct rt_value *)((char *)dict + sizeof(struct rt_dict) + size * sizeof(struct rt_value));

		/* Setup the struct. */
		memset(&dict->head, 0, sizeof(struct rt_gc_object));
		dict->head.type = RT_GC_TYPE_DICT;
		dict->head.region = RT_GC_REGION_NURSERY;
		dict->head.size = sizeof(struct rt_dict) + 2 * size * sizeof(struct rt_value);
		INSERT_TO_LIST(&dict->head, env->vm->gc.nursery_list, prev, next);
		dict->alloc_size = size;
		dict->size = 0;
		dict->key = key_table;
		dict->value = value_table;
		dict->newer = NULL;
		dict->native_pointer = NULL;
		dict->native_finalizer = NULL;
#if defined(NOCT_USE_MULTITHREAD)
		dict->shared = 0;
		dict->write_lock = 0;
		dict->creator = env;
#endif

		/* Succeeded. */
		return dict;
	}

	/* Failed. */
	rt_out_of_memory(env);
	return NULL;
}

/* Allocates an array object in the graduate region. */
static struct rt_dict *
rt_gc_alloc_dict_graduate(
	struct rt_env *env,
	size_t size)
{
	struct rt_dict *dict;
	struct rt_value *key_table;
	struct rt_value *value_table;

	assert(env != NULL);
	assert(size > 0);

	/* Check for overflow. */
	if (size >= SIZE_MAX / 2 / sizeof(struct rt_value)) {
		rt_out_of_memory(env);
		return NULL;
	}
	if (size > (SIZE_MAX - sizeof(struct rt_dict)) / 2 / sizeof(struct rt_value)) {
		rt_out_of_memory(env);
		return NULL;
	}

	/*
	 * This function is only called from the young GC,
	 * and thus, we don't use young GC for a retry here.
	 */

	/* Try allocating in the graduate region. */
	do {
		/* Allocate a rt_dict buffer. */
		dict = graduate_alloc(env, sizeof(struct rt_dict) + 2 * size * sizeof(struct rt_value));
		if (dict == NULL)
			break;

		/* Get the address of the key array block. */
		key_table = (struct rt_value *)((char *)dict + sizeof(struct rt_dict));

		/* Get the address of the value array block. */
		value_table = (struct rt_value *)((char *)dict + sizeof(struct rt_dict) + size * sizeof(struct rt_value));

		/* Setup a struct. */
		memset(&dict->head, 0, sizeof(struct rt_gc_object));
		dict->head.type = RT_GC_TYPE_DICT;
		dict->head.region = RT_GC_REGION_GRADUATE;
		dict->head.size = sizeof(struct rt_dict) + 2 * size * sizeof(struct rt_value);
		INSERT_TO_LIST(&dict->head, env->vm->gc.graduate_new_list, prev, next);
		dict->alloc_size = size;
		dict->size = 0;
		dict->key = key_table;
		dict->value = value_table;
		dict->newer = NULL;
		dict->native_pointer = NULL;
		dict->native_finalizer = NULL;
#if defined(NOCT_USE_MULTITHREAD)
		dict->shared = 0;
		dict->write_lock = 0;
		dict->creator = env;
#endif

		/* Succeeded. (graduate) */
		return dict;
	} while (0);

	/*
	 * Failed to allocate in the graduate region.
	 * Try allocating in the tenure region.
	 */
	dict = rt_gc_alloc_dict_tenure(env, size);
	if (dict == NULL)
		return NULL;

	/* Succeeded. (tenure) */
	return dict;
}

/* Allocates a dictionary object in the tenure region. */
static struct rt_dict *
rt_gc_alloc_dict_tenure(
	struct rt_env *env,
	size_t size)
{
	struct rt_dict *dict;
	struct rt_value *key_table;
	struct rt_value *value_table;
	int retry;

	assert(env != NULL);
	assert(size > 0);

	/* Check for overflow. */
	if (size >= SIZE_MAX / 2 / sizeof(struct rt_value)) {
		rt_out_of_memory(env);
		return NULL;
	}
	if (size > (SIZE_MAX - sizeof(struct rt_dict)) / 2 / sizeof(struct rt_value)) {
		rt_out_of_memory(env);
		return NULL;
	}

	/* Allocate in the tenure region. */
	for (retry = 0; retry <= 2; retry++) {
		/* Allocate the rt_dict buffer. */
		dict = rt_gc_tenure_alloc(env, sizeof(struct rt_dict) + 2 * size * sizeof(struct rt_value));
		if (dict == NULL) {
			/* Retry. */
			if (retry == 0) {
				rt_gc_old_gc(env);
				continue;
			} else if (retry == 1) {
				rt_gc_compact_gc(env);
				continue;
			} else {
				rt_out_of_memory(env);
				return NULL;
			}
		}

		/* Get the address of the key array block. */
		key_table = (struct rt_value *)((char *)dict + sizeof(struct rt_dict));

		/* Get the address of the value array block. */
		value_table = (struct rt_value *)((char *)dict + sizeof(struct rt_dict) + size * sizeof(struct rt_value));

		/* Setup a value. */
		memset(&dict->head, 0, sizeof(struct rt_gc_object));
		dict->head.type = RT_GC_TYPE_DICT;
		dict->head.region = RT_GC_REGION_TENURE;
		dict->head.size = sizeof(struct rt_dict) + 2 * size * sizeof(struct rt_value);
		INSERT_TO_LIST(&dict->head, env->vm->gc.tenure_list, prev, next);
		dict->alloc_size = size;
		dict->size = 0;
		dict->key = key_table;
		dict->value = value_table;
		dict->newer = NULL;
		dict->native_pointer = NULL;
		dict->native_finalizer = NULL;
#if defined(NOCT_USE_MULTITHREAD)
		dict->shared = 0;
		dict->write_lock = 0;
		dict->creator = env;
#endif

		/* Succeeded. */
		return dict;
	}

	/* Failed. */
	rt_out_of_memory(env);
	return NULL;
}

/*
 * Allocates a packed object in the appropriate region.
 */
struct rt_packed *
rt_gc_alloc_packed(
	struct rt_env *env,
	int type,
	size_t size,
	size_t elem_size,
	void *preallocated)
{
	struct rt_packed *packed;
	void *p;
	int retry;

	assert(env != NULL);
	assert(size > 0);
	assert(elem_size > 0);

	/* If use a preallocated buffer. */
	if (preallocated != NULL)
		size = 0;

	/*
	 * [Large Object Promotion]
	 *  - If the packed is large, allocate in the tenure region.
	 */
	if (size >= env->vm->config.gc_lop_threshold)
		return rt_gc_alloc_packed_tenure(env, type, size, elem_size, preallocated);

	/* Allocate in the nursery region. */
	for (retry = 0; retry <= 1; retry++) {
		/* Allocate a rt_packed buffer. */
		packed = nursery_alloc(env, sizeof(struct rt_packed) + size);
		if (packed == NULL) {
			/* Retry. */
			if (retry == 0) {
				rt_gc_young_gc(env);
				continue;
			} else {
				rt_out_of_memory(env);
				return NULL;
			}
		}

		/* Get the address of the table. */
		if (preallocated == NULL)
			p = (char *)packed + sizeof(struct rt_packed);
		else
			p = preallocated;			

		/* Setup the struct. */
		memset(&packed->head, 0, sizeof(struct rt_gc_object));
		packed->head.type = RT_GC_TYPE_PACKED;
		packed->head.region = RT_GC_REGION_NURSERY;
		packed->head.size = sizeof(struct rt_packed) + size;
		INSERT_TO_LIST(&packed->head, env->vm->gc.nursery_list, prev, next);
		packed->type = type;
		packed->size = size;
		packed->elem_size = elem_size;
		packed->packed_buffer = p;

		/* Succeeded. */
		return packed;
	}

	/* Failed. */
	rt_out_of_memory(env);
	return NULL;
}

/* Allocates ap packed object in the graduate region. */
static struct rt_packed *
rt_gc_alloc_packed_graduate(
	struct rt_env *env,
	int type,
	size_t size,
	size_t elem_size,
	void *preallocated)
{
	struct rt_packed *packed;
	void *p;

	assert(env != NULL);
	assert(size > 0);
	assert(elem_size > 0);

	/* If use a preallocated buffer. */
	if (preallocated != NULL)
		size = 0;

	/*
	 * This function is only called from the young GC,
	 * and thus, we don't use young GC for a retry here.
	 */

	/* Try allocating in the graduate region. */
	do {
		/* Allocate a rt_packed buffer. */
		packed = graduate_alloc(env, sizeof(struct rt_packed) + size);
		if (packed == NULL)
			break;

		/* Get the address of the table. */
		if (preallocated == NULL)
			p = (char *)packed + sizeof(struct rt_packed);
		else
			p = preallocated;			

		/* Setup the struct. */
		memset(&packed->head, 0, sizeof(struct rt_gc_object));
		packed->head.type = RT_GC_TYPE_PACKED;
		packed->head.region = RT_GC_REGION_GRADUATE;
		packed->head.size = sizeof(struct rt_packed) + size;
		INSERT_TO_LIST(&packed->head, env->vm->gc.graduate_new_list, prev, next);
		packed->type = type;
		packed->size = size;
		packed->elem_size = elem_size;
		packed->packed_buffer = p;

		/* Succeeded. (graduate) */
		return packed;
	} while (0);

	/*
	 * Failed to allocate in the graduate region.
	 * Try allocating in the tenure region.
	 */
	packed = rt_gc_alloc_packed_tenure(env, type, size, elem_size, preallocated);
	if (packed == NULL)
		return NULL;

	/* Succeeded. (tenure) */
	return packed;
}

/* Allocates a large packed in the tenure region. */
static struct rt_packed *
rt_gc_alloc_packed_tenure(
	struct rt_env *env,
	int type,
	size_t size,
	size_t elem_size,
	void *preallocated)
{
	struct rt_packed *packed;
	void *p;
	int retry;

	assert(env != NULL);
	assert(size > 0);

	/* If use a preallocated buffer. */
	if (preallocated != NULL)
		size = 0;

	/* Allocate in the tenure region. */
	for (retry = 0; retry <= 2; retry++) {
		/* Allocate a rt_packed buffer. */
		packed = rt_gc_tenure_alloc(env, sizeof(struct rt_packed) + size);
		if (packed == NULL) {
			/* Retry. */
			if (retry == 0) {
				rt_gc_old_gc(env);
				continue;
			} else if (retry == 1) {
				rt_gc_compact_gc(env);
				continue;
			} else {
				rt_out_of_memory(env);
				return NULL;
			}
		}

		/* Get the address of the table. */
		if (preallocated == NULL)
			p = (char *)packed + sizeof(struct rt_packed);
		else
			p = preallocated;			

		/* Setup the struct. */
		memset(&packed->head, 0, sizeof(struct rt_gc_object));
		packed->head.type = RT_GC_TYPE_PACKED;
		packed->head.region = RT_GC_REGION_TENURE;
		packed->head.size = sizeof(struct rt_packed) + size;
		INSERT_TO_LIST(&packed->head, env->vm->gc.tenure_list, prev, next);
		packed->type = type;
		packed->size = size;
		packed->elem_size = elem_size;
		packed->packed_buffer = p;

		/* Succeeded. */
		return packed;
	}

	/* Failed. */
	rt_out_of_memory(env);
	return NULL;
}

/*
 * Write barrier: registers a container in the remember set if it
 * references a young object.
 */
void
rt_gc_array_write_barrier(
	struct rt_env *env,
	struct rt_array *arr,
	size_t index,
	struct rt_value *val)
{
	UNUSED_PARAMETER(index);

	/*
	 * If all of the following are satisfied, add the array to the
	 * remember set.
	 *  - the array is a tenure object
	 *  - the array is not in the remember set
	 *  - the element is a reference
	 *  - the element is a nursery or graduate object
	 */
	if (arr->head.region == RT_GC_REGION_TENURE &&
	    !arr->head.rem_flg &&
	    IS_REF_VAL(val) &&
	    IS_YOUNG_OBJ(val->val.obj)) {
		arr->head.rem_flg = true;
		INSERT_TO_LIST(&arr->head, env->vm->gc.remember_set, rem_prev, rem_next);
	}
}

/*
 * Write barrier: registers a container in the remember set if it
 * references a younger object.
 */
void
rt_gc_dict_write_barrier(
	struct rt_env *env,
	struct rt_dict *dict,
	struct rt_value *val)
{
	/*
	 * If all of the following are satisfied, add the array to the
	 * remember set.
	 *  - the array is a tenure objectf
	 *  - the array is not in the remember set
	 *  - the element is a reference
	 *  - the element is a nursery or graduate object
	 */
	if (dict->head.region == RT_GC_REGION_TENURE &&
	    !dict->head.rem_flg &&
	    IS_REF_VAL(val) &&
	    IS_YOUNG_OBJ(val->val.obj)) {
		dict->head.rem_flg = true;
		INSERT_TO_LIST(&dict->head, env->vm->gc.remember_set, rem_prev, rem_next);
	}
}

/* Executes a young GC in the multithreaded manner. */
static void
rt_gc_young_gc(
	struct rt_env *env)
{
	om_enter_gc(env, RT_GC_LEVEL_0);
	rt_gc_young_gc_body(env);
	om_leave_gc(env);
}

/* Executes a young GC. */
static void
rt_gc_young_gc_body(
	struct rt_env *env)
{
	struct rt_gc_object *obj;
	struct rt_frame *frame;
	uint32_t i;
	int sp;
	struct finalize_table {
		void *native_pointer;
		void (*native_finalizer)(void *native_pointer);
	} *finalize_table;
	uint32_t finalize_size;
	uint32_t finalize_count;

	env->vm->gc.graduate_new_list = NULL;
	finalize_table = NULL;
	finalize_size = 0;
	finalize_count = 0;

	/*
	 * Clear marks.
	 */

	/* Clear marks of the nursery objects. */
	obj = env->vm->gc.nursery_list;
	while (obj != NULL) {
		obj->is_marked = false;
		obj->forward = NULL;
		if (obj->type == RT_GC_TYPE_DICT) {
			if (((struct rt_dict *)obj)->native_finalizer != NULL)
				finalize_size++;
		}
		obj = obj->next;
	}

	/* Clear marks of the graduate (from) objects. */
	obj = env->vm->gc.graduate_list;
	while (obj != NULL) {
		obj->is_marked = false;
		obj->forward = NULL;
		obj = obj->next;
	}

	/* Clear marks of the tenure objects. */
	obj = env->vm->gc.tenure_list;
	while (obj != NULL) {
		obj->is_marked = false;
		obj->forward = NULL;
		obj = obj->next;
	}

	/*
	 * Mark and Copy objects.
	 *  - Recursively visit root objects.
	 *  - Copy nursery objects to the graduate region.
	 *  - Promote graduate objects that satisfy the criteria to the tenure region.
	 */

	/* For all global variables. */
	for (i = 0; i < (uint32_t)env->vm->global_alloc_size; i++) {
		if (env->vm->global[i].name == NULL || env->vm->global[i].is_removed)
			continue;
		if (IS_REF_VAL(&env->vm->global[i].val)) {
			if (!rt_gc_copy_young_object_recursively(env, &env->vm->global[i].val.val.obj))
				return;
		}
	}

	/* For all call frames. */
	for (sp = (int)env->cur_frame_index; sp >= 0; sp--) {
		frame = &env->frame_alloc[sp];

		/* For all temporary variables in the frame. */
		for (i = 0; i < frame->tmpvar_size; i++) {
			if (IS_REF_VAL(&frame->tmpvar[i])) {
				if (!rt_gc_copy_young_object_recursively(env, &frame->tmpvar[i].val.obj))
					return;
			}
		}

		/* For all pinned C local variables in the frame. */
		for (i = 0; i < frame->pinned_count; i++) {
			if (IS_REF_VAL(frame->pinned[i])) {
				if (!rt_gc_copy_young_object_recursively(env, &frame->pinned[i]->val.obj))
					return;
			}
		}
	}

	/* For all pinned C global variables. */
	for (i = 0; i < env->vm->pinned_count; i++) {
		if (IS_REF_VAL(env->vm->pinned[i])) {
			if (!rt_gc_copy_young_object_recursively(env, &env->vm->pinned[i]->val.obj))
				return;
		}
	}

	/* For all remember set objects. */
	obj = env->vm->gc.remember_set;
	while (obj != NULL) {
		if (!rt_gc_copy_young_object_recursively(env, &obj))
			return;
		obj = obj->rem_next;
	}

	/*
	 * Update references from the remember set.
	 */

	/* For each remember set object, update the addresses of the inner elements using the forwarding pointer technique. */
	obj = env->vm->gc.remember_set;
	while (obj != NULL) {
		if (obj->type == RT_GC_TYPE_ARRAY) {
			struct rt_array *arr = (struct rt_array *)obj;
			for (i = 0; i < arr->size; i++) {
				if (IS_REF_VAL(&arr->table[i]) &&
				    IS_YOUNG_OBJ(arr->table[i].val.obj) &&
				    arr->table[i].val.obj->forward != NULL) {
					arr->table[i].val.obj = arr->table[i].val.obj->forward;
				}
			}
		} else {
			struct rt_dict *dict = (struct rt_dict *)obj;
			for (i = 0; i < dict->alloc_size; i++) {
				if (dict->key[i].type != NOCT_VALUE_STRING)
					continue; /* Removed or empty. */
				if (IS_YOUNG_OBJ(dict->key[i].val.obj) &&
				    dict->key[i].val.obj->forward != NULL) {
					dict->key[i].val.obj = dict->key[i].val.obj->forward;
				}
				if (IS_REF_VAL(&dict->value[i]) &&
				    IS_YOUNG_OBJ(dict->value[i].val.obj) &&
				    dict->value[i].val.obj->forward != NULL) {
					dict->value[i].val.obj = dict->value[i].val.obj->forward;
				}
			}
		}
		obj = obj->rem_next;
	}

	/* For each remember set object, remove from the remember set if the object doesn't have a cross generation reference. */
	obj = env->vm->gc.remember_set;
	while (obj != NULL) {
		bool has_cross_gen_ref = false;
		if (obj->type == RT_GC_TYPE_ARRAY) {
			struct rt_array *arr = (struct rt_array *)obj;
			for (i = 0; i < arr->size; i++) {
				if (IS_REF_VAL(&arr->table[i]) &&
				    IS_YOUNG_OBJ(arr->table[i].val.obj)) {
					has_cross_gen_ref = true;
					break;
				}
			}
		} else {
			struct rt_dict *dict = (struct rt_dict *)obj;
			for (i = 0; i < dict->alloc_size; i++) {
				if (dict->key[i].type != NOCT_VALUE_STRING)
					continue; /* Removed or empty. */
				if (IS_YOUNG_OBJ(dict->key[i].val.obj)) {
					has_cross_gen_ref = true;
					break;
				}
				if (IS_REF_VAL(&dict->value[i]) &&
				    IS_YOUNG_OBJ(dict->value[i].val.obj)) {
					has_cross_gen_ref = true;
					break;
				}
			}
		}
		if (!has_cross_gen_ref) {
			/* Unlink from the remember set list. */
			obj->rem_flg = false;
			UNLINK_FROM_LIST(obj, env->vm->gc.remember_set, rem_prev, rem_next);
		}
		obj = obj->rem_next;
	}

	/*
	 * Make finalize table.
	 */

	if (finalize_size > 0) {
		finalize_table = malloc(sizeof(struct finalize_table) * finalize_size);
		if (finalize_table == NULL)
			return;

		/* For all nursery objects. */
		obj = env->vm->gc.nursery_list;
		while (obj != NULL) {
			if (!obj->is_marked &&
			    obj->type == RT_GC_TYPE_DICT &&
			    ((struct rt_dict *)obj)->native_finalizer != NULL) {
				finalize_table[finalize_count].native_pointer = ((struct rt_dict *)obj)->native_pointer;
				finalize_table[finalize_count].native_finalizer = ((struct rt_dict *)obj)->native_finalizer;
				finalize_count++;
			}
			obj = obj->next;
		}
	}

	/*
	 * Finish.
	 */

	/* Clear the nursery. */
	arena_unwind(&env->vm->gc.nursery_arena);
		   
	/* Clear the graduate (from) */
	arena_unwind(&env->vm->gc.graduate_arena[env->vm->gc.cur_grad_from]);

	/* Clear the nursery list. */
	env->vm->gc.nursery_list = NULL;

	/* Swap "to" and "from". */
	if (env->vm->gc.cur_grad_from == 0) {
		env->vm->gc.cur_grad_from = 1;
		env->vm->gc.cur_grad_to = 0;
	} else {
		env->vm->gc.cur_grad_from = 0;
		env->vm->gc.cur_grad_to = 1;
	}
	env->vm->gc.graduate_list = env->vm->gc.graduate_new_list;
	env->vm->gc.graduate_new_list = NULL;

	/*
	 * Call native finalizers.
	 */

	for (i = 0; i < finalize_count; i++)
		finalize_table[i].native_finalizer(finalize_table[i].native_pointer);
}

/* Marks-and-copies objects recursively. */
static bool
rt_gc_copy_young_object_recursively(
	struct rt_env *env,
	struct rt_gc_object **obj)
{
	struct rt_gc_object *new_obj;
	struct rt_array *arr;
	struct rt_dict *dict;
	bool is_promoted;
	uint32_t i;

	/* If this is an array or dictionary, get the forwarder. */
	rt_gc_array_dict_follow_newer(env, obj);

	/* If already processed. */
	if ((*obj)->is_marked) {
		/* And if this is a young object. */
		if (IS_YOUNG_OBJ(*obj) && (*obj)->forward != NULL) {
			/* Rewrite the reference. */
			*obj = (*obj)->forward;
		}

		/* Skip. */
		return true;
	}

	/* Copy or promote. */
	is_promoted = false;
	new_obj = NULL;
	if ((*obj)->region != RT_GC_REGION_TENURE) {
		/* Check for the promotion. */
		if ((*obj)->promotion_count < env->vm->config.gc_promotion_threshold) {
			/* No promotion, just copy the object. */
			switch ((*obj)->type) {
			case RT_GC_TYPE_STRING:
				new_obj = rt_gc_copy_string_to_graduate(env, (struct rt_string *)*obj);
				break;
			case RT_GC_TYPE_ARRAY:
				new_obj = rt_gc_copy_array_to_graduate(env, (struct rt_array *)*obj);
				break;
			case RT_GC_TYPE_DICT:
				new_obj = rt_gc_copy_dict_to_graduate(env, (struct rt_dict *)*obj);
				break;
			case RT_GC_TYPE_PACKED:
				new_obj = rt_gc_copy_packed_to_graduate(env, (struct rt_packed *)*obj);
				break;
			default:
				assert(NEVER_COME_HERE);
				break;
			}

			/* Set the forwarding pointer. */
			(*obj)->forward = new_obj;

			/* Increment the promotion count. */
			new_obj->promotion_count = (*obj)->promotion_count + 1;
		} else {
			/* Promote the object. */
			new_obj = rt_gc_promote_object(env, *obj);
			if (new_obj == NULL)
				return false;

			/* Mark as promoted. */
			is_promoted = true;

			/* The forwarding pointer is set in rt_gc_promote_object(). */
		}

		/* Mark the old object processed. */
		(*obj)->is_marked = true;

		/* Rewrite the reference. */
		*obj = new_obj;
	}

	/* Recursively copy. */
	switch ((*obj)->type) {
	case RT_GC_TYPE_STRING:
	case RT_GC_TYPE_PACKED:
		/* No inner values. */
		break;
	case RT_GC_TYPE_ARRAY:
		arr = (struct rt_array *)*obj;
		for (i = 0; i < arr->size; i++) {
			if (IS_REF_VAL(&arr->table[i])) {
				if (!rt_gc_copy_young_object_recursively(env, &arr->table[i].val.obj))
					return false;
			}
		}
		break;
	case RT_GC_TYPE_DICT:
		dict = (struct rt_dict *)*obj;
		for (i = 0; i < dict->alloc_size; i++) {
			if (dict->key[i].type != NOCT_VALUE_STRING)
				continue; /* Removed or empty. */
			if (!rt_gc_copy_young_object_recursively(env, &dict->key[i].val.obj))
				return false;
			if (IS_REF_VAL(&dict->value[i])) {
				if (!rt_gc_copy_young_object_recursively(env, &dict->value[i].val.obj))
					return false;
			}
		}
		break;
	default:
		assert(NEVER_COME_HERE);
		break;
	}

	/* When promoted, check for cross-generation references. */
	if (is_promoted) {
		if ((*obj)->type == RT_GC_TYPE_ARRAY) {
			/* Check for array cross-generation references. */
			arr = (struct rt_array *)*obj;
			if (arr->head.rem_flg)
				return true;
			for (i = 0; i < arr->size; i++) {
				/* If the element is young generation. */
				if (IS_REF_VAL(&arr->table[i]) &&
				    IS_YOUNG_OBJ(arr->table[i].val.obj)) {
					/* And if the element is not promoted to the tenure region. */
					if (arr->table[i].val.obj->forward != NULL &&
					    arr->table[i].val.obj->forward->region == RT_GC_REGION_TENURE)
						continue;

					/* Add to remember set. */
					arr->head.rem_flg = true;
					INSERT_TO_LIST(&arr->head, env->vm->gc.remember_set,rem_prev, rem_next);
					break;
				}
			}
		} else if ((*obj)->type == RT_GC_TYPE_DICT) {
			/* Check for dictionary cross-generation references. */
			dict = (struct rt_dict *)*obj;
			if (dict->head.rem_flg)
				return true;
			for (i = 0; i < dict->alloc_size; i++) {
				if (dict->key[i].type != NOCT_VALUE_STRING)
					continue; /* Removed or empty. */

				/* If the key is young generation. */
				if (IS_YOUNG_OBJ(dict->key[i].val.obj)) {
					/* And if the element is not promoted to the tenure region. */
					if (dict->key[i].val.obj->forward != NULL &&
					    dict->key[i].val.obj->forward->region == RT_GC_REGION_TENURE)
						continue;

					/* Add to remember set. */
					dict->head.rem_flg = true;
					INSERT_TO_LIST(&dict->head, env->vm->gc.remember_set, rem_prev, rem_next);
					break;
				}

				/* If the value is young generation. */
				if (IS_REF_VAL(&dict->value[i]) &&
				    IS_YOUNG_OBJ(dict->value[i].val.obj)) {
					/* And if the element is not promoted to the tenure region. */
					if (dict->value[i].val.obj->forward != NULL &&
					    dict->value[i].val.obj->forward->region == RT_GC_REGION_TENURE)
						continue;

					/* Add to remember set. */
					dict->head.rem_flg = true;
					INSERT_TO_LIST(&dict->head, env->vm->gc.remember_set,rem_prev, rem_next);
					break;
				}
			}
		}
	}

	return true;
}

/* If this is an array or dictionary, get the forwarder. */
static void
rt_gc_array_dict_follow_newer(
	struct rt_env *env,
	struct rt_gc_object **obj)
{
	UNUSED_PARAMETER(env);

	if ((*obj)->type == RT_GC_TYPE_ARRAY) {
		struct rt_array *arr = (struct rt_array *)*obj;
		if (arr->newer == NULL)
			return;		
		while (arr->newer != NULL)
			arr = arr->newer;
		*obj = &arr->head;
	} else if ((*obj)->type == RT_GC_TYPE_DICT) {
		struct rt_dict *dict = (struct rt_dict *)*obj;
		if (dict->newer == NULL)
			return;		
		while (dict->newer != NULL)
			dict = dict->newer;
		*obj = &dict->head;
	}
}

/* Promotes an object. */
static struct rt_gc_object *
rt_gc_promote_object(
	struct rt_env *env,
	struct rt_gc_object *obj)
{
	struct rt_gc_object *ret;
	switch (obj->type) {
	case RT_GC_TYPE_STRING:
		ret = rt_gc_promote_string(env, obj);
		if (ret == NULL)
			return NULL;
		break;
	case RT_GC_TYPE_ARRAY:
		ret = rt_gc_promote_array(env, obj);
		if (ret == NULL)
			return NULL;
		break;
	case RT_GC_TYPE_DICT:
		ret = rt_gc_promote_dict(env, obj);
		if (ret == NULL)
			return NULL;
		break;
	case RT_GC_TYPE_PACKED:
		ret = rt_gc_promote_packed(env, obj);
		if (ret == NULL)
			return NULL;
		break;
	default:
		assert(NEVER_COME_HERE);
		ret = NULL;
		break;
	}

	return ret;
}

/* Promotes a string object. */
static struct rt_gc_object *
rt_gc_promote_string(
	struct rt_env *env,
	struct rt_gc_object *obj)
{
	struct rt_string *old_rts, *new_rts;

	/* Allocate a string object. */
	old_rts = (struct rt_string *)obj;
	new_rts = rt_gc_alloc_string_tenure(env, old_rts->data, old_rts->len, old_rts->hash);
	if (new_rts == NULL)
		return false;

	/* Set the forwarding pointer. */
	obj->forward = &new_rts->head;

	return &new_rts->head;
}

/* Promotes an array object. */
static struct rt_gc_object *
rt_gc_promote_array(
	struct rt_env *env,
	struct rt_gc_object *obj)
{
	struct rt_array *old_arr, *new_arr;
	size_t alloc_size;

	/* Get the allocation size. */
	old_arr = (struct rt_array *)obj;
	alloc_size = old_arr->size;
	if (alloc_size == 0)
		alloc_size = old_arr->alloc_size;

	/* Allocate an array object. */
	new_arr = rt_gc_alloc_array_tenure(env, alloc_size);
	if (new_arr == NULL)
		return false;

	/* Copy the table. */
	new_arr->size = old_arr->size;
	if (new_arr->size > 0)
		memcpy(new_arr->table, old_arr->table, new_arr->size * sizeof(struct rt_value));

	/* Set the forwarding pointer. */
	obj->forward = &new_arr->head;

	return &new_arr->head;
}

/* Promotes a dictionary object. */
static struct rt_gc_object *
rt_gc_promote_dict(
	struct rt_env *env,
	struct rt_gc_object *obj)
{
	struct rt_dict *old_dict, *new_dict;
	size_t alloc_size;
	uint32_t index, i, j;

	/* Get the allocation size. */
	old_dict = (struct rt_dict *)obj;
	alloc_size = old_dict->alloc_size;

	/* Allocate a dictionary object. */
	new_dict = rt_gc_alloc_dict_tenure(env, alloc_size);
	if (new_dict == NULL)
		return false;

	/* Rehash. (Copy the keys and values.) */
	for (i = 0; i < old_dict->alloc_size; i++) {
		if (old_dict->key[i].type != NOCT_VALUE_STRING)
			continue; /* Removed or empty. */

		index = rt_string_hash(old_dict->key[i].val.str->data) & ((uint32_t)new_dict->alloc_size - 1);
		for (j = index;
		     j != ((index - 1 + (uint32_t)new_dict->alloc_size) & (new_dict->alloc_size - 1));
		     j = (j + 1) & ((uint32_t)new_dict->alloc_size - 1)) {
			if (new_dict->key[j].type != NOCT_VALUE_STRING) {
				/* Copy the item. */
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
	obj->forward = &new_dict->head;

	new_dict->native_pointer = old_dict->native_pointer;
	new_dict->native_finalizer = old_dict->native_finalizer;

	return &new_dict->head;
}

/* Promotes a packed object. */
static struct rt_gc_object *
rt_gc_promote_packed(
	struct rt_env *env,
	struct rt_gc_object *obj)
{
	struct rt_packed *old_packed, *new_packed;

	/* Allocate a string object. */
	old_packed = (struct rt_packed *)obj;
	new_packed = rt_gc_alloc_packed_tenure(env,
					       old_packed->type,
					       old_packed->size,
					       old_packed->elem_size,
					       (old_packed->size == 0) ? old_packed->packed_buffer : NULL);
	if (new_packed == NULL)
		return false;

	/* Set the forwarding pointer. */
	obj->forward = &new_packed->head;

	return &new_packed->head;
}

/* Copies a string object to the graduate region. */
static struct rt_gc_object *
rt_gc_copy_string_to_graduate(
	struct rt_env *env,
	struct rt_string *old_obj)
{
	struct rt_string *new_obj;

	/*
	 * Strings larger than noct_conf_gc_lop_threshold must not be in the
	 * nursery or graduate regions.
	 */
	assert(old_obj->len < env->vm->config.gc_lop_threshold);

	/* Allocate in the graduate region. */
	new_obj = rt_gc_alloc_string_graduate(env, old_obj->data, old_obj->len, old_obj->hash);
	if (new_obj == NULL)
		return NULL;

	/* Succeeded. */
	return &new_obj->head;
}

/* Copies an array object to the graduate region. */
static struct rt_gc_object *
rt_gc_copy_array_to_graduate(
	struct rt_env *env,
	struct rt_array *old_obj)
{
	struct rt_array *new_obj;
	uint32_t i;
	size_t size;

	assert(env != NULL);
	assert(old_obj != NULL);
	assert(old_obj->alloc_size > 0);

	size = old_obj->size;
	if (size == 0)
		size = old_obj->alloc_size;

	/*
	 * Arrays larger than this size must not be in the nursery or
	 * graduate regions.
	 */
	assert(size * sizeof(struct rt_value *) < env->vm->config.gc_lop_threshold);

	/* Allocate in the graduate region. (If failed, in the tenure region.) */
	new_obj = rt_gc_alloc_array_graduate(env, size);
	if (new_obj == NULL)
		return NULL;

	/* Copy the table. */
	new_obj->size = old_obj->size;
	memcpy(new_obj->table, old_obj->table, old_obj->size * sizeof(struct rt_value));

	/* Check for cross-generation references. */
	if (new_obj->head.region == RT_GC_REGION_TENURE) {
		for (i = 0; i < new_obj->size; i++) {
			if (IS_REF_VAL(&new_obj->table[i]) &&
			    IS_YOUNG_OBJ(new_obj->table[i].val.obj)) {
				new_obj->head.rem_flg = true;
				INSERT_TO_LIST(&new_obj->head, env->vm->gc.remember_set,rem_prev, rem_next);
				break;
			}
		}
	}

	return &new_obj->head;
}

/* Copies a dictionary object to the graduate region. */
static struct rt_gc_object *
rt_gc_copy_dict_to_graduate(
	struct rt_env *env,
	struct rt_dict *old_obj)
{
	struct rt_dict *new_obj;
	uint32_t i;
	size_t size;

	assert(env != NULL);
	assert(old_obj != NULL);
	assert(old_obj->alloc_size > 0);

	size = old_obj->alloc_size;

	/*
	 * Dictionaries larger than this value must not be in the
	 * nursery or graduate regions.
	 */
	assert(size * sizeof(struct rt_value *) * 2 < env->vm->config.gc_lop_threshold);

	/* Allocate in the graduate region. (If failed, in the tenure region.) */
	new_obj = rt_gc_alloc_dict_graduate(env, size);
	if (new_obj == NULL)
		return NULL;

	/* Copy the keys and values. */
	new_obj->size = old_obj->size;
	memcpy(new_obj->key, old_obj->key, old_obj->alloc_size * sizeof(struct rt_value));
	memcpy(new_obj->value, old_obj->value, old_obj->alloc_size * sizeof(struct rt_value));

	/* Check for cross-generation references. */
	if (new_obj->head.region == RT_GC_REGION_TENURE) {
		for (i = 0; i < (uint32_t)new_obj->size; i++) {
			if (IS_YOUNG_OBJ(new_obj->key[i].val.obj)) {
				new_obj->head.rem_flg = true;
				INSERT_TO_LIST(&new_obj->head, env->vm->gc.remember_set,rem_prev, rem_next);
				break;
			}
			if (IS_REF_VAL(&new_obj->value[i]) &&
			    IS_YOUNG_OBJ(new_obj->value[i].val.obj)) {
				new_obj->head.rem_flg = true;
				INSERT_TO_LIST(&new_obj->head, env->vm->gc.remember_set,rem_prev, rem_next);
				break;
			}
		}
	}

	new_obj->native_pointer = old_obj->native_pointer;
	new_obj->native_finalizer = old_obj->native_finalizer;

	/* Succeeded. */
	return &new_obj->head;
}

/* Copies a string object to the graduate region. */
static struct rt_gc_object *
rt_gc_copy_packed_to_graduate(
	struct rt_env *env,
	struct rt_packed *old_obj)
{
	struct rt_packed *new_obj;

	/*
	 * Packed larger than noct_conf_gc_lop_threshold must not be in the
	 * nursery or graduate regions.
	 */
	assert(old_obj->size < env->vm->config.gc_lop_threshold);

	/* Allocate in the graduate region. */
	new_obj = rt_gc_alloc_packed_graduate(env,
					      old_obj->type,
					      old_obj->size,
					      old_obj->elem_size,
					      (old_obj->size == 0) ? old_obj->packed_buffer : NULL);
	if (new_obj == NULL)
		return NULL;

	/* If not a preallocated. (that means packed_buffer is not managed by GC)  */
	if (old_obj->size != 0)
		memcpy(new_obj->packed_buffer, old_obj->packed_buffer, old_obj->size);

	/* Succeeded. */
	return &new_obj->head;
}

/* Executes an old GC in the multithreaded manner. */
static void
rt_gc_old_gc(
	struct rt_env *env)
{
	om_enter_gc(env, RT_GC_LEVEL_1);
	rt_gc_old_gc_body(env);
	om_leave_gc(env);
}

/* Executes an old GC. */
static void
rt_gc_old_gc_body(
	struct rt_env *env)
{
	struct rt_gc_object *obj, *next_obj;
	struct rt_frame *frame;
	uint32_t i;
	int sp;

	/*
	 * Clear marks.
	 */

	/* Clear marks of the nursery objects. */
	obj = env->vm->gc.nursery_list;
	while (obj != NULL) {
		obj->is_marked = false;
		obj = obj->next;
	}

	/* Clear marks of the graduate objects. */
	obj = env->vm->gc.graduate_list;
	while (obj != NULL) {
		obj->is_marked = false;
		obj = obj->next;
	}

	/* Clear marks of the tenure objects. */
	obj = env->vm->gc.tenure_list;
	while (obj != NULL) {
		obj->is_marked = false;
		obj = obj->next;
	}

	/*
	 * Mark.
	 */

	/* For all global variables. */
	for (i = 0; i < (uint32_t)env->vm->global_alloc_size; i++) {
		if (env->vm->global[i].name == NULL || env->vm->global[i].is_removed)
			continue;
		if (IS_REF_VAL(&env->vm->global[i].val))
			rt_gc_mark_old_object_recursively(env, &env->vm->global[i].val.val.obj);
	}

	/* For all call frames. */
	for (sp = env->cur_frame_index; sp >= 0; sp--) {
		frame = &env->frame_alloc[sp];

		/* For all temporary variables in the frame. */
		for (i = 0; i < frame->tmpvar_size; i++) {
			if (IS_REF_VAL(&frame->tmpvar[i]))
				rt_gc_mark_old_object_recursively(env, &frame->tmpvar[i].val.obj);
		}

		/* For all pinned C local variables in the frame. */
		for (i = 0; i < frame->pinned_count; i++) {
			if (IS_REF_VAL(frame->pinned[i]))
				rt_gc_mark_old_object_recursively(env, &frame->pinned[i]->val.obj);
		}
	}

	/* For all pinned C global variables. */
	for (i = 0; i < env->vm->pinned_count; i++) {
		if (IS_REF_VAL(env->vm->pinned[i]))
			rt_gc_mark_old_object_recursively(env, &env->vm->pinned[i]->val.obj);
	}

	/*
	 * Sweep.
	 */

	/* For all tenure objects. */
	obj = env->vm->gc.tenure_list;
	while (obj != NULL) {
		next_obj = obj->next;

		/* Free if not marked. */
		if (!obj->is_marked)
			rt_gc_free_old_object(env, obj);

		obj = next_obj;
	}
}

/* Mark object recursively for the old GC. */
static void
rt_gc_mark_old_object_recursively(
	struct rt_env *env,
	struct rt_gc_object **obj)
{
	uint32_t i;

	/* Follow the newer array/dict. */
	rt_gc_array_dict_follow_newer(env, obj);

	/* If already marked, just return. */
	if ((*obj)->is_marked)
		return;

	/* Mark. */
	(*obj)->is_marked = true;

	/* Mark recursively. */
	if ((*obj)->type == RT_GC_TYPE_ARRAY) {
		struct rt_array *arr = (struct rt_array *)*obj;
		for (i = 0; i < arr->size; i++) {
			if (IS_REF_VAL(&arr->table[i]))
				rt_gc_mark_old_object_recursively(env, &arr->table[i].val.obj);
		}
	} else if ((*obj)->type == RT_GC_TYPE_DICT) {
		struct rt_dict *dict = (struct rt_dict *)*obj;
		for (i = 0; i < dict->alloc_size; i++) {
			if (dict->key[i].type != NOCT_VALUE_STRING)
				continue; /* Removed or empty. */

			rt_gc_mark_old_object_recursively(env, &dict->key[i].val.obj);
			if (IS_REF_VAL(&dict->value[i]))
				rt_gc_mark_old_object_recursively(env, &dict->value[i].val.obj);
		}
	}
}

/* Free a string, array, or dictionary object. */
static void
rt_gc_free_old_object(
	struct rt_env *env,
	struct rt_gc_object *obj)
{
	assert(obj->region == RT_GC_REGION_TENURE);

	/*
	 * Nursery and graduate objects are allocated by arena
	 * allocater, and no need for freeing.
	 */
	if (obj->region != RT_GC_REGION_TENURE)
		return;

	/* Unlink from the tenure list. */
	UNLINK_FROM_LIST(obj, env->vm->gc.tenure_list, prev, next);

	/* Unlink from the remember set. */
	if (obj->rem_flg)
		UNLINK_FROM_LIST(obj, env->vm->gc.remember_set, rem_prev, rem_next);

	/* Free. */
	if (obj->type == RT_GC_TYPE_STRING) {
		struct rt_string *str;
		str = (struct rt_string *)obj;
		rt_gc_tenure_free(env, str);
	} else if (obj->type == RT_GC_TYPE_ARRAY) {
		struct rt_array *arr;
		arr = (struct rt_array *)obj;
		rt_gc_tenure_free(env, arr);
	} else if (obj->type == RT_GC_TYPE_DICT) {
		struct rt_dict *dict;
		dict = (struct rt_dict *)obj;
		rt_gc_tenure_free(env, dict);
	} else if (obj->type == RT_GC_TYPE_PACKED) {
		struct rt_packed *packed;
		packed = (struct rt_packed *)obj;
		rt_gc_tenure_free(env, packed);
	}
}

/* Create the compaction table. */
static bool
rt_gc_compact_gc(
	struct rt_env *env)
{
	struct rt_gc_object *obj, **objpp;
	char *cur_blk, *remap_top;
	uint32_t index, i;
	int sp;
	struct rt_frame *frame;

	/*
	 * Count all tenure objects.
	 */

	env->vm->gc.compact_count = 0;
	obj = env->vm->gc.tenure_list;
	while (obj != NULL) {
		env->vm->gc.compact_count++;
		obj = obj->next;
	}
	if (env->vm->gc.compact_count == 0)
		return true;

	/*
	 * Initialize the compaction table.
	 */

	/* Allocate the compaction table. */
	env->vm->gc.compact_before = noct_malloc(env->vm->gc.compact_count * sizeof(void *));
	if (env->vm->gc.compact_before == NULL) {
		rt_out_of_memory(env);
		return false;
	}
	env->vm->gc.compact_after = noct_malloc(env->vm->gc.compact_count * sizeof(void *));
	if (env->vm->gc.compact_after == NULL) {
		noct_free(env->vm->gc.compact_before);
		rt_out_of_memory(env);
		return false;
	}

	/* Traverse the tenure region. */
	cur_blk = env->vm->gc.tenure_freelist.top;
	remap_top = env->vm->gc.tenure_freelist.top;
	index = 0;
	while (cur_blk < env->vm->gc.tenure_freelist.end) {
		size_t blk_size;
		bool blk_used;
		struct rt_gc_object *obj;

		blk_size = *(size_t *)cur_blk;
		blk_used = blk_size & RT_GC_FREELIST_USED_BIT ? true : false;
		obj = (struct rt_gc_object *)(cur_blk + sizeof(size_t));

		/* Check for the end of the list. */
		if (blk_size == 0)
			break;

		/* Skip if unused. */
		if (!blk_used) {
			cur_blk += sizeof(size_t) + blk_size;
			continue;
		}

		/* Record. */
		env->vm->gc.compact_before[index] = obj;
		env->vm->gc.compact_after[index] = remap_top + sizeof(size_t);
		remap_top += sizeof(size_t) + obj->size;
		index++;

		cur_blk += sizeof(size_t) + blk_size;
	}
	assert(cur_blk < env->vm->gc.tenure_freelist.end);
	assert(index == env->vm->gc.compact_count);

	/*
	 * Slide tenure objects.
	 */

	for (i = 0; i < env->vm->gc.compact_count; i++) {
		/* Get the real object size. */
		size_t obj_size = ((struct rt_gc_object *)env->vm->gc.compact_before[i])->size;

		/* Move. */
		memmove(env->vm->gc.compact_after[i],
			env->vm->gc.compact_before[i],
			obj_size);

		/* Store the size header. */
		*(size_t *)((char *)env->vm->gc.compact_after[i] - sizeof(size_t)) = obj_size;

		/* Fill the reminder. */
		if (i == env->vm->gc.compact_count - 1) {
			memset((char *)env->vm->gc.compact_after[i] + obj_size,
			       0,
			       (size_t)(env->vm->gc.tenure_freelist.end - ((char *)env->vm->gc.compact_after[i] + obj_size)));
		}
	}

	/*
	 * Rewrite all references.
	 */

	/* For all tenure list references. */
	objpp = &env->vm->gc.tenure_list;
	while (*objpp != NULL) {
		/* Rewrite pointers. */
		if ((*objpp)->type == RT_GC_TYPE_ARRAY) {
			struct rt_array *arr;
			arr = (struct rt_array *)*objpp;
			arr->table = (struct rt_value *)((char *)arr + sizeof(struct rt_array));
		} else if ((*objpp)->type == RT_GC_TYPE_DICT) {
			struct rt_dict *dict;
			dict = (struct rt_dict *)*objpp;
			dict->key = (struct rt_value *)((char *)dict + sizeof(struct rt_dict));
			dict->value = (struct rt_value *)((char *)dict + sizeof(struct rt_dict) + dict->alloc_size * sizeof(struct rt_value));
		} else if ((*objpp)->type == RT_GC_TYPE_PACKED) {
			struct rt_packed *packed;
			packed = (struct rt_packed *)*objpp;
			/* Move reference if not a preallocated buffer. */
			if (packed->size != 0)
				packed->packed_buffer = ((char *)packed + sizeof(struct rt_packed));
		}

		/* Rewrite ->next. */
		rt_gc_update_tenure_ref(env, objpp);

		/* Rewrite ->prev. */
		if ((*objpp)->prev != NULL)
			rt_gc_update_tenure_ref(env, &(*objpp)->prev);
		objpp = &(*objpp)->next;
	}

	/* For all remember set references. */
	objpp = &env->vm->gc.remember_set;
	while (*objpp != NULL) {
		/* Rewrite ->rem_next. */
		rt_gc_update_tenure_ref_recursively(env, objpp);

		/* Rewrite ->rem_prev. */
		if ((*objpp)->rem_prev != NULL)
			rt_gc_update_tenure_ref(env, &(*objpp)->rem_prev);
		objpp = &(*objpp)->rem_next;
	}

	/* For all global variables. */
	for (i = 0; i < (uint32_t)env->vm->global_alloc_size; i++) {
		if (env->vm->global[i].name == NULL || env->vm->global[i].is_removed)
			continue;
		if (IS_REF_VAL(&env->vm->global[i].val))
			rt_gc_update_tenure_ref_recursively(env, &env->vm->global[i].val.val.obj);
	}

	/* For all call frames. */
	for (sp = env->cur_frame_index; sp >= 0; sp--) {
		frame = &env->frame_alloc[sp];

		/* For all temporary variables in the frame. */
		for (i = 0; i < frame->tmpvar_size; i++) {
			if (IS_REF_VAL(&frame->tmpvar[i]))
				rt_gc_update_tenure_ref_recursively(env, &frame->tmpvar[i].val.obj);
		}

		/* For all pinned C local variables in the frame. */
		for (i = 0; i < frame->pinned_count; i++) {
			if (IS_REF_VAL(frame->pinned[i]))
				rt_gc_update_tenure_ref_recursively(env, &frame->pinned[i]->val.obj);
		}
	}

	/* For all pinned C global variables. */
	for (i = 0; i < env->vm->pinned_count; i++) {
		if (IS_REF_VAL(env->vm->pinned[i]))
			rt_gc_update_tenure_ref_recursively(env, &env->vm->pinned[i]->val.obj);
	}

	/*
	 * Cleanup the compaction table.
	 */

	if (env->vm->gc.compact_before != NULL) {
		noct_free(env->vm->gc.compact_before);
		env->vm->gc.compact_before = NULL;
	}
	if (env->vm->gc.compact_after != NULL) {
		noct_free(env->vm->gc.compact_after);
		env->vm->gc.compact_after = NULL;
	}

	return true;
}

/* Recursively update the tenure references for compaction. */
static void
rt_gc_update_tenure_ref(
	struct rt_env *env,
	struct rt_gc_object **obj)
{
	uint32_t i;

	/* Search in the compaction table. */
	for (i = 0; i < (uint32_t)env->vm->gc.compact_count; i++) {
		if (env->vm->gc.compact_before[i] == *obj)
			break;
	}
	if (i == env->vm->gc.compact_count)
		return;	/* Not found. */

	/* Update the reference. */
	*obj = env->vm->gc.compact_after[i];
}

/* Recursively update the tenure references for compaction. */
static void
rt_gc_update_tenure_ref_recursively(
	struct rt_env *env,
	struct rt_gc_object **obj)
{
	uint32_t i;

	/* Search in the compaction table. */
	for (i = 0; i < (uint32_t)env->vm->gc.compact_count; i++) {
		if (env->vm->gc.compact_before[i] == *obj)
			break;
	}
	if (i == env->vm->gc.compact_count)
		return;	/* Not found. */

	/* Update the reference. */
	*obj = env->vm->gc.compact_after[i];

	/* Recursively update. */
	if ((*obj)->type == RT_GC_TYPE_ARRAY) {
		struct rt_array *arr = (struct rt_array *)*obj;
		for (i = 0; i < arr->size; i++) {
			if (IS_REF_VAL(&arr->table[i]))
				rt_gc_update_tenure_ref_recursively(env, &arr->table[i].val.obj);
		}
	} else if ((*obj)->type == RT_GC_TYPE_DICT) {
		struct rt_dict *dict = (struct rt_dict *)*obj;
		for (i = 0; i < dict->alloc_size; i++) {
			if (dict->key[i].type != NOCT_VALUE_STRING)
				continue; /* Removed or empty. */

			rt_gc_update_tenure_ref_recursively(env, &dict->key[i].val.obj);
			if (IS_REF_VAL(&dict->value[i]))
				rt_gc_update_tenure_ref_recursively(env, &dict->value[i].val.obj);
		}
	}
}

/*
 * Pins a C global variable.
 */
bool
rt_gc_pin_global(
	struct rt_env *env,
	struct rt_value *val)
{
	assert(env != NULL);
	assert(val != NULL);

	if (env->vm->pinned_count == RT_GLOBAL_PIN_MAX) {
		rt_error(env, N_TR("Too many pinned global variables."));
		return false;
	}

	env->vm->pinned[env->vm->pinned_count++] = val;

	return true;
}

/*
 * Unpins a C global variable.
 */
bool
rt_gc_unpin_global(
	struct rt_env *env,
	struct rt_value *val)
{
	int i;

	assert(env != NULL);
	assert(val != NULL);

	for (i = (int)env->vm->pinned_count - 1; i >= 0; i--) {
		if (env->vm->pinned[i] == val) {
			if (i != (int)env->vm->pinned_count - 1) {
				memmove(&env->vm->pinned[i],
					&env->vm->pinned[i+1],
					(size_t)(RT_GLOBAL_PIN_MAX - i - 1) * sizeof(struct rt_value *));
			}
			env->vm->pinned_count--;

			/* Succeeded. */
			return true;
		}
	}

	/* Failed. */
	assert(PINNED_VAR_NOT_FOUND);
	return false;
}

/*
 * Pins a C local variable.
 */
bool
rt_gc_pin_local(
	struct rt_env *env,
	struct rt_value *val)
{
	assert(env != NULL);
	assert(val != NULL);

	if (env->frame->pinned_count == RT_LOCAL_PIN_MAX) {
		rt_error(env, N_TR("Too many pinned local variables."));
		return false;
	}

	env->frame->pinned[env->frame->pinned_count++] = val;

	return true;
}

/*
 * Unpins a C local variable.
 */
bool
rt_gc_unpin_local(
	struct rt_env *env,
	struct rt_value *val)
{
	uint32_t i;

	assert(env != NULL);
	assert(val != NULL);

	for (i = 0; i < env->frame->pinned_count; i++) {
		if (env->frame->pinned[i] == val) {
			memmove(&env->frame->pinned[i], &env->frame->pinned[i+1], (RT_LOCAL_PIN_MAX - i - 1) * sizeof(struct rt_value *));
			env->frame->pinned_count--;

			/* Succeeded. */
			return true;
		}
	}

	/* Failed. */
	assert(PINNED_VAR_NOT_FOUND);
	return false;
}

/*
 * Retrieves the approximate memory usage, in bytes.
 */
bool
rt_gc_get_heap_usage(
	struct rt_env *env,
	size_t *ret)
{
	UNUSED_PARAMETER(env);

	assert(env != NULL);
	assert(ret != NULL);

	/* TODO */
	*ret = 0;

	return true;
}

/*
 * Manually trigger the young GC.
 */
void
rt_gc_level1_gc(struct rt_env *env)
{
	rt_gc_young_gc(env);
}

/*
 * Manually trigger the old GC.
 */
void rt_gc_level2_gc(struct rt_env *env)
{
	rt_gc_young_gc(env);
	rt_gc_old_gc(env);
}

/*
 * Manually triggers a  GC.
 */
void rt_gc_level3_gc(struct rt_env *env)
{
	rt_gc_young_gc(env);
	rt_gc_old_gc(env);
	rt_gc_compact_gc(env);
}

static void *
nursery_alloc(
	struct rt_env *env,
	size_t size)
{
	/* Allocate from the nursery arena. */
	return arena_alloc(&env->vm->gc.nursery_arena, size);
}

static void *
graduate_alloc(
	struct rt_env *env,
	size_t size)
{
	/* Allocate from the graduate arena. */
	return arena_alloc(&env->vm->gc.graduate_arena[env->vm->gc.cur_grad_to], size);
}

/*
 * Allocate a tenure block.
 *
 * The allocator for the tenure region.
 * Each block has its size at the block top.
 * The LSB of the block size indicates the block is used (set) or freed (clear).
 */
static void *
rt_gc_tenure_alloc(
	struct rt_env *env,
	size_t size)
{
	char *cur;

	assert(size > 0);
	if (size == 0)
		return NULL;

	/* Align. */
	size = (size + RT_GC_FREELIST_ALIGN - 1) & ~(RT_GC_FREELIST_ALIGN - 1);

	/* The second blk */
	cur = env->vm->gc.tenure_freelist.top;

	/* Search for the first match. */
	while (*(size_t *)cur) {
		size_t blk_size = *(size_t *)cur;
		bool is_used;

		/* Check for the end of the list. */
		if (blk_size == 0)
			break;

		is_used = blk_size & RT_GC_FREELIST_USED_BIT;
		blk_size &= blk_size & RT_GC_FREELIST_SIZE_MASK;

		/* Check if the block is used or the size is small. */
		if (is_used || size > blk_size) {
			cur = cur + sizeof(size_t) + blk_size;
			assert(cur < env->vm->gc.tenure_freelist.end);
			continue;
		}

		/* Reuse this block. */
		blk_size |= RT_GC_FREELIST_USED_BIT;
		*(size_t *)cur = blk_size;

		/* Return the address of the block top + the size of the size header. */
		return cur + sizeof(size_t);
	}

	/* Check if the remaining size fits. */
	if (size > (size_t)(env->vm->gc.tenure_freelist.end - (uintptr_t)cur - sizeof(size_t)))
		return NULL;

	/* Allocate at the end of the free list. */
	*(size_t *)cur = size | RT_GC_FREELIST_USED_BIT;
	return cur + sizeof(size_t);
}

/* Free a tenure block. */
static void
rt_gc_tenure_free(
	struct rt_env *env,
	void *p)
{
	size_t *header;
	size_t size;

	UNUSED_PARAMETER(env);

	/* Get the header address. */
	header = (size_t *)((char *)p - sizeof(size_t));

	/* Get the block size. */
	size = *header;

	/* Block must be used. (check the used bit.) */
	assert(size & RT_GC_FREELIST_USED_BIT);

	/* Erase the used bit. */
	*header = size & RT_GC_FREELIST_SIZE_MASK;
}
