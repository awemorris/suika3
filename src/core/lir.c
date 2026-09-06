/* -*- coding: utf-8; tab-width: 8; indent-tabs-mode: t; -*- */

/*
 * Noct Programming Language
 * Copyright (c) 2025, 2026, Awe Morris
 */

/*
 * LIR: Low-level Intermediate Representation
 */

#include <noct/noct.h>
#include "lir.h"
#include "hir.h"
#include "bytecode.h"
#if defined(NOCT_USE_OPTIMIZER)
#include "fast.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <assert.h>

/* False assertion */
#define NEVER_COME_HERE		0
#define INVALID_OPCODE		0

/* Debug print */
#undef DEBUG_BLOCK_ORDER
/* DEBUG_DUMP_LIR may be supplied by a diagnostic build. */

/*
 * Target LIR.
 */

#define BYTECODE_BUF_SIZE	65536

/* Bytecode array. */
/*static uint8_t bytecode[BYTECODE_BUF_SIZE];*/
static uint8_t *bytecode;

/* Cuurent bytecode length. */
static uint32_t bytecode_top;

/*
 * Variable table.
 */

static uint32_t tmpvar_top;
static uint32_t tmpvar_count;
static uint32_t tmpvar_local_count;

enum lir_tmp_type_state {
	LIR_TMP_TYPE_UNSEEN,
	LIR_TMP_TYPE_FIXED,
	LIR_TMP_TYPE_DYNAMIC
};
static uint8_t tmpvar_type_state[LIR_TMPVAR_MAX];
static int8_t tmpvar_fixed_type[LIR_TMPVAR_MAX];
/* A compiler-temp lifetime must record every direct/manual definition before
 * it can contribute a fixed-type fact to a reused frame slot. */
static bool tmpvar_lifetime_noted[LIR_TMPVAR_MAX];
/* Primitive ordinary-local annotations checked at every definition. */
static int8_t tmpvar_contract_type[LIR_TMPVAR_MAX];

/* ABI/prologue metadata for the function currently being built. */
static bool has_vector_ops;
static bool has_fma_ops;

/* Per-function virtual PBASE allocation IDs (0/1 are currently useful). */
static uint8_t pbase_hint_next;

/*
 * Typed-op emission state (docs/design/07-typed-ops.md, D-TOP11).
 * Reset per lir_build().
 */
static int typed_emit_int_count;
static int typed_emit_float_count;
static int typed_generic_count;
static int typed_emit_checked_div_count;
static int typed_disabled;
static int materialize_emit_count;

/*
 * Relocation table.
 */

/* Maximum relocation count */
#define LOC_MAX	1024

/* Relocation type */
#define LOC_BLOCK_TOP		0
#define LOC_BLOCK_CONTINUE	1
#define LOC_ABSOLUTE		2

struct loc_entry {
	/* Type. */
	int type;

	/* Location offset. */
	uint32_t offset;

	/* Branch target. */
	struct hir_block *block;
	uint32_t addr;
};

static struct loc_entry loc_tbl[LOC_MAX];
static int loc_count;

/*
 * Error position and message.
 */

static char *lir_file_name;
static int lir_error_line;
static char lir_error_message[1024];

/*
 * Optimize lelve.
 */
int lir_optimize_level = 0;
static bool lir_lineinfo = true;

/*
 * Set the optimization level. (Propagated from NoctConfig; see
 * docs/design/01-abce.md section 3.7.)
 */
void
lir_set_optimize_level(int level)
{
	lir_optimize_level = level;
}

void
lir_set_lineinfo(bool enable)
{
	lir_lineinfo = enable;
}

/*
 * Forward declaration.
 */
static uint32_t lir_count_local(struct hir_block *func);
static bool lir_visit_block(struct hir_block *block);
static bool lir_visit_basic_block(struct hir_block *block);
static bool lir_check_succ_loop_head(struct hir_block *block, struct hir_block **loop);
static bool lir_visit_if_block(struct hir_block *block);
static bool lir_visit_for_block(struct hir_block *block);
static bool lir_visit_vfor_block(struct hir_block *block);
static bool lir_visit_for_range_block(struct hir_block *block);
static bool lir_visit_for_kv_block(struct hir_block *block);
static bool lir_visit_for_v_block(struct hir_block *block);
static int lir_get_local_index(struct hir_block *block, const char *symbol);
static struct hir_local *lir_get_local_by_index(struct hir_block *block, int index);
static bool lir_visit_while_block(struct hir_block *block);
static bool lir_visit_stmt(struct hir_block *block, struct hir_stmt *stmt);
static bool lir_check_lhs_local(struct hir_block *block, struct hir_expr *lhs, int *rhs_tmpvar);
static bool lir_visit_expr(int dst_tmpvar, struct hir_expr *expr, struct hir_block *block);
static int lir_expr_proven_type(struct hir_expr *expr, struct hir_block *block);
static bool lir_visit_abce_unary_expr(int dst_tmpvar, struct hir_expr *expr, struct hir_block *block);
static bool lir_visit_abce_typetest_expr(int dst_tmpvar, struct hir_expr *expr, struct hir_block *block);
static bool lir_visit_unary_expr(int dst_tmpvar, struct hir_expr *expr, struct hir_block *block);
static bool lir_visit_binary_expr(int dst_tmpvar, struct hir_expr *expr, struct hir_block *block);
static bool lir_visit_logical_expr(int dst_tmpvar, struct hir_expr *expr, struct hir_block *block);
static bool lir_visit_dot_expr(int dst_tmpvar, struct hir_expr *expr, struct hir_block *block);
static bool lir_visit_capture_expr(int dst_tmpvar, struct hir_expr *expr, struct hir_block *block);
static bool lir_visit_call_expr(int dst_tmpvar, struct hir_expr *expr, struct hir_block *block);
static bool lir_visit_thiscall_expr(int dst_tmpvar, struct hir_expr *expr, struct hir_block *block);
static bool lir_visit_array_expr(int dst_tmpvar, struct hir_expr *expr, struct hir_block *block);
static bool lir_visit_dict_expr(int dst_tmpvar, struct hir_expr *expr, struct hir_block *block);
static bool lir_visit_new_expr(int dst_tmpvar, struct hir_expr *expr, struct hir_block *block);
static bool lir_visit_term(int dst_tmpvar, struct hir_term *term, struct hir_block *block);
static bool lir_visit_symbol_term(int dst_tmpvar, struct hir_term *term, struct hir_block *block);
static bool lir_visit_int_term(int dst_tmpvar, struct hir_term *term);
static bool lir_visit_long_term(int dst_tmpvar, struct hir_term *term);
static bool lir_visit_float_term(int dst_tmpvar, struct hir_term *term);
static bool lir_visit_double_term(int dst_tmpvar, struct hir_term *term);
static bool lir_visit_string_term(int dst_tmpvar, struct hir_term *term);
static bool lir_visit_empty_array_term(int dst_tmpvar, struct hir_term *term);
static bool lir_visit_empty_dict_term(int dst_tmpvar, struct hir_term *term);
static bool lir_increment_tmpvar(int *tmpvar_index);
static bool lir_decrement_tmpvar(int tmpvar_index);
static bool lir_put_opcode(uint8_t op);
static bool lir_put_tmpvar(uint16_t index);
static bool lir_put_imm8(uint8_t imm);
static bool lir_put_imm32(uint32_t imm);
static bool lir_put_imm64(uint64_t imm);
static bool lir_put_string(const char *data);
static bool lir_put_branch_addr(struct hir_block *block);
static bool lir_put_continue_addr(struct hir_block *block);
static bool lir_put_absolute_addr(uint32_t addr);
static bool lir_put_u8(uint8_t b);
static bool lir_put_u16(uint16_t b);
static bool lir_put_u32(uint32_t b);
static bool lir_put_u64(uint64_t b);
static void patch_block_address(uint32_t prefix);
static void lir_note_tmpvar_type(int tmpvar, int type);
static bool lir_prepend_tmpvar_types(uint32_t *prefix);
static bool lir_put_materialize_type(int tmpvar, int type);
static void lir_fatal(const char *msg, ...);
static void lir_out_of_memory(void);
static void lir_free_func_body(struct lir_func *func);

/* Encode a declared type for OP_CHECKTYPE. */
static int
lir_typecheck_code(
	int type,
	int packed_type,
	bool restricted,
	bool is_return,
	bool is_local)
{
	int code;

	code = type;
	if (type == NOCT_VALUE_PACKED &&
	    packed_type >= 0 && packed_type != NOCT_PACKED_ANY)
		code = (restricted ? TYPECHECK_RPACKED_BASE :
			TYPECHECK_PACKED_BASE) + packed_type;
	if (is_return)
		code |= TYPECHECK_RETURN_FLAG;
	if (is_local)
		code |= TYPECHECK_LOCAL_FLAG;
	return code;
}

static struct hir_block *
lir_root_func(struct hir_block *block)
{
	while (block != NULL && block->type != HIR_BLOCK_FUNC)
		block = block->parent;
	return block;
}

static bool
lir_emit_return_check(struct hir_block *block)
{
	struct hir_block *func;
	int code;

	func = lir_root_func(block);
	assert(func != NULL);
	if ((lir_optimize_level < 2 && !func->val.func.is_fast) ||
	    func->val.func.return_type < 0) {
		return true;
	}
	code = lir_typecheck_code(func->val.func.return_type,
				  func->val.func.return_packed_type,
				  false, true, false);
	if (!lir_put_opcode(OP_CHECKTYPE))
		return false;
	if (!lir_put_tmpvar(0))
		return false;
	if (!lir_put_imm8((uint8_t)code))
		return false;
	return true;
}

/*
 * Build a LIR function from a HIR function.
 */
bool
lir_build(
	struct hir_block *hir_func,
	struct lir_func **lir_func)
{
	struct hir_block *cur_block;
	uint32_t i;

	assert(hir_func != NULL);
	assert(hir_func->type == HIR_BLOCK_FUNC);
	assert(lir_func != NULL);

	*lir_func = NULL;

	/* Copy the file name. */
	noct_free(lir_file_name);
	lir_file_name = NULL;
	lir_file_name = noct_strdup(hir_func->val.func.file_name);
	if (lir_file_name == NULL) {
		lir_out_of_memory();
		return false;
	}

	/* Initialize the bytecode buffer. */
	if (bytecode != NULL) {
		noct_free(bytecode);
		bytecode = NULL;
	}
	bytecode = noct_calloc(BYTECODE_BUF_SIZE, 1);
	if (bytecode == NULL) {
		lir_out_of_memory();
		noct_free(bytecode);
		bytecode = NULL;
		return false;
	}
	bytecode_top = 0;
	has_vector_ops = false;
	has_fma_ops = false;
	pbase_hint_next = 0;

	/* Typed-op emission state (design 07). */
	typed_emit_int_count = 0;
	typed_emit_float_count = 0;
	typed_generic_count = 0;
	typed_emit_checked_div_count = 0;
	materialize_emit_count = 0;
	typed_disabled = (getenv("NOCT_TYPED_DISABLE") != NULL);

	/* Initialize the tmpvars. */
	tmpvar_top = lir_count_local(hir_func);
	if (tmpvar_top == 0) {
		/* For the return value. */
		tmpvar_top = 1;
	}
	tmpvar_count = tmpvar_top;
	tmpvar_local_count = tmpvar_top;
	memset(tmpvar_type_state, LIR_TMP_TYPE_UNSEEN,
	       sizeof(tmpvar_type_state));
	memset(tmpvar_fixed_type, -1, sizeof(tmpvar_fixed_type));
	memset(tmpvar_contract_type, -1, sizeof(tmpvar_contract_type));
	memset(tmpvar_lifetime_noted, 0, sizeof(tmpvar_lifetime_noted));
	{
		struct hir_local *local = hir_func->val.func.local;
		while (local != NULL) {
			if ((lir_optimize_level >= 1 ||
			     hir_func->val.func.is_fast) &&
			    !local->is_parameter &&
			    (local->declared_type == NOCT_VALUE_INT ||
			     local->declared_type == NOCT_VALUE_LONG ||
			     local->declared_type == NOCT_VALUE_FLOAT ||
			     local->declared_type == NOCT_VALUE_DOUBLE))
				tmpvar_contract_type[local->index] =
					(int8_t)local->declared_type;
			lir_note_tmpvar_type(local->index, local->proven_type);
			local = local->next;
		}
	}

	/* Initialize the relocation table. */
	loc_count = 0;

	/* __fast keeps ordinary checked entry semantics without optimization. */
	if (lir_optimize_level >= 1 || hir_func->val.func.is_fast) {
		uint32_t k;
		for (k = 0; k < hir_func->val.func.param_count; k++) {
			int check_type;

			if (hir_func->val.func.param_type[k] < 0)
				continue;
			check_type = lir_typecheck_code(
				hir_func->val.func.param_type[k],
				hir_func->val.func.param_packed_type[k],
				hir_func->val.func.param_restricted[k], false, false);
			if (!lir_put_opcode(OP_CHECKTYPE))
				return false;
			if (!lir_put_tmpvar((uint16_t)k))
				return false;
			if (!lir_put_imm8((uint8_t)check_type))
				return false;
		}
	}

	cur_block = hir_func->val.func.inner;
	while (cur_block != NULL) {
		if (!lir_visit_block(cur_block))
			return false;
		if (cur_block->stop) {
			assert(cur_block->succ->type == HIR_BLOCK_END);
			cur_block->succ->addr = (uint32_t)bytecode_top;
			break;
		}
		cur_block = cur_block->succ;
	}
	{
		uint32_t prefix;

		if (!lir_prepend_tmpvar_types(&prefix))
			return false;
		patch_block_address(prefix);
	}

	/* Make an lir_func. */
	*lir_func = noct_calloc(1, sizeof(struct lir_func));
	if (*lir_func == NULL) {
		lir_out_of_memory();
		return false;
	}

	(*lir_func)->is_fast = false;
	(*lir_func)->fast_info = NULL;
#if defined(NOCT_USE_OPTIMIZER)
	(*lir_func)->is_fast = hir_func->val.func.fast_optimized;
	if ((*lir_func)->is_fast) {
		if (hir_func->val.func.fast_info == NULL) {
			lir_fatal(N_TR("Invalid __fast function signature."));
			goto fail_func;
		}

		(*lir_func)->fast_info =
			fast_info_clone(hir_func->val.func.fast_info);
		if ((*lir_func)->fast_info == NULL) {
			lir_out_of_memory();
			goto fail_func;
		}
	}
#endif

	/* Copy the function name. */
	(*lir_func)->func_name = noct_strdup(hir_func->val.func.name);
	if ((*lir_func)->func_name == NULL) {
		lir_out_of_memory();
		goto fail_func;
	}

	/* Copy the parameter names.  */
	(*lir_func)->param_count = hir_func->val.func.param_count;
	for (i = 0; i < LIR_PARAM_SIZE; i++) {
		(*lir_func)->param_type[i] = -1;
		(*lir_func)->param_packed_type[i] = -1;
		(*lir_func)->param_restricted[i] = false;
	}
	for (i = 0; i < hir_func->val.func.param_count; i++) {
		(*lir_func)->param_type[i] = hir_func->val.func.param_type[i];
		(*lir_func)->param_packed_type[i] =
			hir_func->val.func.param_packed_type[i];
		(*lir_func)->param_restricted[i] =
			hir_func->val.func.param_restricted[i];
	}
	(*lir_func)->return_type = hir_func->val.func.return_type;
	(*lir_func)->return_packed_type =
		hir_func->val.func.return_packed_type;
	(*lir_func)->return_type_checked =
		(lir_optimize_level >= 2 || hir_func->val.func.is_fast) &&
		(*lir_func)->return_type >= 0;
	for (i = 0; i < hir_func->val.func.param_count; i++) {
		(*lir_func)->param_name[i] = noct_strdup(hir_func->val.func.param_name[i]);
		if ((*lir_func)->param_name[i] == NULL) {
			lir_out_of_memory();
			goto fail_func;
		}
	}

	/* Copy the bytecode. */
	if (bytecode_top != 0) {
		(*lir_func)->bytecode = noct_malloc((size_t)bytecode_top);
		if ((*lir_func)->bytecode == NULL) {
			lir_out_of_memory();
			goto fail_func;
		}
		memcpy((*lir_func)->bytecode, bytecode, (size_t)bytecode_top);
	} else {
		(*lir_func)->bytecode = NULL;
	}
	(*lir_func)->bytecode_size = bytecode_top;
	noct_free(bytecode);
	bytecode = NULL;

	/* Copy the file name. */
	(*lir_func)->file_name = noct_strdup(hir_func->val.func.file_name);
	if ((*lir_func)->file_name == NULL) {
		lir_out_of_memory();
		goto fail_func;
	}

	(*lir_func)->tmpvar_size = tmpvar_count + 1;
	(*lir_func)->has_vector_ops = has_vector_ops;
	(*lir_func)->has_fma_ops = has_fma_ops;

#ifdef DEBUG_DUMP_LIR
	lir_dump(*lir_func);
#endif

	/* Typed-op observability (design 07 D-TOP11). */
	if (getenv("NOCT_TYPED_DEBUG") != NULL) {
		fprintf(stderr,
			"TYPED: %s: emitted=%d (int=%d float=%d) sites_generic=%d checked_div=%d\n",
			hir_func->val.func.name != NULL ?
			hir_func->val.func.name : "?",
			typed_emit_int_count + typed_emit_float_count,
			typed_emit_int_count, typed_emit_float_count,
			typed_generic_count, typed_emit_checked_div_count);
	}
	if (getenv("NOCT_TAGSTORE_DEBUG") != NULL) {
		uint32_t fixed = 0;
		uint32_t dynamic = 0;
		uint32_t ti;

		for (ti = 0; ti < tmpvar_count; ti++) {
			if (tmpvar_type_state[ti] == LIR_TMP_TYPE_FIXED)
				fixed++;
			else
				dynamic++;
		}
		fprintf(stderr,
			"TAGSTORE: %s: fixed_slots=%u dynamic_slots=%u materialize_ops=%d\n",
			hir_func->val.func.name != NULL ?
			hir_func->val.func.name : "?",
			fixed, dynamic, materialize_emit_count);
	}

	return true;

fail_func:
	lir_free_func_body(*lir_func);
	*lir_func = NULL;

	return false;
}

/* Count the number of the local variables in a func. */
static uint32_t
lir_count_local(
	struct hir_block *func)
{
	struct hir_local *local;
	uint32_t count;

	count = 0;
	local = func->val.func.local;
	while (local != NULL) {
		count++;
		local = local->next;
	}

	return count;
}

static bool
lir_visit_block(
	struct hir_block *block)
{
	assert(block != NULL);

#ifdef DEBUG_BLOCK_ORDER
	printf("LIR-pass: BLOCK %d\n", block->id);
#endif

	lir_error_line = block->line;

