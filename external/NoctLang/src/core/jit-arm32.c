/* -*- coding: utf-8; tab-width: 8; indent-tabs-mode: nil; -*- */

/*
 * Copyright (c) 2025, Awe Morris. All rights reserved.
 */

/*
 * JIT (arm32): Just-In-Time native code generation
 */

#include <noct/c89compat.h>     /* NOCT_ARCH_ARM64 */

#if defined(NOCT_ARCH_ARM32) && defined(NOCT_USE_JIT)

#include "runtime.h"
#include "jit.h"
#include "execution.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

/* False asseretion */
#define JIT_OP_NOT_IMPLEMENTED  0
#define NEVER_COME_HERE         0

/* PC entry size. */
#define PC_ENTRY_MAX            2048

/* Branch pathch size. */
#define BRANCH_PATCH_MAX        2048

/* Branch patch type */
#define PATCH_BAL               0
#define PATCH_BEQ               1
#define PATCH_BNE               2

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
                        rt_error(env, "Memory mapping failed.");
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

/* mov reg, reg */
#define MOV(rd, rs)             if (!jit_put_mov(ctx, rd, rs)) return false
static INLINE bool
jit_put_mov(
        struct jit_context *ctx,
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
        struct jit_context *ctx,
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
        struct jit_context *ctx,
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
        struct jit_context *ctx,
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
        struct jit_context *ctx,
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
        struct jit_context *ctx,
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
        struct jit_context *ctx,
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
        struct jit_context *ctx,
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
        struct jit_context *ctx,
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
        struct jit_context *ctx,
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
        struct jit_context *ctx)
{
        if (!jit_put_word(ctx, 0xe1500001))
                return false;
        return true;
}

/* bal #imm */
#define BAL(imm)        if (!jit_put_bal(ctx, imm)) return false
static INLINE bool
jit_put_bal(
        struct jit_context *ctx,
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
        struct jit_context *ctx,
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
        struct jit_context *ctx,
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
        struct jit_context *ctx,
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
        struct jit_context *ctx)
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
        struct jit_context *ctx,
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
        struct jit_context *ctx,
        int r)
{
        if (!jit_put_word(ctx,
                          0xe49d0004 |          /* ldr rN, [sp], #4 */
                          ((uint32_t)r << 12)))
                return false;
        return true;
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
        struct jit_context *ctx)
{
        uint32_t line;

        CONSUME_IMM32(line);

        ASM {
                /* r11 = env */
                /* r12 = &env->frame->tmpvar[0] */

                /* rt->line = line; */
                MOVW            (REG_R0, line);
                STR             (REG_R0, REG_R11, 4);
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
                STR     (REG_R1, REG_R0, 4);
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
                STR     (REG_R1, REG_R0, 4);
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

                /* Call rt_make_string_with_hash(). */
                MOVW            (REG_R4, ((uint32_t)rt_make_string_with_hash) & 0xffff);
                MOVT            (REG_R4, (((uint32_t)rt_make_string_with_hash) >> 16) & 0xffff);
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
        struct jit_context *ctx)
{
        int dst;

        CONSUME_TMPVAR(dst);

        dst *= (int)sizeof(struct rt_value);

        /* rt_make_empty_array(env, &env->frame->tmpvar[dst]); */
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

                /* Call rt_make_empty_array(). */
                MOVW            (REG_R3, ((uint32_t)rt_make_empty_array) & 0xffff);
                MOVT            (REG_R3, (((uint32_t)rt_make_empty_array) >> 16) & 0xffff);
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
        struct jit_context *ctx)
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

                /* Call rt_make_empty_array(). */
                MOVW            (REG_R3, ((uint32_t)rt_make_empty_dict) & 0xffff);
                MOVT            (REG_R3, (((uint32_t)rt_make_empty_dict) >> 16) & 0xffff);
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
        struct jit_context *ctx)
{
        int dst;

        CONSUME_TMPVAR(dst);

        dst *= (int)sizeof(struct rt_value);

        /* Increment an integer. */
        ASM {
                /* r11 = env */
                /* r12 = &env->frame->tmpvar[0] */

                /* Get &env->frame->tmpvar[dst] at r0. */
                MOVW    (REG_R0, (uint32_t)dst);        /* dst */
                ADD     (REG_R0, REG_R0, REG_R12);      /* r0 = &env->frame->tmpvar[dst] = &env->frame->tmpvar[dst].type */

                /* env->frame->tmpvar[dst].val.i++ */
                LDR     (REG_R1, REG_R0, 4);            /* tmp = &env->frame->tmpvar[dst].val.i */
                ADD_IMM (REG_R1, REG_R1, 1);            /* tmp++ */
                STR     (REG_R1, REG_R0, 4);            /* env->frame->tmpvar[dst].val.i = tmp */
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
                /* r11 = env */
                /* r12 = &env->frame->tmpvar[0] */

                /* r0 = &env->frame->tmpvar[src1].val.i */
                MOVW            (REG_R0, (uint32_t)src1);       /* src1 */
                ADD             (REG_R0, REG_R0, REG_R12);
                LDR             (REG_R0, REG_R0, 4);

                /* r1 = &env->frame->tmpvar[src2].val.i */
                MOVW            (REG_R1, (uint32_t)src2);       /* src1 */
                ADD             (REG_R1, REG_R1, REG_R12);
                LDR             (REG_R1, REG_R1, 4);

                /* src1 == src2 */
                CMP_R0_R1       ();
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
        src = (uint32_t)src_s;

        /* if (!rt_loadsymbol_helper(env, dst, src, len, hash)) return false; */
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

                /* Call rt_loadsymbol_helper(). */
                MOVW            (REG_R4, (uint32_t)rt_loadsymbol_helper & 0xffff);
                MOVT            (REG_R4, ((uint32_t)rt_loadsymbol_helper >> 16) & 0xffff);
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
        struct jit_context *ctx)
{
        const char *dst_s;
        uint32_t len, hash, dst;
        int src;

