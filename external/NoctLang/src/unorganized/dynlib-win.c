/* -*- coding: utf-8; tab-width: 8; indent-tabs-mode: t; -*- */

/*
 * Noct Programming Language
 * Copyright (c) 2025, 2026, Awe Morris
 */

#include "dynlib-os.h"

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static wchar_t *
dynlib_utf8_to_wide(const char *text)
{
	int count;
	wchar_t *wide;

	count = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS,
				    text, -1, NULL, 0);
	if (count <= 0)
		return NULL;
	wide = malloc(sizeof(*wide) * (size_t)count);
	if (wide == NULL)
		return NULL;
	if (MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS,
				text, -1, wide, count) != count) {
		free(wide);
		return NULL;
	}
	return wide;
}

static char *
dynlib_wide_to_utf8(const wchar_t *wide)
{
	int count;
	char *text;

	count = WideCharToMultiByte(CP_UTF8, 0, wide, -1,
				    NULL, 0, NULL, NULL);
	if (count <= 0)
		return NULL;
	text = malloc((size_t)count);
	if (text == NULL)
		return NULL;
	if (WideCharToMultiByte(CP_UTF8, 0, wide, -1, text, count,
				NULL, NULL) != count) {
		free(text);
		return NULL;
	}
	return text;
}

static void
dynlib_windows_error(char *error, size_t error_size)
{
	snprintf(error, error_size, "Windows error %lu",
		 (unsigned long)GetLastError());
}

int
dynlib_os_resolve(
	const char *path,
	char **canonical,
	char *error,
	size_t error_size)
{
	wchar_t *wide;
	wchar_t full[4096];
	DWORD attr;
	DWORD count;

	*canonical = NULL;
	wide = dynlib_utf8_to_wide(path);
	if (wide == NULL) {
		snprintf(error, error_size, "invalid UTF-8 path or out of memory");
		return -1;
	}
	attr = GetFileAttributesW(wide);
	if (attr == INVALID_FILE_ATTRIBUTES) {
		DWORD code = GetLastError();
		free(wide);
		if (code == ERROR_FILE_NOT_FOUND || code == ERROR_PATH_NOT_FOUND)
			return 0;
		SetLastError(code);
		dynlib_windows_error(error, error_size);
		return -1;
	}
	count = GetFullPathNameW(wide,
				 (DWORD)(sizeof(full) / sizeof(full[0])),
				 full, NULL);
	free(wide);
	if (count == 0 || count >= (DWORD)(sizeof(full) / sizeof(full[0]))) {
		dynlib_windows_error(error, error_size);
		return -1;
	}
	*canonical = dynlib_wide_to_utf8(full);
	if (*canonical == NULL) {
		snprintf(error, error_size, "out of memory");
		return -1;
	}
	return 1;
}

bool
dynlib_os_open(
	const char *path,
	void **handle,
	char *error,
	size_t error_size)
{
	wchar_t *wide;
	HMODULE module;

	wide = dynlib_utf8_to_wide(path);
	if (wide == NULL) {
		snprintf(error, error_size, "invalid UTF-8 path or out of memory");
		return false;
	}
	module = LoadLibraryW(wide);
	free(wide);
	if (module == NULL) {
		dynlib_windows_error(error, error_size);
		return false;
	}
	*handle = (void *)module;
	return true;
}

void *
dynlib_os_symbol(
	void *handle,
	const char *name,
	char *error,
	size_t error_size)
{
	FARPROC symbol;

	symbol = GetProcAddress((HMODULE)handle, name);
	if (symbol == NULL) {
		dynlib_windows_error(error, error_size);
		return NULL;
	}
	return (void *)symbol;
}
