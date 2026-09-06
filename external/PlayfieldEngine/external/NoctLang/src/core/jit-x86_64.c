/* -*- coding: utf-8; tab-width: 8; indent-tabs-mode: nil; -*- */

/*
 * Noct Programming Language
 * Copyright (c) 2025, 2026, Awe Morris
 */

/*
 * JIT (x86_64): Just-In-Time native code generation
 */

#include <noct/noct.h>

#if defined(NOCT_ARCH_X86_64) && defined(NOCT_USE_JIT)

#include <noct/noct.h>
#include "runtime.h"
#include "jit.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#if defined(_MSC_VER)
#include <intrin.h>
#endif

/*
 * ABI Check (MS ABI or SYSV ABI)
 */
#define IS_MSABI                (sizeof(long) == 4)

/* False asseretion */
#define JIT_OP_NOT_IMPLEMENTED  0
#define NEVER_COME_HERE         0

/* Branch patch type */
#define PATCH_JMP               0
#define PATCH_JE                1
#define PATCH_JNE               2

/* Forward declaration */
static bool jit_visit_bytecode(struct rt_jit_context *ctx);
static bool jit_patch_branch(struct rt_jit_context *ctx, int patch_index);
static uint32_t jit_detect_simd_caps(void);
static void jit_x86_64_dump_code(struct rt_jit_context *ctx,
	void *generated_end);
#if defined(NOCT_USE_OPTIMIZER)
static uint16_t jit_x86_64_read_u16(const uint8_t *p);
static uint32_t jit_x86_64_read_u32(const uint8_t *p);
static bool jit_x86_64_scan_vector_loop(struct rt_jit_context *ctx,
	int index_tmp, int remaining_tmp, int lanes);
static bool jit_x86_64_is_packed_index_alias(struct rt_jit_context *ctx,
	int tmp);
static void jit_x86_64_remove_packed_index_alias(struct rt_jit_context *ctx,
	int tmp);
static bool jit_x86_64_add_packed_index_alias(struct rt_jit_context *ctx,
	int tmp);
static int jit_x86_64_resolve_packed_base(struct rt_jit_context *ctx, int tmp);
static void jit_x86_64_remove_packed_base_alias(struct rt_jit_context *ctx,
	int tmp);
static bool jit_x86_64_set_packed_base_alias(struct rt_jit_context *ctx,
	int dst, int src);
static void jit_x86_64_gpr_reset(struct rt_jit_context *ctx);
static int jit_x86_64_gpr_limit(void);
static bool jit_x86_64_gpr_spill(struct rt_jit_context *ctx, int slot);
static bool jit_x86_64_gpr_alloc(struct rt_jit_context *ctx, int tmp,
	unsigned pin_mask, bool load, int *reg);
static bool jit_x86_64_gpr_get(struct rt_jit_context *ctx, int tmp,
	unsigned pin_mask, int *reg);
static bool jit_x86_64_gpr_dest(struct rt_jit_context *ctx, int tmp,
	unsigned pin_mask, int *reg);
static bool jit_x86_64_gpr_mov(struct rt_jit_context *ctx, int dst, int src);
static bool jit_x86_64_gpr_rebind(struct rt_jit_context *ctx, int dst,
	int src, int *reg);
static bool jit_x86_64_gpr_flush(struct rt_jit_context *ctx);
static bool jit_x86_64_gpr_flush_required(struct rt_jit_context *ctx);
static bool jit_x86_64_gpr_publish_remat(struct rt_jit_context *ctx);
static bool jit_x86_64_packed_cursor(struct rt_jit_context *ctx, int base,
	int ofs, int scale, uint8_t *sib, int *cursor, int32_t *byte_disp);
#endif

/*
 * Generate a JIT-compiled code for a function.
 */
bool
jit_build(
          struct rt_env *env,
          struct rt_func *func)
{
        struct rt_jit_context ctx;
	struct rt_jit_slab *slab;
	void *code_top;
	void *code_end;
	void *generated_end;
	int attempt;
        int i;

	for (attempt = 0; attempt < 2; attempt++) {
		if (!rt_jit_slab_acquire(env, &slab, &code_top, &code_end))
			return false;

		/* A failed attempt never advances slab->current, so the whole
		 * function can be regenerated without invalid branch patches. */
		memset(&ctx, 0, sizeof(struct rt_jit_context));
		ctx.code_top = code_top;
		ctx.code_end = code_end;
		ctx.code = ctx.code_top;
		ctx.env = env;
		ctx.func = func;
		if (!rt_jit_context_init_tables(&ctx))
			return false;
		rt_jit_configure_simd(&ctx, jit_detect_simd_caps(), "x86_64");

		if (!jit_visit_bytecode(&ctx)) {
			if (ctx.code_overflow && attempt == 0 &&
			    ((uint8_t *)code_top != slab->base ||
			     slab->size < rt_jit_get_code_size(env))) {
				rt_jit_slab_abandon(env, slab);
				rt_jit_slab_clear_overflow(env);
				rt_jit_context_dispose_tables(&ctx);
				continue;
			}
			rt_jit_context_dispose_tables(&ctx);
			return false;
		}

		generated_end = ctx.code;
		for (i = 0; i < ctx.branch_patch_count; i++) {
			if (!jit_patch_branch(&ctx, i)) {
				rt_jit_context_dispose_tables(&ctx);
				return false;
			}
		}
		jit_x86_64_dump_code(&ctx, generated_end);
		rt_jit_slab_finish(env, slab, generated_end);
		if (getenv("NOCT_JIT_CODEGEN_DEBUG") != NULL) {
			fprintf(stderr,
				"noct-jit-codegen: x86_64: func=%s bytes=%lu pc_entries=%u branches=%d\n",
				func->name != NULL ? func->name : "?",
				(unsigned long)((uint8_t *)generated_end -
						(uint8_t *)ctx.code_top),
				ctx.pc_entry_count, ctx.branch_patch_count);
		}

		func->jit_code = (bool (*)(struct rt_env *))ctx.code_top;
		rt_jit_context_dispose_tables(&ctx);
		return true;
	}
	return false;
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

static void
jit_x86_64_dump_code(struct rt_jit_context *ctx, void *generated_end)
{
	const char *dir;
	char name[96];
	char path[512];
	const char *src;
	size_t i, n;
	FILE *fp;

	dir = getenv("NOCT_JIT_DUMP_DIR");
	if (dir == NULL || dir[0] == '\0')
		return;
	src = ctx->func->name != NULL ? ctx->func->name : "anonymous";
	for (i = 0; src[i] != '\0' && i + 1 < sizeof(name); i++) {
		char c;

		c = src[i];
		name[i] = (c >= 'a' && c <= 'z') ||
			  (c >= 'A' && c <= 'Z') ||
			  (c >= '0' && c <= '9') || c == '_' || c == '-' ? c : '_';
	}
	name[i] = '\0';
	if (snprintf(path, sizeof(path), "%s/%s-%p.x86_64.bin", dir, name,
		     (void *)ctx->func->bytecode) >= (int)sizeof(path))
		return;
	fp = fopen(path, "wb");
	if (fp == NULL)
		return;
	n = (size_t)((uint8_t *)generated_end - (uint8_t *)ctx->code_top);
	(void)fwrite(ctx->code_top, 1, n, fp);
	(void)fclose(fp);
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

/* Put a instruction word. */
#define IQ(q)                        if (!jit_put_qword(ctx, q)) return false
static INLINE bool
jit_put_qword(
        struct rt_jit_context *ctx,
        uint64_t qw)
{
        if ((uint8_t *)ctx->code + 8 > (uint8_t *)ctx->code_end) {
		ctx->code_overflow = true;
                rt_error(ctx->env, N_TR("Code too big."));
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

#if defined(NOCT_USE_OPTIMIZER)
static INLINE void
jit_x86_64_invalidate_packed_load(struct rt_jit_context *ctx, int tmp)
{
	int i;

	for (i = 0; i < 3; i++) {
		if (ctx->gpr_load_tmp[i] == tmp)
			ctx->gpr_load_tmp[i] = -1;
	}
}

static INLINE void
jit_x86_64_invalidate_all_packed_loads(struct rt_jit_context *ctx)
{
	int i;

	for (i = 0; i < 3; i++)
		ctx->gpr_load_tmp[i] = -1;
}

static INLINE bool
jit_x86_64_gpr_is_cached(struct rt_jit_context *ctx, int tmp)
{
	int i;

	for (i = 0; i < 3; i++)
		if (ctx->gpr_load_tmp[i] == tmp) return true;
	return false;
}
#endif

/* Visit a OP_LINEINFO instruction. */
static INLINE bool
jit_visit_lineinfo_op(
        struct rt_jit_context *ctx)
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
		int dst_ofs;
		int src_ofs;
		int base_root;
		int i;

		dst_ofs = dst * (int)sizeof(struct rt_value);
		src_ofs = src * (int)sizeof(struct rt_value);
		if (!jit_x86_64_set_packed_base_alias(ctx, dst, src))
			return false;
		base_root = jit_x86_64_resolve_packed_base(ctx, dst);
		for (i = 0; i < 3; i++) {
			if (base_root == ctx->packed_loop_base_tmp[i])
				return true;
		}
		if (jit_x86_64_is_packed_index_alias(ctx, src)) {
			if (!jit_x86_64_add_packed_index_alias(ctx, dst)) {
				ctx->packed_loop_hint_active = false;
			} else {
				return true;
			}
		} else {
			jit_x86_64_remove_packed_index_alias(ctx, dst);
		}
		if (ctx->gpr_cache_active) {
			int src_reg;
			int dst_reg;
			unsigned pin;

			if (ctx->gpr_reg_limit == 1) {
				if (!jit_x86_64_gpr_publish_remat(ctx) ||
				    !jit_x86_64_gpr_flush_required(ctx))
					return false;
			} else {
				if (!jit_x86_64_gpr_get(ctx, src, 0, &src_reg))
					return false;
				pin = 1u << (src_reg - 8);
				if (!jit_x86_64_gpr_dest(ctx, dst, pin, &dst_reg) ||
				    !jit_x86_64_gpr_mov(ctx, dst_reg, src_reg))
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
				jit_x86_64_invalidate_packed_load(ctx, dst);
				return true;
			}
		}
		/* Preserve rbx/rsi/rdi, which hold Packed cursor state. */
		ASM {
			IB(0x49); IB(0x8b); IB(0x87); ID((uint32_t)src_ofs);
			IB(0x49); IB(0x8b); IB(0x8f); ID((uint32_t)(src_ofs + 8));
			IB(0x49); IB(0x89); IB(0x87); ID((uint32_t)dst_ofs);
			IB(0x49); IB(0x89); IB(0x8f); ID((uint32_t)(dst_ofs + 8));
		}
		return true;
	}
	if (ctx->tmp_fixed_type != NULL &&
	    ctx->tmp_fixed_type[dst] >= 0 &&
	    rt_jit_tmp_has_fixed_primitive_type(ctx, src,
					 ctx->tmp_fixed_type[dst])) {
		int dst_ofs;
		int src_ofs;

		dst_ofs = dst * (int)sizeof(struct rt_value);
		src_ofs = src * (int)sizeof(struct rt_value);

		ASM {
			IB(0x49); IB(0x8b); IB(0x87); ID((uint32_t)(src_ofs + 8));
			IB(0x49); IB(0x89); IB(0x87); ID((uint32_t)(dst_ofs + 8));
		}
		return true;
	}
	if (ctx->tmp_fixed_type != NULL && ctx->tmp_fixed_type[src] >= 0 &&
	    !ctx->tmp_frame_tag_known[src]) {
		int src_ofs;

		src_ofs = src * (int)sizeof(struct rt_value);
		ASM {
			IB(0x41); IB(0xc7); IB(0x87); ID((uint32_t)src_ofs);
			ID((uint32_t)ctx->tmp_fixed_type[src]);
		}
		}
#endif

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
	if (ctx->packed_loop_hint_active)
		jit_x86_64_remove_packed_index_alias(ctx, dst);
	if (ctx->packed_loop_hint_active)
		jit_x86_64_remove_packed_base_alias(ctx, dst);
	if (ctx->gpr_cache_active) {
		if (ctx->gpr_reg_limit == 1) {
			int reg;

			if (!jit_x86_64_gpr_dest(ctx, dst, 0, &reg))
				return false;
			ASM { IB(0x41); IB((uint8_t)(0xb8 + (reg & 7)));
			      ID(val); }
			ctx->gpr_tmp_dirty[dst] = 1;
			ctx->gpr_range_valid[dst] = 1;
			ctx->gpr_range_min[dst] = (int32_t)val;
			ctx->gpr_range_max[dst] = (int32_t)val;
			jit_x86_64_invalidate_packed_load(ctx, dst);
			return true;
		}
		if (ctx->gpr_tmp_reg[dst] >= 0) {
			int slot;

			slot = ctx->gpr_tmp_reg[dst];
			ctx->gpr_reg_tmp[slot] = -1;
			ctx->gpr_tmp_reg[dst] = -1;
		}
		ctx->gpr_remat_valid[dst] = 1;
		ctx->gpr_remat_value[dst] = (int32_t)val;
		ctx->gpr_tmp_dirty[dst] = 0;
		ctx->gpr_range_valid[dst] = 1;
		ctx->gpr_range_min[dst] = (int32_t)val;
		ctx->gpr_range_max[dst] = (int32_t)val;
		jit_x86_64_invalidate_packed_load(ctx, dst);
		return true;
	}
#endif

	write_tag = !rt_jit_tmp_has_fixed_primitive_type(ctx, dst,
						     NOCT_VALUE_INT);
        dst *= (int)sizeof(struct rt_value);

        /* env->frame->tmpvar[dst].type = NOCT_VALUE_INT; */
        /* env->frame->tmpvar[dst].val.i = val; */
        ASM {
                /* r13: exception_handler */
                /* r14: env */
                /* r15: &env->frame->tmpvar[0] */

                /* %rax = &env->frame->tmpvar[dst] */
                /* movq $dst -> %rax */            IB(0x48); IB(0xc7); IB(0xc0); ID((uint32_t)dst);
                /* addq %r15 -> %rax */            IB(0x4c); IB(0x01); IB(0xf8);

                /* env->frame->tmpvar[dst].type = NOCT_VALUE_INT */
                /* movl $0 -> (%rax), unless fixed and known */
		if (write_tag) { IB(0xc7); IB(0x00); ID(0); }

                /* env->frame->tmpvar[dst].val.i = val */
                /* movl $val ->8 (%rax) */         IB(0xc7); IB(0x40); IB(0x08); ID((uint32_t)val);
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

        /* env->frame->tmpvar[dst].type = RT_VALUE_LONG; */
        /* env->frame->tmpvar[dst].val.l = val; */
        ASM {
                /* r13: exception_handler */
                /* r14: env */
                /* r15: &env->frame->tmpvar[0] */

                /* %rax = &env->frame->tmpvar[dst] */
                /* movq $dst -> %rax */            IB(0x48); IB(0xc7); IB(0xc0); ID((uint32_t)dst);
                /* addq %r15 -> %rax */            IB(0x4c); IB(0x01); IB(0xf8);

                /* env->frame->tmpvar[dst].type = NOCT_VALUE_LONG */
                /* movl $5 -> 0(%rax), unless fixed and known */
		if (write_tag) { IB(0xc7); IB(0x00); ID(5); }

                /* env->frame->tmpvar[dst].val.l = val */
                /* movabs $val -> %rcx */          IB(0x48); IB(0xb9); IQ(val);
                /* movl %rcx -> 8(%rax) */         IB(0x48); IB(0x89); IB(0x48); IB(0x08);
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

        /* &env->frame->tmpvar[dst].type = RT_VALUE_INT; */
        /* &env->frame->tmpvar[dst].val.i = val; */
        ASM {
                /* r13: exception_handler */
                /* r14: env */
                /* r15: &env->frame->tmpvar[0] */

                /* movq $dst -> %rax */             IB(0x48); IB(0xc7); IB(0xc0); ID((uint32_t)dst);
                /* addq %r15 -> %rax */             IB(0x4c); IB(0x01); IB(0xf8);
                /* movl $1 -> (%rax), unless fixed and known */
		if (write_tag) { IB(0xc7); IB(0x00); ID(1); }
                /* movl $val -> 8(%rax) */          IB(0xc7); IB(0x40); IB(0x08); ID(val);
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

        /* env->frame->tmpvar[dst].type = RT_VALUE_DOUBLE; */
        /* env->frame->tmpvar[dst].val.lf = val; */
        ASM {
                /* r13: exception_handler */
                /* r14: env */
                /* r15: &env->frame->tmpvar[0] */

                /* %rax = &env->frame->tmpvar[dst] */
                /* movq $dst -> %rax */            IB(0x48); IB(0xc7); IB(0xc0); ID((uint32_t)dst);
                /* addq %r15 -> %rax */            IB(0x4c); IB(0x01); IB(0xf8);

                /* env->frame->tmpvar[dst].type = NOCT_VALUE_DOUBLE */
                /* movl $6 -> 0(%rax), unless fixed and known */
		if (write_tag) { IB(0xc7); IB(0x00); ID(6); }

                /* env->frame->tmpvar[dst].val.l = val */
                /* movabs $val -> %rcx */          IB(0x48); IB(0xb9); IQ(val);
                /* movl %rcx -> 8(%rax) */         IB(0x48); IB(0x89); IB(0x48); IB(0x08);
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
        struct rt_jit_context *ctx)
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
        struct rt_jit_context *ctx)
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

        /* &env->frame->tmpvar[dst].val.i++ */
        ASM {
                /* r13: exception_handler */
                /* r14: env */
                /* r15: &env->frame->tmpvar[0] */

                /* movq $dst -> %rax */      IB(0x48); IB(0xc7); IB(0xc0); ID((uint32_t)dst);
                /* addq %r15 -> %rax */      IB(0x4c); IB(0x01); IB(0xf8);
                /* addq $step, 8(%rax) */    IB(0x48); IB(0x83); IB(0x40); IB(0x08); IB((uint8_t)step);
        }

	return true;
}

#if defined(NOCT_USE_OPTIMIZER)
/* x86-64 vector-register encoders.  Keep the logical SIMD operations
 * three-address; legacy SSE lowering uses the same helpers and inserts a
 * copy only when the ISA requires destructive two-address execution.
 * GNU as/objdump high-register oracles used while reviewing these fields:
 *   vpsrld $24,xmm11,xmm10       c4 c1 29 72 d3 18
 *   vmulps xmm12,xmm11,xmm10    c4 41 20 59 d4
 *   vfmadd231ps xmm12,xmm11,xmm10 c4 42 21 b8 d4
 *   paddd xmm12,xmm10           66 45 0f fe d4
 * (operands above are written in AT&T source order). */
static INLINE bool
jit_x86_64_valid_xmm(int reg)
{
	return reg >= 0 && reg < 16;
}

static INLINE bool
jit_x86_64_put_rex_rr(struct rt_jit_context *ctx, int reg, int rm)
{
	uint8_t rex;

	if (!jit_x86_64_valid_xmm(reg) || !jit_x86_64_valid_xmm(rm))
		return false;
	rex = (uint8_t)(0x40 | ((reg & 8) != 0 ? 4 : 0) |
			      ((rm & 8) != 0 ? 1 : 0));
	return rex == 0x40 || jit_put_byte(ctx, rex);
}

/* Legacy SSE register/register instruction.  map is 1 for 0f, 2 for
 * 0f38, and 3 for 0f3a. */
static INLINE bool
jit_x86_64_put_sse_rr(struct rt_jit_context *ctx, uint8_t prefix, int map,
			 uint8_t opcode, int dst, int src)
{
	if (!jit_x86_64_valid_xmm(dst) || !jit_x86_64_valid_xmm(src))
		return false;
	if (prefix != 0 && !jit_put_byte(ctx, prefix))
		return false;
	if (!jit_x86_64_put_rex_rr(ctx, dst, src) ||
	    !jit_put_byte(ctx, 0x0f))
		return false;
	if (map == 2 && !jit_put_byte(ctx, 0x38))
		return false;
	if (map == 3 && !jit_put_byte(ctx, 0x3a))
		return false;
	return jit_put_byte(ctx, opcode) &&
		jit_put_byte(ctx, (uint8_t)(0xc0 | ((dst & 7) << 3) |
					       (src & 7)));
}

static INLINE bool
jit_x86_64_put_vex3(struct rt_jit_context *ctx, int map, int pp, int dst,
			int rm, int src1, bool has_src1)
{
	uint8_t b2, b3;

	if (!jit_x86_64_valid_xmm(dst) || !jit_x86_64_valid_xmm(rm) ||
	    (has_src1 && !jit_x86_64_valid_xmm(src1)))
		return false;
	b2 = (uint8_t)(((dst & 8) == 0 ? 0x80 : 0) | 0x40 |
		       ((rm & 8) == 0 ? 0x20 : 0) | (map & 0x1f));
	b3 = (uint8_t)((has_src1 ? ((~src1 & 15) << 3) : 0x78) |
		       (pp & 3));
	return jit_put_byte(ctx, 0xc4) && jit_put_byte(ctx, b2) &&
		jit_put_byte(ctx, b3);
}

/* VEX.NDS.128 register binary operation: dst = src1 op src2. */
static INLINE bool
jit_x86_64_put_vex_rrr(struct rt_jit_context *ctx, int map, int pp,
			  uint8_t opcode, int dst, int src1, int src2)
{
	if (!jit_x86_64_put_vex3(ctx, map, pp, dst, src2, src1, true))
		return false;
	return jit_put_byte(ctx, opcode) &&
		jit_put_byte(ctx, (uint8_t)(0xc0 | ((dst & 7) << 3) |
					       (src2 & 7)));
}

/* VEX.128 two-operand operation whose vvvv field is reserved. */
static INLINE bool
jit_x86_64_put_vex_rr(struct rt_jit_context *ctx, int map, int pp,
			 uint8_t opcode, int dst, int src)
{
	if (!jit_x86_64_put_vex3(ctx, map, pp, dst, src, 0, false))
		return false;
	return jit_put_byte(ctx, opcode) &&
		jit_put_byte(ctx, (uint8_t)(0xc0 | ((dst & 7) << 3) |
					       (src & 7)));
}

/* VEX immediate packed shift: vvvv encodes dst, ModRM.rm encodes src,
 * and ModRM.reg remains the opcode extension. */
static INLINE bool
jit_x86_64_put_vex_shift(struct rt_jit_context *ctx, int ext, int dst,
			    int src, uint8_t imm)
{
	if (!jit_x86_64_put_vex3(ctx, 1, 1, 0, src, dst, true))
		return false;
	return jit_put_byte(ctx, 0x72) &&
		jit_put_byte(ctx, (uint8_t)(0xc0 | ((ext & 7) << 3) |
					       (src & 7))) &&
		jit_put_byte(ctx, imm);
}

#endif
static INLINE void
jit_x86_64_patch_local_rel32(uint8_t *disp, uint8_t *target)
{
	int32_t rel;

	rel = (int32_t)(target - (disp + 4));
	memcpy(disp, &rel, sizeof(rel));
}

#if defined(NOCT_USE_OPTIMIZER)
static uint16_t
jit_x86_64_read_u16(const uint8_t *p)
{
	return (uint16_t)(((uint16_t)p[0] << 8) | p[1]);
}

static uint32_t
jit_x86_64_read_u32(const uint8_t *p)
{
	return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) |
	       ((uint32_t)p[2] << 8) | (uint32_t)p[3];
}

/* Strictly prove the call-free vector region before assigning rbx/rsi/rdi. */
static bool
jit_x86_64_scan_vector_loop(struct rt_jit_context *ctx, int index_tmp,
			    int remaining_tmp, int lanes)
{
	uint32_t p, body_lpc, size;
	uint16_t base, ofs, value;
	int i, inc_count;
	uint8_t op, imm, shift;

	ctx->vector_base_tmp[0] = -1;
	ctx->vector_base_tmp[1] = -1;
	ctx->vector_imm_value = -1;
	ctx->vector_imm_shift = -1;
	ctx->vector_imm_reg = -1;
	body_lpc = ctx->lpc;
	p = body_lpc;
	inc_count = 0;
	while (p < ctx->func->bytecode_size) {
		op = ctx->func->bytecode[p];
		size = 0;
		base = 0xffffu;
		switch (op) {
		case OP_VLOADI32X4:
		case OP_VLOADF32X4:
			if (p + 6 > ctx->func->bytecode_size) return false;
			base = jit_x86_64_read_u16(&ctx->func->bytecode[p + 2]);
			ofs = jit_x86_64_read_u16(&ctx->func->bytecode[p + 4]);
			if (ofs != (uint16_t)index_tmp) return false;
			size = 6; break;
		case OP_VSTOREI32X4:
		case OP_VSTOREF32X4:
			if (p + 6 > ctx->func->bytecode_size) return false;
			base = jit_x86_64_read_u16(&ctx->func->bytecode[p + 1]);
			ofs = jit_x86_64_read_u16(&ctx->func->bytecode[p + 3]);
			if (ofs != (uint16_t)index_tmp) return false;
			size = 6; break;
		case OP_VSPLATI32: case OP_VSPLATF32:
			size = 4; break;
		case OP_VGETLANEI32: case OP_VGETLANEF32:
			size = 5; break;
		case OP_VMOV128: case OP_VCVTI32F32X4:
		case OP_VCVTF32I32X4:
			size = 3; break;
		case OP_VADDI32X4: case OP_VSUBI32X4:
		case OP_VMULI32X4: case OP_VAND128: case OP_VOR128:
		case OP_VXOR128: case OP_VSHLI32X4: case OP_VSHRI32X4:
		case OP_VADDF32X4: case OP_VSUBF32X4:
		case OP_VMULF32X4: case OP_VDIVF32X4:
		case OP_VMINS32X4: case OP_VMAXS32X4:
			size = 4; break;
		case OP_VFMAF32X4:
			if ((ctx->simd_caps & JIT_SIMD_CAP_FMAF32X4) == 0)
				return false;
			size = 5; break;
		case OP_VCMPI32X4:
		case OP_VCMPF32X4:
		case OP_VSELECT128:
			size = 5; break;
		case OP_VMASKSTOREI32X4:
			if (p + 7 > ctx->func->bytecode_size ||
			    (ctx->simd_caps & JIT_SIMD_CAP_AVX) == 0) {
				/* A helper call cannot be mixed with register-canonical
				 * values.  Select the portable tier for the whole region. */
				ctx->simd_caps = 0;
				return false;
			}
			base = jit_x86_64_read_u16(&ctx->func->bytecode[p + 1]);
			ofs = jit_x86_64_read_u16(&ctx->func->bytecode[p + 3]);
			if (ofs != (uint16_t)index_tmp) return false;
			size = 7; break;
		case OP_VINDUCTF32X4:
			size = 6; break;
		case OP_VGATHERI32X4_CHECKED:
			size = 7; break;
		case OP_VORI32X4I:
			if (p + 5 > ctx->func->bytecode_size) return false;
			imm = ctx->func->bytecode[p + 3];
			shift = ctx->func->bytecode[p + 4];
			if (ctx->vector_imm_value >= 0 &&
			    (ctx->vector_imm_value != imm ||
			     ctx->vector_imm_shift != shift))
				return false;
			ctx->vector_imm_value = imm;
			ctx->vector_imm_shift = shift;
			size = 5; break;
		case OP_INC:
			if (p + 4 > ctx->func->bytecode_size ||
			    jit_x86_64_read_u16(&ctx->func->bytecode[p + 1]) !=
				(uint16_t)index_tmp ||
			    ctx->func->bytecode[p + 3] != (uint8_t)lanes)
				return false;
			inc_count++;
			size = 4; break;
		case OP_SUBJNZ:
			if (p + 8 > ctx->func->bytecode_size) return false;
			value = jit_x86_64_read_u16(&ctx->func->bytecode[p + 1]);
			if (value != (uint16_t)remaining_tmp ||
			    ctx->func->bytecode[p + 3] != (uint8_t)lanes ||
			    jit_x86_64_read_u32(&ctx->func->bytecode[p + 4]) !=
				body_lpc)
				return false;
			return ctx->vector_base_tmp[0] >= 0 && inc_count == 1;
		default:
			return false;
		}
		if (base != 0xffffu) {
			for (i = 0; i < 2; i++) {
				if (ctx->vector_base_tmp[i] == (int)base)
					break;
				if (ctx->vector_base_tmp[i] < 0) {
					ctx->vector_base_tmp[i] = (int)base;
					break;
				}
			}
			if (i == 2) return false;
		}
		p += size;
	}
	return false;
}

static bool
jit_x86_64_is_packed_index_alias(struct rt_jit_context *ctx, int tmp)
{
	int i;

	for (i = 0; i < ctx->packed_loop_index_alias_count; i++) {
		if (ctx->packed_loop_index_alias[i] == (uint16_t)tmp)
			return true;
	}
	return false;
}

static void
jit_x86_64_remove_packed_index_alias(struct rt_jit_context *ctx, int tmp)
{
	int i;

	for (i = 1; i < ctx->packed_loop_index_alias_count; i++) {
		if (ctx->packed_loop_index_alias[i] == (uint16_t)tmp) {
			ctx->packed_loop_index_alias[i] =
				ctx->packed_loop_index_alias[--ctx->packed_loop_index_alias_count];
			return;
		}
	}
}

static bool
jit_x86_64_add_packed_index_alias(struct rt_jit_context *ctx, int tmp)
{
	if (jit_x86_64_is_packed_index_alias(ctx, tmp))
		return true;
	if (ctx->packed_loop_index_alias_count >=
	    (int)(sizeof(ctx->packed_loop_index_alias) /
		  sizeof(ctx->packed_loop_index_alias[0])))
		return false;
	ctx->packed_loop_index_alias[ctx->packed_loop_index_alias_count++] =
		(uint16_t)tmp;
	return true;
}

static int
jit_x86_64_resolve_packed_base(struct rt_jit_context *ctx, int tmp)
{
	int i;

	for (i = ctx->packed_loop_base_alias_count - 1; i >= 0; i--) {
		if (ctx->packed_loop_base_alias_tmp[i] == (uint16_t)tmp)
			return ctx->packed_loop_base_alias_root[i];
	}
	return tmp;
}

static void
jit_x86_64_remove_packed_base_alias(struct rt_jit_context *ctx, int tmp)
{
	int i;

	for (i = 0; i < ctx->packed_loop_base_alias_count; i++) {
		if (ctx->packed_loop_base_alias_tmp[i] == (uint16_t)tmp) {
			ctx->packed_loop_base_alias_tmp[i] =
				ctx->packed_loop_base_alias_tmp[
					--ctx->packed_loop_base_alias_count];
			ctx->packed_loop_base_alias_root[i] =
				ctx->packed_loop_base_alias_root[
					ctx->packed_loop_base_alias_count];
			return;
		}
	}
}

static bool
jit_x86_64_set_packed_base_alias(struct rt_jit_context *ctx, int dst, int src)
{
	int root;

	root = jit_x86_64_resolve_packed_base(ctx, src);
	jit_x86_64_remove_packed_base_alias(ctx, dst);
	if (ctx->packed_loop_base_alias_count >=
	    (int)(sizeof(ctx->packed_loop_base_alias_tmp) /
		  sizeof(ctx->packed_loop_base_alias_tmp[0])))
		return false;
	ctx->packed_loop_base_alias_tmp[ctx->packed_loop_base_alias_count] =
		(uint16_t)dst;
	ctx->packed_loop_base_alias_root[ctx->packed_loop_base_alias_count] =
		(uint16_t)root;
	ctx->packed_loop_base_alias_count++;
	return true;
}

static void
jit_x86_64_gpr_reset(struct rt_jit_context *ctx)
{
	uint32_t i;

	for (i = 0; i < ctx->func->tmpvar_size; i++) {
		ctx->gpr_tmp_reg[i] = -1;
		ctx->gpr_tmp_dirty[i] = 0;
		ctx->gpr_remat_valid[i] = 0;
		ctx->gpr_range_valid[i] = 0;
	}
	for (i = 0; i < 4; i++)
		ctx->gpr_reg_tmp[i] = -1;
	ctx->gpr_next_victim = 0;
	for (i = 0; i < 3; i++) {
		ctx->gpr_load_tmp[i] = -1;
		ctx->gpr_load_opcode[i] = -1;
		ctx->gpr_load_disp[i] = 0;
	}
}

static int
jit_x86_64_gpr_limit(void)
{
	const char *value;
	char *end;
	long limit;

	value = getenv("NOCT_JIT_GPR_LIMIT");
	if (value == NULL || *value == '\0')
		return 4;
	limit = strtol(value, &end, 10);
	if (*end != '\0' || limit < 0)
		return 4;
	if (limit > 4)
		limit = 4;
	return (int)limit;
}

static bool
jit_x86_64_gpr_spill(struct rt_jit_context *ctx, int slot)
{
	int tmp;
	int ofs;
	int reg;
	bool cached;
	int i;

	tmp = ctx->gpr_reg_tmp[slot];
	if (tmp < 0)
		return true;
	cached = false;
	for (i = 0; i < 3; i++) {
		if (ctx->gpr_load_tmp[i] == tmp) {
			cached = true;
			break;
		}
	}
	if (!cached && !ctx->has_vector_ops && ctx->packed_loop_hint_active &&
	    ctx->tmp_compiler_temp != NULL && ctx->tmp_compiler_temp[tmp] &&
	    rt_jit_ploop_next_use_lpc(ctx, tmp, ctx->lpc) == UINT32_MAX) {
		ctx->gpr_dead_drops++;
		ctx->gpr_tmp_dirty[tmp] = 0;
	}
	if (ctx->gpr_tmp_dirty[tmp]) {
		ofs = tmp * (int)sizeof(struct rt_value);
		reg = 8 + slot;
		if (ctx->tmp_fixed_type == NULL ||
		    ctx->tmp_fixed_type[tmp] != NOCT_VALUE_INT ||
		    !ctx->tmp_frame_tag_known[tmp]) {
			ASM {
				IB(0x41); IB(0xc7); IB(0x87); ID((uint32_t)ofs);
				ID((uint32_t)NOCT_VALUE_INT);
			}
			if (ctx->tmp_fixed_type != NULL &&
			    ctx->tmp_fixed_type[tmp] == NOCT_VALUE_INT)
				ctx->tmp_frame_tag_known[tmp] = 1;
		}
		ASM {
			IB(0x45); IB(0x89);
			IB((uint8_t)(0x87 | ((reg & 7) << 3)));
			ID((uint32_t)(ofs + 8));
		}
		ctx->gpr_spills++;
	}
	ctx->gpr_tmp_reg[tmp] = -1;
	ctx->gpr_tmp_dirty[tmp] = 0;
	ctx->gpr_reg_tmp[slot] = -1;
	jit_x86_64_invalidate_packed_load(ctx, tmp);
	return true;
}

static bool
jit_x86_64_gpr_alloc(struct rt_jit_context *ctx, int tmp,
			     unsigned pin_mask, bool load, int *reg)
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
		*reg = 8 + slot;
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
			bool cached;

			candidate = i;
			cached = false;
			if ((pin_mask & (1u << candidate)) != 0)
				continue;
			held = ctx->gpr_reg_tmp[candidate];
			for (j = 0; j < 3; j++)
				if (ctx->gpr_load_tmp[j] == held) cached = true;
			next = cached ? ctx->lpc :
				rt_jit_ploop_next_use_lpc(ctx, held, ctx->lpc);
			if (slot < 0 || next == UINT32_MAX || next >= farthest) {
				slot = candidate;
				farthest = next;
				if (next == UINT32_MAX)
					break;
			}
		}
		if (slot < 0)
			return false;
		if (!jit_x86_64_gpr_spill(ctx, slot))
			return false;
	}
	ctx->gpr_reg_tmp[slot] = tmp;
	ctx->gpr_tmp_reg[tmp] = slot;
	ctx->gpr_tmp_dirty[tmp] = 0;
	*reg = 8 + slot;
	if (load) {
		if (ctx->gpr_remat_valid[tmp]) {
			ASM { IB(0x41); IB((uint8_t)(0xb8 + ((*reg) & 7)));
			      ID((uint32_t)ctx->gpr_remat_value[tmp]); }
		} else {
			ofs = tmp * (int)sizeof(struct rt_value);
			ASM {
				IB(0x45); IB(0x8b);
				IB((uint8_t)(0x87 | (((*reg) & 7) << 3)));
				ID((uint32_t)(ofs + 8));
			}
		}
	}
	return true;
}

