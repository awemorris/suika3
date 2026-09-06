/* -*- coding: utf-8; tab-width: 8; indent-tabs-mode: t; -*- */

/*
 * Noct Programming Language
 * Copyright (c) 2025, 2026, Awe Morris
 */

/*
 * Target-neutral facts shared by HIR optimizer passes.
 */

#include "hir_opt_analysis.h"

#include <limits.h>
#include <string.h>

#define HIR_OPT_ANALYSIS_MAX_DEPTH	64

#define HIR_OPT_INT64_MAX	((int64_t)(((uint64_t)-1) >> 1))
#define HIR_OPT_INT64_MIN	(-HIR_OPT_INT64_MAX - 1)

static uint32_t analysis_value_mask(const struct hir_opt_scalar_query *query);
static uint32_t analysis_arithmetic_mask(const struct hir_opt_scalar_query *query);
static bool analysis_type_allowed(int type, uint32_t mask);
static int analysis_expr_scalar_type(const struct hir_expr *expr, const struct hir_opt_scalar_query *query, int depth);
static const char *analysis_term_symbol(const struct hir_expr *expr);
static bool analysis_expr_int(const struct hir_expr *expr, int *value);
static bool analysis_add_i64(int64_t first, int64_t second, int64_t *result);
static bool analysis_sub_i64(int64_t first, int64_t second, int64_t *result);
static bool analysis_neg_i64(int64_t value, int64_t *result);
static bool analysis_expr_constant_i64(const struct hir_expr *expr, int64_t *value, int depth);
static bool analysis_find_interval(const struct hir_opt_interval_binding *binding, size_t binding_count, const char *symbol, struct hir_opt_interval *interval);
static bool analysis_expr_interval(const struct hir_expr *expr, const struct hir_opt_interval_binding *binding, size_t binding_count, struct hir_opt_interval *interval, int depth);

/*
 * Tests whether a value tag is a primitive scalar type.
 */
bool
hir_opt_scalar_type_is_primitive(
	int type)
{
	if (type == NOCT_VALUE_INT)
		return true;
	if (type == NOCT_VALUE_LONG)
		return true;
	if (type == NOCT_VALUE_FLOAT)
		return true;
	if (type == NOCT_VALUE_DOUBLE)
		return true;

	return false;
}

/*
 * Promotes two proven scalar types using the runtime numeric rules.
 */
int
hir_opt_scalar_type_promote(
	int first,
	int second,
	uint32_t arithmetic_mask)
{
	int result;

	if (!analysis_type_allowed(first, arithmetic_mask))
		return HIR_OPT_SCALAR_UNKNOWN;
	if (!analysis_type_allowed(second, arithmetic_mask))
		return HIR_OPT_SCALAR_UNKNOWN;

	if (first == NOCT_VALUE_DOUBLE ||
	    second == NOCT_VALUE_DOUBLE) {
		result = NOCT_VALUE_DOUBLE;
	} else if (first == NOCT_VALUE_FLOAT ||
		   second == NOCT_VALUE_FLOAT) {
		result = NOCT_VALUE_FLOAT;
	} else if (first == NOCT_VALUE_LONG ||
		   second == NOCT_VALUE_LONG) {
		result = NOCT_VALUE_LONG;
	} else {
		result = NOCT_VALUE_INT;
	}

	if (!analysis_type_allowed(result, arithmetic_mask))
		return HIR_OPT_SCALAR_UNKNOWN;

	return result;
}

/*
 * Queries the proven primitive scalar type of an expression.
 *
 * The query returns HIR_OPT_SCALAR_UNKNOWN whenever a required fact is
 * absent or an expression kind is not covered by the conservative rules.
 */
int
hir_opt_expr_scalar_type(
	const struct hir_expr *expr,
	const struct hir_opt_scalar_query *query)
{
	return analysis_expr_scalar_type(expr, query, 0);
}

/*
 * Normalizes an index into the shared counter-affine representation.
 */
bool
hir_opt_normalize_index(
	const struct hir_expr *expr,
	const char *counter,
	struct hir_affine_index *index)
{
	const char *symbol;
	const struct hir_expr *left;
	const struct hir_expr *right;
	int constant;

	if (index == NULL)
		return false;

