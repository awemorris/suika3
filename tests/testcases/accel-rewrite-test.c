/* -*- coding: utf-8; tab-width: 8; indent-tabs-mode: t; -*- */

/*
 * Noct Programming Language
 * Copyright (c) 2025, 2026, Awe Morris
 */

/*
 * Focused transactional accelerator rewrite tests.
 */

#include "accel_context.h"
#include "accel_lir_budget.h"
#include "accel_private.h"
#include "accel_program.h"
#include "accel_rewrite.h"
#include "accel_test_backend.h"
#include "ast.h"
#include "hir.h"
#include "hir_opt.h"
#include "lir.h"
#include "runtime.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ACCEL_REWRITE_STRESS_COUNT	40

struct accel_rewrite_test_session {
	struct accel_live_session live;
	uint32_t *orphan_count;
};

struct accel_zero_parameter_test_state {
	struct accel_context *context;
	struct accel_function_plan *plan;
	struct accel_prepared_program prepared[1];
	struct accel_registry_reservation *reservation;
	struct accel_registry_commit_guard guard;
	struct accel_rewrite *rewrite;
	struct accel_program *owned_program;
	struct accel_ir_kernel *ir;
};

static char *read_source(const char *directory, const char *name);
static bool build_case(const char *directory, const char *name, struct hir_block **func_block);
static void cleanup_case(void);
static struct hir_block *find_accel_function(void);
static uint32_t count_locals(const struct hir_block *func_block);
static struct hir_local *find_local(struct hir_block *func_block, const char *symbol);
static bool create_context(struct rt_vm *vm, struct accel_test_backend_observer *observer, struct accel_context **context);
static void destroy_context(struct accel_context *context);
static bool expression_is_symbol(const struct hir_expr *expression, const char *symbol);
static bool expression_is_integer(const struct hir_expr *expression, int *value);
static bool expression_is_subscript(const struct hir_expr *expression, const char *symbol, int index);
static bool expression_is_thiscall(const struct hir_expr *expression, const char *function_name);
static bool block_is_reachable(const struct hir_block *func_block, const struct hir_block *target);
static bool inspect_applied_shape(struct hir_block *func_block, struct hir_block *replacement, struct hir_block *after, uint32_t *program_id);
static bool inspect_multi_region_shape(struct hir_block *func_block, struct hir_block *replacement, struct hir_block *after, uint32_t region_index, uint32_t kernel_count, uint32_t *program_id);
static bool inspect_dosum_shape(struct hir_block *func_block, struct hir_block *replacement, struct hir_block *after, uint32_t *program_id);
static bool inspect_transparent_dosum_shape(struct hir_block *func_block, struct hir_block *replacement, struct hir_block *after, struct hir_local *local, const struct hir_stmt *source_initializer, uint32_t *program_id);
static bool inspect_split_dosum_shape(struct hir_block *func_block, struct hir_block *replacement, struct hir_block *after, uint32_t *program_id);
static bool stress_registry(struct accel_context *context, const struct accel_prepared_program *first, uint32_t first_id);
static void cleanup_stress_registry(struct accel_context *context, struct accel_prepared_program prepared[], uint32_t prepared_count, struct accel_prepared_program *final_program, struct accel_registry_reservation *reservation, struct accel_registry_reservation *cancelled, struct accel_registry_reservation *final_reservation, struct accel_registry_commit_guard *guard);
static bool run_applied_case(const char *directory);
static bool run_device_only_case(const char *directory);
static bool run_local_link_guard_case(const char *directory);
static bool run_multi_region_case(const char *directory);
static bool run_dosum_case(const char *directory);
static bool run_transparent_dosum_case(const char *directory);
static bool run_dosum_prefix_split_case(const char *directory);
static bool run_zero_parameter_case(const char *directory);
static void cleanup_zero_parameter_failure(struct accel_zero_parameter_test_state *state);
static bool run_compile_decline_case(const char *directory);
static bool run_multi_region_decline_case(const char *directory, const char *name);
static bool run_backend_decline_case(const char *directory);
static bool run_budget_case(const char *directory);
static void *orphan_test_session(struct accel_live_session *session);
static void destroy_test_orphan(void *payload);

/*
 * Runs the focused transactional accelerator rewrite tests.
 */
int
main(
	int argc,
	char *argv[])
{
	if (argc != 2) {
		fprintf(stderr, "usage: %s CASE-DIRECTORY\n", argv[0]);
		return 2;
	}

	if (!run_applied_case(argv[1]))
		return 1;
	if (!run_device_only_case(argv[1]))
		return 1;
	if (!run_local_link_guard_case(argv[1]))
		return 1;
	if (!run_multi_region_case(argv[1]))
		return 1;
	if (!run_dosum_case(argv[1]))
		return 1;
	if (!run_transparent_dosum_case(argv[1]))
		return 1;
	if (!run_dosum_prefix_split_case(argv[1]))
		return 1;
	if (!run_zero_parameter_case(argv[1]))
		return 1;
	if (!run_compile_decline_case(argv[1]))
		return 1;
	if (!run_multi_region_decline_case(
		argv[1],
		"multi-region-declined.noct")) {
		return 1;
	}
	if (!run_multi_region_decline_case(
		argv[1],
		"dosum-later-declined.noct")) {
		return 1;
	}
	if (!run_backend_decline_case(argv[1]))
		return 1;
	if (!run_budget_case(argv[1]))
		return 1;

	puts("PASS");

	return 0;
}

/* Read one owned NUL-terminated fixture from the requested directory. */
static char *
read_source(
	const char *directory,
	const char *name)
{
	char path[1024];
	char *source;
	FILE *file;
	long length;
	size_t read_size;
	int path_length;

	path_length = snprintf(path, sizeof(path), "%s/%s", directory, name);
	if (path_length < 0 || (size_t)path_length >= sizeof(path))
		return NULL;

	file = fopen(path, "rb");
	if (file == NULL)
		return NULL;
	if (fseek(file, 0, SEEK_END) != 0) {
		fclose(file);
		return NULL;
	}

	length = ftell(file);
	if (length < 0) {
		fclose(file);
		return NULL;
	}
	if (fseek(file, 0, SEEK_SET) != 0) {
		fclose(file);
		return NULL;
	}

	source = malloc((size_t)length + 1);
	if (source == NULL) {
		fclose(file);
		return NULL;
	}

	read_size = fread(source, 1, (size_t)length, file);
	if (read_size != (size_t)length) {
		free(source);
		fclose(file);
		return NULL;
	}

	source[length] = '\0';
	fclose(file);

	return source;
}

/* Parse, build, type, and return one fixture accelerator function. */
static bool
build_case(
	const char *directory,
	const char *name,
	struct hir_block **func_block)
{
	char *source;

	*func_block = NULL;
	source = read_source(directory, name);
	if (source == NULL) {
		fprintf(stderr, "failed to read %s\n", name);
		return false;
	}

	if (!ast_build(name, source)) {
		fprintf(
			stderr,
			"%s:%d: %s\n",
			name,
			ast_get_error_line(),
			ast_get_error_message());
		free(source);
		ast_cleanup();
		return false;
	}
	free(source);

	if (!hir_build()) {
		fprintf(
			stderr,
			"%s:%d: %s\n",
			name,
			hir_get_error_line(),
			hir_get_error_message());
		hir_cleanup();
		ast_cleanup();
		return false;
	}

	*func_block = find_accel_function();
	if (*func_block == NULL) {
		fprintf(stderr, "%s has no unique accelerator function\n", name);
		cleanup_case();
		return false;
	}

	if (!hir_opt_typed_func(*func_block)) {
		fprintf(stderr, "%s typed pass failed\n", name);
		cleanup_case();
		return false;
	}

	return true;
}

/* Release the current HIR before its source AST arena. */
static void
cleanup_case(
	void)
{
	hir_cleanup();
	ast_cleanup();
}

/* Find the single accelerator-hinted function in the current HIR table. */
static struct hir_block *
find_accel_function(
	void)
{
	struct hir_block *result;
	struct hir_block *func_block;
	uint32_t count;
	uint32_t i;

	result = NULL;
	count = hir_get_function_count();

	/* Find the one fixture function carrying the accelerator hint. */
	for (i = 0; i < count; i++) {
		func_block = hir_get_function(i);
		if (!func_block->val.func.is_accel)
			continue;
		if (result != NULL)
			return NULL;
		result = func_block;
	}

	return result;
}

/* Count current function locals using the same frame-list rule as LIR. */
static uint32_t
count_locals(
	const struct hir_block *func_block)
{
	const struct hir_local *local;
	uint32_t count;

	count = 0;
	local = func_block->val.func.local;

	/* Count every local frame entry. */
	while (local != NULL) {
		count++;
		local = local->next;
	}

	return count;
}

/* Find one mutable source local by its stable symbol. */
static struct hir_local *
find_local(
	struct hir_block *func_block,
	const char *symbol)
{
	struct hir_local *local;

	local = func_block->val.func.local;

	/* Search every function local in declaration order. */
	while (local != NULL) {
		if (strcmp(local->symbol, symbol) == 0)
			return local;
		local = local->next;
	}

	return NULL;
}

/* Create and attach one fake backend context to a zeroed test VM. */
static bool
create_context(
	struct rt_vm *vm,
	struct accel_test_backend_observer *observer,
	struct accel_context **context)
{
	struct accel_backend_ops ops;
	void *backend_state;

	memset(vm, 0, sizeof(*vm));
	*context = NULL;
	backend_state = NULL;
	if (!accel_test_backend_create(observer, &ops, &backend_state))
		return false;

	if (!accel_context_create(vm, &ops, backend_state, context)) {
		ops.destroy_backend_state(backend_state);
		return false;
	}

	accel_context_attach(*context);

	return true;
}

/* Detach and destroy one test context. */
static void
destroy_context(
	struct accel_context *context)
{
	if (context == NULL)
		return;

	accel_context_detach(context);
	accel_context_destroy(context);
}

/* Match one ordinary HIR symbol term expression. */
static bool
expression_is_symbol(
	const struct hir_expr *expression,
	const char *symbol)
{
	if (expression == NULL)
		return false;
	if (expression->type != HIR_EXPR_TERM)
		return false;
	if (expression->val.term.term == NULL)
		return false;
	if (expression->val.term.term->type != HIR_TERM_SYMBOL)
		return false;
	if (expression->val.term.term->val.symbol == NULL)
		return false;

	return strcmp(expression->val.term.term->val.symbol, symbol) == 0;
}