	switch (block->type) {
	case HIR_BLOCK_BASIC:
		if (!lir_visit_basic_block(block))
			return false;
		break;
	case HIR_BLOCK_IF:
		if (!lir_visit_if_block(block))
			return false;
		break;
	case HIR_BLOCK_FOR:
		if (!lir_visit_for_block(block))
			return false;
		break;
	case HIR_BLOCK_WHILE:
		if (!lir_visit_while_block(block))
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

static bool
lir_visit_basic_block(
	struct hir_block *block)
{
	struct hir_stmt *stmt;

	assert(block != NULL);
	assert(block->type == HIR_BLOCK_BASIC);
	assert(block->parent != NULL);

	/* Store the block address. */
	block->addr = (uint32_t)bytecode_top;

	/* Put a line number. */
	if (lir_lineinfo) {
		if (!lir_put_opcode(OP_LINEINFO))
			return false;
		if (!lir_put_imm32((uint32_t)block->line))
			return false;
	}

	/* Visit statements. */
	stmt = block->val.basic.stmt_list;
	while (stmt != NULL) {
		/* Visit a statement. */
		if (!lir_visit_stmt(block, stmt))
			return false;
		stmt = stmt->next;
	}

	/* A source-level fallthrough has no return operand to prove.  Check
	   the existing implicit return slot only on that edge. */
	if (block->succ != NULL && block->succ->type == HIR_BLOCK_END &&
	    !block->is_return_edge) {
		if (!lir_emit_return_check(block))
			return false;
	}

	/*
	 * If the block is the tail of the siblings, it needs an explicit
	 * jump unless its successor is the block control would reach by
	 * falling through anyway.
	 *
	 * Falling through is correct only in two cases:
	 *
	 *  - The block ends a loop body and continues that same loop:
	 *    the loop emitter appends the incrementer and the back edge
	 *    right after this block.
	 *
	 *  - The block ends the function body and goes to the end block,
	 *    which is emitted next.
	 *
	 * Everything else (a break, a return from inside a loop, or a
	 * continue targeting an outer loop) has to jump. Treating those
	 * as fall-through is what used to make a "return" at the tail of
	 * a loop body run the rest of the loop instead of returning.
	 */
	if (block->stop) {
		struct hir_block *loop;
		struct hir_block *parent;
		bool falls_through;

		parent = block->parent;
		falls_through = false;
		if (parent->type == HIR_BLOCK_FOR) {
			falls_through = block->succ == parent->val.for_.inner;
		} else if (parent->type == HIR_BLOCK_WHILE) {
			falls_through = block->succ == parent->val.while_.inner;
		} else if (parent->type == HIR_BLOCK_FUNC) {
			falls_through = block->succ != NULL &&
					block->succ->type == HIR_BLOCK_END;
		}

		if (!falls_through) {
			/* Check if succ is a loop head. */
			if (lir_check_succ_loop_head(block, &loop)) {
				/* Put a safepoint. */
				if (!lir_put_opcode(OP_SAFEPOINT))
					return false;

				/* Continue edge. */
				if (!lir_put_opcode(OP_JMP))
					return false;
				if (!lir_put_continue_addr(loop))
					return false;
			} else {
				/* Break or return edge. */
				if (!lir_put_opcode(OP_JMP))
					return false;
				if (!lir_put_branch_addr(block->succ))
					return false;
			}
		}
	}

	return true;
}

/* Check if succ is a loop head. (Detects a continue edge) */
static bool
lir_check_succ_loop_head(
	struct hir_block *block,
	struct hir_block **loop)
{
	struct hir_block *b;

	b = block;
	while (b != NULL) {
		if (b->type == HIR_BLOCK_FOR) {
			if (b->val.for_.inner == block->succ) {
				*loop = b;
				return true;
			}
		}
		if (b->type == HIR_BLOCK_WHILE) {
			if (b->val.while_.inner == block->succ) {
				*loop = b;
				return true;
			}
		}
		b = b->parent;
	}
	return false;
}

static bool
lir_visit_if_block(
	struct hir_block *block)
{
	int cond_tmpvar;
	bool is_else;
	struct hir_block *b;

	assert(block != NULL);
	assert(block->type == HIR_BLOCK_IF);

	/* Store the block address. */
	block->addr = (uint32_t)bytecode_top;

	/* Put a line number. */
	if (lir_lineinfo) {
		if (!lir_put_opcode(OP_LINEINFO))
			return false;
		if (!lir_put_imm32((uint32_t)block->line))
			return false;
	}

	/* Is an else-block? */
	if (block->val.if_.cond == NULL) {
		is_else = true;
	} else {
		is_else = false;
	}

	/* If this is not an else-block. */
	if (!is_else) {
		/* Skip this block if the condition is not met. */
		if (!lir_increment_tmpvar(&cond_tmpvar))
			return false;
		if (!lir_visit_expr(cond_tmpvar, block->val.if_.cond, block))
			return false;
		if (!lir_put_materialize_type(
			    cond_tmpvar,
			    lir_expr_proven_type(block->val.if_.cond, block)))
			return false;
		if (!lir_put_opcode(OP_JMPIFFALSE))
			return false;
		if (!lir_put_tmpvar((uint16_t)cond_tmpvar))
			return false;
		if (block->val.if_.chain_next != NULL) {
			/* Jump to a chaining else-block. */
			if (!lir_put_branch_addr(block->val.if_.chain_next))
				return false;
		} else {
			/* Jump to a first non-if block. */
			if (block->succ != NULL) {
				/* if-block */
				if (!lir_put_branch_addr(block->succ))
					return false;
			} else {
				/* elif-block */
				if (!lir_put_branch_addr(block->parent->succ))
					return false;
			}
		}
		lir_decrement_tmpvar(cond_tmpvar);
	}

	/* Visit an inner block. */
	b = block->val.if_.inner;
	while (b != NULL) {
		if (!lir_visit_block(b))
			return false;
		if (b->stop)
			break;
		b = b->succ;
	}

	/* If this is an if-block or an else-if block. */
	if (!is_else) {
		/* Jump to a first non-if block. */
		if (!lir_put_opcode(OP_JMP))
			return false;
		if (block->succ != NULL) {
			/* if-block */
			if (!lir_put_branch_addr(block->succ))
				return false;
		} else {
			/* elif-block */
			if (!lir_put_branch_addr(block->parent->succ))
				return false;
		}
	}

	/* Visit a chaining block if exists. */
	if (block->val.if_.chain_next != NULL) {
		if (!lir_visit_block(block->val.if_.chain_next))
			return false;
	}

	return true;
}

static bool
lir_visit_for_block(
	struct hir_block *block)
{
	assert(block != NULL);
	assert(block->type == HIR_BLOCK_FOR);

	/* Dispatch by type. */
	if (block->val.for_.is_vector) {
		/* This is a vectorized strip loop (design 06). */
		if (!lir_visit_vfor_block(block))
			return false;
	} else if (block->val.for_.is_ranged) {
		/* This is a ranged-for loop. */
		if (!lir_visit_for_range_block(block))
			return false;
	} else if (block->val.for_.key_symbol != NULL) {
		/* This is a for-each-key-and-value loop. */
		if (!lir_visit_for_kv_block(block))
			return false;
	} else {
		/* This is a for-each-value loop. */
		if (!lir_visit_for_v_block(block))
			return false;
	}

	return true;
}

/*
 * Vectorized strip-loop lowering (docs/design/06-simd.md).
 *
 * The body is the eligible vector grammar (checked by
 * hir_opt_simd.c): a single basic block of "temp = expr" and
 * "PSTORE32/PSTOREF32(sb, i) = expr" statements over homogeneous
 * int32 or float32 constants, invariant locals and temp locals.
 *
 * vreg plan (MUST mirror hir_opt_simd.c's budget computation):
 *   [0 .. nconst)               one per distinct int constant
 *   [nconst .. +ninv)           one per invariant local
 *   [.. +ntemp)                 one per temp local
 *   [.. 8)                      LIFO expression stack
 * TERM operands are consumed directly from their home vregs; only
 * non-term subtree results occupy stack slots.
 */

/* Bytecode owns 16 logical vector registers.  A JIT whose native mapping is
 * smaller consumes OP_VINDEX_HINT before the first vector instruction and
 * selects its direct-scalar vector tier for the whole region. */
#define VFOR_VREG_MAX		16
#define VFOR_MAX_CONSTS		8
#define VFOR_MAX_LOCALS		8
#define VFOR_CACHE_CANDIDATE_MAX 32
#define VFOR_CACHE_MAX		4

struct vfor_cache_entry {
	struct hir_expr *expr;
	int count;
	int size;
	int reg;
	bool emitted;
};

struct vfor_plan {
	struct hir_block *loop;
	const char *counter;
	int counter_tmpvar;

	uint32_t consts[VFOR_MAX_CONSTS];	/* int value or float bits */
	uint8_t const_type[VFOR_MAX_CONSTS];
	int const_count;
	const char *inv[VFOR_MAX_LOCALS];
	uint8_t inv_type[VFOR_MAX_LOCALS];
	int inv_count;
	const char *temp[VFOR_MAX_LOCALS];
	uint8_t temp_type[VFOR_MAX_LOCALS];
	bool temp_induction[VFOR_MAX_LOCALS];
	int temp_count;
	int stack_base;
	int required_vregs;
	struct vfor_cache_entry cache_candidate[VFOR_CACHE_CANDIDATE_MAX];
	int cache_candidate_count;
	struct vfor_cache_entry cache[VFOR_CACHE_MAX];
	int cache_count;
	bool is_float;
	bool force_scalar;
	uint8_t native_requirements;
};

static struct hir_expr *lir_vfor_strip_par(struct hir_expr *e);
static int lir_vfor_term_vreg(struct vfor_plan *plan, struct hir_expr *e);
static bool lir_vfor_expr_reads(struct hir_expr *e, const char *sym);

static int
lir_vfor_local_type(struct vfor_plan *plan, const char *sym)
{
	struct hir_block *b = plan->loop;
	struct hir_local *local;
	while (b->parent != NULL)
		b = b->parent;
	if (b->type != HIR_BLOCK_FUNC)
		return -1;
	local = b->val.func.local;
	while (local != NULL) {
		if (strcmp(local->symbol, sym) == 0)
			return local->proven_type;
		local = local->next;
	}
	return -1;
}

static int
lir_vfor_expr_type(struct vfor_plan *plan, struct hir_expr *e)
{
	int a, b;
	e = lir_vfor_strip_par(e);
	switch (e->type) {
	case HIR_EXPR_TERM:
	{
		int i;
		if (e->val.term.term->type == HIR_TERM_INT)
			return NOCT_VALUE_INT;
		if (e->val.term.term->type == HIR_TERM_FLOAT)
			return NOCT_VALUE_FLOAT;
		if (e->val.term.term->type == HIR_TERM_SYMBOL) {
			for (i = 0; i < plan->inv_count; i++)
				if (strcmp(plan->inv[i],
					   e->val.term.term->val.symbol) == 0)
					return plan->inv_type[i];
			for (i = 0; i < plan->temp_count; i++)
				if (strcmp(plan->temp[i],
					   e->val.term.term->val.symbol) == 0)
					return plan->temp_type[i];
			return lir_vfor_local_type(plan,
				e->val.term.term->val.symbol);
		}
		return -1;
	}
	case HIR_EXPR_PLOAD32:
	case HIR_EXPR_PGATHER32: return NOCT_VALUE_INT;
	case HIR_EXPR_PLOADF32: return NOCT_VALUE_FLOAT;
	case HIR_EXPR_VINDUCTF32: return NOCT_VALUE_FLOAT;
	case HIR_EXPR_CALL:
		return hir_get_intrinsic_call(e) == HIR_INTRINSIC_INT_FROM ?
			NOCT_VALUE_INT : NOCT_VALUE_FLOAT;
	case HIR_EXPR_SELECT:
		return lir_vfor_expr_type(plan, e->val.select.if_true);
	case HIR_EXPR_PLUS:
	case HIR_EXPR_MINUS:
	case HIR_EXPR_MUL:
	case HIR_EXPR_DIV:
		a = lir_vfor_expr_type(plan, e->val.binary.expr[0]);
		b = lir_vfor_expr_type(plan, e->val.binary.expr[1]);
		return a == NOCT_VALUE_FLOAT || b == NOCT_VALUE_FLOAT ?
			NOCT_VALUE_FLOAT : NOCT_VALUE_INT;
	default:
		return NOCT_VALUE_INT;
	}
}

static struct hir_expr *
lir_vfor_strip_par(struct hir_expr *e)
{
	while (e->type == HIR_EXPR_PAR)
		e = e->val.unary.expr;
	return e;
}

/* Structural equality for the pure vector-expression grammar. */
static bool
lir_vfor_expr_equal(struct hir_expr *a, struct hir_expr *b)
{
	a = lir_vfor_strip_par(a);
	b = lir_vfor_strip_par(b);
	if (a->type != b->type)
		return false;
	switch (a->type) {
	case HIR_EXPR_TERM:
		if (a->val.term.term->type != b->val.term.term->type)
			return false;
		switch (a->val.term.term->type) {
		case HIR_TERM_INT:
			return a->val.term.term->val.i == b->val.term.term->val.i;
		case HIR_TERM_FLOAT:
			return memcmp(&a->val.term.term->val.f,
				      &b->val.term.term->val.f,
				      sizeof(float)) == 0;
		case HIR_TERM_SYMBOL:
			return strcmp(a->val.term.term->val.symbol,
				      b->val.term.term->val.symbol) == 0;
		default:
			return false;
		}
	case HIR_EXPR_CALL:
		return hir_get_intrinsic_call(a) == hir_get_intrinsic_call(b) &&
		       a->val.call.arg_count == 1 && b->val.call.arg_count == 1 &&
		       lir_vfor_expr_equal(a->val.call.arg[0], b->val.call.arg[0]);
	case HIR_EXPR_SELECT:
		return lir_vfor_expr_equal(a->val.select.cond,
					   b->val.select.cond) &&
		       lir_vfor_expr_equal(a->val.select.if_true,
					   b->val.select.if_true) &&
		       lir_vfor_expr_equal(a->val.select.if_false,
					   b->val.select.if_false);
	case HIR_EXPR_PGATHER32:
		return lir_vfor_expr_equal(a->val.gather.base,
					    b->val.gather.base) &&
		       lir_vfor_expr_equal(a->val.gather.length,
					    b->val.gather.length) &&
		       lir_vfor_expr_equal(a->val.gather.index,
					    b->val.gather.index);
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
		return lir_vfor_expr_equal(a->val.binary.expr[0],
					    b->val.binary.expr[0]) &&
		       lir_vfor_expr_equal(a->val.binary.expr[1],
					    b->val.binary.expr[1]);
	default:
		return false;
	}
}

static int
lir_vfor_expr_size(struct hir_expr *e)
{
	e = lir_vfor_strip_par(e);
	switch (e->type) {
	case HIR_EXPR_TERM:
		return 1;
	case HIR_EXPR_CALL:
		return 1 + lir_vfor_expr_size(e->val.call.arg[0]);
	case HIR_EXPR_SELECT:
		return 1 + lir_vfor_expr_size(e->val.select.cond) +
			lir_vfor_expr_size(e->val.select.if_true) +
			lir_vfor_expr_size(e->val.select.if_false);
	case HIR_EXPR_PGATHER32:
		return 1 + lir_vfor_expr_size(e->val.gather.index);
	case HIR_EXPR_VINDUCTF32:
		return 1;
	default:
		return 1 + lir_vfor_expr_size(e->val.binary.expr[0]) +
			lir_vfor_expr_size(e->val.binary.expr[1]);
	}
}

static bool
lir_vfor_cache_eligible(struct vfor_plan *plan, struct hir_expr *e)
{
	e = lir_vfor_strip_par(e);
	if (e->type == HIR_EXPR_PLOAD32 || e->type == HIR_EXPR_PLOADF32)
		return true;
	if (e->type == HIR_EXPR_CALL)
		return hir_get_intrinsic_call(e) == HIR_INTRINSIC_INT_FROM ||
		       hir_get_intrinsic_call(e) == HIR_INTRINSIC_FLOAT_FROM;
	return (e->type == HIR_EXPR_PLUS || e->type == HIR_EXPR_MINUS ||
		e->type == HIR_EXPR_MUL || e->type == HIR_EXPR_DIV) &&
	       lir_vfor_expr_type(plan, e) == NOCT_VALUE_FLOAT;
}

static bool
lir_vfor_cache_reads_body_temp(struct vfor_plan *plan, struct hir_expr *e)
{
	int i;
	for (i = 0; i < plan->temp_count; i++) {
		/* Caches are materialized at the loop header, before body-local
		 * assignments.  Hoisting any expression that reads such a home is
		 * invalid, not only the visibly stateful induction case. */
		if (lir_vfor_expr_reads(e, plan->temp[i]))
			return true;
	}
	return false;
}

static void
lir_vfor_cache_collect(struct vfor_plan *plan, struct hir_expr *e)
{
	int i;

	e = lir_vfor_strip_par(e);
	if (lir_vfor_cache_eligible(plan, e)) {
		for (i = 0; i < plan->cache_candidate_count; i++) {
			if (lir_vfor_expr_equal(plan->cache_candidate[i].expr, e)) {
				plan->cache_candidate[i].count++;
				break;
			}
		}
		if (i == plan->cache_candidate_count &&
		    i < VFOR_CACHE_CANDIDATE_MAX) {
			plan->cache_candidate[i].expr = e;
			plan->cache_candidate[i].count = 1;
			plan->cache_candidate[i].size = lir_vfor_expr_size(e);
			plan->cache_candidate_count++;
		}
	}
	switch (e->type) {
	case HIR_EXPR_TERM:
		break;
	case HIR_EXPR_CALL:
		lir_vfor_cache_collect(plan, e->val.call.arg[0]);
		break;
	case HIR_EXPR_SELECT:
		lir_vfor_cache_collect(plan, e->val.select.cond);
		lir_vfor_cache_collect(plan, e->val.select.if_true);
		lir_vfor_cache_collect(plan, e->val.select.if_false);
		break;
	case HIR_EXPR_PGATHER32:
		lir_vfor_cache_collect(plan, e->val.gather.index);
		break;
	case HIR_EXPR_VINDUCTF32:
		break;
	default:
		lir_vfor_cache_collect(plan, e->val.binary.expr[0]);
		lir_vfor_cache_collect(plan, e->val.binary.expr[1]);
		break;
	}
}

/* Count occurrences of needle in the same pure-expression walk used by
 * cache collection.  Once a selected parent is materialized, all but one of
 * its repeated evaluations disappear; this lets cache ranking charge that
 * saving to nested candidates instead of selecting both parent and child. */
static int
lir_vfor_expr_occurrences(struct hir_expr *haystack, struct hir_expr *needle)
{
	haystack = lir_vfor_strip_par(haystack);
	needle = lir_vfor_strip_par(needle);
	if (lir_vfor_expr_equal(haystack, needle))
		return 1;
	switch (haystack->type) {
	case HIR_EXPR_TERM:
	case HIR_EXPR_VINDUCTF32:
		return 0;
	case HIR_EXPR_CALL:
		return lir_vfor_expr_occurrences(haystack->val.call.arg[0], needle);
	case HIR_EXPR_SELECT:
		return lir_vfor_expr_occurrences(haystack->val.select.cond, needle) +
		       lir_vfor_expr_occurrences(haystack->val.select.if_true, needle) +
		       lir_vfor_expr_occurrences(haystack->val.select.if_false, needle);
	case HIR_EXPR_PGATHER32:
		return lir_vfor_expr_occurrences(haystack->val.gather.index, needle);
	default:
		return lir_vfor_expr_occurrences(haystack->val.binary.expr[0], needle) +
		       lir_vfor_expr_occurrences(haystack->val.binary.expr[1], needle);
	}
}

static int
lir_vfor_cached_reg(struct vfor_plan *plan, struct hir_expr *e)
{
	int i;
	for (i = 0; i < plan->cache_count; i++) {
		if (plan->cache[i].emitted &&
		    lir_vfor_expr_equal(plan->cache[i].expr, e))
			return plan->cache[i].reg;
	}
	return -1;
}

static int
lir_vfor_physical_reg(struct vfor_plan *plan, int reg)
{
	UNUSED_PARAMETER(plan);
	if (reg >= 0 && reg < VFOR_VREG_MAX)
		return reg;
	return -1;
}

/* Map a TERM to its home vreg (const or local). */
static int
lir_vfor_term_vreg(struct vfor_plan *plan, struct hir_expr *e)
{
	struct hir_term *t = e->val.term.term;
	int i;

	if (t->type == HIR_TERM_INT || t->type == HIR_TERM_FLOAT) {
		uint32_t bits;
		int type = t->type == HIR_TERM_INT ?
			NOCT_VALUE_INT : NOCT_VALUE_FLOAT;
		if (t->type == HIR_TERM_INT)
			bits = (uint32_t)t->val.i;
		else
			memcpy(&bits, &t->val.f, sizeof(bits));
		for (i = 0; i < plan->const_count; i++) {
			if (plan->consts[i] == bits && plan->const_type[i] == type)
				return i;
		}
		return -1;
	}
	if (t->type == HIR_TERM_SYMBOL) {
		for (i = 0; i < plan->inv_count; i++) {
			if (strcmp(plan->inv[i], t->val.symbol) == 0)
				return plan->const_count + i;
		}
		for (i = 0; i < plan->temp_count; i++) {
			if (strcmp(plan->temp[i], t->val.symbol) == 0)
				return plan->const_count + plan->inv_count + i;
		}
	}
	return -1;
}

static int
lir_vfor_value_vreg(struct vfor_plan *plan, struct hir_expr *e)
{
	e = lir_vfor_strip_par(e);
	if (e->type == HIR_EXPR_TERM)
		return lir_vfor_term_vreg(plan, e);
	return lir_vfor_cached_reg(plan, e);
}

static int lir_vfor_scratch_need(struct vfor_plan *plan,
				 struct hir_expr *e);

/* Select A*B+C for the O3-only fused operation.  For two products the
   cheaper product to materialize as C is selected by the scratch model. */
static bool
lir_vfor_fma_parts(struct vfor_plan *plan, struct hir_expr *e,
		   struct hir_expr **a, struct hir_expr **b,
		   struct hir_expr **c)
{
	struct hir_expr *l;
	struct hir_expr *r;
	bool lm, rm;

	e = lir_vfor_strip_par(e);
	if (lir_optimize_level < 3 || e->type != HIR_EXPR_PLUS ||
	    lir_vfor_expr_type(plan, e) != NOCT_VALUE_FLOAT)
		return false;
	l = lir_vfor_strip_par(e->val.binary.expr[0]);
	r = lir_vfor_strip_par(e->val.binary.expr[1]);
	lm = l->type == HIR_EXPR_MUL &&
		lir_vfor_expr_type(plan, l) == NOCT_VALUE_FLOAT;
	rm = r->type == HIR_EXPR_MUL &&
		lir_vfor_expr_type(plan, r) == NOCT_VALUE_FLOAT;
	if (!lm && !rm)
		return false;
	if (lm && rm && lir_vfor_scratch_need(plan, l) <
			lir_vfor_scratch_need(plan, r)) {
		/* Keep the more expensive product as the fused multiplication. */
		*a = lir_vfor_strip_par(r->val.binary.expr[0]);
		*b = lir_vfor_strip_par(r->val.binary.expr[1]);
		*c = l;
	} else if (lm) {
		*a = lir_vfor_strip_par(l->val.binary.expr[0]);
		*b = lir_vfor_strip_par(l->val.binary.expr[1]);
		*c = r;
	} else {
		*a = lir_vfor_strip_par(r->val.binary.expr[0]);
		*b = lir_vfor_strip_par(r->val.binary.expr[1]);
		*c = l;
	}
	return true;
}

/* Scratch slots for simultaneously resident multiplication operands. */
static int
lir_vfor_fma_factor_need(struct vfor_plan *plan,
			 struct hir_expr *a, struct hir_expr *b)
{
	int an, bn, ab, ba;
	bool av, bv;

	av = lir_vfor_value_vreg(plan, a) >= 0;
	bv = lir_vfor_value_vreg(plan, b) >= 0;
	if (av && bv)
		return 0;
	an = lir_vfor_scratch_need(plan, a);
	bn = lir_vfor_scratch_need(plan, b);
	if (av)
		return 1 + bn;
	if (bv)
		return 1 + an;
	ab = 1 + an > 2 + bn ? 1 + an : 2 + bn;
	ba = 1 + bn > 2 + an ? 1 + bn : 2 + an;
	return ab < ba ? ab : ba;
}

/* Number of scratch vregs needed in addition to the destination.  Cached
   values and terms are already resident.  This mirrors lir_vfor_expr(). */
static int
lir_vfor_scratch_need(struct vfor_plan *plan, struct hir_expr *e)
{
	struct hir_expr *l;
	struct hir_expr *r;
	struct hir_expr *fa;
	struct hir_expr *fb;
	struct hir_expr *fc;
	int ln, rn;
	int cn, fn;
	bool lv, rv;

	e = lir_vfor_strip_par(e);
	if (e->type == HIR_EXPR_TERM || lir_vfor_cached_reg(plan, e) >= 0)
		return 0;
	if (lir_vfor_fma_parts(plan, e, &fa, &fb, &fc)) {
		cn = lir_vfor_scratch_need(plan, fc);
		fn = lir_vfor_fma_factor_need(plan, fa, fb);
		return cn > fn ? cn : fn;
	}
	switch (e->type) {
	case HIR_EXPR_PLOAD32:
	case HIR_EXPR_PLOADF32:
	case HIR_EXPR_VINDUCTF32:
		return 0;
	case HIR_EXPR_PGATHER32:
		return lir_vfor_scratch_need(plan, e->val.gather.index);
	case HIR_EXPR_CALL:
		return lir_vfor_scratch_need(plan, e->val.call.arg[0]);
	case HIR_EXPR_SELECT:
	{
		int cn = lir_vfor_scratch_need(plan, e->val.select.cond);
		int tn = 1 + lir_vfor_scratch_need(plan,
						 e->val.select.if_true);
		int fn2 = 2 + lir_vfor_scratch_need(plan,
						  e->val.select.if_false);
		int need = cn > tn ? cn : tn;
		return need > fn2 ? need : fn2;
	}
	case HIR_EXPR_LT:
	case HIR_EXPR_LTE:
	case HIR_EXPR_GT:
	case HIR_EXPR_GTE:
	case HIR_EXPR_EQ:
	case HIR_EXPR_NEQ:
		ln = lir_vfor_scratch_need(plan, e->val.binary.expr[0]);
		rn = 1 + lir_vfor_scratch_need(plan, e->val.binary.expr[1]);
		return ln > rn ? ln : rn;
	case HIR_EXPR_SHL:
	case HIR_EXPR_SHR:
		return lir_vfor_scratch_need(plan, e->val.binary.expr[0]);
	default:
		break;
	}
	l = lir_vfor_strip_par(e->val.binary.expr[0]);
	r = lir_vfor_strip_par(e->val.binary.expr[1]);
	lv = lir_vfor_value_vreg(plan, l) >= 0;
	rv = lir_vfor_value_vreg(plan, r) >= 0;
	ln = lir_vfor_scratch_need(plan, l);
	rn = lir_vfor_scratch_need(plan, r);
	if (lv && rv)
		return 0;
	if (!lv && rv)
		return ln;
	if (lv && !rv) {
		if (e->type != HIR_EXPR_MINUS && e->type != HIR_EXPR_DIV)
			return rn;
		return 1 + rn;
	}
	return ln > 1 + rn ? ln : 1 + rn;
}

static bool
lir_vfor_plan_fits(struct vfor_plan *plan, struct hir_block *block,
		   int home_count)
{
	struct hir_stmt *stmt;
	int i;
	int need;
	int peak;

	plan->stack_base = home_count + plan->cache_count;
	if (plan->stack_base > VFOR_VREG_MAX)
		return false;
	peak = plan->stack_base;
	for (i = 0; i < plan->cache_count; i++) {
		plan->cache[i].reg = home_count + i;
		plan->cache[i].emitted = false;
	}
	/* A cache may consume only caches materialized before it. */
	for (i = 0; i < plan->cache_count; i++) {
		need = lir_vfor_scratch_need(plan, plan->cache[i].expr);
		if (need > 0 && plan->stack_base + need > VFOR_VREG_MAX)
			return false;
		if (plan->stack_base + need > peak)
			peak = plan->stack_base + need;
		plan->cache[i].emitted = true;
	}
	for (stmt = block->val.for_.inner->val.basic.stmt_list;
	     stmt != NULL; stmt = stmt->next) {
		struct hir_expr *rhs = lir_vfor_strip_par(stmt->rhs);
		need = lir_vfor_scratch_need(plan, rhs);
		if (stmt->lhs->type == HIR_EXPR_TERM) {
			const char *sym = stmt->lhs->val.term.term->val.symbol;
			if (lir_vfor_expr_reads(stmt->rhs, sym))
				need++;
			if (need > 0 && plan->stack_base + need >
			    VFOR_VREG_MAX)
				return false;
			if (plan->stack_base + need > peak)
				peak = plan->stack_base + need;
		} else if (rhs->type != HIR_EXPR_TERM) {
			/* The store value itself occupies stack_base. */
			if (plan->stack_base + 1 + need > VFOR_VREG_MAX)
				return false;
			if (plan->stack_base + 1 + need > peak)
				peak = plan->stack_base + 1 + need;
			if (stmt->lhs->type == HIR_EXPR_PMASKSTORE32) {
				int mn = lir_vfor_scratch_need(plan,
					stmt->lhs->val.mask_store.mask);
				/* value in stack_base, mask in stack_base+1 */
				if (plan->stack_base + 2 + mn > VFOR_VREG_MAX)
					return false;
				if (plan->stack_base + 2 + mn > peak)
					peak = plan->stack_base + 2 + mn;
			}
		}
	}
	for (i = 0; i < plan->cache_count; i++)
		plan->cache[i].emitted = false;
	plan->required_vregs = peak > 0 ? peak : 1;
	return true;
}

/* Plan collection walk (mirror of hir_opt_simd.c's collection). */
static bool
lir_vfor_collect(struct vfor_plan *plan, struct hir_expr *e)
{
	int i;

	switch (e->type) {
	case HIR_EXPR_TERM:
		if (e->val.term.term->type == HIR_TERM_INT ||
		    e->val.term.term->type == HIR_TERM_FLOAT) {
			uint32_t v;
			int type = e->val.term.term->type == HIR_TERM_FLOAT ?
				NOCT_VALUE_FLOAT : NOCT_VALUE_INT;
			if (type == NOCT_VALUE_FLOAT)
				memcpy(&v, &e->val.term.term->val.f, sizeof(v));
			else
				v = (uint32_t)e->val.term.term->val.i;
			for (i = 0; i < plan->const_count; i++) {
				if (plan->consts[i] == v &&
				    plan->const_type[i] == type)
					return true;
			}
			if (plan->const_count >= VFOR_MAX_CONSTS)
				return false;
			plan->consts[plan->const_count] = v;
			plan->const_type[plan->const_count++] = (uint8_t)type;
			return true;
		}
		if (e->val.term.term->type == HIR_TERM_SYMBOL) {
			const char *sym = e->val.term.term->val.symbol;
			for (i = 0; i < plan->temp_count; i++) {
				if (strcmp(plan->temp[i], sym) == 0)
					return true;
			}
			for (i = 0; i < plan->inv_count; i++) {
				if (strcmp(plan->inv[i], sym) == 0)
					return true;
			}
			if (plan->inv_count >= VFOR_MAX_LOCALS)
				return false;
			plan->inv[plan->inv_count] = sym;
			plan->inv_type[plan->inv_count++] =
				(uint8_t)lir_vfor_local_type(plan, sym);
			return true;
		}
		return false;
	case HIR_EXPR_PAR:
		return lir_vfor_collect(plan, e->val.unary.expr);
	case HIR_EXPR_CALL:
		return e->val.call.arg_count == 1 &&
		       lir_vfor_collect(plan, e->val.call.arg[0]);
	case HIR_EXPR_SELECT:
		return lir_vfor_collect(plan, e->val.select.cond) &&
		       lir_vfor_collect(plan, e->val.select.if_true) &&
		       lir_vfor_collect(plan, e->val.select.if_false);
	case HIR_EXPR_PLOAD32:
	case HIR_EXPR_PLOADF32:
		/* Base local + bare-counter index: no vreg operands. */
		return true;
	case HIR_EXPR_PGATHER32:
		plan->force_scalar = true;
		plan->native_requirements |= VINDEX_REQUIRE_GATHER;
		return lir_vfor_collect(plan, e->val.gather.index);
	case HIR_EXPR_VINDUCTF32:
		plan->force_scalar = true;
		plan->native_requirements |= VINDEX_REQUIRE_INDUCT;
		/* state/step are scalar tmpvars encoded directly in the opcode */
		return true;
	case HIR_EXPR_SHL:
	case HIR_EXPR_SHR:
		/* The count is an immediate, not a vector operand. */
		return lir_vfor_collect(plan, e->val.binary.expr[0]);
	default:
		if (!lir_vfor_collect(plan, e->val.binary.expr[0]))
			return false;
		return lir_vfor_collect(plan, e->val.binary.expr[1]);
	}
}

/* Emit one vector opcode with mixed u16/imm8 operands. */
static bool
lir_vfor_put3(int op, int a_is_imm8, int a, int b_is_imm8, int b,
	      int c_is_imm8, int c, int c_present)
{
	if (!lir_put_opcode((uint8_t)op))
		return false;
	if (a_is_imm8) {
		if (!lir_put_imm8((uint8_t)a))
			return false;
	} else {
		if (!lir_put_tmpvar((uint16_t)a))
			return false;
	}
	if (b_is_imm8) {
		if (!lir_put_imm8((uint8_t)b))
			return false;
	} else {
		if (!lir_put_tmpvar((uint16_t)b))
			return false;
	}
	if (!c_present)
		return true;
	if (c_is_imm8) {
		if (!lir_put_imm8((uint8_t)c))
			return false;
	} else {
		if (!lir_put_tmpvar((uint16_t)c))
			return false;
	}
	return true;
}

/* Does the expression read the given symbol anywhere? */
static bool
lir_vfor_expr_reads(struct hir_expr *e, const char *sym)
{
	switch (e->type) {
	case HIR_EXPR_TERM:
		return e->val.term.term->type == HIR_TERM_SYMBOL &&
			strcmp(e->val.term.term->val.symbol, sym) == 0;
	case HIR_EXPR_PAR:
		return lir_vfor_expr_reads(e->val.unary.expr, sym);
	case HIR_EXPR_CALL:
	{
		uint32_t i;
		for (i = 0; i < e->val.call.arg_count; i++) {
			if (lir_vfor_expr_reads(e->val.call.arg[i], sym))
				return true;
		}
		return false;
	}
	case HIR_EXPR_SELECT:
		return lir_vfor_expr_reads(e->val.select.cond, sym) ||
		       lir_vfor_expr_reads(e->val.select.if_true, sym) ||
		       lir_vfor_expr_reads(e->val.select.if_false, sym);
	case HIR_EXPR_PGATHER32:
		return lir_vfor_expr_reads(e->val.gather.index, sym);
	case HIR_EXPR_VINDUCTF32:
		return lir_vfor_expr_reads(e->val.binary.expr[1], sym);
	case HIR_EXPR_PLOAD32:
	case HIR_EXPR_PLOADF32:
		return false;
	default:
		return lir_vfor_expr_reads(e->val.binary.expr[0], sym) ||
			lir_vfor_expr_reads(e->val.binary.expr[1], sym);
	}
}

static bool lir_vfor_expr(struct vfor_plan *plan, int dst, int sp,
			  struct hir_expr *e);

static int
lir_vfor_compare_predicate(int hir_type)
{
	switch (hir_type) {
	case HIR_EXPR_EQ:  return VCMP_EQ;
	case HIR_EXPR_NEQ: return VCMP_NE;
	case HIR_EXPR_LT:  return VCMP_LT;
	case HIR_EXPR_LTE: return VCMP_LE;
	case HIR_EXPR_GT:  return VCMP_GT;
	case HIR_EXPR_GTE: return VCMP_GE;
	default:           return -1;
	}
}

/* Recognize the signed clamp shapes produced by transactional if-conversion.
 * Keep this target-independent: the source has signed Noct ints even though
 * the reference C kernel happens to use unsigned pixel intermediates. */
static bool
lir_vfor_select_minmax(struct vfor_plan *plan, struct hir_expr *e,
			int *op, struct hir_expr **x, struct hir_expr **bound)
{
	struct hir_expr *cond;
	struct hir_expr *l;
	struct hir_expr *r;
	struct hir_expr *t;
	struct hir_expr *f;

	if (e->type != HIR_EXPR_SELECT)
		return false;
	cond = lir_vfor_strip_par(e->val.select.cond);
	if (cond->type != HIR_EXPR_LT && cond->type != HIR_EXPR_GT)
		return false;
	l = lir_vfor_strip_par(cond->val.binary.expr[0]);
	r = lir_vfor_strip_par(cond->val.binary.expr[1]);
	t = lir_vfor_strip_par(e->val.select.if_true);
	f = lir_vfor_strip_par(e->val.select.if_false);
	if (lir_vfor_expr_type(plan, l) != NOCT_VALUE_INT ||
	    lir_vfor_expr_type(plan, r) != NOCT_VALUE_INT ||
	    lir_vfor_expr_type(plan, t) != NOCT_VALUE_INT ||
	    lir_vfor_expr_type(plan, f) != NOCT_VALUE_INT)
		return false;

	/* x > C ? C : x, or C < x ? C : x */
	if ((cond->type == HIR_EXPR_GT &&
	     lir_vfor_expr_equal(t, r) && lir_vfor_expr_equal(f, l)) ||
	    (cond->type == HIR_EXPR_LT &&
	     lir_vfor_expr_equal(t, l) && lir_vfor_expr_equal(f, r))) {
		*op = OP_VMINS32X4;
		*x = f;
		*bound = t;
		return true;
	}

	/* x < C ? C : x, or C > x ? C : x */
	if ((cond->type == HIR_EXPR_LT &&
	     lir_vfor_expr_equal(t, r) && lir_vfor_expr_equal(f, l)) ||
	    (cond->type == HIR_EXPR_GT &&
	     lir_vfor_expr_equal(t, l) && lir_vfor_expr_equal(f, r))) {
		*op = OP_VMAXS32X4;
		*x = f;
		*bound = t;
		return true;
	}
	return false;
}

static bool
lir_vfor_emit_fma(struct vfor_plan *plan, int dst, int sp,
		  struct hir_expr *e)
{
	struct hir_expr *a;
	struct hir_expr *b;
	struct hir_expr *c;
	struct hir_expr *tmp;
	int va, vb;
	int an, bn, ab, ba;

	if (!lir_vfor_fma_parts(plan, e, &a, &b, &c))
		return false;
	if (!lir_vfor_expr(plan, dst, sp, c))
		return false;
	va = lir_vfor_value_vreg(plan, a);
	vb = lir_vfor_value_vreg(plan, b);
	if (va < 0 && vb < 0) {
		an = lir_vfor_scratch_need(plan, a);
		bn = lir_vfor_scratch_need(plan, b);
		ab = 1 + an > 2 + bn ? 1 + an : 2 + bn;
		ba = 1 + bn > 2 + an ? 1 + bn : 2 + an;
		if (ba < ab) {
			tmp = a; a = b; b = tmp;
		}
	}
	va = lir_vfor_value_vreg(plan, a);
	if (va < 0) {
		va = lir_vfor_physical_reg(plan, sp);
		if (va < 0) {
			lir_fatal(N_TR("SIMD FMA: vreg stack overflow."));
			return false;
		}
		if (!lir_vfor_expr(plan, va, sp + 1, a))
			return false;
		sp++;
	}
	vb = lir_vfor_value_vreg(plan, b);
	if (vb < 0) {
		vb = lir_vfor_physical_reg(plan, sp);
		if (vb < 0) {
			lir_fatal(N_TR("SIMD FMA: vreg stack overflow."));
			return false;
		}
		if (!lir_vfor_expr(plan, vb, sp + 1, b))
			return false;
	}
	if (!lir_put_opcode(OP_VFMAF32X4) ||
	    !lir_put_imm8((uint8_t)dst) ||
	    !lir_put_imm8((uint8_t)va) ||
	    !lir_put_imm8((uint8_t)vb) ||
	    !lir_put_imm8((uint8_t)dst))
		return false;
	return true;
}

/*
 * Evaluate a vector expression into dst (always a stack slot or a
 * destination the expression provably does not read; the statement
 * lowering guarantees this).  sp = first free stack vreg.  The
 * strategy and its slot-need formula MUST mirror hir_opt_simd.c's
 * simd_check_expr(): non-term left operands (and, for commutative
 * ops, lone non-term right operands) are built in the destination
 * itself; only a second concurrent subtree takes a stack slot.
 */
static bool
lir_vfor_expr(struct vfor_plan *plan, int dst, int sp, struct hir_expr *e)
{
	int op;
	int cached;

	e = lir_vfor_strip_par(e);
	dst = lir_vfor_physical_reg(plan, dst);
	if (dst < 0) {
		lir_fatal(N_TR("SIMD: vreg stack overflow."));
		return false;
	}
	cached = lir_vfor_cached_reg(plan, e);
	if (cached >= 0) {
		if (cached == dst)
			return true;
		return lir_vfor_put3(OP_VMOV128, 1, dst, 1, cached, 0, 0, 0);
	}

	switch (e->type) {
	case HIR_EXPR_TERM:
	{
		int src = lir_vfor_term_vreg(plan, e);
		if (src < 0) {
			lir_fatal(N_TR("SIMD: unplanned term."));
			return false;
		}
		if (src == dst)
			return true;
		return lir_vfor_put3(OP_VMOV128, 1, dst, 1, src, 0, 0, 0);
	}
	case HIR_EXPR_PLOAD32:
	case HIR_EXPR_PLOADF32:
	{
		int base_tmpvar = lir_get_local_index(plan->loop,
			e->val.binary.expr[0]->val.term.term->val.symbol);
		return lir_vfor_put3(e->type == HIR_EXPR_PLOADF32 ?
				     OP_VLOADF32X4 : OP_VLOADI32X4,
				     1, dst,
				     0, base_tmpvar,
				     0, plan->counter_tmpvar, 1);
	}
	case HIR_EXPR_PGATHER32:
	{
		int base = lir_get_local_index(plan->loop,
			e->val.gather.base->val.term.term->val.symbol);
		int plen = lir_get_local_index(plan->loop,
			e->val.gather.length->val.term.term->val.symbol);
		if (!lir_vfor_expr(plan, dst, sp, e->val.gather.index))
			return false;
		return lir_put_opcode(OP_VGATHERI32X4_CHECKED) &&
		       lir_put_imm8((uint8_t)dst) &&
		       lir_put_tmpvar((uint16_t)base) &&
		       lir_put_tmpvar((uint16_t)plen) &&
		       lir_put_imm8((uint8_t)dst);
	}
	case HIR_EXPR_VINDUCTF32:
	{
		int state = lir_get_local_index(plan->loop,
			e->val.binary.expr[0]->val.term.term->val.symbol);
		int step = lir_get_local_index(plan->loop,
			e->val.binary.expr[1]->val.term.term->val.symbol);
		return lir_put_opcode(OP_VINDUCTF32X4) &&
		       lir_put_imm8((uint8_t)dst) &&
		       lir_put_tmpvar((uint16_t)state) &&
		       lir_put_tmpvar((uint16_t)step);
	}
	case HIR_EXPR_CALL:
	{
		struct hir_expr *arg = lir_vfor_strip_par(e->val.call.arg[0]);
		int src;
		src = lir_vfor_value_vreg(plan, arg);
		if (src >= 0) {
		} else {
			if (!lir_vfor_expr(plan, dst, sp, arg))
				return false;
			src = dst;
		}
		op = hir_get_intrinsic_call(e) == HIR_INTRINSIC_FLOAT_FROM ?
			OP_VCVTI32F32X4 : OP_VCVTF32I32X4;
		return lir_vfor_put3(op, 1, dst, 1, src, 0, 0, 0);
	}
	case HIR_EXPR_LT:
	case HIR_EXPR_LTE:
	case HIR_EXPR_GT:
	case HIR_EXPR_GTE:
	case HIR_EXPR_EQ:
	case HIR_EXPR_NEQ:
	{
		struct hir_expr *l = lir_vfor_strip_par(e->val.binary.expr[0]);
		struct hir_expr *r = lir_vfor_strip_par(e->val.binary.expr[1]);
		int va, vb, pred, cmpop;

		va = lir_vfor_value_vreg(plan, l);
		if (va < 0) {
			if (!lir_vfor_expr(plan, dst, sp, l))
				return false;
			va = dst;
		}
		vb = lir_vfor_value_vreg(plan, r);
		if (vb < 0) {
			vb = lir_vfor_physical_reg(plan, sp);
			if (vb < 0) {
				lir_fatal(N_TR("SIMD: vreg stack overflow."));
				return false;
			}
			if (!lir_vfor_expr(plan, vb, sp + 1, r))
				return false;
		}
		pred = lir_vfor_compare_predicate(e->type);
		cmpop = lir_vfor_expr_type(plan, l) == NOCT_VALUE_FLOAT ?
			OP_VCMPF32X4 : OP_VCMPI32X4;
		return lir_put_opcode((uint8_t)cmpop) &&
		       lir_put_imm8((uint8_t)dst) &&
		       lir_put_imm8((uint8_t)va) &&
		       lir_put_imm8((uint8_t)vb) &&
		       lir_put_imm8((uint8_t)pred);
	}
	case HIR_EXPR_SELECT:
	{
		int vm, vt, vf;
		int minmax_op;
		int vx, vb;
		struct hir_expr *minmax_x;
		struct hir_expr *minmax_bound;
		struct hir_expr *t = lir_vfor_strip_par(e->val.select.if_true);
		struct hir_expr *f = lir_vfor_strip_par(e->val.select.if_false);

		if (lir_vfor_select_minmax(plan, e, &minmax_op,
					    &minmax_x, &minmax_bound)) {
			vx = lir_vfor_value_vreg(plan, minmax_x);
			vb = lir_vfor_value_vreg(plan, minmax_bound);
			if (vx < 0) {
				if (!lir_vfor_expr(plan, dst, sp, minmax_x))
					return false;
				vx = dst;
			}
			if (vb < 0) {
				if (vx == dst) {
					vb = lir_vfor_physical_reg(plan, sp);
					if (vb < 0 || !lir_vfor_expr(plan, vb, sp + 1,
								   minmax_bound))
						return false;
				} else {
					/* min/max is commutative: build the complex bound
					 * in dst so legacy two-address lowering is safe. */
					if (!lir_vfor_expr(plan, dst, sp, minmax_bound))
						return false;
					vb = vx;
					vx = dst;
				}
			}
			return lir_vfor_put3(minmax_op, 1, dst, 1, vx, 1, vb, 1);
		}

		if (!lir_vfor_expr(plan, dst, sp, e->val.select.cond))
			return false;
		vm = dst;
		vt = lir_vfor_value_vreg(plan, t);
		if (vt < 0) {
			vt = lir_vfor_physical_reg(plan, sp);
			if (vt < 0 || !lir_vfor_expr(plan, vt, sp + 1, t))
				return false;
		}
		vf = lir_vfor_value_vreg(plan, f);
		if (vf < 0) {
			vf = lir_vfor_physical_reg(plan, sp + 1);
			if (vf < 0 || !lir_vfor_expr(plan, vf, sp + 2, f))
				return false;
		}
		return lir_put_opcode(OP_VSELECT128) &&
		       lir_put_imm8((uint8_t)dst) &&
		       lir_put_imm8((uint8_t)vm) &&
		       lir_put_imm8((uint8_t)vt) &&
		       lir_put_imm8((uint8_t)vf);
	}
	case HIR_EXPR_SHL:
	case HIR_EXPR_SHR:
	{
		struct hir_expr *x = lir_vfor_strip_par(e->val.binary.expr[0]);
		int count = e->val.binary.expr[1]->val.term.term->val.i;
		int src;
		src = lir_vfor_value_vreg(plan, x);
		if (src >= 0) {
		} else {
			if (!lir_vfor_expr(plan, dst, sp, x))
				return false;
			src = dst;
		}
		if (count == 0) {
			if (src == dst)
				return true;
			return lir_vfor_put3(OP_VMOV128, 1, dst, 1, src, 0, 0, 0);
		}
		op = (e->type == HIR_EXPR_SHL) ? OP_VSHLI32X4 : OP_VSHRI32X4;
		return lir_vfor_put3(op, 1, dst, 1, src, 1, count, 1);
	}
	case HIR_EXPR_PLUS:
	case HIR_EXPR_MINUS:
	case HIR_EXPR_MUL:
	case HIR_EXPR_DIV:
	case HIR_EXPR_AND:
	case HIR_EXPR_OR:
	case HIR_EXPR_XOR:
	{
		struct hir_expr *l = lir_vfor_strip_par(e->val.binary.expr[0]);
		struct hir_expr *r = lir_vfor_strip_par(e->val.binary.expr[1]);
		struct hir_expr *fma_a, *fma_b, *fma_c;
		bool commutative = (e->type != HIR_EXPR_MINUS &&
				    e->type != HIR_EXPR_DIV);
		int va, vb;
		bool lvalue, rvalue;

		if (e->type == HIR_EXPR_PLUS && lir_optimize_level >= 3 &&
		    lir_vfor_expr_type(plan, e) == NOCT_VALUE_FLOAT)
			if (lir_vfor_fma_parts(plan, e,
						&fma_a, &fma_b, &fma_c))
				return lir_vfor_emit_fma(plan, dst, sp, e);

		/* A logical right shift by 24 already produces an 8-bit value;
		   the source-level & 0xff is redundant. */
		if (e->type == HIR_EXPR_AND) {
			struct hir_expr *shifted = NULL;
			struct hir_expr *mask = NULL;
			if (l->type == HIR_EXPR_SHR) {
				shifted = l; mask = r;
			} else if (r->type == HIR_EXPR_SHR) {
				shifted = r; mask = l;
			}
			if (shifted != NULL && mask->type == HIR_EXPR_TERM &&
			    mask->val.term.term->type == HIR_TERM_INT &&
			    mask->val.term.term->val.i == 255 &&
			    shifted->val.binary.expr[1]->type == HIR_EXPR_TERM &&
			    shifted->val.binary.expr[1]->val.term.term->type ==
				HIR_TERM_INT &&
			    shifted->val.binary.expr[1]->val.term.term->val.i == 24)
				return lir_vfor_expr(plan, dst, sp, shifted);
		}

		/* Preserve opaque-alpha intent as one portable vector immediate
		   operation.  ARM64 lowers this to one ORR-immediate. */
#if defined(NOCT_ARCH_ARM64) || defined(NOCT_ARCH_X86_64) || \
    defined(NOCT_ARCH_X86)
		if (e->type == HIR_EXPR_OR) {
			struct hir_expr *opaque = NULL;
			struct hir_expr *other = NULL;
			if (l->type == HIR_EXPR_SHL) {
				opaque = l; other = r;
			} else if (r->type == HIR_EXPR_SHL) {
				opaque = r; other = l;
			}
			if (opaque != NULL &&
			    opaque->val.binary.expr[0]->type == HIR_EXPR_TERM &&
			    opaque->val.binary.expr[0]->val.term.term->type ==
				HIR_TERM_INT &&
			    opaque->val.binary.expr[0]->val.term.term->val.i == 255 &&
			    opaque->val.binary.expr[1]->type == HIR_EXPR_TERM &&
			    opaque->val.binary.expr[1]->val.term.term->type ==
				HIR_TERM_INT &&
			    opaque->val.binary.expr[1]->val.term.term->val.i == 24) {
				if (!lir_vfor_expr(plan, dst, sp, other))
					return false;
				if (!lir_put_opcode(OP_VORI32X4I) ||
				    !lir_put_imm8((uint8_t)dst) ||
				    !lir_put_imm8((uint8_t)dst) ||
				    !lir_put_imm8(0xff) || !lir_put_imm8(24))
					return false;
				return true;
			}
		}
#endif

		switch (e->type) {
		case HIR_EXPR_PLUS:  op = lir_vfor_expr_type(plan, e) == NOCT_VALUE_FLOAT ? OP_VADDF32X4 : OP_VADDI32X4; break;
		case HIR_EXPR_MINUS: op = lir_vfor_expr_type(plan, e) == NOCT_VALUE_FLOAT ? OP_VSUBF32X4 : OP_VSUBI32X4; break;
		case HIR_EXPR_MUL:   op = lir_vfor_expr_type(plan, e) == NOCT_VALUE_FLOAT ? OP_VMULF32X4 : OP_VMULI32X4; break;
		case HIR_EXPR_DIV:   op = OP_VDIVF32X4; break;
		case HIR_EXPR_AND:   op = OP_VAND128;   break;
		case HIR_EXPR_OR:    op = OP_VOR128;    break;
		default:             op = OP_VXOR128;   break;
		}

		va = lir_vfor_value_vreg(plan, l);
		vb = lir_vfor_value_vreg(plan, r);
		lvalue = va >= 0;
		rvalue = vb >= 0;
		if (lvalue && rvalue) {
			/* dst is stack or an unread home: never == vb. */
			return lir_vfor_put3(op, 1, dst, 1, va, 1, vb, 1);
		}
		if (!lvalue && rvalue) {
			/* Build the left side in dst, combine in place. */
			if (!lir_vfor_expr(plan, dst, sp, l))
				return false;
			return lir_vfor_put3(op, 1, dst, 1, dst, 1, vb, 1);
		}
		if (lvalue && !rvalue) {
			if (commutative) {
				/* Build the right side in dst. */
				if (!lir_vfor_expr(plan, dst, sp, r))
					return false;
				return lir_vfor_put3(op, 1, dst, 1, dst,
						     1, va, 1);
			}
			/* SUB needs operand order: rhs into a slot. */
			if (lir_vfor_physical_reg(plan, sp) < 0) {
				lir_fatal(N_TR("SIMD: vreg stack overflow."));
				return false;
			}
			if (!lir_vfor_expr(plan, sp, sp + 1, r))
				return false;
			return lir_vfor_put3(op, 1, dst, 1, va, 1,
					     lir_vfor_physical_reg(plan, sp), 1);
		}
		/* Both non-term: left in dst, right in a slot. */
		if (!lir_vfor_expr(plan, dst, sp, l))
			return false;
		if (lir_vfor_physical_reg(plan, sp) < 0) {
			lir_fatal(N_TR("SIMD: vreg stack overflow."));
			return false;
		}
		if (!lir_vfor_expr(plan, sp, sp + 1, r))
			return false;
		return lir_vfor_put3(op, 1, dst, 1, dst, 1,
				     lir_vfor_physical_reg(plan, sp), 1);
	}
	default:
		lir_fatal(N_TR("SIMD: unexpected vector expression."));
		return false;
	}
}

static bool
lir_visit_vfor_block(
	struct hir_block *block)
{
	struct vfor_plan plan;
	uint32_t loop_addr;
	int start_tmpvar, stop_tmpvar, remaining_tmpvar, guard_tmpvar;
	int scratch_tmpvar;
	struct hir_stmt *stmt;
	int i, j, pick, best, best_count, best_score, score, store_count;
	int effective_count, occurrences, selected_loads;
	bool requires_maskstore;
	bool cache_selected[VFOR_CACHE_CANDIDATE_MAX];
	struct vfor_cache_entry cache_swap;

	assert(block->type == HIR_BLOCK_FOR);
	assert(block->val.for_.is_vector);
	assert(block->val.for_.inner != NULL);
	assert(block->val.for_.inner->type == HIR_BLOCK_BASIC);

	block->addr = (uint32_t)bytecode_top;

	memset(&plan, 0, sizeof(plan));
	plan.loop = block;
	plan.is_float = !block->val.for_.typed_int_region;
	plan.counter = block->val.for_.counter_symbol;
	plan.counter_tmpvar = lir_get_local_index(block, plan.counter);
	store_count = 0;
	requires_maskstore = false;
	memset(cache_selected, 0, sizeof(cache_selected));

	/* Collect the plan (mirror of the HIR-side budget check). */
	for (stmt = block->val.for_.inner->val.basic.stmt_list;
	     stmt != NULL; stmt = stmt->next) {
		if (stmt->lhs->type == HIR_EXPR_TERM) {
			const char *sym = stmt->lhs->val.term.term->val.symbol;
			bool have = false;
			for (i = 0; i < plan.temp_count; i++) {
				if (strcmp(plan.temp[i], sym) == 0) {
					have = true;
					break;
				}
			}
			if (!have) {
				if (plan.temp_count >= VFOR_MAX_LOCALS) {
					lir_fatal(N_TR("SIMD: too many temps."));
					return false;
				}
				plan.temp[plan.temp_count] = sym;
				plan.temp_type[plan.temp_count] =
					(uint8_t)lir_vfor_expr_type(&plan, stmt->rhs);
				plan.temp_induction[plan.temp_count] =
					stmt->rhs->type == HIR_EXPR_VINDUCTF32;
				plan.temp_count++;
			}
			if (!lir_vfor_collect(&plan, stmt->rhs))
				return false;
			lir_vfor_cache_collect(&plan, stmt->rhs);
		} else {
			/* PSTORE32/PSTOREF32: value expr only. */
			if (!lir_vfor_collect(&plan, stmt->rhs))
				return false;
			lir_vfor_cache_collect(&plan, stmt->rhs);
			if (stmt->lhs->type == HIR_EXPR_PMASKSTORE32) {
				requires_maskstore = true;
				if (!lir_vfor_collect(&plan,
						      stmt->lhs->val.mask_store.mask))
					return false;
				lir_vfor_cache_collect(&plan,
						stmt->lhs->val.mask_store.mask);
			}
			store_count++;
		}
	}

	/* One-store vector bodies have no intervening memory clobber.  Select
	 * repeated pure DAG nodes by saved emission work instead of source order.
	 * Stable candidate index is the final tie-breaker, keeping bytecode
	 * deterministic. */
	if (store_count == 1) {
		for (pick = 0; pick < VFOR_CACHE_MAX; pick++) {
			best = -1;
			best_count = 0;
			best_score = 0;
			for (i = 0; i < plan.cache_candidate_count; i++) {
				if (cache_selected[i] ||
				    plan.cache_candidate[i].count < 2 ||
				    lir_vfor_cache_reads_body_temp(&plan,
					plan.cache_candidate[i].expr))
					continue;
				effective_count = plan.cache_candidate[i].count;
				for (j = 0; j < plan.cache_count; j++) {
					occurrences = lir_vfor_expr_occurrences(
						plan.cache[j].expr,
						plan.cache_candidate[i].expr);
					if (occurrences > 0)
						effective_count -=
							(plan.cache[j].count - 1) *
							occurrences;
				}
				if (effective_count < 2)
					continue;
				score = (effective_count - 1) *
					plan.cache_candidate[i].size;
				if (score > best_score ||
				    (score == best_score && best >= 0 &&
				     plan.cache_candidate[i].size >
					plan.cache_candidate[best].size)) {
					best = i;
					best_count = effective_count;
					best_score = score;
				}
			}
			if (best < 0)
				break;
			cache_selected[best] = true;
			plan.cache[plan.cache_count++] =
				plan.cache_candidate[best];
			/* Subsequent child scores must subtract only the evaluations
			 * that remained when this parent cache was selected. */
			plan.cache[plan.cache_count - 1].count = best_count;
		}
	}
	i = plan.const_count + plan.inv_count + plan.temp_count;
	while (!lir_vfor_plan_fits(&plan, block, i) && plan.cache_count > 0)
		plan.cache_count--;
	if (!lir_vfor_plan_fits(&plan, block, i)) {
		lir_fatal(N_TR("SIMD: vreg budget exceeded."));
		return false;
	}
	/* Materialization must be child-before-parent even though selection was
	 * benefit ordered.  Expression size is a deterministic topological proxy
	 * for the pure tree grammar. */
	for (i = 1; i < plan.cache_count; i++) {
		for (j = i; j > 0 && plan.cache[j - 1].size > plan.cache[j].size;
		     j--) {
			cache_swap = plan.cache[j - 1];
			plan.cache[j - 1] = plan.cache[j];
			plan.cache[j] = cache_swap;
		}
	}
	if (!lir_vfor_plan_fits(&plan, block,
		plan.const_count + plan.inv_count + plan.temp_count)) {
		lir_fatal(N_TR("SIMD: ranked cache dependency plan exceeded vreg budget."));
		return false;
	}
	/* Masked stores no longer inflate the architecture-neutral logical
	 * register requirement.  Backends decide whether the whole region has a
	 * native masked-store tier before emitting its first vector operation. */
	if (getenv("NOCT_LIR_VFOR_DEBUG") != NULL) {
		selected_loads = 0;
		for (i = 0; i < plan.cache_count; i++) {
			struct hir_expr *ce = lir_vfor_strip_par(plan.cache[i].expr);
			if (ce->type == HIR_EXPR_PLOAD32 ||
			    ce->type == HIR_EXPR_PLOADF32)
				selected_loads++;
		}
		fprintf(stderr,
			"noct-lir-vfor: max=%d required=%d homes=%d candidates=%d caches=%d loads=%d stack=%d\n",
			VFOR_VREG_MAX, plan.required_vregs, i, plan.cache_candidate_count,
			plan.cache_count, selected_loads, plan.stack_base);
	}

	/* Evaluate start/stop once. */
	if (!lir_increment_tmpvar(&start_tmpvar))
		return false;
	if (!lir_visit_expr(start_tmpvar, block->val.for_.start, block))
		return false;
	if (!lir_increment_tmpvar(&stop_tmpvar))
		return false;
	if (!lir_visit_expr(stop_tmpvar, block->val.for_.stop, block))
		return false;

	/* Empty-range skip (mirrors the scalar for-range; dead in
	   practice because the strip guard proved lo < mid). */
	if (!lir_increment_tmpvar(&guard_tmpvar))
		return false;
	if (!lir_put_opcode(OP_GTE))
		return false;
	if (!lir_put_tmpvar((uint16_t)guard_tmpvar))
		return false;
	if (!lir_put_tmpvar((uint16_t)start_tmpvar))
		return false;
	if (!lir_put_tmpvar((uint16_t)stop_tmpvar))
		return false;
	if (!lir_put_opcode(OP_JMPIFTRUE))
		return false;
	if (!lir_put_tmpvar((uint16_t)guard_tmpvar))
		return false;
	if (!lir_put_branch_addr(block->succ))
		return false;
	lir_decrement_tmpvar(guard_tmpvar);

	/* counter = start */
	if (!lir_put_opcode(OP_ASSIGN))
		return false;
	if (!lir_put_tmpvar((uint16_t)plan.counter_tmpvar))
		return false;
	if (!lir_put_tmpvar((uint16_t)start_tmpvar))
		return false;

	/* remaining = stop - start.  The strip-loop guard above proves a
	   positive multiple of four, so the countdown latch is sufficient. */
	if (!lir_increment_tmpvar(&remaining_tmpvar))
		return false;
	if (!lir_put_opcode(OP_ISUB))
		return false;
	if (!lir_put_tmpvar((uint16_t)remaining_tmpvar))
		return false;
	if (!lir_put_tmpvar((uint16_t)stop_tmpvar))
		return false;
	if (!lir_put_tmpvar((uint16_t)start_tmpvar))
		return false;

	/* Declare the logical-register requirement before the first vector
	 * opcode.  flags=0 makes this an allocation declaration only; the
	 * recurrent-loop index hint is emitted after the splat preheader. */
	if (!lir_put_opcode(OP_VINDEX_HINT))
		return false;
	if (!lir_put_tmpvar((uint16_t)plan.counter_tmpvar) ||
	    !lir_put_tmpvar((uint16_t)stop_tmpvar) ||
	    !lir_put_tmpvar((uint16_t)remaining_tmpvar) ||
	    !lir_put_imm8((uint8_t)plan.required_vregs) || !lir_put_imm8(4) ||
	    !lir_put_imm8((plan.force_scalar ? VINDEX_FORCE_SCALAR : 0) |
			  plan.native_requirements |
			  (requires_maskstore ? VINDEX_REQUIRE_MASKSTORE : 0)))
		return false;

	/*
	 * Preheader: splat constants and invariant locals.  The allocation
	 * declaration above must precede every vector opcode so a backend with a
	 * smaller native map can select its direct-scalar tier coherently.
	 */
	if (plan.const_count > 0) {
		if (!lir_increment_tmpvar(&scratch_tmpvar))
			return false;
		for (i = 0; i < plan.const_count; i++) {
			if (!lir_put_opcode(plan.const_type[i] == NOCT_VALUE_FLOAT ?
					    OP_FCONST : OP_ICONST))
				return false;
			if (!lir_put_tmpvar((uint16_t)scratch_tmpvar))
				return false;
			if (!lir_put_imm32((uint32_t)plan.consts[i]))
				return false;
			if (!lir_vfor_put3(plan.const_type[i] == NOCT_VALUE_FLOAT ?
					   OP_VSPLATF32 : OP_VSPLATI32,
					   1, i,
					   0, scratch_tmpvar, 0, 0, 0))
				return false;
		}
		lir_decrement_tmpvar(scratch_tmpvar);
	}
	for (i = 0; i < plan.inv_count; i++) {
		int idx = lir_get_local_index(block, plan.inv[i]);
		if (!lir_vfor_put3(plan.inv_type[i] == NOCT_VALUE_FLOAT ?
				   OP_VSPLATF32 : OP_VSPLATI32,
				   1, plan.const_count + i,
				   0, idx, 0, 0, 0))
			return false;
	}

	/* Native backends scan from this second declaration through SUBJNZ to
	 * reserve the counter and packed-base registers. */
	if (!lir_put_opcode(OP_VINDEX_HINT))
		return false;
	if (!lir_put_tmpvar((uint16_t)plan.counter_tmpvar) ||
	    !lir_put_tmpvar((uint16_t)stop_tmpvar) ||
	    !lir_put_tmpvar((uint16_t)remaining_tmpvar) ||
	    !lir_put_imm8((uint8_t)plan.required_vregs) || !lir_put_imm8(4) ||
	    !lir_put_imm8(VINDEX_CURSOR_ONLY | VINDEX_WRITEBACK_STOP |
		(plan.force_scalar ? VINDEX_FORCE_SCALAR : 0) |
		plan.native_requirements |
		(requires_maskstore ? VINDEX_REQUIRE_MASKSTORE : 0)))
		return false;

	/* First recurrent body opcode. */
	loop_addr = (uint32_t)bytecode_top;

	/* Materialize the selected per-iteration value DAG in dependency
	   order.  Earlier entries become operands of later entries. */
	for (i = 0; i < plan.cache_count; i++) {
		if (!lir_vfor_expr(&plan, plan.cache[i].reg,
				   plan.stack_base, plan.cache[i].expr))
			return false;
		plan.cache[i].emitted = true;
	}

	/* The vector body. */
	for (stmt = block->val.for_.inner->val.basic.stmt_list;
	     stmt != NULL; stmt = stmt->next) {
		if (stmt->lhs->type == HIR_EXPR_TERM) {
			int home = -1;
			const char *sym = stmt->lhs->val.term.term->val.symbol;
			for (i = 0; i < plan.temp_count; i++) {
				if (strcmp(plan.temp[i], sym) == 0) {
					home = plan.const_count +
						plan.inv_count + i;
					break;
				}
			}
			assert(home >= 0);
			if (lir_vfor_expr_reads(stmt->rhs, sym)) {
				/* Build in a stack slot: the home must
				   stay readable during evaluation. */
				if (!lir_vfor_expr(&plan, plan.stack_base,
						   plan.stack_base + 1,
						   stmt->rhs))
					return false;
				if (!lir_vfor_put3(OP_VMOV128, 1, home,
						   1, plan.stack_base,
						   0, 0, 0))
					return false;
			} else {
				if (!lir_vfor_expr(&plan, home,
						   plan.stack_base,
						   stmt->rhs))
					return false;
			}
		} else {
			/* PSTORE32/PSTOREF32(sb, counter) = expr */
			struct hir_expr *v = lir_vfor_strip_par(stmt->rhs);
			int vs;
			struct hir_expr *base_expr =
				stmt->lhs->type == HIR_EXPR_PMASKSTORE32 ?
				stmt->lhs->val.mask_store.base :
				stmt->lhs->val.binary.expr[0];
			int base_tmpvar = lir_get_local_index(block,
				base_expr->val.term.term->val.symbol);
			if (v->type == HIR_EXPR_TERM) {
				vs = lir_vfor_term_vreg(&plan, v);
				if (vs < 0) {
					lir_fatal(N_TR("SIMD: unplanned term."));
					return false;
				}
			} else {
				if (!lir_vfor_expr(&plan, plan.stack_base,
						   plan.stack_base + 1,
						   stmt->rhs))
					return false;
				vs = plan.stack_base;
			}
			if (stmt->lhs->type == HIR_EXPR_PMASKSTORE32) {
				int vm = lir_vfor_value_vreg(&plan,
					stmt->lhs->val.mask_store.mask);
				if (vm < 0) {
					vm = plan.stack_base + 1;
					if (!lir_vfor_expr(&plan, vm, vm + 1,
							   stmt->lhs->val.mask_store.mask))
						return false;
				}
				if (!lir_put_opcode(OP_VMASKSTOREI32X4) ||
				    !lir_put_tmpvar((uint16_t)base_tmpvar) ||
				    !lir_put_tmpvar((uint16_t)plan.counter_tmpvar) ||
				    !lir_put_imm8((uint8_t)vs) ||
				    !lir_put_imm8((uint8_t)vm))
					return false;
			} else if (!lir_vfor_put3(stmt->lhs->type == HIR_EXPR_PSTOREF32 ?
					   OP_VSTOREF32X4 : OP_VSTOREI32X4,
					   0, base_tmpvar,
					   0, plan.counter_tmpvar,
					   1, vs, 1))
				return false;
		}
	}

	/* i += 4 (the vector step is explicit in OP_INC). */
	block->val.for_.inc_addr = (uint32_t)bytecode_top;
	block->cont_addr = (uint32_t)bytecode_top;
	if (!lir_put_opcode(OP_INC))
		return false;
	if (!lir_put_tmpvar((uint16_t)plan.counter_tmpvar))
		return false;
	if (!lir_put_imm8(4))
		return false;
	if (!lir_put_opcode(OP_SUBJNZ))
		return false;
	if (!lir_put_tmpvar((uint16_t)remaining_tmpvar))
		return false;
	if (!lir_put_imm8(4))
		return false;
	if (!lir_put_absolute_addr(loop_addr))
		return false;

	/* exit label: extract each temp's lane 3 (= iteration mid-1,
	   the last executed strip iteration; the remainder loop
	   overwrites these when it runs at all). */
	for (i = 0; i < plan.temp_count; i++) {
		int idx = lir_get_local_index(block, plan.temp[i]);
		if (plan.temp_induction[i])
			continue; /* OP_VINDUCT already wrote state after lane 3 */
		if (!lir_vfor_put3(plan.temp_type[i] == NOCT_VALUE_FLOAT ?
				   OP_VGETLANEF32 : OP_VGETLANEI32,
				   0, idx,
				   1, plan.const_count + plan.inv_count + i,
				   1, 3, 1))
			return false;
	}

	lir_decrement_tmpvar(remaining_tmpvar);
	lir_decrement_tmpvar(stop_tmpvar);
	lir_decrement_tmpvar(start_tmpvar);

	return true;
}

static bool
lir_ploop_has_control(struct hir_block *loop)
{
	struct hir_block *b;

	b = loop->val.for_.inner;
	while (b != NULL) {
		if (b->type != HIR_BLOCK_BASIC)
			return true;
		if (b->stop)
			break;
		b = b->succ;
	}
	return false;
}

static bool
lir_visit_for_range_block(
	struct hir_block *block)
{
	uint32_t loop_addr;
	int start_tmpvar, stop_tmpvar, loop_tmpvar, cmp_tmpvar, guard_tmpvar;
	int remaining_tmpvar;
	int packed_flags;
	int packed_factor;
	struct hir_block *b;

	assert(block != NULL);
	assert(block->type == HIR_BLOCK_FOR);
	assert(block->val.for_.is_ranged);
	assert(block->val.for_.counter_symbol);
	assert(block->val.for_.start);
	assert(block->val.for_.stop);

	/* Store the block address. */
	block->addr = (uint32_t)bytecode_top;

	/* Put a line number. */
	if (lir_lineinfo) {
		if (!lir_put_opcode(OP_LINEINFO))
			return false;
		if (!lir_put_imm32((uint32_t)block->line))
			return false;
	}

	/* Visit the start expr. */
	if (!lir_increment_tmpvar(&start_tmpvar))
		return false;
	if (!lir_visit_expr(start_tmpvar, block->val.for_.start, block))
		return false;

	/* Visit the stop expr. */
	if (!lir_increment_tmpvar(&stop_tmpvar))
		return false;
	if (!lir_visit_expr(stop_tmpvar, block->val.for_.stop, block))
		return false;

	/*
	 * Skip the whole loop when the range is empty.
	 *
	 * The per-iteration test below is an equality (OP_EQI/OP_JMPIFEQ,
	 * a pair the JIT backends fuse): the loop ends when the counter
	 * *reaches* the stop value. A range whose start is already past its
	 * stop never satisfies that, and the loop runs away. This is not an
	 * exotic case -- "for (i in 1..n)" is the ordinary way to write an
	 * insertion sort, and it becomes "1..0" every time the collection
	 * is empty.
	 *
	 * One comparison before the loop settles it, and the fused test in
	 * the body stays as it was.
	 */
	if (!lir_increment_tmpvar(&guard_tmpvar))
		return false;
	if (!lir_put_opcode(OP_GTE))
		return false;
	if (!lir_put_tmpvar((uint16_t)guard_tmpvar))
		return false;
	if (!lir_put_tmpvar((uint16_t)start_tmpvar))
		return false;
	if (!lir_put_tmpvar((uint16_t)stop_tmpvar))
		return false;
	if (!lir_put_opcode(OP_JMPIFTRUE))
		return false;
	if (!lir_put_tmpvar((uint16_t)guard_tmpvar))
		return false;
	if (!lir_put_branch_addr(block->succ))
		return false;
	lir_decrement_tmpvar(guard_tmpvar);

	/* Put the start value to a loop variable. */
	loop_tmpvar = lir_get_local_index(block, block->val.for_.counter_symbol);
	if (!lir_put_opcode(OP_ASSIGN))
		return false;
	if (!lir_put_tmpvar((uint16_t)loop_tmpvar))
		return false;
	if (!lir_put_tmpvar((uint16_t)start_tmpvar))
		return false;

	/*
	 * ABCE-proven scalar Packed loops use the same countdown latch as a
	 * vector remainder.  OP_PLOOP_HINT is semantic no-op metadata; an older
	 * or resource-constrained backend may ignore it and emit the ordinary
	 * scalar operations below.
	 */
	if (block->val.for_.packed_lanes == 1) {
		packed_factor = block->val.for_.scalar_unroll == 4 ? 4 : 1;
		assert(block->val.for_.scalar_unroll == 0 ||
		       block->val.for_.scalar_unroll == 1 ||
		       block->val.for_.scalar_unroll == 4);
		if (!lir_increment_tmpvar(&remaining_tmpvar))
			return false;
		if (!lir_put_opcode(OP_ISUB) ||
		    !lir_put_tmpvar((uint16_t)remaining_tmpvar) ||
		    !lir_put_tmpvar((uint16_t)stop_tmpvar) ||
		    !lir_put_tmpvar((uint16_t)start_tmpvar))
			return false;
		packed_flags = PLOOP_ALLOW_REGCACHE;
		if (block->val.for_.typed_int_region)
			packed_flags |= PLOOP_TYPED_INT;
		if (lir_ploop_has_control(block))
			packed_flags |= PLOOP_HAS_CONTROL;
		if (packed_factor == 4)
			packed_flags |= PLOOP_UNROLL4;
		if (!lir_put_opcode(OP_PLOOP_HINT) ||
		    !lir_put_tmpvar((uint16_t)loop_tmpvar) ||
		    !lir_put_tmpvar((uint16_t)stop_tmpvar) ||
		    !lir_put_tmpvar((uint16_t)remaining_tmpvar) ||
		    !lir_put_imm8(1) ||
		    !lir_put_imm8((uint8_t)packed_flags))
			return false;

		loop_addr = (uint32_t)bytecode_top;
		b = block->val.for_.inner;
		while (b != NULL) {
			if (!lir_visit_block(b))
				return false;
			if (b->stop)
				break;
			b = b->succ;
		}

		block->val.for_.inc_addr = (uint32_t)bytecode_top;
		block->cont_addr = (uint32_t)bytecode_top;
		if (!lir_put_opcode(OP_INC) ||
		    !lir_put_tmpvar((uint16_t)loop_tmpvar) ||
		    !lir_put_imm8((uint8_t)packed_factor))
			return false;
		if (!lir_put_opcode(OP_SUBJNZ) ||
		    !lir_put_tmpvar((uint16_t)remaining_tmpvar) ||
		    !lir_put_imm8((uint8_t)packed_factor) ||
		    !lir_put_absolute_addr(loop_addr))
			return false;

		lir_decrement_tmpvar(remaining_tmpvar);
		lir_decrement_tmpvar(stop_tmpvar);
		lir_decrement_tmpvar(start_tmpvar);
		return true;
	}

	/* Put a loop header. */
	loop_addr = (uint32_t)bytecode_top;
	if (!lir_increment_tmpvar(&cmp_tmpvar))
		return false;
	if (!lir_put_opcode(OP_EQI))
		return false;
	if (!lir_put_tmpvar((uint16_t)cmp_tmpvar))
		return false;
	if (!lir_put_tmpvar((uint16_t)loop_tmpvar))
		return false;
	if (!lir_put_tmpvar((uint16_t)stop_tmpvar))
		return false;
	if (!lir_put_opcode(OP_JMPIFEQ))
		return false;
	if (!lir_put_tmpvar((uint16_t)cmp_tmpvar))
		return false;
	if (!lir_put_branch_addr(block->succ))
		return false;

	/* Visit an inner block. */
	b = block->val.for_.inner;
	while (b != NULL) {
		if (!lir_visit_block(b))
			return false;
		if (b->stop)
			break;
		b = b->succ;
	}

	/* Store the incrementer address. A "continue" jumps here. */
	block->val.for_.inc_addr = (uint32_t)bytecode_top;
	block->cont_addr = (uint32_t)bytecode_top;

	/* Increment the loop variable. */
	if (!lir_put_opcode(OP_INC))
		return false;
	if (!lir_put_tmpvar((uint16_t)loop_tmpvar))
		return false;
	if (!lir_put_imm8(1))
		return false;

	/* Put a back-edge jump. */
	if (!lir_put_opcode(OP_JMP))
		return false;
	if (!lir_put_absolute_addr(loop_addr))
		return false;

	lir_decrement_tmpvar(cmp_tmpvar);
	lir_decrement_tmpvar(stop_tmpvar);
	lir_decrement_tmpvar(start_tmpvar);

	return true;
}

static bool
lir_visit_for_kv_block(
	struct hir_block *block)
{
	uint32_t loop_addr;
	int col_tmpvar, size_tmpvar, i_tmpvar, key_tmpvar, val_tmpvar, cmp_tmpvar;
	struct hir_block *b;

	assert(block != NULL);
	assert(block->type == HIR_BLOCK_FOR);
	assert(!block->val.for_.is_ranged);
	assert(block->val.for_.key_symbol != NULL);
	assert(block->val.for_.value_symbol != NULL);
	assert(block->val.for_.collection != NULL);

	/* Store the block address. */
	block->addr = (uint32_t)bytecode_top;

	/* Put a line number. */
	if (lir_lineinfo) {
		if (!lir_put_opcode(OP_LINEINFO))
			return false;
		if (!lir_put_imm32((uint32_t)block->line))
			return false;
	}

	/* Visit a collection expr. */
	if (!lir_increment_tmpvar(&col_tmpvar))
		return false;
	if (!lir_visit_expr(col_tmpvar, block->val.for_.collection, block))
		return false;

	/* Get a collection size. */
	if (!lir_increment_tmpvar(&size_tmpvar))
		return false;
	if (!lir_put_opcode(OP_LEN))
		return false;
	if (!lir_put_tmpvar((uint16_t)size_tmpvar))
		return false;
	if (!lir_put_tmpvar((uint16_t)col_tmpvar))
		return false;

	/* Assign 0 to `i`. */
	if (!lir_increment_tmpvar(&i_tmpvar))
		return false;
	if (!lir_put_opcode(OP_ICONST))
		return false;
	if (!lir_put_tmpvar((uint16_t)i_tmpvar))
		return false;
	if (!lir_put_imm32(0))
		return false;

	/* Prepare a key and a value. */
	key_tmpvar = lir_get_local_index(block, block->val.for_.key_symbol);
	val_tmpvar = lir_get_local_index(block, block->val.for_.value_symbol);
	if (!lir_increment_tmpvar(&cmp_tmpvar))
		return false;

	/* Put a loop header. */
	loop_addr = (uint32_t)bytecode_top;		/* LOOP: */
	if (!lir_put_opcode(OP_EQI)) 			/*  if i == size then break */
		return false;
	if (!lir_put_tmpvar((uint16_t)cmp_tmpvar))
		return false;
	if (!lir_put_tmpvar((uint16_t)i_tmpvar))
		return false;
	if (!lir_put_tmpvar((uint16_t)size_tmpvar))
		return false;
	if (!lir_put_opcode(OP_JMPIFEQ)) 		/*  if i == size then break */
		return false;
	if (!lir_put_tmpvar((uint16_t)cmp_tmpvar))
		return false;
	if (!lir_put_branch_addr(block->succ))
		return false;
	if (!lir_put_opcode(OP_GETDICTKEYBYINDEX))	/* key = dict.getKeyByIndex(i) */
		return false;
	if (!lir_put_tmpvar((uint16_t)key_tmpvar))
		return false;
	if (!lir_put_tmpvar((uint16_t)col_tmpvar))
		return false;
	if (!lir_put_tmpvar((uint16_t)i_tmpvar))
		return false;
	if (!lir_put_opcode(OP_GETDICTVALBYINDEX)) 	/* val = dict.getValByIndex(i) */
		return false;
	if (!lir_put_tmpvar((uint16_t)val_tmpvar))
		return false;
	if (!lir_put_tmpvar((uint16_t)col_tmpvar))
		return false;
	if (!lir_put_tmpvar((uint16_t)i_tmpvar))
		return false;
	if (!lir_put_opcode(OP_INC)) 		/* i++ */
		return false;
	if (!lir_put_tmpvar((uint16_t)i_tmpvar))
		return false;
	if (!lir_put_imm8(1))
		return false;

	/*
	 * A "continue" jumps to the loop head: the cursor has already
	 * been advanced above, before the body runs.
	 */
	block->cont_addr = loop_addr;

	/* Visit an inner block. */
	b = block->val.for_.inner;
	while (b != NULL) {
		if (!lir_visit_block(b))
			return false;
		if (b->stop)
			break;
		b = b->succ;
	}

	/* Put a back-edge jump. */
	if (!lir_put_opcode(OP_JMP))
		return false;
	if (!lir_put_absolute_addr(loop_addr))
		return false;

	lir_decrement_tmpvar(cmp_tmpvar);
	lir_decrement_tmpvar(i_tmpvar);
	lir_decrement_tmpvar(size_tmpvar);
	lir_decrement_tmpvar(col_tmpvar);

	return true;
}

static bool
lir_visit_for_v_block(
	struct hir_block *block)
{
	uint32_t loop_addr;
	int arr_tmpvar, size_tmpvar, i_tmpvar, val_tmpvar, cmp_tmpvar;
	struct hir_block *b;

	assert(block != NULL);
	assert(block->type == HIR_BLOCK_FOR);
	assert(!block->val.for_.is_ranged);
	assert(block->val.for_.value_symbol != NULL);
	assert(block->val.for_.collection != NULL);

	/* Store the block address. */
	block->addr = (uint32_t)bytecode_top;

	/* Put a line number. */
	if (lir_lineinfo) {
		if (!lir_put_opcode(OP_LINEINFO))
			return false;
		if (!lir_put_imm32((uint32_t)block->line))
			return false;
	}

	/* Visit an array expr. */
	if (!lir_increment_tmpvar(&arr_tmpvar))
		return false;
	if (!lir_visit_expr(arr_tmpvar, block->val.for_.collection, block))
		return false;

	/* Get a collection size. */
	if (!lir_increment_tmpvar(&size_tmpvar))
		return false;
	if (!lir_put_opcode(OP_LEN))
		return false;
	if (!lir_put_tmpvar((uint16_t)size_tmpvar))
		return false;
	if (!lir_put_tmpvar((uint16_t)arr_tmpvar))
		return false;

	/* Assign 0 to `i`. */
	if (!lir_increment_tmpvar(&i_tmpvar))
		return false;
	if (!lir_put_opcode(OP_ICONST))
		return false;
	if (!lir_put_tmpvar((uint16_t)i_tmpvar))
		return false;
	if (!lir_put_imm32(0))
		return false;

	/* Prepare a value. */
	val_tmpvar = lir_get_local_index(block, block->val.for_.value_symbol);
	if (!lir_increment_tmpvar(&cmp_tmpvar))
		return false;

	/* Put a loop header. */
	loop_addr = (uint32_t)bytecode_top;		/* LOOP: */
	if (!lir_put_opcode(OP_EQI)) 			/*  if i == size then break */
		return false;
	if (!lir_put_tmpvar((uint16_t)cmp_tmpvar))
		return false;
	if (!lir_put_tmpvar((uint16_t)i_tmpvar))
		return false;
	if (!lir_put_tmpvar((uint16_t)size_tmpvar))
		return false;
	if (!lir_put_opcode(OP_JMPIFEQ))
		return false;
	if (!lir_put_tmpvar((uint16_t)cmp_tmpvar))
		return false;
	if (!lir_put_branch_addr(block->succ))
		return false;
	if (!lir_put_opcode(OP_LOADARRAY)) 	/* val = array[i] */
		return false;
	if (!lir_put_tmpvar((uint16_t)val_tmpvar))
		return false;
	if (!lir_put_tmpvar((uint16_t)arr_tmpvar))
		return false;
	if (!lir_put_tmpvar((uint16_t)i_tmpvar))
		return false;
	if (!lir_put_opcode(OP_INC)) 		/* i++ */
		return false;
	if (!lir_put_tmpvar((uint16_t)i_tmpvar))
		return false;
	if (!lir_put_imm8(1))
		return false;

	/*
	 * A "continue" jumps to the loop head: the cursor has already
	 * been advanced above, before the body runs.
	 */
	block->cont_addr = loop_addr;

	/* Visit an inner block. */
	b = block->val.for_.inner;
	while (b != NULL) {
		if (!lir_visit_block(b))
			return false;
		if (b->stop)
			break;
		b = b->succ;
	}

	/* Put a back-edge jump. */
	if (!lir_put_opcode(OP_JMP))
		return false;
	if (!lir_put_absolute_addr(loop_addr))
		return false;

	lir_decrement_tmpvar(cmp_tmpvar);
	lir_decrement_tmpvar(i_tmpvar);
	lir_decrement_tmpvar(size_tmpvar);
	lir_decrement_tmpvar(arr_tmpvar);

	return true;
}

/* Check whether LHS is local. */
static int
lir_get_local_index(
	struct hir_block *block,
	const char *symbol)
{
	struct hir_block *func;
	struct hir_local *local;

	/* Get a root func block. */
	func = block;
	while (func->type != HIR_BLOCK_FUNC)
		func = func->parent;

	/* Search in an explicit local variable list. */
	local = func->val.func.local;
	while (local != NULL) {
		if (strcmp(local->symbol, symbol) == 0)
			break;
		local = local->next;
	}
	assert(local != NULL);

	return local->index;
}

static struct hir_local *
lir_get_local_by_index(
	struct hir_block *block,
	int index)
{
	struct hir_block *func;
	struct hir_local *local;

	func = lir_root_func(block);
	if (func == NULL)
		return NULL;
	local = func->val.func.local;
	while (local != NULL) {
		if (local->index == index)
			return local;
		local = local->next;
	}
	return NULL;
}

static bool
lir_visit_while_block(
	struct hir_block *block)
{
	uint32_t loop_addr;
	int cmp_tmpvar;
	struct hir_block *b;

	assert(block != NULL);
	assert(block->type == HIR_BLOCK_WHILE);

	/* Store the block address. */
	block->addr = (uint32_t)bytecode_top;

	/* Put a line number. */
	if (lir_lineinfo) {
		if (!lir_put_opcode(OP_LINEINFO))
			return false;
		if (!lir_put_imm32((uint32_t)block->line))
			return false;
	}

	/* Put a loop header. */
	loop_addr = (uint32_t)bytecode_top;
	if (!lir_increment_tmpvar(&cmp_tmpvar))
		return false;
	if (!lir_visit_expr(cmp_tmpvar, block->val.while_.cond, block))
		return false;
	if (!lir_put_materialize_type(
		    cmp_tmpvar,
		    lir_expr_proven_type(block->val.while_.cond, block)))
		return false;
	if (!lir_put_opcode(OP_JMPIFFALSE))
		return false;
	if (!lir_put_tmpvar((uint16_t)cmp_tmpvar))
		return false;
	if (!lir_put_branch_addr(block->succ))
		return false;
	lir_decrement_tmpvar(cmp_tmpvar);

	/* A "continue" jumps to the loop head, re-testing the condition. */
	block->cont_addr = loop_addr;

	/* Visit an inner block. */
	b = block->val.while_.inner;
	while (b != NULL) {
		if (!lir_visit_block(b))
			return false;
		if (b->stop)
			break;
		b = b->succ;
	}

	/* Put a back-edge jump. */
	if (!lir_put_opcode(OP_JMP))
		return false;
	if (!lir_put_absolute_addr(loop_addr))
		return false;

	return true;
}

static bool
lir_visit_stmt(
	struct hir_block *parent,
	struct hir_stmt *stmt)
{
	struct hir_block *func;
	int rhs_tmpvar, obj_tmpvar, access_tmpvar;
	bool is_lhs_local;
	bool is_return;
	bool checked_annotations;

	assert(stmt != NULL);
	assert(stmt->rhs != NULL);

	func = lir_root_func(parent);
	assert(func != NULL);
	checked_annotations = lir_optimize_level >= 1 ||
		func->val.func.is_fast;

	/* Put a line number. */
	if (lir_lineinfo) {
		if (!lir_put_opcode(OP_LINEINFO))
			return false;
		if (!lir_put_imm32((uint32_t)stmt->line))
			return false;
	}

	/* Check whether LHS is a local variable. */
	is_lhs_local = lir_check_lhs_local(parent, stmt->lhs, &rhs_tmpvar);
	is_return = is_lhs_local && stmt->lhs != NULL &&
		stmt->lhs->type == HIR_EXPR_TERM &&
		stmt->lhs->val.term.term->type == HIR_TERM_SYMBOL &&
		strcmp(stmt->lhs->val.term.term->val.symbol, "$return") == 0;

	/* Prepare a tmpvar for RHS if LHS is not an explicit local variable. */
	if (!is_lhs_local) {
		if (!lir_increment_tmpvar(&rhs_tmpvar))
			return false;
	}

	/* Visit RHS. */
	if (!lir_visit_expr(rhs_tmpvar, stmt->rhs, parent))
		return false;

	/*
	 * Primitive annotations are checked at -O1 and above.  A source fast
	 * hint keeps this ordinary checked path even without optimization.
	 */
	if (is_lhs_local && !is_return && checked_annotations) {
		struct hir_local *local;
		int declared;
		int proven;
		bool widening;

		local = lir_get_local_by_index(parent, rhs_tmpvar);
		declared = local != NULL ? local->declared_type : -1;
		if (local != NULL &&
		    !local->is_parameter &&
		    (declared == NOCT_VALUE_INT ||
		     declared == NOCT_VALUE_LONG ||
		     declared == NOCT_VALUE_FLOAT ||
		     declared == NOCT_VALUE_DOUBLE)) {
			proven = lir_expr_proven_type(stmt->rhs, parent);
			widening = (declared == NOCT_VALUE_LONG &&
				    proven == NOCT_VALUE_INT) ||
				   (declared == NOCT_VALUE_DOUBLE &&
				    proven == NOCT_VALUE_FLOAT);
			if (proven >= 0 && proven != declared && !widening) {
				lir_error_line = stmt->line;
				lir_fatal(N_TR("Local initializer or assignment does not match its declared type."));
				return false;
			}
			if (proven < 0 || widening) {
				int code;
				code = lir_typecheck_code(declared, -1, false,
							  false, true);
				if (!lir_put_opcode(OP_CHECKTYPE) ||
				    !lir_put_tmpvar((uint16_t)rhs_tmpvar) ||
				    !lir_put_imm8((uint8_t)code))
					return false;
			}
		}
	}

	/*
	 * Return contracts are checked at level 2, or on the ordinary checked
	 * path for a source fast hint.
	 */
	if (is_return &&
	    (lir_optimize_level >= 2 || func->val.func.is_fast)) {
		int proven;

		if (func->val.func.return_type == HIR_TYPE_VOID &&
		    !stmt->is_bare_return) {
			lir_error_line = stmt->line;
			lir_fatal(N_TR("A void function cannot return a value."));
			return false;
		}
		if (func->val.func.return_type >= 0 && stmt->is_bare_return) {
			lir_error_line = stmt->line;
			lir_fatal(N_TR("A typed function must return a value."));
			return false;
		}
		if (func->val.func.return_type >= 0) {
			proven = lir_expr_proven_type(stmt->rhs, parent);
			if (proven >= 0 && proven != func->val.func.return_type) {
				lir_error_line = stmt->line;
				lir_fatal(N_TR("Return operand does not match the declared type."));
				return false;
			}
			if (proven < 0 && !lir_emit_return_check(parent))
				return false;
		}
	}
	if (is_return &&
	    !lir_put_materialize_type(rhs_tmpvar,
				       lir_expr_proven_type(stmt->rhs, parent)))
		return false;

	/* Visit LHS if LHS is not an explicit local variable. */
	if (stmt->lhs != NULL && !is_lhs_local) {
		if (stmt->lhs->type == HIR_EXPR_TERM) {
			assert(stmt->lhs->val.term.term->type == HIR_TERM_SYMBOL);

			/* Put a storesymbol. */
			if (!lir_put_materialize_type(
				    rhs_tmpvar, lir_expr_proven_type(stmt->rhs, parent)))
				return false;
			if (!lir_put_opcode(OP_STORESYMBOL))
				return false;
			if (!lir_put_string(stmt->lhs->val.term.term->val.symbol))
				return false;
			if (!lir_put_tmpvar((uint16_t)rhs_tmpvar))
				return false;
		} else if (stmt->lhs->type == HIR_EXPR_SUBSCR) {
			assert(stmt->lhs->val.binary.expr[0] != NULL);
			assert(stmt->lhs->val.binary.expr[1] != NULL);

			/* Visit an array. */
			if (!lir_increment_tmpvar(&obj_tmpvar))
				return false;
			if (!lir_visit_expr(obj_tmpvar, stmt->lhs->val.binary.expr[0], parent))
				return false;

			/* Visit a subscript. */
			if (!lir_increment_tmpvar(&access_tmpvar))
				return false;
			if (!lir_visit_expr(access_tmpvar, stmt->lhs->val.binary.expr[1], parent))
				return false;
			if (!lir_put_materialize_type(
				    access_tmpvar,
				    lir_expr_proven_type(stmt->lhs->val.binary.expr[1], parent)) ||
			    !lir_put_materialize_type(
				    rhs_tmpvar, lir_expr_proven_type(stmt->rhs, parent)))
				return false;

			/* Put a store. */
			if (!lir_put_opcode(OP_STOREARRAY))
				return false;
			if (!lir_put_tmpvar((uint16_t)obj_tmpvar))
				return false;
			if (!lir_put_tmpvar((uint16_t)access_tmpvar))
				return false;
			if (!lir_put_tmpvar((uint16_t)rhs_tmpvar))
				return false;

			lir_decrement_tmpvar(access_tmpvar);
			lir_decrement_tmpvar(obj_tmpvar);
		} else if (stmt->lhs->type == HIR_EXPR_PSTORE8 ||
			   stmt->lhs->type == HIR_EXPR_PSTORE16 ||
			   stmt->lhs->type == HIR_EXPR_PSTORE32 ||
			   stmt->lhs->type == HIR_EXPR_PSTORE64 ||
			   stmt->lhs->type == HIR_EXPR_PSTOREF32) {
			assert(stmt->lhs->val.binary.expr[0] != NULL);
			assert(stmt->lhs->val.binary.expr[1] != NULL);

			/* Visit the base address. */
			if (!lir_increment_tmpvar(&obj_tmpvar))
				return false;
			if (!lir_visit_expr(obj_tmpvar, stmt->lhs->val.binary.expr[0], parent))
				return false;

			/* Visit the offset. */
			if (!lir_increment_tmpvar(&access_tmpvar))
				return false;
			if (!lir_visit_expr(access_tmpvar, stmt->lhs->val.binary.expr[1], parent))
				return false;

			/* Put a raw store. */
			{
				int pst;
				switch (stmt->lhs->type) {
				case HIR_EXPR_PSTORE16: pst = OP_PSTORE16; break;
				case HIR_EXPR_PSTORE32: pst = OP_PSTORE32; break;
				case HIR_EXPR_PSTORE64: pst = OP_PSTORE64; break;
				case HIR_EXPR_PSTOREF32: pst = OP_PSTOREF32; break;
				default:                pst = OP_PSTORE8;  break;
				}
				if (!lir_put_opcode((uint8_t)pst))
					return false;
			}
			if (!lir_put_tmpvar((uint16_t)obj_tmpvar))
				return false;
			if (!lir_put_tmpvar((uint16_t)access_tmpvar))
				return false;
			if (!lir_put_tmpvar((uint16_t)rhs_tmpvar))
				return false;

			lir_decrement_tmpvar(access_tmpvar);
			lir_decrement_tmpvar(obj_tmpvar);
		} else if (stmt->lhs->type == HIR_EXPR_DOT) {
			assert(stmt->lhs->val.dot.obj != NULL);
			assert(stmt->lhs->val.dot.symbol != NULL);

			/* Visit an object. */
			if (!lir_increment_tmpvar(&obj_tmpvar))
				return false;
			if (!lir_visit_expr(obj_tmpvar, stmt->lhs->val.dot.obj, parent))
				return false;
			if (!lir_put_materialize_type(
				    rhs_tmpvar, lir_expr_proven_type(stmt->rhs, parent)))
				return false;

			/* Put a store. */
			if (!lir_put_opcode(OP_STOREDOT))
				return false;
			if (!lir_put_tmpvar((uint16_t)obj_tmpvar))
				return false;
			if (!lir_put_string(stmt->lhs->val.dot.symbol))
				return false;
			if (!lir_put_tmpvar((uint16_t)rhs_tmpvar))
				return false;

			lir_decrement_tmpvar(obj_tmpvar);
		} else {
			lir_fatal(N_TR("LHS is not a term or an array element."));
			return false;
		}
	}

	if (!is_lhs_local)
		lir_decrement_tmpvar(rhs_tmpvar);

	return true;
}

/* Check whether LHS is local variable. */
static bool
lir_check_lhs_local(
	struct hir_block *block,
	struct hir_expr *lhs,
	int *rhs_tmpvar)
{
	struct hir_block *func;
	struct hir_local *local;
	const char * symbol;

	/* Exclude non symbol term LHS. */
	if (lhs == NULL)
		return false;
	if (lhs->type != HIR_EXPR_TERM)
		return false;
	if (lhs->val.term.term->type != HIR_TERM_SYMBOL)
		return false;

	/* Get a symbol. */
	symbol = lhs->val.term.term->val.symbol;

	/* Check for a return value. */
	if (strcmp(symbol, "$return") == 0) {
		*rhs_tmpvar = 0;
		return true;
	}

	/* Get a root func block. */
	func = block->parent;
	while (func->type != HIR_BLOCK_FUNC)
		func = func->parent;

	/* Search in the local variable list. */
	local = func->val.func.local;
	while (local != NULL) {
		if (strcmp(local->symbol, symbol) == 0)
			break;
		local = local->next;
	}
	if (local == NULL)
		return false;

	/* Use a tmpvar index for the local variable. */
	*rhs_tmpvar = local->index;

	return true;
}

static bool
lir_visit_expr(
	int dst_tmpvar,
	struct hir_expr *expr,
	struct hir_block *block)
{
	assert(expr != NULL);

	switch (expr->type) {
	case HIR_EXPR_TERM:
		/* Visit a term inside the expr. */
		if (!lir_visit_term(dst_tmpvar, expr->val.term.term, block))
			return false;
		break;
	case HIR_EXPR_PAR:
		/* Visit an expr inside the expr. */
		if (!lir_visit_expr(dst_tmpvar, expr->val.unary.expr, block))
			return false;
		break;
	case HIR_EXPR_NEG:
	case HIR_EXPR_NOT:
		/* For the unary operators. */
		if (!lir_visit_unary_expr(dst_tmpvar, expr, block))
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
		/* For the binary operators. */
		if (!lir_visit_binary_expr(dst_tmpvar, expr, block))
			return false;
		break;
	case HIR_EXPR_PGATHER32:
	{
		/* Scalar remainder/fallback retains the ordinary checked
		   subscript semantics through the owner packed object. */
		struct hir_expr sub;
		memset(&sub, 0, sizeof(sub));
		sub.type = HIR_EXPR_SUBSCR;
		sub.val.binary.expr[0] = expr->val.gather.packed;
		sub.val.binary.expr[1] = expr->val.gather.index;
		if (!lir_visit_binary_expr(dst_tmpvar, &sub, block))
			return false;
		break;
	}
	case HIR_EXPR_PBASE:
	case HIR_EXPR_PLEN:
		/* ABCE unary ops. */
		if (!lir_visit_abce_unary_expr(dst_tmpvar, expr, block))
			return false;
		break;
	case HIR_EXPR_PCHECK:
	case HIR_EXPR_TYPEIS:
		/* ABCE type-test ops. (Operand 2 is an int-constant imm8.) */
		if (!lir_visit_abce_typetest_expr(dst_tmpvar, expr, block))
			return false;
		break;
	case HIR_EXPR_LAND:
	case HIR_EXPR_LOR:
		/* For the short-circuiting logical operators. */
		if (!lir_visit_logical_expr(dst_tmpvar, expr, block))
			return false;
		break;
	case HIR_EXPR_DOT:
		/* For the dot operator. */
		if (!lir_visit_dot_expr(dst_tmpvar, expr, block))
			return false;
		break;
	case HIR_EXPR_CAPTURE:
		/* For the CSE capture operator. */
		if (!lir_visit_capture_expr(dst_tmpvar, expr, block))
			return false;
		break;
	case HIR_EXPR_CALL:
		/* For a function call. */
		if (!lir_visit_call_expr(dst_tmpvar, expr, block))
			return false;
		break;
	case HIR_EXPR_THISCALL:
		/* For a method call. */
		if (!lir_visit_thiscall_expr(dst_tmpvar, expr, block))
			return false;
		break;
	case HIR_EXPR_ARRAY:
		/* For an array expression. */
		if (!lir_visit_array_expr(dst_tmpvar, expr, block))
			return false;
		break;
	case HIR_EXPR_DICT:
		/* For a dictionary expression. */
		if (!lir_visit_dict_expr(dst_tmpvar, expr, block))
			return false;
		break;
	case HIR_EXPR_NEW:
		/* For a new expression. */
		if (!lir_visit_new_expr(dst_tmpvar, expr, block))
			return false;
		break;
	default:
		assert(NEVER_COME_HERE);
		abort();
		break;
	}

	/* Aggregate all definitions of a VM-frame slot.  A slot reused by
	 * multiple compiler-temporary lifetimes remains fixed only when every
	 * observed definition has the same primitive runtime tag. */
	lir_note_tmpvar_type(dst_tmpvar, lir_expr_proven_type(expr, block));

	return true;
}

static void
lir_note_tmpvar_type(int tmpvar, int type)
{
	if (tmpvar < 0 || tmpvar >= LIR_TMPVAR_MAX)
		return;
	if (tmpvar >= (int)tmpvar_local_count)
		tmpvar_lifetime_noted[tmpvar] = true;
	/* -O0 preserves the canonical tag+payload producer behavior. */
	if (lir_optimize_level < 1) {
		tmpvar_type_state[tmpvar] = LIR_TMP_TYPE_DYNAMIC;
		return;
	}
	/* Every definition of a checked ordinary local is either proven to
	 * match or followed by OP_CHECKTYPE.  Its canonical post-definition
	 * tag is therefore the declared primitive type even when the producer
	 * itself was dynamic. */
	if (tmpvar_contract_type[tmpvar] >= 0) {
		tmpvar_type_state[tmpvar] = LIR_TMP_TYPE_FIXED;
		tmpvar_fixed_type[tmpvar] = tmpvar_contract_type[tmpvar];
		return;
	}
	if (type != NOCT_VALUE_INT && type != NOCT_VALUE_LONG &&
	    type != NOCT_VALUE_FLOAT && type != NOCT_VALUE_DOUBLE) {
		tmpvar_type_state[tmpvar] = LIR_TMP_TYPE_DYNAMIC;
		return;
	}
	if (tmpvar_type_state[tmpvar] == LIR_TMP_TYPE_UNSEEN) {
		tmpvar_type_state[tmpvar] = LIR_TMP_TYPE_FIXED;
		tmpvar_fixed_type[tmpvar] = (int8_t)type;
	} else if (tmpvar_type_state[tmpvar] == LIR_TMP_TYPE_FIXED &&
		   tmpvar_fixed_type[tmpvar] != type) {
		tmpvar_type_state[tmpvar] = LIR_TMP_TYPE_DYNAMIC;
	}
}

static bool
lir_put_materialize_type(int tmpvar, int type)
{
	if (lir_optimize_level < 1)
		return true;
	if (type != NOCT_VALUE_INT && type != NOCT_VALUE_LONG &&
	    type != NOCT_VALUE_FLOAT && type != NOCT_VALUE_DOUBLE)
		return true;
	if (!lir_put_opcode(OP_MATERIALIZE_TYPE) ||
	    !lir_put_tmpvar((uint16_t)tmpvar) ||
	    !lir_put_imm8((uint8_t)type))
		return false;
	materialize_emit_count++;
	return true;
}

/* OP_TMPVAR_TYPE is metadata and therefore may be prepended after the body
 * has established the complete slot-type meet.  Relocation offsets and
 * targets are adjusted by patch_block_address(prefix). */
static bool
lir_prepend_tmpvar_types(uint32_t *prefix)
{
	uint32_t i;
	uint32_t count;
	uint32_t p;

	count = 0;
	for (i = 0; i < tmpvar_count; i++) {
		if (tmpvar_type_state[i] == LIR_TMP_TYPE_FIXED ||
		    i >= tmpvar_local_count)
			count++;
	}
	*prefix = count * 4;
	if (*prefix == 0)
		return true;
	if (bytecode_top > BYTECODE_BUF_SIZE - *prefix) {
		lir_fatal(N_TR("Bytecode is too large."));
		return false;
	}
	memmove(bytecode + *prefix, bytecode, bytecode_top);
	bytecode_top += *prefix;
	p = 0;
	for (i = 0; i < tmpvar_count; i++) {
		if (tmpvar_type_state[i] != LIR_TMP_TYPE_FIXED &&
		    i < tmpvar_local_count)
			continue;
		bytecode[p++] = OP_TMPVAR_TYPE;
		bytecode[p++] = (uint8_t)(i >> 8);
		bytecode[p++] = (uint8_t)i;
		bytecode[p++] = (uint8_t)(
			tmpvar_type_state[i] == LIR_TMP_TYPE_FIXED ?
			tmpvar_fixed_type[i] : TMPVAR_TYPE_DYNAMIC) |
			(i >= tmpvar_local_count ? TMPVAR_TYPE_COMPILER_TEMP : 0);
	}
	assert(p == *prefix);
	return true;
}

static bool
lir_visit_unary_expr(
	int dst_tmpvar,
	struct hir_expr *expr,
	struct hir_block *block)
{
	int opr_tmpvar;

	assert(expr != NULL);
	assert(expr->type == HIR_EXPR_NEG || expr->type == HIR_EXPR_NOT);

	/* Visit the operand expr. */
	if (!lir_increment_tmpvar(&opr_tmpvar))
		return false;
	if (!lir_visit_expr(opr_tmpvar, expr->val.unary.expr, block))
		return false;
	if (!lir_put_materialize_type(
		    opr_tmpvar,
		    lir_expr_proven_type(expr->val.unary.expr, block)))
		return false;

	/* Put an opcode. */
	switch (expr->type) {
	case HIR_EXPR_NEG:
		if (!lir_put_opcode(OP_NEG))
			return false;
		if (!lir_put_tmpvar((uint16_t)dst_tmpvar))
			return false;
		if (!lir_put_tmpvar((uint16_t)opr_tmpvar))
			return false;
		break;
	case HIR_EXPR_NOT:
		if (!lir_put_opcode(OP_NOT))
			return false;
		if (!lir_put_tmpvar((uint16_t)dst_tmpvar))
			return false;
		if (!lir_put_tmpvar((uint16_t)opr_tmpvar))
			return false;
		break;
	default:
		assert(NEVER_COME_HERE);
		break;
	}

	lir_decrement_tmpvar(opr_tmpvar);

	return true;
}

/*
 * Lower a short-circuiting logical operator (&& or ||).
 *
 * The result is a boolean 1 or 0, and the second operand is evaluated
 * only when the first has not already decided the result. Jump targets
 * inside the expression are backpatched here, because the block-level
 * patch table only handles jumps to whole HIR blocks.
 *
 *   &&:  eval a; if false -> Lzero
 *        eval b; if false -> Lzero
 *        dst = 1; jmp Lend
 *   Lzero: dst = 0
 *   Lend:
 *
 *   ||:  eval a; if true -> Lone
 *        eval b; if true -> Lone
 *        dst = 0; jmp Lend
 *   Lone: dst = 1
 *   Lend:
 */
static bool
lir_patch_u32(uint32_t at, uint32_t value)
{
	if (loc_count >= LOC_MAX) {
		lir_fatal(N_TR("Too many jumps."));
		return false;
	}
	bytecode[at]     = (uint8_t)((value >> 24) & 0xff);
	bytecode[at + 1] = (uint8_t)((value >> 16) & 0xff);
	bytecode[at + 2] = (uint8_t)((value >> 8) & 0xff);
	bytecode[at + 3] = (uint8_t)(value & 0xff);
	loc_tbl[loc_count].type = LOC_ABSOLUTE;
	loc_tbl[loc_count].offset = at;
	loc_tbl[loc_count].block = NULL;
	loc_tbl[loc_count].addr = value;
	loc_count++;
	return true;
}

static bool
lir_visit_logical_expr(
	int dst_tmpvar,
	struct hir_expr *expr,
	struct hir_block *block)
{
	int cond_tmpvar;
	bool is_and;
	uint8_t short_op;
	uint32_t patch0, patch1, patch_skip, decided_addr, end_addr;

	is_and = (expr->type == HIR_EXPR_LAND);
	/* && short-circuits on a false operand, || on a true one. */
	short_op = is_and ? OP_JMPIFFALSE : OP_JMPIFTRUE;

	if (!lir_increment_tmpvar(&cond_tmpvar))
		return false;

	/* Operand 0. */
	if (!lir_visit_expr(cond_tmpvar, expr->val.binary.expr[0], block))
		return false;
	if (!lir_put_materialize_type(
		    cond_tmpvar,
		    lir_expr_proven_type(expr->val.binary.expr[0], block)))
		return false;
	if (!lir_put_opcode(short_op))
		return false;
	if (!lir_put_tmpvar((uint16_t)cond_tmpvar))
		return false;
	patch0 = bytecode_top;
	if (!lir_put_u32(0))
		return false;

	/* Operand 1. */
	if (!lir_visit_expr(cond_tmpvar, expr->val.binary.expr[1], block))
		return false;
	if (!lir_put_materialize_type(
		    cond_tmpvar,
		    lir_expr_proven_type(expr->val.binary.expr[1], block)))
		return false;
	if (!lir_put_opcode(short_op))
		return false;
	if (!lir_put_tmpvar((uint16_t)cond_tmpvar))
		return false;
	patch1 = bytecode_top;
	if (!lir_put_u32(0))
		return false;

	/* Neither operand triggered the short circuit: the "not decided"
	 * result (0 for &&, 1 for ||). */
	if (!lir_put_opcode(OP_ICONST))
		return false;
	if (!lir_put_tmpvar((uint16_t)dst_tmpvar))
		return false;
	if (!lir_put_imm32(is_and ? 1u : 0u))
		return false;
	if (!lir_put_opcode(OP_JMP))
		return false;
	patch_skip = bytecode_top;
	if (!lir_put_u32(0))
		return false;

	/* The short-circuit target: the decided result. */
	decided_addr = bytecode_top;
	if (!lir_put_opcode(OP_ICONST))
		return false;
	if (!lir_put_tmpvar((uint16_t)dst_tmpvar))
		return false;
	if (!lir_put_imm32(is_and ? 0u : 1u))
		return false;

	end_addr = bytecode_top;

	if (!lir_patch_u32(patch0, decided_addr) ||
	    !lir_patch_u32(patch1, decided_addr) ||
	    !lir_patch_u32(patch_skip, end_addr))
		return false;

	lir_decrement_tmpvar(cond_tmpvar);

	return true;
}

/* Visit an ABCE unary expr (PBASE / PLEN). */
static bool
lir_visit_abce_unary_expr(
	int dst_tmpvar,
	struct hir_expr *expr,
	struct hir_block *block)
{
	int opr_tmpvar;
	int opcode;

	assert(expr != NULL);

	/* Visit the operand expr. */
	if (!lir_increment_tmpvar(&opr_tmpvar))
		return false;
	if (!lir_visit_expr(opr_tmpvar, expr->val.unary.expr, block))
		return false;

	opcode = (expr->type == HIR_EXPR_PBASE) ? OP_PBASE : OP_PLEN;
	if (!lir_put_opcode((uint8_t)opcode))
		return false;
	if (!lir_put_tmpvar((uint16_t)dst_tmpvar))
		return false;
	if (!lir_put_tmpvar((uint16_t)opr_tmpvar))
		return false;
	if (opcode == OP_PBASE) {
		uint8_t base_id = pbase_hint_next < 2 ? pbase_hint_next++ : 0xff;
		if (!lir_put_imm8(base_id))
			return false;
	}

	lir_decrement_tmpvar(opr_tmpvar);

	return true;
}

/* Visit an ABCE type-test expr (PCHECK / TYPEIS). */
static bool
lir_visit_abce_typetest_expr(
	int dst_tmpvar,
	struct hir_expr *expr,
	struct hir_block *block)
{
	int opr_tmpvar;
	int opcode;
	int imm;

	assert(expr != NULL);
	assert(expr->val.binary.expr[1]->type == HIR_EXPR_TERM);
	assert(expr->val.binary.expr[1]->val.term.term->type == HIR_TERM_INT);

	/* Visit the value expr. */
	if (!lir_increment_tmpvar(&opr_tmpvar))
		return false;
	if (!lir_visit_expr(opr_tmpvar, expr->val.binary.expr[0], block))
		return false;
	if (!lir_put_materialize_type(
		    opr_tmpvar,
		    lir_expr_proven_type(expr->val.binary.expr[0], block)))
		return false;

	/* The type constant. */
	imm = expr->val.binary.expr[1]->val.term.term->val.i;

	opcode = (expr->type == HIR_EXPR_PCHECK) ? OP_PCHECK : OP_TYPEIS;
	if (!lir_put_opcode((uint8_t)opcode))
		return false;
	if (!lir_put_tmpvar((uint16_t)dst_tmpvar))
		return false;
	if (!lir_put_tmpvar((uint16_t)opr_tmpvar))
		return false;
	if (!lir_put_imm8((uint8_t)imm))
		return false;

	lir_decrement_tmpvar(opr_tmpvar);

	return true;
}

/*
 * Typed-op emission (docs/design/07-typed-ops.md).
 *
 * lir_expr_proven_type() answers "what tag does this expression
 * provably carry at runtime?" using only local, already-computed
 * facts: literals, annotated parameters (sound because OP_CHECKTYPE
 * runs at level >= 1), ABCE typed-int regions (Stage A), and the
 * closure over arithmetic.  TYPED_UNKNOWN is always sound.
 */

#define TYPED_UNKNOWN	(-1)
#define TYPED_INT	NOCT_VALUE_INT
#define TYPED_LONG	NOCT_VALUE_LONG
#define TYPED_FLOAT	NOCT_VALUE_FLOAT
#define TYPED_DOUBLE	NOCT_VALUE_DOUBLE

static int
lir_promote_numeric_type(int a, int b)
{
	if ((a != TYPED_INT && a != TYPED_LONG &&
	     a != TYPED_FLOAT && a != TYPED_DOUBLE) ||
	    (b != TYPED_INT && b != TYPED_LONG &&
	     b != TYPED_FLOAT && b != TYPED_DOUBLE))
		return TYPED_UNKNOWN;
	if (a == TYPED_DOUBLE || b == TYPED_DOUBLE)
		return TYPED_DOUBLE;
	if (a == TYPED_FLOAT || b == TYPED_FLOAT)
		return TYPED_FLOAT;
	if (a == TYPED_LONG || b == TYPED_LONG)
		return TYPED_LONG;
	if (a == TYPED_INT && b == TYPED_INT)
		return TYPED_INT;
	return TYPED_UNKNOWN;
}

static int
lir_symbol_proven_type(
	const char *symbol,
	struct hir_block *block)
{
	struct hir_block *func;
	struct hir_block *b;
	struct hir_local *local;
	bool in_region;

	/* Stage A: an enclosing ABCE fast loop proves int for every
	   local/param read under it (globals cannot appear there, but
	   the local-list check below keeps this safe regardless). */
	in_region = false;
	b = block;
	while (b != NULL && b->type != HIR_BLOCK_FUNC) {
		if (b->type == HIR_BLOCK_FOR && b->val.for_.typed_int_region)
			in_region = true;
		b = b->parent;
	}
	if (b == NULL)
		return TYPED_UNKNOWN;
	func = b;

	/* Globals prove nothing. */
	local = func->val.func.local;
	while (local != NULL) {
		if (strcmp(local->symbol, symbol) == 0)
			break;
		local = local->next;
	}
	if (local == NULL)
		return TYPED_UNKNOWN;

	if (in_region)
		return TYPED_INT;

	/*
	 * Stage B lattice proof (rule 5b), computed by
	 * hir_opt_typed_func().  This subsumes the plain "annotated
	 * parameter" rule 5a: the lattice seeds parameters from their
	 * (CHECKTYPE-backed) annotations and then meets every body
	 * assignment in, so a reassigned parameter correctly loses
	 * its proof -- consulting the annotation directly here would
	 * be unsound (found in review: "func f(a: int) { a = 1.5;
	 * ... }" changes a's runtime tag after the entry check).
	 * Without the optimizer, proven_type stays -1 and no typed op
	 * is ever emitted.
	 */
	if (local->proven_type == NOCT_VALUE_INT)
		return TYPED_INT;
	if (local->proven_type == NOCT_VALUE_LONG)
		return TYPED_LONG;
	if (local->proven_type == NOCT_VALUE_FLOAT)
		return TYPED_FLOAT;
	if (local->proven_type == NOCT_VALUE_DOUBLE)
		return TYPED_DOUBLE;

	return TYPED_UNKNOWN;
}

static int
lir_expr_proven_type(
	struct hir_expr *expr,
	struct hir_block *block)
{
	int a, b;

	switch (expr->type) {
	case HIR_EXPR_TERM:
		switch (expr->val.term.term->type) {
		case HIR_TERM_INT:
			return TYPED_INT;
		case HIR_TERM_LONG:
			return NOCT_VALUE_LONG;
		case HIR_TERM_FLOAT:
			return TYPED_FLOAT;
		case HIR_TERM_DOUBLE:
			return NOCT_VALUE_DOUBLE;
		case HIR_TERM_STRING:
			return NOCT_VALUE_STRING;
		case HIR_TERM_EMPTY_ARRAY:
			return NOCT_VALUE_ARRAY;
		case HIR_TERM_EMPTY_DICT:
			return NOCT_VALUE_DICT;
		case HIR_TERM_SYMBOL:
			return lir_symbol_proven_type(
				expr->val.term.term->val.symbol, block);
		default:
			return TYPED_UNKNOWN;
		}
	case HIR_EXPR_PAR:
		return lir_expr_proven_type(expr->val.unary.expr, block);
	case HIR_EXPR_NEG:
		return lir_expr_proven_type(expr->val.unary.expr, block);
	case HIR_EXPR_NOT:
		a = lir_expr_proven_type(expr->val.unary.expr, block);
		return a == TYPED_UNKNOWN ? TYPED_UNKNOWN : TYPED_INT;
	case HIR_EXPR_CAPTURE:
		/* Yields the inner expression's value. */
		return lir_expr_proven_type(expr->val.capture.expr, block);
	case HIR_EXPR_PLUS:
	case HIR_EXPR_MINUS:
	case HIR_EXPR_MUL:
	case HIR_EXPR_DIV:
		a = lir_expr_proven_type(expr->val.binary.expr[0], block);
		b = lir_expr_proven_type(expr->val.binary.expr[1], block);
		return lir_promote_numeric_type(a, b);
	case HIR_EXPR_MOD:
	case HIR_EXPR_AND:
	case HIR_EXPR_OR:
	case HIR_EXPR_XOR:
	case HIR_EXPR_SHL:
	case HIR_EXPR_SHR:
		a = lir_expr_proven_type(expr->val.binary.expr[0], block);
		b = lir_expr_proven_type(expr->val.binary.expr[1], block);
		if ((a == TYPED_INT || a == TYPED_LONG) &&
		    (b == TYPED_INT || b == TYPED_LONG))
			return (a == TYPED_LONG || b == TYPED_LONG) ?
			       TYPED_LONG : TYPED_INT;
		return TYPED_UNKNOWN;
	case HIR_EXPR_LT:
	case HIR_EXPR_LTE:
	case HIR_EXPR_GT:
	case HIR_EXPR_GTE:
	case HIR_EXPR_EQ:
	case HIR_EXPR_NEQ:
		/* Comparisons yield an int 0/1 when they yield at all;
		   conservatively require proven operands (rule 3). */
		a = lir_expr_proven_type(expr->val.binary.expr[0], block);
		b = lir_expr_proven_type(expr->val.binary.expr[1], block);
		if (a != TYPED_UNKNOWN && b != TYPED_UNKNOWN)
			return TYPED_INT;
		return TYPED_UNKNOWN;
	case HIR_EXPR_PLOAD8U:
	case HIR_EXPR_PLOAD8S:
	case HIR_EXPR_PLOAD16U:
	case HIR_EXPR_PLOAD16S:
	case HIR_EXPR_PLOAD32:
	case HIR_EXPR_PLEN:
	case HIR_EXPR_PCHECK:
	case HIR_EXPR_TYPEIS:
		return TYPED_INT;
	case HIR_EXPR_PLOADF32:
		return TYPED_FLOAT;
	case HIR_EXPR_PLOAD64:
	case HIR_EXPR_PBASE:
		return TYPED_LONG;
	case HIR_EXPR_CALL:
		switch (hir_get_intrinsic_call(expr)) {
		case HIR_INTRINSIC_INT_FROM:
			return TYPED_INT;
		case HIR_INTRINSIC_FLOAT_FROM:
			return TYPED_FLOAT;
		default:
			return TYPED_UNKNOWN;
		}
	case HIR_EXPR_ARRAY:
		return NOCT_VALUE_ARRAY;
	case HIR_EXPR_DICT:
		return NOCT_VALUE_DICT;
	default:
		/* LAND, LOR, DOT, SUBSCR, ordinary CALL, ... */
		return TYPED_UNKNOWN;
	}
}

/*
 * Pick a typed opcode for a binary expression, or -1 for the generic
 * path.  Never called for SHL/SHR (they have a different operand
 * shape and are handled inline in lir_visit_binary_expr).
 */
static int
lir_typed_binary_opcode(
	struct hir_expr *expr,
	struct hir_block *block)
{
	int a, b;
	struct hir_expr *rhs;

	if (lir_optimize_level < 1 || typed_disabled)
		return -1;

	switch (expr->type) {
	case HIR_EXPR_PLUS:
	case HIR_EXPR_MINUS:
	case HIR_EXPR_MUL:
	case HIR_EXPR_DIV:
	case HIR_EXPR_MOD:
	case HIR_EXPR_AND:
	case HIR_EXPR_OR:
	case HIR_EXPR_XOR:
	case HIR_EXPR_LT:
	case HIR_EXPR_LTE:
	case HIR_EXPR_GT:
	case HIR_EXPR_GTE:
		break;
	default:
		return -1;
	}

	a = lir_expr_proven_type(expr->val.binary.expr[0], block);
	b = lir_expr_proven_type(expr->val.binary.expr[1], block);

	if (a == TYPED_INT && b == TYPED_INT) {
		switch (expr->type) {
		case HIR_EXPR_PLUS:	return OP_IADD;
		case HIR_EXPR_MINUS:	return OP_ISUB;
		case HIR_EXPR_MUL:	return OP_IMUL;
		case HIR_EXPR_DIV:
		case HIR_EXPR_MOD:
			/* Keep statically error-free literal divisions on the
			   unchecked fast path.  All other proven-int divisions
			   use the checked typed op rather than generic dispatch. */
			rhs = expr->val.binary.expr[1];
			if (rhs->type == HIR_EXPR_TERM &&
			    rhs->val.term.term->type == HIR_TERM_INT &&
			    rhs->val.term.term->val.i != 0 &&
			    rhs->val.term.term->val.i != -1)
				return expr->type == HIR_EXPR_DIV ? OP_IDIV : OP_IMOD;
			return expr->type == HIR_EXPR_DIV ?
				OP_IDIV_CHECKED : OP_IMOD_CHECKED;
		case HIR_EXPR_AND:	return OP_IAND;
		case HIR_EXPR_OR:	return OP_IOR;
		case HIR_EXPR_XOR:	return OP_IXOR;
		case HIR_EXPR_LT:	return OP_ILT;
		case HIR_EXPR_LTE:	return OP_ILTE;
		case HIR_EXPR_GT:	return OP_IGT;
		case HIR_EXPR_GTE:	return OP_IGTE;
		default:		return -1;
		}
	}
	if (a == TYPED_FLOAT && b == TYPED_FLOAT) {
		switch (expr->type) {
		case HIR_EXPR_PLUS:	return OP_FADD;
		case HIR_EXPR_MINUS:	return OP_FSUB;
		case HIR_EXPR_MUL:	return OP_FMUL;
		case HIR_EXPR_DIV:
			/* IEEE-total after 07 Part 0: no divisor rule. */
			return OP_FDIV;
		case HIR_EXPR_LT:	return OP_FLT;
		case HIR_EXPR_LTE:	return OP_FLTE;
		case HIR_EXPR_GT:	return OP_FGT;
		case HIR_EXPR_GTE:	return OP_FGTE;
		default:		return -1;
		}
	}

	return -1;
}

static bool
lir_visit_binary_expr(
	int dst_tmpvar,
	struct hir_expr *expr,
	struct hir_block *block)
{
	int opr1_tmpvar, opr2_tmpvar;
	int opcode;
	bool typed_op;

	assert(expr != NULL);
	typed_op = false;

	/*
	 * Typed shifts (design 07): different operand shape -- the
	 * shift count is a compile-time immediate in operand 3, so the
	 * count operand is never materialized.  Requirements: proven
	 * int lhs and an int literal count in [0, 31] (an out-of-range
	 * literal stays generic and errors at runtime, unchanged).
	 */
	if ((expr->type == HIR_EXPR_SHL || expr->type == HIR_EXPR_SHR) &&
	    lir_optimize_level >= 1 && !typed_disabled) {
		struct hir_expr *rhs = expr->val.binary.expr[1];
		if (rhs->type == HIR_EXPR_TERM &&
		    rhs->val.term.term->type == HIR_TERM_INT &&
		    rhs->val.term.term->val.i >= 0 &&
		    rhs->val.term.term->val.i <= 31 &&
		    lir_expr_proven_type(expr->val.binary.expr[0], block) == TYPED_INT) {
			if (!lir_increment_tmpvar(&opr1_tmpvar))
				return false;
			if (!lir_visit_expr(opr1_tmpvar, expr->val.binary.expr[0], block))
				return false;
			if (!lir_put_opcode((uint8_t)(expr->type == HIR_EXPR_SHL ?
						      OP_ISHL : OP_ISHR)))
				return false;
			if (!lir_put_tmpvar((uint16_t)dst_tmpvar))
				return false;
			if (!lir_put_tmpvar((uint16_t)opr1_tmpvar))
				return false;
			/* The count is an imm8, NOT a tmpvar: the
			   operand validators reject tmpvar indices
			   beyond the frame size. */
			if (!lir_put_imm8((uint8_t)rhs->val.term.term->val.i))
				return false;
			lir_decrement_tmpvar(opr1_tmpvar);
			typed_emit_int_count++;
			return true;
		}
	}

	/* Visit the operand1 expr. */
	if (!lir_increment_tmpvar(&opr1_tmpvar))
		return false;
	if (!lir_visit_expr(opr1_tmpvar, expr->val.binary.expr[0], block))
		return false;

	/* Visit the operand2 expr. */
	if (!lir_increment_tmpvar(&opr2_tmpvar))
		return false;
	if (!lir_visit_expr(opr2_tmpvar, expr->val.binary.expr[1], block))
		return false;

	/* Typed arithmetic (design 07): same operand shape as the
	   generic ops, so only the opcode differs. */
	opcode = lir_typed_binary_opcode(expr, block);
	if (opcode >= 0) {
		typed_op = true;
		if (opcode == OP_IDIV_CHECKED || opcode == OP_IMOD_CHECKED) {
			typed_emit_int_count++;
			typed_emit_checked_div_count++;
		} else if (opcode >= OP_FADD)
			typed_emit_float_count++;
		else
			typed_emit_int_count++;
		goto put;
	}

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
	case HIR_EXPR_XOR:
	case HIR_EXPR_SHL:
	case HIR_EXPR_SHR:
		if (lir_optimize_level >= 1 && !typed_disabled)
			typed_generic_count++;
		break;
	default:
		break;
	}

	/* Put an opcode. */
	switch (expr->type) {
	case HIR_EXPR_LT:
		opcode = OP_LT;
		break;
	case HIR_EXPR_LTE:
		opcode = OP_LTE;
		break;
	case HIR_EXPR_EQ:
		opcode = OP_EQ;
		break;
	case HIR_EXPR_NEQ:
		opcode = OP_NEQ;
		break;
	case HIR_EXPR_GTE:
		opcode = OP_GTE;
		break;
	case HIR_EXPR_GT:
		opcode = OP_GT;
		break;
	case HIR_EXPR_PLUS:
		opcode = OP_ADD;
		break;
	case HIR_EXPR_MINUS:
		opcode = OP_SUB;
		break;
	case HIR_EXPR_MUL:
		opcode = OP_MUL;
		break;
	case HIR_EXPR_DIV:
		opcode = OP_DIV;
		break;
	case HIR_EXPR_MOD:
		opcode = OP_MOD;
		break;
	case HIR_EXPR_AND:
		opcode = OP_AND;
		break;
	case HIR_EXPR_OR:
		opcode = OP_OR;
		break;
	case HIR_EXPR_XOR:
		opcode = OP_XOR;
		break;
	case HIR_EXPR_SHL:
		opcode = OP_SHL;
		break;
	case HIR_EXPR_SHR:
		opcode = OP_SHR;
		break;
	case HIR_EXPR_SUBSCR:
		opcode = OP_LOADARRAY;
		break;
	case HIR_EXPR_PLOAD8U:
		opcode = OP_PLOAD8U;
		break;
	case HIR_EXPR_PLOAD8S:
		opcode = OP_PLOAD8S;
		break;
	case HIR_EXPR_PLOAD16U:
		opcode = OP_PLOAD16U;
		break;
	case HIR_EXPR_PLOAD16S:
		opcode = OP_PLOAD16S;
		break;
	case HIR_EXPR_PLOAD32:
		opcode = OP_PLOAD32;
		break;
	case HIR_EXPR_PLOAD64:
		opcode = OP_PLOAD64;
		break;
	case HIR_EXPR_PLOADF32:
		opcode = OP_PLOADF32;
		break;
	default:
		opcode = -1;
		assert(NEVER_COME_HERE);
		break;
	}

put:
	if (!typed_op) {
		if (!lir_put_materialize_type(
			    opr1_tmpvar,
			    lir_expr_proven_type(expr->val.binary.expr[0], block)) ||
		    !lir_put_materialize_type(
			    opr2_tmpvar,
			    lir_expr_proven_type(expr->val.binary.expr[1], block)))
			return false;
	}
	if (!lir_put_opcode((uint8_t)opcode))
		return false;
	if (!lir_put_tmpvar((uint16_t)dst_tmpvar))
		return false;
	if (!lir_put_tmpvar((uint16_t)opr1_tmpvar))
		return false;
	if (!lir_put_tmpvar((uint16_t)opr2_tmpvar))
		return false;

	lir_decrement_tmpvar(opr2_tmpvar);
	lir_decrement_tmpvar(opr1_tmpvar);

	return true;
}

static bool
lir_visit_dot_expr(
	int dst_tmpvar,
	struct hir_expr *expr,
	struct hir_block *block)
{
	int opr_tmpvar;

