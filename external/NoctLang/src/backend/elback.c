/* -*- coding: utf-8; tab-width: 8; indent-tabs-mode: t; -*- */

/*
 * Copyright (c) 2025, Awe Morris. All rights reserved.
 */

/*
 * elispback: GNU Emacs Lisp translation backend
 */

#include "elback.h"
#include "hir.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <assert.h>

/* False assertions. */
#define NEVER_COME_HERE		0
#define NOT_IMPLEMENTED		0

/* Allocation size. */
#define ALLOC_SIZE		(8192)

/*
 * String writers.
 */

#define PUT(s)		if (!elback_put(s)) return false
#define PUT1(s, x)	if (!elback_put(s, x)) return false
#define PUT2(s, x, y)	if (!elback_put(s, x, y)) return false
#define PUT_INDENT()	if (!elback_put_indent()) return false

/*
 * Translation context.
 */

static FILE *fp;
static int indent;

#if 0
static char *body;
static size_t body_len;
static size_t body_alloc_size;
#endif

/*
 * Error position and message.
 */

static char *elback_file_name;
static int elback_error_line;
static char elback_error_message[65536];

/*
 * Forward declaration
 */

static bool is_parameter_name(struct hir_block *func, const char *symbol);
static bool elback_visit_block(struct hir_block *block);
static bool elback_visit_basic_block(struct hir_block *block);
static bool elback_visit_if_block(struct hir_block *block);
static bool elback_visit_for_block(struct hir_block *block);
static bool elback_visit_for_range_block(struct hir_block *block);
static bool elback_visit_for_kv_block(struct hir_block *block);
static bool elback_visit_for_v_block(struct hir_block *block);
static bool elback_visit_while_block(struct hir_block *block);
static bool elback_visit_stmt(struct hir_stmt *stmt);
static bool elback_visit_expr(struct hir_expr *expr);
static bool elback_visit_unary_expr(struct hir_expr *expr);
static bool elback_visit_binary_expr(struct hir_expr *expr);
static bool elback_visit_dot_expr(struct hir_expr *expr);
static bool elback_visit_call_expr(struct hir_expr *expr);
static bool elback_visit_thiscall_expr(struct hir_expr *expr);
static bool elback_visit_array_expr(struct hir_expr *expr);
static bool elback_visit_dict_expr(struct hir_expr *expr);
static bool elback_visit_new_expr(struct hir_expr *expr);
static bool elback_visit_term(struct hir_term *term);
static bool elback_put(const char *format, ...);
static bool elback_put_indent(void);
static void elback_fatal(const char *msg, ...);
static void elback_out_of_memory(void);

/*
 * Clear translator states.
 */
bool
elback_init(const char *fname)
{
	indent = 0;

	fp = fopen(fname, "w");
	if (fp == NULL) {
		elback_fatal(N_TR("Cannot open file %s."));
		return false;
	}
	return true;
#if 0
	if (body != NULL) {
		free(body);
		body = NULL;
	}

	body = malloc(ALLOC_SIZE);
	if (body == NULL) {
		elback_out_of_memory();
		return false;
	}
	body_len = 0;
	body_alloc_size = ALLOC_SIZE;

	return true;
#endif
}

/*
 * Translate a HIR function to C.
 */
bool
elback_translate_func(
	struct hir_block *func)
{
	struct hir_block *cur_block;
	struct hir_local *local;
	int i;

	/* Copy the file name. */
	elback_file_name = strdup(func->val.func.file_name);
	if (elback_file_name == NULL) {
		elback_out_of_memory();
		return false;
	}

	/* Open defun. */
	elback_put("(defun %s (", func->val.func.name);

	/* Put the parameter list. */
	for (i = 0; i < func->val.func.param_count; i++) {
		PUT1("%s", func->val.func.param_name[i]);
		if (i != func->val.func.param_count - 1)
			PUT(" ");
	}
	PUT(")\n");

	/* Open the local variable list. */
	indent++;
	PUT_INDENT();
	elback_put("(let (");
	local = func->val.func.local;
	while (local != NULL) {
		if (!is_parameter_name(func, local->symbol)) {
			if (local != func->val.func.local)
				PUT(" ");
			elback_put("(%s 0)", local->symbol);
		}
		local = local->next;
	}
	elback_put(")\n");

	/* Open the catch. */
	indent++;
	PUT_INDENT();
	PUT("(catch \'early-return\n");

