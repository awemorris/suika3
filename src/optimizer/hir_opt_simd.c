/* -*- coding: utf-8; tab-width: 8; indent-tabs-mode: t; -*- */

/*
 * Noct Programming Language
 * Copyright (c) 2025, 2026, Awe Morris
 */

/*
 * HIR Optimizer: 128-bit SIMD auto-vectorization
 *
 * Runs right after ABCE.  For every ABCE-versioned fast loop whose
 * body fits the vector grammar (single basic block, homogeneous
 * int32 or float32 packed accesses; integer +,-,*,&,|,^/constant
 * shifts or float +,-,*,/, no counter-as-value, no loop-carried
 * temps, alias discipline), the fast loop is split into a 4-lane
 * strip loop plus a scalar remainder:
 *
 *   B1:  $baseK = PBASE(pK); ...            (existing)
 *        $simdN_mid = $hi - (($hi - $lo) & 3);
 *        $simdN_sbS = $baseK (+|-) 4L * u;  (per non-trivial offset)
 *        $simdN_vg  = (0 <= $lo) && ($lo < $simdN_mid) && <disjoint>;
 *   GV:  if ($simdN_vg)  { VFOR (i in $lo..$mid)  vector body
 *                          RFOR (i in $mid..$hi)  scalar clone }
 *   GS:  if (!$simdN_vg) { SFOR (i in $lo..$hi)   scalar clone }
 *
 * The strip loop touches exactly a prefix of the scalar iteration
 * sequence: (mid - lo) is divisible by 4 by construction, and the
 * entry condition 0 <= lo confines the strip range to [0, 2^31) where
 * the 32-bit counter cannot wrap, so lanes i..i+3 are exactly the
 * elements the scalar iterations would touch.
 *
 * Packed payloads CAN partially overlap through the preallocated-
 * buffer C API, so cross-packed stores take a runtime disjointness
 * guard instead of relying on object identity.
 *
 * The vector body is the same HIR expression tree, lowered by
 * lir_visit_vfor_block() through a parallel visitor into the vector
 * opcodex, site indexes are rewritten to the bare counter with the
 * affine offset folded into a per-site adjusted base, so the strip
 * body needs no scalar arithmetic at all.
 */

#include "hir_opt.h"
#include "hir_opt_parallel.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <limits.h>

/* Program-visible vregs (must match the LIR planner's budget). */
#define SIMD_VREG_MAX		16

#define SIMD_MAX_BASES		8	/* = ABCE_MAX_PACKED           */
#define SIMD_MAX_OFFS		4	/* distinct offsets per base    */
#define SIMD_MAX_CONSTS		8	/* distinct int consts in body  */
#define SIMD_MAX_LOCALS		16	/* INV + TEMP locals in body   */
#define SIMD_MAX_LOOPS		16

/* Minimum estimated scalar work before entering a vector strip. */
#define SIMD_MIN_WORK		32

#define SIMD_INLINE_MAX		32

/* Reject-reason breadcrumb for NOCT_SIMD_DEBUG. */
#define SIMD_REJECT(why) do { simd_reject_reason = (why); return false; } while (0)
#define SIMD_REJECT_I(why) do { simd_reject_reason = (why); return -1; } while (0)

#define SIMD_APPEND(stmt_expr)						\
	do {							\
		struct hir_stmt *ns_;				\
								\
		ns_ = (stmt_expr);					\
		if (ns_ == NULL)					\
			return false;					\
		tail->next = ns_;					\
		tail = ns_;						\
	} while (0)

/* Index shapes (mirrors the ABCE affine shapes). */
enum simd_shape {
	SIMD_SHAPE_I,
	SIMD_SHAPE_I_PLUS_U,
	SIMD_SHAPE_U_PLUS_I,
	SIMD_SHAPE_I_MINUS_U
};

struct simd_off {
	int shape;
	bool u_is_const;
	int u_const;
	const char *u_name;
	char sb_name[64];	/* adjusted-base local ("" = use base) */
};

struct simd_base {
	const char *base_sym;	/* $abceN_baseK local        */
	const char *packed_sym;	/* the packed local (for PLEN) */
	bool restricted;	/* rpacked* function parameter */
	bool has_read;
	bool has_store;
	int element_kind;
	int off_count;
	struct simd_off off[SIMD_MAX_OFFS];
};

struct simd_ctx {
	struct hir_block *func;
	struct hir_block *loop;		/* the abce_fast FOR (becomes VFOR) */
	struct hir_block *b1;		/* the PBASE hoist block            */
	struct hir_block *scalar_body;	/* source body before if-conversion */
	const char *counter;
	const char *lo_name;		/* $abceN_lo   */
	const char *hi_name;		/* $abceN_hi   */

	struct simd_base bases[SIMD_MAX_BASES];
	int base_count;

	/* Planner sets (must stay in sync with lir.c's planner). */
	uint32_t consts[SIMD_MAX_CONSTS];	/* int value or float bits */
	uint8_t const_type[SIMD_MAX_CONSTS];
	int const_count;
	const char *inv[SIMD_MAX_LOCALS];
	int inv_count;
	const char *temp[SIMD_MAX_LOCALS];
	int temp_count;
	int max_depth;
	int body_cost;
	int min_trip;
	bool kind_set;
	bool is_float;
	bool has_checked_gather;
	bool has_fp_induction;
	struct hir_memory_catalog parallel_catalog;
	struct hir_loop_summary *parallel_summary;
	struct hir_doall_result parallel_memory;

	/* New local names. */
	char mid_name[32];
	char vg_name[32];
};

static int simd_loop_seq;

static const char *simd_reject_reason;

static struct hir_term *simd_mk_term_int(int v);
static struct hir_term *simd_mk_term_long(int64_t v);
static struct hir_term *simd_mk_term_float(float v);
static struct hir_term *simd_mk_term_sym(const char *sym);
static struct hir_expr *simd_mk_expr_term(struct hir_term *t);
static struct hir_expr *simd_mk_sym(const char *sym);
static struct hir_expr *simd_mk_int(int v);
static struct hir_expr *simd_mk_long(int64_t v);
static struct hir_expr *simd_mk_binary(int type, struct hir_expr *l, struct hir_expr *r);
static struct hir_expr *simd_mk_unary(int type, struct hir_expr *x);
static struct hir_expr *simd_mk_select(struct hir_expr *cond, struct hir_expr *if_true, struct hir_expr *if_false);
static struct hir_expr *simd_mk_mask_store(struct hir_expr *base, struct hir_expr *offset, struct hir_expr *mask);
static struct hir_stmt *simd_mk_assign(int line, struct hir_expr *lhs, struct hir_expr *rhs);
static struct hir_block *simd_mk_block(int type, int line, struct hir_block *parent);
static struct hir_expr *simd_clone_expr(struct hir_expr *e);
static struct hir_stmt *simd_clone_stmt_list(struct hir_stmt *head);
static bool simd_body_expr_pure(struct hir_expr *e);
static bool simd_append_stmt_clone(struct hir_stmt **head, struct hir_stmt **tail, struct hir_stmt *src);
static struct hir_block *simd_if_convert_body(struct hir_block *loop);
static struct hir_block *simd_clone_scalar_chain(struct hir_block *src, struct hir_block *parent);
static bool simd_inline_pure(struct hir_expr *e);
static struct hir_expr *simd_expand_expr(struct hir_expr *e, const char **names, struct hir_expr **defs, int count);
static bool simd_live_expr(struct hir_expr *e, const char *sym);
static bool simd_live_chain(struct hir_block *head, const char *sym);
static struct hir_stmt *simd_inline_temps(struct hir_block *loop);
static bool simd_is_local(struct simd_ctx *ctx, const char *name);
static int simd_local_type(struct simd_ctx *ctx, const char *name);
static int simd_resolve_scalar_symbol(void *data, const char *symbol);
static int simd_expr_type(struct simd_ctx *ctx, struct hir_expr *e);
static struct simd_base *simd_find_base(struct simd_ctx *ctx, const char *sym);
static bool simd_note_const(struct simd_ctx *ctx, uint32_t v, int type);
static bool simd_in_list(const char **list, int count, const char *name);
static bool simd_parse_index(struct simd_ctx *ctx, struct hir_expr *f, struct simd_off *out);
static bool simd_off_equal(const struct simd_off *a, const struct simd_off *b);
static bool simd_note_site(struct simd_ctx *ctx, struct hir_expr *site, bool is_store);
static bool simd_note_gather(struct simd_ctx *ctx, struct hir_expr *site);
static bool simd_expr_reads(struct hir_expr *e, const char *sym);
static struct hir_expr *simd_strip_par(struct hir_expr *e);
static bool simd_normalize_fp_inductions(struct simd_ctx *ctx, struct hir_block *body);
static int simd_check_expr(struct simd_ctx *ctx, struct hir_expr *e);
static bool simd_check_body(struct simd_ctx *ctx, struct hir_block *body);
static bool simd_find_environment(struct simd_ctx *ctx);
static bool simd_is_fp_induction_symbol(struct simd_ctx *ctx, const char *symbol);
static bool simd_build_parallel_facts(struct simd_ctx *ctx, bool *safe, const char **reason);
static bool simd_alias_required(const struct simd_ctx *ctx, int first, int second);
static void simd_free_parallel_facts(struct simd_ctx *ctx);
static int simd_count_locals(struct simd_ctx *ctx);
static struct hir_expr *simd_mk_u_expr(const struct simd_off *off);
static bool simd_rewrite_expr(struct simd_ctx *ctx, struct hir_expr *e);
static bool simd_vectorize(struct simd_ctx *ctx);
static void simd_collect_loops(struct hir_block *head, struct hir_block **loops, int *count);

/*
 * Vectorizes eligible Packed loops in a function.
 */
bool
hir_opt_simd_func(
	struct hir_block *func_block,
	bool simd_info)
{
	struct hir_block *loops[SIMD_MAX_LOOPS];
	int loop_count;
	int i;

	assert(func_block != NULL);
	assert(func_block->type == HIR_BLOCK_FUNC);

	if (func_block->val.func.inner == NULL)
		return true;

	loop_count = 0;
	simd_collect_loops(func_block->val.func.inner, loops, &loop_count);

	/* Analyze and vectorize each collected loop. */
	for (i = 0; i < loop_count; i++) {
		struct simd_ctx *ctx;
		struct hir_block *source_body;
		struct hir_block *normalized_body;
		struct hir_stmt *original_body;
		struct hir_stmt *inlined_body;
		bool eligible;
		bool parallel_safe;
		const char *parallel_reason;

		ctx = hir_malloc(sizeof(struct simd_ctx));
		if (ctx == NULL) {
			hir_out_of_memory();
			return false;
		}

		memset(ctx, 0, sizeof(*ctx));
		ctx->func = func_block;
		ctx->loop = loops[i];
		ctx->counter = loops[i]->val.for_.counter_symbol;

		source_body = loops[i]->val.for_.inner;
		normalized_body = source_body;

		if (source_body != NULL &&
		    source_body->type == HIR_BLOCK_BASIC &&
		    source_body->stop) {
			normalized_body = simd_mk_block(
				HIR_BLOCK_BASIC,
				source_body->line,
				loops[i]);
			if (normalized_body == NULL)
				return false;

			normalized_body->stop = true;
			normalized_body->is_return_edge =
				source_body->is_return_edge;
			normalized_body->is_break_edge =
				source_body->is_break_edge;
			normalized_body->is_continue_edge =
				source_body->is_continue_edge;
			normalized_body->succ = source_body->succ;
			normalized_body->val.basic.stmt_list =
				simd_clone_stmt_list(
					source_body->val.basic.stmt_list);
			if (source_body->val.basic.stmt_list != NULL &&
			    normalized_body->val.basic.stmt_list == NULL)
				return false;

			loops[i]->val.for_.inner = normalized_body;
		} else if (source_body == NULL ||
			   source_body->type != HIR_BLOCK_BASIC ||
			   !source_body->stop) {
			normalized_body = simd_if_convert_body(loops[i]);
			if (normalized_body == NULL)
				continue;

			loops[i]->val.for_.inner = normalized_body;
		}

		ctx->scalar_body = source_body;

		if (!simd_normalize_fp_inductions(ctx, normalized_body)) {
			loops[i]->val.for_.inner = source_body;
			continue;
		}

		original_body = normalized_body->val.basic.stmt_list;
		inlined_body = simd_inline_temps(loops[i]);
		if (inlined_body != NULL)
			loops[i]->val.for_.inner->val.basic.stmt_list = inlined_body;

		simd_reject_reason = "?";
		eligible = simd_check_body(ctx, loops[i]->val.for_.inner);
		if (eligible)
			eligible = simd_find_environment(ctx);

		if (!eligible) {
			normalized_body->val.basic.stmt_list = original_body;
			loops[i]->val.for_.inner = source_body;
			continue;
		}

		if (!simd_build_parallel_facts(
			    ctx,
			    &parallel_safe,
			    &parallel_reason)) {
			normalized_body->val.basic.stmt_list = original_body;
			loops[i]->val.for_.inner = source_body;
			return false;
		}

		if (!parallel_safe) {
			simd_free_parallel_facts(ctx);
			normalized_body->val.basic.stmt_list = original_body;
			loops[i]->val.for_.inner = source_body;
			continue;
		}

		/* Frame budget: locals we would add. */
		{
			int adds;
			int k;
			int j;

			adds = 2;	/* mid + vg */

			/* Count adjusted-base locals for every vector base. */
			for (k = 0; k < ctx->base_count; k++) {

				/* Count non-trivial offsets for this base. */
				for (j = 0; j < ctx->bases[k].off_count; j++) {
					if (ctx->bases[k].off[j].shape !=
					    SIMD_SHAPE_I)
						adds++;
				}
			}

			if (simd_count_locals(ctx) + adds > 112) {
				simd_free_parallel_facts(ctx);
				normalized_body->val.basic.stmt_list = original_body;
				loops[i]->val.for_.inner = source_body;
				continue;
			}
		}

		if (!simd_vectorize(ctx)) {
			simd_free_parallel_facts(ctx);
			return false;
		}

		if (simd_info) {
			fprintf(
				stderr,
				"SIMD: %s:%d: vectorized (%s)\n",
				hir_get_file_name(),
				loops[i]->line,
				ctx->is_float ? "f32x4" : "i32x4");
		}

		simd_free_parallel_facts(ctx);
	}

	return true;
}

