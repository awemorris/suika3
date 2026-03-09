/* -*- coding: utf-8; tab-width: 8; indent-tabs-mode: nil; -*- */

/*
 * Copyright (c) 2025, Awe Morris. All rights reserved.
 */

/*
 * JIT (x86_64): Just-In-Time native code generation
 */

#include <noct/c89compat.h>        /* NOCT_ARCH_X86_64 */

#if defined(NOCT_ARCH_X86_64) && defined(NOCT_USE_JIT)

#include "runtime.h"
#include "jit.h"
#include "execution.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

/*
 * ABI Check (MS ABI or SYSV ABI)
 */
#define IS_MSABI                (sizeof(long) == 4)

/* False asseretion */
#define JIT_OP_NOT_IMPLEMENTED  0
#define NEVER_COME_HERE         0

/* PC entry size. */
#define PC_ENTRY_MAX            2048

/* Branch pathch size. */
#define BRANCH_PATCH_MAX        2048

/* Branch patch type */
#define PATCH_JMP               0
#define PATCH_JE                1
#define PATCH_JNE               2

/* Generated code. */
static uint8_t *jit_code_region;
static uint8_t *jit_code_region_cur;
static uint8_t *jit_code_region_tail;

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
                jit_code_region_tail = jit_code_region + JIT_CODE_MAX;
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
        if (!is_writable)
                jit_map_writable(jit_code_region, JIT_CODE_MAX);

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

/* Serif */
#define ASM

/* Put a instruction byte. */
#define IB(b)                        if (!jit_put_byte(ctx, b)) return false
static INLINE bool
jit_put_byte(
        struct jit_context *ctx,
        uint8_t b)
{
        if ((uint8_t *)ctx->code + 1 > (uint8_t *)ctx->code_end) {
                rt_error(ctx->env, "Code too big.");
                return false;
        }

        *(uint8_t *)ctx->code = b;
        ctx->code = (uint8_t *)ctx->code + 1;

        return true;
}

/* Put a instruction double word. */
#define ID(d)                        if (!jit_put_dword(ctx, d)) return false
static INLINE bool
jit_put_dword(
        struct jit_context *ctx,
        uint32_t dw)
{
        if ((uint8_t *)ctx->code + 4 > (uint8_t *)ctx->code_end) {
                rt_error(ctx->env, "Code too big.");
                return false;
        }

        *(uint8_t *)ctx->code = (uint8_t)(dw & 0xff);
        ctx->code = (uint8_t *)ctx->code + 1;
        *(uint8_t *)ctx->code = (uint8_t)((dw >> 8) & 0xff);
        ctx->code = (uint8_t *)ctx->code + 1;
        *(uint8_t *)ctx->code = (uint8_t)((dw >> 16) & 0xff);
        ctx->code = (uint8_t *)ctx->code + 1;
        *(uint8_t *)ctx->code = (uint8_t)((dw >> 24) & 0xff);
        ctx->code = (uint8_t *)ctx->code + 1;

        return true;
}

/* Put a instruction word. */
#define IQ(q)                        if (!jit_put_qword(ctx, q)) return false
static INLINE bool
jit_put_qword(
        struct jit_context *ctx,
        uint64_t qw)
{
        if ((uint8_t *)ctx->code + 8 > (uint8_t *)ctx->code_end) {
                rt_error(ctx->env, "Code too big.");
                return false;
        }

        *(uint8_t *)ctx->code = (uint8_t)(qw & 0xff);
        ctx->code = (uint8_t *)ctx->code + 1;
        *(uint8_t *)ctx->code = (uint8_t)((qw >> 8) & 0xff);
        ctx->code = (uint8_t *)ctx->code + 1;
        *(uint8_t *)ctx->code = (uint8_t)((qw >> 16) & 0xff);
        ctx->code = (uint8_t *)ctx->code + 1;
        *(uint8_t *)ctx->code = (uint8_t)((qw >> 24) & 0xff);
        ctx->code = (uint8_t *)ctx->code + 1;
        *(uint8_t *)ctx->code = (uint8_t)((qw >> 32) & 0xff);
        ctx->code = (uint8_t *)ctx->code + 1;
        *(uint8_t *)ctx->code = (uint8_t)((qw >> 40) & 0xff);
        ctx->code = (uint8_t *)ctx->code + 1;
        *(uint8_t *)ctx->code = (uint8_t)((qw >> 48) & 0xff);
        ctx->code = (uint8_t *)ctx->code + 1;
        *(uint8_t *)ctx->code = (uint8_t)((qw >> 56) & 0xff);
        ctx->code = (uint8_t *)ctx->code + 1;

        return true;
}

/*
 * Templates
 */

#define ASM_BINARY_OP(f)                                                                                            \
    if (IS_MSABI) {                                                                                                 \
        /* if (!f(env, dst, src1, src2)) return false; */                                                           \
        ASM {                                                                                                       \
            /* r13: exception_handler */                                                                            \
            /* r14: env */                                                                                          \
            /* r15: &env->frame->tmpvar[0] */                                                                       \
                                                                                                                    \
            /* subq %rsp, 32 */              IB(0x48); IB(0x83); IB(0xec); IB(0x20);                                \
            /* (1st) movq %r14 -> %rcx */    IB(0x4c); IB(0x89); IB(0xf1);                                          \
            /* (2nd) movq dst -> %rdx */     IB(0x48); IB(0xc7); IB(0xc2); ID((uint32_t)dst);                       \
            /* (3rd) movq src1 -> %r8 */     IB(0x49); IB(0xb8); IQ((uint64_t)src1);                                \
            /* (4th) movq src2 -> %r9 */     IB(0x49); IB(0xb9); IQ((uint64_t)src2);                                \
            /* movabs f -> %rax */           IB(0x48); IB(0xb8); IQ((uint64_t)f);                                   \
            /* call *%rax */                 IB(0xff); IB(0xd0);                                                    \
            /* addq %rsp, 32 */              IB(0x48); IB(0x83); IB(0xc4); IB(0x20);                                \
                                                                                                                    \
            /* testl %eax, %eax */           IB(0x83); IB(0xF8); IB(0x00);                                          \
            /* jne 8 <next> */               IB(0x75); IB(0x03);                                                    \
            /* jmp *%r13 */                  IB(0x41); IB(0xFF); IB(0xE5);                                          \
        /* next: */                                                                                                 \
        }                                                                                                           \
    } else {                                                                                                        \
        /* if (!f(env, dst, src1, src2)) return false; */                                                           \
        ASM {                                                                                                       \
            /* r13: exception_handler */                                                                            \
            /* r14: env */                                                                                          \
            /* r15: &env->frame->tmpvar[0] */                                                                       \
                                                                                                                    \
            /* (1st) movq %r14 -> %rdi */    IB(0x4c); IB(0x89); IB(0xf7);                                          \
            /* (2st) movq dst -> %rsi */     IB(0x48); IB(0xc7); IB(0xc6); ID((uint32_t)dst);                       \
            /* (3rd) movq src1 -> %rdx */    IB(0x48); IB(0xc7); IB(0xc2); ID((uint32_t)src1);                      \
            /* (4th) movq src2 -> %rcx */    IB(0x48); IB(0xc7); IB(0xc1); ID((uint32_t)src2);                      \
            /* movabs f -> %rax */           IB(0x48); IB(0xb8); IQ((uint64_t)f);                                   \
            /* call *%rax */                 IB(0xff); IB(0xd0);                                                    \
                                                                                                                    \
            /* testl %eax, %eax */           IB(0x83); IB(0xF8); IB(0x00);                                          \
            /* jne 8 <next> */               IB(0x75); IB(0x03);                                                    \
            /* jmp *%r13 */                  IB(0x41); IB(0xff); IB(0xe5);                                          \
            /* next: */                                                                                             \
        }                                                                                                           \
    }

