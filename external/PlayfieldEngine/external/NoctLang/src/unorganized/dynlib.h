/* -*- coding: utf-8; tab-width: 8; indent-tabs-mode: t; -*- */

/*
 * Noct Programming Language
 * Copyright (c) 2025, 2026, Awe Morris
 */

#ifndef NOCT_DYNLIB_H
#define NOCT_DYNLIB_H

#include "runtime.h"

bool rt_load_library(struct rt_env *env, const char *name,
		     bool optional, bool *loaded);
void rt_cleanup_libraries(struct rt_vm *vm);

#endif
