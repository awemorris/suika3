/* -*- coding: utf-8; tab-width: 8; indent-tabs-mode: t; -*- */

/*
 * Noct Programming Language
 * Copyright (c) 2025, 2026, Awe Morris
 */

/*
 * Optimizer-owned checked HIR support for __fast functions.
 */

#include "hir_fast_checked.h"
#include "fast.h"
#include "hir.h"
#include "ast.h"

#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define FAST_CHECKED_EDGE_INITIAL	16
#define FAST_CHECKED_EXPR_DEPTH_MAX	64
#define FAST_CHECKED_PROTOTYPE_INITIAL	16
#define FAST_CHECKED_TYPE_ERROR		(-3)

struct fast_checked_edge {
	uint32_t caller;
	uint32_t callee;
	int line;
};

struct fast_checked_prototype {
	char *name;
	bool is_fast;
	struct fast_signature signature;
};

struct fast_checked_module {
	struct hir_block *const *func_table;
	uint32_t func_count;
	struct fast_checked_edge *edge;
	uint32_t edge_count;
	uint32_t edge_capacity;
};

struct fast_checked_context {
	struct fast_checked_module *module;
	struct hir_block *func;
};

static const char *const fast_checked_intrinsic_names[] = {
	"min", "max", "abs", "sqrt", "sin", "cos", "tan",
	"asin", "acos", "atan", "atan2", "exp", "ln", "log2",
	"log10", "int", "long", "float", "double"
};
static struct fast_checked_prototype *fast_checked_prototype_table;
static uint32_t fast_checked_prototype_count;
static uint32_t fast_checked_prototype_capacity;
static struct fast_checked_edge *fast_checked_prototype_edge_table;
static uint32_t fast_checked_prototype_edge_count;
static uint32_t fast_checked_prototype_edge_capacity;

static struct fast_checked_prototype *fast_checked_find_prototype(const char *name);
static bool fast_checked_build_ast_signature(const struct ast_func *func, struct fast_signature *signature);
static bool fast_checked_build_hir_signature(struct hir_block *func);
static const struct fast_signature *fast_checked_get_signature(const struct hir_block *func);
static bool fast_checked_signature_equal(const struct fast_signature *first, const struct fast_signature *second);
static bool fast_checked_add_prototype_edge(const char *caller_name, const char *callee_name, int line);
static bool fast_checked_validate_prototype_cycles(void);
static bool fast_checked_visit_prototype_cycle(uint32_t prototype, unsigned char *state);
static bool fast_checked_validate_function(struct fast_checked_module *module, struct hir_block *func);
static bool fast_checked_validate_locals(struct fast_checked_context *context);
static bool fast_checked_chain(struct fast_checked_context *context, struct hir_block *head);
static bool fast_checked_statement(struct fast_checked_context *context, struct hir_stmt *statement);
static bool fast_checked_expr(struct fast_checked_context *context, struct hir_expr **expr, int line, uint32_t depth, int *type);
static bool fast_checked_term(struct fast_checked_context *context, const struct hir_term *term, int line, int *type);
static bool fast_checked_binary(struct fast_checked_context *context, struct hir_expr *expr, int line, uint32_t depth, int *type);
static bool fast_checked_subscript(struct fast_checked_context *context, struct hir_expr *expr, int line, uint32_t depth, int *type);
static bool fast_checked_call(struct fast_checked_context *context, struct hir_expr *expr, int line, uint32_t depth, int *type);
static bool fast_checked_intrinsic(struct fast_checked_context *context, struct hir_expr *expr, const char *name, int line, uint32_t depth, int *type);
static bool fast_checked_rewrite_intrinsic(struct hir_expr *call, const char *name);
static bool fast_checked_lower_multi_index(struct fast_checked_context *context, struct hir_expr *subscript, const struct fast_param_contract *contract, int line);
static struct hir_expr *fast_checked_extent_expr(struct fast_checked_context *context, const struct fast_extent *extent);
static struct hir_expr *fast_checked_symbol_expr(const char *symbol);
static struct hir_local *fast_checked_find_local(const struct hir_block *func, const char *symbol);
static int fast_checked_find_param(const struct hir_block *func, const char *symbol);
static int fast_checked_find_function(const struct fast_checked_module *module, const char *symbol);
static int fast_checked_packed_type(int packed_type);
static bool fast_checked_primitive(int type);
static bool fast_checked_integer(int type);
static bool fast_checked_local_spelling(const struct hir_local *local);
static bool fast_checked_intrinsic_name(const char *name);
static bool fast_checked_intrinsic_transcendental(const char *name);
static bool fast_checked_validate_call_shape(struct fast_checked_context *context, const struct fast_signature *callee, const struct hir_expr *call, uint32_t argument, int line);
static bool fast_checked_extent_matches(struct fast_checked_context *context, const struct fast_extent *actual, const struct fast_extent *formal, const struct hir_expr *call);
static bool fast_checked_add_edge(struct fast_checked_module *module, uint32_t caller, uint32_t callee, int line);
static bool fast_checked_validate_cycles(struct fast_checked_module *module);
static bool fast_checked_visit_cycle(struct fast_checked_module *module, uint32_t func, unsigned char *state);
static bool fast_checked_reject_nonfast_multi(struct hir_block *func);
static bool fast_checked_find_multi_chain(struct hir_block *head, int *line);
static bool fast_checked_find_multi_expr(const struct hir_expr *expr);

/*
 * Collects externally visible optimizer prototypes from the current AST.
 */
bool
hir_fast_checked_collect_prototypes(
	void)
{
	struct ast_func_list *func_list;
	struct ast_func *func;
	struct fast_signature signature;
	const struct fast_signature *signature_ptr;

	func_list = ast_get_func_list();
	func = func_list != NULL ? func_list->list : NULL;

	/* Collect every function that is visible outside this source file. */
	while (func != NULL) {
		if (func->is_static) {
			func = func->next;
			continue;
		}

		fast_signature_init(&signature);
		signature_ptr = NULL;

		if (func->is_fast) {
			if (!fast_checked_build_ast_signature(func, &signature)) {
				fast_signature_free(&signature);
				return false;
			}

			signature_ptr = &signature;
		}

		if (!hir_fast_checked_add_prototype(
			func->name,
			func->is_fast,
			signature_ptr)) {
			fast_signature_free(&signature);
			return false;
		}

		fast_signature_free(&signature);
		func = func->next;
	}

	return true;
}

/*
 * Clears every externally collected function prototype.
 */
void
hir_fast_checked_reset_prototypes(
	void)
{
	struct fast_checked_prototype *prototype;
	uint32_t i;

	/* Release every prototype and its owned fast signature. */
	for (i = 0; i < fast_checked_prototype_count; i++) {
		prototype = &fast_checked_prototype_table[i];
		noct_free(prototype->name);
		prototype->name = NULL;
		fast_signature_free(&prototype->signature);
	}

	noct_free(fast_checked_prototype_table);
	fast_checked_prototype_table = NULL;
	fast_checked_prototype_count = 0;
	fast_checked_prototype_capacity = 0;

	noct_free(fast_checked_prototype_edge_table);
	fast_checked_prototype_edge_table = NULL;
	fast_checked_prototype_edge_count = 0;
	fast_checked_prototype_edge_capacity = 0;
}

/*
 * Adds an externally collected function prototype.
 */
