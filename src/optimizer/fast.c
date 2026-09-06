/* -*- coding: utf-8; tab-width: 8; indent-tabs-mode: t; -*- */

/*
 * Noct Programming Language
 * Copyright (c) 2025, 2026, Awe Morris
 */

/*
 * Noct optimizer __fast function contracts.
 */

#include <noct/noct.h>
#include <noct/executor.h>
#include "fast.h"
#include "runtime.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#define FAST_ANNOTATION_BASE_MAX	64

enum fast_decimal_result {
	FAST_DECIMAL_OK,
	FAST_DECIMAL_INVALID,
	FAST_DECIMAL_ZERO,
	FAST_DECIMAL_OVERFLOW
};

static void fast_set_error(char *error, size_t error_size, const char *message);
static bool fast_primitive_type(int type);
static bool fast_primitive_spelling(const char *name, int type);
static bool fast_packed_spelling(const char *name, int type);
static bool fast_identifier(const char *text, size_t length);
static int fast_find_param(uint32_t count, const char *const *name, const char *start, size_t length);
static int fast_parse_positive_decimal(const char *text, size_t length, int64_t *value);
static bool fast_parse_shape(struct fast_extent *extent, uint32_t *rank, uint32_t param_count, const char *const *param_name, const int *param_type, const char *annotation, char *error, size_t error_size);
static bool fast_build_param(struct fast_param_contract *contract, uint32_t param_count, const char *const *param_name, const int *param_type, const char *annotation, int value_type, int packed_type, bool restricted, char *error, size_t error_size);
static bool fast_signature_layout_valid(const struct fast_signature *signature);

/*
 * Initializes an empty signature.
 */
void
fast_signature_init(
	struct fast_signature *signature)
{
	assert(signature != NULL);

	signature->version = NOCT_FAST_SIGNATURE_VERSION;
	signature->valid = false;
	signature->param_count = 0;
	signature->param = NULL;
	signature->return_type = -1;
}

/*
 * Releases a signature and restores its empty state.
 */
void
fast_signature_free(
	struct fast_signature *signature)
{
	uint32_t i;

	if (signature == NULL)
		return;

	if (signature->param != NULL) {
		/* Release each exact-rank extent table. */
		for (i = 0; i < signature->param_count; i++) {
			noct_free(signature->param[i].extent);
			signature->param[i].extent = NULL;
		}

		noct_free(signature->param);
	}

	fast_signature_init(signature);
}

/*
 * Copies the base spelling before an optional shape.
 */
bool
fast_annotation_base(
	const char *annotation,
	char *base,
	size_t base_size,
	bool *has_shape)
{
	const char *open;
	const char *close;
	const char *first_close;
	size_t length;
	bool shaped;

	if (annotation == NULL)
		return false;
	if (base == NULL || base_size == 0)
		return false;
	if (has_shape == NULL)
		return false;

	open = strchr(annotation, '(');
	shaped = open != NULL;

	if (!shaped) {
		if (strchr(annotation, ')') != NULL)
			return false;

		length = strlen(annotation);
	} else {
		close = strrchr(open + 1, ')');
		if (close == NULL)
			return false;
		if (close[1] != '\0')
			return false;
		if (open + 1 == close)
			return false;
		if (strchr(open + 1, '(') != NULL)
			return false;

		first_close = strchr(annotation, ')');
		if (first_close != close)
			return false;

		length = (size_t)(open - annotation);
	}

	if (length == 0 || length >= base_size)
		return false;

	memcpy(base, annotation, length);
	base[length] = '\0';
	*has_shape = shaped;

	return true;
}

/*
 * Builds and validates a complete function contract.
 */
