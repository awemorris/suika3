/* -*- coding: utf-8; tab-width: 8; indent-tabs-mode: t; -*- */

/*
 * Noct Programming Language
 * Copyright (c) 2025, 2026, Awe Morris
 */

#ifndef NOCT_DYNLIB_OS_H
#define NOCT_DYNLIB_OS_H

#include <noct/noct.h>

/* 1 = exists/resolved, 0 = absent, -1 = error. */
int dynlib_os_resolve(const char *path, char **canonical,
		      char *error, size_t error_size);
bool dynlib_os_open(const char *path, void **handle,
		    char *error, size_t error_size);
void *dynlib_os_symbol(void *handle, const char *name,
		       char *error, size_t error_size);

#endif
