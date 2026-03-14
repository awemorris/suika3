/* -*- coding: utf-8; tab-width: 8; indent-tabs-mode: t; -*- */

/*
 * Playfield Engine
 * Compiler (AOT) main
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

#include "playfield/playfield.h"
#include "noct/noct.h"
#include "runtime.h"
#include "ast.h"
#include "hir.h"
#include "lir.h"
#include "cback.h"

#include <stdio.h>
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

/* i18n.c */
#if defined(NOCT_USE_TRANSLATION)
void noct_init_locale(void);
#endif

/* Forward declaration. */
static void show_usage(int argc, char *argv[]);
static int command_aot(int argc, char *argv[]);
static bool do_transpile_c(const char *out_file, int in_file_count, const char *in_file[]);
static bool add_file(const char *fname, bool (*add_file_hook)(const char *));
static bool add_file_hook_c(const char *fname);
static bool load_file_content(const char *fname, char **data, size_t *size);
static int wide_printf(const char *format, ...);

/*
 * Main
 */

/*
 * The top level function for the C translation mode.
 */
int main(int argc, char *argv[])
{
#if defined(USE_TRANSLATION)
	noct_init_locale();
#endif

	if (argc < 2 ||
	    (strcmp(argv[1], "--help") == 0 ||
	     strcmp(argv[1], "-h") == 0)) {
		show_usage(argc, argv);
		return 1;
	}

	return command_aot(argc, argv);
}

/* Show the usage message. */
static void show_usage(int argc, char *argv[])
{
	wide_printf("Script AOT Compiler\n");
	wide_printf("Usage: %s <files>\n", argv[0]);
}

/* Compile. */
int command_aot(int argc, char *argv[])
{
	if (argc < 2) {
		show_usage(argc, argv);
		return 1;
	}

	if (!do_transpile_c("library.c", 1, (const char **)&argv[1]))
		return 1;

	return 0;
}

/* Do C translation. */
static bool do_transpile_c(const char *out_file, int in_file_count, const char *in_file[])
{
	int i;

	/* Initialize the backend. */
	if (!cback_init(out_file))
		return false;

	/* For each input file or directory. */
	for (i = 0; i < in_file_count; i++) {
		/* Recursively add files. */
		if (!add_file(in_file[i], add_file_hook_c))
			return false;
	}

	/* Put a epilogue code. */
	if (!cback_finalize_dll())
		return false;

	return true;
}

