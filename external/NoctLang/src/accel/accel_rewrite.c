/* -*- coding: utf-8; tab-width: 8; indent-tabs-mode: t; -*- */

/*
 * Noct Programming Language
 * Copyright (c) 2025, 2026, Awe Morris
 */

/*
 * Detached accelerator HIR rewrite construction.
 */

#include "accel_rewrite.h"
#include "hir.h"

#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ACCEL_REWRITE_MAX_BLOCKS	4096
#define ACCEL_REWRITE_NAME_SIZE	64

struct accel_rewrite_initializer {
	struct hir_local *local;
	const struct hir_stmt *source_statement;
	struct hir_stmt *replacement_statement;
	uint32_t before_kernel;
	bool remove;
};

struct accel_rewrite_region {
	struct hir_block **link;
	struct hir_block *first;
	struct hir_block *last;
	struct hir_block *replacement;
	uint32_t initializer_count;
	struct accel_rewrite_initializer initializer[ACCEL_MAX_KERNELS];
	char args_name[ACCEL_REWRITE_NAME_SIZE];
	char session_name[ACCEL_REWRITE_NAME_SIZE];
};

struct accel_rewrite {
	struct hir_block *func_block;
	struct hir_local *original_local_head;
	struct hir_local *detached_local_head;
	struct hir_local *detached_local_tail;
	uint32_t region_count;
	struct accel_rewrite_region *region;
	bool locals_added;
	bool committed;
};

static enum accel_compile_status accel_rewrite_error(struct hir_block *func_block, const char *message);
static bool accel_rewrite_find_region(struct hir_block *func_block, const struct accel_program *program, struct hir_block *search_start, struct hir_block *search_prev, struct accel_rewrite_region *region, struct hir_block **next_search, struct hir_block **next_prev);
static bool accel_rewrite_validate_kernels(struct hir_block *func_block, const struct accel_program *program, struct accel_rewrite_region *region, struct hir_block *first, struct hir_block *last);
static bool accel_rewrite_validate_initializer(struct hir_block *func_block, const struct accel_program *program, struct accel_rewrite_region *region, struct hir_block *block, uint32_t before_kernel);
static bool accel_rewrite_validate_device_initializer(struct hir_block *func_block, const struct accel_program *program, struct accel_rewrite_region *region, struct hir_block *block);
static bool accel_rewrite_stage_locals(struct accel_rewrite *rewrite);
static bool accel_rewrite_stage_local(struct accel_rewrite *rewrite, const char *symbol, int index);
static bool accel_rewrite_local_exists(const struct hir_block *func_block, const char *name);
static struct hir_local *accel_rewrite_find_local(struct hir_block *func_block, const char *name);
static const char *accel_rewrite_symbol(const struct hir_expr *expression);
static bool accel_rewrite_zero(const struct hir_expr *expression);
static bool accel_rewrite_build_region(struct hir_block *func_block, const struct accel_program *program, uint32_t program_id, struct accel_rewrite_region *region);
static bool accel_rewrite_append_initializers(struct hir_block *block, struct hir_stmt **tail, struct accel_rewrite_region *region, uint32_t before_kernel);
static struct hir_block *accel_rewrite_new_block(struct hir_block *func_block, struct hir_block *last, int line);
static struct hir_stmt *accel_rewrite_new_statement(int line, struct hir_expr *lhs, struct hir_expr *rhs);
static struct hir_expr *accel_rewrite_new_symbol(const char *symbol);
static struct hir_expr *accel_rewrite_new_integer(int value);
static struct hir_expr *accel_rewrite_clone_zero(const struct hir_expr *source);
static struct hir_expr *accel_rewrite_new_subscript(const char *symbol, uint32_t index);
static struct hir_expr *accel_rewrite_new_args_array(const struct hir_block *func_block, const struct accel_program *program);
static struct hir_expr *accel_rewrite_new_thiscall(const char *function_name, uint32_t arg_count, struct hir_expr *const argument[]);
static bool accel_rewrite_append_statement(struct hir_block *block, struct hir_stmt **tail, struct hir_stmt *statement);

/*
 * Stages detached ordinary HIR for every planned accelerator region.
 */
enum accel_compile_status
accel_rewrite_stage(
	struct hir_block *func_block,
	const struct accel_function_plan *plan,
	const struct accel_registry_reservation *reservation,
	struct accel_rewrite **result)
{
	struct accel_rewrite *rewrite;
	const struct accel_program *program;
	struct hir_block *search;
	struct hir_block *previous;
	struct hir_block *next_search;
	struct hir_block *next_previous;
	uint32_t program_id;
	uint32_t expected_local_count;
	uint32_t i;
	uint32_t j;

	if (result == NULL)
		return ACCEL_COMPILE_ERROR;

	*result = NULL;

