/* -*- coding: utf-8; tab-width: 8; indent-tabs-mode: t; -*- */

/*
 * Noct Programming Language
 * Copyright (c) 2025, 2026, Awe Morris
 */

/*
 * HIR Optimizer: CSE pass.
 *
 * Common subexpression elimination over the HIR.
 *
 *  - An assignment to a local gives the local a fresh value number,
 *    so stale facts about it become unreachable by key ("kill by
 *    re-keying"). No table scans, no GEN/KILL sets, no fixpoint.
 *  - Heap-dependent facts (DOT/SUBSCR/PLOAD*, global reads) embed a
 *    memory epoch in their key. Heap stores and calls bump the epoch.
 *  - Facts created inside a conditionally- or repeatedly-executed
 *    region are reverted at the region's exit (undo-logged "avail"
 *    flags), because their home slot may never have been written on
 *    the path that reaches the join (untaken branch, zero-iteration
 *    loop).
 *  - Kills are monotone: re-keying and epoch bumps are never undone.
 *    This is why break/continue/return need no special handling: a
 *    path that jumps ahead has at most *more* kills applied than the
 *    structural walk assumed, and over-killing is always sound for a
 *    must-analysis.
 *  - Loop-body entry bumps the epoch unconditionally and re-keys every
 *    local assigned in the body (the kill-summary approximation of the
 *    loop-head meet).
 *
 * The rewrite preserves evaluation order exactly: the first
 * occurrence is wrapped in place with HIR_EXPR_CAPTURE (evaluate,
 * store to a $cseN home local, yield), later available occurrences
 * are replaced by a read of the home local.  Nothing is hoisted, so
 * error ordering and effect ordering are unchanged.
 *
 * Two passes with an identical traversal: ANALYZE counts how often
 * each value number would be reused, then homes are assigned to
 * profitable entries.  REWRITE re-runs the same walk and performs the
 * wrapping/substitution.  Determinism between the passes relies on
 * resetting all volatile state (symbol VNs, epoch, avail flags)
 * while keeping the interned entry table.
 */

#include <noct/noct.h>
#include "hir.h"
#include "hir_opt.h"
#include "lir.h"	/* LIR_TMPVAR_MAX: frame budget for $cseN homes */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

/* Limits. */
#define CSE_MAX_VALUES		256	/* interned value numbers        */
#define CSE_MAX_CAPTURES	32	/* $cseN home locals (D-CSE9)    */
#define CSE_MAX_LOCALS		128	/* locals per function           */
#define CSE_MAX_UNDO		512	/* scoped avail insertions       */
#define CSE_MAX_SCOPES		64	/* scope nesting                 */

/*
 * LIR frame budget for the capture homes.  Adding $cseN locals grows
 * the frame lir_build() starts from, so a function near LIR_TMPVAR_MAX
 * could compile at level 0 but fail at level 2.
 * We therefore cap the number of homes by a conservative upper bound
 * on the function's peak temp usage: per statement, LIR never holds
 * more temps than the statement's expression node count (each held
 * temp is the target of one distinct child-node visit), plus a small
 * per-statement overhead, plus the temps every enclosing loop keeps
 * alive across its body (ranged-for holds start/stop/cmp, for-each
 * holds col/size/i/key/val/cmp).  Over-counting only shrinks the
 * budget (missed optimization), never breaks compilation.
 */
#define CSE_TMPVAR_MARGIN	8	/* per-statement lowering overhead    */
#define CSE_LOOP_TEMPS		6	/* temps a loop holds across its body */

/* Value number sentinel: not CSE-able (effectful or overflowed). */
#define CSE_NOVN		(-1)

/* Pseudo-ops for value-number keys (must not collide with HIR_EXPR_*). */
#define CSE_OP_FRESH		(-2)	/* unknown value of a local      */
#define CSE_OP_GLOBAL		(-3)	/* global variable read          */
#define CSE_OP_CONST		(-4)	/* constant term                 */

/* Pass modes. */
#define CSE_PASS_ANALYZE	0
#define CSE_PASS_REWRITE	1

/* An interned value. */
struct cse_value {
	/* Key. */
	int op;			/* HIR_EXPR_* or CSE_OP_*         */
	int vn0;		/* child VN or fresh serial       */
	int vn1;		/* child VN or -1                 */
	int epoch;		/* memory epoch, -1 for pure      */
	const char *aux;	/* field/symbol/string, or NULL   */
	int const_type;		/* HIR_TERM_* for CSE_OP_CONST    */
	unsigned char bits[8];	/* constant payload               */
	size_t bits_len;

	/* Pass state. */
	int is_candidate;	/* worth capturing?               */
	int hits;		/* ANALYZE: reuse count           */
	int home;		/* capture slot no, -1 = none     */
	int avail;		/* value available in home now?   */
};

/* Pass context. */
struct cse_ctx {
	struct hir_block *func;

	/* Value table (persists across the two passes). */
	struct cse_value values[CSE_MAX_VALUES];
	int value_count;

	/* Volatile state (reset between passes). */
	/* -1 is unassigned; 0 is the valid first value-number entry. */
	int sym_vn[CSE_MAX_LOCALS];
	int fresh_seq;
	int mem_epoch;
	int undo[CSE_MAX_UNDO];
	int undo_top;
	int scope_mark[CSE_MAX_SCOPES];
	int scope_top;

	/* Mode and results. */
	int mode;
	int capture_budget;	/* frame-aware cap on homes, <= CSE_MAX_CAPTURES */
	int capture_count;
	int stat_captures;
	int stat_substs;
	int oom;
};

