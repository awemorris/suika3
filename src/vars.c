/* -*- coding: utf-8; tab-width: 8; indent-tabs-mode: t; -*- */

/*
 * Suika3
 * Variable Subsystem
 */

/*-
 * SPDX-License-Identifier: Zlib
 *
 * Copyright (c) 1996-2026 Awe Morris / SCHOLA SUIKAE
 *
 * This software is provided 'as-is', without any express or implied
 * warranty. In no event will the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software. If you use this software
 *    in a product, an acknowledgment in the product documentation would be
 *    appreciated but is not required.
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
 */

#include <suika3/suika3.h>
#include "vars.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define START_SIZE	32

/* Key-value store. */
struct item {
	char *key;
	char *value;
	uint32_t hash;
	uint32_t len;
	bool is_global;
};
static struct item *tbl;
static size_t alloc_size;
static size_t used_size;

/* Forward declaration. */
static bool expand_table(void);
static uint32_t string_hash(const char *s);

/*
 * Initialize the variable subsystem.
 */
bool
s3i_init_vars(void)
{
	tbl = calloc(sizeof(struct item) * START_SIZE, 1);
	if (tbl == NULL) {
		s3_log_out_of_memory();
		return false;
	}

	alloc_size = START_SIZE;
	used_size = 0;

	return true;
}

/*
 * Cleanup the variable subsystem.
 */
void
s3i_cleanup_vars(void)
{
	uint32_t i;

	for (i = 0; i < alloc_size; i++) {
		if (tbl[i].key != NULL)
			free(tbl[i].key);
		if (tbl[i].value != NULL)
			free(tbl[i].value);
	}

	if (tbl != NULL) {
		free(tbl);
		tbl = NULL;
	}

	alloc_size = 0;
	used_size = 0;
}

/*
 * Set an integer value to a variable.
 */
bool
s3_set_variable_int(
	const char *name,
	int val)
{
	char digits[128];

	snprintf(digits, sizeof(digits), "%d", val);
	s3_set_variable_string(name, digits);

	return true;
}

/*
 * Set a float value to a variable.
 */
bool
s3_set_variable_float(
	const char *name,
	float val)
{
	char digits[128];

	snprintf(digits, sizeof(digits), "%f", val);
	s3_set_variable_string(name, digits);

	return true;
}

/*
 * Set a string value to a variable.
 */
bool
s3_set_variable_string(
	const char *name,
	const char* val)
{
	uint32_t hash;
	uint32_t len;
	uint32_t index;
	uint32_t i;

	hash = string_hash(name);
	len = (uint32_t)strlen(name);

	/* Search for the key to replace the value. */
	index = hash & ((uint32_t)alloc_size - 1);
	for (i = index;
	     i != ((index - 1 + (uint32_t)alloc_size) & ((uint32_t)alloc_size - 1));
	     i = (i + 1) & ((uint32_t)alloc_size - 1)) {
		if (tbl[i].key == NULL)
			break;
		if (tbl[i].len == len &&
		    tbl[i].hash == hash &&
		    strcmp(tbl[i].key, name) == 0) {
			/* Found, replace the value. */
			free(tbl[i].value);
			tbl[i].value = strdup(val);
			if (tbl[i].value == NULL) {
				s3_log_out_of_memory();
				return false;
			}
			return true;
		}
	}

	/* Key doesn't exist. Add new one. */

	/* Expand the size if 75% is used. */
	if (used_size >= alloc_size / 4 * 3) {
		/* Reallocate the table. */
		if (!expand_table())
			return false;
	}

	/* Append. */
	index = hash & ((uint32_t)alloc_size - 1);
	for (i = index;
	     i != ((index - 1 + (uint32_t)alloc_size) & ((uint32_t)alloc_size - 1));
	     i = (i + 1) & ((uint32_t)alloc_size - 1)) {
		if (tbl[i].key == NULL) {
			/* Make a key value. */
			tbl[i].key = strdup(name);
			if (tbl[i].key == NULL) {
				s3_log_out_of_memory();
				return false;
			}
			tbl[i].value = strdup(val);
			if (tbl[i].value == NULL) {
				s3_log_out_of_memory();
				return false;
			}
			tbl[i].hash = hash;
			tbl[i].len = len;
			break;
		}
	}
	used_size++;

	return true;
}

