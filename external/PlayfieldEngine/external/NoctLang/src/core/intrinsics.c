/* -*- coding: utf-8; tab-width: 8; indent-tabs-mode: t; -*- */

/*
 * Noct Programming Language
 * Copyright (c) 2025, 2026, Awe Morris
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
#include <limits.h>
#include <assert.h>

#define NEVER_COME_HERE		0

static bool rt_intrin_Int_from(NoctEnv *env);
static bool rt_intrin_Long_from(NoctEnv *env);
static bool rt_intrin_Float_from(NoctEnv *env);
static bool rt_intrin_Double_from(NoctEnv *env);
static bool rt_intrin_String_from(NoctEnv *env);
static bool rt_intrin_String_charCount(NoctEnv *env);
static bool rt_intrin_String_charAt(NoctEnv *env);
static bool rt_intrin_String_substring(NoctEnv *env);
static bool rt_intrin_String_indexOf(NoctEnv *env);
static bool rt_intrin_Array_make(NoctEnv *env);
static bool rt_intrin_Array_size(NoctEnv *env);
static bool rt_intrin_Array_push(NoctEnv *env);
static bool rt_intrin_Array_pop(NoctEnv *env);
static bool rt_intrin_Array_resize(NoctEnv *env);
static bool rt_intrin_Array_copy(NoctEnv *env);
static bool rt_intrin_Dict_make(NoctEnv *env);
static bool rt_intrin_Dict_merge(NoctEnv *env);
static bool rt_intrin_Dict_size(NoctEnv *env);
static bool rt_intrin_Dict_hasKey(NoctEnv *env);
static bool rt_intrin_Dict_remove(NoctEnv *env);
static bool rt_intrin_Dict_copy(NoctEnv *env);
static bool rt_intrin_Packed_int8(NoctEnv *env);
static bool rt_intrin_Packed_int16(NoctEnv *env);
static bool rt_intrin_Packed_int32(NoctEnv *env);
static bool rt_intrin_Packed_int64(NoctEnv *env);
static bool rt_intrin_Packed_uint8(NoctEnv *env);
static bool rt_intrin_Packed_uint16(NoctEnv *env);
static bool rt_intrin_Packed_uint32(NoctEnv *env);
static bool rt_intrin_Packed_uint64(NoctEnv *env);
static bool rt_intrin_Packed_float32(NoctEnv *env);
static bool rt_intrin_Packed_float64(NoctEnv *env);
static bool rt_intrin_Packed_size(NoctEnv *env);
static bool rt_intrin_Packed_type(NoctEnv *env);
static bool rt_intrin_Math_abs(NoctEnv *env);
static bool rt_intrin_Math_sqrt(NoctEnv *env);
static bool rt_intrin_Math_sin(NoctEnv *env);
static bool rt_intrin_Math_cos(NoctEnv *env);
static bool rt_intrin_Math_tan(NoctEnv *env);
static bool rt_intrin_Math_random(NoctEnv *env);
static bool rt_intrin_Global_hasVariable(NoctEnv *env);
static bool rt_intrin_GC_youngGC(NoctEnv *env);
static bool rt_intrin_GC_oldGC(NoctEnv *env);
static bool rt_intrin_GC_compactGC(NoctEnv *env);

struct intrin_item {
	const char *package_name;
	const char *field_name;
	const char *reg_name;
	bool (*cfunc)(struct rt_env *rt);
	uint32_t param_count;
	const char *param[NOCT_ARG_MAX];
} intrin_items[] = {
	{"Int",		"from",		"Int.from",		rt_intrin_Int_from,		1, {"val"}},
	{"Long",	"from",		"Long.from",		rt_intrin_Long_from,		1, {"val"}},
	{"Float",	"from",		"Float.from",		rt_intrin_Float_from,		1, {"val"}},
	{"Double",	"from",		"Double.from",		rt_intrin_Double_from,		1, {"val"}},
	{"String",	"from",		"String.from",		rt_intrin_String_from,		1, {"val"}},
	{"String",	"charCount",	"String.charCount",	rt_intrin_String_charCount,	1, {"s"}},
	{"String",	"charAt",	"String.charAt",	rt_intrin_String_charAt,	2, {"s", "index"}},
	{"String",	"substring",	"String.substring",	rt_intrin_String_substring,	3, {"s", "start", "len"}},
	{"String",	"indexOf",	"String.indexOf",	rt_intrin_String_indexOf,	2, {"s", "search"}},
	{"Array",	"make",		"Array.make",		rt_intrin_Array_make,		1, {"size"}},
	{"Array",	"size",		"Array.size",		rt_intrin_Array_size,		1, {"arr"}},
	{"Array",	"push",		"Array.push",		rt_intrin_Array_push,		2, {"arr", "val"}},
	{"Array",	"pop",		"Array.pop",		rt_intrin_Array_pop,		1, {"arr"}},
	{"Array",	"resize",	"Array.resize",		rt_intrin_Array_resize,		2, {"arr", "size"}},
	{"Array",	"copy",		"Array.copy",		rt_intrin_Array_copy,		1, {"arr"}},
	{"Dict",	"make",		"Dict.make",		rt_intrin_Dict_make,		0, {NULL}},
	{"Dict",	"merge",	"Dict.merge",		rt_intrin_Dict_merge,		2, {"src1", "src2"}},
	{"Dict",	"size",		"Dict.size",		rt_intrin_Dict_size,		1, {"dict"}},
	{"Dict",	"hasKey",	"Dict.hasKey",		rt_intrin_Dict_hasKey,		2, {"dict", "key"}},
	{"Dict",	"remove",	"Dict.remove",		rt_intrin_Dict_remove,		2, {"dict", "key"}},
	{"Dict",	"copy",		"Dict.copy",		rt_intrin_Dict_copy,		1, {"dict"}},
	{"Packed",	"int8",		"Packed.int8",		rt_intrin_Packed_int8,		1, {"size"}},
	{"Packed",	"int16",	"Packed.int16",		rt_intrin_Packed_int16,		1, {"size"}},
	{"Packed",	"int32",	"Packed.int32",		rt_intrin_Packed_int32,		1, {"size"}},
	{"Packed",	"int64",	"Packed.int64",		rt_intrin_Packed_int64,		1, {"size"}},
	{"Packed",	"uint8",	"Packed.uint8",		rt_intrin_Packed_uint8,		1, {"size"}},
	{"Packed",	"uint16",	"Packed.uint16",	rt_intrin_Packed_uint16,	1, {"size"}},
	{"Packed",	"uint32",	"Packed.uint32",	rt_intrin_Packed_uint32,	1, {"size"}},
	{"Packed",	"uint64",	"Packed.uint64",	rt_intrin_Packed_uint64,	1, {"size"}},
	{"Packed",	"float32",	"Packed.float32",	rt_intrin_Packed_float32,	1, {"size"}},
	{"Packed",	"float64",	"Packed.float64",	rt_intrin_Packed_float64,	1, {"size"}},
	{"Packed",	"size",		"Packed.size",		rt_intrin_Packed_size,		1, {"packed"}},
	{"Packed",	"type",		"Packed.type",		rt_intrin_Packed_type,		1, {"packed"}},
	{"Math",	"abs",		"Math.abs",		rt_intrin_Math_abs,		1, {"x"}},
	{"Math",	"sqrt",		"Math.sqrt",		rt_intrin_Math_sqrt,		1, {"x"}},
	{"Math",	"sin",		"Math.sin",		rt_intrin_Math_sin,		1, {"x"}},
	{"Math",	"cos",		"Math.cos",		rt_intrin_Math_cos,		1, {"x"}},
	{"Math",	"tan",		"Math.tan",		rt_intrin_Math_tan,		1, {"x"}},
	{"Math",	"random",	"Math.random",		rt_intrin_Math_random,		0, {NULL}},
	{"Global",	"isSet",	"Global.isSet",		rt_intrin_Global_hasVariable,	1, {"name"}},
	{"GC",		"youngGC",	"GC.youngGC",		rt_intrin_GC_youngGC,		0, {NULL}},
	{"GC",		"oldGC",	"GC.oldGC",		rt_intrin_GC_oldGC,		0, {NULL}},
	{"GC",		"compactGC",	"GC.compactGC",		rt_intrin_GC_compactGC,		0, {NULL}},
};

static size_t get_string_length(const char *text);
static int utf8_to_utf32(const char *mbs, uint32_t *wc);

bool
rt_register_intrinsics(
	struct rt_env *env)
{
	struct rt_value pkg;
	struct rt_func *func;
	const char *last_pkg_name;
	struct rt_value val;
	int i;

	/* For each table entry in intrin_items: */
	last_pkg_name = NULL;
	for (i = 0; i < (int)(sizeof(intrin_items) / sizeof(struct intrin_item)); i++) {
		/* Register a function at the entry. */
		if (!noct_register_cfunc(env,
					 intrin_items[i].reg_name,
					 intrin_items[i].param_count,
					 intrin_items[i].param,
					 intrin_items[i].cfunc,
					 &func))
			return false;

		/* If the function is not in a package. */
		if (intrin_items[i].package_name == NULL)
			continue;

		/* Load the package. */
		if (last_pkg_name == NULL ||
		    strcmp(intrin_items[i].package_name, last_pkg_name) != 0) {
			last_pkg_name = intrin_items[i].package_name;
			if (!rt_check_global(env, last_pkg_name)) {
				if (!rt_make_empty_dict(env, &pkg))
					return false;
				if (!rt_set_global(env, last_pkg_name, &pkg))
					return false;
			} else {
				if (!rt_get_global(env, last_pkg_name, &pkg))
					return false;
			}
		}

		/* Add the function to the package */
		val.type = NOCT_VALUE_FUNC;
		val.val.func = func;
		if (!rt_set_dict_elem_cstr(env, &pkg, intrin_items[i].field_name, &val))
			return false;
	}

	return true;
}

