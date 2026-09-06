/* -*- coding: utf-8; tab-width: 8; indent-tabs-mode: t; -*- */

/*
 * Noct Programming Language
 * Copyright (c) 2025, 2026, Awe Morris
 */

/*
 * Scalar Packed-loop unrolling.
 *
 * This pass runs after SIMD.  A loop which is still marked abce_fast was
 * bounds-check eliminated but rejected by the x4 vector grammar.  For the
 * deliberately small, side-effect-free integer subset below, split it into
 * a four-iteration bulk loop and a scalar tail.  Each Packed index is made
 * explicit as counter+lane; the JIT PLOOP scanner may later fold that
 * addition into a PBASE displacement.  The expanded HIR remains correct for
 * the interpreter and every backend which ignores the PLOOP hint.
 */

#include "hir_opt.h"

#include <assert.h>
#include <limits.h>
#include <string.h>

#define UNROLL_FACTOR		4
#define UNROLL_MAX_LOOPS	32
#define UNROLL_MAX_NODES	64
#define UNROLL_MAX_CLONED_NODES	(UNROLL_MAX_NODES * UNROLL_FACTOR)

struct unroll_ctx {
	struct hir_block *func;
	struct hir_block *loop;
	const char *counter;
	int nodes;
	int accesses;
	bool expensive_division;
};

static bool unroll_is_load(int type);
static bool unroll_is_store(int type);
static bool unroll_is_counter_term(const struct hir_expr *e, const char *counter);
static bool unroll_int_term(const struct hir_expr *e, int *value);
static bool unroll_parse_index(struct unroll_ctx *ctx, const struct hir_expr *e, int *offset);
static bool unroll_check_expr(struct unroll_ctx *ctx, const struct hir_expr *e);
static bool unroll_check_term(struct unroll_ctx *ctx, const struct hir_expr *e);
static bool unroll_check_body(struct unroll_ctx *ctx);
static struct hir_term *unroll_clone_term(const struct hir_term *term);
static struct hir_expr *unroll_mk_term_int(int value);
static struct hir_expr *unroll_mk_term_symbol(const char *symbol);
static struct hir_expr *unroll_mk_binary(int type, struct hir_expr *left, struct hir_expr *right);
static struct hir_expr *unroll_mk_index(struct unroll_ctx *ctx, int offset);
static struct hir_expr *unroll_clone_expr(struct unroll_ctx *ctx, const struct hir_expr *expr, int lane);
static struct hir_stmt *unroll_clone_stmts(struct unroll_ctx *ctx, const struct hir_stmt *source, int lane);
static bool unroll_append_stmts(struct hir_stmt **head, struct hir_stmt **tail, struct hir_stmt *list);
static struct hir_block *unroll_mk_basic(struct hir_block *parent, int line, struct hir_stmt *stmts);
static struct hir_block *unroll_mk_for(struct hir_block *parent, int line);
static struct hir_expr *unroll_clone_bound(const struct hir_expr *expr);
static struct hir_expr *unroll_mk_mid(const struct hir_expr *lo, const struct hir_expr *hi);
static bool unroll_transform(struct unroll_ctx *ctx);
static void unroll_collect(struct hir_block *head, struct hir_block **loops, int *count);

/*
 * Unrolls eligible scalar Packed loops.
 */
bool
hir_opt_unroll_func(
	struct hir_block *func_block)
{
	struct hir_block *loops[UNROLL_MAX_LOOPS];
	int count;
	int i;

	assert(func_block != NULL);
	assert(func_block->type == HIR_BLOCK_FUNC);

	if (func_block->val.func.inner == NULL)
		return true;

#if !defined(NOCT_ARCH_ARM64) && !defined(NOCT_ARCH_X86_64)
	/*
	 * Other architectures execute the portable expanded bytecode
	 * correctly, but have no offset-aware native lowering yet.
	 */
	return true;
#endif

	count = 0;
	unroll_collect(func_block->val.func.inner, loops, &count);

	/* Unroll each eligible scalar loop. */
	for (i = 0; i < count; i++) {
		struct unroll_ctx ctx;

		memset(&ctx, 0, sizeof(ctx));
		ctx.func = func_block;
		ctx.loop = loops[i];
		ctx.counter = loops[i]->val.for_.counter_symbol;

		if (ctx.counter == NULL) {
			loops[i]->val.for_.abce_fast = false;
			continue;
		}
		if (!unroll_check_body(&ctx)) {
			loops[i]->val.for_.abce_fast = false;
			continue;
		}

		if (!unroll_transform(&ctx))
			return false;

		if (ctx.loop->val.for_.scalar_unroll != UNROLL_FACTOR) {
			/* A non-allocation range-shape rejection leaves the loop intact. */
			ctx.loop->val.for_.abce_fast = false;
			continue;
		}
	}

