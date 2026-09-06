/* -*- coding: utf-8; tab-width: 8; indent-tabs-mode: nil; -*- */

/*
 * Noct Programming Language
 * Copyright (c) 2025, 2026, Awe Morris
 */

/*
 * JIT (ppc32): Just-In-Time native code generation
 */

#include <noct/noct.h>

#if defined(NOCT_ARCH_PPC32) && defined(NOCT_USE_JIT)

#include "runtime.h"
#include "jit.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#if defined(__linux__)
#include <sys/auxv.h>
#include <asm/cputable.h>
#endif

/* False asseretion */
#define JIT_OP_NOT_IMPLEMENTED  0
#define NEVER_COME_HERE         0

/* Branch patch type */
#define PATCH_BAL               0
#define PATCH_BEQ               1
#define PATCH_BNE               2

/* Decoration */
#define ASM

/* Registers */
#define REG_R0          0       /* volatile */
#define REG_R1          1       /* stack pointer */
#define REG_R2          2       /* (TOC pointer) */
#define REG_R3          3       /* volatile, parameter, return */
#define REG_R4          4       /* volatile, parameter */
#define REG_R5          5       /* volatile, parameter */
#define REG_R6          6       /* volatile, parameter */
#define REG_R7          7       /* volatile, parameter */
#define REG_R8          8       /* volatile, parameter */
#define REG_R9          9       /* volatile, parameter */
#define REG_R10         10      /* volatile, parameter */
#define REG_R11         11      /* (volatile, environment pointer) */
#define REG_R12         12      /* (exception handling, glink) */
#define REG_R13         13      /* (thread ID) */
#define REG_R14         14      /* rt, non-volatile, local */
#define REG_R15         15      /* env->frame->tmpvar[0], non-volatile, local */
#define REG_R16         16      /* exception_handler, non-volatile, local */
#define REG_R17         17      /* (non-volatile, local) */
#define REG_R18         18      /* (non-volatile, local) */
#define REG_R19         19      /* (non-volatile, local) */
#define REG_R20         20      /* (non-volatile, local) */
#define REG_R21         21      /* (non-volatile, local) */
#define REG_R22         22      /* (non-volatile, local) */
#define REG_R23         23      /* (non-volatile, local) */
#define REG_R24         24      /* (non-volatile, local) */
#define REG_R25         25      /* (non-volatile, local) */
#define REG_R26         26      /* (non-volatile, local) */
#define REG_R27         27      /* (non-volatile, local) */
#define REG_R28         28      /* (non-volatile, local) */
#define REG_R29         29      /* (non-volatile, local) */
#define REG_R30         30      /* (non-volatile, local) */
#define REG_R31         31      /* (non-volatile, local) */

/* Forward declarations */
static bool jit_visit_bytecode(struct rt_jit_context *ctx);
static bool jit_patch_branch(struct rt_jit_context *ctx, int patch_index);
static uint32_t jit_detect_simd_caps(void);
#if defined(NOCT_USE_OPTIMIZER)
static bool jit_put_altivec_sync(struct rt_jit_context *ctx, bool load);
#endif

/*
 * Generate a JIT-compiled code for a function.
 */
