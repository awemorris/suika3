/* -*- coding: utf-8; tab-width: 8; indent-tabs-mode: nil; -*- */

/*
 * Noct Programming Language
 * Copyright (c) 2025, 2026, Awe Morris
 */

/*
 * JIT (mips32): Just-In-Time native code generation
 */

#include <noct/noct.h>

#if defined(NOCT_ARCH_MIPS32) && defined(NOCT_USE_JIT)

#include "runtime.h"
#include "jit.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

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

/*
 * Generate a JIT-compiled code for a function.
 */
bool
jit_build(
          struct rt_env *env,
          struct rt_func *func)
{
	return rt_jit_build_standard(env, func, 0, "mips32",
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

/* Absolute jump through $t9 (four slots including the delay slot). */
static INLINE bool
jit_put_abs_jump(
        struct rt_jit_context *ctx,
        uint32_t target)
{
        if (!jit_put_word(ctx, 0x3c190000 | hi16(target))) return false;
        if (!jit_put_word(ctx, 0x37390000 | lo16(target))) return false;
        if (!jit_put_word(ctx, 0x03200008)) return false;
        if (!jit_put_word(ctx, 0x00000000)) return false;
        return true;
}

#define EXCEPTION_IF_EQUAL(rs, rt) do {                                 \
        if (!jit_put_word(ctx, 0x14000005 |                              \
                          ((uint32_t)(rs) << 21) |                       \
                          ((uint32_t)(rt) << 16))) return false;         \
        if (!jit_put_word(ctx, 0)) return false;                         \
        if (!jit_put_abs_jump(ctx,                                      \
                (uint32_t)(uintptr_t)ctx->exception_code)) return false; \
} while (0)

#define EXCEPTION_IF_ZERO(rs) EXCEPTION_IF_EQUAL((rs), REG_ZERO)

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
                /* lui  $t9, f@h */             IW(0x3c190000 | hi16((uint32_t)f));             \
                /* ori  $t9, $t9, f@l */        IW(0x37390000 | lo16((uint32_t)f));             \
                /* move $s2, $ra */             IW(0x03e09025);                                 \
                /* jalr $t9 */                  IW(0x0320f809);                                 \
                /* nop */                       IW(0x00000000);                                 \
                /* move $ra, $s2 */             IW(0x0240f825);                                 \
                                                                                                \
                /* If failed: */                                                                \
                EXCEPTION_IF_ZERO(REG_V0);                                                       \
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
                /* lui  $t9, f@h */             IW(0x3c190000 | hi16((uint32_t)f));             \
                /* ori  $t9, $t9, f@l */        IW(0x37390000 | lo16((uint32_t)f));             \
                /* move $s2, $ra */             IW(0x03e09025);                                 \
                /* jalr $t9 */                  IW(0x0320f809);                                 \
                /* nop */                       IW(0x00000000);                                 \
                /* move $ra, $s2 */             IW(0x0240f825);                                 \
                                                                                                \
                /* If failed: */                                                                \
                EXCEPTION_IF_ZERO(REG_V0);                                                       \
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
                /* $s0: env */
                /* $s1: &env->frame->tmpvar[0] */

                /* env->line = line; */
                /* li $t0, line */      IW(0x24080000 | lo16(line));
                /* env->line is at offset 4 on 32-bit targets. */
                /* sw $t0, 4($s0) */    IW(0xae080004);
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
                /* lw $t2, 8($t1) */            IW(0x8d2a0008);
                /* lw $t3, 12($t1) */           IW(0x8d2b000c);
                /* sw $t2, 8($t0) */            IW(0xad0a0008);
                /* sw $t3, 12($t0) */           IW(0xad0b000c);
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
                /* $s0: env */
                /* $s1: &env->frame->tmpvar[0] */

                /* $t0 = dst_addr = &env->frame->tmpvar[dst] */
                /* li $t0, dst */               IW(0x24080000 | lo16((uint32_t)dst));
                /* addu $t0, $t0, $s1 */        IW(0x01114021);

                /* env->frame->tmpvar[dst].type = NOCT_VALUE_INT */
                /* li $t1, 0 */                 IW(0x24090000);
                /* sw $t1, 0($t0) */            IW(0xad090000);

                /* env->frame->tmpvar[dst].val.i = val */
                /* lui $t1, val@h */            IW(0x3c090000 | hi16(val));
                /* ori $t1, $t1, val@l */       IW(0x35290000 | lo16(val));
                /* sw  $t1, 8($t0) */           IW(0xad090008);
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

