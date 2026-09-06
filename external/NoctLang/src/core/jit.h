/* -*- coding: utf-8; tab-width: 8; indent-tabs-mode: t; -*- */

/*
 * Noct Programming Language
 * Copyright (c) 2025, 2026, Awe Morris
 */

/*
 * JIT (mips32): Just-In-Time native code generation
 */

#ifndef NOCT_JIT_H
#define NOCT_JIT_H

#include <noct/noct.h>
#include <noct/executor.h>
#include "bytecode.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define ex_make_string_with_hash noct_ex_make_string_with_hash
#define ex_make_empty_array noct_ex_make_empty_array
#define ex_make_empty_dict noct_ex_make_empty_dict
#define ex_add_helper noct_ex_add_helper
#define ex_sub_helper noct_ex_sub_helper
#define ex_mul_helper noct_ex_mul_helper
#define ex_div_helper noct_ex_div_helper
#define ex_mod_helper noct_ex_mod_helper
#define ex_and_helper noct_ex_and_helper
#define ex_or_helper noct_ex_or_helper
#define ex_xor_helper noct_ex_xor_helper
#define ex_shl_helper noct_ex_shl_helper
#define ex_shr_helper noct_ex_shr_helper
#define ex_neg_helper noct_ex_neg_helper
#define ex_not_helper noct_ex_not_helper
#define ex_lt_helper noct_ex_lt_helper
#define ex_lte_helper noct_ex_lte_helper
#define ex_eq_helper noct_ex_eq_helper
#define ex_neq_helper noct_ex_neq_helper
#define ex_gte_helper noct_ex_gte_helper
#define ex_gt_helper noct_ex_gt_helper
#define ex_storearray_helper noct_ex_storearray_helper
#define ex_loadarray_helper noct_ex_loadarray_helper
#define ex_len_helper noct_ex_len_helper
#define ex_getdictkeybyindex_helper noct_ex_getdictkeybyindex_helper
#define ex_getdictvalbyindex_helper noct_ex_getdictvalbyindex_helper
#define ex_loadsymbol_helper noct_ex_loadsymbol_helper
#define ex_storesymbol_helper noct_ex_storesymbol_helper
#define ex_loaddot_helper noct_ex_loaddot_helper
#define ex_storedot_helper noct_ex_storedot_helper
#define ex_call_helper noct_ex_call_helper
#define ex_thiscall_helper noct_ex_thiscall_helper
#define ex_safepoint_helper noct_ex_safepoint_helper
#define ex_pbase_helper noct_ex_pbase_helper
#define ex_pcheck_helper noct_ex_pcheck_helper
#define ex_typeis_helper noct_ex_typeis_helper
#define ex_plen_helper noct_ex_plen_helper
#define ex_pload8u_helper noct_ex_pload8u_helper
#define ex_pstore8_helper noct_ex_pstore8_helper
#define ex_checktype_helper noct_ex_checktype_helper
#define ex_condition_helper noct_ex_condition_helper
#define ex_pload8s_helper noct_ex_pload8s_helper
#define ex_pload16u_helper noct_ex_pload16u_helper
#define ex_pload16s_helper noct_ex_pload16s_helper
#define ex_pload32_helper noct_ex_pload32_helper
#define ex_pload64_helper noct_ex_pload64_helper
#define ex_pstore16_helper noct_ex_pstore16_helper
#define ex_pstore32_helper noct_ex_pstore32_helper
#define ex_pstore64_helper noct_ex_pstore64_helper
#define ex_iadd_helper noct_ex_iadd_helper
#define ex_isub_helper noct_ex_isub_helper
#define ex_imul_helper noct_ex_imul_helper
#define ex_idiv_helper noct_ex_idiv_helper
#define ex_imod_helper noct_ex_imod_helper
#define ex_iand_helper noct_ex_iand_helper
#define ex_ior_helper noct_ex_ior_helper
#define ex_ixor_helper noct_ex_ixor_helper
#define ex_ishl_helper noct_ex_ishl_helper
#define ex_ishr_helper noct_ex_ishr_helper
#define ex_ilt_helper noct_ex_ilt_helper
#define ex_ilte_helper noct_ex_ilte_helper
#define ex_igt_helper noct_ex_igt_helper
#define ex_igte_helper noct_ex_igte_helper
#define ex_fadd_helper noct_ex_fadd_helper
#define ex_fsub_helper noct_ex_fsub_helper
#define ex_fmul_helper noct_ex_fmul_helper
#define ex_fdiv_helper noct_ex_fdiv_helper
#define ex_flt_helper noct_ex_flt_helper
#define ex_flte_helper noct_ex_flte_helper
#define ex_fgt_helper noct_ex_fgt_helper
#define ex_fgte_helper noct_ex_fgte_helper
#define ex_vloadi32x4_helper noct_ex_vloadi32x4_helper
#define ex_vstorei32x4_helper noct_ex_vstorei32x4_helper
#define ex_vsplati32_helper noct_ex_vsplati32_helper
#define ex_vgetlanei32_helper noct_ex_vgetlanei32_helper
#define ex_vmov128_helper noct_ex_vmov128_helper
#define ex_vaddi32x4_helper noct_ex_vaddi32x4_helper
#define ex_vsubi32x4_helper noct_ex_vsubi32x4_helper
#define ex_vmuli32x4_helper noct_ex_vmuli32x4_helper
#define ex_vand128_helper noct_ex_vand128_helper
#define ex_vor128_helper noct_ex_vor128_helper
#define ex_vxor128_helper noct_ex_vxor128_helper
#define ex_vshli32x4_helper noct_ex_vshli32x4_helper
#define ex_vshri32x4_helper noct_ex_vshri32x4_helper
#define ex_vloadf32x4_helper noct_ex_vloadf32x4_helper
#define ex_vstoref32x4_helper noct_ex_vstoref32x4_helper
#define ex_vsplatf32_helper noct_ex_vsplatf32_helper
#define ex_vgetlanef32_helper noct_ex_vgetlanef32_helper
#define ex_vaddf32x4_helper noct_ex_vaddf32x4_helper
#define ex_vsubf32x4_helper noct_ex_vsubf32x4_helper
#define ex_vmulf32x4_helper noct_ex_vmulf32x4_helper
#define ex_vdivf32x4_helper noct_ex_vdivf32x4_helper
#define ex_vori32x4i_helper noct_ex_vori32x4i_helper
#define ex_vfmaf32x4_helper noct_ex_vfmaf32x4_helper
#define ex_vcmpi32x4_helper noct_ex_vcmpi32x4_helper
#define ex_vcmpf32x4_helper noct_ex_vcmpf32x4_helper
#define ex_vselect128_helper noct_ex_vselect128_helper
#define ex_vmaskstorei32x4_helper noct_ex_vmaskstorei32x4_helper
#define ex_vinductf32x4_helper noct_ex_vinductf32x4_helper
#define ex_vgatheri32x4_checked_helper noct_ex_vgatheri32x4_checked_helper
#define ex_ploadf32_helper noct_ex_ploadf32_helper
#define ex_pstoref32_helper noct_ex_pstoref32_helper