bool
hir_fast_checked_add_prototype(
	const char *name,
	bool is_fast,
	const struct fast_signature *signature)
{
	struct fast_checked_prototype *prototype;
	struct fast_checked_prototype *table;
	char message[256];
	uint32_t capacity;
	bool compatible;

	if (name == NULL || name[0] == '\0') {
		hir_error(0, N_TR("Invalid function prototype."));
		return false;
	}
	if (is_fast) {
		if (signature == NULL) {
			hir_error(0, N_TR("Invalid __fast function prototype."));
			return false;
		}
		if (!signature->valid) {
			hir_error(0, N_TR("Invalid __fast function prototype."));
			return false;
		}
		if (!fast_signature_valid(signature)) {
			hir_error(0, N_TR("Invalid __fast function prototype."));
			return false;
		}
	}

	prototype = fast_checked_find_prototype(name);
	if (prototype != NULL) {
		compatible = prototype->is_fast == is_fast;
		if (compatible && is_fast) {
			compatible = fast_checked_signature_equal(
				&prototype->signature,
				signature);
		}

		if (!compatible) {
			snprintf(
				message,
				sizeof(message),
				N_TR("Incompatible duplicate __fast prototype '%s'."),
				name);
			hir_error(0, message);
			return false;
		}

		return true;
	}

	if (fast_checked_prototype_count ==
	    fast_checked_prototype_capacity) {
		capacity = fast_checked_prototype_capacity == 0 ?
			FAST_CHECKED_PROTOTYPE_INITIAL :
			fast_checked_prototype_capacity * 2;
		if (capacity < fast_checked_prototype_capacity ||
		    capacity > UINT32_MAX / sizeof(*table)) {
			hir_error(
				0,
				N_TR("Too many function prototypes in the require graph."));
			return false;
		}

		table = noct_realloc(
			fast_checked_prototype_table,
			(size_t)capacity * sizeof(*table));
		if (table == NULL) {
			hir_out_of_memory();
			return false;
		}

		fast_checked_prototype_table = table;
		fast_checked_prototype_capacity = capacity;
	}

	prototype = &fast_checked_prototype_table[
		fast_checked_prototype_count];
	memset(prototype, 0, sizeof(*prototype));
	fast_signature_init(&prototype->signature);

	prototype->name = noct_strdup(name);
	if (prototype->name == NULL) {
		hir_out_of_memory();
		return false;
	}

	prototype->is_fast = is_fast;
	if (is_fast) {
		if (!fast_signature_clone(&prototype->signature, signature)) {
			noct_free(prototype->name);
			prototype->name = NULL;
			fast_signature_free(&prototype->signature);
			hir_out_of_memory();
			return false;
		}
	}

	fast_checked_prototype_count++;

	return true;
}

/*
 * Validates every fast function and installs its checked index lowering.
 */
bool
hir_fast_checked_module(
	struct hir_block *const *func_table,
	uint32_t func_count)
{
	struct fast_checked_module module;
	uint32_t i;
	bool result;

	if (func_count > 0 && func_table == NULL)
		return false;

	memset(&module, 0, sizeof(module));
	module.func_table = func_table;
	module.func_count = func_count;
	result = true;

	/* Build every optimizer-owned signature before validating calls. */
	for (i = 0; i < func_count; i++) {
		if (!fast_checked_build_hir_signature(func_table[i])) {
			result = false;
			break;
		}
	}

	/* Reject the multi-index syntax outside its shaped fast contract. */
	if (result) {
		/* Inspect every ordinary function for residual multi-index syntax. */
		for (i = 0; i < func_count; i++) {
			if (func_table[i]->val.func.is_fast)
				continue;
			if (!fast_checked_reject_nonfast_multi(func_table[i])) {
				result = false;
				break;
			}
		}
	}

	/* Validate and lower each fast function after all prototypes exist. */
	if (result) {
		/* Visit every function that carries a fast contract. */
		for (i = 0; i < func_count; i++) {
			if (!func_table[i]->val.func.is_fast)
				continue;
			if (!fast_checked_validate_function(&module, func_table[i])) {
				result = false;
				break;
			}
		}
	}

	if (result)
		result = fast_checked_validate_cycles(&module);
	if (result)
		result = fast_checked_validate_prototype_cycles();

	noct_free(module.edge);

	return result;
}

/*
 * Releases optimizer-owned metadata attached to one HIR function.
 */
void
hir_fast_checked_cleanup_func(
	struct hir_block *func)
{
	if (func == NULL || func->type != HIR_BLOCK_FUNC)
		return;

	fast_info_free(func->val.func.fast_info);
	func->val.func.fast_info = NULL;
	func->val.func.fast_optimized = false;
}

/* Find one externally collected function prototype. */
static struct fast_checked_prototype *
fast_checked_find_prototype(
	const char *name)
{
	uint32_t i;

	/* Search every collected prototype by its complete link name. */
	for (i = 0; i < fast_checked_prototype_count; i++) {
		if (strcmp(fast_checked_prototype_table[i].name, name) == 0)
			return &fast_checked_prototype_table[i];
	}

	return NULL;
}

/* Build one prototype contract directly from its source AST declaration. */
static bool
fast_checked_build_ast_signature(
	const struct ast_func *func,
	struct fast_signature *signature)
{
	struct ast_param *param;
	const char *param_name[HIR_PARAM_SIZE];
	const char *param_annotation[HIR_PARAM_SIZE];
	int param_type[HIR_PARAM_SIZE];
	int param_packed_type[HIR_PARAM_SIZE];
	bool param_restricted[HIR_PARAM_SIZE];
	char message[256];
	uint32_t param_count;
	int return_type;
	int return_packed_type;
	bool return_restricted;

	assert(func != NULL);
	assert(func->is_fast);
	assert(signature != NULL);

	param = func->param_list != NULL ? func->param_list->list : NULL;
	param_count = 0;

	/* Resolve every parameter without constructing its function body. */
	while (param != NULL) {
		if (param_count == HIR_PARAM_SIZE) {
			hir_error(0, N_TR("Exceeded the maximum parameter count."));
			return false;
		}

		param_name[param_count] = param->name;
		param_annotation[param_count] = param->type_name;
		if (!hir_resolve_type_annotation(
			0,
			param->type_name,
			true,
			&param_type[param_count],
			&param_packed_type[param_count],
			&param_restricted[param_count])) {
			return false;
		}

		param_count++;
		param = param->next;
	}

	if (!hir_resolve_type_annotation(
		0,
		func->return_type_name,
		false,
		&return_type,
		&return_packed_type,
		&return_restricted)) {
		return false;
	}

	if (!fast_signature_build(
		signature,
		true,
		param_count,
		param_name,
		param_annotation,
		param_type,
		param_packed_type,
		param_restricted,
		func->return_type_name,
		return_type,
		message,
		sizeof(message))) {
		hir_error(0, message);
		return false;
	}

	return true;
}

/* Build and attach the optimizer-owned contract for one HIR function. */
static bool
fast_checked_build_hir_signature(
	struct hir_block *func)
{
	struct fast_signature candidate;
	struct fast_signature *signature;
	const char *param_name[HIR_PARAM_SIZE];
	const char *param_annotation[HIR_PARAM_SIZE];
	char message[256];
	uint32_t i;

	assert(func != NULL);
	assert(func->type == HIR_BLOCK_FUNC);

	fast_info_free(func->val.func.fast_info);
	func->val.func.fast_info = NULL;
	func->val.func.fast_optimized = false;
	if (!func->val.func.is_fast)
		return true;

	/* Borrow the source metadata retained by the target-neutral HIR. */
	for (i = 0; i < func->val.func.param_count; i++) {
		param_name[i] = func->val.func.param_name[i];
		param_annotation[i] = func->val.func.param_type_name[i];
	}