static bool
jit_x86_64_gpr_get(struct rt_jit_context *ctx, int tmp,
			  unsigned pin_mask, int *reg)
{
	return jit_x86_64_gpr_alloc(ctx, tmp, pin_mask, true, reg);
}

static bool
jit_x86_64_gpr_dest(struct rt_jit_context *ctx, int tmp,
			   unsigned pin_mask, int *reg)
{
	ctx->gpr_remat_valid[tmp] = 0;
	return jit_x86_64_gpr_alloc(ctx, tmp, pin_mask, false, reg);
}

static bool
jit_x86_64_gpr_mov(struct rt_jit_context *ctx, int dst, int src)
{
	UNUSED_PARAMETER(ctx);

	if (dst == src)
		return true;
	ASM {
		IB(0x45); IB(0x89);
		IB((uint8_t)(0xc0 | ((src & 7) << 3) | (dst & 7)));
	}
	return true;
}

/* Transfer a dead source's physical register to a defining destination. */
static bool
jit_x86_64_gpr_rebind(struct rt_jit_context *ctx, int dst, int src, int *reg)
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
	ctx->gpr_remat_valid[dst] = 0;
	jit_x86_64_invalidate_packed_load(ctx, src);
	*reg = 8 + slot;
	return true;
}

static bool
jit_x86_64_gpr_flush(struct rt_jit_context *ctx)
{
	int slot;

	/* Cached Packed loads never carry across the cursor update. */
	jit_x86_64_invalidate_all_packed_loads(ctx);
	for (slot = 0; slot < 4; slot++) {
		if (!jit_x86_64_gpr_spill(ctx, slot))
			return false;
	}
	return true;
}

/* A generic instruction is about to consume the current values from the VM
 * frame, so even values with no later bytecode use must be published. */
static bool
jit_x86_64_gpr_flush_required(struct rt_jit_context *ctx)
{
	bool active;
	bool ok;

	active = ctx->packed_loop_hint_active;
	ctx->packed_loop_hint_active = false;
	ok = jit_x86_64_gpr_flush(ctx);
	ctx->packed_loop_hint_active = active;
	return ok;
}

/* A generic visitor reads the VM frame directly.  Publish constants that
 * were intentionally kept only as rematerialization facts before leaving
 * the register-cache fast path. */
static bool
jit_x86_64_gpr_publish_remat(struct rt_jit_context *ctx)
{
	uint32_t tmp;
	int ofs;

	for (tmp = 0; tmp < ctx->func->tmpvar_size; tmp++) {
		if (!ctx->gpr_remat_valid[tmp])
			continue;
		ofs = (int)tmp * (int)sizeof(struct rt_value);
		if (ctx->tmp_fixed_type == NULL ||
		    ctx->tmp_fixed_type[tmp] != NOCT_VALUE_INT ||
		    !ctx->tmp_frame_tag_known[tmp]) {
			ASM { IB(0x41); IB(0xc7); IB(0x87); ID((uint32_t)ofs);
			      ID((uint32_t)NOCT_VALUE_INT); }
			if (ctx->tmp_fixed_type != NULL &&
			    ctx->tmp_fixed_type[tmp] == NOCT_VALUE_INT)
				ctx->tmp_frame_tag_known[tmp] = 1;
		}
		ASM { IB(0x41); IB(0xc7); IB(0x87); ID((uint32_t)(ofs + 8));
		      ID((uint32_t)ctx->gpr_remat_value[tmp]); }
	}
	return true;
}

static INLINE bool
jit_visit_x86_64_ploop_hint_op(struct rt_jit_context *ctx)
{
	int stop_ofs;
	int remaining_ofs;
	int base_ofs;
	int scale_bits;
	int i;

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
		/* Rebuild aliases in bytecode order while visitors emit the region. */
		ctx->packed_loop_index_alias_count = 1;
		ctx->packed_loop_index_alias[0] =
			(uint16_t)ctx->packed_loop_index_tmp;
		ctx->packed_loop_index_alias_disp[0] = 0;
		ctx->packed_loop_base_alias_count = 0;
	}
	if (getenv("NOCT_JIT_REGCACHE_DEBUG") != NULL)
		fprintf(stderr,
			"noct-jit-regcache: func=%s hint lanes=%d flags=0x%x accepted=%d reason=%s\n",
			ctx->func->name != NULL ? ctx->func->name : "?",
			ctx->packed_loop_lanes, ctx->packed_loop_flags,
			ctx->packed_loop_hint_active ? 1 : 0,
			ctx->packed_loop_reject_reason != NULL ?
			ctx->packed_loop_reject_reason : "disabled");
	if (!ctx->packed_loop_hint_active)
		return true;
	jit_x86_64_gpr_reset(ctx);
	ctx->gpr_reg_limit = jit_x86_64_gpr_limit();
	ctx->gpr_cache_active = ctx->gpr_reg_limit > 0;

	/* rax=stop; each base register becomes raw_base + stop*scale. */
	stop_ofs = ctx->packed_loop_stop_tmp * (int)sizeof(struct rt_value);
	ASM { IB(0x49); IB(0x63); IB(0x87); ID((uint32_t)(stop_ofs + 8)); }
	for (i = 0; i < 3 && ctx->packed_loop_base_tmp[i] >= 0; i++) {
		base_ofs = ctx->packed_loop_base_tmp[i] *
			(int)sizeof(struct rt_value);
		scale_bits = ctx->packed_loop_base_scale[i] == 4 ? 2 :
			ctx->packed_loop_base_scale[i] == 2 ? 1 : 0;
		if (i == 0) {
			ASM {
				IB(0x49); IB(0x8b); IB(0x9f);
				ID((uint32_t)(base_ofs + 8));
				IB(0x48); IB(0x8d); IB(0x1c);
				IB((uint8_t)((scale_bits << 6) | 0x03));
			}
		} else if (i == 1) {
			ASM {
				IB(0x49); IB(0x8b); IB(0xb7);
				ID((uint32_t)(base_ofs + 8));
				IB(0x48); IB(0x8d); IB(0x34);
				IB((uint8_t)((scale_bits << 6) | 0x06));
			}
		} else {
			ASM {
				/* r12 = raw_base + stop * scale */
				IB(0x4d); IB(0x8b); IB(0xa7);
				ID((uint32_t)(base_ofs + 8));
				IB(0x4d); IB(0x8d); IB(0x24);
				IB((uint8_t)((scale_bits << 6) | 0x04));
			}
		}
	}
	/* rdi=-(stop-start), matching the adjusted-base cursor. */
	remaining_ofs = ctx->packed_loop_remaining_tmp *
		(int)sizeof(struct rt_value);
	ASM {
		IB(0x49); IB(0x63); IB(0xbf);
		ID((uint32_t)(remaining_ofs + 8));
		IB(0x48); IB(0xf7); IB(0xdf);
	}
	if (getenv("NOCT_JIT_REGCACHE_DEBUG") != NULL)
		fprintf(stderr,
			"noct-jit-regcache: func=%s mode=cursor bases=%d gprs=%d\n",
			ctx->func->name != NULL ? ctx->func->name : "?",
			ctx->packed_loop_base_tmp[2] >= 0 ? 3 :
			ctx->packed_loop_base_tmp[1] >= 0 ? 2 : 1,
			ctx->gpr_reg_limit);
	return true;
}

static INLINE bool
jit_visit_vindex_hint_op(struct rt_jit_context *ctx)
{
	int a, b, c, required_vregs, lanes, flags;
	int ofs, base_ofs;
	uint32_t imm_value;
	int portable_force;

	CONSUME_TMPVAR(a); CONSUME_TMPVAR(b); CONSUME_TMPVAR(c);
	CONSUME_IMM8(required_vregs); CONSUME_IMM8(lanes); CONSUME_IMM8(flags);
	ctx->vector_vreg_limit =
		(ctx->simd_caps & JIT_SIMD_CAP_AVX) != 0 ? 15 : 13;
	portable_force = (flags & VINDEX_FORCE_SCALAR) != 0;
	if (portable_force &&
	    (flags & (VINDEX_REQUIRE_INDUCT | VINDEX_REQUIRE_GATHER)) != 0 &&
	    (flags & ~(VINDEX_CURSOR_ONLY | VINDEX_WRITEBACK_STOP |
		       VINDEX_FORCE_SCALAR | VINDEX_REQUIRE_INDUCT |
		       VINDEX_REQUIRE_GATHER | VINDEX_REQUIRE_MASKSTORE)) == 0 &&
	    (ctx->simd_caps & JIT_SIMD_CAP_SSE41) != 0)
		portable_force = 0;
	if (required_vregs > ctx->vector_vreg_limit ||
	    portable_force ||
	    ((flags & VINDEX_REQUIRE_MASKSTORE) != 0 &&
	     (ctx->simd_caps & JIT_SIMD_CAP_AVX) == 0))
		ctx->simd_caps = 0;
	ctx->vector_hint_active = !IS_MSABI && lanes > 0 &&
		(ctx->simd_caps & JIT_SIMD_CAP_SSE2) != 0 &&
		(flags & VINDEX_CURSOR_ONLY) != 0 &&
		jit_x86_64_scan_vector_loop(ctx, a, c, lanes);
	ctx->vector_imm_reg = -1;
	if (ctx->vector_hint_active && ctx->vector_imm_value >= 0) {
		if (ctx->vector_vreg_limit <= 13)
			ctx->vector_imm_reg = 15;
		else if (required_vregs < ctx->vector_vreg_limit)
			/* Logical registers occupy [0, required_vregs).  Keep xmm15
			 * instruction-local and use the first otherwise-unused register. */
			ctx->vector_imm_reg = required_vregs;
	}
	ctx->vector_hint_index_tmp = a;
	ctx->vector_hint_stop_tmp = b;
	ctx->vector_hint_remaining_tmp = c;
	ctx->vector_hint_lanes = lanes;
	ctx->vector_hint_flags = flags;
	if ((flags & VINDEX_CURSOR_ONLY) != 0 &&
	    getenv("NOCT_JIT_VECTOR_DEBUG") != NULL) {
		int native;

		native = !IS_MSABI &&
			(ctx->simd_caps & JIT_SIMD_CAP_SSE2) != 0 &&
			required_vregs <= ctx->vector_vreg_limit;
		fprintf(stderr,
			"noct-jit-vector: func=%s required=%d peak=%d physical=%d scratch=%d spills=0 mode=%s cursor=%s imm=%s\n",
			ctx->func->name != NULL ? ctx->func->name : "?",
			required_vregs, required_vregs,
			native ? required_vregs : 0,
			native ? (ctx->vector_vreg_limit > 13 ? 1 : 3) : 0,
			native ? "native" : "portable",
			ctx->vector_hint_active ? "register" : "memory",
			ctx->vector_imm_value < 0 ? "none" :
			ctx->vector_imm_reg >= 0 ? "preheader" : "inline");
	}
	if (!ctx->vector_hint_active)
		return true;
	/* rax=stop; adjusted bases are raw_base + stop*4. */
	ofs = b * (int)sizeof(struct rt_value);
	ASM { IB(0x49); IB(0x63); IB(0x87); ID((uint32_t)(ofs + 8)); }
	base_ofs = ctx->vector_base_tmp[0] * (int)sizeof(struct rt_value);
	ASM {
		IB(0x49); IB(0x8b); IB(0x9f); ID((uint32_t)(base_ofs + 8));
		IB(0x48); IB(0x8d); IB(0x1c); IB(0x83);
	}
	if (ctx->vector_base_tmp[1] >= 0) {
		base_ofs = ctx->vector_base_tmp[1] *
			(int)sizeof(struct rt_value);
		ASM {
			IB(0x49); IB(0x8b); IB(0xb7); ID((uint32_t)(base_ofs + 8));
			IB(0x48); IB(0x8d); IB(0x34); IB(0x86);
		}
	}
	/* rdi = -(stop-start), sign extended to 64 bits. */
	ofs = c * (int)sizeof(struct rt_value);
	ASM {
		IB(0x49); IB(0x63); IB(0xbf); ID((uint32_t)(ofs + 8));
		IB(0x48); IB(0xf7); IB(0xdf);
	}
	if (ctx->vector_imm_reg >= 0) {
		imm_value = (uint32_t)ctx->vector_imm_value <<
			    ((uint32_t)ctx->vector_imm_shift & 31u);
		/* mov imm,eax; movd eax,xmmN; pshufd $0,xmmN,xmmN. */
		ASM { IB(0xb8); ID(imm_value); IB(0x66); }
		if ((ctx->vector_imm_reg & 8) != 0)
			ASM { IB(0x44); }
		ASM { IB(0x0f); IB(0x6e);
		      IB((uint8_t)(0xc0 | ((ctx->vector_imm_reg & 7) << 3)));
		      IB(0x66); }
		if ((ctx->vector_imm_reg & 8) != 0)
			ASM { IB(0x45); }
		ASM { IB(0x0f); IB(0x70);
		      IB((uint8_t)(0xc0 | ((ctx->vector_imm_reg & 7) << 3) |
				   (ctx->vector_imm_reg & 7)));
		      IB(0x00); }
	}
	return true;
}

static INLINE bool
jit_visit_subjnz_op(struct rt_jit_context *ctx)
{
	int value, decrement;
	uint32_t target_lpc;
	bool hinted;
	bool packed_hinted;
	int ofs;

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
	if (packed_hinted) {
		int index_ofs;
		int stop_ofs;

		/* rdi is negative remaining: advance by the loop factor. */
		ASM { IB(0x48); IB(0x83); IB(0xc7); IB((uint8_t)decrement); }
		ctx->branch_patch[ctx->branch_patch_count].code = ctx->code;
		ctx->branch_patch[ctx->branch_patch_count].lpc = target_lpc;
		ctx->branch_patch[ctx->branch_patch_count].type = PATCH_JNE;
		ctx->branch_patch_count++;
		ASM { IB(0x0f); IB(0x85); ID(0); }
		/* The backedge keeps register state canonical.  Spill live-outs
		 * only on the fall-through path, never once per iteration. */
		ctx->packed_loop_hint_active = false;
		if (ctx->gpr_cache_active && !jit_x86_64_gpr_flush(ctx))
			return false;
		ctx->gpr_cache_active = false;
		ofs = value * (int)sizeof(struct rt_value);
		index_ofs = ctx->packed_loop_index_tmp *
			(int)sizeof(struct rt_value);
		stop_ofs = ctx->packed_loop_stop_tmp *
			(int)sizeof(struct rt_value);
		ASM {
			IB(0x41); IB(0xc7); IB(0x87);
			ID((uint32_t)(ofs + 8)); ID(0);
			IB(0x41); IB(0x8b); IB(0x87);
			ID((uint32_t)(stop_ofs + 8));
			IB(0x41); IB(0x89); IB(0x87);
			ID((uint32_t)(index_ofs + 8));
		}
		if (getenv("NOCT_JIT_REGCACHE_DEBUG") != NULL)
			fprintf(stderr,
				"noct-jit-regcache: func=%s hits=%u misses=%u spills=%u proven-div=%u\n",
				ctx->func->name != NULL ? ctx->func->name : "?",
				ctx->gpr_hits, ctx->gpr_misses, ctx->gpr_spills,
				ctx->gpr_proven_divisions);
		return true;
	}
	hinted = ctx->vector_hint_active &&
		 value == ctx->vector_hint_remaining_tmp &&
		 decrement == ctx->vector_hint_lanes;
	if (hinted) {
		/* rdi is the negative remaining count: addq lanes; jne body. */
		ASM { IB(0x48); IB(0x83); IB(0xc7); IB((uint8_t)decrement); }
		ctx->branch_patch[ctx->branch_patch_count].code = ctx->code;
		ctx->branch_patch[ctx->branch_patch_count].lpc = target_lpc;
		ctx->branch_patch[ctx->branch_patch_count].type = PATCH_JNE;
		ctx->branch_patch_count++;
		ASM { IB(0x0f); IB(0x85); ID(0); }
		/* Preserve bytecode-visible state at loop exit. */
		ofs = value * (int)sizeof(struct rt_value);
		ASM {
			IB(0x41); IB(0xc7); IB(0x87); ID((uint32_t)(ofs + 8)); ID(0);
		}
		if ((ctx->vector_hint_flags & VINDEX_WRITEBACK_STOP) != 0) {
			int index_ofs;
			int stop_ofs;

			index_ofs = ctx->vector_hint_index_tmp *
				(int)sizeof(struct rt_value);
			stop_ofs = ctx->vector_hint_stop_tmp *
				(int)sizeof(struct rt_value);
			ASM {
				IB(0x41); IB(0x8b); IB(0x87); ID((uint32_t)(stop_ofs + 8));
				IB(0x41); IB(0x89); IB(0x87); ID((uint32_t)(index_ofs + 8));
			}
		}
		ctx->vector_hint_active = false;
		return true;
	}
	value *= (int)sizeof(struct rt_value);
	ASM {
		/* rax = &tmpvar[value] */
		IB(0x48); IB(0xc7); IB(0xc0); ID((uint32_t)value);
		IB(0x4c); IB(0x01); IB(0xf8);
		/* subl $decrement, 8(%rax) */
		IB(0x83); IB(0x68); IB(0x08); IB((uint8_t)decrement);
	}
	ctx->branch_patch[ctx->branch_patch_count].code = ctx->code;
	ctx->branch_patch[ctx->branch_patch_count].lpc = target_lpc;
	ctx->branch_patch[ctx->branch_patch_count].type = PATCH_JNE;
	ctx->branch_patch_count++;
	ASM { IB(0x0f); IB(0x85); ID(0); }
	return true;
}

static INLINE bool
jit_visit_vori32x4i_op(struct rt_jit_context *ctx)
{
	int dst, src, imm, shift;
	uint32_t value;
	int src1, src2;

	CONSUME_IMM8(dst); CONSUME_IMM8(src);
	CONSUME_IMM8(imm); CONSUME_IMM8(shift);
	if (dst < 0 || dst >= 16 || src < 0 || src >= 16) {
		rt_error(ctx->env, BROKEN_BYTECODE);
		return false;
	}
	if (IS_MSABI || (ctx->simd_caps & JIT_SIMD_CAP_SSE2) == 0) {
		src1 = src;
		src2 = (imm << 8) | shift;
		ASM_BINARY_OP(ex_vori32x4i_helper);
		return true;
	}
	if (dst >= ctx->vector_vreg_limit || src >= ctx->vector_vreg_limit) {
		rt_error(ctx->env, BROKEN_BYTECODE);
		return false;
	}
	if (ctx->vector_hint_active && ctx->vector_imm_reg >= 0 &&
	    ctx->vector_imm_value == imm &&
	    ctx->vector_imm_shift == shift) {
		if ((ctx->simd_caps & JIT_SIMD_CAP_AVX) != 0) {
			return jit_x86_64_put_vex_rrr(ctx, 1, 1, 0xeb,
						      dst, src, ctx->vector_imm_reg);
		}
		if (dst != src && !jit_x86_64_put_sse_rr(ctx, 0x66, 1,
							  0x6f, dst, src))
			return false;
		return jit_x86_64_put_sse_rr(ctx, 0x66, 1, 0xeb, dst,
					       ctx->vector_imm_reg);
	}
	value = (uint32_t)imm << ((uint32_t)shift & 31);
	/* In the wide AVX map xmm15 is the sole instruction-local scratch.
	 * Materialize this operation's immediate just before use so compare or
	 * select may freely reuse xmm15 earlier in the loop. */
	ASM {
		IB(0xb8); ID(value);
		IB(0x66); IB(0x44); IB(0x0f); IB(0x6e); IB(0xf8);
		IB(0x66); IB(0x45); IB(0x0f); IB(0x70); IB(0xff); IB(0x00);
	}
	if ((ctx->simd_caps & JIT_SIMD_CAP_AVX) != 0)
		return jit_x86_64_put_vex_rrr(ctx, 1, 1, 0xeb, dst, src, 15);
	if (dst != src && !jit_x86_64_put_sse_rr(ctx, 0x66, 1, 0x6f,
							  dst, src))
		return false;
	return jit_x86_64_put_sse_rr(ctx, 0x66, 1, 0xeb, dst, 15);
}

static INLINE bool
jit_visit_vfmaf32x4_op(struct rt_jit_context *ctx)
{
	int dst, src1, src2, addend;
	uint8_t opcode;

	CONSUME_IMM8(dst);
	CONSUME_IMM8(src1);
	CONSUME_IMM8(src2);
	CONSUME_IMM8(addend);
	if (dst < 0 || dst >= 16 || src1 < 0 || src1 >= 16 ||
	    src2 < 0 || src2 >= 16 || addend < 0 || addend >= 16) {
		rt_error(ctx->env, BROKEN_BYTECODE);
		return false;
	}
	if (IS_MSABI ||
	    (ctx->simd_caps & JIT_SIMD_CAP_FMAF32X4) == 0) {
		int packed_src2_addend;

		packed_src2_addend = (src2 << 8) | addend;
		src2 = packed_src2_addend;
		ASM_BINARY_OP(ex_vfmaf32x4_helper);
		return true;
	}
	if (dst < 0 || dst >= ctx->vector_vreg_limit ||
	    src1 < 0 || src1 >= ctx->vector_vreg_limit ||
	    src2 < 0 || src2 >= ctx->vector_vreg_limit ||
	    addend < 0 || addend >= ctx->vector_vreg_limit) {
		rt_error(ctx->env, BROKEN_BYTECODE);
		return false;
	}
	if (dst == addend) {
		/* vfmadd231ps dst, src1, src2 */
		opcode = 0xb8;
		return jit_x86_64_put_vex_rrr(ctx, 2, 1, opcode,
					      dst, src1, src2);
	} else if (dst == src1) {
		/* vfmadd213ps dst, src2, addend */
		opcode = 0xa8;
		return jit_x86_64_put_vex_rrr(ctx, 2, 1, opcode,
					      dst, src2, addend);
	} else if (dst == src2) {
		/* vfmadd213ps dst, src1, addend */
		opcode = 0xa8;
		return jit_x86_64_put_vex_rrr(ctx, 2, 1, opcode,
					      dst, src1, addend);
	} else {
		/* movdqa xmmAddend, xmmDst; vfmadd231ps dst,src1,src2 */
		if (!jit_x86_64_put_vex_rr(ctx, 1, 1, 0x6f, dst, addend))
			return false;
		opcode = 0xb8;
		return jit_x86_64_put_vex_rrr(ctx, 2, 1, opcode,
					      dst, src1, src2);
	}
}

/* Visit an OP_VCMPI32X4 instruction. */
static INLINE bool
jit_visit_vcmpi32x4_op(
        struct rt_jit_context *ctx)
{
	int dst;
	int src1;
	int src2;
	int pred;
	int left;
	int right;

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
	if (IS_MSABI || (ctx->simd_caps & JIT_SIMD_CAP_SSE2) == 0) {
		src2 = (src2 << 8) | pred;
		ASM_BINARY_OP(ex_vcmpi32x4_helper);
		return true;
	}
	if ((ctx->simd_caps & JIT_SIMD_CAP_AVX) != 0) {
		if (dst >= ctx->vector_vreg_limit ||
		    src1 >= ctx->vector_vreg_limit ||
		    src2 >= ctx->vector_vreg_limit)
			goto broken_vcmpi32x4;
		left = src1;
		right = src2;
		if (pred == VCMP_LT || pred == VCMP_GE) {
			left = src2;
			right = src1;
		}
		if (!jit_x86_64_put_vex_rrr(ctx, 1, 1,
				pred == VCMP_EQ || pred == VCMP_NE ? 0x76 : 0x66,
				dst, left, right))
			return false;
		if (pred == VCMP_NE || pred == VCMP_LE || pred == VCMP_GE) {
			/* Logical vregs occupy xmm0..xmm14 in the wide map. */
			if (!jit_x86_64_put_vex_rrr(ctx, 1, 1, 0x76,
						      15, 15, 15) ||
			    !jit_x86_64_put_vex_rrr(ctx, 1, 1, 0xef,
						      dst, dst, 15))
				return false;
		}
		return true;
	}
	if (dst >= 13 || src1 >= 13 || src2 >= 13)
		goto broken_vcmpi32x4;
	left = src1;
	right = src2;
	if (pred == VCMP_LT || pred == VCMP_GE) {
		left = src2;
		right = src1;
	}
	if (!jit_x86_64_put_sse_rr(ctx, 0x66, 1, 0x6f, 13, left))
		return false;
	if (pred == VCMP_EQ || pred == VCMP_NE) {
		if (!jit_x86_64_put_sse_rr(ctx, 0x66, 1, 0x76, 13, right))
			return false;
	} else if (!jit_x86_64_put_sse_rr(ctx, 0x66, 1, 0x66,
					       13, right)) {
		return false;
	}
	if (pred == VCMP_NE || pred == VCMP_LE || pred == VCMP_GE) {
		if (!jit_x86_64_put_sse_rr(ctx, 0x66, 1, 0x76, 14, 14) ||
		    !jit_x86_64_put_sse_rr(ctx, 0x66, 1, 0xef, 13, 14))
			return false;
	}
	if (dst != 13 && !jit_x86_64_put_sse_rr(ctx, 0x66, 1, 0x6f,
						    dst, 13))
		return false;
	return true;

broken_vcmpi32x4:
	rt_error(ctx->env, BROKEN_BYTECODE);
	return false;
}

/* Visit an OP_VCMPF32X4 instruction. */
static INLINE bool
jit_visit_vcmpf32x4_op(
        struct rt_jit_context *ctx)
{
	int dst;
	int src1;
	int src2;
	int pred;
	int left;
	int right;
	int imm;

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
	if (IS_MSABI || (ctx->simd_caps & JIT_SIMD_CAP_SSE2) == 0) {
		src2 = (src2 << 8) | pred;
		ASM_BINARY_OP(ex_vcmpf32x4_helper);
		return true;
	}
	if ((ctx->simd_caps & JIT_SIMD_CAP_AVX) != 0) {
		if (dst >= ctx->vector_vreg_limit ||
		    src1 >= ctx->vector_vreg_limit ||
		    src2 >= ctx->vector_vreg_limit)
			goto broken_vcmpf32x4;
		left = src1;
		right = src2;
		switch (pred) {
		case VCMP_EQ: imm = 0; break;
		case VCMP_NE: imm = 4; break;
		case VCMP_LT: imm = 1; break;
		case VCMP_LE: imm = 2; break;
		case VCMP_GT: imm = 1; left = src2; right = src1; break;
		case VCMP_GE: imm = 2; left = src2; right = src1; break;
		default: return false;
		}
		return jit_x86_64_put_vex_rrr(ctx, 1, 0, 0xc2,
						      dst, left, right) &&
		       jit_put_byte(ctx, (uint8_t)imm);
	}
	if (dst >= 13 || src1 >= 13 || src2 >= 13)
		goto broken_vcmpf32x4;
	left = src1;
	right = src2;
	switch (pred) {
	case VCMP_EQ: imm = 0; break;
	case VCMP_NE: imm = 4; break; /* unordered-or-not-equal */
	case VCMP_LT: imm = 1; break;
	case VCMP_LE: imm = 2; break;
	case VCMP_GT: imm = 1; left = src2; right = src1; break;
	case VCMP_GE: imm = 2; left = src2; right = src1; break;
	default: return false;
	}
	if (!jit_x86_64_put_sse_rr(ctx, 0, 1, 0x28, 13, left) ||
	    !jit_x86_64_put_sse_rr(ctx, 0, 1, 0xc2, 13, right) ||
	    !jit_put_byte(ctx, (uint8_t)imm))
		return false;
	if (dst != 13 && !jit_x86_64_put_sse_rr(ctx, 0x66, 1, 0x6f,
						    dst, 13))
		return false;
	return true;

broken_vcmpf32x4:
	rt_error(ctx->env, BROKEN_BYTECODE);
	return false;
}

