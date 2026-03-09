/* -*- coding: utf-8; tab-width: 8; indent-tabs-mode: t; -*- */

/*
 * Copyright (c) 2025, Awe Morris. All rights reserved.
 */

/*
 * Execution Helpers
 */

#include <noct/c89compat.h>
#include "runtime.h"
#include "execution.h"
#include "intrinsics.h"
#include "hash.h"

#include <stdio.h>
#include <string.h>
#include <assert.h>

/*
 * Add helper.
 */
bool
rt_add_helper(
	struct rt_env *env,
	int dst,
	int src1,
	int src2)
{
	struct rt_value *dst_val;
	struct rt_value *src1_val;
	struct rt_value *src2_val;

	dst_val = &env->frame->tmpvar[dst];
	src1_val = &env->frame->tmpvar[src1];
	src2_val = &env->frame->tmpvar[src2];

	switch (src1_val->type) {
	case NOCT_VALUE_INT:
		switch (src2_val->type) {
		case NOCT_VALUE_INT:
			dst_val->type = NOCT_VALUE_INT;
			dst_val->val.i = src1_val->val.i + src2_val->val.i;
			break;
		case NOCT_VALUE_FLOAT:
			dst_val->type = NOCT_VALUE_FLOAT;
			dst_val->val.f = (float)src1_val->val.i + src2_val->val.f;
			break;
		case NOCT_VALUE_STRING:
			if (!noct_make_string_format(env, dst_val, "%d%s", src1_val->val.i, src2_val->val.str->data))
				return false;
			break;
		default:
			rt_error(env, N_TR("Value is not a number or a string."));
			return false;
		}
		break;
	case NOCT_VALUE_FLOAT:
		switch (src2_val->type) {
		case NOCT_VALUE_INT:
			dst_val->type = NOCT_VALUE_FLOAT;
			dst_val->val.f = src1_val->val.f + (float)src2_val->val.i;
			break;
		case NOCT_VALUE_FLOAT:
			dst_val->type = NOCT_VALUE_FLOAT;
			dst_val->val.f = src1_val->val.f + src2_val->val.f;
			break;
		case NOCT_VALUE_STRING:
			if (!noct_make_string_format(env, dst_val, "%f%s", src1_val->val.f, src2_val->val.str->data))
				return false;
			break;
		default:
			rt_error(env, N_TR("Value is not a number or a string."));
			return false;
		}
		break;
	case NOCT_VALUE_STRING:
		switch (src2_val->type) {
		case NOCT_VALUE_INT:
			if (!noct_make_string_format(env, dst_val, "%s%d", src1_val->val.str->data, src2_val->val.i))
				return false;
			break;
		case NOCT_VALUE_FLOAT:
			if (!noct_make_string_format(env, dst_val, "%s%f", src1_val->val.str->data, src2_val->val.f))
				return false;
			break;
		case NOCT_VALUE_STRING:
			if (!noct_make_string_format(env, dst_val, "%s%s", src1_val->val.str->data, src2_val->val.str->data))
				return false;
			break;
		default:
			rt_error(env, N_TR("Value is not a number or a string."));
			return false;
		}
		break;
	default:
		rt_error(env, N_TR("Value is not a number or a string."));
		return false;
	}

	return true;
}

/*
 * Subtract helper.
 */
bool
rt_sub_helper(
	struct rt_env *env,
	int dst,
	int src1,
	int src2)
{
	struct rt_value *dst_val;
	struct rt_value *src1_val;
	struct rt_value *src2_val;

	dst_val = &env->frame->tmpvar[dst];
	src1_val = &env->frame->tmpvar[src1];
	src2_val = &env->frame->tmpvar[src2];

	switch (src1_val->type) {
	case NOCT_VALUE_INT:
		switch (src2_val->type) {
		case NOCT_VALUE_INT:
			dst_val->type = NOCT_VALUE_INT;
			dst_val->val.i = src1_val->val.i - src2_val->val.i;
			break;
		case NOCT_VALUE_FLOAT:
			dst_val->type = NOCT_VALUE_FLOAT;
			dst_val->val.f = (float)src1_val->val.i - src2_val->val.f;
			break;
		default:
			rt_error(env, N_TR("Value is not a number."));
			return false;
		}
		break;
	case NOCT_VALUE_FLOAT:
		switch (src2_val->type) {
		case NOCT_VALUE_INT:
			dst_val->type = NOCT_VALUE_FLOAT;
			dst_val->val.f = src1_val->val.f - (float)src2_val->val.i;
			break;
		case NOCT_VALUE_FLOAT:
			dst_val->type = NOCT_VALUE_FLOAT;
			dst_val->val.f = src1_val->val.f - src2_val->val.f;
			break;
		default:
			rt_error(env, N_TR("Value is not a number."));
			return false;
		}
		break;
	default:
		rt_error(env, N_TR("Value is not a number."));
		return false;
	}

	return true;
}

/*
 * Multiply helper.
 */