bool
fast_signature_build(
	struct fast_signature *signature,
	bool is_fast,
	uint32_t param_count,
	const char *const *param_name,
	const char *const *param_annotation,
	const int *param_type,
	const int *param_packed_type,
	const bool *param_restricted,
	const char *return_annotation,
	int return_type,
	char *error,
	size_t error_size)
{
	struct fast_signature candidate;
	uint32_t i;

	assert(signature != NULL);

	fast_set_error(error, error_size, "");

	if (param_count > NOCT_ARG_MAX) {
		if (is_fast) {
			fast_set_error(
				error,
				error_size,
				N_TR("Too many __fast parameters."));
		} else {
			fast_set_error(
				error,
				error_size,
				N_TR("Too many function parameters."));
		}
		return false;
	}

	if (return_annotation != NULL &&
	    strchr(return_annotation, '(') != NULL) {
		fast_set_error(
			error,
			error_size,
			N_TR("A function return type cannot have a shape."));
		return false;
	}

	if (param_count > 0 && param_annotation == NULL) {
		fast_set_error(
			error,
			error_size,
			N_TR("Invalid function parameter metadata."));
		return false;
	}

	if (!is_fast) {
		/* Reject shaped annotations outside a fast function. */
		for (i = 0; i < param_count; i++) {
			if (param_annotation[i] != NULL &&
			    strchr(param_annotation[i], '(') != NULL) {
				fast_set_error(
					error,
					error_size,
					N_TR("Shaped parameter types are valid only on __fast func."));
				return false;
			}
		}

		fast_signature_init(&candidate);
		fast_signature_free(signature);
		*signature = candidate;

		return true;
	}

	if (param_count > 0 &&
	    (param_name == NULL ||
	     param_type == NULL ||
	     param_packed_type == NULL ||
	     param_restricted == NULL)) {
		fast_set_error(
			error,
			error_size,
			N_TR("Invalid __fast signature metadata."));
		return false;
	}

	if (return_annotation == NULL) {
		fast_set_error(
			error,
			error_size,
			N_TR("A __fast func must declare its return type."));
		return false;
	}

	if (return_type == NOCT_FAST_RETURN_VOID) {
		if (strcmp(return_annotation, "void") != 0) {
			fast_set_error(
				error,
				error_size,
				N_TR("A __fast func return type must be void, int, long, float, or double."));
			return false;
		}
	} else if (!fast_primitive_spelling(return_annotation, return_type)) {
		fast_set_error(
			error,
			error_size,
			N_TR("A __fast func return type must be void, int, long, float, or double."));
		return false;
	}

	/* Validate every parameter name before allocating the candidate. */
	for (i = 0; i < param_count; i++) {
		if (param_name[i] == NULL) {
			fast_set_error(
				error,
				error_size,
				N_TR("Invalid __fast signature metadata."));
			return false;
		}
	}

	fast_signature_init(&candidate);
	candidate.param_count = param_count;
	candidate.return_type = return_type;

	if (param_count > 0) {
		candidate.param = noct_calloc(
			(size_t)param_count,
			sizeof(*candidate.param));
		if (candidate.param == NULL) {
			fast_set_error(
				error,
				error_size,
				N_TR("Out of memory while building __fast signature."));
			return false;
		}
	}

	/* Build every exact parameter contract. */
	for (i = 0; i < param_count; i++) {
		if (!fast_build_param(
			&candidate.param[i],
			param_count,
			param_name,
			param_type,
			param_annotation[i],
			param_type[i],
			param_packed_type[i],
			param_restricted[i],
			error,
			error_size)) {
			fast_signature_free(&candidate);
			return false;
		}
	}

	candidate.valid = true;

	fast_signature_free(signature);
	*signature = candidate;

	return true;
}

/*
 * Clones a signature without changing the destination on failure.
 */
bool
fast_signature_clone(
	struct fast_signature *destination,
	const struct fast_signature *source)
{
	struct fast_signature candidate;
	struct fast_param_contract *destination_param;
	const struct fast_param_contract *source_param;
	uint32_t i;

	assert(destination != NULL);
	assert(source != NULL);

	if (!fast_signature_layout_valid(source))
		return false;

	if (destination == source)
		return true;

	fast_signature_init(&candidate);
	candidate.version = source->version;
	candidate.param_count = source->param_count;
	candidate.return_type = source->return_type;

	if (source->param_count > 0) {
		candidate.param = noct_calloc(
			(size_t)source->param_count,
			sizeof(*candidate.param));
		if (candidate.param == NULL)
			return false;
	}

	/* Clone each parameter and its exact-rank extent table. */
	for (i = 0; i < source->param_count; i++) {
		destination_param = &candidate.param[i];
		source_param = &source->param[i];

		destination_param->value_type = source_param->value_type;
		destination_param->packed_type = source_param->packed_type;
		destination_param->restricted = source_param->restricted;
		destination_param->rank = source_param->rank;

		if (source_param->rank == 0)
			continue;

		destination_param->extent = noct_calloc(
			(size_t)source_param->rank,
			sizeof(*destination_param->extent));
		if (destination_param->extent == NULL) {
			fast_signature_free(&candidate);
			return false;
		}

		memcpy(
			destination_param->extent,
			source_param->extent,
			(size_t)source_param->rank *
				sizeof(*destination_param->extent));
	}

	candidate.valid = source->valid;

	fast_signature_free(destination);
	*destination = candidate;

	return true;
}