static INLINE bool
jit_visit_vselect128_op(struct rt_jit_context *ctx)
{
	int dst, mask, src1, src2;

	CONSUME_IMM8(dst); CONSUME_IMM8(mask);
	CONSUME_IMM8(src1); CONSUME_IMM8(src2);
	if (dst < 0 || dst >= 16 || mask < 0 || mask >= 16 ||
	    src1 < 0 || src1 >= 16 || src2 < 0 || src2 >= 16) {
		rt_error(ctx->env, BROKEN_BYTECODE);
		return false;
	}
	if (IS_MSABI || (ctx->simd_caps & JIT_SIMD_CAP_SSE2) == 0) {
		src2 = (src1 << 8) | src2;
		src1 = mask;
		ASM_BINARY_OP(ex_vselect128_helper);
		return true;
	}
	if ((ctx->simd_caps & JIT_SIMD_CAP_AVX) != 0) {
		if (dst >= ctx->vector_vreg_limit ||
		    mask >= ctx->vector_vreg_limit ||
		    src1 >= ctx->vector_vreg_limit ||
		    src2 >= ctx->vector_vreg_limit)
			goto broken_vselect;
		/* xmm15 is instruction-local: true&mask, then false&~mask. */
		return jit_x86_64_put_vex_rrr(ctx, 1, 1, 0xdb,
						      15, mask, src1) &&
		       jit_x86_64_put_vex_rrr(ctx, 1, 1, 0xdf,
						      dst, mask, src2) &&
		       jit_x86_64_put_vex_rrr(ctx, 1, 1, 0xeb,
						      dst, dst, 15);
	}
	if (dst >= 13 || mask >= 13 || src1 >= 13 || src2 >= 13) {
		goto broken_vselect;
	}
	/* xmm13 = mask & true; xmm14 = ~mask & false. */
	if (!jit_x86_64_put_sse_rr(ctx, 0x66, 1, 0x6f, 13, mask) ||
	    !jit_x86_64_put_sse_rr(ctx, 0x66, 1, 0xdb, 13, src1) ||
	    !jit_x86_64_put_sse_rr(ctx, 0x66, 1, 0x6f, 14, mask) ||
	    !jit_x86_64_put_sse_rr(ctx, 0x66, 1, 0xdf, 14, src2) ||
	    !jit_x86_64_put_sse_rr(ctx, 0x66, 1, 0xeb, 13, 14))
		return false;
	if (dst != 13 && !jit_x86_64_put_sse_rr(ctx, 0x66, 1, 0x6f,
						    dst, 13))
		return false;
	return true;

broken_vselect:
	rt_error(ctx->env, BROKEN_BYTECODE);
	return false;
}

static INLINE bool
jit_visit_vmaskstorei32x4_op(struct rt_jit_context *ctx)
{
	int dst, src1, src2;
	int mask, base, ofs, cursor;

	CONSUME_TMPVAR(dst); CONSUME_TMPVAR(src1);
	CONSUME_IMM8(src2); CONSUME_IMM8(mask);
	if (src2 < 0 || src2 >= 16 || mask < 0 || mask >= 16) {
		rt_error(ctx->env, BROKEN_BYTECODE);
		return false;
	}
	if (!IS_MSABI && (ctx->simd_caps & JIT_SIMD_CAP_AVX) != 0) {
		if (src2 >= ctx->vector_vreg_limit ||
		    mask >= ctx->vector_vreg_limit) {
			rt_error(ctx->env, BROKEN_BYTECODE);
			return false;
		}
		cursor = -1;
		if (ctx->vector_hint_active) {
			if (ctx->vector_base_tmp[0] == dst) cursor = 0;
			else if (ctx->vector_base_tmp[1] == dst) cursor = 1;
		}
		if (cursor < 0) {
			base = dst * (int)sizeof(struct rt_value);
			ofs = src1 * (int)sizeof(struct rt_value);
			ASM {
				IB(0x49); IB(0x8b); IB(0x87); ID((uint32_t)(base + 8));
				IB(0x49); IB(0x63); IB(0x8f); ID((uint32_t)(ofs + 8));
			}
		}
		/* VEX.128.66.0f38.WIG 2e /r: vvvv=mask, reg=data. */
		if (!jit_x86_64_put_vex3(ctx, 2, 1, src2, 0, mask, true) ||
		    !jit_put_byte(ctx, 0x2e) ||
		    !jit_put_byte(ctx, (uint8_t)(0x04 | ((src2 & 7) << 3))) ||
		    !jit_put_byte(ctx, (uint8_t)(cursor == 0 ? 0xbb :
						 cursor == 1 ? 0xbe : 0x88)))
			return false;
		return true;
	}
	if ((ctx->simd_caps & JIT_SIMD_CAP_SSE2) != 0) {
		rt_error(ctx->env, BROKEN_BYTECODE);
		return false;
	}
	src2 = (src2 << 8) | mask;
	ASM_BINARY_OP(ex_vmaskstorei32x4_helper);
	return true;
}

static INLINE bool
jit_visit_vinductf32x4_op(struct rt_jit_context *ctx)
{
	int dst, src1, src2;
	int state_ofs, step_ofs, lane;

	CONSUME_IMM8(dst); CONSUME_TMPVAR(src1); CONSUME_TMPVAR(src2);
	if (dst < 0 || dst >= 16) {
		rt_error(ctx->env, BROKEN_BYTECODE);
		return false;
	}
	if (IS_MSABI || (ctx->simd_caps & JIT_SIMD_CAP_SSE41) == 0) {
		ASM_BINARY_OP(ex_vinductf32x4_helper);
		return true;
	}
	if (dst >= ctx->vector_vreg_limit) {
		rt_error(ctx->env, BROKEN_BYTECODE);
		return false;
	}
	state_ofs = src1 * (int)sizeof(struct rt_value) + 8;
	step_ofs = src2 * (int)sizeof(struct rt_value) + 8;
	/* xmm15 is the scalar recurrent state.  Each addss rounds before the
	 * next lane and the fourth result is written back exactly as the helper
	 * specifies. */
	ASM {
		IB(0xf3); IB(0x45); IB(0x0f); IB(0x10); IB(0xbf);
		ID((uint32_t)state_ofs);
	}
	if (!jit_x86_64_put_vex_rrr(ctx, 1, 1, 0xef, dst, dst, dst))
		return false;
	for (lane = 0; lane < 4; lane++) {
		if (!jit_x86_64_put_vex_rrr(ctx, 3, 1, 0x21,
						      dst, dst, 15) ||
		    !jit_put_byte(ctx, (uint8_t)(lane << 4)))
			return false;
		ASM {
			IB(0xf3); IB(0x45); IB(0x0f); IB(0x58); IB(0xbf);
			ID((uint32_t)step_ofs);
		}
	}
	ASM {
		IB(0xf3); IB(0x45); IB(0x0f); IB(0x11); IB(0xbf);
		ID((uint32_t)state_ofs);
	}
	return true;
}

static INLINE bool
jit_visit_vgatheri32x4_checked_op(struct rt_jit_context *ctx)
{
	int dst, src1, plen, vi, src2;
	int base_ofs, plen_ofs, lane;
	uint32_t vbase;
	uint8_t *fail_patch[8];
	uint8_t *done_patch;
	uint8_t *fail_target;
	uint8_t *done_target;
	int fail_count;

	CONSUME_IMM8(dst); CONSUME_TMPVAR(src1);
	CONSUME_TMPVAR(plen); CONSUME_IMM8(vi);
	if (dst < 0 || dst >= 16 || vi < 0 || vi >= 16) {
		rt_error(ctx->env, BROKEN_BYTECODE);
		return false;
	}
	if (IS_MSABI || (ctx->simd_caps & JIT_SIMD_CAP_SSE41) == 0) {
		src2 = (plen << 8) | vi;
		ASM_BINARY_OP(ex_vgatheri32x4_checked_helper);
		return true;
	}
	if (dst >= ctx->vector_vreg_limit || vi >= ctx->vector_vreg_limit) {
		rt_error(ctx->env, BROKEN_BYTECODE);
		return false;
	}
	base_ofs = src1 * (int)sizeof(struct rt_value) + 8;
	plen_ofs = plen * (int)sizeof(struct rt_value) + 8;
	vbase = (uint32_t)offsetof(struct rt_env, vreg) + (uint32_t)vi * 16;
	fail_count = 0;
	ASM {
		/* rax=packed bytes, edx=element count. */
		IB(0x49); IB(0x8b); IB(0x87); ID((uint32_t)base_ofs);
		IB(0x41); IB(0x8b); IB(0x97); ID((uint32_t)plen_ofs);
	}
	for (lane = 0; lane < 4; lane++) {
		/* pextrd ecx,xmmVi,lane */
		ASM { IB(0x66); }
		if ((vi & 8) != 0) { ASM { IB(0x44); } }
		ASM {
			IB(0x0f); IB(0x3a); IB(0x16);
			IB((uint8_t)(0xc1 | ((vi & 7) << 3)));
			IB((uint8_t)lane);
			IB(0x85); IB(0xc9);             /* test ecx,ecx */
			IB(0x0f); IB(0x88);             /* js fail */
		}
		fail_patch[fail_count++] = (uint8_t *)ctx->code;
		if (!jit_put_dword(ctx, 0)) return false;
		ASM {
			IB(0x39); IB(0xd1);             /* cmp ecx,edx */
			IB(0x0f); IB(0x83);             /* jae fail */
		}
		fail_patch[fail_count++] = (uint8_t *)ctx->code;
		if (!jit_put_dword(ctx, 0)) return false;
		ASM { IB(0x8b); IB(0x0c); IB(0x88); } /* mov ecx,[rax+rcx*4] */
		ASM { IB(0x66); }
		if ((dst & 8) != 0) { ASM { IB(0x44); } }
		ASM {
			IB(0x0f); IB(0x3a); IB(0x22);
			IB((uint8_t)(0xc1 | ((dst & 7) << 3)));
			IB((uint8_t)lane);
		}
	}
	ASM { IB(0xe9); }
	done_patch = (uint8_t *)ctx->code;
	if (!jit_put_dword(ctx, 0)) return false;

	fail_target = (uint8_t *)ctx->code;
	for (lane = 0; lane < fail_count; lane++)
		jit_x86_64_patch_local_rel32(fail_patch[lane], fail_target);
	/* Cold failure: synchronize only the index vector required by the
	 * canonical helper, which rechecks lanes in source order and preserves
	 * the existing diagnostic text. */
	ASM { IB(0xf3); }
	ASM { IB((uint8_t)((vi & 8) != 0 ? 0x45 : 0x41)); }
	ASM {
		IB(0x0f); IB(0x7f);
		IB((uint8_t)(0x86 | ((vi & 7) << 3))); ID(vbase);
	}
	src2 = (plen << 8) | vi;
	ASM_BINARY_OP(ex_vgatheri32x4_checked_helper);
	done_target = (uint8_t *)ctx->code;
	jit_x86_64_patch_local_rel32(done_patch, done_target);
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
                        /* movabs rt_loadsymbol_helper -> %rax */ IB(0x48); IB(0xb8); IQ((uint64_t)ex_loadsymbol_helper);
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
                        /* movabs rt_loadsymbol_helper -> %rax */ IB(0x48); IB(0xb8); IQ((uint64_t)ex_loadsymbol_helper);
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
        struct rt_jit_context *ctx)
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
                        /* movabs rt_storesymbol_helper -> %rax */ IB(0x48); IB(0xb8); IQ((uint64_t)ex_storesymbol_helper);
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
                        /* movabs rt_storesymbol_helper -> %rax */ IB(0x48); IB(0xb8); IQ((uint64_t)ex_storesymbol_helper);
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
        struct rt_jit_context *ctx)
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
                        /* movabs rt_loaddot_helper -> %rax */    IB(0x48); IB(0xb8); IQ((uint64_t)ex_loaddot_helper);
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
                        /* movabs rt_loaddot_helper -> %rax */    IB(0x48); IB(0xb8); IQ((uint64_t)ex_loaddot_helper);
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
        struct rt_jit_context *ctx)
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
                        /* movabs rt_storedot_helper -> %rax */  IB(0x48); IB(0xb8); IQ((uint64_t)ex_storedot_helper);
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
                        /* movabs ex_storedot_helper -> %rax */  IB(0x48); IB(0xb8); IQ((uint64_t)ex_storedot_helper);
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
                        /* jmp (5 + arg_count * 4) */
                        IB(0xe9);
                        ID((uint32_t)(4 * arg_count));
                }
                arg_addr = (uint64_t)(intptr_t)ctx->code;
                for (i = 0; i < arg_count; i++) {
                        memcpy(ctx->code, &arg[i], sizeof(arg[i]));
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
                        /* movabs ex_call_helper -> rax */       IB(0x48); IB(0xB8); IQ((uint64_t)ex_call_helper);
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
                        /* movabs ex_call_helper -> rax */       IB(0x48); IB(0xB8); IQ((uint64_t)ex_call_helper);
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
        struct rt_jit_context *ctx)
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
        CONSUME_TMPVAR(arg_tmp);
        symbol = NULL;
        symbol_len = 0;
        symbol_hash = (uint32_t)arg_tmp;
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
                memcpy(ctx->code, &arg[i], sizeof(arg[i]));
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
                        /* movabs ex_thiscall_helper -> %rax */        IB(0x48); IB(0xb8); IQ((uint64_t)ex_thiscall_helper);
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
                        /* movabs ex_thiscall_helper -> %r10 */       IB(0x49); IB(0xba); IQ((uint64_t)ex_thiscall_helper);
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
                /* je 6 */                                IB(0x0f); IB(0x84); ID(0);
        }

        return true;
}

/* Visit a OP_SAFEPOINT instruction. */
static INLINE bool
jit_visit_safepoint_op(
        struct rt_jit_context *ctx)
{
        if (IS_MSABI) {
                /* if (!rt_safepoint_helper(env)) return false; */
                ASM {
                        /* r13: exception_handler */
                        /* r14: env */
                        /* r14: &env->frame->tmpvar[0] */

                        /* subq %rsp, $32 */                      IB(0x48); IB(0x83); IB(0xec); IB(0x20);
                        /* (1st) movq %r14 -> %rcx */             IB(0x4c); IB(0x89); IB(0xf1);
                        /* movabs rt_safepoint_helper -> %rax */  IB(0x48); IB(0xb8); IQ((uint64_t)ex_safepoint_helper);
                        /* call *%rax */                          IB(0xff); IB(0xd0);
                        /* addq %rsp, $32 */                      IB(0x48); IB(0x83); IB(0xc4); IB(0x20);

                        /* testl %eax, %eax */                    IB(0x83); IB(0xf8); IB(0x00);
                        /* jne 8 <next> */                        IB(0x75); IB(0x03);
                        /* jmp *%r13 */                           IB(0x41); IB(0xff); IB(0xe5);
                /* next: */
                }
        } else {
                /* if (!rt_safepoint_helper(env)) return false; */
                ASM {
                        /* r13: exception_handler */
                        /* r14: env */
                        /* r14: &env->frame->tmpvar[0] */

                        /* (1st) movq %r14, %rdi */               IB(0x4c); IB(0x89); IB(0xf7);
                        /* movabs rt_safepoint_helper -> %rax */  IB(0x48); IB(0xb8); IQ((uint64_t)ex_safepoint_helper);
                        /* call *%rax */                          IB(0xff); IB(0xd0);

                        /* testl %eax, %eax */                    IB(0x83); IB(0xf8); IB(0x00);
                        /* jne 8 <next> */                        IB(0x75); IB(0x03);
                        /* jmp *%r13 */                           IB(0x41); IB(0xff); IB(0xe5);
                /* next:*/
                }
        }

        return true;
}

/* Fixed primitive slots expose their tag only at explicit materialization
 * boundaries.  Dynamic slots retain the canonical producer store. */
#define STORE_TAG_IF_DYNAMIC(dstofs, tag)                                      \
	if (!rt_jit_tmp_has_fixed_primitive_type(                                   \
		ctx, (int)((dstofs) / (int)sizeof(struct rt_value)), (tag))) {      \
		IB(0x41); IB(0xc7); IB(0x87); ID((uint32_t)(dstofs));              \
		ID((uint32_t)(tag));                                               \
	}

/* Visit a OP_PBASE instruction. (ABCE; inline machine code.)
 *
 * The ABCE guard has already proven the operand is a packed, so this
 * trusts the value and loads the payload pointer directly.
 */
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

        /* env->frame->tmpvar[dst].type = NOCT_VALUE_LONG; */
        /* env->frame->tmpvar[dst].val.l = (int64_t)packed->packed_buffer; */
        ASM {
                /* r15: &env->frame->tmpvar[0] */

                /* movq src+8(%r15) -> %rax */   IB(0x49); IB(0x8b); IB(0x87); ID((uint32_t)(src + 8));
                /* movq buf_ofs(%rax) -> %rax */ IB(0x48); IB(0x8b); IB(0x80); ID(buf_ofs);
                /* movl $LONG -> dst(%r15), for dynamic destinations */
		STORE_TAG_IF_DYNAMIC(dst, NOCT_VALUE_LONG);
                /* movq %rax -> dst+8(%r15) */   IB(0x49); IB(0x89); IB(0x87); ID((uint32_t)(dst + 8));
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

#if defined(NOCT_USE_OPTIMIZER)
static bool
jit_x86_64_packed_cursor(struct rt_jit_context *ctx, int base, int ofs,
			 int scale, uint8_t *sib, int *cursor,
			 int32_t *byte_disp)
{
	int i;
	int scale_bits;
	int base_root;
	int32_t element_disp;

	UNUSED_PARAMETER(ofs);

	if (!ctx->packed_loop_hint_active ||
	    !rt_jit_ploop_current_access_disp(ctx, &element_disp))
		return false;
	if (element_disp < INT32_MIN / scale ||
	    element_disp > INT32_MAX / scale)
		return false;
	*byte_disp = element_disp * scale;
	base_root = jit_x86_64_resolve_packed_base(ctx, base);
	for (i = 0; i < 3; i++) {
		if (ctx->packed_loop_base_tmp[i] == base_root &&
		    ctx->packed_loop_base_scale[i] == scale) {
			scale_bits = scale == 4 ? 2 : scale == 2 ? 1 : 0;
			*sib = (uint8_t)((scale_bits << 6) | 0x38 |
				(i == 0 ? 0x03 : i == 1 ? 0x06 : 0x04));
			*cursor = i;
			return true;
		}
	}
	return false;
}
#endif

/* Visit a OP_PLOAD8U instruction. (ABCE; inline machine code.) */
static INLINE bool
jit_visit_pload8u_op(
        struct rt_jit_context *ctx)
{
	int dst;
	int base;
	int ofs;
#if defined(NOCT_USE_OPTIMIZER)
	uint8_t sib;
	int dst_ofs;
	int cursor;
	int reg;
	int cached_tmp;
	int cached_reg;
	int opcode_key;
	int32_t byte_disp;
	uint8_t mod;
#endif

	CONSUME_TMPVAR(dst);
	CONSUME_TMPVAR(base);
	CONSUME_TMPVAR(ofs);
#if defined(NOCT_USE_OPTIMIZER)
	if (jit_x86_64_packed_cursor(ctx, base, ofs, 1, &sib,
				     &cursor, &byte_disp)) {
		jit_x86_64_remove_packed_index_alias(ctx, dst);
		jit_x86_64_remove_packed_base_alias(ctx, dst);
		if (ctx->gpr_cache_active) {
			opcode_key = 2;
			cached_tmp = ctx->gpr_load_tmp[cursor];
			if (ctx->gpr_reg_limit == 1 && cached_tmp != dst)
				cached_tmp = -1;
			if (cached_tmp >= 0 &&
			    ctx->gpr_load_opcode[cursor] == opcode_key &&
			    ctx->gpr_load_disp[cursor] == byte_disp &&
			    ctx->gpr_tmp_reg[cached_tmp] >= 0) {
				if (!jit_x86_64_gpr_get(ctx, cached_tmp, 0,
							&cached_reg))
					return false;
				if (rt_jit_ploop_next_use_lpc(ctx, cached_tmp,
							ctx->lpc) == UINT32_MAX) {
					if (!jit_x86_64_gpr_rebind(ctx, dst,
								 cached_tmp, &reg))
						return false;
				} else if (!jit_x86_64_gpr_dest(ctx, dst,
						1u << (cached_reg - 8), &reg) ||
					   !jit_x86_64_gpr_mov(ctx, reg, cached_reg)) {
					return false;
				}
			} else {
				mod = byte_disp == 0 ? 0x00 :
					byte_disp >= -128 && byte_disp <= 127 ?
					0x40 : 0x80;
				if (!jit_x86_64_gpr_dest(ctx, dst, 0, &reg))
					return false;
				ASM {
				        IB((uint8_t)(cursor == 2 ? 0x45 : 0x44));
				        IB(0x0f); IB(0xb6);
				        IB((uint8_t)(mod | ((reg & 7) << 3) | 0x04));
				        IB(sib);
				        if (mod == 0x40) IB((uint8_t)byte_disp);
				        if (mod == 0x80) ID((uint32_t)byte_disp);
				}
			}
			ctx->gpr_tmp_dirty[dst] = 1;
			ctx->gpr_range_valid[dst] = 0;
			ctx->gpr_load_tmp[cursor] = dst;
			ctx->gpr_load_opcode[cursor] = opcode_key;
			ctx->gpr_load_disp[cursor] = byte_disp;
			return true;
		}
		mod = byte_disp == 0 ? 0x00 :
			byte_disp >= -128 && byte_disp <= 127 ? 0x40 : 0x80;
		if (cursor == 2)
		        ASM { IB(0x41); }
		ASM {
		        IB(0x0f); IB(0xb6);
		        IB((uint8_t)(mod | 0x14)); IB(sib);
		        if (mod == 0x40) IB((uint8_t)byte_disp);
		        if (mod == 0x80) ID((uint32_t)byte_disp);
		}
		dst_ofs = dst * (int)sizeof(struct rt_value);
		ASM {
			STORE_TAG_IF_DYNAMIC(dst_ofs, NOCT_VALUE_INT);
			IB(0x41); IB(0x89); IB(0x97);
			ID((uint32_t)(dst_ofs + 8));
		}
		return true;
	}
#endif

	dst *= (int)sizeof(struct rt_value);
        base *= (int)sizeof(struct rt_value);
        ofs *= (int)sizeof(struct rt_value);

        /* dst.val.i = *(uint8_t *)(base.val.l + ofs.val.i); dst.type = INT; */
        ASM {
                /* r15: &env->frame->tmpvar[0] */

                /* movq base+8(%r15) -> %rax */    IB(0x49); IB(0x8b); IB(0x87); ID((uint32_t)(base + 8));
                /* movslq ofs+8(%r15) -> %rcx */   IB(0x49); IB(0x63); IB(0x8f); ID((uint32_t)(ofs + 8));
                /* movzbl (%rax,%rcx) -> %edx */   IB(0x0f); IB(0xb6); IB(0x14); IB(0x08);
                /* movl $INT -> dst(%r15), for dynamic destinations */
		STORE_TAG_IF_DYNAMIC(dst, NOCT_VALUE_INT);
                /* movl %edx -> dst+8(%r15) */     IB(0x41); IB(0x89); IB(0x97); ID((uint32_t)(dst + 8));
        }

        return true;
}

/* Visit a OP_PSTORE8 instruction. (ABCE; inline machine code.
 * Operand order: base, ofs, src.) */
static INLINE bool
jit_visit_pstore8_op(
        struct rt_jit_context *ctx)
{
	int base;
	int ofs;
	int src;
#if defined(NOCT_USE_OPTIMIZER)
	uint8_t sib;
	int src_ofs;
	int cursor;
	int reg;
	int32_t byte_disp;
	uint8_t mod;
#endif

	CONSUME_TMPVAR(base);
	CONSUME_TMPVAR(ofs);
	CONSUME_TMPVAR(src);
#if defined(NOCT_USE_OPTIMIZER)
	if (jit_x86_64_packed_cursor(ctx, base, ofs, 1, &sib,
				     &cursor, &byte_disp)) {
		if (ctx->gpr_cache_active) {
			mod = byte_disp == 0 ? 0x00 :
				byte_disp >= -128 && byte_disp <= 127 ?
				0x40 : 0x80;
			if (!jit_x86_64_gpr_get(ctx, src, 0, &reg))
				return false;
			ASM {
			        IB((uint8_t)(cursor == 2 ? 0x45 : 0x44));
			        IB(0x88);
			        IB((uint8_t)(mod | ((reg & 7) << 3) | 0x04));
			        IB(sib);
			        if (mod == 0x40) IB((uint8_t)byte_disp);
			        if (mod == 0x80) ID((uint32_t)byte_disp);
			}
			/* PLOOP eligibility requires restricted Packed roots. */
			ctx->gpr_load_tmp[cursor] = -1;
			return true;
		}
		mod = byte_disp == 0 ? 0x00 :
			byte_disp >= -128 && byte_disp <= 127 ? 0x40 : 0x80;
		src_ofs = src * (int)sizeof(struct rt_value);
		ASM {
			IB(0x41); IB(0x8b); IB(0x97);
			ID((uint32_t)(src_ofs + 8));
		}
		ASM {
		        if (cursor == 2) IB(0x41);
		        IB(0x88);
		        IB((uint8_t)(mod | 0x14));
		        IB(sib);
		        if (mod == 0x40) IB((uint8_t)byte_disp);
		        if (mod == 0x80) ID((uint32_t)byte_disp);
		}
		return true;
	}
#endif

	base *= (int)sizeof(struct rt_value);
        ofs *= (int)sizeof(struct rt_value);
        src *= (int)sizeof(struct rt_value);

        /* *(uint8_t *)(base.val.l + ofs.val.i) = (uint8_t)src.val.i; */
        ASM {
                /* r15: &env->frame->tmpvar[0] */

                /* movq base+8(%r15) -> %rax */    IB(0x49); IB(0x8b); IB(0x87); ID((uint32_t)(base + 8));
                /* movslq ofs+8(%r15) -> %rcx */   IB(0x49); IB(0x63); IB(0x8f); ID((uint32_t)(ofs + 8));
                /* movl src+8(%r15) -> %edx */     IB(0x41); IB(0x8b); IB(0x97); ID((uint32_t)(src + 8));
                /* movb %dl -> (%rax,%rcx) */      IB(0x88); IB(0x14); IB(0x08);
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
        int flags;

        CONSUME_TMPVAR(dst);
        CONSUME_IMM8(src);

	/* if (!ex_checktype_helper(env, slot, type)) return false; */
	ASM_UNARY_OP(ex_checktype_helper);
	flags = src & (TYPECHECK_RETURN_FLAG | TYPECHECK_LOCAL_FLAG);
	src &= ~(TYPECHECK_RETURN_FLAG | TYPECHECK_LOCAL_FLAG);
#if defined(NOCT_USE_OPTIMIZER)
	if (ctx->tmp_fixed_type != NULL &&
	    flags == 0 && ctx->tmp_fixed_type[dst] == src)
		ctx->tmp_frame_tag_known[dst] = 1;
#else
	UNUSED_PARAMETER(flags);
#endif

	return true;
}

/* Publish a fixed primitive tag at a dynamic observation boundary. */
static INLINE bool
jit_visit_x86_64_materialize_type_op(struct rt_jit_context *ctx)
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
		IB(0x41); IB(0xc7); IB(0x87); ID((uint32_t)ofs);
		ID((uint32_t)type);
	}
	return true;
}

/* Visit a OP_PLOAD8S instruction. (ABCE; inline machine code.) */
static INLINE bool
jit_visit_pload8s_op(
        struct rt_jit_context *ctx)
{
        int dst;
        int base;
        int ofs;
#if defined(NOCT_USE_OPTIMIZER)
	uint8_t sib;
	int dst_ofs;
	int cursor;
	int reg;
	int cached_tmp;
	int cached_reg;
	int opcode_key;
	int32_t byte_disp;
	uint8_t mod;
#endif

        CONSUME_TMPVAR(dst);
        CONSUME_TMPVAR(base);
        CONSUME_TMPVAR(ofs);
#if defined(NOCT_USE_OPTIMIZER)
	if (jit_x86_64_packed_cursor(ctx, base, ofs, 1, &sib,
				     &cursor, &byte_disp)) {
		jit_x86_64_remove_packed_index_alias(ctx, dst);
		jit_x86_64_remove_packed_base_alias(ctx, dst);
		if (ctx->gpr_cache_active) {
			opcode_key = 3;
			cached_tmp = ctx->gpr_load_tmp[cursor];
			if (ctx->gpr_reg_limit == 1 && cached_tmp != dst)
				cached_tmp = -1;
			if (cached_tmp >= 0 &&
			    ctx->gpr_load_opcode[cursor] == opcode_key &&
			    ctx->gpr_load_disp[cursor] == byte_disp &&
			    ctx->gpr_tmp_reg[cached_tmp] >= 0) {
				if (!jit_x86_64_gpr_get(ctx, cached_tmp, 0,
							&cached_reg))
					return false;
				if (rt_jit_ploop_next_use_lpc(ctx, cached_tmp,
							ctx->lpc) == UINT32_MAX) {
					if (!jit_x86_64_gpr_rebind(ctx, dst,
								 cached_tmp, &reg))
						return false;
				} else if (!jit_x86_64_gpr_dest(ctx, dst,
						1u << (cached_reg - 8), &reg) ||
					   !jit_x86_64_gpr_mov(ctx, reg, cached_reg)) {
					return false;
				}
			} else {
				mod = byte_disp == 0 ? 0x00 :
					byte_disp >= -128 && byte_disp <= 127 ?
					0x40 : 0x80;
				if (!jit_x86_64_gpr_dest(ctx, dst, 0, &reg))
					return false;
				ASM {
				        IB((uint8_t)(cursor == 2 ? 0x45 : 0x44));
				        IB(0x0f); IB(0xbe);
				        IB((uint8_t)(mod | ((reg & 7) << 3) | 0x04));
				        IB(sib);
				        if (mod == 0x40) IB((uint8_t)byte_disp);
				        if (mod == 0x80) ID((uint32_t)byte_disp);
				}
			}
			ctx->gpr_tmp_dirty[dst] = 1;
			ctx->gpr_range_valid[dst] = 0;
			ctx->gpr_load_tmp[cursor] = dst;
			ctx->gpr_load_opcode[cursor] = opcode_key;
			ctx->gpr_load_disp[cursor] = byte_disp;
			return true;
		}
		mod = byte_disp == 0 ? 0x00 :
			byte_disp >= -128 && byte_disp <= 127 ? 0x40 : 0x80;
		if (cursor == 2)
		        ASM { IB(0x41); }
		ASM {
		        IB(0x0f); IB(0xbe);
		        IB((uint8_t)(mod | 0x14)); IB(sib);
		        if (mod == 0x40) IB((uint8_t)byte_disp);
		        if (mod == 0x80) ID((uint32_t)byte_disp);
		}
		dst_ofs = dst * (int)sizeof(struct rt_value);
		ASM {
			STORE_TAG_IF_DYNAMIC(dst_ofs, NOCT_VALUE_INT);
			IB(0x41); IB(0x89); IB(0x97);
			ID((uint32_t)(dst_ofs + 8));
		}
		return true;
	}
#endif

        dst *= (int)sizeof(struct rt_value);
        base *= (int)sizeof(struct rt_value);
        ofs *= (int)sizeof(struct rt_value);

        ASM {
                /* r15: &env->frame->tmpvar[0] */

                /* movq base+8(%r15) -> %rax */    IB(0x49); IB(0x8b); IB(0x87); ID((uint32_t)(base + 8));
                /* movslq ofs+8(%r15) -> %rcx */   IB(0x49); IB(0x63); IB(0x8f); ID((uint32_t)(ofs + 8));
                /* load scaled -> %(e|r)dx */      IB(0x0f); IB(0xbe); IB(0x14); IB(0x08);
                /* movl $tag -> dst(%r15), for dynamic destinations */
		STORE_TAG_IF_DYNAMIC(dst, NOCT_VALUE_INT);
                /* movl %edx -> dst+8(%r15) */   IB(0x41); IB(0x89); IB(0x97); ID((uint32_t)(dst + 8));
        }

        return true;
}

/* Visit a OP_PLOAD16U instruction. (ABCE; inline machine code.) */
static INLINE bool
jit_visit_pload16u_op(
        struct rt_jit_context *ctx)
{
        int dst;
        int base;
        int ofs;
#if defined(NOCT_USE_OPTIMIZER)
	uint8_t sib;
	int dst_ofs;
	int cursor;
	int reg;
	int cached_tmp;
	int cached_reg;
	int opcode_key;
	int32_t byte_disp;
	uint8_t mod;
#endif

