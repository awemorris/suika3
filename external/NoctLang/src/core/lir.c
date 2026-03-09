/* -*- coding: utf-8; tab-width: 8; indent-tabs-mode: t; -*- */

/*
 * Copyright (c) 2025, Awe Morris. All rights reserved.
 */

/*
 * LIR: Low-level intermediate representation
 */

#include <noct/noct.h>
#include "lir.h"
#include "hir.h"
#include "bytecode.h"
#include "hash.h"

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
#undef DEBUG_DUMP_LIR

/*
 * Config
 */
extern int noct_conf_optimize;

/*
 * Target LIR.
 */

#define BYTECODE_BUF_SIZE	65536

/* Bytecode array. */
static uint8_t bytecode[BYTECODE_BUF_SIZE];

/* Cuurent bytecode length. */
static int bytecode_top;

/*
 * Variable table.
 */

#define TMPVAR_MAX	128

static int tmpvar_top;
static int tmpvar_count;

/*
 * Relocation table.
 */

/* Maximum relocation count */
#define LOC_MAX	1024

/* Relocation type */
#define LOC_BLOCK_TOP		0
#define LOC_BLOCK_INCREMENTER	1

struct loc_entry {
	/* Type. */
	int type;

	/* Location offset. */
	uint32_t offset;

	/* Branch target. */
	struct hir_block *block;
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
 * Forward declaration.
 */
static int lir_count_local(struct hir_block *func);
static bool lir_visit_block(struct hir_block *block);
static bool lir_visit_basic_block(struct hir_block *block);
static bool lir_check_succ_loop_head(struct hir_block *block, struct hir_block **loop);
static bool lir_visit_if_block(struct hir_block *block);
static bool lir_visit_for_block(struct hir_block *block);
static bool lir_visit_for_range_block(struct hir_block *block);
static bool lir_visit_for_kv_block(struct hir_block *block);
static bool lir_visit_for_v_block(struct hir_block *block);
static int lir_get_local_index(struct hir_block *block, const char *symbol);
static bool lir_visit_while_block(struct hir_block *block);
static bool lir_visit_stmt(struct hir_block *block, struct hir_stmt *stmt);
static bool lir_check_lhs_local(struct hir_block *block, struct hir_expr *lhs, int *rhs_tmpvar);
static bool lir_visit_expr(int dst_tmpvar, struct hir_expr *expr, struct hir_block *block);
static bool lir_visit_unary_expr(int dst_tmpvar, struct hir_expr *expr, struct hir_block *block);
static bool lir_visit_binary_expr(int dst_tmpvar, struct hir_expr *expr, struct hir_block *block);
static bool lir_visit_dot_expr(int dst_tmpvar, struct hir_expr *expr, struct hir_block *block);
static bool lir_visit_call_expr(int dst_tmpvar, struct hir_expr *expr, struct hir_block *block);
static bool lir_visit_thiscall_expr(int dst_tmpvar, struct hir_expr *expr, struct hir_block *block);
static bool lir_visit_array_expr(int dst_tmpvar, struct hir_expr *expr, struct hir_block *block);
static bool lir_visit_dict_expr(int dst_tmpvar, struct hir_expr *expr, struct hir_block *block);
static bool lir_visit_new_expr(int dst_tmpvar, struct hir_expr *expr, struct hir_block *block);
static bool lir_visit_term(int dst_tmpvar, struct hir_term *term, struct hir_block *block);
static bool lir_visit_symbol_term(int dst_tmpvar, struct hir_term *term, struct hir_block *block);
static bool lir_visit_int_term(int dst_tmpvar, struct hir_term *term);
static bool lir_visit_float_term(int dst_tmpvar, struct hir_term *term);
static bool lir_visit_string_term(int dst_tmpvar, struct hir_term *term);
static bool lir_visit_empty_array_term(int dst_tmpvar, struct hir_term *term);
static bool lir_visit_empty_dict_term(int dst_tmpvar, struct hir_term *term);
static bool lir_increment_tmpvar(int *tmpvar_index);
static bool lir_decrement_tmpvar(int tmpvar_index);
static bool lir_put_opcode(uint8_t op);
static bool lir_put_tmpvar(uint16_t index);
static bool lir_put_imm8(uint8_t imm);
static bool lir_put_imm32(uint32_t imm);
static bool lir_put_string(const char *data);
static bool lir_put_branch_addr(struct hir_block *block);
static bool lir_put_incrementer_addr(struct hir_block *block);
static bool lir_put_u8(uint8_t b);
static bool lir_put_u16(uint16_t b);
static bool lir_put_u32(uint32_t b);
static void patch_block_address(void);
static void lir_fatal(const char *msg, ...);
static void lir_out_of_memory(void);

/*
 * Build
 */

bool
lir_build(
	struct hir_block *hir_func,
	struct lir_func **lir_func)
{
	struct hir_block *cur_block;
	int i;

