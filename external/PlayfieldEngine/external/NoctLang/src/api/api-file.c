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
static bool cfunc_File_open(NoctEnv *env);
static bool cfunc_File_close(NoctEnv *env);
static bool cfunc_File_tell(NoctEnv *env);
static bool cfunc_File_seek(NoctEnv *env);
static bool cfunc_File_read(NoctEnv *env);
static bool cfunc_File_write(NoctEnv *env);
static void file_finalizer(void *native_pointer);

static bool cfunc_FileUtil_checkFileExists(NoctEnv *env);
static bool cfunc_FileUtil_getFileSize(NoctEnv *env);
static bool cfunc_FileUtil_readText(NoctEnv *env);
static bool cfunc_FileUtil_writeText(NoctEnv *env);
static bool cfunc_FileUtil_readForEachLine(NoctEnv *env);
static bool cfunc_FileUtil_writeForEachLine(NoctEnv *env);

/* FFI table. */
struct ffi_item {
	const char *global_name;
	const char *package_name;
	const char *field_name;
	size_t param_count;
	const char *param[NOCT_ARG_MAX];
	bool (*cfunc)(NoctEnv *env);
};
static struct ffi_item ffi_items[] = {
	/* File */
	{"File.open",			"File",		"open",			2,	{"path", "mode"},	cfunc_File_open},
	{"File.close",			"File",		"close",		1,	{"file"},		cfunc_File_close},
	{"File.tell",			"File",		"tell",			1,	{"file"},		cfunc_File_tell},
	{"File.seek",			"File",		"seek",			2,	{"file", "offset"},	cfunc_File_seek},
	{"File.read",			"File",		"read",			2,	{"file", "len"},	cfunc_File_read},
	{"File.write",			"File",		"write",		4,	{"file", "data", "offset", "size"},	cfunc_File_write},

	/* FileUtil */
	{"FileUtil.checkFileExists",	"FileUtil",	"checkFileExists",	1,	{"path"},		cfunc_FileUtil_checkFileExists},
	{"FileUtil.getFileSize",	"FileUtil",	"getFileSize",		1,	{"path"},		cfunc_FileUtil_getFileSize},
	{"FileUtil.readText",		"FileUtil",	"readText",		1,	{"path"},		cfunc_FileUtil_readText},
	{"FileUtil.writeText",		"FileUtil",	"writeText",		2,	{"path", "text"},	cfunc_FileUtil_writeText},
	{"FileUtil.readForEachLine",	"FileUtil",	"readForEachLine",	2,	{"path", "func"},	cfunc_FileUtil_readForEachLine},
	{"FileUtil.writeForEachLine",	"FileUtil",	"writeForEachLine",	2,	{"path", "lines"},	cfunc_FileUtil_writeForEachLine},
};

/*
 * Register "File.*" functions.
 */
NOCT_DLL
bool
noct_register_api_file(
	NoctEnv *env)
{
	NoctValue file_dict;
	NoctValue fileutil_dict;
	size_t i;

	/* Make global variables "File" and "FileUtil". */
	if (!noct_make_empty_dict(env, &file_dict))
		return false;
	if (!noct_make_empty_dict(env, &fileutil_dict))
		return false;
	if (!noct_set_global(env, "File", &file_dict))
		return false;
	if (!noct_set_global(env, "FileUtil", &fileutil_dict))
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
		if (strcmp(ffi_items[i].package_name, "File") == 0) {
			if (!noct_set_dict_elem_cstr(env, &file_dict, ffi_items[i].field_name, &funcval))
				return false;
		} else {
			if (!noct_set_dict_elem_cstr(env, &fileutil_dict, ffi_items[i].field_name, &funcval))
				return false;
		}
	}

	return true;
}

/* Implementation of File.open() */
static bool
cfunc_File_open(
	NoctEnv *env)
{
	NoctValue path, mode, ret;
	const char *path_s, *mode_s;
	FILE *fp;

	noct_pin_local(env, 3, &path, &mode, &ret);

	/* Get parameters. */
	if (!noct_get_arg_check_string(env, 0, &path, &path_s))
		return false;
	if (!noct_get_arg_check_string(env, 1, &mode, &mode_s))
		return false;
	
	/*  */
	if (strcmp(mode_s, "w") == 0)
		fp = fopen(path_s, "wb");
	else
		fp = fopen(path_s, "rb");

	/* Make a return value. */
	if (!noct_make_empty_dict(env, &ret)) {
		fclose(fp);
		return false;
	}
	if (!noct_set_dict_native_pointer(env, &ret, fp, file_finalizer)) {
		fclose(fp);
		return false;
	}
	if (!noct_set_return(env, &ret))
		return false;

	noct_unpin_local(env, 3, &path, &mode, &ret);

	return true;
}