/*
 * Small constructors (arena-allocated; failures return NULL and the
 * caller propagates OOM via hir_out_of_memory()).
 */

static struct hir_term *
simd_mk_term_int(
	int v)
{
	struct hir_term *t;

	t = hir_malloc(sizeof(struct hir_term));
	if (t == NULL) {
		hir_out_of_memory();
		return NULL;
	}

	memset(t, 0, sizeof(*t));
	t->type = HIR_TERM_INT;
	t->val.i = v;

	return t;
}

static struct hir_term *
simd_mk_term_long(
	int64_t v)
{
	struct hir_term *t;

	t = hir_malloc(sizeof(struct hir_term));
	if (t == NULL) {
		hir_out_of_memory();
		return NULL;
	}

	memset(t, 0, sizeof(*t));
	t->type = HIR_TERM_LONG;
	t->val.l = v;

	return t;
}

static struct hir_term *
simd_mk_term_float(
	float v)
{
	struct hir_term *t;

	t = hir_malloc(sizeof(struct hir_term));
	if (t == NULL) {
		hir_out_of_memory();
		return NULL;
	}

	memset(t, 0, sizeof(*t));
	t->type = HIR_TERM_FLOAT;
	t->val.f = v;

	return t;
}

static struct hir_term *
simd_mk_term_sym(
	const char *sym)
{
	struct hir_term *t;

	t = hir_malloc(sizeof(struct hir_term));
	if (t == NULL) {
		hir_out_of_memory();
		return NULL;
	}

	memset(t, 0, sizeof(*t));
	t->type = HIR_TERM_SYMBOL;
	t->val.symbol = hir_strdup(sym);
	if (t->val.symbol == NULL) {
		hir_out_of_memory();
		return NULL;
	}

	return t;
}

static struct hir_expr *
simd_mk_expr_term(
	struct hir_term *t)
{
	struct hir_expr *e;

	if (t == NULL)
		return NULL;

	e = hir_malloc(sizeof(struct hir_expr));
	if (e == NULL) {
		hir_out_of_memory();
		return NULL;
	}

	memset(e, 0, sizeof(*e));
	e->type = HIR_EXPR_TERM;
	e->val.term.term = t;

	return e;
}

static struct hir_expr *
simd_mk_sym(
	const char *sym)
{
	struct hir_term *term;

	term = simd_mk_term_sym(sym);
	if (term == NULL)
		return NULL;

	return simd_mk_expr_term(term);
}

static struct hir_expr *
simd_mk_int(
	int v)
{
	struct hir_term *term;

	term = simd_mk_term_int(v);
	if (term == NULL)
		return NULL;

	return simd_mk_expr_term(term);
}

static struct hir_expr *
simd_mk_long(
	int64_t v)
{
	struct hir_term *term;

	term = simd_mk_term_long(v);
	if (term == NULL)
		return NULL;

	return simd_mk_expr_term(term);
}

static struct hir_expr *
simd_mk_binary(
	int type,
	struct hir_expr *l,
	struct hir_expr *r)
{
	struct hir_expr *e;

	if (l == NULL)
		return NULL;
	if (r == NULL)
		return NULL;

	e = hir_malloc(sizeof(struct hir_expr));
	if (e == NULL) {
		hir_out_of_memory();
		return NULL;
	}

	memset(e, 0, sizeof(*e));
	e->type = type;
	e->val.binary.expr[0] = l;
	e->val.binary.expr[1] = r;

	return e;
}

static struct hir_expr *
simd_mk_unary(
	int type,
	struct hir_expr *x)
{
	struct hir_expr *e;

	if (x == NULL)
		return NULL;

	e = hir_malloc(sizeof(struct hir_expr));
	if (e == NULL) {
		hir_out_of_memory();
		return NULL;
	}

	memset(e, 0, sizeof(*e));
	e->type = type;
	e->val.unary.expr = x;

	return e;
}

static struct hir_expr *
simd_mk_select(
	struct hir_expr *cond,
	struct hir_expr *if_true,
	struct hir_expr *if_false)
{
	struct hir_expr *e;

	if (cond == NULL)
		return NULL;
	if (if_true == NULL)
		return NULL;
	if (if_false == NULL)
		return NULL;

	e = hir_malloc(sizeof(*e));
	if (e == NULL) {
		hir_out_of_memory();
		return NULL;
	}

	memset(e, 0, sizeof(*e));
	e->type = HIR_EXPR_SELECT;
	e->val.select.cond = cond;
	e->val.select.if_true = if_true;
	e->val.select.if_false = if_false;

	return e;
}

static struct hir_expr *
simd_mk_mask_store(
	struct hir_expr *base,
	struct hir_expr *offset,
	struct hir_expr *mask)
{
	struct hir_expr *e;

	if (base == NULL)
		return NULL;
	if (offset == NULL)
		return NULL;
	if (mask == NULL)
		return NULL;

	e = hir_malloc(sizeof(*e));
	if (e == NULL) {
		hir_out_of_memory();
		return NULL;
	}

	memset(e, 0, sizeof(*e));
	e->type = HIR_EXPR_PMASKSTORE32;
	e->val.mask_store.base = base;
	e->val.mask_store.offset = offset;
	e->val.mask_store.mask = mask;

	return e;
}

static struct hir_stmt *
simd_mk_assign(
	int line,
	struct hir_expr *lhs,
	struct hir_expr *rhs)
{
	struct hir_stmt *s;

	if (lhs == NULL)
		return NULL;
	if (rhs == NULL)
		return NULL;

	s = hir_malloc(sizeof(struct hir_stmt));
	if (s == NULL) {
		hir_out_of_memory();
		return NULL;
	}

	memset(s, 0, sizeof(*s));
	s->line = line;
	s->lhs = lhs;
	s->rhs = rhs;

	return s;
}

static struct hir_block *
simd_mk_block(
	int type,
	int line,
	struct hir_block *parent)
{
	struct hir_block *b;

	b = hir_malloc(sizeof(struct hir_block));
	if (b == NULL) {
		hir_out_of_memory();
		return NULL;
	}

	memset(b, 0, sizeof(*b));
	b->type = type;
	b->line = line;
	b->parent = parent;
	b->id = hir_next_block_id();

	return b;
}

/*
 * Deep copy of an eligible body expression (the vector grammar only:
 * int/symbol terms, PAR, arithmetic binaries, PLOAD32/PSTORE32).
 */
static struct hir_expr *
simd_clone_expr(
	struct hir_expr *e)
{
	struct hir_expr *n;

	if (e == NULL)
		return NULL;

	n = hir_malloc(sizeof(struct hir_expr));
	if (n == NULL) {
		hir_out_of_memory();
		return NULL;
	}

	memset(n, 0, sizeof(*n));
	n->type = e->type;

	/* Clone the payload owned by this expression shape. */
	switch (e->type) {
	case HIR_EXPR_TERM:
	{
		struct hir_term *t;

		t = e->val.term.term;
		if (t->type == HIR_TERM_INT)
			n->val.term.term = simd_mk_term_int(t->val.i);
		else if (t->type == HIR_TERM_FLOAT)
			n->val.term.term = simd_mk_term_float(t->val.f);
		else if (t->type == HIR_TERM_SYMBOL)
			n->val.term.term = simd_mk_term_sym(t->val.symbol);
		else
			return NULL;	/* outside the grammar */

		if (n->val.term.term == NULL)
			return NULL;

		return n;
	}
	case HIR_EXPR_PAR:
	case HIR_EXPR_NEG:
		n->val.unary.expr = simd_clone_expr(e->val.unary.expr);
		if (n->val.unary.expr == NULL)
			return NULL;

		return n;
	case HIR_EXPR_CALL:
	{
		uint32_t i;

		n->val.call.func = simd_clone_expr(e->val.call.func);
		if (n->val.call.func == NULL)
			return NULL;

		n->val.call.arg_count = e->val.call.arg_count;

		/* Clone the call arguments in source order. */
		for (i = 0; i < e->val.call.arg_count; i++) {
			n->val.call.arg[i] = simd_clone_expr(e->val.call.arg[i]);
			if (n->val.call.arg[i] == NULL)
				return NULL;
		}

		return n;
	}
	case HIR_EXPR_DOT:
		n->val.dot.obj = simd_clone_expr(e->val.dot.obj);
		if (n->val.dot.obj == NULL)
			return NULL;

		n->val.dot.symbol = hir_strdup(e->val.dot.symbol);
		if (n->val.dot.symbol == NULL)
			return NULL;

		return n;
	case HIR_EXPR_SELECT:
		n->val.select.cond = simd_clone_expr(e->val.select.cond);
		if (n->val.select.cond == NULL)
			return NULL;

		n->val.select.if_true = simd_clone_expr(e->val.select.if_true);
		if (n->val.select.if_true == NULL)
			return NULL;

		n->val.select.if_false = simd_clone_expr(e->val.select.if_false);
		if (n->val.select.if_false == NULL)
			return NULL;

		return n;
	case HIR_EXPR_PMASKSTORE32:
		n->val.mask_store.base = simd_clone_expr(e->val.mask_store.base);
		if (n->val.mask_store.base == NULL)
			return NULL;

		n->val.mask_store.offset = simd_clone_expr(e->val.mask_store.offset);
		if (n->val.mask_store.offset == NULL)
			return NULL;

		n->val.mask_store.mask = simd_clone_expr(e->val.mask_store.mask);
		if (n->val.mask_store.mask == NULL)
			return NULL;

		return n;
	case HIR_EXPR_PGATHER32:
		n->val.gather.base = simd_clone_expr(e->val.gather.base);
		if (n->val.gather.base == NULL)
			return NULL;

		n->val.gather.length = simd_clone_expr(e->val.gather.length);
		if (n->val.gather.length == NULL)
			return NULL;

		n->val.gather.index = simd_clone_expr(e->val.gather.index);
		if (n->val.gather.index == NULL)
			return NULL;

		n->val.gather.packed = simd_clone_expr(e->val.gather.packed);
		if (n->val.gather.packed == NULL)
			return NULL;

		return n;
	default:
		/* Binary shapes (arith, shifts, PLOAD32/PSTORE32). */
		n->val.binary.expr[0] = simd_clone_expr(e->val.binary.expr[0]);
		if (n->val.binary.expr[0] == NULL)
			return NULL;

		n->val.binary.expr[1] = simd_clone_expr(e->val.binary.expr[1]);
		if (n->val.binary.expr[1] == NULL)
			return NULL;

		return n;
	}
}