        /* Set a long integer constant. */
        ASM {
                /* $s0: env */
                /* $s1: &env->frame->tmpvar[0] */

                /* $t0 = dst_addr = &env->frame->tmpvar[dst] */
                /* li $t0, dst */               IW(0x24080000 | lo16((uint32_t)dst));
                /* addu $t0, $t0, $s1 */        IW(0x01114021);

                /* env->frame->tmpvar[dst].type = NOCT_VALUE_LONG */
                /* li $t1, 5 */                 IW(0x24090005);
                /* sw $t1, 0($t0) */            IW(0xad090000);

                /* env->frame->tmpvar[dst].val.i = val */
#if defined(NOCT_ARCH_LE)
                /* lui $t1, val@lh */           IW(0x3c090000 | hi16((uint32_t)(val & 0xffffffff)));
                /* ori $t1, $t1, val@ll */      IW(0x35290000 | lo16((uint32_t)(val & 0xffffffff)));
                /* sw  $t1, 8($t0) */           IW(0xad090008);
                /* lui $t1, val@h */            IW(0x3c090000 | hi16((uint32_t)((val >> 32) & 0xffffffff)));
                /* ori $t1, $t1, val@l */       IW(0x35290000 | lo16((uint32_t)((val >> 32) & 0xffffffff)));
                /* sw  $t1, 12($t0) */          IW(0xad09000c);
#else
                /* lui $t1, val@lh */           IW(0x3c090000 | hi16((uint32_t)((val >> 32) & 0xffffffff)));
                /* ori $t1, $t1, val@ll */      IW(0x35290000 | lo16((uint32_t)((val >> 32) & 0xffffffff)));
                /* sw  $t1, 8($t0) */           IW(0xad090008);
                /* lui $t1, val@h */            IW(0x3c090000 | hi16((uint32_t)(val & 0xffffffff)));
                /* ori $t1, $t1, val@l */       IW(0x35290000 | lo16((uint32_t)(val & 0xffffffff)));
                /* sw  $t1, 12($t0) */          IW(0xad09000c);
#endif
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
                /* $s0: env */
                /* $s1: &env->frame->tmpvar[0] */

                /* $t0 = dst_addr = &env->frame->tmpvar[dst] */
                /* li $t0, dst */               IW(0x24080000 | lo16((uint32_t)dst));
                /* addu $t0, $t0, $s1 */        IW(0x01114021);

                /* env->frame->tmpvar[dst].type = NOCT_VALUE_FLOAT */
                /* li $t1, 1 */                 IW(0x24090001);
                /* sw $t1, 0($t0) */            IW(0xad090000);

                /* env->frame->tmpvar[dst].val.i = val */
                /* lui $t1, val@h */            IW(0x3c090000 | hi16(val));
                /* ori $t1, $t1, val@l */       IW(0x35290000 | lo16(val));
                /* sw  $t1, 8($t0) */           IW(0xad090008);
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

        /* Set a long integer constant. */
        ASM {
                /* $s0: env */
                /* $s1: &env->frame->tmpvar[0] */

                /* $t0 = dst_addr = &env->frame->tmpvar[dst] */
                /* li $t0, dst */               IW(0x24080000 | lo16((uint32_t)dst));
                /* addu $t0, $t0, $s1 */        IW(0x01114021);

                /* env->frame->tmpvar[dst].type = NOCT_VALUE_DOUBLE */
                /* li $t1, 6 */                 IW(0x24090006);
                /* sw $t1, 0($t0) */            IW(0xad090000);

                /* env->frame->tmpvar[dst].val.i = val */
#if defined(NOCT_ARCH_LE)
                /* lui $t1, val@lh */           IW(0x3c090000 | hi16((uint32_t)(val & 0xffffffff)));
                /* ori $t1, $t1, val@ll */      IW(0x35290000 | lo16((uint32_t)(val & 0xffffffff)));
                /* sw  $t1, 8($t0) */           IW(0xad090008);
                /* lui $t1, val@h */            IW(0x3c090000 | hi16((uint32_t)((val >> 32) & 0xffffffff)));
                /* ori $t1, $t1, val@l */       IW(0x35290000 | lo16((uint32_t)((val >> 32) & 0xffffffff)));
                /* sw  $t1, 12($t0) */          IW(0xad09000c);
#else
                /* lui $t1, val@lh */           IW(0x3c090000 | hi16((uint32_t)((val >> 32) & 0xffffffff)));
                /* ori $t1, $t1, val@ll */      IW(0x35290000 | lo16((uint32_t)((val >> 32) & 0xffffffff)));
                /* sw  $t1, 8($t0) */           IW(0xad090008);
                /* lui $t1, val@h */            IW(0x3c090000 | hi16((uint32_t)(val & 0xffffffff)));
                /* ori $t1, $t1, val@l */       IW(0x35290000 | lo16((uint32_t)(val & 0xffffffff)));
                /* sw  $t1, 12($t0) */          IW(0xad09000c);
#endif
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
        uint32_t f;

        CONSUME_TMPVAR(dst);
        CONSUME_STRING(val, len, hash);

        f = (uint32_t)ex_make_string_with_hash;
        dst *= (int)sizeof(struct rt_value);

        /* ex_make_string_with_hash(env, &env->frame->tmpvar[dst], val, len, hash); */
        ASM {
                /* $s0: env */
                /* $s1: &env->frame->tmpvar[0] */

                /* addiu $sp, $sp, -32 */       IW(0x27bdffe0);

                /* Arg1 $a0 = [sp+0] = env */
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

                /* Call ex_make_string_with_hash(). */
                /* lui  $t9, f@h */             IW(0x3c190000 | hi16(f));
                /* ori  $t9, $t9, f@l */        IW(0x37390000 | lo16(f));
                /* move $s2, $ra */             IW(0x03e09025);
                /* jalr $t9 */                  IW(0x0320f809);
                /* nop */                       IW(0x00000000);
                /* move $ra, $s2 */             IW(0x0240f825);
                /* addiu $sp, $sp, 32 */        IW(0x27bd0020);

                /* If failed: */
                EXCEPTION_IF_ZERO(REG_V0);
        }

        return true;
}

/* Visit a OP_ACONST instruction. */
static INLINE bool
jit_visit_aconst_op(
        struct rt_jit_context *ctx)
{
        int dst;
        uint32_t f;

        CONSUME_TMPVAR(dst);

        f = (uint32_t)ex_make_empty_array;
        dst *= (int)sizeof(struct rt_value);

        /* ex_make_empty_array(env, &env->frame->tmpvar[dst]); */
        ASM {
                /* $s0: env */
                /* $s1: &env->frame->tmpvar[0] */

                /* Arg1 $a0 = rt */
                /* move $a0, $s0 */             IW(0x02002025);

                /* Arg2 $a1 = dst_addr = &env->frame->tmpvar[dst] */
                /* li $a1, dst */               IW(0x24050000 | lo16((uint32_t)dst));
                /* addu $a1, $a1, $s1 */        IW(0x00b12821);

                /* Call ex_make_empty_array(). */
                /* lui  $t9, f@h */             IW(0x3c190000 | hi16(f));
                /* ori  $t9, $t9, f@l */        IW(0x37390000 | lo16(f));
                /* move $s2, $ra */             IW(0x03e09025);
                /* jalr $t9 */                  IW(0x0320f809);
                /* nop */                       IW(0x00000000);
                /* move $ra, $s2 */             IW(0x0240f825);

                /* If failed: */
                EXCEPTION_IF_ZERO(REG_V0);
        }

        return true;
}

/* Visit a OP_DCONST instruction. */
static INLINE bool
jit_visit_dconst_op(
        struct rt_jit_context *ctx)
{
        int dst;
        uint32_t f;

        CONSUME_TMPVAR(dst);

        f = (uint32_t)ex_make_empty_dict;
        dst *= (int)sizeof(struct rt_value);

        /* ex_make_empty_dict(env, &env->frame->tmpvar[dst]); */
        ASM {
                /* $s0: env */
                /* $s1: &env->frame->tmpvar[0] */

                /* Arg1 $a0 = rt */
                /* move $a0, $s0 */             IW(0x02002025);

                /* Arg2 $a1 = dst_addr = &env->frame->tmpvar[dst] */
                /* li $a1, dst */               IW(0x24050000 | lo16((uint32_t)dst));
                /* addu $a1, $a1, $s1 */        IW(0x00b12821);

                /* Call ex_make_empty_dict(). */
                /* lui  $t9, f@h */             IW(0x3c190000 | hi16(f));
                /* ori  $t9, $t9, f@l */        IW(0x37390000 | lo16(f));
                /* move $s2, $ra */             IW(0x03e09025);
                /* jalr $t9 */                  IW(0x0320f809);
                /* nop */                       IW(0x00000000);
                /* move $ra, $s2 */             IW(0x0240f825);

                /* If failed: */
                EXCEPTION_IF_ZERO(REG_V0);
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
                /* $s0: env */
                /* $s1: &env->frame->tmpvar[0] */

                /* $t0 = dst_addr = &env->frame->tmpvar[dst] */
                /* li $t0, dst */               IW(0x24080000 | lo16((uint32_t)dst));
                /* addu $t0, $t0, $s1 */        IW(0x01114021);

                /* env->frame->tmpvar[dst].val.i++ */
                /* lw    $t1, 8($t0) */         IW(0x8d090008);
                /* addiu $t1, $t1, step */      IW(0x25290000 | lo16((uint32_t)step));
                /* sw    $t1, 8($t0) */         IW(0xad090008);
        }

        return true;
}

static INLINE bool
jit_visit_vindex_hint_op(struct rt_jit_context *ctx)
{
	int a,b,c,id,lanes,flags;
	CONSUME_TMPVAR(a); CONSUME_TMPVAR(b); CONSUME_TMPVAR(c);
	CONSUME_IMM8(id); CONSUME_IMM8(lanes); CONSUME_IMM8(flags);
	UNUSED_PARAMETER(a); UNUSED_PARAMETER(b); UNUSED_PARAMETER(c);
	UNUSED_PARAMETER(id); UNUSED_PARAMETER(lanes); UNUSED_PARAMETER(flags);
	return true;
}

static INLINE bool jit_visit_vori32x4i_op(struct rt_jit_context *ctx)
{
	int a,b,c,d; CONSUME_IMM8(a); CONSUME_IMM8(b);
	CONSUME_IMM8(c); CONSUME_IMM8(d);
	UNUSED_PARAMETER(a); UNUSED_PARAMETER(b); UNUSED_PARAMETER(c); UNUSED_PARAMETER(d);
	return false;
}

static INLINE bool jit_visit_vfmaf32x4_op(struct rt_jit_context *ctx)
{
	int a,b,c,d; CONSUME_IMM8(a); CONSUME_IMM8(b);
	CONSUME_IMM8(c); CONSUME_IMM8(d);
	UNUSED_PARAMETER(a); UNUSED_PARAMETER(b); UNUSED_PARAMETER(c); UNUSED_PARAMETER(d);
	return false;
}

static INLINE bool
jit_visit_subjnz_op(struct rt_jit_context *ctx)
{
	int value, decrement; uint32_t target_lpc;
	CONSUME_TMPVAR(value); CONSUME_IMM8(decrement); CONSUME_IMM32(target_lpc);
	if (target_lpc >= (uint32_t)(ctx->func->bytecode_size + 1)) {
		rt_error(ctx->env, BROKEN_BYTECODE); return false;
	}
	value *= (int)sizeof(struct rt_value);
	ASM {
		IW(0x24080000 | lo16((uint32_t)value));
		IW(0x01114021);
		IW(0x8d090008);
		IW(0x25290000 | lo16((uint32_t)(-(int32_t)decrement)));
		IW(0xad090008);
		IW(0x01200825);
	}
	ctx->branch_patch[ctx->branch_patch_count].code=ctx->code;
	ctx->branch_patch[ctx->branch_patch_count].lpc=target_lpc;
	ctx->branch_patch[ctx->branch_patch_count].type=PATCH_BNE;
	ctx->branch_patch_count++;
	ASM { IW(0x14200000); IW(0); IW(0); IW(0); IW(0); IW(0); }
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
                /* lw $t0, 8($t0) */            IW(0x8d080008);

                /* $t1 = env->frame->tmpvar[src2].val.i */
                /* li $t1, src2 */              IW(0x24090000 | lo16((uint32_t)src2));
                /* addu $t1, $t1, $s1 */        IW(0x01314821);
                /* lw $t1, 8($t1) */            IW(0x8d290008);

                /* src1 == src2 */
                /* subu $at, $t0, $t1 */        IW(0x01090823);
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
        uint32_t len, hash;
        uint32_t src;
        uint32_t f;

        CONSUME_TMPVAR(dst);
        CONSUME_STRING(src_s, len, hash);

        src = (uint32_t)(intptr_t)src_s;
        f = (uint32_t)ex_loadsymbol_helper;

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

                /* Call ex_loadsymbol_helper(). */
                /* lui  $t9, f@h */             IW(0x3c190000 | hi16(f));
                /* ori  $t9, $t9, f@l */        IW(0x37390000 | lo16(f));
                /* move $s2, $ra */             IW(0x03e09025);
                /* jalr $t9 */                  IW(0x0320f809);
                /* nop */                       IW(0x00000000);
                /* move $ra, $s2 */             IW(0x0240f825);
                /* addiu $sp, $sp, 32 */        IW(0x27bd0020);

                /* If failed: */
                EXCEPTION_IF_ZERO(REG_V0);
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
        uint32_t f;

        CONSUME_STRING(dst_s, len, hash);
        CONSUME_TMPVAR(src);

        dst = (uint32_t)(intptr_t)dst_s;
        f = (uint32_t)ex_storesymbol_helper;

        /* if (!ex_storesymbol_helper(env, dst, len, hash, src)) return false; */
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

                /* Call ex_storesymbol_helper(). */
                /* lui  $t9, f@h */             IW(0x3c190000 | hi16(f));
                /* ori  $t9, $t9, f@l */        IW(0x37390000 | lo16(f));
                /* move $s2, $ra */             IW(0x03e09025);
                /* jalr $t9 */                  IW(0x0320f809);
                /* nop */                       IW(0x00000000);
                /* move $ra, $s2 */             IW(0x0240f825);
                /* addiu $sp, $sp, 32 */        IW(0x27bd0020);

                /* If failed: */
                EXCEPTION_IF_ZERO(REG_V0);
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
        uint32_t f;

        CONSUME_TMPVAR(dst);
        CONSUME_TMPVAR(dict);
        CONSUME_STRING(field_s, len, hash);

        field = (uint32_t)(intptr_t)field_s;
        f = (uint32_t)ex_loaddot_helper;

        /* if (!ex_loaddot_helper(env, dst, dict, field, len, hash)) return false; */
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

                /* Call ex_loaddot_helper(). */
                /* lui  $t9, f@h */             IW(0x3c190000 | hi16(f));
                /* ori  $t9, $t9, f@l */        IW(0x37390000 | lo16(f));
                /* move $s2, $ra */             IW(0x03e09025);
                /* jalr $t9 */                  IW(0x0320f809);
                /* nop */                       IW(0x00000000);
                /* move $ra, $s2 */             IW(0x0240f825);
                /* addiu $sp, $sp, 32 */        IW(0x27bd0020);

                /* If failed: */
                EXCEPTION_IF_ZERO(REG_V0);
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
        uint32_t f;

        CONSUME_TMPVAR(dict);
        CONSUME_STRING(field_s, len, hash);
        CONSUME_TMPVAR(src);

        field = (uint32_t)(intptr_t)field_s;
        f = (uint32_t)ex_storedot_helper;

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
                /* ori $t0, $zero, src */       IW(0x24080000 | lo16((uint32_t)src));
                /* sw $t0, 20($sp) */           IW(0xafa80014);

                /* Call ex_storedot_helper(). */
                /* lui  $t9, f@h */             IW(0x3c190000 | hi16(f));
                /* ori  $t9, $t9, f@l */        IW(0x37390000 | lo16(f));
                /* move $s2, $ra */             IW(0x03e09025);
                /* jalr $t9 */                  IW(0x0320f809);
                /* nop */                       IW(0x00000000);
                /* move $ra, $s2 */             IW(0x0240f825);
                /* addiu $sp, $sp, 32 */        IW(0x27bd0020);

                /* If failed: */
                EXCEPTION_IF_ZERO(REG_V0);
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

        f = (uint32_t)ex_call_helper;

        /* if (!ex_call_helper(env, dst, func, arg_count, arg)) return false; */
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
                /* addiu $sp, $sp, -32 */       IW(0x27bdffe0);
                /* sw $t0, 16($sp) */           IW(0xafa80010);

                /* Call ex_call_helper(). */
                /* lui  $t9, f@h */             IW(0x3c190000 | hi16(f));
                /* ori  $t9, $t9, f@l */        IW(0x37390000 | lo16(f));
                /* move $s2, $ra */             IW(0x03e09025);
                /* jalr $t9 */                  IW(0x0320f809);
                /* nop */                       IW(0x00000000);
                /* move $ra, $s2 */             IW(0x0240f825);
                /* addiu $sp, $sp, 32 */        IW(0x27bd0020);

                /* If failed: */
                EXCEPTION_IF_ZERO(REG_V0);
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
        uint32_t tmp;
        uint32_t arg_addr;
        int i;
        uint32_t f;

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

        f = (uint32_t)ex_thiscall_helper;

        /* if (!ex_thiscall_helper(env, dst, obj, symbol, len, hash, arg_count, arg)) return false; */
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
                /* ori $t0, $zero, arg_count */ IW(0x24080000 | lo16((uint32_t)arg_count));
                /* sw $t0, 24($sp) */           IW(0xafa80018);

                /* Arg8 [sp+28] = arg */
                /* lui $t0, arg@h */            IW(0x3c080000 | hi16(arg_addr));
                /* ori $t0, $t0, arg@l */       IW(0x35080000 | lo16(arg_addr));
                /* sw $t0, 28($sp) */           IW(0xafa8001c);

                /* Call ex_thiscall_helper(). */
                /* lui  $t9, f@h */             IW(0x3c190000 | hi16(f));
                /* ori  $t9, $t9, f@l */        IW(0x37390000 | lo16(f));
                /* move $s2, $ra */             IW(0x03e09025);
                /* jalr $t9 */                  IW(0x0320f809);
                /* nop */                       IW(0x00000000);
                /* move $ra, $s2 */             IW(0x0240f825);
                /* addiu $sp, $sp, 32 */        IW(0x27bd0020);

                /* If failed: */
                EXCEPTION_IF_ZERO(REG_V0);
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
                /* b 0 */       IW(0x10000000);
                /* nop */       IW(0x00000000);
                IW(0x00000000); IW(0x00000000);
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

        ASM {
                /* $v0 = ex_condition_helper(env, src): 1/0, -1 on error. */
                IW(0x02002025);
                IW(0x24050000 | tvar16(src));
                IW(0x3c190000 | hi16((uint32_t)ex_condition_helper));
                IW(0x37390000 | lo16((uint32_t)ex_condition_helper));
                IW(0x03e09025);
                IW(0x0320f809);
                IW(0x00000000);
                IW(0x0240f825);
                /* if ($v0 == -1) goto exception_code */
                IW(0x2408ffff);
                EXCEPTION_IF_EQUAL(REG_V0, REG_T0);
                /* Branch patching expects the condition in $at. */
                IW(0x00400825);
        }

        /* Patch later. */
        ctx->branch_patch[ctx->branch_patch_count].code = ctx->code;
        ctx->branch_patch[ctx->branch_patch_count].lpc = target_lpc;
        ctx->branch_patch[ctx->branch_patch_count].type = PATCH_BNE;
        ctx->branch_patch_count++;

        ASM {
                /* Patched later. */
                /* bne $at, 0, target */        IW(0x14200000);
                /* nop */                       IW(0x00000000);
                IW(0x00000000); IW(0x00000000); IW(0x00000000); IW(0x00000000);
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

        ASM {
                /* $v0 = ex_condition_helper(env, src): 1/0, -1 on error. */
                IW(0x02002025);
                IW(0x24050000 | tvar16(src));
                IW(0x3c190000 | hi16((uint32_t)ex_condition_helper));
                IW(0x37390000 | lo16((uint32_t)ex_condition_helper));
                IW(0x03e09025);
                IW(0x0320f809);
                IW(0x00000000);
                IW(0x0240f825);
                IW(0x2408ffff);
                EXCEPTION_IF_EQUAL(REG_V0, REG_T0);
                IW(0x00400825);
        }

        /* Patch later. */
        ctx->branch_patch[ctx->branch_patch_count].code = ctx->code;
        ctx->branch_patch[ctx->branch_patch_count].lpc = target_lpc;
        ctx->branch_patch[ctx->branch_patch_count].type = PATCH_BEQ;
        ctx->branch_patch_count++;

        ASM {
                /* Patched later. */
                /* beq $at, 0, target */        IW(0x10200000);
                /* nop */                       IW(0x00000000);
                IW(0x00000000); IW(0x00000000); IW(0x00000000); IW(0x00000000);
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
                /* beq $at, 0, taget */         IW(0x10200000);
                /* nop */                       IW(0x00000000);
                IW(0x00000000); IW(0x00000000); IW(0x00000000); IW(0x00000000);
        }

        return true;
}

/* Visit a OP_SAFEPOINT instruction. */
static inline bool
jit_visit_safepoint_op(
        struct rt_jit_context *ctx)
{
        uint32_t f;

        f = (uint32_t)ex_safepoint_helper;

        /* if (!ex_safepoint_helper(env)) return false; */
        ASM {
                /* $s0: env */
                /* $s1: &env->frame->tmpvar[0] */

                /* Arg1 $a0 = env */
                /* move $a0, $s0 */             IW(0x02002025);

                /* Call ex_call_helper(). */
                /* lui  $t9, f@h */             IW(0x3c190000 | hi16(f));
                /* ori  $t9, $t9, f@l */        IW(0x37390000 | lo16(f));
                /* move $s2, $ra */             IW(0x03e09025);
                /* jalr $t9 */                  IW(0x0320f809);
                /* nop */                       IW(0x00000000);
                /* move $ra, $s2 */             IW(0x0240f825);

                /* If failed: */
                EXCEPTION_IF_ZERO(REG_V0);
        }

        return true;
}

/* Visit a OP_PBASE instruction. (ABCE; inline machine code, mips32 BE.)
 * The pointer union member sits in the FIRST word (+8); the 64-bit
 * long stores it BE: high(0) at +8, low(ptr) at +12. */
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
                /* $s1: &env->frame->tmpvar[0] */

                /* lw $t0, src+8($s1)  (packed pointer member) */
                IW(0x8E280000 | lo16((uint32_t)(src + 8)));
                /* lw $t0, buf_ofs($t0) */      IW(0x8D080000 | lo16(buf_ofs));
                /* li $t2, LONG */              IW(0x240a0000 | lo16((uint32_t)NOCT_VALUE_LONG));
                /* sw $t2, dst($s1) */          IW(0xAE2A0000 | lo16((uint32_t)dst));
                /* sw $zero, dst+8($s1) */      IW(0xAE200000 | lo16((uint32_t)(dst + 8)));
                /* sw $t0, dst+12($s1) */       IW(0xAE280000 | lo16((uint32_t)(dst + 12)));
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

/* Visit a OP_PLOAD8U instruction. (ABCE; inline machine code, mips32 BE.) */
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
                /* $s1: &env->frame->tmpvar[0] */

                /* lw $t0, base+12($s1)  (BE: low word of the long) */
                IW(0x8E280000 | lo16((uint32_t)(base + 12)));
                /* lw $t1, ofs+8($s1) */        IW(0x8E290000 | lo16((uint32_t)(ofs + 8)));
                /* addu $t0,$t0,$t1 */        IW(0x01094021);
                /* load elem -> $t1 */          IW(0x90000000 | (8 << 21) | (9 << 16));
                /* li $t2, INT */               IW(0x240a0000 | lo16((uint32_t)NOCT_VALUE_INT));
                /* sw $t2, dst($s1) */          IW(0xAE2A0000 | lo16((uint32_t)dst));
                /* sw $t1, dst+8($s1) */        IW(0xAE290000 | lo16((uint32_t)(dst + 8)));
        }

        return true;
}

/* Visit a OP_PSTORE8 instruction. (ABCE; inline, mips32 BE. Int source.) */
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
                /* $s1: &env->frame->tmpvar[0] */