	/* Put the code. */
	indent++;
	cur_block = func->val.func.inner;
	while (cur_block != NULL) {
		/* Visit a block. */
		elback_visit_block(cur_block);

		/* If this is the final block, break. */
		if (cur_block->stop)
			break;

		/* Move to a next. */
		cur_block = cur_block->succ;
	}

	/* Close the catch. */
	PUT("\n");
	PUT_INDENT();
	PUT("0\n");
	indent--;
	PUT_INDENT();
	PUT(")\n");
	indent--;	

	/* Close the local variable list.*/
	if (func->val.func.local != NULL) {
		PUT_INDENT();
		PUT(")\n");
		indent--;
	}

	/* Close defun. */
	elback_put(")\n");

	return true;
}

static bool is_parameter_name(struct hir_block *func, const char *symbol)
{
	int i;

	for (i = 0; i < func->val.func.param_count; i++) {
		if (strcmp(func->val.func.param_name[i], symbol) == 0)
			return true;
	}
	return false;
}

/* Visit a block. */
static bool
elback_visit_block(
	struct hir_block *block)
{
	assert(block != NULL);

	elback_error_line = block->line;

	switch (block->type) {
	case HIR_BLOCK_BASIC:
		if (!elback_visit_basic_block(block))
			return false;
		break;
	case HIR_BLOCK_IF:
		if (!elback_visit_if_block(block))
			return false;
		break;
	case HIR_BLOCK_FOR:
		if (!elback_visit_for_block(block))
			return false;
		break;
	case HIR_BLOCK_WHILE:
		if (!elback_visit_while_block(block))
			return false;
		break;
	case HIR_BLOCK_END:
		return true;
	default:
		assert(NEVER_COME_HERE);
		break;
	}

	return true;
}

/* Visit a basic block. */
static bool
elback_visit_basic_block(
	struct hir_block *block)
{
	struct hir_stmt *stmt;

	assert(block != NULL);
	assert(block->type == HIR_BLOCK_BASIC);

	/* Visit statements. */
	stmt = block->val.basic.stmt_list;
	while (stmt != NULL) {
		/* Visit a statement. */
		if (!elback_visit_stmt(stmt))
			return false;
		stmt = stmt->next;
	}

	return true;
}

/* Visit an if block. */
static bool
elback_visit_if_block(
	struct hir_block *block)
{
	struct hir_block *b;

	assert(block != NULL);
	assert(block->type == HIR_BLOCK_IF);

	PUT_INDENT();
	PUT("(if ");
	if (!elback_visit_expr(block->val.if_.cond))
		return false;

	/* Visit an inner block. */
	PUT("\n");
	indent++;
	PUT_INDENT();
	PUT("(progn\n");
	indent++;
	b = block->val.if_.inner;
	while (b != NULL) {
		if (!elback_visit_block(b))
			return false;
		if (b->stop)
			break;
		b = b->succ;
	}
	indent--;
	PUT_INDENT();
	PUT(")\n");

	/* Visit an else block if exitst. */
	if (block->val.if_.chain_next != NULL) {
		/* If the else block doesn't have a condition. */
		if (block->val.if_.chain_next->val.if_.cond == NULL) {
			PUT_INDENT();
			PUT("(progn\n");
			indent++;
			b = block->val.if_.chain_next->val.if_.inner;
			while (b != NULL) {
				if (!elback_visit_block(b))
					return false;
				if (b->stop)
					break;
				b = b->succ;
			}
			indent--;
			PUT_INDENT();
			PUT(")\n");
		} else {
			/* The else block has a if condition. */
			if (!elback_visit_if_block(block->val.if_.chain_next))
				return false;
		}
	}

	indent--;
	PUT_INDENT();
	PUT(")\n");

	return true;
}

/* Visit a "for" block. */
static bool
elback_visit_for_block(
	struct hir_block *block)
{
	assert(block != NULL);
	assert(block->type == HIR_BLOCK_FOR);

	/* Dispatch by type. */
	if (block->val.for_.is_ranged) {
		/* This is a ranged-for loop. */
		if (!elback_visit_for_range_block(block))
			return false;
	} else if (block->val.for_.key_symbol != NULL) {
		/* This is a for-each-key-and-value loop. */
		if (!elback_visit_for_kv_block(block))
			return false;
	} else {
		/* This is a for-each-value loop. */
		if (!elback_visit_for_v_block(block))
			return false;
	}

	return true;
}

/* Visit a ranged-for block. (e.g., "for (i in 0..10)" */
static bool
elback_visit_for_range_block(
	struct hir_block *block)
{
	struct hir_block *b;