/* Generate a JIT-compiled code for a function. */
bool
jit_build(
	struct rt_env *env,
	struct rt_func *func);

/* Commit written code. */
bool
jit_commit(
	struct rt_env *env);

/* Free all JIT-compiled code. */
bool
jit_free(
	struct rt_env *env);

/*
 * If JIT is enabled.
 */
#if defined(NOCT_USE_JIT)

/* Per-function dynamic JIT table bounds used by architecture backends. */
#define PC_ENTRY_MAX			(ctx->pc_entry_capacity)
#define BRANCH_PATCH_MAX		(ctx->branch_patch_capacity)

/* Runtime SIMD capabilities, detected independently by each JIT backend. */
#define JIT_SIMD_CAP_SSE2		(1u << 0)
#define JIT_SIMD_CAP_SSE3		(1u << 1)
#define JIT_SIMD_CAP_SSE41		(1u << 2)
#define JIT_SIMD_CAP_NEON		(1u << 3)
#define JIT_SIMD_CAP_ALTIVEC		(1u << 4)
#define JIT_SIMD_CAP_FMAF32X4		(1u << 5)
#define JIT_SIMD_CAP_AVX		(1u << 6)

/* Error message. */
#define BROKEN_BYTECODE			N_TR("Broken bytecode.")

