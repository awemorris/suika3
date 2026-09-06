/* -*- coding: utf-8; tab-width: 8; indent-tabs-mode: t; -*- */

/*
 * Noct Programming Language
 * Copyright (c) 2025, 2026, Awe Morris
 */

/*
 * JIT (riscv32): Just-In-Time native code generation
 */

#include <noct/noct.h>

#if defined(NOCT_ARCH_RISCV32) && defined(NOCT_USE_JIT)

#include "runtime.h"
#include "jit.h"

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

/* Forward declarations */
static bool jit_visit_bytecode(struct rt_jit_context *ctx);
static bool jit_patch_branch(struct rt_jit_context *ctx, int patch_index);
static bool jit_put_fsw(struct rt_jit_context *ctx, uint32_t fs,
                        uint32_t imm, uint32_t rs);

/*
 * Generate a JIT-compiled code for a function.
 */
bool
jit_build(
	  struct rt_env *env,
	  struct rt_func *func)
{
	return rt_jit_build_standard(env, func, 0, "riscv32",
                                  jit_visit_bytecode,
                                  jit_patch_branch);
}

/*
 * Free all JIT-compiled code.
 */
bool
jit_free(
	 struct rt_env *env)
{
	return rt_jit_slab_free_all(env);
}

/*
 * Commit written code.
 */
bool
jit_commit(
	struct rt_env *env)
{
	return rt_jit_slab_commit_all(env);
}

/*
 * Assembler output functions
 */

/* Put a instruction word. */
static INLINE bool
jit_put_word(
	struct rt_jit_context *ctx,
	uint32_t word)
{
	if (ctx->code >= ctx->code_end) {
		ctx->code_overflow = true;
		rt_error(ctx->env, N_TR("Code too big."));
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
	struct rt_jit_context *ctx,
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
	struct rt_jit_context *ctx,
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
	struct rt_jit_context *ctx,
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
	struct rt_jit_context *ctx,
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
	struct rt_jit_context *ctx,
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
	struct rt_jit_context *ctx,
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
	struct rt_jit_context *ctx,
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

/* LB */
#define LB(rd, imm, rs)	if (!jit_put_lb(ctx, rd, imm, rs)) return false
static INLINE bool
jit_put_lb(
	struct rt_jit_context *ctx,
	uint32_t rd,
	uint32_t imm,
	uint32_t rs)
{
	if (!jit_put_word(ctx,
			  ((imm & 0xfff) << 20) |
			  (rs << 15) |
			  (0 << 12) |
			  (rd << 7) |
			  0x03))
		return false;
	return true;
}

/* LH */
#define LH(rd, imm, rs)	if (!jit_put_lh(ctx, rd, imm, rs)) return false
static INLINE bool
jit_put_lh(
	struct rt_jit_context *ctx,
	uint32_t rd,
	uint32_t imm,
	uint32_t rs)
{
	if (!jit_put_word(ctx,
			  ((imm & 0xfff) << 20) |
			  (rs << 15) |
			  (1 << 12) |
			  (rd << 7) |
			  0x03))
		return false;
	return true;
}

/* LBU */
#define LBU(rd, imm, rs)	if (!jit_put_lbu(ctx, rd, imm, rs)) return false
static INLINE bool
jit_put_lbu(
	struct rt_jit_context *ctx,
	uint32_t rd,
	uint32_t imm,
	uint32_t rs)
{
	if (!jit_put_word(ctx,
			  ((imm & 0xfff) << 20) |
			  (rs << 15) |
			  (4 << 12) |
			  (rd << 7) |
			  0x03))
		return false;
	return true;
}

/* LHU */
#define LHU(rd, imm, rs)	if (!jit_put_lhu(ctx, rd, imm, rs)) return false
static INLINE bool
jit_put_lhu(
	struct rt_jit_context *ctx,
	uint32_t rd,
	uint32_t imm,
	uint32_t rs)
{
	if (!jit_put_word(ctx,
			  ((imm & 0xfff) << 20) |
			  (rs << 15) |
			  (5 << 12) |
			  (rd << 7) |
			  0x03))
		return false;
	return true;
}

/* SB */
#define SB(rs2, imm, rs1)	if (!jit_put_sb(ctx, rs2, imm, rs1)) return false
static INLINE bool
jit_put_sb(
	struct rt_jit_context *ctx,
	uint32_t rs2,
	uint32_t imm,
	uint32_t rs1)
{
	if (!jit_put_word(ctx,
			  (((imm & 0xfff) >> 5) << 25) |
			  (rs2 << 20) |
			  (rs1 << 15) |
			  (0 << 12) |
			  ((imm & 0x1f) << 7) |
			  0x23))
		return false;
	return true;
}

/* SH */
#define SH(rs2, imm, rs1)	if (!jit_put_sh(ctx, rs2, imm, rs1)) return false
static INLINE bool
jit_put_sh(
	struct rt_jit_context *ctx,
	uint32_t rs2,
	uint32_t imm,
	uint32_t rs1)
{
	if (!jit_put_word(ctx,
			  (((imm & 0xfff) >> 5) << 25) |
			  (rs2 << 20) |
			  (rs1 << 15) |
			  (1 << 12) |
			  ((imm & 0x1f) << 7) |
			  0x23))
		return false;
	return true;
}

/* JAL */
#define JAL(rd, imm)	if (!jit_put_jal(ctx, rd, imm)) return false
static INLINE bool
jit_put_jal(
	struct rt_jit_context *ctx,
	uint32_t rd,
	uint32_t imm)
{
	uint32_t imm_20;
	uint32_t imm_19_12;
	uint32_t imm_11;
	uint32_t imm_10_1;

	imm_20 = (imm & 0x100000) >> 20;
	imm_19_12 = (imm & 0x0ff000) >> 12;
	imm_11 = (imm & 0x000800) >> 11;
	imm_10_1 = (imm & 0x0007fe) >> 1;

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
	struct rt_jit_context *ctx,
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

/* AUIPC + JALR: full signed 32-bit PC-relative jump. */
static INLINE bool
jit_put_far_jump(
	struct rt_jit_context *ctx,
	int32_t rel)
{
	int32_t hi20;
	int32_t lo12;

	hi20 = (rel + 0x800) >> 12;
	lo12 = rel - (hi20 << 12);
	if (!jit_put_word(ctx,
			  (((uint32_t)hi20 & 0xfffff) << 12) |
			  (REG_T2 << 7) | 0x17))
		return false;
	if (!jit_put_jalr(ctx, REG_ZERO, (uint32_t)lo12, REG_T2))
		return false;
	return true;
}

/* Branch over a far jump when a helper succeeded. */
#define EXCEPTION_IF_ZERO(reg) do {                                      \
	if (!jit_put_bne(ctx, (reg), REG_ZERO, 12)) return false;         \
	if (!jit_put_far_jump(ctx,                                      \
		(int32_t)((intptr_t)ctx->exception_code -                  \
		          (intptr_t)ctx->code))) return false;             \
} while (0)

/* BEQ */
#define BEQ(rs1, rs2, rel)	if (!jit_put_beq(ctx, rs1, rs2, rel)) return false
static INLINE bool
jit_put_beq(
	struct rt_jit_context *ctx,
	uint32_t rs1,
	uint32_t rs2,
	uint32_t rel)
{
	uint32_t imm_12;
	uint32_t imm_11;
	uint32_t imm_10_5;
	uint32_t imm_4_1;

	imm_12 = (rel & 0x1000) >> 12;
	imm_11 = (rel & 0x0800) >> 11;
	imm_10_5 = (rel & 0x07e0) >> 5;
	imm_4_1 = (rel & 0x001e) >> 1;

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
	struct rt_jit_context *ctx,
	uint32_t rs1,
	uint32_t rs2,
	uint32_t rel)
{
	uint32_t imm_12;
	uint32_t imm_11;
	uint32_t imm_10_5;
	uint32_t imm_4_1;

	imm_12 = (rel & 0x1000) >> 12;
	imm_11 = (rel & 0x0800) >> 11;
	imm_10_5 = (rel & 0x07e0) >> 5;
	imm_4_1 = (rel & 0x001e) >> 1;

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
	struct rt_jit_context *ctx,
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
		EXCEPTION_IF_ZERO(REG_A0);								\
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
		EXCEPTION_IF_ZERO(REG_A0);								\
	}

/*
 * Bytecode visitors
 */

/* Visit a OP_LINEINFO instruction. */
static INLINE bool
jit_visit_lineinfo_op(
	struct rt_jit_context *ctx)
{
	uint32_t line;

	CONSUME_IMM32(line);

	ASM {
		/* s10: env */
		/* s11: &env->frame->tmpvar[0] */

		/* env->line = line; (offset 4 on 32-bit: after a 4-byte
		   frame pointer; also use LI_32 so lines > 2047 survive) */
		LI_32	(REG_T0, IMM32(line));
		SW	(REG_T0, 4, REG_S10);
	}

	return true;
}

/* Visit a OP_ASSIGN instruction. */
static INLINE bool
jit_visit_assign_op(
	struct rt_jit_context *ctx)
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
		LW	(REG_T2, 8, REG_T1);
		LW	(REG_T3, 12, REG_T1);
		SW	(REG_T2, 8, REG_T0);
		SW	(REG_T3, 12, REG_T0);
	}

	return true;
}

/* Visit a OP_ICONST instruction. */
static INLINE bool
jit_visit_iconst_op(
	struct rt_jit_context *ctx)
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
		SW	(REG_T2, 8, REG_T0);
	}

	return true;
}

/* Visit a OP_LICONST instruction. */
static INLINE bool
jit_visit_liconst_op(
	struct rt_jit_context *ctx)
{
	int dst;
	uint64_t val;

	CONSUME_TMPVAR(dst);
	CONSUME_IMM64(val);

	dst *= (int)sizeof(struct rt_value);

	/* Set an integer constant. */
	ASM {
		/* s10: env */
		/* s11: &env->frame->tmpvar[0] */

		/* t0 = &env->frame->tmpvar[dst] */
		ORI	(REG_T0, REG_ZERO, IMM12(dst));
		ADD	(REG_T0, REG_S11, REG_T0);

		/* env->frame->tmpvar[dst].type = NOCT_VALUE_LONG */
		ORI	(REG_T1, REG_ZERO, IMM12(5));
		SW	(REG_T1, 0, REG_T0);

		/* env->frame->tmpvar[dst].val.i = val */
		LI_32	(REG_T2, IMM32((uint32_t)(val & 0xffffffff)));
		SW	(REG_T2, 8, REG_T0);
		LI_32	(REG_T2, IMM32((uint32_t)((val >> 32) & 0xffffffff)));
		SW	(REG_T2, 12, REG_T0);
	}

	return true;
}

/* Visit a OP_FCONST instruction. */
static INLINE bool
jit_visit_fconst_op(
	struct rt_jit_context *ctx)
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
		SW	(REG_T2, 8, REG_T0);
	}

	return true;
}

/* Visit a OP_LFCONST instruction. */
static INLINE bool
jit_visit_lfconst_op(
	struct rt_jit_context *ctx)
{
	int dst;
	uint64_t val;

	CONSUME_TMPVAR(dst);
	CONSUME_IMM64(val);

	dst *= (int)sizeof(struct rt_value);

	/* Set an integer constant. */
	ASM {
		/* s10: env */
		/* s11: &env->frame->tmpvar[0] */

		/* t0 = &env->frame->tmpvar[dst] */
		ORI	(REG_T0, REG_ZERO, IMM12(dst));
		ADD	(REG_T0, REG_S11, REG_T0);

		/* env->frame->tmpvar[dst].type = NOCT_VALUE_DOUBLE */
		ORI	(REG_T1, REG_ZERO, IMM12(6));
		SW	(REG_T1, 0, REG_T0);

		/* env->frame->tmpvar[dst].val.i = val */
		LI_32	(REG_T2, IMM32((uint32_t)(val & 0xffffffff)));
		SW	(REG_T2, 8, REG_T0);
		LI_32	(REG_T2, IMM32((uint32_t)((val >> 32) & 0xffffffff)));
		SW	(REG_T2, 12, REG_T0);
	}

	return true;
}

/* Visit a OP_SCONST instruction. */
static INLINE bool
jit_visit_sconst_op(
	struct rt_jit_context *ctx)
{
	int dst;
	const char *val;
	uint32_t len, hash;

	CONSUME_TMPVAR(dst);
	CONSUME_STRING(val, len, hash);

	dst *= (int)sizeof(struct rt_value);

	/* ex_make_string(env, &env->frame->tmpvar[dst], val, len, hash); */
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

		/* Call ex_make_string(). */
		LI_32	(REG_T0, IMM32((uint32_t)ex_make_string_with_hash));
		JALR	(REG_RA, IMM12(0), REG_T0);

		/* If failed: */
		EXCEPTION_IF_ZERO(REG_A0);
	}

	return true;
}

/* Visit a OP_ACONST instruction. */
static INLINE bool
jit_visit_aconst_op(
	struct rt_jit_context *ctx)
{
	int dst;

	CONSUME_TMPVAR(dst);

	dst *= (int)sizeof(struct rt_value);

	/* ex_make_empty_array(env, &env->frame->tmpvar[dst]); */
	ASM {
		/* s10: env */
		/* s11: &env->frame->tmpvar[0] */

		/* Arg1 a0: env */
		MV	(REG_A0, REG_S10);

		/* Arg2 a1: &env->frame->tmpvar[dst] */
		ORI	(REG_A1, REG_ZERO, IMM12(dst));
		ADD	(REG_A1, REG_S11, REG_A1);

		/* Call ex_make_empty_array(). */
		LI_32	(REG_T0, IMM32((uint32_t)ex_make_empty_array));
		JALR	(REG_RA, IMM12(0), REG_T0);

		/* If failed: */
		EXCEPTION_IF_ZERO(REG_A0);
	}

