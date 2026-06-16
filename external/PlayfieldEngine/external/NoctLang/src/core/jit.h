/* -*- coding: utf-8; tab-width: 8; indent-tabs-mode: t; -*- */

/*
 * Noct Programming Language
 * Copyright (c) 2025, 2026, Awe Morris
 */

/*
 * JIT (mips32): Just-In-Time native code generation
 */

#ifndef NOCT_JIT_H
#define NOCT_JIT_H

#include <noct/c89compat.h>
#include "bytecode.h"

#include <string.h>

/* Default JIT threshold. */
#define JIT_DEFAULT_THRESHOLD	5

#define ex_make_string_with_hash noct_ex_make_string_with_hash
#define ex_make_empty_array noct_ex_make_empty_array
#define ex_make_empty_dict noct_ex_make_empty_dict
#define ex_add_helper noct_ex_add_helper
#define ex_sub_helper noct_ex_sub_helper
#define ex_mul_helper noct_ex_mul_helper
#define ex_div_helper noct_ex_div_helper
#define ex_mod_helper noct_ex_mod_helper
#define ex_and_helper noct_ex_and_helper
#define ex_or_helper noct_ex_or_helper
#define ex_xor_helper noct_ex_xor_helper
#define ex_shl_helper noct_ex_shl_helper
#define ex_shr_helper noct_ex_shr_helper
#define ex_neg_helper noct_ex_neg_helper
#define ex_not_helper noct_ex_not_helper
#define ex_lt_helper noct_ex_lt_helper
#define ex_lte_helper noct_ex_lte_helper
#define ex_eq_helper noct_ex_eq_helper
#define ex_neq_helper noct_ex_neq_helper
#define ex_gte_helper noct_ex_gte_helper
#define ex_gt_helper noct_ex_gt_helper
#define ex_storearray_helper noct_ex_storearray_helper
#define ex_loadarray_helper noct_ex_loadarray_helper
#define ex_len_helper noct_ex_len_helper
#define ex_getdictkeybyindex_helper noct_ex_getdictkeybyindex_helper
#define ex_getdictvalbyindex_helper noct_ex_getdictvalbyindex_helper
#define ex_loadsymbol_helper noct_ex_loadsymbol_helper
#define ex_storesymbol_helper noct_ex_storesymbol_helper
#define ex_loaddot_helper noct_ex_loaddot_helper
#define ex_storedot_helper noct_ex_storedot_helper
#define ex_call_helper noct_ex_call_helper
#define ex_thiscall_helper noct_ex_thiscall_helper
#define ex_safepoint_helper noct_ex_safepoint_helper

/* Generate a JIT-compiled code for a function. */
bool
jit_build(
	struct rt_env *env,
	struct rt_func *func);

/* Commit written code. */
void
jit_commit(
	struct rt_env *env);

/* Free all JIT-compiled code. */
void
jit_free(
	struct rt_env *env);

/*
 * If JIT is enabled.
 */
#if defined(NOCT_USE_JIT)

/* Error message. */
#define BROKEN_BYTECODE		N_TR("Broken bytecode.")

/* Code size. */
#if !defined(NOCT_TARGET_DOS4G)
#define JIT_CODE_MAX		(16 * 1024 * 1024)
#else
#define JIT_CODE_MAX		(1 * 1024 * 1024)
#endif

/* PC entry size. */
#define PC_ENTRY_MAX		2048

/* Branch pathch size. */
#define BRANCH_PATCH_MAX	2048

/*
 * JIT codegen context
 */
struct jit_context {
	struct rt_env *env;
	struct rt_func *func;

	/* Top of the mapped code area. */
	void *code_top;

	/* End of the mapped code area. */
	void *code_end;

	/* Current code position in the mapped code area. */
	void *code;

	/* Exception handler address of the current function. */
	void *exception_code;

	/* Current PC of LIR. */
	uint32_t lpc;

	/* Mapping table from LIR-PC to Native-PC. */
	struct pc_entry {
		/* LIR-PC */
		uint32_t lpc;

		/* Native-PC */
		uint32_t *code;
	} pc_entry[PC_ENTRY_MAX];
	uint32_t pc_entry_count;

	/* Delayed branch patching table. */
	struct branch_patch {
		/* Native code address. */
		uint32_t *code;

		/* LIR-PC */
		uint32_t lpc;

		/* Branch type. */
		int type;
	} branch_patch[BRANCH_PATCH_MAX];
	int branch_patch_count;
};

/* Map a region. */
bool jit_map_memory_region(void **region, size_t size);

/* Unmap a region. */
void jit_unmap_memory_region(void *region, size_t size);

/* Make a region writable. */
void jit_map_writable(void *region, size_t size);

/* Make a region executable. */
void jit_map_executable(void * region, size_t size);

/*
 * Get an opcode.
 */
#define CONSUME_OPCODE(d)	if (!jit_get_opcode(ctx, &d)) return false
static INLINE bool
jit_get_opcode(
	struct jit_context *ctx,
	uint8_t *opcode)
{
	if (ctx->lpc + 1 > ctx->func->bytecode_size) {
		rt_error(ctx->env, BROKEN_BYTECODE);
		*opcode = 0;
		return false;
	}

	*opcode = ctx->func->bytecode[ctx->lpc];

	ctx->lpc++;

	return true;
}

