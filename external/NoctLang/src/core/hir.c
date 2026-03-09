/* -*- coding: utf-8; tab-width: 8; indent-tabs-mode: t; -*- */

/*
 * Copyright (c) 2025, Awe Morris. All rights reserved.
 */

/*
 * HIR: High-level intermediate representation
 */

#include "hir.h"
#include "ast.h"
#include "arena.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <assert.h>

/* False assertions. */
#define NEVER_COME_HERE		(0)
#define UNIMPLEMENTED		(0)

/* Debug dump */
#undef DEBUG_DUMP

/* Arena allocator size. */
#define ARENA_SIZE		(4 * 1024 * 1024)

/* List-add function. */
#define HIR_ADD_TO_LAST(type, list, p)			\
	do {						\
		if (list == NULL) {			\
			list = p;			\
		} else {				\
			type *elem = list;		\
			while (elem->next)		\
				elem = elem->next;	\
			elem->next = p;			\
		}					\
	} while (0);

/*
 * Constructed HIR.
 */

#define HIR_FUNC_MAX	128

char *hir_file_name;
int hir_func_count;
struct hir_block *hir_func_tbl[HIR_FUNC_MAX];

/*
 * Error position and message.
 */

static int hir_error_line;
static char hir_error_message[1024];

/*
 * Block id top.
 */
static int block_id_top;

/*
 * Anonymous functions.
 */

#define ANON_FUNC_SIZE	256

static int hir_anon_func_count;
static char *hir_anon_func_name[ANON_FUNC_SIZE];
static struct ast_param_list *hir_anon_func_param_list[ANON_FUNC_SIZE];
static struct ast_stmt_list *hir_anon_func_stmt_list[ANON_FUNC_SIZE];

/*
 * Arena allocator.
 */
static struct arena_info hir_arena;

/*
 * Forward Declaration
 */
static bool hir_visit_func(struct ast_func *afunc);
static bool hir_visit_stmt_list(struct hir_block **cur_block, struct hir_block **prev_block, struct hir_block *parent_block, struct ast_stmt_list *stmt_list);
static bool hir_visit_stmt(struct hir_block **cur_block, struct hir_block **prev_block, struct hir_block *parent_block, struct ast_stmt *cur_astmt);
static bool hir_visit_expr_stmt(struct hir_block **cur_block, struct hir_block **prev_block, struct hir_block *parent_block, struct ast_stmt *cur_astmt);
static bool hir_visit_assign_stmt(struct hir_block **cur_block, struct hir_block **prev_block, struct hir_block *parent_block, struct ast_stmt *cur_astmt);
static bool hir_add_local(struct hir_block *cur_block, const char *symbol);
static bool hir_visit_if_stmt(struct hir_block **cur_block, struct hir_block **prev_block, struct hir_block *parent_block, struct ast_stmt *cur_astmt);
static bool hir_visit_elif_stmt(struct hir_block **cur_block, struct hir_block **prev_block, struct hir_block *parent_block, struct ast_stmt *cur_astmt);
static bool hir_visit_else_stmt(struct hir_block **cur_block, struct hir_block **prev_block, struct hir_block *parent_block, struct ast_stmt *cur_astmt);
static bool hir_visit_while_stmt(struct hir_block **cur_block, struct hir_block **prev_block, struct hir_block *parent_block, struct ast_stmt *cur_astmt);
static bool hir_visit_for_stmt(struct hir_block **cur_block, struct hir_block **prev_block, struct hir_block *parent_block, struct ast_stmt *cur_astmt);
static bool hir_visit_return_stmt(struct hir_block **cur_block, struct hir_block **prev_block, struct hir_block *parent_block, struct ast_stmt *cur_astmt);
static bool hir_visit_expr(struct hir_expr **hexpr, struct ast_expr *aexpr);
static bool hir_visit_term_expr(struct hir_expr **hexpr, struct ast_expr *aexpr);
static bool hir_visit_binary_expr(struct hir_expr **hexpr, struct ast_expr *aexpr, int type);
static bool hir_visit_unary_expr(struct hir_expr **hexpr, struct ast_expr *aexpr, int type);
static bool hir_visit_dot_expr(struct hir_expr **hexpr, struct ast_expr *aexpr);
static bool hir_visit_call_expr(struct hir_expr **hexpr, struct ast_expr *aexpr);
static bool hir_visit_thiscall_expr(struct hir_expr **hexpr, struct ast_expr *aexpr);
static bool hir_visit_array_expr(struct hir_expr **hexpr, struct ast_expr *aexpr);
static bool hir_visit_dict_expr(struct hir_expr **hexpr, struct ast_expr *aexpr);
static bool hir_visit_func_expr(struct hir_expr **hexpr, struct ast_expr *aexpr);
static bool hir_visit_new_expr(struct hir_expr **hexpr, struct ast_expr *aexpr);
static bool hir_visit_term(struct hir_term **hterm, struct ast_term *aterm);
static bool hir_visit_param_list(struct hir_block *hfunc,struct ast_func *afunc);
static bool hir_defer_anon_func(struct ast_expr *aexpr, char **symbol);
static void hir_free_block(struct hir_block *b);
static void hir_free_stmt(struct hir_stmt *s);
static void hir_free_expr(struct hir_expr *e);
static void hir_free_term(struct hir_term *t);
static void hir_free_local(struct hir_local *local);
static void hir_fatal(int line, const char *msg);
static void hir_out_of_memory(void);
static void *hir_malloc(size_t size);
static char *hir_strdup(const char *s);
static void hir_free(void *p);

/*
 * Construct an HIR from an AST.
 */
bool
hir_build(void)
{
	struct ast_func_list *func_list;
	struct ast_func *func;
	int i;

	assert(hir_file_name == NULL);
	assert(hir_func_count == 0);

	/* Initialize the arena allocator. */
	if (!arena_init(&hir_arena, ARENA_SIZE)) {
		hir_out_of_memory();
		return false;
	}

	hir_anon_func_count = 0;

	/* Copy a file name. */
	hir_file_name = hir_strdup(ast_get_file_name());
	if (hir_file_name == NULL) {
		hir_out_of_memory();
		return false;
	}

	hir_anon_func_count = 0;

	/* Copy a file name. */
	hir_file_name = hir_strdup(ast_get_file_name());
	if (hir_file_name == NULL)
		return false;

	/* Construct a HIR func for each AST func: */
	func_list = ast_get_func_list();
	assert(func_list != NULL);
	func = func_list->list;
	while (func != NULL) {
		/* Visit an AST func. */
		if (!hir_visit_func(func))
			return false;

		func = func->next;

		/*
		 * If an anonymous func appears while a visit,
		 * it is queued to the deffered table.
		 */
	}

	/* Construct a HIR func for each deffered anonymous func: */
	for (i = 0; i < hir_anon_func_count; i++) {
		/* Visit an AST func. */
		struct ast_func afunc;
		afunc.name = hir_anon_func_name[i];
		afunc.param_list = hir_anon_func_param_list[i];
		afunc.stmt_list = hir_anon_func_stmt_list[i];
		afunc.next = NULL;
		if (!hir_visit_func(&afunc))
			return false;

		hir_anon_func_name[i] = NULL;
		hir_anon_func_param_list[i] = NULL;
		hir_anon_func_stmt_list[i] = NULL;
	}

	return true;
}

/*
 * Free constructed HIR functions.
 */
void
hir_cleanup(void)
{
	int i;

	if (hir_file_name != NULL) {
		hir_free(hir_file_name);
		hir_file_name = NULL;
	}

	for (i = 0; i < hir_func_count; i++) {
		hir_free_block(hir_func_tbl[i]);
		hir_func_tbl[i] = NULL;
	}

	hir_func_count = 0;
}

/*
 * Get a number of constructed functions.
 */
int
hir_get_function_count(void)
{
	return hir_func_count;
}

/*
 * Get a constructed HIR function.
 */
struct hir_block *
hir_get_function(int index)
{
	struct hir_block *func;

	assert(index >= 0);
	assert(index < hir_func_count);

	func = hir_func_tbl[index];

	return func;
}

/* Visit an AST func. */
static bool
hir_visit_func(
	struct ast_func *afunc)
{
	struct hir_block *func_block;
	struct hir_block *end_block;
	struct hir_block *cur_block;
	struct hir_block *prev_block;

	/* Check maximum functions. */
	if (hir_func_count >= HIR_FUNC_MAX) {
		hir_fatal(0, N_TR("Too many functions."));
		return false;
	}

	/* Alloc a func block. */
	func_block = hir_malloc(sizeof(struct hir_block));
	if (func_block == NULL) {
		hir_out_of_memory();
		return false;
	}
	memset(func_block, 0, sizeof(struct hir_block));
	func_block->id = block_id_top++;
	func_block->type = HIR_BLOCK_FUNC;
	func_block->val.func.file_name = hir_strdup(hir_file_name);
	if (func_block->val.func.file_name == NULL) {
		hir_out_of_memory();
		return false;
	}

