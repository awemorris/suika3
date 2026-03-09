/* -*- coding: utf-8; tab-width: 8; indent-tabs-mode: t; -*- */

/*
 * Copyright (c) 2025, Awe Morris. All rights reserved.
 */

/*
 * JIT (riscv32): Just-In-Time native code generation
 */

#include <noct/c89compat.h>	/* NOCT_ARCH_RISCV32 */

#if defined(NOCT_ARCH_RISCV32) && defined(NOCT_USE_JIT)

#include "runtime.h"
#include "jit.h"
#include "execution.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

/* False asseretion */
#define JIT_OP_NOT_IMPLEMENTED	0
#define NEVER_COME_HERE		0

/* Branch patch type */
#define PATCH_JAL		0
#define PATCH_BEQ		1
#define PATCH_BNE		2

/* Generated code. */
static uint32_t *jit_code_region;
static uint32_t *jit_code_region_cur;
static uint32_t *jit_code_region_tail;

/* Write mapped? */
static bool is_writable;

/* Forward declaration */
static bool jit_visit_bytecode(struct jit_context *ctx);
static bool jit_patch_branch(struct jit_context *ctx, int patch_index);

/*
 * Generate a JIT-compiled code for a function.
 */
bool
jit_build(
	  struct rt_env *env,
	  struct rt_func *func)
{
	struct jit_context ctx;
	int i;

	/* If the first call, map a memory region for the generated code. */
	if (jit_code_region == NULL) {
		if (!jit_map_memory_region((void **)&jit_code_region, JIT_CODE_MAX)) {
			rt_error(env, N_TR("Memory mapping failed."));
			return false;
		}
		jit_code_region_cur = jit_code_region;
		jit_code_region_tail = jit_code_region + JIT_CODE_MAX / 4;
		is_writable = true;
	}

	/* Make a context. */
	memset(&ctx, 0, sizeof(struct jit_context));
	ctx.code_top = jit_code_region_cur;
	ctx.code_end = jit_code_region_tail;
	ctx.code = ctx.code_top;
	ctx.env = env;
	ctx.func = func;

	/* Make code writable and non-executable. */
	if (!is_writable) {
		jit_map_writable(jit_code_region, JIT_CODE_MAX);
		is_writable = true;
	}

	/* Visit over the bytecode. */
	if (!jit_visit_bytecode(&ctx))
		return false;

	jit_code_region_cur = ctx.code;

	/* Patch branches. */
	for (i = 0; i < ctx.branch_patch_count; i++) {
		if (!jit_patch_branch(&ctx, i))
			return false;
	}

	func->jit_code = (bool (*)(struct rt_env *))ctx.code_top;

	return true;
}

/*
 * Free all JIT-compiled code.
 */
void
jit_free(
	 struct rt_env *env)
{
	UNUSED_PARAMETER(env);

	if (jit_code_region != NULL) {
		jit_unmap_memory_region(jit_code_region, JIT_CODE_MAX);

		jit_code_region = NULL;
		jit_code_region_cur = NULL;
		jit_code_region_tail = NULL;
	}
}

/*
 * Commit written code.
 */
void
jit_commit(
	struct rt_env *env)
{
	/* Make code executable and non-writable. */
	jit_map_executable(jit_code_region, JIT_CODE_MAX);

	is_writable = false;
}

/*
 * Assembler output functions
 */

/* Decoration */
#define ASM

/* Registers */
#define REG_ZERO	0
#define REG_RA		1
#define REG_SP		2
#define REG_GP		3
#define REG_TP		4
#define REG_T0		5
#define REG_T1		6
#define REG_T2		7
#define REG_S0		8
#define REG_S1		9
#define REG_A0		10
#define REG_A1		11
#define REG_A2		12
#define REG_A3		13
#define REG_A4		14
#define REG_A5		15
#define REG_A6		16
#define REG_A7		17
#define REG_S2		18
#define REG_S3		19
#define REG_S4		20
#define REG_S5		21
#define REG_S6		22
#define REG_S7		23
#define REG_S8		24
#define REG_S9		25
#define REG_S10		26
#define REG_S11		27
#define REG_T3		28
#define REG_T4		29
#define REG_T5		30
#define REG_T6		31

/* Immediate */
#define IMM5(v)		((uint32_t)((v) & 0x1f))
#define IMM9(v)		((uint32_t)((v) & 0x1ff))
#define IMM12(v)	((uint32_t)((v) & 0xfff))
#define IMM13(v)	((uint32_t)((v) & 0x1fff))
#define IMM21(v)	((uint32_t)((v) & 0x1fffff))
#define IMM32(v)	((uint32_t)(v))

/* Put a instruction word. */
static INLINE bool
jit_put_word(
	struct jit_context *ctx,
	uint32_t word)
{
	if (ctx->code >= ctx->code_end) {
		rt_error(ctx->env, "Code too big.");
		return false;
	}

	*(uint32_t *)ctx->code = word;
	ctx->code = (uint32_t *)ctx->code + 1;

	return true;
}

/* MV */
#define MV(rd, rs)	if (!jit_put_mv(ctx, rd, rs)) return false
static INLINE bool
jit_put_mv(
	struct jit_context *ctx,
	uint32_t rd,
	uint32_t rs)
{
	if (!jit_put_word(ctx,
			  (0 << 25) |		/* funct7 */
			  (rs << 20) |		/* rs2 */
			  (0 << 15) |		/* rs1 */
			  (6 << 12) |		/* funct3 */
			  (rd << 7) |		/* rd */
			  0x33))		/* opcode */
		return false;
	return true;
}

/* ADD */
#define ADD(rd, rs1, rs2)	if (!jit_put_add(ctx, rd, rs1, rs2)) return false
static INLINE bool
jit_put_add(
	struct jit_context *ctx,
	uint32_t rd,
	uint32_t rs1,
	uint32_t rs2)
{
	if (!jit_put_word(ctx,
			  (0 << 25) |		/* funct7 */
			  (rs2 << 20) |
			  (rs1 << 15) |
			  (0 << 12) |		/* funct3 */
			  (rd << 7) |
			  0x33))
		return false;
	return true;
}

/* ADDI */
#define ADDI(rd, rs, imm)	if (!jit_put_addi(ctx, rd, rs, imm)) return false
static INLINE bool
jit_put_addi(
	struct jit_context *ctx,
	uint32_t rd,
	uint32_t rs,
	uint32_t imm)
{
	if (!jit_put_word(ctx,
			  (imm << 20) |		/* imm */
			  (rs << 15) |		/* rs1 */
			  (0 << 12) |		/* funct3 */
			  (rd << 7) |		/* rd */
			  0x13))		/* opcode */
		return false;
	return true;
}

/* ORI */
#define ORI(rd, rs, imm)	if (!jit_put_ori(ctx, rd, rs, imm)) return false
static INLINE bool
jit_put_ori(
	struct jit_context *ctx,
	uint32_t rd,
	uint32_t rs,
	uint32_t imm)
{
	if (!jit_put_word(ctx,
			  (imm << 20) |		/* imm */
			  (rs << 15) |		/* rs1 */
			  (6 << 12) |		/* funct3 */
			  (rd << 7) |		/* rd */
			  0x13))		/* opcode */
		return false;
	return true;
}

