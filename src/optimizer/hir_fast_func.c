/* -*- coding: utf-8; tab-width: 8; indent-tabs-mode: t; -*- */

/*
 * Noct Programming Language
 * Copyright (c) 2025, 2026, Awe Morris
 */

/*
 * Checked-index simplification for __fast functions.
 */

#include "hir_fast_func.h"

#include "fast.h"
#include "hir_opt.h"
#include "hir_opt_analysis.h"

#include <assert.h>
#include <limits.h>
#include <string.h>

#define FAST_LOOP_DEPTH_MAX	64
#define FAST_INDEX_RANK_MIN	2
#define FAST_INDEX_RANK_MAX	8

struct fast_ctx {
	const struct hir_block *func;
	struct hir_opt_interval_binding binding[FAST_LOOP_DEPTH_MAX];
	size_t binding_count;
	int line;
	bool conditional;
};

/* Forward declarations. */
static bool fast_rewrite_chain(struct fast_ctx *ctx, struct hir_block *head);
static bool fast_chain_has_control_edge(const struct hir_block *head);
static bool fast_chain_assigns_symbol(const struct hir_block *head, const char *symbol);
static bool fast_rewrite_expr(struct fast_ctx *ctx, struct hir_expr **slot);
static bool fast_rewrite_conditional_expr(struct fast_ctx *ctx, struct hir_expr **slot);
static bool fast_check_rank_one_subscript(struct fast_ctx *ctx, const struct hir_expr *expr);
static bool fast_try_index_call(struct fast_ctx *ctx, struct hir_expr **slot);
static int fast_index_rank(const struct hir_expr *expr);
static bool fast_term_is_symbol(const struct hir_expr *expr, const char *symbol);
static bool fast_product_fits(const int64_t *extent, int rank);
static struct hir_expr *fast_build_index(struct hir_expr *const *arg, const int64_t *extent, int rank);
static struct hir_expr *fast_make_int(int64_t value);
static struct hir_expr *fast_make_binary(int type, struct hir_expr *left, struct hir_expr *right);

/*
 * Optimizes one checked fast-function body.
 *
 * A checked multi-axis helper is replaced only when every extent and index
 * domain is exact.  Every uncertain access keeps its ordinary checked path.
 */
bool
hir_fast_func_pass(
	struct hir_block *func_block)
{
	struct fast_ctx ctx;

	assert(func_block != NULL);
	assert(func_block->type == HIR_BLOCK_FUNC);

	if (!func_block->val.func.is_fast)
		return true;
	if (func_block->val.func.inner == NULL)
		return true;

	memset(&ctx, 0, sizeof(ctx));
	ctx.func = func_block;
	ctx.line = func_block->line;

	return fast_rewrite_chain(&ctx, func_block->val.func.inner);
}

/* Rewrite checked indices in one sibling block chain. */
static bool
fast_rewrite_chain(
	struct fast_ctx *ctx,
	struct hir_block *head)
{
	struct hir_block *block;
	struct hir_block *arm;
	struct hir_stmt *stmt;
	struct hir_opt_interval interval;
	size_t saved_binding_count;
	bool chain_conditional;
	bool saved_conditional;
	bool exact_loop;
	bool control_leaves;

	block = head;
	chain_conditional = ctx->conditional;