static struct hir_stmt *
simd_clone_stmt_list(
	struct hir_stmt *head)
{
	struct hir_stmt *nh;
	struct hir_stmt *tail;
	struct hir_stmt *s;

	nh = NULL;
	tail = NULL;

	/* Clone each statement and preserve list order. */
	for (s = head; s != NULL; s = s->next) {
		struct hir_stmt *n;

		n = hir_malloc(sizeof(struct hir_stmt));
		if (n == NULL) {
			hir_out_of_memory();
			return NULL;
		}

		memset(n, 0, sizeof(*n));
		n->line = s->line;

		if (s->lhs != NULL) {
			n->lhs = simd_clone_expr(s->lhs);
			if (n->lhs == NULL)
				return NULL;
		}

		n->rhs = simd_clone_expr(s->rhs);
		if (n->rhs == NULL)
			return NULL;

		if (tail == NULL)
			nh = n;
		else
			tail->next = n;
		tail = n;
	}

	return nh;
}

static bool
simd_body_expr_pure(
	struct hir_expr *e)
{
	uint32_t i;

	if (e == NULL)
		return false;

	/* Classify the expression and recursively check its operands. */
	switch (e->type) {
	case HIR_EXPR_TERM:
		if (e->val.term.term->type == HIR_TERM_INT)
			return true;
		if (e->val.term.term->type == HIR_TERM_FLOAT)
			return true;
		if (e->val.term.term->type == HIR_TERM_SYMBOL)
			return true;

		return false;
	case HIR_EXPR_PAR:
	case HIR_EXPR_NEG:
		return simd_body_expr_pure(e->val.unary.expr);
	case HIR_EXPR_CALL:
		if (hir_get_intrinsic_call(e) == HIR_INTRINSIC_NONE)
			return false;
		if (e->val.call.arg_count != 1)
			return false;

		/* Check every intrinsic argument for purity. */
		for (i = 0; i < e->val.call.arg_count; i++) {
			if (!simd_body_expr_pure(e->val.call.arg[i]))
				return false;
		}

		return true;
	case HIR_EXPR_PLOAD32:
	case HIR_EXPR_PLOADF32:
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
	case HIR_EXPR_AND:
	case HIR_EXPR_OR:
	case HIR_EXPR_XOR:
	case HIR_EXPR_SHL:
	case HIR_EXPR_SHR:
		if (!simd_body_expr_pure(e->val.binary.expr[0]))
			return false;
		if (!simd_body_expr_pure(e->val.binary.expr[1]))
			return false;

		return true;
	default:
		return false;
	}
}

static bool
simd_append_stmt_clone(
	struct hir_stmt **head,
	struct hir_stmt **tail,
	struct hir_stmt *src)
{
	struct hir_stmt *n;

	n = simd_clone_stmt_list(src);
	if (n == NULL)
		return false;

	if (*tail == NULL)
		*head = n;
	else
		(*tail)->next = n;

	/* Find the tail of the appended clone. */
	while (n->next != NULL)
		n = n->next;

	*tail = n;

	return true;
}

/*
 * Convert the deliberately small draw-image CFG subset into one basic block.
 * Only an if without else whose body is one local assignment is accepted:
 *
 *     if (c) x = v;   ->   x = SELECT(c, v, x)
 *
 * The source CFG is retained and cloned into both scalar paths later.
 */
static struct hir_block *
simd_if_convert_body(
	struct hir_block *loop)
{
	struct hir_block *b;
	struct hir_block *out;
	struct hir_stmt *head;
	struct hir_stmt *tail;

	head = NULL;
	tail = NULL;

	/* Convert each block in the accepted straight-line CFG. */
	for (b = loop->val.for_.inner; b != NULL;) {
		if (b->is_return_edge ||
		    b->is_break_edge ||
		    b->is_continue_edge) {
			return NULL;
		}

		if (b->type == HIR_BLOCK_BASIC) {
			if (b->val.basic.stmt_list != NULL) {
				if (!simd_append_stmt_clone(
					    &head,
					    &tail,
					    b->val.basic.stmt_list)) {
					return NULL;
				}
			}
		} else if (b->type == HIR_BLOCK_IF) {
			struct hir_block *ib;
			struct hir_stmt *s;
			struct hir_stmt *n;
			const char *sym;
			struct hir_expr *lhs;
			struct hir_expr *if_false;
			struct hir_expr *rhs;

			ib = b->val.if_.inner;
			sym = NULL;

			if (b->val.if_.chain_next != NULL)
				return NULL;
			if (b->val.if_.chain_prev != NULL)
				return NULL;
			if (ib == NULL)
				return NULL;
			if (ib->type != HIR_BLOCK_BASIC)
				return NULL;
			if (ib->is_return_edge ||
			    ib->is_break_edge ||
			    ib->is_continue_edge) {
				return NULL;
			}
			if (!ib->stop)
				return NULL;
			if (ib->val.basic.stmt_list == NULL)
				return NULL;
			if (ib->val.basic.stmt_list->next != NULL)
				return NULL;
			if (!simd_body_expr_pure(b->val.if_.cond))
				return NULL;

			s = ib->val.basic.stmt_list;
			if (s->lhs == NULL)
				return NULL;
			if (!simd_body_expr_pure(s->rhs))
				return NULL;

			if (s->lhs->type == HIR_EXPR_TERM &&
			    s->lhs->val.term.term->type == HIR_TERM_SYMBOL) {
				sym = s->lhs->val.term.term->val.symbol;
				lhs = simd_mk_sym(sym);
				if (lhs == NULL)
					return NULL;

				if_false = simd_mk_sym(sym);
				if (if_false == NULL)
					return NULL;
			} else if (s->lhs->type == HIR_EXPR_PSTORE32 ||
				   s->lhs->type == HIR_EXPR_PSTOREF32) {
				if (s->lhs->type != HIR_EXPR_PSTORE32)
					return NULL;
				lhs = simd_mk_mask_store(
					simd_clone_expr(s->lhs->val.binary.expr[0]),
					simd_clone_expr(s->lhs->val.binary.expr[1]),
					simd_clone_expr(b->val.if_.cond));
				if (lhs == NULL)
					return NULL;

				if_false = NULL;
			} else {
				return NULL;
			}
			if (s->lhs->type == HIR_EXPR_PSTORE32) {
				rhs = simd_clone_expr(s->rhs);
				if (rhs == NULL)
					return NULL;

				n = simd_mk_assign(s->line, lhs, rhs);
			} else {
				rhs = simd_mk_select(
					simd_clone_expr(b->val.if_.cond),
					simd_clone_expr(s->rhs),
					if_false);
				if (rhs == NULL)
					return NULL;

				n = simd_mk_assign(s->line, lhs, rhs);
			}
			if (n == NULL)
				return NULL;

			if (tail == NULL)
				head = n;
			else
				tail->next = n;
			tail = n;
		} else {
			return NULL;
		}
		if (b->stop)
			break;

		b = b->succ;
	}

	if (b == NULL)
		return NULL;
	if (head == NULL)
		return NULL;

	out = simd_mk_block(HIR_BLOCK_BASIC, loop->line, loop);
	if (out == NULL)
		return NULL;

	out->val.basic.stmt_list = head;
	out->stop = true;
	out->succ = out;

	return out;
}

/* Clone the original BASIC/IF body for a scalar remainder/fallback loop. */
static struct hir_block *
simd_clone_scalar_chain(
	struct hir_block *src,
	struct hir_block *parent)
{
	struct hir_block *next;
	struct hir_block *n;

	next = NULL;

	if (src == NULL)
		return NULL;

	if (!src->stop) {
		next = simd_clone_scalar_chain(src->succ, parent);
		if (next == NULL)
			return NULL;
	}

	n = simd_mk_block(src->type, src->line, parent);
	if (n == NULL)
		return NULL;

	n->stop = src->stop;
	n->is_return_edge = src->is_return_edge;
	n->is_break_edge = src->is_break_edge;
	n->is_continue_edge = src->is_continue_edge;

	/* Clone the payload owned by this block shape. */
	switch (src->type) {
	case HIR_BLOCK_BASIC:
		n->val.basic.stmt_list =
			simd_clone_stmt_list(src->val.basic.stmt_list);
		if (src->val.basic.stmt_list != NULL &&
		    n->val.basic.stmt_list == NULL)
			return NULL;
		break;
	case HIR_BLOCK_IF:
		if (src->val.if_.chain_next != NULL ||
		    src->val.if_.chain_prev != NULL)
			return NULL;
		n->val.if_.cond = simd_clone_expr(src->val.if_.cond);
		if (n->val.if_.cond == NULL)
			return NULL;
		n->val.if_.inner =
			simd_clone_scalar_chain(src->val.if_.inner, n);
		if (n->val.if_.inner == NULL)
			return NULL;
		/* The accepted inner chain is a single stopped BASIC. */
		n->val.if_.inner->succ = next != NULL ? next :
						 n->val.if_.inner;
		break;
	default:
		return NULL;
	}

	n->succ = next != NULL ? next : n;

	return n;
}

static bool
simd_inline_pure(
	struct hir_expr *e)
{
	uint32_t i;

	/* Classify the expression and recursively check its operands. */
	switch (e->type) {
	case HIR_EXPR_TERM:
		if (e->val.term.term->type == HIR_TERM_INT)
			return true;
		if (e->val.term.term->type == HIR_TERM_FLOAT)
			return true;
		if (e->val.term.term->type == HIR_TERM_SYMBOL)
			return true;

		return false;
	case HIR_EXPR_PAR:
	case HIR_EXPR_NEG:
		return simd_inline_pure(e->val.unary.expr);
	case HIR_EXPR_SELECT:
		if (!simd_inline_pure(e->val.select.cond))
			return false;
		if (!simd_inline_pure(e->val.select.if_true))
			return false;
		if (!simd_inline_pure(e->val.select.if_false))
			return false;

		return true;
	case HIR_EXPR_CALL:
		if (hir_get_intrinsic_call(e) == HIR_INTRINSIC_NONE)
			return false;
		if (e->val.call.arg_count != 1)
			return false;

		/* Check every intrinsic argument for purity. */
		for (i = 0; i < e->val.call.arg_count; i++) {
			if (!simd_inline_pure(e->val.call.arg[i]))
				return false;
		}
		return true;
	case HIR_EXPR_PLOAD32:
	case HIR_EXPR_PLOADF32:
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
	case HIR_EXPR_AND:
	case HIR_EXPR_OR:
	case HIR_EXPR_XOR:
	case HIR_EXPR_SHL:
	case HIR_EXPR_SHR:
		if (!simd_inline_pure(e->val.binary.expr[0]))
			return false;
		if (!simd_inline_pure(e->val.binary.expr[1]))
			return false;

		return true;
	case HIR_EXPR_PGATHER32:
		return simd_inline_pure(e->val.gather.index);
	default:
		return false;
	}
}

