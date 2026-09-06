/* -*- coding: utf-8; tab-width: 8; indent-tabs-mode: t; -*- */

/*
 * Noct Programming Language
 * Copyright (C) 2025, 2026 Awe Morris
 *
 * SPDX-License-Identifier: Zlib
 */

/*
 * The Console API.
 */

#include <noct/noct.h>

#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
#endif

#define NEVER_COME_HERE		(0)

static const char *console_print_param[NOCT_ARG_MAX] = {
	"msg"
};

static bool cfunc_Console_print(NoctEnv *env);
static bool serialize_printer(NoctEnv *env, char *buf, size_t size, NoctValue *value, bool is_inside_obj);
static int console_wide_printf(const char *format, ...);

/*
 * Registers the Console API functions.
 */
NOCT_DLL
bool
noct_register_api_console(
	NoctEnv *env)
{
	NoctValue dict;
	NoctValue funcval;

	/* Creates the global Console dictionary. */
	if (!noct_make_empty_dict(env, &dict))
		return false;

	/* Publishes the empty Console dictionary. */
	if (!noct_set_global(env, "Console", &dict))
		return false;

	/* Registers the native Console.print function. */
	if (!noct_register_cfunc(
		env,
		"Console.print",
		1,
		console_print_param,
		cfunc_Console_print,
		NULL)) {
		return false;
	}

	/* Reads the registered function value. */
	if (!noct_get_global(env, "Console.print", &funcval))
		return false;

	/* Publishes the function in the Console dictionary. */
	if (!noct_set_dict_elem_cstr(
		env,
		&dict,
		"print",
		&funcval)) {
		return false;
	}

	/* Reports successful Console API registration. */
	return true;
}

/* Implements Console.print(). */
static bool
cfunc_Console_print(
	NoctEnv *env)
{
	char buf[8192];
	NoctValue value;

	/* Pins the argument while its value is serialized. */
	if (!noct_pin_local(env, 1, &value))
		return false;

	/* Reads the value to print. */
	if (!noct_get_arg(env, 0, &value)) {
		/* Releases the argument root after a failed lookup. */
		noct_unpin_local(env, 1, &value);

		/* Reports the argument lookup failure. */
		return false;
	}

	/* Serializes the value through the existing best-effort path. */
	memset(buf, 0, sizeof(buf));
	(void)serialize_printer(
		env,
		buf,
		sizeof(buf),
		&value,
		false);

	/* Writes the serialized value and its trailing newline. */
	(void)console_wide_printf("%s\n", buf);

	/* Releases the rooted argument after printing. */
	noct_unpin_local(env, 1, &value);

	/* Reports a completed print operation. */
	return true;
}