	do {
		/* Set a func name. */
		func_block->val.func.name = hir_strdup(afunc->name);
		if (func_block->val.func.name == NULL) {
			hir_out_of_memory();
			break;
		}

		/* Parse the parameters. */
		hir_visit_param_list(func_block, afunc);

		/* Alloc an end block. */
		end_block = hir_malloc(sizeof(struct hir_block));
		if (end_block == NULL) {
			hir_out_of_memory();
			break;
		}
		memset(end_block, 0, sizeof(struct hir_block));
		end_block->id = block_id_top++;
		end_block->type = HIR_BLOCK_END;

		/* Set end_block to the succ of func_block. */
		func_block->succ = end_block;

		/* Visit the stmt_list. */
		if (afunc->stmt_list != NULL) {
			/* Pre-allocate a first inner basic block. */
			func_block->val.func.inner = hir_malloc(sizeof(struct hir_block));
			if (func_block->val.func.inner == NULL) {
				hir_out_of_memory();
				break;
			}
			memset(func_block->val.func.inner, 0, sizeof(struct hir_block));
			func_block->val.func.inner->id = block_id_top++;
			func_block->val.func.inner->type = HIR_BLOCK_BASIC;
			func_block->val.func.inner->parent = func_block;

			/* Visit the stmt_list. */
			cur_block = func_block->val.func.inner;
			prev_block = NULL;
			if (!hir_visit_stmt_list(&cur_block,		/* cur_block */
						 &prev_block,		/* prev_block */
						 func_block,		/* parent_block*/
						 afunc->stmt_list))	/* stmt_list */
				break;

			/* If the first inner block was garbage-collected. */
			if (cur_block == NULL)
				func_block->val.func.inner = NULL;
		}

		/* Store func_block to the table. */
		hir_func_tbl[hir_func_count] = func_block;
		hir_func_count++;

#ifdef DEBUG_DUMP
		hir_dump_block(func_block);
#endif

		/* Succeeded. */
		return true;
	} while (0);

	/* Failed. */
	if (func_block != NULL)
		hir_free_block(func_block);
	return false;
}

/* Visit an AST stmt_list. */
static bool
hir_visit_stmt_list(
	struct hir_block **cur_block,
	struct hir_block **prev_block,
	struct hir_block *parent_block,
	struct ast_stmt_list *stmt_list)
{
	struct hir_block *p_search;
	struct ast_stmt *cur_astmt;
	bool is_control;

	assert(cur_block != NULL);
	assert(prev_block != NULL);
	assert(parent_block != NULL);

	/* Assume we have a first block allocated. */
	assert(*cur_block != NULL);
	assert((*cur_block)->type == HIR_BLOCK_BASIC);

	/* Visit each stmt. */
	cur_astmt = NULL;
	is_control = false;
	if (stmt_list != NULL) {
		assert(*cur_block != NULL);

		cur_astmt = stmt_list->list;
		while (cur_astmt != NULL) {
			/* Break if the astmt is a loop-control statement. */
			if (cur_astmt->type == AST_STMT_CONTINUE ||
			    cur_astmt->type == AST_STMT_BREAK) {
				is_control = true;
				break;
			}

			/* Visit a stmt. */
			if (!hir_visit_stmt(cur_block, prev_block, parent_block, cur_astmt))
				return false;

			assert(*cur_block != NULL);

			/* Break if the astmt is a control statement. */
			if (cur_astmt->type == AST_STMT_RETURN) {
				is_control = true;
				break;
			}

			cur_astmt = cur_astmt->next;
		}
	}

	/* Terminate with a proper succ. */
	if (cur_astmt != NULL && is_control) {
		/* If the control stopped with... */
		assert(cur_astmt != NULL);
		switch (cur_astmt->type) {
		case AST_STMT_CONTINUE:
			/* Find the inner most loop. */
			p_search = parent_block;
			while (p_search != NULL) {
				if (p_search->type == HIR_BLOCK_FOR ||
				    p_search->type == HIR_BLOCK_WHILE)
					break;
				p_search = p_search->parent;
			}
			if (p_search == NULL) {
				hir_fatal(cur_astmt->line, N_TR("continue appeared outside loop."));
				return false;
			}

			/* Continue with the first inner block. */
			if (p_search->type == HIR_BLOCK_FOR) {
				assert(p_search->val.for_.inner != NULL);
				(*cur_block)->succ = p_search->val.for_.inner;
				(*cur_block)->stop = true;
			} else if (p_search->type == HIR_BLOCK_WHILE) {
				assert(p_search->val.while_.inner != NULL);
				(*cur_block)->succ = p_search->val.while_.inner;
				(*cur_block)->stop = true;
			}
			break;
		case AST_STMT_BREAK:
			/* Find the inner most loop. */
			p_search = parent_block;
			while (p_search != NULL) {
				if (p_search->type == HIR_BLOCK_FOR ||
				    p_search->type == HIR_BLOCK_WHILE)
					break;
				p_search = p_search->parent;
			}
			if (p_search == NULL) {
				hir_fatal(cur_astmt->line, N_TR("continue appeared outside loop."));
				return false;
			}

			/* Continue with the block after the loop. */
			(*cur_block)->succ = p_search->succ;
			(*cur_block)->stop = true;
			break;
		case AST_STMT_RETURN:
			/* Search a func block.*/
			p_search = *cur_block;
			do {
				if (p_search->parent != NULL) {
					p_search = p_search->parent;
				} else {
					if (p_search->type == HIR_BLOCK_FUNC)
						break;
					assert(p_search->type == HIR_BLOCK_IF);
					p_search = p_search->val.if_.chain_prev;
				}
			} while (1);
			assert(p_search->succ != NULL);
			assert(p_search->succ->type == HIR_BLOCK_END);

			/* Go to HIR_BLOCK_END. */
			(*cur_block)->succ = p_search->succ;
			(*cur_block)->stop = true;
			break;
		default:
			assert(NEVER_COME_HERE);
			break;
		}
	} else {
		/* If the end of... */
		switch (parent_block->type) {
		case HIR_BLOCK_FUNC:
			/* Search a func block.*/
			p_search = parent_block;
			while (p_search->parent != NULL)
				p_search = p_search->parent;
			assert(p_search->type == HIR_BLOCK_FUNC);
			assert(p_search->succ != NULL);
			assert(p_search->succ->type == HIR_BLOCK_END);

			/* Go to HIR_BLOCK_END. */
			(*cur_block)->succ = p_search->succ;
			(*cur_block)->stop = true;
			break;
		case HIR_BLOCK_IF:
			/* Find the chain-top if block. */
			if (parent_block->succ != NULL) {
				/* Parent is if block */
				p_search = parent_block;
			} else {
				/* Parent is else-if or else block. Use its parent, i.e., if block. */
				p_search = parent_block->parent;
			}

			/* Go to the placeholder block after if block. */
			(*cur_block)->succ = p_search->succ;
			(*cur_block)->stop = true;
			break;
		case HIR_BLOCK_FOR:
			/* Continue to the first inner block. */
			assert(parent_block->val.for_.inner != NULL);
			(*cur_block)->succ = parent_block->val.for_.inner;
			(*cur_block)->stop = true;
			break;
		case HIR_BLOCK_WHILE:
			/* Continue to the first inner block. */
			assert(parent_block->val.while_.inner != NULL);
			(*cur_block)->succ = parent_block->val.while_.inner;
			(*cur_block)->stop = true;
			break;
		default:
			assert(NEVER_COME_HERE);
			break;
		}
	}

	return true;
}

/* Visit an AST stmt. */
static bool
hir_visit_stmt(
	struct hir_block **cur_block,
	struct hir_block **prev_block,
	struct hir_block *parent_block,
	struct ast_stmt *cur_astmt)
{
	bool result;

	assert(cur_block != NULL);
	assert(*cur_block != NULL);
	assert(prev_block != NULL);
	assert(parent_block != NULL);
	assert(cur_astmt != NULL);

	hir_error_line = cur_astmt->line;

	switch (cur_astmt->type) {
	case AST_STMT_EXPR:
		result = hir_visit_expr_stmt(cur_block, prev_block, parent_block, cur_astmt);
		break;
	case AST_STMT_ASSIGN:
		result = hir_visit_assign_stmt(cur_block, prev_block, parent_block, cur_astmt);
		break;
	case AST_STMT_IF:
		result = hir_visit_if_stmt(cur_block, prev_block, parent_block, cur_astmt);
		break;
	case AST_STMT_ELIF:
		result = hir_visit_elif_stmt(cur_block, prev_block, parent_block, cur_astmt);
		break;
	case AST_STMT_ELSE:
		result = hir_visit_else_stmt(cur_block, prev_block, parent_block, cur_astmt);
		break;
	case AST_STMT_WHILE:
		result = hir_visit_while_stmt(cur_block, prev_block, parent_block, cur_astmt);
		break;
	case AST_STMT_FOR:
		result = hir_visit_for_stmt(cur_block, prev_block, parent_block, cur_astmt);
		break;
	case AST_STMT_RETURN:
		result = hir_visit_return_stmt(cur_block, prev_block, parent_block, cur_astmt);
		break;
	default:
		result = false;
		assert(NEVER_COME_HERE);
		break;
	}

	return result;
}

/* Visit an AST expr stmt. */
static bool
hir_visit_expr_stmt(
	struct hir_block **cur_block,
	struct hir_block **prev_block,
	struct hir_block *parent_block,
	struct ast_stmt *cur_astmt)
{
	struct hir_stmt *hstmt;

	assert(cur_block != NULL);
	assert(*cur_block != NULL);
	assert((*cur_block)->type == HIR_BLOCK_BASIC);
	assert(prev_block != NULL);
	assert(parent_block != NULL);
	assert(cur_astmt != NULL);
	assert(cur_astmt->type == AST_STMT_EXPR);

	/* Assume we are on a basic block. */
	assert((*cur_block)->type == HIR_BLOCK_BASIC);

	/* Allocate an hstmt. */
	hstmt = hir_malloc(sizeof(struct hir_stmt));
	if (hstmt == NULL) {
		hir_out_of_memory();
		return false;
	}
	memset(hstmt, 0, sizeof(struct hir_stmt));
	hstmt->line = cur_astmt->line;