	fast_signature_init(&candidate);
	if (!fast_signature_build(
		&candidate,
		true,
		func->val.func.param_count,
		param_name,
		param_annotation,
		func->val.func.param_type,
		func->val.func.param_packed_type,
		func->val.func.param_restricted,
		func->val.func.return_type_name,
		func->val.func.return_type,
		message,
		sizeof(message))) {
		hir_error(func->line, message);
		return false;
	}

	signature = noct_malloc(sizeof(*signature));
	if (signature == NULL) {
		fast_signature_free(&candidate);
		hir_out_of_memory();
		return false;
	}

	*signature = candidate;
	func->val.func.fast_info = signature;

	return true;
}

/* Read a HIR function's optimizer-owned contract. */
static const struct fast_signature *
fast_checked_get_signature(
	const struct hir_block *func)
{
	assert(func != NULL);
	assert(func->type == HIR_BLOCK_FUNC);

	return fast_info_signature(func->val.func.fast_info);
}

/* Compare every owned field of two fast signatures. */
static bool
fast_checked_signature_equal(
	const struct fast_signature *first,
	const struct fast_signature *second)
{
	const struct fast_param_contract *first_param;
	const struct fast_param_contract *second_param;
	const struct fast_extent *first_extent;
	const struct fast_extent *second_extent;
	uint32_t i;
	uint32_t axis;

	if (first == second)
		return true;
	if (first == NULL || second == NULL)
		return false;
	if (first->version != second->version)
		return false;
	if (first->valid != second->valid)
		return false;
	if (first->param_count != second->param_count)
		return false;
	if (first->return_type != second->return_type)
		return false;

	/* Compare every parameter contract and exact shape. */
	for (i = 0; i < first->param_count; i++) {
		first_param = &first->param[i];
		second_param = &second->param[i];

		if (first_param->value_type != second_param->value_type)
			return false;
		if (first_param->packed_type != second_param->packed_type)
			return false;
		if (first_param->restricted != second_param->restricted)
			return false;
		if (first_param->rank != second_param->rank)
			return false;

		/* Compare every constant or parameter-dependent extent. */
		for (axis = 0; axis < first_param->rank; axis++) {
			first_extent = &first_param->extent[axis];
			second_extent = &second_param->extent[axis];

			if (first_extent->kind != second_extent->kind)
				return false;
			if (first_extent->kind == FAST_EXTENT_CONST &&
			    first_extent->value.constant !=
				second_extent->value.constant) {
				return false;
			}
			if (first_extent->kind == FAST_EXTENT_PARAM &&
			    first_extent->value.param_index !=
				second_extent->value.param_index) {
				return false;
			}
		}
	}

	return true;
}

/* Append one non-static fast call edge to the prototype graph. */
static bool
fast_checked_add_prototype_edge(
	const char *caller_name,
	const char *callee_name,
	int line)
{
	struct fast_checked_prototype *caller;
	struct fast_checked_prototype *callee;
	struct fast_checked_edge *edge;
	uint32_t caller_index;
	uint32_t callee_index;
	uint32_t capacity;
	uint32_t i;

	caller = fast_checked_find_prototype(caller_name);
	callee = fast_checked_find_prototype(callee_name);
	if (caller == NULL || callee == NULL)
		return true;
	if (!caller->is_fast || !callee->is_fast)
		return true;

	caller_index = (uint32_t)(caller - fast_checked_prototype_table);
	callee_index = (uint32_t)(callee - fast_checked_prototype_table);

	/* Ignore an edge already recorded while validating this graph. */
	for (i = 0; i < fast_checked_prototype_edge_count; i++) {
		edge = &fast_checked_prototype_edge_table[i];
		if (edge->caller == caller_index &&
		    edge->callee == callee_index) {
			return true;
		}
	}

	if (fast_checked_prototype_edge_count ==
	    fast_checked_prototype_edge_capacity) {
		capacity = fast_checked_prototype_edge_capacity == 0 ?
			FAST_CHECKED_EDGE_INITIAL :
			fast_checked_prototype_edge_capacity * 2;
		if (capacity < fast_checked_prototype_edge_capacity ||
		    capacity > UINT32_MAX / sizeof(*edge)) {
			hir_error(line, N_TR("Too many direct __fast call edges."));
			return false;
		}

		edge = noct_realloc(
			fast_checked_prototype_edge_table,
			(size_t)capacity * sizeof(*edge));
		if (edge == NULL) {
			hir_out_of_memory();
			return false;
		}

		fast_checked_prototype_edge_table = edge;
		fast_checked_prototype_edge_capacity = capacity;
	}

	edge = &fast_checked_prototype_edge_table[
		fast_checked_prototype_edge_count];
	edge->caller = caller_index;
	edge->callee = callee_index;
	edge->line = line;
	fast_checked_prototype_edge_count++;

	return true;
}

/* Reject recursion spanning non-static prototypes in the require graph. */
static bool
fast_checked_validate_prototype_cycles(
	void)
{
	unsigned char *state;
	uint32_t i;
	bool result;

	state = noct_calloc(fast_checked_prototype_count, sizeof(*state));
	if (state == NULL && fast_checked_prototype_count > 0) {
		hir_out_of_memory();
		return false;
	}

	result = true;

	/* Start a depth-first search at every unvisited fast prototype. */
	for (i = 0; i < fast_checked_prototype_count; i++) {
		if (!fast_checked_prototype_table[i].is_fast || state[i] != 0)
			continue;
		if (!fast_checked_visit_prototype_cycle(i, state)) {
			result = false;
			break;
		}
	}

	noct_free(state);

	return result;
}

/* Visit one node in the require graph's fast prototype call graph. */
static bool
fast_checked_visit_prototype_cycle(
	uint32_t prototype,
	unsigned char *state)
{
	const struct fast_checked_edge *edge;
	uint32_t i;

	state[prototype] = 1;

	/* Follow every outgoing call to a non-static fast prototype. */
	for (i = 0; i < fast_checked_prototype_edge_count; i++) {
		edge = &fast_checked_prototype_edge_table[i];
		if (edge->caller != prototype)
			continue;
		if (state[edge->callee] == 1) {
			hir_error(
				edge->line,
				N_TR("Direct or mutually recursive __fast calls are not supported."));
			return false;
		}
		if (state[edge->callee] == 0) {
			if (!fast_checked_visit_prototype_cycle(
				edge->callee,
				state)) {
				return false;
			}
		}
	}

	state[prototype] = 2;

	return true;
}

/* Validate one complete fast function. */
static bool
fast_checked_validate_function(
	struct fast_checked_module *module,
	struct hir_block *func)
{
	struct fast_checked_context context;
	const struct fast_signature *signature;

	assert(module != NULL);
	assert(func != NULL);
	assert(func->type == HIR_BLOCK_FUNC);

	signature = fast_checked_get_signature(func);
	if (signature == NULL ||
	    !signature->valid ||
	    signature->version != NOCT_FAST_SIGNATURE_VERSION ||
	    signature->param_count != func->val.func.param_count) {
		hir_error(0, N_TR("Invalid __fast function signature."));
		return false;
	}

	if (fast_checked_intrinsic_name(func->val.func.name) ||
	    strncmp(func->val.func.name, "$Fast", 5) == 0) {
		hir_error(0, N_TR("A __fast function name conflicts with a compiler intrinsic."));
		return false;
	}

	if (func->val.func.return_type != HIR_TYPE_VOID &&
	    !func->val.func.returns_on_all_paths) {
		hir_error(
			0,
			N_TR("Every reachable path of a non-void __fast func must return a value."));
		return false;
	}

	context.module = module;
	context.func = func;

	if (!fast_checked_validate_locals(&context))
		return false;

	return fast_checked_chain(&context, func->val.func.inner);
}

