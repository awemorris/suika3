/* -*- coding: utf-8; tab-width: 8; indent-tabs-mode: t; -*- */

/*
 * Noct Programming Language
 * Copyright (c) 2025, 2026, Awe Morris
 */

/*
 * JIT (common): Just-In-Time native code generation
 */

#include <noct/c89compat.h>

#include "jit-x86.c"
#include "jit-x86_64.c"
#include "jit-arm32.c"
#include "jit-arm64.c"
#include "jit-mips32.c"
#include "jit-mips64.c"
#include "jit-ppc32.c"
#include "jit-ppc64.c"
#include "jit-riscv32.c"
#include "jit-riscv64.c"

/* Disable JIT on targets without an architecture implementation. */
#if defined(NOCT_USE_JIT)
#if !defined(NOCT_ARCH_X86) && !defined(NOCT_ARCH_X86_64) && \
    !defined(NOCT_ARCH_ARM32) && !defined(NOCT_ARCH_ARM64) && \
    !defined(NOCT_ARCH_MIPS32) && !defined(NOCT_ARCH_MIPS64) && \
    !defined(NOCT_ARCH_PPC32) && !defined(NOCT_ARCH_PPC64) && \
    !defined(NOCT_ARCH_RISCV32) && !defined(NOCT_ARCH_RISCV64)
#undef NOCT_USE_JIT
#endif
#endif

/*
 * Architecture Independent
 *
 * Scalar JIT support is independent of the optimizer.  Optimizer-owned
 * vector, packed-loop, and register-cache metadata is guarded separately.
 */
#if defined(NOCT_USE_JIT) && \
      (                               \
        defined(NOCT_ARCH_X86)     || \
        defined(NOCT_ARCH_X86_64)  || \
        defined(NOCT_ARCH_ARM32)   || \
        defined(NOCT_ARCH_ARM64)   || \
        defined(NOCT_ARCH_MIPS32)  || \
        defined(NOCT_ARCH_MIPS64)  || \
        defined(NOCT_ARCH_PPC32)   || \
        defined(NOCT_ARCH_PPC64)   || \
        defined(NOCT_ARCH_RISCV32) || \
        defined(NOCT_ARCH_RISCV64)    \
      )

#include "runtime.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#if defined(_WIN32)
#include <windows.h>		/* VirtualAlloc(), VirtualProtect(), VirtualFree() */
#elif defined(NOCT_TARGET_DOS)
#include <dos.h>
#include <i86.h>
#elif defined(NOCT_TARGET_POSIX)
#include <errno.h>		/* errno */
#include <sys/mman.h>		/* mmap(), mprotect(), munmap() */
#include <unistd.h>		/* sysconf() */
#endif

/* Forward declaration. */
static bool jit_debug_enabled(void);
static void jit_debug_memory(const char *operation, size_t size,bool success, unsigned long error);
static size_t jit_page_size(void);
static size_t jit_align_up(size_t value, size_t alignment);
static bool jit_slab_allocate(struct rt_env *env, size_t requested_size, struct rt_jit_slab **result);
#if defined(NOCT_USE_OPTIMIZER)
static uint32_t jit_apply_simd_max(uint32_t detected);
#endif

/* Architecture-neutral JIT helpers. */
size_t
rt_jit_get_code_size(
	struct rt_env *env)
{
	size_t size;

	size = env->vm->config.jit_code_size;

	if (size == 0 || size > JIT_CODE_MAX)
		size = JIT_CODE_MAX;

	return size;
}

#if defined(NOCT_USE_OPTIMIZER) && \
    (defined(NOCT_ARCH_X86_64) || defined(NOCT_ARCH_ARM64))
uint16_t
rt_jit_ploop_read_u16(const uint8_t *p)
{
	return (uint16_t)(((uint16_t)p[0] << 8) | p[1]);
}

uint32_t
rt_jit_ploop_read_u32(const uint8_t *p)
{
	return ((uint32_t)p[0] << 24) |
		((uint32_t)p[1] << 16) |
		((uint32_t)p[2] << 8) |
		(uint32_t)p[3];
}

bool
rt_jit_ploop_reject(struct rt_jit_context *ctx, const char *reason)
{
	ctx->packed_loop_reject_reason = reason;

	return false;
}

bool
rt_jit_ploop_add_base(struct rt_jit_context *ctx, uint16_t base, int scale)
{
	int i;

	for (i = 0; i < 3; i++) {
		if (ctx->packed_loop_base_tmp[i] == (int)base) {
			return ctx->packed_loop_base_scale[i] == scale ?
				true :
				rt_jit_ploop_reject(ctx, "mixed-base-scale");
		}

		if (ctx->packed_loop_base_tmp[i] < 0) {
			ctx->packed_loop_base_tmp[i] = (int)base;
			ctx->packed_loop_base_scale[i] = scale;
			return true;
		}
	}

	return rt_jit_ploop_reject(ctx, "too-many-bases");
}

bool
rt_jit_ploop_is_index_alias(struct rt_jit_context *ctx, int tmp)
{
	int i;

	for (i = 0; i < ctx->packed_loop_index_alias_count; i++) {
		if (ctx->packed_loop_index_alias[i] == (uint16_t)tmp)
			return true;
	}

	return false;
}

bool
rt_jit_ploop_index_alias_disp(struct rt_jit_context *ctx, int tmp, int32_t *disp)
{
	int i;

	for (i = 0; i < ctx->packed_loop_index_alias_count; i++) {
		if (ctx->packed_loop_index_alias[i] == (uint16_t)tmp) {
			*disp = ctx->packed_loop_index_alias_disp[i];
			return true;
		}
	}

	return false;
}

void
rt_jit_ploop_remove_index_alias(struct rt_jit_context *ctx, int tmp)
{
	int i;

	for (i = 1; i < ctx->packed_loop_index_alias_count; i++) {
		if (ctx->packed_loop_index_alias[i] == (uint16_t)tmp) {
			ctx->packed_loop_index_alias[i] = ctx->packed_loop_index_alias[--ctx->packed_loop_index_alias_count];
			ctx->packed_loop_index_alias_disp[i] = ctx->packed_loop_index_alias_disp[ctx->packed_loop_index_alias_count];
			return;
		}
	}
}

bool
rt_jit_ploop_add_index_alias_disp(struct rt_jit_context *ctx, int tmp,
			       int32_t disp)
{
	int i;

	for (i = 0; i < ctx->packed_loop_index_alias_count; i++) {
		if (ctx->packed_loop_index_alias[i] == (uint16_t)tmp) {
			ctx->packed_loop_index_alias_disp[i] = disp;
			return true;
		}
	}

	if (ctx->packed_loop_index_alias_count >=
	    (int)(sizeof(ctx->packed_loop_index_alias) /
		  sizeof(ctx->packed_loop_index_alias[0]))) {
		return rt_jit_ploop_reject(ctx, "index-alias-overflow");
	}

	ctx->packed_loop_index_alias[ctx->packed_loop_index_alias_count] = (uint16_t)tmp;
	ctx->packed_loop_index_alias_disp[ctx->packed_loop_index_alias_count] = disp;
	ctx->packed_loop_index_alias_count++;

	return true;
}

bool
rt_jit_ploop_add_index_alias(struct rt_jit_context *ctx, int tmp)
{
	return rt_jit_ploop_add_index_alias_disp(ctx, tmp, 0);
}

int
rt_jit_ploop_resolve_base(struct rt_jit_context *ctx, int tmp)
{
	int i;

	for (i = ctx->packed_loop_base_alias_count - 1; i >= 0; i--) {
		if (ctx->packed_loop_base_alias_tmp[i] == (uint16_t)tmp)
			return ctx->packed_loop_base_alias_root[i];
	}
	return tmp;
}

void
rt_jit_ploop_remove_base_alias(struct rt_jit_context *ctx, int tmp)
{
	int i;

	for (i = 0; i < ctx->packed_loop_base_alias_count; i++) {
		if (ctx->packed_loop_base_alias_tmp[i] == (uint16_t)tmp) {
			ctx->packed_loop_base_alias_tmp[i] = ctx->packed_loop_base_alias_tmp[--ctx->packed_loop_base_alias_count];
			ctx->packed_loop_base_alias_root[i] = ctx->packed_loop_base_alias_root[ctx->packed_loop_base_alias_count];
			return;
		}
	}
}

