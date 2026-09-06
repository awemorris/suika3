/* -*- coding: utf-8; tab-width: 8; indent-tabs-mode: t; -*- */

/*
 * Noct Programming Language
 * Copyright (c) 2025, 2026, Awe Morris
 */

#define _POSIX_C_SOURCE 200809L

#include <noct/noct.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/* Forward declarations. */
static int require_global(NoctEnv *env, const char *name);
static int test_fileutil_euc_jp(NoctEnv *env);
static int test_regex_api_boundary(NoctEnv *env);

/*
 * Run the public API boundary tests.
 */
int
main(
	void)
{
	NoctVM *vm;
	NoctEnv *env;

	if (!noct_create_vm(&vm, &env, NULL))
		return 10;
	if (!test_regex_api_boundary(env))
		return 11;
	if (!noct_register_api_file(env))
		return 12;
	if (!noct_register_api_term(env))
		return 12;
	if (!require_global(env, "File"))
		return 13;
	if (!require_global(env, "FileUtil"))
		return 13;
	if (!require_global(env, "Term"))
		return 13;
	if (!test_fileutil_euc_jp(env))
		return 14;
	if (!noct_destroy_vm(vm))
		return 15;

	puts("Public Regex API boundary: PASS");
	puts("Public File/Term API registration: PASS");
	puts("Public FileUtil EUC-JP decoding: PASS");

	return 0;
}

/* Check whether one public global is registered. */
static int
require_global(
	NoctEnv *env,
	const char *name)
{
	NoctValue value;

	memset(&value, 0, sizeof(value));
	if (!noct_get_global(env, name, &value))
		return 0;

	return 1;
}

/* Check public EUC-JP file decoding. */
static int
test_fileutil_euc_jp(
	NoctEnv *env)
{
	static const unsigned char encoded[] = {
		'A', 0xa4, 0xa2, 0x8e, 0xb1
	};
	static const char expected[] = "A\xe3\x81\x82\xef\xbd\xb1";
	NoctValue fileutil;
	NoctValue function_value;
	NoctValue argument;
	NoctValue result;
	NoctFunc *function;
	const char *decoded;
	char path[sizeof("/tmp/noct-api-euc-jp-XXXXXX")];
	ssize_t written;
	int descriptor;
	int ok;

	memcpy(path, "/tmp/noct-api-euc-jp-XXXXXX", sizeof(path));
	ok = 0;
	memset(&fileutil, 0, sizeof(fileutil));
	memset(&function_value, 0, sizeof(function_value));
	memset(&argument, 0, sizeof(argument));
	memset(&result, 0, sizeof(result));
	descriptor = mkstemp(path);
	if (descriptor < 0)
		return 0;
	written = write(descriptor, encoded, sizeof(encoded));
	if (close(descriptor) != 0)
		goto cleanup_file;
	if (written != (ssize_t)sizeof(encoded))
		goto cleanup_file;

	if (!noct_pin_local(
		env,
		4,
		&fileutil,
		&function_value,
		&argument,
		&result))
		goto cleanup_file;
	if (!noct_get_global(env, "FileUtil", &fileutil))
		goto cleanup_values;
	if (!noct_get_dict_elem_check_func(
		env,
		&fileutil,
		"readTextEucJp",
		&function_value,
		&function))
		goto cleanup_values;
	if (!noct_make_string(env, &argument, path))
		goto cleanup_values;
	if (!noct_call(env, function, 1, &argument, &result))
		goto cleanup_values;
	if (!noct_get_string(env, &result, &decoded))
		goto cleanup_values;
	ok = strcmp(decoded, expected) == 0;

cleanup_values:
	(void)noct_unpin_local(
		env,
		4,
		&fileutil,
		&function_value,
		&argument,
		&result);
cleanup_file:
	(void)unlink(path);

	return ok;
}

/* Check that Regex is an explicitly registered API package. */
static int
test_regex_api_boundary(
	NoctEnv *env)
{
	NoctValue string_package;
	NoctValue regex_package;
	NoctValue function_value;
	NoctValue argument[2];
	NoctValue result;
	NoctFunc *function;
	bool present;
	int matches;
	int ok;

	memset(&string_package, 0, sizeof(string_package));
	memset(&regex_package, 0, sizeof(regex_package));
	memset(&function_value, 0, sizeof(function_value));
	memset(argument, 0, sizeof(argument));
	memset(&result, 0, sizeof(result));
	ok = 0;

	if (!noct_pin_local(
		env,
		6,
		&string_package,
		&regex_package,
		&function_value,
		&argument[0],
		&argument[1],
		&result))
		return 0;

	/* Regex must not be installed by the core VM. */
	if (!noct_check_global(env, "Regex", &present))
		goto cleanup;
	if (present)
		goto cleanup;
	if (!noct_get_global(env, "String", &string_package))
		goto cleanup;
	if (!noct_check_dict_key_cstr(
		env,
		&string_package,
		"search",
		&present))
		goto cleanup;
	if (present)
		goto cleanup;
	if (!noct_check_dict_key_cstr(
		env,
		&string_package,
		"matches",
		&present))
		goto cleanup;
	if (present)
		goto cleanup;
	if (!noct_check_dict_key_cstr(
		env,
		&string_package,
		"replaceAll",
		&present))
		goto cleanup;
	if (present)
		goto cleanup;

	if (!noct_register_api_regex(env))
		goto cleanup;
	if (!noct_check_global(env, "Regex", &present))
		goto cleanup;
	if (!present)
		goto cleanup;
	if (!noct_get_global(env, "Regex", &regex_package))
		goto cleanup;
	if (!noct_get_dict_elem_check_func(
		env,
		&regex_package,
		"matches",
		&function_value,
		&function))
		goto cleanup;
	if (!noct_make_string(env, &argument[0], "a+b*"))
		goto cleanup;
	if (!noct_make_string(env, &argument[1], "aaabb"))
		goto cleanup;
	if (!noct_call(env, function, 2, argument, &result))
		goto cleanup;
	if (!noct_get_int(env, &result, &matches))
		goto cleanup;
	if (matches != 1)
		goto cleanup;

	ok = 1;

cleanup:
	(void)noct_unpin_local(
		env,
		6,
		&string_package,
		&regex_package,
		&function_value,
		&argument[0],
		&argument[1],
		&result);

	return ok;
}