/* Validate the source declarations retained on every local. */
static bool
fast_checked_validate_locals(
	struct fast_checked_context *context)
{
	struct hir_local *local;

	local = context->func->val.func.local;

	/* Require an exact primitive spelling on every explicit local. */
	while (local != NULL) {
		if ((local->declaration_kind == HIR_LOCAL_DECL_LET ||
		     local->declaration_kind == HIR_LOCAL_DECL_VAR) &&
		    !fast_checked_local_spelling(local)) {
			hir_error(
				local->declaration_line,
				N_TR("Every local in a __fast func requires an exact primitive type annotation."));
			return false;
		}

		local = local->next;
	}

	return true;
}

/* Validate every block in one structured successor chain. */
static bool
fast_checked_chain(
	struct fast_checked_context *context,
	struct hir_block *head)
{
	struct hir_block *block;
	struct hir_block *branch;
	struct hir_stmt *statement;
	struct hir_local *counter;
	int type;
	int stop_type;

	block = head;

	/* Walk the structured HIR without following loop back-edges. */
	while (block != NULL) {
		/* Dispatch the expressions owned by this block shape. */
		switch (block->type) {
		case HIR_BLOCK_BASIC:
			statement = block->val.basic.stmt_list;

			/* Validate every statement in source order. */
			while (statement != NULL) {
				if (!fast_checked_statement(context, statement))
					return false;
				statement = statement->next;
			}
			break;
		case HIR_BLOCK_IF:
			branch = block;

			/* Validate every arm of the conditional chain. */
			while (branch != NULL) {
				if (branch->val.if_.cond != NULL) {
					if (!fast_checked_expr(
						context,
						&branch->val.if_.cond,
						branch->line,
						0,
						&type))
						return false;
					if (type != NOCT_VALUE_INT) {
						hir_error(
							branch->line,
							N_TR("A __fast condition must have type int."));
						return false;
					}
				}

				if (!fast_checked_chain(
					context,
					branch->val.if_.inner))
					return false;
				branch = branch->val.if_.chain_next;
			}
			break;
		case HIR_BLOCK_FOR:
			if (!block->val.for_.is_ranged) {
				hir_error(
					block->line,
					N_TR("A __fast func supports only ranged for loops."));
				return false;
			}

			if (!fast_checked_expr(
				context,
				&block->val.for_.start,
				block->line,
				0,
				&type)) {
				return false;
			}
			if (!fast_checked_expr(
				context,
				&block->val.for_.stop,
				block->line,
				0,
				&stop_type)) {
				return false;
			}

			if (!fast_checked_integer(type) || type != stop_type) {
				hir_error(
					block->line,
					N_TR("A __fast ranged loop requires matching int or long bounds."));
				return false;
			}

			counter = fast_checked_find_local(
				context->func,
				block->val.for_.counter_symbol);
			if (counter == NULL ||
			    counter->declaration_kind != HIR_LOCAL_DECL_LOOP_COUNTER) {
				hir_error(block->line, N_TR("Invalid __fast loop counter."));
				return false;
			}
			counter->declared_type = type;

			if (!fast_checked_chain(context, block->val.for_.inner))
				return false;
			break;
		case HIR_BLOCK_WHILE:
			if (!fast_checked_expr(
				context,
				&block->val.while_.cond,
				block->line,
				0,
				&type))
				return false;
			if (type != NOCT_VALUE_INT) {
				hir_error(
					block->line,
					N_TR("A __fast condition must have type int."));
				return false;
			}
			if (!fast_checked_chain(context, block->val.while_.inner))
				return false;
			break;
		case HIR_BLOCK_END:
			return true;
		default:
			hir_error(block->line, N_TR("Unsupported block in __fast func."));
			return false;
		}

		if (block->stop)
			break;
		block = block->succ;
	}

	return true;
}

/* Validate one statement and its exact assignment contract. */
static bool
fast_checked_statement(
	struct fast_checked_context *context,
	struct hir_stmt *statement)
{
	struct hir_local *local;
	const char *symbol;
	int left_type;
	int right_type;

	if (statement->lhs == NULL) {
		return fast_checked_expr(
			context,
			&statement->rhs,
			statement->line,
			0,
			&right_type);
	}

	if (statement->lhs->type == HIR_EXPR_TERM &&
	    statement->lhs->val.term.term->type == HIR_TERM_SYMBOL) {
		symbol = statement->lhs->val.term.term->val.symbol;
		if (strcmp(symbol, "$return") == 0) {
			if (statement->is_bare_return) {
				if (context->func->val.func.return_type != HIR_TYPE_VOID) {
					hir_error(
						statement->line,
						N_TR("A non-void __fast func must return a value."));
					return false;
				}

				return true;
			}

			if (!fast_checked_expr(
				context,
				&statement->rhs,
				statement->line,
				0,
				&right_type))
				return false;
			if (right_type != context->func->val.func.return_type) {
				hir_error(
					statement->line,
					N_TR("A __fast return value does not match its declared type."));
				return false;
			}

			return true;
		}

		local = fast_checked_find_local(context->func, symbol);
		if (local == NULL || !fast_checked_primitive(local->declared_type)) {
			hir_error(
				statement->line,
				N_TR("A __fast assignment target must be a typed local."));
			return false;
		}

		if (!fast_checked_expr(
			context,
			&statement->rhs,
			statement->line,
			0,
			&right_type))
			return false;
		if (right_type != local->declared_type) {
			hir_error(
				statement->line,
				N_TR("A __fast assignment requires an exact type match."));
			return false;
		}

		return true;
	}

	if (statement->lhs->type != HIR_EXPR_SUBSCR) {
		hir_error(
			statement->line,
			N_TR("Unsupported assignment target inside __fast func."));
		return false;
	}

	if (!fast_checked_expr(
		context,
		&statement->lhs,
		statement->line,
		0,
		&left_type)) {
		return false;
	}
	if (!fast_checked_expr(
		context,
		&statement->rhs,
		statement->line,
		0,
		&right_type)) {
		return false;
	}
	if (left_type != right_type) {
		hir_error(
			statement->line,
			N_TR("A __fast packed store requires an exact element type match."));
		return false;
	}

	return true;
}

/* Infer and validate one expression under the fast subset. */
static bool
fast_checked_expr(
	struct fast_checked_context *context,
	struct hir_expr **expr,
	int line,
	uint32_t depth,
	int *type)
{
	int operand;

	if (expr == NULL ||
	    *expr == NULL ||
	    type == NULL)
		return false;
	if (depth > FAST_CHECKED_EXPR_DEPTH_MAX) {
		hir_error(line, N_TR("A __fast expression is too deeply nested."));
		return false;
	}

	/* Validate the expression shape and compute its exact result type. */
	switch ((*expr)->type) {
	case HIR_EXPR_TERM:
		return fast_checked_term(
			context,
			(*expr)->val.term.term,
			line,
			type);
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
		return fast_checked_binary(
			context,
			*expr,
			line,
			depth + 1,
			type);
	case HIR_EXPR_NEG:
	case HIR_EXPR_PAR:
		if (!fast_checked_expr(
			context,
			&(*expr)->val.unary.expr,
			line,
			depth + 1,
			&operand))
			return false;
		if (!fast_checked_primitive(operand)) {
			hir_error(line, N_TR("A __fast unary operand must be primitive."));
			return false;
		}
		*type = operand;
		return true;
	case HIR_EXPR_NOT:
		if (!fast_checked_expr(
			context,
			&(*expr)->val.unary.expr,
			line,
			depth + 1,
			&operand))
			return false;
		if (operand != NOCT_VALUE_INT) {
			hir_error(line, N_TR("A __fast logical operand must have type int."));
			return false;
		}
		*type = NOCT_VALUE_INT;
		return true;
	case HIR_EXPR_SUBSCR:
		return fast_checked_subscript(
			context,
			*expr,
			line,
			depth + 1,
			type);
	case HIR_EXPR_CALL:
		return fast_checked_call(
			context,
			*expr,
			line,
			depth + 1,
			type);
	default:
		hir_error(
			line,
			N_TR("This expression is not available inside __fast func."));
		return false;
	}
}

