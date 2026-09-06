/* -*- coding: utf-8; tab-width: 8; indent-tabs-mode: t; -*- */

/*
 * Noct Programming Language
 * Copyright (c) 2025, 2026, Awe Morris
 */

/*
 * Pure LIR temporary-slot preflight for accelerator rewrites.
 */

#include "accel_lir_budget.h"
#include "lir.h"

#include <limits.h>
#include <stdlib.h>
#include <string.h>

#define ACCEL_BUDGET_MAX_BLOCKS		4096
#define ACCEL_BUDGET_MAX_STATEMENTS	65536

#define ACCEL_BUDGET_TYPE_UNKNOWN	(-1)

struct accel_budget_state {
	struct hir_block *func_block;
	const struct accel_function_plan *plan;
	uint32_t base;
	uint32_t current;
	uint32_t maximum;
	uint32_t block_count;
	uint32_t statement_count;
	enum accel_compile_status status;
};

static bool accel_budget_error(struct accel_budget_state *state, const char *message);
static bool accel_budget_decline(struct accel_budget_state *state);
static bool accel_budget_push(struct accel_budget_state *state);
static void accel_budget_pop(struct accel_budget_state *state);
static bool accel_budget_visit_top_level(struct accel_budget_state *state);
static bool accel_budget_visit_chain(struct accel_budget_state *state, struct hir_block *block);
static bool accel_budget_visit_block(struct accel_budget_state *state, struct hir_block *block);
static bool accel_budget_visit_statement(struct accel_budget_state *state, struct hir_block *block, const struct hir_stmt *statement);
static bool accel_budget_visit_expression(struct accel_budget_state *state, struct hir_block *block, const struct hir_expr *expression);
static bool accel_budget_visit_term(struct accel_budget_state *state, const struct hir_term *term);
static bool accel_budget_visit_unary(struct accel_budget_state *state, struct hir_block *block, const struct hir_expr *operand);
static bool accel_budget_visit_binary(struct accel_budget_state *state, struct hir_block *block, const struct hir_expr *left, const struct hir_expr *right);
static bool accel_budget_visit_call(struct accel_budget_state *state, struct hir_block *block, const struct hir_expr *function, uint32_t arg_count, struct hir_expr *const argument[]);
static bool accel_budget_visit_thiscall(struct accel_budget_state *state, struct hir_block *block, const struct hir_expr *object, uint32_t arg_count, struct hir_expr *const argument[]);
static bool accel_budget_visit_virtual_region(struct accel_budget_state *state, const struct accel_program *program, struct hir_block *first, struct hir_block *last);
static bool accel_budget_find_region_last(struct accel_budget_state *state, const struct accel_program *program, struct hir_block *first, struct hir_block **last);
static bool accel_budget_validate_initializer(struct accel_budget_state *state, const struct accel_program *program, struct hir_block *block, uint32_t before_kernel);
static bool accel_budget_validate_device_initializer(struct accel_budget_state *state, const struct accel_program *program, struct hir_block *block);
static bool accel_budget_args_count(struct accel_budget_state *state, const struct accel_program *program, uint32_t *count);
static bool accel_budget_count_statement(struct accel_budget_state *state);
static struct hir_local *accel_budget_find_local(const struct accel_budget_state *state, const char *symbol);
static const char *accel_budget_symbol(const struct hir_expr *expression);
static bool accel_budget_zero(const struct hir_expr *expression);
static bool accel_budget_lhs_is_local(const struct accel_budget_state *state, const struct hir_expr *lhs);
static bool accel_budget_typed_shift(const struct accel_budget_state *state, struct hir_block *block, const struct hir_expr *expression);
static int accel_budget_expression_type(const struct accel_budget_state *state, struct hir_block *block, const struct hir_expr *expression);
static int accel_budget_symbol_type(const struct accel_budget_state *state, struct hir_block *block, const char *symbol);
static int accel_budget_promote_numeric(int left, int right);

/*
 * Checks the final virtual HIR against the serialized LIR slot limit.
 */
enum accel_compile_status
accel_lir_budget_check(
	struct hir_block *func_block,
	const struct accel_function_plan *plan,
	uint32_t *serialized_tmpvar_size)
{
	struct accel_budget_state state;
	struct hir_local *local;
	uint32_t local_count;
	uint32_t final_local_count;
	uint32_t peak_top;
	uint32_t expected_local_count;

	if (serialized_tmpvar_size == NULL)
		return ACCEL_COMPILE_ERROR;

	*serialized_tmpvar_size = 0;
	memset(&state, 0, sizeof(state));
	state.func_block = func_block;
	state.plan = plan;
	state.status = ACCEL_COMPILE_APPLIED;

	if (func_block == NULL)
		return ACCEL_COMPILE_ERROR;
	if (func_block->type != HIR_BLOCK_FUNC) {
		accel_budget_error(&state, N_TR("Invalid accelerator budget function."));
		return state.status;
	}
	if (plan == NULL) {
		accel_budget_error(&state, N_TR("Missing accelerator function plan."));
		return state.status;
	}
	if (plan->region_count == 0) {
		accel_budget_error(&state, N_TR("Empty accelerator function plan."));
		return state.status;
	}
	if (plan->region_count > UINT32_MAX / 2) {
		accel_budget_error(&state, N_TR("Invalid accelerator local count."));
		return state.status;
	}

	expected_local_count = plan->region_count * 2;
	if (plan->generated_local_count != expected_local_count) {
		accel_budget_error(&state, N_TR("Invalid accelerator local count."));
		return state.status;
	}

	local_count = 0;
	local = func_block->val.func.local;

	/* Count every live LIR-frame local and reject a malformed cycle. */
	while (local != NULL) {
		if (local_count == UINT32_MAX) {
			accel_budget_error(&state, N_TR("Invalid accelerator local list."));
			return state.status;
		}
		local_count++;
		if (local_count > ACCEL_BUDGET_MAX_STATEMENTS) {
			accel_budget_error(&state, N_TR("Invalid accelerator local list."));
			return state.status;
		}
		local = local->next;
	}

	if (local_count > UINT32_MAX - plan->generated_local_count) {
		accel_budget_error(&state, N_TR("Invalid accelerator local count."));
		return state.status;
	}

	final_local_count = local_count + plan->generated_local_count;
	state.base = final_local_count;
	if (state.base == 0)
		state.base = 1;

	if (!accel_budget_visit_top_level(&state))
		return state.status;
	if (state.current != 0) {
		accel_budget_error(&state, N_TR("Invalid accelerator scratch budget."));
		return state.status;
	}
	if (state.base > UINT32_MAX - state.maximum) {
		accel_budget_error(&state, N_TR("Invalid accelerator scratch budget."));
		return state.status;
	}

	peak_top = state.base + state.maximum;
	if (peak_top >= LIR_TMPVAR_MAX)
		return ACCEL_COMPILE_DECLINED;

	*serialized_tmpvar_size = peak_top + 1;

	return ACCEL_COMPILE_APPLIED;
}