bool
rt_jit_ploop_set_base_alias(struct rt_jit_context *ctx, int dst, int src)
{
	int root;

	root = rt_jit_ploop_resolve_base(ctx, src);
	rt_jit_ploop_remove_base_alias(ctx, dst);

	if (ctx->packed_loop_base_alias_count >=
	    (int)(sizeof(ctx->packed_loop_base_alias_tmp) /
		  sizeof(ctx->packed_loop_base_alias_tmp[0]))) {
		return rt_jit_ploop_reject(ctx, "base-alias-overflow");
	}

	ctx->packed_loop_base_alias_tmp[ctx->packed_loop_base_alias_count] = (uint16_t)dst;
	ctx->packed_loop_base_alias_root[ctx->packed_loop_base_alias_count] = (uint16_t)root;
	ctx->packed_loop_base_alias_count++;

	return true;
}

void
rt_jit_ploop_note_use(struct rt_jit_context *ctx, int tmp)
{
	/*
	 * gpr_tmp_dirty/range_valid are scratch bitsets during the grammar scan.
	 * They are reset by the backend before register allocation starts.
	 */
	if (tmp >= 0 && (uint32_t)tmp < ctx->func->tmpvar_size &&
	    ctx->gpr_tmp_dirty[tmp] == 0)
		ctx->gpr_range_valid[tmp] = 1;
}

void
rt_jit_ploop_note_def(struct rt_jit_context *ctx, int tmp)
{
	if (tmp >= 0 && (uint32_t)tmp < ctx->func->tmpvar_size)
		ctx->gpr_tmp_dirty[tmp] = 1;
}

bool
rt_jit_ploop_has_loop_carried_scalar(struct rt_jit_context *ctx)
{
	uint32_t i;

	for (i = 0; i < ctx->func->tmpvar_size; i++) {
		if (ctx->gpr_tmp_dirty[i] != 0 &&
		    ctx->gpr_range_valid[i] != 0)
			return true;
	}

	return false;
}

void
rt_jit_ploop_count_use(struct rt_jit_context *ctx, int tmp, bool address_only)
{
	uint32_t def;

	if (tmp < 0 || (uint32_t)tmp >= ctx->func->tmpvar_size)
		return;

	def = ctx->packed_def_lpc[tmp];
	if (def == UINT32_MAX || def >= ctx->func->bytecode_size)
		return;

	ctx->packed_lpc_use_count[def]++;

	if (address_only)
		ctx->packed_lpc_address_use_count[def]++;
}