	if (func_block == NULL)
		return accel_rewrite_error(func_block, N_TR("Invalid accelerator rewrite function."));
	if (func_block->type != HIR_BLOCK_FUNC)
		return accel_rewrite_error(func_block, N_TR("Invalid accelerator rewrite function."));
	if (plan == NULL)
		return accel_rewrite_error(func_block, N_TR("Missing accelerator function plan."));
	if (reservation == NULL)
		return accel_rewrite_error(func_block, N_TR("Missing accelerator registry reservation."));
	if (plan->region_count == 0)
		return accel_rewrite_error(func_block, N_TR("Empty accelerator function plan."));
	if (plan->region_count > UINT32_MAX / 2)
		return accel_rewrite_error(func_block, N_TR("Invalid accelerator local count."));

	expected_local_count = plan->region_count * 2;
	if (plan->generated_local_count != expected_local_count) {
		return accel_rewrite_error(
			func_block,
			N_TR("Invalid accelerator local count."));
	}

	rewrite = noct_calloc(1, sizeof(*rewrite));
	if (rewrite == NULL) {
		hir_out_of_memory();
		return ACCEL_COMPILE_ERROR;
	}

	rewrite->region = noct_calloc(
		plan->region_count,
		sizeof(*rewrite->region));
	if (rewrite->region == NULL) {
		noct_free(rewrite);
		hir_out_of_memory();
		return ACCEL_COMPILE_ERROR;
	}

	rewrite->func_block = func_block;
	rewrite->original_local_head = func_block->val.func.local;
	rewrite->region_count = plan->region_count;
	search = func_block->val.func.inner;
	previous = NULL;

	/* Resolve every planned region against the unchanged top-level chain. */
	for (i = 0; i < plan->region_count; i++) {
		program = accel_function_plan_get_region(plan, i);
		if (program == NULL) {
			accel_rewrite_destroy(rewrite);
			return accel_rewrite_error(
				func_block,
				N_TR("Invalid accelerator region plan."));
		}

		if (!accel_rewrite_find_region(
			func_block,
			program,
			search,
			previous,
			&rewrite->region[i],
			&next_search,
			&next_previous)) {
			accel_rewrite_destroy(rewrite);
			return accel_rewrite_error(
				func_block,
				N_TR("Accelerator plan does not match live HIR."));
		}

		program_id = accel_registry_reservation_get_id(reservation, i);
		if (program_id == 0 || program_id > (uint32_t)INT_MAX) {
			accel_rewrite_destroy(rewrite);
			return accel_rewrite_error(
				func_block,
				N_TR("Invalid accelerator program identifier."));
		}

		if (!accel_rewrite_build_region(
			func_block,
			program,
			program_id,
			&rewrite->region[i])) {
			accel_rewrite_destroy(rewrite);
			return ACCEL_COMPILE_ERROR;
		}

		/* Reject duplicate compiler-owned names before adding any local. */
		for (j = 0; j < i; j++) {
			if (strcmp(
				rewrite->region[j].args_name,
				rewrite->region[i].args_name) == 0 ||
			    strcmp(
				rewrite->region[j].session_name,
				rewrite->region[i].session_name) == 0) {
				accel_rewrite_destroy(rewrite);
				return accel_rewrite_error(
					func_block,
					N_TR("Duplicate accelerator region index."));
			}
		}

		search = next_search;
		previous = next_previous;
	}

	/* Preallocate every generated local without linking it into live HIR. */
	if (!accel_rewrite_stage_locals(rewrite)) {
		accel_rewrite_destroy(rewrite);
		return ACCEL_COMPILE_ERROR;
	}

	*result = rewrite;

	return ACCEL_COMPILE_APPLIED;
}

/*
 * Links all preallocated generated locals into unchanged live HIR.
 */
bool
accel_rewrite_add_locals(
	struct accel_rewrite *rewrite)
{
	if (rewrite == NULL)
		return false;
	if (rewrite->locals_added)
		return false;
	if (rewrite->committed)
		return false;
	if (rewrite->func_block->val.func.local !=
	    rewrite->original_local_head) {
		return false;
	}
	if (rewrite->detached_local_head == NULL ||
	    rewrite->detached_local_tail == NULL) {
		return false;
	}

	/* Link the complete detached chain with no remaining failure point. */
	rewrite->detached_local_tail->next = rewrite->original_local_head;
	rewrite->func_block->val.func.local = rewrite->detached_local_head;

	rewrite->locals_added = true;

	return true;
}

/*
 * Commits every staged link swap without allocation or failure.
 */
void
accel_rewrite_commit(
	struct accel_rewrite *rewrite)
{
	struct accel_rewrite_initializer *initializer;
	uint32_t i;
	uint32_t j;

	assert(rewrite != NULL);
	assert(rewrite->locals_added);
	assert(!rewrite->committed);

	/* Replace every disjoint top-level region atomically under its guard. */
	for (i = 0; i < rewrite->region_count; i++) {
		assert(*rewrite->region[i].link == rewrite->region[i].first);
		*rewrite->region[i].link = rewrite->region[i].replacement;

		/* Redirect declaration metadata to each detached initializer clone. */
		for (j = 0; j < rewrite->region[i].initializer_count; j++) {
			initializer = &rewrite->region[i].initializer[j];
			assert(initializer->local->declaration_stmt ==
			       initializer->source_statement);
			if (initializer->remove) {
				assert(initializer->replacement_statement == NULL);
				initializer->local->declaration_stmt = NULL;
				initializer->local->initializer = NULL;
				continue;
			}
			assert(initializer->replacement_statement != NULL);
			initializer->local->declaration_stmt =
				initializer->replacement_statement;
			initializer->local->initializer =
				initializer->replacement_statement->rhs;
		}
	}

