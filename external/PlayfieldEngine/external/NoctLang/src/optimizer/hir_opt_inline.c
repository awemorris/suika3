/* -*- coding: utf-8; tab-width: 8; indent-tabs-mode: t; -*- */

/*
 * Noct Programming Language
 * Copyright (c) 2025, 2026, Awe Morris
 */

/*
 * Conservative HIR inlining for file-local `static __inline` functions.
 */

#include <noct/noct.h>
#include "hir.h"
#include "hir_opt.h"

#include <string.h>

#define INLINE_DEPTH_MAX 32

/* Forward declarations. */
static struct hir_term *inline_clone_term(const struct hir_term *src);
static int inline_param_index(const struct hir_block *callee, const char *name);
static struct hir_expr *inline_clone_expr(const struct hir_expr *src, const struct hir_block *callee, struct hir_expr *const *arg);
static bool inline_pure_expr(const struct hir_expr *e);
static void inline_count_params(const struct hir_expr *e, const struct hir_block *callee, unsigned *count);
static struct hir_block *inline_find_callee(const char *name);
static const struct hir_expr *inline_return_expr(const struct hir_block *callee);
static bool inline_try_call(struct hir_expr **ep, struct hir_block *caller, int depth);
static bool inline_rewrite_expr(struct hir_expr **ep, struct hir_block *caller, int depth);
static bool inline_rewrite_chain(struct hir_block *head, struct hir_block *caller);

/*
 * Inlines eligible calls in one HIR function.
 */
bool
hir_opt_inline_func(
	struct hir_block *func_block)
{
	/* Keep fast calls checked until guarded inlining is implemented. */
	if (func_block->val.func.is_fast)
		return true;

	return inline_rewrite_chain(func_block->val.func.inner, func_block);
}

static struct hir_term *
inline_clone_term(
	const struct hir_term *src)
{
	struct hir_term *dst;

	dst = hir_malloc(sizeof(*dst));
	if (dst == NULL) {
		hir_out_of_memory();
		return NULL;
	}

	memcpy(dst, src, sizeof(*dst));
	if (src->type == HIR_TERM_SYMBOL ||
	    src->type == HIR_TERM_STRING) {
		dst->val.symbol = hir_strdup(src->val.symbol);
		if (dst->val.symbol == NULL) {
			hir_out_of_memory();
			return NULL;
		}
	}

	return dst;
}

static int
inline_param_index(
	const struct hir_block *callee,
	const char *name)
{
	uint32_t i;

	if (callee == NULL)
		return -1;

	/* Find the parameter with the requested name. */
	for (i = 0; i < callee->val.func.param_count; i++) {
		if (strcmp(callee->val.func.param_name[i], name) == 0)
			return (int)i;
	}

	return -1;
}

static struct hir_expr *
inline_clone_expr(
	const struct hir_expr *src,
	const struct hir_block *callee,
	struct hir_expr *const *arg)
{
	struct hir_expr *dst;
	uint32_t i;
	int p;

	if (src == NULL)
		return NULL;

	if (src->type == HIR_EXPR_TERM &&
	    src->val.term.term->type == HIR_TERM_SYMBOL) {
		p = inline_param_index(callee, src->val.term.term->val.symbol);
		if (p >= 0)
			return inline_clone_expr(arg[p], NULL, NULL);
	}

	dst = hir_malloc(sizeof(*dst));
	if (dst == NULL) {
		hir_out_of_memory();
		return NULL;
	}

	memset(dst, 0, sizeof(*dst));
	dst->type = src->type;

