/* -*- coding: utf-8; tab-width: 8; indent-tabs-mode: t; -*- */

/*
 * Copyright (c) 2025, Awe Morris. All rights reserved.
 */

/*
 * Garbage Collector
 *
 * The heap is divided into three regions, with an additional remember set:
 *
 * - Nursery Region:
 *   For newly allocated small objects.
 *   Managed in a dedicated 2MB memory region.
 *   Collected via copying GC along with the graduate generation.
 *
 * - Graduate Region:
 *   For surviving nursery objects.
 *   Uses semi-space copying GC with two 256KB regions (from and to spaces).
 *
 * - Tenure Region:
 *   For long-lived, or large objects. Collected using Mark-Sweep GC,
 *   and compacted using Slide Compaction.
 *
 * - Remember Set:
 *   Tracks tenure-region arrays or dictionaries that reference
 *   young-generation objects (in the nursery or graduate region).
 *   Maintained via a write barrier triggered on container updates.
 *   Ensures proper GC reachability across regions.
 */

#ifndef NOCT_GC_H
#define NOCT_GC_H

#include <noct/noct.h>
#include "arena.h"

/*
 * Region Size
 */
#if !defined(NOCT_TARGET_WASM)
#define RT_GC_NURSERY_SIZE		(2 * 1024 * 1024)
#define RT_GC_GRADUATE_SIZE		(256 * 1024)
#define RT_GC_TENURE_SIZE		(256 * 1024 * 1024)
#else
#define RT_GC_NURSERY_SIZE		(2 * 1024 * 1024)
#define RT_GC_GRADUATE_SIZE		(256 * 1024)
#define RT_GC_TENURE_SIZE		(64 * 1024 * 1024)
#endif

/*
 * Large Object Promotion Threshold - A new object that has a size
 * beyond this value will be allocated directly in the tenure region.
 */
#define RT_GC_LOP_THRESHOLD		(32 * 1024)

/*
 * Survivor Promotion Threshold - A graduate object that survived the
 * young GC beyond this value will be promoted to the tenure region.
 */
#define RT_GC_PROMOTION_THRESHOLD	(2)

/*
 * Regions.
 */
enum gt_gc_generation {
	RT_GC_REGION_NURSERY,
	RT_GC_REGION_GRADUATE,
	RT_GC_REGION_TENURE,
};

/*
 * Object Types.
 */
enum rt_gc_object_type {
	RT_GC_TYPE_STRING,
	RT_GC_TYPE_ARRAY,
	RT_GC_TYPE_DICT,
	RT_GC_TYPE_FUNC,
};

/*
 * Free List Constants.
 */
#define RT_GC_FREELIST_ALIGN		(sizeof(void *))
#define RT_GC_FREELIST_USED_BIT		0x1
#define RT_GC_FREELIST_SIZE_MASK	(~(size_t)1)

/*
 * Garbage Collector state structure that is embedded to struct rt_vm.
 */
struct rt_gc_info {
	/* Memory region for the nursery generation (2MB). */
	struct arena_info nursery_arena;

	/*
	 * Memory regions for the graduate generation:
	 * [0] = from-space, [1] = to-space (256KB each).
	 */
	struct arena_info graduate_arena[2];
	int cur_grad_from;
	int cur_grad_to;

	/* Memory region for the tenure generation. */
	struct freelist {
		char *top;
		char *end;
	} tenure_freelist;

	/* Linked list of objects in the nursery generation. */
	struct rt_gc_object *nursery_list;

	/* Linked list of objects in the graduate generation. */
	struct rt_gc_object *graduate_list;
	struct rt_gc_object *graduate_new_list;

	/* Linked list of objects in the tenure generation. */
	struct rt_gc_object *tenure_list;

	/* Linked list of the remember set. */
	struct rt_gc_object *remember_set;

	/* Compaction address tables. */
	int compact_count;
	void **compact_before;
	void **compact_after;
};

/*
 * GC-managed object header for strings, arrays, and dictionaries.
 * This struct is embedded at the beginning of each managed object.
 */
struct rt_gc_object {
	/*
	 * Object type: one of RT_GC_TYPE_STRING, RT_GC_TYPE_ARRAY, or
	 * RT_GC_TYPE_DICT.
	 */
	int type;

	/*
	 * Region tag: indicates which GC generation this object
	 * belongs to.
	 */
	int region;

	/* Object size. (for compaction) */
	size_t size;

	/*
	 * Intrusive doubly-linked list for the corresponding
	 * region's list (nursery, graduate, or tenure).
	 */
	struct rt_gc_object *prev;
	struct rt_gc_object *next;

	/*
	 * Intrusive doubly-linked list for the remember set.
	 */
	struct rt_gc_object *rem_prev;
	struct rt_gc_object *rem_next; 
	bool rem_flg;

	/* Mark bit used in mark-and-sweep GC for the tenure generation. */
	bool is_marked;

	/* Promotion count. */
	int promotion_count;

	/* Forwarding pointer. */
	struct rt_gc_object *forward;
};

/*
 * The following functions are part of the GC interface used by runtime.c.
 */

/* Initializes the garbage collector and allocate memory regions. */
bool rt_gc_init(struct rt_vm *vm);

/* Cleanups the garbage collector and deallocate memory regions. */
void rt_gc_cleanup(struct rt_vm *vm);

/* Allocates a string object in the appropriate region. */
struct rt_string *rt_gc_alloc_string(struct rt_env *env, const char *data, size_t len, uint32_t hash);

/* Allocates an array object in the appropriate region. */
struct rt_array *rt_gc_alloc_array(struct rt_env *env, size_t size);

/* Allocates a dictionary object in the appropriate region. */
struct rt_dict *rt_gc_alloc_dict(struct rt_env *env, size_t size);

/* Write barrier: registers a container in the remember set if it references a young object. */
void rt_gc_array_write_barrier(struct rt_env *env, struct rt_array *arr, int index, struct rt_value *val);

/* Write barrier: registers a container in the remember set if it references a young object. */
void rt_gc_dict_write_barrier(struct rt_env *env, struct rt_dict *dict, struct rt_value *val);

/* Pins a C global variable. */
bool rt_gc_pin_global(struct rt_env *env, struct rt_value *val);

/* Unpins a C global variable. */
bool rt_gc_unpin_global(struct rt_env *env, struct rt_value *val);

/* Pins a C local variable. */
bool rt_gc_pin_local(struct rt_env *env, struct rt_value *val);

/* Unpins a C local variable. */
bool rt_gc_unpin_local(struct rt_env *env, struct rt_value *val);

/* Retrieves the approximate memory usage, in bytes. */
bool rt_gc_get_heap_usage(struct rt_env *env, size_t *ret);

/* Manually triggers a young GC. (nursery + graduate, copying GC) */
void rt_gc_level1_gc(struct rt_env *env);

/* Manually triggers an old GC. (tenure, mark-and-sweep) */
void rt_gc_level2_gc(struct rt_env *env);

/* Manually triggers a full GC. (tenure, nursery + graduate) */
void rt_gc_level3_gc(struct rt_env *env);

/*
 * Multithread Support
 */

/* Initialize an environment. */
void rt_gc_init_env(struct rt_env *env);

/* Make a GC safe point. */
void rt_gc_safepoint(struct rt_env *env);

#endif