	return true;
}

/* Visit a OP_DCONST instruction. */
static INLINE bool
jit_visit_dconst_op(
	struct rt_jit_context *ctx)
{
	int dst;

	CONSUME_TMPVAR(dst);

	dst *= (int)sizeof(struct rt_value);

	/* ex_make_empty_dict(env, &env->frame->tmpvar[dst]); */
	ASM {
		/* s10: env */
		/* s11: &env->frame->tmpvar[0] */

		/* Arg1 a0: env */
		MV	(REG_A0, REG_S10);

		/* Arg2 a1: &env->frame->tmpvar[dst] */
		ORI	(REG_A1, REG_ZERO, IMM12(dst));
		ADD	(REG_A1, REG_S11, REG_A1);

		/* Call ex_make_empty_dict(). */
		LI_32	(REG_T0, IMM32((uint32_t)ex_make_empty_dict));
		JALR	(REG_RA, IMM12(0), REG_T0);

		/* If failed: */
		EXCEPTION_IF_ZERO(REG_A0);
	}

	return true;
}

/* Visit a OP_INC instruction. */
static INLINE bool
jit_visit_inc_op(
	struct rt_jit_context *ctx)
{
	int dst;
	int step;

	CONSUME_TMPVAR(dst);
	CONSUME_IMM8(step);

	dst *= (int)sizeof(struct rt_value);

	/* Increment an integer. */
	ASM {
		/* s10: env */
		/* s11: &env->frame->tmpvar[0] */

		/* t0 = &env->frame->tmpvar[dst]. */
		ORI	(REG_T0, REG_ZERO, IMM12(dst));
		ADD	(REG_T0, REG_S11, REG_T0);

		/* env->frame->tmpvar[dst].val.i++ */
		LW	(REG_T1, 8, REG_T0);		/* tmp = &env->frame->tmpvar[dst].val.i */
		ADDI	(REG_T1, REG_T1, IMM12(step));
		SW	(REG_T1, 8, REG_T0);		/* env->frame->tmpvar[dst].val.i = tmp */
	}

	return true;
}

static INLINE bool
jit_visit_vindex_hint_op(struct rt_jit_context *ctx)
{
	int a;
	int b;
	int c;
	int id;
	int lanes;
	int flags;

	CONSUME_TMPVAR(a);
	CONSUME_TMPVAR(b);
	CONSUME_TMPVAR(c);
	CONSUME_IMM8(id);
	CONSUME_IMM8(lanes);
	CONSUME_IMM8(flags);

	UNUSED_PARAMETER(a);
	UNUSED_PARAMETER(b);
	UNUSED_PARAMETER(c);
	UNUSED_PARAMETER(id);
	UNUSED_PARAMETER(lanes);
	UNUSED_PARAMETER(flags);

	return true;
}

static INLINE bool
jit_visit_vori32x4i_op(struct rt_jit_context *ctx)
{
	int a;
	int b;
	int c;
	int d;

	CONSUME_IMM8(a);
	CONSUME_IMM8(b);
	CONSUME_IMM8(c);
	CONSUME_IMM8(d);

	UNUSED_PARAMETER(a);
	UNUSED_PARAMETER(b);
	UNUSED_PARAMETER(c);
	UNUSED_PARAMETER(d);

	return false;
}

static INLINE bool
jit_visit_vfmaf32x4_op(struct rt_jit_context *ctx)
{
	int a;
	int b;
	int c;
	int d;

	CONSUME_IMM8(a);
	CONSUME_IMM8(b);
	CONSUME_IMM8(c);
	CONSUME_IMM8(d);

	UNUSED_PARAMETER(a);
	UNUSED_PARAMETER(b);
	UNUSED_PARAMETER(c);
	UNUSED_PARAMETER(d);

	return false;
}

static INLINE bool
jit_visit_subjnz_op(struct rt_jit_context *ctx)
{
	int value;
	int decrement;
	uint32_t target_lpc;

	CONSUME_TMPVAR(value);
	CONSUME_IMM8(decrement);
	CONSUME_IMM32(target_lpc);

	if (target_lpc >= (uint32_t)(ctx->func->bytecode_size + 1)) {
		rt_error(ctx->env, BROKEN_BYTECODE);
		return false;
	}
	value *= (int)sizeof(struct rt_value);
	ASM {
		ORI(REG_T0, REG_ZERO, IMM12(value));
		ADD(REG_T0, REG_S11, REG_T0);
		LW(REG_T1, 8, REG_T0);
		ADDI(REG_T1, REG_T1, IMM12(-(int32_t)decrement));
		SW(REG_T1, 8, REG_T0);
		ADDI(REG_T0, REG_T1, IMM12(0));
		ORI(REG_T1, REG_ZERO, IMM12(0));
	}
	ctx->branch_patch[ctx->branch_patch_count].code = ctx->code;
	ctx->branch_patch[ctx->branch_patch_count].lpc = target_lpc;
	ctx->branch_patch[ctx->branch_patch_count].type = PATCH_BNE;
	ctx->branch_patch_count++;
	ASM {
		BNE(REG_T0, REG_T1, IMM13(0));
	}
	if (!jit_put_word(ctx, 0x00000013) || !jit_put_word(ctx, 0x00000013))
		return false;

	return true;
}

/* Visit a OP_ADD instruction. */
static INLINE bool
jit_visit_add_op(
	struct rt_jit_context *ctx)
{
	int dst;
	int src1;
	int src2;

	CONSUME_TMPVAR(dst);
	CONSUME_TMPVAR(src1);
	CONSUME_TMPVAR(src2);

	/* if (!ex_add_helper(env, dst, src1, src2)) return false; */
	ASM_BINARY_OP(ex_add_helper);

	return true;
}

/* Visit a OP_SUB instruction. */
static INLINE bool
jit_visit_sub_op(
	struct rt_jit_context *ctx)
{
	int dst;
	int src1;
	int src2;

	CONSUME_TMPVAR(dst);
	CONSUME_TMPVAR(src1);
	CONSUME_TMPVAR(src2);

	/* if (!ex_sub_helper(env, dst, src1, src2)) return false; */
	ASM_BINARY_OP(ex_sub_helper);

	return true;
}

/* Visit a OP_MUL instruction. */
static INLINE bool
jit_visit_mul_op(
	struct rt_jit_context *ctx)
{
	int dst;
	int src1;
	int src2;

	CONSUME_TMPVAR(dst);
	CONSUME_TMPVAR(src1);
	CONSUME_TMPVAR(src2);

	/* if (!ex_mul_helper(env, dst, src1, src2)) return false; */
	ASM_BINARY_OP(ex_mul_helper);

	return true;
}

/* Visit a OP_DIV instruction. */
static INLINE bool
jit_visit_div_op(
	struct rt_jit_context *ctx)
{
	int dst;
	int src1;
	int src2;

	CONSUME_TMPVAR(dst);
	CONSUME_TMPVAR(src1);
	CONSUME_TMPVAR(src2);

	/* if (!ex_div_helper(env, dst, src1, src2)) return false; */
	ASM_BINARY_OP(ex_div_helper);

	return true;
}

/* Visit a OP_MOD instruction. */
static INLINE bool
jit_visit_mod_op(
	struct rt_jit_context *ctx)
{
	int dst;
	int src1;
	int src2;

	CONSUME_TMPVAR(dst);
	CONSUME_TMPVAR(src1);
	CONSUME_TMPVAR(src2);

	/* if (!ex_mod_helper(env, dst, src1, src2)) return false; */
	ASM_BINARY_OP(ex_mod_helper);

	return true;
}

/* Visit a OP_AND instruction. */
static INLINE bool
jit_visit_and_op(
	struct rt_jit_context *ctx)
{
	int dst;
	int src1;
	int src2;

	CONSUME_TMPVAR(dst);
	CONSUME_TMPVAR(src1);
	CONSUME_TMPVAR(src2);

	/* if (!ex_and_helper(env, dst, src1, src2)) return false; */
	ASM_BINARY_OP(ex_and_helper);

	return true;
}

/* Visit a OP_OR instruction. */
static INLINE bool
jit_visit_or_op(
	struct rt_jit_context *ctx)
{
	int dst;
	int src1;
	int src2;

	CONSUME_TMPVAR(dst);
	CONSUME_TMPVAR(src1);
	CONSUME_TMPVAR(src2);

	/* if (!ex_or_helper(env, dst, src1, src2)) return false; */
	ASM_BINARY_OP(ex_or_helper);

	return true;
}

/* Visit a OP_XOR instruction. */
static INLINE bool
jit_visit_xor_op(
	struct rt_jit_context *ctx)
{
	int dst;
	int src1;
	int src2;

	CONSUME_TMPVAR(dst);
	CONSUME_TMPVAR(src1);
	CONSUME_TMPVAR(src2);

	/* if (!ex_xor_helper(env, dst, src1, src2)) return false; */
	ASM_BINARY_OP(ex_xor_helper);

	return true;
}

/* Visit a OP_SHL instruction. */
static INLINE bool
jit_visit_shl_op(
        struct rt_jit_context *ctx)
{
        int dst;
        int src1;
        int src2;

        CONSUME_TMPVAR(dst);
        CONSUME_TMPVAR(src1);
        CONSUME_TMPVAR(src2);

        /* if (!jit_shl_helper(env, dst, src1, src2)) return false; */
        ASM_BINARY_OP(ex_shl_helper);

        return true;
}

/* Visit a OP_SHR instruction. */
static INLINE bool
jit_visit_shr_op(
        struct rt_jit_context *ctx)
{
        int dst;
        int src1;
        int src2;

        CONSUME_TMPVAR(dst);
        CONSUME_TMPVAR(src1);
        CONSUME_TMPVAR(src2);

        /* if (!jit_shr_helper(env, dst, src1, src2)) return false; */
        ASM_BINARY_OP(ex_shr_helper);

        return true;
}

/* Visit a OP_NEG instruction. */
static INLINE bool
jit_visit_neg_op(
	struct rt_jit_context *ctx)
{
	int dst;
	int src;

	CONSUME_TMPVAR(dst);
	CONSUME_TMPVAR(src);

	/* if (!ex_neg_helper(env, dst, src)) return false; */
	ASM_UNARY_OP(ex_neg_helper);

	return true;
}

/* Visit a OP_NOT instruction. */
static INLINE bool
jit_visit_not_op(
	struct rt_jit_context *ctx)
{
	int dst;
	int src;

	CONSUME_TMPVAR(dst);
	CONSUME_TMPVAR(src);

	/* if (!ex_not_helper(env, dst, src)) return false; */
	ASM_UNARY_OP(ex_not_helper);

	return true;
}

/* Visit a OP_LT instruction. */
static INLINE bool
jit_visit_lt_op(
	struct rt_jit_context *ctx)
{
	int dst;
	int src1;
	int src2;

	CONSUME_TMPVAR(dst);
	CONSUME_TMPVAR(src1);
	CONSUME_TMPVAR(src2);

	/* if (!ex_lt_helper(env, dst, src1, src2)) return false; */
	ASM_BINARY_OP(ex_lt_helper);

	return true;
}

/* Visit a OP_LTE instruction. */
static INLINE bool
jit_visit_lte_op(
	struct rt_jit_context *ctx)
{
	int dst;
	int src1;
	int src2;

	CONSUME_TMPVAR(dst);
	CONSUME_TMPVAR(src1);
	CONSUME_TMPVAR(src2);

	/* if (!ex_lte_helper(env, dst, src1, src2)) return false; */
	ASM_BINARY_OP(ex_lte_helper);

	return true;
}

/* Visit a OP_EQ instruction. */
static INLINE bool
jit_visit_eq_op(
	struct rt_jit_context *ctx)
{
	int dst;
	int src1;
	int src2;

	CONSUME_TMPVAR(dst);
	CONSUME_TMPVAR(src1);
	CONSUME_TMPVAR(src2);

	/* if (!ex_eq_helper(env, dst, src1, src2)) return false; */
	ASM_BINARY_OP(ex_eq_helper);

	return true;
}

/* Visit a OP_NEQ instruction. */
static INLINE bool
jit_visit_neq_op(
	struct rt_jit_context *ctx)
{
	int dst;
	int src1;
	int src2;

	CONSUME_TMPVAR(dst);
	CONSUME_TMPVAR(src1);
	CONSUME_TMPVAR(src2);

	/* if (!ex_neq_helper(env, dst, src1, src2)) return false; */
	ASM_BINARY_OP(ex_neq_helper);

	return true;
}

/* Visit a OP_GTE instruction. */
static INLINE bool
jit_visit_gte_op(
	struct rt_jit_context *ctx)
{
	int dst;
	int src1;
	int src2;

	CONSUME_TMPVAR(dst);
	CONSUME_TMPVAR(src1);
	CONSUME_TMPVAR(src2);

	/* if (!ex_gte_helper(env, dst, src1, src2)) return false; */
	ASM_BINARY_OP(ex_gte_helper);

	return true;
}

/* Visit a OP_GT instruction. */
static INLINE bool
jit_visit_gt_op(
	struct rt_jit_context *ctx)
{
	int dst;
	int src1;
	int src2;

	CONSUME_TMPVAR(dst);
	CONSUME_TMPVAR(src1);
	CONSUME_TMPVAR(src2);