/* Report a malformed-HIR or transaction-invariant failure. */
static bool
accel_budget_error(
	struct accel_budget_state *state,
	const char *message)
{
	int line;

	line = 0;
	if (state->func_block != NULL)
		line = state->func_block->line;

	hir_error(line, message);
	state->status = ACCEL_COMPILE_ERROR;

	return false;
}

/* Record a valid shape whose exact scalar lowering is unsupported. */
static bool
accel_budget_decline(
	struct accel_budget_state *state)
{
	state->status = ACCEL_COMPILE_DECLINED;

	return false;
}

/* Mirror one lir_increment_tmpvar() operation. */
static bool
accel_budget_push(
	struct accel_budget_state *state)
{
	if (state->current == UINT32_MAX)
		return accel_budget_decline(state);

	state->current++;
	if (state->current > state->maximum)
		state->maximum = state->current;

	return true;
}

/* Mirror one LIFO lir_decrement_tmpvar() operation. */
static void
accel_budget_pop(
	struct accel_budget_state *state)
{
	if (state->current == 0) {
		accel_budget_error(state, N_TR("Invalid accelerator scratch budget."));
		return;
	}

	state->current--;
}

/* Visit retained top-level HIR and substitute every virtual region. */
static bool
accel_budget_visit_top_level(
	struct accel_budget_state *state)
{
	const struct accel_program *program;
	struct hir_block *block;
	struct hir_block *last;
	uint32_t region_index;

	region_index = 0;
	block = state->func_block->val.func.inner;

	/* Walk the final virtual top-level successor chain once. */
	while (block != NULL && block != state->func_block->succ) {
		if (state->block_count >= ACCEL_BUDGET_MAX_BLOCKS)
			return accel_budget_decline(state);
		if (block->parent != state->func_block) {
			return accel_budget_error(
				state,
				N_TR("Malformed accelerator function control flow."));
		}

		program = NULL;
		if (region_index < state->plan->region_count) {
			program = accel_function_plan_get_region(
				state->plan,
				region_index);
			if (program == NULL) {
				return accel_budget_error(
					state,
					N_TR("Invalid accelerator region plan."));
			}
		}

		if (program != NULL && block->id == program->first_block_id) {
			if (!accel_budget_find_region_last(
				state,
				program,
				block,
				&last)) {
				return false;
			}
			if (!accel_budget_visit_virtual_region(
				state,
				program,
				block,
				last)) {
				return false;
			}

			block = last->succ;
			region_index++;
			continue;
		}

		if (!accel_budget_visit_block(state, block))
			return false;
		if (block->stop)
			break;
		block = block->succ;
	}

	if (region_index != state->plan->region_count) {
		return accel_budget_error(
			state,
			N_TR("Accelerator plan does not match live HIR."));
	}

	return true;
}

/* Visit one structured inner successor chain exactly as lir.c does. */
static bool
accel_budget_visit_chain(
	struct accel_budget_state *state,
	struct hir_block *block)
{
	/* Visit each emitted inner block until its structured tail. */
	while (block != NULL) {
		if (!accel_budget_visit_block(state, block))
			return false;
		if (block->stop)
			break;
		block = block->succ;
	}

	return true;
}

/* Mirror the scratch discipline of one lir_visit_block() dispatch. */
static bool
accel_budget_visit_block(
	struct accel_budget_state *state,
	struct hir_block *block)
{
	struct hir_stmt *statement;
	bool ranged;

	if (block == NULL)
		return accel_budget_error(state, N_TR("Malformed accelerator HIR block."));
	if (state->block_count++ >= ACCEL_BUDGET_MAX_BLOCKS)
		return accel_budget_decline(state);

	/* Mirror every scalar block visitor that can reach lir_visit_expr(). */
	switch (block->type) {
	case HIR_BLOCK_BASIC:
		statement = block->val.basic.stmt_list;

		/* Visit statements in their emitted order. */
		while (statement != NULL) {
			if (!accel_budget_visit_statement(state, block, statement))
				return false;
			statement = statement->next;
		}
		break;
	case HIR_BLOCK_IF:
		if (block->val.if_.cond != NULL) {
			if (!accel_budget_push(state))
				return false;
			if (!accel_budget_visit_expression(
				state,
				block,
				block->val.if_.cond)) {
				return false;
			}
			accel_budget_pop(state);
			if (state->status != ACCEL_COMPILE_APPLIED)
				return false;
		}

		if (!accel_budget_visit_chain(state, block->val.if_.inner))
			return false;
		if (block->val.if_.chain_next != NULL) {
			if (!accel_budget_visit_block(
				state,
				block->val.if_.chain_next)) {
				return false;
			}
		}
		break;
	case HIR_BLOCK_FOR:
		if (block->val.for_.is_vector)
			return accel_budget_decline(state);

		ranged = block->val.for_.is_ranged;
		if (ranged) {
			if (block->val.for_.start == NULL ||
			    block->val.for_.stop == NULL) {
				return accel_budget_error(
					state,
					N_TR("Malformed accelerator ranged loop."));
			}

			if (!accel_budget_push(state))
				return false;
			if (!accel_budget_visit_expression(
				state,
				block,
				block->val.for_.start)) {
				return false;
			}

			if (!accel_budget_push(state))
				return false;
			if (!accel_budget_visit_expression(
				state,
				block,
				block->val.for_.stop)) {
				return false;
			}

			if (!accel_budget_push(state))
				return false;
			accel_budget_pop(state);

			if (!accel_budget_push(state))
				return false;
			if (!accel_budget_visit_chain(state, block->val.for_.inner))
				return false;
			accel_budget_pop(state);
			accel_budget_pop(state);
			accel_budget_pop(state);
		} else {
			if (block->val.for_.collection == NULL) {
				return accel_budget_error(
					state,
					N_TR("Malformed accelerator collection loop."));
			}

			if (!accel_budget_push(state))
				return false;
			if (!accel_budget_visit_expression(
				state,
				block,
				block->val.for_.collection)) {
				return false;
			}

			if (!accel_budget_push(state))
				return false;
			if (!accel_budget_push(state))
				return false;
			if (!accel_budget_push(state))
				return false;
			if (!accel_budget_visit_chain(state, block->val.for_.inner))
				return false;
			accel_budget_pop(state);
			accel_budget_pop(state);
			accel_budget_pop(state);
			accel_budget_pop(state);
		}
		break;
	case HIR_BLOCK_WHILE:
		if (block->val.while_.cond == NULL) {
			return accel_budget_error(
				state,
				N_TR("Malformed accelerator while loop."));
		}

		if (!accel_budget_push(state))
			return false;
		if (!accel_budget_visit_expression(
			state,
			block,
			block->val.while_.cond)) {
			return false;
		}
		accel_budget_pop(state);
		if (!accel_budget_visit_chain(state, block->val.while_.inner))
			return false;
		break;
	case HIR_BLOCK_END:
		break;
	default:
		return accel_budget_error(state, N_TR("Malformed accelerator HIR block."));
	}