	rewrite->func_block->val.func.is_accel = false;
	rewrite->committed = true;
}

/*
 * Destroys rewrite staging metadata without freeing arena objects.
 */
void
accel_rewrite_destroy(
	struct accel_rewrite *rewrite)
{
	if (rewrite == NULL)
		return;

	noct_free(rewrite->region);
	noct_free(rewrite);
}

/* Report one deterministic hard rewrite failure. */
static enum accel_compile_status
accel_rewrite_error(
	struct hir_block *func_block,
	const char *message)
{
	int line;

	line = 0;
	if (func_block != NULL)
		line = func_block->line;

	hir_error(line, message);

	return ACCEL_COMPILE_ERROR;
}

/* Resolve one source-ordered maximal region and its predecessor link. */
static bool
accel_rewrite_find_region(
	struct hir_block *func_block,
	const struct accel_program *program,
	struct hir_block *search_start,
	struct hir_block *search_prev,
	struct accel_rewrite_region *region,
	struct hir_block **next_search,
	struct hir_block **next_prev)
{
	struct hir_block *block;
	struct hir_block *previous;
	uint32_t visited;
	bool found_last;

	assert(func_block != NULL);
	assert(program != NULL);
	assert(region != NULL);
	assert(next_search != NULL);
	assert(next_prev != NULL);

	block = search_start;
	previous = search_prev;
	visited = 0;

	/* Preserve top-level blocks before the next planned region. */
	while (block != NULL && block != func_block->succ) {
		if (visited++ >= ACCEL_REWRITE_MAX_BLOCKS)
			return false;
		if (block->parent != func_block)
			return false;
		if (block->id == program->first_block_id)
			break;

		previous = block;
		block = block->succ;
	}

	if (block == NULL || block == func_block->succ)
		return false;

	region->first = block;
	if (previous == NULL)
		region->link = &func_block->val.func.inner;
	else
		region->link = &previous->succ;

	found_last = false;

	/* Find the inclusive end of the planned top-level region. */
	while (block != NULL && block != func_block->succ) {
		if (visited++ >= ACCEL_REWRITE_MAX_BLOCKS)
			return false;
		if (block->parent != func_block)
			return false;
		if (block->id == program->last_block_id) {
			found_last = true;
			break;
		}

		block = block->succ;
	}

	if (!found_last)
		return false;
	if (!accel_rewrite_validate_kernels(
		func_block,
		program,
		region,
		region->first,
		block)) {
		return false;
	}

	region->last = block;
	*next_search = block->succ;
	*next_prev = block;

	return true;
}

/* Match every kernel loop ID against the selected live region. */
static bool
accel_rewrite_validate_kernels(
	struct hir_block *func_block,
	const struct accel_program *program,
	struct accel_rewrite_region *region,
	struct hir_block *first,
	struct hir_block *last)
{
	struct hir_block *block;
	uint32_t kernel_index;
	uint32_t visited;
	uint32_t i;
	uint32_t j;
	bool found;

	if (program->kernel_count == 0 ||
	    program->kernel_count > ACCEL_MAX_KERNELS) {
		return false;
	}

	kernel_index = 0;
	visited = 0;
	block = first;

	/* Match selected ranged loops in source order. */
	while (block != NULL) {
		if (visited++ >= ACCEL_REWRITE_MAX_BLOCKS)
			return false;

		if (block->type == HIR_BLOCK_FOR) {
			if (!block->val.for_.is_ranged)
				return false;
			if (kernel_index >= program->kernel_count)
				return false;
			if (program->kernel[kernel_index].loop_block_id != block->id)
				return false;
			kernel_index++;
		} else if (block->type == HIR_BLOCK_BASIC) {
			if (block->val.basic.stmt_list != NULL) {
				if (!accel_rewrite_validate_initializer(
					func_block,
					program,
					region,
					block,
					kernel_index)) {
					return false;
				}
			}
		} else {
			return false;
		}

		if (block == last)
			break;
		block = block->succ;
	}

	if (block != last)
		return false;
	if (kernel_index != program->kernel_count)
		return false;

	/* Require one cloned declaration for every nonleading producer. */
	for (i = 0; i < program->scalar_result_count; i++) {
		if (program->scalar_result[i].producer_kernel == 0)
			continue;

		found = false;
		for (j = 0; j < region->initializer_count; j++) {
			if (region->initializer[j].before_kernel ==
			    program->scalar_result[i].producer_kernel) {
				found = true;
				break;
			}
		}
		if (!found)
			return false;
	}

	return true;
}