	/* There is no LHS for an expr stmt. */
	hstmt->lhs = NULL;

	/* Visit an expr. */
	if (!hir_visit_expr(&hstmt->rhs, cur_astmt->val.expr.expr)) {
		hir_free_stmt(hstmt);
		return false;
	}

	/* Add hstmt to the end of the block. */
	HIR_ADD_TO_LAST(struct hir_stmt, (*cur_block)->val.basic.stmt_list, hstmt);

	/* Set a block line number if this is a first stmt in the block. */
	if ((*cur_block)->val.basic.stmt_list == hstmt)
		(*cur_block)->line = cur_astmt->line;

	/* Continue on the same basic block. */

	return true;
}

/* Visit an assign stmt. */
static bool
hir_visit_assign_stmt(
	struct hir_block **cur_block,
	struct hir_block **prev_block,
	struct hir_block *parent_block,
	struct ast_stmt *cur_astmt)
{
	struct hir_stmt *hstmt;
	bool is_lhs_ok;

	assert(cur_block != NULL);
	assert(*cur_block != NULL);
	assert((*cur_block)->type == HIR_BLOCK_BASIC);
	assert(prev_block != NULL);
	assert(parent_block != NULL);
	assert(cur_astmt != NULL);
	assert(cur_astmt->type == AST_STMT_ASSIGN);

	/* Allocate an hstmt. */
	hstmt = hir_malloc(sizeof(struct hir_stmt));
	if (hstmt == NULL) {
		hir_out_of_memory();
		return false;
	}
	memset(hstmt, 0, sizeof(struct hir_stmt));
	hstmt->line = cur_astmt->line;

	/* Visit LHS. */
	if (!hir_visit_expr(&hstmt->lhs, cur_astmt->val.assign.lhs)) {
		hir_free_stmt(hstmt);
		return false;
	}

	/* Check LHS. */
	is_lhs_ok = false;
	if (hstmt->lhs->type == HIR_EXPR_TERM &&
	    hstmt->lhs->val.term.term->type == HIR_TERM_SYMBOL)
		is_lhs_ok = true;
	else if (hstmt->lhs->type == HIR_EXPR_SUBSCR)
		is_lhs_ok = true;
	else if (hstmt->lhs->type == HIR_EXPR_DOT)
		is_lhs_ok = true;
	if (!is_lhs_ok) {
		hir_fatal(cur_astmt->line, N_TR("LHS is not a term or an array element."));
		hir_free_stmt(hstmt);
		return false;
	}

	/* If this is a "var" assign. */
	if (cur_astmt->val.assign.is_var) {
		if (hstmt->lhs->type == HIR_EXPR_TERM &&
		    hstmt->lhs->val.term.term->type == HIR_TERM_SYMBOL) {
			if (!hir_add_local(*cur_block, hstmt->lhs->val.term.term->val.symbol))
				return false;
		} else {
			hir_fatal(cur_astmt->line, N_TR("var is specified without a single symbol."));
			hir_free_stmt(hstmt);
			return false;
		}
	}

	/* Visit RHS. */
	if (!hir_visit_expr(&hstmt->rhs, cur_astmt->val.assign.rhs)) {
		hir_free_stmt(hstmt);
		return false;
	}

	/* Add hstmt to the end of the block. */
	HIR_ADD_TO_LAST(struct hir_stmt, (*cur_block)->val.basic.stmt_list, hstmt);

	/* Set a block line number if this is a first stmt in the block. */
	if ((*cur_block)->val.basic.stmt_list == hstmt)
		(*cur_block)->line = cur_astmt->line;

	/* Continue on the same basic block. */

	return true;
}

/* Add a local variable entry. */
static bool
hir_add_local(
	struct hir_block *cur_block,
	const char *symbol)
{
	struct hir_block *func;
	struct hir_local *local;
	int index;

	/* Get a root func block. */
	func = cur_block;
	while (func->type != HIR_BLOCK_FUNC)
		func = func->parent;

	/* Search a symbol. */
	index = 0;
	local = func->val.func.local;
	while (local != NULL) {
		/* If already exists. */
		if (strcmp(local->symbol, symbol) == 0)
			return true;
		index++;
		local = local->next;
	}

	/* Add a local variable symbol. */
	local = hir_malloc(sizeof(struct hir_local));
	if (local == NULL) {
		hir_out_of_memory();
		return false;
	}
	local->symbol = hir_strdup(symbol);
	if (local->symbol == NULL) {
		hir_out_of_memory();
		hir_free(local);
	}
	local->index = index;
	local->next = func->val.func.local;
	func->val.func.local = local;

	return true;
}

/* Visit an AST "if" stmt. */
static bool
hir_visit_if_stmt(
	struct hir_block **cur_block,
	struct hir_block **prev_block,
	struct hir_block *parent_block,
	struct ast_stmt *cur_astmt)
{
	struct hir_block *if_block;
	struct hir_block *exit_block;
	struct hir_block *inner_cur_block;
	struct hir_block *inner_prev_block;

	assert(cur_block != NULL);
	assert(*cur_block != NULL);
	assert(prev_block != NULL);
	assert(parent_block != NULL);
	assert(cur_astmt != NULL);
	assert(cur_astmt->type == AST_STMT_IF);

	/* Allocate an if block. */
	if ((*cur_block)->type == HIR_BLOCK_BASIC &&
	    (*cur_block)->val.basic.stmt_list == NULL) {
		/* Reuse an empty basic block. */
		(*cur_block)->type = HIR_BLOCK_IF;
		if_block = *cur_block;
	} else {
		/* Simply allocate. */
		if_block = hir_malloc(sizeof(struct hir_block));
		if (if_block == NULL) {
			hir_out_of_memory();
			return false;
		}
		if_block->type = HIR_BLOCK_IF;
		(*cur_block)->succ = if_block;
	}
	if_block->line = cur_astmt->line;
	if_block->parent = parent_block;
	if_block->val.if_.chain_next = NULL;
	if_block->val.if_.chain_prev = NULL;

	/* Alloc an inner block. */
	if_block->val.if_.inner = hir_malloc(sizeof(struct hir_block));
	if (if_block->val.if_.inner == NULL) {
		hir_out_of_memory();
		return false;
	}
	memset(if_block->val.if_.inner, 0, sizeof(struct hir_block));
	if_block->val.if_.inner->id = block_id_top++;
	if_block->val.if_.inner->type = HIR_BLOCK_BASIC;
	if_block->val.if_.inner->line = cur_astmt->line;
	if_block->val.if_.inner->parent = if_block;

	/* Allocate an exit block. (This may be reused as a basic block.) */
	exit_block = hir_malloc(sizeof(struct hir_block));
	if (exit_block == NULL) {
		hir_out_of_memory();
		return false;
	}
	memset(exit_block, 0, sizeof(struct hir_block));
	exit_block->id = block_id_top++;
	exit_block->type = HIR_BLOCK_BASIC;
	exit_block->succ = parent_block->succ;
	exit_block->parent = parent_block;
	if_block->succ = exit_block;

	/* Visit a cond expr. */
	if (!hir_visit_expr(&if_block->val.if_.cond, cur_astmt->val.if_.cond)) {
		hir_free_block(if_block);
		return false;
	}

	/* Visit an inner stmt_list */
	if (cur_astmt->val.if_.stmt_list != NULL) {
		inner_cur_block = if_block->val.if_.inner;
		inner_prev_block = NULL;
		if (!hir_visit_stmt_list(&inner_cur_block,	/* cur_block */
					 &inner_prev_block,	/* prev_block */
					 if_block,		/* parent_block */
					 cur_astmt->val.if_.stmt_list)) {
			hir_free_block(if_block);
			return false;
		}
	}

	/* Move the cursor to the exit block. */
	*cur_block = exit_block;
	*prev_block = if_block;

	assert((*cur_block)->type != HIR_BLOCK_END);

	return true;
}

/* Visit an AST "else if" stmt. */
static bool
hir_visit_elif_stmt(
	struct hir_block **cur_block,
	struct hir_block **prev_block,
	struct hir_block *parent_block,
	struct ast_stmt *cur_astmt)
{
	struct hir_block *elif_block;
	struct hir_block *inner_cur_block;
	struct hir_block *inner_prev_block;
	struct hir_block *b;

	assert(cur_block != NULL);
	assert(*cur_block != NULL);
	assert(prev_block != NULL);
	assert((*prev_block)->type == HIR_BLOCK_IF);
	assert(parent_block != NULL);
	assert(cur_astmt != NULL);

	/* Check the previous block. */
	if (*prev_block == NULL || (*prev_block)->type != HIR_BLOCK_IF) {
		hir_fatal(cur_astmt->line, N_TR("else-if block appeared without if block."));
		return false;
	}
	if ((*prev_block)->val.if_.cond == NULL) {
		hir_fatal(cur_astmt->line, N_TR("else-if appeared after else."));
		return false;
	}
	assert((*prev_block)->val.if_.chain_next == NULL);

	/* Get the exit block. */
	assert(parent_block->type != HIR_BLOCK_IF);
	assert(parent_block->succ != NULL);

	/* Alloc an else-if block. */
	elif_block = hir_malloc(sizeof(struct hir_block));
	if (elif_block == NULL) {
		hir_out_of_memory();
		return false;
	}
	elif_block->id = block_id_top++;
	elif_block->type = HIR_BLOCK_IF;
	elif_block->succ = NULL;
	elif_block->parent = parent_block;
	elif_block->line = cur_astmt->line;
	elif_block->val.if_.chain_prev = (*prev_block);
	(*prev_block)->val.if_.chain_next = elif_block;