/* Read one ordinary HIR integer term expression. */
static bool
expression_is_integer(
	const struct hir_expr *expression,
	int *value)
{
	if (expression == NULL)
		return false;
	if (expression->type != HIR_EXPR_TERM)
		return false;
	if (expression->val.term.term == NULL)
		return false;
	if (expression->val.term.term->type != HIR_TERM_INT)
		return false;

	*value = expression->val.term.term->val.i;

	return true;
}

/* Match one generated array read at an exact constant argument slot. */
static bool
expression_is_subscript(
	const struct hir_expr *expression,
	const char *symbol,
	int index)
{
	int actual_index;

	if (expression == NULL)
		return false;
	if (expression->type != HIR_EXPR_SUBSCR)
		return false;
	if (!expression_is_symbol(expression->val.binary.expr[0], symbol))
		return false;
	if (!expression_is_integer(
		expression->val.binary.expr[1],
		&actual_index)) {
		return false;
	}

	return actual_index == index;
}

/* Match one compiler-generated private-package member call. */
static bool
expression_is_thiscall(
	const struct hir_expr *expression,
	const char *function_name)
{
	if (expression == NULL)
		return false;
	if (expression->type != HIR_EXPR_THISCALL)
		return false;
	if (!expression_is_symbol(expression->val.thiscall.obj, "__Accel"))
		return false;
	if (expression->val.thiscall.func == NULL)
		return false;

	return strcmp(expression->val.thiscall.func, function_name) == 0;
}

/* Check one block against the reachable top-level successor chain. */
static bool
block_is_reachable(
	const struct hir_block *func_block,
	const struct hir_block *target)
{
	const struct hir_block *block;
	uint32_t visited;

	visited = 0;
	block = func_block->val.func.inner;

	/* Search only blocks emitted on the fixture's top-level chain. */
	while (block != NULL && block != func_block->succ) {
		if (block == target)
			return true;
		if (visited++ > 1024)
			return false;
		if (block->stop)
			break;
		block = block->succ;
	}

	return false;
}

/* Validate the complete ordinary-HIR replacement for one two-kernel region. */
static bool
inspect_applied_shape(
	struct hir_block *func_block,
	struct hir_block *replacement,
	struct hir_block *after,
	uint32_t *program_id)
{
	struct hir_stmt *statement;
	struct hir_expr *expression;
	uint32_t i;
	int value;

	if (replacement == NULL || replacement->type != HIR_BLOCK_BASIC)
		return false;
	if (replacement->parent != func_block || replacement->succ != after)
		return false;
	if (replacement->stop || replacement->is_return_edge ||
	    replacement->is_break_edge || replacement->is_continue_edge)
		return false;
	if (replacement->addr != 0 || replacement->cont_addr != 0)
		return false;

	statement = replacement->val.basic.stmt_list;
	if (statement == NULL || statement->is_bare_return)
		return false;
	if (!expression_is_symbol(statement->lhs, "$accel.args.0"))
		return false;
	if (statement->rhs == NULL || statement->rhs->type != HIR_EXPR_ARRAY)
		return false;
	if (statement->rhs->val.array.elem_count != 3)
		return false;
	if (!expression_is_symbol(statement->rhs->val.array.elem[0], "source"))
		return false;
	if (!expression_is_symbol(statement->rhs->val.array.elem[1], "destination"))
		return false;
	if (!expression_is_symbol(statement->rhs->val.array.elem[2], "n"))
		return false;

	statement = statement->next;
	if (statement == NULL || statement->is_bare_return)
		return false;
	if (!expression_is_symbol(statement->lhs, "$accel.session.0"))
		return false;
	expression = statement->rhs;
	if (!expression_is_thiscall(expression, "begin"))
		return false;
	if (expression->val.thiscall.arg_count != 2)
		return false;
	if (!expression_is_integer(expression->val.thiscall.arg[0], &value))
		return false;
	if (value <= 0)
		return false;
	if (!expression_is_symbol(
		expression->val.thiscall.arg[1],
		"$accel.args.0")) {
		return false;
	}
	*program_id = (uint32_t)value;

	/* Verify deterministic zero-based dispatch indices. */
	for (i = 0; i < 2; i++) {
		statement = statement->next;
		if (statement == NULL || statement->lhs != NULL)
			return false;
		if (statement->is_bare_return)
			return false;
		expression = statement->rhs;
		if (!expression_is_thiscall(expression, "dispatch"))
			return false;
		if (expression->val.thiscall.arg_count != 2)
			return false;
		if (!expression_is_symbol(
			expression->val.thiscall.arg[0],
			"$accel.session.0")) {
			return false;
		}
		if (!expression_is_integer(expression->val.thiscall.arg[1], &value))
			return false;
		if (value != (int)i)
			return false;
	}

	statement = statement->next;
	if (statement == NULL || statement->lhs != NULL)
		return false;
	if (statement->is_bare_return)
		return false;
	expression = statement->rhs;
	if (!expression_is_thiscall(expression, "finish"))
		return false;
	if (expression->val.thiscall.arg_count != 2)
		return false;
	if (!expression_is_symbol(
		expression->val.thiscall.arg[0],
		"$accel.session.0")) {
		return false;
	}
	if (!expression_is_symbol(
		expression->val.thiscall.arg[1],
		"$accel.args.0")) {
		return false;
	}
	if (statement->next != NULL)
		return false;

	return true;
}

/* Validate one generated block in a source-ordered multi-region rewrite. */
static bool
inspect_multi_region_shape(
	struct hir_block *func_block,
	struct hir_block *replacement,
	struct hir_block *after,
	uint32_t region_index,
	uint32_t kernel_count,
	uint32_t *program_id)
{
	struct hir_stmt *statement;
	struct hir_expr *expression;
	char args_name[64];
	char session_name[64];
	uint32_t i;
	int args_length;
	int session_length;
	int value;

	args_length = snprintf(
		args_name,
		sizeof(args_name),
		"$accel.args.%lu",
		(unsigned long)region_index);
	session_length = snprintf(
		session_name,
		sizeof(session_name),
		"$accel.session.%lu",
		(unsigned long)region_index);
	if (args_length < 0 || (size_t)args_length >= sizeof(args_name))
		return false;
	if (session_length < 0 ||
	    (size_t)session_length >= sizeof(session_name)) {
		return false;
	}

	if (replacement == NULL || replacement->type != HIR_BLOCK_BASIC)
		return false;
	if (replacement->parent != func_block || replacement->succ != after)
		return false;

	/* Match the region-local argument array and session begin. */
	statement = replacement->val.basic.stmt_list;
	if (statement == NULL ||
	    !expression_is_symbol(statement->lhs, args_name)) {
		return false;
	}
	if (statement->rhs == NULL || statement->rhs->type != HIR_EXPR_ARRAY)
		return false;

	statement = statement->next;
	if (statement == NULL ||
	    !expression_is_symbol(statement->lhs, session_name)) {
		return false;
	}
	expression = statement->rhs;
	if (!expression_is_thiscall(expression, "begin"))
		return false;
	if (expression->val.thiscall.arg_count != 2)
		return false;
	if (!expression_is_integer(expression->val.thiscall.arg[0], &value))
		return false;
	if (value <= 0)
		return false;
	if (!expression_is_symbol(
		expression->val.thiscall.arg[1],
		args_name)) {
		return false;
	}
	*program_id = (uint32_t)value;

	/* Match zero-based dispatch indices within this region only. */
	for (i = 0; i < kernel_count; i++) {
		statement = statement->next;
		if (statement == NULL || statement->lhs != NULL)
			return false;
		expression = statement->rhs;
		if (!expression_is_thiscall(expression, "dispatch"))
			return false;
		if (expression->val.thiscall.arg_count != 2)
			return false;
		if (!expression_is_symbol(
			expression->val.thiscall.arg[0],
			session_name)) {
			return false;
		}
		if (!expression_is_integer(
			expression->val.thiscall.arg[1],
			&value)) {
			return false;
		}
		if (value != (int)i)
			return false;
	}

	/* Match the region-local finish and the end of the replacement. */
	statement = statement->next;
	if (statement == NULL || statement->lhs != NULL)
		return false;
	expression = statement->rhs;
	if (!expression_is_thiscall(expression, "finish"))
		return false;
	if (expression->val.thiscall.arg_count != 2)
		return false;
	if (!expression_is_symbol(
		expression->val.thiscall.arg[0],
		session_name)) {
		return false;
	}
	if (!expression_is_symbol(
		expression->val.thiscall.arg[1],
		args_name)) {
		return false;
	}
	if (statement->next != NULL)
		return false;

	return true;
}

/* Validate one reduction replacement and its explicit CPU publication. */
static bool
inspect_dosum_shape(
	struct hir_block *func_block,
	struct hir_block *replacement,
	struct hir_block *after,
	uint32_t *program_id)
{
	struct hir_stmt *statement;
	struct hir_expr *expression;
	uint32_t i;
	int value;

	if (replacement == NULL || replacement->type != HIR_BLOCK_BASIC)
		return false;
	if (replacement->parent != func_block || replacement->succ != after)
		return false;

	/* Match parameters followed by the scalar-result identity slot. */
	statement = replacement->val.basic.stmt_list;
	if (statement == NULL ||
	    !expression_is_symbol(statement->lhs, "$accel.args.0")) {
		return false;
	}
	expression = statement->rhs;
	if (expression == NULL || expression->type != HIR_EXPR_ARRAY)
		return false;
	if (expression->val.array.elem_count != 5)
		return false;
	if (!expression_is_symbol(expression->val.array.elem[0], "source"))
		return false;
	if (!expression_is_symbol(expression->val.array.elem[1], "destination"))
		return false;
	if (!expression_is_symbol(expression->val.array.elem[2], "n"))
		return false;
	if (!expression_is_symbol(expression->val.array.elem[3], "temporary"))
		return false;
	if (!expression_is_integer(expression->val.array.elem[4], &value))
		return false;
	if (value != 0)
		return false;

	/* Match the session begin and its private registry identifier. */
	statement = statement->next;
	if (statement == NULL ||
	    !expression_is_symbol(statement->lhs, "$accel.session.0")) {
		return false;
	}
	expression = statement->rhs;
	if (!expression_is_thiscall(expression, "begin"))
		return false;
	if (expression->val.thiscall.arg_count != 2)
		return false;
	if (!expression_is_integer(expression->val.thiscall.arg[0], &value))
		return false;
	if (value <= 0)
		return false;
	if (!expression_is_symbol(
		expression->val.thiscall.arg[1],
		"$accel.args.0")) {
		return false;
	}
	*program_id = (uint32_t)value;

	/* Match the producer and consumer dispatches in source order. */
	for (i = 0; i < 2; i++) {
		statement = statement->next;
		if (statement == NULL || statement->lhs != NULL)
			return false;
		expression = statement->rhs;
		if (!expression_is_thiscall(expression, "dispatch"))
			return false;
		if (expression->val.thiscall.arg_count != 2)
			return false;
		if (!expression_is_symbol(
			expression->val.thiscall.arg[0],
			"$accel.session.0")) {
			return false;
		}
		if (!expression_is_integer(
			expression->val.thiscall.arg[1],
			&value)) {
			return false;
		}
		if (value != (int)i)
			return false;
	}

	/* Match completion before the result becomes visible to CPU HIR. */
	statement = statement->next;
	if (statement == NULL || statement->lhs != NULL)
		return false;
	expression = statement->rhs;
	if (!expression_is_thiscall(expression, "finish"))
		return false;
	if (expression->val.thiscall.arg_count != 2)
		return false;
	if (!expression_is_symbol(
		expression->val.thiscall.arg[0],
		"$accel.session.0")) {
		return false;
	}
	if (!expression_is_symbol(
		expression->val.thiscall.arg[1],
		"$accel.args.0")) {
		return false;
	}

	/* Match the explicit local assignment consumed by later CPU code. */
	statement = statement->next;
	if (statement == NULL || !expression_is_symbol(statement->lhs, "sum"))
		return false;
	if (!expression_is_subscript(statement->rhs, "$accel.args.0", 4))
		return false;
	if (statement->next != NULL)
		return false;

	return true;
}