	assert(block != NULL);
	assert(block->type == HIR_BLOCK_FOR);
	assert(block->val.for_.is_ranged);
	assert(block->val.for_.counter_symbol);
	assert(block->val.for_.start);
	assert(block->val.for_.stop);

	PUT_INDENT();
	PUT1("(let ((%s ", block->val.for_.counter_symbol);

	/* Visit the start expr. */
	if (!elback_visit_expr(block->val.for_.start))
		return false;

	PUT("))\n");
	indent++;
	PUT_INDENT();
	PUT1("(while (< %s ", block->val.for_.counter_symbol);

	/* Visit the stop expr. */
	if (!elback_visit_expr(block->val.for_.stop))
		return false;

	PUT(")\n");
	indent++;
	PUT_INDENT();
	PUT("(progn\n");
	indent++;

	/* Visit an inner block. */
	b = block->val.for_.inner;
	while (b != NULL) {
		if (!elback_visit_block(b))
			return false;
		if (b->stop)
			break;
		b = b->succ;
	}

	/* Increment the loop variable. */
	PUT_INDENT();
	PUT2("(setq %s (+ 1 %s))\n", block->val.for_.counter_symbol, block->val.for_.counter_symbol);

	indent--;
	PUT_INDENT();
	PUT(")\n");
	indent--;
	PUT_INDENT();
	PUT(")\n");
	indent--;
	PUT_INDENT();
	PUT(")\n");

	return true;
}

/* Visit a for-key-value block. */
static bool
elback_visit_for_kv_block(
	struct hir_block *block)
{
	struct hir_block *b;

	assert(block != NULL);
	assert(block->type == HIR_BLOCK_FOR);
	assert(!block->val.for_.is_ranged);
	assert(block->val.for_.key_symbol != NULL);
	assert(block->val.for_.value_symbol != NULL);
	assert(block->val.for_.collection != NULL);

	PUT_INDENT();
	PUT("(dolist (pair ");

	/* Visit a collection expr. */
	if (!elback_visit_expr(block->val.for_.collection))
		return false;

	PUT("))\n");
	indent++;
	PUT_INDENT();
	PUT1("(let ((%s (car pair))\n", block->val.for_.key_symbol);
	PUT_INDENT();
	PUT1("      (%s (cdr pair)))\n", block->val.for_.value_symbol);
	indent++;

	/* Visit an inner block. */
	b = block->val.for_.inner;
	while (b != NULL) {
		if (!elback_visit_block(b))
			return false;
		if (b->stop)
			break;
		b = b->succ;
	}

	indent--;
	PUT_INDENT();
	PUT(")\n");
	indent--;
	PUT_INDENT();
	PUT(")\n");

	return true;
}

/* Visit a for-value block. */
static bool
elback_visit_for_v_block(
	struct hir_block *block)
{
	struct hir_block *b;

	assert(block != NULL);
	assert(block->type == HIR_BLOCK_FOR);
	assert(!block->val.for_.is_ranged);
	assert(block->val.for_.value_symbol != NULL);
	assert(block->val.for_.collection != NULL);

	PUT_INDENT();
	PUT1("(dolist (%s ", block->val.for_.value_symbol);

	/* Visit a collection expr. */
	if (!elback_visit_expr(block->val.for_.collection))
		return false;

	PUT(")\n");
	indent++;
	PUT_INDENT();
	PUT("(progn\n");
	indent++;

	/* Visit an inner block. */
	b = block->val.for_.inner;
	while (b != NULL) {
		if (!elback_visit_block(b))
			return false;
		if (b->stop)
			break;
		b = b->succ;
	}
	PUT("\n");
	indent--;
	PUT_INDENT();
	PUT(")\n");
	indent--;
	PUT_INDENT();
	PUT(")\n");

	return true;
}

/* Visit a "while" block. */
static bool
elback_visit_while_block(
	struct hir_block *block)
{
	struct hir_block *b;

	assert(block != NULL);
	assert(block->type == HIR_BLOCK_WHILE);

	PUT_INDENT();
	PUT("(while ");

	/* Put a condition. */
	if (!elback_visit_expr(block->val.while_.cond))
		return false;
	indent++;
	PUT("\n");
	PUT_INDENT();
	PUT("(progn\n");
	indent++;
	
	/* Visit an inner block. */
	b = block->val.while_.inner;
	while (b != NULL) {
		if (!elback_visit_block(b))
			return false;
		if (b->stop)
			break;
		b = b->succ;
	}