/*
 * Validates the complete in-memory signature representation.
 */
bool
fast_signature_valid(
	const struct fast_signature *signature)
{
	return fast_signature_layout_valid(signature);
}

/*
 * Clones a signature without exposing its layout to the owner.
 */
void *
fast_info_clone(
	const void *fast_info)
{
	const struct fast_signature *source;
	struct fast_signature *destination;

	if (fast_info == NULL)
		return NULL;

	source = fast_info;
	destination = noct_malloc(sizeof(*destination));
	if (destination == NULL)
		return NULL;

	fast_signature_init(destination);
	if (!fast_signature_clone(destination, source)) {
		noct_free(destination);
		return NULL;
	}

	return destination;
}

/*
 * Releases an opaque fast signature.
 */
void
fast_info_free(
	void *fast_info)
{
	struct fast_signature *signature;

	if (fast_info == NULL)
		return;

	signature = fast_info;
	fast_signature_free(signature);
	noct_free(signature);
}

/*
 * Returns the signature behind the optimizer-owned handle.
 */
const struct fast_signature *
fast_info_signature(
	const void *fast_info)
{
	return fast_info;
}

/*
 * Restores an optimized generated function through the AOT interface.
 */
bool
fast_mark_func(
	NoctFunc *func,
	uint32_t tmpvar_size,
	int return_type,
	uint32_t param_count,
	const int *value_type,
	const int *packed_type,
	const int *restricted,
	const uint32_t *rank,
	const int *extent_kind,
	const int64_t *extent_value)
{
	return fast_mark_runtime_func(
		(struct rt_func *)func,
		tmpvar_size,
		return_type,
		param_count,
		value_type,
		packed_type,
		restricted,
		rank,
		extent_kind,
		extent_value);
}

/*
 * Restores an optimized generated function's runtime contract.
 */