	/* Get a first if-block. */
	b = elif_block->val.if_.chain_prev;
	while (b->val.if_.chain_prev != NULL)
		b = b->val.if_.chain_prev;
	elif_block->parent = b;

	/* Alloc an inner block. */
	elif_block->val.if_.inner = hir_malloc(sizeof(struct hir_block));
	if (elif_block->val.if_.inner == NULL) {
		hir_out_of_memory();
		return false;
	}
	memset(elif_block->val.if_.inner, 0, sizeof(struct hir_block));
	elif_block->val.if_.inner->id = block_id_top++;
	elif_block->val.if_.inner->type = HIR_BLOCK_BASIC;
	elif_block->val.if_.inner->parent = elif_block;
	elif_block->val.if_.inner->line = cur_astmt->line;

	/* Visit a cond expr. */
	if (!hir_visit_expr(&elif_block->val.if_.cond, cur_astmt->val.if_.cond)) {
		hir_free_block(elif_block);
		return false;
	}

	/* Visit an inner stmt_list */
	if (cur_astmt->val.elif.stmt_list != NULL) {
		inner_cur_block = elif_block->val.if_.inner;
		inner_prev_block = NULL;
		if (!hir_visit_stmt_list(&inner_cur_block,	/* cur_block */
					 &inner_prev_block,	/* prev_block */
					 elif_block,		/* parent_block */
					 cur_astmt->val.elif.stmt_list)) {
			hir_free_block(elif_block);
			return false;
		}
	}

	/* Move the cursor to the exit block. */
	*cur_block = elif_block->parent->succ;
	*prev_block = elif_block;

	return true;
}

/* Visit an AST "else" stmt. */
static bool
hir_visit_else_stmt(
	struct hir_block **cur_block,
	struct hir_block **prev_block,
	struct hir_block *parent_block,
	struct ast_stmt *cur_astmt)
{
	struct hir_block *else_block;
	struct hir_block *inner_cur_block;
	struct hir_block *inner_prev_block;
	struct hir_block *b;

	assert(cur_block != NULL);
	assert(*cur_block != NULL);
	assert(prev_block != NULL);
	assert((*prev_block)->type == HIR_BLOCK_IF);
	assert(parent_block != NULL);
	assert(cur_astmt != NULL);

	/* Check the previous block. */
	if (*prev_block == NULL || (*prev_block)->type != HIR_BLOCK_IF) {
		hir_fatal(cur_astmt->line, N_TR("else-if block appeared without if block."));
		return false;
	}
	if ((*prev_block)->val.if_.cond == NULL) {
		hir_fatal(cur_astmt->line, N_TR("else appeared after else."));
		return false;
	}
	assert((*prev_block)->val.if_.chain_next == NULL);

	/* Alloc an else block. */
	else_block = hir_malloc(sizeof(struct hir_block));
	if (else_block == NULL) {
		hir_out_of_memory();
		return false;
	}
	memset(else_block, 0, sizeof(struct hir_block));
	else_block->id = block_id_top++;
	else_block->type = HIR_BLOCK_IF;
	else_block->succ = NULL;
	else_block->parent = parent_block;
	else_block->line = cur_astmt->line;
	else_block->val.if_.chain_next = NULL;
	else_block->val.if_.chain_prev = (*prev_block);
	(*prev_block)->val.if_.chain_next = else_block;

	/* Get a first if-block. */
	b = else_block->val.if_.chain_prev;
	while (b->val.if_.chain_prev != NULL)
		b = b->val.if_.chain_prev;
	else_block->parent = b;

	/* Alloc an inner block. */
	else_block->val.if_.inner = hir_malloc(sizeof(struct hir_block));
	if (else_block->val.if_.inner == NULL) {
		hir_out_of_memory();
		return false;
	}
	memset(else_block->val.if_.inner, 0, sizeof(struct hir_block));
	else_block->val.if_.inner->id = block_id_top++;
	else_block->val.if_.inner->type = HIR_BLOCK_BASIC;
	else_block->val.if_.inner->parent = else_block;
	else_block->val.if_.inner->line = cur_astmt->line;

	/* Visit an inner stmt_list */
	if (cur_astmt->val.else_.stmt_list != NULL) {
		inner_cur_block = else_block->val.if_.inner;
		inner_prev_block = NULL;
		if (!hir_visit_stmt_list(&inner_cur_block,	/* cur_block */
					 &inner_prev_block,	/* prev_block */
					 else_block,		/* parent_block */
					 cur_astmt->val.else_.stmt_list)) {
			hir_free_block(else_block);
			return false;
		}
	}

	/* Move the cursor. */
	*cur_block = else_block->parent->succ;
	*prev_block = else_block;

	return true;
}

/* Visit an AST "while" stmt. */
static bool
hir_visit_while_stmt(
	struct hir_block **cur_block,
	struct hir_block **prev_block,
	struct hir_block *parent_block,
	struct ast_stmt *cur_astmt)
{
	struct hir_block *while_block;
	struct hir_block *exit_block;
	struct hir_block *inner_cur_block;
	struct hir_block *inner_prev_block;

	assert(cur_block != NULL);
	assert(*cur_block != NULL);
	assert(prev_block != NULL);
	assert(parent_block != NULL);
	assert(cur_astmt != NULL);
	assert(cur_astmt->type == AST_STMT_WHILE);

	/* Alloc a while block. */
	if ((*cur_block)->type == HIR_BLOCK_BASIC &&
	    (*cur_block)->val.basic.stmt_list == NULL) {
		/* Reuse an empty basic block. */
		while_block = *cur_block;
		while_block->type = HIR_BLOCK_WHILE;
		while_block->parent = parent_block;
		while_block->line = cur_astmt->line;
	} else {
		while_block = hir_malloc(sizeof(struct hir_block));
		if (while_block == NULL) {
			hir_out_of_memory();
			return false;
		}
		while_block->id = block_id_top++;
		while_block->type = HIR_BLOCK_WHILE;
		while_block->parent = parent_block;
		while_block->line = cur_astmt->line;
		(*cur_block)->succ = while_block;
	}

	/* Alloc an inner block. */
	while_block->val.while_.inner = hir_malloc(sizeof(struct hir_block));
	if (while_block->val.while_.inner == NULL) {
		hir_out_of_memory();
		return false;
	}
	memset(while_block->val.while_.inner, 0, sizeof(struct hir_block));
	while_block->id = block_id_top++;
	while_block->val.while_.inner->type = HIR_BLOCK_BASIC;
	while_block->val.while_.inner->parent = while_block;
	while_block->val.while_.inner->line = cur_astmt->line;

	/* Alloc an exit-block. */
	exit_block = hir_malloc(sizeof(struct hir_block));
	if (exit_block == NULL) {
		hir_out_of_memory();
		return false;
	}
	memset(exit_block, 0, sizeof(struct hir_block));
	exit_block->id = block_id_top++;
	exit_block->type = HIR_BLOCK_BASIC;
	exit_block->parent = parent_block;
	while_block->succ = exit_block;

	/* Visit a cond expr. */
	if (!hir_visit_expr(&while_block->val.while_.cond, cur_astmt->val.while_.cond)) {
		hir_free_block(while_block);
		return false;
	}

	/* Visit an inner stmt_list */
	if (cur_astmt->val.while_.stmt_list != NULL) {
		inner_cur_block = while_block->val.while_.inner;
		inner_prev_block = NULL;
		if (!hir_visit_stmt_list(&inner_cur_block,	/* cur_block */
					 &inner_prev_block,	/* prev_block */
					 while_block,		/* parent_block */
					 cur_astmt->val.while_.stmt_list)) {
			hir_free_block(while_block);
			return false;
		}
	}

	/* Move the cursor to the exit block. */
	*cur_block = exit_block;
	*prev_block = while_block;

	return true;
}

/* Visit an AST "for" stmt. */
static bool
hir_visit_for_stmt(
	struct hir_block **cur_block,
	struct hir_block **prev_block,
	struct hir_block *parent_block,
	struct ast_stmt *cur_astmt)
{
	struct hir_block *for_block;
	struct hir_block *exit_block;
	struct hir_block *inner_cur_block;
	struct hir_block *inner_prev_block;

	assert(cur_block != NULL);
	assert(*cur_block != NULL);
	assert(prev_block != NULL);
	assert(parent_block != NULL);
	assert(cur_astmt != NULL);
	assert(cur_astmt->type == AST_STMT_FOR);

	/* Alloc an for block. */
	if ((*cur_block)->type == HIR_BLOCK_BASIC &&
	    (*cur_block)->val.basic.stmt_list == NULL) {
		/* Reuse an empty basic block. */
		for_block = *cur_block;
		for_block->type = HIR_BLOCK_FOR;
		for_block->parent = parent_block;
		for_block->line = cur_astmt->line;
	} else {
		for_block = hir_malloc(sizeof(struct hir_block));
		if (for_block == NULL) {
			hir_out_of_memory();
			return false;
		}
		memset(for_block, 0, sizeof(struct hir_block));
		for_block->id = block_id_top++;
		for_block->type = HIR_BLOCK_FOR;
		for_block->parent = parent_block;
		for_block->line = cur_astmt->line;
		(*cur_block)->succ = for_block;
	}

	/* Alloc an inner block. */
	for_block->val.for_.inner = hir_malloc(sizeof(struct hir_block));
	if (for_block->val.for_.inner == NULL) {
		hir_out_of_memory();
		return false;
	}
	memset(for_block->val.for_.inner, 0, sizeof(struct hir_block));
	for_block->val.for_.inner->id = block_id_top++;
	for_block->val.for_.inner->type = HIR_BLOCK_BASIC;
	for_block->val.for_.inner->parent = for_block;
	for_block->val.for_.inner->line = cur_astmt->line;