	indent--;
	PUT_INDENT();
	PUT(")\n");
	indent--;
	PUT_INDENT();
	PUT(")\n");

	return true;
}

/* Visit a statement. */
static bool
elback_visit_stmt(
	struct hir_stmt *stmt)
{
	assert(stmt != NULL);
	assert(stmt->rhs != NULL);

	/* Visit LHS. */
	if (stmt->lhs != NULL) {
		if (stmt->lhs->type == HIR_EXPR_TERM) {
			assert(stmt->lhs->val.term.term->type == HIR_TERM_SYMBOL);

			PUT_INDENT();
			if (strcmp(stmt->lhs->val.term.term->val.symbol, "$return") == 0) {
				/* Put a throw for a return. */
				PUT("(throw \'early-return ");
				if (!elback_visit_expr(stmt->rhs))
					return false;
				elback_put(")\n");
			} else{
				/* Put an assignment. */
				PUT1("(setq %s ", stmt->lhs->val.term.term->val.symbol);
				if (!elback_visit_expr(stmt->rhs))
					return false;
				elback_put(")\n");
			}
		} else if (stmt->lhs->type == HIR_EXPR_SUBSCR) {
			PUT_INDENT();
			PUT("(noct-array-assign ");
			if (!elback_visit_expr(stmt->lhs->val.binary.expr[0]))
				return false;
			PUT(" ");
			if (!elback_visit_expr(stmt->lhs->val.binary.expr[1]))
				return false;
			PUT(" ");
			if (!elback_visit_expr(stmt->rhs))
				return false;
			PUT(")");
		} else if (stmt->lhs->type == HIR_EXPR_DOT) {
			PUT_INDENT();
			PUT("(noct-dot-assign ");
			if (!elback_visit_expr(stmt->lhs->val.dot.obj))
				return false;
			PUT1(" %s ", stmt->lhs->val.dot.symbol);
			if (!elback_visit_expr(stmt->rhs))
				return false;
			PUT(")");
		} else {
			elback_fatal(N_TR("LHS is not a symbol or an array element."));
			return false;
		}
	} else {
		PUT_INDENT();
		if (!elback_visit_expr(stmt->rhs))
			return false;
	}

	return true;
}

/* Visit an expression. */
static bool
elback_visit_expr(
	struct hir_expr *expr)
{
	assert(expr != NULL);

	switch (expr->type) {
	case HIR_EXPR_TERM:
		/* Visit a term inside the expr. */
		if (!elback_visit_term(expr->val.term.term))
			return false;
		break;
	case HIR_EXPR_PAR:
		assert(NOT_IMPLEMENTED);
		/* Visit an expr inside the expr. */
		PUT("(");
		if (!elback_visit_expr(expr->val.unary.expr))
			return false;
		PUT(")");
		break;
	case HIR_EXPR_NEG:
		/* For the unary operator. (Currently NEG only) */
		if (!elback_visit_unary_expr(expr))
			return false;
		break;
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
	case HIR_EXPR_SUBSCR:
		/* For the binary operators. */
		if (!elback_visit_binary_expr(expr))
			return false;
		break;
	case HIR_EXPR_DOT:
		/* For the dot operator. */
		if (!elback_visit_dot_expr(expr))
			return false;
		break;
	case HIR_EXPR_CALL:
		/* For a function call. */
		if (!elback_visit_call_expr(expr))
			return false;
		break;
	case HIR_EXPR_THISCALL:
		/* For a method call. */
		if (!elback_visit_thiscall_expr(expr))
			return false;
		break;
	case HIR_EXPR_ARRAY:
		/* For an array expression. */
		if (!elback_visit_array_expr(expr))
			return false;
		break;
	case HIR_EXPR_DICT:
		/* For a dictionary expression. */
		if (!elback_visit_dict_expr(expr))
			return false;
		break;
	case HIR_EXPR_NEW:
		/* For a new expression. */
		if (!elback_visit_new_expr(expr))
			return false;
		break;
	default:
		assert(NEVER_COME_HERE);
		abort();
		break;
	}

	return true;
}

/* Visit an unary expression. */
static bool
elback_visit_unary_expr(
	struct hir_expr *expr)
{
	int opr_tmpvar;

	assert(expr != NULL);
	assert(expr->type == HIR_EXPR_NEG);

