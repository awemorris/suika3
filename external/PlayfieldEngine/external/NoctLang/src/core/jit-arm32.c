/* -*- coding: utf-8; tab-width: 8; indent-tabs-mode: nil; -*- */

/*
 * Noct Programming Language
 * Copyright (c) 2025, 2026, Awe Morris
 */

/*
 * JIT (arm32): Just-In-Time native code generation
 */

#include <noct/noct.h>

#if defined(NOCT_ARCH_ARM32) && defined(NOCT_USE_JIT)

#include "runtime.h"
#include "jit.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#if defined(__linux__)
#include <sys/auxv.h>
#include <asm/hwcap.h>
#endif

/* False asseretion */
#define JIT_OP_NOT_IMPLEMENTED  0
#define NEVER_COME_HERE         0

/* Branch patch type */
#define PATCH_BAL               0
#define PATCH_BEQ               1
#define PATCH_BNE               2

/* Forward declaration */
static bool jit_visit_bytecode(struct rt_jit_context *ctx);
static bool jit_patch_branch(struct rt_jit_context *ctx, int patch_index);
static uint32_t jit_detect_simd_caps(void);

/*
 * Generate a JIT-compiled code for a function.
 */
bool
jit_build(
          struct rt_env *env,
          struct rt_func *func)
{
	return rt_jit_build_standard(env,
                                  func,
                                  jit_detect_simd_caps(),
                                  "arm32",
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
#define REG_R0          0
#define REG_R1          1
#define REG_R2          2
#define REG_R3          3
#define REG_R4          4
#define REG_R5          5
#define REG_R6          6
#define REG_R7          7
#define REG_R8          8
#define REG_R9          9
#define REG_R10         10      /* exception_handler */
#define REG_R11         11      /* env */
#define REG_R12         12      /* &env->frame->tmpvar[0] */
#define REG_SP          13
#define REG_LR          14
#define REG_PC          15

/* Immediate */
#define IMM8(v)         (v)
#define IMM9(v)         (v)
#define IMM12(v)        (v)
#define IMM16(v)        (v)
#define IMM19(v)        (v)

/* Shift */
#define LSL_0           0
#define LSL_16          16
#define LSL_32          32
#define LSL_48          48

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

/* mov reg, reg */
#define MOV(rd, rs)             if (!jit_put_mov(ctx, rd, rs)) return false
static INLINE bool
jit_put_mov(
        struct rt_jit_context *ctx,
        int rd,
        int rs)
{
        if (!jit_put_word(ctx,
                          0xe1a00000 |                  /* mov */
                          (uint32_t)(rd << 12) |        /* rd */
                          (uint32_t)rs))                /* rs */
                return false;
        return true;
}

/* movw rd, imm */
#define MOVW(rd, imm)           if (!jit_put_movw(ctx, rd, imm)) return false
static INLINE bool
jit_put_movw(
        struct rt_jit_context *ctx,
        int rd,
        uint32_t imm)
{
        if (!jit_put_word(ctx,
                          0xe3000000 |                  /* movw */
                          (uint32_t)(rd << 12) |        /* rd */
                          ((imm & 0xf000) << 4) |       /* imm[15:12] */
                          (imm & 0xfff)))               /* imm[11:0] */
                return false;
        return true;
}

/* movt rd, imm */
#define MOVT(rd, imm)           if (!jit_put_movt(ctx, rd, imm)) return false
static INLINE bool
jit_put_movt(
        struct rt_jit_context *ctx,
        int rd,
        uint32_t imm)
{
        if (!jit_put_word(ctx,
                          0xe3400000 |                  /* movt */
                          (uint32_t)(rd << 12) |        /* rd */
                          (imm & 0xfff) |               /* imm[11:0] */
                          ((imm >> 12) & 0xff) << 16))  /* imm[15:12] */
                return false;
        return true;
}

/* add rd, ra, rb */
#define ADD(rd, ra, rb)         if (!jit_put_add(ctx, rd, ra, rb)) return false
static INLINE bool
jit_put_add(
        struct rt_jit_context *ctx,
        int rd,
        int ra,
        int rb)
{
        if (!jit_put_word(ctx,
                          0xe0800000 |                  /* add */
                          (uint32_t)(rd << 12) |        /* rd */
                          (uint32_t)(ra << 16) |        /* ra */
                          (uint32_t)rb))
                return false;
        return true;
}

/* add rd, rs, imm */
#define ADD_IMM(rd, rs, imm)            if (!jit_put_add_imm(ctx, rd, rs, imm)) return false
static INLINE bool
jit_put_add_imm(
        struct rt_jit_context *ctx,
        int rd,
        int rs,
        uint32_t imm)
{
        if (!jit_put_word(ctx,
                          0xe2800000 |                  /* add */
                          (uint32_t)(rd << 12) |        /* rd */
                          (uint32_t)(rs << 16) |        /* rs */
                          (uint32_t)imm))               /* imm */
                return false;
        return true;
}

/* sub rd, rs, imm */
#define SUB_IMM(rd, rs, imm)            if (!jit_put_sub_imm(ctx, rd, rs, imm)) return false
static INLINE bool
jit_put_sub_imm(
        struct rt_jit_context *ctx,
        int rd,
        int rs,
        uint32_t imm)
{
        if (!jit_put_word(ctx,
                          0xe2400000 |                  /* sub */
                          (uint32_t)(rd << 12) |        /* rd */
                          (uint32_t)(rs << 16) |        /* rs */
                          imm))                         /* imm */
                return false;
        return true;
}

/* lsl rd, rs, imm */
#define LSL_3(rd, rs)           if (!jit_put_lsl_3(ctx, rd, rs)) return false
static INLINE bool
jit_put_lsl_3(
        struct rt_jit_context *ctx,
        int rd,
        int rs)
{
        if (!jit_put_word(ctx,
                          0xe1a00180 |                  /* lsl #3 */
                          (uint32_t)(rd << 12) |        /* rd */
                          (uint32_t)rs))                /* rs */
                return false;
        return true;
}

/* ldr rd, [rs + #imm] */
#define LDR(rd, rs, imm)        if (!jit_put_ldr(ctx, rd, rs, imm)) return false
static INLINE bool
jit_put_ldr(
        struct rt_jit_context *ctx,
        int rd,
        int rs,
        uint32_t imm)
{
        if (!jit_put_word(ctx,
                          0xe5900000 |                  /* ldr */
                          (uint32_t)(rd << 12) |        /* rd */
                          (uint32_t)(rs << 16) |        /* rs */
                          imm))                         /* imm */
                return false;
        return true;
}

/* str rs, [rd + #imm] */
#define STR(rs, rd, imm)        if (!jit_put_str(ctx, rs, rd, imm)) return false
static INLINE bool
jit_put_str(
        struct rt_jit_context *ctx,
        int rs,
        int rd,
        uint32_t imm)
{
        if (!jit_put_word(ctx,
                          0xe5800000 |                  /* str */
                          (uint32_t)(rs << 12) |        /* rd */
                          (uint32_t)(rd << 16) |        /* rs */
                          imm))                         /* imm */
                return false;
        return true;
}

/* cmp rs, #imm */
#define CMP_IMM(rs, imm)        if (!jit_put_cmp_imm(ctx, rs, imm)) return false
static INLINE bool
jit_put_cmp_imm(
        struct rt_jit_context *ctx,
        int rs,
        uint32_t imm)
{
        if (!jit_put_word(ctx,
                          0xe3500000 |                  /* ldr */
                          (uint32_t)(rs << 16) |        /* rd */
                          imm))                         /* imm */
                return false;
        return true;
}

/* cmp r0, r1 */
#define CMP_R0_R1()             if (!jit_put_cmp_r0_r1(ctx)) return false
static INLINE bool
jit_put_cmp_r0_r1(
        struct rt_jit_context *ctx)
{
        if (!jit_put_word(ctx, 0xe1500001))
                return false;
        return true;
}

/* bal #imm */
#define BAL(imm)        if (!jit_put_bal(ctx, imm)) return false
static INLINE bool
jit_put_bal(
        struct rt_jit_context *ctx,
        uint32_t imm)
{
        if (!jit_put_word(ctx,
                          0xea000000 |                  /* bal */
                          ((imm / 4 - 2) & 0xffffff)))  /* imm */
                return false;
        return true;
}

/* beq #imm */
#define BEQ(imm)        if (!jit_put_beq(ctx, imm)) return false
static INLINE bool
jit_put_beq(
        struct rt_jit_context *ctx,
        uint32_t imm)
{
        if (!jit_put_word(ctx,
                          0x0a000000 |                  /* beq */
                          ((imm / 4 - 2) & 0xffffff)))  /* imm */
                return false;
        return true;
}

/* bne #imm */
#define BNE(imm)        if (!jit_put_bne(ctx, imm)) return false
static INLINE bool
jit_put_bne(
        struct rt_jit_context *ctx,
        uint32_t imm)
{
        if (!jit_put_word(ctx,
                          0x1a000000 |                  /* bne */
                          ((imm / 4 - 2) & 0xffffff)))  /* imm */
                return false;
        return true;
}

/* blx reg */
#define BLX(reg)                if (!jit_put_blx(ctx, reg)) return false
static INLINE bool
jit_put_blx(
        struct rt_jit_context *ctx,
        int reg)
{
        if (!jit_put_word(ctx,
                          0xe12fff30 |          /* blx */
                          (uint32_t)reg))       /* reg */
                return false;
        return true;
}

/* ret */
#define RET()                   if (!jit_put_ret(ctx)) return false
static INLINE bool
jit_put_ret(
        struct rt_jit_context *ctx)
{
        if (!jit_put_word(ctx,
                          0xe12fff1e))          /* bx lr */
                return false;
        return true;
}

/* push {reg1} */
#define PUSH(r)                 if (!jit_put_push(ctx, r)) return false
static INLINE bool
jit_put_push(
        struct rt_jit_context *ctx,
        int r)
{
        if (!jit_put_word(ctx,
                          0xe52d0004 |          /* str rN, [sp, #-4]! */
                          ((uint32_t)r << 12)))
                return false;
        return true;
}

/* pop {reg1} */
#define POP(r)                  if (!jit_put_pop2(ctx, r)) return false
static INLINE bool
jit_put_pop2(
        struct rt_jit_context *ctx,
        int r)
{
        if (!jit_put_word(ctx,
                          0xe49d0004 |          /* ldr rN, [sp], #4 */
                          ((uint32_t)r << 12)))
                return false;
        return true;
}

/* ldrb rt, [rn, r0] */
#define LDRB_R(rt, rn)          if (!jit_put_ldrb_r(ctx, rt, rn)) return false
static INLINE bool
jit_put_ldrb_r(
        struct rt_jit_context *ctx,
        int rt,
        int rn)
{
        if (!jit_put_word(ctx,
                          0xe7d00000 |          /* ldrb */
                          ((uint32_t)rn << 16) | /* rn */
                          ((uint32_t)rt << 12))) /* rt */
                return false;
        return true;
}

/* ldrb rt, [rn] */
#define LDRB0(rt, rn)           if (!jit_put_ldrb0(ctx, rt, rn)) return false
static INLINE bool
jit_put_ldrb0(
        struct rt_jit_context *ctx,
        int rt,
        int rn)
{
        if (!jit_put_word(ctx,
                          0xe5d00000 |          /* ldrb */
                          ((uint32_t)rn << 16) | /* rn */
                          ((uint32_t)rt << 12))) /* rt */
                return false;
        return true;
}

/* ldrsb rt, [rn] */
#define LDRSB0(rt, rn)          if (!jit_put_ldrsb0(ctx, rt, rn)) return false
static INLINE bool
jit_put_ldrsb0(
        struct rt_jit_context *ctx,
        int rt,
        int rn)
{
        if (!jit_put_word(ctx,
                          0xe1d000d0 |          /* ldrsb */
                          ((uint32_t)rn << 16) | /* rn */
                          ((uint32_t)rt << 12))) /* rt */
                return false;
        return true;
}

/* ldrh rt, [rn] */
#define LDRH0(rt, rn)           if (!jit_put_ldrh0(ctx, rt, rn)) return false
static INLINE bool
jit_put_ldrh0(
        struct rt_jit_context *ctx,
        int rt,
        int rn)
{
        if (!jit_put_word(ctx,
                          0xe1d000b0 |          /* ldrh */
                          ((uint32_t)rn << 16) | /* rn */
                          ((uint32_t)rt << 12))) /* rt */
                return false;
        return true;
}

/* ldrsh rt, [rn] */
#define LDRSH0(rt, rn)          if (!jit_put_ldrsh0(ctx, rt, rn)) return false
static INLINE bool
jit_put_ldrsh0(
        struct rt_jit_context *ctx,
        int rt,
        int rn)
{
        if (!jit_put_word(ctx,
                          0xe1d000f0 |          /* ldrsh */
                          ((uint32_t)rn << 16) | /* rn */
                          ((uint32_t)rt << 12))) /* rt */
                return false;
        return true;
}

/* strb rt, [rn] */
#define STRB0(rt, rn)           if (!jit_put_strb0(ctx, rt, rn)) return false
static INLINE bool
jit_put_strb0(
        struct rt_jit_context *ctx,
        int rt,
        int rn)
{
        if (!jit_put_word(ctx,
                          0xe5c00000 |          /* strb */
                          ((uint32_t)rn << 16) | /* rn */
                          ((uint32_t)rt << 12))) /* rt */
                return false;
        return true;
}

/* strh rt, [rn] */
#define STRH0(rt, rn)           if (!jit_put_strh0(ctx, rt, rn)) return false
static INLINE bool
jit_put_strh0(
        struct rt_jit_context *ctx,
        int rt,
        int rn)
{
        if (!jit_put_word(ctx,
                          0xe1c000b0 |          /* strh */
                          ((uint32_t)rn << 16) | /* rn */
                          ((uint32_t)rt << 12))) /* rt */
                return false;
        return true;
}

/* lsl rd, rs, #sh */
#define LSL_IMM(rd, rs, sh)     if (!jit_put_lsl_imm(ctx, rd, rs, sh)) return false
static INLINE bool
jit_put_lsl_imm(
        struct rt_jit_context *ctx,
        int rd,
        int rs,
        int sh)
{
        if (!jit_put_word(ctx,
                          0xe1a00000 |          /* lsl */
                          ((uint32_t)rd << 12) | /* rd */
                          ((uint32_t)sh << 7) |  /* sh */
                          (uint32_t)rs))         /* rs */
                return false;
        return true;
}

static INLINE uint32_t
neon_q3(uint32_t base, int qd, int qn, int qm)
{
	uint32_t dd;
	uint32_t dn;
	uint32_t dm;

	dd = (uint32_t)(16 + qd * 2);
	dn = (uint32_t)(16 + qn * 2);
	dm = (uint32_t)(16 + qm * 2);
	return base |
	       ((dd & 16u) << 18) | ((dd & 15u) << 12) |
	       ((dn & 15u) << 16) | ((dn & 16u) << 3) |
	       ((dm & 16u) << 1) | (dm & 15u);
}

static INLINE uint32_t
neon_q2_imm(uint32_t base, int qd, int qm, uint32_t imm)
{
	uint32_t dd;
	uint32_t dm;

	dd = (uint32_t)(16 + qd * 2);
	dm = (uint32_t)(16 + qm * 2);
	return base | (imm << 16) |
	       ((dd & 16u) << 18) | ((dd & 15u) << 12) |
	       ((dm & 16u) << 1) | (dm & 15u);
}

/*
 * Templates
 */

#define ASM_BINARY_OP(f)                                                                        \
        ASM {                                                                                   \
                /* r11 = env */                                                                 \
                /* r12 = &env->frame->tmpvar[0] */                                              \
                                                                                                \
                PUSH            (REG_R4);                                                       \
                PUSH            (REG_R11);                                                      \
                PUSH            (REG_R12);                                                      \
                PUSH            (REG_LR);                                                       \
                                                                                                \
                /* Arg1 r0: env */                                                              \
                MOV             (REG_R0, REG_R11);                                              \
                                                                                                \
                /* Arg2 r1: dst */                                                              \
                MOVW            (REG_R1, (uint32_t)dst);                                        \
                                                                                                \
                /* Arg3 r2: src1 */                                                             \
                MOVW            (REG_R2, (uint32_t)src1);                                       \
                                                                                                \
                /* Arg4 r3: src2 */                                                             \
                MOVW            (REG_R3, (uint32_t)src2);                                       \
                                                                                                \
                /* Call f(). */                                                                 \
                MOVW            (REG_R4, (uint32_t)(f) & 0xffff);                               \
                MOVT            (REG_R4, ((uint32_t)(f) >> 16) & 0xffff);                       \
                BLX             (REG_R4);                                                       \
                                                                                                \
                /* If failed: */                                                                \
                CMP_IMM         (REG_R0, 0);                                                    \
                POP             (REG_LR);                                                       \
                POP             (REG_R12);                                                      \
                POP             (REG_R11);                                                      \
                POP             (REG_R4);                                                       \
                BEQ             ((uint32_t)ctx->exception_code - (uint32_t)ctx->code);          \
        }

#define ASM_UNARY_OP(f)                                                                         \
        ASM {                                                                                   \
                /* r11 = env */                                                                 \
                /* r12 = &env->frame->tmpvar[0] */                                              \
                                                                                                \
                PUSH            (REG_R4);                                                       \
                PUSH            (REG_R11);                                                      \
                PUSH            (REG_R12);                                                      \
                PUSH            (REG_LR);                                                       \
                                                                                                \
                /* Arg1 r0: env */                                                              \
                MOV             (REG_R0, REG_R11);                                              \
                                                                                                \
                /* Arg2 r1: dst */                                                              \
                MOVW            (REG_R1, (uint32_t)dst);                                        \
                                                                                                \
                /* Arg3 r2: src */                                                              \
                MOVW            (REG_R2, (uint32_t)src);                                        \
                                                                                                \
                /* Call f(). */                                                                 \
                MOVW            (REG_R3, (uint32_t)(f) & 0xffff);                               \
                MOVT            (REG_R3, ((uint32_t)(f) >> 16) & 0xffff);                       \
                BLX             (REG_R3);                                                       \
                                                                                                \
                /* If failed: */                                                                \
                CMP_IMM         (REG_R0, 0);                                                    \
                POP             (REG_LR);                                                       \
                POP             (REG_R12);                                                      \
                POP             (REG_R11);                                                      \
                POP             (REG_R4);                                                       \
                BEQ             ((uint32_t)ctx->exception_code - (uint32_t)ctx->code);          \
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
                /* r11 = env */
                /* r12 = &env->frame->tmpvar[0] */

                /* rt->line = line; */
                MOVW            (REG_R0, line);
                /* env->line is at offset 4 on 32-bit targets. */
                STR             (REG_R0, REG_R11, 4);
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
                /* r11 = env */
                /* r12 = &env->frame->tmpvar[0] */

                /* r0 = dst_addr = &env->frame->tmpvar[dst] */
                MOVW    (REG_R0, (uint32_t)dst);
                ADD     (REG_R0, REG_R0, REG_R12);

                /* r1 = src_addr = &env->frame->tmpvar[src] */
                MOVW    (REG_R1, (uint32_t)src);
                ADD     (REG_R1, REG_R1, REG_R12);

                /* *dst_addr = *src_addr (8-byte)*/
                LDR     (REG_R2, REG_R1, 0);
                LDR     (REG_R3, REG_R1, 4);
                STR     (REG_R2, REG_R0, 0);
                STR     (REG_R3, REG_R0, 4);
                LDR     (REG_R2, REG_R1, 8);
                LDR     (REG_R3, REG_R1, 12);
                STR     (REG_R2, REG_R0, 8);
                STR     (REG_R3, REG_R0, 12);
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
                /* r11 = env */
                /* r12 = &env->frame->tmpvar[0] */

                /* r0 = &env->frame->tmpvar[dst] */
                MOVW    (REG_R0, (uint32_t)dst);
                ADD     (REG_R0, REG_R0, REG_R12);

                /* env->frame->tmpvar[dst].type = RT_VALUE_INT */
                MOVW    (REG_R1, 0);
                STR     (REG_R1, REG_R0, 0);

                /* env->frame->tmpvar[dst].val.i = val */
                MOVW    (REG_R1, val & 0xffff);
                MOVT    (REG_R1, (val >> 16) & 0xffff);
                STR     (REG_R1, REG_R0, 8);
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
                /* r11 = env */
                /* r12 = &env->frame->tmpvar[0] */

                /* r0 = &env->frame->tmpvar[dst] */
                MOVW    (REG_R0, (uint32_t)dst);
                ADD     (REG_R0, REG_R0, REG_R12);

                /* env->frame->tmpvar[dst].type = NOCT_VALUE_LONG */
                MOVW    (REG_R1, 5);
                STR     (REG_R1, REG_R0, 0);

                /* env->frame->tmpvar[dst].val.i = val */
                MOVW    (REG_R1, val & 0xffff);
                MOVT    (REG_R1, (val >> 16) & 0xffff);
                STR     (REG_R1, REG_R0, 8);
                MOVW    (REG_R1, (val >> 32) & 0xffff);
                MOVT    (REG_R1, (val >> 48) & 0xffff);
                STR     (REG_R1, REG_R0, 12);
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
                /* r11 = env */
                /* r12 = &env->frame->tmpvar[0] */

                /* r0 = &env->frame->tmpvar[dst] */
                MOVW    (REG_R0, (uint32_t)dst);
                ADD     (REG_R0, REG_R0, REG_R12);

                /* Assign env->frame->tmpvar[dst].type = RT_VALUE_FLOAT. */
                MOVW    (REG_R1, 1);
                STR     (REG_R1, REG_R0, 0);

                /* Assign env->frame->tmpvar[dst].val.f = val. */
                MOVW    (REG_R1, val & 0xffff);
                MOVT    (REG_R1, (val >> 16) & 0xffff);
                STR     (REG_R1, REG_R0, 8);
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
                /* r11 = env */
                /* r12 = &env->frame->tmpvar[0] */

                /* r0 = &env->frame->tmpvar[dst] */
                MOVW    (REG_R0, (uint32_t)dst);
                ADD     (REG_R0, REG_R0, REG_R12);

                /* env->frame->tmpvar[dst].type = NOCT_VALUE_DOUBLE */
                MOVW    (REG_R1, 6);
                STR     (REG_R1, REG_R0, 0);

                /* env->frame->tmpvar[dst].val.i = val */
                MOVW    (REG_R1, val & 0xffff);
                MOVT    (REG_R1, (val >> 16) & 0xffff);
                STR     (REG_R1, REG_R0, 8);
                MOVW    (REG_R1, (val >> 32) & 0xffff);
                MOVT    (REG_R1, (val >> 48) & 0xffff);
                STR     (REG_R1, REG_R0, 12);
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
                /* r11 = env */
                /* r12 = &env->frame->tmpvar[0] */

                PUSH            (REG_R4);
                PUSH            (REG_R11);
                PUSH            (REG_R12);
                PUSH            (REG_LR);

                /* Arg1 r0: env */
                MOV             (REG_R0, REG_R11);

                /* Arg2 r1: &env->frame->tmpvar[dst] */
                MOVW            (REG_R1, (uint32_t)dst);
                ADD             (REG_R1, REG_R1, REG_R12);
        
                /* Arg3 r2: val */
                MOVW            (REG_R2, (uint32_t)val & 0xffff);
                MOVT            (REG_R2, ((uint32_t)val >> 16) & 0xffff);
        
                /* Arg4 r3: len */
                MOVW            (REG_R3, len & 0xffff);
                MOVT            (REG_R3, (len >> 16) & 0xffff);

                /* Arg5 [sp+0]: hash */
                MOVW            (REG_R4, hash & 0xffff);
                MOVT            (REG_R4, (hash >> 16) & 0xffff);
                SUB_IMM         (REG_SP, REG_SP, 16);
                STR             (REG_R4, REG_SP, 0);

                /* Call ex_make_string_with_hash(). */
                MOVW            (REG_R4, ((uint32_t)ex_make_string_with_hash) & 0xffff);
                MOVT            (REG_R4, (((uint32_t)ex_make_string_with_hash) >> 16) & 0xffff);
                BLX             (REG_R4);
                ADD_IMM         (REG_SP, REG_SP, IMM16(16));
        
                /* If failed: */
                CMP_IMM         (REG_R0, 0);
                POP             (REG_LR);
                POP             (REG_R12);
                POP             (REG_R11);
                POP             (REG_R4);
                BEQ             ((uint32_t)ctx->exception_code - (uint32_t)ctx->code);
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
                /* r11 = env */
                /* r12 = &env->frame->tmpvar[0] */

                PUSH            (REG_R4); /* dummy */
                PUSH            (REG_R11);
                PUSH            (REG_R12);
                PUSH            (REG_LR);
                
                /* Arg1 r0: env */
                MOV             (REG_R0, REG_R11);

                /* Arg2 r1: &env->frame->tmpvar[dst] */
                MOVW            (REG_R1, (uint32_t)dst);
                ADD             (REG_R1, REG_R1, REG_R12);

                /* Call ex_make_empty_array(). */
                MOVW            (REG_R3, ((uint32_t)ex_make_empty_array) & 0xffff);
                MOVT            (REG_R3, (((uint32_t)ex_make_empty_array) >> 16) & 0xffff);
                BLX             (REG_R3);

                /* If failed: */
                CMP_IMM         (REG_R0, 0);
                POP             (REG_LR);
                POP             (REG_R12);
                POP             (REG_R11);
                POP             (REG_R4);
                BEQ             ((uint32_t)ctx->exception_code - (uint32_t)ctx->code);
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

        /* rt_make_empty_dict(env, &env->frame->tmpvar[dst]); */
        ASM {
                /* r11 = env */
                /* r12 = &env->frame->tmpvar[0] */

                PUSH            (REG_R4); /* dummy */
                PUSH            (REG_R11);
                PUSH            (REG_R12);
                PUSH            (REG_LR);
                
                /* Arg1 r0: env */
                MOV             (REG_R0, REG_R11);

                /* Arg2 r1: &env->frame->tmpvar[dst] */
                MOVW            (REG_R1, (uint32_t)dst);
                ADD             (REG_R1, REG_R1, REG_R12);

                /* Call ex_make_empty_array(). */
                MOVW            (REG_R3, ((uint32_t)ex_make_empty_dict) & 0xffff);
                MOVT            (REG_R3, (((uint32_t)ex_make_empty_dict) >> 16) & 0xffff);
                BLX             (REG_R3);

                /* If failed: */
                CMP_IMM         (REG_R0, 0);
                POP             (REG_LR);
                POP             (REG_R12);
                POP             (REG_R11);
                POP             (REG_R4);
                BEQ             ((uint32_t)ctx->exception_code - (uint32_t)ctx->code);
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
                /* r11 = env */
                /* r12 = &env->frame->tmpvar[0] */

                /* Get &env->frame->tmpvar[dst] at r0. */
                MOVW    (REG_R0, (uint32_t)dst);        /* dst */
                ADD     (REG_R0, REG_R0, REG_R12);      /* r0 = &env->frame->tmpvar[dst] = &env->frame->tmpvar[dst].type */

                /* env->frame->tmpvar[dst].val.i++ */
                LDR     (REG_R1, REG_R0, 8);            /* tmp = &env->frame->tmpvar[dst].val.i */
                ADD_IMM (REG_R1, REG_R1, (uint32_t)step);
                STR     (REG_R1, REG_R0, 8);            /* env->frame->tmpvar[dst].val.i = tmp */
        }

        return true;
}

#if defined(NOCT_USE_OPTIMIZER)
static INLINE bool
jit_visit_vindex_hint_op(struct rt_jit_context *ctx)
{
	int a, b, c, required_vregs, lanes, flags;

	UNUSED_PARAMETER(a);
        UNUSED_PARAMETER(b);
        UNUSED_PARAMETER(c);
	UNUSED_PARAMETER(lanes);
        UNUSED_PARAMETER(flags);

	CONSUME_TMPVAR(a);
        CONSUME_TMPVAR(b);
        CONSUME_TMPVAR(c);
	CONSUME_IMM8(required_vregs);
        CONSUME_IMM8(lanes);
        CONSUME_IMM8(flags);

	if (required_vregs > 8)
		ctx->simd_caps = 0;

	return true;
}

static INLINE bool jit_visit_vori32x4i_op(struct rt_jit_context *ctx)
{
	int a,b,c,d;

	UNUSED_PARAMETER(a);
        UNUSED_PARAMETER(b);
        UNUSED_PARAMETER(c);
        UNUSED_PARAMETER(d);

        CONSUME_IMM8(a);
        CONSUME_IMM8(b);
	CONSUME_IMM8(c);
        CONSUME_IMM8(d);

        /* clean interpreter fallback for foreign bytecode */
	return false;
}

static INLINE bool jit_visit_vfmaf32x4_op(struct rt_jit_context *ctx)
{
	int a,b,c,d;

	UNUSED_PARAMETER(a);
        UNUSED_PARAMETER(b);
        UNUSED_PARAMETER(c);
        UNUSED_PARAMETER(d);

        CONSUME_IMM8(a);
        CONSUME_IMM8(b);
	CONSUME_IMM8(c);
        CONSUME_IMM8(d);

	/* no native Armv7 FMA contract; use the interpreter */
	return false;
}
#endif

static INLINE bool
jit_visit_subjnz_op(struct rt_jit_context *ctx)
{
	int value, decrement;
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
		MOVW            (REG_R0, (uint32_t)value);
		ADD             (REG_R0, REG_R0, REG_R12);
		LDR             (REG_R1, REG_R0, 8);
		SUB_IMM         (REG_R1, REG_R1, (uint32_t)decrement);
		STR             (REG_R1, REG_R0, 8);
		CMP_IMM         (REG_R1, 0);
	}

	ctx->branch_patch[ctx->branch_patch_count].code = ctx->code;
	ctx->branch_patch[ctx->branch_patch_count].lpc = target_lpc;
	ctx->branch_patch[ctx->branch_patch_count].type = PATCH_BNE;
	ctx->branch_patch_count++;

	ASM {
                BNE(0);
        }

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
                /* r11 = env */
                /* r12 = &env->frame->tmpvar[0] */

                /* r0 = &env->frame->tmpvar[src1].val.i */
                MOVW            (REG_R0, (uint32_t)src1);       /* src1 */
                ADD             (REG_R0, REG_R0, REG_R12);
                LDR             (REG_R0, REG_R0, 8);

                /* r1 = &env->frame->tmpvar[src2].val.i */
                MOVW            (REG_R1, (uint32_t)src2);       /* src2 */
                ADD             (REG_R1, REG_R1, REG_R12);
                LDR             (REG_R1, REG_R1, 8);

                /* src1 == src2 */
                CMP_R0_R1       ();
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
        uint32_t len, hash, src;

        CONSUME_TMPVAR(dst);
        CONSUME_STRING(src_s, len, hash);
        src = (uint32_t)src_s;

        /* if (!ex_loadsymbol_helper(env, dst, src, len, hash)) return false; */
        ASM {
                /* r11 = env */
                /* r12 = &env->frame->tmpvar[0] */

                PUSH            (REG_R4);
                PUSH            (REG_R11);
                PUSH            (REG_R12);
                PUSH            (REG_LR);

                /* Arg1 r0: env */
                MOV             (REG_R0, REG_R11);

                /* Arg2 r1: dst */
                MOVW            (REG_R1, (uint32_t)dst);

                /* Arg3 r2: src */
                MOVW            (REG_R2, src & 0xffff);
                MOVT            (REG_R2, (src >> 16) & 0xffff);

                /* Arg4 r3: len */
                MOVW            (REG_R3, len & 0xffff);
                MOVT            (REG_R3, (len >> 16) & 0xffff);

                /* Arg5 [sp+0]: hash */
                MOVW            (REG_R4, hash & 0xffff);
                MOVT            (REG_R4, (hash >> 16) & 0xffff);
                SUB_IMM         (REG_SP, REG_SP, 16);
                STR             (REG_R4, REG_SP, 0);

                /* Call ex_loadsymbol_helper(). */
                MOVW            (REG_R4, (uint32_t)ex_loadsymbol_helper & 0xffff);
                MOVT            (REG_R4, ((uint32_t)ex_loadsymbol_helper >> 16) & 0xffff);
                BLX             (REG_R4);
                ADD_IMM         (REG_SP, REG_SP, IMM16(16));

                /* If failed: */
                CMP_IMM         (REG_R0, 0);
                POP             (REG_LR);
                POP             (REG_R12);
                POP             (REG_R11);
                POP             (REG_R4);
                BEQ             ((uint32_t)ctx->exception_code - (uint32_t)ctx->code);
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
        dst = (uint32_t)dst_s;

        /* if (!ex_storesymbol_helper(env, dst, len, hash, src)) return false; */
        ASM {
                /* r11 = env */
                /* r12 = &env->frame->tmpvar[0] */

                PUSH            (REG_R4);
                PUSH            (REG_R11);
                PUSH            (REG_R12);
                PUSH            (REG_LR);

                /* Arg1 r0: env */
                MOV             (REG_R0, REG_R11);

                /* Arg2 r1: dst */
                MOVW            (REG_R1, dst & 0xffff);
                MOVT            (REG_R1, (dst >> 16) & 0xffff);

                /* Arg3 r2: len */
                MOVW            (REG_R2, len & 0xffff);
                MOVT            (REG_R2, (len >> 16) & 0xffff);

                /* Arg4 r3: hash */
                MOVW            (REG_R3, hash & 0xffff);
                MOVT            (REG_R3, (hash >> 16) & 0xffff);

                /* Arg4 [sp+0]: src */
                MOVW            (REG_R4, (uint32_t)src);
                SUB_IMM         (REG_SP, REG_SP, 16);
                STR             (REG_R4, REG_SP, 0);

                /* Call ex_storesymbol_helper(). */
                MOVW            (REG_R4, (uint32_t)ex_storesymbol_helper & 0xffff);
                MOVT            (REG_R4, ((uint32_t)ex_storesymbol_helper >> 16) & 0xffff);
                BLX             (REG_R4);
                ADD_IMM         (REG_SP, REG_SP, IMM16(16));

                /* If failed: */
                CMP_IMM         (REG_R0, 0);
                POP             (REG_LR);
                POP             (REG_R12);
                POP             (REG_R11);
                POP             (REG_R4);
                BEQ             ((uint32_t)ctx->exception_code - (uint32_t)ctx->code);
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
        field = (uint32_t)field_s;

        /* if (!ex_loaddot_helper(env, dst, dict, field, len, hash)) return false; */
        ASM {
                /* r11 = env */
                /* r12 = &env->frame->tmpvar[0] */

                PUSH            (REG_R4);
                PUSH            (REG_R11);
                PUSH            (REG_R12);
                PUSH            (REG_LR);

                /* Arg1 r0: env */
                MOV             (REG_R0, REG_R11);

                /* Arg2 r1: dst */
                MOVW            (REG_R1, (uint32_t)dst);

                /* Arg3 r2: dict */
                MOVW            (REG_R2, (uint32_t)dict);

                /* Arg4 r3: field */
                MOVW            (REG_R3, field & 0xffff);
                MOVT            (REG_R3, (field >> 16) & 0xffff);

                /* Arg5 [sp+0]: len */
                MOVW            (REG_R4, len & 0xffff);
                MOVT            (REG_R4, (len >> 16) & 0xffff);
                SUB_IMM         (REG_SP, REG_SP, 16);
                STR             (REG_R4, REG_SP, 0);

                /* Arg6 [sp+4]: hash */
                MOVW            (REG_R4, hash & 0xffff);
                MOVT            (REG_R4, (hash >> 16) & 0xffff);
                STR             (REG_R4, REG_SP, 4);

                /* Call ex_loaddot_helper(). */
                MOVW            (REG_R4, (uint32_t)ex_loaddot_helper & 0xffff);
                MOVT            (REG_R4, ((uint32_t)ex_loaddot_helper >> 16) & 0xffff);
                BLX             (REG_R4);
                ADD_IMM         (REG_SP, REG_SP, IMM16(16));

                /* If failed: */
                CMP_IMM         (REG_R0, 0);
                POP             (REG_LR);
                POP             (REG_R12);
                POP             (REG_R11);
                POP             (REG_R4);
                BEQ             ((uint32_t)ctx->exception_code - (uint32_t)ctx->code);
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
        field = (uint32_t)field_s;

        /* if (!ex_storedot_helper(env, dict, field, len, hash, src)) return false; */
        ASM {
                /* r11 = env */
                /* r12 = &env->frame->tmpvar[0] */

                PUSH            (REG_R4);
                PUSH            (REG_R11);
                PUSH            (REG_R12);
                PUSH            (REG_LR);

                /* Arg1 r0: env */
                MOV             (REG_R0, REG_R11);

                /* Arg2 r1: dict */
                MOVW            (REG_R1, (uint32_t)dict);

                /* Arg3 r2: field */
                MOVW            (REG_R2, field & 0xffff);
                MOVT            (REG_R2, (field >> 16) & 0xffff);

                /* Arg4 r3: len */
                MOVW            (REG_R3, len & 0xffff);
                MOVT            (REG_R3, (len >> 16) & 0xffff);

                /* Arg5 [sp+0]: hash */
                MOVW            (REG_R4, hash & 0xffff);
                MOVT            (REG_R4, (hash >> 16) & 0xffff);
                SUB_IMM         (REG_SP, REG_SP, 16);
                STR             (REG_R4, REG_SP, 0);

                /* Arg6 [sp+4]: src */
                MOVW            (REG_R4, (uint32_t)src);
                STR             (REG_R4, REG_SP, 4);

                /* Call ex_storedot_helper(). */
                MOVW            (REG_R4, (uint32_t)ex_storedot_helper & 0xffff);
                MOVT            (REG_R4, ((uint32_t)ex_storedot_helper >> 16) & 0xffff);
                BLX             (REG_R4);
                ADD_IMM         (REG_SP, REG_SP, IMM16(16));

                /* If failed: */
                CMP_IMM         (REG_R0, 0);
                POP             (REG_LR);
                POP             (REG_R12);
                POP             (REG_R11);
                POP             (REG_R4);
                BEQ             ((uint32_t)ctx->exception_code - (uint32_t)ctx->code);
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
                        BAL             ((uint32_t)(4 + 4 * arg_count));
                }
                arg_addr = (uint32_t)ctx->code;
                for (i = 0; i < arg_count; i++) {
                        *(uint32_t *)ctx->code = (uint32_t)arg[i];
                        ctx->code = (uint32_t *)ctx->code + 1;
                }
        } else {
                arg_addr = 0;
        }

        /* if (!ex_call_helper(env, dst, func, arg_count, arg)) return false; */
        ASM {
                /* r11 = env */
                /* r12 = &env->frame->tmpvar[0] */

                PUSH            (REG_R4);
                PUSH            (REG_R11);
                PUSH            (REG_R12);
                PUSH            (REG_LR);

                /* Arg1 r0: env */
                MOV             (REG_R0, REG_R11);

                /* Arg2 r1: dst */
                MOVW            (REG_R1, (uint32_t)dst);

                /* Arg3 r2: func */
                MOVW            (REG_R2, (uint32_t)func);

                /* Arg4 r3: arg_count */
                MOVW            (REG_R3, (uint32_t)arg_count);

                /* Arg5 [sp+0]: arg */
                MOVW            (REG_R4, arg_addr & 0xffff);
                MOVT            (REG_R4, (arg_addr >> 16) & 0xffff);
                SUB_IMM         (REG_SP, REG_SP, 16);
                STR             (REG_R4, REG_SP, 0);

                /* Call ex_call_helper(). */
                MOVW            (REG_R4, (uint32_t)ex_call_helper & 0xffff);
                MOVT            (REG_R4, ((uint32_t)ex_call_helper >> 16) & 0xffff);
                BLX             (REG_R4);
                ADD_IMM         (REG_SP, REG_SP, 16);

                /* If failed: */
                POP             (REG_LR);
                POP             (REG_R12);
                POP             (REG_R11);
                POP             (REG_R4);
                CMP_IMM         (REG_R0, 0);
                BEQ             ((uint32_t)ctx->exception_code - (uint32_t)ctx->code);
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

        /* Embed arguments. */
        ASM {
                BAL             ((uint32_t)(4 + 4 * arg_count));
        }
        arg_addr = (uint32_t)ctx->code;
        for (i = 0; i < arg_count; i++) {
                *(uint32_t *)ctx->code = (uint32_t)arg[i];
                ctx->code = (uint32_t *)ctx->code + 1;
        }

        /* if (!ex_thiscall_helper(env, dst, obj, symbol, len, hash, arg_count, arg)) return false; */
        ASM {
                /* r11 = env */
                /* r12 = &env->frame->tmpvar[0] */

                PUSH            (REG_R4);
                PUSH            (REG_R11);
                PUSH            (REG_R12);
                PUSH            (REG_LR);

                /* Arg1 r0: env */
                MOV             (REG_R0, REG_R11);

                /* Arg2 r1: dst */
                MOVW            (REG_R1, (uint32_t)dst);

                /* Arg3 r2: obj */
                MOVW            (REG_R2, (uint32_t)obj);

                /* Arg4 r3: symbol */
                MOVW            (REG_R3, (uint32_t)symbol & 0xffff);
                MOVT            (REG_R3, ((uint32_t)symbol >> 16) & 0xffff);

                /* Arg5 [sp+0]: len */
                MOVW            (REG_R4, len & 0xffff);
                MOVT            (REG_R4, (len >> 16) & 0xffff);
                SUB_IMM         (REG_SP, REG_SP, 16);
                STR             (REG_R4, REG_SP, 0);

                /* Arg6 [sp+4]: hash */
                MOVW            (REG_R4, hash & 0xffff);
                MOVT            (REG_R4, (hash >> 16) & 0xffff);
                STR             (REG_R4, REG_SP, 4);

                /* Arg7 [sp+8]: arg_count */
                MOVW            (REG_R4, (uint32_t)arg_count);
                STR             (REG_R4, REG_SP, 8);

                /* Arg8 [sp+12]: arg */
                MOVW            (REG_R4, arg_addr & 0xffff);
                MOVT            (REG_R4, (arg_addr >> 16) & 0xffff);
                STR             (REG_R4, REG_SP, 12);

                /* Call ex_thiscall_helper(). */
                MOVW            (REG_R4, (uint32_t)ex_thiscall_helper & 0xffff);
                MOVT            (REG_R4, ((uint32_t)ex_thiscall_helper >> 16) & 0xffff);
                BLX             (REG_R4);

                /* If failed: */
                ADD_IMM         (REG_SP, REG_SP, 16);
                POP             (REG_LR);
                POP             (REG_R12);
                POP             (REG_R11);
                POP             (REG_R4);
                CMP_IMM         (REG_R0, 0);
                BEQ             ((uint32_t)ctx->exception_code - (uint32_t)ctx->code);
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
        if (target_lpc >= (uint32_t)ctx->func->bytecode_size + 1) {
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
                BAL     (0);
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
        if (target_lpc >= (uint32_t)ctx->func->bytecode_size + 1) {
                rt_error(ctx->env, BROKEN_BYTECODE);
                return false;
        }

        src *= (int)sizeof(struct rt_value);

        ASM {
                /* r11 = env */
                /* r12 = &env->frame->tmpvar[0] */

                /* r0 = &env->frame->tmpvar[src].val.i */
                MOVW    (REG_R0, (uint32_t)src);
                ADD     (REG_R0, REG_R0, REG_R12);
                LDR     (REG_R1, REG_R0, 8);

                /* Compare: env->frame->tmpvar[dst].val.i == 1 */
                CMP_IMM (REG_R1, 0);
        }

        /* Patch later. */
        ctx->branch_patch[ctx->branch_patch_count].code = ctx->code;
        ctx->branch_patch[ctx->branch_patch_count].lpc = target_lpc;
        ctx->branch_patch[ctx->branch_patch_count].type = PATCH_BNE;
        ctx->branch_patch_count++;

        ASM {
                /* Patched later. */
                BNE     (0);
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
        if (target_lpc >= (uint32_t)ctx->func->bytecode_size + 1) {
                rt_error(ctx->env, BROKEN_BYTECODE);
                return false;
        }

        src *= (int)sizeof(struct rt_value);

        ASM {
                /* r11 = env */
                /* r12 = &env->frame->tmpvar[0] */

                /* r0 = &env->frame->tmpvar[src].val.i */
                MOVW    (REG_R0, (uint32_t)src);
                ADD     (REG_R0, REG_R0, REG_R12);
                LDR     (REG_R1, REG_R0, 8);

                /* Compare: env->frame->tmpvar[dst].val.i == 0 */
                CMP_IMM (REG_R1, IMM12(0));
        }
        
        /* Patch later. */
        ctx->branch_patch[ctx->branch_patch_count].code = ctx->code;
        ctx->branch_patch[ctx->branch_patch_count].lpc = target_lpc;
        ctx->branch_patch[ctx->branch_patch_count].type = PATCH_BEQ;
        ctx->branch_patch_count++;

        ASM {
                /* Patched later. */
                BEQ     (0);
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
        if (target_lpc >= (uint32_t)ctx->func->bytecode_size + 1) {
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
                BEQ     (0);
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
                /* r11 = env */
                /* r12 = &env->frame->tmpvar[0] */

                PUSH            (REG_R4);
                PUSH            (REG_R11);
                PUSH            (REG_R12);
                PUSH            (REG_LR);

                /* Arg1 r0: env */
                MOV             (REG_R0, REG_R11);

                /* Call ex_safepoint_helper(). */
                MOVW            (REG_R4, (uint32_t)ex_safepoint_helper & 0xffff);
                MOVT            (REG_R4, ((uint32_t)ex_safepoint_helper >> 16) & 0xffff);
                BLX             (REG_R4);
                ADD_IMM         (REG_SP, REG_SP, IMM16(16));

                /* If failed: */
                CMP_IMM         (REG_R0, 0);
                POP             (REG_LR);
                POP             (REG_R12);
                POP             (REG_R11);
                POP             (REG_R4);
                BEQ             ((uint32_t)ctx->exception_code - (uint32_t)ctx->code);
        }

        return true;
}

/* Visit a OP_PBASE instruction. */
static INLINE bool
jit_visit_pbase_op(
        struct rt_jit_context *ctx)
{
        int dst;
        int src;
        int base_id;
        uint32_t buf_ofs;

        UNUSED_PARAMETER(base_id);

        CONSUME_TMPVAR(dst);
        CONSUME_TMPVAR(src);
        CONSUME_IMM8(base_id);

        dst *= (int)sizeof(struct rt_value);
        src *= (int)sizeof(struct rt_value);
        buf_ofs = (uint32_t)offsetof(struct rt_packed, packed_buffer);

        ASM {
                /* r12 = &env->frame->tmpvar[0] */

                MOVW    (REG_R0, (uint32_t)(src + 8));
                ADD     (REG_R0, REG_R0, REG_R12);
                LDR     (REG_R0, REG_R0, 0);
                MOVW    (REG_R1, buf_ofs);
                ADD     (REG_R0, REG_R0, REG_R1);
                LDR     (REG_R1, REG_R0, 0);
                MOVW    (REG_R0, (uint32_t)dst);
                ADD     (REG_R0, REG_R0, REG_R12);
                MOVW    (REG_R2, (uint32_t)NOCT_VALUE_LONG);
                STR     (REG_R2, REG_R0, 0);
                STR     (REG_R1, REG_R0, 8);
                MOVW    (REG_R2, 0);
                STR     (REG_R2, REG_R0, 12);
        }

        return true;
}

/* Visit a OP_PLEN instruction. */
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

/* Visit a OP_PCHECK instruction. */
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

/* Visit a OP_TYPEIS instruction. */
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

/* Visit a OP_PLOAD8U instruction. */
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
                /* r12 = &env->frame->tmpvar[0] */

                /* r0 = base pointer (low word of the long) */
                MOVW    (REG_R0, (uint32_t)(base + 8));
                ADD     (REG_R0, REG_R0, REG_R12);
                LDR     (REG_R0, REG_R0, 0);

                /* r1 = element index */
                MOVW    (REG_R1, (uint32_t)(ofs + 8));
                ADD     (REG_R1, REG_R1, REG_R12);
                LDR     (REG_R1, REG_R1, 0);
                ADD     (REG_R0, REG_R0, REG_R1);

                /* r1 = element */
                LDRB0   (REG_R1, REG_R0);

                /* dst slot */
                MOVW    (REG_R0, (uint32_t)dst);
                ADD     (REG_R0, REG_R0, REG_R12);
                MOVW    (REG_R2, (uint32_t)NOCT_VALUE_INT);
                STR     (REG_R2, REG_R0, 0);
                STR     (REG_R1, REG_R0, 8);
        }

        return true;
}

/* Visit a OP_PSTORE8 instruction. */
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
                /* r12 = &env->frame->tmpvar[0] */

                MOVW    (REG_R0, (uint32_t)(base + 8));
                ADD     (REG_R0, REG_R0, REG_R12);
                LDR     (REG_R0, REG_R0, 0);
                MOVW    (REG_R1, (uint32_t)(ofs + 8));
                ADD     (REG_R1, REG_R1, REG_R12);
                LDR     (REG_R1, REG_R1, 0);
                ADD     (REG_R0, REG_R0, REG_R1);
                MOVW    (REG_R1, (uint32_t)(src + 8));
                ADD     (REG_R1, REG_R1, REG_R12);
                LDR     (REG_R1, REG_R1, 0);
                STRB0   (REG_R1, REG_R0);
        }

        return true;
}

/* Visit a OP_CHECKTYPE instruction. */
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

/* Visit a OP_PLOAD8S instruction. */
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
                /* r12 = &env->frame->tmpvar[0] */

                /* r0 = base pointer (low word of the long) */
                MOVW    (REG_R0, (uint32_t)(base + 8));
                ADD     (REG_R0, REG_R0, REG_R12);
                LDR     (REG_R0, REG_R0, 0);

                /* r1 = element index */
                MOVW    (REG_R1, (uint32_t)(ofs + 8));
                ADD     (REG_R1, REG_R1, REG_R12);
                LDR     (REG_R1, REG_R1, 0);
                ADD     (REG_R0, REG_R0, REG_R1);

                /* r1 = element */
                LDRSB0   (REG_R1, REG_R0);

                /* dst slot */
                MOVW    (REG_R0, (uint32_t)dst);
                ADD     (REG_R0, REG_R0, REG_R12);
                MOVW    (REG_R2, (uint32_t)NOCT_VALUE_INT);
                STR     (REG_R2, REG_R0, 0);
                STR     (REG_R1, REG_R0, 8);
        }

        return true;
}

/* Visit a OP_PLOAD16U instruction. */
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
                /* r12 = &env->frame->tmpvar[0] */

                /* r0 = base pointer (low word of the long) */
                MOVW    (REG_R0, (uint32_t)(base + 8));
                ADD     (REG_R0, REG_R0, REG_R12);
                LDR     (REG_R0, REG_R0, 0);

                /* r1 = element index */
                MOVW    (REG_R1, (uint32_t)(ofs + 8));
                ADD     (REG_R1, REG_R1, REG_R12);
                LDR     (REG_R1, REG_R1, 0);
                LSL_IMM (REG_R1, REG_R1, 1);
                ADD     (REG_R0, REG_R0, REG_R1);

                /* r1 = element */
                LDRH0   (REG_R1, REG_R0);

                /* dst slot */
                MOVW    (REG_R0, (uint32_t)dst);
                ADD     (REG_R0, REG_R0, REG_R12);
                MOVW    (REG_R2, (uint32_t)NOCT_VALUE_INT);
                STR     (REG_R2, REG_R0, 0);
                STR     (REG_R1, REG_R0, 8);
        }