	/* Alloc an exit-block. */
	exit_block = hir_malloc(sizeof(struct hir_block));
	if (exit_block == NULL) {
		hir_out_of_memory();
		return false;
	}
	memset(exit_block, 0, sizeof(struct hir_block));
	exit_block->id = block_id_top++;
	exit_block->type = HIR_BLOCK_BASIC;
	exit_block->parent = parent_block;
	exit_block->succ = parent_block->succ;
	for_block->succ = exit_block;

	/* Copy the iterator, key, and value symbols. */
	if (cur_astmt->val.for_.counter_symbol) {
		for_block->val.for_.is_ranged = true;
		for_block->val.for_.counter_symbol = hir_strdup(cur_astmt->val.for_.counter_symbol);
		if (for_block->val.for_.counter_symbol == NULL) {
			hir_out_of_memory();
			return false;
		}
		if (!hir_add_local(*cur_block, for_block->val.for_.counter_symbol))
			return false;
	}
	if (cur_astmt->val.for_.key_symbol) {
		for_block->val.for_.key_symbol = hir_strdup(cur_astmt->val.for_.key_symbol);
		if (for_block->val.for_.key_symbol == NULL) {
			hir_out_of_memory();
			return false;
		}
		if (!hir_add_local(*cur_block, for_block->val.for_.key_symbol))
			return false;
	}
	if (cur_astmt->val.for_.value_symbol) {
		for_block->val.for_.value_symbol = hir_strdup(cur_astmt->val.for_.value_symbol);
		if (for_block->val.for_.value_symbol == NULL) {
			hir_out_of_memory();
			return false;
		}
		if (!hir_add_local(*cur_block, for_block->val.for_.value_symbol))
			return false;
	}

	/* Visit the start and stop exprs. */
	if (cur_astmt->val.for_.start != NULL) {
		if (!hir_visit_expr(&for_block->val.for_.start, cur_astmt->val.for_.start)) {
			hir_free_block(for_block);
			return false;
		}
	}
	if (cur_astmt->val.for_.stop != NULL) {
		if (!hir_visit_expr(&for_block->val.for_.stop, cur_astmt->val.for_.stop)) {
			hir_free_block(for_block);
			return false;
		}
	}

	/* Visit the collection expr. */
	if (cur_astmt->val.for_.collection != NULL) {
		if (!hir_visit_expr(&for_block->val.for_.collection, cur_astmt->val.for_.collection)) {
			hir_free_block(for_block);
			return false;
		}
	}

	/* Visit an inner stmt_list */
	inner_cur_block = for_block->val.for_.inner;
	inner_prev_block = NULL;
	if (!hir_visit_stmt_list(&inner_cur_block,	/* cur_block */
				 &inner_prev_block,	/* prev_block */
				 for_block,		/* parent_block */
				 cur_astmt->val.for_.stmt_list)) {
		hir_free_block(for_block);
		return false;
	}

	/* Move the cursor to the exit block. */
	*cur_block = exit_block;
	*prev_block = for_block;

	return true;
}

/* Visit an AST return stmt. */
static bool
hir_visit_return_stmt(
	struct hir_block **cur_block,
	struct hir_block **prev_block,
	struct hir_block *parent_block,
	struct ast_stmt *cur_astmt)
{
	struct hir_stmt *hstmt;

	assert(cur_block != NULL);
	assert(*cur_block != NULL);
	assert((*cur_block)->type == HIR_BLOCK_BASIC);
	assert(prev_block != NULL);
	assert(parent_block != NULL);
	assert(cur_astmt != NULL);
	assert(cur_astmt->type == AST_STMT_RETURN);

	/* Assume we are on a basic block. */
	assert((*cur_block)->type == HIR_BLOCK_BASIC);

	/* Allocate an hstmt. */
	hstmt = hir_malloc(sizeof(struct hir_stmt));
	if (hstmt == NULL) {
		hir_out_of_memory();
		return false;
	}
	memset(hstmt, 0, sizeof(struct hir_stmt));
	hstmt->line = cur_astmt->line;

	/* Set LHS. */
	hstmt->lhs = hir_malloc(sizeof(struct hir_expr));
	if (hstmt->lhs == NULL) {
		hir_out_of_memory();
		return false;
	}
	memset(hstmt->lhs, 0, sizeof(struct hir_expr));
	hstmt->lhs->type = HIR_EXPR_TERM;
	hstmt->lhs->val.term.term = hir_malloc(sizeof(struct hir_term));
	if (hstmt->lhs->val.term.term == NULL) {
		hir_out_of_memory();
		return false;
	}
	memset(hstmt->lhs->val.term.term, 0, sizeof(struct hir_term));
	hstmt->lhs->val.term.term->type = HIR_TERM_SYMBOL;
	hstmt->lhs->val.term.term->val.symbol = hir_strdup("$return");
	if (hstmt->lhs->val.term.term->val.symbol == NULL) {
		hir_out_of_memory();
		return false;
	}

	/* Visit an expr. */
	if (!hir_visit_expr(&hstmt->rhs, cur_astmt->val.return_.expr)) {
		hir_free_stmt(hstmt);
		return false;
	}

	/* Add hstmt to the end of the block. */
	HIR_ADD_TO_LAST(struct hir_stmt, (*cur_block)->val.basic.stmt_list, hstmt);

	/* Continue on the same basic block. */

	return true;
}

/* Visit an AST expr. */
static bool
hir_visit_expr(
	struct hir_expr **hexpr,
	struct ast_expr *aexpr)
{
	bool result;

	assert(hexpr != NULL);
	assert(*hexpr == NULL);
	assert(aexpr != NULL);

	/* Visit by type. */
	switch (aexpr->type) {
	case AST_EXPR_TERM:
		result = hir_visit_term_expr(hexpr, aexpr);
		break;
	case AST_EXPR_LT:
		result = hir_visit_binary_expr(hexpr, aexpr, HIR_EXPR_LT);
		break;
	case AST_EXPR_LTE:
		result = hir_visit_binary_expr(hexpr, aexpr, HIR_EXPR_LTE);
		break;
	case AST_EXPR_GT:
		result = hir_visit_binary_expr(hexpr, aexpr, HIR_EXPR_GT);
		break;
	case AST_EXPR_GTE:
		result = hir_visit_binary_expr(hexpr, aexpr, HIR_EXPR_GTE);
		break;
	case AST_EXPR_EQ:
		result = hir_visit_binary_expr(hexpr, aexpr, HIR_EXPR_EQ);
		break;
	case AST_EXPR_NEQ:
		result = hir_visit_binary_expr(hexpr, aexpr, HIR_EXPR_NEQ);
		break;
	case AST_EXPR_PLUS:
		result = hir_visit_binary_expr(hexpr, aexpr, HIR_EXPR_PLUS);
		break;
	case AST_EXPR_MINUS:
		result = hir_visit_binary_expr(hexpr, aexpr, HIR_EXPR_MINUS);
		break;
	case AST_EXPR_MUL:
		result = hir_visit_binary_expr(hexpr, aexpr, HIR_EXPR_MUL);
		break;
	case AST_EXPR_DIV:
		result = hir_visit_binary_expr(hexpr, aexpr, HIR_EXPR_DIV);
		break;
	case AST_EXPR_MOD:
		result = hir_visit_binary_expr(hexpr, aexpr, HIR_EXPR_MOD);
		break;
	case AST_EXPR_AND:
		result = hir_visit_binary_expr(hexpr, aexpr, HIR_EXPR_AND);
		break;
	case AST_EXPR_OR:
		result = hir_visit_binary_expr(hexpr, aexpr, HIR_EXPR_OR);
		break;
	case AST_EXPR_XOR:
		result = hir_visit_binary_expr(hexpr, aexpr, HIR_EXPR_XOR);
		break;
	case AST_EXPR_SHL:
		result = hir_visit_binary_expr(hexpr, aexpr, HIR_EXPR_SHL);
		break;
	case AST_EXPR_SHR:
		result = hir_visit_binary_expr(hexpr, aexpr, HIR_EXPR_SHR);
		break;
	case AST_EXPR_SUBSCR:
		result = hir_visit_binary_expr(hexpr, aexpr, HIR_EXPR_SUBSCR);
		break;
	case AST_EXPR_NEG:
		result = hir_visit_unary_expr(hexpr, aexpr, HIR_EXPR_NEG);
		break;
	case AST_EXPR_NOT:
		result = hir_visit_unary_expr(hexpr, aexpr, HIR_EXPR_NOT);
		break;
	case AST_EXPR_PAR:
		result = hir_visit_unary_expr(hexpr, aexpr, HIR_EXPR_PAR);
		break;
	case AST_EXPR_DOT:
		result = hir_visit_dot_expr(hexpr, aexpr);
		break;
	case AST_EXPR_CALL:
		result = hir_visit_call_expr(hexpr, aexpr);
		break;
	case AST_EXPR_THISCALL:
		result = hir_visit_thiscall_expr(hexpr, aexpr);
		break;
	case AST_EXPR_ARRAY:
		result = hir_visit_array_expr(hexpr, aexpr);
		break;
	case AST_EXPR_DICT:
		result = hir_visit_dict_expr(hexpr, aexpr);
		break;
	case AST_EXPR_FUNC:
		result = hir_visit_func_expr(hexpr, aexpr);
		break;
	case AST_EXPR_NEW:
		result = hir_visit_new_expr(hexpr, aexpr);
		break;
	default:
		result = false;
		assert(UNIMPLEMENTED);
		break;
	}