bool
rt_mul_helper(
	struct rt_env *env,
	int dst,
	int src1,
	int src2)
{
	struct rt_value *dst_val;
	struct rt_value *src1_val;
	struct rt_value *src2_val;

	dst_val = &env->frame->tmpvar[dst];
	src1_val = &env->frame->tmpvar[src1];
	src2_val = &env->frame->tmpvar[src2];

	switch (src1_val->type) {
	case NOCT_VALUE_INT:
		switch (src2_val->type) {
		case NOCT_VALUE_INT:
			dst_val->type = NOCT_VALUE_INT;
			dst_val->val.i = src1_val->val.i * src2_val->val.i;
			break;
		case NOCT_VALUE_FLOAT:
			dst_val->type = NOCT_VALUE_FLOAT;
			dst_val->val.f = (float)src1_val->val.i * src2_val->val.f;
			break;
		default:
			rt_error(env, N_TR("Value is not a number."));
			return false;
		}
		break;
	case NOCT_VALUE_FLOAT:
		switch (src2_val->type) {
		case NOCT_VALUE_INT:
			dst_val->type = NOCT_VALUE_FLOAT;
			dst_val->val.f = src1_val->val.f * (float)src2_val->val.i;
			break;
		case NOCT_VALUE_FLOAT:
			dst_val->type = NOCT_VALUE_FLOAT;
			dst_val->val.f = src1_val->val.f * src2_val->val.f;
			break;
		default:
			rt_error(env, N_TR("Value is not a number."));
			return false;
		}
		break;
	default:
		rt_error(env, N_TR("Value is not a number."));
		return false;
	}

	return true;
}

/*
 * Multiply helper.
 */
bool
rt_div_helper(
	struct rt_env *env,
	int dst,
	int src1,
	int src2)
{
	struct rt_value *dst_val;
	struct rt_value *src1_val;
	struct rt_value *src2_val;

	dst_val = &env->frame->tmpvar[dst];
	src1_val = &env->frame->tmpvar[src1];
	src2_val = &env->frame->tmpvar[src2];

	switch (src1_val->type) {
	case NOCT_VALUE_INT:
		switch (src2_val->type) {
		case NOCT_VALUE_INT:
			if (src2_val->val.i == 0) {
				rt_error(env, N_TR("Division by zero."));
				return false;
			}
			dst_val->type = NOCT_VALUE_INT;
			dst_val->val.i = src1_val->val.i / src2_val->val.i;
			break;
		case NOCT_VALUE_FLOAT:
			if (src2_val->val.f == 0) {
				rt_error(env, N_TR("Division by zero."));
				return false;
			}
			dst_val->type = NOCT_VALUE_FLOAT;
			dst_val->val.f = (float)src1_val->val.i / src2_val->val.f;
			break;
		default:
			rt_error(env, N_TR("Value is not a number."));
			return false;
		}
		break;
	case NOCT_VALUE_FLOAT:
		switch (src2_val->type) {
		case NOCT_VALUE_INT:
			if (src2_val->val.i == 0) {
				rt_error(env, N_TR("Division by zero."));
				return false;
			}
			dst_val->type = NOCT_VALUE_FLOAT;
			dst_val->val.f = src1_val->val.f / (float)src2_val->val.i;
			break;
		case NOCT_VALUE_FLOAT:
			if (src2_val->val.f == 0) {
				rt_error(env, N_TR("Division by zero."));
				return false;
			}
			dst_val->type = NOCT_VALUE_FLOAT;
			dst_val->val.f = src1_val->val.f / src2_val->val.f;
			break;
		default:
			rt_error(env, N_TR("Value is not a number."));
			return false;
		}
		break;
	default:
		rt_error(env, N_TR("Value is not a number."));
		return false;
	}

	return true;
}

/*
 * MOD helper. (modulo)
 */
bool
rt_mod_helper(
	struct rt_env *env,
	int dst,
	int src1,
	int src2)
{
	struct rt_value *dst_val;
	struct rt_value *src1_val;
	struct rt_value *src2_val;

	dst_val = &env->frame->tmpvar[dst];
	src1_val = &env->frame->tmpvar[src1];
	src2_val = &env->frame->tmpvar[src2];

	switch (src1_val->type) {
	case NOCT_VALUE_INT:
		switch (src2_val->type) {
		case NOCT_VALUE_INT:
			dst_val->type = NOCT_VALUE_INT;
			dst_val->val.i = src1_val->val.i % src2_val->val.i;
			break;
		default:
			rt_error(env, N_TR("Value is not an integer."));
			return false;
		}
		break;
	default:
		rt_error(env, N_TR("Value is not an integer."));
		return false;
	}

	return true;
}

/*
 * AND helper.
 */
bool
rt_and_helper(
	struct rt_env *env,
	int dst,
	int src1,
	int src2)
{
	struct rt_value *dst_val;
	struct rt_value *src1_val;
	struct rt_value *src2_val;

	dst_val = &env->frame->tmpvar[dst];
	src1_val = &env->frame->tmpvar[src1];
	src2_val = &env->frame->tmpvar[src2];

	switch (src1_val->type) {
	case NOCT_VALUE_INT:
		switch (src2_val->type) {
		case NOCT_VALUE_INT:
			dst_val->type = NOCT_VALUE_INT;
			dst_val->val.i = src1_val->val.i & src2_val->val.i;
			break;
		default:
			rt_error(env, N_TR("Value is not an integer."));
			return false;
		}
		break;
	default:
		rt_error(env, N_TR("Value is not an integer."));
		return false;
	}

	return true;
}