                /* lw $t0, base+12($s1)  (BE: low word of the long) */
                IW(0x8E280000 | lo16((uint32_t)(base + 12)));
                /* lw $t1, ofs+8($s1) */        IW(0x8E290000 | lo16((uint32_t)(ofs + 8)));
                /* addu $t0,$t0,$t1 */        IW(0x01094021);
                /* lw $t2, src+8($s1) */        IW(0x8E2A0000 | lo16((uint32_t)(src + 8)));
                /* store elem */                IW(0xa0000000 | (8 << 21) | (10 << 16));
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

/* Visit a OP_PLOAD8S instruction. (ABCE; inline machine code, mips32 BE.) */
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
                /* $s1: &env->frame->tmpvar[0] */

                /* lw $t0, base+12($s1)  (BE: low word of the long) */
                IW(0x8E280000 | lo16((uint32_t)(base + 12)));
                /* lw $t1, ofs+8($s1) */        IW(0x8E290000 | lo16((uint32_t)(ofs + 8)));
                /* addu $t0,$t0,$t1 */        IW(0x01094021);
                /* load elem -> $t1 */          IW(0x80000000 | (8 << 21) | (9 << 16));
                /* li $t2, INT */               IW(0x240a0000 | lo16((uint32_t)NOCT_VALUE_INT));
                /* sw $t2, dst($s1) */          IW(0xAE2A0000 | lo16((uint32_t)dst));
                /* sw $t1, dst+8($s1) */        IW(0xAE290000 | lo16((uint32_t)(dst + 8)));
        }

        return true;
}

