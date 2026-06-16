/* -*- coding: utf-8; tab-width: 8; indent-tabs-mode: t; -*- */

/*
 * Noct Programming Language
 * Copyright (c) 2025, 2026, Awe Morris
 */

/*
 * CLI: Native API routines
 */

#include "cli-main.h"

/*
 * NAPI Functions
 */

/* NAPI function implementation. */
static bool cfunc_print(NoctEnv *env);
static bool serialize_printer(NoctEnv *env, char *buf, size_t size, NoctValue *value, bool is_inside_obj);

/* NAPU table. */
struct napi_item {
	const char *name;
	uint32_t param_count;
	const char *param[NOCT_ARG_MAX];
	bool (*cfunc)(NoctEnv *env);
} napi_items[] = {
	{"print", 1, {"msg"}, cfunc_print},
};

/*
 * Register NAPI functions.
 */
bool register_cli_cfunc(NoctEnv *env)
{
	int i;

	for (i = 0; i < (int)(sizeof(napi_items) / sizeof(struct napi_item)); i++) {
		if (!noct_register_cfunc(env,
					 napi_items[i].name,
					 napi_items[i].param_count,
					 napi_items[i].param,
					 napi_items[i].cfunc,
					 NULL))
			return false;
	}

	return true;
}

/* Implementation of print() */
static bool
cfunc_print(
	NoctEnv *env)
{
	char buf[8192];
	NoctValue value;

	if (!noct_pin_local(env, 1, &value))
		return false;

	if (!noct_get_arg(env, 0, &value)) {
		noct_unpin_local(env, 1, &value);
		return false;
	}

	memset(buf, 0, sizeof(buf));
	serialize_printer(env, buf, sizeof(buf), &value, false);

	wide_printf("%s\n", buf);

	noct_unpin_local(env, 1, &value);

	return true;
}
	
static bool serialize_printer(
	NoctEnv *env,
	char *buf,
	size_t size,
	NoctValue *value,
	bool is_inside_obj)
{
	int type;
	int ival;
	int64_t lval;
	float fval;
	double lfval;
	const char *sval;
	size_t items, i;
	char digits[1024];

	if (!noct_get_value_type(env, value, &type))
		return false;

	switch (type) {
	case NOCT_VALUE_INT:
		if (!noct_get_int(env, value, &ival))
			return false;
		snprintf(digits, sizeof(digits), "%d", ival);
		strncat(buf, digits, size);
		break;
	case NOCT_VALUE_LONG:
		if (!noct_get_long(env, value, &lval))
			return false;
		snprintf(digits, sizeof(digits), "%" PRId64, lval);
		strncat(buf, digits, size);
		break;
	case NOCT_VALUE_FLOAT:
		if (!noct_get_float(env, value, &fval))
			return false;
		snprintf(digits, sizeof(digits), "%.7g", fval);
		strncat(buf, digits, size);
		break;
	case NOCT_VALUE_DOUBLE:
		if (!noct_get_double(env, value, &lfval))
			return false;
		snprintf(digits, sizeof(digits), "%.15g", lfval);
		strncat(buf, digits, size);
		break;
	case NOCT_VALUE_STRING:
		if (!noct_get_string(env, value, &sval))
			return false;
		if (is_inside_obj)
			strncat(buf, "\"", size);
		strncat(buf, sval, size);
		if (is_inside_obj)
			strncat(buf, "\"", size);
		break;
	case NOCT_VALUE_ARRAY:
		if (!noct_get_array_size(env, value, &items))
			return false;
		strncat(buf, "[", size);
		for (i = 0; i < items; i++) {
			NoctValue elem;
			if (!noct_get_array_elem(env, value, i, &elem))
				return false;
			if (!serialize_printer(env, buf, size, &elem, true))
				return false;
			if (i != items - 1)
				strncat(buf, ", ", size);
		}
		strncat(buf, "]", size);
		break;
	case NOCT_VALUE_DICT:
		if (!noct_get_dict_size(env, value, &items))
			return false;
		strncat(buf, "{", size);
		for (i = 0; i < items; i++) {
			NoctValue k, v;
			if (!noct_get_dict_key_by_index(env, value, i, &k))
				return false;
			if (!noct_get_string(env, &k, &sval))
				return false;
			strncat(buf, sval, size);
			strncat(buf, ": ", size);
			if (!noct_get_dict_value_by_index(env, value, i, &v))
				return false;
			serialize_printer(env, buf, size, &v, true);
			if (i != items - 1)
				strncat(buf, ", ", size);
		}
		strncat(buf, "}", size);
		break;
	case NOCT_VALUE_FUNC:
		strncat(buf, "<func>", size);
		break;
	default:
		assert(0);
		break;
	}

	return true;
}