	/* Clone the payload for this expression shape. */
	switch (src->type) {
	case HIR_EXPR_TERM:
		dst->val.term.term = inline_clone_term(src->val.term.term);
		if (dst->val.term.term == NULL)
			return NULL;

		return dst;
	case HIR_EXPR_PAR:
	case HIR_EXPR_NEG:
	case HIR_EXPR_NOT:
	case HIR_EXPR_PBASE:
	case HIR_EXPR_PLEN:
		dst->val.unary.expr = inline_clone_expr(
			src->val.unary.expr,
			callee,
			arg);
		if (dst->val.unary.expr == NULL)
			return NULL;

		return dst;
	case HIR_EXPR_CAPTURE:
		dst->val.capture.expr = inline_clone_expr(
			src->val.capture.expr,
			callee,
			arg);
		if (dst->val.capture.expr == NULL)
			return NULL;

		dst->val.capture.symbol = hir_strdup(src->val.capture.symbol);
		if (dst->val.capture.symbol == NULL)
			return NULL;

		return dst;
	case HIR_EXPR_DOT:
		dst->val.dot.obj = inline_clone_expr(
			src->val.dot.obj,
			callee,
			arg);
		if (dst->val.dot.obj == NULL)
			return NULL;

		dst->val.dot.symbol = hir_strdup(src->val.dot.symbol);
		if (dst->val.dot.symbol == NULL)
			return NULL;

		return dst;
	case HIR_EXPR_CALL:
		dst->val.call.func = inline_clone_expr(
			src->val.call.func,
			callee,
			arg);
		if (dst->val.call.func == NULL)
			return NULL;

		dst->val.call.arg_count = src->val.call.arg_count;

		/* Clone every ordinary call argument. */
		for (i = 0; i < src->val.call.arg_count; i++) {
			dst->val.call.arg[i] = inline_clone_expr(
				src->val.call.arg[i],
				callee,
				arg);
			if (dst->val.call.arg[i] == NULL)
				return NULL;
		}

		return dst;
	case HIR_EXPR_THISCALL:
		dst->val.thiscall.obj = inline_clone_expr(
			src->val.thiscall.obj,
			callee,
			arg);
		if (dst->val.thiscall.obj == NULL)
			return NULL;

		dst->val.thiscall.func = hir_strdup(src->val.thiscall.func);
		if (dst->val.thiscall.func == NULL)
			return NULL;

		dst->val.thiscall.arg_count = src->val.thiscall.arg_count;

		/* Clone every method-call argument. */
		for (i = 0; i < src->val.thiscall.arg_count; i++) {
			dst->val.thiscall.arg[i] = inline_clone_expr(
				src->val.thiscall.arg[i],
				callee,
				arg);
			if (dst->val.thiscall.arg[i] == NULL)
				return NULL;
		}

		return dst;
	case HIR_EXPR_ARRAY:
		dst->val.array.elem_count = src->val.array.elem_count;
		dst->val.array.is_multi_index = src->val.array.is_multi_index;
		dst->val.array.elem = hir_malloc(
			sizeof(*dst->val.array.elem) * src->val.array.elem_count);
		if (dst->val.array.elem == NULL &&
		    src->val.array.elem_count != 0)
			return NULL;

		/* Clone every array element. */
		for (i = 0; i < src->val.array.elem_count; i++) {
			dst->val.array.elem[i] = inline_clone_expr(
				src->val.array.elem[i],
				callee,
				arg);
			if (dst->val.array.elem[i] == NULL)
				return NULL;
		}

		return dst;
	case HIR_EXPR_DICT:
		dst->val.dict.kv_count = src->val.dict.kv_count;
		dst->val.dict.key = hir_malloc(
			sizeof(*dst->val.dict.key) * src->val.dict.kv_count);
		if (dst->val.dict.key == NULL &&
		    src->val.dict.kv_count != 0)
			return NULL;

		dst->val.dict.value = hir_malloc(
			sizeof(*dst->val.dict.value) * src->val.dict.kv_count);
		if (dst->val.dict.value == NULL &&
		    src->val.dict.kv_count != 0)
			return NULL;

		/* Clone every dictionary entry. */
		for (i = 0; i < src->val.dict.kv_count; i++) {
			dst->val.dict.key[i] = hir_strdup(src->val.dict.key[i]);
			if (dst->val.dict.key[i] == NULL)
				return NULL;

			dst->val.dict.value[i] = inline_clone_expr(
				src->val.dict.value[i],
				callee,
				arg);
			if (dst->val.dict.value[i] == NULL)
				return NULL;
		}

		return dst;
	case HIR_EXPR_NEW:
		dst->val.new_.cls = hir_strdup(src->val.new_.cls);
		if (dst->val.new_.cls == NULL)
			return NULL;

		dst->val.new_.init = inline_clone_expr(
			src->val.new_.init,
			callee,
			arg);
		if (src->val.new_.init != NULL &&
		    dst->val.new_.init == NULL)
			return NULL;

		return dst;
	default:
		dst->val.binary.expr[0] = inline_clone_expr(
			src->val.binary.expr[0],
			callee,
			arg);
		if (dst->val.binary.expr[0] == NULL)
			return NULL;

		dst->val.binary.expr[1] = inline_clone_expr(
			src->val.binary.expr[1],
			callee,
			arg);
		if (dst->val.binary.expr[1] == NULL)
			return NULL;

		return dst;
	}
}