	/* Rewrite every reachable block in this sibling chain. */
	while (block != NULL) {
		ctx->line = block->line;

		/* Rewrite the expressions owned by this block shape. */
		switch (block->type) {
		case HIR_BLOCK_BASIC:
			stmt = block->val.basic.stmt_list;

			/* Rewrite every statement in source order. */
			while (stmt != NULL) {
				ctx->line = stmt->line;
				if (!fast_rewrite_expr(ctx, &stmt->lhs))
					return false;
				if (!fast_rewrite_expr(ctx, &stmt->rhs))
					return false;

				stmt = stmt->next;
			}
			break;
		case HIR_BLOCK_IF:
			saved_conditional = ctx->conditional;
			ctx->conditional = true;
			control_leaves = false;
			arm = block;

			/* Preserve checks throughout every conditional arm. */
			while (arm != NULL) {
				ctx->line = arm->line;
				if (!fast_rewrite_expr(ctx, &arm->val.if_.cond))
					return false;
				if (!fast_rewrite_chain(ctx, arm->val.if_.inner))
					return false;
				if (fast_chain_has_control_edge(arm->val.if_.inner))
					control_leaves = true;

				arm = arm->val.if_.chain_next;
			}

			ctx->conditional = saved_conditional;
			if (control_leaves)
				ctx->conditional = true;
			break;
		case HIR_BLOCK_FOR:
			if (!fast_rewrite_expr(ctx, &block->val.for_.start))
				return false;
			if (!fast_rewrite_expr(ctx, &block->val.for_.stop))
				return false;
			if (!fast_rewrite_expr(ctx, &block->val.for_.collection))
				return false;

			saved_binding_count = ctx->binding_count;
			saved_conditional = ctx->conditional;
			exact_loop = hir_opt_ranged_loop_interval(block, &interval);
			if (exact_loop &&
			    fast_chain_has_control_edge(block->val.for_.inner)) {
				exact_loop = false;
			}
			if (exact_loop &&
			    fast_chain_assigns_symbol(
				block->val.for_.inner,
				block->val.for_.counter_symbol)) {
				exact_loop = false;
			}
			if (!exact_loop) {
				ctx->conditional = true;
			} else if (ctx->binding_count < FAST_LOOP_DEPTH_MAX) {
				ctx->binding[ctx->binding_count].symbol =
					block->val.for_.counter_symbol;
				ctx->binding[ctx->binding_count].interval = interval;
				ctx->binding_count++;
			}

			if (!fast_rewrite_chain(ctx, block->val.for_.inner))
				return false;

			ctx->binding_count = saved_binding_count;
			ctx->conditional = saved_conditional;
			if (fast_chain_has_control_edge(block->val.for_.inner))
				ctx->conditional = true;
			break;
		case HIR_BLOCK_WHILE:
			saved_conditional = ctx->conditional;
			ctx->conditional = true;
			if (!fast_rewrite_expr(ctx, &block->val.while_.cond))
				return false;
			if (!fast_rewrite_chain(ctx, block->val.while_.inner))
				return false;

			ctx->conditional = saved_conditional;
			if (fast_chain_has_control_edge(block->val.while_.inner))
				ctx->conditional = true;
			break;
		default:
			break;
		}

		if (block->stop)
			break;
		block = block->succ;
	}

	ctx->conditional = chain_conditional;

	return true;
}

/* Test whether a structural block chain contains a control edge. */
static bool
fast_chain_has_control_edge(
	const struct hir_block *head)
{
	const struct hir_block *block;
	const struct hir_block *arm;

	block = head;

	/* Search every reachable block in this sibling chain. */
	while (block != NULL) {
		if (block->is_return_edge ||
		    block->is_break_edge ||
		    block->is_continue_edge) {
			return true;
		}

		/* Search the nested blocks owned by this block shape. */
		switch (block->type) {
		case HIR_BLOCK_IF:
			arm = block;

			/* Search every arm in the conditional chain. */
			while (arm != NULL) {
				if (fast_chain_has_control_edge(arm->val.if_.inner))
					return true;

				arm = arm->val.if_.chain_next;
			}
			break;
		case HIR_BLOCK_FOR:
			if (fast_chain_has_control_edge(block->val.for_.inner))
				return true;
			break;
		case HIR_BLOCK_WHILE:
			if (fast_chain_has_control_edge(block->val.while_.inner))
				return true;
			break;
		default:
			break;
		}

		if (block->stop)
			break;
		block = block->succ;
	}

	return false;
}

