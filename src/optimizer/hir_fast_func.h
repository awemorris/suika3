/* -*- coding: utf-8; tab-width: 8; indent-tabs-mode: t; -*- */

/*
 * Noct Programming Language
 * Copyright (c) 2025, 2026, Awe Morris
 */

/*
 * HIR optimizer support for __fast functions.
 */

#ifndef NOCT_HIR_FAST_FUNC_H
#define NOCT_HIR_FAST_FUNC_H

#include <noct/noct.h>

struct hir_block;

/*
 * Optimizes one checked fast-function body.
 */
bool
hir_fast_func_pass(
	struct hir_block *func_block);

#endif