/* Visit a OP_PLOAD16U instruction. (ABCE; inline machine code, mips32 BE.) */
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
                /* $s1: &env->frame->tmpvar[0] */

                /* lw $t0, base+12($s1)  (BE: low word of the long) */
                IW(0x8E280000 | lo16((uint32_t)(base + 12)));
                /* lw $t1, ofs+8($s1) */        IW(0x8E290000 | lo16((uint32_t)(ofs + 8)));
                /* sll $t1,$t1,1 */       IW(0x00094800 | (1 << 6));
                /* addu $t0,$t0,$t1 */        IW(0x01094021);
                /* load elem -> $t1 */          IW(0x94000000 | (8 << 21) | (9 << 16));
                /* li $t2, INT */               IW(0x240a0000 | lo16((uint32_t)NOCT_VALUE_INT));
                /* sw $t2, dst($s1) */          IW(0xAE2A0000 | lo16((uint32_t)dst));
                /* sw $t1, dst+8($s1) */        IW(0xAE290000 | lo16((uint32_t)(dst + 8)));
        }

        return true;
}

/* Visit a OP_PLOAD16S instruction. (ABCE; inline machine code, mips32 BE.) */
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
                /* $s1: &env->frame->tmpvar[0] */

                /* lw $t0, base+12($s1)  (BE: low word of the long) */
                IW(0x8E280000 | lo16((uint32_t)(base + 12)));
                /* lw $t1, ofs+8($s1) */        IW(0x8E290000 | lo16((uint32_t)(ofs + 8)));
                /* sll $t1,$t1,1 */       IW(0x00094800 | (1 << 6));
                /* addu $t0,$t0,$t1 */        IW(0x01094021);
                /* load elem -> $t1 */          IW(0x84000000 | (8 << 21) | (9 << 16));
                /* li $t2, INT */               IW(0x240a0000 | lo16((uint32_t)NOCT_VALUE_INT));
                /* sw $t2, dst($s1) */          IW(0xAE2A0000 | lo16((uint32_t)dst));
                /* sw $t1, dst+8($s1) */        IW(0xAE290000 | lo16((uint32_t)(dst + 8)));
        }

        return true;
}

