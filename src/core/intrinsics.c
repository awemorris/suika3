/* -*- coding: utf-8; tab-width: 8; indent-tabs-mode: t; -*- */

/*
 * Noct Programming Language
 * Copyright (c) 2025, 2026, Awe Morris
 */

/*
 * Intrinsics Implementation
 */

#include "runtime.h"
#include "objectmodel.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <limits.h>
#include <assert.h>

#define NEVER_COME_HERE		0

static bool rt_intrin_Int_from(NoctEnv *env);
static bool rt_intrin_Long_from(NoctEnv *env);
static bool rt_intrin_Float_from(NoctEnv *env);
static bool rt_intrin_Double_from(NoctEnv *env);
static bool rt_intrin_Fast_index2(NoctEnv *env);
static bool rt_intrin_Fast_index3(NoctEnv *env);
static bool rt_intrin_Fast_index4(NoctEnv *env);
static bool rt_intrin_Fast_index5(NoctEnv *env);
static bool rt_intrin_Fast_index6(NoctEnv *env);
static bool rt_intrin_Fast_index7(NoctEnv *env);
static bool rt_intrin_Fast_index8(NoctEnv *env);
static bool rt_intrin_Fast_shape1(NoctEnv *env);
static bool rt_intrin_Fast_shape2(NoctEnv *env);
static bool rt_intrin_Fast_shape3(NoctEnv *env);
static bool rt_intrin_Fast_shape4(NoctEnv *env);
static bool rt_intrin_Fast_shape5(NoctEnv *env);
static bool rt_intrin_Fast_shape6(NoctEnv *env);
static bool rt_intrin_Fast_shape7(NoctEnv *env);
static bool rt_intrin_Fast_shape8(NoctEnv *env);
static bool rt_intrin_Fast_math(NoctEnv *env);
static bool rt_intrin_Fast_index(NoctEnv *env, int rank);
static bool rt_intrin_Fast_shape(NoctEnv *env, int rank);
static bool rt_fast_get_integer_arg(NoctEnv *env, uint32_t arg_index, bool extent, int64_t *number);
static bool rt_fast_numeric_double(NoctEnv *env, const NoctValue *value, double *number);
static bool rt_fast_convert_int(NoctEnv *env, const NoctValue *value, NoctValue *ret);
static bool rt_fast_convert_long(NoctEnv *env, const NoctValue *value, NoctValue *ret);
static bool rt_fast_convert_float(NoctEnv *env, const NoctValue *value, NoctValue *ret);
static bool rt_fast_convert_double(NoctEnv *env, const NoctValue *value, NoctValue *ret);
static bool rt_fast_abs(NoctEnv *env, const NoctValue *value, NoctValue *ret);
static double rt_fast_minmax_double(double a, double b, bool maximum);
static bool rt_fast_minmax(NoctEnv *env, const NoctValue *a, const NoctValue *b, bool maximum, NoctValue *ret);
static bool rt_fast_atan2(NoctEnv *env, const NoctValue *y, const NoctValue *x, NoctValue *ret);
static bool rt_fast_transcendental(NoctEnv *env, const char *name, const NoctValue *value, NoctValue *ret);
static bool rt_intrin_String_from(NoctEnv *env);
static bool rt_intrin_String_charCount(NoctEnv *env);
static bool rt_intrin_String_charAt(NoctEnv *env);
static bool rt_intrin_String_charCodeAt(NoctEnv *env);
static bool rt_intrin_String_toUpperCase(NoctEnv *env);
static bool rt_intrin_String_toLowerCase(NoctEnv *env);
static bool rt_intrin_String_substring(NoctEnv *env);
static bool rt_intrin_String_indexOf(NoctEnv *env);
static bool rt_intrin_Array_make(NoctEnv *env);
static bool rt_intrin_Array_size(NoctEnv *env);
static bool rt_intrin_Array_push(NoctEnv *env);
static bool rt_intrin_Array_pop(NoctEnv *env);
static bool rt_intrin_Array_resize(NoctEnv *env);
static bool rt_intrin_Array_copy(NoctEnv *env);
static bool rt_intrin_Dict_make(NoctEnv *env);
static bool rt_intrin_Dict_merge(NoctEnv *env);
static bool rt_intrin_Dict_freeze(NoctEnv *env);
static bool rt_intrin_Global_markConst(NoctEnv *env);
static bool rt_intrin_Dict_size(NoctEnv *env);
static bool rt_intrin_Dict_hasKey(NoctEnv *env);
static bool rt_intrin_Dict_remove(NoctEnv *env);
static bool rt_intrin_Dict_copy(NoctEnv *env);
static bool rt_intrin_Packed_int8(NoctEnv *env);
static bool rt_intrin_Packed_int16(NoctEnv *env);
static bool rt_intrin_Packed_int32(NoctEnv *env);
static bool rt_intrin_Packed_int64(NoctEnv *env);
static bool rt_intrin_Packed_uint8(NoctEnv *env);
static bool rt_intrin_Packed_uint16(NoctEnv *env);
static bool rt_intrin_Packed_uint32(NoctEnv *env);
static bool rt_intrin_Packed_uint64(NoctEnv *env);
static bool rt_intrin_Packed_float32(NoctEnv *env);
static bool rt_intrin_Packed_float64(NoctEnv *env);
static bool rt_intrin_Packed_size(NoctEnv *env);
static bool rt_intrin_Packed_type(NoctEnv *env);
static bool rt_intrin_Packed_copy(NoctEnv *env);
static bool rt_intrin_Packed_fill(NoctEnv *env);
static bool rt_intrin_Packed_toString(NoctEnv *env);
static bool rt_intrin_Packed_fromString(NoctEnv *env);
static bool rt_intrin_String_byteLength(NoctEnv *env);
static bool rt_intrin_String_chr(NoctEnv *env);
static size_t packed_elem_bytes(int type);
static bool rt_intrin_Math_abs(NoctEnv *env);
static bool rt_intrin_Math_sqrt(NoctEnv *env);
static bool rt_intrin_Math_sin(NoctEnv *env);
static bool rt_intrin_Math_cos(NoctEnv *env);
static bool rt_intrin_Math_tan(NoctEnv *env);
static bool rt_intrin_Math_random(NoctEnv *env);
static bool rt_intrin_Global_hasVariable(NoctEnv *env);
static bool rt_intrin_Global_get(NoctEnv *env);
static bool rt_intrin_Type_of(NoctEnv *env);
static bool rt_intrin_GC_youngGC(NoctEnv *env);
static bool rt_intrin_GC_oldGC(NoctEnv *env);
static bool rt_intrin_GC_compactGC(NoctEnv *env);

struct intrin_item {
	const char *package_name;
	const char *field_name;
	const char *reg_name;
	bool (*cfunc)(struct rt_env *rt);
	uint32_t param_count;
	const char *param[NOCT_ARG_MAX];
} intrin_items[] = {
	{"Int",		"from",		"Int.from",		rt_intrin_Int_from,		1, {"val"}},
	{"Long",	"from",		"Long.from",		rt_intrin_Long_from,		1, {"val"}},
	{"Float",	"from",		"Float.from",		rt_intrin_Float_from,		1, {"val"}},
	{"Double",	"from",		"Double.from",		rt_intrin_Double_from,		1, {"val"}},
	{"$Fast", "index2", "$Fast.index2", rt_intrin_Fast_index2, 4, {"i0", "d0", "i1", "d1"}},
	{"$Fast", "index3", "$Fast.index3", rt_intrin_Fast_index3, 6, {"i0", "d0", "i1", "d1", "i2", "d2"}},
	{"$Fast", "index4", "$Fast.index4", rt_intrin_Fast_index4, 8, {"i0", "d0", "i1", "d1", "i2", "d2", "i3", "d3"}},
	{"$Fast", "index5", "$Fast.index5", rt_intrin_Fast_index5, 10, {"i0", "d0", "i1", "d1", "i2", "d2", "i3", "d3", "i4", "d4"}},
	{"$Fast", "index6", "$Fast.index6", rt_intrin_Fast_index6, 12, {"i0", "d0", "i1", "d1", "i2", "d2", "i3", "d3", "i4", "d4", "i5", "d5"}},
	{"$Fast", "index7", "$Fast.index7", rt_intrin_Fast_index7, 14, {"i0", "d0", "i1", "d1", "i2", "d2", "i3", "d3", "i4", "d4", "i5", "d5", "i6", "d6"}},
	{"$Fast", "index8", "$Fast.index8", rt_intrin_Fast_index8, 16, {"i0", "d0", "i1", "d1", "i2", "d2", "i3", "d3", "i4", "d4", "i5", "d5", "i6", "d6", "i7", "d7"}},
	{"$Fast", "shape1", "$Fast.shape1", rt_intrin_Fast_shape1, 2, {"packed", "d0"}},
	{"$Fast", "shape2", "$Fast.shape2", rt_intrin_Fast_shape2, 3, {"packed", "d0", "d1"}},
	{"$Fast", "shape3", "$Fast.shape3", rt_intrin_Fast_shape3, 4, {"packed", "d0", "d1", "d2"}},
	{"$Fast", "shape4", "$Fast.shape4", rt_intrin_Fast_shape4, 5, {"packed", "d0", "d1", "d2", "d3"}},
	{"$Fast", "shape5", "$Fast.shape5", rt_intrin_Fast_shape5, 6, {"packed", "d0", "d1", "d2", "d3", "d4"}},
	{"$Fast", "shape6", "$Fast.shape6", rt_intrin_Fast_shape6, 7, {"packed", "d0", "d1", "d2", "d3", "d4", "d5"}},
	{"$Fast", "shape7", "$Fast.shape7", rt_intrin_Fast_shape7, 8, {"packed", "d0", "d1", "d2", "d3", "d4", "d5", "d6"}},
	{"$Fast", "shape8", "$Fast.shape8", rt_intrin_Fast_shape8, 9, {"packed", "d0", "d1", "d2", "d3", "d4", "d5", "d6", "d7"}},
	{"$FastMath", "min", "$FastMath.min", rt_intrin_Fast_math, 2, {"a", "b"}},
	{"$FastMath", "max", "$FastMath.max", rt_intrin_Fast_math, 2, {"a", "b"}},
	{"$FastMath", "abs", "$FastMath.abs", rt_intrin_Fast_math, 1, {"x"}},
	{"$FastMath", "sqrt", "$FastMath.sqrt", rt_intrin_Fast_math, 1, {"x"}},
	{"$FastMath", "sin", "$FastMath.sin", rt_intrin_Fast_math, 1, {"x"}},
	{"$FastMath", "cos", "$FastMath.cos", rt_intrin_Fast_math, 1, {"x"}},
	{"$FastMath", "tan", "$FastMath.tan", rt_intrin_Fast_math, 1, {"x"}},
	{"$FastMath", "asin", "$FastMath.asin", rt_intrin_Fast_math, 1, {"x"}},
	{"$FastMath", "acos", "$FastMath.acos", rt_intrin_Fast_math, 1, {"x"}},
	{"$FastMath", "atan", "$FastMath.atan", rt_intrin_Fast_math, 1, {"x"}},
	{"$FastMath", "atan2", "$FastMath.atan2", rt_intrin_Fast_math, 2, {"y", "x"}},
	{"$FastMath", "exp", "$FastMath.exp", rt_intrin_Fast_math, 1, {"x"}},
	{"$FastMath", "ln", "$FastMath.ln", rt_intrin_Fast_math, 1, {"x"}},
	{"$FastMath", "log2", "$FastMath.log2", rt_intrin_Fast_math, 1, {"x"}},
	{"$FastMath", "log10", "$FastMath.log10", rt_intrin_Fast_math, 1, {"x"}},
	{"$FastMath", "int", "$FastMath.int", rt_intrin_Fast_math, 1, {"x"}},
	{"$FastMath", "long", "$FastMath.long", rt_intrin_Fast_math, 1, {"x"}},
	{"$FastMath", "float", "$FastMath.float", rt_intrin_Fast_math, 1, {"x"}},
	{"$FastMath", "double", "$FastMath.double", rt_intrin_Fast_math, 1, {"x"}},
	{"String",	"from",		"String.from",		rt_intrin_String_from,		1, {"val"}},
	{"String",	"charCount",	"String.charCount",	rt_intrin_String_charCount,	1, {"s"}},
	{"String",	"charAt",	"String.charAt",	rt_intrin_String_charAt,	2, {"s", "index"}},
	{"String",	"charCodeAt",	"String.charCodeAt",	rt_intrin_String_charCodeAt,	2, {"s", "index"}},
	{"String",	"toUpperCase",	"String.toUpperCase",	rt_intrin_String_toUpperCase,	1, {"s"}},
	{"String",	"toLowerCase",	"String.toLowerCase",	rt_intrin_String_toLowerCase,	1, {"s"}},
	{"String",	"substring",	"String.substring",	rt_intrin_String_substring,	3, {"s", "start", "len"}},
	{"String",	"indexOf",	"String.indexOf",	rt_intrin_String_indexOf,	2, {"s", "search"}},
	{"String",	"byteLength",	"String.byteLength",	rt_intrin_String_byteLength,	1, {"s"}},
	{"String",	"chr",		"String.chr",		rt_intrin_String_chr,		1, {"codepoint"}},
	{"Array",	"make",		"Array.make",		rt_intrin_Array_make,		1, {"size"}},
	{"Array",	"size",		"Array.size",		rt_intrin_Array_size,		1, {"arr"}},
	{"Array",	"push",		"Array.push",		rt_intrin_Array_push,		2, {"arr", "val"}},
	{"Array",	"pop",		"Array.pop",		rt_intrin_Array_pop,		1, {"arr"}},
	{"Array",	"resize",	"Array.resize",		rt_intrin_Array_resize,		2, {"arr", "size"}},
	{"Array",	"copy",		"Array.copy",		rt_intrin_Array_copy,		1, {"arr"}},
	{"Dict",	"make",		"Dict.make",		rt_intrin_Dict_make,		0, {NULL}},
	{"Dict",	"merge",	"Dict.merge",		rt_intrin_Dict_merge,		2, {"src1", "src2"}},
	{"Dict",	"freeze",	"Dict.freeze",		rt_intrin_Dict_freeze,		1, {"dict"}},
	{"Dict",	"size",		"Dict.size",		rt_intrin_Dict_size,		1, {"dict"}},
	{"Dict",	"hasKey",	"Dict.hasKey",		rt_intrin_Dict_hasKey,		2, {"dict", "key"}},
	{"Dict",	"remove",	"Dict.remove",		rt_intrin_Dict_remove,		2, {"dict", "key"}},
	{"Dict",	"copy",		"Dict.copy",		rt_intrin_Dict_copy,		1, {"dict"}},
	{"Packed",	"int8",		"Packed.int8",		rt_intrin_Packed_int8,		1, {"size"}},
	{"Packed",	"int16",	"Packed.int16",		rt_intrin_Packed_int16,		1, {"size"}},
	{"Packed",	"int32",	"Packed.int32",		rt_intrin_Packed_int32,		1, {"size"}},
	{"Packed",	"int64",	"Packed.int64",		rt_intrin_Packed_int64,		1, {"size"}},
	{"Packed",	"uint8",	"Packed.uint8",		rt_intrin_Packed_uint8,		1, {"size"}},
	{"Packed",	"uint16",	"Packed.uint16",	rt_intrin_Packed_uint16,	1, {"size"}},
	{"Packed",	"uint32",	"Packed.uint32",	rt_intrin_Packed_uint32,	1, {"size"}},
	{"Packed",	"uint64",	"Packed.uint64",	rt_intrin_Packed_uint64,	1, {"size"}},
	{"Packed",	"float32",	"Packed.float32",	rt_intrin_Packed_float32,	1, {"size"}},
	{"Packed",	"float64",	"Packed.float64",	rt_intrin_Packed_float64,	1, {"size"}},
	{"Packed",	"size",		"Packed.size",		rt_intrin_Packed_size,		1, {"packed"}},
	{"Packed",	"type",		"Packed.type",		rt_intrin_Packed_type,		1, {"packed"}},
	{"Packed",	"copy",		"Packed.copy",		rt_intrin_Packed_copy,		5, {"dst", "dstIndex", "src", "srcIndex", "count"}},
	{"Packed",	"fill",		"Packed.fill",		rt_intrin_Packed_fill,		4, {"dst", "index", "count", "value"}},
	{"Packed",	"toString",	"Packed.toString",	rt_intrin_Packed_toString,	3, {"src", "offset", "length"}},
	{"Packed",	"fromString",	"Packed.fromString",	rt_intrin_Packed_fromString,	3, {"dst", "offset", "s"}},
	{"Math",	"abs",		"Math.abs",		rt_intrin_Math_abs,		1, {"x"}},
	{"Math",	"sqrt",		"Math.sqrt",		rt_intrin_Math_sqrt,		1, {"x"}},
	{"Math",	"sin",		"Math.sin",		rt_intrin_Math_sin,		1, {"x"}},
	{"Math",	"cos",		"Math.cos",		rt_intrin_Math_cos,		1, {"x"}},
	{"Math",	"tan",		"Math.tan",		rt_intrin_Math_tan,		1, {"x"}},
	{"Math",	"random",	"Math.random",		rt_intrin_Math_random,		0, {NULL}},
	{"Global",	"isSet",	"Global.isSet",		rt_intrin_Global_hasVariable,	1, {"name"}},
	{"Global",	"hasVariable",	"Global.hasVariable",	rt_intrin_Global_hasVariable,	1, {"name"}},
	{"Global",	"get",	"Global.get",		rt_intrin_Global_get,		1, {"name"}},
	{"Global",	"markConst",	"Global.markConst",	rt_intrin_Global_markConst,	1, {"name"}},
	{"Type",	"of",		"Type.of",		rt_intrin_Type_of,		1, {"val"}},
	{"GC",		"youngGC",	"GC.youngGC",		rt_intrin_GC_youngGC,		0, {NULL}},
	{"GC",		"oldGC",	"GC.oldGC",		rt_intrin_GC_oldGC,		0, {NULL}},
	{"GC",		"compactGC",	"GC.compactGC",		rt_intrin_GC_compactGC,		0, {NULL}},
};