/* Stage one sole zero declaration between two planned kernels. */
static bool
accel_rewrite_validate_initializer(
	struct hir_block *func_block,
	const struct accel_program *program,
	struct accel_rewrite_region *region,
	struct hir_block *block,
	uint32_t before_kernel)
{
	const struct accel_scalar_result *result;
	struct accel_rewrite_initializer *initializer;
	struct hir_local *local;
	struct hir_stmt *statement;
	const char *symbol;
	uint32_t i;

	/* Recognize the sole removable device constructor at the region start. */
	if (before_kernel == 0) {
		return accel_rewrite_validate_device_initializer(
			func_block,
			program,
			region,
			block);
	}
	if (before_kernel >= program->kernel_count)
		return false;
	if (region->initializer_count >= ACCEL_MAX_KERNELS)
		return false;

	/* Reject duplicate declarations for the same producer boundary. */
	for (i = 0; i < region->initializer_count; i++) {
		if (region->initializer[i].before_kernel == before_kernel)
			return false;
	}

	statement = block->val.basic.stmt_list;
	if (statement == NULL || statement->next != NULL)
		return false;

	result = NULL;

	/* Match the upcoming producer to exactly one dense scalar result. */
	for (i = 0; i < program->scalar_result_count; i++) {
		if (program->scalar_result[i].producer_kernel != before_kernel)
			continue;
		if (result != NULL)
			return false;
		result = &program->scalar_result[i];
	}

	if (result == NULL)
		return false;
	if (result->name == NULL)
		return false;

	symbol = accel_rewrite_symbol(statement->lhs);
	if (symbol == NULL || strcmp(symbol, result->name) != 0)
		return false;
	if (!accel_rewrite_zero(statement->rhs))
		return false;

	local = accel_rewrite_find_local(func_block, result->name);
	if (local == NULL)
		return false;
	if (local->declaration_kind != HIR_LOCAL_DECL_VAR ||
	    local->declaration_stmt != statement ||
	    local->initializer != statement->rhs) {
		return false;
	}

	initializer = &region->initializer[region->initializer_count++];
	initializer->local = local;
	initializer->source_statement = statement;
	initializer->before_kernel = before_kernel;

	return true;
}

/* Stage one exact device-local constructor for removal at commit time. */
static bool
accel_rewrite_validate_device_initializer(
	struct hir_block *func_block,
	const struct accel_program *program,
	struct accel_rewrite_region *region,
	struct hir_block *block)
{
	const struct accel_buffer_binding *buffer;
	struct accel_rewrite_initializer *initializer;
	struct hir_local *local;
	struct hir_stmt *statement;
	const struct hir_expr *rhs;
	const char *object_name;
	const char *function_name;
	const char *symbol;
	uint32_t i;

	/* Require exactly one sole declaration at the leading region boundary. */
	if (region->initializer_count >= ACCEL_MAX_KERNELS)
		return false;
	statement = block->val.basic.stmt_list;
	if (statement == NULL || statement->next != NULL)
		return false;
	symbol = accel_rewrite_symbol(statement->lhs);
	if (symbol == NULL)
		return false;

	buffer = NULL;

	/* Match the source symbol to exactly one device-only descriptor. */
	for (i = 0; i < program->buffer_count; i++) {
		if (program->buffer[i].origin != ACCEL_BUFFER_LOCAL_DEVICE)
			continue;
		if (strcmp(program->buffer[i].name, symbol) != 0)
			continue;
		if (buffer != NULL)
			return false;
		buffer = &program->buffer[i];
	}
	if (buffer == NULL)
		return false;

	/* Revalidate the exact typed one-argument Packed constructor. */
	rhs = statement->rhs;
	if (rhs == NULL || rhs->type != HIR_EXPR_THISCALL)
		return false;
	if (rhs->val.thiscall.arg_count != 1 || rhs->val.thiscall.func == NULL)
		return false;
	object_name = accel_rewrite_symbol(rhs->val.thiscall.obj);
	if (object_name == NULL || strcmp(object_name, "Packed") != 0)
		return false;
	function_name = NULL;
	if (buffer->element_kind == NOCT_PACKED_INT32)
		function_name = "int32";
	else if (buffer->element_kind == NOCT_PACKED_UINT32)
		function_name = "uint32";
	else if (buffer->element_kind == NOCT_PACKED_FLOAT32)
		function_name = "float32";
	if (function_name == NULL ||
	    strcmp(rhs->val.thiscall.func, function_name) != 0) {
		return false;
	}

	/* Match live declaration metadata without mutating it during staging. */
	local = accel_rewrite_find_local(func_block, symbol);
	if (local == NULL ||
	    local->declaration_stmt != statement ||
	    local->initializer != rhs ||
	    local->declared_packed_type != buffer->element_kind) {
		return false;
	}

	initializer = &region->initializer[region->initializer_count++];
	initializer->local = local;
	initializer->source_statement = statement;
	initializer->before_kernel = 0;
	initializer->remove = true;

	/* Reports a transactionally staged constructor removal. */
	return true;
}

/* Preallocate the complete generated-local chain outside live HIR. */
static bool
accel_rewrite_stage_locals(
	struct accel_rewrite *rewrite)
{
	const struct hir_local *local;
	uint32_t generated_count;
	uint32_t existing_count;
	uint32_t i;
	int index;

	/* Prepare the unchanged source-local traversal. */
	existing_count = 0;
	local = rewrite->original_local_head;