static bool
inline_pure_expr(
	const struct hir_expr *e)
{
	if (e == NULL)
		return false;

	/* Classify the expression and recursively check its operands. */
	switch (e->type) {
	case HIR_EXPR_TERM:
		return true;
	case HIR_EXPR_PAR:
	case HIR_EXPR_NEG:
	case HIR_EXPR_NOT:
		return inline_pure_expr(e->val.unary.expr);
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
		if (!inline_pure_expr(e->val.binary.expr[0]))
			return false;

		return inline_pure_expr(e->val.binary.expr[1]);
	default:
		return false;
	}
}

static void
inline_count_params(
	const struct hir_expr *e,
	const struct hir_block *callee,
	unsigned *count)
{
	uint32_t i;
	int p;

	if (e == NULL)
		return;
	if (e->type == HIR_EXPR_TERM) {
		if (e->val.term.term->type == HIR_TERM_SYMBOL) {
			p = inline_param_index(callee, e->val.term.term->val.symbol);
			if (p >= 0)
				count[p]++;
		}
		return;
	}

	/* Count parameter references in this expression shape. */
	switch (e->type) {
	case HIR_EXPR_PAR:
	case HIR_EXPR_NEG:
	case HIR_EXPR_NOT:
	case HIR_EXPR_PBASE:
	case HIR_EXPR_PLEN:
		inline_count_params(e->val.unary.expr, callee, count);
		return;
	case HIR_EXPR_CAPTURE:
		inline_count_params(e->val.capture.expr, callee, count);
		return;
	case HIR_EXPR_DOT:
		inline_count_params(e->val.dot.obj, callee, count);
		return;
	case HIR_EXPR_CALL:
		inline_count_params(e->val.call.func, callee, count);

		/* Count references in every ordinary call argument. */
		for (i = 0; i < e->val.call.arg_count; i++)
			inline_count_params(e->val.call.arg[i], callee, count);
		return;
	case HIR_EXPR_THISCALL:
		inline_count_params(e->val.thiscall.obj, callee, count);

		/* Count references in every method-call argument. */
		for (i = 0; i < e->val.thiscall.arg_count; i++)
			inline_count_params(e->val.thiscall.arg[i], callee, count);
		return;
	case HIR_EXPR_ARRAY:

		/* Count references in every array element. */
		for (i = 0; i < e->val.array.elem_count; i++)
			inline_count_params(e->val.array.elem[i], callee, count);
		return;
	case HIR_EXPR_DICT:

		/* Count references in every dictionary value. */
		for (i = 0; i < e->val.dict.kv_count; i++)
			inline_count_params(e->val.dict.value[i], callee, count);
		return;
	case HIR_EXPR_NEW:
		inline_count_params(e->val.new_.init, callee, count);
		return;
	default:
		inline_count_params(e->val.binary.expr[0], callee, count);
		inline_count_params(e->val.binary.expr[1], callee, count);
		return;
	}
}

