/* -*- coding: utf-8; tab-width: 8; indent-tabs-mode: t; -*- */

/*
 * Noct Programming Language
 * Copyright (c) 2025, 2026, Awe Morris
 */

/*
 * GC-Unified Mutable Object Model
 */

#ifndef NOCT_OBJECTMODEL_H
#define NOCT_OBJECTMODEL_H

#include <noct/noct.h>
#include "runtime.h"
#include "gc.h"

/*
 * ===========================================================================
 *  Array
 * ===========================================================================
 */

/*
 * Make an empty array.
 */
bool
om_make_array(
	struct rt_env *env,
	struct rt_value *val);

/*
 * Get the size of an array.
 */
bool
om_get_array_size(
	struct rt_env *env,
	struct rt_value *arr_val,
	size_t *size);

/*
 * Read an array element.
 */
bool
om_read_array(
	struct rt_env *env,
	struct rt_value *arr_val,
	size_t index,
	struct rt_value *val);

/*
 * Write an array element.
 */
bool
om_write_array(
	struct rt_env *env,
	struct rt_value *arr_val,
	size_t index,
	struct rt_value *val);

/*
 * Resize an array.
 */
bool
om_resize_array(
	struct rt_env *env,
	struct rt_value *arr_val,
	size_t size);

/*
 * Make a copy of an array.
 */
bool
om_copy_array(
	struct rt_env *env,
	struct rt_value *dst,
	struct rt_value *src);

/*
 * ===========================================================================
 *  Dictionary
 * ===========================================================================
 */

/*
 * Make an empty array.
 */
bool
om_make_dict(
	struct rt_env *env,
	struct rt_value *val);

/*
 * Get the size of a dictionary.
 */
bool
om_get_dict_size(
	struct rt_env *env,
	struct rt_value *val,
	size_t *size);

/*
 * Get the allocation size of a dictionary.
 */
bool
om_get_dict_alloc_size(
	struct rt_env *env,
	struct rt_value *val,
	size_t *size);

/*
 * Check if a key exists in a dictionary.
 */
bool
om_check_dict_key(
	struct rt_env *env,
	struct rt_value *dict_val,
	struct rt_value *key,
	bool *ret);

/*
 * Read a dictionary element.
 */
bool
om_read_dict(
	struct rt_env *env,
	struct rt_value *dict_val,
	struct rt_value *key,
	struct rt_value *val);

/*
 * Read a dictionary element.
 */
bool
om_read_dict_with_hash(
	struct rt_env *env,
	struct rt_value *dict_val,
	const char *key,
	size_t key_len,
	uint32_t key_hash,
	struct rt_value *val);

/*
 * Read a dictionary element by a slot index. (for traverse)
 */
bool
om_read_dict_index(
	struct rt_env *env,
	struct rt_value *dict_val,
	size_t index,
	struct rt_value *key,
	struct rt_value *val);

/*
 * Write a dictionary element.
 */
bool
om_write_dict(
	struct rt_env *env,
	struct rt_value *dict_val,
	struct rt_value *key,
	struct rt_value *val);

/*
 * Write a dictionary element.
 */
bool
om_write_dict_with_hash(
	struct rt_env *env,
	struct rt_value *dict_val,
	const char *key,
	size_t len,
	uint32_t hash,
	struct rt_value *val);

/*
 * Erase a dictionary element.
 */
bool
om_erase_dict_entry(
	struct rt_env *env,
	struct rt_value *dict_val,
	struct rt_value *key);

/*
 * Make a shallow copy of a dictionary.
 */
bool
om_copy_dict(
	struct rt_env *env,
	struct rt_value *dst,
	struct rt_value *src);

/*
 * Merges a dictionary.
 */
bool
om_merge_dict(
	struct rt_env *env,
	struct rt_value *dst,
	struct rt_value *src1,
	struct rt_value *src2);

/*
 * ===========================================================================
 *  GC
 * ===========================================================================
 */

/*
 * Enter a GC execution.
 */
bool
om_enter_gc(
	struct rt_env *env,
	int gc_level);

/*
 * Leave a GC execution.
 */
void
om_leave_gc(
	struct rt_env *env);

/*
 * ===========================================================================
 *  Safepoint
 * ===========================================================================
 */

/*
 * Make a safepoint.
 */
void
om_safepoint(
	struct rt_env *env);

/*
 * ===========================================================================
 *  Other
 * ===========================================================================
 */

/*
 * Make thread in-flight for thread creation.
 */
void
om_init_env(
	struct rt_env *env);

#endif