bool
fast_mark_runtime_func(
	struct rt_func *func,
	uint32_t tmpvar_size,
	int return_type,
	uint32_t param_count,
	const int *value_type,
	const int *packed_type,
	const int *restricted,
	const uint32_t *rank,
	const int *extent_kind,
	const int64_t *extent_value)
{
	struct fast_signature candidate;
	struct fast_param_contract *contract;
	struct fast_extent *extent;
	struct fast_signature *fast_info;
	uint32_t extent_count;
	uint32_t param_index;
	uint32_t i;
	uint32_t axis;

	if (func == NULL)
		return false;
	if (param_count != func->param_count || param_count > NOCT_ARG_MAX)
		return false;
	if (tmpvar_size < param_count + 1 || tmpvar_size > RT_TMPVAR_MAX)
		return false;
	if (param_count > 0 &&
	    (value_type == NULL ||
	     packed_type == NULL ||
	     restricted == NULL ||
	     rank == NULL)) {
		return false;
	}

	extent_count = 0;

	/* Validate and total every exact-rank extent table. */
	for (i = 0; i < param_count; i++) {
		if (restricted[i] != 0 && restricted[i] != 1)
			return false;
		if (rank[i] > NOCT_FAST_RANK_MAX)
			return false;
		if (extent_count > UINT32_MAX - rank[i])
			return false;

		extent_count += rank[i];
	}

	if (extent_count > 0 &&
	    (extent_kind == NULL || extent_value == NULL)) {
		return false;
	}

	fast_signature_init(&candidate);
	candidate.valid = true;
	candidate.param_count = param_count;
	candidate.return_type = return_type;

	if (param_count > 0) {
		candidate.param = noct_calloc(
			(size_t)param_count,
			sizeof(*candidate.param));
		if (candidate.param == NULL)
			return false;
	}

	extent_count = 0;

	/* Restore every parameter and its exact-rank extent table. */
	for (i = 0; i < param_count; i++) {
		contract = &candidate.param[i];
		contract->value_type = value_type[i];
		contract->packed_type = packed_type[i];
		contract->restricted = restricted[i] != 0;
		contract->rank = rank[i];

		if (rank[i] == 0)
			continue;

		contract->extent = noct_calloc(
			(size_t)rank[i],
			sizeof(*contract->extent));
		if (contract->extent == NULL) {
			fast_signature_free(&candidate);
			return false;
		}

		/* Decode this parameter's consecutive extent entries. */
		for (axis = 0; axis < rank[i]; axis++) {
			extent = &contract->extent[axis];
			extent->kind = extent_kind[extent_count];

			if (extent->kind == FAST_EXTENT_CONST) {
				extent->value.constant = extent_value[extent_count];
			} else if (extent->kind == FAST_EXTENT_PARAM) {
				if (extent_value[extent_count] < 0) {
					fast_signature_free(&candidate);
					return false;
				}

				param_index = (uint32_t)extent_value[extent_count];
				if ((int64_t)param_index != extent_value[extent_count]) {
					fast_signature_free(&candidate);
					return false;
				}
				extent->value.param_index = param_index;
			} else {
				fast_signature_free(&candidate);
				return false;
			}

			extent_count++;
		}
	}

	if (!fast_signature_valid(&candidate)) {
		fast_signature_free(&candidate);
		return false;
	}

	fast_info = noct_malloc(sizeof(*fast_info));
	if (fast_info == NULL) {
		fast_signature_free(&candidate);
		return false;
	}
	*fast_info = candidate;

	fast_info_free(func->fast_info);
	func->fast_info = fast_info;
	func->is_fast = true;
	func->tmpvar_size = tmpvar_size;
	func->return_type = return_type;
	func->return_packed_type = -1;

	/* Mirror the contract in ordinary optimizer metadata. */
	for (i = 0; i < param_count; i++) {
		func->param_type[i] = value_type[i];
		func->param_packed_type[i] = packed_type[i];
		func->param_restricted[i] = restricted[i] != 0;
	}

	return true;
}

/*
 * Validates an optimized entry contract against rooted arguments.
 */
