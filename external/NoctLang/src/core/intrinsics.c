/* -*- coding: utf-8; tab-width: 8; indent-tabs-mode: t; -*- */

/*
 * Copyright (c) 2025, Awe Morris. All rights reserved.
 */

/*
 * Intrinsics Implementation
 */

#include "runtime.h"
#include "intrinsics.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>

#define NEVER_COME_HERE		0

static bool rt_intrin_new(NoctEnv *env);
static bool rt_intrin_int(NoctEnv *env);
static bool rt_intrin_float(NoctEnv *env);
static bool rt_intrin_push(NoctEnv *env);
static bool rt_intrin_pop(NoctEnv *env);
static bool rt_intrin_isset(NoctEnv *env);
static bool rt_intrin_unset(NoctEnv *env);
static bool rt_intrin_newArray(NoctEnv *env);
static bool rt_intrin_resize(NoctEnv *env);
static bool rt_intrin_charCount(NoctEnv *env);
static bool rt_intrin_charAt(NoctEnv *env);
static bool rt_intrin_substring(NoctEnv *env);
static bool rt_intrin_fast_gc(NoctEnv *env);
static bool rt_intrin_full_gc(NoctEnv *env);
static bool rt_intrin_compact_gc(NoctEnv *env);

struct intrin_item {
	const char *field_name;
	const char *reg_name;
	int param_count;
	const char *param[NOCT_ARG_MAX];
	bool (*cfunc)(struct rt_env *rt);
	bool is_thiscall;
	struct rt_func *func;
} intrin_items[] = {
	{"new",        "new",         2, {"cls", "init"         }, rt_intrin_new,        false, NULL},
	{"int",        "int",         1, {"val"                 }, rt_intrin_int,        false, NULL},
	{"float",      "float",       1, {"val"                 }, rt_intrin_float,      false, NULL},
	{"push",       "__push",      2, {"this", "val"         }, rt_intrin_push,       true,  NULL},
	{"pop",        "__pop",       1, {"this"                }, rt_intrin_pop,        true,  NULL},
	{"newArray",   "newArray",    1, {"size"                }, rt_intrin_newArray,   true,  NULL},
	{"resize",     "__resize",    2, {"this", "size"        }, rt_intrin_resize,     true,  NULL},
	{"charCount",  "__charCount", 1, {"this",               }, rt_intrin_charCount,  true,  NULL},
	{"charAt",     "__charAt",    2, {"this", "index"       }, rt_intrin_charAt,     true,  NULL},
	{"substring",  "__substring", 3, {"this", "start", "len"}, rt_intrin_substring,  true,  NULL},
	{"isset",      "__isset",     2, {"this", "key"         }, rt_intrin_isset,      true,  NULL},
	{"unset",      "__unset",     2, {"this", "key"         }, rt_intrin_unset,      true,  NULL},
	{"fast_gc",    "fast_gc",     0, {NULL},                   rt_intrin_fast_gc,    false, NULL},
	{"full_gc",    "full_gc",     0, {NULL},                   rt_intrin_full_gc,    false, NULL},
	{"compact_gc", "compact_gc",  0, {NULL},                   rt_intrin_compact_gc, true,  NULL},
};

size_t get_string_length(const char *text);
static int utf8_to_utf32(const char *mbs, uint32_t *wc);

bool
rt_register_intrinsics(
	struct rt_env *env)
{
	int i;

	for (i = 0; i < (int)(sizeof(intrin_items) / sizeof(struct intrin_item)); i++) {
		if (!noct_register_cfunc(env,
					 intrin_items[i].reg_name,
					 intrin_items[i].param_count,
					 intrin_items[i].param,
					 intrin_items[i].cfunc,
					 &intrin_items[i].func))
			return false;
	}

	return true;
}

bool
rt_get_intrin_thiscall_func(
	NoctEnv *env,
	const char *name,
	struct rt_func **func)
{
	int i;

	for (i = 0; i < (int)(sizeof(intrin_items) / sizeof(struct intrin_item)); i++) {
		if (!intrin_items[i].is_thiscall)
			continue;
		if (strcmp(intrin_items[i].field_name, name) != 0)
			continue;

		/* Found. */
		*func = intrin_items[i].func;
		return true;
	}

	/* Not found/ */
	*func = NULL;
	return true;
}

