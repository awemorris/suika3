/* -*- coding: utf-8; tab-width: 8; indent-tabs-mode: t; -*- */

/*
 * Noct Programming Language
 * Copyright (c) 2025, 2026, Awe Morris
 */

/*
 * Target-neutral facts shared by HIR optimizer passes.
 */

#ifndef NOCT_HIR_OPT_ANALYSIS_H
#define NOCT_HIR_OPT_ANALYSIS_H

#include "hir.h"

#define HIR_OPT_SCALAR_UNKNOWN		(-1)

#define HIR_OPT_SCALAR_INT		(1U << NOCT_VALUE_INT)
#define HIR_OPT_SCALAR_FLOAT		(1U << NOCT_VALUE_FLOAT)
#define HIR_OPT_SCALAR_LONG		(1U << NOCT_VALUE_LONG)
#define HIR_OPT_SCALAR_DOUBLE		(1U << NOCT_VALUE_DOUBLE)
#define HIR_OPT_SCALAR_ALL		(HIR_OPT_SCALAR_INT | \
					 HIR_OPT_SCALAR_FLOAT | \
					 HIR_OPT_SCALAR_LONG | \
					 HIR_OPT_SCALAR_DOUBLE)

enum hir_affine_kind {
	HIR_AFFINE_UNKNOWN,
	HIR_AFFINE_INVARIANT,
	HIR_AFFINE_COUNTER_OFFSET
};

enum hir_opt_bounds_proof {
	HIR_OPT_BOUNDS_UNKNOWN,
	HIR_OPT_BOUNDS_WITHIN,
	HIR_OPT_BOUNDS_EXCEEDS
};

struct hir_affine_index {
	int kind;
	int offset;
	const char *invariant_symbol;
	int invariant_sign;
	const struct hir_expr *expr;
};

struct hir_opt_scalar_query {
	/* Zero selects every primitive scalar type. */
	uint32_t value_mask;

	/* Zero selects value_mask for arithmetic expressions. */
	uint32_t arithmetic_mask;

	void *data;
	int (*resolve_symbol)(void *data, const char *symbol);
	int (*resolve_subscript)(void *data, const struct hir_expr *expr);
};

/* An exact inclusive interval. */
struct hir_opt_interval {
	int64_t lower;
	int64_t upper;
};

/* An exact inclusive domain for one symbol. */
struct hir_opt_interval_binding {
	const char *symbol;
	struct hir_opt_interval interval;
};

bool
hir_opt_scalar_type_is_primitive(
	int type);

int
hir_opt_scalar_type_promote(
	int first,
	int second,
	uint32_t arithmetic_mask);

int
hir_opt_expr_scalar_type(
	const struct hir_expr *expr,
	const struct hir_opt_scalar_query *query);

bool
hir_opt_normalize_index(
	const struct hir_expr *expr,
	const char *counter,
	struct hir_affine_index *index);

bool
hir_opt_affine_equal(
	const struct hir_affine_index *first,
	const struct hir_affine_index *second);

bool
hir_opt_expr_constant_i64(
	const struct hir_expr *expr,
	int64_t *value);

bool
hir_opt_ranged_loop_interval(
	const struct hir_block *loop,
	struct hir_opt_interval *interval);

bool
hir_opt_index_interval(
	const struct hir_expr *expr,
	const struct hir_opt_interval_binding *binding,
	size_t binding_count,
	struct hir_opt_interval *interval);

int
hir_opt_prove_index_bounds(
	const struct hir_expr *expr,
	const struct hir_opt_interval_binding *binding,
	size_t binding_count,
	int64_t extent);

#endif