static size_t get_string_length(const char *text);
static int utf8_to_utf32(const char *mbs, uint32_t *wc);

/*
 * Registers every built-in intrinsic function and package.
 */
bool
rt_register_intrinsics(
	struct rt_env *env)
{
	struct rt_value pkg;
	struct rt_func *func;
	const char *last_pkg_name;
	struct rt_value val;
	int i;

	/* For each table entry in intrin_items: */
	last_pkg_name = NULL;
	for (i = 0; i < (int)(sizeof(intrin_items) / sizeof(struct intrin_item)); i++) {
		/* Register a function at the entry. */
		if (!noct_register_cfunc(env,
					 intrin_items[i].reg_name,
					 intrin_items[i].param_count,
					 intrin_items[i].param,
					 intrin_items[i].cfunc,
					 &func))
			return false;

		/* If the function is not in a package. */
		if (intrin_items[i].package_name == NULL)
			continue;

		/* Load the package. */
		if (last_pkg_name == NULL ||
		    strcmp(intrin_items[i].package_name, last_pkg_name) != 0) {
			last_pkg_name = intrin_items[i].package_name;
			if (!rt_check_global(env, last_pkg_name)) {
				if (!rt_make_empty_dict(env, &pkg))
					return false;
				if (!rt_set_global(env, last_pkg_name, &pkg))
					return false;
			} else {
				if (!rt_get_global(env, last_pkg_name, &pkg))
					return false;
			}
		}

		/* Add the function to the package */
		val.type = NOCT_VALUE_FUNC;
		val.val.func = func;
		if (!rt_set_dict_elem_cstr(env, &pkg, intrin_items[i].field_name, &val))
			return false;
	}

	/* Freeze every package whose identity the compiler relies upon. */
	if (!rt_get_global(env, "Int", &pkg))
		return false;
	if (!om_freeze_dict(env, &pkg))
		return false;
	if (!rt_mark_global_const(env, "Int"))
		return false;

	if (!rt_get_global(env, "Float", &pkg))
		return false;
	if (!om_freeze_dict(env, &pkg))
		return false;
	if (!rt_mark_global_const(env, "Float"))
		return false;

	if (!rt_get_global(env, "$Fast", &pkg))
		return false;
	if (!om_freeze_dict(env, &pkg))
		return false;
	if (!rt_mark_global_const(env, "$Fast"))
		return false;

	if (!rt_get_global(env, "$FastMath", &pkg))
		return false;
	if (!om_freeze_dict(env, &pkg))
		return false;
	if (!rt_mark_global_const(env, "$FastMath"))
		return false;

	return true;
}

/* Compute a row-major offset for two axes. */
static bool
rt_intrin_Fast_index2(
	NoctEnv *env)
{
	return rt_intrin_Fast_index(env, 2);
}

/* Compute a row-major offset for three axes. */
static bool
rt_intrin_Fast_index3(
	NoctEnv *env)
{
	return rt_intrin_Fast_index(env, 3);
}

/* Compute a row-major offset for four axes. */
static bool
rt_intrin_Fast_index4(
	NoctEnv *env)
{
	return rt_intrin_Fast_index(env, 4);
}

/* Compute a row-major offset for five axes. */
static bool
rt_intrin_Fast_index5(
	NoctEnv *env)
{
	return rt_intrin_Fast_index(env, 5);
}

/* Compute a row-major offset for six axes. */
static bool
rt_intrin_Fast_index6(
	NoctEnv *env)
{
	return rt_intrin_Fast_index(env, 6);
}

/* Compute a row-major offset for seven axes. */
static bool
rt_intrin_Fast_index7(
	NoctEnv *env)
{
	return rt_intrin_Fast_index(env, 7);
}

/* Compute a row-major offset for eight axes. */
static bool
rt_intrin_Fast_index8(
	NoctEnv *env)
{
	return rt_intrin_Fast_index(env, 8);
}

/* Compute a checked row-major offset for the requested rank. */
static bool
rt_intrin_Fast_index(
	NoctEnv *env,
	int rank)
{
	NoctValue ret;
	uint64_t offset;
	int axis;

	offset = 0;

	/* Fold each axis into the row-major offset. */
	for (axis = 0; axis < rank; axis++) {
		int64_t index;
		int64_t extent;

		if (!rt_fast_get_integer_arg(env, (uint32_t)(axis * 2), false, &index))
			return false;
		if (!rt_fast_get_integer_arg(env, (uint32_t)(axis * 2 + 1), true, &extent))
			return false;

		if (extent <= 0) {
			noct_error(env, N_TR("A __fast shape extent must be positive."));
			return false;
		}

		if (index < 0 || index >= extent) {
			noct_error(env, N_TR("Index out of bounds."));
			return false;
		}

		if (offset > ((uint64_t)INT64_MAX - (uint64_t)index) /
			     (uint64_t)extent) {
			noct_error(env, N_TR("__fast row-major index overflow."));
			return false;
		}

		offset = offset * (uint64_t)extent + (uint64_t)index;
	}

	memset(&ret, 0, sizeof(ret));
	ret.type = NOCT_VALUE_LONG;
	ret.val.l = (int64_t)offset;

	return noct_set_return(env, &ret);
}

/* Check the exact element count for a one-dimensional Packed value. */
static bool
rt_intrin_Fast_shape1(
	NoctEnv *env)
{
	return rt_intrin_Fast_shape(env, 1);
}

/* Check the exact element count for a two-dimensional Packed value. */
static bool
rt_intrin_Fast_shape2(
	NoctEnv *env)
{
	return rt_intrin_Fast_shape(env, 2);
}

/* Check the exact element count for a three-dimensional Packed value. */
static bool
rt_intrin_Fast_shape3(
	NoctEnv *env)
{
	return rt_intrin_Fast_shape(env, 3);
}

/* Check the exact element count for a four-dimensional Packed value. */
static bool
rt_intrin_Fast_shape4(
	NoctEnv *env)
{
	return rt_intrin_Fast_shape(env, 4);
}

/* Check the exact element count for a five-dimensional Packed value. */
static bool
rt_intrin_Fast_shape5(
	NoctEnv *env)
{
	return rt_intrin_Fast_shape(env, 5);
}

/* Check the exact element count for a six-dimensional Packed value. */
static bool
rt_intrin_Fast_shape6(
	NoctEnv *env)
{
	return rt_intrin_Fast_shape(env, 6);
}

/* Check the exact element count for a seven-dimensional Packed value. */
static bool
rt_intrin_Fast_shape7(
	NoctEnv *env)
{
	return rt_intrin_Fast_shape(env, 7);
}

/* Check the exact element count for an eight-dimensional Packed value. */
static bool
rt_intrin_Fast_shape8(
	NoctEnv *env)
{
	return rt_intrin_Fast_shape(env, 8);
}