/* Validate a cloned zero initializer inside one three-kernel replacement. */
static bool
inspect_transparent_dosum_shape(
	struct hir_block *func_block,
	struct hir_block *replacement,
	struct hir_block *after,
	struct hir_local *local,
	const struct hir_stmt *source_initializer,
	uint32_t *program_id)
{
	struct hir_stmt *statement;
	struct hir_stmt *cloned_initializer;
	struct hir_expr *expression;
	uint32_t i;
	int value;

	if (replacement == NULL || replacement->type != HIR_BLOCK_BASIC)
		return false;
	if (replacement->parent != func_block || replacement->succ != after)
		return false;
	if (local == NULL || source_initializer == NULL)
		return false;

	/* Match the three parameters and dense scalar-result identity. */
	statement = replacement->val.basic.stmt_list;
	if (statement == NULL ||
	    !expression_is_symbol(statement->lhs, "$accel.args.0")) {
		return false;
	}
	expression = statement->rhs;
	if (expression == NULL || expression->type != HIR_EXPR_ARRAY)
		return false;
	if (expression->val.array.elem_count != 4)
		return false;
	if (!expression_is_symbol(expression->val.array.elem[0], "source"))
		return false;
	if (!expression_is_symbol(expression->val.array.elem[1], "destination"))
		return false;
	if (!expression_is_symbol(expression->val.array.elem[2], "n"))
		return false;
	if (!expression_is_integer(expression->val.array.elem[3], &value) ||
	    value != 0) {
		return false;
	}

	/* Match the session begin and retain its registry identifier. */
	statement = statement->next;
	if (statement == NULL ||
	    !expression_is_symbol(statement->lhs, "$accel.session.0")) {
		return false;
	}
	expression = statement->rhs;
	if (!expression_is_thiscall(expression, "begin"))
		return false;
	if (!expression_is_integer(expression->val.thiscall.arg[0], &value) ||
	    value <= 0) {
		return false;
	}
	*program_id = (uint32_t)value;

	/* Match the first independent-kernel dispatch. */
	statement = statement->next;
	if (statement == NULL || statement->lhs != NULL)
		return false;
	if (!expression_is_thiscall(statement->rhs, "dispatch"))
		return false;
	if (!expression_is_integer(
		statement->rhs->val.thiscall.arg[1],
		&value) || value != 0) {
		return false;
	}

	/* Require a detached declaration clone immediately before DOSUM. */
	cloned_initializer = statement->next;
	if (cloned_initializer == NULL ||
	    cloned_initializer == source_initializer ||
	    cloned_initializer->rhs == source_initializer->rhs) {
		return false;
	}
	if (cloned_initializer->line != source_initializer->line)
		return false;
	if (!expression_is_symbol(cloned_initializer->lhs, "sum"))
		return false;
	if (!expression_is_integer(cloned_initializer->rhs, &value) || value != 0)
		return false;
	if (local->declaration_stmt != cloned_initializer ||
	    local->initializer != cloned_initializer->rhs) {
		return false;
	}

	statement = cloned_initializer;

	/* Match the DOSUM and later result-consuming dispatches. */
	for (i = 1; i < 3; i++) {
		statement = statement->next;
		if (statement == NULL || statement->lhs != NULL)
			return false;
		if (!expression_is_thiscall(statement->rhs, "dispatch"))
			return false;
		if (!expression_is_integer(
			statement->rhs->val.thiscall.arg[1],
			&value) || value != (int)i) {
			return false;
		}
	}

	/* Match finish followed by explicit CPU publication. */
	statement = statement->next;
	if (statement == NULL || statement->lhs != NULL)
		return false;
	if (!expression_is_thiscall(statement->rhs, "finish"))
		return false;

	statement = statement->next;
	if (statement == NULL || !expression_is_symbol(statement->lhs, "sum"))
		return false;
	if (!expression_is_subscript(statement->rhs, "$accel.args.0", 3))
		return false;
	if (statement->next != NULL)
		return false;

	return true;
}

/* Validate the DOSUM replacement following a retained declaration prefix. */
static bool
inspect_split_dosum_shape(
	struct hir_block *func_block,
	struct hir_block *replacement,
	struct hir_block *after,
	uint32_t *program_id)
{
	struct hir_stmt *statement;
	struct hir_expr *expression;
	uint32_t i;
	int value;

	if (replacement == NULL || replacement->type != HIR_BLOCK_BASIC)
		return false;
	if (replacement->parent != func_block || replacement->succ != after)
		return false;

	/* Match region-one args with its dense scalar-result identity. */
	statement = replacement->val.basic.stmt_list;
	if (statement == NULL ||
	    !expression_is_symbol(statement->lhs, "$accel.args.1")) {
		return false;
	}
	expression = statement->rhs;
	if (expression == NULL || expression->type != HIR_EXPR_ARRAY)
		return false;
	if (expression->val.array.elem_count != 4)
		return false;
	if (!expression_is_integer(expression->val.array.elem[3], &value) ||
	    value != 0) {
		return false;
	}

	statement = statement->next;
	if (statement == NULL ||
	    !expression_is_symbol(statement->lhs, "$accel.session.1")) {
		return false;
	}
	if (!expression_is_thiscall(statement->rhs, "begin"))
		return false;
	if (!expression_is_integer(
		statement->rhs->val.thiscall.arg[0],
		&value) || value <= 0) {
		return false;
	}
	*program_id = (uint32_t)value;

	/* Match the region-local DOSUM producer and consumer dispatches. */
	for (i = 0; i < 2; i++) {
		statement = statement->next;
		if (statement == NULL || statement->lhs != NULL)
			return false;
		if (!expression_is_thiscall(statement->rhs, "dispatch"))
			return false;
		if (!expression_is_integer(
			statement->rhs->val.thiscall.arg[1],
			&value) || value != (int)i) {
			return false;
		}
	}

	statement = statement->next;
	if (statement == NULL || statement->lhs != NULL)
		return false;
	if (!expression_is_thiscall(statement->rhs, "finish"))
		return false;

	statement = statement->next;
	if (statement == NULL || !expression_is_symbol(statement->lhs, "sum"))
		return false;
	if (!expression_is_subscript(statement->rhs, "$accel.args.1", 3))
		return false;
	if (statement->next != NULL)
		return false;

	return true;
}

/* Force table growth, a cancelled hole, and a later non-reused ID. */
static bool
stress_registry(
	struct accel_context *context,
	const struct accel_prepared_program *first,
	uint32_t first_id)
{
	struct accel_prepared_program prepared[ACCEL_REWRITE_STRESS_COUNT];
	struct accel_prepared_program final_program[1];
	struct accel_registry_reservation *reservation;
	struct accel_registry_reservation *cancelled;
	struct accel_registry_reservation *final_reservation;
	struct accel_registry_commit_guard guard;
	const struct accel_prepared_program *borrowed;
	const struct accel_program *source_program;
	void *first_payload;
	uint32_t cancelled_id;
	uint32_t final_id;
	uint32_t prepared_count;
	uint32_t i;
	enum accel_compile_status status;

	memset(prepared, 0, sizeof(prepared));
	memset(final_program, 0, sizeof(final_program));
	memset(&guard, 0, sizeof(guard));
	reservation = NULL;
	cancelled = NULL;
	final_reservation = NULL;
	prepared_count = 0;
	first_payload = first->payload;
	source_program = accel_test_backend_get_program(first);
	if (source_program == NULL)
		return false;

	if (!accel_context_reserve_programs(
		context,
		ACCEL_REWRITE_STRESS_COUNT,
		&reservation)) {
		return false;
	}

	/* Prepare independent owned payloads for every no-fail publication slot. */
	for (i = 0; i < ACCEL_REWRITE_STRESS_COUNT; i++) {
		status = context->ops.prepare_program(
			context->backend_state,
			source_program,
			&prepared[i]);
		if (status != ACCEL_COMPILE_APPLIED) {
			cleanup_stress_registry(
				context,
				prepared,
				prepared_count,
				&final_program[0],
				reservation,
				cancelled,
				final_reservation,
				&guard);
			return false;
		}
		prepared_count++;
	}

	if (!accel_context_lock_commit(context, reservation, &guard)) {
		cleanup_stress_registry(
			context,
			prepared,
			prepared_count,
			&final_program[0],
			reservation,
			cancelled,
			final_reservation,
			&guard);
		return false;
	}
	accel_context_publish_programs_locked(&guard, prepared);
	reservation = NULL;
	prepared_count = 0;
	accel_context_unlock_commit(&guard);

	borrowed = accel_context_lookup_program(context, first_id);
	if (borrowed != first || borrowed->payload != first_payload)
		return false;

	if (!accel_context_reserve_programs(context, 1, &cancelled))
		return false;
	cancelled_id = accel_registry_reservation_get_id(cancelled, 0);
	accel_context_cancel_reservation(context, cancelled);
	cancelled = NULL;
	if (accel_context_lookup_program(context, cancelled_id) != NULL)
		return false;

	if (!accel_context_reserve_programs(context, 1, &final_reservation))
		return false;
	final_id = accel_registry_reservation_get_id(final_reservation, 0);
	if (final_id <= cancelled_id) {
		cleanup_stress_registry(
			context,
			prepared,
			prepared_count,
			&final_program[0],
			reservation,
			cancelled,
			final_reservation,
			&guard);
		return false;
	}

	status = context->ops.prepare_program(
		context->backend_state,
		source_program,
		&final_program[0]);
	if (status != ACCEL_COMPILE_APPLIED) {
		cleanup_stress_registry(
			context,
			prepared,
			prepared_count,
			&final_program[0],
			reservation,
			cancelled,
			final_reservation,
			&guard);
		return false;
	}

	if (!accel_context_lock_commit(context, final_reservation, &guard)) {
		cleanup_stress_registry(
			context,
			prepared,
			prepared_count,
			&final_program[0],
			reservation,
			cancelled,
			final_reservation,
			&guard);
		return false;
	}
	accel_context_publish_programs_locked(&guard, final_program);
	final_reservation = NULL;
	accel_context_unlock_commit(&guard);

	if (accel_context_lookup_program(context, final_id) == NULL)
		return false;

	return true;
}

