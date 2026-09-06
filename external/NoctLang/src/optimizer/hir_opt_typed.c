/* -*- coding: utf-8; tab-width: 8; indent-tabs-mode: t; -*- */

/*
 * Noct Programming Language
 * Copyright (c) 2025, 2026, Awe Morris
 */

/*
 * HIR Optimizer: Stage B type lattice.
 *
 * A flow-insensitive, optimistic fixpoint over the function's locals
 * on the lattice TOP > {INT, LONG, FLOAT, DOUBLE} > UNKNOWN:
 *
 *   - Every local starts at TOP ("no assignment seen yet").  A local
 *     still TOP at the end was never assigned and therefore always
 *     reads its zero-initialized value, integer 0 -> proven INT.
 *     (Every declaration carries an assignment in HIR: the annotated
 *     uninitialized form "var x: t;" is parsed as "x = 0", and the
 *     static TDZ (design 04) rejects reads before the declaration.)
 *   - Annotated parameters start at their annotation tag (sound at
 *     optimize level >= 2 because OP_CHECKTYPE runs at entry) and
 *     assignments meet into them, so a reassigned parameter loses
 *     its proof.  Unannotated parameters start at UNKNOWN (their
 *     incoming value is arbitrary).
 *   - Assignment edges: statement "local = expr", the ranged-for
 *     counter := start-expr (OP_INC preserves the tag, so the
 *     counter's tag is start's tag for the whole loop), for-each
 *     key/value := UNKNOWN, and every CSE CAPTURE node (it assigns
 *     its home local the inner expression's value).
 *
 * The pass only fills hir_local.proven_type; it mutates nothing
 * else.  The LIR generator consumes the proofs at level >= 2.
 */

#include "hir_opt_analysis.h"
#include "hir_opt.h"

#include <string.h>
#include <assert.h>

/* Lattice values.  Primitive values are the NOCT_VALUE_* tags. */
#define TP_TOP		(-2)
#define TP_UNKNOWN	(-1)
#define TP_INT		NOCT_VALUE_INT
#define TP_LONG		NOCT_VALUE_LONG
#define TP_FLOAT	NOCT_VALUE_FLOAT
#define TP_DOUBLE	NOCT_VALUE_DOUBLE

struct tp_ctx {
	struct hir_block *func;
	bool changed;
};

static int tp_combine(int cur, int t);
static bool tp_is_primitive(int t);
static int tp_promote_numeric(int a, int b);
static struct hir_local *tp_find_local(struct tp_ctx *ctx, const char *symbol);
static int tp_eval_expr(struct tp_ctx *ctx, struct hir_expr *e, bool in_region);
static void tp_meet_symbol(struct tp_ctx *ctx, const char *symbol, int t);
static void tp_apply_captures(struct tp_ctx *ctx, struct hir_expr *e, bool in_region);
static void tp_scan_stmt(struct tp_ctx *ctx, struct hir_stmt *stmt, bool in_region);
static void tp_scan_chain(struct tp_ctx *ctx, struct hir_block *head, bool in_region);

/*
 * Infers primitive local types for one HIR function.
 */
bool
hir_opt_typed_func(
	struct hir_block *func_block)
{
	struct tp_ctx ctx;
	struct hir_local *local;
	uint32_t k;
	int pass;

	assert(func_block != NULL);
	assert(func_block->type == HIR_BLOCK_FUNC);

	ctx.func = func_block;

	/*
	 * Seed: TOP everywhere, except checked primitive locals and parameters.
	 * Ordinary local annotations are enforced for every definition by LIR.
	 * Parameters -- annotated ones
	 * start at their annotation tag (CHECKTYPE-backed), the rest
	 * at UNKNOWN.  Parameters occupy local slots 0..param_count-1.
	 */
	local = func_block->val.func.local;