/* Visit a OP_PLOAD32 instruction. (ABCE; inline machine code, mips32 BE.) */
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
                /* $s1: &env->frame->tmpvar[0] */

                /* lw $t0, base+12($s1)  (BE: low word of the long) */
                IW(0x8E280000 | lo16((uint32_t)(base + 12)));
                /* lw $t1, ofs+8($s1) */        IW(0x8E290000 | lo16((uint32_t)(ofs + 8)));
                /* sll $t1,$t1,2 */       IW(0x00094800 | (2 << 6));
                /* addu $t0,$t0,$t1 */        IW(0x01094021);
                /* load elem -> $t1 */          IW(0x8c000000 | (8 << 21) | (9 << 16));
                /* li $t2, INT */               IW(0x240a0000 | lo16((uint32_t)NOCT_VALUE_INT));
                /* sw $t2, dst($s1) */          IW(0xAE2A0000 | lo16((uint32_t)dst));
                /* sw $t1, dst+8($s1) */        IW(0xAE290000 | lo16((uint32_t)(dst + 8)));
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

/* Visit a OP_PSTORE16 instruction. (ABCE; inline, mips32 BE. Int source.) */
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
                /* $s1: &env->frame->tmpvar[0] */

                /* lw $t0, base+12($s1)  (BE: low word of the long) */
                IW(0x8E280000 | lo16((uint32_t)(base + 12)));
                /* lw $t1, ofs+8($s1) */        IW(0x8E290000 | lo16((uint32_t)(ofs + 8)));
                /* sll $t1,$t1,1 */       IW(0x00094800 | (1 << 6));
                /* addu $t0,$t0,$t1 */        IW(0x01094021);
                /* lw $t2, src+8($s1) */        IW(0x8E2A0000 | lo16((uint32_t)(src + 8)));
                /* store elem */                IW(0xa4000000 | (8 << 21) | (10 << 16));
        }

        return true;
}

/* Visit a OP_PSTORE32 instruction. (ABCE; inline, mips32 BE. Int source.) */
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
                /* $s1: &env->frame->tmpvar[0] */

                /* lw $t0, base+12($s1)  (BE: low word of the long) */
                IW(0x8E280000 | lo16((uint32_t)(base + 12)));
                /* lw $t1, ofs+8($s1) */        IW(0x8E290000 | lo16((uint32_t)(ofs + 8)));
                /* sll $t1,$t1,2 */       IW(0x00094800 | (2 << 6));
                /* addu $t0,$t0,$t1 */        IW(0x01094021);
                /* lw $t2, src+8($s1) */        IW(0x8E2A0000 | lo16((uint32_t)(src + 8)));
                /* store elem */                IW(0xac000000 | (8 << 21) | (10 << 16));
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


/* Visit an OP_VLOADI32X4 instruction. */
static INLINE bool
jit_visit_vloadi32x4_op(
        struct rt_jit_context *ctx)
{
        uint32_t vreg_ofs;
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
        IW(0x3c0c0000 | hi16(vreg_ofs));
        IW(0x358c0000 | lo16(vreg_ofs));
        IW(0x01906021);

        base = base_tmp * (int)sizeof(struct rt_value) + 12;
        ofs = ofs_tmp * (int)sizeof(struct rt_value) + 8;
        IW(0x8e280000 | lo16((uint32_t)base));
        IW(0x8e290000 | lo16((uint32_t)ofs));
        IW(0x00094880); /* sll t1,t1,2 */
        IW(0x01094021); /* addu t0,t0,t1 */
        for (lane = 0; lane < 4; lane++) {
                IW(0x8d0a0000 | (uint32_t)lane * 4);
                IW(0xad8a0000 | (uint32_t)vd * 16 + (uint32_t)lane * 4);
        }
        return true;
}

/* Visit an OP_VSTOREI32X4 instruction. */
static INLINE bool
jit_visit_vstorei32x4_op(
        struct rt_jit_context *ctx)
{
        uint32_t vreg_ofs;
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
        IW(0x3c0c0000 | hi16(vreg_ofs));
        IW(0x358c0000 | lo16(vreg_ofs));
        IW(0x01906021);

        base = base_tmp * (int)sizeof(struct rt_value) + 12;
        ofs = ofs_tmp * (int)sizeof(struct rt_value) + 8;
        IW(0x8e280000 | lo16((uint32_t)base));
        IW(0x8e290000 | lo16((uint32_t)ofs));
        IW(0x00094880);
        IW(0x01094021);
        for (lane = 0; lane < 4; lane++) {
                IW(0x8d8a0000 | (uint32_t)vs * 16 + (uint32_t)lane * 4);
                IW(0xad0a0000 | (uint32_t)lane * 4);
        }
        return true;
}

/* Visit an OP_VSPLATI32 instruction. */
static INLINE bool
jit_visit_vsplati32_op(
        struct rt_jit_context *ctx)
{
        uint32_t vreg_ofs;
        int vd;
        int src_tmp;
        int lane;
        int src;

        CONSUME_IMM8(vd);
        CONSUME_TMPVAR(src_tmp);

        vreg_ofs = (uint32_t)offsetof(struct rt_env, vreg);
        IW(0x3c0c0000 | hi16(vreg_ofs));
        IW(0x358c0000 | lo16(vreg_ofs));
        IW(0x01906021);

        src = src_tmp * (int)sizeof(struct rt_value) + 8;
        IW(0x8e2a0000 | lo16((uint32_t)src));
        for (lane = 0; lane < 4; lane++)
                IW(0xad8a0000 | (uint32_t)vd * 16 + (uint32_t)lane * 4);
        return true;
}

/* Visit an OP_VGETLANEI32 instruction. */
static INLINE bool
jit_visit_vgetlanei32_op(
        struct rt_jit_context *ctx)
{
        uint32_t vreg_ofs;
        int dst_tmp;
        int vs;
        int lane_index;
        int d;
        uint32_t tag;

        CONSUME_TMPVAR(dst_tmp);
        CONSUME_IMM8(vs);
        CONSUME_IMM8(lane_index);

        vreg_ofs = (uint32_t)offsetof(struct rt_env, vreg);
        IW(0x3c0c0000 | hi16(vreg_ofs));
        IW(0x358c0000 | lo16(vreg_ofs));
        IW(0x01906021);