	return true;
}

static bool
unroll_is_load(
	int type)
{
	if (type == HIR_EXPR_PLOAD8U)
		return true;
	if (type == HIR_EXPR_PLOAD8S)
		return true;
	if (type == HIR_EXPR_PLOAD16U)
		return true;
	if (type == HIR_EXPR_PLOAD16S)
		return true;
	if (type == HIR_EXPR_PLOAD32)
		return true;

	return false;
}

static bool
unroll_is_store(
	int type)
{
	if (type == HIR_EXPR_PSTORE8)
		return true;
	if (type == HIR_EXPR_PSTORE16)
		return true;
	if (type == HIR_EXPR_PSTORE32)
		return true;

	return false;
}

static bool
unroll_is_counter_term(
	const struct hir_expr *e,
	const char *counter)
{
	/* Skip redundant parenthesized expressions. */
	while (e != NULL && e->type == HIR_EXPR_PAR)
		e = e->val.unary.expr;
	if (e == NULL)
		return false;

	if (e->type != HIR_EXPR_TERM)
		return false;
	if (e->val.term.term == NULL)
		return false;
	if (e->val.term.term->type != HIR_TERM_SYMBOL)
		return false;
	if (strcmp(e->val.term.term->val.symbol, counter) != 0)
		return false;

	return true;
}

static bool
unroll_int_term(
	const struct hir_expr *e,
	int *value)
{
	/* Skip redundant parenthesized expressions. */
	while (e != NULL && e->type == HIR_EXPR_PAR)
		e = e->val.unary.expr;

	if (e == NULL)
		return false;
	if (e->type != HIR_EXPR_TERM)
		return false;
	if (e->val.term.term == NULL)
		return false;
	if (e->val.term.term->type != HIR_TERM_INT)
		return false;

	*value = e->val.term.term->val.i;

	return true;
}

/* Recognize i, i+C, C+i and i-C. */
static bool
unroll_parse_index(
	struct unroll_ctx *ctx,
	const struct hir_expr *e,
	int *offset)
{
	int value;

	/* Skip redundant parenthesized expressions. */
	while (e != NULL && e->type == HIR_EXPR_PAR)
		e = e->val.unary.expr;

	if (unroll_is_counter_term(e, ctx->counter)) {
		*offset = 0;
		return true;
	}

	if (e == NULL ||
	    (e->type != HIR_EXPR_PLUS &&
	     e->type != HIR_EXPR_MINUS))
		return false;

	if (unroll_is_counter_term(e->val.binary.expr[0], ctx->counter)) {
		if (!unroll_int_term(e->val.binary.expr[1], &value))
			return false;

		if (e->type == HIR_EXPR_MINUS) {
			if (value == INT_MIN)
				return false;
			value = -value;
		}

		*offset = value;

		return true;
	}

	if (e->type != HIR_EXPR_PLUS)
		return false;
	if (!unroll_int_term(e->val.binary.expr[0], &value))
		return false;
	if (!unroll_is_counter_term(e->val.binary.expr[1], ctx->counter))
		return false;

	*offset = value;

	return true;
}

static bool
unroll_check_term(
	struct unroll_ctx *ctx,
	const struct hir_expr *e)
{
	const struct hir_term *term;

	term = e->val.term.term;

	if (term == NULL)
		return false;

	if (term->type == HIR_TERM_SYMBOL) {
		/* The induction value is legal only as a Packed index. */
		if (strcmp(term->val.symbol, ctx->counter) == 0)
			return false;

		return true;
	}

	if (term->type == HIR_TERM_INT)
		return true;
	if (term->type == HIR_TERM_LONG)
		return true;

	return false;
}