/* SLLI */
#define SLLI(rd, rs, imm)	if (!jit_put_slli(ctx, rd, rs, imm)) return false
static INLINE bool
jit_put_slli(
	struct jit_context *ctx,
	uint32_t rd,
	uint32_t rs,
	uint32_t imm)
{
	if (!jit_put_word(ctx,
			  (0 << 25) |		/* funct7 */
			  (imm << 20) |		/* shamt */
			  (rs << 15) |
			  (1 << 12) |		/* funct3 */
			  (rd << 7) |
			  0x13))
		return false;
	return true;
}

/* SW */
#define SW(rs2, imm, rs1)	if (!jit_put_sw(ctx, rs2, imm, rs1)) return false
static INLINE bool
jit_put_sw(
	struct jit_context *ctx,
	uint32_t rs2,
	uint32_t imm,
	uint32_t rs1)
{
	if (!jit_put_word(ctx,
			  (((imm & 0xfff) >> 5) << 25) |	/* imm[11:5] */
			  (rs2 << 20) |				/* rs2 */
			  (rs1 << 15) |				/* rs1 */
			  (2 << 12) |				/* funct3 */
			  ((imm & 0x1f) << 7) |			/* imm[4:0] */
			  0x23)) 				/* opcode */
		return false;
	return true;
}

/* LW */
#define LW(rd, imm, rs)		if (!jit_put_lw(ctx, rd, imm, rs)) return false
static INLINE bool
jit_put_lw(
	struct jit_context *ctx,
	uint32_t rd,
	uint32_t imm,
	uint32_t rs)
{
	if (!jit_put_word(ctx,
			  ((imm & 0xfff) << 20) |	/* imm[11:0] */
			  (rs << 15) |			/* rs */
			  (2 << 12) |			/* funct3 */
			  (rd << 7) |			/* rd */
			  0x03)) 			/* opcode */
		return false;
	return true;
}

/* JAL */
#define JAL(rd, imm)	if (!jit_put_jal(ctx, rd, imm)) return false
static INLINE bool
jit_put_jal(
	struct jit_context *ctx,
	uint32_t rd,
	uint32_t imm)
{
	uint32_t imm_20    = (imm & 0x100000) >> 20;
	uint32_t imm_19_12 = (imm & 0x0ff000) >> 12;
	uint32_t imm_11    = (imm & 0x000800) >> 11;
	uint32_t imm_10_1  = (imm & 0x0007fe) >> 1;
	if (!jit_put_word(ctx,
			  (imm_20 << 31) |
			  (imm_10_1 << 21) |
			  (imm_11 << 20) |
			  (imm_19_12 << 12) |
			  (rd << 7) |
			  0x6f))
		return false;
	return true;
}

/* JALR */
#define JALR(rd, imm, rs)	if (!jit_put_jalr(ctx, rd, imm, rs)) return false
static INLINE bool
jit_put_jalr(
	struct jit_context *ctx,
	uint32_t rd,
	uint32_t imm,
	uint32_t rs)
{
	if (!jit_put_word(ctx,
			  ((imm & 0xfff) << 20) |	/* imm */
			  (rs << 15) |			/* rs1 */
			  (0 << 12) |			/* funct3 */
			  (rd << 7) |			/* rd */
			  0x67)) 			/* opcode */
		return false;
	return true;
}

/* BEQ */
#define BEQ(rs1, rs2, rel)	if (!jit_put_beq(ctx, rs1, rs2, rel)) return false
static INLINE bool
jit_put_beq(
	struct jit_context *ctx,
	uint32_t rs1,
	uint32_t rs2,
	uint32_t rel)    
{
	uint32_t imm_12   = (rel & 0x1000) >> 12;
	uint32_t imm_11   = (rel & 0x0800) >> 11;
	uint32_t imm_10_5 = (rel & 0x07e0) >> 5;
	uint32_t imm_4_1  = (rel & 0x001e) >> 1;
	if (!jit_put_word(ctx,
			  (imm_12 << 31) |
			  (imm_10_5 << 25) |
			  (rs2 << 20) |
			  (rs1 << 15) |
			  (0 << 12) |		/* funct3 */
			  (imm_4_1 << 8) |
			  (imm_11 << 7) |
			  0x63))
		return false;
	return true;
}

/* BNE */
#define BNE(rs1, rs2, rel)	if (!jit_put_bne(ctx, rs1, rs2, rel)) return false
static INLINE bool
jit_put_bne(
	struct jit_context *ctx,
	uint32_t rs1,
	uint32_t rs2,
	uint32_t rel)    
{
	uint32_t imm_12   = (rel & 0x1000) >> 12;
	uint32_t imm_11   = (rel & 0x0800) >> 11;
	uint32_t imm_10_5 = (rel & 0x07e0) >> 5;
	uint32_t imm_4_1  = (rel & 0x001e) >> 1;
	if (!jit_put_word(ctx,
			  (imm_12 << 31) |
			  (imm_10_5 << 25) |
			  (rs2 << 20) |
			  (rs1 << 15) |
			  (1 << 12) |		/* funct3 */
			  (imm_4_1 << 8) |
			  (imm_11 << 7) |
			  0x63))
		return false;
	return true;
}

/* LI_32 */
#define LI_32(rd, imm)	if (!jit_put_li32(ctx, rd, imm)) return false
static INLINE bool
jit_put_li32(
	struct jit_context *ctx,
	uint32_t rd,
	uint32_t imm)
{
	/* auipc rd, o */
	if (!jit_put_word(ctx,
			  ((0 & 0xfffff) >> 12) |	/* imm */
			  (rd << 7) |			/* rd */
			  0x17))			/* opcode */
		return false;

	LW(rd, 12, rd);
	JAL(REG_ZERO, IMM21(8));

	/* .dowrd imm */
	if (!jit_put_word(ctx, imm))
		return false;

	return true;
}

/* ret */
#define RET()	JALR(REG_ZERO, IMM12(0), REG_RA)


/*
 * Templates
 */

#define ASM_BINARY_OP(f)											\
	ASM {													\
		/* s10: env */											\
		/* s11: &env->frame->tmpvar[0] */								\
														\
		/* Arg1 a0: env */										\
		MV	(REG_A0, REG_S10);									\
														\
		/* Arg2 a1: dst */										\
		ORI	(REG_A1, REG_ZERO, IMM12(dst));								\
														\
		/* Arg3 a2: src1 */										\
		ORI	(REG_A2, REG_ZERO, IMM12(src1));							\
														\
		/* Arg4 a3: src2 */										\
		ORI	(REG_A3, REG_ZERO, IMM12(src2));							\
														\
		/* Call f(). */											\
		LI_32	(REG_T0, IMM32((uint32_t)f));								\
		JALR	(REG_RA, IMM12(0), REG_T0);								\
														\
		/* If failed: */										\
		BEQ	(REG_A0, REG_ZERO, IMM13((uint32_t)ctx->exception_code - (uint32_t)ctx->code));		\
	}

#define ASM_UNARY_OP(f)												\
	ASM {													\
		/* s10: env */											\
		/* s11: &env->frame->tmpvar[0] */								\
														\
		/* Arg1 a0: env */										\
		MV	(REG_A0, REG_S10);									\
														\
		/* Arg2 a1: dst */										\
		ORI	(REG_A1, REG_ZERO, IMM12(dst));								\
														\
		/* Arg3 a2: src */										\
		ORI	(REG_A2, REG_ZERO, IMM12(src));								\
														\
		/* Call f(). */											\
		LI_32	(REG_T0, IMM32((uint32_t)f));								\
		JALR	(REG_RA, IMM12(0), REG_T0);								\
														\
		/* If failed: */										\
		BEQ	(REG_A0, REG_ZERO, IMM13((uint32_t)ctx->exception_code - (uint32_t)ctx->code));		\
	}