bool
fast_check_runtime_call(
	struct rt_env *env,
	struct rt_func *func,
	uint32_t arg_count)
{
	const struct fast_signature *signature;
	const struct fast_param_contract *contract;
	const struct fast_extent *extent;
	struct rt_value *arguments;
	struct rt_value *argument;
	struct rt_value *extent_argument;
	struct rt_packed *packed;
	uint64_t extent_value;
	size_t element_count;
	uint32_t i;
	uint32_t axis;

	assert(env != NULL);
	assert(env->frame != NULL);
	assert(func != NULL);

	signature = fast_info_signature(func->fast_info);
	arguments = env->frame->tmpvar;
	if (signature == NULL ||
	    !signature->valid ||
	    signature->version != NOCT_FAST_SIGNATURE_VERSION ||
	    signature->param_count != arg_count ||
	    (arg_count > 0 && signature->param == NULL)) {
		rt_error(
			env,
			N_TR("Invalid __fast function signature for '%s'."),
			func->name);
		return false;
	}

	/* Validate every exact value tag and Packed element kind first. */
	for (i = 0; i < arg_count; i++) {
		contract = &signature->param[i];
		argument = &arguments[i];

		if (argument->type != contract->value_type) {
			rt_error(
				env,
				N_TR("__fast call '%s': argument %u has the wrong primitive type."),
				func->name,
				(unsigned int)i + 1);
			return false;
		}

		if (contract->value_type != NOCT_VALUE_PACKED)
			continue;

		packed = argument->val.packed;
		if (packed == NULL || packed->type != contract->packed_type) {
			rt_error(
				env,
				N_TR("__fast call '%s': argument %u has the wrong packed element type."),
				func->name,
				(unsigned int)i + 1);
			return false;
		}
	}

	/* Validate every Packed shape after all scalar tags are known valid. */
	for (i = 0; i < arg_count; i++) {
		contract = &signature->param[i];
		if (contract->value_type != NOCT_VALUE_PACKED)
			continue;

		if (contract->rank == 0 ||
		    contract->rank > NOCT_FAST_RANK_MAX ||
		    contract->extent == NULL) {
			rt_error(
				env,
				N_TR("Invalid __fast function signature for '%s'."),
				func->name);
			return false;
		}

		element_count = 1;

		/* Multiply every positive extent into the exact element count. */
		for (axis = 0; axis < contract->rank; axis++) {
			extent = &contract->extent[axis];
			if (extent->kind == FAST_EXTENT_CONST) {
				if (extent->value.constant <= 0) {
					rt_error(
						env,
						N_TR("__fast call '%s': shape extents must be positive."),
						func->name);
					return false;
				}

				extent_value = (uint64_t)extent->value.constant;
			} else if (extent->kind == FAST_EXTENT_PARAM) {
				if (extent->value.param_index >= arg_count) {
					rt_error(
						env,
						N_TR("Invalid __fast function signature for '%s'."),
						func->name);
					return false;
				}

				extent_argument = &arguments[extent->value.param_index];
				if (extent_argument->type == NOCT_VALUE_INT) {
					if (extent_argument->val.i <= 0) {
						rt_error(
							env,
							N_TR("__fast call '%s': shape extents must be positive."),
							func->name);
						return false;
					}

					extent_value =
						(uint64_t)(uint32_t)
							extent_argument->val.i;
				} else if (extent_argument->type == NOCT_VALUE_LONG) {
					if (extent_argument->val.l <= 0) {
						rt_error(
							env,
							N_TR("__fast call '%s': shape extents must be positive."),
							func->name);
						return false;
					}

					extent_value = (uint64_t)extent_argument->val.l;
				} else {
					rt_error(
						env,
						N_TR("Invalid __fast function signature for '%s'."),
						func->name);
					return false;
				}
			} else {
				rt_error(
					env,
					N_TR("Invalid __fast function signature for '%s'."),
					func->name);
				return false;
			}

			if (extent_value > (uint64_t)SIZE_MAX ||
			    element_count > SIZE_MAX / (size_t)extent_value) {
				rt_error(
					env,
					N_TR("__fast call '%s': shape element count overflow."),
					func->name);
				return false;
			}

			element_count *= (size_t)extent_value;
		}

		packed = arguments[i].val.packed;
		if (packed->elem_size != element_count) {
			rt_error(
				env,
				N_TR("__fast call '%s': argument %u does not match the exact shape."),
				func->name,
				(unsigned int)i + 1);
			return false;
		}
	}

	return true;
}

/* Copy one fixed diagnostic into the caller's buffer. */
static void
fast_set_error(
	char *error,
	size_t error_size,
	const char *message)
{
	size_t length;

	if (error == NULL || error_size == 0)
		return;

	length = strlen(message);
	if (length >= error_size)
		length = error_size - 1;

	memcpy(error, message, length);
	error[length] = '\0';
}

/* Test whether a runtime value tag is an allowed fast primitive. */
static bool
fast_primitive_type(
	int type)
{
	if (type == NOCT_VALUE_INT)
		return true;
	if (type == NOCT_VALUE_LONG)
		return true;
	if (type == NOCT_VALUE_FLOAT)
		return true;
	if (type == NOCT_VALUE_DOUBLE)
		return true;

	return false;
}

/* Match an exact primitive source spelling to its runtime tag. */
static bool
fast_primitive_spelling(
	const char *name,
	int type)
{
	if (type == NOCT_VALUE_INT && strcmp(name, "int") == 0)
		return true;
	if (type == NOCT_VALUE_LONG && strcmp(name, "long") == 0)
		return true;
	if (type == NOCT_VALUE_FLOAT && strcmp(name, "float") == 0)
		return true;
	if (type == NOCT_VALUE_DOUBLE && strcmp(name, "double") == 0)
		return true;

	return false;
}