/* new() */
static bool
rt_intrin_new(
	NoctEnv *env)
{
	struct rt_value cls, init, ret, key, val;
	const char *key_s;
	uint32_t i, count;

	noct_pin_local(env, 5, &cls, &init, &ret, &key, &val);

	if (!noct_get_arg_check_dict(env, 0, &cls))
		return false;
	if (!noct_get_arg_check_dict(env, 1, &init))
		return false;

	/* Make a dictionary. */
	if (!noct_make_empty_dict(env, &ret))
		return false;

	/* Copy the class elements. */
	if (!noct_get_dict_size(env, &cls, &count))
		return false;
	for (i = 0; i < count; i++) {
		if (!noct_get_dict_key_by_index(env, &cls, i, &key))
			return false;
		if (!noct_get_dict_value_by_index(env, &cls, i, &val))
			return false;
		if (!noct_get_string(env, &key, &key_s))
			return false;
		if (!noct_set_dict_elem(env, &ret, key_s, &val))
			return false;
	}

	/* Copy the initializer. */
	if (!noct_get_dict_size(env, &init, &count))
		return false;
	for (i = 0; i < count; i++) {
		if (!noct_get_dict_key_by_index(env, &init, i, &key))
			return false;
		if (!noct_get_dict_value_by_index(env, &init, i, &val))
			return false;
		if (!noct_get_string(env, &key, &key_s))
			return false;
		if (!noct_set_dict_elem(env, &ret, key_s, &val))
			return false;
	}

	/* Set the return value. */
	if (!noct_set_return(env, &ret))
		return false;

	return true;
}

/* int() */
static bool
rt_intrin_int(
	NoctEnv *env)
{
	struct rt_value val, ret;
	int type;

	noct_pin_local(env, 2, &val, &ret);

	if (!noct_get_arg(env, 0, &val))
		return false;
	if (!noct_get_value_type(env, &val, &type))
		return false;

	switch (type) {
	case NOCT_VALUE_INT:
		if (!noct_set_return(env, &val))
			return false;
		break;
	case NOCT_VALUE_FLOAT:
	{
		float val_f;
		if (!noct_get_float(env, &val, &val_f))
			return false;
		if (!noct_set_return_make_int(env, &ret, (int)val_f))
			return false;
		break;
	}
	case NOCT_VALUE_STRING:
	{
		const char *val_s;
		if (!noct_get_string(env, &val, &val_s))
			return false;
		if (!noct_set_return_make_int(env, &ret, atoi(val_s)))
			return false;
		break;
	}
	default:
		noct_error(env, N_TR("Value is not a number or a string."));
		return false;
	}

	return true;
}

/* float() */
static bool
rt_intrin_float(
	NoctEnv *env)
{
	struct rt_value val, ret;
	int type;

	noct_pin_local(env, 2, &val, &ret);

	if (!noct_get_arg(env, 0, &val))
		return false;
	if (!noct_get_value_type(env, &val, &type))
		return false;

	switch (type) {
	case NOCT_VALUE_INT:
	{
		int val_i;
		if (!noct_get_int(env, &val, &val_i))
			return false;
		if (!noct_set_return_make_float(env, &ret, (float)val_i))
			return false;
		break;
	}
	case NOCT_VALUE_FLOAT:
		if (!noct_set_return(env, &val))
			return false;
		break;
	case NOCT_VALUE_STRING:
	{
		const char *val_s;
		if (!noct_get_string(env, &val, &val_s))
			return false;
		if (!noct_set_return_make_float(env, &ret, (float)atof(val_s)))
			return false;
		break;
	}
	default:
		noct_error(env, N_TR("Value is not a number or a string."));
		return false;
	}

	return true;
}

/* push() */
static bool
rt_intrin_push(
	NoctEnv *env)
{
	struct rt_value arr, val;
	uint32_t size;

	noct_pin_local(env, 2, &arr, &val);

	if (!noct_get_arg_check_array(env, 0, &arr))
		return false;
	if (!noct_get_arg(env, 1, &val))
		return false;

	if (!noct_get_array_size(env, &arr, &size))
		return false;
	if (!noct_set_array_elem(env, &arr, size, &val))
		return false;
	if (!noct_set_return(env, &arr))
		return false;

	return true;
}