/*
 * Bytecode visitors
 */

/* Visit a OP_LINEINFO instruction. */
static INLINE bool
jit_visit_lineinfo_op(
	struct jit_context *ctx)
{
	uint32_t line;

	CONSUME_IMM32(line);

	ASM {
		/* s10: env */
		/* s11: &env->frame->tmpvar[0] */

		/* env->line = line; */
		ORI	(REG_T0, REG_ZERO, IMM12(line & 0xfff));
		SW	(REG_T0, 8, REG_S10);
	}

	return true;
}

/* Visit a OP_ASSIGN instruction. */
static INLINE bool
jit_visit_assign_op(
	struct jit_context *ctx)
{
	int dst;
	int src;

	CONSUME_TMPVAR(dst);
	CONSUME_TMPVAR(src);

	dst *= (int)sizeof(struct rt_value);
	src *= (int)sizeof(struct rt_value);

	/* env->frame->tmpvar[dst] = env->frame->tmpvar[src]; */
	ASM {
		/* s10: env */
		/* s11: &env->frame->tmpvar[0] */

		/* t0 = dst_addr = &evn->frame->tmpvar[dst] */
		ORI	(REG_T0, REG_ZERO, IMM12(dst));
		ADD	(REG_T0, REG_S11, REG_T0);

		/* t1 = src_addr = &env->frame->tmpvar[src] */
		ORI	(REG_T1, REG_ZERO, IMM12(src));
		ADD	(REG_T1, REG_S11, REG_T1);

		/* *dst_addr = *src_addr */
		LW	(REG_T2, 0, REG_T1);
		LW	(REG_T3, 4, REG_T1);
		SW	(REG_T2, 0, REG_T0);
		SW	(REG_T3, 4, REG_T0);
	}

	return true;
}

/* Visit a OP_ICONST instruction. */
static INLINE bool
jit_visit_iconst_op(
	struct jit_context *ctx)
{
	int dst;
	uint32_t val;

	CONSUME_TMPVAR(dst);
	CONSUME_IMM32(val);

	dst *= (int)sizeof(struct rt_value);

	/* Set an integer constant. */
	ASM {
		/* s10: env */
		/* s11: &env->frame->tmpvar[0] */

		/* t0 = &env->frame->tmpvar[dst] */
		ORI	(REG_T0, REG_ZERO, IMM12(dst));
		ADD	(REG_T0, REG_S11, REG_T0);

		/* env->frame->tmpvar[dst].type = RT_VALUE_INT */
		ORI	(REG_T1, REG_ZERO, IMM12(0));
		SW	(REG_T1, 0, REG_T0);

		/* env->frame->tmpvar[dst].val.i = val */
		LI_32	(REG_T2, IMM32(val));
		SW	(REG_T2, 4, REG_T0);
	}

	return true;
}

/* Visit a OP_FCONST instruction. */
static INLINE bool
jit_visit_fconst_op(
	struct jit_context *ctx)
{
	int dst;
	uint32_t val;

	CONSUME_TMPVAR(dst);
	CONSUME_IMM32(val);

	dst *= (int)sizeof(struct rt_value);

	/* Set a floating-point constant. */
	ASM {
		/* s10: env */
		/* s11: &env->frame->tmpvar[0] */

		/* x2 = &env->frame->tmpvar[dst] */
		ORI	(REG_T0, REG_ZERO, IMM12(dst));
		ADD	(REG_T0, REG_S11, REG_T0);

		/* Assign env->frame->tmpvar[dst].type = RT_VALUE_FLOAT. */
		ORI	(REG_T1, REG_ZERO, IMM12(1));
		SW	(REG_T1, 0, REG_T0);

		/* Assign env->frame->tmpvar[dst].val.f = val. */
		LI_32	(REG_T2, IMM32(val));
		SW	(REG_T2, 4, REG_T0);
	}

	return true;
}

/* Visit a OP_SCONST instruction. */
static INLINE bool
jit_visit_sconst_op(
	struct jit_context *ctx)
{
	int dst;
	const char *val;
	uint32_t len, hash;

	CONSUME_TMPVAR(dst);
	CONSUME_STRING(val, len, hash);

	dst *= (int)sizeof(struct rt_value);

	/* rt_make_string(env, &env->frame->tmpvar[dst], val, len, hash); */
	ASM {
		/* s10: env */
		/* s11: &env->frame->tmpvar[0] */

		/* Arg1 a0: env */
		MV	(REG_A0, REG_S10);

		/* Arg2 a1: &env->frame->tmpvar[dst] */
		ORI	(REG_A1, REG_ZERO, IMM12(dst));
		ADD	(REG_A1, REG_S11, REG_A1);

		/* Arg3: a2: val */
		LI_32	(REG_A2, IMM32(val));

		/* Arg4: a3: len */
		LI_32	(REG_A3, IMM32(len));

		/* Arg5: a4: hash */
		LI_32	(REG_A4, IMM32(hash));

		/* Call rt_make_string(). */
		LI_32	(REG_T0, IMM32((uint32_t)rt_make_string_with_hash));
		JALR	(REG_RA, IMM12(0), REG_T0);

		/* If failed: */
		BEQ	(REG_A0, REG_ZERO, IMM13((uint32_t)ctx->exception_code - (uint32_t)ctx->code));
	}

	return true;
}

/* Visit a OP_ACONST instruction. */
static INLINE bool
jit_visit_aconst_op(
	struct jit_context *ctx)
{
	int dst;

	CONSUME_TMPVAR(dst);

	dst *= (int)sizeof(struct rt_value);

	/* rt_make_empty_array(env, &env->frame->tmpvar[dst]); */
	ASM {
		/* s10: env */
		/* s11: &env->frame->tmpvar[0] */

		/* Arg1 a0: env */
		MV	(REG_A0, REG_S10);

		/* Arg2 a1: &env->frame->tmpvar[dst] */
		ORI	(REG_A1, REG_ZERO, IMM12(dst));
		ADD	(REG_A1, REG_S11, REG_A1);

		/* Call rt_make_empty_array(). */
		LI_32	(REG_T0, IMM32((uint32_t)rt_make_empty_array));
		JALR	(REG_RA, IMM12(0), REG_T0);

		/* If failed: */
		BEQ	(REG_A0, REG_ZERO, IMM13((uint32_t)ctx->exception_code - (uint32_t)ctx->code));
	}

	return true;
}

/* Visit a OP_DCONST instruction. */
static INLINE bool
jit_visit_dconst_op(
	struct jit_context *ctx)
{
	int dst;

	CONSUME_TMPVAR(dst);

	dst *= (int)sizeof(struct rt_value);

	/* rt_make_empty_dict(env, &env->frame->tmpvar[dst]); */
	ASM {
		/* s10: env */
		/* s11: &env->frame->tmpvar[0] */

		/* Arg1 a0: env */
		MV	(REG_A0, REG_S10);

		/* Arg2 a1: &env->frame->tmpvar[dst] */
		ORI	(REG_A1, REG_ZERO, IMM12(dst));
		ADD	(REG_A1, REG_S11, REG_A1);

		/* Call rt_make_empty_dict(). */
		LI_32	(REG_T0, IMM32((uint32_t)rt_make_empty_dict));
		JALR	(REG_RA, IMM12(0), REG_T0);

		/* If failed: */
		BEQ	(REG_A0, REG_ZERO, IMM13((uint32_t)ctx->exception_code - (uint32_t)ctx->code));
	}