	return result;
}

/* Visit an AST term expr. */
static bool
hir_visit_term_expr(
	struct hir_expr **hexpr,
	struct ast_expr *aexpr)
{
	struct hir_expr *e;

	assert(hexpr != NULL);
	assert(*hexpr == NULL);
	assert(aexpr != NULL);
	assert(aexpr->type == AST_EXPR_TERM);

	/* Allocate an hexpr. */
	e = hir_malloc(sizeof(struct hir_expr));
	if (e == NULL) {
		hir_out_of_memory();
		return false;
	}
	memset(e, 0, sizeof(struct hir_expr));
	e->type = HIR_EXPR_TERM;

	/* Visit a term. */
	if (!hir_visit_term(&e->val.term.term, aexpr->val.term.term)) {
		hir_free_expr(e);
		return false;
	}

	*hexpr = e;

	return true;
}

/* Visit an AST binary-op expr. */
static bool
hir_visit_binary_expr(
	struct hir_expr **hexpr,
	struct ast_expr *aexpr,
	int type)
{
	struct hir_expr *e;

	assert(hexpr != NULL);
	assert(*hexpr == NULL);
	assert(aexpr != NULL);

	/* Allocate an hexpr. */
	e = hir_malloc(sizeof(struct hir_expr));
	if (e == NULL) {
		hir_out_of_memory();
		return false;
	}
	memset(e, 0, sizeof(struct hir_expr));
	e->type = type;

	/* Visit the two expressions. */
	if (!hir_visit_expr(&e->val.binary.expr[0], aexpr->val.binary.expr[0])) {
		hir_free_expr(e);
		return false;
	}
	if (!hir_visit_expr(&e->val.binary.expr[1], aexpr->val.binary.expr[1])) {
		hir_free_expr(e);
		return false;
	}

	*hexpr = e;

	return true;
}

/* Visit an AST unary-op expr. */
static bool
hir_visit_unary_expr(
	struct hir_expr **hexpr,
	struct ast_expr *aexpr,
	int type)
{
	struct hir_expr *e;

	assert(hexpr != NULL);
	assert(*hexpr == NULL);
	assert(aexpr != NULL);
	assert(aexpr->type == AST_EXPR_NEG ||
	       aexpr->type == AST_EXPR_NOT ||
	       aexpr->type == AST_EXPR_PAR);

	/* Allocate an hexpr. */
	e = hir_malloc(sizeof(struct hir_expr));
	if (e == NULL) {
		hir_out_of_memory();
		return false;
	}
	memset(e, 0, sizeof(struct hir_expr));
	e->type = type;

	/* Visit the expression. */
	if (!hir_visit_expr(&e->val.unary.expr, aexpr->val.unary.expr)) {
		hir_free_expr(e);
		return false;
	}

	*hexpr = e;

	return true;
}

/* Visit an AST dot expr. */
static bool
hir_visit_dot_expr(
	struct hir_expr **hexpr,
	struct ast_expr *aexpr)
{
	struct hir_expr *e;

	assert(hexpr != NULL);
	assert(*hexpr == NULL);
	assert(aexpr != NULL);
	assert(aexpr->type == AST_EXPR_DOT);

	/* Allocate an hexpr. */
	e = hir_malloc(sizeof(struct hir_expr));
	if (e == NULL) {
		hir_out_of_memory();
		return false;
	}
	memset(e, 0, sizeof(struct hir_expr));
	e->type = HIR_EXPR_DOT;

	/* Visit the expression. */
	if (!hir_visit_expr(&e->val.dot.obj, aexpr->val.dot.obj)) {
		hir_free_expr(e);
		return false;
	}

	/* Copy the member symbol. */
	e->val.dot.symbol = hir_strdup(aexpr->val.dot.symbol);
	if (e->val.dot.symbol == NULL) {
		hir_free_expr(e);
		return false;
	}

	*hexpr = e;

	return true;
}

/* Visit an AST call expr. */
static bool
hir_visit_call_expr(
	struct hir_expr **hexpr,
	struct ast_expr *aexpr)
{
	struct hir_expr *e;
	struct ast_expr *arg;

	assert(hexpr != NULL);
	assert(*hexpr == NULL);
	assert(aexpr != NULL);
	assert(aexpr->type == AST_EXPR_CALL);

	/* Allocate an hexpr. */
	e = hir_malloc(sizeof(struct hir_expr));
	if (e == NULL) {
		hir_out_of_memory();
		return false;
	}
	memset(e, 0, sizeof(struct hir_expr));
	e->type = HIR_EXPR_CALL;

	/* Visit the func expression. */
	if (!hir_visit_expr(&e->val.call.func, aexpr->val.call.func)) {
		hir_free_expr(e);
		return false;
	}

	/* Visit the argument expressions. */
	if (aexpr->val.call.arg_list != NULL) {
		arg = aexpr->val.call.arg_list->list;
		while (arg != NULL) {
			if (!hir_visit_expr(&e->val.call.arg[e->val.call.arg_count], arg)) {
				hir_free_expr(e);
				return false;
			}
			arg = arg->next;
			e->val.call.arg_count++;
			if (e->val.call.arg_count > HIR_PARAM_SIZE) {
				hir_fatal(hir_error_line, N_TR("Exceeded the maximum argument count."));
				return false;
			}
		}
	}

	*hexpr = e;

	return true;
}

/* Visit an AST call expr. */
static bool
hir_visit_thiscall_expr(
	struct hir_expr **hexpr,
	struct ast_expr *aexpr)
{
	struct hir_expr *e;
	struct ast_expr *arg;

	assert(hexpr != NULL);
	assert(*hexpr == NULL);
	assert(aexpr != NULL);
	assert(aexpr->type == AST_EXPR_THISCALL);

	/* Allocate an hexpr. */
	e = hir_malloc(sizeof(struct hir_expr));
	if (e == NULL) {
		hir_out_of_memory();
		return false;
	}
	memset(e, 0, sizeof(struct hir_expr));
	e->type = HIR_EXPR_THISCALL;

	/* Visit the object expression. */
	if (!hir_visit_expr(&e->val.thiscall.obj, aexpr->val.thiscall.obj)) {
		hir_free_expr(e);
		return false;
	}

	/* Copy the function name. */
	e->val.thiscall.func = hir_strdup(aexpr->val.thiscall.func);
	if (e->val.thiscall.func == NULL) {
		hir_free_expr(e);
		return false;
	}

	/* Visit the argument expressions. */
	if (aexpr->val.thiscall.arg_list != NULL) {
		arg = aexpr->val.thiscall.arg_list->list;
		while (arg != NULL) {
			if (!hir_visit_expr(&e->val.thiscall.arg[e->val.thiscall.arg_count], arg)) {
				hir_free_expr(e);
				return false;
			}
			arg = arg->next;
			e->val.thiscall.arg_count++;
		}
	}

	*hexpr = e;

	return true;
}

/* Visit an AST array expr. */
static bool
hir_visit_array_expr(
	struct hir_expr **hexpr,
	struct ast_expr *aexpr)
{
	struct hir_expr *e;
	struct ast_expr *elem;
	int count, index;

	assert(hexpr != NULL);
	assert(*hexpr == NULL);
	assert(aexpr != NULL);
	assert(aexpr->type == AST_EXPR_ARRAY);

	/* Allocate an hexpr. */
	e = hir_malloc(sizeof(struct hir_expr));
	if (e == NULL) {
		hir_out_of_memory();
		return false;
	}
	memset(e, 0, sizeof(struct hir_expr));
	e->type = HIR_EXPR_ARRAY;

	/* Count the elements and allocate a table. */
	count = 0;
	if (aexpr->val.array.elem_list != NULL) {
		elem = aexpr->val.array.elem_list->list;
		while (elem != NULL) {
			elem = elem->next;
			count++;
		}

		e->val.array.elem_count = count;
		e->val.array.elem = hir_malloc(count * sizeof(struct hir_exp *));
		if (e->val.array.elem == NULL) {
			hir_out_of_memory();
			return false;
		}
		memset(e->val.array.elem, 0, count * sizeof(struct hir_exp *));
	}

	/* Visit the argument expressions. */
	if (aexpr->val.array.elem_list != NULL) {
		elem = aexpr->val.array.elem_list->list;
		index = 0;
		while (elem != NULL) {
			if (!hir_visit_expr(&e->val.array.elem[index], elem)) {
				hir_free_expr(e);
				return false;
			}
			elem = elem->next;
			index++;
		}
	}

	*hexpr = e;

	return true;
}

/* Visit an AST dictionary expr. */
static bool
hir_visit_dict_expr(
	struct hir_expr **hexpr,
	struct ast_expr *aexpr)
{
	struct hir_expr *e;
	struct ast_kv *kv;
	int count, index;

	assert(hexpr != NULL);
	assert(*hexpr == NULL);
	assert(aexpr != NULL);
	assert(aexpr->type == AST_EXPR_DICT);

	/* Allocate an hexpr. */
	e = hir_malloc(sizeof(struct hir_expr));
	if (e == NULL) {
		hir_out_of_memory();
		return false;
	}
	memset(e, 0, sizeof(struct hir_expr));
	e->type = HIR_EXPR_DICT;