/* pop() */
static bool
rt_intrin_pop(
	NoctEnv *env)
{
	struct rt_value arr, val;
	uint32_t size;

	noct_pin_local(env, 2, &arr, &val);

	if (!noct_get_arg_check_array(env, 0, &arr))
		return false;

	if (!noct_get_array_size(env, &arr, &size))
		return false;
	if (size == 0) {
		noct_error(env, N_TR("Empty array."));
		return false;
	}

	if (!noct_get_array_elem(env, &arr, size - 1, &val))
		return false;
	if (!noct_resize_array(env, &arr, size - 1))
		return false;

	if (!noct_set_return(env, &val))
		return false;

	return true;
}

/* newArray() */
static bool
rt_intrin_newArray(
	NoctEnv *env)
{
	struct rt_value arr, size;
	int size_i;

	noct_pin_local(env, 1, &size);

	if (!noct_get_arg_check_int(env, 0, &size, &size_i))
		return false;

	if (!noct_make_empty_array(env, &arr))
		return false;
	if (!noct_resize_array(env, &arr, size_i))
		return false;
	if (!noct_set_return(env, &arr))
		return false;

	return true;
}

/* resize() */
static bool
rt_intrin_resize(
	NoctEnv *env)
{
	struct rt_value arr, size;
	int size_i;

	noct_pin_local(env, 2, &arr, &size);

	if (!noct_get_arg_check_array(env, 0, &arr))
		return false;
	if (!noct_get_arg_check_int(env, 1, &size, &size_i))
		return false;

	if (!noct_resize_array(env, &arr, size_i))
		return false;

	return true;
}

size_t
get_string_length(
	const char *text)
{
	const char *s;
	size_t i;

	s = text;
	i = 0;
	while (*s != '\0') {
		uint32_t codepoint;
		int mblen;

		mblen = utf8_to_utf32(s, &codepoint);
		if (mblen <= 0) {
			/* UTF-8 error. */
			return 0;
		}

		i++;
		s += mblen;
	}

	return i;
}

/* charCount() */
static bool
rt_intrin_charCount(
	NoctEnv *env)
{
	NoctValue str, ret;
	const char *str_s;
	int len;

	noct_pin_local(env, 2, &str, &ret);

	if (!noct_get_arg_check_string(env, 0, &str, &str_s))
		return false;

	len = (int)get_string_length(str_s);

	if (!noct_set_return_make_int(env, &ret, len))
		return false;

	return true;
}

/* charAt() */
static bool
rt_intrin_charAt(
	NoctEnv *env)
{
	NoctValue str, index, ret;
	const char *str_s;
	int index_i;
	size_t len;
	const char *s;
	char d[8];
	int i, ofs;

	noct_pin_local(env, 3, &str, &index, &ret);

	if (!noct_get_arg_check_string(env, 0, &str, &str_s))
		return false;
	if (!noct_get_arg_check_int(env, 1, &index, &index_i))
		return false;

	len = get_string_length(str_s);

	if (index_i < 0) {
		if (!noct_make_string(env, &ret, ""))
			return false;
		if (!noct_set_return(env, &ret))
			return false;
		return true;
	}

	s = str_s;
	i = 0;
	ofs = 0;
	while (*s != '\0' && i <= index_i) {
		uint32_t codepoint;
		int mblen;

		mblen = utf8_to_utf32(s, &codepoint);
		if (mblen <= 0) {
			/* UTF-8 error. */
			break;
		}

		if (i == index_i) {
			/* Succeeded. */
			strncpy(d, &str_s[ofs], mblen);
			d[mblen] = '\0';
			if (!noct_set_return_make_string(env, &ret, d))
				return false;
			return true;
		}

		s += mblen;
		ofs += mblen;
		i++;
	}

	/* Failed. */
	if (!noct_make_string(env, &ret, ""))
		return false;
	if (!noct_set_return(env, &ret))
		return false;

	noct_unpin_local(env, 3, &str, &index, &ret);

	return true;
}

/* substring() */
static bool
rt_intrin_substring(
	NoctEnv *env)
{
	NoctValue str, start, len, ret;
	const char *str_s;
	int start_i, len_i, i, ofs, copy_start, copy_mblen;
	const char *s;
	char *tmp;

	if (!noct_get_arg_check_string(env, 0, &str, &str_s))
		return false;
	if (!noct_get_arg_check_int(env, 1, &start, &start_i))
		return false;
	if (!noct_get_arg_check_int(env, 2, &len, &len_i))
		return false;

	if (start_i < 0)
		start_i = 0;

	s = str_s;
	i = 0;
	ofs = 0;
	copy_start = -1;
	copy_mblen = 0;
	while (*s != '\0') {
		uint32_t codepoint;
		int mblen;

		mblen = utf8_to_utf32(s, &codepoint);
		if (mblen <= 0) {
			/* UTF-8 error. */
			break;
		}
		if (i == start_i)
			copy_start = ofs;
		if (i == (size_t)start_i + (size_t)len_i)
			break;
		if (copy_start != -1)
			copy_mblen += mblen;

		s += mblen;
		ofs += mblen;
		i++;
	}

	tmp = noct_malloc((size_t)copy_mblen + 1);
	if (tmp == NULL) {
		rt_out_of_memory(env);
		return false;
	}

	strncpy(tmp, str_s + copy_start, (size_t)copy_mblen);
	tmp[copy_mblen] = '\0';

	if (!noct_set_return_make_string(env, &ret, tmp))
		return false;

	noct_free(tmp);

	return true;
}