static struct hir_expr *
simd_expand_expr(
	struct hir_expr *e,
	const char **names,
	struct hir_expr **defs,
	int count)
{
	struct hir_expr *n;
	uint32_t i;

	if (e->type == HIR_EXPR_TERM &&
	    e->val.term.term->type == HIR_TERM_SYMBOL) {

		/* Substitute a matching temporary definition. */
		for (i = 0; i < (uint32_t)count; i++) {
			if (strcmp(names[i], e->val.term.term->val.symbol) == 0)
				return simd_clone_expr(defs[i]);
		}
	}
	n = simd_clone_expr(e);
	if (n == NULL)
		return NULL;

	/* Expand the children owned by this expression shape. */
	switch (e->type) {
	case HIR_EXPR_TERM:
		return n;
	case HIR_EXPR_PAR:
	case HIR_EXPR_NEG:
		n->val.unary.expr = simd_expand_expr(
			e->val.unary.expr,
			names,
			defs,
			count);
		if (n->val.unary.expr == NULL)
			return NULL;

		return n;
	case HIR_EXPR_CALL:

		/* Expand every call argument in source order. */
		for (i = 0; i < e->val.call.arg_count; i++) {
			n->val.call.arg[i] = simd_expand_expr(
				e->val.call.arg[i],
				names,
				defs,
				count);
			if (n->val.call.arg[i] == NULL)
				return NULL;
		}
		return n;
	case HIR_EXPR_SELECT:
		n->val.select.cond = simd_expand_expr(
			e->val.select.cond,
			names,
			defs,
			count);
		if (n->val.select.cond == NULL)
			return NULL;

		n->val.select.if_true = simd_expand_expr(
			e->val.select.if_true,
			names,
			defs,
			count);
		if (n->val.select.if_true == NULL)
			return NULL;

		n->val.select.if_false = simd_expand_expr(
			e->val.select.if_false,
			names,
			defs,
			count);
		if (n->val.select.if_false == NULL)
			return NULL;

		return n;
	case HIR_EXPR_PGATHER32:
		n->val.gather.index = simd_expand_expr(
			e->val.gather.index,
			names,
			defs,
			count);
		if (n->val.gather.index == NULL)
			return NULL;

		return n;
	default:
		n->val.binary.expr[0] = simd_expand_expr(
			e->val.binary.expr[0],
			names,
			defs,
			count);
		if (n->val.binary.expr[0] == NULL)
			return NULL;

		n->val.binary.expr[1] = simd_expand_expr(
			e->val.binary.expr[1],
			names,
			defs,
			count);
		if (n->val.binary.expr[1] == NULL)
			return NULL;

		return n;
	}
}

static bool
simd_live_expr(
	struct hir_expr *e,
	const char *sym)
{
	uint32_t i;

	if (e == NULL)
		return false;

	/* Search the children owned by this expression shape. */
	switch (e->type) {
	case HIR_EXPR_TERM:
		if (e->val.term.term->type != HIR_TERM_SYMBOL)
			return false;

		return strcmp(e->val.term.term->val.symbol, sym) == 0;
	case HIR_EXPR_PAR:
	case HIR_EXPR_NEG:
	case HIR_EXPR_NOT:
	case HIR_EXPR_PLEN:
	case HIR_EXPR_PBASE:
		return simd_live_expr(e->val.unary.expr, sym);
	case HIR_EXPR_CAPTURE:
		if (strcmp(e->val.capture.symbol, sym) == 0)
			return true;

		return simd_live_expr(e->val.capture.expr, sym);
	case HIR_EXPR_SELECT:
		if (simd_live_expr(e->val.select.cond, sym))
			return true;
		if (simd_live_expr(e->val.select.if_true, sym))
			return true;

		return simd_live_expr(e->val.select.if_false, sym);
	case HIR_EXPR_PGATHER32:
		if (simd_live_expr(e->val.gather.index, sym))
			return true;

		return simd_live_expr(e->val.gather.packed, sym);
	case HIR_EXPR_VINDUCTF32:
		if (simd_live_expr(e->val.binary.expr[0], sym))
			return true;

		return simd_live_expr(e->val.binary.expr[1], sym);
	case HIR_EXPR_DOT:
		return simd_live_expr(e->val.dot.obj, sym);
	case HIR_EXPR_CALL:
		if (simd_live_expr(e->val.call.func, sym))
			return true;

		/* Search the call arguments for the symbol. */
		for (i = 0; i < e->val.call.arg_count; i++) {
			if (simd_live_expr(e->val.call.arg[i], sym))
				return true;
		}

		return false;
	case HIR_EXPR_THISCALL:
		if (simd_live_expr(e->val.thiscall.obj, sym))
			return true;

		/* Search the method arguments for the symbol. */
		for (i = 0; i < e->val.thiscall.arg_count; i++) {
			if (simd_live_expr(e->val.thiscall.arg[i], sym))
				return true;
		}

		return false;
	case HIR_EXPR_ARRAY:

		/* Search every array element for the symbol. */
		for (i = 0; i < e->val.array.elem_count; i++) {
			if (simd_live_expr(e->val.array.elem[i], sym))
				return true;
		}

		return false;
	case HIR_EXPR_DICT:

		/* Search every dictionary value for the symbol. */
		for (i = 0; i < e->val.dict.kv_count; i++) {
			if (simd_live_expr(e->val.dict.value[i], sym))
				return true;
		}

		return false;
	case HIR_EXPR_NEW:
		return simd_live_expr(e->val.new_.init, sym);
	default:
		if (simd_live_expr(e->val.binary.expr[0], sym))
			return true;

		return simd_live_expr(e->val.binary.expr[1], sym);
	}
}

static bool
simd_live_chain(
	struct hir_block *head,
	const char *sym)
{
	struct hir_block *b;
	struct hir_block *c;
	struct hir_stmt *s;

	/* Search the reachable block chain for the symbol. */
	for (b = head; b != NULL; b = b->succ) {

		/* Search the expressions and children owned by this block shape. */
		switch (b->type) {
		case HIR_BLOCK_BASIC:

			/* Search both sides of every statement. */
			for (s = b->val.basic.stmt_list; s != NULL; s = s->next) {
				if (simd_live_expr(s->lhs, sym))
					return true;
				if (simd_live_expr(s->rhs, sym))
					return true;
			}
			break;
		case HIR_BLOCK_IF:

			/* Search every branch in the conditional chain. */
			for (c = b; c != NULL; c = c->val.if_.chain_next) {
				if (simd_live_expr(c->val.if_.cond, sym))
					return true;
				if (simd_live_chain(c->val.if_.inner, sym))
					return true;
			}
			break;
		case HIR_BLOCK_FOR:
			if (simd_live_expr(b->val.for_.start, sym))
				return true;
			if (simd_live_expr(b->val.for_.stop, sym))
				return true;
			if (simd_live_expr(b->val.for_.collection, sym))
				return true;
			if (simd_live_chain(b->val.for_.inner, sym))
				return true;
			break;
		case HIR_BLOCK_WHILE:
			if (simd_live_expr(b->val.while_.cond, sym))
				return true;
			if (simd_live_chain(b->val.while_.inner, sym))
				return true;
			break;
		default:
			break;
		}
		if (b->stop)
			break;
	}

	return false;
}

/*
 * Collapse a straight-line, pure, single-store body into the final store.
 * The caller keeps the original list and restores it on SIMD rejection.
 */
static struct hir_stmt *
simd_inline_temps(
	struct hir_block *loop)
{
	struct hir_block *body;
	struct hir_block *post;
	const char *names[SIMD_INLINE_MAX];
	struct hir_expr *defs[SIMD_INLINE_MAX];
	struct hir_stmt *s;
	struct hir_stmt *last;
	struct hir_stmt *out;
	struct hir_stmt *lead[4];
	int lead_count;
	int count;
	int i;

	body = loop->val.for_.inner;
	last = NULL;
	lead_count = 0;
	count = 0;

	if (body == NULL ||
	    body->type != HIR_BLOCK_BASIC ||
	    !body->stop) {
		return NULL;
	}

	s = body->val.basic.stmt_list;

	/* Preserve leading floating-point induction updates. */
	while (s != NULL &&
	       s->rhs != NULL &&
	       s->rhs->type == HIR_EXPR_VINDUCTF32 &&
	       lead_count < 4) {
		lead[lead_count++] = s;
		s = s->next;
	}

	/* Record pure temporary definitions before the final store. */
	for (;
	     s != NULL;
	     s = s->next) {
		last = s;
		if (s->next == NULL)
			break;
		if (s->lhs == NULL ||
		    s->lhs->type != HIR_EXPR_TERM ||
		    s->lhs->val.term.term->type != HIR_TERM_SYMBOL ||
		    count >= SIMD_INLINE_MAX) {
			return NULL;
		}
		if (!simd_inline_pure(s->rhs))
			return NULL;

		/* Reject duplicate temporary definitions. */
		for (i = 0; i < count; i++) {
			if (strcmp(
				    names[i],
				    s->lhs->val.term.term->val.symbol) == 0) {
				return NULL;
			}
		}
		names[count] = s->lhs->val.term.term->val.symbol;
		defs[count] = simd_expand_expr(s->rhs, names, defs, count);
		if (defs[count] == NULL)
			return NULL;
		count++;
	}

	if (count == 0 ||
	    last == NULL ||
	    last->lhs == NULL ||
	    (last->lhs->type != HIR_EXPR_PSTORE32 &&
	     last->lhs->type != HIR_EXPR_PSTOREF32)) {
		return NULL;
	}
	if (!simd_inline_pure(last->rhs))
		return NULL;

	/* FAST -> FEXIT -> X1 -> G2 -> X2 -> original successor. */
	post = loop->succ;
	if (post != NULL)
		post = post->succ;
	if (post != NULL)
		post = post->succ;
	if (post != NULL)
		post = post->succ;
	if (post != NULL)
		post = post->succ;

	/* Reject temporaries that remain live after the SIMD region. */
	for (i = 0; i < count; i++) {
		if (simd_live_chain(post, names[i]))
			return NULL;
	}
	out = hir_malloc(sizeof(*out));
	if (out == NULL) {
		hir_out_of_memory();
		return NULL;
	}

	memset(out, 0, sizeof(*out));
	out->line = last->line;

	out->lhs = simd_clone_expr(last->lhs);
	if (out->lhs == NULL)
		return NULL;

	out->rhs = simd_expand_expr(last->rhs, names, defs, count);
	if (out->rhs == NULL)
		return NULL;

	if (lead_count > 0) {
		struct hir_stmt *head;
		struct hir_stmt *tail;

		head = NULL;
		tail = NULL;

		/* Clone the preserved induction updates. */
		for (i = 0; i < lead_count; i++) {
			struct hir_stmt *n;

			n = hir_malloc(sizeof(*n));
			if (n == NULL) {
				hir_out_of_memory();
				return NULL;
			}

			memset(n, 0, sizeof(*n));
			n->line = lead[i]->line;

			n->lhs = simd_clone_expr(lead[i]->lhs);
			if (n->lhs == NULL)
				return NULL;

			n->rhs = simd_clone_expr(lead[i]->rhs);
			if (n->rhs == NULL)
				return NULL;

			if (tail == NULL)
				head = n;
			else
				tail->next = n;
			tail = n;
		}

		tail->next = out;

		return head;
	}

	return out;
}

/*
 * ------------------------------------------------------------------
 * Eligibility (design 06, E1..E9).
 * ------------------------------------------------------------------
 */

static bool
simd_is_local(
	struct simd_ctx *ctx,
	const char *name)
{
	struct hir_local *local;

	local = ctx->func->val.func.local;

	/* Search the function locals for the requested name. */
	while (local != NULL) {
		if (strcmp(local->symbol, name) == 0)
			return true;
		local = local->next;
	}

	return false;
}

static int
simd_local_type(
	struct simd_ctx *ctx,
	const char *name)
{
	struct hir_local *local;

	local = ctx->func->val.func.local;

	/* Find the proven type of the requested local. */
	while (local != NULL) {
		if (strcmp(local->symbol, name) == 0)
			return local->proven_type;
		local = local->next;
	}

	return -1;
}

/* Resolve a symbol type using the function-wide proven local facts. */
static int
simd_resolve_scalar_symbol(
	void *data,
	const char *symbol)
{
	struct simd_ctx *ctx;

	ctx = data;

	return simd_local_type(ctx, symbol);
}

/* Return the scalar type used by SIMD eligibility checks. */
static int
simd_expr_type(
	struct simd_ctx *ctx,
	struct hir_expr *e)
{
	struct hir_opt_scalar_query query;

	query.value_mask = HIR_OPT_SCALAR_ALL;
	query.arithmetic_mask = HIR_OPT_SCALAR_INT | HIR_OPT_SCALAR_FLOAT;
	query.data = ctx;
	query.resolve_symbol = simd_resolve_scalar_symbol;
	query.resolve_subscript = NULL;

	return hir_opt_expr_scalar_type(e, &query);
}

