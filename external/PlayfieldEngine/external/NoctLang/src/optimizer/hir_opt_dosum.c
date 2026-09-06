/* -*- coding: utf-8; tab-width: 8; indent-tabs-mode: t; -*- */

/*
 * Noct Programming Language
 * Copyright (c) 2025, 2026, Awe Morris
 */

/*
 * Target-neutral canonical additive reduction recognition.
 */

#include "hir_opt_parallel.h"

#include <string.h>

#define HIR_DOSUM_MAX_VISITED 1024

struct hir_dosum_scan {
	const struct hir_loop_summary *summary;
	const char *symbol;
	uint32_t update_count;
	const struct hir_stmt *update;
	struct hir_block *update_block;
	struct hir_block *visited[HIR_DOSUM_MAX_VISITED];
	uint32_t visited_count;
};

static const char *hir_dosum_symbol(const struct hir_expr *expr);
static const struct hir_expr *hir_dosum_unwrap(const struct hir_expr *expr);
static bool hir_dosum_zero(const struct hir_expr *expr, int scalar_kind);
static bool hir_dosum_contains(const struct hir_expr *expr, const char *symbol);
static bool hir_dosum_within(const struct hir_block *block, const struct hir_block *loop);
static bool hir_dosum_scan_block(struct hir_dosum_scan *scan, struct hir_block *block);
static bool hir_dosum_preceded_by_decl(struct hir_block *block, struct hir_block *loop, const struct hir_stmt *decl, struct hir_block **visited, uint32_t *visited_count);
static const struct hir_expr *hir_dosum_mapped_expr(const struct hir_stmt *stmt, const char *symbol);

/*
 * Classifies a loop as a canonical additive reduction.
 */
bool
hir_dosum_classify(
	const struct hir_loop_summary *summary,
	struct hir_dosum_result *result)
{
	const struct hir_scalar_effect *effect;
	const struct hir_scalar_effect *candidate_effect;
	struct hir_local *local;
	struct hir_local *candidate;
	struct hir_dosum_scan scan;
	struct hir_block *visited[HIR_DOSUM_MAX_VISITED];
	struct hir_block *parent;
	uint32_t visited_count;
	uint32_t i;
	uint32_t candidates;
	const struct hir_expr *mapped;
	bool supported_type;

	if (summary == NULL)
		return false;
	if (result == NULL)
		return false;

	memset(result, 0, sizeof(*result));
	result->classification = HIR_PAR_CLASS_UNKNOWN;
	result->reason = HIR_PAR_REASON_REDUCTION_SHAPE;
	result->operator_ = HIR_REDUCTION_NONE;
	result->value_type = HIR_DECL_SCALAR_UNKNOWN;

	if (summary->analysis_status != HIR_ANALYSIS_COMPLETE) {
		result->reason = summary->analysis_reason;
		return true;
	}
	if (summary->has_nested_loop) {
		result->reason = HIR_PAR_REASON_REDUCTION_EFFECT;
		return true;
	}
	if (summary->has_while_loop) {
		result->reason = HIR_PAR_REASON_REDUCTION_EFFECT;
		return true;
	}

	/* Reject loops that write memory. */
	for (i = 0; i < summary->access_count; i++) {
		if (summary->access[i].kind == HIR_MEMORY_WRITE) {
			result->classification = HIR_PAR_CLASS_DEPENDENT;
			result->reason = HIR_PAR_REASON_REDUCTION_EFFECT;
			return true;
		}
	}

	/* Reject loops that make impure calls. */
	for (i = 0; i < summary->call_count; i++) {
		if (!summary->call[i].is_pure) {
			result->reason = HIR_PAR_REASON_REDUCTION_EFFECT;
			return true;
		}
	}

	candidate = NULL;
	candidate_effect = NULL;
	candidates = 0;

