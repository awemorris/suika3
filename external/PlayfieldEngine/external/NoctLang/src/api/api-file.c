/* -*- coding: utf-8; tab-width: 8; indent-tabs-mode: t; -*- */

/*
 * Noct Programming Language
 * Copyright (c) 2025, 2026, Awe Morris
 */

/*
 * API: File.*
 */

#include <noct/noct.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <assert.h>

#if defined(NOCT_TARGET_WINDOWS)
#include <fcntl.h>
#elif defined(NOCT_TARGET_DOS4G)
#include <io.h>
#else
#include <unistd.h>
#endif

#define NEVER_COME_HERE		(0)

/* Forward declaration. */
static bool cfunc_File_readText(NoctEnv *env);
static bool cfunc_File_writeText(NoctEnv *env);
static bool cfunc_File_readForEachLine(NoctEnv *env);
static bool cfunc_File_writeForEachLine(NoctEnv *env);
static bool cfunc_File_checkFileExists(NoctEnv *env);

/* FFI table. */
struct ffi_item {
	const char *global_name;
	const char *field_name;
	size_t param_count;
	const char *param[NOCT_ARG_MAX];
	bool (*cfunc)(NoctEnv *env);
};
static struct ffi_item ffi_items[] = {
	{"File.readText",		"readText",		1,	{"file"},		cfunc_File_readText},
	{"File.writeText",		"writeText",		2,	{"file", "text"},	cfunc_File_writeText},
	{"File.readForEachLine",	"readForEachLine",	2,	{"file", "func"},	cfunc_File_readForEachLine},
	{"File.writeForEachLine",	"writeForEachLine",	2,	{"file", "lines"},	cfunc_File_writeForEachLine},
	{"File.checkFileExists",	"checkFileExists",	1,	{"file"},		cfunc_File_checkFileExists},
};

/*
 * Register "File.*" functions.
 */
NOCT_DLL
bool
noct_register_api_file(
	NoctEnv *env)
{
	NoctValue dict;
	size_t i;

	/* Make a global variable "File". */
	if (!noct_make_empty_dict(env, &dict))
		return false;
	if (!noct_set_global(env, "File", &dict))
		return false;

	/* Register functions. */
	for (i = 0; i < sizeof(ffi_items) / sizeof(struct ffi_item); i++) {
		NoctValue funcval;

		/* Register a cfunc. */
		if (!noct_register_cfunc(
			    env,
			    ffi_items[i].global_name,
			    ffi_items[i].param_count,
			    ffi_items[i].param,
			    ffi_items[i].cfunc,
			    NULL))
			return false;

		/* Get a function value. */
		if (!noct_get_global(env, ffi_items[i].global_name, &funcval))
			return false;

		/* Make a dictionary element. */
		if (!noct_set_dict_elem_cstr(
			    env,
			    &dict,
			    ffi_items[i].field_name,
			    &funcval))
			return false;
	}

	return true;
}

/* Implementation of File.readText() */
static bool
cfunc_File_readText(
	NoctEnv *env)
{
	NoctValue file, ret;
	const char *fname;
	FILE *fp;
	size_t size;
	char *data;

	if (!noct_pin_local(env, 2, &file, &ret))
		return false;

	if (!noct_get_arg_check_string(env, 0, &file, &fname))
		return false;

	/* Open the file. */
	fp = fopen(fname, "rb");
	if (fp == NULL) {
		noct_error(env, N_TR("Cannot open file %s.\n"), fname);
		return false;
	}

	/* Get the file size. */
	fseek(fp, 0, SEEK_END);
	size = (size_t)ftell(fp);
	fseek(fp, 0, SEEK_SET);

	/* Allocate a buffer. */
	data = malloc(size + 1);
	if (data == NULL) {
		noct_error(env, N_TR("Out of memory.\n"));
		return false;
	}

	/* Read the data. */
	if (fread(data, 1, size, fp) != size) {
		noct_error(env, N_TR("Cannot read file %s.\n"), fname);
		return false;
	}

	/* Terminate the string. */
	data[size] = '\0';

	fclose(fp);
	
	/* Make a return value. */
	if (!noct_set_return_make_string(env, &ret, data)) {
		free(data);
		return false;
	}
	free(data);

	noct_unpin_local(env, 2, &file, &ret);

	return true;
}