static void
file_finalizer(void *native_pointer)
{
	if (native_pointer != NULL)
		fclose((FILE *)native_pointer);
}

/* Implementation of File.close() */
static bool
cfunc_File_close(
	NoctEnv *env)
{
	NoctValue file, ret;
	FILE *fp;
	void (*native_finalizer)(void *);

	noct_pin_local(env, 2, &file, &ret);

	/* Get parameters. */
	if (!noct_get_arg_check_dict(env, 0, &file))
		return false;

	/* Get the fp from the file arg. */
	if (!noct_get_dict_native_pointer(env, &file, (void *)&fp, &native_finalizer))
		return false;

	/* Close the fp. */
	fclose(fp);

	/* Clear the native pointer of the dict. */
	fp = NULL;
	native_finalizer = NULL;
	if (!noct_set_dict_native_pointer(env, &file, fp, native_finalizer))
		return false;

	/* Make a return value. */
	if (!noct_set_return_make_int(env, &ret, 0))
		return false;

	noct_unpin_local(env, 2, &file, &ret);

	return true;
}

/* Implementation of File.tell() */
static bool
cfunc_File_tell(
	NoctEnv *env)
{
	NoctValue file, ret;
	FILE *fp;
	size_t ofs;
	void (*native_finalizer)(void *);

	noct_pin_local(env, 2, &file, &ret);

	/* Get parameters. */
	if (!noct_get_arg_check_dict(env, 0, &file))
		return false;

	/* Get the fp from the file arg. */
	if (!noct_get_dict_native_pointer(env, &file, (void *)&fp, &native_finalizer))
		return false;

	/* Get the offset. */
	ofs = (size_t)ftell(fp);

	/* Make a return value. */
	if (!noct_set_return_make_int_long(env, &ret, ofs))
		return false;

	noct_unpin_local(env, 2, &file, &ret);

	return true;
}

/* Implementation of File.seek() */
static bool
cfunc_File_seek(
	NoctEnv *env)
{
	NoctValue file, offset, ret;
	size_t ofs;
	FILE *fp;
	void (*native_finalizer)(void *);

	noct_pin_local(env, 2, &file, &ret);

	/* Get parameters. */
	if (!noct_get_arg_check_dict(env, 0, &file))
		return false;
	if (!noct_get_arg_check_int_long(env, 1, &offset, &ofs))
		return false;

	/* Get the fp from the file arg. */
	if (!noct_get_dict_native_pointer(env, &file, (void *)&fp, &native_finalizer))
		return false;

	/* Get the offset. */
	fseek(fp, (long)ofs, SEEK_SET);

	/* Make a return value. */
	if (!noct_set_return_make_int(env, &ret, 1))
		return false;

	noct_unpin_local(env, 2, &file, &ret);

	return true;
}

/* Implementation of File.read() */
static bool
cfunc_File_read(
	NoctEnv *env)
{
	NoctValue file, len, ret;
	size_t len_n;
	FILE *fp;
	void *buf;
	void (*native_finalizer)(void *);

	noct_pin_local(env, 3, &file, &len, &ret);

	/* Get parameters. */
	if (!noct_get_arg_check_dict(env, 0, &file))
		return false;
	if (!noct_get_arg_check_int_long(env, 1, &len, &len_n))
		return false;

	/* Get the fp from the file arg. */
	if (!noct_get_dict_native_pointer(env, &file, (void *)&fp, &native_finalizer))
		return false;

	/* Read. */
	buf = malloc(len_n);
	if (buf == NULL) {
		noct_error(env, N_TR("Out of memory."));
		return false;
	}
	if (fread(buf, len_n, 1, fp) != 1) {
		free(buf);
		noct_error(env, N_TR("File read error."));
		return false;
	}

	/* Make a return value. */
	if (!noct_make_packed(env, &ret, NOCT_PACKED_UINT8, len_n, len_n, buf)) {
		free(buf);
		return false;
	}
	if (!noct_set_return(env, &ret)) {
		free(buf);
		return false;
	}

	noct_unpin_local(env, 3, &file, &len, &ret);

	return true;
}