/* "On file add" callback for the recursive file search. */
static bool add_file_hook_c(const char *fname)
{
	char *data;
	size_t len;
	int func_count, j;

	/* Load an argument file. */
	if (!load_file_content(fname, &data, &len))
		return false;

	/* Do parse, build AST. */
	if (!ast_build(fname, data)) {
		wide_printf(PF_TR("Error: %s: %d: %s"),
			    ast_get_file_name(),
			    ast_get_error_line(),
			    ast_get_error_message());
		wide_printf("\n");
		return false;
	}

	/* Transform AST to HIR. */
	if (!hir_build()) {
		wide_printf(PF_TR("Error: %s: %d: %s"),
			    hir_get_file_name(),
			    hir_get_error_line(),
			    hir_get_error_message());
		wide_printf("\n");
		return false;
	}

	/* For each HIR function. */
	func_count = hir_get_function_count();
	for (j = 0; j < func_count; j++) {
		struct hir_block *hfunc;
		struct lir_func *lfunc;

		/* Transform HIR to LIR (bytecode). */
		hfunc = hir_get_function(j);
		if (!lir_build(hfunc, &lfunc)) {
			wide_printf(PF_TR("Error: %s: %d: %s"),
				    lir_get_file_name(),
				    lir_get_error_line(),
				    lir_get_error_message());
			wide_printf("\n");
			return false;;
		}

		/* Put a C function. */
		if (!cback_translate_func(lfunc))
			return false;

		/* Free a single LIR. */
		lir_cleanup(lfunc);
	}

	/* Free intermediated. */
	hir_cleanup();
	ast_cleanup();

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
 * Load a file.
 */
static bool load_file_content(const char *fname, char **data, size_t *size)
{
	FILE *fp;

	/* Open the file. */
	fp = fopen(fname, "rb");
	if (fp == NULL) {
		wide_printf(PF_TR("Cannot open file \"%s\"."), fname);
		wide_printf("\n");
		return false;
	}

	/* Get the file size. */
	fseek(fp, 0, SEEK_END);
	*size = ftell(fp);
	fseek(fp, 0, SEEK_SET);

	/* Allocate a buffer. */
	*data = malloc(*size + 1);
	if (*data == NULL) {
		wide_printf(PF_TR("Out of memory."));
		wide_printf("\n");
		return false;
	}

	/* Read the data. */
	if (fread(*data, 1, *size, fp) != *size) {
		wide_printf(PF_TR("Cannot read file \"%s\"."), fname);
		return false;
	}

	/* Terminate the string. */
	(*data)[*size] = '\0';

	fclose(fp);

	return true;
}

/*
 * For POSIX
 */
#if !defined(_WIN32)

/*
 * Recursively add files.
 */
static bool add_file(const char *fname, bool (*add_file_hook)(const char *))
{
	struct stat st;

	if (stat(fname, &st) != 0) {
		wide_printf(PF_TR("Cannot read file \"%s\"."), fname);
		wide_printf("\n");
		return false;
	}
	if (S_ISDIR(st.st_mode)) {
		struct dirent **names;
		int count;
		int i;

		wide_printf(PF_TR("Searching directory \"%s\"."), fname);
		wide_printf("\n");

		/* Get directory content. */
		count = scandir(fname, &names, NULL, alphasort);
		if (count < 0) {
			wide_printf(PF_TR("Skipping empty directory \"%s\"."), fname);
			wide_printf("\n");
			return false;
		}

		for (i = 0; i < count; i++) {
			char next_path[256 + 1];

			if (strcmp(names[i]->d_name, ".") == 0)
				continue;
			if (strcmp(names[i]->d_name, "..") == 0)
				continue;

			snprintf(next_path, sizeof(next_path), "%s/%s", fname, names[i]->d_name);
			if (!add_file(next_path, add_file_hook))
				return false;
		}
	} else if(S_ISREG(st.st_mode)) {
		wide_printf(PF_TR("Adding file \"%s\"."), fname);
		wide_printf("\n");
		if (!add_file_hook(fname))
			return false;
	}

	return true;
}

/*
 * For Windows
 */
#else

#define BUF_SIZE	1024

static wchar_t wszMessage[BUF_SIZE];
static char szMessage[BUF_SIZE];

/* Convert UTF-8 to UTF-16. */
const wchar_t *win32_utf8_to_utf16(const char *utf8_message)
{
	assert(utf8_message != NULL);
	MultiByteToWideChar(CP_UTF8, 0, utf8_message, -1, wszMessage, BUF_SIZE - 1);
	return wszMessage;
}

/* Convert UTF-16 to UTF-8. */
const char *win32_utf16_to_utf8(const wchar_t *utf16_message)
{
	assert(utf16_message != NULL);
	WideCharToMultiByte(CP_UTF8, 0, utf16_message, -1, szMessage, BUF_SIZE - 1, NULL, NULL);
	return szMessage;
}

/*
 * Recursively add files.
 */
static bool add_file(const char *fname, bool (*add_file_hook)(const char *))
{
	WIN32_FIND_DATAW wfd;
	HANDLE hFind;
	DWORD dwAttr;

	dwAttr = GetFileAttributesW(win32_utf8_to_utf16(fname));
	if (dwAttr == INVALID_FILE_ATTRIBUTES)
		return false;
	if (dwAttr & FILE_ATTRIBUTE_DIRECTORY)
	{
		wchar_t wszFindPath[BUF_SIZE];

		_snwprintf(wszFindPath, BUF_SIZE, L"%s\\*.*", win32_utf8_to_utf16(fname));

		hFind = FindFirstFileW(wszFindPath, &wfd);
		if(hFind == INVALID_HANDLE_VALUE)
			return false;
		do
		{
			wchar_t wszNextPath[BUF_SIZE];

			if (wcscmp(wfd.cFileName, L".") == 0)
				continue;
			if (wcscmp(wfd.cFileName, L"..") == 0)
				continue;

			_snwprintf(wszNextPath, BUF_SIZE, L"%s\\%s", win32_utf8_to_utf16(fname), wfd.cFileName);
			if (!add_file(win32_utf16_to_utf8(wszNextPath), add_file_hook))
				return false;
		}
		while(FindNextFileW(hFind, &wfd));
	}
	else
	{
		wide_printf(PF_TR("Adding file \"%s\"."), fname);
		wide_printf("\n");
		if (!add_file_hook(fname))
			return false;
	}

	return true;
}

#endif	/* !defined(_WIN32) */

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