/* Code size. */
#if defined(NOCT_JIT_CODE_MAX)
#define JIT_CODE_MAX			NOCT_JIT_CODE_MAX
#elif !defined(NOCT_TARGET_DOS)
#define JIT_CODE_MAX			(16 * 1024 * 1024)
#else
#define JIT_CODE_MAX			(1 * 1024 * 1024)
#endif

/*
 * JIT codegen context
 */
struct rt_jit_context {
	/* Env. */
	struct rt_env *env;

	/* Function. */
	struct rt_func *func;

	/* Top of the mapped code area. */
	void *code_top;

	/* End of the mapped code area. */
	void *code_end;

	/* Current code position in the mapped code area. */
	void *code;

	/* The current function did not fit and may be retried on a fresh slab. */
	bool code_overflow;

	/* Exception handler address of the current function. */
	void *exception_code;

	/* Current PC in LIR. */
	uint32_t lpc;

	/*
	 * Mapping table from LIR-PC to Native-PC.
	 */
	struct pc_entry {
		/* LIR-PC */
		uint32_t lpc;

		/* Native-PC */
		uint32_t *code;
	} *pc_entry;
	uint32_t pc_entry_count;
	uint32_t pc_entry_capacity;

	/*
	 * Delayed branch patching table.
	 */
	struct branch_patch {
		/* Native code address. */
		uint32_t *code;

		/* LIR-PC */
		uint32_t lpc;

		/* Branch type. */
		int type;
	} *branch_patch;
	int branch_patch_count;
	uint32_t branch_patch_capacity;

#if defined(NOCT_USE_OPTIMIZER)
	/*
	 * For vectorized code.
	 */
	uint32_t simd_caps;
	bool has_vector_ops;
	int vector_kind;		/* 0 unknown, 1 integer, 2 float region */
	bool vector_hint_active;
	int vector_hint_index_tmp;
	int vector_hint_stop_tmp;
	int vector_hint_remaining_tmp;
	int vector_hint_lanes;
	int vector_hint_flags;
	int vector_vreg_limit;
	int vector_base_tmp[2];
	uint32_t vector_base_last_lpc[2];
	int vector_imm_value;
	int vector_imm_shift;
	int vector_imm_reg;

	/*
	 * For loop optimized code.
	 */
	bool packed_loop_hint_active;
	int packed_loop_index_tmp;
	int packed_loop_stop_tmp;
	int packed_loop_remaining_tmp;
	int packed_loop_lanes;
	int packed_loop_flags;
	int packed_loop_base_tmp[3];
	int packed_loop_base_scale[3];
	uint16_t packed_loop_index_alias[32];
	int32_t packed_loop_index_alias_disp[32];
	int packed_loop_index_alias_count;
	uint16_t packed_loop_base_alias_tmp[64];
	uint16_t packed_loop_base_alias_root[64];
	int packed_loop_base_alias_count;
	int32_t *packed_index_disp;
	int32_t *packed_const_value;
	int32_t *packed_access_disp;
	uint8_t *packed_index_valid;
	uint8_t *packed_const_valid;
	uint8_t *packed_access_valid;
	uint8_t *packed_elide_lpc;
	uint32_t *packed_def_lpc;
	uint32_t *packed_lpc_use_count;
	uint32_t *packed_lpc_address_use_count;
	const char *packed_loop_reject_reason;

	/*
	 * GPR allocation / spill.
	 */
	int *gpr_tmp_reg;
	uint8_t *gpr_tmp_dirty;
	uint8_t *gpr_remat_valid;
	int32_t *gpr_remat_value;
	int8_t *tmp_fixed_type;
	uint8_t *tmp_frame_tag_known;
	uint8_t *tmp_compiler_temp;
	int32_t *gpr_range_min;
	int32_t *gpr_range_max;
	uint8_t *gpr_range_valid;
	int gpr_reg_tmp[6];
	int gpr_next_victim;
	int gpr_reg_limit;
	bool gpr_cache_active;
	int gpr_load_tmp[3];
	int gpr_load_opcode[3];
	int gpr_load_disp[3];
	unsigned gpr_hits;
	unsigned gpr_misses;
	unsigned gpr_spills;
	unsigned gpr_dead_drops;
	unsigned gpr_proven_divisions;
#endif
};

