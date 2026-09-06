/* -*- coding: utf-8; tab-width: 8; indent-tabs-mode: nil; -*- */

/*
 * Noct Programming Language
 * Copyright (c) 2025, 2026, Awe Morris
 */

/*
 * JIT (x86): Just-In-Time native code generation
 */

#include <noct/noct.h>

#if defined(NOCT_ARCH_X86) && defined(NOCT_USE_JIT)

#include "runtime.h"
#include "jit.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#if defined(_MSC_VER)
#include <intrin.h>
#endif

/* False asseretion */
#define JIT_OP_NOT_IMPLEMENTED        0
#define NEVER_COME_HERE                0

/* Branch patch type */
#define PATCH_JMP                0
#define PATCH_JE                1
#define PATCH_JNE                2

/* Forward declaration */
static bool jit_visit_bytecode(struct rt_jit_context *ctx);
static bool jit_patch_branch(struct rt_jit_context *ctx, int patch_index);
static uint32_t jit_detect_simd_caps(void);
static bool jit_x86_has_cpuid(void);

/*
 * Generate a JIT-compiled code for a function.
 */
bool
jit_build(
          struct rt_env *env,
          struct rt_func *func)
{
        if (func->bytecode_size == 0 || func->cfunc != NULL)
                return false;
	return rt_jit_build_standard(env, func, jit_detect_simd_caps(), "x86",
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

#if defined(__GNUC__) || defined(__clang__)
static bool
jit_x86_has_cpuid(void)
{
        uint32_t before;
        uint32_t after;

        /* CPUID itself is not safe as a feature probe on old i386 CPUs.
         * EFLAGS.ID is writable exactly when CPUID is available. */
        __asm__ __volatile__(
                "pushfl\n\t"
                "pushfl\n\t"
                "popl %0\n\t"
                "movl %0, %1\n\t"
                "xorl $0x200000, %1\n\t"
                "pushl %1\n\t"
                "popfl\n\t"
                "pushfl\n\t"
                "popl %1\n\t"
                "popfl"
                : "=&r"(before), "=&r"(after)
                :
                : "cc");
        return ((before ^ after) & (1u << 21)) != 0;
}
#elif defined(_MSC_VER)
static bool
jit_x86_has_cpuid(void)
{
        uint32_t before;
        uint32_t after;

        __asm {
                pushfd
                pushfd
                pop eax
                mov before, eax
                xor eax, 00200000h
                push eax
                popfd
                pushfd
                pop eax
                mov after, eax
                popfd
        }
        return ((before ^ after) & (1u << 21)) != 0;
}
#endif

static uint32_t
jit_detect_simd_caps(void)
{
#if defined(__GNUC__) || defined(__clang__)
        uint32_t a, b, c, d;
	uint32_t caps;

	caps = 0;
        if (!jit_x86_has_cpuid())
                return 0;
        __asm__ __volatile__("cpuid"
                             : "=a"(a), "=b"(b), "=c"(c), "=d"(d)
                             : "a"(0), "c"(0));
        if (a < 1)
                return 0;
        __asm__ __volatile__("cpuid"
                             : "=a"(a), "=b"(b), "=c"(c), "=d"(d)
                             : "a"(1), "c"(0));
	if ((d & (1u << 26)) != 0)
		caps |= JIT_SIMD_CAP_SSE2;
	if ((c & (1u << 0)) != 0)
		caps |= JIT_SIMD_CAP_SSE3;
	if ((c & (1u << 19)) != 0)
		caps |= JIT_SIMD_CAP_SSE41;
	return caps;
#elif defined(_MSC_VER)
        int regs[4];
        uint32_t caps;

        caps = 0;
        if (!jit_x86_has_cpuid())
                return 0;
        __cpuid(regs, 0);
        if (regs[0] < 1)
                return 0;
        __cpuid(regs, 1);
        if (((uint32_t)regs[3] & (1u << 26)) != 0)
                caps |= JIT_SIMD_CAP_SSE2;
	if (((uint32_t)regs[2] & (1u << 0)) != 0)
		caps |= JIT_SIMD_CAP_SSE3;
        if (((uint32_t)regs[2] & (1u << 19)) != 0)
                caps |= JIT_SIMD_CAP_SSE41;
        return caps;
#else
        return 0;
#endif
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
        struct rt_jit_context *ctx,
        uint8_t b)
{
        if ((uint8_t *)ctx->code + 1 > (uint8_t *)ctx->code_end) {
		ctx->code_overflow = true;
                rt_error(ctx->env, N_TR("Code too big."));
                return false;
        }

        *(uint8_t *)ctx->code = b;
        ctx->code = (uint8_t *)ctx->code + 1;
        return true;
}

/* Put a instruction word. */
#define IW(b)                        if (!jit_put_word(ctx, w)) return false
static INLINE bool
jit_put_word(
        struct rt_jit_context *ctx,
        uint16_t w)
{
        if ((uint8_t *)ctx->code + 2 > (uint8_t *)ctx->code_end) {
		ctx->code_overflow = true;
                rt_error(ctx->env, N_TR("Code too big."));
                return false;
        }

        *(uint8_t *)ctx->code = (uint8_t)(w & 0xff);
        ctx->code = (uint8_t *)ctx->code + 1;

        *(uint8_t *)ctx->code = (uint8_t)((w >> 8) & 0xff);
        ctx->code = (uint8_t *)ctx->code + 1;

        return true;
}

/* Put a instruction double word. */
#define ID(d)                        if (!jit_put_dword(ctx, d)) return false
static INLINE bool
jit_put_dword(
        struct rt_jit_context *ctx,
        uint32_t dw)
{
        if ((uint8_t *)ctx->code + 4 > (uint8_t *)ctx->code_end) {
		ctx->code_overflow = true;
                rt_error(ctx->env, N_TR("Code too big."));
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

/*
 * Templates
 */

#define ASM_BINARY_OP(f)                                                         \
        /* if (!f(env, dst, src1, src2)) return false; */                        \
        ASM {                                                                    \
                /* ebp-4: &env->frame->tmpvar[0] */                              \
                /* ebp-8: env */                                                 \
                /* ebp-12: exception_handler */                                  \
                                                                                 \
                /* movl $src2, %eax */          IB(0xb8); ID((uint32_t)src2);    \
                /* pushl %eax */                IB(0x50);                        \
                                                                                 \
                /* movl $src1, %eax */          IB(0xb8); ID((uint32_t)src1);    \
                /* pushl %eax */                IB(0x50);                        \
                                                                                 \
                /* movl $dst, %eax */           IB(0xb8); ID((uint32_t)dst);     \
                /* pushl %eax */                IB(0x50);                        \
                                                                                 \
                /* movl -8(%ebp), %eax */       IB(0x8b); IB(0x45); IB(0xf8);    \
                /* pushl %eax */                IB(0x50);                        \
                                                                                 \
                /* movl $f, %eax */             IB(0xb8); ID((uint32_t)f);       \
                /* call *%eax */                IB(0xff); IB(0xd0);              \
                /* addl $16, %esp */            IB(0x83); IB(0xc4); IB(16);      \
                                                                                 \
                /* cmpl $0, %eax */             IB(0x83); IB(0xf8); IB(0x00);    \
                /* jne next */                  IB(0x75); IB(0x03);              \
                /* jmp -12(%ebp) */             IB(0xff); IB(0x65); IB(0xf4);    \
        /* next:*/                                                               \
        }

#define ASM_UNARY_OP(f)                                                          \
        /* if (!f(env, dst, src)) return false; */                               \
        ASM {                                                                    \
                /* ebp-4: &env->frame->tmpvar[0] */                               \
                /* ebp-8: env */                                                  \
                /* ebp-12: exception_handler */                                  \
                                                                                 \
                /* movl $src, %eax */           IB(0xb8); ID((uint32_t)src);     \
                /* push %eax */                 IB(0x50);                        \
                                                                                 \
                /* movl $dst, %eax */           IB(0xb8); ID((uint32_t)dst);     \
                /* pushl %eax */                IB(0x50);                        \
                                                                                 \
                /* movl -8(%ebp), %eax */       IB(0x8b); IB(0x45); IB(0xf8);    \
                /* pushl %eax */                IB(0x50);                        \
                                                                                 \
                /* movl $f, %eax */             IB(0xb8); ID((uint32_t)f);       \
                /* call *%eax */                IB(0xff); IB(0xd0);              \
                /* addl $12, %esp */            IB(0x83); IB(0xc4); IB(12);      \
                                                                                 \
                /* cmpl $0, %eax */             IB(0x83); IB(0xf8); IB(0x00);    \
                /* jne next */                  IB(0x75); IB(0x03);              \
                /* jmp -12(%ebp) */             IB(0xff); IB(0x65); IB(0xf4);    \
        /* next:*/                                                               \
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

        /* env->line = line; */
        ASM {
                /* ebp-4: &env->frame->tmpvar[0] */
                /* ebp-8: env */
                /* ebp-12: exception_handler */

                /* movl $line, %eax */          IB(0xb8); ID(line);
                /* movl -8(%ebp), %ebx */       IB(0x8b); IB(0x5d); IB(0xf8);
                /* movl %eax, 4(%ebx) */        IB(0x89); IB(0x43); IB(0x04);
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
                /* ebp-4: &env->frame->tmpvar[0] */
                /* ebp-8: env */
                /* ebp-12: exception_handler */

                /* movl $dst, %eax */            IB(0xb8); ID((uint32_t)dst);
                /* movl $src, %ebx */            IB(0xbb); ID((uint32_t)src);
                /* addl -4(%ebp), %eax */        IB(0x03); IB(0x45); IB(0xfc);
                /* addl -4(%ebp), %ebx */        IB(0x03); IB(0x5d); IB(0xfc);
                /* movl (%ebx), %ecx */          IB(0x8b); IB(0x0b);
                /* movl 8(%ebx), %edx */         IB(0x8b); IB(0x53); IB(0x08);
                /* movl %ecx, (%eax) */          IB(0x89); IB(0x08);
                /* movl %edx, 8(%eax) */         IB(0x89); IB(0x50); IB(0x08);
                /* The value union is 8 bytes: long/double live in
                   +8..+15, so the high word must be copied too (it
                   was dropped, truncating every long/double copied
                   through OP_ASSIGN on this backend). */
                /* movl 12(%ebx), %edx */        IB(0x8b); IB(0x53); IB(0x0c);
                /* movl %edx, 12(%eax) */        IB(0x89); IB(0x50); IB(0x0c);
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

        /* env->frame->tmpvar[dst].type = NOCT_VALUE_INT; */
        /* env->frame->tmpvar[dst].val.i = val; */
        ASM {
                /* ebp-4: &env->frame->tmpvar[0] */
                /* ebp-8: env */
                /* ebp-12: exception_handler */

                /* %eax = &env->frame->tmpvar[dst] */
                /* movl $dst, %eax */          IB(0xb8); ID((uint32_t)dst);
                /* addl -4(%ebp), %eax */      IB(0x03); IB(0x45); IB(0xfc);

                /* env->frame->tmpvar[dst].type = NOCT_VALUE_INT */
                /* movl $0, (%eax) */          IB(0xc7); IB(0x00); ID(0);

                /* env->frame->tmpvar[dst].val.i = val */
                /* movl $val, 8(%eax) */       IB(0xc7); IB(0x40); IB(0x08); ID(val);
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

        /* env->frame->tmpvar[dst].type = NOCT_VALUE_LONG; */
        /* env->frame->tmpvar[dst].val.l = val; */
        ASM {
                /* ebp-4: &env->frame->tmpvar[0] */
                /* ebp-8: env */
                /* ebp-12: exception_handler */

                /* %eax = &env->frame->tmpvar[dst] */
                /* movl $dst -> %eax */          IB(0xb8); ID((uint32_t)dst);
                /* addl -4(%ebp) -> %eax */      IB(0x03); IB(0x45); IB(0xfc);

                /* env->frame->tmpvar[dst].type = NOCT_VALUE_LONG */
                /* movl $5 -> (%eax) */          IB(0xc7); IB(0x00); ID(5);

                /* env->frame->tmpvar[dst].val.i = val */
                /* movl LO(val) -> 8(%eax) */    IB(0xc7); IB(0x40); IB(0x08); ID((uint32_t)val);
                /* movl HI(val) -> 12(%eax) */   IB(0xc7); IB(0x40); IB(0x0c); ID((uint32_t)(val >> 32));
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

        /* &env->frame->tmpvar[dst].type = NOCT_VALUE_FLOAT; */
        /* &env->frame->tmpvar[dst].val.i = val; */
        ASM {
                /* ebp-4: &env->frame->tmpvar[0] */
                /* ebp-8: env */
                /* ebp-12: exception_handler */

                /* %eax = &env->frame->tmpvar[dst] */
                /* movl $dst -> %eax */          IB(0xb8); ID((uint32_t)dst);
                /* addl -4(%ebp) -> %eax */      IB(0x03); IB(0x45); IB(0xfc);

                /* env->frame->tmpvar[dst].type = NOCT_VALUE_DOUBLE */
                /* movl $1 -> (%eax) */          IB(0xc7); IB(0x00); ID(1);

                /* env->frame->tmpvar[dst].val.i = val */
                /* movl $val -> 8(%eax) */       IB(0xc7); IB(0x40); IB(0x08); ID(val);
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

        /* env->frame->tmpvar[dst].type = NOCT_VALUE_DOUBLE; */
        /* env->frame->tmpvar[dst].val.l = val; */
        ASM {
                /* ebp-4: &env->frame->tmpvar[0] */
                /* ebp-8: env */
                /* ebp-12: exception_handler */

                /* %eax = &env->frame->tmpvar[dst] */
                /* movl $dst -> %eax */          IB(0xb8); ID((uint32_t)dst);
                /* addl -4(%ebp) -> %eax */      IB(0x03); IB(0x45); IB(0xfc);

                /* env->frame->tmpvar[dst].type = NOCT_VALUE_DOUBLE */
                /* movl $6 -> (%eax) */          IB(0xc7); IB(0x00); ID(6);

                /* env->frame->tmpvar[dst].val.i = val */
                /* movl LO(val) -> 8(%eax) */    IB(0xc7); IB(0x40); IB(0x08); ID((uint32_t)val);
                /* movl HI(val) -> 12(%eax) */   IB(0xc7); IB(0x40); IB(0x0c); ID((uint32_t)(val >> 32));
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

        /* rt_make_string_with_hash(env, &env->frame->tmpvar[dst], val, len, hash); */
        ASM {
                /* ebp-4: &env->frame->tmpvar[0] */
                /* ebp-8: env */
                /* ebp-12: exception_handler */

                /* movl $hash, %eax */            IB(0xb8); ID(hash);
                /* pushl %eax */                  IB(0x50);

                /* movl $len, %eax */             IB(0xb8); ID(len);
                /* pushl %eax */                  IB(0x50);

                /* movl $val, %eax */             IB(0xb8); ID((uint32_t)val);
                /* pushl %eax */                  IB(0x50);

                /* movl $dst, %eax */             IB(0xb8); ID((uint32_t)dst);
                /* addl -4(%ebp), %eax */         IB(0x03); IB(0x45); IB(0xfc);
                /* pushl %eax */                  IB(0x50);

                /* movl -8(%ebp), %eax */         IB(0x8b); IB(0x45); IB(0xf8);
                /* pushl %eax */                  IB(0x50);

                /* movl $ex_make_string_with_hash, %eax */  IB(0xb8); ID((uint32_t)ex_make_string_with_hash);
                /* call *%eax */                  IB(0xff); IB(0xd0);
                /* addl $20, %esp */              IB(0x83); IB(0xc4); IB(20);

                /* cmpl $0, %eax */               IB(0x83); IB(0xf8); IB(0x00);
                /* jne next */                    IB(0x75); IB(0x03);
                /* jmp -12(%ebp) */               IB(0xff); IB(0x65); IB(0xf4);
                /* next:*/
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

        /* rt_make_empty_array(env, &env->frame->tmpvar[dst]); */
        ASM {
                /* ebp-4: &env->frame->tmpvar[0] */
                /* ebp-8: env */
                /* ebp-12: exception_handler */

                /* movl $dst, %eax */                   IB(0xb8); ID((uint32_t)dst);
                /* addl -4(%ebp), %eax */               IB(0x03); IB(0x45); IB(0xfc);
                /* pushl %eax */                        IB(0x50);

                /* movl -8(%ebp), %eax */               IB(0x8b); IB(0x45); IB(0xf8);
                /* pushl %eax */                        IB(0x50);

                /* movl $ex_make_empty_array, %eax */   IB(0xb8); ID((uint32_t)ex_make_empty_array);
                /* call *%eax */                        IB(0xff); IB(0xd0);
                /* addl $8, %esp */                     IB(0x83); IB(0xc4); IB(8);

                /* cmpl $0, %eax */                     IB(0x83); IB(0xf8); IB(0x00);
                /* jne next */                          IB(0x75); IB(0x03);
                /* jmp -12(%ebp) */                     IB(0xff); IB(0x65); IB(0xf4);
                /* next:*/
        /* next: */
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
                /* ebp-4: &env->frame->tmpvar[0] */
                /* ebp-8: env */
                /* ebp-12: exception_handler */

                /* movl $dst, %eax */                   IB(0xb8); ID((uint32_t)dst);
                /* addl -4(%ebp), %eax */               IB(0x03); IB(0x45); IB(0xfc);
                /* pushl %eax */                        IB(0x50);

                /* movl -8(%ebp), %eax */               IB(0x8b); IB(0x45); IB(0xf8);
                /* pushl %eax */                        IB(0x50);

                /* movl $ex_make_empty_dict, %eax */    IB(0xb8); ID((uint32_t)ex_make_empty_dict);
                /* call *%eax */                        IB(0xff); IB(0xd0);
                /* addl $8, %esp */                     IB(0x83); IB(0xc4); IB(8);

                /* cmpl $0, %eax */                     IB(0x83); IB(0xf8); IB(0x00);
                /* jne next */                          IB(0x75); IB(0x03);
                /* jmp -12(%ebp) */                     IB(0xff); IB(0x65); IB(0xf4);
                /* next:*/
        /* next: */
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

        /* env->frame->tmpvar[dst].val.i++ */
        ASM {
                /* ebp-4: &env->frame->tmpvar[0] */
                /* ebp-8: env */
                /* ebp-12: exception_handler */

                /* movl $dst, %eax */                   IB(0xb8); ID((uint32_t)dst);
                /* addl -4(%ebp), %eax */               IB(0x03); IB(0x45); IB(0xfc);
                /* addl $step, 8(%eax) */               IB(0x83); IB(0x40); IB(0x08); IB((uint8_t)step);
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

	CONSUME_TMPVAR(a); CONSUME_TMPVAR(b); CONSUME_TMPVAR(c);
	CONSUME_IMM8(required_vregs); CONSUME_IMM8(lanes); CONSUME_IMM8(flags);
	if (required_vregs > 8)
		ctx->simd_caps = 0;
	return true;
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
		IB(0xb8); ID((uint32_t)value);
		IB(0x03); IB(0x45); IB(0xfc);
		IB(0x83); IB(0x68); IB(0x08); IB((uint8_t)decrement);
	}
	ctx->branch_patch[ctx->branch_patch_count].code = ctx->code;
	ctx->branch_patch[ctx->branch_patch_count].lpc = target_lpc;
	ctx->branch_patch[ctx->branch_patch_count].type = PATCH_JNE;
	ctx->branch_patch_count++;
	ASM { IB(0x0f); IB(0x85); ID(0); }
	return true;
}

#if defined(NOCT_USE_OPTIMIZER)
static INLINE bool
jit_visit_vori32x4i_op(struct rt_jit_context *ctx)
{
	int dst, src, imm, shift;
	uint32_t value;
	int k;
	int src1, src2;

	CONSUME_IMM8(dst); CONSUME_IMM8(src);
	CONSUME_IMM8(imm); CONSUME_IMM8(shift);
	if ((ctx->simd_caps & JIT_SIMD_CAP_SSE2) == 0) {
		src1 = src;
		src2 = (imm << 8) | shift;
		ASM_BINARY_OP(ex_vori32x4i_helper);
		return true;
	}
	if (dst != src) {
		ASM { IB(0x66); IB(0x0f); IB(0x6f); IB((uint8_t)(0xc0 | (dst << 3) | src)); }
	}
	value = (uint32_t)imm << ((uint32_t)shift & 31);
	ASM { IB(0x83); IB(0xec); IB(0x10); }
	for (k = 0; k < 4; k++) {
		if (k == 0) {
			ASM { IB(0xc7); IB(0x04); IB(0x24); ID(value); }
		} else {
			ASM { IB(0xc7); IB(0x44); IB(0x24); IB((uint8_t)(k * 4)); ID(value); }
		}
	}
	ASM {
		IB(0x66); IB(0x0f); IB(0xeb); IB((uint8_t)(0x04 | (dst << 3))); IB(0x24);
		IB(0x83); IB(0xc4); IB(0x10);
	}
	return true;
}
#endif

#if defined(NOCT_USE_OPTIMIZER)
static INLINE bool
jit_visit_vfmaf32x4_op(struct rt_jit_context *ctx)
{
	int dst, src1, vb, vc, src2;

	CONSUME_IMM8(dst);
	CONSUME_IMM8(src1);
	CONSUME_IMM8(vb);
	CONSUME_IMM8(vc);
	src2 = (vb << 8) | vc;
	ASM_BINARY_OP(ex_vfmaf32x4_helper);
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

        /* if (!rt_shr_helper(env, dst, src1, src2)) return false; */
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

        /* Set EFLAGS by (src2 - src1) */
        ASM {
                /* ebp-4: &env->frame->tmpvar[0] */
                /* ebp-8: env */
                /* ebp-12: exception_handler */

                /* movl $src1, %eax */           IB(0xb8); ID((uint32_t)src1);
                /* addl -4(%ebp), %eax */        IB(0x03); IB(0x45); IB(0xfc);

                /* movl $src2, %ebx */           IB(0xbb); ID((uint32_t)src2);
                /* addl -4(%ebp), %ebx */        IB(0x03); IB(0x5d); IB(0xfc);

                /* movl 8(%eax), %ecx */         IB(0x8b); IB(0x48); IB(0x08);
                /* movl 8(%ebx), %edx */         IB(0x8b); IB(0x53); IB(0x08);
                /* cmpl %ecx, %edx */            IB(0x39); IB(0xca);
        }

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

        /* if (!rt_storearray_helper(env, dst, src1, src2)) return false; */
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

        /* if (!rt_len_helper(env, dst, src)) return false; */
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

        /* if (!rt_getdictkeybyindex_helper(env, dst, src1, src2)) return false; */
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

        /* if (!rt_getdictvalbyindex_helper(env, dst, src1, src2)) return false; */
        ASM_BINARY_OP(ex_getdictvalbyindex_helper);

        return true;
}

/* Visit a OP_LOADSYMBOL instruction. */
static INLINE bool
jit_visit_loadsymbol_op(
        struct rt_jit_context *ctx)
{
        int dst;
        const char *src;
        uint32_t len, hash;

        CONSUME_TMPVAR(dst);
        CONSUME_STRING(src, len, hash);

        /* if (!rt_loadsymbol_helper(env, dst, src, len, hash)) return false; */
        ASM {
                /* ebp-4: &env->frame->tmpvar[0] */
                /* ebp-8: env */
                /* ebp-12: exception_handler */

                /* movl $hash, %eax */                  IB(0xb8); ID(hash);
                /* push %eax */                         IB(0x50);

                /* movl len, %eax */                    IB(0xb8); ID(len);
                /* push %eax */                         IB(0x50);

                /* movl $src, %eax */                   IB(0xb8); ID((uint32_t)src);
                /* push %eax */                         IB(0x50);

                /* movl $dst, %eax */                   IB(0xb8); ID((uint32_t)dst);
                /* pushl %eax */                        IB(0x50);

                /* movl -8(%ebp), %eax */               IB(0x8b); IB(0x45); IB(0xf8);
                /* pushl %eax */                        IB(0x50);

                /* movl $ex_loadsymbol_helper, %eax */  IB(0xb8); ID((uint32_t)ex_loadsymbol_helper);
                /* call *%eax */                        IB(0xff); IB(0xd0);
                /* addl $20, %esp */                    IB(0x83); IB(0xc4); IB(20);

                /* cmpl $0, %eax */                     IB(0x83); IB(0xf8); IB(0x00);
                /* jne next */                          IB(0x75); IB(0x03);
                /* jmp -12(%ebp) */                     IB(0xff); IB(0x65); IB(0xf4);
        /* next:*/
        }

        return true;
}

/* Visit a OP_STORESYMBOL instruction. */
static INLINE bool
jit_visit_storesymbol_op(
        struct rt_jit_context *ctx)
{
        const char *dst;
        uint32_t len, hash;
        int src;

        CONSUME_STRING(dst, len, hash);
        CONSUME_TMPVAR(src);

        /* if (!rt_storesymbol_helper(env, dst, len, hash, src)) return false; */
        ASM {
                /* ebp-4: &env->frame->tmpvar[0] */
                /* ebp-8: env */
                /* ebp-12: exception_handler */

                /* movl $src, %eax */                    IB(0xb8); ID((uint32_t)src);
                /* push %eax */                          IB(0x50);

                /* movl $hash, %eax */                   IB(0xb8); ID(hash);
                /* pushl %eax */                         IB(0x50);

                /* movl $len, %eax */                    IB(0xb8); ID(len);
                /* pushl %eax */                         IB(0x50);

                /* movl $dst, %eax */                    IB(0xb8); ID((uint32_t)dst);
                /* pushl %eax */                         IB(0x50);

                /* movl -8(%ebp), %eax */                IB(0x8b); IB(0x45); IB(0xf8);
                /* pushl %eax */                         IB(0x50);

                /* movl $ex_storesymbol_helper, %eax */  IB(0xb8); ID((uint32_t)ex_storesymbol_helper);
                /* call *%eax */                         IB(0xff); IB(0xd0);
                /* addl $20, %esp */                     IB(0x83); IB(0xc4); IB(20);

                /* cmpl $0, %eax */                      IB(0x83); IB(0xf8); IB(0x00);
                /* jne next */                           IB(0x75); IB(0x03);
                /* jmp -12(%ebp) */                      IB(0xff); IB(0x65); IB(0xf4);
        /* next:*/
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
        const char *field;
        uint32_t len, hash;

        CONSUME_TMPVAR(dst);
        CONSUME_TMPVAR(dict);
        CONSUME_STRING(field, len, hash);

        /* if (!rt_loaddot_helper(env, dst, dict, field, len, hash)) return false; */
        ASM {
                /* ebp-4: &env->frame->tmpvar[0] */
                /* ebp-8: env */
                /* ebp-12: exception_handler */

                /* movl $hash, %eax */               IB(0xb8); ID(hash);
                /* push %eax */                      IB(0x50);

                /* movl $len, %eax */                IB(0xb8); ID(len);
                /* push %eax */                      IB(0x50);

                /* movl $field, %eax */              IB(0xb8); ID((uint32_t)field);
                /* push %eax */                      IB(0x50);

                /* movl $dict, %eax */               IB(0xb8); ID((uint32_t)dict);
                /* push %eax */                      IB(0x50);

                /* movl $dst, %eax */                IB(0xb8); ID((uint32_t)dst);
                /* pushl %eax */                     IB(0x50);

                /* movl -8(%ebp), %eax */            IB(0x8b); IB(0x45); IB(0xf8);
                /* pushl %eax */                     IB(0x50);

                /* movl $ex_loaddot_helper, %eax */  IB(0xb8); ID((uint32_t)ex_loaddot_helper);
                /* call *%eax */                     IB(0xff); IB(0xd0);
                /* addl $24, %esp */                 IB(0x83); IB(0xc4); IB(24);

                /* cmpl $0, %eax */                  IB(0x83); IB(0xf8); IB(0x00);
                /* jne next */                       IB(0x75); IB(0x03);
                /* jmp -12(%ebp) */                  IB(0xff); IB(0x65); IB(0xf4);
        /* next:*/
        }

        return true;
}

/* Visit a OP_STOREDOT instruction. */
static INLINE bool
jit_visit_storedot_op(
        struct rt_jit_context *ctx)
{
        int dict, src;
        const char *field;
        uint32_t len, hash;

        CONSUME_TMPVAR(dict);
        CONSUME_STRING(field, len, hash);
        CONSUME_TMPVAR(src);

        /* if (!jit_storedot_helper(env, dict, field, len, hash, src)) return false; */
        ASM {
                /* ebp-4: &env->frame->tmpvar[0] */
                /* ebp-8: env */
                /* ebp-12: exception_handler */

                /* movl $src, %eax */                   IB(0xb8); ID((uint32_t)src);
                /* push %eax */                         IB(0x50);

                /* movl $hash, %eax */                  IB(0xb8); ID(hash);
                /* push %eax */                         IB(0x50);

                /* movl $len, %eax */                   IB(0xb8); ID(len);
                /* push %eax */                         IB(0x50);

                /* movl $field, %eax */                 IB(0xb8); ID((uint32_t)field);
                /* push %eax */                         IB(0x50);

                /* movl $dict, %eax */                  IB(0xb8); ID((uint32_t)dict);
                /* push %eax */                         IB(0x50);

                /* movl -8(%ebp), %eax */               IB(0x8b); IB(0x45); IB(0xf8);
                /* pushl %eax */                        IB(0x50);

                /* movl $ex_storedot_helper, %eax */    IB(0xb8); ID((uint32_t)ex_storedot_helper);
                /* call *%eax */                        IB(0xff); IB(0xd0);
                /* addl $24, %esp */                    IB(0x83); IB(0xc4); IB(24);

                /* cmpl $0, %eax */                     IB(0x83); IB(0xf8); IB(0x00);
                /* jne next */                          IB(0x75); IB(0x03);
                /* jmp -12(%ebp) */                     IB(0xff); IB(0x65); IB(0xf4);
        /* next:*/
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
                        /* jmp (5 + arg_count * 4) */
                        IB(0xe9);
                        ID((uint32_t)(4 * arg_count));
                }
                arg_addr = (uint32_t)(intptr_t)ctx->code;
                for (i = 0; i < arg_count; i++) {
                        *(int *)ctx->code = arg[i];
                        ctx->code = (uint8_t *)ctx->code + 4;
                }
        } else {
                arg_addr = 0;
        }

        /* if (!rt_call_helper(env, dst, func, arg_count, arg)) return false; */
        ASM {
                /* ebp-4: &env->frame->tmpvar[0] */
                /* ebp-8: env */
                /* ebp-12: exception_handler */

                /* movl $arg_addr, %eax */       IB(0xb8); ID(arg_addr);
                /* pushl %eax */                 IB(0x50);

                /* movl $arg_count, %eax */      IB(0xb8); ID((uint32_t)arg_count);
                /* pushl %eax */                 IB(0x50);

                /* movl func, %eax */            IB(0xb8); ID((uint32_t)func);
                /* pushl %eax */                 IB(0x50);

                /* movl dst, %eax */             IB(0xb8); ID((uint32_t)dst);
                /* pushl %eax */                 IB(0x50);

                /* movl -8(%ebp), %eax */        IB(0x8b); IB(0x45); IB(0xf8);
                /* pushl %eax */                 IB(0x50);

                /* movl $ex_call_helper, %eax */ IB(0xb8); ID((uint32_t)ex_call_helper);
                /* call *%eax */                 IB(0xff); IB(0xd0);
                /* addl $20, %esp */             IB(0x83); IB(0xc4); IB(20);

                /* cmpl $0, %eax */              IB(0x83); IB(0xf8); IB(0x00);
                /* jne next */                   IB(0x75); IB(0x03);
                /* jmp -12(%ebp) */              IB(0xff); IB(0x65); IB(0xf4);
                /* next:*/
        }
        
        return true;
}

/* Visit a OP_THISCALL instruction. */
static inline bool
jit_visit_thiscall_op(
        struct rt_jit_context *ctx)
{
        int dst, obj, arg_count, arg_tmp, i;
        const char *symbol;
        uint32_t len, hash;
        int arg[NOCT_ARG_MAX];
        uint32_t arg_addr;

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
        ASM {
                /* jmp (5 + arg_count * 4) */
                IB(0xe9);
                ID((uint32_t)(4 * arg_count));
        }
        arg_addr = (uint32_t)(intptr_t)ctx->code;
        for (i = 0; i < arg_count; i++) {
                *(int *)ctx->code = arg[i];
                ctx->code = (uint8_t *)ctx->code + 4;
        }

        /* if (!rt_thiscall_helper(env, dst, obj, symbol, len, hash, arg_count, arg)) return false; */
        ASM {
                /* ebp-4: &env->frame->tmpvar[0] */
                /* ebp-8: env */
                /* ebp-12: exception_handler */

                /* movl $arg_addr, %eax */              IB(0xb8); ID(arg_addr);
                /* pushl %eax */                        IB(0x50);

                /* movl $arg_count, %eax */             IB(0xb8); ID((uint32_t)arg_count);
                /* pushl %eax */                        IB(0x50);

                /* movl hash, %eax */                   IB(0xb8); ID(hash);
                /* pushl %eax */                        IB(0x50);

                /* movl $len, %eax */                   IB(0xb8); ID(len);
                /* pushl %eax */                        IB(0x50);

                /* movl $symbol, %eax */                IB(0xb8); ID((uint32_t)symbol);
                /* pushl %eax */                        IB(0x50);

                /* movl obj, %eax */                    IB(0xb8); ID((uint32_t)obj);
                /* pushl %eax */                        IB(0x50);

                /* movl dst, %eax */                    IB(0xb8); ID((uint32_t)dst);
                /* pushl %eax */                        IB(0x50);

                /* movl -8(%ebp), %eax */               IB(0x8b); IB(0x45); IB(0xf8);
                /* pushl %eax */                        IB(0x50);

                /* movl $ex_thiscall_helper, %eax */    IB(0xb8); ID((uint32_t)ex_thiscall_helper);
                /* call *%eax */                        IB(0xff); IB(0xd0);
                /* addl $32, %esp */                    IB(0x83); IB(0xc4); IB(32);

                /* cmpl $0, %eax */                     IB(0x83); IB(0xf8); IB(0x00);
                /* jne next */                          IB(0x75); IB(0x03);
                /* jmp -12(%ebp) */                     IB(0xff); IB(0x65); IB(0xf4);
                /* next:*/
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

        src *= (int)sizeof(struct rt_value);

        ASM {
                /* ebp-4: &env->frame->tmpvar[0] */
                /* ebp-8: env */
                /* ebp-12: exception_handler */

                /* movl $src, %eax */           IB(0xb8); ID((uint32_t)src);
                /* addl -4(%ebp), %eax */       IB(0x03); IB(0x45); IB(0xfc);
                /* movl 8(%eax), %eax */        IB(0x8b); IB(0x40); IB(0x08);

                /* Compare: env->frame->tmpvar[dst].val.i == 0 */
                /* cmpl $0, %eax */             IB(0x83); IB(0xf8); IB(0x00);
        }
        
        /* Patch later. */
        ctx->branch_patch[ctx->branch_patch_count].code = ctx->code;
        ctx->branch_patch[ctx->branch_patch_count].lpc = target_lpc;
        ctx->branch_patch[ctx->branch_patch_count].type = PATCH_JNE;
        ctx->branch_patch_count++;

        ASM {
                /* Patched later. */
                /* jne 6 */                     IB(0x0f); IB(0x84); ID(0);
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

        src *= (int)sizeof(struct rt_value);

        ASM {
                /* ebp-4: &env->frame->tmpvar[0] */
                /* ebp-8: env */
                /* ebp-12: exception_handler */

                /* movl $src, %eax */           IB(0xb8); ID((uint32_t)src);
                /* addl -4(%ebp), %eax */       IB(0x03); IB(0x45); IB(0xfc);
                /* movl 8(%eax), %eax */        IB(0x8b); IB(0x40); IB(0x08);

                /* Compare: env->frame->tmpvar[dst].val.i == 0 */
                /* cmpl $0, %eax */             IB(0x83); IB(0xf8); IB(0x00);
        }
        
        /* Patch later. */
        ctx->branch_patch[ctx->branch_patch_count].code = ctx->code;
        ctx->branch_patch[ctx->branch_patch_count].lpc = target_lpc;
        ctx->branch_patch[ctx->branch_patch_count].type = PATCH_JE;
        ctx->branch_patch_count++;

        ASM {
                /* Patched later. */
                /* je 6 */                      IB(0x0f); IB(0x85); ID(0);
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
        ctx->branch_patch[ctx->branch_patch_count].type = PATCH_JE;
        ctx->branch_patch_count++;

        ASM {
                /* Patched later. */
                /* je 6 */                      IB(0x0f); IB(0x84); ID(0);
        }

        return true;
}

/* Visit a OP_SAFEPOINT instruction. */
static INLINE bool
jit_visit_safepoint_op(
        struct rt_jit_context *ctx)
{
        int dst;
        int dict;
        const char *field;
        uint32_t len, hash;

        CONSUME_TMPVAR(dst);
        CONSUME_TMPVAR(dict);
        CONSUME_STRING(field, len, hash);

        /* if (!rt_safepoint_helper(env)) return false; */
        ASM {
                /* ebp-4: &env->frame->tmpvar[0] */
                /* ebp-8: env */
                /* ebp-12: exception_handler */

                /* movl -8(%ebp), %eax */            IB(0x8b); IB(0x45); IB(0xf8);
                /* pushl %eax */                     IB(0x50);

                /* movl $ex_safepoint_helper, %eax */IB(0xb8); ID((uint32_t)ex_safepoint_helper);
                /* call *%eax */                     IB(0xff); IB(0xd0);
                /* addl $4, %esp */                  IB(0x83); IB(0xc4); IB(4);

                /* cmpl $0, %eax */                  IB(0x83); IB(0xf8); IB(0x00);
                /* jne next */                       IB(0x75); IB(0x03);
                /* jmp -12(%ebp) */                  IB(0xff); IB(0x65); IB(0xf4);
        /* next:*/
        }

        return true;
}

/* Visit a OP_PBASE instruction. (ABCE; inline machine code, i386.)
 * The guard has proven the operand is a packed. */
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
                /* ebp-4: &env->frame->tmpvar[0] */

                /* movl -4(%ebp) -> %ebx */        IB(0x8b); IB(0x5d); IB(0xfc);
                /* movl src+8(%ebx) -> %eax */     IB(0x8b); IB(0x83); ID((uint32_t)(src + 8));
                /* movl buf_ofs(%eax) -> %eax */   IB(0x8b); IB(0x80); ID(buf_ofs);
                /* movl $LONG -> dst(%ebx) */      IB(0xc7); IB(0x83); ID((uint32_t)dst); ID((uint32_t)NOCT_VALUE_LONG);
                /* movl %eax -> dst+8(%ebx) */     IB(0x89); IB(0x83); ID((uint32_t)(dst + 8));
                /* movl $0 -> dst+12(%ebx) */      IB(0xc7); IB(0x83); ID((uint32_t)(dst + 12)); ID(0);
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

/* Visit a OP_PLOAD8U instruction. (ABCE; inline machine code, i386.) */
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
                /* ebp-4: &env->frame->tmpvar[0] */

                /* movl -4(%ebp) -> %ebx */        IB(0x8b); IB(0x5d); IB(0xfc);
                /* movl base+8(%ebx) -> %eax */    IB(0x8b); IB(0x83); ID((uint32_t)(base + 8));
                /* movl ofs+8(%ebx) -> %ecx */     IB(0x8b); IB(0x8b); ID((uint32_t)(ofs + 8));
                /* load scaled -> %edx */          IB(0x0f); IB(0xb6); IB(0x14); IB(0x08);
                /* movl $INT -> dst(%ebx) */       IB(0xc7); IB(0x83); ID((uint32_t)dst); ID((uint32_t)NOCT_VALUE_INT);
                /* movl %edx -> dst+8(%ebx) */     IB(0x89); IB(0x93); ID((uint32_t)(dst + 8));
        }

        return true;
}

/* Visit a OP_PSTORE8 instruction. (ABCE; inline, i386. Int source per ABCE rules.) */
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
                /* ebp-4: &env->frame->tmpvar[0] */

                /* movl -4(%ebp) -> %ebx */        IB(0x8b); IB(0x5d); IB(0xfc);
                /* movl base+8(%ebx) -> %eax */    IB(0x8b); IB(0x83); ID((uint32_t)(base + 8));
                /* movl ofs+8(%ebx) -> %ecx */     IB(0x8b); IB(0x8b); ID((uint32_t)(ofs + 8));
                /* movl src+8(%ebx) -> %edx */     IB(0x8b); IB(0x93); ID((uint32_t)(src + 8));
                /* store scaled */                 IB(0x88); IB(0x14); IB(0x08);
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

/* Visit a OP_PLOAD8S instruction. (ABCE; inline machine code, i386.) */
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
                /* ebp-4: &env->frame->tmpvar[0] */

                /* movl -4(%ebp) -> %ebx */        IB(0x8b); IB(0x5d); IB(0xfc);
                /* movl base+8(%ebx) -> %eax */    IB(0x8b); IB(0x83); ID((uint32_t)(base + 8));
                /* movl ofs+8(%ebx) -> %ecx */     IB(0x8b); IB(0x8b); ID((uint32_t)(ofs + 8));
                /* load scaled -> %edx */          IB(0x0f); IB(0xbe); IB(0x14); IB(0x08);
                /* movl $INT -> dst(%ebx) */       IB(0xc7); IB(0x83); ID((uint32_t)dst); ID((uint32_t)NOCT_VALUE_INT);
                /* movl %edx -> dst+8(%ebx) */     IB(0x89); IB(0x93); ID((uint32_t)(dst + 8));
        }

        return true;
}

/* Visit a OP_PLOAD16U instruction. (ABCE; inline machine code, i386.) */
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
                /* ebp-4: &env->frame->tmpvar[0] */

                /* movl -4(%ebp) -> %ebx */        IB(0x8b); IB(0x5d); IB(0xfc);
                /* movl base+8(%ebx) -> %eax */    IB(0x8b); IB(0x83); ID((uint32_t)(base + 8));
                /* movl ofs+8(%ebx) -> %ecx */     IB(0x8b); IB(0x8b); ID((uint32_t)(ofs + 8));
                /* load scaled -> %edx */          IB(0x0f); IB(0xb7); IB(0x14); IB(0x48);
                /* movl $INT -> dst(%ebx) */       IB(0xc7); IB(0x83); ID((uint32_t)dst); ID((uint32_t)NOCT_VALUE_INT);
                /* movl %edx -> dst+8(%ebx) */     IB(0x89); IB(0x93); ID((uint32_t)(dst + 8));
        }

        return true;
}

/* Visit a OP_PLOAD16S instruction. (ABCE; inline machine code, i386.) */
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
                /* ebp-4: &env->frame->tmpvar[0] */

                /* movl -4(%ebp) -> %ebx */        IB(0x8b); IB(0x5d); IB(0xfc);
                /* movl base+8(%ebx) -> %eax */    IB(0x8b); IB(0x83); ID((uint32_t)(base + 8));
                /* movl ofs+8(%ebx) -> %ecx */     IB(0x8b); IB(0x8b); ID((uint32_t)(ofs + 8));
                /* load scaled -> %edx */          IB(0x0f); IB(0xbf); IB(0x14); IB(0x48);
                /* movl $INT -> dst(%ebx) */       IB(0xc7); IB(0x83); ID((uint32_t)dst); ID((uint32_t)NOCT_VALUE_INT);
                /* movl %edx -> dst+8(%ebx) */     IB(0x89); IB(0x93); ID((uint32_t)(dst + 8));
        }

        return true;
}

/* Visit a OP_PLOAD32 instruction. (ABCE; inline machine code, i386.) */
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
                /* ebp-4: &env->frame->tmpvar[0] */

                /* movl -4(%ebp) -> %ebx */        IB(0x8b); IB(0x5d); IB(0xfc);
                /* movl base+8(%ebx) -> %eax */    IB(0x8b); IB(0x83); ID((uint32_t)(base + 8));
                /* movl ofs+8(%ebx) -> %ecx */     IB(0x8b); IB(0x8b); ID((uint32_t)(ofs + 8));
                /* load scaled -> %edx */          IB(0x8b); IB(0x14); IB(0x88);
                /* movl $INT -> dst(%ebx) */       IB(0xc7); IB(0x83); ID((uint32_t)dst); ID((uint32_t)NOCT_VALUE_INT);
                /* movl %edx -> dst+8(%ebx) */     IB(0x89); IB(0x93); ID((uint32_t)(dst + 8));
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

/* Visit a OP_PSTORE16 instruction. (ABCE; inline, i386. Int source per ABCE rules.) */
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
                /* ebp-4: &env->frame->tmpvar[0] */

                /* movl -4(%ebp) -> %ebx */        IB(0x8b); IB(0x5d); IB(0xfc);
                /* movl base+8(%ebx) -> %eax */    IB(0x8b); IB(0x83); ID((uint32_t)(base + 8));
                /* movl ofs+8(%ebx) -> %ecx */     IB(0x8b); IB(0x8b); ID((uint32_t)(ofs + 8));
                /* movl src+8(%ebx) -> %edx */     IB(0x8b); IB(0x93); ID((uint32_t)(src + 8));
                /* store scaled */                 IB(0x66); IB(0x89); IB(0x14); IB(0x48);
        }

        return true;
}

/* Visit a OP_PSTORE32 instruction. (ABCE; inline, i386. Int source per ABCE rules.) */
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
                /* ebp-4: &env->frame->tmpvar[0] */

                /* movl -4(%ebp) -> %ebx */        IB(0x8b); IB(0x5d); IB(0xfc);
                /* movl base+8(%ebx) -> %eax */    IB(0x8b); IB(0x83); ID((uint32_t)(base + 8));
                /* movl ofs+8(%ebx) -> %ecx */     IB(0x8b); IB(0x8b); ID((uint32_t)(ofs + 8));
                /* movl src+8(%ebx) -> %edx */     IB(0x8b); IB(0x93); ID((uint32_t)(src + 8));
                /* store scaled */                 IB(0x89); IB(0x14); IB(0x88);
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


static INLINE void
jit_x86_patch_local_rel32(uint8_t *disp, uint8_t *target)
{
	int32_t rel;

	rel = (int32_t)(target - (disp + 4));
	memcpy(disp, &rel, sizeof(rel));
}

/* Direct checked int32 division for i386.  ebx is the established tmpvar
 * base scratch used throughout this backend; the zero case alone calls the
 * checked helper. */
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
        int dst_ofs;
        int src1_ofs;
        int src2_ofs;
        uint8_t *zero_patch, *divide_patch[2], *store_patch, *done_patch;
        uint8_t *target;

        CONSUME_TMPVAR(dst);
        CONSUME_TMPVAR(src1);
        CONSUME_TMPVAR(src2);

        dst_ofs = dst * (int)sizeof(struct rt_value);
        src1_ofs = src1 * (int)sizeof(struct rt_value);
        src2_ofs = src2 * (int)sizeof(struct rt_value);
        ASM {
                IB(0x8b); IB(0x5d); IB(0xfc);  /* mov -4(ebp),ebx */
                IB(0x8b); IB(0x83); ID((uint32_t)(src1_ofs + 8));
                IB(0x8b); IB(0x8b); ID((uint32_t)(src2_ofs + 8));
                IB(0x85); IB(0xc9);
                IB(0x0f); IB(0x84);
        }
        zero_patch = (uint8_t *)ctx->code;
        if (!jit_put_dword(ctx, 0)) return false;
        ASM { IB(0x83); IB(0xf9); IB(0xff); IB(0x0f); IB(0x85); }
        divide_patch[0] = (uint8_t *)ctx->code;
        if (!jit_put_dword(ctx, 0)) return false;
        ASM { IB(0x3d); ID(0x80000000u); IB(0x0f); IB(0x85); }
        divide_patch[1] = (uint8_t *)ctx->code;
        if (!jit_put_dword(ctx, 0)) return false;

        ASM { IB(0xe9); }
        store_patch = (uint8_t *)ctx->code;
        if (!jit_put_dword(ctx, 0)) return false;

        target = (uint8_t *)ctx->code;
        jit_x86_patch_local_rel32(divide_patch[0], target);
        jit_x86_patch_local_rel32(divide_patch[1], target);
        ASM { IB(0x99); IB(0xf7); IB(0xf9); }

        target = (uint8_t *)ctx->code;
        jit_x86_patch_local_rel32(store_patch, target);
        ASM {
                IB(0xc7); IB(0x83); ID((uint32_t)dst_ofs);
                ID((uint32_t)NOCT_VALUE_INT);
                IB(0x89); IB(0x83); ID((uint32_t)(dst_ofs + 8));
                IB(0xe9);
        }
        done_patch = (uint8_t *)ctx->code;
        if (!jit_put_dword(ctx, 0)) return false;

        target = (uint8_t *)ctx->code;
        jit_x86_patch_local_rel32(zero_patch, target);
        ASM_BINARY_OP(ex_idiv_helper);
        target = (uint8_t *)ctx->code;
        jit_x86_patch_local_rel32(done_patch, target);
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
        uint8_t *zero_patch, *divide_patch[2], *store_patch, *done_patch;
        uint8_t *target;

        CONSUME_TMPVAR(dst);
        CONSUME_TMPVAR(src1);
        CONSUME_TMPVAR(src2);

        dst_ofs = dst * (int)sizeof(struct rt_value);
        src1_ofs = src1 * (int)sizeof(struct rt_value);
        src2_ofs = src2 * (int)sizeof(struct rt_value);
        ASM {
                IB(0x8b); IB(0x5d); IB(0xfc);  /* mov -4(ebp),ebx */
                IB(0x8b); IB(0x83); ID((uint32_t)(src1_ofs + 8));
                IB(0x8b); IB(0x8b); ID((uint32_t)(src2_ofs + 8));
                IB(0x85); IB(0xc9);
                IB(0x0f); IB(0x84);
        }
        zero_patch = (uint8_t *)ctx->code;
        if (!jit_put_dword(ctx, 0)) return false;
        ASM { IB(0x83); IB(0xf9); IB(0xff); IB(0x0f); IB(0x85); }
        divide_patch[0] = (uint8_t *)ctx->code;
        if (!jit_put_dword(ctx, 0)) return false;
        ASM { IB(0x3d); ID(0x80000000u); IB(0x0f); IB(0x85); }
        divide_patch[1] = (uint8_t *)ctx->code;
        if (!jit_put_dword(ctx, 0)) return false;
        ASM { IB(0x31); IB(0xc0); }
        ASM { IB(0xe9); }
        store_patch = (uint8_t *)ctx->code;
        if (!jit_put_dword(ctx, 0)) return false;

        target = (uint8_t *)ctx->code;
        jit_x86_patch_local_rel32(divide_patch[0], target);
        jit_x86_patch_local_rel32(divide_patch[1], target);
        ASM { IB(0x99); IB(0xf7); IB(0xf9); }
        ASM { IB(0x89); IB(0xd0); }

        target = (uint8_t *)ctx->code;
        jit_x86_patch_local_rel32(store_patch, target);
        ASM {
                IB(0xc7); IB(0x83); ID((uint32_t)dst_ofs);
                ID((uint32_t)NOCT_VALUE_INT);
                IB(0x89); IB(0x83); ID((uint32_t)(dst_ofs + 8));
                IB(0xe9);
        }
        done_patch = (uint8_t *)ctx->code;
        if (!jit_put_dword(ctx, 0)) return false;

        target = (uint8_t *)ctx->code;
        jit_x86_patch_local_rel32(zero_patch, target);
        ASM_BINARY_OP(ex_imod_helper);
        target = (uint8_t *)ctx->code;
        jit_x86_patch_local_rel32(done_patch, target);
        return true;
}


#if defined(NOCT_USE_OPTIMIZER)
/*
 * 128-bit vector ops: native SSE tiers or direct scalar lowering over
 * env->vreg, selected from the runtime capability mask.
 */
/* Visit vector instructions with SSE or direct scalar lowering. */
/* Visit an OP_VLOADI32X4 instruction. */
static INLINE bool
jit_visit_vloadi32x4_op(
        struct rt_jit_context *ctx)
{
        int vd;
        int base_tmp;
        int ofs_tmp;
        int base;
        int ofs;

        CONSUME_IMM8(vd);
        CONSUME_TMPVAR(base_tmp);
        CONSUME_TMPVAR(ofs_tmp);

        if ((ctx->simd_caps & JIT_SIMD_CAP_SSE2) == 0) {
                uint32_t vbase;
                int lane;
                int base;
                int ofs;

                vbase = (uint32_t)offsetof(struct rt_env, vreg);
                base = base_tmp * (int)sizeof(struct rt_value);
                ofs = ofs_tmp * (int)sizeof(struct rt_value);
                ASM {
                        /* eax = packed payload; ecx = signed element index. */
                        IB(0x8b); IB(0x5d); IB(0xfc);
                        IB(0x8b); IB(0x83); ID((uint32_t)(base + 8));
                        IB(0x8b); IB(0x8b); ID((uint32_t)(ofs + 8));
                        /* edx = env. */
                        IB(0x8b); IB(0x55); IB(0xf8);
                }
                for (lane = 0; lane < 4; lane++) {
                        /* mov lane*4(eax,ecx,4),ebx */
                        IB(0x8b); IB(0x5c); IB(0x88); IB((uint8_t)(lane * 4));
                        /* mov ebx,env->vreg[vd][lane] */
                        IB(0x89); IB(0x9a);
                        ID(vbase + (uint32_t)vd * 16 + (uint32_t)lane * 4);
                }
                return true;
        }

        base = base_tmp * (int)sizeof(struct rt_value);
        ofs = ofs_tmp * (int)sizeof(struct rt_value);
        ASM {
                /* ebx = tmpvar; eax = base pointer; ecx = signed index. */
                IB(0x8b); IB(0x5d); IB(0xfc);
                IB(0x8b); IB(0x83); ID((uint32_t)(base + 8));
                IB(0x8b); IB(0x8b); ID((uint32_t)(ofs + 8));
                /* movdqu (%eax,%ecx,4), xmmD */
                IB(0xf3); IB(0x0f); IB(0x6f);
                IB((uint8_t)(0x04 | (vd << 3))); IB(0x88);
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
        int base;
        int ofs;

        CONSUME_TMPVAR(base_tmp);
        CONSUME_TMPVAR(ofs_tmp);
        CONSUME_IMM8(vs);

        if ((ctx->simd_caps & JIT_SIMD_CAP_SSE2) == 0) {
                uint32_t vbase;
                int lane;
                int base;
                int ofs;

                vbase = (uint32_t)offsetof(struct rt_env, vreg);
                base = base_tmp * (int)sizeof(struct rt_value);
                ofs = ofs_tmp * (int)sizeof(struct rt_value);
                ASM {
                        IB(0x8b); IB(0x5d); IB(0xfc);
                        IB(0x8b); IB(0x83); ID((uint32_t)(base + 8));
                        IB(0x8b); IB(0x8b); ID((uint32_t)(ofs + 8));
                        IB(0x8b); IB(0x55); IB(0xf8);
                }
                for (lane = 0; lane < 4; lane++) {
                        IB(0x8b); IB(0x9a);
                        ID(vbase + (uint32_t)vs * 16 + (uint32_t)lane * 4);
                        /* mov ebx,lane*4(eax,ecx,4) */
                        IB(0x89); IB(0x5c); IB(0x88); IB((uint8_t)(lane * 4));
                }
                return true;
        }

        base = base_tmp * (int)sizeof(struct rt_value);
        ofs = ofs_tmp * (int)sizeof(struct rt_value);
        ASM {
                IB(0x8b); IB(0x5d); IB(0xfc);
                IB(0x8b); IB(0x83); ID((uint32_t)(base + 8));
                IB(0x8b); IB(0x8b); ID((uint32_t)(ofs + 8));
                /* movdqu xmmS, (%eax,%ecx,4) */
                IB(0xf3); IB(0x0f); IB(0x7f);
                IB((uint8_t)(0x04 | (vs << 3))); IB(0x88);
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
        int src;

        CONSUME_IMM8(vd);
        CONSUME_TMPVAR(src_tmp);

        if ((ctx->simd_caps & JIT_SIMD_CAP_SSE2) == 0) {
                uint32_t vbase;
                int lane;
                int src;

                vbase = (uint32_t)offsetof(struct rt_env, vreg);
                src = src_tmp * (int)sizeof(struct rt_value);
                ASM {
                        IB(0x8b); IB(0x5d); IB(0xfc);
                        IB(0x8b); IB(0x8b); ID((uint32_t)(src + 8));
                        IB(0x8b); IB(0x45); IB(0xf8);
                }
                for (lane = 0; lane < 4; lane++) {
                        IB(0x89); IB(0x88);
                        ID(vbase + (uint32_t)vd * 16 + (uint32_t)lane * 4);
                }
                return true;
        }

        src = src_tmp * (int)sizeof(struct rt_value);
        ASM {
                IB(0x8b); IB(0x5d); IB(0xfc);
                /* movd src+8(%ebx),xmmD; pshufd $0,xmmD,xmmD */
                IB(0x66); IB(0x0f); IB(0x6e);
                IB((uint8_t)(0x83 | (vd << 3))); ID((uint32_t)(src + 8));
                IB(0x66); IB(0x0f); IB(0x70);
                IB((uint8_t)(0xc0 | (vd << 3) | vd)); IB(0x00);
        }
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

        if ((ctx->simd_caps & JIT_SIMD_CAP_SSE2) == 0) {
                uint32_t vbase;
                int d;

                vbase = (uint32_t)offsetof(struct rt_env, vreg);

                d = dst_tmp * (int)sizeof(struct rt_value);
                ASM {
                        IB(0x8b); IB(0x45); IB(0xf8);
                        IB(0x8b); IB(0x88);
                        ID(vbase + (uint32_t)vs * 16 + (uint32_t)lane_index * 4);
                        IB(0x8b); IB(0x5d); IB(0xfc);
                        IB(0xc7); IB(0x83); ID((uint32_t)d);
                        ID((uint32_t)(NOCT_VALUE_INT));
                        IB(0x89); IB(0x8b); ID((uint32_t)(d + 8));
                }
                return true;
        }

        d = dst_tmp * (int)sizeof(struct rt_value);
        ASM { IB(0x8b); IB(0x5d); IB(0xfc); }
        if ((ctx->simd_caps & JIT_SIMD_CAP_SSE41) != 0) {
                ASM {
                        /* pextrd $lane,xmmS,dst_tmp+8(%ebx) */
                        IB(0x66); IB(0x0f); IB(0x3a); IB(0x16);
                        IB((uint8_t)(0x83 | (vs << 3))); ID((uint32_t)(d + 8));
                        IB((uint8_t)lane_index);
                }
        } else {
                ASM {
                        /* SSE2: join the two 16-bit halves in eax. */
                        IB(0x66); IB(0x0f); IB(0xc5); IB((uint8_t)(0xc0 | vs)); IB((uint8_t)(lane_index * 2));
                        IB(0x66); IB(0x0f); IB(0xc5); IB((uint8_t)(0xc8 | vs)); IB((uint8_t)(lane_index * 2 + 1));
                        IB(0xc1); IB(0xe1); IB(0x10);
                        IB(0x09); IB(0xc8);
                        IB(0x89); IB(0x83); ID((uint32_t)(d + 8));
                }
        }
        ASM {
                IB(0xc7); IB(0x83); ID((uint32_t)d);
                ID((uint32_t)(NOCT_VALUE_INT));
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

        CONSUME_IMM8(vd);
        CONSUME_IMM8(vs);

        if ((ctx->simd_caps & JIT_SIMD_CAP_SSE2) == 0) {
                uint32_t vbase;
                int lane;

                vbase = (uint32_t)offsetof(struct rt_env, vreg);
                ASM { IB(0x8b); IB(0x45); IB(0xf8); }
                for (lane = 0; lane < 4; lane++) {
                        IB(0x8b); IB(0x88);
                        ID(vbase + (uint32_t)vs * 16 + (uint32_t)lane * 4);
                        IB(0x89); IB(0x88);
                        ID(vbase + (uint32_t)vd * 16 + (uint32_t)lane * 4);
                }
                return true;
        }

        if (vd != vs) {
                ASM { IB(0x66); IB(0x0f); IB(0x6f);
                      IB((uint8_t)(0xc0 | (vd << 3) | vs)); }
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

        CONSUME_IMM8(vd);
        CONSUME_IMM8(lhs);
        CONSUME_IMM8(rhs);

        if ((ctx->simd_caps & JIT_SIMD_CAP_SSE2) == 0) {
                uint32_t vbase;
                int lane;

                vbase = (uint32_t)offsetof(struct rt_env, vreg);
                ASM { IB(0x8b); IB(0x45); IB(0xf8); }
                for (lane = 0; lane < 4; lane++) {
                        uint32_t a;
                        uint32_t b;
                        uint32_t d;

                        a = vbase + (uint32_t)lhs * 16 + (uint32_t)lane * 4;
                        b = vbase + (uint32_t)rhs * 16 + (uint32_t)lane * 4;
                        d = vbase + (uint32_t)vd * 16 + (uint32_t)lane * 4;
                        IB(0x8b); IB(0x88); ID(a); /* mov a,ecx */
                        IB(0x03); IB(0x88); ID(b);
                        IB(0x89); IB(0x88); ID(d);
                }
                return true;
        }

        if (vd != lhs) {
                ASM { IB(0x66); IB(0x0f); IB(0x6f);
                      IB((uint8_t)(0xc0 | (vd << 3) | lhs)); }
        }
        ASM { IB(0x66); IB(0x0f); IB(0xfe); IB((uint8_t)(0xc0 | (vd << 3) | rhs)); }
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

        CONSUME_IMM8(vd);
        CONSUME_IMM8(lhs);
        CONSUME_IMM8(rhs);

        if ((ctx->simd_caps & JIT_SIMD_CAP_SSE2) == 0) {
                uint32_t vbase;
                int lane;

                vbase = (uint32_t)offsetof(struct rt_env, vreg);
                ASM { IB(0x8b); IB(0x45); IB(0xf8); }
                for (lane = 0; lane < 4; lane++) {
                        uint32_t a;
                        uint32_t b;
                        uint32_t d;

                        a = vbase + (uint32_t)lhs * 16 + (uint32_t)lane * 4;
                        b = vbase + (uint32_t)rhs * 16 + (uint32_t)lane * 4;
                        d = vbase + (uint32_t)vd * 16 + (uint32_t)lane * 4;
                        IB(0x8b); IB(0x88); ID(a); /* mov a,ecx */
                        IB(0x2b); IB(0x88); ID(b);
                        IB(0x89); IB(0x88); ID(d);
                }
                return true;
        }

        if (vd != lhs) {
                ASM { IB(0x66); IB(0x0f); IB(0x6f);
                      IB((uint8_t)(0xc0 | (vd << 3) | lhs)); }
        }
        ASM { IB(0x66); IB(0x0f); IB(0xfa); IB((uint8_t)(0xc0 | (vd << 3) | rhs)); }
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
        int scratch1;
        int scratch2;
        int i;

        CONSUME_IMM8(vd);
        CONSUME_IMM8(lhs);
        CONSUME_IMM8(rhs);

        if ((ctx->simd_caps & JIT_SIMD_CAP_SSE2) == 0) {
                uint32_t vbase;
                int lane;

                vbase = (uint32_t)offsetof(struct rt_env, vreg);
                ASM { IB(0x8b); IB(0x45); IB(0xf8); }
                for (lane = 0; lane < 4; lane++) {
                        uint32_t a;
                        uint32_t b;
                        uint32_t d;

                        a = vbase + (uint32_t)lhs * 16 + (uint32_t)lane * 4;
                        b = vbase + (uint32_t)rhs * 16 + (uint32_t)lane * 4;
                        d = vbase + (uint32_t)vd * 16 + (uint32_t)lane * 4;
                        IB(0x8b); IB(0x88); ID(a); /* mov a,ecx */
                        IB(0x0f); IB(0xaf); IB(0x88); ID(b);
                        IB(0x89); IB(0x88); ID(d);
                }
                return true;
        }

        scratch1 = -1;
        scratch2 = -1;
        if ((ctx->simd_caps & JIT_SIMD_CAP_SSE41) == 0) {
                /* x86 has no extra XMM registers.  Preserve two registers
                 * that are not operands and use them for the SSE2 multiply. */
                for (i = 0; i < 8; i++) {
                        if (i == vd || i == lhs || i == rhs)
                                continue;
                        if (scratch1 < 0)
                                scratch1 = i;
                        else {
                                scratch2 = i;
                                break;
                        }
                }

                assert(scratch1 >= 0 && scratch2 >= 0);

                ASM {
                        /* Save scratch XMM registers to an unaligned stack area. */
                        IB(0x83); IB(0xec); IB(0x20);
                        IB(0xf3); IB(0x0f); IB(0x7f); IB((uint8_t)(0x04 | (scratch1 << 3))); IB(0x24);
                        IB(0xf3); IB(0x0f); IB(0x7f); IB((uint8_t)(0x44 | (scratch2 << 3))); IB(0x24); IB(0x10);
                        /* Odd lanes: shuffle to even positions, then pmuludq. */
                        IB(0x66); IB(0x0f); IB(0x6f); IB((uint8_t)(0xc0 | (scratch1 << 3) | lhs));
                        IB(0x66); IB(0x0f); IB(0x70); IB((uint8_t)(0xc0 | (scratch1 << 3) | scratch1)); IB(0xf5);
                        IB(0x66); IB(0x0f); IB(0x6f); IB((uint8_t)(0xc0 | (scratch2 << 3) | rhs));
                        IB(0x66); IB(0x0f); IB(0x70); IB((uint8_t)(0xc0 | (scratch2 << 3) | scratch2)); IB(0xf5);
                        IB(0x66); IB(0x0f); IB(0xf4); IB((uint8_t)(0xc0 | (scratch1 << 3) | scratch2));
                }
        }
        if (vd != lhs) {
                ASM { IB(0x66); IB(0x0f); IB(0x6f);
                      IB((uint8_t)(0xc0 | (vd << 3) | lhs)); }
        }
        if ((ctx->simd_caps & JIT_SIMD_CAP_SSE41) != 0) {
                ASM { IB(0x66); IB(0x0f); IB(0x38); IB(0x40); IB((uint8_t)(0xc0 | (vd << 3) | rhs)); }
        } else {
                ASM {
                        /* Even products in vd; merge their low dwords
                         * with the odd products from scratch1. */
                        IB(0x66); IB(0x0f); IB(0xf4); IB((uint8_t)(0xc0 | (vd << 3) | rhs));
                        IB(0x66); IB(0x0f); IB(0x70); IB((uint8_t)(0xc0 | (vd << 3) | vd)); IB(0x88);
                        IB(0x66); IB(0x0f); IB(0x70); IB((uint8_t)(0xc0 | (scratch1 << 3) | scratch1)); IB(0x88);
                        IB(0x66); IB(0x0f); IB(0x62); IB((uint8_t)(0xc0 | (vd << 3) | scratch1));
                        /* Restore the borrowed XMM registers. */
                        IB(0xf3); IB(0x0f); IB(0x6f); IB((uint8_t)(0x04 | (scratch1 << 3))); IB(0x24);
                        IB(0xf3); IB(0x0f); IB(0x6f); IB((uint8_t)(0x44 | (scratch2 << 3))); IB(0x24); IB(0x10);
                        IB(0x83); IB(0xc4); IB(0x20);
                }
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

        CONSUME_IMM8(vd);
        CONSUME_IMM8(lhs);
        CONSUME_IMM8(rhs);

        if ((ctx->simd_caps & JIT_SIMD_CAP_SSE2) == 0) {
                uint32_t vbase;
                int lane;

                vbase = (uint32_t)offsetof(struct rt_env, vreg);
                ASM { IB(0x8b); IB(0x45); IB(0xf8); }
                for (lane = 0; lane < 4; lane++) {
                        uint32_t a;
                        uint32_t b;
                        uint32_t d;

                        a = vbase + (uint32_t)lhs * 16 + (uint32_t)lane * 4;
                        b = vbase + (uint32_t)rhs * 16 + (uint32_t)lane * 4;
                        d = vbase + (uint32_t)vd * 16 + (uint32_t)lane * 4;
                        IB(0x8b); IB(0x88); ID(a); /* mov a,ecx */
                        IB(0x23); IB(0x88); ID(b);
                        IB(0x89); IB(0x88); ID(d);
                }
                return true;
        }

        if (vd != lhs) {
                ASM { IB(0x66); IB(0x0f); IB(0x6f);
                      IB((uint8_t)(0xc0 | (vd << 3) | lhs)); }
        }
        ASM { IB(0x66); IB(0x0f); IB(0xdb); IB((uint8_t)(0xc0 | (vd << 3) | rhs)); }
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

        CONSUME_IMM8(vd);
        CONSUME_IMM8(lhs);
        CONSUME_IMM8(rhs);

        if ((ctx->simd_caps & JIT_SIMD_CAP_SSE2) == 0) {
                uint32_t vbase;
                int lane;

                vbase = (uint32_t)offsetof(struct rt_env, vreg);
                ASM { IB(0x8b); IB(0x45); IB(0xf8); }
                for (lane = 0; lane < 4; lane++) {
                        uint32_t a;
                        uint32_t b;
                        uint32_t d;

                        a = vbase + (uint32_t)lhs * 16 + (uint32_t)lane * 4;
                        b = vbase + (uint32_t)rhs * 16 + (uint32_t)lane * 4;
                        d = vbase + (uint32_t)vd * 16 + (uint32_t)lane * 4;
                        IB(0x8b); IB(0x88); ID(a); /* mov a,ecx */
                        IB(0x0b); IB(0x88); ID(b);
                        IB(0x89); IB(0x88); ID(d);
                }
                return true;
        }

        if (vd != lhs) {
                ASM { IB(0x66); IB(0x0f); IB(0x6f);
                      IB((uint8_t)(0xc0 | (vd << 3) | lhs)); }
        }
        ASM { IB(0x66); IB(0x0f); IB(0xeb); IB((uint8_t)(0xc0 | (vd << 3) | rhs)); }
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

        CONSUME_IMM8(vd);
        CONSUME_IMM8(lhs);
        CONSUME_IMM8(rhs);

        if ((ctx->simd_caps & JIT_SIMD_CAP_SSE2) == 0) {
                uint32_t vbase;
                int lane;

                vbase = (uint32_t)offsetof(struct rt_env, vreg);
                ASM { IB(0x8b); IB(0x45); IB(0xf8); }
                for (lane = 0; lane < 4; lane++) {
                        uint32_t a;
                        uint32_t b;
                        uint32_t d;

                        a = vbase + (uint32_t)lhs * 16 + (uint32_t)lane * 4;
                        b = vbase + (uint32_t)rhs * 16 + (uint32_t)lane * 4;
                        d = vbase + (uint32_t)vd * 16 + (uint32_t)lane * 4;
                        IB(0x8b); IB(0x88); ID(a); /* mov a,ecx */
                        IB(0x33); IB(0x88); ID(b);
                        IB(0x89); IB(0x88); ID(d);
                }
                return true;
        }

        if (vd != lhs) {
                ASM { IB(0x66); IB(0x0f); IB(0x6f);
                      IB((uint8_t)(0xc0 | (vd << 3) | lhs)); }
        }
        ASM { IB(0x66); IB(0x0f); IB(0xef); IB((uint8_t)(0xc0 | (vd << 3) | rhs)); }
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

        CONSUME_IMM8(vd);
        CONSUME_IMM8(vs);
        CONSUME_IMM8(shift);

        if ((ctx->simd_caps & JIT_SIMD_CAP_SSE2) == 0) {
                uint32_t vbase;
                int lane;

                vbase = (uint32_t)offsetof(struct rt_env, vreg);
                ASM { IB(0x8b); IB(0x45); IB(0xf8); }
                for (lane = 0; lane < 4; lane++) {
                        uint32_t s;
                        uint32_t d;

                        s = vbase + (uint32_t)vs * 16 + (uint32_t)lane * 4;
                        d = vbase + (uint32_t)vd * 16 + (uint32_t)lane * 4;
                        IB(0x8b); IB(0x88); ID(s);
                        IB(0xc1); IB((uint8_t)(0xe1));
                        IB((uint8_t)shift);
                        IB(0x89); IB(0x88); ID(d);
                }
                return true;
        }

        if (vd != vs) {
                ASM { IB(0x66); IB(0x0f); IB(0x6f);
                      IB((uint8_t)(0xc0 | (vd << 3) | vs)); }
        }
        ASM { IB(0x66); IB(0x0f); IB(0x72);
              IB((uint8_t)(0xf0 | vd)); IB((uint8_t)shift); }

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

        CONSUME_IMM8(vd);
        CONSUME_IMM8(vs);
        CONSUME_IMM8(shift);

        if ((ctx->simd_caps & JIT_SIMD_CAP_SSE2) == 0) {
                uint32_t vbase;
                int lane;

                vbase = (uint32_t)offsetof(struct rt_env, vreg);
                ASM { IB(0x8b); IB(0x45); IB(0xf8); }
                for (lane = 0; lane < 4; lane++) {
                        uint32_t s;
                        uint32_t d;

                        s = vbase + (uint32_t)vs * 16 + (uint32_t)lane * 4;
                        d = vbase + (uint32_t)vd * 16 + (uint32_t)lane * 4;
                        IB(0x8b); IB(0x88); ID(s);
                        IB(0xc1); IB((uint8_t)(0xe9));
                        IB((uint8_t)shift);
                        IB(0x89); IB(0x88); ID(d);
                }
                return true;
        }

        if (vd != vs) {
                ASM { IB(0x66); IB(0x0f); IB(0x6f);
                      IB((uint8_t)(0xc0 | (vd << 3) | vs)); }
        }
        ASM { IB(0x66); IB(0x0f); IB(0x72);
              IB((uint8_t)(0xd0 | vd)); IB((uint8_t)shift); }

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
        int base;
        int ofs;

        CONSUME_IMM8(vd);
        CONSUME_TMPVAR(base_tmp);
        CONSUME_TMPVAR(ofs_tmp);

        if ((ctx->simd_caps & JIT_SIMD_CAP_SSE2) == 0) {
                uint32_t vbase;
                int lane;
                int base;
                int ofs;

                vbase = (uint32_t)offsetof(struct rt_env, vreg);
                base = base_tmp * (int)sizeof(struct rt_value);
                ofs = ofs_tmp * (int)sizeof(struct rt_value);
                ASM {
                        /* eax = packed payload; ecx = signed element index. */
                        IB(0x8b); IB(0x5d); IB(0xfc);
                        IB(0x8b); IB(0x83); ID((uint32_t)(base + 8));
                        IB(0x8b); IB(0x8b); ID((uint32_t)(ofs + 8));
                        /* edx = env. */
                        IB(0x8b); IB(0x55); IB(0xf8);
                }
                for (lane = 0; lane < 4; lane++) {
                        /* mov lane*4(eax,ecx,4),ebx */
                        IB(0x8b); IB(0x5c); IB(0x88); IB((uint8_t)(lane * 4));
                        /* mov ebx,env->vreg[vd][lane] */
                        IB(0x89); IB(0x9a);
                        ID(vbase + (uint32_t)vd * 16 + (uint32_t)lane * 4);
                }
                return true;
        }

        base = base_tmp * (int)sizeof(struct rt_value);
        ofs = ofs_tmp * (int)sizeof(struct rt_value);
        ASM {
                /* ebx = tmpvar; eax = base pointer; ecx = signed index. */
                IB(0x8b); IB(0x5d); IB(0xfc);
                IB(0x8b); IB(0x83); ID((uint32_t)(base + 8));
                IB(0x8b); IB(0x8b); ID((uint32_t)(ofs + 8));
                /* movdqu (%eax,%ecx,4), xmmD */
                IB(0xf3); IB(0x0f); IB(0x6f);
                IB((uint8_t)(0x04 | (vd << 3))); IB(0x88);
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
        int base;
        int ofs;

        CONSUME_TMPVAR(base_tmp);
        CONSUME_TMPVAR(ofs_tmp);
        CONSUME_IMM8(vs);

        if ((ctx->simd_caps & JIT_SIMD_CAP_SSE2) == 0) {
                uint32_t vbase;
                int lane;
                int base;
                int ofs;

                vbase = (uint32_t)offsetof(struct rt_env, vreg);
                base = base_tmp * (int)sizeof(struct rt_value);
                ofs = ofs_tmp * (int)sizeof(struct rt_value);
                ASM {
                        IB(0x8b); IB(0x5d); IB(0xfc);
                        IB(0x8b); IB(0x83); ID((uint32_t)(base + 8));
                        IB(0x8b); IB(0x8b); ID((uint32_t)(ofs + 8));
                        IB(0x8b); IB(0x55); IB(0xf8);
                }
                for (lane = 0; lane < 4; lane++) {
                        IB(0x8b); IB(0x9a);
                        ID(vbase + (uint32_t)vs * 16 + (uint32_t)lane * 4);
                        /* mov ebx,lane*4(eax,ecx,4) */
                        IB(0x89); IB(0x5c); IB(0x88); IB((uint8_t)(lane * 4));
                }
                return true;
        }

        base = base_tmp * (int)sizeof(struct rt_value);
        ofs = ofs_tmp * (int)sizeof(struct rt_value);
        ASM {
                IB(0x8b); IB(0x5d); IB(0xfc);
                IB(0x8b); IB(0x83); ID((uint32_t)(base + 8));
                IB(0x8b); IB(0x8b); ID((uint32_t)(ofs + 8));
                /* movdqu xmmS, (%eax,%ecx,4) */
                IB(0xf3); IB(0x0f); IB(0x7f);
                IB((uint8_t)(0x04 | (vs << 3))); IB(0x88);
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
        int src;

        CONSUME_IMM8(vd);
        CONSUME_TMPVAR(src_tmp);

        if ((ctx->simd_caps & JIT_SIMD_CAP_SSE2) == 0) {
                uint32_t vbase;
                int lane;
                int src;

                vbase = (uint32_t)offsetof(struct rt_env, vreg);
                src = src_tmp * (int)sizeof(struct rt_value);
                ASM {
                        IB(0x8b); IB(0x5d); IB(0xfc);
                        IB(0x8b); IB(0x8b); ID((uint32_t)(src + 8));
                        IB(0x8b); IB(0x45); IB(0xf8);
                }
                for (lane = 0; lane < 4; lane++) {
                        IB(0x89); IB(0x88);
                        ID(vbase + (uint32_t)vd * 16 + (uint32_t)lane * 4);
                }
                return true;
        }

        src = src_tmp * (int)sizeof(struct rt_value);
        ASM {
                IB(0x8b); IB(0x5d); IB(0xfc);
                /* movd src+8(%ebx),xmmD; pshufd $0,xmmD,xmmD */
                IB(0x66); IB(0x0f); IB(0x6e);
                IB((uint8_t)(0x83 | (vd << 3))); ID((uint32_t)(src + 8));
                IB(0x66); IB(0x0f); IB(0x70);
                IB((uint8_t)(0xc0 | (vd << 3) | vd)); IB(0x00);
        }
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

        if ((ctx->simd_caps & JIT_SIMD_CAP_SSE2) == 0) {
                uint32_t vbase;
                int d;

                vbase = (uint32_t)offsetof(struct rt_env, vreg);

                d = dst_tmp * (int)sizeof(struct rt_value);
                ASM {
                        IB(0x8b); IB(0x45); IB(0xf8);
                        IB(0x8b); IB(0x88);
                        ID(vbase + (uint32_t)vs * 16 + (uint32_t)lane_index * 4);
                        IB(0x8b); IB(0x5d); IB(0xfc);
                        IB(0xc7); IB(0x83); ID((uint32_t)d);
                        ID((uint32_t)(NOCT_VALUE_FLOAT));
                        IB(0x89); IB(0x8b); ID((uint32_t)(d + 8));
                }
                return true;
        }

        d = dst_tmp * (int)sizeof(struct rt_value);
        ASM { IB(0x8b); IB(0x5d); IB(0xfc); }
        if ((ctx->simd_caps & JIT_SIMD_CAP_SSE41) != 0) {
                ASM {
                        /* pextrd $lane,xmmS,dst_tmp+8(%ebx) */
                        IB(0x66); IB(0x0f); IB(0x3a); IB(0x16);
                        IB((uint8_t)(0x83 | (vs << 3))); ID((uint32_t)(d + 8));
                        IB((uint8_t)lane_index);
                }
        } else {
                ASM {
                        /* SSE2: join the two 16-bit halves in eax. */
                        IB(0x66); IB(0x0f); IB(0xc5); IB((uint8_t)(0xc0 | vs)); IB((uint8_t)(lane_index * 2));
                        IB(0x66); IB(0x0f); IB(0xc5); IB((uint8_t)(0xc8 | vs)); IB((uint8_t)(lane_index * 2 + 1));
                        IB(0xc1); IB(0xe1); IB(0x10);
                        IB(0x09); IB(0xc8);
                        IB(0x89); IB(0x83); ID((uint32_t)(d + 8));
                }
        }
        ASM {
                IB(0xc7); IB(0x83); ID((uint32_t)d);
                ID((uint32_t)(NOCT_VALUE_FLOAT));
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

        CONSUME_IMM8(vd);
        CONSUME_IMM8(lhs);
        CONSUME_IMM8(rhs);

        if ((ctx->simd_caps & JIT_SIMD_CAP_SSE2) == 0) {
                uint32_t vbase;
                int lane;

                vbase = (uint32_t)offsetof(struct rt_env, vreg);
                ASM { IB(0x8b); IB(0x45); IB(0xf8); }
                for (lane = 0; lane < 4; lane++) {
                        uint32_t a;
                        uint32_t b;
                        uint32_t d;

                        a = vbase + (uint32_t)lhs * 16 + (uint32_t)lane * 4;
                        b = vbase + (uint32_t)rhs * 16 + (uint32_t)lane * 4;
                        d = vbase + (uint32_t)vd * 16 + (uint32_t)lane * 4;
                        IB(0xd9); IB(0x80); ID(a); /* fld dword ptr [eax+a] */
                        IB(0xd8);
                        IB(0x80);
                        ID(b);
                        IB(0xd9); IB(0x98); ID(d); /* fstp dword ptr [eax+d] */
                }
                return true;
        }

        if (vd != lhs) {
                ASM { IB(0x66); IB(0x0f); IB(0x6f);
                      IB((uint8_t)(0xc0 | (vd << 3) | lhs)); }
        }
        ASM { IB(0x0f); IB(0x58); IB((uint8_t)(0xc0 | (vd << 3) | rhs)); }

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

        CONSUME_IMM8(vd);
        CONSUME_IMM8(lhs);
        CONSUME_IMM8(rhs);

        if ((ctx->simd_caps & JIT_SIMD_CAP_SSE2) == 0) {
                uint32_t vbase;
                int lane;

                vbase = (uint32_t)offsetof(struct rt_env, vreg);
                ASM { IB(0x8b); IB(0x45); IB(0xf8); }
                for (lane = 0; lane < 4; lane++) {
                        uint32_t a;
                        uint32_t b;
                        uint32_t d;

                        a = vbase + (uint32_t)lhs * 16 + (uint32_t)lane * 4;
                        b = vbase + (uint32_t)rhs * 16 + (uint32_t)lane * 4;
                        d = vbase + (uint32_t)vd * 16 + (uint32_t)lane * 4;
                        IB(0xd9); IB(0x80); ID(a); /* fld dword ptr [eax+a] */
                        IB(0xd8);
                        IB(0xa0);
                        ID(b);
                        IB(0xd9); IB(0x98); ID(d); /* fstp dword ptr [eax+d] */
                }
                return true;
        }

        if (vd != lhs) {
                ASM { IB(0x66); IB(0x0f); IB(0x6f);
                      IB((uint8_t)(0xc0 | (vd << 3) | lhs)); }
        }
        ASM { IB(0x0f); IB(0x5c); IB((uint8_t)(0xc0 | (vd << 3) | rhs)); }

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

        CONSUME_IMM8(vd);
        CONSUME_IMM8(lhs);
        CONSUME_IMM8(rhs);

        if ((ctx->simd_caps & JIT_SIMD_CAP_SSE2) == 0) {
                uint32_t vbase;
                int lane;

                vbase = (uint32_t)offsetof(struct rt_env, vreg);
                ASM { IB(0x8b); IB(0x45); IB(0xf8); }
                for (lane = 0; lane < 4; lane++) {
                        uint32_t a;
                        uint32_t b;
                        uint32_t d;

                        a = vbase + (uint32_t)lhs * 16 + (uint32_t)lane * 4;
                        b = vbase + (uint32_t)rhs * 16 + (uint32_t)lane * 4;
                        d = vbase + (uint32_t)vd * 16 + (uint32_t)lane * 4;
                        IB(0xd9); IB(0x80); ID(a); /* fld dword ptr [eax+a] */
                        IB(0xd8);
                        IB(0x88);
                        ID(b);
                        IB(0xd9); IB(0x98); ID(d); /* fstp dword ptr [eax+d] */
                }
                return true;
        }

        if (vd != lhs) {
                ASM { IB(0x66); IB(0x0f); IB(0x6f);
                      IB((uint8_t)(0xc0 | (vd << 3) | lhs)); }
        }
        ASM { IB(0x0f); IB(0x59); IB((uint8_t)(0xc0 | (vd << 3) | rhs)); }

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

        CONSUME_IMM8(vd);
        CONSUME_IMM8(lhs);
        CONSUME_IMM8(rhs);

        if ((ctx->simd_caps & JIT_SIMD_CAP_SSE2) == 0) {
                uint32_t vbase;
                int lane;

                vbase = (uint32_t)offsetof(struct rt_env, vreg);
                ASM { IB(0x8b); IB(0x45); IB(0xf8); }
                for (lane = 0; lane < 4; lane++) {
                        uint32_t a;
                        uint32_t b;
                        uint32_t d;

                        a = vbase + (uint32_t)lhs * 16 + (uint32_t)lane * 4;
                        b = vbase + (uint32_t)rhs * 16 + (uint32_t)lane * 4;
                        d = vbase + (uint32_t)vd * 16 + (uint32_t)lane * 4;
                        IB(0xd9); IB(0x80); ID(a); /* fld dword ptr [eax+a] */
                        IB(0xd8);
                        IB(0xb0);
                        ID(b);
                        IB(0xd9); IB(0x98); ID(d); /* fstp dword ptr [eax+d] */
                }
                return true;
        }

        if (vd != lhs) {
                ASM { IB(0x66); IB(0x0f); IB(0x6f);
                      IB((uint8_t)(0xc0 | (vd << 3) | lhs)); }
        }
        ASM { IB(0x0f); IB(0x5e); IB((uint8_t)(0xc0 | (vd << 3) | rhs)); }

        return true;
}

/* Visit an OP_VCVTI32F32X4 instruction. */
static INLINE bool
jit_visit_vcvti32f32x4_op(
        struct rt_jit_context *ctx)
{
        int vd;
        int vs;

        CONSUME_IMM8(vd);
        CONSUME_IMM8(vs);

        if ((ctx->simd_caps & JIT_SIMD_CAP_SSE2) == 0) {
                uint32_t vbase;
                int lane;

                vbase = (uint32_t)offsetof(struct rt_env, vreg);
                ASM { IB(0x8b); IB(0x45); IB(0xf8); }
                for (lane = 0; lane < 4; lane++) {
                        uint32_t s;
                        uint32_t d;

                        s = vbase + (uint32_t)vs * 16 +
                                (uint32_t)lane * 4;
                        d = vbase + (uint32_t)vd * 16 +
                                (uint32_t)lane * 4;
                        IB(0xdb); IB(0x80); ID(s); /* fild m32 */
                        IB(0xd9); IB(0x98); ID(d); /* fstp m32 */
                }
                return true;
        }

        ASM { IB(0x0f); IB(0x5b); IB((uint8_t)(0xc0 | (vd << 3) | vs)); }

        return true;
}

/* Visit an OP_VCVTF32I32X4 instruction. */
static INLINE bool
jit_visit_vcvtf32i32x4_op(
        struct rt_jit_context *ctx)
{
        int vd;
        int vs;

        CONSUME_IMM8(vd);
        CONSUME_IMM8(vs);

        if ((ctx->simd_caps & JIT_SIMD_CAP_SSE2) == 0) {
                uint32_t vbase;
                int lane;

                vbase = (uint32_t)offsetof(struct rt_env, vreg);
                ASM { IB(0x8b); IB(0x45); IB(0xf8); }
                for (lane = 0; lane < 4; lane++) {
                        uint32_t s;
                        uint32_t d;

                        s = vbase + (uint32_t)vs * 16 +
                                (uint32_t)lane * 4;
                        d = vbase + (uint32_t)vd * 16 +
                                (uint32_t)lane * 4;
                        /* Temporarily select x87 round-toward-zero. */
                        IB(0x83); IB(0xec); IB(0x08);
                        IB(0xd9); IB(0x3c); IB(0x24);
                        IB(0x0f); IB(0xb7); IB(0x0c); IB(0x24);
                        IB(0x80); IB(0xcd); IB(0x0c);
                        IB(0x66); IB(0x89); IB(0x4c); IB(0x24); IB(0x02);
                        IB(0xd9); IB(0x6c); IB(0x24); IB(0x02);
                        IB(0xd9); IB(0x80); ID(s);
                        IB(0xdb); IB(0x98); ID(d);
                        IB(0xd9); IB(0x2c); IB(0x24);
                        IB(0x83); IB(0xc4); IB(0x08);
                }
                return true;
        }

        ASM { IB(0xf3); IB(0x0f); IB(0x5b); IB((uint8_t)(0xc0 | (vd << 3) | vs)); }

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

        CONSUME_IMM8(vd);
        CONSUME_IMM8(lhs);
        CONSUME_IMM8(rhs);

        if ((ctx->simd_caps & JIT_SIMD_CAP_SSE2) == 0) {

                assert(NEVER_COME_HERE);

                return false;
        }

        assert(NEVER_COME_HERE);

        return false;
}

/* Visit an OP_VMAXS32X4 instruction. */
static INLINE bool
jit_visit_vmaxs32x4_op(
        struct rt_jit_context *ctx)
{
        int vd;
        int lhs;
        int rhs;

        CONSUME_IMM8(vd);
        CONSUME_IMM8(lhs);
        CONSUME_IMM8(rhs);

        if ((ctx->simd_caps & JIT_SIMD_CAP_SSE2) == 0) {

                assert(NEVER_COME_HERE);

                return false;
        }

        assert(NEVER_COME_HERE);

        return false;
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
        /* prologue: */
                /* mov 4(%esp), %eax; env */            IB(0x8b); IB(0x44); IB(0x24); IB(0x04);

                /* pushl %ebx */                        IB(0x53);
                /* pushl %ecx */                        IB(0x51);
                /* pushl %edx */                        IB(0x52);
                /* pushl %edi */                        IB(0x57);
                /* pushl %esi */                        IB(0x56);
                /* pushl %ebp */                        IB(0x55);

                /* movl %esp, %ebp */                   IB(0x89); IB(0xe5);
                /* subl $12, %esp */                    IB(0x83); IB(0xec); IB(0x0c);

                /* (ebp-8): rt */
                /* movl %eax, -8(%ebp) */               IB(0x89); IB(0x45); IB(0xf8);

                /* (ebp-4): &env->frame->tmpvar[0] */
                /* movl (%eax), %eax */                 IB(0x8b); IB(0x00);
                /* movl (%eax), %eax */                 IB(0x8b); IB(0x00);
                /* movl %eax, -4(%ebp) */               IB(0x89); IB(0x45); IB(0xfc);

                /* (ebp-12): exception_handler */
                /* movl $(ctx->code + 6), -12(%ebp) */  IB(0xc7); IB(0x45); IB(0xf4); ID((uint32_t)((uint8_t *)ctx->code + 6));

                /* Skip an exception handler. */
                /* jmp exception_handler_end */         IB(0xeb); IB(0x0f);
        }

        /* Put an exception handler. */
        ctx->exception_code = ctx->code;
        ASM {
        /* exception_handler: */
                /* addl $12, %esp */                    IB(0x83); IB(0xc4); IB(0x0c);
                /* popl %ebp */                         IB(0x5d);
                /* popl %esi */                         IB(0x5e);
                /* popl %edi */                         IB(0x5f);
                /* popl %edx */                         IB(0x5a);
                /* popl %ecx */                         IB(0x59);
                /* popl %ebx */                         IB(0x5b);
                /* movl $0, %eax */                     IB(0xb8); ID(0);
                /* ret */                               IB(0xc3);
        /* exception_handler_end: */
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
#if 0
                        if (!jit_visit_eq_op(ctx))
                                return false;
#endif
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
#if 0
                        if (!jit_visit_jmpiftrue_op(ctx))
                                return false;
#endif
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
			if (!jit_visit_vindex_hint_op(ctx)) return false;
			break;
#endif
		case OP_PLOOP_HINT:
			if (!rt_jit_visit_ploop_hint_op(ctx)) return false;
			break;
		case OP_TMPVAR_TYPE:
			if (!rt_jit_visit_tmpvar_type_op(ctx)) return false;
			break;
		case OP_MATERIALIZE_TYPE:
			if (!rt_jit_visit_materialize_type_metadata_op(ctx)) return false;
			break;
		case OP_SUBJNZ:
			if (!jit_visit_subjnz_op(ctx)) return false;
			break;
#if defined(NOCT_USE_OPTIMIZER)
		case OP_VORI32X4I:
			if (!jit_visit_vori32x4i_op(ctx)) return false;
			break;
		case OP_VFMAF32X4:
			if (!jit_visit_vfmaf32x4_op(ctx)) return false;
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
        /* epilogue: */
                /* addl $12, %esp */    IB(0x83); IB(0xc4); IB(0x0c);
                /* popl %ebp */         IB(0x5d);
                /* popl %esi */         IB(0x5e);
                /* popl %edi */         IB(0x5f);
                /* popl %edx */         IB(0x5a);
                /* popl %ecx */         IB(0x59);
                /* popl %ebx */         IB(0x5b);
                /* movl $1, %eax */     IB(0xb8); ID(1);
                /* ret */               IB(0xc3);
        }

        return true;
}

static bool
jit_patch_branch(
    struct rt_jit_context *ctx,
    int patch_index)
{
        uint8_t *target_code;
        int offset;
        intptr_t wide_offset;
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
                rt_error(ctx->env, N_TR("Branch target not found."));
                return false;
        }

        /* Calc a branch offset. */
        wide_offset = (intptr_t)target_code -
                      (intptr_t)ctx->branch_patch[patch_index].code;
        if (wide_offset < (-2147483647L - 1L) || wide_offset > 2147483647L) {
                rt_error(ctx->env, N_TR("Branch target too far."));
                return false;
        }
        offset = (int)wide_offset;

        /* Set the assembler cursor. */
        ctx->code = ctx->branch_patch[patch_index].code;

        /* Assemble. */
        if (ctx->branch_patch[patch_index].type == PATCH_JMP) {
                offset -= 5;
                ASM {
                        /* jmp offset */    IB(0xe9); ID((uint32_t)offset);
                }
        } else if (ctx->branch_patch[patch_index].type == PATCH_JE) {
                offset -= 6;
                ASM {
                        /* je offset */     IB(0x0f); IB(0x84); ID((uint32_t)offset);
                }
        } else if (ctx->branch_patch[patch_index].type == PATCH_JNE) {
                offset -= 6;
                ASM {
                        /* jne offset */    IB(0x0f); IB(0x85); ID((uint32_t)offset);
                }
        }

        return true;
}

#endif /* defined(NOCT_ARCH_X86) && defined(NOCT_USE_JIT) */