	return state->status == ACCEL_COMPILE_APPLIED;
}

/* Mirror one scalar lir_visit_stmt() scratch lifetime. */
static bool
accel_budget_visit_statement(
	struct accel_budget_state *state,
	struct hir_block *block,
	const struct hir_stmt *statement)
{
	bool lhs_is_local;
	const struct hir_expr *lhs;

	if (state->statement_count++ >= ACCEL_BUDGET_MAX_STATEMENTS)
		return accel_budget_decline(state);
	if (statement->rhs == NULL)
		return accel_budget_error(state, N_TR("Malformed accelerator HIR statement."));

	lhs = statement->lhs;
	lhs_is_local = accel_budget_lhs_is_local(state, lhs);
	if (!lhs_is_local) {
		if (!accel_budget_push(state))
			return false;
	}

	if (!accel_budget_visit_expression(state, block, statement->rhs))
		return false;

	if (lhs != NULL && !lhs_is_local) {
		/* Mirror the extra object/access temporaries for non-local stores. */
		switch (lhs->type) {
		case HIR_EXPR_TERM:
			if (lhs->val.term.term == NULL ||
			    lhs->val.term.term->type != HIR_TERM_SYMBOL) {
				return accel_budget_error(
					state,
					N_TR("Malformed accelerator assignment target."));
			}
			break;
		case HIR_EXPR_SUBSCR:
		case HIR_EXPR_PSTORE8:
		case HIR_EXPR_PSTORE16:
		case HIR_EXPR_PSTORE32:
		case HIR_EXPR_PSTORE64:
		case HIR_EXPR_PSTOREF32:
			if (!accel_budget_push(state))
				return false;
			if (!accel_budget_visit_expression(
				state,
				block,
				lhs->val.binary.expr[0])) {
				return false;
			}

			if (!accel_budget_push(state))
				return false;
			if (!accel_budget_visit_expression(
				state,
				block,
				lhs->val.binary.expr[1])) {
				return false;
			}
			accel_budget_pop(state);
			accel_budget_pop(state);
			break;
		case HIR_EXPR_DOT:
			if (!accel_budget_push(state))
				return false;
			if (!accel_budget_visit_expression(
				state,
				block,
				lhs->val.dot.obj)) {
				return false;
			}
			accel_budget_pop(state);
			break;
		case HIR_EXPR_PMASKSTORE32:
			return accel_budget_decline(state);
		default:
			return accel_budget_error(
				state,
				N_TR("Malformed accelerator assignment target."));
		}
	}

	if (!lhs_is_local)
		accel_budget_pop(state);

	return state->status == ACCEL_COMPILE_APPLIED;
}

/* Mirror scalar lir_visit_expr() and each visitor's LIFO nesting. */
static bool
accel_budget_visit_expression(
	struct accel_budget_state *state,
	struct hir_block *block,
	const struct hir_expr *expression)
{
	uint32_t i;

	if (expression == NULL)
		return accel_budget_error(state, N_TR("Malformed accelerator HIR expression."));

	/* Keep this table synchronized with lir_visit_expr() in src/core/lir.c. */
	switch (expression->type) {
	case HIR_EXPR_TERM:
		return accel_budget_visit_term(state, expression->val.term.term);
	case HIR_EXPR_PAR:
	case HIR_EXPR_CAPTURE:
		if (expression->type == HIR_EXPR_PAR) {
			return accel_budget_visit_expression(
				state,
				block,
				expression->val.unary.expr);
		}

		return accel_budget_visit_expression(
			state,
			block,
			expression->val.capture.expr);
	case HIR_EXPR_NEG:
	case HIR_EXPR_NOT:
	case HIR_EXPR_PBASE:
	case HIR_EXPR_PLEN:
		return accel_budget_visit_unary(
			state,
			block,
			expression->val.unary.expr);
	case HIR_EXPR_PCHECK:
	case HIR_EXPR_TYPEIS:
		if (expression->val.binary.expr[1] == NULL ||
		    expression->val.binary.expr[1]->type != HIR_EXPR_TERM ||
		    expression->val.binary.expr[1]->val.term.term == NULL ||
		    expression->val.binary.expr[1]->val.term.term->type != HIR_TERM_INT) {
			return accel_budget_error(
				state,
				N_TR("Malformed accelerator type test."));
		}

		return accel_budget_visit_unary(
			state,
			block,
			expression->val.binary.expr[0]);
	case HIR_EXPR_LAND:
	case HIR_EXPR_LOR:
		if (!accel_budget_push(state))
			return false;
		if (!accel_budget_visit_expression(
			state,
			block,
			expression->val.binary.expr[0])) {
			return false;
		}
		if (!accel_budget_visit_expression(
			state,
			block,
			expression->val.binary.expr[1])) {
			return false;
		}
		accel_budget_pop(state);
		return state->status == ACCEL_COMPILE_APPLIED;
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
	case HIR_EXPR_PLOAD8U:
	case HIR_EXPR_PLOAD8S:
	case HIR_EXPR_PLOAD16U:
	case HIR_EXPR_PLOAD16S:
	case HIR_EXPR_PLOAD32:
	case HIR_EXPR_PLOAD64:
	case HIR_EXPR_PLOADF32:
		if (accel_budget_typed_shift(state, block, expression)) {
			return accel_budget_visit_unary(
				state,
				block,
				expression->val.binary.expr[0]);
		}

		return accel_budget_visit_binary(
			state,
			block,
			expression->val.binary.expr[0],
			expression->val.binary.expr[1]);
	case HIR_EXPR_PGATHER32:
		return accel_budget_visit_binary(
			state,
			block,
			expression->val.gather.packed,
			expression->val.gather.index);
	case HIR_EXPR_DOT:
		return accel_budget_visit_unary(
			state,
			block,
			expression->val.dot.obj);
	case HIR_EXPR_CALL:
		return accel_budget_visit_call(
			state,
			block,
			expression->val.call.func,
			expression->val.call.arg_count,
			expression->val.call.arg);
	case HIR_EXPR_THISCALL:
		if (expression->val.thiscall.func == NULL) {
			return accel_budget_error(
				state,
				N_TR("Malformed accelerator member call."));
		}

		return accel_budget_visit_thiscall(
			state,
			block,
			expression->val.thiscall.obj,
			expression->val.thiscall.arg_count,
			expression->val.thiscall.arg);
	case HIR_EXPR_ARRAY:
		if (expression->val.array.elem_count == 0 ||
		    expression->val.array.elem == NULL) {
			return accel_budget_error(
				state,
				N_TR("Malformed accelerator array expression."));
		}

		if (!accel_budget_push(state))
			return false;
		if (!accel_budget_push(state))
			return false;
		if (!accel_budget_push(state))
			return false;

		/* Evaluate every array element while all builder slots stay live. */
		for (i = 0; i < expression->val.array.elem_count; i++) {
			if (!accel_budget_visit_expression(
				state,
				block,
				expression->val.array.elem[i])) {
				return false;
			}
		}

		accel_budget_pop(state);
		accel_budget_pop(state);
		accel_budget_pop(state);
		return state->status == ACCEL_COMPILE_APPLIED;
	case HIR_EXPR_DICT:
		if (expression->val.dict.kv_count != 0 &&
		    (expression->val.dict.key == NULL ||
		     expression->val.dict.value == NULL)) {
			return accel_budget_error(
				state,
				N_TR("Malformed accelerator dictionary expression."));
		}

		if (!accel_budget_push(state))
			return false;
		if (!accel_budget_push(state))
			return false;
		if (!accel_budget_push(state))
			return false;
		if (!accel_budget_push(state))
			return false;

		/* Evaluate every dictionary value while all builder slots stay live. */
		for (i = 0; i < expression->val.dict.kv_count; i++) {
			if (expression->val.dict.key[i] == NULL) {
				return accel_budget_error(
					state,
					N_TR("Malformed accelerator dictionary expression."));
			}
			if (!accel_budget_visit_expression(
				state,
				block,
				expression->val.dict.value[i])) {
				return false;
			}
		}

		accel_budget_pop(state);
		accel_budget_pop(state);
		accel_budget_pop(state);
		accel_budget_pop(state);
		return state->status == ACCEL_COMPILE_APPLIED;
	case HIR_EXPR_NEW:
		if (expression->val.new_.cls == NULL) {
			return accel_budget_error(
				state,
				N_TR("Malformed accelerator new expression."));
		}

		if (!accel_budget_push(state))
			return false;
		if (!accel_budget_push(state))
			return false;
		if (!accel_budget_push(state))
			return false;
		if (expression->val.new_.init != NULL) {
			if (!accel_budget_visit_expression(
				state,
				block,
				expression->val.new_.init)) {
				return false;
			}
		}
		accel_budget_pop(state);
		accel_budget_pop(state);
		accel_budget_pop(state);
		return state->status == ACCEL_COMPILE_APPLIED;
	case HIR_EXPR_SELECT:
	case HIR_EXPR_VINDUCTF32:
	case HIR_EXPR_PMASKSTORE32:
		return accel_budget_decline(state);
	default:
		return accel_budget_decline(state);
	}
}

