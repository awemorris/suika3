/* -*- coding: utf-8; tab-width: 8; indent-tabs-mode: nil; -*- */

/*
 * Noct Programming Language
 * Copyright (c) 2025, 2026, Awe Morris
 */

/*
 * JIT (arm64): Just-In-Time native code generation
 */

#include <noct/noct.h>

#if defined(NOCT_ARCH_ARM64) && defined(NOCT_USE_JIT)

#include "runtime.h"
#include "jit.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

/* False asseretion */
#define JIT_OP_NOT_IMPLEMENTED          0
#define NEVER_COME_HERE                 0

/* Branch patch type */
#define PATCH_BAL                       0
#define PATCH_BEQ                       1
#define PATCH_BNE                       2

/* Forward declaration */
static bool jit_visit_bytecode(struct rt_jit_context *ctx);
static bool jit_patch_branch(struct rt_jit_context *ctx, int patch_index);
static bool jit_put_ldr_imm(struct rt_jit_context *ctx, uint32_t rd,
	uint32_t rs, uint32_t imm);
static bool jit_put_str_imm(struct rt_jit_context *ctx, uint32_t rs,
	uint32_t rd, uint32_t imm);
static bool jit_put_add(struct rt_jit_context *ctx, uint32_t rd,
	uint32_t ra, uint32_t rb);
static bool jit_put_add_imm(struct rt_jit_context *ctx, uint32_t rd,
	uint32_t rs, uint32_t imm);
#if 0
static bool jit_put_lsl4(struct rt_jit_context *ctx, uint32_t rd,
	uint32_t rs);
#endif
static bool jit_put_cmp_imm(struct rt_jit_context *ctx, uint32_t rs,
	uint32_t imm);
static bool jit_put_cmp_w3_imm(struct rt_jit_context *ctx, uint32_t imm);
static bool jit_put_cmp_w3_w4(struct rt_jit_context *ctx);
#if defined(NOCT_USE_OPTIMIZER)
static uint16_t jit_arm64_read_u16(const uint8_t *p);
static bool jit_arm64_packed_cursor(struct rt_jit_context *ctx, int base,
	int ofs, int scale, uint32_t *base_reg, int *cursor,
	int32_t *byte_disp);
static bool jit_arm64_put_packed_access_disp(struct rt_jit_context *ctx,
	bool store, int scale, bool is_signed, uint32_t rt, uint32_t rn,
	int32_t byte_disp);
static void jit_arm64_gpr_reset(struct rt_jit_context *ctx);
static int jit_arm64_gpr_limit(void);
static bool jit_arm64_gpr_spill(struct rt_jit_context *ctx, int slot);
static bool jit_arm64_gpr_alloc(struct rt_jit_context *ctx, int tmp,
	unsigned pin_mask, bool load, uint32_t *reg);
static bool jit_arm64_gpr_rebind(struct rt_jit_context *ctx, int dst,
	int src, uint32_t *reg);
static bool jit_arm64_gpr_flush(struct rt_jit_context *ctx);
static bool jit_arm64_gpr_flush_required(struct rt_jit_context *ctx);
static bool jit_arm64_scan_vector_bases(struct rt_jit_context *ctx);
#endif

/*
 * Generate a JIT-compiled code for a function.
 */
bool
jit_build(
          struct rt_env *env,
          struct rt_func *func)
{
	/* Advanced SIMD is part of the AArch64 application-profile ABI. */
	return rt_jit_build_standard(env, func,
                                  JIT_SIMD_CAP_NEON | JIT_SIMD_CAP_FMAF32X4,
                                  "arm64",
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

/* Decoration */
#define ASM

/* Registers */
#define REG_X0                  0
#define REG_X1                  1
#define REG_X2                  2
#define REG_X3                  3
#define REG_X4                  4
#define REG_X5                  5
#define REG_X6                  6
#define REG_X7                  7
#define REG_X8                  8
#define REG_X9                  9
#define REG_X10                 10
#define REG_X11                 11
#define REG_X12                 12
#define REG_X13                 13
#define REG_X14                 14
#define REG_X15                 15
#define REG_X16                 16
#define REG_X17                 17
#define REG_X18                 18
#define REG_X19                 19
#define REG_X20                 20
#define REG_X21                 21
#define REG_X22                 22
#define REG_X23                 23
#define REG_X24                 24
#define REG_X25                 25
#define REG_X26                 26
#define REG_X27                 27
#define REG_X28                 28
#define REG_X29                 29
#define REG_X30                 30
#define REG_XZR                 31
#define REG_SP                  31

/* Immediate */
#define IMM8(v)                 (uint32_t)(v)
#define IMM9(v)                 (uint32_t)(v)
#define IMM12(v)                (uint32_t)(v)
#define IMM16(v)                (uint32_t)(v)
#define IMM19(v)                (uint32_t)(v)

/* Shift */
#define LSL_0                   0
#define LSL_16                  16
#define LSL_32                  32
#define LSL_48                  48

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

/* movz */
#define MOVZ(rd, imm, lsl)              if (!jit_put_movz(ctx, rd, imm, lsl)) return false
static INLINE bool
jit_put_movz(
        struct rt_jit_context *ctx,
        uint32_t n,
        uint32_t imm,
        uint32_t lsl)
{
        if (!jit_put_word(ctx,
                          0xd2800000 |          /* movz */
                          ((lsl / 16) << 21) |  /* shift */
                          (imm << 5) |          /* imm16 */
                          n))                   /* reg */
                return false;
        return true;
}

/* movk */
#define MOVK(rd, imm, lsl)              if (!jit_put_movk(ctx, rd, imm, lsl)) return false
static INLINE bool
jit_put_movk(
        struct rt_jit_context *ctx,
        uint32_t n,
        uint32_t imm,
        uint32_t lsl)
{
        if (!jit_put_word(ctx,
                          0xf2800000 |          /* movk */
                          ((lsl / 16) << 21) |  /* shift */
                          (imm << 5) |          /* imm16 */
                          n))                   /* reg */
                return false;
        return true;
}

/* ldr imm */
#define LDR(rd, rs)                     if (!jit_put_ldr_imm(ctx, rd, rs, 0)) return false
#define LDR_IMM(rd, rs, imm)            if (!jit_put_ldr_imm(ctx, rd, rs, imm)) return false
static bool
jit_put_ldr_imm(
        struct rt_jit_context *ctx,
        uint32_t rd,
        uint32_t rs,
        uint32_t imm)
{
        if (!jit_put_word(ctx,
                          0xf9400000 |                  /* ldr */
                          (rs << 5) |                   /* rd */
                          (rd) |                        /* rs */
                          (((imm / 8) & 0x1ff) << 10))) /* imm */
                return false;
        return true;
}

/* str imm */
#define STR(rs, rd)                     if (!jit_put_str_imm(ctx, rs, rd, 0)) return false
#define STR_IMM(rs, rd, imm)            if (!jit_put_str_imm(ctx, rs, rd, imm)) return false
static bool
jit_put_str_imm(
        struct rt_jit_context *ctx,
        uint32_t rs,
        uint32_t rd,
        uint32_t imm)
{
        if (!jit_put_word(ctx,
                          0xf9000000 |                  /* str */
                          (rs)        |                 /* rs */
                          (rd << 5) |                   /* rd */
                          (((imm / 8) & 0x1ff) << 10))) /* imm */
                return false;
        return true;
}

/* ABCE: sized loads/stores and shifts (register addressing, imm=0). */
#define LDR_W_IMM(rd, rs, imm)          if (!jit_put_ldr_w_imm(ctx, rd, rs, imm)) return false
static INLINE bool
jit_put_ldr_w_imm(
        struct rt_jit_context *ctx,
        uint32_t rd,
        uint32_t rs,
        uint32_t imm)
{
        /* ldr wRd, [xRs, #imm] (imm scaled by 4) */
        if (!jit_put_word(ctx, 0xb9400000 | (((imm / 4) & 0xfff) << 10) | (rs << 5) | rd))
                return false;
        return true;
}

#define STR_W_IMM(rd, rs, imm)          if (!jit_put_str_w_imm(ctx, rd, rs, imm)) return false
static INLINE bool
jit_put_str_w_imm(
        struct rt_jit_context *ctx,
        uint32_t rd,
        uint32_t rs,
        uint32_t imm)
{
        /* str wRd, [xRs, #imm] (imm scaled by 4) */
        if (!jit_put_word(ctx, 0xb9000000 | (((imm / 4) & 0xfff) << 10) |
                          (rs << 5) | rd))
                return false;
        return true;
}

#define LSL_IMM(rd, rs, sh)             if (!jit_put_lsl_imm(ctx, rd, rs, sh)) return false
static INLINE bool
jit_put_lsl_imm(
        struct rt_jit_context *ctx,
        uint32_t rd,
        uint32_t rs,
        uint32_t sh)
{
        /* lsl xRd, xRs, #sh == ubfm xRd, xRs, #((64-sh)&63), #(63-sh) */
        if (!jit_put_word(ctx, 0xd3400000 |
                          (((64 - sh) & 63) << 16) |
                          ((63 - sh) << 10) |
                          (rs << 5) | rd))
                return false;
        return true;
}

#define LDRB_R(rt, rn)                  if (!jit_put_word(ctx, 0x39400000 | (rn << 5) | (rt))) return false
#define LDRSB_R(rt, rn)                 if (!jit_put_word(ctx, 0x39c00000 | (rn << 5) | (rt))) return false
#define LDRH_R(rt, rn)                  if (!jit_put_word(ctx, 0x79400000 | (rn << 5) | (rt))) return false
#define LDRSH_R(rt, rn)                 if (!jit_put_word(ctx, 0x79c00000 | (rn << 5) | (rt))) return false
#define LDRW_R(rt, rn)                  if (!jit_put_word(ctx, 0xb9400000 | (rn << 5) | (rt))) return false
#define LDRX_R(rt, rn)                  if (!jit_put_word(ctx, 0xf9400000 | (rn << 5) | (rt))) return false
#define STRB_R(rt, rn)                  if (!jit_put_word(ctx, 0x39000000 | (rn << 5) | (rt))) return false
#define STRH_R(rt, rn)                  if (!jit_put_word(ctx, 0x79000000 | (rn << 5) | (rt))) return false
#define STRW_R(rt, rn)                  if (!jit_put_word(ctx, 0xb9000000 | (rn << 5) | (rt))) return false

/* ldp xN, xM, [sp], #16 */
#define LDP_POP(ra, rb)                 if (!jit_put_ldp_pop(ctx, ra, rb)) return false
static INLINE bool
jit_put_ldp_pop(
        struct rt_jit_context *ctx,
        uint32_t ra,
        uint32_t rb)
{
        if (!jit_put_word(ctx,
                          0xa8c103e0 |  /* ldp */
                          ra |          /* ra */
                          (rb << 10)))  /* rb */
                return false;
        return true;
}

/* stp xN, xM, [sp, #-16]! */
#define STP_PUSH(ra, rb)                if (!jit_put_stp_push(ctx, ra, rb)) return false
static INLINE bool
jit_put_stp_push(
        struct rt_jit_context *ctx,
        uint32_t ra,
        uint32_t rb)
{
        if (!jit_put_word(ctx,
                          0xa9bf03e0 |  /* stp */
                          ra |          /* ra */
                          (rb << 10)))  /* rb */
                return false;
        return true;
}

/* add */
#define ADD(rd, ra, rb)                 if (!jit_put_add(ctx, rd, ra, rb)) return false
static bool
jit_put_add(
        struct rt_jit_context *ctx,
        uint32_t rd,
        uint32_t ra,
        uint32_t rb)
{
        if (!jit_put_word(ctx,
                          0x8b000000 |  /* add */
                          rd |          /* rd */
                          (ra << 5) |   /* ra */
                          (rb << 16)))  /* rb */
                return false;
        return true;
}

/* add_imm */
#define ADD_IMM(rd, rs, imm)            if (!jit_put_add_imm(ctx, rd, rs, imm)) return false
static bool
jit_put_add_imm(
        struct rt_jit_context *ctx,
        uint32_t rd,
        uint32_t rs,
        uint32_t imm)
{
        if (!jit_put_word(ctx,
                          0x91000000 |                  /* add */
                          rd |                          /* rd */
                          (rs << 5) |                   /* rs */
                          ((imm & 0xfff) << 10)))       /* imm */
                return false;
        return true;
}

#if 0
/* lsl #4 */
#define LSL_4(rd, rs)                   if (!jit_put_lsl4(ctx, rd, rs)) return false
static bool
jit_put_lsl4(
        struct rt_jit_context *ctx,
        uint32_t rd,
        uint32_t rs)
{
        if (!jit_put_word(ctx,
                          0xd37cec00 |  /* madd */
                          rd |          /* rd */
                          (rs << 5)))   /* ra */
                return false;
        return true;
}
#endif

/* cmp_imm */
#define CMP_IMM(rs, imm)                if (!jit_put_cmp_imm(ctx, rs, imm)) return false
static bool
jit_put_cmp_imm(
        struct rt_jit_context *ctx,
        uint32_t rs,
        uint32_t imm)
{
        if (!jit_put_word(ctx,
                          0xf100001f |                  /* cmp */
                          (rs << 5) |                   /* rs */
                          ((imm & 0xfff) << 10)))       /* imm */
                return false;
        return true;
}

/* cmp_w3_imm */
#define CMP_W3_IMM(imm)                 if (!jit_put_cmp_w3_imm(ctx, imm)) return false
static bool
jit_put_cmp_w3_imm(
        struct rt_jit_context *ctx,
        uint32_t imm)
{
        if (!jit_put_word(ctx,
                          0x7100007f |                  /* cmp */
                          ((imm & 0xfff) << 10)))       /* imm */
                return false;
        return true;
}

/* cmp w3, w4 */
#define CMP_W3_W4()                     if (!jit_put_cmp_w3_w4(ctx)) return false
static bool
jit_put_cmp_w3_w4(
        struct rt_jit_context *ctx)
{
        if (!jit_put_word(ctx, 0x6b04007f))
                return false;
        return true;
}

/* B (unconditional, imm26: +/-128 MiB). */
#define B(rel)                          if (!jit_put_b(ctx, rel)) return false
static INLINE bool
jit_put_b(
        struct rt_jit_context *ctx,
        int32_t rel)
{
        if (!jit_put_word(ctx,
                          0x14000000 |
                          (((uint32_t)(rel / 4)) & 0x03ffffff)))
                return false;
        return true;
}

/* BAL (b.cond al; short conditional form). */
#define BAL(rel)                        if (!jit_put_bal(ctx, rel)) return false
static INLINE bool
jit_put_bal(
        struct rt_jit_context *ctx,
        uint32_t rel)    
{
        if (!jit_put_word(ctx,
                          0x54000000 |                                  /* b.cond */
                          (0xe) |                                       /* always */
                          ((((uint32_t)(rel / 4)) & 0x7ffff) << 5)))    /* rel */
                return false;
        return true;
}

/* BEQ */
#define BEQ(rel)                        if (!jit_put_beq(ctx, rel)) return false
static INLINE bool
jit_put_beq(
        struct rt_jit_context *ctx,
        uint32_t rel)    
{
        if (!jit_put_word(ctx,
                          0x54000000 |                                  /* b.cond */
                          (0x0) |                                       /* eq */
                          ((((uint32_t)(rel / 4)) & 0x7ffff) << 5)))    /* rel */
                return false;
        return true;
}

/* BNE */
#define BNE(rel)                        if (!jit_put_bne(ctx, rel)) return false
static INLINE bool
jit_put_bne(
        struct rt_jit_context *ctx,
        uint32_t rel)    
{
        if (!jit_put_word(ctx,
                          0x54000000 |                                  /* b.cond */
                          (0x1) |                                       /* ne */
                          ((((uint32_t)(rel / 4)) & 0x7ffff) << 5)))    /* rel */
                return false;
        return true;
}

/* Jump to the shared exception epilogue without relying on imm19 range. */
#define EXCEPTION_IF_EQUAL() do {                                      \
        if (!jit_put_bne(ctx, 8)) return false;                        \
        if (!jit_put_b(ctx, (int32_t)((intptr_t)ctx->exception_code - \
                                      (intptr_t)ctx->code)))           \
                return false;                                          \
} while (0)

/* BLR */
#define BLR(rd)                         if (!jit_put_blr(ctx, rd)) return false
static INLINE bool
jit_put_blr(
        struct rt_jit_context *ctx,
        uint32_t rd)
{
        if (!jit_put_word(ctx,
                          0xd63f0000 |  /* blr */
                          (rd << 5)))   /* rel */
                return false;
        return true;
}

/* ret */
#define RET()                           if (!jit_put_ret(ctx)) return false
static INLINE bool
jit_put_ret(
        struct rt_jit_context *ctx)
{
        if (!jit_put_word(ctx,
                          0xd65f03c0))  /* ret */
                return false;
        return true;
}

/*
 * Templates
 */

#define ASM_BINARY_OP(f)                                                                                \
        ASM {                                                                                           \
                /* x0 = env */                                                                          \
                /* x1 = &env->frame->tmpvar[0] */                                                        \
                                                                                                        \
                STP_PUSH        (REG_X0, REG_X1);                                                       \
                STP_PUSH        (REG_X30, REG_XZR);                                                     \
                                                                                                        \
                /* Arg1 x0: env */                                                                      \
                                                                                                        \
                /* Arg2 x1: dst */                                                                      \
                MOVZ            (REG_X1, IMM16(dst), LSL_0);                                            \
                                                                                                        \
                /* Arg3 x2: src1 */                                                                     \
                MOVZ            (REG_X2, IMM16(src1), LSL_0);                                           \
                                                                                                        \
                /* Arg4 x3: src2 */                                                                     \
                MOVZ            (REG_X3, IMM16(src2), LSL_0);                                           \
                                                                                                        \
                /* Call f(). */                                                                         \
                MOVZ            (REG_X4, IMM16(((uint64_t)(f)) & 0xffff), LSL_0);                       \
                MOVK            (REG_X4, IMM16((((uint64_t)(f)) >> 16) & 0xffff), LSL_16);              \
                MOVK            (REG_X4, IMM16((((uint64_t)(f)) >> 32) & 0xffff), LSL_32);              \
                MOVK            (REG_X4, IMM16((((uint64_t)(f)) >> 48) & 0xffff), LSL_48);              \
                BLR             (REG_X4);                                                               \
                                                                                                        \
                /* If failed: */                                                                        \
                CMP_IMM         (REG_X0, IMM12(0));                                                     \
                LDP_POP         (REG_X30, REG_X1);                                                      \
                LDP_POP         (REG_X0, REG_X1);                                                       \
                EXCEPTION_IF_EQUAL();                                                                  \
        }

#define ASM_UNARY_OP(f)                                                                                 \
        ASM {                                                                                           \
                /* x0 = env */                                                                          \
                /* x1 = &env->frame->tmpvar[0] */                                                        \
                                                                                                        \
                STP_PUSH        (REG_X0, REG_X1);                                                       \
                STP_PUSH        (REG_X30, REG_XZR);                                                     \
                                                                                                        \
                /* Arg1 x0: env */                                                                      \
                                                                                                        \
                /* Arg2 x1: dst */                                                                      \
                MOVZ            (REG_X1, IMM16(dst), LSL_0);                                            \
                                                                                                        \
                /* Arg3 x2: src */                                                                      \
                MOVZ            (REG_X2, IMM16(src), LSL_0);                                            \
                                                                                                        \
                /* Call f(). */                                                                         \
                MOVZ            (REG_X3, IMM16(((uint64_t)f) & 0xffff), LSL_0);                         \
                MOVK            (REG_X3, IMM16((((uint64_t)f) >> 16) & 0xffff), LSL_16);                \
                MOVK            (REG_X3, IMM16((((uint64_t)f) >> 32) & 0xffff), LSL_32);                \
                MOVK            (REG_X3, IMM16((((uint64_t)f) >> 48) & 0xffff), LSL_48);                \
                BLR             (REG_X3);                                                               \
                                                                                                        \
                /* If failed: */                                                                        \
                CMP_IMM         (REG_X0, IMM12(0));                                                     \
                LDP_POP         (REG_X30, REG_X1);                                                      \
                LDP_POP         (REG_X0, REG_X1);                                                       \
                EXCEPTION_IF_EQUAL();                                                                  \
        }

#if defined(NOCT_USE_OPTIMIZER)
static INLINE void
jit_arm64_invalidate_packed_load(struct rt_jit_context *ctx, int tmp)
{
	int i;

	for (i = 0; i < 3; i++) {
		if (ctx->gpr_load_tmp[i] == tmp)
			ctx->gpr_load_tmp[i] = -1;
	}
}

static INLINE bool
jit_arm64_gpr_get(struct rt_jit_context *ctx, int tmp,
			  unsigned pin_mask, uint32_t *reg)
{
	return jit_arm64_gpr_alloc(ctx, tmp, pin_mask, true, reg);
}

static INLINE bool
jit_arm64_gpr_dest(struct rt_jit_context *ctx, int tmp,
			   unsigned pin_mask, uint32_t *reg)
{
	ctx->gpr_remat_valid[tmp] = 0;
	return jit_arm64_gpr_alloc(ctx, tmp, pin_mask, false, reg);
}

static INLINE bool
jit_arm64_gpr_mov(struct rt_jit_context *ctx, uint32_t dst, uint32_t src)
{
	if (dst == src)
		return true;
	/* mov wD,wS == orr wD,wzr,wS */
	return jit_put_word(ctx, 0x2a0003e0u | (src << 16) | dst);
}
#endif

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
                /* x0 = env */
                /* x1 = &env->frame->tmpvar[0] */

                /* env->line = line; */
                MOVZ            (REG_X2, IMM16(line & 0xffff), LSL_0);
                STR_IMM         (REG_X2, REG_X0, IMM9(8));
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
#if defined(NOCT_USE_OPTIMIZER)
	if (rt_jit_ploop_current_elided(ctx, 5))
		return true;
	if (ctx->packed_loop_hint_active) {
		int root;
		int i;

		if (!rt_jit_ploop_set_base_alias(ctx, dst, src))
			return false;
		root = rt_jit_ploop_resolve_base(ctx, dst);
		for (i = 0; i < 3; i++) {
			if (root == ctx->packed_loop_base_tmp[i])
				return true;
		}
		if (rt_jit_ploop_is_index_alias(ctx, src)) {
			if (!rt_jit_ploop_add_index_alias(ctx, dst))
				return false;
			return true;
		}
		rt_jit_ploop_remove_index_alias(ctx, dst);
		if (ctx->gpr_cache_active) {
			uint32_t src_reg;
			uint32_t dst_reg;
			unsigned pin;

			if (ctx->gpr_reg_limit == 1) {
				if (!jit_arm64_gpr_flush_required(ctx))
					return false;
			} else {
				if (!jit_arm64_gpr_get(ctx, src, 0, &src_reg))
					return false;
				pin = 1u << (src_reg - REG_X23);
				if (!jit_arm64_gpr_dest(ctx, dst, pin, &dst_reg) ||
				    !jit_arm64_gpr_mov(ctx, dst_reg, src_reg))
					return false;
				ctx->gpr_tmp_dirty[dst] = 1;
				ctx->gpr_range_valid[dst] =
					ctx->gpr_range_valid[src];
				if (ctx->gpr_range_valid[src]) {
					ctx->gpr_range_min[dst] =
						ctx->gpr_range_min[src];
					ctx->gpr_range_max[dst] =
						ctx->gpr_range_max[src];
				}
				jit_arm64_invalidate_packed_load(ctx, dst);
				return true;
			}
		}
	}
	if (ctx->tmp_fixed_type != NULL &&
	    ctx->tmp_fixed_type[dst] >= 0 &&
	    rt_jit_tmp_has_fixed_primitive_type(ctx, src,
					 ctx->tmp_fixed_type[dst])) {
		int dst_ofs = dst * (int)sizeof(struct rt_value);
		int src_ofs = src * (int)sizeof(struct rt_value);

		ASM {
			MOVZ(REG_X3, IMM16(src_ofs), LSL_0);
			ADD(REG_X3, REG_X3, REG_X1);
			LDR_IMM(REG_X2, REG_X3, IMM9(8));
			MOVZ(REG_X3, IMM16(dst_ofs), LSL_0);
			ADD(REG_X3, REG_X3, REG_X1);
			STR_IMM(REG_X2, REG_X3, IMM9(8));
		}
		return true;
	}
	if (ctx->tmp_fixed_type != NULL && ctx->tmp_fixed_type[src] >= 0 &&
	    !ctx->tmp_frame_tag_known[src]) {
		int src_ofs = src * (int)sizeof(struct rt_value);

		ASM {
			MOVZ(REG_X2, IMM16(ctx->tmp_fixed_type[src]), LSL_0);
			MOVZ(REG_X3, IMM16(src_ofs), LSL_0);
			ADD(REG_X3, REG_X3, REG_X1);
			STR(REG_X2, REG_X3);
		}
		}
#endif

	dst *= (int)sizeof(struct rt_value);
        src *= (int)sizeof(struct rt_value);