        return true;
}

/* Visit a OP_PLOAD16S instruction. */
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
                /* r12 = &env->frame->tmpvar[0] */

                /* r0 = base pointer (low word of the long) */
                MOVW    (REG_R0, (uint32_t)(base + 8));
                ADD     (REG_R0, REG_R0, REG_R12);
                LDR     (REG_R0, REG_R0, 0);

                /* r1 = element index */
                MOVW    (REG_R1, (uint32_t)(ofs + 8));
                ADD     (REG_R1, REG_R1, REG_R12);
                LDR     (REG_R1, REG_R1, 0);
                LSL_IMM (REG_R1, REG_R1, 1);
                ADD     (REG_R0, REG_R0, REG_R1);

                /* r1 = element */
                LDRSH0   (REG_R1, REG_R0);

                /* dst slot */
                MOVW    (REG_R0, (uint32_t)dst);
                ADD     (REG_R0, REG_R0, REG_R12);
                MOVW    (REG_R2, (uint32_t)NOCT_VALUE_INT);
                STR     (REG_R2, REG_R0, 0);
                STR     (REG_R1, REG_R0, 8);
        }

        return true;
}

/* Visit a OP_PLOAD32 instruction. */
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
                /* r12 = &env->frame->tmpvar[0] */

                /* r0 = base pointer (low word of the long) */
                MOVW    (REG_R0, (uint32_t)(base + 8));
                ADD     (REG_R0, REG_R0, REG_R12);
                LDR     (REG_R0, REG_R0, 0);

                /* r1 = element index */
                MOVW    (REG_R1, (uint32_t)(ofs + 8));
                ADD     (REG_R1, REG_R1, REG_R12);
                LDR     (REG_R1, REG_R1, 0);
                LSL_IMM (REG_R1, REG_R1, 2);
                ADD     (REG_R0, REG_R0, REG_R1);

                /* r1 = element */
                LDR     (REG_R1, REG_R0, 0);

                /* dst slot */
                MOVW    (REG_R0, (uint32_t)dst);
                ADD     (REG_R0, REG_R0, REG_R12);
                MOVW    (REG_R2, (uint32_t)NOCT_VALUE_INT);
                STR     (REG_R2, REG_R0, 0);
                STR     (REG_R1, REG_R0, 8);
        }

        return true;
}