/* Validate a term whose lowering allocates no additional scratch. */
static bool
accel_budget_visit_term(
	struct accel_budget_state *state,
	const struct hir_term *term)
{
	if (term == NULL)
		return accel_budget_error(state, N_TR("Malformed accelerator HIR term."));

	/* Validate every scalar visitor case in lir_visit_term(). */
	switch (term->type) {
	case HIR_TERM_SYMBOL:
		if (term->val.symbol == NULL)
			return accel_budget_error(state, N_TR("Malformed accelerator symbol term."));
		break;
	case HIR_TERM_STRING:
		if (term->val.s == NULL)
			return accel_budget_error(state, N_TR("Malformed accelerator string term."));
		break;
	case HIR_TERM_INT:
	case HIR_TERM_LONG:
	case HIR_TERM_FLOAT:
	case HIR_TERM_DOUBLE:
	case HIR_TERM_EMPTY_ARRAY:
	case HIR_TERM_EMPTY_DICT:
		break;
	default:
		return accel_budget_decline(state);
	}

	return true;
}

/* Visit one operand held in one LIR scratch slot. */
static bool
accel_budget_visit_unary(
	struct accel_budget_state *state,
	struct hir_block *block,
	const struct hir_expr *operand)
{
	if (!accel_budget_push(state))
		return false;
	if (!accel_budget_visit_expression(state, block, operand))
		return false;
	accel_budget_pop(state);

	return state->status == ACCEL_COMPILE_APPLIED;
}

/* Visit two operands retained in two LIR scratch slots. */
static bool
accel_budget_visit_binary(
	struct accel_budget_state *state,
	struct hir_block *block,
	const struct hir_expr *left,
	const struct hir_expr *right)
{
	if (!accel_budget_push(state))
		return false;
	if (!accel_budget_visit_expression(state, block, left))
		return false;

	if (!accel_budget_push(state))
		return false;
	if (!accel_budget_visit_expression(state, block, right))
		return false;

	accel_budget_pop(state);
	accel_budget_pop(state);

	return state->status == ACCEL_COMPILE_APPLIED;
}

/* Visit a function expression and all simultaneously rooted arguments. */
static bool
accel_budget_visit_call(
	struct accel_budget_state *state,
	struct hir_block *block,
	const struct hir_expr *function,
	uint32_t arg_count,
	struct hir_expr *const argument[])
{
	uint32_t i;

	if (arg_count >= HIR_PARAM_SIZE)
		return accel_budget_error(state, N_TR("Malformed accelerator call expression."));

	if (!accel_budget_push(state))
		return false;
	if (!accel_budget_visit_expression(state, block, function))
		return false;

	/* Retain every preceding argument while evaluating the next one. */
	for (i = 0; i < arg_count; i++) {
		if (!accel_budget_push(state))
			return false;
		if (!accel_budget_visit_expression(state, block, argument[i]))
			return false;
	}

	/* Release arguments and the function slot in reverse allocation order. */
	for (i = 0; i < arg_count; i++)
		accel_budget_pop(state);
	accel_budget_pop(state);

	return state->status == ACCEL_COMPILE_APPLIED;
}

/* Visit a receiver, resolved method, and simultaneously rooted arguments. */
static bool
accel_budget_visit_thiscall(
	struct accel_budget_state *state,
	struct hir_block *block,
	const struct hir_expr *object,
	uint32_t arg_count,
	struct hir_expr *const argument[])
{
	uint32_t i;

	if (arg_count >= HIR_PARAM_SIZE)
		return accel_budget_error(state, N_TR("Malformed accelerator member call."));

	if (!accel_budget_push(state))
		return false;
	if (!accel_budget_visit_expression(state, block, object))
		return false;