/*
 * Slab: VM-local JIT allocation.
 */
struct rt_jit_slab {
	uint8_t *base;
	uint8_t *current;
	uint8_t *committed;
	uint8_t *end;
	size_t size;
	struct rt_jit_slab *next;
};

/* Map a region. */
bool rt_jit_map_memory_region(void **region, size_t size);

/* Unmap a region. */
bool rt_jit_unmap_memory_region(void *region, size_t size);

/* Make a region executable. */
bool rt_jit_map_executable(void *region, size_t size);

/* Acquire a JIT slab. */
bool
rt_jit_slab_acquire(
	struct rt_env *env,
	struct rt_jit_slab **slab,
	void **code_top,
	void **code_end);

/* Reserve a JIT slab. */
bool
rt_jit_slab_reserve(
	struct rt_env *env,
	size_t estimated_size);

/* Finish a JIT slab. */
void
rt_jit_slab_finish(
	struct rt_env *env,
	struct rt_jit_slab *slab,
	void *code_end);

/* Abandon a JIT slab. */
void
rt_jit_slab_abandon(
	struct rt_env *env,
	struct rt_jit_slab *slab);

/* Clear a JIT slab overflow. */
void
rt_jit_slab_clear_overflow(
	struct rt_env *env);

/* Commit all JIT slabs. */
bool
rt_jit_slab_commit_all(
	struct rt_env *env);

/* Free all JIT slabs. */
bool
rt_jit_slab_free_all(
	struct rt_env *env);

/* Return the per-VM JIT reservation. */
size_t
rt_jit_get_code_size(
	struct rt_env *env);

#if defined(NOCT_ARCH_X86_64) || defined(NOCT_ARCH_ARM64)
/*
 * Target-neutral scalar Packed-loop scanner.
 *
 * A backend may reserve registers only after this scanner has proved that the
 * region is call-free, has the canonical one-step latch, and addresses no
 * more than three Packed roots through aliases of the induction variable.
 * The hint remains optional: a rejected region is emitted by the ordinary
 * memory-canonical visitors.
 */
uint16_t
rt_jit_ploop_read_u16(const uint8_t *p);

uint32_t
rt_jit_ploop_read_u32(const uint8_t *p);

bool
rt_jit_ploop_reject(struct rt_jit_context *ctx, const char *reason);

bool
rt_jit_ploop_add_base(struct rt_jit_context *ctx, uint16_t base, int scale);

bool
rt_jit_ploop_is_index_alias(struct rt_jit_context *ctx, int tmp);

bool
rt_jit_ploop_index_alias_disp(struct rt_jit_context *ctx, int tmp, int32_t *disp);

void
rt_jit_ploop_remove_index_alias(struct rt_jit_context *ctx, int tmp);

bool
rt_jit_ploop_add_index_alias_disp(struct rt_jit_context *ctx, int tmp,
			       int32_t disp);

bool
rt_jit_ploop_add_index_alias(struct rt_jit_context *ctx, int tmp);

int
rt_jit_ploop_resolve_base(struct rt_jit_context *ctx, int tmp);

void
rt_jit_ploop_remove_base_alias(struct rt_jit_context *ctx, int tmp);

bool
rt_jit_ploop_set_base_alias(struct rt_jit_context *ctx, int dst, int src);

void
rt_jit_ploop_note_use(struct rt_jit_context *ctx, int tmp);

void
rt_jit_ploop_note_def(struct rt_jit_context *ctx, int tmp);

bool
rt_jit_ploop_has_loop_carried_scalar(struct rt_jit_context *ctx);

void
rt_jit_ploop_count_use(struct rt_jit_context *ctx, int tmp, bool address_only);

bool
rt_jit_scan_packed_loop(struct rt_jit_context *ctx, bool reject_loop_carried);

