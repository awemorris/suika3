/* -*- coding: utf-8; tab-width: 8; indent-tabs-mode: t; -*- */

/*
 * Noct Programming Language
 * Copyright (c) 2025, 2026, Awe Morris
 */

/*
 * Execution Helpers
 */

#include <noct/noct.h>
#include "runtime.h"
#include "intrinsics.h"

#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <assert.h>

/*
 * Some exotic compilers for x86 including Watcom utilize registers to
 * pass function arguments. However, our JIT-generated code for x86
 * uses the stack for function arguments. To bridge this gap, we use
 * the CDECL keyword in this module.
 */

/*
 * Make a string.
 */
NOCT_DLL
bool
CDECL
noct_ex_make_string_with_hash(
	NoctEnv *env,
	NoctValue *val,
	const char *data,
	size_t len,
	uint32_t hash)
{
	/* Be careful: calling convention may be different. */
	return rt_make_string_with_hash(env, val, data, len, hash);
}

/*
 * Make an empty array.
 */
NOCT_DLL
bool
CDECL
noct_ex_make_empty_array(
	NoctEnv *env,
	NoctValue *val)
{
	/* Be careful: calling convention may be different. */
	return rt_make_empty_array(env, val);
}

/*
 * Make an empty dictionary.
 */
NOCT_DLL
bool
CDECL
noct_ex_make_empty_dict(
	NoctEnv *env,
	NoctValue *val)
{
	/* Be careful: calling convention may be different. */
	return rt_make_empty_dict(env, val);
}

/*
 * Add helper.
 */