	if (!accel_budget_push(state))
		return false;

	/* Retain the receiver and method while evaluating every argument. */
	for (i = 0; i < arg_count; i++) {
		if (!accel_budget_push(state))
			return false;
		if (!accel_budget_visit_expression(state, block, argument[i]))
			return false;
	}

	/* Release arguments, method, and receiver in reverse order. */
	for (i = 0; i < arg_count; i++)
		accel_budget_pop(state);
	accel_budget_pop(state);
	accel_budget_pop(state);

	return state->status == ACCEL_COMPILE_APPLIED;
}

/* Account for the exact detached region HIR shape. */
static bool
accel_budget_visit_virtual_region(
	struct accel_budget_state *state,
	const struct accel_program *program,
	struct hir_block *first,
	struct hir_block *last)
{
	struct hir_block *block;
	struct hir_stmt *initializer;
	struct hir_local *local;
	uint32_t args_count;
	uint32_t kernel_index;
	uint32_t slot;
	uint32_t i;

	if (program->kernel_count == 0)
		return accel_budget_error(state, N_TR("Empty accelerator region program."));

	if (state->block_count++ >= ACCEL_BUDGET_MAX_BLOCKS)
		return accel_budget_decline(state);

	/* Account for the generated args assignment and its array builder. */
	if (!accel_budget_count_statement(state))
		return false;
	if (!accel_budget_args_count(state, program, &args_count))
		return false;
	if (args_count != 0) {
		if (!accel_budget_push(state))
			return false;
		if (!accel_budget_push(state))
			return false;
		if (!accel_budget_push(state))
			return false;
		accel_budget_pop(state);
		accel_budget_pop(state);
		accel_budget_pop(state);
	}

	/* The begin assignment roots receiver, method, ID, and args. */
	if (!accel_budget_count_statement(state))
		return false;
	for (i = 0; i < 4; i++) {
		if (!accel_budget_push(state))
			return false;
	}

	/* Release every begin-call scratch slot. */
	for (i = 0; i < 4; i++)
		accel_budget_pop(state);

	kernel_index = 0;
	block = first;

	/* Interleave cloned declarations with their source-ordered dispatches. */
	while (block != NULL) {
		if (block->type == HIR_BLOCK_BASIC &&
		    block->val.basic.stmt_list != NULL) {
			initializer = block->val.basic.stmt_list;
			if (kernel_index != 0) {
				if (!accel_budget_visit_statement(
					state,
					block,
					initializer)) {
					return false;
				}
			}
		} else if (block->type == HIR_BLOCK_FOR) {
			if (kernel_index >= program->kernel_count)
				return accel_budget_error(
					state,
					N_TR("Accelerator plan does not match live HIR."));

			if (!accel_budget_count_statement(state))
				return false;

			/* Root destination, receiver, method, and two arguments. */
			for (slot = 0; slot < 5; slot++) {
				if (!accel_budget_push(state))
					return false;
			}

			/* Release every dispatch-call scratch slot. */
			for (slot = 0; slot < 5; slot++)
				accel_budget_pop(state);

			kernel_index++;
		}

		if (block == last)
			break;
		block = block->succ;
	}

	if (block != last || kernel_index != program->kernel_count) {
		return accel_budget_error(
			state,
			N_TR("Accelerator plan does not match live HIR."));
	}

	/* The finish expression statement has the same two-argument shape. */
	if (!accel_budget_count_statement(state))
		return false;
	for (i = 0; i < 5; i++) {
		if (!accel_budget_push(state))
			return false;
	}

	/* Release every finish-call scratch slot. */
	for (i = 0; i < 5; i++)
		accel_budget_pop(state);

	/* Account for each explicit result publication after finish. */
	for (i = 0; i < program->scalar_result_count; i++) {
		if (!program->scalar_result[i].cpu_publication)
			continue;
		if (program->scalar_result[i].name == NULL) {
			return accel_budget_error(
				state,
				N_TR("Invalid accelerator scalar result."));
		}

		local = accel_budget_find_local(
			state,
			program->scalar_result[i].name);
		if (local == NULL) {
			return accel_budget_error(
				state,
				N_TR("Accelerator scalar result has no local."));
		}
		if (program->scalar_result[i].args_slot >= HIR_PARAM_SIZE) {
			return accel_budget_error(
				state,
				N_TR("Invalid accelerator scalar-result slot."));
		}
		if (!accel_budget_count_statement(state))
			return false;

		/* The args subscript retains its object and index operands. */
		if (!accel_budget_push(state))
			return false;
		if (!accel_budget_push(state))
			return false;
		accel_budget_pop(state);
		accel_budget_pop(state);
	}

	return state->status == ACCEL_COMPILE_APPLIED;
}

/* Find and validate an inclusive planned region in the live top-level chain. */
static bool
accel_budget_find_region_last(
	struct accel_budget_state *state,
	const struct accel_program *program,
	struct hir_block *first,
	struct hir_block **last)
{
	struct hir_block *block;
	uint32_t kernel_index;
	uint32_t initializer_mask;
	uint32_t visited;
	uint32_t i;

	if (program->kernel_count == 0 ||
	    program->kernel_count > ACCEL_MAX_KERNELS) {
		return accel_budget_error(
			state,
			N_TR("Empty accelerator region program."));
	}

	*last = NULL;
	block = first;
	kernel_index = 0;
	initializer_mask = 0;
	visited = 0;

	/* Match every source-ordered kernel until the inclusive last block. */
	while (block != NULL && block != state->func_block->succ) {
		if (visited++ >= ACCEL_BUDGET_MAX_BLOCKS)
			return accel_budget_decline(state);
		if (block->parent != state->func_block) {
			return accel_budget_error(
				state,
				N_TR("Malformed accelerator region control flow."));
		}

		if (block->type == HIR_BLOCK_FOR) {
			if (!block->val.for_.is_ranged) {
				return accel_budget_error(
					state,
					N_TR("Accelerator plan does not match live HIR."));
			}
			if (kernel_index >= program->kernel_count ||
			    program->kernel[kernel_index].loop_block_id != block->id) {
				return accel_budget_error(
					state,
					N_TR("Accelerator plan does not match live HIR."));
			}
			kernel_index++;
		} else if (block->type == HIR_BLOCK_BASIC) {
			if (block->val.basic.stmt_list != NULL) {
				if (kernel_index >= ACCEL_MAX_KERNELS ||
				    (initializer_mask &
				     ((uint32_t)1U << kernel_index)) != 0) {
					return accel_budget_error(
						state,
						N_TR("Accelerator plan does not match live HIR."));
				}
				if (!accel_budget_validate_initializer(
					state,
					program,
					block,
					kernel_index)) {
					return false;
				}
				initializer_mask |= (uint32_t)1U << kernel_index;
			}
		} else {
			return accel_budget_error(
				state,
				N_TR("Accelerator plan does not match live HIR."));
		}

		if (block->id == program->last_block_id) {
			*last = block;
			break;
		}
		block = block->succ;
	}