/* Visit a OP_PLOAD64 instruction. */
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

/* Visit a OP_PSTORE16 instruction. */
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
                /* r12 = &env->frame->tmpvar[0] */

                MOVW    (REG_R0, (uint32_t)(base + 8));
                ADD     (REG_R0, REG_R0, REG_R12);
                LDR     (REG_R0, REG_R0, 0);
                MOVW    (REG_R1, (uint32_t)(ofs + 8));
                ADD     (REG_R1, REG_R1, REG_R12);
                LDR     (REG_R1, REG_R1, 0);
                LSL_IMM (REG_R1, REG_R1, 1);
                ADD     (REG_R0, REG_R0, REG_R1);
                MOVW    (REG_R1, (uint32_t)(src + 8));
                ADD     (REG_R1, REG_R1, REG_R12);
                LDR     (REG_R1, REG_R1, 0);
                STRH0   (REG_R1, REG_R0);
        }

        return true;
}

/* Visit a OP_PSTORE32 instruction. */
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
                /* r12 = &env->frame->tmpvar[0] */

                MOVW    (REG_R0, (uint32_t)(base + 8));
                ADD     (REG_R0, REG_R0, REG_R12);
                LDR     (REG_R0, REG_R0, 0);
                MOVW    (REG_R1, (uint32_t)(ofs + 8));
                ADD     (REG_R1, REG_R1, REG_R12);
                LDR     (REG_R1, REG_R1, 0);
                LSL_IMM (REG_R1, REG_R1, 2);
                ADD     (REG_R0, REG_R0, REG_R1);
                MOVW    (REG_R1, (uint32_t)(src + 8));
                ADD     (REG_R1, REG_R1, REG_R12);
                LDR     (REG_R1, REG_R1, 0);
                STR     (REG_R1, REG_R0, 0);
        }

        return true;
}