/* Get a top character of a utf-8 string as a utf-32. */
static int utf8_to_utf32(const char *mbs, uint32_t *wc)
{
	size_t mbslen, octets, i;
	uint32_t ret;

	assert(mbs != NULL);

	/* If mbs is empty. */
	mbslen = strlen(mbs);
	if(mbslen == 0)
		return 0;

	/* Check the first byte, get an octet count. */
	if (mbs[0] == '\0')
		octets = 0;
	else if ((mbs[0] & 0x80) == 0)
		octets = 1;
	else if ((mbs[0] & 0xe0) == 0xc0)
		octets = 2;
	else if ((mbs[0] & 0xf0) == 0xe0)
		octets = 3;
	else if ((mbs[0] & 0xf8) == 0xf0)
		octets = 4;
	else
		return -1;	/* Not suppoerted. */

	/* Check the mbs length. */
	if (mbslen < octets)
		return -1;	/* mbs is too short. */

	/* Check for 2-4 bytes. */
	for (i = 1; i < octets; i++) {
		if((mbs[i] & 0xc0) != 0x80)
			return -1;	/* Non-understandable */
	}

	/* Compose a utf-32 character. */
	switch (octets) {
	case 0:
		ret = 0;
		break;
	case 1:
		ret = (uint32_t)mbs[0];
		break;
	case 2:
		ret = (uint32_t)(((mbs[0] & 0x1f) << 6) |
				 (mbs[1] & 0x3f));
		break;
	case 3:
		ret = (uint32_t)(((mbs[0] & 0x0f) << 12) |
				 ((mbs[1] & 0x3f) << 6) |
				 (mbs[2] & 0x3f));
		break;
	case 4:
		ret = (uint32_t)(((mbs[0] & 0x07) << 18) |
				 ((mbs[1] & 0x3f) << 12) |
				 ((mbs[2] & 0x3f) << 6) |
				 (mbs[3] & 0x3f));
		break;
	default:
		/* never come here */
		assert(0);
		return -1;
	}

	/* Store the result. */
	if(wc != NULL)
		*wc = ret;

	/* Return the octet count. */
	return (int)octets;
}

/* isset() */
static bool
rt_intrin_isset(
	struct rt_env *env)
{
	NoctValue arr, val, ret;
	const char *val_s;
	bool is_key_available;

	noct_pin_local(env, 3, &arr, &val, &ret);

	if (!noct_get_arg_check_dict(env, 0, &arr))
		return false;
	if (!noct_get_arg_check_string(env, 1, &val, &val_s))
		return false;

	if (!noct_check_dict_key(env, &arr, val_s, &is_key_available))
		return false;

	if (!noct_set_return_make_int(env, &ret, is_key_available ? 1 : 0))
		return false;

	return true;
}

/* unset() */
static bool
rt_intrin_unset(
	struct rt_env *env)
{
	NoctValue arr, val;
	const char *val_s;

	noct_pin_local(env, 2, &arr, &val);

	if (!noct_get_arg_check_dict(env, 0, &arr))
		return false;
	if (!noct_get_arg_check_string(env, 1, &val, &val_s))
		return false;

	if (!noct_remove_dict_elem(env, &arr, val_s))
		return false;

	return true;
}

/* fast_gc() */
static bool
rt_intrin_fast_gc(
	NoctEnv *env)
{
	noct_fast_gc(env);
	return true;
}

/* full_gc() */
static bool
rt_intrin_full_gc(
	NoctEnv *env)
{
	noct_full_gc(env);
	return true;
}

/* compact_gc() */
static bool
rt_intrin_compact_gc(
	NoctEnv *env)
{
	noct_compact_gc(env);
	return true;
}