	/* Count the unchanged source locals before assigning generated indices. */
	while (local != NULL) {
		if (existing_count >= (uint32_t)INT_MAX) {
			hir_error(
				rewrite->func_block->line,
				N_TR("Too many accelerator locals."));
			return false;
		}
		existing_count++;
		local = local->next;
	}

	/* Validate the complete index interval before allocating any entry. */
	generated_count = rewrite->region_count * 2;
	if (generated_count > (uint32_t)INT_MAX - existing_count) {
		hir_error(
			rewrite->func_block->line,
			N_TR("Too many accelerator locals."));
		return false;
	}

	/* Allocate names and local records in the historical insertion order. */
	index = (int)existing_count;
	for (i = 0; i < rewrite->region_count; i++) {
		if (!accel_rewrite_stage_local(
			rewrite,
			rewrite->region[i].args_name,
			index)) {
			return false;
		}
		index++;

		if (!accel_rewrite_stage_local(
			rewrite,
			rewrite->region[i].session_name,
			index)) {
			return false;
		}
		index++;
	}

	/* Reports a complete detached list ready for a no-fail link swap. */
	return true;
}

/* Allocate and prepend one fully initialized detached generated local. */
static bool
accel_rewrite_stage_local(
	struct accel_rewrite *rewrite,
	const char *symbol,
	int index)
{
	struct hir_local *local;
	char *owned_symbol;

	/* Allocate the detached local record. */
	local = hir_malloc(sizeof(*local));
	if (local == NULL) {
		hir_out_of_memory();
		return false;
	}

	/* Allocate the detached symbol before initializing the record. */
	owned_symbol = hir_strdup(symbol);
	if (owned_symbol == NULL) {
		hir_out_of_memory();
		return false;
	}

	/* Initialize the generated entry with the ordinary HIR local defaults. */
	memset(local, 0, sizeof(*local));
	local->symbol = owned_symbol;
	local->index = index;
	local->proven_type = -1;
	local->declaration_kind = HIR_LOCAL_DECL_UNKNOWN;
	local->declared_type = -1;
	local->declared_scalar_kind = HIR_DECL_SCALAR_UNKNOWN;
	local->declared_packed_type = -1;
	local->storage_class = HIR_LOCAL_STORAGE_UNKNOWN;
	local->declaration_line = -1;
	local->next = rewrite->detached_local_head;
	rewrite->detached_local_head = local;

	/* Retain the first allocated entry as the final link-chain tail. */
	if (rewrite->detached_local_tail == NULL)
		rewrite->detached_local_tail = local;

	/* Reports one fully initialized detached entry. */
	return true;
}

/* Check whether a generated symbol would collide with a live local. */
static bool
accel_rewrite_local_exists(
	const struct hir_block *func_block,
	const char *name)
{
	const struct hir_local *local;

	if (name == NULL)
		return false;

	local = func_block->val.func.local;

	/* Search every current local before staging compiler-owned names. */
	while (local != NULL) {
		if (strcmp(local->symbol, name) == 0)
			return true;
		local = local->next;
	}

	return false;
}

/* Find one mutable function local by its stable source symbol. */
static struct hir_local *
accel_rewrite_find_local(
	struct hir_block *func_block,
	const char *name)
{
	struct hir_local *local;

	if (name == NULL)
		return NULL;

	local = func_block->val.func.local;

	/* Search the function-local list in declaration order. */
	while (local != NULL) {
		if (strcmp(local->symbol, name) == 0)
			return local;
		local = local->next;
	}

	return NULL;
}

/* Extract a symbol from one optionally parenthesized term expression. */
static const char *
accel_rewrite_symbol(
	const struct hir_expr *expression)
{
	/* Remove redundant source parentheses. */
	while (expression != NULL && expression->type == HIR_EXPR_PAR)
		expression = expression->val.unary.expr;

	if (expression == NULL || expression->type != HIR_EXPR_TERM)
		return NULL;
	if (expression->val.term.term == NULL ||
	    expression->val.term.term->type != HIR_TERM_SYMBOL) {
		return NULL;
	}

	return expression->val.term.term->val.symbol;
}

/* Recognize the exact integer identity supported by scalar-result lowering. */
static bool
accel_rewrite_zero(
	const struct hir_expr *expression)
{
	/* Remove redundant source parentheses. */
	while (expression != NULL && expression->type == HIR_EXPR_PAR)
		expression = expression->val.unary.expr;

	if (expression == NULL || expression->type != HIR_EXPR_TERM)
		return false;
	if (expression->val.term.term == NULL ||
	    expression->val.term.term->type != HIR_TERM_INT) {
		return false;
	}
	if (expression->val.term.term->val.i != 0)
		return false;

	return true;
}

/* Build one detached begin/dispatch/finish basic block. */
static bool
accel_rewrite_build_region(
	struct hir_block *func_block,
	const struct accel_program *program,
	uint32_t program_id,
	struct accel_rewrite_region *region)
{
	struct hir_block *block;
	struct hir_stmt *statement;
	struct hir_stmt *tail;
	struct hir_expr *lhs;
	struct hir_expr *rhs;
	struct hir_expr *argument[2];
	uint32_t i;
	int length;

