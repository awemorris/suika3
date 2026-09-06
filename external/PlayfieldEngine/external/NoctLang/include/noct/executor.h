/* -*- coding: utf-8; tab-width: 8; indent-tabs-mode: t; -*- */

/*
 * Noct Programming Language
 * Copyright (c) 2025, 2026, Awe Morris
 */

/*
 * Noct Virtual Machine Executor Interface
 */

#ifndef NOCT_AOT_H
#define NOCT_AOT_H

#include <noct/noct.h>

#if defined(NOCT_AOT_INTERNAL)
/*
 * Head of the full struct rt_frame in src/core/runtime.h.
 */
struct rt_frame {
	/* tmpvar pointer. */
	struct rt_value *tmpvar;
};

/*
 * Head of the full struct rt_env in src/core/runtime.h.
 */
struct rt_env {
	/* Stack pointer. */
	struct rt_frame *frame;
};
#endif /* defined(NOCT_AOT_INTERNAL) */

/*
 * Entry point defined by a C source generated with "noct --ansic".
 * Registers all translated functions to the given environment.
 */
bool
init_aot_code(
	NoctEnv *env);

/*
 * AOT Execution Helpers
 *
 * Some exotic compilers for x86 including Watcom utilize registers to
 * pass function arguments. However, our JIT-generated code for x86
 * passes function arguments via the stack. To bridge this gap, we use
 * the CDECL keyword in these helpers to be properly called from the
 * JIT-generated code.
 */

NOCT_DLL
bool
CDECL
noct_ex_make_string_with_hash(
	NoctEnv *env,
	NoctValue *val,
	const char *data,
	size_t len,
	uint32_t hash);

NOCT_DLL
bool
CDECL
noct_ex_make_empty_array(
	NoctEnv *env,
	NoctValue *val);

NOCT_DLL
bool
CDECL
noct_ex_make_empty_dict(
	NoctEnv *env,
	NoctValue *val);

NOCT_DLL
bool
CDECL
noct_ex_assign_helper(
	NoctEnv *rt,
	int dst,
	int src);

NOCT_DLL
bool
CDECL
noct_ex_add_helper(
	NoctEnv *rt,
	int dst,
	int src1,
	int src2);

NOCT_DLL
bool
CDECL
noct_ex_sub_helper(
	NoctEnv *rt,
	int dst,
	int src1,
	int src2);

NOCT_DLL
bool
CDECL
noct_ex_mul_helper(
	NoctEnv *rt,
	int dst,
	int src1,
	int src2);

NOCT_DLL
bool
CDECL
noct_ex_div_helper(
	NoctEnv *rt,
	int dst,
	int src1,
	int src2);

NOCT_DLL
bool
CDECL
noct_ex_mod_helper(
	NoctEnv *rt,
	int dst,
	int src1,
	int src2);

NOCT_DLL
bool
CDECL
noct_ex_and_helper(
	NoctEnv *rt,
	int dst,
	int src1,
	int src2);

NOCT_DLL
bool
CDECL
noct_ex_or_helper(
	NoctEnv *rt,
	int dst,
	int src1,
	int src2);

NOCT_DLL
bool
CDECL
noct_ex_xor_helper(
	NoctEnv *rt,
	int dst,
	int src1,
	int src2);

NOCT_DLL
bool
CDECL
noct_ex_shl_helper(
	NoctEnv *rt,
	int dst,
	int src1,
	int src2);

NOCT_DLL
bool
CDECL
noct_ex_shr_helper(
	NoctEnv *rt,
	int dst,
	int src1,
	int src2);

NOCT_DLL
bool
CDECL
noct_ex_neg_helper(
	NoctEnv *rt,
	int dst,
	int src);

NOCT_DLL
bool
CDECL
noct_ex_not_helper(
	NoctEnv *rt,
	int dst,
	int src);

NOCT_DLL
bool
CDECL
noct_ex_lt_helper(
	NoctEnv *rt,
	int dst,
	int src1,
	int src2);

NOCT_DLL
bool
CDECL
noct_ex_lte_helper(
	NoctEnv *rt,
	int dst,
	int src1,
	int src2);

NOCT_DLL
bool
CDECL
noct_ex_eq_helper(
	NoctEnv *rt,
	int dst,
	int src1,
	int src2);

NOCT_DLL
bool
CDECL
noct_ex_neq_helper(
	NoctEnv *rt,
	int dst,
	int src1,
	int src2);

NOCT_DLL
bool
CDECL
noct_ex_gte_helper(
	NoctEnv *rt,
	int dst,
	int src1,
	int src2);

