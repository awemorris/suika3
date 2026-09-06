/* -*- coding: utf-8; tab-width: 8; indent-tabs-mode: t; -*- */

/*
 * Noct Programming Language
 * Copyright (c) 2025, 2026, Awe Morris
 */

/*
 * Optimizer-owned checked HIR support for __fast functions.
 */

#ifndef NOCT_HIR_FAST_CHECKED_H
#define NOCT_HIR_FAST_CHECKED_H

#include <noct/noct.h>

#if defined(NOCT_USE_OPTIMIZER)

struct hir_block;
struct fast_signature;

/* Collects externally visible prototypes from the current AST. */
bool
hir_fast_checked_collect_prototypes(
	void);

/*
 * Clears every externally collected function prototype.
 */
void
hir_fast_checked_reset_prototypes(
	void);

/*
 * Adds an externally collected function prototype.
 */
bool
hir_fast_checked_add_prototype(
	const char *name,
	bool is_fast,
	const struct fast_signature *signature);

bool
hir_fast_checked_module(
	struct hir_block *const *func_table,
	uint32_t func_count);

/* Releases optimizer-owned metadata attached to one HIR function. */
void
hir_fast_checked_cleanup_func(
	struct hir_block *func);

#endif /* defined(NOCT_USE_OPTIMIZER) */

#endif