/*
 * Int.from(val)
 */
static bool
rt_intrin_Int_from(
	NoctEnv *env)
{
	struct rt_value val, ret;
	int type;

	/* Pin the local variables to prevent them collected by GC. */
	noct_pin_local(env, 2, &val, &ret);

	/* Get the argument "val". */
	if (!noct_get_arg(env, 0, &val))
		return false;

	/* Check the value type for "val". */
	if (!noct_get_value_type(env, &val, &type))
		return false;

	/* Branch by the type. */
	switch (type) {
	case NOCT_VALUE_INT:
		/* If it is an int, just return it. */
		if (!noct_set_return(env, &val))
			return false;
		break;
	case NOCT_VALUE_LONG:
	{
		/* If it is a long, trancate it. */
		int64_t val_l;
		if (!noct_get_long(env, &val, &val_l))
			return false;
		if (!noct_set_return_make_int(env, &ret, (int32_t)(uint32_t)val_l))
			return false;
		break;
	}
	case NOCT_VALUE_FLOAT:
	{
		/* If it is a float, floor it. */
		float val_f;
		if (!noct_get_float(env, &val, &val_f))
			return false;
		if (!noct_set_return_make_int(env, &ret, (int32_t)val_f))
			return false;
		break;
	}
	case NOCT_VALUE_DOUBLE:
	{
		/* If it is a double, floor it. */
		double val_lf;
		if (!noct_get_double(env, &val, &val_lf))
			return false;
		if (!noct_set_return_make_int(env, &ret, (int32_t)val_lf))
			return false;
		break;
	}
	case NOCT_VALUE_STRING:
	{
		/* If it is a string, call atoi(). */
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

	/* Unpin the local variables to allow them collected by GC. */
	noct_unpin_local(env, 2, &val, &ret);

	return true;
}

/*
 * Long.from(val)
 */
static bool
rt_intrin_Long_from(
	NoctEnv *env)
{
	struct rt_value val, ret;
	int type;

	/* Pin the local variables to prevent them collected by GC. */
	noct_pin_local(env, 2, &val, &ret);

	/* Get the argument "val". */
	if (!noct_get_arg(env, 0, &val))
		return false;

	/* Check the value type for "val". */
	if (!noct_get_value_type(env, &val, &type))
		return false;

	/* Branch by the type. */
	switch (type) {
	case NOCT_VALUE_INT:
	{
		/* If it is an int, widen it. */
		int val_i;
		if (!noct_get_int(env, &val, &val_i))
			return false;
		if (!noct_set_return_make_long(env, &ret, val_i))
			return false;
		break;
	}
	case NOCT_VALUE_LONG:
		/* If it is a long, just return it. */
		if (!noct_set_return(env, &val))
			return false;
		break;
	case NOCT_VALUE_FLOAT:
	{
		/* If it is a float, floor it. */
		float val_f;
		if (!noct_get_float(env, &val, &val_f))
			return false;
		if (!noct_set_return_make_long(env, &ret, (int64_t)val_f))
			return false;
		break;
	}
	case NOCT_VALUE_DOUBLE:
	{
		/* If it is a double, floor it. */
		double val_lf;
		if (!noct_get_double(env, &val, &val_lf))
			return false;
		if (!noct_set_return_make_long(env, &ret, (int64_t)val_lf))
			return false;
		break;
	}
	case NOCT_VALUE_STRING:
	{
		/* If it is a string, call atoi(). */
		const char *val_s;
		if (!noct_get_string(env, &val, &val_s))
			return false;
		if (!noct_set_return_make_long(env, &ret, atol(val_s)))
			return false;
		break;
	}
	default:
		noct_error(env, N_TR("Value is not a number or a string."));
		return false;
	}

	/* Unpin the local variables to allow them collected by GC. */
	noct_unpin_local(env, 2, &val, &ret);

	return true;
}

/*
 * Float.from(val)
 */
static bool
rt_intrin_Float_from(
	NoctEnv *env)
{
	struct rt_value val, ret;
	int type;

	/* Pin the local variables to prevent them collected by GC. */
	noct_pin_local(env, 2, &val, &ret);

	/* Get the argument "val". */
	if (!noct_get_arg(env, 0, &val))
		return false;

	/* Check the value type for "val". */
	if (!noct_get_value_type(env, &val, &type))
		return false;

	/* Branch by the type. */
	switch (type) {
	case NOCT_VALUE_INT:
	{
		/* If it is an int, convert to float. */
		int val_i;
		if (!noct_get_int(env, &val, &val_i))
			return false;
		if (!noct_set_return_make_float(env, &ret, (float)val_i))
			return false;
		break;
	}
	case NOCT_VALUE_LONG:
	{
		/* If it is a long, convert to float. */
		int64_t val_l;
		if (!noct_get_long(env, &val, &val_l))
			return false;
		if (!noct_set_return_make_float(env, &ret, (float)val_l))
			return false;
		break;
	}
	case NOCT_VALUE_FLOAT:
		/* If it is a float, just set it as a return value. */
		if (!noct_set_return(env, &val))
			return false;
		break;
	case NOCT_VALUE_DOUBLE:
	{
		/* If it is a double, floor it. */
		double val_lf;
		if (!noct_get_double(env, &val, &val_lf))
			return false;
		if (!noct_set_return_make_float(env, &ret, (float)val_lf))
			return false;
		break;
	}
	case NOCT_VALUE_STRING:
	{
		/* If it is a string, call atoi(). */
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

	/* Unpin the local variables to allow them collected by GC. */
	noct_unpin_local(env, 2, &val, &ret);

	return true;
}

/*
 * Double.from(val)
 */
static bool
rt_intrin_Double_from(
	NoctEnv *env)
{
	struct rt_value val, ret;
	int type;

	/* Pin the local variables to prevent them collected by GC. */
	noct_pin_local(env, 2, &val, &ret);

	/* Get the argument "val". */
	if (!noct_get_arg(env, 0, &val))
		return false;

	/* Check the value type for "val". */
	if (!noct_get_value_type(env, &val, &type))
		return false;

	/* Branch by the type. */
	switch (type) {
	case NOCT_VALUE_INT:
	{
		/* If it is an int, convert to double. */
		int val_i;
		if (!noct_get_int(env, &val, &val_i))
			return false;
		if (!noct_set_return_make_double(env, &ret, (double)val_i))
			return false;
		break;
	}
	case NOCT_VALUE_LONG:
	{
		/* If it is a long, convert to float. */
		int64_t val_l;
		if (!noct_get_long(env, &val, &val_l))
			return false;
		if (!noct_set_return_make_double(env, &ret, (double)val_l))
			return false;
		break;
	}
	case NOCT_VALUE_FLOAT:
	{
		/* If it is a float, convert to double. */
		float val_f;
		if (!noct_get_float(env, &val, &val_f))
			return false;
		if (!noct_set_return_make_double(env, &ret, (double)val_f))
			return false;
		break;
	}
	case NOCT_VALUE_DOUBLE:
		/* If it is a double, just set it as a return value. */
		if (!noct_set_return(env, &val))
			return false;
		break;
	case NOCT_VALUE_STRING:
	{
		/* If it is a string, call atoi(). */
		const char *val_s;
		if (!noct_get_string(env, &val, &val_s))
			return false;
		if (!noct_set_return_make_double(env, &ret, atof(val_s)))
			return false;
		break;
	}
	default:
		noct_error(env, N_TR("Value is not a number or a string."));
		return false;
	}

	/* Unpin the local variables to allow them collected by GC. */
	noct_unpin_local(env, 2, &val, &ret);

	return true;
}

/*
 * String.from(val)
 */
static bool
rt_intrin_String_from(
	NoctEnv *env)
{
	struct rt_value val, ret;
	int type;
	char buf[128];

	/* Pin the local variables to prevent them collected by GC. */
	noct_pin_local(env, 2, &val, &ret);

	/* Get the argument "val". */
	if (!noct_get_arg(env, 0, &val))
		return false;

	/* Check the value type for "val". */
	if (!noct_get_value_type(env, &val, &type))
		return false;

	/* Branch by the type. */
	switch (type) {
	case NOCT_VALUE_INT:
	{
		/* If it is an int, convert to double. */
		int val_i;
		if (!noct_get_int(env, &val, &val_i))
			return false;
		snprintf(buf, sizeof(buf), "%d", val_i);
		if (!noct_set_return_make_string(env, &ret, buf))
			return false;
		break;
	}
	case NOCT_VALUE_LONG:
	{
		/* If it is a long, convert to float. */
		int64_t val_l;
		if (!noct_get_long(env, &val, &val_l))
			return false;
		if (!noct_set_return_make_double(env, &ret, (double)val_l))
			return false;
		break;
	}
	case NOCT_VALUE_FLOAT:
	{
		/* If it is a float, convert to double. */
		float val_f;
		if (!noct_get_float(env, &val, &val_f))
			return false;
		if (!noct_set_return_make_double(env, &ret, (double)val_f))
			return false;
		break;
	}
	case NOCT_VALUE_DOUBLE:
		/* If it is a double, just set it as a return value. */
		if (!noct_set_return(env, &val))
			return false;
		break;
	case NOCT_VALUE_STRING:
	{
		/* If it is a string, call atoi(). */
		const char *val_s;
		if (!noct_get_string(env, &val, &val_s))
			return false;
		if (!noct_set_return_make_double(env, &ret, atof(val_s)))
			return false;
		break;
	}
	default:
		noct_error(env, N_TR("Value is not a number or a string."));
		return false;
	}

	/* Unpin the local variables to allow them collected by GC. */
	noct_unpin_local(env, 2, &val, &ret);

	return true;
}

/*
 * String.charCount(s)
 */
static bool
rt_intrin_String_charCount
(NoctEnv *env)
{
	NoctValue str, ret;
	const char *str_s;
	size_t len;

	/* Pin the local variables to prevent them collected by GC. */
	noct_pin_local(env, 2, &str, &ret);

	/* Get the arg "str". */
	if (!noct_get_arg_check_string(env, 0, &str, &str_s))
		return false;

	/* Get the length of str. */
	len = get_string_length(str_s);

	/* Set the length as a return value. */
	if (!noct_set_return_make_int_long(env, &ret, len))
		return false;

	/* Unpin the local variables to allow them collected by GC. */
	noct_unpin_local(env, 2, &str, &ret);

	return true;
}

/*
 * String.charAt(s, i)
 */
static bool
rt_intrin_String_charAt(
	NoctEnv *env)
{
	NoctValue str, index, ret;
	const char *str_s;
	size_t index_i;
	const char *s;
	char d[8];
	size_t i, ofs;
	int mblen;

	noct_pin_local(env, 3, &str, &index, &ret);

	/* Get the argument "s". */
	if (!noct_get_arg_check_string(env, 0, &str, &str_s))
		return false;

	/* Get the argument "i". */
	if (!noct_get_arg_check_int_long(env, 1, &index, &index_i))
		return false;

	/*
         * Iterate through the UTF-8 string to locate and extract the
         * character at 'index_i'. Handles multi-byte characters
         * correctly using utf8_to_utf32().
         */
	s = str_s;
	i = 0;
	ofs = 0;
	d[0] = '\0';
	while (*s != '\0' && i <= index_i) {
		uint32_t codepoint;

		mblen = utf8_to_utf32(s, &codepoint);
		if (mblen <= 0) {
			/* UTF-8 error. */
			return false;
		}

		if (i == index_i) {
			/* Succeeded. */
			strncpy(d, &str_s[ofs], (size_t)mblen);
			d[mblen] = '\0';
			break;
		}

		s += mblen;
		ofs += (uint32_t)mblen;
		i++;
	}

	/* Set the string as a return value. */
	if (!noct_set_return_make_string(env, &ret, d))
		return false;

	noct_unpin_local(env, 3, &str, &index, &ret);

	return true;
}

/*
 * String.substring(s, start, len)
 */
static bool
rt_intrin_String_substring(
	NoctEnv *env)
{
	NoctValue str, start, len, ret;
	const char *str_s;
	size_t start_i, len_i, i, ofs, copy_start, copy_mblen;
	const char *s;
	char *tmp;

	noct_pin_local(env, 4, &str, &start, &len, &ret);

	/* Get the argument "s". */
	if (!noct_get_arg_check_string(env, 0, &str, &str_s))
		return false;

	/* Get the argument "start". */
	if (!noct_get_arg_check_int_long(env, 1, &start, &start_i))
		return false;

	/* Get the argument "len". */
	if (!noct_get_arg_check_int_long(env, 2, &len, &len_i))
		return false;
	if (len_i == (size_t)-1)
		len_i = LONG_MAX;

	/* Search the position (start_i) and (start_i + len). */
	s = str_s;
	i = 0;
	ofs = 0;
	copy_start = (size_t)-1;
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
		if (i == start_i + len_i)
			break;
		if (copy_start != (size_t)-1)
			copy_mblen += (size_t)mblen;

		s += mblen;
		ofs += (size_t)mblen;
		i++;
	}

	/* Allocate a buffer for the new string. */
	tmp = noct_malloc((size_t)copy_mblen + 1);
	if (tmp == NULL) {
		rt_out_of_memory(env);
		return false;
	}

	/* Make a string. */
	strncpy(tmp, str_s + copy_start, (size_t)copy_mblen);
	tmp[copy_mblen] = '\0';

	/* Make a return string value. */
	if (!noct_set_return_make_string(env, &ret, tmp))
		return false;

	/* Free the buffer. */
	noct_free(tmp);

	noct_unpin_local(env, 4, &str, &start, &len, &ret);

	return true;
}

/*
 * String.indexOf()
 */
static bool
rt_intrin_String_indexOf(
	NoctEnv *env)
{
	NoctValue str, substr, ret;
	const char *str_s;
	const char *substr_s;
	size_t i, len_str, len_substr, range_max;
	int result;

	noct_pin_local(env, 3, &str, &substr, &ret);

	/* Get the argument "s". */
	if (!noct_get_arg_check_string(env, 0, &str, &str_s))
		return false;

	/* Get the argument "search". */
	if (!noct_get_arg_check_string(env, 1, &substr, &substr_s))
		return false;

	/* Do search. */
	len_str = strlen(str_s);
	len_substr = strlen(substr_s);
	result = -1;
	if (len_str > len_substr) {
		range_max = len_str - len_substr;
		for (i = 0; i < range_max; i++) {
			if (strncmp(str_s + i, substr_s, len_substr) == 0) {
				result = (int)i;
				break;
			}
		}
	}

	/* Set the return value. */
	if (!noct_set_return_make_int(env, &ret, result))
		return false;

	noct_unpin_local(env, 3, &str, &substr, &ret);

	return true;
}

/* Get the Unicode character count of a string. */
static size_t
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

/*
 * Array.make(size)
 */
static bool
rt_intrin_Array_make(
	NoctEnv *env)
{
	struct rt_value arr, size;
	int size_i;

	noct_pin_local(env, 2, &arr, &size);

	/* Retrieve the "size" argument from the first parameter (index 0). */
	if (!noct_get_arg_check_int(env, 0, &size, &size_i))
		return false;

	/* Initialize a new empty array object. */
	if (!noct_make_empty_array(env, &arr))
		return false;

	/* Allocate the requested capacity for the array. */
	if (!noct_resize_array(env, &arr, (uint32_t)size_i))
		return false;

	/* Set the allocated array as the function's return value. */
	if (!noct_set_return(env, &arr))
		return false;

	noct_unpin_local(env, 2, &arr, &size);

	return true;
}

/*
 * Array.size(arr)
 */
static bool
rt_intrin_Array_size(
	NoctEnv *env)
{
	struct rt_value arr, ret;
	size_t size;

	noct_pin_local(env, 2, &arr, &ret);

	/* Retrieve the "arr" argument from the first parameter (index 0). */
	if (!noct_get_arg_check_array(env, 0, &arr))
		return false;

	/* Get the current size of the array. */
	if (!noct_get_array_size(env, &arr, &size))
		return false;

	/* Set the return value. */
	if (!noct_set_return_make_int_long(env, &ret, size))
		return false;

	noct_unpin_local(env, 2, &arr, &ret);

	return true;
}

/*
 * Array.push(arr, val)
 */
static bool
rt_intrin_Array_push(
	NoctEnv *env)
{
	struct rt_value arr, val;
	size_t size;

	noct_pin_local(env, 2, &arr, &val);

	/* Retrieve the "arr" argument from the first parameter (index 0). */
	if (!noct_get_arg_check_array(env, 0, &arr))
		return false;

	/* Retrieve the "val" argument from the first parameter (index 1). */
	if (!noct_get_arg(env, 1, &val))
		return false;

	/* Get the current size of the array to determine the tail index. */
	if (!noct_get_array_size(env, &arr, &size))
		return false;

	/* Append "val" to the end of the array. */
	if (!noct_set_array_elem(env, &arr, size, &val))
		return false;

	/* Set the modified array as the return value. */
	if (!noct_set_return(env, &arr))
		return false;

	noct_unpin_local(env, 2, &arr, &val);

	return true;
}

/*
 * Array.pop(arr)
 */
static bool
rt_intrin_Array_pop(
	NoctEnv *env)
{
	struct rt_value arr, val;
	size_t size;

	noct_pin_local(env, 2, &arr, &val);

	/* Retrieve the "arr" argument from the first parameter (index 0). */
	if (!noct_get_arg_check_array(env, 0, &arr))
		return false;

	/* Get the current size of the array to determine the tail index. */
	if (!noct_get_array_size(env, &arr, &size))
		return false;
	if (size == 0) {
		/* Error: arr is empty, cannot pop. */
		noct_error(env, N_TR("Empty array."));
		return false;
	}

	/* Get the tail element to be popped.*/
	if (!noct_get_array_elem(env, &arr, size - 1, &val))
		return false;

	/* Shrink the array to (size - 1). */
	if (!noct_resize_array(env, &arr, size - 1))
		return false;

	/* Set the popped value as the return value. */
	if (!noct_set_return(env, &val))
		return false;

	noct_unpin_local(env, 2, &arr, &val);

	return true;
}

/*
 * Array.resize(arr, size)
 */
static bool
rt_intrin_Array_resize(
	NoctEnv *env)
{
	struct rt_value arr, size;
	size_t size_i;

	noct_pin_local(env, 2, &arr, &size);

	/* Retrieve the argument "arr" at the index 0. */
	if (!noct_get_arg_check_array(env, 0, &arr))
		return false;

	/* Retrieve the argument "size" at the index 1. */
	if (!noct_get_arg_check_int_long(env, 1, &size, &size_i))
		return false;

	/* Resize the array "arr". */
	if (!noct_resize_array(env, &arr, size_i))
		return false;

	/* Set the modified array as the return value. */
	if (!noct_set_return(env, &arr))
		return false;

	noct_unpin_local(env, 2, &arr, &size);

	return true;
}

/*
 * Array.copy(arr)
 */
static bool
rt_intrin_Array_copy(
	NoctEnv *env)
{
	struct rt_value arr, ret;

	noct_pin_local(env, 2, &arr, &ret);

	/* Retrieve the argument "arr" at the index 0. */
	if (!noct_get_arg_check_array(env, 0, &arr))
		return false;

	/* Make a copy of the array. */
	if (!noct_make_array_copy(env, &ret, &arr))
		return false;

	/* Set the new array as the return value. */
	if (!noct_set_return(env, &ret))
		return false;

	noct_unpin_local(env, 2, &arr, &ret);

	return true;
}

/*
 * Dict.make()
 */
static bool
rt_intrin_Dict_make(
	NoctEnv *env)
{
	struct rt_value ret;

	noct_pin_local(env, 1, &ret);

	/* Make an empty dictionary. */
	if (!noct_make_empty_dict(env, &ret))
		return false;

	/* Set the new dictionary as the return value. */
	if (!noct_set_return(env, &ret))
		return false;

	noct_pin_local(env, 1, &ret);

	return true;
}

/*
 * Dict.merge(src1, src2)
 */
static bool
rt_intrin_Dict_merge(
	NoctEnv *env)
{
	struct rt_value src1, src2, ret;

	noct_pin_local(env, 3, &src1, &src2, &ret);

	/* Retrieve the argument "src1`" at the index 0. */
	if (!noct_get_arg_check_dict(env, 0, &src1))
		return false;

	/* Retrieve the argument "b" at the index 1. */
	if (!noct_get_arg_check_dict(env, 1, &src2))
		return false;

	/* Merge "cls" and "init" dictionaries into "ret". */
	if (!noct_merge_dict(env, &ret, &src1, &src2))
		return false;

	/* Set the merged dictionary as the return value. */
	if (!noct_set_return(env, &ret))
		return false;

	noct_unpin_local(env, 3, &src1, &src2, &ret);

	return true;
}

/*
 * Dict.size(dict)
 */
static bool
rt_intrin_Dict_size(
	struct rt_env *env)
{
	NoctValue dict, ret;
	size_t size;

	noct_pin_local(env, 2, &dict, &ret);

	/* Retrieve the argument "dict" at the index 0. */
	if (!noct_get_arg_check_dict(env, 0, &dict))
		return false;

	/* Get the size. */
	if (!noct_get_dict_size(env, &dict, &size))
		return false;

	/* Set the return value. */
	if (!noct_set_return_make_int_long(env, &ret, size))
		return false;

	noct_unpin_local(env, 2, &dict, &ret);

	return true;
}

/*
 * Dict.hasKey(dict, key)
 */
static bool
rt_intrin_Dict_hasKey(
	struct rt_env *env)
{
	NoctValue dict, key, ret;
	const char *key_s;
	bool is_key_available;

	noct_pin_local(env, 3, &dict, &key, &ret);

	/* Retrieve the argument "dict" at the index 0. */
	if (!noct_get_arg_check_dict(env, 0, &dict))
		return false;

	/* Retrieve the argument "key" at the index 1. */
	if (!noct_get_arg_check_string(env, 1, &key, &key_s))
		return false;

	/* Check for the key. */
	if (!noct_check_dict_key(env, &dict, &key, &is_key_available))
		return false;

	/* Set the return value. */
	if (!noct_set_return_make_int(env, &ret, is_key_available ? 1 : 0))
		return false;

	noct_unpin_local(env, 3, &dict, &key, &ret);

	return true;
}

/*
 * Dict.remove(dict, key)
 */
static bool
rt_intrin_Dict_remove(
	struct rt_env *env)
{
	NoctValue dict, key;
	const char *key_s;

	noct_pin_local(env, 2, &dict, &key);

	/* Retrieve the argument "dict" at the index 0. */
	if (!noct_get_arg_check_dict(env, 0, &dict))
		return false;

	/* Retrieve the argument "key" at the index 1. */
	if (!noct_get_arg_check_string(env, 1, &key, &key_s))
		return false;

	/* Remove the key. */
	if (!noct_remove_dict_elem(env, &dict, &key))
		return false;

	noct_unpin_local(env, 2, &dict, &key);

	return true;
}

/*
 * Dict.copy(dict)
 */
static bool
rt_intrin_Dict_copy(
	struct rt_env *env)
{
	NoctValue dict, ret;

	noct_pin_local(env, 2, &dict, &ret);

	/* Retrieve the argument "dict" at the index 0. */
	if (!noct_get_arg_check_dict(env, 0, &dict))
		return false;

	/* Make a copy of the dicttionary. */
	if (!noct_make_dict_copy(env, &ret, &dict))
		return false;

	/* Set the return value. */
	if (!noct_set_return(env, &ret))
		return false;

	noct_unpin_local(env, 2, &dict, &ret);

	return true;
}

/*
 * Packed.int8()
 */
static bool
rt_intrin_Packed_int8(
	NoctEnv *env)
{
	NoctValue v_size, v_ret;
	uint32_t i_size;

	noct_pin_local(env, 2, &v_size, &v_ret);

	if (!noct_get_arg_check_int(env, 0, &v_size, (int *)&i_size))
		return false;

	if (i_size == 0) {
		noct_error(env, "Packed size is 0.");
		return false;
	}

	if (!noct_make_packed(env, &v_ret, NOCT_PACKED_INT8, i_size, i_size, NULL))
		return false;
	if (!noct_set_return(env, &v_ret))
		return false;

	noct_unpin_local(env, 2, &v_size, &v_ret);

	return true;
}

/*
 * Packed.uint8()
 */
static bool
rt_intrin_Packed_uint8(
	NoctEnv *env)
{
	NoctValue v_size, v_ret;
	size_t i_size;

	noct_pin_local(env, 2, &v_size, &v_ret);

	if (!noct_get_arg(env, 0, &v_size))
		return false;
	if (!noct_get_size_t(env, &v_size, &i_size))
		return false;
	if (i_size == 0) {
		noct_error(env, "Packed size is 0.");
		return false;
	}

	if (!noct_make_packed(env, &v_ret, NOCT_PACKED_UINT8, i_size, i_size, NULL))
		return false;
	if (!noct_set_return(env, &v_ret))
		return false;

	noct_unpin_local(env, 2, &v_size, &v_ret);

	return true;
}

/*
 * Packed.int16()
 */
static bool
rt_intrin_Packed_int16(
	NoctEnv *env)
{
	NoctValue v_size, v_ret;
	size_t i_size;

	noct_pin_local(env, 2, &v_size, &v_ret);

	if (!noct_get_arg(env, 0, &v_size))
		return false;
	if (!noct_get_size_t(env, &v_size, &i_size))
		return false;
	if (i_size == 0) {
		noct_error(env, "Packed size is 0.");
		return false;
	}

	if (!noct_make_packed(env, &v_ret, NOCT_PACKED_INT16, i_size * 2, i_size, NULL))
		return false;
	if (!noct_set_return(env, &v_ret))
		return false;

	noct_unpin_local(env, 2, &v_size, &v_ret);

	return true;
}

/*
 * Packed.uint16()
 */
static bool
rt_intrin_Packed_uint16(
	NoctEnv *env)
{
	NoctValue v_size, v_ret;
	size_t i_size;

	noct_pin_local(env, 2, &v_size, &v_ret);

	if (!noct_get_arg(env, 0, &v_size))
		return false;
	if (!noct_get_size_t(env, &v_size, &i_size))
		return false;
	if (i_size == 0) {
		noct_error(env, "Packed size is 0.");
		return false;
	}

	if (!noct_make_packed(env, &v_ret, NOCT_PACKED_UINT16, i_size * 2, i_size, NULL))
		return false;
	if (!noct_set_return(env, &v_ret))
		return false;

	noct_unpin_local(env, 2, &v_size, &v_ret);

	return true;
}

/*
 * Packed.int32()
 */
static bool
rt_intrin_Packed_int32(
	NoctEnv *env)
{
	NoctValue v_size, v_ret;
	size_t i_size;

	noct_pin_local(env, 2, &v_size, &v_ret);

	if (!noct_get_arg(env, 0, &v_size))
		return false;
	if (!noct_get_size_t(env, &v_size, &i_size))
		return false;
	if (i_size == 0) {
		noct_error(env, "Packed size is 0.");
		return false;
	}

	if (!noct_make_packed(env, &v_ret, NOCT_PACKED_INT32, i_size * 4, i_size, NULL))
		return false;
	if (!noct_set_return(env, &v_ret))
		return false;

	noct_unpin_local(env, 2, &v_size, &v_ret);

	return true;
}

/*
 * Packed.uint32()
 */
static bool
rt_intrin_Packed_uint32(
	NoctEnv *env)
{
	NoctValue v_size, v_ret;
	size_t i_size;

	noct_pin_local(env, 2, &v_size, &v_ret);

	if (!noct_get_arg(env, 0, &v_size))
		return false;
	if (!noct_get_size_t(env, &v_size, &i_size))
		return false;
	if (i_size == 0) {
		noct_error(env, "Packed size is 0.");
		return false;
	}

	if (!noct_make_packed(env, &v_ret, NOCT_PACKED_UINT32, i_size * 4, i_size, NULL))
		return false;
	if (!noct_set_return(env, &v_ret))
		return false;

	noct_unpin_local(env, 2, &v_size, &v_ret);

	return true;
}

/*
 * Packed.int64()
 */
static bool
rt_intrin_Packed_int64(
	NoctEnv *env)
{
	NoctValue v_size, v_ret;
	size_t i_size;

	noct_pin_local(env, 2, &v_size, &v_ret);

	if (!noct_get_arg(env, 0, &v_size))
		return false;
	if (!noct_get_size_t(env, &v_size, &i_size))
		return false;
	if (i_size == 0) {
		noct_error(env, "Packed size is 0.");
		return false;
	}

	if (!noct_make_packed(env, &v_ret, NOCT_PACKED_INT64, i_size * 8, i_size, NULL))
		return false;
	if (!noct_set_return(env, &v_ret))
		return false;

	noct_unpin_local(env, 2, &v_size, &v_ret);

	return true;
}

/*
 * Packed.uint64()
 */
static bool
rt_intrin_Packed_uint64(
	NoctEnv *env)
{
	NoctValue v_size, v_ret;
	size_t i_size;

	noct_pin_local(env, 2, &v_size, &v_ret);

	if (!noct_get_arg(env, 0, &v_size))
		return false;
	if (!noct_get_size_t(env, &v_size, &i_size))
		return false;
	if (i_size == 0) {
		noct_error(env, "Packed size is 0.");
		return false;
	}

	if (!noct_make_packed(env, &v_ret, NOCT_PACKED_UINT64, i_size * 8, i_size, NULL))
		return false;
	if (!noct_set_return(env, &v_ret))
		return false;

	noct_unpin_local(env, 2, &v_size, &v_ret);

	return true;
}

/*
 * Packed.float32()
 */
static bool
rt_intrin_Packed_float32(
	NoctEnv *env)
{
	NoctValue v_size, v_ret;
	size_t i_size;

	noct_pin_local(env, 2, &v_size, &v_ret);

	if (!noct_get_arg(env, 0, &v_size))
		return false;
	if (!noct_get_size_t(env, &v_size, &i_size))
		return false;
	if (i_size == 0) {
		noct_error(env, "Packed size is 0.");
		return false;
	}

	if (!noct_make_packed(env, &v_ret, NOCT_PACKED_FLOAT32, i_size * 4, i_size, NULL))
		return false;
	if (!noct_set_return(env, &v_ret))
		return false;

	noct_unpin_local(env, 2, &v_size, &v_ret);

	return true;
}

/*
 * Packed.float64()
 */
static bool
rt_intrin_Packed_float64(
	NoctEnv *env)
{
	NoctValue v_size, v_ret;
	size_t i_size;

	noct_pin_local(env, 2, &v_size, &v_ret);

	if (!noct_get_arg(env, 0, &v_size))
		return false;
	if (!noct_get_size_t(env, &v_size, &i_size))
		return false;
	if (i_size == 0) {
		noct_error(env, "Packed size is 0.");
		return false;
	}

	if (!noct_make_packed(env, &v_ret, NOCT_PACKED_FLOAT32, i_size * 8, i_size, NULL))
		return false;
	if (!noct_set_return(env, &v_ret))
		return false;

	noct_unpin_local(env, 2, &v_size, &v_ret);

	return true;
}

/*
 * Packed.size(packed)
 */
static bool
rt_intrin_Packed_size(
	struct rt_env *env)
{
	NoctValue packed, ret;
	size_t size;

	noct_pin_local(env, 2, &packed, &ret);

	/* Retrieve the argument "packed" at the index 0. */
	if (!noct_get_arg_check_packed(env, 0, &packed, NOCT_PACKED_ANY))
		return false;

	/* Get the size. */
	if (!noct_get_packed_size(env, &packed, &size))
		return false;

	/* Set the return value. */
	if (!noct_set_return_make_int_long(env, &ret, size))
		return false;

	noct_unpin_local(env, 2, &packed, &ret);

	return true;
}

/*
 * Packed.type(packed)
 */
static bool
rt_intrin_Packed_type(
	struct rt_env *env)
{
	NoctValue packed, ret;
	int type;

	noct_pin_local(env, 2, &packed, &ret);

	/* Retrieve the argument "packed" at the index 0. */
	if (!noct_get_arg_check_packed(env, 0, &packed, NOCT_PACKED_ANY))
		return false;

	/* Get the type. */
	if (!noct_get_packed_type(env, &packed, &type))
		return false;

	/* Set the return value. */
	switch (type) {
	case NOCT_PACKED_INT8:
		if (!noct_set_return_make_string(env, &ret, "int8"))
			return false;
		break;
	case NOCT_PACKED_INT16:
		if (!noct_set_return_make_string(env, &ret, "int16"))
			return false;
		break;
	case NOCT_PACKED_INT32:
		if (!noct_set_return_make_string(env, &ret, "int32"))
			return false;
		break;
	case NOCT_PACKED_INT64:
		if (!noct_set_return_make_string(env, &ret, "int64"))
			return false;
		break;
	case NOCT_PACKED_UINT8:
		if (!noct_set_return_make_string(env, &ret, "uint8"))
			return false;
		break;
	case NOCT_PACKED_UINT16:
		if (!noct_set_return_make_string(env, &ret, "uint16"))
			return false;
		break;
	case NOCT_PACKED_UINT32:
		if (!noct_set_return_make_string(env, &ret, "uint32"))
			return false;
		break;
	case NOCT_PACKED_UINT64:
		if (!noct_set_return_make_string(env, &ret, "uint64"))
			return false;
		break;
	default:
		assert(0);
		if (!noct_set_return_make_string(env, &ret, "(error)"))
			return false;
		break;
	}

	noct_unpin_local(env, 2, &packed, &ret);

	return true;
}

/*
 * Math.abs()
 */
static bool
rt_intrin_Math_abs(
	NoctEnv *env)
{
	NoctValue x, ret;
	int type;
	int ival;
	float fval;

	noct_pin_local(env, 2, &x, &ret);

	if (!noct_get_arg(env, 0, &x))
		return false;
	if (!noct_get_value_type(env, &x, &type))
		return false;

	switch (type) {
	case NOCT_VALUE_INT:
		if (!noct_get_int(env, &x, &ival))
			return false;
		if (ival < 0)
			ival = -ival;
		if (!noct_set_return_make_int(env, &ret, ival))
			return false;
		break;
	case NOCT_VALUE_FLOAT:
		if (!noct_get_float(env, &x, &fval))
			return false;
		if (fval < 0)
			fval = -fval;
		if (!noct_set_return_make_float(env, &ret, fval))
			return false;
		break;
	default:
		noct_error(env, N_TR("Value is not a number."));
		return false;
	}

	noct_unpin_local(env, 2, &x, &ret);

	return true;
}

/* Math.sqrt() */
static bool
rt_intrin_Math_sqrt(
	NoctEnv *env)
{
	NoctValue x, ret;
	int type;
	int ival;
	float fval;

	noct_pin_local(env, 2, &x, &ret);

	if (!noct_get_arg(env, 0, &x))
		return false;
	if (!noct_get_value_type(env, &x, &type))
		return false;

	switch (type) {
	case NOCT_VALUE_INT:
		if (!noct_get_int(env, &x, &ival))
			return false;
		if (!noct_set_return_make_float(env, &ret, sqrtf((float)ival)))
			return false;
		break;
	case NOCT_VALUE_FLOAT:
		if (!noct_get_float(env, &x, &fval))
			return false;
		if (!noct_set_return_make_float(env, &ret, sqrtf(fval)))
			return false;
		break;
	default:
		noct_error(env, N_TR("Value is not a number."));
		return false;
	}

	noct_unpin_local(env, 2, &x, &ret);

	return true;
}

/*
 * Math.sin()
 */
static bool
rt_intrin_Math_sin(
	NoctEnv *env)
{
	NoctValue x, ret;
	float x_f;
	float sinx;

	noct_pin_local(env, 2, &x, &ret);

	if (!noct_get_arg_check_float(env, 0, &x, &x_f))
		return false;

	sinx = sinf(x_f);

	if (!noct_set_return_make_float(env, &ret, sinx))
		return false;

	noct_unpin_local(env, 2, &x, &ret);

	return true;
}

/*
 * Math.cos()
 */
static bool
rt_intrin_Math_cos(
	NoctEnv *env)
{
	NoctValue x, ret;
	float x_f;
	float cosx;

	noct_pin_local(env, 2, &x, &ret);

	if (!noct_get_arg_check_float(env, 0, &x, &x_f))
		return false;

	cosx = cosf(x_f);

	if (!noct_set_return_make_float(env, &ret, cosx))
		return false;

	noct_unpin_local(env, 2, &x, &ret);

	return true;
}

/*
 * Math.tan()
 */
static bool
rt_intrin_Math_tan(
	NoctEnv *env)
{
	NoctValue x, ret;
	float x_f;
	float tanx;

	noct_pin_local(env, 2, &x, &ret);

	if (!noct_get_arg_check_float(env, 0, &x, &x_f))
		return false;

	tanx = tanf(x_f);

	if (!noct_set_return_make_float(env, &ret, tanx))
		return false;

	noct_unpin_local(env, 2, &x, &ret);

	return true;
}

/*
 * Math.random()
 */
static bool
rt_intrin_Math_random(
	NoctEnv *env)
{
	NoctValue ret;
	float r;

	noct_pin_local(env, 1, &ret);

	r = (float)rand() / (float)RAND_MAX;

	if (!noct_set_return_make_float(env, &ret, r))
		return false;

	noct_unpin_local(env, 1, &ret);

	return true;
}

/*
 * Global.hasVariable(name)
 */
static bool
rt_intrin_Global_hasVariable(
	NoctEnv *env)
{
	NoctValue name, ret;
	const char *name_s;
	bool has_var;

	noct_pin_local(env, 2, &name, &ret);

	/* Retrieve the argument "name" at the index 0. */
	if (!noct_get_arg_check_string(env, 0, &name, &name_s))
		return false;

	/* Check for the variable. */
	if (!noct_check_global(env, name_s, &has_var))
		return false;

	/* Set the return value. */
	if (!noct_set_return_make_int(env, &ret, has_var ? 1 : 0))
		return false;

	noct_unpin_local(env, 2, &name, &ret);

	return true;
}

/*
 * GC.youngGC()
 */
static bool
rt_intrin_GC_youngGC(
	NoctEnv *env)
{
	noct_fast_gc(env);
	return true;
}

/*
 * GC.oldGC()
 */
static bool
rt_intrin_GC_oldGC(
	NoctEnv *env)
{
	noct_full_gc(env);
	return true;
}

/*
 * GC.compactGC()
 */
static bool
rt_intrin_GC_compactGC(
	NoctEnv *env)
{
	noct_compact_gc(env);
	return true;
}