NOCT_DLL
bool
CDECL
noct_ex_gt_helper(
	NoctEnv *rt,
	int dst,
	int src1,
	int src2);

NOCT_DLL
bool
CDECL
noct_ex_storearray_helper(
	NoctEnv *rt,
	int arr,
	int subscr,
	int val);

NOCT_DLL
bool
CDECL
noct_ex_loadarray_helper(
	NoctEnv *rt,
	int dst,
	int arr,
	int subscr);

NOCT_DLL
bool
CDECL
noct_ex_len_helper(
	NoctEnv *rt,
	int dst,
	int src);

NOCT_DLL
bool
CDECL
noct_ex_getdictkeybyindex_helper(
	NoctEnv *rt,
	int dst,
	int dict,
	int subscr);

NOCT_DLL
bool
CDECL
noct_ex_getdictvalbyindex_helper(
	NoctEnv *rt,
	int dst,
	int dict,
	int subscr);

NOCT_DLL
bool
CDECL
noct_ex_loadsymbol_helper(
	NoctEnv *rt,
	int dst,
	const char *symbol,
	uint32_t symbol_len,
	uint32_t symbol_hash);

NOCT_DLL
bool
CDECL
noct_ex_storesymbol_helper(
	NoctEnv *rt,
	const char *symbol,
	uint32_t symbol_len,
	uint32_t symbol_hash,
	int src);

NOCT_DLL
bool
CDECL
noct_ex_loaddot_helper(
	NoctEnv *rt,
	int dst,
	int dict,
	const char *field,
	uint32_t field_len,
	uint32_t field_hash);

NOCT_DLL
bool
CDECL
noct_ex_storedot_helper(
	NoctEnv *rt,
	int dict,
	const char *field,
	uint32_t field_len,
	uint32_t field_hash,
	int src);

NOCT_DLL
bool
CDECL
noct_ex_call_helper(
	NoctEnv *rt,
	int dst,
	int func,
	int arg_count,
	int *arg);

NOCT_DLL
bool
CDECL
noct_ex_thiscall_helper(
	NoctEnv *rt,
	int dst,
	int obj,
	const char *name,
	uint32_t name_len,
	uint32_t name_hash,
	int arg_count,
	int *arg);

NOCT_DLL
bool
CDECL
noct_ex_safepoint_helper(
	NoctEnv *rt);

/*
 * For ABCE Optimized Code
 */

NOCT_DLL
bool
CDECL
noct_ex_pbase_helper(
	NoctEnv *rt,
	int dst,
	int src);

NOCT_DLL
bool
CDECL
noct_ex_pcheck_helper(
	NoctEnv *rt,
	int dst,
	int src,
	int packed_type);

NOCT_DLL
bool
CDECL
noct_ex_typeis_helper(
	NoctEnv *rt,
	int dst,
	int src,
	int value_type);

NOCT_DLL
bool
CDECL
noct_ex_plen_helper(
	NoctEnv *rt,
	int dst,
	int src);

NOCT_DLL
bool
CDECL
noct_ex_pload8u_helper(
	NoctEnv *rt,
	int dst,
	int base,
	int ofs);

NOCT_DLL
bool
CDECL
noct_ex_pstore8_helper(
	NoctEnv *rt,
	int base,
	int ofs,
	int src);

NOCT_DLL
bool
CDECL
noct_ex_pload8s_helper(
	NoctEnv *rt,
	int dst,
	int base,
	int ofs);

NOCT_DLL
bool
CDECL
noct_ex_pload16u_helper(
	NoctEnv *rt,
	int dst,
	int base,
	int ofs);

NOCT_DLL
bool
CDECL
noct_ex_pload16s_helper(
	NoctEnv *rt,
	int dst,
	int base,
	int ofs);

NOCT_DLL
bool
CDECL
noct_ex_pload32_helper(
	NoctEnv *rt,
	int dst,
	int base,
	int ofs);

NOCT_DLL
bool
CDECL
noct_ex_pload64_helper(
	NoctEnv *rt,
	int dst,
	int base,
	int ofs);

NOCT_DLL
bool
CDECL
noct_ex_pstore16_helper(
	NoctEnv *rt,
	int base,
	int ofs,
	int src);

NOCT_DLL
bool
CDECL
noct_ex_pstore32_helper(
	NoctEnv *rt,
	int base,
	int ofs,
	int src);

NOCT_DLL
bool
CDECL
noct_ex_pstore64_helper(
	NoctEnv *rt,
	int base,
	int ofs,
	int src);

/*
 * For Type Optimized Code
 */

NOCT_DLL
bool
CDECL
noct_ex_checktype_helper(
	NoctEnv *rt,
	int slot,
	int value_type);