/* Forward declarations. */
static bool cse_run(struct cse_ctx *ctx);
static void cse_reset_volatile(struct cse_ctx *ctx);
static int cse_assign_homes(struct cse_ctx *ctx);
static void cse_walk_chain(struct cse_ctx *ctx, struct hir_block *head);
static void cse_walk_if(struct cse_ctx *ctx, struct hir_block *b);
static void cse_walk_while(struct cse_ctx *ctx, struct hir_block *b);
static void cse_walk_for(struct cse_ctx *ctx, struct hir_block *b);
static void cse_stmt(struct cse_ctx *ctx, struct hir_stmt *stmt);
static int cse_expr(struct cse_ctx *ctx, struct hir_expr **slot, int *weight);
static int cse_term(struct cse_ctx *ctx, struct hir_term *term);
static int cse_intern(struct cse_ctx *ctx, const struct cse_value *key, int is_candidate);
static int cse_fresh(struct cse_ctx *ctx);
static int cse_local_index(struct cse_ctx *ctx, const char *symbol);
static int cse_count_expr(struct hir_expr *e);
static void cse_measure_chain(struct hir_block *head, int base, int *max_cost);
static void cse_kill_local(struct cse_ctx *ctx, const char *symbol);
static void cse_defs_chain(struct cse_ctx *ctx, struct hir_block *head);
static void cse_defs_stmt_list(struct cse_ctx *ctx, struct hir_stmt *stmt);
static void cse_scope_open(struct cse_ctx *ctx);
static void cse_scope_close(struct cse_ctx *ctx);
static int cse_set_avail(struct cse_ctx *ctx, int vn);
static void cse_visit_value(struct cse_ctx *ctx, int vn, struct hir_expr **slot);
static void cse_home_name(int home, char *buf, size_t size);
static struct hir_expr *cse_mk_symbol_expr(struct cse_ctx *ctx, const char *name);

/*
 * Eliminates common subexpressions in one function.
 *
 * Level gating is done by the driver hir_optimize_func() in hir.c.
 */
bool
hir_opt_cse_func(
	struct hir_block *func_block)
{
	static struct cse_ctx ctx;
	struct hir_local *local;
	int local_count;
	int max_cost;
	int budget;
	bool ok;

	assert(func_block != NULL);
	assert(func_block->type == HIR_BLOCK_FUNC);

	/* Count the locals (indices are dense: max index + 1). */
	local_count = 0;
	local = func_block->val.func.local;

	/* Find the highest local index in the function. */
	while (local != NULL) {
		if (local->index + 1 > local_count)
			local_count = local->index + 1;
		local = local->next;
	}

	/* Peak-temp upper bound for the frame budget (see above). */
	max_cost = 0;
	if (func_block->val.func.inner != NULL)
		cse_measure_chain(func_block->val.func.inner, 0, &max_cost);

	/*
	 * Home budget: never let locals + homes + peak temps exceed the
	 * LIR frame, and never let a home's index escape sym_vn[].
	 * budget <= 0 means the function is too tight: skip the pass
	 * (a no-op is always sound).
	 */
	budget = LIR_TMPVAR_MAX - CSE_TMPVAR_MARGIN - local_count - max_cost;
	if (budget > CSE_MAX_CAPTURES)
		budget = CSE_MAX_CAPTURES;
	if (budget > CSE_MAX_LOCALS - local_count)
		budget = CSE_MAX_LOCALS - local_count;
	if (budget <= 0)
		return true;

	memset(&ctx, 0, sizeof(ctx));
	ctx.func = func_block;
	ctx.capture_budget = budget;

	ok = cse_run(&ctx);

	if (getenv("NOCT_CSE_DEBUG") != NULL &&
	    ctx.capture_count > 0) {
		fprintf(
			stderr,
			"[cse] %s:%s: %d captures, %d substitutions\n",
			hir_get_file_name(),
			func_block->val.func.name,
			ctx.stat_captures,
			ctx.stat_substs);
	}

	return ok;
}

/* Run the ANALYZE and REWRITE passes. */
static bool
cse_run(
	struct cse_ctx *ctx)
{
	if (ctx->func->val.func.inner == NULL)
		return true;

	/* Pass 1: ANALYZE. */
	ctx->mode = CSE_PASS_ANALYZE;
	cse_reset_volatile(ctx);
	cse_walk_chain(ctx, ctx->func->val.func.inner);

	/* Assign home slots to profitable entries. */
	(void)cse_assign_homes(ctx);
	if (ctx->oom) {
		hir_out_of_memory();
		return false;
	}
	if (ctx->capture_count == 0)
		return true;

	/* Pass 2: REWRITE (identical traversal). */
	ctx->mode = CSE_PASS_REWRITE;
	cse_reset_volatile(ctx);
	cse_walk_chain(ctx, ctx->func->val.func.inner);
	if (ctx->oom) {
		hir_out_of_memory();
		return false;
	}

	return true;
}

/* Reset the per-pass volatile state; keep the value table and hits. */
static void
cse_reset_volatile(
	struct cse_ctx *ctx)
{
	int i;

	/* Clear every local value-number binding. */
	for (i = 0; i < CSE_MAX_LOCALS; i++)
		ctx->sym_vn[i] = -1;

	/* Clear every value's availability flag. */
	for (i = 0; i < ctx->value_count; i++)
		ctx->values[i].avail = 0;

	ctx->fresh_seq = 0;
	ctx->mem_epoch = 0;
	ctx->undo_top = 0;
	ctx->scope_top = 0;
}

/*
 * Assign home slots ($cse0..) to candidate entries that ANALYZE found
 * reused at least once, in first-occurrence order, within the budget.
 * Registers the home locals on the function.  Returns the number of
 * homes assigned.
 */
static int
cse_assign_homes(
	struct cse_ctx *ctx)
{
	char name[16];
	int i;

	ctx->capture_count = 0;

	/* Assign homes to profitable values in first-occurrence order. */
	for (i = 0; i < ctx->value_count; i++) {
		struct cse_value *v;

		v = &ctx->values[i];
		if (!v->is_candidate ||
		    v->hits <= 0) {
			continue;
		}
		if (ctx->capture_count >= ctx->capture_budget)
			break;
		v->home = ctx->capture_count++;
		cse_home_name(v->home, name, sizeof(name));
		if (!hir_add_local(ctx->func, name)) {
			ctx->oom = 1;
			return 0;
		}
	}

	return ctx->capture_count;
}

/* Format a home local name. */
static void
cse_home_name(
	int home,
	char *buf,
	size_t size)
{
	snprintf(buf, size, "$cse%d", home);
}

