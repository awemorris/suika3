/* -*- coding: utf-8; tab-width: 8; indent-tabs-mode: t; -*- */

/*
 * Noct Programming Language
 * Copyright (c) 2025, 2026, Awe Morris
 */

/*
 * Object Model (Single Thread Implementation)
 */

#include <noct/noct.h>
#include "objectmodel.h"
#include "runtime.h"
#include "gc.h"
#include "atomic.h"

#include <assert.h>
#include <stdio.h>

/* False Assertion */
#define NO_SPACE_FOR_DICTIONARY		(0)

static INLINE bool expand_array(struct rt_env *env, struct rt_value *arr_val, struct rt_array *arr, size_t size);
static INLINE bool expand_dict(struct rt_env *env, struct rt_value *dict_val, struct rt_dict *dict, size_t size);

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
	struct rt_value *val)
{
	struct rt_array *arr;
	const uint32_t START_SIZE = 16;

	/** Allocate an array. This may occur GC. */
	arr = rt_gc_alloc_array(env, START_SIZE);
	if (arr == NULL) {
		rt_out_of_memory(env);
		return false;
	}

	/* Setup a value. */
	val->type = NOCT_VALUE_ARRAY;
	val->val.arr = arr;

	return true;
}

/*
 * Get the size of an array.
 */
bool
om_get_array_size(
	struct rt_env *env,
	struct rt_value *arr_val,
	size_t *size)
{
	struct rt_array *arr;

	UNUSED_PARAMETER(env);

	/* Track the newer chain. */
	arr = arr_val->val.arr;
	while (arr->newer != NULL)
		arr = arr->newer;

	/* Get the size. */
	*size = arr->size;

	return true;
}

/*
 * Read an array element.
 */
bool
om_read_array(
	struct rt_env *env,
	struct rt_value *arr_val,
	size_t index,
	struct rt_value *val)
{
	struct rt_array *arr;

	UNUSED_PARAMETER(env);

	/* Track the newer chain. */
	arr = arr_val->val.arr;
	while (arr->newer != NULL)
		arr = arr->newer;

	/* Check for the array size. */
	if (index >= arr->size) {
		/* Index is out-of-range. */
		rt_error(env, N_TR("Array index %d is out-of-range."), index);
		return false;
	}

	/* Read. */
	*val = arr->table[index];

	/* Succeeded. */
	return true;
}

/*
 * Write an array element.
 */
bool
om_write_array(
	struct rt_env *env,
	struct rt_value *arr_val,
	size_t index,
	struct rt_value *val)
{
	struct rt_array *arr;
	size_t new_size;

	/* Track the newer chain. */
	arr = arr_val->val.arr;
	while (arr->newer != NULL)
		arr = arr->newer;

	/* In case expand is required. */
	if (index >= arr->alloc_size) {
		/* Array expansion is required. Determine the new size. */
		new_size = arr->alloc_size * 2;
		while (new_size < index)
			new_size *= 2;

		/* Try expanding. */
		if (!expand_array(env, arr_val, arr, new_size)) {
			/* Out-of-memory error. */
			rt_out_of_memory(env);
			return false;
		}

		/* Track the newer chain. */
		arr = arr_val->val.arr;
		while (arr->newer != NULL)
			arr = arr->newer;
	}

	/* Write. */
	arr->table[index] = *val;
	if (index >= arr->size)
		arr->size = index + 1;

	/* Write barrier for the remember set. */
	rt_gc_array_write_barrier(env, arr, index, val);

	/* Succeeded. */
	return true;
}