static bool
unroll_check_expr(
	struct unroll_ctx *ctx,
	const struct hir_expr *e)
{
	int offset;
	bool is_memory;

	if (e == NULL)
		return false;

	if (++ctx->nodes > UNROLL_MAX_NODES)
		return false;

	is_memory = unroll_is_load(e->type);
	if (!is_memory)
		is_memory = unroll_is_store(e->type);

	if (is_memory) {
		const struct hir_expr *base = e->val.binary.expr[0];

		if (base == NULL ||
		    base->type != HIR_EXPR_TERM ||
		    base->val.term.term == NULL ||
		    base->val.term.term->type != HIR_TERM_SYMBOL) {
			return false;
		}
		if (!unroll_parse_index(ctx, e->val.binary.expr[1], &offset)) {
			return false;
		}
		if (offset > INT_MAX - (UNROLL_FACTOR - 1)) {
			return false;
		}

		ctx->accesses++;

		return true;
	}

	/* Validate the expression grammar recursively. */
	switch (e->type) {
	case HIR_EXPR_TERM:
		if (!unroll_check_term(ctx, e))
			return false;

		return true;
	case HIR_EXPR_PAR:
	case HIR_EXPR_NEG:
	case HIR_EXPR_NOT:
		if (!unroll_check_expr(ctx, e->val.unary.expr))
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
		if (!unroll_check_expr(ctx, e->val.binary.expr[0]))
			return false;
		if (!unroll_check_expr(ctx, e->val.binary.expr[1]))
			return false;

		return true;
	case HIR_EXPR_DIV:
	case HIR_EXPR_MOD:
		ctx->expensive_division = true;
		if (!unroll_check_expr(ctx, e->val.binary.expr[0]))
			return false;
		if (!unroll_check_expr(ctx, e->val.binary.expr[1]))
			return false;

		return true;
	case HIR_EXPR_AND:
	case HIR_EXPR_OR:
	case HIR_EXPR_LAND:
	case HIR_EXPR_LOR:
	case HIR_EXPR_XOR:
	case HIR_EXPR_SHL:
	case HIR_EXPR_SHR:
		if (!unroll_check_expr(ctx, e->val.binary.expr[0]))
			return false;
		if (!unroll_check_expr(ctx, e->val.binary.expr[1]))
			return false;

		return true;
	default:
		return false;
	}
}

static bool
unroll_check_body(
	struct unroll_ctx *ctx)
{
	struct hir_block *body;
	struct hir_stmt *stmt;

	body = ctx->loop->val.for_.inner;

	if (!ctx->loop->val.for_.is_ranged ||
	    !ctx->loop->val.for_.typed_int_region ||
	    ctx->loop->val.for_.packed_lanes != 1 ||
	    ctx->loop->val.for_.is_vector ||
	    body == NULL ||
	    body->type != HIR_BLOCK_BASIC ||
	    !body->stop ||
	    body->is_return_edge ||
	    body->is_break_edge ||
	    body->is_continue_edge) {
		return false;
	}

	/* Validate every statement in the scalar loop body. */
	for (stmt = body->val.basic.stmt_list; stmt != NULL;
	     stmt = stmt->next) {
		/* Phase one accepts only Packed stores, never scalar recurrence. */
		if (stmt->lhs == NULL)
			return false;
		if (!unroll_is_store(stmt->lhs->type))
			return false;
		if (!unroll_check_expr(ctx, stmt->lhs))
			return false;
		if (!unroll_check_expr(ctx, stmt->rhs))
			return false;
	}

	if (ctx->accesses == 0)
		return false;

	/*
	 * The current scalar register cache does not keep four
	 * variable-divide lanes live cheaply.  Measurements on x86_64
	 * and Apple M5 showed a substantial regression, so keep this
	 * profitability guard until the lane scheduler/allocator can
	 * expose division ILP.
	 */
	if (ctx->expensive_division)
		return false;

	if (ctx->nodes * UNROLL_FACTOR > UNROLL_MAX_CLONED_NODES)
		return false;

	return true;
}

static struct hir_term *
unroll_clone_term(
	const struct hir_term *term)
{
	struct hir_term *copy;

	copy = hir_malloc(sizeof(*copy));
	if (copy == NULL) {
		hir_out_of_memory();
		return NULL;
	}

	memset(copy, 0, sizeof(*copy));
	copy->type = term->type;

	/* Copy the payload owned by the term. */
	switch (term->type) {
	case HIR_TERM_SYMBOL:
		copy->val.symbol = hir_strdup(term->val.symbol);
		if (copy->val.symbol == NULL) {
			hir_out_of_memory();
			return NULL;
		}
		break;
	case HIR_TERM_INT:
		copy->val.i = term->val.i;
		break;
	case HIR_TERM_LONG:
		copy->val.l = term->val.l;
		break;
	default:
		return NULL;
	}

	return copy;
}

