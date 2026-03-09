/* -*- coding: utf-8; tab-width: 8; indent-tabs-mode: t; -*- */

/*
 * Copyright (c) 2025, Awe Morris. All rights reserved.
 */

/*
 * API: Console.*
 */

#include <noct/noct.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <assert.h>

#ifdef _WIN32
#include <windows.h>
#endif

#define NEVER_COME_HERE		(0)

/* Forward declaration. */
static bool cfunc_Console_print(NoctEnv *env);
static bool serialize_printer(NoctEnv *env, char *buf, size_t size, NoctValue *value, bool is_inside_obj);
static int console_wide_printf(const char *format, ...);

/* FFI table. */
struct ffi_item {
	const char *global_name;
	const char *field_name;
	int param_count;
	const char *param[NOCT_ARG_MAX];
	bool (*cfunc)(NoctEnv *env);
};
static struct ffi_item ffi_items[] = {
	{"__Console_print",	"print",	1,	{"msg"},	cfunc_Console_print},
};

	
/*
 * Register "Console.*" functions.
 */
bool noct_register_api_console(NoctEnv *env)
{
	NoctValue dict;
	int i;

	/* Make a global variable "System". */
	if (!noct_make_empty_dict(env, &dict))
		return false;
	if (!noct_set_global(env, "Console", &dict))
		return false;

	/* Register functions. */
	for (i = 0; i < sizeof(ffi_items) / sizeof(struct ffi_item); i++) {
		NoctValue funcval;

		/* Register a cfunc. */
		if (!noct_register_cfunc(
			    env,
			    ffi_items[i].global_name,
			    1,
			    ffi_items[i].param,
			    ffi_items[i].cfunc,
			    NULL))
			return false;

		/* Get a function value. */
		if (!noct_get_global(env, ffi_items[i].global_name, &funcval))
			return false;

		/* Make a dictionary element. */
		if (!noct_set_dict_elem(
			    env,
			    &dict,
			    ffi_items[i].field_name,
			    &funcval))
			return false;
	}

	return true;
}

/* Implementation of Console.print() */
static bool
cfunc_Console_print(
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

	console_wide_printf("%s\n", buf);

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
	float fval;
	const char *sval;
	int items;
	int i;
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
	case NOCT_VALUE_FLOAT:
		if (!noct_get_float(env, value, &fval))
			return false;
		snprintf(digits, sizeof(digits), "%f", fval);
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
		assert(NEVER_COME_HERE);
		break;
	}

	return true;
}

/* Print to console. (supports wide characters on Windows console.) */
static int console_wide_printf(const char *format, ...)
{
	static char buf[4096];
	va_list ap;
	int size;

	va_start(ap, format);
	size = vsnprintf(buf, sizeof(buf), format, ap);
	va_end(ap);

#if !defined(_WIN32)
	return printf("%s", buf);
#else
	/* MSVC or MinGW: Use WriteConsoleW(). (Otherwise we can't write CJK.) */
	static wchar_t wbuf[4096];
	DWORD dwWritten;
	memset(wbuf, 0, sizeof(wbuf));
	MultiByteToWideChar(CP_UTF8, 0, buf, -1, wbuf, sizeof(wbuf) / sizeof(wchar_t));
	WriteConsoleW(GetStdHandle(STD_OUTPUT_HANDLE), wbuf, lstrlenW(wbuf), &dwWritten, NULL);
	return size;
#endif
}