/* Implementation of File.write() */
static bool
cfunc_File_write(
	NoctEnv *env)
{
	NoctValue file, data, offset, len, ret;
	size_t offset_n, len_n, packed_size;
	FILE *fp;
	void *buf;
	void (*native_finalizer)(void *);

	noct_pin_local(env, 5, &file, &data, &offset, &len, &ret);

	/* Get parameters. */
	if (!noct_get_arg_check_dict(env, 0, &file))
		return false;
	if (!noct_get_arg_check_packed(env, 1, &data, NOCT_PACKED_UINT8))
		return false;
	if (!noct_get_arg_check_int_long(env, 2, &offset, &offset_n))
		return false;
	if (!noct_get_arg_check_int_long(env, 3, &len, &len_n))
		return false;

	/* Get the fp from the file arg. */
	if (!noct_get_dict_native_pointer(env, &file, (void *)&fp, &native_finalizer))
		return false;

	/* Check the packed size. */
	if (!noct_get_packed_size(env, &data, &packed_size))
		return false;
	if (offset_n + len_n > packed_size) {
		noct_error(env, N_TR("Offset is out-of-range."));
		return false;
	}

	/* Write. */
	if (!noct_get_packed_pointer(env, &data, &buf))
		return false;
	if (fwrite((char *)buf + offset_n, len_n, 1, fp) != 1) {
		noct_error(env, N_TR("File write error."));
		return false;
	}

	/* Make a return value. */
	if (!noct_set_return_make_int(env, &ret, 1))
		return false;

	noct_unpin_local(env, 5, &file, &data, &offset, &len, &ret);

	return true;
}

/* Implementation of FileUtil.checkFileExists() */
static bool
cfunc_FileUtil_checkFileExists(
	NoctEnv *env)
{
	NoctValue path, ret;
	const char *path_s;
	int ret_i;

	noct_pin_local(env, 2, &path, &ret);

	/* Get parameters. */
	if (!noct_get_arg_check_string(env, 0, &path, &path_s))
		return false;

	/* Check the file. */
	ret_i = 0;
	if (access(path_s, 0) == 0)
		ret_i = 1;

	/* Make a return value. */
	if (!noct_set_return_make_int(env, &ret, ret_i))
		return false;

	noct_unpin_local(env, 2, &path, &ret);

	return true;
}

/* Implementation of FileUtil.getFileSize() */
static bool
cfunc_FileUtil_getFileSize(
	NoctEnv *env)
{
	NoctValue path, ret;
	FILE *fp;
	const char *path_s;
	size_t size;

	noct_pin_local(env, 2, &path, &ret);

	/* Get the "file" parameer. */
	if (!noct_get_arg_check_string(env, 0, &path, &path_s))
		return false;

	/* Check the file. */
	fp = fopen(path_s, "rb");
	if (fp == NULL) {
		/* Make a return value. */
		if (!noct_set_return_make_int(env, &ret, 0))
			return false;

		noct_unpin_local(env, 2, &path, &ret);
		return true;
	}
	fseek(fp, 0, SEEK_END);
	size = (size_t)ftell(fp);
	fclose(fp);
	
	/* Make a return value. */
	if (!noct_set_return_make_int_long(env, &ret, size))
		return false;

	noct_unpin_local(env, 2, &path, &ret);

	return true;
}

/* Implementation of FileUtil.readText() */
static bool
cfunc_FileUtil_readText(
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

/* Implementation of FileUtil.writeText() */
static bool
cfunc_FileUtil_writeText(
	NoctEnv *env)
{
	NoctValue path, text, ret;
	const char *path_s, *text_s;
	FILE *fp;
	size_t len;

	if (!noct_pin_local(env, 3, &path, &text, &ret))
		return false;

	if (!noct_get_arg_check_string(env, 0, &path, &path_s))
		return false;

	if (!noct_get_arg_check_string(env, 1, &text, &text_s))
		return false;

	/* Open the file. */
	fp = fopen(path_s, "wb");
	if (fp == NULL) {
		noct_error(env, N_TR("Cannot open file %s.\n"), path_s);
		return false;
	}

	/* Write the data. */
	len = strlen(text_s);
	if (fwrite(text_s, 1, len, fp) != len) {
		noct_error(env, N_TR("Cannot write file %s.\n"), path_s);
		return false;
	}

	fclose(fp);
	
	/* Make a return value. */
	if (!noct_set_return_make_int(env, &ret, 1)) {
		return false;
	}

	noct_unpin_local(env, 3, &path, &text, &ret);

	return true;
}

/* Implementation of FileUtil.readForEachLine() */
static bool
cfunc_FileUtil_readForEachLine(
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

/* Implementation of FileUtil.writeForEachLine() */
static bool
cfunc_FileUtil_writeForEachLine(
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
		if (!noct_get_array_elem(env, &lines, i, &line)) {
			fclose(fp);
			return false;
		}
		if (!noct_get_string(env, &line, &data)) {
			fclose(fp);
			return false;
		}
		fprintf(fp, "%s\n", data);
	}
	
	fclose(fp);

	/* Make a return value. */
	if (!noct_set_return_make_int(env, &ret, 1))
		return false;

	noct_unpin_local(env, 4, &file, &lines, &line, &ret);

	return true;
}