/* Visit a OP_PSTORE64 instruction. */
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

/* Visit a OP_PLOADF32 instruction. */
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

/* Visit a OP_PSTOREF32 instruction. */
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

#if defined(NOCT_USE_OPTIMIZER)
/* Visit vector instructions with NEON or direct ARM/VFP lowering. */
/* Visit an OP_VLOADI32X4 instruction. */
static INLINE bool
jit_visit_vloadi32x4_op(
        struct rt_jit_context *ctx)
{
        int vd;
        int base_tmp;
        int ofs_tmp;
        int lane;
        int base;
        int ofs;

        CONSUME_IMM8(vd);
        CONSUME_TMPVAR(base_tmp);
        CONSUME_TMPVAR(ofs_tmp);

        if ((ctx->simd_caps & JIT_SIMD_CAP_NEON) != 0) {
                int base = base_tmp * (int)sizeof(struct rt_value);
                int ofs = ofs_tmp * (int)sizeof(struct rt_value);
                uint32_t dd = (uint32_t)(16 + vd * 2);

                ASM {
                        LDR(REG_R2, REG_R12, base + 8);
                        LDR(REG_R3, REG_R12, ofs + 8);
                        LSL_IMM(REG_R3, REG_R3, 2);
                        ADD(REG_R2, REG_R2, REG_R3);
                }
                if (!jit_put_word(ctx, 0xf4200a0f |
                                  ((dd & 16u) << 18) |
                                  ((dd & 15u) << 12) | (2u << 16)))
                        return false;

                return true;
        }

