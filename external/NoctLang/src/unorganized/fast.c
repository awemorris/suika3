/* -*- coding: utf-8; tab-width: 8; indent-tabs-mode: t; -*- */

/*
 * Noct Programming Language
 * Copyright (c) 2025, 2026, Awe Morris
 */

/*
 * Noct __fast function contracts.
 */

#include "fast.h"
#include "accel.h"

#include <ctype.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void
fast_signature_init(struct fast_signature *sig)
{
	uint32_t i;
	int j;

	memset(sig, 0, sizeof(*sig));
	sig->version = NOCT_FAST_SIGNATURE_VERSION;
	sig->return_type = -1;
	for (i = 0; i < NOCT_ARG_MAX; i++) {
		sig->param[i].value_type = -1;
		sig->param[i].packed_type = -1;
		for (j = 0; j < NOCT_FAST_RANK_MAX; j++) {
			sig->param[i].extent[j].kind = FAST_EXTENT_NONE;
			sig->param[i].extent[j].constant = 0;
			sig->param[i].extent[j].param_index = -1;
		}
	}
}

bool
fast_annotation_base(const char *annotation, char *base, size_t base_size,
		     bool *has_shape)
{
	const char *open;
	size_t len;

	if (annotation == NULL || base == NULL || base_size == 0)
		return false;
	open = strchr(annotation, '(');
	len = open != NULL ? (size_t)(open - annotation) : strlen(annotation);
	if (len == 0 || len >= base_size)
		return false;
	memcpy(base, annotation, len);
	base[len] = '\0';
	*has_shape = open != NULL;
	if (open != NULL) {
		size_t total;
		total = strlen(annotation);
		if (total < 3 || annotation[total - 1] != ')')
			return false;
	}
	return true;
}

static bool
fast_primitive_type(int type)
{
	return type == NOCT_VALUE_INT || type == NOCT_VALUE_LONG ||
	       type == NOCT_VALUE_FLOAT || type == NOCT_VALUE_DOUBLE;
}

static bool
fast_primitive_spelling(const char *name, int type)
{
	return (type == NOCT_VALUE_INT && strcmp(name, "int") == 0) ||
	       (type == NOCT_VALUE_LONG && strcmp(name, "long") == 0) ||
	       (type == NOCT_VALUE_FLOAT && strcmp(name, "float") == 0) ||
	       (type == NOCT_VALUE_DOUBLE && strcmp(name, "double") == 0);
}

static int
fast_find_param(uint32_t count, const char *const *name,
		const char *start, size_t len)
{
	uint32_t i;

	for (i = 0; i < count; i++) {
		if (strlen(name[i]) == len && strncmp(name[i], start, len) == 0)
			return (int)i;
	}
	return -1;
}

static bool
fast_parse_shape(struct fast_param_contract *contract,
		 uint32_t param_count, const char *const *param_name,
		 const int *param_type, const char *annotation,
		 char *error, size_t error_size)
{
	const char *p;
	const char *end;
	int rank;

	p = strchr(annotation, '(');
	if (p == NULL) {
		snprintf(error, error_size,
			 "A restricted packed __fast parameter requires an exact shape.");
		return false;
	}
	p++;
	end = strrchr(p, ')');
	if (end == NULL || end[1] != '\0' || p == end) {
		snprintf(error, error_size, "Invalid __fast parameter shape.");
		return false;
	}
	rank = 0;
	while (p < end) {
		const char *item;
		size_t len;
		struct fast_extent *extent;

		if (rank >= NOCT_FAST_RANK_MAX) {
			snprintf(error, error_size,
				 "A __fast parameter shape has more than 8 dimensions.");
			return false;
		}
		item = p;
		while (p < end && *p != ',') p++;
		len = (size_t)(p - item);
		if (len == 0) {
			snprintf(error, error_size, "Invalid empty __fast shape extent.");
			return false;
		}
		extent = &contract->extent[rank];
		if (isdigit((unsigned char)item[0])) {
			char number[32];
			char *tail;
			long long value;
			if (len >= sizeof(number)) {
				snprintf(error, error_size, "__fast shape extent is too large.");
				return false;
			}
			memcpy(number, item, len);
			number[len] = '\0';
			value = strtoll(number, &tail, 10);
			if (*tail != '\0' || value <= 0) {
				snprintf(error, error_size,
					 "A __fast shape extent must be positive.");
				return false;
			}
			extent->kind = FAST_EXTENT_CONST;
			extent->constant = (int64_t)value;
		} else {
			int index;
			size_t k;
			if (!(isalpha((unsigned char)item[0]) || item[0] == '_')) {
				snprintf(error, error_size, "Invalid __fast shape extent.");
				return false;
			}
			for (k = 1; k < len; k++) {
				if (!(isalnum((unsigned char)item[k]) || item[k] == '_')) {
					snprintf(error, error_size, "Invalid __fast shape extent.");
					return false;
				}
			}
			index = fast_find_param(param_count, param_name, item, len);
			if (index < 0 || (param_type[index] != NOCT_VALUE_INT &&
					  param_type[index] != NOCT_VALUE_LONG)) {
				snprintf(error, error_size,
					 "A dynamic __fast shape extent must name an int or long parameter.");
				return false;
			}
			extent->kind = FAST_EXTENT_PARAM;
			extent->param_index = index;
		}
		rank++;
		if (p < end) p++;
	}
	contract->rank = rank;
	return true;
}

