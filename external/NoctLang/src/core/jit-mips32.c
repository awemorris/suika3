/* -*- coding: utf-8; tab-width: 8; indent-tabs-mode: nil; -*- */

/*
 * Copyright (c) 2025, Awe Morris. All rights reserved.
 */

/*
 * JIT (mips32): Just-In-Time native code generation
 */

#include <noct/c89compat.h>     /* NOCT_ARCH_MIPS32 */

#if defined(NOCT_ARCH_MIPS32) && defined(NOCT_USE_JIT)

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
#define REG_ZERO        0
#define REG_AT          1
#define REG_V0          2
#define REG_V1          3
#define REG_A0          4
#define REG_A1          5
#define REG_A2          6
#define REG_A3          7
#define REG_T0          8
#define REG_T1          9
#define REG_T2          10
#define REG_T3          11
#define REG_T4          12
#define REG_T5          13
#define REG_T6          14
#define REG_T7          15
#define REG_S0          16
#define REG_S1          17
#define REG_S2          18
#define REG_S3          19
#define REG_S4          20
#define REG_S5          21
#define REG_S6          22
#define REG_S7          23
#define REG_T8          24
#define REG_T9          25
#define REG_K0          26
#define REG_K1          27
#define REG_GP          28
#define REG_SP          29
#define REG_FP          30
#define REG_RA          31

/* Put a instruction word. */
#define IW(w)                           if (!jit_put_word(ctx, w)) return false
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

/*
 * Templates
 */

static INLINE uint32_t hi16(uint32_t d)
{
        return (d >> 16) & 0xffff;
}

static INLINE uint32_t lo16(uint32_t d)
{
        return d & 0xffff;
}

static INLINE uint32_t tvar16(int d)
{
        return (uint32_t)d & 0xffff;
}

#define EXC()   exc((uint32_t)ctx->exception_code, (uint32_t)ctx->code)
static INLINE uint32_t exc(uint32_t handler, uint32_t cur)
{
        return ((handler - cur - 4) / 4) & 0xffff;
}

#define ASM_BINARY_OP(f)                                                                        \
        ASM {                                                                                   \
                /* $s0: env */                                                                  \
                /* $s1: &env->frame->tmpvar[0] */                                               \
                                                                                                \
                /* Arg1 $a0 = env */                                                            \
                /* move $a0, $s0 */             IW(0x02002025);                                 \
                                                                                                \
                /* Arg2 $a1 = dst */                                                            \
                /* li $a1, dst */               IW(0x24050000 | lo16((uint32_t)dst));           \
                                                                                                \
                /* Arg3 $a2 = src1 */                                                           \
                /* li $a2, src1 */              IW(0x24060000 | tvar16(src1));                  \
                                                                                                \
                /* Arg4 $a3: src2 */                                                            \
                /* li $a3, src2 */              IW(0x24070000 | tvar16(src2));                  \
                                                                                                \
                /* Call f(). */                                                                 \
                /* lui  $t0, f@h */             IW(0x3c080000 | hi16((uint32_t)f));             \
                /* ori  $t0, $t0, f@l */        IW(0x35080000 | lo16((uint32_t)f));             \
                /* move $s2, $ra */             IW(0x03e09025);                                 \
                /* jalr $t0 */                  IW(0x0100f809);                                 \
                /* nop */                       IW(0x00000000);                                 \
                /* move $ra, $s2 */             IW(0x0240f825);                                 \
                                                                                                \
                /* If failed: */                                                                \
                /* beqz $v0, $zero, exc */      IW(0x10400000 | EXC());                         \
                /* nop */                       IW(0x00000000);                                 \
        }