	/* Find the single scalar accumulator candidate. */
	for (i = 0; i < summary->scalar_count; i++) {
		effect = &summary->scalar[i];
		if (effect->is_counter)
			continue;
		if (effect->writes != 1)
			continue;
		if (effect->reads == 0)
			continue;
		if (effect->declared_inside_loop)
			continue;

		local = summary->func->val.func.local;

		/* Resolve the scalar effect to its local declaration. */
		while (local != NULL) {
			if (strcmp(local->symbol, effect->symbol) == 0)
				break;
			local = local->next;
		}
		if (local == NULL)
			continue;
		if (local->declaration_kind != HIR_LOCAL_DECL_VAR)
			continue;

		candidates++;
		candidate = local;
		candidate_effect = effect;
	}

	if (candidates != 1) {
		result->classification = HIR_PAR_CLASS_DEPENDENT;
		result->reason = HIR_PAR_REASON_REDUCTION_SHAPE;
		return true;
	}
	if (candidate == NULL) {
		result->classification = HIR_PAR_CLASS_DEPENDENT;
		result->reason = HIR_PAR_REASON_REDUCTION_SHAPE;
		return true;
	}
	if (candidate_effect == NULL) {
		result->classification = HIR_PAR_CLASS_DEPENDENT;
		result->reason = HIR_PAR_REASON_REDUCTION_SHAPE;
		return true;
	}

	supported_type = false;

	/* Recognize the scalar types supported by the reduction pass. */
	switch (candidate->declared_scalar_kind) {
	case HIR_DECL_SCALAR_INT32:
	case HIR_DECL_SCALAR_UINT32:
	case HIR_DECL_SCALAR_FLOAT32:
		supported_type = true;
		break;
	default:
		break;
	}

	if (!supported_type) {
		result->classification = HIR_PAR_CLASS_DEPENDENT;
		result->reason = HIR_PAR_REASON_REDUCTION_TYPE;
		return true;
	}
	if (!hir_dosum_zero(
		candidate->initializer,
		candidate->declared_scalar_kind)) {
		result->classification = HIR_PAR_CLASS_DEPENDENT;
		result->reason = HIR_PAR_REASON_REDUCTION_IDENTITY;
		return true;
	}

	visited_count = 0;

	if (!hir_dosum_preceded_by_decl(
		summary->func->val.func.inner,
		summary->loop,
		candidate->declaration_stmt,
		visited,
		&visited_count)) {
		result->classification = HIR_PAR_CLASS_DEPENDENT;
		result->reason = HIR_PAR_REASON_REDUCTION_SHAPE;
		return true;
	}

	memset(&scan, 0, sizeof(scan));
	scan.summary = summary;
	scan.symbol = candidate->symbol;

	if (!hir_dosum_scan_block(&scan, summary->loop->val.for_.inner))
		return false;
	if (scan.update_count != 1) {
		result->classification = HIR_PAR_CLASS_DEPENDENT;
		result->reason = HIR_PAR_REASON_REDUCTION_SHAPE;
		return true;
	}
	if (scan.update == NULL) {
		result->classification = HIR_PAR_CLASS_DEPENDENT;
		result->reason = HIR_PAR_REASON_REDUCTION_SHAPE;
		return true;
	}

	parent = scan.update_block;

	/* Reject updates guarded by a conditional ancestor. */
	while (parent != NULL) {
		if (parent == summary->loop)
			break;

		if (parent->type == HIR_BLOCK_IF) {
			result->classification = HIR_PAR_CLASS_DEPENDENT;
			result->reason = HIR_PAR_REASON_REDUCTION_PATH;
			return true;
		}
		parent = parent->parent;
	}

	mapped = hir_dosum_mapped_expr(scan.update, candidate->symbol);
	if (mapped == NULL) {
		result->classification = HIR_PAR_CLASS_DEPENDENT;
		result->reason = HIR_PAR_REASON_REDUCTION_SHAPE;
		return true;
	}

	/* Reject additional scalar effects outside the reduction. */
	for (i = 0; i < summary->scalar_count; i++) {
		effect = &summary->scalar[i];
		if (effect == candidate_effect)
			continue;
		if (effect->is_counter)
			continue;
		if (effect->writes == 0)
			continue;
		if (effect->declared_inside_loop)
			continue;

		result->classification = HIR_PAR_CLASS_DEPENDENT;
		result->reason = HIR_PAR_REASON_REDUCTION_EFFECT;
		return true;
	}

