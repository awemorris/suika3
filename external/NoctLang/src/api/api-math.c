/* -*- coding: utf-8; tab-width: 8; indent-tabs-mode: t; -*- */

/*
 * Copyright (c) 2025, Awe Morris. All rights reserved.
 */

/*
 * API: Console.*
 */

#include <noct/noct.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>

#ifdef _WIN32
#include <windows.h>
#endif

#define NEVER_COME_HERE		(0)

/* Forward declaration. */
static bool cfunc_Math_abs(struct rt_env *env);
static bool cfunc_Math_sqrt(struct rt_env *env);
static bool cfunc_Math_sin(struct rt_env *env);
static bool cfunc_Math_cos(struct rt_env *env);
static bool cfunc_Math_tan(struct rt_env *env);
static bool cfunc_Math_random(struct rt_env *env);

/* FFI table. */
struct ffi_item {
	const char *global_name;
	const char *field_name;
	int param_count;
	const char *param[NOCT_ARG_MAX];
	bool (*cfunc)(NoctEnv *env);
};
static struct ffi_item ffi_items[] = {
	{"__Math_abs",		"abs",		1,	{"x"},	cfunc_Math_abs},
	{"__Math_sqrt",		"sqrt",		1,	{"x"},	cfunc_Math_sqrt},
	{"__Math_sin",		"sin",		1,	{"x"},	cfunc_Math_sin},
	{"__Math_cos",		"cos",		1,	{"x"},	cfunc_Math_cos},
	{"__Math_tan",		"tan",		1,	{"x"},	cfunc_Math_tan},
	{"__Math_random",	"random",	0,	{NULL},	cfunc_Math_random},
};

	
/*
 * Register "Console.*" functions.
 */
bool noct_register_api_math(NoctEnv *env)
{
	NoctValue dict;
	int i;

	srand((unsigned int)time(NULL));

	/* Make a global variable "System". */
	if (!noct_make_empty_dict(env, &dict))
		return false;
	if (!noct_set_global(env, "Math", &dict))
		return false;

	/* Register functions. */
	for (i = 0; i < sizeof(ffi_items) / sizeof(struct ffi_item); i++) {
		NoctValue funcval;

		/* Register a cfunc. */
		if (!noct_register_cfunc(
			    env,
			    ffi_items[i].global_name,
			    ffi_items[i].param_count,
			    ffi_items[i].param,
			    ffi_items[i].cfunc,
			    NULL))
			return false;

		/* Get a function value. */
		if (!noct_get_global(env, ffi_items[i].global_name, &funcval))
			return false;

		/* Make a dictionary element. */
		if (!noct_set_dict_elem(
			    env,
			    &dict,
			    ffi_items[i].field_name,
			    &funcval))
			return false;
	}

	return true;
}

/* Math.abs() */
static bool
cfunc_Math_abs(
	NoctEnv *env)
{
	NoctValue x, ret;
	int type;
	int ival;
	float fval;

	noct_pin_local(env, 2, &x, &ret);

	if (!noct_get_arg(env, 0, &x))
		return false;
	if (!noct_get_value_type(env, &x, &type))
		return false;

	switch (type) {
	case NOCT_VALUE_INT:
		if (!noct_get_int(env, &x, &ival))
			return false;
		if (ival < 0)
			ival = -ival;
		if (!noct_set_return_make_int(env, &ret, ival))
			return false;
		break;
	case NOCT_VALUE_FLOAT:
		if (!noct_get_float(env, &x, &fval))
			return false;
		if (fval < 0)
			fval = -fval;
		if (!noct_set_return_make_float(env, &ret, fval))
			return false;
		break;
	default:
		noct_error(env, N_TR("Value is not a number."));
		return false;
	}

	noct_unpin_local(env, 2, &x, &ret);

	return true;
}

/* Math.sqrt() */
static bool
cfunc_Math_sqrt(
	NoctEnv *env)
{
	NoctValue x, ret;
	int type;
	int ival;
	float fval;

	noct_pin_local(env, 2, &x, &ret);

	if (!noct_get_arg(env, 0, &x))
		return false;
	if (!noct_get_value_type(env, &x, &type))
		return false;

	switch (type) {
	case NOCT_VALUE_INT:
		if (!noct_get_int(env, &x, &ival))
			return false;
		if (!noct_set_return_make_float(env, &ret, sqrtf((float)ival)))
			return false;
		break;
	case NOCT_VALUE_FLOAT:
		if (!noct_get_float(env, &x, &fval))
			return false;
		if (!noct_set_return_make_float(env, &ret, sqrtf(fval)))
			return false;
		break;
	default:
		noct_error(env, N_TR("Value is not a number."));
		return false;
	}

	noct_unpin_local(env, 2, &x, &ret);

	return true;
}


/* sin() */
static bool
cfunc_Math_sin(
	NoctEnv *env)
{
	NoctValue x, ret;
	float x_f;
	float sinx;

	noct_pin_local(env, 2, &x, &ret);

	if (!noct_get_arg_check_float(env, 0, &x, &x_f))
		return false;

	sinx = sinf(x_f);

	if (!noct_set_return_make_float(env, &ret, sinx))
		return false;

	noct_unpin_local(env, 2, &x, &ret);

	return true;
}

/* cos() */
static bool
cfunc_Math_cos(
	NoctEnv *env)
{
	NoctValue x, ret;
	float x_f;
	float cosx;

	noct_pin_local(env, 2, &x, &ret);

	if (!noct_get_arg_check_float(env, 0, &x, &x_f))
		return false;

	cosx = cosf(x_f);

	if (!noct_set_return_make_float(env, &ret, cosx))
		return false;

	noct_unpin_local(env, 2, &x, &ret);

	return true;
}

/* tan() */
static bool
cfunc_Math_tan(
	NoctEnv *env)
{
	NoctValue x, ret;
	float x_f;
	float tanx;

	noct_pin_local(env, 2, &x, &ret);

	if (!noct_get_arg_check_float(env, 0, &x, &x_f))
		return false;

	tanx = tanf(x_f);

	if (!noct_set_return_make_float(env, &ret, tanx))
		return false;

	noct_unpin_local(env, 2, &x, &ret);

	return true;
}

/* random() */
static bool
cfunc_Math_random(
	NoctEnv *env)
{
	NoctValue ret;
	float r;

	noct_pin_local(env, 1, &ret);

	r = (float)rand() / (float)RAND_MAX;

	if (!noct_set_return_make_float(env, &ret, r))
		return false;

	noct_unpin_local(env, 1, &ret);

	return true;
}