	memset(index, 0, sizeof(*index));
	index->kind = HIR_AFFINE_UNKNOWN;
	index->invariant_sign = 1;
	index->expr = expr;

	/* Remove redundant parentheses from the index expression. */
	while (expr != NULL && expr->type == HIR_EXPR_PAR)
		expr = expr->val.unary.expr;

	symbol = analysis_term_symbol(expr);
	if (symbol != NULL) {
		if (counter != NULL) {
			if (strcmp(symbol, counter) == 0) {
				index->kind = HIR_AFFINE_COUNTER_OFFSET;
				return true;
			}
		}

		index->kind = HIR_AFFINE_INVARIANT;
		index->invariant_symbol = symbol;
		return true;
	}

	if (analysis_expr_int(expr, &constant)) {
		index->kind = HIR_AFFINE_INVARIANT;
		index->offset = constant;
		return true;
	}

	if (expr == NULL)
		return true;
	if (expr->type != HIR_EXPR_PLUS &&
	    expr->type != HIR_EXPR_MINUS)
		return true;

	left = expr->val.binary.expr[0];
	right = expr->val.binary.expr[1];
	symbol = analysis_term_symbol(left);
	if (symbol != NULL && counter != NULL) {
		if (strcmp(symbol, counter) == 0) {
			if (analysis_expr_int(right, &constant)) {
				if (expr->type == HIR_EXPR_MINUS &&
				    constant == INT_MIN) {
					return true;
				}
				if (expr->type == HIR_EXPR_MINUS)
					constant = -constant;

				index->kind = HIR_AFFINE_COUNTER_OFFSET;
				index->offset = constant;
				return true;
			}

			symbol = analysis_term_symbol(right);
			if (symbol != NULL) {
				if (strcmp(symbol, counter) != 0) {
					index->kind = HIR_AFFINE_COUNTER_OFFSET;
					index->invariant_symbol = symbol;
					if (expr->type == HIR_EXPR_MINUS)
						index->invariant_sign = -1;
					else
						index->invariant_sign = 1;
				}
			}

			return true;
		}
	}

	if (expr->type == HIR_EXPR_PLUS) {
		symbol = analysis_term_symbol(right);
		if (symbol != NULL && counter != NULL) {
			if (strcmp(symbol, counter) == 0) {
				if (analysis_expr_int(left, &constant)) {
					index->kind = HIR_AFFINE_COUNTER_OFFSET;
					index->offset = constant;
					return true;
				}

				symbol = analysis_term_symbol(left);
				if (symbol != NULL) {
					if (strcmp(symbol, counter) != 0) {
						index->kind =
							HIR_AFFINE_COUNTER_OFFSET;
						index->invariant_symbol = symbol;
						index->invariant_sign = 1;
					}
				}
			}
		}
	}

	return true;
}

/*
 * Compares two normalized affine indices.
 */
bool
hir_opt_affine_equal(
	const struct hir_affine_index *first,
	const struct hir_affine_index *second)
{
	if (first == NULL)
		return false;
	if (second == NULL)
		return false;
	if (first->kind != second->kind)
		return false;
	if (first->offset != second->offset)
		return false;
	if (first->invariant_sign != second->invariant_sign)
		return false;
	if (first->invariant_symbol == NULL &&
	    second->invariant_symbol == NULL) {
		return true;
	}
	if (first->invariant_symbol == NULL)
		return false;
	if (second->invariant_symbol == NULL)
		return false;
	if (strcmp(first->invariant_symbol, second->invariant_symbol) != 0)
		return false;

	return true;
}

/*
 * Evaluates an exact signed 64-bit integer constant expression.
 */
bool
hir_opt_expr_constant_i64(
	const struct hir_expr *expr,
	int64_t *value)
{
	if (value == NULL)
		return false;

	return analysis_expr_constant_i64(expr, value, 0);
}

/*
 * Computes the exact inclusive domain of a constant ranged loop.
 */
bool
hir_opt_ranged_loop_interval(
	const struct hir_block *loop,
	struct hir_opt_interval *interval)
{
	int64_t start;
	int64_t stop;

	if (loop == NULL)
		return false;
	if (interval == NULL)
		return false;
	if (loop->type != HIR_BLOCK_FOR)
		return false;
	if (!loop->val.for_.is_ranged)
		return false;
	if (loop->val.for_.counter_symbol == NULL)
		return false;