static struct simd_base *
simd_find_base(
	struct simd_ctx *ctx,
	const char *sym)
{
	int i;

	/* Find the registered base with the requested symbol. */
	for (i = 0; i < ctx->base_count; i++) {
		if (strcmp(ctx->bases[i].base_sym, sym) == 0)
			return &ctx->bases[i];
	}

	return NULL;
}

static bool
simd_note_const(
	struct simd_ctx *ctx,
	uint32_t v,
	int type)
{
	int i;

	/* Reuse an existing vector constant entry when possible. */
	for (i = 0; i < ctx->const_count; i++) {
		if (ctx->consts[i] == v &&
		    ctx->const_type[i] == type) {
			return true;
		}
	}

	if (ctx->const_count >= SIMD_MAX_CONSTS)
		return false;

	ctx->consts[ctx->const_count] = v;
	ctx->const_type[ctx->const_count++] = (uint8_t)type;

	return true;
}

static bool
simd_in_list(
	const char **list,
	int count,
	const char *name)
{
	int i;

	/* Search the bounded symbol list for the requested name. */
	for (i = 0; i < count; i++) {
		if (strcmp(list[i], name) == 0)
			return true;
	}

	return false;
}

/* Parse a site index expr into an offset shape (counter-affine). */
static bool
simd_parse_index(
	struct simd_ctx *ctx,
	struct hir_expr *f,
	struct simd_off *out)
{
	struct hir_affine_index index;

	memset(out, 0, sizeof(*out));

	if (!hir_opt_normalize_index(f, ctx->counter, &index))
		return false;
	if (index.kind != HIR_AFFINE_COUNTER_OFFSET)
		return false;

	if (index.invariant_symbol == NULL &&
	    index.offset == 0) {
		out->shape = SIMD_SHAPE_I;
		return true;
	}
	if (index.invariant_symbol != NULL) {
		out->shape = index.invariant_sign < 0 ?
			SIMD_SHAPE_I_MINUS_U : SIMD_SHAPE_I_PLUS_U;
		out->u_name = index.invariant_symbol;
	} else {
		out->u_is_const = true;
		if (index.offset < 0) {
			if (index.offset == INT_MIN)
				return false;
			out->shape = SIMD_SHAPE_I_MINUS_U;
			out->u_const = -index.offset;
		} else {
			out->shape = SIMD_SHAPE_I_PLUS_U;
			out->u_const = index.offset;
		}
	}

	return true;
}

static bool
simd_off_equal(
	const struct simd_off *a,
	const struct simd_off *b)
{
	if (a->shape != b->shape)
		return false;
	if (a->u_is_const != b->u_is_const)
		return false;
	if (a->u_is_const)
		return a->u_const == b->u_const;
	if (a->u_name == NULL)
		return a->u_name == b->u_name;
	if (b->u_name == NULL)
		return a->u_name == b->u_name;

	return strcmp(a->u_name, b->u_name) == 0;
}

/* Register a homogeneous PLOAD/PSTORE i32 or f32 site. */
static bool
simd_note_site(
	struct simd_ctx *ctx,
	struct hir_expr *site,
	bool is_store)
{
	struct simd_base *base;
	struct simd_off off;
	const char *base_sym;
	int site_kind;
	int i;
	bool is_float;
	struct hir_expr *base_expr;
	struct hir_expr *index_expr;

	is_float = site->type == HIR_EXPR_PLOADF32 ||
		   site->type == HIR_EXPR_PSTOREF32;
	base_expr = site->type == HIR_EXPR_PMASKSTORE32 ?
		site->val.mask_store.base : site->val.binary.expr[0];
	index_expr = site->type == HIR_EXPR_PMASKSTORE32 ?
		site->val.mask_store.offset : site->val.binary.expr[1];

	if (!ctx->kind_set) {
		ctx->kind_set = true;
		ctx->is_float = is_float;
	} else if (is_float) {
		ctx->is_float = true;
	}

	if (base_expr->type != HIR_EXPR_TERM ||
	    base_expr->val.term.term->type != HIR_TERM_SYMBOL)
		return false;

	base_sym = base_expr->val.term.term->val.symbol;

	if (!simd_parse_index(ctx, index_expr, &off))
		return false;

	site_kind = is_float ? NOCT_PACKED_FLOAT32 : NOCT_PACKED_INT32;
	base = simd_find_base(ctx, base_sym);
	if (base == NULL) {
		if (ctx->base_count >= SIMD_MAX_BASES)
			return false;
		base = &ctx->bases[ctx->base_count++];
		memset(base, 0, sizeof(*base));
		base->base_sym = base_sym;
		base->element_kind = site_kind;
	} else if (base->element_kind != site_kind)
		return false;
	if (!is_store)
		base->has_read = true;
	if (is_store)
		base->has_store = true;

	/* Reuse an equivalent offset already registered for this base. */
	for (i = 0; i < base->off_count; i++) {
		if (simd_off_equal(&base->off[i], &off))
			return true;
	}

	if (base->off_count >= SIMD_MAX_OFFS)
		return false;

	base->off[base->off_count++] = off;

	return true;
}

static bool
simd_note_gather(
	struct simd_ctx *ctx,
	struct hir_expr *site)
{
	struct simd_base *base;
	const char *base_sym;
	struct hir_expr *b;

	b = site->val.gather.base;
	if (b->type != HIR_EXPR_TERM ||
	    b->val.term.term->type != HIR_TERM_SYMBOL)
		return false;

	base_sym = b->val.term.term->val.symbol;
	base = simd_find_base(ctx, base_sym);
	if (base == NULL) {
		if (ctx->base_count >= SIMD_MAX_BASES)
			return false;
		base = &ctx->bases[ctx->base_count++];
		memset(base, 0, sizeof(*base));
		base->base_sym = base_sym;
		base->element_kind = NOCT_PACKED_INT32;
	} else if (base->element_kind != NOCT_PACKED_INT32)
		return false;
	base->has_read = true;
	ctx->kind_set = true;
	ctx->has_checked_gather = true;

	return true;
}

/* Does the expression read the given symbol anywhere? */
static bool
simd_expr_reads(
	struct hir_expr *e,
	const char *sym)
{

	/* Dispatch according to the expression shape. */
	switch (e->type) {
	case HIR_EXPR_TERM:
		if (e->val.term.term->type != HIR_TERM_SYMBOL)
			return false;

		return strcmp(e->val.term.term->val.symbol, sym) == 0;
	case HIR_EXPR_PAR:
	case HIR_EXPR_NEG:
		return simd_expr_reads(e->val.unary.expr, sym);
	case HIR_EXPR_SELECT:
		if (simd_expr_reads(e->val.select.cond, sym))
			return true;
		if (simd_expr_reads(e->val.select.if_true, sym))
			return true;

		return simd_expr_reads(e->val.select.if_false, sym);
	case HIR_EXPR_PGATHER32:
		return simd_expr_reads(e->val.gather.index, sym);
	case HIR_EXPR_VINDUCTF32:
		/* The state operand is semantic writeback, not a vector read. */
		return simd_expr_reads(e->val.binary.expr[1], sym);
	case HIR_EXPR_CALL:
	{
		uint32_t i;

		/* Search each call argument for the symbol. */
		for (i = 0; i < e->val.call.arg_count; i++) {
			if (simd_expr_reads(e->val.call.arg[i], sym))
				return true;
		}
		return false;
	}
	case HIR_EXPR_PLOAD32:
	case HIR_EXPR_PLOADF32:
		return false;	/* base/index are not vector operands */
	default:
		if (simd_expr_reads(e->val.binary.expr[0], sym))
			return true;

		return simd_expr_reads(e->val.binary.expr[1], sym);
	}
}

/* PARs are transparent for operand-position decisions. */
static struct hir_expr *
simd_strip_par(
	struct hir_expr *e)
{

	/* Skip transparent parenthesized expressions. */
	while (e->type == HIR_EXPR_PAR)
		e = e->val.unary.expr;

	return e;
}

/*
 * Move a suffix of canonical strict-f32 recurrence updates to the vector
 * body head.  The new expression yields the four pre-update scalar states
 * and writes the fifth state back, so all original body reads see the lane
 * ramp while RFOR/SFOR retain the untouched scalar CFG.
 */
static bool
simd_normalize_fp_inductions(
	struct simd_ctx *ctx,
	struct hir_block *body)
{
	struct hir_stmt *v[64];
	struct hir_stmt *s;
	int n;
	int first;
	int i;
	int j;

	n = 0;

	if (body == NULL ||
	    body->type != HIR_BLOCK_BASIC) {
		return true;
	}

	/* Collect the bounded body statement list. */
	for (s = body->val.basic.stmt_list;
	     s != NULL &&
	     n < 64;
	     s = s->next) {
		v[n++] = s;
	}

	if (s != NULL)
		return false;

	first = n;

	/* Find the maximal suffix of canonical f32 induction updates. */
	while (first > 0) {
		struct hir_stmt *u;
		struct hir_expr *r;
		const char *state;
		const char *step;

		u = v[first - 1];
		r = u->rhs;
		if (u->lhs == NULL ||
		    u->lhs->type != HIR_EXPR_TERM ||
		    u->lhs->val.term.term->type != HIR_TERM_SYMBOL ||
		    r == NULL ||
		    r->type != HIR_EXPR_PLUS ||
		    r->val.binary.expr[0]->type != HIR_EXPR_TERM ||
		    r->val.binary.expr[1]->type != HIR_EXPR_TERM ||
		    r->val.binary.expr[0]->val.term.term->type != HIR_TERM_SYMBOL ||
		    r->val.binary.expr[1]->val.term.term->type != HIR_TERM_SYMBOL)
			break;
		state = u->lhs->val.term.term->val.symbol;
		step = r->val.binary.expr[1]->val.term.term->val.symbol;
		if (strcmp(
			    state,
			    r->val.binary.expr[0]->val.term.term->val.symbol) != 0) {
			break;
		}
		if (simd_local_type(ctx, state) != NOCT_VALUE_FLOAT)
			break;
		if (simd_local_type(ctx, step) != NOCT_VALUE_FLOAT)
			break;

		first--;
	}

	if (first == n)
		return true;

	/* No other assignment to an induction state is permitted. */
	for (i = first; i < n; i++) {
		const char *state;

		state = v[i]->lhs->val.term.term->val.symbol;

		/* Search the preceding statements for another assignment. */
		for (j = 0; j < first; j++) {
			if (v[j]->lhs != NULL &&
			    v[j]->lhs->type == HIR_EXPR_TERM &&
			    v[j]->lhs->val.term.term->type == HIR_TERM_SYMBOL &&
			    strcmp(v[j]->lhs->val.term.term->val.symbol, state) == 0)
				return false;
		}
		v[i]->rhs->type = HIR_EXPR_VINDUCTF32;
	}

	if (first > 0)
		v[first - 1]->next = NULL;

	v[n - 1]->next = body->val.basic.stmt_list;
	body->val.basic.stmt_list = v[first];
	ctx->has_fp_induction = true;

	return true;
}

/*
 * Expression walk: grammar check (E4), counter-position check (E5),
 * local read/const collection, and the extra-stack-slot need f(e)
 * for evaluating e into a given destination vreg (the destination
 * itself is not counted; TERM operands are consumed directly from
 * their home vregs).  Must stay in lockstep with the LIR planner
 * (lir_vfor_expr_need).  Returns -1 on rejection.
 */
static int
simd_check_expr(
	struct simd_ctx *ctx,
	struct hir_expr *e)
{
	int l, r;

	/* Terms and parentheses are free; every other node is one unit. */
	if (e->type != HIR_EXPR_TERM &&
	    e->type != HIR_EXPR_PAR) {
		ctx->body_cost++;
	}