bool
rt_jit_ploop_current_access_disp(struct rt_jit_context *ctx, int32_t *disp);

bool
rt_jit_ploop_current_elided(struct rt_jit_context *ctx, uint32_t size);

/*
 * Return the next bytecode position that reads tmp, stopping at its
 * next definition.  Accepted PLOOP regions have a small, closed
 * opcode set, so a linear walk is simpler and deterministic; with
 * four/six physical GPRs it is also cheaper than maintaining a
 * general CFG liveness structure.
 */
uint32_t
rt_jit_ploop_next_use_lpc(struct rt_jit_context *ctx, int tmp, uint32_t from);
#endif /* PLOOP scanner backends */

/*
 * One bytecode byte is the smallest possible instruction, so bytecode_size
 * entries cover every instruction and delayed branch.  The extra PC entry
 * maps the end of the bytecode.  Tables are per function and impose no fixed
 * 2048-instruction ceiling.
 */
bool
rt_jit_context_init_tables(struct rt_jit_context *ctx);

/* Allocate scalar register-cache analysis storage only for a function that
 * actually contains an eligible PLOOP region. */
bool
rt_jit_context_init_regcache(struct rt_jit_context *ctx);

void
rt_jit_context_dispose_tables(struct rt_jit_context *ctx);

void
rt_jit_configure_simd(struct rt_jit_context *ctx, uint32_t detected,
		   const char *backend);

void
rt_jit_dump_standard_code(struct rt_jit_context *ctx, void *generated_end,
		       const char *backend);


/* Build a function with the standard JIT backend workflow. */
bool
rt_jit_build_standard(
	struct rt_env *env,
	struct rt_func *func,
	uint32_t detected_caps,
	const char *backend,
	bool (*visit_bytecode)(struct rt_jit_context *ctx),
	bool (*patch_branch)(struct rt_jit_context *ctx, int patch_index));


/*
 * Get an opcode.
 */
#define CONSUME_OPCODE(d)	if (!rt_jit_get_opcode(ctx, &d)) return false
static INLINE bool
rt_jit_get_opcode(
	struct rt_jit_context *ctx,
	uint8_t *opcode)
{
	if (ctx->lpc + 1 > ctx->func->bytecode_size) {
		rt_error(ctx->env, BROKEN_BYTECODE);
		*opcode = 0;
		return false;
	}

	*opcode = ctx->func->bytecode[ctx->lpc];

	ctx->lpc++;

	return true;
}

/*
 * Get an imm32 operand.
 */
#define CONSUME_IMM32(d)	if (!rt_jit_get_opr_imm32(ctx, &d)) return false
static INLINE bool
rt_jit_get_opr_imm32(
	struct rt_jit_context *ctx,
	uint32_t *d)
{
	if (ctx->lpc + 4 > ctx->func->bytecode_size) {
		rt_error(ctx->env, BROKEN_BYTECODE);
		*d = 0;
		return false;
	}

	*d = ((uint32_t)ctx->func->bytecode[ctx->lpc] << 24) |
	     (uint32_t)(ctx->func->bytecode[ctx->lpc + 1] << 16) |
	     (uint32_t)(ctx->func->bytecode[ctx->lpc + 2] << 8) |
	     (uint32_t)ctx->func->bytecode[ctx->lpc + 3];

	ctx->lpc += 4;

	return true;
}

/*
 * Get an imm64 operand.
 */
#define CONSUME_IMM64(d)	if (!rt_jit_get_opr_imm64(ctx, &d)) return false
static INLINE bool
rt_jit_get_opr_imm64(
	struct rt_jit_context *ctx,
	uint64_t *d)
{
	if (ctx->lpc + 8 > ctx->func->bytecode_size) {
		rt_error(ctx->env, BROKEN_BYTECODE);
		*d = 0;
		return false;
	}

	*d = ((uint64_t)ctx->func->bytecode[ctx->lpc + 0] << 56) |
	     ((uint64_t)ctx->func->bytecode[ctx->lpc + 1] << 48) |
	     ((uint64_t)ctx->func->bytecode[ctx->lpc + 2] << 40) |
	     ((uint64_t)ctx->func->bytecode[ctx->lpc + 3] << 32) |
	     ((uint64_t)ctx->func->bytecode[ctx->lpc + 4] << 24) |
	     ((uint64_t)ctx->func->bytecode[ctx->lpc + 5] << 16) |
	     ((uint64_t)ctx->func->bytecode[ctx->lpc + 6] << 8) |
             ((uint64_t)ctx->func->bytecode[ctx->lpc + 7]);

	ctx->lpc += 8;

	return true;
}