	/* Seed every local with its initial lattice value. */
	while (local != NULL) {
		local->proven_type = TP_TOP;
		if (!local->is_parameter &&
		    tp_is_primitive(local->declared_type)) {
			local->proven_type = local->declared_type;
		} else if (local->index < (int)func_block->val.func.param_count) {
			int seed = TP_UNKNOWN;

			/* Find the parameter annotation for this local. */
			for (k = 0; k < func_block->val.func.param_count; k++) {
				if (strcmp(
					    func_block->val.func.param_name[k],
					    local->symbol) != 0)
					continue;

				if (func_block->val.func.param_type[k] == NOCT_VALUE_INT)
					seed = TP_INT;
				else if (func_block->val.func.param_type[k] == NOCT_VALUE_LONG)
					seed = TP_LONG;
				else if (func_block->val.func.param_type[k] == NOCT_VALUE_FLOAT)
					seed = TP_FLOAT;
				else if (func_block->val.func.param_type[k] == NOCT_VALUE_DOUBLE)
					seed = TP_DOUBLE;
				break;
			}

			local->proven_type = seed;
		}
		local = local->next;
	}

	/*
	 * Optimistic fixpoint: values only move down the lattice, and
	 * its height is 2, so this settles quickly; the pass cap is a
	 * defensive backstop, and on overrun everything degrades to
	 * UNKNOWN (never the other direction).
	 */

	/* Iterate until the lattice reaches a fixed point. */
	for (pass = 0; pass < 64; pass++) {
		ctx.changed = false;
		if (func_block->val.func.inner != NULL)
			tp_scan_chain(&ctx, func_block->val.func.inner, false);
		if (!ctx.changed)
			break;
	}

	if (pass == 64) {
		local = func_block->val.func.local;

		/* Degrade every proof when the defensive cap is reached. */
		while (local != NULL) {
			local->proven_type = TP_UNKNOWN;
			local = local->next;
		}

		return true;
	}

	/*
	 * Finalize: TOP = never assigned = always the zero-init value
	 * (integer 0) -> proven INT.
	 */
	local = func_block->val.func.local;

	/* Convert never-assigned locals to their zero-initialized type. */
	while (local != NULL) {
		if (local->proven_type == TP_TOP)
			local->proven_type = TP_INT;
		local = local->next;
	}

	return true;
}

/* Meet-into: returns the combined value of cur and incoming t. */
static int
tp_combine(
	int cur,
	int t)
{
	if (t == TP_TOP)
		return cur;	/* No information yet: keep. */
	if (cur == TP_TOP)
		return t;
	if (cur == t)
		return cur;

	return TP_UNKNOWN;
}

static bool
tp_is_primitive(
	int t)
{
	return hir_opt_scalar_type_is_primitive(t);
}

/* Match the generic numeric dispatch in execution.c. */
static int
tp_promote_numeric(
	int a,
	int b)
{
	return hir_opt_scalar_type_promote(a, b, HIR_OPT_SCALAR_ALL);
}

static struct hir_local *
tp_find_local(
	struct tp_ctx *ctx,
	const char *symbol)
{
	struct hir_local *local;

	local = ctx->func->val.func.local;

	/* Find the local with the requested symbol. */
	while (local != NULL) {
		if (strcmp(local->symbol, symbol) == 0)
			return local;
		local = local->next;
	}

	return NULL;
}