/*
 * OR helper.
 */
bool
rt_or_helper(
	struct rt_env *env,
	int dst,
	int src1,
	int src2)
{
	struct rt_value *dst_val;
	struct rt_value *src1_val;
	struct rt_value *src2_val;

	dst_val = &env->frame->tmpvar[dst];
	src1_val = &env->frame->tmpvar[src1];
	src2_val = &env->frame->tmpvar[src2];

	switch (src1_val->type) {
	case NOCT_VALUE_INT:
		switch (src2_val->type) {
		case NOCT_VALUE_INT:
			dst_val->type = NOCT_VALUE_INT;
			dst_val->val.i = src1_val->val.i | src2_val->val.i;
			break;
		default:
			rt_error(env, N_TR("Value is not an integer."));
			return false;
		}
		break;
	default:
		rt_error(env, N_TR("Value is not an integer."));
		return false;
	}

	return true;
}

/*
 * XOR helper.
 */
bool
rt_xor_helper(
	struct rt_env *env,
	int dst,
	int src1,
	int src2)
{
	struct rt_value *dst_val;
	struct rt_value *src1_val;
	struct rt_value *src2_val;

	dst_val = &env->frame->tmpvar[dst];
	src1_val = &env->frame->tmpvar[src1];
	src2_val = &env->frame->tmpvar[src2];

	switch (src1_val->type) {
	case NOCT_VALUE_INT:
		switch (src2_val->type) {
		case NOCT_VALUE_INT:
			dst_val->type = NOCT_VALUE_INT;
			dst_val->val.i = src1_val->val.i ^ src2_val->val.i;
			break;
		default:
			rt_error(env, N_TR("Value is not an integer."));
			return false;
		}
		break;
	default:
		rt_error(env, N_TR("Value is not an integer."));
		return false;
	}

	return true;
}

/*
 * SHL helper.
 */
bool
rt_shl_helper(
	struct rt_env *env,
	int dst,
	int src1,
	int src2)
{
	struct rt_value *dst_val;
	struct rt_value *src1_val;
	struct rt_value *src2_val;

	dst_val = &env->frame->tmpvar[dst];
	src1_val = &env->frame->tmpvar[src1];
	src2_val = &env->frame->tmpvar[src2];

	switch (src1_val->type) {
	case NOCT_VALUE_INT:
		switch (src2_val->type) {
		case NOCT_VALUE_INT:
			dst_val->type = NOCT_VALUE_INT;
			dst_val->val.i = src1_val->val.i << src2_val->val.i;
			break;
		default:
			rt_error(env, N_TR("Value is not an integer."));
			return false;
		}
		break;
	default:
		rt_error(env, N_TR("Value is not an integer."));
		return false;
	}

	return true;
}

/*
 * SHR helper.
 */
bool
rt_shr_helper(
	struct rt_env *env,
	int dst,
	int src1,
	int src2)
{
	struct rt_value *dst_val;
	struct rt_value *src1_val;
	struct rt_value *src2_val;

	dst_val = &env->frame->tmpvar[dst];
	src1_val = &env->frame->tmpvar[src1];
	src2_val = &env->frame->tmpvar[src2];

	switch (src1_val->type) {
	case NOCT_VALUE_INT:
		switch (src2_val->type) {
		case NOCT_VALUE_INT:
			dst_val->type = NOCT_VALUE_INT;
			dst_val->val.i = src1_val->val.i >> src2_val->val.i;
			break;
		default:
			rt_error(env, N_TR("Value is not an integer."));
			return false;
		}
		break;
	default:
		rt_error(env, N_TR("Value is not an integer."));
		return false;
	}

	return true;
}

/*
 * NEG helper.
 */
bool
rt_neg_helper(
	struct rt_env *env,
	int dst,
	int src)
{
	struct rt_value *dst_val;
	struct rt_value *src_val;

	dst_val = &env->frame->tmpvar[dst];
	src_val = &env->frame->tmpvar[src];

	switch (src_val->type) {
	case NOCT_VALUE_INT:
		dst_val->type = NOCT_VALUE_INT;
		dst_val->val.i = -src_val->val.i;
		break;
	case NOCT_VALUE_FLOAT:
		dst_val->type = NOCT_VALUE_FLOAT;
		dst_val->val.f = -src_val->val.f;
		break;
	default:
		rt_error(env, N_TR("Value is not a number."));
		return false;
	}

	return true;
}

/*
 * NEG helper.
 */
bool
rt_not_helper(
	struct rt_env *env,
	int dst,
	int src)
{
	struct rt_value *dst_val;
	struct rt_value *src_val;

	dst_val = &env->frame->tmpvar[dst];
	src_val = &env->frame->tmpvar[src];

	switch (src_val->type) {
	case NOCT_VALUE_INT:
		dst_val->type = NOCT_VALUE_INT;
		dst_val->val.i = src_val->val.i == 0 ? 1 : 0;
		break;
	default:
		rt_error(env, N_TR("Value is not an integer."));
		return false;
	}

	return true;
}