/* Helper: Expand an array. */
static INLINE bool
expand_array(
	struct rt_env *env,
	struct rt_value *arr_val,
	struct rt_array *arr,
	size_t new_alloc_size)
{
	struct rt_array *new_arr;
	size_t copy_size, i;

	/* Allocate the new array. */
	new_arr = rt_gc_alloc_array(env, new_alloc_size);
	if (new_arr == NULL) {
		/*
		 * Error: out-of-memory.
		 */
		rt_out_of_memory(env);
		return false;
	}

	/* Get the array pointer. */
	arr = arr_val->val.arr;
	while (arr->newer != NULL)
		arr = arr->newer;

	/* Do copy. */
	if (new_alloc_size < arr->size)
		copy_size = new_alloc_size;
	else
		copy_size = arr->size;
	for (i = 0; i < copy_size; i++) {
		/* Copy an entry. */
		new_arr->table[i] = arr->table[i];
		new_arr->size++;

		/* Write barrier for the remember set. */
		rt_gc_array_write_barrier(env, new_arr, i, &new_arr->table[i]);
	}

	/* Link the new object. */
	arr->newer = new_arr;

	return true;
}

/*
 * Resize an array.
 */
bool
om_resize_array(
	struct rt_env *env,
	struct rt_value *arr_val,
	size_t req_size)
{
	struct rt_array *arr;

	/* Track the newer chain. */
	arr = arr_val->val.arr;
	while (arr->newer != NULL)
		arr = arr->newer;

	/* Try expanding. (including size reduction) */
	if (!expand_array(env, arr_val, arr, req_size)) {
		/* Out-of-memory error. */
		rt_out_of_memory(env);
		return false;
	}

	/* Track the newer chain. */
	arr = arr_val->val.arr;
	while (arr->newer != NULL)
		arr = arr->newer;

	arr->size = req_size;

	/* Succeeded to expand. */
	return true;
}

/*
 * Make a shallow copy of an array.
 */
bool
om_copy_array(
	struct rt_env *env,
	struct rt_value *dst,
	struct rt_value *src)
{
	struct rt_array *d, *s;
	size_t size, i;

	/* Track the newer chain for src. */
	s = src->val.arr;
	while (s->newer != NULL)
		s = s->newer;

	/* Get the source array allocation size. */
	size = s->alloc_size;

	/* Make a dictionary. */
	d = rt_gc_alloc_array(env, size);
	if (d == NULL) {
		/* Out-of-memory error. */
		rt_out_of_memory(env);
		return false;
	}

	/* Make the dst value. */
	dst->type = NOCT_VALUE_ARRAY;
	dst->val.arr = d;

	/* Track the newer chain for src again. */
	s = src->val.arr;
	while (s->newer != NULL)
		s = s->newer;

	/* Copy the dictionary with write-barrier. */
	for (i = 0; i < size; i++) {
		/* Copy an item. */
		d->table[i] = s->table[i];
		d->size++;

		/* Write barrier. */
		rt_gc_array_write_barrier(env, d, i, &d->table[i]);
	}

	return true;
}

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
	struct rt_value *val)
{
	struct rt_dict *dict;
	const uint32_t START_SIZE = 16;

	/* Allocate a dictionary. This may occur GC. */
	dict = rt_gc_alloc_dict(env, START_SIZE);
	if (dict == NULL) {
		rt_out_of_memory(env);
		return false;
	}

	/* Setup a value. */
	val->type = NOCT_VALUE_DICT;
	val->val.dict = dict;

	return true;

}

/*
 * Get the size of a dictionary.
 */
bool
om_get_dict_size(
	struct rt_env *env,
	struct rt_value *val,
	size_t *size)
{
	struct rt_dict *dict;

	UNUSED_PARAMETER(env);

	/* Track the newer chain. */
	dict = val->val.dict;
	while (dict->newer != NULL)
		dict = dict->newer;

	/* Get the allocated size. */
	*size = dict->size;

	return true;
}

/*
 * Get the allocation size of a dictionary.
 */
bool
om_get_dict_alloc_size(
	struct rt_env *env,
	struct rt_value *val,
	size_t *size)
{
	struct rt_dict *dict;

	UNUSED_PARAMETER(env);

	/* Track the newer chain. */
	dict = val->val.dict;
	while (dict->newer != NULL)
		dict = dict->newer;

	/* Get the allocated size. */
	*size = dict->alloc_size;

	return true;
}

/*
 * Check if a key exists in a dictionary.
 */