	switch (expr->type) {
	case HIR_EXPR_NEG:
		PUT("(- ");
		if (!elback_visit_expr(expr->val.unary.expr))
			return false;
		PUT(")");
		break;
	default:
		assert(NEVER_COME_HERE);
		break;
	}

	return true;
}

/* Visit a binary expression. */
static bool
elback_visit_binary_expr(
	struct hir_expr *expr)
{
	assert(expr != NULL);

	/* Put the operator. */
	switch (expr->type) {
	case HIR_EXPR_LT:
		PUT("(< ");
		break;
	case HIR_EXPR_LTE:
		PUT("(<= ");
		break;
	case HIR_EXPR_EQ:
		PUT("(= ");
		break;
	case HIR_EXPR_NEQ:
		PUT("(not (= ");
		break;
	case HIR_EXPR_GTE:
		PUT("(>= ");
		break;
	case HIR_EXPR_GT:
		PUT("(> ");
		break;
	case HIR_EXPR_PLUS:
		PUT("(+ ");
		break;
	case HIR_EXPR_MINUS:
		PUT("(- ");
		break;
	case HIR_EXPR_MUL:
		PUT("(* ");
		break;
	case HIR_EXPR_DIV:
		PUT("(/ ");
		break;
	case HIR_EXPR_MOD:
		PUT("(% ");
		break;
	case HIR_EXPR_AND:
		PUT("(and ");
		break;
	case HIR_EXPR_OR:
		PUT("(or ");
		break;
	case HIR_EXPR_SUBSCR:
		PUT("(noct-array ");
		break;
	default:
		assert(NEVER_COME_HERE);
		break;
	}

	/* Put the two operands. */
	if (!elback_visit_expr(expr->val.binary.expr[0]))
		return false;
	PUT(" ");
	if (!elback_visit_expr(expr->val.binary.expr[1]))
		return false;

	/* Add an extra ) for a not. */
	if (expr->type == HIR_EXPR_NEQ)
		PUT(")");

	/* Close. */
	PUT(")");

	return true;
}

/* Visit a dot expression. */
static bool
elback_visit_dot_expr(
	struct hir_expr *expr)
{
	assert(expr != NULL);
	assert(expr->type == HIR_EXPR_DOT);
	assert(expr->val.dot.obj != NULL);
	assert(expr->val.dot.symbol != NULL);

	PUT("(noct-dot ");
	if (!elback_visit_expr(expr->val.dot.obj))
		return false;
	PUT1(" %s)", expr->val.dot.symbol);

	return true;
}

/* Visit a call expression. */
static bool
elback_visit_call_expr(
	struct hir_expr *expr)
{
	int arg_count, i;

	assert(expr != NULL);
	assert(expr->type == HIR_EXPR_CALL);
	assert(expr->val.call.func != NULL);
	assert(expr->val.call.arg_count >= 0);
	assert(expr->val.call.arg_count < HIR_PARAM_SIZE);

	arg_count = expr->val.call.arg_count;
	
	/* Put (func. */
	PUT("(");
	if (!elback_visit_expr(expr->val.call.func))
		return false;

	/* Put the arguments. */
	for (i = 0; i < arg_count; i++) {
		PUT(" ");
		if (!elback_visit_expr(expr->val.call.arg[i]))
			return false;
	}

	/* Close. */
	PUT(")");

	return true;
}

/* Visit a thiscall expression. */
static bool
elback_visit_thiscall_expr(
	struct hir_expr *expr)
{
	int arg_count, i;

	assert(expr != NULL);
	assert(expr->type == HIR_EXPR_THISCALL);
	assert(expr->val.thiscall.func != NULL);
	assert(expr->val.thiscall.arg_count >= 0);
	assert(expr->val.thiscall.arg_count < HIR_PARAM_SIZE);

	arg_count = expr->val.thiscall.arg_count;
	
	/* Put "((noct-dot obj func)". */
	PUT("((noct-dot ");
	if (!elback_visit_expr(expr->val.thiscall.obj))
		return false;
	PUT1(" %s)", expr->val.thiscall.func);

	/* Put the arguments. */
	for (i = 0; i < arg_count; i++) {
		PUT(" ");
		if (!elback_visit_expr(expr->val.thiscall.arg[i]))
			return false;
	}

	/* Close. */
	PUT(")");

	return true;
}

/* Visit an array expression. (e.g., "[1 2 3]") */
static bool
elback_visit_array_expr(
	struct hir_expr *expr)
{
	int elem_count, i;