	/* if (!ex_gt_helper(env, dst, src1, src2)) return false; */
	ASM_BINARY_OP(ex_gt_helper);

	return true;
}

/* Visit a OP_EQI instruction. */
static INLINE bool
jit_visit_eqi_op(
	struct rt_jit_context *ctx)
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
		LW	(REG_T0, 8, REG_T0);

		/* t1 = &env->frame->tmpvar[src2].val.i */
		ORI	(REG_T1, REG_ZERO, IMM12(src2));
		ADD	(REG_T1, REG_T1, REG_S11);
		LW	(REG_T1, 8, REG_T1);

		/* Here, t0 = src1, t1 = src2 */
	}

	return true;
}

/* Visit a OP_LOADARRAY instruction. */
static INLINE bool
jit_visit_loadarray_op(
	struct rt_jit_context *ctx)
{
	int dst;
	int src1;
	int src2;

	CONSUME_TMPVAR(dst);
	CONSUME_TMPVAR(src1);
	CONSUME_TMPVAR(src2);

	/* if (!ex_loadarray_helper(env, dst, src1, src2)) return false; */
	ASM_BINARY_OP(ex_loadarray_helper);

	return true;
}

/* Visit a OP_STOREARRAY instruction. */
static INLINE bool
jit_visit_storearray_op(
	struct rt_jit_context *ctx)
{
	int dst;
	int src1;
	int src2;

	CONSUME_TMPVAR(dst);
	CONSUME_TMPVAR(src1);
	CONSUME_TMPVAR(src2);

	/* if (!ex_storearray_helper(env, dst, src1, src2)) return false; */
	ASM_BINARY_OP(ex_storearray_helper);

	return true;
}

/* Visit a OP_LEN instruction. */
static INLINE bool
jit_visit_len_op(
	struct rt_jit_context *ctx)
{
	int dst;
	int src;

	CONSUME_TMPVAR(dst);
	CONSUME_TMPVAR(src);

	/* if (!ex_len_helper(env, dst, src)) return false; */
	ASM_UNARY_OP(ex_len_helper);

	return true;
}

/* Visit a OP_GETDICTKEYBYINDEX instruction. */
static INLINE bool
jit_visit_getdictkeybyindex_op(
	struct rt_jit_context *ctx)
{
	int dst;
	int src1;
	int src2;

	CONSUME_TMPVAR(dst);
	CONSUME_TMPVAR(src1);
	CONSUME_TMPVAR(src2);

	/* if (!ex_getdictkeybyindex_helper(env, dst, src1, src2)) return false; */
	ASM_BINARY_OP(ex_getdictkeybyindex_helper);

	return true;
}

/* Visit a OP_GETDICTVALBYINDEX instruction. */
static INLINE bool
jit_visit_getdictvalbyindex_op(
	struct rt_jit_context *ctx)
{
	int dst;
	int src1;
	int src2;

	CONSUME_TMPVAR(dst);
	CONSUME_TMPVAR(src1);
	CONSUME_TMPVAR(src2);

	/* if (!ex_getdictvalbyindex_helper(env, dst, src1, src2)) return false; */
	ASM_BINARY_OP(ex_getdictvalbyindex_helper);

	return true;
}

/* Visit a OP_LOADSYMBOL instruction. */
static INLINE bool
jit_visit_loadsymbol_op(
	struct rt_jit_context *ctx)
{
	int dst;
	const char *src_s;
	uint32_t len, hash, src;

	CONSUME_TMPVAR(dst);
	CONSUME_STRING(src_s, len, hash);
	src = (uint32_t)(intptr_t)src_s;

	/* if (!ex_loadsymbol_helper(env, dst, src, len, hash)) return false; */
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

		/* Call ex_loadsymbol_helper(). */
		LI_32	(REG_T0, IMM32((uint32_t)ex_loadsymbol_helper));
		JALR	(REG_RA, IMM12(0), REG_T0);

		/* If failed: */
		EXCEPTION_IF_ZERO(REG_A0);
	}

	return true;
}

/* Visit a OP_STORESYMBOL instruction. */
static INLINE bool
jit_visit_storesymbol_op(
	struct rt_jit_context *ctx)
{
	const char *dst_s;
	uint32_t len, hash, dst;
	int src;

	CONSUME_STRING(dst_s, len, hash);
	CONSUME_TMPVAR(src);
	dst = (uint32_t)(intptr_t)dst_s;

	/* if (!ex_storesymbol_helper(env, dst, len, hash, src)) return false; */
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

		/* Call ex_storesymbol_helper(). */
		LI_32	(REG_T0, IMM32((uint32_t)ex_storesymbol_helper));
		JALR	(REG_RA, IMM12(0), REG_T0);

		/* If failed: */
		EXCEPTION_IF_ZERO(REG_A0);
	}

	return true;
}

/* Visit a OP_LOADDOT instruction. */
static INLINE bool
jit_visit_loaddot_op(
	struct rt_jit_context *ctx)
{
	int dst;
	int dict;
	const char *field_s;
	uint32_t len, hash, field;

	CONSUME_TMPVAR(dst);
	CONSUME_TMPVAR(dict);
	CONSUME_STRING(field_s, len, hash);
	field = (uint32_t)(intptr_t)field_s;

	/* if (!ex_loaddot_helper(env, dst, dict, field, len, hash)) return false; */
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

		/* Call ex_loaddot_helper(). */
		LI_32	(REG_T0, IMM32((uint32_t)ex_loaddot_helper));
		JALR	(REG_RA, IMM12(0), REG_T0);

		/* If failed: */
		EXCEPTION_IF_ZERO(REG_A0);
	}

	return true;
}

/* Visit a OP_STOREDOT instruction. */
static INLINE bool
jit_visit_storedot_op(
	struct rt_jit_context *ctx)
{
	int dict;
	const char *field_s;
	uint32_t len, hash, field;
	int src;

	CONSUME_TMPVAR(dict);
	CONSUME_STRING(field_s, len, hash);
	CONSUME_TMPVAR(src);
	field = (uint32_t)(intptr_t)field_s;

	/* if (!ex_storedot_helper(env, dict, field, len, hash, src)) return false; */
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

		/* Call ex_storedot_helper(). */
		LI_32	(REG_T0, IMM32((uint32_t)ex_storedot_helper));
		JALR	(REG_RA, IMM12(0), REG_T0);

		/* If failed: */
		EXCEPTION_IF_ZERO(REG_A0);
	}

	return true;
}

/* Visit a OP_CALL instruction. */
static inline bool
jit_visit_call_op(
	struct rt_jit_context *ctx)
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

	/* if (!ex_call_helper(env, dst, func, arg_count, arg)) return false; */
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

		/* Call ex_call_helper(). */
		LI_32	(REG_T0, IMM32((uint32_t)ex_call_helper));
		JALR	(REG_RA, IMM12(0), REG_T0);

		/* If failed: */
		EXCEPTION_IF_ZERO(REG_A0);
	}

	return true;
}

/* Visit a OP_THISCALL instruction. */
static inline bool
jit_visit_thiscall_op(
	struct rt_jit_context *ctx)
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
	CONSUME_TMPVAR(arg_tmp);
	symbol = NULL;
	len = 0;
	hash = (uint32_t)arg_tmp;
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

	/* if (!ex_thiscall_helper(env, dst, obj, symbol, arg_count, arg)) return false; */
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

		/* Call ex_thiscall_helper(). */
		LI_32	(REG_T0, IMM32((uint32_t)ex_thiscall_helper));
		JALR	(REG_RA, IMM12(0), REG_T0);

		/* If failed: */
		EXCEPTION_IF_ZERO(REG_A0);
	}

	return true;
}

/* Visit a OP_JMP instruction. */
static inline bool
jit_visit_jmp_op(
	struct rt_jit_context *ctx)
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
	if (!jit_put_word(ctx, 0x00000013))	/* reserved far-jump slot */
		return false;

	return true;
}

/* Visit a OP_JMPIFTRUE instruction. */
static inline bool
jit_visit_jmpiftrue_op(
	struct rt_jit_context *ctx)
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
		LW	(REG_T0, 8, REG_T0);

		/* Compare: env->frame->tmpvar[dst].val.i != 0 */
		ORI	(REG_T1, REG_ZERO, IMM12(0));
	}

	/* Patch later. */
	ctx->branch_patch[ctx->branch_patch_count].code = ctx->code;
	ctx->branch_patch[ctx->branch_patch_count].lpc = target_lpc;
	ctx->branch_patch[ctx->branch_patch_count].type = PATCH_BNE;
	ctx->branch_patch_count++;

	ASM {
		/* Patched later (two slots: a far target needs an
		   inverted branch + JAL pair; B-type reaches only
		   +-4KiB). */
		BNE	(REG_T0, REG_T1, IMM13(0));
	}
	if (!jit_put_word(ctx, 0x00000013))	/* nop */
		return false;
	if (!jit_put_word(ctx, 0x00000013))	/* reserved far-jump slot */
		return false;

	return true;
}

/* Visit a OP_JMPIFFALSE instruction. */
static inline bool
jit_visit_jmpiffalse_op(
	struct rt_jit_context *ctx)
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
		LW	(REG_T0, 8, REG_T0);

		/* Compare: env->frame->tmpvar[dst].val.i == 0 */
		ORI	(REG_T1, REG_ZERO, IMM12(0));
	}

	/* Patch later. */
	ctx->branch_patch[ctx->branch_patch_count].code = ctx->code;
	ctx->branch_patch[ctx->branch_patch_count].lpc = target_lpc;
	ctx->branch_patch[ctx->branch_patch_count].type = PATCH_BEQ;
	ctx->branch_patch_count++;

	ASM {
		/* Patched later (two slots: a far target needs an
		   inverted branch + JAL pair; B-type reaches only
		   +-4KiB). */
		BEQ	(REG_T0, REG_T1, IMM13(0));
	}
	if (!jit_put_word(ctx, 0x00000013))	/* nop */
		return false;
	if (!jit_put_word(ctx, 0x00000013))	/* reserved far-jump slot */
		return false;

	return true;
}

/* Visit a OP_JMPIFEQ instruction. */
static inline bool
jit_visit_jmpifeq_op(
	struct rt_jit_context *ctx)
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

		/* Patched later (two slots: a far target needs an
		   inverted branch + JAL pair; B-type reaches only
		   +-4KiB). */
		BEQ	(REG_T0, REG_T1, IMM13(0));
	}
	if (!jit_put_word(ctx, 0x00000013))	/* nop */
		return false;
	if (!jit_put_word(ctx, 0x00000013))	/* reserved far-jump slot */
		return false;

	return true;
}

/* Visit a OP_SAFEPOINT instruction. */
static INLINE bool
jit_visit_safepoint_op(
	struct rt_jit_context *ctx)
{
	/* if (!ex_safepoint_helper(env)) return false; */
	ASM {
		/* s10: env */
		/* s11: &env->frame->tmpvar[0] */

		/* Arg1 a0: env */
		MV	(REG_A0, REG_S10);

		/* Call ex_safepoint_helper(). */
		LI_32	(REG_T0, IMM32((uint32_t)ex_safepoint_helper));
		JALR	(REG_RA, IMM12(0), REG_T0);

		/* If failed: */
		EXCEPTION_IF_ZERO(REG_A0);
	}

	return true;
}

/* Visit a OP_PBASE instruction. (ABCE; inline machine code, rv32.)
 * The guard has proven the operand is a packed.  The 64-bit base
 * value stores the 32-bit pointer in the low word; the high word is
 * zeroed. */
static INLINE bool
jit_visit_pbase_op(
        struct rt_jit_context *ctx)
{
        int dst;
        int src;
        int base_id;
	uint32_t buf_ofs;

	CONSUME_TMPVAR(dst);
	CONSUME_TMPVAR(src);
	CONSUME_IMM8(base_id);

	UNUSED_PARAMETER(base_id);

	dst *= (int)sizeof(struct rt_value);
	src *= (int)sizeof(struct rt_value);
	buf_ofs = (uint32_t)offsetof(struct rt_packed, packed_buffer);

	ASM {
		/* s11: &env->frame->tmpvar[0] */

		LW	(REG_T1, IMM12(src + 8), REG_S11);
		LW	(REG_T1, IMM12(buf_ofs), REG_T1);
		ORI	(REG_T4, REG_ZERO, IMM12(NOCT_VALUE_LONG));
		SW	(REG_T4, IMM12(dst), REG_S11);
		SW	(REG_T1, IMM12(dst + 8), REG_S11);
		SW	(REG_ZERO, IMM12(dst + 12), REG_S11);
	}

	return true;
}

/* Visit a OP_PLEN instruction. (ABCE; helper-call implementation.) */
static INLINE bool
jit_visit_plen_op(
        struct rt_jit_context *ctx)
{
        int dst;
        int src;

	CONSUME_TMPVAR(dst);
	CONSUME_TMPVAR(src);

        /* if (!ex_plen_helper(env, dst, src)) return false; */
        ASM_UNARY_OP(ex_plen_helper);

        return true;
}

/* Visit a OP_PCHECK instruction. (ABCE; helper-call implementation.) */
static INLINE bool
jit_visit_pcheck_op(
        struct rt_jit_context *ctx)
{
        int dst;
        int src1;
        int src2;

        CONSUME_TMPVAR(dst);
        CONSUME_TMPVAR(src1);
        CONSUME_IMM8(src2);

        /* if (!ex_pcheck_helper(env, dst, src, type)) return false; */
        ASM_BINARY_OP(ex_pcheck_helper);

        return true;
}