	/* Count the elements and allocate a table. */
	count = 0;
	if (aexpr->val.dict.kv_list != NULL) {
		kv = aexpr->val.dict.kv_list->list;
		while (kv != NULL) {
			kv = kv->next;
			count++;
		}

		e->val.dict.kv_count = count;

		e->val.dict.key = hir_malloc(count * sizeof(char *));
		if (e->val.dict.key == NULL) {
			hir_out_of_memory();
			return false;
		}
		memset(e->val.dict.key, 0, count * sizeof(char *));

		e->val.dict.value = hir_malloc(count * sizeof(struct hir_exp *));
		if (e->val.dict.value == NULL) {
			hir_out_of_memory();
			return false;
		}
		memset(e->val.dict.value, 0, count * sizeof(struct hir_exp *));
	}

	/* Visit the argument expressions. */
	if (aexpr->val.dict.kv_list != NULL) {
		kv = aexpr->val.dict.kv_list->list;
		index = 0;
		while (kv != NULL) {
			/* Copy the key. */
			e->val.dict.key[index] = hir_strdup(kv->key);
			if (e->val.dict.key[index] == NULL) {
				hir_out_of_memory();
				return false;
			}

			/* Copy the value. */
			if (!hir_visit_expr(&e->val.dict.value[index], kv->value)) {
				hir_free_expr(e);
				return false;
			}

			kv = kv->next;
			index++;
		}
	}

	*hexpr = e;

	return true;
}

/* Visit an AST anonymous function expr. */
static bool
hir_visit_func_expr(
	struct hir_expr **hexpr,
	struct ast_expr *aexpr)
{
	struct hir_expr *e;
	struct hir_term *t;

	assert(hexpr != NULL);
	assert(*hexpr == NULL);
	assert(aexpr != NULL);
	assert(aexpr->type == AST_EXPR_FUNC);

	/* Here, we replace an anonymous function to a symbol. */

	/* Alocate an hterm. */
	t = hir_malloc(sizeof(struct hir_term));
	if (t == NULL) {
		hir_out_of_memory();
		return false;
	}
	memset(t, 0, sizeof(struct hir_term));
	t->type = HIR_TERM_SYMBOL;

	/* Allocate an hexpr. */
	e = hir_malloc(sizeof(struct hir_expr));
	if (e == NULL) {
		hir_out_of_memory();
		return false;
	}
	memset(e, 0, sizeof(struct hir_expr));
	e->type = HIR_EXPR_TERM;
	e->val.term.term = t;

	/* Defer the analysis of the anonymous function. */
	if (!hir_defer_anon_func(aexpr, &t->val.symbol))
		return false;

	*hexpr = e;

	return true;
}

/* Visit an AST new expr. */
static bool
hir_visit_new_expr(
	struct hir_expr **hexpr,
	struct ast_expr *aexpr)
{
	struct hir_expr *e;

	assert(hexpr != NULL);
	assert(*hexpr == NULL);
	assert(aexpr != NULL);
	assert(aexpr->type == AST_EXPR_NEW);

	/* Allocate an hexpr. */
	e = hir_malloc(sizeof(struct hir_expr));
	if (e == NULL) {
		hir_out_of_memory();
		return false;
	}
	memset(e, 0, sizeof(struct hir_expr));
	e->type = HIR_EXPR_NEW;
	e->val.new_.cls = hir_strdup(aexpr->val.new_.cls);
	if (e->val.new_.cls == NULL) {
		hir_out_of_memory();
		return false;
	}

	/* Visit an expr. */
	if (aexpr->val.new_.init != NULL) {
		if (!hir_visit_expr(&e->val.new_.init, aexpr->val.new_.init)) {
			hir_free_expr(e);
			return false;
		}
	}

	*hexpr = e;

	return true;
}

/* Visit an AST term. */
static bool
hir_visit_term(
	struct hir_term **hterm,
	struct ast_term *aterm)
{
	struct hir_term *t;

	/* Allocate an hterm. */
	t = hir_malloc(sizeof(struct hir_term));
	if (t == NULL) {
		hir_out_of_memory();
		return false;
	}
	memset(t, 0, sizeof(struct hir_term));

	/* Copy the value. */
	switch (aterm->type) {
	case AST_TERM_SYMBOL:
		t->type = HIR_TERM_SYMBOL;
		t->val.symbol = hir_strdup(aterm->val.symbol);
		if (t->val.symbol == NULL) {
			hir_out_of_memory();
			return false;
		}
		break;
	case AST_TERM_INT:
		t->type = HIR_TERM_INT;
		t->val.i = aterm->val.i;
		break;
	case AST_TERM_FLOAT:
		t->type = HIR_TERM_FLOAT;
		t->val.f = (float)aterm->val.f;
		break;
	case AST_TERM_STRING:
		t->type = HIR_TERM_STRING;
		t->val.s = hir_strdup(aterm->val.s);
		if (t->val.symbol == NULL) {
			hir_out_of_memory();
			return false;
		}
		break;
	case AST_TERM_EMPTY_ARRAY:
		t->type = HIR_TERM_EMPTY_ARRAY;
		break;
	case AST_TERM_EMPTY_DICT:
		t->type = HIR_TERM_EMPTY_DICT;
		break;
	default:
		assert(NEVER_COME_HERE);
		break;
	}

	*hterm = t;

	return true;
}

/* Copy parameter names and count parameters. */
static bool
hir_visit_param_list(
	struct hir_block *hfunc,
	struct ast_func *afunc)
{
	struct ast_param *param;
	int param_count;

	/* If there is no param_list. */
	if (afunc->param_list == NULL) {
		hfunc->val.func.param_count = 0;
		return true;
	}

	/* Assume we have at lease one parameter. */
	assert(afunc->param_list->list != NULL);

	/* Do traverse. */
	param = afunc->param_list->list;
	param_count = 0;
	while (param != NULL) {
		/* Copy names and count parameters. */
		hfunc->val.func.param_name[param_count] = hir_strdup(param->name);
		if (param->name == NULL) {
			hir_out_of_memory();
			return false;
		}
		param_count++;

		/* Add to a local variable list. */
		if (!hir_add_local(hfunc, param->name))
			return false;

		param = param->next;
	}
	hfunc->val.func.param_count = param_count;

	return true;
}

/* Defer an analysis of an anonymous function. */
static bool
hir_defer_anon_func(
	struct ast_expr *aexpr,
	char **symbol)
{
	char name[1024];

	snprintf(name, sizeof(name), "$anon.%s.%d", hir_file_name, hir_anon_func_count);
	*symbol = hir_strdup(name);
	if (*symbol == NULL) {
		hir_out_of_memory();
		return false;
	}

	hir_anon_func_name[hir_anon_func_count] = *symbol;
	hir_anon_func_param_list[hir_anon_func_count] = aexpr->val.func.param_list;
	hir_anon_func_stmt_list[hir_anon_func_count] = aexpr->val.func.stmt_list;

	hir_anon_func_count++;
	if (hir_anon_func_count >= ANON_FUNC_SIZE) {
		hir_fatal(hir_error_line, N_TR("Too many anonymous functions."));
		return false;
	}

	return true;
}

/* Free a block and its siblings. */
static void
hir_free_block(
	struct hir_block *b)
{
	int i;

	switch (b->type) {
	case HIR_BLOCK_FUNC:
		if (b->val.func.name != NULL) {
			hir_free(b->val.func.name);
			b->val.func.name = NULL;
		}
		for (i = 0; i < b->val.func.param_count; i++) {
			if (b->val.func.param_name[i] != NULL) {
				hir_free(b->val.func.param_name[i]);
				b->val.func.param_name[i] = NULL;
			}
		}
		if (b->val.func.inner != NULL) {
			hir_free_block(b->val.func.inner);
			b->val.func.inner = NULL;
		}
		if (b->val.func.local != NULL) {
			hir_free_local(b->val.func.local);
			b->val.func.local = NULL;
		}
		break;
	case HIR_BLOCK_BASIC:
		if (b->val.basic.stmt_list != NULL) {
			hir_free_stmt(b->val.basic.stmt_list);
			b->val.basic.stmt_list = NULL;
		}
		break;
	case HIR_BLOCK_IF:
		if (b->val.if_.cond != NULL) {
			hir_free_expr(b->val.if_.cond);
			b->val.if_.cond = NULL;
		}
		if (b->val.if_.inner != NULL) {
			hir_free_block(b->val.if_.inner);
			b->val.if_.inner = NULL;
		}
		if (b->val.if_.chain_next != NULL) {
			hir_free_block(b->val.if_.chain_next);
			b->val.if_.chain_next = NULL;
		}
		break;
	case HIR_BLOCK_FOR:
		if (b->val.for_.counter_symbol != NULL) {
			hir_free(b->val.for_.counter_symbol);
			b->val.for_.counter_symbol = NULL;
		}
		if (b->val.for_.key_symbol != NULL) {
			hir_free(b->val.for_.key_symbol);
			b->val.for_.key_symbol = NULL;
		}
		if (b->val.for_.value_symbol != NULL) {
			hir_free(b->val.for_.value_symbol);
			b->val.for_.value_symbol = NULL;
		}
		if (b->val.for_.collection != NULL) {
			hir_free_expr(b->val.for_.collection);
			b->val.for_.collection = NULL;
		}
		if (b->val.for_.inner != NULL) {
			hir_free_block(b->val.for_.inner);
			b->val.for_.inner = NULL;
		}
		break;
	case HIR_BLOCK_WHILE:
		if (b->val.while_.cond != NULL) {
			hir_free_expr(b->val.while_.cond);
			b->val.while_.cond = NULL;
		}
		if (b->val.while_.inner != NULL) {
			hir_free_block(b->val.while_.inner);
			b->val.while_.inner = NULL;
		}
		break;
	case HIR_BLOCK_END:
		break;
	default:
		assert(NEVER_COME_HERE);
		break;
	}