	if (!hir_opt_expr_constant_i64(loop->val.for_.start, &start))
		return false;
	if (!hir_opt_expr_constant_i64(loop->val.for_.stop, &stop))
		return false;
	if (stop <= start)
		return false;

	interval->lower = start;
	if (!analysis_sub_i64(stop, 1, &interval->upper))
		return false;

	return true;
}

/*
 * Computes an exact inclusive interval for a conservative index grammar.
 *
 * Bindings describe exact inclusive symbol domains.  The grammar accepts a
 * bound symbol with recursively added or subtracted integer constants.
 */
bool
hir_opt_index_interval(
	const struct hir_expr *expr,
	const struct hir_opt_interval_binding *binding,
	size_t binding_count,
	struct hir_opt_interval *interval)
{
	if (interval == NULL)
		return false;
	if (binding == NULL && binding_count != 0)
		return false;

	return analysis_expr_interval(
		expr,
		binding,
		binding_count,
		interval,
		0);
}

/*
 * Proves whether an index interval stays inside a positive extent.
 */
int
hir_opt_prove_index_bounds(
	const struct hir_expr *expr,
	const struct hir_opt_interval_binding *binding,
	size_t binding_count,
	int64_t extent)
{
	struct hir_opt_interval interval;

	if (extent <= 0)
		return HIR_OPT_BOUNDS_UNKNOWN;

	if (!hir_opt_index_interval(
		expr,
		binding,
		binding_count,
		&interval)) {
		return HIR_OPT_BOUNDS_UNKNOWN;
	}

	if (interval.lower >= 0 &&
	    interval.upper < extent) {
		return HIR_OPT_BOUNDS_WITHIN;
	}

	return HIR_OPT_BOUNDS_EXCEEDS;
}

/* Return the enabled scalar-result mask for a query. */
static uint32_t
analysis_value_mask(
	const struct hir_opt_scalar_query *query)
{
	if (query == NULL)
		return HIR_OPT_SCALAR_ALL;
	if (query->value_mask == 0)
		return HIR_OPT_SCALAR_ALL;

	return query->value_mask;
}

/* Return the enabled arithmetic-operand mask for a query. */
static uint32_t
analysis_arithmetic_mask(
	const struct hir_opt_scalar_query *query)
{
	if (query == NULL)
		return HIR_OPT_SCALAR_ALL;
	if (query->arithmetic_mask == 0)
		return analysis_value_mask(query);

	return query->arithmetic_mask;
}

/* Test whether a scalar tag is enabled in a mask. */
static bool
analysis_type_allowed(
	int type,
	uint32_t mask)
{
	if (!hir_opt_scalar_type_is_primitive(type))
		return false;
	if ((mask & (1U << type)) == 0)
		return false;

	return true;
}