	assert(hir_func != NULL);
	assert(hir_func->type == HIR_BLOCK_FUNC);

	/* Copy the file name. */
	lir_file_name = noct_strdup(hir_func->val.func.file_name);
	if (lir_file_name == NULL) {
		lir_out_of_memory();
		return false;
	}

	/* Initialize the bytecode buffer. */
	bytecode_top = 0;
	memset(bytecode, 0, BYTECODE_BUF_SIZE);

	/* Initialize the tmpvars. */
	tmpvar_top = lir_count_local(hir_func);
	if (tmpvar_top == 0) {
		/* For the return value. */
		tmpvar_top = 1;
	}
	tmpvar_count = tmpvar_top;

	/* Initialize the relocation table. */
	loc_count = 0;

	/* Visit blocks. */
	cur_block = hir_func->val.func.inner;
	while (cur_block != NULL) {
		/* Visit a block. */
		lir_visit_block(cur_block);

		/* Move to a next. */
		if (cur_block->stop) {
			assert(cur_block->succ->type == HIR_BLOCK_END);
			cur_block->succ->addr = (uint32_t)bytecode_top;
			break;
		}
		cur_block = cur_block->succ;
	}

	/* Patch block address. */
	patch_block_address();

	/* Make an lir_func. */
	*lir_func = noct_malloc(sizeof(struct lir_func));
	if (lir_func == NULL) {
		lir_out_of_memory();
		return false;
	}

	/* Copy the function name. */
	(*lir_func)->func_name = noct_strdup(hir_func->val.func.name);
	if ((*lir_func)->func_name == NULL) {
		lir_out_of_memory();
		return false;
	}

	/* Copy the parameter names.  */
	(*lir_func)->param_count = hir_func->val.func.param_count;
	for (i = 0; i < hir_func->val.func.param_count; i++) {
		(*lir_func)->param_name[i] = noct_strdup(hir_func->val.func.param_name[i]);
		if ((*lir_func)->param_name[i] == NULL) {
			lir_out_of_memory();
			return false;
		}
	}

	/* Copy the bytecode. */
	(*lir_func)->bytecode = noct_malloc((size_t)bytecode_top);
	if ((*lir_func)->bytecode == NULL) {
		lir_out_of_memory();
		return false;
	}
	(*lir_func)->bytecode_size = bytecode_top;
	memcpy((*lir_func)->bytecode, bytecode, (size_t)bytecode_top);

	/* Copy the file name. */
	(*lir_func)->file_name = noct_strdup(hir_func->val.func.file_name);
	if ((*lir_func)->file_name == NULL) {
		lir_out_of_memory();
		return false;
	}

	(*lir_func)->tmpvar_size = tmpvar_count + 1;

#ifdef DEBUG_DUMP_LIR
	lir_dump(*lir_func);
#endif

	return true;
}

/* Count the number of the local variables in a func. */
static int
lir_count_local(
	struct hir_block *func)
{
	struct hir_local *local;
	int count;

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

#if 0
	/* Put a line number. */
	if (conf_optimize == 0) {
		if (!lir_put_opcode(LOP_LINEINFO))
			return false;
		if (!lir_put_imm32((uint32_t)block->line))
			return false;
	}
#endif

	/* Visit statements. */
	stmt = block->val.basic.stmt_list;
	while (stmt != NULL) {
		/* Visit a statement. */
		if (!lir_visit_stmt(block, stmt))
			return false;
		stmt = stmt->next;
	}