	length = snprintf(
		region->args_name,
		sizeof(region->args_name),
		"$accel.args.%lu",
		(unsigned long)program->region_index);
	if (length < 0 || (size_t)length >= sizeof(region->args_name)) {
		hir_error(program->source_line, N_TR("Accelerator local name is too long."));
		return false;
	}

	length = snprintf(
		region->session_name,
		sizeof(region->session_name),
		"$accel.session.%lu",
		(unsigned long)program->region_index);
	if (length < 0 || (size_t)length >= sizeof(region->session_name)) {
		hir_error(program->source_line, N_TR("Accelerator local name is too long."));
		return false;
	}

	if (accel_rewrite_local_exists(func_block, region->args_name) ||
	    accel_rewrite_local_exists(func_block, region->session_name)) {
		hir_error(program->source_line, N_TR("Accelerator local name collision."));
		return false;
	}

	block = accel_rewrite_new_block(
		func_block,
		region->last,
		region->first->line);
	if (block == NULL)
		return false;

	tail = NULL;
	lhs = accel_rewrite_new_symbol(region->args_name);
	if (lhs == NULL)
		return false;

	rhs = accel_rewrite_new_args_array(func_block, program);
	if (rhs == NULL)
		return false;

	statement = accel_rewrite_new_statement(block->line, lhs, rhs);
	if (statement == NULL)
		return false;

	if (!accel_rewrite_append_statement(block, &tail, statement))
		return false;

	lhs = accel_rewrite_new_symbol(region->session_name);
	if (lhs == NULL)
		return false;

	argument[0] = accel_rewrite_new_integer((int)program_id);
	if (argument[0] == NULL)
		return false;

	argument[1] = accel_rewrite_new_symbol(region->args_name);
	if (argument[1] == NULL)
		return false;

	rhs = accel_rewrite_new_thiscall("begin", 2, argument);
	if (rhs == NULL)
		return false;

	statement = accel_rewrite_new_statement(block->line, lhs, rhs);
	if (statement == NULL)
		return false;

	if (!accel_rewrite_append_statement(block, &tail, statement))
		return false;

	/* Emit every region kernel dispatch in deterministic source order. */
	for (i = 0; i < program->kernel_count; i++) {
		if (!accel_rewrite_append_initializers(
			block,
			&tail,
			region,
			i)) {
			return false;
		}

		argument[0] = accel_rewrite_new_symbol(region->session_name);
		if (argument[0] == NULL)
			return false;

		argument[1] = accel_rewrite_new_integer((int)i);
		if (argument[1] == NULL)
			return false;

		rhs = accel_rewrite_new_thiscall("dispatch", 2, argument);
		if (rhs == NULL)
			return false;

		statement = accel_rewrite_new_statement(block->line, NULL, rhs);
		if (statement == NULL)
			return false;

		if (!accel_rewrite_append_statement(block, &tail, statement))
			return false;
	}

	argument[0] = accel_rewrite_new_symbol(region->session_name);
	if (argument[0] == NULL)
		return false;

	argument[1] = accel_rewrite_new_symbol(region->args_name);
	if (argument[1] == NULL)
		return false;

	rhs = accel_rewrite_new_thiscall("finish", 2, argument);
	if (rhs == NULL)
		return false;

	statement = accel_rewrite_new_statement(block->line, NULL, rhs);
	if (statement == NULL)
		return false;

	if (!accel_rewrite_append_statement(block, &tail, statement))
		return false;

	/* Publish every CPU-visible scalar result after the session completes. */
	for (i = 0; i < program->scalar_result_count; i++) {
		if (!program->scalar_result[i].cpu_publication)
			continue;
		if (!accel_rewrite_local_exists(
			func_block,
			program->scalar_result[i].name)) {
			hir_error(
				program->source_line,
				N_TR("Accelerator scalar result has no local."));
			return false;
		}

		lhs = accel_rewrite_new_symbol(program->scalar_result[i].name);
		if (lhs == NULL)
			return false;

		rhs = accel_rewrite_new_subscript(
			region->args_name,
			program->scalar_result[i].args_slot);
		if (rhs == NULL)
			return false;

		statement = accel_rewrite_new_statement(block->line, lhs, rhs);
		if (statement == NULL)
			return false;

		if (!accel_rewrite_append_statement(block, &tail, statement))
			return false;
	}

	region->replacement = block;

	return true;
}

/* Clone every transparent declaration immediately before its producer. */
static bool
accel_rewrite_append_initializers(
	struct hir_block *block,
	struct hir_stmt **tail,
	struct accel_rewrite_region *region,
	uint32_t before_kernel)
{
	struct accel_rewrite_initializer *initializer;
	struct hir_expr *lhs;
	struct hir_expr *rhs;
	struct hir_stmt *statement;
	uint32_t i;

	/* Preserve source order when more scalar-result forms become available. */
	for (i = 0; i < region->initializer_count; i++) {
		initializer = &region->initializer[i];
		if (initializer->before_kernel != before_kernel)
			continue;
		if (initializer->remove)
			continue;

		lhs = accel_rewrite_new_symbol(initializer->local->symbol);
		if (lhs == NULL)
			return false;

		rhs = accel_rewrite_clone_zero(
			initializer->source_statement->rhs);
		if (rhs == NULL)
			return false;

		statement = accel_rewrite_new_statement(
			initializer->source_statement->line,
			lhs,
			rhs);
		if (statement == NULL)
			return false;
		if (!accel_rewrite_append_statement(block, tail, statement))
			return false;

		initializer->replacement_statement = statement;
	}

	return true;
}