/* Evaluate an expression's proven tag under the current lattice. */
static int
tp_eval_expr(
	struct tp_ctx *ctx,
	struct hir_expr *e,
	bool in_region)
{
	int a, b;

	/* Evaluate the lattice value for this expression shape. */
	switch (e->type) {
	case HIR_EXPR_TERM:

		/* Classify a literal or resolve a symbol proof. */
		switch (e->val.term.term->type) {
		case HIR_TERM_INT:
			return TP_INT;
		case HIR_TERM_LONG:
			return TP_LONG;
		case HIR_TERM_FLOAT:
			return TP_FLOAT;
		case HIR_TERM_DOUBLE:
			return TP_DOUBLE;
		case HIR_TERM_SYMBOL:
		{
			struct hir_local *local;

			local = tp_find_local(ctx, e->val.term.term->val.symbol);
			if (local == NULL)
				return TP_UNKNOWN;	/* Global. */
			/*
			 * An enclosing ABCE typed-int region proves
			 * int dynamically (TYPEIS guards), whatever
			 * the flow-insensitive lattice says.
			 */
			if (in_region)
				return TP_INT;

			return local->proven_type;
		}
		default:
			return TP_UNKNOWN;
		}
	case HIR_EXPR_PAR:
		return tp_eval_expr(ctx, e->val.unary.expr, in_region);
	case HIR_EXPR_NEG:
		a = tp_eval_expr(ctx, e->val.unary.expr, in_region);
		if (!tp_is_primitive(a))
			return TP_UNKNOWN;

		return a;
	case HIR_EXPR_NOT:
		a = tp_eval_expr(ctx, e->val.unary.expr, in_region);
		if (a != TP_INT)
			return TP_UNKNOWN;

		return TP_INT;
	case HIR_EXPR_CAPTURE:
		return tp_eval_expr(ctx, e->val.capture.expr, in_region);
	case HIR_EXPR_SELECT:
		a = tp_eval_expr(ctx, e->val.select.if_true, in_region);
		b = tp_eval_expr(ctx, e->val.select.if_false, in_region);
		if (a == b)
			return a;

		return tp_combine(a, b);
	case HIR_EXPR_PLUS:
	case HIR_EXPR_MINUS:
	case HIR_EXPR_MUL:
	case HIR_EXPR_DIV:
		a = tp_eval_expr(ctx, e->val.binary.expr[0], in_region);
		b = tp_eval_expr(ctx, e->val.binary.expr[1], in_region);
		if (a == TP_TOP ||
		    b == TP_TOP)
			return TP_TOP;

		return tp_promote_numeric(a, b);
	case HIR_EXPR_MOD:
	case HIR_EXPR_AND:
	case HIR_EXPR_OR:
	case HIR_EXPR_XOR:
	case HIR_EXPR_SHL:
	case HIR_EXPR_SHR:
		a = tp_eval_expr(ctx, e->val.binary.expr[0], in_region);
		b = tp_eval_expr(ctx, e->val.binary.expr[1], in_region);
		if (a == TP_TOP ||
		    b == TP_TOP)
			return TP_TOP;
		if ((a == TP_INT ||
		     a == TP_LONG) &&
		    (b == TP_INT ||
		     b == TP_LONG)) {
			if (a == TP_LONG)
				return TP_LONG;
			if (b == TP_LONG)
				return TP_LONG;

			return TP_INT;
		}

		return TP_UNKNOWN;
	case HIR_EXPR_LT:
	case HIR_EXPR_LTE:
	case HIR_EXPR_GT:
	case HIR_EXPR_GTE:
	case HIR_EXPR_EQ:
	case HIR_EXPR_NEQ:
		a = tp_eval_expr(ctx, e->val.binary.expr[0], in_region);
		b = tp_eval_expr(ctx, e->val.binary.expr[1], in_region);
		if (a == TP_TOP ||
		    b == TP_TOP)
			return TP_TOP;
		if (a != TP_UNKNOWN &&
		    b != TP_UNKNOWN)
			return TP_INT;

		return TP_UNKNOWN;
	case HIR_EXPR_PLOAD8U:
	case HIR_EXPR_PLOAD8S:
	case HIR_EXPR_PLOAD16U:
	case HIR_EXPR_PLOAD16S:
	case HIR_EXPR_PLOAD32:
	case HIR_EXPR_PGATHER32:
	case HIR_EXPR_PLEN:
	case HIR_EXPR_PCHECK:
	case HIR_EXPR_TYPEIS:
		return TP_INT;
	case HIR_EXPR_PLOAD64:
		return TP_LONG;
	case HIR_EXPR_PLOADF32:
	case HIR_EXPR_VINDUCTF32:
		return TP_FLOAT;
	case HIR_EXPR_CALL:

		/* Classify the supported conversion intrinsics. */
		switch (hir_get_intrinsic_call(e)) {
		case HIR_INTRINSIC_INT_FROM:
			return TP_INT;
		case HIR_INTRINSIC_FLOAT_FROM:
			return TP_FLOAT;
		default:
			return TP_UNKNOWN;
		}
	default:
		/*
		 * PLOAD64/PBASE (long), NEG, NOT, LAND, LOR, DOT,
		 * SUBSCR, CALL, THISCALL, ARRAY, DICT, NEW, ...
		 */
		return TP_UNKNOWN;
	}
}