/*
 * Scopes: an undo log of 0->1 "avail" transitions.  Closing a scope
 * reverts the flags set inside it.  Re-keying kills are NOT undone.
 */

static void
cse_scope_open(
	struct cse_ctx *ctx)
{
	if (ctx->scope_top < CSE_MAX_SCOPES)
		ctx->scope_mark[ctx->scope_top] = ctx->undo_top;
	ctx->scope_top++;
}

static void
cse_scope_close(
	struct cse_ctx *ctx)
{
	int mark;

	assert(ctx->scope_top > 0);

	ctx->scope_top--;
	if (ctx->scope_top >= CSE_MAX_SCOPES) {
		/*
		 * Untracked scope (nesting deeper than we record): revert
		 * to the deepest tracked mark.  Marks are monotone in
		 * nesting order (undo_top never sinks below an open
		 * scope's mark), so this never pops entries belonging to
		 * a still-open outer scope; it merely over-reverts the
		 * deepest tracked scope's facts, which is sound (facts
		 * lost, never wrongly kept).  Popping to 0 here would be
		 * WRONG: it would strand outer marks above undo_top, and
		 * facts created afterwards inside those still-open scopes
		 * could then escape their region.
		 */
		mark = ctx->scope_mark[CSE_MAX_SCOPES - 1];
	} else {
		mark = ctx->scope_mark[ctx->scope_top];
	}

	/* Undo every availability fact created in the closing scope. */
	while (ctx->undo_top > mark) {
		ctx->undo_top--;
		ctx->values[ctx->undo[ctx->undo_top]].avail = 0;
	}
}

/* Mark a value available, undo-logged.  Drops the fact if the log is full. */
static int
cse_set_avail(
	struct cse_ctx *ctx,
	int vn)
{
	if (ctx->undo_top >= CSE_MAX_UNDO)
		return 0;

	ctx->undo[ctx->undo_top++] = vn;
	ctx->values[vn].avail = 1;

	return 1;
}

/*
 * Intern a value key.  Returns its VN, or CSE_NOVN when the table is
 * full.  An existing entry's candidacy is upgraded (never downgraded)
 * because syntactically different subtrees can share a VN via local
 * copy propagation.
 */
static int
cse_intern(
	struct cse_ctx *ctx,
	const struct cse_value *key,
	int is_candidate)
{
	int i;

	/* Find an existing value with the same key. */
	for (i = 0; i < ctx->value_count; i++) {
		struct cse_value *v;

		v = &ctx->values[i];
		if (v->op != key->op ||
		    v->vn0 != key->vn0 ||
		    v->vn1 != key->vn1 ||
		    v->epoch != key->epoch) {
			continue;
		}
		if (v->const_type != key->const_type)
			continue;
		if (v->bits_len != key->bits_len)
			continue;
		if (key->bits_len != 0) {
			if (memcmp(v->bits, key->bits, key->bits_len) != 0)
				continue;
		}
		if ((v->aux == NULL) != (key->aux == NULL))
			continue;
		if (key->aux != NULL) {
			if (strcmp(v->aux, key->aux) != 0)
				continue;
		}
		if (is_candidate)
			v->is_candidate = 1;

		return i;
	}

	if (ctx->value_count >= CSE_MAX_VALUES)
		return CSE_NOVN;

	i = ctx->value_count++;
	ctx->values[i] = *key;
	ctx->values[i].is_candidate = is_candidate;
	ctx->values[i].hits = 0;
	ctx->values[i].home = -1;
	ctx->values[i].avail = 0;

	return i;
}

/* Intern a fresh (unknown) value for a local. */
static int
cse_fresh(
	struct cse_ctx *ctx)
{
	struct cse_value key;

	memset(&key, 0, sizeof(key));
	key.op = CSE_OP_FRESH;
	key.vn0 = ctx->fresh_seq++;
	key.vn1 = -1;
	key.epoch = -1;

	return cse_intern(ctx, &key, 0);
}

/* Find a local's index by name; -1 if not a local (i.e. a global). */
static int
cse_local_index(
	struct cse_ctx *ctx,
	const char *symbol)
{
	struct hir_local *local;

	local = ctx->func->val.func.local;

	/* Find the local with the requested symbol. */
	while (local != NULL) {
		if (strcmp(local->symbol, symbol) == 0)
			return local->index;
		local = local->next;
	}

	return -1;
}

/* Re-key a local (assignment kill). */
static void
cse_kill_local(
	struct cse_ctx *ctx,
	const char *symbol)
{
	int index;

	index = cse_local_index(ctx, symbol);
	if (index >= 0 &&
	    index < CSE_MAX_LOCALS) {
		ctx->sym_vn[index] = cse_fresh(ctx);
	}
}

/*
 * Frame-budget measurement (see the CSE_TMPVAR_MARGIN comment): an
 * upper bound on the LIR temps a statement's lowering can hold, per
 * statement, charged with the temps every enclosing loop keeps alive
 * across its body.
 */

/* Count the nodes of an expression subtree. */
static int
cse_count_expr(
	struct hir_expr *e)
{
	int n;
	uint32_t i;
	size_t j;

	if (e == NULL)
		return 0;

	n = 1;