bool
om_check_dict_key(
	struct rt_env *env,
	struct rt_value *dict_val,
	struct rt_value *key,
	bool *ret)
{
	struct rt_dict *dict;
	size_t size, index, i;

	UNUSED_PARAMETER(env);

	/* Track the newer chain. */
	dict = dict_val->val.dict;
	while (dict->newer != NULL)
		dict = dict->newer;

	/* Get the dictionary allocation size. */
	size = dict->alloc_size;

	/* Search for the key. */
	index = key->val.str->hash & (uint32_t)(dict->alloc_size - 1);
	for (i = index;
	     i != ((index - 1 + size) & (size - 1));
	     i = (i + 1) & ((uint32_t)size - 1)) {
		/* Skip if a removed slot. (tombstone) */
		if (dict->key[i].type == NOCT_VALUE_FLOAT)
			continue;

		/* Stop if an empty slot. */
		if (dict->key[i].type == NOCT_VALUE_INT) {
			/* Failed: Key not found. */
			*ret = false;
			break;
		}

		/* Get the key string pointer at the second word. */
		if (dict->key[i].val.str->len == key->val.str->len &&
		    dict->key[i].val.str->hash == key->val.str->hash &&
		    strcmp(dict->key[i].val.str->data, key->val.str->data) == 0) {
			/* Found the key. */
			*ret = true;
			break;
		}
	}

	/* Not found. */
	if (!ret)
		return false;

	/* Found. */
	return true;
}

/*
 * Read a dictionary element.
 */
bool
om_read_dict(
	struct rt_env *env,
	struct rt_value *dict_val,
	struct rt_value *key,
	struct rt_value *val)
{
	if (!om_read_dict_with_hash(env,
				    dict_val,
				    key->val.str->data,
				    key->val.str->len,
				    key->val.str->hash,
				    val))
		return false;

	return true;
}

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
	struct rt_value *val)
{
	struct rt_dict *dict;
	size_t index, i;

	UNUSED_PARAMETER(env);

	/* Track the newer chain. */
	dict = dict_val->val.dict;
	while (dict->newer != NULL)
		dict = dict->newer;

	/* Search for the key. */
	index = key_hash & (uint32_t)(dict->alloc_size - 1);
	for (i = index;
	     i != ((index - 1 + dict->alloc_size) & (dict->alloc_size - 1));
	     i = (i + 1) & ((uint32_t)dict->alloc_size - 1)) {
		/* Skip if a removed slot. (tombstone) */
		if (dict->key[i].type == NOCT_VALUE_FLOAT)
			continue;

		/* Stop if an empty slot. */
		if (dict->key[i].type == NOCT_VALUE_INT) {
			/* Failed: Key not found. */
			rt_error(env, N_TR("Dictionary key \"%s\" not found."), key);
			return false;
		}

		if (dict->key[i].val.str->len == key_len &&
		    dict->key[i].val.str->hash == key_hash &&
		    strcmp(dict->key[i].val.str->data, key) == 0) {
			/* Found the key. */
			*val = dict->value[i];
			break;
		}
	}

	/* Succeeded. */
	return true;
}

/*
 * Read a dictionary element by a slot index. (for traverse)
 */
bool
om_read_dict_index(
	struct rt_env *env,
	struct rt_value *dict_val,
	size_t index,
	struct rt_value *key,
	struct rt_value *val)
{
	struct rt_dict *dict;
	size_t i, items;

	UNUSED_PARAMETER(env);

	/* Track the newer chain. */
	dict = dict_val->val.dict;
	while (dict->newer != NULL)
		dict = dict->newer;

	/* Check for the size. */
	if (index >= dict->size) {
		/* Index is out-of-bound. */
		rt_error(env, N_TR("Dictionary index %ld is out-of-range."), index);
		return false;
	}

	/* Search. */
	items = 0;
	for (i = 0 ; i < dict->alloc_size; i++) {
		if (dict->key[i].type != NOCT_VALUE_STRING)
			continue;
		if (items == index) {
			*val = dict->value[i];
			*key = dict->key[i];
			break;
		}
		items++;
	}
	if (i == dict->alloc_size) {
		/* Failed. */
		rt_error(env, N_TR("Dictionary index %ld is out-of-range."), index);
		return false;
	}