/* Visit a OP_TYPEIS instruction. (ABCE; helper-call implementation.) */
static INLINE bool
jit_visit_typeis_op(
        struct rt_jit_context *ctx)
{
        int dst;
        int src1;
        int src2;

        CONSUME_TMPVAR(dst);
        CONSUME_TMPVAR(src1);
        CONSUME_IMM8(src2);

        /* if (!ex_typeis_helper(env, dst, src, type)) return false; */
        ASM_BINARY_OP(ex_typeis_helper);

        return true;
}

/* Visit a OP_PLOAD8U instruction. (ABCE; inline machine code, rv32.) */
static INLINE bool
jit_visit_pload8u_op(
	struct rt_jit_context *ctx)
{
	int dst;
	int base;
	int ofs;

	CONSUME_TMPVAR(dst);
	CONSUME_TMPVAR(base);
	CONSUME_TMPVAR(ofs);

	dst *= (int)sizeof(struct rt_value);
	base *= (int)sizeof(struct rt_value);
	ofs *= (int)sizeof(struct rt_value);

	ASM {
		/* s11: &env->frame->tmpvar[0] */

		/* t1 = base pointer (low word of the long) */
		LW	(REG_T1, IMM12(base + 8), REG_S11);
		/* t2 = element index */
		LW	(REG_T2, IMM12(ofs + 8), REG_S11);
		ADD	(REG_T1, REG_T1, REG_T2);
		/* t3 = loaded element */
		LBU	(REG_T3, 0, REG_T1);
		/* tag = INT */
		ORI	(REG_T4, REG_ZERO, IMM12(NOCT_VALUE_INT));
		SW	(REG_T4, IMM12(dst), REG_S11);
		SW	(REG_T3, IMM12(dst + 8), REG_S11);
	}

	return true;
}

/* Visit a OP_PSTORE8 instruction. (ABCE; inline, rv32. Int source per ABCE rules.) */
static INLINE bool
jit_visit_pstore8_op(
	struct rt_jit_context *ctx)
{
	int base;
	int ofs;
	int src;

	CONSUME_TMPVAR(base);
	CONSUME_TMPVAR(ofs);
	CONSUME_TMPVAR(src);

	base *= (int)sizeof(struct rt_value);
	ofs *= (int)sizeof(struct rt_value);
	src *= (int)sizeof(struct rt_value);

	ASM {
		/* s11: &env->frame->tmpvar[0] */

		LW	(REG_T1, IMM12(base + 8), REG_S11);
		LW	(REG_T2, IMM12(ofs + 8), REG_S11);
		ADD	(REG_T1, REG_T1, REG_T2);
		LW	(REG_T3, IMM12(src + 8), REG_S11);
		SB	(REG_T3, 0, REG_T1);
	}

	return true;
}

/* Visit a OP_CHECKTYPE instruction. (Typed entry check.) */
static INLINE bool
jit_visit_checktype_op(
        struct rt_jit_context *ctx)
{
        int dst;
        int src;

        CONSUME_TMPVAR(dst);
        CONSUME_IMM8(src);

        /* if (!ex_checktype_helper(env, slot, type)) return false; */
        ASM_UNARY_OP(ex_checktype_helper);

        return true;
}

/* Visit a OP_PLOAD8S instruction. (ABCE; inline machine code, rv32.) */
static INLINE bool
jit_visit_pload8s_op(
	struct rt_jit_context *ctx)
{
	int dst;
	int base;
	int ofs;

	CONSUME_TMPVAR(dst);
	CONSUME_TMPVAR(base);
	CONSUME_TMPVAR(ofs);

	dst *= (int)sizeof(struct rt_value);
	base *= (int)sizeof(struct rt_value);
	ofs *= (int)sizeof(struct rt_value);

	ASM {
		/* s11: &env->frame->tmpvar[0] */

		/* t1 = base pointer (low word of the long) */
		LW	(REG_T1, IMM12(base + 8), REG_S11);
		/* t2 = element index */
		LW	(REG_T2, IMM12(ofs + 8), REG_S11);
		ADD	(REG_T1, REG_T1, REG_T2);
		/* t3 = loaded element */
		LB	(REG_T3, 0, REG_T1);
		/* tag = INT */
		ORI	(REG_T4, REG_ZERO, IMM12(NOCT_VALUE_INT));
		SW	(REG_T4, IMM12(dst), REG_S11);
		SW	(REG_T3, IMM12(dst + 8), REG_S11);
	}

	return true;
}

/* Visit a OP_PLOAD16U instruction. (ABCE; inline machine code, rv32.) */
static INLINE bool
jit_visit_pload16u_op(
	struct rt_jit_context *ctx)
{
	int dst;
	int base;
	int ofs;

	CONSUME_TMPVAR(dst);
	CONSUME_TMPVAR(base);
	CONSUME_TMPVAR(ofs);

	dst *= (int)sizeof(struct rt_value);
	base *= (int)sizeof(struct rt_value);
	ofs *= (int)sizeof(struct rt_value);

	ASM {
		/* s11: &env->frame->tmpvar[0] */

		/* t1 = base pointer (low word of the long) */
		LW	(REG_T1, IMM12(base + 8), REG_S11);
		/* t2 = element index */
		LW	(REG_T2, IMM12(ofs + 8), REG_S11);
		SLLI	(REG_T2, REG_T2, 1);
		ADD	(REG_T1, REG_T1, REG_T2);
		/* t3 = loaded element */
		LHU	(REG_T3, 0, REG_T1);
		/* tag = INT */
		ORI	(REG_T4, REG_ZERO, IMM12(NOCT_VALUE_INT));
		SW	(REG_T4, IMM12(dst), REG_S11);
		SW	(REG_T3, IMM12(dst + 8), REG_S11);
	}

	return true;
}

/* Visit a OP_PLOAD16S instruction. (ABCE; inline machine code, rv32.) */
static INLINE bool
jit_visit_pload16s_op(
	struct rt_jit_context *ctx)
{
	int dst;
	int base;
	int ofs;

	CONSUME_TMPVAR(dst);
	CONSUME_TMPVAR(base);
	CONSUME_TMPVAR(ofs);

	dst *= (int)sizeof(struct rt_value);
	base *= (int)sizeof(struct rt_value);
	ofs *= (int)sizeof(struct rt_value);

	ASM {
		/* s11: &env->frame->tmpvar[0] */

		/* t1 = base pointer (low word of the long) */
		LW	(REG_T1, IMM12(base + 8), REG_S11);
		/* t2 = element index */
		LW	(REG_T2, IMM12(ofs + 8), REG_S11);
		SLLI	(REG_T2, REG_T2, 1);
		ADD	(REG_T1, REG_T1, REG_T2);
		/* t3 = loaded element */
		LH	(REG_T3, 0, REG_T1);
		/* tag = INT */
		ORI	(REG_T4, REG_ZERO, IMM12(NOCT_VALUE_INT));
		SW	(REG_T4, IMM12(dst), REG_S11);
		SW	(REG_T3, IMM12(dst + 8), REG_S11);
	}

	return true;
}

/* Visit a OP_PLOAD32 instruction. (ABCE; inline machine code, rv32.) */
static INLINE bool
jit_visit_pload32_op(
	struct rt_jit_context *ctx)
{
	int dst;
	int base;
	int ofs;

	CONSUME_TMPVAR(dst);
	CONSUME_TMPVAR(base);
	CONSUME_TMPVAR(ofs);

	dst *= (int)sizeof(struct rt_value);
	base *= (int)sizeof(struct rt_value);
	ofs *= (int)sizeof(struct rt_value);

	ASM {
		/* s11: &env->frame->tmpvar[0] */

		/* t1 = base pointer (low word of the long) */
		LW	(REG_T1, IMM12(base + 8), REG_S11);
		/* t2 = element index */
		LW	(REG_T2, IMM12(ofs + 8), REG_S11);
		SLLI	(REG_T2, REG_T2, 2);
		ADD	(REG_T1, REG_T1, REG_T2);
		/* t3 = loaded element */
		LW	(REG_T3, 0, REG_T1);
		/* tag = INT */
		ORI	(REG_T4, REG_ZERO, IMM12(NOCT_VALUE_INT));
		SW	(REG_T4, IMM12(dst), REG_S11);
		SW	(REG_T3, IMM12(dst + 8), REG_S11);
	}

	return true;
}

/* Visit a OP_PLOAD64 instruction. (ABCE width op; helper-call.) */
static INLINE bool
jit_visit_pload64_op(
        struct rt_jit_context *ctx)
{
        int dst;
        int src1;
        int src2;

        CONSUME_TMPVAR(dst);
        CONSUME_TMPVAR(src1);
        CONSUME_TMPVAR(src2);

        /* if (!ex_pload64_helper(env, a, b, c)) return false; */
        ASM_BINARY_OP(ex_pload64_helper);

        return true;
}

/* Visit a OP_PSTORE16 instruction. (ABCE; inline, rv32. Int source per ABCE rules.) */
static INLINE bool
jit_visit_pstore16_op(
	struct rt_jit_context *ctx)
{
	int base;
	int ofs;
	int src;

	CONSUME_TMPVAR(base);
	CONSUME_TMPVAR(ofs);
	CONSUME_TMPVAR(src);

	base *= (int)sizeof(struct rt_value);
	ofs *= (int)sizeof(struct rt_value);
	src *= (int)sizeof(struct rt_value);

	ASM {
		/* s11: &env->frame->tmpvar[0] */

		LW	(REG_T1, IMM12(base + 8), REG_S11);
		LW	(REG_T2, IMM12(ofs + 8), REG_S11);
		SLLI	(REG_T2, REG_T2, 1);
		ADD	(REG_T1, REG_T1, REG_T2);
		LW	(REG_T3, IMM12(src + 8), REG_S11);
		SH	(REG_T3, 0, REG_T1);
	}

	return true;
}

/* Visit a OP_PSTORE32 instruction. (ABCE; inline, rv32. Int source per ABCE rules.) */
static INLINE bool
jit_visit_pstore32_op(
	struct rt_jit_context *ctx)
{
	int base;
	int ofs;
	int src;

	CONSUME_TMPVAR(base);
	CONSUME_TMPVAR(ofs);
	CONSUME_TMPVAR(src);

	base *= (int)sizeof(struct rt_value);
	ofs *= (int)sizeof(struct rt_value);
	src *= (int)sizeof(struct rt_value);

	ASM {
		/* s11: &env->frame->tmpvar[0] */

		LW	(REG_T1, IMM12(base + 8), REG_S11);
		LW	(REG_T2, IMM12(ofs + 8), REG_S11);
		SLLI	(REG_T2, REG_T2, 2);
		ADD	(REG_T1, REG_T1, REG_T2);
		LW	(REG_T3, IMM12(src + 8), REG_S11);
		SW	(REG_T3, 0, REG_T1);
	}

	return true;
}

/* Visit a OP_PSTORE64 instruction. (ABCE width op; helper-call.) */
static INLINE bool
jit_visit_pstore64_op(
        struct rt_jit_context *ctx)
{
        int dst;
        int src1;
        int src2;

        CONSUME_TMPVAR(dst);
        CONSUME_TMPVAR(src1);
        CONSUME_TMPVAR(src2);

        /* if (!ex_pstore64_helper(env, a, b, c)) return false; */
        ASM_BINARY_OP(ex_pstore64_helper);

        return true;
}

/* Visit a OP_PLOADF32 instruction. (ABCE float32 width op; helper-call.) */
static INLINE bool
jit_visit_ploadf32_op(
        struct rt_jit_context *ctx)
{
        int dst;
        int src1;
        int src2;

        CONSUME_TMPVAR(dst);
        CONSUME_TMPVAR(src1);
        CONSUME_TMPVAR(src2);

        ASM_BINARY_OP(ex_ploadf32_helper);
        return true;
}

/* Visit a OP_PSTOREF32 instruction. (ABCE float32 width op; helper-call.) */
static INLINE bool
jit_visit_pstoref32_op(
        struct rt_jit_context *ctx)
{
        int dst;
        int src1;
        int src2;

        CONSUME_TMPVAR(dst);
        CONSUME_TMPVAR(src1);
        CONSUME_TMPVAR(src2);

        ASM_BINARY_OP(ex_pstoref32_helper);
        return true;
}

/* Visit an OP_IADD instruction. */
static INLINE bool
jit_visit_iadd_op(
        struct rt_jit_context *ctx)
{
        int dst;
        int src1;
        int src2;

        CONSUME_TMPVAR(dst);
        CONSUME_TMPVAR(src1);
        CONSUME_TMPVAR(src2);

        ASM_BINARY_OP(ex_iadd_helper);
        return true;
}

/* Visit an OP_ISUB instruction. */
static INLINE bool
jit_visit_isub_op(
        struct rt_jit_context *ctx)
{
        int dst;
        int src1;
        int src2;

        CONSUME_TMPVAR(dst);
        CONSUME_TMPVAR(src1);
        CONSUME_TMPVAR(src2);

        ASM_BINARY_OP(ex_isub_helper);
        return true;
}

/* Visit an OP_IMUL instruction. */
static INLINE bool
jit_visit_imul_op(
        struct rt_jit_context *ctx)
{
        int dst;
        int src1;
        int src2;

        CONSUME_TMPVAR(dst);
        CONSUME_TMPVAR(src1);
        CONSUME_TMPVAR(src2);

        ASM_BINARY_OP(ex_imul_helper);
        return true;
}

/* Visit an OP_IDIV instruction. */
static INLINE bool
jit_visit_idiv_op(
        struct rt_jit_context *ctx)
{
        int dst;
        int src1;
        int src2;