	/* Count nodes according to the expression shape. */
	switch (e->type) {
	case HIR_EXPR_TERM:
		break;
	case HIR_EXPR_PAR:
	case HIR_EXPR_NEG:
	case HIR_EXPR_NOT:
	case HIR_EXPR_PBASE:
	case HIR_EXPR_PLEN:
		n += cse_count_expr(e->val.unary.expr);
		break;
	case HIR_EXPR_CAPTURE:
		n += cse_count_expr(e->val.capture.expr);
		break;
	case HIR_EXPR_SELECT:
		n += cse_count_expr(e->val.select.cond);
		n += cse_count_expr(e->val.select.if_true);
		n += cse_count_expr(e->val.select.if_false);
		break;
	case HIR_EXPR_PMASKSTORE32:
		n += cse_count_expr(e->val.mask_store.base);
		n += cse_count_expr(e->val.mask_store.offset);
		n += cse_count_expr(e->val.mask_store.mask);
		break;
	case HIR_EXPR_PGATHER32:
		n += cse_count_expr(e->val.gather.base);
		n += cse_count_expr(e->val.gather.length);
		n += cse_count_expr(e->val.gather.index);
		n += cse_count_expr(e->val.gather.packed);
		break;
	case HIR_EXPR_VINDUCTF32:
		n += cse_count_expr(e->val.binary.expr[0]);
		n += cse_count_expr(e->val.binary.expr[1]);
		break;
	case HIR_EXPR_DOT:
		n += cse_count_expr(e->val.dot.obj);
		break;
	case HIR_EXPR_CALL:
		n += cse_count_expr(e->val.call.func);

		/* Count every ordinary call argument. */
		for (i = 0; i < e->val.call.arg_count; i++)
			n += cse_count_expr(e->val.call.arg[i]);
		break;
	case HIR_EXPR_THISCALL:
		n += cse_count_expr(e->val.thiscall.obj);

		/* Count every method-call argument. */
		for (i = 0; i < e->val.thiscall.arg_count; i++)
			n += cse_count_expr(e->val.thiscall.arg[i]);
		break;
	case HIR_EXPR_ARRAY:

		/* Count every array element. */
		for (j = 0; j < e->val.array.elem_count; j++)
			n += cse_count_expr(e->val.array.elem[j]);
		break;
	case HIR_EXPR_DICT:

		/* Count every dictionary value. */
		for (j = 0; j < e->val.dict.kv_count; j++)
			n += cse_count_expr(e->val.dict.value[j]);
		break;
	case HIR_EXPR_NEW:
		n += cse_count_expr(e->val.new_.init);
		break;
	default:
		/* Binary shapes (operators, SUBSCR, PLOAD/PSTORE). */
		n += cse_count_expr(e->val.binary.expr[0]);
		n += cse_count_expr(e->val.binary.expr[1]);
		break;
	}

	return n;
}

/* Measure the peak statement cost of a region, at loop depth 'base'. */
static void
cse_measure_chain(
	struct hir_block *head,
	int base,
	int *max_cost)
{
	struct hir_block *b;
	struct hir_block *c;
	struct hir_stmt *stmt;
	int cost;

	b = head;

	/* Measure every block in the sibling chain. */
	while (b != NULL) {

		/* Measure the payload selected by the block shape. */
		switch (b->type) {
		case HIR_BLOCK_BASIC:
			stmt = b->val.basic.stmt_list;

			/* Measure every statement in the basic block. */
			while (stmt != NULL) {
				cost = base;
				cost += cse_count_expr(stmt->rhs);
				cost += cse_count_expr(stmt->lhs);
				if (cost > *max_cost)
					*max_cost = cost;
				stmt = stmt->next;
			}
			break;
		case HIR_BLOCK_IF:
			c = b;

			/* Measure every arm in the conditional chain. */
			while (c != NULL) {
				cost = base + cse_count_expr(c->val.if_.cond);
				if (cost > *max_cost)
					*max_cost = cost;
				if (c->val.if_.inner != NULL)
					cse_measure_chain(c->val.if_.inner, base, max_cost);
				c = c->val.if_.chain_next;
			}
			break;
		case HIR_BLOCK_WHILE:
			cost = base + cse_count_expr(b->val.while_.cond);
			if (cost > *max_cost)
				*max_cost = cost;
			if (b->val.while_.inner != NULL) {
				cse_measure_chain(
					b->val.while_.inner,
					base + CSE_LOOP_TEMPS,
					max_cost);
			}
			break;
		case HIR_BLOCK_FOR:
			cost = base;
			cost += cse_count_expr(b->val.for_.start);
			cost += cse_count_expr(b->val.for_.stop);
			cost += cse_count_expr(b->val.for_.collection);
			if (cost > *max_cost)
				*max_cost = cost;
			if (b->val.for_.inner != NULL) {
				cse_measure_chain(
					b->val.for_.inner,
					base + CSE_LOOP_TEMPS,
					max_cost);
			}
			break;
		default:
			break;
		}
		if (b->stop)
			break;
		b = b->succ;
	}
}

/*
 * At a candidate occurrence: count (ANALYZE) or rewrite (REWRITE).
 *
 * Soundness note: REWRITE's correctness does NOT depend on matching
 * ANALYZE's avail bookkeeping.  A substitution is justified purely by
 * REWRITE's own discipline (same VN + avail implies a capture of the
 * same value was executed earlier on every path, per the scope/kill
 * rules), and an extra capture is just a redundant store of the right
 * value.  ANALYZE only chooses WHICH value numbers get home slots, so
 * a divergence (e.g. an is_candidate flag upgraded late in ANALYZE
 * being set from the start in REWRITE) can at worst misplace captures
 * or waste a slot, never miscompile.
 */
static void
cse_visit_value(
	struct cse_ctx *ctx,
	int vn,
	struct hir_expr **slot)
{
	struct cse_value *v;
	char name[16];
	struct hir_expr *cap;

	v = &ctx->values[vn];
	if (!v->is_candidate)
		return;

	if (ctx->mode == CSE_PASS_ANALYZE) {
		if (v->avail)
			v->hits++;
		else
			cse_set_avail(ctx, vn);
		return;
	}

	/* REWRITE. */
	if (v->home < 0) {
		/* Keep the avail discipline identical to ANALYZE. */
		if (!v->avail)
			cse_set_avail(ctx, vn);
		return;
	}

	cse_home_name(v->home, name, sizeof(name));