/* Allocate and zero one detached replacement basic block. */
static struct hir_block *
accel_rewrite_new_block(
	struct hir_block *func_block,
	struct hir_block *last,
	int line)
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
	block->parent = func_block;
	block->succ = last->succ;
	block->id = hir_next_block_id();

	return block;
}

/* Allocate and zero one detached statement. */
static struct hir_stmt *
accel_rewrite_new_statement(
	int line,
	struct hir_expr *lhs,
	struct hir_expr *rhs)
{
	struct hir_stmt *statement;

	assert(rhs != NULL);

	statement = hir_malloc(sizeof(*statement));
	if (statement == NULL) {
		hir_out_of_memory();
		return NULL;
	}

	memset(statement, 0, sizeof(*statement));
	statement->line = line;
	statement->lhs = lhs;
	statement->rhs = rhs;

	return statement;
}

/* Allocate a zeroed symbol term expression and its owned string. */
static struct hir_expr *
accel_rewrite_new_symbol(
	const char *symbol)
{
	struct hir_expr *expression;
	struct hir_term *term;

	assert(symbol != NULL);

	expression = hir_malloc(sizeof(*expression));
	if (expression == NULL) {
		hir_out_of_memory();
		return NULL;
	}

	memset(expression, 0, sizeof(*expression));
	expression->type = HIR_EXPR_TERM;

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

	expression->val.term.term = term;

	return expression;
}

/* Allocate a zeroed integer term expression. */
static struct hir_expr *
accel_rewrite_new_integer(
	int value)
{
	struct hir_expr *expression;
	struct hir_term *term;

	expression = hir_malloc(sizeof(*expression));
	if (expression == NULL) {
		hir_out_of_memory();
		return NULL;
	}

	memset(expression, 0, sizeof(*expression));
	expression->type = HIR_EXPR_TERM;

	term = hir_malloc(sizeof(*term));
	if (term == NULL) {
		hir_out_of_memory();
		return NULL;
	}

	memset(term, 0, sizeof(*term));
	term->type = HIR_TERM_INT;
	term->val.i = value;
	expression->val.term.term = term;

	return expression;
}

/* Clone a parenthesized integer-zero initializer into detached HIR. */
static struct hir_expr *
accel_rewrite_clone_zero(
	const struct hir_expr *source)
{
	struct hir_expr *expression;

	if (source == NULL)
		return NULL;
	if (source->type == HIR_EXPR_TERM) {
		if (!accel_rewrite_zero(source))
			return NULL;

		return accel_rewrite_new_integer(0);
	}
	if (source->type != HIR_EXPR_PAR)
		return NULL;

	expression = hir_malloc(sizeof(*expression));
	if (expression == NULL) {
		hir_out_of_memory();
		return NULL;
	}

	memset(expression, 0, sizeof(*expression));
	expression->type = HIR_EXPR_PAR;
	expression->val.unary.expr = accel_rewrite_clone_zero(
		source->val.unary.expr);
	if (expression->val.unary.expr == NULL)
		return NULL;

	return expression;
}

/* Build one ordinary array subscript from a generated local and slot. */
static struct hir_expr *
accel_rewrite_new_subscript(
	const char *symbol,
	uint32_t index)
{
	struct hir_expr *expression;

	if (index >= HIR_PARAM_SIZE)
		return NULL;

	expression = hir_malloc(sizeof(*expression));
	if (expression == NULL) {
		hir_out_of_memory();
		return NULL;
	}

	memset(expression, 0, sizeof(*expression));
	expression->type = HIR_EXPR_SUBSCR;
	expression->val.binary.expr[0] = accel_rewrite_new_symbol(symbol);
	if (expression->val.binary.expr[0] == NULL)
		return NULL;
	expression->val.binary.expr[1] = accel_rewrite_new_integer((int)index);
	if (expression->val.binary.expr[1] == NULL)
		return NULL;

	return expression;
}