/* Test whether a structured block chain assigns one local symbol. */
static bool
fast_chain_assigns_symbol(
	const struct hir_block *head,
	const char *symbol)
{
	const struct hir_block *block;
	const struct hir_block *arm;
	const struct hir_stmt *stmt;

	if (symbol == NULL)
		return true;

	block = head;

	/* Search every reachable block without following loop back-edges. */
	while (block != NULL) {
		if (block->type == HIR_BLOCK_BASIC) {
			stmt = block->val.basic.stmt_list;

			/* Search every direct local assignment in source order. */
			while (stmt != NULL) {
				if (fast_term_is_symbol(stmt->lhs, symbol))
					return true;

				stmt = stmt->next;
			}
		} else if (block->type == HIR_BLOCK_IF) {
			arm = block;

			/* Search every conditional arm. */
			while (arm != NULL) {
				if (fast_chain_assigns_symbol(
					arm->val.if_.inner,
					symbol)) {
					return true;
				}

				arm = arm->val.if_.chain_next;
			}
		} else if (block->type == HIR_BLOCK_FOR) {
			if (fast_chain_assigns_symbol(
				block->val.for_.inner,
				symbol)) {
				return true;
			}
		} else if (block->type == HIR_BLOCK_WHILE) {
			if (fast_chain_assigns_symbol(
				block->val.while_.inner,
				symbol)) {
				return true;
			}
		}

		if (block->stop)
			break;
		block = block->succ;
	}

	return false;
}

/* Rewrite checked indices recursively in one expression. */
static bool
fast_rewrite_expr(
	struct fast_ctx *ctx,
	struct hir_expr **slot)
{
	struct hir_expr *expr;
	uint32_t i;

	if (slot == NULL)
		return true;
	if (*slot == NULL)
		return true;

	expr = *slot;

	/* Rewrite children according to this expression's storage shape. */
	switch (expr->type) {
	case HIR_EXPR_TERM:
		return true;
	case HIR_EXPR_NEG:
	case HIR_EXPR_NOT:
	case HIR_EXPR_PAR:
	case HIR_EXPR_PBASE:
	case HIR_EXPR_PLEN:
		return fast_rewrite_expr(ctx, &expr->val.unary.expr);
	case HIR_EXPR_CAPTURE:
		return fast_rewrite_expr(ctx, &expr->val.capture.expr);
	case HIR_EXPR_DOT:
		return fast_rewrite_expr(ctx, &expr->val.dot.obj);
	case HIR_EXPR_SUBSCR:
		if (!fast_rewrite_expr(ctx, &expr->val.binary.expr[0]))
			return false;
		if (!fast_rewrite_expr(ctx, &expr->val.binary.expr[1]))
			return false;

		return fast_check_rank_one_subscript(ctx, expr);
	case HIR_EXPR_CALL:
		if (!fast_rewrite_expr(ctx, &expr->val.call.func))
			return false;

		/* Rewrite every eagerly evaluated call argument. */
		for (i = 0; i < expr->val.call.arg_count; i++) {
			if (!fast_rewrite_expr(ctx, &expr->val.call.arg[i]))
				return false;
		}

		return fast_try_index_call(ctx, slot);
	case HIR_EXPR_THISCALL:
		if (!fast_rewrite_expr(ctx, &expr->val.thiscall.obj))
			return false;

		/* Rewrite every eagerly evaluated method argument. */
		for (i = 0; i < expr->val.thiscall.arg_count; i++) {
			if (!fast_rewrite_expr(ctx, &expr->val.thiscall.arg[i]))
				return false;
		}

		return true;
	case HIR_EXPR_ARRAY:

		/* Rewrite every eagerly evaluated array element. */
		for (i = 0; i < expr->val.array.elem_count; i++) {
			if (!fast_rewrite_expr(ctx, &expr->val.array.elem[i]))
				return false;
		}

		return true;
	case HIR_EXPR_DICT:

		/* Rewrite every eagerly evaluated dictionary value. */
		for (i = 0; i < expr->val.dict.kv_count; i++) {
			if (!fast_rewrite_expr(ctx, &expr->val.dict.value[i]))
				return false;
		}

		return true;
	case HIR_EXPR_NEW:
		return fast_rewrite_expr(ctx, &expr->val.new_.init);
	case HIR_EXPR_LAND:
	case HIR_EXPR_LOR:
		if (!fast_rewrite_conditional_expr(
			    ctx,
			    &expr->val.binary.expr[0])) {
			return false;
		}

		return fast_rewrite_conditional_expr(
			ctx,
			&expr->val.binary.expr[1]);
	case HIR_EXPR_SELECT:
		if (!fast_rewrite_conditional_expr(ctx, &expr->val.select.cond))
			return false;
		if (!fast_rewrite_conditional_expr(ctx, &expr->val.select.if_true))
			return false;

		return fast_rewrite_conditional_expr(
			ctx,
			&expr->val.select.if_false);
	case HIR_EXPR_PMASKSTORE32:
		if (!fast_rewrite_expr(ctx, &expr->val.mask_store.base))
			return false;
		if (!fast_rewrite_expr(ctx, &expr->val.mask_store.offset))
			return false;

		return fast_rewrite_expr(ctx, &expr->val.mask_store.mask);
	case HIR_EXPR_PGATHER32:
		if (!fast_rewrite_expr(ctx, &expr->val.gather.base))
			return false;
		if (!fast_rewrite_expr(ctx, &expr->val.gather.length))
			return false;
		if (!fast_rewrite_expr(ctx, &expr->val.gather.index))
			return false;

		return fast_rewrite_expr(ctx, &expr->val.gather.packed);
	case HIR_EXPR_VINDUCTF32:
		if (!fast_rewrite_expr(ctx, &expr->val.binary.expr[0]))
			return false;

		return fast_rewrite_expr(ctx, &expr->val.binary.expr[1]);
	default:
		if (!fast_rewrite_expr(ctx, &expr->val.binary.expr[0]))
			return false;

		return fast_rewrite_expr(ctx, &expr->val.binary.expr[1]);
	}
}