/* Check one Packed value against an exact runtime shape. */
static bool
rt_intrin_Fast_shape(
	NoctEnv *env,
	int rank)
{
	NoctValue packed_value;
	NoctValue ret;
	size_t element_count;
	int axis;

	if (!noct_get_arg(env, 0, &packed_value))
		return false;
	if (packed_value.type != NOCT_VALUE_PACKED ||
	    packed_value.val.packed == NULL) {
		noct_error(env, N_TR("__fast call: argument has the wrong packed element type."));
		return false;
	}

	element_count = 1;

	/* Multiply every positive extent into the exact element count. */
	for (axis = 0; axis < rank; axis++) {
		int64_t extent;

		if (!rt_fast_get_integer_arg(
			env,
			(uint32_t)axis + 1,
			true,
			&extent)) {
			return false;
		}
		if (extent <= 0) {
			noct_error(env, N_TR("__fast call: shape extents must be positive."));
			return false;
		}
		if ((uint64_t)extent > (uint64_t)SIZE_MAX ||
		    element_count > SIZE_MAX / (size_t)extent) {
			noct_error(env, N_TR("__fast call: shape element count overflow."));
			return false;
		}

		element_count *= (size_t)extent;
	}

	if (packed_value.val.packed->elem_size != element_count) {
		noct_error(env, N_TR("__fast call: argument does not match the exact shape."));
		return false;
	}

	memset(&ret, 0, sizeof(ret));
	ret.type = NOCT_VALUE_INT;

	return noct_set_return(env, &ret);
}

/* Read one integer index or extent argument. */
static bool
rt_fast_get_integer_arg(
	NoctEnv *env,
	uint32_t arg_index,
	bool extent,
	int64_t *number)
{
	NoctValue value;

	if (!noct_get_arg(env, arg_index, &value))
		return false;

	/* Convert either supported integer representation. */
	switch (value.type) {
	case NOCT_VALUE_INT:
		*number = value.val.i;
		return true;
	case NOCT_VALUE_LONG:
		*number = value.val.l;
		return true;
	default:
		break;
	}

	if (extent) {
		noct_error(env, N_TR("A __fast shape extent must be int or long."));
	} else {
		noct_error(env, N_TR("A __fast array index must be int or long."));
	}

	return false;
}

/* Dispatch one scalar operation used by a __fast function. */
static bool
rt_intrin_Fast_math(
	NoctEnv *env)
{
	const char *name;
	NoctValue a;
	NoctValue b;
	NoctValue ret;

	name = env->frame->func->name;
	if (strncmp(name, "$FastMath.", 10) != 0) {
		noct_error(env, N_TR("Unknown __fast numeric intrinsic."));
		return false;
	}
	name += 10;

	if (!noct_get_arg(env, 0, &a))
		return false;

	memset(&ret, 0, sizeof(ret));

	if (strcmp(name, "int") == 0) {
		if (!rt_fast_convert_int(env, &a, &ret))
			return false;

		return noct_set_return(env, &ret);
	}

	if (strcmp(name, "long") == 0) {
		if (!rt_fast_convert_long(env, &a, &ret))
			return false;

		return noct_set_return(env, &ret);
	}

	if (strcmp(name, "float") == 0) {
		if (!rt_fast_convert_float(env, &a, &ret))
			return false;

		return noct_set_return(env, &ret);
	}

	if (strcmp(name, "double") == 0) {
		if (!rt_fast_convert_double(env, &a, &ret))
			return false;

		return noct_set_return(env, &ret);
	}

	if (strcmp(name, "abs") == 0) {
		if (!rt_fast_abs(env, &a, &ret))
			return false;

		return noct_set_return(env, &ret);
	}

	if (strcmp(name, "min") == 0) {
		if (!noct_get_arg(env, 1, &b))
			return false;
		if (!rt_fast_minmax(env, &a, &b, false, &ret))
			return false;

		return noct_set_return(env, &ret);
	}

	if (strcmp(name, "max") == 0) {
		if (!noct_get_arg(env, 1, &b))
			return false;
		if (!rt_fast_minmax(env, &a, &b, true, &ret))
			return false;

		return noct_set_return(env, &ret);
	}

	if (strcmp(name, "atan2") == 0) {
		if (!noct_get_arg(env, 1, &b))
			return false;
		if (!rt_fast_atan2(env, &a, &b, &ret))
			return false;

		return noct_set_return(env, &ret);
	}

	if (!rt_fast_transcendental(env, name, &a, &ret))
		return false;

	return noct_set_return(env, &ret);
}

/* Convert one numeric value to double for scalar calculations. */
static bool
rt_fast_numeric_double(
	NoctEnv *env,
	const NoctValue *value,
	double *number)
{
	/* Convert each numeric representation without changing its tag. */
	switch (value->type) {
	case NOCT_VALUE_INT:
		*number = (double)value->val.i;
		return true;
	case NOCT_VALUE_LONG:
		*number = (double)value->val.l;
		return true;
	case NOCT_VALUE_FLOAT:
		*number = (double)value->val.f;
		return true;
	case NOCT_VALUE_DOUBLE:
		*number = value->val.lf;
		return true;
	default:
		noct_error(env, N_TR("A __fast numeric intrinsic requires numeric arguments."));
		return false;
	}
}

/* Convert one numeric value to int without an out-of-range cast. */
static bool
rt_fast_convert_int(
	NoctEnv *env,
	const NoctValue *value,
	NoctValue *ret)
{
	double number;

	if (value->type == NOCT_VALUE_INT) {
		ret->type = NOCT_VALUE_INT;
		ret->val.i = value->val.i;
		return true;
	}

	if (value->type == NOCT_VALUE_LONG) {
		if (value->val.l < INT32_MIN || value->val.l > INT32_MAX) {
			noct_error(env, N_TR("Value is out of range for int."));
			return false;
		}

		ret->type = NOCT_VALUE_INT;
		ret->val.i = (int32_t)value->val.l;
		return true;
	}

	if (!rt_fast_numeric_double(env, value, &number))
		return false;

	if (!(number >= -2147483648.0 && number < 2147483648.0)) {
		noct_error(env, N_TR("Value is out of range for int."));
		return false;
	}

	ret->type = NOCT_VALUE_INT;
	ret->val.i = (int32_t)number;

	return true;
}

/* Convert one numeric value to long without an out-of-range cast. */
static bool
rt_fast_convert_long(
	NoctEnv *env,
	const NoctValue *value,
	NoctValue *ret)
{
	double number;
	double limit;

	if (value->type == NOCT_VALUE_INT) {
		ret->type = NOCT_VALUE_LONG;
		ret->val.l = value->val.i;
		return true;
	}

	if (value->type == NOCT_VALUE_LONG) {
		ret->type = NOCT_VALUE_LONG;
		ret->val.l = value->val.l;
		return true;
	}

	if (!rt_fast_numeric_double(env, value, &number))
		return false;

	limit = 9223372036854775808.0;
	if (!(number >= -limit && number < limit)) {
		noct_error(env, N_TR("Value is out of range for long."));
		return false;
	}

	ret->type = NOCT_VALUE_LONG;
	ret->val.l = (int64_t)number;

	return true;
}

/* Convert one numeric value to float. */
static bool
rt_fast_convert_float(
	NoctEnv *env,
	const NoctValue *value,
	NoctValue *ret)
{
	double number;

	if (!rt_fast_numeric_double(env, value, &number))
		return false;

	ret->type = NOCT_VALUE_FLOAT;
	ret->val.f = (float)number;

	return true;
}

/* Convert one numeric value to double. */
static bool
rt_fast_convert_double(
	NoctEnv *env,
	const NoctValue *value,
	NoctValue *ret)
{
	double number;

	if (!rt_fast_numeric_double(env, value, &number))
		return false;

	ret->type = NOCT_VALUE_DOUBLE;
	ret->val.lf = number;

	return true;
}

/* Compute an absolute value while preserving the input type. */
static bool
rt_fast_abs(
	NoctEnv *env,
	const NoctValue *value,
	NoctValue *ret)
{
	ret->type = value->type;

	/* Handle each supported numeric representation. */
	switch (value->type) {
	case NOCT_VALUE_INT:
		if (value->val.i == INT32_MIN) {
			ret->val.i = INT32_MIN;
		} else if (value->val.i < 0) {
			ret->val.i = -value->val.i;
		} else {
			ret->val.i = value->val.i;
		}
		break;
	case NOCT_VALUE_LONG:
		if (value->val.l == INT64_MIN) {
			ret->val.l = INT64_MIN;
		} else if (value->val.l < 0) {
			ret->val.l = -value->val.l;
		} else {
			ret->val.l = value->val.l;
		}
		break;
	case NOCT_VALUE_FLOAT:
		ret->val.f = (float)fabs((double)value->val.f);
		break;
	case NOCT_VALUE_DOUBLE:
		ret->val.lf = fabs(value->val.lf);
		break;
	default:
		noct_error(env, N_TR("A __fast numeric intrinsic requires numeric arguments."));
		return false;
	}

	return true;
}

/* Select a floating-point minimum or maximum with stable NaN and zero rules. */
static double
rt_fast_minmax_double(
	double a,
	double b,
	bool maximum)
{
	bool a_negative;
	bool b_negative;

	if (a != a)
		return b;
	if (b != b)
		return a;

	if (a == 0.0 && b == 0.0) {
		a_negative = signbit(a) != 0;
		b_negative = signbit(b) != 0;

		if (maximum) {
			if (a_negative && !b_negative)
				return b;

			return a;
		}

		if (!a_negative && b_negative)
			return b;

		return a;
	}

	if (maximum) {
		if (a > b)
			return a;

		return b;
	}

	if (a < b)
		return a;

	return b;
}

/* Select a minimum or maximum while preserving the operand type. */
static bool
rt_fast_minmax(
	NoctEnv *env,
	const NoctValue *a,
	const NoctValue *b,
	bool maximum,
	NoctValue *ret)
{
	if (a->type != b->type) {
		noct_error(env, N_TR("A __fast binary intrinsic requires operands of the same type."));
		return false;
	}

	ret->type = a->type;

	/* Compare each supported numeric representation. */
	switch (a->type) {
	case NOCT_VALUE_INT:
		if (maximum) {
			if (a->val.i > b->val.i)
				ret->val.i = a->val.i;
			else
				ret->val.i = b->val.i;
		} else {
			if (a->val.i < b->val.i)
				ret->val.i = a->val.i;
			else
				ret->val.i = b->val.i;
		}
		break;
	case NOCT_VALUE_LONG:
		if (maximum) {
			if (a->val.l > b->val.l)
				ret->val.l = a->val.l;
			else
				ret->val.l = b->val.l;
		} else {
			if (a->val.l < b->val.l)
				ret->val.l = a->val.l;
			else
				ret->val.l = b->val.l;
		}
		break;
	case NOCT_VALUE_FLOAT:
		ret->val.f = (float)rt_fast_minmax_double(
			(double)a->val.f,
			(double)b->val.f,
			maximum);
		break;
	case NOCT_VALUE_DOUBLE:
		ret->val.lf = rt_fast_minmax_double(
			a->val.lf,
			b->val.lf,
			maximum);
		break;
	default:
		noct_error(env, N_TR("A __fast numeric intrinsic requires numeric arguments."));
		return false;
	}

	return true;
}

/* Compute atan2 for same-typed floating-point operands. */
static bool
rt_fast_atan2(
	NoctEnv *env,
	const NoctValue *y,
	const NoctValue *x,
	NoctValue *ret)
{
	if (y->type != x->type) {
		noct_error(env, N_TR("A __fast binary intrinsic requires operands of the same type."));
		return false;
	}

	/* Preserve the common floating-point operand type. */
	switch (y->type) {
	case NOCT_VALUE_FLOAT:
		ret->type = NOCT_VALUE_FLOAT;
		ret->val.f = (float)atan2((double)y->val.f, (double)x->val.f);
		break;
	case NOCT_VALUE_DOUBLE:
		ret->type = NOCT_VALUE_DOUBLE;
		ret->val.lf = atan2(y->val.lf, x->val.lf);
		break;
	default:
		noct_error(env, N_TR("atan2() requires float or double operands."));
		return false;
	}

	return true;
}

/* Compute one same-typed floating-point transcendental operation. */
static bool
rt_fast_transcendental(
	NoctEnv *env,
	const char *name,
	const NoctValue *value,
	NoctValue *ret)
{
	double number;
	double result;

	if (value->type != NOCT_VALUE_FLOAT &&
	    value->type != NOCT_VALUE_DOUBLE) {
		noct_error(env, N_TR("A __fast transcendental intrinsic requires float or double."));
		return false;
	}

	if (!rt_fast_numeric_double(env, value, &number))
		return false;

	if (strcmp(name, "sqrt") == 0)
		result = sqrt(number);
	else if (strcmp(name, "sin") == 0)
		result = sin(number);
	else if (strcmp(name, "cos") == 0)
		result = cos(number);
	else if (strcmp(name, "tan") == 0)
		result = tan(number);
	else if (strcmp(name, "asin") == 0)
		result = asin(number);
	else if (strcmp(name, "acos") == 0)
		result = acos(number);
	else if (strcmp(name, "atan") == 0)
		result = atan(number);
	else if (strcmp(name, "exp") == 0)
		result = exp(number);
	else if (strcmp(name, "ln") == 0)
		result = log(number);
	else if (strcmp(name, "log2") == 0)
		result = log(number) / log(2.0);
	else if (strcmp(name, "log10") == 0)
		result = log10(number);
	else {
		noct_error(env, N_TR("Unknown __fast numeric intrinsic."));
		return false;
	}

	ret->type = value->type;
	if (value->type == NOCT_VALUE_FLOAT)
		ret->val.f = (float)result;
	else
		ret->val.lf = result;

	return true;
}