static struct hir_expr *
unroll_mk_term_int(
	int value)
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

	memset(term, 0, sizeof(*term));
	term->type = HIR_TERM_INT;
	term->val.i = value;

	memset(expr, 0, sizeof(*expr));
	expr->type = HIR_EXPR_TERM;
	expr->val.term.term = term;

	return expr;
}

static struct hir_expr *
unroll_mk_term_symbol(
	const char *symbol)
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

	memset(term, 0, sizeof(*term));
	term->type = HIR_TERM_SYMBOL;
	term->val.symbol = hir_strdup(symbol);
	if (term->val.symbol == NULL) {
		hir_out_of_memory();
		return NULL;
	}

	memset(expr, 0, sizeof(*expr));
	expr->type = HIR_EXPR_TERM;
	expr->val.term.term = term;

	return expr;
}

static struct hir_expr *
unroll_mk_binary(
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

static struct hir_expr *
unroll_mk_index(
	struct unroll_ctx *ctx,
	int offset)
{
	struct hir_expr *counter;
	struct hir_expr *value;

	counter = unroll_mk_term_symbol(ctx->counter);
	if (counter == NULL)
		return NULL;

	if (offset == 0)
		return counter;

	if (offset > 0) {
		value = unroll_mk_term_int(offset);
		if (value == NULL)
			return NULL;

		return unroll_mk_binary(
			HIR_EXPR_PLUS,
			counter,
			value);
	}

	if (offset == INT_MIN)
		return NULL;

	value = unroll_mk_term_int(-offset);
	if (value == NULL)
		return NULL;

	return unroll_mk_binary(
		HIR_EXPR_MINUS,
		counter,
		value);
}

static struct hir_expr *
unroll_clone_expr(
	struct unroll_ctx *ctx,
	const struct hir_expr *expr,
	int lane)
{
	struct hir_expr *copy;
	int offset;
	bool is_memory;

	if (expr == NULL)
		return NULL;

	copy = hir_malloc(sizeof(*copy));
	if (copy == NULL) {
		hir_out_of_memory();
		return NULL;
	}

	memset(copy, 0, sizeof(*copy));
	copy->type = expr->type;

	is_memory = unroll_is_load(expr->type);
	if (!is_memory)
		is_memory = unroll_is_store(expr->type);

	if (is_memory) {
		if (!unroll_parse_index(ctx, expr->val.binary.expr[1], &offset))
			return NULL;
		if (offset > INT_MAX - lane)
			return NULL;

		copy->val.binary.expr[0] =
			unroll_clone_expr(ctx, expr->val.binary.expr[0], 0);
		if (copy->val.binary.expr[0] == NULL)
			return NULL;

		copy->val.binary.expr[1] = unroll_mk_index(ctx, offset + lane);
		if (copy->val.binary.expr[1] == NULL)
			return NULL;

		return copy;
	}

	/* Clone the expression payload selected by its type. */
	switch (expr->type) {
	case HIR_EXPR_TERM:
		copy->val.term.term = unroll_clone_term(expr->val.term.term);
		if (copy->val.term.term == NULL)
			return NULL;

		return copy;
	case HIR_EXPR_PAR:
	case HIR_EXPR_NEG:
	case HIR_EXPR_NOT:
		copy->val.unary.expr =
			unroll_clone_expr(ctx, expr->val.unary.expr, lane);
		if (copy->val.unary.expr == NULL)
			return NULL;

		return copy;
	default:
		copy->val.binary.expr[0] =
			unroll_clone_expr(ctx, expr->val.binary.expr[0], lane);
		if (copy->val.binary.expr[0] == NULL)
			return NULL;

		copy->val.binary.expr[1] =
			unroll_clone_expr(ctx, expr->val.binary.expr[1], lane);
		if (copy->val.binary.expr[1] == NULL)
			return NULL;

		return copy;
	}
}

static struct hir_stmt *
unroll_clone_stmts(
	struct unroll_ctx *ctx,
	const struct hir_stmt *source,
	int lane)
{
	struct hir_stmt *head;
	struct hir_stmt *tail;

	head = NULL;
	tail = NULL;

	/* Clone every remaining source statement. */
	for (;
	     source != NULL;
	     source = source->next) {
		struct hir_stmt *copy;

		copy = hir_malloc(sizeof(*copy));
		if (copy == NULL) {
			hir_out_of_memory();
			return NULL;
		}

		memset(copy, 0, sizeof(*copy));
		copy->line = source->line;
		copy->is_bare_return = source->is_bare_return;

		copy->lhs = unroll_clone_expr(ctx, source->lhs, lane);
		if (copy->lhs == NULL)
			return NULL;

		copy->rhs = unroll_clone_expr(ctx, source->rhs, lane);
		if (copy->rhs == NULL)
			return NULL;

		if (tail == NULL)
			head = copy;
		else
			tail->next = copy;
		tail = copy;
	}

	return head;
}

static bool
unroll_append_stmts(
	struct hir_stmt **head,
	struct hir_stmt **tail,
	struct hir_stmt *list)
{
	if (list == NULL)
		return false;

	if (*tail == NULL)
		*head = list;
	else
		(*tail)->next = list;

	/* Find the final statement in the appended list. */
	while (list->next != NULL)
		list = list->next;

	*tail = list;

	return true;
}

static struct hir_block *
unroll_mk_basic(
	struct hir_block *parent,
	int line,
	struct hir_stmt *stmts)
{
	struct hir_block *block;

	block = hir_malloc(sizeof(*block));
	if (block == NULL) {
		hir_out_of_memory();
		return NULL;
	}

	memset(block, 0, sizeof(*block));
	block->type = HIR_BLOCK_BASIC;
	block->line = line;
	block->parent = parent;
	block->stop = true;
	block->id = hir_next_block_id();
	block->val.basic.stmt_list = stmts;
	block->succ = block;

	return block;
}

static struct hir_block *
unroll_mk_for(
	struct hir_block *parent,
	int line)
{
	struct hir_block *block;

	block = hir_malloc(sizeof(*block));
	if (block == NULL) {
		hir_out_of_memory();
		return NULL;
	}

	memset(block, 0, sizeof(*block));
	block->type = HIR_BLOCK_FOR;
	block->line = line;
	block->parent = parent;
	block->id = hir_next_block_id();

	return block;
}

static struct hir_expr *
unroll_clone_bound(
	const struct hir_expr *expr)
{
	struct hir_expr *copy;

	if (expr == NULL)
		return NULL;
	if (expr->type != HIR_EXPR_TERM)
		return NULL;
	if (expr->val.term.term == NULL)
		return NULL;

	copy = hir_malloc(sizeof(*copy));
	if (copy == NULL) {
		hir_out_of_memory();
		return NULL;
	}

	memset(copy, 0, sizeof(*copy));
	copy->type = HIR_EXPR_TERM;
	copy->val.term.term = unroll_clone_term(expr->val.term.term);
	if (copy->val.term.term == NULL)
		return NULL;

	return copy;
}

/* hi - ((hi - lo) & 3), built afresh for each FOR bound. */
static struct hir_expr *
unroll_mk_mid(
	const struct hir_expr *lo,
	const struct hir_expr *hi)
{
	struct hir_expr *upper;
	struct hir_expr *span_upper;
	struct hir_expr *span_lower;
	struct hir_expr *span;
	struct hir_expr *mask;
	struct hir_expr *remainder;

	upper = unroll_clone_bound(hi);
	if (upper == NULL)
		return NULL;

	span_upper = unroll_clone_bound(hi);
	if (span_upper == NULL)
		return NULL;

	span_lower = unroll_clone_bound(lo);
	if (span_lower == NULL)
		return NULL;

	span = unroll_mk_binary(
		HIR_EXPR_MINUS,
		span_upper,
		span_lower);
	if (span == NULL)
		return NULL;

	mask = unroll_mk_term_int(UNROLL_FACTOR - 1);
	if (mask == NULL)
		return NULL;

	remainder = unroll_mk_binary(
		HIR_EXPR_AND,
		span,
		mask);
	if (remainder == NULL)
		return NULL;

	return unroll_mk_binary(
		HIR_EXPR_MINUS,
		upper,
		remainder);
}

static bool
unroll_transform(
	struct unroll_ctx *ctx)
{
	struct hir_block *loop;
	struct hir_block *source_body;
	struct hir_block *bulk_body;
	struct hir_block *tail_loop;
	struct hir_block *tail_body;
	struct hir_block *old_succ;
	struct hir_expr *old_stop;
	struct hir_expr *bulk_mid;
	struct hir_expr *tail_mid;
	struct hir_expr *tail_stop;
	struct hir_stmt *bulk_head;
	struct hir_stmt *bulk_tail;
	struct hir_stmt *tail_stmts;
	int lane;

	loop = ctx->loop;
	source_body = loop->val.for_.inner;
	old_succ = loop->succ;
	old_stop = loop->val.for_.stop;
	bulk_head = NULL;
	bulk_tail = NULL;

	if (loop->val.for_.start == NULL ||
	    old_stop == NULL ||
	    loop->val.for_.start->type != HIR_EXPR_TERM ||
	    old_stop->type != HIR_EXPR_TERM) {
		return true;
	}

	bulk_mid = unroll_mk_mid(loop->val.for_.start, old_stop);
	if (bulk_mid == NULL)
		return false;

	tail_mid = unroll_mk_mid(loop->val.for_.start, old_stop);
	if (tail_mid == NULL)
		return false;

	tail_stop = unroll_clone_bound(old_stop);
	if (tail_stop == NULL)
		return false;

	/* Clone the scalar body for every lane in the bulk loop. */
	for (lane = 0; lane < UNROLL_FACTOR; lane++) {
		struct hir_stmt *copy;

		copy = unroll_clone_stmts(
			ctx,
			source_body->val.basic.stmt_list,
			lane);
		if (copy == NULL)
			return false;

		if (!unroll_append_stmts(&bulk_head, &bulk_tail, copy))
			return false;
	}

	tail_stmts = unroll_clone_stmts(
		ctx,
		source_body->val.basic.stmt_list,
		0);
	if (tail_stmts == NULL)
		return false;

	/* Allocate and fill every new node before mutating the old loop. */
	tail_loop = unroll_mk_for(loop->parent, loop->line);
	if (tail_loop == NULL)
		return false;

	bulk_body = unroll_mk_basic(loop, source_body->line, bulk_head);
	if (bulk_body == NULL)
		return false;

	tail_body = unroll_mk_basic(tail_loop, source_body->line, tail_stmts);
	if (tail_body == NULL)
		return false;

	tail_loop->val.for_.is_ranged = true;
	tail_loop->val.for_.counter_symbol = loop->val.for_.counter_symbol;
	tail_loop->val.for_.start = tail_mid;
	tail_loop->val.for_.stop = tail_stop;
	tail_loop->val.for_.typed_int_region = loop->val.for_.typed_int_region;
	tail_loop->val.for_.abce_fast = false;
	tail_loop->val.for_.is_vector = false;
	tail_loop->val.for_.packed_lanes = 1;
	tail_loop->val.for_.scalar_unroll = 1;
	tail_loop->val.for_.inner = tail_body;
	tail_loop->stop = loop->stop;
	tail_loop->succ = old_succ;
	tail_loop->is_return_edge = loop->is_return_edge;
	tail_loop->is_break_edge = loop->is_break_edge;
	tail_loop->is_continue_edge = loop->is_continue_edge;

	loop->val.for_.stop = bulk_mid;
	loop->val.for_.inner = bulk_body;
	loop->val.for_.abce_fast = false;
	loop->val.for_.scalar_unroll = UNROLL_FACTOR;
	loop->stop = false;
	loop->is_return_edge = false;
	loop->is_break_edge = false;
	loop->is_continue_edge = false;
	loop->succ = tail_loop;

	return true;
}

static void
unroll_collect(
	struct hir_block *head,
	struct hir_block **loops,
	int *count)
{
	struct hir_block *block;
	struct hir_block *chain;

	/* Traverse every block reachable through the structured HIR chains. */
	for (block = head;
	     block != NULL;
	     block = block->succ) {

		/* Recurse into the nested block shape. */
		switch (block->type) {
		case HIR_BLOCK_IF:

			/* Visit every arm in the conditional chain. */
			for (chain = block;
			     chain != NULL;
			     chain = chain->val.if_.chain_next) {
				if (chain->val.if_.inner != NULL) {
					unroll_collect(
						chain->val.if_.inner,
						loops,
						count);
				}
			}
			break;
		case HIR_BLOCK_FOR:
			if (block->val.for_.abce_fast &&
			    *count < UNROLL_MAX_LOOPS)
				loops[(*count)++] = block;
			if (block->val.for_.inner != NULL) {
				unroll_collect(
					block->val.for_.inner,
					loops,
					count);
			}
			break;
		case HIR_BLOCK_WHILE:
			if (block->val.while_.inner != NULL) {
				unroll_collect(
					block->val.while_.inner,
					loops,
					count);
			}
			break;
		default:
			break;
		}

		if (block->stop)
			break;
	}
}
