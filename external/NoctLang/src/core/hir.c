/* -*- coding: utf-8; tab-width: 8; indent-tabs-mode: t; -*- */

/*
 * Noct Programming Language
 * Copyright (c) 2025, 2026, Awe Morris
 */

/*
 * HIR: High-level Intermediate Representation
 */

#include <noct/noct.h>
#include "hir.h"
#if defined(NOCT_USE_OPTIMIZER)
#include "hir_fast_checked.h"
#include "hir_opt.h"
#include "hir_fast_func.h"
#include "hir_opt_parallel.h"
#endif
#include "ast.h"
#include "arena.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <assert.h>
#include <limits.h>

/* False assertions. */
#define NEVER_COME_HERE		(0)
#define UNIMPLEMENTED		(0)

/* Debug dump */
#undef DEBUG_DUMP

/* Arena allocator size. */
#if !defined(NOCT_MEMORY_SMALL)
#define ARENA_SIZE		(64 * 1024 * 1024)
#else
#define ARENA_SIZE		(1024 * 1024)
#endif

/* Maximum number of functions. */
#define HIR_FUNC_MAX		1024

/* Maximum number of anonymous functions. */
#define ANON_FUNC_SIZE	256

/* Maximum exact Packed rank accepted by the checked source form. */
#define HIR_FAST_RANK_MAX	8

/* List-add function. */
#define HIR_ADD_TO_LAST(type, list, p)			\
	do {						\
		if (list == NULL) {			\
			list = p;			\
		} else {				\
			type *elem = list;		\
						\
			/* Find the current list tail. */	\
			while (elem->next)		\
				elem = elem->next;	\
			elem->next = p;			\
		}					\
	} while (0);

enum hir_decimal_result {
	HIR_DECIMAL_OK,
	HIR_DECIMAL_INVALID,
	HIR_DECIMAL_ZERO,
	HIR_DECIMAL_OVERFLOW
};

enum hir_prepare_mode {
	HIR_PREPARE_NONE,
	HIR_PREPARE_CHECKED,
	HIR_PREPARE_OPTIMIZER
};

/*
 * Scopes are pushed for the function body and for every if/elif/else,
 * for, and while body. Declarations are pre-scanned when the scope is
 * pushed so that a use before the declaration is a static error.
 */
struct hir_scope_decl {
	char *src_name;
	char *int_name;
	bool declared;
	bool is_let;
	struct hir_scope_decl *next;
};

struct hir_scope {
	struct hir_scope_decl *decls;
	struct hir_scope *parent;
};

static const char *const hir_fast_intrinsic_names[] = {
	"min", "max", "abs", "sqrt", "sin", "cos", "tan",
	"asin", "acos", "atan", "atan2", "exp", "ln", "log2",
	"log10", "int", "long", "float", "double"
};

/* Constructed HIR. */
static char *hir_file_name;
static uint32_t hir_func_count;
static struct hir_block *hir_func_tbl[HIR_FUNC_MAX];

/*
 * Error position and message.
 */

static int hir_error_line;
static char hir_error_message[1024];

/*
 * Block id top.
 */
static int block_id_top;

/* Module-wide lowering mode selected by the first backend request. */
static enum hir_prepare_mode hir_prepare_mode;

/* Anonymous functions. */
static int hir_anon_func_count;
static char *hir_anon_func_name[ANON_FUNC_SIZE];
static struct ast_param_list *hir_anon_func_param_list[ANON_FUNC_SIZE];
static struct ast_stmt_list *hir_anon_func_stmt_list[ANON_FUNC_SIZE];

/*
 * Arena allocator.
 */
static struct arena_info hir_arena;

/* Lexical scope state for the function currently being constructed. */
static struct hir_scope *hir_scope_top;
static int hir_scope_seq;

/* Forward declarations. */
static void hir_scope_begin_func(void);
static bool hir_scope_push(const struct ast_stmt_list *stmt_list);
static void hir_scope_pop(void);
static bool hir_scope_add_param(int line, const char *src_name);
static bool hir_scope_declare(int line, const char *src_name, bool is_let, const char **int_name, struct hir_scope_decl **decl_ret);
static void hir_scope_mark_declared(struct hir_scope_decl *decl);
static bool hir_scope_resolve(int line, const char *src_name, const char **int_name);
static bool hir_scope_check_assign(int line, const char *int_name);
static struct hir_scope_decl *hir_scope_find_here(const struct hir_scope *scope, const char *src_name);
static struct hir_scope_decl *hir_scope_find(const char *src_name);
static bool hir_scope_add_decl(int line, const char *src_name, bool declared, bool is_let, struct hir_scope_decl **decl_ret);
static bool hir_scope_intern(struct hir_scope_decl *decl);
static bool hir_visit_func(struct ast_func *afunc);
static bool hir_fast_stmt_list_returns(const struct ast_stmt_list *list);
static bool hir_prepare_module(int optimize_level);
static bool hir_lower_checked_fast_syntax(void);
static bool hir_lower_checked_fast_chain(struct hir_block *func, struct hir_block *head);
static bool hir_lower_checked_fast_expr(struct hir_block *func, struct hir_expr **slot, int line);
static bool hir_lower_checked_fast_multi(struct hir_block *func, struct hir_expr *subscript, int line);
static bool hir_rewrite_fast_intrinsic(struct hir_expr *call, const char *name);
static bool hir_is_fast_intrinsic_name(const char *name);
static bool hir_has_function_name(const char *name);
static int hir_find_shaped_param(const struct hir_block *func, const struct hir_expr *base);
static bool hir_add_checked_fast_prologues(void);
static bool hir_add_checked_fast_prologue(struct hir_block *func);
static bool hir_build_shape_check_stmt(struct hir_block *func, uint32_t param_index, struct hir_stmt **stmt_ret);
static bool hir_parse_shape_extent_exprs(struct hir_block *func, const char *annotation, struct hir_expr *extent[HIR_FAST_RANK_MAX], uint32_t *rank_ret);
static int hir_parse_positive_decimal(const char *text, size_t length, int64_t *value);
static int hir_find_param_index(const struct hir_block *func, const char *text, size_t length);
static bool hir_is_identifier(const char *text, size_t length);
static struct hir_expr *hir_make_symbol_expr(const char *symbol);
static struct hir_expr *hir_make_integer_expr(int64_t value);
static bool hir_visit_stmt_list(struct hir_block **cur_block, struct hir_block **prev_block, struct hir_block *parent_block, struct ast_stmt_list *stmt_list);
static bool hir_visit_stmt(struct hir_block **cur_block, struct hir_block **prev_block, struct hir_block *parent_block, struct ast_stmt *cur_astmt);
static bool hir_visit_expr_stmt(struct hir_block **cur_block, struct hir_block **prev_block, struct hir_block *parent_block, struct ast_stmt *cur_astmt);
static bool hir_visit_assign_stmt(struct hir_block **cur_block, struct hir_block **prev_block, struct hir_block *parent_block, struct ast_stmt *cur_astmt);
static bool hir_visit_if_stmt(struct hir_block **cur_block, struct hir_block **prev_block, struct hir_block *parent_block, struct ast_stmt *cur_astmt);
static bool hir_visit_elif_stmt(struct hir_block **cur_block, struct hir_block **prev_block, struct hir_block *parent_block, struct ast_stmt *cur_astmt);
static bool hir_visit_else_stmt(struct hir_block **cur_block, struct hir_block **prev_block, struct hir_block *parent_block, struct ast_stmt *cur_astmt);
static bool hir_visit_while_stmt(struct hir_block **cur_block, struct hir_block **prev_block, struct hir_block *parent_block, struct ast_stmt *cur_astmt);
static bool hir_visit_for_stmt(struct hir_block **cur_block, struct hir_block **prev_block, struct hir_block *parent_block, struct ast_stmt *cur_astmt);
static bool hir_visit_return_stmt(struct hir_block **cur_block, struct hir_block **prev_block, struct hir_block *parent_block, struct ast_stmt *cur_astmt);
static bool hir_visit_expr(struct hir_expr **hexpr, struct ast_expr *aexpr);
static bool hir_visit_term_expr(struct hir_expr **hexpr, struct ast_expr *aexpr);
static bool hir_visit_binary_expr(struct hir_expr **hexpr, struct ast_expr *aexpr, int type);
static bool hir_resolve_type_name(const char *name, int *tag, int *packed_type, bool *restricted);
static bool hir_visit_unary_expr(struct hir_expr **hexpr, struct ast_expr *aexpr, int type);
static bool hir_visit_dot_expr(struct hir_expr **hexpr, struct ast_expr *aexpr);
static bool hir_visit_call_expr(struct hir_expr **hexpr, struct ast_expr *aexpr);
static bool hir_visit_thiscall_expr(struct hir_expr **hexpr, struct ast_expr *aexpr);
static bool hir_visit_array_expr(struct hir_expr **hexpr, struct ast_expr *aexpr);
static bool hir_visit_dict_expr(struct hir_expr **hexpr, struct ast_expr *aexpr);
static bool hir_visit_func_expr(struct hir_expr **hexpr, struct ast_expr *aexpr);
static bool hir_visit_new_expr(struct hir_expr **hexpr, struct ast_expr *aexpr);
static bool hir_visit_term(struct hir_term **hterm, struct ast_term *aterm);
static bool hir_annotation_base(const char *annotation, char *base, size_t base_size);
static bool hir_visit_param_list(struct hir_block *hfunc, struct ast_func *afunc);
static bool hir_defer_anon_func(struct ast_expr *aexpr, char **symbol);
static struct hir_local *hir_find_local(struct hir_block *block, const char *symbol);
static bool hir_set_local_declaration(struct hir_block *block, const char *symbol, int declaration_kind, int declared_type, const char *declared_type_name, int declared_scalar_kind, int declared_packed_type, int storage_class, int line, const struct hir_stmt *declaration_stmt, const struct hir_expr *initializer);
static int hir_packed_constructor_type(const struct hir_expr *expr);
static int hir_declared_scalar_kind(const char *type_name);
static bool hir_wrap_freeze(struct hir_expr **hexpr, struct hir_expr *inner);
static void hir_free_block(struct hir_block *b);
static void hir_free_stmt(struct hir_stmt *s);
static void hir_free_expr(struct hir_expr *e);
static void hir_free_term(struct hir_term *t);
static void hir_free_local(struct hir_local *local);
static void hir_fatal(int line, const char *msg);
static void hir_free(void *p);
static void hir_dump_block_at_level(struct hir_block *block, int level);

/*
 * Identifies a recognized intrinsic call expression.
 */
int
hir_get_intrinsic_call(
	const struct hir_expr *expr)
{
	const struct hir_expr *fn;
	const struct hir_expr *obj;
	const char *pkg;

	if (expr == NULL ||
	    expr->type != HIR_EXPR_CALL ||
	    expr->val.call.arg_count != 1)
		return HIR_INTRINSIC_NONE;
	fn = expr->val.call.func;
	if (fn == NULL ||
	    fn->type != HIR_EXPR_DOT ||
	    strcmp(fn->val.dot.symbol, "from") != 0)
		return HIR_INTRINSIC_NONE;
	obj = fn->val.dot.obj;
	if (obj == NULL ||
	    obj->type != HIR_EXPR_TERM ||
	    obj->val.term.term->type != HIR_TERM_SYMBOL)
		return HIR_INTRINSIC_NONE;
	pkg = obj->val.term.term->val.symbol;
	if (strcmp(pkg, "Int") == 0)
		return HIR_INTRINSIC_INT_FROM;
	if (strcmp(pkg, "Float") == 0)
		return HIR_INTRINSIC_FLOAT_FROM;

	return HIR_INTRINSIC_NONE;
}

/*
 * Constructs an HIR from an AST.
 */