/*
 * Reallocate the table.
 */
static bool
expand_table(void)
{
	struct item *new_tbl;
	size_t i, old_size, new_size;
	uint32_t index, j;

	old_size = alloc_size;
	new_size = old_size * 2;

	/* Allocate the new array. */
	new_tbl = calloc(sizeof(struct item) * new_size, 1);
	if (new_tbl == NULL) {
		s3_log_out_of_memory();
		return false;
	}

	/* Rehash. */
	for (i = 0; i < old_size; i++) {
		if (tbl[i].key == NULL)
			continue;

		index = tbl[i].hash & ((uint32_t)new_size - 1);
		for (j = index;
		     j != ((index - 1 + (uint32_t)new_size) & ((uint32_t)new_size - 1));
		     j = (j + 1) & ((uint32_t)new_size - 1)) {
			if (new_tbl[i].key == NULL) {
				/* Copy the key and values. */
				new_tbl[j].key = tbl[i].key;
				new_tbl[j].value = tbl[i].value;
				new_tbl[j].hash = tbl[i].hash;
				new_tbl[j].len = tbl[i].len;
				break;
			}
		}
	}

	alloc_size = new_size;

	free(tbl);
	tbl = new_tbl;

	return true;
}

/*
 * Unset a variable.
 */
bool
s3_unset_variable(
	const char *name)
{
	uint32_t hash;
	uint32_t len;
	uint32_t index;
	uint32_t i;

	hash = string_hash(name);
	len = (uint32_t)strlen(name);

	/* Search for the key. */
	index = hash & ((uint32_t)alloc_size - 1);
	for (i = index;
	     i != ((index - 1 + (uint32_t)alloc_size) & ((uint32_t)alloc_size - 1));
	     i = (i + 1) & ((uint32_t)alloc_size - 1)) {
		if (tbl[i].key == NULL)
			break;
		if (tbl[i].len == len &&
		    tbl[i].hash == hash &&
		    strcmp(tbl[i].key, name) == 0) {
			/* Found, remove it. */
			free(tbl[i].key);
			free(tbl[i].value);
			tbl[i].key = NULL;
			tbl[i].value = NULL;
			tbl[i].hash = 0;
			tbl[i].len = 0;
			used_size--;
			return true;
		}
	}

	return false;
}

/*
 * Set a variable global.
 */
bool
s3_make_variable_global(
	const char *name,
	bool is_global)
{
	uint32_t hash;
	uint32_t len;
	uint32_t index;
	uint32_t i;

	hash = string_hash(name);
	len = (uint32_t)strlen(name);

	/* Search for the key. */
	index = hash & ((uint32_t)alloc_size - 1);
	for (i = index;
	     i != ((index - 1 + (uint32_t)alloc_size) & ((uint32_t)alloc_size - 1));
	     i = (i + 1) & ((uint32_t)alloc_size - 1)) {
		if (tbl[i].key == NULL)
			break;
		if (tbl[i].len == len &&
		    tbl[i].hash == hash &&
		    strcmp(tbl[i].key, name) == 0) {
			/* Found, make it global or not. */
			tbl[i].is_global = is_global;
			return true;
		}
	}

	return false;
}

/*
 * Get an integer value from a variable.
 */
int
s3_get_variable_int(
	const char *name)
{
	const char *s_value;
	int i_value;

	s_value = s3_get_variable_string(name);
	if (s_value == NULL)
		return 0;

	i_value = atoi(s_value);

	return i_value;
}

/*
 * Get a float value from a variable.
 */
float
s3_get_variable_float(
	const char *name)
{
	const char *s_value;
	float f_value;

	s_value = s3_get_variable_string(name);
	if (s_value == NULL)
		return 0;

	f_value = (float)atof(s_value);

	return f_value;
}