bool
jit_build(
          struct rt_env *env,
          struct rt_func *func)
{
	return rt_jit_build_standard(env, func, jit_detect_simd_caps(), "ppc32",
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
#define IW(w)                           if (!jit_put_word(ctx, w)) return false
static INLINE bool
jit_put_word(
        struct rt_jit_context *ctx,
        uint32_t word)
{
        uint32_t tmp;

        if ((uint32_t *)ctx->code >= (uint32_t *)ctx->code_end) {
		ctx->code_overflow = true;
                rt_error(ctx->env, N_TR("Code too big."));
                return false;
        }

        tmp = ((word & 0xff) << 24) |
              (((word >> 8) & 0xff) << 16) |
              (((word >> 16) & 0xff) << 8) |
              ((word >> 24) & 0xff);

        *(uint32_t *)ctx->code = tmp;
        ctx->code = (uint32_t *)ctx->code + 1;

        return true;
}

/* Convert a Power ISA instruction word to the byte-oriented IW form. */
static INLINE uint32_t
jit_ppc_iw(uint32_t word)
{
        return ((word & 0xff) << 24) |
               (((word >> 8) & 0xff) << 16) |
               (((word >> 16) & 0xff) << 8) |
               ((word >> 24) & 0xff);
}

static INLINE uint32_t
jit_ppc_vx(uint32_t base, int vd, int va, int vb)
{
        return jit_ppc_iw(base | ((uint32_t)vd << 21) |
                          ((uint32_t)va << 16) | ((uint32_t)vb << 11));
}

/* Save/reload the eight program-visible volatile AltiVec registers. */
#if defined(NOCT_USE_OPTIMIZER)
static bool
jit_put_altivec_sync(struct rt_jit_context *ctx, bool load)
{
        uint32_t ofs;
        uint32_t hi;
        int i;

        ofs = (uint32_t)offsetof(struct rt_env, vreg);
        hi = (ofs + 0x8000) >> 16;

        /* addis r11,r14,vreg@ha; addi r11,r11,vreg@l; li r12,0 */
        IW(jit_ppc_iw(0x3d6e0000 | (hi & 0xffff)));
        IW(jit_ppc_iw(0x396b0000 | (ofs & 0xffff)));
        IW(jit_ppc_iw(0x39800000));
        for (i = 0; i < 8; i++) {
                IW(jit_ppc_vx(load ? 0x7c0000ce : 0x7c0001ce,
                              i, REG_R11, REG_R12));
                if (i != 7)
                        IW(jit_ppc_iw(0x398c0010)); /* addi r12,r12,16 */
        }
        return true;
}
#endif

/*
 * Templates
 */

static INLINE uint32_t hi16(uint32_t d)
{
        uint32_t b2;
        uint32_t b3;

        b2 = (d >> 16) & 0xff;
        b3 = (d >> 24) & 0xff;

        return (b2 << 24) | (b3 << 16);
}

static INLINE uint32_t lo16(uint32_t d)
{
        uint32_t b0;
        uint32_t b1;

        b0 = d & 0xff;
        b1 = (d >> 8) & 0xff;

        return (b0 << 24) | (b1 << 16);
}

static INLINE uint32_t tvar16(int d)
{
        uint32_t b0;
        uint32_t b1;

        b0 = d & 0xff;
        b1 = (d >> 8) & 0xff;

        return (b0 << 24) | (b1 << 16);
}

#define EXCEPTION_IF_EQUAL() if (!jit_put_exception_if_equal(ctx)) return false
static INLINE bool jit_put_exception_if_equal(struct rt_jit_context *ctx)
{
        intptr_t offset;

        /* Invert EQ and skip the following wide unconditional branch. */
        if (!jit_put_word(ctx, 0x08008240))
                return false;
        offset = (intptr_t)ctx->exception_code - (intptr_t)ctx->code;
        if (offset < -33554432 || offset > 33554428) {
                rt_error(ctx->env, N_TR("Exception target too far."));
                return false;
        }
        return jit_put_word(ctx,
                            0x00000048 |
                            (((uint32_t)offset & 0xff) << 24) |
                            ((((uint32_t)offset >> 8) & 0xff) << 16) |
                            ((((uint32_t)offset >> 16) & 0xff) << 8) |
                            (((uint32_t)offset >> 24) & 0x03));
}

#define ASM_BINARY_OP(f)                                                                        \
        ASM {                                                                                   \
                /* R14: env */                                                                  \
                /* R15: &env->frame->tmpvar[0] */                                               \
                /* R31: saved LR */                                                             \
                                                                                                \
                /* Arg1 R3: env */                                                               \
                /* mr r3, r14 */                IW(0x7873c37d);                                 \
                                                                                                \
                /* Arg2 R4: dst */                                                              \
                /* li r4, dst */                IW(0x00008038 | tvar16(dst));                   \
                                                                                                \
                /* Arg3 R5: src1 */                                                             \
                /* li r5, src1 */               IW(0x0000a038 | tvar16(src1));                  \
                                                                                                \
                /* Arg4 R6: src2 */                                                             \
                /* li r6, src2 */               IW(0x0000c038 | tvar16(src2));                  \
                                                                                                \
                /* Call f(). */                                                                 \
                /* lis r12, f[31:16] */         IW(0x0000803d | hi16((uint32_t)f));             \
                /* ori  r12, r12, f[15:0] */    IW(0x00008c61 | lo16((uint32_t)f));             \
                /* mflr r31 */                  IW(0xa602e87f);                                 \
                /* mtctr r12 */                 IW(0xa603897d);                                 \
                /* bctrl */                     IW(0x2104804e);                                 \
                /* mtlr r31 */                  IW(0xa603e87f);                                 \
                                                                                                \
                /* If failed: */                                                                \
                /* cmpwi r3, 0 */               IW(0x0000032c);                                 \
                EXCEPTION_IF_EQUAL();                                                           \
        }

#define ASM_UNARY_OP(f)                                                                         \
        ASM {                                                                                   \
                /* R14: env */                                                                  \
                /* R15: &env->frame->tmpvar[0] */                                               \
                /* R31: saved LR */                                                             \
                                                                                                \
                /* Arg1 R3: env */                                                               \
                /* mr r3, r14 */                IW(0x7873c37d);                                 \
                                                                                                \
                /* Arg2 R4: dst */                                                              \
                /* li r4, dst */                IW(0x00008038 | tvar16(dst));                   \
                                                                                                \
                /* Arg3 R5: src */                                                              \
                /* li r5, src */                IW(0x0000a038 | tvar16(src));                   \
                                                                                                \
                /* Call f(). */                                                                 \
                /* lis r12, f[31:16] */         IW(0x0000803d | hi16((uint32_t)f));             \
                /* ori  r12, r12, f[15:0] */    IW(0x00008c61 | lo16((uint32_t)f));             \
                /* mflr r31 */                  IW(0xa602e87f);                                 \
                /* mtctr r12 */                 IW(0xa603897d);                                 \
                /* bctrl */                     IW(0x2104804e);                                 \
                /* mtlr r31 */                  IW(0xa603e87f);                                 \
                                                                                                \
                /* If failed: */                                                                \
                /* cmpwi r3, 0 */               IW(0x0000032c);                                 \
                EXCEPTION_IF_EQUAL();                                                           \
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
                /* R14: env */
                /* R15: &env->frame->tmpvar[0] */
                /* R31: saved LR */

                /* rt->line = line; */
                /* li r0, line */       IW(0x00000038 | lo16(line));
                /* env->line is at offset 4 on 32-bit targets. */
                /* stw r0, 4(r14) */    IW(0x04000e90);
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
                /* R14: env */
                /* R15: &env->frame->tmpvar[0] */
                /* R31: saved LR */

                /* R3 = dst_addr = &env->frame->tmpvar[dst] */
                /* li r3, dst */        IW(0x00006038 | lo16((uint32_t)dst));
                /* add r3, r3, r15 */   IW(0x147a637c);

                /* R4 = src_addr = &env->frame->tmpvar[src] */
                /* li r4, src */        IW(0x00008038 | lo16((uint32_t)src));
                /* add r4, r4, r15 */   IW(0x147a847c);

                /* *dst_addr = *src_addr */
                /* lwz r5, 0(r4) */     IW(0x0000a480);
                /* lwz r6, 4(r4) */     IW(0x0400c480);
                /* stw r5, 0(r3) */     IW(0x0000a390);
                /* stw r6, 4(r3) */     IW(0x0400c390);
                /* lwz r5, 8(r4) */     IW(0x0800a480);
                /* lwz r6, 12(r4) */    IW(0x0c00c480);
                /* stw r5, 8(r3) */     IW(0x0800a390);
                /* stw r6, 12(r3) */    IW(0x0c00c390);
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
                /* R14: env */
                /* R15: &env->frame->tmpvar[0] */
                /* R31: saved LR */

                /* R3 = dst_addr = &env->frame->tmpvar[dst] */
                /* li r3, dst */        IW(0x00006038 | tvar16(dst));
                /* add r3, r3, r15 */   IW(0x147a637c);

                /* env->frame->tmpvar[dst].type = NOCT_VALUE_INT */
                /* li r4, 0 */          IW(0x00008038);
                /* stw r4, 0(r3) */     IW(0x00008390);

                /* env->frame->tmpvar[dst].val.i = val */
                /* lis r4, val@h */     IW(0x0000803c | hi16(val));
                /* ori r4, r4, val@l */ IW(0x00008460 | lo16(val));
                /* stw r4, 8(r3) */     IW(0x08008390);
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
                /* R14: env */
                /* R15: &env->frame->tmpvar[0] */
                /* R31: saved LR */

                /* R3 = dst_addr = &env->frame->tmpvar[dst] */
                /* li r3, dst */        IW(0x00006038 | tvar16(dst));
                /* add r3, r3, r15 */   IW(0x147a637c);

                /* env->frame->tmpvar[dst].type = NOCT_VALUE_LONG */
                /* li r4, 5 */          IW(0x05008038);
                /* stw r4, 0(r3) */     IW(0x00008390);

                /* env->frame->tmpvar[dst].val.i = val */
#if defined(NOCT_ARCH_LE)
                /* lis r4, val@h */     IW(0x0000803c | hi16((uint32_t)(val & 0xffffffff)));
                /* ori r4, r4, val@l */ IW(0x00008460 | lo16((uint32_t)(val & 0xffffffff)));
                /* stw r4, 8(r3) */     IW(0x08008390);
                /* lis r4, val@h */     IW(0x0000803c | hi16((uint32_t)((val >> 32) & 0xffffffff)));
                /* ori r4, r4, val@l */ IW(0x00008460 | lo16((uint32_t)((val >> 32) & 0xffffffff)));
                /* stw r4, 12(r3) */    IW(0x0c008390);
#else
                /* lis r4, val@h */     IW(0x0000803c | hi16((uint32_t)((val >> 32) & 0xffffffff)));
                /* ori r4, r4, val@l */ IW(0x00008460 | lo16((uint32_t)((val >> 32) & 0xffffffff)));
                /* stw r4, 8(r3) */     IW(0x08008390);
                /* lis r4, val@h */     IW(0x0000803c | hi16((uint32_t)(val & 0xffffffff)));
                /* ori r4, r4, val@l */ IW(0x00008460 | lo16((uint32_t)(val & 0xffffffff)));
                /* stw r4, 12(r3) */    IW(0x0c008390);
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
                /* R14: env */
                /* R15: &env->frame->tmpvar[0] */
                /* R31: saved LR */

                /* R3 = dst_addr = &env->frame->tmpvar[dst] */
                /* li r3, dst */        IW(0x00006038 | lo16((uint32_t)dst));
                /* add r3, r3, r15 */   IW(0x147a637c);

                /* env->frame->tmpvar[dst].type = NOCT_VALUE_FLOAT */
                /* li r4, 1 */          IW(0x01008038);
                /* stw r4, 0(r3) */     IW(0x00008390);

                /* env->frame->tmpvar[dst].val.i = val */
                /* lis r4, val@h */             IW(0x0000803c | hi16(val));
                /* ori r4, r4, val@l */         IW(0x00008460 | lo16(val));
                /* stw r4, 8(r3) */             IW(0x08008390);
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
                /* R14: env */
                /* R15: &env->frame->tmpvar[0] */
                /* R31: saved LR */

                /* R3 = dst_addr = &env->frame->tmpvar[dst] */
                /* li r3, dst */        IW(0x00006038 | tvar16(dst));
                /* add r3, r3, r15 */   IW(0x147a637c);

                /* env->frame->tmpvar[dst].type = NOCT_VALUE_DOUBLE */
                /* li r4, 6 */          IW(0x06008038);
                /* stw r4, 0(r3) */     IW(0x00008390);

                /* env->frame->tmpvar[dst].val.i = val */
#if defined(NOCT_ARCH_LE)
                /* lis r4, val@h */     IW(0x0000803c | hi16((uint32_t)(val & 0xffffffff)));
                /* ori r4, r4, val@l */ IW(0x00008460 | lo16((uint32_t)(val & 0xffffffff)));
                /* stw r4, 8(r3) */     IW(0x08008390);
                /* lis r4, val@h */     IW(0x0000803c | hi16((uint32_t)((val >> 32) & 0xffffffff)));
                /* ori r4, r4, val@l */ IW(0x00008460 | lo16((uint32_t)((val >> 32) & 0xffffffff)));
                /* stw r4, 12(r3) */    IW(0x0c008390);
#else
                /* lis r4, val@h */     IW(0x0000803c | hi16((uint32_t)((val >> 32) & 0xffffffff)));
                /* ori r4, r4, val@l */ IW(0x00008460 | lo16((uint32_t)((val >> 32) & 0xffffffff)));
                /* stw r4, 8(r3) */     IW(0x08008390);
                /* lis r4, val@h */     IW(0x0000803c | hi16((uint32_t)(val & 0xffffffff)));
                /* ori r4, r4, val@l */ IW(0x00008460 | lo16((uint32_t)(val & 0xffffffff)));
                /* stw r4, 12(r3) */    IW(0x0c008390);
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
        uint32_t len, hash, f;

        CONSUME_TMPVAR(dst);
        CONSUME_STRING(val, len, hash);

        f = (uint32_t)ex_make_string_with_hash;
        dst *= (int)sizeof(struct rt_value);

        /* rt_make_string_with_hash(env, &env->frame->tmpvar[dst], val, len, hash); */
        ASM {
                /* R14: env */
                /* R15: &env->frame->tmpvar[0] */
                /* R31: saved LR */

                /* Arg1 R3: env */
                /* mr r3, r14 */                IW(0x7873c37d);

                /* Arg2 R4 = dst_addr = &env->frame->tmpvar[dst] */
                /* li r4, dst */                IW(0x00008038 | lo16((uint32_t)dst));
                /* add r4, r4, r15 */           IW(0x147a847c);

                /* Arg3: R5 = val */
                /* lis  r5, val[31:16] */       IW(0x0000a03c | hi16((uint32_t)val));
                /* ori  r5, r5, val[15:0] */    IW(0x0000a560 | lo16((uint32_t)val));

                /* Arg4: R6 = len */
                /* lis  r6, r6, len[31:16] */   IW(0x0000c03c | hi16(len));
                /* ori  r6, r6, len[15:0] */    IW(0x0000c660 | lo16(len));

                /* Arg5 R7 = hash */
                /* lis  r7, hash[31:16] */      IW(0x0000e03c | hi16(hash));
                /* ori  r7, r7, hash[15:0] */   IW(0x0000e760 | lo16(hash));

                /* Call rt_make_string_with_hash(). */
                /* lis  r12, f[31:16] */        IW(0x0000803d | hi16(f));
                /* ori  r12, r12, f[15:0] */    IW(0x00008c61 | lo16(f));
                /* mflr r31 */                  IW(0xa602e87f);
                /* mtctr r12 */                 IW(0xa603897d);
                /* bctrl */                     IW(0x2104804e);
                /* mtlr r31 */                  IW(0xa603e87f);

                /* If failed: */
                /* cmpwi r3, 0 */               IW(0x0000032c);
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
        uint32_t f;

        CONSUME_TMPVAR(dst);

        f = (uint32_t)ex_make_empty_array;
        dst *= (int)sizeof(struct rt_value);

        /* rt_make_empty_array(env, &env->frame->tmpvar[dst]); */
        ASM {
                /* R14: env */
                /* R15: &env->frame->tmpvar[0] */
                /* R31: saved LR */

                /* Arg1 R3: env */
                /* mr r3, r14 */                IW(0x7873c37d);

                /* Arg2 R4 = dst_addr = &env->frame->tmpvar[dst] */
                /* li r4, dst */                IW(0x00008038 | lo16((uint32_t)dst));
                /* add r4, r4, r15 */           IW(0x147a847c);

                /* Call rt_make_empty_array(). */
                /* lis  r12, f[31:16] */        IW(0x0000803d | hi16(f));
                /* ori  r12, r12, f[15:0] */    IW(0x00008c61 | lo16(f));
                /* mflr r31 */                  IW(0xa602e87f);
                /* mtctr r12 */                 IW(0xa603897d);
                /* bctrl */                     IW(0x2104804e);
                /* mtlr r31 */                  IW(0xa603e87f);

                /* If failed: */
                /* cmpwi r3, 0 */               IW(0x0000032c);
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
        uint32_t f;

        CONSUME_TMPVAR(dst);

        f = (uint32_t)ex_make_empty_dict;
        dst *= (int)sizeof(struct rt_value);

        /* rt_make_empty_dict(env, &env->frame->tmpvar[dst]); */
        ASM {
                /* R14: env */
                /* R15: &env->frame->tmpvar[0] */
                /* R31: saved LR */

                /* Arg1 R3: env */
                /* mr r3, r14 */                IW(0x7873c37d);

                /* Arg2 R4 = dst_addr = &env->frame->tmpvar[dst] */
                /* li r4, dst */                IW(0x00008038 | lo16((uint32_t)dst));
                /* add r4, r4, r15 */           IW(0x147a847c);

                /* Call rt_make_empty_dict(). */
                /* lis  r12, f[31:16] */        IW(0x0000803d | hi16(f));
                /* ori  r12, r12, f[15:0] */    IW(0x00008c61 | lo16(f));
                /* mflr r31 */                  IW(0xa602e87f);
                /* mtctr r12 */                 IW(0xa603897d);
                /* bctrl */                     IW(0x2104804e);
                /* mtlr r31 */                  IW(0xa603e87f);

                /* If failed: */
                /* cmpwi r3, 0 */               IW(0x0000032c);
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

        dst *= (int)sizeof(struct rt_value);

        /* Increment an integer. */
        ASM {
                /* R14: env */
                /* R15: &env->frame->tmpvar[0] */
                /* R31: saved LR */

                /* R3 = dst_addr = &env->frame->tmpvar[dst] */
                /* li r3, dst */        IW(0x00006038 | lo16((uint32_t)dst));
                /* add r3, r3, r15 */   IW(0x147a637c);

                /* env->frame->tmpvar[dst].val.i++ */
                /* lwz r4, 8(r3) */     IW(0x08008380);
		/* IW is byte-oriented: place the signed I-form immediate with
		 * lo16(), rather than shifting the host integer into ISA bit 16.
		 * The latter encoded step 1 as 256 on both PPC endian variants. */
		/* addi r4, r4, step */ IW(0x00008438 |
					 lo16((uint32_t)step));
                /* stw r4, 8(r3) */     IW(0x08008390);
        }

        return true;
}

#if defined(NOCT_USE_OPTIMIZER)
static INLINE bool
jit_visit_vindex_hint_op(struct rt_jit_context *ctx)
{
	int a;
	int b;
	int c;
	int required_vregs;
	int lanes;
	int flags;

	CONSUME_TMPVAR(a);
	CONSUME_TMPVAR(b);
	CONSUME_TMPVAR(c);
	CONSUME_IMM8(required_vregs);
	CONSUME_IMM8(lanes);
	CONSUME_IMM8(flags);

	UNUSED_PARAMETER(a);
	UNUSED_PARAMETER(b);
	UNUSED_PARAMETER(c);
	UNUSED_PARAMETER(lanes);
	UNUSED_PARAMETER(flags);

	if (required_vregs > 8)
		ctx->simd_caps = 0;

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
#endif

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
		IW(0x00006038 | lo16((uint32_t)value));
		IW(0x147a637c);
		IW(0x08008380);
		/* addi r4, r4, -decrement (byte-oriented signed imm16) */
		IW(0x00008438 |
		   lo16((uint32_t)(int32_t)(-(int16_t)decrement)));
		IW(0x08008390);
		IW(0x0000042c);
	}
	ctx->branch_patch[ctx->branch_patch_count].code = ctx->code;
	ctx->branch_patch[ctx->branch_patch_count].lpc = target_lpc;
	ctx->branch_patch[ctx->branch_patch_count].type = PATCH_BNE;
	ctx->branch_patch_count++;
	ASM {
		IW(0x00008240);
		IW(0x00000060);
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

        /* if (!ex_shl_helper(env, dst, src1, src2)) return false; */
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

        /* if (!ex_shr_helper(env, dst, src1, src2)) return false; */
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
                /* R14: env */
                /* R15: &env->frame->tmpvar[0] */
                /* R31: saved LR */

                /* R3 = src1_addr = &env->frame->tmpvar[src1] */
                /* li r3, src */        IW(0x00006038 | lo16((uint32_t)src1));
                /* add r3, r3, r15 */   IW(0x147a637c);
                /* lwz r3, 8(r3) */     IW(0x08006380);

                /* R4 = src2_addr = &env->frame->tmpvar[src2] */
                /* li r4, src2 */       IW(0x00008038 | lo16((uint32_t)src2));
                /* add r4, r4, r15 */   IW(0x147a847c);
                /* lwz r4, 8(r4) */     IW(0x08008480);

                /* src1 == src2 */
                /* cmpw r3, r4 */       IW(0x0020037c);
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
        uint32_t len, hash, src, f;

        CONSUME_TMPVAR(dst);
        CONSUME_STRING(src_s, len, hash);

        src = (uint32_t)(intptr_t)src_s;
        f = (uint32_t)ex_loadsymbol_helper;

        /* if (!ex_loadsymbol_helper(env, dst, src, len, hash)) return false; */
        ASM {
                /* R14: env */
                /* R15: &env->frame->tmpvar[0] */
                /* R31: saved LR */

                /* Arg1 R3 = env */
                /* mr r3, r14 */                IW(0x7873c37d);

                /* Arg2 R4 = dst */
                /* li r4, dst */                IW(0x00008038 | tvar16(dst));

                /* Arg3 R5 = src */
                /* lis  r5, src[31:16] */       IW(0x0000a03c | hi16(src));
                /* ori  r5, r5, src[15:0] */    IW(0x0000a560 | lo16(src));

                /* Arg4 R6 = len */
                /* lis  r6, len[31:16] */       IW(0x0000c03c | hi16(len));
                /* ori  r6, r6, len[15:0] */    IW(0x0000c660 | lo16(len));

                /* Arg5 R7 = hash */
                /* lis  r7, hash[31:16] */      IW(0x0000e03c | hi16(hash));
                /* ori  r7, r7, hash[15:0] */   IW(0x0000e760 | lo16(hash));

                /* Call rt_loadsymbol_helper(). */
                /* lis  r12, f[31:16] */        IW(0x0000803d | hi16(f));
                /* ori  r12, r12, f[15:0] */    IW(0x00008c61 | lo16(f));
                /* mflr r31 */                  IW(0xa602e87f);
                /* mtctr r12 */                 IW(0xa603897d);
                /* bctrl */                     IW(0x2104804e);
                /* mtlr r31 */                  IW(0xa603e87f);

                /* If failed: */
                /* cmpwi r3, 0 */               IW(0x0000032c);
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
        uint32_t dst;
        uint32_t len, hash;
        int src;
        uint32_t f;

        CONSUME_STRING(dst_s, len, hash);
        CONSUME_TMPVAR(src);

        dst = (uint32_t)(intptr_t)dst_s;
        f = (uint32_t)ex_storesymbol_helper;

        /* if (!ex_storesymbol_helper(env, dst, len, hash, src)) return false; */
        ASM {
                /* R14: env */
                /* R15: &env->frame->tmpvar[0] */
                /* R31: saved LR */

                /* Arg1 R3 = env */
                /* mr r3, r14 */                IW(0x7873c37d);

                /* Arg2: R4 = dst */
                /* lis  r4, dst[31:16] */       IW(0x0000803c | hi16(dst));
                /* ori  r4, r4, dst[15:0] */    IW(0x00008460 | lo16(dst));

                /* Arg3 R5 = len */
                /* lis  r5, len[31:16] */       IW(0x0000a03c | hi16(len));
                /* ori  r5, r5, len[15:0] */    IW(0x0000a560 | lo16(len));

                /* Arg4 R6 = hash */
                /* lis  r6, hash[31:16] */      IW(0x0000c03c | hi16(hash));
                /* ori  r6, r6, hash[15:0] */   IW(0x0000c660 | lo16(hash));

                /* Arg5 R7 = src */
                /* li r7, src */                IW(0x0000e038 | lo16(src));

                /* Call ex_storesymbol_helper(). */
                /* lis  r12, f[31:16] */        IW(0x0000803d | hi16(f));
                /* ori  r12, r12, f[15:0] */    IW(0x00008c61 | lo16(f));
                /* mflr r31 */                  IW(0xa602e87f);
                /* mtctr r12 */                 IW(0xa603897d);
                /* bctrl */                     IW(0x2104804e);
                /* mtlr r31 */                  IW(0xa603e87f);

                /* If failed: */
                /* cmpwi r3, 0 */               IW(0x0000032c);
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
        uint32_t len, hash, field;
        uint32_t f;

        CONSUME_TMPVAR(dst);
        CONSUME_TMPVAR(dict);
        CONSUME_STRING(field_s, len, hash);

        field = (uint32_t)(intptr_t)field_s;
        f = (uint32_t)ex_loaddot_helper;

        /* if (!ex_loaddot_helper(env, dst, dict, field, len, hash)) return false; */
        ASM {
                /* R14: env */
                /* R15: &env->frame->tmpvar[0] */
                /* R31: saved LR */

                /* Arg1 R3 = env */
                /* mr r3, r14 */                IW(0x7873c37d);

                /* Arg2 R4 = dst */
                /* li r4, dst */                IW(0x00008038 | tvar16(dst));

                /* Arg3 R5 = dict */
                /* li r5, dict */               IW(0x0000a038 | tvar16(dict));

                /* Arg4 R6 = field */
                /* lis  r6, r6, field[31:16] */ IW(0x0000c03c | hi16(field));
                /* ori  r6, r6, field[15:0] */  IW(0x0000c660 | lo16(field));

                /* Arg5 R7 = len */
                /* lis  r7, len[31:16] */       IW(0x0000e03c | hi16(len));
                /* ori  r7, r7, len[15:0] */    IW(0x0000e760 | lo16(len));

                /* Arg6 R8: hash */
                /* lis  r8, hash[31:16] */      IW(0x0000003d | hi16(hash));
                /* ori  r8, r8, hash[15:0] */   IW(0x00000861 | lo16(hash));

                /* Call ex_loaddot_helper(). */
                /* lis  r12, f[31:16] */        IW(0x0000803d | hi16(f));
                /* ori  r12, r12, f[15:0] */    IW(0x00008c61 | lo16(f));
                /* mflr r31 */                  IW(0xa602e87f);
                /* mtctr r12 */                 IW(0xa603897d);
                /* bctrl */                     IW(0x2104804e);
                /* mtlr r31 */                  IW(0xa603e87f);

                /* If failed: */
                /* cmpwi r3, 0 */               IW(0x0000032c);
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
        uint32_t len, hash, field;
        int src;
        uint32_t f;

        CONSUME_TMPVAR(dict);
        CONSUME_STRING(field_s, len, hash);
        CONSUME_TMPVAR(src);

        field = (uint32_t)(intptr_t)field_s;
        f = (uint32_t)ex_storedot_helper;

        /* if (!ex_storedot_helper(env, dict, field, len, hash, src)) return false; */
        ASM {
                /* R14: env */
                /* R15: &env->frame->tmpvar[0] */
                /* R31: saved LR */

                /* Arg1 R3 = env */
                /* mr r3, r14 */                IW(0x7873c37d);

                /* Arg2 R4 = dict */
                /* li r4, dict */               IW(0x00008038 | tvar16(dict));

                /* Arg3 R5 = field */
                /* lis  r5, field[31:16] */     IW(0x0000a03c | hi16(field));
                /* ori  r5, r5, field[15:0] */  IW(0x0000a560 | lo16(field));

                /* Arg4 R6 = len */
                /* lis  r6, len[31:16] */       IW(0x0000c03c | hi16(len));
                /* ori  r6, r6, len[15:0] */    IW(0x0000c660 | lo16(len));

                /* Arg5 R7 = hash */
                /* lis  r7, hash[31:16] */      IW(0x0000e03c | hi16(hash));
                /* ori  r7, r7, hash[15:0] */   IW(0x0000e760 | lo16(hash));

                /* Arg6 R8: src */
                /* li r8, src */                IW(0x00000039 | tvar16(src));

                /* Call ex_storedot_helper(). */
                /* lis  r12, f[31:16] */        IW(0x0000803d | hi16(f));
                /* ori  r12, r12, f[15:0] */    IW(0x00008c61 | lo16(f));
                /* mflr r31 */                  IW(0xa602e87f);
                /* mtctr r12 */                 IW(0xa603897d);
                /* bctrl */                     IW(0x2104804e);
                /* mtlr r31 */                  IW(0xa603e87f);

                /* If failed: */
                /* cmpwi r3, 0 */               IW(0x0000032c);
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

        /* Embed arguments to the code. */
        if (arg_count > 0) {
                tmp = (uint32_t)(4 + 4 * arg_count);
                ASM {
                        /* b tmp */
                        IW(0x00000048 | lo16(tmp));
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
                /* R14: env */
                /* R15: &env->frame->tmpvar[0] */
                /* R31: saved LR */

                /* Arg1 R3 = env */
                /* mr r3, r14 */                IW(0x7873c37d);

                /* Arg2 R4 = dst */
                /* li r4, dst */                IW(0x00008038 | tvar16(dst));

                /* Arg3 R5 = func */
                /* li r5, func */               IW(0x0000a038 | tvar16(func));

                /* Arg4 R6: arg_count */
                /* li r6, arg_count */          IW(0x0000c038 | lo16((uint32_t)arg_count));

                /* Arg5 R7 = arg */
                /* lis  r7, arg[31:16] */       IW(0x0000e03c | hi16(arg_addr));
                /* ori  r7, r7, arg[15:0] */    IW(0x0000e760 | lo16(arg_addr));

                /* Call ex_call_helper(). */
                /* lis  r12, f[31:16] */        IW(0x0000803d | hi16(f));
                /* ori  r12, r12, f[15:0] */    IW(0x00008c61 | lo16(f));
                /* mflr r31 */                  IW(0xa602e87f);
                /* mtctr r12 */                 IW(0xa603897d);
                /* bctrl */                     IW(0x2104804e);
                /* mtlr r31 */                  IW(0xa603e87f);

                /* If failed: */
                /* cmpwi r3, 0 */               IW(0x0000032c);
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

        /* Embed arguments. */
        if (arg_count > 0) {
                tmp = (uint32_t)(4 + 4 * arg_count);
                ASM {
                        /* b tmp */
                        IW(0x00000048 | lo16(tmp));
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
                /* R14: env */
                /* R15: &rhenv->frame->tmpvar[0] */
                /* R31: saved LR */

                /* Arg1 R3 = env */
                /* mr r3, r14 */                IW(0x7873c37d);

                /* Arg2 R4 = dst */
                /* li r4, dst */                IW(0x00008038 | tvar16(dst));

                /* Arg3 R5 = obj */
                /* li r5, obj */                IW(0x0000a038 | tvar16(obj));

                /* Arg4 R6 = symbol */
                /* lis  r6, symbol[31:16] */    IW(0x0000c03c | hi16((uint32_t)symbol));
                /* ori  r6, r6, symbol[15:0] */ IW(0x0000c660 | lo16((uint32_t)symbol));

                /* Arg5 R7 = len */
                /* lis  r7, len[31:16] */       IW(0x0000e03c | hi16(len));
                /* ori  r7, r7, len[15:0] */    IW(0x0000e760 | lo16(len));

                /* Arg6 R8: hash */
                /* lis  r8, hash[31:16] */      IW(0x0000003d | hi16(hash));
                /* ori  r8, r8, hash[15:0] */   IW(0x00000861 | lo16(hash));

                /* Arg7 R9 = arg_count */
                /* li r9, arg_count */          IW(0x00002039 | lo16((uint32_t)arg_count));

                /* Arg8 R10: arg */
                /* lis  r10, arg[31:16] */      IW(0x0000403d | hi16(arg_addr));
                /* ori  r10, r10, arg[15:0] */  IW(0x00004a61 | lo16(arg_addr));

                /* Call ex_thiscall_helper(). */
                /* lis  r12, f[31:16] */        IW(0x0000803d | hi16(f));
                /* ori  r12, r12, f[15:0] */    IW(0x00008c61 | lo16(f));
                /* mflr r31 */                  IW(0xa602e87f);
                /* mtctr r12 */                 IW(0xa603897d);
                /* bctrl */                     IW(0x2104804e);
                /* mtlr r31 */                  IW(0xa603e87f);

                /* If failed: */
                /* cmpwi r3, 0 */               IW(0x0000032c);
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
                /* b 0 */       IW(0x00000048);
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
                /* r3 = ex_condition_helper(env, src): 1/0, -1 on error. */
                IW(0x7873c37d);
                IW(0x00008038 | tvar16(src));
                IW(0x0000803d | hi16((uint32_t)ex_condition_helper));
                IW(0x00008c61 | lo16((uint32_t)ex_condition_helper));
                IW(0xa602e87f);
                IW(0xa603897d);
                IW(0x2104804e);
                IW(0xa603e87f);
                /* -1 means the helper reported a type error. */
                IW(0xffff032c);
                EXCEPTION_IF_EQUAL();
                /* Compare truth value with zero for the patched branch. */
                IW(0x0000032c);
        }

        /* Patch later. */
        ctx->branch_patch[ctx->branch_patch_count].code = ctx->code;
        ctx->branch_patch[ctx->branch_patch_count].lpc = target_lpc;
        ctx->branch_patch[ctx->branch_patch_count].type = PATCH_BNE;
        ctx->branch_patch_count++;

        ASM {
                /* Patched later. */
                /* bne 0 */     IW(0x00008240);
                /* nop: reserved long-branch slot */ IW(0x00000060);
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
                /* r3 = ex_condition_helper(env, src): 1/0, -1 on error. */
                IW(0x7873c37d);
                IW(0x00008038 | tvar16(src));
                IW(0x0000803d | hi16((uint32_t)ex_condition_helper));
                IW(0x00008c61 | lo16((uint32_t)ex_condition_helper));
                IW(0xa602e87f);
                IW(0xa603897d);
                IW(0x2104804e);
                IW(0xa603e87f);
                IW(0xffff032c);
                EXCEPTION_IF_EQUAL();
                IW(0x0000032c);
        }

        /* Patch later. */
        ctx->branch_patch[ctx->branch_patch_count].code = ctx->code;
        ctx->branch_patch[ctx->branch_patch_count].lpc = target_lpc;
        ctx->branch_patch[ctx->branch_patch_count].type = PATCH_BEQ;
        ctx->branch_patch_count++;

        ASM {
                /* Patched later. */
                /* beq 0 */     IW(0x00008241);
                /* nop: reserved long-branch slot */ IW(0x00000060);
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
                /* beq 0 */     IW(0x00008241);
                /* nop: reserved long-branch slot */ IW(0x00000060);
        }

        return true;
}

/* Visit a OP_SAFEPOINT instruction. */
static INLINE bool
jit_visit_safepoint_op(
        struct rt_jit_context *ctx)
{
        uint32_t f;

        f = (uint32_t)ex_safepoint_helper;

        /* if (!ex_safepoint_helper(env)) return false; */
        ASM {
                /* R14: env */
                /* R15: &env->frame->tmpvar[0] */
                /* R31: saved LR */

                /* Arg1 R3 = env */
                /* mr r3, r14 */                IW(0x7873c37d);

                /* Call ex_safepoint_helper(). */
                /* lis  r12, f[31:16] */        IW(0x0000803d | hi16(f));
                /* ori  r12, r12, f[15:0] */    IW(0x00008c61 | lo16(f));
                /* mflr r31 */                  IW(0xa602e87f);
                /* mtctr r12 */                 IW(0xa603897d);
                /* bctrl */                     IW(0x2104804e);
                /* mtlr r31 */                  IW(0xa603e87f);

                /* If failed: */
                /* cmpwi r3, 0 */               IW(0x0000032c);
                EXCEPTION_IF_EQUAL();
        }

        return true;
}

/* Visit a OP_PBASE instruction. (ABCE; inline machine code, ppc32 BE.)
 * Pointer member at +8; the long stores it BE: high(0) at +8,
 * low(ptr) at +12. */
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
                /* r15: &env->frame->tmpvar[0] */

                /* lwz r3, src+8(r15)  (packed pointer member) */
                IW(0x00006f80 | lo16((uint32_t)(src + 8)));
                IW(0x00006380 | lo16(buf_ofs));
                IW(0x0000a038 | lo16((uint32_t)NOCT_VALUE_LONG));
                IW(0x0000af90 | lo16((uint32_t)dst));
                /* high word = 0 */
                IW(0x00008038);
                IW(0x00008f90 | lo16((uint32_t)(dst + 8)));
                /* low word = pointer */
                IW(0x00006f90 | lo16((uint32_t)(dst + 12)));
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

/* Visit a OP_PLOAD8U instruction. (ABCE; inline machine code, ppc32 BE.) */
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
                /* r15: &env->frame->tmpvar[0] */

                /* lwz r3, base+12(r15)  (BE-32: low word of the long) */
                IW(0x00006f80 | lo16((uint32_t)(base + 12)));
                /* lwz r4, ofs+8(r15) */
                IW(0x00008f80 | lo16((uint32_t)(ofs + 8)));
                IW(0x1422637c);
                /* load element -> r4 */
                IW(0x00008388);
                IW(0x0000a038 | lo16((uint32_t)NOCT_VALUE_INT));
                IW(0x0000af90 | lo16((uint32_t)dst));
                IW(0x00008f90 | lo16((uint32_t)(dst + 8)));
        }

        return true;
}

/* Visit a OP_PSTORE8 instruction. (ABCE; inline, ppc32 BE. Int source.) */
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
                /* r15: &env->frame->tmpvar[0] */

                IW(0x00006f80 | lo16((uint32_t)(base + 12)));
                IW(0x00008f80 | lo16((uint32_t)(ofs + 8)));
                IW(0x1422637c);
                IW(0x00008f80 | lo16((uint32_t)(src + 8)));
                IW(0x00008398);
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

/* Visit a OP_PLOAD8S instruction. (ABCE; inline machine code, ppc32 BE.) */
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
                /* r15: &env->frame->tmpvar[0] */

                /* lwz r3, base+12(r15)  (BE-32: low word of the long) */
                IW(0x00006f80 | lo16((uint32_t)(base + 12)));
                /* lwz r4, ofs+8(r15) */
                IW(0x00008f80 | lo16((uint32_t)(ofs + 8)));
                IW(0x1422637c);
                /* load element -> r4 */
                IW(0x00008388);
                IW(0x7407847c);
                IW(0x0000a038 | lo16((uint32_t)NOCT_VALUE_INT));
                IW(0x0000af90 | lo16((uint32_t)dst));
                IW(0x00008f90 | lo16((uint32_t)(dst + 8)));
        }

        return true;
}

/* Visit a OP_PLOAD16U instruction. (ABCE; inline machine code, ppc32 BE.) */
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
                /* r15: &env->frame->tmpvar[0] */

                /* lwz r3, base+12(r15)  (BE-32: low word of the long) */
                IW(0x00006f80 | lo16((uint32_t)(base + 12)));
                /* lwz r4, ofs+8(r15) */
                IW(0x00008f80 | lo16((uint32_t)(ofs + 8)));
                IW(0x1422847c);
                IW(0x1422637c);
                /* load element -> r4 */
                IW(0x000083a0);
                IW(0x0000a038 | lo16((uint32_t)NOCT_VALUE_INT));
                IW(0x0000af90 | lo16((uint32_t)dst));
                IW(0x00008f90 | lo16((uint32_t)(dst + 8)));
        }

        return true;
}

/* Visit a OP_PLOAD16S instruction. (ABCE; inline machine code, ppc32 BE.) */
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
                /* r15: &env->frame->tmpvar[0] */

                /* lwz r3, base+12(r15)  (BE-32: low word of the long) */
                IW(0x00006f80 | lo16((uint32_t)(base + 12)));
                /* lwz r4, ofs+8(r15) */
                IW(0x00008f80 | lo16((uint32_t)(ofs + 8)));
                IW(0x1422847c);
                IW(0x1422637c);
                /* load element -> r4 */
                IW(0x000083a8);
                IW(0x0000a038 | lo16((uint32_t)NOCT_VALUE_INT));
                IW(0x0000af90 | lo16((uint32_t)dst));
                IW(0x00008f90 | lo16((uint32_t)(dst + 8)));
        }

        return true;
}

/* Visit a OP_PLOAD32 instruction. (ABCE; inline machine code, ppc32 BE.) */
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
                /* r15: &env->frame->tmpvar[0] */

                /* lwz r3, base+12(r15)  (BE-32: low word of the long) */
                IW(0x00006f80 | lo16((uint32_t)(base + 12)));
                /* lwz r4, ofs+8(r15) */
                IW(0x00008f80 | lo16((uint32_t)(ofs + 8)));
                IW(0x1422847c);
                IW(0x1422847c);
                IW(0x1422637c);
                /* load element -> r4 */
                IW(0x00008380);
                IW(0x0000a038 | lo16((uint32_t)NOCT_VALUE_INT));
                IW(0x0000af90 | lo16((uint32_t)dst));
                IW(0x00008f90 | lo16((uint32_t)(dst + 8)));
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

/* Visit a OP_PSTORE16 instruction. (ABCE; inline, ppc32 BE. Int source.) */
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
                /* r15: &env->frame->tmpvar[0] */

                IW(0x00006f80 | lo16((uint32_t)(base + 12)));
                IW(0x00008f80 | lo16((uint32_t)(ofs + 8)));
                IW(0x1422847c);
                IW(0x1422637c);
                IW(0x00008f80 | lo16((uint32_t)(src + 8)));
                IW(0x000083b0);
        }

        return true;
}

/* Visit a OP_PSTORE32 instruction. (ABCE; inline, ppc32 BE. Int source.) */
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
                /* r15: &env->frame->tmpvar[0] */

                IW(0x00006f80 | lo16((uint32_t)(base + 12)));
                IW(0x00008f80 | lo16((uint32_t)(ofs + 8)));
                IW(0x1422847c);
                IW(0x1422847c);
                IW(0x1422637c);
                IW(0x00008f80 | lo16((uint32_t)(src + 8)));
                IW(0x00008390);
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

#if defined(NOCT_USE_OPTIMIZER)
/*
 * 128-bit vector ops: native AltiVec where available, with direct scalar
 * lowering over env->vreg for the remaining operations.
 */

/* Visit vector instructions with AltiVec or direct scalar lowering. */
/* Visit an OP_VLOADI32X4 instruction. */
static INLINE bool
jit_visit_vloadi32x4_op(
        struct rt_jit_context *ctx)
{
        uint32_t vreg_ofs;
        uint32_t vreg_hi;
        int vd;
        int base_tmp;
        int ofs_tmp;
        int lane;
        int base;
        int ofs;

        CONSUME_IMM8(vd);
        CONSUME_TMPVAR(base_tmp);
        CONSUME_TMPVAR(ofs_tmp);

        /* AltiVec lacks several required portable operations.  Keep its
         * register state coherent around direct scalar lowering. */
        if ((ctx->simd_caps & JIT_SIMD_CAP_ALTIVEC) != 0 &&
            !jit_put_altivec_sync(ctx, false))
                return false;

        vreg_ofs = (uint32_t)offsetof(struct rt_env, vreg);
        vreg_hi = (vreg_ofs + 0x8000) >> 16;
        IW(jit_ppc_iw(0x3cae0000 | (vreg_hi & 0xffff)));
        IW(jit_ppc_iw(0x38a50000 | (vreg_ofs & 0xffff)));

        base = base_tmp * (int)sizeof(struct rt_value);
        ofs = ofs_tmp * (int)sizeof(struct rt_value);
#if defined(NOCT_ARCH_BE)
        base += 12;
#else
        base += 8;
#endif
        /* lwz r3,base(r15); lwz r4,ofs+8(r15); index *= 4; add */
        IW(jit_ppc_iw(0x806f0000 | ((uint32_t)base & 0xffff)));
        IW(jit_ppc_iw(0x808f0000 | ((uint32_t)(ofs + 8) & 0xffff)));
        IW(jit_ppc_iw(0x7c842214));
        IW(jit_ppc_iw(0x7c842214));
        IW(jit_ppc_iw(0x7c632214));
        for (lane = 0; lane < 4; lane++) {
                IW(jit_ppc_iw(0x80c30000 | ((uint32_t)lane * 4)));
                IW(jit_ppc_iw(0x90c50000 |
                              ((uint32_t)vd * 16 + (uint32_t)lane * 4)));
        }

        if ((ctx->simd_caps & JIT_SIMD_CAP_ALTIVEC) != 0 &&
            !jit_put_altivec_sync(ctx, true))
                return false;

        return true;
}

/* Visit an OP_VSTOREI32X4 instruction. */
static INLINE bool
jit_visit_vstorei32x4_op(
        struct rt_jit_context *ctx)
{
        uint32_t vreg_ofs;
        uint32_t vreg_hi;
        int base_tmp;
        int ofs_tmp;
        int vs;
        int lane;
        int base;
        int ofs;

        CONSUME_TMPVAR(base_tmp);
        CONSUME_TMPVAR(ofs_tmp);
        CONSUME_IMM8(vs);

        /* AltiVec lacks several required portable operations.  Keep its
         * register state coherent around direct scalar lowering. */
        if ((ctx->simd_caps & JIT_SIMD_CAP_ALTIVEC) != 0 &&
            !jit_put_altivec_sync(ctx, false))
                return false;

        vreg_ofs = (uint32_t)offsetof(struct rt_env, vreg);
        vreg_hi = (vreg_ofs + 0x8000) >> 16;
        IW(jit_ppc_iw(0x3cae0000 | (vreg_hi & 0xffff)));
        IW(jit_ppc_iw(0x38a50000 | (vreg_ofs & 0xffff)));

        base = base_tmp * (int)sizeof(struct rt_value);
        ofs = ofs_tmp * (int)sizeof(struct rt_value);
#if defined(NOCT_ARCH_BE)
        base += 12;
#else
        base += 8;
#endif
        IW(jit_ppc_iw(0x806f0000 | ((uint32_t)base & 0xffff)));
        IW(jit_ppc_iw(0x808f0000 | ((uint32_t)(ofs + 8) & 0xffff)));
        IW(jit_ppc_iw(0x7c842214));
        IW(jit_ppc_iw(0x7c842214));
        IW(jit_ppc_iw(0x7c632214));
        for (lane = 0; lane < 4; lane++) {
                IW(jit_ppc_iw(0x80c50000 |
                              ((uint32_t)vs * 16 + (uint32_t)lane * 4)));
                IW(jit_ppc_iw(0x90c30000 | ((uint32_t)lane * 4)));
        }

        if ((ctx->simd_caps & JIT_SIMD_CAP_ALTIVEC) != 0 &&
            !jit_put_altivec_sync(ctx, true))
                return false;

        return true;
}

/* Visit an OP_VSPLATI32 instruction. */
static INLINE bool
jit_visit_vsplati32_op(
        struct rt_jit_context *ctx)
{
        uint32_t vreg_ofs;
        uint32_t vreg_hi;
        int vd;
        int src_tmp;
        int lane;
        int src;

        CONSUME_IMM8(vd);
        CONSUME_TMPVAR(src_tmp);

        /* AltiVec lacks several required portable operations.  Keep its
         * register state coherent around direct scalar lowering. */
        if ((ctx->simd_caps & JIT_SIMD_CAP_ALTIVEC) != 0 &&
            !jit_put_altivec_sync(ctx, false))
                return false;

        vreg_ofs = (uint32_t)offsetof(struct rt_env, vreg);
        vreg_hi = (vreg_ofs + 0x8000) >> 16;
        IW(jit_ppc_iw(0x3cae0000 | (vreg_hi & 0xffff)));
        IW(jit_ppc_iw(0x38a50000 | (vreg_ofs & 0xffff)));

        src = src_tmp * (int)sizeof(struct rt_value);
        IW(jit_ppc_iw(0x80cf0000 | ((uint32_t)(src + 8) & 0xffff)));
        for (lane = 0; lane < 4; lane++)
                IW(jit_ppc_iw(0x90c50000 |
                              ((uint32_t)vd * 16 + (uint32_t)lane * 4)));

        if ((ctx->simd_caps & JIT_SIMD_CAP_ALTIVEC) != 0 &&
            !jit_put_altivec_sync(ctx, true))
                return false;

        return true;
}

/* Visit an OP_VGETLANEI32 instruction. */
static INLINE bool
jit_visit_vgetlanei32_op(
        struct rt_jit_context *ctx)
{
        uint32_t vreg_ofs;
        uint32_t vreg_hi;
        int dst_tmp;
        int vs;
        int lane_index;
        int d;
        uint32_t tag;

        CONSUME_TMPVAR(dst_tmp);
        CONSUME_IMM8(vs);
        CONSUME_IMM8(lane_index);

        /* AltiVec lacks several required portable operations.  Keep its
         * register state coherent around direct scalar lowering. */
        if ((ctx->simd_caps & JIT_SIMD_CAP_ALTIVEC) != 0 &&
            !jit_put_altivec_sync(ctx, false))
                return false;

        vreg_ofs = (uint32_t)offsetof(struct rt_env, vreg);
        vreg_hi = (vreg_ofs + 0x8000) >> 16;
        IW(jit_ppc_iw(0x3cae0000 | (vreg_hi & 0xffff)));
        IW(jit_ppc_iw(0x38a50000 | (vreg_ofs & 0xffff)));

        d = dst_tmp * (int)sizeof(struct rt_value);
        tag = (uint32_t)(NOCT_VALUE_INT);
        IW(jit_ppc_iw(0x80c50000 |
                      ((uint32_t)vs * 16 + (uint32_t)lane_index * 4)));
        IW(jit_ppc_iw(0x38e00000 | tag)); /* li r7,tag */
        IW(jit_ppc_iw(0x90ef0000 | ((uint32_t)d & 0xffff)));
        IW(jit_ppc_iw(0x90cf0000 | ((uint32_t)(d + 8) & 0xffff)));

        if ((ctx->simd_caps & JIT_SIMD_CAP_ALTIVEC) != 0 &&
            !jit_put_altivec_sync(ctx, true))
                return false;

        return true;
}

/* Visit an OP_VMOV128 instruction. */
static INLINE bool
jit_visit_vmov128_op(
        struct rt_jit_context *ctx)
{
        uint32_t vreg_ofs;
        uint32_t vreg_hi;
        int vd;
        int vs;
        int lane;

        CONSUME_IMM8(vd);
        CONSUME_IMM8(vs);

        if ((ctx->simd_caps & JIT_SIMD_CAP_ALTIVEC) != 0) {
                uint32_t base;

                base = 0x10000484;
                IW(jit_ppc_vx(base, vd, vs, vs));
                return true;
        }

        vreg_ofs = (uint32_t)offsetof(struct rt_env, vreg);
        vreg_hi = (vreg_ofs + 0x8000) >> 16;
        IW(jit_ppc_iw(0x3cae0000 | (vreg_hi & 0xffff)));
        IW(jit_ppc_iw(0x38a50000 | (vreg_ofs & 0xffff)));

        for (lane = 0; lane < 4; lane++) {
                IW(jit_ppc_iw(0x80c50000 |
                              ((uint32_t)vs * 16 + (uint32_t)lane * 4)));
                IW(jit_ppc_iw(0x90c50000 |
                              ((uint32_t)vd * 16 + (uint32_t)lane * 4)));
        }
        return true;
}

/* Visit an OP_VADDI32X4 instruction. */
static INLINE bool
jit_visit_vaddi32x4_op(
        struct rt_jit_context *ctx)
{
        uint32_t vreg_ofs;
        uint32_t vreg_hi;
        int vd;
        int lhs;
        int rhs;
        int lane;

        CONSUME_IMM8(vd);
        CONSUME_IMM8(lhs);
        CONSUME_IMM8(rhs);

        if ((ctx->simd_caps & JIT_SIMD_CAP_ALTIVEC) != 0) {
                uint32_t base;

                base = 0x10000080;
                IW(jit_ppc_vx(base, vd, lhs, rhs));
                return true;
        }

        vreg_ofs = (uint32_t)offsetof(struct rt_env, vreg);
        vreg_hi = (vreg_ofs + 0x8000) >> 16;
        IW(jit_ppc_iw(0x3cae0000 | (vreg_hi & 0xffff)));
        IW(jit_ppc_iw(0x38a50000 | (vreg_ofs & 0xffff)));

        for (lane = 0; lane < 4; lane++) {
                uint32_t a;
                uint32_t b;
                uint32_t d;
                uint32_t word;

                a = (uint32_t)lhs * 16 + (uint32_t)lane * 4;
                b = (uint32_t)rhs * 16 + (uint32_t)lane * 4;
                d = (uint32_t)vd * 16 + (uint32_t)lane * 4;
                IW(jit_ppc_iw(0x80c50000 | a));
                IW(jit_ppc_iw(0x80e50000 | b));
                word = 0x7cc63a14;
                IW(jit_ppc_iw(word));
                IW(jit_ppc_iw(0x90c50000 | d));
        }
        return true;
}

/* Visit an OP_VSUBI32X4 instruction. */
static INLINE bool
jit_visit_vsubi32x4_op(
        struct rt_jit_context *ctx)
{
        uint32_t vreg_ofs;
        uint32_t vreg_hi;
        int vd;
        int lhs;
        int rhs;
        int lane;

        CONSUME_IMM8(vd);
        CONSUME_IMM8(lhs);
        CONSUME_IMM8(rhs);

        if ((ctx->simd_caps & JIT_SIMD_CAP_ALTIVEC) != 0) {
                uint32_t base;

                base = 0x10000480;
                IW(jit_ppc_vx(base, vd, lhs, rhs));
                return true;
        }

        vreg_ofs = (uint32_t)offsetof(struct rt_env, vreg);
        vreg_hi = (vreg_ofs + 0x8000) >> 16;
        IW(jit_ppc_iw(0x3cae0000 | (vreg_hi & 0xffff)));
        IW(jit_ppc_iw(0x38a50000 | (vreg_ofs & 0xffff)));

        for (lane = 0; lane < 4; lane++) {
                uint32_t a;
                uint32_t b;
                uint32_t d;
                uint32_t word;

                a = (uint32_t)lhs * 16 + (uint32_t)lane * 4;
                b = (uint32_t)rhs * 16 + (uint32_t)lane * 4;
                d = (uint32_t)vd * 16 + (uint32_t)lane * 4;
                IW(jit_ppc_iw(0x80c50000 | a));
                IW(jit_ppc_iw(0x80e50000 | b));
                word = 0x7cc73050;
                IW(jit_ppc_iw(word));
                IW(jit_ppc_iw(0x90c50000 | d));
        }
        return true;
}

/* Visit an OP_VMULI32X4 instruction. */
static INLINE bool
jit_visit_vmuli32x4_op(
        struct rt_jit_context *ctx)
{
        uint32_t vreg_ofs;
        uint32_t vreg_hi;
        int vd;
        int lhs;
        int rhs;
        int lane;

        CONSUME_IMM8(vd);
        CONSUME_IMM8(lhs);
        CONSUME_IMM8(rhs);

        /* AltiVec lacks several required portable operations.  Keep its
         * register state coherent around direct scalar lowering. */
        if ((ctx->simd_caps & JIT_SIMD_CAP_ALTIVEC) != 0 &&
            !jit_put_altivec_sync(ctx, false))
                return false;

        vreg_ofs = (uint32_t)offsetof(struct rt_env, vreg);
        vreg_hi = (vreg_ofs + 0x8000) >> 16;
        IW(jit_ppc_iw(0x3cae0000 | (vreg_hi & 0xffff)));
        IW(jit_ppc_iw(0x38a50000 | (vreg_ofs & 0xffff)));

        for (lane = 0; lane < 4; lane++) {
                uint32_t a;
                uint32_t b;
                uint32_t d;
                uint32_t word;

                a = (uint32_t)lhs * 16 + (uint32_t)lane * 4;
                b = (uint32_t)rhs * 16 + (uint32_t)lane * 4;
                d = (uint32_t)vd * 16 + (uint32_t)lane * 4;
                IW(jit_ppc_iw(0x80c50000 | a));
                IW(jit_ppc_iw(0x80e50000 | b));
                word = 0x7cc639d6;
                IW(jit_ppc_iw(word));
                IW(jit_ppc_iw(0x90c50000 | d));
        }

        if ((ctx->simd_caps & JIT_SIMD_CAP_ALTIVEC) != 0 &&
            !jit_put_altivec_sync(ctx, true))
                return false;

        return true;
}

/* Visit an OP_VAND128 instruction. */
static INLINE bool
jit_visit_vand128_op(
        struct rt_jit_context *ctx)
{
        uint32_t vreg_ofs;
        uint32_t vreg_hi;
        int vd;
        int lhs;
        int rhs;
        int lane;

        CONSUME_IMM8(vd);
        CONSUME_IMM8(lhs);
        CONSUME_IMM8(rhs);

        if ((ctx->simd_caps & JIT_SIMD_CAP_ALTIVEC) != 0) {
                uint32_t base;

                base = 0x10000404;
                IW(jit_ppc_vx(base, vd, lhs, rhs));
                return true;
        }

        vreg_ofs = (uint32_t)offsetof(struct rt_env, vreg);
        vreg_hi = (vreg_ofs + 0x8000) >> 16;
        IW(jit_ppc_iw(0x3cae0000 | (vreg_hi & 0xffff)));
        IW(jit_ppc_iw(0x38a50000 | (vreg_ofs & 0xffff)));

        for (lane = 0; lane < 4; lane++) {
                uint32_t a;
                uint32_t b;
                uint32_t d;
                uint32_t word;

                a = (uint32_t)lhs * 16 + (uint32_t)lane * 4;
                b = (uint32_t)rhs * 16 + (uint32_t)lane * 4;
                d = (uint32_t)vd * 16 + (uint32_t)lane * 4;
                IW(jit_ppc_iw(0x80c50000 | a));
                IW(jit_ppc_iw(0x80e50000 | b));
                word = 0x7cc63838;
                IW(jit_ppc_iw(word));
                IW(jit_ppc_iw(0x90c50000 | d));
        }
        return true;
}

/* Visit an OP_VOR128 instruction. */
static INLINE bool
jit_visit_vor128_op(
        struct rt_jit_context *ctx)
{
        uint32_t vreg_ofs;
        uint32_t vreg_hi;
        int vd;
        int lhs;
        int rhs;
        int lane;

        CONSUME_IMM8(vd);
        CONSUME_IMM8(lhs);
        CONSUME_IMM8(rhs);

        if ((ctx->simd_caps & JIT_SIMD_CAP_ALTIVEC) != 0) {
                uint32_t base;

                base = 0x10000484;
                IW(jit_ppc_vx(base, vd, lhs, rhs));
                return true;
        }

        vreg_ofs = (uint32_t)offsetof(struct rt_env, vreg);
        vreg_hi = (vreg_ofs + 0x8000) >> 16;
        IW(jit_ppc_iw(0x3cae0000 | (vreg_hi & 0xffff)));
        IW(jit_ppc_iw(0x38a50000 | (vreg_ofs & 0xffff)));

        for (lane = 0; lane < 4; lane++) {
                uint32_t a;
                uint32_t b;
                uint32_t d;
                uint32_t word;

                a = (uint32_t)lhs * 16 + (uint32_t)lane * 4;
                b = (uint32_t)rhs * 16 + (uint32_t)lane * 4;
                d = (uint32_t)vd * 16 + (uint32_t)lane * 4;
                IW(jit_ppc_iw(0x80c50000 | a));
                IW(jit_ppc_iw(0x80e50000 | b));
                word = 0x7cc63b78;
                IW(jit_ppc_iw(word));
                IW(jit_ppc_iw(0x90c50000 | d));
        }
        return true;
}

/* Visit an OP_VXOR128 instruction. */
static INLINE bool
jit_visit_vxor128_op(
        struct rt_jit_context *ctx)
{
        uint32_t vreg_ofs;
        uint32_t vreg_hi;
        int vd;
        int lhs;
        int rhs;
        int lane;

        CONSUME_IMM8(vd);
        CONSUME_IMM8(lhs);
        CONSUME_IMM8(rhs);

        if ((ctx->simd_caps & JIT_SIMD_CAP_ALTIVEC) != 0) {
                uint32_t base;

                base = 0x100004c4;
                IW(jit_ppc_vx(base, vd, lhs, rhs));
                return true;
        }

        vreg_ofs = (uint32_t)offsetof(struct rt_env, vreg);
        vreg_hi = (vreg_ofs + 0x8000) >> 16;
        IW(jit_ppc_iw(0x3cae0000 | (vreg_hi & 0xffff)));
        IW(jit_ppc_iw(0x38a50000 | (vreg_ofs & 0xffff)));

        for (lane = 0; lane < 4; lane++) {
                uint32_t a;
                uint32_t b;
                uint32_t d;
                uint32_t word;

                a = (uint32_t)lhs * 16 + (uint32_t)lane * 4;
                b = (uint32_t)rhs * 16 + (uint32_t)lane * 4;
                d = (uint32_t)vd * 16 + (uint32_t)lane * 4;
                IW(jit_ppc_iw(0x80c50000 | a));
                IW(jit_ppc_iw(0x80e50000 | b));
                word = 0x7cc63a78;
                IW(jit_ppc_iw(word));
                IW(jit_ppc_iw(0x90c50000 | d));
        }
        return true;
}

/* Visit an OP_VSHLI32X4 instruction. */
static INLINE bool
jit_visit_vshli32x4_op(
        struct rt_jit_context *ctx)
{
        uint32_t vreg_ofs;
        uint32_t vreg_hi;
        int vd;
        int vs;
        int shift;
        int lane;

        CONSUME_IMM8(vd);
        CONSUME_IMM8(vs);
        CONSUME_IMM8(shift);

        /* AltiVec lacks several required portable operations.  Keep its
         * register state coherent around direct scalar lowering. */
        if ((ctx->simd_caps & JIT_SIMD_CAP_ALTIVEC) != 0 &&
            !jit_put_altivec_sync(ctx, false))
                return false;

        vreg_ofs = (uint32_t)offsetof(struct rt_env, vreg);
        vreg_hi = (vreg_ofs + 0x8000) >> 16;
        IW(jit_ppc_iw(0x3cae0000 | (vreg_hi & 0xffff)));
        IW(jit_ppc_iw(0x38a50000 | (vreg_ofs & 0xffff)));

        IW(jit_ppc_iw(0x38e00000 | ((uint32_t)shift & 31u)));
        for (lane = 0; lane < 4; lane++) {
                uint32_t s;
                uint32_t d;

                s = (uint32_t)vs * 16 + (uint32_t)lane * 4;
                d = (uint32_t)vd * 16 + (uint32_t)lane * 4;
                IW(jit_ppc_iw(0x80c50000 | s));
                IW(jit_ppc_iw(0x7cc63830));
                IW(jit_ppc_iw(0x90c50000 | d));
        }

        if ((ctx->simd_caps & JIT_SIMD_CAP_ALTIVEC) != 0 &&
            !jit_put_altivec_sync(ctx, true))
                return false;

        return true;
}

/* Visit an OP_VSHRI32X4 instruction. */
static INLINE bool
jit_visit_vshri32x4_op(
        struct rt_jit_context *ctx)
{
        uint32_t vreg_ofs;
        uint32_t vreg_hi;
        int vd;
        int vs;
        int shift;
        int lane;

        CONSUME_IMM8(vd);
        CONSUME_IMM8(vs);
        CONSUME_IMM8(shift);

        /* AltiVec lacks several required portable operations.  Keep its
         * register state coherent around direct scalar lowering. */
        if ((ctx->simd_caps & JIT_SIMD_CAP_ALTIVEC) != 0 &&
            !jit_put_altivec_sync(ctx, false))
                return false;

        vreg_ofs = (uint32_t)offsetof(struct rt_env, vreg);
        vreg_hi = (vreg_ofs + 0x8000) >> 16;
        IW(jit_ppc_iw(0x3cae0000 | (vreg_hi & 0xffff)));
        IW(jit_ppc_iw(0x38a50000 | (vreg_ofs & 0xffff)));

        IW(jit_ppc_iw(0x38e00000 | ((uint32_t)shift & 31u)));
        for (lane = 0; lane < 4; lane++) {
                uint32_t s;
                uint32_t d;

                s = (uint32_t)vs * 16 + (uint32_t)lane * 4;
                d = (uint32_t)vd * 16 + (uint32_t)lane * 4;
                IW(jit_ppc_iw(0x80c50000 | s));
                IW(jit_ppc_iw(0x7cc63c30));
                IW(jit_ppc_iw(0x90c50000 | d));
        }

        if ((ctx->simd_caps & JIT_SIMD_CAP_ALTIVEC) != 0 &&
            !jit_put_altivec_sync(ctx, true))
                return false;

        return true;
}

/* Visit an OP_VLOADF32X4 instruction. */
static INLINE bool
jit_visit_vloadf32x4_op(
        struct rt_jit_context *ctx)
{
        uint32_t vreg_ofs;
        uint32_t vreg_hi;
        int vd;
        int base_tmp;
        int ofs_tmp;
        int lane;
        int base;
        int ofs;

        CONSUME_IMM8(vd);
        CONSUME_TMPVAR(base_tmp);
        CONSUME_TMPVAR(ofs_tmp);

        /* AltiVec lacks several required portable operations.  Keep its
         * register state coherent around direct scalar lowering. */
        if ((ctx->simd_caps & JIT_SIMD_CAP_ALTIVEC) != 0 &&
            !jit_put_altivec_sync(ctx, false))
                return false;

        vreg_ofs = (uint32_t)offsetof(struct rt_env, vreg);
        vreg_hi = (vreg_ofs + 0x8000) >> 16;
        IW(jit_ppc_iw(0x3cae0000 | (vreg_hi & 0xffff)));
        IW(jit_ppc_iw(0x38a50000 | (vreg_ofs & 0xffff)));

        base = base_tmp * (int)sizeof(struct rt_value);
        ofs = ofs_tmp * (int)sizeof(struct rt_value);
#if defined(NOCT_ARCH_BE)
        base += 12;
#else
        base += 8;
#endif
        /* lwz r3,base(r15); lwz r4,ofs+8(r15); index *= 4; add */
        IW(jit_ppc_iw(0x806f0000 | ((uint32_t)base & 0xffff)));
        IW(jit_ppc_iw(0x808f0000 | ((uint32_t)(ofs + 8) & 0xffff)));
        IW(jit_ppc_iw(0x7c842214));
        IW(jit_ppc_iw(0x7c842214));
        IW(jit_ppc_iw(0x7c632214));
        for (lane = 0; lane < 4; lane++) {
                IW(jit_ppc_iw(0x80c30000 | ((uint32_t)lane * 4)));
                IW(jit_ppc_iw(0x90c50000 |
                              ((uint32_t)vd * 16 + (uint32_t)lane * 4)));
        }

        if ((ctx->simd_caps & JIT_SIMD_CAP_ALTIVEC) != 0 &&
            !jit_put_altivec_sync(ctx, true))
                return false;

        return true;
}

/* Visit an OP_VSTOREF32X4 instruction. */
static INLINE bool
jit_visit_vstoref32x4_op(
        struct rt_jit_context *ctx)
{
        uint32_t vreg_ofs;
        uint32_t vreg_hi;
        int base_tmp;
        int ofs_tmp;
        int vs;
        int lane;
        int base;
        int ofs;

        CONSUME_TMPVAR(base_tmp);
        CONSUME_TMPVAR(ofs_tmp);
        CONSUME_IMM8(vs);

        /* AltiVec lacks several required portable operations.  Keep its
         * register state coherent around direct scalar lowering. */
        if ((ctx->simd_caps & JIT_SIMD_CAP_ALTIVEC) != 0 &&
            !jit_put_altivec_sync(ctx, false))
                return false;

        vreg_ofs = (uint32_t)offsetof(struct rt_env, vreg);
        vreg_hi = (vreg_ofs + 0x8000) >> 16;
        IW(jit_ppc_iw(0x3cae0000 | (vreg_hi & 0xffff)));
        IW(jit_ppc_iw(0x38a50000 | (vreg_ofs & 0xffff)));

        base = base_tmp * (int)sizeof(struct rt_value);
        ofs = ofs_tmp * (int)sizeof(struct rt_value);
#if defined(NOCT_ARCH_BE)
        base += 12;
#else
        base += 8;
#endif
        IW(jit_ppc_iw(0x806f0000 | ((uint32_t)base & 0xffff)));
        IW(jit_ppc_iw(0x808f0000 | ((uint32_t)(ofs + 8) & 0xffff)));
        IW(jit_ppc_iw(0x7c842214));
        IW(jit_ppc_iw(0x7c842214));
        IW(jit_ppc_iw(0x7c632214));
        for (lane = 0; lane < 4; lane++) {
                IW(jit_ppc_iw(0x80c50000 |
                              ((uint32_t)vs * 16 + (uint32_t)lane * 4)));
                IW(jit_ppc_iw(0x90c30000 | ((uint32_t)lane * 4)));
        }

        if ((ctx->simd_caps & JIT_SIMD_CAP_ALTIVEC) != 0 &&
            !jit_put_altivec_sync(ctx, true))
                return false;

        return true;
}

/* Visit an OP_VSPLATF32 instruction. */
static INLINE bool
jit_visit_vsplatf32_op(
        struct rt_jit_context *ctx)
{
        uint32_t vreg_ofs;
        uint32_t vreg_hi;
        int vd;
        int src_tmp;
        int lane;
        int src;

        CONSUME_IMM8(vd);
        CONSUME_TMPVAR(src_tmp);

        /* AltiVec lacks several required portable operations.  Keep its
         * register state coherent around direct scalar lowering. */
        if ((ctx->simd_caps & JIT_SIMD_CAP_ALTIVEC) != 0 &&
            !jit_put_altivec_sync(ctx, false))
                return false;

        vreg_ofs = (uint32_t)offsetof(struct rt_env, vreg);
        vreg_hi = (vreg_ofs + 0x8000) >> 16;
        IW(jit_ppc_iw(0x3cae0000 | (vreg_hi & 0xffff)));
        IW(jit_ppc_iw(0x38a50000 | (vreg_ofs & 0xffff)));

        src = src_tmp * (int)sizeof(struct rt_value);
        IW(jit_ppc_iw(0x80cf0000 | ((uint32_t)(src + 8) & 0xffff)));
        for (lane = 0; lane < 4; lane++)
                IW(jit_ppc_iw(0x90c50000 |
                              ((uint32_t)vd * 16 + (uint32_t)lane * 4)));

        if ((ctx->simd_caps & JIT_SIMD_CAP_ALTIVEC) != 0 &&
            !jit_put_altivec_sync(ctx, true))
                return false;

        return true;
}

/* Visit an OP_VGETLANEF32 instruction. */
static INLINE bool
jit_visit_vgetlanef32_op(
        struct rt_jit_context *ctx)
{
        uint32_t vreg_ofs;
        uint32_t vreg_hi;
        int dst_tmp;
        int vs;
        int lane_index;
        int d;
        uint32_t tag;

        CONSUME_TMPVAR(dst_tmp);
        CONSUME_IMM8(vs);
        CONSUME_IMM8(lane_index);

        /* AltiVec lacks several required portable operations.  Keep its
         * register state coherent around direct scalar lowering. */
        if ((ctx->simd_caps & JIT_SIMD_CAP_ALTIVEC) != 0 &&
            !jit_put_altivec_sync(ctx, false))
                return false;

        vreg_ofs = (uint32_t)offsetof(struct rt_env, vreg);
        vreg_hi = (vreg_ofs + 0x8000) >> 16;
        IW(jit_ppc_iw(0x3cae0000 | (vreg_hi & 0xffff)));
        IW(jit_ppc_iw(0x38a50000 | (vreg_ofs & 0xffff)));

        d = dst_tmp * (int)sizeof(struct rt_value);
        tag = (uint32_t)(NOCT_VALUE_FLOAT);
        IW(jit_ppc_iw(0x80c50000 |
                      ((uint32_t)vs * 16 + (uint32_t)lane_index * 4)));
        IW(jit_ppc_iw(0x38e00000 | tag)); /* li r7,tag */
        IW(jit_ppc_iw(0x90ef0000 | ((uint32_t)d & 0xffff)));
        IW(jit_ppc_iw(0x90cf0000 | ((uint32_t)(d + 8) & 0xffff)));

        if ((ctx->simd_caps & JIT_SIMD_CAP_ALTIVEC) != 0 &&
            !jit_put_altivec_sync(ctx, true))
                return false;

        return true;
}

/* Visit an OP_VADDF32X4 instruction. */
static INLINE bool
jit_visit_vaddf32x4_op(
        struct rt_jit_context *ctx)
{
        uint32_t vreg_ofs;
        uint32_t vreg_hi;
        int vd;
        int lhs;
        int rhs;
        int lane;

        CONSUME_IMM8(vd);
        CONSUME_IMM8(lhs);
        CONSUME_IMM8(rhs);

        if ((ctx->simd_caps & JIT_SIMD_CAP_ALTIVEC) != 0) {
                uint32_t base;

                base = 0x1000000a;
                IW(jit_ppc_vx(base, vd, lhs, rhs));
                return true;
        }

        vreg_ofs = (uint32_t)offsetof(struct rt_env, vreg);
        vreg_hi = (vreg_ofs + 0x8000) >> 16;
        IW(jit_ppc_iw(0x3cae0000 | (vreg_hi & 0xffff)));
        IW(jit_ppc_iw(0x38a50000 | (vreg_ofs & 0xffff)));

        for (lane = 0; lane < 4; lane++) {
                uint32_t a;
                uint32_t b;
                uint32_t d;
                uint32_t word;

                a = (uint32_t)lhs * 16 + (uint32_t)lane * 4;
                b = (uint32_t)rhs * 16 + (uint32_t)lane * 4;
                d = (uint32_t)vd * 16 + (uint32_t)lane * 4;
                IW(jit_ppc_iw(0xc0050000 | a)); /* lfs f0,a(r5) */
                IW(jit_ppc_iw(0xc0250000 | b)); /* lfs f1,b(r5) */
                word = 0xec40082a;
                IW(jit_ppc_iw(word));
                IW(jit_ppc_iw(0xd0450000 | d)); /* stfs f2,d(r5) */
        }
        return true;
}

/* Visit an OP_VSUBF32X4 instruction. */
static INLINE bool
jit_visit_vsubf32x4_op(
        struct rt_jit_context *ctx)
{
        uint32_t vreg_ofs;
        uint32_t vreg_hi;
        int vd;
        int lhs;
        int rhs;
        int lane;

        CONSUME_IMM8(vd);
        CONSUME_IMM8(lhs);
        CONSUME_IMM8(rhs);

        if ((ctx->simd_caps & JIT_SIMD_CAP_ALTIVEC) != 0) {
                uint32_t base;

                base = 0x1000004a;
                IW(jit_ppc_vx(base, vd, lhs, rhs));
                return true;
        }

        vreg_ofs = (uint32_t)offsetof(struct rt_env, vreg);
        vreg_hi = (vreg_ofs + 0x8000) >> 16;
        IW(jit_ppc_iw(0x3cae0000 | (vreg_hi & 0xffff)));
        IW(jit_ppc_iw(0x38a50000 | (vreg_ofs & 0xffff)));

        for (lane = 0; lane < 4; lane++) {
                uint32_t a;
                uint32_t b;
                uint32_t d;
                uint32_t word;

                a = (uint32_t)lhs * 16 + (uint32_t)lane * 4;
                b = (uint32_t)rhs * 16 + (uint32_t)lane * 4;
                d = (uint32_t)vd * 16 + (uint32_t)lane * 4;
                IW(jit_ppc_iw(0xc0050000 | a)); /* lfs f0,a(r5) */
                IW(jit_ppc_iw(0xc0250000 | b)); /* lfs f1,b(r5) */
                word = 0xec400828;
                IW(jit_ppc_iw(word));
                IW(jit_ppc_iw(0xd0450000 | d)); /* stfs f2,d(r5) */
        }
        return true;
}

/* Visit an OP_VMULF32X4 instruction. */
static INLINE bool
jit_visit_vmulf32x4_op(
        struct rt_jit_context *ctx)
{
        uint32_t vreg_ofs;
        uint32_t vreg_hi;
        int vd;
        int lhs;
        int rhs;
        int lane;

        CONSUME_IMM8(vd);
        CONSUME_IMM8(lhs);
        CONSUME_IMM8(rhs);

        if ((ctx->simd_caps & JIT_SIMD_CAP_ALTIVEC) != 0) {
                /* vxor v8,v8,v8; vmaddfp vd,va,vb,v8 (va * vb + 0). */
                IW(jit_ppc_vx(0x100004c4, 8, 8, 8));
                IW(jit_ppc_iw(0x1000002e | ((uint32_t)vd << 21) |
                              ((uint32_t)lhs << 16) |
                              ((uint32_t)8 << 11) |
                              ((uint32_t)rhs << 6)));
                return true;
        }

        vreg_ofs = (uint32_t)offsetof(struct rt_env, vreg);
        vreg_hi = (vreg_ofs + 0x8000) >> 16;
        IW(jit_ppc_iw(0x3cae0000 | (vreg_hi & 0xffff)));
        IW(jit_ppc_iw(0x38a50000 | (vreg_ofs & 0xffff)));

        for (lane = 0; lane < 4; lane++) {
                uint32_t a;
                uint32_t b;
                uint32_t d;
                uint32_t word;

                a = (uint32_t)lhs * 16 + (uint32_t)lane * 4;
                b = (uint32_t)rhs * 16 + (uint32_t)lane * 4;
                d = (uint32_t)vd * 16 + (uint32_t)lane * 4;
                IW(jit_ppc_iw(0xc0050000 | a)); /* lfs f0,a(r5) */
                IW(jit_ppc_iw(0xc0250000 | b)); /* lfs f1,b(r5) */
                word = 0xec400072;
                IW(jit_ppc_iw(word));
                IW(jit_ppc_iw(0xd0450000 | d)); /* stfs f2,d(r5) */
        }
        return true;
}

/* Visit an OP_VDIVF32X4 instruction. */
static INLINE bool
jit_visit_vdivf32x4_op(
        struct rt_jit_context *ctx)
{
        uint32_t vreg_ofs;
        uint32_t vreg_hi;
        int vd;
        int lhs;
        int rhs;
        int lane;

        CONSUME_IMM8(vd);
        CONSUME_IMM8(lhs);
        CONSUME_IMM8(rhs);

        /* AltiVec lacks several required portable operations.  Keep its
         * register state coherent around direct scalar lowering. */
        if ((ctx->simd_caps & JIT_SIMD_CAP_ALTIVEC) != 0 &&
            !jit_put_altivec_sync(ctx, false))
                return false;

        vreg_ofs = (uint32_t)offsetof(struct rt_env, vreg);
        vreg_hi = (vreg_ofs + 0x8000) >> 16;
        IW(jit_ppc_iw(0x3cae0000 | (vreg_hi & 0xffff)));
        IW(jit_ppc_iw(0x38a50000 | (vreg_ofs & 0xffff)));

        for (lane = 0; lane < 4; lane++) {
                uint32_t a;
                uint32_t b;
                uint32_t d;
                uint32_t word;

                a = (uint32_t)lhs * 16 + (uint32_t)lane * 4;
                b = (uint32_t)rhs * 16 + (uint32_t)lane * 4;
                d = (uint32_t)vd * 16 + (uint32_t)lane * 4;
                IW(jit_ppc_iw(0xc0050000 | a)); /* lfs f0,a(r5) */
                IW(jit_ppc_iw(0xc0250000 | b)); /* lfs f1,b(r5) */
                word = 0xec400824;
                IW(jit_ppc_iw(word));
                IW(jit_ppc_iw(0xd0450000 | d)); /* stfs f2,d(r5) */
        }

        if ((ctx->simd_caps & JIT_SIMD_CAP_ALTIVEC) != 0 &&
            !jit_put_altivec_sync(ctx, true))
                return false;

        return true;
}

/* Visit an OP_VCVTI32F32X4 instruction. */
static INLINE bool
jit_visit_vcvti32f32x4_op(
        struct rt_jit_context *ctx)
{
        uint32_t vreg_ofs;
        uint32_t vreg_hi;
        int vd;
        int vs;
        int dst;
        int src1;
        int src2;

        CONSUME_IMM8(vd);
        CONSUME_IMM8(vs);

        if ((ctx->simd_caps & JIT_SIMD_CAP_ALTIVEC) != 0) {
                IW(jit_ppc_iw(0x1000034a | ((uint32_t)vd << 21) |
                              ((uint32_t)vs << 11)));
                return true;
        }

        dst = vd;
        src1 = vs;
        src2 = 0;

        vreg_ofs = (uint32_t)offsetof(struct rt_env, vreg);
        vreg_hi = (vreg_ofs + 0x8000) >> 16;
        IW(jit_ppc_iw(0x3cae0000 | (vreg_hi & 0xffff)));
        IW(jit_ppc_iw(0x38a50000 | (vreg_ofs & 0xffff)));

        ASM_BINARY_OP(noct_ex_vcvti32f32x4_helper);
        return true;
}

/* Visit an OP_VCVTF32I32X4 instruction. */
static INLINE bool
jit_visit_vcvtf32i32x4_op(
        struct rt_jit_context *ctx)
{
        uint32_t vreg_ofs;
        uint32_t vreg_hi;
        int vd;
        int vs;
        int dst;
        int src1;
        int src2;

        CONSUME_IMM8(vd);
        CONSUME_IMM8(vs);

        if ((ctx->simd_caps & JIT_SIMD_CAP_ALTIVEC) != 0) {
                IW(jit_ppc_iw(0x100003ca | ((uint32_t)vd << 21) |
                              ((uint32_t)vs << 11)));
                return true;
        }

        dst = vd;
        src1 = vs;
        src2 = 0;

        vreg_ofs = (uint32_t)offsetof(struct rt_env, vreg);
        vreg_hi = (vreg_ofs + 0x8000) >> 16;
        IW(jit_ppc_iw(0x3cae0000 | (vreg_hi & 0xffff)));
        IW(jit_ppc_iw(0x38a50000 | (vreg_ofs & 0xffff)));

        ASM_BINARY_OP(noct_ex_vcvtf32i32x4_helper);
        return true;
}

/* Visit an OP_VMINS32X4 instruction. */
static INLINE bool
jit_visit_vmins32x4_op(
        struct rt_jit_context *ctx)
{
        uint32_t vreg_ofs;
        uint32_t vreg_hi;
        int vd;
        int lhs;
        int rhs;
        int dst;
        int src1;
        int src2;

        CONSUME_IMM8(vd);
        CONSUME_IMM8(lhs);
        CONSUME_IMM8(rhs);

        if ((ctx->simd_caps & JIT_SIMD_CAP_ALTIVEC) != 0) {
                uint32_t base;

                base = 0x10000382;
                IW(jit_ppc_vx(base, vd, lhs, rhs));
                return true;
        }

        dst = vd;
        src1 = lhs;
        src2 = rhs;

        vreg_ofs = (uint32_t)offsetof(struct rt_env, vreg);
        vreg_hi = (vreg_ofs + 0x8000) >> 16;
        IW(jit_ppc_iw(0x3cae0000 | (vreg_hi & 0xffff)));
        IW(jit_ppc_iw(0x38a50000 | (vreg_ofs & 0xffff)));

        ASM_BINARY_OP(noct_ex_vmins32x4_helper);
        return true;
}

/* Visit an OP_VMAXS32X4 instruction. */
static INLINE bool
jit_visit_vmaxs32x4_op(
        struct rt_jit_context *ctx)
{
        uint32_t vreg_ofs;
        uint32_t vreg_hi;
        int vd;
        int lhs;
        int rhs;
        int dst;
        int src1;
        int src2;

        CONSUME_IMM8(vd);
        CONSUME_IMM8(lhs);
        CONSUME_IMM8(rhs);

        if ((ctx->simd_caps & JIT_SIMD_CAP_ALTIVEC) != 0) {
                uint32_t base;

                base = 0x10000182;
                IW(jit_ppc_vx(base, vd, lhs, rhs));
                return true;
        }

        dst = vd;
        src1 = lhs;
        src2 = rhs;

        vreg_ofs = (uint32_t)offsetof(struct rt_env, vreg);
        vreg_hi = (vreg_ofs + 0x8000) >> 16;
        IW(jit_ppc_iw(0x3cae0000 | (vreg_hi & 0xffff)));
        IW(jit_ppc_iw(0x38a50000 | (vreg_ofs & 0xffff)));

        ASM_BINARY_OP(noct_ex_vmaxs32x4_helper);
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
                /* R14: env */
                /* R15: &env->frame->tmpvar[0] */
                /* R31: saved LR */

                /* Push the general-purpose registers. */
                /* stw r14, -8(r1) */           IW(0xf8ffc191);
                /* stw r15, -16(r1) */          IW(0xf0ffe191);
                /* stw r31, -24(r1) */          IW(0xe8ffe193);
                /* addi r1, r1, -64 */          IW(0xc0ff2138);

                /* R14 = env */
                /* mr r14, r3 */                IW(0x781b6e7c);

                /* R15 = *env->frame = &env->frame->tmpvar[0] */
                /* lwz r15, 0(r14) */           IW(0x0000ee81);
                /* lwz r15, 0(r15) */           IW(0x0000ef81);

                /* Skip an exception handler. */
                /* b body */                    IW(0x1c000048);
        }

        /* Put an exception handler. */
        ctx->exception_code = ctx->code;
        ASM {
        /* EXCEPTION: */
                /* addi r1, r1, 64 */           IW(0x40002138);
                /* lwz r31, -24(r1) */          IW(0xe8ffe183);
                /* lwz r15, -16(r1) */          IW(0xf0ffe181);
                /* lwz r14, -8(r1) */           IW(0xf8ffc181);
                /* li r3, 0 */                  IW(0x00006038);
                /* blr */                       IW(0x2000804e);
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
                /* addi r1, r1, 64 */           IW(0x40002138);
                /* lwz r31, -24(r1) */          IW(0xe8ffe183);
                /* lwz r15, -16(r1) */          IW(0xf0ffe181);
                /* lwz r14, -8(r1) */           IW(0xf8ffc181);
                /* li r3, 1 */                  IW(0x01006038);
                /* blr */                       IW(0x2000804e);
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

        /* Assemble. */
        if (ctx->branch_patch[patch_index].type == PATCH_BAL) {
                if (offset < -33554432 || offset > 33554428) {
                        rt_error(ctx->env, N_TR("Branch target too far."));
                        return false;
                }

                ASM {
                        /* b offset */
                        IW(0x00000048 |
                           (((uint32_t)offset & 0xff) << 24) |
                           ((((uint32_t)offset >> 8) & 0xff) << 16) |
                           ((((uint32_t)offset >> 16) & 0xff) << 8) |
                           (((uint32_t)offset >> 24) & 0x03));
                }
        } else if (ctx->branch_patch[patch_index].type == PATCH_BEQ) {
                if (getenv("NOCT_JIT_FORCE_LONG_BRANCH") == NULL &&
                    offset >= -32768 && offset <= 32764) {
                        ASM {
                                /* beq offset; nop */
                                IW(0x00008241 |
                                   (((uint32_t)offset & 0xff) << 24) |
                                   ((((uint32_t)offset >> 8) & 0xff) << 16));
                                IW(0x00000060);
                        }
                } else {
                        int long_offset = offset - 4;

                        ASM {
                                /* bne +8; b long_offset */
                                IW(0x08008240);
                                IW(0x00000048 |
                                   (((uint32_t)long_offset & 0xff) << 24) |
                                   ((((uint32_t)long_offset >> 8) & 0xff) << 16) |
                                   ((((uint32_t)long_offset >> 16) & 0xff) << 8) |
                                   (((uint32_t)long_offset >> 24) & 0x03));
                        }
                }
        } else if (ctx->branch_patch[patch_index].type == PATCH_BNE) {
                if (getenv("NOCT_JIT_FORCE_LONG_BRANCH") == NULL &&
                    offset >= -32768 && offset <= 32764) {
                        ASM {
                                /* bne offset; nop */
                                IW(0x00008240 |
                                   (((uint32_t)offset & 0xff) << 24) |
                                   ((((uint32_t)offset >> 8) & 0xff) << 16));
                                IW(0x00000060);
                        }
                } else {
                        int long_offset = offset - 4;

                        ASM {
                                /* beq +8; b long_offset */
                                IW(0x08008241);
                                IW(0x00000048 |
                                   (((uint32_t)long_offset & 0xff) << 24) |
                                   ((((uint32_t)long_offset >> 8) & 0xff) << 16) |
                                   ((((uint32_t)long_offset >> 16) & 0xff) << 8) |
                                   (((uint32_t)long_offset >> 24) & 0x03));
                        }
                }
        }

        return true;
}

static uint32_t
jit_detect_simd_caps(void)
{
#if defined(__linux__) && defined(PPC_FEATURE_HAS_ALTIVEC)
	if ((getauxval(AT_HWCAP) & PPC_FEATURE_HAS_ALTIVEC) != 0)
		return JIT_SIMD_CAP_ALTIVEC;
#endif
	return 0;
}

#endif /* defined(NOCT_ARCH_PPC32) && defined(NOCT_USE_JIT) */