	assert(expr != NULL);
	assert(expr->type == HIR_EXPR_ARRAY);
	assert(expr->val.array.elem_count > 0);

	elem_count = expr->val.array.elem_count;
	
	PUT("\'(");

	/* Put the elements. */
	for (i = 0; i < elem_count; i++) {
		/* Visit the element. */
		if (!elback_visit_expr(expr->val.array.elem[i]))
			return false;
		if (i != elem_count - 1)
			PUT(" ");
	}

	PUT(")");

	return true;
}

/* Visit a dictionary expression. (e.g., "{key: value}") */
static bool
elback_visit_dict_expr(
	struct hir_expr *expr)
{
	int kv_count, i;

	assert(expr != NULL);
	assert(expr->type == HIR_EXPR_DICT);
	assert(expr->val.dict.kv_count > 0);

	kv_count = expr->val.dict.kv_count;
	
	PUT("\'(");

	/* Put the elements. */
	for (i = 0; i < kv_count; i++) {
		PUT1("(%s . ", expr->val.dict.key[i]);

		/* Visit the element. */
		if (!elback_visit_expr(expr->val.dict.value[i]))
			return false;

		PUT(")");
		if (i != kv_count - 1)
			PUT(" ");
	}

	return true;
}

/* Visit a new expression. */
static bool
elback_visit_new_expr(
	struct hir_expr *expr)
{
	assert(expr != NULL);
	assert(expr->type == HIR_EXPR_NEW);

	PUT1("(noct-new %s \'(", expr->val.new_.cls);

	/* Put the elements. */
	if (expr->val.new_.init != NULL) {
		int i, kv_count = expr->val.new_.init->val.dict.kv_count;
		for (i = 0; i < kv_count; i++) {
			PUT1("(%s . ", expr->val.new_.init->val.dict.key[i]);

			/* Visit the element. */
			if (!elback_visit_expr(expr->val.new_.init->val.dict.value[i]))
				return false;

			PUT(")");
			if (i != kv_count - 1)
				PUT(" ");
		}
	}

	PUT(")");

	return true;
}

/* Visit a term. */
static bool
elback_visit_term(
	struct hir_term *term)
{
	assert(term != NULL);

	switch (term->type) {
	case HIR_TERM_SYMBOL:
		if (!elback_put(term->val.symbol))
			return false;
		break;
	case HIR_TERM_INT:
		if (!elback_put("%d", term->val.i))
			return false;
		break;
	case HIR_TERM_FLOAT:
		if (!elback_put("%f", term->val.f))
			return false;
		break;
	case HIR_TERM_STRING:
		if (!elback_put("\"%s\"", term->val.s))
			return false;
		break;
	case HIR_TERM_EMPTY_ARRAY:
		if (!elback_put("'()"))
			return false;
		break;
	case HIR_TERM_EMPTY_DICT:
		if (!elback_put("'()"))
			return false;
		break;
	default:
		assert(NEVER_COME_HERE);
		break;
	}

	return true;

}

/*
 * Finalize the file output.
 */
bool
elback_finalize(void)
{
	fclose(fp);
	fp = NULL;
	return true;
}

/* Put a translated string. */
static bool
elback_put(const char *format, ...)
{
	va_list ap;

	va_start(ap, format);
	vfprintf(fp, format, ap);
	va_end(ap);

	return true;

#if 0
	char buf[8192];
	va_list ap;
	size_t add_len, rem_len;

	va_start(ap, format);
	vsnprintf(buf, sizeof(buf), format, ap);
	va_end(ap);

	add_len = strlen(buf);
	rem_len = body_alloc_size - body_len - 1;
	if (add_len > rem_len) {
		body = realloc(buf, body_alloc_size + ALLOC_SIZE);
		if (body == NULL) {
			elback_out_of_memory();
			return false;
		}
		body_alloc_size += ALLOC_SIZE;
	}

	strcat(body, buf);
	body_len += add_len;

	return true;
#endif
}

/* Put a translated string. */
static bool
elback_put_indent(void)
{
	int i;

	for (i = 0; i < indent; i++)
		fprintf(fp, "  ");

	return true;
}


/* Set an error message. */
static void
elback_fatal(
	const char *msg,
	...)
{
	va_list ap;

	va_start(ap, msg);
	vsnprintf(elback_error_message,
		  sizeof(elback_error_message),
		  msg,
		  ap);
	va_end(ap);
}

/* Out of memory. */
static void
elback_out_of_memory(void)
{
	printf("Out of memory.\n");
}