/* Implementation of File.writeText() */
static bool
cfunc_File_writeText(
	NoctEnv *env)
{
	NoctValue file, text, ret;
	const char *fname, *text_s;
	FILE *fp;
	size_t len;

	if (!noct_pin_local(env, 3, &file, &text, &ret))
		return false;

	if (!noct_get_arg_check_string(env, 0, &file, &fname))
		return false;

	if (!noct_get_arg_check_string(env, 1, &text, &text_s))
		return false;

	/* Open the file. */
	fp = fopen(fname, "wb");
	if (fp == NULL) {
		noct_error(env, N_TR("Cannot open file %s.\n"), fname);
		return false;
	}

	/* Write the data. */
	len = strlen(text_s);
	if (fwrite(text_s, 1, len, fp) != len) {
		noct_error(env, N_TR("Cannot write file %s.\n"), fname);
		return false;
	}

	fclose(fp);
	
	/* Make a return value. */
	if (!noct_set_return_make_int(env, &ret, 1)) {
		return false;
	}

	noct_unpin_local(env, 3, &file, &text, &ret);

	return true;
}

/* Implementation of File.readForEachLine() */
static bool
cfunc_File_readForEachLine(
	NoctEnv *env)
{
	char buf[8192];
	NoctValue file, func, line, ret;
	NoctFunc *f;
	const char *fname;
	FILE *fp;

	if (!noct_pin_local(env, 4, &file, &func, &line, &ret))
		return false;

	if (!noct_get_arg_check_string(env, 0, &file, &fname))
		return false;

	if (!noct_get_arg_check_func(env, 1, &func, &f))
		return false;

	/* Open the file. */
	fp = fopen(fname, "rb");
	if (fp == NULL) {
		noct_error(env, N_TR("Cannot open file %s.\n"), fname);
		return false;
	}

	while (fgets(buf, sizeof(buf) - 2, fp) != NULL) {
		size_t len;

		len = strlen(buf);
		if (len == 0)
			break;

		if (buf[len - 1] == '\n')
			buf[len - 1] = '\0';

		if (!noct_make_string(env, &line, buf)) {
			fclose(fp);
			return false;
		}

		if (!noct_call(env, f, 1, &line, &ret)) {
			fclose(fp);
			return false;
		}
	}

	fclose(fp);

	/* Make a return value. */
	if (!noct_set_return_make_int(env, &ret, 1))
		return false;

	noct_unpin_local(env, 4, &file, &func, &line, &ret);

	return true;
}

/* Implementation of File.writeForEachLine() */
static bool
cfunc_File_writeForEachLine(
	NoctEnv *env)
{
	const char *fname;
	NoctValue file, lines, line, ret;
	FILE *fp;
	const char *data;
	size_t line_count;
	size_t i;

	if (!noct_pin_local(env, 4, &file, &lines, &line, &ret))
		return false;

	if (!noct_get_arg_check_string(env, 0, &file, &fname))
		return false;

	if (!noct_get_arg_check_array(env, 1, &lines))
		return false;

	if (!noct_get_array_size(env, &lines, &line_count))
		return false;

	/* Open the file. */
	fp = fopen(fname, "wb");
	if (fp == NULL) {
		noct_error(env, N_TR("Cannot open file %s.\n"), fname);
		return false;
	}

	for (i = 0; i < line_count; i++) {
		if (!noct_get_array_elem(env, &lines, i, &line))
			return false;
		if (!noct_get_string(env, &line, &data))
			return false;
		fprintf(fp, "%s\n", data);
	}
	
	fclose(fp);

	/* Make a return value. */
	if (!noct_set_return_make_int(env, &ret, 1))
		return false;

	noct_unpin_local(env, 4, &file, &lines, &line, &ret);

	return true;
}

/* Implementation of File.checkFileExists() */
static bool
cfunc_File_checkFileExists(
	NoctEnv *env)
{
	NoctValue file, ret;
	const char *s;
	int ret_i;

	noct_pin_local(env, 2, &file, &ret);

	/* Get the "file" parameer. */
	if (!noct_get_arg_check_string(env, 0, &file, &s))
		return false;

	/* Run a command. */
	ret_i = 0;
	if (access(s, 0) == 0)
		ret_i = 1;

	/* Make a return value. */
	if (!noct_set_return_make_int(env, &ret, ret_i))
		return false;

	noct_unpin_local(env, 2, &file, &ret);

	return true;
}
