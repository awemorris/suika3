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
static void printer_append(char *buf, size_t size, const char *text);

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
	int type;
	const char *text;

	if (!noct_pin_local(env, 1, &value))
		return false;

	if (!noct_get_arg(env, 0, &value)) {
		noct_unpin_local(env, 1, &value);
		return false;
	}
	/* Do not force a top-level string through the diagnostic object buffer. */
	if (!noct_get_value_type(env, &value, &type)) {
		noct_unpin_local(env, 1, &value);
		return false;
	}
	if (type == NOCT_VALUE_STRING) {
		if (!noct_get_string(env, &value, &text)) {
			noct_unpin_local(env, 1, &value);
			return false;
		}
#if defined(_WIN32)
		wide_printf("%s\n", text);
#else
		printf("%s\n", text);
#endif
		noct_unpin_local(env, 1, &value);
		return true;
	}

	memset(buf, 0, sizeof(buf));
	serialize_printer(env, buf, sizeof(buf), &value, false);

	wide_printf("%s\n", buf);

	noct_unpin_local(env, 1, &value);

	return true;
}

static void
printer_append(
	char *buf,
	size_t size,
	const char *text)
{
	size_t used;

	if (size == 0)
		return;
	used = strlen(buf);
	if (used >= size - 1)
		return;
	strncat(buf, text, size - used - 1);
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
		printer_append(buf, size, digits);
		break;
	case NOCT_VALUE_LONG:
		if (!noct_get_long(env, value, &lval))
			return false;
		snprintf(digits, sizeof(digits), "%" PRId64, lval);
		printer_append(buf, size, digits);
		break;
	case NOCT_VALUE_FLOAT:
		if (!noct_get_float(env, value, &fval))
			return false;
		snprintf(digits, sizeof(digits), "%.7g", fval);
		printer_append(buf, size, digits);
		break;
	case NOCT_VALUE_DOUBLE:
		if (!noct_get_double(env, value, &lfval))
			return false;
		snprintf(digits, sizeof(digits), "%.15g", lfval);
		printer_append(buf, size, digits);
		break;
	case NOCT_VALUE_STRING:
		if (!noct_get_string(env, value, &sval))
			return false;
		if (is_inside_obj)
			printer_append(buf, size, "\"");
		printer_append(buf, size, sval);
		if (is_inside_obj)
			printer_append(buf, size, "\"");
		break;
	case NOCT_VALUE_ARRAY:
		if (!noct_get_array_size(env, value, &items))
			return false;
		printer_append(buf, size, "[");
		for (i = 0; i < items; i++) {
			NoctValue elem;
			if (!noct_get_array_elem(env, value, i, &elem))
				return false;
			if (!serialize_printer(env, buf, size, &elem, true))
				return false;
			if (i != items - 1)
				printer_append(buf, size, ", ");
		}
		printer_append(buf, size, "]");
		break;
	case NOCT_VALUE_DICT:
		if (!noct_get_dict_size(env, value, &items))
			return false;
		printer_append(buf, size, "{");
		for (i = 0; i < items; i++) {
			NoctValue k, v;
			if (!noct_get_dict_by_index(env, value, i, &k, &v))
				return false;
			if (!noct_get_string(env, &k, &sval))
				return false;
			printer_append(buf, size, sval);
			printer_append(buf, size, ": ");
			serialize_printer(env, buf, size, &v, true);
			if (i != items - 1)
				printer_append(buf, size, ", ");
		}
		printer_append(buf, size, "}");
		break;
	case NOCT_VALUE_FUNC:
		printer_append(buf, size, "<func>");
		break;
	default:
		assert(0);
		break;
	}

	return true;
}