        CONSUME_TMPVAR(dst);
        CONSUME_TMPVAR(src1);
        CONSUME_TMPVAR(src2);

        ASM_BINARY_OP(ex_idiv_helper);
        return true;
}

/* Visit an OP_IMOD instruction. */
static INLINE bool
jit_visit_imod_op(
        struct rt_jit_context *ctx)
{
        int dst;
        int src1;
        int src2;

        CONSUME_TMPVAR(dst);
        CONSUME_TMPVAR(src1);
        CONSUME_TMPVAR(src2);

        ASM_BINARY_OP(ex_imod_helper);
        return true;
}

/* Visit an OP_IAND instruction. */
static INLINE bool
jit_visit_iand_op(
        struct rt_jit_context *ctx)
{
        int dst;
        int src1;
        int src2;

        CONSUME_TMPVAR(dst);
        CONSUME_TMPVAR(src1);
        CONSUME_TMPVAR(src2);

        ASM_BINARY_OP(ex_iand_helper);
        return true;
}

/* Visit an OP_IOR instruction. */
static INLINE bool
jit_visit_ior_op(
        struct rt_jit_context *ctx)
{
        int dst;
        int src1;
        int src2;

        CONSUME_TMPVAR(dst);
        CONSUME_TMPVAR(src1);
        CONSUME_TMPVAR(src2);

        ASM_BINARY_OP(ex_ior_helper);
        return true;
}

/* Visit an OP_IXOR instruction. */
static INLINE bool
jit_visit_ixor_op(
        struct rt_jit_context *ctx)
{
        int dst;
        int src1;
        int src2;

        CONSUME_TMPVAR(dst);
        CONSUME_TMPVAR(src1);
        CONSUME_TMPVAR(src2);

        ASM_BINARY_OP(ex_ixor_helper);
        return true;
}

/* Visit an OP_ISHL instruction. */
static INLINE bool
jit_visit_ishl_op(
        struct rt_jit_context *ctx)
{
        int dst;
        int src1;
        int src2;

        CONSUME_TMPVAR(dst);
        CONSUME_TMPVAR(src1);
        CONSUME_IMM8(src2);

        ASM_BINARY_OP(ex_ishl_helper);
        return true;
}

/* Visit an OP_ISHR instruction. */
static INLINE bool
jit_visit_ishr_op(
        struct rt_jit_context *ctx)
{
        int dst;
        int src1;
        int src2;

        CONSUME_TMPVAR(dst);
        CONSUME_TMPVAR(src1);
        CONSUME_IMM8(src2);

        ASM_BINARY_OP(ex_ishr_helper);
        return true;
}

/* Visit an OP_ILT instruction. */
static INLINE bool
jit_visit_ilt_op(
        struct rt_jit_context *ctx)
{
        int dst;
        int src1;
        int src2;

        CONSUME_TMPVAR(dst);
        CONSUME_TMPVAR(src1);
        CONSUME_TMPVAR(src2);

        ASM_BINARY_OP(ex_ilt_helper);
        return true;
}

/* Visit an OP_ILTE instruction. */
static INLINE bool
jit_visit_ilte_op(
        struct rt_jit_context *ctx)
{
        int dst;
        int src1;
        int src2;

        CONSUME_TMPVAR(dst);
        CONSUME_TMPVAR(src1);
        CONSUME_TMPVAR(src2);

        ASM_BINARY_OP(ex_ilte_helper);
        return true;
}

/* Visit an OP_IGT instruction. */
static INLINE bool
jit_visit_igt_op(
        struct rt_jit_context *ctx)
{
        int dst;
        int src1;
        int src2;

        CONSUME_TMPVAR(dst);
        CONSUME_TMPVAR(src1);
        CONSUME_TMPVAR(src2);

        ASM_BINARY_OP(ex_igt_helper);
        return true;
}

/* Visit an OP_IGTE instruction. */
static INLINE bool
jit_visit_igte_op(
        struct rt_jit_context *ctx)
{
        int dst;
        int src1;
        int src2;

        CONSUME_TMPVAR(dst);
        CONSUME_TMPVAR(src1);
        CONSUME_TMPVAR(src2);

        ASM_BINARY_OP(ex_igte_helper);
        return true;
}

/* Visit an OP_FADD instruction. */
static INLINE bool
jit_visit_fadd_op(
        struct rt_jit_context *ctx)
{
        int dst;
        int src1;
        int src2;

        CONSUME_TMPVAR(dst);
        CONSUME_TMPVAR(src1);
        CONSUME_TMPVAR(src2);

        ASM_BINARY_OP(ex_fadd_helper);
        return true;
}

/* Visit an OP_FSUB instruction. */
static INLINE bool
jit_visit_fsub_op(
        struct rt_jit_context *ctx)
{
        int dst;
        int src1;
        int src2;

        CONSUME_TMPVAR(dst);
        CONSUME_TMPVAR(src1);
        CONSUME_TMPVAR(src2);

        ASM_BINARY_OP(ex_fsub_helper);
        return true;
}

/* Visit an OP_FMUL instruction. */
static INLINE bool
jit_visit_fmul_op(
        struct rt_jit_context *ctx)
{
        int dst;
        int src1;
        int src2;

        CONSUME_TMPVAR(dst);
        CONSUME_TMPVAR(src1);
        CONSUME_TMPVAR(src2);

        ASM_BINARY_OP(ex_fmul_helper);
        return true;
}

/* Visit an OP_FDIV instruction. */
static INLINE bool
jit_visit_fdiv_op(
        struct rt_jit_context *ctx)
{
        int dst;
        int src1;
        int src2;

        CONSUME_TMPVAR(dst);
        CONSUME_TMPVAR(src1);
        CONSUME_TMPVAR(src2);

        ASM_BINARY_OP(ex_fdiv_helper);
        return true;
}

/* Visit an OP_FLT instruction. */
static INLINE bool
jit_visit_flt_op(
        struct rt_jit_context *ctx)
{
        int dst;
        int src1;
        int src2;

        CONSUME_TMPVAR(dst);
        CONSUME_TMPVAR(src1);
        CONSUME_TMPVAR(src2);

        ASM_BINARY_OP(ex_flt_helper);
        return true;
}

/* Visit an OP_FLTE instruction. */
static INLINE bool
jit_visit_flte_op(
        struct rt_jit_context *ctx)
{
        int dst;
        int src1;
        int src2;

        CONSUME_TMPVAR(dst);
        CONSUME_TMPVAR(src1);
        CONSUME_TMPVAR(src2);

        ASM_BINARY_OP(ex_flte_helper);
        return true;
}

/* Visit an OP_FGT instruction. */
static INLINE bool
jit_visit_fgt_op(
        struct rt_jit_context *ctx)
{
        int dst;
        int src1;
        int src2;

        CONSUME_TMPVAR(dst);
        CONSUME_TMPVAR(src1);
        CONSUME_TMPVAR(src2);

        ASM_BINARY_OP(ex_fgt_helper);
        return true;
}

/* Visit an OP_FGTE instruction. */
static INLINE bool
jit_visit_fgte_op(
        struct rt_jit_context *ctx)
{
        int dst;
        int src1;
        int src2;

        CONSUME_TMPVAR(dst);
        CONSUME_TMPVAR(src1);
        CONSUME_TMPVAR(src2);

        ASM_BINARY_OP(ex_fgte_helper);
        return true;
}

/* Visit an OP_IDIV_CHECKED instruction. */
static INLINE bool
jit_visit_idiv_checked_op(
        struct rt_jit_context *ctx)
{
        int dst;
        int src1;
        int src2;

        CONSUME_TMPVAR(dst);
        CONSUME_TMPVAR(src1);
        CONSUME_TMPVAR(src2);

        ASM_BINARY_OP(ex_idiv_helper);
        return true;
}

/* Visit an OP_IMOD_CHECKED instruction. */
static INLINE bool
jit_visit_imod_checked_op(
        struct rt_jit_context *ctx)
{
        int dst;
        int src1;
        int src2;

        CONSUME_TMPVAR(dst);
        CONSUME_TMPVAR(src1);
        CONSUME_TMPVAR(src2);

        ASM_BINARY_OP(ex_imod_helper);
        return true;
}

/*
 * 128-bit vector ops use direct scalar lane operations over env->vreg.
 */

static bool
jit_put_fsw(struct rt_jit_context *ctx, uint32_t fs, uint32_t imm,
	    uint32_t rs)
{
	return jit_put_word(ctx, (((imm & 0xfff) >> 5) << 25) |
			    (fs << 20) | (rs << 15) | (2 << 12) |
			    ((imm & 0x1f) << 7) | 0x27);
}

/* Visit an OP_VLOADI32X4 instruction. */
static INLINE bool
jit_visit_vloadi32x4_op(
        struct rt_jit_context *ctx)
{
        uint32_t vreg_ofs;
        uint32_t vreg_upper;
        uint32_t vreg_lower;
        int vd;
        int base_tmp;
        int ofs_tmp;
        int lane;
        int base;
        int ofs;

        CONSUME_IMM8(vd);
        CONSUME_TMPVAR(base_tmp);
        CONSUME_TMPVAR(ofs_tmp);

        vreg_ofs = (uint32_t)offsetof(struct rt_env, vreg);
        vreg_upper = (vreg_ofs + 0x800) >> 12;
        vreg_lower = vreg_ofs & 0xfff;
        if (!jit_put_word(ctx, (vreg_upper << 12) |
                          (REG_T4 << 7) | 0x37))
                return false;
        ADDI(REG_T4, REG_T4, vreg_lower);
        ADD(REG_T4, REG_S10, REG_T4);

        base = base_tmp * (int)sizeof(struct rt_value) + 8;
        ofs = ofs_tmp * (int)sizeof(struct rt_value) + 8;
        LW(REG_T0, IMM12(base), REG_S11);
        LW(REG_T1, IMM12(ofs), REG_S11);
        SLLI(REG_T1, REG_T1, 2);
        ADD(REG_T0, REG_T0, REG_T1);
        for (lane = 0; lane < 4; lane++) {
                LW(REG_T2, lane * 4, REG_T0);
                SW(REG_T2, vd * 16 + lane * 4, REG_T4);
        }
        return true;
}

/* Visit an OP_VSTOREI32X4 instruction. */
static INLINE bool
jit_visit_vstorei32x4_op(
        struct rt_jit_context *ctx)
{
        uint32_t vreg_ofs;
        uint32_t vreg_upper;
        uint32_t vreg_lower;
        int base_tmp;
        int ofs_tmp;
        int vs;
        int lane;
        int base;
        int ofs;

        CONSUME_TMPVAR(base_tmp);
        CONSUME_TMPVAR(ofs_tmp);
        CONSUME_IMM8(vs);

        vreg_ofs = (uint32_t)offsetof(struct rt_env, vreg);
        vreg_upper = (vreg_ofs + 0x800) >> 12;
        vreg_lower = vreg_ofs & 0xfff;
        if (!jit_put_word(ctx, (vreg_upper << 12) |
                          (REG_T4 << 7) | 0x37))
                return false;
        ADDI(REG_T4, REG_T4, vreg_lower);
        ADD(REG_T4, REG_S10, REG_T4);

        base = base_tmp * (int)sizeof(struct rt_value) + 8;
        ofs = ofs_tmp * (int)sizeof(struct rt_value) + 8;
        LW(REG_T0, IMM12(base), REG_S11);
        LW(REG_T1, IMM12(ofs), REG_S11);
        SLLI(REG_T1, REG_T1, 2);
        ADD(REG_T0, REG_T0, REG_T1);
        for (lane = 0; lane < 4; lane++) {
                LW(REG_T2, vs * 16 + lane * 4, REG_T4);
                SW(REG_T2, lane * 4, REG_T0);
        }
        return true;
}

/* Visit an OP_VSPLATI32 instruction. */
static INLINE bool
jit_visit_vsplati32_op(
        struct rt_jit_context *ctx)
{
        uint32_t vreg_ofs;
        uint32_t vreg_upper;
        uint32_t vreg_lower;
        int vd;
        int src_tmp;
        int lane;
        int src;

        CONSUME_IMM8(vd);
        CONSUME_TMPVAR(src_tmp);

        vreg_ofs = (uint32_t)offsetof(struct rt_env, vreg);
        vreg_upper = (vreg_ofs + 0x800) >> 12;
        vreg_lower = vreg_ofs & 0xfff;
        if (!jit_put_word(ctx, (vreg_upper << 12) |
                          (REG_T4 << 7) | 0x37))
                return false;
        ADDI(REG_T4, REG_T4, vreg_lower);
        ADD(REG_T4, REG_S10, REG_T4);

        src = src_tmp * (int)sizeof(struct rt_value) + 8;
        LW(REG_T2, IMM12(src), REG_S11);
        for (lane = 0; lane < 4; lane++)
                SW(REG_T2, vd * 16 + lane * 4, REG_T4);
        return true;
}

/* Visit an OP_VGETLANEI32 instruction. */
static INLINE bool
jit_visit_vgetlanei32_op(
        struct rt_jit_context *ctx)
{
        uint32_t vreg_ofs;
        uint32_t vreg_upper;
        uint32_t vreg_lower;
        int dst_tmp;
        int vs;
        int lane_index;
        int d;
        uint32_t tag;

        CONSUME_TMPVAR(dst_tmp);
        CONSUME_IMM8(vs);
        CONSUME_IMM8(lane_index);