bool
hir_build(
	void)
{
	struct ast_func_list *func_list;
	struct ast_func *func;
	int i;

	assert(hir_file_name == NULL);
	assert(hir_func_count == 0);

	hir_prepare_mode = HIR_PREPARE_NONE;

	/* Initialize the arena allocator. */
	if (!arena_init(&hir_arena, ARENA_SIZE)) {
		hir_out_of_memory();
		return false;
	}

	/* Copy a file name. */
	hir_file_name = hir_strdup(ast_get_file_name());
	if (hir_file_name == NULL) {
		hir_out_of_memory();
		return false;
	}

	hir_anon_func_count = 0;

	/* Construct an HIR function for each AST function. */
	func_list = ast_get_func_list();
	assert(func_list != NULL);
	func = func_list->list;

	/* Visit every source function. */
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

	/* Construct every deferred anonymous function. */
	for (i = 0; i < hir_anon_func_count; i++) {
		/* Visit an AST func. */
		struct ast_func afunc;

		afunc.name = hir_anon_func_name[i];
		afunc.param_list = hir_anon_func_param_list[i];
		afunc.return_type_name = NULL;
		afunc.is_static = false;
		afunc.is_inline = false;
		afunc.is_fast = false;
		afunc.is_accel = false;
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
 * Frees all constructed HIR functions.
 */
void
hir_cleanup(
	void)
{
	uint32_t i;

	if (hir_file_name != NULL) {
		hir_free(hir_file_name);
		hir_file_name = NULL;
	}

	/* Free every constructed function. */
	for (i = 0; i < hir_func_count; i++) {
		hir_free_block(hir_func_tbl[i]);
		hir_func_tbl[i] = NULL;
	}

	hir_func_count = 0;
	hir_prepare_mode = HIR_PREPARE_NONE;
	arena_cleanup(&hir_arena);
}

/*
 * Gets the number of constructed functions.
 */
uint32_t
hir_get_function_count(
	void)
{
	return hir_func_count;
}

/*
 * Gets a constructed HIR function.
 */
struct hir_block *
hir_get_function(
	uint32_t index)
{
	struct hir_block *func;

	assert(index < hir_func_count);

	func = hir_func_tbl[index];

	return func;
}

/*
 * Replaces a function's link name.
 */
bool
hir_set_function_name(
	struct hir_block *func,
	const char *name)
{
	char *copy;

	assert(func != NULL);
	assert(func->type == HIR_BLOCK_FUNC);

	copy = hir_strdup(name);
	if (copy == NULL) {
		hir_out_of_memory();
		return false;
	}

	func->val.func.name = copy;

	return true;
}

/*
 * Gets the source file name.
 */
const char *
hir_get_file_name(
	void)
{
	assert(hir_file_name);

	return hir_file_name;
}

/*
 * Gets the current error line number.
 */
int
hir_get_error_line(
	void)
{
	return hir_error_line;
}

/*
 * Gets the current error message.
 */
const char *
hir_get_error_message(
	void)
{
	return hir_error_message;
}

/*
 * Records an HIR compilation error.
 */
void
hir_error(
	int line,
	const char *message)
{
	hir_error_line = line;
	snprintf(
		hir_error_message,
		sizeof(hir_error_message),
		"%s",
		message != NULL ? message : "HIR compilation failed.");
}

/*
 * Resolves an optional annotation and rejects an unknown name.
 */
bool
hir_resolve_type_annotation(
	int line,
	const char *type_name,
	bool allow_shape,
	int *tag,
	int *packed_type,
	bool *restricted)
{
	char base[64];
	char msg[256];
	const char *resolved_name;

	if (type_name == NULL) {
		*tag = -1;
		*packed_type = -1;
		*restricted = false;
		return true;
	}

	resolved_name = type_name;
	if (strchr(type_name, '(') != NULL) {
		if (!allow_shape ||
		    !hir_annotation_base(
			type_name,
			base,
			sizeof(base))) {
			hir_fatal(line, N_TR("Invalid shaped type annotation."));
			return false;
		}

		resolved_name = base;
	}

	if (!hir_resolve_type_name(
		resolved_name,
		tag,
		packed_type,
		restricted)) {
		snprintf(
			msg,
			sizeof(msg),
			N_TR("Unknown type name '%s'."),
			type_name);
		hir_fatal(line, msg);
		return false;
	}

	return true;
}

/*
 * Adds a local variable entry.
 */
bool
hir_add_local(
	struct hir_block *cur_block,
	const char *symbol)
{
	struct hir_block *func;
	struct hir_local *local;
	int index;

	/* Find the function containing the local. */
	func = cur_block;

	/* Walk from the current block to its function. */
	while (func->type != HIR_BLOCK_FUNC)
		func = func->parent;

	/* Reuse an existing local with the same symbol. */
	index = 0;
	local = func->val.func.local;

	/* Search every local in the function. */
	while (local != NULL) {
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
		return false;
	}

	local->index = index;
	/* -1 = unproven; NOT zero (NOCT_VALUE_INT == 0; see hir.h). */
	local->proven_type = -1;
	local->is_parameter = false;
	local->is_let = false;
	local->declaration_kind = HIR_LOCAL_DECL_UNKNOWN;
	local->declared_type = -1;
	local->declared_type_name = NULL;
	local->declared_scalar_kind = HIR_DECL_SCALAR_UNKNOWN;
	local->declared_packed_type = -1;
	local->storage_class = HIR_LOCAL_STORAGE_UNKNOWN;
	local->declaration_line = -1;
	local->declaration_stmt = NULL;
	local->initializer = NULL;
	local->next = func->val.func.local;
	func->val.func.local = local;

	return true;
}

/*
 * Records an out-of-memory error.
 */
void
hir_out_of_memory(
	void)
{
	snprintf(
		hir_error_message,
		sizeof(hir_error_message),
		"%s: Out of memory error.",
		hir_file_name != NULL ? hir_file_name : "");
}

/*
 * Allocates memory from the HIR arena.
 */
void *
hir_malloc(
	size_t size)
{
	return arena_alloc(&hir_arena, size);
}

/*
 * Duplicates a string in the HIR arena.
 */
char *
hir_strdup(
	const char *s)
{
	char *ret;
	size_t len;

	len = strlen(s) + 1;
	ret = arena_alloc(&hir_arena, len);
	if (ret == NULL)
		return NULL;

	memcpy(ret, s, len);

	return ret;
}

/*
 * Allocates a fresh HIR block identifier.
 */
int
hir_next_block_id(
	void)
{
	return block_id_top++;
}

/*
 * Optimizes one HIR function.
 */
bool
hir_optimize_func(
	struct hir_block *func_block,
	int optimize_level,
	bool print_simd_info,
	bool (*accel_optimize_func)(struct hir_block *func_block,
				    void *userdata),
	void *accel_optimize_userdata)
{
#if !defined(NOCT_USE_OPTIMIZER)
	UNUSED_PARAMETER(print_simd_info);
	UNUSED_PARAMETER(accel_optimize_func);
	UNUSED_PARAMETER(accel_optimize_userdata);
#endif

	assert(func_block != NULL);
	assert(func_block->type == HIR_BLOCK_FUNC);
	assert((accel_optimize_func == NULL &&
		accel_optimize_userdata == NULL) ||
	       (accel_optimize_func != NULL &&
		accel_optimize_userdata != NULL));

	if (!hir_prepare_module(optimize_level))
		return false;

#if !defined(NOCT_USE_OPTIMIZER)
	return true;
#else
	if (getenv("NOCT_PARALLEL_DEBUG") != NULL) {
		if (!hir_parallel_diagnose_func(
			func_block,
			stderr,
			"parallel-analysis"))
			return false;
	}

	if (optimize_level >= 1) {
		/* Function inlining. */
		if (!hir_opt_inline_func(func_block))
			return false;

		/* Typed ops. */
		if (!hir_opt_typed_func(func_block))
			return false;

		/* Offer marked functions to the optional accelerator optimizer. */
		if (func_block->val.func.is_accel &&
		    accel_optimize_func != NULL) {
			if (!accel_optimize_func(
				func_block,
				accel_optimize_userdata))
				return false;

			/* A consumed hint denotes a committed accelerator rewrite. */
			if (!func_block->val.func.is_accel) {
				if (!hir_opt_typed_func(func_block))
					return false;

				return true;
			}
		}

		/* Remove only statically proven fast index checks. */
		if (!hir_fast_func_pass(func_block))
			return false;
		if (func_block->val.func.is_fast)
			func_block->val.func.fast_optimized = true;
	}

	if (optimize_level >= 2) {
		/* ABCE. */
		if (!hir_opt_abce_func(func_block))
			return false;

		/* SIMD vectorization. */
		if (!hir_opt_simd_func(func_block, print_simd_info))
			return false;

		/* Loop unrolling. */
		if (!hir_opt_unroll_func(func_block))
			return false;
	}

	if (optimize_level >= 1) {
		/* CSE. */
		if (!hir_opt_cse_func(func_block))
			return false;

		/* After CSE: the lattice must see CAPTURE home assignments. */
		if (!hir_opt_typed_func(func_block))
			return false;
	}

	return true;
#endif
}

/*
 * Dumps an HIR block tree.
 */
void
hir_dump_block(
	struct hir_block *block)
{
	hir_dump_block_at_level(block, 0);
}

/* Select ordinary checked lowering or optimizer-owned fast validation. */
static bool
hir_prepare_module(
	int optimize_level)
{
	enum hir_prepare_mode requested_mode;

#if defined(NOCT_USE_OPTIMIZER)
	requested_mode = optimize_level >= 1 ?
		HIR_PREPARE_OPTIMIZER : HIR_PREPARE_CHECKED;
#else
	UNUSED_PARAMETER(optimize_level);

	requested_mode = HIR_PREPARE_CHECKED;
#endif

	if (hir_prepare_mode == requested_mode)
		return true;
	if (hir_prepare_mode != HIR_PREPARE_NONE) {
		hir_error(
			0,
			N_TR("Inconsistent optimization levels for one HIR module."));
		return false;
	}

#if defined(NOCT_USE_OPTIMIZER)
	if (requested_mode == HIR_PREPARE_OPTIMIZER) {
		if (!hir_fast_checked_module(hir_func_tbl, hir_func_count))
			return false;

		hir_prepare_mode = requested_mode;
		return true;
	}
#endif

	if (!hir_lower_checked_fast_syntax())
		return false;
	if (!hir_add_checked_fast_prologues())
		return false;

	hir_prepare_mode = requested_mode;

	return true;
}

/* Lower source fast conveniences that must remain safe without an optimizer. */
static bool
hir_lower_checked_fast_syntax(
	void)
{
	struct hir_block *func;
	uint32_t i;

	/* Lower every source fast body independently. */
	for (i = 0; i < hir_func_count; i++) {
		func = hir_func_tbl[i];
		if (!func->val.func.is_fast)
			continue;
		if (!hir_lower_checked_fast_chain(func, func->val.func.inner))
			return false;
	}

	return true;
}

/* Lower residual checked syntax in one structured block chain. */
static bool
hir_lower_checked_fast_chain(
	struct hir_block *func,
	struct hir_block *head)
{
	struct hir_block *block;
	struct hir_block *branch;
	struct hir_stmt *stmt;

	block = head;

	/* Walk every block until this structured chain stops. */
	while (block != NULL) {
		/* Lower the expressions owned by this block shape. */
		switch (block->type) {
		case HIR_BLOCK_BASIC:
			stmt = block->val.basic.stmt_list;

			/* Lower every statement expression in source order. */
			while (stmt != NULL) {
				if (!hir_lower_checked_fast_expr(
					func,
					&stmt->lhs,
					stmt->line)) {
					return false;
				}
				if (!hir_lower_checked_fast_expr(
					func,
					&stmt->rhs,
					stmt->line)) {
					return false;
				}
				stmt = stmt->next;
			}
			break;
		case HIR_BLOCK_IF:
			branch = block;

			/* Lower every arm of the conditional chain. */
			while (branch != NULL) {
				if (!hir_lower_checked_fast_expr(
					func,
					&branch->val.if_.cond,
					branch->line)) {
					return false;
				}
				if (!hir_lower_checked_fast_chain(
					func,
					branch->val.if_.inner)) {
					return false;
				}
				branch = branch->val.if_.chain_next;
			}
			break;
		case HIR_BLOCK_FOR:
			if (!hir_lower_checked_fast_expr(
				func,
				&block->val.for_.start,
				block->line)) {
				return false;
			}
			if (!hir_lower_checked_fast_expr(
				func,
				&block->val.for_.stop,
				block->line)) {
				return false;
			}
			if (!hir_lower_checked_fast_expr(
				func,
				&block->val.for_.collection,
				block->line)) {
				return false;
			}
			if (!hir_lower_checked_fast_chain(
				func,
				block->val.for_.inner)) {
				return false;
			}
			break;
		case HIR_BLOCK_WHILE:
			if (!hir_lower_checked_fast_expr(
				func,
				&block->val.while_.cond,
				block->line)) {
				return false;
			}
			if (!hir_lower_checked_fast_chain(
				func,
				block->val.while_.inner)) {
				return false;
			}
			break;
		case HIR_BLOCK_END:
			return true;
		default:
			assert(NEVER_COME_HERE);
			return false;
		}

		if (block->stop)
			break;
		block = block->succ;
	}

	return true;
}

/* Lower residual checked syntax in one expression tree. */
static bool
hir_lower_checked_fast_expr(
	struct hir_block *func,
	struct hir_expr **slot,
	int line)
{
	struct hir_expr *expr;
	struct hir_term *term;
	const char *name;
	uint32_t i;

	if (slot == NULL || *slot == NULL)
		return true;

	expr = *slot;

	/* Recurse according to the expression's owned children. */
	switch (expr->type) {
	case HIR_EXPR_TERM:
		return true;
	case HIR_EXPR_NEG:
	case HIR_EXPR_NOT:
	case HIR_EXPR_PAR:
	case HIR_EXPR_PBASE:
	case HIR_EXPR_PLEN:
		return hir_lower_checked_fast_expr(
			func,
			&expr->val.unary.expr,
			line);
	case HIR_EXPR_CAPTURE:
		return hir_lower_checked_fast_expr(
			func,
			&expr->val.capture.expr,
			line);
	case HIR_EXPR_DOT:
		return hir_lower_checked_fast_expr(
			func,
			&expr->val.dot.obj,
			line);
	case HIR_EXPR_SUBSCR:
		if (!hir_lower_checked_fast_expr(
			func,
			&expr->val.binary.expr[0],
			line)) {
			return false;
		}
		if (!hir_lower_checked_fast_expr(
			func,
			&expr->val.binary.expr[1],
			line)) {
			return false;
		}
		if (expr->val.binary.expr[1] != NULL &&
		    expr->val.binary.expr[1]->type == HIR_EXPR_ARRAY &&
		    expr->val.binary.expr[1]->val.array.is_multi_index) {
			return hir_lower_checked_fast_multi(func, expr, line);
		}

		return true;
	case HIR_EXPR_CALL:
		if (!hir_lower_checked_fast_expr(
			func,
			&expr->val.call.func,
			line)) {
			return false;
		}

		/* Lower every eagerly evaluated call argument. */
		for (i = 0; i < expr->val.call.arg_count; i++) {
			if (!hir_lower_checked_fast_expr(
				func,
				&expr->val.call.arg[i],
				line)) {
				return false;
			}
		}

		if (expr->val.call.func == NULL ||
		    expr->val.call.func->type != HIR_EXPR_TERM)
			return true;
		term = expr->val.call.func->val.term.term;
		if (term == NULL || term->type != HIR_TERM_SYMBOL)
			return true;
		name = term->val.symbol;
		if (!hir_is_fast_intrinsic_name(name) ||
		    hir_has_function_name(name)) {
			return true;
		}

		return hir_rewrite_fast_intrinsic(expr, name);
	case HIR_EXPR_THISCALL:
		if (!hir_lower_checked_fast_expr(
			func,
			&expr->val.thiscall.obj,
			line)) {
			return false;
		}

		/* Lower every eagerly evaluated method argument. */
		for (i = 0; i < expr->val.thiscall.arg_count; i++) {
			if (!hir_lower_checked_fast_expr(
				func,
				&expr->val.thiscall.arg[i],
				line)) {
				return false;
			}
		}

		return true;
	case HIR_EXPR_ARRAY:
		/* Lower every eagerly evaluated array element. */
		for (i = 0; i < expr->val.array.elem_count; i++) {
			if (!hir_lower_checked_fast_expr(
				func,
				&expr->val.array.elem[i],
				line)) {
				return false;
			}
		}

		return true;
	case HIR_EXPR_DICT:
		/* Lower every eagerly evaluated dictionary value. */
		for (i = 0; i < expr->val.dict.kv_count; i++) {
			if (!hir_lower_checked_fast_expr(
				func,
				&expr->val.dict.value[i],
				line)) {
				return false;
			}
		}

		return true;
	case HIR_EXPR_NEW:
		return hir_lower_checked_fast_expr(
			func,
			&expr->val.new_.init,
			line);
	case HIR_EXPR_SELECT:
		if (!hir_lower_checked_fast_expr(
			func,
			&expr->val.select.cond,
			line)) {
			return false;
		}
		if (!hir_lower_checked_fast_expr(
			func,
			&expr->val.select.if_true,
			line)) {
			return false;
		}

		return hir_lower_checked_fast_expr(
			func,
			&expr->val.select.if_false,
			line);
	case HIR_EXPR_PMASKSTORE32:
		if (!hir_lower_checked_fast_expr(
			func,
			&expr->val.mask_store.base,
			line)) {
			return false;
		}
		if (!hir_lower_checked_fast_expr(
			func,
			&expr->val.mask_store.offset,
			line)) {
			return false;
		}

		return hir_lower_checked_fast_expr(
			func,
			&expr->val.mask_store.mask,
			line);
	case HIR_EXPR_PGATHER32:
		if (!hir_lower_checked_fast_expr(
			func,
			&expr->val.gather.base,
			line)) {
			return false;
		}
		if (!hir_lower_checked_fast_expr(
			func,
			&expr->val.gather.length,
			line)) {
			return false;
		}
		if (!hir_lower_checked_fast_expr(
			func,
			&expr->val.gather.index,
			line)) {
			return false;
		}

		return hir_lower_checked_fast_expr(
			func,
			&expr->val.gather.packed,
			line);
	default:
		if (!hir_lower_checked_fast_expr(
			func,
			&expr->val.binary.expr[0],
			line)) {
			return false;
		}

		return hir_lower_checked_fast_expr(
			func,
			&expr->val.binary.expr[1],
			line);
	}
}

/* Lower one direct shaped-parameter multi-index to a checked helper call. */
static bool
hir_lower_checked_fast_multi(
	struct hir_block *func,
	struct hir_expr *subscript,
	int line)
{
	struct hir_expr *extent[HIR_FAST_RANK_MAX];
	struct hir_expr *array;
	struct hir_expr *call;
	struct hir_expr *dot;
	char helper[16];
	uint32_t rank;
	uint32_t i;
	int param_index;

	assert(func != NULL);
	assert(subscript != NULL);
	assert(subscript->type == HIR_EXPR_SUBSCR);

	array = subscript->val.binary.expr[1];
	param_index = hir_find_shaped_param(
		func,
		subscript->val.binary.expr[0]);
	if (param_index < 0) {
		hir_error(
			line,
			N_TR("A multi-dimensional subscript requires a directly named shaped parameter."));
		return false;
	}

	memset(extent, 0, sizeof(extent));
	if (!hir_parse_shape_extent_exprs(
		func,
		func->val.func.param_type_name[param_index],
		extent,
		&rank)) {
		return false;
	}
	if (array->val.array.elem_count != rank) {
		hir_error(
			line,
			N_TR("The number of indices does not match the __fast parameter rank."));
		return false;
	}

	dot = hir_malloc(sizeof(*dot));
	if (dot == NULL) {
		hir_out_of_memory();
		return false;
	}

	memset(dot, 0, sizeof(*dot));
	dot->type = HIR_EXPR_DOT;
	dot->val.dot.obj = hir_make_symbol_expr("$Fast");
	if (dot->val.dot.obj == NULL)
		return false;
	snprintf(helper, sizeof(helper), "index%u", rank);
	dot->val.dot.symbol = hir_strdup(helper);
	if (dot->val.dot.symbol == NULL) {
		hir_out_of_memory();
		return false;
	}

	call = hir_malloc(sizeof(*call));
	if (call == NULL) {
		hir_out_of_memory();
		return false;
	}

	memset(call, 0, sizeof(*call));
	call->type = HIR_EXPR_CALL;
	call->val.call.func = dot;
	call->val.call.arg_count = rank * 2;

	/* Interleave each source index with its exact runtime extent. */
	for (i = 0; i < rank; i++) {
		call->val.call.arg[i * 2] = array->val.array.elem[i];
		call->val.call.arg[i * 2 + 1] = extent[i];
	}

	subscript->val.binary.expr[1] = call;

	return true;
}

/* Replace one plain numeric intrinsic with its immutable internal package. */
static bool
hir_rewrite_fast_intrinsic(
	struct hir_expr *call,
	const char *name)
{
	struct hir_expr *dot;

	dot = hir_malloc(sizeof(*dot));
	if (dot == NULL) {
		hir_out_of_memory();
		return false;
	}

	memset(dot, 0, sizeof(*dot));
	dot->type = HIR_EXPR_DOT;
	dot->val.dot.obj = hir_make_symbol_expr("$FastMath");
	if (dot->val.dot.obj == NULL)
		return false;
	dot->val.dot.symbol = hir_strdup(name);
	if (dot->val.dot.symbol == NULL) {
		hir_out_of_memory();
		return false;
	}

	call->val.call.func = dot;

	return true;
}

/* Return whether one plain source name denotes a checked numeric intrinsic. */
static bool
hir_is_fast_intrinsic_name(
	const char *name)
{
	size_t i;

	/* Search the stable internal intrinsic table. */
	for (i = 0;
	     i < sizeof(hir_fast_intrinsic_names) /
		     sizeof(hir_fast_intrinsic_names[0]);
	     i++) {
		if (strcmp(name, hir_fast_intrinsic_names[i]) == 0)
			return true;
	}

	return false;
}

/* Return whether the current source file declares one direct function name. */
static bool
hir_has_function_name(
	const char *name)
{
	uint32_t i;

	/* Search every constructed source function. */
	for (i = 0; i < hir_func_count; i++) {
		if (strcmp(hir_func_tbl[i]->val.func.name, name) == 0)
			return true;
	}

	return false;
}

/* Find the directly named shaped parameter used as a subscript base. */
static int
hir_find_shaped_param(
	const struct hir_block *func,
	const struct hir_expr *base)
{
	const struct hir_term *term;
	const char *symbol;
	const char *annotation;
	uint32_t i;

	if (base == NULL || base->type != HIR_EXPR_TERM)
		return -1;
	term = base->val.term.term;
	if (term == NULL || term->type != HIR_TERM_SYMBOL)
		return -1;
	symbol = term->val.symbol;

	/* Find a shaped parameter with this scope-resolved name. */
	for (i = 0; i < func->val.func.param_count; i++) {
		if (strcmp(func->val.func.param_name[i], symbol) != 0)
			continue;
		annotation = func->val.func.param_type_name[i];
		if (annotation == NULL || strchr(annotation, '(') == NULL)
			return -1;
		if (func->val.func.param_type[i] != NOCT_VALUE_PACKED)
			return -1;

		return (int)i;
	}

	return -1;
}

/* Add ordinary exact-shape entry checks to every source fast function. */
static bool
hir_add_checked_fast_prologues(
	void)
{
	uint32_t i;

	/* Process every constructed function after checked lowering. */
	for (i = 0; i < hir_func_count; i++) {
		if (!hir_add_checked_fast_prologue(hir_func_tbl[i]))
			return false;
	}

	return true;
}

/* Prepend the exact-shape checks required by one source signature. */
static bool
hir_add_checked_fast_prologue(
	struct hir_block *func)
{
	struct hir_block *prologue;
	struct hir_stmt *first;
	struct hir_stmt *last;
	uint32_t i;

	assert(func != NULL);
	assert(func->type == HIR_BLOCK_FUNC);

	if (!func->val.func.is_fast)
		return true;

	first = NULL;
	last = NULL;

	/* Build one ordinary checked call for every shaped parameter. */
	for (i = 0; i < func->val.func.param_count; i++) {
		struct hir_stmt *stmt;
		const char *annotation;

		annotation = func->val.func.param_type_name[i];
		if (annotation == NULL || strchr(annotation, '(') == NULL)
			continue;
		if (func->val.func.param_type[i] != NOCT_VALUE_PACKED) {
			hir_error(
				func->line,
				N_TR("A shaped __fast parameter must be a Packed type."));
			return false;
		}
		if (!hir_build_shape_check_stmt(func, i, &stmt))
			return false;

		if (first == NULL)
			first = stmt;
		else
			last->next = stmt;
		last = stmt;
	}

	if (first == NULL)
		return true;

	prologue = hir_malloc(sizeof(*prologue));
	if (prologue == NULL) {
		hir_out_of_memory();
		return false;
	}

	memset(prologue, 0, sizeof(*prologue));
	prologue->id = hir_next_block_id();
	prologue->type = HIR_BLOCK_BASIC;
	prologue->line = func->line;
	prologue->parent = func;
	prologue->succ = func->val.func.inner != NULL ?
		func->val.func.inner : func->succ;
	prologue->stop = func->val.func.inner == NULL;
	prologue->val.basic.stmt_list = first;
	func->val.func.inner = prologue;

	return true;
}
/* Build one call to the immutable internal exact-shape helper. */
static bool
hir_build_shape_check_stmt(
	struct hir_block *func,
	uint32_t param_index,
	struct hir_stmt **stmt_ret)
{
	struct hir_expr *extent[HIR_FAST_RANK_MAX];
	struct hir_expr *call;
	struct hir_expr *dot;
	struct hir_stmt *stmt;
	char helper[16];
	uint32_t rank;
	uint32_t i;

	assert(func != NULL);
	assert(func->type == HIR_BLOCK_FUNC);
	assert(param_index < func->val.func.param_count);
	assert(stmt_ret != NULL);

	memset(extent, 0, sizeof(extent));
	if (!hir_parse_shape_extent_exprs(
		func,
		func->val.func.param_type_name[param_index],
		extent,
		&rank)) {
		return false;
	}

	dot = hir_malloc(sizeof(*dot));
	if (dot == NULL) {
		hir_out_of_memory();
		return false;
	}

	memset(dot, 0, sizeof(*dot));
	dot->type = HIR_EXPR_DOT;
	dot->val.dot.obj = hir_make_symbol_expr("$Fast");
	if (dot->val.dot.obj == NULL)
		return false;
	snprintf(helper, sizeof(helper), "shape%u", rank);
	dot->val.dot.symbol = hir_strdup(helper);
	if (dot->val.dot.symbol == NULL) {
		hir_out_of_memory();
		return false;
	}

	call = hir_malloc(sizeof(*call));
	if (call == NULL) {
		hir_out_of_memory();
		return false;
	}

	memset(call, 0, sizeof(*call));
	call->type = HIR_EXPR_CALL;
	call->val.call.func = dot;
	call->val.call.arg_count = rank + 1;
	call->val.call.arg[0] = hir_make_symbol_expr(
		func->val.func.param_name[param_index]);
	if (call->val.call.arg[0] == NULL)
		return false;

	/* Copy each parsed extent after the Packed argument. */
	for (i = 0; i < rank; i++)
		call->val.call.arg[i + 1] = extent[i];

	stmt = hir_malloc(sizeof(*stmt));
	if (stmt == NULL) {
		hir_out_of_memory();
		return false;
	}

	memset(stmt, 0, sizeof(*stmt));
	stmt->line = func->line;
	stmt->rhs = call;
	*stmt_ret = stmt;

	return true;
}

/* Parse one exact shape into ordinary HIR extent expressions. */
static bool
hir_parse_shape_extent_exprs(
	struct hir_block *func,
	const char *annotation,
	struct hir_expr *extent[HIR_FAST_RANK_MAX],
	uint32_t *rank_ret)
{
	const char *cursor;
	const char *close;
	const char *item;
	size_t length;
	uint32_t rank;
	int decimal_result;
	int param_index;
	int64_t constant;

	assert(func != NULL);
	assert(annotation != NULL);
	assert(extent != NULL);
	assert(rank_ret != NULL);

	cursor = strchr(annotation, '(');
	if (cursor == NULL) {
		hir_error(func->line, N_TR("Invalid __fast parameter shape."));
		return false;
	}

	close = strrchr(cursor + 1, ')');
	if (close == NULL ||
	    close[1] != '\0' ||
	    cursor + 1 == close) {
		hir_error(func->line, N_TR("Invalid __fast parameter shape."));
		return false;
	}

	cursor++;
	rank = 0;

	/* Parse every comma-separated extent in source order. */
	while (cursor < close) {
		if (rank >= HIR_FAST_RANK_MAX) {
			hir_error(
				func->line,
				N_TR("A __fast parameter shape has more than 8 dimensions."));
			return false;
		}

		item = cursor;

		/* Find the end of this extent spelling. */
		while (cursor < close && *cursor != ',')
			cursor++;

		length = (size_t)(cursor - item);
		if (length == 0) {
			hir_error(
				func->line,
				N_TR("Invalid empty __fast shape extent."));
			return false;
		}

		if (item[0] >= '0' && item[0] <= '9') {
			decimal_result = hir_parse_positive_decimal(
				item,
				length,
				&constant);
			if (decimal_result == HIR_DECIMAL_OVERFLOW) {
				hir_error(
					func->line,
					N_TR("__fast shape extent is too large."));
				return false;
			}
			if (decimal_result == HIR_DECIMAL_ZERO) {
				hir_error(
					func->line,
					N_TR("A __fast shape extent must be positive."));
				return false;
			}
			if (decimal_result != HIR_DECIMAL_OK) {
				hir_error(
					func->line,
					N_TR("Invalid __fast shape extent."));
				return false;
			}

			extent[rank] = hir_make_integer_expr(constant);
		} else {
			if (!hir_is_identifier(item, length)) {
				hir_error(
					func->line,
					N_TR("Invalid __fast shape extent."));
				return false;
			}

			param_index = hir_find_param_index(func, item, length);
			if (param_index < 0 ||
			    (func->val.func.param_type[param_index] != NOCT_VALUE_INT &&
			     func->val.func.param_type[param_index] != NOCT_VALUE_LONG)) {
				hir_error(
					func->line,
					N_TR("A dynamic __fast shape extent must name an int or long parameter."));
				return false;
			}

			extent[rank] = hir_make_symbol_expr(
				func->val.func.param_name[param_index]);
		}

		if (extent[rank] == NULL)
			return false;
		rank++;

		if (cursor < close) {
			cursor++;
			if (cursor == close) {
				hir_error(
					func->line,
					N_TR("Invalid empty __fast shape extent."));
				return false;
			}
		}
	}

	*rank_ret = rank;

	return true;
}

/* Parse one positive decimal with explicit signed-64-bit bounds. */
static int
hir_parse_positive_decimal(
	const char *text,
	size_t length,
	int64_t *value)
{
	uint64_t accumulated;
	uint64_t limit;
	uint64_t digit;
	size_t i;
	char ch;

	if (length == 0)
		return HIR_DECIMAL_INVALID;

	accumulated = 0;
	limit = (uint64_t)INT64_MAX;

	/* Accumulate every digit without relying on libc overflow behavior. */
	for (i = 0; i < length; i++) {
		ch = text[i];
		if (ch < '0' || ch > '9')
			return HIR_DECIMAL_INVALID;

		digit = (uint64_t)(unsigned int)(ch - '0');
		if (accumulated > (limit - digit) / 10U)
			return HIR_DECIMAL_OVERFLOW;

		accumulated = accumulated * 10U + digit;
	}

	if (accumulated == 0)
		return HIR_DECIMAL_ZERO;

	*value = (int64_t)accumulated;

	return HIR_DECIMAL_OK;
}

/* Find one complete parameter spelling. */
static int
hir_find_param_index(
	const struct hir_block *func,
	const char *text,
	size_t length)
{
	uint32_t i;

	/* Search every declared parameter. */
	for (i = 0; i < func->val.func.param_count; i++) {
		if (strlen(func->val.func.param_name[i]) != length)
			continue;
		if (strncmp(func->val.func.param_name[i], text, length) == 0)
			return (int)i;
	}

	return -1;
}

/* Test whether one shape item is an ASCII identifier. */
static bool
hir_is_identifier(
	const char *text,
	size_t length)
{
	size_t i;
	char ch;

	if (length == 0)
		return false;

	ch = text[0];
	if (!((ch >= 'a' && ch <= 'z') ||
	      (ch >= 'A' && ch <= 'Z') ||
	      ch == '_')) {
		return false;
	}

	/* Check every remaining identifier character. */
	for (i = 1; i < length; i++) {
		ch = text[i];
		if (!((ch >= 'a' && ch <= 'z') ||
		      (ch >= 'A' && ch <= 'Z') ||
		      (ch >= '0' && ch <= '9') ||
		      ch == '_')) {
			return false;
		}
	}

	return true;
}

/* Construct one symbol expression in the HIR arena. */
static struct hir_expr *
hir_make_symbol_expr(
	const char *symbol)
{
	struct hir_expr *expr;
	struct hir_term *term;

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

	expr = hir_malloc(sizeof(*expr));
	if (expr == NULL) {
		hir_out_of_memory();
		return NULL;
	}

	memset(expr, 0, sizeof(*expr));
	expr->type = HIR_EXPR_TERM;
	expr->val.term.term = term;

	return expr;
}

/* Construct one signed integer expression in the HIR arena. */
static struct hir_expr *
hir_make_integer_expr(
	int64_t value)
{
	struct hir_expr *expr;
	struct hir_term *term;

	term = hir_malloc(sizeof(*term));
	if (term == NULL) {
		hir_out_of_memory();
		return NULL;
	}

	memset(term, 0, sizeof(*term));
	if (value <= INT_MAX) {
		term->type = HIR_TERM_INT;
		term->val.i = (int)value;
	} else {
		term->type = HIR_TERM_LONG;
		term->val.l = value;
	}

	expr = hir_malloc(sizeof(*expr));
	if (expr == NULL) {
		hir_out_of_memory();
		return NULL;
	}

	memset(expr, 0, sizeof(*expr));
	expr->type = HIR_EXPR_TERM;
	expr->val.term.term = term;

	return expr;
}

/* Start scope processing for one function. */
static void
hir_scope_begin_func(
	void)
{
	hir_scope_top = NULL;
	hir_scope_seq = 0;
}

/* Push one lexical block and pre-scan its declarations. */
static bool
hir_scope_push(
	const struct ast_stmt_list *stmt_list)
{
	struct hir_scope *scope;
	const struct ast_stmt *stmt;
	const struct ast_expr *lhs;

	scope = hir_malloc(sizeof(struct hir_scope));
	if (scope == NULL) {
		hir_out_of_memory();
		return false;
	}

	memset(scope, 0, sizeof(struct hir_scope));
	scope->parent = hir_scope_top;
	hir_scope_top = scope;

	stmt = stmt_list != NULL ? stmt_list->list : NULL;

	/* Pre-register every declaration in the lexical block. */
	while (stmt != NULL) {
		if (stmt->type == AST_STMT_ASSIGN &&
		    (stmt->val.assign.is_var || stmt->val.assign.is_let)) {
			lhs = stmt->val.assign.lhs;
			if (lhs != NULL &&
			    lhs->type == AST_EXPR_TERM &&
			    lhs->val.term.term != NULL &&
			    lhs->val.term.term->type == AST_TERM_SYMBOL) {
				if (!hir_scope_add_decl(
					stmt->line,
					lhs->val.term.term->val.symbol,
					false,
					stmt->val.assign.is_let,
					NULL)) {
					hir_scope_top = scope->parent;
					return false;
				}
			}
		}
		stmt = stmt->next;
	}

	return true;
}

/* Pop the current lexical block. */
static void
hir_scope_pop(
	void)
{
	assert(hir_scope_top != NULL);

	hir_scope_top = hir_scope_top->parent;
}

/* Register one already-declared function parameter. */
static bool
hir_scope_add_param(
	int line,
	const char *src_name)
{
	struct hir_scope_decl *decl;

	if (!hir_scope_add_decl(line, src_name, true, false, &decl))
		return false;

	decl->int_name = decl->src_name;

	return true;
}

/* Begin one declaration in the current lexical block. */
static bool
hir_scope_declare(
	int line,
	const char *src_name,
	bool is_let,
	const char **int_name,
	struct hir_scope_decl **decl_ret)
{
	struct hir_scope_decl *decl;
	char msg[256];

	assert(hir_scope_top != NULL);
	assert(src_name != NULL);
	assert(int_name != NULL);
	assert(decl_ret != NULL);

	decl = hir_scope_find_here(hir_scope_top, src_name);
	if (decl == NULL) {
		if (!hir_scope_add_decl(
			line,
			src_name,
			false,
			is_let,
			&decl)) {
			return false;
		}
	}

	if (decl->declared) {
		snprintf(
			msg,
			sizeof(msg),
			N_TR("Variable '%s' is already declared in this scope."),
			src_name);
		hir_error(line, msg);
		return false;
	}

	if (!hir_scope_intern(decl))
		return false;

	decl->is_let = is_let;
	*int_name = decl->int_name;
	*decl_ret = decl;

	return true;
}

/* Make one declaration visible and end its TDZ. */
static void
hir_scope_mark_declared(
	struct hir_scope_decl *decl)
{
	assert(decl != NULL);
	assert(!decl->declared);

	decl->declared = true;
}

/* Resolve one source name to its internal name. */
static bool
hir_scope_resolve(
	int line,
	const char *src_name,
	const char **int_name)
{
	struct hir_scope_decl *decl;
	char msg[256];

	assert(src_name != NULL);
	assert(int_name != NULL);

	*int_name = NULL;
	if (src_name[0] == '$')
		return true;

	decl = hir_scope_find(src_name);
	if (decl == NULL)
		return true;

	if (!decl->declared) {
		snprintf(
			msg,
			sizeof(msg),
			N_TR("Variable '%s' is used before its declaration."),
			src_name);
		hir_error(line, msg);
		return false;
	}

	*int_name = decl->int_name;

	return true;
}

/* Reject assignment to an immutable binding. */
static bool
hir_scope_check_assign(
	int line,
	const char *int_name)
{
	struct hir_scope *scope;
	struct hir_scope_decl *decl;
	char msg[256];

	assert(int_name != NULL);

	scope = hir_scope_top;

	/* Search every active lexical scope. */
	while (scope != NULL) {
		decl = scope->decls;

		/* Search every declaration in this scope. */
		while (decl != NULL) {
			if (decl->int_name != NULL &&
			    strcmp(decl->int_name, int_name) == 0) {
				if (decl->is_let) {
					snprintf(
						msg,
						sizeof(msg),
						N_TR("Cannot assign to 'let' variable '%s'."),
						decl->src_name);
					hir_error(line, msg);
					return false;
				}
				return true;
			}
			decl = decl->next;
		}
		scope = scope->parent;
	}

	return true;
}

/* Find one declaration in the selected lexical scope. */
static struct hir_scope_decl *
hir_scope_find_here(
	const struct hir_scope *scope,
	const char *src_name)
{
	struct hir_scope_decl *decl;

	if (scope == NULL)
		return NULL;

	decl = scope->decls;

	/* Search declarations in the selected scope. */
	while (decl != NULL) {
		if (strcmp(decl->src_name, src_name) == 0)
			return decl;
		decl = decl->next;
	}

	return NULL;
}

/* Find one declaration from the innermost active scope outward. */
static struct hir_scope_decl *
hir_scope_find(
	const char *src_name)
{
	struct hir_scope *scope;
	struct hir_scope_decl *decl;

	scope = hir_scope_top;

	/* Search from the innermost scope outward. */
	while (scope != NULL) {
		decl = hir_scope_find_here(scope, src_name);
		if (decl != NULL)
			return decl;
		scope = scope->parent;
	}

	return NULL;
}

/* Add one unresolved declaration to the current lexical scope. */
static bool
hir_scope_add_decl(
	int line,
	const char *src_name,
	bool declared,
	bool is_let,
	struct hir_scope_decl **decl_ret)
{
	struct hir_scope_decl *decl;
	char msg[256];

	assert(hir_scope_top != NULL);
	assert(src_name != NULL);

	if (hir_scope_find_here(hir_scope_top, src_name) != NULL) {
		snprintf(
			msg,
			sizeof(msg),
			N_TR("Variable '%s' is already declared in this scope."),
			src_name);
		hir_error(line, msg);
		return false;
	}

	decl = hir_malloc(sizeof(struct hir_scope_decl));
	if (decl == NULL) {
		hir_out_of_memory();
		return false;
	}

	memset(decl, 0, sizeof(struct hir_scope_decl));
	decl->src_name = hir_strdup(src_name);
	if (decl->src_name == NULL) {
		hir_out_of_memory();
		return false;
	}
	decl->declared = declared;
	decl->is_let = is_let;
	decl->next = hir_scope_top->decls;
	hir_scope_top->decls = decl;

	if (decl_ret != NULL)
		*decl_ret = decl;

	return true;
}

/* Assign one flat HIR-local name to a lexical declaration. */
static bool
hir_scope_intern(
	struct hir_scope_decl *decl)
{
	char suffix[32];
	size_t name_size;

	assert(hir_scope_top != NULL);
	assert(decl != NULL);

	/*
	 * Function-scope declarations retain their source names. Inner
	 * declarations are renamed unconditionally because HIR keeps a flat
	 * function-local list; the renamed binding must not remain visible
	 * after its lexical scope is popped.
	 */
	if (hir_scope_top->parent == NULL) {
		decl->int_name = decl->src_name;
		return true;
	}

	hir_scope_seq++;
	snprintf(suffix, sizeof(suffix), "$%d", hir_scope_seq);
	name_size = strlen(decl->src_name) + strlen(suffix) + 1;
	decl->int_name = hir_malloc(name_size);
	if (decl->int_name == NULL) {
		hir_out_of_memory();
		return false;
	}
	snprintf(decl->int_name, name_size, "%s%s", decl->src_name, suffix);

	return true;
}

/* Test whether every syntactic path in one statement list returns. */
static bool
hir_fast_stmt_list_returns(
	const struct ast_stmt_list *list)
{
	const struct ast_stmt *stmt;

	stmt = list != NULL ? list->list : NULL;

	/* Find a return or a complete returning conditional chain. */
	while (stmt != NULL) {
		if (stmt->type == AST_STMT_RETURN)
			return true;

		if (stmt->type == AST_STMT_IF) {
			const struct ast_stmt *branch = stmt->next;
			bool all_return = hir_fast_stmt_list_returns(
				stmt->val.if_.stmt_list);
			bool has_else = false;

			/* Check every directly following elif and else branch. */
			while (branch != NULL &&
			       (branch->type == AST_STMT_ELIF ||
				branch->type == AST_STMT_ELSE)) {
				if (branch->type == AST_STMT_ELIF) {
					all_return = all_return &&
						hir_fast_stmt_list_returns(
							branch->val.elif.stmt_list);
				} else {
					has_else = true;
					all_return = all_return &&
						hir_fast_stmt_list_returns(
							branch->val.else_.stmt_list);
				}

				branch = branch->next;
			}

			if (all_return && has_else)
				return true;
		}

		stmt = stmt->next;
	}

	return false;
}

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
	/* Construct the function through one cleanup-controlled attempt. */
	do {
		/* Set a func name. */
		func_block->val.func.name = hir_strdup(afunc->name);
		if (func_block->val.func.name == NULL) {
			hir_out_of_memory();
			break;
		}
		func_block->val.func.is_static = afunc->is_static;
		func_block->val.func.is_inline = afunc->is_inline;
		func_block->val.func.is_fast = afunc->is_fast;
		func_block->val.func.is_accel = afunc->is_accel;
		func_block->val.func.fast_optimized = false;
		func_block->val.func.returns_on_all_paths =
			hir_fast_stmt_list_returns(afunc->stmt_list);
		func_block->val.func.fast_info = NULL;
		if (afunc->return_type_name != NULL) {
			func_block->val.func.return_type_name =
				hir_strdup(afunc->return_type_name);
			if (func_block->val.func.return_type_name == NULL) {
				hir_out_of_memory();
				break;
			}
		}

		/* Parse the parameters. */
		if (!hir_visit_param_list(func_block, afunc))
			break;

		/*
		 * Resolve the optional return type. Restrict is an input alias
		 * contract and is meaningless on a returned value.
		 */
		{
			bool return_restricted;

			if (!hir_resolve_type_annotation(
				0,
				afunc->return_type_name,
				false,
				&func_block->val.func.return_type,
				&func_block->val.func.return_packed_type,
				&return_restricted))
				break;
			if (return_restricted) {
				hir_fatal(0, N_TR("A restricted packed type is only valid for a parameter."));
				break;
			}
		}

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
		/* Begin the function scope (params + top-level vars). */
		hir_scope_begin_func();
		if (!hir_scope_push(afunc->stmt_list))
			break;
		{
			struct hir_local *plocal;
			bool param_ok;

			/*
			 * Register the parameters added by hir_visit_param_list().
			 * At this point, the local list contains only parameters.
			 */
			param_ok = true;
			plocal = func_block->val.func.local;

			/* Register every function parameter in the scope. */
			while (plocal != NULL) {
				if (!hir_scope_add_param(0, plocal->symbol)) {
					param_ok = false;
					break;
				}
				plocal = plocal->next;
			}
			if (!param_ok)
				break;
		}

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
			if (!hir_visit_stmt_list(
				&cur_block,
				&prev_block,
				func_block,
				afunc->stmt_list))
				break;

			/* If the first inner block was garbage-collected. */
			if (cur_block == NULL)
				func_block->val.func.inner = NULL;
		}

		/* End the function scope. */
		hir_scope_pop();

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

		/* Visit every statement until control leaves the block. */
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

		/* Connect the block for the selected control transfer. */
		switch (cur_astmt->type) {
		case AST_STMT_CONTINUE:
			/* Find the inner most loop. */
			p_search = parent_block;

			/* Walk outward to the enclosing loop. */
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
				(*cur_block)->is_continue_edge = true;
			} else if (p_search->type == HIR_BLOCK_WHILE) {
				assert(p_search->val.while_.inner != NULL);
				(*cur_block)->succ = p_search->val.while_.inner;
				(*cur_block)->stop = true;
				(*cur_block)->is_continue_edge = true;
			}
			break;
		case AST_STMT_BREAK:
			/* Find the inner most loop. */
			p_search = parent_block;

			/* Walk outward to the enclosing loop. */
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
			(*cur_block)->is_break_edge = true;
			break;
		case AST_STMT_RETURN:
			/* Search a func block.*/
			p_search = *cur_block;

			/* Walk outward through blocks and conditional chains. */
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
			(*cur_block)->is_return_edge = true;
			break;
		default:
			assert(NEVER_COME_HERE);
			break;
		}
	} else {

		/* Connect the ordinary end of the statement list. */
		switch (parent_block->type) {
		case HIR_BLOCK_FUNC:
			/* Search a func block.*/
			p_search = parent_block;

			/* Walk to the outermost function block. */
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

	/* Dispatch the source statement to its specialized visitor. */
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

	UNUSED_PARAMETER(prev_block);
	UNUSED_PARAMETER(parent_block);

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

	UNUSED_PARAMETER(prev_block);
	UNUSED_PARAMETER(parent_block);

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

	/*
	 * Declare var/let bindings first so the LHS never passes through
	 * use resolution and the initializer remains inside the TDZ.
	 */
	if (cur_astmt->val.assign.is_var || cur_astmt->val.assign.is_let) {
		struct ast_expr *alhs;
		struct hir_term *lhs_term;
		struct hir_expr *lhs_expr;
		struct hir_scope_decl *scope_decl;
		const char *src_name;
		const char *int_name;
		int anno_tag;
		int anno_packed_type;
		bool anno_restricted;
		int constructor_packed_type;
		int declared_scalar_kind;
		int storage_class;

		alhs = cur_astmt->val.assign.lhs;
		if (alhs == NULL ||
		    alhs->type != AST_EXPR_TERM ||
		    alhs->val.term.term->type != AST_TERM_SYMBOL) {
			hir_fatal(cur_astmt->line, N_TR("var is specified without a single symbol."));
			hir_free_stmt(hstmt);
			return false;
		}
		src_name = alhs->val.term.term->val.symbol;

		/* Validate the optional type annotation (hint only). */
		if (!hir_resolve_type_annotation(
			cur_astmt->line,
			cur_astmt->val.assign.type_name,
			false,
			&anno_tag,
			&anno_packed_type,
			&anno_restricted)) {
			hir_free_stmt(hstmt);
			return false;
		}

		if (!hir_scope_declare(
			cur_astmt->line,
			src_name,
			cur_astmt->val.assign.is_let,
			&int_name,
			&scope_decl)) {
			hir_free_stmt(hstmt);
			return false;
		}

		/* Visit RHS while the binding is still in its TDZ. */
		if (!hir_visit_expr(&hstmt->rhs, cur_astmt->val.assign.rhs)) {
			hir_free_stmt(hstmt);
			return false;
		}
		hir_scope_mark_declared(scope_decl);

		/* Build the LHS term with the internal name. */
		lhs_term = hir_malloc(sizeof(struct hir_term));
		lhs_expr = hir_malloc(sizeof(struct hir_expr));
		if (lhs_term == NULL) {
			hir_out_of_memory();
			return false;
		}

		if (lhs_expr == NULL) {
			hir_out_of_memory();
			return false;
		}

		memset(lhs_term, 0, sizeof(struct hir_term));
		lhs_term->type = HIR_TERM_SYMBOL;
		lhs_term->val.symbol = hir_strdup(int_name);
		if (lhs_term->val.symbol == NULL) {
			hir_out_of_memory();
			return false;
		}

		memset(lhs_expr, 0, sizeof(struct hir_expr));
		lhs_expr->type = HIR_EXPR_TERM;
		lhs_expr->val.term.term = lhs_term;
		hstmt->lhs = lhs_expr;

		if (!hir_add_local(*cur_block, lhs_term->val.symbol))
			return false;
		constructor_packed_type = hir_packed_constructor_type(hstmt->rhs);
		if (anno_packed_type < 0 && constructor_packed_type >= 0) {
			anno_tag = NOCT_VALUE_PACKED;
			anno_packed_type = constructor_packed_type;
		}
		storage_class = anno_packed_type >= 0 ||
			constructor_packed_type >= 0 ?
			HIR_LOCAL_STORAGE_LOGICAL_BUFFER :
			HIR_LOCAL_STORAGE_SCALAR;
		declared_scalar_kind =
			hir_declared_scalar_kind(cur_astmt->val.assign.type_name);
		if (!hir_set_local_declaration(
			*cur_block,
			lhs_term->val.symbol,
			cur_astmt->val.assign.is_let ?
				HIR_LOCAL_DECL_LET :
				HIR_LOCAL_DECL_VAR,
			anno_tag,
			cur_astmt->val.assign.type_name,
			declared_scalar_kind,
			anno_packed_type,
			storage_class,
			cur_astmt->line,
			hstmt,
			hstmt->rhs))
			return false;

		/* Add hstmt to the end of the block. */
		HIR_ADD_TO_LAST(struct hir_stmt, (*cur_block)->val.basic.stmt_list, hstmt);
		if ((*cur_block)->val.basic.stmt_list == hstmt)
			(*cur_block)->line = cur_astmt->line;
		return true;
	}

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

	/* Reject assignment to a let binding. */
	if (hstmt->lhs->type == HIR_EXPR_TERM &&
	    hstmt->lhs->val.term.term->type == HIR_TERM_SYMBOL) {
		if (!hir_scope_check_assign(
			cur_astmt->line,
			hstmt->lhs->val.term.term->val.symbol)) {
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

static struct hir_local *
hir_find_local(
	struct hir_block *block,
	const char *symbol)
{
	struct hir_local *local;

	/* Walk from the selected block to its function. */
	while (block != NULL && block->type != HIR_BLOCK_FUNC)
		block = block->parent;

	if (block == NULL)
		return NULL;

	local = block->val.func.local;

	/* Search every local in the function. */
	while (local != NULL) {
		if (strcmp(local->symbol, symbol) == 0)
			return local;
		local = local->next;
	}

	return NULL;
}

static bool
hir_set_local_declaration(
	struct hir_block *block,
	const char *symbol,
	int declaration_kind,
	int declared_type,
	const char *declared_type_name,
	int declared_scalar_kind,
	int declared_packed_type,
	int storage_class,
	int line,
	const struct hir_stmt *declaration_stmt,
	const struct hir_expr *initializer)
{
	struct hir_local *local;

	local = hir_find_local(block, symbol);
	assert(local != NULL);
	if (local == NULL)
		return false;

	if (declared_type_name != NULL) {
		local->declared_type_name = hir_strdup(declared_type_name);
		if (local->declared_type_name == NULL) {
			hir_out_of_memory();
			return false;
		}
	}

	local->is_parameter = declaration_kind == HIR_LOCAL_DECL_PARAMETER;
	local->is_let = declaration_kind == HIR_LOCAL_DECL_LET;
	local->declaration_kind = declaration_kind;
	local->declared_type = declared_type;
	local->declared_scalar_kind = declared_scalar_kind;
	local->declared_packed_type = declared_packed_type;
	local->storage_class = storage_class;
	local->declaration_line = line;
	local->declaration_stmt = declaration_stmt;
	local->initializer = initializer;

	return true;
}

/* Return a NOCT_PACKED_* kind for a direct Packed.* constructor. */
static int
hir_packed_constructor_type(
	const struct hir_expr *expr)
{
	const struct hir_expr *obj;
	const char *name;

	if (expr == NULL || expr->type != HIR_EXPR_THISCALL)
		return -1;
	obj = expr->val.thiscall.obj;
	if (obj == NULL ||
	    obj->type != HIR_EXPR_TERM ||
	    obj->val.term.term == NULL ||
	    obj->val.term.term->type != HIR_TERM_SYMBOL ||
	    strcmp(obj->val.term.term->val.symbol, "Packed") != 0)
		return -1;

	name = expr->val.thiscall.func;

	if (strcmp(name, "int8") == 0)
		return NOCT_PACKED_INT8;
	if (strcmp(name, "uint8") == 0)
		return NOCT_PACKED_UINT8;
	if (strcmp(name, "int16") == 0)
		return NOCT_PACKED_INT16;
	if (strcmp(name, "uint16") == 0)
		return NOCT_PACKED_UINT16;
	if (strcmp(name, "int32") == 0)
		return NOCT_PACKED_INT32;
	if (strcmp(name, "uint32") == 0)
		return NOCT_PACKED_UINT32;
	if (strcmp(name, "int64") == 0)
		return NOCT_PACKED_INT64;
	if (strcmp(name, "uint64") == 0)
		return NOCT_PACKED_UINT64;
	if (strcmp(name, "float32") == 0)
		return NOCT_PACKED_FLOAT32;
	if (strcmp(name, "float64") == 0)
		return NOCT_PACKED_FLOAT64;

	return -1;
}

static int
hir_declared_scalar_kind(
	const char *type_name)
{
	if (type_name == NULL)
		return HIR_DECL_SCALAR_UNKNOWN;

	if (strcmp(type_name, "int") == 0)
		return HIR_DECL_SCALAR_INT32;
	if (strcmp(type_name, "i32") == 0)
		return HIR_DECL_SCALAR_INT32;
	if (strcmp(type_name, "u32") == 0)
		return HIR_DECL_SCALAR_UINT32;
	if (strcmp(type_name, "float") == 0)
		return HIR_DECL_SCALAR_FLOAT32;

	return HIR_DECL_SCALAR_OTHER;
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
	if (!hir_scope_push(cur_astmt->val.if_.stmt_list))
		return false;
	if (cur_astmt->val.if_.stmt_list != NULL) {
		inner_cur_block = if_block->val.if_.inner;
		inner_prev_block = NULL;
		if (!hir_visit_stmt_list(
			&inner_cur_block,
			&inner_prev_block,
			if_block,
			cur_astmt->val.if_.stmt_list)) {
			hir_free_block(if_block);
			return false;
		}
	}

	/* End the block scope. */
	hir_scope_pop();

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

	/*
	 * The exit block is taken from the chain's first if-block below,
	 * so parent_block itself is not used here. When an if/else-if
	 * chain is nested inside another if's branch, parent_block is
	 * that enclosing if-block, which is fine.
	 */

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

	/* Walk to the first block in the conditional chain. */
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
	if (!hir_scope_push(cur_astmt->val.elif.stmt_list))
		return false;
	if (cur_astmt->val.elif.stmt_list != NULL) {
		inner_cur_block = elif_block->val.if_.inner;
		inner_prev_block = NULL;
		if (!hir_visit_stmt_list(
			&inner_cur_block,
			&inner_prev_block,
			elif_block,
			cur_astmt->val.elif.stmt_list)) {
			hir_free_block(elif_block);
			return false;
		}
	}

	/* End the block scope. */
	hir_scope_pop();

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

	/* Walk to the first block in the conditional chain. */
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
	if (!hir_scope_push(cur_astmt->val.else_.stmt_list))
		return false;
	if (cur_astmt->val.else_.stmt_list != NULL) {
		inner_cur_block = else_block->val.if_.inner;
		inner_prev_block = NULL;
		if (!hir_visit_stmt_list(
			&inner_cur_block,
			&inner_prev_block,
			else_block,
			cur_astmt->val.else_.stmt_list)) {
			hir_free_block(else_block);
			return false;
		}
	}

	/* End the block scope. */
	hir_scope_pop();

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
	if (!hir_scope_push(cur_astmt->val.while_.stmt_list))
		return false;
	if (cur_astmt->val.while_.stmt_list != NULL) {
		inner_cur_block = while_block->val.while_.inner;
		inner_prev_block = NULL;
		if (!hir_visit_stmt_list(
			&inner_cur_block,
			&inner_prev_block,
			while_block,
			cur_astmt->val.while_.stmt_list)) {
			hir_free_block(while_block);
			return false;
		}
	}

	/* End the block scope. */
	hir_scope_pop();

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

	/*
	 * Mark a ranged-for loop. Its variables are registered after the
	 * range or collection expressions are visited in the outer scope.
	 */
	if (cur_astmt->val.for_.counter_symbol)
		for_block->val.for_.is_ranged = true;

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

	/* Enter the loop-body scope; loop variables live in it. */
	if (!hir_scope_push(cur_astmt->val.for_.stmt_list)) {
		hir_free_block(for_block);
		return false;
	}
	{
		static const char *empty = "";
		const char *names[3];
		char **fields[3];
		int k;

		names[0] = cur_astmt->val.for_.counter_symbol;
		names[1] = cur_astmt->val.for_.key_symbol;
		names[2] = cur_astmt->val.for_.value_symbol;
		fields[0] = &for_block->val.for_.counter_symbol;
		fields[1] = &for_block->val.for_.key_symbol;
		fields[2] = &for_block->val.for_.value_symbol;
		(void)empty;

		/* Register every loop variable in the body scope. */
		for (k = 0; k < 3; k++) {
			struct hir_scope_decl *scope_decl;
			const char *iname;

			if (names[k] == NULL)
				continue;

			if (!hir_scope_declare(
				cur_astmt->line,
				names[k],
				false,
				&iname,
				&scope_decl)) {
				hir_free_block(for_block);
				return false;
			}
			hir_scope_mark_declared(scope_decl);
			*fields[k] = hir_strdup(iname);
			if (*fields[k] == NULL) {
				hir_out_of_memory();
				return false;
			}
			if (!hir_add_local(*cur_block, *fields[k]))
				return false;
			if (!hir_set_local_declaration(
				*cur_block,
				*fields[k],
				HIR_LOCAL_DECL_LOOP_COUNTER,
				-1,
				NULL,
				HIR_DECL_SCALAR_UNKNOWN,
				-1,
				HIR_LOCAL_STORAGE_SCALAR,
				cur_astmt->line,
				NULL,
				NULL))
				return false;
		}
	}

	/* Visit an inner stmt_list */
	inner_cur_block = for_block->val.for_.inner;
	inner_prev_block = NULL;
	if (!hir_visit_stmt_list(
		&inner_cur_block,
		&inner_prev_block,
		for_block,
		cur_astmt->val.for_.stmt_list)) {
		hir_free_block(for_block);
		return false;
	}

	/* End the loop-body scope. */
	hir_scope_pop();

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

	UNUSED_PARAMETER(prev_block);
	UNUSED_PARAMETER(parent_block);

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
	hstmt->is_bare_return = !cur_astmt->val.return_.has_value;

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
	case AST_EXPR_LAND:
		result = hir_visit_binary_expr(hexpr, aexpr, HIR_EXPR_LAND);
		break;
	case AST_EXPR_LOR:
		result = hir_visit_binary_expr(hexpr, aexpr, HIR_EXPR_LOR);
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
	{
		struct ast_expr *func;
		struct ast_expr *obj;
		const char *symbol;
		bool is_thiscall;

		func = aexpr->val.call.func;
		is_thiscall = false;
		if (func != NULL &&
		    func->type == AST_EXPR_DOT) {
			is_thiscall = true;
			obj = func->val.dot.obj;

			if (obj->type == AST_EXPR_TERM &&
			    obj->val.term.term->type == AST_TERM_SYMBOL) {
				symbol = obj->val.term.term->val.symbol;
				if (strcmp(symbol, "Int") == 0) {
					if (strcmp(func->val.dot.symbol, "from") == 0)
						is_thiscall = false;
				} else if (strcmp(symbol, "Float") == 0) {
					if (strcmp(func->val.dot.symbol, "from") == 0)
						is_thiscall = false;
				}
			}
		}

		if (is_thiscall)
			result = hir_visit_thiscall_expr(hexpr, aexpr);
		else
			result = hir_visit_call_expr(hexpr, aexpr);
		break;
	}
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
	assert(
		aexpr->type == AST_EXPR_NEG ||
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

		/* Convert every call argument. */
		while (arg != NULL) {
			if (e->val.call.arg_count >= HIR_PARAM_SIZE) {
				hir_fatal(hir_error_line, N_TR("Exceeded the maximum argument count."));
				return false;
			}
			if (!hir_visit_expr(&e->val.call.arg[e->val.call.arg_count], arg)) {
				hir_free_expr(e);
				return false;
			}
			arg = arg->next;
			e->val.call.arg_count++;
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
	struct ast_expr *dot;
	struct ast_expr *obj;
	struct ast_expr *arg;
	struct ast_arg_list *arg_list;
	const char *func;

	assert(hexpr != NULL);
	assert(*hexpr == NULL);
	assert(aexpr != NULL);
	assert(aexpr->type == AST_EXPR_CALL);
	assert(aexpr->val.call.func != NULL);
	assert(aexpr->val.call.func->type == AST_EXPR_DOT);
	dot = aexpr->val.call.func;
	obj = dot->val.dot.obj;
	func = dot->val.dot.symbol;
	arg_list = aexpr->val.call.arg_list;

	/* Allocate an hexpr. */
	e = hir_malloc(sizeof(struct hir_expr));
	if (e == NULL) {
		hir_out_of_memory();
		return false;
	}

	memset(e, 0, sizeof(struct hir_expr));
	e->type = HIR_EXPR_THISCALL;

	/* Visit the object expression. */
	if (!hir_visit_expr(&e->val.thiscall.obj, obj)) {
		hir_free_expr(e);
		return false;
	}

	/* Copy the function name. */
	e->val.thiscall.func = hir_strdup(func);
	if (e->val.thiscall.func == NULL) {
		hir_free_expr(e);
		return false;
	}

	/* Visit the argument expressions. */
	if (arg_list != NULL) {
		arg = arg_list->list;

		/* Convert every method-call argument. */
		while (arg != NULL) {
			if (e->val.thiscall.arg_count >= HIR_PARAM_SIZE) {
				hir_fatal(hir_error_line, N_TR("Too many parameters."));
				hir_free_expr(e);
				return false;
			}
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
	size_t count, index;

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
	e->val.array.is_multi_index = aexpr->val.array.is_multi_index;

	/* Count the elements and allocate a table. */
	count = 0;
	if (aexpr->val.array.elem_list != NULL) {
		elem = aexpr->val.array.elem_list->list;

		/* Count every array element. */
		while (elem != NULL) {
			elem = elem->next;
			count++;
		}

		if (count > SIZE_MAX / sizeof(struct hir_exp *)) {
			hir_out_of_memory();
			hir_free_expr(e);
			return false;
		}

		e->val.array.elem_count = count;
		e->val.array.elem = hir_malloc((uint32_t)count * sizeof(struct hir_exp *));
		if (e->val.array.elem == NULL) {
			hir_out_of_memory();
			return false;
		}

		memset(e->val.array.elem, 0, (size_t)count * sizeof(struct hir_exp *));
	}

	/* Visit the argument expressions. */
	if (aexpr->val.array.elem_list != NULL) {
		elem = aexpr->val.array.elem_list->list;
		index = 0;

		/* Convert every array element. */
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
	size_t count, index;

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

		/* Count every dictionary entry. */
		while (kv != NULL) {
			kv = kv->next;
			count++;
		}

		if (count > SIZE_MAX / sizeof(char *)) {
			hir_out_of_memory();
			hir_free_expr(e);
			return false;
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

		/* Convert every dictionary entry. */
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

	/* A class literal is frozen at creation. */
	if (aexpr->val.dict.is_class)
		return hir_wrap_freeze(hexpr, e);

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

	/* An extend result is a frozen class template. */
	if (aexpr->val.new_.is_extend)
		return hir_wrap_freeze(hexpr, e);

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
	{
		const char *resolved;

		/* Scope resolution (alpha-renaming + static TDZ). */
		if (!hir_scope_resolve(
			hir_error_line,
			aterm->val.symbol,
			&resolved))
			return false;

		t->type = HIR_TERM_SYMBOL;
		if (resolved == NULL)
			resolved = ast_resolve_static_symbol(aterm->val.symbol);

		t->val.symbol = hir_strdup(resolved);
		if (t->val.symbol == NULL) {
			hir_out_of_memory();
			return false;
		}
		break;
	}
	case AST_TERM_INT:
		t->type = HIR_TERM_INT;
		t->val.i = aterm->val.i;
		break;
	case AST_TERM_LONG:
		t->type = HIR_TERM_LONG;
		t->val.l = aterm->val.l;
		break;
	case AST_TERM_FLOAT:
		t->type = HIR_TERM_FLOAT;
		t->val.f = aterm->val.f;
		break;
	case AST_TERM_DOUBLE:
	{
		t->type = HIR_TERM_DOUBLE;
		t->val.lf = aterm->val.lf;
		break;
	}
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

/*
 * Type-annotation name table (docs/design/02-typing.md).  Sized
 * integer names degrade to their storage-class tag.  Returns the
 * NOCT_VALUE_* tag, or -1 for an unknown name (caller errors).
 */
static bool
hir_resolve_type_name(
	const char *name,
	int *tag,
	int *packed_type,
	bool *restricted)
{
	static const struct {
		const char *name;
		int tag;
		int packed_type;
		bool restricted;
	} tbl[] = {
		{ "void",            HIR_TYPE_VOID,       -1, false },
		{ "int",             NOCT_VALUE_INT,    -1, false },
		{ "long",            NOCT_VALUE_LONG,   -1, false },
		{ "float",           NOCT_VALUE_FLOAT,  -1, false },
		{ "double",          NOCT_VALUE_DOUBLE, -1, false },
		{ "string",          NOCT_VALUE_STRING, -1, false },
		{ "array",           NOCT_VALUE_ARRAY,  -1, false },
		{ "dict",            NOCT_VALUE_DICT,   -1, false },
		{ "packed",          NOCT_VALUE_PACKED, NOCT_PACKED_ANY, false },
		{ "func",            NOCT_VALUE_FUNC,   -1, false },
		{ "i8",              NOCT_VALUE_INT,    -1, false },
		{ "i16",             NOCT_VALUE_INT,    -1, false },
		{ "i32",             NOCT_VALUE_INT,    -1, false },
		{ "u8",              NOCT_VALUE_INT,    -1, false },
		{ "u16",             NOCT_VALUE_INT,    -1, false },
		{ "u32",             NOCT_VALUE_INT,    -1, false },
		{ "i64",             NOCT_VALUE_LONG,   -1, false },
		{ "u64",             NOCT_VALUE_LONG,   -1, false },
		{ "packedint8",      NOCT_VALUE_PACKED, NOCT_PACKED_INT8, false },
		{ "packeduint8",     NOCT_VALUE_PACKED, NOCT_PACKED_UINT8, false },
		{ "packedint16",     NOCT_VALUE_PACKED, NOCT_PACKED_INT16, false },
		{ "packeduint16",    NOCT_VALUE_PACKED, NOCT_PACKED_UINT16, false },
		{ "packedint32",     NOCT_VALUE_PACKED, NOCT_PACKED_INT32, false },
		{ "packeduint32",    NOCT_VALUE_PACKED, NOCT_PACKED_UINT32, false },
		{ "packedint64",     NOCT_VALUE_PACKED, NOCT_PACKED_INT64, false },
		{ "packeduint64",    NOCT_VALUE_PACKED, NOCT_PACKED_UINT64, false },
		{ "packedfloat",     NOCT_VALUE_PACKED, NOCT_PACKED_FLOAT32, false },
		{ "packeddouble",    NOCT_VALUE_PACKED, NOCT_PACKED_FLOAT64, false },
		{ "rpackedint8",     NOCT_VALUE_PACKED, NOCT_PACKED_INT8, true },
		{ "rpackeduint8",    NOCT_VALUE_PACKED, NOCT_PACKED_UINT8, true },
		{ "rpackedint16",    NOCT_VALUE_PACKED, NOCT_PACKED_INT16, true },
		{ "rpackeduint16",   NOCT_VALUE_PACKED, NOCT_PACKED_UINT16, true },
		{ "rpackedint32",    NOCT_VALUE_PACKED, NOCT_PACKED_INT32, true },
		{ "rpackeduint32",   NOCT_VALUE_PACKED, NOCT_PACKED_UINT32, true },
		{ "rpackedint64",    NOCT_VALUE_PACKED, NOCT_PACKED_INT64, true },
		{ "rpackeduint64",   NOCT_VALUE_PACKED, NOCT_PACKED_UINT64, true },
		{ "rpackedfloat",    NOCT_VALUE_PACKED, NOCT_PACKED_FLOAT32, true },
		{ "rpackeddouble",   NOCT_VALUE_PACKED, NOCT_PACKED_FLOAT64, true }
	};
	size_t i;

	/* Match the source name against every supported annotation. */
	for (i = 0; i < sizeof(tbl) / sizeof(tbl[0]); i++) {
		if (strcmp(tbl[i].name, name) == 0) {
			*tag = tbl[i].tag;
			*packed_type = tbl[i].packed_type;
			*restricted = tbl[i].restricted;
			return true;
		}
	}

	return false;
}

/* Copy the base spelling before an optional shape annotation. */
static bool
hir_annotation_base(
	const char *annotation,
	char *base,
	size_t base_size)
{
	const char *open;
	const char *close;
	const char *first_close;
	size_t length;

	if (annotation == NULL ||
	    base == NULL ||
	    base_size == 0)
		return false;

	open = strchr(annotation, '(');
	if (open == NULL) {
		if (strchr(annotation, ')') != NULL)
			return false;

		length = strlen(annotation);
	} else {
		close = strrchr(open + 1, ')');
		if (close == NULL ||
		    close[1] != '\0' ||
		    open + 1 == close)
			return false;
		if (strchr(open + 1, '(') != NULL)
			return false;

		first_close = strchr(annotation, ')');
		if (first_close != close)
			return false;

		length = (size_t)(open - annotation);
	}

	if (length == 0 || length >= base_size)
		return false;

	memcpy(base, annotation, length);
	base[length] = '\0';

	return true;
}

/* Copy parameter names and count parameters. */
static bool
hir_visit_param_list(
	struct hir_block *hfunc,
	struct ast_func *afunc)
{
	struct ast_param *param;
	uint32_t param_count;

	/* -1 = unannotated (0 would read as NOCT_VALUE_INT). */
	{
		uint32_t k;

		/* Reset every parameter annotation slot. */
		for (k = 0; k < HIR_PARAM_SIZE; k++) {
			hfunc->val.func.param_type[k] = -1;
			hfunc->val.func.param_packed_type[k] = -1;
			hfunc->val.func.param_restricted[k] = false;
			hfunc->val.func.param_type_name[k] = NULL;
		}
	}

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

	/* Copy every function parameter. */
	while (param != NULL) {
		const char *annotation;
		int declared_scalar_kind;
		int storage_class;

		if (param_count >= HIR_PARAM_SIZE) {
			hir_fatal(hir_error_line, N_TR("Too many parameters."));
			return false;
		}

		/* Copy names and count parameters. */
		hfunc->val.func.param_name[param_count] = hir_strdup(param->name);
		if (hfunc->val.func.param_name[param_count] == NULL) {
			hir_out_of_memory();
			return false;
		}

		annotation = param->type_name;
		if (!hfunc->val.func.is_fast &&
		    annotation != NULL &&
		    strchr(annotation, '(') != NULL) {
			hir_fatal(
				0,
				N_TR("Shaped parameter types are valid only on __fast func."));
			return false;
		}
		if (annotation != NULL) {
			hfunc->val.func.param_type_name[param_count] =
				hir_strdup(annotation);
			if (hfunc->val.func.param_type_name[param_count] == NULL) {
				hir_out_of_memory();
				return false;
			}
		}

		/* Resolve the optional type annotation. */
		if (!hir_resolve_type_annotation(
			0,
			annotation,
			true,
			&hfunc->val.func.param_type[param_count],
			&hfunc->val.func.param_packed_type[param_count],
			&hfunc->val.func.param_restricted[param_count]))
			return false;

		/* Add to a local variable list. */
		if (!hir_add_local(hfunc, param->name))
			return false;

		declared_scalar_kind = hir_declared_scalar_kind(annotation);
		if (hfunc->val.func.param_packed_type[param_count] >= 0)
			storage_class = HIR_LOCAL_STORAGE_LOGICAL_BUFFER;
		else
			storage_class = HIR_LOCAL_STORAGE_SCALAR;

		if (!hir_set_local_declaration(
			hfunc,
			param->name,
			HIR_LOCAL_DECL_PARAMETER,
			hfunc->val.func.param_type[param_count],
			annotation,
			declared_scalar_kind,
			hfunc->val.func.param_packed_type[param_count],
			storage_class,
			-1,
			NULL,
			NULL))
			return false;
		param_count++;

		param = param->next;
	}
	hfunc->val.func.param_count = param_count;

	return true;
}

/* Wrap an expression in a Dict.freeze(...) call (class/extend). */
static bool
hir_wrap_freeze(
	struct hir_expr **hexpr,
	struct hir_expr *inner)
{
	struct hir_expr *call;
	struct hir_expr *fn;
	struct hir_term *fn_term;

	call = hir_malloc(sizeof(struct hir_expr));
	fn = hir_malloc(sizeof(struct hir_expr));
	fn_term = hir_malloc(sizeof(struct hir_term));
	if (call == NULL) {
		hir_out_of_memory();
		return false;
	}

	if (fn == NULL) {
		hir_out_of_memory();
		return false;
	}

	if (fn_term == NULL) {
		hir_out_of_memory();
		return false;
	}

	memset(fn_term, 0, sizeof(struct hir_term));
	fn_term->type = HIR_TERM_SYMBOL;
	fn_term->val.symbol = hir_strdup("Dict.freeze");
	if (fn_term->val.symbol == NULL) {
		hir_out_of_memory();
		return false;
	}

	memset(fn, 0, sizeof(struct hir_expr));
	fn->type = HIR_EXPR_TERM;
	fn->val.term.term = fn_term;

	memset(call, 0, sizeof(struct hir_expr));
	call->type = HIR_EXPR_CALL;
	call->val.call.func = fn;
	call->val.call.arg_count = 1;
	call->val.call.arg[0] = inner;

	*hexpr = call;

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

	if (hir_anon_func_count >= ANON_FUNC_SIZE) {
		hir_fatal(hir_error_line, N_TR("Too many anonymous functions."));
		return false;
	}

	hir_anon_func_name[hir_anon_func_count] = *symbol;
	hir_anon_func_param_list[hir_anon_func_count] = aexpr->val.func.param_list;
	hir_anon_func_stmt_list[hir_anon_func_count] = aexpr->val.func.stmt_list;
	hir_anon_func_count++;

	return true;
}

/* Free a block and its siblings. */
static void
hir_free_block(
	struct hir_block *b)
{
	uint32_t i;

	/* Free the resources owned by this block type. */
	switch (b->type) {
	case HIR_BLOCK_FUNC:
		if (b->val.func.name != NULL) {
			hir_free(b->val.func.name);
			b->val.func.name = NULL;
		}

		/* Free every function parameter name. */
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
#if defined(NOCT_USE_OPTIMIZER)
		hir_fast_checked_cleanup_func(b);
#endif
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
	uint32_t i;

	/* Free the resources owned by this expression type. */
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
	case HIR_EXPR_LAND:
	case HIR_EXPR_LOR:
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
	case HIR_EXPR_CAPTURE:
		if (e->val.capture.expr != NULL) {
			hir_free_expr(e->val.capture.expr);
			e->val.capture.expr = NULL;
		}
		if (e->val.capture.symbol != NULL) {
			hir_free(e->val.capture.symbol);
			e->val.capture.symbol = NULL;
		}
		break;
	case HIR_EXPR_SELECT:
		if (e->val.select.cond != NULL) {
			hir_free_expr(e->val.select.cond);
			e->val.select.cond = NULL;
		}
		if (e->val.select.if_true != NULL) {
			hir_free_expr(e->val.select.if_true);
			e->val.select.if_true = NULL;
		}
		if (e->val.select.if_false != NULL) {
			hir_free_expr(e->val.select.if_false);
			e->val.select.if_false = NULL;
		}
		break;
	case HIR_EXPR_PMASKSTORE32:
		if (e->val.mask_store.base != NULL)
			hir_free_expr(e->val.mask_store.base);
		if (e->val.mask_store.offset != NULL)
			hir_free_expr(e->val.mask_store.offset);
		if (e->val.mask_store.mask != NULL)
			hir_free_expr(e->val.mask_store.mask);
		break;
	case HIR_EXPR_PGATHER32:
		if (e->val.gather.base != NULL)
			hir_free_expr(e->val.gather.base);
		if (e->val.gather.length != NULL)
			hir_free_expr(e->val.gather.length);
		if (e->val.gather.index != NULL)
			hir_free_expr(e->val.gather.index);
		if (e->val.gather.packed != NULL)
			hir_free_expr(e->val.gather.packed);
		break;
	case HIR_EXPR_CALL:
		if (e->val.call.func != NULL) {
			hir_free_expr(e->val.call.func);
			e->val.call.func = NULL;
		}

		/* Free every call argument. */
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

		/* Free every method-call argument. */
		for (i = 0; i < e->val.thiscall.arg_count; i++) {
			if (e->val.thiscall.arg[i] != NULL) {
				hir_free_expr(e->val.thiscall.arg[i]);
				e->val.thiscall.arg[i] = NULL;
			}
		}
		break;
	case HIR_EXPR_ARRAY:

		/* Free every array element. */
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

		/* Free every dictionary entry. */
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
	case HIR_EXPR_PBASE:
	case HIR_EXPR_PLEN:
		/* ABCE unary ops. */
		if (e->val.unary.expr != NULL) {
			hir_free_expr(e->val.unary.expr);
			e->val.unary.expr = NULL;
		}
		break;
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
	case HIR_EXPR_VINDUCTF32:
		/* ABCE binary ops. */
		if (e->val.binary.expr[0] != NULL) {
			hir_free_expr(e->val.binary.expr[0]);
			e->val.binary.expr[0] = NULL;
		}
		if (e->val.binary.expr[1] != NULL) {
			hir_free_expr(e->val.binary.expr[1]);
			e->val.binary.expr[1] = NULL;
		}
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

	/* Free the resources owned by this term type. */
	switch (t->type) {
	case HIR_TERM_INT:
	case HIR_TERM_LONG:
	case HIR_TERM_FLOAT:
	case HIR_TERM_DOUBLE:
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
	hir_free(local->declared_type_name);
}

/* Set a fatal error message. */
static void
hir_fatal(
	int line,
	const char *msg)
{
	hir_error_line = line;

	/*
	 * Store the bare message: every consumer (rt_register_source,
	 * elback, cback) formats the file name and line by itself, so
	 * embedding them here used to print them twice.
	 */
	snprintf(
		hir_error_message,
		sizeof(hir_error_message),
		"%s",
		msg);
}

/* free() alternative. */
static void
hir_free(
	void *p)
{
	UNUSED_PARAMETER(p);

	/*
	 * In the current implementation, we don't free individual
	 * objects because we use an arena allocator.
	 */
}

static void
hir_dump_block_at_level(
	struct hir_block *block,
	int level)
{
	int i;

	/* Dump every block in the successor chain. */
	while (block != NULL) {

		/* Indent the current block. */
		for (i = 0; i < level * 4; i++)
			printf(" ");
		printf("BLOCK(%d)", block->id);

		/* Dump the fields selected by the block type. */
		switch (block->type) {
		case HIR_BLOCK_FUNC:
		{
			printf(" FUNC parent=%d, succ=%d\n", block->parent->id, block->succ->id);

			if (block->val.func.inner != NULL) {

				/* Indent the inner-block marker. */
				for (i = 0; i < (level + 1) * 4; i++)
					printf(" ");
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

			/* Walk every statement reserved for detailed dumping. */
			while (s != NULL) {
				/* hir_dump_stmt(level + 1, s); */
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

				/* Indent the loop-body marker. */
				for (i = 0; i < (level + 1) * 4; i++)
					printf(" ");
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

				/* Indent the conditional-body marker. */
				for (i = 0; i < (level + 1) * 4; i++)
					printf(" ");
				printf("[INNER]\n");
				hir_dump_block_at_level(block->val.if_.inner, level + 1);
			}
			if (block->val.if_.chain_next != NULL) {

				/* Indent the conditional-chain marker. */
				for (i = 0; i < (level + 1) * 4; i++)
					printf(" ");
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

				/* Indent the stop marker. */
				for (i = 0; i < level * 4; i++)
					printf(" ");
				printf("[STOP %d]\n", block->succ->id);
				break;
			}
		}
		block = block->succ;
	}
}