/* Query one expression while enforcing a recursion limit. */
static int
analysis_expr_scalar_type(
	const struct hir_expr *expr,
	const struct hir_opt_scalar_query *query,
	int depth)
{
	int first;
	int second;
	int type;
	uint32_t value_mask;
	uint32_t arithmetic_mask;

	if (expr == NULL)
		return HIR_OPT_SCALAR_UNKNOWN;
	if (depth >= HIR_OPT_ANALYSIS_MAX_DEPTH)
		return HIR_OPT_SCALAR_UNKNOWN;

	value_mask = analysis_value_mask(query);
	arithmetic_mask = analysis_arithmetic_mask(query);

	/* Classify the expression using only sound scalar facts. */
	switch (expr->type) {
	case HIR_EXPR_TERM:
		if (expr->val.term.term == NULL)
			return HIR_OPT_SCALAR_UNKNOWN;

		/* Classify a literal or query a symbol fact. */
		switch (expr->val.term.term->type) {
		case HIR_TERM_INT:
			type = NOCT_VALUE_INT;
			break;
		case HIR_TERM_LONG:
			type = NOCT_VALUE_LONG;
			break;
		case HIR_TERM_FLOAT:
			type = NOCT_VALUE_FLOAT;
			break;
		case HIR_TERM_DOUBLE:
			type = NOCT_VALUE_DOUBLE;
			break;
		case HIR_TERM_SYMBOL:
			if (query == NULL)
				return HIR_OPT_SCALAR_UNKNOWN;
			if (query->resolve_symbol == NULL)
				return HIR_OPT_SCALAR_UNKNOWN;

			type = query->resolve_symbol(
				query->data,
				expr->val.term.term->val.symbol);
			break;
		default:
			return HIR_OPT_SCALAR_UNKNOWN;
		}

		if (!analysis_type_allowed(type, value_mask))
			return HIR_OPT_SCALAR_UNKNOWN;

		return type;
	case HIR_EXPR_PAR:
	case HIR_EXPR_CAPTURE:
		if (expr->type == HIR_EXPR_PAR) {
			return analysis_expr_scalar_type(
				expr->val.unary.expr,
				query,
				depth + 1);
		}

		return analysis_expr_scalar_type(
			expr->val.capture.expr,
			query,
			depth + 1);
	case HIR_EXPR_NEG:
		first = analysis_expr_scalar_type(
			expr->val.unary.expr,
			query,
			depth + 1);
		if (!analysis_type_allowed(first, arithmetic_mask))
			return HIR_OPT_SCALAR_UNKNOWN;

		return first;
	case HIR_EXPR_SELECT:
		first = analysis_expr_scalar_type(
			expr->val.select.if_true,
			query,
			depth + 1);
		second = analysis_expr_scalar_type(
			expr->val.select.if_false,
			query,
			depth + 1);
		if (first != second)
			return HIR_OPT_SCALAR_UNKNOWN;

		return first;
	case HIR_EXPR_PLUS:
	case HIR_EXPR_MINUS:
	case HIR_EXPR_MUL:
	case HIR_EXPR_DIV:
		first = analysis_expr_scalar_type(
			expr->val.binary.expr[0],
			query,
			depth + 1);
		second = analysis_expr_scalar_type(
			expr->val.binary.expr[1],
			query,
			depth + 1);
		type = hir_opt_scalar_type_promote(
			first,
			second,
			arithmetic_mask);
		if (!analysis_type_allowed(type, value_mask))
			return HIR_OPT_SCALAR_UNKNOWN;

		return type;
	case HIR_EXPR_MOD:
	case HIR_EXPR_AND:
	case HIR_EXPR_OR:
	case HIR_EXPR_XOR:
	case HIR_EXPR_SHL:
	case HIR_EXPR_SHR:
		first = analysis_expr_scalar_type(
			expr->val.binary.expr[0],
			query,
			depth + 1);
		second = analysis_expr_scalar_type(
			expr->val.binary.expr[1],
			query,
			depth + 1);
		if ((first != NOCT_VALUE_INT &&
		     first != NOCT_VALUE_LONG) ||
		    (second != NOCT_VALUE_INT &&
		     second != NOCT_VALUE_LONG)) {
			return HIR_OPT_SCALAR_UNKNOWN;
		}
		if (!analysis_type_allowed(first, arithmetic_mask))
			return HIR_OPT_SCALAR_UNKNOWN;
		if (!analysis_type_allowed(second, arithmetic_mask))
			return HIR_OPT_SCALAR_UNKNOWN;

		if (first == NOCT_VALUE_LONG ||
		    second == NOCT_VALUE_LONG) {
			type = NOCT_VALUE_LONG;
		} else {
			type = NOCT_VALUE_INT;
		}

		if (!analysis_type_allowed(type, value_mask))
			return HIR_OPT_SCALAR_UNKNOWN;

		return type;
	case HIR_EXPR_SUBSCR:
		if (query == NULL)
			return HIR_OPT_SCALAR_UNKNOWN;
		if (query->resolve_subscript == NULL)
			return HIR_OPT_SCALAR_UNKNOWN;

		type = query->resolve_subscript(query->data, expr);
		if (!analysis_type_allowed(type, value_mask))
			return HIR_OPT_SCALAR_UNKNOWN;

		return type;
	case HIR_EXPR_CALL:

		/* Classify the supported conversion intrinsic. */
		switch (hir_get_intrinsic_call(expr)) {
		case HIR_INTRINSIC_INT_FROM:
			type = NOCT_VALUE_INT;
			break;
		case HIR_INTRINSIC_FLOAT_FROM:
			type = NOCT_VALUE_FLOAT;
			break;
		default:
			return HIR_OPT_SCALAR_UNKNOWN;
		}

		if (!analysis_type_allowed(type, value_mask))
			return HIR_OPT_SCALAR_UNKNOWN;

		return type;
	case HIR_EXPR_PLOAD8U:
	case HIR_EXPR_PLOAD8S:
	case HIR_EXPR_PLOAD16U:
	case HIR_EXPR_PLOAD16S:
	case HIR_EXPR_PLOAD32:
	case HIR_EXPR_PGATHER32:
	case HIR_EXPR_PLEN:
	case HIR_EXPR_PCHECK:
	case HIR_EXPR_TYPEIS:
		type = NOCT_VALUE_INT;
		break;
	case HIR_EXPR_PLOAD64:
	case HIR_EXPR_PBASE:
		type = NOCT_VALUE_LONG;
		break;
	case HIR_EXPR_PLOADF32:
	case HIR_EXPR_VINDUCTF32:
		type = NOCT_VALUE_FLOAT;
		break;
	default:
		return HIR_OPT_SCALAR_UNKNOWN;
	}

	if (!analysis_type_allowed(type, value_mask))
		return HIR_OPT_SCALAR_UNKNOWN;

	return type;
}