	if (v->avail) {
		/* Reuse: replace the subtree with a home read. */
		struct hir_expr *sym = cse_mk_symbol_expr(ctx, name);

		if (sym == NULL)
			return;

		*slot = sym;
		ctx->stat_substs++;
	} else {
		/* First occurrence: wrap in place. */
		cap = hir_malloc(sizeof(struct hir_expr));
		if (cap == NULL) {
			ctx->oom = 1;
			return;
		}

		memset(cap, 0, sizeof(*cap));
		cap->type = HIR_EXPR_CAPTURE;
		cap->val.capture.expr = *slot;
		cap->val.capture.symbol = hir_strdup(name);
		if (cap->val.capture.symbol == NULL) {
			ctx->oom = 1;
			return;
		}

		*slot = cap;

		if (!cse_set_avail(ctx, vn)) {
			/*
			 * The undo log is full, so the fact cannot be
			 * tracked and no reuse will ever see it; the
			 * capture is a harmless extra MOVE.
			 */
		}

		ctx->stat_captures++;
	}
}

/* Build a symbol term expression. */
static struct hir_expr *
cse_mk_symbol_expr(
	struct cse_ctx *ctx,
	const char *name)
{
	struct hir_expr *e;
	struct hir_term *t;

	e = hir_malloc(sizeof(struct hir_expr));
	if (e == NULL) {
		ctx->oom = 1;
		return NULL;
	}

	t = hir_malloc(sizeof(struct hir_term));
	if (t == NULL) {
		ctx->oom = 1;
		return NULL;
	}

	memset(t, 0, sizeof(*t));
	t->type = HIR_TERM_SYMBOL;
	t->val.symbol = hir_strdup(name);
	if (t->val.symbol == NULL) {
		ctx->oom = 1;
		return NULL;
	}

	memset(e, 0, sizeof(*e));
	e->type = HIR_EXPR_TERM;
	e->val.term.term = t;

	return e;
}

/* Value-number a term. */
static int
cse_term(
	struct cse_ctx *ctx,
	struct hir_term *term)
{
	struct cse_value key;
	int index;

	memset(&key, 0, sizeof(key));
	key.vn0 = -1;
	key.vn1 = -1;
	key.epoch = -1;

	/* Value-number the payload selected by the term type. */
	switch (term->type) {
	case HIR_TERM_SYMBOL:
		if (strcmp(term->val.symbol, "$return") == 0)
			return CSE_NOVN;
		index = cse_local_index(ctx, term->val.symbol);
		if (index >= 0) {
			if (index >= CSE_MAX_LOCALS)
				return CSE_NOVN;
			if (ctx->sym_vn[index] < 0)
				ctx->sym_vn[index] = cse_fresh(ctx);
			return ctx->sym_vn[index];
		}
		/* Global read: epoch-keyed (killed by stores/calls). */
		key.op = CSE_OP_GLOBAL;
		key.aux = term->val.symbol;
		key.epoch = ctx->mem_epoch;
		return cse_intern(ctx, &key, 0);
	case HIR_TERM_INT:
		key.op = CSE_OP_CONST;
		key.const_type = HIR_TERM_INT;
		key.bits_len = sizeof(term->val.i);
		memcpy(key.bits, &term->val.i, key.bits_len);
		return cse_intern(ctx, &key, 0);
	case HIR_TERM_LONG:
		key.op = CSE_OP_CONST;
		key.const_type = HIR_TERM_LONG;
		key.bits_len = sizeof(term->val.l);
		memcpy(key.bits, &term->val.l, key.bits_len);
		return cse_intern(ctx, &key, 0);
	case HIR_TERM_FLOAT:
		/* Exact bit compare: -0.0/NaN stay distinct from 0.0. */
		key.op = CSE_OP_CONST;
		key.const_type = HIR_TERM_FLOAT;
		key.bits_len = sizeof(term->val.f);
		memcpy(key.bits, &term->val.f, key.bits_len);
		return cse_intern(ctx, &key, 0);
	case HIR_TERM_DOUBLE:
		key.op = CSE_OP_CONST;
		key.const_type = HIR_TERM_DOUBLE;
		key.bits_len = sizeof(term->val.lf);
		memcpy(key.bits, &term->val.lf, key.bits_len);
		return cse_intern(ctx, &key, 0);
	case HIR_TERM_STRING:
		key.op = CSE_OP_CONST;
		key.const_type = HIR_TERM_STRING;
		key.aux = term->val.s;
		return cse_intern(ctx, &key, 0);
	case HIR_TERM_EMPTY_ARRAY:
	case HIR_TERM_EMPTY_DICT:
		/* Allocations: never a CSE value (identity matters). */
		return CSE_NOVN;
	default:
		assert(0);
		return CSE_NOVN;
	}
}

/*
 * Value-number an expression subtree, post-order, mirroring the LIR
 * evaluation order (children left to right).  Returns the node's VN
 * or CSE_NOVN, and adds the subtree's operator count to *weight.
 * In REWRITE mode this also performs the capture/substitution on
 * *slot.
 */