/* Rewrite an expression without trusting facts across conditional control. */
static bool
fast_rewrite_conditional_expr(
	struct fast_ctx *ctx,
	struct hir_expr **slot)
{
	bool saved_conditional;
	bool result;

	saved_conditional = ctx->conditional;
	ctx->conditional = true;
	result = fast_rewrite_expr(ctx, slot);
	ctx->conditional = saved_conditional;

	return result;
}

/* Diagnose an unconditional rank-one access with an exact bad domain. */
static bool
fast_check_rank_one_subscript(
	struct fast_ctx *ctx,
	const struct hir_expr *expr)
{
	const struct fast_signature *signature;
	const struct fast_param_contract *contract;
	const struct hir_expr *base;
	const char *symbol;
	int64_t extent;
	uint32_t i;
	int proof;

	if (ctx->conditional)
		return true;

	base = expr->val.binary.expr[0];
	if (base == NULL)
		return true;
	if (base->type != HIR_EXPR_TERM)
		return true;
	if (base->val.term.term == NULL)
		return true;
	if (base->val.term.term->type != HIR_TERM_SYMBOL)
		return true;

	symbol = base->val.term.term->val.symbol;
	if (symbol == NULL)
		return true;

	signature = fast_info_signature(ctx->func->val.func.fast_info);
	if (signature == NULL)
		return true;
	if (!signature->valid)
		return true;
	if (signature->param_count != ctx->func->val.func.param_count)
		return true;

	contract = NULL;

	/* Find the exact shaped parameter owning this subscript. */
	for (i = 0; i < signature->param_count; i++) {
		if (strcmp(ctx->func->val.func.param_name[i], symbol) != 0)
			continue;

		contract = &signature->param[i];
		break;
	}

	if (contract == NULL)
		return true;
	if (contract->rank != 1)
		return true;
	if (contract->extent == NULL)
		return true;
	if (contract->extent[0].kind != FAST_EXTENT_CONST)
		return true;

	extent = contract->extent[0].value.constant;
	if (extent <= 0)
		return true;

	proof = hir_opt_prove_index_bounds(
		expr->val.binary.expr[1],
		ctx->binding,
		ctx->binding_count,
		extent);
	if (proof != HIR_OPT_BOUNDS_EXCEEDS)
		return true;

	hir_error(
		ctx->line,
		N_TR("A __fast packed access is provably out of bounds."));

	return false;
}

