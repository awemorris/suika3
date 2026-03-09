/* -*- coding: utf-8; tab-width: 8; indent-tabs-mode: t; -*- */

/*
 * Playfield Engine
 * Compiler main
 */

/*-
 * SPDX-License-Identifier: Zlib
 *
 * Playfield Engine
 * Copyright (c) 2025-2026 Awe Morris
 *
 * This software is derived from the codebase of Suika2.
 * Copyright (c) 1996-2024 Keiichi Tabata
 *
 * This software is provided 'as-is', without any express or implied
 * warranty. In no event will the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software. If you use this software
 *    in a product, an acknowledgment in the product documentation would be
 *    appreciated but is not required.
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
 */

#include "noct/noct.h"
#include "runtime.h"
#include "ast.h"
#include "hir.h"
#include "lir.h"
#include "cback.h"
#include "elback.h"

#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <assert.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
#endif

/* Bytecode File Header */
#define BYTECODE_HEADER		"Noct Bytecode"

/* Runtime's configuration. */
extern bool noct_conf_use_jit;

/* i18n.c */
#if defined(USE_TRANSLATION)
void noct_init_locale(void);
#endif

/* Forward declaration. */
static void show_usage(void);
static int command_compile(int argc, char *argv[]);
static bool compile_source(const char *file_name);
static bool load_file_content(const char *fname, char **data, size_t *size);
static int wide_printf(const char *format, ...);

/*
 * Main
 */

int main(int argc, char *argv[])
{
#if defined(USE_TRANSLATION)
	noct_init_locale();
#endif

	if (argc < 2 ||
	    (strcmp(argv[1], "--help") == 0 ||
	     strcmp(argv[1], "-h") == 0)) {
		show_usage();
		return 1;
	}

	return command_compile(argc, argv);
}

/* Show the usage message. */
static void show_usage(void)
{
	wide_printf("Playfield Script Compiler\n");
	wide_printf("Usage: playfield-compiler <files>\n");
}

/*
 * Compile
 */

/* The top level function for the compile mode. */
static int command_compile(int argc, char *argv[])
{
	int i;

	/* For each argument file. */
	for (i = 1; i < argc; i++) {
		/* Compile a source to bytecode. */
		if (!compile_source(argv[i]))
			return 1;
	}

	return 1;
}

/* Compile a source file. */
static bool compile_source(const char *file_name)
{
	char bc_fname[1024];
	FILE *fp;
	char *source_data, *dot;
	size_t source_length;
	int func_count, i, j;

	/* Load an argument source file. */
	if (!load_file_content(file_name, &source_data, &source_length))
		return false;

	/* Do parse, build AST. */
	if (!ast_build(file_name, source_data)) {
		wide_printf(N_TR("Error: %s: %d: %s\n"),
			    ast_get_file_name(),
			    ast_get_error_line(),
			    ast_get_error_message());
		return false;
	}

	/* Transform AST to HIR. */
	if (!hir_build()) {
		wide_printf(N_TR("Error: %s: %d: %s\n"),
			    hir_get_file_name(),
			    hir_get_error_line(),
			    hir_get_error_message());
		return false;
	}

	/* Make an output file name. (*.pfc) */
	strcpy(bc_fname, file_name);
	dot = strstr(bc_fname, ".");
	if (dot != NULL)
		strcpy(dot, ".pfc");
	else
		strcat(bc_fname, ".pfc");

	/* Open an output .nb bytecode file. */
	fp = fopen(bc_fname, "wb");
	if (fp == NULL) {
		wide_printf(N_TR("Cannot open file %s.\n"), bc_fname);
		return false;
	}

	/* Put a file header. */
	func_count = hir_get_function_count();
	fprintf(fp, "Noct Bytecode 1.0\n");
	fprintf(fp, "Source\n");
	fprintf(fp, "%s\n", file_name);
	fprintf(fp, "Number Of Functions\n");
	fprintf(fp, "%d\n", func_count);

	/* For each HIR function. */
	for (i = 0; i < func_count; i++) {
		struct hir_block *hfunc;
		struct lir_func *lfunc;

		/* Transform HIR to LIR (bytecode). */
		hfunc = hir_get_function(i);
		if (!lir_build(hfunc, &lfunc)) {
			wide_printf(N_TR("Error: %s: %d: %s\n"),
				    lir_get_file_name(),
				    lir_get_error_line(),
				    lir_get_error_message());
			return false;
		}

		/* Put a bytcode. */
		fprintf(fp, "Begin Function\n");
		fprintf(fp, "Name\n");
		fprintf(fp, "%s\n", lfunc->func_name);
		fprintf(fp, "Parameters\n");
		fprintf(fp, "%d\n", lfunc->param_count);
		for (j = 0; j < lfunc->param_count; j++)
			fprintf(fp, "%s\n", lfunc->param_name[j]);
		fprintf(fp, "Temporary Size\n");
		fprintf(fp, "%d\n", lfunc->tmpvar_size);
		fprintf(fp, "Bytecode Size\n");
		fprintf(fp, "%d\n", lfunc->bytecode_size);
		fwrite(lfunc->bytecode, (size_t)lfunc->bytecode_size, 1, fp);
		fprintf(fp, "\nEnd Function\n");

		/* Free a single LIR. */
		lir_cleanup(lfunc);
	}

	fclose(fp);

	/* Free intermediates. */
	hir_cleanup();
	ast_cleanup();

	return true;
}

/*
 * Helpers
 */

/* Load a file. */
static bool load_file_content(const char *fname, char **data, size_t *size)
{
	FILE *fp;

	/* Open the file. */
	fp = fopen(fname, "rb");
	if (fp == NULL) {
		wide_printf(N_TR("Cannot open file %s.\n"), fname);
		return false;
	}

	/* Get the file size. */
	fseek(fp, 0, SEEK_END);
	*size = ftell(fp);
	fseek(fp, 0, SEEK_SET);

	/* Allocate a buffer. */
	*data = malloc(*size + 1);
	if (*data == NULL) {
		wide_printf(N_TR("Out of memory.\n"));
		return false;
	}

	/* Read the data. */
	if (fread(*data, 1, *size, fp) != *size) {
		wide_printf(N_TR("Cannot read file %s.\n"), fname);
		return false;
	}

	/* Terminate the string. */
	(*data)[*size] = '\0';

	fclose(fp);

	return true;
}

/* Print to console. (supports wide characters on Windows console.) */
static int wide_printf(const char *format, ...)
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
	/* MSVC or MinGW: Use wprintf() and wide-string. (Otherwise, we'll see garbages.) */
	static wchar_t wbuf[4096];
	DWORD dwWritten;
	memset(wbuf, 0, sizeof(wbuf));
	MultiByteToWideChar(CP_UTF8, 0, buf, -1, wbuf, sizeof(wbuf) / sizeof(wchar_t));
	WriteConsoleW(GetStdHandle(STD_OUTPUT_HANDLE), wbuf, lstrlenW(wbuf), &dwWritten, NULL);
	return size;
#endif
}

/*
 * Windows Quirk
 */

#ifdef _WIN32
#include <windows.h>
int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow)
{
    /* Dispatch to main(). */
    int argc, i;
    wchar_t **wargv = CommandLineToArgvW(GetCommandLineW(), &argc);
    char **argv = malloc(sizeof(char *) * argc);
    for (i = 0; i < argc; i++)
    {
        char tmp[1024];
	WideCharToMultiByte(CP_ACP, 0, wargv[i], -1, tmp, sizeof(tmp) - 1, NULL, NULL);
        argv[i] = strdup(tmp);
    }
    return main(argc, argv);
}
#endif