        ASM {
                MOVW(REG_R4, (uint32_t)offsetof(struct rt_env, vreg) & 0xffff);
                MOVT(REG_R4, ((uint32_t)offsetof(struct rt_env, vreg) >> 16) & 0xffff);
                ADD(REG_R4, REG_R11, REG_R4);
        }

        base = base_tmp * (int)sizeof(struct rt_value);
        ofs = ofs_tmp * (int)sizeof(struct rt_value);

        ASM {
                LDR(REG_R0, REG_R12, base + 8);
                LDR(REG_R1, REG_R12, ofs + 8);
                LSL_IMM(REG_R1, REG_R1, 2);
                ADD(REG_R0, REG_R0, REG_R1);
        }
        for (lane = 0; lane < 4; lane++) {
                LDR(REG_R2, REG_R0, lane * 4);
                STR(REG_R2, REG_R4, vd * 16 + lane * 4);
        }

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
        int lane;
        int base;
        int ofs;

        CONSUME_TMPVAR(base_tmp);
        CONSUME_TMPVAR(ofs_tmp);
        CONSUME_IMM8(vs);

        if ((ctx->simd_caps & JIT_SIMD_CAP_NEON) != 0) {
                int base = base_tmp * (int)sizeof(struct rt_value);
                int ofs = ofs_tmp * (int)sizeof(struct rt_value);
                uint32_t dd = (uint32_t)(16 + vs * 2);

                ASM {
                        LDR(REG_R2, REG_R12, base + 8);
                        LDR(REG_R3, REG_R12, ofs + 8);
                        LSL_IMM(REG_R3, REG_R3, 2);
                        ADD(REG_R2, REG_R2, REG_R3);
                }
                if (!jit_put_word(ctx, 0xf4000a0f |
                                  ((dd & 16u) << 18) |
                                  ((dd & 15u) << 12) | (2u << 16)))
                        return false;
                return true;
        }

        ASM {
                MOVW(REG_R4, (uint32_t)offsetof(struct rt_env, vreg) & 0xffff);
                MOVT(REG_R4, ((uint32_t)offsetof(struct rt_env, vreg) >> 16) & 0xffff);
                ADD(REG_R4, REG_R11, REG_R4);
        }

        base = base_tmp * (int)sizeof(struct rt_value);
        ofs = ofs_tmp * (int)sizeof(struct rt_value);