/*
 * LT helper.
 */
bool
rt_lt_helper(
	struct rt_env *env,
	int dst,
	int src1,
	int src2)
{
	struct rt_value *dst_val;
	struct rt_value *src1_val;
	struct rt_value *src2_val;

	dst_val = &env->frame->tmpvar[dst];
	src1_val = &env->frame->tmpvar[src1];
	src2_val = &env->frame->tmpvar[src2];

	switch (src1_val->type) {
	case NOCT_VALUE_INT:
		switch (src2_val->type) {
		case NOCT_VALUE_INT:
			dst_val->type = NOCT_VALUE_INT;
			dst_val->val.i = (src1_val->val.i < src2_val->val.i) ? 1 : 0;
			break;
		case NOCT_VALUE_FLOAT:
			dst_val->type = NOCT_VALUE_INT;
			dst_val->val.i = ((float)src1_val->val.i < src2_val->val.f) ? 1 : 0;
			break;
		default:
			rt_error(env, N_TR("Value is not a number."));
			return false;
		}
		break;
	case NOCT_VALUE_FLOAT:
		switch (src2_val->type) {
		case NOCT_VALUE_INT:
			dst_val->type = NOCT_VALUE_INT;
			dst_val->val.i = (src1_val->val.f < (float)src2_val->val.i) ? 1 : 0;
			break;
		case NOCT_VALUE_FLOAT:
			dst_val->type = NOCT_VALUE_INT;
			dst_val->val.i = (src1_val->val.f < src2_val->val.f) ? 1 : 0;
			break;
		default:
			rt_error(env, N_TR("Value is not a number."));
			return false;
		}
		break;
	case NOCT_VALUE_STRING:
		switch (src2_val->type) {
		case NOCT_VALUE_STRING:
			dst_val->type = NOCT_VALUE_INT;
			dst_val->val.i = strcmp(src1_val->val.str->data, src2_val->val.str->data) < 0 ? 1 : 0;
			break;
		default:
			rt_error(env, N_TR("Value is not a string."));
			break;
		}
		break;
	default:
		rt_error(env, N_TR("Value is not a number or a string."));
		return false;
	}

	return true;
}

/*
 * LTE helper.
 */
bool
rt_lte_helper(
	struct rt_env *env,
	int dst,
	int src1,
	int src2)
{
	struct rt_value *dst_val;
	struct rt_value *src1_val;
	struct rt_value *src2_val;

	dst_val = &env->frame->tmpvar[dst];
	src1_val = &env->frame->tmpvar[src1];
	src2_val = &env->frame->tmpvar[src2];

	switch (src1_val->type) {
	case NOCT_VALUE_INT:
		switch (src2_val->type) {
		case NOCT_VALUE_INT:
			dst_val->type = NOCT_VALUE_INT;
			dst_val->val.i = (src1_val->val.i <= src2_val->val.i) ? 1 : 0;
			break;
		case NOCT_VALUE_FLOAT:
			dst_val->type = NOCT_VALUE_INT;
			dst_val->val.i = ((float)src1_val->val.i <= src2_val->val.f) ? 1 : 0;
			break;
		default:
			rt_error(env, N_TR("Value is not a number."));
			return false;
		}
		break;
	case NOCT_VALUE_FLOAT:
		switch (src2_val->type) {
		case NOCT_VALUE_INT:
			dst_val->type = NOCT_VALUE_INT;
			dst_val->val.i = (src1_val->val.f <= (float)src2_val->val.i) ? 1 : 0;
			break;
		case NOCT_VALUE_FLOAT:
			dst_val->type = NOCT_VALUE_INT;
			dst_val->val.i = (src1_val->val.f <= src2_val->val.f) ? 1 : 0;
			break;
		default:
			rt_error(env, N_TR("Value is not a number."));
			return false;
		}
		break;
	case NOCT_VALUE_STRING:
		switch (src2_val->type) {
		case NOCT_VALUE_STRING:
			dst_val->type = NOCT_VALUE_INT;
			dst_val->val.i = strcmp(src1_val->val.str->data, src2_val->val.str->data) <= 0 ? 1 : 0;
			break;
		default:
			rt_error(env, N_TR("Value is not a string."));
			break;
		}
		break;
	default:
		rt_error(env, N_TR("Value is not a number or a string."));
		return false;
	}

	return true;
}

/*
 * GT helper.
 */