	assert(expr != NULL);
	assert(expr->type == HIR_EXPR_DOT);
	assert(expr->val.dot.obj != NULL);
	assert(expr->val.dot.symbol != NULL);

	/* Visit the operand expr. */
	if (!lir_increment_tmpvar(&opr_tmpvar))
		return false;
	if (!lir_visit_expr(opr_tmpvar, expr->val.dot.obj, block))
		return false;

	/* Put a bytecode sequence. */
	if (!lir_put_opcode(OP_LOADDOT))
		return false;
	if (!lir_put_tmpvar((uint16_t)dst_tmpvar))
		return false;
	if (!lir_put_tmpvar((uint16_t)opr_tmpvar))
		return false;
	if (!lir_put_string(expr->val.dot.symbol))
		return false;

	lir_decrement_tmpvar(opr_tmpvar);

	return true;
}

/*
 * Visit a CSE capture expr (docs/design/05-cse.md): evaluate the
 * inner expression into dst, then copy the value into the home
 * local's slot.  The value semantics is that of the inner expression.
 */
static bool
lir_visit_capture_expr(
	int dst_tmpvar,
	struct hir_expr *expr,
	struct hir_block *block)
{
	int local_index;

	assert(expr != NULL);
	assert(expr->type == HIR_EXPR_CAPTURE);
	assert(expr->val.capture.expr != NULL);
	assert(expr->val.capture.symbol != NULL);