static int
cse_expr(
	struct cse_ctx *ctx,
	struct hir_expr **slot,
	int *weight)
{
	struct hir_expr *e;
	struct cse_value key;
	int w;
	int vn0, vn1, vn;
	int is_candidate;
	uint32_t i;
	size_t j;

	assert(*slot != NULL);

	e = *slot;
	memset(&key, 0, sizeof(key));
	key.vn0 = -1;
	key.vn1 = -1;
	key.epoch = -1;

	/* Value-number the payload selected by the expression shape. */
	switch (e->type) {
	case HIR_EXPR_TERM:
		return cse_term(ctx, e->val.term.term);

	case HIR_EXPR_PAR:
		/* Transparent. */
		return cse_expr(ctx, &e->val.unary.expr, weight);

	case HIR_EXPR_CAPTURE:
		/* Created by this pass; transparent, never revisited. */
		return cse_expr(ctx, &e->val.capture.expr, weight);

	case HIR_EXPR_SELECT:
		/*
		 * SIMD SELECT evaluates all three inputs.  Traverse children so
		 * their bookkeeping remains valid, but do not value-number the
		 * ternary node itself.
		 */
		w = 0;
		(void)cse_expr(ctx, &e->val.select.cond, &w);
		(void)cse_expr(ctx, &e->val.select.if_true, &w);
		(void)cse_expr(ctx, &e->val.select.if_false, &w);
		*weight += w + 1;
		return CSE_NOVN;

	case HIR_EXPR_PMASKSTORE32:
		w = 0;
		(void)cse_expr(ctx, &e->val.mask_store.base, &w);
		(void)cse_expr(ctx, &e->val.mask_store.offset, &w);
		(void)cse_expr(ctx, &e->val.mask_store.mask, &w);
		*weight += w + 1;
		return CSE_NOVN;

	case HIR_EXPR_PGATHER32:
		w = 0;
		(void)cse_expr(ctx, &e->val.gather.index, &w);
		*weight += w + 1;
		return CSE_NOVN;

	case HIR_EXPR_VINDUCTF32:
		*weight += 1;
		return CSE_NOVN;

	case HIR_EXPR_NEG:
	case HIR_EXPR_NOT:
	case HIR_EXPR_PBASE:
	case HIR_EXPR_PLEN:
		w = 0;
		vn0 = cse_expr(ctx, &e->val.unary.expr, &w);
		*weight += w + 1;
		if (vn0 == CSE_NOVN)
			return CSE_NOVN;
		key.op = e->type;
		key.vn0 = vn0;
		is_candidate = (w + 1 >= 2);
		vn = cse_intern(ctx, &key, is_candidate);
		if (vn != CSE_NOVN)
			cse_visit_value(ctx, vn, slot);
		return vn;

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
	case HIR_EXPR_XOR:
	case HIR_EXPR_SHL:
	case HIR_EXPR_SHR:
	case HIR_EXPR_PCHECK:
	case HIR_EXPR_TYPEIS:
		/* Pure binary operators (may throw; order preserved). */
		w = 0;
		vn0 = cse_expr(ctx, &e->val.binary.expr[0], &w);
		vn1 = cse_expr(ctx, &e->val.binary.expr[1], &w);
		*weight += w + 1;
		if (vn0 == CSE_NOVN ||
		    vn1 == CSE_NOVN) {
			return CSE_NOVN;
		}
		key.op = e->type;
		key.vn0 = vn0;
		key.vn1 = vn1;
		is_candidate = (w + 1 >= 2);
		vn = cse_intern(ctx, &key, is_candidate);
		if (vn != CSE_NOVN)
			cse_visit_value(ctx, vn, slot);
		return vn;

	case HIR_EXPR_LAND:
	case HIR_EXPR_LOR:
		/* Short-circuit: the RHS is conditionally evaluated. */
		w = 0;
		vn0 = cse_expr(ctx, &e->val.binary.expr[0], &w);
		cse_scope_open(ctx);
		vn1 = cse_expr(ctx, &e->val.binary.expr[1], &w);
		cse_scope_close(ctx);
		*weight += w + 1;
		if (vn0 == CSE_NOVN ||
		    vn1 == CSE_NOVN) {
			return CSE_NOVN;
		}
		key.op = e->type;
		key.vn0 = vn0;
		key.vn1 = vn1;
		is_candidate = (w + 1 >= 2);
		vn = cse_intern(ctx, &key, is_candidate);
		if (vn != CSE_NOVN)
			cse_visit_value(ctx, vn, slot);
		return vn;

	case HIR_EXPR_DOT:
		/* Heap read: epoch-keyed. */
		w = 0;
		vn0 = cse_expr(ctx, &e->val.dot.obj, &w);
		*weight += w + 1;
		if (vn0 == CSE_NOVN)
			return CSE_NOVN;
		key.op = e->type;
		key.vn0 = vn0;
		key.aux = e->val.dot.symbol;
		key.epoch = ctx->mem_epoch;
		vn = cse_intern(ctx, &key, 1);
		if (vn != CSE_NOVN)
			cse_visit_value(ctx, vn, slot);
		return vn;

	case HIR_EXPR_SUBSCR:
	case HIR_EXPR_PLOAD8U:
	case HIR_EXPR_PLOAD8S:
	case HIR_EXPR_PLOAD16U:
	case HIR_EXPR_PLOAD16S:
	case HIR_EXPR_PLOAD32:
	case HIR_EXPR_PLOAD64:
	case HIR_EXPR_PLOADF32:
		/* Heap reads: epoch-keyed. */
		w = 0;
		vn0 = cse_expr(ctx, &e->val.binary.expr[0], &w);
		vn1 = cse_expr(ctx, &e->val.binary.expr[1], &w);
		*weight += w + 1;
		if (vn0 == CSE_NOVN ||
		    vn1 == CSE_NOVN) {
			return CSE_NOVN;
		}
		key.op = e->type;
		key.vn0 = vn0;
		key.vn1 = vn1;
		key.epoch = ctx->mem_epoch;
		vn = cse_intern(ctx, &key, 1);
		if (vn != CSE_NOVN)
			cse_visit_value(ctx, vn, slot);
		return vn;

	case HIR_EXPR_CALL:
		w = 0;
		(void)cse_expr(ctx, &e->val.call.func, &w);

		/* Visit every ordinary call argument. */
		for (i = 0; i < e->val.call.arg_count; i++)
			(void)cse_expr(ctx, &e->val.call.arg[i], &w);
		ctx->mem_epoch++;
		*weight += w + 1;
		return CSE_NOVN;

	case HIR_EXPR_THISCALL:
		w = 0;
		(void)cse_expr(ctx, &e->val.thiscall.obj, &w);

		/* Visit every method-call argument. */
		for (i = 0; i < e->val.thiscall.arg_count; i++)
			(void)cse_expr(ctx, &e->val.thiscall.arg[i], &w);
		ctx->mem_epoch++;
		*weight += w + 1;
		return CSE_NOVN;

	case HIR_EXPR_NEW:
		w = 0;
		if (e->val.new_.init != NULL)
			(void)cse_expr(ctx, &e->val.new_.init, &w);
		ctx->mem_epoch++;
		*weight += w + 1;
		return CSE_NOVN;

	case HIR_EXPR_ARRAY:
		/* Pure construction: no epoch bump, but a fresh object. */
		w = 0;

		/* Visit every array element. */
		for (j = 0; j < e->val.array.elem_count; j++)
			(void)cse_expr(ctx, &e->val.array.elem[j], &w);
		*weight += w + 1;
		return CSE_NOVN;

	case HIR_EXPR_DICT:
		w = 0;

		/* Visit every dictionary value. */
		for (j = 0; j < e->val.dict.kv_count; j++)
			(void)cse_expr(ctx, &e->val.dict.value[j], &w);
		*weight += w + 1;
		return CSE_NOVN;

	default:
		/* PSTORE* only appear as statement LHS. */
		assert(0);
		return CSE_NOVN;
	}
}