bool
rt_gt_helper(
	struct rt_env *env,
	int dst,
	int src1,
	int src2)
{
	struct rt_value *dst_val;
	struct rt_value *src1_val;
	struct rt_value *src2_val;

	dst_val = &env->frame->tmpvar[dst];
	src1_val = &env->frame->tmpvar[src1];
	src2_val = &env->frame->tmpvar[src2];

	switch (src1_val->type) {
	case NOCT_VALUE_INT:
		switch (src2_val->type) {
		case NOCT_VALUE_INT:
			dst_val->type = NOCT_VALUE_INT;
			dst_val->val.i = (src1_val->val.i > src2_val->val.i) ? 1 : 0;
			break;
		case NOCT_VALUE_FLOAT:
			dst_val->type = NOCT_VALUE_INT;
			dst_val->val.i = ((float)src1_val->val.i > src2_val->val.f) ? 1 : 0;
			break;
		default:
			rt_error(env, N_TR("Value is not a number."));
			return false;
		}
		break;
	case NOCT_VALUE_FLOAT:
		switch (src2_val->type) {
		case NOCT_VALUE_INT:
			dst_val->type = NOCT_VALUE_INT;
			dst_val->val.i = (src1_val->val.f > (float)src2_val->val.i) ? 1 : 0;
			break;
		case NOCT_VALUE_FLOAT:
			dst_val->type = NOCT_VALUE_INT;
			dst_val->val.i = (src1_val->val.f > src2_val->val.f) ? 1 : 0;
			break;
		default:
			rt_error(env, N_TR("Value is not a number."));
			return false;
		}
		break;
	case NOCT_VALUE_STRING:
		switch (src2_val->type) {
		case NOCT_VALUE_STRING:
			dst_val->type = NOCT_VALUE_INT;
			dst_val->val.i = strcmp(src1_val->val.str->data, src2_val->val.str->data) > 0 ? 1 : 0;
			break;
		default:
			rt_error(env, N_TR("Value is not a string."));
			break;
		}
		break;
	default:
		rt_error(env, N_TR("Value is not a number or a string."));
		return false;
	}

	return true;
}

/*
 * GTE helper.
 */
bool
rt_gte_helper(
	struct rt_env *env,
	int dst,
	int src1,
	int src2)
{
	struct rt_value *dst_val;
	struct rt_value *src1_val;
	struct rt_value *src2_val;

	dst_val = &env->frame->tmpvar[dst];
	src1_val = &env->frame->tmpvar[src1];
	src2_val = &env->frame->tmpvar[src2];

	switch (src1_val->type) {
	case NOCT_VALUE_INT:
		switch (src2_val->type) {
		case NOCT_VALUE_INT:
			dst_val->type = NOCT_VALUE_INT;
			dst_val->val.i = (src1_val->val.i >= src2_val->val.i) ? 1 : 0;
			break;
		case NOCT_VALUE_FLOAT:
			dst_val->type = NOCT_VALUE_INT;
			dst_val->val.i = ((float)src1_val->val.i >= src2_val->val.f) ? 1 : 0;
			break;
		default:
			rt_error(env, N_TR("Value is not a number."));
			return false;
		}
		break;
	case NOCT_VALUE_FLOAT:
		switch (src2_val->type) {
		case NOCT_VALUE_INT:
			dst_val->type = NOCT_VALUE_INT;
			dst_val->val.i = (src1_val->val.f >= (float)src2_val->val.i) ? 1 : 0;
			break;
		case NOCT_VALUE_FLOAT:
			dst_val->type = NOCT_VALUE_INT;
			dst_val->val.i = (src1_val->val.f >= src2_val->val.f) ? 1 : 0;
			break;
		default:
			rt_error(env, N_TR("Value is not a number."));
			return false;
		}
		break;
	case NOCT_VALUE_STRING:
		switch (src2_val->type) {
		case NOCT_VALUE_STRING:
			dst_val->type = NOCT_VALUE_INT;
			dst_val->val.i = strcmp(src1_val->val.str->data, src2_val->val.str->data) >= 0 ? 1 : 0;
			break;
		default:
			rt_error(env, N_TR("Value is not a string."));
			break;
		}
		break;
	default:
		rt_error(env, N_TR("Value is not a number or a string."));
		return false;
	}

	return true;
}

/*
 * EQ helper.
 */
bool
rt_eq_helper(
	struct rt_env *env,
	int dst,
	int src1,
	int src2)
{
	struct rt_value *dst_val;
	struct rt_value *src1_val;
	struct rt_value *src2_val;

	dst_val = &env->frame->tmpvar[dst];
	src1_val = &env->frame->tmpvar[src1];
	src2_val = &env->frame->tmpvar[src2];

	switch (src1_val->type) {
	case NOCT_VALUE_INT:
		switch (src2_val->type) {
		case NOCT_VALUE_INT:
			dst_val->type = NOCT_VALUE_INT;
			dst_val->val.i = (src1_val->val.i == src2_val->val.i) ? 1 : 0;
			break;
		case NOCT_VALUE_FLOAT:
			dst_val->type = NOCT_VALUE_INT;
			dst_val->val.i = ((float)src1_val->val.i == src2_val->val.f) ? 1 : 0;
			break;
		default:
			dst_val->type = NOCT_VALUE_INT;
			dst_val->val.i = 0;
			break;
		}
		break;
	case NOCT_VALUE_FLOAT:
		switch (src2_val->type) {
		case NOCT_VALUE_INT:
			dst_val->type = NOCT_VALUE_INT;
			dst_val->val.i = (src1_val->val.f == (float)src2_val->val.i) ? 1 : 0;
			break;
		case NOCT_VALUE_FLOAT:
			dst_val->type = NOCT_VALUE_INT;
			dst_val->val.i = (src1_val->val.f == src2_val->val.f) ? 1 : 0;
			break;
		default:
			dst_val->type = NOCT_VALUE_INT;
			dst_val->val.i = 0;
			break;
		}
		break;
	case NOCT_VALUE_STRING:
		switch (src2_val->type) {
		case NOCT_VALUE_STRING:
			rt_cache_string_hash(src1_val->val.str);
			rt_cache_string_hash(src2_val->val.str);

			dst_val->type = NOCT_VALUE_INT;
			if (src1_val->val.str->len == src2_val->val.str->len &&
			    src1_val->val.str->hash == src2_val->val.str->hash)
				dst_val->val.i = strcmp(src1_val->val.str->data, src2_val->val.str->data) == 0 ? 1 : 0;
			else
				dst_val->val.i = 0;
			break;
		default:
			dst_val->type = NOCT_VALUE_INT;
			dst_val->val.i = 0;
			break;
		}
		break;
	default:
		dst_val->type = NOCT_VALUE_INT;
		dst_val->val.i = 0;
		break;
	}

	return true;
}