        ASM {
                LDR(REG_R0, REG_R12, base + 8);
                LDR(REG_R1, REG_R12, ofs + 8);
                LSL_IMM(REG_R1, REG_R1, 2);
                ADD(REG_R0, REG_R0, REG_R1);
        }
        for (lane = 0; lane < 4; lane++) {
                LDR(REG_R2, REG_R4, vs * 16 + lane * 4);
                STR(REG_R2, REG_R0, lane * 4);
        }

        return true;
}

/* Visit an OP_VSPLATI32 instruction. */
static INLINE bool
jit_visit_vsplati32_op(
        struct rt_jit_context *ctx)
{
        int vd;
        int src_tmp;
        int lane;
        int src;

        CONSUME_IMM8(vd);
        CONSUME_TMPVAR(src_tmp);

        if ((ctx->simd_caps & JIT_SIMD_CAP_NEON) != 0) {
                uint32_t dd = (uint32_t)(16 + vd * 2);
                int src = src_tmp * (int)sizeof(struct rt_value);

                ASM { LDR(REG_R3, REG_R12, src + 8); }
                if (!jit_put_word(ctx, 0xee800b90 | (3u << 12) |
                                  ((dd & 16u) << 17) |
                                  ((dd & 15u) << 16)))
                        return false;
                return true;
        }

        ASM {
                MOVW(REG_R4, (uint32_t)offsetof(struct rt_env, vreg) & 0xffff);
                MOVT(REG_R4, ((uint32_t)offsetof(struct rt_env, vreg) >> 16) & 0xffff);
                ADD(REG_R4, REG_R11, REG_R4);
        }

        src = src_tmp * (int)sizeof(struct rt_value);
        LDR(REG_R2, REG_R12, src + 8);
        for (lane = 0; lane < 4; lane++)
                STR(REG_R2, REG_R4, vd * 16 + lane * 4);
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
        int d;

        CONSUME_TMPVAR(dst_tmp);
        CONSUME_IMM8(vs);
        CONSUME_IMM8(lane_index);

        if ((ctx->simd_caps & JIT_SIMD_CAP_NEON) != 0) {
                uint32_t dd = (uint32_t)(16 + vs * 2 + lane_index / 2);
                int d = dst_tmp * (int)sizeof(struct rt_value);

                if (!jit_put_word(ctx, 0xee100b90 | (3u << 12) |
                                  ((uint32_t)(lane_index & 1) << 21) |
                                  ((dd & 16u) << 3) |
                                  ((dd & 15u) << 16)))
                        return false;
                ASM {
                        MOVW(REG_R2, NOCT_VALUE_INT);
                        STR(REG_R2, REG_R12, d);
                        STR(REG_R3, REG_R12, d + 8);
                }
                return true;
        }

        ASM {
                MOVW(REG_R4, (uint32_t)offsetof(struct rt_env, vreg) & 0xffff);
                MOVT(REG_R4, ((uint32_t)offsetof(struct rt_env, vreg) >> 16) & 0xffff);
                ADD(REG_R4, REG_R11, REG_R4);
        }