/* Match an exact restricted Packed spelling to its element tag. */
static bool
fast_packed_spelling(
	const char *name,
	int type)
{
	if (type == NOCT_PACKED_INT8 && strcmp(name, "rpackedint8") == 0)
		return true;
	if (type == NOCT_PACKED_UINT8 && strcmp(name, "rpackeduint8") == 0)
		return true;
	if (type == NOCT_PACKED_INT16 && strcmp(name, "rpackedint16") == 0)
		return true;
	if (type == NOCT_PACKED_UINT16 && strcmp(name, "rpackeduint16") == 0)
		return true;
	if (type == NOCT_PACKED_INT32 && strcmp(name, "rpackedint32") == 0)
		return true;
	if (type == NOCT_PACKED_UINT32 && strcmp(name, "rpackeduint32") == 0)
		return true;
	if (type == NOCT_PACKED_INT64 && strcmp(name, "rpackedint64") == 0)
		return true;
	if (type == NOCT_PACKED_UINT64 && strcmp(name, "rpackeduint64") == 0)
		return true;
	if (type == NOCT_PACKED_FLOAT32 && strcmp(name, "rpackedfloat") == 0)
		return true;
	if (type == NOCT_PACKED_FLOAT64 && strcmp(name, "rpackeddouble") == 0)
		return true;

	return false;
}

/* Test whether one shape item is an ASCII identifier. */
static bool
fast_identifier(
	const char *text,
	size_t length)
{
	size_t i;
	char ch;

	if (length == 0)
		return false;

	ch = text[0];
	if (!((ch >= 'a' && ch <= 'z') ||
	      (ch >= 'A' && ch <= 'Z') ||
	      ch == '_')) {
		return false;
	}

	/* Check every remaining identifier character. */
	for (i = 1; i < length; i++) {
		ch = text[i];
		if (!((ch >= 'a' && ch <= 'z') ||
		      (ch >= 'A' && ch <= 'Z') ||
		      (ch >= '0' && ch <= '9') ||
		      ch == '_')) {
			return false;
		}
	}

	return true;
}

/* Find a parameter whose complete spelling matches one shape item. */
static int
fast_find_param(
	uint32_t count,
	const char *const *name,
	const char *start,
	size_t length)
{
	uint32_t i;

	/* Search every declared parameter. */
	for (i = 0; i < count; i++) {
		if (strlen(name[i]) != length)
			continue;
		if (strncmp(name[i], start, length) == 0)
			return (int)i;
	}

	return -1;
}

/* Parse one positive decimal with explicit signed-64-bit overflow checks. */
static int
fast_parse_positive_decimal(
	const char *text,
	size_t length,
	int64_t *value)
{
	uint64_t accumulated;
	uint64_t limit;
	uint64_t digit;
	size_t i;
	char ch;

	if (length == 0)
		return FAST_DECIMAL_INVALID;

	accumulated = 0;
	limit = (uint64_t)INT64_MAX;

	/* Accumulate every digit without invoking a libc conversion. */
	for (i = 0; i < length; i++) {
		ch = text[i];
		if (ch < '0' || ch > '9')
			return FAST_DECIMAL_INVALID;

		digit = (uint64_t)(unsigned int)(ch - '0');
		if (accumulated > (limit - digit) / 10U)
			return FAST_DECIMAL_OVERFLOW;

		accumulated = accumulated * 10U + digit;
	}

	if (accumulated == 0)
		return FAST_DECIMAL_ZERO;

	*value = (int64_t)accumulated;

	return FAST_DECIMAL_OK;
}