/* Replace one fully proven checked index helper. */
static bool
fast_try_index_call(
	struct fast_ctx *ctx,
	struct hir_expr **slot)
{
	struct hir_expr *expr;
	struct hir_expr *replacement;
	int64_t extent[FAST_INDEX_RANK_MAX];
	int rank;
	int axis;
	int proof;
	bool all_within;

	if (ctx->conditional)
		return true;

	expr = *slot;
	rank = fast_index_rank(expr);
	if (rank == 0)
		return true;
	if (expr->val.call.arg_count != (uint32_t)(rank * 2))
		return true;

	all_within = true;

	/* Prove every axis against its exact positive extent. */
	for (axis = 0; axis < rank; axis++) {
		if (!hir_opt_expr_constant_i64(
			expr->val.call.arg[axis * 2 + 1],
			&extent[axis])) {
			all_within = false;
			continue;
		}
		if (extent[axis] <= 0) {
			all_within = false;
			continue;
		}

		proof = hir_opt_prove_index_bounds(
			expr->val.call.arg[axis * 2],
			ctx->binding,
			ctx->binding_count,
			extent[axis]);
		if (proof == HIR_OPT_BOUNDS_EXCEEDS) {
			hir_error(
				ctx->line,
				N_TR("A __fast packed access is provably out of bounds."));
			return false;
		}
		if (proof != HIR_OPT_BOUNDS_WITHIN)
			all_within = false;
	}

	if (!all_within)
		return true;
	if (!fast_product_fits(extent, rank))
		return true;

	replacement = fast_build_index(expr->val.call.arg, extent, rank);
	if (replacement == NULL)
		return false;

	*slot = replacement;

	return true;
}

/* Return the rank encoded by an internal checked helper call. */
static int
fast_index_rank(
	const struct hir_expr *expr)
{
	const struct hir_expr *func;
	const char *name;
	size_t prefix_length;
	int rank;

	if (expr == NULL)
		return 0;
	if (expr->type != HIR_EXPR_CALL)
		return 0;

	func = expr->val.call.func;
	if (func == NULL)
		return 0;

	name = NULL;
	if (func->type == HIR_EXPR_DOT) {
		if (!fast_term_is_symbol(func->val.dot.obj, "$Fast"))
			return 0;

		name = func->val.dot.symbol;
		if (name == NULL)
			return 0;

		prefix_length = strlen("index");
		if (strncmp(name, "index", prefix_length) != 0)
			return 0;

		name += prefix_length;
	} else if (func->type == HIR_EXPR_TERM &&
		   func->val.term.term != NULL &&
		   func->val.term.term->type == HIR_TERM_SYMBOL) {
		name = func->val.term.term->val.symbol;
		if (name == NULL)
			return 0;

		prefix_length = strlen("$Fast.index");
		if (strncmp(name, "$Fast.index", prefix_length) != 0)
			return 0;

		name += prefix_length;
	} else {
		return 0;
	}

	if (name[0] < '0' || name[0] > '9')
		return 0;
	if (name[1] != '\0')
		return 0;

	rank = name[0] - '0';
	if (rank < FAST_INDEX_RANK_MIN || rank > FAST_INDEX_RANK_MAX)
		return 0;

	return rank;
}