        CONSUME_TMPVAR(dst);
        CONSUME_TMPVAR(base);
        CONSUME_TMPVAR(ofs);
#if defined(NOCT_USE_OPTIMIZER)
	if (jit_x86_64_packed_cursor(ctx, base, ofs, 2, &sib,
				     &cursor, &byte_disp)) {
		jit_x86_64_remove_packed_index_alias(ctx, dst);
		jit_x86_64_remove_packed_base_alias(ctx, dst);
		if (ctx->gpr_cache_active) {
			opcode_key = 4;
			cached_tmp = ctx->gpr_load_tmp[cursor];
			if (ctx->gpr_reg_limit == 1 && cached_tmp != dst)
				cached_tmp = -1;
			if (cached_tmp >= 0 &&
			    ctx->gpr_load_opcode[cursor] == opcode_key &&
			    ctx->gpr_load_disp[cursor] == byte_disp &&
			    ctx->gpr_tmp_reg[cached_tmp] >= 0) {
				if (!jit_x86_64_gpr_get(ctx, cached_tmp, 0,
							&cached_reg))
					return false;
				if (rt_jit_ploop_next_use_lpc(ctx, cached_tmp,
							ctx->lpc) == UINT32_MAX) {
					if (!jit_x86_64_gpr_rebind(ctx, dst,
								 cached_tmp, &reg))
						return false;
				} else if (!jit_x86_64_gpr_dest(ctx, dst,
						1u << (cached_reg - 8), &reg) ||
					   !jit_x86_64_gpr_mov(ctx, reg, cached_reg)) {
					return false;
				}
			} else {
				mod = byte_disp == 0 ? 0x00 :
					byte_disp >= -128 && byte_disp <= 127 ?
					0x40 : 0x80;
				if (!jit_x86_64_gpr_dest(ctx, dst, 0, &reg))
					return false;
				ASM {
				        IB((uint8_t)(cursor == 2 ? 0x45 : 0x44));
				        IB(0x0f); IB(0xb7);
				        IB((uint8_t)(mod | ((reg & 7) << 3) | 0x04));
				        IB(sib);
				        if (mod == 0x40) IB((uint8_t)byte_disp);
				        if (mod == 0x80) ID((uint32_t)byte_disp);
				}
			}
			ctx->gpr_tmp_dirty[dst] = 1;
			ctx->gpr_range_valid[dst] = 0;
			ctx->gpr_load_tmp[cursor] = dst;
			ctx->gpr_load_opcode[cursor] = opcode_key;
			ctx->gpr_load_disp[cursor] = byte_disp;
			return true;
		}
		mod = byte_disp == 0 ? 0x00 :
			byte_disp >= -128 && byte_disp <= 127 ? 0x40 : 0x80;
		if (cursor == 2)
		        ASM { IB(0x41); }
		ASM {
		        IB(0x0f); IB(0xb7);
		        IB((uint8_t)(mod | 0x14)); IB(sib);
		        if (mod == 0x40) IB((uint8_t)byte_disp);
		        if (mod == 0x80) ID((uint32_t)byte_disp);
		}
		dst_ofs = dst * (int)sizeof(struct rt_value);
		ASM {
			STORE_TAG_IF_DYNAMIC(dst_ofs, NOCT_VALUE_INT);
			IB(0x41); IB(0x89); IB(0x97);
			ID((uint32_t)(dst_ofs + 8));
		}
		return true;
	}
#endif

        dst *= (int)sizeof(struct rt_value);
        base *= (int)sizeof(struct rt_value);
        ofs *= (int)sizeof(struct rt_value);

        ASM {
                /* r15: &env->frame->tmpvar[0] */

                /* movq base+8(%r15) -> %rax */    IB(0x49); IB(0x8b); IB(0x87); ID((uint32_t)(base + 8));
                /* movslq ofs+8(%r15) -> %rcx */   IB(0x49); IB(0x63); IB(0x8f); ID((uint32_t)(ofs + 8));
                /* load scaled -> %(e|r)dx */      IB(0x0f); IB(0xb7); IB(0x14); IB(0x48);
                /* movl $tag -> dst(%r15), for dynamic destinations */
		STORE_TAG_IF_DYNAMIC(dst, NOCT_VALUE_INT);
                /* movl %edx -> dst+8(%r15) */   IB(0x41); IB(0x89); IB(0x97); ID((uint32_t)(dst + 8));
        }

        return true;
}

/* Visit a OP_PLOAD16S instruction. (ABCE; inline machine code.) */
static INLINE bool
jit_visit_pload16s_op(
        struct rt_jit_context *ctx)
{
        int dst;
        int base;
        int ofs;
#if defined(NOCT_USE_OPTIMIZER)
	uint8_t sib;
	int dst_ofs;
	int cursor;
	int reg;
	int cached_tmp;
	int cached_reg;
	int opcode_key;
	int32_t byte_disp;
	uint8_t mod;
#endif

        CONSUME_TMPVAR(dst);
        CONSUME_TMPVAR(base);
        CONSUME_TMPVAR(ofs);
#if defined(NOCT_USE_OPTIMIZER)
	if (jit_x86_64_packed_cursor(ctx, base, ofs, 2, &sib,
				     &cursor, &byte_disp)) {
		jit_x86_64_remove_packed_index_alias(ctx, dst);
		jit_x86_64_remove_packed_base_alias(ctx, dst);
		if (ctx->gpr_cache_active) {
			opcode_key = 5;
			cached_tmp = ctx->gpr_load_tmp[cursor];
			if (ctx->gpr_reg_limit == 1 && cached_tmp != dst)
				cached_tmp = -1;
			if (cached_tmp >= 0 &&
			    ctx->gpr_load_opcode[cursor] == opcode_key &&
			    ctx->gpr_load_disp[cursor] == byte_disp &&
			    ctx->gpr_tmp_reg[cached_tmp] >= 0) {
				if (!jit_x86_64_gpr_get(ctx, cached_tmp, 0,
							&cached_reg))
					return false;
				if (rt_jit_ploop_next_use_lpc(ctx, cached_tmp,
							ctx->lpc) == UINT32_MAX) {
					if (!jit_x86_64_gpr_rebind(ctx, dst,
								 cached_tmp, &reg))
						return false;
				} else if (!jit_x86_64_gpr_dest(ctx, dst,
						1u << (cached_reg - 8), &reg) ||
					   !jit_x86_64_gpr_mov(ctx, reg, cached_reg)) {
					return false;
				}
			} else {
				mod = byte_disp == 0 ? 0x00 :
					byte_disp >= -128 && byte_disp <= 127 ?
					0x40 : 0x80;
				if (!jit_x86_64_gpr_dest(ctx, dst, 0, &reg))
					return false;
				ASM {
				        IB((uint8_t)(cursor == 2 ? 0x45 : 0x44));
				        IB(0x0f); IB(0xbf);
				        IB((uint8_t)(mod | ((reg & 7) << 3) | 0x04));
				        IB(sib);
				        if (mod == 0x40) IB((uint8_t)byte_disp);
				        if (mod == 0x80) ID((uint32_t)byte_disp);
				}
			}
			ctx->gpr_tmp_dirty[dst] = 1;
			ctx->gpr_range_valid[dst] = 0;
			ctx->gpr_load_tmp[cursor] = dst;
			ctx->gpr_load_opcode[cursor] = opcode_key;
			ctx->gpr_load_disp[cursor] = byte_disp;
			return true;
		}
		mod = byte_disp == 0 ? 0x00 :
			byte_disp >= -128 && byte_disp <= 127 ? 0x40 : 0x80;
		if (cursor == 2)
		        ASM { IB(0x41); }
		ASM {
		        IB(0x0f); IB(0xbf);
		        IB((uint8_t)(mod | 0x14)); IB(sib);
		        if (mod == 0x40) IB((uint8_t)byte_disp);
		        if (mod == 0x80) ID((uint32_t)byte_disp);
		}
		dst_ofs = dst * (int)sizeof(struct rt_value);
		ASM {
			STORE_TAG_IF_DYNAMIC(dst_ofs, NOCT_VALUE_INT);
			IB(0x41); IB(0x89); IB(0x97);
			ID((uint32_t)(dst_ofs + 8));
		}
		return true;
	}
#endif

        dst *= (int)sizeof(struct rt_value);
        base *= (int)sizeof(struct rt_value);
        ofs *= (int)sizeof(struct rt_value);

        ASM {
                /* r15: &env->frame->tmpvar[0] */

                /* movq base+8(%r15) -> %rax */    IB(0x49); IB(0x8b); IB(0x87); ID((uint32_t)(base + 8));
                /* movslq ofs+8(%r15) -> %rcx */   IB(0x49); IB(0x63); IB(0x8f); ID((uint32_t)(ofs + 8));
                /* load scaled -> %(e|r)dx */      IB(0x0f); IB(0xbf); IB(0x14); IB(0x48);
                /* movl $tag -> dst(%r15), for dynamic destinations */
		STORE_TAG_IF_DYNAMIC(dst, NOCT_VALUE_INT);
                /* movl %edx -> dst+8(%r15) */   IB(0x41); IB(0x89); IB(0x97); ID((uint32_t)(dst + 8));
        }

        return true;
}

/* Visit a OP_PLOAD32 instruction. (ABCE; inline machine code.) */
static INLINE bool
jit_visit_pload32_op(
        struct rt_jit_context *ctx)
{
        int dst;
        int base;
        int ofs;
#if defined(NOCT_USE_OPTIMIZER)
	uint8_t sib;
	int dst_ofs;
	int cursor;
	int reg;
	int cached_tmp;
	int cached_reg;
	int opcode_key;
	int32_t byte_disp;
	uint8_t mod;
#endif

        CONSUME_TMPVAR(dst);
        CONSUME_TMPVAR(base);
        CONSUME_TMPVAR(ofs);
#if defined(NOCT_USE_OPTIMIZER)
	if (jit_x86_64_packed_cursor(ctx, base, ofs, 4, &sib,
				     &cursor, &byte_disp)) {
		jit_x86_64_remove_packed_index_alias(ctx, dst);
		jit_x86_64_remove_packed_base_alias(ctx, dst);
		if (ctx->gpr_cache_active) {
			opcode_key = 8;
			cached_tmp = ctx->gpr_load_tmp[cursor];
			if (ctx->gpr_reg_limit == 1 && cached_tmp != dst)
				cached_tmp = -1;
			if (cached_tmp >= 0 &&
			    ctx->gpr_load_opcode[cursor] == opcode_key &&
			    ctx->gpr_load_disp[cursor] == byte_disp &&
			    ctx->gpr_tmp_reg[cached_tmp] >= 0) {
				if (!jit_x86_64_gpr_get(ctx, cached_tmp, 0,
							&cached_reg))
					return false;
				if (rt_jit_ploop_next_use_lpc(ctx, cached_tmp,
							ctx->lpc) == UINT32_MAX) {
					if (!jit_x86_64_gpr_rebind(ctx, dst,
								 cached_tmp, &reg))
						return false;
				} else if (!jit_x86_64_gpr_dest(ctx, dst,
						1u << (cached_reg - 8), &reg) ||
					   !jit_x86_64_gpr_mov(ctx, reg, cached_reg)) {
					return false;
				}
			} else {
				mod = byte_disp == 0 ? 0x00 :
					byte_disp >= -128 && byte_disp <= 127 ?
					0x40 : 0x80;
				if (!jit_x86_64_gpr_dest(ctx, dst, 0, &reg))
					return false;
				ASM {
				        IB((uint8_t)(cursor == 2 ? 0x45 : 0x44));
				        IB(0x8b);
				        IB((uint8_t)(mod | ((reg & 7) << 3) | 0x04));
				        IB(sib);
				        if (mod == 0x40) IB((uint8_t)byte_disp);
				        if (mod == 0x80) ID((uint32_t)byte_disp);
				}
			}
			ctx->gpr_tmp_dirty[dst] = 1;
			ctx->gpr_range_valid[dst] = 0;
			ctx->gpr_load_tmp[cursor] = dst;
			ctx->gpr_load_opcode[cursor] = opcode_key;
			ctx->gpr_load_disp[cursor] = byte_disp;
			return true;
		}
		mod = byte_disp == 0 ? 0x00 :
			byte_disp >= -128 && byte_disp <= 127 ? 0x40 : 0x80;
		if (cursor == 2)
		        ASM { IB(0x41); }
		ASM {
		        IB(0x8b); IB((uint8_t)(mod | 0x14)); IB(sib);
		        if (mod == 0x40) IB((uint8_t)byte_disp);
		        if (mod == 0x80) ID((uint32_t)byte_disp);
		}
		dst_ofs = dst * (int)sizeof(struct rt_value);
		ASM {
			STORE_TAG_IF_DYNAMIC(dst_ofs, NOCT_VALUE_INT);
			IB(0x41); IB(0x89); IB(0x97);
			ID((uint32_t)(dst_ofs + 8));
		}
		return true;
	}
#endif

        dst *= (int)sizeof(struct rt_value);
        base *= (int)sizeof(struct rt_value);
        ofs *= (int)sizeof(struct rt_value);

        ASM {
                /* r15: &env->frame->tmpvar[0] */

                /* movq base+8(%r15) -> %rax */    IB(0x49); IB(0x8b); IB(0x87); ID((uint32_t)(base + 8));
                /* movslq ofs+8(%r15) -> %rcx */   IB(0x49); IB(0x63); IB(0x8f); ID((uint32_t)(ofs + 8));
                /* load scaled -> %(e|r)dx */      IB(0x8b); IB(0x14); IB(0x88);
                /* movl $tag -> dst(%r15), for dynamic destinations */
		STORE_TAG_IF_DYNAMIC(dst, NOCT_VALUE_INT);
                /* movl %edx -> dst+8(%r15) */   IB(0x41); IB(0x89); IB(0x97); ID((uint32_t)(dst + 8));
        }

        return true;
}

/* Visit a OP_PLOAD64 instruction. (ABCE; inline machine code.) */
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
                /* r15: &env->frame->tmpvar[0] */

                /* movq base+8(%r15) -> %rax */    IB(0x49); IB(0x8b); IB(0x87); ID((uint32_t)(base + 8));
                /* movslq ofs+8(%r15) -> %rcx */   IB(0x49); IB(0x63); IB(0x8f); ID((uint32_t)(ofs + 8));
                /* load scaled -> %(e|r)dx */      IB(0x48); IB(0x8b); IB(0x14); IB(0xc8);
                /* movl $tag -> dst(%r15), for dynamic destinations */
		STORE_TAG_IF_DYNAMIC(dst, NOCT_VALUE_LONG);
                /* movq %rdx -> dst+8(%r15) */   IB(0x49); IB(0x89); IB(0x97); ID((uint32_t)(dst + 8));
        }

        return true;
}

/* Visit a OP_PSTORE16 instruction. (ABCE; inline. Int source per ABCE rules.) */
static INLINE bool
jit_visit_pstore16_op(
        struct rt_jit_context *ctx)
{
        int base;
        int ofs;
        int src;
#if defined(NOCT_USE_OPTIMIZER)
	uint8_t sib;
	int src_ofs;
	int cursor;
	int reg;
	int32_t byte_disp;
	uint8_t mod;
#endif

        CONSUME_TMPVAR(base);
        CONSUME_TMPVAR(ofs);
        CONSUME_TMPVAR(src);
#if defined(NOCT_USE_OPTIMIZER)
	if (jit_x86_64_packed_cursor(ctx, base, ofs, 2, &sib,
				     &cursor, &byte_disp)) {
		if (ctx->gpr_cache_active) {
			mod = byte_disp == 0 ? 0x00 :
				byte_disp >= -128 && byte_disp <= 127 ?
				0x40 : 0x80;
			if (!jit_x86_64_gpr_get(ctx, src, 0, &reg))
				return false;
			ASM {
			        IB(0x66);
			        IB((uint8_t)(cursor == 2 ? 0x45 : 0x44));
			        IB(0x89);
			        IB((uint8_t)(mod | ((reg & 7) << 3) | 0x04));
			        IB(sib);
			        if (mod == 0x40) IB((uint8_t)byte_disp);
			        if (mod == 0x80) ID((uint32_t)byte_disp);
			}
			/* PLOOP eligibility requires restricted Packed roots. */
			ctx->gpr_load_tmp[cursor] = -1;
			return true;
		}
		mod = byte_disp == 0 ? 0x00 :
			byte_disp >= -128 && byte_disp <= 127 ? 0x40 : 0x80;
		src_ofs = src * (int)sizeof(struct rt_value);
		ASM {
			IB(0x41); IB(0x8b); IB(0x97);
			ID((uint32_t)(src_ofs + 8));
		}
		ASM {
		        IB(0x66);
		        if (cursor == 2) IB(0x41);
		        IB(0x89);
		        IB((uint8_t)(mod | 0x14));
		        IB(sib);
		        if (mod == 0x40) IB((uint8_t)byte_disp);
		        if (mod == 0x80) ID((uint32_t)byte_disp);
		}
		return true;
	}
#endif

        base *= (int)sizeof(struct rt_value);
        ofs *= (int)sizeof(struct rt_value);
        src *= (int)sizeof(struct rt_value);

        ASM {
                /* r15: &env->frame->tmpvar[0] */

                /* movq base+8(%r15) -> %rax */    IB(0x49); IB(0x8b); IB(0x87); ID((uint32_t)(base + 8));
                /* movslq ofs+8(%r15) -> %rcx */   IB(0x49); IB(0x63); IB(0x8f); ID((uint32_t)(ofs + 8));
                /* movl src+8(%r15) -> %edx */     IB(0x41); IB(0x8b); IB(0x97); ID((uint32_t)(src + 8));
                /* store scaled */                 IB(0x66); IB(0x89); IB(0x14); IB(0x48);
        }

        return true;
}

/* Visit a OP_PSTORE32 instruction. (ABCE; inline. Int source per ABCE rules.) */
static INLINE bool
jit_visit_pstore32_op(
        struct rt_jit_context *ctx)
{
        int base;
        int ofs;
        int src;
#if defined(NOCT_USE_OPTIMIZER)
	uint8_t sib;
	int src_ofs;
	int cursor;
	int reg;
	int32_t byte_disp;
	uint8_t mod;
#endif

        CONSUME_TMPVAR(base);
        CONSUME_TMPVAR(ofs);
        CONSUME_TMPVAR(src);
#if defined(NOCT_USE_OPTIMIZER)
	if (jit_x86_64_packed_cursor(ctx, base, ofs, 4, &sib,
				     &cursor, &byte_disp)) {
		if (ctx->gpr_cache_active) {
			mod = byte_disp == 0 ? 0x00 :
				byte_disp >= -128 && byte_disp <= 127 ?
				0x40 : 0x80;
			if (!jit_x86_64_gpr_get(ctx, src, 0, &reg))
				return false;
			ASM {
			        IB((uint8_t)(cursor == 2 ? 0x45 : 0x44));
			        IB(0x89);
			        IB((uint8_t)(mod | ((reg & 7) << 3) | 0x04));
			        IB(sib);
			        if (mod == 0x40) IB((uint8_t)byte_disp);
			        if (mod == 0x80) ID((uint32_t)byte_disp);
			}
			/* PLOOP eligibility requires restricted Packed roots. */
			ctx->gpr_load_tmp[cursor] = -1;
			return true;
		}
		mod = byte_disp == 0 ? 0x00 :
			byte_disp >= -128 && byte_disp <= 127 ? 0x40 : 0x80;
		src_ofs = src * (int)sizeof(struct rt_value);
		ASM {
			IB(0x41); IB(0x8b); IB(0x97);
			ID((uint32_t)(src_ofs + 8));
		}
		ASM {
		        if (cursor == 2) IB(0x41);
		        IB(0x89);
		        IB((uint8_t)(mod | 0x14));
		        IB(sib);
		        if (mod == 0x40) IB((uint8_t)byte_disp);
		        if (mod == 0x80) ID((uint32_t)byte_disp);
		}
		return true;
	}
#endif

        base *= (int)sizeof(struct rt_value);
        ofs *= (int)sizeof(struct rt_value);
        src *= (int)sizeof(struct rt_value);