/* Validate one literal or local symbol. */
static bool
fast_checked_term(
	struct fast_checked_context *context,
	const struct hir_term *term,
	int line,
	int *type)
{
	struct hir_local *local;

	if (term == NULL)
		return false;

	/* Classify literals and resolve local symbols. */
	switch (term->type) {
	case HIR_TERM_INT:
		*type = NOCT_VALUE_INT;
		return true;
	case HIR_TERM_LONG:
		*type = NOCT_VALUE_LONG;
		return true;
	case HIR_TERM_FLOAT:
		*type = NOCT_VALUE_FLOAT;
		return true;
	case HIR_TERM_DOUBLE:
		*type = NOCT_VALUE_DOUBLE;
		return true;
	case HIR_TERM_SYMBOL:
		local = fast_checked_find_local(context->func, term->val.symbol);
		if (local == NULL) {
			char message[256];

			snprintf(
				message,
				sizeof(message),
				N_TR("Global symbol '%s' is not available inside __fast func."),
				term->val.symbol);
			hir_error(line, message);
			return false;
		}
		if (local->declared_type == NOCT_VALUE_PACKED &&
		    local->declared_packed_type >= 0) {
			*type = NOCT_VALUE_PACKED;
			return true;
		}
		if (!fast_checked_primitive(local->declared_type)) {
			hir_error(
				line,
				N_TR("A local used inside __fast func must have an exact primitive type."));
			return false;
		}
		*type = local->declared_type;
		return true;
	default:
		hir_error(line, N_TR("Only numeric literals are available inside __fast func."));
		return false;
	}
}

/* Validate an exact-type binary operation. */
static bool
fast_checked_binary(
	struct fast_checked_context *context,
	struct hir_expr *expr,
	int line,
	uint32_t depth,
	int *type)
{
	int left;
	int right;

	if (!fast_checked_expr(
		context,
		&expr->val.binary.expr[0],
		line,
		depth,
		&left)) {
		return false;
	}
	if (!fast_checked_expr(
		context,
		&expr->val.binary.expr[1],
		line,
		depth,
		&right)) {
		return false;
	}

	if (left != right || !fast_checked_primitive(left)) {
		hir_error(
			line,
			N_TR("A __fast binary operation requires equal primitive operand types."));
		return false;
	}

	/* Enforce the operator-specific primitive subset. */
	switch (expr->type) {
	case HIR_EXPR_LT:
	case HIR_EXPR_LTE:
	case HIR_EXPR_GT:
	case HIR_EXPR_GTE:
	case HIR_EXPR_EQ:
	case HIR_EXPR_NEQ:
		*type = NOCT_VALUE_INT;
		return true;
	case HIR_EXPR_LAND:
	case HIR_EXPR_LOR:
		if (left != NOCT_VALUE_INT) {
			hir_error(line, N_TR("A __fast logical operand must have type int."));
			return false;
		}
		*type = NOCT_VALUE_INT;
		return true;
	case HIR_EXPR_MOD:
	case HIR_EXPR_AND:
	case HIR_EXPR_OR:
	case HIR_EXPR_XOR:
	case HIR_EXPR_SHL:
	case HIR_EXPR_SHR:
		if (!fast_checked_integer(left)) {
			hir_error(line, N_TR("A __fast integer operation requires int or long."));
			return false;
		}
		*type = left;
		return true;
	default:
		*type = left;
		return true;
	}
}

/* Validate a shaped packed access and lower a multi-index access. */
static bool
fast_checked_subscript(
	struct fast_checked_context *context,
	struct hir_expr *expr,
	int line,
	uint32_t depth,
	int *type)
{
	struct hir_expr *base;
	struct hir_expr *index;
	struct hir_local *local;
	const struct fast_param_contract *contract;
	int parameter;
	int index_type;
	uint32_t count;
	uint32_t i;

	base = expr->val.binary.expr[0];
	index = expr->val.binary.expr[1];
	if (base == NULL ||
	    base->type != HIR_EXPR_TERM ||
	    base->val.term.term->type != HIR_TERM_SYMBOL) {
		hir_error(
			line,
			N_TR("A __fast subscript base must be a shaped rpacked parameter."));
		return false;
	}

	parameter = fast_checked_find_param(
		context->func,
		base->val.term.term->val.symbol);
	if (parameter < 0) {
		hir_error(
			line,
			N_TR("A __fast subscript base must be a shaped rpacked parameter."));
		return false;
	}

	local = fast_checked_find_local(
		context->func,
		base->val.term.term->val.symbol);
	contract = &fast_checked_get_signature(context->func)->param[parameter];
	if (local == NULL ||
	    local->declared_type != NOCT_VALUE_PACKED ||
	    contract->rank == 0 ||
	    contract->extent == NULL) {
		hir_error(
			line,
			N_TR("A __fast subscript base must be a shaped rpacked parameter."));
		return false;
	}

	if (index->type == HIR_EXPR_ARRAY &&
	    index->val.array.is_multi_index) {
		count = (uint32_t)index->val.array.elem_count;
		if (count != contract->rank) {
			hir_error(
				line,
				N_TR("The number of indices does not match the __fast parameter rank."));
			return false;
		}

		/* Validate every axis before constructing the checked helper call. */
		for (i = 0; i < count; i++) {
			if (!fast_checked_expr(
				context,
				&index->val.array.elem[i],
				line,
				depth,
				&index_type))
				return false;
			if (!fast_checked_integer(index_type)) {
				hir_error(
					line,
					N_TR("A __fast array index must be int or long."));
				return false;
			}
		}

		if (!fast_checked_lower_multi_index(context, expr, contract, line))
			return false;
	} else {
		if (contract->rank != 1) {
			hir_error(
				line,
				N_TR("The number of indices does not match the __fast parameter rank."));
			return false;
		}
		if (!fast_checked_expr(
			context,
			&expr->val.binary.expr[1],
			line,
			depth,
			&index_type))
			return false;
		if (!fast_checked_integer(index_type)) {
			hir_error(line, N_TR("A __fast array index must be int or long."));
			return false;
		}
	}

	*type = fast_checked_packed_type(contract->packed_type);
	if (*type == FAST_CHECKED_TYPE_ERROR) {
		hir_error(line, N_TR("Unsupported packed element type in __fast func."));
		return false;
	}

	return true;
}