        d = dst_tmp * (int)sizeof(struct rt_value);
        tag = (uint32_t)(NOCT_VALUE_INT);
        IW(0x8d8a0000 | (uint32_t)vs * 16 + (uint32_t)lane_index * 4);
        IW(0x240b0000 | tag);
        IW(0xae2b0000 | lo16((uint32_t)d));
        IW(0xae2a0000 | lo16((uint32_t)(d + 8)));
        return true;
}

/* Visit an OP_VMOV128 instruction. */
static INLINE bool
jit_visit_vmov128_op(
        struct rt_jit_context *ctx)
{
        uint32_t vreg_ofs;
        int vd;
        int vs;
        int lane;

        CONSUME_IMM8(vd);
        CONSUME_IMM8(vs);

        vreg_ofs = (uint32_t)offsetof(struct rt_env, vreg);
        IW(0x3c0c0000 | hi16(vreg_ofs));
        IW(0x358c0000 | lo16(vreg_ofs));
        IW(0x01906021);

        for (lane = 0; lane < 4; lane++) {
                IW(0x8d8a0000 | (uint32_t)vs * 16 + (uint32_t)lane * 4);
                IW(0xad8a0000 | (uint32_t)vd * 16 + (uint32_t)lane * 4);
        }
        return true;
}

/* Visit an OP_VADDI32X4 instruction. */
static INLINE bool
jit_visit_vaddi32x4_op(
        struct rt_jit_context *ctx)
{
        uint32_t vreg_ofs;
        int vd;
        int lhs;
        int rhs;
        int lane;

        CONSUME_IMM8(vd);
        CONSUME_IMM8(lhs);
        CONSUME_IMM8(rhs);

        vreg_ofs = (uint32_t)offsetof(struct rt_env, vreg);
        IW(0x3c0c0000 | hi16(vreg_ofs));
        IW(0x358c0000 | lo16(vreg_ofs));
        IW(0x01906021);

        for (lane = 0; lane < 4; lane++) {
                uint32_t a;
                uint32_t b;
                uint32_t d;
                uint32_t word;

                a = (uint32_t)lhs * 16 + (uint32_t)lane * 4;
                b = (uint32_t)rhs * 16 + (uint32_t)lane * 4;
                d = (uint32_t)vd * 16 + (uint32_t)lane * 4;
                IW(0x8d880000 | a);
                IW(0x8d890000 | b);
                word = 0x01095021;
                IW(word);
                IW(0xad8a0000 | d);
        }
        return true;
}

/* Visit an OP_VSUBI32X4 instruction. */
static INLINE bool
jit_visit_vsubi32x4_op(
        struct rt_jit_context *ctx)
{
        uint32_t vreg_ofs;
        int vd;
        int lhs;
        int rhs;
        int lane;

        CONSUME_IMM8(vd);
        CONSUME_IMM8(lhs);
        CONSUME_IMM8(rhs);

        vreg_ofs = (uint32_t)offsetof(struct rt_env, vreg);
        IW(0x3c0c0000 | hi16(vreg_ofs));
        IW(0x358c0000 | lo16(vreg_ofs));
        IW(0x01906021);

        for (lane = 0; lane < 4; lane++) {
                uint32_t a;
                uint32_t b;
                uint32_t d;
                uint32_t word;

                a = (uint32_t)lhs * 16 + (uint32_t)lane * 4;
                b = (uint32_t)rhs * 16 + (uint32_t)lane * 4;
                d = (uint32_t)vd * 16 + (uint32_t)lane * 4;
                IW(0x8d880000 | a);
                IW(0x8d890000 | b);
                word = 0x01095023;
                IW(word);
                IW(0xad8a0000 | d);
        }
        return true;
}

/* Visit an OP_VMULI32X4 instruction. */
static INLINE bool
jit_visit_vmuli32x4_op(
        struct rt_jit_context *ctx)
{
        uint32_t vreg_ofs;
        int vd;
        int lhs;
        int rhs;
        int lane;

        CONSUME_IMM8(vd);
        CONSUME_IMM8(lhs);
        CONSUME_IMM8(rhs);

        vreg_ofs = (uint32_t)offsetof(struct rt_env, vreg);
        IW(0x3c0c0000 | hi16(vreg_ofs));
        IW(0x358c0000 | lo16(vreg_ofs));
        IW(0x01906021);

        for (lane = 0; lane < 4; lane++) {
                uint32_t a;
                uint32_t b;
                uint32_t d;
                uint32_t word;

                a = (uint32_t)lhs * 16 + (uint32_t)lane * 4;
                b = (uint32_t)rhs * 16 + (uint32_t)lane * 4;
                d = (uint32_t)vd * 16 + (uint32_t)lane * 4;
                IW(0x8d880000 | a);
                IW(0x8d890000 | b);
                word = 0x71095002;
                IW(word);
                IW(0xad8a0000 | d);
        }
        return true;
}

/* Visit an OP_VAND128 instruction. */
static INLINE bool
jit_visit_vand128_op(
        struct rt_jit_context *ctx)
{
        uint32_t vreg_ofs;
        int vd;
        int lhs;
        int rhs;
        int lane;

        CONSUME_IMM8(vd);
        CONSUME_IMM8(lhs);
        CONSUME_IMM8(rhs);

        vreg_ofs = (uint32_t)offsetof(struct rt_env, vreg);
        IW(0x3c0c0000 | hi16(vreg_ofs));
        IW(0x358c0000 | lo16(vreg_ofs));
        IW(0x01906021);

        for (lane = 0; lane < 4; lane++) {
                uint32_t a;
                uint32_t b;
                uint32_t d;
                uint32_t word;

                a = (uint32_t)lhs * 16 + (uint32_t)lane * 4;
                b = (uint32_t)rhs * 16 + (uint32_t)lane * 4;
                d = (uint32_t)vd * 16 + (uint32_t)lane * 4;
                IW(0x8d880000 | a);
                IW(0x8d890000 | b);
                word = 0x01095024;
                IW(word);
                IW(0xad8a0000 | d);
        }
        return true;
}

/* Visit an OP_VOR128 instruction. */
static INLINE bool
jit_visit_vor128_op(
        struct rt_jit_context *ctx)
{
        uint32_t vreg_ofs;
        int vd;
        int lhs;
        int rhs;
        int lane;

        CONSUME_IMM8(vd);
        CONSUME_IMM8(lhs);
        CONSUME_IMM8(rhs);

        vreg_ofs = (uint32_t)offsetof(struct rt_env, vreg);
        IW(0x3c0c0000 | hi16(vreg_ofs));
        IW(0x358c0000 | lo16(vreg_ofs));
        IW(0x01906021);

        for (lane = 0; lane < 4; lane++) {
                uint32_t a;
                uint32_t b;
                uint32_t d;
                uint32_t word;

                a = (uint32_t)lhs * 16 + (uint32_t)lane * 4;
                b = (uint32_t)rhs * 16 + (uint32_t)lane * 4;
                d = (uint32_t)vd * 16 + (uint32_t)lane * 4;
                IW(0x8d880000 | a);
                IW(0x8d890000 | b);
                word = 0x01095025;
                IW(word);
                IW(0xad8a0000 | d);
        }
        return true;
}

/* Visit an OP_VXOR128 instruction. */
static INLINE bool
jit_visit_vxor128_op(
        struct rt_jit_context *ctx)
{
        uint32_t vreg_ofs;
        int vd;
        int lhs;
        int rhs;
        int lane;

        CONSUME_IMM8(vd);
        CONSUME_IMM8(lhs);
        CONSUME_IMM8(rhs);

        vreg_ofs = (uint32_t)offsetof(struct rt_env, vreg);
        IW(0x3c0c0000 | hi16(vreg_ofs));
        IW(0x358c0000 | lo16(vreg_ofs));
        IW(0x01906021);