/*
 * Int.from(val)
 */
static bool
rt_intrin_Int_from(
	NoctEnv *env)
{
	struct rt_value val, ret;
	int type;

	/* Pin the local variables to prevent them collected by GC. */
	noct_pin_local(env, 2, &val, &ret);

	/* Get the argument "val". */
	if (!noct_get_arg(env, 0, &val))
		return false;

	/* Check the value type for "val". */
	if (!noct_get_value_type(env, &val, &type))
		return false;

	/* Branch by the type. */
	switch (type) {
	case NOCT_VALUE_INT:
		/* If it is an int, just return it. */
		if (!noct_set_return(env, &val))
			return false;
		break;
	case NOCT_VALUE_LONG:
	{
		/* If it is a long, trancate it. */
		int64_t val_l;
		if (!noct_get_long(env, &val, &val_l))
			return false;
		if (!noct_set_return_make_int(env, &ret, (int32_t)(uint32_t)val_l))
			return false;
		break;
	}
	case NOCT_VALUE_FLOAT:
	{
		/* If it is a float, floor it. */
		float val_f;
		if (!noct_get_float(env, &val, &val_f))
			return false;
		if (!noct_set_return_make_int(env, &ret, (int32_t)val_f))
			return false;
		break;
	}
	case NOCT_VALUE_DOUBLE:
	{
		/* If it is a double, floor it. */
		double val_lf;
		if (!noct_get_double(env, &val, &val_lf))
			return false;
		if (!noct_set_return_make_int(env, &ret, (int32_t)val_lf))
			return false;
		break;
	}
	case NOCT_VALUE_STRING:
	{
		/* If it is a string, call atoi(). */
		const char *val_s;
		if (!noct_get_string(env, &val, &val_s))
			return false;
		if (!noct_set_return_make_int(env, &ret, atoi(val_s)))
			return false;
		break;
	}
	default:
		noct_error(env, N_TR("Value is not a number or a string."));
		return false;
	}

	/* Unpin the local variables to allow them collected by GC. */
	noct_unpin_local(env, 2, &val, &ret);

	return true;
}

/*
 * Long.from(val)
 */
static bool
rt_intrin_Long_from(
	NoctEnv *env)
{
	struct rt_value val, ret;
	int type;

	/* Pin the local variables to prevent them collected by GC. */
	noct_pin_local(env, 2, &val, &ret);

	/* Get the argument "val". */
	if (!noct_get_arg(env, 0, &val))
		return false;

	/* Check the value type for "val". */
	if (!noct_get_value_type(env, &val, &type))
		return false;

	/* Branch by the type. */
	switch (type) {
	case NOCT_VALUE_INT:
	{
		/* If it is an int, widen it. */
		int val_i;
		if (!noct_get_int(env, &val, &val_i))
			return false;
		if (!noct_set_return_make_long(env, &ret, val_i))
			return false;
		break;
	}
	case NOCT_VALUE_LONG:
		/* If it is a long, just return it. */
		if (!noct_set_return(env, &val))
			return false;
		break;
	case NOCT_VALUE_FLOAT:
	{
		/* If it is a float, floor it. */
		float val_f;
		if (!noct_get_float(env, &val, &val_f))
			return false;
		if (!noct_set_return_make_long(env, &ret, (int64_t)val_f))
			return false;
		break;
	}
	case NOCT_VALUE_DOUBLE:
	{
		/* If it is a double, floor it. */
		double val_lf;
		if (!noct_get_double(env, &val, &val_lf))
			return false;
		if (!noct_set_return_make_long(env, &ret, (int64_t)val_lf))
			return false;
		break;
	}
	case NOCT_VALUE_STRING:
	{
		/* If it is a string, call atoi(). */
		const char *val_s;
		if (!noct_get_string(env, &val, &val_s))
			return false;
		if (!noct_set_return_make_long(env, &ret, atoll(val_s)))
			return false;
		break;
	}
	default:
		noct_error(env, N_TR("Value is not a number or a string."));
		return false;
	}

	/* Unpin the local variables to allow them collected by GC. */
	noct_unpin_local(env, 2, &val, &ret);

	return true;
}

/*
 * Float.from(val)
 */
static bool
rt_intrin_Float_from(
	NoctEnv *env)
{
	struct rt_value val, ret;
	int type;

	/* Pin the local variables to prevent them collected by GC. */
	noct_pin_local(env, 2, &val, &ret);

	/* Get the argument "val". */
	if (!noct_get_arg(env, 0, &val))
		return false;

	/* Check the value type for "val". */
	if (!noct_get_value_type(env, &val, &type))
		return false;

	/* Branch by the type. */
	switch (type) {
	case NOCT_VALUE_INT:
	{
		/* If it is an int, convert to float. */
		int val_i;
		if (!noct_get_int(env, &val, &val_i))
			return false;
		if (!noct_set_return_make_float(env, &ret, (float)val_i))
			return false;
		break;
	}
	case NOCT_VALUE_LONG:
	{
		/* If it is a long, convert to float. */
		int64_t val_l;
		if (!noct_get_long(env, &val, &val_l))
			return false;
		if (!noct_set_return_make_float(env, &ret, (float)val_l))
			return false;
		break;
	}
	case NOCT_VALUE_FLOAT:
		/* If it is a float, just set it as a return value. */
		if (!noct_set_return(env, &val))
			return false;
		break;
	case NOCT_VALUE_DOUBLE:
	{
		/* If it is a double, floor it. */
		double val_lf;
		if (!noct_get_double(env, &val, &val_lf))
			return false;
		if (!noct_set_return_make_float(env, &ret, (float)val_lf))
			return false;
		break;
	}
	case NOCT_VALUE_STRING:
	{
		/* If it is a string, call atoi(). */
		const char *val_s;
		if (!noct_get_string(env, &val, &val_s))
			return false;
		if (!noct_set_return_make_float(env, &ret, (float)atof(val_s)))
			return false;
		break;
	}
	default:
		noct_error(env, N_TR("Value is not a number or a string."));
		return false;
	}

	/* Unpin the local variables to allow them collected by GC. */
	noct_unpin_local(env, 2, &val, &ret);

	return true;
}

/*
 * Double.from(val)
 */
static bool
rt_intrin_Double_from(
	NoctEnv *env)
{
	struct rt_value val, ret;
	int type;

	/* Pin the local variables to prevent them collected by GC. */
	noct_pin_local(env, 2, &val, &ret);

	/* Get the argument "val". */
	if (!noct_get_arg(env, 0, &val))
		return false;

	/* Check the value type for "val". */
	if (!noct_get_value_type(env, &val, &type))
		return false;

	/* Branch by the type. */
	switch (type) {
	case NOCT_VALUE_INT:
	{
		/* If it is an int, convert to double. */
		int val_i;
		if (!noct_get_int(env, &val, &val_i))
			return false;
		if (!noct_set_return_make_double(env, &ret, (double)val_i))
			return false;
		break;
	}
	case NOCT_VALUE_LONG:
	{
		/* If it is a long, convert to float. */
		int64_t val_l;
		if (!noct_get_long(env, &val, &val_l))
			return false;
		if (!noct_set_return_make_double(env, &ret, (double)val_l))
			return false;
		break;
	}
	case NOCT_VALUE_FLOAT:
	{
		/* If it is a float, convert to double. */
		float val_f;
		if (!noct_get_float(env, &val, &val_f))
			return false;
		if (!noct_set_return_make_double(env, &ret, (double)val_f))
			return false;
		break;
	}
	case NOCT_VALUE_DOUBLE:
		/* If it is a double, just set it as a return value. */
		if (!noct_set_return(env, &val))
			return false;
		break;
	case NOCT_VALUE_STRING:
	{
		/* If it is a string, call atoi(). */
		const char *val_s;
		if (!noct_get_string(env, &val, &val_s))
			return false;
		if (!noct_set_return_make_double(env, &ret, atof(val_s)))
			return false;
		break;
	}
	default:
		noct_error(env, N_TR("Value is not a number or a string."));
		return false;
	}

	/* Unpin the local variables to allow them collected by GC. */
	noct_unpin_local(env, 2, &val, &ret);

	return true;
}

/*
 * String.from(val)
 */
static bool
rt_intrin_String_from(
	NoctEnv *env)
{
	struct rt_value val, ret;
	int type;
	char buf[128];

	/* Pin the local variables to prevent them collected by GC. */
	noct_pin_local(env, 2, &val, &ret);

	/* Get the argument "val". */
	if (!noct_get_arg(env, 0, &val))
		return false;

	/* Check the value type for "val". */
	if (!noct_get_value_type(env, &val, &type))
		return false;

	/* Branch by the type. */
	switch (type) {
	case NOCT_VALUE_INT:
	{
		/* If it is an int, convert to double. */
		int val_i;
		if (!noct_get_int(env, &val, &val_i))
			return false;
		snprintf(buf, sizeof(buf), "%d", val_i);
		if (!noct_set_return_make_string(env, &ret, buf))
			return false;
		break;
	}
	case NOCT_VALUE_LONG:
	{
		/* If it is a long, convert to float. */
		int64_t val_l;
		if (!noct_get_long(env, &val, &val_l))
			return false;
		snprintf(buf, sizeof(buf), "%" PRId64, val_l);
		if (!noct_set_return_make_string(env, &ret, buf))
			return false;
		break;
	}
	case NOCT_VALUE_FLOAT:
	{
		/* If it is a float, convert to double. */
		float val_f;
		if (!noct_get_float(env, &val, &val_f))
			return false;
		snprintf(buf, sizeof(buf), "%.7g", val_f);
		if (!noct_set_return_make_string(env, &ret, buf))
			return false;
		break;
	}
	case NOCT_VALUE_DOUBLE:
	{
		/* If it is a double, just set it as a return value. */
		double val_lf;
		if (!noct_get_double(env, &val, &val_lf))
			return false;
		snprintf(buf, sizeof(buf), "%.15g", val_lf);
		if (!noct_set_return_make_string(env, &ret, buf))
			return false;
		break;
	}
	case NOCT_VALUE_STRING:
	{
		if (!noct_set_return(env, &val))
			return false;
		break;
	}
	default:
		noct_error(env, N_TR("Value is not a number or a string."));
		return false;
	}

	/* Unpin the local variables to allow them collected by GC. */
	noct_unpin_local(env, 2, &val, &ret);

	return true;
}

/*
 * String.charCount(s)
 */
static bool
rt_intrin_String_charCount
(NoctEnv *env)
{
	NoctValue str, ret;
	const char *str_s;
	size_t len;

	/* Pin the local variables to prevent them collected by GC. */
	noct_pin_local(env, 2, &str, &ret);

	/* Get the arg "str". */
	if (!noct_get_arg_check_string(env, 0, &str, &str_s))
		return false;

	/* Get the length of str. */
	len = get_string_length(str_s);

	/* Set the length as a return value. */
	if (!noct_set_return_make_int_long(env, &ret, len))
		return false;

	/* Unpin the local variables to allow them collected by GC. */
	noct_unpin_local(env, 2, &str, &ret);

	return true;
}

/*
 * String.charAt(s, i)
 */