/*
 * Get an imm16 operand that represents tmpvar index.
 */
#define CONSUME_TMPVAR(d)	if (!rt_jit_get_opr_tmpvar(ctx, &d)) return false
static INLINE bool
rt_jit_get_opr_tmpvar(
	struct rt_jit_context *ctx,
	int *d)
{
	if (ctx->lpc + 2 > ctx->func->bytecode_size) {
		rt_error(ctx->env, BROKEN_BYTECODE);
		*d = 0;
		return false;
	}

	*d = (ctx->func->bytecode[ctx->lpc] << 8) |
	      ctx->func->bytecode[ctx->lpc + 1];
	if ((uint32_t)*d >= ctx->func->tmpvar_size) {
		rt_error(ctx->env, BROKEN_BYTECODE);
		return false;
	}

	ctx->lpc += 2;

	return true;
}

/*
 * Get an imm8 operand.
 */
#define CONSUME_IMM8(d)		if (!rt_jit_get_imm8(ctx, &d)) return false
static INLINE bool
rt_jit_get_imm8(
	struct rt_jit_context *ctx,
	int *imm8)
{
	if (ctx->lpc + 1 > ctx->func->bytecode_size) {
		rt_error(ctx->env, BROKEN_BYTECODE);
		*imm8 = 0;
		return false;
	}

	*imm8 = ctx->func->bytecode[ctx->lpc];

	ctx->lpc++;

	return true;
}

/* Consume the architecture-neutral scalar Packed-loop hint. */
bool
rt_jit_visit_ploop_hint_op(struct rt_jit_context *ctx);

/* Function-head metadata emitted after whole-function type aggregation. */
bool
rt_jit_visit_tmpvar_type_op(struct rt_jit_context *ctx);

/* Non-optimizing backends keep frame tags canonical and only need to consume
 * the explicit materialization boundary. */
bool
rt_jit_visit_materialize_type_metadata_op(struct rt_jit_context *ctx);

bool
rt_jit_tmp_has_fixed_primitive_type(struct rt_jit_context *ctx, int tmp, int type);

/*
 * Get a string operand.
 */
#define CONSUME_STRING(s,l,h)	if (!rt_jit_get_opr_string(ctx, &s, &l, &h)) return false
static INLINE bool
rt_jit_get_opr_string(
	struct rt_jit_context *ctx,
	const char **s,
	uint32_t *len,
	uint32_t *hash)
{
	if (ctx->lpc + 8 > ctx->func->bytecode_size) {
		rt_error(ctx->env, BROKEN_BYTECODE);
		*s = 0;
		return false;
	}

	*len = ((uint32_t)ctx->func->bytecode[ctx->lpc] << 24) |
		(uint32_t)(ctx->func->bytecode[ctx->lpc + 1] << 16) |
		(uint32_t)(ctx->func->bytecode[ctx->lpc + 2] << 8) |
		(uint32_t)ctx->func->bytecode[ctx->lpc + 3];

	*hash = ((uint32_t)ctx->func->bytecode[ctx->lpc + 4] << 24) |
		(uint32_t)(ctx->func->bytecode[ctx->lpc + 5] << 16) |
		(uint32_t)(ctx->func->bytecode[ctx->lpc + 6] << 8) |
		(uint32_t)ctx->func->bytecode[ctx->lpc + 7];

	if (ctx->lpc + 8 + *len > ctx->func->bytecode_size) {
		rt_error(ctx->env, BROKEN_BYTECODE);
		*s = NULL;
		return false;
	}

	*s = (const char *)&ctx->func->bytecode[ctx->lpc + 8];

	ctx->lpc += 8 + *len;

	return true;
}

#endif /* defined(NOCT_USE_JIT) */

#endif