	/* Visit the inner expr. */
	if (!lir_visit_expr(dst_tmpvar, expr->val.capture.expr, block))
		return false;

	/* Copy the value into the home local variable. */
	local_index = lir_get_local_index(block, expr->val.capture.symbol);
	assert(local_index >= 0);
	if (!lir_put_opcode(OP_ASSIGN))
		return false;
	if (!lir_put_tmpvar((uint16_t)local_index))
		return false;
	if (!lir_put_tmpvar((uint16_t)dst_tmpvar))
		return false;

	return true;
}

static bool
lir_visit_call_expr(
	int dst_tmpvar,
	struct hir_expr *expr,
	struct hir_block *block)
{
	int arg_tmpvar[HIR_PARAM_SIZE];
	uint32_t arg_count;
	int func_tmpvar;
	int i;

	assert(expr != NULL);
	assert(expr->type == HIR_EXPR_CALL);
	assert(expr->val.call.func != NULL);
	assert(expr->val.call.arg_count < HIR_PARAM_SIZE);

	arg_count = expr->val.call.arg_count;
	
	/* Visit the func expr. */
	if (!lir_increment_tmpvar(&func_tmpvar))
		return false;
	if (!lir_visit_expr(func_tmpvar, expr->val.call.func, block))
		return false;