	return true;
}

/*
 * Write a dictionary element.
 */
bool
om_write_dict(
	struct rt_env *env,
	struct rt_value *dict_val,
	struct rt_value *key,
	struct rt_value *val)
{
	struct rt_dict *dict;
	size_t new_size;
	size_t index, i;
	bool is_insertion;
	bool found_tombstone;
	size_t first_tombstone_index;

	UNUSED_PARAMETER(env);

retry:
	/* Track the newer chain. */
	dict = dict_val->val.dict;
	while (dict->newer != NULL)
		dict = dict->newer;

	/* If 75% of the space is used, go to the expand phase. */
	if (dict->size >= dict->alloc_size / 4 * 3)
		goto expand_phase;

	/* Make sure we have at least one space. */
	if (dict->size == dict->alloc_size) {
		/* No space. */
		assert(NO_SPACE_FOR_DICTIONARY);
		goto expand_phase;
	}

	/* Search an index for in-place write. */
	index = key->val.str->hash & (uint32_t)(dict->alloc_size - 1);
	found_tombstone = false;
	for (i = index;
	     i != ((index - 1 + dict->alloc_size) & (dict->alloc_size - 1));
	     i = (i + 1) & ((uint32_t)dict->alloc_size - 1)) {
		/*
		 * Found a tombstone for a removed slot.
		 * Skip for now with a  mark.
		 */
		if (dict->key[i].type == NOCT_VALUE_FLOAT) {
			if (!found_tombstone) {
				found_tombstone = true;
				first_tombstone_index = i;
			}
			continue;
		}

		/*
		 * Found an empty slot. Proceed to the expand phase if 75% of
		 * the space is used.
		 */
		if (dict->key[i].type == NOCT_VALUE_INT) {
			/* Proceed to write. (insertion) */
			is_insertion = true;
			break;
		}

		/* Found the same key. Proceed to write. */
		if (dict->key[i].val.str->len == key->val.str->len &&
		    dict->key[i].val.str->hash == key->val.str->hash &&
		    strcmp(dict->key[i].val.str->data, key->val.str->data) == 0) {
			/* Proceed to write. (non-insertion) */
			is_insertion = false;
			break;
		}
	}
	if (found_tombstone) {
		i = first_tombstone_index;
		is_insertion = true;
	}

	/* Execute an in-place write */
	dict->value[i] = *val;
	if (is_insertion) {
		dict->key[i] = *key;
		dict->size++;
	}

	/* Write barrier for the remember set. */
	rt_gc_dict_write_barrier(env, dict, val);
	if (is_insertion)
		rt_gc_dict_write_barrier(env, dict, key);

	/* Succeeded. */
	return true;

	/*
	 * Expand phase.
	 */
expand_phase:
	/* Determine the new size. */
	new_size = dict->alloc_size * 2;

	/* Try expanding. */
	if (!expand_dict(env, dict_val, dict, new_size)) {
		/* Out-of-memory error.	 */
		rt_out_of_memory(env);
		return false;
	}

	/* Succeeded to expand. Restart from the beginning. */
	goto retry;
}