	return true;
}

/* Visit a OP_INC instruction. */
static INLINE bool
jit_visit_inc_op(
	struct jit_context *ctx)
{
	int dst;

	CONSUME_TMPVAR(dst);

	dst *= (int)sizeof(struct rt_value);

	/* Increment an integer. */
	ASM {
		/* s10: env */
		/* s11: &env->frame->tmpvar[0] */

		/* t0 = &env->frame->tmpvar[dst]. */
		ORI	(REG_T0, REG_ZERO, IMM12(dst));
		ADD	(REG_T0, REG_S11, REG_T0);

		/* env->frame->tmpvar[dst].val.i++ */
		LW	(REG_T1, 4, REG_T0);		/* tmp = &env->frame->tmpvar[dst].val.i */
		ADDI	(REG_T1, REG_T1, IMM12(1));	/* tmp++ */
		SW	(REG_T1, 4, REG_T0);		/* env->frame->tmpvar[dst].val.i = tmp */
	}

	return true;
}

/* Visit a OP_ADD instruction. */
static INLINE bool
jit_visit_add_op(
	struct jit_context *ctx)
{
	int dst;
	int src1;
	int src2;

	CONSUME_TMPVAR(dst);
	CONSUME_TMPVAR(src1);
	CONSUME_TMPVAR(src2);

	/* if (!rt_add_helper(env, dst, src1, src2)) return false; */
	ASM_BINARY_OP(rt_add_helper);

	return true;
}

/* Visit a OP_SUB instruction. */
static INLINE bool
jit_visit_sub_op(
	struct jit_context *ctx)
{
	int dst;
	int src1;
	int src2;

	CONSUME_TMPVAR(dst);
	CONSUME_TMPVAR(src1);
	CONSUME_TMPVAR(src2);

	/* if (!rt_sub_helper(env, dst, src1, src2)) return false; */
	ASM_BINARY_OP(rt_sub_helper);

	return true;
}

/* Visit a OP_MUL instruction. */
static INLINE bool
jit_visit_mul_op(
	struct jit_context *ctx)
{
	int dst;
	int src1;
	int src2;

	CONSUME_TMPVAR(dst);
	CONSUME_TMPVAR(src1);
	CONSUME_TMPVAR(src2);

	/* if (!rt_mul_helper(env, dst, src1, src2)) return false; */
	ASM_BINARY_OP(rt_mul_helper);

	return true;
}

/* Visit a OP_DIV instruction. */
static INLINE bool
jit_visit_div_op(
	struct jit_context *ctx)
{
	int dst;
	int src1;
	int src2;

	CONSUME_TMPVAR(dst);
	CONSUME_TMPVAR(src1);
	CONSUME_TMPVAR(src2);

	/* if (!rt_div_helper(env, dst, src1, src2)) return false; */
	ASM_BINARY_OP(rt_div_helper);

	return true;
}

/* Visit a OP_MOD instruction. */
static INLINE bool
jit_visit_mod_op(
	struct jit_context *ctx)
{
	int dst;
	int src1;
	int src2;

	CONSUME_TMPVAR(dst);
	CONSUME_TMPVAR(src1);
	CONSUME_TMPVAR(src2);

	/* if (!rt_mod_helper(env, dst, src1, src2)) return false; */
	ASM_BINARY_OP(rt_mod_helper);

	return true;
}

/* Visit a OP_AND instruction. */
static INLINE bool
jit_visit_and_op(
	struct jit_context *ctx)
{
	int dst;
	int src1;
	int src2;

	CONSUME_TMPVAR(dst);
	CONSUME_TMPVAR(src1);
	CONSUME_TMPVAR(src2);

	/* if (!rt_and_helper(env, dst, src1, src2)) return false; */
	ASM_BINARY_OP(rt_and_helper);

	return true;
}

/* Visit a OP_OR instruction. */
static INLINE bool
jit_visit_or_op(
	struct jit_context *ctx)
{
	int dst;
	int src1;
	int src2;

	CONSUME_TMPVAR(dst);
	CONSUME_TMPVAR(src1);
	CONSUME_TMPVAR(src2);

	/* if (!rt_or_helper(env, dst, src1, src2)) return false; */
	ASM_BINARY_OP(rt_or_helper);

	return true;
}

/* Visit a OP_XOR instruction. */
static INLINE bool
jit_visit_xor_op(
	struct jit_context *ctx)
{
	int dst;
	int src1;
	int src2;

	CONSUME_TMPVAR(dst);
	CONSUME_TMPVAR(src1);
	CONSUME_TMPVAR(src2);

	/* if (!rt_xor_helper(env, dst, src1, src2)) return false; */
	ASM_BINARY_OP(rt_xor_helper);

	return true;
}

/* Visit a OP_SHL instruction. */
static INLINE bool
jit_visit_shl_op(
        struct jit_context *ctx)
{
        int dst;
        int src1;
        int src2;

        CONSUME_TMPVAR(dst);
        CONSUME_TMPVAR(src1);
        CONSUME_TMPVAR(src2);

        /* if (!jit_shl_helper(env, dst, src1, src2)) return false; */
        ASM_BINARY_OP(rt_shl_helper);

        return true;
}

/* Visit a OP_SHR instruction. */
static INLINE bool
jit_visit_shr_op(
        struct jit_context *ctx)
{
        int dst;
        int src1;
        int src2;

        CONSUME_TMPVAR(dst);
        CONSUME_TMPVAR(src1);
        CONSUME_TMPVAR(src2);

        /* if (!jit_shr_helper(env, dst, src1, src2)) return false; */
        ASM_BINARY_OP(rt_shr_helper);

        return true;
}

/* Visit a OP_NEG instruction. */
static INLINE bool
jit_visit_neg_op(
	struct jit_context *ctx)
{
	int dst;
	int src;

	CONSUME_TMPVAR(dst);
	CONSUME_TMPVAR(src);

	/* if (!rt_neg_helper(env, dst, src)) return false; */
	ASM_UNARY_OP(rt_neg_helper);

	return true;
}

/* Visit a OP_NOT instruction. */
static INLINE bool
jit_visit_not_op(
	struct jit_context *ctx)
{
	int dst;
	int src;

	CONSUME_TMPVAR(dst);
	CONSUME_TMPVAR(src);

	/* if (!rt_not_helper(env, dst, src)) return false; */
	ASM_UNARY_OP(rt_not_helper);

	return true;
}

/* Visit a OP_LT instruction. */
static INLINE bool
jit_visit_lt_op(
	struct jit_context *ctx)
{
	int dst;
	int src1;
	int src2;

	CONSUME_TMPVAR(dst);
	CONSUME_TMPVAR(src1);
	CONSUME_TMPVAR(src2);

	/* if (!rt_lt_helper(env, dst, src1, src2)) return false; */
	ASM_BINARY_OP(rt_lt_helper);

	return true;
}

/* Visit a OP_LTE instruction. */
static INLINE bool
jit_visit_lte_op(
	struct jit_context *ctx)
{
	int dst;
	int src1;
	int src2;

	CONSUME_TMPVAR(dst);
	CONSUME_TMPVAR(src1);
	CONSUME_TMPVAR(src2);