	/* Check and account for the expression shape. */
	switch (e->type) {
	case HIR_EXPR_TERM:

		/* Account for the concrete term kind. */
		switch (e->val.term.term->type) {
		case HIR_TERM_INT:
			if (!simd_note_const(
				    ctx,
				    (uint32_t)e->val.term.term->val.i,
				    NOCT_VALUE_INT)) {
				SIMD_REJECT_I("E8 const cap");
			}

			return 0;
		case HIR_TERM_FLOAT:
		{
			uint32_t bits;

			memcpy(&bits, &e->val.term.term->val.f, sizeof(bits));
			if (!simd_note_const(ctx, bits, NOCT_VALUE_FLOAT))
				SIMD_REJECT_I("E8 const cap");
			return 0;
		}
		case HIR_TERM_SYMBOL:
		{
			const char *sym;

			sym = e->val.term.term->val.symbol;

			if (strcmp(sym, ctx->counter) == 0)
				SIMD_REJECT_I("E5 counter value");
			if (simd_find_base(ctx, sym) != NULL)
				SIMD_REJECT_I("E4 base ref");
			if (!simd_is_local(ctx, sym))
				SIMD_REJECT_I("E4 global");

			/* Record the read. */
			if (!simd_in_list(ctx->temp, ctx->temp_count, sym)) {
				if (!simd_in_list(ctx->inv, ctx->inv_count, sym)) {
					if (ctx->inv_count >= SIMD_MAX_LOCALS)
						SIMD_REJECT_I("E8 local cap");

					ctx->inv[ctx->inv_count++] = sym;
				}
			}

			return 0;
		}
		default:
			return -1;
		}
	case HIR_EXPR_PAR:
		return simd_check_expr(ctx, e->val.unary.expr);
	case HIR_EXPR_CALL:
		if (e->val.call.arg_count != 1)
			SIMD_REJECT_I("E4 call");
		if (hir_get_intrinsic_call(e) == HIR_INTRINSIC_NONE)
			SIMD_REJECT_I("E4 call");

		return simd_check_expr(ctx, e->val.call.arg[0]);
	case HIR_EXPR_LT:
	case HIR_EXPR_LTE:
	case HIR_EXPR_GT:
	case HIR_EXPR_GTE:
	case HIR_EXPR_EQ:
	case HIR_EXPR_NEQ:
		l = simd_check_expr(ctx, e->val.binary.expr[0]);
		if (l < 0)
			return -1;
		r = simd_check_expr(ctx, e->val.binary.expr[1]);
		if (r < 0)
			return -1;

		{
			int lt;
			int rt;

			lt = simd_expr_type(ctx, e->val.binary.expr[0]);
			rt = simd_expr_type(ctx, e->val.binary.expr[1]);
			if (lt >= 0 &&
			    rt >= 0 &&
			    lt != rt) {
				SIMD_REJECT_I("E4 compare types");
			}
		}

		return l > 1 + r ? l : 1 + r;
	case HIR_EXPR_SELECT:
	{
		int c, t, f, need;

		c = simd_check_expr(ctx, e->val.select.cond);
		if (c < 0)
			return -1;
		t = simd_check_expr(ctx, e->val.select.if_true);
		if (t < 0)
			return -1;
		f = simd_check_expr(ctx, e->val.select.if_false);
		if (f < 0)
			return -1;

		{
			int tt;
			int ft;

			tt = simd_expr_type(ctx, e->val.select.if_true);
			ft = simd_expr_type(ctx, e->val.select.if_false);
			if (tt >= 0 &&
			    ft >= 0 &&
			    tt != ft) {
				SIMD_REJECT_I("E4 select types");
			}
		}
		need = c;
		if (1 + t > need)
			need = 1 + t;
		if (2 + f > need)
			need = 2 + f;

		return need;
	}
	case HIR_EXPR_PLOAD32:
	case HIR_EXPR_PLOADF32:
		if (!simd_note_site(ctx, e, false))
			SIMD_REJECT_I("E7 load site");
		return 0;	/* loads go straight to the destination */
	case HIR_EXPR_PGATHER32:
		if (!simd_note_gather(ctx, e))
			SIMD_REJECT_I("E7 gather site");

		l = simd_check_expr(ctx, e->val.gather.index);
		if (l < 0)
			SIMD_REJECT_I("E7 gather index");
		if (simd_expr_type(ctx, e->val.gather.index) != NOCT_VALUE_INT)
			SIMD_REJECT_I("E7 gather index");

		return l;
	case HIR_EXPR_VINDUCTF32:
		if (e->val.binary.expr[0]->type != HIR_EXPR_TERM ||
		    e->val.binary.expr[1]->type != HIR_EXPR_TERM ||
		    e->val.binary.expr[0]->val.term.term->type != HIR_TERM_SYMBOL ||
		    e->val.binary.expr[1]->val.term.term->type != HIR_TERM_SYMBOL) {
			SIMD_REJECT_I("E6 f32 induction");
		}
		if (simd_local_type(
			    ctx,
			    e->val.binary.expr[0]->val.term.term->val.symbol) !=
		    NOCT_VALUE_FLOAT) {
			SIMD_REJECT_I("E6 f32 induction");
		}
		if (simd_local_type(
			    ctx,
			    e->val.binary.expr[1]->val.term.term->val.symbol) !=
		    NOCT_VALUE_FLOAT) {
			SIMD_REJECT_I("E6 f32 induction");
		}

		ctx->kind_set = true;
		ctx->is_float = true;
		ctx->has_fp_induction = true;
		return 0;
	case HIR_EXPR_PLUS:
	case HIR_EXPR_MINUS:
	case HIR_EXPR_MUL:
	case HIR_EXPR_DIV:
	case HIR_EXPR_AND:
	case HIR_EXPR_OR:
	case HIR_EXPR_XOR:
	{
		/*
		 * f(e) = extra stack slots to evaluate e into a given
		 * destination, with destination reuse: a non-term left
		 * operand is built in the destination itself, and for
		 * commutative ops a lone non-term right operand is too.
		 * Must mirror lir_vfor_expr() exactly.
		 */
		bool lterm, rterm;
		bool commutative;

		commutative = e->type != HIR_EXPR_MINUS &&
			      e->type != HIR_EXPR_DIV;
		l = simd_check_expr(ctx, e->val.binary.expr[0]);
		if (l < 0)
			return -1;
		r = simd_check_expr(ctx, e->val.binary.expr[1]);
		if (r < 0)
			return -1;

		if (simd_expr_type(ctx, e) < 0) {
			if (!ctx->kind_set)
				SIMD_REJECT_I("E4 mixed types");
		}

		if (e->type == HIR_EXPR_DIV) {
			if (simd_expr_type(ctx, e) == NOCT_VALUE_INT)
				SIMD_REJECT_I("E4 i32 divide");
			if (simd_expr_type(ctx, e) < 0) {
				if (!ctx->is_float)
					SIMD_REJECT_I("E4 i32 divide");
			}
		}

		lterm = simd_strip_par(e->val.binary.expr[0])->type == HIR_EXPR_TERM;
		rterm = simd_strip_par(e->val.binary.expr[1])->type == HIR_EXPR_TERM;
		if (lterm &&
		    rterm) {
			return 0;
		}
		if (lterm)
			return commutative ? r : 1 + r;
		if (rterm)
			return l;
		return l > (1 + r) ? l : (1 + r);
	}
	case HIR_EXPR_SHL:
	case HIR_EXPR_SHR:
	{
		struct hir_expr *c;

		c = e->val.binary.expr[1];
		if (c->type != HIR_EXPR_TERM ||
		    c->val.term.term->type != HIR_TERM_INT)
			SIMD_REJECT_I("E9 shift count");
		if (c->val.term.term->val.i < 0 ||
		    c->val.term.term->val.i > 31)
			SIMD_REJECT_I("E9 shift range");
		l = simd_check_expr(ctx, e->val.binary.expr[0]);
		if (l < 0)
			return -1;

		if (simd_expr_type(ctx, e) != NOCT_VALUE_INT) {
			if (simd_expr_type(ctx, e) >= 0)
				SIMD_REJECT_I("E4 f32 shift");
			if (!ctx->kind_set)
				SIMD_REJECT_I("E4 f32 shift");
			if (ctx->is_float)
				SIMD_REJECT_I("E4 f32 shift");
		}

		/* Shift is applied in place on the destination. */
		return l;
	}
	default:
		/*
		 * Reject DIV, MOD, comparisons, LAND/LOR, NEG, NOT,
		 * CAPTURE, other PLOAD/PSTORE widths, DOT, CALL, and peers.
		 */
		SIMD_REJECT_I("E4 grammar");
	}
}

/* Scan the (single) body block; returns false if ineligible. */
static bool
simd_check_body(
	struct simd_ctx *ctx,
	struct hir_block *body)
{
	struct hir_stmt *stmt;
	int d;

	if (body == NULL ||
	    body->type != HIR_BLOCK_BASIC ||
	    !body->stop ||
	    body->is_return_edge ||
	    body->is_break_edge ||
	    body->is_continue_edge) {
		SIMD_REJECT("E2 body shape");
	}

	/* Check every statement in the single basic body. */
	for (stmt = body->val.basic.stmt_list; stmt != NULL; stmt = stmt->next) {
		if (stmt->lhs == NULL)
			SIMD_REJECT("E3 bare stmt");
		if (stmt->lhs->type == HIR_EXPR_TERM &&
		    stmt->lhs->val.term.term->type == HIR_TERM_SYMBOL) {
			const char *sym;

			sym = stmt->lhs->val.term.term->val.symbol;
			if (strcmp(sym, "$return") == 0)
				SIMD_REJECT("E3 return");
			if (simd_find_base(ctx, sym) != NULL)
				SIMD_REJECT("E3 base assign");

			d = simd_check_expr(ctx, stmt->rhs);
			ctx->body_cost++; /* vector temporary assignment */
			if (d < 0)
				return false;

			/*
			 * If the RHS reads the assigned temp, the value is built
			 * in a stack slot and moved.  Its home vreg stays intact.
			 */
			if (simd_expr_reads(stmt->rhs, sym))
				d = d + 1;
			if (d > ctx->max_depth)
				ctx->max_depth = d;

			/* The common scalar-effect summary owns the carried check. */
			if (!simd_in_list(ctx->temp, ctx->temp_count, sym)) {
				if (ctx->temp_count >= SIMD_MAX_LOCALS)
					SIMD_REJECT("E6 temp cap");
				ctx->temp[ctx->temp_count++] = sym;
			}
		} else if (stmt->lhs->type == HIR_EXPR_PSTORE32 ||
			   stmt->lhs->type == HIR_EXPR_PSTOREF32) {
			if (!simd_note_site(ctx, stmt->lhs, true))
				SIMD_REJECT("E7 store site");

			d = simd_check_expr(ctx, stmt->rhs);
			ctx->body_cost++; /* packed store */
			if (d < 0)
				return false;

			/* A non-term store value is built in one slot. */
			if (simd_strip_par(stmt->rhs)->type != HIR_EXPR_TERM)
				d = d + 1;
			if (d > ctx->max_depth)
				ctx->max_depth = d;
		} else if (stmt->lhs->type == HIR_EXPR_PMASKSTORE32) {
			int m;

			if (!simd_note_site(ctx, stmt->lhs, true))
				SIMD_REJECT("E7 masked store site");
			m = simd_check_expr(ctx, stmt->lhs->val.mask_store.mask);
			d = simd_check_expr(ctx, stmt->rhs);
			ctx->body_cost++;
			if (m < 0)
				return false;
			if (d < 0)
				return false;

			/* Store value and mask must be live together. */
			m++;
			d++;
			if (d > m)
				m = d;
			if (m > ctx->max_depth)
				ctx->max_depth = m;
		} else {
			SIMD_REJECT("E1 store width");
		}
	}

	/* E8: the vreg budget (mirror of the LIR planner). */
	if (ctx->const_count + ctx->inv_count + ctx->temp_count +
	    ctx->max_depth > SIMD_VREG_MAX)
		SIMD_REJECT("E8 vreg budget");

	/* Round ceil(min-work/body-cost) up to a complete four-lane group. */
	ctx->min_trip = (SIMD_MIN_WORK + ctx->body_cost - 1) /
		ctx->body_cost;
	if (ctx->min_trip < 4)
		ctx->min_trip = 4;

	ctx->min_trip = (ctx->min_trip + 3) & ~3;

	return true;
}

/*
 * Find the loop's surroundings: parent guard IF (G1), the PBASE
 * hoist block B1, and map base symbols to packed symbols.
 */
static bool
simd_find_environment(
	struct simd_ctx *ctx)
{
	struct hir_block *g1;
	struct hir_block *b1;
	struct hir_stmt *s;
	int i;

	g1 = ctx->loop->parent;
	if (g1 == NULL ||
	    g1->type != HIR_BLOCK_IF) {
		SIMD_REJECT("env G1");
	}