/* Helper: Expand an array. */
static INLINE bool
expand_dict(
	struct rt_env *env,
	struct rt_value *dict_val,
	struct rt_dict *dict,
	size_t size)
{
	struct rt_dict *new_dict;
	size_t old_alloc_size;
	size_t i, j, index;

	/* Allocate the new dictionary. */
	new_dict = rt_gc_alloc_dict(env, size);
	if (new_dict == NULL) {
		/* Error: out-of-memory. */
		rt_out_of_memory(env);
		return false;
	}

	/* Get the dictionary pointer again. */
	dict = dict_val->val.dict;
	while (dict->newer != NULL)
		dict = dict->newer;

	/* Do copy/rehash. */
	old_alloc_size = dict->alloc_size;
	for (i = 0; i < old_alloc_size; i++) {
		/* SKip if an empty slot. */
		if (dict->key[i].type == NOCT_VALUE_INT)
			continue;

		/* SKip if a removed slot. (tombstone) */
		if (dict->key[i].type == NOCT_VALUE_FLOAT)
			continue;

		/* Do an in-place write. */
		index = rt_string_hash(dict->key[i].val.str->data) & ((uint32_t)new_dict->alloc_size - 1);
		for (j = index;
		     j != ((index - 1 + new_dict->alloc_size) & (new_dict->alloc_size - 1));
		     j = (j + 1) & ((uint32_t)new_dict->alloc_size - 1)) {
			/* If an empty entry. */
			if (new_dict->key[j].type == NOCT_VALUE_INT) {
				/* Copy the key and the value. */
				new_dict->key[j] = dict->key[i];
				new_dict->value[j] = dict->value[i];

				/* Write barriers for the key and the value. */
				rt_gc_dict_write_barrier(env, new_dict, &new_dict->key[j]);
				rt_gc_dict_write_barrier(env, new_dict, &new_dict->value[j]);

				break;
			}
		}
	}
	new_dict->size = dict->size;

	/* Link the new object, that is, do copy-publish. */
	dict->newer = new_dict;

	/* Restarting from the beginning of the transaction is required. */
	return true;
}

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
	struct rt_value *val)
{
	struct rt_value s;

	rt_pin_local(env, &s);

	if (!rt_make_string_with_hash(env, &s, key, len, hash)) {
		rt_unpin_local(env, &s);
		return false;
	}

	if (!om_write_dict(env, dict_val, &s, val)) {
		rt_unpin_local(env, &s);
		return false;
	}

	rt_unpin_local(env, &s);

	return true;
}

/*
 * Write a dictionary element.
 */
bool
om_erase_dict_entry(
	struct rt_env *env,
	struct rt_value *dict_val,
	struct rt_value *key)
{
	struct rt_dict *dict;
	size_t index, i;
	bool is_not_found;

	UNUSED_PARAMETER(env);

	/* Track the newer chain. */
	dict = dict_val->val.dict;
	while (dict->newer != NULL)
		dict = dict->newer;

	/* Search an index for in-place write. */
	index = key->val.str->hash & (uint32_t)(dict->alloc_size - 1);
	for (i = index;
	     i != ((index - 1 + dict->alloc_size) & (dict->alloc_size - 1));
	     i = (i + 1) & ((uint32_t)dict->alloc_size - 1)) {
		/*
		 * Found a tombstone for a removed slot.
		 * Skip for now with a mark.
		 */
		if (dict->key[i].type == NOCT_VALUE_FLOAT)
			continue;

		/* Found the same key. Proceed to write. */
		if (dict->key[i].val.str->len == key->val.str->len &&
		    dict->key[i].val.str->hash == key->val.str->hash &&
		    strcmp(dict->key[i].val.str->data, key->val.str->data) == 0) {
			/* Proceed to write. (non-insertion) */
			is_not_found = false;
			break;
		}
	}
	if (is_not_found) {
		/* Failed: Key not found. */
		rt_error(env, N_TR("Dictionary key \"%s\" not found."), key);
		return false;
	}

	/* Execute an in-place write. */
	dict->key[i].type = NOCT_VALUE_FLOAT;
	dict->key[i].val.f = 0.0f;
	dict->value[i].type = NOCT_VALUE_INT;
	dict->value[i].val.i = 0;

	/* Succeeded. */
	return true;
}

/*
 * Make a shallow copy of a dictionary.
 */
bool
om_copy_dict(
	struct rt_env *env,
	struct rt_value *dst,
	struct rt_value *src)
{
	struct rt_dict *d, *s;
	size_t size, i;

	/* Track the newer chain. */
	s = src->val.dict;
	while (s->newer != NULL)
		s = s->newer;

	/* Get the source dictionary allocation size. */
	size = s->alloc_size;

	/* Make a dictionary. */
	d = rt_gc_alloc_dict(env, size);
	if (d == NULL) {
		/* Out-of-memory error. */
		rt_out_of_memory(env);
		return false;
	}

	/* Make a dst value. */
	dst->type = NOCT_VALUE_DICT;
	dst->val.dict = d;