/* Validate a direct fast call or a source-level fast intrinsic. */
static bool
fast_checked_call(
	struct fast_checked_context *context,
	struct hir_expr *expr,
	int line,
	uint32_t depth,
	int *type)
{
	struct hir_expr *function;
	struct hir_block *callee;
	struct fast_checked_prototype *prototype;
	const struct fast_signature *signature;
	const char *name;
	int callee_index;
	int argument_type;
	uint32_t caller_index;
	uint32_t i;

	function = expr->val.call.func;
	if (function == NULL ||
	    function->type != HIR_EXPR_TERM ||
	    function->val.term.term->type != HIR_TERM_SYMBOL) {
		hir_error(
			line,
			N_TR("A __fast func may call only a direct __fast function or intrinsic."));
		return false;
	}

	name = function->val.term.term->val.symbol;
	callee_index = fast_checked_find_function(context->module, name);
	prototype = NULL;
	if (callee_index < 0)
		prototype = fast_checked_find_prototype(name);

	if (callee_index < 0 && prototype == NULL) {
		if (fast_checked_intrinsic_name(name)) {
			return fast_checked_intrinsic(
				context,
				expr,
				name,
				line,
				depth,
				type);
		}
	}

	callee = NULL;
	signature = NULL;
	if (callee_index >= 0) {
		callee = context->module->func_table[callee_index];
		if (callee->val.func.is_fast)
			signature = fast_checked_get_signature(callee);
	} else if (prototype != NULL && prototype->is_fast) {
		signature = &prototype->signature;
	}

	if (signature == NULL) {
		char message[256];

		snprintf(
			message,
			sizeof(message),
			N_TR("Call to non-fast function '%s' is not allowed inside __fast func."),
			name);
		hir_error(line, message);
		return false;
	}

	if (!signature->valid ||
	    signature->param_count != expr->val.call.arg_count) {
		hir_error(line, N_TR("A direct __fast call has the wrong argument count."));
		return false;
	}

	/* Validate every exact argument and shaped view. */
	for (i = 0; i < expr->val.call.arg_count; i++) {
		if (!fast_checked_expr(
			context,
			&expr->val.call.arg[i],
			line,
			depth,
			&argument_type))
			return false;
		if (argument_type != signature->param[i].value_type) {
			hir_error(
				line,
				N_TR("A direct __fast call argument does not match the callee type."));
			return false;
		}
		if (argument_type == NOCT_VALUE_PACKED) {
			if (!fast_checked_validate_call_shape(
				context,
				signature,
				expr,
				i,
				line)) {
				return false;
			}
		}
	}

	if (callee_index >= 0) {
		caller_index = (uint32_t)fast_checked_find_function(
			context->module,
			context->func->val.func.name);
		if (!fast_checked_add_edge(
			context->module,
			caller_index,
			(uint32_t)callee_index,
			line)) {
			return false;
		}
	}

	if (!fast_checked_add_prototype_edge(
		context->func->val.func.name,
		name,
		line)) {
		return false;
	}

	*type = signature->return_type;

	return true;
}

/* Validate and rewrite one source-level fast intrinsic call. */
static bool
fast_checked_intrinsic(
	struct fast_checked_context *context,
	struct hir_expr *expr,
	const char *name,
	int line,
	uint32_t depth,
	int *type)
{
	uint32_t expected;
	int first;
	int second;

	expected = 1;
	if (strcmp(name, "min") == 0 ||
	    strcmp(name, "max") == 0 ||
	    strcmp(name, "atan2") == 0)
		expected = 2;

	if (expr->val.call.arg_count != expected) {
		hir_error(line, N_TR("Wrong number of arguments for __fast intrinsic."));
		return false;
	}

	if (!fast_checked_expr(
		context,
		&expr->val.call.arg[0],
		line,
		depth,
		&first))
		return false;
	if (!fast_checked_primitive(first)) {
		hir_error(
			line,
			N_TR("A __fast intrinsic requires a statically typed numeric argument."));
		return false;
	}

	if (expected == 2) {
		if (!fast_checked_expr(
			context,
			&expr->val.call.arg[1],
			line,
			depth,
			&second))
			return false;
		if (second != first) {
			hir_error(
				line,
				N_TR("A binary __fast intrinsic requires operands of the same type."));
			return false;
		}
	}

	if (fast_checked_intrinsic_transcendental(name) &&
	    first != NOCT_VALUE_FLOAT &&
	    first != NOCT_VALUE_DOUBLE) {
		hir_error(
			line,
			N_TR("A transcendental __fast intrinsic requires float or double."));
		return false;
	}

	if (!fast_checked_rewrite_intrinsic(expr, name))
		return false;

	if (strcmp(name, "int") == 0)
		*type = NOCT_VALUE_INT;
	else if (strcmp(name, "long") == 0)
		*type = NOCT_VALUE_LONG;
	else if (strcmp(name, "float") == 0)
		*type = NOCT_VALUE_FLOAT;
	else if (strcmp(name, "double") == 0)
		*type = NOCT_VALUE_DOUBLE;
	else
		*type = first;

	return true;
}