        /* env->frame->tmpvar[dst] = env->frame->tmpvar[src]; */
        ASM {
                /* x0 = env */
                /* x1 = &env->frame->tmpvar[0] */

                /* x2 = dst_addr = &env->frame->tmpvar[dst] */
                MOVZ            (REG_X2, IMM16(dst), LSL_0);
                ADD             (REG_X2, REG_X2, REG_X1);

                /* x3 = src_addr = &env->frame->tmpvar[src] */
                MOVZ            (REG_X3, IMM16(src), LSL_0);
                ADD             (REG_X3, REG_X3, REG_X1);

                /* *dst_addr = *src_addr */
                LDR_IMM         (REG_X4, REG_X3, 0);
                LDR_IMM         (REG_X5, REG_X3, 8);
                STR_IMM         (REG_X4, REG_X2, 0);
                STR_IMM         (REG_X5, REG_X2, 8);
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
	bool write_tag;

	CONSUME_TMPVAR(dst);
	CONSUME_IMM32(val);
#if defined(NOCT_USE_OPTIMIZER)
	if (rt_jit_ploop_current_elided(ctx, 7))
		return true;
	if (ctx->packed_loop_hint_active) {
		rt_jit_ploop_remove_index_alias(ctx, dst);
		rt_jit_ploop_remove_base_alias(ctx, dst);
	}
	if (ctx->gpr_cache_active) {
		uint32_t reg;

		if (!jit_arm64_gpr_dest(ctx, dst, 0, &reg))
			return false;
		if (!jit_put_movz(ctx, reg, val & 0xffffu, LSL_0) ||
		    !jit_put_movk(ctx, reg, (val >> 16) & 0xffffu, LSL_16))
			return false;
		ctx->gpr_tmp_dirty[dst] = 1;
		ctx->gpr_range_valid[dst] = 1;
		ctx->gpr_range_min[dst] = (int32_t)val;
		ctx->gpr_range_max[dst] = (int32_t)val;
		jit_arm64_invalidate_packed_load(ctx, dst);
		return true;
	}
#endif

	write_tag = !rt_jit_tmp_has_fixed_primitive_type(ctx, dst,
						     NOCT_VALUE_INT);
        dst *= (int)sizeof(struct rt_value);

        /* Set an integer constant. */
        ASM {
                /* x0 = env */
                /* x1 = &env->frame->tmpvar[0] */

                /* x2 = &env->frame->tmpvar[dst] */
                MOVZ            (REG_X2, IMM16(dst), LSL_0);
                ADD             (REG_X2, REG_X2, REG_X1);

                /* env->frame->tmpvar[dst].type = RT_VALUE_INT */
                if (write_tag) {
                MOVZ            (REG_X3, IMM16(0), LSL_0);
                STR             (REG_X3, REG_X2); }

                /* env->frame->tmpvar[dst].val.i = val */
                MOVZ            (REG_X3, IMM16(val & 0xffff), LSL_0);
                MOVK            (REG_X3, IMM16((val >> 16) & 0xffff), LSL_16);
                STR_IMM         (REG_X3, REG_X2, IMM9(8));
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
	bool write_tag;

        CONSUME_TMPVAR(dst);
        CONSUME_IMM64(val);

	write_tag = !rt_jit_tmp_has_fixed_primitive_type(ctx, dst,
						     NOCT_VALUE_LONG);
        dst *= (int)sizeof(struct rt_value);

        /* Set an integer constant. */
        ASM {
                /* x0 = env */
                /* x1 = &env->frame->tmpvar[0] */

                /* x2 = &env->frame->tmpvar[dst] */
                MOVZ            (REG_X2, IMM16(dst), LSL_0);
                ADD             (REG_X2, REG_X2, REG_X1);

                /* env->frame->tmpvar[dst].type = NOCT_VALUE_LONG */
                if (write_tag) {
                MOVZ            (REG_X3, IMM16(5), LSL_0);
                STR             (REG_X3, REG_X2); }

                /* env->frame->tmpvar[dst].val.i = val */
                MOVZ            (REG_X3, IMM16(val & 0xffff), LSL_0);
                MOVK            (REG_X3, IMM16((val >> 16) & 0xffff), LSL_16);
                MOVK            (REG_X3, IMM16((val >> 32) & 0xffff), LSL_32);
                MOVK            (REG_X3, IMM16((val >> 48) & 0xffff), LSL_48);
                STR_IMM         (REG_X3, REG_X2, IMM9(8));
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
	bool write_tag;

        CONSUME_TMPVAR(dst);
        CONSUME_IMM32(val);

	write_tag = !rt_jit_tmp_has_fixed_primitive_type(ctx, dst,
						     NOCT_VALUE_FLOAT);
        dst *= (int)sizeof(struct rt_value);

        /* Set a floating-point constant. */
        ASM {
                /* x0 = env */
                /* x1 = &env->frame->tmpvar[0] */

                /* x2 = &env->frame->tmpvar[dst] */
                MOVZ            (REG_X2, IMM16(dst), LSL_0);
                ADD             (REG_X2, REG_X2, REG_X1);

                /* Assign env->frame->tmpvar[dst].type = RT_VALUE_FLOAT. */
                if (write_tag) {
                MOVZ            (REG_X3, IMM16(NOCT_VALUE_FLOAT), LSL_0);
                STR             (REG_X3, REG_X2); }

                /* Assign env->frame->tmpvar[dst].val.f = val. */
                MOVZ            (REG_X3, IMM16(val & 0xffff), LSL_0);
                MOVK            (REG_X3, IMM16((val >> 16) & 0xffff), LSL_16);
                STR_IMM         (REG_X3, REG_X2, IMM9(8));
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
	bool write_tag;

        CONSUME_TMPVAR(dst);
        CONSUME_IMM64(val);

	write_tag = !rt_jit_tmp_has_fixed_primitive_type(ctx, dst,
						     NOCT_VALUE_DOUBLE);
        dst *= (int)sizeof(struct rt_value);

        /* Set an integer constant. */
        ASM {
                /* x0 = env */
                /* x1 = &env->frame->tmpvar[0] */

                /* x2 = &env->frame->tmpvar[dst] */
                MOVZ            (REG_X2, IMM16(dst), LSL_0);
                ADD             (REG_X2, REG_X2, REG_X1);

                /* env->frame->tmpvar[dst].type = NOCT_VALUE_DOUBLE */
                if (write_tag) {
                MOVZ            (REG_X3, IMM16(6), LSL_0);
                STR             (REG_X3, REG_X2); }

                /* env->frame->tmpvar[dst].val.i = val */
                MOVZ            (REG_X3, IMM16(val & 0xffff), LSL_0);
                MOVK            (REG_X3, IMM16((val >> 16) & 0xffff), LSL_16);
                MOVK            (REG_X3, IMM16((val >> 32) & 0xffff), LSL_32);
                MOVK            (REG_X3, IMM16((val >> 48) & 0xffff), LSL_48);
                STR_IMM         (REG_X3, REG_X2, IMM9(8));
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
                /* x0 = env */
                /* x1 = &env->frame->tmpvar[0] */

                STP_PUSH        (REG_X0, REG_X1);
                STP_PUSH        (REG_X30, REG_XZR);
                
                /* Arg1 x0: env */

                /* Arg2 x1: &env->frame->tmpvar[dst] */
                MOVZ            (REG_X2, IMM16(dst), LSL_0);
                ADD             (REG_X1, REG_X1, REG_X2);

                /* Arg3: x2: val */
                MOVZ            (REG_X2, IMM16(((uint64_t)val) & 0xffff), LSL_0);
                MOVK            (REG_X2, IMM16((((uint64_t)val >> 16)) & 0xffff), LSL_16);
                MOVK            (REG_X2, IMM16((((uint64_t)val >> 32)) & 0xffff), LSL_32);
                MOVK            (REG_X2, IMM16((((uint64_t)val >> 48)) & 0xffff), LSL_48);

                /* Arg4: x3 = len */
                MOVZ            (REG_X3, IMM16(len & 0xffff), LSL_0);
                MOVK            (REG_X3, IMM16((len >> 16) & 0xffff), LSL_16);

                /* Arg5: x4 = hash */
                MOVZ            (REG_X4, IMM16(hash & 0xffff), LSL_0);
                MOVK            (REG_X4, IMM16((hash >> 16) & 0xffff), LSL_16);

                /* Call ex_make_string_with_hash(). */
                MOVZ            (REG_X5, IMM16(((uint64_t)ex_make_string_with_hash) & 0xffff), LSL_0);
                MOVK            (REG_X5, IMM16((((uint64_t)ex_make_string_with_hash) >> 16) & 0xffff), LSL_16);
                MOVK            (REG_X5, IMM16((((uint64_t)ex_make_string_with_hash) >> 32) & 0xffff), LSL_32);
                MOVK            (REG_X5, IMM16((((uint64_t)ex_make_string_with_hash) >> 48) & 0xffff), LSL_48);
                BLR             (REG_X5);

                /* If failed: */
                CMP_IMM         (REG_X0, IMM12(0));
                LDP_POP         (REG_X30, REG_X1);
                LDP_POP         (REG_X0, REG_X1);
                EXCEPTION_IF_EQUAL();
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
                /* x0 = env */
                /* x1 = &env->frame->tmpvar[0] */

                STP_PUSH        (REG_X0, REG_X1);
                STP_PUSH        (REG_X30, REG_XZR);

                /* Arg1 x0: env */

                /* Arg2 x1: &env->frame->tmpvar[dst] */
                MOVZ            (REG_X2, IMM16(dst), LSL_0);
                ADD             (REG_X1, REG_X1, REG_X2);

                /* Call ex_make_empty_array(). */
                MOVZ            (REG_X2, IMM16(((uint64_t)ex_make_empty_array) & 0xffff), LSL_0);
                MOVK            (REG_X2, IMM16((((uint64_t)ex_make_empty_array) >> 16) & 0xffff), LSL_16);
                MOVK            (REG_X2, IMM16((((uint64_t)ex_make_empty_array) >> 32) & 0xffff), LSL_32);
                MOVK            (REG_X2, IMM16((((uint64_t)ex_make_empty_array) >> 48) & 0xffff), LSL_48);
                BLR             (REG_X2);

                /* If failed: */
                CMP_IMM         (REG_X0, IMM12(0));
                LDP_POP         (REG_X30, REG_X1);
                LDP_POP         (REG_X0, REG_X1);
                EXCEPTION_IF_EQUAL();
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
                /* x0 = env */
                /* x1 = &env->frame->tmpvar[0] */

                STP_PUSH        (REG_X0, REG_X1);
                STP_PUSH        (REG_X30, REG_XZR);

                /* Arg1 x0: env */

                /* Arg2 x1: &env->frame->tmpvar[dst] */
                MOVZ            (REG_X2, IMM16(dst), LSL_0);
                ADD             (REG_X1, REG_X1, REG_X2);

                /* Call ex_make_empty_dict(). */
                MOVZ            (REG_X2, IMM16(((uint64_t)ex_make_empty_dict) & 0xffff), LSL_0);
                MOVK            (REG_X2, IMM16((((uint64_t)ex_make_empty_dict) >> 16) & 0xffff), LSL_16);
                MOVK            (REG_X2, IMM16((((uint64_t)ex_make_empty_dict) >> 32) & 0xffff), LSL_32);
                MOVK            (REG_X2, IMM16((((uint64_t)ex_make_empty_dict) >> 48) & 0xffff), LSL_48);
                BLR             (REG_X2);

                /* If failed: */
                CMP_IMM         (REG_X0, IMM12(0));
                LDP_POP         (REG_X30, REG_X1);
                LDP_POP         (REG_X0, REG_X1);
                EXCEPTION_IF_EQUAL();
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
#if defined(NOCT_USE_OPTIMIZER)
	if (ctx->vector_hint_active &&
	    dst == ctx->vector_hint_index_tmp &&
	    step == ctx->vector_hint_lanes)
		return true;
	if (ctx->packed_loop_hint_active &&
	    dst == ctx->packed_loop_index_tmp &&
	    step == ((ctx->packed_loop_flags & PLOOP_UNROLL4) != 0 ? 4 : 1))
		return true;
#endif

	dst *= (int)sizeof(struct rt_value);

        /* Increment an integer. */
        ASM {
                /* x0 = env */
                /* x1 = &env->frame->tmpvar[0] */

                /* Get &env->frame->tmpvar[dst] at x3. */
                MOVZ            (REG_X2, IMM16(dst), LSL_0);                        /* dst */
                ADD             (REG_X2, REG_X2, REG_X1);                        /* x3 = &env->frame->tmpvar[dst] = &env->frame->tmpvar[dst].type */

                /* env->frame->tmpvar[dst].val.i++ */
                LDR_IMM         (REG_X3, REG_X2, IMM9(8));                        /* tmp = &env->frame->tmpvar[dst].val.i */
                ADD_IMM         (REG_X3, REG_X3, IMM12(step));
                STR_IMM         (REG_X3, REG_X2, IMM9(8));                        /* env->frame->tmpvar[dst].val.i = tmp */
        }

	return true;
}

#if defined(NOCT_USE_OPTIMIZER)
static uint16_t
jit_arm64_read_u16(const uint8_t *p)
{
	return (uint16_t)(((uint16_t)p[0] << 8) | p[1]);
}

/* Emit a scalar Packed access through the negative w21 cursor. */
static INLINE bool
jit_arm64_put_packed_access(struct rt_jit_context *ctx, bool store,
			    int scale, bool is_signed, uint32_t rt,
			    uint32_t rn)
{
	uint32_t insn;

	if (store) {
		insn = scale == 4 ? 0xb820c800u :
			scale == 2 ? 0x7820c800u : 0x3820c800u;
	} else if (scale == 4) {
		insn = 0xb860c800u;
	} else if (scale == 2) {
		insn = is_signed ? 0x78e0c800u : 0x7860c800u;
	} else {
		insn = is_signed ? 0x38e0c800u : 0x3860c800u;
	}
	if (scale != 1)
		insn |= 1u << 12;
	return jit_put_word(ctx, insn | (REG_X21 << 16) | (rn << 5) | rt);
}

static INLINE uint32_t
jit_arm64_packed_base_reg(int slot)
{
	return slot == 0 ? REG_X19 : slot == 1 ? REG_X20 : REG_X22;
}

static bool
jit_arm64_packed_cursor(struct rt_jit_context *ctx, int base, int ofs,
			int scale, uint32_t *base_reg, int *cursor,
			int32_t *byte_disp)
{
	int i;
	int root;
	int32_t element_disp;

	UNUSED_PARAMETER(ofs);

	if (!ctx->packed_loop_hint_active ||
	    !rt_jit_ploop_current_access_disp(ctx, &element_disp))
		return false;
	if (element_disp < INT32_MIN / scale ||
	    element_disp > INT32_MAX / scale)
		return false;
	*byte_disp = element_disp * scale;
	root = rt_jit_ploop_resolve_base(ctx, base);
	for (i = 0; i < 3; i++) {
		if (ctx->packed_loop_base_tmp[i] == root &&
		    ctx->packed_loop_base_scale[i] == scale) {
			*base_reg = jit_arm64_packed_base_reg(i);
			*cursor = i;
			return true;
		}
	}
	return false;
}

static bool
jit_arm64_put_packed_access_disp(struct rt_jit_context *ctx, bool store,
				 int scale, bool is_signed, uint32_t rt,
				 uint32_t rn, int32_t byte_disp)
{
	uint32_t insn;
	uint32_t imm12;
	int shift;

	if ((ctx->packed_loop_flags & PLOOP_UNROLL4) != 0) {
		if (byte_disp >= 0 && byte_disp % scale == 0 &&
		    (uint32_t)(byte_disp / scale) <= 4095u) {
			imm12 = (uint32_t)(byte_disp / scale);
			if (store)
				insn = scale == 4 ? 0xb9000000u :
					scale == 2 ? 0x79000000u : 0x39000000u;
			else if (scale == 4)
				insn = 0xb9400000u;
			else if (scale == 2)
				insn = is_signed ? 0x79c00000u : 0x79400000u;
			else
				insn = is_signed ? 0x39c00000u : 0x39400000u;
			return jit_put_word(ctx, insn | (imm12 << 10) |
					    (rn << 5) | rt);
		}
		if (byte_disp < -256 || byte_disp > 255)
			return false;
		if (store)
			insn = scale == 4 ? 0xb8000000u :
				scale == 2 ? 0x78000000u : 0x38000000u;
		else if (scale == 4)
			insn = 0xb8400000u;
		else if (scale == 2)
			insn = is_signed ? 0x78c00000u : 0x78400000u;
		else
			insn = is_signed ? 0x38c00000u : 0x38400000u;
		return jit_put_word(ctx, insn |
			(((uint32_t)byte_disp & 0x1ffu) << 12) |
			(rn << 5) | rt);
	}
	if (byte_disp == 0)
		return jit_arm64_put_packed_access(ctx, store, scale,
					       is_signed, rt, rn);
	if (byte_disp < 0 || byte_disp % scale != 0 ||
	    (uint32_t)(byte_disp / scale) > 4095u)
		return false;
	shift = scale == 4 ? 2 : scale == 2 ? 1 : 0;
	/* x2 = adjusted_base + (w21 sxtw #scale). */
	if (!jit_put_word(ctx, 0x8b20c000u | (REG_X21 << 16) |
			 ((uint32_t)shift << 10) | (rn << 5) | REG_X2))
		return false;
	imm12 = (uint32_t)(byte_disp / scale);
	if (store)
		insn = scale == 4 ? 0xb9000000u :
			scale == 2 ? 0x79000000u : 0x39000000u;
	else if (scale == 4)
		insn = 0xb9400000u;
	else if (scale == 2)
		insn = is_signed ? 0x79c00000u : 0x79400000u;
	else
		insn = is_signed ? 0x39c00000u : 0x39400000u;
	return jit_put_word(ctx, insn | (imm12 << 10) |
			    (REG_X2 << 5) | rt);
}

static INLINE void
jit_arm64_invalidate_all_packed_loads(struct rt_jit_context *ctx)
{
	int i;

	for (i = 0; i < 3; i++)
		ctx->gpr_load_tmp[i] = -1;
}

static INLINE bool
jit_arm64_gpr_is_cached(struct rt_jit_context *ctx, int tmp)
{
	int i;

	for (i = 0; i < 3; i++)
		if (ctx->gpr_load_tmp[i] == tmp) return true;
	return false;
}

static void
jit_arm64_gpr_reset(struct rt_jit_context *ctx)
{
	uint32_t i;

	for (i = 0; i < ctx->func->tmpvar_size; i++) {
		ctx->gpr_tmp_reg[i] = -1;
		ctx->gpr_tmp_dirty[i] = 0;
		ctx->gpr_remat_valid[i] = 0;
		ctx->gpr_range_valid[i] = 0;
	}
	for (i = 0; i < 6; i++)
		ctx->gpr_reg_tmp[i] = -1;
	ctx->gpr_next_victim = 0;
	for (i = 0; i < 3; i++) {
		ctx->gpr_load_tmp[i] = -1;
		ctx->gpr_load_opcode[i] = -1;
		ctx->gpr_load_disp[i] = 0;
	}
}

static int
jit_arm64_gpr_limit(void)
{
	const char *value;
	char *end;
	long limit;

	value = getenv("NOCT_JIT_GPR_LIMIT");
	if (value == NULL || *value == '\0')
		return 6;
	limit = strtol(value, &end, 10);
	if (*end != '\0' || limit < 0)
		return 6;
	if (limit > 6)
		limit = 6;
	return (int)limit;
}

static bool
jit_arm64_gpr_spill(struct rt_jit_context *ctx, int slot)
{
	int tmp;
	int ofs;
	uint32_t reg;
	bool cached;
	int i;

	tmp = ctx->gpr_reg_tmp[slot];
	if (tmp < 0)
		return true;
	cached = false;
	for (i = 0; i < 3; i++)
		if (ctx->gpr_load_tmp[i] == tmp) cached = true;
	if (!cached && !ctx->has_vector_ops && ctx->packed_loop_hint_active &&
	    ctx->tmp_compiler_temp != NULL && ctx->tmp_compiler_temp[tmp] &&
	    rt_jit_ploop_next_use_lpc(ctx, tmp, ctx->lpc) == UINT32_MAX) {
		ctx->gpr_dead_drops++;
		ctx->gpr_tmp_dirty[tmp] = 0;
	}
	if (ctx->gpr_tmp_dirty[tmp]) {
		ofs = tmp * (int)sizeof(struct rt_value);
		reg = (uint32_t)(REG_X23 + slot);
		if (ctx->tmp_fixed_type == NULL ||
		    ctx->tmp_fixed_type[tmp] != NOCT_VALUE_INT ||
		    !ctx->tmp_frame_tag_known[tmp]) {
			ASM {
				MOVZ(REG_X4, IMM16(NOCT_VALUE_INT), LSL_0);
				STR_IMM(REG_X4, REG_X1, IMM9(ofs));
			}
			if (ctx->tmp_fixed_type != NULL &&
			    ctx->tmp_fixed_type[tmp] == NOCT_VALUE_INT)
				ctx->tmp_frame_tag_known[tmp] = 1;
		}
		ASM { STR_IMM(reg, REG_X1, IMM9(ofs + 8)); }
		ctx->gpr_spills++;
	}
	ctx->gpr_tmp_reg[tmp] = -1;
	ctx->gpr_tmp_dirty[tmp] = 0;
	ctx->gpr_reg_tmp[slot] = -1;
	jit_arm64_invalidate_packed_load(ctx, tmp);
	return true;
}

static bool
jit_arm64_gpr_alloc(struct rt_jit_context *ctx, int tmp,
			    unsigned pin_mask, bool load, uint32_t *reg)
{
	int slot;
	int i;
	int ofs;
	uint32_t next;
	uint32_t farthest;

	if (tmp < 0 || (uint32_t)tmp >= ctx->func->tmpvar_size)
		return false;
	slot = ctx->gpr_tmp_reg[tmp];
	if (slot >= 0) {
		ctx->gpr_hits++;
		*reg = (uint32_t)(REG_X23 + slot);
		return true;
	}
	ctx->gpr_misses++;
	for (slot = 0; slot < ctx->gpr_reg_limit; slot++) {
		if (ctx->gpr_reg_tmp[slot] < 0)
			break;
	}
	if (slot == ctx->gpr_reg_limit) {
		farthest = 0;
		slot = -1;
		for (i = 0; i < ctx->gpr_reg_limit; i++) {
			int candidate;
			int held;
			int j;
			bool is_cached;

			candidate = i;
			is_cached = false;
			if ((pin_mask & (1u << candidate)) != 0)
				continue;
			held = ctx->gpr_reg_tmp[candidate];
			for (j = 0; j < 3; j++)
				if (ctx->gpr_load_tmp[j] == held) is_cached = true;
			next = is_cached ? ctx->lpc :
				rt_jit_ploop_next_use_lpc(ctx, held, ctx->lpc);
			if (slot < 0 || next == UINT32_MAX || next >= farthest) {
				slot = candidate;
				farthest = next;
				if (next == UINT32_MAX) break;
			}
		}
		if (slot < 0)
			return false;
		if (!jit_arm64_gpr_spill(ctx, slot))
			return false;
	}
	ctx->gpr_reg_tmp[slot] = tmp;
	ctx->gpr_tmp_reg[tmp] = slot;
	ctx->gpr_tmp_dirty[tmp] = 0;
	*reg = (uint32_t)(REG_X23 + slot);
	if (load) {
		ofs = tmp * (int)sizeof(struct rt_value);
		if (!jit_put_ldr_w_imm(ctx, *reg, REG_X1,
					 (uint32_t)(ofs + 8)))
			return false;
	}
	return true;
}

static bool
jit_arm64_gpr_rebind(struct rt_jit_context *ctx, int dst, int src,
			 uint32_t *reg)
{
	int slot;
	int old;

	slot = ctx->gpr_tmp_reg[src];
	if (slot < 0)
		return false;
	old = ctx->gpr_tmp_reg[dst];
	if (old >= 0 && old != slot) {
		ctx->gpr_reg_tmp[old] = -1;
		ctx->gpr_tmp_reg[dst] = -1;
	}
	ctx->gpr_tmp_reg[src] = -1;
	ctx->gpr_tmp_dirty[src] = 0;
	ctx->gpr_reg_tmp[slot] = dst;
	ctx->gpr_tmp_reg[dst] = slot;
	jit_arm64_invalidate_packed_load(ctx, src);
	*reg = (uint32_t)(REG_X23 + slot);
	return true;
}

static bool
jit_arm64_gpr_flush(struct rt_jit_context *ctx)
{
	int slot;

	jit_arm64_invalidate_all_packed_loads(ctx);
	for (slot = 0; slot < 6; slot++) {
		if (!jit_arm64_gpr_spill(ctx, slot))
			return false;
	}
	return true;
}

static bool
jit_arm64_gpr_flush_required(struct rt_jit_context *ctx)
{
	bool active;
	bool ok;

	active = ctx->packed_loop_hint_active;
	ctx->packed_loop_hint_active = false;
	ok = jit_arm64_gpr_flush(ctx);
	ctx->packed_loop_hint_active = active;
	return ok;
}

/* Discover the at-most-two packed bases and their last memory opcode. */
static bool
jit_arm64_scan_vector_bases(struct rt_jit_context *ctx)
{
	uint32_t p;
	uint32_t size;
	int base;
	int i;
	uint8_t op;

	ctx->vector_base_tmp[0] = -1;
	ctx->vector_base_tmp[1] = -1;
	ctx->vector_base_last_lpc[0] = 0;
	ctx->vector_base_last_lpc[1] = 0;
	p = ctx->lpc;
	while (p < ctx->func->bytecode_size) {
		op = ctx->func->bytecode[p];
		base = -1;
		size = 0;
		switch (op) {
		case OP_VLOADI32X4:
		case OP_VLOADF32X4:
			if (p + 6 > ctx->func->bytecode_size) return false;
			base = jit_arm64_read_u16(&ctx->func->bytecode[p + 2]);
			size = 6;
			break;
		case OP_VSTOREI32X4:
		case OP_VSTOREF32X4:
			if (p + 6 > ctx->func->bytecode_size) return false;
			base = jit_arm64_read_u16(&ctx->func->bytecode[p + 1]);
			size = 6;
			break;
		case OP_VSPLATI32:
		case OP_VSPLATF32:
			size = 4; break;
		case OP_VGETLANEI32:
		case OP_VGETLANEF32:
			size = 5; break;
		case OP_VMOV128:
		case OP_VCVTI32F32X4:
		case OP_VCVTF32I32X4:
			size = 3; break;
		case OP_VADDI32X4: case OP_VSUBI32X4:
		case OP_VMULI32X4: case OP_VAND128:
		case OP_VOR128: case OP_VXOR128:
		case OP_VSHLI32X4: case OP_VSHRI32X4:
		case OP_VADDF32X4: case OP_VSUBF32X4:
		case OP_VMULF32X4: case OP_VDIVF32X4:
		case OP_VMINS32X4: case OP_VMAXS32X4:
			size = 4; break;
		case OP_VORI32X4I:
			size = 5; break;
		case OP_VFMAF32X4:
			size = 5; break;
		case OP_VCMPI32X4:
		case OP_VCMPF32X4:
		case OP_VSELECT128:
			size = 5; break;
		case OP_VMASKSTOREI32X4:
			if (p + 7 > ctx->func->bytecode_size) return false;
			base = jit_arm64_read_u16(&ctx->func->bytecode[p + 1]);
			size = 7; break;
		case OP_VINDUCTF32X4:
			size = 6; break;
		case OP_VGATHERI32X4_CHECKED:
			size = 7; break;
		case OP_INC:
			size = 4; break;
		case OP_SUBJNZ:
			return true;
		default:
			return false;
		}
		if (p + size > ctx->func->bytecode_size)
			return false;
		if (base >= 0) {
			for (i = 0; i < 2; i++) {
				if (ctx->vector_base_tmp[i] == base)
					break;
				if (ctx->vector_base_tmp[i] < 0) {
					ctx->vector_base_tmp[i] = base;
					break;
				}
			}
			if (i >= 2)
				return false;
			ctx->vector_base_last_lpc[i] = p;
		}
		p += size;
	}
	return false;
}

/* Scalar Packed-loop declaration.  w21 is the negative element cursor. */
static INLINE bool
jit_visit_arm64_ploop_hint_op(struct rt_jit_context *ctx)
{
	int stop_ofs;
	int remaining_ofs;
	int base_ofs;
	int shift;
	int i;
	uint32_t base_reg;

	if (!rt_jit_visit_ploop_hint_op(ctx))
		return false;
	if (!rt_jit_context_init_regcache(ctx))
		return false;
	ctx->packed_loop_hint_active =
		getenv("NOCT_JIT_REGCACHE_DISABLE") == NULL &&
		(ctx->packed_loop_flags & (PLOOP_TYPED_INT |
		 PLOOP_ALLOW_REGCACHE | PLOOP_HAS_CONTROL)) ==
		(PLOOP_TYPED_INT | PLOOP_ALLOW_REGCACHE) &&
		rt_jit_scan_packed_loop(ctx, true);
	if (ctx->packed_loop_hint_active) {
		ctx->packed_loop_index_alias_count = 1;
		ctx->packed_loop_index_alias[0] =
			(uint16_t)ctx->packed_loop_index_tmp;
		ctx->packed_loop_index_alias_disp[0] = 0;
		ctx->packed_loop_base_alias_count = 0;
	}
	if (getenv("NOCT_JIT_REGCACHE_DEBUG") != NULL)
		fprintf(stderr,
			"noct-jit-regcache: func=%s arm64 hint index=%d stop=%d remaining=%d flags=0x%x accepted=%d reason=%s\n",
			ctx->func->name != NULL ? ctx->func->name : "?",
			ctx->packed_loop_index_tmp, ctx->packed_loop_stop_tmp,
			ctx->packed_loop_remaining_tmp, ctx->packed_loop_flags,
			ctx->packed_loop_hint_active ? 1 : 0,
			ctx->packed_loop_reject_reason != NULL ?
			ctx->packed_loop_reject_reason : "disabled");
	if (!ctx->packed_loop_hint_active)
		return true;
	jit_arm64_gpr_reset(ctx);
	ctx->gpr_reg_limit = jit_arm64_gpr_limit();
	ctx->gpr_cache_active = ctx->gpr_reg_limit > 0;

	/* x2 = sign-extended stop. */
	stop_ofs = ctx->packed_loop_stop_tmp * (int)sizeof(struct rt_value);
	ASM { LDR_W_IMM(REG_X2, REG_X1, (uint32_t)(stop_ofs + 8)); }
	if (!jit_put_word(ctx, 0x93407c42u)) /* sxtw x2,w2 */
		return false;
	remaining_ofs = ctx->packed_loop_remaining_tmp *
		(int)sizeof(struct rt_value);
	if ((ctx->packed_loop_flags & PLOOP_UNROLL4) != 0) {
		ASM { LDR_W_IMM(REG_X21, REG_X1,
				    (uint32_t)(remaining_ofs + 8)); }
		/* x2 = stop - remaining = bulk start. */
		if (!jit_put_word(ctx, 0xcb20c000u | (REG_X21 << 16) |
				 (REG_X2 << 5) | REG_X2))
			return false;
	}
	for (i = 0; i < 3 && ctx->packed_loop_base_tmp[i] >= 0; i++) {
		base_ofs = ctx->packed_loop_base_tmp[i] *
			(int)sizeof(struct rt_value);
		base_reg = jit_arm64_packed_base_reg(i);
		shift = ctx->packed_loop_base_scale[i] == 4 ? 2 :
			ctx->packed_loop_base_scale[i] == 2 ? 1 : 0;
		if (!jit_put_ldr_imm(ctx, base_reg, REG_X1,
				     (uint32_t)(base_ofs + 8)) ||
		    !jit_put_word(ctx, 0x8b000000u | (REG_X2 << 16) |
				 ((uint32_t)shift << 10) |
				 (base_reg << 5) | base_reg))
			return false;
	}
	if ((ctx->packed_loop_flags & PLOOP_UNROLL4) == 0) {
		ASM {
			LDR_W_IMM(REG_X21, REG_X1,
				    (uint32_t)(remaining_ofs + 8));
		}
		if (!jit_put_word(ctx, 0x4b0003e0u |
				 (REG_X21 << 16) | REG_X21))
			return false; /* neg w21,w21 */
	}
	if (getenv("NOCT_JIT_REGCACHE_DEBUG") != NULL)
		fprintf(stderr,
			"noct-jit-regcache: func=%s arm64 mode=cursor bases=%d gprs=%d\n",
			ctx->func->name != NULL ? ctx->func->name : "?",
			ctx->packed_loop_base_tmp[2] >= 0 ? 3 :
			ctx->packed_loop_base_tmp[1] >= 0 ? 2 : 1,
			ctx->gpr_reg_limit);
	return true;
}

/* Vector-loop register declaration.  x21 holds the remaining count. */
static INLINE bool
jit_visit_vindex_hint_op(
	struct rt_jit_context *ctx)
{
	int index_tmp, stop_tmp, remaining_tmp;
	int required_vregs, lanes, flags;
	int remaining_ofs;

	CONSUME_TMPVAR(index_tmp);
	CONSUME_TMPVAR(stop_tmp);
	CONSUME_TMPVAR(remaining_tmp);
	CONSUME_IMM8(required_vregs);
	CONSUME_IMM8(lanes);
	CONSUME_IMM8(flags);
	if (required_vregs > 16 || (flags & VINDEX_FORCE_SCALAR) != 0)
		ctx->simd_caps = 0;

	ctx->vector_hint_active = lanes > 0 &&
		(ctx->simd_caps & JIT_SIMD_CAP_NEON) != 0 &&
		(flags & VINDEX_CURSOR_ONLY) != 0 &&
		jit_arm64_scan_vector_bases(ctx);
	ctx->vector_hint_index_tmp = index_tmp;
	ctx->vector_hint_stop_tmp = stop_tmp;
	ctx->vector_hint_remaining_tmp = remaining_tmp;
	ctx->vector_hint_lanes = lanes;
	ctx->vector_hint_flags = flags;
	remaining_ofs = remaining_tmp * (int)sizeof(struct rt_value);
	if (ctx->vector_hint_active) {
		int base0_ofs = ctx->vector_base_tmp[0] *
			(int)sizeof(struct rt_value);

		ASM {
			LDR_W_IMM(REG_X21, REG_X1,
				    (uint32_t)(remaining_ofs + 8));
			LDR_IMM(REG_X19, REG_X1, IMM9(base0_ofs + 8));
		}
		if (ctx->vector_base_tmp[1] >= 0) {
			int base1_ofs = ctx->vector_base_tmp[1] *
				(int)sizeof(struct rt_value);

			ASM { LDR_IMM(REG_X20, REG_X1, IMM9(base1_ofs + 8)); }
		}
	}
	return true;
}

/* Semantic countdown latch; accepted hints keep the count in x21. */
static INLINE bool
jit_visit_subjnz_op(
	struct rt_jit_context *ctx)
{
	int value, decrement;
	uint32_t target_lpc;
	uint32_t *target_code;
	uint32_t i;
	int offset;
	int value_ofs;
	bool hinted;
	bool packed_hinted;

	CONSUME_TMPVAR(value);
	CONSUME_IMM8(decrement);
	CONSUME_IMM32(target_lpc);
	if (target_lpc >= (uint32_t)(ctx->func->bytecode_size + 1)) {
		rt_error(ctx->env, BROKEN_BYTECODE);
		return false;
	}
	packed_hinted = ctx->packed_loop_hint_active &&
		 value == ctx->packed_loop_remaining_tmp &&
		 decrement == ((ctx->packed_loop_flags & PLOOP_UNROLL4) != 0 ?
			4 : 1);
	hinted = (ctx->vector_hint_active &&
		  value == ctx->vector_hint_remaining_tmp &&
		  decrement == ctx->vector_hint_lanes) || packed_hinted;
	value_ofs = value * (int)sizeof(struct rt_value);
	if (!hinted) {
		ASM {
			LDR_W_IMM(REG_X21, REG_X1, (uint32_t)(value_ofs + 8));
		}
	}
	if (packed_hinted) {
		if ((ctx->packed_loop_flags & PLOOP_UNROLL4) != 0) {
			int slot;

			for (slot = 0; slot < 3 &&
			     ctx->packed_loop_base_tmp[slot] >= 0; slot++) {
				uint32_t reg;
				uint32_t amount;

				reg = jit_arm64_packed_base_reg(slot);
				amount = (uint32_t)
					(4 * ctx->packed_loop_base_scale[slot]);
				if (!jit_put_word(ctx, 0x91000000u |
						 (amount << 10) |
						 (reg << 5) | reg))
					return false;
			}
		}
		/* Advance the negative cursor by the loop factor and set NZCV. */
		if (!jit_put_word(ctx,
			(ctx->packed_loop_flags & PLOOP_UNROLL4) != 0 ?
				(0x71000000u | ((uint32_t)decrement << 10) |
				 (REG_X21 << 5) | REG_X21) :
				(0x31000000u | ((uint32_t)decrement << 10) |
				 (REG_X21 << 5) | REG_X21)))
			return false;
	} else {
		/* subs x21, x21, #decrement */
		if (!jit_put_word(ctx, 0xf1000015u |
				 ((uint32_t)decrement << 10) |
				 (REG_X21 << 5)))
			return false;
	}
	if (!hinted) {
		ASM {
			STR_W_IMM(REG_X21, REG_X1, (uint32_t)(value_ofs + 8));
		}
	}

	/* A backward loop target is already in the PC map, so emit the
	   one-word short branch directly. */
	target_code = NULL;
	for (i = 0; i < ctx->pc_entry_count; i++) {
		if (ctx->pc_entry[i].lpc == target_lpc) {
			target_code = ctx->pc_entry[i].code;
			break;
		}
	}
	if (target_code == NULL) {
		rt_error(ctx->env, N_TR("Branch target not found."));
		return false;
	}
	offset = (int)((intptr_t)target_code - (intptr_t)ctx->code);
	if (getenv("NOCT_JIT_FORCE_LONG_BRANCH") == NULL &&
	    offset >= -1048576 && offset <= 1048572) {
		ASM { BNE(IMM19(offset)); }
	} else {
		ASM {
			BEQ(IMM19(8));
			B(offset - 4);
		}
	}
	/* The scanner rejects scalar loop-carried values.  Keep the cache
	 * register-canonical over the backedge and materialize it only on the
	 * fall-through exit. */
	if (packed_hinted) {
		ctx->packed_loop_hint_active = false;
		if (ctx->gpr_cache_active && !jit_arm64_gpr_flush(ctx))
			return false;
	}

	if (ctx->vector_hint_active && hinted &&
	    (ctx->vector_hint_flags & VINDEX_WRITEBACK_STOP) != 0) {
		int index_ofs = ctx->vector_hint_index_tmp *
			(int)sizeof(struct rt_value);
		int stop_ofs = ctx->vector_hint_stop_tmp *
			(int)sizeof(struct rt_value);

		ASM {
			LDR_W_IMM(REG_X2, REG_X1, (uint32_t)(stop_ofs + 8));
			STR_W_IMM(REG_X2, REG_X1, (uint32_t)(index_ofs + 8));
		}
	}
	if (packed_hinted) {
		int index_ofs = ctx->packed_loop_index_tmp *
			(int)sizeof(struct rt_value);
		int stop_ofs = ctx->packed_loop_stop_tmp *
			(int)sizeof(struct rt_value);

		ASM {
			MOVZ(REG_X2, IMM16(0), LSL_0);
			STR_W_IMM(REG_X2, REG_X1, (uint32_t)(value_ofs + 8));
			LDR_W_IMM(REG_X2, REG_X1, (uint32_t)(stop_ofs + 8));
			STR_W_IMM(REG_X2, REG_X1, (uint32_t)(index_ofs + 8));
		}
		if (getenv("NOCT_JIT_REGCACHE_DEBUG") != NULL)
			fprintf(stderr,
				"noct-jit-regcache: func=%s arm64 hits=%u misses=%u spills=%u proven-div=%u\n",
				ctx->func->name != NULL ? ctx->func->name : "?",
				ctx->gpr_hits, ctx->gpr_misses,
				ctx->gpr_spills,
				ctx->gpr_proven_divisions);
	}
	ctx->vector_hint_active = false;
	ctx->packed_loop_hint_active = false;
	ctx->gpr_cache_active = false;
	return true;
}

static INLINE bool
jit_visit_vori32x4i_op(
	struct rt_jit_context *ctx)
{
	int dst, src1, imm, shift;
	int src2;
	int vd, vs;

	CONSUME_IMM8(dst);
	CONSUME_IMM8(src1);
	CONSUME_IMM8(imm);
	CONSUME_IMM8(shift);
	if ((ctx->simd_caps & JIT_SIMD_CAP_NEON) == 0) {
		src2 = (imm << 8) | shift;
		ASM_BINARY_OP(ex_vori32x4i_helper);
		return true;
	}
	vd = dst < 8 ? dst : dst < 16 ? dst + 8 : -1;
	vs = src1 < 8 ? src1 : src1 < 16 ? src1 + 8 : -1;
	if (vd < 0 || vs < 0) {
		rt_error(ctx->env, BROKEN_BYTECODE);
		return false;
	}
	if (vd != vs) {
		/* mov vD.16b, vS.16b (alias of orr vD.16b,vS.16b,vS.16b) */
		if (!jit_put_word(ctx, 0x4ea01c00u |
				 ((uint32_t)vs << 16) |
				 ((uint32_t)vs << 5) | (uint32_t)vd))
			return false;
	}
	if (imm == 0xff && shift == 24) {
		if (!jit_put_word(ctx, 0x4f0777e0u | (uint32_t)vd))
			return false;
		return true;
	}
	/* Current LIR only emits the verified opaque-alpha form. */
	rt_error(ctx->env, BROKEN_BYTECODE);
	return false;
}

static INLINE bool
jit_visit_vfmaf32x4_op(
	struct rt_jit_context *ctx)
{
	int dst, src1, src2, src3;
	int vd, vn, vm, va, vacc;

	CONSUME_IMM8(dst);
	CONSUME_IMM8(src1);
	CONSUME_IMM8(src2);
	CONSUME_IMM8(src3);
	if ((ctx->simd_caps & JIT_SIMD_CAP_FMAF32X4) == 0) {
		int packed_src2_src3 = (src2 << 8) | src3;

		src2 = packed_src2_src3;
		ASM_BINARY_OP(ex_vfmaf32x4_helper);
		return true;
	}
	vd = dst < 8 ? dst : dst < 16 ? dst + 8 : -1;
	vn = src1 < 8 ? src1 : src1 < 16 ? src1 + 8 : -1;
	vm = src2 < 8 ? src2 : src2 < 16 ? src2 + 8 : -1;
	va = src3 < 8 ? src3 : src3 < 16 ? src3 + 8 : -1;
	if (vd < 0 || vn < 0 || vm < 0 || va < 0) {
		rt_error(ctx->env, BROKEN_BYTECODE);
		return false;
	}
	vacc = vd;
	if (vd != va && (vd == vn || vd == vm))
		vacc = 31; /* scratch outside the logical vreg mapping */
	if (vacc != va) {
		/* mov vD.16b, vA.16b */
		if (!jit_put_word(ctx, 0x4ea01c00u |
				 ((uint32_t)va << 16) |
				 ((uint32_t)va << 5) | (uint32_t)vacc))
			return false;
	}
	/* fmla vD.4s, vN.4s, vM.4s: D = A + N * M. */
	if (!jit_put_word(ctx, 0x4e20cc00u |
			  ((uint32_t)vm << 16) |
			  ((uint32_t)vn << 5) | (uint32_t)vacc))
		return false;
	if (vacc != vd) {
		/* mov vD.16b, v31.16b */
		return jit_put_word(ctx, 0x4ea01c00u | (31u << 16) |
				    (31u << 5) | (uint32_t)vd);
	}
	return true;
}

static INLINE bool
jit_visit_vcmpi32x4_op(struct rt_jit_context *ctx)
{
	int dst, src1, src2, pred;
	int vd, vn, vm;
	uint32_t base;
	bool invert;
	bool swap;

	CONSUME_IMM8(dst);
	CONSUME_IMM8(src1);
	CONSUME_IMM8(src2);
	CONSUME_IMM8(pred);
	if (dst < 0 || dst >= 16 || src1 < 0 || src1 >= 16 ||
	    src2 < 0 || src2 >= 16 || pred < 0 ||
	    pred >= VCMP_PREDICATE_COUNT) {
		rt_error(ctx->env, BROKEN_BYTECODE);
		return false;
	}
	if ((ctx->simd_caps & JIT_SIMD_CAP_NEON) == 0) {
		src2 = (src2 << 8) | pred;
		ASM_BINARY_OP(ex_vcmpi32x4_helper);
		return true;
	}
	vd = dst < 8 ? dst : dst + 8;
	vn = src1 < 8 ? src1 : src1 + 8;
	vm = src2 < 8 ? src2 : src2 + 8;
	invert = false;
	swap = false;
	if (pred == VCMP_NE) {
		base = 0x6ea08c00u;
		invert = true;
	} else if (pred == VCMP_EQ) {
		base = 0x6ea08c00u;
	} else {
		if (pred == VCMP_LT || pred == VCMP_LE)
			swap = true;
		base = (pred == VCMP_LE || pred == VCMP_GE) ?
			0x4ea03c00u : 0x4ea03400u;
	}
	if (swap) {
		int tmp = vn;

		vn = vm;
		vm = tmp;
	}
	if (!jit_put_word(ctx, base | ((uint32_t)vm << 16) |
				 ((uint32_t)vn << 5) | 31u))
		return false;
	if (invert && !jit_put_word(ctx, 0x6e205800u | (31u << 5) | 31u))
		return false;
	if (vd != 31 && !jit_put_word(ctx, 0x4ea01c00u | (31u << 16) |
					     (31u << 5) | (uint32_t)vd))
		return false;
	return true;
}

static INLINE bool
jit_visit_vcmpf32x4_op(struct rt_jit_context *ctx)
{
	int dst, src1, src2, pred;
	int vd, vn, vm;
	uint32_t base;
	bool invert;
	bool swap;

	CONSUME_IMM8(dst);
	CONSUME_IMM8(src1);
	CONSUME_IMM8(src2);
	CONSUME_IMM8(pred);
	if (dst < 0 || dst >= 16 || src1 < 0 || src1 >= 16 ||
	    src2 < 0 || src2 >= 16 || pred < 0 ||
	    pred >= VCMP_PREDICATE_COUNT) {
		rt_error(ctx->env, BROKEN_BYTECODE);
		return false;
	}
	if ((ctx->simd_caps & JIT_SIMD_CAP_NEON) == 0) {
		src2 = (src2 << 8) | pred;
		ASM_BINARY_OP(ex_vcmpf32x4_helper);
		return true;
	}
	vd = dst < 8 ? dst : dst + 8;
	vn = src1 < 8 ? src1 : src1 + 8;
	vm = src2 < 8 ? src2 : src2 + 8;
	invert = false;
	swap = false;
	if (pred == VCMP_NE) {
		base = 0x4e20e400u;
		invert = true;
	} else if (pred == VCMP_EQ) {
		base = 0x4e20e400u;
	} else {
		if (pred == VCMP_LT || pred == VCMP_LE)
			swap = true;
		base = (pred == VCMP_LE || pred == VCMP_GE) ?
			0x6e20e400u : 0x6ea0e400u;
	}
	if (swap) {
		int tmp = vn;

		vn = vm;
		vm = tmp;
	}
	if (!jit_put_word(ctx, base | ((uint32_t)vm << 16) |
				 ((uint32_t)vn << 5) | 31u))
		return false;
	if (invert && !jit_put_word(ctx, 0x6e205800u | (31u << 5) | 31u))
		return false;
	if (vd != 31 && !jit_put_word(ctx, 0x4ea01c00u | (31u << 16) |
					     (31u << 5) | (uint32_t)vd))
		return false;
	return true;
}

static INLINE bool
jit_visit_vselect128_op(struct rt_jit_context *ctx)
{
	int dst, mask, src1, src2;
	int vd, vm, vt, vf;

	CONSUME_IMM8(dst); CONSUME_IMM8(mask);
	CONSUME_IMM8(src1); CONSUME_IMM8(src2);
	if (dst < 0 || dst >= 16 || mask < 0 || mask >= 16 ||
	    src1 < 0 || src1 >= 16 || src2 < 0 || src2 >= 16) {
		rt_error(ctx->env, BROKEN_BYTECODE);
		return false;
	}
	if ((ctx->simd_caps & JIT_SIMD_CAP_NEON) == 0) {
		src2 = (src1 << 8) | src2;
		src1 = mask;
		ASM_BINARY_OP(ex_vselect128_helper);
		return true;
	}
	vd = dst < 8 ? dst : dst + 8;
	vm = mask < 8 ? mask : mask + 8;
	vt = src1 < 8 ? src1 : src1 + 8;
	vf = src2 < 8 ? src2 : src2 + 8;
	/* v31 = mask; bsl v31.16b, true.16b, false.16b. */
	if (!jit_put_word(ctx, 0x4ea01c00u | ((uint32_t)vm << 16) |
				 ((uint32_t)vm << 5) | 31u) ||
	    !jit_put_word(ctx, 0x6e601c00u | ((uint32_t)vf << 16) |
				 ((uint32_t)vt << 5) | 31u))
		return false;
	if (vd != 31 && !jit_put_word(ctx, 0x4ea01c00u | (31u << 16) |
					     (31u << 5) | (uint32_t)vd))
		return false;
	return true;
}

static INLINE bool
jit_visit_vmaskstorei32x4_op(struct rt_jit_context *ctx)
{
	int dst, src1, src2, mask;
	int vv, vm;
	int base, ofs, lane;

	CONSUME_TMPVAR(dst); CONSUME_TMPVAR(src1);
	CONSUME_IMM8(src2); CONSUME_IMM8(mask);
	if (src2 < 0 || src2 >= 16 || mask < 0 || mask >= 16) {
		rt_error(ctx->env, BROKEN_BYTECODE);
		return false;
	}
	if ((ctx->simd_caps & JIT_SIMD_CAP_NEON) == 0) {
		src2 = (src2 << 8) | mask;
		ASM_BINARY_OP(ex_vmaskstorei32x4_helper);
		return true;
	}
	vv = src2 < 8 ? src2 : src2 + 8;
	vm = mask < 8 ? mask : mask + 8;
	base = dst * (int)sizeof(struct rt_value);
	ofs = src1 * (int)sizeof(struct rt_value);
	ASM {
		LDR_IMM(REG_X2, REG_X1, IMM9(base + 8));
		LDR_W_IMM(REG_X3, REG_X1, (uint32_t)(ofs + 8));
		LSL_IMM(REG_X3, REG_X3, 2);
		ADD(REG_X2, REG_X2, REG_X3);
	}
	for (lane = 0; lane < 4; lane++) {
		uint32_t imm5;

		imm5 = ((uint32_t)lane << 3) | 4u;
		/* umov w3, vMask.s[lane]; cbz w3, after lane store */
		if (!jit_put_word(ctx, 0x0e003c00u | (imm5 << 16) |
					 ((uint32_t)vm << 5) | 3u) ||
		    !jit_put_word(ctx, 0x34000000u | (3u << 5) | 3u) ||
		    !jit_put_word(ctx, 0x0e003c00u | (imm5 << 16) |
					 ((uint32_t)vv << 5) | 4u))
			return false;
		STR_W_IMM(REG_X4, REG_X2, (uint32_t)lane * 4u);
	}
	return true;
}

static INLINE bool
jit_visit_vinductf32x4_op(struct rt_jit_context *ctx)
{
	int dst, src1, src2;

	CONSUME_IMM8(dst); CONSUME_TMPVAR(src1); CONSUME_TMPVAR(src2);
	if (dst < 0 || dst >= 16) {
		rt_error(ctx->env, BROKEN_BYTECODE);
		return false;
	}
	ASM_BINARY_OP(ex_vinductf32x4_helper);
	return true;
}

static INLINE bool
jit_visit_vgatheri32x4_checked_op(struct rt_jit_context *ctx)
{
	int dst, src1, plen, vi, src2;

	CONSUME_IMM8(dst); CONSUME_TMPVAR(src1);
	CONSUME_TMPVAR(plen); CONSUME_IMM8(vi);
	if (dst < 0 || dst >= 16 || vi < 0 || vi >= 16) {
		rt_error(ctx->env, BROKEN_BYTECODE);
		return false;
	}
	src2 = (plen << 8) | vi;
	ASM_BINARY_OP(ex_vgatheri32x4_checked_helper);
	return true;
}
#endif

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

        /* if (!rt_add_helper(env, dst, src1, src2)) return false; */
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

        /* if (!rt_sub_helper(env, dst, src1, src2)) return false; */
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

        /* if (!rt_mul_helper(env, dst, src1, src2)) return false; */
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

        /* if (!rt_div_helper(env, dst, src1, src2)) return false; */
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

        /* if (!rt_mod_helper(env, dst, src1, src2)) return false; */
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

        /* if (!rt_and_helper(env, dst, src1, src2)) return false; */
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

        /* if (!rt_or_helper(env, dst, src1, src2)) return false; */
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

        /* if (!rt_xor_helper(env, dst, src1, src2)) return false; */
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

        /* if (!rt_shl_helper(env, dst, src1, src2)) return false; */
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

        /* if (!rt_shl_helper(env, dst, src1, src2)) return false; */
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

        /* if (!rt_neg_helper(env, dst, src)) return false; */
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

        /* if (!rt_not_helper(env, dst, src)) return false; */
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

        /* if (!rt_lt_helper(env, dst, src1, src2)) return false; */
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

        /* if (!rt_lte_helper(env, dst, src1, src2)) return false; */
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

        /* if (!rt_eq_helper(env, dst, src1, src2)) return false; */
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

        /* if (!rt_neq_helper(env, dst, src1, src2)) return false; */
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

        /* if (!rt_gte_helper(env, dst, src1, src2)) return false; */
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

        /* if (!rt_gt_helper(env, dst, src1, src2)) return false; */
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
                /* x0 = env */
                /* x1 = &env->frame->tmpvar[0] */

                /* x3 = &env->frame->tmpvar[src1].val.i */
                MOVZ                    (REG_X3, IMM16(src1), LSL_0);
                ADD                     (REG_X3, REG_X3, REG_X1);
                LDR_IMM                 (REG_X3, REG_X3, 8);

                /* x4 = &env->frame->tmpvar[src2].val.i */
                MOVZ                    (REG_X4, IMM16(src2), LSL_0);
                ADD                     (REG_X4, REG_X4, REG_X1);
                LDR_IMM                 (REG_X4, REG_X4, 8);

                /* src1 == src2 */
                CMP_W3_W4        ();
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

        /* if (!rt_loadarray_helper(env, dst, src1, src2)) return false; */
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

        /* if (!jit_storearray_helper(env, dst, src1, src2)) return false; */
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

        /* if (!jit_len_helper(env, dst, src)) return false; */
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

        /* if (!jit_getdictkeybyindex_helper(env, dst, src1, src2)) return false; */
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

        /* if (!jit_getdictvalbyindex_helper(env, dst, src1, src2)) return false; */
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
        uint32_t len, hash;
        uint64_t src;

        CONSUME_TMPVAR(dst);
        CONSUME_STRING(src_s, len, hash);
        src = (uint64_t)(intptr_t)src_s;

        /* if (!jit_loadsymbol_helper(env, dst, src, len, hash)) return false; */
        ASM {
                /* x0 = env */
                /* x1 = &env->frame->tmpvar[0] */

                STP_PUSH        (REG_X0, REG_X1);
                STP_PUSH        (REG_X30, REG_XZR);

                /* Arg1 x0: env */

                /* Arg2 x1: dst */
                MOVZ            (REG_X1, IMM16(dst), LSL_0);

                /* Arg3 x2: src */
                MOVZ            (REG_X2, IMM16(src & 0xffff), LSL_0);
                MOVK            (REG_X2, IMM16((src >> 16) & 0xffff), LSL_16);
                MOVK            (REG_X2, IMM16((src >> 32) & 0xffff), LSL_32);
                MOVK            (REG_X2, IMM16((src >> 48) & 0xffff), LSL_48);

                /* Arg4 x3: len */
                MOVZ            (REG_X3, IMM16(len & 0xffff), LSL_0);
                MOVK            (REG_X3, IMM16((len >> 16) & 0xffff), LSL_16);

                /* Arg5 x4: hash */
                MOVZ            (REG_X4, IMM16(hash & 0xffff), LSL_0);
                MOVK            (REG_X4, IMM16((hash >> 16) & 0xffff), LSL_16);

                /* Call ex_loadsymbol_helper(). */
                MOVZ            (REG_X6, IMM16(((uint64_t)ex_loadsymbol_helper) & 0xffff), LSL_0);
                MOVK            (REG_X6, IMM16((((uint64_t)ex_loadsymbol_helper) >> 16) & 0xffff), LSL_16);
                MOVK            (REG_X6, IMM16((((uint64_t)ex_loadsymbol_helper) >> 32) & 0xffff), LSL_32);
                MOVK            (REG_X6, IMM16((((uint64_t)ex_loadsymbol_helper) >> 48) & 0xffff), LSL_48);
                BLR             (REG_X6);

                /* If failed: */
                CMP_IMM         (REG_X0, IMM12(0));
                LDP_POP         (REG_X30, REG_X1);
                LDP_POP         (REG_X0, REG_X1);
                EXCEPTION_IF_EQUAL();
        }

        return true;
}

/* Visit a OP_STORESYMBOL instruction. */
static INLINE bool
jit_visit_storesymbol_op(
        struct rt_jit_context *ctx)
{
        const char *dst_s;
        uint32_t len, hash;
        uint64_t dst;
        int src;

        CONSUME_STRING(dst_s, len, hash);
        CONSUME_TMPVAR(src);
        dst = (uint64_t)(intptr_t)dst_s;

        /* if (!rt_storesymbol_helper(env, dst, len, hash, src)) return false; */
        ASM {
                /* x0 = env */
                /* x1 = &env->frame->tmpvar[0] */

                STP_PUSH        (REG_X0, REG_X1);
                STP_PUSH        (REG_X30, REG_XZR);

                /* Arg1 x0: env */

                /* Arg2 x1: dst */
                MOVZ            (REG_X1, IMM16(dst & 0xffff), LSL_0);
                MOVK            (REG_X1, IMM16((dst >> 16) & 0xffff), LSL_16);
                MOVK            (REG_X1, IMM16((dst >> 32) & 0xffff), LSL_32);
                MOVK            (REG_X1, IMM16((dst >> 48) & 0xffff), LSL_48);

                /* Arg3 x2: len */
                MOVZ            (REG_X2, IMM16(len & 0xffff), LSL_0);
                MOVK            (REG_X2, IMM16((len >> 16) & 0xffff), LSL_16);

                /* Arg4 x3: hash */
                MOVZ            (REG_X3, IMM16(hash & 0xffff), LSL_0);
                MOVK            (REG_X3, IMM16((hash >> 16) & 0xffff), LSL_16);

                /* Arg5 x4: src */
                MOVZ            (REG_X4, IMM16(src), LSL_0);

                /* Call ex_storesymbol_helper(). */
                MOVZ            (REG_X5, IMM16(((uint64_t)ex_storesymbol_helper) & 0xffff), LSL_0);
                MOVK            (REG_X5, IMM16((((uint64_t)ex_storesymbol_helper) >> 16) & 0xffff), LSL_16);
                MOVK            (REG_X5, IMM16((((uint64_t)ex_storesymbol_helper) >> 32) & 0xffff), LSL_32);
                MOVK            (REG_X5, IMM16((((uint64_t)ex_storesymbol_helper) >> 48) & 0xffff), LSL_48);
                BLR             (REG_X5);

                /* If failed: */
                CMP_IMM         (REG_X0, IMM12(0));
                LDP_POP         (REG_X30, REG_X1);
                LDP_POP         (REG_X0, REG_X1);
                EXCEPTION_IF_EQUAL();
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
        uint32_t len, hash;
        uint64_t field;

        CONSUME_TMPVAR(dst);
        CONSUME_TMPVAR(dict);
        CONSUME_STRING(field_s, len, hash);
        field = (uint64_t)(intptr_t)field_s;

        /* if (!rt_loaddot_helper(env, dst, dict, field, len, hash)) return false; */
        ASM {
                /* x0 = env */
                /* x1 = &env->frame->tmpvar[0] */

                STP_PUSH        (REG_X0, REG_X1);
                STP_PUSH        (REG_X30, REG_XZR);

                /* Arg1 x0: env */

                /* Arg2 x1: dst */
                MOVZ            (REG_X1, IMM16(dst), LSL_0);

                /* Arg3 x2: dict */
                MOVZ            (REG_X2, IMM16(dict), LSL_0);

                /* Arg4 x3: field */
                MOVZ            (REG_X3, IMM16(field & 0xffff), LSL_0);
                MOVK            (REG_X3, IMM16((field >> 16) & 0xffff), LSL_16);
                MOVK            (REG_X3, IMM16((field >> 32) & 0xffff), LSL_32);
                MOVK            (REG_X3, IMM16((field >> 48) & 0xffff), LSL_48);

                /* Arg5 x4: len */
                MOVZ            (REG_X4, IMM16(len & 0xffff), LSL_0);
                MOVK            (REG_X4, IMM16((len >> 16) & 0xffff), LSL_16);

                /* Arg6 x5: hash: */
                MOVZ            (REG_X5, IMM16(hash & 0xffff), LSL_0);
                MOVK            (REG_X5, IMM16((hash >> 16) & 0xffff), LSL_16);

                /* Call ex_loaddot_helper(). */
                MOVZ            (REG_X6, IMM16(((uint64_t)ex_loaddot_helper) & 0xffff), LSL_0);
                MOVK            (REG_X6, IMM16((((uint64_t)ex_loaddot_helper) >> 16) & 0xffff), LSL_16);
                MOVK            (REG_X6, IMM16((((uint64_t)ex_loaddot_helper) >> 32) & 0xffff), LSL_32);
                MOVK            (REG_X6, IMM16((((uint64_t)ex_loaddot_helper) >> 48) & 0xffff), LSL_48);
                BLR             (REG_X6);

                /* If failed: */
                CMP_IMM         (REG_X0, IMM12(0));
                LDP_POP         (REG_X30, REG_X1);
                LDP_POP         (REG_X0, REG_X1);
                EXCEPTION_IF_EQUAL();
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
        uint32_t len, hash;
        uint64_t field;
        int src;

        CONSUME_TMPVAR(dict);
        CONSUME_STRING(field_s, len, hash);
        CONSUME_TMPVAR(src);
        field = (uint64_t)(intptr_t)field_s;

        /* if (!jit_storedot_helper(env, dict, field, len, hash, src)) return false; */
        ASM {
                /* x0 = env */
                /* x1 = &env->frame->tmpvar[0] */

                STP_PUSH        (REG_X0, REG_X1);
                STP_PUSH        (REG_X30, REG_XZR);

                /* Arg1 x0: env */

                /* Arg2 x1: dict */
                MOVZ            (REG_X1, IMM16(dict), LSL_0);

                /* Arg3 x2: field */
                MOVZ            (REG_X2, IMM16(field & 0xffff), LSL_0);
                MOVK            (REG_X2, IMM16((field >> 16) & 0xffff), LSL_16);
                MOVK            (REG_X2, IMM16((field >> 32) & 0xffff), LSL_32);
                MOVK            (REG_X2, IMM16((field >> 48) & 0xffff), LSL_48);

                /* Arg4 x3: len */
                MOVZ            (REG_X3, IMM16(len & 0xffff), LSL_0);
                MOVK            (REG_X3, IMM16((len >> 16) & 0xffff), LSL_16);

                /* Arg5 x4: hash */
                MOVZ            (REG_X4, IMM16(hash & 0xffff), LSL_0);
                MOVK            (REG_X4, IMM16((hash >> 16) & 0xffff), LSL_16);

                /* Arg6 x5: src */
                MOVZ            (REG_X5, IMM16(src), LSL_0);

                /* Call ex_storedot_helper(). */
                MOVZ            (REG_X6, IMM16(((uint64_t)ex_storedot_helper) & 0xffff), LSL_0);
                MOVK            (REG_X6, IMM16((((uint64_t)ex_storedot_helper) >> 16) & 0xffff), LSL_16);
                MOVK            (REG_X6, IMM16((((uint64_t)ex_storedot_helper) >> 32) & 0xffff), LSL_32);
                MOVK            (REG_X6, IMM16((((uint64_t)ex_storedot_helper) >> 48) & 0xffff), LSL_48);
                BLR             (REG_X6);

                /* If failed: */
                CMP_IMM         (REG_X0, IMM12(0));
                LDP_POP         (REG_X30, REG_X1);
                LDP_POP         (REG_X0, REG_X1);
                EXCEPTION_IF_EQUAL();
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
        uint64_t arg_addr;
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
                        BAL             (IMM12(4 + 4 * arg_count));
                }
                arg_addr = (uint64_t)(intptr_t)ctx->code;
                for (i = 0; i < arg_count; i++) {
                        *(uint32_t *)ctx->code = (uint32_t)arg[i];
                        ctx->code = (uint32_t *)ctx->code + 1;
                }
        } else {
                arg_addr = 0;
        }

        /* if (!rt_call_helper(env, dst, func, arg_count, arg)) return false; */
        ASM {
                /* x0 = env */
                /* x1 = &env->frame->tmpvar[0] */

                STP_PUSH        (REG_X0, REG_X1);
                STP_PUSH        (REG_X30, REG_XZR);

                /* Arg1 x0: env */

                /* Arg2 x1: dst */
                MOVZ            (REG_X1, IMM16(dst), LSL_0);

                /* Arg3 x2: func */
                MOVZ            (REG_X2, IMM16(func), LSL_0);

                /* Arg4 x3: arg_count */
                MOVZ            (REG_X3, IMM16(arg_count), LSL_0);

                /* Arg5 x4: arg */
                MOVZ            (REG_X4, IMM16(arg_addr & 0xffff), LSL_0);
                MOVK            (REG_X4, IMM16((arg_addr >> 16) & 0xffff), LSL_16);
                MOVK            (REG_X4, IMM16((arg_addr >> 32) & 0xffff), LSL_32);
                MOVK            (REG_X4, IMM16((arg_addr >> 48) & 0xffff), LSL_48);

                /* Call ex_call_helper(). */
                MOVZ            (REG_X5, IMM16(((uint64_t)ex_call_helper) & 0xffff), LSL_0);
                MOVK            (REG_X5, IMM16((((uint64_t)ex_call_helper) >> 16) & 0xffff), LSL_16);
                MOVK            (REG_X5, IMM16((((uint64_t)ex_call_helper) >> 32) & 0xffff), LSL_32);
                MOVK            (REG_X5, IMM16((((uint64_t)ex_call_helper) >> 48) & 0xffff), LSL_48);
                BLR             (REG_X5);

                /* If failed: */
                CMP_IMM         (REG_X0, IMM12(0));
                LDP_POP         (REG_X30, REG_X1);
                LDP_POP         (REG_X0, REG_X1);
                EXCEPTION_IF_EQUAL();
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
        uint64_t arg_addr;
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

        /* Embed arguments to the code. */
        if (arg_count > 0) {
                ASM {
                        BAL             (IMM12(4 + 4 * arg_count));
                }
                arg_addr = (uint64_t)(intptr_t)ctx->code;
                for (i = 0; i < arg_count; i++) {
                        *(uint32_t *)ctx->code = (uint32_t)arg[i];
                        ctx->code = (uint32_t *)ctx->code + 1;
                }
        } else {
                arg_addr = 0;
        }

        /* if (!rt_thiscall_helper(env, dst, obj, symbol, arg_count, arg)) return false; */
        ASM {
                /* x0 = env */
                /* x1 = &env->frame->tmpvar[0] */

                STP_PUSH        (REG_X0, REG_X1);
                STP_PUSH        (REG_X30, REG_XZR);

                /* Arg1 x0: env */

                /* Arg2 x1: dst */
                MOVZ            (REG_X1, IMM16(dst), LSL_0);

                /* Arg3 x2: obj */
                MOVZ            (REG_X2, IMM16(obj), LSL_0);

                /* Arg4 x3: symbol */
                MOVZ            (REG_X3, IMM16((intptr_t)symbol & 0xffff), LSL_0);
                MOVK            (REG_X3, IMM16(((intptr_t)symbol >> 16) & 0xffff), LSL_16);
                MOVK            (REG_X3, IMM16(((intptr_t)symbol >> 32) & 0xffff), LSL_32);
                MOVK            (REG_X3, IMM16(((intptr_t)symbol >> 48) & 0xffff), LSL_48);

                /* Arg5 x4: len */
                MOVZ            (REG_X4, IMM16(len & 0xffff), LSL_0);
                MOVK            (REG_X4, IMM16((len >> 16) & 0xffff), LSL_16);

                /* Arg6 x5: hash */
                MOVZ            (REG_X5, IMM16(hash & 0xffff), LSL_0);
                MOVK            (REG_X5, IMM16((hash >> 16) & 0xffff), LSL_16);

                /* Arg7 x6: argcount */
                MOVZ            (REG_X6, IMM16(arg_count), LSL_0);

                /* Arg8 x7: arg */
                MOVZ            (REG_X7, IMM16(arg_addr & 0xffff), LSL_0);
                MOVK            (REG_X7, IMM16((arg_addr >> 16) & 0xffff), LSL_16);
                MOVK            (REG_X7, IMM16((arg_addr >> 32) & 0xffff), LSL_32);
                MOVK            (REG_X7, IMM16((arg_addr >> 48) & 0xffff), LSL_48);

                /* Call ex_thiscall_helper(). */
                MOVZ            (REG_X8, IMM16(((uint64_t)ex_thiscall_helper) & 0xffff), LSL_0);
                MOVK            (REG_X8, IMM16((((uint64_t)ex_thiscall_helper) >> 16) & 0xffff), LSL_16);
                MOVK            (REG_X8, IMM16((((uint64_t)ex_thiscall_helper) >> 32) & 0xffff), LSL_32);
                MOVK            (REG_X8, IMM16((((uint64_t)ex_thiscall_helper) >> 48) & 0xffff), LSL_48);
                BLR             (REG_X8);

                /* If failed: */
                CMP_IMM         (REG_X0, IMM12(0));
                LDP_POP         (REG_X30, REG_X1);
                LDP_POP         (REG_X0, REG_X1);
                EXCEPTION_IF_EQUAL();
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
        ctx->branch_patch[ctx->branch_patch_count].type = PATCH_BAL;
        ctx->branch_patch_count++;

        ASM {
                /* Patched later. */
                B               (0);
        }

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
                /* x0 = env */
                /* x1 = &env->frame->tmpvar[0] */

                /* x3 = &env->frame->tmpvar[src].val.i */
                MOVZ            (REG_X2, IMM16(src), LSL_0);
                ADD             (REG_X2, REG_X2, REG_X1);
                LDR_IMM         (REG_X3, REG_X2, IMM9(8));

                /* Compare: env->frame->tmpvar[dst].val.i != 0 */
                CMP_W3_IMM      (IMM12(0));
        }

        /* Patch later. */
        ctx->branch_patch[ctx->branch_patch_count].code = ctx->code;
        ctx->branch_patch[ctx->branch_patch_count].lpc = target_lpc;
        ctx->branch_patch[ctx->branch_patch_count].type = PATCH_BNE;
        ctx->branch_patch_count++;

        ASM {
                /* Patched later. */
                BNE             (IMM19(0));
                if (!jit_put_word(ctx, 0xd503201f)) return false;
        }

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
                /* x0 = env */
                /* x1 = &env->frame->tmpvar[0] */

                /* x3 = &env->frame->tmpvar[src].val.i */
                MOVZ            (REG_X2, IMM16(src), LSL_0);
                ADD             (REG_X2, REG_X2, REG_X1);
                LDR_IMM         (REG_X3, REG_X2, IMM9(8));

                /* Compare: env->frame->tmpvar[dst].val.i == 0 */
                CMP_W3_IMM      (IMM12(0));
        }

        /* Patch later. */
        ctx->branch_patch[ctx->branch_patch_count].code = ctx->code;
        ctx->branch_patch[ctx->branch_patch_count].lpc = target_lpc;
        ctx->branch_patch[ctx->branch_patch_count].type = PATCH_BEQ;
        ctx->branch_patch_count++;

        ASM {
                /* Patched later. */
                BEQ             (IMM19(0));
                if (!jit_put_word(ctx, 0xd503201f)) return false;
        }

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
                /* Patched later. */
                BEQ             (IMM19(0));
                if (!jit_put_word(ctx, 0xd503201f)) return false;
        }

        return true;
}

/* Visit a OP_SAFEPOINT instruction. */
static INLINE bool
jit_visit_safepoint_op(
        struct rt_jit_context *ctx)
{
        /* if (!ex_safepoint_helper(env)) return false; */
        ASM {
                /* x0 = env */

                STP_PUSH        (REG_X0, REG_X1);
                STP_PUSH        (REG_X30, REG_XZR);

                /* Arg1 x0: env */

                /* Call ex_call_helper(). */
                MOVZ            (REG_X5, IMM16(((uint64_t)ex_safepoint_helper) & 0xffff), LSL_0);
                MOVK            (REG_X5, IMM16((((uint64_t)ex_safepoint_helper) >> 16) & 0xffff), LSL_16);
                MOVK            (REG_X5, IMM16((((uint64_t)ex_safepoint_helper) >> 32) & 0xffff), LSL_32);
                MOVK            (REG_X5, IMM16((((uint64_t)ex_safepoint_helper) >> 48) & 0xffff), LSL_48);
                BLR             (REG_X5);

                /* If failed: */
                CMP_IMM         (REG_X0, IMM12(0));
                LDP_POP         (REG_X30, REG_X1);
                LDP_POP         (REG_X0, REG_X1);
                EXCEPTION_IF_EQUAL();
        }
        
        return true;
}

/* Visit a OP_PBASE instruction. (ABCE; inline machine code, arm64.)
 * The guard has proven the operand is a packed. */
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

        dst *= (int)sizeof(struct rt_value);
        src *= (int)sizeof(struct rt_value);
        buf_ofs = (uint32_t)offsetof(struct rt_packed, packed_buffer);

        ASM {
                /* x1 = &env->frame->tmpvar[0] */

                LDR_IMM         (REG_X2, REG_X1, IMM9(src + 8));
                LDR_IMM         (REG_X2, REG_X2, IMM9(buf_ofs));
                MOVZ            (REG_X4, IMM16(NOCT_VALUE_LONG), LSL_0);
                STR_IMM         (REG_X4, REG_X1, IMM9(dst));
                STR_IMM         (REG_X2, REG_X1, IMM9(dst + 8));
        }
	if (base_id == 0) {
		ASM { ADD_IMM(REG_X19, REG_X2, IMM12(0)); }
	} else if (base_id == 1) {
		ASM { ADD_IMM(REG_X20, REG_X2, IMM12(0)); }
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

/* Visit a OP_PLOAD8U instruction. (ABCE; inline machine code, arm64.) */
static INLINE bool
jit_visit_pload8u_op(
        struct rt_jit_context *ctx)
{
        int dst;
        int base;
        int ofs;
#if defined(NOCT_USE_OPTIMIZER)
        uint32_t base_reg;
        uint32_t reg;
        uint32_t cached_reg;
        int cursor;
        int dst_ofs;
        int cached_tmp;
        int opcode_key;
        int32_t byte_disp;
#endif

        CONSUME_TMPVAR(dst);
        CONSUME_TMPVAR(base);
        CONSUME_TMPVAR(ofs);
#if defined(NOCT_USE_OPTIMIZER)
        if (jit_arm64_packed_cursor(ctx, base, ofs, 1,
                                    &base_reg, &cursor, &byte_disp)) {
                rt_jit_ploop_remove_index_alias(ctx, dst);
                rt_jit_ploop_remove_base_alias(ctx, dst);
                if (ctx->gpr_cache_active) {
                        opcode_key = 2;
                        cached_tmp = ctx->gpr_load_tmp[cursor];
                        if (ctx->gpr_reg_limit == 1 && cached_tmp != dst)
                                cached_tmp = -1;
                        if (cached_tmp >= 0 &&
                            ctx->gpr_load_opcode[cursor] == opcode_key &&
                            ctx->gpr_load_disp[cursor] == byte_disp &&
                            ctx->gpr_tmp_reg[cached_tmp] >= 0) {
                                if (!jit_arm64_gpr_get(ctx, cached_tmp, 0,
                                                       &cached_reg))
                                        return false;
                                if (rt_jit_ploop_next_use_lpc(ctx, cached_tmp,
                                                          ctx->lpc) ==
                                    UINT32_MAX) {
                                        if (!jit_arm64_gpr_rebind(ctx, dst,
                                                                  cached_tmp,
                                                                  &reg))
                                                return false;
                                } else if (!jit_arm64_gpr_dest(
                                                   ctx, dst,
                                                   1u << (cached_reg - REG_X23),
                                                   &reg) ||
                                           !jit_arm64_gpr_mov(ctx, reg,
                                                              cached_reg)) {
                                        return false;
                                }
                        } else {
                                if (!jit_arm64_gpr_dest(ctx, dst, 0, &reg) ||
                                    !jit_arm64_put_packed_access_disp(
                                             ctx, false, 1, false, reg,
                                             base_reg, byte_disp))
                                        return false;
                        }
                        ctx->gpr_tmp_dirty[dst] = 1;
                ctx->gpr_range_valid[dst] = 1;
                ctx->gpr_range_min[dst] = 0;
                ctx->gpr_range_max[dst] = 255;
                        ctx->gpr_load_tmp[cursor] = dst;
                        ctx->gpr_load_opcode[cursor] = opcode_key;
                        ctx->gpr_load_disp[cursor] = byte_disp;
                        return true;
                }
                if (!jit_arm64_put_packed_access_disp(ctx, false, 1,
                                                      false, REG_X3,
                                                      base_reg, byte_disp))
                        return false;
                dst_ofs = dst * (int)sizeof(struct rt_value);
                ASM {
                        MOVZ(REG_X4, IMM16(NOCT_VALUE_INT), LSL_0);
                        STR_IMM(REG_X4, REG_X1, IMM9(dst_ofs));
                        STR_IMM(REG_X3, REG_X1, IMM9(dst_ofs + 8));
                }
                return true;
        }
#endif

        dst *= (int)sizeof(struct rt_value);
        base *= (int)sizeof(struct rt_value);
        ofs *= (int)sizeof(struct rt_value);

        ASM {
                /* x1 = &env->frame->tmpvar[0] */

                /* x2 = base pointer */
                LDR_IMM         (REG_X2, REG_X1, IMM9(base + 8));
                /* x3 = element index */
                LDR_W_IMM       (REG_X3, REG_X1, (uint32_t)(ofs + 8));
                ADD             (REG_X2, REG_X2, REG_X3);
                /* x3 = element */
                LDRB_R         (REG_X3, REG_X2);
                /* tag */
                MOVZ            (REG_X4, IMM16(NOCT_VALUE_INT), LSL_0);
                STR_IMM         (REG_X4, REG_X1, IMM9(dst));
                /* value (full 8-byte union write; LE) */
                STR_IMM         (REG_X3, REG_X1, IMM9(dst + 8));
        }

        return true;
}

/* Visit a OP_PSTORE8 instruction. (ABCE; inline, arm64. Int source per ABCE rules.) */
static INLINE bool
jit_visit_pstore8_op(
        struct rt_jit_context *ctx)
{
        int base;
        int ofs;
        int src;
#if defined(NOCT_USE_OPTIMIZER)
        uint32_t base_reg;
        uint32_t reg;
        int cursor;
        int src_ofs;
        int32_t byte_disp;
#endif

        CONSUME_TMPVAR(base);
        CONSUME_TMPVAR(ofs);
        CONSUME_TMPVAR(src);
#if defined(NOCT_USE_OPTIMIZER)
        if (jit_arm64_packed_cursor(ctx, base, ofs, 1,
                                    &base_reg, &cursor, &byte_disp)) {
                if (ctx->gpr_cache_active) {
                        if (!jit_arm64_gpr_get(ctx, src, 0, &reg) ||
                            !jit_arm64_put_packed_access_disp(
                                     ctx, true, 1, false, reg, base_reg,
                                     byte_disp))
                                return false;
                        ctx->gpr_load_tmp[cursor] = -1;
                        return true;
                }
                src_ofs = src * (int)sizeof(struct rt_value);
                ASM {
                        LDR_W_IMM(REG_X4, REG_X1,
                                  (uint32_t)(src_ofs + 8));
                }
                return jit_arm64_put_packed_access_disp(
                                ctx, true, 1, false, REG_X4, base_reg,
                                byte_disp);
        }
#endif

        base *= (int)sizeof(struct rt_value);
        ofs *= (int)sizeof(struct rt_value);
        src *= (int)sizeof(struct rt_value);

        ASM {
                /* x1 = &env->frame->tmpvar[0] */

                LDR_IMM         (REG_X2, REG_X1, IMM9(base + 8));
                LDR_W_IMM       (REG_X3, REG_X1, (uint32_t)(ofs + 8));
                ADD             (REG_X2, REG_X2, REG_X3);
                LDR_W_IMM       (REG_X4, REG_X1, (uint32_t)(src + 8));
                STRB_R          (REG_X4, REG_X2);
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
#if defined(NOCT_USE_OPTIMIZER)
	int flags;
#endif

        CONSUME_TMPVAR(dst);
        CONSUME_IMM8(src);

	/* if (!ex_checktype_helper(env, slot, type)) return false; */
	ASM_UNARY_OP(ex_checktype_helper);
#if defined(NOCT_USE_OPTIMIZER)
	flags = src & (TYPECHECK_RETURN_FLAG | TYPECHECK_LOCAL_FLAG);
	src &= ~(TYPECHECK_RETURN_FLAG | TYPECHECK_LOCAL_FLAG);
	if (ctx->tmp_fixed_type != NULL &&
	    flags == 0 && ctx->tmp_fixed_type[dst] == src)
		ctx->tmp_frame_tag_known[dst] = 1;
#endif

	return true;
}

/* Publish a fixed primitive tag at a dynamic observation boundary. */
static INLINE bool
jit_visit_arm64_materialize_type_op(struct rt_jit_context *ctx)
{
	int tmp;
	int type;
	int ofs;

	CONSUME_TMPVAR(tmp);
	CONSUME_IMM8(type);
	if (type != NOCT_VALUE_INT && type != NOCT_VALUE_LONG &&
	    type != NOCT_VALUE_FLOAT && type != NOCT_VALUE_DOUBLE) {
		rt_error(ctx->env, BROKEN_BYTECODE);
		return false;
	}
#if defined(NOCT_USE_OPTIMIZER)
	if (ctx->tmp_frame_tag_known != NULL &&
	    ctx->tmp_frame_tag_known[tmp])
		return true;
#endif
	ofs = tmp * (int)sizeof(struct rt_value);
	ASM {
		MOVZ(REG_X2, IMM16(type), LSL_0);
		MOVZ(REG_X3, IMM16(ofs), LSL_0);
		ADD(REG_X3, REG_X3, REG_X1);
		STR(REG_X2, REG_X3);
	}
	return true;
}

/* Visit a OP_PLOAD8S instruction. (ABCE; inline machine code, arm64.) */
static INLINE bool
jit_visit_pload8s_op(
        struct rt_jit_context *ctx)
{
        int dst;
        int base;
        int ofs;
#if defined(NOCT_USE_OPTIMIZER)
        uint32_t base_reg;
        uint32_t reg;
        uint32_t cached_reg;
        int cursor;
        int dst_ofs;
        int cached_tmp;
        int opcode_key;
        int32_t byte_disp;
#endif

        CONSUME_TMPVAR(dst);
        CONSUME_TMPVAR(base);
        CONSUME_TMPVAR(ofs);
#if defined(NOCT_USE_OPTIMIZER)
        if (jit_arm64_packed_cursor(ctx, base, ofs, 1,
                                    &base_reg, &cursor, &byte_disp)) {
                rt_jit_ploop_remove_index_alias(ctx, dst);
                rt_jit_ploop_remove_base_alias(ctx, dst);
                if (ctx->gpr_cache_active) {
                        opcode_key = 3;
                        cached_tmp = ctx->gpr_load_tmp[cursor];
                        if (ctx->gpr_reg_limit == 1 && cached_tmp != dst)
                                cached_tmp = -1;
                        if (cached_tmp >= 0 &&
                            ctx->gpr_load_opcode[cursor] == opcode_key &&
                            ctx->gpr_load_disp[cursor] == byte_disp &&
                            ctx->gpr_tmp_reg[cached_tmp] >= 0) {
                                if (!jit_arm64_gpr_get(ctx, cached_tmp, 0,
                                                       &cached_reg))
                                        return false;
                                if (rt_jit_ploop_next_use_lpc(ctx, cached_tmp,
                                                          ctx->lpc) ==
                                    UINT32_MAX) {
                                        if (!jit_arm64_gpr_rebind(ctx, dst,
                                                                  cached_tmp,
                                                                  &reg))
                                                return false;
                                } else if (!jit_arm64_gpr_dest(
                                                   ctx, dst,
                                                   1u << (cached_reg - REG_X23),
                                                   &reg) ||
                                           !jit_arm64_gpr_mov(ctx, reg,
                                                              cached_reg)) {
                                        return false;
                                }
                        } else {
                                if (!jit_arm64_gpr_dest(ctx, dst, 0, &reg) ||
                                    !jit_arm64_put_packed_access_disp(
                                             ctx, false, 1, true, reg,
                                             base_reg, byte_disp))
                                        return false;
                        }
                        ctx->gpr_tmp_dirty[dst] = 1;
                ctx->gpr_range_valid[dst] = 1;
                ctx->gpr_range_min[dst] = -128;
                ctx->gpr_range_max[dst] = 127;
                        ctx->gpr_load_tmp[cursor] = dst;
                        ctx->gpr_load_opcode[cursor] = opcode_key;
                        ctx->gpr_load_disp[cursor] = byte_disp;
                        return true;
                }
                if (!jit_arm64_put_packed_access_disp(ctx, false, 1,
                                                      true, REG_X3,
                                                      base_reg, byte_disp))
                        return false;
                dst_ofs = dst * (int)sizeof(struct rt_value);
                ASM {
                        MOVZ(REG_X4, IMM16(NOCT_VALUE_INT), LSL_0);
                        STR_IMM(REG_X4, REG_X1, IMM9(dst_ofs));
                        STR_IMM(REG_X3, REG_X1, IMM9(dst_ofs + 8));
                }
                return true;
        }
#endif

        dst *= (int)sizeof(struct rt_value);
        base *= (int)sizeof(struct rt_value);
        ofs *= (int)sizeof(struct rt_value);

        ASM {
                /* x1 = &env->frame->tmpvar[0] */

                /* x2 = base pointer */
                LDR_IMM         (REG_X2, REG_X1, IMM9(base + 8));
                /* x3 = element index */
                LDR_W_IMM       (REG_X3, REG_X1, (uint32_t)(ofs + 8));
                ADD             (REG_X2, REG_X2, REG_X3);
                /* x3 = element */
                LDRSB_R         (REG_X3, REG_X2);
                /* tag */
                MOVZ            (REG_X4, IMM16(NOCT_VALUE_INT), LSL_0);
                STR_IMM         (REG_X4, REG_X1, IMM9(dst));
                /* value (full 8-byte union write; LE) */
                STR_IMM         (REG_X3, REG_X1, IMM9(dst + 8));
        }

        return true;
}

/* Visit a OP_PLOAD16U instruction. (ABCE; inline machine code, arm64.) */
static INLINE bool
jit_visit_pload16u_op(
        struct rt_jit_context *ctx)
{
        int dst;
        int base;
        int ofs;
#if defined(NOCT_USE_OPTIMIZER)
        uint32_t base_reg;
        uint32_t reg;
        uint32_t cached_reg;
        int cursor;
        int dst_ofs;
        int cached_tmp;
        int opcode_key;
        int32_t byte_disp;
#endif

        CONSUME_TMPVAR(dst);
        CONSUME_TMPVAR(base);
        CONSUME_TMPVAR(ofs);
#if defined(NOCT_USE_OPTIMIZER)
        if (jit_arm64_packed_cursor(ctx, base, ofs, 2,
                                    &base_reg, &cursor, &byte_disp)) {
                rt_jit_ploop_remove_index_alias(ctx, dst);
                rt_jit_ploop_remove_base_alias(ctx, dst);
                if (ctx->gpr_cache_active) {
                        opcode_key = 4;
                        cached_tmp = ctx->gpr_load_tmp[cursor];
                        if (ctx->gpr_reg_limit == 1 && cached_tmp != dst)
                                cached_tmp = -1;
                        if (cached_tmp >= 0 &&
                            ctx->gpr_load_opcode[cursor] == opcode_key &&
                            ctx->gpr_load_disp[cursor] == byte_disp &&
                            ctx->gpr_tmp_reg[cached_tmp] >= 0) {
                                if (!jit_arm64_gpr_get(ctx, cached_tmp, 0,
                                                       &cached_reg))
                                        return false;
                                if (rt_jit_ploop_next_use_lpc(ctx, cached_tmp,
                                                          ctx->lpc) ==
                                    UINT32_MAX) {
                                        if (!jit_arm64_gpr_rebind(ctx, dst,
                                                                  cached_tmp,
                                                                  &reg))
                                                return false;
                                } else if (!jit_arm64_gpr_dest(
                                                   ctx, dst,
                                                   1u << (cached_reg - REG_X23),
                                                   &reg) ||
                                           !jit_arm64_gpr_mov(ctx, reg,
                                                              cached_reg)) {
                                        return false;
                                }
                        } else {
                                if (!jit_arm64_gpr_dest(ctx, dst, 0, &reg) ||
                                    !jit_arm64_put_packed_access_disp(
                                             ctx, false, 2, false, reg,
                                             base_reg, byte_disp))
                                        return false;
                        }
                        ctx->gpr_tmp_dirty[dst] = 1;
                ctx->gpr_range_valid[dst] = 1;
                ctx->gpr_range_min[dst] = 0;
                ctx->gpr_range_max[dst] = 65535;
                        ctx->gpr_load_tmp[cursor] = dst;
                        ctx->gpr_load_opcode[cursor] = opcode_key;
                        ctx->gpr_load_disp[cursor] = byte_disp;
                        return true;
                }
                if (!jit_arm64_put_packed_access_disp(ctx, false, 2,
                                                      false, REG_X3,
                                                      base_reg, byte_disp))
                        return false;
                dst_ofs = dst * (int)sizeof(struct rt_value);
                ASM {
                        MOVZ(REG_X4, IMM16(NOCT_VALUE_INT), LSL_0);
                        STR_IMM(REG_X4, REG_X1, IMM9(dst_ofs));
                        STR_IMM(REG_X3, REG_X1, IMM9(dst_ofs + 8));
                }
                return true;
        }
#endif

        dst *= (int)sizeof(struct rt_value);
        base *= (int)sizeof(struct rt_value);
        ofs *= (int)sizeof(struct rt_value);

        ASM {
                /* x1 = &env->frame->tmpvar[0] */

                /* x2 = base pointer */
                LDR_IMM         (REG_X2, REG_X1, IMM9(base + 8));
                /* x3 = element index */
                LDR_W_IMM       (REG_X3, REG_X1, (uint32_t)(ofs + 8));
                LSL_IMM         (REG_X3, REG_X3, 1);
                ADD             (REG_X2, REG_X2, REG_X3);
                /* x3 = element */
                LDRH_R         (REG_X3, REG_X2);
                /* tag */
                MOVZ            (REG_X4, IMM16(NOCT_VALUE_INT), LSL_0);
                STR_IMM         (REG_X4, REG_X1, IMM9(dst));
                /* value (full 8-byte union write; LE) */
                STR_IMM         (REG_X3, REG_X1, IMM9(dst + 8));
        }

        return true;
}

/* Visit a OP_PLOAD16S instruction. (ABCE; inline machine code, arm64.) */
static INLINE bool
jit_visit_pload16s_op(
        struct rt_jit_context *ctx)
{
        int dst;
        int base;
        int ofs;
#if defined(NOCT_USE_OPTIMIZER)
        uint32_t base_reg;
        uint32_t reg;
        uint32_t cached_reg;
        int cursor;
        int dst_ofs;
        int cached_tmp;
        int opcode_key;
        int32_t byte_disp;
#endif

        CONSUME_TMPVAR(dst);
        CONSUME_TMPVAR(base);
        CONSUME_TMPVAR(ofs);
#if defined(NOCT_USE_OPTIMIZER)
        if (jit_arm64_packed_cursor(ctx, base, ofs, 2,
                                    &base_reg, &cursor, &byte_disp)) {
                rt_jit_ploop_remove_index_alias(ctx, dst);
                rt_jit_ploop_remove_base_alias(ctx, dst);
                if (ctx->gpr_cache_active) {
                        opcode_key = 5;
                        cached_tmp = ctx->gpr_load_tmp[cursor];
                        if (ctx->gpr_reg_limit == 1 && cached_tmp != dst)
                                cached_tmp = -1;
                        if (cached_tmp >= 0 &&
                            ctx->gpr_load_opcode[cursor] == opcode_key &&
                            ctx->gpr_load_disp[cursor] == byte_disp &&
                            ctx->gpr_tmp_reg[cached_tmp] >= 0) {
                                if (!jit_arm64_gpr_get(ctx, cached_tmp, 0,
                                                       &cached_reg))
                                        return false;
                                if (rt_jit_ploop_next_use_lpc(ctx, cached_tmp,
                                                          ctx->lpc) ==
                                    UINT32_MAX) {
                                        if (!jit_arm64_gpr_rebind(ctx, dst,
                                                                  cached_tmp,
                                                                  &reg))
                                                return false;
                                } else if (!jit_arm64_gpr_dest(
                                                   ctx, dst,
                                                   1u << (cached_reg - REG_X23),
                                                   &reg) ||
                                           !jit_arm64_gpr_mov(ctx, reg,
                                                              cached_reg)) {
                                        return false;
                                }
                        } else {
                                if (!jit_arm64_gpr_dest(ctx, dst, 0, &reg) ||
                                    !jit_arm64_put_packed_access_disp(
                                             ctx, false, 2, true, reg,
                                             base_reg, byte_disp))
                                        return false;
                        }
                        ctx->gpr_tmp_dirty[dst] = 1;
                ctx->gpr_range_valid[dst] = 1;
                ctx->gpr_range_min[dst] = -32768;
                ctx->gpr_range_max[dst] = 32767;
                        ctx->gpr_load_tmp[cursor] = dst;
                        ctx->gpr_load_opcode[cursor] = opcode_key;
                        ctx->gpr_load_disp[cursor] = byte_disp;
                        return true;
                }
                if (!jit_arm64_put_packed_access_disp(ctx, false, 2,
                                                      true, REG_X3,
                                                      base_reg, byte_disp))
                        return false;
                dst_ofs = dst * (int)sizeof(struct rt_value);
                ASM {
                        MOVZ(REG_X4, IMM16(NOCT_VALUE_INT), LSL_0);
                        STR_IMM(REG_X4, REG_X1, IMM9(dst_ofs));
                        STR_IMM(REG_X3, REG_X1, IMM9(dst_ofs + 8));
                }
                return true;
        }
#endif

        dst *= (int)sizeof(struct rt_value);
        base *= (int)sizeof(struct rt_value);
        ofs *= (int)sizeof(struct rt_value);

        ASM {
                /* x1 = &env->frame->tmpvar[0] */

                /* x2 = base pointer */
                LDR_IMM         (REG_X2, REG_X1, IMM9(base + 8));
                /* x3 = element index */
                LDR_W_IMM       (REG_X3, REG_X1, (uint32_t)(ofs + 8));
                LSL_IMM         (REG_X3, REG_X3, 1);
                ADD             (REG_X2, REG_X2, REG_X3);
                /* x3 = element */
                LDRSH_R         (REG_X3, REG_X2);
                /* tag */
                MOVZ            (REG_X4, IMM16(NOCT_VALUE_INT), LSL_0);
                STR_IMM         (REG_X4, REG_X1, IMM9(dst));
                /* value (full 8-byte union write; LE) */
                STR_IMM         (REG_X3, REG_X1, IMM9(dst + 8));
        }

        return true;
}

/* Visit a OP_PLOAD32 instruction. (ABCE; inline machine code, arm64.) */
static INLINE bool
jit_visit_pload32_op(
        struct rt_jit_context *ctx)
{
        int dst;
        int base;
        int ofs;
#if defined(NOCT_USE_OPTIMIZER)
        uint32_t base_reg;
        uint32_t reg;
        uint32_t cached_reg;
        int cursor;
        int dst_ofs;
        int cached_tmp;
        int opcode_key;
        int32_t byte_disp;
#endif

        CONSUME_TMPVAR(dst);
        CONSUME_TMPVAR(base);
        CONSUME_TMPVAR(ofs);
#if defined(NOCT_USE_OPTIMIZER)
        if (jit_arm64_packed_cursor(ctx, base, ofs, 4,
                                    &base_reg, &cursor, &byte_disp)) {
                rt_jit_ploop_remove_index_alias(ctx, dst);
                rt_jit_ploop_remove_base_alias(ctx, dst);
                if (ctx->gpr_cache_active) {
                        opcode_key = 8;
                        cached_tmp = ctx->gpr_load_tmp[cursor];
                        if (ctx->gpr_reg_limit == 1 && cached_tmp != dst)
                                cached_tmp = -1;
                        if (cached_tmp >= 0 &&
                            ctx->gpr_load_opcode[cursor] == opcode_key &&
                            ctx->gpr_load_disp[cursor] == byte_disp &&
                            ctx->gpr_tmp_reg[cached_tmp] >= 0) {
                                if (!jit_arm64_gpr_get(ctx, cached_tmp, 0,
                                                       &cached_reg))
                                        return false;
                                if (rt_jit_ploop_next_use_lpc(ctx, cached_tmp,
                                                          ctx->lpc) ==
                                    UINT32_MAX) {
                                        if (!jit_arm64_gpr_rebind(ctx, dst,
                                                                  cached_tmp,
                                                                  &reg))
                                                return false;
                                } else if (!jit_arm64_gpr_dest(
                                                   ctx, dst,
                                                   1u << (cached_reg - REG_X23),
                                                   &reg) ||
                                           !jit_arm64_gpr_mov(ctx, reg,
                                                              cached_reg)) {
                                        return false;
                                }
                        } else {
                                if (!jit_arm64_gpr_dest(ctx, dst, 0, &reg) ||
                                    !jit_arm64_put_packed_access_disp(
                                             ctx, false, 4, false, reg,
                                             base_reg, byte_disp))
                                        return false;
                        }
                        ctx->gpr_tmp_dirty[dst] = 1;
                ctx->gpr_range_valid[dst] = 0;
                        ctx->gpr_load_tmp[cursor] = dst;
                        ctx->gpr_load_opcode[cursor] = opcode_key;
                        ctx->gpr_load_disp[cursor] = byte_disp;
                        return true;
                }
                if (!jit_arm64_put_packed_access_disp(ctx, false, 4,
                                                      false, REG_X3,
                                                      base_reg, byte_disp))
                        return false;
                dst_ofs = dst * (int)sizeof(struct rt_value);
                ASM {
                        MOVZ(REG_X4, IMM16(NOCT_VALUE_INT), LSL_0);
                        STR_IMM(REG_X4, REG_X1, IMM9(dst_ofs));
                        STR_IMM(REG_X3, REG_X1, IMM9(dst_ofs + 8));
                }
                return true;
        }
#endif

        dst *= (int)sizeof(struct rt_value);
        base *= (int)sizeof(struct rt_value);
        ofs *= (int)sizeof(struct rt_value);

        ASM {
                /* x1 = &env->frame->tmpvar[0] */

                /* x2 = base pointer */
                LDR_IMM         (REG_X2, REG_X1, IMM9(base + 8));
                /* x3 = element index */
                LDR_W_IMM       (REG_X3, REG_X1, (uint32_t)(ofs + 8));
                LSL_IMM         (REG_X3, REG_X3, 2);
                ADD             (REG_X2, REG_X2, REG_X3);
                /* x3 = element */
                LDRW_R         (REG_X3, REG_X2);
                /* tag */
                MOVZ            (REG_X4, IMM16(NOCT_VALUE_INT), LSL_0);
                STR_IMM         (REG_X4, REG_X1, IMM9(dst));
                /* value (full 8-byte union write; LE) */
                STR_IMM         (REG_X3, REG_X1, IMM9(dst + 8));
        }

        return true;
}

/* Visit a OP_PLOAD64 instruction. (ABCE; inline machine code, arm64.) */
static INLINE bool
jit_visit_pload64_op(
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
                /* x1 = &env->frame->tmpvar[0] */

                /* x2 = base pointer */
                LDR_IMM         (REG_X2, REG_X1, IMM9(base + 8));
                /* x3 = element index */
                LDR_W_IMM       (REG_X3, REG_X1, (uint32_t)(ofs + 8));
                LSL_IMM         (REG_X3, REG_X3, 3);
                ADD             (REG_X2, REG_X2, REG_X3);
                /* x3 = element */
                LDRX_R         (REG_X3, REG_X2);
                /* tag */
                MOVZ            (REG_X4, IMM16(NOCT_VALUE_LONG), LSL_0);
                STR_IMM         (REG_X4, REG_X1, IMM9(dst));
                /* value (full 8-byte union write; LE) */
                STR_IMM         (REG_X3, REG_X1, IMM9(dst + 8));
        }

        return true;
}

/* Visit a OP_PSTORE16 instruction. (ABCE; inline, arm64. Int source per ABCE rules.) */
static INLINE bool
jit_visit_pstore16_op(
        struct rt_jit_context *ctx)
{
        int base;
        int ofs;
        int src;
#if defined(NOCT_USE_OPTIMIZER)
        uint32_t base_reg;
        uint32_t reg;
        int cursor;
        int src_ofs;
        int32_t byte_disp;
#endif

        CONSUME_TMPVAR(base);
        CONSUME_TMPVAR(ofs);
        CONSUME_TMPVAR(src);
#if defined(NOCT_USE_OPTIMIZER)
        if (jit_arm64_packed_cursor(ctx, base, ofs, 2,
                                    &base_reg, &cursor, &byte_disp)) {
                if (ctx->gpr_cache_active) {
                        if (!jit_arm64_gpr_get(ctx, src, 0, &reg) ||
                            !jit_arm64_put_packed_access_disp(
                                     ctx, true, 2, false, reg, base_reg,
                                     byte_disp))
                                return false;
                        ctx->gpr_load_tmp[cursor] = -1;
                        return true;
                }
                src_ofs = src * (int)sizeof(struct rt_value);
                ASM {
                        LDR_W_IMM(REG_X4, REG_X1,
                                  (uint32_t)(src_ofs + 8));
                }
                return jit_arm64_put_packed_access_disp(
                                ctx, true, 2, false, REG_X4, base_reg,
                                byte_disp);
        }
#endif

        base *= (int)sizeof(struct rt_value);
        ofs *= (int)sizeof(struct rt_value);
        src *= (int)sizeof(struct rt_value);

        ASM {
                /* x1 = &env->frame->tmpvar[0] */

                LDR_IMM         (REG_X2, REG_X1, IMM9(base + 8));
                LDR_W_IMM       (REG_X3, REG_X1, (uint32_t)(ofs + 8));
                LSL_IMM         (REG_X3, REG_X3, 1);
                ADD             (REG_X2, REG_X2, REG_X3);
                LDR_W_IMM       (REG_X4, REG_X1, (uint32_t)(src + 8));
                STRH_R          (REG_X4, REG_X2);
        }

        return true;
}

/* Visit a OP_PSTORE32 instruction. (ABCE; inline, arm64. Int source per ABCE rules.) */
static INLINE bool
jit_visit_pstore32_op(
        struct rt_jit_context *ctx)
{
        int base;
        int ofs;
        int src;
#if defined(NOCT_USE_OPTIMIZER)
        uint32_t base_reg;
        uint32_t reg;
        int cursor;
        int src_ofs;
        int32_t byte_disp;
#endif

        CONSUME_TMPVAR(base);
        CONSUME_TMPVAR(ofs);
        CONSUME_TMPVAR(src);
#if defined(NOCT_USE_OPTIMIZER)
        if (jit_arm64_packed_cursor(ctx, base, ofs, 4,
                                    &base_reg, &cursor, &byte_disp)) {
                if (ctx->gpr_cache_active) {
                        if (!jit_arm64_gpr_get(ctx, src, 0, &reg) ||
                            !jit_arm64_put_packed_access_disp(
                                     ctx, true, 4, false, reg, base_reg,
                                     byte_disp))
                                return false;
                        ctx->gpr_load_tmp[cursor] = -1;
                        return true;
                }
                src_ofs = src * (int)sizeof(struct rt_value);
                ASM {
                        LDR_W_IMM(REG_X4, REG_X1,
                                  (uint32_t)(src_ofs + 8));
                }
                return jit_arm64_put_packed_access_disp(
                                ctx, true, 4, false, REG_X4, base_reg,
                                byte_disp);
        }
#endif

        base *= (int)sizeof(struct rt_value);
        ofs *= (int)sizeof(struct rt_value);
        src *= (int)sizeof(struct rt_value);

        ASM {
                /* x1 = &env->frame->tmpvar[0] */

                LDR_IMM         (REG_X2, REG_X1, IMM9(base + 8));
                LDR_W_IMM       (REG_X3, REG_X1, (uint32_t)(ofs + 8));
                LSL_IMM         (REG_X3, REG_X3, 2);
                ADD             (REG_X2, REG_X2, REG_X3);
                LDR_W_IMM       (REG_X4, REG_X1, (uint32_t)(src + 8));
                STRW_R          (REG_X4, REG_X2);
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


/*
 * Typed arithmetic ops (docs/design/07-typed-ops.md): inline machine
 * code, arm64.  Every op trusts the operand tags (proven by the LIR
 * layer); w2/w3/w4 and s0/s1 are per-op scratch (v0/v1 are
 * caller-saved under AAPCS64 and unused by the other emitters).
 *
 * Float comparisons: after FCMP the unordered (NaN) result sets C
 * and V, so the signed-int conditions LT/LE would read as true.  The
 * NaN-safe condition set is MI (<), LS (<=), GT (>), GE (>=) -- do
 * not "simplify" MI/LS to LT/LE (07-typed-ops.md par.6).
 */

/* AArch64 condition codes (for CSET). */
#define TYPED_COND_MI   4
#define TYPED_COND_LS   9
#define TYPED_COND_GE   10
#define TYPED_COND_LT   11
#define TYPED_COND_GT   12
#define TYPED_COND_LE   13

/* cset wRd, cond == csinc wRd, wzr, wzr, !cond */
#define TYPED_CSET_W(rd, cond)                                          \
        if (!jit_put_word(ctx, 0x1a9f07e0 | (((uint32_t)(cond) ^ 1u) << 12) | (rd))) \
                return false

/* Store an int/float result: tag then the 8-byte union slot.  The
   value register was produced by a 32-bit op (zero-extended), so the
   64-bit store writes value bits + zero padding, the same full-union
   write the PLOAD32 emitter does. */
#define TYPED_STORE(vreg, dstofs, tag, write_tag)                       \
        if (write_tag) {                                                \
        MOVZ(REG_X4, IMM16(tag), LSL_0);                                \
        STR_IMM(REG_X4, REG_X1, IMM9(dstofs)); }                        \
        STR_IMM(vreg, REG_X1, IMM9((dstofs) + 8))

static INLINE void
jit_arm64_patch_local_branch(uint8_t *code, uint8_t *target, bool cbz_w3)
{
	intptr_t words;
	uint32_t insn;

	words = (target - code) / 4;
	if (cbz_w3)
		insn = 0x34000003u | (((uint32_t)words & 0x7ffffu) << 5);
	else
		insn = 0x14000000u | ((uint32_t)words & 0x03ffffffu);
	memcpy(code, &insn, sizeof(insn));
}

/* Visit an OP_IADD instruction. */
static INLINE bool
jit_visit_iadd_op(
        struct rt_jit_context *ctx)
{
        int dst;
        int src1;
        int src2;
        int dst_tmp;
        int result_type;
        bool write_tag;

        CONSUME_TMPVAR(dst);
        CONSUME_TMPVAR(src1);
        CONSUME_TMPVAR(src2);

#if defined(NOCT_USE_OPTIMIZER)
        if (rt_jit_ploop_current_elided(ctx, 7))
                return true;
        if (ctx->packed_loop_hint_active) {
                rt_jit_ploop_remove_index_alias(ctx, dst);
                rt_jit_ploop_remove_base_alias(ctx, dst);
        }
        if (ctx->gpr_cache_active && ctx->gpr_reg_limit != 1) {
                uint32_t r1;
                uint32_t r2;
                uint32_t rd;
                unsigned pins;
                bool v1;
                bool v2;
                int32_t min1;
                int32_t max1;
                int32_t min2;
                int32_t max2;
                int64_t lo;
                int64_t hi;

                v1 = ctx->gpr_range_valid[src1] != 0;
                min1 = v1 ? ctx->gpr_range_min[src1] : 0;
                max1 = v1 ? ctx->gpr_range_max[src1] : 0;
                v2 = ctx->gpr_range_valid[src2] != 0;
                min2 = v2 ? ctx->gpr_range_min[src2] : 0;
                max2 = v2 ? ctx->gpr_range_max[src2] : 0;
                if (!jit_arm64_gpr_get(ctx, src1, 0, &r1))
                        return false;
                pins = 1u << (r1 - REG_X23);
                if (!jit_arm64_gpr_get(ctx, src2, pins, &r2))
                        return false;
                pins |= 1u << (r2 - REG_X23);
                if (!jit_arm64_gpr_is_cached(ctx, src1) &&
                    rt_jit_ploop_next_use_lpc(ctx, src1, ctx->lpc) ==
                    UINT32_MAX) {
                        if (!jit_arm64_gpr_rebind(ctx, dst, src1, &rd))
                                return false;
                } else if (!jit_arm64_gpr_is_cached(ctx, src2) &&
                           rt_jit_ploop_next_use_lpc(ctx, src2, ctx->lpc) ==
                           UINT32_MAX) {
                        if (!jit_arm64_gpr_rebind(ctx, dst, src2, &rd))
                                return false;
                } else if (!jit_arm64_gpr_dest(ctx, dst, pins, &rd)) {
                        return false;
                }
                if (!jit_put_word(ctx, 0x0b000000u | (r2 << 16) |
                                         (r1 << 5) | rd))
                        return false;
                ctx->gpr_tmp_dirty[dst] = 1;
                ctx->gpr_range_valid[dst] = 0;
                if (v1 && v2) {
                        lo = (int64_t)min1 + min2;
                        hi = (int64_t)max1 + max2;
                        if (lo >= INT32_MIN && hi <= INT32_MAX) {
                                ctx->gpr_range_valid[dst] = 1;
                                ctx->gpr_range_min[dst] = (int32_t)lo;
                                ctx->gpr_range_max[dst] = (int32_t)hi;
                        }
                }
                jit_arm64_invalidate_packed_load(ctx, dst);
                return true;
        }
        if (ctx->gpr_cache_active && !jit_arm64_gpr_flush_required(ctx))
                return false;

#endif
        dst_tmp = dst;
        result_type = NOCT_VALUE_INT;
        write_tag = !rt_jit_tmp_has_fixed_primitive_type(ctx, dst_tmp,
                                                     result_type);

        dst *= (int)sizeof(struct rt_value);
        src1 *= (int)sizeof(struct rt_value);
        src2 *= (int)sizeof(struct rt_value);

        ASM {
                /* x1 = &env->frame->tmpvar[0] */
                /* w2 = src1 value, w3 = src2 value */
                LDR_W_IMM       (REG_X2, REG_X1, (uint32_t)(src1 + 8));
                LDR_W_IMM       (REG_X3, REG_X1, (uint32_t)(src2 + 8));
        }
        /* add w2, w2, w3 */
        if (!jit_put_word(ctx, 0x0b000000 | (3u << 16) | (2u << 5) | 2u))
                return false;
        ASM {
                TYPED_STORE(REG_X2, dst, NOCT_VALUE_INT, write_tag);
        }
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
        int dst_tmp;
        int result_type;
        bool write_tag;

        CONSUME_TMPVAR(dst);
        CONSUME_TMPVAR(src1);
        CONSUME_TMPVAR(src2);

#if defined(NOCT_USE_OPTIMIZER)
        if (rt_jit_ploop_current_elided(ctx, 7))
                return true;
        if (ctx->packed_loop_hint_active) {
                rt_jit_ploop_remove_index_alias(ctx, dst);
                rt_jit_ploop_remove_base_alias(ctx, dst);
        }
        if (ctx->gpr_cache_active && ctx->gpr_reg_limit != 1) {
                uint32_t r1;
                uint32_t r2;
                uint32_t rd;
                unsigned pins;
                bool v1;
                bool v2;
                int32_t min1;
                int32_t max1;
                int32_t min2;
                int32_t max2;
                int64_t lo;
                int64_t hi;

                v1 = ctx->gpr_range_valid[src1] != 0;
                min1 = v1 ? ctx->gpr_range_min[src1] : 0;
                max1 = v1 ? ctx->gpr_range_max[src1] : 0;
                v2 = ctx->gpr_range_valid[src2] != 0;
                min2 = v2 ? ctx->gpr_range_min[src2] : 0;
                max2 = v2 ? ctx->gpr_range_max[src2] : 0;
                if (!jit_arm64_gpr_get(ctx, src1, 0, &r1))
                        return false;
                pins = 1u << (r1 - REG_X23);
                if (!jit_arm64_gpr_get(ctx, src2, pins, &r2))
                        return false;
                pins |= 1u << (r2 - REG_X23);
                if (!jit_arm64_gpr_is_cached(ctx, src1) &&
                    rt_jit_ploop_next_use_lpc(ctx, src1, ctx->lpc) ==
                    UINT32_MAX) {
                        if (!jit_arm64_gpr_rebind(ctx, dst, src1, &rd))
                                return false;
                } else if (!jit_arm64_gpr_is_cached(ctx, src2) &&
                           rt_jit_ploop_next_use_lpc(ctx, src2, ctx->lpc) ==
                           UINT32_MAX) {
                        if (!jit_arm64_gpr_rebind(ctx, dst, src2, &rd))
                                return false;
                } else if (!jit_arm64_gpr_dest(ctx, dst, pins, &rd)) {
                        return false;
                }
                if (!jit_put_word(ctx, 0x4b000000u | (r2 << 16) |
                                         (r1 << 5) | rd))
                        return false;
                ctx->gpr_tmp_dirty[dst] = 1;
                ctx->gpr_range_valid[dst] = 0;
                if (v1 && v2) {
                        lo = (int64_t)min1 - max2;
                        hi = (int64_t)max1 - min2;
                        if (lo >= INT32_MIN && hi <= INT32_MAX) {
                                ctx->gpr_range_valid[dst] = 1;
                                ctx->gpr_range_min[dst] = (int32_t)lo;
                                ctx->gpr_range_max[dst] = (int32_t)hi;
                        }
                }
                jit_arm64_invalidate_packed_load(ctx, dst);
                return true;
        }
        if (ctx->gpr_cache_active && !jit_arm64_gpr_flush_required(ctx))
                return false;

#endif
        dst_tmp = dst;
        result_type = NOCT_VALUE_INT;
        write_tag = !rt_jit_tmp_has_fixed_primitive_type(ctx, dst_tmp,
                                                     result_type);

        dst *= (int)sizeof(struct rt_value);
        src1 *= (int)sizeof(struct rt_value);
        src2 *= (int)sizeof(struct rt_value);

        ASM {
                /* x1 = &env->frame->tmpvar[0] */
                /* w2 = src1 value, w3 = src2 value */
                LDR_W_IMM       (REG_X2, REG_X1, (uint32_t)(src1 + 8));
                LDR_W_IMM       (REG_X3, REG_X1, (uint32_t)(src2 + 8));
        }
        /* sub w2, w2, w3 */
        if (!jit_put_word(ctx, 0x4b000000 | (3u << 16) | (2u << 5) | 2u))
                return false;
        ASM {
                TYPED_STORE(REG_X2, dst, NOCT_VALUE_INT, write_tag);
        }
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
        int dst_tmp;
        int result_type;
        bool write_tag;

        CONSUME_TMPVAR(dst);
        CONSUME_TMPVAR(src1);
        CONSUME_TMPVAR(src2);

#if defined(NOCT_USE_OPTIMIZER)
        if (rt_jit_ploop_current_elided(ctx, 7))
                return true;
        if (ctx->packed_loop_hint_active) {
                rt_jit_ploop_remove_index_alias(ctx, dst);
                rt_jit_ploop_remove_base_alias(ctx, dst);
        }
        if (ctx->gpr_cache_active && ctx->gpr_reg_limit != 1) {
                uint32_t r1;
                uint32_t r2;
                uint32_t rd;
                unsigned pins;

                if (!jit_arm64_gpr_get(ctx, src1, 0, &r1))
                        return false;
                pins = 1u << (r1 - REG_X23);
                if (!jit_arm64_gpr_get(ctx, src2, pins, &r2))
                        return false;
                pins |= 1u << (r2 - REG_X23);
                if (!jit_arm64_gpr_is_cached(ctx, src1) &&
                    rt_jit_ploop_next_use_lpc(ctx, src1, ctx->lpc) ==
                    UINT32_MAX) {
                        if (!jit_arm64_gpr_rebind(ctx, dst, src1, &rd))
                                return false;
                } else if (!jit_arm64_gpr_is_cached(ctx, src2) &&
                           rt_jit_ploop_next_use_lpc(ctx, src2, ctx->lpc) ==
                           UINT32_MAX) {
                        if (!jit_arm64_gpr_rebind(ctx, dst, src2, &rd))
                                return false;
                } else if (!jit_arm64_gpr_dest(ctx, dst, pins, &rd)) {
                        return false;
                }
                if (!jit_put_word(ctx, 0x1b007c00u | (r2 << 16) |
                                         (r1 << 5) | rd))
                        return false;
                ctx->gpr_tmp_dirty[dst] = 1;
                ctx->gpr_range_valid[dst] = 0;
                jit_arm64_invalidate_packed_load(ctx, dst);
                return true;
        }
        if (ctx->gpr_cache_active && !jit_arm64_gpr_flush_required(ctx))
                return false;

#endif
        dst_tmp = dst;
        result_type = NOCT_VALUE_INT;
        write_tag = !rt_jit_tmp_has_fixed_primitive_type(ctx, dst_tmp,
                                                     result_type);

        dst *= (int)sizeof(struct rt_value);
        src1 *= (int)sizeof(struct rt_value);
        src2 *= (int)sizeof(struct rt_value);

        ASM {
                /* x1 = &env->frame->tmpvar[0] */
                /* w2 = src1 value, w3 = src2 value */
                LDR_W_IMM       (REG_X2, REG_X1, (uint32_t)(src1 + 8));
                LDR_W_IMM       (REG_X3, REG_X1, (uint32_t)(src2 + 8));
        }
        /* mul w2, w2, w3 (madd w2, w2, w3, wzr) */
        if (!jit_put_word(ctx, 0x1b007c00 | (3u << 16) | (2u << 5) | 2u))
                return false;
        ASM {
                TYPED_STORE(REG_X2, dst, NOCT_VALUE_INT, write_tag);
        }
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
        int dst_tmp;
        int result_type;
        bool write_tag;

        CONSUME_TMPVAR(dst);
        CONSUME_TMPVAR(src1);
        CONSUME_TMPVAR(src2);

#if defined(NOCT_USE_OPTIMIZER)
        if (rt_jit_ploop_current_elided(ctx, 7))
                return true;
        if (ctx->packed_loop_hint_active) {
                rt_jit_ploop_remove_index_alias(ctx, dst);
                rt_jit_ploop_remove_base_alias(ctx, dst);
        }
        if (ctx->gpr_cache_active && ctx->gpr_reg_limit != 1) {
                uint32_t r1;
                uint32_t r2;
                uint32_t rd;
                unsigned pins;

                if (!jit_arm64_gpr_get(ctx, src1, 0, &r1))
                        return false;
                pins = 1u << (r1 - REG_X23);
                if (!jit_arm64_gpr_get(ctx, src2, pins, &r2))
                        return false;
                pins |= 1u << (r2 - REG_X23);
                if (!jit_arm64_gpr_is_cached(ctx, src1) &&
                    rt_jit_ploop_next_use_lpc(ctx, src1, ctx->lpc) ==
                    UINT32_MAX) {
                        if (!jit_arm64_gpr_rebind(ctx, dst, src1, &rd))
                                return false;
                } else if (!jit_arm64_gpr_is_cached(ctx, src2) &&
                           rt_jit_ploop_next_use_lpc(ctx, src2, ctx->lpc) ==
                           UINT32_MAX) {
                        if (!jit_arm64_gpr_rebind(ctx, dst, src2, &rd))
                                return false;
                } else if (!jit_arm64_gpr_dest(ctx, dst, pins, &rd)) {
                        return false;
                }
                if (!jit_put_word(ctx, 0x1ac00c00u | (r2 << 16) |
                                         (r1 << 5) | rd))
                        return false;
                ctx->gpr_tmp_dirty[dst] = 1;
                ctx->gpr_range_valid[dst] = 0;
                jit_arm64_invalidate_all_packed_loads(ctx);
                return true;
        }
        if (ctx->gpr_cache_active && !jit_arm64_gpr_flush_required(ctx))
                return false;

#endif
        dst_tmp = dst;
        result_type = NOCT_VALUE_INT;
        write_tag = !rt_jit_tmp_has_fixed_primitive_type(ctx, dst_tmp,
                                                     result_type);

        dst *= (int)sizeof(struct rt_value);
        src1 *= (int)sizeof(struct rt_value);
        src2 *= (int)sizeof(struct rt_value);

        ASM {
                /* x1 = &env->frame->tmpvar[0] */
                /* w2 = src1 value, w3 = src2 value */
                LDR_W_IMM       (REG_X2, REG_X1, (uint32_t)(src1 + 8));
                LDR_W_IMM       (REG_X3, REG_X1, (uint32_t)(src2 + 8));
        }
        /* sdiv w2, w2, w3 (AArch64: no trap; /0 = 0) */
        if (!jit_put_word(ctx, 0x1ac00c00 | (3u << 16) | (2u << 5) | 2u))
                return false;
        ASM {
                TYPED_STORE(REG_X2, dst, NOCT_VALUE_INT, write_tag);
        }
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
        int dst_tmp;
        int result_type;
        bool write_tag;

        CONSUME_TMPVAR(dst);
        CONSUME_TMPVAR(src1);
        CONSUME_TMPVAR(src2);

#if defined(NOCT_USE_OPTIMIZER)
        if (rt_jit_ploop_current_elided(ctx, 7))
                return true;
        if (ctx->packed_loop_hint_active) {
                rt_jit_ploop_remove_index_alias(ctx, dst);
                rt_jit_ploop_remove_base_alias(ctx, dst);
        }
        if (ctx->gpr_cache_active && ctx->gpr_reg_limit != 1) {
                uint32_t r1;
                uint32_t r2;
                uint32_t rd;
                unsigned pins;

                if (!jit_arm64_gpr_get(ctx, src1, 0, &r1))
                        return false;
                pins = 1u << (r1 - REG_X23);
                if (!jit_arm64_gpr_get(ctx, src2, pins, &r2))
                        return false;
                pins |= 1u << (r2 - REG_X23);
                if (!jit_arm64_gpr_is_cached(ctx, src1) &&
                    rt_jit_ploop_next_use_lpc(ctx, src1, ctx->lpc) ==
                    UINT32_MAX) {
                        if (!jit_arm64_gpr_rebind(ctx, dst, src1, &rd))
                                return false;
                } else if (!jit_arm64_gpr_is_cached(ctx, src2) &&
                           rt_jit_ploop_next_use_lpc(ctx, src2, ctx->lpc) ==
                           UINT32_MAX) {
                        if (!jit_arm64_gpr_rebind(ctx, dst, src2, &rd))
                                return false;
                } else if (!jit_arm64_gpr_dest(ctx, dst, pins, &rd)) {
                        return false;
                }
                if (!jit_put_word(ctx, 0x1ac00c00u | (r2 << 16) |
                                         (r1 << 5) | REG_X4) ||
                    !jit_put_word(ctx, 0x1b008000u | (r2 << 16) |
                                         (r1 << 10) | (REG_X4 << 5) | rd))
                        return false;
                ctx->gpr_tmp_dirty[dst] = 1;
                ctx->gpr_range_valid[dst] = 0;
                jit_arm64_invalidate_all_packed_loads(ctx);
                return true;
        }
        if (ctx->gpr_cache_active && !jit_arm64_gpr_flush_required(ctx))
                return false;

#endif
        dst_tmp = dst;
        result_type = NOCT_VALUE_INT;
        write_tag = !rt_jit_tmp_has_fixed_primitive_type(ctx, dst_tmp,
                                                     result_type);

        dst *= (int)sizeof(struct rt_value);
        src1 *= (int)sizeof(struct rt_value);
        src2 *= (int)sizeof(struct rt_value);

        ASM {
                /* x1 = &env->frame->tmpvar[0] */
                /* w2 = src1 value, w3 = src2 value */
                LDR_W_IMM       (REG_X2, REG_X1, (uint32_t)(src1 + 8));
                LDR_W_IMM       (REG_X3, REG_X1, (uint32_t)(src2 + 8));
        }
        /* sdiv w4, w2, w3; msub w2, w4, w3, w2 */
        if (!jit_put_word(ctx, 0x1ac00c00 | (3u << 16) | (2u << 5) | 4u))
                return false;
        if (!jit_put_word(ctx, 0x1b008000 | (3u << 16) | (2u << 10) | (4u << 5) | 2u))
                return false;
        ASM {
                TYPED_STORE(REG_X2, dst, NOCT_VALUE_INT, write_tag);
        }
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
        int dst_tmp;
        int result_type;
        bool write_tag;

        CONSUME_TMPVAR(dst);
        CONSUME_TMPVAR(src1);
        CONSUME_TMPVAR(src2);

#if defined(NOCT_USE_OPTIMIZER)
        if (rt_jit_ploop_current_elided(ctx, 7))
                return true;
        if (ctx->packed_loop_hint_active) {
                rt_jit_ploop_remove_index_alias(ctx, dst);
                rt_jit_ploop_remove_base_alias(ctx, dst);
        }
        if (ctx->gpr_cache_active && ctx->gpr_reg_limit != 1) {
                uint32_t r1;
                uint32_t r2;
                uint32_t rd;
                unsigned pins;
                bool v1;
                bool v2;
                int32_t min1;
                int32_t max1;
                int32_t min2;
                int32_t max2;

                v1 = ctx->gpr_range_valid[src1] != 0;
                min1 = v1 ? ctx->gpr_range_min[src1] : 0;
                max1 = v1 ? ctx->gpr_range_max[src1] : 0;
                v2 = ctx->gpr_range_valid[src2] != 0;
                min2 = v2 ? ctx->gpr_range_min[src2] : 0;
                max2 = v2 ? ctx->gpr_range_max[src2] : 0;
                if (!jit_arm64_gpr_get(ctx, src1, 0, &r1))
                        return false;
                pins = 1u << (r1 - REG_X23);
                if (!jit_arm64_gpr_get(ctx, src2, pins, &r2))
                        return false;
                pins |= 1u << (r2 - REG_X23);
                if (!jit_arm64_gpr_is_cached(ctx, src1) &&
                    rt_jit_ploop_next_use_lpc(ctx, src1, ctx->lpc) ==
                    UINT32_MAX) {
                        if (!jit_arm64_gpr_rebind(ctx, dst, src1, &rd))
                                return false;
                } else if (!jit_arm64_gpr_is_cached(ctx, src2) &&
                           rt_jit_ploop_next_use_lpc(ctx, src2, ctx->lpc) ==
                           UINT32_MAX) {
                        if (!jit_arm64_gpr_rebind(ctx, dst, src2, &rd))
                                return false;
                } else if (!jit_arm64_gpr_dest(ctx, dst, pins, &rd)) {
                        return false;
                }
                if (!jit_put_word(ctx, 0x0a000000u | (r2 << 16) |
                                         (r1 << 5) | rd))
                        return false;
                ctx->gpr_tmp_dirty[dst] = 1;
                ctx->gpr_range_valid[dst] = 0;
                if (v1 && min1 == max1 && min1 >= 0) {
                        ctx->gpr_range_valid[dst] = 1;
                        ctx->gpr_range_min[dst] = 0;
                        ctx->gpr_range_max[dst] = min1;
                } else if (v2 && min2 == max2 && min2 >= 0) {
                        ctx->gpr_range_valid[dst] = 1;
                        ctx->gpr_range_min[dst] = 0;
                        ctx->gpr_range_max[dst] = min2;
                }
                jit_arm64_invalidate_packed_load(ctx, dst);
                return true;
        }
        if (ctx->gpr_cache_active && !jit_arm64_gpr_flush_required(ctx))
                return false;

#endif
        dst_tmp = dst;
        result_type = NOCT_VALUE_INT;
        write_tag = !rt_jit_tmp_has_fixed_primitive_type(ctx, dst_tmp,
                                                     result_type);

        dst *= (int)sizeof(struct rt_value);
        src1 *= (int)sizeof(struct rt_value);
        src2 *= (int)sizeof(struct rt_value);

        ASM {
                /* x1 = &env->frame->tmpvar[0] */
                /* w2 = src1 value, w3 = src2 value */
                LDR_W_IMM       (REG_X2, REG_X1, (uint32_t)(src1 + 8));
                LDR_W_IMM       (REG_X3, REG_X1, (uint32_t)(src2 + 8));
        }
        /* and w2, w2, w3 */
        if (!jit_put_word(ctx, 0x0a000000 | (3u << 16) | (2u << 5) | 2u))
                return false;
        ASM {
                TYPED_STORE(REG_X2, dst, NOCT_VALUE_INT, write_tag);
        }
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
        int dst_tmp;
        int result_type;
        bool write_tag;

        CONSUME_TMPVAR(dst);
        CONSUME_TMPVAR(src1);
        CONSUME_TMPVAR(src2);

#if defined(NOCT_USE_OPTIMIZER)
        if (rt_jit_ploop_current_elided(ctx, 7))
                return true;
        if (ctx->packed_loop_hint_active) {
                rt_jit_ploop_remove_index_alias(ctx, dst);
                rt_jit_ploop_remove_base_alias(ctx, dst);
        }
        if (ctx->gpr_cache_active && ctx->gpr_reg_limit != 1) {
                uint32_t r1;
                uint32_t r2;
                uint32_t rd;
                unsigned pins;

                if (!jit_arm64_gpr_get(ctx, src1, 0, &r1))
                        return false;
                pins = 1u << (r1 - REG_X23);
                if (!jit_arm64_gpr_get(ctx, src2, pins, &r2))
                        return false;
                pins |= 1u << (r2 - REG_X23);
                if (!jit_arm64_gpr_is_cached(ctx, src1) &&
                    rt_jit_ploop_next_use_lpc(ctx, src1, ctx->lpc) ==
                    UINT32_MAX) {
                        if (!jit_arm64_gpr_rebind(ctx, dst, src1, &rd))
                                return false;
                } else if (!jit_arm64_gpr_is_cached(ctx, src2) &&
                           rt_jit_ploop_next_use_lpc(ctx, src2, ctx->lpc) ==
                           UINT32_MAX) {
                        if (!jit_arm64_gpr_rebind(ctx, dst, src2, &rd))
                                return false;
                } else if (!jit_arm64_gpr_dest(ctx, dst, pins, &rd)) {
                        return false;
                }
                if (!jit_put_word(ctx, 0x2a000000u | (r2 << 16) |
                                         (r1 << 5) | rd))
                        return false;
                ctx->gpr_tmp_dirty[dst] = 1;
                ctx->gpr_range_valid[dst] = 0;
                jit_arm64_invalidate_packed_load(ctx, dst);
                return true;
        }
        if (ctx->gpr_cache_active && !jit_arm64_gpr_flush_required(ctx))
                return false;

#endif
        dst_tmp = dst;
        result_type = NOCT_VALUE_INT;
        write_tag = !rt_jit_tmp_has_fixed_primitive_type(ctx, dst_tmp,
                                                     result_type);

        dst *= (int)sizeof(struct rt_value);
        src1 *= (int)sizeof(struct rt_value);
        src2 *= (int)sizeof(struct rt_value);

        ASM {
                /* x1 = &env->frame->tmpvar[0] */
                /* w2 = src1 value, w3 = src2 value */
                LDR_W_IMM       (REG_X2, REG_X1, (uint32_t)(src1 + 8));
                LDR_W_IMM       (REG_X3, REG_X1, (uint32_t)(src2 + 8));
        }
        /* orr w2, w2, w3 */
        if (!jit_put_word(ctx, 0x2a000000 | (3u << 16) | (2u << 5) | 2u))
                return false;
        ASM {
                TYPED_STORE(REG_X2, dst, NOCT_VALUE_INT, write_tag);
        }
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
        int dst_tmp;
        int result_type;
        bool write_tag;

        CONSUME_TMPVAR(dst);
        CONSUME_TMPVAR(src1);
        CONSUME_TMPVAR(src2);

#if defined(NOCT_USE_OPTIMIZER)
        if (rt_jit_ploop_current_elided(ctx, 7))
                return true;
        if (ctx->packed_loop_hint_active) {
                rt_jit_ploop_remove_index_alias(ctx, dst);
                rt_jit_ploop_remove_base_alias(ctx, dst);
        }
        if (ctx->gpr_cache_active && ctx->gpr_reg_limit != 1) {
                uint32_t r1;
                uint32_t r2;
                uint32_t rd;
                unsigned pins;

                if (!jit_arm64_gpr_get(ctx, src1, 0, &r1))
                        return false;
                pins = 1u << (r1 - REG_X23);
                if (!jit_arm64_gpr_get(ctx, src2, pins, &r2))
                        return false;
                pins |= 1u << (r2 - REG_X23);
                if (!jit_arm64_gpr_is_cached(ctx, src1) &&
                    rt_jit_ploop_next_use_lpc(ctx, src1, ctx->lpc) ==
                    UINT32_MAX) {
                        if (!jit_arm64_gpr_rebind(ctx, dst, src1, &rd))
                                return false;
                } else if (!jit_arm64_gpr_is_cached(ctx, src2) &&
                           rt_jit_ploop_next_use_lpc(ctx, src2, ctx->lpc) ==
                           UINT32_MAX) {
                        if (!jit_arm64_gpr_rebind(ctx, dst, src2, &rd))
                                return false;
                } else if (!jit_arm64_gpr_dest(ctx, dst, pins, &rd)) {
                        return false;
                }
                if (!jit_put_word(ctx, 0x4a000000u | (r2 << 16) |
                                         (r1 << 5) | rd))
                        return false;
                ctx->gpr_tmp_dirty[dst] = 1;
                ctx->gpr_range_valid[dst] = 0;
                jit_arm64_invalidate_packed_load(ctx, dst);
                return true;
        }
        if (ctx->gpr_cache_active && !jit_arm64_gpr_flush_required(ctx))
                return false;

#endif
        dst_tmp = dst;
        result_type = NOCT_VALUE_INT;
        write_tag = !rt_jit_tmp_has_fixed_primitive_type(ctx, dst_tmp,
                                                     result_type);

        dst *= (int)sizeof(struct rt_value);
        src1 *= (int)sizeof(struct rt_value);
        src2 *= (int)sizeof(struct rt_value);

        ASM {
                /* x1 = &env->frame->tmpvar[0] */
                /* w2 = src1 value, w3 = src2 value */
                LDR_W_IMM       (REG_X2, REG_X1, (uint32_t)(src1 + 8));
                LDR_W_IMM       (REG_X3, REG_X1, (uint32_t)(src2 + 8));
        }
        /* eor w2, w2, w3 */
        if (!jit_put_word(ctx, 0x4a000000 | (3u << 16) | (2u << 5) | 2u))
                return false;
        ASM {
                TYPED_STORE(REG_X2, dst, NOCT_VALUE_INT, write_tag);
        }
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
        int dst_tmp;
        int result_type;
        bool write_tag;

        CONSUME_TMPVAR(dst);
        CONSUME_TMPVAR(src1);
        CONSUME_IMM8(src2);

#if defined(NOCT_USE_OPTIMIZER)
        if (rt_jit_ploop_current_elided(ctx, 7))
                return true;
        if (ctx->packed_loop_hint_active) {
                rt_jit_ploop_remove_index_alias(ctx, dst);
                rt_jit_ploop_remove_base_alias(ctx, dst);
        }
        if (ctx->gpr_cache_active && ctx->gpr_reg_limit != 1) {
                uint32_t r1;
                uint32_t rd;
                uint32_t sh;
                unsigned pins;

                if (!jit_arm64_gpr_get(ctx, src1, 0, &r1))
                        return false;
                pins = 1u << (r1 - REG_X23);
                if (!jit_arm64_gpr_is_cached(ctx, src1) &&
                    rt_jit_ploop_next_use_lpc(ctx, src1, ctx->lpc) ==
                    UINT32_MAX) {
                        if (!jit_arm64_gpr_rebind(ctx, dst, src1, &rd))
                                return false;
                } else if (!jit_arm64_gpr_dest(ctx, dst, pins, &rd)) {
                        return false;
                }
                sh = (uint32_t)(src2 & 31);
                if (sh == 0) {
                        if (!jit_arm64_gpr_mov(ctx, rd, r1))
                                return false;
                } else if (!jit_put_word(ctx, 0x53000000u |
                                                   (((32u - sh) & 31u) << 16) |
                                                   ((31u - sh) << 10) |
                                                   (r1 << 5) | rd)) {
                        return false;
                }
                ctx->gpr_tmp_dirty[dst] = 1;
                ctx->gpr_range_valid[dst] = 0;
                jit_arm64_invalidate_packed_load(ctx, dst);
                return true;
        }
        if (ctx->gpr_cache_active && !jit_arm64_gpr_flush_required(ctx))
                return false;

#endif
        dst_tmp = dst;
        result_type = NOCT_VALUE_INT;
        write_tag = !rt_jit_tmp_has_fixed_primitive_type(ctx, dst_tmp,
                                                     result_type);

        dst *= (int)sizeof(struct rt_value);
        src1 *= (int)sizeof(struct rt_value);

        /* src2 is the shift-count immediate (0..31). */
        ASM {
                LDR_W_IMM       (REG_X2, REG_X1, (uint32_t)(src1 + 8));
        }
        if ((src2 & 31) != 0) {
                uint32_t sh;

                sh = (uint32_t)(src2 & 31);
                /* lsl w2, w2, #sh (ubfm w2, w2, #(32-sh)&31, #(31-sh)) */
                if (!jit_put_word(ctx, 0x53000000 |
                                  (((32u - sh) & 31u) << 16) |
                                  ((31u - sh) << 10) |
                                  (2u << 5) | 2u))
                        return false;
        }
        ASM {
                TYPED_STORE(REG_X2, dst, NOCT_VALUE_INT, write_tag);
        }
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
        int dst_tmp;
        int result_type;
        bool write_tag;

        CONSUME_TMPVAR(dst);
        CONSUME_TMPVAR(src1);
        CONSUME_IMM8(src2);

#if defined(NOCT_USE_OPTIMIZER)
        if (rt_jit_ploop_current_elided(ctx, 7))
                return true;
        if (ctx->packed_loop_hint_active) {
                rt_jit_ploop_remove_index_alias(ctx, dst);
                rt_jit_ploop_remove_base_alias(ctx, dst);
        }
        if (ctx->gpr_cache_active && ctx->gpr_reg_limit != 1) {
                uint32_t r1;
                uint32_t rd;
                uint32_t sh;
                unsigned pins;

                if (!jit_arm64_gpr_get(ctx, src1, 0, &r1))
                        return false;
                pins = 1u << (r1 - REG_X23);
                if (!jit_arm64_gpr_is_cached(ctx, src1) &&
                    rt_jit_ploop_next_use_lpc(ctx, src1, ctx->lpc) ==
                    UINT32_MAX) {
                        if (!jit_arm64_gpr_rebind(ctx, dst, src1, &rd))
                                return false;
                } else if (!jit_arm64_gpr_dest(ctx, dst, pins, &rd)) {
                        return false;
                }
                sh = (uint32_t)(src2 & 31);
                if (sh == 0) {
                        if (!jit_arm64_gpr_mov(ctx, rd, r1))
                                return false;
                } else if (!jit_put_word(ctx, 0x53000000u |
                                                   (sh << 16) |
                                                   (31u << 10) |
                                                   (r1 << 5) | rd)) {
                        return false;
                }
                ctx->gpr_tmp_dirty[dst] = 1;
                ctx->gpr_range_valid[dst] = 0;
                jit_arm64_invalidate_packed_load(ctx, dst);
                return true;
        }
        if (ctx->gpr_cache_active && !jit_arm64_gpr_flush_required(ctx))
                return false;

#endif
        dst_tmp = dst;
        result_type = NOCT_VALUE_INT;
        write_tag = !rt_jit_tmp_has_fixed_primitive_type(ctx, dst_tmp,
                                                     result_type);

        dst *= (int)sizeof(struct rt_value);
        src1 *= (int)sizeof(struct rt_value);

        /* src2 is the shift-count immediate (0..31). */
        ASM {
                LDR_W_IMM       (REG_X2, REG_X1, (uint32_t)(src1 + 8));
        }
        if ((src2 & 31) != 0) {
                uint32_t sh;

                sh = (uint32_t)(src2 & 31);
                /* lsr w2, w2, #sh (LOGICAL; ubfm w2, w2, #sh, #31) */
                if (!jit_put_word(ctx, 0x53000000 |
                                  (sh << 16) |
                                  (31u << 10) |
                                  (2u << 5) | 2u))
                        return false;
        }
        ASM {
                TYPED_STORE(REG_X2, dst, NOCT_VALUE_INT, write_tag);
        }
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
        int dst_tmp;
        int result_type;
        bool write_tag;

        CONSUME_TMPVAR(dst);
        CONSUME_TMPVAR(src1);
        CONSUME_TMPVAR(src2);

#if defined(NOCT_USE_OPTIMIZER)
        if (rt_jit_ploop_current_elided(ctx, 7))
                return true;
        if (ctx->packed_loop_hint_active) {
                rt_jit_ploop_remove_index_alias(ctx, dst);
                rt_jit_ploop_remove_base_alias(ctx, dst);
        }
        if (ctx->gpr_cache_active && ctx->gpr_reg_limit != 1) {
                uint32_t r1;
                uint32_t r2;
                uint32_t rd;
                unsigned pins;

                if (!jit_arm64_gpr_get(ctx, src1, 0, &r1))
                        return false;
                pins = 1u << (r1 - REG_X23);
                if (!jit_arm64_gpr_get(ctx, src2, pins, &r2))
                        return false;
                pins |= 1u << (r2 - REG_X23);
                if (!jit_arm64_gpr_is_cached(ctx, src1) &&
                    rt_jit_ploop_next_use_lpc(ctx, src1, ctx->lpc) ==
                    UINT32_MAX) {
                        if (!jit_arm64_gpr_rebind(ctx, dst, src1, &rd))
                                return false;
                } else if (!jit_arm64_gpr_is_cached(ctx, src2) &&
                           rt_jit_ploop_next_use_lpc(ctx, src2, ctx->lpc) ==
                           UINT32_MAX) {
                        if (!jit_arm64_gpr_rebind(ctx, dst, src2, &rd))
                                return false;
                } else if (!jit_arm64_gpr_dest(ctx, dst, pins, &rd)) {
                        return false;
                }
                if (!jit_put_word(ctx, 0x6b00001fu | (r2 << 16) |
                                         (r1 << 5)))
                        return false;
                if (!jit_put_word(ctx, 0x1a9f07e0u |
                                         ((TYPED_COND_LT ^ 1u) << 12) | rd))
                        return false;
                ctx->gpr_tmp_dirty[dst] = 1;
                ctx->gpr_range_valid[dst] = 0;
                jit_arm64_invalidate_packed_load(ctx, dst);
                return true;
        }
        if (ctx->gpr_cache_active && !jit_arm64_gpr_flush_required(ctx))
                return false;

#endif
        dst_tmp = dst;
        result_type = NOCT_VALUE_INT;
        write_tag = !rt_jit_tmp_has_fixed_primitive_type(ctx, dst_tmp,
                                                     result_type);

        dst *= (int)sizeof(struct rt_value);
        src1 *= (int)sizeof(struct rt_value);
        src2 *= (int)sizeof(struct rt_value);

        ASM {
                /* x1 = &env->frame->tmpvar[0] */
                /* w2 = src1 value, w3 = src2 value */
                LDR_W_IMM       (REG_X2, REG_X1, (uint32_t)(src1 + 8));
                LDR_W_IMM       (REG_X3, REG_X1, (uint32_t)(src2 + 8));
        }
        /* cmp w2, w3 (subs wzr, w2, w3) */
        if (!jit_put_word(ctx, 0x6b000000 | (3u << 16) | (2u << 5) | 31u))
                return false;
        TYPED_CSET_W(2, TYPED_COND_LT);
        ASM {
                TYPED_STORE(REG_X2, dst, NOCT_VALUE_INT, write_tag);
        }
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
        int dst_tmp;
        int result_type;
        bool write_tag;

        CONSUME_TMPVAR(dst);
        CONSUME_TMPVAR(src1);
        CONSUME_TMPVAR(src2);

#if defined(NOCT_USE_OPTIMIZER)
        if (rt_jit_ploop_current_elided(ctx, 7))
                return true;
        if (ctx->packed_loop_hint_active) {
                rt_jit_ploop_remove_index_alias(ctx, dst);
                rt_jit_ploop_remove_base_alias(ctx, dst);
        }
        if (ctx->gpr_cache_active && ctx->gpr_reg_limit != 1) {
                uint32_t r1;
                uint32_t r2;
                uint32_t rd;
                unsigned pins;

                if (!jit_arm64_gpr_get(ctx, src1, 0, &r1))
                        return false;
                pins = 1u << (r1 - REG_X23);
                if (!jit_arm64_gpr_get(ctx, src2, pins, &r2))
                        return false;
                pins |= 1u << (r2 - REG_X23);
                if (!jit_arm64_gpr_is_cached(ctx, src1) &&
                    rt_jit_ploop_next_use_lpc(ctx, src1, ctx->lpc) ==
                    UINT32_MAX) {
                        if (!jit_arm64_gpr_rebind(ctx, dst, src1, &rd))
                                return false;
                } else if (!jit_arm64_gpr_is_cached(ctx, src2) &&
                           rt_jit_ploop_next_use_lpc(ctx, src2, ctx->lpc) ==
                           UINT32_MAX) {
                        if (!jit_arm64_gpr_rebind(ctx, dst, src2, &rd))
                                return false;
                } else if (!jit_arm64_gpr_dest(ctx, dst, pins, &rd)) {
                        return false;
                }
                if (!jit_put_word(ctx, 0x6b00001fu | (r2 << 16) |
                                         (r1 << 5)))
                        return false;
                if (!jit_put_word(ctx, 0x1a9f07e0u |
                                         ((TYPED_COND_LE ^ 1u) << 12) | rd))
                        return false;
                ctx->gpr_tmp_dirty[dst] = 1;
                ctx->gpr_range_valid[dst] = 0;
                jit_arm64_invalidate_packed_load(ctx, dst);
                return true;
        }
        if (ctx->gpr_cache_active && !jit_arm64_gpr_flush_required(ctx))
                return false;

#endif
        dst_tmp = dst;
        result_type = NOCT_VALUE_INT;
        write_tag = !rt_jit_tmp_has_fixed_primitive_type(ctx, dst_tmp,
                                                     result_type);

        dst *= (int)sizeof(struct rt_value);
        src1 *= (int)sizeof(struct rt_value);
        src2 *= (int)sizeof(struct rt_value);

        ASM {
                /* x1 = &env->frame->tmpvar[0] */
                /* w2 = src1 value, w3 = src2 value */
                LDR_W_IMM       (REG_X2, REG_X1, (uint32_t)(src1 + 8));
                LDR_W_IMM       (REG_X3, REG_X1, (uint32_t)(src2 + 8));
        }
        /* cmp w2, w3 (subs wzr, w2, w3) */
        if (!jit_put_word(ctx, 0x6b000000 | (3u << 16) | (2u << 5) | 31u))
                return false;
        TYPED_CSET_W(2, TYPED_COND_LE);
        ASM {
                TYPED_STORE(REG_X2, dst, NOCT_VALUE_INT, write_tag);
        }
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
        int dst_tmp;
        int result_type;
        bool write_tag;

        CONSUME_TMPVAR(dst);
        CONSUME_TMPVAR(src1);
        CONSUME_TMPVAR(src2);

#if defined(NOCT_USE_OPTIMIZER)
        if (rt_jit_ploop_current_elided(ctx, 7))
                return true;
        if (ctx->packed_loop_hint_active) {
                rt_jit_ploop_remove_index_alias(ctx, dst);
                rt_jit_ploop_remove_base_alias(ctx, dst);
        }
        if (ctx->gpr_cache_active && ctx->gpr_reg_limit != 1) {
                uint32_t r1;
                uint32_t r2;
                uint32_t rd;
                unsigned pins;

                if (!jit_arm64_gpr_get(ctx, src1, 0, &r1))
                        return false;
                pins = 1u << (r1 - REG_X23);
                if (!jit_arm64_gpr_get(ctx, src2, pins, &r2))
                        return false;
                pins |= 1u << (r2 - REG_X23);
                if (!jit_arm64_gpr_is_cached(ctx, src1) &&
                    rt_jit_ploop_next_use_lpc(ctx, src1, ctx->lpc) ==
                    UINT32_MAX) {
                        if (!jit_arm64_gpr_rebind(ctx, dst, src1, &rd))
                                return false;
                } else if (!jit_arm64_gpr_is_cached(ctx, src2) &&
                           rt_jit_ploop_next_use_lpc(ctx, src2, ctx->lpc) ==
                           UINT32_MAX) {
                        if (!jit_arm64_gpr_rebind(ctx, dst, src2, &rd))
                                return false;
                } else if (!jit_arm64_gpr_dest(ctx, dst, pins, &rd)) {
                        return false;
                }
                if (!jit_put_word(ctx, 0x6b00001fu | (r2 << 16) |
                                         (r1 << 5)))
                        return false;
                if (!jit_put_word(ctx, 0x1a9f07e0u |
                                         ((TYPED_COND_GT ^ 1u) << 12) | rd))
                        return false;
                ctx->gpr_tmp_dirty[dst] = 1;
                ctx->gpr_range_valid[dst] = 0;
                jit_arm64_invalidate_packed_load(ctx, dst);
                return true;
        }
        if (ctx->gpr_cache_active && !jit_arm64_gpr_flush_required(ctx))
                return false;

#endif
        dst_tmp = dst;
        result_type = NOCT_VALUE_INT;
        write_tag = !rt_jit_tmp_has_fixed_primitive_type(ctx, dst_tmp,
                                                     result_type);

        dst *= (int)sizeof(struct rt_value);
        src1 *= (int)sizeof(struct rt_value);
        src2 *= (int)sizeof(struct rt_value);

        ASM {
                /* x1 = &env->frame->tmpvar[0] */
                /* w2 = src1 value, w3 = src2 value */
                LDR_W_IMM       (REG_X2, REG_X1, (uint32_t)(src1 + 8));
                LDR_W_IMM       (REG_X3, REG_X1, (uint32_t)(src2 + 8));
        }
        /* cmp w2, w3 (subs wzr, w2, w3) */
        if (!jit_put_word(ctx, 0x6b000000 | (3u << 16) | (2u << 5) | 31u))
                return false;
        TYPED_CSET_W(2, TYPED_COND_GT);
        ASM {
                TYPED_STORE(REG_X2, dst, NOCT_VALUE_INT, write_tag);
        }
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
        int dst_tmp;
        int result_type;
        bool write_tag;

        CONSUME_TMPVAR(dst);
        CONSUME_TMPVAR(src1);
        CONSUME_TMPVAR(src2);

#if defined(NOCT_USE_OPTIMIZER)
        if (rt_jit_ploop_current_elided(ctx, 7))
                return true;
        if (ctx->packed_loop_hint_active) {
                rt_jit_ploop_remove_index_alias(ctx, dst);
                rt_jit_ploop_remove_base_alias(ctx, dst);
        }
        if (ctx->gpr_cache_active && ctx->gpr_reg_limit != 1) {
                uint32_t r1;
                uint32_t r2;
                uint32_t rd;
                unsigned pins;

                if (!jit_arm64_gpr_get(ctx, src1, 0, &r1))
                        return false;
                pins = 1u << (r1 - REG_X23);
                if (!jit_arm64_gpr_get(ctx, src2, pins, &r2))
                        return false;
                pins |= 1u << (r2 - REG_X23);
                if (!jit_arm64_gpr_is_cached(ctx, src1) &&
                    rt_jit_ploop_next_use_lpc(ctx, src1, ctx->lpc) ==
                    UINT32_MAX) {
                        if (!jit_arm64_gpr_rebind(ctx, dst, src1, &rd))
                                return false;
                } else if (!jit_arm64_gpr_is_cached(ctx, src2) &&
                           rt_jit_ploop_next_use_lpc(ctx, src2, ctx->lpc) ==
                           UINT32_MAX) {
                        if (!jit_arm64_gpr_rebind(ctx, dst, src2, &rd))
                                return false;
                } else if (!jit_arm64_gpr_dest(ctx, dst, pins, &rd)) {
                        return false;
                }
                if (!jit_put_word(ctx, 0x6b00001fu | (r2 << 16) |
                                         (r1 << 5)))
                        return false;
                if (!jit_put_word(ctx, 0x1a9f07e0u |
                                         ((TYPED_COND_GE ^ 1u) << 12) | rd))
                        return false;
                ctx->gpr_tmp_dirty[dst] = 1;
                ctx->gpr_range_valid[dst] = 0;
                jit_arm64_invalidate_packed_load(ctx, dst);
                return true;
        }
        if (ctx->gpr_cache_active && !jit_arm64_gpr_flush_required(ctx))
                return false;

#endif
        dst_tmp = dst;
        result_type = NOCT_VALUE_INT;
        write_tag = !rt_jit_tmp_has_fixed_primitive_type(ctx, dst_tmp,
                                                     result_type);

        dst *= (int)sizeof(struct rt_value);
        src1 *= (int)sizeof(struct rt_value);
        src2 *= (int)sizeof(struct rt_value);

        ASM {
                /* x1 = &env->frame->tmpvar[0] */
                /* w2 = src1 value, w3 = src2 value */
                LDR_W_IMM       (REG_X2, REG_X1, (uint32_t)(src1 + 8));
                LDR_W_IMM       (REG_X3, REG_X1, (uint32_t)(src2 + 8));
        }
        /* cmp w2, w3 (subs wzr, w2, w3) */
        if (!jit_put_word(ctx, 0x6b000000 | (3u << 16) | (2u << 5) | 31u))
                return false;
        TYPED_CSET_W(2, TYPED_COND_GE);
        ASM {
                TYPED_STORE(REG_X2, dst, NOCT_VALUE_INT, write_tag);
        }
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
        int dst_tmp;
        int result_type;
        bool write_tag;

        CONSUME_TMPVAR(dst);
        CONSUME_TMPVAR(src1);
        CONSUME_TMPVAR(src2);

#if defined(NOCT_USE_OPTIMIZER)
        if (rt_jit_ploop_current_elided(ctx, 7))
                return true;
        if (ctx->packed_loop_hint_active) {
                rt_jit_ploop_remove_index_alias(ctx, dst);
                rt_jit_ploop_remove_base_alias(ctx, dst);
        }
        if (ctx->gpr_cache_active && !jit_arm64_gpr_flush_required(ctx))
                return false;

#endif
        dst_tmp = dst;
        result_type = NOCT_VALUE_FLOAT;
        write_tag = !rt_jit_tmp_has_fixed_primitive_type(ctx, dst_tmp,
                                                     result_type);

        dst *= (int)sizeof(struct rt_value);
        src1 *= (int)sizeof(struct rt_value);
        src2 *= (int)sizeof(struct rt_value);

        ASM {
                /* s0 = src1 float bits, s1 = src2 float bits */
                LDR_W_IMM       (REG_X2, REG_X1, (uint32_t)(src1 + 8));
                LDR_W_IMM       (REG_X3, REG_X1, (uint32_t)(src2 + 8));
        }
        /* fmov s0, w2; fmov s1, w3 */
        if (!jit_put_word(ctx, 0x1e270000 | (2u << 5) | 0u))
                return false;
        if (!jit_put_word(ctx, 0x1e270000 | (3u << 5) | 1u))
                return false;
        /* fadd s0, s0, s1 */
        if (!jit_put_word(ctx, 0x1e202800 | (1u << 16) | (0u << 5) | 0u))
                return false;
        /* fmov w2, s0 */
        if (!jit_put_word(ctx, 0x1e260000 | (0u << 5) | 2u))
                return false;
        ASM {
                TYPED_STORE(REG_X2, dst, NOCT_VALUE_FLOAT, write_tag);
        }
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
        int dst_tmp;
        int result_type;
        bool write_tag;

        CONSUME_TMPVAR(dst);
        CONSUME_TMPVAR(src1);
        CONSUME_TMPVAR(src2);

#if defined(NOCT_USE_OPTIMIZER)
        if (rt_jit_ploop_current_elided(ctx, 7))
                return true;
        if (ctx->packed_loop_hint_active) {
                rt_jit_ploop_remove_index_alias(ctx, dst);
                rt_jit_ploop_remove_base_alias(ctx, dst);
        }
        if (ctx->gpr_cache_active && !jit_arm64_gpr_flush_required(ctx))
                return false;

#endif
        dst_tmp = dst;
        result_type = NOCT_VALUE_FLOAT;
        write_tag = !rt_jit_tmp_has_fixed_primitive_type(ctx, dst_tmp,
                                                     result_type);

        dst *= (int)sizeof(struct rt_value);
        src1 *= (int)sizeof(struct rt_value);
        src2 *= (int)sizeof(struct rt_value);

        ASM {
                /* s0 = src1 float bits, s1 = src2 float bits */
                LDR_W_IMM       (REG_X2, REG_X1, (uint32_t)(src1 + 8));
                LDR_W_IMM       (REG_X3, REG_X1, (uint32_t)(src2 + 8));
        }
        /* fmov s0, w2; fmov s1, w3 */
        if (!jit_put_word(ctx, 0x1e270000 | (2u << 5) | 0u))
                return false;
        if (!jit_put_word(ctx, 0x1e270000 | (3u << 5) | 1u))
                return false;
        /* fsub s0, s0, s1 */
        if (!jit_put_word(ctx, 0x1e203800 | (1u << 16) | (0u << 5) | 0u))
                return false;
        /* fmov w2, s0 */
        if (!jit_put_word(ctx, 0x1e260000 | (0u << 5) | 2u))
                return false;
        ASM {
                TYPED_STORE(REG_X2, dst, NOCT_VALUE_FLOAT, write_tag);
        }
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
        int dst_tmp;
        int result_type;
        bool write_tag;

        CONSUME_TMPVAR(dst);
        CONSUME_TMPVAR(src1);
        CONSUME_TMPVAR(src2);

#if defined(NOCT_USE_OPTIMIZER)
        if (rt_jit_ploop_current_elided(ctx, 7))
                return true;
        if (ctx->packed_loop_hint_active) {
                rt_jit_ploop_remove_index_alias(ctx, dst);
                rt_jit_ploop_remove_base_alias(ctx, dst);
        }
        if (ctx->gpr_cache_active && !jit_arm64_gpr_flush_required(ctx))
                return false;

#endif
        dst_tmp = dst;
        result_type = NOCT_VALUE_FLOAT;
        write_tag = !rt_jit_tmp_has_fixed_primitive_type(ctx, dst_tmp,
                                                     result_type);

        dst *= (int)sizeof(struct rt_value);
        src1 *= (int)sizeof(struct rt_value);
        src2 *= (int)sizeof(struct rt_value);

        ASM {
                /* s0 = src1 float bits, s1 = src2 float bits */
                LDR_W_IMM       (REG_X2, REG_X1, (uint32_t)(src1 + 8));
                LDR_W_IMM       (REG_X3, REG_X1, (uint32_t)(src2 + 8));
        }
        /* fmov s0, w2; fmov s1, w3 */
        if (!jit_put_word(ctx, 0x1e270000 | (2u << 5) | 0u))
                return false;
        if (!jit_put_word(ctx, 0x1e270000 | (3u << 5) | 1u))
                return false;
        /* fmul s0, s0, s1 */
        if (!jit_put_word(ctx, 0x1e200800 | (1u << 16) | (0u << 5) | 0u))
                return false;
        /* fmov w2, s0 */
        if (!jit_put_word(ctx, 0x1e260000 | (0u << 5) | 2u))
                return false;
        ASM {
                TYPED_STORE(REG_X2, dst, NOCT_VALUE_FLOAT, write_tag);
        }
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
        int dst_tmp;
        int result_type;
        bool write_tag;

        CONSUME_TMPVAR(dst);
        CONSUME_TMPVAR(src1);
        CONSUME_TMPVAR(src2);

#if defined(NOCT_USE_OPTIMIZER)
        if (rt_jit_ploop_current_elided(ctx, 7))
                return true;
        if (ctx->packed_loop_hint_active) {
                rt_jit_ploop_remove_index_alias(ctx, dst);
                rt_jit_ploop_remove_base_alias(ctx, dst);
        }
        if (ctx->gpr_cache_active && !jit_arm64_gpr_flush_required(ctx))
                return false;

#endif
        dst_tmp = dst;
        result_type = NOCT_VALUE_FLOAT;
        write_tag = !rt_jit_tmp_has_fixed_primitive_type(ctx, dst_tmp,
                                                     result_type);

        dst *= (int)sizeof(struct rt_value);
        src1 *= (int)sizeof(struct rt_value);
        src2 *= (int)sizeof(struct rt_value);

        ASM {
                /* s0 = src1 float bits, s1 = src2 float bits */
                LDR_W_IMM       (REG_X2, REG_X1, (uint32_t)(src1 + 8));
                LDR_W_IMM       (REG_X3, REG_X1, (uint32_t)(src2 + 8));
        }
        /* fmov s0, w2; fmov s1, w3 */
        if (!jit_put_word(ctx, 0x1e270000 | (2u << 5) | 0u))
                return false;
        if (!jit_put_word(ctx, 0x1e270000 | (3u << 5) | 1u))
                return false;
        /* fdiv s0, s0, s1 (IEEE-total; 07 Part 0) */
        if (!jit_put_word(ctx, 0x1e201800 | (1u << 16) | (0u << 5) | 0u))
                return false;
        /* fmov w2, s0 */
        if (!jit_put_word(ctx, 0x1e260000 | (0u << 5) | 2u))
                return false;
        ASM {
                TYPED_STORE(REG_X2, dst, NOCT_VALUE_FLOAT, write_tag);
        }
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
        int dst_tmp;
        int result_type;
        bool write_tag;

        CONSUME_TMPVAR(dst);
        CONSUME_TMPVAR(src1);
        CONSUME_TMPVAR(src2);

#if defined(NOCT_USE_OPTIMIZER)
        if (rt_jit_ploop_current_elided(ctx, 7))
                return true;
        if (ctx->packed_loop_hint_active) {
                rt_jit_ploop_remove_index_alias(ctx, dst);
                rt_jit_ploop_remove_base_alias(ctx, dst);
        }
        if (ctx->gpr_cache_active && !jit_arm64_gpr_flush_required(ctx))
                return false;

#endif
        dst_tmp = dst;
        result_type = NOCT_VALUE_INT;
        write_tag = !rt_jit_tmp_has_fixed_primitive_type(ctx, dst_tmp,
                                                     result_type);

        dst *= (int)sizeof(struct rt_value);
        src1 *= (int)sizeof(struct rt_value);
        src2 *= (int)sizeof(struct rt_value);

        ASM {
                LDR_W_IMM       (REG_X2, REG_X1, (uint32_t)(src1 + 8));
                LDR_W_IMM       (REG_X3, REG_X1, (uint32_t)(src2 + 8));
        }
        /* fmov s0, w2; fmov s1, w3 */
        if (!jit_put_word(ctx, 0x1e270000 | (2u << 5) | 0u))
                return false;
        if (!jit_put_word(ctx, 0x1e270000 | (3u << 5) | 1u))
                return false;
        /* fcmp s0, s1 */
        if (!jit_put_word(ctx, 0x1e202000 | (1u << 16) | (0u << 5)))
                return false;
        TYPED_CSET_W(2, TYPED_COND_MI);
        ASM {
                TYPED_STORE(REG_X2, dst, NOCT_VALUE_INT, write_tag);
        }
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
        int dst_tmp;
        int result_type;
        bool write_tag;

        CONSUME_TMPVAR(dst);
        CONSUME_TMPVAR(src1);
        CONSUME_TMPVAR(src2);

#if defined(NOCT_USE_OPTIMIZER)
        if (rt_jit_ploop_current_elided(ctx, 7))
                return true;
        if (ctx->packed_loop_hint_active) {
                rt_jit_ploop_remove_index_alias(ctx, dst);
                rt_jit_ploop_remove_base_alias(ctx, dst);
        }
        if (ctx->gpr_cache_active && !jit_arm64_gpr_flush_required(ctx))
                return false;

#endif
        dst_tmp = dst;
        result_type = NOCT_VALUE_INT;
        write_tag = !rt_jit_tmp_has_fixed_primitive_type(ctx, dst_tmp,
                                                     result_type);

        dst *= (int)sizeof(struct rt_value);
        src1 *= (int)sizeof(struct rt_value);
        src2 *= (int)sizeof(struct rt_value);

        ASM {
                LDR_W_IMM       (REG_X2, REG_X1, (uint32_t)(src1 + 8));
                LDR_W_IMM       (REG_X3, REG_X1, (uint32_t)(src2 + 8));
        }
        /* fmov s0, w2; fmov s1, w3 */
        if (!jit_put_word(ctx, 0x1e270000 | (2u << 5) | 0u))
                return false;
        if (!jit_put_word(ctx, 0x1e270000 | (3u << 5) | 1u))
                return false;
        /* fcmp s0, s1 */
        if (!jit_put_word(ctx, 0x1e202000 | (1u << 16) | (0u << 5)))
                return false;
        TYPED_CSET_W(2, TYPED_COND_LS);
        ASM {
                TYPED_STORE(REG_X2, dst, NOCT_VALUE_INT, write_tag);
        }
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
        int dst_tmp;
        int result_type;
        bool write_tag;

        CONSUME_TMPVAR(dst);
        CONSUME_TMPVAR(src1);
        CONSUME_TMPVAR(src2);

#if defined(NOCT_USE_OPTIMIZER)
        if (rt_jit_ploop_current_elided(ctx, 7))
                return true;
        if (ctx->packed_loop_hint_active) {
                rt_jit_ploop_remove_index_alias(ctx, dst);
                rt_jit_ploop_remove_base_alias(ctx, dst);
        }
        if (ctx->gpr_cache_active && !jit_arm64_gpr_flush_required(ctx))
                return false;

#endif
        dst_tmp = dst;
        result_type = NOCT_VALUE_INT;
        write_tag = !rt_jit_tmp_has_fixed_primitive_type(ctx, dst_tmp,
                                                     result_type);

        dst *= (int)sizeof(struct rt_value);
        src1 *= (int)sizeof(struct rt_value);
        src2 *= (int)sizeof(struct rt_value);

        ASM {
                LDR_W_IMM       (REG_X2, REG_X1, (uint32_t)(src1 + 8));
                LDR_W_IMM       (REG_X3, REG_X1, (uint32_t)(src2 + 8));
        }
        /* fmov s0, w2; fmov s1, w3 */
        if (!jit_put_word(ctx, 0x1e270000 | (2u << 5) | 0u))
                return false;
        if (!jit_put_word(ctx, 0x1e270000 | (3u << 5) | 1u))
                return false;
        /* fcmp s0, s1 */
        if (!jit_put_word(ctx, 0x1e202000 | (1u << 16) | (0u << 5)))
                return false;
        TYPED_CSET_W(2, TYPED_COND_GT);
        ASM {
                TYPED_STORE(REG_X2, dst, NOCT_VALUE_INT, write_tag);
        }
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
        int dst_tmp;
        int result_type;
        bool write_tag;

        CONSUME_TMPVAR(dst);
        CONSUME_TMPVAR(src1);
        CONSUME_TMPVAR(src2);

#if defined(NOCT_USE_OPTIMIZER)
        if (rt_jit_ploop_current_elided(ctx, 7))
                return true;
        if (ctx->packed_loop_hint_active) {
                rt_jit_ploop_remove_index_alias(ctx, dst);
                rt_jit_ploop_remove_base_alias(ctx, dst);
        }
        if (ctx->gpr_cache_active && !jit_arm64_gpr_flush_required(ctx))
                return false;

#endif
        dst_tmp = dst;
        result_type = NOCT_VALUE_INT;
        write_tag = !rt_jit_tmp_has_fixed_primitive_type(ctx, dst_tmp,
                                                     result_type);

        dst *= (int)sizeof(struct rt_value);
        src1 *= (int)sizeof(struct rt_value);
        src2 *= (int)sizeof(struct rt_value);

        ASM {
                LDR_W_IMM       (REG_X2, REG_X1, (uint32_t)(src1 + 8));
                LDR_W_IMM       (REG_X3, REG_X1, (uint32_t)(src2 + 8));
        }
        /* fmov s0, w2; fmov s1, w3 */
        if (!jit_put_word(ctx, 0x1e270000 | (2u << 5) | 0u))
                return false;
        if (!jit_put_word(ctx, 0x1e270000 | (3u << 5) | 1u))
                return false;
        /* fcmp s0, s1 */
        if (!jit_put_word(ctx, 0x1e202000 | (1u << 16) | (0u << 5)))
                return false;
        TYPED_CSET_W(2, TYPED_COND_GE);
        ASM {
                TYPED_STORE(REG_X2, dst, NOCT_VALUE_INT, write_tag);
        }
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
        int dst_ofs;
        int src1_ofs;
        int src2_ofs;
        uint8_t *zero_branch;
        uint8_t *done_branch;
        uint8_t *target;

        CONSUME_TMPVAR(dst);
        CONSUME_TMPVAR(src1);
        CONSUME_TMPVAR(src2);

#if defined(NOCT_USE_OPTIMIZER)
        if (rt_jit_ploop_current_elided(ctx, 7))
                return true;
        if (ctx->packed_loop_hint_active) {
                rt_jit_ploop_remove_index_alias(ctx, dst);
                rt_jit_ploop_remove_base_alias(ctx, dst);
        }
        if (ctx->gpr_cache_active && ctx->gpr_reg_limit != 1 &&
            ctx->gpr_range_valid[src2] != 0 &&
            (ctx->gpr_range_min[src2] > 0 ||
             ctx->gpr_range_max[src2] < -1)) {
                uint32_t r1;
                uint32_t r2;
                uint32_t rd;
                unsigned pins;

                if (!jit_arm64_gpr_get(ctx, src1, 0, &r1))
                        return false;
                pins = 1u << (r1 - REG_X23);
                if (!jit_arm64_gpr_get(ctx, src2, pins, &r2))
                        return false;
                pins |= 1u << (r2 - REG_X23);
                if (!jit_arm64_gpr_is_cached(ctx, src1) &&
                    rt_jit_ploop_next_use_lpc(ctx, src1, ctx->lpc) ==
                    UINT32_MAX) {
                        if (!jit_arm64_gpr_rebind(ctx, dst, src1, &rd))
                                return false;
                } else if (!jit_arm64_gpr_is_cached(ctx, src2) &&
                           rt_jit_ploop_next_use_lpc(ctx, src2, ctx->lpc) ==
                           UINT32_MAX) {
                        if (!jit_arm64_gpr_rebind(ctx, dst, src2, &rd))
                                return false;
                } else if (!jit_arm64_gpr_dest(ctx, dst, pins, &rd)) {
                        return false;
                }
                if (!jit_put_word(ctx, 0x1ac00c00u | (r2 << 16) |
                                         (r1 << 5) | rd))
                        return false;
                ctx->gpr_tmp_dirty[dst] = 1;
                ctx->gpr_range_valid[dst] = 0;
                ctx->gpr_proven_divisions++;
                jit_arm64_invalidate_all_packed_loads(ctx);
                return true;
        }
        if (ctx->gpr_cache_active && !jit_arm64_gpr_flush_required(ctx))
                return false;
#endif
        dst_ofs = dst * (int)sizeof(struct rt_value);
        src1_ofs = src1 * (int)sizeof(struct rt_value);
        src2_ofs = src2 * (int)sizeof(struct rt_value);
        ASM {
                LDR_W_IMM(REG_X2, REG_X1, (uint32_t)(src1_ofs + 8));
                LDR_W_IMM(REG_X3, REG_X1, (uint32_t)(src2_ofs + 8));
        }
        /* cbz w3,cold -- patched after the inline path is emitted. */
        zero_branch = (uint8_t *)ctx->code;
        if (!jit_put_word(ctx, 0x34000003u))
                return false;
        if (!jit_put_word(ctx, 0x1ac00c00 | (3u << 16) |
                          (2u << 5) | 2u))
                return false;
        ASM { TYPED_STORE(REG_X2, dst_ofs, NOCT_VALUE_INT, true); }
        done_branch = (uint8_t *)ctx->code;
        if (!jit_put_word(ctx, 0x14000000u))
                return false;

        target = (uint8_t *)ctx->code;
        jit_arm64_patch_local_branch(zero_branch, target, true);
        ASM_BINARY_OP(ex_idiv_helper);
        target = (uint8_t *)ctx->code;
        jit_arm64_patch_local_branch(done_branch, target, false);
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
        int dst_ofs;
        int src1_ofs;
        int src2_ofs;
        uint8_t *zero_branch;
        uint8_t *done_branch;
        uint8_t *target;

        CONSUME_TMPVAR(dst);
        CONSUME_TMPVAR(src1);
        CONSUME_TMPVAR(src2);

#if defined(NOCT_USE_OPTIMIZER)
        if (rt_jit_ploop_current_elided(ctx, 7))
                return true;
        if (ctx->packed_loop_hint_active) {
                rt_jit_ploop_remove_index_alias(ctx, dst);
                rt_jit_ploop_remove_base_alias(ctx, dst);
        }
        if (ctx->gpr_cache_active && ctx->gpr_reg_limit != 1 &&
            ctx->gpr_range_valid[src2] != 0 &&
            (ctx->gpr_range_min[src2] > 0 ||
             ctx->gpr_range_max[src2] < -1)) {
                uint32_t r1;
                uint32_t r2;
                uint32_t rd;
                unsigned pins;

                if (!jit_arm64_gpr_get(ctx, src1, 0, &r1))
                        return false;
                pins = 1u << (r1 - REG_X23);
                if (!jit_arm64_gpr_get(ctx, src2, pins, &r2))
                        return false;
                pins |= 1u << (r2 - REG_X23);
                if (!jit_arm64_gpr_is_cached(ctx, src1) &&
                    rt_jit_ploop_next_use_lpc(ctx, src1, ctx->lpc) ==
                    UINT32_MAX) {
                        if (!jit_arm64_gpr_rebind(ctx, dst, src1, &rd))
                                return false;
                } else if (!jit_arm64_gpr_is_cached(ctx, src2) &&
                           rt_jit_ploop_next_use_lpc(ctx, src2, ctx->lpc) ==
                           UINT32_MAX) {
                        if (!jit_arm64_gpr_rebind(ctx, dst, src2, &rd))
                                return false;
                } else if (!jit_arm64_gpr_dest(ctx, dst, pins, &rd)) {
                        return false;
                }
                if (!jit_put_word(ctx, 0x1ac00c00u | (r2 << 16) |
                                         (r1 << 5) | REG_X4) ||
                    !jit_put_word(ctx, 0x1b008000u | (r2 << 16) |
                                         (r1 << 10) | (REG_X4 << 5) | rd))
                        return false;
                ctx->gpr_tmp_dirty[dst] = 1;
                ctx->gpr_range_valid[dst] = 0;
                ctx->gpr_proven_divisions++;
                jit_arm64_invalidate_all_packed_loads(ctx);
                return true;
        }
        if (ctx->gpr_cache_active && !jit_arm64_gpr_flush_required(ctx))
                return false;
#endif
        dst_ofs = dst * (int)sizeof(struct rt_value);
        src1_ofs = src1 * (int)sizeof(struct rt_value);
        src2_ofs = src2 * (int)sizeof(struct rt_value);
        ASM {
                LDR_W_IMM(REG_X2, REG_X1, (uint32_t)(src1_ofs + 8));
                LDR_W_IMM(REG_X3, REG_X1, (uint32_t)(src2_ofs + 8));
        }
        /* cbz w3,cold -- patched after the inline path is emitted. */
        zero_branch = (uint8_t *)ctx->code;
        if (!jit_put_word(ctx, 0x34000003u))
                return false;
        /* sdiv w4,w2,w3; msub w2,w4,w3,w2 */
        if (!jit_put_word(ctx, 0x1ac00c00 | (3u << 16) |
                          (2u << 5) | 4u) ||
            !jit_put_word(ctx, 0x1b008000 | (3u << 16) |
                          (2u << 10) | (4u << 5) | 2u))
                return false;
        ASM { TYPED_STORE(REG_X2, dst_ofs, NOCT_VALUE_INT, true); }
        done_branch = (uint8_t *)ctx->code;
        if (!jit_put_word(ctx, 0x14000000u))
                return false;

        target = (uint8_t *)ctx->code;
        jit_arm64_patch_local_branch(zero_branch, target, true);
        ASM_BINARY_OP(ex_imod_helper);
        target = (uint8_t *)ctx->code;
        jit_arm64_patch_local_branch(done_branch, target, false);
        return true;
}


/*
 * 128-bit SIMD ops (docs/design/06-simd.md): inline NEON, arm64.
 * Logical vreg 0..7 maps to v0..v7 and 8..15 maps to v16..v23.
 * Both ranges are caller-saved under AAPCS64; callee-saved v8..v15
 * are deliberately skipped.  Vector state lives only inside a strip
 * region that emits no calls.  NEON is baseline on AArch64: no
 * feature gate.  NEON ALU is three-address, so there is no
 * two-address copy dance and no vd==vb hazard at all.
 */

static INLINE int
jit_arm64_vreg(int logical)
{
	if (logical < 0 || logical >= 16)
		return -1;
	return logical < 8 ? logical : logical + 8;
}

#if defined(NOCT_USE_OPTIMIZER)
/* Visit vector instructions with NEON or direct scalar lowering. */
/* Visit an OP_VLOADI32X4 instruction. */
static INLINE bool
jit_visit_vloadi32x4_op(
        struct rt_jit_context *ctx)
{
        int vd;
        int base_tmp;
        int ofs_tmp;
        int va;
        uint32_t op_lpc;
        int cursor_id;
        int base;
        int ofs;

        CONSUME_IMM8(vd);
        CONSUME_TMPVAR(base_tmp);
        CONSUME_TMPVAR(ofs_tmp);

        if ((ctx->simd_caps & JIT_SIMD_CAP_NEON) == 0) {
                int lane;
                int base;
                int ofs;

                MOVZ(REG_X5, IMM16((uint32_t)offsetof(struct rt_env, vreg) & 0xffff), LSL_0);
                MOVK(REG_X5, IMM16(((uint32_t)offsetof(struct rt_env, vreg) >> 16) & 0xffff), LSL_16);
                ADD(REG_X5, REG_X0, REG_X5);

                base = base_tmp * (int)sizeof(struct rt_value);
                ofs = ofs_tmp * (int)sizeof(struct rt_value);
                ASM {
                        LDR_IMM(REG_X2, REG_X1, IMM9(base + 8));
                        LDR_W_IMM(REG_X3, REG_X1, (uint32_t)(ofs + 8));
                        LSL_IMM(REG_X3, REG_X3, 2);
                        ADD(REG_X2, REG_X2, REG_X3);
                }
                for (lane = 0; lane < 4; lane++) {
                        LDR_W_IMM(REG_X3, REG_X2, (uint32_t)lane * 4);
                        STR_W_IMM(REG_X3, REG_X5,
                                  (uint32_t)vd * 16 + (uint32_t)lane * 4);
                }
                return true;
        }
        va = jit_arm64_vreg(vd);
        if (va < 0) {
                rt_error(ctx->env, BROKEN_BYTECODE);
                return false;
        }
        op_lpc = ctx->pc_entry[ctx->pc_entry_count - 1].lpc;

        base = base_tmp * (int)sizeof(struct rt_value);
        ofs = ofs_tmp * (int)sizeof(struct rt_value);
        cursor_id = -1;
        if (ctx->vector_hint_active) {
                if (ctx->vector_base_tmp[0] == base_tmp)
                        cursor_id = 0;
                else if (ctx->vector_base_tmp[1] == base_tmp)
                        cursor_id = 1;
        }
        if (cursor_id >= 0) {
                uint32_t rn = cursor_id == 0 ? REG_X19 : REG_X20;

                if (ctx->vector_base_last_lpc[cursor_id] == op_lpc) {
                        /* ldr qA, [xCursor], #16 */
                        if (!jit_put_word(ctx, 0x3cc10400u |
                                         (rn << 5) | (uint32_t)va))
                                return false;
                } else if (!jit_put_word(ctx, 0x3dc00000u |
                                         (rn << 5) | (uint32_t)va)) {
                        return false;
                }
                return true;
        }
        ASM {
                /* x1 = &env->frame->tmpvar[0] */
                LDR_IMM         (REG_X2, REG_X1, IMM9(base + 8));
                LDR_W_IMM       (REG_X3, REG_X1, (uint32_t)(ofs + 8));
                LSL_IMM         (REG_X3, REG_X3, 2);
                ADD             (REG_X2, REG_X2, REG_X3);
        }
        /* ldr qA, [x2] */
        if (!jit_put_word(ctx, 0x3dc00000 | (2u << 5) | (uint32_t)va))
                return false;
        return true;
}

/* Visit an OP_VSTOREI32X4 instruction. */
static INLINE bool
jit_visit_vstorei32x4_op(
        struct rt_jit_context *ctx)
{
        int base_tmp;
        int ofs_tmp;
        int vs;
        int vc;
        uint32_t op_lpc;
        int cursor_id;
        int base;
        int ofs;

        CONSUME_TMPVAR(base_tmp);
        CONSUME_TMPVAR(ofs_tmp);
        CONSUME_IMM8(vs);

        if ((ctx->simd_caps & JIT_SIMD_CAP_NEON) == 0) {
                int lane;
                int base;
                int ofs;

                MOVZ(REG_X5, IMM16((uint32_t)offsetof(struct rt_env, vreg) & 0xffff), LSL_0);
                MOVK(REG_X5, IMM16(((uint32_t)offsetof(struct rt_env, vreg) >> 16) & 0xffff), LSL_16);
                ADD(REG_X5, REG_X0, REG_X5);

                base = base_tmp * (int)sizeof(struct rt_value);
                ofs = ofs_tmp * (int)sizeof(struct rt_value);
                ASM {
                        LDR_IMM(REG_X2, REG_X1, IMM9(base + 8));
                        LDR_W_IMM(REG_X3, REG_X1, (uint32_t)(ofs + 8));
                        LSL_IMM(REG_X3, REG_X3, 2);
                        ADD(REG_X2, REG_X2, REG_X3);
                }
                for (lane = 0; lane < 4; lane++) {
                        LDR_W_IMM(REG_X3, REG_X5,
                                  (uint32_t)vs * 16 + (uint32_t)lane * 4);
                        STR_W_IMM(REG_X3, REG_X2, (uint32_t)lane * 4);
                }
                return true;
        }
        vc = jit_arm64_vreg(vs);
        if (vc < 0) {
                rt_error(ctx->env, BROKEN_BYTECODE);
                return false;
        }
        op_lpc = ctx->pc_entry[ctx->pc_entry_count - 1].lpc;

        base = base_tmp * (int)sizeof(struct rt_value);
        ofs = ofs_tmp * (int)sizeof(struct rt_value);
        cursor_id = -1;
        if (ctx->vector_hint_active) {
                if (ctx->vector_base_tmp[0] == base_tmp)
                        cursor_id = 0;
                else if (ctx->vector_base_tmp[1] == base_tmp)
                        cursor_id = 1;
        }
        if (cursor_id >= 0 &&
            ctx->vector_base_last_lpc[cursor_id] == op_lpc) {
                uint32_t rn = cursor_id == 0 ? REG_X19 : REG_X20;

                /* str qC, [xCursor], #16 */
                if (!jit_put_word(ctx, 0x3c810400u |
                                 (rn << 5) | (uint32_t)vc))
                        return false;
                return true;
        }
        ASM {
                LDR_IMM         (REG_X2, REG_X1, IMM9(base + 8));
                LDR_W_IMM       (REG_X3, REG_X1, (uint32_t)(ofs + 8));
                LSL_IMM         (REG_X3, REG_X3, 2);
                ADD             (REG_X2, REG_X2, REG_X3);
        }
        /* str qC, [x2] */
        if (!jit_put_word(ctx, 0x3d800000 | (2u << 5) | (uint32_t)vc))
                return false;
        return true;
}

/* Visit an OP_VSPLATI32 instruction. */
static INLINE bool
jit_visit_vsplati32_op(
        struct rt_jit_context *ctx)
{
        int vd;
        int src_tmp;
        int va;
        int src;

        CONSUME_IMM8(vd);
        CONSUME_TMPVAR(src_tmp);

        if ((ctx->simd_caps & JIT_SIMD_CAP_NEON) == 0) {
                int lane;
                int src;

                MOVZ(REG_X5, IMM16((uint32_t)offsetof(struct rt_env, vreg) & 0xffff), LSL_0);
                MOVK(REG_X5, IMM16(((uint32_t)offsetof(struct rt_env, vreg) >> 16) & 0xffff), LSL_16);
                ADD(REG_X5, REG_X0, REG_X5);

                src = src_tmp * (int)sizeof(struct rt_value);
                LDR_W_IMM(REG_X3, REG_X1, (uint32_t)(src + 8));
                for (lane = 0; lane < 4; lane++)
                        STR_W_IMM(REG_X3, REG_X5,
                                  (uint32_t)vd * 16 + (uint32_t)lane * 4);
                return true;
        }
        va = jit_arm64_vreg(vd);
        if (va < 0) {
                rt_error(ctx->env, BROKEN_BYTECODE);
                return false;
        }

        src = src_tmp * (int)sizeof(struct rt_value);
        ASM {
                LDR_W_IMM       (REG_X3, REG_X1, (uint32_t)(src + 8));
        }
        /* dup vA.4s, w3 */
        if (!jit_put_word(ctx, 0x4e040c00 | (3u << 5) | (uint32_t)va))
                return false;
        return true;
}

/* Visit an OP_VGETLANEI32 instruction. */
static INLINE bool
jit_visit_vgetlanei32_op(
        struct rt_jit_context *ctx)
{
        int dst_tmp;
        int vs;
        int lane_index;
        int vb;
        int dst;

        CONSUME_TMPVAR(dst_tmp);
        CONSUME_IMM8(vs);
        CONSUME_IMM8(lane_index);

        if ((ctx->simd_caps & JIT_SIMD_CAP_NEON) == 0) {
                int d;

                MOVZ(REG_X5, IMM16((uint32_t)offsetof(struct rt_env, vreg) & 0xffff), LSL_0);
                MOVK(REG_X5, IMM16(((uint32_t)offsetof(struct rt_env, vreg) >> 16) & 0xffff), LSL_16);
                ADD(REG_X5, REG_X0, REG_X5);

                d = dst_tmp * (int)sizeof(struct rt_value);
                LDR_W_IMM(REG_X3, REG_X5,
                          (uint32_t)vs * 16 + (uint32_t)lane_index * 4);
                ASM {
                        MOVZ(REG_X4,
                             IMM16(NOCT_VALUE_INT),
                             LSL_0);
                        STR_IMM(REG_X4, REG_X1, IMM9(d));
                        STR_IMM(REG_X3, REG_X1, IMM9(d + 8));
                }
                return true;
        }
        vb = jit_arm64_vreg(vs);
        if (vb < 0) {
                rt_error(ctx->env, BROKEN_BYTECODE);
                return false;
        }

        dst = dst_tmp * (int)sizeof(struct rt_value);
        /* umov w3, vB.s[lane_index] (imm5 = (lane << 3) | 4) */
        if (!jit_put_word(ctx, 0x0e003c00 |
                          ((((uint32_t)lane_index << 3) | 4u) << 16) |
                          ((uint32_t)vb << 5) | 3u))
                return false;
        ASM {
                /* Tag + full-union value store (w3 is
                   zero-extended into x3 by umov). */
                MOVZ            (REG_X4,
                                  IMM16(NOCT_VALUE_INT),
                                  LSL_0);
                STR_IMM         (REG_X4, REG_X1, IMM9(dst));
                STR_IMM         (REG_X3, REG_X1, IMM9(dst + 8));
        }
        return true;
}

/* Visit an OP_VMOV128 instruction. */
static INLINE bool
jit_visit_vmov128_op(
        struct rt_jit_context *ctx)
{
        int vd;
        int vs;
        int va, vb;

        CONSUME_IMM8(vd);
        CONSUME_IMM8(vs);

        if ((ctx->simd_caps & JIT_SIMD_CAP_NEON) == 0) {
                int lane;

                MOVZ(REG_X5, IMM16((uint32_t)offsetof(struct rt_env, vreg) & 0xffff), LSL_0);
                MOVK(REG_X5, IMM16(((uint32_t)offsetof(struct rt_env, vreg) >> 16) & 0xffff), LSL_16);
                ADD(REG_X5, REG_X0, REG_X5);

                for (lane = 0; lane < 4; lane++) {
                        LDR_W_IMM(REG_X3, REG_X5,
                                  (uint32_t)vs * 16 + (uint32_t)lane * 4);
                        STR_W_IMM(REG_X3, REG_X5,
                                  (uint32_t)vd * 16 + (uint32_t)lane * 4);
                }
                return true;
        }
        va = jit_arm64_vreg(vd);
        vb = jit_arm64_vreg(vs);
        if (va < 0 || vb < 0) {
                rt_error(ctx->env, BROKEN_BYTECODE);
                return false;
        }

        if (va != vb) {
                /* mov vA.16b, vB.16b (orr vA, vB, vB) */
                if (!jit_put_word(ctx, 0x4ea01c00 |
                                  ((uint32_t)vb << 16) |
                                  ((uint32_t)vb << 5) | (uint32_t)va))
                        return false;
        }

        return true;
}

/* Visit an OP_VADDI32X4 instruction. */
static INLINE bool
jit_visit_vaddi32x4_op(
        struct rt_jit_context *ctx)
{
        int vd;
        int lhs;
        int rhs;
        int va, vb, vc;

        CONSUME_IMM8(vd);
        CONSUME_IMM8(lhs);
        CONSUME_IMM8(rhs);

        if ((ctx->simd_caps & JIT_SIMD_CAP_NEON) == 0) {
                int lane;

                MOVZ(REG_X5, IMM16((uint32_t)offsetof(struct rt_env, vreg) & 0xffff), LSL_0);
                MOVK(REG_X5, IMM16(((uint32_t)offsetof(struct rt_env, vreg) >> 16) & 0xffff), LSL_16);
                ADD(REG_X5, REG_X0, REG_X5);

                for (lane = 0; lane < 4; lane++) {
                        uint32_t a;
                        uint32_t b;
                        uint32_t d;

                        a = (uint32_t)lhs * 16 + (uint32_t)lane * 4;
                        b = (uint32_t)rhs * 16 + (uint32_t)lane * 4;
                        d = (uint32_t)vd * 16 + (uint32_t)lane * 4;
                        LDR_W_IMM(REG_X3, REG_X5, a);
                        LDR_W_IMM(REG_X4, REG_X5, b);

                        if (!jit_put_word(ctx, 0x0b000000 | (4u << 16) |
                                          (3u << 5) | 3u)) return false;
                        STR_W_IMM(REG_X3, REG_X5, d);
                }
                return true;
        }
        va = jit_arm64_vreg(vd);
        vb = jit_arm64_vreg(lhs);
        vc = jit_arm64_vreg(rhs);
        if (va < 0 || vb < 0 || vc < 0) {
                rt_error(ctx->env, BROKEN_BYTECODE);
                return false;
        }

        /* add vA.4s, vB.4s, vC.4s */
        if (!jit_put_word(ctx, 0x4ea08400 | ((uint32_t)vc << 16) |
                          ((uint32_t)vb << 5) | (uint32_t)va))
                return false;

        return true;
}

/* Visit an OP_VSUBI32X4 instruction. */
static INLINE bool
jit_visit_vsubi32x4_op(
        struct rt_jit_context *ctx)
{
        int vd;
        int lhs;
        int rhs;
        int va, vb, vc;

        CONSUME_IMM8(vd);
        CONSUME_IMM8(lhs);
        CONSUME_IMM8(rhs);

        if ((ctx->simd_caps & JIT_SIMD_CAP_NEON) == 0) {
                int lane;

                MOVZ(REG_X5, IMM16((uint32_t)offsetof(struct rt_env, vreg) & 0xffff), LSL_0);
                MOVK(REG_X5, IMM16(((uint32_t)offsetof(struct rt_env, vreg) >> 16) & 0xffff), LSL_16);
                ADD(REG_X5, REG_X0, REG_X5);

                for (lane = 0; lane < 4; lane++) {
                        uint32_t a;
                        uint32_t b;
                        uint32_t d;

                        a = (uint32_t)lhs * 16 + (uint32_t)lane * 4;
                        b = (uint32_t)rhs * 16 + (uint32_t)lane * 4;
                        d = (uint32_t)vd * 16 + (uint32_t)lane * 4;
                        LDR_W_IMM(REG_X3, REG_X5, a);
                        LDR_W_IMM(REG_X4, REG_X5, b);

                        if (!jit_put_word(ctx, 0x4b000000 | (4u << 16) |
                                          (3u << 5) | 3u)) return false;
                        STR_W_IMM(REG_X3, REG_X5, d);
                }
                return true;
        }
        va = jit_arm64_vreg(vd);
        vb = jit_arm64_vreg(lhs);
        vc = jit_arm64_vreg(rhs);
        if (va < 0 || vb < 0 || vc < 0) {
                rt_error(ctx->env, BROKEN_BYTECODE);
                return false;
        }

        /* sub vA.4s, vB.4s, vC.4s */
        if (!jit_put_word(ctx, 0x6ea08400 | ((uint32_t)vc << 16) |
                          ((uint32_t)vb << 5) | (uint32_t)va))
                return false;

        return true;
}

/* Visit an OP_VMULI32X4 instruction. */
static INLINE bool
jit_visit_vmuli32x4_op(
        struct rt_jit_context *ctx)
{
        int vd;
        int lhs;
        int rhs;
        int va, vb, vc;

        CONSUME_IMM8(vd);
        CONSUME_IMM8(lhs);
        CONSUME_IMM8(rhs);

        if ((ctx->simd_caps & JIT_SIMD_CAP_NEON) == 0) {
                int lane;

                MOVZ(REG_X5, IMM16((uint32_t)offsetof(struct rt_env, vreg) & 0xffff), LSL_0);
                MOVK(REG_X5, IMM16(((uint32_t)offsetof(struct rt_env, vreg) >> 16) & 0xffff), LSL_16);
                ADD(REG_X5, REG_X0, REG_X5);

                for (lane = 0; lane < 4; lane++) {
                        uint32_t a;
                        uint32_t b;
                        uint32_t d;

                        a = (uint32_t)lhs * 16 + (uint32_t)lane * 4;
                        b = (uint32_t)rhs * 16 + (uint32_t)lane * 4;
                        d = (uint32_t)vd * 16 + (uint32_t)lane * 4;
                        LDR_W_IMM(REG_X3, REG_X5, a);
                        LDR_W_IMM(REG_X4, REG_X5, b);

                        if (!jit_put_word(ctx, 0x1b007c00 | (4u << 16) |
                                          (3u << 5) | 3u)) return false;
                        STR_W_IMM(REG_X3, REG_X5, d);
                }
                return true;
        }
        va = jit_arm64_vreg(vd);
        vb = jit_arm64_vreg(lhs);
        vc = jit_arm64_vreg(rhs);
        if (va < 0 || vb < 0 || vc < 0) {
                rt_error(ctx->env, BROKEN_BYTECODE);
                return false;
        }

        /* mul vA.4s, vB.4s, vC.4s */
        if (!jit_put_word(ctx, 0x4ea09c00 | ((uint32_t)vc << 16) |
                          ((uint32_t)vb << 5) | (uint32_t)va))
                return false;

        return true;
}

/* Visit an OP_VAND128 instruction. */
static INLINE bool
jit_visit_vand128_op(
        struct rt_jit_context *ctx)
{
        int vd;
        int lhs;
        int rhs;
        int va, vb, vc;

        CONSUME_IMM8(vd);
        CONSUME_IMM8(lhs);
        CONSUME_IMM8(rhs);

        if ((ctx->simd_caps & JIT_SIMD_CAP_NEON) == 0) {
                int lane;

                MOVZ(REG_X5, IMM16((uint32_t)offsetof(struct rt_env, vreg) & 0xffff), LSL_0);
                MOVK(REG_X5, IMM16(((uint32_t)offsetof(struct rt_env, vreg) >> 16) & 0xffff), LSL_16);
                ADD(REG_X5, REG_X0, REG_X5);

                for (lane = 0; lane < 4; lane++) {
                        uint32_t a;
                        uint32_t b;
                        uint32_t d;

                        a = (uint32_t)lhs * 16 + (uint32_t)lane * 4;
                        b = (uint32_t)rhs * 16 + (uint32_t)lane * 4;
                        d = (uint32_t)vd * 16 + (uint32_t)lane * 4;
                        LDR_W_IMM(REG_X3, REG_X5, a);
                        LDR_W_IMM(REG_X4, REG_X5, b);

                        if (!jit_put_word(ctx, 0x0a000000 | (4u << 16) |
                                          (3u << 5) | 3u)) return false;
                        STR_W_IMM(REG_X3, REG_X5, d);
                }
                return true;
        }
        va = jit_arm64_vreg(vd);
        vb = jit_arm64_vreg(lhs);
        vc = jit_arm64_vreg(rhs);
        if (va < 0 || vb < 0 || vc < 0) {
                rt_error(ctx->env, BROKEN_BYTECODE);
                return false;
        }

        /* and vA.16b, vB.16b, vC.16b */
        if (!jit_put_word(ctx, 0x4e201c00 | ((uint32_t)vc << 16) |
                          ((uint32_t)vb << 5) | (uint32_t)va))
                return false;

        return true;
}

/* Visit an OP_VOR128 instruction. */
static INLINE bool
jit_visit_vor128_op(
        struct rt_jit_context *ctx)
{
        int vd;
        int lhs;
        int rhs;
        int va, vb, vc;

        CONSUME_IMM8(vd);
        CONSUME_IMM8(lhs);
        CONSUME_IMM8(rhs);

        if ((ctx->simd_caps & JIT_SIMD_CAP_NEON) == 0) {
                int lane;

                MOVZ(REG_X5, IMM16((uint32_t)offsetof(struct rt_env, vreg) & 0xffff), LSL_0);
                MOVK(REG_X5, IMM16(((uint32_t)offsetof(struct rt_env, vreg) >> 16) & 0xffff), LSL_16);
                ADD(REG_X5, REG_X0, REG_X5);

                for (lane = 0; lane < 4; lane++) {
                        uint32_t a;
                        uint32_t b;
                        uint32_t d;

                        a = (uint32_t)lhs * 16 + (uint32_t)lane * 4;
                        b = (uint32_t)rhs * 16 + (uint32_t)lane * 4;
                        d = (uint32_t)vd * 16 + (uint32_t)lane * 4;
                        LDR_W_IMM(REG_X3, REG_X5, a);
                        LDR_W_IMM(REG_X4, REG_X5, b);

                        if (!jit_put_word(ctx, 0x2a000000 | (4u << 16) |
                                          (3u << 5) | 3u)) return false;
                        STR_W_IMM(REG_X3, REG_X5, d);
                }
                return true;
        }
        va = jit_arm64_vreg(vd);
        vb = jit_arm64_vreg(lhs);
        vc = jit_arm64_vreg(rhs);
        if (va < 0 || vb < 0 || vc < 0) {
                rt_error(ctx->env, BROKEN_BYTECODE);
                return false;
        }

        /* orr vA.16b, vB.16b, vC.16b */
        if (!jit_put_word(ctx, 0x4ea01c00 | ((uint32_t)vc << 16) |
                          ((uint32_t)vb << 5) | (uint32_t)va))
                return false;

        return true;
}

/* Visit an OP_VXOR128 instruction. */
static INLINE bool
jit_visit_vxor128_op(
        struct rt_jit_context *ctx)
{
        int vd;
        int lhs;
        int rhs;
        int va, vb, vc;

        CONSUME_IMM8(vd);
        CONSUME_IMM8(lhs);
        CONSUME_IMM8(rhs);

        if ((ctx->simd_caps & JIT_SIMD_CAP_NEON) == 0) {
                int lane;

                MOVZ(REG_X5, IMM16((uint32_t)offsetof(struct rt_env, vreg) & 0xffff), LSL_0);
                MOVK(REG_X5, IMM16(((uint32_t)offsetof(struct rt_env, vreg) >> 16) & 0xffff), LSL_16);
                ADD(REG_X5, REG_X0, REG_X5);

                for (lane = 0; lane < 4; lane++) {
                        uint32_t a;
                        uint32_t b;
                        uint32_t d;

                        a = (uint32_t)lhs * 16 + (uint32_t)lane * 4;
                        b = (uint32_t)rhs * 16 + (uint32_t)lane * 4;
                        d = (uint32_t)vd * 16 + (uint32_t)lane * 4;
                        LDR_W_IMM(REG_X3, REG_X5, a);
                        LDR_W_IMM(REG_X4, REG_X5, b);

                        if (!jit_put_word(ctx, 0x4a000000 | (4u << 16) |
                                          (3u << 5) | 3u)) return false;
                        STR_W_IMM(REG_X3, REG_X5, d);
                }
                return true;
        }
        va = jit_arm64_vreg(vd);
        vb = jit_arm64_vreg(lhs);
        vc = jit_arm64_vreg(rhs);
        if (va < 0 || vb < 0 || vc < 0) {
                rt_error(ctx->env, BROKEN_BYTECODE);
                return false;
        }

        /* eor vA.16b, vB.16b, vC.16b */
        if (!jit_put_word(ctx, 0x6e201c00 | ((uint32_t)vc << 16) |
                          ((uint32_t)vb << 5) | (uint32_t)va))
                return false;

        return true;
}

/* Visit an OP_VSHLI32X4 instruction. */
static INLINE bool
jit_visit_vshli32x4_op(
        struct rt_jit_context *ctx)
{
        int vd;
        int vs;
        int shift;
        int va, vb;

        CONSUME_IMM8(vd);
        CONSUME_IMM8(vs);
        CONSUME_IMM8(shift);

        if ((ctx->simd_caps & JIT_SIMD_CAP_NEON) == 0) {
                int lane;

                MOVZ(REG_X5, IMM16((uint32_t)offsetof(struct rt_env, vreg) & 0xffff), LSL_0);
                MOVK(REG_X5, IMM16(((uint32_t)offsetof(struct rt_env, vreg) >> 16) & 0xffff), LSL_16);
                ADD(REG_X5, REG_X0, REG_X5);

                for (lane = 0; lane < 4; lane++) {
                        uint32_t s;
                        uint32_t d;
                        uint32_t word;

                        s = (uint32_t)vs * 16 + (uint32_t)lane * 4;
                        d = (uint32_t)vd * 16 + (uint32_t)lane * 4;
                        LDR_W_IMM(REG_X3, REG_X5, s);
                        word = 0x53000000 |
                                       (((32u - (uint32_t)shift) & 31u) << 16) |
                                       ((31u - (uint32_t)shift) << 10) |
                                       (3u << 5) | 3u;
                        if (!jit_put_word(ctx, word))
                                return false;
                        STR_W_IMM(REG_X3, REG_X5, d);
                }
                return true;
        }
        va = jit_arm64_vreg(vd);
        vb = jit_arm64_vreg(vs);
        if (va < 0 || vb < 0) {
                rt_error(ctx->env, BROKEN_BYTECODE);
                return false;
        }

        /* shl vA.4s, vB.4s, #shift (immh:immb = 32 + shift) */
        if (!jit_put_word(ctx, 0x4f005400 |
                          ((32u + (uint32_t)shift) << 16) |
                          ((uint32_t)vb << 5) | (uint32_t)va))
                return false;

        return true;
}

/* Visit an OP_VSHRI32X4 instruction. */
static INLINE bool
jit_visit_vshri32x4_op(
        struct rt_jit_context *ctx)
{
        int vd;
        int vs;
        int shift;
        int va, vb;

        CONSUME_IMM8(vd);
        CONSUME_IMM8(vs);
        CONSUME_IMM8(shift);

        if ((ctx->simd_caps & JIT_SIMD_CAP_NEON) == 0) {
                int lane;

                MOVZ(REG_X5, IMM16((uint32_t)offsetof(struct rt_env, vreg) & 0xffff), LSL_0);
                MOVK(REG_X5, IMM16(((uint32_t)offsetof(struct rt_env, vreg) >> 16) & 0xffff), LSL_16);
                ADD(REG_X5, REG_X0, REG_X5);

                for (lane = 0; lane < 4; lane++) {
                        uint32_t s;
                        uint32_t d;
                        uint32_t word;

                        s = (uint32_t)vs * 16 + (uint32_t)lane * 4;
                        d = (uint32_t)vd * 16 + (uint32_t)lane * 4;
                        LDR_W_IMM(REG_X3, REG_X5, s);
                        word = 0x53000000 | ((uint32_t)shift << 16) |
                                       (31u << 10) | (3u << 5) | 3u;
                        if (!jit_put_word(ctx, word))
                                return false;
                        STR_W_IMM(REG_X3, REG_X5, d);
                }
                return true;
        }
        va = jit_arm64_vreg(vd);
        vb = jit_arm64_vreg(vs);
        if (va < 0 || vb < 0) {
                rt_error(ctx->env, BROKEN_BYTECODE);
                return false;
        }

        /* ushr vA.4s, vB.4s, #shift (LOGICAL; immh:immb = 64 - shift;
           the LIR layer never emits shift == 0) */
        if (!jit_put_word(ctx, 0x6f000400 |
                          ((64u - (uint32_t)shift) << 16) |
                          ((uint32_t)vb << 5) | (uint32_t)va))
                return false;

        return true;
}

/* Visit an OP_VLOADF32X4 instruction. */
static INLINE bool
jit_visit_vloadf32x4_op(
        struct rt_jit_context *ctx)
{
        int vd;
        int base_tmp;
        int ofs_tmp;
        int va;
        uint32_t op_lpc;
        int cursor_id;
        int base;
        int ofs;

        CONSUME_IMM8(vd);
        CONSUME_TMPVAR(base_tmp);
        CONSUME_TMPVAR(ofs_tmp);

        if ((ctx->simd_caps & JIT_SIMD_CAP_NEON) == 0) {
                int lane;
                int base;
                int ofs;

                MOVZ(REG_X5, IMM16((uint32_t)offsetof(struct rt_env, vreg) & 0xffff), LSL_0);
                MOVK(REG_X5, IMM16(((uint32_t)offsetof(struct rt_env, vreg) >> 16) & 0xffff), LSL_16);
                ADD(REG_X5, REG_X0, REG_X5);

                base = base_tmp * (int)sizeof(struct rt_value);
                ofs = ofs_tmp * (int)sizeof(struct rt_value);
                ASM {
                        LDR_IMM(REG_X2, REG_X1, IMM9(base + 8));
                        LDR_W_IMM(REG_X3, REG_X1, (uint32_t)(ofs + 8));
                        LSL_IMM(REG_X3, REG_X3, 2);
                        ADD(REG_X2, REG_X2, REG_X3);
                }
                for (lane = 0; lane < 4; lane++) {
                        LDR_W_IMM(REG_X3, REG_X2, (uint32_t)lane * 4);
                        STR_W_IMM(REG_X3, REG_X5,
                                  (uint32_t)vd * 16 + (uint32_t)lane * 4);
                }
                return true;
        }
        va = jit_arm64_vreg(vd);
        if (va < 0) {
                rt_error(ctx->env, BROKEN_BYTECODE);
                return false;
        }
        op_lpc = ctx->pc_entry[ctx->pc_entry_count - 1].lpc;

        base = base_tmp * (int)sizeof(struct rt_value);
        ofs = ofs_tmp * (int)sizeof(struct rt_value);
        cursor_id = -1;
        if (ctx->vector_hint_active) {
                if (ctx->vector_base_tmp[0] == base_tmp)
                        cursor_id = 0;
                else if (ctx->vector_base_tmp[1] == base_tmp)
                        cursor_id = 1;
        }
        if (cursor_id >= 0) {
                uint32_t rn = cursor_id == 0 ? REG_X19 : REG_X20;

                if (ctx->vector_base_last_lpc[cursor_id] == op_lpc) {
                        /* ldr qA, [xCursor], #16 */
                        if (!jit_put_word(ctx, 0x3cc10400u |
                                         (rn << 5) | (uint32_t)va))
                                return false;
                } else if (!jit_put_word(ctx, 0x3dc00000u |
                                         (rn << 5) | (uint32_t)va)) {
                        return false;
                }
                return true;
        }
        ASM {
                /* x1 = &env->frame->tmpvar[0] */
                LDR_IMM         (REG_X2, REG_X1, IMM9(base + 8));
                LDR_W_IMM       (REG_X3, REG_X1, (uint32_t)(ofs + 8));
                LSL_IMM         (REG_X3, REG_X3, 2);
                ADD             (REG_X2, REG_X2, REG_X3);
        }
        /* ldr qA, [x2] */
        if (!jit_put_word(ctx, 0x3dc00000 | (2u << 5) | (uint32_t)va))
                return false;
        return true;
}

/* Visit an OP_VSTOREF32X4 instruction. */
static INLINE bool
jit_visit_vstoref32x4_op(
        struct rt_jit_context *ctx)
{
        int base_tmp;
        int ofs_tmp;
        int vs;
        int vc;
        uint32_t op_lpc;
        int cursor_id;
        int base;
        int ofs;

        CONSUME_TMPVAR(base_tmp);
        CONSUME_TMPVAR(ofs_tmp);
        CONSUME_IMM8(vs);

        if ((ctx->simd_caps & JIT_SIMD_CAP_NEON) == 0) {
                int lane;
                int base;
                int ofs;

                MOVZ(REG_X5, IMM16((uint32_t)offsetof(struct rt_env, vreg) & 0xffff), LSL_0);
                MOVK(REG_X5, IMM16(((uint32_t)offsetof(struct rt_env, vreg) >> 16) & 0xffff), LSL_16);
                ADD(REG_X5, REG_X0, REG_X5);

                base = base_tmp * (int)sizeof(struct rt_value);
                ofs = ofs_tmp * (int)sizeof(struct rt_value);
                ASM {
                        LDR_IMM(REG_X2, REG_X1, IMM9(base + 8));
                        LDR_W_IMM(REG_X3, REG_X1, (uint32_t)(ofs + 8));
                        LSL_IMM(REG_X3, REG_X3, 2);
                        ADD(REG_X2, REG_X2, REG_X3);
                }
                for (lane = 0; lane < 4; lane++) {
                        LDR_W_IMM(REG_X3, REG_X5,
                                  (uint32_t)vs * 16 + (uint32_t)lane * 4);
                        STR_W_IMM(REG_X3, REG_X2, (uint32_t)lane * 4);
                }
                return true;
        }
        vc = jit_arm64_vreg(vs);
        if (vc < 0) {
                rt_error(ctx->env, BROKEN_BYTECODE);
                return false;
        }
        op_lpc = ctx->pc_entry[ctx->pc_entry_count - 1].lpc;

        base = base_tmp * (int)sizeof(struct rt_value);
        ofs = ofs_tmp * (int)sizeof(struct rt_value);
        cursor_id = -1;
        if (ctx->vector_hint_active) {
                if (ctx->vector_base_tmp[0] == base_tmp)
                        cursor_id = 0;
                else if (ctx->vector_base_tmp[1] == base_tmp)
                        cursor_id = 1;
        }
        if (cursor_id >= 0 &&
            ctx->vector_base_last_lpc[cursor_id] == op_lpc) {
                uint32_t rn = cursor_id == 0 ? REG_X19 : REG_X20;

                /* str qC, [xCursor], #16 */
                if (!jit_put_word(ctx, 0x3c810400u |
                                 (rn << 5) | (uint32_t)vc))
                        return false;
                return true;
        }
        ASM {
                LDR_IMM         (REG_X2, REG_X1, IMM9(base + 8));
                LDR_W_IMM       (REG_X3, REG_X1, (uint32_t)(ofs + 8));
                LSL_IMM         (REG_X3, REG_X3, 2);
                ADD             (REG_X2, REG_X2, REG_X3);
        }
        /* str qC, [x2] */
        if (!jit_put_word(ctx, 0x3d800000 | (2u << 5) | (uint32_t)vc))
                return false;
        return true;
}

/* Visit an OP_VSPLATF32 instruction. */
static INLINE bool
jit_visit_vsplatf32_op(
        struct rt_jit_context *ctx)
{
        int vd;
        int src_tmp;
        int va;
        int src;

        CONSUME_IMM8(vd);
        CONSUME_TMPVAR(src_tmp);

        if ((ctx->simd_caps & JIT_SIMD_CAP_NEON) == 0) {
                int lane;
                int src;

                MOVZ(REG_X5, IMM16((uint32_t)offsetof(struct rt_env, vreg) & 0xffff), LSL_0);
                MOVK(REG_X5, IMM16(((uint32_t)offsetof(struct rt_env, vreg) >> 16) & 0xffff), LSL_16);
                ADD(REG_X5, REG_X0, REG_X5);

                src = src_tmp * (int)sizeof(struct rt_value);
                LDR_W_IMM(REG_X3, REG_X1, (uint32_t)(src + 8));
                for (lane = 0; lane < 4; lane++)
                        STR_W_IMM(REG_X3, REG_X5,
                                  (uint32_t)vd * 16 + (uint32_t)lane * 4);
                return true;
        }
        va = jit_arm64_vreg(vd);
        if (va < 0) {
                rt_error(ctx->env, BROKEN_BYTECODE);
                return false;
        }

        src = src_tmp * (int)sizeof(struct rt_value);
        ASM {
                LDR_W_IMM       (REG_X3, REG_X1, (uint32_t)(src + 8));
        }
        /* dup vA.4s, w3 */
        if (!jit_put_word(ctx, 0x4e040c00 | (3u << 5) | (uint32_t)va))
                return false;
        return true;
}

/* Visit an OP_VGETLANEF32 instruction. */
static INLINE bool
jit_visit_vgetlanef32_op(
        struct rt_jit_context *ctx)
{
        int dst_tmp;
        int vs;
        int lane_index;
        int vb;
        int dst;

        CONSUME_TMPVAR(dst_tmp);
        CONSUME_IMM8(vs);
        CONSUME_IMM8(lane_index);

        if ((ctx->simd_caps & JIT_SIMD_CAP_NEON) == 0) {
                int d;

                MOVZ(REG_X5, IMM16((uint32_t)offsetof(struct rt_env, vreg) & 0xffff), LSL_0);
                MOVK(REG_X5, IMM16(((uint32_t)offsetof(struct rt_env, vreg) >> 16) & 0xffff), LSL_16);
                ADD(REG_X5, REG_X0, REG_X5);

                d = dst_tmp * (int)sizeof(struct rt_value);
                LDR_W_IMM(REG_X3, REG_X5,
                          (uint32_t)vs * 16 + (uint32_t)lane_index * 4);
                ASM {
                        MOVZ(REG_X4,
                             IMM16(NOCT_VALUE_FLOAT),
                             LSL_0);
                        STR_IMM(REG_X4, REG_X1, IMM9(d));
                        STR_IMM(REG_X3, REG_X1, IMM9(d + 8));
                }
                return true;
        }
        vb = jit_arm64_vreg(vs);
        if (vb < 0) {
                rt_error(ctx->env, BROKEN_BYTECODE);
                return false;
        }

        dst = dst_tmp * (int)sizeof(struct rt_value);
        /* umov w3, vB.s[lane_index] (imm5 = (lane << 3) | 4) */
        if (!jit_put_word(ctx, 0x0e003c00 |
                          ((((uint32_t)lane_index << 3) | 4u) << 16) |
                          ((uint32_t)vb << 5) | 3u))
                return false;
        ASM {
                /* Tag + full-union value store (w3 is
                   zero-extended into x3 by umov). */
                MOVZ            (REG_X4,
                                  IMM16(NOCT_VALUE_FLOAT),
                                  LSL_0);
                STR_IMM         (REG_X4, REG_X1, IMM9(dst));
                STR_IMM         (REG_X3, REG_X1, IMM9(dst + 8));
        }
        return true;
}

/* Visit an OP_VADDF32X4 instruction. */
static INLINE bool
jit_visit_vaddf32x4_op(
        struct rt_jit_context *ctx)
{
        int vd;
        int lhs;
        int rhs;
        int va, vb, vc;

        CONSUME_IMM8(vd);
        CONSUME_IMM8(lhs);
        CONSUME_IMM8(rhs);

        if ((ctx->simd_caps & JIT_SIMD_CAP_NEON) == 0) {
                int lane;

                MOVZ(REG_X5, IMM16((uint32_t)offsetof(struct rt_env, vreg) & 0xffff), LSL_0);
                MOVK(REG_X5, IMM16(((uint32_t)offsetof(struct rt_env, vreg) >> 16) & 0xffff), LSL_16);
                ADD(REG_X5, REG_X0, REG_X5);

                for (lane = 0; lane < 4; lane++) {
                        uint32_t a;
                        uint32_t b;
                        uint32_t d;
                        uint32_t base;

                        a = (uint32_t)lhs * 16 + (uint32_t)lane * 4;
                        b = (uint32_t)rhs * 16 + (uint32_t)lane * 4;
                        d = (uint32_t)vd * 16 + (uint32_t)lane * 4;
                        /* ldr s0,[x5,#a]; ldr s1,[x5,#b] */
                        if (!jit_put_word(ctx, 0xbd400000 | ((a / 4) << 10) |
                                                  (REG_X5 << 5)) ||
                            !jit_put_word(ctx, 0xbd400001 | ((b / 4) << 10) |
                                                  (REG_X5 << 5)))
                                return false;
                        base = 0x1e202800;
                        if (!jit_put_word(ctx, base | (1u << 16)) ||
                            !jit_put_word(ctx, 0xbd000000 | ((d / 4) << 10) |
                                                  (REG_X5 << 5)))
                                return false;
                }
                return true;
        }
        va = jit_arm64_vreg(vd);
        vb = jit_arm64_vreg(lhs);
        vc = jit_arm64_vreg(rhs);
        if (va < 0 || vb < 0 || vc < 0) {
                rt_error(ctx->env, BROKEN_BYTECODE);
                return false;
        }

        /* fadd vA.4s, vB.4s, vC.4s */
        if (!jit_put_word(ctx, 0x4e20d400 | ((uint32_t)vc << 16) |
                          ((uint32_t)vb << 5) | (uint32_t)va))
                return false;

        return true;
}

/* Visit an OP_VSUBF32X4 instruction. */
static INLINE bool
jit_visit_vsubf32x4_op(
        struct rt_jit_context *ctx)
{
        int vd;
        int lhs;
        int rhs;
        int va, vb, vc;

        CONSUME_IMM8(vd);
        CONSUME_IMM8(lhs);
        CONSUME_IMM8(rhs);

        if ((ctx->simd_caps & JIT_SIMD_CAP_NEON) == 0) {
                int lane;

                MOVZ(REG_X5, IMM16((uint32_t)offsetof(struct rt_env, vreg) & 0xffff), LSL_0);
                MOVK(REG_X5, IMM16(((uint32_t)offsetof(struct rt_env, vreg) >> 16) & 0xffff), LSL_16);
                ADD(REG_X5, REG_X0, REG_X5);

                for (lane = 0; lane < 4; lane++) {
                        uint32_t a;
                        uint32_t b;
                        uint32_t d;
                        uint32_t base;

                        a = (uint32_t)lhs * 16 + (uint32_t)lane * 4;
                        b = (uint32_t)rhs * 16 + (uint32_t)lane * 4;
                        d = (uint32_t)vd * 16 + (uint32_t)lane * 4;
                        /* ldr s0,[x5,#a]; ldr s1,[x5,#b] */
                        if (!jit_put_word(ctx, 0xbd400000 | ((a / 4) << 10) |
                                                  (REG_X5 << 5)) ||
                            !jit_put_word(ctx, 0xbd400001 | ((b / 4) << 10) |
                                                  (REG_X5 << 5)))
                                return false;
                        base = 0x1e203800;
                        if (!jit_put_word(ctx, base | (1u << 16)) ||
                            !jit_put_word(ctx, 0xbd000000 | ((d / 4) << 10) |
                                                  (REG_X5 << 5)))
                                return false;
                }
                return true;
        }
        va = jit_arm64_vreg(vd);
        vb = jit_arm64_vreg(lhs);
        vc = jit_arm64_vreg(rhs);
        if (va < 0 || vb < 0 || vc < 0) {
                rt_error(ctx->env, BROKEN_BYTECODE);
                return false;
        }

        /* fsub vA.4s, vB.4s, vC.4s */
        if (!jit_put_word(ctx, 0x4ea0d400 | ((uint32_t)vc << 16) |
                          ((uint32_t)vb << 5) | (uint32_t)va))
                return false;

        return true;
}

/* Visit an OP_VMULF32X4 instruction. */
static INLINE bool
jit_visit_vmulf32x4_op(
        struct rt_jit_context *ctx)
{
        int vd;
        int lhs;
        int rhs;
        int va, vb, vc;

        CONSUME_IMM8(vd);
        CONSUME_IMM8(lhs);
        CONSUME_IMM8(rhs);

        if ((ctx->simd_caps & JIT_SIMD_CAP_NEON) == 0) {
                int lane;

                MOVZ(REG_X5, IMM16((uint32_t)offsetof(struct rt_env, vreg) & 0xffff), LSL_0);
                MOVK(REG_X5, IMM16(((uint32_t)offsetof(struct rt_env, vreg) >> 16) & 0xffff), LSL_16);
                ADD(REG_X5, REG_X0, REG_X5);

                for (lane = 0; lane < 4; lane++) {
                        uint32_t a;
                        uint32_t b;
                        uint32_t d;
                        uint32_t base;

                        a = (uint32_t)lhs * 16 + (uint32_t)lane * 4;
                        b = (uint32_t)rhs * 16 + (uint32_t)lane * 4;
                        d = (uint32_t)vd * 16 + (uint32_t)lane * 4;
                        /* ldr s0,[x5,#a]; ldr s1,[x5,#b] */
                        if (!jit_put_word(ctx, 0xbd400000 | ((a / 4) << 10) |
                                                  (REG_X5 << 5)) ||
                            !jit_put_word(ctx, 0xbd400001 | ((b / 4) << 10) |
                                                  (REG_X5 << 5)))
                                return false;
                        base = 0x1e200800;
                        if (!jit_put_word(ctx, base | (1u << 16)) ||
                            !jit_put_word(ctx, 0xbd000000 | ((d / 4) << 10) |
                                                  (REG_X5 << 5)))
                                return false;
                }
                return true;
        }
        va = jit_arm64_vreg(vd);
        vb = jit_arm64_vreg(lhs);
        vc = jit_arm64_vreg(rhs);
        if (va < 0 || vb < 0 || vc < 0) {
                rt_error(ctx->env, BROKEN_BYTECODE);
                return false;
        }

        /* fmul vA.4s, vB.4s, vC.4s */
        if (!jit_put_word(ctx, 0x6e20dc00 | ((uint32_t)vc << 16) |
                          ((uint32_t)vb << 5) | (uint32_t)va))
                return false;

        return true;
}

/* Visit an OP_VDIVF32X4 instruction. */
static INLINE bool
jit_visit_vdivf32x4_op(
        struct rt_jit_context *ctx)
{
        int vd;
        int lhs;
        int rhs;
        int va, vb, vc;

        CONSUME_IMM8(vd);
        CONSUME_IMM8(lhs);
        CONSUME_IMM8(rhs);

        if ((ctx->simd_caps & JIT_SIMD_CAP_NEON) == 0) {
                int lane;

                MOVZ(REG_X5, IMM16((uint32_t)offsetof(struct rt_env, vreg) & 0xffff), LSL_0);
                MOVK(REG_X5, IMM16(((uint32_t)offsetof(struct rt_env, vreg) >> 16) & 0xffff), LSL_16);
                ADD(REG_X5, REG_X0, REG_X5);

                for (lane = 0; lane < 4; lane++) {
                        uint32_t a;
                        uint32_t b;
                        uint32_t d;
                        uint32_t base;

                        a = (uint32_t)lhs * 16 + (uint32_t)lane * 4;
                        b = (uint32_t)rhs * 16 + (uint32_t)lane * 4;
                        d = (uint32_t)vd * 16 + (uint32_t)lane * 4;
                        /* ldr s0,[x5,#a]; ldr s1,[x5,#b] */
                        if (!jit_put_word(ctx, 0xbd400000 | ((a / 4) << 10) |
                                                  (REG_X5 << 5)) ||
                            !jit_put_word(ctx, 0xbd400001 | ((b / 4) << 10) |
                                                  (REG_X5 << 5)))
                                return false;
                        base = 0x1e201800;
                        if (!jit_put_word(ctx, base | (1u << 16)) ||
                            !jit_put_word(ctx, 0xbd000000 | ((d / 4) << 10) |
                                                  (REG_X5 << 5)))
                                return false;
                }
                return true;
        }
        va = jit_arm64_vreg(vd);
        vb = jit_arm64_vreg(lhs);
        vc = jit_arm64_vreg(rhs);
        if (va < 0 || vb < 0 || vc < 0) {
                rt_error(ctx->env, BROKEN_BYTECODE);
                return false;
        }

        /* fdiv vA.4s, vB.4s, vC.4s */
        if (!jit_put_word(ctx, 0x6e20fc00 | ((uint32_t)vc << 16) |
                          ((uint32_t)vb << 5) | (uint32_t)va))
                return false;

        return true;
}

/* Visit an OP_VCVTI32F32X4 instruction. */
static INLINE bool
jit_visit_vcvti32f32x4_op(
        struct rt_jit_context *ctx)
{
        int vd;
        int vs;
        int va, vb;

        CONSUME_IMM8(vd);
        CONSUME_IMM8(vs);

        if ((ctx->simd_caps & JIT_SIMD_CAP_NEON) == 0) {
                int lane;

                MOVZ(REG_X5, IMM16((uint32_t)offsetof(struct rt_env, vreg) & 0xffff), LSL_0);
                MOVK(REG_X5, IMM16(((uint32_t)offsetof(struct rt_env, vreg) >> 16) & 0xffff), LSL_16);
                ADD(REG_X5, REG_X0, REG_X5);

                for (lane = 0; lane < 4; lane++) {
                        uint32_t s;
                        uint32_t d;

                        s = (uint32_t)vs * 16 + (uint32_t)lane * 4;
                        d = (uint32_t)vd * 16 + (uint32_t)lane * 4;
                        LDR_W_IMM(REG_X3, REG_X5, s);
                        if (!jit_put_word(ctx, 0x1e220000 | (3u << 5)) ||
                            !jit_put_word(ctx, 0xbd000000 | ((d / 4) << 10) |
                                              (REG_X5 << 5)))
                                return false;
                }
                return true;
        }
        va = jit_arm64_vreg(vd);
        vb = jit_arm64_vreg(vs);
        if (va < 0 || vb < 0) {
                rt_error(ctx->env, BROKEN_BYTECODE);
                return false;
        }

        if (!jit_put_word(ctx, 0x4e21d800 |
                          ((uint32_t)vb << 5) | (uint32_t)va))
                return false;

        return true;
}

/* Visit an OP_VCVTF32I32X4 instruction. */
static INLINE bool
jit_visit_vcvtf32i32x4_op(
        struct rt_jit_context *ctx)
{
        int vd;
        int vs;
        int va, vb;

        CONSUME_IMM8(vd);
        CONSUME_IMM8(vs);

        if ((ctx->simd_caps & JIT_SIMD_CAP_NEON) == 0) {
                int lane;

                MOVZ(REG_X5, IMM16((uint32_t)offsetof(struct rt_env, vreg) & 0xffff), LSL_0);
                MOVK(REG_X5, IMM16(((uint32_t)offsetof(struct rt_env, vreg) >> 16) & 0xffff), LSL_16);
                ADD(REG_X5, REG_X0, REG_X5);

                for (lane = 0; lane < 4; lane++) {
                        uint32_t s;
                        uint32_t d;

                        s = (uint32_t)vs * 16 + (uint32_t)lane * 4;
                        d = (uint32_t)vd * 16 + (uint32_t)lane * 4;
                        if (!jit_put_word(ctx, 0xbd400000 | ((s / 4) << 10) |
                                              (REG_X5 << 5)) ||
                            !jit_put_word(ctx, 0x1e380003))
                                return false;
                        STR_W_IMM(REG_X3, REG_X5, d);
                }
                return true;
        }
        va = jit_arm64_vreg(vd);
        vb = jit_arm64_vreg(vs);
        if (va < 0 || vb < 0) {
                rt_error(ctx->env, BROKEN_BYTECODE);
                return false;
        }

        if (!jit_put_word(ctx, 0x4ea1b800 |
                          ((uint32_t)vb << 5) | (uint32_t)va))
                return false;

        return true;
}

/* Visit an OP_VMINS32X4 instruction. */
static INLINE bool
jit_visit_vmins32x4_op(
        struct rt_jit_context *ctx)
{
        int vd;
        int lhs;
        int rhs;
        int va, vb, vc;

        CONSUME_IMM8(vd);
        CONSUME_IMM8(lhs);
        CONSUME_IMM8(rhs);

        if ((ctx->simd_caps & JIT_SIMD_CAP_NEON) == 0) {
                int lane;

                MOVZ(REG_X5, IMM16((uint32_t)offsetof(struct rt_env, vreg) & 0xffff), LSL_0);
                MOVK(REG_X5, IMM16(((uint32_t)offsetof(struct rt_env, vreg) >> 16) & 0xffff), LSL_16);
                ADD(REG_X5, REG_X0, REG_X5);

                for (lane = 0; lane < 4; lane++) {
                        uint32_t a;
                        uint32_t b;
                        uint32_t d;

                        a = (uint32_t)lhs * 16 + (uint32_t)lane * 4;
                        b = (uint32_t)rhs * 16 + (uint32_t)lane * 4;
                        d = (uint32_t)vd * 16 + (uint32_t)lane * 4;
                        LDR_W_IMM(REG_X3, REG_X5, a);
                        LDR_W_IMM(REG_X4, REG_X5, b);
                        /* cmp w3,w4; csel w3,w3,w4,le/ge */
                        if (!jit_put_word(ctx, 0x6b04007f) ||
                            !jit_put_word(ctx, 0x1a800000 | (4u << 16) |
                                ((uint32_t)(0xd)
                                 << 12) | (3u << 5) | 3u))
                                return false;
                        STR_W_IMM(REG_X3, REG_X5, d);
                }
                return true;
        }
        va = jit_arm64_vreg(vd);
        vb = jit_arm64_vreg(lhs);
        vc = jit_arm64_vreg(rhs);
        if (va < 0 || vb < 0 || vc < 0) {
                rt_error(ctx->env, BROKEN_BYTECODE);
                return false;
        }

        /* smin vA.4s, vB.4s, vC.4s */
        if (!jit_put_word(ctx, 0x4ea06c00 | ((uint32_t)vc << 16) |
                          ((uint32_t)vb << 5) | (uint32_t)va))
                return false;

        return true;
}

/* Visit an OP_VMAXS32X4 instruction. */
static INLINE bool
jit_visit_vmaxs32x4_op(
        struct rt_jit_context *ctx)
{
        int vd;
        int lhs;
        int rhs;
        int va, vb, vc;

        CONSUME_IMM8(vd);
        CONSUME_IMM8(lhs);
        CONSUME_IMM8(rhs);

        if ((ctx->simd_caps & JIT_SIMD_CAP_NEON) == 0) {
                int lane;

                MOVZ(REG_X5, IMM16((uint32_t)offsetof(struct rt_env, vreg) & 0xffff), LSL_0);
                MOVK(REG_X5, IMM16(((uint32_t)offsetof(struct rt_env, vreg) >> 16) & 0xffff), LSL_16);
                ADD(REG_X5, REG_X0, REG_X5);

                for (lane = 0; lane < 4; lane++) {
                        uint32_t a;
                        uint32_t b;
                        uint32_t d;

                        a = (uint32_t)lhs * 16 + (uint32_t)lane * 4;
                        b = (uint32_t)rhs * 16 + (uint32_t)lane * 4;
                        d = (uint32_t)vd * 16 + (uint32_t)lane * 4;
                        LDR_W_IMM(REG_X3, REG_X5, a);
                        LDR_W_IMM(REG_X4, REG_X5, b);
                        /* cmp w3,w4; csel w3,w3,w4,le/ge */
                        if (!jit_put_word(ctx, 0x6b04007f) ||
                            !jit_put_word(ctx, 0x1a800000 | (4u << 16) |
                                ((uint32_t)(0xa)
                                 << 12) | (3u << 5) | 3u))
                                return false;
                        STR_W_IMM(REG_X3, REG_X5, d);
                }
                return true;
        }
        va = jit_arm64_vreg(vd);
        vb = jit_arm64_vreg(lhs);
        vc = jit_arm64_vreg(rhs);
        if (va < 0 || vb < 0 || vc < 0) {
                rt_error(ctx->env, BROKEN_BYTECODE);
                return false;
        }

        /* smax vA.4s, vB.4s, vC.4s */
        if (!jit_put_word(ctx, 0x4ea06400 | ((uint32_t)vc << 16) |
                          ((uint32_t)vb << 5) | (uint32_t)va))
                return false;

        return true;
}


/* Visit a bytecode of a function. */
#endif

static bool
jit_visit_bytecode(
        struct rt_jit_context *ctx)
{
        uint8_t opcode;

        /* Put a prologue. */
        ASM {
                /* Push the general-purpose registers. */
                STP_PUSH        (REG_X29, REG_X30);
                STP_PUSH        (REG_X27, REG_X28);
                STP_PUSH        (REG_X25, REG_X26);
                STP_PUSH        (REG_X23, REG_X24);
                STP_PUSH        (REG_X21, REG_X22);
                STP_PUSH        (REG_X19, REG_X20);
                STP_PUSH        (REG_X17, REG_X18);
                STP_PUSH        (REG_X15, REG_X16);
                STP_PUSH        (REG_X13, REG_X14);
                STP_PUSH        (REG_X11, REG_X12);
                STP_PUSH        (REG_X9, REG_X10);
                STP_PUSH        (REG_X7, REG_X8);
                STP_PUSH        (REG_X5, REG_X6);
                STP_PUSH        (REG_X3, REG_X4);
                STP_PUSH        (REG_X1, REG_X2);
                STP_PUSH        (REG_XZR, REG_X0); /* XZR is dummy */

                /* x0 = env */

                /* x1 = *env->frame = &env->frame->tmpvar[0] */
                LDR             (REG_X1, REG_X0);
                LDR             (REG_X1, REG_X1);

                /* Skip an exception handler. */
                BAL             (IMM19(76));
        }

        /* Put an exception handler. */
        ctx->exception_code = ctx->code;
        ASM {
        /* EXCEPTION: */
                LDP_POP         (REG_X1, REG_X0); /* x1 is dummy */
                LDP_POP         (REG_X1, REG_X2);
                LDP_POP         (REG_X3, REG_X4);
                LDP_POP         (REG_X5, REG_X6);
                LDP_POP         (REG_X7, REG_X8);
                LDP_POP         (REG_X9, REG_X10);
                LDP_POP         (REG_X11, REG_X12);
                LDP_POP         (REG_X13, REG_X14);
                LDP_POP         (REG_X15, REG_X16);
                LDP_POP         (REG_X17, REG_X18);
                LDP_POP         (REG_X19, REG_X20);
                LDP_POP         (REG_X21, REG_X22);
                LDP_POP         (REG_X23, REG_X24);
                LDP_POP         (REG_X25, REG_X26);
                LDP_POP         (REG_X27, REG_X28);
                LDP_POP         (REG_X29, REG_X30);
                MOVZ            (REG_X0, IMM16(0), LSL_0);
                RET             ();
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
#if defined(NOCT_USE_OPTIMIZER)
		case OP_VINDEX_HINT:
			if (!jit_visit_vindex_hint_op(ctx))
				return false;
			break;
		case OP_PLOOP_HINT:
			if (!jit_visit_arm64_ploop_hint_op(ctx))
				return false;
			break;
#endif
		case OP_TMPVAR_TYPE:
			if (!rt_jit_visit_tmpvar_type_op(ctx)) return false;
			break;
		case OP_MATERIALIZE_TYPE:
			if (!jit_visit_arm64_materialize_type_op(ctx)) return false;
			break;
#if defined(NOCT_USE_OPTIMIZER)
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
		case OP_VCMPI32X4:
			if (!jit_visit_vcmpi32x4_op(ctx))
				return false;
			break;
		case OP_VCMPF32X4:
			if (!jit_visit_vcmpf32x4_op(ctx))
				return false;
			break;
		case OP_VSELECT128:
			if (!jit_visit_vselect128_op(ctx))
				return false;
			break;
		case OP_VMASKSTOREI32X4:
			if (!jit_visit_vmaskstorei32x4_op(ctx))
				return false;
			break;
		case OP_VINDUCTF32X4:
			if (!jit_visit_vinductf32x4_op(ctx)) return false;
			break;
		case OP_VGATHERI32X4_CHECKED:
			if (!jit_visit_vgatheri32x4_checked_op(ctx)) return false;
			break;
#endif
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
#if defined(NOCT_USE_OPTIMIZER)
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
#endif
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
                LDP_POP         (REG_X1, REG_X0); /* x1 is dummy */
                LDP_POP         (REG_X1, REG_X2);
                LDP_POP         (REG_X3, REG_X4);
                LDP_POP         (REG_X5, REG_X6);
                LDP_POP         (REG_X7, REG_X8);
                LDP_POP         (REG_X9, REG_X10);
                LDP_POP         (REG_X11, REG_X12);
                LDP_POP         (REG_X13, REG_X14);
                LDP_POP         (REG_X15, REG_X16);
                LDP_POP         (REG_X17, REG_X18);
                LDP_POP         (REG_X19, REG_X20);
                LDP_POP         (REG_X21, REG_X22);
                LDP_POP         (REG_X23, REG_X24);
                LDP_POP         (REG_X25, REG_X26);
                LDP_POP         (REG_X27, REG_X28);
                LDP_POP         (REG_X29, REG_X30);
                MOVZ            (REG_X0, IMM16(1), LSL_0);
                RET             ();
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
        uint32_t i;

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
        if (offset < -134217728 || offset > 134217724) {
                rt_error(ctx->env, N_TR("Branch target too far."));
                return false;
        }

        /* Set the assembler cursor. */
        ctx->code = ctx->branch_patch[patch_index].code;

        /* Assemble. */
        if (ctx->branch_patch[patch_index].type == PATCH_BAL) {
                ASM {
                        B       (offset);
                }
        } else if (ctx->branch_patch[patch_index].type == PATCH_BEQ) {
                if (getenv("NOCT_JIT_FORCE_LONG_BRANCH") == NULL &&
                    offset >= -1048576 && offset <= 1048572) {
                        ASM {
                                BEQ     (IMM19(offset));
                                if (!jit_put_word(ctx, 0xd503201f)) return false;
                        }
                } else {
                        ASM {
                                /* If not equal, skip the long B. */
                                BNE     (IMM19(8));
                                B       (offset - 4);
                        }
                }
        } else if (ctx->branch_patch[patch_index].type == PATCH_BNE) {
                if (getenv("NOCT_JIT_FORCE_LONG_BRANCH") == NULL &&
                    offset >= -1048576 && offset <= 1048572) {
                        ASM {
                                BNE     (IMM19(offset));
                                if (!jit_put_word(ctx, 0xd503201f)) return false;
                        }
                } else {
                        ASM {
                                /* If equal, skip the long B. */
                                BEQ     (IMM19(8));
                                B       (offset - 4);
                        }
                }
        }

        return true;
}

#endif /* defined(NOCT_ARCH_ARM64) && defined(NOCT_USE_JIT) */