	/* if (!rt_lte_helper(env, dst, src1, src2)) return false; */
	ASM_BINARY_OP(rt_lte_helper);

	return true;
}

/* Visit a OP_EQ instruction. */
static INLINE bool
jit_visit_eq_op(
	struct jit_context *ctx)
{
	int dst;
	int src1;
	int src2;

	CONSUME_TMPVAR(dst);
	CONSUME_TMPVAR(src1);
	CONSUME_TMPVAR(src2);

	/* if (!rt_eq_helper(env, dst, src1, src2)) return false; */
	ASM_BINARY_OP(rt_eq_helper);

	return true;
}

/* Visit a OP_NEQ instruction. */
static INLINE bool
jit_visit_neq_op(
	struct jit_context *ctx)
{
	int dst;
	int src1;
	int src2;

	CONSUME_TMPVAR(dst);
	CONSUME_TMPVAR(src1);
	CONSUME_TMPVAR(src2);

	/* if (!rt_neq_helper(env, dst, src1, src2)) return false; */
	ASM_BINARY_OP(rt_neq_helper);

	return true;
}

/* Visit a OP_GTE instruction. */
static INLINE bool
jit_visit_gte_op(
	struct jit_context *ctx)
{
	int dst;
	int src1;
	int src2;

	CONSUME_TMPVAR(dst);
	CONSUME_TMPVAR(src1);
	CONSUME_TMPVAR(src2);

	/* if (!rt_gte_helper(env, dst, src1, src2)) return false; */
	ASM_BINARY_OP(rt_gte_helper);

	return true;
}

/* Visit a OP_GT instruction. */
static INLINE bool
jit_visit_gt_op(
	struct jit_context *ctx)
{
	int dst;
	int src1;
	int src2;

	CONSUME_TMPVAR(dst);
	CONSUME_TMPVAR(src1);
	CONSUME_TMPVAR(src2);

	/* if (!rt_gt_helper(env, dst, src1, src2)) return false; */
	ASM_BINARY_OP(rt_gt_helper);

	return true;
}

/* Visit a OP_EQI instruction. */
static INLINE bool
jit_visit_eqi_op(
	struct jit_context *ctx)
{
	int dst;
	int src1;
	int src2;

	CONSUME_TMPVAR(dst);
	CONSUME_TMPVAR(src1);
	CONSUME_TMPVAR(src2);

	src1 *= (int)sizeof(struct rt_value);
	src2 *= (int)sizeof(struct rt_value);

	/* src1 == src2 */
	ASM {
		/* s10: env */
		/* s11: &env->frame->tmpvar[0] */

		/* t0 = &env->frame->tmpvar[src1].val.i */
		ORI	(REG_T0, REG_ZERO, IMM12(src1));
		ADD	(REG_T0, REG_T0, REG_S11);
		LW	(REG_T0, 4, REG_T0);

		/* t1 = &env->frame->tmpvar[src2].val.i */
		ORI	(REG_T1, REG_ZERO, IMM12(src2));
		ADD	(REG_T1, REG_T1, REG_S11);
		LW	(REG_T1, 4, REG_T1);

		/* Here, t0 = src1, t1 = src2 */
	}

	return true;
}

/* Visit a OP_LOADARRAY instruction. */
static INLINE bool
jit_visit_loadarray_op(
	struct jit_context *ctx)
{
	int dst;
	int src1;
	int src2;

	CONSUME_TMPVAR(dst);
	CONSUME_TMPVAR(src1);
	CONSUME_TMPVAR(src2);

	/* if (!rt_loadarray_helper(env, dst, src1, src2)) return false; */
	ASM_BINARY_OP(rt_loadarray_helper);

	return true;
}

/* Visit a OP_STOREARRAY instruction. */
static INLINE bool
jit_visit_storearray_op(
	struct jit_context *ctx)
{
	int dst;
	int src1;
	int src2;

	CONSUME_TMPVAR(dst);
	CONSUME_TMPVAR(src1);
	CONSUME_TMPVAR(src2);

	/* if (!jit_storearray_helper(env, dst, src1, src2)) return false; */
	ASM_BINARY_OP(rt_storearray_helper);

	return true;
}

/* Visit a OP_LEN instruction. */
static INLINE bool
jit_visit_len_op(
	struct jit_context *ctx)
{
	int dst;
	int src;

	CONSUME_TMPVAR(dst);
	CONSUME_TMPVAR(src);

	/* if (!jit_len_helper(env, dst, src)) return false; */
	ASM_UNARY_OP(rt_len_helper);

	return true;
}

/* Visit a OP_GETDICTKEYBYINDEX instruction. */
static INLINE bool
jit_visit_getdictkeybyindex_op(
	struct jit_context *ctx)
{
	int dst;
	int src1;
	int src2;

	CONSUME_TMPVAR(dst);
	CONSUME_TMPVAR(src1);
	CONSUME_TMPVAR(src2);

	/* if (!jit_getdictkeybyindex_helper(env, dst, src1, src2)) return false; */
	ASM_BINARY_OP(rt_getdictkeybyindex_helper);

	return true;
}

/* Visit a OP_GETDICTVALBYINDEX instruction. */
static INLINE bool
jit_visit_getdictvalbyindex_op(
	struct jit_context *ctx)
{
	int dst;
	int src1;
	int src2;

	CONSUME_TMPVAR(dst);
	CONSUME_TMPVAR(src1);
	CONSUME_TMPVAR(src2);

	/* if (!jit_getdictvalbyindex_helper(env, dst, src1, src2)) return false; */
	ASM_BINARY_OP(rt_getdictvalbyindex_helper);

	return true;
}

/* Visit a OP_LOADSYMBOL instruction. */
static INLINE bool
jit_visit_loadsymbol_op(
	struct jit_context *ctx)
{
	int dst;
	const char *src_s;
	uint32_t len, hash, src;

	CONSUME_TMPVAR(dst);
	CONSUME_STRING(src_s, len, hash);
	src = (uint32_t)(intptr_t)src_s;

	/* if (!jit_loadsymbol_helper(env, dst, src, len, hash)) return false; */
	ASM {
		/* s10: env */
		/* s11: &env->frame->tmpvar[0] */

		/* Arg1 a0: env */
		MV	(REG_A0, REG_S10);

		/* Arg2 a1: dst */
		ORI	(REG_A1, REG_ZERO, IMM12(dst));

		/* Arg3 a2: src */
		LI_32	(REG_A2, IMM32(src));

		/* Arg4 a3: len */
		LI_32	(REG_A3, IMM32(len));

		/* Arg5 a4: hash */
		LI_32	(REG_A4, IMM32(hash));

		/* Call rt_loadsymbol_helper(). */
		LI_32	(REG_T0, IMM32((uint32_t)rt_loadsymbol_helper));
		JALR	(REG_RA, IMM12(0), REG_T0);

		/* If failed: */
		BEQ	(REG_A0, REG_ZERO, IMM13((uint32_t)ctx->exception_code - (uint32_t)ctx->code));
	}

	return true;
}

/* Visit a OP_STORESYMBOL instruction. */
static INLINE bool
jit_visit_storesymbol_op(
	struct jit_context *ctx)
{
	const char *dst_s;
	uint32_t len, hash, dst;
	int src;

	CONSUME_STRING(dst_s, len, hash);
	CONSUME_TMPVAR(src);
	dst = (uint32_t)(intptr_t)dst_s;