	b1 = g1->val.if_.inner;
	if (b1 == NULL ||
	    b1->type != HIR_BLOCK_BASIC) {
		SIMD_REJECT("env B1");
	}
	if (b1->succ != ctx->loop)
		SIMD_REJECT("env B1 succ");

	ctx->b1 = b1;

	/* $baseK = PBASE(pK) statements give the packed mapping. */
	for (s = b1->val.basic.stmt_list; s != NULL; s = s->next) {
		struct simd_base *base;

		if (s->lhs == NULL ||
		    s->rhs == NULL) {
			continue;
		}
		if (s->lhs->type != HIR_EXPR_TERM ||
		    s->lhs->val.term.term->type != HIR_TERM_SYMBOL)
			continue;
		if (s->rhs->type != HIR_EXPR_PBASE)
			continue;
		if (s->rhs->val.unary.expr->type != HIR_EXPR_TERM ||
		    s->rhs->val.unary.expr->val.term.term->type != HIR_TERM_SYMBOL)
			continue;

		base = simd_find_base(ctx, s->lhs->val.term.term->val.symbol);
		if (base != NULL) {
			base->packed_sym =
				s->rhs->val.unary.expr->val.term.term->val.symbol;
		}
	}

	/* Resolve restrictions for every registered packed base. */
	for (i = 0; i < ctx->base_count; i++) {
		uint32_t k;

		if (ctx->bases[i].packed_sym == NULL)
			SIMD_REJECT("env packed map");

		/* Find the matching restricted function parameter. */
		for (k = 0; k < ctx->func->val.func.param_count; k++) {
			if (ctx->func->val.func.param_restricted[k] &&
			    strcmp(
				    ctx->func->val.func.param_name[k],
				    ctx->bases[i].packed_sym) == 0) {
				ctx->bases[i].restricted = true;
				break;
			}
		}
	}

	/* $lo/$hi from the loop bounds (symbols by construction). */
	if (ctx->loop->val.for_.start->type != HIR_EXPR_TERM ||
	    ctx->loop->val.for_.start->val.term.term->type != HIR_TERM_SYMBOL)
		SIMD_REJECT("env lo");
	if (ctx->loop->val.for_.stop->type != HIR_EXPR_TERM ||
	    ctx->loop->val.for_.stop->val.term.term->type != HIR_TERM_SYMBOL)
		SIMD_REJECT("env hi");

	ctx->lo_name = ctx->loop->val.for_.start->val.term.term->val.symbol;
	ctx->hi_name = ctx->loop->val.for_.stop->val.term.term->val.symbol;

	return true;
}

static bool
simd_is_fp_induction_symbol(
	struct simd_ctx *ctx,
	const char *symbol)
{
	struct hir_stmt *stmt;
	struct hir_expr *lhs;

	if (!ctx->has_fp_induction ||
	    ctx->loop->val.for_.inner == NULL ||
	    ctx->loop->val.for_.inner->type != HIR_BLOCK_BASIC) {
		return false;
	}

	/* Find a canonical induction write to the requested symbol. */
	for (stmt = ctx->loop->val.for_.inner->val.basic.stmt_list;
	     stmt != NULL; stmt = stmt->next) {
		lhs = stmt->lhs;
		if (lhs == NULL)
			continue;
		if (lhs->type != HIR_EXPR_TERM)
			continue;
		if (lhs->val.term.term->type != HIR_TERM_SYMBOL)
			continue;
		if (strcmp(lhs->val.term.term->val.symbol, symbol) != 0)
			continue;
		if (stmt->rhs == NULL)
			continue;
		if (stmt->rhs->type == HIR_EXPR_VINDUCTF32)
			return true;
	}

	return false;
}

static bool
simd_build_parallel_facts(
	struct simd_ctx *ctx,
	bool *safe,
	const char **reason)
{
	struct hir_memory_object object;
	const struct hir_scalar_effect *scalar;
	uint32_t i;

	*safe = false;
	*reason = "internal";
	hir_memory_catalog_init(&ctx->parallel_catalog);
	ctx->parallel_catalog.allow_non_affine_reads = true;

	/* Add every SIMD base to the shared memory catalog. */
	for (i = 0; i < (uint32_t)ctx->base_count; i++) {
		memset(&object, 0, sizeof(object));
		object.id = (int)i;
		object.symbol = ctx->bases[i].base_sym;
		object.source_line = ctx->loop->line;
		object.element_kind = ctx->bases[i].element_kind;
		object.element_width = 4;
		object.storage = HIR_MEMORY_STORAGE_PARAMETER;
		/*
		 * Logical Packed identity is not sufficient for normal func:
		 * preallocated buffers can partially overlap.  Preserve every
		 * cross-object write pair as a runtime alias requirement.
		 */
		object.alias_kind = HIR_ALIAS_MAY_ALIAS;
		object.alias_class = (int)i;
		object.readable = ctx->bases[i].has_read;
		object.writable = ctx->bases[i].has_store;
		if (!hir_memory_catalog_add(&ctx->parallel_catalog, &object)) {
			*reason = "catalog";
			return true;
		}
	}

	if (!hir_loop_analyze(
		    ctx->func,
		    ctx->loop,
		    &ctx->parallel_catalog,
		    &ctx->parallel_summary)) {
		return false;
	}

	if (ctx->parallel_summary->analysis_status != HIR_ANALYSIS_COMPLETE) {
		*reason = hir_parallel_reason_string(
			ctx->parallel_summary->analysis_reason);
		return true;
	}

	/* Reject scalar dependences not handled as f32 inductions. */
	for (i = 0; i < ctx->parallel_summary->scalar_count; i++) {
		scalar = &ctx->parallel_summary->scalar[i];
		if (scalar->is_counter ||
		    scalar->writes == 0) {
			continue;
		}
		if (simd_is_fp_induction_symbol(ctx, scalar->symbol))
			continue;
		/*
		 * A write-before-read scalar is SIMD-private/lastprivate; the
		 * existing planner handles its optional live-out extraction.
		 */
		if (scalar->read_before_write) {
			*reason = "scalar-carried";
			return true;
		}
	}

	if (!hir_doall_classify_memory(
		    ctx->parallel_summary,
		    &ctx->parallel_memory)) {
		return false;
	}

	if (ctx->parallel_memory.classification == HIR_PAR_CLASS_DEPENDENT) {
		*reason = hir_parallel_reason_string(ctx->parallel_memory.reason);
		return true;
	}
	if (ctx->parallel_memory.classification == HIR_PAR_CLASS_UNKNOWN &&
	    ctx->parallel_memory.reason != HIR_PAR_REASON_MAY_ALIAS) {
		*reason = hir_parallel_reason_string(ctx->parallel_memory.reason);
		return true;
	}
	*safe = true;
	*reason = ctx->parallel_memory.alias_requirement_count != 0 ?
		"runtime-alias-guard" : "none";

	return true;
}

static bool
simd_alias_required(
	const struct simd_ctx *ctx,
	int first,
	int second)
{
	uint32_t i;

	/* Search the shared analysis requirements for this base pair. */
	for (i = 0; i < ctx->parallel_memory.alias_requirement_count; i++) {
		if (ctx->parallel_memory.alias_requirement[i].first_object_id == first &&
		    ctx->parallel_memory.alias_requirement[i].second_object_id == second)
			return true;
	}

	return false;
}

static void
simd_free_parallel_facts(
	struct simd_ctx *ctx)
{
	if (ctx->parallel_summary != NULL) {
		hir_loop_summary_free(ctx->parallel_summary);
		ctx->parallel_summary = NULL;
	}
}

/* Count the function's locals (frame-budget check). */
static int
simd_count_locals(
	struct simd_ctx *ctx)
{
	struct hir_local *local;
	int n;

	local = ctx->func->val.func.local;
	n = 0;

	/* Count every local in the function frame. */
	while (local != NULL) {
		n++;
		local = local->next;
	}

	return n;
}

/* Build the site's index offset as an int expression (u or const). */
static struct hir_expr *
simd_mk_u_expr(
	const struct simd_off *off)
{
	if (off->u_is_const)
		return simd_mk_int(off->u_const);

	return simd_mk_sym(off->u_name);
}

/* Rewrite VFOR body sites to (adjusted base, bare counter). */
static bool
simd_rewrite_expr(
	struct simd_ctx *ctx,
	struct hir_expr *e)
{
	if (e == NULL)
		return true;

	/* Rewrite each supported expression shape in place. */
	switch (e->type) {
	case HIR_EXPR_TERM:
		return true;
	case HIR_EXPR_PAR:
		return simd_rewrite_expr(ctx, e->val.unary.expr);
	case HIR_EXPR_CALL:
	{
		uint32_t i;

		/* Rewrite every call argument in source order. */
		for (i = 0; i < e->val.call.arg_count; i++) {
			if (!simd_rewrite_expr(ctx, e->val.call.arg[i]))
				return false;
		}

		return true;
	}
	case HIR_EXPR_SELECT:
		if (!simd_rewrite_expr(ctx, e->val.select.cond))
			return false;
		if (!simd_rewrite_expr(ctx, e->val.select.if_true))
			return false;

		return simd_rewrite_expr(ctx, e->val.select.if_false);
	case HIR_EXPR_PMASKSTORE32:
	{
		struct simd_base *base;
		struct simd_off off;
		struct hir_expr *idx;
		int i;

		base = simd_find_base(
			ctx,
			e->val.mask_store.base->val.term.term->val.symbol);

		assert(base != NULL);

		if (!simd_parse_index(ctx, e->val.mask_store.offset, &off))
			return false;

		/* Find the registered form of this masked-store offset. */
		for (i = 0; i < base->off_count; i++) {
			if (simd_off_equal(&base->off[i], &off))
				break;
		}

		assert(i < base->off_count);

		if (base->off[i].sb_name[0] != '\0') {
			e->val.mask_store.base = simd_mk_sym(base->off[i].sb_name);
			if (e->val.mask_store.base == NULL)
				return false;
		}

		idx = simd_mk_sym(ctx->counter);
		if (idx == NULL)
			return false;

		e->val.mask_store.offset = idx;

		return simd_rewrite_expr(ctx, e->val.mask_store.mask);
	}
	case HIR_EXPR_PGATHER32:
		/* Base/length are scalar hoists; only the lane index is vector. */
		return simd_rewrite_expr(ctx, e->val.gather.index);
	case HIR_EXPR_VINDUCTF32:
		return true;
	case HIR_EXPR_PLOAD32:
	case HIR_EXPR_PSTORE32:
	case HIR_EXPR_PLOADF32:
	case HIR_EXPR_PSTOREF32:
	{
		struct simd_base *base;
		struct simd_off off;
		struct hir_expr *idx;
		int i;

		base = simd_find_base(
			ctx,
			e->val.binary.expr[0]->val.term.term->val.symbol);

		assert(base != NULL);

		if (!simd_parse_index(ctx, e->val.binary.expr[1], &off))
			return false;

		/* Find the registered form of this packed offset. */
		for (i = 0; i < base->off_count; i++) {
			if (simd_off_equal(&base->off[i], &off))
				break;
		}

		assert(i < base->off_count);

		if (base->off[i].sb_name[0] != '\0') {
			struct hir_expr *nb;

			nb = simd_mk_sym(base->off[i].sb_name);
			if (nb == NULL)
				return false;

			e->val.binary.expr[0] = nb;
		}

		idx = simd_mk_sym(ctx->counter);
		if (idx == NULL)
			return false;

		e->val.binary.expr[1] = idx;

		return true;
	}
	default:
		if (!simd_rewrite_expr(ctx, e->val.binary.expr[0]))
			return false;
		return simd_rewrite_expr(ctx, e->val.binary.expr[1]);
	}
}

/*
 * The transform.  Returns false only on OOM.
 */
static bool
simd_vectorize(
	struct simd_ctx *ctx)
{
	struct hir_block *F;
	struct hir_block *G1;
	struct hir_block *FEXIT;
	struct hir_block *GV, *GS, *XV, *XS, *RFOR, *SFOR, *EV, *ES;
	struct hir_block *body1;
	struct hir_block *body2;
	struct hir_stmt *tail;
	struct hir_expr *vg;
	struct hir_expr *vg_sym;
	int line;
	int i, j;

