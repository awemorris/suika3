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
#define INSERT_TO_LIST(elem, list, prev, next)				\
	do {								\
		(elem)->prev = NULL;					\
		(elem)->next = (list);					\
		if ((list) != NULL) {					\
			assert(elem != (list)->next);			\
			(list)->prev = (elem);				\
		}							\
		(list) = (elem);					\
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
#define IS_REF_VAL(v)							\
	((v)->type == NOCT_VALUE_STRING ||				\
	 (v)->type == NOCT_VALUE_ARRAY ||				\
	 (v)->type == NOCT_VALUE_DICT ||				\
	 (v)->type == NOCT_VALUE_PACKED)

/*
 * Heap lock.
 *  - Guards the nursery arena, the tenure freelist, and the GC object
 *    lists against concurrent mutator allocations.
 *  - Critical sections are short and never park at a safepoint nor
 *    trigger a GC, so a plain spin lock does not deadlock with the
 *    stop-the-world machinery.
 */
#if defined(NOCT_USE_MULTITHREAD)
#define HEAP_LOCK(env)							\
	do {								\
		atomic_spin_lock(&(env)->vm->heap_lock);		\
	} while (0)
#define HEAP_UNLOCK(env)						\
	do {								\
		atomic_spin_unlock(&(env)->vm->heap_lock);	\
	} while (0)
#else
#define HEAP_LOCK(env)		do {} while (0)
#define HEAP_UNLOCK(env)	do {} while (0)
#endif

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
static struct rt_packed *rt_gc_alloc_packed_graduate(struct rt_env *env, int type, size_t size, size_t elem_size, void *preallocated, void *native_pointer, void (*native_finalizer)(void *native_pointer));
static struct rt_packed *rt_gc_alloc_packed_tenure(struct rt_env *env, int type, size_t size, size_t elem_size, void *preallocated, void *native_pointer, void (*native_finalizer)(void *native_pointer));
static void rt_gc_young_gc(struct rt_env *env);
static void rt_gc_young_gc_body(struct rt_env *env);
static bool rt_gc_copy_young_object(struct rt_env *env, struct rt_gc_object **obj);
static bool rt_gc_work_push(struct rt_env *env, struct rt_gc_object **slot);
static bool rt_gc_promoted_push(struct rt_env *env, struct rt_gc_object *obj);
static void rt_gc_note_promoted_cross_refs(struct rt_env *env, struct rt_gc_object *obj);
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
static bool rt_gc_mark_old_object(struct rt_env *env, struct rt_gc_object **obj);
static void rt_gc_finalize_object(struct rt_gc_object *obj);
static void rt_gc_free_old_object(struct rt_env *env, struct rt_gc_object *obj);
static bool rt_gc_compact_gc(struct rt_env *env);
static struct rt_gc_object *rt_gc_compact_remap(struct rt_env *env, struct rt_gc_object *obj);
static void rt_gc_compact_update_value(struct rt_env *env, struct rt_value *v);
static void rt_gc_compact_update_object(struct rt_env *env, struct rt_gc_object *obj);
static bool rt_gc_compact_gc_body(struct rt_env *env);
static void *nursery_alloc(struct rt_env *env, size_t size);
static int rt_gc_tenure_bin(size_t size);
static void rt_gc_tenure_rebuild_bins(struct rt_env *env);
static void *graduate_alloc(struct rt_env *env, size_t size);
static void *rt_gc_tenure_alloc(struct rt_env *env, size_t size);
static void rt_gc_tenure_free(struct rt_env *env, void *p);
static void env_gc_cleanup(struct rt_vm *vm);

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
	vm->gc.tenure_frontier = vm->gc.tenure_freelist.top;

	return true;
}

void
rt_gc_cleanup(
	struct rt_vm *vm)
{
	struct rt_gc_object *obj;

	/* Finalize all native owners that remain reachable at VM shutdown. */
	for (obj = vm->gc.nursery_list; obj != NULL; obj = obj->next)
		rt_gc_finalize_object(obj);
	for (obj = vm->gc.graduate_list; obj != NULL; obj = obj->next)
		rt_gc_finalize_object(obj);
	for (obj = vm->gc.graduate_new_list; obj != NULL; obj = obj->next)
		rt_gc_finalize_object(obj);
	for (obj = vm->gc.tenure_list; obj != NULL; obj = obj->next)
		rt_gc_finalize_object(obj);

	env_gc_cleanup(vm);
}

/*
 * Cleanups the garbage collector and deallocate memory regions.
 */
