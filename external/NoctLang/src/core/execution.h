/* -*- coding: utf-8; tab-width: 8; indent-tabs-mode: t; -*- */

/*
 * Copyright (c) 2025, Awe Morris. All rights reserved.
 */

/*
 * Execution Helpers (runtime.c)
 */

#ifndef NOCT_EXECUTION_H
#define NOCT_EXECUTION_H

#include <noct/c89compat.h>

bool
rt_assign_helper(
	struct rt_env *rt,
	int dst,
	int src);

bool
rt_add_helper(
	struct rt_env *rt,
	int dst,
	int src1,
	int src2);

bool
rt_sub_helper(
	struct rt_env *rt,
	int dst,
	int src1,
	int src2);

bool
rt_mul_helper(
	struct rt_env *rt,
	int dst,
	int src1,
	int src2);

bool
rt_div_helper(
	struct rt_env *rt,
	int dst,
	int src1,
	int src2);

bool
rt_mod_helper(
	struct rt_env *rt,
	int dst,
	int src1,
	int src2);

bool
rt_and_helper(
	struct rt_env *rt,
	int dst,
	int src1,
	int src2);

bool
rt_or_helper(
	struct rt_env *rt,
	int dst,
	int src1,
	int src2);

bool
rt_xor_helper(
	struct rt_env *rt,
	int dst,
	int src1,
	int src2);

bool
rt_shl_helper(
	struct rt_env *rt,
	int dst,
	int src1,
	int src2);

bool
rt_shr_helper(
	struct rt_env *rt,
	int dst,
	int src1,
	int src2);

bool
rt_neg_helper(
	struct rt_env *rt,
	int dst,
	int src);

bool
rt_not_helper(
	struct rt_env *rt,
	int dst,
	int src);

bool
rt_lt_helper(
	struct rt_env *rt,
	int dst,
	int src1,
	int src2);

bool
rt_lte_helper(
	struct rt_env *rt,
	int dst,
	int src1,
	int src2);

bool
rt_eq_helper(
	struct rt_env *rt,
	int dst,
	int src1,
	int src2);

bool
rt_neq_helper(
	struct rt_env *rt,
	int dst,
	int src1,
	int src2);

bool
rt_gte_helper(
	struct rt_env *rt,
	int dst,
	int src1,
	int src2);

bool
rt_gt_helper(
	struct rt_env *rt,
	int dst,
	int src1,
	int src2);

bool
rt_storearray_helper(
	struct rt_env *rt,
	int arr,
	int subscr,
	int val);

bool
rt_loadarray_helper(
	struct rt_env *rt,
	int dst,
	int arr,
	int subscr);

bool
rt_len_helper(
	struct rt_env *rt,
	int dst,
	int src);

bool
rt_getdictkeybyindex_helper(
	struct rt_env *rt,
	int dst,
	int dict,
	int subscr);

bool
rt_getdictvalbyindex_helper(
	struct rt_env *rt,
	int dst,
	int dict,
	int subscr);

bool
rt_loadsymbol_helper(
	struct rt_env *rt,
	int dst,
	const char *symbol,
	uint32_t symbol_len,
	uint32_t symbol_hash);

bool
rt_storesymbol_helper(
	struct rt_env *rt,
	const char *symbol,
	uint32_t symbol_len,
	uint32_t symbol_hash,
	int src);

bool
rt_loaddot_helper(
	struct rt_env *rt,
	int dst,
	int dict,
	const char *field,
	uint32_t field_len,
	uint32_t field_hash);

bool
rt_storedot_helper(
	struct rt_env *rt,
	int dict,
	const char *field,
	uint32_t field_len,
	uint32_t field_hash,
	int src);

bool
rt_call_helper(
	struct rt_env *rt,
	int dst,
	int func,
	int arg_count,
	int *arg);

bool
rt_thiscall_helper(
	struct rt_env *rt,
	int dst,
	int obj,
	const char *name,
	uint32_t name_len,
	uint32_t name_hash,
	int arg_count,
	int *arg);

#endif