        ASM {
                /* r15: &env->frame->tmpvar[0] */

                /* movq base+8(%r15) -> %rax */    IB(0x49); IB(0x8b); IB(0x87); ID((uint32_t)(base + 8));
                /* movslq ofs+8(%r15) -> %rcx */   IB(0x49); IB(0x63); IB(0x8f); ID((uint32_t)(ofs + 8));
                /* movl src+8(%r15) -> %edx */     IB(0x41); IB(0x8b); IB(0x97); ID((uint32_t)(src + 8));
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


/*
 * Typed arithmetic ops (docs/design/07-typed-ops.md): inline machine
 * code.  Every op trusts the operand tags (proven by the LIR layer);
 * rax/rcx/rdx and xmm0 are per-op scratch, like the other emitters.
 *
 * The float comparisons are NaN-safe by construction: ucomiss sets
 * CF=ZF=PF=1 on an unordered compare, so seta/setae (which read
 * CF/ZF) yield 0 for NaN, matching the C semantics of the scalar
 * helpers.  "a < b" is emitted as "b > a" (load b, compare against
 * a, seta) so that the unordered case falls on the false side; never
 * replace seta/setae with setl/setle here.
 */

/* Emit "movl ofs+8(%r15), %eax". */
#define TYPED_LOAD_EAX(ofs)     IB(0x41); IB(0x8b); IB(0x87); ID((uint32_t)((ofs) + 8))
/* Emit "movss ofs+8(%r15), %xmm0". */
#define TYPED_LOAD_XMM0(ofs)    IB(0xf3); IB(0x41); IB(0x0f); IB(0x10); IB(0x87); ID((uint32_t)((ofs) + 8))
/* Emit "movl $tag, dst(%r15); movl %eax, dst+8(%r15)". */
#define TYPED_STORE_EAX(dst, tag, write_tag)                                                    \
        if (write_tag) {                                                                        \
        IB(0x41); IB(0xc7); IB(0x87); ID((uint32_t)(dst)); ID((uint32_t)(tag)); }               \
        IB(0x41); IB(0x89); IB(0x87); ID((uint32_t)((dst) + 8))
/* Emit "setcc %al; movzbl %al, %eax" (cc = setcc second opcode byte). */
#define TYPED_SETCC_EAX(cc)                                                                     \
        IB(0x0f); IB(cc); IB(0xc0);                                                             \
        IB(0x0f); IB(0xb6); IB(0xc0)

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
        if (ctx->packed_loop_hint_active)
                jit_x86_64_remove_packed_index_alias(ctx, dst);
        if (ctx->packed_loop_hint_active)
                jit_x86_64_remove_packed_base_alias(ctx, dst);
        if (ctx->gpr_cache_active && ctx->gpr_reg_limit != 1) {
                int r1;
                int r2;
                int rd;
                unsigned pins;
                bool v1;
                bool v2;
                int32_t min1;
                int32_t max1;
                int32_t min2;
                int32_t max2;
                int64_t lo;
                int64_t hi;
                bool imm2;

                v1 = ctx->gpr_range_valid[src1] != 0;
                min1 = v1 ? ctx->gpr_range_min[src1] : 0;
                max1 = v1 ? ctx->gpr_range_max[src1] : 0;
                v2 = ctx->gpr_range_valid[src2] != 0;
                min2 = v2 ? ctx->gpr_range_min[src2] : 0;
                max2 = v2 ? ctx->gpr_range_max[src2] : 0;
                imm2 = ctx->gpr_remat_valid[src2] != 0;
                if (!jit_x86_64_gpr_get(ctx, src1, 0, &r1))
                        return false;
                pins = 1u << (r1 - 8);
                if (imm2) {
                        int32_t imm;

                        imm = ctx->gpr_remat_value[src2];
                        if (dst == src1) {
                                rd = r1;
                        } else if (!jit_x86_64_gpr_is_cached(ctx, src1) &&
                                   rt_jit_ploop_next_use_lpc(ctx, src1,
                                                         ctx->lpc) ==
                                   UINT32_MAX) {
                                if (!jit_x86_64_gpr_rebind(ctx, dst, src1,
                                                         &rd))
                                        return false;
                        } else {
                                if (!jit_x86_64_gpr_dest(ctx, dst, pins,
                                                       &rd) ||
                                    !jit_x86_64_gpr_mov(ctx, rd, r1))
                                        return false;
                        }
                        ASM {
                                IB(0x41); IB(0x81);
                                IB((uint8_t)(0xc0 | (rd & 7)));
                                ID((uint32_t)imm);
                        }
                } else {
                        if (!jit_x86_64_gpr_get(ctx, src2, pins, &r2))
                                return false;
                        pins |= 1u << (r2 - 8);
                        if (dst == src1) {
                                rd = r1;
                        } else if (dst == src2) {
                                rd = r2;
                                r2 = r1;
                        } else if (!jit_x86_64_gpr_is_cached(ctx, src1) &&
                                   rt_jit_ploop_next_use_lpc(ctx, src1,
                                                         ctx->lpc) ==
                                   UINT32_MAX) {
                                if (!jit_x86_64_gpr_rebind(ctx, dst, src1,
                                                         &rd))
                                        return false;
                        } else if (!jit_x86_64_gpr_is_cached(ctx, src2) &&
                                   rt_jit_ploop_next_use_lpc(ctx, src2,
                                                         ctx->lpc) ==
                                   UINT32_MAX) {
                                if (!jit_x86_64_gpr_rebind(ctx, dst, src2,
                                                         &rd))
                                        return false;
                                r2 = r1;
                        } else {
                                if (!jit_x86_64_gpr_dest(ctx, dst, pins,
                                                       &rd) ||
                                    !jit_x86_64_gpr_mov(ctx, rd, r1))
                                        return false;
                        }
                        ASM {
                                IB(0x45); IB(0x03);
                                IB((uint8_t)(0xc0 | ((rd & 7) << 3) |
                                             (r2 & 7)));
                        }
                }
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
                jit_x86_64_invalidate_packed_load(ctx, dst);
                return true;
        }
        if (ctx->gpr_cache_active) {
                if (!jit_x86_64_gpr_publish_remat(ctx) ||
                    !jit_x86_64_gpr_flush_required(ctx))
                        return false;
        }

#endif
        dst_tmp = dst;
        result_type = NOCT_VALUE_INT;
        write_tag = !rt_jit_tmp_has_fixed_primitive_type(ctx, dst_tmp,
                                                     result_type);

        dst *= (int)sizeof(struct rt_value);
        src2 *= (int)sizeof(struct rt_value);
        src1 *= (int)sizeof(struct rt_value);

        ASM {
                /* r15: &env->frame->tmpvar[0] */
                TYPED_LOAD_EAX(src1);
        }
        ASM { IB(0x41); IB(0x03); IB(0x87); ID((uint32_t)(src2 + 8)); }
        ASM {
                TYPED_STORE_EAX(dst, NOCT_VALUE_INT, write_tag);
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
        if (ctx->packed_loop_hint_active)
                jit_x86_64_remove_packed_index_alias(ctx, dst);
        if (ctx->packed_loop_hint_active)
                jit_x86_64_remove_packed_base_alias(ctx, dst);
        if (ctx->gpr_cache_active && ctx->gpr_reg_limit != 1) {
                int r1;
                int r2;
                int rd;
                unsigned pins;
                bool v1;
                bool v2;
                int32_t min1;
                int32_t max1;
                int32_t min2;
                int32_t max2;
                int64_t lo;
                int64_t hi;
                bool imm2;

                v1 = ctx->gpr_range_valid[src1] != 0;
                min1 = v1 ? ctx->gpr_range_min[src1] : 0;
                max1 = v1 ? ctx->gpr_range_max[src1] : 0;
                v2 = ctx->gpr_range_valid[src2] != 0;
                min2 = v2 ? ctx->gpr_range_min[src2] : 0;
                max2 = v2 ? ctx->gpr_range_max[src2] : 0;
                imm2 = ctx->gpr_remat_valid[src2] != 0;
                if (!jit_x86_64_gpr_get(ctx, src1, 0, &r1))
                        return false;
                pins = 1u << (r1 - 8);
                if (imm2) {
                        int32_t imm;

                        imm = ctx->gpr_remat_value[src2];
                        if (dst == src1) {
                                rd = r1;
                        } else if (!jit_x86_64_gpr_is_cached(ctx, src1) &&
                                   rt_jit_ploop_next_use_lpc(ctx, src1,
                                                         ctx->lpc) ==
                                   UINT32_MAX) {
                                if (!jit_x86_64_gpr_rebind(ctx, dst, src1,
                                                         &rd))
                                        return false;
                        } else {
                                if (!jit_x86_64_gpr_dest(ctx, dst, pins,
                                                       &rd) ||
                                    !jit_x86_64_gpr_mov(ctx, rd, r1))
                                        return false;
                        }
                        ASM {
                                IB(0x41); IB(0x81);
                                IB((uint8_t)(0xc0 | (5 << 3) | (rd & 7)));
                                ID((uint32_t)imm);
                        }
                } else {
                        if (!jit_x86_64_gpr_get(ctx, src2, pins, &r2))
                                return false;
                        pins |= 1u << (r2 - 8);
                        if (dst == src1) {
                                rd = r1;
                        } else if (dst == src2) {
                                goto fallback_isub;
                        } else if (!jit_x86_64_gpr_is_cached(ctx, src1) &&
                                   rt_jit_ploop_next_use_lpc(ctx, src1,
                                                         ctx->lpc) ==
                                   UINT32_MAX) {
                                if (!jit_x86_64_gpr_rebind(ctx, dst, src1,
                                                         &rd))
                                        return false;
                        } else {
                                if (!jit_x86_64_gpr_dest(ctx, dst, pins,
                                                       &rd) ||
                                    !jit_x86_64_gpr_mov(ctx, rd, r1))
                                        return false;
                        }
                        ASM {
                                IB(0x45); IB(0x2b);
                                IB((uint8_t)(0xc0 | ((rd & 7) << 3) |
                                             (r2 & 7)));
                        }
                }
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
                jit_x86_64_invalidate_packed_load(ctx, dst);
                return true;
        }

fallback_isub:
        if (ctx->gpr_cache_active) {
                if (!jit_x86_64_gpr_publish_remat(ctx) ||
                    !jit_x86_64_gpr_flush_required(ctx))
                        return false;
        }

#endif
        dst_tmp = dst;
        result_type = NOCT_VALUE_INT;
        write_tag = !rt_jit_tmp_has_fixed_primitive_type(ctx, dst_tmp,
                                                     result_type);

        dst *= (int)sizeof(struct rt_value);
        src2 *= (int)sizeof(struct rt_value);
        src1 *= (int)sizeof(struct rt_value);

        ASM {
                /* r15: &env->frame->tmpvar[0] */
                TYPED_LOAD_EAX(src1);
        }
        ASM { IB(0x41); IB(0x2b); IB(0x87); ID((uint32_t)(src2 + 8)); }
        ASM {
                TYPED_STORE_EAX(dst, NOCT_VALUE_INT, write_tag);
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
        if (ctx->packed_loop_hint_active)
                jit_x86_64_remove_packed_index_alias(ctx, dst);
        if (ctx->packed_loop_hint_active)
                jit_x86_64_remove_packed_base_alias(ctx, dst);
        if (ctx->gpr_cache_active && ctx->gpr_reg_limit != 1) {
                int r1;
                int r2;
                int rd;
                unsigned pins;
                bool imm2;

                imm2 = ctx->gpr_remat_valid[src2] != 0;
                if (!jit_x86_64_gpr_get(ctx, src1, 0, &r1))
                        return false;
                pins = 1u << (r1 - 8);
                if (imm2) {
                        int32_t imm;

                        imm = ctx->gpr_remat_value[src2];
                        if (dst == src1) {
                                rd = r1;
                        } else if (!jit_x86_64_gpr_is_cached(ctx, src1) &&
                                   rt_jit_ploop_next_use_lpc(ctx, src1,
                                                         ctx->lpc) ==
                                   UINT32_MAX) {
                                if (!jit_x86_64_gpr_rebind(ctx, dst, src1,
                                                         &rd))
                                        return false;
                        } else {
                                if (!jit_x86_64_gpr_dest(ctx, dst, pins,
                                                       &rd) ||
                                    !jit_x86_64_gpr_mov(ctx, rd, r1))
                                        return false;
                        }
                        ASM {
                                IB(0x45); IB(0x69);
                                IB((uint8_t)(0xc0 | ((rd & 7) << 3) |
                                             (rd & 7)));
                                ID((uint32_t)imm);
                        }
                } else {
                        if (!jit_x86_64_gpr_get(ctx, src2, pins, &r2))
                                return false;
                        pins |= 1u << (r2 - 8);
                        if (dst == src1) {
                                rd = r1;
                        } else if (dst == src2) {
                                rd = r2;
                                r2 = r1;
                        } else if (!jit_x86_64_gpr_is_cached(ctx, src1) &&
                                   rt_jit_ploop_next_use_lpc(ctx, src1,
                                                         ctx->lpc) ==
                                   UINT32_MAX) {
                                if (!jit_x86_64_gpr_rebind(ctx, dst, src1,
                                                         &rd))
                                        return false;
                        } else if (!jit_x86_64_gpr_is_cached(ctx, src2) &&
                                   rt_jit_ploop_next_use_lpc(ctx, src2,
                                                         ctx->lpc) ==
                                   UINT32_MAX) {
                                if (!jit_x86_64_gpr_rebind(ctx, dst, src2,
                                                         &rd))
                                        return false;
                                r2 = r1;
                        } else {
                                if (!jit_x86_64_gpr_dest(ctx, dst, pins,
                                                       &rd) ||
                                    !jit_x86_64_gpr_mov(ctx, rd, r1))
                                        return false;
                        }
                        ASM {
                                IB(0x45); IB(0x0f); IB(0xaf);
                                IB((uint8_t)(0xc0 | ((rd & 7) << 3) |
                                             (r2 & 7)));
                        }
                }
                ctx->gpr_tmp_dirty[dst] = 1;
                ctx->gpr_range_valid[dst] = 0;
                jit_x86_64_invalidate_packed_load(ctx, dst);
                return true;
        }
        if (ctx->gpr_cache_active) {
                if (!jit_x86_64_gpr_publish_remat(ctx) ||
                    !jit_x86_64_gpr_flush_required(ctx))
                        return false;
        }

#endif
        dst_tmp = dst;
        result_type = NOCT_VALUE_INT;
        write_tag = !rt_jit_tmp_has_fixed_primitive_type(ctx, dst_tmp,
                                                     result_type);

        dst *= (int)sizeof(struct rt_value);
        src2 *= (int)sizeof(struct rt_value);
        src1 *= (int)sizeof(struct rt_value);

        ASM {
                /* r15: &env->frame->tmpvar[0] */
                TYPED_LOAD_EAX(src1);
        }
        ASM { IB(0x41); IB(0x0f); IB(0xaf); IB(0x87); ID((uint32_t)(src2 + 8)); }
        ASM {
                TYPED_STORE_EAX(dst, NOCT_VALUE_INT, write_tag);
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
        if (ctx->packed_loop_hint_active)
                jit_x86_64_remove_packed_index_alias(ctx, dst);
        if (ctx->packed_loop_hint_active)
                jit_x86_64_remove_packed_base_alias(ctx, dst);
        if (ctx->gpr_cache_active && ctx->gpr_reg_limit != 1) {
                int r1;
                int r2;
                int rd;
                unsigned pins;

                if (!jit_x86_64_gpr_get(ctx, src1, 0, &r1))
                        return false;
                pins = 1u << (r1 - 8);
                if (!jit_x86_64_gpr_get(ctx, src2, pins, &r2))
                        return false;
                pins |= 1u << (r2 - 8);
                if (!jit_x86_64_gpr_dest(ctx, dst, pins, &rd))
                        return false;
                ASM {
                        /* eax=dividend; edx:eax sign extension; idiv r2d. */
                        IB(0x44); IB(0x89);
                        IB((uint8_t)(0xc0 | ((r1 & 7) << 3)));
                        IB(0x99);
                        IB(0x41); IB(0xf7);
                        IB((uint8_t)(0xf8 | (r2 & 7)));
                }
                ASM {
                        IB(0x41); IB(0x89);
                        IB((uint8_t)(0xc0 | (rd & 7)));
                }
                ctx->gpr_tmp_dirty[dst] = 1;
                ctx->gpr_range_valid[dst] = 0;
                jit_x86_64_invalidate_all_packed_loads(ctx);
                return true;
        }
        if (ctx->gpr_cache_active) {
                if (!jit_x86_64_gpr_publish_remat(ctx) ||
                    !jit_x86_64_gpr_flush_required(ctx))
                        return false;
        }

#endif
        dst_tmp = dst;
        result_type = NOCT_VALUE_INT;
        write_tag = !rt_jit_tmp_has_fixed_primitive_type(ctx, dst_tmp,
                                                     result_type);

        dst *= (int)sizeof(struct rt_value);
        src2 *= (int)sizeof(struct rt_value);
        src1 *= (int)sizeof(struct rt_value);

        /* The LIR layer only emits these with a literal
           divisor outside {0, -1}: no trap is reachable from
           compiled code.  (Crafted bytecode is outside the
           JIT trust model, as with the PLOAD family.) */
        ASM {
                TYPED_LOAD_EAX(src1);
                /* cltd */              IB(0x99);
                /* idivl src2+8(%r15) */ IB(0x41); IB(0xf7); IB(0xbf); ID((uint32_t)(src2 + 8));
        }

        ASM {
                TYPED_STORE_EAX(dst, NOCT_VALUE_INT, write_tag);
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
        if (ctx->packed_loop_hint_active)
                jit_x86_64_remove_packed_index_alias(ctx, dst);
        if (ctx->packed_loop_hint_active)
                jit_x86_64_remove_packed_base_alias(ctx, dst);
        if (ctx->gpr_cache_active && ctx->gpr_reg_limit != 1) {
                int r1;
                int r2;
                int rd;
                unsigned pins;

                if (!jit_x86_64_gpr_get(ctx, src1, 0, &r1))
                        return false;
                pins = 1u << (r1 - 8);
                if (!jit_x86_64_gpr_get(ctx, src2, pins, &r2))
                        return false;
                pins |= 1u << (r2 - 8);
                if (!jit_x86_64_gpr_dest(ctx, dst, pins, &rd))
                        return false;
                ASM {
                        /* eax=dividend; edx:eax sign extension; idiv r2d. */
                        IB(0x44); IB(0x89);
                        IB((uint8_t)(0xc0 | ((r1 & 7) << 3)));
                        IB(0x99);
                        IB(0x41); IB(0xf7);
                        IB((uint8_t)(0xf8 | (r2 & 7)));
                }
                ASM { IB(0x89); IB(0xd0); }
                ASM {
                        IB(0x41); IB(0x89);
                        IB((uint8_t)(0xc0 | (rd & 7)));
                }
                ctx->gpr_tmp_dirty[dst] = 1;
                ctx->gpr_range_valid[dst] = 0;
                jit_x86_64_invalidate_all_packed_loads(ctx);
                return true;
        }
        if (ctx->gpr_cache_active) {
                if (!jit_x86_64_gpr_publish_remat(ctx) ||
                    !jit_x86_64_gpr_flush_required(ctx))
                        return false;
        }

#endif
        dst_tmp = dst;
        result_type = NOCT_VALUE_INT;
        write_tag = !rt_jit_tmp_has_fixed_primitive_type(ctx, dst_tmp,
                                                     result_type);

        dst *= (int)sizeof(struct rt_value);
        src2 *= (int)sizeof(struct rt_value);
        src1 *= (int)sizeof(struct rt_value);

        /* The LIR layer only emits these with a literal
           divisor outside {0, -1}: no trap is reachable from
           compiled code.  (Crafted bytecode is outside the
           JIT trust model, as with the PLOAD family.) */
        ASM {
                TYPED_LOAD_EAX(src1);
                /* cltd */              IB(0x99);
                /* idivl src2+8(%r15) */ IB(0x41); IB(0xf7); IB(0xbf); ID((uint32_t)(src2 + 8));
        }
        /* movl %edx, %eax */
        ASM { IB(0x89); IB(0xd0); }
        ASM {
                TYPED_STORE_EAX(dst, NOCT_VALUE_INT, write_tag);
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
        if (ctx->packed_loop_hint_active)
                jit_x86_64_remove_packed_index_alias(ctx, dst);
        if (ctx->packed_loop_hint_active)
                jit_x86_64_remove_packed_base_alias(ctx, dst);
        if (ctx->gpr_cache_active && ctx->gpr_reg_limit != 1) {
                int r1;
                int r2;
                int rd;
                unsigned pins;
                bool v1;
                bool v2;
                int32_t min1;
                int32_t max1;
                int32_t min2;
                int32_t max2;
                bool imm2;

                v1 = ctx->gpr_range_valid[src1] != 0;
                min1 = v1 ? ctx->gpr_range_min[src1] : 0;
                max1 = v1 ? ctx->gpr_range_max[src1] : 0;
                v2 = ctx->gpr_range_valid[src2] != 0;
                min2 = v2 ? ctx->gpr_range_min[src2] : 0;
                max2 = v2 ? ctx->gpr_range_max[src2] : 0;
                imm2 = ctx->gpr_remat_valid[src2] != 0;
                if (!jit_x86_64_gpr_get(ctx, src1, 0, &r1))
                        return false;
                pins = 1u << (r1 - 8);
                if (imm2) {
                        int32_t imm;

                        imm = ctx->gpr_remat_value[src2];
                        if (dst == src1) {
                                rd = r1;
                        } else if (!jit_x86_64_gpr_is_cached(ctx, src1) &&
                                   rt_jit_ploop_next_use_lpc(ctx, src1,
                                                         ctx->lpc) ==
                                   UINT32_MAX) {
                                if (!jit_x86_64_gpr_rebind(ctx, dst, src1,
                                                         &rd))
                                        return false;
                        } else {
                                if (!jit_x86_64_gpr_dest(ctx, dst, pins,
                                                       &rd) ||
                                    !jit_x86_64_gpr_mov(ctx, rd, r1))
                                        return false;
                        }
                        ASM {
                                IB(0x41); IB(0x81);
                                IB((uint8_t)(0xc0 | (4 << 3) | (rd & 7)));
                                ID((uint32_t)imm);
                        }
                } else {
                        if (!jit_x86_64_gpr_get(ctx, src2, pins, &r2))
                                return false;
                        pins |= 1u << (r2 - 8);
                        if (dst == src1) {
                                rd = r1;
                        } else if (dst == src2) {
                                rd = r2;
                                r2 = r1;
                        } else if (!jit_x86_64_gpr_is_cached(ctx, src1) &&
                                   rt_jit_ploop_next_use_lpc(ctx, src1,
                                                         ctx->lpc) ==
                                   UINT32_MAX) {
                                if (!jit_x86_64_gpr_rebind(ctx, dst, src1,
                                                         &rd))
                                        return false;
                        } else if (!jit_x86_64_gpr_is_cached(ctx, src2) &&
                                   rt_jit_ploop_next_use_lpc(ctx, src2,
                                                         ctx->lpc) ==
                                   UINT32_MAX) {
                                if (!jit_x86_64_gpr_rebind(ctx, dst, src2,
                                                         &rd))
                                        return false;
                                r2 = r1;
                        } else {
                                if (!jit_x86_64_gpr_dest(ctx, dst, pins,
                                                       &rd) ||
                                    !jit_x86_64_gpr_mov(ctx, rd, r1))
                                        return false;
                        }
                        ASM {
                                IB(0x45); IB(0x23);
                                IB((uint8_t)(0xc0 | ((rd & 7) << 3) |
                                             (r2 & 7)));
                        }
                }
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
                jit_x86_64_invalidate_packed_load(ctx, dst);
                return true;
        }
        if (ctx->gpr_cache_active) {
                if (!jit_x86_64_gpr_publish_remat(ctx) ||
                    !jit_x86_64_gpr_flush_required(ctx))
                        return false;
        }

#endif
        dst_tmp = dst;
        result_type = NOCT_VALUE_INT;
        write_tag = !rt_jit_tmp_has_fixed_primitive_type(ctx, dst_tmp,
                                                     result_type);

        dst *= (int)sizeof(struct rt_value);
        src2 *= (int)sizeof(struct rt_value);
        src1 *= (int)sizeof(struct rt_value);

        ASM {
                /* r15: &env->frame->tmpvar[0] */
                TYPED_LOAD_EAX(src1);
        }
        ASM { IB(0x41); IB(0x23); IB(0x87); ID((uint32_t)(src2 + 8)); }
        ASM {
                TYPED_STORE_EAX(dst, NOCT_VALUE_INT, write_tag);
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
        if (ctx->packed_loop_hint_active)
                jit_x86_64_remove_packed_index_alias(ctx, dst);
        if (ctx->packed_loop_hint_active)
                jit_x86_64_remove_packed_base_alias(ctx, dst);
        if (ctx->gpr_cache_active && ctx->gpr_reg_limit != 1) {
                int r1;
                int r2;
                int rd;
                unsigned pins;
                bool imm2;

                imm2 = ctx->gpr_remat_valid[src2] != 0;
                if (!jit_x86_64_gpr_get(ctx, src1, 0, &r1))
                        return false;
                pins = 1u << (r1 - 8);
                if (imm2) {
                        int32_t imm;

                        imm = ctx->gpr_remat_value[src2];
                        if (dst == src1) {
                                rd = r1;
                        } else if (!jit_x86_64_gpr_is_cached(ctx, src1) &&
                                   rt_jit_ploop_next_use_lpc(ctx, src1,
                                                         ctx->lpc) ==
                                   UINT32_MAX) {
                                if (!jit_x86_64_gpr_rebind(ctx, dst, src1,
                                                         &rd))
                                        return false;
                        } else {
                                if (!jit_x86_64_gpr_dest(ctx, dst, pins,
                                                       &rd) ||
                                    !jit_x86_64_gpr_mov(ctx, rd, r1))
                                        return false;
                        }
                        ASM {
                                IB(0x41); IB(0x81);
                                IB((uint8_t)(0xc0 | (1 << 3) | (rd & 7)));
                                ID((uint32_t)imm);
                        }
                } else {
                        if (!jit_x86_64_gpr_get(ctx, src2, pins, &r2))
                                return false;
                        pins |= 1u << (r2 - 8);
                        if (dst == src1) {
                                rd = r1;
                        } else if (dst == src2) {
                                rd = r2;
                                r2 = r1;
                        } else if (!jit_x86_64_gpr_is_cached(ctx, src1) &&
                                   rt_jit_ploop_next_use_lpc(ctx, src1,
                                                         ctx->lpc) ==
                                   UINT32_MAX) {
                                if (!jit_x86_64_gpr_rebind(ctx, dst, src1,
                                                         &rd))
                                        return false;
                        } else if (!jit_x86_64_gpr_is_cached(ctx, src2) &&
                                   rt_jit_ploop_next_use_lpc(ctx, src2,
                                                         ctx->lpc) ==
                                   UINT32_MAX) {
                                if (!jit_x86_64_gpr_rebind(ctx, dst, src2,
                                                         &rd))
                                        return false;
                                r2 = r1;
                        } else {
                                if (!jit_x86_64_gpr_dest(ctx, dst, pins,
                                                       &rd) ||
                                    !jit_x86_64_gpr_mov(ctx, rd, r1))
                                        return false;
                        }
                        ASM {
                                IB(0x45); IB(0x0b);
                                IB((uint8_t)(0xc0 | ((rd & 7) << 3) |
                                             (r2 & 7)));
                        }
                }
                ctx->gpr_tmp_dirty[dst] = 1;
                ctx->gpr_range_valid[dst] = 0;
                jit_x86_64_invalidate_packed_load(ctx, dst);
                return true;
        }
        if (ctx->gpr_cache_active) {
                if (!jit_x86_64_gpr_publish_remat(ctx) ||
                    !jit_x86_64_gpr_flush_required(ctx))
                        return false;
        }

#endif
        dst_tmp = dst;
        result_type = NOCT_VALUE_INT;
        write_tag = !rt_jit_tmp_has_fixed_primitive_type(ctx, dst_tmp,
                                                     result_type);

        dst *= (int)sizeof(struct rt_value);
        src2 *= (int)sizeof(struct rt_value);
        src1 *= (int)sizeof(struct rt_value);

        ASM {
                /* r15: &env->frame->tmpvar[0] */
                TYPED_LOAD_EAX(src1);
        }
        ASM { IB(0x41); IB(0x0b); IB(0x87); ID((uint32_t)(src2 + 8)); }
        ASM {
                TYPED_STORE_EAX(dst, NOCT_VALUE_INT, write_tag);
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
        if (ctx->packed_loop_hint_active)
                jit_x86_64_remove_packed_index_alias(ctx, dst);
        if (ctx->packed_loop_hint_active)
                jit_x86_64_remove_packed_base_alias(ctx, dst);
        if (ctx->gpr_cache_active && ctx->gpr_reg_limit != 1) {
                int r1;
                int r2;
                int rd;
                unsigned pins;
                bool imm2;

                imm2 = ctx->gpr_remat_valid[src2] != 0;
                if (!jit_x86_64_gpr_get(ctx, src1, 0, &r1))
                        return false;
                pins = 1u << (r1 - 8);
                if (imm2) {
                        int32_t imm;

                        imm = ctx->gpr_remat_value[src2];
                        if (dst == src1) {
                                rd = r1;
                        } else if (!jit_x86_64_gpr_is_cached(ctx, src1) &&
                                   rt_jit_ploop_next_use_lpc(ctx, src1,
                                                         ctx->lpc) ==
                                   UINT32_MAX) {
                                if (!jit_x86_64_gpr_rebind(ctx, dst, src1,
                                                         &rd))
                                        return false;
                        } else {
                                if (!jit_x86_64_gpr_dest(ctx, dst, pins,
                                                       &rd) ||
                                    !jit_x86_64_gpr_mov(ctx, rd, r1))
                                        return false;
                        }
                        ASM {
                                IB(0x41); IB(0x81);
                                IB((uint8_t)(0xc0 | (6 << 3) | (rd & 7)));
                                ID((uint32_t)imm);
                        }
                } else {
                        if (!jit_x86_64_gpr_get(ctx, src2, pins, &r2))
                                return false;
                        pins |= 1u << (r2 - 8);
                        if (dst == src1) {
                                rd = r1;
                        } else if (dst == src2) {
                                rd = r2;
                                r2 = r1;
                        } else if (!jit_x86_64_gpr_is_cached(ctx, src1) &&
                                   rt_jit_ploop_next_use_lpc(ctx, src1,
                                                         ctx->lpc) ==
                                   UINT32_MAX) {
                                if (!jit_x86_64_gpr_rebind(ctx, dst, src1,
                                                         &rd))
                                        return false;
                        } else if (!jit_x86_64_gpr_is_cached(ctx, src2) &&
                                   rt_jit_ploop_next_use_lpc(ctx, src2,
                                                         ctx->lpc) ==
                                   UINT32_MAX) {
                                if (!jit_x86_64_gpr_rebind(ctx, dst, src2,
                                                         &rd))
                                        return false;
                                r2 = r1;
                        } else {
                                if (!jit_x86_64_gpr_dest(ctx, dst, pins,
                                                       &rd) ||
                                    !jit_x86_64_gpr_mov(ctx, rd, r1))
                                        return false;
                        }
                        ASM {
                                IB(0x45); IB(0x33);
                                IB((uint8_t)(0xc0 | ((rd & 7) << 3) |
                                             (r2 & 7)));
                        }
                }
                ctx->gpr_tmp_dirty[dst] = 1;
                ctx->gpr_range_valid[dst] = 0;
                jit_x86_64_invalidate_packed_load(ctx, dst);
                return true;
        }
        if (ctx->gpr_cache_active) {
                if (!jit_x86_64_gpr_publish_remat(ctx) ||
                    !jit_x86_64_gpr_flush_required(ctx))
                        return false;
        }

#endif
        dst_tmp = dst;
        result_type = NOCT_VALUE_INT;
        write_tag = !rt_jit_tmp_has_fixed_primitive_type(ctx, dst_tmp,
                                                     result_type);

        dst *= (int)sizeof(struct rt_value);
        src2 *= (int)sizeof(struct rt_value);
        src1 *= (int)sizeof(struct rt_value);

        ASM {
                /* r15: &env->frame->tmpvar[0] */
                TYPED_LOAD_EAX(src1);
        }
        ASM { IB(0x41); IB(0x33); IB(0x87); ID((uint32_t)(src2 + 8)); }
        ASM {
                TYPED_STORE_EAX(dst, NOCT_VALUE_INT, write_tag);
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
        if (ctx->packed_loop_hint_active)
                jit_x86_64_remove_packed_index_alias(ctx, dst);
        if (ctx->packed_loop_hint_active)
                jit_x86_64_remove_packed_base_alias(ctx, dst);
        if (ctx->gpr_cache_active && ctx->gpr_reg_limit != 1) {
                int r1;
                int rd;
                unsigned pins;

                if (!jit_x86_64_gpr_get(ctx, src1, 0, &r1))
                        return false;
                pins = 1u << (r1 - 8);
                if (dst == src1) {
                        rd = r1;
                } else if (!jit_x86_64_gpr_is_cached(ctx, src1) &&
                           rt_jit_ploop_next_use_lpc(ctx, src1, ctx->lpc) ==
                           UINT32_MAX) {
                        if (!jit_x86_64_gpr_rebind(ctx, dst, src1, &rd))
                                return false;
                } else {
                        if (!jit_x86_64_gpr_dest(ctx, dst, pins, &rd) ||
                            !jit_x86_64_gpr_mov(ctx, rd, r1))
                                return false;
                }
                if ((src2 & 31) != 0) {
                        ASM {
                                IB(0x41); IB(0xc1);
                                IB((uint8_t)(0xc0 | 0x20 | (rd & 7)));
                                IB((uint8_t)(src2 & 31));
                        }
                }
                ctx->gpr_tmp_dirty[dst] = 1;
                ctx->gpr_range_valid[dst] = 0;
                jit_x86_64_invalidate_packed_load(ctx, dst);
                return true;
        }
        if (ctx->gpr_cache_active) {
                if (!jit_x86_64_gpr_publish_remat(ctx) ||
                    !jit_x86_64_gpr_flush_required(ctx))
                        return false;
        }

#endif
        dst_tmp = dst;
        result_type = NOCT_VALUE_INT;
        write_tag = !rt_jit_tmp_has_fixed_primitive_type(ctx, dst_tmp,
                                                     result_type);

        dst *= (int)sizeof(struct rt_value);

        src1 *= (int)sizeof(struct rt_value);

        /* src2 is the shift-count immediate (0..31). */
        ASM {
                TYPED_LOAD_EAX(src1);
        }
        if ((src2 & 31) != 0) {
                /* shll $imm, %eax */
                ASM { IB(0xc1); IB(0xe0); IB((uint8_t)(src2 & 31)); }
        }
        ASM {
                TYPED_STORE_EAX(dst, NOCT_VALUE_INT, write_tag);
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
        if (ctx->packed_loop_hint_active)
                jit_x86_64_remove_packed_index_alias(ctx, dst);
        if (ctx->packed_loop_hint_active)
                jit_x86_64_remove_packed_base_alias(ctx, dst);
        if (ctx->gpr_cache_active && ctx->gpr_reg_limit != 1) {
                int r1;
                int rd;
                unsigned pins;

                if (!jit_x86_64_gpr_get(ctx, src1, 0, &r1))
                        return false;
                pins = 1u << (r1 - 8);
                if (dst == src1) {
                        rd = r1;
                } else if (!jit_x86_64_gpr_is_cached(ctx, src1) &&
                           rt_jit_ploop_next_use_lpc(ctx, src1, ctx->lpc) ==
                           UINT32_MAX) {
                        if (!jit_x86_64_gpr_rebind(ctx, dst, src1, &rd))
                                return false;
                } else {
                        if (!jit_x86_64_gpr_dest(ctx, dst, pins, &rd) ||
                            !jit_x86_64_gpr_mov(ctx, rd, r1))
                                return false;
                }
                if ((src2 & 31) != 0) {
                        ASM {
                                IB(0x41); IB(0xc1);
                                IB((uint8_t)(0xc0 | 0x28 | (rd & 7)));
                                IB((uint8_t)(src2 & 31));
                        }
                }
                ctx->gpr_tmp_dirty[dst] = 1;
                ctx->gpr_range_valid[dst] = 0;
                jit_x86_64_invalidate_packed_load(ctx, dst);
                return true;
        }
        if (ctx->gpr_cache_active) {
                if (!jit_x86_64_gpr_publish_remat(ctx) ||
                    !jit_x86_64_gpr_flush_required(ctx))
                        return false;
        }

#endif
        dst_tmp = dst;
        result_type = NOCT_VALUE_INT;
        write_tag = !rt_jit_tmp_has_fixed_primitive_type(ctx, dst_tmp,
                                                     result_type);

        dst *= (int)sizeof(struct rt_value);

        src1 *= (int)sizeof(struct rt_value);

        /* src2 is the shift-count immediate (0..31). */
        ASM {
                TYPED_LOAD_EAX(src1);
        }
        if ((src2 & 31) != 0) {
                /* shrl $imm, %eax (LOGICAL) */
                ASM { IB(0xc1); IB(0xe8); IB((uint8_t)(src2 & 31)); }
        }
        ASM {
                TYPED_STORE_EAX(dst, NOCT_VALUE_INT, write_tag);
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
        if (ctx->packed_loop_hint_active)
                jit_x86_64_remove_packed_index_alias(ctx, dst);
        if (ctx->packed_loop_hint_active)
                jit_x86_64_remove_packed_base_alias(ctx, dst);
        if (ctx->gpr_cache_active) {
                if (!jit_x86_64_gpr_publish_remat(ctx) ||
                    !jit_x86_64_gpr_flush_required(ctx))
                        return false;
        }

#endif
        dst_tmp = dst;
        result_type = NOCT_VALUE_INT;
        write_tag = !rt_jit_tmp_has_fixed_primitive_type(ctx, dst_tmp,
                                                     result_type);

        dst *= (int)sizeof(struct rt_value);
        src2 *= (int)sizeof(struct rt_value);
        src1 *= (int)sizeof(struct rt_value);

        ASM {
                TYPED_LOAD_EAX(src1);
                /* cmpl src2+8(%r15), %eax */
                IB(0x41); IB(0x3b); IB(0x87); ID((uint32_t)(src2 + 8));
        }
        ASM { TYPED_SETCC_EAX(0x9c); }
        ASM {
                TYPED_STORE_EAX(dst, NOCT_VALUE_INT, write_tag);
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
        if (ctx->packed_loop_hint_active)
                jit_x86_64_remove_packed_index_alias(ctx, dst);
        if (ctx->packed_loop_hint_active)
                jit_x86_64_remove_packed_base_alias(ctx, dst);
        if (ctx->gpr_cache_active) {
                if (!jit_x86_64_gpr_publish_remat(ctx) ||
                    !jit_x86_64_gpr_flush_required(ctx))
                        return false;
        }

#endif
        dst_tmp = dst;
        result_type = NOCT_VALUE_INT;
        write_tag = !rt_jit_tmp_has_fixed_primitive_type(ctx, dst_tmp,
                                                     result_type);

        dst *= (int)sizeof(struct rt_value);
        src2 *= (int)sizeof(struct rt_value);
        src1 *= (int)sizeof(struct rt_value);

        ASM {
                TYPED_LOAD_EAX(src1);
                /* cmpl src2+8(%r15), %eax */
                IB(0x41); IB(0x3b); IB(0x87); ID((uint32_t)(src2 + 8));
        }
        ASM { TYPED_SETCC_EAX(0x9e); }
        ASM {
                TYPED_STORE_EAX(dst, NOCT_VALUE_INT, write_tag);
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
        if (ctx->packed_loop_hint_active)
                jit_x86_64_remove_packed_index_alias(ctx, dst);
        if (ctx->packed_loop_hint_active)
                jit_x86_64_remove_packed_base_alias(ctx, dst);
        if (ctx->gpr_cache_active) {
                if (!jit_x86_64_gpr_publish_remat(ctx) ||
                    !jit_x86_64_gpr_flush_required(ctx))
                        return false;
        }

#endif
        dst_tmp = dst;
        result_type = NOCT_VALUE_INT;
        write_tag = !rt_jit_tmp_has_fixed_primitive_type(ctx, dst_tmp,
                                                     result_type);

        dst *= (int)sizeof(struct rt_value);
        src2 *= (int)sizeof(struct rt_value);
        src1 *= (int)sizeof(struct rt_value);

        ASM {
                TYPED_LOAD_EAX(src1);
                /* cmpl src2+8(%r15), %eax */
                IB(0x41); IB(0x3b); IB(0x87); ID((uint32_t)(src2 + 8));
        }
        ASM { TYPED_SETCC_EAX(0x9f); }
        ASM {
                TYPED_STORE_EAX(dst, NOCT_VALUE_INT, write_tag);
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
        if (ctx->packed_loop_hint_active)
                jit_x86_64_remove_packed_index_alias(ctx, dst);
        if (ctx->packed_loop_hint_active)
                jit_x86_64_remove_packed_base_alias(ctx, dst);
        if (ctx->gpr_cache_active) {
                if (!jit_x86_64_gpr_publish_remat(ctx) ||
                    !jit_x86_64_gpr_flush_required(ctx))
                        return false;
        }

#endif
        dst_tmp = dst;
        result_type = NOCT_VALUE_INT;
        write_tag = !rt_jit_tmp_has_fixed_primitive_type(ctx, dst_tmp,
                                                     result_type);

        dst *= (int)sizeof(struct rt_value);
        src2 *= (int)sizeof(struct rt_value);
        src1 *= (int)sizeof(struct rt_value);

        ASM {
                TYPED_LOAD_EAX(src1);
                /* cmpl src2+8(%r15), %eax */
                IB(0x41); IB(0x3b); IB(0x87); ID((uint32_t)(src2 + 8));
        }
        ASM { TYPED_SETCC_EAX(0x9d); }
        ASM {
                TYPED_STORE_EAX(dst, NOCT_VALUE_INT, write_tag);
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
        if (ctx->packed_loop_hint_active)
                jit_x86_64_remove_packed_index_alias(ctx, dst);
        if (ctx->packed_loop_hint_active)
                jit_x86_64_remove_packed_base_alias(ctx, dst);
        if (ctx->gpr_cache_active) {
                if (!jit_x86_64_gpr_publish_remat(ctx) ||
                    !jit_x86_64_gpr_flush_required(ctx))
                        return false;
        }

#endif
        dst_tmp = dst;
        result_type = NOCT_VALUE_FLOAT;
        write_tag = !rt_jit_tmp_has_fixed_primitive_type(ctx, dst_tmp,
                                                     result_type);

        dst *= (int)sizeof(struct rt_value);
        src2 *= (int)sizeof(struct rt_value);
        src1 *= (int)sizeof(struct rt_value);

        ASM {
                TYPED_LOAD_XMM0(src1);
        }
        ASM { IB(0xf3); IB(0x41); IB(0x0f); IB(0x58); IB(0x87); ID((uint32_t)(src2 + 8)); }
        ASM {
                /* movl $tag, dst(%r15), unless fixed and known */
                if (write_tag) {
                IB(0x41); IB(0xc7); IB(0x87); ID((uint32_t)dst); ID((uint32_t)NOCT_VALUE_FLOAT); }
                /* movss %xmm0, dst+8(%r15) */
                IB(0xf3); IB(0x41); IB(0x0f); IB(0x11); IB(0x87); ID((uint32_t)(dst + 8));
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
        if (ctx->packed_loop_hint_active)
                jit_x86_64_remove_packed_index_alias(ctx, dst);
        if (ctx->packed_loop_hint_active)
                jit_x86_64_remove_packed_base_alias(ctx, dst);
        if (ctx->gpr_cache_active) {
                if (!jit_x86_64_gpr_publish_remat(ctx) ||
                    !jit_x86_64_gpr_flush_required(ctx))
                        return false;
        }

#endif
        dst_tmp = dst;
        result_type = NOCT_VALUE_FLOAT;
        write_tag = !rt_jit_tmp_has_fixed_primitive_type(ctx, dst_tmp,
                                                     result_type);

        dst *= (int)sizeof(struct rt_value);
        src2 *= (int)sizeof(struct rt_value);
        src1 *= (int)sizeof(struct rt_value);

        ASM {
                TYPED_LOAD_XMM0(src1);
        }
        ASM { IB(0xf3); IB(0x41); IB(0x0f); IB(0x5c); IB(0x87); ID((uint32_t)(src2 + 8)); }
        ASM {
                /* movl $tag, dst(%r15), unless fixed and known */
                if (write_tag) {
                IB(0x41); IB(0xc7); IB(0x87); ID((uint32_t)dst); ID((uint32_t)NOCT_VALUE_FLOAT); }
                /* movss %xmm0, dst+8(%r15) */
                IB(0xf3); IB(0x41); IB(0x0f); IB(0x11); IB(0x87); ID((uint32_t)(dst + 8));
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
        if (ctx->packed_loop_hint_active)
                jit_x86_64_remove_packed_index_alias(ctx, dst);
        if (ctx->packed_loop_hint_active)
                jit_x86_64_remove_packed_base_alias(ctx, dst);
        if (ctx->gpr_cache_active) {
                if (!jit_x86_64_gpr_publish_remat(ctx) ||
                    !jit_x86_64_gpr_flush_required(ctx))
                        return false;
        }

#endif
        dst_tmp = dst;
        result_type = NOCT_VALUE_FLOAT;
        write_tag = !rt_jit_tmp_has_fixed_primitive_type(ctx, dst_tmp,
                                                     result_type);

        dst *= (int)sizeof(struct rt_value);
        src2 *= (int)sizeof(struct rt_value);
        src1 *= (int)sizeof(struct rt_value);

        ASM {
                TYPED_LOAD_XMM0(src1);
        }
        ASM { IB(0xf3); IB(0x41); IB(0x0f); IB(0x59); IB(0x87); ID((uint32_t)(src2 + 8)); }
        ASM {
                /* movl $tag, dst(%r15), unless fixed and known */
                if (write_tag) {
                IB(0x41); IB(0xc7); IB(0x87); ID((uint32_t)dst); ID((uint32_t)NOCT_VALUE_FLOAT); }
                /* movss %xmm0, dst+8(%r15) */
                IB(0xf3); IB(0x41); IB(0x0f); IB(0x11); IB(0x87); ID((uint32_t)(dst + 8));
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
        if (ctx->packed_loop_hint_active)
                jit_x86_64_remove_packed_index_alias(ctx, dst);
        if (ctx->packed_loop_hint_active)
                jit_x86_64_remove_packed_base_alias(ctx, dst);
        if (ctx->gpr_cache_active) {
                if (!jit_x86_64_gpr_publish_remat(ctx) ||
                    !jit_x86_64_gpr_flush_required(ctx))
                        return false;
        }

#endif
        dst_tmp = dst;
        result_type = NOCT_VALUE_FLOAT;
        write_tag = !rt_jit_tmp_has_fixed_primitive_type(ctx, dst_tmp,
                                                     result_type);

        dst *= (int)sizeof(struct rt_value);
        src2 *= (int)sizeof(struct rt_value);
        src1 *= (int)sizeof(struct rt_value);

        ASM {
                TYPED_LOAD_XMM0(src1);
        }
        ASM { IB(0xf3); IB(0x41); IB(0x0f); IB(0x5e); IB(0x87); ID((uint32_t)(src2 + 8)); }
        ASM {
                /* movl $tag, dst(%r15), unless fixed and known */
                if (write_tag) {
                IB(0x41); IB(0xc7); IB(0x87); ID((uint32_t)dst); ID((uint32_t)NOCT_VALUE_FLOAT); }
                /* movss %xmm0, dst+8(%r15) */
                IB(0xf3); IB(0x41); IB(0x0f); IB(0x11); IB(0x87); ID((uint32_t)(dst + 8));
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
        if (ctx->packed_loop_hint_active)
                jit_x86_64_remove_packed_index_alias(ctx, dst);
        if (ctx->packed_loop_hint_active)
                jit_x86_64_remove_packed_base_alias(ctx, dst);
        if (ctx->gpr_cache_active) {
                if (!jit_x86_64_gpr_publish_remat(ctx) ||
                    !jit_x86_64_gpr_flush_required(ctx))
                        return false;
        }

#endif
        dst_tmp = dst;
        result_type = NOCT_VALUE_INT;
        write_tag = !rt_jit_tmp_has_fixed_primitive_type(ctx, dst_tmp,
                                                     result_type);

        dst *= (int)sizeof(struct rt_value);
        src2 *= (int)sizeof(struct rt_value);
        src1 *= (int)sizeof(struct rt_value);

        /* a < b  ==  b > a: load b, compare against a. */
        ASM {
                TYPED_LOAD_XMM0(src2);
                /* ucomiss src1+8(%r15), %xmm0 */
                IB(0x41); IB(0x0f); IB(0x2e); IB(0x87); ID((uint32_t)(src1 + 8));
        }
        ASM { TYPED_SETCC_EAX(0x97); }  /* seta  */
        ASM {
                TYPED_STORE_EAX(dst, NOCT_VALUE_INT, write_tag);
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
        if (ctx->packed_loop_hint_active)
                jit_x86_64_remove_packed_index_alias(ctx, dst);
        if (ctx->packed_loop_hint_active)
                jit_x86_64_remove_packed_base_alias(ctx, dst);
        if (ctx->gpr_cache_active) {
                if (!jit_x86_64_gpr_publish_remat(ctx) ||
                    !jit_x86_64_gpr_flush_required(ctx))
                        return false;
        }

#endif
        dst_tmp = dst;
        result_type = NOCT_VALUE_INT;
        write_tag = !rt_jit_tmp_has_fixed_primitive_type(ctx, dst_tmp,
                                                     result_type);

        dst *= (int)sizeof(struct rt_value);
        src2 *= (int)sizeof(struct rt_value);
        src1 *= (int)sizeof(struct rt_value);

        /* a < b  ==  b > a: load b, compare against a. */
        ASM {
                TYPED_LOAD_XMM0(src2);
                /* ucomiss src1+8(%r15), %xmm0 */
                IB(0x41); IB(0x0f); IB(0x2e); IB(0x87); ID((uint32_t)(src1 + 8));
        }
        ASM { TYPED_SETCC_EAX(0x93); }  /* setae */
        ASM {
                TYPED_STORE_EAX(dst, NOCT_VALUE_INT, write_tag);
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
        if (ctx->packed_loop_hint_active)
                jit_x86_64_remove_packed_index_alias(ctx, dst);
        if (ctx->packed_loop_hint_active)
                jit_x86_64_remove_packed_base_alias(ctx, dst);
        if (ctx->gpr_cache_active) {
                if (!jit_x86_64_gpr_publish_remat(ctx) ||
                    !jit_x86_64_gpr_flush_required(ctx))
                        return false;
        }

#endif
        dst_tmp = dst;
        result_type = NOCT_VALUE_INT;
        write_tag = !rt_jit_tmp_has_fixed_primitive_type(ctx, dst_tmp,
                                                     result_type);

        dst *= (int)sizeof(struct rt_value);
        src2 *= (int)sizeof(struct rt_value);
        src1 *= (int)sizeof(struct rt_value);

        ASM {
                TYPED_LOAD_XMM0(src1);
                /* ucomiss src2+8(%r15), %xmm0 */
                IB(0x41); IB(0x0f); IB(0x2e); IB(0x87); ID((uint32_t)(src2 + 8));
        }
        ASM { TYPED_SETCC_EAX(0x97); }  /* seta  */
        ASM {
                TYPED_STORE_EAX(dst, NOCT_VALUE_INT, write_tag);
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
        if (ctx->packed_loop_hint_active)
                jit_x86_64_remove_packed_index_alias(ctx, dst);
        if (ctx->packed_loop_hint_active)
                jit_x86_64_remove_packed_base_alias(ctx, dst);
        if (ctx->gpr_cache_active) {
                if (!jit_x86_64_gpr_publish_remat(ctx) ||
                    !jit_x86_64_gpr_flush_required(ctx))
                        return false;
        }

#endif
        dst_tmp = dst;
        result_type = NOCT_VALUE_INT;
        write_tag = !rt_jit_tmp_has_fixed_primitive_type(ctx, dst_tmp,
                                                     result_type);

        dst *= (int)sizeof(struct rt_value);
        src2 *= (int)sizeof(struct rt_value);
        src1 *= (int)sizeof(struct rt_value);

        ASM {
                TYPED_LOAD_XMM0(src1);
                /* ucomiss src2+8(%r15), %xmm0 */
                IB(0x41); IB(0x0f); IB(0x2e); IB(0x87); ID((uint32_t)(src2 + 8));
        }
        ASM { TYPED_SETCC_EAX(0x93); }  /* setae */
        ASM {
                TYPED_STORE_EAX(dst, NOCT_VALUE_INT, write_tag);
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
        uint8_t *zero_patch;
        uint8_t *divide_patch[2];
        uint8_t *store_patch;
        uint8_t *done_patch;
        uint8_t *divide_target;
        uint8_t *store_target;
        uint8_t *cold_target;
        uint8_t *done_target;

        CONSUME_TMPVAR(dst);
        CONSUME_TMPVAR(src1);
        CONSUME_TMPVAR(src2);

#if defined(NOCT_USE_OPTIMIZER)
        if (rt_jit_ploop_current_elided(ctx, 7))
                return true;
        if (ctx->packed_loop_hint_active)
                jit_x86_64_remove_packed_index_alias(ctx, dst);
        if (ctx->packed_loop_hint_active)
                jit_x86_64_remove_packed_base_alias(ctx, dst);
        if (ctx->gpr_cache_active && ctx->gpr_reg_limit != 1) {
                int r1;
                int r2;
                int rd;
                unsigned pins;
                bool v2;
                int32_t min2;
                int32_t max2;
                bool proven;

                v2 = ctx->gpr_range_valid[src2] != 0;
                min2 = v2 ? ctx->gpr_range_min[src2] : 0;
                max2 = v2 ? ctx->gpr_range_max[src2] : 0;
                proven = v2 && (min2 > 0 || max2 < -1);
                if (proven) {
                        if (!jit_x86_64_gpr_get(ctx, src1, 0, &r1))
                                return false;
                        pins = 1u << (r1 - 8);
                        if (!jit_x86_64_gpr_get(ctx, src2, pins, &r2))
                                return false;
                        pins |= 1u << (r2 - 8);
                        if (!jit_x86_64_gpr_dest(ctx, dst, pins, &rd))
                                return false;
                        ASM {
                                /* eax=dividend; edx:eax sign extension; idiv r2d. */
                                IB(0x44); IB(0x89);
                                IB((uint8_t)(0xc0 | ((r1 & 7) << 3)));
                                IB(0x99);
                                IB(0x41); IB(0xf7);
                                IB((uint8_t)(0xf8 | (r2 & 7)));
                        }
                        ASM {
                                IB(0x41); IB(0x89);
                                IB((uint8_t)(0xc0 | (rd & 7)));
                        }
                        ctx->gpr_tmp_dirty[dst] = 1;
                        ctx->gpr_range_valid[dst] = 0;
                        ctx->gpr_proven_divisions++;
                        jit_x86_64_invalidate_all_packed_loads(ctx);
                        return true;
                }
        }
        if (ctx->gpr_cache_active) {
                if (!jit_x86_64_gpr_publish_remat(ctx) ||
                    !jit_x86_64_gpr_flush_required(ctx))
                        return false;
        }
#endif
        dst_ofs = dst * (int)sizeof(struct rt_value);
        src1_ofs = src1 * (int)sizeof(struct rt_value);
        src2_ofs = src2 * (int)sizeof(struct rt_value);
        ASM {
                /* eax = dividend, ecx = divisor. */
                IB(0x41); IB(0x8b); IB(0x87); ID((uint32_t)(src1_ofs + 8));
                IB(0x41); IB(0x8b); IB(0x8f); ID((uint32_t)(src2_ofs + 8));
                IB(0x85); IB(0xc9);             /* test ecx,ecx */
                IB(0x0f); IB(0x84);             /* je cold */
        }
        zero_patch = (uint8_t *)ctx->code;
        if (!jit_put_dword(ctx, 0))
                return false;
        ASM {
                IB(0x83); IB(0xf9); IB(0xff);   /* cmp ecx,-1 */
                IB(0x0f); IB(0x85);             /* jne divide */
        }
        divide_patch[0] = (uint8_t *)ctx->code;
        if (!jit_put_dword(ctx, 0))
                return false;
        ASM {
                IB(0x3d); ID(0x80000000u);      /* cmp eax,INT_MIN */
                IB(0x0f); IB(0x85);             /* jne divide */
        }
        divide_patch[1] = (uint8_t *)ctx->code;
        if (!jit_put_dword(ctx, 0))
                return false;

        ASM { IB(0xe9); }
        store_patch = (uint8_t *)ctx->code;
        if (!jit_put_dword(ctx, 0))
                return false;

        divide_target = (uint8_t *)ctx->code;
        jit_x86_64_patch_local_rel32(divide_patch[0], divide_target);
        jit_x86_64_patch_local_rel32(divide_patch[1], divide_target);
        ASM {
                IB(0x99);                       /* cdq */
                IB(0xf7); IB(0xf9);             /* idiv ecx */
        }

        store_target = (uint8_t *)ctx->code;
        jit_x86_64_patch_local_rel32(store_patch, store_target);
        ASM {
                IB(0x41); IB(0xc7); IB(0x87); ID((uint32_t)dst_ofs);
                ID((uint32_t)NOCT_VALUE_INT);
                IB(0x41); IB(0x89); IB(0x87); ID((uint32_t)(dst_ofs + 8));
                IB(0xe9);
        }
        done_patch = (uint8_t *)ctx->code;
        if (!jit_put_dword(ctx, 0))
                return false;

        cold_target = (uint8_t *)ctx->code;
        jit_x86_64_patch_local_rel32(zero_patch, cold_target);
        ASM_BINARY_OP(ex_idiv_helper);
        done_target = (uint8_t *)ctx->code;
        jit_x86_64_patch_local_rel32(done_patch, done_target);
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
        uint8_t *zero_patch;
        uint8_t *divide_patch[2];
        uint8_t *store_patch;
        uint8_t *done_patch;
        uint8_t *divide_target;
        uint8_t *store_target;
        uint8_t *cold_target;
        uint8_t *done_target;

        CONSUME_TMPVAR(dst);
        CONSUME_TMPVAR(src1);
        CONSUME_TMPVAR(src2);

#if defined(NOCT_USE_OPTIMIZER)
        if (rt_jit_ploop_current_elided(ctx, 7))
                return true;
        if (ctx->packed_loop_hint_active)
                jit_x86_64_remove_packed_index_alias(ctx, dst);
        if (ctx->packed_loop_hint_active)
                jit_x86_64_remove_packed_base_alias(ctx, dst);
        if (ctx->gpr_cache_active && ctx->gpr_reg_limit != 1) {
                int r1;
                int r2;
                int rd;
                unsigned pins;
                bool v2;
                int32_t min2;
                int32_t max2;
                bool proven;

                v2 = ctx->gpr_range_valid[src2] != 0;
                min2 = v2 ? ctx->gpr_range_min[src2] : 0;
                max2 = v2 ? ctx->gpr_range_max[src2] : 0;
                proven = v2 && (min2 > 0 || max2 < -1);
                if (proven) {
                        if (!jit_x86_64_gpr_get(ctx, src1, 0, &r1))
                                return false;
                        pins = 1u << (r1 - 8);
                        if (!jit_x86_64_gpr_get(ctx, src2, pins, &r2))
                                return false;
                        pins |= 1u << (r2 - 8);
                        if (!jit_x86_64_gpr_dest(ctx, dst, pins, &rd))
                                return false;
                        ASM {
                                /* eax=dividend; edx:eax sign extension; idiv r2d. */
                                IB(0x44); IB(0x89);
                                IB((uint8_t)(0xc0 | ((r1 & 7) << 3)));
                                IB(0x99);
                                IB(0x41); IB(0xf7);
                                IB((uint8_t)(0xf8 | (r2 & 7)));
                        }
                        ASM { IB(0x89); IB(0xd0); }
                        ASM {
                                IB(0x41); IB(0x89);
                                IB((uint8_t)(0xc0 | (rd & 7)));
                        }
                        ctx->gpr_tmp_dirty[dst] = 1;
                        ctx->gpr_range_valid[dst] = 0;
                        ctx->gpr_proven_divisions++;
                        jit_x86_64_invalidate_all_packed_loads(ctx);
                        return true;
                }
        }
        if (ctx->gpr_cache_active) {
                if (!jit_x86_64_gpr_publish_remat(ctx) ||
                    !jit_x86_64_gpr_flush_required(ctx))
                        return false;
        }
#endif
        dst_ofs = dst * (int)sizeof(struct rt_value);
        src1_ofs = src1 * (int)sizeof(struct rt_value);
        src2_ofs = src2 * (int)sizeof(struct rt_value);
        ASM {
                /* eax = dividend, ecx = divisor. */
                IB(0x41); IB(0x8b); IB(0x87); ID((uint32_t)(src1_ofs + 8));
                IB(0x41); IB(0x8b); IB(0x8f); ID((uint32_t)(src2_ofs + 8));
                IB(0x85); IB(0xc9);             /* test ecx,ecx */
                IB(0x0f); IB(0x84);             /* je cold */
        }
        zero_patch = (uint8_t *)ctx->code;
        if (!jit_put_dword(ctx, 0))
                return false;
        ASM {
                IB(0x83); IB(0xf9); IB(0xff);   /* cmp ecx,-1 */
                IB(0x0f); IB(0x85);             /* jne divide */
        }
        divide_patch[0] = (uint8_t *)ctx->code;
        if (!jit_put_dword(ctx, 0))
                return false;
        ASM {
                IB(0x3d); ID(0x80000000u);      /* cmp eax,INT_MIN */
                IB(0x0f); IB(0x85);             /* jne divide */
        }
        divide_patch[1] = (uint8_t *)ctx->code;
        if (!jit_put_dword(ctx, 0))
                return false;
        /* INT_MIN % -1 = 0; quotient leaves eax as INT_MIN. */
        ASM { IB(0x31); IB(0xc0); }
        ASM { IB(0xe9); }
        store_patch = (uint8_t *)ctx->code;
        if (!jit_put_dword(ctx, 0))
                return false;

        divide_target = (uint8_t *)ctx->code;
        jit_x86_64_patch_local_rel32(divide_patch[0], divide_target);
        jit_x86_64_patch_local_rel32(divide_patch[1], divide_target);
        ASM {
                IB(0x99);                       /* cdq */
                IB(0xf7); IB(0xf9);             /* idiv ecx */
        }
        ASM { IB(0x89); IB(0xd0); }     /* mov edx,eax */

        store_target = (uint8_t *)ctx->code;
        jit_x86_64_patch_local_rel32(store_patch, store_target);
        ASM {
                IB(0x41); IB(0xc7); IB(0x87); ID((uint32_t)dst_ofs);
                ID((uint32_t)NOCT_VALUE_INT);
                IB(0x41); IB(0x89); IB(0x87); ID((uint32_t)(dst_ofs + 8));
                IB(0xe9);
        }
        done_patch = (uint8_t *)ctx->code;
        if (!jit_put_dword(ctx, 0))
                return false;

        cold_target = (uint8_t *)ctx->code;
        jit_x86_64_patch_local_rel32(zero_patch, cold_target);
        ASM_BINARY_OP(ex_imod_helper);
        done_target = (uint8_t *)ctx->code;
        jit_x86_64_patch_local_rel32(done_patch, done_target);
        return true;
}


/*
 * 128-bit SIMD ops (docs/design/06-simd.md).
 *
 * SysV x86_64 with SSE2/SSE4.1 (runtime CPUID gate): inline vector code,
 * vreg k -> xmm k (xmm0..xmm12 are available to call-free vector
 * regions; xmm13..xmm15 are backend scratch/invariant registers). Win64 uses
 * direct scalar lowering over env->vreg so it never owns nonvolatile
 * xmm6/xmm7; scalar FP uses only volatile xmm0.
 */

/* Per-build CPUID: no unsynchronised process-global feature cache. */
static uint32_t
jit_detect_simd_caps(void)
{
#if defined(__GNUC__)
        uint32_t a, b, c, d;
	uint32_t xcr0_lo, xcr0_hi;
	uint32_t caps;

	UNUSED_PARAMETER(xcr0_hi);

	caps = 0;
        __asm__ __volatile__("cpuid"
                             : "=a"(a), "=b"(b), "=c"(c), "=d"(d)
                             : "a"(1), "c"(0));
	if ((d & (1u << 26)) != 0)
		caps |= JIT_SIMD_CAP_SSE2;
	if ((c & (1u << 0)) != 0)
		caps |= JIT_SIMD_CAP_SSE3;
	if ((c & (1u << 19)) != 0)
		caps |= JIT_SIMD_CAP_SSE41;
	if ((c & (1u << 27)) != 0 && (c & (1u << 28)) != 0) {
		__asm__ __volatile__("xgetbv"
				     : "=a"(xcr0_lo), "=d"(xcr0_hi)
				     : "c"(0));
		if ((xcr0_lo & 6u) == 6u) {
			caps |= JIT_SIMD_CAP_AVX;
			if ((c & (1u << 12)) != 0)
				caps |= JIT_SIMD_CAP_FMAF32X4;
		}
	}
	return caps;
#elif defined(_MSC_VER)
	int regs[4];
	unsigned __int64 xcr0;
	uint32_t caps;

	caps = 0;
	__cpuidex(regs, 1, 0);
	if (((uint32_t)regs[3] & (1u << 26)) != 0)
		caps |= JIT_SIMD_CAP_SSE2;
	if (((uint32_t)regs[2] & (1u << 0)) != 0)
		caps |= JIT_SIMD_CAP_SSE3;
	if (((uint32_t)regs[2] & (1u << 19)) != 0)
		caps |= JIT_SIMD_CAP_SSE41;
	if (((uint32_t)regs[2] & (1u << 27)) != 0 &&
	    ((uint32_t)regs[2] & (1u << 28)) != 0) {
		xcr0 = _xgetbv(0);
		if ((xcr0 & 6u) == 6u) {
			caps |= JIT_SIMD_CAP_AVX;
			if (((uint32_t)regs[2] & (1u << 12)) != 0)
				caps |= JIT_SIMD_CAP_FMAF32X4;
		}
	}
	return caps;
#else
	return 0;
#endif
}

#if defined(NOCT_USE_OPTIMIZER)
/* Visit vector instructions with SSE/AVX or direct scalar lowering. */
/* Visit an OP_VLOADI32X4 instruction. */
static INLINE bool
jit_visit_vloadi32x4_op(
        struct rt_jit_context *ctx)
{
        int vd;
        int base_tmp;
        int ofs_tmp;
        int inline_ok;
        int vreg_limit;
        uint32_t vbase;
        int lane;
        int base;
        int ofs;
        int cursor;

        CONSUME_IMM8(vd);
        CONSUME_TMPVAR(base_tmp);
        CONSUME_TMPVAR(ofs_tmp);

        /* Win64 keeps the memory-canonical direct scalar tier until xmm6/xmm7
           receive an explicitly tested save area.  SysV needs only SSE2;
           SSE4.1 selects shorter multiply/extract sequences below. */
        inline_ok = !IS_MSABI &&
                    (ctx->simd_caps & JIT_SIMD_CAP_SSE2) != 0;

        if (!inline_ok) {
                vbase = (uint32_t)offsetof(struct rt_env, vreg);
                base = base_tmp * (int)sizeof(struct rt_value);
                ofs = ofs_tmp * (int)sizeof(struct rt_value);
                ASM {
                        IB(0x49); IB(0x8b); IB(0x87); ID((uint32_t)(base + 8));
                        IB(0x49); IB(0x63); IB(0x8f); ID((uint32_t)(ofs + 8));
                }
                for (lane = 0; lane < 4; lane++) {
                        IB(0x8b); IB(0x54); IB(0x88); IB((uint8_t)(lane * 4));
                        IB(0x41); IB(0x89); IB(0x96);
                        ID(vbase + (uint32_t)vd * 16 + (uint32_t)lane * 4);
                }
                return true;
        }
        vreg_limit = ctx->vector_vreg_limit > 0 ? ctx->vector_vreg_limit : 13;

        /* SSE keeps xmm13/xmm14 scratch and xmm15 invariant.  AVX uses
         * non-destructive forms and admits logical xmm0..xmm14, reserving only
         * xmm15 as instruction-local scratch. */
        if (vd < 0 || vd >= vreg_limit) goto broken_vreg;

        base = base_tmp * (int)sizeof(struct rt_value);
        ofs = ofs_tmp * (int)sizeof(struct rt_value);
        cursor = -1;
        if (ctx->vector_hint_active) {
                if (ctx->vector_base_tmp[0] == base_tmp)
                        cursor = 0;
                else if (ctx->vector_base_tmp[1] == base_tmp)
                        cursor = 1;
        }
        if (cursor >= 0) {
                /* movdqu (rbx|rsi,rdi,4), xmmA */
                ASM { IB(0xf3); }
                if ((vd & 8) != 0) { ASM { IB(0x44); } }
                ASM { IB(0x0f); IB(0x6f);
                      IB((uint8_t)(0x04 | ((vd & 7) << 3)));
                      IB((uint8_t)(cursor == 0 ? 0xbb : 0xbe)); }
                return true;
        }
        ASM {
                /* r15: &env->frame->tmpvar[0] */
                /* movq base+8(%r15) -> %rax */    IB(0x49); IB(0x8b); IB(0x87); ID((uint32_t)(base + 8));
                /* movslq ofs+8(%r15) -> %rcx */   IB(0x49); IB(0x63); IB(0x8f); ID((uint32_t)(ofs + 8));
                /* movdqu (%rax,%rcx,4) -> %xmmA */ IB(0xf3);
        }
        if ((vd & 8) != 0) { ASM { IB(0x44); } }
        ASM { IB(0x0f); IB(0x6f);
              IB((uint8_t)(0x04 | ((vd & 7) << 3))); IB(0x88); }
        return true;

        broken_vreg:
        rt_error(ctx->env, BROKEN_BYTECODE);
        return false;
}

/* Visit an OP_VSTOREI32X4 instruction. */
static INLINE bool
jit_visit_vstorei32x4_op(
        struct rt_jit_context *ctx)
{
        int base_tmp;
        int ofs_tmp;
        int vs;
        int inline_ok;
        int vreg_limit;
        uint32_t vbase;
        int lane;
        int base;
        int ofs;
        int cursor;

        CONSUME_TMPVAR(base_tmp);
        CONSUME_TMPVAR(ofs_tmp);
        CONSUME_IMM8(vs);

        /* Win64 keeps the memory-canonical direct scalar tier until xmm6/xmm7
           receive an explicitly tested save area.  SysV needs only SSE2;
           SSE4.1 selects shorter multiply/extract sequences below. */
        inline_ok = !IS_MSABI &&
                    (ctx->simd_caps & JIT_SIMD_CAP_SSE2) != 0;

        if (!inline_ok) {
                vbase = (uint32_t)offsetof(struct rt_env, vreg);
                base = base_tmp * (int)sizeof(struct rt_value);
                ofs = ofs_tmp * (int)sizeof(struct rt_value);
                ASM {
                        IB(0x49); IB(0x8b); IB(0x87); ID((uint32_t)(base + 8));
                        IB(0x49); IB(0x63); IB(0x8f); ID((uint32_t)(ofs + 8));
                }
                for (lane = 0; lane < 4; lane++) {
                        IB(0x41); IB(0x8b); IB(0x96);
                        ID(vbase + (uint32_t)vs * 16 + (uint32_t)lane * 4);
                        IB(0x89); IB(0x54); IB(0x88); IB((uint8_t)(lane * 4));
                }
                return true;
        }
        vreg_limit = ctx->vector_vreg_limit > 0 ? ctx->vector_vreg_limit : 13;

        /* SSE keeps xmm13/xmm14 scratch and xmm15 invariant.  AVX uses
         * non-destructive forms and admits logical xmm0..xmm14, reserving only
         * xmm15 as instruction-local scratch. */
        if (vs < 0 || vs >= vreg_limit) goto broken_vreg;

        base = base_tmp * (int)sizeof(struct rt_value);
        ofs = ofs_tmp * (int)sizeof(struct rt_value);
        cursor = -1;
        if (ctx->vector_hint_active) {
                if (ctx->vector_base_tmp[0] == base_tmp)
                        cursor = 0;
                else if (ctx->vector_base_tmp[1] == base_tmp)
                        cursor = 1;
        }
        if (cursor >= 0) {
                /* movdqu xmmC, (rbx|rsi,rdi,4) */
                ASM { IB(0xf3); }
                if ((vs & 8) != 0) { ASM { IB(0x44); } }
                ASM { IB(0x0f); IB(0x7f);
                      IB((uint8_t)(0x04 | ((vs & 7) << 3)));
                      IB((uint8_t)(cursor == 0 ? 0xbb : 0xbe)); }
                return true;
        }
        ASM {
                /* movq base+8(%r15) -> %rax */    IB(0x49); IB(0x8b); IB(0x87); ID((uint32_t)(base + 8));
                /* movslq ofs+8(%r15) -> %rcx */   IB(0x49); IB(0x63); IB(0x8f); ID((uint32_t)(ofs + 8));
                /* movdqu %xmmC -> (%rax,%rcx,4) */ IB(0xf3);
        }
        if ((vs & 8) != 0) { ASM { IB(0x44); } }
        ASM { IB(0x0f); IB(0x7f);
              IB((uint8_t)(0x04 | ((vs & 7) << 3))); IB(0x88); }
        return true;

        broken_vreg:
        rt_error(ctx->env, BROKEN_BYTECODE);
        return false;
}

/* Visit an OP_VSPLATI32 instruction. */
static INLINE bool
jit_visit_vsplati32_op(
        struct rt_jit_context *ctx)
{
        int vd;
        int src_tmp;
        int inline_ok;
        int vreg_limit;
        uint32_t vbase;
        int lane;
        int src;

        CONSUME_IMM8(vd);
        CONSUME_TMPVAR(src_tmp);

        /* Win64 keeps the memory-canonical direct scalar tier until xmm6/xmm7
           receive an explicitly tested save area.  SysV needs only SSE2;
           SSE4.1 selects shorter multiply/extract sequences below. */
        inline_ok = !IS_MSABI &&
                    (ctx->simd_caps & JIT_SIMD_CAP_SSE2) != 0;

        if (!inline_ok) {
                vbase = (uint32_t)offsetof(struct rt_env, vreg);
                src = src_tmp * (int)sizeof(struct rt_value);
                ASM { IB(0x41); IB(0x8b); IB(0x87); ID((uint32_t)(src + 8)); }
                for (lane = 0; lane < 4; lane++) {
                        IB(0x41); IB(0x89); IB(0x86);
                        ID(vbase + (uint32_t)vd * 16 + (uint32_t)lane * 4);
                }
                return true;
        }
        vreg_limit = ctx->vector_vreg_limit > 0 ? ctx->vector_vreg_limit : 13;

        /* SSE keeps xmm13/xmm14 scratch and xmm15 invariant.  AVX uses
         * non-destructive forms and admits logical xmm0..xmm14, reserving only
         * xmm15 as instruction-local scratch. */
        if (vd < 0 || vd >= vreg_limit) goto broken_vreg;

        src = src_tmp * (int)sizeof(struct rt_value);
        ASM { IB(0x66); IB((uint8_t)((vd & 8) != 0 ? 0x45 : 0x41));
              IB(0x0f); IB(0x6e);
              IB((uint8_t)(0x87 | ((vd & 7) << 3)));
              ID((uint32_t)(src + 8)); }
        if (!jit_x86_64_put_sse_rr(ctx, 0x66, 1, 0x70, vd, vd))
                return false;
        ASM { IB(0x00); }
        return true;

        broken_vreg:
        rt_error(ctx->env, BROKEN_BYTECODE);
        return false;
}

/* Visit an OP_VGETLANEI32 instruction. */
static INLINE bool
jit_visit_vgetlanei32_op(
        struct rt_jit_context *ctx)
{
        int dst_tmp;
        int vs;
        int lane_index;
        int inline_ok;
        int vreg_limit;
        uint32_t vbase;
        int d;
        int dst;

        CONSUME_TMPVAR(dst_tmp);
        CONSUME_IMM8(vs);
        CONSUME_IMM8(lane_index);

        /* Win64 keeps the memory-canonical direct scalar tier until xmm6/xmm7
           receive an explicitly tested save area.  SysV needs only SSE2;
           SSE4.1 selects shorter multiply/extract sequences below. */
        inline_ok = !IS_MSABI &&
                    (ctx->simd_caps & JIT_SIMD_CAP_SSE2) != 0;

        if (!inline_ok) {
                vbase = (uint32_t)offsetof(struct rt_env, vreg);

                d = dst_tmp * (int)sizeof(struct rt_value);
                ASM {
                        IB(0x41); IB(0x8b); IB(0x86);
                        ID(vbase + (uint32_t)vs * 16 + (uint32_t)lane_index * 4);
                        IB(0x41); IB(0xc7); IB(0x87); ID((uint32_t)d);
                        ID((uint32_t)(NOCT_VALUE_INT));
                        IB(0x41); IB(0x89); IB(0x87); ID((uint32_t)(d + 8));
                }
                return true;
        }
        vreg_limit = ctx->vector_vreg_limit > 0 ? ctx->vector_vreg_limit : 13;

        /* SSE keeps xmm13/xmm14 scratch and xmm15 invariant.  AVX uses
         * non-destructive forms and admits logical xmm0..xmm14, reserving only
         * xmm15 as instruction-local scratch. */
        if (vs < 0 || vs >= vreg_limit) goto broken_vreg;

        dst = dst_tmp * (int)sizeof(struct rt_value);
        if ((ctx->simd_caps & JIT_SIMD_CAP_SSE41) != 0) {
                ASM { IB(0x66);
                      IB((uint8_t)((vs & 8) != 0 ? 0x45 : 0x41));
                      IB(0x0f); IB(0x3a); IB(0x16);
                      IB((uint8_t)(0x87 | ((vs & 7) << 3)));
                      ID((uint32_t)(dst + 8)); IB((uint8_t)lane_index); }
        } else {
                ASM { IB(0x66); }
                if ((vs & 8) != 0) { ASM { IB(0x41); } }
                ASM {
                        /* SSE2: combine two pextrw results without changing xmmB. */
                        IB(0x0f); IB(0xc5); IB((uint8_t)(0xc0 | (vs & 7))); IB((uint8_t)(lane_index * 2));
                        IB(0x66);
                }
                if ((vs & 8) != 0) { ASM { IB(0x41); } }
                ASM {
                        IB(0x0f); IB(0xc5); IB((uint8_t)(0xc8 | (vs & 7))); IB((uint8_t)(lane_index * 2 + 1));
                        IB(0xc1); IB(0xe1); IB(0x10);
                        IB(0x09); IB(0xc8);
                        IB(0x41); IB(0x89); IB(0x87); ID((uint32_t)(dst + 8));
                }
        }
        ASM {
                /* Both i32 and f32 lanes are raw 32-bit payloads. */
                IB(0x41); IB(0xc7); IB(0x87); ID((uint32_t)dst);
                ID((uint32_t)(NOCT_VALUE_INT));
        }
        return true;

        broken_vreg:
        rt_error(ctx->env, BROKEN_BYTECODE);
        return false;
}

/* Visit an OP_VMOV128 instruction. */
static INLINE bool
jit_visit_vmov128_op(
        struct rt_jit_context *ctx)
{
        int vd;
        int vs;
        int inline_ok;
        int vreg_limit;
        uint32_t vbase;
        int lane;

        CONSUME_IMM8(vd);
        CONSUME_IMM8(vs);

        /* Win64 keeps the memory-canonical direct scalar tier until xmm6/xmm7
           receive an explicitly tested save area.  SysV needs only SSE2;
           SSE4.1 selects shorter multiply/extract sequences below. */
        inline_ok = !IS_MSABI &&
                    (ctx->simd_caps & JIT_SIMD_CAP_SSE2) != 0;

        if (!inline_ok) {
                vbase = (uint32_t)offsetof(struct rt_env, vreg);
                for (lane = 0; lane < 4; lane++) {
                        IB(0x41); IB(0x8b); IB(0x86);
                        ID(vbase + (uint32_t)vs * 16 + (uint32_t)lane * 4);
                        IB(0x41); IB(0x89); IB(0x86);
                        ID(vbase + (uint32_t)vd * 16 + (uint32_t)lane * 4);
                }
                return true;
        }
        vreg_limit = ctx->vector_vreg_limit > 0 ? ctx->vector_vreg_limit : 13;

        /* SSE keeps xmm13/xmm14 scratch and xmm15 invariant.  AVX uses
         * non-destructive forms and admits logical xmm0..xmm14, reserving only
         * xmm15 as instruction-local scratch. */
        if (vd < 0 || vd >= vreg_limit || vs < 0 || vs >= vreg_limit)
                goto broken_vreg;

        if (vd != vs) {
                if ((ctx->simd_caps & JIT_SIMD_CAP_AVX) != 0) {
                        if (!jit_x86_64_put_vex_rr(ctx, 1, 1, 0x6f,
                                                 vd, vs))
                                return false;
                } else if (!jit_x86_64_put_sse_rr(ctx, 0x66, 1,
                                                  0x6f, vd, vs)) {
                        return false;
                }
        }

        return true;

        broken_vreg:
        rt_error(ctx->env, BROKEN_BYTECODE);
        return false;
}

/* Visit an OP_VADDI32X4 instruction. */
static INLINE bool
jit_visit_vaddi32x4_op(
        struct rt_jit_context *ctx)
{
        int vd;
        int lhs;
        int rhs;
        int inline_ok;
        int vreg_limit;
        uint32_t vbase;
        int lane;

        CONSUME_IMM8(vd);
        CONSUME_IMM8(lhs);
        CONSUME_IMM8(rhs);

        /* Win64 keeps the memory-canonical direct scalar tier until xmm6/xmm7
           receive an explicitly tested save area.  SysV needs only SSE2;
           SSE4.1 selects shorter multiply/extract sequences below. */
        inline_ok = !IS_MSABI &&
                    (ctx->simd_caps & JIT_SIMD_CAP_SSE2) != 0;

        if (!inline_ok) {
                vbase = (uint32_t)offsetof(struct rt_env, vreg);
                for (lane = 0; lane < 4; lane++) {
                        uint32_t a;
                        uint32_t b;
                        uint32_t d;

                        a = vbase + (uint32_t)lhs * 16 + (uint32_t)lane * 4;
                        b = vbase + (uint32_t)rhs * 16 + (uint32_t)lane * 4;
                        d = vbase + (uint32_t)vd * 16 + (uint32_t)lane * 4;
                        IB(0x41); IB(0x8b); IB(0x86); ID(a);

                        IB(0x41);
                        IB(0x03); IB(0x86); ID(b);
                        IB(0x41); IB(0x89); IB(0x86); ID(d);
                }
                return true;
        }
        vreg_limit = ctx->vector_vreg_limit > 0 ? ctx->vector_vreg_limit : 13;

        /* SSE keeps xmm13/xmm14 scratch and xmm15 invariant.  AVX uses
         * non-destructive forms and admits logical xmm0..xmm14, reserving only
         * xmm15 as instruction-local scratch. */
        if (vd < 0 || vd >= vreg_limit || lhs < 0 || lhs >= vreg_limit ||
            rhs < 0 || rhs >= vreg_limit)
                goto broken_vreg;

        if ((ctx->simd_caps & JIT_SIMD_CAP_AVX) != 0) {
                int map;
                uint8_t opcode;

                map = 1;
                opcode = 0xfe;
                if (!jit_x86_64_put_vex_rrr(ctx, map, 1, opcode,
                                              vd, lhs, rhs))
                        return false;
                return true;
        }
        /* Legacy two-address lowering. */

        if (vd != lhs && !jit_x86_64_put_sse_rr(ctx, 0x66, 1, 0x6f,
                                                 vd, lhs))
                return false;
        if (!jit_x86_64_put_sse_rr(ctx, 0x66, 1, 0xfe, vd, rhs))
                return false;

        return true;

        broken_vreg:
        rt_error(ctx->env, BROKEN_BYTECODE);
        return false;
}

/* Visit an OP_VSUBI32X4 instruction. */
static INLINE bool
jit_visit_vsubi32x4_op(
        struct rt_jit_context *ctx)
{
        int vd;
        int lhs;
        int rhs;
        int inline_ok;
        int vreg_limit;
        uint32_t vbase;
        int lane;

        CONSUME_IMM8(vd);
        CONSUME_IMM8(lhs);
        CONSUME_IMM8(rhs);

        /* Win64 keeps the memory-canonical direct scalar tier until xmm6/xmm7
           receive an explicitly tested save area.  SysV needs only SSE2;
           SSE4.1 selects shorter multiply/extract sequences below. */
        inline_ok = !IS_MSABI &&
                    (ctx->simd_caps & JIT_SIMD_CAP_SSE2) != 0;

        if (!inline_ok) {
                vbase = (uint32_t)offsetof(struct rt_env, vreg);
                for (lane = 0; lane < 4; lane++) {
                        uint32_t a;
                        uint32_t b;
                        uint32_t d;

                        a = vbase + (uint32_t)lhs * 16 + (uint32_t)lane * 4;
                        b = vbase + (uint32_t)rhs * 16 + (uint32_t)lane * 4;
                        d = vbase + (uint32_t)vd * 16 + (uint32_t)lane * 4;
                        IB(0x41); IB(0x8b); IB(0x86); ID(a);

                        IB(0x41);
                        IB(0x2b); IB(0x86); ID(b);
                        IB(0x41); IB(0x89); IB(0x86); ID(d);
                }
                return true;
        }
        vreg_limit = ctx->vector_vreg_limit > 0 ? ctx->vector_vreg_limit : 13;

        /* SSE keeps xmm13/xmm14 scratch and xmm15 invariant.  AVX uses
         * non-destructive forms and admits logical xmm0..xmm14, reserving only
         * xmm15 as instruction-local scratch. */
        if (vd < 0 || vd >= vreg_limit || lhs < 0 || lhs >= vreg_limit ||
            rhs < 0 || rhs >= vreg_limit)
                goto broken_vreg;

        if ((ctx->simd_caps & JIT_SIMD_CAP_AVX) != 0) {
                int map;
                uint8_t opcode;

                map = 1;
                opcode = 0xfa;
                if (!jit_x86_64_put_vex_rrr(ctx, map, 1, opcode,
                                              vd, lhs, rhs))
                        return false;
                return true;
        }
        /* Legacy two-address lowering. */

        if (vd != lhs && !jit_x86_64_put_sse_rr(ctx, 0x66, 1, 0x6f,
                                                 vd, lhs))
                return false;
        if (!jit_x86_64_put_sse_rr(ctx, 0x66, 1, 0xfa, vd, rhs))
                return false;

        return true;

        broken_vreg:
        rt_error(ctx->env, BROKEN_BYTECODE);
        return false;
}

/* Visit an OP_VMULI32X4 instruction. */
static INLINE bool
jit_visit_vmuli32x4_op(
        struct rt_jit_context *ctx)
{
        int vd;
        int lhs;
        int rhs;
        int inline_ok;
        int vreg_limit;
        uint32_t vbase;
        int lane;

        CONSUME_IMM8(vd);
        CONSUME_IMM8(lhs);
        CONSUME_IMM8(rhs);

        /* Win64 keeps the memory-canonical direct scalar tier until xmm6/xmm7
           receive an explicitly tested save area.  SysV needs only SSE2;
           SSE4.1 selects shorter multiply/extract sequences below. */
        inline_ok = !IS_MSABI &&
                    (ctx->simd_caps & JIT_SIMD_CAP_SSE2) != 0;

        if (!inline_ok) {
                vbase = (uint32_t)offsetof(struct rt_env, vreg);
                for (lane = 0; lane < 4; lane++) {
                        uint32_t a;
                        uint32_t b;
                        uint32_t d;

                        a = vbase + (uint32_t)lhs * 16 + (uint32_t)lane * 4;
                        b = vbase + (uint32_t)rhs * 16 + (uint32_t)lane * 4;
                        d = vbase + (uint32_t)vd * 16 + (uint32_t)lane * 4;
                        IB(0x41); IB(0x8b); IB(0x86); ID(a);

                        IB(0x41);
                        IB(0x0f); IB(0xaf); IB(0x86); ID(b);
                        IB(0x41); IB(0x89); IB(0x86); ID(d);
                }
                return true;
        }
        vreg_limit = ctx->vector_vreg_limit > 0 ? ctx->vector_vreg_limit : 13;

        /* SSE keeps xmm13/xmm14 scratch and xmm15 invariant.  AVX uses
         * non-destructive forms and admits logical xmm0..xmm14, reserving only
         * xmm15 as instruction-local scratch. */
        if (vd < 0 || vd >= vreg_limit || lhs < 0 || lhs >= vreg_limit ||
            rhs < 0 || rhs >= vreg_limit)
                goto broken_vreg;

        if ((ctx->simd_caps & JIT_SIMD_CAP_AVX) != 0) {
                int map;
                uint8_t opcode;

                map = 2;
                opcode = 0x40;
                if (!jit_x86_64_put_vex_rrr(ctx, map, 1, opcode,
                                              vd, lhs, rhs))
                        return false;
                return true;
        }
        /* Legacy two-address lowering. */
        if ((ctx->simd_caps & JIT_SIMD_CAP_SSE41) == 0) {
                /* xmm13/xmm14 are reserved outside the logical map. */
                if (!jit_x86_64_put_sse_rr(ctx, 0x66, 1, 0x6f, 13, lhs) ||
                    !jit_x86_64_put_sse_rr(ctx, 0x66, 1, 0x70, 13, 13))
                        return false;
                ASM { IB(0xf5); }
                if (!jit_x86_64_put_sse_rr(ctx, 0x66, 1, 0x6f, 14, rhs) ||
                    !jit_x86_64_put_sse_rr(ctx, 0x66, 1, 0x70, 14, 14))
                        return false;
                ASM { IB(0xf5); }
                if (!jit_x86_64_put_sse_rr(ctx, 0x66, 1, 0xf4, 13, 14))
                        return false;
        }
        if (vd != lhs && !jit_x86_64_put_sse_rr(ctx, 0x66, 1, 0x6f,
                                                 vd, lhs))
                return false;
        if ((ctx->simd_caps & JIT_SIMD_CAP_SSE41) != 0) {
                if (!jit_x86_64_put_sse_rr(ctx, 0x66, 2, 0x40,
                                          vd, rhs))
                        return false;
        } else {
                if (!jit_x86_64_put_sse_rr(ctx, 0x66, 1, 0xf4,
                                          vd, rhs) ||
                    !jit_x86_64_put_sse_rr(ctx, 0x66, 1, 0x70,
                                          vd, vd))
                        return false;
                ASM { IB(0x88); }
                if (!jit_x86_64_put_sse_rr(ctx, 0x66, 1, 0x70,
                                          13, 13))
                        return false;
                ASM { IB(0x88); }
                if (!jit_x86_64_put_sse_rr(ctx, 0x66, 1, 0x62,
                                          vd, 13))
                        return false;
        }

        return true;

        broken_vreg:
        rt_error(ctx->env, BROKEN_BYTECODE);
        return false;
}

/* Visit an OP_VAND128 instruction. */
static INLINE bool
jit_visit_vand128_op(
        struct rt_jit_context *ctx)
{
        int vd;
        int lhs;
        int rhs;
        int inline_ok;
        int vreg_limit;
        uint32_t vbase;
        int lane;

        CONSUME_IMM8(vd);
        CONSUME_IMM8(lhs);
        CONSUME_IMM8(rhs);

        /* Win64 keeps the memory-canonical direct scalar tier until xmm6/xmm7
           receive an explicitly tested save area.  SysV needs only SSE2;
           SSE4.1 selects shorter multiply/extract sequences below. */
        inline_ok = !IS_MSABI &&
                    (ctx->simd_caps & JIT_SIMD_CAP_SSE2) != 0;

        if (!inline_ok) {
                vbase = (uint32_t)offsetof(struct rt_env, vreg);
                for (lane = 0; lane < 4; lane++) {
                        uint32_t a;
                        uint32_t b;
                        uint32_t d;

                        a = vbase + (uint32_t)lhs * 16 + (uint32_t)lane * 4;
                        b = vbase + (uint32_t)rhs * 16 + (uint32_t)lane * 4;
                        d = vbase + (uint32_t)vd * 16 + (uint32_t)lane * 4;
                        IB(0x41); IB(0x8b); IB(0x86); ID(a);

                        IB(0x41);
                        IB(0x23); IB(0x86); ID(b);
                        IB(0x41); IB(0x89); IB(0x86); ID(d);
                }
                return true;
        }
        vreg_limit = ctx->vector_vreg_limit > 0 ? ctx->vector_vreg_limit : 13;

        /* SSE keeps xmm13/xmm14 scratch and xmm15 invariant.  AVX uses
         * non-destructive forms and admits logical xmm0..xmm14, reserving only
         * xmm15 as instruction-local scratch. */
        if (vd < 0 || vd >= vreg_limit || lhs < 0 || lhs >= vreg_limit ||
            rhs < 0 || rhs >= vreg_limit)
                goto broken_vreg;

        if ((ctx->simd_caps & JIT_SIMD_CAP_AVX) != 0) {
                int map;
                uint8_t opcode;

                map = 1;
                opcode = 0xdb;
                if (!jit_x86_64_put_vex_rrr(ctx, map, 1, opcode,
                                              vd, lhs, rhs))
                        return false;
                return true;
        }
        /* Legacy two-address lowering. */

        if (vd != lhs && !jit_x86_64_put_sse_rr(ctx, 0x66, 1, 0x6f,
                                                 vd, lhs))
                return false;
        if (!jit_x86_64_put_sse_rr(ctx, 0x66, 1, 0xdb, vd, rhs))
                return false;

        return true;

        broken_vreg:
        rt_error(ctx->env, BROKEN_BYTECODE);
        return false;
}

/* Visit an OP_VOR128 instruction. */
static INLINE bool
jit_visit_vor128_op(
        struct rt_jit_context *ctx)
{
        int vd;
        int lhs;
        int rhs;
        int inline_ok;
        int vreg_limit;
        uint32_t vbase;
        int lane;

        CONSUME_IMM8(vd);
        CONSUME_IMM8(lhs);
        CONSUME_IMM8(rhs);

        /* Win64 keeps the memory-canonical direct scalar tier until xmm6/xmm7
           receive an explicitly tested save area.  SysV needs only SSE2;
           SSE4.1 selects shorter multiply/extract sequences below. */
        inline_ok = !IS_MSABI &&
                    (ctx->simd_caps & JIT_SIMD_CAP_SSE2) != 0;

        if (!inline_ok) {
                vbase = (uint32_t)offsetof(struct rt_env, vreg);
                for (lane = 0; lane < 4; lane++) {
                        uint32_t a;
                        uint32_t b;
                        uint32_t d;

                        a = vbase + (uint32_t)lhs * 16 + (uint32_t)lane * 4;
                        b = vbase + (uint32_t)rhs * 16 + (uint32_t)lane * 4;
                        d = vbase + (uint32_t)vd * 16 + (uint32_t)lane * 4;
                        IB(0x41); IB(0x8b); IB(0x86); ID(a);

                        IB(0x41);
                        IB(0x0b); IB(0x86); ID(b);
                        IB(0x41); IB(0x89); IB(0x86); ID(d);
                }
                return true;
        }
        vreg_limit = ctx->vector_vreg_limit > 0 ? ctx->vector_vreg_limit : 13;

        /* SSE keeps xmm13/xmm14 scratch and xmm15 invariant.  AVX uses
         * non-destructive forms and admits logical xmm0..xmm14, reserving only
         * xmm15 as instruction-local scratch. */
        if (vd < 0 || vd >= vreg_limit || lhs < 0 || lhs >= vreg_limit ||
            rhs < 0 || rhs >= vreg_limit)
                goto broken_vreg;

        if ((ctx->simd_caps & JIT_SIMD_CAP_AVX) != 0) {
                int map;
                uint8_t opcode;

                map = 1;
                opcode = 0xeb;
                if (!jit_x86_64_put_vex_rrr(ctx, map, 1, opcode,
                                              vd, lhs, rhs))
                        return false;
                return true;
        }
        /* Legacy two-address lowering. */

        if (vd != lhs && !jit_x86_64_put_sse_rr(ctx, 0x66, 1, 0x6f,
                                                 vd, lhs))
                return false;
        if (!jit_x86_64_put_sse_rr(ctx, 0x66, 1, 0xeb, vd, rhs))
                return false;

        return true;

        broken_vreg:
        rt_error(ctx->env, BROKEN_BYTECODE);
        return false;
}

/* Visit an OP_VXOR128 instruction. */
static INLINE bool
jit_visit_vxor128_op(
        struct rt_jit_context *ctx)
{
        int vd;
        int lhs;
        int rhs;
        int inline_ok;
        int vreg_limit;
        uint32_t vbase;
        int lane;

        CONSUME_IMM8(vd);
        CONSUME_IMM8(lhs);
        CONSUME_IMM8(rhs);

        /* Win64 keeps the memory-canonical direct scalar tier until xmm6/xmm7
           receive an explicitly tested save area.  SysV needs only SSE2;
           SSE4.1 selects shorter multiply/extract sequences below. */
        inline_ok = !IS_MSABI &&
                    (ctx->simd_caps & JIT_SIMD_CAP_SSE2) != 0;

        if (!inline_ok) {
                vbase = (uint32_t)offsetof(struct rt_env, vreg);
                for (lane = 0; lane < 4; lane++) {
                        uint32_t a;
                        uint32_t b;
                        uint32_t d;

                        a = vbase + (uint32_t)lhs * 16 + (uint32_t)lane * 4;
                        b = vbase + (uint32_t)rhs * 16 + (uint32_t)lane * 4;
                        d = vbase + (uint32_t)vd * 16 + (uint32_t)lane * 4;
                        IB(0x41); IB(0x8b); IB(0x86); ID(a);

                        IB(0x41);
                        IB(0x33); IB(0x86); ID(b);
                        IB(0x41); IB(0x89); IB(0x86); ID(d);
                }
                return true;
        }
        vreg_limit = ctx->vector_vreg_limit > 0 ? ctx->vector_vreg_limit : 13;

        /* SSE keeps xmm13/xmm14 scratch and xmm15 invariant.  AVX uses
         * non-destructive forms and admits logical xmm0..xmm14, reserving only
         * xmm15 as instruction-local scratch. */
        if (vd < 0 || vd >= vreg_limit || lhs < 0 || lhs >= vreg_limit ||
            rhs < 0 || rhs >= vreg_limit)
                goto broken_vreg;

        if ((ctx->simd_caps & JIT_SIMD_CAP_AVX) != 0) {
                int map;
                uint8_t opcode;

                map = 1;
                opcode = 0xef;
                if (!jit_x86_64_put_vex_rrr(ctx, map, 1, opcode,
                                              vd, lhs, rhs))
                        return false;
                return true;
        }
        /* Legacy two-address lowering. */

        if (vd != lhs && !jit_x86_64_put_sse_rr(ctx, 0x66, 1, 0x6f,
                                                 vd, lhs))
                return false;
        if (!jit_x86_64_put_sse_rr(ctx, 0x66, 1, 0xef, vd, rhs))
                return false;

        return true;

        broken_vreg:
        rt_error(ctx->env, BROKEN_BYTECODE);
        return false;
}

/* Visit an OP_VSHLI32X4 instruction. */
static INLINE bool
jit_visit_vshli32x4_op(
        struct rt_jit_context *ctx)
{
        int vd;
        int vs;
        int shift;
        int inline_ok;
        int vreg_limit;
        uint32_t vbase;
        int lane;

        CONSUME_IMM8(vd);
        CONSUME_IMM8(vs);
        CONSUME_IMM8(shift);

        /* Win64 keeps the memory-canonical direct scalar tier until xmm6/xmm7
           receive an explicitly tested save area.  SysV needs only SSE2;
           SSE4.1 selects shorter multiply/extract sequences below. */
        inline_ok = !IS_MSABI &&
                    (ctx->simd_caps & JIT_SIMD_CAP_SSE2) != 0;

        if (!inline_ok) {
                vbase = (uint32_t)offsetof(struct rt_env, vreg);
                for (lane = 0; lane < 4; lane++) {
                        uint32_t s;
                        uint32_t d;

                        s = vbase + (uint32_t)vs * 16 + (uint32_t)lane * 4;
                        d = vbase + (uint32_t)vd * 16 + (uint32_t)lane * 4;
                        IB(0x41); IB(0x8b); IB(0x86); ID(s);
                        IB(0xc1); IB((uint8_t)(0xe0));
                        IB((uint8_t)shift);
                        IB(0x41); IB(0x89); IB(0x86); ID(d);
                }
                return true;
        }
        vreg_limit = ctx->vector_vreg_limit > 0 ? ctx->vector_vreg_limit : 13;

        /* SSE keeps xmm13/xmm14 scratch and xmm15 invariant.  AVX uses
         * non-destructive forms and admits logical xmm0..xmm14, reserving only
         * xmm15 as instruction-local scratch. */
        if (vd < 0 || vd >= vreg_limit || vs < 0 || vs >= vreg_limit)
                goto broken_vreg;

        if ((ctx->simd_caps & JIT_SIMD_CAP_AVX) != 0) {
                if (!jit_x86_64_put_vex_shift(ctx,
                                6,
                                vd, vs, (uint8_t)shift))
                        return false;
                return true;
        }
        if (vd != vs && !jit_x86_64_put_sse_rr(ctx, 0x66, 1, 0x6f,
                                                 vd, vs))
                return false;
        ASM { IB(0x66); }
        if ((vd & 8) != 0) { ASM { IB(0x41); } }
        ASM { IB(0x0f); IB(0x72);
              IB((uint8_t)((0xf0) |
                           (vd & 7))); IB((uint8_t)shift); }

        return true;

        broken_vreg:
        rt_error(ctx->env, BROKEN_BYTECODE);
        return false;
}

/* Visit an OP_VSHRI32X4 instruction. */
static INLINE bool
jit_visit_vshri32x4_op(
        struct rt_jit_context *ctx)
{
        int vd;
        int vs;
        int shift;
        int inline_ok;
        int vreg_limit;
        uint32_t vbase;
        int lane;

        CONSUME_IMM8(vd);
        CONSUME_IMM8(vs);
        CONSUME_IMM8(shift);

        /* Win64 keeps the memory-canonical direct scalar tier until xmm6/xmm7
           receive an explicitly tested save area.  SysV needs only SSE2;
           SSE4.1 selects shorter multiply/extract sequences below. */
        inline_ok = !IS_MSABI &&
                    (ctx->simd_caps & JIT_SIMD_CAP_SSE2) != 0;

        if (!inline_ok) {
                vbase = (uint32_t)offsetof(struct rt_env, vreg);
                for (lane = 0; lane < 4; lane++) {
                        uint32_t s;
                        uint32_t d;

                        s = vbase + (uint32_t)vs * 16 + (uint32_t)lane * 4;
                        d = vbase + (uint32_t)vd * 16 + (uint32_t)lane * 4;
                        IB(0x41); IB(0x8b); IB(0x86); ID(s);
                        IB(0xc1); IB((uint8_t)(0xe8));
                        IB((uint8_t)shift);
                        IB(0x41); IB(0x89); IB(0x86); ID(d);
                }
                return true;
        }
        vreg_limit = ctx->vector_vreg_limit > 0 ? ctx->vector_vreg_limit : 13;

        /* SSE keeps xmm13/xmm14 scratch and xmm15 invariant.  AVX uses
         * non-destructive forms and admits logical xmm0..xmm14, reserving only
         * xmm15 as instruction-local scratch. */
        if (vd < 0 || vd >= vreg_limit || vs < 0 || vs >= vreg_limit)
                goto broken_vreg;

        if ((ctx->simd_caps & JIT_SIMD_CAP_AVX) != 0) {
                if (!jit_x86_64_put_vex_shift(ctx,
                                2,
                                vd, vs, (uint8_t)shift))
                        return false;
                return true;
        }
        if (vd != vs && !jit_x86_64_put_sse_rr(ctx, 0x66, 1, 0x6f,
                                                 vd, vs))
                return false;
        ASM { IB(0x66); }
        if ((vd & 8) != 0) { ASM { IB(0x41); } }
        ASM { IB(0x0f); IB(0x72);
              IB((uint8_t)((0xd0) |
                           (vd & 7))); IB((uint8_t)shift); }

        return true;

        broken_vreg:
        rt_error(ctx->env, BROKEN_BYTECODE);
        return false;
}

/* Visit an OP_VLOADF32X4 instruction. */
static INLINE bool
jit_visit_vloadf32x4_op(
        struct rt_jit_context *ctx)
{
        int vd;
        int base_tmp;
        int ofs_tmp;
        int inline_ok;
        int vreg_limit;
        uint32_t vbase;
        int lane;
        int base;
        int ofs;
        int cursor;

        CONSUME_IMM8(vd);
        CONSUME_TMPVAR(base_tmp);
        CONSUME_TMPVAR(ofs_tmp);

        /* Win64 keeps the memory-canonical direct scalar tier until xmm6/xmm7
           receive an explicitly tested save area.  SysV needs only SSE2;
           SSE4.1 selects shorter multiply/extract sequences below. */
        inline_ok = !IS_MSABI &&
                    (ctx->simd_caps & JIT_SIMD_CAP_SSE2) != 0;

        if (!inline_ok) {
                vbase = (uint32_t)offsetof(struct rt_env, vreg);
                base = base_tmp * (int)sizeof(struct rt_value);
                ofs = ofs_tmp * (int)sizeof(struct rt_value);
                ASM {
                        IB(0x49); IB(0x8b); IB(0x87); ID((uint32_t)(base + 8));
                        IB(0x49); IB(0x63); IB(0x8f); ID((uint32_t)(ofs + 8));
                }
                for (lane = 0; lane < 4; lane++) {
                        IB(0x8b); IB(0x54); IB(0x88); IB((uint8_t)(lane * 4));
                        IB(0x41); IB(0x89); IB(0x96);
                        ID(vbase + (uint32_t)vd * 16 + (uint32_t)lane * 4);
                }
                return true;
        }
        vreg_limit = ctx->vector_vreg_limit > 0 ? ctx->vector_vreg_limit : 13;

        /* SSE keeps xmm13/xmm14 scratch and xmm15 invariant.  AVX uses
         * non-destructive forms and admits logical xmm0..xmm14, reserving only
         * xmm15 as instruction-local scratch. */
        if (vd < 0 || vd >= vreg_limit) goto broken_vreg;

        base = base_tmp * (int)sizeof(struct rt_value);
        ofs = ofs_tmp * (int)sizeof(struct rt_value);
        cursor = -1;
        if (ctx->vector_hint_active) {
                if (ctx->vector_base_tmp[0] == base_tmp)
                        cursor = 0;
                else if (ctx->vector_base_tmp[1] == base_tmp)
                        cursor = 1;
        }
        if (cursor >= 0) {
                /* movdqu (rbx|rsi,rdi,4), xmmA */
                ASM { IB(0xf3); }
                if ((vd & 8) != 0) { ASM { IB(0x44); } }
                ASM { IB(0x0f); IB(0x6f);
                      IB((uint8_t)(0x04 | ((vd & 7) << 3)));
                      IB((uint8_t)(cursor == 0 ? 0xbb : 0xbe)); }
                return true;
        }
        ASM {
                /* r15: &env->frame->tmpvar[0] */
                /* movq base+8(%r15) -> %rax */    IB(0x49); IB(0x8b); IB(0x87); ID((uint32_t)(base + 8));
                /* movslq ofs+8(%r15) -> %rcx */   IB(0x49); IB(0x63); IB(0x8f); ID((uint32_t)(ofs + 8));
                /* movdqu (%rax,%rcx,4) -> %xmmA */ IB(0xf3);
        }
        if ((vd & 8) != 0) { ASM { IB(0x44); } }
        ASM { IB(0x0f); IB(0x6f);
              IB((uint8_t)(0x04 | ((vd & 7) << 3))); IB(0x88); }
        return true;

        broken_vreg:
        rt_error(ctx->env, BROKEN_BYTECODE);
        return false;
}

/* Visit an OP_VSTOREF32X4 instruction. */
static INLINE bool
jit_visit_vstoref32x4_op(
        struct rt_jit_context *ctx)
{
        int base_tmp;
        int ofs_tmp;
        int vs;
        int inline_ok;
        int vreg_limit;
        uint32_t vbase;
        int lane;
        int base;
        int ofs;
        int cursor;

        CONSUME_TMPVAR(base_tmp);
        CONSUME_TMPVAR(ofs_tmp);
        CONSUME_IMM8(vs);

        /* Win64 keeps the memory-canonical direct scalar tier until xmm6/xmm7
           receive an explicitly tested save area.  SysV needs only SSE2;
           SSE4.1 selects shorter multiply/extract sequences below. */
        inline_ok = !IS_MSABI &&
                    (ctx->simd_caps & JIT_SIMD_CAP_SSE2) != 0;

        if (!inline_ok) {
                vbase = (uint32_t)offsetof(struct rt_env, vreg);
                base = base_tmp * (int)sizeof(struct rt_value);
                ofs = ofs_tmp * (int)sizeof(struct rt_value);
                ASM {
                        IB(0x49); IB(0x8b); IB(0x87); ID((uint32_t)(base + 8));
                        IB(0x49); IB(0x63); IB(0x8f); ID((uint32_t)(ofs + 8));
                }
                for (lane = 0; lane < 4; lane++) {
                        IB(0x41); IB(0x8b); IB(0x96);
                        ID(vbase + (uint32_t)vs * 16 + (uint32_t)lane * 4);
                        IB(0x89); IB(0x54); IB(0x88); IB((uint8_t)(lane * 4));
                }
                return true;
        }
        vreg_limit = ctx->vector_vreg_limit > 0 ? ctx->vector_vreg_limit : 13;

        /* SSE keeps xmm13/xmm14 scratch and xmm15 invariant.  AVX uses
         * non-destructive forms and admits logical xmm0..xmm14, reserving only
         * xmm15 as instruction-local scratch. */
        if (vs < 0 || vs >= vreg_limit) goto broken_vreg;

        base = base_tmp * (int)sizeof(struct rt_value);
        ofs = ofs_tmp * (int)sizeof(struct rt_value);
        cursor = -1;
        if (ctx->vector_hint_active) {
                if (ctx->vector_base_tmp[0] == base_tmp)
                        cursor = 0;
                else if (ctx->vector_base_tmp[1] == base_tmp)
                        cursor = 1;
        }
        if (cursor >= 0) {
                /* movdqu xmmC, (rbx|rsi,rdi,4) */
                ASM { IB(0xf3); }
                if ((vs & 8) != 0) { ASM { IB(0x44); } }
                ASM { IB(0x0f); IB(0x7f);
                      IB((uint8_t)(0x04 | ((vs & 7) << 3)));
                      IB((uint8_t)(cursor == 0 ? 0xbb : 0xbe)); }
                return true;
        }
        ASM {
                /* movq base+8(%r15) -> %rax */    IB(0x49); IB(0x8b); IB(0x87); ID((uint32_t)(base + 8));
                /* movslq ofs+8(%r15) -> %rcx */   IB(0x49); IB(0x63); IB(0x8f); ID((uint32_t)(ofs + 8));
                /* movdqu %xmmC -> (%rax,%rcx,4) */ IB(0xf3);
        }
        if ((vs & 8) != 0) { ASM { IB(0x44); } }
        ASM { IB(0x0f); IB(0x7f);
              IB((uint8_t)(0x04 | ((vs & 7) << 3))); IB(0x88); }
        return true;

        broken_vreg:
        rt_error(ctx->env, BROKEN_BYTECODE);
        return false;
}

/* Visit an OP_VSPLATF32 instruction. */
static INLINE bool
jit_visit_vsplatf32_op(
        struct rt_jit_context *ctx)
{
        int vd;
        int src_tmp;
        int inline_ok;
        int vreg_limit;
        uint32_t vbase;
        int lane;
        int src;

        CONSUME_IMM8(vd);
        CONSUME_TMPVAR(src_tmp);

        /* Win64 keeps the memory-canonical direct scalar tier until xmm6/xmm7
           receive an explicitly tested save area.  SysV needs only SSE2;
           SSE4.1 selects shorter multiply/extract sequences below. */
        inline_ok = !IS_MSABI &&
                    (ctx->simd_caps & JIT_SIMD_CAP_SSE2) != 0;

        if (!inline_ok) {
                vbase = (uint32_t)offsetof(struct rt_env, vreg);
                src = src_tmp * (int)sizeof(struct rt_value);
                ASM { IB(0x41); IB(0x8b); IB(0x87); ID((uint32_t)(src + 8)); }
                for (lane = 0; lane < 4; lane++) {
                        IB(0x41); IB(0x89); IB(0x86);
                        ID(vbase + (uint32_t)vd * 16 + (uint32_t)lane * 4);
                }
                return true;
        }
        vreg_limit = ctx->vector_vreg_limit > 0 ? ctx->vector_vreg_limit : 13;

        /* SSE keeps xmm13/xmm14 scratch and xmm15 invariant.  AVX uses
         * non-destructive forms and admits logical xmm0..xmm14, reserving only
         * xmm15 as instruction-local scratch. */
        if (vd < 0 || vd >= vreg_limit) goto broken_vreg;

        src = src_tmp * (int)sizeof(struct rt_value);
        ASM { IB(0x66); IB((uint8_t)((vd & 8) != 0 ? 0x45 : 0x41));
              IB(0x0f); IB(0x6e);
              IB((uint8_t)(0x87 | ((vd & 7) << 3)));
              ID((uint32_t)(src + 8)); }
        if (!jit_x86_64_put_sse_rr(ctx, 0x66, 1, 0x70, vd, vd))
                return false;
        ASM { IB(0x00); }
        return true;

        broken_vreg:
        rt_error(ctx->env, BROKEN_BYTECODE);
        return false;
}

/* Visit an OP_VGETLANEF32 instruction. */
static INLINE bool
jit_visit_vgetlanef32_op(
        struct rt_jit_context *ctx)
{
        int dst_tmp;
        int vs;
        int lane_index;
        int inline_ok;
        int vreg_limit;
        uint32_t vbase;
        int d;
        int dst;

        CONSUME_TMPVAR(dst_tmp);
        CONSUME_IMM8(vs);
        CONSUME_IMM8(lane_index);

        /* Win64 keeps the memory-canonical direct scalar tier until xmm6/xmm7
           receive an explicitly tested save area.  SysV needs only SSE2;
           SSE4.1 selects shorter multiply/extract sequences below. */
        inline_ok = !IS_MSABI &&
                    (ctx->simd_caps & JIT_SIMD_CAP_SSE2) != 0;

        if (!inline_ok) {
                vbase = (uint32_t)offsetof(struct rt_env, vreg);

                d = dst_tmp * (int)sizeof(struct rt_value);
                ASM {
                        IB(0x41); IB(0x8b); IB(0x86);
                        ID(vbase + (uint32_t)vs * 16 + (uint32_t)lane_index * 4);
                        IB(0x41); IB(0xc7); IB(0x87); ID((uint32_t)d);
                        ID((uint32_t)(NOCT_VALUE_FLOAT));
                        IB(0x41); IB(0x89); IB(0x87); ID((uint32_t)(d + 8));
                }
                return true;
        }
        vreg_limit = ctx->vector_vreg_limit > 0 ? ctx->vector_vreg_limit : 13;

        /* SSE keeps xmm13/xmm14 scratch and xmm15 invariant.  AVX uses
         * non-destructive forms and admits logical xmm0..xmm14, reserving only
         * xmm15 as instruction-local scratch. */
        if (vs < 0 || vs >= vreg_limit) goto broken_vreg;

        dst = dst_tmp * (int)sizeof(struct rt_value);
        if ((ctx->simd_caps & JIT_SIMD_CAP_SSE41) != 0) {
                ASM { IB(0x66);
                      IB((uint8_t)((vs & 8) != 0 ? 0x45 : 0x41));
                      IB(0x0f); IB(0x3a); IB(0x16);
                      IB((uint8_t)(0x87 | ((vs & 7) << 3)));
                      ID((uint32_t)(dst + 8)); IB((uint8_t)lane_index); }
        } else {
                ASM { IB(0x66); }
                if ((vs & 8) != 0) { ASM { IB(0x41); } }
                ASM {
                        /* SSE2: combine two pextrw results without changing xmmB. */
                        IB(0x0f); IB(0xc5); IB((uint8_t)(0xc0 | (vs & 7))); IB((uint8_t)(lane_index * 2));
                        IB(0x66);
                }
                if ((vs & 8) != 0) { ASM { IB(0x41); } }
                ASM {
                        IB(0x0f); IB(0xc5); IB((uint8_t)(0xc8 | (vs & 7))); IB((uint8_t)(lane_index * 2 + 1));
                        IB(0xc1); IB(0xe1); IB(0x10);
                        IB(0x09); IB(0xc8);
                        IB(0x41); IB(0x89); IB(0x87); ID((uint32_t)(dst + 8));
                }
        }
        ASM {
                /* Both i32 and f32 lanes are raw 32-bit payloads. */
                IB(0x41); IB(0xc7); IB(0x87); ID((uint32_t)dst);
                ID((uint32_t)(NOCT_VALUE_FLOAT));
        }
        return true;

        broken_vreg:
        rt_error(ctx->env, BROKEN_BYTECODE);
        return false;
}

/* Visit an OP_VADDF32X4 instruction. */
static INLINE bool
jit_visit_vaddf32x4_op(
        struct rt_jit_context *ctx)
{
        int vd;
        int lhs;
        int rhs;
        int inline_ok;
        int vreg_limit;
        uint32_t vbase;
        int lane;
        uint8_t opcode;

        CONSUME_IMM8(vd);
        CONSUME_IMM8(lhs);
        CONSUME_IMM8(rhs);

        /* Win64 keeps the memory-canonical direct scalar tier until xmm6/xmm7
           receive an explicitly tested save area.  SysV needs only SSE2;
           SSE4.1 selects shorter multiply/extract sequences below. */
        inline_ok = !IS_MSABI &&
                    (ctx->simd_caps & JIT_SIMD_CAP_SSE2) != 0;

        if (!inline_ok) {
                vbase = (uint32_t)offsetof(struct rt_env, vreg);
                for (lane = 0; lane < 4; lane++) {
                        uint32_t a;
                        uint32_t b;
                        uint32_t d;

                        a = vbase + (uint32_t)lhs * 16 + (uint32_t)lane * 4;
                        b = vbase + (uint32_t)rhs * 16 + (uint32_t)lane * 4;
                        d = vbase + (uint32_t)vd * 16 + (uint32_t)lane * 4;
                        IB(0xf3); IB(0x41); IB(0x0f); IB(0x10); IB(0x86); ID(a);
                        IB(0xf3); IB(0x41); IB(0x0f);
                        IB(0x58);
                        IB(0x86); ID(b);
                        IB(0xf3); IB(0x41); IB(0x0f); IB(0x11); IB(0x86); ID(d);
                }
                return true;
        }
        vreg_limit = ctx->vector_vreg_limit > 0 ? ctx->vector_vreg_limit : 13;

        /* SSE keeps xmm13/xmm14 scratch and xmm15 invariant.  AVX uses
         * non-destructive forms and admits logical xmm0..xmm14, reserving only
         * xmm15 as instruction-local scratch. */
        if (vd < 0 || vd >= vreg_limit || lhs < 0 || lhs >= vreg_limit ||
            rhs < 0 || rhs >= vreg_limit)
                goto broken_vreg;

        opcode = 0x58;
        if ((ctx->simd_caps & JIT_SIMD_CAP_AVX) != 0) {
                if (!jit_x86_64_put_vex_rrr(ctx, 1, 0, opcode,
                                              vd, lhs, rhs))
                        return false;
        } else {
                if (vd != lhs && !jit_x86_64_put_sse_rr(ctx, 0x66, 1,
                                                     0x6f, vd, lhs))
                        return false;
                if (!jit_x86_64_put_sse_rr(ctx, 0, 1, opcode, vd, rhs))
                        return false;
        }
        return true;

        broken_vreg:
        rt_error(ctx->env, BROKEN_BYTECODE);
        return false;
}

/* Visit an OP_VSUBF32X4 instruction. */
static INLINE bool
jit_visit_vsubf32x4_op(
        struct rt_jit_context *ctx)
{
        int vd;
        int lhs;
        int rhs;
        int inline_ok;
        int vreg_limit;
        uint32_t vbase;
        int lane;
        uint8_t opcode;

        CONSUME_IMM8(vd);
        CONSUME_IMM8(lhs);
        CONSUME_IMM8(rhs);

        /* Win64 keeps the memory-canonical direct scalar tier until xmm6/xmm7
           receive an explicitly tested save area.  SysV needs only SSE2;
           SSE4.1 selects shorter multiply/extract sequences below. */
        inline_ok = !IS_MSABI &&
                    (ctx->simd_caps & JIT_SIMD_CAP_SSE2) != 0;

        if (!inline_ok) {
                vbase = (uint32_t)offsetof(struct rt_env, vreg);
                for (lane = 0; lane < 4; lane++) {
                        uint32_t a;
                        uint32_t b;
                        uint32_t d;

                        a = vbase + (uint32_t)lhs * 16 + (uint32_t)lane * 4;
                        b = vbase + (uint32_t)rhs * 16 + (uint32_t)lane * 4;
                        d = vbase + (uint32_t)vd * 16 + (uint32_t)lane * 4;
                        IB(0xf3); IB(0x41); IB(0x0f); IB(0x10); IB(0x86); ID(a);
                        IB(0xf3); IB(0x41); IB(0x0f);
                        IB(0x5c);
                        IB(0x86); ID(b);
                        IB(0xf3); IB(0x41); IB(0x0f); IB(0x11); IB(0x86); ID(d);
                }
                return true;
        }
        vreg_limit = ctx->vector_vreg_limit > 0 ? ctx->vector_vreg_limit : 13;

        /* SSE keeps xmm13/xmm14 scratch and xmm15 invariant.  AVX uses
         * non-destructive forms and admits logical xmm0..xmm14, reserving only
         * xmm15 as instruction-local scratch. */
        if (vd < 0 || vd >= vreg_limit || lhs < 0 || lhs >= vreg_limit ||
            rhs < 0 || rhs >= vreg_limit)
                goto broken_vreg;

        opcode = 0x5c;
        if ((ctx->simd_caps & JIT_SIMD_CAP_AVX) != 0) {
                if (!jit_x86_64_put_vex_rrr(ctx, 1, 0, opcode,
                                              vd, lhs, rhs))
                        return false;
        } else {
                if (vd != lhs && !jit_x86_64_put_sse_rr(ctx, 0x66, 1,
                                                     0x6f, vd, lhs))
                        return false;
                if (!jit_x86_64_put_sse_rr(ctx, 0, 1, opcode, vd, rhs))
                        return false;
        }
        return true;

        broken_vreg:
        rt_error(ctx->env, BROKEN_BYTECODE);
        return false;
}

/* Visit an OP_VMULF32X4 instruction. */
static INLINE bool
jit_visit_vmulf32x4_op(
        struct rt_jit_context *ctx)
{
        int vd;
        int lhs;
        int rhs;
        int inline_ok;
        int vreg_limit;
        uint32_t vbase;
        int lane;
        uint8_t opcode;

        CONSUME_IMM8(vd);
        CONSUME_IMM8(lhs);
        CONSUME_IMM8(rhs);

        /* Win64 keeps the memory-canonical direct scalar tier until xmm6/xmm7
           receive an explicitly tested save area.  SysV needs only SSE2;
           SSE4.1 selects shorter multiply/extract sequences below. */
        inline_ok = !IS_MSABI &&
                    (ctx->simd_caps & JIT_SIMD_CAP_SSE2) != 0;

        if (!inline_ok) {
                vbase = (uint32_t)offsetof(struct rt_env, vreg);
                for (lane = 0; lane < 4; lane++) {
                        uint32_t a;
                        uint32_t b;
                        uint32_t d;

                        a = vbase + (uint32_t)lhs * 16 + (uint32_t)lane * 4;
                        b = vbase + (uint32_t)rhs * 16 + (uint32_t)lane * 4;
                        d = vbase + (uint32_t)vd * 16 + (uint32_t)lane * 4;
                        IB(0xf3); IB(0x41); IB(0x0f); IB(0x10); IB(0x86); ID(a);
                        IB(0xf3); IB(0x41); IB(0x0f);
                        IB(0x59);
                        IB(0x86); ID(b);
                        IB(0xf3); IB(0x41); IB(0x0f); IB(0x11); IB(0x86); ID(d);
                }
                return true;
        }
        vreg_limit = ctx->vector_vreg_limit > 0 ? ctx->vector_vreg_limit : 13;

        /* SSE keeps xmm13/xmm14 scratch and xmm15 invariant.  AVX uses
         * non-destructive forms and admits logical xmm0..xmm14, reserving only
         * xmm15 as instruction-local scratch. */
        if (vd < 0 || vd >= vreg_limit || lhs < 0 || lhs >= vreg_limit ||
            rhs < 0 || rhs >= vreg_limit)
                goto broken_vreg;

        opcode = 0x59;
        if ((ctx->simd_caps & JIT_SIMD_CAP_AVX) != 0) {
                if (!jit_x86_64_put_vex_rrr(ctx, 1, 0, opcode,
                                              vd, lhs, rhs))
                        return false;
        } else {
                if (vd != lhs && !jit_x86_64_put_sse_rr(ctx, 0x66, 1,
                                                     0x6f, vd, lhs))
                        return false;
                if (!jit_x86_64_put_sse_rr(ctx, 0, 1, opcode, vd, rhs))
                        return false;
        }
        return true;

        broken_vreg:
        rt_error(ctx->env, BROKEN_BYTECODE);
        return false;
}

/* Visit an OP_VDIVF32X4 instruction. */
static INLINE bool
jit_visit_vdivf32x4_op(
        struct rt_jit_context *ctx)
{
        int vd;
        int lhs;
        int rhs;
        int inline_ok;
        int vreg_limit;
        uint32_t vbase;
        int lane;
        uint8_t opcode;

        CONSUME_IMM8(vd);
        CONSUME_IMM8(lhs);
        CONSUME_IMM8(rhs);

        /* Win64 keeps the memory-canonical direct scalar tier until xmm6/xmm7
           receive an explicitly tested save area.  SysV needs only SSE2;
           SSE4.1 selects shorter multiply/extract sequences below. */
        inline_ok = !IS_MSABI &&
                    (ctx->simd_caps & JIT_SIMD_CAP_SSE2) != 0;

        if (!inline_ok) {
                vbase = (uint32_t)offsetof(struct rt_env, vreg);
                for (lane = 0; lane < 4; lane++) {
                        uint32_t a;
                        uint32_t b;
                        uint32_t d;

                        a = vbase + (uint32_t)lhs * 16 + (uint32_t)lane * 4;
                        b = vbase + (uint32_t)rhs * 16 + (uint32_t)lane * 4;
                        d = vbase + (uint32_t)vd * 16 + (uint32_t)lane * 4;
                        IB(0xf3); IB(0x41); IB(0x0f); IB(0x10); IB(0x86); ID(a);
                        IB(0xf3); IB(0x41); IB(0x0f);
                        IB(0x5e);
                        IB(0x86); ID(b);
                        IB(0xf3); IB(0x41); IB(0x0f); IB(0x11); IB(0x86); ID(d);
                }
                return true;
        }
        vreg_limit = ctx->vector_vreg_limit > 0 ? ctx->vector_vreg_limit : 13;

        /* SSE keeps xmm13/xmm14 scratch and xmm15 invariant.  AVX uses
         * non-destructive forms and admits logical xmm0..xmm14, reserving only
         * xmm15 as instruction-local scratch. */
        if (vd < 0 || vd >= vreg_limit || lhs < 0 || lhs >= vreg_limit ||
            rhs < 0 || rhs >= vreg_limit)
                goto broken_vreg;

        opcode = 0x5e;
        if ((ctx->simd_caps & JIT_SIMD_CAP_AVX) != 0) {
                if (!jit_x86_64_put_vex_rrr(ctx, 1, 0, opcode,
                                              vd, lhs, rhs))
                        return false;
        } else {
                if (vd != lhs && !jit_x86_64_put_sse_rr(ctx, 0x66, 1,
                                                     0x6f, vd, lhs))
                        return false;
                if (!jit_x86_64_put_sse_rr(ctx, 0, 1, opcode, vd, rhs))
                        return false;
        }
        return true;

        broken_vreg:
        rt_error(ctx->env, BROKEN_BYTECODE);
        return false;
}

/* Visit an OP_VCVTI32F32X4 instruction. */
static INLINE bool
jit_visit_vcvti32f32x4_op(
        struct rt_jit_context *ctx)
{
        int vd;
        int vs;
        int inline_ok;
        int vreg_limit;
        uint32_t vbase;
        int lane;

        CONSUME_IMM8(vd);
        CONSUME_IMM8(vs);

        /* Win64 keeps the memory-canonical direct scalar tier until xmm6/xmm7
           receive an explicitly tested save area.  SysV needs only SSE2;
           SSE4.1 selects shorter multiply/extract sequences below. */
        inline_ok = !IS_MSABI &&
                    (ctx->simd_caps & JIT_SIMD_CAP_SSE2) != 0;

        if (!inline_ok) {
                vbase = (uint32_t)offsetof(struct rt_env, vreg);
                for (lane = 0; lane < 4; lane++) {
                        uint32_t s;
                        uint32_t d;

                        s = vbase + (uint32_t)vs * 16 +
                                (uint32_t)lane * 4;
                        d = vbase + (uint32_t)vd * 16 +
                                (uint32_t)lane * 4;
                        /* cvtsi2ssl s(%r14), xmm0; movss xmm0,d(%r14) */
                        IB(0xf3); IB(0x41); IB(0x0f); IB(0x2a); IB(0x86); ID(s);
                        IB(0xf3); IB(0x41); IB(0x0f); IB(0x11); IB(0x86); ID(d);
                }
                return true;
        }
        vreg_limit = ctx->vector_vreg_limit > 0 ? ctx->vector_vreg_limit : 13;

        /* SSE keeps xmm13/xmm14 scratch and xmm15 invariant.  AVX uses
         * non-destructive forms and admits logical xmm0..xmm14, reserving only
         * xmm15 as instruction-local scratch. */
        if (vd < 0 || vd >= vreg_limit || vs < 0 || vs >= vreg_limit)
                goto broken_vreg;

        if ((ctx->simd_caps & JIT_SIMD_CAP_AVX) != 0) {
                if (!jit_x86_64_put_vex_rr(ctx, 1, 0, 0x5b, vd, vs))
                        return false;
        } else if (!jit_x86_64_put_sse_rr(ctx, 0, 1, 0x5b, vd, vs)) {
                return false;
        }

        return true;

        broken_vreg:
        rt_error(ctx->env, BROKEN_BYTECODE);
        return false;
}

/* Visit an OP_VCVTF32I32X4 instruction. */
static INLINE bool
jit_visit_vcvtf32i32x4_op(
        struct rt_jit_context *ctx)
{
        int vd;
        int vs;
        int inline_ok;
        int vreg_limit;
        uint32_t vbase;
        int lane;

        CONSUME_IMM8(vd);
        CONSUME_IMM8(vs);

        /* Win64 keeps the memory-canonical direct scalar tier until xmm6/xmm7
           receive an explicitly tested save area.  SysV needs only SSE2;
           SSE4.1 selects shorter multiply/extract sequences below. */
        inline_ok = !IS_MSABI &&
                    (ctx->simd_caps & JIT_SIMD_CAP_SSE2) != 0;

        if (!inline_ok) {
                vbase = (uint32_t)offsetof(struct rt_env, vreg);
                for (lane = 0; lane < 4; lane++) {
                        uint32_t s;
                        uint32_t d;

                        s = vbase + (uint32_t)vs * 16 +
                                (uint32_t)lane * 4;
                        d = vbase + (uint32_t)vd * 16 +
                                (uint32_t)lane * 4;
                        /* cvttss2si s(%r14),eax; mov eax,d(%r14) */
                        IB(0xf3); IB(0x41); IB(0x0f); IB(0x2c); IB(0x86); ID(s);
                        IB(0x41); IB(0x89); IB(0x86); ID(d);
                }
                return true;
        }
        vreg_limit = ctx->vector_vreg_limit > 0 ? ctx->vector_vreg_limit : 13;

        /* SSE keeps xmm13/xmm14 scratch and xmm15 invariant.  AVX uses
         * non-destructive forms and admits logical xmm0..xmm14, reserving only
         * xmm15 as instruction-local scratch. */
        if (vd < 0 || vd >= vreg_limit || vs < 0 || vs >= vreg_limit)
                goto broken_vreg;

        if ((ctx->simd_caps & JIT_SIMD_CAP_AVX) != 0) {
                if (!jit_x86_64_put_vex_rr(ctx, 1, 2, 0x5b, vd, vs))
                        return false;
        } else if (!jit_x86_64_put_sse_rr(ctx, 0xf3, 1, 0x5b,
                                                  vd, vs)) {
                return false;
        }

        return true;

        broken_vreg:
        rt_error(ctx->env, BROKEN_BYTECODE);
        return false;
}

/* Visit an OP_VMINS32X4 instruction. */
static INLINE bool
jit_visit_vmins32x4_op(
        struct rt_jit_context *ctx)
{
        int vd;
        int lhs;
        int rhs;
        int inline_ok;
        int vreg_limit;
        uint32_t vbase;
        int lane;

        CONSUME_IMM8(vd);
        CONSUME_IMM8(lhs);
        CONSUME_IMM8(rhs);

        /* Win64 keeps the memory-canonical direct scalar tier until xmm6/xmm7
           receive an explicitly tested save area.  SysV needs only SSE2;
           SSE4.1 selects shorter multiply/extract sequences below. */
        inline_ok = !IS_MSABI &&
                    (ctx->simd_caps & JIT_SIMD_CAP_SSE2) != 0;

        if (!inline_ok) {
                vbase = (uint32_t)offsetof(struct rt_env, vreg);
                for (lane = 0; lane < 4; lane++) {
                        uint32_t a;
                        uint32_t b;
                        uint32_t d;

                        a = vbase + (uint32_t)lhs * 16 + (uint32_t)lane * 4;
                        b = vbase + (uint32_t)rhs * 16 + (uint32_t)lane * 4;
                        d = vbase + (uint32_t)vd * 16 + (uint32_t)lane * 4;
                        IB(0x41); IB(0x8b); IB(0x86); ID(a);
                        /* cmp b,eax; signed min uses cmovg, max cmovl. */
                        IB(0x41); IB(0x3b); IB(0x86); ID(b);
                        IB(0x41); IB(0x0f);
                        IB((uint8_t)(0x4f));
                        IB(0x86); ID(b);
                        IB(0x41); IB(0x89); IB(0x86); ID(d);
                }
                return true;
        }
        vreg_limit = ctx->vector_vreg_limit > 0 ? ctx->vector_vreg_limit : 13;

        /* SSE keeps xmm13/xmm14 scratch and xmm15 invariant.  AVX uses
         * non-destructive forms and admits logical xmm0..xmm14, reserving only
         * xmm15 as instruction-local scratch. */
        if (vd < 0 || vd >= vreg_limit || lhs < 0 || lhs >= vreg_limit ||
            rhs < 0 || rhs >= vreg_limit)
                goto broken_vreg;

        if ((ctx->simd_caps & JIT_SIMD_CAP_AVX) != 0) {
                int map;
                uint8_t opcode;

                map = 2;
                opcode = 0x39;
                if ((ctx->simd_caps & JIT_SIMD_CAP_SSE41) != 0) {
                        if (!jit_x86_64_put_vex_rrr(ctx, map, 1, opcode,
                                                      vd, lhs, rhs))
                                return false;
                        return true;
                }
        }
        /* Legacy two-address lowering. */

        if (vd != lhs && !jit_x86_64_put_sse_rr(ctx, 0x66, 1, 0x6f,
                                                 vd, lhs))
                return false;
        if ((ctx->simd_caps & JIT_SIMD_CAP_SSE41) != 0) {
                if (!jit_x86_64_put_sse_rr(ctx, 0x66, 2,
                                0x39,
                                vd, rhs))
                        return false;
        } else {
                /* SSE2 signed min/max via (lhs>rhs) mask.  xmm13/xmm14
                 * are outside the logical map. */
                if (!jit_x86_64_put_sse_rr(ctx, 0x66, 1, 0x6f,
                                         13, lhs) ||
                    !jit_x86_64_put_sse_rr(ctx, 0x66, 1, 0x66,
                                         13, rhs) ||
                    !jit_x86_64_put_sse_rr(ctx, 0x66, 1, 0x6f,
                                         14, 13))
                        return false;
                if (!jit_x86_64_put_sse_rr(ctx, 0x66, 1,
                                         0xdb, 14, rhs) ||
                    !jit_x86_64_put_sse_rr(ctx, 0x66, 1,
                                         0xdf, 13, lhs))
                        return false;
                if (!jit_x86_64_put_sse_rr(ctx, 0x66, 1, 0xeb,
                                         14, 13) ||
                    (vd != 14 && !jit_x86_64_put_sse_rr(ctx, 0x66,
                                                      1, 0x6f, vd, 14)))
                        return false;
        }

        return true;

        broken_vreg:
        rt_error(ctx->env, BROKEN_BYTECODE);
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
        int inline_ok;
        int vreg_limit;
        uint32_t vbase;
        int lane;

        CONSUME_IMM8(vd);
        CONSUME_IMM8(lhs);
        CONSUME_IMM8(rhs);

        /* Win64 keeps the memory-canonical direct scalar tier until xmm6/xmm7
           receive an explicitly tested save area.  SysV needs only SSE2;
           SSE4.1 selects shorter multiply/extract sequences below. */
        inline_ok = !IS_MSABI &&
                    (ctx->simd_caps & JIT_SIMD_CAP_SSE2) != 0;

        if (!inline_ok) {
                vbase = (uint32_t)offsetof(struct rt_env, vreg);
                for (lane = 0; lane < 4; lane++) {
                        uint32_t a;
                        uint32_t b;
                        uint32_t d;

                        a = vbase + (uint32_t)lhs * 16 + (uint32_t)lane * 4;
                        b = vbase + (uint32_t)rhs * 16 + (uint32_t)lane * 4;
                        d = vbase + (uint32_t)vd * 16 + (uint32_t)lane * 4;
                        IB(0x41); IB(0x8b); IB(0x86); ID(a);
                        /* cmp b,eax; signed min uses cmovg, max cmovl. */
                        IB(0x41); IB(0x3b); IB(0x86); ID(b);
                        IB(0x41); IB(0x0f);
                        IB((uint8_t)(0x4c));
                        IB(0x86); ID(b);
                        IB(0x41); IB(0x89); IB(0x86); ID(d);
                }
                return true;
        }
        vreg_limit = ctx->vector_vreg_limit > 0 ? ctx->vector_vreg_limit : 13;

        /* SSE keeps xmm13/xmm14 scratch and xmm15 invariant.  AVX uses
         * non-destructive forms and admits logical xmm0..xmm14, reserving only
         * xmm15 as instruction-local scratch. */
        if (vd < 0 || vd >= vreg_limit || lhs < 0 || lhs >= vreg_limit ||
            rhs < 0 || rhs >= vreg_limit)
                goto broken_vreg;

        if ((ctx->simd_caps & JIT_SIMD_CAP_AVX) != 0) {
                int map;
                uint8_t opcode;

                map = 2;
                opcode = 0x3d;
                if ((ctx->simd_caps & JIT_SIMD_CAP_SSE41) != 0) {
                        if (!jit_x86_64_put_vex_rrr(ctx, map, 1, opcode,
                                                      vd, lhs, rhs))
                                return false;
                        return true;
                }
        }
        /* Legacy two-address lowering. */

        if (vd != lhs && !jit_x86_64_put_sse_rr(ctx, 0x66, 1, 0x6f,
                                                 vd, lhs))
                return false;
        if ((ctx->simd_caps & JIT_SIMD_CAP_SSE41) != 0) {
                if (!jit_x86_64_put_sse_rr(ctx, 0x66, 2,
                                0x3d,
                                vd, rhs))
                        return false;
        } else {
                /* SSE2 signed min/max via (lhs>rhs) mask.  xmm13/xmm14
                 * are outside the logical map. */
                if (!jit_x86_64_put_sse_rr(ctx, 0x66, 1, 0x6f,
                                         13, lhs) ||
                    !jit_x86_64_put_sse_rr(ctx, 0x66, 1, 0x66,
                                         13, rhs) ||
                    !jit_x86_64_put_sse_rr(ctx, 0x66, 1, 0x6f,
                                         14, 13))
                        return false;
                if (!jit_x86_64_put_sse_rr(ctx, 0x66, 1,
                                         0xdb, 14, lhs) ||
                    !jit_x86_64_put_sse_rr(ctx, 0x66, 1,
                                         0xdf, 13, rhs))
                        return false;
                if (!jit_x86_64_put_sse_rr(ctx, 0x66, 1, 0xeb,
                                         14, 13) ||
                    (vd != 14 && !jit_x86_64_put_sse_rr(ctx, 0x66,
                                                      1, 0x6f, vd, 14)))
                        return false;
        }

        return true;

        broken_vreg:
        rt_error(ctx->env, BROKEN_BYTECODE);
        return false;
}
#endif


/* Visit a bytecode of a function. */
static bool
jit_visit_bytecode(
        struct rt_jit_context *ctx)
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
			/* pushq %r12 */                        IB(0x41); IB(0x54);
                        /* pushq %r13 */                        IB(0x41); IB(0x55);
                        /* pushq %r14 */                        IB(0x41); IB(0x56);
                        /* pushq %r15 */                        IB(0x41); IB(0x57);

			/* align stack to 16 bytes */
			/* sub rsp, 8 */                        IB(0x48); IB(0x83); IB(0xec); IB(0x08);

                        /* r14 = env */
                        /* movq %rdi, %r14 */                   IB(0x49); IB(0x89); IB(0xfe);

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
			/* addq $8, %rsp (align back) */        IB(0x48); IB(0x83); IB(0xc4); IB(0x08);
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
			if (!jit_visit_vindex_hint_op(ctx)) return false;
			break;
		case OP_PLOOP_HINT:
			if (!jit_visit_x86_64_ploop_hint_op(ctx)) return false;
			break;
#endif
		case OP_TMPVAR_TYPE:
			if (!rt_jit_visit_tmpvar_type_op(ctx)) return false;
			break;
		case OP_MATERIALIZE_TYPE:
			if (!jit_visit_x86_64_materialize_type_op(ctx)) return false;
			break;
#if defined(NOCT_USE_OPTIMIZER)
		case OP_SUBJNZ:
			if (!jit_visit_subjnz_op(ctx)) return false;
			break;
		case OP_VORI32X4I:
			if (!jit_visit_vori32x4i_op(ctx)) return false;
			break;
		case OP_VFMAF32X4:
			if (!jit_visit_vfmaf32x4_op(ctx)) return false;
			break;
		case OP_VCMPI32X4:
			if (!jit_visit_vcmpi32x4_op(ctx)) return false;
			break;
		case OP_VCMPF32X4:
			if (!jit_visit_vcmpf32x4_op(ctx)) return false;
			break;
		case OP_VSELECT128:
			if (!jit_visit_vselect128_op(ctx)) return false;
			break;
		case OP_VMASKSTOREI32X4:
			if (!jit_visit_vmaskstorei32x4_op(ctx)) return false;
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
			/* addq $8, %rsp (align back) */ IB(0x48); IB(0x83); IB(0xc4); IB(0x08);
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
        uint32_t i;

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