/*
 * Get a string value from a variable.
 */
const char *
s3_get_variable_string(
	const char *name)
{
	uint32_t hash;
	uint32_t len;
	uint32_t index;
	uint32_t i;

	hash = string_hash(name);
	len = (uint32_t)strlen(name);

	/* Search for the key. */
	index = hash & ((uint32_t)alloc_size - 1);
	for (i = index;
	     i != ((index - 1 + (uint32_t)alloc_size) & ((uint32_t)alloc_size - 1));
	     i = (i + 1) & ((uint32_t)alloc_size - 1)) {
		if (tbl[i].key == NULL)
			break;
		if (tbl[i].len == len &&
		    tbl[i].hash == hash &&
		    strcmp(tbl[i].key, name) == 0) {
			/* Found, remove it. */
			return tbl[i].value;
		}
	}

	return NULL;
}

/*
 * Get the numbers of the variables.
 */
int
s3_get_variable_count(void)
{
	return (int)used_size;
}

/*
 * Get the name of the variable at an index.
 */
const char *
s3_get_variable_name(
	int index)
{
	uint32_t i;
	int valid_count;

	valid_count = 0;
	for (i = 0; i < alloc_size; i++) {
		if (tbl[i].key == NULL)
			continue;
		if (valid_count == index)
			return tbl[i].key;
		valid_count++;
	}
				     
	return NULL;
}

/*
 * Check if a variable exists for a name.
 */
bool
s3_check_variable_exists(
	const char *name)
{
	uint32_t hash;
	uint32_t len;
	uint32_t index;
	uint32_t i;

	hash = string_hash(name);
	len = (uint32_t)strlen(name);

	/* Search for the key. */
	index = hash & ((uint32_t)alloc_size - 1);
	for (i = index;
	     i != ((index - 1 + (uint32_t)alloc_size) & ((uint32_t)alloc_size - 1));
	     i = (i + 1) & ((uint32_t)alloc_size - 1)) {
		if (tbl[i].key == NULL)
			break;
		if (tbl[i].len == len &&
		    tbl[i].hash == hash &&
		    strcmp(tbl[i].value, name) == 0) {
			/* Found. */
			return true;
		}
	}

	return false;
}

/*
 * Check if a variable is global.
 */
bool
s3_is_global_variable(
	const char *name)
{
	uint32_t hash;
	uint32_t len;
	uint32_t index;
	uint32_t i;

	hash = string_hash(name);
	len = (uint32_t)strlen(name);

	/* Search for the key. */
	index = hash & ((uint32_t)alloc_size - 1);
	for (i = index;
	     i != ((index - 1 + (uint32_t)alloc_size) & ((uint32_t)alloc_size - 1));
	     i = (i + 1) & ((uint32_t)alloc_size - 1)) {
		if (tbl[i].key == NULL)
			break;
		if (tbl[i].len == len &&
		    tbl[i].hash == hash &&
		    strcmp(tbl[i].key, name) == 0) {
			/* Found. */
			return tbl[i].is_global;
		}
	}

	return false;
}

/*
 * Unset all local variables.
 */
void
s3_unset_local_variables(void)
{
	uint32_t i;

	/* Search for the key. */
	for (i = 0; i < alloc_size; i++) {
		if (tbl[i].key == NULL)
			continue;
		if (!tbl[i].is_global) {
			/* Found, remove it. */
			free(tbl[i].key);
			free(tbl[i].value);
			tbl[i].key = NULL;
			tbl[i].value = NULL;
			tbl[i].hash = 0;
			tbl[i].len = 0;
			used_size--;
			if (used_size == 0)
				break;
		}
	}
}

/* FNV-1a */
static uint32_t
string_hash(const char *s)
{
	uint32_t hash = 2166136261u;
	while (*s) {
		hash ^= (uint8_t)*s++;
		hash *= 16777619u;
	}
	return hash;
}