static bool
rt_intrin_String_charAt(
	NoctEnv *env)
{
	NoctValue str, index, ret;
	struct rt_string *rts;
	const char *str_s;
	size_t index_i;
	const char *s;
	char d[8];
	size_t i, ofs;
	int mblen;

	noct_pin_local(env, 3, &str, &index, &ret);

	/* Get the argument "s". */
	if (!noct_get_arg_check_string(env, 0, &str, &str_s))
		return false;

	/* Get the argument "i". */
	if (!noct_get_arg_check_int_long(env, 1, &index, &index_i))
		return false;

	/*
         * Iterate through the UTF-8 string to locate and extract the
         * character at 'index_i'. Handles multi-byte characters
         * correctly using utf8_to_utf32().
         *
         * The scan starts from the string's cached (character, offset)
         * pair when that lands at or before the character wanted. A
         * caller walking a string in order -- which is what every parser
         * does -- then advances one character per call instead of
         * counting from the front each time, and the walk costs O(n)
         * rather than O(n^2).
         */
	rts = str.val.str;
	s = str_s;
	i = 0;
	ofs = 0;
	if (rts->cache_index != 0 && rts->cache_index <= index_i) {
		i = rts->cache_index;
		ofs = rts->cache_ofs;
		s = str_s + ofs;
	}
	d[0] = '\0';
	while (*s != '\0' && i <= index_i) {
		uint32_t codepoint;

		mblen = utf8_to_utf32(s, &codepoint);
		if (mblen <= 0 || mblen > 4) {
			/* UTF-8 error. */
			return false;
		}

		if (i == index_i) {
			/* Succeeded. */
			strncpy(d, &str_s[ofs], (size_t)mblen);
			d[mblen] = '\0';
			rts->cache_index = i;
			rts->cache_ofs = ofs;
			break;
		}

		s += mblen;
		ofs += (uint32_t)mblen;
		i++;
	}

	/* Set the string as a return value. */
	if (!noct_set_return_make_string(env, &ret, d))
		return false;

	noct_unpin_local(env, 3, &str, &index, &ret);

	return true;
}

/*
 * String.charCodeAt(s, i)
 *
 * Returns the Unicode codepoint of the character at index i (in
 * characters), or -1 if i is out of range.
 */
static bool
rt_intrin_String_charCodeAt(
	NoctEnv *env)
{
	NoctValue str, index, ret;
	struct rt_string *rts;
	const char *str_s;
	size_t index_i;
	const char *s;
	size_t i, ofs;
	int mblen;

	memset(&str, 0, sizeof(str));
	memset(&index, 0, sizeof(index));
	memset(&ret, 0, sizeof(ret));
	noct_pin_local(env, 3, &str, &index, &ret);

	if (!noct_get_arg_check_string(env, 0, &str, &str_s))
		return false;
	if (!noct_get_arg_check_int_long(env, 1, &index, &index_i))
		return false;

	/* Resume from the cached character position; see charAt above. */
	rts = str.val.str;
	s = str_s;
	i = 0;
	ofs = 0;
	if (rts->cache_index != 0 && rts->cache_index <= index_i) {
		i = rts->cache_index;
		ofs = rts->cache_ofs;
		s = str_s + ofs;
	}
	while (*s != '\0') {
		uint32_t codepoint;
		mblen = utf8_to_utf32(s, &codepoint);
		if (mblen <= 0)
			break;
		if (i == index_i) {
			rts->cache_index = i;
			rts->cache_ofs = ofs;
			if (!noct_set_return_make_int(env, &ret, (int)codepoint))
				return false;
			noct_unpin_local(env, 3, &str, &index, &ret);
			return true;
		}
		s += mblen;
		ofs += (size_t)mblen;
		i++;
	}

	if (!noct_set_return_make_int(env, &ret, -1))
		return false;

	noct_unpin_local(env, 3, &str, &index, &ret);

	return true;
}

/*
 * String.toUpperCase(s) / String.toLowerCase(s)
 *
 * ASCII-only case mapping; other characters pass through.
 */
static bool
rt_intrin_String_case_common(
	NoctEnv *env,
	bool upper)
{
	NoctValue str, ret;
	const char *str_s;
	char *buf;
	size_t len, i;

	memset(&str, 0, sizeof(str));
	memset(&ret, 0, sizeof(ret));
	noct_pin_local(env, 2, &str, &ret);

	if (!noct_get_arg_check_string(env, 0, &str, &str_s))
		return false;

	len = strlen(str_s);
	buf = malloc(len + 1);
	if (buf == NULL) {
		rt_out_of_memory(env);
		return false;
	}
	for (i = 0; i < len; i++) {
		char c = str_s[i];
		if (upper && c >= 'a' && c <= 'z')
			c = (char)(c - 0x20);
		else if (!upper && c >= 'A' && c <= 'Z')
			c = (char)(c + 0x20);
		buf[i] = c;
	}
	buf[len] = '\0';

	if (!noct_set_return_make_string(env, &ret, buf)) {
		free(buf);
		return false;
	}
	free(buf);

	noct_unpin_local(env, 2, &str, &ret);

	return true;
}

static bool
rt_intrin_String_toUpperCase(
	NoctEnv *env)
{
	return rt_intrin_String_case_common(env, true);
}

static bool
rt_intrin_String_toLowerCase(
	NoctEnv *env)
{
	return rt_intrin_String_case_common(env, false);
}

/*
 * String.substring(s, start, len)
 */
static bool
rt_intrin_String_substring(
	NoctEnv *env)
{
	NoctValue str, start, len, ret;
	const char *str_s;
	size_t start_i, len_i, i, ofs, copy_start, copy_mblen;
	const char *s;
	char *tmp;

	noct_pin_local(env, 4, &str, &start, &len, &ret);

	/* Get the argument "s". */
	if (!noct_get_arg_check_string(env, 0, &str, &str_s))
		return false;

	/* Get the argument "start". */
	if (!noct_get_arg_check_int_long(env, 1, &start, &start_i))
		return false;
	if ((int64_t)start_i < 0)
		start_i = 0;

	/* Get the argument "len". */
	if (!noct_get_arg_check_int_long(env, 2, &len, &len_i))
		return false;
	if (len_i == (size_t)-1)
		len_i = LONG_MAX;

	/* Search the position (start_i) and (start_i + len). */
	s = str_s;
	i = 0;
	ofs = 0;
	copy_start = (size_t)-1;
	copy_mblen = 0;
	while (*s != '\0') {
		uint32_t codepoint;
		int mblen;

		mblen = utf8_to_utf32(s, &codepoint);
		if (mblen <= 0) {
			/* UTF-8 error. */
			break;
		}
		if (i == start_i)
			copy_start = ofs;
		if (i == start_i + len_i)
			break;
		if (copy_start != (size_t)-1)
			copy_mblen += (size_t)mblen;

		s += mblen;
		ofs += (size_t)mblen;
		i++;
	}

	/* Allocate a buffer for the new string. */
	tmp = noct_malloc((size_t)copy_mblen + 1);
	if (tmp == NULL) {
		rt_out_of_memory(env);
		return false;
	}

	/* Make a string. */
	if (copy_start != (size_t)-1)
		strncpy(tmp, str_s + copy_start, (size_t)copy_mblen);
	tmp[copy_mblen] = '\0';

	/* Make a return string value. */
	if (!noct_set_return_make_string(env, &ret, tmp))
		return false;

	/* Free the buffer. */
	noct_free(tmp);

	noct_unpin_local(env, 4, &str, &start, &len, &ret);

	return true;
}

/*
 * String.indexOf()
 */
static bool
rt_intrin_String_indexOf(
	NoctEnv *env)
{
	NoctValue str, substr, ret;
	const char *str_s;
	const char *substr_s;
	const char *s;
	size_t len_str, len_substr, char_index;
	int result;

	memset(&str, 0, sizeof(str));
	memset(&substr, 0, sizeof(substr));
	memset(&ret, 0, sizeof(ret));
	noct_pin_local(env, 3, &str, &substr, &ret);

	/* Get the argument "s". */
	if (!noct_get_arg_check_string(env, 0, &str, &str_s))
		return false;

	/* Get the argument "search". */
	if (!noct_get_arg_check_string(env, 1, &substr, &substr_s))
		return false;

	/*
	 * Do search.
	 *
	 * The result is a character index, matching String.charAt() and
	 * String.substring(). Walking character by character also keeps
	 * a match from starting in the middle of a multibyte sequence.
	 */
	len_str = strlen(str_s);
	len_substr = strlen(substr_s);
	result = -1;
	if (len_str >= len_substr) {
		s = str_s;
		char_index = 0;
		while (true) {
			uint32_t codepoint;
			int mblen;

			if (strncmp(s, substr_s, len_substr) == 0) {
				result = (int)char_index;
				break;
			}
			if (*s == '\0')
				break;

			mblen = utf8_to_utf32(s, &codepoint);
			if (mblen <= 0) {
				/* UTF-8 error. */
				break;
			}
			s += mblen;
			char_index++;
		}
	}

	/* Set the return value. */
	if (!noct_set_return_make_int(env, &ret, result))
		return false;

	noct_unpin_local(env, 3, &str, &substr, &ret);

	return true;
}

/* Get the Unicode character count of a string. */
static size_t
get_string_length(
	const char *text)
{
	const char *s;
	size_t i;

	s = text;
	i = 0;
	while (*s != '\0') {
		uint32_t codepoint;
		int mblen;

		mblen = utf8_to_utf32(s, &codepoint);
		if (mblen <= 0) {
			/* UTF-8 error. */
			return 0;
		}

		i++;
		s += mblen;
	}

	return i;
}

/* Get a top character of a utf-8 string as a utf-32. */
static int utf8_to_utf32(const char *mbs, uint32_t *wc)
{
	size_t octets, i;
	uint32_t ret;

	assert(mbs != NULL);

	/*
	 * Never call strlen() here: this runs once per character in
	 * every string scan, and taking the length of the remaining
	 * string each time turns those scans quadratic. The
	 * continuation-byte checks below catch truncated sequences.
	 */

	/* Check the first byte, get an octet count. */
	if (mbs[0] == '\0')
		return 0;
	else if ((mbs[0] & 0x80) == 0)
		octets = 1;
	else if ((mbs[0] & 0xe0) == 0xc0)
		octets = 2;
	else if ((mbs[0] & 0xf0) == 0xe0)
		octets = 3;
	else if ((mbs[0] & 0xf8) == 0xf0)
		octets = 4;
	else
		return -1;	/* Not supported. */

	/* Check for 2-4 bytes. */
	for (i = 1; i < octets; i++) {
		if((mbs[i] & 0xc0) != 0x80)
			return -1;	/* Non-understandable */
	}

	/* Compose a utf-32 character. */
	switch (octets) {
	case 0:
		ret = 0;
		break;
	case 1:
		ret = (uint32_t)mbs[0];
		break;
	case 2:
		ret = (uint32_t)(((mbs[0] & 0x1f) << 6) |
				 (mbs[1] & 0x3f));
		break;
	case 3:
		ret = (uint32_t)(((mbs[0] & 0x0f) << 12) |
				 ((mbs[1] & 0x3f) << 6) |
				 (mbs[2] & 0x3f));
		break;
	case 4:
		ret = (uint32_t)(((mbs[0] & 0x07) << 18) |
				 ((mbs[1] & 0x3f) << 12) |
				 ((mbs[2] & 0x3f) << 6) |
				 (mbs[3] & 0x3f));
		break;
	default:
		/* never come here */
		assert(0);
		return -1;
	}

	/* Store the result. */
	if(wc != NULL)
		*wc = ret;

	/* Return the octet count. */
	return (int)octets;
}

/*
 * Array.make(size)
 */
static bool
rt_intrin_Array_make(
	NoctEnv *env)
{
	struct rt_value arr, size;
	size_t size_i;

	noct_pin_local(env, 2, &arr, &size);

	/* Retrieve the "size" argument from the first parameter (index 0). */
	if (!noct_get_arg_check_int_long(env, 0, &size, &size_i))
		return false;

	/* Initialize a new empty array object. */
	if (!noct_make_empty_array(env, &arr))
		return false;

	/* Allocate the requested capacity for the array. */
	if (!noct_resize_array(env, &arr, size_i))
		return false;

	/* Set the allocated array as the function's return value. */
	if (!noct_set_return(env, &arr))
		return false;

	noct_unpin_local(env, 2, &arr, &size);

	return true;
}

/*
 * Array.size(arr)
 */
static bool
rt_intrin_Array_size(
	NoctEnv *env)
{
	struct rt_value arr, ret;
	size_t size;

	noct_pin_local(env, 2, &arr, &ret);

	/* Retrieve the "arr" argument from the first parameter (index 0). */
	if (!noct_get_arg_check_array(env, 0, &arr))
		return false;

	/* Get the current size of the array. */
	if (!noct_get_array_size(env, &arr, &size))
		return false;

	/* Set the return value. */
	if (!noct_set_return_make_int_long(env, &ret, size))
		return false;

	noct_unpin_local(env, 2, &arr, &ret);

	return true;
}

/*
 * Array.push(arr, val)
 */