	/* Track the newer chain again. */
	s = src->val.dict;
	while (s->newer != NULL)
		s = s->newer;

	/* Copy the dictionary with write-barrier. */
	for (i = 0; i < size; i++) {
		/* Skip an empty or removed entry. */
		if (s->key[i].type != NOCT_VALUE_STRING)
			continue;

		/* Copy an item. */
		d->key[i] = s->key[i];
		d->value[i] = s->value[i];
		d->size++;

		/* Write barrier. */
		rt_gc_dict_write_barrier(env, d, &d->key[i]);
		rt_gc_dict_write_barrier(env, d, &d->value[i]);
	}

	return true;
}

/*
 * Merges a dictionary.
 */
bool
om_merge_dict(
	struct rt_env *env,
	struct rt_value *dst,
	struct rt_value *src1,
	struct rt_value *src2)
{
	struct rt_dict *d, *s1, *s2;
	size_t src1_size, src2_size, dst_size;
	size_t i, j, k, index;
	bool is_insert;

	/* Track the newer chains. */
	s1 = src1->val.dict;
	while (s1->newer != NULL)
		s1 = s1->newer;
	s2 = src2->val.dict;
	while (s2->newer != NULL)
		s2 = s2->newer;

	/* Get the source dictionary item sizes. */
	src1_size = s1->size;
	src2_size = s2->size;

	/* Determine the destination size. */
	dst_size = 1;
	while (dst_size < src1_size || dst_size < src2_size)
		dst_size *= 2;

	/* Make a dictionary. */
	d = rt_gc_alloc_dict(env, dst_size);
	if (d == NULL) {
		/* Out-of-memory error. */
		rt_out_of_memory(env);
		return false;
	}

	/* Make the dst value. */
	dst->type = NOCT_VALUE_DICT;
	dst->val.dict = d;

	/* Start a source dictionary read again. */
	s1 = src1->val.dict;
	while (s1->newer != NULL)
		s1 = s1->newer;
	s2 = src2->val.dict;
	while (s2->newer != NULL)
		s2 = s2->newer;

	/* Copy the dictionary with write-barrier. */
	for (i = 0; i < 2; i++) {
		struct rt_dict *sn;
		if (i == 0)
			sn = s1;
		else
			sn = s2;
			
		for (j = 0; j < sn->alloc_size; j++) {
			/* Skip an empty or removed entry. */
			if (sn->key[j].type != NOCT_VALUE_STRING)
				continue;

			/* In-place write. */
			is_insert = true;
			index = sn->key[j].val.str->hash & (uint32_t)(d->alloc_size - 1);
			for (k = index;
			     k != ((index - 1 + d->alloc_size) & (d->alloc_size - 1));
			     k = (k + 1) & ((uint32_t)d->alloc_size - 1)) {
				/* Skip an empty slot. */
				if (d->key[k].type != NOCT_VALUE_STRING)
					break;

				/* Found the same key. */
				if (d->key[k].val.str->len == sn->key[j].val.str->len &&
				    d->key[k].val.str->hash == sn->key[j].val.str->hash &&
				    strcmp(d->key[k].val.str->data, sn->key[j].val.str->data) == 0) {
					is_insert = false;
					break;
				}
			}

			/* Copy an item. */
			d->value[k] = sn->value[j];
			if (is_insert) {
				d->key[k] = sn->key[j];
				d->size++;
			}

			/* Write barrier. */
			rt_gc_dict_write_barrier(env, d, &d->key[k]);
			rt_gc_dict_write_barrier(env, d, &d->value[k]);
		}
	}

	return true;
}

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
	int gc_level)
{
	UNUSED_PARAMETER(gc_level);

	env->vm->gc_in_progress_counter++;

	return true;
}

/*
 * Leave a GC execution.
 */
void
om_leave_gc(
	struct rt_env *env)
{
	env->vm->gc_in_progress_counter--;
}

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
	struct rt_env *env)
{
	UNUSED_PARAMETER(env);
}

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
	struct rt_env *env)
{
	UNUSED_PARAMETER(env);
}