        for (lane = 0; lane < 4; lane++) {
                uint32_t a;
                uint32_t b;
                uint32_t d;
                uint32_t word;

                a = (uint32_t)lhs * 16 + (uint32_t)lane * 4;
                b = (uint32_t)rhs * 16 + (uint32_t)lane * 4;
                d = (uint32_t)vd * 16 + (uint32_t)lane * 4;
                IW(0x8d880000 | a);
                IW(0x8d890000 | b);
                word = 0x01095026;
                IW(word);
                IW(0xad8a0000 | d);
        }
        return true;
}

/* Visit an OP_VSHLI32X4 instruction. */
static INLINE bool
jit_visit_vshli32x4_op(
        struct rt_jit_context *ctx)
{
        uint32_t vreg_ofs;
        int vd;
        int vs;
        int shift;
        int lane;

        CONSUME_IMM8(vd);
        CONSUME_IMM8(vs);
        CONSUME_IMM8(shift);

        vreg_ofs = (uint32_t)offsetof(struct rt_env, vreg);
        IW(0x3c0c0000 | hi16(vreg_ofs));
        IW(0x358c0000 | lo16(vreg_ofs));
        IW(0x01906021);

        IW(0x240b0000 | ((uint32_t)shift & 31u));
        for (lane = 0; lane < 4; lane++) {
                uint32_t s;
                uint32_t d;

                s = (uint32_t)vs * 16 + (uint32_t)lane * 4;
                d = (uint32_t)vd * 16 + (uint32_t)lane * 4;
                IW(0x8d880000 | s);
                IW(0x01685004);
                IW(0xad8a0000 | d);
        }
        return true;
}

/* Visit an OP_VSHRI32X4 instruction. */
static INLINE bool
jit_visit_vshri32x4_op(
        struct rt_jit_context *ctx)
{
        uint32_t vreg_ofs;
        int vd;
        int vs;
        int shift;
        int lane;

        CONSUME_IMM8(vd);
        CONSUME_IMM8(vs);
        CONSUME_IMM8(shift);

        vreg_ofs = (uint32_t)offsetof(struct rt_env, vreg);
        IW(0x3c0c0000 | hi16(vreg_ofs));
        IW(0x358c0000 | lo16(vreg_ofs));
        IW(0x01906021);

        IW(0x240b0000 | ((uint32_t)shift & 31u));
        for (lane = 0; lane < 4; lane++) {
                uint32_t s;
                uint32_t d;

                s = (uint32_t)vs * 16 + (uint32_t)lane * 4;
                d = (uint32_t)vd * 16 + (uint32_t)lane * 4;
                IW(0x8d880000 | s);
                IW(0x01685006);
                IW(0xad8a0000 | d);
        }
        return true;
}

/* Visit an OP_VLOADF32X4 instruction. */
static INLINE bool
jit_visit_vloadf32x4_op(
        struct rt_jit_context *ctx)
{
        uint32_t vreg_ofs;
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
        IW(0x3c0c0000 | hi16(vreg_ofs));
        IW(0x358c0000 | lo16(vreg_ofs));
        IW(0x01906021);

        base = base_tmp * (int)sizeof(struct rt_value) + 12;
        ofs = ofs_tmp * (int)sizeof(struct rt_value) + 8;
        IW(0x8e280000 | lo16((uint32_t)base));
        IW(0x8e290000 | lo16((uint32_t)ofs));
        IW(0x00094880); /* sll t1,t1,2 */
        IW(0x01094021); /* addu t0,t0,t1 */
        for (lane = 0; lane < 4; lane++) {
                IW(0x8d0a0000 | (uint32_t)lane * 4);
                IW(0xad8a0000 | (uint32_t)vd * 16 + (uint32_t)lane * 4);
        }
        return true;
}

/* Visit an OP_VSTOREF32X4 instruction. */
static INLINE bool
jit_visit_vstoref32x4_op(
        struct rt_jit_context *ctx)
{
        uint32_t vreg_ofs;
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
        IW(0x3c0c0000 | hi16(vreg_ofs));
        IW(0x358c0000 | lo16(vreg_ofs));
        IW(0x01906021);

        base = base_tmp * (int)sizeof(struct rt_value) + 12;
        ofs = ofs_tmp * (int)sizeof(struct rt_value) + 8;
        IW(0x8e280000 | lo16((uint32_t)base));
        IW(0x8e290000 | lo16((uint32_t)ofs));
        IW(0x00094880);
        IW(0x01094021);
        for (lane = 0; lane < 4; lane++) {
                IW(0x8d8a0000 | (uint32_t)vs * 16 + (uint32_t)lane * 4);
                IW(0xad0a0000 | (uint32_t)lane * 4);
        }
        return true;
}

/* Visit an OP_VSPLATF32 instruction. */
static INLINE bool
jit_visit_vsplatf32_op(
        struct rt_jit_context *ctx)
{
        uint32_t vreg_ofs;
        int vd;
        int src_tmp;
        int lane;
        int src;

        CONSUME_IMM8(vd);
        CONSUME_TMPVAR(src_tmp);

        vreg_ofs = (uint32_t)offsetof(struct rt_env, vreg);
        IW(0x3c0c0000 | hi16(vreg_ofs));
        IW(0x358c0000 | lo16(vreg_ofs));
        IW(0x01906021);

        src = src_tmp * (int)sizeof(struct rt_value) + 8;
        IW(0x8e2a0000 | lo16((uint32_t)src));
        for (lane = 0; lane < 4; lane++)
                IW(0xad8a0000 | (uint32_t)vd * 16 + (uint32_t)lane * 4);
        return true;
}

/* Visit an OP_VGETLANEF32 instruction. */
static INLINE bool
jit_visit_vgetlanef32_op(
        struct rt_jit_context *ctx)
{
        uint32_t vreg_ofs;
        int dst_tmp;
        int vs;
        int lane_index;
        int d;
        uint32_t tag;

        CONSUME_TMPVAR(dst_tmp);
        CONSUME_IMM8(vs);
        CONSUME_IMM8(lane_index);

        vreg_ofs = (uint32_t)offsetof(struct rt_env, vreg);
        IW(0x3c0c0000 | hi16(vreg_ofs));
        IW(0x358c0000 | lo16(vreg_ofs));
        IW(0x01906021);

        d = dst_tmp * (int)sizeof(struct rt_value);
        tag = (uint32_t)(NOCT_VALUE_FLOAT);
        IW(0x8d8a0000 | (uint32_t)vs * 16 + (uint32_t)lane_index * 4);
        IW(0x240b0000 | tag);
        IW(0xae2b0000 | lo16((uint32_t)d));
        IW(0xae2a0000 | lo16((uint32_t)(d + 8)));
        return true;
}

/* Visit an OP_VADDF32X4 instruction. */
static INLINE bool
jit_visit_vaddf32x4_op(
        struct rt_jit_context *ctx)
{
        uint32_t vreg_ofs;
        int vd;
        int lhs;
        int rhs;
        int lane;

        CONSUME_IMM8(vd);
        CONSUME_IMM8(lhs);
        CONSUME_IMM8(rhs);

        vreg_ofs = (uint32_t)offsetof(struct rt_env, vreg);
        IW(0x3c0c0000 | hi16(vreg_ofs));
        IW(0x358c0000 | lo16(vreg_ofs));
        IW(0x01906021);

        for (lane = 0; lane < 4; lane++) {
                uint32_t a;
                uint32_t b;
                uint32_t d;
                uint32_t word;

                a = (uint32_t)lhs * 16 + (uint32_t)lane * 4;
                b = (uint32_t)rhs * 16 + (uint32_t)lane * 4;
                d = (uint32_t)vd * 16 + (uint32_t)lane * 4;
                IW(0xc5800000 | a);
                IW(0xc5820000 | b);
                word = 0x46020100;
                IW(word);
                IW(0xe5840000 | d);
        }
        return true;
}