NOCT_DLL
int
CDECL
noct_ex_condition_helper(
	NoctEnv *rt,
	int slot);

NOCT_DLL
bool
CDECL
noct_ex_iadd_helper(
	NoctEnv *rt,
	int dst,
	int src1,
	int src2);

NOCT_DLL
bool
CDECL
noct_ex_isub_helper(
	NoctEnv *rt,
	int dst,
	int src1,
	int src2);

NOCT_DLL
bool
CDECL
noct_ex_imul_helper(
	NoctEnv *rt,
	int dst,
	int src1,
	int src2);

NOCT_DLL
bool
CDECL
noct_ex_idiv_helper(
	NoctEnv *rt,
	int dst,
	int src1,
	int src2);

NOCT_DLL
bool
CDECL
noct_ex_imod_helper(
	NoctEnv *rt,
	int dst,
	int src1,
	int src2);

NOCT_DLL
bool
CDECL
noct_ex_iand_helper(
	NoctEnv *rt,
	int dst,
	int src1,
	int src2);

NOCT_DLL
bool
CDECL
noct_ex_ior_helper(
	NoctEnv *rt,
	int dst,
	int src1,
	int src2);

NOCT_DLL
bool
CDECL
noct_ex_ixor_helper(
	NoctEnv *rt,
	int dst,
	int src1,
	int src2);

NOCT_DLL
bool
CDECL
noct_ex_ishl_helper(
	NoctEnv *rt,
	int dst,
	int src1,
	int src2);

NOCT_DLL
bool
CDECL
noct_ex_ishr_helper(
	NoctEnv *rt,
	int dst,
	int src1,
	int src2);

NOCT_DLL
bool
CDECL
noct_ex_ilt_helper(
	NoctEnv *rt,
	int dst,
	int src1,
	int src2);

NOCT_DLL
bool
CDECL
noct_ex_ilte_helper(
	NoctEnv *rt,
	int dst,
	int src1,
	int src2);

NOCT_DLL
bool
CDECL
noct_ex_igt_helper(
	NoctEnv *rt,
	int dst,
	int src1,
	int src2);

NOCT_DLL
bool
CDECL
noct_ex_igte_helper(
	NoctEnv *rt,
	int dst,
	int src1,
	int src2);

NOCT_DLL
bool
CDECL
noct_ex_fadd_helper(
	NoctEnv *rt,
	int dst,
	int src1,
	int src2);

NOCT_DLL
bool
CDECL
noct_ex_fsub_helper(
	NoctEnv *rt,
	int dst,
	int src1,
	int src2);

NOCT_DLL
bool
CDECL
noct_ex_fmul_helper(
	NoctEnv *rt,
	int dst,
	int src1,
	int src2);

NOCT_DLL
bool
CDECL
noct_ex_fdiv_helper(
	NoctEnv *rt,
	int dst,
	int src1,
	int src2);

NOCT_DLL
bool
CDECL
noct_ex_flt_helper(
	NoctEnv *rt,
	int dst,
	int src1,
	int src2);

NOCT_DLL
bool
CDECL
noct_ex_flte_helper(
	NoctEnv *rt,
	int dst,
	int src1,
	int src2);

NOCT_DLL
bool
CDECL
noct_ex_fgt_helper(
	NoctEnv *rt,
	int dst,
	int src1,
	int src2);

NOCT_DLL
bool
CDECL
noct_ex_fgte_helper(
	NoctEnv *rt,
	int dst,
	int src1,
	int src2);

NOCT_DLL
bool
CDECL
noct_ex_vloadi32x4_helper(
	NoctEnv *rt,
	int dst,
	int src1,
	int src2);

NOCT_DLL
bool
CDECL
noct_ex_vstorei32x4_helper(
	NoctEnv *rt,
	int dst,
	int src1,
	int src2);

NOCT_DLL
bool
CDECL
noct_ex_vsplati32_helper(
	NoctEnv *rt,
	int dst,
	int src1,
	int src2);

NOCT_DLL
bool
CDECL
noct_ex_vgetlanei32_helper(
	NoctEnv *rt,
	int dst,
	int src1,
	int src2);

NOCT_DLL
bool
CDECL
noct_ex_vmov128_helper(
	NoctEnv *rt,
	int dst,
	int src1,
	int src2);

NOCT_DLL
bool
CDECL
noct_ex_vaddi32x4_helper(
	NoctEnv *rt,
	int dst,
	int src1,
	int src2);

NOCT_DLL
bool
CDECL
noct_ex_vsubi32x4_helper(
	NoctEnv *rt,
	int dst,
	int src1,
	int src2);