#define ASM_UNARY_OP(f)                                                                         \
        ASM {                                                                                   \
                /* $s0: env */                                                                  \
                /* $s1: &env->frame->tmpvar[0] */                                               \
                                                                                                \
                /* Arg1 $a0 = env */                                                            \
                /* move $a0, $s0 */             IW(0x02002025);                                 \
                                                                                                \
                /* Arg2 $a1 = dst */                                                            \
                /* li $a1, dst */               IW(0x24050000 | lo16((uint32_t)dst));           \
                                                                                                \
                /* Arg3 $a2 = src */                                                            \
                /* li $a2, src */               IW(0x24060000 | tvar16(src));                   \
                                                                                                \
                /* Call f(). */                                                                 \
                /* lui  $t0, f@h */             IW(0x3c080000 | hi16((uint32_t)f));             \
                /* ori  $t0, $t0, f@l */        IW(0x35080000 | lo16((uint32_t)f));             \
                /* move $s2, $ra */             IW(0x03e09025);                                 \
                /* jalr $t0 */                  IW(0x0100f809);                                 \
                /* nop */                       IW(0x00000000);                                 \
                /* move $ra, $s2 */             IW(0x0240f825);                                 \
                                                                                                \
                /* If failed: */                                                                \
                /* beqz $v0, $zero, exc */      IW(0x10400000 | EXC());                         \
                /* nop */                       IW(0x00000000);                                 \
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
                /* $s0: env */
                /* $s1: &env->frame->tmpvar[0] */

                /* env->line = line; */
                /* li $t0, line */      IW(0x24080000 | lo16(line));
                /* sw $t0, 4($s0) */    IW(0xae080004);
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
                /* $s0: env */
                /* $s1: &env->frame->tmpvar[0] */

                /* $t0 = dst_addr = &env->frame->tmpvar[dst] */
                /* li $t0, dst */               IW(0x24080000 | lo16((uint32_t)dst));
                /* addu $t0, $t0, $s1 */        IW(0x01114021);

                /* $t1 = src_addr = &env->frame->tmpvar[src] */
                /* li $t1, src */               IW(0x24090000 | lo16((uint32_t)src));
                /* addu $t1, $t1, $s1 */        IW(0x01314821);

                /* *dst_addr = *src_addr */
                /* lw $t2, 0($t1) */            IW(0x8d2a0000);
                /* lw $t3, 4($t1) */            IW(0x8d2b0004);
                /* sw $t2, 0($t0) */            IW(0xad0a0000);
                /* sw $t3, 4($t0) */            IW(0xad0b0004);
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
                /* $s0: env */
                /* $s1: &env->frame->tmpvar[0] */

                /* $t0 = dst_addr = &env->frame->tmpvar[dst] */
                /* li $t0, dst */               IW(0x24080000 | lo16((uint32_t)dst));
                /* addu $t0, $t0, $s1 */        IW(0x01114021);

                /* env->frame->tmpvar[dst].type = RT_VALUE_INT */
                /* li $t1, 0 */                 IW(0x24090000);
                /* sw $t1, 0($t0) */            IW(0xad090000);

                /* env->frame->tmpvar[dst].val.i = val */
                /* lui $t1, val@h */            IW(0x3c090000 | hi16(val));
                /* ori $t1, $t1, val@l */       IW(0x35290000 | lo16(val));
                /* sw  $t1, 4($t0) */           IW(0xad090004);
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
                /* $s0: env */
                /* $s1: &env->frame->tmpvar[0] */

                /* $t0 = dst_addr = &env->frame->tmpvar[dst] */
                /* li $t0, dst */               IW(0x24080000 | lo16((uint32_t)dst));
                /* addu $t0, $t0, $s1 */        IW(0x01114021);

                /* env->frame->tmpvar[dst].type = RT_VALUE_FLOAT */
                /* li $t1, 1 */                 IW(0x24090001);
                /* sw $t1, 0($t0) */            IW(0xad090000);

                /* env->frame->tmpvar[dst].val.i = val */
                /* lui $t1, val@h */            IW(0x3c090000 | hi16(val));
                /* ori $t1, $t1, val@l */       IW(0x35290000 | lo16(val));
                /* sw  $t1, 4($t0) */           IW(0xad090004);
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
        uint32_t f;

        CONSUME_TMPVAR(dst);
        CONSUME_STRING(val, len, hash);

        f = (uint32_t)rt_make_string_with_hash;
        dst *= (int)sizeof(struct rt_value);

        /* rt_make_string_with_hash(env, &env->frame->tmpvar[dst], val, len, hash); */
        ASM {
                /* $s0: env */
                /* $s1: &env->frame->tmpvar[0] */

                /* addiu $sp, $sp, -32 */       IW(0x27bdffe0);

                /* Arg1 $a0 = [sp+0] = rt */
                /* move $a0, $s0 */             IW(0x02002025);
                /* sw $a0, 0($sp) */            IW(0xafa40000);

                /* Arg2 $a1 = [sp+4] = dst_addr = &env->frame->tmpvar[dst] */
                /* li $a1, dst */               IW(0x24050000 | lo16((uint32_t)dst));
                /* addu $a1, $a1, $s1 */        IW(0x00b12821);
                /* sw $a1, 4($sp) */            IW(0xafa50004);

                /* Arg3 $a2 = [sp+8] = val */
                /* lui $a2, val@h */            IW(0x3c060000 | hi16((uint32_t)val));
                /* ori $a2, $a2, val@l */       IW(0x34c60000 | lo16((uint32_t)val));
                /* sw $a2, 8($sp) */            IW(0xafa60008);

                /* Arg4 $a3 = [sp+12] = len */
                /* lui $a3, len@h */            IW(0x3c070000 | hi16(len));
                /* ori $a3, $a3, len@l */       IW(0x34e70000 | lo16(len));
                /* sw $a3, 12($sp) */           IW(0xafa7000c);

                /* Arg5 [sp+16] = hash */
                /* lui $t0, hash@h */           IW(0x3c080000 | hi16(hash));
                /* ori $t0, $t0, hash@l */      IW(0x35080000 | lo16(hash));
                /* sw $t0, 16($sp) */           IW(0xafa80010);

                /* Call rt_make_string_with_hash(). */
                /* lui  $t0, f@h */             IW(0x3c080000 | hi16(f));
                /* ori  $t0, $t0, f@l */        IW(0x35080000 | lo16(f));
                /* move $s2, $ra */             IW(0x03e09025);
                /* jalr $t0 */                  IW(0x0100f809);
                /* nop */                       IW(0x00000000);
                /* move $ra, $s2 */             IW(0x0240f825);
                /* addiu $sp, $sp, 32 */        IW(0x27bd0020);

                /* If failed: */
                /* beqz $v0, $zero, exc */      IW(0x10400000 | EXC());
                /* nop */                       IW(0x00000000);
        }

        return true;
}