static bool
rt_intrin_Array_push(
	NoctEnv *env)
{
	struct rt_value arr, val;
	size_t size;

	noct_pin_local(env, 2, &arr, &val);

	/* Retrieve the "arr" argument from the first parameter (index 0). */
	if (!noct_get_arg_check_array(env, 0, &arr))
		return false;

	/* Retrieve the "val" argument from the first parameter (index 1). */
	if (!noct_get_arg(env, 1, &val))
		return false;

	/* Get the current size of the array to determine the tail index. */
	if (!noct_get_array_size(env, &arr, &size))
		return false;

	/* Append "val" to the end of the array. */
	if (!noct_set_array_elem(env, &arr, size, &val))
		return false;

	/* Set the modified array as the return value. */
	if (!noct_set_return(env, &arr))
		return false;

	noct_unpin_local(env, 2, &arr, &val);

	return true;
}

/*
 * Array.pop(arr)
 */
static bool
rt_intrin_Array_pop(
	NoctEnv *env)
{
	struct rt_value arr, val;
	size_t size;

	noct_pin_local(env, 2, &arr, &val);

	/* Retrieve the "arr" argument from the first parameter (index 0). */
	if (!noct_get_arg_check_array(env, 0, &arr))
		return false;

	/* Get the current size of the array to determine the tail index. */
	if (!noct_get_array_size(env, &arr, &size))
		return false;
	if (size == 0) {
		/* Error: arr is empty, cannot pop. */
		noct_error(env, N_TR("Empty array."));
		return false;
	}

	/* Get the tail element to be popped.*/
	if (!noct_get_array_elem(env, &arr, size - 1, &val))
		return false;

	/* Shrink the array to (size - 1). */
	if (!noct_resize_array(env, &arr, size - 1))
		return false;

	/* Set the popped value as the return value. */
	if (!noct_set_return(env, &val))
		return false;

	noct_unpin_local(env, 2, &arr, &val);

	return true;
}

/*
 * Array.resize(arr, size)
 */
static bool
rt_intrin_Array_resize(
	NoctEnv *env)
{
	struct rt_value arr, size;
	size_t size_i;

	noct_pin_local(env, 2, &arr, &size);

	/* Retrieve the argument "arr" at the index 0. */
	if (!noct_get_arg_check_array(env, 0, &arr))
		return false;

	/* Retrieve the argument "size" at the index 1. */
	if (!noct_get_arg_check_int_long(env, 1, &size, &size_i))
		return false;

	/* Resize the array "arr". */
	if (!noct_resize_array(env, &arr, size_i))
		return false;

	/* Set the modified array as the return value. */
	if (!noct_set_return(env, &arr))
		return false;

	noct_unpin_local(env, 2, &arr, &size);

	return true;
}

/*
 * Array.copy(arr)
 */
static bool
rt_intrin_Array_copy(
	NoctEnv *env)
{
	struct rt_value arr, ret;

	noct_pin_local(env, 2, &arr, &ret);

	/* Retrieve the argument "arr" at the index 0. */
	if (!noct_get_arg_check_array(env, 0, &arr))
		return false;

	/* Make a copy of the array. */
	if (!noct_make_array_copy(env, &ret, &arr))
		return false;

	/* Set the new array as the return value. */
	if (!noct_set_return(env, &ret))
		return false;

	noct_unpin_local(env, 2, &arr, &ret);

	return true;
}

/*
 * Dict.make()
 */
static bool
rt_intrin_Dict_make(
	NoctEnv *env)
{
	struct rt_value ret;

	noct_pin_local(env, 1, &ret);

	/* Make an empty dictionary. */
	if (!noct_make_empty_dict(env, &ret))
		return false;

	/* Set the new dictionary as the return value. */
	if (!noct_set_return(env, &ret))
		return false;

	noct_unpin_local(env, 1, &ret);

	return true;
}

/*
 * Dict.merge(src1, src2)
 */
static bool
rt_intrin_Dict_merge(
	NoctEnv *env)
{
	struct rt_value src1, src2, ret;

	noct_pin_local(env, 3, &src1, &src2, &ret);

	/* Retrieve the argument "src1`" at the index 0. */
	if (!noct_get_arg_check_dict(env, 0, &src1))
		return false;

	/* Retrieve the argument "b" at the index 1. */
	if (!noct_get_arg_check_dict(env, 1, &src2))
		return false;

	/* Merge "cls" and "init" dictionaries into "ret". */
	if (!noct_merge_dict(env, &ret, &src1, &src2))
		return false;

	/* Set the merged dictionary as the return value. */
	if (!noct_set_return(env, &ret))
		return false;

	noct_unpin_local(env, 3, &src1, &src2, &ret);

	return true;
}

/*
 * Dict.freeze(dict)
 *
 * Makes a dictionary read-only and returns it.  Compiler-emitted for
 * class literals and "extend"; also public (docs/design/03-class.md).
 */
static bool
rt_intrin_Dict_freeze(
	NoctEnv *env)
{
	struct rt_value dict;

	noct_pin_local(env, 1, &dict);

	/* Retrieve the argument "dict" at the index 0. */
	if (!noct_get_arg_check_dict(env, 0, &dict))
		return false;

	/* Set the frozen flag. */
	if (!om_freeze_dict(env, &dict))
		return false;

	/* Return the same dictionary. */
	if (!noct_set_return(env, &dict))
		return false;

	noct_unpin_local(env, 1, &dict);

	return true;
}

/*
 * Global.markConst(name)
 *
 * Marks a global binding constant.  Compiler-emitted for top-level
 * "let" declarations; also public.
 */
static bool
rt_intrin_Global_markConst(
	NoctEnv *env)
{
	struct rt_value name;
	const char *name_s;
	uint32_t i;

	noct_pin_local(env, 1, &name);

	if (!noct_get_arg_check_string(env, 0, &name, &name_s))
		return false;

	for (i = 0; i < env->vm->global_alloc_size; i++) {
		if (env->vm->global[i].name == NULL)
			continue;
		if (env->vm->global[i].is_removed)
			continue;
		if (strcmp(env->vm->global[i].name, name_s) == 0) {
			env->vm->global[i].is_const = true;
			noct_unpin_local(env, 1, &name);
			return true;
		}
	}

	noct_unpin_local(env, 1, &name);
	rt_error(env, N_TR("Symbol \"%s\" not found."), name_s);
	return false;
}

/*
 * Dict.size(dict)
 */
static bool
rt_intrin_Dict_size(
	struct rt_env *env)
{
	NoctValue dict, ret;
	size_t size;

	noct_pin_local(env, 2, &dict, &ret);

	/* Retrieve the argument "dict" at the index 0. */
	if (!noct_get_arg_check_dict(env, 0, &dict))
		return false;

	/* Get the size. */
	if (!noct_get_dict_size(env, &dict, &size))
		return false;

	/* Set the return value. */
	if (!noct_set_return_make_int_long(env, &ret, size))
		return false;

	noct_unpin_local(env, 2, &dict, &ret);

	return true;
}

/*
 * Dict.hasKey(dict, key)
 */
static bool
rt_intrin_Dict_hasKey(
	struct rt_env *env)
{
	NoctValue dict, key, ret;
	const char *key_s;
	bool is_key_available;

	noct_pin_local(env, 3, &dict, &key, &ret);

	/* Retrieve the argument "dict" at the index 0. */
	if (!noct_get_arg_check_dict(env, 0, &dict))
		return false;

	/* Retrieve the argument "key" at the index 1. */
	if (!noct_get_arg_check_string(env, 1, &key, &key_s))
		return false;

	/* Check for the key. */
	if (!noct_check_dict_key(env, &dict, &key, &is_key_available))
		return false;

	/* Set the return value. */
	if (!noct_set_return_make_int(env, &ret, is_key_available ? 1 : 0))
		return false;

	noct_unpin_local(env, 3, &dict, &key, &ret);

	return true;
}

/*
 * Dict.remove(dict, key)
 */
static bool
rt_intrin_Dict_remove(
	struct rt_env *env)
{
	NoctValue dict, key;
	const char *key_s;

	noct_pin_local(env, 2, &dict, &key);

	/* Retrieve the argument "dict" at the index 0. */
	if (!noct_get_arg_check_dict(env, 0, &dict))
		return false;

	/* Retrieve the argument "key" at the index 1. */
	if (!noct_get_arg_check_string(env, 1, &key, &key_s))
		return false;

	/* Removing an absent key is a no-op, not an error. */
	{
		bool has;
		if (!noct_check_dict_key_cstr(env, &dict, key_s, &has))
			return false;
		if (has) {
			if (!noct_remove_dict_elem(env, &dict, &key))
				return false;
		}
	}

	noct_unpin_local(env, 2, &dict, &key);

	return true;
}

/*
 * Dict.copy(dict)
 */
static bool
rt_intrin_Dict_copy(
	struct rt_env *env)
{
	NoctValue dict, ret;

	noct_pin_local(env, 2, &dict, &ret);

	/* Retrieve the argument "dict" at the index 0. */
	if (!noct_get_arg_check_dict(env, 0, &dict))
		return false;

	/* Make a copy of the dicttionary. */
	if (!noct_make_dict_copy(env, &ret, &dict))
		return false;

	/* Set the return value. */
	if (!noct_set_return(env, &ret))
		return false;

	noct_unpin_local(env, 2, &dict, &ret);

	return true;
}

/*
 * Packed.int8()
 */
static bool
rt_intrin_Packed_int8(
	NoctEnv *env)
{
	NoctValue v_size, v_ret;
	size_t i_size;

	noct_pin_local(env, 2, &v_size, &v_ret);

	if (!noct_get_arg_check_int_long(env, 0, &v_size, &i_size))
		return false;
	if (i_size == 0) {
		noct_error(env, N_TR("Packed size is 0."));
		return false;
	}

	if (!noct_make_packed(env, &v_ret, NOCT_PACKED_INT8, i_size, i_size,
			      NULL, NULL, NULL))
		return false;
	if (!noct_set_return(env, &v_ret))
		return false;

	noct_unpin_local(env, 2, &v_size, &v_ret);

	return true;
}

/*
 * Packed.uint8()
 */
static bool
rt_intrin_Packed_uint8(
	NoctEnv *env)
{
	NoctValue v_size, v_ret;
	size_t i_size;

	noct_pin_local(env, 2, &v_size, &v_ret);

	if (!noct_get_arg_check_int_long(env, 0, &v_size, &i_size))
		return false;
	if (i_size == 0) {
		noct_error(env, N_TR("Packed size is 0."));
		return false;
	}

	if (!noct_make_packed(env, &v_ret, NOCT_PACKED_UINT8, i_size, i_size,
			      NULL, NULL, NULL))
		return false;
	if (!noct_set_return(env, &v_ret))
		return false;

	noct_unpin_local(env, 2, &v_size, &v_ret);

	return true;
}

/*
 * Packed.int16()
 */
static bool
rt_intrin_Packed_int16(
	NoctEnv *env)
{
	NoctValue v_size, v_ret;
	size_t i_size;

	noct_pin_local(env, 2, &v_size, &v_ret);

	if (!noct_get_arg_check_int_long(env, 0, &v_size, &i_size))
		return false;
	if (i_size == 0) {
		noct_error(env, N_TR("Packed size is 0."));
		return false;
	}
	if (i_size > SIZE_MAX / 2) {
		noct_error(env, N_TR("Packed size is too large."));
		return false;
	}

	if (!noct_make_packed(env, &v_ret, NOCT_PACKED_INT16, i_size * 2, i_size,
			      NULL, NULL, NULL))
		return false;
	if (!noct_set_return(env, &v_ret))
		return false;

	noct_unpin_local(env, 2, &v_size, &v_ret);

	return true;
}

/*
 * Packed.uint16()
 */
static bool
rt_intrin_Packed_uint16(
	NoctEnv *env)
{
	NoctValue v_size, v_ret;
	size_t i_size;

	noct_pin_local(env, 2, &v_size, &v_ret);

	if (!noct_get_arg(env, 0, &v_size))
		return false;
	if (!noct_get_size_t(env, &v_size, &i_size))
		return false;
	if (i_size == 0) {
		noct_error(env, N_TR("Packed size is 0."));
		return false;
	}
	if (i_size > SIZE_MAX / 2) {
		noct_error(env, N_TR("Packed size is too large."));
		return false;
	}

	if (!noct_make_packed(env, &v_ret, NOCT_PACKED_UINT16, i_size * 2, i_size,
			      NULL, NULL, NULL))
		return false;
	if (!noct_set_return(env, &v_ret))
		return false;

	noct_unpin_local(env, 2, &v_size, &v_ret);

	return true;
}

/*
 * Packed.int32()
 */