/* Walk one statement, mirroring lir_visit_stmt's evaluation order. */
static void
cse_stmt(
	struct cse_ctx *ctx,
	struct hir_stmt *stmt)
{
	int w;
	int rhs_vn;
	struct hir_expr *lhs;

	assert(stmt->rhs != NULL);

	/* RHS first. */
	w = 0;
	rhs_vn = cse_expr(ctx, &stmt->rhs, &w);

	lhs = stmt->lhs;
	if (lhs == NULL)
		return;

	if (lhs->type == HIR_EXPR_TERM) {
		const char *symbol;

		assert(lhs->val.term.term->type == HIR_TERM_SYMBOL);
		symbol = lhs->val.term.term->val.symbol;
		if (strcmp(symbol, "$return") == 0) {
			/* Return-value slot: no heap effect, no local. */
			return;
		}
		if (cse_local_index(ctx, symbol) >= 0) {
			/*
			 * Local assignment: re-key, then copy the RHS
			 * value number so the local aliases it.
			 */
			cse_kill_local(ctx, symbol);
			if (rhs_vn != CSE_NOVN) {
				int index = cse_local_index(ctx, symbol);

				if (index >= 0 &&
				    index < CSE_MAX_LOCALS) {
					ctx->sym_vn[index] = rhs_vn;
				}
			}
		} else {
			/* Global store: kills global-read facts. */
			ctx->mem_epoch++;
		}
		return;
	}

	if (lhs->type == HIR_EXPR_DOT) {
		w = 0;
		(void)cse_expr(ctx, &lhs->val.dot.obj, &w);
		ctx->mem_epoch++;
		return;
	}

	if (lhs->type == HIR_EXPR_SUBSCR ||
	    lhs->type == HIR_EXPR_PSTORE8 ||
	    lhs->type == HIR_EXPR_PSTORE16 ||
	    lhs->type == HIR_EXPR_PSTORE32 ||
	    lhs->type == HIR_EXPR_PSTORE64 ||
	    lhs->type == HIR_EXPR_PSTOREF32) {
		w = 0;
		(void)cse_expr(ctx, &lhs->val.binary.expr[0], &w);
		(void)cse_expr(ctx, &lhs->val.binary.expr[1], &w);
		ctx->mem_epoch++;
		return;
	}

	assert(0);	/* Unknown LHS shape. */
}

/*
 * Loop-head kill summary: re-key every local assigned anywhere in the
 * loop body subtree (including nested loop counters and for-each
 * key/value variables).  The epoch bump is done by the caller.
 */

static void
cse_defs_stmt_list(
	struct cse_ctx *ctx,
	struct hir_stmt *stmt)
{

	/* Record every local assigned by the statement list. */
	while (stmt != NULL) {
		if (stmt->lhs != NULL &&
		    stmt->lhs->type == HIR_EXPR_TERM &&
		    stmt->lhs->val.term.term->type == HIR_TERM_SYMBOL) {
			const char *symbol = stmt->lhs->val.term.term->val.symbol;

			if (strcmp(symbol, "$return") != 0)
				cse_kill_local(ctx, symbol);
		}
		stmt = stmt->next;
	}
}

static void
cse_defs_chain(
	struct cse_ctx *ctx,
	struct hir_block *head)
{
	struct hir_block *b;
	struct hir_block *c;

	b = head;

	/* Record definitions in every block of the sibling chain. */
	while (b != NULL) {

		/* Recurse according to the current block shape. */
		switch (b->type) {
		case HIR_BLOCK_BASIC:
			cse_defs_stmt_list(ctx, b->val.basic.stmt_list);
			break;
		case HIR_BLOCK_IF:
			c = b;

			/* Record definitions in every conditional arm. */
			while (c != NULL) {
				if (c->val.if_.inner != NULL)
					cse_defs_chain(ctx, c->val.if_.inner);
				c = c->val.if_.chain_next;
			}
			break;
		case HIR_BLOCK_WHILE:
			if (b->val.while_.inner != NULL)
				cse_defs_chain(ctx, b->val.while_.inner);
			break;
		case HIR_BLOCK_FOR:
			if (b->val.for_.is_ranged) {
				if (b->val.for_.counter_symbol != NULL)
					cse_kill_local(ctx, b->val.for_.counter_symbol);
			} else {
				if (b->val.for_.key_symbol != NULL)
					cse_kill_local(ctx, b->val.for_.key_symbol);
				if (b->val.for_.value_symbol != NULL)
					cse_kill_local(ctx, b->val.for_.value_symbol);
			}
			if (b->val.for_.inner != NULL)
				cse_defs_chain(ctx, b->val.for_.inner);
			break;
		default:
			break;
		}
		if (b->stop)
			break;
		b = b->succ;
	}
}

/* Walk an if/elif/else chain. */
static void
cse_walk_if(
	struct cse_ctx *ctx,
	struct hir_block *b)
{
	struct hir_block *elem;
	int w;

	/*
	 * The head's condition is evaluated on every path through the
	 * chain, so its facts belong to the enclosing scope.  Later
	 * conditions are evaluated on strictly fewer paths: their
	 * facts may serve later arms but must die at the join.
	 */
	if (b->val.if_.cond != NULL) {
		w = 0;
		(void)cse_expr(ctx, &b->val.if_.cond, &w);
	}

	cse_scope_open(ctx);
	elem = b;