        d = dst_tmp * (int)sizeof(struct rt_value);
        ASM {
                LDR(REG_R1, REG_R4, vs * 16 + lane_index * 4);
                MOVW(REG_R2, (uint32_t)(NOCT_VALUE_INT));
                STR(REG_R2, REG_R12, d);
                STR(REG_R1, REG_R12, d + 8);
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
        int lane;

        CONSUME_IMM8(vd);
        CONSUME_IMM8(vs);

        if ((ctx->simd_caps & JIT_SIMD_CAP_NEON) != 0) {
                if (vd != vs &&
                    !jit_put_word(ctx, neon_q3(0xf2200150,
                                                 vd, vs, vs)))
                        return false;
                return true;
        }

        ASM {
                MOVW(REG_R4, (uint32_t)offsetof(struct rt_env, vreg) & 0xffff);
                MOVT(REG_R4, ((uint32_t)offsetof(struct rt_env, vreg) >> 16) & 0xffff);
                ADD(REG_R4, REG_R11, REG_R4);
        }

        for (lane = 0; lane < 4; lane++) {
                LDR(REG_R2, REG_R4, vs * 16 + lane * 4);
                STR(REG_R2, REG_R4, vd * 16 + lane * 4);
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
        int lane;

        CONSUME_IMM8(vd);
        CONSUME_IMM8(lhs);
        CONSUME_IMM8(rhs);

        if ((ctx->simd_caps & JIT_SIMD_CAP_NEON) != 0) {
                return jit_put_word(ctx, neon_q3(0xf2200840, vd, lhs, rhs));
        }

        ASM {
                MOVW(REG_R4, (uint32_t)offsetof(struct rt_env, vreg) & 0xffff);
                MOVT(REG_R4, ((uint32_t)offsetof(struct rt_env, vreg) >> 16) & 0xffff);
                ADD(REG_R4, REG_R11, REG_R4);
        }

        for (lane = 0; lane < 4; lane++) {
                LDR(REG_R0, REG_R4, lhs * 16 + lane * 4);
                LDR(REG_R1, REG_R4, rhs * 16 + lane * 4);

                if (!jit_put_word(ctx, 0xe0802001))
                        return false;
                STR(REG_R2, REG_R4, vd * 16 + lane * 4);
        }
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
        int lane;

        CONSUME_IMM8(vd);
        CONSUME_IMM8(lhs);
        CONSUME_IMM8(rhs);

        if ((ctx->simd_caps & JIT_SIMD_CAP_NEON) != 0) {
                return jit_put_word(ctx, neon_q3(0xf3200840,
                                                       vd, lhs, rhs));
        }

        ASM {
                MOVW(REG_R4, (uint32_t)offsetof(struct rt_env, vreg) & 0xffff);
                MOVT(REG_R4, ((uint32_t)offsetof(struct rt_env, vreg) >> 16) & 0xffff);
                ADD(REG_R4, REG_R11, REG_R4);
        }

        for (lane = 0; lane < 4; lane++) {
                LDR(REG_R0, REG_R4, lhs * 16 + lane * 4);
                LDR(REG_R1, REG_R4, rhs * 16 + lane * 4);

                if (!jit_put_word(ctx, 0xe0402001))
                        return false;
                STR(REG_R2, REG_R4, vd * 16 + lane * 4);
        }
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
        int lane;

        CONSUME_IMM8(vd);
        CONSUME_IMM8(lhs);
        CONSUME_IMM8(rhs);

        if ((ctx->simd_caps & JIT_SIMD_CAP_NEON) != 0) {
                return jit_put_word(ctx, neon_q3(0xf2200950,
                                                       vd, lhs, rhs));
        }

        ASM {
                MOVW(REG_R4, (uint32_t)offsetof(struct rt_env, vreg) & 0xffff);
                MOVT(REG_R4, ((uint32_t)offsetof(struct rt_env, vreg) >> 16) & 0xffff);
                ADD(REG_R4, REG_R11, REG_R4);
        }

        for (lane = 0; lane < 4; lane++) {
                LDR(REG_R0, REG_R4, lhs * 16 + lane * 4);
                LDR(REG_R1, REG_R4, rhs * 16 + lane * 4);

                if (!jit_put_word(ctx, 0xe0020190))
                        return false;
                STR(REG_R2, REG_R4, vd * 16 + lane * 4);
        }
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
        int lane;

        CONSUME_IMM8(vd);
        CONSUME_IMM8(lhs);
        CONSUME_IMM8(rhs);

        if ((ctx->simd_caps & JIT_SIMD_CAP_NEON) != 0) {
                return jit_put_word(ctx, neon_q3(0xf2000150,
                                                       vd, lhs, rhs));
        }

        ASM {
                MOVW(REG_R4, (uint32_t)offsetof(struct rt_env, vreg) & 0xffff);
                MOVT(REG_R4, ((uint32_t)offsetof(struct rt_env, vreg) >> 16) & 0xffff);
                ADD(REG_R4, REG_R11, REG_R4);
        }

        for (lane = 0; lane < 4; lane++) {
                LDR(REG_R0, REG_R4, lhs * 16 + lane * 4);
                LDR(REG_R1, REG_R4, rhs * 16 + lane * 4);

                if (!jit_put_word(ctx, 0xe0002001))
                        return false;
                STR(REG_R2, REG_R4, vd * 16 + lane * 4);
        }
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
        int lane;

        CONSUME_IMM8(vd);
        CONSUME_IMM8(lhs);
        CONSUME_IMM8(rhs);

        if ((ctx->simd_caps & JIT_SIMD_CAP_NEON) != 0) {
                return jit_put_word(ctx, neon_q3(0xf2200150,
                                                       vd, lhs, rhs));
        }

        ASM {
                MOVW(REG_R4, (uint32_t)offsetof(struct rt_env, vreg) & 0xffff);
                MOVT(REG_R4, ((uint32_t)offsetof(struct rt_env, vreg) >> 16) & 0xffff);
                ADD(REG_R4, REG_R11, REG_R4);
        }

        for (lane = 0; lane < 4; lane++) {
                LDR(REG_R0, REG_R4, lhs * 16 + lane * 4);
                LDR(REG_R1, REG_R4, rhs * 16 + lane * 4);

                if (!jit_put_word(ctx, 0xe1802001))
                        return false;
                STR(REG_R2, REG_R4, vd * 16 + lane * 4);
        }
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
        int lane;

        CONSUME_IMM8(vd);
        CONSUME_IMM8(lhs);
        CONSUME_IMM8(rhs);

        if ((ctx->simd_caps & JIT_SIMD_CAP_NEON) != 0) {
                return jit_put_word(ctx, neon_q3(0xf3000150,
                                                       vd, lhs, rhs));
        }

        ASM {
                MOVW(REG_R4, (uint32_t)offsetof(struct rt_env, vreg) & 0xffff);
                MOVT(REG_R4, ((uint32_t)offsetof(struct rt_env, vreg) >> 16) & 0xffff);
                ADD(REG_R4, REG_R11, REG_R4);
        }

        for (lane = 0; lane < 4; lane++) {
                LDR(REG_R0, REG_R4, lhs * 16 + lane * 4);
                LDR(REG_R1, REG_R4, rhs * 16 + lane * 4);

                if (!jit_put_word(ctx, 0xe0202001))
                        return false;
                STR(REG_R2, REG_R4, vd * 16 + lane * 4);
        }
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
        int lane;

        CONSUME_IMM8(vd);
        CONSUME_IMM8(vs);
        CONSUME_IMM8(shift);

        if ((ctx->simd_caps & JIT_SIMD_CAP_NEON) != 0) {
                return jit_put_word(ctx, neon_q2_imm(0xf2800550,
                                                            vd, vs,
                                                            32u + (uint32_t)shift));
        }

        ASM {
                MOVW(REG_R4, (uint32_t)offsetof(struct rt_env, vreg) & 0xffff);
                MOVT(REG_R4, ((uint32_t)offsetof(struct rt_env, vreg) >> 16) & 0xffff);
                ADD(REG_R4, REG_R11, REG_R4);
        }

        for (lane = 0; lane < 4; lane++) {
                uint32_t word;

                LDR(REG_R0, REG_R4, vs * 16 + lane * 4);
                word = (0xe1a02000) |
                       ((uint32_t)shift << 7);
                if (!jit_put_word(ctx, word))
                        return false;
                STR(REG_R2, REG_R4, vd * 16 + lane * 4);
        }
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
        int lane;

        CONSUME_IMM8(vd);
        CONSUME_IMM8(vs);
        CONSUME_IMM8(shift);

        if ((ctx->simd_caps & JIT_SIMD_CAP_NEON) != 0) {
                return jit_put_word(ctx, neon_q2_imm(0xf3800050,
                                                            vd, vs,
                                                            64u - (uint32_t)shift));
        }

        ASM {
                MOVW(REG_R4, (uint32_t)offsetof(struct rt_env, vreg) & 0xffff);
                MOVT(REG_R4, ((uint32_t)offsetof(struct rt_env, vreg) >> 16) & 0xffff);
                ADD(REG_R4, REG_R11, REG_R4);
        }

        for (lane = 0; lane < 4; lane++) {
                uint32_t word;

                LDR(REG_R0, REG_R4, vs * 16 + lane * 4);
                word = (0xe1a02020) |
                       ((uint32_t)shift << 7);
                if (!jit_put_word(ctx, word))
                        return false;
                STR(REG_R2, REG_R4, vd * 16 + lane * 4);
        }
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
        int lane;
        int base;
        int ofs;

        CONSUME_IMM8(vd);
        CONSUME_TMPVAR(base_tmp);
        CONSUME_TMPVAR(ofs_tmp);

        if ((ctx->simd_caps & JIT_SIMD_CAP_NEON) != 0) {
                int base = base_tmp * (int)sizeof(struct rt_value);
                int ofs = ofs_tmp * (int)sizeof(struct rt_value);
                uint32_t dd = (uint32_t)(16 + vd * 2);

                ASM {
                        LDR(REG_R2, REG_R12, base + 8);
                        LDR(REG_R3, REG_R12, ofs + 8);
                        LSL_IMM(REG_R3, REG_R3, 2);
                        ADD(REG_R2, REG_R2, REG_R3);
                }
                if (!jit_put_word(ctx, 0xf4200a0f |
                                  ((dd & 16u) << 18) |
                                  ((dd & 15u) << 12) | (2u << 16)))
                        return false;
                return true;
        }

        ASM {
                MOVW(REG_R4, (uint32_t)offsetof(struct rt_env, vreg) & 0xffff);
                MOVT(REG_R4, ((uint32_t)offsetof(struct rt_env, vreg) >> 16) & 0xffff);
                ADD(REG_R4, REG_R11, REG_R4);
        }

        base = base_tmp * (int)sizeof(struct rt_value);
        ofs = ofs_tmp * (int)sizeof(struct rt_value);
        ASM {
                LDR(REG_R0, REG_R12, base + 8);
                LDR(REG_R1, REG_R12, ofs + 8);
                LSL_IMM(REG_R1, REG_R1, 2);
                ADD(REG_R0, REG_R0, REG_R1);
        }
        for (lane = 0; lane < 4; lane++) {
                LDR(REG_R2, REG_R0, lane * 4);
                STR(REG_R2, REG_R4, vd * 16 + lane * 4);
        }
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
        int lane;
        int base;
        int ofs;

        CONSUME_TMPVAR(base_tmp);
        CONSUME_TMPVAR(ofs_tmp);
        CONSUME_IMM8(vs);

        if ((ctx->simd_caps & JIT_SIMD_CAP_NEON) != 0) {
                int base = base_tmp * (int)sizeof(struct rt_value);
                int ofs = ofs_tmp * (int)sizeof(struct rt_value);
                uint32_t dd = (uint32_t)(16 + vs * 2);

                ASM {
                        LDR(REG_R2, REG_R12, base + 8);
                        LDR(REG_R3, REG_R12, ofs + 8);
                        LSL_IMM(REG_R3, REG_R3, 2);
                        ADD(REG_R2, REG_R2, REG_R3);
                }
                if (!jit_put_word(ctx, 0xf4000a0f |
                                  ((dd & 16u) << 18) |
                                  ((dd & 15u) << 12) | (2u << 16)))
                        return false;
                return true;
        }

        ASM {
                MOVW(REG_R4, (uint32_t)offsetof(struct rt_env, vreg) & 0xffff);
                MOVT(REG_R4, ((uint32_t)offsetof(struct rt_env, vreg) >> 16) & 0xffff);
                ADD(REG_R4, REG_R11, REG_R4);
        }

        base = base_tmp * (int)sizeof(struct rt_value);
        ofs = ofs_tmp * (int)sizeof(struct rt_value);
        ASM {
                LDR(REG_R0, REG_R12, base + 8);
                LDR(REG_R1, REG_R12, ofs + 8);
                LSL_IMM(REG_R1, REG_R1, 2);
                ADD(REG_R0, REG_R0, REG_R1);
        }
        for (lane = 0; lane < 4; lane++) {
                LDR(REG_R2, REG_R4, vs * 16 + lane * 4);
                STR(REG_R2, REG_R0, lane * 4);
        }
        return true;
}

/* Visit an OP_VSPLATF32 instruction. */
static INLINE bool
jit_visit_vsplatf32_op(
        struct rt_jit_context *ctx)
{
        int vd;
        int src_tmp;
        int lane;
        int src;

        CONSUME_IMM8(vd);
        CONSUME_TMPVAR(src_tmp);

        if ((ctx->simd_caps & JIT_SIMD_CAP_NEON) != 0) {
                uint32_t dd = (uint32_t)(16 + vd * 2);
                int src = src_tmp * (int)sizeof(struct rt_value);

                ASM { LDR(REG_R3, REG_R12, src + 8); }
                if (!jit_put_word(ctx, 0xee800b90 | (3u << 12) |
                                  ((dd & 16u) << 17) |
                                  ((dd & 15u) << 16)))
                        return false;
                return true;
        }

        ASM {
                MOVW(REG_R4, (uint32_t)offsetof(struct rt_env, vreg) & 0xffff);
                MOVT(REG_R4, ((uint32_t)offsetof(struct rt_env, vreg) >> 16) & 0xffff);
                ADD(REG_R4, REG_R11, REG_R4);
        }

        src = src_tmp * (int)sizeof(struct rt_value);
        LDR(REG_R2, REG_R12, src + 8);
        for (lane = 0; lane < 4; lane++)
                STR(REG_R2, REG_R4, vd * 16 + lane * 4);
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
        int d;

        CONSUME_TMPVAR(dst_tmp);
        CONSUME_IMM8(vs);
        CONSUME_IMM8(lane_index);

        if ((ctx->simd_caps & JIT_SIMD_CAP_NEON) != 0) {
                uint32_t dd = (uint32_t)(16 + vs * 2 + lane_index / 2);
                int d = dst_tmp * (int)sizeof(struct rt_value);

                if (!jit_put_word(ctx, 0xee100b90 | (3u << 12) |
                                  ((uint32_t)(lane_index & 1) << 21) |
                                  ((dd & 16u) << 3) |
                                  ((dd & 15u) << 16)))
                        return false;
                ASM {
                        MOVW(REG_R2, NOCT_VALUE_FLOAT);
                        STR(REG_R2, REG_R12, d);
                        STR(REG_R3, REG_R12, d + 8);
                }
                return true;
        }

        ASM {
                MOVW(REG_R4, (uint32_t)offsetof(struct rt_env, vreg) & 0xffff);
                MOVT(REG_R4, ((uint32_t)offsetof(struct rt_env, vreg) >> 16) & 0xffff);
                ADD(REG_R4, REG_R11, REG_R4);
        }

        d = dst_tmp * (int)sizeof(struct rt_value);
        ASM {
                LDR(REG_R1, REG_R4, vs * 16 + lane_index * 4);
                MOVW(REG_R2, (uint32_t)(NOCT_VALUE_FLOAT));
                STR(REG_R2, REG_R12, d);
                STR(REG_R1, REG_R12, d + 8);
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
        int lane;

        CONSUME_IMM8(vd);
        CONSUME_IMM8(lhs);
        CONSUME_IMM8(rhs);

        if ((ctx->simd_caps & JIT_SIMD_CAP_NEON) != 0) {
                return jit_put_word(ctx, neon_q3(0xf2000d40,
                                               vd, lhs, rhs));
        }

        ASM {
                MOVW(REG_R4, (uint32_t)offsetof(struct rt_env, vreg) & 0xffff);
                MOVT(REG_R4, ((uint32_t)offsetof(struct rt_env, vreg) >> 16) & 0xffff);
                ADD(REG_R4, REG_R11, REG_R4);
        }

        for (lane = 0; lane < 4; lane++) {
                uint32_t a;
                uint32_t b;
                uint32_t d;
                uint32_t word;

                a = (uint32_t)(lhs * 16 + lane * 4) / 4;
                b = (uint32_t)(rhs * 16 + lane * 4) / 4;
                d = (uint32_t)(vd * 16 + lane * 4) / 4;
                /* vldr s0,[r4,#a]; vldr s1,[r4,#b] */
                if (!jit_put_word(ctx, 0xed940a00 | a) ||
                    !jit_put_word(ctx, 0xedd40a00 | b))
                        return false;
                word = 0xee301a20;
                if (!jit_put_word(ctx, word) ||
                    !jit_put_word(ctx, 0xed841a00 | d))
                        return false;
        }
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
        int lane;

        CONSUME_IMM8(vd);
        CONSUME_IMM8(lhs);
        CONSUME_IMM8(rhs);

        if ((ctx->simd_caps & JIT_SIMD_CAP_NEON) != 0) {
                return jit_put_word(ctx, neon_q3(0xf2200d40,
                                               vd, lhs, rhs));
        }

        ASM {
                MOVW(REG_R4, (uint32_t)offsetof(struct rt_env, vreg) & 0xffff);
                MOVT(REG_R4, ((uint32_t)offsetof(struct rt_env, vreg) >> 16) & 0xffff);
                ADD(REG_R4, REG_R11, REG_R4);
        }

        for (lane = 0; lane < 4; lane++) {
                uint32_t a;
                uint32_t b;
                uint32_t d;
                uint32_t word;

                a = (uint32_t)(lhs * 16 + lane * 4) / 4;
                b = (uint32_t)(rhs * 16 + lane * 4) / 4;
                d = (uint32_t)(vd * 16 + lane * 4) / 4;
                /* vldr s0,[r4,#a]; vldr s1,[r4,#b] */
                if (!jit_put_word(ctx, 0xed940a00 | a) ||
                    !jit_put_word(ctx, 0xedd40a00 | b))
                        return false;
                word = 0xee301a60;
                if (!jit_put_word(ctx, word) ||
                    !jit_put_word(ctx, 0xed841a00 | d))
                        return false;
        }
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
        int lane;

        CONSUME_IMM8(vd);
        CONSUME_IMM8(lhs);
        CONSUME_IMM8(rhs);

        if ((ctx->simd_caps & JIT_SIMD_CAP_NEON) != 0) {
                return jit_put_word(ctx, neon_q3(0xf3000d50,
                                               vd, lhs, rhs));
        }

        ASM {
                MOVW(REG_R4, (uint32_t)offsetof(struct rt_env, vreg) & 0xffff);
                MOVT(REG_R4, ((uint32_t)offsetof(struct rt_env, vreg) >> 16) & 0xffff);
                ADD(REG_R4, REG_R11, REG_R4);
        }

        for (lane = 0; lane < 4; lane++) {
                uint32_t a;
                uint32_t b;
                uint32_t d;
                uint32_t word;

                a = (uint32_t)(lhs * 16 + lane * 4) / 4;
                b = (uint32_t)(rhs * 16 + lane * 4) / 4;
                d = (uint32_t)(vd * 16 + lane * 4) / 4;
                /* vldr s0,[r4,#a]; vldr s1,[r4,#b] */
                if (!jit_put_word(ctx, 0xed940a00 | a) ||
                    !jit_put_word(ctx, 0xedd40a00 | b))
                        return false;
                word = 0xee201a20;
                if (!jit_put_word(ctx, word) ||
                    !jit_put_word(ctx, 0xed841a00 | d))
                        return false;
        }
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
        int lane;

        CONSUME_IMM8(vd);
        CONSUME_IMM8(lhs);
        CONSUME_IMM8(rhs);

        if ((ctx->simd_caps & JIT_SIMD_CAP_NEON) != 0) {
                int source[2] = { lhs, rhs };
                int k;
                int lane;
                uint32_t dd;

                /*
                 * ARMv7 NEON has no vector divide.  Spill
                 * just the two operands to their canonical
                 * homes, perform the four VFP divides, then
                 * reload the native destination register.
                 */
                ASM {
                        MOVW(REG_R4, (uint32_t)offsetof(struct rt_env, vreg) & 0xffff);
                        MOVT(REG_R4, ((uint32_t)offsetof(struct rt_env, vreg) >> 16) & 0xffff);
                        ADD(REG_R4, REG_R11, REG_R4);
                }

                for (k = 0; k < 2; k++) {
                        dd = (uint32_t)(16 + source[k] * 2);
                        ASM {
                                MOVW(REG_R2, (uint32_t)(source[k] * 16));
                                ADD(REG_R2, REG_R4, REG_R2);
                        }
                        if (!jit_put_word(ctx, 0xf4000a0f |
                                          ((dd & 16u) << 18) |
                                          ((dd & 15u) << 12) |
                                          (2u << 16)))
                                return false;
                }


                ASM {
                        MOVW(REG_R4, (uint32_t)offsetof(struct rt_env, vreg) & 0xffff);
                        MOVT(REG_R4, ((uint32_t)offsetof(struct rt_env, vreg) >> 16) & 0xffff);
                        ADD(REG_R4, REG_R11, REG_R4);
                }

                for (lane = 0; lane < 4; lane++) {
                        uint32_t a;
                        uint32_t b;
                        uint32_t d;
                        uint32_t word;

                        a = (uint32_t)(lhs * 16 + lane * 4) / 4;
                        b = (uint32_t)(rhs * 16 + lane * 4) / 4;
                        d = (uint32_t)(vd * 16 + lane * 4) / 4;
                        /* vldr s0,[r4,#a]; vldr s1,[r4,#b] */
                        if (!jit_put_word(ctx, 0xed940a00 | a) ||
                            !jit_put_word(ctx, 0xedd40a00 | b))
                                return false;
                        word = 0xee801a20;
                        if (!jit_put_word(ctx, word) ||
                            !jit_put_word(ctx, 0xed841a00 | d))
                                return false;
                }

                dd = (uint32_t)(16 + vd * 2);

                ASM {
                        MOVW(REG_R2, (uint32_t)(vd * 16));
                        ADD(REG_R2, REG_R4, REG_R2);
                }

                return jit_put_word(ctx, 0xf4200a0f |
                                    ((dd & 16u) << 18) |
                                    ((dd & 15u) << 12) |
                                    (2u << 16));
        }

        ASM {
                MOVW(REG_R4, (uint32_t)offsetof(struct rt_env, vreg) & 0xffff);
                MOVT(REG_R4, ((uint32_t)offsetof(struct rt_env, vreg) >> 16) & 0xffff);
                ADD(REG_R4, REG_R11, REG_R4);
        }

        for (lane = 0; lane < 4; lane++) {
                uint32_t a;
                uint32_t b;
                uint32_t d;
                uint32_t word;

                a = (uint32_t)(lhs * 16 + lane * 4) / 4;
                b = (uint32_t)(rhs * 16 + lane * 4) / 4;
                d = (uint32_t)(vd * 16 + lane * 4) / 4;
                /* vldr s0,[r4,#a]; vldr s1,[r4,#b] */
                if (!jit_put_word(ctx, 0xed940a00 | a) ||
                    !jit_put_word(ctx, 0xedd40a00 | b))
                        return false;
                word = 0xee801a20;
                if (!jit_put_word(ctx, word) ||
                    !jit_put_word(ctx, 0xed841a00 | d))
                        return false;
        }
        return true;
}

/* Visit an OP_VCVTI32F32X4 instruction. */
static INLINE bool
jit_visit_vcvti32f32x4_op(
        struct rt_jit_context *ctx)
{
        int vd;
        int vs;
        int lane;

        CONSUME_IMM8(vd);
        CONSUME_IMM8(vs);

        if ((ctx->simd_caps & JIT_SIMD_CAP_NEON) != 0) {
                return jit_put_word(ctx,
                        neon_q2_imm(0xf3bb0640, vd, vs, 0));
        }

        ASM {
                MOVW(REG_R4, (uint32_t)offsetof(struct rt_env, vreg) & 0xffff);
                MOVT(REG_R4, ((uint32_t)offsetof(struct rt_env, vreg) >> 16) & 0xffff);
                ADD(REG_R4, REG_R11, REG_R4);
        }

        for (lane = 0; lane < 4; lane++) {
                LDR(REG_R2, REG_R4, vs * 16 + lane * 4);
                if (!jit_put_word(ctx, 0xee002a10) ||
                    !jit_put_word(ctx, 0xeeb80ac0) ||
                    !jit_put_word(ctx, 0xee102a10))
                        return false;
                STR(REG_R2, REG_R4, vd * 16 + lane * 4);
        }
        return true;
}

/* Visit an OP_VCVTF32I32X4 instruction. */
static INLINE bool
jit_visit_vcvtf32i32x4_op(
        struct rt_jit_context *ctx)
{
        int vd;
        int vs;
        int lane;

        CONSUME_IMM8(vd);
        CONSUME_IMM8(vs);

        if ((ctx->simd_caps & JIT_SIMD_CAP_NEON) != 0) {
                return jit_put_word(ctx,
                        neon_q2_imm(0xf3bb0740, vd, vs, 0));
        }

        ASM {
                MOVW(REG_R4, (uint32_t)offsetof(struct rt_env, vreg) & 0xffff);
                MOVT(REG_R4, ((uint32_t)offsetof(struct rt_env, vreg) >> 16) & 0xffff);
                ADD(REG_R4, REG_R11, REG_R4);
        }

        for (lane = 0; lane < 4; lane++) {
                LDR(REG_R2, REG_R4, vs * 16 + lane * 4);
                if (!jit_put_word(ctx, 0xee002a10) ||
                    !jit_put_word(ctx, 0xeebd0ac0) ||
                    !jit_put_word(ctx, 0xee102a10))
                        return false;
                STR(REG_R2, REG_R4, vd * 16 + lane * 4);
        }
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
        int lane;

        CONSUME_IMM8(vd);
        CONSUME_IMM8(lhs);
        CONSUME_IMM8(rhs);

        if ((ctx->simd_caps & JIT_SIMD_CAP_NEON) != 0) {
                return jit_put_word(ctx, neon_q3(0xf2200650,
                                                       vd, lhs, rhs));
        }

        ASM {
                MOVW(REG_R4, (uint32_t)offsetof(struct rt_env, vreg) & 0xffff);
                MOVT(REG_R4, ((uint32_t)offsetof(struct rt_env, vreg) >> 16) & 0xffff);
                ADD(REG_R4, REG_R11, REG_R4);
        }

        for (lane = 0; lane < 4; lane++) {
                LDR(REG_R0, REG_R4, lhs * 16 + lane * 4);
                LDR(REG_R1, REG_R4, rhs * 16 + lane * 4);
                /* cmp r0,r1; mov{gt|lt} r2,r1 after r2=r0. */
                if (!jit_put_word(ctx, 0xe1a02000) ||
                    !jit_put_word(ctx, 0xe1500001) ||
                    !jit_put_word(ctx, 0xc1a02001))
                        return false;
                STR(REG_R2, REG_R4, vd * 16 + lane * 4);
        }
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
        int lane;

        CONSUME_IMM8(vd);
        CONSUME_IMM8(lhs);
        CONSUME_IMM8(rhs);

        if ((ctx->simd_caps & JIT_SIMD_CAP_NEON) != 0) {
                return jit_put_word(ctx, neon_q3(0xf2200640,
                                                       vd, lhs, rhs));
        }

        ASM {
                MOVW(REG_R4, (uint32_t)offsetof(struct rt_env, vreg) & 0xffff);
                MOVT(REG_R4, ((uint32_t)offsetof(struct rt_env, vreg) >> 16) & 0xffff);
                ADD(REG_R4, REG_R11, REG_R4);
        }

        for (lane = 0; lane < 4; lane++) {
                LDR(REG_R0, REG_R4, lhs * 16 + lane * 4);
                LDR(REG_R1, REG_R4, rhs * 16 + lane * 4);
                /* cmp r0,r1; mov{gt|lt} r2,r1 after r2=r0. */
                if (!jit_put_word(ctx, 0xe1a02000) ||
                    !jit_put_word(ctx, 0xe1500001) ||
                    !jit_put_word(ctx, 0xb1a02001))
                        return false;
                STR(REG_R2, REG_R4, vd * 16 + lane * 4);
        }
        return true;
}
#endif


/* Visit a bytecode of a function. */
static bool
jit_visit_bytecode(
        struct rt_jit_context *ctx)
{
        uint8_t opcode;

        /* Put a prologue. */
        ASM {
                /* r0: env */

                /* Push the general-purpose registers. */
                PUSH            (REG_SP);
                PUSH            (REG_LR);
                PUSH            (REG_R12);
                PUSH            (REG_R11);
                PUSH            (REG_R10);
                PUSH            (REG_R9);
                PUSH            (REG_R8);
                PUSH            (REG_R7);
                PUSH            (REG_R6);
                PUSH            (REG_R5);
                PUSH            (REG_R4);
                PUSH            (REG_R3);
                PUSH            (REG_R2);
                PUSH            (REG_R1);
                PUSH            (REG_R0);
                PUSH            (REG_R0); /* dummy */

                /* r11 = r0 = rt */
                MOV             (REG_R11, REG_R0);

                /* r12 = *env->frame = &env->frame->tmpvar[0] */
                LDR             (REG_R12, REG_R11, 0);
                LDR             (REG_R12, REG_R12, 0);

                /* r11 = env */
                /* r12 = &env->frame->tmpvar[0] */

                /* Skip an exception handler. */
                BAL             (76);
        }

        /* Put an exception handler. */
        ctx->exception_code = ctx->code;
        ASM {
        /* EXCEPTION: */
                POP             (REG_R0); /* dummy */
                POP             (REG_R0);
                POP             (REG_R1);
                POP             (REG_R2);
                POP             (REG_R3);
                POP             (REG_R4);
                POP             (REG_R5);
                POP             (REG_R6);
                POP             (REG_R7);
                POP             (REG_R8);
                POP             (REG_R9);
                POP             (REG_R10);
                POP             (REG_R11);
                POP             (REG_R12);
                POP             (REG_LR);
                POP             (REG_SP);
                MOVW            (REG_R0, 0);
                RET             ();
        }

        /* Put a body. */
        while (ctx->lpc < ctx->func->bytecode_size) {
                /* Save LPC and addr. */
                if (ctx->pc_entry_count >= PC_ENTRY_MAX) {
                        rt_error(ctx->env, N_TR("Too big code."));
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
#endif
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
#if defined(NOCT_USE_OPTIMIZER)
                case OP_VORI32X4I:
                        if (!jit_visit_vori32x4i_op(ctx))
                                return false;
                        break;
                case OP_VFMAF32X4:
                        if (!jit_visit_vfmaf32x4_op(ctx))
                                return false;
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
                POP             (REG_R0); /* dummy */
                POP             (REG_R0);
                POP             (REG_R1);
                POP             (REG_R2);
                POP             (REG_R3);
                POP             (REG_R4);
                POP             (REG_R5);
                POP             (REG_R6);
                POP             (REG_R7);
                POP             (REG_R8);
                POP             (REG_R9);
                POP             (REG_R10);
                POP             (REG_R11);
                POP             (REG_R12);
                POP             (REG_LR);
                POP             (REG_SP);
                MOVW            (REG_R0, 1);
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
        offset = (intptr_t)target_code - (intptr_t)ctx->branch_patch[patch_index].code;
        /* ARM B/cond uses signed imm24 words relative to PC+8. */
        if (offset < -33554424 || offset > 33554436) {
                rt_error(ctx->env, N_TR("Branch target too far."));
                return false;
        }

        /* Set the assembler cursor. */
        ctx->code = ctx->branch_patch[patch_index].code;

        /* Assemble. */
        if (ctx->branch_patch[patch_index].type == PATCH_BAL) {
                ASM {
                        BAL     ((uint32_t)offset);
                }
        } else if (ctx->branch_patch[patch_index].type == PATCH_BEQ) {
                ASM {
                        BEQ     ((uint32_t)offset);
                }
        } else if (ctx->branch_patch[patch_index].type == PATCH_BNE) {
                ASM {
                        BNE     ((uint32_t)offset);
                }
        }

        return true;
}

static uint32_t
jit_detect_simd_caps(void)
{
#if defined(__linux__) && defined(HWCAP_NEON)
	if ((getauxval(AT_HWCAP) & HWCAP_NEON) != 0)
		return JIT_SIMD_CAP_NEON;
#endif
	return 0;
}

#endif /* defined(NOCT_ARCH_ARM32) && defined(NOCT_USE_JIT) */