bool
rt_jit_scan_packed_loop(struct rt_jit_context *ctx, bool reject_loop_carried)
{
	uint32_t p;
	uint32_t body_lpc;
	uint32_t size;
	uint16_t base;
	uint16_t ofs;
	uint16_t dst;
	uint16_t src1;
	uint16_t src2;
	uint16_t value;
	uint8_t op;
	int scale;
	int inc_count;
	uint32_t i;
	bool address_expr;

	ctx->packed_loop_reject_reason = "none";

	if (ctx->gpr_tmp_dirty == NULL || ctx->gpr_range_valid == NULL)
		return rt_jit_ploop_reject(ctx, "analysis-storage");

	for (i = 0; i < ctx->func->tmpvar_size; i++) {
		ctx->gpr_tmp_dirty[i] = 0;
		ctx->gpr_range_valid[i] = 0;
	}

	ctx->packed_loop_base_tmp[0] = -1;
	ctx->packed_loop_base_tmp[1] = -1;
	ctx->packed_loop_base_tmp[2] = -1;
	ctx->packed_loop_base_scale[0] = 0;
	ctx->packed_loop_base_scale[1] = 0;
	ctx->packed_loop_base_scale[2] = 0;
	ctx->packed_loop_index_alias_count = 1;
	ctx->packed_loop_index_alias[0] = (uint16_t)ctx->packed_loop_index_tmp;
	ctx->packed_loop_index_alias_disp[0] = 0;
	ctx->packed_loop_base_alias_count = 0;

	memset(ctx->packed_index_valid, 0, ctx->func->tmpvar_size);
	memset(ctx->packed_const_valid, 0, ctx->func->tmpvar_size);
	memset(ctx->packed_access_valid, 0, ctx->func->bytecode_size);
	memset(ctx->packed_elide_lpc, 0, ctx->func->bytecode_size);
	memset(ctx->packed_lpc_use_count, 0, ctx->func->bytecode_size * sizeof(*ctx->packed_lpc_use_count));
	memset(ctx->packed_lpc_address_use_count, 0, ctx->func->bytecode_size * sizeof(*ctx->packed_lpc_address_use_count));

	for (i = 0; i < ctx->func->tmpvar_size; i++)
		ctx->packed_def_lpc[i] = UINT32_MAX;

	ctx->packed_index_valid[ctx->packed_loop_index_tmp] = 1;
	ctx->packed_index_disp[ctx->packed_loop_index_tmp] = 0;

	body_lpc = ctx->lpc;
	p = body_lpc;
	inc_count = 0;
	while (p < ctx->func->bytecode_size) {
		op = ctx->func->bytecode[p];
		size = 0;
		base = 0xffffu;
		scale = 0;
		switch (op) {
		case OP_LINEINFO:
			if (p + 5 > ctx->func->bytecode_size)
				return rt_jit_ploop_reject(ctx, "malformed-region");
			size = 5;
			break;
		case OP_MATERIALIZE_TYPE:
			/*
			 * A fixed-type tag materialization does not
			 * read or change the cached payload. Keep it
			 * in the packed-loop region.
			 */
			if (p + 4 > ctx->func->bytecode_size)
				return rt_jit_ploop_reject(ctx, "malformed-region");

			dst = rt_jit_ploop_read_u16(&ctx->func->bytecode[p + 1]);
			if (dst >= ctx->func->tmpvar_size ||
			    (ctx->func->bytecode[p + 3] != NOCT_VALUE_INT &&
			     ctx->func->bytecode[p + 3] != NOCT_VALUE_LONG &&
			     ctx->func->bytecode[p + 3] != NOCT_VALUE_FLOAT &&
			     ctx->func->bytecode[p + 3] != NOCT_VALUE_DOUBLE))
				return rt_jit_ploop_reject(ctx, "malformed-region");
			size = 4;
			break;
		case OP_ASSIGN:
			if (p + 5 > ctx->func->bytecode_size)
				return rt_jit_ploop_reject(ctx, "malformed-region");
			size = 5;
			dst = rt_jit_ploop_read_u16(&ctx->func->bytecode[p + 1]);
			src1 = rt_jit_ploop_read_u16(&ctx->func->bytecode[p + 3]);
			rt_jit_ploop_note_use(ctx, src1);
			if (dst == (uint16_t)ctx->packed_loop_index_tmp ||
			    dst == (uint16_t)ctx->packed_loop_remaining_tmp)
				return rt_jit_ploop_reject(ctx, "index-escape");
			if (ctx->packed_index_valid[src1]) {
				ctx->packed_index_valid[dst] = 1;
				ctx->packed_index_disp[dst] = ctx->packed_index_disp[src1];

				if (!rt_jit_ploop_add_index_alias_disp(ctx, dst,
						ctx->packed_index_disp[dst]))
					return false;

				ctx->packed_elide_lpc[p] = 1;
				rt_jit_ploop_count_use(ctx, src1, true);
			} else {
				rt_jit_ploop_count_use(ctx, src1, false);
				ctx->packed_index_valid[dst] = 0;
				rt_jit_ploop_remove_index_alias(ctx, dst);
			}
			ctx->packed_const_valid[dst] = ctx->packed_const_valid[src1];
			if (ctx->packed_const_valid[src1])
				ctx->packed_const_value[dst] = ctx->packed_const_value[src1];
			if (!rt_jit_ploop_set_base_alias(ctx, dst, src1))
				return false;
			ctx->packed_def_lpc[dst] = p;
			rt_jit_ploop_note_def(ctx, dst);
			break;
		case OP_ICONST:
			if (p + 7 > ctx->func->bytecode_size)
				return rt_jit_ploop_reject(ctx, "malformed-region");
			size = 7;
			dst = rt_jit_ploop_read_u16(&ctx->func->bytecode[p + 1]);
			ctx->packed_def_lpc[dst] = p;
			if (dst == (uint16_t)ctx->packed_loop_index_tmp ||
			    dst == (uint16_t)ctx->packed_loop_remaining_tmp)
				return rt_jit_ploop_reject(ctx, "index-escape");
			rt_jit_ploop_remove_index_alias(ctx, dst);
			ctx->packed_index_valid[dst] = 0;
			ctx->packed_const_valid[dst] = 1;
			ctx->packed_const_value[dst] = (int32_t)
				rt_jit_ploop_read_u32(&ctx->func->bytecode[p + 3]);
			rt_jit_ploop_remove_base_alias(ctx, dst);
			rt_jit_ploop_note_def(ctx, dst);
			break;
		case OP_PLOAD8U:
		case OP_PLOAD8S:
		case OP_PLOAD16U:
		case OP_PLOAD16S:
		case OP_PLOAD32:
			if (p + 7 > ctx->func->bytecode_size)
				return rt_jit_ploop_reject(ctx, "malformed-region");
			size = 7;
			base = (uint16_t)rt_jit_ploop_resolve_base(ctx,
				rt_jit_ploop_read_u16(&ctx->func->bytecode[p + 3]));
			ofs = rt_jit_ploop_read_u16(&ctx->func->bytecode[p + 5]);
			if (!ctx->packed_index_valid[ofs])
				return rt_jit_ploop_reject(ctx, "index-escape");
			ctx->packed_access_valid[p] = 1;
			ctx->packed_access_disp[p] = ctx->packed_index_disp[ofs];
			rt_jit_ploop_count_use(ctx, ofs, true);
			dst = rt_jit_ploop_read_u16(&ctx->func->bytecode[p + 1]);
			rt_jit_ploop_remove_index_alias(ctx, dst);
			ctx->packed_index_valid[dst] = 0;
			ctx->packed_const_valid[dst] = 0;
			rt_jit_ploop_remove_base_alias(ctx, dst);
			ctx->packed_def_lpc[dst] = p;
			rt_jit_ploop_note_def(ctx, dst);
			scale = op == OP_PLOAD32 ? 4 :
				op == OP_PLOAD16U || op == OP_PLOAD16S ? 2 : 1;
			break;
		case OP_PSTORE8:
		case OP_PSTORE16:
		case OP_PSTORE32:
			if (p + 7 > ctx->func->bytecode_size)
				return rt_jit_ploop_reject(ctx, "malformed-region");
			size = 7;
			base = (uint16_t)rt_jit_ploop_resolve_base(ctx,
				rt_jit_ploop_read_u16(&ctx->func->bytecode[p + 1]));
			ofs = rt_jit_ploop_read_u16(&ctx->func->bytecode[p + 3]);
			if (!ctx->packed_index_valid[ofs])
				return rt_jit_ploop_reject(ctx, "index-escape");
			ctx->packed_access_valid[p] = 1;
			ctx->packed_access_disp[p] = ctx->packed_index_disp[ofs];
			rt_jit_ploop_count_use(ctx, ofs, true);
			src1 = rt_jit_ploop_read_u16(&ctx->func->bytecode[p + 5]);
			if (rt_jit_ploop_is_index_alias(ctx, src1))
				return rt_jit_ploop_reject(ctx, "index-escape");
			rt_jit_ploop_note_use(ctx, src1);
			rt_jit_ploop_count_use(ctx, src1, false);
			scale = op == OP_PSTORE32 ? 4 :
				op == OP_PSTORE16 ? 2 : 1;
			break;
		case OP_IADD:
		case OP_ISUB:
		case OP_IMUL:
		case OP_IDIV:
		case OP_IMOD:
		case OP_IAND:
		case OP_IOR:
		case OP_IXOR:
		case OP_ILT:
		case OP_ILTE:
		case OP_IGT:
		case OP_IGTE:
		case OP_IDIV_CHECKED:
		case OP_IMOD_CHECKED:
			if (p + 7 > ctx->func->bytecode_size)
				return rt_jit_ploop_reject(ctx, "malformed-region");
			size = 7;
			dst = rt_jit_ploop_read_u16(&ctx->func->bytecode[p + 1]);
			src1 = rt_jit_ploop_read_u16(&ctx->func->bytecode[p + 3]);
			src2 = rt_jit_ploop_read_u16(&ctx->func->bytecode[p + 5]);
			address_expr = false;
			rt_jit_ploop_note_use(ctx, src1);
			rt_jit_ploop_note_use(ctx, src2);
			if (dst == (uint16_t)ctx->packed_loop_index_tmp ||
			    dst == (uint16_t)ctx->packed_loop_remaining_tmp)
				return rt_jit_ploop_reject(ctx, "index-escape");
			ctx->packed_index_valid[dst] = 0;
			ctx->packed_const_valid[dst] = 0;
			if ((op == OP_IADD || op == OP_ISUB) &&
			    ctx->packed_index_valid[src1] &&
			    ctx->packed_const_valid[src2]) {
				int64_t d = ctx->packed_index_disp[src1];

				d += op == OP_IADD ?
					ctx->packed_const_value[src2] :
					-ctx->packed_const_value[src2];
				if (d < INT32_MIN || d > INT32_MAX)
					return rt_jit_ploop_reject(ctx,
						"index-displacement-overflow");
				ctx->packed_index_valid[dst] = 1;
				ctx->packed_index_disp[dst] = (int32_t)d;
				if (!rt_jit_ploop_add_index_alias_disp(ctx, dst,
						(int32_t)d))
					return false;
				ctx->packed_elide_lpc[p] = 1;
				address_expr = true;
			} else if (op == OP_IADD &&
				   ctx->packed_const_valid[src1] &&
				   ctx->packed_index_valid[src2]) {
				int64_t d = (int64_t)ctx->packed_const_value[src1] +
					ctx->packed_index_disp[src2];

				if (d < INT32_MIN || d > INT32_MAX)
					return rt_jit_ploop_reject(ctx,
						"index-displacement-overflow");
				ctx->packed_index_valid[dst] = 1;
				ctx->packed_index_disp[dst] = (int32_t)d;
				if (!rt_jit_ploop_add_index_alias_disp(ctx, dst,
						(int32_t)d))
					return false;
				ctx->packed_elide_lpc[p] = 1;
				address_expr = true;
			} else if (ctx->packed_index_valid[src1] ||
				   ctx->packed_index_valid[src2]) {
				return rt_jit_ploop_reject(ctx, "index-escape");
			}
			rt_jit_ploop_remove_index_alias(ctx, dst);
			if (ctx->packed_index_valid[dst] &&
			    !rt_jit_ploop_add_index_alias_disp(ctx, dst,
					ctx->packed_index_disp[dst]))
				return false;
			rt_jit_ploop_remove_base_alias(ctx, dst);
			rt_jit_ploop_count_use(ctx, src1, address_expr);
			rt_jit_ploop_count_use(ctx, src2, address_expr);
			ctx->packed_def_lpc[dst] = p;
			rt_jit_ploop_note_def(ctx, dst);
			break;
		case OP_ISHL:
		case OP_ISHR:
			if (p + 6 > ctx->func->bytecode_size)
				return rt_jit_ploop_reject(ctx, "malformed-region");
			size = 6;
			dst = rt_jit_ploop_read_u16(&ctx->func->bytecode[p + 1]);
			src1 = rt_jit_ploop_read_u16(&ctx->func->bytecode[p + 3]);
			rt_jit_ploop_note_use(ctx, src1);
			if (dst == (uint16_t)ctx->packed_loop_index_tmp ||
			    dst == (uint16_t)ctx->packed_loop_remaining_tmp ||
			    rt_jit_ploop_is_index_alias(ctx, src1))
				return rt_jit_ploop_reject(ctx, "index-escape");
			rt_jit_ploop_remove_index_alias(ctx, dst);
			rt_jit_ploop_remove_base_alias(ctx, dst);
			rt_jit_ploop_count_use(ctx, src1, false);
			ctx->packed_def_lpc[dst] = p;
			rt_jit_ploop_note_def(ctx, dst);
			break;
		case OP_INC:
			{
				int factor;

				factor = (ctx->packed_loop_flags &
					  PLOOP_UNROLL4) != 0 ? 4 : 1;
			if (p + 4 > ctx->func->bytecode_size ||
			    rt_jit_ploop_read_u16(&ctx->func->bytecode[p + 1]) !=
				(uint16_t)ctx->packed_loop_index_tmp ||
			    ctx->func->bytecode[p + 3] != factor)
				return rt_jit_ploop_reject(ctx, "wrong-latch");
			inc_count++;
			size = 4;
			break;
			}
		case OP_SUBJNZ:
			if (p + 8 > ctx->func->bytecode_size)
				return rt_jit_ploop_reject(ctx, "malformed-region");
			value = rt_jit_ploop_read_u16(&ctx->func->bytecode[p + 1]);
			if (value != (uint16_t)ctx->packed_loop_remaining_tmp ||
			    ctx->func->bytecode[p + 3] !=
				((ctx->packed_loop_flags & PLOOP_UNROLL4) != 0 ?
				 4 : 1) ||
			    rt_jit_ploop_read_u32(&ctx->func->bytecode[p + 4]) !=
				body_lpc || inc_count != 1 ||
			    ctx->packed_loop_base_tmp[0] < 0)
				return rt_jit_ploop_reject(ctx, "wrong-latch");
			if (reject_loop_carried &&
			    rt_jit_ploop_has_loop_carried_scalar(ctx))
				return rt_jit_ploop_reject(ctx,
					"loop-carried-scalar");
			for (i = body_lpc; i <= p; i++) {
				if (ctx->packed_lpc_use_count[i] != 0 &&
				    ctx->packed_lpc_use_count[i] ==
					ctx->packed_lpc_address_use_count[i] &&
				    ctx->func->bytecode[i] == OP_ICONST)
					ctx->packed_elide_lpc[i] = 1;
			}
			if (getenv("NOCT_JIT_REGCACHE_SCAN_DEBUG") != NULL) {
				unsigned accesses = 0;
				unsigned elided = 0;
				uint32_t q;

				for (q = body_lpc; q <= p; q++) {
					if (ctx->packed_access_valid[q]) accesses++;
					if (ctx->packed_elide_lpc[q]) elided++;
				}
				fprintf(stderr,
					"noct-jit-regcache-scan: factor=%d accesses=%u elided=%u\n",
					(ctx->packed_loop_flags & PLOOP_UNROLL4) != 0 ? 4 : 1,
					accesses, elided);
			}
			return true;
		default:
			return rt_jit_ploop_reject(ctx, "unsupported-opcode");
		}
		if (p + size > ctx->func->bytecode_size)
			return rt_jit_ploop_reject(ctx, "malformed-region");
		if (base != 0xffffu &&
		    !rt_jit_ploop_add_base(ctx, base, scale))
			return false;
		p += size;
	}
	return rt_jit_ploop_reject(ctx, "malformed-region");
}