/* Visit a OP_ACONST instruction. */
static INLINE bool
jit_visit_aconst_op(
        struct jit_context *ctx)
{
        int dst;
        uint32_t f;

        CONSUME_TMPVAR(dst);

        f = (uint32_t)rt_make_empty_array;
        dst *= (int)sizeof(struct rt_value);

        /* rt_make_empty_array(env, &env->frame->tmpvar[dst]); */
        ASM {
                /* $s0: env */
                /* $s1: &env->frame->tmpvar[0] */

                /* Arg1 $a0 = rt */
                /* move $a0, $s0 */             IW(0x02002025);

                /* Arg2 $a1 = dst_addr = &env->frame->tmpvar[dst] */
                /* li $a1, dst */               IW(0x24050000 | lo16((uint32_t)dst));
                /* addu $a1, $a1, $s1 */        IW(0x00b12821);

                /* Call rt_make_empty_array(). */
                /* lui  $t0, f@h */             IW(0x3c080000 | hi16(f));
                /* ori  $t0, $t0, f@l */        IW(0x35080000 | lo16(f));
                /* move $s2, $ra */             IW(0x03e09025);
                /* jalr $t0 */                  IW(0x0100f809);
                /* nop */                       IW(0x00000000);
                /* move $ra, $s2 */             IW(0x0240f825);

                /* If failed: */
                /* beqz $v0, $zero, exc */      IW(0x10400000 | EXC());
                /* nop */                       IW(0x00000000);
        }

        return true;
}

/* Visit a OP_DCONST instruction. */
static INLINE bool
jit_visit_dconst_op(
        struct jit_context *ctx)
{
        int dst;
        uint32_t f;

        CONSUME_TMPVAR(dst);

        f = (uint32_t)rt_make_empty_dict;
        dst *= (int)sizeof(struct rt_value);