/* Build the ordered parameter and host-local runtime argument array. */
static struct hir_expr *
accel_rewrite_new_args_array(
	const struct hir_block *func_block,
	const struct accel_program *program)
{
	struct hir_expr *expression;
	struct hir_term *term;
	struct hir_expr **element;
	uint32_t element_count;
	uint32_t i;
	size_t size;

	/* Match the deep-owned prefix to the live function signature. */
	if (program->parameter_count != func_block->val.func.param_count)
		return NULL;

	element_count = func_block->val.func.param_count;

	/* Include every CPU-backed local at its planned dense argument slot. */
	for (i = 0; i < program->buffer_count; i++) {
		if (program->buffer[i].origin != ACCEL_BUFFER_LOCAL_HOST)
			continue;
		if (program->buffer[i].args_slot >= HIR_PARAM_SIZE)
			return NULL;
		if (program->buffer[i].args_slot >= element_count)
			element_count = program->buffer[i].args_slot + 1;
	}

	/* Reserve every scalar-result output slot, including GPU-only results. */
	for (i = 0; i < program->scalar_result_count; i++) {
		if (program->scalar_result[i].args_slot >= HIR_PARAM_SIZE)
			return NULL;
		if (program->scalar_result[i].args_slot >= element_count)
			element_count = program->scalar_result[i].args_slot + 1;
	}

	if (element_count == 0) {
		expression = hir_malloc(sizeof(*expression));
		if (expression == NULL) {
			hir_out_of_memory();
			return NULL;
		}

		memset(expression, 0, sizeof(*expression));
		expression->type = HIR_EXPR_TERM;

		term = hir_malloc(sizeof(*term));
		if (term == NULL) {
			hir_out_of_memory();
			return NULL;
		}

		memset(term, 0, sizeof(*term));
		term->type = HIR_TERM_EMPTY_ARRAY;
		expression->val.term.term = term;

		return expression;
	}

	if (element_count > HIR_PARAM_SIZE) {
		hir_error(func_block->line, N_TR("Invalid accelerator parameter count."));
		return NULL;
	}

	expression = hir_malloc(sizeof(*expression));
	if (expression == NULL) {
		hir_out_of_memory();
		return NULL;
	}

	memset(expression, 0, sizeof(*expression));
	expression->type = HIR_EXPR_ARRAY;
	expression->val.array.elem_count = element_count;

	size = sizeof(*element) * element_count;
	element = hir_malloc(size);
	if (element == NULL) {
		hir_out_of_memory();
		return NULL;
	}

	memset(element, 0, size);
	expression->val.array.elem = element;

	/* Preserve function parameters in declaration order. */
	for (i = 0; i < func_block->val.func.param_count; i++) {
		element[i] = accel_rewrite_new_symbol(
			func_block->val.func.param_name[i]);
		if (element[i] == NULL)
			return NULL;
	}

	/* Append only planned CPU-backed local buffers after the parameters. */
	for (i = 0; i < program->buffer_count; i++) {
		if (program->buffer[i].origin != ACCEL_BUFFER_LOCAL_HOST)
			continue;
		if (element[program->buffer[i].args_slot] != NULL)
			return NULL;
		element[program->buffer[i].args_slot] = accel_rewrite_new_symbol(
			program->buffer[i].name);
		if (element[program->buffer[i].args_slot] == NULL)
			return NULL;
	}

	/* Initialize result output slots with their exact additive identities. */
	for (i = 0; i < program->scalar_result_count; i++) {
		if (element[program->scalar_result[i].args_slot] != NULL)
			return NULL;
		element[program->scalar_result[i].args_slot] =
			accel_rewrite_new_integer(
				(int)program->scalar_result[i].identity_bits);
		if (element[program->scalar_result[i].args_slot] == NULL)
			return NULL;
	}

	/* Reject a malformed sparse runtime argument namespace. */
	for (i = 0; i < element_count; i++) {
		if (element[i] == NULL)
			return NULL;
	}

	return expression;
}

/* Build one ordinary private-package member call. */
static struct hir_expr *
accel_rewrite_new_thiscall(
	const char *function_name,
	uint32_t arg_count,
	struct hir_expr *const argument[])
{
	struct hir_expr *expression;
	uint32_t i;

	assert(function_name != NULL);
	assert(argument != NULL);

	if (arg_count >= HIR_PARAM_SIZE)
		return NULL;

	expression = hir_malloc(sizeof(*expression));
	if (expression == NULL) {
		hir_out_of_memory();
		return NULL;
	}

	memset(expression, 0, sizeof(*expression));
	expression->type = HIR_EXPR_THISCALL;
	expression->val.thiscall.obj = accel_rewrite_new_symbol("__Accel");
	if (expression->val.thiscall.obj == NULL)
		return NULL;

	expression->val.thiscall.func = hir_strdup(function_name);
	if (expression->val.thiscall.func == NULL) {
		hir_out_of_memory();
		return NULL;
	}

	expression->val.thiscall.arg_count = arg_count;

	/* Copy the already detached arguments into the inline HIR array. */
	for (i = 0; i < arg_count; i++) {
		if (argument[i] == NULL)
			return NULL;
		expression->val.thiscall.arg[i] = argument[i];
	}

	return expression;
}

/* Append one detached statement to a replacement block. */
static bool
accel_rewrite_append_statement(
	struct hir_block *block,
	struct hir_stmt **tail,
	struct hir_stmt *statement)
{
	assert(block != NULL);
	assert(block->type == HIR_BLOCK_BASIC);
	assert(tail != NULL);
	assert(statement != NULL);

	if (*tail == NULL)
		block->val.basic.stmt_list = statement;
	else
		(*tail)->next = statement;

	*tail = statement;

	return true;
}