bool
fast_signature_build(struct fast_signature *sig, int func_kind,
		     uint32_t param_count, const char *const *param_name,
		     const char *const *param_annotation,
		     const int *param_type, const int *param_packed_type,
		     const bool *param_restricted,
		     const char *return_annotation, int return_type,
		     char *error, size_t error_size)
{
	uint32_t i;

	fast_signature_init(sig);
	if (return_annotation != NULL && strchr(return_annotation, '(') != NULL) {
		snprintf(error, error_size, "A function return type cannot have a shape.");
		return false;
	}
	if (func_kind != NOCT_FUNC_FAST) {
		for (i = 0; i < param_count; i++) {
			if (param_annotation[i] != NULL &&
			    strchr(param_annotation[i], '(') != NULL) {
				snprintf(error, error_size,
					 "Shaped parameter types are valid only on __fast func.");
				return false;
			}
		}
		return true;
	}
	if (param_count > NOCT_ARG_MAX) {
		snprintf(error, error_size, "Too many __fast parameters.");
		return false;
	}
	if (return_annotation == NULL) {
		snprintf(error, error_size, "A __fast func must declare its return type.");
		return false;
	}
	if (!((return_type == NOCT_FAST_RETURN_VOID &&
	       strcmp(return_annotation, "void") == 0) ||
	      fast_primitive_spelling(return_annotation, return_type))) {
		snprintf(error, error_size,
			 "A __fast func return type must be void, int, long, float, or double.");
		return false;
	}
	sig->valid = true;
	sig->param_count = param_count;
	sig->return_type = return_type;
	for (i = 0; i < param_count; i++) {
		struct fast_param_contract *contract;
		bool has_shape;
		char base[64];

		if (param_annotation[i] == NULL) {
			snprintf(error, error_size,
				 "Every __fast func parameter requires a type annotation.");
			return false;
		}
		if (!fast_annotation_base(param_annotation[i], base,
					 sizeof(base), &has_shape)) {
			snprintf(error, error_size, "Invalid __fast parameter type.");
			return false;
		}
		contract = &sig->param[i];
		contract->value_type = param_type[i];
		contract->packed_type = param_packed_type[i];
		contract->restricted = param_restricted[i];
		if (fast_primitive_type(param_type[i]) && param_packed_type[i] < 0) {
			if (!fast_primitive_spelling(base, param_type[i])) {
				snprintf(error, error_size,
					 "A primitive __fast parameter must use int, long, float, or double exactly.");
				return false;
			}
			if (has_shape) {
				snprintf(error, error_size,
					 "A primitive __fast parameter cannot have a shape.");
				return false;
			}
			continue;
		}
		if (param_type[i] != NOCT_VALUE_PACKED ||
		    param_packed_type[i] < 0 || !param_restricted[i]) {
			snprintf(error, error_size,
				 "A __fast func parameter must be primitive or shaped rpacked.");
			return false;
		}
		if (!fast_parse_shape(contract, param_count, param_name,
				      param_type, param_annotation[i],
				      error, error_size))
			return false;
	}
	return true;
}

bool
fast_signature_equal(const struct fast_signature *a,
		     const struct fast_signature *b)
{
	return memcmp(a, b, sizeof(*a)) == 0;
}