        /* rt_make_empty_dict(env, &env->frame->tmpvar[dst]); */
        ASM {
                /* $s0: env */
                /* $s1: &env->frame->tmpvar[0] */

                /* Arg1 $a0 = rt */
                /* move $a0, $s0 */             IW(0x02002025);

                /* Arg2 $a1 = dst_addr = &env->frame->tmpvar[dst] */
                /* li $a1, dst */               IW(0x24050000 | lo16((uint32_t)dst));
                /* addu $a1, $a1, $s1 */        IW(0x00b12821);

                /* Call rt_make_empty_dict(). */
                /* lui  $t0, f@h */             IW(0x3c080000 | hi16(f));
                /* ori  $t0, $t0, f@l */        IW(0x35080000 | lo16(f));
                /* move $s2, $ra */             IW(0x03e09025);
                /* jalr $t0 */                  IW(0x0100f809);
                /* nop */                       IW(0x00000000);
                /* move $ra, $s2 */             IW(0x0240f825);

                /* If failed: */
                /* beqz $v0, $zero, exc */      IW(0x10400000 | EXC());
                /* nop */                       IW(0x00000000);
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
                /* $s0: env */
                /* $s1: &env->frame->tmpvar[0] */

                /* $t0 = dst_addr = &env->frame->tmpvar[dst] */
                /* li $t0, dst */               IW(0x24080000 | lo16((uint32_t)dst));
                /* addu $t0, $t0, $s1 */        IW(0x01114021);

                /* env->frame->tmpvar[dst].val.i++ */
                /* lw    $t1, 4($t0) */         IW(0x8d090004);
                /* addiu $t1, $t1, 1 */         IW(0x25290001);
                /* sw    $t1, 4($t0) */         IW(0xad090004);
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

        dst *= (int)sizeof(struct rt_value);
        src1 *= (int)sizeof(struct rt_value);
        src2 *= (int)sizeof(struct rt_value);

        /* src1 == src2 */
        ASM {
                /* $s0: env */
                /* $s1: &env->frame->tmpvar[0] */

                /* $t0 = env->frame->tmpvar[src1].val.i */
                /* li $t0, src1 */              IW(0x24080000 | lo16((uint32_t)src1));
                /* addu $t0, $t0, $s1 */        IW(0x01114021);
                /* lw $t0, 4($t0) */            IW(0x8d080004);

                /* $t1 = env->frame->tmpvar[src2].val.i */
                /* li $t1, src2 */              IW(0x24090000 | lo16((uint32_t)src2));
                /* addu $t1, $t1, $s1 */        IW(0x01314821);
                /* lw $t1, 4($t1) */            IW(0x8d290004);

                /* src1 == src2 */
                /* subu $at, $t0, $t1 */        IW(0x01090823);
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
        uint32_t len, hash;
        uint32_t src;
        uint32_t f;

        CONSUME_TMPVAR(dst);
        CONSUME_STRING(src_s, len, hash);

        src = (uint32_t)(intptr_t)src_s;
        f = (uint32_t)rt_loadsymbol_helper;

        /* if (!jit_loadsymbol_helper(env, dst, src, len, hash)) return false; */
        ASM {
                /* $s0: env */
                /* $s1: &env->frame->tmpvar[0] */

                /* addiu $sp, $sp, -16 */       IW(0x27bdffe0);

                /* Arg1 $a0 = [sp+0] = rt */
                /* move $a0, $s0 */             IW(0x02002025);
                /* sw $a0, 0($sp) */            IW(0xafa40000);

                /* Arg2 $a1 = [sp+4] = dst */
                /* li $a1, dst */               IW(0x24050000 | tvar16(dst));
                /* sw $a1, 4($sp) */            IW(0xafa50004);

                /* Arg3 $a2 = [sp+8] = src */
                /* lui $a2, src@h */            IW(0x3c060000 | hi16(src));
                /* ori $a2, src@l */            IW(0x34c60000 | lo16(src));
                /* sw $a2, 8($sp) */            IW(0xafa60008);

                /* Arg4 $a3 = [sp+12] = len */
                /* lui $a3, len@h */            IW(0x3c070000 | hi16(len));
                /* ori $a3, $a3, len@l */       IW(0x34e70000 | lo16(len));
                /* sw $a3, 12($sp) */           IW(0xafa7000c);

                /* Arg5 [sp+16] = hash */
                /* lui $t0, hash@h */           IW(0x3c080000 | hi16(hash));
                /* ori $t0, $t0, hash@l */      IW(0x35080000 | lo16(hash));
                /* sw $t0, 16($sp) */           IW(0xafa80010);

                /* Call rt_loadsymbol_helper(). */
                /* lui  $t0, f@h */             IW(0x3c080000 | hi16(f));
                /* ori  $t0, $t0, f@l */        IW(0x35080000 | lo16(f));
                /* move $s2, $ra */             IW(0x03e09025);
                /* jalr $t0 */                  IW(0x0100f809);
                /* nop */                       IW(0x00000000);
                /* move $ra, $s2 */             IW(0x0240f825);
                /* addiu $sp, $sp, 32 */        IW(0x27bd0020);

                /* If failed: */
                /* beqz $v0, $zero, exc */      IW(0x10400000 | EXC());
                /* nop */                       IW(0x00000000);
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
        uint32_t f;

        CONSUME_STRING(dst_s, len, hash);
        CONSUME_TMPVAR(src);

        dst = (uint32_t)(intptr_t)dst_s;
        f = (uint32_t)rt_storesymbol_helper;

        /* if (!rt_storesymbol_helper(env, dst, len, hash, src)) return false; */
        ASM {
                /* $s0: env */
                /* $s1: &env->frame->tmpvar[0] */

                /* addiu $sp, $sp, -32 */       IW(0x27bdffe0);

                /* Arg1 $a0 = [sp+0] = env */
                /* move $a0, $s0 */             IW(0x02002025);
                /* sw   $a0, 0($sp) */          IW(0xafa40000);

                /* Arg2 $a1 = [sp+4] = dst */
                /* lui $a1, dst@h */            IW(0x3c050000 | hi16(dst));
                /* ori $a1, dst@l */            IW(0x34a50000 | lo16(dst));
                /* sw  $a1, 4($sp) */           IW(0xafa50004);

                /* Arg3 $a2 = [sp+8] = len */
                /* lui $a2, len@h */            IW(0x3c060000 | hi16(len));
                /* ori $a2, len@l */            IW(0x34c60000 | lo16(len));
                /* sw  $a2, 8($sp) */           IW(0xafa60008);

                /* Arg4 $a3 = [sp+12] = hash */
                /* lui $a3, hash@h */           IW(0x3c070000 | hi16(hash));
                /* ori $a3, $a3, hash@l */      IW(0x34e70000 | lo16(hash));
                /* sw  $a3, 12($sp) */          IW(0xafa7000c);

                /* Arg5 [sp+16] = src */
                /* ori $t0, $zero, src */       IW(0x24080000 | tvar16(src));
                /* sw $t0, 16($sp) */           IW(0xafa80010);

                /* Call rt_storesymbol_helper(). */
                /* lui  $t0, f@h */             IW(0x3c080000 | hi16(f));
                /* ori  $t0, $t0, f@l */        IW(0x35080000 | lo16(f));
                /* move $s2, $ra */             IW(0x03e09025);
                /* jalr $t0 */                  IW(0x0100f809);
                /* nop */                       IW(0x00000000);
                /* move $ra, $s2 */             IW(0x0240f825);
                /* addiu $sp, $sp, 32 */        IW(0x27bd0020);

                /* If failed: */
                /* beqz $v0, $zero, exc */      IW(0x10400000 | EXC());
                /* nop */                       IW(0x00000000);
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
        uint32_t f;

        CONSUME_TMPVAR(dst);
        CONSUME_TMPVAR(dict);
        CONSUME_STRING(field_s, len, hash);

        field = (uint32_t)(intptr_t)field_s;
        f = (uint32_t)rt_loaddot_helper;

        /* if (!rt_loaddot_helper(env, dst, dict, field, len, hash)) return false; */
        ASM {
                /* $s0: env */
                /* $s1: &env->frame->tmpvar[0] */

                /* addiu $sp, $sp, -32 */       IW(0x27bdffe0);

                /* Arg1 $a0 = [sp+0] = rt */
                /* move $a0, $s0 */             IW(0x02002025);
                /* sw $a0, 0($sp) */            IW(0xafa40000);

                /* Arg2 $a1 = [sp+4] = dst */
                /* li $a1, dst */               IW(0x24050000 | tvar16(dst));
                /* sw $a1, 4($sp) */            IW(0xafa50004);

                /* Arg3 $a2 = [sp+8] = dict */
                /* li $a2, dict */              IW(0x24060000 | tvar16(dict));
                /* sw $a2, 8($sp) */            IW(0xafa60008);

                /* Arg4 $a3 = [sp+12] = field */
                /* lui  $a3, field@h */         IW(0x3c070000 | hi16(field));
                /* ori  $a3, $a3, field@l */    IW(0x34e70000 | lo16(field));
                /* sw $a3, 12($sp) */           IW(0xafa7000c);

                /* Arg5 [sp+16] = len */
                /* lui $t0, len@h */            IW(0x3c080000 | hi16(len));
                /* ori $t0, $t0, len@l */       IW(0x35080000 | lo16(len));
                /* sw $t0, 16($sp) */           IW(0xafa80010);

                /* Arg6 [sp+20] = hash */
                /* lui $t0, hash@h */           IW(0x3c080000 | hi16(hash));
                /* ori $t0, $t0, hash@l */      IW(0x35080000 | lo16(hash));
                /* sw $t0, 16($sp) */           IW(0xafa80014);

                /* Call rt_loaddot_helper(). */
                /* lui  $t0, f@h */             IW(0x3c080000 | hi16(f));
                /* ori  $t0, $t0, f@l */        IW(0x35080000 | lo16(f));
                /* move $s2, $ra */             IW(0x03e09025);
                /* jalr $t0 */                  IW(0x0100f809);
                /* nop */                       IW(0x00000000);
                /* move $ra, $s2 */             IW(0x0240f825);
                /* addiu $sp, $sp, 32 */        IW(0x27bd0020);

                /* If failed: */
                /* beqz $v0, $zero, exc */      IW(0x10400000 | EXC());
                /* nop */                       IW(0x00000000);
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
        uint32_t f;

        CONSUME_TMPVAR(dict);
        CONSUME_STRING(field_s, len, hash);
        CONSUME_TMPVAR(src);

        field = (uint32_t)(intptr_t)field_s;
        f = (uint32_t)rt_storedot_helper;

        /* if (!jit_storedot_helper(env, dict, field, len, hash, src)) return false; */
        ASM {
                /* $s0: env */
                /* $s1: &env->frame->tmpvar[0] */

                /* addiu $sp, $sp, -32 */       IW(0x27bdffe0);

                /* Arg1 $a0 = [sp+0] = env */
                /* move $a0, $s0 */             IW(0x02002025);
                /* sw $a0, 0($sp) */            IW(0xafa40000);

                /* Arg2 $a1 = [sp+4] = dic */
                /* li $a1, dict */              IW(0x24050000 | tvar16(dict));
                /* sw $a1, 4($sp) */            IW(0xafa50004);

                /* Arg3 $a2 = [sp+8] = field */
                /* lui $a2, field@h */          IW(0x3c060000 | hi16(field));
                /* ori $a2, $a2, field@l */     IW(0x34c60000 | lo16(field));
                /* sw $a2, 8($sp) */            IW(0xafa60008);

                /* Arg4 $a3 = [sp+12] = len */
                /* lui $a3, len@h */            IW(0x3c070000 | hi16(len));
                /* ori $a3, $a3, len@l */       IW(0x34e70000 | lo16(len));
                /* sw $a3, 12($sp) */           IW(0xafa7000c);

                /* Arg5 [sp+16] = hash */
                /* lui $t0, hash@h */           IW(0x3c080000 | hi16(hash));
                /* ori $t0, $t0, hash@l */      IW(0x35080000 | lo16(hash));
                /* sw $t0, 16($sp) */           IW(0xafa80010);

                /* Arg6 [sp+20] = src */
                /* ori $t0, $zero, src */       IW(0x24080000 | lo16(src));
                /* sw $t0, 20($sp) */           IW(0xafa80014);

                /* Call rt_storedot_helper(). */
                /* lui  $t0, f@h */             IW(0x3c080000 | hi16(f));
                /* ori  $t0, $t0, f@l */        IW(0x35080000 | lo16(f));
                /* move $s2, $ra */             IW(0x03e09025);
                /* jalr $t0 */                  IW(0x0100f809);
                /* nop */                       IW(0x00000000);
                /* move $ra, $s2 */             IW(0x0240f825);
                /* addiu $sp, $sp, 32 */        IW(0x27bd0020);

                /* If failed: */
                /* beqz $v0, $zero, exc */      IW(0x10400000 | EXC());
                /* nop */                       IW(0x00000000);
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
        uint32_t tmp;
        uint32_t arg_addr;
        int i;
        uint32_t f;

        CONSUME_TMPVAR(dst);
        CONSUME_TMPVAR(func);
        CONSUME_IMM8(arg_count);
        for (i = 0; i < arg_count; i++) {
                CONSUME_TMPVAR(arg_tmp);
                arg[i] = arg_tmp;
        }

        if (arg_count > 0) {
                /* Embed arguments to the code. */
                tmp = (uint32_t)((8 + 4 * arg_count - 4) / 4);
                ASM {
                        /* b */         IW(0x10000000 | tmp);
                        /* nop */       IW(0x00000000);
                }
                arg_addr = (uint32_t)(intptr_t)ctx->code;
                for (i = 0; i < arg_count; i++) {
                        *(uint32_t *)ctx->code = (uint32_t)arg[i];
                        ctx->code = (uint32_t *)ctx->code + 1;
                }
        } else {
                arg_addr = 0;
        }

        f = (uint32_t)rt_call_helper;

        /* if (!rt_call_helper(env, dst, func, arg_count, arg)) return false; */
        ASM {
                /* $s0: env */
                /* $s1: &env->frame->tmpvar[0] */

                /* Arg1 $a0 = rt */
                /* move $a0, $s0 */             IW(0x02002025);

                /* Arg2 $a1 = dst */
                /* li $a1, dst */               IW(0x24050000 | tvar16(dst));

                /* Arg3 $a2 = func */
                /* li $a2, func */              IW(0x24060000 | tvar16(func));

                /* Arg4 $a3 = arg_count */
                /* li $a3, arg_count */         IW(0x24070000 | lo16((uint32_t)arg_count));

                /* Arg5 arg */
                /* lui $t0, arg@h */            IW(0x3c080000 | hi16(arg_addr));
                /* ori $t0, $t0, arg@l */       IW(0x35080000 | lo16(arg_addr));
                /* addiu $sp, $sp, -24 */       IW(0x27bdffe8);
                /* sw $t0, 16($sp) */           IW(0xafa80010);

                /* Call rt_call_helper(). */
                /* lui  $t0, f@h */             IW(0x3c080000 | hi16(f));
                /* ori  $t0, $t0, f@l */        IW(0x35080000 | lo16(f));
                /* move $s2, $ra */             IW(0x03e09025);
                /* jalr $t0 */                  IW(0x0100f809);
                /* nop */                       IW(0x00000000);
                /* move $ra, $s2 */             IW(0x0240f825);
                /* addiu $sp, $sp, 24 */        IW(0x27bd0018);

                /* If failed: */
                /* beqz $v0, $zero, exc */      IW(0x10400000 | EXC());
                /* nop */                       IW(0x00000000);
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
        uint32_t tmp;
        uint32_t arg_addr;
        int i;
        uint32_t f;

        CONSUME_TMPVAR(dst);
        CONSUME_TMPVAR(obj);
        CONSUME_STRING(symbol, len, hash);
        CONSUME_IMM8(arg_count);
        for (i = 0; i < arg_count; i++) {
                CONSUME_TMPVAR(arg_tmp);
                arg[i] = arg_tmp;
        }

        if (arg_count > 0) {
                /* Embed arguments to the code. */
                tmp = (uint32_t)((8 + 4 * arg_count - 4) / 4);
                ASM {
                        /* b */         IW(0x10000000 | tmp);
                        /* nop */       IW(0x00000000);
                }
                arg_addr = (uint32_t)(intptr_t)ctx->code;
                for (i = 0; i < arg_count; i++) {
                        *(uint32_t *)ctx->code = (uint32_t)arg[i];
                        ctx->code = (uint32_t *)ctx->code + 1;
                }
        } else {
                arg_addr = 0;
        }

        f = (uint32_t)rt_thiscall_helper;

        /* if (!rt_thiscall_helper(env, dst, obj, symbol, len, hash, arg_count, arg)) return false; */
        ASM {
                /* $s0: env */
                /* $s1: &env->frame->tmpvar[0] */

                /* addiu $sp, $sp, -32 */       IW(0x27bdffe0);

                /* Arg1 $a0 = [sp+0] = rt */
                /* move $a0, $s0 */             IW(0x02002025);
                /* sw $a0, 0($sp) */            IW(0xafa40000);

                /* Arg2 $a1 = [sp+4] = dst */
                /* li $a1, dst */               IW(0x24050000 | tvar16(dst));
                /* sw $a1, 4($sp) */            IW(0xafa50004);

                /* Arg3 $a2 = [sp+8] = obj */
                /* li $a2, obj */               IW(0x24060000 | tvar16(obj));
                /* sw $a2, 8($sp) */            IW(0xafa60008);

                /* Arg4 $a3 = [sp+12] = symbol */
                /* lui $a3, symbol@h */         IW(0x3c070000 | hi16((uint32_t)symbol));
                /* ori $a3, $a3, symbol@l */    IW(0x34e70000 | lo16((uint32_t)symbol));
                /* sw $a3, 12($sp) */           IW(0xafa7000c);

                /* Arg5 [sp+16] = len */
                /* lui $t0, len@h */            IW(0x3c080000 | hi16(len));
                /* ori $t0, $t0, len@l */       IW(0x35080000 | lo16(len));
                /* sw $t0, 16($sp) */           IW(0xafa80010);

                /* Arg6 [sp+20] = hash */
                /* lui $t0, hash@h */           IW(0x3c080000 | hi16(hash));
                /* ori $t0, $t0, hash@l */      IW(0x35080000 | lo16(hash));
                /* sw $t0, 20($sp) */           IW(0xafa80014);

                /* Arg7 [sp+24] = argc_count */
                /* ori $t0, $zero, arg_count */ IW(0x24080000 | lo16(arg_count));
                /* sw $t0, 24($sp) */           IW(0xafa80018);

                /* Arg8 [sp+28] = arg */
                /* lui $t0, arg@h */            IW(0x3c080000 | hi16(arg_addr));
                /* ori $t0, $t0, arg@l */       IW(0x35080000 | lo16(arg_addr));
                /* sw $t0, 28($sp) */           IW(0xafa8001c);

                /* Call rt_thiscall_helper(). */
                /* lui  $t0, f@h */             IW(0x3c080000 | hi16(f));
                /* ori  $t0, $t0, f@l */        IW(0x35080000 | lo16(f));
                /* move $s2, $ra */             IW(0x03e09025);
                /* jalr $t0 */                  IW(0x0100f809);
                /* nop */                       IW(0x00000000);
                /* move $ra, $s2 */             IW(0x0240f825);
                /* addiu $sp, $sp, 32 */        IW(0x27bd0020);

                /* If failed: */
                /* beqz $v0, $zero, exc */      IW(0x10400000 | EXC());
                /* nop */                       IW(0x00000000);
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
        ctx->branch_patch[ctx->branch_patch_count].type = PATCH_BAL;
        ctx->branch_patch_count++;

        ASM {
                /* Patched later. */
                /* b 0 */       IW(0x10000000);
                /* nop */       IW(0x00000000);
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

        src *= (int)sizeof(struct rt_value);

        ASM {
                /* $s0: env */
                /* $s1: &env->frame->tmpvar[0] */

                /* $at = env->frame->tmpvar[src].val.i */
                /* li $t0, src */               IW(0x24080000 | tvar16(src));
                /* addu $t0, $t0, $s1 */        IW(0x01114021);
                /* lw $at, 4($t0) */            IW(0x8d010004);
        }

        /* Patch later. */
        ctx->branch_patch[ctx->branch_patch_count].code = ctx->code;
        ctx->branch_patch[ctx->branch_patch_count].lpc = target_lpc;
        ctx->branch_patch[ctx->branch_patch_count].type = PATCH_BNE;
        ctx->branch_patch_count++;

        ASM {
                /* Patched later. */
                /* bne $at, 0, taget */         IW(0x14200000);
                /* nop */                       IW(0x00000000);
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

        src *= (int)sizeof(struct rt_value);

        ASM {
                /* $s0: env */
                /* $s1: &env->frame->tmpvar[0] */

                /* $at = env->frame->tmpvar[src].val.i */
                /* li $t0, src */               IW(0x24080000 | tvar16(src));
                /* addu $t0, $t0, $s1 */        IW(0x01114021);
                /* lw $at, 4($t0) */            IW(0x8d010004);
        }
        
        /* Patch later. */
        ctx->branch_patch[ctx->branch_patch_count].code = ctx->code;
        ctx->branch_patch[ctx->branch_patch_count].lpc = target_lpc;
        ctx->branch_patch[ctx->branch_patch_count].type = PATCH_BEQ;
        ctx->branch_patch_count++;

        ASM {
                /* Patched later. */
                /* beq $at, 0, taget */         IW(0x10200000);
                /* nop */                       IW(0x00000000);
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
                /* Patched later. */
                /* beq $at, 0, taget */         IW(0x10200000);
                /* nop */                       IW(0x00000000);
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
                /* s0: rt */
                /* s1: &env->frame->tmpvar[0] */

                /* Push the general-purpose registers. */
                /* addui $sp, $sp, -32 */       IW(0x27bdffe0);
                /* sw $s0, 28($sp) */           IW(0xafb0001c);
                /* sw $s1, 24($sp) */           IW(0xafb10018);
                /* sw $s2, 20($sp) */           IW(0xafb20014);
                /* sw $s3, 16($sp) */           IW(0xafb30010);
                /* sw $s4, 12($sp) */           IW(0xafb4000c);
                /* sw $s5, 8($sp) */            IW(0xafb50008);
                /* sw $s6, 4($sp) */            IW(0xafb60004);
                /* sw $s7, 0($sp) */            IW(0xafb70000);

                /* s0 = env */
                /* move $s0, $a0 */             IW(0x00808025);

                /* s1 = *env->frame = &env->frame->tmpvar[0] */
                /* lw $s1, 0($a0) */            IW(0x8c910000);
                /* nop */                       IW(0x00000000);
                /* lw $s1, 0($s1) */            IW(0x8e310000);
                /* nop */                       IW(0x00000000);

                /* Skip an exception handler. */
                /* b body */                    IW(0x1000000d);
                /* nop */                       IW(0x00000000);
        }

        /* Put an exception handler. */
        ctx->exception_code = ctx->code;
        ASM {
        /* EXCEPTION: */
                /* lw $s7, 0($sp) */            IW(0x8fb70000);
                /* lw $s6, 4($sp) */            IW(0x8fb60004);
                /* lw $s5, 8($sp) */            IW(0x8fb50008);
                /* lw $s4, 12($sp) */           IW(0x8fb4000c);
                /* lw $s3, 16($sp) */           IW(0x8fb30010);
                /* lw $s2, 20($sp) */           IW(0x8fb20014);
                /* lw $s0, 28($sp) */           IW(0x8fb0001c);
                /* lw $s1, 24($sp) */           IW(0x8fb10018);
                /* addiu $sp, $sp, 32 */        IW(0x27bd0020);
                /* ori $v0, $zero, 0 */         IW(0x34020000);
                /* jr $ra */                    IW(0x03e00008);
                /* nop */                       IW(0x00000000);
        }

        /* Put a body. */
        while (ctx->lpc < ctx->func->bytecode_size) {
                /* Save LPC and addr. */
                if (ctx->pc_entry_count >= PC_ENTRY_MAX) {
                        rt_error(ctx->env, "Code too big.");
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
                /* lw $s7, 0($sp) */            IW(0x8fb70000);
                /* lw $s6, 4($sp) */            IW(0x8fb60004);
                /* lw $s5, 8($sp) */            IW(0x8fb50008);
                /* lw $s4, 12($sp) */           IW(0x8fb4000c);
                /* lw $s3, 16($sp) */           IW(0x8fb30010);
                /* lw $s2, 20($sp) */           IW(0x8fb20014);
                /* lw $s0, 28($sp) */           IW(0x8fb0001c);
                /* lw $s1, 24($sp) */           IW(0x8fb10018);
                /* addiu $sp, $sp, 32 */        IW(0x27bd0020);
                /* ori $v0, $zero, 1 */         IW(0x34020001);
                /* jr $ra */                    IW(0x03e00008);
                /* nop */                       IW(0x00000000);
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
        offset = (int)((intptr_t)target_code - (intptr_t)ctx->branch_patch[patch_index].code - 4) / 4;
        if (abs(offset) & ~0xffff) {
                rt_error(ctx->env, "Branch target too far.");
                return false;
        }

        /* Set the assembler cursor. */
        ctx->code = ctx->branch_patch[patch_index].code;

        /* Assemble. */
        if (ctx->branch_patch[patch_index].type == PATCH_BAL) {
                ASM {
                        /* b offset */          IW(0x10000000 | lo16((uint32_t)offset));
                }
        } else if (ctx->branch_patch[patch_index].type == PATCH_BEQ) {
                ASM {
                        /* beq offset */        IW(0x10200000 | lo16((uint32_t)offset));
                }
        } else if (ctx->branch_patch[patch_index].type == PATCH_BNE) {
                ASM {
                        /* bne offset */        IW(0x14200000 | lo16((uint32_t)offset));
                }
        }

        return true;
}

#endif /* defined(NOCT_ARCH_PPC32) && defined(NOCT_USE_JIT) */