NOCT_DLL
bool
CDECL
noct_ex_add_helper(
	NoctEnv *env,
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
		case NOCT_VALUE_LONG:
			dst_val->type = NOCT_VALUE_LONG;
			dst_val->val.l = (int64_t)src1_val->val.i + src2_val->val.l;
			break;
		case NOCT_VALUE_FLOAT:
			dst_val->type = NOCT_VALUE_FLOAT;
			dst_val->val.f = (float)src1_val->val.i + src2_val->val.f;
			break;
		case NOCT_VALUE_DOUBLE:
			dst_val->type = NOCT_VALUE_DOUBLE;
			dst_val->val.lf = (double)src1_val->val.i + src2_val->val.lf;
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
	case NOCT_VALUE_LONG:
		switch (src2_val->type) {
		case NOCT_VALUE_INT:
			dst_val->type = NOCT_VALUE_LONG;
			dst_val->val.l = src1_val->val.l + (int64_t)src2_val->val.i;
			break;
		case NOCT_VALUE_LONG:
			dst_val->type = NOCT_VALUE_LONG;
			dst_val->val.l = src1_val->val.l + src2_val->val.l;
			break;
		case NOCT_VALUE_FLOAT:
			dst_val->type = NOCT_VALUE_FLOAT;
			dst_val->val.f = (float)src1_val->val.l + src2_val->val.f;
			break;
		case NOCT_VALUE_DOUBLE:
			dst_val->type = NOCT_VALUE_DOUBLE;
			dst_val->val.lf = (double)src1_val->val.l + src2_val->val.lf;
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
		case NOCT_VALUE_LONG:
			dst_val->type = NOCT_VALUE_FLOAT;
			dst_val->val.f = src1_val->val.f + (float)src2_val->val.l;
			break;
		case NOCT_VALUE_FLOAT:
			dst_val->type = NOCT_VALUE_FLOAT;
			dst_val->val.f = src1_val->val.f + src2_val->val.f;
			break;
		case NOCT_VALUE_DOUBLE:
			dst_val->type = NOCT_VALUE_DOUBLE;
			dst_val->val.lf = (double)src1_val->val.f + src2_val->val.lf;
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
	case NOCT_VALUE_DOUBLE:
		switch (src2_val->type) {
		case NOCT_VALUE_INT:
			dst_val->type = NOCT_VALUE_DOUBLE;
			dst_val->val.lf = src1_val->val.lf + (double)src2_val->val.i;
			break;
		case NOCT_VALUE_LONG:
			dst_val->type = NOCT_VALUE_DOUBLE;
			dst_val->val.lf = src1_val->val.lf + (double)src2_val->val.l;
			break;
		case NOCT_VALUE_FLOAT:
			dst_val->type = NOCT_VALUE_DOUBLE;
			dst_val->val.lf = src1_val->val.lf + (double)src2_val->val.f;
			break;
		case NOCT_VALUE_DOUBLE:
			dst_val->type = NOCT_VALUE_DOUBLE;
			dst_val->val.lf = src1_val->val.lf + src2_val->val.lf;
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
		case NOCT_VALUE_LONG:
			if (!noct_make_string_format(env, dst_val, "%s%lld", src1_val->val.str->data, src2_val->val.l))
				return false;
			break;
		case NOCT_VALUE_FLOAT:
			if (!noct_make_string_format(env, dst_val, "%s%.7g", src1_val->val.str->data, src2_val->val.f))
				return false;
			break;
		case NOCT_VALUE_DOUBLE:
			if (!noct_make_string_format(env, dst_val, "%s%.15g", src1_val->val.str->data, src2_val->val.lf))
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
NOCT_DLL
bool
CDECL
noct_ex_sub_helper(
	NoctEnv *env,
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
		case NOCT_VALUE_LONG:
			dst_val->type = NOCT_VALUE_LONG;
			dst_val->val.l = (int64_t)src1_val->val.i - src2_val->val.l;
			break;
		case NOCT_VALUE_FLOAT:
			dst_val->type = NOCT_VALUE_FLOAT;
			dst_val->val.f = (float)src1_val->val.i - src2_val->val.f;
			break;
		case NOCT_VALUE_DOUBLE:
			dst_val->type = NOCT_VALUE_DOUBLE;
			dst_val->val.lf = (double)src1_val->val.i - src2_val->val.lf;
			break;
		default:
			rt_error(env, N_TR("Value is not a number."));
			return false;
		}
		break;
	case NOCT_VALUE_LONG:
		switch (src2_val->type) {
		case NOCT_VALUE_INT:
			dst_val->type = NOCT_VALUE_LONG;
			dst_val->val.l = src1_val->val.l - (int64_t)src2_val->val.i;
			break;
		case NOCT_VALUE_LONG:
			dst_val->type = NOCT_VALUE_LONG;
			dst_val->val.l = src1_val->val.l - src2_val->val.l;
			break;
		case NOCT_VALUE_FLOAT:
			dst_val->type = NOCT_VALUE_FLOAT;
			dst_val->val.f = (float)src1_val->val.l - src2_val->val.f;
			break;
		case NOCT_VALUE_DOUBLE:
			dst_val->type = NOCT_VALUE_DOUBLE;
			dst_val->val.lf = (double)src1_val->val.l - src2_val->val.lf;
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
		case NOCT_VALUE_LONG:
			dst_val->type = NOCT_VALUE_FLOAT;
			dst_val->val.f = src1_val->val.f - (float)src2_val->val.l;
			break;
		case NOCT_VALUE_FLOAT:
			dst_val->type = NOCT_VALUE_FLOAT;
			dst_val->val.f = src1_val->val.f - src2_val->val.f;
			break;
		case NOCT_VALUE_DOUBLE:
			dst_val->type = NOCT_VALUE_DOUBLE;
			dst_val->val.lf = (double)src1_val->val.f - src2_val->val.lf;
			break;
		default:
			rt_error(env, N_TR("Value is not a number."));
			return false;
		}
		break;
	case NOCT_VALUE_DOUBLE:
		switch (src2_val->type) {
		case NOCT_VALUE_INT:
			dst_val->type = NOCT_VALUE_DOUBLE;
			dst_val->val.lf = src1_val->val.lf - (double)src2_val->val.i;
			break;
		case NOCT_VALUE_LONG:
			dst_val->type = NOCT_VALUE_DOUBLE;
			dst_val->val.lf = src1_val->val.lf - (double)src2_val->val.l;
			break;
		case NOCT_VALUE_FLOAT:
			dst_val->type = NOCT_VALUE_DOUBLE;
			dst_val->val.lf = src1_val->val.lf - (double)src2_val->val.f;
			break;
		case NOCT_VALUE_DOUBLE:
			dst_val->type = NOCT_VALUE_DOUBLE;
			dst_val->val.lf = src1_val->val.lf - src2_val->val.lf;
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
NOCT_DLL
bool
CDECL
noct_ex_mul_helper(
	NoctEnv *env,
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
		case NOCT_VALUE_LONG:
			dst_val->type = NOCT_VALUE_LONG;
			dst_val->val.l = (int64_t)src1_val->val.i * src2_val->val.l;
			break;
		case NOCT_VALUE_FLOAT:
			dst_val->type = NOCT_VALUE_FLOAT;
			dst_val->val.f = (float)src1_val->val.i * src2_val->val.f;
			break;
		case NOCT_VALUE_DOUBLE:
			dst_val->type = NOCT_VALUE_DOUBLE;
			dst_val->val.lf = (double)src1_val->val.i * src2_val->val.lf;
			break;
		default:
			rt_error(env, N_TR("Value is not a number."));
			return false;
		}
		break;
	case NOCT_VALUE_LONG:
		switch (src2_val->type) {
		case NOCT_VALUE_INT:
			dst_val->type = NOCT_VALUE_LONG;
			dst_val->val.l = src1_val->val.l * (int64_t)src2_val->val.i;
			break;
		case NOCT_VALUE_LONG:
			dst_val->type = NOCT_VALUE_LONG;
			dst_val->val.l = src1_val->val.l * src2_val->val.l;
			break;
		case NOCT_VALUE_FLOAT:
			dst_val->type = NOCT_VALUE_FLOAT;
			dst_val->val.f = (float)src1_val->val.l * src2_val->val.f;
			break;
		case NOCT_VALUE_DOUBLE:
			dst_val->type = NOCT_VALUE_DOUBLE;
			dst_val->val.lf = (double)src1_val->val.l * src2_val->val.lf;
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
		case NOCT_VALUE_LONG:
			dst_val->type = NOCT_VALUE_FLOAT;
			dst_val->val.f = src1_val->val.f * (float)src2_val->val.l;
			break;
		case NOCT_VALUE_FLOAT:
			dst_val->type = NOCT_VALUE_FLOAT;
			dst_val->val.f = src1_val->val.f * src2_val->val.f;
			break;
		case NOCT_VALUE_DOUBLE:
			dst_val->type = NOCT_VALUE_DOUBLE;
			dst_val->val.lf = (double)src1_val->val.f * src2_val->val.lf;
			break;
		default:
			rt_error(env, N_TR("Value is not a number."));
			return false;
		}
		break;
	case NOCT_VALUE_DOUBLE:
		switch (src2_val->type) {
		case NOCT_VALUE_INT:
			dst_val->type = NOCT_VALUE_DOUBLE;
			dst_val->val.lf = src1_val->val.lf * (double)src2_val->val.i;
			break;
		case NOCT_VALUE_LONG:
			dst_val->type = NOCT_VALUE_DOUBLE;
			dst_val->val.lf = src1_val->val.lf * (double)src2_val->val.l;
			break;
		case NOCT_VALUE_FLOAT:
			dst_val->type = NOCT_VALUE_DOUBLE;
			dst_val->val.lf = src1_val->val.lf * (double)src2_val->val.f;
			break;
		case NOCT_VALUE_DOUBLE:
			dst_val->type = NOCT_VALUE_DOUBLE;
			dst_val->val.lf = src1_val->val.lf * src2_val->val.lf;
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
NOCT_DLL
bool
CDECL
noct_ex_div_helper(
	NoctEnv *env,
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
		case NOCT_VALUE_LONG:
			if (src2_val->val.l == 0) {
				rt_error(env, N_TR("Division by zero."));
				return false;
			}
			dst_val->type = NOCT_VALUE_LONG;
			dst_val->val.l = (int64_t)src1_val->val.i / src2_val->val.l;
			break;
		case NOCT_VALUE_FLOAT:
			if (src2_val->val.f == 0) {
				rt_error(env, N_TR("Division by zero."));
				return false;
			}
			dst_val->type = NOCT_VALUE_FLOAT;
			dst_val->val.f = (float)src1_val->val.i / src2_val->val.f;
			break;
		case NOCT_VALUE_DOUBLE:
			if (src2_val->val.lf == 0) {
				rt_error(env, N_TR("Division by zero."));
				return false;
			}
			dst_val->type = NOCT_VALUE_DOUBLE;
			dst_val->val.lf = (double)src1_val->val.i / src2_val->val.lf;
			break;
		default:
			rt_error(env, N_TR("Value is not a number."));
			return false;
		}
		break;
	case NOCT_VALUE_LONG:
		switch (src2_val->type) {
		case NOCT_VALUE_INT:
			if (src2_val->val.i == 0) {
				rt_error(env, N_TR("Division by zero."));
				return false;
			}
			dst_val->type = NOCT_VALUE_LONG;
			dst_val->val.l = src1_val->val.l / (int64_t)src2_val->val.i;
			break;
		case NOCT_VALUE_LONG:
			if (src2_val->val.l == 0) {
				rt_error(env, N_TR("Division by zero."));
				return false;
			}
			dst_val->type = NOCT_VALUE_LONG;
			dst_val->val.l = src1_val->val.l / src2_val->val.l;
			break;
		case NOCT_VALUE_FLOAT:
			if (src2_val->val.f == 0) {
				rt_error(env, N_TR("Division by zero."));
				return false;
			}
			dst_val->type = NOCT_VALUE_FLOAT;
			dst_val->val.f = (float)src1_val->val.l / src2_val->val.f;
			break;
		case NOCT_VALUE_DOUBLE:
			if (src2_val->val.lf == 0) {
				rt_error(env, N_TR("Division by zero."));
				return false;
			}
			dst_val->type = NOCT_VALUE_DOUBLE;
			dst_val->val.lf = (double)src1_val->val.l / src2_val->val.lf;
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
		case NOCT_VALUE_LONG:
			if (src2_val->val.l == 0) {
				rt_error(env, N_TR("Division by zero."));
				return false;
			}
			dst_val->type = NOCT_VALUE_FLOAT;
			dst_val->val.f = src1_val->val.f / (float)src2_val->val.l;
			break;
		case NOCT_VALUE_FLOAT:
			if (src2_val->val.f == 0) {
				rt_error(env, N_TR("Division by zero."));
				return false;
			}
			dst_val->type = NOCT_VALUE_FLOAT;
			dst_val->val.f = src1_val->val.f / src2_val->val.f;
			break;
		case NOCT_VALUE_DOUBLE:
			if (src2_val->val.lf == 0) {
				rt_error(env, N_TR("Division by zero."));
				return false;
			}
			dst_val->type = NOCT_VALUE_DOUBLE;
			dst_val->val.lf = (double)src1_val->val.f / src2_val->val.lf;
			break;
		default:
			rt_error(env, N_TR("Value is not a number."));
			return false;
		}
		break;
	case NOCT_VALUE_DOUBLE:
		switch (src2_val->type) {
		case NOCT_VALUE_INT:
			if (src2_val->val.i == 0) {
				rt_error(env, N_TR("Division by zero."));
				return false;
			}
			dst_val->type = NOCT_VALUE_DOUBLE;
			dst_val->val.lf = src1_val->val.lf / (double)src2_val->val.i;
			break;
		case NOCT_VALUE_LONG:
			if (src2_val->val.l == 0) {
				rt_error(env, N_TR("Division by zero."));
				return false;
			}
			dst_val->type = NOCT_VALUE_DOUBLE;
			dst_val->val.lf = src1_val->val.lf / (double)src2_val->val.l;
			break;
		case NOCT_VALUE_FLOAT:
			if (src2_val->val.f == 0) {
				rt_error(env, N_TR("Division by zero."));
				return false;
			}
			dst_val->type = NOCT_VALUE_DOUBLE;
			dst_val->val.lf = src1_val->val.lf / (double)src2_val->val.f;
			break;
		case NOCT_VALUE_DOUBLE:
			if (src2_val->val.lf == 0) {
				rt_error(env, N_TR("Division by zero."));
				return false;
			}
			dst_val->type = NOCT_VALUE_DOUBLE;
			dst_val->val.lf = src1_val->val.lf / src2_val->val.lf;
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
NOCT_DLL
bool
CDECL
noct_ex_mod_helper(
	NoctEnv *env,
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
		case NOCT_VALUE_LONG:
			dst_val->type = NOCT_VALUE_LONG;
			dst_val->val.l = (int64_t)src1_val->val.i % src2_val->val.l;
			break;
		default:
			rt_error(env, N_TR("Value is not an integer."));
			return false;
		}
		break;
	case NOCT_VALUE_LONG:
		switch (src2_val->type) {
		case NOCT_VALUE_INT:
			dst_val->type = NOCT_VALUE_LONG;
			dst_val->val.l = src1_val->val.l % (int64_t)src2_val->val.i;
			break;
		case NOCT_VALUE_LONG:
			dst_val->type = NOCT_VALUE_LONG;
			dst_val->val.l = src1_val->val.l % src2_val->val.l;
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
NOCT_DLL
bool
CDECL
noct_ex_and_helper(
	NoctEnv *env,
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
			dst_val->val.i = (int32_t)((uint32_t)src1_val->val.i & (uint32_t)src2_val->val.i);
			break;
		case NOCT_VALUE_LONG:
			dst_val->type = NOCT_VALUE_LONG;
			dst_val->val.l = (int64_t)((uint64_t)(uint32_t)src1_val->val.i & (uint64_t)src2_val->val.l);
			break;
		default:
			rt_error(env, N_TR("Value is not an integer."));
			return false;
		}
		break;
	case NOCT_VALUE_LONG:
		switch (src2_val->type) {
		case NOCT_VALUE_INT:
			dst_val->type = NOCT_VALUE_LONG;
			dst_val->val.l = (int64_t)((uint64_t)src1_val->val.l & (uint64_t)(uint32_t)src2_val->val.i);
			break;
		case NOCT_VALUE_LONG:
			dst_val->type = NOCT_VALUE_LONG;
			dst_val->val.l = (int64_t)((uint64_t)src1_val->val.l & (uint64_t)src2_val->val.l);
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
NOCT_DLL
bool
CDECL
noct_ex_or_helper(
	NoctEnv *env,
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
			dst_val->val.i = (int32_t)((uint32_t)src1_val->val.i | (uint32_t)src2_val->val.i);
			break;
		case NOCT_VALUE_LONG:
			dst_val->type = NOCT_VALUE_LONG;
			dst_val->val.l = (int64_t)((uint64_t)(uint32_t)src1_val->val.i | (uint64_t)src2_val->val.l);
			break;
		default:
			rt_error(env, N_TR("Value is not an integer."));
			return false;
		}
		break;
	case NOCT_VALUE_LONG:
		switch (src2_val->type) {
		case NOCT_VALUE_INT:
			dst_val->type = NOCT_VALUE_LONG;
			dst_val->val.l = (int64_t)((uint64_t)src1_val->val.l | (uint64_t)(uint32_t)src2_val->val.i);
			break;
		case NOCT_VALUE_LONG:
			dst_val->type = NOCT_VALUE_LONG;
			dst_val->val.l = (int64_t)((uint64_t)(uint32_t)src1_val->val.l | (uint64_t)src2_val->val.l);
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
NOCT_DLL
bool
CDECL
noct_ex_xor_helper(
	NoctEnv *env,
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
			dst_val->val.i = (int32_t)((uint32_t)src1_val->val.i ^ (uint32_t)src2_val->val.i);
			break;
		case NOCT_VALUE_LONG:
			dst_val->type = NOCT_VALUE_LONG;
			dst_val->val.l = (int64_t)((uint64_t)(uint32_t)src1_val->val.i ^ (uint64_t)src2_val->val.l);
			break;
		default:
			rt_error(env, N_TR("Value is not an integer."));
			return false;
		}
		break;
	case NOCT_VALUE_LONG:
		switch (src2_val->type) {
		case NOCT_VALUE_INT:
			dst_val->type = NOCT_VALUE_LONG;
			dst_val->val.l = (int64_t)((uint64_t)src1_val->val.l ^ (uint64_t)(uint32_t)src2_val->val.i);
			break;
		case NOCT_VALUE_LONG:
			dst_val->type = NOCT_VALUE_LONG;
			dst_val->val.l = (int64_t)((uint64_t)src1_val->val.l ^ (uint64_t)src2_val->val.l);
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
NOCT_DLL
bool
CDECL
noct_ex_shl_helper(
	NoctEnv *env,
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
NOCT_DLL
bool
CDECL
noct_ex_shr_helper(
	NoctEnv *env,
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
NOCT_DLL
bool
CDECL
noct_ex_neg_helper(
	NoctEnv *env,
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
 * NOT helper.
 */
NOCT_DLL
bool
CDECL
noct_ex_not_helper(
	NoctEnv *env,
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
NOCT_DLL
bool
CDECL
noct_ex_lt_helper(
	NoctEnv *env,
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
NOCT_DLL
bool
CDECL
noct_ex_lte_helper(
	NoctEnv *env,
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
NOCT_DLL
bool
CDECL
noct_ex_gt_helper(
	NoctEnv *env,
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
NOCT_DLL
bool
CDECL
noct_ex_gte_helper(
	NoctEnv *env,
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
NOCT_DLL
bool
CDECL
noct_ex_eq_helper(
	NoctEnv *env,
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
		case NOCT_VALUE_LONG:
			dst_val->type = NOCT_VALUE_INT;
			dst_val->val.i = ((uint64_t)(uint32_t)src1_val->val.i == (uint64_t)src2_val->val.l) ? 1 : 0;
			break;
		case NOCT_VALUE_FLOAT:
			dst_val->type = NOCT_VALUE_INT;
			dst_val->val.i = ((float)src1_val->val.i == src2_val->val.f) ? 1 : 0;
			break;
		case NOCT_VALUE_DOUBLE:
			dst_val->type = NOCT_VALUE_INT;
			dst_val->val.i = ((double)src1_val->val.i == src2_val->val.lf) ? 1 : 0;
			break;
		default:
			dst_val->type = NOCT_VALUE_INT;
			dst_val->val.i = 0;
			break;
		}
		break;
	case NOCT_VALUE_LONG:
		switch (src2_val->type) {
		case NOCT_VALUE_INT:
			dst_val->type = NOCT_VALUE_INT;
			dst_val->val.i = ((uint64_t)src1_val->val.l == (uint64_t)(uint32_t)src2_val->val.i) ? 1 : 0;
			break;
		case NOCT_VALUE_LONG:
			dst_val->type = NOCT_VALUE_INT;
			dst_val->val.i = (src1_val->val.l == src2_val->val.l) ? 1 : 0;
			break;
		case NOCT_VALUE_FLOAT:
			dst_val->type = NOCT_VALUE_INT;
			dst_val->val.i = ((float)src1_val->val.l == src2_val->val.f) ? 1 : 0;
			break;
		case NOCT_VALUE_DOUBLE:
			dst_val->type = NOCT_VALUE_INT;
			dst_val->val.i = ((double)src1_val->val.l == src2_val->val.lf) ? 1 : 0;
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
		case NOCT_VALUE_LONG:
			dst_val->type = NOCT_VALUE_INT;
			dst_val->val.i = (src1_val->val.f == (float)src2_val->val.l) ? 1 : 0;
			break;
		case NOCT_VALUE_FLOAT:
			dst_val->type = NOCT_VALUE_INT;
			dst_val->val.i = (src1_val->val.f == src2_val->val.f) ? 1 : 0;
			break;
		case NOCT_VALUE_DOUBLE:
			dst_val->type = NOCT_VALUE_INT;
			dst_val->val.i = (src1_val->val.f == src2_val->val.lf) ? 1 : 0;
			break;
		default:
			dst_val->type = NOCT_VALUE_INT;
			dst_val->val.i = 0;
			break;
		}
		break;
	case NOCT_VALUE_DOUBLE:
		switch (src2_val->type) {
		case NOCT_VALUE_INT:
			dst_val->type = NOCT_VALUE_INT;
			dst_val->val.i = (src1_val->val.lf == (double)src2_val->val.i) ? 1 : 0;
			break;
		case NOCT_VALUE_LONG:
			dst_val->type = NOCT_VALUE_INT;
			dst_val->val.i = (src1_val->val.lf == (double)src2_val->val.l) ? 1 : 0;
			break;
		case NOCT_VALUE_FLOAT:
			dst_val->type = NOCT_VALUE_INT;
			dst_val->val.i = (src1_val->val.lf == (double)src2_val->val.f) ? 1 : 0;
			break;
		case NOCT_VALUE_DOUBLE:
			dst_val->type = NOCT_VALUE_INT;
			dst_val->val.i = (src1_val->val.lf == src2_val->val.lf) ? 1 : 0;
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
NOCT_DLL
bool
CDECL
noct_ex_neq_helper(
	NoctEnv *env,
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
NOCT_DLL
bool
CDECL
noct_ex_storearray_helper(
	NoctEnv *env,
	int arr,
	int subscr,
	int val)
{
	struct rt_value *arr_val;
	struct rt_value *subscr_val;
	struct rt_value *val_val;
	size_t index;

	/* Get the container. */
	arr_val = &env->frame->tmpvar[arr];

	/* Get the subscription. */
	subscr_val = &env->frame->tmpvar[subscr];

	/* Get the value to assign. */
	val_val = &env->frame->tmpvar[val];

	if (arr_val->type == NOCT_VALUE_ARRAY) {
		if (subscr_val->type == NOCT_VALUE_INT) {
			index = (size_t)(uint32_t)subscr_val->val.i;
		} else if (subscr_val->type == NOCT_VALUE_LONG) {
			index = (size_t)(uint64_t)subscr_val->val.l;
		} else {
			rt_error(env, N_TR("Subscript not an integer."));
			return false;
		}

		/* Store to the array. */
		if (!rt_set_array_elem(env, arr_val, index, val_val))
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
		if (!rt_set_dict_elem(env,
				      arr_val,
				      subscr_val,
				      val_val))
			return false;
		return true;
	} else if (arr_val->type == NOCT_VALUE_PACKED) {
		if (subscr_val->type == NOCT_VALUE_INT) {
			index = (size_t)(uint32_t)subscr_val->val.i;
		} else if (subscr_val->type == NOCT_VALUE_LONG) {
			index = (size_t)(uint64_t)subscr_val->val.l;
		} else {
			rt_error(env, N_TR("Subscript not an integer."));
			return false;
		}

		/* Store to the packed. */
		if (!rt_set_packed_elem(env, arr_val, index, val_val))
			return false;
		return true;
	}

	rt_error(env, N_TR("Not an array or a dictionary."));
	return false;
}

/*
 * LOADARRAY helper.
 */
NOCT_DLL
bool
CDECL
noct_ex_loadarray_helper(
	NoctEnv *env,
	int dst,
	int arr,
	int subscr)
{
	struct rt_value *dst_val;
	struct rt_value *arr_val;
	struct rt_value *subscr_val;
	size_t index;

	dst_val = &env->frame->tmpvar[dst];
	arr_val = &env->frame->tmpvar[arr];
	subscr_val = &env->frame->tmpvar[subscr];

	/* Check the array type. */
	if (arr_val->type == NOCT_VALUE_ARRAY) {
		if (subscr_val->type == NOCT_VALUE_INT) {
			index = (size_t)(uint32_t)subscr_val->val.i;
		} else if (subscr_val->type == NOCT_VALUE_LONG) {
			index = (size_t)(uint64_t)subscr_val->val.l;
		} else {
			rt_error(env, N_TR("Subscript not an integer."));
			return false;
		}

		/* Load the array element. */
		if (!rt_get_array_elem(env, arr_val, index, dst_val))
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
		if (!rt_get_dict_elem(env, arr_val, subscr_val, dst_val))
			return false;
		return true;
	} else if (arr_val->type == NOCT_VALUE_PACKED) {
		if (subscr_val->type == NOCT_VALUE_INT) {
			index = (size_t)(uint32_t)subscr_val->val.i;
		} else if (subscr_val->type == NOCT_VALUE_LONG) {
			index = (size_t)(uint64_t)subscr_val->val.l;
		} else {
			rt_error(env, N_TR("Subscript not an integer."));
			return false;
		}

		/* Load the packed element. */
		if (!rt_get_packed_elem(env, arr_val, index, dst_val))
			return false;
		return true;
	}

	rt_error(env, N_TR("Not an array or a dictionary."));
	return false;
}

/*
 * LEN helper.
 */
NOCT_DLL
bool
CDECL
noct_ex_len_helper(
	NoctEnv *env,
	int dst,
	int src)
{
	struct rt_value *dst_val;
	struct rt_value *src_val;
	size_t val;

	dst_val = &env->frame->tmpvar[dst];
	src_val = &env->frame->tmpvar[src];

	switch (src_val->type) {
	case NOCT_VALUE_STRING:
		val = (src_val->val.str->len - 1); /* Exclude NUL */
		break;
	case NOCT_VALUE_ARRAY:
		rt_get_array_size(env, src_val, &val);
		break;
	case NOCT_VALUE_DICT:
		rt_get_dict_size(env, src_val, &val);
		break;
	case NOCT_VALUE_PACKED:
		rt_get_packed_size(env, src_val, &val);
		break;
	default:
		rt_error(env, N_TR("Value is not a string, an array, or a dictionary."));
		return false;
	}

	if (val <= INT_MAX) {
		dst_val->type = NOCT_VALUE_INT;
		dst_val->val.i = (int32_t)val;
	} else {
		dst_val->type = NOCT_VALUE_LONG;
		dst_val->val.l = (int64_t)val;
	}

	return true;
}

/*
 * GETDICTKEYBYINDEX helper.
 */
NOCT_DLL
bool
CDECL
noct_ex_getdictkeybyindex_helper(
	NoctEnv *env,
	int dst,
	int dict,
	int subscr)
{
	struct rt_value *dst_val;
	struct rt_value *dict_val;
	struct rt_value *subscr_val;
	size_t index;

	dst_val = &env->frame->tmpvar[dst];
	dict_val = &env->frame->tmpvar[dict];
	subscr_val = &env->frame->tmpvar[subscr];

	if (dict_val->type != NOCT_VALUE_DICT) {
		rt_error(env, N_TR("Not a dictionary."));
		return false;
	}
	if (subscr_val->type == NOCT_VALUE_INT) {
		index = (size_t)(uint32_t)subscr_val->val.i;
	} else if (subscr_val->type == NOCT_VALUE_LONG) {
		index = (size_t)(uint64_t)subscr_val->val.l;
	} else {
		rt_error(env, N_TR("Subscript not an integer."));
		return false;
	}

	/* Load the element. */
	if (!rt_get_dict_key_by_index(env, dict_val, index, dst_val))
		return false;

	return true;
}

/*
 * GETDICTVALBYINDEX helper.
 */
NOCT_DLL
bool
CDECL
noct_ex_getdictvalbyindex_helper(
	NoctEnv *env,
	int dst,
	int dict,
	int subscr)
{
	struct rt_value *dst_val;
	struct rt_value *dict_val;
	struct rt_value *subscr_val;
	size_t index;

	dst_val = &env->frame->tmpvar[dst];
	dict_val = &env->frame->tmpvar[dict];
	subscr_val = &env->frame->tmpvar[subscr];

	if (dict_val->type != NOCT_VALUE_DICT) {
		rt_error(env, N_TR("Not a dictionary."));
		return false;
	}
	if (subscr_val->type == NOCT_VALUE_INT) {
		index = (size_t)(uint32_t)subscr_val->val.i;
	} else if (subscr_val->type == NOCT_VALUE_LONG) {
		index = (size_t)(uint64_t)subscr_val->val.l;
	} else {
		rt_error(env, N_TR("Subscript not an integer."));
		return false;
	}

	/* Load the element. */
	if (!rt_get_dict_value_by_index(env, dict_val, index, dst_val))
		return false;

	return true;
}

/*
 * LOADSYMBOL helper.
 */
NOCT_DLL
bool
CDECL
noct_ex_loadsymbol_helper(
	NoctEnv *env,
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
 * STORESYMBOL helper.
 */
NOCT_DLL
bool
CDECL
noct_ex_storesymbol_helper(
	NoctEnv *env,
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
 * LOADDOT helper.
 */
NOCT_DLL
bool
CDECL
noct_ex_loaddot_helper(
	NoctEnv *env,
	int dst,
	int dict,
	const char *field,
	uint32_t field_len,
	uint32_t field_hash)
{
	if (env->frame->tmpvar[dict].type != NOCT_VALUE_DICT) {
		rt_error(env, N_TR("Not a dictionary."));
		return false;
	}

	if (!rt_get_dict_elem_with_hash(env,
					&env->frame->tmpvar[dict],
					field,
					field_len,
					field_hash,
					&env->frame->tmpvar[dst]))
		return false;

	return true;
}

/*
 * STOREDOT helper.
 */
NOCT_DLL
bool
CDECL
noct_ex_storedot_helper(
	NoctEnv *env,
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
	if (!rt_set_dict_elem_with_hash(env,
					dict_val,
					field,
					field_len,
					field_hash,
					val))
		return false;

	return true;
}

/*
 * CALL helper.
 */
NOCT_DLL
bool
CDECL
noct_ex_call_helper(
	NoctEnv *env,
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
	if (!rt_call(env, callee, (uint32_t)arg_count, &arg_val[0], &ret))
		return false;

	/* Store a return value. */
	env->frame->tmpvar[dst] = ret;

	return true;
}

/*
 * THISCALL helper.
 */
NOCT_DLL
bool
CDECL
noct_ex_thiscall_helper(
	NoctEnv *env,
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

	/* If not an intrinsic call, object must be a dictionary. */
	if (env->frame->tmpvar[obj].type != NOCT_VALUE_DICT) {
		rt_error(env, N_TR("Not a dictionary."));
		return false;
	}

	/* Get a function from a receiver object. */
	if (!rt_get_dict_elem_with_hash(env,
					&env->frame->tmpvar[obj],
					name,
					name_len,
					name_hash,
					&callee_value))
		return false;
	if (callee_value.type != NOCT_VALUE_FUNC) {
		rt_error(env, N_TR("Not a function."));
		return false;
	}
	callee = callee_value.val.func;

	/* Get values of arguments. */
	arg_count++;
	arg_val[0] = *obj_val;
	for (i = 1; i < arg_count; i++)
		arg_val[i] = env->frame->tmpvar[arg[i - 1]];

	/* Do call. */
	if (!rt_call(env, callee, (uint32_t)arg_count, &arg_val[0], &ret))
		return false;

	/* Store a return value. */
	env->frame->tmpvar[dst] = ret;

	return true;
}

/*
 * SAFEPOINT helper.
 */
NOCT_DLL
bool
CDECL
noct_ex_safepoint_helper(
	NoctEnv *env)
{
	/* Do call. */
	if (!rt_safepoint(env))
		return false;

	return true;
}