	/* if (!rt_storesymbol_helper(env, dst, len, hash, src)) return false; */
	ASM {
		/* s10: env */
		/* s11: &env->frame->tmpvar[0] */

		/* Arg1 a0: env */
		MV	(REG_A0, REG_S10);

		/* Arg2 a1: dst */
		LI_32	(REG_A1, IMM32(dst));

		/* Arg3 a2: len */
		LI_32	(REG_A2, IMM32(len));

		/* Arg4 a3: hash */
		LI_32	(REG_A3, IMM32(hash));

		/* Arg5 a4: src */
		ORI	(REG_A4, REG_ZERO, IMM12(src));

		/* Call rt_storesymbol_helper(). */
		LI_32	(REG_T0, IMM32((uint32_t)rt_storesymbol_helper));
		JALR	(REG_RA, IMM12(0), REG_T0);

		/* If failed: */
		BEQ	(REG_A0, REG_ZERO, IMM13((uint32_t)ctx->exception_code - (uint32_t)ctx->code));
	}

	return true;
}

/* Visit a OP_LOADDOT instruction. */
static INLINE bool
jit_visit_loaddot_op(
	struct jit_context *ctx)
{
	int dst;
	int dict;
	const char *field_s;
	uint32_t len, hash, field;

	CONSUME_TMPVAR(dst);
	CONSUME_TMPVAR(dict);
	CONSUME_STRING(field_s, len, hash);
	field = (uint32_t)(intptr_t)field_s;

	/* if (!rt_loaddot_helper(env, dst, dict, field, len, hash)) return false; */
	ASM {
		/* s10: env */
		/* s11: &env->frame->tmpvar[0] */

		/* Arg1 a0: env */
		MV	(REG_A0, REG_S10);

		/* Arg2 a1: dst */
		ORI	(REG_A1, REG_ZERO, IMM12(dst));

		/* Arg3 a2: dict */
		ORI	(REG_A2, REG_ZERO, IMM12(dict));

		/* Arg4 a3: field */
		LI_32	(REG_A3, IMM32(field));

		/* Arg5 a4: len */
		LI_32	(REG_A4, IMM32(len));

		/* Arg6 a5: hash */
		LI_32	(REG_A5, IMM32(hash));

		/* Call rt_loaddot_helper(). */
		LI_32	(REG_T0, IMM32((uint32_t)rt_loaddot_helper));
		JALR	(REG_RA, IMM12(0), REG_T0);

		/* If failed: */
		BEQ	(REG_A0, REG_ZERO, IMM13((uint32_t)ctx->exception_code - (uint32_t)ctx->code));
	}

	return true;
}

/* Visit a OP_STOREDOT instruction. */
static INLINE bool
jit_visit_storedot_op(
	struct jit_context *ctx)
{
	int dict;
	const char *field_s;
	uint32_t len, hash, field;
	int src;

	CONSUME_TMPVAR(dict);
	CONSUME_STRING(field_s, len, hash);
	CONSUME_TMPVAR(src);
	field = (uint32_t)(intptr_t)field_s;

	/* if (!jit_storedot_helper(env, dict, field, len, hash, src)) return false; */
	ASM {
		/* s10: env */
		/* s11: &env->frame->tmpvar[0] */

		/* Arg1 a0: env */
		MV	(REG_A0, REG_S10);

		/* Arg2 a1: dict */
		ORI	(REG_A1, REG_ZERO, IMM12(dict));

		/* Arg3 a2: field */
		LI_32	(REG_A2, IMM32(field));

		/* Arg4 a3: len */
		LI_32	(REG_A3, IMM32(len));

		/* Arg5 a4: hash */
		LI_32	(REG_A4, IMM32(hash));

		/* Arg6 a5: src */
		ORI	(REG_A5, REG_ZERO, IMM12(src));

		/* Call rt_storedot_helper(). */
		LI_32	(REG_T0, IMM32((uint32_t)rt_storedot_helper));
		JALR	(REG_RA, IMM12(0), REG_T0);

		/* If failed: */
		BEQ	(REG_A0, REG_ZERO, IMM13((uint32_t)ctx->exception_code - (uint32_t)ctx->code));
	}

	return true;
}

/* Visit a OP_CALL instruction. */
static inline bool
jit_visit_call_op(
	struct jit_context *ctx)
{
	int dst;
	int func;
	int arg_count;
	int arg_tmp;
	int arg[NOCT_ARG_MAX];
	uint32_t arg_addr;
	int i;

	CONSUME_TMPVAR(dst);
	CONSUME_TMPVAR(func);
	CONSUME_IMM8(arg_count);
	for (i = 0; i < arg_count; i++) {
		CONSUME_TMPVAR(arg_tmp);
		arg[i] = arg_tmp;
	}

	/* Embed arguments to the code. */
	if (arg_count > 0) {
		ASM {
			JAL	(REG_ZERO, IMM21(4 + 4 * arg_count));
		}
		arg_addr = (uint32_t)(intptr_t)ctx->code;
		for (i = 0; i < arg_count; i++) {
			*(uint32_t *)ctx->code = (uint32_t)arg[i];
			ctx->code = (uint32_t *)ctx->code + 1;
		}
	} else {
		arg_addr = 0;
	}

	/* if (!rt_call_helper(env, dst, func, arg_count, arg)) return false; */
	ASM {
		/* s10: env */
		/* s11: &env->frame->tmpvar[0] */

		/* Arg1 a0: env */
		MV	(REG_A0, REG_S10);

		/* Arg2 a1: dst */
		ORI	(REG_A1, REG_ZERO, IMM12(dst));

		/* Arg3 a2: func */
		ORI	(REG_A2, REG_ZERO, IMM12(func));

		/* Arg4 a3: arg_count */
		ORI	(REG_A3, REG_ZERO, IMM12(arg_count));

		/* Arg5 a4: arg */
		LI_32	(REG_A4, IMM32(arg_addr));

		/* Call rt_call_helper(). */
		LI_32	(REG_T0, IMM32((uint32_t)rt_call_helper));
		JALR	(REG_RA, IMM12(0), REG_T0);

		/* If failed: */
		BEQ	(REG_A0, REG_ZERO, IMM13((uint32_t)ctx->exception_code - (uint32_t)ctx->code));
	}

	return true;
}

/* Visit a OP_THISCALL instruction. */
static inline bool
jit_visit_thiscall_op(
	struct jit_context *ctx)
{
	int dst;
	int obj;
	const char *symbol;
	uint32_t len, hash;
	int arg_count;
	int arg_tmp;
	int arg[NOCT_ARG_MAX];
	uint32_t arg_addr;
	int i;

	CONSUME_TMPVAR(dst);
	CONSUME_TMPVAR(obj);
	CONSUME_STRING(symbol, len, hash);
	CONSUME_IMM8(arg_count);
	for (i = 0; i < arg_count; i++) {
		CONSUME_TMPVAR(arg_tmp);
		arg[i] = arg_tmp;
	}

	/* Embed arguments. */
	ASM {
		JAL	(REG_ZERO, IMM21(4 + 4 * arg_count));
	}
	arg_addr = (uint32_t)(intptr_t)ctx->code;
	for (i = 0; i < arg_count; i++) {
		*(uint32_t *)ctx->code = (uint32_t)arg[i];
		ctx->code = (uint32_t *)ctx->code + 1;
	}

