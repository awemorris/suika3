/* -*- coding: utf-8; tab-width: 8; indent-tabs-mode: t; -*- */

/*
 * Noct Programming Language
 * Copyright (c) 2025, 2026, Awe Morris
 */

/*
 * Detached accelerator HIR rewrite construction.
 */

#ifndef NOCT_ACCEL_REWRITE_H
#define NOCT_ACCEL_REWRITE_H

#include "accel_context.h"

struct accel_rewrite;

/*
 * Stages detached ordinary HIR for every planned accelerator region.
 */
enum accel_compile_status
accel_rewrite_stage(
	struct hir_block *func_block,
	const struct accel_function_plan *plan,
	const struct accel_registry_reservation *reservation,
	struct accel_rewrite **result);

/*
 * Links all preallocated generated locals into unchanged live HIR.
 */
bool
accel_rewrite_add_locals(
	struct accel_rewrite *rewrite);

/*
 * Commits every staged link swap without allocation or failure.
 */
void
accel_rewrite_commit(
	struct accel_rewrite *rewrite);

/*
 * Destroys rewrite staging metadata without freeing arena objects.
 */
void
accel_rewrite_destroy(
	struct accel_rewrite *rewrite);

#endif