/* NEQ helper. */
bool
rt_neq_helper(
	struct rt_env *env,
	int dst,
	int src1,
	int src2)
{
	struct rt_value *dst_val;
	struct rt_value *src1_val;
	struct rt_value *src2_val;

	dst_val = &env->frame->tmpvar[dst];
	src1_val = &env->frame->tmpvar[src1];
	src2_val = &env->frame->tmpvar[src2];

	switch (src1_val->type) {
	case NOCT_VALUE_INT:
		switch (src2_val->type) {
		case NOCT_VALUE_INT:
			dst_val->type = NOCT_VALUE_INT;
			dst_val->val.i = (src1_val->val.i != src2_val->val.i) ? 1 : 0;
			break;
		case NOCT_VALUE_FLOAT:
			dst_val->type = NOCT_VALUE_INT;
			dst_val->val.i = ((float)src1_val->val.i != src2_val->val.f) ? 1 : 0;
			break;
		default:
			dst_val->type = NOCT_VALUE_INT;
			dst_val->val.i = 1;
			break;
		}
		break;
	case NOCT_VALUE_FLOAT:
		switch (src2_val->type) {
		case NOCT_VALUE_INT:
			dst_val->type = NOCT_VALUE_INT;
			dst_val->val.i = (src1_val->val.f != (float)src2_val->val.i) ? 1 : 0;
			break;
		case NOCT_VALUE_FLOAT:
			dst_val->type = NOCT_VALUE_INT;
			dst_val->val.i = (src1_val->val.f != src2_val->val.f) ? 1 : 0;
			break;
		default:
			dst_val->type = NOCT_VALUE_INT;
			dst_val->val.i = 1;
			break;
		}
		break;
	case NOCT_VALUE_STRING:
		switch (src2_val->type) {
		case NOCT_VALUE_STRING:
			dst_val->type = NOCT_VALUE_INT;
			dst_val->val.i = strcmp(src1_val->val.str->data, src2_val->val.str->data) != 0 ? 1 : 0;
			break;
		default:
			dst_val->type = NOCT_VALUE_INT;
			dst_val->val.i = 1;
			break;
		}
		break;
	default:
		dst_val->type = NOCT_VALUE_INT;
		dst_val->val.i = 1;
		break;
	}

	return true;
}

/*
 * STOREARRAY helper.
 */
bool
rt_storearray_helper(
	struct rt_env *env,
	int arr,
	int subscr,
	int val)
{
	struct rt_value *arr_val;
	struct rt_value *subscr_val;
	struct rt_value *val_val;

	/* Get the container. */
	arr_val = &env->frame->tmpvar[arr];

	/* Get the subscription. */
	subscr_val = &env->frame->tmpvar[subscr];

	/* Get the value to assign. */
	val_val = &env->frame->tmpvar[val];

	if (arr_val->type == NOCT_VALUE_ARRAY) {
		if (subscr_val->type != NOCT_VALUE_INT) {
			rt_error(env, N_TR("Subscript not an integer."));
			return false;
		}

		/* Store to the array. */
		if (!rt_set_array_elem(env, &arr_val->val.arr, subscr_val->val.i, val_val))
			return false;
		return true;
	} else if (arr_val->type == NOCT_VALUE_DICT) {
		if (subscr_val->type != NOCT_VALUE_STRING) {
			rt_error(env, N_TR("Subscript not a string."));
			return false;
		}

		/* Cache the key string hash. */
		rt_cache_string_hash(subscr_val->val.str);

		/* Store to the dictionary. */
		if (!rt_set_dict_elem_with_hash(env,
						&arr_val->val.dict,
						subscr_val->val.str->data,
						subscr_val->val.str->len,
						subscr_val->val.str->hash,
						val_val))
			return false;
		return true;
	}

	rt_error(env, N_TR("Not an array or a dictionary."));
	return false;
}

/*
 * loadarray helper.
 */