/* Release every registry resource still owned by a failed stress step. */
static void
cleanup_stress_registry(
	struct accel_context *context,
	struct accel_prepared_program prepared[],
	uint32_t prepared_count,
	struct accel_prepared_program *final_program,
	struct accel_registry_reservation *reservation,
	struct accel_registry_reservation *cancelled,
	struct accel_registry_reservation *final_reservation,
	struct accel_registry_commit_guard *guard)
{
	uint32_t i;

	if (guard->locked)
		accel_context_unlock_commit(guard);
	if (reservation != NULL)
		accel_context_cancel_reservation(context, reservation);
	if (cancelled != NULL)
		accel_context_cancel_reservation(context, cancelled);
	if (final_reservation != NULL)
		accel_context_cancel_reservation(context, final_reservation);

	/* Release only fake programs not transferred into registry ownership. */
	for (i = 0; i < prepared_count; i++) {
		context->ops.destroy_prepared_program(
			context->backend_state,
			&prepared[i]);
	}
	context->ops.destroy_prepared_program(
		context->backend_state,
		final_program);
}

/* Apply, inspect, and stress one complete two-kernel transaction. */
static bool
run_applied_case(
	const char *directory)
{
	struct rt_vm vm;
	struct accel_test_backend_observer observer;
	struct accel_context *context;
	struct hir_block *func_block;
	struct hir_block *block;
	struct hir_block *previous;
	struct hir_block *first_loop;
	struct hir_block *last_loop;
	struct hir_block *after;
	struct hir_block *replacement;
	const struct accel_prepared_program *prepared;
	const struct accel_program *program;
	struct accel_rewrite_test_session session[2];
	uint32_t local_count;
	uint32_t program_id;
	uint32_t orphan_count;
	bool success;

	context = NULL;
	func_block = NULL;
	orphan_count = 0;
	memset(session, 0, sizeof(session));
	if (!build_case(directory, "two-kernel.noct", &func_block))
		return false;

	previous = NULL;
	first_loop = NULL;
	last_loop = NULL;
	block = func_block->val.func.inner;

	/* Locate the original maximal top-level loop group. */
	while (block != NULL && block != func_block->succ) {
		if (block->type == HIR_BLOCK_FOR) {
			if (first_loop == NULL)
				first_loop = block;
			last_loop = block;
		} else if (first_loop == NULL) {
			previous = block;
		}
		if (block->stop)
			break;
		block = block->succ;
	}

	if (first_loop == NULL || last_loop == NULL) {
		fprintf(stderr, "two-kernel fixture has no loop group\n");
		cleanup_case();
		return false;
	}
	after = last_loop->succ;
	local_count = count_locals(func_block);

	if (!create_context(&vm, &observer, &context)) {
		cleanup_case();
		return false;
	}

	success = vm.accel_optimize_func(
		func_block,
		vm.accel_optimize_userdata);
	if (!success) {
		fprintf(
			stderr,
			"applied callback failed: %s\n",
			hir_get_error_message());
		destroy_context(context);
		cleanup_case();
		return false;
	}

	if (func_block->val.func.is_accel) {
		fprintf(stderr, "applied hint was not consumed\n");
		destroy_context(context);
		cleanup_case();
		return false;
	}
	if (count_locals(func_block) != local_count + 2) {
		fprintf(stderr, "generated locals were not added exactly once\n");
		destroy_context(context);
		cleanup_case();
		return false;
	}

	if (previous == NULL)
		replacement = func_block->val.func.inner;
	else
		replacement = previous->succ;

	if (!inspect_applied_shape(
		func_block,
		replacement,
		after,
		&program_id)) {
		fprintf(stderr, "generated HIR shape is incorrect\n");
		destroy_context(context);
		cleanup_case();
		return false;
	}
	if (block_is_reachable(func_block, first_loop) ||
	    block_is_reachable(func_block, last_loop)) {
		fprintf(stderr, "original accelerator loop remains reachable\n");
		destroy_context(context);
		cleanup_case();
		return false;
	}

	prepared = accel_context_lookup_program(context, program_id);
	if (prepared == NULL) {
		fprintf(stderr, "published accelerator program is missing\n");
		destroy_context(context);
		cleanup_case();
		return false;
	}
	program = accel_test_backend_get_program(prepared);
	if (program == NULL || program->kernel_count != 2) {
		fprintf(stderr, "fake backend did not retain the two-kernel plan\n");
		destroy_context(context);
		cleanup_case();
		return false;
	}

	if (!stress_registry(context, prepared, program_id)) {
		fprintf(stderr, "registry stability test failed\n");
		destroy_context(context);
		cleanup_case();
		return false;
	}

	session[0].live.orphan_locked = orphan_test_session;
	session[0].live.destroy_orphan = destroy_test_orphan;
	session[0].orphan_count = &orphan_count;
	session[1].live.orphan_locked = orphan_test_session;
	session[1].live.destroy_orphan = destroy_test_orphan;
	session[1].orphan_count = &orphan_count;
	accel_context_state_lock(context);
	accel_context_link_session_locked(context, &session[0].live);
	accel_context_link_session_locked(context, &session[1].live);
	accel_context_unlink_session_locked(context, &session[0].live);
	session[0].live.orphan_locked(&session[0].live);
	accel_context_state_unlock(context);

	accel_context_detach(context);
	cleanup_case();
	if (strcmp(program->function_name, "transform") != 0) {
		fprintf(stderr, "prepared program retained HIR arena storage\n");
		accel_context_destroy(context);
		return false;
	}

	accel_context_destroy(context);
	if (orphan_count != 2) {
		fprintf(stderr, "live sessions were not orphaned exactly once\n");
		return false;
	}
	if (observer.prepare_count != observer.destroy_program_count) {
		fprintf(stderr, "prepared payload ownership was unbalanced\n");
		return false;
	}
	if (observer.destroy_state_count != 1) {
		fprintf(stderr, "fake backend state was not destroyed once\n");
		return false;
	}

	return true;
}

/* Remove one proven device constructor with no generated host argument. */
static bool
run_device_only_case(
	const char *directory)
{
	struct rt_vm vm;
	struct accel_test_backend_observer observer;
	struct accel_context *context;
	struct hir_block *func_block;
	struct hir_block *block;
	struct hir_block *previous;
	struct hir_block *declaration_previous;
	struct hir_block *declaration;
	struct hir_block *first_loop;
	struct hir_block *last_loop;
	struct hir_block *after;
	struct hir_block *replacement;
	struct hir_local *temporary;
	const struct hir_stmt *source_declaration;
	struct hir_stmt *statement;
	const struct accel_prepared_program *prepared;
	const struct accel_program *program;
	const struct accel_buffer_binding *device;
	char error[256];
	uint32_t local_count;
	uint32_t program_id;
	uint32_t device_count;
	uint32_t i;
	bool success;

	context = NULL;
	func_block = NULL;
	declaration_previous = NULL;
	declaration = NULL;
	first_loop = NULL;
	last_loop = NULL;
	device = NULL;
	if (!build_case(directory, "device-only.noct", &func_block))
		return false;

	temporary = find_local(func_block, "temporary");
	if (temporary == NULL || temporary->declaration_stmt == NULL ||
	    temporary->initializer == NULL) {
		fprintf(stderr, "device-only local metadata is missing\n");
		cleanup_case();
		return false;
	}
	source_declaration = temporary->declaration_stmt;

	/* Locate the removable declaration and its unique two-kernel region. */
	previous = NULL;
	block = func_block->val.func.inner;
	while (block != NULL && block != func_block->succ) {
		if (block->type == HIR_BLOCK_BASIC &&
		    block->val.basic.stmt_list == source_declaration) {
			declaration_previous = previous;
			declaration = block;
		} else if (block->type == HIR_BLOCK_FOR) {
			if (first_loop == NULL)
				first_loop = block;
			last_loop = block;
		}
		previous = block;
		if (block->stop)
			break;
		block = block->succ;
	}
	if (declaration == NULL || first_loop == NULL || last_loop == NULL ||
	    declaration->succ != first_loop || first_loop == last_loop ||
	    source_declaration->next != NULL) {
		fprintf(stderr, "device-only fixture has the wrong source shape\n");
		cleanup_case();
		return false;
	}
	after = last_loop->succ;
	local_count = count_locals(func_block);

	if (!create_context(&vm, &observer, &context)) {
		cleanup_case();
		return false;
	}

	/* Commit the complete compile, prepare, rewrite, and publish transaction. */
	success = vm.accel_optimize_func(
		func_block,
		vm.accel_optimize_userdata);
	if (!success) {
		fprintf(stderr, "device-only callback failed\n");
		destroy_context(context);
		cleanup_case();
		return false;
	}
	if (func_block->val.func.is_accel ||
	    count_locals(func_block) != local_count + 2) {
		fprintf(stderr, "device-only rewrite committed partial metadata\n");
		destroy_context(context);
		cleanup_case();
		return false;
	}

	/* The replacement starts where the constructor block was removed. */
	if (declaration_previous == NULL)
		replacement = func_block->val.func.inner;
	else
		replacement = declaration_previous->succ;
	if (!inspect_multi_region_shape(
		func_block,
		replacement,
		after,
		0,
		2,
		&program_id)) {
		fprintf(stderr, "device-only replacement shape is invalid\n");
		destroy_context(context);
		cleanup_case();
		return false;
	}

	/* Generated arguments retain only the exact source-parameter prefix. */
	statement = replacement->val.basic.stmt_list;
	if (statement->rhs == NULL || statement->rhs->type != HIR_EXPR_ARRAY ||
	    statement->rhs->val.array.elem_count != 3 ||
	    !expression_is_symbol(statement->rhs->val.array.elem[0], "source") ||
	    !expression_is_symbol(
		statement->rhs->val.array.elem[1],
		"destination") ||
	    !expression_is_symbol(statement->rhs->val.array.elem[2], "n")) {
		fprintf(stderr, "device-only rewrite generated a placeholder argument\n");
		destroy_context(context);
		cleanup_case();
		return false;
	}

	/* Both the declaration and source loops must leave the live HIR graph. */
	if (block_is_reachable(func_block, declaration) ||
	    block_is_reachable(func_block, first_loop) ||
	    block_is_reachable(func_block, last_loop)) {
		fprintf(stderr, "device-only source region remains reachable\n");
		destroy_context(context);
		cleanup_case();
		return false;
	}
	if (temporary->declaration_stmt != NULL || temporary->initializer != NULL) {
		fprintf(stderr, "removed device constructor retained live metadata\n");
		destroy_context(context);
		cleanup_case();
		return false;
	}

	/* Verify the published plan owns one sentinel, non-host device buffer. */
	prepared = accel_context_lookup_program(context, program_id);
	program = accel_test_backend_get_program(prepared);
	if (program == NULL || program->parameter_count != 3 ||
	    program->kernel_count != 2 ||
	    !accel_program_validate(program, error, sizeof(error))) {
		fprintf(stderr, "published device-only program is invalid\n");
		destroy_context(context);
		cleanup_case();
		return false;
	}
	device_count = 0;
	for (i = 0; i < program->buffer_count; i++) {
		if (program->buffer[i].origin != ACCEL_BUFFER_LOCAL_DEVICE)
			continue;
		device = &program->buffer[i];
		device_count++;
	}
	if (device_count != 1 || device == NULL ||
	    device->args_slot != ACCEL_ARGS_SLOT_NONE ||
	    device->extent_expression == ACCEL_PROGRAM_INDEX_NONE ||
	    device->host_visible || device->upload_required ||
	    device->download_required || device->materialization_required) {
		fprintf(stderr, "published device binding violates its invariants\n");
		destroy_context(context);
		cleanup_case();
		return false;
	}

	destroy_context(context);
	cleanup_case();

	if (observer.prepare_count != 1 || observer.destroy_program_count != 1 ||
	    observer.destroy_state_count != 1) {
		fprintf(stderr, "device-only backend ownership is unbalanced\n");
		return false;
	}

	return true;
}