	/* if (!rt_thiscall_helper(env, dst, obj, symbol, arg_count, arg)) return false; */
	ASM {
		/* s10: env */
		/* s11: &env->frame->tmpvar[0] */

		/* Arg1 a0: env */
		MV	(REG_A0, REG_S10);

		/* Arg2 a1: dst */
		ORI	(REG_A1, REG_ZERO, IMM12(dst));

		/* Arg3 a2: obj */
		ORI	(REG_A2, REG_ZERO, IMM12(obj));

		/* Arg4 a3: symbol */
		LI_32	(REG_A3, IMM32((uint32_t)symbol));

		/* Arg5 a4: len */
		LI_32	(REG_A4, IMM32(len));

		/* Arg6 a5: hash */
		LI_32	(REG_A5, IMM32(hash));

		/* Arg7 a6: argcount */
		ORI	(REG_A6, REG_ZERO, IMM12(arg_count));

		/* Arg8 a7: arg */
		LI_32	(REG_A7, IMM32(arg_addr));

		/* Call rt_thiscall_helper(). */
		LI_32	(REG_T0, IMM32((uint32_t)rt_thiscall_helper));
		JALR	(REG_RA, IMM12(0), REG_T0);

		/* If failed: */
		BEQ	(REG_A0, REG_ZERO, IMM13((uint32_t)ctx->exception_code - (uint32_t)ctx->code));
	}

	return true;
}

/* Visit a OP_JMP instruction. */
static inline bool
jit_visit_jmp_op(
	struct jit_context *ctx)
{
	uint32_t target_lpc;

	CONSUME_IMM32(target_lpc);
	if (target_lpc >= (uint32_t)(ctx->func->bytecode_size + 1)) {
		rt_error(ctx->env, BROKEN_BYTECODE);
		return false;
	}

	/* Patch later. */
	ctx->branch_patch[ctx->branch_patch_count].code = ctx->code;
	ctx->branch_patch[ctx->branch_patch_count].lpc = target_lpc;
	ctx->branch_patch[ctx->branch_patch_count].type = PATCH_JAL;
	ctx->branch_patch_count++;

	ASM {
		/* s10: env */
		/* s11: &env->frame->tmpvar[0] */

		/* Patched later. */
		JAL	(REG_ZERO, IMM21(0));
	}

	return true;
}

/* Visit a OP_JMPIFTRUE instruction. */
static inline bool
jit_visit_jmpiftrue_op(
	struct jit_context *ctx)
{
	int src;
	uint32_t target_lpc;

	CONSUME_TMPVAR(src);
	CONSUME_IMM32(target_lpc);
	if (target_lpc >= (uint32_t)(ctx->func->bytecode_size + 1)) {
		rt_error(ctx->env, BROKEN_BYTECODE);
		return false;
	}

	src *= sizeof(struct rt_value);

	ASM {
		/* s10: env */
		/* s11: &env->frame->tmpvar[0] */

		/* t0 = &env->frame->tmpvar[src].val.i */
		ORI	(REG_T0, REG_ZERO, IMM12(src));
		ADD	(REG_T0, REG_S11, REG_T0);
		LW	(REG_T0, 4, REG_T0);

		/* Compare: env->frame->tmpvar[dst].val.i != 0 */
		ORI	(REG_T1, REG_ZERO, IMM12(0));
	}

	/* Patch later. */
	ctx->branch_patch[ctx->branch_patch_count].code = ctx->code;
	ctx->branch_patch[ctx->branch_patch_count].lpc = target_lpc;
	ctx->branch_patch[ctx->branch_patch_count].type = PATCH_BNE;
	ctx->branch_patch_count++;

	ASM {
		/* Patched later. */
		BNE	(REG_T0, REG_T1, IMM13(0));
	}

	return true;
}

/* Visit a OP_JMPIFFALSE instruction. */
static inline bool
jit_visit_jmpiffalse_op(
	struct jit_context *ctx)
{
	int src;
	uint32_t target_lpc;

	CONSUME_TMPVAR(src);
	CONSUME_IMM32(target_lpc);
	if (target_lpc >= (uint32_t)(ctx->func->bytecode_size + 1)) {
		rt_error(ctx->env, BROKEN_BYTECODE);
		return false;
	}

	src *= sizeof(struct rt_value);

	ASM {
		/* s10: env */
		/* s11: &env->frame->tmpvar[0] */

		/* t0 = &env->frame->tmpvar[src].val.i */
		ORI	(REG_T0, REG_ZERO, IMM12(src));
		ADD	(REG_T0, REG_S11, REG_T0);
		LW	(REG_T0, 4, REG_T0);

		/* Compare: env->frame->tmpvar[dst].val.i == 0 */
		ORI	(REG_T1, REG_ZERO, IMM12(0));
	}

	/* Patch later. */
	ctx->branch_patch[ctx->branch_patch_count].code = ctx->code;
	ctx->branch_patch[ctx->branch_patch_count].lpc = target_lpc;
	ctx->branch_patch[ctx->branch_patch_count].type = PATCH_BEQ;
	ctx->branch_patch_count++;

	ASM {
		/* Patched later. */
		BEQ	(REG_T0, REG_T1, IMM13(0));
	}

	return true;
}

/* Visit a OP_JMPIFEQ instruction. */
static inline bool
jit_visit_jmpifeq_op(
	struct jit_context *ctx)
{
	int src;
	uint32_t target_lpc;

	CONSUME_TMPVAR(src);
	CONSUME_IMM32(target_lpc);
	if (target_lpc >= (uint32_t)(ctx->func->bytecode_size + 1)) {
		rt_error(ctx->env, BROKEN_BYTECODE);
		return false;
	}

	/* Patch later. */
	ctx->branch_patch[ctx->branch_patch_count].code = ctx->code;
	ctx->branch_patch[ctx->branch_patch_count].lpc = target_lpc;
	ctx->branch_patch[ctx->branch_patch_count].type = PATCH_BEQ;
	ctx->branch_patch_count++;

	ASM {
		/* s10: env */
		/* s11: &env->frame->tmpvar[0] */

		/* Patched later. */
		BEQ	(REG_T0, REG_T1, IMM13(0));
	}

	return true;
}

/* Visit a bytecode of a function. */
bool
jit_visit_bytecode(
	struct jit_context *ctx)
{
	uint8_t opcode;

	/* Put a prologue. */
	ASM {
		/* Push the general-purpose registers. */
		ADDI	(REG_SP, REG_SP, IMM12(-32));
		SW	(REG_RA,  16, REG_SP);
		SW	(REG_S10,  8, REG_SP);
		SW	(REG_S11,  0, REG_SP);

		/* s10 = rt */
		MV	(REG_S10, REG_A0);

		/* s11 = &env->frame->tmpvar[0] */
		LW	(REG_S11, 0, REG_A0);
		LW	(REG_S11, 0, REG_S11);

		/* Skip an exception handler. */
		JAL	(REG_ZERO, IMM21(28));
	}

	/* Put an exception handler. */
	ctx->exception_code = ctx->code;
	ASM {
	/* EXCEPTION: */
		LW	(REG_S11,  0, REG_SP);
		LW	(REG_S10,  8, REG_SP);
		LW	(REG_RA,  16, REG_SP);
		ADDI	(REG_SP, REG_SP, IMM12(32));
		ORI	(REG_A0, REG_ZERO, IMM12(0));
		RET	();
	}