	/* Visit the arg exprs. */
	for (i = 0; i < (int)arg_count; i++) {
		if (!lir_increment_tmpvar(&arg_tmpvar[i]))
			return false;
		if (!lir_visit_expr(arg_tmpvar[i], expr->val.call.arg[i], block))
			return false;
	}
	for (i = 0; i < (int)arg_count; i++) {
		if (!lir_put_materialize_type(
			    arg_tmpvar[i],
			    lir_expr_proven_type(expr->val.call.arg[i], block)))
			return false;
	}

	/* Put a bytecode sequence. */
	if (!lir_put_opcode(OP_CALL))
		return false;
	if (!lir_put_tmpvar((uint16_t)dst_tmpvar))
		return false;
	if (!lir_put_tmpvar((uint16_t)func_tmpvar))
		return false;
	if (!lir_put_imm8((uint8_t)arg_count))
		return false;
	for (i = 0; i < (int)arg_count; i++) {
		if (!lir_put_tmpvar((uint16_t)arg_tmpvar[i]))
			return false;
	}

	for (i = (int)arg_count - 1; i >= 0; i--)
		lir_decrement_tmpvar(arg_tmpvar[i]);
	lir_decrement_tmpvar(func_tmpvar);

	return true;
}

static bool
lir_visit_thiscall_expr(
	int dst_tmpvar,
	struct hir_expr *expr,
	struct hir_block *block)
{
	int arg_tmpvar[HIR_PARAM_SIZE];
	int arg_count;
	int obj_tmpvar;
	int func_tmpvar;
	int i;