        vreg_ofs = (uint32_t)offsetof(struct rt_env, vreg);
        vreg_upper = (vreg_ofs + 0x800) >> 12;
        vreg_lower = vreg_ofs & 0xfff;
        if (!jit_put_word(ctx, (vreg_upper << 12) |
                          (REG_T4 << 7) | 0x37))
                return false;
        ADDI(REG_T4, REG_T4, vreg_lower);
        ADD(REG_T4, REG_S10, REG_T4);

        d = dst_tmp * (int)sizeof(struct rt_value);
        tag = (uint32_t)(NOCT_VALUE_INT);
        LW(REG_T2, vs * 16 + lane_index * 4, REG_T4);
        ORI(REG_T3, REG_ZERO, tag);
        SW(REG_T3, IMM12(d), REG_S11);
        SW(REG_T2, IMM12(d + 8), REG_S11);
        return true;
}

/* Visit an OP_VMOV128 instruction. */
static INLINE bool
jit_visit_vmov128_op(
        struct rt_jit_context *ctx)
{
        uint32_t vreg_ofs;
        uint32_t vreg_upper;
        uint32_t vreg_lower;
        int vd;
        int vs;
        int lane;

        CONSUME_IMM8(vd);
        CONSUME_IMM8(vs);

        vreg_ofs = (uint32_t)offsetof(struct rt_env, vreg);
        vreg_upper = (vreg_ofs + 0x800) >> 12;
        vreg_lower = vreg_ofs & 0xfff;
        if (!jit_put_word(ctx, (vreg_upper << 12) |
                          (REG_T4 << 7) | 0x37))
                return false;
        ADDI(REG_T4, REG_T4, vreg_lower);
        ADD(REG_T4, REG_S10, REG_T4);

        for (lane = 0; lane < 4; lane++) {
                LW(REG_T2, vs * 16 + lane * 4, REG_T4);
                SW(REG_T2, vd * 16 + lane * 4, REG_T4);
        }
        return true;
}

/* Visit an OP_VADDI32X4 instruction. */
static INLINE bool
jit_visit_vaddi32x4_op(
        struct rt_jit_context *ctx)
{
        uint32_t vreg_ofs;
        uint32_t vreg_upper;
        uint32_t vreg_lower;
        int vd;
        int lhs;
        int rhs;
        int lane;

        CONSUME_IMM8(vd);
        CONSUME_IMM8(lhs);
        CONSUME_IMM8(rhs);

        vreg_ofs = (uint32_t)offsetof(struct rt_env, vreg);
        vreg_upper = (vreg_ofs + 0x800) >> 12;
        vreg_lower = vreg_ofs & 0xfff;
        if (!jit_put_word(ctx, (vreg_upper << 12) |
                          (REG_T4 << 7) | 0x37))
                return false;
        ADDI(REG_T4, REG_T4, vreg_lower);
        ADD(REG_T4, REG_S10, REG_T4);

        for (lane = 0; lane < 4; lane++) {
                uint32_t word;

                LW(REG_T0, lhs * 16 + lane * 4, REG_T4);
                LW(REG_T1, rhs * 16 + lane * 4, REG_T4);
                word = 0x006283b3;
                if (!jit_put_word(ctx, word))
                        return false;
                SW(REG_T2, vd * 16 + lane * 4, REG_T4);
        }
        return true;
}

/* Visit an OP_VSUBI32X4 instruction. */
static INLINE bool
jit_visit_vsubi32x4_op(
        struct rt_jit_context *ctx)
{
        uint32_t vreg_ofs;
        uint32_t vreg_upper;
        uint32_t vreg_lower;
        int vd;
        int lhs;
        int rhs;
        int lane;

        CONSUME_IMM8(vd);
        CONSUME_IMM8(lhs);
        CONSUME_IMM8(rhs);

        vreg_ofs = (uint32_t)offsetof(struct rt_env, vreg);
        vreg_upper = (vreg_ofs + 0x800) >> 12;
        vreg_lower = vreg_ofs & 0xfff;
        if (!jit_put_word(ctx, (vreg_upper << 12) |
                          (REG_T4 << 7) | 0x37))
                return false;
        ADDI(REG_T4, REG_T4, vreg_lower);
        ADD(REG_T4, REG_S10, REG_T4);

        for (lane = 0; lane < 4; lane++) {
                uint32_t word;

                LW(REG_T0, lhs * 16 + lane * 4, REG_T4);
                LW(REG_T1, rhs * 16 + lane * 4, REG_T4);
                word = 0x406283b3;
                if (!jit_put_word(ctx, word))
                        return false;
                SW(REG_T2, vd * 16 + lane * 4, REG_T4);
        }
        return true;
}

/* Visit an OP_VMULI32X4 instruction. */
static INLINE bool
jit_visit_vmuli32x4_op(
        struct rt_jit_context *ctx)
{
        uint32_t vreg_ofs;
        uint32_t vreg_upper;
        uint32_t vreg_lower;
        int vd;
        int lhs;
        int rhs;
        int lane;

        CONSUME_IMM8(vd);
        CONSUME_IMM8(lhs);
        CONSUME_IMM8(rhs);

        vreg_ofs = (uint32_t)offsetof(struct rt_env, vreg);
        vreg_upper = (vreg_ofs + 0x800) >> 12;
        vreg_lower = vreg_ofs & 0xfff;
        if (!jit_put_word(ctx, (vreg_upper << 12) |
                          (REG_T4 << 7) | 0x37))
                return false;
        ADDI(REG_T4, REG_T4, vreg_lower);
        ADD(REG_T4, REG_S10, REG_T4);

        for (lane = 0; lane < 4; lane++) {
                uint32_t word;

                LW(REG_T0, lhs * 16 + lane * 4, REG_T4);
                LW(REG_T1, rhs * 16 + lane * 4, REG_T4);
                word = 0x026283b3;
                if (!jit_put_word(ctx, word))
                        return false;
                SW(REG_T2, vd * 16 + lane * 4, REG_T4);
        }
        return true;
}

/* Visit an OP_VAND128 instruction. */
static INLINE bool
jit_visit_vand128_op(
        struct rt_jit_context *ctx)
{
        uint32_t vreg_ofs;
        uint32_t vreg_upper;
        uint32_t vreg_lower;
        int vd;
        int lhs;
        int rhs;
        int lane;

        CONSUME_IMM8(vd);
        CONSUME_IMM8(lhs);
        CONSUME_IMM8(rhs);

        vreg_ofs = (uint32_t)offsetof(struct rt_env, vreg);
        vreg_upper = (vreg_ofs + 0x800) >> 12;
        vreg_lower = vreg_ofs & 0xfff;
        if (!jit_put_word(ctx, (vreg_upper << 12) |
                          (REG_T4 << 7) | 0x37))
                return false;
        ADDI(REG_T4, REG_T4, vreg_lower);
        ADD(REG_T4, REG_S10, REG_T4);

        for (lane = 0; lane < 4; lane++) {
                uint32_t word;

                LW(REG_T0, lhs * 16 + lane * 4, REG_T4);
                LW(REG_T1, rhs * 16 + lane * 4, REG_T4);
                word = 0x0062f3b3;
                if (!jit_put_word(ctx, word))
                        return false;
                SW(REG_T2, vd * 16 + lane * 4, REG_T4);
        }
        return true;
}

/* Visit an OP_VOR128 instruction. */
static INLINE bool
jit_visit_vor128_op(
        struct rt_jit_context *ctx)
{
        uint32_t vreg_ofs;
        uint32_t vreg_upper;
        uint32_t vreg_lower;
        int vd;
        int lhs;
        int rhs;
        int lane;

        CONSUME_IMM8(vd);
        CONSUME_IMM8(lhs);
        CONSUME_IMM8(rhs);

        vreg_ofs = (uint32_t)offsetof(struct rt_env, vreg);
        vreg_upper = (vreg_ofs + 0x800) >> 12;
        vreg_lower = vreg_ofs & 0xfff;
        if (!jit_put_word(ctx, (vreg_upper << 12) |
                          (REG_T4 << 7) | 0x37))
                return false;
        ADDI(REG_T4, REG_T4, vreg_lower);
        ADD(REG_T4, REG_S10, REG_T4);

        for (lane = 0; lane < 4; lane++) {
                uint32_t word;

                LW(REG_T0, lhs * 16 + lane * 4, REG_T4);
                LW(REG_T1, rhs * 16 + lane * 4, REG_T4);
                word = 0x0062e3b3;
                if (!jit_put_word(ctx, word))
                        return false;
                SW(REG_T2, vd * 16 + lane * 4, REG_T4);
        }
        return true;
}

/* Visit an OP_VXOR128 instruction. */
static INLINE bool
jit_visit_vxor128_op(
        struct rt_jit_context *ctx)
{
        uint32_t vreg_ofs;
        uint32_t vreg_upper;
        uint32_t vreg_lower;
        int vd;
        int lhs;
        int rhs;
        int lane;

        CONSUME_IMM8(vd);
        CONSUME_IMM8(lhs);
        CONSUME_IMM8(rhs);

        vreg_ofs = (uint32_t)offsetof(struct rt_env, vreg);
        vreg_upper = (vreg_ofs + 0x800) >> 12;
        vreg_lower = vreg_ofs & 0xfff;
        if (!jit_put_word(ctx, (vreg_upper << 12) |
                          (REG_T4 << 7) | 0x37))
                return false;
        ADDI(REG_T4, REG_T4, vreg_lower);
        ADD(REG_T4, REG_S10, REG_T4);

        for (lane = 0; lane < 4; lane++) {
                uint32_t word;

                LW(REG_T0, lhs * 16 + lane * 4, REG_T4);
                LW(REG_T1, rhs * 16 + lane * 4, REG_T4);
                word = 0x0062c3b3;
                if (!jit_put_word(ctx, word))
                        return false;
                SW(REG_T2, vd * 16 + lane * 4, REG_T4);
        }
        return true;
}

/* Visit an OP_VSHLI32X4 instruction. */
static INLINE bool
jit_visit_vshli32x4_op(
        struct rt_jit_context *ctx)
{
        uint32_t vreg_ofs;
        uint32_t vreg_upper;
        uint32_t vreg_lower;
        int vd;
        int vs;
        int shift;
        int lane;

        CONSUME_IMM8(vd);
        CONSUME_IMM8(vs);
        CONSUME_IMM8(shift);

        vreg_ofs = (uint32_t)offsetof(struct rt_env, vreg);
        vreg_upper = (vreg_ofs + 0x800) >> 12;
        vreg_lower = vreg_ofs & 0xfff;
        if (!jit_put_word(ctx, (vreg_upper << 12) |
                          (REG_T4 << 7) | 0x37))
                return false;
        ADDI(REG_T4, REG_T4, vreg_lower);
        ADD(REG_T4, REG_S10, REG_T4);

        ORI(REG_T3, REG_ZERO, (uint32_t)shift & 31u);
        for (lane = 0; lane < 4; lane++) {
                LW(REG_T0, vs * 16 + lane * 4, REG_T4);
                if (!jit_put_word(ctx, 0x01c293b3))
                        return false;
                SW(REG_T2, vd * 16 + lane * 4, REG_T4);
        }
        return true;
}

/* Visit an OP_VSHRI32X4 instruction. */
static INLINE bool
jit_visit_vshri32x4_op(
        struct rt_jit_context *ctx)
{
        uint32_t vreg_ofs;
        uint32_t vreg_upper;
        uint32_t vreg_lower;
        int vd;
        int vs;
        int shift;
        int lane;

        CONSUME_IMM8(vd);
        CONSUME_IMM8(vs);
        CONSUME_IMM8(shift);

        vreg_ofs = (uint32_t)offsetof(struct rt_env, vreg);
        vreg_upper = (vreg_ofs + 0x800) >> 12;
        vreg_lower = vreg_ofs & 0xfff;
        if (!jit_put_word(ctx, (vreg_upper << 12) |
                          (REG_T4 << 7) | 0x37))
                return false;
        ADDI(REG_T4, REG_T4, vreg_lower);
        ADD(REG_T4, REG_S10, REG_T4);

        ORI(REG_T3, REG_ZERO, (uint32_t)shift & 31u);
        for (lane = 0; lane < 4; lane++) {
                LW(REG_T0, vs * 16 + lane * 4, REG_T4);
                if (!jit_put_word(ctx, 0x01c2d3b3))
                        return false;
                SW(REG_T2, vd * 16 + lane * 4, REG_T4);
        }
        return true;
}

/* Visit an OP_VLOADF32X4 instruction. */
static INLINE bool
jit_visit_vloadf32x4_op(
        struct rt_jit_context *ctx)
{
        uint32_t vreg_ofs;
        uint32_t vreg_upper;
        uint32_t vreg_lower;
        int vd;
        int base_tmp;
        int ofs_tmp;
        int lane;
        int base;
        int ofs;

        CONSUME_IMM8(vd);
        CONSUME_TMPVAR(base_tmp);
        CONSUME_TMPVAR(ofs_tmp);

        vreg_ofs = (uint32_t)offsetof(struct rt_env, vreg);
        vreg_upper = (vreg_ofs + 0x800) >> 12;
        vreg_lower = vreg_ofs & 0xfff;
        if (!jit_put_word(ctx, (vreg_upper << 12) |
                          (REG_T4 << 7) | 0x37))
                return false;
        ADDI(REG_T4, REG_T4, vreg_lower);
        ADD(REG_T4, REG_S10, REG_T4);

        base = base_tmp * (int)sizeof(struct rt_value) + 8;
        ofs = ofs_tmp * (int)sizeof(struct rt_value) + 8;
        LW(REG_T0, IMM12(base), REG_S11);
        LW(REG_T1, IMM12(ofs), REG_S11);
        SLLI(REG_T1, REG_T1, 2);
        ADD(REG_T0, REG_T0, REG_T1);
        for (lane = 0; lane < 4; lane++) {
                LW(REG_T2, lane * 4, REG_T0);
                SW(REG_T2, vd * 16 + lane * 4, REG_T4);
        }
        return true;
}