static bool
rt_intrin_Packed_int32(
	NoctEnv *env)
{
	NoctValue v_size, v_ret;
	size_t i_size;

	noct_pin_local(env, 2, &v_size, &v_ret);

	if (!noct_get_arg_check_int_long(env, 0, &v_size, &i_size))
		return false;
	if (i_size == 0) {
		noct_error(env, N_TR("Packed size is 0."));
		return false;
	}
	if (i_size > SIZE_MAX / 4) {
		noct_error(env, N_TR("Packed size is too large."));
		return false;
	}

	if (!noct_make_packed(env, &v_ret, NOCT_PACKED_INT32, i_size * 4, i_size,
			      NULL, NULL, NULL))
		return false;
	if (!noct_set_return(env, &v_ret))
		return false;

	noct_unpin_local(env, 2, &v_size, &v_ret);

	return true;
}

/*
 * Packed.uint32()
 */
static bool
rt_intrin_Packed_uint32(
	NoctEnv *env)
{
	NoctValue v_size, v_ret;
	size_t i_size;

	noct_pin_local(env, 2, &v_size, &v_ret);

	if (!noct_get_arg_check_int_long(env, 0, &v_size, &i_size))
		return false;
	if (i_size == 0) {
		noct_error(env, N_TR("Packed size is 0."));
		return false;
	}
	if (i_size > SIZE_MAX / 4) {
		noct_error(env, N_TR("Packed size is too large."));
		return false;
	}

	if (!noct_make_packed(env, &v_ret, NOCT_PACKED_UINT32, i_size * 4, i_size,
			      NULL, NULL, NULL))
		return false;
	if (!noct_set_return(env, &v_ret))
		return false;

	noct_unpin_local(env, 2, &v_size, &v_ret);

	return true;
}

/*
 * Packed.int64()
 */
static bool
rt_intrin_Packed_int64(
	NoctEnv *env)
{
	NoctValue v_size, v_ret;
	size_t i_size;

	noct_pin_local(env, 2, &v_size, &v_ret);

	if (!noct_get_arg_check_int_long(env, 0, &v_size, &i_size))
		return false;
	if (i_size == 0) {
		noct_error(env, N_TR("Packed size is 0."));
		return false;
	}
	if (i_size > SIZE_MAX / 8) {
		noct_error(env, N_TR("Packed size is too large."));
		return false;
	}

	if (!noct_make_packed(env, &v_ret, NOCT_PACKED_INT64, i_size * 8, i_size,
			      NULL, NULL, NULL))
		return false;
	if (!noct_set_return(env, &v_ret))
		return false;

	noct_unpin_local(env, 2, &v_size, &v_ret);

	return true;
}

/*
 * Packed.uint64()
 */
static bool
rt_intrin_Packed_uint64(
	NoctEnv *env)
{
	NoctValue v_size, v_ret;
	size_t i_size;

	noct_pin_local(env, 2, &v_size, &v_ret);

	if (!noct_get_arg_check_int_long(env, 0, &v_size, &i_size))
		return false;
	if (i_size == 0) {
		noct_error(env, N_TR("Packed size is 0."));
		return false;
	}
	if (i_size > SIZE_MAX / 8) {
		noct_error(env, N_TR("Packed size is too large."));
		return false;
	}

	if (!noct_make_packed(env, &v_ret, NOCT_PACKED_UINT64, i_size * 8, i_size,
			      NULL, NULL, NULL))
		return false;
	if (!noct_set_return(env, &v_ret))
		return false;

	noct_unpin_local(env, 2, &v_size, &v_ret);

	return true;
}

/*
 * Packed.float32()
 */
static bool
rt_intrin_Packed_float32(
	NoctEnv *env)
{
	NoctValue v_size, v_ret;
	size_t i_size;

	noct_pin_local(env, 2, &v_size, &v_ret);

	if (!noct_get_arg_check_int_long(env, 0, &v_size, &i_size))
		return false;
	if (i_size == 0) {
		noct_error(env, N_TR("Packed size is 0."));
		return false;
	}
	if (i_size > SIZE_MAX / 4) {
		noct_error(env, N_TR("Packed size is too large."));
		return false;
	}

	if (!noct_make_packed(env, &v_ret, NOCT_PACKED_FLOAT32, i_size * 4, i_size,
			      NULL, NULL, NULL))
		return false;
	if (!noct_set_return(env, &v_ret))
		return false;

	noct_unpin_local(env, 2, &v_size, &v_ret);

	return true;
}

/*
 * Packed.float64()
 */
static bool
rt_intrin_Packed_float64(
	NoctEnv *env)
{
	NoctValue v_size, v_ret;
	size_t i_size;

	noct_pin_local(env, 2, &v_size, &v_ret);

	if (!noct_get_arg_check_int_long(env, 0, &v_size, &i_size))
		return false;
	if (i_size == 0) {
		noct_error(env, N_TR("Packed size is 0."));
		return false;
	}
	if (i_size > SIZE_MAX / 8) {
		noct_error(env, N_TR("Packed size is too large."));
		return false;
	}

	if (!noct_make_packed(env, &v_ret, NOCT_PACKED_FLOAT64, i_size * 8, i_size,
			      NULL, NULL, NULL))
		return false;
	if (!noct_set_return(env, &v_ret))
		return false;

	noct_unpin_local(env, 2, &v_size, &v_ret);

	return true;
}

/*
 * Packed.size(packed)
 */
/*
 * Get the byte size of one packed element.
 */
static size_t
packed_elem_bytes(
	int type)
{
	switch (type) {
	case NOCT_PACKED_INT8:
	case NOCT_PACKED_UINT8:
		return 1;
	case NOCT_PACKED_INT16:
	case NOCT_PACKED_UINT16:
		return 2;
	case NOCT_PACKED_INT32:
	case NOCT_PACKED_UINT32:
	case NOCT_PACKED_FLOAT32:
		return 4;
	default:
		break;
	}
	return 8;
}

/*
 * Packed.copy()
 *
 * Copies "count" elements from src to dst. Indices and the count are
 * in elements, matching Packed.size() and the [] notation.
 *
 * The two regions may overlap, which is what makes this usable for
 * moving the gap of a gap buffer.
 */
static bool
rt_intrin_Packed_copy(
	NoctEnv *env)
{
	NoctValue dst, dst_index, src, src_index, count, ret;
	size_t dst_index_n, src_index_n, count_n;
	size_t dst_size, src_size, elem_bytes;
	int dst_type, src_type;
	void *dst_buf, *src_buf;

	memset(&dst, 0, sizeof(dst));
	memset(&dst_index, 0, sizeof(dst_index));
	memset(&src, 0, sizeof(src));
	memset(&src_index, 0, sizeof(src_index));
	memset(&count, 0, sizeof(count));
	memset(&ret, 0, sizeof(ret));
	noct_pin_local(env, 6, &dst, &dst_index, &src, &src_index, &count, &ret);

	/* Get the arguments. */
	if (!noct_get_arg_check_packed(env, 0, &dst, NOCT_PACKED_ANY))
		return false;
	if (!noct_get_arg_check_int_long(env, 1, &dst_index, &dst_index_n))
		return false;
	if (!noct_get_arg_check_packed(env, 2, &src, NOCT_PACKED_ANY))
		return false;
	if (!noct_get_arg_check_int_long(env, 3, &src_index, &src_index_n))
		return false;
	if (!noct_get_arg_check_int_long(env, 4, &count, &count_n))
		return false;

	/* Both sides must hold the same element type. */
	if (!noct_get_packed_type(env, &dst, &dst_type))
		return false;
	if (!noct_get_packed_type(env, &src, &src_type))
		return false;
	if (dst_type != src_type) {
		noct_error(env, N_TR("Packed element types do not match."));
		return false;
	}

	/* Check the ranges. */
	if (!noct_get_packed_size(env, &dst, &dst_size))
		return false;
	if (!noct_get_packed_size(env, &src, &src_size))
		return false;
	if (dst_index_n > dst_size || count_n > dst_size - dst_index_n) {
		noct_error(env, N_TR("Packed destination range is out-of-bounds."));
		return false;
	}
	if (src_index_n > src_size || count_n > src_size - src_index_n) {
		noct_error(env, N_TR("Packed source range is out-of-bounds."));
		return false;
	}

	/* Copy. */
	if (count_n > 0) {
		if (!noct_get_packed_pointer(env, &dst, &dst_buf))
			return false;
		if (!noct_get_packed_pointer(env, &src, &src_buf))
			return false;
		elem_bytes = packed_elem_bytes(dst_type);
		memmove((char *)dst_buf + dst_index_n * elem_bytes,
			(char *)src_buf + src_index_n * elem_bytes,
			count_n * elem_bytes);
	}

	/* Set the return value. */
	if (!noct_set_return_make_int_long(env, &ret, count_n))
		return false;

	noct_unpin_local(env, 6, &dst, &dst_index, &src, &src_index, &count, &ret);

	return true;
}

/*
 * Packed.fill()
 *
 * Sets "count" elements of dst to "value", starting at "index".
 */
static bool
rt_intrin_Packed_fill(
	NoctEnv *env)
{
	NoctValue dst, index, count, value, ret;
	size_t index_n, count_n, dst_size, i;
	int dst_type, value_type;
	int64_t ival;
	double dval;
	void *dst_buf;

	memset(&dst, 0, sizeof(dst));
	memset(&index, 0, sizeof(index));
	memset(&count, 0, sizeof(count));
	memset(&value, 0, sizeof(value));
	memset(&ret, 0, sizeof(ret));
	noct_pin_local(env, 5, &dst, &index, &count, &value, &ret);

	/* Get the arguments. */
	if (!noct_get_arg_check_packed(env, 0, &dst, NOCT_PACKED_ANY))
		return false;
	if (!noct_get_arg_check_int_long(env, 1, &index, &index_n))
		return false;
	if (!noct_get_arg_check_int_long(env, 2, &count, &count_n))
		return false;
	if (!noct_get_arg(env, 3, &value))
		return false;

	/* Check the range. */
	if (!noct_get_packed_size(env, &dst, &dst_size))
		return false;
	if (index_n > dst_size || count_n > dst_size - index_n) {
		noct_error(env, N_TR("Packed destination range is out-of-bounds."));
		return false;
	}

	/* Accept any numeric value and convert it to the element type. */
	if (!noct_get_value_type(env, &value, &value_type))
		return false;
	ival = 0;
	dval = 0.0;
	switch (value_type) {
	case NOCT_VALUE_INT:
		ival = value.val.i;
		dval = (double)value.val.i;
		break;
	case NOCT_VALUE_LONG:
		ival = value.val.l;
		dval = (double)value.val.l;
		break;
	case NOCT_VALUE_FLOAT:
		ival = (int64_t)value.val.f;
		dval = (double)value.val.f;
		break;
	case NOCT_VALUE_DOUBLE:
		ival = (int64_t)value.val.lf;
		dval = value.val.lf;
		break;
	default:
		noct_error(env, N_TR("Fill value is not a number."));
		return false;
	}

	/* Fill. */
	if (count_n > 0) {
		if (!noct_get_packed_type(env, &dst, &dst_type))
			return false;
		if (!noct_get_packed_pointer(env, &dst, &dst_buf))
			return false;

		switch (dst_type) {
		case NOCT_PACKED_INT8:
		case NOCT_PACKED_UINT8:
			memset((char *)dst_buf + index_n, (int)(uint8_t)ival, count_n);
			break;
		case NOCT_PACKED_INT16:
		case NOCT_PACKED_UINT16:
			for (i = 0; i < count_n; i++)
				((uint16_t *)dst_buf)[index_n + i] = (uint16_t)ival;
			break;
		case NOCT_PACKED_INT32:
		case NOCT_PACKED_UINT32:
			for (i = 0; i < count_n; i++)
				((uint32_t *)dst_buf)[index_n + i] = (uint32_t)ival;
			break;
		case NOCT_PACKED_FLOAT32:
			for (i = 0; i < count_n; i++)
				((float *)dst_buf)[index_n + i] = (float)dval;
			break;
		case NOCT_PACKED_FLOAT64:
			for (i = 0; i < count_n; i++)
				((double *)dst_buf)[index_n + i] = dval;
			break;
		default:
			for (i = 0; i < count_n; i++)
				((uint64_t *)dst_buf)[index_n + i] = (uint64_t)ival;
			break;
		}
	}

	/* Set the return value. */
	if (!noct_set_return_make_int_long(env, &ret, count_n))
		return false;

	noct_unpin_local(env, 5, &dst, &index, &count, &value, &ret);

	return true;
}

/*
 * Packed.toString()
 *
 * Interprets "length" bytes of a uint8/int8 packed array, starting at
 * "offset", as UTF-8 text and returns it as a string.
 *
 * Note: strings are NUL-terminated internally, so a NUL byte inside
 * the range truncates the result.
 */