	/* (b->succ == b) is a loop. */
	if (!b->stop && b->succ != NULL) {
		hir_free_block(b->succ);
		b->succ = NULL;
	}
}

/* Free an hstmt. */
static void
hir_free_stmt(
	struct hir_stmt *s)
{
	if (s->next != NULL) {
		hir_free_stmt(s->next);
		s->next = NULL;
	}
	if (s->lhs != NULL) {
		hir_free_expr(s->lhs);
		s->lhs = NULL;
	}
	if (s->rhs != NULL) {
		hir_free_expr(s->rhs);
		s->rhs = NULL;
	}
}

/* Free an hexpr. */
static void
hir_free_expr(
	struct hir_expr *e)
{
	int i;

	switch (e->type) {
	case HIR_EXPR_TERM:
		if (e->val.term.term != NULL) {
			hir_free_term(e->val.term.term);
			e->val.term.term = NULL;
		}
		break;
	/* Binary OPs  */
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
	case HIR_EXPR_SUBSCR:
		if (e->val.binary.expr[0] != NULL) {
			hir_free_expr(e->val.binary.expr[0]);
			e->val.binary.expr[0] = NULL;
		}
		if (e->val.binary.expr[1] != NULL) {
			hir_free_expr(e->val.binary.expr[1]);
			e->val.binary.expr[1] = NULL;
		}
		break;
	/* Unary OPs */
	case HIR_EXPR_NEG:
	case HIR_EXPR_NOT:
	case HIR_EXPR_PAR:
		if (e->val.unary.expr != NULL) {
			hir_free_expr(e->val.unary.expr);
			e->val.unary.expr = NULL;
		}
		break;
	case HIR_EXPR_DOT:
		if (e->val.dot.obj != NULL) {
			hir_free_expr(e->val.dot.obj);
			e->val.dot.obj = NULL;
		}
		if (e->val.dot.symbol != NULL) {
			hir_free(e->val.dot.symbol);
			e->val.dot.symbol = NULL;
		}
		break;
	case HIR_EXPR_CALL:
		if (e->val.call.func != NULL) {
			hir_free_expr(e->val.call.func);
			e->val.call.func = NULL;
		}
		for (i = 0; i < e->val.call.arg_count; i++) {
			if (e->val.call.arg[i] != NULL) {
				hir_free_expr(e->val.call.arg[i]);
				e->val.call.arg[i] = NULL;
			}
		}
		break;
	case HIR_EXPR_THISCALL:
		if (e->val.thiscall.obj != NULL) {
			hir_free_expr(e->val.thiscall.obj);
			e->val.thiscall.obj = NULL;
		}
		if (e->val.thiscall.func != NULL) {
			hir_free(e->val.thiscall.func);
			e->val.thiscall.func = NULL;
		}
		for (i = 0; i < e->val.thiscall.arg_count; i++) {
			if (e->val.thiscall.arg[i] != NULL) {
				hir_free_expr(e->val.thiscall.arg[i]);
				e->val.thiscall.arg[i] = NULL;
			}
		}
		break;
	case HIR_EXPR_ARRAY:
		for (i = 0; i < e->val.array.elem_count; i++) {
			if (e->val.array.elem[i] != NULL) {
				hir_free_expr(e->val.array.elem[i]);
				e->val.array.elem[i] = NULL;
			}
		}
		if (e->val.array.elem != NULL) {
			hir_free(e->val.array.elem);
			e->val.array.elem = NULL;
		}
		break;
	case HIR_EXPR_DICT:
		for (i = 0; i < e->val.dict.kv_count; i++) {
			if (e->val.dict.key[i] != NULL) {
				hir_free(e->val.dict.key[i]);
				e->val.dict.key[i] = NULL;
			}
			if (e->val.dict.value[i] != NULL) {
				hir_free_expr(e->val.dict.value[i]);
				e->val.dict.value[i] = NULL;
			}
		}
		if (e->val.dict.key != NULL) {
			hir_free(e->val.dict.key);
			e->val.dict.key = NULL;
		}
		if (e->val.dict.value != NULL) {
			hir_free(e->val.dict.value);
			e->val.dict.value = NULL;
		}
		break;
	case HIR_EXPR_NEW:
		hir_free(e->val.new_.cls);
		if (e->val.new_.init != NULL)
			hir_free_expr(e->val.new_.init);
		break;
	default:
		assert(NEVER_COME_HERE);
		break;
	}
	hir_free(e);
}

/* Free an hterm. */
static void
hir_free_term(
	struct hir_term *t)
{
	switch (t->type) {
	case HIR_TERM_INT:
	case HIR_TERM_FLOAT:
		break;
	case HIR_TERM_SYMBOL:
		if (t->val.symbol != NULL) {
			hir_free(t->val.symbol);
			t->val.symbol = NULL;
		}
		break;
	case HIR_TERM_STRING:
		if (t->val.s != NULL) {
			hir_free(t->val.s);
			t->val.s = NULL;
		}
		break;
	case HIR_TERM_EMPTY_ARRAY:
		break;
	case HIR_TERM_EMPTY_DICT:
		break;
	default:
		assert(NEVER_COME_HERE);
		break;
	}
}

/* Free a local variable list. */
static void
hir_free_local(
	struct hir_local *local)
{
	if (local->next != NULL)
		hir_free_local(local->next);

	hir_free(local->symbol);
}

/* Set a fatal error message. */
static void
hir_fatal(
	int line,
	const char *msg)
{
	hir_error_line = line;

	snprintf(hir_error_message,
		 sizeof(hir_error_message),
		 "%s:%d: %s",
		 hir_file_name,
		 line,
		 msg);
}

/* Show out-of-memory error. */
static void hir_out_of_memory(void)
{
	snprintf(hir_error_message,
		 sizeof(hir_error_message),
		 "%s: Out of memory error.",
		 hir_file_name != NULL ? hir_file_name : "");
}

/*
 * Get a file name.
 */
const char *
hir_get_file_name(void)
{
	assert(hir_file_name);

	return hir_file_name;
}

/*
 * Get an error line number.
 */
int hir_get_error_line(void)
{
	return hir_error_line;
}

/*
 * Get an error message.
 */
const char *hir_get_error_message(void)
{
	return hir_error_message;
}

/*
 * Allocator
 */

static void *hir_malloc(size_t size)
{
	return arena_alloc(&hir_arena, size);
}

static char *hir_strdup(const char *s)
{
	char *ret;

	ret = arena_alloc(&hir_arena, strlen(s) + 1);
	if (ret == NULL)
		return NULL;

	strcpy(ret, s);
	return ret;
}

static void hir_free(void *p)
{
}

/*
 * Debug printer
 */

#ifdef DEBUG_DUMP

static void hir_dump_block_at_level(struct hir_block *block, int level);

void
hir_dump_block(
	struct hir_block *block)
{
	hir_dump_block_at_level(block, 0);
}

static void
hir_dump_block_at_level(
	struct hir_block *block,
	int level)
{
	int i;

	while (block != NULL) {
		for (i = 0; i < level * 4; i++) printf(" ");
		printf("BLOCK(%d)", block->id);

		switch (block->type) {
		case HIR_BLOCK_FUNC:
		{
			printf(" FUNC parent=%d, succ=%d\n", block->parent->id, block->succ->id);

			if (block->val.func.inner != NULL) {
				for (i = 0; i < (level + 1) * 4; i++) printf(" ");
				printf("[INNER]\n");
				hir_dump_block_at_level(block->val.func.inner, level + 1);
			}
			break;
		}
		case HIR_BLOCK_BASIC:
		{
			struct hir_stmt *s;
			if (block->succ != NULL)
				printf(" BASIC parent=%d, succ=%d\n", block->parent->id, block->succ->id);
			else
				printf(" BASIC succ=NULL\n");
			s = block->val.basic.stmt_list;
			while (s != NULL) {
				//hir_dump_stmt(level + 1, s);
				s = s->next;
			}
			break;
		}
		case HIR_BLOCK_FOR:
		{
			if (block->succ != NULL)
				printf(" FOR parent=%d, succ=%d\n", block->parent->id, block->succ->id);
			else
				printf(" FOR succ=NULL\n");

			if (block->val.for_.inner != NULL) {
				for (i = 0; i < (level + 1) * 4; i++) printf(" ");
				printf("[INNER]\n");
				hir_dump_block_at_level(block->val.for_.inner, level + 1);
			}
			break;
		}
		case HIR_BLOCK_END:
		{
			printf(" END\n");
			break;
		}
		case HIR_BLOCK_IF:
			printf(" IF parent=%d, succ=%d, prev=%d, next=%d\n", block->parent->id, block->succ->id, block->val.if_.chain_prev->id, block->val.if_.chain_next->id);
			if (block->val.if_.inner != NULL) {
				for (i = 0; i < (level + 1) * 4; i++) printf(" ");
				printf("[INNER]\n");
				hir_dump_block_at_level(block->val.if_.inner, level + 1);
			}
			if (block->val.if_.chain_next != NULL) {
				for (i = 0; i < (level + 1) * 4; i++) printf(" ");
				printf("[CHAIN]\n");
				hir_dump_block_at_level(block->val.if_.chain_next, level + 1);
			}
			break;
		case HIR_BLOCK_WHILE:
			printf(" WHILE\n");
			break;
		default:
			printf(" SKIP %d\n", block->type);
			break;
		}

		if (block->succ != NULL) {
			if (block->stop) {
				for (i = 0; i < level * 4; i++) printf(" ");
				printf("[STOP %d]\n", block->succ->id);
				break;
			}
		}
		block = block->succ;
	}
}

#endif /* DEBUG_DUMP */