static struct hir_block *
inline_find_callee(
	const char *name)
{
	uint32_t i;

	/* Find an inlineable file-local function with this name. */
	for (i = 0; i < hir_get_function_count(); i++) {
		struct hir_block *f;

		f = hir_get_function(i);
		if (!f->val.func.is_static)
			continue;
		if (!f->val.func.is_inline)
			continue;
		if (f->val.func.is_fast)
			continue;
		if (strcmp(f->val.func.name, name) == 0)
			return f;
	}

	return NULL;
}

static const struct hir_expr *
inline_return_expr(
	const struct hir_block *callee)
{
	const struct hir_block *b;
	const struct hir_stmt *s;

	b = callee->val.func.inner;

	if (b == NULL)
		return NULL;
	if (b->type != HIR_BLOCK_BASIC)
		return NULL;
	if (!b->stop)
		return NULL;

	s = b->val.basic.stmt_list;
	if (s == NULL)
		return NULL;
	if (s->next != NULL)
		return NULL;
	if (s->lhs == NULL)
		return NULL;
	if (s->is_bare_return)
		return NULL;
	if (s->lhs->type != HIR_EXPR_TERM)
		return NULL;
	if (s->lhs->val.term.term->type != HIR_TERM_SYMBOL)
		return NULL;
	if (strcmp(s->lhs->val.term.term->val.symbol, "$return") != 0)
		return NULL;

	return s->rhs;
}

static bool
inline_try_call(
	struct hir_expr **ep,
	struct hir_block *caller,
	int depth)
{
	struct hir_expr *e;
	struct hir_block *callee;
	const struct hir_expr *body;
	unsigned count[HIR_PARAM_SIZE];
	uint32_t i;
	struct hir_expr *replacement;

	e = *ep;
	memset(count, 0, sizeof(count));

	if (e->type != HIR_EXPR_CALL)
		return true;
	if (e->val.call.func == NULL)
		return true;
	if (e->val.call.func->type != HIR_EXPR_TERM)
		return true;
	if (e->val.call.func->val.term.term->type != HIR_TERM_SYMBOL)
		return true;

	callee = inline_find_callee(e->val.call.func->val.term.term->val.symbol);
	if (callee == NULL)
		return true;
	if (callee == caller)
		return true;
	if (callee->val.func.param_count != e->val.call.arg_count)
		return true;

	body = inline_return_expr(callee);
	if (body == NULL)
		return true;

	inline_count_params(body, callee, count);

	/* Check that every argument can be substituted exactly once. */
	for (i = 0; i < e->val.call.arg_count; i++) {
		if (count[i] != 1)
			return true;
		if (!inline_pure_expr(e->val.call.arg[i]))
			return true;
	}

	replacement = inline_clone_expr(body, callee, e->val.call.arg);
	if (replacement == NULL)
		return false;

	*ep = replacement;

	return inline_rewrite_expr(ep, caller, depth + 1);
}