/* Reject a stale local-list head without linking any staged local. */
static bool
run_local_link_guard_case(
	const char *directory)
{
	struct rt_vm vm;
	struct accel_test_backend_observer observer;
	struct accel_context *context;
	struct accel_function_plan *plan;
	struct accel_registry_reservation *reservation;
	struct accel_rewrite *rewrite;
	struct hir_block *func_block;
	struct hir_local *original_head;
	struct hir_local *external_head;
	enum accel_compile_status status;
	uint32_t region_count;
	uint32_t local_count;
	bool added;

	context = NULL;
	plan = NULL;
	reservation = NULL;
	rewrite = NULL;
	func_block = NULL;
	if (!build_case(directory, "two-kernel.noct", &func_block))
		return false;

	/* Build the immutable plan without changing the live local list. */
	original_head = func_block->val.func.local;
	local_count = count_locals(func_block);
	status = accel_compile_func(func_block, &plan);
	if (status != ACCEL_COMPILE_APPLIED || plan == NULL) {
		fprintf(stderr, "local-link guard fixture did not produce a plan\n");
		cleanup_case();
		accel_function_plan_destroy(plan);
		return false;
	}

	if (!create_context(&vm, &observer, &context)) {
		cleanup_case();
		accel_function_plan_destroy(plan);
		return false;
	}

	/* Reserve stable program identifiers needed by detached HIR staging. */
	region_count = accel_function_plan_get_region_count(plan);
	if (!accel_context_reserve_programs(
		context,
		region_count,
		&reservation)) {
		fprintf(stderr, "local-link guard reservation failed\n");
		destroy_context(context);
		cleanup_case();
		accel_function_plan_destroy(plan);
		return false;
	}

	/* Stage all replacement HIR and generated locals without publishing them. */
	status = accel_rewrite_stage(
		func_block,
		plan,
		reservation,
		&rewrite);
	if (status != ACCEL_COMPILE_APPLIED || rewrite == NULL ||
	    func_block->val.func.local != original_head ||
	    count_locals(func_block) != local_count) {
		fprintf(stderr, "detached local staging mutated live HIR\n");
		accel_rewrite_destroy(rewrite);
		accel_context_cancel_reservation(context, reservation);
		destroy_context(context);
		cleanup_case();
		accel_function_plan_destroy(plan);
		return false;
	}

	/* Simulate an intervening optimizer mutation of the local-list head. */
	added = hir_add_local(func_block, "$accel.external.guard");
	if (!added) {
		fprintf(stderr, "failed to add local-link guard sentinel\n");
		accel_rewrite_destroy(rewrite);
		accel_context_cancel_reservation(context, reservation);
		destroy_context(context);
		cleanup_case();
		accel_function_plan_destroy(plan);
		return false;
	}
	external_head = func_block->val.func.local;

	/* Reject the stale transaction without inserting a partial detached list. */
	if (accel_rewrite_add_locals(rewrite) ||
	    func_block->val.func.local != external_head ||
	    count_locals(func_block) != local_count + 1) {
		fprintf(stderr, "stale local-list guard linked staged locals\n");
		accel_rewrite_destroy(rewrite);
		accel_context_cancel_reservation(context, reservation);
		destroy_context(context);
		cleanup_case();
		accel_function_plan_destroy(plan);
		return false;
	}

	accel_rewrite_destroy(rewrite);
	accel_context_cancel_reservation(context, reservation);
	destroy_context(context);
	cleanup_case();
	accel_function_plan_destroy(plan);

	return true;
}

/* Apply two disjoint regions while retaining their CPU boundary. */
static bool
run_multi_region_case(
	const char *directory)
{
	struct rt_vm vm;
	struct accel_test_backend_observer observer;
	struct accel_context *context;
	struct hir_block *func_block;
	struct hir_block *block;
	struct hir_block *previous;
	struct hir_block *first_previous;
	struct hir_block *first_after;
	struct hir_block *second_previous;
	struct hir_block *second_after;
	struct hir_block *cpu_boundary;
	struct hir_block *original_loop[3];
	struct hir_block *replacement;
	const struct accel_prepared_program *prepared;
	const struct accel_program *program;
	uint32_t local_count;
	uint32_t loop_count;
	uint32_t first_program_id;
	uint32_t second_program_id;
	bool success;

	context = NULL;
	func_block = NULL;
	first_previous = NULL;
	first_after = NULL;
	second_previous = NULL;
	second_after = NULL;
	cpu_boundary = NULL;
	memset(original_loop, 0, sizeof(original_loop));
	if (!build_case(directory, "multi-region.noct", &func_block))
		return false;

	/* Locate both original regions and their retained predecessor links. */
	previous = NULL;
	loop_count = 0;
	block = func_block->val.func.inner;
	while (block != NULL && block != func_block->succ) {
		if (block->type == HIR_BLOCK_FOR) {
			if (loop_count >= 3) {
				destroy_context(context);
				cleanup_case();
				return false;
			}
			original_loop[loop_count] = block;
			if (loop_count == 0)
				first_previous = previous;
			if (loop_count == 2)
				second_previous = previous;
			loop_count++;
		} else if (block->type == HIR_BLOCK_BASIC &&
			   block->val.basic.stmt_list != NULL &&
			   loop_count == 2) {
			cpu_boundary = block;
		}
		previous = block;
		block = block->succ;
	}
	if (loop_count != 3 || second_previous == NULL || cpu_boundary == NULL) {
		fprintf(stderr, "multi-region fixture has the wrong loop shape\n");
		cleanup_case();
		return false;
	}

	first_after = original_loop[1]->succ;
	second_after = original_loop[2]->succ;
	local_count = count_locals(func_block);
	if (!create_context(&vm, &observer, &context)) {
		cleanup_case();
		return false;
	}

	/* Run the full budget, prepare, reserve, rewrite, and publish transaction. */
	success = vm.accel_optimize_func(
		func_block,
		vm.accel_optimize_userdata);
	if (!success) {
		fprintf(stderr, "multi-region callback failed\n");
		destroy_context(context);
		cleanup_case();
		return false;
	}
	if (func_block->val.func.is_accel ||
	    count_locals(func_block) != local_count + 4) {
		fprintf(stderr, "multi-region callback committed partial metadata\n");
		destroy_context(context);
		cleanup_case();
		return false;
	}

	/* Inspect the first replacement and its region-local dispatch indices. */
	if (first_previous == NULL)
		replacement = func_block->val.func.inner;
	else
		replacement = first_previous->succ;
	if (!inspect_multi_region_shape(
		func_block,
		replacement,
		first_after,
		0,
		2,
		&first_program_id)) {
		fprintf(stderr, "first multi-region replacement is invalid\n");
		destroy_context(context);
		cleanup_case();
		return false;
	}

	/* Inspect the retained CPU boundary's successor as the second region. */
	replacement = second_previous->succ;
	if (!inspect_multi_region_shape(
		func_block,
		replacement,
		second_after,
		1,
		1,
		&second_program_id)) {
		fprintf(stderr, "second multi-region replacement is invalid\n");
		destroy_context(context);
		cleanup_case();
		return false;
	}
	if (first_program_id == second_program_id) {
		fprintf(stderr, "multi-region programs share one registry id\n");
		destroy_context(context);
		cleanup_case();
		return false;
	}

	/* Require every source loop to be unreachable after the atomic swap. */
	if (block_is_reachable(func_block, original_loop[0]) ||
	    block_is_reachable(func_block, original_loop[1]) ||
	    block_is_reachable(func_block, original_loop[2])) {
		fprintf(stderr, "multi-region source loop remains reachable\n");
		destroy_context(context);
		cleanup_case();
		return false;
	}
	if (!block_is_reachable(func_block, cpu_boundary)) {
		fprintf(stderr, "multi-region CPU boundary was removed\n");
		destroy_context(context);
		cleanup_case();
		return false;
	}

	/* Verify both separately prepared programs retained their local indices. */
	prepared = accel_context_lookup_program(context, first_program_id);
	program = accel_test_backend_get_program(prepared);
	if (program == NULL || program->region_index != 0 ||
	    program->kernel_count != 2) {
		fprintf(stderr, "first multi-region program is missing\n");
		destroy_context(context);
		cleanup_case();
		return false;
	}
	prepared = accel_context_lookup_program(context, second_program_id);
	program = accel_test_backend_get_program(prepared);
	if (program == NULL || program->region_index != 1 ||
	    program->kernel_count != 1 ||
	    program->kernel[0].kernel_index != 0) {
		fprintf(stderr, "second multi-region program is missing\n");
		destroy_context(context);
		cleanup_case();
		return false;
	}
	if (observer.prepare_count != 2) {
		fprintf(stderr, "multi-region programs were not prepared together\n");
		destroy_context(context);
		cleanup_case();
		return false;
	}

	destroy_context(context);
	cleanup_case();

	return true;
}