/* Meet an incoming tag into a local (by symbol; globals ignored). */
static void
tp_meet_symbol(
	struct tp_ctx *ctx,
	const char *symbol,
	int t)
{
	struct hir_local *local;
	int merged;

	local = tp_find_local(ctx, symbol);
	if (local == NULL)
		return;		/* Global or $return: no proof kept. */

	/*
	 * At -O1 and above, primitive annotations on ordinary locals are
	 * checked at every definition by LIR.  They are therefore contracts,
	 * not optimistic guesses.  Parameters are excluded: their annotation
	 * checks only the incoming value, and a later reassignment must still
	 * participate in the meet.
	 */
	if (!local->is_parameter &&
	    tp_is_primitive(local->declared_type)) {
		if (local->proven_type != local->declared_type) {
			local->proven_type = local->declared_type;
			ctx->changed = true;
		}
		return;
	}

	merged = tp_combine(local->proven_type, t);
	if (merged != local->proven_type) {
		local->proven_type = merged;
		ctx->changed = true;
	}
}

/*
 * Walk an expression tree applying CAPTURE edges (a CAPTURE assigns
 * the inner value to its home local wherever it appears).
 */
static void
tp_apply_captures(
	struct tp_ctx *ctx,
	struct hir_expr *e,
	bool in_region)
{
	uint32_t i;
	int type;

	if (e == NULL)
		return;

	/* Apply capture edges for this expression shape. */
	switch (e->type) {
	case HIR_EXPR_TERM:
		return;
	case HIR_EXPR_PMASKSTORE32:
		tp_apply_captures(ctx, e->val.mask_store.base, in_region);
		tp_apply_captures(ctx, e->val.mask_store.offset, in_region);
		tp_apply_captures(ctx, e->val.mask_store.mask, in_region);
		return;
	case HIR_EXPR_PGATHER32:
		tp_apply_captures(ctx, e->val.gather.index, in_region);
		return;
	case HIR_EXPR_VINDUCTF32:
		return;
	case HIR_EXPR_PAR:
	case HIR_EXPR_NEG:
	case HIR_EXPR_NOT:
	case HIR_EXPR_PLEN:
	case HIR_EXPR_PBASE:
		tp_apply_captures(ctx, e->val.unary.expr, in_region);
		return;
	case HIR_EXPR_CAPTURE:
		tp_apply_captures(ctx, e->val.capture.expr, in_region);
		type = tp_eval_expr(ctx, e->val.capture.expr, in_region);
		tp_meet_symbol(ctx, e->val.capture.symbol, type);
		return;
	case HIR_EXPR_SELECT:
		tp_apply_captures(ctx, e->val.select.cond, in_region);
		tp_apply_captures(ctx, e->val.select.if_true, in_region);
		tp_apply_captures(ctx, e->val.select.if_false, in_region);
		return;
	case HIR_EXPR_DOT:
		tp_apply_captures(ctx, e->val.dot.obj, in_region);
		return;
	case HIR_EXPR_CALL:
		tp_apply_captures(ctx, e->val.call.func, in_region);

		/* Apply captures from every ordinary call argument. */
		for (i = 0; i < e->val.call.arg_count; i++)
			tp_apply_captures(ctx, e->val.call.arg[i], in_region);
		return;
	case HIR_EXPR_THISCALL:
		tp_apply_captures(ctx, e->val.thiscall.obj, in_region);

		/* Apply captures from every method-call argument. */
		for (i = 0; i < e->val.thiscall.arg_count; i++)
			tp_apply_captures(ctx, e->val.thiscall.arg[i], in_region);
		return;
	case HIR_EXPR_ARRAY:

		/* Apply captures from every array element. */
		for (i = 0; i < e->val.array.elem_count; i++)
			tp_apply_captures(ctx, e->val.array.elem[i], in_region);
		return;
	case HIR_EXPR_DICT:

		/* Apply captures from every dictionary value. */
		for (i = 0; i < e->val.dict.kv_count; i++)
			tp_apply_captures(ctx, e->val.dict.value[i], in_region);
		return;
	case HIR_EXPR_NEW:
		tp_apply_captures(ctx, e->val.new_.init, in_region);
		return;
	default:
		/* Binary nodes (incl. SUBSCR and the PLOAD family). */
		tp_apply_captures(ctx, e->val.binary.expr[0], in_region);
		tp_apply_captures(ctx, e->val.binary.expr[1], in_region);
		return;
	}
}