/* Test whether an expression is one exact symbol term. */
static bool
fast_term_is_symbol(
	const struct hir_expr *expr,
	const char *symbol)
{
	if (expr == NULL)
		return false;
	if (expr->type != HIR_EXPR_TERM)
		return false;
	if (expr->val.term.term == NULL)
		return false;
	if (expr->val.term.term->type != HIR_TERM_SYMBOL)
		return false;
	if (expr->val.term.term->val.symbol == NULL)
		return false;
	if (strcmp(expr->val.term.term->val.symbol, symbol) != 0)
		return false;

	return true;
}

/* Prove that row-major arithmetic cannot overflow a signed int. */
static bool
fast_product_fits(
	const int64_t *extent,
	int rank)
{
	int64_t product;
	int axis;

	product = 1;

	/* Accumulate the full element count without signed overflow. */
	for (axis = 0; axis < rank; axis++) {
		if (extent[axis] <= 0)
			return false;
		if (extent[axis] > INT_MAX)
			return false;
		if (product > INT_MAX / extent[axis])
			return false;

		product *= extent[axis];
	}

	return true;
}

/* Build an int-valued row-major expression from proven helper arguments. */
static struct hir_expr *
fast_build_index(
	struct hir_expr *const *arg,
	const int64_t *extent,
	int rank)
{
	struct hir_expr *flat;
	struct hir_expr *dimension;
	struct hir_expr *scaled;
	int64_t flat_value;
	int64_t index_value;
	int64_t scaled_value;
	int axis;
	bool flat_constant;

	flat = arg[0];
	flat_constant = hir_opt_expr_constant_i64(flat, &flat_value);

	/* Fold every trailing axis into the row-major offset. */
	for (axis = 1; axis < rank; axis++) {
		if (flat_constant) {
			scaled_value = flat_value * extent[axis];
			scaled = fast_make_int(scaled_value);
			if (scaled == NULL)
				return NULL;
		} else {
			dimension = fast_make_int(extent[axis]);
			if (dimension == NULL)
				return NULL;

			scaled = fast_make_binary(HIR_EXPR_MUL, flat, dimension);
			if (scaled == NULL)
				return NULL;
		}

		if (flat_constant &&
		    hir_opt_expr_constant_i64(arg[axis * 2], &index_value)) {
			flat_value = scaled_value + index_value;
			flat = fast_make_int(flat_value);
			if (flat == NULL)
				return NULL;

			continue;
		}

		flat = fast_make_binary(
			HIR_EXPR_PLUS,
			scaled,
			arg[axis * 2]);
		if (flat == NULL)
			return NULL;

		flat_constant = false;
	}

	return flat;
}

/* Allocate one signed int literal. */
static struct hir_expr *
fast_make_int(
	int64_t value)
{
	struct hir_expr *expr;
	struct hir_term *term;

	expr = hir_malloc(sizeof(*expr));
	if (expr == NULL) {
		hir_out_of_memory();
		return NULL;
	}

	term = hir_malloc(sizeof(*term));
	if (term == NULL) {
		hir_out_of_memory();
		return NULL;
	}

	memset(expr, 0, sizeof(*expr));
	expr->type = HIR_EXPR_TERM;
	expr->val.term.term = term;

	memset(term, 0, sizeof(*term));
	term->type = HIR_TERM_INT;
	term->val.i = (int)value;

	return expr;
}

/* Allocate one binary arithmetic expression. */
static struct hir_expr *
fast_make_binary(
	int type,
	struct hir_expr *left,
	struct hir_expr *right)
{
	struct hir_expr *expr;

	if (left == NULL)
		return NULL;
	if (right == NULL)
		return NULL;

	expr = hir_malloc(sizeof(*expr));
	if (expr == NULL) {
		hir_out_of_memory();
		return NULL;
	}

	memset(expr, 0, sizeof(*expr));
	expr->type = type;
	expr->val.binary.expr[0] = left;
	expr->val.binary.expr[1] = right;

	return expr;
}