	assert(expr != NULL);
	assert(expr->type == HIR_EXPR_THISCALL);
	assert(expr->val.thiscall.func != NULL);
	assert(expr->val.thiscall.arg_count < HIR_PARAM_SIZE);

	arg_count = (int)expr->val.thiscall.arg_count;
	
	/* Visit the object expr. */
	if (!lir_increment_tmpvar(&obj_tmpvar))
		return false;
	if (!lir_visit_expr(obj_tmpvar, expr->val.thiscall.obj, block))
		return false;

	/* Resolve and root the callee before evaluating arguments. */
	if (!lir_increment_tmpvar(&func_tmpvar))
		return false;
	if (!lir_put_opcode(OP_LOADDOT))
		return false;
	if (!lir_put_tmpvar((uint16_t)func_tmpvar))
		return false;
	if (!lir_put_tmpvar((uint16_t)obj_tmpvar))
		return false;
	if (!lir_put_string(expr->val.thiscall.func))
		return false;

	/* Visit the arg exprs. */
	for (i = 0; i < arg_count; i++) {
		if (!lir_increment_tmpvar(&arg_tmpvar[i]))
			return false;
		if (!lir_visit_expr(arg_tmpvar[i], expr->val.thiscall.arg[i], block))
			return false;
	}
	for (i = 0; i < arg_count; i++) {
		if (!lir_put_materialize_type(
			    arg_tmpvar[i],
			    lir_expr_proven_type(expr->val.thiscall.arg[i], block)))
			return false;
	}

	/* Put a bytecode sequence. */
	if (!lir_put_opcode(OP_THISCALL))
		return false;
	if (!lir_put_tmpvar((uint16_t)dst_tmpvar))
		return false;
	if (!lir_put_tmpvar((uint16_t)obj_tmpvar))
		return false;
	if (!lir_put_tmpvar((uint16_t)func_tmpvar))
		return false;
	if (!lir_put_imm8((uint8_t)arg_count))
		return false;
	for (i = 0; i < arg_count; i++) {
		if (!lir_put_tmpvar((uint16_t)arg_tmpvar[i]))
			return false;
	}

	for (i = arg_count - 1; i >= 0; i--)
		lir_decrement_tmpvar(arg_tmpvar[i]);
	lir_decrement_tmpvar(func_tmpvar);
	lir_decrement_tmpvar(obj_tmpvar);

	return true;
}

static bool
lir_visit_array_expr(
	int dst_tmpvar,
	struct hir_expr *expr,
	struct hir_block *block)
{
	size_t elem_count, i;
	int build_tmpvar;
	int elem_tmpvar;
	int index_tmpvar;

	assert(expr != NULL);
	assert(expr->type == HIR_EXPR_ARRAY);
	assert(expr->val.array.elem_count > 0);

	elem_count = expr->val.array.elem_count;

	/* Build in a scratch tmpvar; see lir_visit_dict_expr. */
	if (!lir_increment_tmpvar(&build_tmpvar))
		return false;
	/* build_tmpvar is written by EMPTYARRAY directly rather than through
	 * lir_visit_expr(); record the reference definition in the slot meet. */
	lir_note_tmpvar_type(build_tmpvar, NOCT_VALUE_ARRAY);

	/* Create an array. */
	if (!lir_put_opcode(OP_ACONST))
		return false;
	if (!lir_put_tmpvar((uint16_t)build_tmpvar))
		return false;

	/* Push the elements. */
	if (!lir_increment_tmpvar(&elem_tmpvar))
		return false;
	if (!lir_increment_tmpvar(&index_tmpvar))
		return false;
	for (i = 0; i < elem_count; i++) {
		/* Visit the element. */
		if (!lir_visit_expr(elem_tmpvar, expr->val.array.elem[i], block))
			return false;

		/* Add to the array. */
		if (!lir_put_opcode(OP_ICONST))
			return false;
		if (!lir_put_tmpvar((uint16_t)index_tmpvar))
			return false;
		if (!lir_put_imm32((uint32_t)i))
			return false;
		if (!lir_put_materialize_type(
			    index_tmpvar, NOCT_VALUE_INT) ||
		    !lir_put_materialize_type(
			    elem_tmpvar,
			    lir_expr_proven_type(expr->val.array.elem[i], block)))
			return false;
		if (!lir_put_opcode(OP_STOREARRAY))
			return false;
		if (!lir_put_tmpvar((uint16_t)build_tmpvar))
			return false;
		if (!lir_put_tmpvar((uint16_t)index_tmpvar))
			return false;
		if (!lir_put_tmpvar((uint16_t)elem_tmpvar))
			return false;
	}