/* Apply one DOSUM region and verify result publication remains transactional. */
static bool
run_dosum_case(
	const char *directory)
{
	struct rt_vm vm;
	struct accel_test_backend_observer observer;
	struct accel_context *context;
	struct hir_block *func_block;
	struct hir_block *block;
	struct hir_block *previous;
	struct hir_block *first_loop;
	struct hir_block *last_loop;
	struct hir_block *after;
	struct hir_block *replacement;
	struct lir_func *lir_func;
	const struct accel_prepared_program *prepared;
	const struct accel_program *program;
	char error[160];
	uint32_t local_count;
	uint32_t program_id;
	bool success;

	context = NULL;
	func_block = NULL;
	previous = NULL;
	first_loop = NULL;
	last_loop = NULL;
	lir_func = NULL;
	if (!build_case(directory, "dosum.noct", &func_block))
		return false;

	block = func_block->val.func.inner;

	/* Locate the retained initializer and consecutive reduction region. */
	while (block != NULL && block != func_block->succ) {
		if (block->type == HIR_BLOCK_FOR) {
			if (first_loop == NULL)
				first_loop = block;
			last_loop = block;
		} else if (first_loop == NULL) {
			previous = block;
		}
		block = block->succ;
	}
	if (first_loop == NULL || last_loop == NULL || previous == NULL) {
		fprintf(stderr, "dosum fixture has no retained initializer\n");
		cleanup_case();
		return false;
	}

	after = last_loop->succ;
	local_count = count_locals(func_block);
	if (!create_context(&vm, &observer, &context)) {
		cleanup_case();
		return false;
	}

	/* Commit the same prepare/reserve/rewrite transaction as production. */
	success = vm.accel_optimize_func(
		func_block,
		vm.accel_optimize_userdata);
	if (!success || func_block->val.func.is_accel) {
		fprintf(stderr, "dosum callback did not commit\n");
		destroy_context(context);
		cleanup_case();
		return false;
	}
	if (count_locals(func_block) != local_count + 2) {
		fprintf(stderr, "dosum generated an incorrect local count\n");
		destroy_context(context);
		cleanup_case();
		return false;
	}

	replacement = previous->succ;
	if (!inspect_dosum_shape(
		func_block,
		replacement,
		after,
		&program_id)) {
		fprintf(stderr, "dosum rewrite shape is incorrect\n");
		destroy_context(context);
		cleanup_case();
		return false;
	}
	if (block_is_reachable(func_block, first_loop) ||
	    block_is_reachable(func_block, last_loop) ||
	    !block_is_reachable(func_block, previous) ||
	    !block_is_reachable(func_block, after)) {
		fprintf(stderr, "dosum rewrite changed the wrong HIR interval\n");
		destroy_context(context);
		cleanup_case();
		return false;
	}

	prepared = accel_context_lookup_program(context, program_id);
	program = accel_test_backend_get_program(prepared);
	if (program == NULL || program->kernel_count != 2 ||
	    program->scalar_result_count != 1 ||
	    program->scalar_result[0].gpu_consumer_mask !=
		((uint32_t)1U << 1)) {
		fprintf(stderr, "dosum prepared program is incorrect\n");
		destroy_context(context);
		cleanup_case();
		return false;
	}
	if (!accel_program_validate(program, error, sizeof(error))) {
		fprintf(stderr, "dosum prepared program is invalid: %s\n", error);
		destroy_context(context);
		cleanup_case();
		return false;
	}

	/* Re-run the production post-rewrite typing and ordinary LIR lowering. */
	if (!hir_opt_typed_func(func_block)) {
		fprintf(stderr, "dosum rewritten HIR did not type\n");
		destroy_context(context);
		cleanup_case();
		return false;
	}
	lir_set_optimize_level(1);
	if (!lir_build(func_block, &lir_func)) {
		fprintf(stderr, "dosum rewritten HIR did not lower to LIR\n");
		destroy_context(context);
		cleanup_case();
		return false;
	}
	lir_cleanup(lir_func);

	destroy_context(context);
	cleanup_case();

	return true;
}

/* Apply one transparent initializer and compare budgeted with emitted LIR. */
static bool
run_transparent_dosum_case(
	const char *directory)
{
	struct rt_vm vm;
	struct accel_test_backend_observer observer;
	struct accel_context *context;
	struct accel_function_plan *plan;
	struct accel_registry_reservation *reservation;
	struct accel_rewrite *rewrite;
	struct hir_block *func_block;
	struct hir_block *inner;
	struct hir_block *block;
	struct hir_block *previous;
	struct hir_block *first_previous;
	struct hir_block *initializer_block;
	struct hir_block *original_loop[3];
	struct hir_block *after;
	struct hir_block *replacement;
	struct hir_local *local;
	const struct hir_stmt *source_initializer;
	const struct accel_prepared_program *prepared;
	const struct accel_program *program;
	struct lir_func *lir_func;
	enum accel_compile_status status;
	uint32_t serialized_tmpvar_size;
	uint32_t local_count;
	uint32_t loop_count;
	uint32_t program_id;
	bool success;

	context = NULL;
	plan = NULL;
	reservation = NULL;
	rewrite = NULL;
	func_block = NULL;
	first_previous = NULL;
	initializer_block = NULL;
	after = NULL;
	lir_func = NULL;
	serialized_tmpvar_size = 0;
	memset(original_loop, 0, sizeof(original_loop));
	if (!build_case(directory, "dosum-transparent.noct", &func_block))
		return false;

	local = find_local(func_block, "sum");
	if (local == NULL || local->declaration_stmt == NULL) {
		fprintf(stderr, "transparent DOSUM local metadata is missing\n");
		cleanup_case();
		return false;
	}
	source_initializer = local->declaration_stmt;

	previous = NULL;
	loop_count = 0;
	block = func_block->val.func.inner;

	/* Locate the three loops and the sole declaration between the first two. */
	while (block != NULL && block != func_block->succ) {
		if (block->type == HIR_BLOCK_FOR) {
			if (loop_count >= 3) {
				fprintf(stderr, "transparent DOSUM has too many loops\n");
				cleanup_case();
				return false;
			}
			if (loop_count == 0)
				first_previous = previous;
			original_loop[loop_count++] = block;
		} else if (block->type == HIR_BLOCK_BASIC &&
			   block->val.basic.stmt_list == source_initializer) {
			initializer_block = block;
		}

		previous = block;
		block = block->succ;
	}

	if (loop_count != 3 || initializer_block == NULL ||
	    initializer_block->val.basic.stmt_list->next != NULL) {
		fprintf(stderr, "transparent DOSUM source shape is invalid\n");
		cleanup_case();
		return false;
	}
	after = original_loop[2]->succ;
	local_count = count_locals(func_block);

	/* Preflight the exact virtual declaration and publication sequence. */
	status = accel_compile_func(func_block, &plan);
	if (status != ACCEL_COMPILE_APPLIED || plan == NULL) {
		fprintf(stderr, "transparent DOSUM did not produce a plan\n");
		cleanup_case();
		accel_function_plan_destroy(plan);
		return false;
	}
	status = accel_lir_budget_check(
		func_block,
		plan,
		&serialized_tmpvar_size);
	if (status != ACCEL_COMPILE_APPLIED || serialized_tmpvar_size == 0) {
		fprintf(stderr, "transparent DOSUM budget preflight failed\n");
		cleanup_case();
		accel_function_plan_destroy(plan);
		return false;
	}

	if (!create_context(&vm, &observer, &context)) {
		cleanup_case();
		accel_function_plan_destroy(plan);
		return false;
	}

	/* Cancel a fully staged clone and require all live metadata to roll back. */
	inner = func_block->val.func.inner;
	if (!accel_context_reserve_programs(context, 1, &reservation)) {
		fprintf(stderr, "transparent DOSUM staging reservation failed\n");
		destroy_context(context);
		cleanup_case();
		accel_function_plan_destroy(plan);
		return false;
	}
	status = accel_rewrite_stage(
		func_block,
		plan,
		reservation,
		&rewrite);
	if (status != ACCEL_COMPILE_APPLIED || rewrite == NULL) {
		fprintf(stderr, "transparent DOSUM staging failed\n");
		accel_context_cancel_reservation(context, reservation);
		destroy_context(context);
		cleanup_case();
		accel_function_plan_destroy(plan);
		return false;
	}
	if (func_block->val.func.inner != inner ||
	    local->declaration_stmt != source_initializer ||
	    local->initializer != source_initializer->rhs ||
	    count_locals(func_block) != local_count) {
		fprintf(stderr, "transparent DOSUM staging mutated live HIR\n");
		accel_rewrite_destroy(rewrite);
		accel_context_cancel_reservation(context, reservation);
		destroy_context(context);
		cleanup_case();
		accel_function_plan_destroy(plan);
		return false;
	}
	accel_rewrite_destroy(rewrite);
	rewrite = NULL;
	accel_context_cancel_reservation(context, reservation);
	reservation = NULL;
	accel_function_plan_destroy(plan);
	plan = NULL;

	/* Commit the production prepare, rewrite, and registry transaction. */
	success = vm.accel_optimize_func(
		func_block,
		vm.accel_optimize_userdata);
	if (!success || func_block->val.func.is_accel) {
		fprintf(stderr, "transparent DOSUM callback did not commit\n");
		destroy_context(context);
		cleanup_case();
		return false;
	}
	if (count_locals(func_block) != local_count + 2) {
		fprintf(stderr, "transparent DOSUM local count is invalid\n");
		destroy_context(context);
		cleanup_case();
		return false;
	}

	if (first_previous == NULL)
		replacement = func_block->val.func.inner;
	else
		replacement = first_previous->succ;
	if (!inspect_transparent_dosum_shape(
		func_block,
		replacement,
		after,
		local,
		source_initializer,
		&program_id)) {
		fprintf(stderr, "transparent DOSUM rewrite shape is invalid\n");
		destroy_context(context);
		cleanup_case();
		return false;
	}

	/* Require the declaration block and every source loop to be detached. */
	if (block_is_reachable(func_block, initializer_block) ||
	    block_is_reachable(func_block, original_loop[0]) ||
	    block_is_reachable(func_block, original_loop[1]) ||
	    block_is_reachable(func_block, original_loop[2])) {
		fprintf(stderr, "transparent DOSUM left source HIR reachable\n");
		destroy_context(context);
		cleanup_case();
		return false;
	}

	prepared = accel_context_lookup_program(context, program_id);
	program = accel_test_backend_get_program(prepared);
	if (program == NULL || program->kernel_count != 3 ||
	    program->scalar_result_count != 1 ||
	    program->scalar_result[0].producer_kernel != 1 ||
	    program->scalar_result[0].gpu_consumer_mask !=
		((uint32_t)1U << 2)) {
		fprintf(stderr, "transparent DOSUM prepared program is invalid\n");
		destroy_context(context);
		cleanup_case();
		return false;
	}

	/* Verify the exact virtual budget against ordinary post-rewrite LIR. */
	if (!hir_opt_typed_func(func_block)) {
		fprintf(stderr, "transparent DOSUM rewritten HIR did not type\n");
		destroy_context(context);
		cleanup_case();
		return false;
	}
	lir_set_optimize_level(1);
	if (!lir_build(func_block, &lir_func)) {
		fprintf(stderr, "transparent DOSUM rewritten HIR did not lower\n");
		destroy_context(context);
		cleanup_case();
		return false;
	}
	if (lir_func->tmpvar_size != serialized_tmpvar_size) {
		fprintf(
			stderr,
			"transparent DOSUM budget mismatch: %lu != %lu\n",
			(unsigned long)serialized_tmpvar_size,
			(unsigned long)lir_func->tmpvar_size);
		lir_cleanup(lir_func);
		destroy_context(context);
		cleanup_case();
		return false;
	}
	lir_cleanup(lir_func);

	destroy_context(context);
	cleanup_case();

	return true;
}