bool
rt_jit_ploop_current_access_disp(struct rt_jit_context *ctx, int32_t *disp)
{
	uint32_t p;

	if (!ctx->packed_loop_hint_active || ctx->lpc < 7 ||
	    ctx->packed_access_valid == NULL)
		return false;
	p = ctx->lpc - 7;
	if (p >= ctx->func->bytecode_size || !ctx->packed_access_valid[p])
		return false;
	*disp = ctx->packed_access_disp[p];
	return true;
}

bool
rt_jit_ploop_current_elided(struct rt_jit_context *ctx, uint32_t size)
{
	uint32_t p;

	if (!ctx->packed_loop_hint_active || ctx->lpc < size ||
	    ctx->packed_elide_lpc == NULL)
		return false;
	p = ctx->lpc - size;
	return p < ctx->func->bytecode_size && ctx->packed_elide_lpc[p] != 0;
}

uint32_t
rt_jit_ploop_next_use_lpc(struct rt_jit_context *ctx, int tmp, uint32_t from)
{
	const uint8_t *bc;
	uint32_t p;
	uint16_t a, b, c;
	uint8_t op;

	bc = ctx->func->bytecode;
	p = from;

	while (p < ctx->func->bytecode_size) {
		op = bc[p];
		switch (op) {
		case OP_LINEINFO: p += 5; break;
		case OP_ASSIGN:
			a = rt_jit_ploop_read_u16(bc + p + 1);
			b = rt_jit_ploop_read_u16(bc + p + 3);
			if (b == (uint16_t)tmp) return p;
			if (a == (uint16_t)tmp) return UINT32_MAX;
			p += 5; break;
		case OP_ICONST:
			a = rt_jit_ploop_read_u16(bc + p + 1);
			if (a == (uint16_t)tmp) return UINT32_MAX;
			p += 7; break;
		case OP_PLOAD8U: case OP_PLOAD8S:
		case OP_PLOAD16U: case OP_PLOAD16S: case OP_PLOAD32:
			a = rt_jit_ploop_read_u16(bc + p + 1);
			b = rt_jit_ploop_read_u16(bc + p + 3);
			c = rt_jit_ploop_read_u16(bc + p + 5);
			if (b == (uint16_t)tmp || c == (uint16_t)tmp) return p;
			if (a == (uint16_t)tmp) return UINT32_MAX;
			p += 7; break;
		case OP_PSTORE8: case OP_PSTORE16: case OP_PSTORE32:
			a = rt_jit_ploop_read_u16(bc + p + 1);
			b = rt_jit_ploop_read_u16(bc + p + 3);
			c = rt_jit_ploop_read_u16(bc + p + 5);
			if (a == (uint16_t)tmp || b == (uint16_t)tmp ||
			    c == (uint16_t)tmp) return p;
			p += 7; break;
		case OP_IADD: case OP_ISUB: case OP_IMUL:
		case OP_IDIV: case OP_IMOD: case OP_IAND: case OP_IOR:
		case OP_IXOR: case OP_ILT: case OP_ILTE: case OP_IGT:
		case OP_IGTE: case OP_IDIV_CHECKED: case OP_IMOD_CHECKED:
			a = rt_jit_ploop_read_u16(bc + p + 1);
			b = rt_jit_ploop_read_u16(bc + p + 3);
			c = rt_jit_ploop_read_u16(bc + p + 5);
			if (b == (uint16_t)tmp || c == (uint16_t)tmp) return p;
			if (a == (uint16_t)tmp) return UINT32_MAX;
			p += 7; break;
		case OP_ISHL: case OP_ISHR:
			a = rt_jit_ploop_read_u16(bc + p + 1);
			b = rt_jit_ploop_read_u16(bc + p + 3);
			if (b == (uint16_t)tmp) return p;
			if (a == (uint16_t)tmp) return UINT32_MAX;
			p += 6; break;
		case OP_INC:
			a = rt_jit_ploop_read_u16(bc + p + 1);
			if (a == (uint16_t)tmp) return p;
			p += 4; break;
		case OP_SUBJNZ:
			a = rt_jit_ploop_read_u16(bc + p + 1);
			return a == (uint16_t)tmp ? p : UINT32_MAX;
		default:
			return UINT32_MAX;
		}
	}
	return UINT32_MAX;
}
#endif /* NOCT_USE_OPTIMIZER && PLOOP scanner backends */

bool
rt_jit_context_init_tables(struct rt_jit_context *ctx)
{
	size_t pc_capacity;
	size_t branch_capacity;

	if (ctx->func->bytecode_size == UINT32_MAX) {
		rt_error(ctx->env, N_TR("JIT bytecode is too large."));
		return false;
	}
	pc_capacity = (size_t)ctx->func->bytecode_size + 1;
	branch_capacity = (size_t)ctx->func->bytecode_size;
	if (pc_capacity > SIZE_MAX / sizeof(*ctx->pc_entry) ||
	    branch_capacity > SIZE_MAX / sizeof(*ctx->branch_patch)) {
		rt_error(ctx->env, N_TR("JIT bytecode is too large."));
		return false;
	}
	ctx->pc_entry = noct_malloc(pc_capacity * sizeof(*ctx->pc_entry));
	if (ctx->pc_entry == NULL) {
		rt_out_of_memory(ctx->env);
		return false;
	}
	ctx->pc_entry_capacity = (uint32_t)pc_capacity;
	if (branch_capacity == 0)
		branch_capacity = 1;
	ctx->branch_patch =
		noct_malloc(branch_capacity * sizeof(*ctx->branch_patch));
	if (ctx->branch_patch == NULL) {
		noct_free(ctx->pc_entry);
		ctx->pc_entry = NULL;
		ctx->pc_entry_capacity = 0;
		rt_out_of_memory(ctx->env);
		return false;
	}
	ctx->branch_patch_capacity = (uint32_t)branch_capacity;
	return true;
}