	/* Move the finished array into dst. */
	if (!lir_put_opcode(OP_ASSIGN))
		return false;
	if (!lir_put_tmpvar((uint16_t)dst_tmpvar))
		return false;
	if (!lir_put_tmpvar((uint16_t)build_tmpvar))
		return false;

	lir_decrement_tmpvar(index_tmpvar);
	lir_decrement_tmpvar(elem_tmpvar);
	lir_decrement_tmpvar(build_tmpvar);

	return true;
}

static bool
lir_visit_dict_expr(
	int dst_tmpvar,
	struct hir_expr *expr,
	struct hir_block *block)
{
	size_t kv_count, i;
	int build_tmpvar;
	int key_tmpvar;
	int value_tmpvar;
	int index_tmpvar;

	assert(expr != NULL);
	assert(expr->type == HIR_EXPR_DICT);

	kv_count = expr->val.dict.kv_count;

	/*
	 * Build the dictionary in a scratch tmpvar, not in dst: dst may
	 * alias a slot the value expressions still read (the $return
	 * slot is parameter 0's slot, so "return {k: param};" would
	 * otherwise store the dictionary into itself).
	 */
	if (!lir_increment_tmpvar(&build_tmpvar))
		return false;
	/* build_tmpvar is written by EMPTYDICT directly rather than through
	 * lir_visit_expr(); record the reference definition in the slot meet. */
	lir_note_tmpvar_type(build_tmpvar, NOCT_VALUE_DICT);
	if (!lir_put_opcode(OP_DCONST))
		return false;
	if (!lir_put_tmpvar((uint16_t)build_tmpvar))
		return false;

	/* Push the elements. */
	if (!lir_increment_tmpvar(&key_tmpvar))
		return false;
	/* key_tmpvar is written by SCONST directly in the loop. */
	lir_note_tmpvar_type(key_tmpvar, NOCT_VALUE_STRING);
	if (!lir_increment_tmpvar(&value_tmpvar))
		return false;
	if (!lir_increment_tmpvar(&index_tmpvar))
		return false;
	for (i = 0; i < kv_count; i++) {
		/* Visit the element. */
		if (!lir_visit_expr(value_tmpvar, expr->val.dict.value[i], block))
			return false;

		/* Add to the dict. */
		if (!lir_put_opcode(OP_SCONST))
			return false;
		if (!lir_put_tmpvar((uint16_t)key_tmpvar))
			return false;
		if (!lir_put_string(expr->val.dict.key[i]))
			return false;
		if (!lir_put_materialize_type(
			    value_tmpvar,
			    lir_expr_proven_type(expr->val.dict.value[i], block)))
			return false;
		if (!lir_put_opcode(OP_STOREARRAY))
			return false;
		if (!lir_put_tmpvar((uint16_t)build_tmpvar))
			return false;
		if (!lir_put_tmpvar((uint16_t)key_tmpvar))
			return false;
		if (!lir_put_tmpvar((uint16_t)value_tmpvar))
			return false;
	}

	/* Move the finished dictionary into dst. */
	if (!lir_put_opcode(OP_ASSIGN))
		return false;
	if (!lir_put_tmpvar((uint16_t)dst_tmpvar))
		return false;
	if (!lir_put_tmpvar((uint16_t)build_tmpvar))
		return false;

	lir_decrement_tmpvar(index_tmpvar);
	lir_decrement_tmpvar(value_tmpvar);
	lir_decrement_tmpvar(key_tmpvar);
	lir_decrement_tmpvar(build_tmpvar);

	return true;
}

static bool
lir_visit_new_expr(
	int dst_tmpvar,
	struct hir_expr *expr,
	struct hir_block *block)
{
	int new_tmpvar, cls_tmpvar, init_tmpvar;

	assert(expr != NULL);
	assert(expr->type == HIR_EXPR_NEW);
	assert(expr->val.new_.cls != NULL);

	/* Load the "new" function. */
	if (!lir_increment_tmpvar(&new_tmpvar))
		return false;
	if (!lir_put_opcode(OP_LOADSYMBOL))
		return false;
	if (!lir_put_tmpvar((uint16_t)new_tmpvar))
		return false;
	if (!lir_put_string("Dict.merge"))
		return false;

	/* Load the class name. */
	if (!lir_increment_tmpvar(&cls_tmpvar))
		return false;
	if (!lir_put_opcode(OP_LOADSYMBOL))
		return false;
	if (!lir_put_tmpvar((uint16_t)cls_tmpvar))
		return false;
	if (!lir_put_string(expr->val.new_.cls))
		return false;

	/* Visit the initializer. */
	if (!lir_increment_tmpvar(&init_tmpvar))
		return false;
	if (expr->val.new_.init != NULL) {
		if (!lir_visit_expr(init_tmpvar, expr->val.new_.init, block))
			return false;
	} else {
		if (!lir_put_opcode(OP_DCONST))
			return false;
		if (!lir_put_tmpvar((uint16_t)init_tmpvar))
			return false;
	}

	/* Put a bytecode sequence. */
	if (!lir_put_opcode(OP_CALL))
		return false;
	if (!lir_put_tmpvar((uint16_t)dst_tmpvar))
		return false;
	if (!lir_put_tmpvar((uint16_t)new_tmpvar))
		return false;
	if (!lir_put_imm8((uint8_t)2))
		return false;
	if (!lir_put_tmpvar((uint16_t)cls_tmpvar))
		return false;
	if (!lir_put_tmpvar((uint16_t)init_tmpvar))
		return false;

	lir_decrement_tmpvar(init_tmpvar);
	lir_decrement_tmpvar(cls_tmpvar);
	lir_decrement_tmpvar(new_tmpvar);

	return true;
}

static bool
lir_visit_term(
	int dst_tmpvar,
	struct hir_term *term,
	struct hir_block *block)
{
	assert(term != NULL);

	switch (term->type) {
	case HIR_TERM_SYMBOL:
		if (!lir_visit_symbol_term(dst_tmpvar, term, block))
			return false;
		break;
	case HIR_TERM_INT:
		if (!lir_visit_int_term(dst_tmpvar, term))
			return false;
		break;
	case HIR_TERM_LONG:
		if (!lir_visit_long_term(dst_tmpvar, term))
			return false;
		break;
	case HIR_TERM_FLOAT:
		if (!lir_visit_float_term(dst_tmpvar, term))
			return false;
		break;
	case HIR_TERM_DOUBLE:
		if (!lir_visit_double_term(dst_tmpvar, term))
			return false;
		break;
	case HIR_TERM_STRING:
		if (!lir_visit_string_term(dst_tmpvar, term))
			return false;
		break;
	case HIR_TERM_EMPTY_ARRAY:
		if (!lir_visit_empty_array_term(dst_tmpvar, term))
			return false;
		break;
	case HIR_TERM_EMPTY_DICT:
		if (!lir_visit_empty_dict_term(dst_tmpvar, term))
			return false;
		break;
	default:
		assert(NEVER_COME_HERE);
		break;
	}

	return true;
}

static bool
lir_visit_symbol_term(
	int dst_tmpvar,
	struct hir_term *term,
	struct hir_block *block)
{
	struct hir_block *func;
	struct hir_local *local;

	assert(term != NULL);
	assert(term->type == HIR_TERM_SYMBOL);

	/* Get a root func block. */
	func = block->parent;
	while (func->type != HIR_BLOCK_FUNC)
		func = func->parent;

	/* Search in a local variable list. */
	local = func->val.func.local;
	while (local != NULL) {
		if (strcmp(local->symbol, term->val.symbol) == 0)
			break;
		local = local->next;
	}

	/* Put an instruction. */
	if (local != NULL) {
		/* The term is an explicit local variable. */
		if (!lir_put_opcode(OP_ASSIGN))
			return false;
		if (!lir_put_tmpvar((uint16_t)dst_tmpvar))
			return false;
		if (!lir_put_tmpvar((uint16_t)local->index))
			return false;
	} else {
		/* The term is not an explicit local variable. */
		if (!lir_put_opcode(OP_LOADSYMBOL))
			return false;
		if (!lir_put_tmpvar((uint16_t)dst_tmpvar))
			return false;
		if (!lir_put_string(term->val.symbol))
			return false;
	}

	return true;
}

static bool
lir_visit_int_term(
	int dst_tmpvar,
	struct hir_term *term)
{
	assert(term != NULL);
	assert(term->type == HIR_TERM_INT);

	if (!lir_put_opcode(OP_ICONST))
		return false;
	if (!lir_put_tmpvar((uint16_t)dst_tmpvar))
		return false;
	if (!lir_put_imm32((uint32_t)term->val.i))
		return false;

	return true;
}

static bool
lir_visit_long_term(
	int dst_tmpvar,
	struct hir_term *term)
{
	assert(term != NULL);
	assert(term->type == HIR_TERM_LONG);

	if (!lir_put_opcode(OP_LICONST))
		return false;
	if (!lir_put_tmpvar((uint16_t)dst_tmpvar))
		return false;
	if (!lir_put_imm64((uint64_t)term->val.l))
		return false;

	return true;
}

static bool
lir_visit_float_term(
	int dst_tmpvar,
	struct hir_term *term)
{
	uint32_t data;

	assert(term != NULL);
	assert(term->type == HIR_TERM_FLOAT);

	data = *(uint32_t *)&term->val.f;

	if (!lir_put_opcode(OP_FCONST))
		return false;
	if (!lir_put_tmpvar((uint16_t)dst_tmpvar))
		return false;
	if (!lir_put_imm32(data))
		return false;

	return true;
}

static bool
lir_visit_double_term(
	int dst_tmpvar,
	struct hir_term *term)
{
	uint64_t data;

	assert(term != NULL);
	assert(term->type == HIR_TERM_DOUBLE);

	data = *(uint64_t *)&term->val.lf;

	if (!lir_put_opcode(OP_LFCONST))
		return false;
	if (!lir_put_tmpvar((uint16_t)dst_tmpvar))
		return false;
	if (!lir_put_imm64(data))
		return false;

	return true;
}

static bool
lir_visit_string_term(
	int dst_tmpvar,
	struct hir_term *term)
{
	assert(term != NULL);
	assert(term->type == HIR_TERM_STRING);

	if (!lir_put_opcode(OP_SCONST))
		return false;
	if (!lir_put_tmpvar((uint16_t)dst_tmpvar))
		return false;
	if (!lir_put_string(term->val.s))
		return false;

	return true;
}

static bool
lir_visit_empty_array_term(
	int dst_tmpvar,
	struct hir_term *term)
{
	UNUSED_PARAMETER(term);

	assert(term != NULL);
	assert(term->type == HIR_TERM_EMPTY_ARRAY);

	if (!lir_put_opcode(OP_ACONST))
		return false;
	if (!lir_put_tmpvar((uint16_t)dst_tmpvar))
		return false;

	return true;
}

static bool
lir_visit_empty_dict_term(
	int dst_tmpvar,
	struct hir_term *term)
{
	UNUSED_PARAMETER(term);

	assert(term != NULL);
	assert(term->type == HIR_TERM_EMPTY_DICT);

	if (!lir_put_opcode(OP_DCONST))
		return false;
	if (!lir_put_tmpvar((uint16_t)dst_tmpvar))
		return false;

	return true;
}

static bool
lir_increment_tmpvar(
	int *tmpvar_index)
{
	if (tmpvar_top >= LIR_TMPVAR_MAX) {
		lir_fatal(N_TR("Too many local variables."));
		return false;
	}

	*tmpvar_index = (int)tmpvar_top;
	tmpvar_lifetime_noted[*tmpvar_index] = false;

	tmpvar_top++;
	if (tmpvar_top > tmpvar_count)
		tmpvar_count = tmpvar_top;

	return true;
}

static bool
lir_decrement_tmpvar(
	int tmpvar_index)
{
	assert(tmpvar_index == (int)tmpvar_top - 1);
	assert(tmpvar_top > 0);
	/* Visitors that write a scratch slot without going through
	 * lir_visit_expr() must explicitly call lir_note_tmpvar_type().
	 * Otherwise that lifetime is dynamic, preventing an earlier fixed
	 * lifetime of the same physical slot from being trusted by the JIT. */
	if (tmpvar_index >= (int)tmpvar_local_count &&
	    !tmpvar_lifetime_noted[tmpvar_index])
		tmpvar_type_state[tmpvar_index] = LIR_TMP_TYPE_DYNAMIC;

	tmpvar_top--;

	return true;
}

static bool
lir_put_opcode(
	uint8_t opcode)
{
	if ((opcode >= OP_VLOADI32X4 && opcode <= OP_VCVTF32I32X4) ||
	    opcode == OP_VORI32X4I || opcode == OP_VFMAF32X4 ||
	    opcode == OP_VCMPI32X4 || opcode == OP_VCMPF32X4 ||
	    opcode == OP_VSELECT128 || opcode == OP_VMINS32X4 ||
	    opcode == OP_VMAXS32X4)
	    /* keep portable masked stores visible to runtime metadata */
	    has_vector_ops = true;
	if (opcode == OP_VMASKSTOREI32X4)
		has_vector_ops = true;
	if (opcode == OP_VINDUCTF32X4 ||
	    opcode == OP_VGATHERI32X4_CHECKED)
		has_vector_ops = true;
	if (opcode == OP_VFMAF32X4)
		has_fma_ops = true;
	if (!lir_put_u8(opcode))
		return false;

	return true;
}

static bool
lir_put_tmpvar(
	uint16_t index)
{
	if (!lir_put_u16(index))
		return false;

	return true;
}

static bool
lir_put_imm8(
	uint8_t imm)
{
	if (!lir_put_u8(imm))
		return false;

	return true;
}


static bool
lir_put_imm32(
	uint32_t imm)
{
	if (!lir_put_u32(imm))
		return false;

	return true;
}

static bool
lir_put_imm64(
	uint64_t imm)
{
	if (!lir_put_u64(imm))
		return false;

	return true;
}

static bool lir_put_branch_addr(
	struct hir_block *block)
{
	assert(block != NULL);

	if (loc_count >= LOC_MAX) {
		lir_fatal(N_TR("Too many jumps."));
		return false;
	}

	loc_tbl[loc_count].type = LOC_BLOCK_TOP;
	loc_tbl[loc_count].offset = (uint32_t)bytecode_top;
	loc_tbl[loc_count].block = block;
	loc_count++;

	bytecode[bytecode_top] = 0xff;
	bytecode[bytecode_top + 1] = 0xff;
	bytecode[bytecode_top + 2] = 0xff;
	bytecode[bytecode_top + 3] = 0xff;
	bytecode_top += 4;

	return true;
}

static bool lir_put_continue_addr(
	struct hir_block *block)
{
	assert(block != NULL);
	assert(block->type == HIR_BLOCK_FOR || block->type == HIR_BLOCK_WHILE);

	if (loc_count >= LOC_MAX) {
		lir_fatal(N_TR("Too many jumps."));
		return false;
	}

	loc_tbl[loc_count].type = LOC_BLOCK_CONTINUE;
	loc_tbl[loc_count].offset = (uint32_t)bytecode_top;
	loc_tbl[loc_count].block = block;
	loc_count++;

	bytecode[bytecode_top] = 0xff;
	bytecode[bytecode_top + 1] = 0xff;
	bytecode[bytecode_top + 2] = 0xff;
	bytecode[bytecode_top + 3] = 0xff;
	bytecode_top += 4;

	return true;
}

static bool
lir_put_absolute_addr(uint32_t addr)
{
	if (loc_count >= LOC_MAX) {
		lir_fatal(N_TR("Too many jumps."));
		return false;
	}
	loc_tbl[loc_count].type = LOC_ABSOLUTE;
	loc_tbl[loc_count].offset = (uint32_t)bytecode_top;
	loc_tbl[loc_count].block = NULL;
	loc_tbl[loc_count].addr = addr;
	loc_count++;
	return lir_put_imm32(addr);
}

static bool
lir_put_string(
	const char *s)
{
	uint32_t len, hash, i;

	/* Put the length. (including NUL)*/
	len = (uint32_t)strlen(s) + 1;
	if (!lir_put_u32(len))
		return false;

	/* Put the hash. */
	hash = noct_string_hash(s);
	if (!lir_put_u32(hash))
		return false;

	/* Put the string. (including NUL terminator) */
	for (i = 0; i < len; i++) {
		if (!lir_put_u8((uint8_t)*s++))
			return false;
	}

	return true;
}

static bool
lir_put_u8(
	uint8_t b)
{
	if (bytecode_top + 1 > BYTECODE_BUF_SIZE)
		return false;

	bytecode[bytecode_top] = b;

	bytecode_top++;

	return true;
}

static bool
lir_put_u16(
	uint16_t b)
{
	if (bytecode_top + 2 > BYTECODE_BUF_SIZE)
		return false;

	/* MSB-first. */
	bytecode[bytecode_top] = (uint8_t)((b >> 8) & 0xff);
	bytecode[bytecode_top + 1] = (uint8_t)(b & 0xff);

	bytecode_top += 2;

	return true;
}

static bool
lir_put_u32(
	uint32_t b)
{
	if (bytecode_top + 4 > BYTECODE_BUF_SIZE)
		return false;

	/* MSB-first. */
	bytecode[bytecode_top] = (uint8_t)((b >> 24) & 0xff);
	bytecode[bytecode_top + 1] = (uint8_t)((b >> 16) & 0xff);
	bytecode[bytecode_top + 2] = (uint8_t)((b >> 8) & 0xff);
	bytecode[bytecode_top + 3] = (uint8_t)(b & 0xff);

	bytecode_top += 4;

	return true;
}

static bool
lir_put_u64(
	uint64_t b)
{
	if (bytecode_top + 8 > BYTECODE_BUF_SIZE)
		return false;

	/* MSB-first. */
	bytecode[bytecode_top] = (uint8_t)((b >> 56) & 0xff);
	bytecode[bytecode_top + 1] = (uint8_t)((b >> 48) & 0xff);
	bytecode[bytecode_top + 2] = (uint8_t)((b >> 40) & 0xff);
	bytecode[bytecode_top + 3] = (uint8_t)((b >> 32) & 0xff);
	bytecode[bytecode_top + 4] = (uint8_t)((b >> 24) & 0xff);
	bytecode[bytecode_top + 5] = (uint8_t)((b >> 16) & 0xff);
	bytecode[bytecode_top + 6] = (uint8_t)((b >> 8) & 0xff);
	bytecode[bytecode_top + 7] = (uint8_t)(b & 0xff);

	bytecode_top += 8;

	return true;
}

static void
patch_block_address(uint32_t prefix)
{
	uint32_t offset, addr;
	int i;

	for (i = 0; i < loc_count; i++) {
		switch (loc_tbl[i].type) {
		case LOC_BLOCK_TOP:
			offset = loc_tbl[i].offset + prefix;
			addr = loc_tbl[i].block->addr + prefix;
			bytecode[offset] = (uint8_t)((addr >> 24) & 0xff);
			bytecode[offset + 1] = (uint8_t)((addr >> 16) & 0xff);
			bytecode[offset + 2] = (uint8_t)((addr >> 8) & 0xff);
			bytecode[offset + 3] = (uint8_t)(addr & 0xff);
			break;
		case LOC_BLOCK_CONTINUE:
			offset = loc_tbl[i].offset + prefix;
			addr = loc_tbl[i].block->cont_addr + prefix;
			bytecode[offset] = (uint8_t)((addr >> 24) & 0xff);
			bytecode[offset + 1] = (uint8_t)((addr >> 16) & 0xff);
			bytecode[offset + 2] = (uint8_t)((addr >> 8) & 0xff);
			bytecode[offset + 3] = (uint8_t)(addr & 0xff);
			break;
		case LOC_ABSOLUTE:
			offset = loc_tbl[i].offset + prefix;
			addr = loc_tbl[i].addr + prefix;
			bytecode[offset] = (uint8_t)((addr >> 24) & 0xff);
			bytecode[offset + 1] = (uint8_t)((addr >> 16) & 0xff);
			bytecode[offset + 2] = (uint8_t)((addr >> 8) & 0xff);
			bytecode[offset + 3] = (uint8_t)(addr & 0xff);
			break;
		default:
			assert(NEVER_COME_HERE);
			break;
		}
	}
}

/*
 * Free a constructed LIR.
 */
void
lir_cleanup(struct lir_func *func)
{
	assert(func != NULL);

	lir_free_func_body(func);
	noct_free(lir_file_name);
	lir_file_name = NULL;
}

/*
 * Get a file name.
 */
const char *
lir_get_file_name(void)
{
	return lir_file_name;
}

/*
 * Get an error line.
 */
int
lir_get_error_line(void)
{
	return lir_error_line;
}

/*
 * Get an error message.
 */
const char *
lir_get_error_message(void)
{
	return lir_error_message;
}

/* Set an error message. */
static void
lir_fatal(
	const char *msg,
	...)
{
	va_list ap;

	va_start(ap, msg);
	vsnprintf(lir_error_message,
		  sizeof(lir_error_message),
		  msg,
		  ap);
	va_end(ap);
}

/* Set an out-of-memory error message. */
static void
lir_out_of_memory(void)
{
	snprintf(lir_error_message,
		 sizeof(lir_error_message),
		 "%s",
		 N_TR("LIR: Out of memory error."));
}

/*
 * Dump
 */

/* IMM 1-byte */
#define IMM1(d) imm1(&pc, &d)
static INLINE void imm1(uint8_t **pc, uint8_t *ret)
{
	*ret = **pc;
	(*pc) += 1;
}

/* IMM 2-byte */
#define IMM2(d) imm2(&pc, &d)
static INLINE void imm2(uint8_t **pc, uint16_t *ret)
{
	uint32_t b0;
	uint32_t b1;

	b0 = **pc;
	b1 = *((*pc) + 1);
	
	*ret = (uint16_t)((b0 << 8) | (b1));

	(*pc) += 2;
}

/* IMM 4-byte */
#define IMM4(d) imm4(&pc, &d)
static INLINE void imm4(uint8_t **pc, uint32_t *ret)
{
	uint32_t b0;
	uint32_t b1;
	uint32_t b2;
	uint32_t b3;

	b0 = **pc;
	b1 = *((*pc) + 1);
	b2 = *((*pc) + 2);
	b3 = *((*pc) + 3);

	*ret = (uint32_t)((b0 << 24) | (b1 << 16) | (b2 << 8) | b3);

	(*pc) += 4;
}

/* IMM 8-byte */
#define IMM8(d) imm8(&pc, &d)
static INLINE void imm8(uint8_t **pc, uint64_t *ret)
{
	uint32_t b0;
	uint32_t b1;
	uint32_t b2;
	uint32_t b3;
	uint32_t b4;
	uint32_t b5;
	uint32_t b6;
	uint32_t b7;

	b0 = **pc;
	b1 = *((*pc) + 1);
	b2 = *((*pc) + 2);
	b3 = *((*pc) + 3);
	b4 = *((*pc) + 4);
	b5 = *((*pc) + 5);
	b6 = *((*pc) + 6);
	b7 = *((*pc) + 7);

	*ret = ((uint64_t)b0 << 56) |
	       ((uint64_t)b1 << 48) |
               ((uint64_t)b2 << 40) |
               ((uint64_t)b3 << 32) |
               ((uint64_t)b4 << 24) |
               ((uint64_t)b5 << 16) |
               ((uint64_t)b6 << 8) |
               ((uint64_t)b7);

	(*pc) += 8;
}

/* IMM string */
#define IMMS(d) imms(&pc, &d)
static INLINE void imms(uint8_t **pc, const char **ret)
{
	(*pc) += 8;
	*ret = (const char *)*pc;
	(*pc) += strlen((const char *)*pc) + 1;
}

void
lir_dump(
	struct lir_func *func)
{
	uint8_t *pc;
	uint8_t *end;

	pc = func->bytecode;
	end = func->bytecode + func->bytecode_size;

