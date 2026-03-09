/* -*- coding: utf-8; tab-width: 8; indent-tabs-mode: t; -*- */

/*
 * Copyright (c) 2025, Awe Morris. All rights reserved.
 */

/*
 * API: System.*
 */

#include <noct/noct.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Forward declaration. */
static bool cfunc_System_import(NoctEnv *env);
static bool cfunc_System_shell(NoctEnv *env);
static bool system_load_file(NoctEnv *env, const char *fname, char **data, size_t *size);

/* FFI table. */
struct ffi_item {
	const char *global_name;
	const char *field_name;
	int param_count;
	const char *param[NOCT_ARG_MAX];
	bool (*cfunc)(NoctEnv *env);
};
static struct ffi_item ffi_items[] = {
	{"__System_import",	"import",	1,	{"file"},	cfunc_System_import},
	{"__System_shell",	"shell",	1,	{"cmd"},	cfunc_System_shell},
};

/*
 * Register "System.*" functions.
 */
bool noct_register_api_system(NoctEnv *env)
{
	NoctValue dict;
	int i;

	/* Make a global variable "System". */
	if (!noct_make_empty_dict(env, &dict))
		return false;
	if (!noct_set_global(env, "System", &dict))
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

/* Implementation of import() */
static bool
cfunc_System_import(
	NoctEnv *env)
{
	NoctValue tmp;
	const char *file;
	char *data;
	size_t len;

	if (!noct_get_arg_check_string(env, 0, &tmp, &file))
		return false;

	/* Load a source file content. */
	if (!system_load_file(env, file, &data, &len))
		return false;

	/* Check for the bytecode header. */
	if (strncmp(data, NOCT_BYTECODE_HEADER, strlen(NOCT_BYTECODE_HEADER)) != 0) {
		/* It's a source file. */
		if (!noct_register_source(env, file, data))
			return false;
	} else {
		/* It's a bytecode file. */
		if (!noct_register_bytecode(env, (void *)data, len))
			return false;
	}

	return true;
}

/* Implementation of shell() */
static bool
cfunc_System_shell(
	NoctEnv *env)
{
	NoctValue tmp;
	const char *s;
	int cmd_ret;

	/* Get a "cmd" parameer. */
	if (!noct_get_arg_check_string(env, 0, &tmp, &s))
		return false;

	/* Run a command. */
	cmd_ret = system(s);

	/* Make a return value. */
	if (!noct_set_return_make_int(env, &tmp, cmd_ret))
		return false;

	return true;
}

/* Load a file. */
static bool system_load_file(NoctEnv *env, const char *fname, char **data, size_t *size)
{
	FILE *fp;

	/* Open the file. */
	fp = fopen(fname, "rb");
	if (fp == NULL) {
		noct_error(env, N_TR("Cannot open file %s.\n"), fname);
		return false;
	}

	/* Get the file size. */
	fseek(fp, 0, SEEK_END);
	*size = ftell(fp);
	fseek(fp, 0, SEEK_SET);

	/* Allocate a buffer. */
	*data = malloc(*size + 1);
	if (*data == NULL) {
		noct_out_of_memory(env);
		return false;
	}

	/* Read the data. */
	if (fread(*data, 1, *size, fp) != *size) {
		noct_error(env, N_TR("Cannot read file %s.\n"), fname);
		return false;
	}

	/* Terminate the string. */
	(*data)[*size] = '\0';

	fclose(fp);

	return true;
}