static bool
rt_intrin_Packed_toString(
	NoctEnv *env)
{
	NoctValue src, offset, length, ret;
	size_t offset_n, length_n, size;
	int type;
	void *buf;
	char *tmp;

	memset(&src, 0, sizeof(src));
	memset(&offset, 0, sizeof(offset));
	memset(&length, 0, sizeof(length));
	memset(&ret, 0, sizeof(ret));
	noct_pin_local(env, 4, &src, &offset, &length, &ret);

	if (!noct_get_arg_check_packed(env, 0, &src, NOCT_PACKED_ANY))
		return false;
	if (!noct_get_arg_check_int_long(env, 1, &offset, &offset_n))
		return false;
	if (!noct_get_arg_check_int_long(env, 2, &length, &length_n))
		return false;

	if (!noct_get_packed_type(env, &src, &type))
		return false;
	if (type != NOCT_PACKED_UINT8 && type != NOCT_PACKED_INT8) {
		noct_error(env, N_TR("Packed element type must be a byte."));
		return false;
	}
	if (!noct_get_packed_size(env, &src, &size))
		return false;
	if (offset_n > size || length_n > size - offset_n) {
		noct_error(env, N_TR("Packed source range is out-of-bounds."));
		return false;
	}
	if (!noct_get_packed_pointer(env, &src, &buf))
		return false;

	tmp = noct_malloc(length_n + 1);
	if (tmp == NULL) {
		noct_out_of_memory(env);
		return false;
	}
	memcpy(tmp, (char *)buf + offset_n, length_n);
	tmp[length_n] = '\0';

	if (!noct_set_return_make_string(env, &ret, tmp)) {
		noct_free(tmp);
		return false;
	}
	noct_free(tmp);

	noct_unpin_local(env, 4, &src, &offset, &length, &ret);

	return true;
}

/*
 * Packed.fromString()
 *
 * Writes the UTF-8 bytes of a string into a uint8/int8 packed array at
 * "offset" and returns the number of bytes written.
 */
static bool
rt_intrin_Packed_fromString(
	NoctEnv *env)
{
	NoctValue dst, offset, str, ret;
	size_t offset_n, size, len;
	const char *str_s;
	int type;
	void *buf;

	memset(&dst, 0, sizeof(dst));
	memset(&offset, 0, sizeof(offset));
	memset(&str, 0, sizeof(str));
	memset(&ret, 0, sizeof(ret));
	noct_pin_local(env, 4, &dst, &offset, &str, &ret);

	if (!noct_get_arg_check_packed(env, 0, &dst, NOCT_PACKED_ANY))
		return false;
	if (!noct_get_arg_check_int_long(env, 1, &offset, &offset_n))
		return false;
	if (!noct_get_arg_check_string(env, 2, &str, &str_s))
		return false;

	if (!noct_get_packed_type(env, &dst, &type))
		return false;
	if (type != NOCT_PACKED_UINT8 && type != NOCT_PACKED_INT8) {
		noct_error(env, N_TR("Packed element type must be a byte."));
		return false;
	}
	if (!noct_get_packed_size(env, &dst, &size))
		return false;
	len = strlen(str_s);
	if (offset_n > size || len > size - offset_n) {
		noct_error(env, N_TR("Packed destination range is out-of-bounds."));
		return false;
	}
	if (!noct_get_packed_pointer(env, &dst, &buf))
		return false;

	memcpy((char *)buf + offset_n, str_s, len);

	if (!noct_set_return_make_int_long(env, &ret, len))
		return false;

	noct_unpin_local(env, 4, &dst, &offset, &str, &ret);

	return true;
}

/*
 * String.byteLength()
 *
 * Returns the UTF-8 byte length of a string, excluding the NUL.
 */
static bool
rt_intrin_String_byteLength(
	NoctEnv *env)
{
	NoctValue str, ret;
	const char *str_s;

	memset(&str, 0, sizeof(str));
	memset(&ret, 0, sizeof(ret));
	noct_pin_local(env, 2, &str, &ret);

	if (!noct_get_arg_check_string(env, 0, &str, &str_s))
		return false;

	if (!noct_set_return_make_int_long(env, &ret, strlen(str_s)))
		return false;

	noct_unpin_local(env, 2, &str, &ret);

	return true;
}

/*
 * String.chr()
 *
 * Returns a one-character string for a Unicode codepoint.
 */
static bool
rt_intrin_String_chr(
	NoctEnv *env)
{
	NoctValue cp_v, ret;
	size_t cp_n;
	int64_t cp;
	char tmp[8];
	size_t len;

	memset(&cp_v, 0, sizeof(cp_v));
	memset(&ret, 0, sizeof(ret));
	noct_pin_local(env, 2, &cp_v, &ret);

	if (!noct_get_arg_check_int_long(env, 0, &cp_v, &cp_n))
		return false;
	cp = (int64_t)cp_n;
	if (cp <= 0 || cp > 0x10FFFF) {
		noct_error(env, N_TR("Invalid codepoint."));
		return false;
	}

	if (cp < 0x80) {
		tmp[0] = (char)cp;
		len = 1;
	} else if (cp < 0x800) {
		tmp[0] = (char)(0xC0 | (cp >> 6));
		tmp[1] = (char)(0x80 | (cp & 0x3F));
		len = 2;
	} else if (cp < 0x10000) {
		tmp[0] = (char)(0xE0 | (cp >> 12));
		tmp[1] = (char)(0x80 | ((cp >> 6) & 0x3F));
		tmp[2] = (char)(0x80 | (cp & 0x3F));
		len = 3;
	} else {
		tmp[0] = (char)(0xF0 | (cp >> 18));
		tmp[1] = (char)(0x80 | ((cp >> 12) & 0x3F));
		tmp[2] = (char)(0x80 | ((cp >> 6) & 0x3F));
		tmp[3] = (char)(0x80 | (cp & 0x3F));
		len = 4;
	}
	tmp[len] = '\0';

	if (!noct_set_return_make_string(env, &ret, tmp))
		return false;

	noct_unpin_local(env, 2, &cp_v, &ret);

	return true;
}

static bool
rt_intrin_Packed_size(
	struct rt_env *env)
{
	NoctValue packed, ret;
	size_t size;

	noct_pin_local(env, 2, &packed, &ret);

	/* Retrieve the argument "packed" at the index 0. */
	if (!noct_get_arg_check_packed(env, 0, &packed, NOCT_PACKED_ANY))
		return false;

	/* Get the size. */
	if (!noct_get_packed_size(env, &packed, &size))
		return false;

	/* Set the return value. */
	if (!noct_set_return_make_int_long(env, &ret, size))
		return false;

	noct_unpin_local(env, 2, &packed, &ret);

	return true;
}

/*
 * Packed.type(packed)
 */
static bool
rt_intrin_Packed_type(
	struct rt_env *env)
{
	NoctValue packed, ret;
	int type;

	noct_pin_local(env, 2, &packed, &ret);

	/* Retrieve the argument "packed" at the index 0. */
	if (!noct_get_arg_check_packed(env, 0, &packed, NOCT_PACKED_ANY))
		return false;

	/* Get the type. */
	if (!noct_get_packed_type(env, &packed, &type))
		return false;

	/* Set the return value. */
	switch (type) {
	case NOCT_PACKED_INT8:
		if (!noct_set_return_make_string(env, &ret, "int8"))
			return false;
		break;
	case NOCT_PACKED_INT16:
		if (!noct_set_return_make_string(env, &ret, "int16"))
			return false;
		break;
	case NOCT_PACKED_INT32:
		if (!noct_set_return_make_string(env, &ret, "int32"))
			return false;
		break;
	case NOCT_PACKED_INT64:
		if (!noct_set_return_make_string(env, &ret, "int64"))
			return false;
		break;
	case NOCT_PACKED_UINT8:
		if (!noct_set_return_make_string(env, &ret, "uint8"))
			return false;
		break;
	case NOCT_PACKED_UINT16:
		if (!noct_set_return_make_string(env, &ret, "uint16"))
			return false;
		break;
	case NOCT_PACKED_UINT32:
		if (!noct_set_return_make_string(env, &ret, "uint32"))
			return false;
		break;
	case NOCT_PACKED_UINT64:
		if (!noct_set_return_make_string(env, &ret, "uint64"))
			return false;
		break;
	default:
		assert(0);
		if (!noct_set_return_make_string(env, &ret, "(error)"))
			return false;
		break;
	}

	noct_unpin_local(env, 2, &packed, &ret);

	return true;
}

/*
 * Math.abs()
 */
static bool
rt_intrin_Math_abs(
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
		if (ival == INT_MIN) {
			if (!noct_set_return_make_long(env, &ret, (int64_t)INT_MAX + 1))
				return false;
			return true;
		} else if (ival < 0) {
			ival = -ival;
		}
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
rt_intrin_Math_sqrt(
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

/*
 * Math.sin()
 */
static bool
rt_intrin_Math_sin(
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

/*
 * Math.cos()
 */
static bool
rt_intrin_Math_cos(
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

/*
 * Math.tan()
 */
static bool
rt_intrin_Math_tan(
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

/*
 * Math.random()
 */
static bool
rt_intrin_Math_random(
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

/*
 * Global.hasVariable(name)
 */
/*
 * Type.of()
 *
 * Returns the type of a value as a string: "int", "long", "float",
 * "double", "string", "array", "dict", "packed" or "func".
 */
static bool
rt_intrin_Type_of(
	NoctEnv *env)
{
	NoctValue val, ret;
	const char *name;
	int type;

	memset(&val, 0, sizeof(NoctValue));
	memset(&ret, 0, sizeof(NoctValue));
	noct_pin_local(env, 2, &val, &ret);

	/* Retrieve the argument "val" at the index 0. */
	if (!noct_get_arg(env, 0, &val))
		return false;

	/* Get the type tag. */
	if (!noct_get_value_type(env, &val, &type))
		return false;

	switch (type) {
	case NOCT_VALUE_INT:	name = "int";		break;
	case NOCT_VALUE_LONG:	name = "long";		break;
	case NOCT_VALUE_FLOAT:	name = "float";		break;
	case NOCT_VALUE_DOUBLE:	name = "double";	break;
	case NOCT_VALUE_STRING:	name = "string";	break;
	case NOCT_VALUE_ARRAY:	name = "array";		break;
	case NOCT_VALUE_DICT:	name = "dict";		break;
	case NOCT_VALUE_PACKED:	name = "packed";	break;
	case NOCT_VALUE_FUNC:	name = "func";		break;
	default:
		name = "unknown";
		break;
	}

	/* Set the return value. */
	if (!noct_set_return_make_string(env, &ret, name))
		return false;

	noct_unpin_local(env, 2, &val, &ret);

	return true;
}

/*
 * Global.get(name)
 *
 * Returns the value of a global variable (including a function) by
 * name, or 0 when it is unset. Used by the Lisp compiler to grab a
 * freshly registered function.
 */
static bool
rt_intrin_Global_get(
	NoctEnv *env)
{
	NoctValue name, ret;
	const char *name_s;
	bool has_var;

	memset(&name, 0, sizeof(name));
	memset(&ret, 0, sizeof(ret));
	noct_pin_local(env, 2, &name, &ret);

	if (!noct_get_arg_check_string(env, 0, &name, &name_s))
		return false;
	if (!noct_check_global(env, name_s, &has_var))
		return false;
	if (!has_var) {
		if (!noct_set_return_make_int(env, &ret, 0))
			return false;
		noct_unpin_local(env, 2, &name, &ret);
		return true;
	}
	if (!noct_get_global(env, name_s, &ret))
		return false;
	if (!noct_set_return(env, &ret))
		return false;

	noct_unpin_local(env, 2, &name, &ret);
	return true;
}

static bool
rt_intrin_Global_hasVariable(
	NoctEnv *env)
{
	NoctValue name, ret;
	const char *name_s;
	bool has_var;

	noct_pin_local(env, 2, &name, &ret);

	/* Retrieve the argument "name" at the index 0. */
	if (!noct_get_arg_check_string(env, 0, &name, &name_s))
		return false;

	/* Check for the variable. */
	if (!noct_check_global(env, name_s, &has_var))
		return false;

	/* Set the return value. */
	if (!noct_set_return_make_int(env, &ret, has_var ? 1 : 0))
		return false;

	noct_unpin_local(env, 2, &name, &ret);

	return true;
}

/*
 * GC.youngGC()
 */
static bool
rt_intrin_GC_youngGC(
	NoctEnv *env)
{
	noct_fast_gc(env);
	return true;
}

/*
 * GC.oldGC()
 */
static bool
rt_intrin_GC_oldGC(
	NoctEnv *env)
{
	noct_full_gc(env);
	return true;
}

/*
 * GC.compactGC()
 */
static bool
rt_intrin_GC_compactGC(
	NoctEnv *env)
{
	noct_compact_gc(env);
	return true;
}