	while (pc < end) {
		int opcode;
		int ofs;
		ofs = (int)(ptrdiff_t)(pc - func->bytecode);
		opcode = *pc++;
		switch (opcode) {
		case OP_LINEINFO:
		{
			uint32_t line;
			IMM4(line);
			printf("%04d: LINEINFO(line:%d)\n", ofs, line);
			break;
		}
		case OP_NOP:
			pc++;
			break;
		case OP_ASSIGN:
		{
			uint16_t dst;
			uint16_t src;
			IMM2(dst);
			IMM2(src);
			printf("%04d: ASSIGN(dst:%d, src:%d)\n", ofs, dst, src);
			break;
		}
		case OP_ICONST:
		{
			uint16_t dst;
			uint32_t val;
			IMM2(dst);
			IMM4(val);
			printf("%04d: ICONST(dst:%d, val:%d)\n", ofs, dst, val);
			break;
		}
		case OP_LICONST:
		{
			uint16_t dst;
			uint64_t val;
			IMM2(dst);
			IMM8(val);
			printf("%04d: LICONST(dst:%d, val:%" PRId64 ")\n", ofs, dst, val);
			break;
		}
		case OP_FCONST:
		{
			uint16_t dst;
			uint32_t val = 0;
			float val_f;
			IMM2(dst);
			IMM4(val);
			val_f = *(float *)&val;
			printf("%04d: FCONST(dst:%d, val:%f)\n", ofs, dst, val_f);
			break;
		}
		case OP_LFCONST:
		{
			uint16_t dst;
			uint64_t val = 0;
			double val_f;
			IMM2(dst);
			IMM8(val);
			val_f = *(double *)&val;
			printf("%04d: LFCONST(dst:%d, val:%f)\n", ofs, dst, val_f);
			break;
		}
		case OP_SCONST:
		{
			uint16_t dst;
			const char *val;
			IMM2(dst);
			IMMS(val);
			printf("%04d: SCONST(dst:%d, val:%s)\n", ofs, dst, val);
			break;
		}
		case OP_ACONST:
		{
			uint16_t dst;
			IMM2(dst);
			printf("%04d: ACONST(dst:%d)\n", ofs, dst);
			break;
		}
		case OP_DCONST:
		{
			uint16_t dst;
			IMM2(dst);
			printf("%04d: DCONST(dst:%d)\n", ofs, dst);
			break;
		}
		case OP_INC:
		{
			uint16_t dst;
			uint8_t step;
			IMM2(dst);
			IMM1(step);
			printf("%04d: INC(dst:%d, step:%u)\n", ofs, dst,
			       (unsigned)step);
			break;
		}
		case OP_NOT:
		{
			uint16_t dst;
			uint16_t src;
			IMM2(dst);
			IMM2(src);
			printf("%04d: NOT(dst:%d, src:%d)\n", ofs, dst, src);
			break;
		}
		case OP_NEG:
		{
			uint16_t dst;
			uint16_t src;
			IMM2(dst);
			IMM2(src);
			printf("%04d: NEG(dst:%d, src:%d)\n", ofs, dst, src);
			break;
		}
		case OP_ADD:
		{
			uint16_t dst;
			uint16_t src1;
			uint16_t src2;
			IMM2(dst);
			IMM2(src1);
			IMM2(src2);
			printf("%04d: ADD(dst:%d, src1:%d, src2: %d)\n", ofs, dst, src1, src2);
			break;
		}
		case OP_SUB:
		{
			uint16_t dst;
			uint16_t src1;
			uint16_t src2;
			IMM2(dst);
			IMM2(src1);
			IMM2(src2);
			printf("%04d: SUB(dst:%d, src1:%d, src2: %d)\n", ofs, dst, src1, src2);
			break;
		}
		case OP_MUL:
		{
			uint16_t dst;
			uint16_t src1;
			uint16_t src2;
			IMM2(dst);
			IMM2(src1);
			IMM2(src2);
			printf("%04d: MUL(dst:%d, src1:%d, src2: %d)\n", ofs, dst, src1, src2);
			break;
		}
		case OP_DIV:
		{
			uint16_t dst;
			uint16_t src1;
			uint16_t src2;
			IMM2(dst);
			IMM2(src1);
			IMM2(src2);
			printf("%04d: DIV(dst:%d, src1:%d, src2: %d)\n", ofs, dst, src1, src2);
			break;
		}
		case OP_MOD:
		{
			uint16_t dst;
			uint16_t src1;
			uint16_t src2;
			IMM2(dst);
			IMM2(src1);
			IMM2(src2);
			printf("%04d: MOD(dst:%d, src1:%d, src2: %d)\n", ofs, dst, src1, src2);
			break;
		}
		case OP_AND:
		{
			uint16_t dst;
			uint16_t src1;
			uint16_t src2;
			IMM2(dst);
			IMM2(src1);
			IMM2(src2);
			printf("%04d: AND(dst:%d, src1:%d, src2: %d)\n", ofs, dst, src1, src2);
			break;
		}
		case OP_OR:
		{
			uint16_t dst;
			uint16_t src1;
			uint16_t src2;
			IMM2(dst);
			IMM2(src1);
			IMM2(src2);
			printf("%04d: OR(dst:%d, src1:%d, src2: %d)\n", ofs, dst, src1, src2);
			break;
		}
		case OP_XOR:
		{
			uint16_t dst;
			uint16_t src1;
			uint16_t src2;
			IMM2(dst);
			IMM2(src1);
			IMM2(src2);
			printf("%04d: XOR(dst:%d, src1:%d, src2: %d)\n", ofs, dst, src1, src2);
			break;
		}
		case OP_SHL:
		{
			uint16_t dst;
			uint16_t src1;
			uint16_t src2;
			IMM2(dst);
			IMM2(src1);
			IMM2(src2);
			printf("%04d: SHL(dst:%d, src1:%d, src2: %d)\n", ofs, dst, src1, src2);
			break;
		}
		case OP_SHR:
		{
			uint16_t dst;
			uint16_t src1;
			uint16_t src2;
			IMM2(dst);
			IMM2(src1);
			IMM2(src2);
			printf("%04d: SHR(dst:%d, src1:%d, src2: %d)\n", ofs, dst, src1, src2);
			break;
		}
		case OP_PBASE:
		{
			uint16_t dst;
			uint16_t src;
			uint8_t base_id;
			IMM2(dst);
			IMM2(src);
			IMM1(base_id);
			printf("%04d: PBASE(dst:%d, src:%d, base:%u)\n", ofs,
			       dst, src, (unsigned)base_id);
			break;
		}
		case OP_PLEN:
		{
			uint16_t dst;
			uint16_t src;
			IMM2(dst);
			IMM2(src);
			printf("%04d: PLEN(dst:%d, src:%d)\n", ofs, dst, src);
			break;
		}
		case OP_PCHECK:
		{
			uint16_t dst;
			uint16_t src;
			uint8_t type;
			IMM2(dst);
			IMM2(src);
			IMM1(type);
			printf("%04d: PCHECK(dst:%d, src:%d, type:%d)\n", ofs, dst, src, type);
			break;
		}
		case OP_TYPEIS:
		{
			uint16_t dst;
			uint16_t src;
			uint8_t type;
			IMM2(dst);
			IMM2(src);
			IMM1(type);
			printf("%04d: TYPEIS(dst:%d, src:%d, type:%d)\n", ofs, dst, src, type);
			break;
		}
		case OP_PLOAD8U:
		{
			uint16_t dst;
			uint16_t base;
			uint16_t o;
			IMM2(dst);
			IMM2(base);
			IMM2(o);
			printf("%04d: PLOAD8U(dst:%d, base:%d, ofs:%d)\n", ofs, dst, base, o);
			break;
		}
		case OP_PSTORE8:
		{
			uint16_t base;
			uint16_t o;
			uint16_t src;
			IMM2(base);
			IMM2(o);
			IMM2(src);
			printf("%04d: PSTORE8(base:%d, ofs:%d, src:%d)\n", ofs, base, o, src);
			break;
		}
		case OP_PLOAD8S:
		{
			uint16_t dst;
			uint16_t base;
			uint16_t o;
			IMM2(dst);
			IMM2(base);
			IMM2(o);
			printf("%04d: PLOAD8S(dst:%d, base:%d, ofs:%d)\n", ofs, dst, base, o);
			break;
		}
		case OP_PLOAD16U:
		{
			uint16_t dst;
			uint16_t base;
			uint16_t o;
			IMM2(dst);
			IMM2(base);
			IMM2(o);
			printf("%04d: PLOAD16U(dst:%d, base:%d, ofs:%d)\n", ofs, dst, base, o);
			break;
		}
		case OP_PLOAD16S:
		{
			uint16_t dst;
			uint16_t base;
			uint16_t o;
			IMM2(dst);
			IMM2(base);
			IMM2(o);
			printf("%04d: PLOAD16S(dst:%d, base:%d, ofs:%d)\n", ofs, dst, base, o);
			break;
		}
		case OP_PLOAD32:
		{
			uint16_t dst;
			uint16_t base;
			uint16_t o;
			IMM2(dst);
			IMM2(base);
			IMM2(o);
			printf("%04d: PLOAD32(dst:%d, base:%d, ofs:%d)\n", ofs, dst, base, o);
			break;
		}
		case OP_PLOAD64:
		{
			uint16_t dst;
			uint16_t base;
			uint16_t o;
			IMM2(dst);
			IMM2(base);
			IMM2(o);
			printf("%04d: PLOAD64(dst:%d, base:%d, ofs:%d)\n", ofs, dst, base, o);
			break;
		}
		case OP_PLOADF32:
		{
			uint16_t dst;
			uint16_t base;
			uint16_t o;
			IMM2(dst);
			IMM2(base);
			IMM2(o);
			printf("%04d: PLOADF32(dst:%d, base:%d, ofs:%d)\n", ofs, dst, base, o);
			break;
		}
		case OP_PSTORE16:
		{
			uint16_t base;
			uint16_t o;
			uint16_t src;
			IMM2(base);
			IMM2(o);
			IMM2(src);
			printf("%04d: PSTORE16(base:%d, ofs:%d, src:%d)\n", ofs, base, o, src);
			break;
		}
		case OP_PSTORE32:
		{
			uint16_t base;
			uint16_t o;
			uint16_t src;
			IMM2(base);
			IMM2(o);
			IMM2(src);
			printf("%04d: PSTORE32(base:%d, ofs:%d, src:%d)\n", ofs, base, o, src);
			break;
		}
		case OP_PSTORE64:
		{
			uint16_t base;
			uint16_t o;
			uint16_t src;
			IMM2(base);
			IMM2(o);
			IMM2(src);
			printf("%04d: PSTORE64(base:%d, ofs:%d, src:%d)\n", ofs, base, o, src);
			break;
		}
		case OP_PSTOREF32:
		{
			uint16_t base;
			uint16_t o;
			uint16_t src;
			IMM2(base);
			IMM2(o);
			IMM2(src);
			printf("%04d: PSTOREF32(base:%d, ofs:%d, src:%d)\n", ofs, base, o, src);
			break;
		}
		case OP_CHECKTYPE:
		{
			uint16_t slot;
			uint8_t type;
			IMM2(slot);
			IMM1(type);
			printf("%04d: CHECKTYPE(slot:%d, type:%d)\n", ofs, slot, type);
			break;
		}
		case OP_VLOADI32X4:
		case OP_VSTOREI32X4:
		case OP_VSPLATI32:
		case OP_VGETLANEI32:
		case OP_VMOV128:
		case OP_VADDI32X4:
		case OP_VSUBI32X4:
		case OP_VMULI32X4:
		case OP_VAND128:
		case OP_VOR128:
		case OP_VXOR128:
		case OP_VSHLI32X4:
		case OP_VSHRI32X4:
		case OP_VLOADF32X4:
		case OP_VSTOREF32X4:
		case OP_VSPLATF32:
		case OP_VGETLANEF32:
		case OP_VADDF32X4:
		case OP_VSUBF32X4:
		case OP_VMULF32X4:
		case OP_VDIVF32X4:
		case OP_VCVTI32F32X4:
		case OP_VCVTF32I32X4:
		{
			/* 128-bit SIMD (design 06); operand shapes vary. */
			static const char *vec_name[] = {
				"VLOADI32X4", "VSTOREI32X4", "VSPLATI32",
				"VGETLANEI32", "VMOV128",
				"VADDI32X4", "VSUBI32X4", "VMULI32X4",
				"VAND128", "VOR128", "VXOR128",
				"VSHLI32X4", "VSHRI32X4",
				"VLOADF32X4", "VSTOREF32X4", "VSPLATF32",
				"VGETLANEF32", "VADDF32X4", "VSUBF32X4",
				"VMULF32X4", "VDIVF32X4",
				"VCVTI32F32X4", "VCVTF32I32X4"
			};
			const char *nm = vec_name[opcode - OP_VLOADI32X4];
			uint16_t t1;
			uint16_t t2;
			uint8_t i1;
			uint8_t i2;
			uint8_t i3;
			switch (opcode) {
			case OP_VLOADI32X4:
			case OP_VLOADF32X4:
				IMM1(i1); IMM2(t1); IMM2(t2);
				printf("%04d: %s(vd:%d, base:%d, ofs:%d)\n", ofs, nm, i1, t1, t2);
				break;
			case OP_VSTOREI32X4:
			case OP_VSTOREF32X4:
				IMM2(t1); IMM2(t2); IMM1(i1);
				printf("%04d: %s(base:%d, ofs:%d, vs:%d)\n", ofs, nm, t1, t2, i1);
				break;
			case OP_VSPLATI32:
			case OP_VSPLATF32:
				IMM1(i1); IMM2(t1);
				printf("%04d: %s(vd:%d, src:%d)\n", ofs, nm, i1, t1);
				break;
			case OP_VGETLANEI32:
			case OP_VGETLANEF32:
				IMM2(t1); IMM1(i1); IMM1(i2);
				printf("%04d: %s(dst:%d, vs:%d, lane:%d)\n", ofs, nm, t1, i1, i2);
				break;
			case OP_VMOV128:
			case OP_VCVTI32F32X4:
			case OP_VCVTF32I32X4:
				IMM1(i1); IMM1(i2);
				printf("%04d: %s(vd:%d, vs:%d)\n", ofs, nm, i1, i2);
				break;
			default:
				IMM1(i1); IMM1(i2); IMM1(i3);
				printf("%04d: %s(vd:%d, va:%d, vb:%d)\n", ofs, nm, i1, i2, i3);
				break;
			}
			break;
		}
		case OP_VINDEX_HINT:
		{
			uint16_t index_tmp, stop_tmp, remaining_tmp;
			uint8_t required_vregs, lanes, flags;
			IMM2(index_tmp); IMM2(stop_tmp); IMM2(remaining_tmp);
			IMM1(required_vregs); IMM1(lanes); IMM1(flags);
			printf("%04d: VINDEX_HINT(index:%d, stop:%d, remaining:%d, vregs:%u, lanes:%u, flags:0x%02x)\n",
			       ofs, index_tmp, stop_tmp, remaining_tmp,
			       (unsigned)required_vregs, (unsigned)lanes,
			       (unsigned)flags);
			break;
		}
		case OP_PLOOP_HINT:
		{
			uint16_t index_tmp, stop_tmp, remaining_tmp;
			uint8_t lanes, flags;
			IMM2(index_tmp); IMM2(stop_tmp); IMM2(remaining_tmp);
			IMM1(lanes); IMM1(flags);
			printf("%04d: PLOOP_HINT(index:%d, stop:%d, remaining:%d, lanes:%u, flags:0x%02x)\n",
			       ofs, index_tmp, stop_tmp, remaining_tmp,
			       (unsigned)lanes, (unsigned)flags);
			break;
		}
		case OP_TMPVAR_TYPE:
		{
			uint16_t tmp;
			uint8_t type;
			IMM2(tmp); IMM1(type);
			printf("%04d: TMPVAR_TYPE(tmp:%d, type:%u, compiler:%u)\n",
			       ofs, tmp, (unsigned)(type & 0x7f),
			       (unsigned)((type & TMPVAR_TYPE_COMPILER_TEMP) != 0));
			break;
		}
		case OP_MATERIALIZE_TYPE:
		{
			uint16_t tmp;
			uint8_t type;
			IMM2(tmp); IMM1(type);
			printf("%04d: MATERIALIZE_TYPE(tmp:%d, type:%u)\n",
			       ofs, tmp, (unsigned)type);
			break;
		}
		case OP_SUBJNZ:
		{
			uint16_t value;
			uint8_t decrement;
			uint32_t target;
			IMM2(value); IMM1(decrement); IMM4(target);
			printf("%04d: SUBJNZ(value:%d, decrement:%u, target:%u)\n",
			       ofs, value, (unsigned)decrement, (unsigned)target);
			break;
		}
		case OP_VORI32X4I:
		{
			uint8_t vd, vs, imm, shift;
			IMM1(vd); IMM1(vs); IMM1(imm); IMM1(shift);
			printf("%04d: VORI32X4I(vd:%u, vs:%u, imm:0x%02x, shift:%u)\n",
			       ofs, (unsigned)vd, (unsigned)vs, (unsigned)imm,
			       (unsigned)shift);
			break;
		}
		case OP_VFMAF32X4:
		{
			uint8_t vd, va, vb, vc;
			IMM1(vd); IMM1(va); IMM1(vb); IMM1(vc);
			printf("%04d: VFMAF32X4(vd:%u, va:%u, vb:%u, vc:%u)\n",
			       ofs, (unsigned)vd, (unsigned)va, (unsigned)vb,
			       (unsigned)vc);
			break;
		}
		case OP_VCMPI32X4:
		case OP_VCMPF32X4:
		{
			uint8_t vd, va, vb, pred;
			IMM1(vd); IMM1(va); IMM1(vb); IMM1(pred);
			printf("%04d: %s(vd:%u, va:%u, vb:%u, pred:%u)\n",
			       ofs, opcode == OP_VCMPF32X4 ? "VCMPF32X4" :
			       "VCMPI32X4", (unsigned)vd, (unsigned)va,
			       (unsigned)vb, (unsigned)pred);
			break;
		}
		case OP_VSELECT128:
		{
			uint8_t vd, vm, vt, vf;
			IMM1(vd); IMM1(vm); IMM1(vt); IMM1(vf);
			printf("%04d: VSELECT128(vd:%u, vm:%u, vt:%u, vf:%u)\n",
			       ofs, (unsigned)vd, (unsigned)vm, (unsigned)vt,
			       (unsigned)vf);
			break;
		}
		case OP_VMINS32X4:
		case OP_VMAXS32X4:
		{
			uint8_t vd, va, vb;
			IMM1(vd); IMM1(va); IMM1(vb);
			printf("%04d: %s(vd:%u, va:%u, vb:%u)\n", ofs,
			       opcode == OP_VMINS32X4 ? "VMINS32X4" :
			       "VMAXS32X4", (unsigned)vd, (unsigned)va,
			       (unsigned)vb);
			break;
		}
		case OP_VMASKSTOREI32X4:
		{
			uint16_t base, index;
			uint8_t vs, vm;
			IMM2(base); IMM2(index); IMM1(vs); IMM1(vm);
			printf("%04d: VMASKSTOREI32X4(base:%u, index:%u, vs:%u, vm:%u)\n",
			       ofs, (unsigned)base, (unsigned)index,
			       (unsigned)vs, (unsigned)vm);
			break;
		}
		case OP_VINDUCTF32X4:
		{
			uint8_t vd; uint16_t state, step;
			IMM1(vd); IMM2(state); IMM2(step);
			printf("%04d: VINDUCTF32X4(vd:%u, state:%u, step:%u)\n",
			       ofs, (unsigned)vd, (unsigned)state, (unsigned)step);
			break;
		}
		case OP_VGATHERI32X4_CHECKED:
		{
			uint8_t vd, vi; uint16_t base, plen;
			IMM1(vd); IMM2(base); IMM2(plen); IMM1(vi);
			printf("%04d: VGATHERI32X4_CHECKED(vd:%u, base:%u, plen:%u, vi:%u)\n",
			       ofs, (unsigned)vd, (unsigned)base,
			       (unsigned)plen, (unsigned)vi);
			break;
		}
		case OP_ISHL:
		case OP_ISHR:
		{
			/* Typed shifts (design 07): imm8 count. */
			uint16_t dst;
			uint16_t s1;
			uint8_t imm;
			IMM2(dst);
			IMM2(s1);
			IMM1(imm);
			printf("%04d: %s(dst:%d, src1:%d, imm:%d)\n", ofs,
			       opcode == OP_ISHL ? "ISHL" : "ISHR",
			       dst, s1, imm);
			break;
		}
		case OP_IDIV_CHECKED:
		case OP_IMOD_CHECKED:
		{
			uint16_t dst;
			uint16_t s1;
			uint16_t s2;
			IMM2(dst);
			IMM2(s1);
			IMM2(s2);
			printf("%04d: %s(dst:%d, src1:%d, src2:%d)\n", ofs,
			       opcode == OP_IDIV_CHECKED ?
			       "IDIV_CHECKED" : "IMOD_CHECKED",
			       dst, s1, s2);
			break;
		}
		case OP_IADD:
		case OP_ISUB:
		case OP_IMUL:
		case OP_IDIV:
		case OP_IMOD:
		case OP_IAND:
		case OP_IOR:
		case OP_IXOR:
		case OP_ILT:
		case OP_ILTE:
		case OP_IGT:
		case OP_IGTE:
		case OP_FADD:
		case OP_FSUB:
		case OP_FMUL:
		case OP_FDIV:
		case OP_FLT:
		case OP_FLTE:
		case OP_FGT:
		case OP_FGTE:
		{
			/* Typed arithmetic (design 07). */
			static const char *typed_name[] = {
				"IADD", "ISUB", "IMUL", "IDIV", "IMOD",
				"IAND", "IOR", "IXOR", "ISHL", "ISHR",
				"ILT", "ILTE", "IGT", "IGTE",
				"FADD", "FSUB", "FMUL", "FDIV",
				"FLT", "FLTE", "FGT", "FGTE"
			};
			uint16_t dst;
			uint16_t s1;
			uint16_t s2;
			IMM2(dst);
			IMM2(s1);
			IMM2(s2);
			printf("%04d: %s(dst:%d, src1:%d, src2:%d)\n", ofs,
			       typed_name[opcode - OP_IADD], dst, s1, s2);
			break;
		}
		case OP_LT:
		{
			uint16_t dst;
			uint16_t src1;
			uint16_t src2;
			IMM2(dst);
			IMM2(src1);
			IMM2(src2);
			printf("%04d: LT(dst:%d, src1:%d, src2: %d)\n", ofs, dst, src1, src2);
			break;
		}
		case OP_LTE:
		{
			uint16_t dst;
			uint16_t src1;
			uint16_t src2;
			IMM2(dst);
			IMM2(src1);
			IMM2(src2);
			printf("%04d: LTE(dst:%d, src1:%d, src2: %d)\n", ofs, dst, src1, src2);
			break;
		}
		case OP_GT:
		{
			uint16_t dst;
			uint16_t src1;
			uint16_t src2;
			IMM2(dst);
			IMM2(src1);
			IMM2(src2);
			printf("%04d: GT(dst:%d, src1:%d, src2: %d)\n", ofs, dst, src1, src2);
			break;
		}
		case OP_GTE:
		{
			uint16_t dst;
			uint16_t src1;
			uint16_t src2;
			IMM2(dst);
			IMM2(src1);
			IMM2(src2);
			printf("%04d: GTE(dst:%d, src1:%d, src2:%d)\n", ofs, dst, src1, src2);
			break;
		}
		case OP_EQ:
		{
			uint16_t dst;
			uint16_t src1;
			uint16_t src2;
			IMM2(dst);
			IMM2(src1);
			IMM2(src2);
			printf("%04d: EQ(dst:%d, src1:%d, src2:%d)\n", ofs, dst, src1, src2);
			break;
		}
		case OP_EQI:
		{
			uint16_t dst;
			uint16_t src1;
			uint16_t src2;
			IMM2(dst);
			IMM2(src1);
			IMM2(src2);
			printf("%04d: EQI(dst:%d, src1:%d, src2:%d)\n", ofs, dst, src1, src2);
			break;
		}
		case OP_NEQ:
		{
			uint16_t dst;
			uint16_t src1;
			uint16_t src2;
			IMM2(dst);
			IMM2(src1);
			IMM2(src2);
			printf("%04d: NEQ(dst:%d, src1:%d, src2: %d)\n", ofs, dst, src1, src2);
			break;
		}
		case OP_LOADARRAY:
		{
			uint16_t dst;
			uint16_t src1;
			uint16_t src2;
			IMM2(dst);
			IMM2(src1);
			IMM2(src2);
			printf("%04d: LOADARRAY(dst:%d, arr:%d, subsc:%d)\n", ofs, dst, src1, src2);
			break;
		}
		case OP_STOREARRAY:
		{
			uint16_t dst;
			uint16_t src1;
			uint16_t src2;
			IMM2(dst);
			IMM2(src1);
			IMM2(src2);
			printf("%04d: STOREARRAY(arr:%d, subsc:%d, val:%d)\n", ofs, dst, src1, src2);
			break;
		}
		case OP_LEN:
		{
			uint16_t dst;
			uint16_t src;
			IMM2(dst);
			IMM2(src);
			printf("%04d: LEN(dst:%d, src:%d)\n", ofs, dst, src);
			break;
		}
		case OP_GETDICTKEYBYINDEX:
		{
			uint16_t dst;
			uint16_t dict;
			uint16_t index;
			IMM2(dst);
			IMM2(dict);
			IMM2(index);
			printf("%04d: GETDICTKEYBYINDEX(dst:%d, dict:%d, index:%d)\n", ofs, dst, dict, index);
			break;
		}
		case OP_GETDICTVALBYINDEX:
		{
			uint16_t dst;
			uint16_t dict;
			uint16_t index;
			IMM2(dst);
			IMM2(dict);
			IMM2(index);
			printf("%04d: GETDICTKEYBYINDEX(dst:%d, dict:%d, index:%d)\n", ofs, dst, dict, index);
			break;
		}
		case OP_STOREDOT:
		{
			const char *symbol;
			uint16_t obj, src;
			IMM2(obj);
			IMMS(symbol);
			IMM2(src);
			printf("%04d: STOREDOT(obj:%d, symbol:%s, src:%d)\n", ofs, obj, symbol, src);
			break;
		}
		case OP_LOADDOT:
		{
			const char *symbol;
			uint16_t dst, obj;
			IMM2(dst);
			IMM2(obj);
			IMMS(symbol);
			printf("%04d: LOADDOT(dst: %d, obj:%d, symbol:%s)\n", ofs, dst, obj, symbol);
			break;
		}
		case OP_STORESYMBOL:
		{
			const char *symbol;
			uint16_t src;
			IMMS(symbol);
			IMM2(src);
			printf("%04d: STORESYMBOL(symbol:%s, src:%d)\n", ofs, symbol, src);
			break;
		}
		case OP_LOADSYMBOL:
		{
			uint16_t dst;
			const char *symbol;
			IMM2(dst);
			IMMS(symbol);
			printf("%04d: LOADSYMBOL(src: %d, symbol:%s)\n", ofs, dst, symbol);
			break;
		}
		case OP_CALL:
		{
			uint16_t dst;
			uint16_t func;
			uint8_t arg_count;
			uint16_t arg;
			int i;
			IMM2(dst);
			IMM2(func);
			IMM1(arg_count);
			printf("%04d: CALL(dst: %d, arg_count:%d", ofs, dst, arg_count);
			for (i = 0; i < arg_count; i++) {
				IMM2(arg);
				printf(", %d", arg);
			}
			printf(")\n");
			break;
		}
		case OP_THISCALL:
		{
			uint16_t dst;
			uint16_t obj;
			uint16_t func;
			uint8_t arg_count;
			uint16_t arg;
			int i;
			IMM2(dst);
			IMM2(obj);
			IMM2(func);
			IMM1(arg_count);
			printf("%04d: THISCALL(dst: %d, obj: %d,arg_count:%d", ofs, dst, obj, arg_count);
			for (i = 0; i < arg_count; i++) {
				IMM2(arg);
				printf(", %d", arg);
			}
			printf(")\n");
			break;
		}
		case OP_JMP:
		{
			uint32_t target;
			IMM4(target);
			printf("%04d: JMP(target:%d)\n", ofs, target);
			break;
		}
		case OP_JMPIFTRUE:
		{
			uint16_t src;
			uint32_t target;
			IMM2(src);
			IMM4(target);
			printf("%04d: JMPIFTRUE(src:%d, target:%d)\n", ofs, src, target);
			break;
		}
		case OP_JMPIFFALSE:
		{
			uint16_t src;
			uint32_t target;
			IMM2(src);
			IMM4(target);
			printf("%04d: JMPIFFALSE(src:%d, target:%d)\n", ofs, src, target);
			break;
		}
		case OP_JMPIFEQ:
		{
			uint16_t src;
			uint32_t target;
			IMM2(src);
			IMM4(target);
			printf("%04d: JMPIFEQ(src:%d, target:%d)\n", ofs, src, target);
			break;
		}
		case OP_SAFEPOINT:
		{
			printf("%04d: SAFEPOINT()\n", ofs);
			break;
		}
		default:
		{
			printf("Unknown Opcode: 0x%x\n", opcode);
			assert(INVALID_OPCODE);
			break;
		}
		}
	}
}

/* Release an LIR function without changing diagnostic state. */
static void
lir_free_func_body(
	struct lir_func *func)
{
	uint32_t i;

	if (func == NULL)
		return;

	noct_free(func->func_name);

	/* Release every possibly constructed parameter name. */
	for (i = 0; i < LIR_PARAM_SIZE; i++)
		noct_free(func->param_name[i]);

	noct_free(func->file_name);
	noct_free(func->bytecode);
#if defined(NOCT_USE_OPTIMIZER)
	fast_info_free(func->fast_info);
#endif
	noct_free(func);
}