bool
rt_loadarray_helper(
	struct rt_env *env,
	int dst,
	int arr,
	int subscr)
{
	struct rt_value *dst_val;
	struct rt_value *arr_val;
	struct rt_value *subscr_val;

	dst_val = &env->frame->tmpvar[dst];
	arr_val = &env->frame->tmpvar[arr];
	subscr_val = &env->frame->tmpvar[subscr];

	/* Check the array type. */
	if (arr_val->type == NOCT_VALUE_ARRAY) {
		/* Get the subscript value. */
		if (subscr_val->type != NOCT_VALUE_INT) {
			rt_error(env, N_TR("Subscript not an integer."));
			return false;
		}

		/* Load the array element. */
		if (!rt_get_array_elem(env, arr_val->val.arr, subscr_val->val.i, dst_val))
			return false;
		return true;
	} else if (arr_val->type == NOCT_VALUE_DICT) {
		/* Get the key string. */
		if (subscr_val->type != NOCT_VALUE_STRING) {
			rt_error(env, N_TR("Subscript not a string."));
			return false;
		}

		/* Cache the key string hash. */
		rt_cache_string_hash(subscr_val->val.str);

		/* Get the dictionary element. */
		if (!rt_get_dict_elem_with_hash(env,
						arr_val->val.dict,
						subscr_val->val.str->data,
						subscr_val->val.str->len,
						subscr_val->val.str->hash,
						dst_val))
			return false;
		return true;
	}

	rt_error(env, N_TR("Not an array or a dictionary."));
	return false;
}

/*
 * LEN helper.
 */
bool
rt_len_helper(
	struct rt_env *env,
	int dst,
	int src)
{
	struct rt_value *dst_val;
	struct rt_value *src_val;
	uint32_t val;

	dst_val = &env->frame->tmpvar[dst];
	src_val = &env->frame->tmpvar[src];

	switch (src_val->type) {
	case NOCT_VALUE_STRING:
		dst_val->type = NOCT_VALUE_INT;
		dst_val->val.i = (int)(src_val->val.str->len - 1); /* Exclude NUL */
		assert(src_val->val.str->len == strlen(src_val->val.str->data));
		break;
	case NOCT_VALUE_ARRAY:
		dst_val->type = NOCT_VALUE_INT;
		rt_get_array_size(env, src_val->val.arr, &val);
		dst_val->val.i = (int)val;
		break;
	case NOCT_VALUE_DICT:
		dst_val->type = NOCT_VALUE_INT;
		rt_get_dict_size(env, src_val->val.dict, &val);
		dst_val->val.i = (int)val;
		break;
	default:
		rt_error(env, N_TR("Value is not a string, an array, or a dictionary."));
		return false;
	}

	return true;
}

/* getdictkeybyindex helper. */
bool
rt_getdictkeybyindex_helper(
	struct rt_env *env,
	int dst,
	int dict,
	int subscr)
{
	struct rt_value *dst_val;
	struct rt_value *dict_val;
	struct rt_value *subscr_val;

	dst_val = &env->frame->tmpvar[dst];
	dict_val = &env->frame->tmpvar[dict];
	subscr_val = &env->frame->tmpvar[subscr];

	if (dict_val->type != NOCT_VALUE_DICT) {
		rt_error(env, N_TR("Not a dictionary."));
		return false;
	}
	if (subscr_val->type != NOCT_VALUE_INT) {
		rt_error(env, N_TR("Subscript not an integer."));
		return false;
	}

	/* Load the element. */
	if (!rt_get_dict_key_by_index(env, dict_val->val.dict, subscr_val->val.i, dst_val))
		return false;

	return true;
}

/*
 * getdictvalbyindex helper.
 */
bool
rt_getdictvalbyindex_helper(
	struct rt_env *env,
	int dst,
	int dict,
	int subscr)
{
	struct rt_value *dst_val;
	struct rt_value *dict_val;
	struct rt_value *subscr_val;

	dst_val = &env->frame->tmpvar[dst];
	dict_val = &env->frame->tmpvar[dict];
	subscr_val = &env->frame->tmpvar[subscr];

	if (dict_val->type != NOCT_VALUE_DICT) {
		rt_error(env, N_TR("Not a dictionary."));
		return false;
	}
	if (subscr_val->type != NOCT_VALUE_INT) {
		rt_error(env, N_TR("Subscript not an integer."));
		return false;
	}

	/* Load the element. */
	if (!rt_get_dict_value_by_index(env, dict_val->val.dict, subscr_val->val.i, dst_val))
		return false;

	return true;
}

/*
 * loadsymbol helper.
 */
bool
rt_loadsymbol_helper(
	struct rt_env *env,
	int dst,
	const char *symbol,
	uint32_t symbol_len,
	uint32_t symbol_hash)
{
	struct rt_value val;

	if (!rt_get_global_with_hash(env, symbol, symbol_len, symbol_hash, &val))
		return false;

	env->frame->tmpvar[dst] = val;

	return true;
}

/*
 * storesymbol helper.
 */
bool
rt_storesymbol_helper(
	struct rt_env *env,
	const char *symbol,
	uint32_t symbol_len,
	uint32_t symbol_hash,
	int src)
{
	if (!rt_set_global_with_hash(env, symbol, symbol_len, symbol_hash, &env->frame->tmpvar[src]))
		return false;

	return true;
}

/*
 * loaddot helper.
 */
