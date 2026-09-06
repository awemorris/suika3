/* -*- coding: utf-8; tab-width: 8; indent-tabs-mode: t; -*- */

/*
 * Noct Programming Language
 * Copyright (c) 2025, 2026, Awe Morris
 */

/*
 * CLI: Windows Argument Conversion
 */

#if defined(_WIN32) && !defined(_WIN32_WINNT)
/* WC_ERR_INVALID_CHARS is available starting with Windows Vista. */
#define _WIN32_WINNT	0x0600
#endif

#include "cli-win32.h"

#include <windows.h>
#include <shellapi.h>

#include <stdlib.h>
#include <wchar.h>

#define CLI_GPU_OPTION_WIDE	L"--gpu="

/*
 * Converts a Windows GPU option value to UTF-8.
 */
bool
cli_windows_gpu_name_utf8(
	int argc,
	int arg_index,
	char **name)
{
	wchar_t **wide_argv;
	const wchar_t *wide_arg;
	const wchar_t *wide_name;
	size_t prefix_length;
	char *utf8_name;
	int wide_argc;
	int utf8_size;
	int converted;
	bool succeeded;

	if (name == NULL)
		return false;
	*name = NULL;

	wide_argv = CommandLineToArgvW(GetCommandLineW(), &wide_argc);
	if (wide_argv == NULL)
		return false;

	utf8_name = NULL;
	succeeded = false;

	if (wide_argc != argc)
		goto cleanup;
	if (arg_index < 0 || arg_index >= wide_argc)
		goto cleanup;

	wide_arg = wide_argv[arg_index];
	prefix_length = wcslen(CLI_GPU_OPTION_WIDE);
	if (wcsncmp(wide_arg, CLI_GPU_OPTION_WIDE, prefix_length) != 0)
		goto cleanup;

	wide_name = wide_arg + prefix_length;
	if (*wide_name == L'\0')
		goto cleanup;

	utf8_size = WideCharToMultiByte(
		CP_UTF8,
		WC_ERR_INVALID_CHARS,
		wide_name,
		-1,
		NULL,
		0,
		NULL,
		NULL);
	if (utf8_size <= 0)
		goto cleanup;

	utf8_name = malloc((size_t)utf8_size);
	if (utf8_name == NULL)
		goto cleanup;

	converted = WideCharToMultiByte(
		CP_UTF8,
		WC_ERR_INVALID_CHARS,
		wide_name,
		-1,
		utf8_name,
		utf8_size,
		NULL,
		NULL);
	if (converted != utf8_size)
		goto cleanup;

	succeeded = true;

cleanup:
	LocalFree(wide_argv);
	if (!succeeded) {
		free(utf8_name);
		return false;
	}

	*name = utf8_name;

	return true;
}