/* Retain a prefixed declaration block between two rewritten regions. */
static bool
run_dosum_prefix_split_case(
	const char *directory)
{
	struct rt_vm vm;
	struct accel_test_backend_observer observer;
	struct accel_context *context;
	struct hir_block *func_block;
	struct hir_block *block;
	struct hir_block *previous;
	struct hir_block *first_previous;
	struct hir_block *boundary;
	struct hir_block *original_loop[3];
	struct hir_block *replacement;
	struct hir_local *local;
	struct hir_stmt *statement;
	const struct hir_stmt *source_initializer;
	const struct accel_prepared_program *prepared;
	const struct accel_program *program;
	uint32_t local_count;
	uint32_t loop_count;
	uint32_t first_program_id;
	uint32_t second_program_id;
	bool success;

	context = NULL;
	func_block = NULL;
	first_previous = NULL;
	boundary = NULL;
	memset(original_loop, 0, sizeof(original_loop));
	if (!build_case(directory, "dosum-prefix-split.noct", &func_block))
		return false;

	local = find_local(func_block, "sum");
	if (local == NULL || local->declaration_stmt == NULL) {
		fprintf(stderr, "prefixed DOSUM local metadata is missing\n");
		cleanup_case();
		return false;
	}
	source_initializer = local->declaration_stmt;

	previous = NULL;
	loop_count = 0;
	block = func_block->val.func.inner;

	/* Locate the retained two-statement boundary and three source loops. */
	while (block != NULL && block != func_block->succ) {
		if (block->type == HIR_BLOCK_FOR) {
			if (loop_count >= 3) {
				fprintf(stderr, "prefixed DOSUM has too many loops\n");
				cleanup_case();
				return false;
			}
			if (loop_count == 0)
				first_previous = previous;
			original_loop[loop_count++] = block;
		} else if (block->type == HIR_BLOCK_BASIC) {
			statement = block->val.basic.stmt_list;

			/* Find the declaration inside its nontransparent prefix. */
			while (statement != NULL) {
				if (statement == source_initializer)
					boundary = block;
				statement = statement->next;
			}
		}

		previous = block;
		block = block->succ;
	}

	if (loop_count != 3 || boundary == NULL ||
	    boundary->val.basic.stmt_list == source_initializer ||
	    source_initializer->next != NULL) {
		fprintf(stderr, "prefixed DOSUM source shape is invalid\n");
		cleanup_case();
		return false;
	}
	local_count = count_locals(func_block);

	if (!create_context(&vm, &observer, &context)) {
		cleanup_case();
		return false;
	}

	/* Commit both programs and both disjoint HIR replacements together. */
	success = vm.accel_optimize_func(
		func_block,
		vm.accel_optimize_userdata);
	if (!success || func_block->val.func.is_accel) {
		fprintf(stderr, "prefixed DOSUM callback did not commit\n");
		destroy_context(context);
		cleanup_case();
		return false;
	}
	if (count_locals(func_block) != local_count + 4) {
		fprintf(stderr, "prefixed DOSUM local count is invalid\n");
		destroy_context(context);
		cleanup_case();
		return false;
	}

	if (first_previous == NULL)
		replacement = func_block->val.func.inner;
	else
		replacement = first_previous->succ;
	if (!inspect_multi_region_shape(
		func_block,
		replacement,
		boundary,
		0,
		1,
		&first_program_id)) {
		fprintf(stderr, "prefixed DOSUM first rewrite is invalid\n");
		destroy_context(context);
		cleanup_case();
		return false;
	}

	replacement = boundary->succ;
	if (!inspect_split_dosum_shape(
		func_block,
		replacement,
		original_loop[2]->succ,
		&second_program_id)) {
		fprintf(stderr, "prefixed DOSUM second rewrite is invalid\n");
		destroy_context(context);
		cleanup_case();
		return false;
	}
	if (first_program_id == second_program_id) {
		fprintf(stderr, "prefixed DOSUM programs share one identifier\n");
		destroy_context(context);
		cleanup_case();
		return false;
	}

	/* Preserve the original declaration metadata outside both regions. */
	if (!block_is_reachable(func_block, boundary) ||
	    local->declaration_stmt != source_initializer ||
	    local->initializer != source_initializer->rhs) {
		fprintf(stderr, "prefixed DOSUM boundary metadata changed\n");
		destroy_context(context);
		cleanup_case();
		return false;
	}
	if (block_is_reachable(func_block, original_loop[0]) ||
	    block_is_reachable(func_block, original_loop[1]) ||
	    block_is_reachable(func_block, original_loop[2])) {
		fprintf(stderr, "prefixed DOSUM source loop remains reachable\n");
		destroy_context(context);
		cleanup_case();
		return false;
	}

	prepared = accel_context_lookup_program(context, second_program_id);
	program = accel_test_backend_get_program(prepared);
	if (program == NULL || program->region_index != 1 ||
	    program->kernel_count != 2 ||
	    program->scalar_result_count != 1 ||
	    program->scalar_result[0].producer_kernel != 0 ||
	    program->scalar_result[0].gpu_consumer_mask !=
		((uint32_t)1U << 1)) {
		fprintf(stderr, "prefixed DOSUM prepared program is invalid\n");
		destroy_context(context);
		cleanup_case();
		return false;
	}

	destroy_context(context);
	cleanup_case();

	return true;
}

/* Record one state-locked session orphan callback. */
static void *
orphan_test_session(
	struct accel_live_session *session)
{
	struct accel_rewrite_test_session *test_session;

	test_session = (struct accel_rewrite_test_session *)session;
	if (test_session->orphan_count == NULL)
		return NULL;

	(*test_session->orphan_count)++;

	return NULL;
}

/* Accept the intentionally empty orphan payload used by this focused test. */
static void
destroy_test_orphan(
	void *payload)
{
	UNUSED_PARAMETER(payload);
}