/* Parse and validate one exact shape into a fixed temporary table. */
static bool
fast_parse_shape(
	struct fast_extent *extent,
	uint32_t *rank,
	uint32_t param_count,
	const char *const *param_name,
	const int *param_type,
	const char *annotation,
	char *error,
	size_t error_size)
{
	struct fast_extent parsed[NOCT_FAST_RANK_MAX];
	const char *cursor;
	const char *close;
	const char *item;
	size_t length;
	uint32_t parsed_rank;
	int decimal_result;
	int param_index;
	int64_t constant;

	memset(parsed, 0, sizeof(parsed));

	cursor = strchr(annotation, '(');
	if (cursor == NULL) {
		fast_set_error(
			error,
			error_size,
			N_TR("A restricted packed __fast parameter requires an exact shape."));
		return false;
	}

	close = strrchr(cursor + 1, ')');
	if (close == NULL ||
	    close[1] != '\0' ||
	    cursor + 1 == close) {
		fast_set_error(
			error,
			error_size,
			N_TR("Invalid __fast parameter shape."));
		return false;
	}

	cursor++;
	parsed_rank = 0;

	/* Parse every comma-separated shape extent. */
	while (cursor < close) {
		if (parsed_rank >= NOCT_FAST_RANK_MAX) {
			fast_set_error(
				error,
				error_size,
				N_TR("A __fast parameter shape has more than 8 dimensions."));
			return false;
		}

		item = cursor;

		/* Find the end of this extent spelling. */
		while (cursor < close && *cursor != ',')
			cursor++;

		length = (size_t)(cursor - item);
		if (length == 0) {
			fast_set_error(
				error,
				error_size,
				N_TR("Invalid empty __fast shape extent."));
			return false;
		}

		if (item[0] >= '0' && item[0] <= '9') {
			decimal_result = fast_parse_positive_decimal(
				item,
				length,
				&constant);
			if (decimal_result == FAST_DECIMAL_OVERFLOW) {
				fast_set_error(
					error,
					error_size,
					N_TR("__fast shape extent is too large."));
				return false;
			}
			if (decimal_result == FAST_DECIMAL_ZERO) {
				fast_set_error(
					error,
					error_size,
					N_TR("A __fast shape extent must be positive."));
				return false;
			}
			if (decimal_result != FAST_DECIMAL_OK) {
				fast_set_error(
					error,
					error_size,
					N_TR("Invalid __fast shape extent."));
				return false;
			}

			parsed[parsed_rank].kind = FAST_EXTENT_CONST;
			parsed[parsed_rank].value.constant = constant;
		} else {
			if (!fast_identifier(item, length)) {
				fast_set_error(
					error,
					error_size,
					N_TR("Invalid __fast shape extent."));
				return false;
			}

			param_index = fast_find_param(
				param_count,
				param_name,
				item,
				length);
			if (param_index < 0) {
				fast_set_error(
					error,
					error_size,
					N_TR("A dynamic __fast shape extent must name an int or long parameter."));
				return false;
			}
			if (param_type[param_index] != NOCT_VALUE_INT &&
			    param_type[param_index] != NOCT_VALUE_LONG) {
				fast_set_error(
					error,
					error_size,
					N_TR("A dynamic __fast shape extent must name an int or long parameter."));
				return false;
			}

			parsed[parsed_rank].kind = FAST_EXTENT_PARAM;
			parsed[parsed_rank].value.param_index =
				(uint32_t)param_index;
		}

		parsed_rank++;

		if (cursor < close) {
			cursor++;
			if (cursor == close) {
				fast_set_error(
					error,
					error_size,
					N_TR("Invalid empty __fast shape extent."));
				return false;
			}
		}
	}

	memcpy(
		extent,
		parsed,
		(size_t)parsed_rank * sizeof(*extent));
	*rank = parsed_rank;

	return true;
}