/* Visit an OP_VSUBF32X4 instruction. */
static INLINE bool
jit_visit_vsubf32x4_op(
        struct rt_jit_context *ctx)
{
        uint32_t vreg_ofs;
        int vd;
        int lhs;
        int rhs;
        int lane;

        CONSUME_IMM8(vd);
        CONSUME_IMM8(lhs);
        CONSUME_IMM8(rhs);

        vreg_ofs = (uint32_t)offsetof(struct rt_env, vreg);
        IW(0x3c0c0000 | hi16(vreg_ofs));
        IW(0x358c0000 | lo16(vreg_ofs));
        IW(0x01906021);

        for (lane = 0; lane < 4; lane++) {
                uint32_t a;
                uint32_t b;
                uint32_t d;
                uint32_t word;

                a = (uint32_t)lhs * 16 + (uint32_t)lane * 4;
                b = (uint32_t)rhs * 16 + (uint32_t)lane * 4;
                d = (uint32_t)vd * 16 + (uint32_t)lane * 4;
                IW(0xc5800000 | a);
                IW(0xc5820000 | b);
                word = 0x46020101;
                IW(word);
                IW(0xe5840000 | d);
        }
        return true;
}

/* Visit an OP_VMULF32X4 instruction. */
static INLINE bool
jit_visit_vmulf32x4_op(
        struct rt_jit_context *ctx)
{
        uint32_t vreg_ofs;
        int vd;
        int lhs;
        int rhs;
        int lane;

        CONSUME_IMM8(vd);
        CONSUME_IMM8(lhs);
        CONSUME_IMM8(rhs);

        vreg_ofs = (uint32_t)offsetof(struct rt_env, vreg);
        IW(0x3c0c0000 | hi16(vreg_ofs));
        IW(0x358c0000 | lo16(vreg_ofs));
        IW(0x01906021);

        for (lane = 0; lane < 4; lane++) {
                uint32_t a;
                uint32_t b;
                uint32_t d;
                uint32_t word;

                a = (uint32_t)lhs * 16 + (uint32_t)lane * 4;
                b = (uint32_t)rhs * 16 + (uint32_t)lane * 4;
                d = (uint32_t)vd * 16 + (uint32_t)lane * 4;
                IW(0xc5800000 | a);
                IW(0xc5820000 | b);
                word = 0x46020102;
                IW(word);
                IW(0xe5840000 | d);
        }
        return true;
}

/* Visit an OP_VDIVF32X4 instruction. */
static INLINE bool
jit_visit_vdivf32x4_op(
        struct rt_jit_context *ctx)
{
        uint32_t vreg_ofs;
        int vd;
        int lhs;
        int rhs;
        int lane;

        CONSUME_IMM8(vd);
        CONSUME_IMM8(lhs);
        CONSUME_IMM8(rhs);

        vreg_ofs = (uint32_t)offsetof(struct rt_env, vreg);
        IW(0x3c0c0000 | hi16(vreg_ofs));
        IW(0x358c0000 | lo16(vreg_ofs));
        IW(0x01906021);

        for (lane = 0; lane < 4; lane++) {
                uint32_t a;
                uint32_t b;
                uint32_t d;
                uint32_t word;

                a = (uint32_t)lhs * 16 + (uint32_t)lane * 4;
                b = (uint32_t)rhs * 16 + (uint32_t)lane * 4;
                d = (uint32_t)vd * 16 + (uint32_t)lane * 4;
                IW(0xc5800000 | a);
                IW(0xc5820000 | b);
                word = 0x46020103;
                IW(word);
                IW(0xe5840000 | d);
        }
        return true;
}

/* Visit an OP_VCVTI32F32X4 instruction. */
static INLINE bool
jit_visit_vcvti32f32x4_op(
        struct rt_jit_context *ctx)
{
        uint32_t vreg_ofs;
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
        IW(0x3c0c0000 | hi16(vreg_ofs));
        IW(0x358c0000 | lo16(vreg_ofs));
        IW(0x01906021);

        ASM_BINARY_OP(noct_ex_vcvti32f32x4_helper);
        return true;
}

/* Visit an OP_VCVTF32I32X4 instruction. */
static INLINE bool
jit_visit_vcvtf32i32x4_op(
        struct rt_jit_context *ctx)
{
        uint32_t vreg_ofs;
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
        IW(0x3c0c0000 | hi16(vreg_ofs));
        IW(0x358c0000 | lo16(vreg_ofs));
        IW(0x01906021);

        ASM_BINARY_OP(noct_ex_vcvtf32i32x4_helper);
        return true;
}

/* Visit an OP_VMINS32X4 instruction. */
static INLINE bool
jit_visit_vmins32x4_op(
        struct rt_jit_context *ctx)
{
        uint32_t vreg_ofs;
        int vd;
        int lhs;
        int rhs;

        CONSUME_IMM8(vd);
        CONSUME_IMM8(lhs);
        CONSUME_IMM8(rhs);

        vreg_ofs = (uint32_t)offsetof(struct rt_env, vreg);
        IW(0x3c0c0000 | hi16(vreg_ofs));
        IW(0x358c0000 | lo16(vreg_ofs));
        IW(0x01906021);

        assert(NEVER_COME_HERE);

        return false;
}

/* Visit an OP_VMAXS32X4 instruction. */
static INLINE bool
jit_visit_vmaxs32x4_op(
        struct rt_jit_context *ctx)
{
        uint32_t vreg_ofs;
        int vd;
        int lhs;
        int rhs;

        CONSUME_IMM8(vd);
        CONSUME_IMM8(lhs);
        CONSUME_IMM8(rhs);

        vreg_ofs = (uint32_t)offsetof(struct rt_env, vreg);
        IW(0x3c0c0000 | hi16(vreg_ofs));
        IW(0x358c0000 | lo16(vreg_ofs));
        IW(0x01906021);

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
                /* s0: env */
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
        for (i = 0; i < (int)ctx->pc_entry_count; i++) {
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
        offset = (int)((intptr_t)target_code - (intptr_t)ctx->branch_patch[patch_index].code - 4) / 4;
        /* Set the assembler cursor. */
        ctx->code = ctx->branch_patch[patch_index].code;

        /* Assemble. */
        if (ctx->branch_patch[patch_index].type == PATCH_BAL) {
                if (getenv("NOCT_JIT_FORCE_LONG_BRANCH") == NULL &&
                    offset >= -32768 && offset <= 32767) {
                        ASM {
                                IW(0x10000000 | lo16((uint32_t)offset));
                                IW(0); IW(0); IW(0);
                        }
                } else if (!jit_put_abs_jump(ctx, (uint32_t)(uintptr_t)target_code)) {
                        return false;
                }
        } else if (ctx->branch_patch[patch_index].type == PATCH_BEQ) {
                if (getenv("NOCT_JIT_FORCE_LONG_BRANCH") == NULL &&
                    offset >= -32768 && offset <= 32767) {
                        ASM {
                                IW(0x10200000 | lo16((uint32_t)offset));
                                IW(0); IW(0); IW(0); IW(0); IW(0);
                        }
                } else {
                        ASM { IW(0x14200005); IW(0); }
                        if (!jit_put_abs_jump(ctx, (uint32_t)(uintptr_t)target_code)) return false;
                }
        } else if (ctx->branch_patch[patch_index].type == PATCH_BNE) {
                if (getenv("NOCT_JIT_FORCE_LONG_BRANCH") == NULL &&
                    offset >= -32768 && offset <= 32767) {
                        ASM {
                                IW(0x14200000 | lo16((uint32_t)offset));
                                IW(0); IW(0); IW(0); IW(0); IW(0);
                        }
                } else {
                        ASM { IW(0x10200005); IW(0); }
                        if (!jit_put_abs_jump(ctx, (uint32_t)(uintptr_t)target_code)) return false;
                }
        }

        return true;
}

#endif /* defined(NOCT_ARCH_MIPS32) && defined(NOCT_USE_JIT) */