	if (*last == NULL || kernel_index != program->kernel_count) {
		return accel_budget_error(
			state,
			N_TR("Accelerator plan does not match live HIR."));
	}

	/* Require one declaration scaffold for every nonleading producer. */
	for (i = 0; i < program->scalar_result_count; i++) {
		if (program->scalar_result[i].producer_kernel == 0)
			continue;
		if (program->scalar_result[i].producer_kernel >=
		    ACCEL_MAX_KERNELS) {
			return accel_budget_error(
				state,
				N_TR("Accelerator plan does not match live HIR."));
		}
		if ((initializer_mask &
		     ((uint32_t)1U <<
		      program->scalar_result[i].producer_kernel)) == 0) {
			return accel_budget_error(
				state,
				N_TR("Accelerator plan does not match live HIR."));
		}
	}

	return true;
}

/* Validate one sole zero declaration before its DOSUM producer kernel. */
static bool
accel_budget_validate_initializer(
	struct accel_budget_state *state,
	const struct accel_program *program,
	struct hir_block *block,
	uint32_t before_kernel)
{
	const struct accel_scalar_result *result;
	struct hir_local *local;
	struct hir_stmt *statement;
	const char *symbol;
	uint32_t i;

	/* Recognize the removable constructor before the first device producer. */
	if (before_kernel == 0) {
		return accel_budget_validate_device_initializer(
			state,
			program,
			block);
	}
	if (before_kernel >= program->kernel_count) {
		return accel_budget_error(
			state,
			N_TR("Accelerator plan does not match live HIR."));
	}

	statement = block->val.basic.stmt_list;
	if (statement == NULL || statement->next != NULL) {
		return accel_budget_error(
			state,
			N_TR("Accelerator plan does not match live HIR."));
	}

	result = NULL;

	/* Match the upcoming producer to exactly one scalar result. */
	for (i = 0; i < program->scalar_result_count; i++) {
		if (program->scalar_result[i].producer_kernel != before_kernel)
			continue;
		if (result != NULL) {
			return accel_budget_error(
				state,
				N_TR("Accelerator plan does not match live HIR."));
		}
		result = &program->scalar_result[i];
	}

	if (result == NULL) {
		return accel_budget_error(
			state,
			N_TR("Accelerator plan does not match live HIR."));
	}
	if (result->name == NULL) {
		return accel_budget_error(
			state,
			N_TR("Accelerator plan does not match live HIR."));
	}

	symbol = accel_budget_symbol(statement->lhs);
	if (symbol == NULL || strcmp(symbol, result->name) != 0) {
		return accel_budget_error(
			state,
			N_TR("Accelerator plan does not match live HIR."));
	}
	if (!accel_budget_zero(statement->rhs)) {
		return accel_budget_error(
			state,
			N_TR("Accelerator plan does not match live HIR."));
	}

	local = accel_budget_find_local(state, result->name);
	if (local == NULL ||
	    local->declaration_kind != HIR_LOCAL_DECL_VAR ||
	    local->declaration_stmt != statement ||
	    local->initializer != statement->rhs) {
		return accel_budget_error(
			state,
			N_TR("Accelerator plan does not match live HIR."));
	}

	return true;
}

/* Validate one sole device-local constructor omitted from virtual HIR. */
static bool
accel_budget_validate_device_initializer(
	struct accel_budget_state *state,
	const struct accel_program *program,
	struct hir_block *block)
{
	const struct accel_buffer_binding *buffer;
	struct hir_local *local;
	struct hir_stmt *statement;
	const struct hir_expr *rhs;
	const char *object_name;
	const char *function_name;
	const char *symbol;
	uint32_t i;

	statement = block->val.basic.stmt_list;
	if (statement == NULL || statement->next != NULL) {
		return accel_budget_error(
			state,
			N_TR("Accelerator plan does not match live HIR."));
	}
	symbol = accel_budget_symbol(statement->lhs);
	if (symbol == NULL) {
		return accel_budget_error(
			state,
			N_TR("Accelerator plan does not match live HIR."));
	}

	buffer = NULL;

	/* Match the declaration to exactly one device-only descriptor. */
	for (i = 0; i < program->buffer_count; i++) {
		if (program->buffer[i].origin != ACCEL_BUFFER_LOCAL_DEVICE)
			continue;
		if (strcmp(program->buffer[i].name, symbol) != 0)
			continue;
		if (buffer != NULL) {
			return accel_budget_error(
				state,
				N_TR("Accelerator plan does not match live HIR."));
		}
		buffer = &program->buffer[i];
	}
	if (buffer == NULL) {
		return accel_budget_error(
			state,
			N_TR("Accelerator plan does not match live HIR."));
	}

	/* Revalidate the exact direct typed Packed constructor. */
	rhs = statement->rhs;
	if (rhs == NULL ||
	    rhs->type != HIR_EXPR_THISCALL ||
	    rhs->val.thiscall.arg_count != 1 ||
	    rhs->val.thiscall.func == NULL) {
		return accel_budget_error(
			state,
			N_TR("Accelerator plan does not match live HIR."));
	}
	object_name = accel_budget_symbol(rhs->val.thiscall.obj);
	if (object_name == NULL || strcmp(object_name, "Packed") != 0) {
		return accel_budget_error(
			state,
			N_TR("Accelerator plan does not match live HIR."));
	}
	function_name = NULL;
	if (buffer->element_kind == NOCT_PACKED_INT32)
		function_name = "int32";
	else if (buffer->element_kind == NOCT_PACKED_UINT32)
		function_name = "uint32";
	else if (buffer->element_kind == NOCT_PACKED_FLOAT32)
		function_name = "float32";
	if (function_name == NULL ||
	    strcmp(rhs->val.thiscall.func, function_name) != 0) {
		return accel_budget_error(
			state,
			N_TR("Accelerator plan does not match live HIR."));
	}

	/* Match the live local metadata without counting the removed statement. */
	local = accel_budget_find_local(state, symbol);
	if (local == NULL ||
	    local->declaration_stmt != statement ||
	    local->initializer != rhs ||
	    local->declared_packed_type != buffer->element_kind) {
		return accel_budget_error(
			state,
			N_TR("Accelerator plan does not match live HIR."));
	}

	/* Reports an exact constructor that contributes no virtual LIR scratch. */
	return true;
}

