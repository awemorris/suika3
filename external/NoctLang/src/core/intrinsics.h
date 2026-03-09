/* -*- coding: utf-8; tab-width: 8; indent-tabs-mode: t; -*- */

/*
 * Copyright (c) 2025, Awe Morris. All rights reserved.
 */

/*
 * Intrinsics
 */

#ifndef NOCT_INTRINSICS_H
#define NOCT_INTRINSICS_H

#include <noct/c89compat.h>

/*
 * Intrinsics
 */

/* Register intrinsics. */
bool
rt_register_intrinsics(
	struct rt_env *rt);

/* Get an intrinsic func. */
bool
rt_get_intrin_thiscall_func(
	struct rt_env *rt,
	const char *name,
	struct rt_func **func);

#endif