	/*
	 * If the block is the tail of the siblings, and not the end of
	 * loop or function, i.e., there is a break, continue, or return.
	 */
	if (block->stop &&
	    (block->parent->type != HIR_BLOCK_FOR &&
	     block->parent->type != HIR_BLOCK_WHILE &&
	     block->parent->type != HIR_BLOCK_FUNC)) {
		struct hir_block *loop;

		/* Check if succ is a loop head. */
		if (lir_check_succ_loop_head(block, &loop)) {
			/* Continue edge. */
			if (!lir_put_opcode(OP_JMP))
				return false;
			if (!lir_put_incrementer_addr(loop))
				return false;
		} else {
			/* Break or return edge. */
			if (!lir_put_opcode(OP_JMP))
				return false;
			if (!lir_put_branch_addr(block->succ))
				return false;
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
	if (noct_conf_optimize == 0) {
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
	if (block->val.for_.is_ranged) {
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

static bool
lir_visit_for_range_block(
	struct hir_block *block)
{
	uint32_t loop_addr;
	int start_tmpvar, stop_tmpvar, loop_tmpvar, cmp_tmpvar;
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
	if (noct_conf_optimize == 0) {
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

	/* Put the start value to a loop variable. */
	loop_tmpvar = lir_get_local_index(block, block->val.for_.counter_symbol);
	if (!lir_put_opcode(OP_ASSIGN))
		return false;
	if (!lir_put_tmpvar((uint16_t)loop_tmpvar))
		return false;
	if (!lir_put_tmpvar((uint16_t)start_tmpvar))
		return false;

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

	/* Store the incrementer address. */
	block->val.for_.inc_addr = (uint32_t)bytecode_top;

	/* Increment the loop variable. */
	if (!lir_put_opcode(OP_INC))
		return false;
	if (!lir_put_tmpvar((uint16_t)loop_tmpvar))
		return false;

	/* Put a back-edge jump. */
	if (!lir_put_opcode(OP_JMP))
		return false;
	if (!lir_put_imm32(loop_addr))
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
	if (noct_conf_optimize == 0) {
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
	if (!lir_put_imm32(loop_addr))
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
	if (noct_conf_optimize == 0) {
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
	if (!lir_put_imm32(loop_addr))
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
	if (noct_conf_optimize == 0) {
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
	if (!lir_put_opcode(OP_JMPIFFALSE))
		return false;
	if (!lir_put_tmpvar((uint16_t)cmp_tmpvar))
		return false;
	if (!lir_put_branch_addr(block->succ))
		return false;
	lir_decrement_tmpvar(cmp_tmpvar);

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
	if (!lir_put_imm32(loop_addr))
		return false;

	return true;
}

static bool
lir_visit_stmt(
	struct hir_block *parent,
	struct hir_stmt *stmt)
{
	int rhs_tmpvar, obj_tmpvar, access_tmpvar;
	bool is_lhs_local;

	assert(stmt != NULL);
	assert(stmt->rhs != NULL);

	/* Put a line number. */
	if (noct_conf_optimize == 0) {
		if (!lir_put_opcode(OP_LINEINFO))
			return false;
		if (!lir_put_imm32((uint32_t)stmt->line))
			return false;
	}

	/* Check whether LHS is a local variable. */
	is_lhs_local = lir_check_lhs_local(parent, stmt->lhs, &rhs_tmpvar);

	/* Prepare a tmpvar for RHS if LHS is not an explicit local variable. */
	if (!is_lhs_local) {
		if (!lir_increment_tmpvar(&rhs_tmpvar))
			return false;
	}

	/* Visit RHS. */
	if (!lir_visit_expr(rhs_tmpvar, stmt->rhs, parent))
		return false;

	/* Visit LHS if LHS is not an explicit local variable. */
	if (stmt->lhs != NULL && !is_lhs_local) {
		if (stmt->lhs->type == HIR_EXPR_TERM) {
			assert(stmt->lhs->val.term.term->type == HIR_TERM_SYMBOL);

			/* Put a storesymbol. */
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
		} else if (stmt->lhs->type == HIR_EXPR_DOT) {
			assert(stmt->lhs->val.dot.obj != NULL);
			assert(stmt->lhs->val.dot.symbol != NULL);

			/* Visit an object. */
			if (!lir_increment_tmpvar(&obj_tmpvar))
				return false;
			if (!lir_visit_expr(obj_tmpvar, stmt->lhs->val.dot.obj, parent))
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
		/* For the binary operators. */
		if (!lir_visit_binary_expr(dst_tmpvar, expr, block))
			return false;
		break;
	case HIR_EXPR_DOT:
		/* For the dot operator. */
		if (!lir_visit_dot_expr(dst_tmpvar, expr, block))
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

static bool
lir_visit_binary_expr(
	int dst_tmpvar,
	struct hir_expr *expr,
	struct hir_block *block)
{
	int opr1_tmpvar, opr2_tmpvar;
	int opcode;

	assert(expr != NULL);

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
	default:
		opcode = -1;
		assert(NEVER_COME_HERE);
		break;
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

static bool
lir_visit_call_expr(
	int dst_tmpvar,
	struct hir_expr *expr,
	struct hir_block *block)
{
	int arg_tmpvar[HIR_PARAM_SIZE];
	int arg_count;
	int func_tmpvar;
	int i;

	assert(expr != NULL);
	assert(expr->type == HIR_EXPR_CALL);
	assert(expr->val.call.func != NULL);
	assert(expr->val.call.arg_count >= 0);
	assert(expr->val.call.arg_count < HIR_PARAM_SIZE);

	arg_count = expr->val.call.arg_count;
	
	/* Visit the func expr. */
	if (!lir_increment_tmpvar(&func_tmpvar))
		return false;
	if (!lir_visit_expr(func_tmpvar, expr->val.call.func, block))
		return false;

	/* Visit the arg exprs. */
	for (i = 0; i < arg_count; i++) {
		if (!lir_increment_tmpvar(&arg_tmpvar[i]))
			return false;
		if (!lir_visit_expr(arg_tmpvar[i], expr->val.call.arg[i], block))
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
	for (i = 0; i < arg_count; i++) {
		if (!lir_put_tmpvar((uint16_t)arg_tmpvar[i]))
			return false;
	}

	for (i = arg_count - 1; i >= 0; i--)
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
	int i;

	assert(expr != NULL);
	assert(expr->type == HIR_EXPR_THISCALL);
	assert(expr->val.thiscall.func != NULL);
	assert(expr->val.thiscall.arg_count >= 0);
	assert(expr->val.thiscall.arg_count < HIR_PARAM_SIZE);

	arg_count = expr->val.thiscall.arg_count;
	
	/* Visit the object expr. */
	if (!lir_increment_tmpvar(&obj_tmpvar))
		return false;
	if (!lir_visit_expr(obj_tmpvar, expr->val.thiscall.obj, block))
		return false;

	/* Visit the arg exprs. */
	for (i = 0; i < arg_count; i++) {
		if (!lir_increment_tmpvar(&arg_tmpvar[i]))
			return false;
		if (!lir_visit_expr(arg_tmpvar[i], expr->val.thiscall.arg[i], block))
			return false;
	}

	/* Put a bytecode sequence. */
	if (!lir_put_opcode(OP_THISCALL))
		return false;
	if (!lir_put_tmpvar((uint16_t)dst_tmpvar))
		return false;
	if (!lir_put_tmpvar((uint16_t)obj_tmpvar))
		return false;
	if (!lir_put_string(expr->val.thiscall.func))
		return false;
	if (!lir_put_imm8((uint8_t)arg_count))
		return false;
	for (i = 0; i < arg_count; i++) {
		if (!lir_put_tmpvar((uint16_t)arg_tmpvar[i]))
			return false;
	}

	for (i = arg_count - 1; i >= 0; i--)
		lir_decrement_tmpvar(arg_tmpvar[i]);
	lir_decrement_tmpvar(obj_tmpvar);

	return true;
}

static bool
lir_visit_array_expr(
	int dst_tmpvar,
	struct hir_expr *expr,
	struct hir_block *block)
{
	int elem_count;
	int elem_tmpvar;
	int index_tmpvar;
	int i;

	assert(expr != NULL);
	assert(expr->type == HIR_EXPR_ARRAY);
	assert(expr->val.array.elem_count > 0);

	elem_count = expr->val.array.elem_count;
	
	/* Create an array. */
	if (!lir_put_opcode(OP_ACONST))
		return false;
	if (!lir_put_tmpvar((uint16_t)dst_tmpvar))
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
		if (!lir_put_opcode(OP_STOREARRAY))
			return false;
		if (!lir_put_tmpvar((uint16_t)dst_tmpvar))
			return false;
		if (!lir_put_tmpvar((uint16_t)index_tmpvar))
			return false;
		if (!lir_put_tmpvar((uint16_t)elem_tmpvar))
			return false;
	}

	lir_decrement_tmpvar(index_tmpvar);
	lir_decrement_tmpvar(elem_tmpvar);

	return true;
}

static bool
lir_visit_dict_expr(
	int dst_tmpvar,
	struct hir_expr *expr,
	struct hir_block *block)
{
	int kv_count;
	int key_tmpvar;
	int value_tmpvar;
	int index_tmpvar;
	int i;

	assert(expr != NULL);
	assert(expr->type == HIR_EXPR_DICT);
	assert(expr->val.dict.kv_count >= 0);

	kv_count = expr->val.dict.kv_count;
	
	/* Create a dictionary. */
	if (!lir_put_opcode(OP_DCONST))
		return false;
	if (!lir_put_tmpvar((uint16_t)dst_tmpvar))
		return false;

	/* Push the elements. */
	if (!lir_increment_tmpvar(&key_tmpvar))
		return false;
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
		if (!lir_put_opcode(OP_STOREARRAY))
			return false;
		if (!lir_put_tmpvar((uint16_t)dst_tmpvar))
			return false;
		if (!lir_put_tmpvar((uint16_t)key_tmpvar))
			return false;
		if (!lir_put_tmpvar((uint16_t)value_tmpvar))
			return false;
	}

	lir_decrement_tmpvar(index_tmpvar);
	lir_decrement_tmpvar(value_tmpvar);
	lir_decrement_tmpvar(key_tmpvar);

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
	if (!lir_put_string("new"))
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
	case HIR_TERM_FLOAT:
		if (!lir_visit_float_term(dst_tmpvar, term))
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
	if (tmpvar_top >= TMPVAR_MAX) {
		lir_fatal(N_TR("Too many local variables."));
		return false;
	}

	*tmpvar_index = tmpvar_top;

	tmpvar_top++;
	if (tmpvar_top > tmpvar_count)
		tmpvar_count = tmpvar_top;

	return true;
}

static bool
lir_decrement_tmpvar(
	int tmpvar_index)
{
	assert(tmpvar_index == tmpvar_top - 1);
	assert(tmpvar_top > 0);

	tmpvar_top--;

	return true;
}

static bool
lir_put_opcode(
	uint8_t opcode)
{
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

static bool lir_put_incrementer_addr(
	struct hir_block *block)
{
	assert(block != NULL);
	assert(block->type == HIR_BLOCK_FOR);

	if (loc_count >= LOC_MAX) {
		lir_fatal(N_TR("Too many jumps."));
		return false;
	}

	loc_tbl[loc_count].type = LOC_BLOCK_INCREMENTER;
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
lir_put_string(
	const char *s)
{
	uint32_t len, hash, i;

	/* Put the length. (including NUL)*/
	len = (uint32_t)strlen(s) + 1;
	if (!lir_put_u32(len))
		return false;

	/* Put the hash. */
	hash = string_hash(s);
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

	bytecode[bytecode_top] = (uint8_t)((b >> 24) & 0xff);
	bytecode[bytecode_top + 1] = (uint8_t)((b >> 16) & 0xff);
	bytecode[bytecode_top + 2] = (uint8_t)((b >> 8) & 0xff);
	bytecode[bytecode_top + 3] = (uint8_t)(b & 0xff);

	bytecode_top += 4;

	return true;
}

static void
patch_block_address(void)
{
	uint32_t offset, addr;
	int i;

	for (i = 0; i < loc_count; i++) {
		switch (loc_tbl[i].type) {
		case LOC_BLOCK_TOP:
			offset = loc_tbl[i].offset;
			addr = loc_tbl[i].block->addr;
			bytecode[offset] = (uint8_t)((addr >> 24) & 0xff);
			bytecode[offset + 1] = (uint8_t)((addr >> 16) & 0xff);
			bytecode[offset + 2] = (uint8_t)((addr >> 8) & 0xff);
			bytecode[offset + 3] = (uint8_t)(addr & 0xff);
			break;
		case LOC_BLOCK_INCREMENTER:
			offset = loc_tbl[i].offset;
			addr = loc_tbl[i].block->val.for_.inc_addr;
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
	int i;

	assert(func != NULL);

	noct_free(func->func_name);
	for (i = 0; i < func->param_count; i++)
		noct_free(func->param_name[i]);
	noct_free(func->bytecode);
	memset(func, 0, sizeof(struct lir_func));
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

#ifdef DEBUG_DUMP_LIR

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

	*ret = (uint32_t)((b0 << 24) | (b1 << 16) | (b2 << 8) | (b3 + 3));

	(*pc) += 4;
}

/* IMM string */
#define IMMS(d) imms(&pc, &d)
static INLINE void imms(uint8_t **pc, const char **ret)
{
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
			IMM2(dst);
			printf("%04d: INC(dst:%d)\n", ofs, dst);
			break;
		}
		//case OP_NEG:
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
		//case OP_SUB:
		//case OP_MUL:
		//case OP_DIV:
		//case OP_MOD:
		//case OP_AND:
		//case OP_OR:
		//case OP_XOR:
		//case OP_LT:
		//case OP_LTE:
		//case OP_GT:
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
		//case OP_NEQ:
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
		//case OP_STOREDOT:
		//case OP_LOADDOT:
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
		//case OP_THISCALL:
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
		default:
			assert(INVALID_OPCODE);
			break;
		}
	}
}

#endif /* DEBUG_DUMP_LIR */