/* Compute and validate the exact dense runtime args-array length. */
static bool
accel_budget_args_count(
	struct accel_budget_state *state,
	const struct accel_program *program,
	uint32_t *count)
{
	bool occupied[HIR_PARAM_SIZE];
	uint32_t param_count;
	uint32_t slot;
	uint32_t i;

	memset(occupied, 0, sizeof(occupied));
	param_count = state->func_block->val.func.param_count;
	if (param_count > HIR_PARAM_SIZE) {
		return accel_budget_error(
			state,
			N_TR("Invalid accelerator parameter count."));
	}
	if (program->parameter_count != param_count) {
		return accel_budget_error(
			state,
			N_TR("Invalid accelerator parameter count."));
	}

	/* Reserve every ordinary function parameter in declaration order. */
	for (i = 0; i < param_count; i++)
		occupied[i] = true;

	*count = param_count;

	/* Append every CPU-backed local buffer at its planned argument slot. */
	for (i = 0; i < program->buffer_count; i++) {
		if (program->buffer[i].origin != ACCEL_BUFFER_LOCAL_HOST)
			continue;

		slot = program->buffer[i].args_slot;
		if (slot >= HIR_PARAM_SIZE || occupied[slot]) {
			return accel_budget_error(
				state,
				N_TR("Invalid accelerator buffer slot."));
		}
		occupied[slot] = true;
		if (slot >= *count)
			*count = slot + 1;
	}

	/* Reserve every scalar-result output after parameters and buffers. */
	for (i = 0; i < program->scalar_result_count; i++) {
		slot = program->scalar_result[i].args_slot;
		if (slot >= HIR_PARAM_SIZE || occupied[slot]) {
			return accel_budget_error(
				state,
				N_TR("Invalid accelerator scalar-result slot."));
		}
		occupied[slot] = true;
		if (slot >= *count)
			*count = slot + 1;
	}

	/* Reject a sparse argument namespace that rewrite construction cannot fill. */
	for (i = 0; i < *count; i++) {
		if (!occupied[i]) {
			return accel_budget_error(
				state,
				N_TR("Invalid accelerator argument slots."));
		}
	}

	return true;
}

/* Count one statement in the final virtual basic block. */
static bool
accel_budget_count_statement(
	struct accel_budget_state *state)
{
	if (state->statement_count++ >= ACCEL_BUDGET_MAX_STATEMENTS)
		return accel_budget_decline(state);

	return true;
}

/* Find one live function local by source symbol. */
static struct hir_local *
accel_budget_find_local(
	const struct accel_budget_state *state,
	const char *symbol)
{
	struct hir_local *local;

	if (symbol == NULL)
		return NULL;

	local = state->func_block->val.func.local;

	/* Search every current local in declaration order. */
	while (local != NULL) {
		if (strcmp(local->symbol, symbol) == 0)
			return local;
		local = local->next;
	}

	return NULL;
}