        CONSUME_STRING(dst_s, len, hash);
        CONSUME_TMPVAR(src);
        dst = (uint32_t)dst_s;

        /* if (!rt_storesymbol_helper(env, dst, len, hash, src)) return false; */
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

                /* Call rt_storesymbol_helper(). */
                MOVW            (REG_R4, (uint32_t)rt_storesymbol_helper & 0xffff);
                MOVT            (REG_R4, ((uint32_t)rt_storesymbol_helper >> 16) & 0xffff);
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
        struct jit_context *ctx)
{
        int dst;
        int dict;
        const char *field_s;
        uint32_t len, hash, field;

        CONSUME_TMPVAR(dst);
        CONSUME_TMPVAR(dict);
        CONSUME_STRING(field_s, len, hash);
        field = (uint32_t)field_s;

        /* if (!rt_loaddot_helper(env, dst, dict, field, len, hash)) return false; */
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

                /* Call rt_loaddot_helper(). */
                MOVW            (REG_R4, (uint32_t)rt_loaddot_helper & 0xffff);
                MOVT            (REG_R4, ((uint32_t)rt_loaddot_helper >> 16) & 0xffff);
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
        struct jit_context *ctx)
{
        int dict;
        const char *field_s;
        uint32_t len, hash, field;
        int src;

        CONSUME_TMPVAR(dict);
        CONSUME_STRING(field_s, len, hash);
        CONSUME_TMPVAR(src);
        field = (uint32_t)field_s;

        /* if (!rt_storedot_helper(env, dict, field, len, hash, src)) return false; */
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

                /* Call rt_storedot_helper(). */
                MOVW            (REG_R4, (uint32_t)rt_storedot_helper & 0xffff);
                MOVT            (REG_R4, ((uint32_t)rt_storedot_helper >> 16) & 0xffff);
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

        /* if (!rt_call_helper(env, dst, func, arg_count, arg)) return false; */
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

                /* Call rt_call_helper(). */
                MOVW            (REG_R4, (uint32_t)rt_call_helper & 0xffff);
                MOVT            (REG_R4, ((uint32_t)rt_call_helper >> 16) & 0xffff);
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
        struct jit_context *ctx)
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
        CONSUME_STRING(symbol, len, hash);
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

        /* if (!rt_thiscall_helper(env, dst, obj, symbol, len, hash, arg_count, arg)) return false; */
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

                /* Call rt_thiscall_helper(). */
                MOVW            (REG_R4, (uint32_t)rt_thiscall_helper & 0xffff);
                MOVT            (REG_R4, ((uint32_t)rt_thiscall_helper >> 16) & 0xffff);
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
        struct jit_context *ctx)
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
        struct jit_context *ctx)
{
        int src;
        uint32_t target_lpc;

        CONSUME_TMPVAR(src);
        CONSUME_IMM32(target_lpc);
        if (target_lpc >= (uint32_t)ctx->func->bytecode_size + 1) {
                rt_error(ctx->env, BROKEN_BYTECODE);
                return false;
        }

        ASM {
                /* r0 = &env->frame->tmpvar[src].val.i */
                MOVW    (REG_R0, (uint32_t)src);
                LSL_3   (REG_R0, REG_R0);               /* src * sizeof(struct rt_value) */
                ADD     (REG_R0, REG_R0, REG_R12);
                LDR     (REG_R1, REG_R0, 4);

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
        struct jit_context *ctx)
{
        int src;
        uint32_t target_lpc;

        CONSUME_TMPVAR(src);
        CONSUME_IMM32(target_lpc);
        if (target_lpc >= (uint32_t)ctx->func->bytecode_size + 1) {
                rt_error(ctx->env, BROKEN_BYTECODE);
                return false;
        }

        ASM {
                /* r11 = env */
                /* r12 = &env->frame->tmpvar[0] */

                /* r0 = &env->frame->tmpvar[src].val.i */
                MOVW    (REG_R0, (uint32_t)src);
                LSL_3   (REG_R0, REG_R0);               /* src * sizeof(struct rt_value) */
                ADD     (REG_R0, REG_R0, REG_R12);
                LDR     (REG_R1, REG_R0, 4);

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
        struct jit_context *ctx)
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

/* Visit a bytecode of a function. */
bool
jit_visit_bytecode(
        struct jit_context *ctx)
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
                        rt_error(ctx->env, "Too big code.");
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
        offset = (intptr_t)target_code - (intptr_t)ctx->branch_patch[patch_index].code;
        if (abs(offset) & ~0xffffff) {
                rt_error(ctx->env, "Branch target too far.");
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

#endif /* defined(NOCT_ARCH_ARM32) && defined(NOCT_USE_JIT) */