	/* Walk every arm in the conditional chain. */
	while (elem != NULL) {
		if (elem != b &&
		    elem->val.if_.cond != NULL) {
			w = 0;
			(void)cse_expr(ctx, &elem->val.if_.cond, &w);
		}
		if (elem->val.if_.inner != NULL) {
			cse_scope_open(ctx);
			cse_walk_chain(ctx, elem->val.if_.inner);
			cse_scope_close(ctx);

			/*
			 * Alias invalidation, per arm: a local assigned in
			 * this arm has an uncertain value everywhere the arm
			 * did not execute — which includes the NEXT elif
			 * condition and every later arm, not just the join.
			 * The walk of an arm may have left a local aliased
			 * (sym_vn copy) to a value computed inside it; that
			 * is a positive fact and must be re-keyed before any
			 * mutually-exclusive code is walked.  Avail flags
			 * were already reverted by the arm scope; re-keying
			 * kills are monotone and never reverted, so
			 * over-killing here is sound.  Doing this after every
			 * arm also covers the join itself.
			 */
			cse_defs_chain(ctx, elem->val.if_.inner);
		}
		elem = elem->val.if_.chain_next;
	}
	cse_scope_close(ctx);
}

/* Walk a while loop. */
static void
cse_walk_while(
	struct cse_ctx *ctx,
	struct hir_block *b)
{
	int w;

	cse_scope_open(ctx);

	/* Loop-head kill summary (D-CSE6: unconditional epoch bump). */
	ctx->mem_epoch++;
	if (b->val.while_.inner != NULL)
		cse_defs_chain(ctx, b->val.while_.inner);

	if (b->val.while_.cond != NULL) {
		w = 0;
		(void)cse_expr(ctx, &b->val.while_.cond, &w);
	}
	if (b->val.while_.inner != NULL)
		cse_walk_chain(ctx, b->val.while_.inner);

	cse_scope_close(ctx);

	/*
	 * Exit invalidation: the body's symbol->VN aliases describe one
	 * symbolic iteration and are wrong after zero or many
	 * iterations, so re-key everything the body assigns (same
	 * rationale as the if-join invalidation).
	 */
	if (b->val.while_.inner != NULL)
		cse_defs_chain(ctx, b->val.while_.inner);
}

/* Walk a for loop (ranged or for-each). */
static void
cse_walk_for(
	struct cse_ctx *ctx,
	struct hir_block *b)
{
	int w;

	/*
	 * A vectorized strip loop (design 06) is off limits: its body
	 * must stay within the vector-lowerable grammar, and a CAPTURE
	 * node would break the vector LIR visitor.  Treat it like any
	 * loop on the OUTSIDE (epoch bump + kill/re-key via the def
	 * scan below runs in the caller's conservative path), but do
	 * not analyze or rewrite anything inside.
	 */
	if (b->val.for_.is_vector) {
		ctx->mem_epoch++;
		if (b->val.for_.counter_symbol != NULL)
			cse_kill_local(ctx, b->val.for_.counter_symbol);
		cse_defs_chain(ctx, b->val.for_.inner);
		return;
	}

	/* Evaluated once, before the loop: enclosing scope. */
	if (b->val.for_.is_ranged) {
		if (b->val.for_.start != NULL) {
			w = 0;
			(void)cse_expr(ctx, &b->val.for_.start, &w);
		}
		if (b->val.for_.stop != NULL) {
			w = 0;
			(void)cse_expr(ctx, &b->val.for_.stop, &w);
		}
	} else {
		if (b->val.for_.collection != NULL) {
			w = 0;
			(void)cse_expr(ctx, &b->val.for_.collection, &w);
		}
	}

	cse_scope_open(ctx);

	/* Loop-head kill summary (D-CSE6: unconditional epoch bump). */
	ctx->mem_epoch++;
	if (b->val.for_.is_ranged) {
		if (b->val.for_.counter_symbol != NULL)
			cse_kill_local(ctx, b->val.for_.counter_symbol);
	} else {
		if (b->val.for_.key_symbol != NULL)
			cse_kill_local(ctx, b->val.for_.key_symbol);
		if (b->val.for_.value_symbol != NULL)
			cse_kill_local(ctx, b->val.for_.value_symbol);
	}
	if (b->val.for_.inner != NULL)
		cse_defs_chain(ctx, b->val.for_.inner);

	if (b->val.for_.inner != NULL)
		cse_walk_chain(ctx, b->val.for_.inner);

	cse_scope_close(ctx);

	/*
	 * Exit invalidation: see cse_walk_while.  The counter/key/value
	 * variables are also re-keyed (they advanced with the loop).
	 */
	if (b->val.for_.is_ranged) {
		if (b->val.for_.counter_symbol != NULL)
			cse_kill_local(ctx, b->val.for_.counter_symbol);
	} else {
		if (b->val.for_.key_symbol != NULL)
			cse_kill_local(ctx, b->val.for_.key_symbol);
		if (b->val.for_.value_symbol != NULL)
			cse_kill_local(ctx, b->val.for_.value_symbol);
	}
	if (b->val.for_.inner != NULL)
		cse_defs_chain(ctx, b->val.for_.inner);
}

/* Walk a sibling chain of blocks (region body). */
static void
cse_walk_chain(
	struct cse_ctx *ctx,
	struct hir_block *head)
{
	struct hir_block *b;
	struct hir_stmt *stmt;

	b = head;

	/* Walk every block in the sibling chain. */
	while (b != NULL) {
		if (ctx->oom)
			return;

		/* Visit the payload selected by the current block shape. */
		switch (b->type) {
		case HIR_BLOCK_BASIC:
			stmt = b->val.basic.stmt_list;

			/* Visit every statement in the basic block. */
			while (stmt != NULL) {
				cse_stmt(ctx, stmt);
				stmt = stmt->next;
			}
			break;
		case HIR_BLOCK_IF:
			cse_walk_if(ctx, b);
			break;
		case HIR_BLOCK_WHILE:
			cse_walk_while(ctx, b);
			break;
		case HIR_BLOCK_FOR:
			cse_walk_for(ctx, b);
			break;
		case HIR_BLOCK_FUNC:
			if (b->val.func.inner != NULL)
				cse_walk_chain(ctx, b->val.func.inner);
			break;
		case HIR_BLOCK_END:
			break;
		default:
			break;
		}

		if (b->stop)
			break;
		b = b->succ;
	}
}