#if defined(NOCT_USE_OPTIMIZER)
bool
rt_jit_context_init_regcache(struct rt_jit_context *ctx)
{
	size_t tmp_capacity;

	if (ctx->gpr_tmp_reg != NULL)
		return true;
	tmp_capacity = ctx->func->tmpvar_size != 0 ?
		(size_t)ctx->func->tmpvar_size : 1;
	if (tmp_capacity > SIZE_MAX / sizeof(*ctx->gpr_tmp_reg) ||
	    tmp_capacity > SIZE_MAX / sizeof(*ctx->gpr_range_min) ||
	    tmp_capacity > SIZE_MAX / sizeof(*ctx->gpr_range_max)) {
		rt_error(ctx->env, N_TR("JIT temporary-variable table is too large."));
		return false;
	}
#if defined(NOCT_ARCH_X86) || defined(NOCT_ARCH_ARM32) || \
	defined(NOCT_ARCH_PPC32) || defined(NOCT_ARCH_MIPS32) || \
	defined(NOCT_ARCH_RISCV32)
	if ((size_t)ctx->func->bytecode_size >
	    SIZE_MAX / sizeof(*ctx->packed_access_disp)) {
		rt_error(ctx->env, N_TR("JIT bytecode analysis table is too large."));
		return false;
	}
#endif
	ctx->gpr_tmp_reg = noct_malloc(tmp_capacity *
				       sizeof(*ctx->gpr_tmp_reg));
	ctx->gpr_tmp_dirty = noct_malloc(tmp_capacity *
					 sizeof(*ctx->gpr_tmp_dirty));
	ctx->gpr_remat_valid = noct_malloc(tmp_capacity *
					 sizeof(*ctx->gpr_remat_valid));
	ctx->gpr_remat_value = noct_malloc(tmp_capacity *
					 sizeof(*ctx->gpr_remat_value));
	ctx->gpr_range_min = noct_malloc(tmp_capacity *
					 sizeof(*ctx->gpr_range_min));
	ctx->gpr_range_max = noct_malloc(tmp_capacity *
					 sizeof(*ctx->gpr_range_max));
	ctx->gpr_range_valid = noct_malloc(tmp_capacity *
					   sizeof(*ctx->gpr_range_valid));
	ctx->packed_index_disp = noct_malloc(tmp_capacity *
					    sizeof(*ctx->packed_index_disp));
	ctx->packed_const_value = noct_malloc(tmp_capacity *
					     sizeof(*ctx->packed_const_value));
	ctx->packed_index_valid = noct_malloc(tmp_capacity *
					     sizeof(*ctx->packed_index_valid));
	ctx->packed_const_valid = noct_malloc(tmp_capacity *
					     sizeof(*ctx->packed_const_valid));
	ctx->packed_access_disp = noct_malloc(ctx->func->bytecode_size != 0 ?
		ctx->func->bytecode_size * sizeof(*ctx->packed_access_disp) :
		sizeof(*ctx->packed_access_disp));
	ctx->packed_access_valid = noct_malloc(ctx->func->bytecode_size != 0 ?
		ctx->func->bytecode_size : 1);
	ctx->packed_elide_lpc = noct_malloc(ctx->func->bytecode_size != 0 ?
		ctx->func->bytecode_size : 1);
	ctx->packed_def_lpc = noct_malloc(tmp_capacity *
					 sizeof(*ctx->packed_def_lpc));
	ctx->packed_lpc_use_count = noct_malloc(
		ctx->func->bytecode_size != 0 ?
		ctx->func->bytecode_size * sizeof(*ctx->packed_lpc_use_count) :
		sizeof(*ctx->packed_lpc_use_count));
	ctx->packed_lpc_address_use_count = noct_malloc(
		ctx->func->bytecode_size != 0 ?
		ctx->func->bytecode_size *
			sizeof(*ctx->packed_lpc_address_use_count) :
		sizeof(*ctx->packed_lpc_address_use_count));
	if (ctx->gpr_tmp_reg == NULL || ctx->gpr_tmp_dirty == NULL ||
	    ctx->gpr_remat_valid == NULL || ctx->gpr_remat_value == NULL ||
	    ctx->gpr_range_min == NULL || ctx->gpr_range_max == NULL ||
	    ctx->gpr_range_valid == NULL || ctx->packed_index_disp == NULL ||
	    ctx->packed_const_value == NULL ||
	    ctx->packed_index_valid == NULL ||
	    ctx->packed_const_valid == NULL ||
	    ctx->packed_access_disp == NULL ||
	    ctx->packed_access_valid == NULL ||
	    ctx->packed_elide_lpc == NULL ||
	    ctx->packed_def_lpc == NULL ||
	    ctx->packed_lpc_use_count == NULL ||
	    ctx->packed_lpc_address_use_count == NULL) {
		noct_free(ctx->packed_lpc_address_use_count);
		noct_free(ctx->packed_lpc_use_count);
		noct_free(ctx->packed_def_lpc);
		noct_free(ctx->packed_elide_lpc);
		noct_free(ctx->packed_access_valid);
		noct_free(ctx->packed_access_disp);
		noct_free(ctx->packed_const_valid);
		noct_free(ctx->packed_index_valid);
		noct_free(ctx->packed_const_value);
		noct_free(ctx->packed_index_disp);
		noct_free(ctx->gpr_range_valid);
		noct_free(ctx->gpr_range_max);
		noct_free(ctx->gpr_range_min);
		noct_free(ctx->gpr_tmp_dirty);
		noct_free(ctx->gpr_remat_value);
		noct_free(ctx->gpr_remat_valid);
		noct_free(ctx->gpr_tmp_reg);
		ctx->gpr_tmp_dirty = NULL;
		ctx->gpr_remat_value = NULL;
		ctx->gpr_remat_valid = NULL;
		ctx->gpr_tmp_reg = NULL;
		ctx->gpr_range_valid = NULL;
		ctx->gpr_range_max = NULL;
		ctx->gpr_range_min = NULL;
		ctx->packed_elide_lpc = NULL;
		ctx->packed_access_valid = NULL;
		ctx->packed_access_disp = NULL;
		ctx->packed_const_valid = NULL;
		ctx->packed_index_valid = NULL;
		ctx->packed_const_value = NULL;
		ctx->packed_index_disp = NULL;
		ctx->packed_lpc_address_use_count = NULL;
		ctx->packed_lpc_use_count = NULL;
		ctx->packed_def_lpc = NULL;
		rt_out_of_memory(ctx->env);
		return false;
	}
	return true;
}
#endif