/* Return a plain symbol term, or NULL for every other expression. */
static const char *
analysis_term_symbol(
	const struct hir_expr *expr)
{
	if (expr == NULL)
		return NULL;
	if (expr->type != HIR_EXPR_TERM)
		return NULL;
	if (expr->val.term.term == NULL)
		return NULL;
	if (expr->val.term.term->type != HIR_TERM_SYMBOL)
		return NULL;

	return expr->val.term.term->val.symbol;
}

/* Return a plain int term without folding other expression forms. */
static bool
analysis_expr_int(
	const struct hir_expr *expr,
	int *value)
{
	if (expr == NULL)
		return false;
	if (value == NULL)
		return false;
	if (expr->type != HIR_EXPR_TERM)
		return false;
	if (expr->val.term.term == NULL)
		return false;
	if (expr->val.term.term->type != HIR_TERM_INT)
		return false;

	*value = expr->val.term.term->val.i;

	return true;
}

/* Add two signed 64-bit values without invoking signed overflow. */
static bool
analysis_add_i64(
	int64_t first,
	int64_t second,
	int64_t *result)
{
	if (second > 0 &&
	    first > HIR_OPT_INT64_MAX - second) {
		return false;
	}
	if (second < 0 &&
	    first < HIR_OPT_INT64_MIN - second) {
		return false;
	}

	*result = first + second;

	return true;
}

/* Subtract two signed 64-bit values without invoking signed overflow. */
static bool
analysis_sub_i64(
	int64_t first,
	int64_t second,
	int64_t *result)
{
	if (second > 0 &&
	    first < HIR_OPT_INT64_MIN + second) {
		return false;
	}
	if (second < 0 &&
	    first > HIR_OPT_INT64_MAX + second) {
		return false;
	}

	*result = first - second;

	return true;
}

/* Negate a signed 64-bit value without invoking signed overflow. */
static bool
analysis_neg_i64(
	int64_t value,
	int64_t *result)
{
	if (value == HIR_OPT_INT64_MIN)
		return false;

	*result = -value;

	return true;
}

/* Evaluate a constant expression under the shared conservative grammar. */
static bool
analysis_expr_constant_i64(
	const struct hir_expr *expr,
	int64_t *value,
	int depth)
{
	int64_t first;
	int64_t second;

	if (expr == NULL)
		return false;
	if (depth >= HIR_OPT_ANALYSIS_MAX_DEPTH)
		return false;

	/* Evaluate the supported constant expression forms. */
	switch (expr->type) {
	case HIR_EXPR_TERM:
		if (expr->val.term.term == NULL)
			return false;
		if (expr->val.term.term->type == HIR_TERM_INT) {
			*value = (int64_t)expr->val.term.term->val.i;
			return true;
		}
		if (expr->val.term.term->type == HIR_TERM_LONG) {
			*value = expr->val.term.term->val.l;
			return true;
		}

		return false;
	case HIR_EXPR_PAR:
		return analysis_expr_constant_i64(
			expr->val.unary.expr,
			value,
			depth + 1);
	case HIR_EXPR_NEG:
		if (!analysis_expr_constant_i64(
			expr->val.unary.expr,
			&first,
			depth + 1)) {
			return false;
		}

		return analysis_neg_i64(first, value);
	case HIR_EXPR_PLUS:
	case HIR_EXPR_MINUS:
		if (!analysis_expr_constant_i64(
			expr->val.binary.expr[0],
			&first,
			depth + 1)) {
			return false;
		}
		if (!analysis_expr_constant_i64(
			expr->val.binary.expr[1],
			&second,
			depth + 1)) {
			return false;
		}
		if (expr->type == HIR_EXPR_PLUS)
			return analysis_add_i64(first, second, value);

		return analysis_sub_i64(first, second, value);
	default:
		return false;
	}
}