/* Extract one optionally parenthesized local symbol. */
static const char *
accel_budget_symbol(
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

/* Recognize one optionally parenthesized integer zero identity. */
static bool
accel_budget_zero(
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

/* Check whether lir_visit_stmt() writes directly into an existing local. */
static bool
accel_budget_lhs_is_local(
	const struct accel_budget_state *state,
	const struct hir_expr *lhs)
{
	const struct hir_local *local;
	const char *symbol;

	if (lhs == NULL)
		return false;
	if (lhs->type != HIR_EXPR_TERM)
		return false;
	if (lhs->val.term.term == NULL)
		return false;
	if (lhs->val.term.term->type != HIR_TERM_SYMBOL)
		return false;

	symbol = lhs->val.term.term->val.symbol;
	if (symbol == NULL)
		return false;
	if (strcmp(symbol, "$return") == 0)
		return true;

	local = state->func_block->val.func.local;

	/* Find the local slot used as the statement's direct destination. */
	while (local != NULL) {
		if (strcmp(local->symbol, symbol) == 0)
			return true;
		local = local->next;
	}

	return false;
}

/* Match lir_visit_binary_expr()'s one-slot typed shift special case. */
static bool
accel_budget_typed_shift(
	const struct accel_budget_state *state,
	struct hir_block *block,
	const struct hir_expr *expression)
{
	const struct hir_expr *right;

	if (expression->type != HIR_EXPR_SHL && expression->type != HIR_EXPR_SHR)
		return false;
	if (getenv("NOCT_TYPED_DISABLE") != NULL)
		return false;
	if (expression->val.binary.expr[0] == NULL)
		return false;

	right = expression->val.binary.expr[1];
	if (right == NULL)
		return false;
	if (right->type != HIR_EXPR_TERM)
		return false;
	if (right->val.term.term == NULL)
		return false;
	if (right->val.term.term->type != HIR_TERM_INT)
		return false;
	if (right->val.term.term->val.i < 0)
		return false;
	if (right->val.term.term->val.i > 31)
		return false;
	if (accel_budget_expression_type(
		state,
		block,
		expression->val.binary.expr[0]) != NOCT_VALUE_INT) {
		return false;
	}

	return true;
}

/* Infer only the tags used by scalar typed-shift lowering. */
static int
accel_budget_expression_type(
	const struct accel_budget_state *state,
	struct hir_block *block,
	const struct hir_expr *expression)
{
	int intrinsic;
	int left;
	int right;

	if (expression == NULL)
		return ACCEL_BUDGET_TYPE_UNKNOWN;

	/* Mirror lir_expr_proven_type() for all recursively relevant cases. */
	switch (expression->type) {
	case HIR_EXPR_TERM:
		if (expression->val.term.term == NULL)
			return ACCEL_BUDGET_TYPE_UNKNOWN;

		/* Infer the exact literal or local symbol tag. */
		switch (expression->val.term.term->type) {
		case HIR_TERM_INT:
			return NOCT_VALUE_INT;
		case HIR_TERM_LONG:
			return NOCT_VALUE_LONG;
		case HIR_TERM_FLOAT:
			return NOCT_VALUE_FLOAT;
		case HIR_TERM_DOUBLE:
			return NOCT_VALUE_DOUBLE;
		case HIR_TERM_STRING:
			return NOCT_VALUE_STRING;
		case HIR_TERM_EMPTY_ARRAY:
			return NOCT_VALUE_ARRAY;
		case HIR_TERM_EMPTY_DICT:
			return NOCT_VALUE_DICT;
		case HIR_TERM_SYMBOL:
			return accel_budget_symbol_type(
				state,
				block,
				expression->val.term.term->val.symbol);
		default:
			return ACCEL_BUDGET_TYPE_UNKNOWN;
		}
	case HIR_EXPR_PAR:
	case HIR_EXPR_NEG:
		return accel_budget_expression_type(
			state,
			block,
			expression->val.unary.expr);
	case HIR_EXPR_CAPTURE:
		return accel_budget_expression_type(
			state,
			block,
			expression->val.capture.expr);
	case HIR_EXPR_NOT:
		left = accel_budget_expression_type(
			state,
			block,
			expression->val.unary.expr);
		if (left == ACCEL_BUDGET_TYPE_UNKNOWN)
			return ACCEL_BUDGET_TYPE_UNKNOWN;
		return NOCT_VALUE_INT;
	case HIR_EXPR_PLUS:
	case HIR_EXPR_MINUS:
	case HIR_EXPR_MUL:
	case HIR_EXPR_DIV:
		left = accel_budget_expression_type(
			state,
			block,
			expression->val.binary.expr[0]);
		right = accel_budget_expression_type(
			state,
			block,
			expression->val.binary.expr[1]);
		return accel_budget_promote_numeric(left, right);
	case HIR_EXPR_MOD:
	case HIR_EXPR_AND:
	case HIR_EXPR_OR:
	case HIR_EXPR_XOR:
	case HIR_EXPR_SHL:
	case HIR_EXPR_SHR:
		left = accel_budget_expression_type(
			state,
			block,
			expression->val.binary.expr[0]);
		right = accel_budget_expression_type(
			state,
			block,
			expression->val.binary.expr[1]);
		if ((left == NOCT_VALUE_INT || left == NOCT_VALUE_LONG) &&
		    (right == NOCT_VALUE_INT || right == NOCT_VALUE_LONG)) {
			if (left == NOCT_VALUE_LONG || right == NOCT_VALUE_LONG)
				return NOCT_VALUE_LONG;
			return NOCT_VALUE_INT;
		}
		return ACCEL_BUDGET_TYPE_UNKNOWN;
	case HIR_EXPR_LT:
	case HIR_EXPR_LTE:
	case HIR_EXPR_GT:
	case HIR_EXPR_GTE:
	case HIR_EXPR_EQ:
	case HIR_EXPR_NEQ:
		left = accel_budget_expression_type(
			state,
			block,
			expression->val.binary.expr[0]);
		right = accel_budget_expression_type(
			state,
			block,
			expression->val.binary.expr[1]);
		if (left != ACCEL_BUDGET_TYPE_UNKNOWN &&
		    right != ACCEL_BUDGET_TYPE_UNKNOWN) {
			return NOCT_VALUE_INT;
		}
		return ACCEL_BUDGET_TYPE_UNKNOWN;
	case HIR_EXPR_PLOAD8U:
	case HIR_EXPR_PLOAD8S:
	case HIR_EXPR_PLOAD16U:
	case HIR_EXPR_PLOAD16S:
	case HIR_EXPR_PLOAD32:
	case HIR_EXPR_PLEN:
	case HIR_EXPR_PCHECK:
	case HIR_EXPR_TYPEIS:
		return NOCT_VALUE_INT;
	case HIR_EXPR_PLOADF32:
		return NOCT_VALUE_FLOAT;
	case HIR_EXPR_PLOAD64:
	case HIR_EXPR_PBASE:
		return NOCT_VALUE_LONG;
	case HIR_EXPR_CALL:
		intrinsic = hir_get_intrinsic_call(expression);
		if (intrinsic == HIR_INTRINSIC_INT_FROM)
			return NOCT_VALUE_INT;
		if (intrinsic == HIR_INTRINSIC_FLOAT_FROM)
			return NOCT_VALUE_FLOAT;
		return ACCEL_BUDGET_TYPE_UNKNOWN;
	case HIR_EXPR_ARRAY:
		return NOCT_VALUE_ARRAY;
	case HIR_EXPR_DICT:
		return NOCT_VALUE_DICT;
	default:
		return ACCEL_BUDGET_TYPE_UNKNOWN;
	}
}

/* Infer one local symbol's tag and enclosing typed-int region. */
static int
accel_budget_symbol_type(
	const struct accel_budget_state *state,
	struct hir_block *block,
	const char *symbol)
{
	const struct hir_local *local;
	struct hir_block *parent;
	bool typed_int_region;

	if (symbol == NULL)
		return ACCEL_BUDGET_TYPE_UNKNOWN;

	typed_int_region = false;
	parent = block;

	/* Detect the same enclosing ABCE typed region used by lir.c. */
	while (parent != NULL && parent->type != HIR_BLOCK_FUNC) {
		if (parent->type == HIR_BLOCK_FOR &&
		    parent->val.for_.typed_int_region) {
			typed_int_region = true;
		}
		parent = parent->parent;
	}

	local = state->func_block->val.func.local;

	/* Find the symbol's already-computed typed-pass lattice value. */
	while (local != NULL) {
		if (strcmp(local->symbol, symbol) == 0)
			break;
		local = local->next;
	}

	if (local == NULL)
		return ACCEL_BUDGET_TYPE_UNKNOWN;
	if (typed_int_region)
		return NOCT_VALUE_INT;

	if (local->proven_type == NOCT_VALUE_INT)
		return NOCT_VALUE_INT;
	if (local->proven_type == NOCT_VALUE_LONG)
		return NOCT_VALUE_LONG;
	if (local->proven_type == NOCT_VALUE_FLOAT)
		return NOCT_VALUE_FLOAT;
	if (local->proven_type == NOCT_VALUE_DOUBLE)
		return NOCT_VALUE_DOUBLE;

	return ACCEL_BUDGET_TYPE_UNKNOWN;
}

/* Promote two proven scalar numeric tags exactly as scalar LIR does. */
static int
accel_budget_promote_numeric(
	int left,
	int right)
{
	if ((left != NOCT_VALUE_INT &&
	     left != NOCT_VALUE_LONG &&
	     left != NOCT_VALUE_FLOAT &&
	     left != NOCT_VALUE_DOUBLE) ||
	    (right != NOCT_VALUE_INT &&
	     right != NOCT_VALUE_LONG &&
	     right != NOCT_VALUE_FLOAT &&
	     right != NOCT_VALUE_DOUBLE)) {
		return ACCEL_BUDGET_TYPE_UNKNOWN;
	}

	if (left == NOCT_VALUE_DOUBLE || right == NOCT_VALUE_DOUBLE)
		return NOCT_VALUE_DOUBLE;
	if (left == NOCT_VALUE_FLOAT || right == NOCT_VALUE_FLOAT)
		return NOCT_VALUE_FLOAT;
	if (left == NOCT_VALUE_LONG || right == NOCT_VALUE_LONG)
		return NOCT_VALUE_LONG;
	if (left == NOCT_VALUE_INT && right == NOCT_VALUE_INT)
		return NOCT_VALUE_INT;

	return ACCEL_BUDGET_TYPE_UNKNOWN;
}