/* Apply a zero-parameter region and require the empty-array term shape. */
static bool
run_zero_parameter_case(
	const char *directory)
{
	struct rt_vm vm;
	struct accel_test_backend_observer observer;
	struct accel_zero_parameter_test_state state;
	struct hir_block *func_block;
	struct hir_block *block;
	struct hir_stmt *statement;
	struct accel_kernel_plan kernel;
	const struct accel_program *program;
	enum accel_compile_status status;
	uint32_t serialized_tmpvar_size;
	uint32_t ignored;

	memset(&state, 0, sizeof(state));
	func_block = NULL;
	serialized_tmpvar_size = 0;
	memset(&kernel, 0, sizeof(kernel));
	if (!build_case(directory, "zero-parameter.noct", &func_block))
		return false;
	if (!create_context(&vm, &observer, &state.context)) {
		cleanup_case();
		return false;
	}

	block = func_block->val.func.inner;

	/* Find the fixture's one ranged loop for a minimal owned test plan. */
	while (block != NULL && block != func_block->succ) {
		if (block->type == HIR_BLOCK_FOR)
			break;
		block = block->succ;
	}
	if (block == NULL || block == func_block->succ) {
		cleanup_zero_parameter_failure(&state);
		return false;
	}

	state.owned_program = accel_program_create(
		"zero-parameter.noct",
		"transform",
		block->line,
		0,
		0,
		0,
		block->id,
		block->id);
	if (state.owned_program == NULL) {
		cleanup_zero_parameter_failure(&state);
		return false;
	}

	state.ir = accel_ir_kernel_create(
		"kernel0",
		block->line,
		block->id,
		0,
		0);
	if (state.ir == NULL) {
		cleanup_zero_parameter_failure(&state);
		return false;
	}

	kernel.kernel_index = 0;
	kernel.source_line = block->line;
	kernel.loop_block_id = block->id;
	kernel.ir = state.ir;
	if (!accel_program_add_kernel(
		state.owned_program,
		&kernel,
		&ignored)) {
		cleanup_zero_parameter_failure(&state);
		return false;
	}
	state.ir = NULL;

	state.plan = accel_function_plan_create();
	if (state.plan == NULL) {
		cleanup_zero_parameter_failure(&state);
		return false;
	}
	if (!accel_function_plan_add_region(
		state.plan,
		state.owned_program)) {
		cleanup_zero_parameter_failure(&state);
		return false;
	}
	state.owned_program = NULL;

	program = accel_function_plan_get_region(state.plan, 0);
	if (program == NULL) {
		cleanup_zero_parameter_failure(&state);
		return false;
	}

	status = state.context->ops.prepare_program(
		state.context->backend_state,
		program,
		&state.prepared[0]);
	if (status != ACCEL_COMPILE_APPLIED) {
		cleanup_zero_parameter_failure(&state);
		return false;
	}
	if (!accel_context_reserve_programs(
		state.context,
		1,
		&state.reservation)) {
		cleanup_zero_parameter_failure(&state);
		return false;
	}

	status = accel_lir_budget_check(
		func_block,
		state.plan,
		&serialized_tmpvar_size);
	if (status != ACCEL_COMPILE_APPLIED) {
		cleanup_zero_parameter_failure(&state);
		return false;
	}
	status = accel_rewrite_stage(
		func_block,
		state.plan,
		state.reservation,
		&state.rewrite);
	if (status != ACCEL_COMPILE_APPLIED) {
		cleanup_zero_parameter_failure(&state);
		return false;
	}
	if (!accel_context_lock_commit(
		state.context,
		state.reservation,
		&state.guard)) {
		cleanup_zero_parameter_failure(&state);
		return false;
	}
	if (!accel_rewrite_add_locals(state.rewrite)) {
		cleanup_zero_parameter_failure(&state);
		return false;
	}
	accel_context_publish_programs_locked(&state.guard, state.prepared);
	state.reservation = NULL;
	accel_rewrite_commit(state.rewrite);
	accel_context_unlock_commit(&state.guard);

	block = func_block->val.func.inner;

	/* Skip the retained CPU prefix before the replacement block. */
	while (block != NULL && block->type != HIR_BLOCK_BASIC)
		block = block->succ;

	/* Find the generated args assignment among retained basic blocks. */
	while (block != NULL && block->val.basic.stmt_list != NULL) {
		if (expression_is_symbol(
			block->val.basic.stmt_list->lhs,
			"$accel.args.0")) {
			break;
		}
		block = block->succ;
	}
	if (block == NULL) {
		cleanup_zero_parameter_failure(&state);
		return false;
	}

	statement = block->val.basic.stmt_list;
	if (statement == NULL || statement->rhs == NULL ||
	    statement->rhs->type != HIR_EXPR_TERM ||
	    statement->rhs->val.term.term == NULL ||
	    statement->rhs->val.term.term->type != HIR_TERM_EMPTY_ARRAY) {
		fprintf(stderr, "zero-parameter rewrite did not use empty-array term\n");
		cleanup_zero_parameter_failure(&state);
		return false;
	}

	accel_rewrite_destroy(state.rewrite);
	accel_function_plan_destroy(state.plan);
	destroy_context(state.context);
	cleanup_case();

	return true;
}

/* Release one incomplete zero-parameter transaction in reverse order. */
static void
cleanup_zero_parameter_failure(
	struct accel_zero_parameter_test_state *state)
{
	if (state->guard.locked)
		accel_context_unlock_commit(&state->guard);
	if (state->reservation != NULL) {
		accel_context_cancel_reservation(
			state->context,
			state->reservation);
	}
	if (state->context != NULL) {
		state->context->ops.destroy_prepared_program(
			state->context->backend_state,
			&state->prepared[0]);
	}
	accel_ir_kernel_destroy(state->ir);
	accel_program_destroy(state->owned_program);
	accel_rewrite_destroy(state->rewrite);
	accel_function_plan_destroy(state->plan);
	destroy_context(state->context);
	cleanup_case();
	fprintf(stderr, "zero-parameter rewrite transaction failed\n");
}

/* Preserve HIR exactly when target-neutral eligibility declines. */
static bool
run_compile_decline_case(
	const char *directory)
{
	struct rt_vm vm;
	struct accel_test_backend_observer observer;
	struct accel_context *context;
	struct hir_block *func_block;
	struct hir_block *inner;
	struct hir_local *local;
	bool success;

	context = NULL;
	func_block = NULL;
	if (!build_case(directory, "declined.noct", &func_block))
		return false;
	if (!create_context(&vm, &observer, &context)) {
		cleanup_case();
		return false;
	}

	inner = func_block->val.func.inner;
	local = func_block->val.func.local;
	success = vm.accel_optimize_func(
		func_block,
		vm.accel_optimize_userdata);
	if (!success || !func_block->val.func.is_accel ||
	    func_block->val.func.inner != inner ||
	    func_block->val.func.local != local) {
		fprintf(stderr, "compile decline mutated live HIR\n");
		destroy_context(context);
		cleanup_case();
		return false;
	}
	if (observer.prepare_count != 0) {
		fprintf(stderr, "backend ran after target-neutral decline\n");
		destroy_context(context);
		cleanup_case();
		return false;
	}

	destroy_context(context);
	cleanup_case();

	return true;
}

/* Roll back an already built first plan when a later region declines. */
static bool
run_multi_region_decline_case(
	const char *directory,
	const char *name)
{
	struct rt_vm vm;
	struct accel_test_backend_observer observer;
	struct accel_context *context;
	struct hir_block *func_block;
	struct hir_block *inner;
	struct hir_local *local;
	bool success;

	context = NULL;
	func_block = NULL;
	if (!build_case(
		directory,
		name,
		&func_block)) {
		return false;
	}
	if (!create_context(&vm, &observer, &context)) {
		cleanup_case();
		return false;
	}

	/* Preserve both live-HIR roots across the target-neutral transaction. */
	inner = func_block->val.func.inner;
	local = func_block->val.func.local;
	success = vm.accel_optimize_func(
		func_block,
		vm.accel_optimize_userdata);
	if (!success || !func_block->val.func.is_accel ||
	    func_block->val.func.inner != inner ||
	    func_block->val.func.local != local) {
		fprintf(stderr, "later region decline mutated live HIR\n");
		destroy_context(context);
		cleanup_case();
		return false;
	}
	if (observer.prepare_count != 0 || observer.destroy_program_count != 0) {
		fprintf(stderr, "partial multi-region plan reached the backend\n");
		destroy_context(context);
		cleanup_case();
		return false;
	}

	destroy_context(context);
	cleanup_case();

	return true;
}

/* Preserve HIR and clean ownership when the selected backend declines. */
static bool
run_backend_decline_case(
	const char *directory)
{
	struct rt_vm vm;
	struct accel_test_backend_observer observer;
	struct accel_context *context;
	struct hir_block *func_block;
	struct hir_block *inner;
	struct hir_local *local;
	bool success;

	context = NULL;
	func_block = NULL;
	if (!build_case(directory, "two-kernel.noct", &func_block))
		return false;
	if (!create_context(&vm, &observer, &context)) {
		cleanup_case();
		return false;
	}

	observer.prepare_status = ACCEL_COMPILE_DECLINED;
	inner = func_block->val.func.inner;
	local = func_block->val.func.local;
	success = vm.accel_optimize_func(
		func_block,
		vm.accel_optimize_userdata);
	if (!success || !func_block->val.func.is_accel ||
	    func_block->val.func.inner != inner ||
	    func_block->val.func.local != local) {
		fprintf(stderr, "backend decline mutated live HIR\n");
		destroy_context(context);
		cleanup_case();
		return false;
	}
	if (observer.prepare_count != 1 || observer.destroy_program_count != 0) {
		fprintf(stderr, "backend decline ownership is incorrect\n");
		destroy_context(context);
		cleanup_case();
		return false;
	}

	destroy_context(context);
	cleanup_case();

	return true;
}

/* Decline a rewrite at the slot boundary while the original CPU HIR builds. */
static bool
run_budget_case(
	const char *directory)
{
	struct accel_function_plan *plan;
	struct hir_block *func_block;
	struct lir_func *lir_func;
	char name[64];
	enum accel_compile_status status;
	uint32_t serialized_tmpvar_size;
	uint32_t local_count;
	int length;

	plan = NULL;
	func_block = NULL;
	lir_func = NULL;
	if (!build_case(directory, "budget.noct", &func_block))
		return false;

	status = accel_compile_func(func_block, &plan);
	if (status != ACCEL_COMPILE_APPLIED || plan == NULL) {
		fprintf(stderr, "budget fixture did not produce a plan\n");
		cleanup_case();
		accel_function_plan_destroy(plan);
		return false;
	}

	local_count = count_locals(func_block);

	/* Raise only the final frame base, leaving the empty CPU loop simple. */
	while (local_count < 121) {
		length = snprintf(
			name,
			sizeof(name),
			"$budget.%lu",
			(unsigned long)local_count);
		if (length < 0 || (size_t)length >= sizeof(name)) {
			accel_function_plan_destroy(plan);
			cleanup_case();
			return false;
		}

		if (!hir_add_local(func_block, name)) {
			accel_function_plan_destroy(plan);
			cleanup_case();
			return false;
		}
		local_count++;
	}

	serialized_tmpvar_size = 0;
	status = accel_lir_budget_check(
		func_block,
		plan,
		&serialized_tmpvar_size);
	if (status != ACCEL_COMPILE_DECLINED || serialized_tmpvar_size != 0) {
		fprintf(stderr, "slot-boundary rewrite did not decline\n");
		accel_function_plan_destroy(plan);
		cleanup_case();
		return false;
	}
	if (!func_block->val.func.is_accel) {
		fprintf(stderr, "budget preflight consumed the hint\n");
		accel_function_plan_destroy(plan);
		cleanup_case();
		return false;
	}

	lir_set_optimize_level(1);
	if (!lir_build(func_block, &lir_func)) {
		fprintf(stderr, "CPU fallback exceeded the LIR slot budget\n");
		accel_function_plan_destroy(plan);
		cleanup_case();
		return false;
	}
	if (lir_func->tmpvar_size > LIR_TMPVAR_MAX) {
		fprintf(stderr, "CPU fallback serialized too many slots\n");
		lir_cleanup(lir_func);
		accel_function_plan_destroy(plan);
		cleanup_case();
		return false;
	}

	lir_cleanup(lir_func);
	accel_function_plan_destroy(plan);
	cleanup_case();

	return true;
}