bool
rt_loaddot_helper(
	struct rt_env *env,
	int dst,
	int dict,
	const char *field,
	uint32_t field_len,
	uint32_t field_hash)
{
	/* Special field "length". */
	if (field_len == 7 &&
	    field_hash == 0x83d03615 &&
	    strcmp(field, "length") == 0) {
		if (env->frame->tmpvar[dict].type == NOCT_VALUE_DICT) {
			uint32_t size;
			if (!rt_get_dict_size(env, env->frame->tmpvar[dict].val.dict, &size))
				return false;
			env->frame->tmpvar[dst].type = NOCT_VALUE_INT;
			env->frame->tmpvar[dst].val.i = (int)size;
			return true;
		} else if (env->frame->tmpvar[dict].type == NOCT_VALUE_ARRAY) {
			uint32_t size;
			if (!rt_get_array_size(env, env->frame->tmpvar[dict].val.arr, &size))
				return false;
			env->frame->tmpvar[dst].type = NOCT_VALUE_INT;
			env->frame->tmpvar[dst].val.i = (int)size;
			return true;
		} else if (env->frame->tmpvar[dict].type == NOCT_VALUE_STRING) {
			env->frame->tmpvar[dst].type = NOCT_VALUE_INT;
			env->frame->tmpvar[dst].val.i = (int)(env->frame->tmpvar[dict].val.str->len - 1); /* Exclude NUL */
			return true;
		}
	}

	if (env->frame->tmpvar[dict].type != NOCT_VALUE_DICT) {
		rt_error(env, N_TR("Not a dictionary."));
		return false;
	}

	if (!rt_get_dict_elem_with_hash(env, env->frame->tmpvar[dict].val.dict, field, field_len, field_hash, &env->frame->tmpvar[dst]))
		return false;

	return true;
}

/*
 * STOREDOT helper.
 */
bool
rt_storedot_helper(
	struct rt_env *env,
	int dict,
	const char *field,
	uint32_t field_len,
	uint32_t field_hash,
	int src)
{
	struct rt_value *dict_val, *val;

	/* Get the dictionary. */
	dict_val = &env->frame->tmpvar[dict];
	if (dict_val->type != NOCT_VALUE_DICT) {
		rt_error(env, N_TR("Not a dictionary."));
		return false;
	}

	/* Get the source value. */
	val = &env->frame->tmpvar[src];

	/* Store the source value to the dictionary with the key. */
	if (!rt_set_dict_elem_with_hash(env, &dict_val->val.dict, field, field_len, field_hash, val))
		return false;

	return true;
}

/* CALL helper. */
bool
rt_call_helper(
	struct rt_env *env,
	int dst,
	int func,
	int arg_count,
	int *arg)
{
	struct rt_value arg_val[NOCT_ARG_MAX];
	struct rt_func *callee;
	struct rt_value ret;
	int i;

	/* Get a function. */
	if (env->frame->tmpvar[func].type != NOCT_VALUE_FUNC) {
		rt_error(env, N_TR("Not a function."));
		return false;
	}
	callee = env->frame->tmpvar[func].val.func;

	/* Get values of arguments. */
	for (i = 0; i < arg_count; i++)
		arg_val[i] = env->frame->tmpvar[arg[i]];

	/* Do call. */
	if (!rt_call(env, callee, arg_count, &arg_val[0], &ret))
		return false;

	/* Store a return value. */
	env->frame->tmpvar[dst] = ret;

	return true;
}

/*
 * THISCALL helper.
 */
bool
rt_thiscall_helper(
	struct rt_env *env,
	int dst,
	int obj,
	const char *name,
	uint32_t name_len,
	uint32_t name_hash,
	int arg_count,
	int *arg)
{
	struct rt_value arg_val[NOCT_ARG_MAX];
	struct rt_value callee_value;
	struct rt_func *callee;
	struct rt_value *obj_val;
	struct rt_value ret;
	int i;

	/* Get a receiver object. */
	obj_val = &env->frame->tmpvar[obj];

	/* Check for an intrinsic. (callee == NULL is not found.)*/
	if (!rt_get_intrin_thiscall_func(env, name, &callee))
		return false;
	if (callee == NULL) {
		/* If not an intrinsic call, object must be a dictionary. */
		if (env->frame->tmpvar[obj].type != NOCT_VALUE_DICT) {
			rt_error(env, N_TR("Not a dictionary."));
			return false;
		}

		/* Get a function from a receiver object. */
		if (!rt_get_dict_elem_with_hash(env, env->frame->tmpvar[obj].val.dict, name, name_len, name_hash, &callee_value))
			return false;
		if (callee_value.type != NOCT_VALUE_FUNC) {
			rt_error(env, N_TR("Not a function."));
			return false;
		}
		callee = callee_value.val.func;
	}

	/* Get values of arguments. */
	arg_count++;
	arg_val[0] = *obj_val;
	for (i = 1; i < arg_count; i++)
		arg_val[i] = env->frame->tmpvar[arg[i - 1]];

	/* Do call. */
	if (!rt_call(env, callee, arg_count, &arg_val[0], &ret))
		return false;

	/* Store a return value. */
	env->frame->tmpvar[dst] = ret;

	return true;
}