void
rt_jit_context_dispose_tables(struct rt_jit_context *ctx)
{
#if defined(NOCT_USE_OPTIMIZER)
	noct_free(ctx->tmp_frame_tag_known);
	noct_free(ctx->tmp_fixed_type);
	noct_free(ctx->tmp_compiler_temp);
#endif
	noct_free(ctx->branch_patch);
	noct_free(ctx->pc_entry);
#if defined(NOCT_USE_OPTIMIZER)
	noct_free(ctx->gpr_tmp_dirty);
	noct_free(ctx->gpr_remat_value);
	noct_free(ctx->gpr_remat_valid);
	noct_free(ctx->gpr_tmp_reg);
	noct_free(ctx->gpr_range_valid);
	noct_free(ctx->gpr_range_max);
	noct_free(ctx->gpr_range_min);
	noct_free(ctx->packed_elide_lpc);
	noct_free(ctx->packed_access_valid);
	noct_free(ctx->packed_access_disp);
	noct_free(ctx->packed_const_valid);
	noct_free(ctx->packed_index_valid);
	noct_free(ctx->packed_const_value);
	noct_free(ctx->packed_index_disp);
	noct_free(ctx->packed_lpc_address_use_count);
	noct_free(ctx->packed_lpc_use_count);
	noct_free(ctx->packed_def_lpc);
#endif
	ctx->branch_patch = NULL;
#if defined(NOCT_USE_OPTIMIZER)
	ctx->tmp_frame_tag_known = NULL;
	ctx->tmp_fixed_type = NULL;
	ctx->tmp_compiler_temp = NULL;
#endif
	ctx->pc_entry = NULL;
#if defined(NOCT_USE_OPTIMIZER)
	ctx->gpr_tmp_dirty = NULL;
	ctx->gpr_remat_value = NULL;
	ctx->gpr_remat_valid = NULL;
	ctx->gpr_tmp_reg = NULL;
	ctx->gpr_range_valid = NULL;
	ctx->gpr_range_max = NULL;
	ctx->gpr_range_min = NULL;
	ctx->packed_elide_lpc = NULL;
	ctx->packed_access_valid = NULL;
	ctx->packed_access_disp = NULL;
	ctx->packed_const_valid = NULL;
	ctx->packed_index_valid = NULL;
	ctx->packed_const_value = NULL;
	ctx->packed_index_disp = NULL;
	ctx->packed_lpc_address_use_count = NULL;
	ctx->packed_lpc_use_count = NULL;
	ctx->packed_def_lpc = NULL;
#endif
	ctx->branch_patch_capacity = 0;
	ctx->pc_entry_capacity = 0;
}

void
rt_jit_configure_simd(struct rt_jit_context *ctx, uint32_t detected,
			   const char *backend)
{
#if defined(NOCT_USE_OPTIMIZER)
	ctx->simd_caps = jit_apply_simd_max(detected);
	ctx->has_vector_ops = ctx->func->has_vector_ops;
	if (ctx->func->has_fma_ops &&
	    (ctx->simd_caps & JIT_SIMD_CAP_FMAF32X4) == 0)
		ctx->simd_caps = 0;
	if (getenv("NOCT_JIT_SIMD_DEBUG") != NULL) {
		fprintf(stderr,
			"noct-jit-simd: %s: caps=0x%x vector=%d fma=%d mode=%s\n",
			backend, (unsigned)ctx->simd_caps,
			ctx->has_vector_ops ? 1 : 0,
			ctx->func->has_fma_ops ? 1 : 0,
			ctx->func->has_fma_ops &&
			(ctx->simd_caps & JIT_SIMD_CAP_FMAF32X4) == 0 ?
					"portable" : "native");
	}
#else
	UNUSED_PARAMETER(ctx);
	UNUSED_PARAMETER(detected);
	UNUSED_PARAMETER(backend);
#endif
}

void
rt_jit_dump_standard_code(struct rt_jit_context *ctx, void *generated_end,
		       const char *backend)
{
	const char *dir;
	const char *src;
	char name[96];
	char path[512];
	size_t i;
	size_t n;
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
			(c >= '0' && c <= '9') || c == '_' || c == '-' ?
			c : '_';
	}
	name[i] = '\0';
	if (snprintf(path, sizeof(path), "%s/%s-%p.%s.bin", dir, name,
		     (void *)ctx->func->bytecode, backend) >= (int)sizeof(path))
		return;
	fp = fopen(path, "wb");
	if (fp == NULL)
		return;
	n = (size_t)((uint8_t *)generated_end - (uint8_t *)ctx->code_top);
	(void)fwrite(ctx->code_top, 1, n, fp);
	(void)fclose(fp);
}

#if defined(NOCT_USE_OPTIMIZER)
bool
rt_jit_visit_ploop_hint_op(struct rt_jit_context *ctx)
{
	int index_tmp;
	int stop_tmp;
	int remaining_tmp;
	int lanes;
	int flags;

	if (!rt_jit_get_opr_tmpvar(ctx, &index_tmp) ||
	    !rt_jit_get_opr_tmpvar(ctx, &stop_tmp) ||
	    !rt_jit_get_opr_tmpvar(ctx, &remaining_tmp) ||
	    !rt_jit_get_imm8(ctx, &lanes) ||
	    !rt_jit_get_imm8(ctx, &flags))
		return false;
	if (lanes != 1 ||
	    ((flags & PLOOP_TYPED_INT) != 0 &&
	     (flags & PLOOP_TYPED_FLOAT) != 0)) {
		rt_error(ctx->env, BROKEN_BYTECODE);
		return false;
	}
	ctx->packed_loop_hint_active = true;
	ctx->packed_loop_index_tmp = index_tmp;
	ctx->packed_loop_stop_tmp = stop_tmp;
	ctx->packed_loop_remaining_tmp = remaining_tmp;
	ctx->packed_loop_lanes = lanes;
	ctx->packed_loop_flags = flags;
	return true;
}

bool
rt_jit_visit_tmpvar_type_op(struct rt_jit_context *ctx)
{
	int tmp;
	int type;
	size_t count;
	bool compiler_temp;

	if (!rt_jit_get_opr_tmpvar(ctx, &tmp) || !rt_jit_get_imm8(ctx, &type))
		return false;

	compiler_temp = (type & TMPVAR_TYPE_COMPILER_TEMP) != 0;
	type &= ~TMPVAR_TYPE_COMPILER_TEMP;
	if (type != TMPVAR_TYPE_DYNAMIC &&
	    type != NOCT_VALUE_INT && type != NOCT_VALUE_LONG &&
	    type != NOCT_VALUE_FLOAT && type != NOCT_VALUE_DOUBLE) {
		rt_error(ctx->env, BROKEN_BYTECODE);
		return false;
	}
	if (ctx->tmp_fixed_type == NULL) {
		count = ctx->func->tmpvar_size != 0 ?
			(size_t)ctx->func->tmpvar_size : 1;
		ctx->tmp_fixed_type = noct_malloc(count);
		ctx->tmp_frame_tag_known = noct_calloc(count, 1);
		ctx->tmp_compiler_temp = noct_calloc(count, 1);
		if (ctx->tmp_fixed_type == NULL ||
		    ctx->tmp_frame_tag_known == NULL ||
		    ctx->tmp_compiler_temp == NULL) {
			noct_free(ctx->tmp_compiler_temp);
			noct_free(ctx->tmp_frame_tag_known);
			noct_free(ctx->tmp_fixed_type);
			ctx->tmp_frame_tag_known = NULL;
			ctx->tmp_fixed_type = NULL;
			ctx->tmp_compiler_temp = NULL;
			rt_out_of_memory(ctx->env);
			return false;
		}
		memset(ctx->tmp_fixed_type, -1, count);
	}
	ctx->tmp_fixed_type[tmp] = type == TMPVAR_TYPE_DYNAMIC ? -1 :
		(int8_t)type;
	ctx->tmp_compiler_temp[tmp] = compiler_temp ? 1 : 0;
	/* A fresh non-parameter slot has zero tag, which is INT. */
	if (!compiler_temp && (uint32_t)tmp >= ctx->func->param_count &&
	    type == NOCT_VALUE_INT)
		ctx->tmp_frame_tag_known[tmp] = 1;
	return true;
}

bool
rt_jit_visit_materialize_type_metadata_op(struct rt_jit_context *ctx)
{
	int tmp;
	int type;

	if (!rt_jit_get_opr_tmpvar(ctx, &tmp) || !rt_jit_get_imm8(ctx, &type))
		return false;
	if (type != NOCT_VALUE_INT && type != NOCT_VALUE_LONG &&
	    type != NOCT_VALUE_FLOAT && type != NOCT_VALUE_DOUBLE) {
		rt_error(ctx->env, BROKEN_BYTECODE);
		return false;
	}
	return true;
}

bool
rt_jit_tmp_has_fixed_primitive_type(struct rt_jit_context *ctx, int tmp, int type)
{
	return ctx->tmp_fixed_type != NULL &&
	       (type == NOCT_VALUE_INT || type == NOCT_VALUE_LONG ||
		type == NOCT_VALUE_FLOAT || type == NOCT_VALUE_DOUBLE) &&
	       ctx->tmp_fixed_type[tmp] == type;
}
#else
bool
rt_jit_visit_ploop_hint_op(struct rt_jit_context *ctx)
{
	int index_tmp;
	int stop_tmp;
	int remaining_tmp;
	int lanes;
	int flags;

	if (!rt_jit_get_opr_tmpvar(ctx, &index_tmp) ||
	    !rt_jit_get_opr_tmpvar(ctx, &stop_tmp) ||
	    !rt_jit_get_opr_tmpvar(ctx, &remaining_tmp) ||
	    !rt_jit_get_imm8(ctx, &lanes) ||
	    !rt_jit_get_imm8(ctx, &flags))
		return false;

	return true;
}