/* Visit an OP_VSTOREF32X4 instruction. */
static INLINE bool
jit_visit_vstoref32x4_op(
        struct rt_jit_context *ctx)
{
        uint32_t vreg_ofs;
        uint32_t vreg_upper;
        uint32_t vreg_lower;
        int base_tmp;
        int ofs_tmp;
        int vs;
        int lane;
        int base;
        int ofs;

        CONSUME_TMPVAR(base_tmp);
        CONSUME_TMPVAR(ofs_tmp);
        CONSUME_IMM8(vs);

        vreg_ofs = (uint32_t)offsetof(struct rt_env, vreg);
        vreg_upper = (vreg_ofs + 0x800) >> 12;
        vreg_lower = vreg_ofs & 0xfff;
        if (!jit_put_word(ctx, (vreg_upper << 12) |
                          (REG_T4 << 7) | 0x37))
                return false;
        ADDI(REG_T4, REG_T4, vreg_lower);
        ADD(REG_T4, REG_S10, REG_T4);

        base = base_tmp * (int)sizeof(struct rt_value) + 8;
        ofs = ofs_tmp * (int)sizeof(struct rt_value) + 8;
        LW(REG_T0, IMM12(base), REG_S11);
        LW(REG_T1, IMM12(ofs), REG_S11);
        SLLI(REG_T1, REG_T1, 2);
        ADD(REG_T0, REG_T0, REG_T1);
        for (lane = 0; lane < 4; lane++) {
                LW(REG_T2, vs * 16 + lane * 4, REG_T4);
                SW(REG_T2, lane * 4, REG_T0);
        }
        return true;
}

/* Visit an OP_VSPLATF32 instruction. */
static INLINE bool
jit_visit_vsplatf32_op(
        struct rt_jit_context *ctx)
{
        uint32_t vreg_ofs;
        uint32_t vreg_upper;
        uint32_t vreg_lower;
        int vd;
        int src_tmp;
        int lane;
        int src;

        CONSUME_IMM8(vd);
        CONSUME_TMPVAR(src_tmp);

        vreg_ofs = (uint32_t)offsetof(struct rt_env, vreg);
        vreg_upper = (vreg_ofs + 0x800) >> 12;
        vreg_lower = vreg_ofs & 0xfff;
        if (!jit_put_word(ctx, (vreg_upper << 12) |
                          (REG_T4 << 7) | 0x37))
                return false;
        ADDI(REG_T4, REG_T4, vreg_lower);
        ADD(REG_T4, REG_S10, REG_T4);

        src = src_tmp * (int)sizeof(struct rt_value) + 8;
        LW(REG_T2, IMM12(src), REG_S11);
        for (lane = 0; lane < 4; lane++)
                SW(REG_T2, vd * 16 + lane * 4, REG_T4);
        return true;
}

/* Visit an OP_VGETLANEF32 instruction. */
static INLINE bool
jit_visit_vgetlanef32_op(
        struct rt_jit_context *ctx)
{
        uint32_t vreg_ofs;
        uint32_t vreg_upper;
        uint32_t vreg_lower;
        int dst_tmp;
        int vs;
        int lane_index;
        int d;
        uint32_t tag;

        CONSUME_TMPVAR(dst_tmp);
        CONSUME_IMM8(vs);
        CONSUME_IMM8(lane_index);

        vreg_ofs = (uint32_t)offsetof(struct rt_env, vreg);
        vreg_upper = (vreg_ofs + 0x800) >> 12;
        vreg_lower = vreg_ofs & 0xfff;
        if (!jit_put_word(ctx, (vreg_upper << 12) |
                          (REG_T4 << 7) | 0x37))
                return false;
        ADDI(REG_T4, REG_T4, vreg_lower);
        ADD(REG_T4, REG_S10, REG_T4);

        d = dst_tmp * (int)sizeof(struct rt_value);
        tag = (uint32_t)(NOCT_VALUE_FLOAT);
        LW(REG_T2, vs * 16 + lane_index * 4, REG_T4);
        ORI(REG_T3, REG_ZERO, tag);
        SW(REG_T3, IMM12(d), REG_S11);
        SW(REG_T2, IMM12(d + 8), REG_S11);
        return true;
}

/* Visit an OP_VADDF32X4 instruction. */
static INLINE bool
jit_visit_vaddf32x4_op(
        struct rt_jit_context *ctx)
{
        uint32_t vreg_ofs;
        uint32_t vreg_upper;
        uint32_t vreg_lower;
        int vd;
        int lhs;
        int rhs;
        int lane;

        CONSUME_IMM8(vd);
        CONSUME_IMM8(lhs);
        CONSUME_IMM8(rhs);

        vreg_ofs = (uint32_t)offsetof(struct rt_env, vreg);
        vreg_upper = (vreg_ofs + 0x800) >> 12;
        vreg_lower = vreg_ofs & 0xfff;
        if (!jit_put_word(ctx, (vreg_upper << 12) |
                          (REG_T4 << 7) | 0x37))
                return false;
        ADDI(REG_T4, REG_T4, vreg_lower);
        ADD(REG_T4, REG_S10, REG_T4);

        for (lane = 0; lane < 4; lane++) {
                uint32_t a;
                uint32_t b;
                uint32_t d;
                uint32_t word;

                a = (uint32_t)(lhs * 16 + lane * 4);
                b = (uint32_t)(rhs * 16 + lane * 4);
                d = (uint32_t)(vd * 16 + lane * 4);
                if (!jit_put_word(ctx, (a << 20) | (REG_T4 << 15) |
                                  (2 << 12) | 0x07))
                        return false;
                if (!jit_put_word(ctx, (b << 20) | (REG_T4 << 15) |
                                  (2 << 12) | (1 << 7) | 0x07))
                        return false;
                word = 0x00107153;
                if (!jit_put_word(ctx, word))
                        return false;
                if (!jit_put_fsw(ctx, 2, d, REG_T4))
                        return false;
        }
        return true;
}

/* Visit an OP_VSUBF32X4 instruction. */
static INLINE bool
jit_visit_vsubf32x4_op(
        struct rt_jit_context *ctx)
{
        uint32_t vreg_ofs;
        uint32_t vreg_upper;
        uint32_t vreg_lower;
        int vd;
        int lhs;
        int rhs;
        int lane;

        CONSUME_IMM8(vd);
        CONSUME_IMM8(lhs);
        CONSUME_IMM8(rhs);

        vreg_ofs = (uint32_t)offsetof(struct rt_env, vreg);
        vreg_upper = (vreg_ofs + 0x800) >> 12;
        vreg_lower = vreg_ofs & 0xfff;
        if (!jit_put_word(ctx, (vreg_upper << 12) |
                          (REG_T4 << 7) | 0x37))
                return false;
        ADDI(REG_T4, REG_T4, vreg_lower);
        ADD(REG_T4, REG_S10, REG_T4);

        for (lane = 0; lane < 4; lane++) {
                uint32_t a;
                uint32_t b;
                uint32_t d;
                uint32_t word;

                a = (uint32_t)(lhs * 16 + lane * 4);
                b = (uint32_t)(rhs * 16 + lane * 4);
                d = (uint32_t)(vd * 16 + lane * 4);
                if (!jit_put_word(ctx, (a << 20) | (REG_T4 << 15) |
                                  (2 << 12) | 0x07))
                        return false;
                if (!jit_put_word(ctx, (b << 20) | (REG_T4 << 15) |
                                  (2 << 12) | (1 << 7) | 0x07))
                        return false;
                word = 0x08107153;
                if (!jit_put_word(ctx, word))
                        return false;
                if (!jit_put_fsw(ctx, 2, d, REG_T4))
                        return false;
        }
        return true;
}

/* Visit an OP_VMULF32X4 instruction. */
static INLINE bool
jit_visit_vmulf32x4_op(
        struct rt_jit_context *ctx)
{
        uint32_t vreg_ofs;
        uint32_t vreg_upper;
        uint32_t vreg_lower;
        int vd;
        int lhs;
        int rhs;
        int lane;

        CONSUME_IMM8(vd);
        CONSUME_IMM8(lhs);
        CONSUME_IMM8(rhs);

        vreg_ofs = (uint32_t)offsetof(struct rt_env, vreg);
        vreg_upper = (vreg_ofs + 0x800) >> 12;
        vreg_lower = vreg_ofs & 0xfff;
        if (!jit_put_word(ctx, (vreg_upper << 12) |
                          (REG_T4 << 7) | 0x37))
                return false;
        ADDI(REG_T4, REG_T4, vreg_lower);
        ADD(REG_T4, REG_S10, REG_T4);

        for (lane = 0; lane < 4; lane++) {
                uint32_t a;
                uint32_t b;
                uint32_t d;
                uint32_t word;

                a = (uint32_t)(lhs * 16 + lane * 4);
                b = (uint32_t)(rhs * 16 + lane * 4);
                d = (uint32_t)(vd * 16 + lane * 4);
                if (!jit_put_word(ctx, (a << 20) | (REG_T4 << 15) |
                                  (2 << 12) | 0x07))
                        return false;
                if (!jit_put_word(ctx, (b << 20) | (REG_T4 << 15) |
                                  (2 << 12) | (1 << 7) | 0x07))
                        return false;
                word = 0x10107153;
                if (!jit_put_word(ctx, word))
                        return false;
                if (!jit_put_fsw(ctx, 2, d, REG_T4))
                        return false;
        }
        return true;
}

/* Visit an OP_VDIVF32X4 instruction. */
static INLINE bool
jit_visit_vdivf32x4_op(
        struct rt_jit_context *ctx)
{
        uint32_t vreg_ofs;
        uint32_t vreg_upper;
        uint32_t vreg_lower;
        int vd;
        int lhs;
        int rhs;
        int lane;

        CONSUME_IMM8(vd);
        CONSUME_IMM8(lhs);
        CONSUME_IMM8(rhs);

        vreg_ofs = (uint32_t)offsetof(struct rt_env, vreg);
        vreg_upper = (vreg_ofs + 0x800) >> 12;
        vreg_lower = vreg_ofs & 0xfff;
        if (!jit_put_word(ctx, (vreg_upper << 12) |
                          (REG_T4 << 7) | 0x37))
                return false;
        ADDI(REG_T4, REG_T4, vreg_lower);
        ADD(REG_T4, REG_S10, REG_T4);

        for (lane = 0; lane < 4; lane++) {
                uint32_t a;
                uint32_t b;
                uint32_t d;
                uint32_t word;

                a = (uint32_t)(lhs * 16 + lane * 4);
                b = (uint32_t)(rhs * 16 + lane * 4);
                d = (uint32_t)(vd * 16 + lane * 4);
                if (!jit_put_word(ctx, (a << 20) | (REG_T4 << 15) |
                                  (2 << 12) | 0x07))
                        return false;
                if (!jit_put_word(ctx, (b << 20) | (REG_T4 << 15) |
                                  (2 << 12) | (1 << 7) | 0x07))
                        return false;
                word = 0x18107153;
                if (!jit_put_word(ctx, word))
                        return false;
                if (!jit_put_fsw(ctx, 2, d, REG_T4))
                        return false;
        }
        return true;
}

/* Visit an OP_VCVTI32F32X4 instruction. */
static INLINE bool
jit_visit_vcvti32f32x4_op(
        struct rt_jit_context *ctx)
{
        uint32_t vreg_ofs;
        uint32_t vreg_upper;
        uint32_t vreg_lower;
        int vd;
        int vs;
        int dst;
        int src1;
        int src2;

        CONSUME_IMM8(vd);
        CONSUME_IMM8(vs);

        dst = vd;
        src1 = vs;
        src2 = 0;

        vreg_ofs = (uint32_t)offsetof(struct rt_env, vreg);
        vreg_upper = (vreg_ofs + 0x800) >> 12;
        vreg_lower = vreg_ofs & 0xfff;
        if (!jit_put_word(ctx, (vreg_upper << 12) |
                          (REG_T4 << 7) | 0x37))
                return false;
        ADDI(REG_T4, REG_T4, vreg_lower);
        ADD(REG_T4, REG_S10, REG_T4);

        ASM_BINARY_OP(noct_ex_vcvti32f32x4_helper);
        return true;
}

/* Visit an OP_VCVTF32I32X4 instruction. */
static INLINE bool
jit_visit_vcvtf32i32x4_op(
        struct rt_jit_context *ctx)
{
        uint32_t vreg_ofs;
        uint32_t vreg_upper;
        uint32_t vreg_lower;
        int vd;
        int vs;
        int dst;
        int src1;
        int src2;

        CONSUME_IMM8(vd);
        CONSUME_IMM8(vs);

        dst = vd;
        src1 = vs;
        src2 = 0;

        vreg_ofs = (uint32_t)offsetof(struct rt_env, vreg);
        vreg_upper = (vreg_ofs + 0x800) >> 12;
        vreg_lower = vreg_ofs & 0xfff;
        if (!jit_put_word(ctx, (vreg_upper << 12) |
                          (REG_T4 << 7) | 0x37))
                return false;
        ADDI(REG_T4, REG_T4, vreg_lower);
        ADD(REG_T4, REG_S10, REG_T4);

        ASM_BINARY_OP(noct_ex_vcvtf32i32x4_helper);
        return true;
}