	F = ctx->loop;
	G1 = F->parent;
	FEXIT = F->succ;
	line = F->line;

	/* Names and locals. */
	snprintf(
		ctx->mid_name,
		sizeof(ctx->mid_name),
		"$simd%d_mid",
		simd_loop_seq);
	snprintf(
		ctx->vg_name,
		sizeof(ctx->vg_name),
		"$simd%d_vg",
		simd_loop_seq);
	if (!hir_add_local(ctx->func, ctx->mid_name))
		return false;
	if (!hir_add_local(ctx->func, ctx->vg_name))
		return false;

	/* Add adjusted-base locals for every non-trivial offset. */
	for (i = 0; i < ctx->base_count; i++) {
		struct simd_base *base;

		base = &ctx->bases[i];

		/* Name each adjusted-base local for this base. */
		for (j = 0; j < base->off_count; j++) {
			if (base->off[j].shape == SIMD_SHAPE_I) {
				base->off[j].sb_name[0] = '\0';
				continue;
			}
			snprintf(
				base->off[j].sb_name,
				sizeof(base->off[j].sb_name),
				"$simd%d_sb%d_%d",
				simd_loop_seq,
				i,
				j);
			if (!hir_add_local(ctx->func, base->off[j].sb_name))
				return false;
		}
	}
	simd_loop_seq++;

	/* Rewrite the vector body in place. */
	{
		struct hir_stmt *s;

		/* Rewrite both sides of every vector-body statement. */
		for (s = F->val.for_.inner->val.basic.stmt_list;
		     s != NULL; s = s->next) {
			if (!simd_rewrite_expr(ctx, s->lhs))
				return false;
			if (!simd_rewrite_expr(ctx, s->rhs))
				return false;
		}
	}

	/* Append to B1: $mid, adjusted bases, $vg. */
	tail = ctx->b1->val.basic.stmt_list;

	assert(tail != NULL);

	/* Find the append point in the PBASE hoist block. */
	while (tail->next != NULL)
		tail = tail->next;

	/* $mid = $hi - (($hi - $lo) & 3) */
	SIMD_APPEND(simd_mk_assign(line, simd_mk_sym(ctx->mid_name),
		simd_mk_binary(HIR_EXPR_MINUS, simd_mk_sym(ctx->hi_name),
			simd_mk_binary(HIR_EXPR_AND,
				simd_mk_binary(HIR_EXPR_MINUS,
					simd_mk_sym(ctx->hi_name),
					simd_mk_sym(ctx->lo_name)),
				simd_mk_int(3)))));

	/* $sbS = $baseK (+|-) 4L * u */

	/* Append every adjusted-base calculation. */
	for (i = 0; i < ctx->base_count; i++) {
		struct simd_base *base;

		base = &ctx->bases[i];

		/* Append the non-trivial offsets for this base. */
		for (j = 0; j < base->off_count; j++) {
			struct simd_off *off;
			int op;

			off = &base->off[j];
			if (off->sb_name[0] == '\0')
				continue;
			op = (off->shape == SIMD_SHAPE_I_MINUS_U) ?
				HIR_EXPR_MINUS : HIR_EXPR_PLUS;
			SIMD_APPEND(simd_mk_assign(line,
				simd_mk_sym(off->sb_name),
				simd_mk_binary(op,
					simd_mk_sym(base->base_sym),
					simd_mk_binary(HIR_EXPR_MUL,
						simd_mk_long(4),
						simd_mk_u_expr(off)))));
		}
	}

	/*
	 * $vg = (0 <= $lo) && ($lo < $mid) &&
	 *       ($mid - $lo >= min_trip) && <disjointness terms>
	 */
	vg = simd_mk_binary(HIR_EXPR_LAND,
		simd_mk_binary(HIR_EXPR_LTE, simd_mk_int(0),
			       simd_mk_sym(ctx->lo_name)),
		simd_mk_binary(HIR_EXPR_LT, simd_mk_sym(ctx->lo_name),
			       simd_mk_sym(ctx->mid_name)));
	if (vg == NULL)
		return false;
	vg = simd_mk_binary(HIR_EXPR_LAND, vg,
		simd_mk_binary(HIR_EXPR_GTE,
			simd_mk_binary(HIR_EXPR_MINUS,
				simd_mk_sym(ctx->mid_name),
				simd_mk_sym(ctx->lo_name)),
			simd_mk_int(ctx->min_trip)));
	if (vg == NULL)
		return false;

	/* Add every required pairwise disjointness guard. */
	for (i = 0; i < ctx->base_count; i++) {

		/* Compare this base with each later base. */
		for (j = i + 1; j < ctx->base_count; j++) {
			struct simd_base *P;
			struct simd_base *Q;
			struct hir_expr *p_end, *q_end, *disj;
			bool same_offs;

			P = &ctx->bases[i];
			Q = &ctx->bases[j];
			if (!simd_alias_required(ctx, i, j))
				continue;
			/* end = base + 4L * PLEN(packed) */
			p_end = simd_mk_binary(HIR_EXPR_PLUS,
				simd_mk_sym(P->base_sym),
				simd_mk_binary(HIR_EXPR_MUL, simd_mk_long(4),
					simd_mk_unary(HIR_EXPR_PLEN,
						simd_mk_sym(P->packed_sym))));
			if (p_end == NULL)
				return false;

			q_end = simd_mk_binary(HIR_EXPR_PLUS,
				simd_mk_sym(Q->base_sym),
				simd_mk_binary(HIR_EXPR_MUL, simd_mk_long(4),
					simd_mk_unary(HIR_EXPR_PLEN,
						simd_mk_sym(Q->packed_sym))));
			if (q_end == NULL)
				return false;

			disj = simd_mk_binary(HIR_EXPR_LOR,
				simd_mk_binary(HIR_EXPR_LTE, p_end,
					       simd_mk_sym(Q->base_sym)),
				simd_mk_binary(HIR_EXPR_LTE, q_end,
					       simd_mk_sym(P->base_sym)));
			if (disj == NULL)
				return false;

			/*
			 * Identical single offsets on the same object are
			 * element-aligned and safe, so allow equality.
			 */
			same_offs = false;
			if (!P->restricted &&
			    !Q->restricted &&
			    P->off_count == 1 &&
			    Q->off_count == 1) {
				same_offs = simd_off_equal(&P->off[0], &Q->off[0]);
			}

			if (same_offs) {
				disj = simd_mk_binary(HIR_EXPR_LOR, disj,
					simd_mk_binary(HIR_EXPR_EQ,
						simd_mk_sym(P->base_sym),
						simd_mk_sym(Q->base_sym)));
				if (disj == NULL)
					return false;
			}

			vg = simd_mk_binary(HIR_EXPR_LAND, vg, disj);
			if (vg == NULL)
				return false;
		}
	}
	SIMD_APPEND(simd_mk_assign(line, simd_mk_sym(ctx->vg_name), vg));

	/* Build the sibling structure. */
	GV = simd_mk_block(HIR_BLOCK_IF, line, G1);
	if (GV == NULL)
		return false;

	XV = simd_mk_block(HIR_BLOCK_BASIC, line, G1);
	if (XV == NULL)
		return false;

	GS = simd_mk_block(HIR_BLOCK_IF, line, G1);
	if (GS == NULL)
		return false;

	XS = simd_mk_block(HIR_BLOCK_BASIC, line, G1);
	if (XS == NULL)
		return false;

	RFOR = simd_mk_block(HIR_BLOCK_FOR, line, GV);
	if (RFOR == NULL)
		return false;

	EV = simd_mk_block(HIR_BLOCK_BASIC, line, GV);
	if (EV == NULL)
		return false;

	SFOR = simd_mk_block(HIR_BLOCK_FOR, line, GS);
	if (SFOR == NULL)
		return false;

	ES = simd_mk_block(HIR_BLOCK_BASIC, line, GS);
	if (ES == NULL)
		return false;

	/* Clone the source CFG before attaching the scalar loops. */
	body1 = simd_clone_scalar_chain(ctx->scalar_body, RFOR);
	if (body1 == NULL)
		return false;

	body2 = simd_clone_scalar_chain(ctx->scalar_body, SFOR);
	if (body2 == NULL)
		return false;

	/* RFOR (remainder): $mid..$hi, scalar clone 1. */
	RFOR->val.for_.is_ranged = true;
	RFOR->val.for_.counter_symbol = F->val.for_.counter_symbol;
	RFOR->val.for_.start = simd_mk_sym(ctx->mid_name);
	if (RFOR->val.for_.start == NULL)
		return false;

	RFOR->val.for_.stop = simd_mk_sym(ctx->hi_name);
	if (RFOR->val.for_.stop == NULL)
		return false;

	RFOR->val.for_.typed_int_region = F->val.for_.typed_int_region;
	RFOR->val.for_.packed_lanes = 1;
	RFOR->val.for_.inner = body1;

	/* SFOR (unvectorized fallback): $lo..$hi, scalar clone 2. */
	SFOR->val.for_.is_ranged = true;
	SFOR->val.for_.counter_symbol = F->val.for_.counter_symbol;
	SFOR->val.for_.start = simd_mk_sym(ctx->lo_name);
	if (SFOR->val.for_.start == NULL)
		return false;

	SFOR->val.for_.stop = simd_mk_sym(ctx->hi_name);
	if (SFOR->val.for_.stop == NULL)
		return false;

	SFOR->val.for_.typed_int_region = F->val.for_.typed_int_region;
	SFOR->val.for_.packed_lanes = 1;
	SFOR->val.for_.inner = body2;

	/* F becomes the strip loop: $lo..$mid, vector body. */
	F->val.for_.stop = simd_mk_sym(ctx->mid_name);
	if (F->val.for_.stop == NULL)
		return false;
	F->val.for_.is_vector = true;
	F->val.for_.abce_fast = false;
	F->val.for_.packed_lanes = 4;
	F->parent = GV;

	/*
	 * Wire B1 -> GV{F -> RFOR -> EV} -> XV ->
	 * GS{SFOR -> ES} -> XS -> FEXIT.
	 */
	ctx->b1->succ = GV;
	GV->val.if_.cond = simd_mk_sym(ctx->vg_name);
	if (GV->val.if_.cond == NULL)
		return false;
	GV->val.if_.inner = F;
	GV->succ = XV;
	F->succ = RFOR;
	F->stop = false;
	RFOR->succ = EV;
	EV->stop = true;
	EV->succ = XV;
	XV->succ = GS;
	vg_sym = simd_mk_sym(ctx->vg_name);
	if (vg_sym == NULL)
		return false;

	GS->val.if_.cond = simd_mk_unary(HIR_EXPR_NOT, vg_sym);
	if (GS->val.if_.cond == NULL)
		return false;
	GS->val.if_.inner = SFOR;
	GS->succ = XS;
	SFOR->succ = ES;
	ES->stop = true;
	ES->succ = XS;
	XS->succ = FEXIT;

	return true;
}

/* Collect abce_fast loops (snapshot; the transform rewires blocks). */
static void
simd_collect_loops(
	struct hir_block *head,
	struct hir_block **loops,
	int *count)
{
	struct hir_block *b;
	struct hir_block *c;

	b = head;

	/* Walk the reachable block chain and snapshot eligible loops. */
	while (b != NULL) {

		/* Recurse according to the current block shape. */
		switch (b->type) {
		case HIR_BLOCK_IF:
			c = b;

			/* Visit every branch in the conditional chain. */
			while (c != NULL) {
				if (c->val.if_.inner != NULL) {
					simd_collect_loops(
						c->val.if_.inner,
						loops,
						count);
				}

				c = c->val.if_.chain_next;
			}
			break;
		case HIR_BLOCK_FOR:
			if (b->val.for_.abce_fast &&
			    *count < SIMD_MAX_LOOPS) {
				loops[(*count)++] = b;
			}
			if (b->val.for_.inner != NULL) {
				simd_collect_loops(
					b->val.for_.inner,
					loops,
					count);
			}
			break;
		case HIR_BLOCK_WHILE:
			if (b->val.while_.inner != NULL) {
				simd_collect_loops(
					b->val.while_.inner,
					loops,
					count);
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