static void
tp_scan_stmt(
	struct tp_ctx *ctx,
	struct hir_stmt *stmt,
	bool in_region)
{
	int type;

	tp_apply_captures(ctx, stmt->rhs, in_region);

	if (stmt->lhs != NULL) {
		tp_apply_captures(ctx, stmt->lhs, in_region);

		if (stmt->lhs->type == HIR_EXPR_TERM &&
		    stmt->lhs->val.term.term->type == HIR_TERM_SYMBOL) {
			type = tp_eval_expr(ctx, stmt->rhs, in_region);
			tp_meet_symbol(
				ctx,
				stmt->lhs->val.term.term->val.symbol,
				type);
		}
	}
}

static void
tp_scan_chain(
	struct tp_ctx *ctx,
	struct hir_block *head,
	bool in_region)
{
	struct hir_block *b;
	struct hir_block *c;
	struct hir_stmt *stmt;

	b = head;

	/* Scan every block in the sibling chain. */
	while (b != NULL) {

		/* Apply the transfer function for this block shape. */
		switch (b->type) {
		case HIR_BLOCK_BASIC:
			stmt = b->val.basic.stmt_list;

			/* Scan every statement in the basic block. */
			while (stmt != NULL) {
				tp_scan_stmt(ctx, stmt, in_region);
				stmt = stmt->next;
			}
			break;
		case HIR_BLOCK_IF:
			c = b;

			/* Scan every arm in the conditional chain. */
			while (c != NULL) {
				if (c->val.if_.cond != NULL)
					tp_apply_captures(ctx, c->val.if_.cond, in_region);

				if (c->val.if_.inner != NULL)
					tp_scan_chain(ctx, c->val.if_.inner, in_region);
				c = c->val.if_.chain_next;
			}
			break;
		case HIR_BLOCK_FOR:
		{
			bool region;
			int type;

			region = in_region || b->val.for_.typed_int_region;
			if (b->val.for_.is_ranged) {
				tp_apply_captures(ctx, b->val.for_.start, in_region);
				tp_apply_captures(ctx, b->val.for_.stop, in_region);
				/* counter := start (OP_INC keeps the tag). */
				if (b->val.for_.counter_symbol != NULL) {
					type = tp_eval_expr(
						ctx,
						b->val.for_.start,
						in_region);
					tp_meet_symbol(
						ctx,
						b->val.for_.counter_symbol,
						type);
				}
			} else {
				tp_apply_captures(ctx, b->val.for_.collection, in_region);

				if (b->val.for_.key_symbol != NULL) {
					tp_meet_symbol(
						ctx,
						b->val.for_.key_symbol,
						TP_UNKNOWN);
				}

				if (b->val.for_.value_symbol != NULL) {
					tp_meet_symbol(
						ctx,
						b->val.for_.value_symbol,
						TP_UNKNOWN);
				}
			}

			if (b->val.for_.inner != NULL)
				tp_scan_chain(ctx, b->val.for_.inner, region);
			break;
		}
		case HIR_BLOCK_WHILE:
			if (b->val.while_.cond != NULL)
				tp_apply_captures(ctx, b->val.while_.cond, in_region);

			if (b->val.while_.inner != NULL)
				tp_scan_chain(ctx, b->val.while_.inner, in_region);
			break;
		default:
			break;
		}

		if (b->stop)
			break;
		b = b->succ;
	}
}