	/* Put a body. */
	while (ctx->lpc < ctx->func->bytecode_size) {
		/* Save LPC and addr. */
		if (ctx->pc_entry_count >= PC_ENTRY_MAX) {
			rt_error(ctx->env, N_TR("Code too big."));
			return false;
		}
		ctx->pc_entry[ctx->pc_entry_count].lpc = (uint32_t)ctx->lpc;
		ctx->pc_entry[ctx->pc_entry_count].code = ctx->code;
		ctx->pc_entry_count++;

		/* Dispatch by opcode. */
		CONSUME_OPCODE(opcode);
		switch (opcode) {
		case OP_LINEINFO:
			if (!jit_visit_lineinfo_op(ctx))
				return false;
			break;
		case OP_ASSIGN:
			if (!jit_visit_assign_op(ctx))
				return false;
			break;
		case OP_ICONST:
			if (!jit_visit_iconst_op(ctx))
				return false;
			break;
		case OP_FCONST:
			if (!jit_visit_fconst_op(ctx))
				return false;
			break;
		case OP_SCONST:
			if (!jit_visit_sconst_op(ctx))
				return false;
			break;
		case OP_ACONST:
			if (!jit_visit_aconst_op(ctx))
				return false;
			break;
		case OP_DCONST:
			if (!jit_visit_dconst_op(ctx))
				return false;
			break;
		case OP_INC:
			if (!jit_visit_inc_op(ctx))
				return false;
			break;
		case OP_ADD:
			if (!jit_visit_add_op(ctx))
				return false;
			break;
		case OP_SUB:
			if (!jit_visit_sub_op(ctx))
				return false;
			break;
		case OP_MUL:
			if (!jit_visit_mul_op(ctx))
				return false;
			break;
		case OP_DIV:
			if (!jit_visit_div_op(ctx))
				return false;
			break;
		case OP_MOD:
			if (!jit_visit_mod_op(ctx))
				return false;
			break;
		case OP_AND:
			if (!jit_visit_and_op(ctx))
				return false;
			break;
		case OP_OR:
			if (!jit_visit_or_op(ctx))
				return false;
			break;
		case OP_XOR:
			if (!jit_visit_xor_op(ctx))
				return false;
			break;
                case OP_SHL:
                        if (!jit_visit_shl_op(ctx))
                                return false;
                        break;
                case OP_SHR:
                        if (!jit_visit_shr_op(ctx))
                                return false;
                        break;
		case OP_NEG:
			if (!jit_visit_neg_op(ctx))
				return false;
			break;
		case OP_NOT:
			if (!jit_visit_not_op(ctx))
				return false;
			break;
		case OP_LT:
			if (!jit_visit_lt_op(ctx))
				return false;
			break;
		case OP_LTE:
			if (!jit_visit_lte_op(ctx))
				return false;
			break;
		case OP_EQ:
			if (!jit_visit_eq_op(ctx))
				return false;
			break;
		case OP_NEQ:
			if (!jit_visit_neq_op(ctx))
				return false;
			break;
		case OP_GTE:
			if (!jit_visit_gte_op(ctx))
				return false;
			break;
		case OP_GT:
			if (!jit_visit_gt_op(ctx))
				return false;
			break;
		case OP_EQI:
			if (!jit_visit_eqi_op(ctx))
				return false;
			break;
		case OP_LOADARRAY:
			if (!jit_visit_loadarray_op(ctx))
				return false;
			break;
		case OP_STOREARRAY:
			if (!jit_visit_storearray_op(ctx))
				return false;
			break;
		case OP_LEN:
			if (!jit_visit_len_op(ctx))
			return false;
			break;
		case OP_GETDICTKEYBYINDEX:
			if (!jit_visit_getdictkeybyindex_op(ctx))
			return false;
			break;
		case OP_GETDICTVALBYINDEX:
			if (!jit_visit_getdictvalbyindex_op(ctx))
				return false;
			break;
		case OP_LOADSYMBOL:
			if (!jit_visit_loadsymbol_op(ctx))
				return false;
			break;
		case OP_STORESYMBOL:
			if (!jit_visit_storesymbol_op(ctx))
				return false;
			break;
		case OP_LOADDOT:
			if (!jit_visit_loaddot_op(ctx))
				return false;
			break;
		case OP_STOREDOT:
			if (!jit_visit_storedot_op(ctx))
				return false;
			break;
		case OP_CALL:
			if (!jit_visit_call_op(ctx))
				return false;
			break;
		case OP_THISCALL:
			if (!jit_visit_thiscall_op(ctx))
				return false;
			break;
		case OP_JMP:
			if (!jit_visit_jmp_op(ctx))
				return false;
			break;
		case OP_JMPIFTRUE:
			if (!jit_visit_jmpiftrue_op(ctx))
				return false;
			break;
		case OP_JMPIFFALSE:
			if (!jit_visit_jmpiffalse_op(ctx))
				return false;
			break;
		case OP_JMPIFEQ:
			if (!jit_visit_jmpifeq_op(ctx))
				return false;
			break;
		default:
			assert(JIT_OP_NOT_IMPLEMENTED);
			break;
		}
	}

	/* Add the tail PC to the table. */
	ctx->pc_entry[ctx->pc_entry_count].lpc = (uint32_t)ctx->lpc;
	ctx->pc_entry[ctx->pc_entry_count].code = ctx->code;
	ctx->pc_entry_count++;

	/* Put an epilogue. */
	ASM {
	/* EPILOGUE: */
		LW	(REG_S11,  0, REG_SP);
		LW	(REG_S10,  8, REG_SP);
		LW	(REG_RA,  16, REG_SP);
		ADDI	(REG_SP, REG_SP, IMM12(32));
		ORI	(REG_A0, REG_ZERO, IMM12(1));
		RET	();
	}

	return true;
}

static bool
jit_patch_branch(
    struct jit_context *ctx,
    int patch_index)
{
	uint32_t *target_code;
	int offset;
	int i;

	if (ctx->pc_entry_count == 0)
		return true;

	/* Search a code addr at lpc. */
	target_code = NULL;
	for (i = 0; i < ctx->pc_entry_count; i++) {
		if (ctx->pc_entry[i].lpc == ctx->branch_patch[patch_index].lpc) {
			target_code = ctx->pc_entry[i].code;
			break;
		}
			
	}
	if (target_code == NULL) {
		rt_error(ctx->env, "Branch target not found.");
		return false;
	}

	/* Calc a branch offset. */
	offset = (int)((intptr_t)target_code - (intptr_t)ctx->branch_patch[patch_index].code);

	/* Set the assembler cursor. */
	ctx->code = ctx->branch_patch[patch_index].code;
	if (abs(offset) & ~0x1fffff) {
		rt_error(ctx->env, "Branch target too far.");
		return false;
	}

	/* Assemble. */
	if (ctx->branch_patch[patch_index].type == PATCH_JAL) {
		ASM {
			JAL	(REG_ZERO, IMM21(offset));
		}
	} else if (ctx->branch_patch[patch_index].type == PATCH_BEQ) {
		ASM {
			BEQ	(REG_T0, REG_T1, IMM13(offset));
		}
	} else if (ctx->branch_patch[patch_index].type == PATCH_BNE) {
		ASM {
			BNE	(REG_T0, REG_T1, IMM13(offset));
		}
	}

	return true;
}

#endif /* defined(NOCT_ARCH_RISCV32) && defined(NOCT_USE_JIT) */