/* Replace a plain intrinsic symbol with its compiler-owned package name. */
static bool
fast_checked_rewrite_intrinsic(
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
	dot->val.dot.obj = fast_checked_symbol_expr("$FastMath");
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

/* Lower an exact-rank multi-index to the always-checked index helper. */
static bool
fast_checked_lower_multi_index(
	struct fast_checked_context *context,
	struct hir_expr *subscript,
	const struct fast_param_contract *contract,
	int line)
{
	struct hir_expr *array;
	struct hir_expr *call;
	struct hir_expr *dot;
	char helper[32];
	uint32_t i;

	UNUSED_PARAMETER(line);

	array = subscript->val.binary.expr[1];
	dot = hir_malloc(sizeof(*dot));
	if (dot == NULL) {
		hir_out_of_memory();
		return false;
	}

	memset(dot, 0, sizeof(*dot));
	dot->type = HIR_EXPR_DOT;
	dot->val.dot.obj = fast_checked_symbol_expr("$Fast");
	if (dot->val.dot.obj == NULL)
		return false;

	snprintf(helper, sizeof(helper), "index%u", contract->rank);
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
	call->val.call.arg_count = contract->rank * 2;

	/* Interleave each source index with its exact runtime extent. */
	for (i = 0; i < contract->rank; i++) {
		call->val.call.arg[i * 2] = array->val.array.elem[i];
		call->val.call.arg[i * 2 + 1] = fast_checked_extent_expr(
			context,
			&contract->extent[i]);
		if (call->val.call.arg[i * 2 + 1] == NULL)
			return false;
	}

	subscript->val.binary.expr[1] = call;

	return true;
}

/* Construct one constant or dynamic extent expression. */
static struct hir_expr *
fast_checked_extent_expr(
	struct fast_checked_context *context,
	const struct fast_extent *extent)
{
	struct hir_expr *expr;
	struct hir_term *term;

	if (extent->kind == FAST_EXTENT_PARAM) {
		if (extent->value.param_index >=
		    context->func->val.func.param_count) {
			hir_error(0, N_TR("Invalid dynamic __fast shape extent."));
			return NULL;
		}

		return fast_checked_symbol_expr(
			context->func->val.func.param_name[
				extent->value.param_index]);
	}

	if (extent->kind != FAST_EXTENT_CONST || extent->value.constant <= 0) {
		hir_error(0, N_TR("Invalid constant __fast shape extent."));
		return NULL;
	}

	term = hir_malloc(sizeof(*term));
	if (term == NULL) {
		hir_out_of_memory();
		return NULL;
	}

	memset(term, 0, sizeof(*term));
	if (extent->value.constant <= INT_MAX) {
		term->type = HIR_TERM_INT;
		term->val.i = (int)extent->value.constant;
	} else {
		term->type = HIR_TERM_LONG;
		term->val.l = extent->value.constant;
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

/* Construct a symbol expression in the HIR arena. */
static struct hir_expr *
fast_checked_symbol_expr(
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

/* Find one local by its scope-resolved symbol. */
static struct hir_local *
fast_checked_find_local(
	const struct hir_block *func,
	const char *symbol)
{
	struct hir_local *local;

	local = func->val.func.local;

	/* Search every local in the function. */
	while (local != NULL) {
		if (strcmp(local->symbol, symbol) == 0)
			return local;
		local = local->next;
	}

	return NULL;
}

/* Find one parameter by its scope-resolved symbol. */
static int
fast_checked_find_param(
	const struct hir_block *func,
	const char *symbol)
{
	uint32_t i;

	/* Search every function parameter. */
	for (i = 0; i < func->val.func.param_count; i++) {
		if (strcmp(func->val.func.param_name[i], symbol) == 0)
			return (int)i;
	}

	return -1;
}

/* Find one module-local function by its resolved name. */
static int
fast_checked_find_function(
	const struct fast_checked_module *module,
	const char *symbol)
{
	uint32_t i;

	/* Search every complete function prototype. */
	for (i = 0; i < module->func_count; i++) {
		if (strcmp(module->func_table[i]->val.func.name, symbol) == 0)
			return (int)i;
	}

	return -1;
}

/* Map a packed element kind to its scalar value tag. */
static int
fast_checked_packed_type(
	int packed_type)
{
	/* Map each supported packed representation to its load result. */
	switch (packed_type) {
	case NOCT_PACKED_INT8:
	case NOCT_PACKED_UINT8:
	case NOCT_PACKED_INT16:
	case NOCT_PACKED_UINT16:
	case NOCT_PACKED_INT32:
	case NOCT_PACKED_UINT32:
		return NOCT_VALUE_INT;
	case NOCT_PACKED_INT64:
	case NOCT_PACKED_UINT64:
		return NOCT_VALUE_LONG;
	case NOCT_PACKED_FLOAT32:
		return NOCT_VALUE_FLOAT;
	case NOCT_PACKED_FLOAT64:
		return NOCT_VALUE_DOUBLE;
	default:
		return FAST_CHECKED_TYPE_ERROR;
	}
}

/* Return whether a tag is an allowed primitive fast value. */
static bool
fast_checked_primitive(
	int type)
{
	return type == NOCT_VALUE_INT ||
	       type == NOCT_VALUE_LONG ||
	       type == NOCT_VALUE_FLOAT ||
	       type == NOCT_VALUE_DOUBLE;
}

/* Return whether a tag is an exact fast integer value. */
static bool
fast_checked_integer(
	int type)
{
	return type == NOCT_VALUE_INT || type == NOCT_VALUE_LONG;
}

/* Check an explicit local's exact source annotation spelling. */
static bool
fast_checked_local_spelling(
	const struct hir_local *local)
{
	if (local->declared_type_name == NULL)
		return false;
	if (local->declared_type == NOCT_VALUE_INT)
		return strcmp(local->declared_type_name, "int") == 0;
	if (local->declared_type == NOCT_VALUE_LONG)
		return strcmp(local->declared_type_name, "long") == 0;
	if (local->declared_type == NOCT_VALUE_FLOAT)
		return strcmp(local->declared_type_name, "float") == 0;
	if (local->declared_type == NOCT_VALUE_DOUBLE)
		return strcmp(local->declared_type_name, "double") == 0;

	return false;
}

/* Return whether a plain source name is a fast intrinsic. */
static bool
fast_checked_intrinsic_name(
	const char *name)
{
	size_t i;

	/* Search the stable compiler intrinsic table. */
	for (i = 0;
	     i < sizeof(fast_checked_intrinsic_names) /
		     sizeof(fast_checked_intrinsic_names[0]);
	     i++) {
		if (strcmp(name, fast_checked_intrinsic_names[i]) == 0)
			return true;
	}

	return false;
}

/* Return whether an intrinsic is restricted to float or double. */
static bool
fast_checked_intrinsic_transcendental(
	const char *name)
{
	if (strcmp(name, "sqrt") == 0)
		return true;
	if (strcmp(name, "sin") == 0)
		return true;
	if (strcmp(name, "cos") == 0)
		return true;
	if (strcmp(name, "tan") == 0)
		return true;
	if (strcmp(name, "asin") == 0)
		return true;
	if (strcmp(name, "acos") == 0)
		return true;
	if (strcmp(name, "atan") == 0)
		return true;
	if (strcmp(name, "atan2") == 0)
		return true;
	if (strcmp(name, "exp") == 0)
		return true;
	if (strcmp(name, "ln") == 0)
		return true;
	if (strcmp(name, "log2") == 0)
		return true;
	if (strcmp(name, "log10") == 0)
		return true;

	return false;
}

/* Validate one packed actual against the callee's exact shaped view. */
static bool
fast_checked_validate_call_shape(
	struct fast_checked_context *context,
	const struct fast_signature *callee,
	const struct hir_expr *call,
	uint32_t argument,
	int line)
{
	const struct fast_param_contract *formal;
	const struct fast_param_contract *actual;
	const struct hir_expr *expr;
	const char *symbol;
	int parameter;
	uint32_t axis;

	formal = &callee->param[argument];
	expr = call->val.call.arg[argument];
	if (expr->type != HIR_EXPR_TERM ||
	    expr->val.term.term->type != HIR_TERM_SYMBOL) {
		hir_error(
			line,
			N_TR("A direct __fast call packed argument must be a parameter."));
		return false;
	}

	symbol = expr->val.term.term->val.symbol;
	parameter = fast_checked_find_param(context->func, symbol);
	if (parameter < 0) {
		hir_error(
			line,
			N_TR("A direct __fast call packed argument must be a parameter."));
		return false;
	}

	actual = &fast_checked_get_signature(context->func)->param[parameter];
	if (actual->packed_type != formal->packed_type) {
		hir_error(
			line,
			N_TR("A direct __fast call packed argument has the wrong element type."));
		return false;
	}
	if (actual->rank != formal->rank) {
		hir_error(
			line,
			N_TR("A direct __fast call packed shape rank does not match."));
		return false;
	}

	/* Compare every callee extent after mapping its scalar arguments. */
	for (axis = 0; axis < actual->rank; axis++) {
		if (!fast_checked_extent_matches(
			context,
			&actual->extent[axis],
			&formal->extent[axis],
			call)) {
			hir_error(
				line,
				N_TR("A direct __fast call packed shape does not match the callee view."));
			return false;
		}
	}

	return true;
}

/* Compare one actual extent with a mapped formal extent. */
static bool
fast_checked_extent_matches(
	struct fast_checked_context *context,
	const struct fast_extent *actual,
	const struct fast_extent *formal,
	const struct hir_expr *call)
{
	const struct hir_expr *mapped;
	int parameter;

	if (formal->kind == FAST_EXTENT_CONST) {
		return actual->kind == FAST_EXTENT_CONST &&
		       actual->value.constant == formal->value.constant;
	}
	if (formal->kind != FAST_EXTENT_PARAM ||
	    formal->value.param_index >= call->val.call.arg_count)
		return false;

	mapped = call->val.call.arg[formal->value.param_index];
	if (actual->kind == FAST_EXTENT_CONST) {
		if (mapped->type != HIR_EXPR_TERM)
			return false;
		if (mapped->val.term.term->type == HIR_TERM_INT) {
			return actual->value.constant ==
			       mapped->val.term.term->val.i;
		}
		if (mapped->val.term.term->type == HIR_TERM_LONG) {
			return actual->value.constant ==
			       mapped->val.term.term->val.l;
		}

		return false;
	}
	if (actual->kind != FAST_EXTENT_PARAM ||
	    mapped->type != HIR_EXPR_TERM ||
	    mapped->val.term.term->type != HIR_TERM_SYMBOL)
		return false;

	parameter = fast_checked_find_param(
		context->func,
		mapped->val.term.term->val.symbol);

	return parameter >= 0 &&
	       actual->value.param_index == (uint32_t)parameter;
}

/* Append one direct fast call edge. */
static bool
fast_checked_add_edge(
	struct fast_checked_module *module,
	uint32_t caller,
	uint32_t callee,
	int line)
{
	struct fast_checked_edge *edge;
	uint32_t capacity;

	if (module->edge_count == module->edge_capacity) {
		capacity = module->edge_capacity == 0 ?
			FAST_CHECKED_EDGE_INITIAL :
			module->edge_capacity * 2;
		if (capacity < module->edge_capacity ||
		    capacity > UINT32_MAX / sizeof(*edge)) {
			hir_error(line, N_TR("Too many direct __fast call edges."));
			return false;
		}

		edge = noct_realloc(
			module->edge,
			(size_t)capacity * sizeof(*edge));
		if (edge == NULL) {
			hir_out_of_memory();
			return false;
		}

		module->edge = edge;
		module->edge_capacity = capacity;
	}

	module->edge[module->edge_count].caller = caller;
	module->edge[module->edge_count].callee = callee;
	module->edge[module->edge_count].line = line;
	module->edge_count++;

	return true;
}

/* Reject direct and mutual recursion in the fast call graph. */
static bool
fast_checked_validate_cycles(
	struct fast_checked_module *module)
{
	unsigned char *state;
	uint32_t i;
	bool result;

	state = noct_calloc(module->func_count, sizeof(*state));
	if (state == NULL && module->func_count > 0) {
		hir_out_of_memory();
		return false;
	}

	result = true;

	/* Start a depth-first search at every unvisited fast function. */
	for (i = 0; i < module->func_count; i++) {
		if (!module->func_table[i]->val.func.is_fast || state[i] != 0)
			continue;
		if (!fast_checked_visit_cycle(module, i, state)) {
			result = false;
			break;
		}
	}

	noct_free(state);

	return result;
}

/* Visit one node in the direct fast call graph. */
static bool
fast_checked_visit_cycle(
	struct fast_checked_module *module,
	uint32_t func,
	unsigned char *state)
{
	const struct fast_checked_edge *edge;
	uint32_t i;

	state[func] = 1;

	/* Follow every outgoing direct call. */
	for (i = 0; i < module->edge_count; i++) {
		edge = &module->edge[i];
		if (edge->caller != func)
			continue;
		if (state[edge->callee] == 1) {
			hir_error(
				edge->line,
				N_TR("Direct or mutually recursive __fast calls are not supported."));
			return false;
		}
		if (state[edge->callee] == 0) {
			if (!fast_checked_visit_cycle(
				module,
				edge->callee,
				state)) {
				return false;
			}
		}
	}

	state[func] = 2;

	return true;
}

/* Reject a multi-index expression in a non-fast function. */
static bool
fast_checked_reject_nonfast_multi(
	struct hir_block *func)
{
	int line;

	line = 0;
	if (!fast_checked_find_multi_chain(func->val.func.inner, &line))
		return true;

	if (line == 0)
		line = func->line;
	hir_error(
		line,
		N_TR("Multi-dimensional subscripts are valid only inside __fast func."));

	return false;
}

/* Find a multi-index expression in one structured block chain. */
static bool
fast_checked_find_multi_chain(
	struct hir_block *head,
	int *line)
{
	struct hir_block *block;
	struct hir_block *branch;
	struct hir_stmt *statement;

	block = head;

	/* Walk every block until this structured chain stops. */
	while (block != NULL) {
		/* Search the expressions owned by this block shape. */
		switch (block->type) {
		case HIR_BLOCK_BASIC:
			statement = block->val.basic.stmt_list;

			/* Search every statement expression. */
			while (statement != NULL) {
				if (fast_checked_find_multi_expr(statement->lhs)) {
					*line = statement->line;
					return true;
				}
				if (fast_checked_find_multi_expr(statement->rhs)) {
					*line = statement->line;
					return true;
				}
				statement = statement->next;
			}
			break;
		case HIR_BLOCK_IF:
			branch = block;

			/* Search every condition and branch body. */
			while (branch != NULL) {
				if (fast_checked_find_multi_expr(
					branch->val.if_.cond)) {
					if (*line == 0)
						*line = branch->line;
					return true;
				}
				if (fast_checked_find_multi_chain(
					branch->val.if_.inner,
					line)) {
					if (*line == 0)
						*line = branch->line;
					return true;
				}
				branch = branch->val.if_.chain_next;
			}
			break;
		case HIR_BLOCK_FOR:
			if (fast_checked_find_multi_expr(block->val.for_.start)) {
				if (*line == 0)
					*line = block->line;
				return true;
			}
			if (fast_checked_find_multi_expr(block->val.for_.stop)) {
				if (*line == 0)
					*line = block->line;
				return true;
			}
			if (fast_checked_find_multi_expr(
				block->val.for_.collection)) {
				if (*line == 0)
					*line = block->line;
				return true;
			}
			if (fast_checked_find_multi_chain(
				block->val.for_.inner,
				line)) {
				if (*line == 0)
					*line = block->line;
				return true;
			}
			break;
		case HIR_BLOCK_WHILE:
			if (fast_checked_find_multi_expr(block->val.while_.cond)) {
				if (*line == 0)
					*line = block->line;
				return true;
			}
			if (fast_checked_find_multi_chain(
				block->val.while_.inner,
				line)) {
				if (*line == 0)
					*line = block->line;
				return true;
			}
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

/* Find a subscript whose index was parsed as an index array. */
static bool
fast_checked_find_multi_expr(
	const struct hir_expr *expr)
{
	uint32_t i;

	if (expr == NULL)
		return false;

	/* Search recursively according to the source-HIR expression shape. */
	switch (expr->type) {
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
		if (fast_checked_find_multi_expr(expr->val.binary.expr[0]))
			return true;

		return fast_checked_find_multi_expr(expr->val.binary.expr[1]);
	case HIR_EXPR_SUBSCR:
		if (expr->val.binary.expr[1] != NULL &&
		    expr->val.binary.expr[1]->type == HIR_EXPR_ARRAY &&
		    expr->val.binary.expr[1]->val.array.is_multi_index)
			return true;
		if (fast_checked_find_multi_expr(expr->val.binary.expr[0]))
			return true;

		return fast_checked_find_multi_expr(expr->val.binary.expr[1]);
	case HIR_EXPR_NEG:
	case HIR_EXPR_NOT:
	case HIR_EXPR_PAR:
		return fast_checked_find_multi_expr(expr->val.unary.expr);
	case HIR_EXPR_DOT:
		return fast_checked_find_multi_expr(expr->val.dot.obj);
	case HIR_EXPR_CALL:
		if (fast_checked_find_multi_expr(expr->val.call.func))
			return true;

		/* Search every function argument. */
		for (i = 0; i < expr->val.call.arg_count; i++) {
			if (fast_checked_find_multi_expr(expr->val.call.arg[i]))
				return true;
		}
		return false;
	case HIR_EXPR_THISCALL:
		if (fast_checked_find_multi_expr(expr->val.thiscall.obj))
			return true;

		/* Search every method argument. */
		for (i = 0; i < expr->val.thiscall.arg_count; i++) {
			if (fast_checked_find_multi_expr(expr->val.thiscall.arg[i]))
				return true;
		}
		return false;
	case HIR_EXPR_ARRAY:
		/* Search every ordinary array-literal element. */
		for (i = 0; i < expr->val.array.elem_count; i++) {
			if (fast_checked_find_multi_expr(expr->val.array.elem[i]))
				return true;
		}
		return false;
	case HIR_EXPR_DICT:
		/* Search every ordinary dictionary value. */
		for (i = 0; i < expr->val.dict.kv_count; i++) {
			if (fast_checked_find_multi_expr(expr->val.dict.value[i]))
				return true;
		}
		return false;
	case HIR_EXPR_NEW:
		return fast_checked_find_multi_expr(expr->val.new_.init);
	default:
		return false;
	}
}
