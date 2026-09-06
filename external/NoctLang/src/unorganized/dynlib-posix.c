/* -*- coding: utf-8; tab-width: 8; indent-tabs-mode: t; -*- */

/*
 * Noct Programming Language
 * Copyright (c) 2025, 2026, Awe Morris
 */

#include "dynlib-os.h"

#include <dlfcn.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

int
dynlib_os_resolve(
	const char *path,
	char **canonical,
	char *error,
	size_t error_size)
{
	struct stat st;
	char resolved[4096];
	size_t len;

	*canonical = NULL;
	if (stat(path, &st) != 0) {
		if (errno == ENOENT || errno == ENOTDIR)
			return 0;
		snprintf(error, error_size, "%s", strerror(errno));
		return -1;
	}
	if (realpath(path, resolved) == NULL) {
		snprintf(error, error_size, "%s", strerror(errno));
		return -1;
	}
	len = strlen(resolved) + 1;
	*canonical = malloc(len);
	if (*canonical == NULL) {
		snprintf(error, error_size, "out of memory");
		return -1;
	}
	memcpy(*canonical, resolved, len);
	return 1;
}

bool
dynlib_os_open(
	const char *path,
	void **handle,
	char *error,
	size_t error_size)
{
	const char *message;

	dlerror();
	*handle = dlopen(path, RTLD_NOW | RTLD_LOCAL);
	if (*handle != NULL)
		return true;
	message = dlerror();
	snprintf(error, error_size, "%s",
		 message != NULL ? message : "dlopen failed");
	return false;
}

void *
dynlib_os_symbol(
	void *handle,
	const char *name,
	char *error,
	size_t error_size)
{
	void *symbol;
	const char *message;

	dlerror();
	symbol = dlsym(handle, name);
	message = dlerror();
	if (message == NULL)
		return symbol;
	snprintf(error, error_size, "%s", message);
	return NULL;
}
