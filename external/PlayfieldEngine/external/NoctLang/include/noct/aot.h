/* -*- coding: utf-8; tab-width: 8; indent-tabs-mode: t; -*- */

/*
 * Noct Programming Language
 * Copyright (c) 2025, 2026, Awe Morris
 */

/*
 * Noct Virtual Machine AOT Interface
 */

#ifndef NOCT_AOT_H
#define NOCT_AOT_H

#include <noct/noct.h>

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

#endif