static bool
inline_rewrite_expr(
	struct hir_expr **ep,
	struct hir_block *caller,
	int depth)
{
	struct hir_expr *e;
	uint32_t i;

	if (ep == NULL ||
	    *ep == NULL)
		return true;
	if (depth >= INLINE_DEPTH_MAX)
		return true;

	e = *ep;

	/* Rewrite the children selected by this expression shape. */
	switch (e->type) {
	case HIR_EXPR_TERM:
		return true;
	case HIR_EXPR_PAR:
	case HIR_EXPR_NEG:
	case HIR_EXPR_NOT:
	case HIR_EXPR_PBASE:
	case HIR_EXPR_PLEN:
		return inline_rewrite_expr(&e->val.unary.expr, caller, depth);
	case HIR_EXPR_CAPTURE:
		return inline_rewrite_expr(&e->val.capture.expr, caller, depth);
	case HIR_EXPR_DOT:
		return inline_rewrite_expr(&e->val.dot.obj, caller, depth);
	case HIR_EXPR_CALL:
		if (!inline_rewrite_expr(&e->val.call.func, caller, depth))
			return false;

		/* Rewrite every ordinary call argument. */
		for (i = 0; i < e->val.call.arg_count; i++) {
			if (!inline_rewrite_expr(
				    &e->val.call.arg[i],
				    caller,
				    depth)) {
				return false;
			}
		}

		return inline_try_call(ep, caller, depth);
	case HIR_EXPR_THISCALL:
		if (!inline_rewrite_expr(&e->val.thiscall.obj, caller, depth))
			return false;

		/* Rewrite every method-call argument. */
		for (i = 0; i < e->val.thiscall.arg_count; i++) {
			if (!inline_rewrite_expr(
				    &e->val.thiscall.arg[i],
				    caller,
				    depth)) {
				return false;
			}
		}

		return true;
	case HIR_EXPR_ARRAY:

		/* Rewrite every array element. */
		for (i = 0; i < e->val.array.elem_count; i++) {
			if (!inline_rewrite_expr(
				    &e->val.array.elem[i],
				    caller,
				    depth)) {
				return false;
			}
		}

		return true;
	case HIR_EXPR_DICT:

		/* Rewrite every dictionary value. */
		for (i = 0; i < e->val.dict.kv_count; i++) {
			if (!inline_rewrite_expr(
				    &e->val.dict.value[i],
				    caller,
				    depth)) {
				return false;
			}
		}

		return true;
	case HIR_EXPR_NEW:
		return inline_rewrite_expr(&e->val.new_.init, caller, depth);
	default:
		if (!inline_rewrite_expr(
			    &e->val.binary.expr[0],
			    caller,
			    depth)) {
			return false;
		}
		if (!inline_rewrite_expr(
			    &e->val.binary.expr[1],
			    caller,
			    depth)) {
			return false;
		}

		return true;
	}
}

static bool
inline_rewrite_chain(
	struct hir_block *head,
	struct hir_block *caller)
{
	struct hir_block *b;
	struct hir_block *c;
	struct hir_stmt *s;

	/* Rewrite every reachable block in the sibling chain. */
	for (b = head;
	     b != NULL;
	     b = b->succ) {

		/* Rewrite the expressions owned by this block shape. */
		switch (b->type) {
		case HIR_BLOCK_BASIC:

			/* Rewrite every statement in the basic block. */
			for (s = b->val.basic.stmt_list;
			     s != NULL;
			     s = s->next) {
				if (!inline_rewrite_expr(&s->lhs, caller, 0))
					return false;
				if (!inline_rewrite_expr(&s->rhs, caller, 0))
					return false;
			}
			break;
		case HIR_BLOCK_IF:

			/* Rewrite every arm in the conditional chain. */
			for (c = b;
			     c != NULL;
			     c = c->val.if_.chain_next) {
				if (!inline_rewrite_expr(&c->val.if_.cond, caller, 0))
					return false;
				if (!inline_rewrite_chain(c->val.if_.inner, caller))
					return false;
			}
			break;
		case HIR_BLOCK_FOR:
			if (!inline_rewrite_expr(&b->val.for_.start, caller, 0))
				return false;
			if (!inline_rewrite_expr(&b->val.for_.stop, caller, 0))
				return false;
			if (!inline_rewrite_expr(&b->val.for_.collection, caller, 0))
				return false;
			if (!inline_rewrite_chain(b->val.for_.inner, caller))
				return false;
			break;
		case HIR_BLOCK_WHILE:
			if (!inline_rewrite_expr(&b->val.while_.cond, caller, 0))
				return false;
			if (!inline_rewrite_chain(b->val.while_.inner, caller))
				return false;
			break;
		default:
			break;
		}

		if (b->stop)
			break;
	}

	return true;
}