/* Build one primitive or exact-shaped Packed parameter contract. */
static bool
fast_build_param(
	struct fast_param_contract *contract,
	uint32_t param_count,
	const char *const *param_name,
	const int *param_type,
	const char *annotation,
	int value_type,
	int packed_type,
	bool restricted,
	char *error,
	size_t error_size)
{
	struct fast_extent extent[NOCT_FAST_RANK_MAX];
	char base[FAST_ANNOTATION_BASE_MAX];
	uint32_t rank;
	bool has_shape;

	if (annotation == NULL) {
		fast_set_error(
			error,
			error_size,
			N_TR("Every __fast func parameter requires a type annotation."));
		return false;
	}

	if (!fast_annotation_base(
		annotation,
		base,
		sizeof(base),
		&has_shape)) {
		fast_set_error(
			error,
			error_size,
			N_TR("Invalid __fast parameter type."));
		return false;
	}

	if (fast_primitive_type(value_type)) {
		if (packed_type >= 0 || restricted) {
			fast_set_error(
				error,
				error_size,
				N_TR("A __fast func parameter must be primitive or shaped rpacked."));
			return false;
		}

		if (!fast_primitive_spelling(base, value_type)) {
			fast_set_error(
				error,
				error_size,
				N_TR("A primitive __fast parameter must use int, long, float, or double exactly."));
			return false;
		}

		if (has_shape) {
			fast_set_error(
				error,
				error_size,
				N_TR("A primitive __fast parameter cannot have a shape."));
			return false;
		}

		contract->value_type = value_type;
		contract->packed_type = -1;
		contract->restricted = false;
		contract->rank = 0;
		contract->extent = NULL;

		return true;
	}

	if (value_type != NOCT_VALUE_PACKED ||
	    packed_type < NOCT_PACKED_INT8 ||
	    packed_type > NOCT_PACKED_FLOAT64 ||
	    !restricted) {
		fast_set_error(
			error,
			error_size,
			N_TR("A __fast func parameter must be primitive or shaped rpacked."));
		return false;
	}

	if (!fast_packed_spelling(base, packed_type)) {
		fast_set_error(
			error,
			error_size,
			N_TR("A __fast func parameter must be primitive or shaped rpacked."));
		return false;
	}

	if (!has_shape) {
		fast_set_error(
			error,
			error_size,
			N_TR("A restricted packed __fast parameter requires an exact shape."));
		return false;
	}

	memset(extent, 0, sizeof(extent));
	rank = 0;
	if (!fast_parse_shape(
		extent,
		&rank,
		param_count,
		param_name,
		param_type,
		annotation,
		error,
		error_size)) {
		return false;
	}

	contract->extent = noct_calloc(
		(size_t)rank,
		sizeof(*contract->extent));
	if (contract->extent == NULL) {
		fast_set_error(
			error,
			error_size,
			N_TR("Out of memory while building __fast signature."));
		return false;
	}

	memcpy(
		contract->extent,
		extent,
		(size_t)rank * sizeof(*contract->extent));

	contract->value_type = value_type;
	contract->packed_type = packed_type;
	contract->restricted = true;
	contract->rank = rank;

	return true;
}

/* Validate an initialized signature before reading its dynamic tables. */
static bool
fast_signature_layout_valid(
	const struct fast_signature *signature)
{
	const struct fast_param_contract *contract;
	const struct fast_extent *extent;
	uint32_t i;
	uint32_t axis;

	if (signature == NULL)
		return false;
	if (signature->version != NOCT_FAST_SIGNATURE_VERSION)
		return false;

	if (!signature->valid) {
		if (signature->param_count != 0)
			return false;
		if (signature->param != NULL)
			return false;
		if (signature->return_type != -1)
			return false;

		return true;
	}

	if (signature->param_count > NOCT_ARG_MAX)
		return false;
	if (signature->param_count == 0 && signature->param != NULL)
		return false;
	if (signature->param_count > 0 && signature->param == NULL)
		return false;

	if (signature->return_type != NOCT_FAST_RETURN_VOID &&
	    !fast_primitive_type(signature->return_type)) {
		return false;
	}

	/* Validate every parameter and its exact-rank table. */
	for (i = 0; i < signature->param_count; i++) {
		contract = &signature->param[i];

		if (fast_primitive_type(contract->value_type)) {
			if (contract->packed_type != -1)
				return false;
			if (contract->restricted)
				return false;
			if (contract->rank != 0)
				return false;
			if (contract->extent != NULL)
				return false;

			continue;
		}

		if (contract->value_type != NOCT_VALUE_PACKED)
			return false;
		if (contract->packed_type < NOCT_PACKED_INT8 ||
		    contract->packed_type > NOCT_PACKED_FLOAT64) {
			return false;
		}
		if (!contract->restricted)
			return false;
		if (contract->rank == 0 ||
		    contract->rank > NOCT_FAST_RANK_MAX) {
			return false;
		}
		if (contract->extent == NULL)
			return false;

		/* Validate every meaningful extent. */
		for (axis = 0; axis < contract->rank; axis++) {
			extent = &contract->extent[axis];

			if (extent->kind == FAST_EXTENT_CONST) {
				if (extent->value.constant <= 0)
					return false;
			} else if (extent->kind == FAST_EXTENT_PARAM) {
				if (extent->value.param_index >=
				    signature->param_count) {
					return false;
				}
				if (signature->param[
					extent->value.param_index].value_type !=
					NOCT_VALUE_INT &&
				    signature->param[
					extent->value.param_index].value_type !=
					NOCT_VALUE_LONG) {
					return false;
				}
			} else {
				return false;
			}
		}
	}

	return true;
}