static void env_gc_cleanup(struct rt_vm *vm)
{
	/* Free the traversal worklist. */
	noct_free(vm->gc.work);
	noct_free(vm->gc.promoted);

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
	char *saved;
	int retry;

	saved = NULL;

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
	for (retry = 0; retry <= 2; retry++) {
		/* Allocate a rt_string buffer. */
		HEAP_LOCK(env);
		rts = nursery_alloc(env, sizeof(struct rt_string) + len);
		if (rts == NULL) {
			HEAP_UNLOCK(env);
			/* Retry. */
			if (retry == 0) {
				/*
				 * "data" may point into the heap we are about
				 * to collect, so save it before collecting.
				 */
				if (saved == NULL) {
					saved = noct_malloc(len);
					if (saved == NULL) {
						rt_out_of_memory(env);
						return NULL;
					}
					memcpy(saved, data, len);
					data = saved;
				}
				rt_gc_young_gc(env);
				continue;
			} else if (retry == 1) {
				/*
				 * The young collection alone was not
				 * enough: the tenure region may be full of
				 * dead copies only the old GC can reclaim,
				 * which also starves the young GC's promote
				 * path (a full tenure demotes promotions
				 * into the tiny graduate space). Collect
				 * the old generation, compact, and give the
				 * young collection one more try -- from
				 * here, at top level, where nesting is not
				 * a concern.
				 */
				rt_gc_old_gc(env);
				rt_gc_compact_gc(env);
				rt_gc_young_gc(env);
				continue;
			} else {
				noct_free(saved);
				rt_out_of_memory(env);
				return NULL;
			}
		}

		/* Get the string top address. */
		s = (char *)rts + sizeof(struct rt_string);

		/* Copy the string. */
		memcpy(s, data, len);
		noct_free(saved);
		saved = NULL;

		/* Setup the struct. */
		memset(&rts->head, 0, sizeof(struct rt_gc_object));
		rts->head.type = RT_GC_TYPE_STRING;
		rts->head.region = RT_GC_REGION_NURSERY;
		rts->head.size = sizeof(struct rt_string) + len;
		INSERT_TO_LIST(&rts->head, env->vm->gc.nursery_list, prev, next);
		HEAP_UNLOCK(env);
		rts->data = s;
		rts->len = len;
		rts->hash = hash;
		rts->cache_index = 0;
		rts->cache_ofs = 0;

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
		rts->cache_index = 0;
		rts->cache_ofs = 0;

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
	char *saved;
	int retry;

	saved = NULL;

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
		HEAP_LOCK(env);
		rts = rt_gc_tenure_alloc(env, sizeof(struct rt_string) + len);
		if (rts == NULL) {
			HEAP_UNLOCK(env);
			/*
			 * "data" may point into the heap we are about
			 * to collect or compact, so save it before
			 * doing so.
			 */
			if (retry <= 1 && saved == NULL) {
				saved = noct_malloc(len);
				if (saved == NULL) {
					rt_out_of_memory(env);
					return NULL;
				}
				memcpy(saved, data, len);
				data = saved;
			}

			/*
			 * Retry -- but never by running another
			 * collection from inside one. The young GC's
			 * promote path lands here when the tenure
			 * region is full, and an old GC or a
			 * compaction started at that point clears the
			 * is_marked/forward bookkeeping the young
			 * collection is standing on: objects lose
			 * their marks mid-copy, get evacuated twice,
			 * and the diverged copies leave slots
			 * pointing at memory the young GC then
			 * wipes. When nested, fail the allocation
			 * instead; the caller demotes the promotion
			 * to a graduate copy.
			 */
			if (env->vm->gc_in_progress_counter > 0) {
				noct_free(saved);
				return NULL;
			}

			/* GC for a retry, or fail. */
			if (retry == 0) {
				rt_gc_old_gc(env);
				continue;
			} else if (retry == 1) {
				rt_gc_compact_gc(env);
				continue;
			} else {
				noct_free(saved);
				rt_out_of_memory(env);
				return NULL;
			}
		}

		/* Get the string top address. */
		s = (char *)rts + sizeof(struct rt_string);

		/* Copy the string. */
		memcpy(s, data, len);
		noct_free(saved);
		saved = NULL;

		/* Setup the struct. */
		memset(&rts->head, 0, sizeof(struct rt_gc_object));
		rts->head.type = RT_GC_TYPE_STRING;
		rts->head.region = RT_GC_REGION_TENURE;
		rts->head.size = sizeof(struct rt_string) + len;
		INSERT_TO_LIST(&rts->head, env->vm->gc.tenure_list, prev, next);
		HEAP_UNLOCK(env);
		rts->data = s;
		rts->len = len;
		rts->hash = hash;
		rts->cache_index = 0;
		rts->cache_ofs = 0;

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

	/*
	 * An empty container still needs a backing block. Use the
	 * minimum allocation size rather than rejecting size 0.
	 */
	if (size < 2)
		size = 2;

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
	for (retry = 0; retry <= 2; retry++) {
		/* Allocate a rt_array buffer. */
		HEAP_LOCK(env);
		arr = nursery_alloc(env, sizeof(struct rt_array) + size * sizeof(struct rt_value));
		if (arr == NULL) {
			HEAP_UNLOCK(env);
			/* Retry. */
			if (retry == 0) {
				rt_gc_young_gc(env);
				continue;
			} else if (retry == 1) {
				/*
				 * Not enough: reclaim the old
				 * generation and retry once more. See
				 * rt_gc_alloc_string.
				 */
				rt_gc_old_gc(env);
				rt_gc_compact_gc(env);
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
		HEAP_UNLOCK(env);
		arr->alloc_size = size;
		arr->size = 0;
		arr->table = table;
		arr->newer = NULL;
#if defined(NOCT_USE_MULTITHREAD)
		arr->shared = 0;
		arr->write_lock = 0;
		arr->seqlock = 0;
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

	/*
	 * An empty container still needs a backing block; use the
	 * minimum allocation size rather than rejecting size 0.
	 */
	if (size < 2)
		size = 2;

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
	 * This function is only called from the young GC, and thus,
	 * we don't use young GC for a retry here.
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
		arr->seqlock = 0;
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

	/*
	 * An empty container still needs a backing block; use the
	 * minimum allocation size rather than rejecting size 0.
	 */
	if (size < 2)
		size = 2;

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
		HEAP_LOCK(env);
		arr = rt_gc_tenure_alloc(env, sizeof(struct rt_array) + size * sizeof(struct rt_value));
		if (arr == NULL) {
			HEAP_UNLOCK(env);
			/*
			 * Retry -- but never by nesting a collection
			 * inside one (see rt_gc_alloc_string_tenure):
			 * when a GC is already running, fail so the
			 * caller can demote the promotion instead.
			 */
			if (env->vm->gc_in_progress_counter > 0)
				return NULL;

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

		/* Clear the table: tenure blocks are reused and hold garbage. */
		memset(table, 0, size * sizeof(struct rt_value));

		/* Setup the struct. */
		memset(&arr->head, 0, sizeof(struct rt_gc_object));
		arr->head.type = RT_GC_TYPE_ARRAY;
		arr->head.region = RT_GC_REGION_TENURE;
		arr->head.size = sizeof(struct rt_array) + size * sizeof(struct rt_value);
		INSERT_TO_LIST(&arr->head, env->vm->gc.tenure_list, prev, next);
		HEAP_UNLOCK(env);
		arr->alloc_size = size;
		arr->size = 0;
		arr->table = table;
		arr->newer = NULL;
#if defined(NOCT_USE_MULTITHREAD)
		arr->shared = 0;
		arr->write_lock = 0;
		arr->seqlock = 0;
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

	/*
	 * An empty container still needs a backing block; use the
	 * minimum allocation size rather than rejecting size 0.
	 */
	if (size < 2)
		size = 2;

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
	for (retry = 0; retry <= 2; retry++) {
		/* Allocate a rt_dict buffer. */
		HEAP_LOCK(env);
		dict = nursery_alloc(env, sizeof(struct rt_dict) + 2 * size * sizeof(struct rt_value));
		if (dict == NULL) {
			HEAP_UNLOCK(env);
			/* Retry. */
			if (retry == 0) {
				rt_gc_young_gc(env);
				continue;
			} else if (retry == 1) {
				/*
				 * Not enough: reclaim the old
				 * generation and retry once more. See
				 * rt_gc_alloc_string.
				 */
				rt_gc_old_gc(env);
				rt_gc_compact_gc(env);
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
		HEAP_UNLOCK(env);
		dict->alloc_size = size;
		dict->size = 0;
		dict->key = key_table;
		dict->value = value_table;
		dict->newer = NULL;
		dict->native_pointer = NULL;
		dict->native_finalizer = NULL;
		dict->is_frozen = false;
#if defined(NOCT_USE_MULTITHREAD)
		dict->shared = 0;
		dict->write_lock = 0;
		dict->seqlock = 0;
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

	/*
	 * An empty container still needs a backing block, use the
	 * minimum allocation size rather than rejecting size 0.
	 */
	if (size < 2)
		size = 2;

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
		dict->is_frozen = false;
#if defined(NOCT_USE_MULTITHREAD)
		dict->shared = 0;
		dict->write_lock = 0;
		dict->seqlock = 0;
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

	/*
	 * An empty container still needs a backing block, use the
	 * minimum allocation size rather than rejecting size 0.
	 */
	if (size < 2)
		size = 2;

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
		HEAP_LOCK(env);
		dict = rt_gc_tenure_alloc(env, sizeof(struct rt_dict) + 2 * size * sizeof(struct rt_value));
		if (dict == NULL) {
			HEAP_UNLOCK(env);
			/*
			 * Retry -- but never by nesting a collection
			 * inside one (see rt_gc_alloc_string_tenure):
			 * when a GC is already running, fail so the
			 * caller can demote the promotion instead.
			 */
			if (env->vm->gc_in_progress_counter > 0)
				return NULL;

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

		/* Clear the tables: tenure blocks are reused and hold garbage. */
		memset(key_table, 0, 2 * size * sizeof(struct rt_value));

		/* Setup a value. */
		memset(&dict->head, 0, sizeof(struct rt_gc_object));
		dict->head.type = RT_GC_TYPE_DICT;
		dict->head.region = RT_GC_REGION_TENURE;
		dict->head.size = sizeof(struct rt_dict) + 2 * size * sizeof(struct rt_value);
		INSERT_TO_LIST(&dict->head, env->vm->gc.tenure_list, prev, next);
		HEAP_UNLOCK(env);
		dict->alloc_size = size;
		dict->size = 0;
		dict->key = key_table;
		dict->value = value_table;
		dict->newer = NULL;
		dict->native_pointer = NULL;
		dict->native_finalizer = NULL;
		dict->is_frozen = false;
#if defined(NOCT_USE_MULTITHREAD)
		dict->shared = 0;
		dict->write_lock = 0;
		dict->seqlock = 0;
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
	void *preallocated,
	void *native_pointer,
	void (*native_finalizer)(void *native_pointer))
{
	struct rt_packed *packed;
	void *p;
	int retry;

	assert(env != NULL);
	assert(size > 0);
	assert(elem_size > 0);
	assert((native_pointer == NULL) == (native_finalizer == NULL));

	/* If use a preallocated buffer. */
	if (preallocated != NULL)
		size = 0;

	/* Owned external buffers stay in tenure and never duplicate ownership. */
	if (native_finalizer != NULL) {
		return rt_gc_alloc_packed_tenure(env,
						 type,
						 size,
						 elem_size,
						 preallocated,
						 native_pointer,
						 native_finalizer);
	}

	/*
	 * [Large Object Promotion]
	 *  - If the packed is large, allocate in the tenure region.
	 */
	if (size >= env->vm->config.gc_lop_threshold)
		return rt_gc_alloc_packed_tenure(env, type, size, elem_size,
						   preallocated, NULL, NULL);

	/* Allocate in the nursery region. */
	for (retry = 0; retry <= 2; retry++) {
		/* Allocate a rt_packed buffer. */
		HEAP_LOCK(env);
		packed = nursery_alloc(env, sizeof(struct rt_packed) + size);
		if (packed == NULL) {
			HEAP_UNLOCK(env);
			/* Retry. */
			if (retry == 0) {
				rt_gc_young_gc(env);
				continue;
			} else if (retry == 1) {
				/*
				 * Not enough: reclaim the old
				 * generation and retry once more. See
				 * rt_gc_alloc_string.
				 */
				rt_gc_old_gc(env);
				rt_gc_compact_gc(env);
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
		HEAP_UNLOCK(env);
		packed->type = type;
		packed->size = size;
		packed->elem_size = elem_size;
		packed->packed_typed = false;
		packed->packed_buffer = p;
		packed->native_pointer = NULL;
		packed->native_finalizer = NULL;

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
	void *preallocated,
	void *native_pointer,
	void (*native_finalizer)(void *native_pointer))
{
	struct rt_packed *packed;
	void *p;

	assert(env != NULL);
	assert(elem_size > 0);
	assert((native_pointer == NULL) == (native_finalizer == NULL));

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
		packed->packed_typed = false;
		packed->packed_buffer = p;
		packed->native_pointer = native_pointer;
		packed->native_finalizer = native_finalizer;

		/* Succeeded. (graduate) */
		return packed;
	} while (0);

	/*
	 * Failed to allocate in the graduate region.
	 * Try allocating in the tenure region.
	 */
	packed = rt_gc_alloc_packed_tenure(env, type, size, elem_size,
					   preallocated, native_pointer,
					   native_finalizer);
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
	void *preallocated,
	void *native_pointer,
	void (*native_finalizer)(void *native_pointer))
{
	struct rt_packed *packed;
	void *p;
	int retry;

	assert(env != NULL);
	assert((native_pointer == NULL) == (native_finalizer == NULL));

	/* If use a preallocated buffer. */
	if (preallocated != NULL)
		size = 0;

	/* Allocate in the tenure region. */
	for (retry = 0; retry <= 2; retry++) {
		/* Allocate a rt_packed buffer. */
		HEAP_LOCK(env);
		packed = rt_gc_tenure_alloc(env, sizeof(struct rt_packed) + size);
		if (packed == NULL) {
			HEAP_UNLOCK(env);

			/*
			 * Retry -- but never by nesting a collection
			 * inside one (see rt_gc_alloc_string_tenure):
			 * when a GC is already running, fail so the
			 * caller can demote the promotion instead.
			 */
			if (env->vm->gc_in_progress_counter > 0)
				return NULL;

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
		HEAP_UNLOCK(env);
		packed->type = type;
		packed->size = size;
		packed->elem_size = elem_size;
		packed->packed_typed = false;
		packed->packed_buffer = p;
		packed->native_pointer = native_pointer;
		packed->native_finalizer = native_finalizer;

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
	    IS_REF_VAL(val) &&
	    IS_YOUNG_OBJ(val->val.obj)) {
		HEAP_LOCK(env);
		if (!arr->head.rem_flg) {
			arr->head.rem_flg = true;
			INSERT_TO_LIST(&arr->head, env->vm->gc.remember_set, rem_prev, rem_next);
		}
		HEAP_UNLOCK(env);
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
	    IS_REF_VAL(val) &&
	    IS_YOUNG_OBJ(val->val.obj)) {
		HEAP_LOCK(env);
		if (!dict->head.rem_flg) {
			dict->head.rem_flg = true;
			INSERT_TO_LIST(&dict->head, env->vm->gc.remember_set, rem_prev, rem_next);
		}
		HEAP_UNLOCK(env);
	}
}

/* Executes a young GC in the multithreaded manner. */
static void
rt_gc_young_gc(
	struct rt_env *env)
{
	/* Make sure the tenure region can absorb this collection before it starts. */
	if (env->vm->gc_in_progress_counter == 0) {
		size_t headroom = (size_t)(env->vm->gc.tenure_freelist.end -
					   env->vm->gc.tenure_frontier);
		size_t need = env->vm->config.gc_nursery_size +
			env->vm->config.gc_graduate_size;
		if (headroom < need) {
			rt_gc_old_gc(env);
			rt_gc_compact_gc(env);
		}
	}

	/* A false return means this collection is redundant. */
	if (!om_enter_gc(env, RT_GC_LEVEL_0))
		return;

	rt_gc_young_gc_body(env);

	om_leave_gc(env);
}

/* Executes a young GC. */
static void
rt_gc_young_gc_body(
	struct rt_env *env)
{
	struct finalize_table {
		void *native_pointer;
		void (*native_finalizer)(void *native_pointer);
	};

	struct rt_gc_object *obj;
	struct rt_frame *frame;
	struct finalize_table *finalize_table;
	struct rt_env *e;
	uint32_t i;
	int sp;
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
		} else if (obj->type == RT_GC_TYPE_PACKED) {
			if (((struct rt_packed *)obj)->native_finalizer != NULL)
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

	/* Evacuees of an aborted previous collection. */
	obj = env->vm->gc.graduate_new_list;
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
			if (!rt_gc_copy_young_object(env, &env->vm->global[i].val.val.obj))
				return;
		}
	}

	/* For all call frames of all thread envs. */
	for (e = env->vm->env_list; e != NULL; e = e->next) {
		for (sp = (int)e->cur_frame_index; sp >= 0; sp--) {
			frame = &e->frame_alloc[sp];

			/* For all temporary variables in the frame. */
			for (i = 0; i < frame->tmpvar_size; i++) {
				if (IS_REF_VAL(&frame->tmpvar[i])) {
					if (!rt_gc_copy_young_object(env, &frame->tmpvar[i].val.obj))
						return;
				}
			}

			/* For all pinned C local variables in the frame. */
			for (i = 0; i < frame->pinned_count; i++) {
				if (IS_REF_VAL(frame->pinned[i])) {
					if (!rt_gc_copy_young_object(env, &frame->pinned[i]->val.obj))
						return;
				}
			}
		}
	}

	/* For all pinned C global variables. */
	for (i = 0; i < env->vm->pinned_count; i++) {
		if (IS_REF_VAL(env->vm->pinned[i])) {
			if (!rt_gc_copy_young_object(env, &env->vm->pinned[i]->val.obj))
				return;
		}
	}
	/* For all remember set objects. */
	obj = env->vm->gc.remember_set;
	while (obj != NULL) {
		/*
		 * Save the next link first: the copy call below may
		 * replace "o" with the newest structural generation,
		 * whose rem_next is not a valid list link.
		 */
		struct rt_gc_object *next = obj->rem_next;
		struct rt_gc_object *o = obj;
		if (!rt_gc_copy_young_object(env, &o))
			return;
		obj = next;
	}

	/*
	 * Update references from the remember set.
	 */

	/*
	 * For each remember set object, update the addresses of the
	 * inner elements using the forwarding pointer technique.
	 *
	 * An object that has a newer structural generation is
	 * skipped: its own slots are stale (the live data is in the
	 * newest generation), so they must not be dereferenced.
	 */
	obj = env->vm->gc.remember_set;
	while (obj != NULL) {
		if (obj->type == RT_GC_TYPE_ARRAY) {
			struct rt_array *arr = (struct rt_array *)obj;
			if (arr->newer != NULL) {
				obj = obj->rem_next;
				continue;
			}
			for (i = 0; i < arr->size; i++) {
				if (IS_REF_VAL(&arr->table[i]) &&
				    IS_YOUNG_OBJ(arr->table[i].val.obj) &&
				    arr->table[i].val.obj->forward != NULL) {
					arr->table[i].val.obj = arr->table[i].val.obj->forward;
				}
			}
		} else  {
			struct rt_dict *dict = (struct rt_dict *)obj;
			if (dict->newer != NULL) {
				obj = obj->rem_next;
				continue;
			}
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

	/*
	 * For each remember set object, remove from the remember set
	 * if the object doesn't have a cross generation reference.
	 */
	obj = env->vm->gc.remember_set;
	while (obj != NULL) {
		struct rt_gc_object *next;
		bool has_cross_gen_ref;
		bool has_newer;

		/* Save the next link first: UNLINK clears obj->rem_next. */
		next = obj->rem_next;

		has_cross_gen_ref = false;
		has_newer = false;
		if (obj->type == RT_GC_TYPE_ARRAY) {
			struct rt_array *arr = (struct rt_array *)obj;
			if (arr->newer != NULL) {
				/* Stale generation; its slots must not be scanned. */
				has_newer = true;
			} else {
				for (i = 0; i < arr->size; i++) {
					if (IS_REF_VAL(&arr->table[i]) &&
					    IS_YOUNG_OBJ(arr->table[i].val.obj)) {
						has_cross_gen_ref = true;
						break;
					}
				}
			}
		} else {
			struct rt_dict *dict = (struct rt_dict *)obj;
			if (dict->newer != NULL) {
				/* Stale generation; its slots must not be scanned. */
				has_newer = true;
			} else {
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
		}
		if (has_newer || !has_cross_gen_ref) {
			/* Unlink from the remember set list. */
			obj->rem_flg = false;
			UNLINK_FROM_LIST(obj, env->vm->gc.remember_set, rem_prev, rem_next);
		}
		obj = next;
	}

	/*
	 * Make finalize table.
	 */

	if (finalize_size > 0) {
		finalize_table = noct_malloc(sizeof(struct finalize_table) * finalize_size);
		if (finalize_table == NULL)
			return;

		/* For all nursery objects. */
		obj = env->vm->gc.nursery_list;
		while (obj != NULL) {
			if (!obj->is_marked && obj->type == RT_GC_TYPE_DICT &&
			    ((struct rt_dict *)obj)->native_finalizer != NULL) {
				struct rt_dict *dict = (struct rt_dict *)obj;
				finalize_table[finalize_count].native_pointer = dict->native_pointer;
				finalize_table[finalize_count].native_finalizer = dict->native_finalizer;
				dict->native_pointer = NULL;
				dict->native_finalizer = NULL;
				finalize_count++;
			} else if (!obj->is_marked &&
				   obj->type == RT_GC_TYPE_PACKED &&
				   ((struct rt_packed *)obj)->native_finalizer != NULL) {
				struct rt_packed *packed = (struct rt_packed *)obj;
				finalize_table[finalize_count].native_pointer = packed->native_pointer;
				finalize_table[finalize_count].native_finalizer = packed->native_finalizer;
				packed->native_pointer = NULL;
				packed->native_finalizer = NULL;
				packed->packed_buffer = NULL;
				packed->elem_size = 0;
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

	noct_free(finalize_table);
}

/* Push a slot onto the traversal worklist. */
static bool
rt_gc_work_push(
	struct rt_env *env,
	struct rt_gc_object **slot)
{
	struct rt_gc_info *gc;

	gc = &env->vm->gc;

	if (gc->work_top == gc->work_cap) {
		size_t cap = gc->work_cap == 0 ? 1024 : gc->work_cap * 2;
		struct rt_gc_object ***w =
			noct_realloc(gc->work, cap * sizeof(*w));
		if (w == NULL) {
			rt_out_of_memory(env);
			return false;
		}
		gc->work = w;
		gc->work_cap = cap;
	}

	gc->work[gc->work_top++] = slot;

	return true;
}

/* Park a freshly promoted object for the after-drain scan. */
static bool
rt_gc_promoted_push(
	struct rt_env *env,
	struct rt_gc_object *obj)
{
	struct rt_gc_info *gc = &env->vm->gc;

	if (gc->promoted_top == gc->promoted_cap) {
		size_t cap = gc->promoted_cap == 0 ? 256 : gc->promoted_cap * 2;
		struct rt_gc_object **p =
			noct_realloc(gc->promoted, cap * sizeof(*p));
		if (p == NULL) {
			rt_out_of_memory(env);
			return false;
		}
		gc->promoted = p;
		gc->promoted_cap = cap;
	}

	gc->promoted[gc->promoted_top++] = obj;

	return true;
}

/*
 * A promoted object now lives in the tenure region. If it still
 * points at young data, the next young collection has to know, so it
 * goes on the remember set. Runs after the worklist drains, when
 * every child slot holds its final address. The same post-order the
 * recursive walk provided by doing this on the way out.
 */
static void
rt_gc_note_promoted_cross_refs(
	struct rt_env *env,
	struct rt_gc_object *obj)
{
	struct rt_array *arr;
	struct rt_dict *dict;
	uint32_t i;

	if (obj->type == RT_GC_TYPE_ARRAY) {
		/* Check for array cross-generation references. */
		arr = (struct rt_array *)obj;
		if (arr->head.rem_flg)
			return;
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
	} else if (obj->type == RT_GC_TYPE_DICT) {
		/* Check for dictionary cross-generation references. */
		dict = (struct rt_dict *)obj;
		if (dict->head.rem_flg)
			return;
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

/*
 * Marks-and-copies the object graph reachable from one root slot.
 * This used to be a massively-recursive function, such as 500 levels.
 */
static bool
rt_gc_copy_young_object(
	struct rt_env *env,
	struct rt_gc_object **root)
{
	struct rt_gc_info *gc = &env->vm->gc;
	struct rt_gc_object *new_obj;
	struct rt_gc_object **obj;
	struct rt_array *arr;
	struct rt_dict *dict;
	uint32_t i;

	if (!rt_gc_work_push(env, root))
		goto fail;

	while (gc->work_top > 0) {
		obj = gc->work[--gc->work_top];

		/* If this is an array or dictionary, get the forwarder. */
		rt_gc_array_dict_follow_newer(env, obj);
		/* A queued slot in an obsolete resized container may be cleared. */
		if (*obj == NULL)
			continue;

		/* If already processed. */
		if ((*obj)->is_marked) {
			/* And if this is a young object. */
			if (IS_YOUNG_OBJ(*obj) && (*obj)->forward != NULL) {
				/* Rewrite the reference. */
				*obj = (*obj)->forward;
			}

			/* Skip. */
			continue;
		}

		/* Copy or promote. */
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
				if (new_obj == NULL)
					goto fail;	/* out of memory */

				/* Set the forwarding pointer. */
				(*obj)->forward = new_obj;

				/* Increment the promotion count. */
				new_obj->promotion_count = (*obj)->promotion_count + 1;
			} else {
				/* Promote the object. */
				new_obj = rt_gc_promote_object(env, *obj);
				if (new_obj != NULL) {
					/* Parked for the after-drain scan. */
					if (!rt_gc_promoted_push(env, new_obj))
						goto fail;

					/* The forwarding pointer is set in rt_gc_promote_object(). */
				} else {
					/*
					 * The tenure region is full, and collecting it from in
					 * here is what the allocator now refuses to do (a nested
					 * collection wrecks this one's own bookkeeping). Demote to an
					 * ordinary graduate copy, the promotion count is carried
					 * over unchanged, so the next collection simply tries
					 * again.
					 */
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
					if (new_obj == NULL)
						goto fail;	/* genuinely out of memory */

					/* Set the forwarding pointer. */
					(*obj)->forward = new_obj;
					new_obj->promotion_count = (*obj)->promotion_count;
				}
			}

			/* Mark the old object processed. */
			(*obj)->is_marked = true;

			/* Rewrite the reference. */
			*obj = new_obj;
		}

		/*
		 * Mark the object the walk descends into: the
		 * graduate copy, the promoted copy, or a tenure
		 * object reached in place. A second visit in this
		 * cycle (a remember-set scan after the root scan, or
		 * another slot pointing here) must not evacuate or
		 * descend again. For the graduate copy that would
		 * fork two diverging live copies, and a tenure object
		 * left unmarked was re-walked on every visit, which
		 * never terminated when tenure objects formed a
		 * cycle.
		 */
		(*obj)->is_marked = true;

		/* Queue the inner slots. */
		switch ((*obj)->type) {
		case RT_GC_TYPE_STRING:
		case RT_GC_TYPE_PACKED:
			/* No inner values. */
			break;
		case RT_GC_TYPE_ARRAY:
			arr = (struct rt_array *)*obj;
			for (i = 0; i < arr->size; i++) {
				if (IS_REF_VAL(&arr->table[i])) {
					if (!rt_gc_work_push(env, &arr->table[i].val.obj))
						goto fail;
				}
			}
			break;
		case RT_GC_TYPE_DICT:
			dict = (struct rt_dict *)*obj;
			for (i = 0; i < dict->alloc_size; i++) {
				if (dict->key[i].type != NOCT_VALUE_STRING)
					continue; /* Removed or empty. */
				if (!rt_gc_work_push(env, &dict->key[i].val.obj))
					goto fail;
				if (IS_REF_VAL(&dict->value[i])) {
					if (!rt_gc_work_push(env, &dict->value[i].val.obj))
						goto fail;
				}
			}
			break;
		default:
			assert(NEVER_COME_HERE);
			break;
		}
	}

	/* When promoted, check for cross-generation references. */
	for (i = 0; i < gc->promoted_top; i++)
		rt_gc_note_promoted_cross_refs(env, gc->promoted[i]);
	gc->promoted_top = 0;

	return true;

fail:
	/* Leave nothing behind for the next collection to trip on. */
	gc->work_top = 0;
	gc->promoted_top = 0;
	return false;
}

/* If this is an array or dictionary, get the forwarder. */
static void
rt_gc_array_dict_follow_newer(
	struct rt_env *env,
	struct rt_gc_object **obj)
{
	UNUSED_PARAMETER(env);
	if (obj == NULL || *obj == NULL)
		return;

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

#if defined(NOCT_USE_MULTITHREAD)
	/*
	 * Keep the ownership and synchronization state across the
	 * move. A mutator parked at this GC safepoint inside
	 * expand_array()/ expand_dict() still holds the write lock,
	 * the moved storage must stay locked for it.
	 */
	new_arr->shared = old_arr->shared;
	new_arr->creator = old_arr->creator;
	new_arr->write_lock = old_arr->write_lock;
	new_arr->seqlock = old_arr->seqlock;
#endif

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
	old_dict->native_pointer = NULL;
	old_dict->native_finalizer = NULL;
	new_dict->is_frozen = old_dict->is_frozen;

#if defined(NOCT_USE_MULTITHREAD)
	/*
	 * Keep the ownership and synchronization state across the
	 * move. A mutator parked at this GC safepoint inside
	 * expand_array()/ expand_dict() still holds the write lock,
	 * the moved storage must stay locked for it.
	 */
	new_dict->shared = old_dict->shared;
	new_dict->creator = old_dict->creator;
	new_dict->write_lock = old_dict->write_lock;
	new_dict->seqlock = old_dict->seqlock;
#endif

	return &new_dict->head;
}

/* Promotes a packed object. */
static struct rt_gc_object *
rt_gc_promote_packed(
	struct rt_env *env,
	struct rt_gc_object *obj)
{
	struct rt_packed *old_packed, *new_packed;

	/* Allocate a packed object. */
	old_packed = (struct rt_packed *)obj;
	new_packed = rt_gc_alloc_packed_tenure(env,
					       old_packed->type,
					       old_packed->size,
					       old_packed->elem_size,
					       (old_packed->size == 0) ? old_packed->packed_buffer : NULL,
					       old_packed->native_pointer,
					       old_packed->native_finalizer);
	if (new_packed == NULL)
		return false;

	if (old_packed->size != 0)
		memcpy(new_packed->packed_buffer, old_packed->packed_buffer, old_packed->size);
	new_packed->packed_typed = old_packed->packed_typed;
	old_packed->native_pointer = NULL;
	old_packed->native_finalizer = NULL;

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
	 * Strings larger than noct_conf_gc_lop_threshold must not be
	 * in the nursery or graduate regions.
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

#if defined(NOCT_USE_MULTITHREAD)
	/*
	 * Keep the ownership and synchronization state across the
	 * move. A mutator parked at this GC safepoint inside
	 * expand_array()/ expand_dict() still holds the write lock,
	 * the moved storage must stay locked for it.
	 */
	new_obj->shared = old_obj->shared;
	new_obj->creator = old_obj->creator;
	new_obj->write_lock = old_obj->write_lock;
	new_obj->seqlock = old_obj->seqlock;
#endif

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

	/*
	 * Check for cross-generation references. The key table is an
	 * open-addressing hash table: iterate the slots, not the
	 * entry count, and skip the empty ones.
	 */
	if (new_obj->head.region == RT_GC_REGION_TENURE) {
		for (i = 0; i < (uint32_t)new_obj->alloc_size; i++) {
			if (new_obj->key[i].type != NOCT_VALUE_STRING)
				continue;
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
	old_obj->native_pointer = NULL;
	old_obj->native_finalizer = NULL;
	new_obj->is_frozen = old_obj->is_frozen;

#if defined(NOCT_USE_MULTITHREAD)
	/*
	 * Keep the ownership and synchronization state across the
	 * move. A mutator parked at this GC safepoint inside
	 * expand_array()/ expand_dict() still holds the write lock,
	 * the moved storage must stay locked for it.
	 */
	new_obj->shared = old_obj->shared;
	new_obj->creator = old_obj->creator;
	new_obj->write_lock = old_obj->write_lock;
	new_obj->seqlock = old_obj->seqlock;
#endif

	/* Succeeded. */
	return &new_obj->head;
}

/* Copies a packed object to the graduate region. */
static struct rt_gc_object *
rt_gc_copy_packed_to_graduate(
	struct rt_env *env,
	struct rt_packed *old_obj)
{
	struct rt_packed *new_obj;

	/*
	 * Packed larger than noct_conf_gc_lop_threshold must not be
	 * in the nursery or graduate regions.
	 */
	assert(old_obj->size < env->vm->config.gc_lop_threshold);

	/* Allocate in the graduate region. */
	new_obj = rt_gc_alloc_packed_graduate(env,
					      old_obj->type,
					      old_obj->size,
					      old_obj->elem_size,
					      (old_obj->size == 0) ? old_obj->packed_buffer : NULL,
					      old_obj->native_pointer,
					      old_obj->native_finalizer);
	if (new_obj == NULL)
		return NULL;

	/* If not a preallocated. (that means packed_buffer is not managed by GC)  */
	if (old_obj->size != 0)
		memcpy(new_obj->packed_buffer, old_obj->packed_buffer, old_obj->size);
	new_obj->packed_typed = old_obj->packed_typed;
	old_obj->native_pointer = NULL;
	old_obj->native_finalizer = NULL;

	/* Succeeded. */
	return &new_obj->head;
}

/* Executes an old GC in the multithreaded manner. */
static void
rt_gc_old_gc(
	struct rt_env *env)
{
	if (!om_enter_gc(env, RT_GC_LEVEL_1))
		return;
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
		if (IS_REF_VAL(&env->vm->global[i].val)) {
			if (!rt_gc_mark_old_object(env, &env->vm->global[i].val.val.obj))
				return;
		}
	}

	/* For all call frames of all thread envs. */
	{
		struct rt_env *e;
		for (e = env->vm->env_list; e != NULL; e = e->next) {
			for (sp = e->cur_frame_index; sp >= 0; sp--) {
				frame = &e->frame_alloc[sp];

				/* For all temporary variables in the frame. */
				for (i = 0; i < frame->tmpvar_size; i++) {
					if (IS_REF_VAL(&frame->tmpvar[i])) {
						if (!rt_gc_mark_old_object(env, &frame->tmpvar[i].val.obj))
							return;
					}
				}

				/* For all pinned C local variables in the frame. */
				for (i = 0; i < frame->pinned_count; i++) {
					if (IS_REF_VAL(frame->pinned[i])) {
						if (!rt_gc_mark_old_object(env, &frame->pinned[i]->val.obj))
							return;
					}
				}
			}
		}
	}

	/* For all pinned C global variables. */
	for (i = 0; i < env->vm->pinned_count; i++) {
		if (IS_REF_VAL(env->vm->pinned[i])) {
			if (!rt_gc_mark_old_object(env, &env->vm->pinned[i]->val.obj))
				return;
		}
	}
	/*
	 * Sweep.
	 *
	 * Reached only when the mark completed: an aborted mark (the
	 * worklist could not grow) returns above instead, because
	 * sweeping an incompletely marked heap frees live objects.
	 * Skipping a whole old-GC cycle merely delays reclamation.
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

/*
 * Marks the object graph reachable from one root slot, for the old GC.
 *
 * Iterative for the same reason as rt_gc_copy_young_object: the graph
 * is as deep as the program's data, and a recursive walk overflowed
 * the C stack on deep cons chains. Returns false only when the
 * worklist cannot grow; the caller must then skip the sweep, because
 * an incompletely marked heap looks like garbage.
 */
static bool
rt_gc_mark_old_object(
	struct rt_env *env,
	struct rt_gc_object **obj)
{
	struct rt_gc_info *gc = &env->vm->gc;
	uint32_t i;

	if (!rt_gc_work_push(env, obj))
		goto fail;

	while (gc->work_top > 0) {
		obj = gc->work[--gc->work_top];

		/* Follow the newer array/dict. */
		rt_gc_array_dict_follow_newer(env, obj);
		/* A queued slot in an obsolete resized container may be cleared. */
		if (*obj == NULL)
			continue;

		/* If already marked, skip. */
		if ((*obj)->is_marked)
			continue;

		/* Mark. */
		(*obj)->is_marked = true;

		/* Queue the inner slots. */
		if ((*obj)->type == RT_GC_TYPE_ARRAY) {
			struct rt_array *arr = (struct rt_array *)*obj;
			for (i = 0; i < arr->size; i++) {
				if (IS_REF_VAL(&arr->table[i])) {
					if (!rt_gc_work_push(env, &arr->table[i].val.obj))
						goto fail;
				}
			}
		} else if ((*obj)->type == RT_GC_TYPE_DICT) {
			struct rt_dict *dict = (struct rt_dict *)*obj;
			for (i = 0; i < dict->alloc_size; i++) {
				if (dict->key[i].type != NOCT_VALUE_STRING)
					continue; /* Removed or empty. */

				if (!rt_gc_work_push(env, &dict->key[i].val.obj))
					goto fail;
				if (IS_REF_VAL(&dict->value[i])) {
					if (!rt_gc_work_push(env, &dict->value[i].val.obj))
						goto fail;
				}
			}
		}
	}

	return true;

fail:
	gc->work_top = 0;
	return false;
}

/* Finalize a native owner, detaching it before invoking foreign code. */
static void
rt_gc_finalize_object(
	struct rt_gc_object *obj)
{
	void *native_pointer = NULL;
	void (*native_finalizer)(void *native_pointer) = NULL;

	if (obj->type == RT_GC_TYPE_DICT) {
		struct rt_dict *dict = (struct rt_dict *)obj;
		native_pointer = dict->native_pointer;
		native_finalizer = dict->native_finalizer;
		dict->native_pointer = NULL;
		dict->native_finalizer = NULL;
	} else if (obj->type == RT_GC_TYPE_PACKED) {
		struct rt_packed *packed = (struct rt_packed *)obj;
		native_pointer = packed->native_pointer;
		native_finalizer = packed->native_finalizer;
		packed->native_pointer = NULL;
		packed->native_finalizer = NULL;
		if (native_finalizer != NULL) {
			packed->packed_buffer = NULL;
			packed->elem_size = 0;
		}
	}

	if (native_finalizer != NULL)
		native_finalizer(native_pointer);
}

/* Free a string, array, dictionary, or packed object. */
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

	/* Native resources are not part of the tenure allocation. */
	rt_gc_finalize_object(obj);

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

/* Compaction: remap an old tenure object address to its new address. */
static struct rt_gc_object *
rt_gc_compact_remap(
	struct rt_env *env,
	struct rt_gc_object *obj)
{
	uint32_t lo, hi;

	/*
	 * Binary search in the compact_before table.
	 * The table is built in ascending address order.
	 */
	lo = 0;
	hi = (uint32_t)env->vm->gc.compact_count;
	while (lo < hi) {
		uint32_t mid = lo + (hi - lo) / 2;
		if ((char *)env->vm->gc.compact_before[mid] < (char *)obj)
			lo = mid + 1;
		else
			hi = mid;
	}
	if (lo < (uint32_t)env->vm->gc.compact_count &&
	    env->vm->gc.compact_before[lo] == (void *)obj)
		return (struct rt_gc_object *)env->vm->gc.compact_after[lo];
	return obj;
}

/* Compaction: remap a value slot if it references a moving object. */
static void
rt_gc_compact_update_value(
	struct rt_env *env,
	struct rt_value *v)
{
	if (IS_REF_VAL(v))
		v->val.obj = rt_gc_compact_remap(env, v->val.obj);
}

/*
 * Compaction: update all outgoing pointers stored in an object.
 *
 * Must be called while every object still resides at its old address,
 * so that old addresses are unambiguous. Each pointer slot is visited
 * exactly once, which makes the remapping alias-safe even though the
 * new address range overlaps the old one.
 */
static void
rt_gc_compact_update_object(
	struct rt_env *env,
	struct rt_gc_object *obj)
{
	uint32_t i;

	/* GC list links. */
	if (obj->prev != NULL)
		obj->prev = rt_gc_compact_remap(env, obj->prev);
	if (obj->next != NULL)
		obj->next = rt_gc_compact_remap(env, obj->next);
	if (obj->rem_prev != NULL)
		obj->rem_prev = rt_gc_compact_remap(env, obj->rem_prev);
	if (obj->rem_next != NULL)
		obj->rem_next = rt_gc_compact_remap(env, obj->rem_next);

	/* Content slots. */
	if (obj->type == RT_GC_TYPE_ARRAY) {
		struct rt_array *arr = (struct rt_array *)obj;
		for (i = 0; i < arr->size; i++)
			rt_gc_compact_update_value(env, &arr->table[i]);
		if (arr->newer != NULL)
			arr->newer = (struct rt_array *)rt_gc_compact_remap(env, (struct rt_gc_object *)arr->newer);
	} else if (obj->type == RT_GC_TYPE_DICT) {
		struct rt_dict *dict = (struct rt_dict *)obj;
		for (i = 0; i < dict->alloc_size; i++) {
			if (dict->key[i].type == NOCT_VALUE_STRING)
				dict->key[i].val.obj = rt_gc_compact_remap(env, dict->key[i].val.obj);
			rt_gc_compact_update_value(env, &dict->value[i]);
		}
		if (dict->newer != NULL)
			dict->newer = (struct rt_dict *)rt_gc_compact_remap(env, (struct rt_gc_object *)dict->newer);
	}
}

/* Executes a compaction GC. */
static bool
rt_gc_compact_gc_body(
	struct rt_env *env)
{
	struct rt_gc_object *obj;
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
		env->vm->gc.compact_before = NULL;
		rt_out_of_memory(env);
		return false;
	}

	/* Traverse the tenure region in ascending address order. */
	cur_blk = env->vm->gc.tenure_freelist.top;
	remap_top = env->vm->gc.tenure_freelist.top;
	index = 0;
	while (cur_blk < env->vm->gc.tenure_freelist.end) {
		size_t blk_size;
		bool blk_used;
		struct rt_gc_object *blk_obj;

		blk_size = *(size_t *)cur_blk;
		blk_used = blk_size & RT_GC_FREELIST_USED_BIT ? true : false;
		blk_size &= RT_GC_FREELIST_SIZE_MASK;
		blk_obj = (struct rt_gc_object *)(cur_blk + sizeof(size_t));

		/* Check for the end of the list. */
		if (blk_size == 0)
			break;

		/* Skip if unused. */
		if (!blk_used) {
			cur_blk += sizeof(size_t) + blk_size;
			continue;
		}

		/* Record. The new block sizes stay aligned. */
		env->vm->gc.compact_before[index] = blk_obj;
		env->vm->gc.compact_after[index] = remap_top + sizeof(size_t);
		remap_top += sizeof(size_t) +
			((blk_obj->size + RT_GC_FREELIST_ALIGN - 1) & ~(RT_GC_FREELIST_ALIGN - 1));
		index++;

		cur_blk += sizeof(size_t) + blk_size;
	}
	assert(index == env->vm->gc.compact_count);

	/*
	 * Phase 1: Rewrite all references while every object is still at
	 * its old address.
	 */

	/* For all objects in all regions, update outgoing pointers. */
	obj = env->vm->gc.nursery_list;
	while (obj != NULL) {
		struct rt_gc_object *next = obj->next;
		rt_gc_compact_update_object(env, obj);
		obj = next;
	}
	obj = env->vm->gc.graduate_list;
	while (obj != NULL) {
		struct rt_gc_object *next = obj->next;
		rt_gc_compact_update_object(env, obj);
		obj = next;
	}
	obj = env->vm->gc.tenure_list;
	while (obj != NULL) {
		struct rt_gc_object *next = obj->next;
		rt_gc_compact_update_object(env, obj);
		obj = next;
	}

	/*
	 * Note: the iterations above read each object's next link before
	 * updating it, so the traversal itself always follows the old,
	 * still-valid addresses.
	 */

	/* List heads. */
	if (env->vm->gc.tenure_list != NULL)
		env->vm->gc.tenure_list = rt_gc_compact_remap(env, env->vm->gc.tenure_list);
	if (env->vm->gc.remember_set != NULL)
		env->vm->gc.remember_set = rt_gc_compact_remap(env, env->vm->gc.remember_set);

	/* For all global variables. */
	for (i = 0; i < (uint32_t)env->vm->global_alloc_size; i++) {
		if (env->vm->global[i].name == NULL || env->vm->global[i].is_removed)
			continue;
		rt_gc_compact_update_value(env, &env->vm->global[i].val);
	}

	/* For all call frames of all thread envs. */
	{
		struct rt_env *e;
		for (e = env->vm->env_list; e != NULL; e = e->next) {
			for (sp = e->cur_frame_index; sp >= 0; sp--) {
				frame = &e->frame_alloc[sp];

				/* For all temporary variables in the frame. */
				for (i = 0; i < frame->tmpvar_size; i++)
					rt_gc_compact_update_value(env, &frame->tmpvar[i]);

				/* For all pinned C local variables in the frame. */
				for (i = 0; i < frame->pinned_count; i++)
					rt_gc_compact_update_value(env, frame->pinned[i]);
			}
		}
	}

	/* For all pinned C global variables. */
	for (i = 0; i < env->vm->pinned_count; i++)
		rt_gc_compact_update_value(env, env->vm->pinned[i]);
	/*
	 * Phase 2: Slide tenure objects in ascending address order.
	 * Destinations never overlap a not-yet-moved source ahead of the
	 * current one, so the region stays consistent during the slide.
	 */

	for (i = 0; i < env->vm->gc.compact_count; i++) {
		/* Get the real object size. */
		size_t obj_size = ((struct rt_gc_object *)env->vm->gc.compact_before[i])->size;
		size_t blk_size = (obj_size + RT_GC_FREELIST_ALIGN - 1) & ~(RT_GC_FREELIST_ALIGN - 1);

		/* Move. */
		memmove(env->vm->gc.compact_after[i],
			env->vm->gc.compact_before[i],
			obj_size);

		/* Store the size header. (aligned, marked as used) */
		*(size_t *)((char *)env->vm->gc.compact_after[i] - sizeof(size_t)) = blk_size | RT_GC_FREELIST_USED_BIT;

		/* Fill the reminder. */
		if (i == env->vm->gc.compact_count - 1) {
			memset((char *)env->vm->gc.compact_after[i] + blk_size,
			       0,
			       (size_t)(env->vm->gc.tenure_freelist.end - ((char *)env->vm->gc.compact_after[i] + blk_size)));
		}
	}

	/*
	 * Phase 3: Fix the interior self-pointers of the moved objects.
	 */

	for (i = 0; i < env->vm->gc.compact_count; i++) {
		obj = (struct rt_gc_object *)env->vm->gc.compact_after[i];
		if (obj->type == RT_GC_TYPE_STRING) {
			struct rt_string *str = (struct rt_string *)obj;
			str->data = (char *)str + sizeof(struct rt_string);
		} else if (obj->type == RT_GC_TYPE_ARRAY) {
			struct rt_array *arr = (struct rt_array *)obj;
			arr->table = (struct rt_value *)((char *)arr + sizeof(struct rt_array));
		} else if (obj->type == RT_GC_TYPE_DICT) {
			struct rt_dict *dict = (struct rt_dict *)obj;
			dict->key = (struct rt_value *)((char *)dict + sizeof(struct rt_dict));
			dict->value = (struct rt_value *)((char *)dict + sizeof(struct rt_dict) + dict->alloc_size * sizeof(struct rt_value));
		} else if (obj->type == RT_GC_TYPE_PACKED) {
			struct rt_packed *packed = (struct rt_packed *)obj;
			/* Move the buffer pointer if not a preallocated buffer. */
			if (packed->size != 0)
				packed->packed_buffer = ((char *)packed + sizeof(struct rt_packed));
		}
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
	env->vm->gc.compact_count = 0;

	return true;
}

/* Executes a compaction GC with a stop-the-world section. */
static bool
rt_gc_compact_gc(
	struct rt_env *env)
{
	bool ret;

	if (!om_enter_gc(env, RT_GC_LEVEL_2))
		return true;
	ret = rt_gc_compact_gc_body(env);

	/*
	 * Compaction rewrote the block layout: every free bin entry and
	 * the frontier now point at moved memory. One walk over the
	 * compacted heap rebuilds both.
	 */
	rt_gc_tenure_rebuild_bins(env);

	om_leave_gc(env);

	return ret;
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

	/*
	 * Zero-clear the slot so it holds a GC-safe integer 0 until the
	 * caller fills in the real value.  Pinning makes this slot a GC
	 * root immediately, so an uninitialized NoctValue whose garbage
	 * type byte happens to pass IS_REF_VAL would be scanned as a wild
	 * pointer (heap corruption).  The intrinsic convention is
	 * pin-first, fill-after, so clearing here is always safe.
	 */
	val->type = NOCT_VALUE_INT;
	val->val.l = 0;

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

/* The size class of a tenure block. */
static int
rt_gc_tenure_bin(size_t size)
{
	int bin = 0;

	/* Bin 0 holds sizes below 16; bin b holds [2^(b+3), 2^(b+4)). */
	size >>= 4;
	while (size != 0 && bin < RT_GC_TENURE_BIN_COUNT - 1) {
		size >>= 1;
		bin++;
	}
	return bin;
}

/*
 * Re-derives the frontier and the free bins from the header chain.
 * Called after compaction, which moves every live block and thereby
 * invalidates both. One walk over the compacted heap restores them
 * and, when the heap held only free blocks (nothing to compact), puts
 * those back into circulation too.
 */
static void
rt_gc_tenure_rebuild_bins(
	struct rt_env *env)
{
	struct rt_gc_info *gc = &env->vm->gc;
	char *cur;
	int i;

	for (i = 0; i < RT_GC_TENURE_BIN_COUNT; i++)
		gc->tenure_bins[i] = NULL;

	cur = gc->tenure_freelist.top;
	while (*(size_t *)cur) {
		size_t blk_size = *(size_t *)cur;
		bool is_used = (blk_size & RT_GC_FREELIST_USED_BIT) != 0;

		blk_size &= RT_GC_FREELIST_SIZE_MASK;
		if (!is_used) {
			int b = rt_gc_tenure_bin(blk_size);
			*(char **)(cur + sizeof(size_t)) = gc->tenure_bins[b];
			gc->tenure_bins[b] = cur;
		}
		cur += sizeof(size_t) + blk_size;
		assert(cur < gc->tenure_freelist.end);
	}
	gc->tenure_frontier = cur;
}

/*
 * Allocate a tenure block.
 *
 * The allocator for the tenure region. Each block has its size at the
 * block top. The LSB of the block size indicates the block is used
 * (set) or freed (clear).
 */
static void *
rt_gc_tenure_alloc(
	struct rt_env *env,
	size_t size)
{
	struct rt_gc_info *gc = &env->vm->gc;
	int b = rt_gc_tenure_bin(size);
	int bb;
	int probe;
	char *blk;
	char *prev;
	char *cur;

	assert(size > 0);
	if (size == 0)
		return NULL;

	/* Align. */
	size = (size + RT_GC_FREELIST_ALIGN - 1) & ~(RT_GC_FREELIST_ALIGN - 1);

	/* The request's own bin: first fit, a few probes deep. */
	prev = NULL;
	blk = gc->tenure_bins[b];
	for (probe = 0; blk != NULL && probe < 8; probe++) {
		size_t blk_size = *(size_t *)blk & RT_GC_FREELIST_SIZE_MASK;
		char *next = *(char **)(blk + sizeof(size_t));

		if (blk_size >= size) {
			if (prev == NULL)
				gc->tenure_bins[b] = next;
			else
				*(char **)(prev + sizeof(size_t)) = next;
			*(size_t *)blk = blk_size | RT_GC_FREELIST_USED_BIT;
			return blk + sizeof(size_t);
		}
		prev = blk;
		blk = next;
	}

	/* Higher bins: every block fits, pop the head. */
	for (bb = b + 1; bb < RT_GC_TENURE_BIN_COUNT; bb++) {
		blk = gc->tenure_bins[bb];
		if (blk == NULL)
			continue;
		gc->tenure_bins[bb] = *(char **)(blk + sizeof(size_t));
		*(size_t *)blk |= RT_GC_FREELIST_USED_BIT;
		return blk + sizeof(size_t);
	}

	cur = gc->tenure_frontier;

	/* Check if the remaining size fits. */
	if ((uintptr_t)cur + sizeof(size_t) > (uintptr_t)env->vm->gc.tenure_freelist.end ||
	    size > (size_t)((uintptr_t)env->vm->gc.tenure_freelist.end - (uintptr_t)cur - sizeof(size_t)))
		return NULL;

	/* Allocate at the frontier. */
	*(size_t *)cur = size | RT_GC_FREELIST_USED_BIT;
	env->vm->gc.tenure_frontier = cur + sizeof(size_t) + size;
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
	int b;

	/* Get the header address. */
	header = (size_t *)((char *)p - sizeof(size_t));

	/* Get the block size. */
	size = *header;

	/* Block must be used. (check the used bit.) */
	assert(size & RT_GC_FREELIST_USED_BIT);

	/* Erase the used bit. */
	size &= RT_GC_FREELIST_SIZE_MASK;
	*header = size;

	/*
	 * Push onto the bin of its size class, so the allocator finds
	 * it without a scan. The next pointer lives in the dead body.
	 */
	b = rt_gc_tenure_bin(size);
	*(char **)p = env->vm->gc.tenure_bins[b];
	env->vm->gc.tenure_bins[b] = (char *)header;
}