/* Serializes one value into the bounded print buffer. */
static bool
serialize_printer(
	NoctEnv *env,
	char *buf,
	size_t size,
	NoctValue *value,
	bool is_inside_obj)
{
	NoctValue elem;
	NoctValue key;
	NoctValue dict_value;
	const char *string_value;
	char digits[1024];
	double double_value;
	float float_value;
	int64_t long_value;
	size_t item_count;
	uint32_t i;
	int int_value;
	int type;

	/* Reads the value representation to serialize. */
	if (!noct_get_value_type(env, value, &type))
		return false;

	/* Serializes the value according to its runtime type. */
	switch (type) {
	case NOCT_VALUE_INT:
		/* Reads the integer value. */
		if (!noct_get_int(env, value, &int_value))
			return false;

		/* Formats and appends the integer value. */
		snprintf(digits, sizeof(digits), "%d", int_value);
		strncat(buf, digits, size - strlen(buf) - 1);
		break;
	case NOCT_VALUE_LONG:
		/* Reads the long integer value. */
		if (!noct_get_long(env, value, &long_value))
			return false;

		/* Formats and appends the long integer value. */
		snprintf(digits, sizeof(digits), "%" PRId64, long_value);
		strncat(buf, digits, size - strlen(buf) - 1);
		break;
	case NOCT_VALUE_FLOAT:
		/* Reads the single-precision value. */
		if (!noct_get_float(env, value, &float_value))
			return false;

		/* Formats and appends the single-precision value. */
		snprintf(digits, sizeof(digits), "%.7g", float_value);
		strncat(buf, digits, size - strlen(buf) - 1);
		break;
	case NOCT_VALUE_DOUBLE:
		/* Reads the double-precision value. */
		if (!noct_get_double(env, value, &double_value))
			return false;

		/* Formats and appends the double-precision value. */
		snprintf(digits, sizeof(digits), "%.15g", double_value);
		strncat(buf, digits, size - strlen(buf) - 1);
		break;
	case NOCT_VALUE_STRING:
		/* Reads the string value. */
		if (!noct_get_string(env, value, &string_value))
			return false;

		/* Opens a quoted string when it belongs to a container. */
		if (is_inside_obj) {
			strncat(buf, "\"", size - strlen(buf) - 1);
		}

		/* Appends the string bytes without escaping them. */
		strncat(buf, string_value, size - strlen(buf) - 1);

		/* Closes a quoted string when it belongs to a container. */
		if (is_inside_obj) {
			strncat(buf, "\"", size - strlen(buf) - 1);
		}
		break;
	case NOCT_VALUE_ARRAY:
		/* Reads the array extent. */
		if (!noct_get_array_size(env, value, &item_count))
			return false;

		/* Opens the array representation. */
		strncat(buf, "[", size - strlen(buf) - 1);

		/* Serializes every array element in index order. */
		for (i = 0; i < item_count; i++) {
			/* Reads the next array element. */
			if (!noct_get_array_elem(env, value, i, &elem))
				return false;

			/* Serializes the nested array element. */
			if (!serialize_printer(
				env,
				buf,
				size,
				&elem,
				true)) {
				return false;
			}

			/* Separates this element from the next element. */
			if (i != item_count - 1) {
				strncat(buf, ", ", size);
			}
		}

		/* Closes the array representation. */
		strncat(buf, "]", size - strlen(buf) - 1);
		break;
	case NOCT_VALUE_DICT:
		/* Reads the dictionary extent. */
		if (!noct_get_dict_size(env, value, &item_count))
			return false;

		/* Opens the dictionary representation. */
		strncat(buf, "{", size - strlen(buf) - 1);

		/* Serializes every dictionary entry in storage order. */
		for (i = 0; i < item_count; i++) {
			/* Reads the next dictionary key and value. */
			if (!noct_get_dict_by_index(
				env,
				value,
				i,
				&key,
				&dict_value)) {
				return false;
			}

			/* Reads the dictionary key as a string. */
			if (!noct_get_string(env, &key, &string_value))
				return false;

			/* Appends the key and its separator. */
			strncat(buf, string_value, size - strlen(buf) - 1);
			strncat(buf, ": ", size - strlen(buf) - 1);

			/* Serializes the value through the existing best-effort path. */
			(void)serialize_printer(
				env,
				buf,
				size,
				&dict_value,
				true);

			/* Separates this entry from the next entry. */
			if (i != item_count - 1) {
				strncat(buf, ", ", size - strlen(buf) - 1);
			}
		}

		/* Closes the dictionary representation. */
		strncat(buf, "}", size - strlen(buf) - 1);
		break;
	case NOCT_VALUE_FUNC:
		/* Appends the opaque function representation. */
		strncat(buf, "<func>", size - strlen(buf) - 1);
		break;
	default:
		/* Rejects an unknown runtime value type in debug builds. */
		assert(NEVER_COME_HERE);
		break;
	}

	/* Reports successful serialization. */
	return true;
}

/* Writes formatted UTF-8 text to the platform console. */
static int
console_wide_printf(
	const char *format,
	...)
{
	static char buf[4096];
#if defined(_WIN32)
	static wchar_t wide_buf[4096];
	DWORD written;
	HANDLE output;
	int wide_length;
#endif
	va_list ap;
	int size;

	/* Formats the complete console message in UTF-8. */
	va_start(ap, format);
	size = vsnprintf(buf, sizeof(buf), format, ap);
	va_end(ap);

#if !defined(_WIN32)
	/* Writes the UTF-8 message to the standard output stream. */
	printf("%s", buf);

	/* Reports the formatter's byte count. */
	return size;
#else
	/* Converts the UTF-8 message into the fixed Windows console buffer. */
	memset(wide_buf, 0, sizeof(wide_buf));
	(void)MultiByteToWideChar(
		CP_UTF8,
		0,
		buf,
		-1,
		wide_buf,
		(int)(sizeof(wide_buf) / sizeof(wchar_t)));

	/* Resolves the output console and converted character count. */
	output = GetStdHandle(STD_OUTPUT_HANDLE);
	wide_length = lstrlenW(wide_buf);

	/* Writes the converted message through the Windows console API. */
	(void)WriteConsoleW(
		output,
		wide_buf,
		(DWORD)wide_length,
		&written,
		NULL);

	/* Reports the formatter's UTF-8 byte count. */
	return size;
#endif
}