/*
 * Get an imm32 operand.
 */
#define CONSUME_IMM32(d)	if (!jit_get_opr_imm32(ctx, &d)) return false
static INLINE bool
jit_get_opr_imm32(
	struct jit_context *ctx,
	uint32_t *d)
{
	if (ctx->lpc + 4 > ctx->func->bytecode_size) {
		rt_error(ctx->env, BROKEN_BYTECODE);
		*d = 0;
		return false;
	}

	*d = ((uint32_t)ctx->func->bytecode[ctx->lpc] << 24) |
	     (uint32_t)(ctx->func->bytecode[ctx->lpc + 1] << 16) |
	     (uint32_t)(ctx->func->bytecode[ctx->lpc + 2] << 8) |
	     (uint32_t)ctx->func->bytecode[ctx->lpc + 3];

	ctx->lpc += 4;

	return true;
}

/*
 * Get an imm64 operand.
 */
#define CONSUME_IMM64(d)	if (!jit_get_opr_imm64(ctx, &d)) return false
static INLINE bool
jit_get_opr_imm64(
	struct jit_context *ctx,
	uint64_t *d)
{
	if (ctx->lpc + 8 > ctx->func->bytecode_size) {
		rt_error(ctx->env, BROKEN_BYTECODE);
		*d = 0;
		return false;
	}

	*d = ((uint64_t)ctx->func->bytecode[ctx->lpc + 0] << 56) |
	     ((uint64_t)ctx->func->bytecode[ctx->lpc + 1] << 48) |
	     ((uint64_t)ctx->func->bytecode[ctx->lpc + 2] << 40) |
	     ((uint64_t)ctx->func->bytecode[ctx->lpc + 3] << 32) |
	     ((uint64_t)ctx->func->bytecode[ctx->lpc + 4] << 24) |
	     ((uint64_t)ctx->func->bytecode[ctx->lpc + 5] << 16) |
	     ((uint64_t)ctx->func->bytecode[ctx->lpc + 6] << 8) |
             ((uint64_t)ctx->func->bytecode[ctx->lpc + 7]);

	ctx->lpc += 8;

	return true;
}

/*
 * Get an imm16 operand that represents tmpvar index.
 */
#define CONSUME_TMPVAR(d)	if (!jit_get_opr_tmpvar(ctx, &d)) return false
static INLINE bool
jit_get_opr_tmpvar(
	struct jit_context *ctx,
	int *d)
{
	if (ctx->lpc + 2 > ctx->func->bytecode_size) {
		rt_error(ctx->env, BROKEN_BYTECODE);
		*d = 0;
		return false;
	}

	*d = (ctx->func->bytecode[ctx->lpc] << 8) |
	      ctx->func->bytecode[ctx->lpc + 1];
	if ((uint32_t)*d >= ctx->func->tmpvar_size) {
		rt_error(ctx->env, BROKEN_BYTECODE);
		return false;
	}

	ctx->lpc += 2;

	return true;
}

/*
 * Get an imm8 operand.
 */
#define CONSUME_IMM8(d)		if (!jit_get_imm8(ctx, &d)) return false
static INLINE bool
jit_get_imm8(
	struct jit_context *ctx,
	int *imm8)
{
	if (ctx->lpc + 1 > ctx->func->bytecode_size) {
		rt_error(ctx->env, BROKEN_BYTECODE);
		*imm8 = 0;
		return false;
	}

	*imm8 = ctx->func->bytecode[ctx->lpc];

	ctx->lpc++;

	return true;
}

/*
 * Get a string operand.
 */
#define CONSUME_STRING(s,l,h)	if (!jit_get_opr_string(ctx, &s, &l, &h)) return false
static INLINE bool
jit_get_opr_string(
	struct jit_context *ctx,
	const char **s,
	uint32_t *len,
	uint32_t *hash)
{
	if (ctx->lpc + 8 > ctx->func->bytecode_size) {
		rt_error(ctx->env, BROKEN_BYTECODE);
		*s = 0;
		return false;
	}

	*len = ((uint32_t)ctx->func->bytecode[ctx->lpc] << 24) |
		(uint32_t)(ctx->func->bytecode[ctx->lpc + 1] << 16) |
		(uint32_t)(ctx->func->bytecode[ctx->lpc + 2] << 8) |
		(uint32_t)ctx->func->bytecode[ctx->lpc + 3];

	*hash = ((uint32_t)ctx->func->bytecode[ctx->lpc + 4] << 24) |
		(uint32_t)(ctx->func->bytecode[ctx->lpc + 5] << 16) |
		(uint32_t)(ctx->func->bytecode[ctx->lpc + 6] << 8) |
		(uint32_t)ctx->func->bytecode[ctx->lpc + 7];

	if (ctx->lpc + 8 + *len > ctx->func->bytecode_size) {
		rt_error(ctx->env, BROKEN_BYTECODE);
		*s = NULL;
		return false;
	}

	*s = (const char *)&ctx->func->bytecode[ctx->lpc + 8];

	ctx->lpc += 8 + *len;

	return true;
}

#endif /* defined(NOCT_USE_JIT) */

#endif