bool
rt_jit_visit_tmpvar_type_op(struct rt_jit_context *ctx)
{
	int tmp;
	int type;

	if (!rt_jit_get_opr_tmpvar(ctx, &tmp) ||
	    !rt_jit_get_imm8(ctx, &type))
		return false;

	return true;
}

bool
rt_jit_visit_materialize_type_metadata_op(struct rt_jit_context *ctx)
{
	int tmp;
	int type;

	if (!rt_jit_get_opr_tmpvar(ctx, &tmp) ||
	    !rt_jit_get_imm8(ctx, &type))
		return false;

	return true;
}

bool
rt_jit_tmp_has_fixed_primitive_type(struct rt_jit_context *ctx, int tmp,
				    int type)
{
	UNUSED_PARAMETER(ctx);
	UNUSED_PARAMETER(tmp);
	UNUSED_PARAMETER(type);

	return false;
}
#endif

/* Build a function with the standard JIT backend workflow. */
bool
rt_jit_build_standard(
	struct rt_env *env,
	struct rt_func *func,
	uint32_t detected_caps,
	const char *backend,
	bool (*visit_bytecode)(struct rt_jit_context *ctx),
	bool (*patch_branch)(struct rt_jit_context *ctx, int patch_index))
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
		memset(&ctx, 0, sizeof(ctx));
		ctx.code_top = code_top;
		ctx.code_end = code_end;
		ctx.code = code_top;
		ctx.env = env;
		ctx.func = func;
		if (!rt_jit_context_init_tables(&ctx))
			return false;
		rt_jit_configure_simd(&ctx, detected_caps, backend);
		if (!visit_bytecode(&ctx)) {
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
			if (!patch_branch(&ctx, i)) {
				rt_jit_context_dispose_tables(&ctx);
				return false;
			}
		}
		rt_jit_dump_standard_code(&ctx, generated_end, backend);
		rt_jit_slab_finish(env, slab, generated_end);
		if (getenv("NOCT_JIT_CODEGEN_DEBUG") != NULL) {
			fprintf(stderr,
				"noct-jit-codegen: %s: func=%s bytes=%lu "
				"pc_entries=%u branches=%d\n",
				backend, func->name != NULL ? func->name : "?",
				(unsigned long)((uint8_t *)generated_end -
						(uint8_t *)ctx.code_top),
				ctx.pc_entry_count, ctx.branch_patch_count);
		}
		func->jit_code =
			(bool (CDECL *)(struct rt_env *))ctx.code_top;
		rt_jit_context_dispose_tables(&ctx);
		return true;
	}
	return false;
}

/*
 * Map the memory region for the generated code.
 */
bool
rt_jit_map_memory_region(
	void **region,
	size_t size)
{
	unsigned long error;

	error = 0;

#if defined(_WIN32)
	*region = VirtualAlloc(NULL, size, MEM_RESERVE | MEM_COMMIT,
			       PAGE_READWRITE);
	if (*region == NULL)
		error = (unsigned long)GetLastError();
#elif defined(__APPLE__)
	/* Use MAP_JIT flag to avoid W^X. */
	*region = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_ANON | MAP_PRIVATE | MAP_JIT, -1, 0);
#elif defined(__FreeBSD__) && defined(PROT_MAX)
	/* Use PROT_MAX() to avoid W^X. */
	*region = mmap(NULL, size, PROT_READ | PROT_WRITE |
		       PROT_MAX(PROT_READ | PROT_WRITE | PROT_EXEC),
		       MAP_ANON | MAP_PRIVATE, -1, 0);
#elif defined(__NetBSD__) && defined(PROT_MPROTECT)
	/* Use PROT_MPROTECT() to avoid W^X. */
	*region = mmap(NULL, size, PROT_READ | PROT_WRITE | PROT_MPROTECT(PROT_READ | PROT_EXEC), MAP_ANON | MAP_PRIVATE, -1, 0);
#elif defined(NOCT_TARGET_DOS)
	*region = noct_malloc(size);
	{
		union REGS regs;
		unsigned short current_cs;

		_asm { mov current_cs, cs }
		regs.w.ax = 0x0008;
		regs.w.bx = current_cs;
		regs.w.cx = 0xFFFF;
		regs.w.dx = 0x000F;
		int386(0x31, &regs, &regs);
		if (regs.w.cflag != 0) {
			printf(N_TR("Failed to expand the CS segment limit.\n"));
			jit_debug_memory("mmap-rw", size, false, (unsigned long)regs.w.ax);
			return false;
		}
	}
#elif defined(NOCT_TARGET_PC98BE)
	/* The i386 bootstrap environment has one flat executable address space. */
	*region = noct_malloc(size);
#else
	/* Assume no W^X. */
	*region = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_ANON | MAP_PRIVATE, -1, 0);
#endif
#if !defined(_WIN32) && !defined(NOCT_TARGET_DOS) && \
    !defined(NOCT_TARGET_PC98BE)
	if (*region == MAP_FAILED) {
		error = (unsigned long)errno;
		*region = NULL;
		jit_debug_memory("mmap-rw", size, false, error);
		return false;
	}
#else
	if (*region == NULL) {
		jit_debug_memory("mmap-rw", size, false, error);
		return false;
	}
#endif

	/* Anonymous mappings and VirtualAlloc are already zero-filled.  Only
	 * malloc-backed freestanding targets require eager initialization. */
#if defined(NOCT_TARGET_DOS) || defined(NOCT_TARGET_PC98BE)
	memset(*region, 0, size);
#endif

	jit_debug_memory("mmap-rw", size, true, error);
	return true;
}

/*
 * Unmap the memory region for the generated code.
 */
bool
rt_jit_unmap_memory_region(
	void *region,
	size_t size)
{
	bool succeeded;
	unsigned long error;

	succeeded = true;
	error = 0;

#if defined(_WIN32)
	UNUSED_PARAMETER(size);
	if (!VirtualFree(region, 0, MEM_RELEASE)) {
		succeeded = false;
		error = (unsigned long)GetLastError();
	}
#elif defined(NOCT_TARGET_DOS)
	/* Do nothing. */
#elif defined(NOCT_TARGET_PC98BE)
	UNUSED_PARAMETER(size);
	noct_free(region);
#else
	if (munmap(region, size) != 0) {
		succeeded = false;
		error = (unsigned long)errno;
	}
#endif
	jit_debug_memory("munmap", size, succeeded, error);
	return succeeded;
}

/*
 * Make a region executable and non-writable.
 */
bool
rt_jit_map_executable(
	void *region,
	size_t size)
{
	bool succeeded;
	unsigned long error;

	succeeded = true;
	error = 0;

#if defined(_WIN32)

	/*
	 * Win32
	 */
	DWORD dwOldProt;

	if (!VirtualProtect(region, size, PAGE_EXECUTE_READ, &dwOldProt)) {
		succeeded = false;
		error = (unsigned long)GetLastError();
	} else if (!FlushInstructionCache(GetCurrentProcess(), region, size)) {
		succeeded = false;
		error = (unsigned long)GetLastError();
	}

#elif defined(NOCT_TARGET_DOS)

	/*
	 * DOS/DPMI
	 */

	UNUSED_PARAMETER(region);
	UNUSED_PARAMETER(size);

	/* No need for mmap() */

#elif defined(NOCT_TARGET_POSIX)

	/*
	 * POSIX
	 */

	if (mprotect(region, size, PROT_EXEC | PROT_READ) != 0) {
		succeeded = false;
		error = (unsigned long)errno;
	} else {
		__builtin___clear_cache((char *)region, (char *)region + size);
	}

#endif

	jit_debug_memory("mprotect-rx", size, succeeded, error);

	return succeeded;
}

/*
 * Acquire the slab.
 */