/* Find the exact interval bound to one symbol. */
static bool
analysis_find_interval(
	const struct hir_opt_interval_binding *binding,
	size_t binding_count,
	const char *symbol,
	struct hir_opt_interval *interval)
{
	size_t i;

	if (symbol == NULL)
		return false;

	/* Find the first exact domain bound to the requested symbol. */
	for (i = 0; i < binding_count; i++) {
		if (binding[i].symbol == NULL)
			continue;
		if (strcmp(binding[i].symbol, symbol) != 0)
			continue;
		if (binding[i].interval.lower > binding[i].interval.upper)
			return false;

		*interval = binding[i].interval;
		return true;
	}

	return false;
}

/* Evaluate an exact interval under the shared conservative grammar. */
static bool
analysis_expr_interval(
	const struct hir_expr *expr,
	const struct hir_opt_interval_binding *binding,
	size_t binding_count,
	struct hir_opt_interval *interval,
	int depth)
{
	struct hir_opt_interval variable;
	int64_t constant;
	int64_t lower;
	int64_t upper;
	const char *symbol;

	if (expr == NULL)
		return false;
	if (depth >= HIR_OPT_ANALYSIS_MAX_DEPTH)
		return false;

	if (analysis_expr_constant_i64(expr, &constant, depth)) {
		interval->lower = constant;
		interval->upper = constant;
		return true;
	}

	symbol = analysis_term_symbol(expr);
	if (symbol != NULL) {
		return analysis_find_interval(
			binding,
			binding_count,
			symbol,
			interval);
	}

	if (expr->type == HIR_EXPR_PAR) {
		return analysis_expr_interval(
			expr->val.unary.expr,
			binding,
			binding_count,
			interval,
			depth + 1);
	}

	if (expr->type != HIR_EXPR_PLUS &&
	    expr->type != HIR_EXPR_MINUS) {
		return false;
	}

	if (analysis_expr_interval(
		expr->val.binary.expr[0],
		binding,
		binding_count,
		&variable,
		depth + 1)) {
		if (analysis_expr_constant_i64(
			expr->val.binary.expr[1],
			&constant,
			depth + 1)) {
			if (expr->type == HIR_EXPR_PLUS) {
				if (!analysis_add_i64(
					variable.lower,
					constant,
					&lower)) {
					return false;
				}
				if (!analysis_add_i64(
					variable.upper,
					constant,
					&upper)) {
					return false;
				}
			} else {
				if (!analysis_sub_i64(
					variable.lower,
					constant,
					&lower)) {
					return false;
				}
				if (!analysis_sub_i64(
					variable.upper,
					constant,
					&upper)) {
					return false;
				}
			}

			interval->lower = lower;
			interval->upper = upper;
			return true;
		}
	}

	if (!analysis_expr_constant_i64(
		expr->val.binary.expr[0],
		&constant,
		depth + 1)) {
		return false;
	}
	if (!analysis_expr_interval(
		expr->val.binary.expr[1],
		binding,
		binding_count,
		&variable,
		depth + 1)) {
		return false;
	}

	if (expr->type == HIR_EXPR_PLUS) {
		if (!analysis_add_i64(constant, variable.lower, &lower))
			return false;
		if (!analysis_add_i64(constant, variable.upper, &upper))
			return false;
	} else {
		if (!analysis_sub_i64(constant, variable.upper, &lower))
			return false;
		if (!analysis_sub_i64(constant, variable.lower, &upper))
			return false;
	}

	interval->lower = lower;
	interval->upper = upper;

	return true;
}