	result->classification = HIR_PAR_CLASS_DOSUM;
	result->reason = HIR_PAR_REASON_NONE;
	result->operator_ = HIR_REDUCTION_ADD;
	result->value_type = candidate->declared_scalar_kind;
	result->accumulator_symbol = candidate->symbol;
	result->identity = candidate->initializer;
	result->mapped_expr = mapped;
	result->trip_expr = summary->stop;
	result->line = scan.update->line;
	result->post_loop_use = true;

	return true;
}

static const char *
hir_dosum_symbol(
	const struct hir_expr *expr)
{
	expr = hir_dosum_unwrap(expr);
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

static const struct hir_expr *
hir_dosum_unwrap(
	const struct hir_expr *expr)
{

	/* Remove redundant parenthesized expressions. */
	while (expr != NULL && expr->type == HIR_EXPR_PAR)
		expr = expr->val.unary.expr;

	return expr;
}

static bool
hir_dosum_zero(
	const struct hir_expr *expr,
	int scalar_kind)
{
	const struct hir_term *term;

	expr = hir_dosum_unwrap(expr);
	if (expr == NULL)
		return false;
	if (expr->type != HIR_EXPR_TERM)
		return false;
	if (expr->val.term.term == NULL)
		return false;

	term = expr->val.term.term;
	if (scalar_kind == HIR_DECL_SCALAR_FLOAT32) {
		if (term->type != HIR_TERM_FLOAT)
			return false;
		if (term->val.f != 0.0f)
			return false;

		return true;
	}

	/* Check the zero representation for each integer scalar kind. */
	switch (scalar_kind) {
	case HIR_DECL_SCALAR_INT32:
	case HIR_DECL_SCALAR_UINT32:
		if (term->type != HIR_TERM_INT)
			return false;
		if (term->val.i != 0)
			return false;

		return true;
	default:
		break;
	}

	return false;
}

static bool
hir_dosum_contains(
	const struct hir_expr *expr,
	const char *symbol)
{
	uint32_t i;
	const char *term_symbol;

	if (expr == NULL)
		return false;

	/* Search the expression shape recursively. */
	switch (expr->type) {
	case HIR_EXPR_TERM:
		term_symbol = hir_dosum_symbol(expr);
		if (term_symbol == NULL)
			return false;
		if (strcmp(term_symbol, symbol) != 0)
			return false;

		return true;
	case HIR_EXPR_LT:
	case HIR_EXPR_LTE:
	case HIR_EXPR_GT:
	case HIR_EXPR_GTE:
	case HIR_EXPR_EQ:
	case HIR_EXPR_NEQ:
	case HIR_EXPR_PLUS:
	case HIR_EXPR_MINUS:
	case HIR_EXPR_MUL:
	case HIR_EXPR_DIV:
	case HIR_EXPR_MOD:
	case HIR_EXPR_AND:
	case HIR_EXPR_OR:
	case HIR_EXPR_LAND:
	case HIR_EXPR_LOR:
	case HIR_EXPR_XOR:
	case HIR_EXPR_SHL:
	case HIR_EXPR_SHR:
	case HIR_EXPR_SUBSCR:
	case HIR_EXPR_PCHECK:
	case HIR_EXPR_TYPEIS:
	case HIR_EXPR_PLOAD8U:
	case HIR_EXPR_PSTORE8:
	case HIR_EXPR_PLOAD8S:
	case HIR_EXPR_PLOAD16U:
	case HIR_EXPR_PLOAD16S:
	case HIR_EXPR_PLOAD32:
	case HIR_EXPR_PLOAD64:
	case HIR_EXPR_PLOADF32:
	case HIR_EXPR_PSTORE16:
	case HIR_EXPR_PSTORE32:
	case HIR_EXPR_PSTORE64:
	case HIR_EXPR_PSTOREF32:
		if (hir_dosum_contains(expr->val.binary.expr[0], symbol))
			return true;
		if (hir_dosum_contains(expr->val.binary.expr[1], symbol))
			return true;

		return false;
	case HIR_EXPR_NEG:
	case HIR_EXPR_NOT:
	case HIR_EXPR_PAR:
	case HIR_EXPR_PBASE:
	case HIR_EXPR_PLEN:
	case HIR_EXPR_VINDUCTF32:
		return hir_dosum_contains(expr->val.unary.expr, symbol);
	case HIR_EXPR_DOT:
		return hir_dosum_contains(expr->val.dot.obj, symbol);
	case HIR_EXPR_CALL:
		if (hir_dosum_contains(expr->val.call.func, symbol))
			return true;

		/* Search every ordinary call argument. */
		for (i = 0; i < expr->val.call.arg_count; i++) {
			if (hir_dosum_contains(expr->val.call.arg[i], symbol))
				return true;
		}

		return false;
	case HIR_EXPR_THISCALL:
		if (hir_dosum_contains(expr->val.thiscall.obj, symbol))
			return true;

		/* Search every method-call argument. */
		for (i = 0; i < expr->val.thiscall.arg_count; i++) {
			if (hir_dosum_contains(expr->val.thiscall.arg[i], symbol))
				return true;
		}

		return false;
	case HIR_EXPR_CAPTURE:
		if (strcmp(expr->val.capture.symbol, symbol) == 0)
			return true;
		if (hir_dosum_contains(expr->val.capture.expr, symbol))
			return true;

		return false;
	case HIR_EXPR_SELECT:
		if (hir_dosum_contains(expr->val.select.cond, symbol))
			return true;
		if (hir_dosum_contains(expr->val.select.if_true, symbol))
			return true;
		if (hir_dosum_contains(expr->val.select.if_false, symbol))
			return true;

		return false;
	case HIR_EXPR_ARRAY:

		/* Search every array element. */
		for (i = 0; i < expr->val.array.elem_count; i++) {
			if (hir_dosum_contains(expr->val.array.elem[i], symbol))
				return true;
		}

		return false;
	case HIR_EXPR_DICT:

		/* Search every dictionary value. */
		for (i = 0; i < expr->val.dict.kv_count; i++) {
			if (hir_dosum_contains(expr->val.dict.value[i], symbol))
				return true;
		}

		return false;
	case HIR_EXPR_NEW:
		return hir_dosum_contains(expr->val.new_.init, symbol);
	default:
		return false;
	}
}

static bool
hir_dosum_within(
	const struct hir_block *block,
	const struct hir_block *loop)
{

	/* Walk from the block to its lexical ancestors. */
	while (block != NULL) {
		if (block == loop)
			return true;
		block = block->parent;
	}

	return false;
}

static bool
hir_dosum_scan_block(
	struct hir_dosum_scan *scan,
	struct hir_block *block)
{
	uint32_t i;
	struct hir_stmt *stmt;
	struct hir_block *chain;
	const char *lhs;

	if (block == NULL)
		return true;
	if (block == scan->summary->loop)
		return true;
	if (!hir_dosum_within(block, scan->summary->loop))
		return true;

	/* Stop when the traversal reaches an already visited block. */
	for (i = 0; i < scan->visited_count; i++) {
		if (scan->visited[i] == block)
			return true;
	}
	if (scan->visited_count >= HIR_DOSUM_MAX_VISITED)
		return false;

	scan->visited[scan->visited_count++] = block;

	if (block->type == HIR_BLOCK_BASIC) {
		stmt = block->val.basic.stmt_list;

		/* Find assignments to the accumulator in this basic block. */
		while (stmt != NULL) {
			lhs = hir_dosum_symbol(stmt->lhs);
			if (lhs != NULL) {
				if (strcmp(lhs, scan->symbol) == 0) {
					scan->update_count++;
					scan->update = stmt;
					scan->update_block = block;
				}
			}
			stmt = stmt->next;
		}
	} else if (block->type == HIR_BLOCK_IF) {
		chain = block;

		/* Scan every arm of the conditional chain. */
		while (chain != NULL) {
			if (!hir_dosum_scan_block(scan, chain->val.if_.inner))
				return false;
			chain = chain->val.if_.chain_next;
		}
	} else if (block->type == HIR_BLOCK_FOR) {
		if (!hir_dosum_scan_block(scan, block->val.for_.inner))
			return false;
	} else if (block->type == HIR_BLOCK_WHILE) {
		if (!hir_dosum_scan_block(scan, block->val.while_.inner))
			return false;
	}

	return hir_dosum_scan_block(scan, block->succ);
}

static bool
hir_dosum_preceded_by_decl(
	struct hir_block *block,
	struct hir_block *loop,
	const struct hir_stmt *decl,
	struct hir_block **visited,
	uint32_t *visited_count)
{
	uint32_t i;
	struct hir_stmt *stmt;
	struct hir_block *chain;

	if (block == NULL)
		return false;

	/* Stop when the search reaches an already visited block. */
	for (i = 0; i < *visited_count; i++) {
		if (visited[i] == block)
			return false;
	}
	if (*visited_count >= HIR_DOSUM_MAX_VISITED)
		return false;

	visited[(*visited_count)++] = block;

	if (block->type == HIR_BLOCK_BASIC) {
		if (block->succ == loop) {
			stmt = block->val.basic.stmt_list;
			if (stmt == NULL)
				return false;

			/* Find the final statement before the loop. */
			while (stmt->next != NULL)
				stmt = stmt->next;
			if (stmt == decl)
				return true;
		}
	}

	if (block->type == HIR_BLOCK_IF) {
		chain = block;

		/* Search every arm of the conditional chain. */
		while (chain != NULL) {
			if (hir_dosum_preceded_by_decl(
				chain->val.if_.inner,
				loop,
				decl,
				visited,
				visited_count))
				return true;
			chain = chain->val.if_.chain_next;
		}
	} else if (block->type == HIR_BLOCK_FOR) {
		if (hir_dosum_preceded_by_decl(
			block->val.for_.inner,
			loop,
			decl,
			visited,
			visited_count))
			return true;
	} else if (block->type == HIR_BLOCK_WHILE) {
		if (hir_dosum_preceded_by_decl(
			block->val.while_.inner,
			loop,
			decl,
			visited,
			visited_count))
			return true;
	}

	return hir_dosum_preceded_by_decl(
		block->succ,
		loop,
		decl,
		visited,
		visited_count);
}

static const struct hir_expr *
hir_dosum_mapped_expr(
	const struct hir_stmt *stmt,
	const char *symbol)
{
	const struct hir_expr *rhs;
	const struct hir_expr *left;
	const struct hir_expr *right;
	const char *left_symbol;
	const char *right_symbol;

	rhs = hir_dosum_unwrap(stmt->rhs);
	if (rhs == NULL)
		return NULL;
	if (rhs->type != HIR_EXPR_PLUS)
		return NULL;

	left = hir_dosum_unwrap(rhs->val.binary.expr[0]);
	right = hir_dosum_unwrap(rhs->val.binary.expr[1]);
	left_symbol = hir_dosum_symbol(left);
	right_symbol = hir_dosum_symbol(right);

	if (left_symbol != NULL) {
		if (strcmp(left_symbol, symbol) == 0) {
			if (right_symbol != NULL) {
				if (strcmp(right_symbol, symbol) == 0)
					return NULL;
			}
			if (hir_dosum_contains(right, symbol))
				return NULL;

			return right;
		}
	}

	if (right_symbol != NULL) {
		if (strcmp(right_symbol, symbol) == 0) {
			if (left_symbol != NULL) {
				if (strcmp(left_symbol, symbol) == 0)
					return NULL;
			}
			if (hir_dosum_contains(left, symbol))
				return NULL;

			return left;
		}
	}

	return NULL;
}