#define ASM_UNARY_OP(f)                                                                                             \
    if (IS_MSABI) {                                                                                                 \
        /* if (!f(env, dst, src)) return false; */                                                                  \
        ASM {                                                                                                       \
            /* r13: exception_handler */                                                                            \
            /* r14: env */                                                                                          \
            /* r15: &env->frame->tmpvar[0] */                                                                       \
                                                                                                                    \
            /* subq %rsp, 32 */              IB(0x48); IB(0x83); IB(0xec); IB(0x20);                                \
            /* (1st) mov %r14 -> %rcx */     IB(0x4c); IB(0x89); IB(0xf1);                                          \
            /* (2nd) mov dst -> %rdx */      IB(0x48); IB(0xc7); IB(0xc2); ID((uint32_t)dst);                       \
            /* (3rd) mov src -> %r8 */       IB(0x49); IB(0xb8); IQ((uint64_t)src);                                 \
            /* movabs f -> %rax */           IB(0x48); IB(0xb8); IQ((uint64_t)f);                                   \
            /* call *%rax */                 IB(0xff); IB(0xd0);                                                    \
            /* addq %rsp, 32 */              IB(0x48); IB(0x83); IB(0xc4); IB(0x20);                                \
                                                                                                                    \
            /* testl %eax, %eax */           IB(0x83); IB(0xf8); IB(0x00);                                          \
            /* jne 8 <next> */               IB(0x75); IB(0x03);                                                    \
            /* jmp *%r13 */                  IB(0x41); IB(0xff); IB(0xe5);                                          \
            /* next:*/                                                                                              \
        /* next: */                                                                                                 \
        }                                                                                                           \
    } else {                                                                                                        \
        /* if (!f(env, dst, src)) return false; */                                                                  \
        ASM {                                                                                                       \
            /* r13: exception_handler */                                                                            \
            /* r14: env */                                                                                          \
            /* r15: &env->frame->tmpvar[0] */                                                                       \
                                                                                                                    \
            /* (1st) movq %r14 -> %rdi */    IB(0x4c); IB(0x89); IB(0xf7);                                          \
            /* (2nd) movq dst -> %rsi */     IB(0x48); IB(0xc7); IB(0xc6); ID((uint32_t)dst);                       \
            /* (3rd) movq src -> %rdx */     IB(0x48); IB(0xc7); IB(0xc2); ID((uint32_t)src);                       \
            /* movabs f -> %r8 */            IB(0x49); IB(0xb8); IQ((uint64_t)f);                                   \
            /* call *%r8 */                  IB(0x41); IB(0xff); IB(0xd0);                                          \
                                                                                                                    \
            /* testl %eax, %eax */           IB(0x83); IB(0xf8); IB(0x00);                                          \
            /* jne 8 <next> */               IB(0x75); IB(0x03);                                                    \
            /* jmp *%r13 */                  IB(0x41); IB(0xff); IB(0xe5);                                          \
        /* next:*/                                                                                                  \
        }                                                                                                           \
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

        /* env->line = line; */
        ASM {
                /* r13: exception_handler */
                /* r14: evn */
                /* r15: &env->frame->tmpvar[0] */

                /* movl line -> %rax */              IB(0x48); IB(0xc7); IB(0xc0); ID(line);
                /* movq %rax -> [%r14 + 8] */        IB(0x49); IB(0x89); IB(0x46); IB(0x08);
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
                /* r13: exception_handler */
                /* r14: env */
                /* r15: &env->frame->tmpvar[0] */

                /* movq $dst -> %rax */           IB(0x48); IB(0xc7); IB(0xc0); ID((uint32_t)dst);
                /* movq $src -> %rbx */           IB(0x48); IB(0xc7); IB(0xc3); ID((uint32_t)src);
                /* addq %rax -> %r15  */          IB(0x4c); IB(0x01); IB(0xf8);
                /* addq %rbx -> %r15  */          IB(0x4c); IB(0x01); IB(0xfb);
                /* movq (%rbx) -> %rcx */         IB(0x48); IB(0x8b); IB(0x0b);
                /* movq 8(%rbx) -> %rdx */        IB(0x48); IB(0x8b); IB(0x53); IB(0x08);
                /* movq %rcx -> (%rax) */         IB(0x48); IB(0x89); IB(0x08);
                /* movq %rdx -> 8(%rax) */        IB(0x48); IB(0x89); IB(0x50); IB(0x08);
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

        /* &env->frame->tmpvar[dst].type = RT_VALUE_INT; */
        /* &env->frame->tmpvar[dst].val.i = val; */
        ASM {
                /* r13: exception_handler */
                /* r14: env */
                /* r15: &env->frame->tmpvar[0] */

                /* movq $dst -> %rax */            IB(0x48); IB(0xc7); IB(0xc0); ID((uint32_t)dst);
                /* addq %r15 -> %rax */            IB(0x4c); IB(0x01); IB(0xf8);
                /* movl $0 -> (%rax) */            IB(0xc7); IB(0x00); ID(0);
                /* movl $val ->8(%rax) */          IB(0xc7); IB(0x40); IB(0x08); ID((uint32_t)val);
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

        /* &env->frame->tmpvar[dst].type = RT_VALUE_INT; */
        /* &env->frame->tmpvar[dst].val.i = val; */
        ASM {
                /* r13: exception_handler */
                /* r14: env */
                /* r15: &env->frame->tmpvar[0] */

                /* movq $dst -> %rax */             IB(0x48); IB(0xc7); IB(0xc0); ID((uint32_t)dst);
                /* addq %r15 -> %rax */             IB(0x4c); IB(0x01); IB(0xf8);
                /* movl $1 -> (%rax) */             IB(0xc7); IB(0x00); ID(1);
                /* movl $val -> 8(%rax) */          IB(0xc7); IB(0x40); IB(0x08); ID(val);
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

        if (IS_MSABI) {
                /* rt_make_string_with_hash(env, &env->frame->tmpvar[dst], val, len, hash); */
                ASM {
                        /* r13: exception_handler */
                        /* r14: env */
                        /* r15: &env->frame->tmpvar[0] */

                        /* subq %rsp, $64 */                           IB(0x48); IB(0x83); IB(0xec); IB(0x40);
                        /* (1st) movq %r14 -> %rcx */                  IB(0x4c); IB(0x89); IB(0xf1);
                        /* (2nd) movq $dst -> %rdx */                  IB(0x48); IB(0xc7); IB(0xc2); ID((uint32_t)dst);
                        /*       addq %r15 -> %rdx */                  IB(0x4c); IB(0x01); IB(0xfa);
                        /* (3rd) movabs $val -> %r8 */                 IB(0x49); IB(0xb8); IQ((uint64_t)val);
                        /* (4th) movq $len -> %r9 */                   IB(0x49); IB(0xc7); IB(0xc1); ID((uint32_t)len);
                        /* (5th) movq $hash -> 32(%rsp) */             IB(0x48); IB(0xc7); IB(0x44); IB(0x24); IB(0x20); ID((uint32_t)hash);
                        /* movabs rt_make_string_with_hash -> %rax */  IB(0x48); IB(0xb8); IQ((uint64_t)rt_make_string_with_hash);
                        /* call *%rax */                               IB(0xff); IB(0xd0);
                        /* addq %rsp, $64 */                           IB(0x48); IB(0x83); IB(0xc4); IB(0x40);

                        /* testl %eax, %eax */                         IB(0x83); IB(0xf8); IB(0x00);
                        /* jne 8 <next> */                             IB(0x75); IB(0x03);
                        /* jmp *%r13 */                                IB(0x41); IB(0xff); IB(0xe5);
                /* next: */
                }
        } else {
                /* rt_make_string_with_hash(env, &env->frame->tmpvar[dst], val, len, hash); */
                ASM {
                        /* r13 = exception_handler */
                        /* r14 = env */
                        /* r15 = &env->frame->tmpvar[0] */

                        /* (1st) movq %r14, %rdi */                    IB(0x4c); IB(0x89); IB(0xf7);
                        /* (2nd) movq $dst, %rsi */                    IB(0x48); IB(0xc7); IB(0xc6); ID((uint32_t)dst);
                        /*       addq %r15, %rsi */                    IB(0x4c); IB(0x01); IB(0xfe);
                        /* (3rd) movabs $val, %rdx */                  IB(0x48); IB(0xba); IQ((uint64_t)val);
                        /* (4th) movq $len -> %rcx */                  IB(0x48); IB(0xc7); IB(0xc1); ID((uint32_t)len);
                        /* (5th) movq $hash -> %r8 */                  IB(0x49); IB(0xc7); IB(0xc0); ID((uint32_t)hash);
                        /* movabs rt_make_string_with_hash -> %rax */  IB(0x48); IB(0xb8); IQ((uint64_t)rt_make_string_with_hash);
                        /* call *%rax */                               IB(0xff); IB(0xd0);

                        /* testl %eax, %eax */                         IB(0x83); IB(0xf8); IB(0x00);
                        /* jne 8 <next> */                             IB(0x75); IB(0x03);
                        /* jmp *%r13 */                                IB(0x41); IB(0xff); IB(0xe5);
                /* next: */
                }
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

        if (IS_MSABI) {
                /* rt_make_empty_array(env, &env->frame->tmpvar[dst]); */
                ASM {
                        /* r13: exception_handler */
                        /* r14: env */
                        /* r15: &env->frame->tmpvar[0] */

                        /* subq %rsp, 32 */                       IB(0x48); IB(0x83); IB(0xec); IB(0x20);
                        /* (1st) movq %r14 -> %rcx */             IB(0x4c); IB(0x89); IB(0xf1);
                        /* (2nd) movq $dst -> %rdx */             IB(0x48); IB(0xc7); IB(0xc2); ID((uint32_t)dst);
                        /*       addq %r15 -> %rdx */             IB(0x4c); IB(0x01); IB(0xfa);
                        /* movabs rt_make_empty_array -> %rax */  IB(0x48); IB(0xb8); IQ((uint64_t)rt_make_empty_array);
                        /* call *%rax */                          IB(0xff); IB(0xd0);
                        /* add %rsp, 32 */                        IB(0x48); IB(0x83); IB(0xc4); IB(0x20);

                        /* testl %eax, %eax */                    IB(0x83); IB(0xf8); IB(0x00);
                        /* jne 8 <next> */                        IB(0x75); IB(0x03);
                        /* jmp *%r13 */                           IB(0x41); IB(0xff); IB(0xe5);
                /* next: */
                }
        } else {
                /* rt_make_empty_array(env, &env->frame->tmpvar[dst]); */
                ASM {
                        /* r13: exception_handler */
                        /* r14: env */
                        /* r15: &env->frame->tmpvar[0] */

                        /* (1st) movq %r14 -> %rdi */             IB(0x4c); IB(0x89); IB(0xf7);
                        /* (2nd) movq $dst -> %rsi */             IB(0x48); IB(0xc7); IB(0xc6); ID((uint32_t)dst);
                        /*       addq %r15 -> %rsi */             IB(0x4c); IB(0x01); IB(0xfe);
                        /* movabs rt_make_empty_array -> %rax */  IB(0x48); IB(0xb8); IQ((uint64_t)rt_make_empty_array);
                        /* call *%rax */                          IB(0xff); IB(0xd0);

                        /* testl %eax, %eax */                    IB(0x83); IB(0xf8); IB(0x00);
                        /* jne 8 <next> */                        IB(0x75); IB(0x03);
                        /* jmp *%r13 */                           IB(0x41); IB(0xff); IB(0xe5);
                /* next: */
                }
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

        if (IS_MSABI) {
                /* rt_make_empty_dict(env, &env->frame->tmpvar[dst]); */
                ASM {
                        /* r13: exception_handler */
                        /* r14: env */
                        /* r15: &env->frame->tmpvar[0] */

                        /* subq %rsp, 32 */                     IB(0x48); IB(0x83); IB(0xec); IB(0x20);
                        /* (1st) movq %r14 -> rb%cx */          IB(0x4c); IB(0x89); IB(0xf1);
                        /* (2nd) movq $dst -> %rdx */           IB(0x48); IB(0xc7); IB(0xc2); ID((uint32_t)dst);
                        /*       add %r15 -> %rdx */            IB(0x4c); IB(0x01); IB(0xfa);
                        /* movabs rt_make_empty_dict -> %rax */ IB(0x48); IB(0xb8); IQ((uint64_t)rt_make_empty_dict);
                        /* call *%rax */                        IB(0xff); IB(0xd0);
                        /* addq %rsp, 32 */                     IB(0x48); IB(0x83); IB(0xc4); IB(0x20);

                        /* testl %eax, %eax */                  IB(0x83); IB(0xf8); IB(0x00);
                        /* jne 8 <next> */                      IB(0x75); IB(0x03);
                        /* jmp *%r13 */                         IB(0x41); IB(0xff); IB(0xe5);
                /* next: */
                }
        } else {
                /* rt_make_empty_dict(env, &env->frame->tmpvar[dst]); */
                ASM {
                        /* r13: exception_handler */
                        /* r14: env */
                        /* r15: &env->frame->tmpvar[0] */

                        /* (1st) movq %r14 -> %rdi */           IB(0x4c); IB(0x89); IB(0xf7);
                        /* (2nd) movq $dst, %rsi */             IB(0x48); IB(0xc7); IB(0xc6); ID((uint32_t)dst);
                        /*       addq %r15, %rsi */             IB(0x4c); IB(0x01); IB(0xfe);
                        /* movabs rt_make_empty_dict, %r8 */    IB(0x49); IB(0xb8); IQ((uint64_t)rt_make_empty_dict);
                        /* call *%r8 */                         IB(0x41); IB(0xff); IB(0xd0);

                        /* testl %eax, %eax */                  IB(0x83); IB(0xf8); IB(0x00);
                        /* jne 8 <next> */                      IB(0x75); IB(0x03);
                        /* jmp *%r13 */                         IB(0x41); IB(0xff); IB(0xe5);
                /* next: */
                }
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

        /* &env->frame->tmpvar[dst].val.i++ */
        ASM {
                /* r13: exception_handler */
                /* r14: env */
                /* r15: &env->frame->tmpvar[0] */

                /* movq $dst -> %rax */      IB(0x48); IB(0xc7); IB(0xc0); ID((uint32_t)dst);
                /* addq %r15 -> %rax */      IB(0x4c); IB(0x01); IB(0xf8);
                /* incq [%rax+8] */          IB(0x48); IB(0xff); IB(0x40); IB(0x08);
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

        /* if (!rt_shl_helper(env, dst, src1, src2)) return false; */
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

        /* if (!rt_shr_helper(env, dst, src1, src2)) return false; */
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

        /* src1 - src2 */
        ASM {
                /* r13: exception_handler */
                /* r14: env */
                /* r15: &env->frame->tmpvar[0] */

                /* movq $dst -> %rax */          IB(0x48); IB(0xc7); IB(0xc0); ID((uint32_t)dst);
                /* addq %r15 -> %rax */          IB(0x4c); IB(0x01); IB(0xf8);

                /* movq $src1 -> %rbx */         IB(0x48); IB(0xc7); IB(0xc3); ID((uint32_t)src1);
                /* addq %r15 -> %rbx */          IB(0x4c); IB(0x01); IB(0xfb);
                
                /* movq $src2 -> %rcx */         IB(0x48); IB(0xc7); IB(0xc1); ID((uint32_t)src2);
                /* addq %r15 -> %rcx */          IB(0x4c); IB(0x01); IB(0xf9);

                /* movq 8(%rbx) -> %rax */       IB(0x48); IB(0x8b); IB(0x43); IB(0x08);
                /* movq 8(%rcx) -> %rdx */       IB(0x48); IB(0x8b); IB(0x51); IB(0x08);
                /* cmpl %eax, %edx */            IB(0x39); IB(0xc2);
        }

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

        /* if (!rt_storearray_helper(env, dst, src1, src2)) return false; */
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

        /* if (!rt_len_helper(env, dst, src)) return false; */
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

        /* if (!rt_getdictkeybyindex_helper(env, dst, src1, src2)) return false; */
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

        /* if (!rt_getdictvalbyindex_helper(env, dst, src1, src2)) return false; */
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
        uint32_t src_len, src_hash;
        uint64_t src;

        CONSUME_TMPVAR(dst);
        CONSUME_STRING(src_s, src_len, src_hash);
        src = (uint64_t)(intptr_t)src_s;

        if (IS_MSABI) {
                /* if (!rt_loadsymbol_helper(env, dst, src, src_len, src_hash)) return false; */
                ASM {
                        /* r13: exception_handler */
                        /* r14: env */
                        /* r14: &env->frame->tmpvar[0] */

                        /* subq %rsp, 64 */                       IB(0x48); IB(0x83); IB(0xec); IB(0x40);
                        /* (1st) movq %r14 -> %rcx */             IB(0x4c); IB(0x89); IB(0xf1);
                        /* (2nd) movq $dst -> %rdx */             IB(0x48); IB(0xc7); IB(0xc2); ID((uint32_t)dst);
                        /* (3rd) movabs $src -> %r8 */            IB(0x49); IB(0xb8); IQ((uint64_t)src);
                        /* (4th) movq $src_len -> %r9 */          IB(0x49); IB(0xc7); IB(0xc1); ID((uint32_t)src_len);
                        /* (5th) movq $src_hash -> 32(%rsp) */    IB(0x48); IB(0xc7); IB(0x44); IB(0x24); IB(0x20); ID((uint32_t)src_hash);
                        /* movabs rt_loadsymbol_helper -> %rax */ IB(0x48); IB(0xb8); IQ((uint64_t)rt_loadsymbol_helper);
                        /* call *%rax */                          IB(0xff); IB(0xd0);
                        /* addq %rsp, 64 */                       IB(0x48); IB(0x83); IB(0xc4); IB(0x40);

                        /* testl %eax, %eax */                    IB(0x83); IB(0xf8); IB(0x00);
                        /* jne 8 <next> */                        IB(0x75); IB(0x03);
                        /* jmp *%r13 */                           IB(0x41); IB(0xff); IB(0xe5);
                /* next: */
                }
        } else {
                /* if (!rt_loadsymbol_helper(env, dst, src, src_len, src_hash)) return false; */
                ASM {
                        /* r13: exception_handler */
                        /* r14: env */
                        /* r14: &env->frame->tmpvar[0] */

                        /* (1st) movq %r14, %rdi */               IB(0x4c); IB(0x89); IB(0xf7);
                        /* (2nd) movq $dst, %rsi */               IB(0x48); IB(0xc7); IB(0xc6); ID((uint32_t)dst);
                        /* (3rd) movabs %src, %rdx */             IB(0x48); IB(0xba); IQ(src);
                        /* (4th) movq $src_len, %rcx */           IB(0x48); IB(0xc7); IB(0xc1); ID((uint32_t)src_len);
                        /* (5th) movq $src_hash, %r8 */           IB(0x49); IB(0xc7); IB(0xc0); ID((uint32_t)src_hash);
                        /* movabs rt_loadsymbol_helper -> %rax */ IB(0x48); IB(0xb8); IQ((uint64_t)rt_loadsymbol_helper);
                        /* call *%rax */                          IB(0xff); IB(0xd0);

                        /* testl %eax, %eax */                    IB(0x83); IB(0xf8); IB(0x00);
                        /* jne 8 <next> */                        IB(0x75); IB(0x03);
                        /* jmp *%r13 */                           IB(0x41); IB(0xff); IB(0xe5);
                /* next:*/
                }
        }

        return true;
}

/* Visit a OP_STORESYMBOL instruction. */
static INLINE bool
jit_visit_storesymbol_op(
        struct jit_context *ctx)
{
        const char *dst_s;
        uint32_t dst_len, dst_hash;
        uint64_t dst;
        int src;

        CONSUME_STRING(dst_s, dst_len, dst_hash);
        CONSUME_TMPVAR(src);
        dst = (uint64_t)(intptr_t)dst_s;

        if (IS_MSABI) {
                /* if (!rt_storesymbol_helper(env, dst, dst_len, dst_hash, src)) return false; */
                ASM {
                        /* r13: exception_handler */
                        /* r14: env */
                        /* r14: &env->frame->tmpvar[0] */

                        /* subq %rsp, $64 */                       IB(0x48); IB(0x83); IB(0xec); IB(0x40);
                        /* (1st) movq %r14 -> %rcx */              IB(0x4c); IB(0x89); IB(0xf1);
                        /* (2nd) movabs $dst -> %rdx */            IB(0x48); IB(0xba); IQ((uint64_t)dst);
                        /* (3rd) movq $dst_len -> %r8 */           IB(0x49); IB(0xc7); IB(0xc0); ID((uint32_t)dst_len);
                        /* (4th) movq $dst_hash -> %r9 */          IB(0x49); IB(0xc7); IB(0xc1); ID((uint32_t)dst_hash);
                        /* (5th) movq $src -> 32(%rsp) */          IB(0x48); IB(0xc7); IB(0x44); IB(0x24); IB(0x20); ID((uint32_t)src);
                        /* movabs rt_storesymbol_helper -> %rax */ IB(0x48); IB(0xb8); IQ((uint64_t)rt_storesymbol_helper);
                        /* call *%rax */                           IB(0xff); IB(0xd0);
                        /* addq %rsp, $64 */                       IB(0x48); IB(0x83); IB(0xc4); IB(0x40);

                        /* testl %eax, %eax */                     IB(0x83); IB(0xf8); IB(0x00);
                        /* jne 8 <next> */                         IB(0x75); IB(0x03);
                        /* jmp *%r13 */                            IB(0x41); IB(0xff); IB(0xe5);
                /* next: */
                }
        } else {
                /* if (!rt_storesymbol_helper(env, dst, dst_len, dst_hash, src)) return false; */
                ASM {
                        /* r13: exception_handler */
                        /* r14: env */
                        /* r14: &env->frame->tmpvar[0] */

                        /* (1st) movq %r14, %rdi */                IB(0x4c); IB(0x89); IB(0xf7);
                        /* (2nd) movabs $dst, %rsi */              IB(0x48); IB(0xbe); IQ(dst);
                        /* (3rd) movq $dst_len, %rdx */            IB(0x48); IB(0xc7); IB(0xc2); ID((uint32_t)dst_len);
                        /* (4th) movq $dst_hash, %rcx */           IB(0x48); IB(0xc7); IB(0xc1); ID((uint32_t)dst_hash);
                        /* (5th) movq $src, %r8 */                 IB(0x49); IB(0xc7); IB(0xc0); ID((uint32_t)src);
                        /* movabs rt_storesymbol_helper -> %rax */ IB(0x48); IB(0xb8); IQ((uint64_t)rt_storesymbol_helper);
                        /* call *%rax */                           IB(0xff); IB(0xd0);

                        /* testl %eax, %eax */                     IB(0x83); IB(0xf8); IB(0x00);
                        /* jne 8 <next> */                         IB(0x75); IB(0x03);
                        /* jmp *%r13 */                            IB(0x41); IB(0xff); IB(0xe5);
                /* next:*/
                }
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
        uint32_t field_len;
        uint32_t field_hash;
        uint64_t field;

        CONSUME_TMPVAR(dst);
        CONSUME_TMPVAR(dict);
        CONSUME_STRING(field_s, field_len, field_hash);
        field = (uint64_t)(intptr_t)field_s;

        if (IS_MSABI) {
                /* if (!rt_loaddot_helper(env, dst, dict, field, field_len, field_hash)) return false; */
                ASM {
                        /* r13: exception_handler */
                        /* r14: env */
                        /* r14: &env->frame->tmpvar[0] */

                        /* subq %rsp, $64 */                      IB(0x48); IB(0x83); IB(0xec); IB(0x40);
                        /* (1st) movq %r14 -> %rcx */             IB(0x4c); IB(0x89); IB(0xf1);
                        /* (2nd) movq $dst -> %rdx */             IB(0x48); IB(0xc7); IB(0xc2); ID((uint32_t)dst);
                        /* (3rd) movq $dict -> %r8 */             IB(0x49); IB(0xb8); IQ((uint64_t)dict);
                        /* (4th) movabs $field -> %r9 */          IB(0x49); IB(0xb9); IQ((uint64_t)field);
                        /* (5th) movq $field_len -> 32(%rsp) */   IB(0x48); IB(0xc7); IB(0x44); IB(0x24); IB(0x20); ID((uint32_t)field_len);
                        /* (6th) movq $field_hash -> 40(%rsp) */  IB(0x48); IB(0xc7); IB(0x44); IB(0x24); IB(0x28); ID((uint32_t)field_hash);
                        /* movabs rt_loaddot_helper -> %rax */    IB(0x48); IB(0xb8); IQ((uint64_t)rt_loaddot_helper);
                        /* call *%rax */                          IB(0xff); IB(0xd0);
                        /* addq %rsp, $64 */                      IB(0x48); IB(0x83); IB(0xc4); IB(0x40);

                        /* testl %eax, %eax */                    IB(0x83); IB(0xf8); IB(0x00);
                        /* jne 8 <next> */                        IB(0x75); IB(0x03);
                        /* jmp *%r13 */                           IB(0x41); IB(0xff); IB(0xe5);
                /* next: */
                }
        } else {
                /* if (!rt_loaddot_helper(env, dst, dict, field, field_len, field_hash)) return false; */
                ASM {
                        /* r13: exception_handler */
                        /* r14: env */
                        /* r14: &env->frame->tmpvar[0] */

                        /* (1st) movq %r14, %rdi */               IB(0x4c); IB(0x89); IB(0xf7);
                        /* (2nd) movq $dst, %rsi */               IB(0x48); IB(0xc7); IB(0xc6); ID((uint32_t)dst);
                        /* (3rd) movq $dict, %rdx */              IB(0x48); IB(0xc7); IB(0xc2); ID((uint32_t)dict);
                        /* (4th) movabs $field, %rcx */           IB(0x48); IB(0xb9); IQ(field);
                        /* (5th) movq $field_len, %r8 */          IB(0x49); IB(0xc7); IB(0xc0); ID((uint32_t)field_len);
                        /* (6th) movq $field_hash, %r9 */         IB(0x49); IB(0xc7); IB(0xc1); ID((uint32_t)field_hash);
                        /* movabs rt_loaddot_helper -> %rax */    IB(0x48); IB(0xb8); IQ((uint64_t)rt_loaddot_helper);
                        /* call *%rax */                          IB(0xff); IB(0xd0);

                        /* testl %eax, %eax */                    IB(0x83); IB(0xf8); IB(0x00);
                        /* jne 8 <next> */                        IB(0x75); IB(0x03);
                        /* jmp *%r13 */                           IB(0x41); IB(0xff); IB(0xe5);
                /* next:*/
                }
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
        uint32_t field_len;
        uint32_t field_hash;
        uint64_t field;
        int src;

        CONSUME_TMPVAR(dict);
        CONSUME_STRING(field_s, field_len, field_hash);
        CONSUME_TMPVAR(src);
        field = (uint64_t)(intptr_t)field_s;

        if (IS_MSABI) {
                /* if (!rt_storedot_helper(env, dict, field, field_len, field_hash, src)) return false; */
                ASM {
                        /* r13: exception_handler */
                        /* r14: env */
                        /* r14: &env->frame->tmpvar[0] */

                        /* subq %rsp, $64 */                     IB(0x48); IB(0x83); IB(0xec); IB(0x40);
                        /* (1st) movq %r14 -> %rcx */            IB(0x4c); IB(0x89); IB(0xf1);
                        /* (2nd) movl $dict -> %edx */           IB(0xba); ID((uint32_t)dict);
                        /* (3rd) movabs $field -> %r8 */         IB(0x49); IB(0xb8); IQ((uint64_t)field);
                        /* (4th) movq $field_len -> %r9 */       IB(0x49); IB(0xc7); IB(0xc1); ID((uint32_t)field_len);
                        /* (5th) movq $field_hash -> 32(%rsp) */ IB(0x48); IB(0xc7); IB(0x44); IB(0x24); IB(0x20); ID((uint32_t)field_hash);
                        /* (6th) movq $src -> 40(%rsp) */        IB(0x48); IB(0xc7); IB(0x44); IB(0x24); IB(0x28); ID((uint32_t)src);
                        /* movabs rt_storedot_helper -> %rax */  IB(0x48); IB(0xb8); IQ((uint64_t)rt_storedot_helper);
                        /* call *%rax */                         IB(0xff); IB(0xd0);
                        /* addq %rsp, $64 */                     IB(0x48); IB(0x83); IB(0xc4); IB(0x40);

                        /* testl %eax, %eax */                   IB(0x83); IB(0xf8); IB(0x00);
                        /* jne 8 <next> */                       IB(0x75); IB(0x03);
                        /* jmp *r13 */                           IB(0x41); IB(0xff); IB(0xe5);
                /* next: */
                }
        } else {
                /* if (!rt_storedot_helper(env, dict, field, field_len, field_hash, src)) return false; */
                ASM {
                        /* r13: exception_handler */
                        /* r14: env */
                        /* r14: &env->frame->tmpvar[0] */

                        /* (1st) movq %r14 -> %rdi */            IB(0x4c); IB(0x89); IB(0xf7);
                        /* (2nd) movq $dict -> %rsi */           IB(0x48); IB(0xc7); IB(0xc6); ID((uint32_t)dict);
                        /* (3rd) movabs $field -> %rdx */        IB(0x48); IB(0xba); IQ(field);
                        /* (4th) movq $field_len -> %rcx */      IB(0x48); IB(0xc7); IB(0xc1); ID((uint32_t)field_len);
                        /* (5th) movq $field_hash -> %r8 */      IB(0x49); IB(0xc7); IB(0xc0); ID((uint32_t)field_hash);
                        /* (6th) movq $src -> %r9 */             IB(0x49); IB(0xc7); IB(0xc1); ID((uint32_t)src);
                        /* movabs rt_storedot_helper -> %rax */  IB(0x48); IB(0xb8); IQ((uint64_t)rt_storedot_helper);
                        /* call *%rax */                         IB(0xff); IB(0xd0);

                        /* testl %eax, %eax */                   IB(0x83); IB(0xf8); IB(0x00);
                        /* jne 8 <next> */                       IB(0x75); IB(0x03);
                        /* jmp *%r13 */                          IB(0x41); IB(0xff); IB(0xe5);
                /* next:*/
                }
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
                        /* jmp (5 + arg_count * 4) */
                        IB(0xe9);
                        ID((uint32_t)(4 * arg_count));
                }
                arg_addr = (uint64_t)(intptr_t)ctx->code;
                for (i = 0; i < arg_count; i++) {
                        *(int *)ctx->code = arg[i];
                        ctx->code = (uint8_t *)ctx->code + 4;
                }
        } else {
                arg_addr = 0;
        }

        if (IS_MSABI) {
                /* if (!rt_call_helper(env, dst, func, arg_count, arg)) return false; */
                ASM {
                        /* r13: exception_handler */
                        /* r14: env */
                        /* r14: &env->frame->tmpvar[0] */

                        /* subq %rsp, $64 */                     IB(0x48); IB(0x83); IB(0xEC); IB(0x40);
                        /* (1st) movq %r14 -> %rcx */            IB(0x4C); IB(0x89); IB(0xF1);
                        /* (2nd) movq $dst -> %rdx */            IB(0x48); IB(0xc7); IB(0xc2); ID((uint32_t)dst);
                        /* (3rd) movq $func -> %r8 */            IB(0x49); IB(0xc7); IB(0xc0); ID((uint32_t)func);
                        /* (4th) movq $arg_count -> %r9 */       IB(0x49); IB(0xc7); IB(0xc1); ID((uint32_t)arg_count);
                        /* (5th) movabs $arg_addr -> %rax */     IB(0x48); IB(0xB8); IQ((uint64_t)arg_addr);
                        /*       movq %rax -> 32(%rsp) */        IB(0x48); IB(0x89); IB(0x44); IB(0x24); IB(0x20);
                        /* movabs rt_call_helper -> rax */       IB(0x48); IB(0xB8); IQ((uint64_t)rt_call_helper);
                        /* call *%rax */                         IB(0xFF); IB(0xD0);
                        /* addq %rsp, $64 */                     IB(0x48); IB(0x83); IB(0xC4); IB(0x40);

                        /* test eax, eax */                      IB(0x83); IB(0xF8); IB(0x00);
                        /* jne 8 <next> */                       IB(0x75); IB(0x03);
                        /* jmp *r13 */                           IB(0x41); IB(0xFF); IB(0xE5);
                /* next: */
                }
        } else {
                /* if (!rt_call_helper(env, dst, func, arg_count, arg)) return false; */
                ASM {
                        /* r13: exception_handler */
                        /* r14: env */
                        /* r14: &env->frame->tmpvar[0] */

                        /* (1st) movq %r14, %rdi */              IB(0x4c); IB(0x89); IB(0xf7);
                        /* (2nd) movq $dst, %rsi */              IB(0x48); IB(0xc7); IB(0xc6); ID((uint32_t)dst);
                        /* (3rd) movq $func, %rdx */             IB(0x48); IB(0xc7); IB(0xc2); ID((uint32_t)func);
                        /* (4th) movq $arg_count, %rcx */        IB(0x48); IB(0xc7); IB(0xc1); ID((uint32_t)arg_count);
                        /* (5th) movabs $arg_addr, %r8 */        IB(0x49); IB(0xb8); IQ(arg_addr);
                        /* movabs rt_call_helper -> rax */       IB(0x48); IB(0xB8); IQ((uint64_t)rt_call_helper);
                        /* call *%rax */                         IB(0xFF); IB(0xD0);

                        /* cmpl $0, %eax */                      IB(0x83); IB(0xf8); IB(0x00);
                        /* jne 8 <next> */                       IB(0x75); IB(0x03);
                        /* jmp *%r13 */                          IB(0x41); IB(0xff); IB(0xe5);
                /* next:*/
                }
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
        uint32_t symbol_len;
        uint32_t symbol_hash;
        int arg_count;
        int arg_tmp;
        int arg[NOCT_ARG_MAX];
        uint64_t arg_addr;
        int i;

        CONSUME_TMPVAR(dst);
        CONSUME_TMPVAR(obj);
        CONSUME_STRING(symbol, symbol_len, symbol_hash);
        CONSUME_IMM8(arg_count);
        for (i = 0; i < arg_count; i++) {
                CONSUME_TMPVAR(arg_tmp);
                arg[i] = arg_tmp;
        }

        /* Embed arguments to the code. */
        ASM {
                /* jmp (5 + arg_count * 4) */
                IB(0xe9);
                ID((uint32_t)(4 * arg_count));
        }
        arg_addr = (uint64_t)(intptr_t)ctx->code;
        for (i = 0; i < arg_count; i++) {
                *(int *)ctx->code = arg[i];
                ctx->code = (uint8_t *)ctx->code + 4;
        }

        if (IS_MSABI) {
                /* if (!rt_thiscall_helper(env, dst, obj, symbol, symbol_len, symbol_hash, arg_count, arg)) return false; */
                ASM {
                        /* r13: exception_handler */
                        /* r14: env */
                        /* r14: &env->frame->tmpvar[0] */

                        /* subq %rsp, $64 */                           IB(0x48); IB(0x83); IB(0xec); IB(0x40);
                        /* (1st) movq %r14 -> %rcx */                  IB(0x4c); IB(0x89); IB(0xf1);
                        /* (2nd) movq $dst -> %rdx */                  IB(0x48); IB(0xc7); IB(0xc2); ID((uint32_t)dst);
                        /* (3rd) movq $obj -> %r8 */                   IB(0x49); IB(0xc7); IB(0xc0); ID((uint32_t)obj);
                        /* (4th) movabs $symbol -> %r9 */              IB(0x49); IB(0xb9); IQ((uint64_t)symbol);
                        /* (5th) movq $symbol_len -> 32(%rsp) */       IB(0x48); IB(0xc7); IB(0x44); IB(0x24); IB(0x20); ID((uint32_t)symbol_len);
                        /* (6th) movq $symbol_hash -> 40(%rsp) */      IB(0x48); IB(0xc7); IB(0x44); IB(0x24); IB(0x28); ID((uint32_t)symbol_hash);
                        /* (7th) movq $arg_count -> 48(%rsp) */        IB(0x48); IB(0xc7); IB(0x44); IB(0x24); IB(0x30); ID((uint32_t)arg_count);
                        /* (8th) movabs $arg_addr -> %rax */           IB(0x48); IB(0xb8); IQ((uint64_t)arg_addr);
                        /*       movq %rax -> 56(%rsp) */              IB(0x48); IB(0x89); IB(0x44); IB(0x24); IB(0x38);
                        /* movabs rt_thiscall_helper -> %rax */        IB(0x48); IB(0xb8); IQ((uint64_t)rt_thiscall_helper);
                        /* call *%rax */                               IB(0xff); IB(0xd0);
                        /* addq %rsp, $64 */                           IB(0x48); IB(0x83); IB(0xc4); IB(0x40);

                        /* testl %eax, %eax */                         IB(0x83); IB(0xF8); IB(0x00);
                        /* jne 8 <next> */                             IB(0x75); IB(0x03);
                        /* jmp *r13 */                                 IB(0x41); IB(0xFF); IB(0xe5);
                /* next: */
                }
        } else {
                /* if (!rt_thiscall_helper(env, dst, obj, symbol, symbol_len, symbol_hash, arg_count, arg)) return false; */
                ASM {
                        /* r13: exception_handler */
                        /* r14: env */
                        /* r14: &env->frame->tmpvar[0] */

                        /* subq %rsp, $32 */                          IB(0x48); IB(0x83); IB(0xEC); IB(0x20);
                        /* (1st) movq %r14 -> %rdi */                 IB(0x4c); IB(0x89); IB(0xf7);
                        /* (2nd) movq $dst -> %rsi */                 IB(0x48); IB(0xc7); IB(0xc6); ID((uint32_t)dst);
                        /* (3rd) movq $obj -> %rdx */                 IB(0x48); IB(0xc7); IB(0xc2); ID((uint32_t)obj);
                        /* (4th) movabs $symbol -> %rcx */            IB(0x48); IB(0xb9); IQ((uint64_t)symbol);
                        /* (5th) movq $symbol_len -> %r8 */           IB(0x49); IB(0xc7); IB(0xc0); ID((uint32_t)symbol_len);
                        /* (6th) movq $symbol_hash -> %r9 */          IB(0x49); IB(0xc7); IB(0xc1); ID((uint32_t)symbol_hash);
                        /* (7th) movq $arg_count -> 0(%rsp) */        IB(0x48); IB(0xc7); IB(0x04); IB(0x24); ID((uint32_t)arg_count);
                        /* (8th) movabs $arg -> %rax */               IB(0x48); IB(0xB8); IQ((uint64_t)arg_addr);
                        /*       movq %rax -> 8(%rsp) */              IB(0x48); IB(0x89); IB(0x44); IB(0x24); IB(0x08);
                        /* movabs rt_thiscall_helper -> %r10 */       IB(0x49); IB(0xba); IQ((uint64_t)rt_thiscall_helper);
                        /* call *%r10 */                              IB(0x41); IB(0xff); IB(0xd2);
                        /* add %rsp, 32 */                            IB(0x48); IB(0x83); IB(0xc4); IB(0x20);

                        /* testl %eax, %eax */                         IB(0x83); IB(0xF8); IB(0x00);
                        /* jne 8 <next> */                            IB(0x75); IB(0x03);
                        /* jmp *%r13 */                               IB(0x41); IB(0xff); IB(0xe5);
                /* next:*/
                }
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
        ctx->branch_patch[ctx->branch_patch_count].type = PATCH_JMP;
        ctx->branch_patch_count++;

        ASM {
                /* Patched later. */
                /* jmp 5 */        IB(0xe9); ID(0);
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
                /* r13: exception_handler */
                /* r14: rt */
                /* r15: &env->frame->tmpvar[0] */

                /* rdx = &env->frame->tmpvar[src] */
                /* movq src, %rdx */               IB(0x48); IB(0xc7); IB(0xc2); ID((uint32_t)src);
                /* addq %r15, %rdx */              IB(0x4c); IB(0x01); IB(0xfa);
                /* movl 8(%rdx), %eax */           IB(0x8b); IB(0x42); IB(0x08);

                /* Compare: env->frame->tmpvar[dst].val.i == 0 */
                /* test %eax, %eax */                        IB(0x85); IB(0xc0);
        }

        /* Patch later. */
        ctx->branch_patch[ctx->branch_patch_count].code = ctx->code;
        ctx->branch_patch[ctx->branch_patch_count].lpc = target_lpc;
        ctx->branch_patch[ctx->branch_patch_count].type = PATCH_JNE;
        ctx->branch_patch_count++;

        ASM {
                /* Patched later. */
                /* jne 6 */                                IB(0x0f); IB(0x85); ID(0);
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
                /* rdx = &env->frame->tmpvar[src] */
                /* movq src, %rdx */               IB(0x48); IB(0xc7); IB(0xc2); ID((uint32_t)src);
                /* addq %r15, %rdx */              IB(0x4c); IB(0x01); IB(0xfa);
                /* movl 8(%rdx), %eax */           IB(0x8b); IB(0x42); IB(0x08);

                /* Compare: env->frame->tmpvar[dst].val.i == 0 */
                /* test %eax, %eax */                        IB(0x85); IB(0xc0);
        }

        /* Patch later. */
        ctx->branch_patch[ctx->branch_patch_count].code = ctx->code;
        ctx->branch_patch[ctx->branch_patch_count].lpc = target_lpc;
        ctx->branch_patch[ctx->branch_patch_count].type = PATCH_JE;
        ctx->branch_patch_count++;

        ASM {
                /* Patched later. */
                /* je 6 */                                IB(0x0f); IB(0x84); ID(0);
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
        ctx->branch_patch[ctx->branch_patch_count].type = PATCH_JE;
        ctx->branch_patch_count++;

        ASM {
                /* Patched later. */
                /* je 6 */                                IB(0x0f); IB(0x84); ID(0);
        }

        return true;
}

/* Visit a bytecode of a function. */
bool
jit_visit_bytecode(
        struct jit_context *ctx)
{
        uint8_t opcode;

        if (IS_MSABI) {
                /* Put a prologue. */
                ASM {
                /* prologue: */
                        /* %rsp = 16n + 8 */

                        /* pushq %rax */                        IB(0x50);
                        /* pushq %rbx */                        IB(0x53);
                        /* pushq %rcx */                        IB(0x51);
                        /* pushq %rdx */                        IB(0x52);
                        /* pushq %rdi */                        IB(0x57);
                        /* pushq %rsi */                        IB(0x56);
                        /* pushq %r12 */                        IB(0x41); IB(0x54);
                        /* pushq %r13 */                        IB(0x41); IB(0x55);
                        /* pushq %r14 */                        IB(0x41); IB(0x56);
                        /* pushq %r15 */                        IB(0x41); IB(0x57);

                        /* align stack to 16 bytes */
                        /* sub rsp, 8 */                        IB(0x48); IB(0x83); IB(0xEC); IB(0x08);

                        /* r14 = env */
                        /* movq %rcx, %r14 */                   IB(0x49); IB(0x89); IB(0xCE);

                        /* r15 = *&env->frame->tmpvar[0] */
                        /* movq (%r14), %rax */                 IB(0x49); IB(0x8b); IB(0x06);
                        /* movq (%rax), %r15 */                 IB(0x4c); IB(0x8b); IB(0x38);

                        /* r13 = exception_handler */
                        /* movabs (ctx->code + 10), %r13 */     IB(0x49); IB(0xbd); IQ((uint64_t)(intptr_t)((uint8_t*)ctx->code + 10));

                        /* Skip an exception handler. */
                        /* jmp exception_handler_end */         IB(0xeb); IB(0x1a);
                }

               /* Put an exception handler. */
                ctx->exception_code = ctx->code;
                ASM {
                /* exception_handler: */
                        /* addq $8, %rsp (align back) */        IB(0x48); IB(0x83); IB(0xC4); IB(0x08);
                        /* popq %r15 */                         IB(0x41); IB(0x5f);
                        /* popq %r14 */                         IB(0x41); IB(0x5e);
                        /* popq %r13 */                         IB(0x41); IB(0x5d);
                        /* popq %r12 */                         IB(0x41); IB(0x5c);
                        /* popq %rsi */                         IB(0x5e);
                        /* popq %rdi */                         IB(0x5f);
                        /* popq %rdx */                         IB(0x5a);
                        /* popq %rcx */                         IB(0x59);
                        /* popq %rbx */                         IB(0x5b);
                        /* popq %rax */                         IB(0x58);
                        /* movq $0, %rax */                     IB(0x48); IB(0xc7); IB(0xc0); ID(0);
                        /* ret */                               IB(0xc3);
                /* exception_handler_end: */
                }
        } else {
                /* Put a prologue. */
                ASM {
                /* prologue: */
                        /* pushq %rax */                        IB(0x50);
                        /* pushq %rbx */                        IB(0x53);
                        /* pushq %rcx */                        IB(0x51);
                        /* pushq %rdx */                        IB(0x52);
                        /* pushq %rdi */                        IB(0x57);
                        /* pushq %rsi */                        IB(0x56);
                        /* pushq %r13 */                        IB(0x41); IB(0x55);
                        /* pushq %r14 */                        IB(0x41); IB(0x56);
                        /* pushq %r15 */                        IB(0x41); IB(0x57);

                        /* r14 = env */
                        /* movq %rdi, %r14 */                   IB(0x49); IB(0x89); IB(0xfe);

                        /* r15 = *&env->frame->tmpvar[0] */
                        /* movq (%r14), %rax */                 IB(0x49); IB(0x8b); IB(0x06);
                        /* movq (%rax), %r15 */                 IB(0x4c); IB(0x8b); IB(0x38);

                        /* r13 = exception_handler */
                        /* movabs (ctx->code + 10), %r13 */     IB(0x49); IB(0xbd); IQ((uint64_t)(intptr_t)((uint8_t*)ctx->code + 10));

                        /* Skip an exception handler. */
                        /* jmp exception_handler_end */         IB(0xeb); IB(0x14);
                }

                /* Put an exception handler. */
                ctx->exception_code = ctx->code;
                ASM {
                /* exception_handler: */
                        /* popq %r15 */                         IB(0x41); IB(0x5f);
                        /* popq %r14 */                         IB(0x41); IB(0x5e);
                        /* popq %r13 */                         IB(0x41); IB(0x5d);
                        /* popq %rsi */                         IB(0x5e);
                        /* popq %rdi */                         IB(0x5f);
                        /* popq %rdx */                         IB(0x5a);
                        /* popq %rcx */                         IB(0x59);
                        /* popq %rbx */                         IB(0x5b);
                        /* popq %rax */                         IB(0x58);
                        /* movq $0, %rax */                     IB(0x48); IB(0xc7); IB(0xc0); ID(0);
                        /* ret */                               IB(0xc3);
                /* exception_handler_end: */
                }
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

        if (IS_MSABI) {
                /* Put an epilogue. */
                ASM {
                /* epilogue: */
                        /* addq $8, %rsp (align back) */ IB(0x48); IB(0x83); IB(0xC4); IB(0x08);
                        /* popq %r15 */                  IB(0x41); IB(0x5f);
                        /* popq %r14 */                  IB(0x41); IB(0x5e);
                        /* popq %r13 */                  IB(0x41); IB(0x5d);
                        /* popq %r12 */                  IB(0x41); IB(0x5c);
                        /* popq %rsi */                  IB(0x5e);
                        /* popq %rdi */                  IB(0x5f);
                        /* popq %rdx */                  IB(0x5a);
                        /* popq %rcx */                  IB(0x59);
                        /* popq %rbx */                  IB(0x5b);
                        /* popq %rax */                  IB(0x58);
                        /* movq $1, %rax */              IB(0x48); IB(0xc7); IB(0xc0); ID(1);
                        /* ret */                        IB(0xc3);
                }
        } else {
                /* Put an epilogue. */
                ASM {
                /* epilogue: */
                        /* popq %r15 */                  IB(0x41); IB(0x5f);
                        /* popq %r14 */                  IB(0x41); IB(0x5e);
                        /* popq %r13 */                  IB(0x41); IB(0x5d);
                        /* popq %rsi */                  IB(0x5e);
                        /* popq %rdi */                  IB(0x5f);
                        /* popq %rdx */                  IB(0x5a);
                        /* popq %rcx */                  IB(0x59);
                        /* popq %rbx */                  IB(0x5b);
                        /* popq %rax */                  IB(0x58);
                        /* movq $1, %rax */              IB(0x48); IB(0xc7); IB(0xc0); ID(1);
                        /* ret */                        IB(0xc3);
                }
        }

        return true;
}

static bool
jit_patch_branch(
    struct jit_context *ctx,
    int patch_index)
{
        uint8_t *target_code;
        int offset;
        int i;

        /* Search a code addr at lpc. */
        target_code = NULL;
        for (i = 0; i < ctx->pc_entry_count; i++) {
                if (ctx->pc_entry[i].lpc == ctx->branch_patch[patch_index].lpc) {
                        target_code = (uint8_t *)ctx->pc_entry[i].code;
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

        /* Assemble. */
        if (ctx->branch_patch[patch_index].type == PATCH_JMP) {
                offset -= 5;
                ASM {
                        /* jmp offset */
                        IB(0xe9);
                        ID((uint32_t)offset);
                }
        } else if (ctx->branch_patch[patch_index].type == PATCH_JE) {
                offset -= 6;
                ASM {
                        /* je offset */
                        IB(0x0f);
                        IB(0x84);
                        ID((uint32_t)offset);
                }
        } else if (ctx->branch_patch[patch_index].type == PATCH_JNE) {
                offset -= 6;
                ASM {
                        /* jne offset */
                        IB(0x0f);
                        IB(0x85);
                        ID((uint32_t)offset);
                }
        }

        return true;
}

#endif /* defined(NOCT_ARCH_X86_64) && defined(NOCT_USE_JIT) */