/* Visit an OP_VMINS32X4 instruction. */
static INLINE bool
jit_visit_vmins32x4_op(
        struct rt_jit_context *ctx)
{
        uint32_t vreg_ofs;
        uint32_t vreg_upper;
        uint32_t vreg_lower;
        int vd;
        int lhs;
        int rhs;

        CONSUME_IMM8(vd);
        CONSUME_IMM8(lhs);
        CONSUME_IMM8(rhs);

        vreg_ofs = (uint32_t)offsetof(struct rt_env, vreg);
        vreg_upper = (vreg_ofs + 0x800) >> 12;
        vreg_lower = vreg_ofs & 0xfff;
        if (!jit_put_word(ctx, (vreg_upper << 12) |
                          (REG_T4 << 7) | 0x37))
                return false;
        ADDI(REG_T4, REG_T4, vreg_lower);
        ADD(REG_T4, REG_S10, REG_T4);

        assert(NEVER_COME_HERE);

        return false;
}

/* Visit an OP_VMAXS32X4 instruction. */
static INLINE bool
jit_visit_vmaxs32x4_op(
        struct rt_jit_context *ctx)
{
        uint32_t vreg_ofs;
        uint32_t vreg_upper;
        uint32_t vreg_lower;
        int vd;
        int lhs;
        int rhs;

        CONSUME_IMM8(vd);
        CONSUME_IMM8(lhs);
        CONSUME_IMM8(rhs);

        vreg_ofs = (uint32_t)offsetof(struct rt_env, vreg);
        vreg_upper = (vreg_ofs + 0x800) >> 12;
        vreg_lower = vreg_ofs & 0xfff;
        if (!jit_put_word(ctx, (vreg_upper << 12) |
                          (REG_T4 << 7) | 0x37))
                return false;
        ADDI(REG_T4, REG_T4, vreg_lower);
        ADD(REG_T4, REG_S10, REG_T4);

        assert(NEVER_COME_HERE);

        return false;
}

/* Visit a bytecode of a function. */
static bool
jit_visit_bytecode(
	struct rt_jit_context *ctx)
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
		case OP_LICONST:
			if (!jit_visit_liconst_op(ctx))
				return false;
			break;
		case OP_FCONST:
			if (!jit_visit_fconst_op(ctx))
				return false;
			break;
		case OP_LFCONST:
			if (!jit_visit_lfconst_op(ctx))
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
                case OP_SAFEPOINT:
#if defined(NOCT_USE_MULTITHREAD)
                        if (!jit_visit_safepoint_op(ctx))
                                return false;
#endif
                        break;
                case OP_PBASE:
                        if (!jit_visit_pbase_op(ctx))
                                return false;
                        break;
                case OP_PLEN:
                        if (!jit_visit_plen_op(ctx))
                                return false;
                        break;
                case OP_PCHECK:
                        if (!jit_visit_pcheck_op(ctx))
                                return false;
                        break;
                case OP_TYPEIS:
                        if (!jit_visit_typeis_op(ctx))
                                return false;
                        break;
                case OP_PLOAD8U:
                        if (!jit_visit_pload8u_op(ctx))
                                return false;
                        break;
                case OP_PSTORE8:
                        if (!jit_visit_pstore8_op(ctx))
                                return false;
                        break;
                case OP_CHECKTYPE:
                        if (!jit_visit_checktype_op(ctx))
                                return false;
                        break;
                case OP_PLOAD8S:
                        if (!jit_visit_pload8s_op(ctx))
                                return false;
                        break;
                case OP_PLOAD16U:
                        if (!jit_visit_pload16u_op(ctx))
                                return false;
                        break;
                case OP_PLOAD16S:
                        if (!jit_visit_pload16s_op(ctx))
                                return false;
                        break;
                case OP_PLOAD32:
                        if (!jit_visit_pload32_op(ctx))
                                return false;
                        break;
                case OP_PLOAD64:
                        if (!jit_visit_pload64_op(ctx))
                                return false;
                        break;
                case OP_PSTORE16:
                        if (!jit_visit_pstore16_op(ctx))
                                return false;
                        break;
                case OP_PSTORE32:
                        if (!jit_visit_pstore32_op(ctx))
                                return false;
                        break;
                case OP_PSTORE64:
                        if (!jit_visit_pstore64_op(ctx))
                                return false;
                        break;
                case OP_PLOADF32:
                        if (!jit_visit_ploadf32_op(ctx))
                                return false;
                        break;
                case OP_PSTOREF32:
                        if (!jit_visit_pstoref32_op(ctx))
                                return false;
                        break;
                case OP_VINDEX_HINT:
                        if (!jit_visit_vindex_hint_op(ctx))
                                return false;
                        break;
                case OP_PLOOP_HINT:
                        if (!rt_jit_visit_ploop_hint_op(ctx))
                                return false;
                        break;
                case OP_TMPVAR_TYPE:
                        if (!rt_jit_visit_tmpvar_type_op(ctx))
                                return false;
                        break;
                case OP_MATERIALIZE_TYPE:
                        if (!rt_jit_visit_materialize_type_metadata_op(ctx))
                                return false;
                        break;
                case OP_SUBJNZ:
                        if (!jit_visit_subjnz_op(ctx))
                                return false;
                        break;
                case OP_VORI32X4I:
                        if (!jit_visit_vori32x4i_op(ctx))
                                return false;
                        break;
                case OP_VFMAF32X4:
                        if (!jit_visit_vfmaf32x4_op(ctx))
                                return false;
                        break;
                case OP_IADD:
                        if (!jit_visit_iadd_op(ctx))
                                return false;
                        break;
                case OP_ISUB:
                        if (!jit_visit_isub_op(ctx))
                                return false;
                        break;
                case OP_IMUL:
                        if (!jit_visit_imul_op(ctx))
                                return false;
                        break;
                case OP_IDIV:
                        if (!jit_visit_idiv_op(ctx))
                                return false;
                        break;
                case OP_IMOD:
                        if (!jit_visit_imod_op(ctx))
                                return false;
                        break;
                case OP_IAND:
                        if (!jit_visit_iand_op(ctx))
                                return false;
                        break;
                case OP_IOR:
                        if (!jit_visit_ior_op(ctx))
                                return false;
                        break;
                case OP_IXOR:
                        if (!jit_visit_ixor_op(ctx))
                                return false;
                        break;
                case OP_ISHL:
                        if (!jit_visit_ishl_op(ctx))
                                return false;
                        break;
                case OP_ISHR:
                        if (!jit_visit_ishr_op(ctx))
                                return false;
                        break;
                case OP_ILT:
                        if (!jit_visit_ilt_op(ctx))
                                return false;
                        break;
                case OP_ILTE:
                        if (!jit_visit_ilte_op(ctx))
                                return false;
                        break;
                case OP_IGT:
                        if (!jit_visit_igt_op(ctx))
                                return false;
                        break;
                case OP_IGTE:
                        if (!jit_visit_igte_op(ctx))
                                return false;
                        break;
                case OP_FADD:
                        if (!jit_visit_fadd_op(ctx))
                                return false;
                        break;
                case OP_FSUB:
                        if (!jit_visit_fsub_op(ctx))
                                return false;
                        break;
                case OP_FMUL:
                        if (!jit_visit_fmul_op(ctx))
                                return false;
                        break;
                case OP_FDIV:
                        if (!jit_visit_fdiv_op(ctx))
                                return false;
                        break;
                case OP_FLT:
                        if (!jit_visit_flt_op(ctx))
                                return false;
                        break;
                case OP_FLTE:
                        if (!jit_visit_flte_op(ctx))
                                return false;
                        break;
                case OP_FGT:
                        if (!jit_visit_fgt_op(ctx))
                                return false;
                        break;
                case OP_FGTE:
                        if (!jit_visit_fgte_op(ctx))
                                return false;
                        break;
                case OP_IDIV_CHECKED:
                        if (!jit_visit_idiv_checked_op(ctx))
                                return false;
                        break;
                case OP_IMOD_CHECKED:
                        if (!jit_visit_imod_checked_op(ctx))
                                return false;
                        break;
                case OP_VLOADI32X4:
                        if (!jit_visit_vloadi32x4_op(ctx))
                                return false;
                        break;
                case OP_VSTOREI32X4:
                        if (!jit_visit_vstorei32x4_op(ctx))
                                return false;
                        break;
                case OP_VSPLATI32:
                        if (!jit_visit_vsplati32_op(ctx))
                                return false;
                        break;
                case OP_VGETLANEI32:
                        if (!jit_visit_vgetlanei32_op(ctx))
                                return false;
                        break;
                case OP_VMOV128:
                        if (!jit_visit_vmov128_op(ctx))
                                return false;
                        break;
                case OP_VADDI32X4:
                        if (!jit_visit_vaddi32x4_op(ctx))
                                return false;
                        break;
                case OP_VSUBI32X4:
                        if (!jit_visit_vsubi32x4_op(ctx))
                                return false;
                        break;
                case OP_VMULI32X4:
                        if (!jit_visit_vmuli32x4_op(ctx))
                                return false;
                        break;
                case OP_VAND128:
                        if (!jit_visit_vand128_op(ctx))
                                return false;
                        break;
                case OP_VOR128:
                        if (!jit_visit_vor128_op(ctx))
                                return false;
                        break;
                case OP_VXOR128:
                        if (!jit_visit_vxor128_op(ctx))
                                return false;
                        break;
                case OP_VSHLI32X4:
                        if (!jit_visit_vshli32x4_op(ctx))
                                return false;
                        break;
                case OP_VSHRI32X4:
                        if (!jit_visit_vshri32x4_op(ctx))
                                return false;
                        break;
                case OP_VLOADF32X4:
                        if (!jit_visit_vloadf32x4_op(ctx))
                                return false;
                        break;
                case OP_VSTOREF32X4:
                        if (!jit_visit_vstoref32x4_op(ctx))
                                return false;
                        break;
                case OP_VSPLATF32:
                        if (!jit_visit_vsplatf32_op(ctx))
                                return false;
                        break;
                case OP_VGETLANEF32:
                        if (!jit_visit_vgetlanef32_op(ctx))
                                return false;
                        break;
                case OP_VADDF32X4:
                        if (!jit_visit_vaddf32x4_op(ctx))
                                return false;
                        break;
                case OP_VSUBF32X4:
                        if (!jit_visit_vsubf32x4_op(ctx))
                                return false;
                        break;
                case OP_VMULF32X4:
                        if (!jit_visit_vmulf32x4_op(ctx))
                                return false;
                        break;
                case OP_VDIVF32X4:
                        if (!jit_visit_vdivf32x4_op(ctx))
                                return false;
                        break;
                case OP_VCVTI32F32X4:
                        if (!jit_visit_vcvti32f32x4_op(ctx))
                                return false;
                        break;
                case OP_VCVTF32I32X4:
                        if (!jit_visit_vcvtf32i32x4_op(ctx))
                                return false;
                        break;
                case OP_VMINS32X4:
                        if (!jit_visit_vmins32x4_op(ctx))
                                return false;
                        break;
                case OP_VMAXS32X4:
                        if (!jit_visit_vmaxs32x4_op(ctx))
                                return false;
                        break;
		default:
			return false; /* interpreter fallback for newer bytecode */
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
    struct rt_jit_context *ctx,
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
		rt_error(ctx->env, N_TR("Branch target not found."));
		return false;
	}

	/* Calc a branch offset. */
	offset = (int)((intptr_t)target_code - (intptr_t)ctx->branch_patch[patch_index].code);

	/* Set the assembler cursor. */
	ctx->code = ctx->branch_patch[patch_index].code;
	/*
	 * Assemble.  Conditional sites reserve three slots so a far
	 * target can use inverted-branch + AUIPC/JALR (signed 32-bit
	 * PC-relative reach).  Unconditional sites reserve two.
	 */
	if (ctx->branch_patch[patch_index].type == PATCH_JAL) {
		if (getenv("NOCT_JIT_FORCE_LONG_BRANCH") == NULL &&
		    offset >= -1048576 && offset <= 1048574) {
			ASM { JAL(REG_ZERO, IMM21(offset)); }
			if (!jit_put_word(ctx, 0x00000013)) return false;
		} else if (!jit_put_far_jump(ctx, offset)) {
			return false;
		}
	} else if (ctx->branch_patch[patch_index].type == PATCH_BEQ) {
		if (getenv("NOCT_JIT_FORCE_LONG_BRANCH") == NULL &&
		    offset >= -4096 && offset <= 4094) {
			ASM {
				BEQ	(REG_T0, REG_T1, IMM13(offset));
			}
			if (!jit_put_word(ctx, 0x00000013)) return false;
			if (!jit_put_word(ctx, 0x00000013)) return false;
		} else {
			ASM { BNE(REG_T0, REG_T1, IMM13(12)); }
			if (!jit_put_far_jump(ctx, offset - 4)) return false;
		}
	} else if (ctx->branch_patch[patch_index].type == PATCH_BNE) {
		if (getenv("NOCT_JIT_FORCE_LONG_BRANCH") == NULL &&
		    offset >= -4096 && offset <= 4094) {
			ASM {
				BNE	(REG_T0, REG_T1, IMM13(offset));
			}
			if (!jit_put_word(ctx, 0x00000013)) return false;
			if (!jit_put_word(ctx, 0x00000013)) return false;
		} else {
			ASM { BEQ(REG_T0, REG_T1, IMM13(12)); }
			if (!jit_put_far_jump(ctx, offset - 4)) return false;
		}
	}

	return true;
}

#endif /* defined(NOCT_ARCH_RISCV32) && defined(NOCT_USE_JIT) */