NOCT_DLL
bool
CDECL
noct_ex_vmuli32x4_helper(
	NoctEnv *rt,
	int dst,
	int src1,
	int src2);

NOCT_DLL
bool
CDECL
noct_ex_vand128_helper(
	NoctEnv *rt,
	int dst,
	int src1,
	int src2);

NOCT_DLL
bool
CDECL
noct_ex_vor128_helper(
	NoctEnv *rt,
	int dst,
	int src1,
	int src2);

NOCT_DLL
bool
CDECL
noct_ex_vxor128_helper(
	NoctEnv *rt,
	int dst,
	int src1,
	int src2);

NOCT_DLL
bool
CDECL
noct_ex_vshli32x4_helper(
	NoctEnv *rt,
	int dst,
	int src1,
	int src2);

NOCT_DLL
bool
CDECL
noct_ex_vshri32x4_helper(
	NoctEnv *rt,
	int dst,
	int src1,
	int src2);

NOCT_DLL
bool
CDECL
noct_ex_vloadf32x4_helper(
	NoctEnv *rt,
	int dst,
	int src1,
	int src2);

NOCT_DLL
bool
CDECL
noct_ex_vstoref32x4_helper(
	NoctEnv *rt,
	int dst,
	int src1,
	int src2);

NOCT_DLL
bool
CDECL
noct_ex_vsplatf32_helper(
	NoctEnv *rt,
	int dst,
	int src1,
	int src2);

NOCT_DLL
bool
CDECL
noct_ex_vgetlanef32_helper(
	NoctEnv *rt,
	int dst,
	int src1,
	int src2);

NOCT_DLL
bool
CDECL
noct_ex_vaddf32x4_helper(
	NoctEnv *rt,
	int dst,
	int src1,
	int src2);

NOCT_DLL
bool
CDECL
noct_ex_vsubf32x4_helper(
	NoctEnv *rt,
	int dst,
	int src1,
	int src2);

NOCT_DLL
bool
CDECL
noct_ex_vmulf32x4_helper(
	NoctEnv *rt,
	int dst,
	int src1,
	int src2);

NOCT_DLL
bool
CDECL
noct_ex_vdivf32x4_helper(
	NoctEnv *rt,
	int dst,
	int src1,
	int src2);

NOCT_DLL
bool
CDECL
noct_ex_vcvti32f32x4_helper(
	NoctEnv *rt,
	int dst,
	int src1,
	int src2);

NOCT_DLL
bool
CDECL
noct_ex_vcvtf32i32x4_helper(
	NoctEnv *rt,
	int dst,
	int src1,
	int src2);

NOCT_DLL
bool
CDECL
noct_ex_vori32x4i_helper(
	NoctEnv *rt,
	int dst,
	int src1,
	int src2);

NOCT_DLL
bool
CDECL
noct_ex_vfmaf32x4_helper(
	NoctEnv *rt,
	int dst,
	int src1,
	int src2);

NOCT_DLL
bool
CDECL
noct_ex_vcmpi32x4_helper(
	NoctEnv *rt,
	int dst,
	int src1,
	int src2);

NOCT_DLL
bool
CDECL
noct_ex_vcmpf32x4_helper(
	NoctEnv *rt,
	int dst,
	int src1,
	int src2);

NOCT_DLL
bool
CDECL
noct_ex_vselect128_helper(
	NoctEnv *rt,
	int dst,
	int src1,
	int src2);

NOCT_DLL
bool
CDECL
noct_ex_vmaskstorei32x4_helper(
	NoctEnv *rt,
	int dst,
	int src1,
	int src2);

NOCT_DLL
bool
CDECL
noct_ex_vinductf32x4_helper(
	NoctEnv *rt,
	int dst,
	int src1,
	int src2);

NOCT_DLL
bool
CDECL
noct_ex_vgatheri32x4_checked_helper(
	NoctEnv *rt,
	int dst,
	int src1,
	int src2);

NOCT_DLL
bool
CDECL
noct_ex_vmins32x4_helper(
	NoctEnv *rt,
	int dst,
	int src1,
	int src2);

NOCT_DLL
bool
CDECL
noct_ex_vmaxs32x4_helper(
	NoctEnv *rt,
	int dst,
	int src1,
	int src2);

NOCT_DLL
bool
CDECL
noct_ex_ploadf32_helper(
	NoctEnv *rt,
	int dst,
	int src1,
	int src2);

NOCT_DLL
bool
CDECL
noct_ex_pstoref32_helper(
	NoctEnv *rt,
	int dst,
	int src1,
	int src2);

#endif