bool
rt_jit_slab_acquire(
	struct rt_env *env,
	struct rt_jit_slab **slab,
	void **code_top,
	void **code_end)
{
	struct rt_jit_slab *current;

	current = (struct rt_jit_slab *)env->vm->jit_slab_current;

	if (current == NULL || current->current >= current->end) {
		if (!jit_slab_allocate(env, 0, &current))
			return false;
	}

	*slab = current;
	*code_top = current->current;
	*code_end = current->end;

	return true;
}

/*
 * Reserve the slab.
 */
bool
rt_jit_slab_reserve(
	struct rt_env *env,
	size_t estimated_size)
{
	struct rt_jit_slab *slab;

	if (!env->vm->config.jit_enable ||
	    env->vm->jit_slab_current != NULL)
		return true;

	return jit_slab_allocate(env, estimated_size, &slab);
}

/*
 * Finish using the slab.
 */
void
rt_jit_slab_finish(struct rt_env *env, struct rt_jit_slab *slab, void *code_end)
{
	assert(slab == (struct rt_jit_slab *)env->vm->jit_slab_current);
	assert((uint8_t *)code_end >= slab->current);
	assert((uint8_t *)code_end <= slab->end);

	slab->current = code_end;
}

/*
 * Abort using the slab.
 */
void
rt_jit_slab_abandon(struct rt_env *env, struct rt_jit_slab *slab)
{
	if ((struct rt_jit_slab *)env->vm->jit_slab_current == slab)
		env->vm->jit_slab_current = NULL;
}

/*
 * Clear the slab overflow status.
 */
void
rt_jit_slab_clear_overflow(
	struct rt_env *env)
{
	env->error_message[0] = '\0';
	env->line = 0;
}

/*
 * Commit the slab.
 */
bool
rt_jit_slab_commit_all(struct rt_env *env)
{
	struct rt_jit_slab *slab;
	size_t page_size;

	page_size = jit_page_size();

	for (slab = (struct rt_jit_slab *)env->vm->jit_slab_head;
	     slab != NULL;
	     slab = slab->next) {
		uint8_t *end;

		if (slab->committed >= slab->current)
			continue;

		end = slab->base + jit_align_up((size_t)(slab->current - slab->base), page_size);

		assert(end <= slab->end);

		if (!rt_jit_map_executable(slab->committed, (size_t)(end - slab->committed)))
			return false;

		slab->committed = end;
		slab->current = end;
	}

	return true;
}

/*
 * Free all slabs.
 */
bool
rt_jit_slab_free_all(struct rt_env *env)
{
	struct rt_jit_slab *slab;
	bool succeeded;

	slab = (struct rt_jit_slab *)env->vm->jit_slab_head;
	succeeded = true;

	while (slab != NULL) {
		struct rt_jit_slab *next;

		next = slab->next;

		if (!rt_jit_unmap_memory_region(slab->base, slab->size))
			succeeded = false;
		noct_free(slab);
		slab = next;
	}
	env->vm->jit_slab_head = NULL;
	env->vm->jit_slab_tail = NULL;
	env->vm->jit_slab_current = NULL;
	return succeeded;
}

/* Check whether JIT memory diagnostics are enabled. */
static bool
jit_debug_enabled(void)
{
	return getenv("NOCT_JIT_DEBUG") != NULL;
}

/* Print a JIT memory operation when diagnostics are enabled. */
static void
jit_debug_memory(
	const char *operation,
	size_t size,
	bool success,
	unsigned long error)
{
	if (!jit_debug_enabled())
		return;
	fprintf(stderr, "noct-jit-memory: %s size=%lu status=%s", operation,
		(unsigned long)size, success ? "ok" : "failed");
	if (!success)
		fprintf(stderr, " error=%lu", error);
	fputc('\n', stderr);
}

/* Limit detected SIMD capabilities according to the test ceiling. */
#if defined(NOCT_USE_OPTIMIZER)
static uint32_t
jit_apply_simd_max(uint32_t detected)
{
	const char *max;

	max = getenv("NOCT_JIT_SIMD_MAX");

	if (max == NULL || max[0] == '\0')
		return detected;
	if (strcmp(max, "scalar") == 0)
		return 0;
	if (strcmp(max, "sse2") == 0)
		return detected & JIT_SIMD_CAP_SSE2;
	if (strcmp(max, "sse3") == 0)
		return detected & (JIT_SIMD_CAP_SSE2 | JIT_SIMD_CAP_SSE3);
	if (strcmp(max, "sse41") == 0)
		return detected & (JIT_SIMD_CAP_SSE2 | JIT_SIMD_CAP_SSE3 |
				   JIT_SIMD_CAP_SSE41);
	if (strcmp(max, "avx") == 0)
		return detected & (JIT_SIMD_CAP_SSE2 | JIT_SIMD_CAP_SSE3 |
				   JIT_SIMD_CAP_SSE41 | JIT_SIMD_CAP_AVX);
	if (strcmp(max, "neon") == 0)
		return detected & (JIT_SIMD_CAP_NEON |
				   JIT_SIMD_CAP_FMAF32X4);
	if (strcmp(max, "altivec") == 0)
		return detected & JIT_SIMD_CAP_ALTIVEC;
	if (strcmp(max, "fma") == 0)
		return detected;
	return detected;
}
#endif

/* Get the OS page size. */
static size_t
jit_page_size(void)
{
#if defined(_WIN32)

	/*
	 * Win32
	 */
	SYSTEM_INFO info;

	GetSystemInfo(&info);
	return (size_t)info.dwPageSize;

#elif defined(NOCT_TARGET_DOS) || defined(NOCT_TARGET_PC98BE)

	/*
	 * DOS/DPMI
	 */

	return 16;

#elif defined(NOCT_TARGET_ZEDBSD)

	/*
	 * XXX: Can we use sysconf()?
	 */

	return 4096;

#else

	/*
	 * POSIX
	 */

	long size;

	size = sysconf(_SC_PAGESIZE);

	return size > 0 ? (size_t)size : 4096;
#endif
}

/* Alignment. */
static size_t
jit_align_up(
	size_t value,
	size_t alignment)
{
	return (value + alignment - 1) / alignment * alignment;
}

/* Slab allocator. */
static bool
jit_slab_allocate(
	struct rt_env *env,
	size_t requested_size,
	struct rt_jit_slab **result)
{
	struct rt_jit_slab *slab;
	size_t size;
	size_t page_size;

	page_size = jit_page_size();

	if (requested_size == 0 || requested_size > rt_jit_get_code_size(env))
		requested_size = rt_jit_get_code_size(env);

	if (requested_size < page_size)
		requested_size = page_size;

	size = jit_align_up(requested_size, page_size);

	slab = noct_malloc(sizeof(*slab));
	if (slab == NULL) {
		rt_out_of_memory(env);
		return false;
	}

	memset(slab, 0, sizeof(*slab));

	if (!rt_jit_map_memory_region((void **)&slab->base, size)) {
		noct_free(slab);
		rt_error(env, N_TR("Memory mapping failed."));
		return false;
	}

	slab->current = slab->base;
	slab->committed = slab->base;
	slab->end = slab->base + size;
	slab->size = size;

	if (env->vm->jit_slab_tail != NULL)
		((struct rt_jit_slab *)env->vm->jit_slab_tail)->next = slab;
	else
		env->vm->jit_slab_head = (void *)slab;

	env->vm->jit_slab_tail = (void *)slab;
	env->vm->jit_slab_current = (void *)slab;

	*result = slab;

	return true;
}

#else /* defined(NOCT_USE_JIT) */

/*
 * Stub for non-JIT build.
 */

#include "runtime.h"

/*
 * Generate a JIT-compiled code for a function.
 */
bool
jit_build(
	struct rt_env *env,
	struct rt_func *func)
{
	UNUSED_PARAMETER(env);
	UNUSED_PARAMETER(func);

	/* stub */
	return false;
}

/*
 * Commit written code.
 */
bool
jit_commit(
	struct rt_env *env)
{
	UNUSED_PARAMETER(env);

	/* stub */
	return true;
}

/*
 * Free a JIT-compiled code for a function.
 */
bool
jit_free(
	struct rt_env *env)
{
	UNUSED_PARAMETER(env);

	/* stub */
	return true;
}

#endif /* defined(NOCT_USE_JIT) */
